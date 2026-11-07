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
#include <unordered_map>
#include <utility>
#include <vector>

namespace netcodec::trace {

struct DemoMessage {
	uint32_t seq = 0;
	uint32_t delta = 0;
	Message msg;
	MessageData data;              // the storage of msg's snapshot and baselines
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
			m.data = MessageData{};
			for (Op &op : m.msg.ops) {
				op = Op{};
				ok_ = ok_ && r_.ReadOp(op, m.data);
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

// The server commands one side holds, by sequence number (tests and tools keep all of them).
class CommandLog final : public CommandHistory {
public:
	void Add(const Message &m) {
		for (const Op &op : m.ops)
			if (op.type == OpType::Command) texts_[op.seq] = op.text;
	}

	[[nodiscard]] const std::string *Find(int64_t seq) const override {
		const auto it = texts_.find(seq);
		return it != texts_.end() ? &it->second : nullptr;
	}

private:
	std::unordered_map<int64_t, std::string> texts_;
};

// The snapshots a client holds, by message sequence, with their header records. A record's `older` points to the
// snapshot its message was coded against, at most 255 messages earlier: keep 2 * 255 messages so that the older
// snapshot of every possible base is still held.
class ClientStore final : public BaseLookup {
public:
	struct Stored {
		SnapshotData data;
		Snapshot snap;                 // a view of data
		HeaderRecord header;
	};
	std::map<uint32_t, Stored> bySeq;
	uint32_t seq = 0;              // sequence of the message being decoded

	bool Find(uint32_t delta, const Snapshot *&snap, const HeaderRecord *&header) const override {
		const auto it = bySeq.find(seq - delta);
		if (it == bySeq.end()) return false;
		snap = &it->second.snap;
		header = &it->second.header;
		return true;
	}

	// keeps a decoded snapshot (a view of the decoder's output) for message seq
	void Keep(uint32_t s, const Snapshot &snap, const HeaderRecord &header) {
		Stored &st = bySeq[s];
		st.data.Assign(snap);
		st.snap = st.data.View();
		st.header = header;
	}

	// drops what no later message can use, after message last
	void Trim(uint32_t last) {
		if (last > 2 * 255) bySeq.erase(bySeq.begin(), bySeq.lower_bound(last - (2 * 255)));
	}
};

// The server side of a stream: for every message whose base the server still holds (a message whose base is
// missing is skipped, as the server would not have sent it against that base), calls
// code(message, base, baselines, history, record), which codes it and fills its header record; history holds the
// commands of the earlier messages. The stored snapshots point into the stream, so the stream must outlive the
// loop. Returns the number of messages code() refused.
template <class F> size_t ServerLoop(const DemoStream &s, F code) {
	struct Stored {
		const Snapshot *snap = nullptr;
		HeaderRecord header;
	};
	std::map<uint32_t, Stored> store;
	BaselineTable baselines;               // a view of the stream's last gamestate
	CommandLog history;
	size_t refused = 0;
	for (const DemoMessage &m : s.messages) {
		Base base;
		base.delta = m.delta;
		const auto it = m.delta != 0 ? store.find(m.seq - m.delta) : store.end();
		if (m.delta != 0 && it == store.end()) {
			history.Add(m.msg);
			continue;
		}
		if (m.delta != 0) {
			base.snap = it->second.snap;
			base.header = &it->second.header;
		}
		HeaderRecord rec;
		if (!code(m, base, std::as_const(baselines), std::as_const(history), rec)) refused++;
		history.Add(m.msg);
		for (const Op &op : m.msg.ops)
			if (op.type == OpType::GameState) baselines = op.gameState.baselines;
		if (const Snapshot *sn = SnapshotOf(m.msg)) store[m.seq] = Stored{ .snap = sn, .header = rec };
		if (m.seq > 255) store.erase(store.begin(), store.lower_bound(m.seq - 255));   // a delta is at most 255
	}
	return refused;
}

} // namespace netcodec::trace
