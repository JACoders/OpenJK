// netcodec: whole-message codec, C++ port of tools/python/codec.py and codec2.py (see codec.h).
// Function names follow the Python ones (put_field = PutField, ...), so that both can be read side by side.
#include "netcodec/codec.h"

#include <bit>
#include <cmath>
#include <cstring>

namespace netcodec {
namespace {

// ------------------------------------------------------------------ schema and tables (generated)
struct FieldDesc {
	uint16_t offset;    // in the state image
	int8_t bits;        // codec width: 0 float, < 0 signed
	uint8_t ctype;      // CType
	uint8_t absolute;   // 1: absolute mode, 0: delta against the base
	uint8_t pmode;      // PredMode
	uint16_t ctx;       // row of the field's contexts (kEntityFieldCtx or kPlayerFieldCtx)
	uint16_t relType;   // position prediction: offsets of trType, time, velocity, and 0 entity / 1 player state
	uint16_t relTime;
	uint16_t relVel;
	uint16_t relKind;
};
enum CType : uint8_t { kInt = 0, kFloat = 1, kBool = 2, kEnum = 3 };
enum PredMode : uint8_t { kPredNone = 0, kPredTime = 1, kPredPos = 2, kPredAngle = 3 };
enum FieldCtx : uint32_t { kA, kD, kK, kIA, kID, kAD, kFD, kT, kFP, kA2, kB, kFieldCtxCount };
using CtxRow = std::array<uint16_t, kFieldCtxCount>;   // table of each context kind of one field

#include "generated/jka_codec.inc"

static_assert(kEntitySize == kEntityBytes && kPlayerSize == kPlayerBytes, "schema and codec.h disagree");
constexpr uint32_t kNumEntityFields = ToU32(std::size(kEntityFields));
constexpr uint32_t kTrInterpolate = 1, kTrLinearStop = 3;
constexpr uint32_t kEfNodraw = 1u << 8;
constexpr double kA16 = 360.0 / 65536;
constexpr size_t kMaxOps = 4096;               // decoder limits against malformed input
constexpr size_t kMaxString = 1u << 16;

struct Model {
	std::vector<uint16_t> cum;
	std::vector<rans::EncSymbol> enc;
	std::vector<std::array<uint16_t, rans::kCoarseSize>> coarse;
	std::vector<rans::Table> tables;
	int8_t trTypeBits = 8;
};

const Model &GetModel() {
	static const Model model = [] {
		Model m;
		const uint32_t total = kTableStart[kTableCount];
		m.cum.resize(total + kTableCount);
		m.enc.resize(total);
		m.coarse.resize(kTableCount);
		m.tables.reserve(kTableCount);
		for (uint32_t t = 0; t < kTableCount; t++) {
			const uint32_t first = kTableStart[t], K = kTableStart[t + 1] - first;
			const std::span<uint16_t> cum(m.cum.data() + first + t, K + 1);
			cum[0] = 0;
			for (uint32_t s = 0; s < K; s++) {
				cum[s + 1] = static_cast<uint16_t>(cum[s] + kFreq[first + s]);
				m.enc[first + s] = rans::MakeEncSymbol(cum[s], kFreq[first + s]);
			}
			rans::BuildCoarse(cum, m.coarse[t]);
			m.tables.push_back(rans::Table{ cum, std::span<const rans::EncSymbol>(m.enc.data() + first, K), m.coarse[t] });
		}
		for (const FieldDesc &f : kEntityFields)
			if (f.relType != 0xFFFF)
				for (const FieldDesc &g : kEntityFields)
					if (g.offset == f.relType) m.trTypeBits = g.bits;
		return m;
	}();
	return model;
}

// ------------------------------------------------------------------ values
using ImageView = std::span<const uint8_t>;
using ImageRef = std::span<uint8_t>;

const PlayerState &ZeroPlayer() { static const PlayerState z{}; return z; }
const EntityState &ZeroEntity() { static const EntityState z{}; return z; }

uint32_t Load(ImageView img, uint32_t offset) {
	uint32_t v = 0;
	std::memcpy(&v, img.subspan(offset, 4).data(), 4);   // little-endian hosts (x86, x64)
	return v;
}

void StoreWord(ImageRef img, uint32_t offset, uint32_t v) { std::memcpy(img.subspan(offset, 4).data(), &v, 4); }

// value of a field as the Python codec sees it: low bits of an unsigned field, sign-extended signed field
int64_t ValueBits(uint32_t w, int bits) {
	if (bits == 0 || bits == 32) return w;
	if (bits > 0) return w & ((1u << bits) - 1);
	const uint32_t ab = static_cast<uint32_t>(-bits);
	if (ab == 32) return static_cast<int32_t>(w);
	const uint32_t x = w & ((1u << ab) - 1);
	return (x >> (ab - 1)) ? static_cast<int64_t>(x) - (int64_t{ 1 } << ab) : static_cast<int64_t>(x);
}

int64_t Value(ImageView img, const FieldDesc &f) { return ValueBits(Load(img, f.offset), f.bits); }
void Store(ImageRef img, const FieldDesc &f, int64_t v) { StoreWord(img, f.offset, static_cast<uint32_t>(v)); }

int64_t S32(uint32_t w) { return static_cast<int32_t>(w); }
int64_t S32(int64_t v) { return static_cast<int32_t>(static_cast<uint32_t>(v)); }
// the float32 whose bit pattern is bitsValue, widened to double as in Python (exact)
double F32(int64_t bitsValue) { return static_cast<double>(std::bit_cast<float>(static_cast<uint32_t>(bitsValue))); }
uint32_t Pack(double x) { return std::bit_cast<uint32_t>(static_cast<float>(x)); }
int64_t Rnd(double x) { return static_cast<int64_t>(std::floor(x + 0.5)); }
uint64_t Zz(int64_t d) { return d >= 0 ? static_cast<uint64_t>(d) << 1 : (static_cast<uint64_t>(-(d + 1)) << 1) + 1; }
int64_t Unzz(uint64_t x) { return (x & 1) == 0 ? static_cast<int64_t>(x >> 1) : -static_cast<int64_t>((x + 1) >> 1); }
int64_t PyMod(int64_t a, int64_t m) { const int64_t r = a % m; return r < 0 ? r + m : r; }
int64_t FloorDiv(int64_t a, int64_t b) {
	const int64_t q = a / b;
	return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

// 0 zero, 1 integral in [-4096, 4096), 2 other (3 = angle index is decided by the caller)
uint32_t FKind(int64_t b) {
	if (b == 0) return 0;
	const double f = F32(b);
	return (f == f && f >= -4096 && f < 4096 && std::trunc(f) == f) ? 1 : 2;
}

bool IsA16(int64_t b) {
	const double f = F32(b);
	if (f != f || std::fabs(f) > 720) return false;
	return Pack(static_cast<double>(Rnd(f / kA16)) * kA16) == static_cast<uint32_t>(b);
}

int64_t OldIndex(int64_t ov) {
	const double f = F32(ov);
	return (ov != 0 && f == f && std::fabs(f) <= 720) ? Rnd(f / kA16) : 0;
}

// ------------------------------------------------------------------ coder wrappers (io.s, io.r, cls_put / cls_get)
class Out {
public:
	Out(rans::Encoder &e, const Model &m) noexcept : e_(e), m_(m) {}

	void S(uint32_t ctx, uint64_t sym) noexcept {
		const rans::Table &t = m_.tables[ctx];
		if (sym >= t.K()) { bad_ = true; return; }
		e_.Put(t.enc[static_cast<uint32_t>(sym)]);   // sym < K, checked above
	}
	void R(uint64_t v, uint32_t n) noexcept {      // raw bits; 30-bit pieces keep the 15-bit chunks of Python
		while (n > 30) { e_.PutRaw(static_cast<uint32_t>(v & 0x3FFFFFFF), 30); v >>= 30; n -= 30; }
		e_.PutRaw(static_cast<uint32_t>(v), n);
	}
	void Cls(uint32_t ctx, uint64_t x) noexcept {
		const uint32_t c = static_cast<uint32_t>(std::bit_width(x));
		S(ctx, c);
		if (c > 1) R(x - (uint64_t{ 1 } << (c - 1)), c - 1);
	}
	void Str(uint32_t ctx, const std::string &text) noexcept {
		for (const char ch : text) {
			const uint8_t b = static_cast<uint8_t>(ch);
			if (b == 0) bad_ = true;
			S(ctx, b);
		}
		S(ctx, 0);
	}
	[[nodiscard]] bool Bad() const noexcept { return bad_; }

private:
	rans::Encoder &e_;
	const Model &m_;
	bool bad_ = false;
};

class In {
public:
	In(std::span<const uint8_t> bytes, const Model &m) noexcept : m_(m) { ok_ = d_.Init(bytes); }

	uint32_t S(uint32_t ctx) noexcept { return rans::Get(d_, m_.tables[ctx]); }
	uint64_t R(uint32_t n) noexcept {
		uint64_t v = 0;
		uint32_t sh = 0;
		while (n > 30) { v |= uint64_t{ d_.GetRaw(30) } << sh; sh += 30; n -= 30; }
		return v | (uint64_t{ d_.GetRaw(n) } << sh);
	}
	uint64_t Cls(uint32_t ctx) noexcept {
		const uint32_t c = S(ctx);
		if (c <= 1) return c;
		return (uint64_t{ 1 } << (c - 1)) | R(c - 1);
	}
	bool Str(uint32_t ctx, std::string &text) {
		text.clear();
		for (;;) {
			const uint32_t b = S(ctx);
			if (b == 0) return true;
			if (text.size() == kMaxString) return false;
			text.push_back(static_cast<char>(b));
		}
	}
	[[nodiscard]] bool Ok() const noexcept { return ok_; }

private:
	rans::Decoder d_;
	const Model &m_;
	bool ok_ = false;
};

// ------------------------------------------------------------------ prediction context (CTX in Python)
struct PredCtx {
	std::optional<int64_t> time;           // serverTime of the message's snapshot, once coded
	std::optional<int64_t> btime;          // serverTime of the base
	const Snapshot *olderSnap = nullptr;   // the base's own base
	ImageView cur, old;                    // state being coded (partly decoded when decoding) and its base state
	const uint8_t *older = nullptr;        // the base state's own base state (kPlayerBytes or kEntityBytes), or none
	bool pred = false;                     // the base state is the same object's previous state
};

std::optional<uint32_t> PredictPos(const PredCtx &c, const FieldDesc &f, const Model &m) {
	if (!c.pred) return std::nullopt;
	int64_t dt = 0;
	if (f.relKind == 0) {
		const int64_t tn = ValueBits(Load(c.cur, f.relType), m.trTypeBits), to = ValueBits(Load(c.old, f.relType), m.trTypeBits);
		if (tn == to && tn == kTrLinearStop) dt = S32(Load(c.cur, f.relTime)) - S32(Load(c.old, f.relTime));
		else if (tn == to && tn == kTrInterpolate && c.btime && c.time) dt = *c.time - *c.btime;
		else return std::nullopt;
	} else {
		dt = S32(Load(c.cur, f.relTime)) - S32(Load(c.old, f.relTime));
	}
	const double o = F32(Load(c.old, f.offset)), vo = F32(Load(c.old, f.relVel)), vn = F32(Load(c.cur, f.relVel));
	const auto fin = [](double x) { return x == x && std::fabs(x) < (1 << 24); };
	if (!(fin(o) && fin(vo) && fin(vn) && (dt < 0 ? -dt : dt) < 65536)) return std::nullopt;
	const auto r = [](double x) { return static_cast<int64_t>(std::floor(x * 65536.0 + 0.5)); };   // 1/65536 units
	const int64_t p = r(o) + FloorDiv((r(vo) + r(vn)) * dt, 2000);
	return Pack(static_cast<double>(p) / 65536.0);
}

std::optional<int64_t> PredictAngle(const PredCtx &c, const FieldDesc &f, int64_t ov) {
	if (!c.pred || c.older == nullptr) return std::nullopt;
	uint32_t o2 = 0;
	std::memcpy(&o2, c.older + f.offset, 4);
	if (!(IsA16(ov) && IsA16(o2))) return std::nullopt;
	return 2 * OldIndex(ov) - OldIndex(o2);
}

// ------------------------------------------------------------------ fields
void PutInt(Out &io, const CtxRow &row, int bits, int64_t nv, int64_t ov, bool absolute) {
	const uint32_t ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const int64_t M = int64_t{ 1 } << ab;
	const bool sgn = bits < 0;
	if (absolute) {
		if (ab <= 8) io.S(row[kA], static_cast<uint64_t>(nv & (M - 1)));
		else io.Cls(row[kA], sgn ? Zz(nv) : static_cast<uint64_t>(nv));
	} else {
		io.Cls(row[kD], Zz(PyMod(nv - ov + M / 2, M) - M / 2));
	}
}

int64_t GetInt(In &io, const CtxRow &row, int bits, int64_t ov, bool absolute) {
	const uint32_t ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const int64_t M = int64_t{ 1 } << ab;
	const bool sgn = bits < 0;
	if (absolute) {
		if (ab <= 8) {
			const int64_t v = io.S(row[kA]);
			return (sgn && v >= M / 2) ? v - M : v;
		}
		const uint64_t x = io.Cls(row[kA]);
		return sgn ? Unzz(x) : static_cast<int64_t>(x);
	}
	const int64_t v = ov + Unzz(io.Cls(row[kD]));
	return sgn ? PyMod(v + M / 2, M) - M / 2 : PyMod(v, M);
}

void PutFloat(Out &io, const CtxRow &row, int64_t nv, int64_t ov, bool absolute) {
	uint32_t k = FKind(nv);
	if (k == 2 && IsA16(nv)) k = 3;
	io.S(row[kK], k);
	const uint32_t ok = FKind(ov);
	if (k == 1) {
		const int64_t t = static_cast<int64_t>(F32(nv)), ot = ok == 1 ? static_cast<int64_t>(F32(ov)) : 0;
		if (absolute) io.Cls(row[kIA], Zz(t));
		else io.Cls(row[kID], Zz(t - ot));
	} else if (k == 3) {
		io.Cls(row[kAD], Zz(Rnd(F32(nv) / kA16) - OldIndex(ov)));
	} else if (k == 2) {
		if (absolute) io.R(static_cast<uint32_t>(nv), 32);
		else io.Cls(row[kFD], Zz(S32(nv) - (ok != 0 ? S32(ov) : 0)));
	}
}

int64_t GetFloatKind(In &io, const CtxRow &row, int64_t ov, bool absolute, uint32_t k) {
	const uint32_t ok = FKind(ov);
	if (k == 0) return 0;
	if (k == 1) {
		const int64_t ot = ok == 1 ? static_cast<int64_t>(F32(ov)) : 0;
		const int64_t t = absolute ? Unzz(io.Cls(row[kIA])) : ot + Unzz(io.Cls(row[kID]));
		return Pack(static_cast<double>(t));
	}
	if (k == 3) return Pack(static_cast<double>(OldIndex(ov) + Unzz(io.Cls(row[kAD]))) * kA16);
	if (absolute) return static_cast<int64_t>(io.R(32));
	return ((ok != 0 ? S32(ov) : 0) + Unzz(io.Cls(row[kFD]))) & 0xFFFFFFFF;
}

void PutField(Out &io, const PredCtx &c, const FieldDesc &f, const CtxRow &row, int64_t nv, int64_t ov, const Model &m) {
	if (f.ctype == kBool) {                 // only changed fields are coded: 0/1 to 1/0 needs no value
		const bool flip = (nv == 0 || nv == 1) && (ov == 0 || ov == 1);
		io.S(row[kB], flip ? 0 : 1);
		if (flip) return;
	}
	if (f.pmode == kPredTime && f.bits == 32 && c.time) {
		const int64_t d = PyMod(nv - (*c.time & 0xFFFFFFFF) + (int64_t{ 1 } << 31), int64_t{ 1 } << 32) - (int64_t{ 1 } << 31);
		io.Cls(row[kT], Zz(d));
		return;
	}
	if (f.pmode != kPredNone && f.bits == 0) {
		uint32_t k = FKind(nv);
		if (k == 2 && IsA16(nv)) k = 3;
		if (f.pmode == kPredPos && k == 2) {
			if (const auto p = PredictPos(c, f, m)) {
				io.S(row[kK], 2);
				io.Cls(row[kFP], Zz(S32(nv) - S32(*p)));
				return;
			}
		}
		if (f.pmode == kPredAngle && k == 3) {
			if (const auto p = PredictAngle(c, f, ov)) {
				io.S(row[kK], 3);
				io.Cls(row[kA2], Zz(Rnd(F32(nv) / kA16) - *p));
				return;
			}
		}
	}
	if (f.bits == 0) PutFloat(io, row, nv, ov, f.absolute != 0);
	else PutInt(io, row, f.bits, nv, ov, f.absolute != 0);
}

int64_t GetField(In &io, const PredCtx &c, const FieldDesc &f, const CtxRow &row, int64_t ov, const Model &m) {
	if (f.ctype == kBool && io.S(row[kB]) == 0) return ov ^ 1;
	if (f.pmode == kPredTime && f.bits == 32 && c.time)
		return PyMod((*c.time & 0xFFFFFFFF) + Unzz(io.Cls(row[kT])), int64_t{ 1 } << 32);
	if ((f.pmode == kPredPos || f.pmode == kPredAngle) && f.bits == 0) {
		const uint32_t k = io.S(row[kK]);
		if (k == 2 && f.pmode == kPredPos) {
			if (const auto p = PredictPos(c, f, m)) return (S32(*p) + Unzz(io.Cls(row[kFP]))) & 0xFFFFFFFF;
		}
		if (k == 3 && f.pmode == kPredAngle) {
			if (const auto p = PredictAngle(c, f, ov)) return Pack(static_cast<double>(*p + Unzz(io.Cls(row[kA2]))) * kA16);
		}
		return GetFloatKind(io, row, ov, f.absolute != 0, k);
	}
	if (f.bits == 0) return GetFloatKind(io, row, ov, f.absolute != 0, io.S(row[kK]));
	return GetInt(io, row, f.bits, ov, f.absolute != 0);
}

// ------------------------------------------------------------------ player states (ps_put2 / ps_get2)
bool AnyNonZero(ImageView ps) {
	for (const uint16_t off : kPlayerAllOffsets)
		if (Load(ps, off) != 0) return true;
	return false;
}

int64_t VehicleNum(ImageView ps) { return ValueBits(Load(ps, kVehicleNumOffset), 10); }
bool IsPilot(ImageView ps) { return VehicleNum(ps) != 0 && (Load(ps, kPlayerEFlagsOffset) & kEfNodraw) != 0; }

const uint8_t *OlderPlayer(const PredCtx &c, bool vehicle) {
	const Snapshot *o = c.olderSnap;
	if (o == nullptr) return nullptr;
	if (!vehicle) return o->ps.data();
	if (VehicleNum(o->ps) != 0 && o->hasVehicle) return o->vps.data();
	return ZeroPlayer().data();
}

void PutPlayer(Out &io, PredCtx &c, std::span<const FieldDesc> fields, std::span<const uint16_t> gap, ImageView nw, ImageView old,
               const uint8_t *older, const Model &m) {
	c.cur = nw; c.old = old; c.older = older; c.pred = AnyNonZero(old);
	const uint32_t n = ToU32(fields.size());
	int64_t prev = -1;
	for (uint32_t i = 0; i < n; i++) {
		const FieldDesc &f = fields[i];
		const int64_t a = Value(nw, f), b = Value(old, f);
		if (a == b) continue;
		io.S(gap[static_cast<size_t>(prev + 1)], i); prev = i;
		PutField(io, c, f, kPlayerFieldCtx[f.ctx], a, b, m);
	}
	io.S(gap[static_cast<size_t>(prev + 1)], n);
}

bool GetPlayer(In &io, PredCtx &c, std::span<const FieldDesc> fields, std::span<const uint16_t> gap, ImageView old, PlayerState &out,
               const uint8_t *older, const Model &m) {
	std::memcpy(out.data(), old.data(), kPlayerBytes);
	c.cur = out; c.old = old; c.older = older; c.pred = AnyNonZero(old);
	const uint32_t n = ToU32(fields.size());
	int64_t prev = -1;
	for (;;) {
		const uint32_t i = io.S(gap[static_cast<size_t>(prev + 1)]);
		if (i == n) return true;
		if (static_cast<int64_t>(i) <= prev) return false;
		const FieldDesc &f = fields[i];
		Store(out, f, GetField(io, c, f, kPlayerFieldCtx[f.ctx], Value(old, f), m));
		prev = i;
	}
}

// ------------------------------------------------------------------ entities (ents_put / ents_get)
uint32_t ClassOf(ImageView st) {
	const int64_t et = Value(st, kEntityFields[kEntityTypeIndex]);
	return et < 18 ? static_cast<uint32_t>(et) : 18;
}

const Entity *FindEntity(std::span<const Entity> list, uint32_t num) {
	size_t lo = 0, hi = list.size();
	while (lo < hi) {
		const size_t mid = (lo + hi) / 2;
		if (list[mid].number < num) lo = mid + 1; else hi = mid;
	}
	return lo < list.size() && list[lo].number == num ? &list[lo] : nullptr;
}

bool Changed(ImageView a, ImageView b) {
	if (std::memcmp(a.data(), b.data(), kEntityBytes) == 0) return false;
	for (const FieldDesc &f : kEntityFields)
		if (Value(a, f) != Value(b, f)) return true;
	return false;
}

void PutEntity(Out &io, PredCtx &c, ImageView nw, ImageView basest, const Model &m) {
	const uint32_t bcls = ClassOf(basest), ncls = ClassOf(nw);
	const FieldDesc &et = kEntityFields[kEntityTypeIndex];
	const int64_t etNew = Value(nw, et);
	const bool etChanged = etNew != Value(basest, et);
	io.S(kTypeChangedCtx[bcls], etChanged ? 1 : 0);
	if (etChanged) io.S(kTypeValueCtx[bcls], static_cast<uint64_t>(etNew));
	int64_t prev = -1;
	for (uint32_t i = 0; i < kNumEntityFields; i++) {
		if (i == kEntityTypeIndex) continue;
		const FieldDesc &f = kEntityFields[i];
		const int64_t a = Value(nw, f), b = Value(basest, f);
		if (a == b) continue;
		io.S(kEntityGapCtx[ncls][static_cast<size_t>(prev + 1)], i); prev = i;
		PutField(io, c, f, kEntityFieldCtx[f.ctx], a, b, m);
	}
	io.S(kEntityGapCtx[ncls][static_cast<size_t>(prev + 1)], kNumEntityFields);
}

void PutEntities(Out &io, PredCtx &c, std::span<const Entity> nw, std::span<const Entity> old,
                 std::span<const Entity> baselines, const Model &m) {
	size_t i = 0, j = 0;
	int64_t prevnum = -1;
	uint32_t prevop = 0;
	while (i < nw.size() || j < old.size()) {
		const uint32_t nn = i < nw.size() ? nw[i].number : kMaxEntities, on = j < old.size() ? old[j].number : kMaxEntities;
		const uint32_t num = nn < on ? nn : on;
		const Entity *ne = nn == num ? &nw[i++] : nullptr;
		const Entity *oe = on == num ? &old[j++] : nullptr;
		uint32_t op = 0;
		ImageView basest;
		if (ne && oe) {
			if (!Changed(ne->state, oe->state)) continue;
			op = 0; basest = oe->state;
		} else if (ne) {
			const Entity *b = FindEntity(baselines, num);
			basest = b ? ImageView(b->state) : ImageView(ZeroEntity());
			op = Changed(ne->state, basest) ? 1 : 3;
		} else {
			op = 2;
		}
		const int64_t gap = num - prevnum - 1;
		prevnum = num;
		if (gap < 40) io.S(kEntGap, static_cast<uint64_t>(gap));
		else { io.S(kEntGap, 40); io.R(static_cast<uint64_t>(gap), 10); }
		io.S(kEntityOpCtx[prevop], op); prevop = op;
		if (op == 2 || op == 3) continue;
		c.cur = ne->state; c.old = basest; c.pred = op == 0;
		const Entity *older = op == 0 && c.olderSnap ? FindEntity(c.olderSnap->entities, num) : nullptr;
		c.older = older ? older->state.data() : nullptr;
		PutEntity(io, c, ne->state, basest, m);
	}
	io.S(kEntGap, 41);
}

bool GetEntities(In &io, PredCtx &c, std::span<const Entity> old, std::span<const Entity> baselines,
                 std::vector<Entity> &out, const Model &m) {
	out.clear();
	size_t j = 0;
	int64_t prevnum = -1;
	uint32_t prevop = 0;
	for (;;) {
		const uint32_t g = io.S(kEntGap);
		if (g == 41) break;
		const uint64_t gap = g == 40 ? io.R(10) : g;
		const uint64_t num = static_cast<uint64_t>(prevnum + 1) + gap;
		if (num >= kMaxEntities) return false;
		prevnum = static_cast<int64_t>(num);
		while (j < old.size() && old[j].number < num) out.push_back(old[j++]);
		const Entity *oe = j < old.size() && old[j].number == num ? &old[j] : nullptr;
		if (oe) j++;
		const uint32_t op = io.S(kEntityOpCtx[prevop]);
		prevop = op;
		if (op == 2) {
			if (!oe) return false;
			continue;
		}
		if (op == 0 && !oe) return false;
		ImageView basest;
		if (op == 0) basest = oe->state;
		else {
			const Entity *b = FindEntity(baselines, static_cast<uint32_t>(num));
			basest = b ? ImageView(b->state) : ImageView(ZeroEntity());
		}
		Entity &st = out.emplace_back();
		st.number = static_cast<uint16_t>(num);
		std::memcpy(st.state.data(), basest.data(), kEntityBytes);
		if (op == 3) continue;
		c.cur = st.state; c.old = basest; c.pred = op == 0;
		const Entity *older = op == 0 && c.olderSnap ? FindEntity(c.olderSnap->entities, static_cast<uint32_t>(num)) : nullptr;
		c.older = older ? older->state.data() : nullptr;
		const uint32_t bcls = ClassOf(basest);
		if (io.S(kTypeChangedCtx[bcls]) != 0) Store(st.state, kEntityFields[kEntityTypeIndex], io.S(kTypeValueCtx[bcls]));
		const uint32_t ncls = ClassOf(st.state);
		int64_t prev = -1;
		for (;;) {
			const uint32_t i = io.S(kEntityGapCtx[ncls][static_cast<size_t>(prev + 1)]);
			if (i == kNumEntityFields) break;
			if (static_cast<int64_t>(i) <= prev || i == kEntityTypeIndex) return false;
			const FieldDesc &f = kEntityFields[i];
			Store(st.state, f, GetField(io, c, f, kEntityFieldCtx[f.ctx], Value(basest, f), m));
			prev = i;
		}
	}
	while (j < old.size()) out.push_back(old[j++]);
	return true;
}

// ------------------------------------------------------------------ messages (msg_put / msg_get)
void PutMessage(Out &io, const Message &msg, const Base &base, const std::vector<Entity> &baselines, HeaderRecord &rec,
                const Model &m) {
	const HeaderRecord *bh = base.header;
	io.S(kMDelta, base.delta);
	PredCtx c;
	c.btime = bh ? bh->time : std::nullopt;
	c.olderSnap = bh ? bh->older : nullptr;
	io.Cls(kMAck, Zz(msg.ack - (bh ? bh->ack : 0)));
	int64_t lastcmd = bh ? bh->cmdSeq : 0, cmdseq = lastcmd;
	std::optional<int64_t> time;
	uint32_t prev = 0;
	const std::vector<Entity> *bl = &baselines;
	const std::span<const Entity> noEntities;
	for (const Op &op : msg.ops) {
		const uint32_t t = static_cast<uint32_t>(op.type);
		io.S(kOpCtx[prev], t); prev = t;
		switch (op.type) {
		case OpType::Command:
			io.Cls(kMCmdSeq, Zz(op.seq - (lastcmd + 1))); lastcmd = op.seq; cmdseq = lastcmd;
			io.Str(kStrCmd, op.text);
			break;
		case OpType::SetGame:
			io.Str(kStrSetgame, op.text);
			break;
		case OpType::GameState: {
			const GameState &gs = op.gameState;
			io.Cls(kMRelSeq, Zz(gs.relSeq));
			io.Cls(kMNumCs, gs.configStrings.size());
			int64_t pi = -1;
			for (const ConfigString &cs : gs.configStrings) {
				io.Cls(kMCsGap, static_cast<uint64_t>(cs.index - pi - 1)); pi = cs.index;
				io.Str(kStrCs, cs.text);
			}
			PutEntities(io, c, gs.baselines, noEntities, noEntities, m);
			io.Cls(kMClientNum, Zz(gs.clientNum));
			io.R(static_cast<uint32_t>(gs.checksum), 32);
			bl = &gs.baselines;
			break;
		}
		case OpType::Download:
			io.Cls(kMDlBlock, op.block);
			if (op.block == 0) io.Cls(kMDlSize, Zz(op.fileSize));
			io.Cls(kMDlLen, op.data.size());
			for (const uint8_t b : op.data) io.S(kDlByte, b);
			break;
		case OpType::Snapshot: {
			time = op.time; c.time = op.time;
			io.Cls(kHTime, Zz(op.time - (bh && bh->time ? *bh->time : 0)));
			io.S(kHFlags, op.flags);
			io.S(kHArea, op.areas.size());
			for (const uint8_t a : op.areas) io.S(kHArea, a);
			const Snapshot &sn = op.snap;
			const bool pil = IsPilot(sn.ps);
			io.S(kPsPilot, pil ? 1 : 0);
			const PlayerState &basePs = base.snap ? base.snap->ps : ZeroPlayer();
			if (pil) PutPlayer(io, c, kPilotFields, kPlayerGapCtx1, sn.ps, basePs, OlderPlayer(c, false), m);
			else PutPlayer(io, c, kPlayerFields, kPlayerGapCtx0, sn.ps, basePs, OlderPlayer(c, false), m);
			if (VehicleNum(sn.ps) != 0) {
				const PlayerState &nv = sn.hasVehicle ? sn.vps : ZeroPlayer();
				const PlayerState &ov = base.snap && base.snap->hasVehicle ? base.snap->vps : ZeroPlayer();
				PutPlayer(io, c, kVehicleFields, kPlayerGapCtx2, nv, ov, OlderPlayer(c, true), m);
			}
			PutEntities(io, c, sn.entities, base.snap ? std::span<const Entity>(base.snap->entities) : noEntities, *bl, m);
			break;
		}
		case OpType::Nop:
		case OpType::MapChange:
			break;
		}
	}
	io.S(kOpCtx[prev], 0);
	rec = HeaderRecord{ msg.ack, time, cmdseq, base.snap };
}

constexpr auto kEntityOffsets = [] {
	std::array<uint16_t, kNumEntityFields> a{};
	for (uint32_t i = 0; i < kNumEntityFields; i++) a[i] = kEntityFields[i].offset;
	return a;
}();

} // namespace

std::span<const uint16_t> EntityFieldOffsets() noexcept { return kEntityOffsets; }
std::span<const uint16_t> PlayerFieldOffsets() noexcept { return kPlayerAllOffsets; }

bool MessageEncoder::Encode(const Message &msg, const Base &base, const std::vector<Entity> &baselines, size_t maxBytes,
                            std::vector<uint8_t> &out, HeaderRecord &record) {
	const Model &m = GetModel();
	if (base.delta > 255 || (base.delta != 0) != (base.snap != nullptr)) return false;
	if (ops_.empty()) ops_.resize(4096);
	for (;;) {
		rans::Encoder e(ops_);
		Out io(e, m);
		PutMessage(io, msg, base, baselines, record, m);
		if (e.Overflow()) { ops_.resize(ops_.size() * 2); continue; }
		if (io.Bad()) return false;
		if (scratch_.size() < maxBytes) scratch_.resize(maxBytes);   // grown once: no clearing per message
		const size_t n = e.Finish(std::span<uint8_t>(scratch_.data(), maxBytes));
		out.assign(scratch_.begin(), scratch_.begin() + static_cast<std::ptrdiff_t>(n));
		return n != 0;
	}
}

bool DecodeMessage(std::span<const uint8_t> bytes, const BaseLookup &lookup, std::vector<Entity> &baselines, Message &msg,
                   uint32_t &delta, HeaderRecord &record) {
	const Model &m = GetModel();
	In io(bytes, m);
	if (!io.Ok()) return false;
	delta = io.S(kMDelta);
	const Snapshot *base = nullptr;
	const HeaderRecord *bh = nullptr;
	if (delta != 0 && !lookup.Find(delta, base, bh)) return false;
	PredCtx c;
	c.btime = bh ? bh->time : std::nullopt;
	c.olderSnap = bh ? bh->older : nullptr;
	msg.ack = (bh ? bh->ack : 0) + Unzz(io.Cls(kMAck));
	msg.ops.clear();
	int64_t lastcmd = bh ? bh->cmdSeq : 0, cmdseq = lastcmd;
	std::optional<int64_t> time;
	uint32_t prev = 0;
	std::optional<size_t> gsOp;            // index of a gamestate op of this message: its baselines apply after it
	const auto currentBaselines = [&]() -> const std::vector<Entity> & {
		return gsOp ? msg.ops[*gsOp].gameState.baselines : baselines;
	};
	const std::span<const Entity> noEntities;
	for (;;) {
		const uint32_t t = io.S(kOpCtx[prev]);
		prev = t;
		if (t == 0) break;
		if (msg.ops.size() == kMaxOps) return false;
		Op &op = msg.ops.emplace_back();
		op.type = static_cast<OpType>(t);
		switch (op.type) {
		case OpType::Command:
			op.seq = lastcmd + 1 + Unzz(io.Cls(kMCmdSeq)); lastcmd = op.seq; cmdseq = lastcmd;
			if (!io.Str(kStrCmd, op.text)) return false;
			break;
		case OpType::SetGame:
			if (!io.Str(kStrSetgame, op.text)) return false;
			break;
		case OpType::GameState: {
			GameState &gs = op.gameState;
			gs.relSeq = Unzz(io.Cls(kMRelSeq));
			const uint64_t n = io.Cls(kMNumCs);
			int64_t pi = -1;
			for (uint64_t k = 0; k < n; k++) {
				const int64_t idx = pi + 1 + static_cast<int64_t>(io.Cls(kMCsGap));
				if (idx > 0xFFFF) return false;
				pi = idx;
				ConfigString &cs = gs.configStrings.emplace_back();
				cs.index = static_cast<uint16_t>(idx);
				if (!io.Str(kStrCs, cs.text)) return false;
			}
			if (!GetEntities(io, c, noEntities, noEntities, gs.baselines, m)) return false;
			gs.clientNum = Unzz(io.Cls(kMClientNum));
			gs.checksum = S32(static_cast<uint32_t>(io.R(32)));
			gsOp = msg.ops.size() - 1;
			break;
		}
		case OpType::Download: {
			op.block = static_cast<uint32_t>(io.Cls(kMDlBlock));
			op.fileSize = op.block == 0 ? Unzz(io.Cls(kMDlSize)) : 0;
			const uint64_t n = io.Cls(kMDlLen);
			op.data.resize(static_cast<uint32_t>(n));     // n < 2^19 (20 classes)
			for (uint8_t &b : op.data) b = static_cast<uint8_t>(io.S(kDlByte));
			break;
		}
		case OpType::Snapshot: {
			op.time = (bh && bh->time ? *bh->time : 0) + Unzz(io.Cls(kHTime));
			time = op.time; c.time = op.time;
			op.flags = io.S(kHFlags);
			op.areas.resize(io.S(kHArea));
			for (uint8_t &a : op.areas) a = static_cast<uint8_t>(io.S(kHArea));
			Snapshot &sn = op.snap;
			const bool pil = io.S(kPsPilot) != 0;
			const PlayerState &basePs = base ? base->ps : ZeroPlayer();
			const bool okPs = pil ? GetPlayer(io, c, kPilotFields, kPlayerGapCtx1, basePs, sn.ps, OlderPlayer(c, false), m)
			                      : GetPlayer(io, c, kPlayerFields, kPlayerGapCtx0, basePs, sn.ps, OlderPlayer(c, false), m);
			if (!okPs) return false;
			sn.hasVehicle = VehicleNum(sn.ps) != 0;
			if (sn.hasVehicle) {
				const PlayerState &ov = base && base->hasVehicle ? base->vps : ZeroPlayer();
				if (!GetPlayer(io, c, kVehicleFields, kPlayerGapCtx2, ov, sn.vps, OlderPlayer(c, true), m)) return false;
			}
			if (!GetEntities(io, c, base ? std::span<const Entity>(base->entities) : noEntities, currentBaselines(), sn.entities, m))
				return false;
			break;
		}
		case OpType::Nop:
		case OpType::MapChange:
			break;
		}
	}
	if (gsOp) baselines = msg.ops[*gsOp].gameState.baselines;
	record = HeaderRecord{ msg.ack, time, cmdseq, base };
	return true;
}

} // namespace netcodec
