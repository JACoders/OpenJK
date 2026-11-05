// CPU + size benchmark of real third-party entropy coder libraries on the same JKA snapshot streams as bench.c.
//
// Symbol-stream methods (new schema, static pre-shared tables per context, 12-bit probabilities):
//   R  ryg_rans  rans_byte.h  (CC0)   -- 32-bit state, byte renormalisation
//   F  FSE (tANS) by Yann Collet (BSD-2 lib) -- ONE tANS state shared by all contexts (tables switched per symbol),
//      table log 12 so the final state flush is 12 bits
// Byte-stream methods (engine layout, drop-in for Huffman: the bytes MSG_WriteBits sends through Huffman are coded,
// the low raw bits stay raw and are added to the size):
//   Q  qbc  (MIT)      adaptive binary arithmetic coder, order-0, state reset per packet
//   N  nibrans (CC0)   adaptive nibble rANS, SSE2; reset per packet (safe on UDP), and "persistent" (model kept across
//                      packets: only valid if no packet is ever lost, shown for information)
//
// Timing covers coding of already built symbol / byte lists only.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
static double now_ns()
{
	static LARGE_INTEGER f;
	LARGE_INTEGER c;
	if (!f.QuadPart) QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&c);
	return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
}
#else
#include <time.h>
static double now_ns()
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
}
#endif

#define FSE_STATIC_LINKING_ONLY
#include "fse.h"
#include "rans_byte.h"
#include "qbc.c"
// nibrans uses C99 compound literals: its implementation is compiled as C (nibrans_impl.c)
extern "C" {
#include "nibrans.h"
}

#define REPEATS 30
#define MAXPKT 16384
#define SCALE_BITS 12

struct Sym { uint16_t ctx, sym; uint8_t raw; };
struct Call { int8_t bits; uint32_t value; };
struct Snap {
	std::vector<Sym> sym;
	std::vector<Call> call;
	std::vector<uint8_t> bytes;     // bytes the engine sends through Huffman
	uint32_t rawbits = 0;           // low raw bits of the same calls
};
struct Ctx {
	uint32_t K = 0;
	std::vector<uint16_t> freq, cum;
	std::vector<uint8_t> lut;
	std::vector<RansEncSymbol> esym;
	FSE_CTable *ct = nullptr;
	FSE_DTable *dt = nullptr;
};

static std::vector<Ctx> ctxs;
static std::vector<Snap> snaps;

static void rd(void *p, size_t n, FILE *f)
{
	if (fread(p, 1, n, f) != n) { fprintf(stderr, "read error\n"); exit(1); }
}
static uint32_t rd32(FILE *f) { uint32_t v; rd(&v, 4, f); return v; }

static void load(const char *fn)
{
	FILE *f = fopen(fn, "rb");
	char magic[4];
	if (!f) { fprintf(stderr, "cannot open %s\n", fn); exit(1); }
	rd(magic, 4, f);
	if (memcmp(magic, "JKB1", 4)) { fprintf(stderr, "bad magic\n"); exit(1); }
	uint32_t nctx = rd32(f), nnode = rd32(f), nsnap = rd32(f), ntree = rd32(f);
	ctxs.resize(nctx);
	for (uint32_t i = 0; i < nctx; i++) {
		Ctx &c = ctxs[i];
		c.K = rd32(f);
		c.freq.resize(c.K);
		rd(c.freq.data(), c.K * 2, f);
		std::vector<uint8_t> hlen(c.K);
		rd(hlen.data(), c.K, f);
		c.cum.resize(c.K + 1);
		c.lut.resize(1 << SCALE_BITS);
		uint32_t x = 0;
		for (uint32_t s = 0; s < c.K; s++) {
			c.cum[s] = (uint16_t)x;
			for (uint32_t j = 0; j < c.freq[s]; j++) c.lut[x + j] = (uint8_t)s;
			x += c.freq[s];
		}
		c.cum[c.K] = (uint16_t)x;
		c.esym.resize(c.K);
		for (uint32_t s = 0; s < c.K; s++) RansEncSymbolInit(&c.esym[s], c.cum[s], c.freq[s], SCALE_BITS);
		if (c.K >= 2) {
			std::vector<short> norm(c.K);
			for (uint32_t s = 0; s < c.K; s++) norm[s] = (short)c.freq[s];
			c.ct = FSE_createCTable(c.K - 1, SCALE_BITS);
			c.dt = FSE_createDTable(SCALE_BITS);
			if (FSE_isError(FSE_buildCTable(c.ct, norm.data(), c.K - 1, SCALE_BITS)) ||
			    FSE_isError(FSE_buildDTable(c.dt, norm.data(), c.K - 1, SCALE_BITS))) {
				fprintf(stderr, "FSE table build failed for context %u (K=%u)\n", i, c.K);
				exit(1);
			}
		}
	}
	std::vector<uint8_t> skip(nnode * 2 + ntree * 16 + 4);
	rd(skip.data(), skip.size(), f);
	snaps.resize(nsnap);
	for (uint32_t i = 0; i < nsnap; i++) {
		Snap &s = snaps[i];
		uint32_t n = rd32(f);
		s.sym.resize(n);
		for (uint32_t j = 0; j < n; j++) { rd(&s.sym[j].ctx, 2, f); rd(&s.sym[j].sym, 2, f); rd(&s.sym[j].raw, 1, f); }
		n = rd32(f);
		std::vector<uint8_t> d(n * 5);
		rd(d.data(), d.size(), f);
		n = rd32(f);
		s.call.resize(n);
		for (uint32_t j = 0; j < n; j++) {
			rd(&s.call[j].bits, 1, f); rd(&s.call[j].value, 4, f);
			int b = s.call[j].bits < 0 ? -s.call[j].bits : s.call[j].bits;
			int nb = b & 7;
			s.rawbits += nb;
			for (int k = 0; k < (b - nb) / 8; k++) s.bytes.push_back((uint8_t)((s.call[j].value >> (nb + 8 * k)) & 255));
		}
		uint32_t nbits = rd32(f), nbytes = rd32(f);
		(void)nbits;
		std::vector<uint8_t> pk(nbytes);
		rd(pk.data(), nbytes, f);
	}
	fclose(f);
}

// ---------------------------------------------------------------- R: ryg_rans
static uint32_t r_encode(const Snap &s, uint8_t *end, uint32_t *rawbytes)
{
	uint8_t *p = end;
	RansState x;
	uint32_t raw = 0;
	for (const Sym &q : s.sym) raw += q.raw;
	*rawbytes = (raw + 7) >> 3;
	RansEncInit(&x);
	for (int32_t i = (int32_t)s.sym.size() - 1; i >= 0; i--)
		RansEncPutSymbol(&x, &p, &ctxs[s.sym[i].ctx].esym[s.sym[i].sym]);
	RansEncFlush(&x, &p);
	return (uint32_t)(end - p);
}
static void r_decode(const Snap &s, const uint8_t *p, uint16_t *out)
{
	RansState x;
	RansDecInit(&x, (uint8_t **)&p);
	for (size_t i = 0; i < s.sym.size(); i++) {
		const Ctx &c = ctxs[s.sym[i].ctx];
		uint32_t cum = RansDecGet(&x, SCALE_BITS);
		uint32_t sym = c.lut[cum];
		out[i] = (uint16_t)sym;
		RansDecAdvance(&x, (uint8_t **)&p, c.cum[sym], c.freq[sym], SCALE_BITS);
	}
}

// ---------------------------------------------------------------- F: FSE, one state, tables switched per symbol
static uint32_t f_encode(const Snap &s, uint8_t *out)
{
	BIT_CStream_t bitC;
	FSE_CState_t cs;
	bool first = true;
	uint32_t raw = 0;
	for (const Sym &q : s.sym) raw += q.raw;
	BIT_initCStream(&bitC, out, MAXPKT);
	for (int32_t i = (int32_t)s.sym.size() - 1; i >= 0; i--) {
		const Ctx &c = ctxs[s.sym[i].ctx];
		if (c.K < 2) continue;
		if (first) { FSE_initCState2(&cs, c.ct, s.sym[i].sym); first = false; }
		else {
			cs.stateTable = (const U16 *)c.ct + 2;
			cs.symbolTT = c.ct + 1 + (1 << (SCALE_BITS - 1));
			FSE_encodeSymbol(&bitC, &cs, s.sym[i].sym);
			BIT_flushBits(&bitC);
		}
	}
	if (!first) FSE_flushCState(&bitC, &cs);
	return (uint32_t)BIT_closeCStream(&bitC) + ((raw + 7) >> 3);
}
static void f_decode(const Snap &s, const uint8_t *buf, uint32_t size, uint16_t *out)
{
	BIT_DStream_t bitD;
	FSE_DState_t ds;
	bool first = true;
	BIT_initDStream(&bitD, buf, size);
	for (size_t i = 0; i < s.sym.size(); i++) {
		const Ctx &c = ctxs[s.sym[i].ctx];
		if (c.K < 2) { out[i] = 0; continue; }
		if (first) { FSE_initDState(&ds, &bitD, c.dt); first = false; }
		else ds.table = c.dt + 1;
		out[i] = FSE_decodeSymbol(&ds, &bitD);
		BIT_reloadDStream(&bitD);
	}
}

// ---------------------------------------------------------------- Q: qbc
static uint32_t q_encode(const Snap &s, uint8_t *out)
{
	bitcodec_t bc;
	bc_init_encode(&bc, out, MAXPKT);
	for (uint8_t b : s.bytes) bc_encode_byte(&bc, b);
	bc_encode_flush(&bc);
	return (uint32_t)bc_length(&bc) + ((s.rawbits + 7) >> 3);
}
static void q_decode(const Snap &s, const uint8_t *buf, uint32_t size, uint8_t *out)
{
	bitcodec_t bc;
	bc_init_decode(&bc, (unsigned char *)buf, (int)size);
	for (size_t i = 0; i < s.bytes.size(); i++) out[i] = (uint8_t)decode_byte(&bc);
}

// ---------------------------------------------------------------- N: nibrans
static uint32_t n_encode(const Snap &s, uint8_t *out, nibrans *m, uint32_t *encoded)
{
	size_t n = nibransEncode(m, out, MAXPKT, s.bytes.data(), s.bytes.size());
	*encoded = (uint32_t)n;
	return (uint32_t)n + ((s.rawbits + 7) >> 3);
}

struct Row { const char *name; double enc, dec, bytes; bool ok; };
static void report(const Row &r)
{
	double c20 = r.enc * 20.0 / 1e9 * 100.0;
	printf("%-46s %8.0f %8.0f %9.1f   %5.2f%% %6.2f%% %6.2f%%  %s\n", r.name, r.enc, r.dec, r.bytes, c20 * 8, c20 * 32, c20 * 64, r.ok ? "" : "VERIFY FAILED");
}

int main(int argc, char **argv)
{
	load(argc > 1 ? argv[1] : "bench_input.bin");
	size_t n = snaps.size();
	std::vector<std::vector<uint8_t>> store(n, std::vector<uint8_t>(MAXPKT));
	std::vector<uint32_t> len(n), rawlen(n), enclen(n);
	std::vector<uint16_t> symout(65536);
	std::vector<uint8_t> byteout(65536);
	double t0, t1, enc, dec, bytes;
	bool ok;
	printf("snapshots %zu, contexts %zu\n", n, ctxs.size());
	printf("%-46s %8s %8s %9s   %6s %7s %7s\n", "method", "enc ns", "dec ns", "bytes", "8 cl", "32 cl", "64 cl");

	// R
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) len[i] = r_encode(snaps[i], store[i].data() + MAXPKT, &rawlen[i]);
	enc = (now_ns() - t0) / (double)(REPEATS * n);
	bytes = 0; for (size_t i = 0; i < n; i++) bytes += len[i] + rawlen[i];
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) r_decode(snaps[i], store[i].data() + MAXPKT - len[i], symout.data());
	dec = (now_ns() - t0) / (double)(REPEATS * n);
	ok = true;
	for (size_t i = 0; i < n; i++) { r_decode(snaps[i], store[i].data() + MAXPKT - len[i], symout.data()); for (size_t j = 0; j < snaps[i].sym.size(); j++) if (symout[j] != snaps[i].sym[j].sym) ok = false; }
	report({ "R ryg_rans rans_byte.h, 12-bit, new schema", enc, dec, bytes / n, ok });

	// F
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) len[i] = f_encode(snaps[i], store[i].data());
	enc = (now_ns() - t0) / (double)(REPEATS * n);
	bytes = 0; for (size_t i = 0; i < n; i++) bytes += len[i];
	// FSE payload size without the raw bits, for decoding
	std::vector<uint32_t> fsz(n);
	for (size_t i = 0; i < n; i++) { uint32_t raw = 0; for (const Sym &q : snaps[i].sym) raw += q.raw; fsz[i] = len[i] - ((raw + 7) >> 3); }
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) f_decode(snaps[i], store[i].data(), fsz[i], symout.data());
	dec = (now_ns() - t0) / (double)(REPEATS * n);
	ok = true;
	for (size_t i = 0; i < n; i++) { f_decode(snaps[i], store[i].data(), fsz[i], symout.data()); for (size_t j = 0; j < snaps[i].sym.size(); j++) if (symout[j] != snaps[i].sym[j].sym) ok = false; }
	report({ "F FSE tANS, one shared state, 12-bit, new schema", enc, dec, bytes / n, ok });

	// Q
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) len[i] = q_encode(snaps[i], store[i].data());
	enc = (now_ns() - t0) / (double)(REPEATS * n);
	bytes = 0; for (size_t i = 0; i < n; i++) bytes += len[i];
	for (size_t i = 0; i < n; i++) rawlen[i] = len[i] - ((snaps[i].rawbits + 7) >> 3);
	t0 = now_ns();
	for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) q_decode(snaps[i], store[i].data(), rawlen[i], byteout.data());
	dec = (now_ns() - t0) / (double)(REPEATS * n);
	ok = true;
	for (size_t i = 0; i < n; i++) { q_decode(snaps[i], store[i].data(), rawlen[i], byteout.data()); for (size_t j = 0; j < snaps[i].bytes.size(); j++) if (byteout[j] != snaps[i].bytes[j]) ok = false; }
	report({ "Q qbc, engine byte stream, reset per packet", enc, dec, bytes / n, ok });

	// N (reset per packet)
	{
		nibrans m;
		t0 = now_ns();
		for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) { nibransInit(&m); len[i] = n_encode(snaps[i], store[i].data(), &m, &enclen[i]); }
		enc = (now_ns() - t0) / (double)(REPEATS * n);
		bytes = 0; for (size_t i = 0; i < n; i++) bytes += len[i];
		t0 = now_ns();
		for (int r = 0; r < REPEATS; r++) for (size_t i = 0; i < n; i++) { nibransInit(&m); nibransDecode(&m, byteout.data(), snaps[i].bytes.size(), store[i].data(), enclen[i]); }
		dec = (now_ns() - t0) / (double)(REPEATS * n);
		ok = true;
		for (size_t i = 0; i < n; i++) { nibransInit(&m); nibransDecode(&m, byteout.data(), snaps[i].bytes.size(), store[i].data(), enclen[i]); for (size_t j = 0; j < snaps[i].bytes.size(); j++) if (byteout[j] != snaps[i].bytes[j]) ok = false; }
		report({ "N nibrans, engine byte stream, reset per packet", enc, dec, bytes / n, ok });
	}
	// N persistent (information only: breaks on the first lost packet)
	{
		nibrans me, md;
		nibransInit(&me); nibransInit(&md);
		bytes = 0; ok = true;
		for (size_t i = 0; i < n; i++) {
			uint32_t e;
			bytes += n_encode(snaps[i], store[i].data(), &me, &e);
			nibransDecode(&md, byteout.data(), snaps[i].bytes.size(), store[i].data(), e);
			for (size_t j = 0; j < snaps[i].bytes.size(); j++) if (byteout[j] != snaps[i].bytes[j]) ok = false;
		}
		printf("%-46s %8s %8s %9.1f   (model kept across packets: invalid on UDP loss)%s\n", "N nibrans, persistent model", "-", "-", bytes / n, ok ? "" : "  VERIFY FAILED");
	}
	return 0;
}
