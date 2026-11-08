// Determinism test of the float steps of the format: the float kind, the angle-index test and the angle index of a
// value, for every 16-bit angle index from -720 to 720 degrees and its neighbouring patterns (±1, ±2), plus special
// patterns, and the decode of an angle index over the range of the second-order angle prediction. The results are
// hashed (FNV-1a 64) and compared with the value of the Python reference functions
// (docs_rewrite/netcode/tools/python/angle_hash.py): every compiler, architecture and build type must agree.
// The position prediction is covered by netcodec.roundtrip (hash of the coded bytes of the corpus). FloorI is also
// compared with std::floor directly.
// Usage: netcodec_test_determinism
#include "fnv.h"
#include "numeric.h"

#include <cinttypes>
#include <cmath>
#include <cstdio>

using namespace netcodec::detail;
using netcodec::test::Fnv;

namespace {

constexpr uint64_t kExpected = 0xCC2C10A05B0B5854ULL;   // angle_hash.py

void Pattern(Fnv &f, uint32_t p) noexcept {
	f.Feed(p, 4);
	f.Feed(FKind(p), 4);
	f.Feed(IsAngleIndex(p) ? 1 : 0, 4);
	f.Feed(static_cast<uint64_t>(OldIndex(p)), 8);
}

uint64_t SplitMix(uint64_t &s) noexcept {
	s += 0x9E3779B97F4A7C15ULL;
	uint64_t z = s;
	z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31U);
}

// FloorI (on 32-bit x86 it rounds through 1.5 * 2^52) against std::floor over its domain, |x| < 2^51: edge values,
// random magnitudes from 2^-20 to 2^51, and random integers and halves
size_t FloorMismatches() noexcept {
	size_t bad = 0;
	const auto check = [&bad](double x) {
		if (FloorI(x) != static_cast<int64_t>(std::floor(x))) bad++;
	};
	for (const double x : { 0.0, -0.0, 0.5, -0.5, 1.0, -1.0, 1.5, -1.5, 2.5, -2.5, 0x1p51 - 1, -0x1p51 + 1, 0x1p51 - 0.5,
	                        -0x1p51 + 0.5 }) {
		check(x);
		check(std::nextafter(x, 0x1p60));
		check(std::nextafter(x, -0x1p60));
	}
	uint64_t s = 1;
	for (int i = 0; i < 2000000; i++) {
		const uint64_t a = SplitMix(s), b = SplitMix(s);
		const double m = std::ldexp(static_cast<double>(a >> 11U), -53) * ((b & 1U) != 0 ? -1.0 : 1.0);   // |m| < 1
		check(std::ldexp(m, static_cast<int>((b >> 1U) % 72U) - 20));
		check(static_cast<double>(static_cast<int64_t>(b >> 12U) - (int64_t{ 1 } << 51U)) * 0.5);
	}
	return bad;
}

// ModPow2 (a mask) against the modulo with the sign of the divisor, for every width 0 to 32 of the fields
size_t ModMismatches() noexcept {
	size_t bad = 0;
	uint64_t s = 2;
	for (int i = 0; i < 100000; i++) {
		const uint64_t a = SplitMix(s);
		for (uint32_t k = 0; k <= 32; k++) {
			const int64_t m = int64_t{ 1 } << k;
			for (const int64_t v : { static_cast<int64_t>(a), static_cast<int64_t>(a) >> 31U, static_cast<int64_t>(a) >> 50U }) {
				const int64_t r = v % m;
				if (ModPow2(v, m) != (r < 0 ? r + m : r)) bad++;
			}
		}
	}
	return bad;
}

} // namespace

int main() {
	Fnv f;
	for (const uint32_t p : { 0x00000000U, 0x80000000U, 0x7F800000U, 0xFF800000U, 0x7FC00000U, 0xFFC00001U, 0x00000001U,
	                          0x80000001U, 0x7F7FFFFFU })
		Pattern(f, p);
	size_t notAngle = 0;
	for (int64_t i = -131072; i <= 131072; i++) {
		const uint32_t base = Pack(static_cast<double>(i) * kAngleStep);
		if (!IsAngleIndex(base) || OldIndex(base) != i) notAngle++;
		for (const int d : { 0, 1, -1, 2, -2 }) Pattern(f, base + static_cast<uint32_t>(d));
	}
	for (int64_t k = -400000; k <= 400000; k++) f.Feed(Pack(static_cast<double>(k) * kAngleStep), 4);
	const size_t floorBad = FloorMismatches(), modBad = ModMismatches();
	std::printf("angle indices not recognised: %zu; FloorI against std::floor: %zu differences; ModPow2 against %%: %zu "
	            "differences; hash 0x%016" PRIX64 ", expected 0x%016" PRIX64 "\n", notAngle, floorBad, modBad, f.h, kExpected);
	return notAngle == 0 && floorBad == 0 && modBad == 0 && f.h == kExpected ? 0 : 1;
}
