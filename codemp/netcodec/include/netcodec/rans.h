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

#include <bit>
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
constexpr uint32_t kLow = 1U << 23U;           // lower bound of the normalisation interval
constexpr uint32_t kRawChunk = 15;             // raw bits are coded in chunks of at most this many bits
constexpr size_t kStateBytes = 4;

// Encoder view of one symbol: the reciprocal of its frequency, so that encoding needs no division (Alverson).
// 8 bytes: the bound and the shift follow from the frequency (half the memory of storing them).
struct EncSymbol {
	uint32_t rcpFreq = 0;
	uint16_t bias = 0;      // start, or start + kProbScale - 1 when freq is 1: below 2^16
	uint16_t cmplFreq = 0;  // kProbScale - freq

	[[nodiscard]] constexpr uint32_t Freq() const noexcept { return kProbScale - cmplFreq; }
	// exclusive bound of the state before renormalisation
	[[nodiscard]] constexpr uint32_t XMax() const noexcept { return ((kLow >> kProbBits) << 8U) * Freq(); }
	// shift of the multiply-high (31 + ceil(log2(freq)), and 32 when freq is 1), minus 32: applied to the product's high
	// word (a 64-bit shift by a variable is a runtime call in MSVC's 32-bit code)
	[[nodiscard]] constexpr uint32_t RcpShiftHigh() const noexcept {
		return static_cast<uint32_t>(31 - std::countl_zero((Freq() - 1) | 1U));   // bit width - 1
	}
};
static_assert(sizeof(EncSymbol) == 8, "encoder symbols are 8 bytes");

// start + freq <= kProbScale, freq >= 1
[[nodiscard]] constexpr EncSymbol MakeEncSymbol(uint32_t start, uint32_t freq) noexcept {
	EncSymbol s{};
	s.cmplFreq = static_cast<uint16_t>(kProbScale - freq);
	if (freq < 2) {             // freq 1: q = x - 1 (state is never 0), see rans_byte.h
		s.rcpFreq = ~0u;
		s.bias = static_cast<uint16_t>(start + kProbScale - 1);
	} else {
		uint32_t shift = 0;
		while (freq > (1u << shift)) shift++;
		s.rcpFreq = static_cast<uint32_t>(((uint64_t{ 1 } << (shift + 31)) + freq - 1) / freq);
		s.bias = static_cast<uint16_t>(start);
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
		ops_[n_++] = Op{ .sym = &s, .value = 0, .bits = 0 };
	}

	// value < 2^bits, bits 0 to 32; chunks of kRawChunk bits, lowest first
	void PutRaw(uint32_t value, uint32_t bits) noexcept {
		assert(bits <= 32 && (bits == 32 || value >> bits == 0));
		while (bits > 0) {
			const uint32_t b = bits < kRawChunk ? bits : kRawChunk;
			if (n_ == ops_.size()) { overflow_ = true; return; }
			ops_[n_++] = Op{ .sym = nullptr, .value = value & ((1u << b) - 1), .bits = b };
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
				const uint32_t xMax = s.XMax();
				while (x >= xMax) {
					if (pos == kStateBytes) return 0;
					out[--pos] = static_cast<uint8_t>(x); x >>= 8U;
				}
				// 64-bit targets: one shift of the product (32 + the shift is off the state's chain); 32-bit: a 32-bit
				// shift of its high word, since a 64-bit shift by a variable is a runtime call in MSVC's 32-bit code
				const uint64_t p = uint64_t{ x } * s.rcpFreq;
				const uint32_t q = sizeof(void *) == 8 ? static_cast<uint32_t>(p >> (32U + s.RcpShiftHigh()))
				                                       : static_cast<uint32_t>(p >> 32U) >> s.RcpShiftHigh();
				x = x + s.bias + (q * s.cmplFreq);
			} else {
				const uint32_t sh = kProbBits - op.bits;           // freq = 2^sh, start = value << sh
				const uint32_t xMax = 1u << (31 - op.bits);       // ((kLow >> kProbBits) << 8) << sh
				while (x >= xMax) {
					if (pos == kStateBytes) return 0;
					out[--pos] = static_cast<uint8_t>(x); x >>= 8U;
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
		x_ = (freq * (x_ >> kProbBits)) + (x_ & kProbMask) - start;
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
		while (x_ < kLow && pos_ < data_.size()) x_ = (x_ << 8U) | data_[pos_++];
	}

	uint32_t x_ = 0;
	std::span<const uint8_t> data_;
	size_t pos_ = 0;
};

constexpr uint32_t kCoarseBits = 6;           // decoder search table: 2^6 buckets of the slot (measured, bench_rans)
constexpr uint32_t kCoarseSize = 1u << kCoarseBits;

// Decoder search entry of one bucket of the slot: the symbol that holds the bucket's first slot, with its slot range
// [start, end), so that a slot inside it is decoded without reading cum (most slots: a bucket is usually inside one
// symbol).
struct CoarseEntry {
	uint16_t sym = 0;
	uint16_t start = 0;   // cum[sym]
	uint16_t end = 0;     // cum[sym + 1], at most kProbScale
	uint16_t unused = 0;  // 8-byte entries: the index is a shift
};

// One context: K symbols, cumulative frequencies cum[0..K] (cum[0] = 0, cum[K] = kProbScale), encoder symbols,
// and the search entry of each bucket of the slot's top kCoarseBits bits.
// Non-owning views: the storage belongs to the caller.
struct Table {
	std::span<const uint16_t> cum;                  // K + 1 entries
	std::span<const EncSymbol> enc;                 // K entries
	std::span<const CoarseEntry, kCoarseSize> coarse;

	[[nodiscard]] uint32_t K() const noexcept { return ToU32(enc.size()); }
};

// cum: K + 1 entries ending with kProbScale
inline void BuildCoarse(std::span<const uint16_t> cum, std::span<CoarseEntry, kCoarseSize> coarse) noexcept {
	uint32_t s = 0;
	for (uint32_t b = 0; b < kCoarseSize; b++) {
		const uint32_t slot = b << (kProbBits - kCoarseBits);
		while (cum[s + 1] <= slot) s++;
		coarse[b] = CoarseEntry{ .sym = static_cast<uint16_t>(s), .start = cum[s], .end = cum[s + 1], .unused = 0 };
	}
}

inline void Put(Encoder &e, const Table &t, uint32_t sym) noexcept { e.Put(t.enc[sym]); }

// symbol whose slot range holds slot: cum[s] <= slot < cum[s + 1]; forward search from the bucket's first symbol
[[nodiscard]] inline uint32_t FindSymbol(const Table &t, uint32_t slot) noexcept {
	uint32_t s = t.coarse[slot >> (kProbBits - kCoarseBits)].sym;
	while (t.cum[s + 1] <= slot) s++;
	return s;
}

[[nodiscard]] inline uint32_t Get(Decoder &d, const Table &t) noexcept {
	const uint32_t slot = d.Slot();
	const CoarseEntry &e = t.coarse[slot >> (kProbBits - kCoarseBits)];
	uint32_t s = e.sym, start = e.start, end = e.end;
	if (slot >= end) {   // past the bucket's first symbol: search on
		s++;
		while (t.cum[s + 1] <= slot) s++;
		start = t.cum[s];
		end = t.cum[s + 1];
	}
	d.Advance(start, end - start);
	return s;
}

} // namespace rans
} // namespace netcodec
