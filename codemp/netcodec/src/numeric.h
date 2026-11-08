// netcodec internals: the integer and float operations of the format. Differences of 32-bit values are taken in
// 64-bit integers (no wrap-around); the float tests use IEEE double arithmetic, which every target computes the same
// way given the build settings (no FMA contraction, SSE2: see CMakeLists.txt).
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

namespace netcodec::detail {

inline constexpr double kAngleStep = 360.0 / 65536;   // a 16-bit angle index step (exact in binary)

// zigzag: 0, -1, 1, -2, ... to 0, 1, 2, 3, ...
[[nodiscard]] constexpr uint64_t Zz(int64_t d) noexcept {
	return d >= 0 ? static_cast<uint64_t>(d) << 1U : (static_cast<uint64_t>(-(d + 1)) << 1U) + 1;
}
[[nodiscard]] constexpr int64_t Unzz(uint64_t x) noexcept {
	return (x & 1U) == 0 ? static_cast<int64_t>(x >> 1U) : -static_cast<int64_t>((x + 1) >> 1U);
}

// a modulo m for m a power of two, with the sign of m (0 <= result < m): a mask instead of the 64-bit division of %
// (an idiv on x64 when m is not a constant, a runtime call on 32-bit x86)
[[nodiscard]] constexpr int64_t ModPow2(int64_t a, int64_t m) noexcept {
	return static_cast<int64_t>(static_cast<uint64_t>(a) & static_cast<uint64_t>(m - 1));
}
// division rounding down
[[nodiscard]] constexpr int64_t FloorDiv(int64_t a, int64_t b) noexcept {
	const int64_t q = a / b;
	return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

[[nodiscard]] constexpr int64_t S32(uint32_t w) noexcept { return static_cast<int32_t>(w); }
[[nodiscard]] constexpr int64_t S32(int64_t v) noexcept { return static_cast<int32_t>(static_cast<uint32_t>(v)); }

// the float32 whose bit pattern is pattern, widened to double (exact)
[[nodiscard]] constexpr double F32(int64_t pattern) noexcept {
	return static_cast<double>(std::bit_cast<float>(static_cast<uint32_t>(pattern)));
}
// a double rounded to float32, as its bit pattern
[[nodiscard]] constexpr uint32_t Pack(double x) noexcept { return std::bit_cast<uint32_t>(static_cast<float>(x)); }
// floor of x, for |x| < 2^51 (not NaN): exact, without the library call std::floor is on SSE2 targets
[[nodiscard]] constexpr int64_t FloorI(double x) noexcept {
#if defined(__i386__) || defined(_M_IX86)
	// 32-bit x86 has no SSE2 conversion between double and int64_t (x87 with two control-word changes, or a library
	// call: half of the decoding time): x + 1.5 * 2^52 is x rounded to the nearest integer, held in the low bits
	constexpr double kRound = 6755399441055744.0;   // 1.5 * 2^52: the sum's last place is 1 for |x| < 2^51
	const double big = x + kRound;
	const int64_t n = std::bit_cast<int64_t>(big) - std::bit_cast<int64_t>(kRound);
	return big - kRound > x ? n - 1 : n;
#else
	const auto t = static_cast<int64_t>(x);   // rounds toward zero
	return static_cast<double>(t) > x ? t - 1 : t;
#endif
}
[[nodiscard]] constexpr int64_t Rnd(double x) noexcept { return FloorI(x + 0.5); }

// float kind: 0 zero, 1 integral in [-4096, 4096), 2 other (3, angle index, is decided by the caller). -0.0 is
// "other": the integral kind would decode it as +0.0.
[[nodiscard]] inline uint32_t FKind(int64_t pattern) noexcept {
	constexpr int64_t kNegativeZero = 0x80000000LL;
	if (pattern == 0) return 0;
	if (pattern == kNegativeZero) return 2;
	const double f = F32(pattern);
	return (f == f && f >= -4096 && f < 4096 && static_cast<double>(static_cast<int32_t>(f)) == f) ? 1 : 2;
}

// the float is exactly a 16-bit angle index times kAngleStep, |f| <= 720
[[nodiscard]] inline bool IsAngleIndex(int64_t pattern) noexcept {
	const double f = F32(pattern);
	if (f != f || std::fabs(f) > 720) return false;
	// a multiply instead of the division: for an angle index f / kAngleStep is within 2^-7 of the index, so both
	// round the same; otherwise the exactness test fails either way (checked on all 2^32 patterns)
	constexpr double kInvAngleStep = 65536 / 360.0;
	return Pack(static_cast<double>(Rnd(f * kInvAngleStep)) * kAngleStep) == static_cast<uint32_t>(pattern);
}

// angle index of a base value (0 when the base is not an angle)
[[nodiscard]] inline int64_t OldIndex(int64_t pattern) noexcept {
	const double f = F32(pattern);
	return (pattern != 0 && f == f && std::fabs(f) <= 720) ? Rnd(f / kAngleStep) : 0;
}

} // namespace netcodec::detail
