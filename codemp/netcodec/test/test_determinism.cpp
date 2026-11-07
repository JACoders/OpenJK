// Determinism test of the float steps of the format: the float kind, the angle-index test and the angle index of a
// value, for every 16-bit angle index from -720 to 720 degrees and its neighbouring patterns (±1, ±2), plus special
// patterns, and the decode of an angle index over the range of the second-order angle prediction. The results are
// hashed (FNV-1a 64) and compared with the value of the Python reference functions
// (docs_rewrite/netcode/tools/python/angle_hash.py): every compiler, architecture and build type must agree.
// The position prediction is covered by netcodec.roundtrip (hash of the coded bytes of the corpus).
// Usage: netcodec_test_determinism
#include "fnv.h"
#include "numeric.h"

#include <cinttypes>
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
	std::printf("angle indices not recognised: %zu; hash 0x%016" PRIX64 ", expected 0x%016" PRIX64 "\n", notAngle, f.h, kExpected);
	return notAngle == 0 && f.h == kExpected ? 0 : 1;
}
