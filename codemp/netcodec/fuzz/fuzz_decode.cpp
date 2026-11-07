// libFuzzer target on the message decoder (preset mingw-clang-fuzz: clang, ASan + UBSan, every netcodec target
// instrumented for coverage). Inputs are decoded with a fixed client state: a synthetic stream (test/synthetic.h,
// seed 1) after 600 messages (the client then holds 2 x 255 decoded snapshots, commands and baselines). Each input
// must decode without fault; when it decodes, it is coded again with the client's state in the server's place and
// decoded again, and the result must equal the first decode (the decoder accepts nothing the encoder cannot code).
// Seeds: netcodec_fuzz_decode --write-seeds=<dir> writes 256 valid messages coded against that state, then exits.
// Run: netcodec_fuzz_decode -max_len=65536 -timeout=10 -rss_limit_mb=2048 -max_total_time=<s> <corpus dir>
#include "synthetic.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <vector>

using namespace netcodec;

namespace {

constexpr size_t kWarmup = 600;
synth::Stream *g_stream = nullptr;

void WriteSeeds(const char *dir) {
	size_t written = 0;
	for (uint64_t k = 0; k < 256; k++) {
		synth::Rng r(0xF00D0000ULL + k);
		const synth::Candidate c = g_stream->Next(r);
		std::vector<uint8_t> out;
		HeaderRecord rec;
		if (!g_stream->Encode(c, out, rec)) continue;
		const std::string path = std::string(dir) + "/seed_" + std::to_string(k);
		FILE *f = std::fopen(path.c_str(), "wb");
		if (f == nullptr || std::fwrite(out.data(), 1, out.size(), f) != out.size()) {
			std::fprintf(stderr, "cannot write %s\n", path.c_str());
			std::exit(1);
		}
		std::fclose(f);
		written++;
	}
	std::printf("%zu seeds written to %s\n", written, dir);
}

} // namespace

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) {
	static synth::Stream stream(1);
	g_stream = &stream;
	std::vector<uint8_t> out;
	Message dm;
	for (size_t k = 0; k < kWarmup; k++) {
		const synth::Candidate c = stream.Next(stream.Generator());
		HeaderRecord rec;
		const bool coded = stream.Encode(c, out, rec);
		if (!stream.Commit(c, coded ? std::span<const uint8_t>(out) : std::span<const uint8_t>(), rec, dm)) std::abort();
	}
	for (int i = 1; i < *argc; i++)
		if (std::strncmp((*argv)[i], "--write-seeds=", 14) == 0) {
			WriteSeeds((*argv)[i] + 14);
			std::exit(0);
		}
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	static Message dm1, dm2;
	static std::vector<uint8_t> again;
	uint32_t d1 = 0, d2 = 0;
	HeaderRecord r1, r2;
	if (!g_stream->DecodeAsClient(std::span<const uint8_t>(data, size), dm1, d1, r1)) return 0;
	if (!g_stream->EncodeFromClient(dm1, d1, again)) {
		std::fprintf(stderr, "the encoder refuses a decoded message\n");
		std::abort();
	}
	const bool same = g_stream->DecodeAsClient(again, dm2, d2, r2) && d2 == d1 && trace::SameMessage(dm2, dm1) &&
	                  r2.ack == r1.ack && r2.time == r1.time && r2.cmdSeq == r1.cmdSeq;
	if (!same) {
		std::fprintf(stderr, "decode(encode(decode(x))) differs from decode(x)\n");
		std::abort();
	}
	return 0;
}
