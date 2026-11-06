// Message-codec golden test: every message of the trace must encode to the bytes of the Python reference codec
// (same tables, same base chain as the server loop of run_full.py), and decode back to the same message.
// Usage: netcodec_test_codec <codec_trace.bin>
#include "codec_trace.h"
#include "message_compare.h"

#include <cstdio>
#include <map>

using namespace netcodec;

namespace {

struct Stored {
	Snapshot snap;
	HeaderRecord header;
};

class Store final : public BaseLookup {
public:
	std::map<uint32_t, Stored> bySeq;
	uint32_t seq = 0;              // sequence of the message being decoded

	bool Find(uint32_t delta, const Snapshot *&snap, const HeaderRecord *&header) const override {
		const auto it = bySeq.find(seq - delta);
		if (it == bySeq.end()) return false;
		snap = &it->second.snap; header = &it->second.header;
		return true;
	}
};

} // namespace

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s codec_trace.bin\n", argv[0]); return 2; }
	std::vector<trace::CodecStream> streams;
	if (!trace::LoadCodecTrace(argv[1], streams)) return 2;

	MessageEncoder enc;
	std::vector<uint8_t> out;
	size_t total = 0, encBad = 0, decBad = 0, bytes = 0;
	for (size_t si = 0; si < streams.size(); si++) {
		Store server, client;
		std::vector<Entity> baselinesServer, baselinesClient;
		for (const trace::CodecMessage &cm : streams[si]) {
			total++;
			Base base;
			base.delta = cm.delta;
			if (cm.delta != 0) {
				const auto it = server.bySeq.find(cm.seq - cm.delta);
				if (it == server.bySeq.end()) { encBad++; continue; }
				base.snap = &it->second.snap; base.header = &it->second.header;
			}
			HeaderRecord rec;
			const bool ok = enc.Encode(cm.msg, base, baselinesServer, 1 << 20, out, rec);
			if (!ok || out != cm.bytes) {
				if (encBad++ < 5)
					fprintf(stderr, "encode mismatch: stream %zu seq %u: %s, %zu bytes, expected %zu\n", si, cm.seq,
					        ok ? "different bytes" : "encoder failed", out.size(), cm.bytes.size());
			}
			for (const Op &op : cm.msg.ops)
				if (op.type == OpType::GameState) baselinesServer = op.gameState.baselines;
			if (const Snapshot *sn = trace::SnapshotOf(cm.msg)) server.bySeq[cm.seq] = Stored{ *sn, rec };

			Message dm;
			uint32_t delta = 0;
			HeaderRecord drec;
			client.seq = cm.seq;
			const bool dok = DecodeMessage(cm.bytes, client, baselinesClient, dm, delta, drec);
			if (!dok || delta != cm.delta || !trace::SameMessage(dm, cm.msg)) {
				if (decBad++ < 5) fprintf(stderr, "decode mismatch: stream %zu seq %u%s\n", si, cm.seq, dok ? "" : " (decoder failed)");
			}
			if (dok)
				if (const Snapshot *sn = trace::SnapshotOf(dm)) client.bySeq[cm.seq] = Stored{ *sn, drec };
			bytes += cm.bytes.size();
		}
	}
	printf("golden: %zu streams, %zu messages, %zu bytes: encode mismatches %zu, decode mismatches %zu\n", streams.size(),
	       total, bytes, encBad, decBad);
	return encBad || decBad ? 1 : 0;
}
