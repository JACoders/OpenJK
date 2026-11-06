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

// modulo with the sign of the divisor, and division rounding down
[[nodiscard]] constexpr int64_t PyMod(int64_t a, int64_t m) noexcept {
	const int64_t r = a % m;
	return r < 0 ? r + m : r;
}
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
[[nodiscard]] inline int64_t Rnd(double x) noexcept { return static_cast<int64_t>(std::floor(x + 0.5)); }

// float kind: 0 zero, 1 integral in [-4096, 4096), 2 other (3, angle index, is decided by the caller)
[[nodiscard]] inline uint32_t FKind(int64_t pattern) noexcept {
	if (pattern == 0) return 0;
	const double f = F32(pattern);
	return (f == f && f >= -4096 && f < 4096 && std::trunc(f) == f) ? 1 : 2;
}

// the float is exactly a 16-bit angle index times kAngleStep, |f| <= 720
[[nodiscard]] inline bool IsAngleIndex(int64_t pattern) noexcept {
	const double f = F32(pattern);
	if (f != f || std::fabs(f) > 720) return false;
	return Pack(static_cast<double>(Rnd(f / kAngleStep)) * kAngleStep) == static_cast<uint32_t>(pattern);
}

// angle index of a base value (0 when the base is not an angle)
[[nodiscard]] inline int64_t OldIndex(int64_t pattern) noexcept {
	const double f = F32(pattern);
	return (pattern != 0 && f == f && std::fabs(f) <= 720) ? Rnd(f / kAngleStep) : 0;
}

} // namespace netcodec::detail
