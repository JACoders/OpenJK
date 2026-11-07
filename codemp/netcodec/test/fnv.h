// FNV-1a 64 hash of the determinism checks: values are fed as little-endian bytes.
#pragma once

#include <cstdint>
#include <span>

namespace netcodec::test {

struct Fnv {
	uint64_t h = 0xCBF29CE484222325ULL;
	void Feed(uint64_t v, int bytes) noexcept {
		for (int i = 0; i < bytes; i++) {
			h = (h ^ (v & 0xFFU)) * 0x100000001B3ULL;
			v >>= 8U;
		}
	}
	void Feed(std::span<const uint8_t> bytes) noexcept {
		for (const uint8_t b : bytes) h = (h ^ b) * 0x100000001B3ULL;
	}
};

} // namespace netcodec::test
