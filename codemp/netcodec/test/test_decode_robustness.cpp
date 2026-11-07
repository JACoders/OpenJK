// Robustness of the message decoder on malformed input (run on every preset, so under ASan + UBSan too): for every
// message of synthetic streams (synthetic.h), mutated copies of its coded bytes are decoded with the client's state
// at that point (bits flipped, bytes changed, truncated, bytes inserted, deleted or appended, or random bytes of the
// same length). The decoder must return without fault. When a mutated message decodes, it is coded again with the
// client's state in the server's place and decoded again: the result must equal the first decode (the decoder
// accepts nothing the encoder cannot code). Then the real message is decoded and the stream goes on.
// The mutations come from the same integer generator: every compiler runs the same inputs.
// Usage: netcodec_test_decode_robustness [streams] [messages per stream] [mutations per message] [seed]
#include "synthetic.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace netcodec;

namespace {

struct Stats {
	size_t messages = 0, streamFailures = 0, decodes = 0, accepted = 0, reencodeRefused = 0, mismatches = 0;
};

void Mutate(synth::Rng &r, const std::vector<uint8_t> &in, std::vector<uint8_t> &out) {
	out = in;
	const auto pos = [&](size_t n) { return n == 0 ? size_t{ 0 } : r.Index(n); };
	switch (r.Index(7)) {
	case 0:   // flip 1 to 4 bits
		for (size_t k = 1 + r.Index(4); k > 0 && !out.empty(); k--) out[pos(out.size())] ^= static_cast<uint8_t>(1U << r.Index(8));
		break;
	case 1:   // change 1 to 4 bytes
		for (size_t k = 1 + r.Index(4); k > 0 && !out.empty(); k--) out[pos(out.size())] = static_cast<uint8_t>(r.Below(256));
		break;
	case 2:   // truncate
		out.resize(pos(out.size()));
		break;
	case 3: { // insert 1 to 8 bytes
		const size_t at = pos(out.size() + 1);
		for (size_t k = 1 + r.Index(8); k > 0; k--)
			out.insert(out.begin() + static_cast<std::ptrdiff_t>(at), static_cast<uint8_t>(r.Below(256)));
		break;
	}
	case 4: { // delete 1 to 8 bytes
		const size_t at = pos(out.size());
		const size_t n = std::min(out.size() - at, 1 + r.Index(8));
		out.erase(out.begin() + static_cast<std::ptrdiff_t>(at), out.begin() + static_cast<std::ptrdiff_t>(at + n));
		break;
	}
	case 5:   // append 1 to 16 bytes
		for (size_t k = 1 + r.Index(16); k > 0; k--) out.push_back(static_cast<uint8_t>(r.Below(256)));
		break;
	default:  // random bytes of the same length
		for (uint8_t &b : out) b = static_cast<uint8_t>(r.Below(256));
		break;
	}
}

void RunStream(uint64_t seed, size_t nmsg, size_t nmut, Stats &st) {
	synth::Stream s(seed);
	synth::Rng mr(seed ^ 0x5DEECE66DULL);
	std::vector<uint8_t> out, bad, again;
	Message dm, dm1, dm2;
	for (size_t k = 0; k <= nmsg; k++) {
		const synth::Candidate c = s.Next(s.Generator());
		HeaderRecord rec;
		st.messages++;
		const bool coded = s.Encode(c, out, rec);
		if (coded) {
			for (size_t m = 0; m < nmut; m++) {
				Mutate(mr, out, bad);
				uint32_t d1 = 0;
				HeaderRecord r1;
				st.decodes++;
				if (!s.DecodeAsClient(bad, dm1, d1, r1)) continue;
				st.accepted++;
				if (!s.EncodeFromClient(dm1, d1, again)) {
					if (st.reencodeRefused++ < 5) std::fprintf(stderr, "re-encoding refused: seed %" PRIu64 " seq %u\n", seed, s.Seq());
					continue;
				}
				uint32_t d2 = 0;
				HeaderRecord r2;
				const bool same = s.DecodeAsClient(again, dm2, d2, r2, 1) && d2 == d1 && trace::SameMessage(dm2, dm1) &&
				                  r2.ack == r1.ack && r2.time == r1.time && r2.cmdSeq == r1.cmdSeq;
				if (!same && st.mismatches++ < 5) std::fprintf(stderr, "decode(encode(decode(x))) differs: seed %" PRIu64 " seq %u\n", seed, s.Seq());
			}
		}
		const bool ok = s.Commit(c, coded ? std::span<const uint8_t>(out) : std::span<const uint8_t>(), rec, dm) && coded;
		if (!ok) st.streamFailures++;
	}
}

} // namespace

int main(int argc, char **argv) {
	const size_t streams = argc > 1 ? static_cast<size_t>(std::strtoull(argv[1], nullptr, 10)) : 20;
	const size_t nmsg = argc > 2 ? static_cast<size_t>(std::strtoull(argv[2], nullptr, 10)) : 500;
	const size_t nmut = argc > 3 ? static_cast<size_t>(std::strtoull(argv[3], nullptr, 10)) : 20;
	const uint64_t seed = argc > 4 ? std::strtoull(argv[4], nullptr, 10) : 1;
	Stats st;
	for (size_t i = 0; i < streams; i++) RunStream((seed * 7000003) + i, nmsg, nmut, st);
	std::printf("decoder robustness: %zu messages, %zu malformed inputs decoded without fault, %zu accepted; re-encoding "
	            "refused %zu, decode(encode(decode(x))) different %zu; stream failures %zu\n",
	            st.messages, st.decodes, st.accepted, st.reencodeRefused, st.mismatches, st.streamFailures);
	return st.decodes != 0 && st.reencodeRefused == 0 && st.mismatches == 0 && st.streamFailures == 0 ? 0 : 1;
}
