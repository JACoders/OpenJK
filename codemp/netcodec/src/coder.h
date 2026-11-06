// netcodec internals: the symbol layer on top of rANS, as the format uses it. A symbol is coded with the table of its
// context; raw bits are coded uniformly; a "class" codes a value by its bit length in a context, then the bits
// below the leading one raw. The writers share the format's classes and strings (SymbolWriter): Out codes, CountOut
// counts the symbols per context for the trainer.
#pragma once

#include "contexts.h"
#include "model.h"

#include <bit>
#include <string>
#include <string_view>

namespace netcodec::detail {

// classes and strings over the writer's Symbol and Raw
template <class W> class SymbolWriter {
public:
	void Class(uint32_t ctx, uint64_t x) noexcept {
		const auto c = static_cast<uint32_t>(std::bit_width(x));
		Self().Symbol(ctx, c);
		if (c > 1) Self().Raw(x - (uint64_t{ 1 } << (c - 1)), c - 1);
	}

	// bytes then a 0 terminator (a 0 byte inside cannot be coded)
	void String(uint32_t ctx, std::string_view text) noexcept {
		for (const char ch : text) {
			const auto b = static_cast<uint8_t>(ch);
			if (b == 0) bad_ = true;
			Self().Symbol(ctx, b);
		}
		Self().Symbol(ctx, 0);
	}

	[[nodiscard]] bool Bad() const noexcept { return bad_; }

protected:
	bool bad_ = false;   // a value outside the model (a bug): the message is refused

private:
	friend W;
	SymbolWriter() = default;
	W &Self() noexcept { return static_cast<W &>(*this); }
};

class Out : public SymbolWriter<Out> {
public:
	Out(rans::Encoder &e, const Model &m) noexcept : e_(&e), m_(&m) {}

	void Symbol(uint32_t ctx, uint64_t sym) noexcept {
		const rans::Table &t = m_->Table(ctx);
		if (sym >= t.K()) { bad_ = true; return; }    // outside the model: a bug, the message is refused
		e_->Put(t.enc[static_cast<uint32_t>(sym)]);
	}

	// raw bits, in 30-bit pieces (a multiple of the coder's 15-bit chunks: the chunking of the format)
	void Raw(uint64_t v, uint32_t n) noexcept {
		while (n > 30) {
			e_->PutRaw(static_cast<uint32_t>(v & 0x3FFFFFFFU), 30);
			v >>= 30U;
			n -= 30;
		}
		e_->PutRaw(static_cast<uint32_t>(v), n);
	}

private:
	rans::Encoder *e_;
	const Model *m_;
};

// counts the symbols per context (symbols[ctx] sized by the context's alphabet), and the raw bits per context of the
// symbol before them (every message starts with a symbol)
class CountOut : public SymbolWriter<CountOut> {
public:
	explicit CountOut(SymbolCounts &counts) noexcept : counts_(&counts) {}

	void Symbol(uint32_t ctx, uint64_t sym) noexcept {
		std::vector<uint64_t> &c = counts_->symbols[ctx];
		if (sym >= c.size()) { bad_ = true; return; }
		c[static_cast<uint32_t>(sym)]++;   // below the alphabet size
		last_ = ctx;
	}

	void Raw(uint64_t /*v*/, uint32_t n) noexcept { counts_->rawBits[last_] += n; }

private:
	SymbolCounts *counts_;
	uint32_t last_ = 0;
};

class In {
public:
	static constexpr size_t kMaxString = size_t{ 1 } << 16U;   // against malformed input

	In(std::span<const uint8_t> bytes, const Model &m) noexcept : m_(&m), ok_(d_.Init(bytes)) {}

	[[nodiscard]] uint32_t Symbol(uint32_t ctx) noexcept { return rans::Get(d_, m_->Table(ctx)); }

	[[nodiscard]] uint64_t Raw(uint32_t n) noexcept {
		uint64_t v = 0;
		uint32_t sh = 0;
		while (n > 30) {
			v |= uint64_t{ d_.GetRaw(30) } << sh;
			sh += 30;
			n -= 30;
		}
		return v | (uint64_t{ d_.GetRaw(n) } << sh);
	}

	[[nodiscard]] uint64_t Class(uint32_t ctx) noexcept {
		const uint32_t c = Symbol(ctx);
		if (c <= 1) return c;
		return (uint64_t{ 1 } << (c - 1)) | Raw(c - 1);
	}

	[[nodiscard]] bool String(uint32_t ctx, std::string &text) {
		text.clear();
		for (;;) {
			const uint32_t b = Symbol(ctx);
			if (b == 0) return true;
			if (text.size() == kMaxString) return false;
			text.push_back(static_cast<char>(b));
		}
	}

	[[nodiscard]] bool Ok() const noexcept { return ok_; }

private:
	rans::Decoder d_;
	const Model *m_;
	bool ok_ = false;
};

} // namespace netcodec::detail
