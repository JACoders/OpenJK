// netcodec: static rANS coder (32-bit state, byte renormalisation, 15-bit probabilities).
//
// Design of ryg_rans rans_byte.h (Fabian Giesen, public domain), with three differences that are part of the wire
// format (docs_rewrite/netcode/progress/step-06-cpp-codec.md):
//  - the encoder starts from state 1, not from the lower bound L: the first coded bits fill the final state, which is
//    sent anyway (about 1.8 bytes per message);
//  - the decoder stops reading at the end of the input, so its state may end below L;
//  - raw bits are coded as uniform symbols of at most 15 bits, with shifts instead of a division.
// The final encoder state is written little-endian in the first 4 bytes of the message.
// Unsigned arithmetic throughout: the coder is bit manipulation with defined wrap-around (C++ Core Guidelines ES.101).
// Buffers are passed as std::span (Core Guidelines I.13); the standard library checks the indices in debug builds.
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

namespace netcodec {

// size or count to uint32_t: a cast on 64-bit targets, none on 32-bit ones (where GCC calls the cast useless)
template <class T> [[nodiscard]] constexpr uint32_t ToU32(T v) noexcept {
	static_assert(std::is_unsigned_v<T>, "ToU32 takes unsigned values");
	assert(v <= std::numeric_limits<uint32_t>::max());
	if constexpr (std::is_same_v<T, uint32_t>) return v;
	else return static_cast<uint32_t>(v);
}

namespace rans {

constexpr uint32_t kProbBits = 15;
constexpr uint32_t kProbScale = 1u << kProbBits;
constexpr uint32_t kProbMask = kProbScale - 1;
constexpr uint32_t kLow = 1u << 23;            // lower bound of the normalisation interval
constexpr uint32_t kRawChunk = 15;             // raw bits are coded in chunks of at most this many bits
constexpr size_t kStateBytes = 4;

// Encoder view of one symbol: the reciprocal of its frequency, so that encoding needs no division (Alverson).
struct EncSymbol {
	uint32_t xMax = 0;      // exclusive bound of the state before renormalisation
	uint32_t rcpFreq = 0;
	uint32_t bias = 0;
	uint16_t cmplFreq = 0;  // kProbScale - freq
	uint16_t rcpShift = 0;  // includes the 32 of the multiply-high
};

// start + freq <= kProbScale, freq >= 1
[[nodiscard]] constexpr EncSymbol MakeEncSymbol(uint32_t start, uint32_t freq) noexcept {
	EncSymbol s{};
	s.xMax = ((kLow >> kProbBits) << 8) * freq;
	s.cmplFreq = static_cast<uint16_t>(kProbScale - freq);
	if (freq < 2) {             // freq 1: q = x - 1 (state is never 0), see rans_byte.h
		s.rcpFreq = ~0u;
		s.rcpShift = 32;
		s.bias = start + kProbScale - 1;
	} else {
		uint32_t shift = 0;
		while (freq > (1u << shift)) shift++;
		s.rcpFreq = static_cast<uint32_t>(((uint64_t{ 1 } << (shift + 31)) + freq - 1) / freq);
		s.rcpShift = static_cast<uint16_t>(shift - 1 + 32);
		s.bias = start;
	}
	return s;
}

// Encoder. rANS codes in reverse: Put* only record the symbols, Finish codes them last to first.
// The caller owns the record buffer (no allocation here).
class Encoder {
public:
	struct Op {
		const EncSymbol *sym = nullptr;   // non-owning; nullptr: raw chunk
		uint32_t value = 0;               // raw: the chunk value
		uint32_t bits = 0;                // raw: chunk width (1 to 15)
	};

	explicit Encoder(std::span<Op> ops) noexcept : ops_(ops) {}

	void Put(const EncSymbol &s) noexcept {
		if (n_ == ops_.size()) { overflow_ = true; return; }
		ops_[n_++] = Op{ &s, 0, 0 };
	}

	// value < 2^bits, bits 0 to 32; chunks of kRawChunk bits, lowest first
	void PutRaw(uint32_t value, uint32_t bits) noexcept {
		assert(bits <= 32 && (bits == 32 || value >> bits == 0));
		while (bits > 0) {
			const uint32_t b = bits < kRawChunk ? bits : kRawChunk;
			if (n_ == ops_.size()) { overflow_ = true; return; }
			ops_[n_++] = Op{ nullptr, value & ((1u << b) - 1), b };
			value >>= b;
			bits -= b;
		}
	}

	// Writes the message at the start of out; returns its size, or 0 if out or the record buffer is too small.
	[[nodiscard]] size_t Finish(std::span<uint8_t> out) const noexcept {
		if (overflow_ || out.size() < kStateBytes) return 0;
		size_t pos = out.size();           // bytes are produced backwards from the end, down to kStateBytes
		uint32_t x = 1;
		for (size_t i = n_; i-- > 0;) {
			const Op &op = ops_[i];
			if (op.sym != nullptr) {
				const EncSymbol &s = *op.sym;
				while (x >= s.xMax) {
					if (pos == kStateBytes) return 0;
					out[--pos] = static_cast<uint8_t>(x); x >>= 8;
				}
				const uint32_t q = static_cast<uint32_t>((uint64_t{ x } * s.rcpFreq) >> s.rcpShift);
				x = x + s.bias + q * s.cmplFreq;
			} else {
				const uint32_t sh = kProbBits - op.bits;           // freq = 2^sh, start = value << sh
				const uint32_t xMax = 1u << (31 - op.bits);       // ((kLow >> kProbBits) << 8) << sh
				while (x >= xMax) {
					if (pos == kStateBytes) return 0;
					out[--pos] = static_cast<uint8_t>(x); x >>= 8;
				}
				x = ((x >> sh) << kProbBits) + (x & ((1u << sh) - 1)) + (op.value << sh);
			}
		}
		const size_t body = out.size() - pos;
		for (size_t i = 0; i < kStateBytes; i++) out[i] = static_cast<uint8_t>(x >> (8 * i));
		if (pos != kStateBytes)
			for (size_t i = 0; i < body; i++) out[kStateBytes + i] = out[pos + i];   // forward copy, pos > kStateBytes
		return kStateBytes + body;
	}

	[[nodiscard]] size_t Count() const noexcept { return n_; }
	[[nodiscard]] bool Overflow() const noexcept { return overflow_; }   // the record buffer was too small

private:
	std::span<Op> ops_;
	size_t n_ = 0;
	bool overflow_ = false;
};

// Decoder. Reads past the end are never made: the state is simply not refilled. Whether the message was complete
// is for the caller to check (the message layer knows where it ends).
class Decoder {
public:
	// false if the input is shorter than the 4-byte state
	[[nodiscard]] bool Init(std::span<const uint8_t> data) noexcept {
		if (data.size() < kStateBytes) return false;
		x_ = 0;
		for (size_t i = 0; i < kStateBytes; i++) x_ |= static_cast<uint32_t>(data[i]) << (8 * i);
		data_ = data; pos_ = kStateBytes;
		return true;
	}

	[[nodiscard]] uint32_t Slot() const noexcept { return x_ & kProbMask; }

	void Advance(uint32_t start, uint32_t freq) noexcept {
		x_ = freq * (x_ >> kProbBits) + (x_ & kProbMask) - start;
		Refill();
	}

	// bits 0 to 32
	[[nodiscard]] uint32_t GetRaw(uint32_t bits) noexcept {
		assert(bits <= 32);
		uint32_t value = 0, shift = 0;
		while (bits > 0) {
			const uint32_t b = bits < kRawChunk ? bits : kRawChunk;
			const uint32_t sh = kProbBits - b;
			const uint32_t slot = x_ & kProbMask;
			const uint32_t v = slot >> sh;
			x_ = ((x_ >> kProbBits) << sh) + (slot & ((1u << sh) - 1));
			Refill();
			value |= v << shift;
			shift += b; bits -= b;
		}
		return value;
	}

	[[nodiscard]] size_t Remaining() const noexcept { return data_.size() - pos_; }

private:
	void Refill() noexcept {
		while (x_ < kLow && pos_ < data_.size()) x_ = (x_ << 8) | data_[pos_++];
	}

	uint32_t x_ = 0;
	std::span<const uint8_t> data_;
	size_t pos_ = 0;
};

constexpr uint32_t kCoarseBits = 6;           // decoder search table: 2^6 buckets of the slot (measured, bench_rans)
constexpr uint32_t kCoarseSize = 1u << kCoarseBits;

// One context: K symbols, cumulative frequencies cum[0..K] (cum[0] = 0, cum[K] = kProbScale), encoder symbols,
// and for each bucket of the slot's top kCoarseBits bits the symbol that holds the bucket's first slot.
// Non-owning views: the storage belongs to the caller.
struct Table {
	std::span<const uint16_t> cum;                  // K + 1 entries
	std::span<const EncSymbol> enc;                 // K entries
	std::span<const uint16_t, kCoarseSize> coarse;

	[[nodiscard]] uint32_t K() const noexcept { return ToU32(enc.size()); }
};

// cum: K + 1 entries ending with kProbScale
inline void BuildCoarse(std::span<const uint16_t> cum, std::span<uint16_t, kCoarseSize> coarse) noexcept {
	uint32_t s = 0;
	for (uint32_t b = 0; b < kCoarseSize; b++) {
		const uint32_t slot = b << (kProbBits - kCoarseBits);
		while (cum[s + 1] <= slot) s++;
		coarse[b] = static_cast<uint16_t>(s);
	}
}

inline void Put(Encoder &e, const Table &t, uint32_t sym) noexcept { e.Put(t.enc[sym]); }

// symbol whose slot range holds slot: cum[s] <= slot < cum[s + 1]; forward search from the bucket's first symbol
[[nodiscard]] inline uint32_t FindSymbol(const Table &t, uint32_t slot) noexcept {
	uint32_t s = t.coarse[slot >> (kProbBits - kCoarseBits)];
	while (t.cum[s + 1] <= slot) s++;
	return s;
}

[[nodiscard]] inline uint32_t Get(Decoder &d, const Table &t) noexcept {
	const uint32_t s = FindSymbol(t, d.Slot());
	d.Advance(t.cum[s], static_cast<uint32_t>(t.cum[s + 1]) - t.cum[s]);
	return s;
}

} // namespace rans
} // namespace netcodec
