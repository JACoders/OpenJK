// netcodec internals: coding of one changed field, with its
// predictions. Only changed fields are coded; the value is coded against the field's value in the base state.
//
// Integers: absolute (small alphabets directly, else by class) or delta modulo the width. Floats: a kind (zero,
// integral, 16-bit angle index, other) then the value in the kind's coding. Booleans: one "flip" symbol when old and
// new are 0 / 1, else the value as a 32-bit integer. Predictions: a time field against its base value advanced by
// the time between the two snapshots (the snapshot time when the base is not the same object's previous state), a
// position from the old position and the mean of the old and new velocities (integer arithmetic in 1/65536 units),
// an angle index to second order from the base and the base's own base.
#pragma once

#include "coder.h"
#include "numeric.h"
#include "schema.h"

#include <optional>

// Forces the inlining of a small function of the field loops. GCC 12 on Linux takes PredictAngle out of the field
// loops depending on unrelated changes; with aligned code, decoding was then about 4 % slower (encoding unchanged).
// The decision must not depend on the inliner's size estimates.
#if defined(_MSC_VER) && !defined(__clang__)
#define NETCODEC_ALWAYS_INLINE __forceinline   // NOLINT(cppcoreguidelines-macro-usage): no portable attribute
#else
#define NETCODEC_ALWAYS_INLINE [[gnu::always_inline]] inline   // NOLINT(cppcoreguidelines-macro-usage): same
#endif

namespace netcodec::detail {

// What the predictions read besides the field's own old value.
struct PredCtx {
	std::optional<int64_t> time;           // serverTime of the message's snapshot, once coded
	std::optional<int64_t> btime;          // serverTime of the base
	const Snapshot *olderSnap = nullptr;   // the base's own base
	ImageView cur;                         // state being coded (when decoding, the part decoded so far)
	ImageView old;                         // its base state
	const uint8_t *older = nullptr;        // the base state's own base state (same structure), or none
	bool pred = false;                     // the base state is the same object's previous state
};

constexpr int64_t kTrInterpolate = 1, kTrLinearStop = 3;
constexpr int64_t kTwo31 = 2147483648LL, kTwo32 = 4294967296LL;

// low 32 bits of a value, as a non-negative value (a 32-bit pattern)
[[nodiscard]] constexpr int64_t Low32(int64_t v) noexcept { return static_cast<uint32_t>(v); }

[[nodiscard]] inline std::optional<uint32_t> PredictPosition(const PredCtx &c, const FieldDesc &f, int trTypeBits) noexcept {
	if (!c.pred) return std::nullopt;
	int64_t dt = 0;
	if (f.relKind == 0) {
		const int64_t tn = ValueBits(Load(c.cur, f.relType), trTypeBits);
		const int64_t to = ValueBits(Load(c.old, f.relType), trTypeBits);
		if (tn == to && tn == kTrLinearStop) dt = S32(Load(c.cur, f.relTime)) - S32(Load(c.old, f.relTime));
		else if (tn == to && tn == kTrInterpolate && c.btime && c.time) dt = *c.time - *c.btime;
		else return std::nullopt;
	} else {
		dt = S32(Load(c.cur, f.relTime)) - S32(Load(c.old, f.relTime));
	}
	const double o = F32(Load(c.old, f.offset)), vo = F32(Load(c.old, f.relVel)), vn = F32(Load(c.cur, f.relVel));
	const auto finite = [](double x) { return x == x && std::fabs(x) < 16777216.0; };
	if (!finite(o) || !finite(vo) || !finite(vn) || (dt < 0 ? -dt : dt) >= 65536) return std::nullopt;
	const auto units = [](double x) { return FloorI((x * 65536.0) + 0.5); };
	const int64_t p = units(o) + FloorDiv((units(vo) + units(vn)) * dt, 2000);
	return Pack(static_cast<double>(p) / 65536.0);
}

// reference of a time field (time prediction mode; time: the snapshot time): when the base state is the same object's previous
// state, its value advanced by the time between the two snapshots (a client's command time lags the server time by a
// steady amount); otherwise the snapshot time. A 32-bit pattern.
[[nodiscard]] inline int64_t TimeReference(const PredCtx &c, const FieldDesc &f, int64_t time) noexcept {
	if (c.pred && c.btime) return Low32(Load(c.old, f.offset) + (time - *c.btime));
	return Low32(time);
}

[[nodiscard]] NETCODEC_ALWAYS_INLINE std::optional<int64_t> PredictAngle(const PredCtx &c, const FieldDesc &f, int64_t ov) noexcept {
	if (!c.pred || c.older == nullptr) return std::nullopt;
	uint32_t o2 = 0;
	std::memcpy(&o2, c.older + f.offset, 4);
	if (!(IsAngleIndex(ov) && IsAngleIndex(o2))) return std::nullopt;
	return (2 * OldIndex(ov)) - OldIndex(o2);
}

template <class O> void PutInt(O &io, ctx::Row row, int bits, int64_t nv, int64_t ov, bool absolute) noexcept {
	const auto ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const auto m = static_cast<int64_t>(uint64_t{ 1 } << ab);
	if (absolute) {
		if (ab <= 8) io.Symbol(row[fctx::kA], static_cast<uint64_t>(nv) & static_cast<uint64_t>(m - 1));
		else io.Class(row[fctx::kA], bits < 0 ? Zz(nv) : static_cast<uint64_t>(nv));
	} else {
		io.Class(row[fctx::kD], Zz(PyMod(nv - ov + (m / 2), m) - (m / 2)));
	}
}

[[nodiscard]] inline int64_t GetInt(In &io, ctx::Row row, int bits, int64_t ov, bool absolute) noexcept {
	const auto ab = static_cast<uint32_t>(bits < 0 ? -bits : bits);
	const auto m = static_cast<int64_t>(uint64_t{ 1 } << ab);
	const bool sgn = bits < 0;
	if (absolute) {
		if (ab <= 8) {
			const int64_t v = io.Symbol(row[fctx::kA]);
			return (sgn && v >= m / 2) ? v - m : v;
		}
		const uint64_t x = io.Class(row[fctx::kA]);
		return sgn ? Unzz(x) : static_cast<int64_t>(x);
	}
	const int64_t v = ov + Unzz(io.Class(row[fctx::kD]));
	return sgn ? PyMod(v + (m / 2), m) - (m / 2) : PyMod(v, m);
}

[[nodiscard]] inline uint32_t FloatKind(int64_t nv) noexcept {
	const uint32_t k = FKind(nv);
	return k == 2 && IsAngleIndex(nv) ? 3 : k;
}

template <class O> void PutFloat(O &io, ctx::Row row, int64_t nv, int64_t ov, bool absolute) noexcept {
	const uint32_t k = FloatKind(nv);
	io.Symbol(row[fctx::kK], k);
	const uint32_t ok = FKind(ov);
	if (k == 1) {
		const auto t = static_cast<int64_t>(F32(nv));
		const int64_t ot = ok == 1 ? static_cast<int64_t>(F32(ov)) : 0;
		if (absolute) io.Class(row[fctx::kIA], Zz(t));
		else io.Class(row[fctx::kID], Zz(t - ot));
	} else if (k == 3) {
		io.Class(row[fctx::kAD], Zz(Rnd(F32(nv) / kAngleStep) - OldIndex(ov)));
	} else if (k == 2) {
		if (absolute) io.Raw(static_cast<uint32_t>(nv), 32);
		else io.Class(row[fctx::kFD], Zz(S32(nv) - (ok != 0 ? S32(ov) : 0)));
	}
}

[[nodiscard]] inline int64_t GetFloatOfKind(In &io, ctx::Row row, int64_t ov, bool absolute, uint32_t k) noexcept {
	const uint32_t ok = FKind(ov);
	if (k == 0) return 0;
	if (k == 1) {
		const int64_t ot = ok == 1 ? static_cast<int64_t>(F32(ov)) : 0;
		const int64_t t = absolute ? Unzz(io.Class(row[fctx::kIA])) : ot + Unzz(io.Class(row[fctx::kID]));
		return Pack(static_cast<double>(t));
	}
	if (k == 3) return Pack(static_cast<double>(OldIndex(ov) + Unzz(io.Class(row[fctx::kAD]))) * kAngleStep);
	if (absolute) return static_cast<int64_t>(io.Raw(32));
	return Low32((ok != 0 ? S32(ov) : 0) + Unzz(io.Class(row[fctx::kFD])));
}

template <class O> void PutField(O &io, const PredCtx &c, const FieldDesc &f, ctx::Row row, int64_t nv, int64_t ov,
                     int trTypeBits) noexcept {
	if (f.ctype == CType::kBool) {
		const bool flip = (nv == 0 || nv == 1) && (ov == 0 || ov == 1);
		io.Symbol(row[fctx::kB], flip ? 0 : 1);
		if (flip) return;
	}
	if (f.pmode == PredMode::kTime && f.bits == 32 && c.time) {
		io.Class(row[fctx::kT], Zz(PyMod(nv - TimeReference(c, f, *c.time) + kTwo31, kTwo32) - kTwo31));
		return;
	}
	if (f.pmode != PredMode::kNone && f.bits == 0) {
		const uint32_t k = FloatKind(nv);
		if (f.pmode == PredMode::kPos && k == 2) {
			if (const auto p = PredictPosition(c, f, trTypeBits)) {
				io.Symbol(row[fctx::kK], 2);
				io.Class(row[fctx::kFP], Zz(S32(nv) - S32(*p)));
				return;
			}
		}
		if (f.pmode == PredMode::kAngle && k == 3) {
			if (const auto p = PredictAngle(c, f, ov)) {
				io.Symbol(row[fctx::kK], 3);
				io.Class(row[fctx::kA2], Zz(Rnd(F32(nv) / kAngleStep) - *p));
				return;
			}
		}
	}
	if (f.bits == 0) PutFloat(io, row, nv, ov, f.absolute);
	else PutInt(io, row, f.bits, nv, ov, f.absolute);
}

[[nodiscard]] inline int64_t GetField(In &io, const PredCtx &c, const FieldDesc &f, ctx::Row row, int64_t ov,
                                      int trTypeBits) noexcept {
	if (f.ctype == CType::kBool && io.Symbol(row[fctx::kB]) == 0) return ov == 0 ? 1 : 0;   // a flip
	if (f.pmode == PredMode::kTime && f.bits == 32 && c.time)
		return PyMod(TimeReference(c, f, *c.time) + Unzz(io.Class(row[fctx::kT])), kTwo32);
	if ((f.pmode == PredMode::kPos || f.pmode == PredMode::kAngle) && f.bits == 0) {
		const uint32_t k = io.Symbol(row[fctx::kK]);
		if (k == 2 && f.pmode == PredMode::kPos) {
			if (const auto p = PredictPosition(c, f, trTypeBits)) return Low32(S32(*p) + Unzz(io.Class(row[fctx::kFP])));
		}
		if (k == 3 && f.pmode == PredMode::kAngle) {
			if (const auto p = PredictAngle(c, f, ov))
				return Pack(static_cast<double>(*p + Unzz(io.Class(row[fctx::kA2]))) * kAngleStep);
		}
		return GetFloatOfKind(io, row, ov, f.absolute, k);
	}
	if (f.bits == 0) return GetFloatOfKind(io, row, ov, f.absolute, io.Symbol(row[fctx::kK]));
	return GetInt(io, row, f.bits, ov, f.absolute);
}

} // namespace netcodec::detail
