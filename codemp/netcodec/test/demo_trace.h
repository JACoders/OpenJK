// Reader of the corpus traces written by docs_rewrite/netcode/tools/python/export_demo_traces.py (train.bin,
// holdout.bin, captures.bin): decoded messages without coded bytes, read one stream (demo) at a time, and the server
// loop that chooses the base of every message as the server did.
#pragma once

#include "codec_trace.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace netcodec::trace {

struct DemoMessage {
	uint32_t seq = 0;
	uint32_t delta = 0;
	Message msg;
	uint32_t engineBytes = 0;   // size of the message the server sent, without svc_EOF
};

struct DemoStream {
	std::vector<std::string> meta;   // set, path, game type, map, ... (see export_demo_traces.py); "capture", name
	std::vector<DemoMessage> messages;

	[[nodiscard]] const std::string &Meta(size_t i) const {
		static const std::string none;
		return i < meta.size() ? meta[i] : none;
	}
};

class DemoTraceFile {
public:
	explicit DemoTraceFile(const char *path) : file_(std::fopen(path, "rb"), &std::fclose), r_(file_.get()) {
		char magic[4];
		uint32_t version = 0;
		ok_ = file_ && std::fread(magic, 1, 4, file_.get()) == 4 && std::memcmp(magic, "NCDT", 4) == 0 && r_.U32(version) &&
		      version == 1 && r_.U32(streams_);
		if (!ok_) std::fprintf(stderr, "cannot read %s\n", path);
	}

	[[nodiscard]] bool Ok() const noexcept { return ok_; }
	[[nodiscard]] uint32_t Streams() const noexcept { return streams_; }

	// reads the next stream into s (its storage is reused); false at the end or on a malformed file
	bool Next(DemoStream &s) {
		if (!ok_ || read_ == streams_) return false;
		std::string meta;
		uint32_t nm = 0;
		ok_ = r_.Bytes(meta) && r_.U32(nm);
		s.meta.clear();
		for (size_t start = 0; ok_;) {
			const size_t tab = meta.find('\t', start);
			s.meta.push_back(meta.substr(start, tab - start));
			if (tab == std::string::npos) break;
			start = tab + 1;
		}
		s.messages.resize(ok_ ? nm : 0);
		for (DemoMessage &m : s.messages) {
			uint32_t nops = 0;
			ok_ = ok_ && r_.U32(m.seq) && r_.U32(m.delta) && r_.I64(m.msg.ack) && r_.U32(nops) && nops < 4096;
			if (!ok_) break;
			m.msg.ops.resize(nops);
			for (Op &op : m.msg.ops) {
				op = Op{};
				ok_ = ok_ && r_.ReadOp(op);
			}
			ok_ = ok_ && r_.U32(m.engineBytes);
		}
		if (!ok_) std::fprintf(stderr, "malformed trace file\n");
		read_++;
		return ok_;
	}

private:
	std::unique_ptr<FILE, int (*)(FILE *)> file_;
	Reader r_;
	uint32_t streams_ = 0;
	uint32_t read_ = 0;
	bool ok_ = false;
};

[[nodiscard]] inline const Snapshot *SnapshotOf(const Message &m) noexcept {
	for (const Op &op : m.ops)
		if (op.type == OpType::Snapshot) return &op.snap;
	return nullptr;
}

// The server side of a stream: for every message whose base the server still holds (a message whose base is
// missing is skipped, as the server would not have sent it against that base), calls
// code(message, base, baselines, record), which codes it and fills its header record. The stored snapshots point
// into the stream, so the stream must outlive the loop. Returns the number of messages code() refused.
template <class F> size_t ServerLoop(const DemoStream &s, F code) {
	struct Stored {
		const Snapshot *snap = nullptr;
		HeaderRecord header;
	};
	std::map<uint32_t, Stored> store;
	std::vector<Entity> baselines;
	size_t refused = 0;
	for (const DemoMessage &m : s.messages) {
		Base base;
		base.delta = m.delta;
		if (m.delta != 0) {
			const auto it = store.find(m.seq - m.delta);
			if (it == store.end()) continue;
			base.snap = it->second.snap;
			base.header = &it->second.header;
		}
		HeaderRecord rec;
		if (!code(m, base, std::as_const(baselines), rec)) refused++;
		for (const Op &op : m.msg.ops)
			if (op.type == OpType::GameState) baselines = op.gameState.baselines;
		if (const Snapshot *sn = SnapshotOf(m.msg)) store[m.seq] = Stored{ .snap = sn, .header = rec };
		if (m.seq > 255) store.erase(store.begin(), store.lower_bound(m.seq - 255));   // a delta is at most 255
	}
	return refused;
}

} // namespace netcodec::trace
