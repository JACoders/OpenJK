/*
 * CPU micro-benchmark of entropy coders on real JKA snapshot symbol streams.
 * Input: bench_input.bin (written by dump_bench.py). C99, single file, no dependencies.
 *
 * Methods:
 *   A  engine adaptive-tree Huffman, current layout (tree walk decode, parent-walk encode like Huff_offsetTransmit)
 *   B  static canonical Huffman per context, new schema (bit by bit, no lookahead table)
 *   C  static rANS32, 12-bit probabilities per context, new schema (LUT decode)
 *   D  adaptive binary arithmetic coder (lpaq style, 12-bit probs, rate 1/32), state reset per packet
 *
 * Timing covers encoding or decoding of the already-parsed symbol list only. Game logic (building the symbols
 * from entity states, applying deltas) is not included and is identical for all methods in the new schema.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static double now_ns(void)
{
	static LARGE_INTEGER f;
	LARGE_INTEGER c;
	if (!f.QuadPart) QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&c);
	return (double)c.QuadPart * 1e9 / (double)f.QuadPart;
}
#else
#include <time.h>
static double now_ns(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
}
#endif

#define REPEATS 30
#define MAXPKT 16384

typedef struct { uint16_t ctx, sym; uint8_t raw; } Sym;
typedef struct { uint32_t node; uint8_t bit; } Dec;
typedef struct { int8_t bits; uint32_t value; } Call;
typedef struct {
	uint32_t nsym; Sym *sym;
	uint32_t ndec; Dec *dec;
	uint32_t ncall; Call *call;
	uint32_t nbits, nbytes; uint8_t *pkt;
	uint32_t rawbits;
} Snap;

typedef struct {
	uint32_t K;
	uint16_t *freq, *cum;
	uint8_t *lut;                 /* 4096 entries slot -> symbol */
	uint8_t *hlen;
	uint32_t *hcode;
	uint32_t cnt[64], first[64], off[64];
	uint16_t *sorted;
	int maxlen;
} Ctx;

typedef struct { int32_t left, right, sym, parent; } TNode;

static uint32_t nctx, nnode, nsnap, ntree, troot;
static Ctx *ctxs;
static uint16_t *pinit;
static TNode *tree;
static int32_t leaf_of[257];
static Snap *snaps;

static void rd(void *p, size_t n, FILE *f)
{
	if (fread(p, 1, n, f) != n) { fprintf(stderr, "read error\n"); exit(1); }
}
static uint32_t rd32(FILE *f) { uint32_t v; rd(&v, 4, f); return v; }

static void build_ctx(Ctx *c)
{
	uint32_t i, len, code = 0, total = 0;
	uint32_t next[64];
	c->maxlen = 0;
	memset(c->cnt, 0, sizeof c->cnt);
	for (i = 0; i < c->K; i++) { if (c->hlen[i] > c->maxlen) c->maxlen = c->hlen[i]; c->cnt[c->hlen[i]]++; }
	if (c->maxlen >= 32) { fprintf(stderr, "code length %d too long\n", c->maxlen); exit(1); }
	c->sorted = (uint16_t *)malloc(c->K * sizeof(uint16_t));
	c->hcode = (uint32_t *)malloc(c->K * sizeof(uint32_t));
	for (len = 1; len <= (uint32_t)c->maxlen; len++) {
		c->first[len] = code; c->off[len] = total; next[len] = code;
		total += c->cnt[len]; code = (code + c->cnt[len]) << 1;
	}
	for (len = 1; len <= (uint32_t)c->maxlen; len++) {
		uint32_t k = 0;
		for (i = 0; i < c->K; i++) if (c->hlen[i] == len) {
			c->sorted[c->off[len] + k] = (uint16_t)i; c->hcode[i] = next[len]++; k++;
		}
	}
	/* rANS tables */
	c->cum = (uint16_t *)malloc((c->K + 1) * sizeof(uint16_t));
	c->lut = (uint8_t *)malloc(4096);
	{
		uint32_t s, x = 0;
		for (s = 0; s < c->K; s++) {
			uint32_t j;
			c->cum[s] = (uint16_t)x;
			for (j = 0; j < c->freq[s]; j++) c->lut[x + j] = (uint8_t)s;
			x += c->freq[s];
		}
		c->cum[c->K] = (uint16_t)x;
	}
}

static void load(const char *fn)
{
	FILE *f = fopen(fn, "rb");
	uint32_t i, j;
	char magic[4];
	if (!f) { fprintf(stderr, "cannot open %s\n", fn); exit(1); }
	rd(magic, 4, f);
	if (memcmp(magic, "JKB1", 4)) { fprintf(stderr, "bad magic\n"); exit(1); }
	nctx = rd32(f); nnode = rd32(f); nsnap = rd32(f); ntree = rd32(f);
	ctxs = (Ctx *)calloc(nctx, sizeof(Ctx));
	for (i = 0; i < nctx; i++) {
		Ctx *c = &ctxs[i];
		c->K = rd32(f);
		c->freq = (uint16_t *)malloc(c->K * 2);
		c->hlen = (uint8_t *)malloc(c->K);
		rd(c->freq, c->K * 2, f);
		rd(c->hlen, c->K, f);
		build_ctx(c);
	}
	pinit = (uint16_t *)malloc(nnode * 2);
	rd(pinit, nnode * 2, f);
	tree = (TNode *)malloc(ntree * sizeof(TNode));
	rd(tree, ntree * sizeof(TNode), f);
	troot = rd32(f);
	for (i = 0; i < 257; i++) leaf_of[i] = -1;
	for (i = 0; i < ntree; i++) if (tree[i].sym >= 0) leaf_of[tree[i].sym] = (int32_t)i;
	snaps = (Snap *)calloc(nsnap, sizeof(Snap));
	for (i = 0; i < nsnap; i++) {
		Snap *s = &snaps[i];
		s->nsym = rd32(f);
		s->sym = (Sym *)malloc(s->nsym * sizeof(Sym));
		for (j = 0; j < s->nsym; j++) {
			rd(&s->sym[j].ctx, 2, f); rd(&s->sym[j].sym, 2, f); rd(&s->sym[j].raw, 1, f);
			s->rawbits += s->sym[j].raw;
		}
		s->ndec = rd32(f);
		s->dec = (Dec *)malloc(s->ndec * sizeof(Dec));
		for (j = 0; j < s->ndec; j++) { rd(&s->dec[j].node, 4, f); rd(&s->dec[j].bit, 1, f); }
		s->ncall = rd32(f);
		s->call = (Call *)malloc(s->ncall * sizeof(Call));
		for (j = 0; j < s->ncall; j++) { rd(&s->call[j].bits, 1, f); rd(&s->call[j].value, 4, f); }
		s->nbits = rd32(f); s->nbytes = rd32(f);
		s->pkt = (uint8_t *)malloc(s->nbytes + 8);
		rd(s->pkt, s->nbytes, f);
	}
	fclose(f);
}

/* ------------------------------------------------------------------ bit io (LSB first, like the engine) */
typedef struct { uint8_t *buf; uint32_t pos; } BW;
static void put_bit(BW *w, uint32_t b)
{
	if ((w->pos & 7) == 0) w->buf[w->pos >> 3] = 0;
	w->buf[w->pos >> 3] |= (uint8_t)(b << (w->pos & 7));
	w->pos++;
}
static void put_bits(BW *w, uint32_t v, int n) { int i; for (i = 0; i < n; i++) put_bit(w, (v >> i) & 1); }
static uint32_t get_bit(const uint8_t *buf, uint32_t *pos)
{
	uint32_t t = (buf[*pos >> 3] >> (*pos & 7)) & 1;
	(*pos)++;
	return t;
}

/* ------------------------------------------------------------------ A: engine Huffman */
static uint32_t a_encode(const Snap *s, uint8_t *out)
{
	BW w; uint32_t i;
	w.buf = out; w.pos = 0;
	for (i = 0; i < s->ncall; i++) {
		int b = s->call[i].bits < 0 ? -s->call[i].bits : s->call[i].bits;
		int nb = b & 7, k;
		uint32_t v = s->call[i].value;
		put_bits(&w, v, nb);
		for (k = 0; k < (b - nb) / 8; k++) {
			uint32_t sym = (v >> (nb + 8 * k)) & 255;
			int32_t n = leaf_of[sym];
			uint8_t stack[40]; int sp = 0;
			while (tree[n].parent >= 0) {         /* walk up like send() does with recursion */
				int32_t p = tree[n].parent;
				stack[sp++] = (tree[p].right == n);
				n = p;
			}
			while (sp) put_bit(&w, stack[--sp]);
		}
	}
	return w.pos;
}
static uint32_t a_decode(const Snap *s, uint32_t *vals)
{
	uint32_t pos = 0, i;
	for (i = 0; i < s->ncall; i++) {
		int b = s->call[i].bits < 0 ? -s->call[i].bits : s->call[i].bits;
		int nb = b & 7, k;
		uint32_t v = 0, j;
		for (j = 0; j < (uint32_t)nb; j++) v |= get_bit(s->pkt, &pos) << j;
		for (k = 0; k < (b - nb) / 8; k++) {
			int32_t n = (int32_t)troot;
			while (tree[n].sym < 0) n = get_bit(s->pkt, &pos) ? tree[n].right : tree[n].left;
			v |= (uint32_t)tree[n].sym << (nb + 8 * k);
		}
		vals[i] = v;
	}
	return pos;
}

/* ------------------------------------------------------------------ B: static canonical Huffman per ctx */
static uint32_t b_encode(const Snap *s, uint8_t *out)
{
	BW w; uint32_t i; int k;
	w.buf = out; w.pos = 0;
	for (i = 0; i < s->nsym; i++) {
		const Ctx *c = &ctxs[s->sym[i].ctx];
		uint32_t code = c->hcode[s->sym[i].sym];
		for (k = c->hlen[s->sym[i].sym] - 1; k >= 0; k--) put_bit(&w, (code >> k) & 1);
		put_bits(&w, 0, s->sym[i].raw);
	}
	return w.pos;
}
static void b_decode(const Snap *s, const uint8_t *buf, uint16_t *out)
{
	uint32_t pos = 0, i;
	for (i = 0; i < s->nsym; i++) {
		const Ctx *c = &ctxs[s->sym[i].ctx];
		uint32_t code = 0, len = 0;
		for (;;) {
			code = (code << 1) | get_bit(buf, &pos); len++;
			if ((int)len > c->maxlen) { out[i] = 0xffff; break; }
			if (code - c->first[len] < c->cnt[len]) { out[i] = c->sorted[c->off[len] + (code - c->first[len])]; break; }
		}
		pos += s->sym[i].raw;
	}
}

/* ------------------------------------------------------------------ C: rANS32, 12-bit */
#define RL (1u << 23)
static uint32_t c_encode(const Snap *s, uint8_t *out_end, uint8_t *rawbuf, uint32_t *rawbytes)
{
	uint8_t *p = out_end; uint32_t x = RL; int32_t i;
	BW w; uint32_t r;
	w.buf = rawbuf; w.pos = 0;
	for (r = 0; r < s->nsym; r++) put_bits(&w, 0, s->sym[r].raw);
	*rawbytes = (w.pos + 7) >> 3;
	for (i = (int32_t)s->nsym - 1; i >= 0; i--) {
		const Ctx *c = &ctxs[s->sym[i].ctx];
		uint32_t freq = c->freq[s->sym[i].sym], start = c->cum[s->sym[i].sym];
		uint32_t xmax = ((RL >> 12) << 8) * freq;
		while (x >= xmax) { *--p = (uint8_t)(x & 255); x >>= 8; }
		x = ((x / freq) << 12) + (x % freq) + start;
	}
	p -= 4; p[0] = (uint8_t)x; p[1] = (uint8_t)(x >> 8); p[2] = (uint8_t)(x >> 16); p[3] = (uint8_t)(x >> 24);
	return (uint32_t)(out_end - p);
}
static void c_decode(const Snap *s, const uint8_t *p, uint16_t *out)
{
	uint32_t x = p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24), i;
	p += 4;
	for (i = 0; i < s->nsym; i++) {
		const Ctx *c = &ctxs[s->sym[i].ctx];
		uint32_t slot = x & 4095, sym = c->lut[slot];
		out[i] = (uint16_t)sym;
		x = c->freq[sym] * (x >> 12) + slot - c->cum[sym];
		while (x < RL) x = (x << 8) | *p++;
	}
}

/* ------------------------------------------------------------------ D: adaptive binary arithmetic coder */
static uint16_t *dcur;
static uint32_t *dstamp;
static uint32_t dpkt = 0;
static uint32_t d_encode(const Snap *s, uint8_t *out)
{
	uint32_t x1 = 0, x2 = 0xffffffffu, i, n = 0;
	dpkt++;
	for (i = 0; i < s->ndec; i++) {
		uint32_t node = s->dec[i].node, bit = s->dec[i].bit, xmid;
		int p;
		if (dstamp[node] != dpkt) { dstamp[node] = dpkt; dcur[node] = pinit[node]; }
		p = dcur[node];
		xmid = x1 + ((x2 - x1) >> 12) * (uint32_t)p;
		if (bit) x2 = xmid; else x1 = xmid + 1;
		p += (int)(((int)(bit << 12) - p) >> 5);
		if (p < 1) p = 1;
		if (p > 4095) p = 4095;
		dcur[node] = (uint16_t)p;
		while (((x1 ^ x2) & 0xff000000u) == 0) {
			out[n++] = (uint8_t)(x2 >> 24); x1 <<= 8; x2 = (x2 << 8) | 255;
		}
	}
	out[n++] = (uint8_t)(x1 >> 24);
	return n;
}
static void d_decode(const Snap *s, const uint8_t *buf, uint32_t nbuf, uint8_t *bits)
{
	uint32_t x1 = 0, x2 = 0xffffffffu, x = 0, pos = 0, i, k;
	dpkt++;
	for (k = 0; k < 4; k++) x = (x << 8) | (pos < nbuf ? buf[pos++] : 255);
	for (i = 0; i < s->ndec; i++) {
		uint32_t node = s->dec[i].node, xmid, bit;
		int p;
		if (dstamp[node] != dpkt) { dstamp[node] = dpkt; dcur[node] = pinit[node]; }
		p = dcur[node];
		xmid = x1 + ((x2 - x1) >> 12) * (uint32_t)p;
		bit = x <= xmid;
		if (bit) x2 = xmid; else x1 = xmid + 1;
		bits[i] = (uint8_t)bit;
		p += (int)(((int)(bit << 12) - p) >> 5);
		if (p < 1) p = 1;
		if (p > 4095) p = 4095;
		dcur[node] = (uint16_t)p;
		while (((x1 ^ x2) & 0xff000000u) == 0) {
			x1 <<= 8; x2 = (x2 << 8) | 255; x = (x << 8) | (pos < nbuf ? buf[pos++] : 255);
		}
	}
}

/* ------------------------------------------------------------------ driver */
static void report(const char *name, double enc_ns, double dec_ns, double bytes)
{
	double c20 = enc_ns * 20.0 / 1e9 * 100.0;       /* % of one core per client at 20 snapshots/s */
	printf("%-44s %8.0f %8.0f %9.1f   %5.2f%% %6.2f%% %6.2f%%\n", name, enc_ns, dec_ns, bytes,
	       c20 * 8, c20 * 32, c20 * 64);
}

int main(int argc, char **argv)
{
	uint32_t i, r, bad;
	static uint8_t encbuf[MAXPKT], rawbuf[MAXPKT];
	static uint32_t vals[65536];
	static uint16_t symout[65536];
	static uint8_t bitout[1 << 20];
	uint8_t **store;
	uint32_t *storelen;
	double t0, t1, enc_ns, dec_ns, bytes;
	const char *fn = argc > 1 ? argv[1] : "bench_input.bin";

	load(fn);
	dcur = (uint16_t *)calloc(nnode, 2);
	dstamp = (uint32_t *)calloc(nnode, 4);
	store = (uint8_t **)calloc(nsnap, sizeof(uint8_t *));
	storelen = (uint32_t *)calloc(nsnap, 4);
	printf("snapshots %u, contexts %u, binary nodes %u\n", nsnap, nctx, nnode);
	printf("enc/dec = nanoseconds per snapshot; %% = share of one core spent on encoding for N clients at 20 snapshots/s\n");
	printf("%-44s %8s %8s %9s   %6s %7s %7s\n", "method", "enc ns", "dec ns", "bytes", "8 cl", "32 cl", "64 cl");

	/* A */
	for (i = 0; i < nsnap; i++) { store[i] = (uint8_t *)malloc(MAXPKT); }
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) storelen[i] = a_encode(&snaps[i], store[i]);
	t1 = now_ns(); enc_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bytes = 0; for (i = 0; i < nsnap; i++) bytes += (storelen[i] + 7) / 8;
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) a_decode(&snaps[i], vals);
	t1 = now_ns(); dec_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bad = 0;
	for (i = 0; i < nsnap; i++) { uint32_t j; a_decode(&snaps[i], vals); for (j = 0; j < snaps[i].ncall; j++) { int b = snaps[i].call[j].bits < 0 ? -snaps[i].call[j].bits : snaps[i].call[j].bits; uint32_t m = b == 32 ? 0xffffffffu : ((1u << b) - 1); if ((vals[j] & m) != (snaps[i].call[j].value & m)) bad++; } if (storelen[i] != snaps[i].nbits) bad++; }
	report("A engine Huffman (current layout)", enc_ns, dec_ns, bytes / nsnap);
	if (bad) printf("   !! A verification failures: %u\n", bad);

	/* B */
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) storelen[i] = b_encode(&snaps[i], store[i]);
	t1 = now_ns(); enc_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bytes = 0; for (i = 0; i < nsnap; i++) bytes += (storelen[i] + 7) / 8;
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) b_decode(&snaps[i], store[i], symout);
	t1 = now_ns(); dec_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bad = 0;
	for (i = 0; i < nsnap; i++) { uint32_t j; b_decode(&snaps[i], store[i], symout); for (j = 0; j < snaps[i].nsym; j++) if (symout[j] != snaps[i].sym[j].sym) bad++; }
	report("B static Huffman per ctx (new schema)", enc_ns, dec_ns, bytes / nsnap);
	if (bad) printf("   !! B verification failures: %u\n", bad);

	/* C */
	{
		uint32_t *clen = (uint32_t *)calloc(nsnap, 4);
		uint32_t *crawlen = (uint32_t *)calloc(nsnap, 4);
		uint8_t **cbuf = (uint8_t **)calloc(nsnap, sizeof(uint8_t *));
		for (i = 0; i < nsnap; i++) cbuf[i] = (uint8_t *)malloc(MAXPKT);
		t0 = now_ns();
		for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) clen[i] = c_encode(&snaps[i], cbuf[i] + MAXPKT, rawbuf, &crawlen[i]);
		t1 = now_ns(); enc_ns = (t1 - t0) / (double)(REPEATS * nsnap);
		bytes = 0; for (i = 0; i < nsnap; i++) bytes += clen[i] + crawlen[i];
		t0 = now_ns();
		for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) c_decode(&snaps[i], cbuf[i] + MAXPKT - clen[i], symout);
		t1 = now_ns(); dec_ns = (t1 - t0) / (double)(REPEATS * nsnap);
		bad = 0;
		for (i = 0; i < nsnap; i++) { uint32_t j; c_decode(&snaps[i], cbuf[i] + MAXPKT - clen[i], symout); for (j = 0; j < snaps[i].nsym; j++) if (symout[j] != snaps[i].sym[j].sym) bad++; }
		report("C static rANS32 12-bit per ctx (new schema)", enc_ns, dec_ns, bytes / nsnap);
		if (bad) printf("   !! C verification failures: %u\n", bad);
	}

	/* D */
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) storelen[i] = d_encode(&snaps[i], store[i]);
	t1 = now_ns(); enc_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bytes = 0; for (i = 0; i < nsnap; i++) bytes += storelen[i] + (snaps[i].rawbits + 7) / 8;
	t0 = now_ns();
	for (r = 0; r < REPEATS; r++) for (i = 0; i < nsnap; i++) d_decode(&snaps[i], store[i], storelen[i], bitout);
	t1 = now_ns(); dec_ns = (t1 - t0) / (double)(REPEATS * nsnap);
	bad = 0;
	for (i = 0; i < nsnap; i++) { uint32_t j; d_decode(&snaps[i], store[i], storelen[i], bitout); for (j = 0; j < snaps[i].ndec; j++) if (bitout[j] != snaps[i].dec[j].bit) bad++; }
	report("D adaptive binary arith, reset/packet", enc_ns, dec_ns, bytes / nsnap);
	if (bad) printf("   !! D verification failures: %u\n", bad);

	{
		double a = 0, b = 0, c = 0;
		for (i = 0; i < nsnap; i++) { a += snaps[i].nsym; b += snaps[i].ndec; c += snaps[i].ncall; }
		printf("(actual) symbols %.0f, binary decisions %.0f, engine calls %.0f\n", a / nsnap, b / nsnap, c / nsnap);
	}
	return 0;
}
