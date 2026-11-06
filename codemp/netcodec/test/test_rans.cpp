// rANS golden test: every message of the trace must encode to the bytes of the Python reference codec, and decode
// back to the same units. Also: truncated and random inputs never read out of bounds (run under ASan / UBSan).
// Usage: netcodec_test_rans <rans_trace.bin>
#include "rans_trace.h"

#include <cstdio>
#include <random>
#include <vector>

using namespace netcodec;

static bool DecodeUnits(const std::vector<trace::Unit> &units, const std::vector<rans::Table> &tables,
                        std::span<const uint8_t> data, bool check) {
	rans::Decoder d;
	if (!d.Init(data)) return false;
	for (const trace::Unit &u : units) {
		const uint32_t v = u.table == trace::kRaw ? d.GetRaw(u.bits) : rans::Get(d, tables[u.table]);
		if (check && v != u.value) return false;
	}
	return !check || d.Remaining() == 0;
}

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s rans_trace.bin\n", argv[0]); return 2; }
	trace::RansTrace t;
	if (!trace::LoadRansTrace(argv[1], t)) return 2;

	std::vector<rans::Encoder::Op> ops;
	std::vector<uint8_t> out;
	size_t encBad = 0, decBad = 0, units = 0, bytes = 0;
	for (const trace::Message &m : t.messages) {
		ops.resize(m.units.size() * 3 + 1);                   // a raw unit of 32 bits takes 3 chunks
		rans::Encoder e(ops);
		for (const trace::Unit &u : m.units) {
			if (u.table == trace::kRaw) e.PutRaw(u.value, u.bits);
			else rans::Put(e, t.tables[u.table], u.value);
		}
		out.assign(m.bytes.size() + 64, 0);
		const size_t n = e.Finish(out);
		if (n != m.bytes.size() || memcmp(out.data(), m.bytes.data(), n) != 0) {
			if (encBad++ < 3) fprintf(stderr, "encode mismatch: message %zu, %zu bytes, expected %zu\n",
			                          static_cast<size_t>(&m - t.messages.data()), n, m.bytes.size());
		}
		if (!DecodeUnits(m.units, t.tables, m.bytes, true)) {
			if (decBad++ < 3) fprintf(stderr, "decode mismatch: message %zu\n", static_cast<size_t>(&m - t.messages.data()));
		}
		units += m.units.size(); bytes += m.bytes.size();
	}
	printf("golden: %zu messages, %zu units, %zu bytes: encode mismatches %zu, decode mismatches %zu\n",
	       t.messages.size(), units, bytes, encBad, decBad);

	// Truncated, mutated and random inputs: the decoder must stay in bounds; results are not checked.
	std::mt19937 rng(1);
	std::vector<uint8_t> buf;
	size_t runs = 0;
	for (size_t i = 0; i < t.messages.size(); i += 7) {
		const trace::Message &m = t.messages[i];
		for (int k = 0; k < 4; k++) {
			buf = m.bytes;
			if (k == 0) buf.resize(rng() % (buf.size() + 1));
			else if (k == 1) { for (int j = 0; j < 3 && !buf.empty(); j++) buf[rng() % buf.size()] ^= static_cast<uint8_t>(1u << (rng() % 8)); }
			else if (k == 2) { for (uint8_t &b : buf) b = static_cast<uint8_t>(rng()); }
			else buf.resize(rng() % 8);
			DecodeUnits(m.units, t.tables, buf, false);
			runs++;
		}
	}
	printf("robustness: %zu truncated / mutated / random inputs decoded without fault\n", runs);

	// Small output buffer: Finish must refuse, not overflow.
	size_t refused = 0;
	for (size_t i = 0; i < t.messages.size(); i += 97) {
		const trace::Message &m = t.messages[i];
		ops.resize(m.units.size() * 3 + 1);
		rans::Encoder e(ops);
		for (const trace::Unit &u : m.units) {
			if (u.table == trace::kRaw) e.PutRaw(u.value, u.bits);
			else rans::Put(e, t.tables[u.table], u.value);
		}
		out.assign(m.bytes.size() - 1, 0);
		refused += e.Finish(out) == 0;
	}
	printf("small buffer: %zu of %zu encodes refused\n", refused, (t.messages.size() + 96) / 97);
	return encBad || decBad || refused != (t.messages.size() + 96) / 97 ? 1 : 0;
}
