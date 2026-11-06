// Reader of the rANS golden trace written by docs_rewrite/netcode/tools/python/export_rans_trace.py.
#pragma once

#include "netcodec/rans.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace netcodec {
namespace trace {

constexpr uint32_t kRaw = 0xFFFFFFFFu;

struct Unit {
	uint32_t table;     // kRaw: raw bits
	uint32_t value;
	uint32_t bits;      // raw only
};

struct Message {
	std::vector<Unit> units;
	std::vector<uint8_t> bytes;
};

struct RansTrace {
	std::vector<std::vector<uint16_t>> cum;
	std::vector<std::vector<rans::EncSymbol>> enc;
	std::vector<std::vector<uint16_t>> coarse;
	std::vector<rans::Table> tables;
	std::vector<Message> messages;
};

inline bool ReadU32(FILE *f, uint32_t &v) {
	uint8_t b[4];
	if (fread(b, 1, 4, f) != 4) return false;
	v = 0;
	for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(b[i]) << (8 * i);
	return true;
}

inline bool LoadRansTrace(const char *path, RansTrace &t) {
	const std::unique_ptr<FILE, int (*)(FILE *)> file(fopen(path, "rb"), &fclose);
	FILE *const f = file.get();
	if (!f) { fprintf(stderr, "cannot open %s\n", path); return false; }
	char magic[4];
	uint32_t version = 0, probBits = 0, ntab = 0, nmsg = 0;
	bool ok = fread(magic, 1, 4, f) == 4 && memcmp(magic, "NCRT", 4) == 0 && ReadU32(f, version) && version == 1 &&
	          ReadU32(f, probBits) && probBits == rans::kProbBits && ReadU32(f, ntab);
	for (uint32_t i = 0; ok && i < ntab; i++) {
		uint32_t K = 0;
		ok = ReadU32(f, K) && K >= 1 && K <= 65536;
		if (!ok) break;
		std::vector<uint16_t> freq(K), cum(K + 1);
		ok = fread(freq.data(), 2, K, f) == K;    // little-endian host assumed (x86, x64)
		cum[0] = 0;
		for (uint32_t s = 0; ok && s < K; s++) {
			ok = freq[s] >= 1 && cum[s] + freq[s] <= rans::kProbScale;
			cum[s + 1] = static_cast<uint16_t>(cum[s] + freq[s]);
		}
		ok = ok && cum[K] == rans::kProbScale;
		std::vector<rans::EncSymbol> enc(K);
		for (uint32_t s = 0; ok && s < K; s++) enc[s] = rans::MakeEncSymbol(cum[s], freq[s]);
		std::vector<uint16_t> coarse(rans::kCoarseSize);
		if (ok) rans::BuildCoarse(cum, std::span<uint16_t, rans::kCoarseSize>(coarse.data(), rans::kCoarseSize));
		t.coarse.push_back(std::move(coarse));
		t.cum.push_back(std::move(cum));
		t.enc.push_back(std::move(enc));
	}
	for (size_t i = 0; ok && i < t.cum.size(); i++)
		t.tables.push_back(rans::Table{ t.cum[i], t.enc[i], std::span<const uint16_t, rans::kCoarseSize>(t.coarse[i].data(), rans::kCoarseSize) });
	ok = ok && ReadU32(f, nmsg);
	t.messages.resize(ok ? nmsg : 0);
	for (uint32_t m = 0; ok && m < nmsg; m++) {
		Message &msg = t.messages[m];
		uint32_t nu = 0, nb = 0;
		ok = ReadU32(f, nu);
		msg.units.resize(ok ? nu : 0);
		for (uint32_t u = 0; ok && u < nu; u++) {
			Unit &x = msg.units[u];
			ok = ReadU32(f, x.table) && ReadU32(f, x.value) && ReadU32(f, x.bits) &&
			     (x.table == kRaw ? x.bits <= 32 : (x.table < ntab && x.value < t.tables[x.table].K()));
		}
		ok = ok && ReadU32(f, nb);
		if (ok) { msg.bytes.resize(nb); ok = fread(msg.bytes.data(), 1, nb, f) == nb; }
	}
	if (!ok) fprintf(stderr, "bad trace file %s\n", path);
	return ok;
}

} // namespace trace
} // namespace netcodec
