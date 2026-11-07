// netcodec: coding of whole messages: player states, entities, server commands, gamestate, download blocks.
#include "field_codec.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <string_view>
#include <utility>

namespace netcodec {

namespace detail {
namespace {

using ctx::Of;
using enum ctx::Scalar;

constexpr size_t kMaxOps = 4096;               // decoder limit against malformed input
constexpr uint32_t kEfNodraw = 1U << 8U;
constexpr uint32_t kEntityClassCap = 18;       // entity types 18 and above share a context class
constexpr uint32_t kEntityGapRaw = 40;         // entity number gaps from 40 on are sent raw (10 bits)
constexpr uint32_t kEntityListEnd = 41;
constexpr uint32_t kEscapedDelta = 256;         // added to the base delta symbol of an escaped message

// ------------------------------------------------------------------ player states
// the base player state has some non-zero networked field: then it is the same player's previous state
bool AnyNonZero(ImageView ps) noexcept {
	return std::ranges::any_of(kPlayerAllOffsets, [&](uint16_t off) { return Load(ps, off) != 0; });
}

int64_t VehicleNum(ImageView ps) noexcept { return ValueBits(Load(ps, kVehicleNumOffset), 10); }
bool IsPilot(ImageView ps) noexcept { return VehicleNum(ps) != 0 && (Load(ps, kPlayerEFlagsOffset) & kEfNodraw) != 0; }

const uint8_t *OlderPlayer(const PredCtx &c, bool vehicle) noexcept {
	const Snapshot *o = c.olderSnap;
	if (o == nullptr) return nullptr;
	if (!vehicle) return o->ps.data();
	if (VehicleNum(o->ps) != 0 && !o->vps.empty()) return o->vps.data();
	return ZeroPlayer().data();
}

// escaped message: every field of the table raw (RawWidth); out of the hot loops
template <class O> void PutFieldsRaw(O &io, std::span<const FieldDesc> fields, ImageView nw, ImageView old) noexcept {
	for (const FieldDesc &f : fields) {
		const bool changed = FieldDiffers(nw, old, f);
		io.Raw(changed ? 1 : 0, 1);
		if (changed) PutRawField(io, f, Value(nw, f));
	}
}

void GetFieldsRaw(In &io, std::span<const FieldDesc> fields, ImageRef out) noexcept {
	for (const FieldDesc &f : fields)
		if (io.Raw(1) != 0) Store(out, f, GetRawField(io, f));
}

// the fields are sent as "index of the next changed field, given the previous one", then the field's value
template <class O>
void PutPlayer(O &io, PredCtx &c, std::span<const FieldDesc> fields, uint32_t gap, ImageView nw,
               ImageView old, const uint8_t *older, const Model &m) noexcept {
	if (c.escaped) [[unlikely]] {
		PutFieldsRaw(io, fields, nw, old);
		return;
	}
	c.cur = nw; c.old = old; c.older = older; c.pred = AnyNonZero(old);
	const uint32_t n = ToU32(fields.size());
	uint32_t next = 0;                         // context index: previous changed field + 1
	for (uint32_t i = 0; i < n; i++) {
		const FieldDesc &f = fields[i];
		if (!FieldDiffers(nw, old, f)) continue;
		io.Symbol(gap + next, i);
		next = i + 1;
		PutField(io, c, f, ctx::PlayerField(f.ctx), Value(nw, f), Value(old, f), m.TrTypeBits());
	}
	io.Symbol(gap + next, n);
}

bool GetPlayer(In &io, PredCtx &c, std::span<const FieldDesc> fields, uint32_t gap, ImageView old,
               ImageRef out, const uint8_t *older, const Model &m) noexcept {
	std::ranges::copy(old, out.begin());
	if (c.escaped) [[unlikely]] {
		GetFieldsRaw(io, fields, out);
		return true;
	}
	c.cur = out; c.old = old; c.older = older; c.pred = AnyNonZero(old);
	const uint32_t n = ToU32(fields.size());
	uint32_t next = 0;
	for (;;) {
		const uint32_t i = io.Symbol(gap + next);
		if (i == n) return true;
		if (i < next) return false;
		const FieldDesc &f = fields[i];
		Store(out, f, GetField(io, c, f, ctx::PlayerField(f.ctx), Value(old, f), m.TrTypeBits()));
		next = i + 1;
	}
}

// ------------------------------------------------------------------ entities
uint32_t ClassOf(ImageView st) noexcept {
	const int64_t et = Value(st, kEntityFields[kEntityTypeIndex]);
	return et < int64_t{ kEntityClassCap } ? static_cast<uint32_t>(et) : kEntityClassCap;
}

// The entities of a list read one after the other, with the current one's number (kMaxEntities past the end): a
// snapshot's ring (the offset wraps at the ring's end), or the baselines of a gamestate (the numbers whose state
// holds its number, in increasing order).
class RingReader {
public:
	explicit RingReader(const EntityRing &r) noexcept
		: begin_(r.ring.data()), end_(r.ring.data() + r.ring.size()), p_(r.ring.data() + (r.first * kEntityBytes)),
		  left_(r.count) {
		Load();
	}
	[[nodiscard]] uint32_t Number() const noexcept { return num_; }
	[[nodiscard]] ImageView State() const noexcept { return { p_, kEntityBytes }; }
	void Next() noexcept {
		left_--;
		p_ += kEntityBytes;
		if (p_ == end_) p_ = begin_;
		Load();
	}

private:
	void Load() noexcept {
		num_ = kMaxEntities;
		if (left_ != 0) std::memcpy(&num_, p_, 4);
	}

	const uint8_t *begin_, *end_, *p_;
	uint32_t left_;
	uint32_t num_ = kMaxEntities;
};

class TableReader {
public:
	explicit TableReader(const BaselineTable &t) noexcept : t_(t) {
		if (t.bytes.empty()) return;
		for (uint32_t n = 1; n < kMaxEntities; n++)
			if (EntityNumber(t[n]) == n) nums_[size_++] = static_cast<uint16_t>(n);
	}
	[[nodiscard]] uint32_t Number() const noexcept { return k_ < size_ ? nums_[k_] : kMaxEntities; }
	[[nodiscard]] ImageView State() const noexcept { return t_[nums_[k_]]; }
	void Next() noexcept { k_++; }

private:
	BaselineTable t_;
	std::array<uint16_t, kMaxEntities> nums_{};
	uint32_t size_ = 0;
	uint32_t k_ = 0;
};

// entities of a snapshot looked up by increasing numbers: a forward cursor instead of a search per lookup
class EntityCursor {
public:
	explicit EntityCursor(const Snapshot *snap) noexcept : r_(snap != nullptr ? snap->entities : EntityRing{}) {}

	// num above the previous lookup's; empty if the snapshot has no such entity
	[[nodiscard]] ImageView Find(uint32_t num) noexcept {
		while (r_.Number() < num) r_.Next();
		return r_.Number() == num ? r_.State() : ImageView{};
	}

private:
	RingReader r_;
};

// where GetEntities writes: a snapshot's ring, in number order (at most maxEntities), or the baseline table (by
// number). Slot returns an empty span when the snapshot is full.
class RingSink {
public:
	explicit RingSink(const SnapshotOut &o) noexcept
		: ring_(o.ring), size_(ToU32(o.ring.size() / kEntityBytes)), first_(o.first),
		  max_(std::min(o.maxEntities, size_)) {}
	[[nodiscard]] ImageRef Slot(uint32_t /*num*/) noexcept {
		if (count_ == max_) return {};
		const uint32_t k = first_ + count_ < size_ ? first_ + count_ : first_ + count_ - size_;
		count_++;
		return ring_.subspan(k * kEntityBytes, kEntityBytes);
	}
	[[nodiscard]] EntityRing View() const noexcept { return EntityRing{ .ring = ring_, .first = first_, .count = count_ }; }

private:
	std::span<uint8_t> ring_;
	uint32_t size_, first_, max_;
	uint32_t count_ = 0;
};

class TableSink {
public:
	explicit TableSink(const BaselineTableOut &t) noexcept : t_(t) {}
	// number 0 has no baseline (its number field means none): malformed
	[[nodiscard]] ImageRef Slot(uint32_t num) const noexcept {
		return num == 0 ? ImageRef{} : t_.bytes.subspan(num * t_.stride, kEntityBytes);
	}

private:
	BaselineTableOut t_;
};

// indices (codec order) of the fields that differ between two entity states; one pass, raw words first
using ChangeList = std::array<uint8_t, kNumEntityFields>;
static_assert(kNumEntityFields <= 256, "change list indices are bytes");

// two entity images are equal: compared inline 8 bytes at a time (most compared entities are unchanged, and a
// library memcmp call costs more than the comparison)
bool SameEntity(ImageView a, ImageView b) noexcept {
	static_assert(kEntityBytes % 8 == 4, "the last word is compared apart");
	uint64_t acc = 0;
	for (size_t k = 0; k + 8 <= kEntityBytes; k += 8) {
		uint64_t wa = 0, wb = 0;
		std::memcpy(&wa, a.subspan(k, 8).data(), 8);
		std::memcpy(&wb, b.subspan(k, 8).data(), 8);
		acc |= wa ^ wb;
	}
	acc |= Load(a, kEntityBytes - 4) ^ Load(b, kEntityBytes - 4);
	return acc == 0;
}

uint32_t Changes(ImageView a, ImageView b, ChangeList &list) noexcept {
	if (SameEntity(a, b)) return 0;
	uint32_t n = 0;
	for (uint32_t i = 0; i < kNumEntityFields; i++)
		if (FieldDiffers(a, b, kEntityFields[i])) list[n++] = static_cast<uint8_t>(i);
	return n;
}

template <class O>
void PutEntity(O &io, const PredCtx &c, ImageView nw, ImageView basest, std::span<const uint8_t> changes, const Model &m) noexcept {
	if (c.escaped) [[unlikely]] {   // every field, the type included
		PutFieldsRaw(io, kEntityFields, nw, basest);
		return;
	}
	const uint32_t bcls = ClassOf(basest), ncls = ClassOf(nw);
	const bool typeChanged = std::ranges::find(changes, kEntityTypeIndex) != changes.end();
	io.Symbol(ctx::kTypeChanged + bcls, typeChanged ? 1 : 0);
	if (typeChanged) io.Symbol(ctx::kTypeValue + bcls, static_cast<uint64_t>(Value(nw, kEntityFields[kEntityTypeIndex])));
	const uint32_t gap = ctx::EntityGap(ncls);
	uint32_t next = 0;
	for (const uint8_t i : changes) {
		if (i == kEntityTypeIndex) continue;
		const FieldDesc &f = kEntityFields[i];
		io.Symbol(gap + next, i);
		next = i + 1U;
		PutField(io, c, f, ctx::EntityField(f.ctx), Value(nw, f), Value(basest, f), m.TrTypeBits());
	}
	io.Symbol(gap + next, kNumEntityFields);
}

// entity operations: 0 changed, 1 new from its baseline, 2 removed, 3 new and equal to its baseline
// nw: a RingReader or a TableReader; entity numbers below kMaxEntities
template <class O, class R>
void PutEntities(O &io, PredCtx &c, R nw, RingReader old, const BaselineTable &baselines, const Model &m) noexcept {
	ChangeList list{};
	EntityCursor olderList(c.olderSnap);
	int64_t prevnum = -1;
	uint32_t prevop = 0;
	for (;;) {
		const uint32_t nn = nw.Number(), on = old.Number();
		const uint32_t num = std::min(nn, on);
		if (num == kMaxEntities) break;
		ImageView ne, oe;
		if (nn == num) {
			ne = nw.State();
			nw.Next();
		}
		if (on == num) {
			oe = old.State();
			old.Next();
		}
		uint32_t op = 2;
		uint32_t nch = 0;
		ImageView basest;
		if (!ne.empty() && !oe.empty()) {
			nch = Changes(ne, oe, list);
			if (nch == 0) continue;
			op = 0;
			basest = oe;
		} else if (!ne.empty()) {
			basest = baselines[num];   // the zero state without a baseline
			nch = Changes(ne, basest, list);
			op = nch != 0 ? 1 : 3;
		}
		const int64_t gap = num - prevnum - 1;
		prevnum = num;
		if (gap < int64_t{ kEntityGapRaw }) {
			io.Symbol(Of(kEntGap), static_cast<uint64_t>(gap));
		} else {
			io.Symbol(Of(kEntGap), kEntityGapRaw);
			io.Raw(static_cast<uint64_t>(gap), 10);
		}
		io.Symbol(ctx::kEntityOp + prevop, op);
		prevop = op;
		if (op == 2 || op == 3) continue;
		c.cur = ne; c.old = basest; c.pred = op == 0;
		const ImageView older = op == 0 ? olderList.Find(num) : ImageView{};
		c.older = older.empty() ? nullptr : older.data();
		PutEntity(io, c, ne, basest, std::span<const uint8_t>(list.data(), nch), m);
	}
	io.Symbol(Of(kEntGap), kEntityListEnd);
}

// out: a RingSink or a TableSink
template <class Sink>
bool GetEntities(In &io, PredCtx &c, RingReader old, const BaselineTable &baselines, Sink &out, const Model &m) {
	const auto keep = [&](ImageView src) {   // an unchanged entity of the base
		const ImageRef d = out.Slot(EntityNumber(src));
		if (d.empty()) return false;
		std::ranges::copy(src, d.begin());
		return true;
	};
	int64_t prevnum = -1;
	uint32_t prevop = 0;
	const FieldDesc &typeField = kEntityFields[kEntityTypeIndex];
	EntityCursor olderList(c.olderSnap);
	for (;;) {
		const uint32_t g = io.Symbol(Of(kEntGap));
		if (g == kEntityListEnd) break;
		const uint64_t gap = g == kEntityGapRaw ? io.Raw(10) : g;
		const uint64_t num = static_cast<uint64_t>(prevnum + 1) + gap;
		if (num >= kMaxEntities) return false;
		prevnum = static_cast<int64_t>(num);
		for (; old.Number() < num; old.Next())
			if (!keep(old.State())) return false;
		const ImageView oe = old.Number() == num ? old.State() : ImageView{};
		if (!oe.empty()) old.Next();
		const uint32_t op = io.Symbol(ctx::kEntityOp + prevop);
		prevop = op;
		if (op == 2) {
			if (oe.empty()) return false;
			continue;
		}
		if (op == 0 && oe.empty()) return false;
		// the base: the previous state, or the baseline (the zero state without one); copied in one go
		const ImageView basest = op == 0 ? oe : baselines[static_cast<uint32_t>(num)];
		const ImageRef st = out.Slot(static_cast<uint32_t>(num));
		if (st.empty()) return false;
		std::ranges::copy(basest, st.begin());
		SetEntityNumber(st, static_cast<uint32_t>(num));
		if (op == 3) continue;
		if (c.escaped) [[unlikely]] {
			GetFieldsRaw(io, kEntityFields, st);
			continue;
		}
		c.cur = st; c.old = basest; c.pred = op == 0;
		const ImageView older = op == 0 ? olderList.Find(static_cast<uint32_t>(num)) : ImageView{};
		c.older = older.empty() ? nullptr : older.data();
		const uint32_t bcls = ClassOf(basest);
		if (io.Symbol(ctx::kTypeChanged + bcls) != 0) Store(st, typeField, io.Symbol(ctx::kTypeValue + bcls));
		const uint32_t gapCtx = ctx::EntityGap(ClassOf(st));
		uint32_t next = 0;
		for (;;) {
			const uint32_t i = io.Symbol(gapCtx + next);
			if (i == kNumEntityFields) break;
			if (i < next || i == kEntityTypeIndex) return false;
			const FieldDesc &f = kEntityFields[i];
			Store(st, f, GetField(io, c, f, ctx::EntityField(f.ctx), Value(basest, f), m.TrTypeBits()));
			next = i + 1;
		}
	}
	for (; old.Number() != kMaxEntities; old.Next())
		if (!keep(old.State())) return false;
	return true;
}

// ------------------------------------------------------------------ server commands
// A command is coded against an earlier command with the same first token (verb) that the client holds: one sent
// earlier in the same message, or one at most kCommandWindow sequence numbers back and not after the base's last
// command. Then the difference of the token counts (tokens are separated by single spaces) and, per token, "same as
// the reference token" or the new token: a difference with a numeric reference token, a number, or text. Without a
// reference (distance 0) the command is sent as text. Text is coded byte by byte given the previous byte.
constexpr int64_t kMaxTokens = 4096;           // decoder limit against malformed input

// the tokens of a text, one after the other
class Tokens {
public:
	explicit Tokens(std::string_view text) noexcept : rest_(text) {}

	[[nodiscard]] static size_t Count(std::string_view text) noexcept {
		return 1 + static_cast<size_t>(std::ranges::count(text, ' '));
	}

	// the next token (call at most Count() times)
	[[nodiscard]] std::string_view Next() noexcept {
		const size_t sp = std::min(rest_.find(' '), rest_.size());
		std::string_view t = rest_;
		t.remove_suffix(t.size() - sp);
		rest_.remove_prefix(std::min(sp + 1, rest_.size()));
		return t;
	}

private:
	std::string_view rest_;
};

std::string_view Verb(std::string_view text) noexcept { return Tokens(text).Next(); }

// a decimal integer in canonical form, so that printing it gives the token back: "0", or an optional '-' and 1 to
// 18 digits without a leading zero
bool ParseNumber(std::string_view t, int64_t &v) noexcept {
	const bool neg = !t.empty() && t.front() == '-';
	if (neg) t.remove_prefix(1);
	if (t.empty() || t.size() > 18 || (t.front() == '0' && (t.size() > 1 || neg))) return false;
	int64_t x = 0;
	for (const char ch : t) {
		if (ch < '0' || ch > '9') return false;
		x = (x * 10) + (ch - '0');
	}
	v = neg ? -x : x;
	return true;
}

// the text of command seq if the client holds it: earlier in this message, or in the history up to the base's last
// command; a null data() if not
std::string_view HeldCommand(int64_t seq, std::span<const Op> before, int64_t known, const CommandHistory &history) {
	for (const Op &op : before)
		if (op.type == OpType::Command && op.seq == seq) return op.text;
	return seq <= known ? history.Find(seq) : std::string_view();
}

// command text: bytes then a 0 terminator, each byte coded given the previous one
template <class O> void PutCommandText(O &io, std::string_view text) {
	uint32_t prev = 0;
	for (const char ch : text) {
		const auto b = static_cast<uint8_t>(ch);
		io.Symbol(ctx::CommandText(prev), b);   // a 0 byte would end the text: commands hold none
		prev = b;
	}
	io.Symbol(ctx::CommandText(prev), 0);
}

bool GetCommandText(In &io, std::string &text) {
	uint32_t prev = 0;
	for (uint32_t b = io.Symbol(ctx::CommandText(0)); b != 0; b = io.Symbol(ctx::CommandText(prev))) {
		if (text.size() >= In::kMaxString) return false;
		text.push_back(static_cast<char>(b));
		prev = b;
	}
	return true;
}

uint32_t SameCtx(bool prevSame, bool refNumber) noexcept {
	return Of(kCmdSame0) + (prevSame ? 0U : 1U) + (refNumber ? 0U : 2U);
}

template <class O>
void PutCommand(O &io, const Op &op, std::span<const Op> before, int64_t known, const CommandHistory &history) {
	const std::string_view verb = Verb(op.text);
	std::string_view ref;   // null data(): none yet
	int64_t dist = 0;
	const size_t nn = Tokens::Count(op.text);
	// more tokens than the decoder accepts against a reference (never in the engine): sent as text
	while (ref.data() == nullptr && dist < kCommandWindow && nn <= static_cast<size_t>(kMaxTokens)) {
		dist++;
		const std::string_view t = HeldCommand(op.seq - dist, before, known, history);
		if (t.data() != nullptr && Verb(t) == verb) ref = t;
	}
	if (ref.data() == nullptr) {
		io.Class(Of(kCmdRef), 0);
		PutCommandText(io, op.text);
		return;
	}
	io.Class(Of(kCmdRef), static_cast<uint64_t>(dist));
	const size_t nr = Tokens::Count(ref);
	io.Class(Of(kCmdTokens), Zz(static_cast<int64_t>(nn) - static_cast<int64_t>(nr)));
	Tokens nt(op.text), rt(ref);
	bool prevSame = true;
	for (size_t i = 0; i < nn; i++) {
		const std::string_view tok = nt.Next();
		int64_t rv = 0;
		bool refNumber = false;
		if (i < nr) {
			const std::string_view rtok = rt.Next();
			refNumber = ParseNumber(rtok, rv);
			const bool same = tok == rtok;
			io.Symbol(SameCtx(prevSame, refNumber), same ? 0 : 1);
			prevSame = same;
			if (same) continue;
		}
		int64_t v = 0;
		const bool number = ParseNumber(tok, v);
		io.Symbol(refNumber ? Of(kCmdKindRefNum) : Of(kCmdKindOther), number ? 0 : 1);
		if (number && refNumber) io.Class(Of(kCmdNumDelta), Zz(v - rv));
		else if (number) io.Class(Of(kCmdNum), Zz(v));
		else PutCommandText(io, tok);
	}
}

bool GetCommand(In &io, Op &op, std::span<const Op> before, int64_t known, const CommandHistory &history) {
	op.text.clear();
	const uint64_t dist = io.Class(Of(kCmdRef));
	if (dist == 0) return GetCommandText(io, op.text);
	if (dist > static_cast<uint64_t>(kCommandWindow)) return false;
	const std::string_view ref = HeldCommand(op.seq - static_cast<int64_t>(dist), before, known, history);
	if (ref.data() == nullptr) return false;
	const size_t nr = Tokens::Count(ref);
	const int64_t n = static_cast<int64_t>(nr) + Unzz(io.Class(Of(kCmdTokens)));
	if (n < 1 || n > kMaxTokens) return false;
	Tokens rt(ref);
	bool prevSame = true;
	std::array<char, 24> digits{};
	for (int64_t i = 0; i < n; i++) {
		if (i > 0) op.text.push_back(' ');
		int64_t rv = 0;
		bool refNumber = false;
		if (std::cmp_less(i, nr)) {
			const std::string_view rtok = rt.Next();
			refNumber = ParseNumber(rtok, rv);
			const bool same = io.Symbol(SameCtx(prevSame, refNumber)) == 0;
			prevSame = same;
			if (same) {
				op.text.append(rtok);
				continue;
			}
		}
		if (io.Symbol(refNumber ? Of(kCmdKindRefNum) : Of(kCmdKindOther)) == 0) {
			// unsigned addition: a malformed difference wraps around instead of overflowing
			const uint64_t x = refNumber ? static_cast<uint64_t>(rv) + static_cast<uint64_t>(Unzz(io.Class(Of(kCmdNumDelta))))
			                             : static_cast<uint64_t>(Unzz(io.Class(Of(kCmdNum))));
			const auto res = std::to_chars(digits.data(), digits.data() + digits.size(), static_cast<int64_t>(x));
			op.text.append(digits.data(), res.ptr);
		} else {
			const size_t start = op.text.size();
			// a token holds no space (the encoder splits on them): malformed otherwise
			if (!GetCommandText(io, op.text) || op.text.find(' ', start) != std::string::npos) return false;
		}
		if (op.text.size() > In::kMaxString) return false;
	}
	return true;
}

// ------------------------------------------------------------------ messages
template <class O>
void PutMessage(O &io, const Message &msg, const Base &base, const BaselineTable &baselines,
                const CommandHistory &history, HeaderRecord &rec, const Model &m, bool escaped) {
	const HeaderRecord *bh = base.header;
	io.Symbol(Of(kMDelta), base.delta + (escaped ? kEscapedDelta : 0U));   // one symbol for both
	PredCtx c;
	c.escaped = escaped;
	c.btime = bh != nullptr ? bh->time : std::nullopt;
	c.olderSnap = bh != nullptr ? bh->older : nullptr;
	io.Class(Of(kMAck), Zz(msg.ack - (bh != nullptr ? bh->ack : 0)));
	int64_t lastcmd = bh != nullptr ? bh->cmdSeq : 0;
	const int64_t known = lastcmd;         // the client holds every command up to this one
	std::optional<int64_t> time;
	uint32_t prev = 0;
	const BaselineTable *bl = &baselines;
	for (const Op &op : msg.ops) {
		const auto t = static_cast<uint32_t>(op.type);
		io.Symbol(ctx::kOp + prev, t);
		prev = t;
		switch (op.type) {
		case OpType::Command:
			io.Class(Of(kMCmdSeq), Zz(op.seq - (lastcmd + 1)));
			lastcmd = op.seq;
			PutCommand(io, op, std::span<const Op>(msg.ops.data(), static_cast<size_t>(&op - msg.ops.data())), known, history);
			break;
		case OpType::SetGame:
			io.String(Of(kStrSetgame), op.text);
			break;
		case OpType::GameState: {
			const GameState &gs = op.gameState;
			io.Class(Of(kMRelSeq), Zz(gs.relSeq));
			io.Class(Of(kMNumCs), gs.configStrings.size());
			int64_t pi = -1;
			for (const ConfigString &cs : gs.configStrings) {
				io.Class(Of(kMCsGap), static_cast<uint64_t>(cs.index - pi - 1));
				pi = cs.index;
				io.String(Of(kStrCs), cs.text);
			}
			PutEntities(io, c, TableReader(gs.baselines), RingReader(EntityRing{}), BaselineTable{}, m);
			io.Class(Of(kMClientNum), Zz(gs.clientNum));
			io.Raw(static_cast<uint32_t>(gs.checksum), 32);
			bl = &gs.baselines;
			break;
		}
		case OpType::Download:
			io.Class(Of(kMDlBlock), op.block);
			if (op.block == 0) io.Class(Of(kMDlSize), Zz(op.fileSize));
			io.Class(Of(kMDlLen), op.data.size());
			for (const uint8_t b : op.data) io.Symbol(Of(kDlByte), b);
			break;
		case OpType::Snapshot: {
			time = op.time;
			c.time = op.time;
			io.Class(Of(kHTime), Zz(op.time - (bh != nullptr && bh->time ? *bh->time : 0)));
			io.Symbol(Of(kHFlags), op.flags);
			io.Symbol(Of(kHArea), op.areas.size());
			for (const uint8_t a : op.areas) io.Symbol(Of(kHArea), a);
			const Snapshot &sn = op.snap;
			const bool pilot = IsPilot(sn.ps);
			io.Symbol(Of(kPsPilot), pilot ? 1 : 0);
			const ImageView basePs = base.snap != nullptr ? base.snap->ps : ImageView(ZeroPlayer());
			if (pilot) PutPlayer(io, c, kPilotFields, ctx::PlayerGap(1), sn.ps, basePs, OlderPlayer(c, false), m);
			else PutPlayer(io, c, kPlayerFields, ctx::PlayerGap(0), sn.ps, basePs, OlderPlayer(c, false), m);
			if (VehicleNum(sn.ps) != 0) {
				const ImageView nv = !sn.vps.empty() ? sn.vps : ImageView(ZeroPlayer());
				const ImageView ov = base.snap != nullptr && !base.snap->vps.empty() ? base.snap->vps : ImageView(ZeroPlayer());
				PutPlayer(io, c, kVehicleFields, ctx::PlayerGap(2), nv, ov, OlderPlayer(c, true), m);
			}
			PutEntities(io, c, RingReader(sn.entities), RingReader(base.snap != nullptr ? base.snap->entities : EntityRing{}), *bl, m);
			break;
		}
		case OpType::Nop:
		case OpType::MapChange:
			break;
		}
	}
	io.Symbol(ctx::kOp + prev, 0);
	rec = HeaderRecord{ .ack = msg.ack, .time = time, .cmdSeq = lastcmd, .older = base.snap };
}

// codes a message into scratch: its size, 0 if a value is outside the model or it does not fit in maxBytes
size_t CodeMessage(std::vector<rans::Encoder::Op> &ops, std::vector<uint8_t> &scratch, const Message &msg, const Base &base,
                   const BaselineTable &baselines, const CommandHistory &history, size_t maxBytes, HeaderRecord &record,
                   const Model &m, bool escaped) {
	for (;;) {
		rans::Encoder e(ops);
		Out io(e, m);
		PutMessage(io, msg, base, baselines, history, record, m, escaped);
		if (e.Overflow()) {
			ops.resize(ops.size() * 2);
			continue;
		}
		return io.Bad() ? 0 : e.Finish(std::span<uint8_t>(scratch.data(), maxBytes));
	}
}

// next operation of a decoded message: reuses the storage of a previous decode (no allocation once warm)
Op &NextOp(Message &msg, size_t &used) {
	if (used == msg.ops.size()) msg.ops.emplace_back();
	Op &op = msg.ops[used++];
	op.seq = 0;
	op.text.clear();
	op.gameState.configStrings.clear();
	op.gameState.baselines = {};
	op.block = 0;
	op.fileSize = 0;
	op.data.clear();
	op.time = 0;
	op.flags = 0;
	op.areas.clear();
	op.snap = {};
	return op;
}

bool GetMessage(In &io, const BaseLookup &lookup, const CommandHistory &history, const BaselineTable &baselines,
                const BaselineTableOut &gamestate, const SnapshotOut &out, Message &msg, uint32_t &delta, HeaderRecord &rec,
                const Model &m) {
	delta = io.Symbol(Of(kMDelta));
	const bool escaped = delta >= kEscapedDelta;
	if (escaped) delta -= kEscapedDelta;
	const Snapshot *base = nullptr;
	const HeaderRecord *bh = nullptr;
	if (delta != 0 && !lookup.Find(delta, base, bh)) return false;
	PredCtx c;
	c.escaped = escaped;
	c.btime = bh != nullptr ? bh->time : std::nullopt;
	c.olderSnap = bh != nullptr ? bh->older : nullptr;
	msg.ack = (bh != nullptr ? bh->ack : 0) + Unzz(io.Class(Of(kMAck)));
	int64_t lastcmd = bh != nullptr ? bh->cmdSeq : 0;
	const int64_t known = lastcmd;
	std::optional<int64_t> time;
	uint32_t prev = 0;
	size_t used = 0;
	bool hadSnapshot = false;
	BaselineTable table = baselines;   // after a gamestate op, its baselines
	for (;;) {
		const uint32_t t = io.Symbol(ctx::kOp + prev);
		prev = t;
		if (t == 0) break;
		if (used == kMaxOps) return false;
		Op &op = NextOp(msg, used);
		op.type = static_cast<OpType>(t);
		switch (op.type) {
		case OpType::Command:
			op.seq = lastcmd + 1 + Unzz(io.Class(Of(kMCmdSeq)));
			lastcmd = op.seq;
			if (!GetCommand(io, op, std::span<const Op>(msg.ops.data(), used - 1), known, history)) return false;
			break;
		case OpType::SetGame:
			if (!io.String(Of(kStrSetgame), op.text)) return false;
			break;
		case OpType::GameState: {
			GameState &gs = op.gameState;
			gs.relSeq = Unzz(io.Class(Of(kMRelSeq)));
			const uint64_t n = io.Class(Of(kMNumCs));
			int64_t pi = -1;
			for (uint64_t k = 0; k < n; k++) {
				const int64_t idx = pi + 1 + static_cast<int64_t>(io.Class(Of(kMCsGap)));
				if (idx > 0xFFFF) return false;
				pi = idx;
				ConfigString &cs = gs.configStrings.emplace_back();
				cs.index = static_cast<uint16_t>(idx);
				if (!io.String(Of(kStrCs), cs.text)) return false;
			}
			TableSink sink(gamestate);
			for (uint32_t e = 0; e < kMaxEntities; e++) std::ranges::fill(gamestate.bytes.subspan(e * gamestate.stride, kEntityBytes), uint8_t{ 0 });
			if (!GetEntities(io, c, RingReader(EntityRing{}), BaselineTable{}, sink, m)) return false;
			table = BaselineTable{ .bytes = gamestate.bytes, .stride = gamestate.stride };
			gs.baselines = table;
			gs.clientNum = Unzz(io.Class(Of(kMClientNum)));
			gs.checksum = S32(static_cast<uint32_t>(io.Raw(32)));
			break;
		}
		case OpType::Download: {
			op.block = static_cast<uint32_t>(io.Class(Of(kMDlBlock)));
			op.fileSize = op.block == 0 ? Unzz(io.Class(Of(kMDlSize))) : 0;
			op.data.resize(static_cast<uint32_t>(io.Class(Of(kMDlLen))));   // below 2^19 (20 classes)
			for (uint8_t &b : op.data) b = static_cast<uint8_t>(io.Symbol(Of(kDlByte)));
			break;
		}
		case OpType::Snapshot: {
			if (hadSnapshot) return false;   // one snapshot per message: it is written to the caller's storage
			hadSnapshot = true;
			op.time = (bh != nullptr && bh->time ? *bh->time : 0) + Unzz(io.Class(Of(kHTime)));
			time = op.time;
			c.time = op.time;
			op.flags = io.Symbol(Of(kHFlags));
			op.areas.resize(io.Symbol(Of(kHArea)));
			for (uint8_t &a : op.areas) a = static_cast<uint8_t>(io.Symbol(Of(kHArea)));
			Snapshot &sn = op.snap;
			const bool pilot = io.Symbol(Of(kPsPilot)) != 0;
			const ImageView basePs = base != nullptr ? base->ps : ImageView(ZeroPlayer());
			const ImageRef ps = out.ps.first(kPlayerBytes);
			const bool okPs = pilot ? GetPlayer(io, c, kPilotFields, ctx::PlayerGap(1), basePs, ps, OlderPlayer(c, false), m)
			                        : GetPlayer(io, c, kPlayerFields, ctx::PlayerGap(0), basePs, ps, OlderPlayer(c, false), m);
			if (!okPs) return false;
			sn.ps = ps;
			if (VehicleNum(ps) != 0) {
				const ImageView ov = base != nullptr && !base->vps.empty() ? base->vps : ImageView(ZeroPlayer());
				const ImageRef vps = out.vps.first(kPlayerBytes);
				if (!GetPlayer(io, c, kVehicleFields, ctx::PlayerGap(2), ov, vps, OlderPlayer(c, true), m)) return false;
				sn.vps = vps;
			}
			RingSink sink(out);
			if (!GetEntities(io, c, RingReader(base != nullptr ? base->entities : EntityRing{}), table, sink, m)) return false;
			sn.entities = sink.View();
			break;
		}
		case OpType::Nop:
		case OpType::MapChange:
			break;
		}
	}
	msg.ops.resize(used);
	rec = HeaderRecord{ .ack = msg.ack, .time = time, .cmdSeq = lastcmd, .older = base };
	return true;
}

constexpr auto kEntityOffsets = [] {
	std::array<uint16_t, kNumEntityFields> a{};
	for (uint32_t i = 0; i < kNumEntityFields; i++) a[i] = kEntityFields[i].offset;
	return a;
}();

} // namespace
} // namespace detail

std::span<const uint16_t> EntityFieldOffsets() noexcept { return detail::kEntityOffsets; }
std::span<const uint16_t> PlayerFieldOffsets() noexcept { return detail::kPlayerAllOffsets; }

bool MessageEncoder::Encode(const Message &msg, const Base &base, const BaselineTable &baselines,
                            const CommandHistory &history, size_t maxBytes,
                            std::vector<uint8_t> &out, HeaderRecord &record) {
	const detail::Model &m = detail::Model::Get();
	if (base.delta > 255 || (base.delta != 0) != (base.snap != nullptr)) return false;
	if (ops_.empty()) ops_.resize(4096);
	if (scratch_.size() < maxBytes) scratch_.resize(maxBytes);   // grown once: no clearing per message
	size_t n = detail::CodeMessage(ops_, scratch_, msg, base, baselines, history, maxBytes, record, m, false);
	if (n != 0) out.assign(scratch_.begin(), scratch_.begin() + static_cast<std::ptrdiff_t>(n));
	// worst case: a large message (none in the recordings comes close) is also coded raw, and the smaller one is sent
	if (n == 0 || n > kEscapeTrial) [[unlikely]] {
		const size_t r = detail::CodeMessage(ops_, scratch_, msg, base, baselines, history, maxBytes, record, m, true);
		if (r != 0 && (n == 0 || r < n)) {
			out.assign(scratch_.begin(), scratch_.begin() + static_cast<std::ptrdiff_t>(r));
			n = r;
		}
	}
	return n != 0;
}

SymbolCounts NewSymbolCounts() {
	SymbolCounts counts;
	counts.symbols.resize(detail::ctx::kCount);
	for (uint32_t c = 0; c < detail::ctx::kCount; c++) counts.symbols[c].resize(detail::ctx::Alphabet(c));
	counts.rawBits.resize(detail::ctx::kCount);
	return counts;
}

bool CountMessage(const Message &msg, const Base &base, const BaselineTable &baselines,
                  const CommandHistory &history, HeaderRecord &record, SymbolCounts &counts) {
	if (base.delta > 255 || (base.delta != 0) != (base.snap != nullptr) || counts.symbols.size() != detail::ctx::kCount ||
	    counts.rawBits.size() != detail::ctx::kCount)
		return false;
	detail::CountOut io(counts);
	detail::PutMessage(io, msg, base, baselines, history, record, detail::Model::Get(), false);
	return !io.Bad();
}

bool DecodeMessage(std::span<const uint8_t> bytes, const BaseLookup &lookup, const CommandHistory &history,
                   const BaselineTable &baselines, const BaselineTableOut &gamestate, const SnapshotOut &snapshot,
                   Message &msg, uint32_t &delta, HeaderRecord &record) {
	const detail::Model &m = detail::Model::Get();
	detail::In io(bytes, m);
	return io.Ok() && detail::GetMessage(io, lookup, history, baselines, gamestate, snapshot, msg, delta, record, m);
}


} // namespace netcodec
