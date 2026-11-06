// Reader of the message-codec golden trace written by docs_rewrite/netcode/tools/python/export_codec_trace.py.
#pragma once

#include "netcodec/codec.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

namespace netcodec {
namespace trace {

struct CodecMessage {
	uint32_t seq = 0;
	uint32_t delta = 0;
	Message msg;
	std::vector<uint8_t> bytes;    // the Python codec's output
};

using CodecStream = std::vector<CodecMessage>;

class Reader {
public:
	explicit Reader(FILE *f) noexcept : f_(f) {}

	bool U8(uint8_t &v) { return fread(&v, 1, 1, f_) == 1; }
	bool U16(uint16_t &v) {
		uint8_t b[2];
		if (fread(b, 1, 2, f_) != 2) return false;
		v = static_cast<uint16_t>(b[0] | (b[1] << 8));
		return true;
	}
	bool U32(uint32_t &v) {
		uint8_t b[4];
		if (fread(b, 1, 4, f_) != 4) return false;
		v = 0;
		for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(b[i]) << (8 * i);
		return true;
	}
	bool I64(int64_t &v) {
		uint32_t lo = 0, hi = 0;
		if (!U32(lo) || !U32(hi)) return false;
		v = static_cast<int64_t>((static_cast<uint64_t>(hi) << 32) | lo);
		return true;
	}
	template <class T> bool Bytes(T &out) {
		uint32_t n = 0;
		if (!U32(n) || n > (1u << 24)) return false;
		out.resize(n);
		return n == 0 || fread(out.data(), 1, n, f_) == n;
	}
	template <size_t N> bool Image(std::array<uint8_t, N> &img, std::span<const uint16_t> offsets) {
		uint16_t n = 0;
		if (!U16(n)) return false;
		for (uint16_t k = 0; k < n; k++) {
			uint16_t idx = 0;
			uint32_t v = 0;
			if (!U16(idx) || !U32(v) || idx >= offsets.size()) return false;
			std::memcpy(img.data() + offsets[idx], &v, 4);
		}
		return true;
	}
	bool Entities(std::vector<Entity> &out) {
		uint32_t n = 0;
		if (!U32(n) || n > kMaxEntities) return false;
		out.resize(n);
		for (Entity &e : out)
			if (!U16(e.number) || !Image(e.state, EntityFieldOffsets())) return false;
		return true;
	}
	bool ReadOp(Op &op) {
		uint8_t t = 0;
		if (!U8(t) || t < 1 || t > 7) return false;
		op.type = static_cast<OpType>(t);
		switch (op.type) {
		case OpType::Command: return I64(op.seq) && Bytes(op.text);
		case OpType::SetGame: return Bytes(op.text);
		case OpType::GameState: {
			GameState &gs = op.gameState;
			uint32_t n = 0;
			if (!I64(gs.relSeq) || !U32(n) || n > 0xFFFF) return false;
			gs.configStrings.resize(n);
			for (ConfigString &cs : gs.configStrings)
				if (!U16(cs.index) || !Bytes(cs.text)) return false;
			return Entities(gs.baselines) && I64(gs.clientNum) && I64(gs.checksum);
		}
		case OpType::Download: return U32(op.block) && I64(op.fileSize) && Bytes(op.data);
		case OpType::Snapshot: {
			uint32_t na = 0;
			uint8_t hv = 0;
			if (!I64(op.time) || !U32(op.flags) || !U32(na) || na > 255) return false;
			op.areas.resize(na);
			if (na != 0 && fread(op.areas.data(), 1, na, f_) != na) return false;
			if (!Image(op.snap.ps, PlayerFieldOffsets()) || !U8(hv)) return false;
			op.snap.hasVehicle = hv != 0;
			if (op.snap.hasVehicle && !Image(op.snap.vps, PlayerFieldOffsets())) return false;
			return Entities(op.snap.entities);
		}
		default: return true;
		}
	}

private:
	FILE *f_;
};

// the snapshot operation of a message, if any
[[nodiscard]] inline const Snapshot *SnapshotOf(const Message &m) noexcept {
	for (const Op &op : m.ops)
		if (op.type == OpType::Snapshot) return &op.snap;
	return nullptr;
}

inline bool LoadCodecTrace(const char *path, std::vector<CodecStream> &streams) {
	const std::unique_ptr<FILE, int (*)(FILE *)> file(fopen(path, "rb"), &fclose);
	if (!file) { fprintf(stderr, "cannot open %s\n", path); return false; }
	Reader r(file.get());
	char magic[4];
	uint32_t version = 0, ns = 0;
	bool ok = fread(magic, 1, 4, file.get()) == 4 && std::memcmp(magic, "NCCT", 4) == 0 && r.U32(version) && version == 1 &&
	          r.U32(ns);
	streams.resize(ok ? ns : 0);
	for (CodecStream &s : streams) {
		uint32_t nm = 0;
		ok = ok && r.U32(nm);
		s.resize(ok ? nm : 0);
		for (CodecMessage &m : s) {
			uint32_t nops = 0;
			ok = ok && r.U32(m.seq) && r.U32(m.delta) && r.I64(m.msg.ack) && r.U32(nops) && nops < 4096;
			if (!ok) break;
			m.msg.ops.resize(nops);
			for (Op &op : m.msg.ops) ok = ok && r.ReadOp(op);
			ok = ok && r.Bytes(m.bytes);
		}
	}
	if (!ok) fprintf(stderr, "bad trace file %s\n", path);
	return ok;
}

} // namespace trace
} // namespace netcodec
