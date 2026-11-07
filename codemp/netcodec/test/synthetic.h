// Synthetic server messages for the tests (C++ port of docs_rewrite/netcode/tools/python/fuzz.py, extended): the
// generator, and a stream that holds both sides, the server (the snapshots it sent with their header records, its
// commands and baselines) and the client (the snapshots it decoded, its commands and baselines).
// Messages are structurally valid: every operation (gamestates, commands, setgame, downloads, snapshots, map changes,
// nops), vehicles and pilot player states, extreme field values (any width, NaNs, infinities, -0.0, denormals),
// structured and random command texts, base ages from 1 to 255 and messages without a base. A state is derived from
// the state the codec codes it against (the previous state, the baseline or the zero state), with only fields of the
// table the codec sends changed, each to a value of its codec width, as the engine's states are (the decoder copies
// the rest from the same base).
// The generator uses only integer arithmetic and exact float conversions, so every compiler generates the same
// messages (the tests hash the coded bytes).
#pragma once

#include "demo_trace.h"
#include "message_compare.h"
#include "numeric.h"
#include "schema.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace netcodec::synth {

using namespace netcodec::detail;

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

inline constexpr uint32_t kEfNodraw = 1U << 8U;

// a value of a codec width (bits != 0), as the codec reads it back
inline int64_t RandInt(Rng &r, int bits) {
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

inline uint32_t RandFloat(Rng &r) {
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

inline const FieldDesc &FieldAt(std::span<const FieldDesc> fields, uint32_t offset) {
	for (const FieldDesc &f : fields)
		if (f.offset == offset) return f;
	std::fprintf(stderr, "no field at offset %u\n", offset);
	std::exit(2);
}

inline std::string RandBytes(Rng &r, size_t maxLen, uint32_t lo, uint32_t hi) {
	std::string s(r.Index(maxLen + 1), ' ');
	for (char &c : s) c = static_cast<char>(r.Range(lo, hi));
	return s;
}

// a server command: random bytes, or a verb and tokens (numbers near earlier ones, text, empty tokens) so that
// commands are often coded against an earlier one; sometimes at the engine's length limit
inline std::string RandCommand(Rng &r) {
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

inline std::vector<Entity> RandBaselines(Rng &r) {
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

inline const Entity *Find(const std::vector<Entity> &list, uint32_t num) {
	const auto it = std::ranges::lower_bound(list, num, {}, &Entity::number);
	return it != list.end() && it->number == num ? &*it : nullptr;
}

// a snapshot derived from the base the client holds (nullptr: none)
inline Snapshot NextSnapshot(Rng &r, const Snapshot *base, const std::vector<Entity> &baselines) {
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

// a generated message and the base it is coded against (sequence distance, 0 = none)
struct Candidate {
	Message msg;
	uint32_t delta = 0;
};

class Stream {
public:
	explicit Stream(uint64_t seed) noexcept : rng_(seed) {}

	[[nodiscard]] Rng &Generator() noexcept { return rng_; }
	[[nodiscard]] uint32_t Seq() const noexcept { return seq_; }

	// the next message, drawn from r; changes neither side
	[[nodiscard]] Candidate Next(Rng &r) const {
		Candidate c;
		Message &msg = c.msg;
		int64_t cmdSeq = cmdSeq_;
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
		if (seq_ == kFirstSeq) {
			gameState();
			return c;
		}
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
			for (uint32_t d = 1; d <= maxAge && d < seq_; d++)
				if (server_.contains(seq_ - d)) cand.push_back(d);
			c.delta = !cand.empty() && !r.PerMille(80) ? cand[r.Index(cand.size())] : 0;
			const Snapshot *base = c.delta != 0 ? &server_.at(seq_ - c.delta).snap : nullptr;
			const std::vector<Entity> *bl = &serverBaselines_;
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
		return c;
	}

	// codes c as the server: false if the encoder refuses it
	bool Encode(const Candidate &c, std::vector<uint8_t> &out, HeaderRecord &rec) {
		Base base;
		base.delta = c.delta;
		if (c.delta != 0) {
			const ServerStored &s = server_.at(seq_ - c.delta);
			base.snap = &s.snap;
			base.header = &s.header;
		}
		return enc_.Encode(c.msg, base, serverBaselines_, serverHistory_, size_t{ 1 } << 20U, out, rec);
	}

	// decodes bytes as the client would at this point (with a copy of its baselines); changes neither side
	bool DecodeAsClient(std::span<const uint8_t> bytes, Message &dm, uint32_t &delta, HeaderRecord &rec) {
		scratchBaselines_ = clientBaselines_;
		client_.seq = seq_;
		return DecodeMessage(bytes, client_, clientHistory_, scratchBaselines_, dm, delta, rec);
	}

	// codes a message the client decoded at this point, with the client's snapshots, commands and baselines in the
	// server's place (for decode(encode(decode(x))) == decode(x)); false if the encoder refuses it
	bool EncodeFromClient(const Message &dm, uint32_t delta, std::vector<uint8_t> &out) {
		Base base;
		base.delta = delta;
		if (delta != 0) {
			const auto it = client_.bySeq.find(seq_ - delta);
			if (it == client_.bySeq.end()) return false;
			base.snap = &it->second.snap;
			base.header = &it->second.header;
		}
		HeaderRecord rec;
		return enc_.Encode(dm, base, clientBaselines_, clientHistory_, size_t{ 1 } << 20U, out, rec);
	}

	// the client decodes the coded message (bytes empty: the encoder refused it), then both sides move to the next
	// message; true if the client decoded c exactly (with its header record)
	bool Commit(const Candidate &c, std::span<const uint8_t> bytes, const HeaderRecord &rec, Message &dm) {
		bool equal = false;
		if (!bytes.empty()) {
			uint32_t delta = 0;
			HeaderRecord drec;
			client_.seq = seq_;
			equal = DecodeMessage(bytes, client_, clientHistory_, clientBaselines_, dm, delta, drec) && delta == c.delta &&
			        trace::SameMessage(dm, c.msg) && drec.ack == rec.ack && drec.time == rec.time && drec.cmdSeq == rec.cmdSeq;
			if (equal) {
				clientHistory_.Add(dm);
				if (const Snapshot *sn = trace::SnapshotOf(dm)) client_.bySeq[seq_] = trace::ClientStore::Stored{ .snap = *sn, .header = drec };
			}
		}
		for (const Op &o : c.msg.ops) {
			if (o.type == OpType::GameState) serverBaselines_ = o.gameState.baselines;
			if (o.type == OpType::Command) cmdSeq_ = std::max(cmdSeq_, o.seq);
		}
		serverHistory_.Add(c.msg);
		if (const Snapshot *sn = trace::SnapshotOf(c.msg)) server_[seq_] = ServerStored{ .snap = *sn, .header = rec };
		if (seq_ > 2 * 255) server_.erase(server_.begin(), server_.lower_bound(seq_ - (2 * 255)));
		client_.Trim(seq_);
		seq_++;
		return equal;
	}

private:
	static constexpr uint32_t kFirstSeq = 100;
	struct ServerStored {
		Snapshot snap;
		HeaderRecord header;
	};
	Rng rng_;
	uint32_t seq_ = kFirstSeq;
	int64_t cmdSeq_ = 0;
	MessageEncoder enc_;
	std::map<uint32_t, ServerStored> server_;
	trace::ClientStore client_;
	trace::CommandLog serverHistory_, clientHistory_;
	std::vector<Entity> serverBaselines_, clientBaselines_, scratchBaselines_;
};

} // namespace netcodec::synth
