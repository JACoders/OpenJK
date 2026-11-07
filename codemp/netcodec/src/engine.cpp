// The engine's interface (engine.h) over the codec's views (codec.h).
#include "netcodec/engine.h"

#include "netcodec/codec.h"

#include <algorithm>
#include <optional>
#include <span>
#include <string_view>

namespace netcodec::engine {

namespace {

static_assert(kMaxEntities == netcodec::kMaxEntities, "entity numbers");
static_assert(kMinCommands >= (2 * kCommandWindow) + 2, "the command history the codec needs");

[[nodiscard]] uint32_t Slot(int32_t seq) noexcept { return static_cast<uint32_t>(seq) % kPacketBackup; }

[[nodiscard]] std::span<const uint8_t> PlayerBytes(const void *p) noexcept {
	return p != nullptr ? std::span<const uint8_t>(static_cast<const uint8_t *>(p), kPlayerBytes) : std::span<const uint8_t>();
}

[[nodiscard]] std::span<const uint8_t> RingBytes(const EntityRing &ring) noexcept {
	return { static_cast<const uint8_t *>(ring.states), size_t{ ring.count } * kEntityBytes };
}

// the baselines of numbers 0 to kMaxEntities - 1
[[nodiscard]] size_t BaselineBytes(size_t stride) noexcept { return ((kMaxEntities - 1) * stride) + kEntityBytes; }

[[nodiscard]] Snapshot View(const Frame &f, const EntityRing &ring) noexcept {
	return Snapshot{ .ps = PlayerBytes(f.ps),
	                 .vps = PlayerBytes(f.vps),
	                 .entities = netcodec::EntityRing{ .ring = RingBytes(ring),
	                                                   .first = static_cast<uint32_t>(f.firstEntity) % ring.count,
	                                                   .count = f.numEntities } };
}

[[nodiscard]] HeaderRecord Header(const Record &r, const Snapshot *older) noexcept {
	return HeaderRecord{ .ack = r.ack, .time = r.time, .cmdSeq = r.cmdSeq, .older = older };
}

[[nodiscard]] Record ToRecord(const HeaderRecord &h, int32_t seq, uint32_t delta) noexcept {
	return Record{ .ack = h.ack,
	               .cmdSeq = h.cmdSeq,
	               .time = h.time.value_or(0),
	               .olderSeq = delta != 0 ? seq - static_cast<int32_t>(delta) : -1 };
}

// the base's own base is used when it is fewer than kPacketBackup messages before message seq (both sides)
[[nodiscard]] bool OlderInRange(const Frame &base, int32_t seq) noexcept {
	return base.record.olderSeq >= 0 && seq - base.record.olderSeq < static_cast<int32_t>(kPacketBackup);
}

class RingHistory final : public CommandHistory {
public:
	explicit RingHistory(const CommandRing &r) noexcept
	    : r_(r), texts_(r.texts, size_t{ r.count } * r.stride) {}

	[[nodiscard]] std::string_view Find(int64_t seq) const override {
		if (seq < 1 || seq > r_.last || seq <= int64_t{ r_.last } - r_.count) return {};
		const std::span<const char> t = texts_.subspan((static_cast<size_t>(seq) % r_.count) * r_.stride, r_.stride);
		return { t.data(), static_cast<size_t>(std::ranges::find(t, '\0') - t.begin()) };   // data() never null here
	}

private:
	CommandRing r_;
	std::span<const char> texts_;
};

// the op at index used of msg, reusing its storage (no allocation once warm)
Op &Next(Message &msg, size_t &used, OpType type) {
	if (used == msg.ops.size()) msg.ops.emplace_back();
	Op &op = msg.ops[used++];
	op.type = type;
	return op;
}

} // namespace

// ------------------------------------------------------------------ server

struct ServerCodec::Impl {
	MessageEncoder enc;
	Message msg;
	std::vector<uint8_t> out;
};

ServerCodec::ServerCodec() : impl_(std::make_unique<Impl>()) {}
ServerCodec::~ServerCodec() = default;

size_t ServerCodec::Encode(const ServerMessage &msg, const Frame *frames, const EntityRing &ring, int32_t nextEntity,
                           const CommandRing &commands, const Baselines &baselines, uint8_t *out, size_t outSize,
                           uint32_t &delta, Record &record) {
	const std::span<const Frame> fr(frames, kPacketBackup);
	const RingHistory history(commands);
	const BaselineTable bl{ .bytes = std::span<const uint8_t>(static_cast<const uint8_t *>(baselines.bytes),
	                                                          BaselineBytes(baselines.stride)),
	                        .stride = baselines.stride };
	Message &m = impl_->msg;
	m.ack = msg.ack;
	size_t used = 0;
	for (int32_t seq = msg.firstCommand; seq <= commands.last; seq++) {
		const std::string_view text = history.Find(seq);
		if (text.data() == nullptr) return 0;
		Op &op = Next(m, used, OpType::Command);
		op.seq = seq;
		op.text.assign(text);
	}
	if (msg.gameState) {
		GameState &gs = Next(m, used, OpType::GameState).gameState;
		gs.relSeq = msg.relSeq;
		gs.configStrings.clear();
		for (uint32_t i = 0; i < msg.numConfigStrings; i++) {
			const char *text = std::span<const char *const>(msg.configStrings, msg.numConfigStrings)[i];
			if (text != nullptr && text[0] != '\0') gs.configStrings.push_back(netcodec::ConfigString{ .index = static_cast<uint16_t>(i), .text = text });
		}
		gs.baselines = bl;
		gs.clientNum = msg.clientNum;
		gs.checksum = msg.checksum;
	}
	if (msg.setGame != nullptr) Next(m, used, OpType::SetGame).text.assign(msg.setGame);

	// the base and the base's own base: held, and on the server their entities still in the shared ring
	const auto held = [&](const Frame &f, int32_t s) {
		return f.seq == s && f.firstEntity > nextEntity - static_cast<int32_t>(ring.count);
	};
	Snapshot baseSnap, olderSnap;
	HeaderRecord baseHeader;
	Base base;
	delta = 0;
	if (msg.snapshot) {
		Op &op = Next(m, used, OpType::Snapshot);
		op.time = msg.time;
		op.flags = msg.flags;
		const std::span<const uint8_t> areas(msg.areas, msg.numAreas);
		op.areas.assign(areas.begin(), areas.end());
		op.snap = View(msg.frame, ring);
		const int32_t bs = msg.seq - static_cast<int32_t>(msg.delta);
		if (msg.delta != 0 && msg.delta < kPacketBackup && held(fr[Slot(bs)], bs)) {
			const Frame &bf = fr[Slot(bs)];
			const bool older = OlderInRange(bf, msg.seq);
			if (!older || held(fr[Slot(bf.record.olderSeq)], bf.record.olderSeq)) {
				if (older) olderSnap = View(fr[Slot(bf.record.olderSeq)], ring);
				baseSnap = View(bf, ring);
				baseHeader = Header(bf.record, older ? &olderSnap : nullptr);
				delta = msg.delta;
				base = Base{ .delta = delta, .snap = &baseSnap, .header = &baseHeader };
			}
		}
	}
	if (msg.download) {
		Op &op = Next(m, used, OpType::Download);
		op.block = msg.block;
		op.fileSize = msg.fileSize;
		const std::span<const uint8_t> data(msg.data, msg.dataSize);
		op.data.assign(data.begin(), data.end());
	}
	if (msg.mapChange) Next(m, used, OpType::MapChange);
	m.ops.resize(used);

	HeaderRecord rec;
	if (!impl_->enc.Encode(m, base, bl, history, outSize, impl_->out, rec)) return 0;
	std::ranges::copy(impl_->out, out);
	record = ToRecord(rec, msg.seq, delta);
	return impl_->out.size();
}

// ------------------------------------------------------------------ client

namespace {

// the base of message seq from the client's frames, and the base's own base under the rule of OlderInRange
class FrameLookup final : public BaseLookup {
public:
	FrameLookup(int32_t seq, std::span<const Frame> frames, const EntityRing &ring) noexcept : seq_(seq), frames_(frames), ring_(ring) {}

	[[nodiscard]] bool Find(uint32_t delta, const Snapshot *&snap, const HeaderRecord *&header) const override {
		if (delta >= kPacketBackup) return false;
		const int32_t bs = seq_ - static_cast<int32_t>(delta);
		const Frame &bf = frames_[Slot(bs)];
		if (bf.seq != bs) return false;
		const bool older = OlderInRange(bf, seq_);
		if (older) {
			const Frame &of = frames_[Slot(bf.record.olderSeq)];
			if (of.seq != bf.record.olderSeq) return false;   // the server used it: the client must hold it
			older_ = View(of, ring_);
		}
		base_ = View(bf, ring_);
		header_ = Header(bf.record, older ? &older_ : nullptr);
		snap = &base_;
		header = &header_;
		return true;
	}

private:
	int32_t seq_;
	std::span<const Frame> frames_;
	EntityRing ring_;
	mutable Snapshot base_, older_;
	mutable HeaderRecord header_;
};

} // namespace

struct ClientCodec::Impl {
	Message msg;
	std::vector<Command> commands;
	std::vector<ConfigString> configStrings;
};

ClientCodec::ClientCodec() : impl_(std::make_unique<Impl>()) {}
ClientCodec::~ClientCodec() = default;

bool ClientCodec::Decode(const uint8_t *bytes, size_t size, int32_t seq, const Frame *frames, const EntityRing &ring,
                         const CommandRing &commands, const ClientTarget &target, ClientMessage &msg) {
	const FrameLookup lookup(seq, std::span<const Frame>(frames, kPacketBackup), ring);
	const RingHistory history(commands);
	const std::span<uint8_t> blBytes(static_cast<uint8_t *>(target.baselines), BaselineBytes(target.baselineStride));
	const SnapshotOut out{ .ps = std::span<uint8_t>(static_cast<uint8_t *>(target.ps), kPlayerBytes),
	                       .vps = std::span<uint8_t>(static_cast<uint8_t *>(target.vps), kPlayerBytes),
	                       .ring = std::span<uint8_t>(static_cast<uint8_t *>(target.ring), size_t{ target.ringCount } * kEntityBytes),
	                       .first = static_cast<uint32_t>(target.firstEntity) % target.ringCount,
	                       .maxEntities = kMaxSnapshotEntities };
	Message &m = impl_->msg;
	uint32_t delta = 0;
	HeaderRecord rec;
	if (!DecodeMessage(std::span<const uint8_t>(bytes, size), lookup, history,
	                   BaselineTable{ .bytes = blBytes, .stride = target.baselineStride },
	                   BaselineTableOut{ .bytes = blBytes, .stride = target.baselineStride }, out, m, delta, rec))
		return false;

	msg = ClientMessage{};
	msg.ack = static_cast<int32_t>(m.ack);
	msg.delta = delta;
	impl_->commands.clear();
	impl_->configStrings.clear();
	for (const Op &op : m.ops) {
		switch (op.type) {
		case OpType::Command:
			impl_->commands.push_back(Command{ .seq = static_cast<int32_t>(op.seq), .text = op.text.c_str(), .length = ToU32(op.text.size()) });
			break;
		case OpType::GameState: {
			const GameState &gs = op.gameState;
			for (const netcodec::ConfigString &cs : gs.configStrings)
				impl_->configStrings.push_back(ConfigString{ .index = cs.index, .text = cs.text.c_str(), .length = ToU32(cs.text.size()) });
			msg.gameState = true;
			msg.relSeq = static_cast<int32_t>(gs.relSeq);
			msg.clientNum = static_cast<int32_t>(gs.clientNum);
			msg.checksum = static_cast<int32_t>(gs.checksum);
			break;
		}
		case OpType::SetGame:
			msg.setGame = op.text.c_str();
			break;
		case OpType::Snapshot:
			msg.snapshot = true;
			msg.time = static_cast<int32_t>(op.time);
			msg.flags = op.flags;
			msg.areas = op.areas.data();
			msg.numAreas = ToU32(op.areas.size());
			msg.vehicle = !op.snap.vps.empty();
			msg.numEntities = op.snap.entities.count;
			msg.record = ToRecord(rec, seq, delta);
			break;
		case OpType::Download:
			msg.download = true;
			msg.block = op.block;
			msg.fileSize = static_cast<int32_t>(op.fileSize);
			msg.data = op.data.data();
			msg.dataSize = ToU32(op.data.size());
			break;
		case OpType::MapChange:
			msg.mapChange = true;
			break;
		case OpType::Nop:
			break;
		}
	}
	msg.commands = impl_->commands.data();
	msg.numCommands = ToU32(impl_->commands.size());
	msg.configStrings = impl_->configStrings.data();
	msg.numConfigStrings = ToU32(impl_->configStrings.size());
	return true;
}

} // namespace netcodec::engine
