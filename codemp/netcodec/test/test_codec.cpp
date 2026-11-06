// Message-codec golden test: every message of the trace must encode to the bytes of the Python reference codec
// (same tables, same base chain as the server loop of run_full.py), and decode back to the same message.
// Usage: netcodec_test_codec <codec_trace.bin>
#include "codec_trace.h"

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

const Snapshot *SnapshotOf(const Message &m) {
	for (const Op &op : m.ops)
		if (op.type == OpType::Snapshot) return &op.snap;
	return nullptr;
}

bool SameEntities(const std::vector<Entity> &a, const std::vector<Entity> &b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); i++)
		if (a[i].number != b[i].number || a[i].state != b[i].state) return false;
	return true;
}

bool SameOp(const Op &a, const Op &b) {
	if (a.type != b.type) return false;
	switch (a.type) {
	case OpType::Command: return a.seq == b.seq && a.text == b.text;
	case OpType::SetGame: return a.text == b.text;
	case OpType::GameState: {
		const GameState &x = a.gameState, &y = b.gameState;
		if (x.relSeq != y.relSeq || x.clientNum != y.clientNum || static_cast<uint32_t>(x.checksum) != static_cast<uint32_t>(y.checksum) ||
		    x.configStrings.size() != y.configStrings.size() || !SameEntities(x.baselines, y.baselines))
			return false;
		for (size_t i = 0; i < x.configStrings.size(); i++)
			if (x.configStrings[i].index != y.configStrings[i].index || x.configStrings[i].text != y.configStrings[i].text) return false;
		return true;
	}
	case OpType::Download: return a.block == b.block && a.fileSize == b.fileSize && a.data == b.data;
	case OpType::Snapshot:
		return a.time == b.time && a.flags == b.flags && a.areas == b.areas && a.snap.ps == b.snap.ps &&
		       a.snap.hasVehicle == b.snap.hasVehicle && (!a.snap.hasVehicle || a.snap.vps == b.snap.vps) &&
		       SameEntities(a.snap.entities, b.snap.entities);
	default: return true;
	}
}

bool SameMessage(const Message &a, const Message &b) {
	if (a.ack != b.ack || a.ops.size() != b.ops.size()) return false;
	for (size_t i = 0; i < a.ops.size(); i++)
		if (!SameOp(a.ops[i], b.ops[i])) return false;
	return true;
}

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
			if (const Snapshot *sn = SnapshotOf(cm.msg)) server.bySeq[cm.seq] = Stored{ *sn, rec };

			Message dm;
			uint32_t delta = 0;
			HeaderRecord drec;
			client.seq = cm.seq;
			const bool dok = DecodeMessage(cm.bytes, client, baselinesClient, dm, delta, drec);
			if (!dok || delta != cm.delta || !SameMessage(dm, cm.msg)) {
				if (decBad++ < 5) fprintf(stderr, "decode mismatch: stream %zu seq %u%s\n", si, cm.seq, dok ? "" : " (decoder failed)");
			}
			if (dok)
				if (const Snapshot *sn = SnapshotOf(dm)) client.bySeq[cm.seq] = Stored{ *sn, drec };
			bytes += cm.bytes.size();
		}
	}
	printf("golden: %zu streams, %zu messages, %zu bytes: encode mismatches %zu, decode mismatches %zu\n", streams.size(),
	       total, bytes, encBad, decBad);
	return encBad || decBad ? 1 : 0;
}
