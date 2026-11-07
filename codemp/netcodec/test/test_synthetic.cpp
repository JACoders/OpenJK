// Synthetic round trip of the message codec (C++ port of docs_rewrite/netcode/tools/python/fuzz.py, extended):
// structurally valid server messages with every operation (gamestates, commands, setgame, downloads, snapshots, map
// changes, nops), vehicles and pilot player states, extreme field values (any width, NaNs, infinities, -0.0,
// denormals), structured and random command texts, base ages from 1 to 255 and messages without a base. Every
// message is coded by the server side and decoded by a client that keeps its own decoded snapshots, and must equal
// the original field by field (with the header record). Says nothing about compression: the trained tables code
// these values at a high cost, which is the point (every symbol of every alphabet must code).
// A state is derived from the state the codec codes it against (the previous state, the baseline or the zero state),
// with only fields of the table the codec sends changed, each to a value of its codec width, as the engine's states
// are (the decoder copies the rest from the same base).
// The generator uses only integer arithmetic and exact float conversions, so every compiler generates the same
// messages: with --hash=<value>, the hash (FNV-1a 64) of the coded bytes must equal it.
// Usage: netcodec_test_synthetic [--hash=0x...] [streams] [messages per stream] [seed]
#include "demo_trace.h"
#include "fnv.h"
#include "message_compare.h"
#include "numeric.h"
#include "schema.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

using namespace netcodec;
using namespace netcodec::detail;

namespace {

// splitmix64: the same sequence on every platform (the standard distributions are not)
class Rng {
public:
	explicit Rng(uint64_t seed) noexcept : s_(seed) {}
	uint64_t Next() noexcept {
		uint64_t z = (s_ += 0x9E3779B97F4A7C15ULL);
		z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31U);
	}
	uint64_t Below(uint64_t n) noexcept { return Next() % n; }   // n > 0; the modulo bias does not matter here
	// an index below n (< 2^32), from the high 32 bits: no cast between uint64_t and size_t (the same type on some targets)
	size_t Index(size_t n) noexcept { return static_cast<uint32_t>(Next() >> 32U) % n; }
	int64_t Range(int64_t lo, int64_t hi) noexcept { return lo + static_cast<int64_t>(Below(static_cast<uint64_t>(hi - lo) + 1)); }
	bool PerMille(uint64_t p) noexcept { return Below(1000) < p; }

private:
	uint64_t s_;
};

constexpr uint32_t kEfNodraw = 1U << 8U;

// a value of a codec width (bits != 0), as the codec reads it back
int64_t RandInt(Rng &r, int bits) {
	const auto ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const int64_t m = int64_t{ 1 } << ab;
	const int64_t lo = bits < 0 ? -m / 2 : 0, hi = bits < 0 ? (m / 2) - 1 : m - 1;
	const uint64_t p = r.Below(1000);
	if (p < 200) return 0;
	if (p < 400) return std::clamp<int64_t>(r.Range(-8, 8), lo, hi);
	if (p < 500) return r.Below(2) != 0 ? lo : hi;
	if (p < 700) return std::clamp<int64_t>(r.Range(-300, 300), lo, hi);
	return r.Range(lo, hi);
}

uint32_t RandFloat(Rng &r) {
	static constexpr std::array<uint32_t, 9> kSpecial{ 0x80000000U, 0x7F800000U, 0xFF800000U, 0x7FC00000U, 0xFFC00001U,
	                                                   0x00000001U, 0x80000001U, 0x7F7FFFFFU, 0x00800000U };
	const uint64_t p = r.Below(1000);
	if (p < 150) return 0;
	if (p < 350) return Pack(static_cast<double>(r.Range(-4100, 4100)));
	if (p < 550) return Pack(static_cast<double>(r.Range(-40000, 40000)) * kAngleStep);
	if (p < 600) return Pack(static_cast<double>(r.Range(-32768, 32767)) * kAngleStep);
	if (p < 650) return Pack(65536 * kAngleStep);
	if (p < 700) return kSpecial[r.Index(kSpecial.size())];
	if (p < 850) return Pack(static_cast<double>(r.Range(-9000 * 1024, 9000 * 1024)) / 1024);
	return static_cast<uint32_t>(r.Next());
}

template <size_t N> void Mutate(Rng &r, std::array<uint8_t, N> &img, std::span<const FieldDesc> fields, uint64_t count) {
	for (uint64_t k = 0; k < count; k++) {
		const FieldDesc &f = fields[r.Index(fields.size())];
		if (f.bits == 0) StoreWord(img, f.offset, RandFloat(r));
		else Store(img, f, RandInt(r, f.bits));
	}
}

const FieldDesc &FieldAt(std::span<const FieldDesc> fields, uint32_t offset) {
	for (const FieldDesc &f : fields)
		if (f.offset == offset) return f;
	std::fprintf(stderr, "no field at offset %u\n", offset);
	std::exit(2);
}

std::string RandBytes(Rng &r, size_t maxLen, uint32_t lo, uint32_t hi) {
	std::string s(r.Index(maxLen + 1), ' ');
	for (char &c : s) c = static_cast<char>(r.Range(lo, hi));
	return s;
}

// a server command: random bytes, or a verb and tokens (numbers near earlier ones, text, empty tokens) so that
// commands are often coded against an earlier one; sometimes at the engine's length limit
std::string RandCommand(Rng &r) {
	static constexpr std::array<const char *, 6> kVerbs{ "cs", "print", "scores", "tinfo", "chat", "cp" };
	const uint64_t p = r.Below(1000);
	if (p < 350) return RandBytes(r, 80, 1, 255);
	if (p < 360) return RandBytes(r, 1022, 1, 255);
	std::string s = kVerbs[r.Index(kVerbs.size())];
	const uint64_t n = r.Below(12);
	for (uint64_t i = 0; i < n; i++) {
		s.push_back(' ');
		const uint64_t q = r.Below(100);
		if (q < 45) s += std::to_string(r.Range(-20, 20) + (static_cast<int64_t>(i) * 100));
		else if (q < 55) s += std::to_string(r.Range(-999999999999999999LL, 999999999999999999LL));
		else if (q < 60) s += q < 58 ? "-0" : "007";   // not canonical numbers: sent as text
		else if (q < 65) {}                            // empty token (two spaces in a row)
		else s += RandBytes(r, 12, 33, 126);
	}
	return s;
}

std::vector<Entity> RandBaselines(Rng &r) {
	// a map, not sort and unique: which duplicate an unstable sort keeps differs between standard libraries
	std::map<uint16_t, EntityState> byNumber;
	const uint64_t n = r.Below(31);
	for (uint64_t k = 0; k < n; k++) {
		const auto num = static_cast<uint16_t>(r.Below(kMaxEntities));
		EntityState st{};
		Mutate(r, st, kEntityFields, 1 + r.Below(8));
		byNumber.emplace(num, st);   // the first one wins
	}
	std::vector<Entity> out;
	for (const auto &[num, st] : byNumber) out.push_back(Entity{ .number = num, .state = st });
	return out;
}

const Entity *Find(const std::vector<Entity> &list, uint32_t num) {
	const auto it = std::ranges::lower_bound(list, num, {}, &Entity::number);
	return it != list.end() && it->number == num ? &*it : nullptr;
}

// a snapshot derived from the base the client holds (nullptr: none)
Snapshot NextSnapshot(Rng &r, const Snapshot *base, const std::vector<Entity> &baselines) {
	Snapshot sn;
	const bool veh = r.PerMille(250);
	const bool pilot = veh && r.PerMille(600);
	sn.ps = base != nullptr ? base->ps : PlayerState{};
	const std::span<const FieldDesc> table = pilot ? std::span<const FieldDesc>(kPilotFields) : std::span<const FieldDesc>(kPlayerFields);
	Mutate(r, sn.ps, table, r.Below(26));
	Store(sn.ps, FieldAt(table, kVehicleNumOffset), veh ? r.Range(1, 1023) : 0);
	const FieldDesc &ef = FieldAt(table, kPlayerEFlagsOffset);
	const auto flags = static_cast<uint32_t>(Value(sn.ps, ef));
	Store(sn.ps, ef, pilot ? (flags | kEfNodraw) : veh ? (flags & ~kEfNodraw) : flags);
	sn.hasVehicle = veh;
	if (veh) {
		sn.vps = base != nullptr && base->hasVehicle ? base->vps : PlayerState{};
		Mutate(r, sn.vps, kVehicleFields, r.Below(21));
	}
	std::map<uint16_t, EntityState> ents;
	if (base != nullptr)
		for (const Entity &e : base->entities) {
			const uint64_t p = r.Below(1000);
			if (p < 100) continue;
			EntityState st = e.state;
			if (p < 450) Mutate(r, st, kEntityFields, 1 + r.Below(10));
			ents.emplace(e.number, st);
		}
	const uint64_t added = r.Below(26);
	for (uint64_t k = 0; k < added; k++) {
		const auto num = static_cast<uint16_t>(r.Below(kMaxEntities));
		if (ents.contains(num) || (base != nullptr && Find(base->entities, num) != nullptr)) continue;
		const Entity *b = Find(baselines, num);
		EntityState st = b != nullptr ? b->state : EntityState{};
		if (b == nullptr || !r.PerMille(150)) Mutate(r, st, kEntityFields, 1 + r.Below(20));
		ents.emplace(num, st);
	}
	for (const auto &[num, st] : ents) sn.entities.push_back(Entity{ .number = num, .state = st });
	return sn;
}

struct Stats {
	size_t messages = 0, failures = 0, bytes = 0, maxBytes = 0, snapshots = 0, vehicle = 0, pilot = 0, noBase = 0,
	       baseOld = 0, gamestates = 0, commands = 0, downloads = 0, entities = 0;
	uint32_t maxDelta = 0;
	test::Fnv hash;
};

struct ServerStored {
	Snapshot snap;
	HeaderRecord header;
};

void RunStream(uint64_t seed, size_t nmsg, Stats &st) {
	Rng r(seed);
	MessageEncoder enc;
	std::vector<uint8_t> out;
	std::map<uint32_t, ServerStored> server;
	trace::ClientStore client;
	trace::CommandLog serverHistory, clientHistory;
	std::vector<Entity> serverBaselines, clientBaselines;
	Message dm;
	int64_t cmdSeq = 0;
	for (uint32_t seq = 100; seq < 100 + nmsg + 1; seq++) {
		Message msg;
		msg.ack = r.Range(-(int64_t{ 1 } << 31), (int64_t{ 1 } << 31) - 1);
		auto gameState = [&] {
			Op op;
			op.type = OpType::GameState;
			GameState &gs = op.gameState;
			gs.relSeq = r.Range(0, 5000);
			std::map<uint16_t, std::string> cs;
			const uint64_t ncs = r.Below(41);
			for (uint64_t k = 0; k < ncs; k++) {
				const auto index = static_cast<uint16_t>(r.Below(1700));
				cs.emplace(index, RandBytes(r, 40, 1, 255));
			}
			for (auto &[index, text] : cs) gs.configStrings.push_back(ConfigString{ .index = index, .text = std::move(text) });
			gs.baselines = RandBaselines(r);
			gs.clientNum = r.Range(0, 31);
			gs.checksum = r.Range(-(int64_t{ 1 } << 31), (int64_t{ 1 } << 31) - 1);
			msg.ops.push_back(std::move(op));
		};
		uint32_t delta = 0;
		if (seq == 100) {
			gameState();
		} else {
			const uint64_t ncmd = std::array<uint64_t, 6>{ 0, 0, 0, 1, 2, 4 }[r.Index(6)];
			for (uint64_t k = 0; k < ncmd; k++) {
				Op op;
				op.type = OpType::Command;
				op.seq = ++cmdSeq;
				op.text = RandCommand(r);
				msg.ops.push_back(std::move(op));
			}
			if (r.PerMille(5)) gameState();
			if (r.PerMille(20)) { Op op; op.type = OpType::SetGame; op.text = RandBytes(r, 20, 1, 255); msg.ops.push_back(std::move(op)); }
			if (r.PerMille(10)) { Op op; op.type = OpType::Nop; msg.ops.push_back(std::move(op)); }
			if (r.PerMille(850)) {
				// base: usually 1 to 11 messages back, sometimes up to 255, sometimes none
				std::vector<uint32_t> cand;
				const uint32_t maxAge = r.PerMille(20) ? 255 : 11;
				for (uint32_t d = 1; d <= maxAge && d < seq; d++)
					if (server.contains(seq - d)) cand.push_back(d);
				delta = !cand.empty() && !r.PerMille(80) ? cand[r.Index(cand.size())] : 0;
				const Snapshot *base = delta != 0 ? &server.at(seq - delta).snap : nullptr;
				const std::vector<Entity> *bl = &serverBaselines;
				for (const Op &o : msg.ops)
					if (o.type == OpType::GameState) bl = &o.gameState.baselines;
				Op op;
				op.type = OpType::Snapshot;
				op.time = r.Range(0, 10000000);
				op.flags = static_cast<uint32_t>(r.Below(256));
				op.areas.resize(r.PerMille(10) ? r.Index(33) : r.Index(7));
				for (uint8_t &a : op.areas) a = static_cast<uint8_t>(r.Below(256));
				op.snap = NextSnapshot(r, base, *bl);
				msg.ops.push_back(std::move(op));
			}
			if (r.PerMille(30)) {
				Op op;
				op.type = OpType::Download;
				op.block = static_cast<uint32_t>(r.Range(0, 65535));
				if (op.block == 0) op.fileSize = r.Range(1, 10000000);
				op.data.resize(r.Index(301));
				for (uint8_t &b : op.data) b = static_cast<uint8_t>(r.Below(256));
				msg.ops.push_back(std::move(op));
			}
			if (r.PerMille(10)) { Op op; op.type = OpType::MapChange; msg.ops.push_back(std::move(op)); }
		}

		Base base;
		base.delta = delta;
		if (delta != 0) {
			const ServerStored &s = server.at(seq - delta);
			base.snap = &s.snap;
			base.header = &s.header;
		}
		HeaderRecord rec;
		st.messages++;
		bool ok = enc.Encode(msg, base, serverBaselines, serverHistory, size_t{ 1 } << 20U, out, rec);
		if (ok) {
			st.bytes += out.size();
			st.maxBytes = std::max(st.maxBytes, out.size());
			st.hash.Feed(out);
			uint32_t ddelta = 0;
			HeaderRecord drec;
			client.seq = seq;
			ok = DecodeMessage(out, client, clientHistory, clientBaselines, dm, ddelta, drec) && ddelta == delta &&
			     trace::SameMessage(dm, msg) && drec.ack == rec.ack && drec.time == rec.time && drec.cmdSeq == rec.cmdSeq;
			if (ok) {
				clientHistory.Add(dm);
				if (const Snapshot *sn = trace::SnapshotOf(dm)) client.bySeq[seq] = trace::ClientStore::Stored{ .snap = *sn, .header = drec };
			}
		}
		if (!ok && st.failures++ < 5) std::fprintf(stderr, "failure: seed %" PRIu64 " seq %u delta %u\n", seed, seq, delta);
		for (const Op &o : msg.ops) {
			if (o.type == OpType::GameState) {
				serverBaselines = o.gameState.baselines;
				st.gamestates++;
			}
			if (o.type == OpType::Command) st.commands++;
			if (o.type == OpType::Download) st.downloads++;
			if (o.type == OpType::Snapshot) {
				st.snapshots++;
				if (o.snap.hasVehicle) st.vehicle++;
				if (o.snap.hasVehicle && (Load(o.snap.ps, kPlayerEFlagsOffset) & kEfNodraw) != 0) st.pilot++;
				if (delta == 0) st.noBase++;
				if (delta > 11) st.baseOld++;
				st.maxDelta = std::max(st.maxDelta, delta);
				st.entities += o.snap.entities.size();
			}
		}
		serverHistory.Add(msg);
		if (const Snapshot *sn = trace::SnapshotOf(msg)) {
			ServerStored &s = server[seq];
			s.snap = *sn;
			s.header = rec;
		}
		if (seq > 2 * 255) server.erase(server.begin(), server.lower_bound(seq - (2 * 255)));
		client.Trim(seq);
	}
}

} // namespace

int main(int argc, char **argv) {
	int a = 1;
	std::optional<uint64_t> expected;
	if (argc > 1 && std::strncmp(argv[1], "--hash=", 7) == 0) expected = std::strtoull(argv[a++] + 7, nullptr, 16);
	const size_t streams = argc > a ? static_cast<size_t>(std::strtoull(argv[a], nullptr, 10)) : 100;
	const size_t nmsg = argc > a + 1 ? static_cast<size_t>(std::strtoull(argv[a + 1], nullptr, 10)) : 1000;
	const uint64_t seed = argc > a + 2 ? std::strtoull(argv[a + 2], nullptr, 10) : 1;
	Stats st;
	for (size_t i = 0; i < streams; i++) RunStream((seed * 1000003) + i, nmsg, st);
	std::printf("synthetic round trip: %zu streams, %zu messages, %zu bytes (largest %zu): failures %zu\n", streams, st.messages,
	            st.bytes, st.maxBytes, st.failures);
	std::printf("  snapshots %zu (vehicle %zu, pilot %zu, no base %zu, base older than 11 %zu, oldest %u), entities %zu, "
	            "gamestates %zu, commands %zu, downloads %zu\n",
	            st.snapshots, st.vehicle, st.pilot, st.noBase, st.baseOld, st.maxDelta, st.entities, st.gamestates, st.commands,
	            st.downloads);
	std::printf("hash of the coded bytes 0x%016" PRIX64, st.hash.h);
	if (expected) std::printf(", expected 0x%016" PRIX64 "%s", *expected, *expected == st.hash.h ? "" : " (MISMATCH)");
	std::printf("\n");
	return st.failures == 0 && st.messages != 0 && (!expected || *expected == st.hash.h) ? 0 : 1;
}
