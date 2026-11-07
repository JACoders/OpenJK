// rANS benchmark on the golden trace: netcodec::rans against ryg_rans rans_byte.h (public domain), same tables and
// symbols. Encoding: record + reverse coding. Decoding: three symbol lookups (binary search, linear search, full
// slot-to-symbol table). Times are the best of several runs over all messages of the trace.
// Usage: netcodec_bench_rans <rans_trace.bin> [runs]
#include "rans_trace.h"
#ifdef _MSC_VER
#pragma warning(push, 0)          // the original rans_byte.h, kept unchanged (system include for the other compilers)
#endif
#include <rans_byte.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace netcodec;

static double NowNs() {
	return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
	           std::chrono::steady_clock::now().time_since_epoch()).count());
}

template <class F> static double Best(int runs, F f) {
	double best = 1e300;
	for (int r = 0; r < runs; r++) {
		const double t0 = NowNs();
		f();
		best = std::min(best, NowNs() - t0);
	}
	return best;
}

struct Lut { std::vector<uint16_t> sym; };     // kProbScale entries per table

// coarse table: for each of the 2^B top-bit buckets of the slot, the symbol that holds the bucket's first slot;
// the search then goes forward from it (few steps: a bucket spans 2^(15-B) slots)
template <uint32_t B> struct Coarse {
	std::vector<uint16_t> first;
	void Build(const rans::Table &t) {
		first.resize(1u << B);
		uint32_t s = 0;
		for (uint32_t b = 0; b < (1u << B); b++) {
			const uint32_t slot = b << (rans::kProbBits - B);
			while (t.cum[s + 1] <= slot) s++;
			first[b] = static_cast<uint16_t>(s);
		}
	}
	uint32_t Find(const rans::Table &t, uint32_t slot) const {
		uint32_t s = first[slot >> (rans::kProbBits - B)];
		while (t.cum[s + 1] <= slot) s++;
		return s;
	}
};

static inline uint32_t FindBinary(const rans::Table &t, uint32_t slot) {
	uint32_t lo = 0, hi = t.K();
	while (hi - lo > 1) {
		const uint32_t mid = (lo + hi) >> 1;
		if (t.cum[mid] <= slot) lo = mid; else hi = mid;
	}
	return lo;
}

static inline uint32_t FindLinear(const rans::Table &t, uint32_t slot) {
	uint32_t s = 0;
	while (t.cum[s + 1] <= slot) s++;
	return s;
}

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s rans_trace.bin [runs]\n", argv[0]); return 2; }
	const int runs = argc > 2 ? atoi(argv[2]) : 20;
	trace::RansTrace t;
	if (!trace::LoadRansTrace(argv[1], t)) return 2;

	size_t nunits = 0, nsym = 0, maxUnits = 0, maxBytes = 0;
	for (const trace::Message &m : t.messages) {
		nunits += m.units.size(); maxUnits = std::max(maxUnits, m.units.size()); maxBytes = std::max(maxBytes, m.bytes.size());
		for (const trace::Unit &u : m.units) nsym += u.table != trace::kRaw;
	}
	const size_t nmsg = t.messages.size();
	std::vector<rans::Encoder::Op> ops(maxUnits * 3 + 1);
	std::vector<uint8_t> out(maxBytes + 4096);
	volatile uint32_t sink = 0;

	// ---- encode: netcodec
	const double encOurs = Best(runs, [&] {
		uint32_t acc = 0;
		for (const trace::Message &m : t.messages) {
			rans::Encoder e(ops);
			for (const trace::Unit &u : m.units) {
				if (u.table == trace::kRaw) e.PutRaw(u.value, u.bits);
				else rans::Put(e, t.tables[u.table], u.value);
			}
			acc += ToU32(e.Finish(out));
		}
		sink = sink + acc;
	});

	// ---- encode: ryg rans_byte.h, symbols with RansEncPutSymbol (reciprocal), raw chunks with RansEncPut (division)
	std::vector<std::vector<RansEncSymbol>> rygEnc(t.tables.size());
	for (size_t i = 0; i < t.tables.size(); i++) {
		rygEnc[i].resize(t.tables[i].K());
		for (uint32_t s = 0; s < t.tables[i].K(); s++)
			RansEncSymbolInit(&rygEnc[i][s], t.tables[i].cum[s], t.tables[i].cum[s + 1] - t.tables[i].cum[s], rans::kProbBits);
	}
	struct RygOp { const RansEncSymbol *sym; uint32_t start, freq; };
	std::vector<RygOp> rops(maxUnits * 3 + 1);
	const double encRyg = Best(runs, [&] {
		uint32_t acc = 0;
		for (const trace::Message &m : t.messages) {
			size_t n = 0;
			for (const trace::Unit &u : m.units) {
				if (u.table != trace::kRaw) { rops[n++] = RygOp{ &rygEnc[u.table][u.value], 0, 0 }; continue; }
				uint32_t v = u.value, bits = u.bits;
				while (bits > 0) {
					const uint32_t b = std::min(bits, rans::kRawChunk), sh = rans::kProbBits - b;
					rops[n++] = RygOp{ nullptr, (v & ((1u << b) - 1)) << sh, 1u << sh };
					v >>= b; bits -= b;
				}
			}
			RansState x; RansEncInit(&x);
			uint8_t *p = out.data() + out.size();
			for (size_t i = n; i-- > 0;) {
				if (rops[i].sym) RansEncPutSymbol(&x, &p, rops[i].sym);
				else RansEncPut(&x, &p, rops[i].start, rops[i].freq, rans::kProbBits);
			}
			RansEncFlush(&x, &p);
			acc += static_cast<uint32_t>(out.data() + out.size() - p);
		}
		sink = sink + acc;
	});

	// ---- decode: netcodec with three lookups
	std::vector<Lut> luts(t.tables.size());
	for (size_t i = 0; i < t.tables.size(); i++) {
		luts[i].sym.resize(rans::kProbScale);
		for (uint32_t s = 0; s < t.tables[i].K(); s++)
			for (uint32_t k = t.tables[i].cum[s]; k < t.tables[i].cum[s + 1]; k++) luts[i].sym[k] = static_cast<uint16_t>(s);
	}
	auto decode = [&](auto find) {
		return Best(runs, [&] {
			uint32_t acc = 0;
			for (const trace::Message &m : t.messages) {
				rans::Decoder d;
				if (!d.Init(m.bytes)) continue;
				for (const trace::Unit &u : m.units) {
					if (u.table == trace::kRaw) { acc += d.GetRaw(u.bits); continue; }
					const rans::Table &tb = t.tables[u.table];
					const uint32_t s = find(u.table, tb, d.Slot());
					d.Advance(tb.cum[s], static_cast<uint32_t>(tb.cum[s + 1]) - tb.cum[s]);
					acc += s;
				}
			}
			sink = sink + acc;
		});
	};
	// the codec's own path: rans::Get decodes a slot inside the bucket's first symbol without reading cum
	const double decOurs = Best(runs, [&] {
		uint32_t acc = 0;
		for (const trace::Message &m : t.messages) {
			rans::Decoder d;
			if (!d.Init(m.bytes)) continue;
			for (const trace::Unit &u : m.units) acc += u.table == trace::kRaw ? d.GetRaw(u.bits) : rans::Get(d, t.tables[u.table]);
		}
		sink = sink + acc;
	});
	const double decFind = decode([](uint32_t, const rans::Table &tb, uint32_t slot) { return rans::FindSymbol(tb, slot); });
	const double decBin = decode([](uint32_t, const rans::Table &tb, uint32_t slot) { return FindBinary(tb, slot); });
	const double decLin = decode([](uint32_t, const rans::Table &tb, uint32_t slot) { return FindLinear(tb, slot); });
	const double decLut = decode([&](uint32_t i, const rans::Table &, uint32_t slot) { return static_cast<uint32_t>(luts[i].sym[slot]); });
	std::vector<Coarse<6>> c6(t.tables.size()); std::vector<Coarse<8>> c8(t.tables.size()); std::vector<Coarse<10>> c10(t.tables.size());
	for (size_t i = 0; i < t.tables.size(); i++) { c6[i].Build(t.tables[i]); c8[i].Build(t.tables[i]); c10[i].Build(t.tables[i]); }
	const double decC6 = decode([&](uint32_t i, const rans::Table &tb, uint32_t slot) { return c6[i].Find(tb, slot); });
	const double decC8 = decode([&](uint32_t i, const rans::Table &tb, uint32_t slot) { return c8[i].Find(tb, slot); });
	const double decC10 = decode([&](uint32_t i, const rans::Table &tb, uint32_t slot) { return c10[i].Find(tb, slot); });

	// ---- decode: ryg rans_byte.h (needs its own encoding: starts from L), full-table lookup
	std::vector<std::vector<uint8_t>> rygBytes(nmsg);
	for (size_t mi = 0; mi < nmsg; mi++) {
		const trace::Message &m = t.messages[mi];
		size_t n = 0;
		for (const trace::Unit &u : m.units) {
			if (u.table != trace::kRaw) { rops[n++] = RygOp{ &rygEnc[u.table][u.value], 0, 0 }; continue; }
			uint32_t v = u.value, bits = u.bits;
			while (bits > 0) {
				const uint32_t b = std::min(bits, rans::kRawChunk), sh = rans::kProbBits - b;
				rops[n++] = RygOp{ nullptr, (v & ((1u << b) - 1)) << sh, 1u << sh };
				v >>= b; bits -= b;
			}
		}
		RansState x; RansEncInit(&x);
		uint8_t *p = out.data() + out.size();
		for (size_t i = n; i-- > 0;) {
			if (rops[i].sym) RansEncPutSymbol(&x, &p, rops[i].sym);
			else RansEncPut(&x, &p, rops[i].start, rops[i].freq, rans::kProbBits);
		}
		RansEncFlush(&x, &p);
		rygBytes[mi].assign(p, out.data() + out.size());
	}
	size_t rygTotal = 0, oursTotal = 0;
	for (size_t mi = 0; mi < nmsg; mi++) { rygTotal += rygBytes[mi].size(); oursTotal += t.messages[mi].bytes.size(); }
	auto decodeRyg = [&](auto find) { return Best(runs, [&] {
		uint32_t acc = 0;
		for (size_t mi = 0; mi < nmsg; mi++) {
			const trace::Message &m = t.messages[mi];
			uint8_t *p = rygBytes[mi].data();
			RansState x; RansDecInit(&x, &p);
			for (const trace::Unit &u : m.units) {
				if (u.table == trace::kRaw) {
					uint32_t bits = u.bits, sh0 = 0, v = 0;
					while (bits > 0) {
						const uint32_t b = std::min(bits, rans::kRawChunk), sh = rans::kProbBits - b;
						const uint32_t c = RansDecGet(&x, rans::kProbBits) >> sh;
						RansDecAdvance(&x, &p, c << sh, 1u << sh, rans::kProbBits);
						v |= c << sh0; sh0 += b; bits -= b;
					}
					acc += v; continue;
				}
				const rans::Table &tb = t.tables[u.table];
				const uint32_t s = find(u.table, tb, RansDecGet(&x, rans::kProbBits));
				RansDecAdvance(&x, &p, tb.cum[s], static_cast<uint32_t>(tb.cum[s + 1]) - tb.cum[s], rans::kProbBits);
				acc += s;
			}
		}
		sink = sink + acc;
	}); };
	const double decRyg = decodeRyg([&](uint32_t i, const rans::Table &, uint32_t slot) { return static_cast<uint32_t>(luts[i].sym[slot]); });
	const double decRygBin = decodeRyg([](uint32_t, const rans::Table &tb, uint32_t slot) { return FindBinary(tb, slot); });
	const double decRygC8 = decodeRyg([&](uint32_t i, const rans::Table &tb, uint32_t slot) { return c8[i].Find(tb, slot); });
	const double decRygFind = decodeRyg([](uint32_t, const rans::Table &tb, uint32_t slot) { return rans::FindSymbol(tb, slot); });

	printf("trace: %zu messages, %zu units (%zu table symbols), %zu tables\n", nmsg, nunits, nsym, t.tables.size());
	printf("size: netcodec %zu bytes, ryg (state starts at L) %zu bytes (%+.2f bytes per message)\n",
	       oursTotal, rygTotal, (static_cast<double>(rygTotal) - static_cast<double>(oursTotal)) / static_cast<double>(nmsg));
	auto row = [&](const char *name, double ns) {
		printf("  %-34s %8.1f ns/message %6.2f ns/unit\n", name, ns / static_cast<double>(nmsg), ns / static_cast<double>(nunits));
	};
	row("encode netcodec", encOurs);
	row("encode ryg", encRyg);
	row("decode netcodec (rans::Get, codec path)", decOurs);
	row("decode netcodec, FindSymbol + cum", decFind);
	row("decode netcodec, binary search", decBin);
	row("decode netcodec, linear search", decLin);
	row("decode netcodec, full table", decLut);
	row("decode netcodec, coarse table 2^6", decC6);
	row("decode netcodec, coarse table 2^8", decC8);
	row("decode netcodec, coarse table 2^10", decC10);
	row("decode ryg, full table", decRyg);
	row("decode ryg, binary search", decRygBin);
	row("decode ryg, coarse table 2^8", decRygC8);
	row("decode ryg, FindSymbol + cum", decRygFind);
	return sink == 0x12345678 ? 1 : 0;
}
