// Test of the engine's interface (engine.h): the corpus coded and decoded through it, with storage laid out as the
// engine's (32 frame slots, a ring of 32 x 256 entity states on each side, rings of 256 commands of 1,024 bytes, server
// baselines inside a larger structure). The coded bytes must be those of netcodec.roundtrip (same hash: both runs,
// as recorded and with every zero float as -0.0), and every decoded message must equal the original.
// The server follows the engine: a message's commands are in its ring before the message is built, a message whose
// base it does not hold is not coded (as in netcodec.roundtrip), and only operations in the engine's order are sent.
// Usage: netcodec_test_engine [--hash=0x...] <trace.bin> ...   (captures.bin, holdout.bin)
#include "demo_trace.h"
#include "fnv.h"
#include "message_compare.h"
#include "negate_zeros.h"
#include "netcodec/engine.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace netcodec;
namespace eng = netcodec::engine;

namespace {

constexpr uint32_t kRingStates = eng::kPacketBackup * eng::kMaxSnapshotEntities;
constexpr uint32_t kCommandBytes = 1024;      // MAX_STRING_CHARS
constexpr size_t kServerBaselineStride = 600; // sizeof(svEntity_t) is larger than an entityState_t
constexpr size_t kMaxMsgLen = 49152;          // MAX_MSGLEN
constexpr uint32_t kMaxConfigStrings = 1700;  // MAX_CONFIGSTRINGS

[[nodiscard]] uint32_t SlotOf(uint32_t seq) noexcept { return seq % eng::kPacketBackup; }

// the engine-like storage of one side
struct Side {
	std::vector<eng::Frame> frames = std::vector<eng::Frame>(eng::kPacketBackup, eng::Frame{ .seq = -1, .ps = nullptr, .vps = nullptr,
	                                                                                        .firstEntity = 0, .numEntities = 0, .record = {} });
	std::vector<PlayerState> ps = std::vector<PlayerState>(eng::kPacketBackup);
	std::vector<PlayerState> vps = std::vector<PlayerState>(eng::kPacketBackup);
	std::vector<EntityState> ring = std::vector<EntityState>(kRingStates);
	int32_t nextEntity = 0;
	std::vector<char> commands = std::vector<char>(size_t{ eng::kMinCommands } * kCommandBytes);
	int32_t lastCommand = 0;

	[[nodiscard]] eng::EntityRing Ring() const noexcept { return eng::EntityRing{ .states = ring.data(), .count = kRingStates }; }
	[[nodiscard]] eng::CommandRing Commands() const noexcept {
		return eng::CommandRing{ .texts = commands.data(), .stride = kCommandBytes, .count = eng::kMinCommands, .last = lastCommand };
	}
	// stores command seq (the engine: only newer ones); false if it does not fit
	bool AddCommand(int64_t seq, const std::string &text) {
		if (seq <= lastCommand) return true;
		if (text.size() >= kCommandBytes) return false;
		const std::span<char> slot = std::span<char>(commands).subspan((static_cast<size_t>(seq) % eng::kMinCommands) * kCommandBytes, kCommandBytes);
		std::ranges::fill(slot, '\0');
		std::ranges::copy(text, slot.begin());
		lastCommand = static_cast<int32_t>(seq);
		return true;
	}
};

struct Totals {
	size_t messages = 0, skipped = 0, encodeFailures = 0, decodeFailures = 0, orderFailures = 0, bytes = 0;
	test::Fnv hash;
};

// the engine's order: commands, then at most one each of gamestate, setgame, snapshot, download, map change
[[nodiscard]] bool EngineOrder(const Message &m) {
	static constexpr std::array<OpType, 5> kOrder{ OpType::GameState, OpType::SetGame, OpType::Snapshot, OpType::Download, OpType::MapChange };
	size_t k = 0;
	bool commands = true;
	for (const Op &op : m.ops) {
		if (op.type == OpType::Command) {
			if (!commands) return false;
			continue;
		}
		commands = false;
		while (k < kOrder.size() && kOrder[k] != op.type) k++;
		if (k == kOrder.size()) return false;
		k++;
	}
	return true;
}

void RoundTrip(const trace::DemoStream &s, const char *label, Totals &t) {
	Side server, client;
	std::vector<uint8_t> serverBaselines(((kMaxEntities - 1) * kServerBaselineStride) + kEntityBytes);
	trace::BaselineData clientBaselines;
	eng::ServerCodec enc;
	eng::ClientCodec dec;
	std::vector<uint8_t> out(kMaxMsgLen);
	std::vector<const char *> configStrings;
	Message dm;
	for (const trace::DemoMessage &m : s.messages) {
		const auto seq = static_cast<int32_t>(m.seq);
		// the server's commands, before the message is built
		int32_t firstCommand = -1;
		bool ok = EngineOrder(m.msg);
		for (const Op &op : m.msg.ops)
			if (op.type == OpType::Command) {
				if (firstCommand < 0) firstCommand = static_cast<int32_t>(op.seq);
				ok = ok && server.AddCommand(op.seq, op.text);
			}
		eng::Frame &sf = server.frames[SlotOf(m.seq)];
		sf.seq = -1;
		// a message whose base the server does not hold is not coded (netcodec.roundtrip does the same)
		if (m.delta != 0 && (m.delta >= eng::kPacketBackup || server.frames[SlotOf(m.seq - m.delta)].seq != seq - static_cast<int32_t>(m.delta))) {
			t.skipped++;
			continue;
		}
		t.messages++;
		if (!ok) {
			if (t.orderFailures++ < 5) std::fprintf(stderr, "not in the engine's order: %s %s seq %u\n", s.Meta(1).c_str(), label, m.seq);
			continue;
		}

		eng::ServerMessage sm{};
		sm.seq = seq;
		sm.ack = static_cast<int32_t>(m.msg.ack);
		sm.firstCommand = firstCommand >= 0 ? firstCommand : server.lastCommand + 1;
		for (const Op &op : m.msg.ops) {
			switch (op.type) {
			case OpType::GameState: {
				const GameState &gs = op.gameState;
				sm.gameState = true;
				sm.relSeq = static_cast<int32_t>(gs.relSeq);
				configStrings.assign(kMaxConfigStrings, nullptr);
				for (const ConfigString &cs : gs.configStrings) configStrings.at(cs.index) = cs.text.c_str();
				sm.configStrings = configStrings.data();
				sm.numConfigStrings = kMaxConfigStrings;
				sm.clientNum = static_cast<int32_t>(gs.clientNum);
				sm.checksum = static_cast<int32_t>(gs.checksum);
				for (uint32_t n = 0; n < kMaxEntities; n++)
					std::ranges::copy(gs.baselines[n], serverBaselines.begin() + static_cast<std::ptrdiff_t>(n * kServerBaselineStride));
				break;
			}
			case OpType::SetGame: sm.setGame = op.text.c_str(); break;
			case OpType::Snapshot: {
				const uint32_t slot = SlotOf(m.seq);
				std::ranges::copy(op.snap.ps, server.ps[slot].begin());
				const bool vehicle = !op.snap.vps.empty();
				if (vehicle) std::ranges::copy(op.snap.vps, server.vps[slot].begin());
				for (uint32_t i = 0; i < op.snap.entities.count; i++)
					std::ranges::copy(op.snap.entities[i], server.ring[static_cast<uint32_t>(server.nextEntity + static_cast<int32_t>(i)) % kRingStates].begin());
				sm.snapshot = true;
				sm.delta = m.delta;
				sm.time = static_cast<int32_t>(op.time);
				sm.flags = op.flags;
				sm.areas = op.areas.data();
				sm.numAreas = ToU32(op.areas.size());
				sm.frame = eng::Frame{ .seq = seq, .ps = server.ps[slot].data(), .vps = vehicle ? server.vps[slot].data() : nullptr,
				                       .firstEntity = server.nextEntity, .numEntities = op.snap.entities.count, .record = {} };
				server.nextEntity += static_cast<int32_t>(op.snap.entities.count);
				break;
			}
			case OpType::Download:
				sm.download = true;
				sm.block = op.block;
				sm.fileSize = static_cast<int32_t>(op.fileSize);
				sm.data = op.data.data();
				sm.dataSize = ToU32(op.data.size());
				break;
			case OpType::MapChange: sm.mapChange = true; break;
			case OpType::Command:
			case OpType::Nop: break;
			}
		}
		uint32_t delta = 0;
		eng::Record rec{};
		const size_t n = enc.Encode(sm, server.frames.data(), server.Ring(), server.nextEntity, server.Commands(),
		                            eng::Baselines{ .bytes = serverBaselines.data(), .stride = kServerBaselineStride }, out.data(),
		                            out.size(), delta, rec);
		if (n == 0 || delta != m.delta) {
			if (t.encodeFailures++ < 5) std::fprintf(stderr, "encode failure: %s %s seq %u (delta %u, asked %u)\n", s.Meta(1).c_str(), label, m.seq, delta, m.delta);
			continue;
		}
		if (sm.snapshot) {
			sf = sm.frame;
			sf.record = rec;
		}
		const std::span<const uint8_t> bytes(out.data(), n);
		t.bytes += n;
		t.hash.Feed(bytes);

		// the client
		const uint32_t slot = SlotOf(m.seq);
		const eng::ClientTarget target{ .ps = client.ps[slot].data(), .vps = client.vps[slot].data(), .ring = client.ring.data(),
		                                .ringCount = kRingStates, .firstEntity = client.nextEntity,
		                                .baselines = clientBaselines.table.data(), .baselineStride = kEntityBytes };
		eng::ClientMessage cm{};
		eng::Frame &cf = client.frames[slot];
		cf.seq = -1;
		const bool decoded = dec.Decode(bytes.data(), bytes.size(), seq, client.frames.data(), client.Ring(), client.Commands(), target, cm);
		bool same = decoded && cm.delta == m.delta;
		if (decoded) {
			// the decoded message as the codec's Message, over the client's storage, for the comparison
			dm.ack = cm.ack;
			dm.ops.clear();
			for (const eng::Command &c : std::span<const eng::Command>(cm.commands, cm.numCommands)) {
				Op &op = dm.ops.emplace_back();
				op.type = OpType::Command;
				op.seq = c.seq;
				op.text.assign(c.text, c.length);
				same = same && client.AddCommand(c.seq, op.text);
			}
			if (cm.gameState) {
				Op &op = dm.ops.emplace_back();
				op.type = OpType::GameState;
				op.gameState.relSeq = cm.relSeq;
				op.gameState.clientNum = cm.clientNum;
				op.gameState.checksum = cm.checksum;
				for (const eng::ConfigString &cs : std::span<const eng::ConfigString>(cm.configStrings, cm.numConfigStrings))
					op.gameState.configStrings.push_back(ConfigString{ .index = static_cast<uint16_t>(cs.index), .text = std::string(cs.text, cs.length) });
				op.gameState.baselines = clientBaselines.View();
			}
			if (cm.setGame != nullptr) {
				Op &op = dm.ops.emplace_back();
				op.type = OpType::SetGame;
				op.text = cm.setGame;
			}
			if (cm.snapshot) {
				Op &op = dm.ops.emplace_back();
				op.type = OpType::Snapshot;
				op.time = cm.time;
				op.flags = cm.flags;
				const std::span<const uint8_t> areas(cm.areas, cm.numAreas);
				op.areas.assign(areas.begin(), areas.end());
				op.snap = Snapshot{ .ps = client.ps[slot],
				                    .vps = cm.vehicle ? std::span<const uint8_t>(client.vps[slot]) : std::span<const uint8_t>(),
				                    .entities = EntityRing{ .ring = trace::Bytes(client.ring),
				                                            .first = static_cast<uint32_t>(client.nextEntity) % kRingStates,
				                                            .count = cm.numEntities } };
				cf = eng::Frame{ .seq = seq, .ps = client.ps[slot].data(), .vps = cm.vehicle ? client.vps[slot].data() : nullptr,
				                 .firstEntity = client.nextEntity, .numEntities = cm.numEntities, .record = cm.record };
				client.nextEntity += static_cast<int32_t>(cm.numEntities);
			}
			if (cm.download) {
				Op &op = dm.ops.emplace_back();
				op.type = OpType::Download;
				op.block = cm.block;
				op.fileSize = cm.fileSize;
				const std::span<const uint8_t> data(cm.data, cm.dataSize);
				op.data.assign(data.begin(), data.end());
			}
			if (cm.mapChange) dm.ops.emplace_back().type = OpType::MapChange;
			same = same && trace::SameMessage(dm, m.msg);
		}
		if (!same && t.decodeFailures++ < 5)
			std::fprintf(stderr, "round trip failure: %s %s seq %u%s\n", s.Meta(1).c_str(), label, m.seq, decoded ? "" : " (decoder failed)");
	}
}

} // namespace

int main(int argc, char **argv) {
	int first = 1;
	std::optional<uint64_t> expected;
	if (argc > 1 && std::strncmp(argv[1], "--hash=", 7) == 0) expected = std::strtoull(argv[first++] + 7, nullptr, 16);
	if (argc <= first) { std::fprintf(stderr, "usage: %s [--hash=0x...] trace.bin ...\n", argv[0]); return 2; }
	Totals recorded, negated;
	size_t streams = 0;
	for (int a = first; a < argc; a++) {
		trace::DemoTraceFile in(argv[a]);
		if (!in.Ok()) return 2;
		trace::DemoStream s;
		while (in.Next(s)) {
			RoundTrip(s, "", recorded);
			NegateZeros(s);
			RoundTrip(s, "(-0.0)", negated);
			streams++;
		}
		if (!in.Ok()) return 2;
	}
	size_t failures = 0;
	for (const auto &[label, t] : { std::pair<const char *, const Totals &>{ "recorded", recorded },
	                                std::pair<const char *, const Totals &>{ "zero floats as -0.0", negated } }) {
		std::printf("engine interface, %s: %zu streams, %zu messages (%zu without a held base), %zu bytes: order failures %zu, "
		            "encode failures %zu, decode failures %zu\n",
		            label, streams, t.messages, t.skipped, t.bytes, t.orderFailures, t.encodeFailures, t.decodeFailures);
		failures += t.orderFailures + t.encodeFailures + t.decodeFailures;
	}
	test::Fnv both;
	both.Feed(recorded.hash.h, 8);
	both.Feed(negated.hash.h, 8);
	std::printf("hash of the coded bytes 0x%016" PRIX64, both.h);
	if (expected) std::printf(", expected 0x%016" PRIX64 "%s", *expected, *expected == both.h ? "" : " (MISMATCH)");
	std::printf("\n");
	const bool ok = (!expected || *expected == both.h) && failures == 0;
	return ok && recorded.messages != 0 ? 0 : 1;
}
