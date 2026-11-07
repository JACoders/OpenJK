// Message codec benchmark: netcodec against the legacy coding of OpenJK (its own msg.cpp and huffman.cpp, driven as
// the engine drives them, bench/legacy/). Same messages, same base chain (the deltas of the recordings), same
// baselines. Measured: bytes, and the best time of several passes over all messages, encoding and decoding.
// Messages with operations the legacy glue does not drive (gamestate, download, ...) only update the stores.
//
// Two timings. Cold: whole passes over the trace; every state comes from main memory, so about half of this time is
// memory latency, and it varies by 5 to 10 % between runs. Warm: each message coded 5 times in a row and the fastest
// kept (time-stamp counter, converted to ns with the pass's own duration), summed over the messages; the states are in
// cache as in the engine, which has just written them, and two runs agree within a fraction of a percent. Compare
// two versions with the warm timing, alternately (step-06-cpp-codec.md, part C).
//
// Memory: the stores point into the loaded trace instead of copying snapshots. Every message is first coded and
// decoded once by both codecs and checked (decoded fields equal to the recording), so the recorded snapshots are
// what each side holds as bases. The trace's own coded bytes (an earlier format) are not used.
// Usage: netcodec_bench_codec <codec_trace.bin> [passes]
#include "demo_trace.h"
#include "legacy/legacy_codec.h"
#include "message_compare.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

using namespace netcodec;

namespace {

double NowNs() {
	return static_cast<double>(
	    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

// time-stamp counter as uint64_t (__rdtsc returns unsigned long long, which is not uint64_t on Linux)
uint64_t Ticks() noexcept { return __rdtsc(); }

template <class F> double Best(int passes, F f) {
	double best = 1e300;
	for (int r = 0; r < passes; r++) {
		const double t0 = NowNs();
		f();
		best = std::min(best, NowNs() - t0);
	}
	return best;
}

bool Benchable(const Message &m) {
	return std::all_of(m.ops.begin(), m.ops.end(), [](const Op &op) { return op.type == OpType::Command || op.type == OpType::Snapshot; });
}

const Op *SnapshotOp(const Message &m) {
	for (const Op &op : m.ops)
		if (op.type == OpType::Snapshot) return &op;
	return nullptr;
}

bool SameFields(const uint8_t *a, const uint8_t *b, std::span<const uint16_t> offsets) {
	return std::all_of(offsets.begin(), offsets.end(), [&](uint16_t off) { return std::memcmp(a + off, b + off, 4) == 0; });
}

struct Stored {
	const Snapshot *snap = nullptr;
	HeaderRecord header;
};

class Store final : public BaseLookup {
public:
	std::map<uint32_t, Stored> bySeq;
	uint32_t seq = 0;
	bool Find(uint32_t delta, const Snapshot *&snap, const HeaderRecord *&header) const override {
		const auto it = bySeq.find(seq - delta);
		if (it == bySeq.end()) return false;
		snap = it->second.snap; header = &it->second.header;
		return true;
	}
};

// Legacy view of one message: pointers into the trace.
struct LegacyInput {
	std::vector<const uint8_t *> entityPtrs;
	std::vector<legacy::Command> commands;
	legacy::MessageView view;
};

struct Item {
	const trace::CodecMessage *cm = nullptr;
	Base base;                                          // netcodec, server side
	std::vector<uint8_t> bytes;                         // netcodec
	BaselineTable baselines;                            // not changed by the benchmarked messages (no gamestate)
	const LegacyInput *legacyIn = nullptr;
	const legacy::SnapshotView *legacyBase = nullptr;
	const uint8_t *const *legacyBaselines = nullptr;
	std::vector<uint8_t> legacyBytes;
};

struct StreamRun {
	std::vector<Item> items;                            // benchmarked messages
	Store server, client;
	trace::CommandLog serverHistory, clientHistory;
	std::vector<std::unique_ptr<trace::BaselineData>> baselineSets;
	std::vector<std::unique_ptr<std::vector<EntityState>>> legacyBaselineImages;
	std::vector<std::unique_ptr<std::vector<const uint8_t *>>> legacyBaselinePtrs;
	std::vector<std::unique_ptr<LegacyInput>> legacyInputs;
	std::map<uint32_t, const LegacyInput *> legacyBySeq;
};

void FillLegacyInput(const Message &m, int delta, LegacyInput &in) {
	in.view.ack = static_cast<int32_t>(m.ack);
	for (const Op &op : m.ops) {
		if (op.type == OpType::Command) in.commands.push_back(legacy::Command{ static_cast<int32_t>(op.seq), op.text.c_str() });
		if (op.type != OpType::Snapshot) continue;
		for (uint32_t i = 0; i < op.snap.entities.count; i++) in.entityPtrs.push_back(op.snap.entities[i].data());
		in.view.hasSnapshot = true;
		in.view.serverTime = static_cast<int32_t>(op.time);
		in.view.delta = delta;
		in.view.flags = static_cast<int>(op.flags);
		in.view.areas = op.areas.data();
		in.view.numAreas = static_cast<int>(op.areas.size());
		in.view.snap = legacy::SnapshotView{ op.snap.ps.data(), op.snap.vps.empty() ? nullptr : op.snap.vps.data(),
		                                     in.entityPtrs.data(), static_cast<int>(in.entityPtrs.size()) };
	}
	in.view.commands = in.commands.data();
	in.view.numCommands = static_cast<int>(in.commands.size());
}

bool LegacyMatches(const legacy::SnapshotOut &o, const Snapshot &sn) {
	if (!SameFields(o.ps, sn.ps.data(), PlayerFieldOffsets()) || o.numEntities != static_cast<int>(sn.entities.count) ||
	    (!sn.vps.empty() && !SameFields(o.vps, sn.vps.data(), PlayerFieldOffsets())))
		return false;
	for (uint32_t e = 0; e < sn.entities.count; e++) {
		const uint8_t *img = o.entities + (e * legacy::kEntityBytes);
		int32_t num = 0;
		std::memcpy(&num, img, 4);
		if (std::cmp_not_equal(num, EntityNumber(sn.entities[e])) || !SameFields(img, sn.entities[e].data(), EntityFieldOffsets()))
			return false;
	}
	return true;
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s codec_trace.bin [passes]\n", argv[0]); return 2; }
	const int passes = argc > 2 ? atoi(argv[2]) : 10;
	std::vector<trace::CodecStream> streams;
	if (!trace::LoadCodecTrace(argv[1], streams)) return 2;   // entity numbers at offset 0, as the legacy code reads them

	MessageEncoder enc;
	std::vector<uint8_t> out;
	std::vector<uint8_t> legacyOut(1 << 17);
	PlayerState sps{}, svps{};
	std::vector<uint8_t> sents(static_cast<size_t>(legacy::kMaxEntities) * legacy::kEntityBytes);
	legacy::SnapshotOut scratch{ sps.data(), svps.data(), sents.data(), 0 };
	trace::DecodeTarget target;            // netcodec's decoded snapshot
	trace::BaselineData gsScratch;         // netcodec's decoded gamestate baselines

	std::vector<std::unique_ptr<StreamRun>> runs;
	size_t nBench = 0, bytesNew = 0, bytesLegacy = 0, legacyBad = 0, newBad = 0;
	for (const trace::CodecStream &s : streams) {
		auto run = std::make_unique<StreamRun>();
		const auto addBaselines = [&](const BaselineTable &bl) {
			auto copy = std::make_unique<trace::BaselineData>();
			copy->Assign(bl);
			auto imgs = std::make_unique<std::vector<EntityState>>(copy->table);
			for (uint32_t n = 0; n < imgs->size(); n++) SetEntityNumber((*imgs)[n], n);   // the legacy code reads every number
			run->baselineSets.push_back(std::move(copy));
			auto ptrs = std::make_unique<std::vector<const uint8_t *>>();
			for (const EntityState &e : *imgs) ptrs->push_back(e.data());
			run->legacyBaselineImages.push_back(std::move(imgs));
			run->legacyBaselinePtrs.push_back(std::move(ptrs));
		};
		addBaselines({});
		for (const trace::CodecMessage &cm : s) {
			Item it;
			it.cm = &cm;
			it.baselines = run->baselineSets.back()->View();
			it.legacyBaselines = run->legacyBaselinePtrs.back()->data();
			it.base.delta = cm.delta;
			bool bench = Benchable(cm.msg);
			if (cm.delta != 0) {
				const auto b = run->server.bySeq.find(cm.seq - cm.delta);
				if (b == run->server.bySeq.end()) continue;
				it.base.snap = b->second.snap; it.base.header = &b->second.header;
				const auto lb = run->legacyBySeq.find(cm.seq - cm.delta);
				it.legacyBase = lb != run->legacyBySeq.end() ? &lb->second->view.snap : nullptr;
				bench = bench && it.legacyBase != nullptr;
			}
			// netcodec once: encodes, decodes to the same message
			HeaderRecord rec, drec;
			if (!enc.Encode(cm.msg, it.base, it.baselines, run->serverHistory, 1 << 20, out, rec)) newBad++;
			it.bytes = out;
			run->serverHistory.Add(cm.msg);
			Message dm;
			uint32_t delta = 0;
			run->client.seq = cm.seq;
			const bool decoded = DecodeMessage(it.bytes, run->client, run->clientHistory, it.baselines, gsScratch.Out(), target.Out(), dm,
			                                   delta, drec);
			if (!decoded || delta != cm.delta || !trace::SameMessage(dm, cm.msg)) newBad++;
			run->clientHistory.Add(dm);
			// legacy once: encode, decode, compare with the recording
			auto in = std::make_unique<LegacyInput>();
			FillLegacyInput(cm.msg, static_cast<int>(cm.delta), *in);
			const Op *sop = SnapshotOp(cm.msg);
			if (bench) {
				const int n = legacy::Encode(in->view, it.legacyBase, it.legacyBaselines, legacyOut.data(), static_cast<int>(legacyOut.size()));
				it.legacyBytes.assign(legacyOut.begin(), legacyOut.begin() + n);
				const bool ok = n > 0 && legacy::Decode(it.legacyBytes.data(), n, it.legacyBase, it.legacyBaselines, scratch) &&
				                (!sop || LegacyMatches(scratch, sop->snap));
				if (!ok) { legacyBad++; bench = false; }
				else { nBench++; bytesNew += it.bytes.size(); bytesLegacy += it.legacyBytes.size(); }
			}
			for (const Op &op : cm.msg.ops)
				if (op.type == OpType::GameState) addBaselines(op.gameState.baselines);
			it.legacyIn = in.get();
			if (sop) {
				run->server.bySeq[cm.seq] = Stored{ &sop->snap, rec };
				run->client.bySeq[cm.seq] = Stored{ &sop->snap, drec };
				run->legacyBySeq[cm.seq] = in.get();
			}
			if (sop || bench) run->legacyInputs.push_back(std::move(in));
			if (bench) run->items.push_back(std::move(it));
		}
		runs.push_back(std::move(run));
	}
	printf("messages benchmarked: %zu (commands and snapshots only); netcodec mismatches %zu, legacy round-trip failures %zu\n",
	       nBench, newBad, legacyBad);

	volatile size_t sink = 0;
	const double tEncNew = Best(passes, [&] {
		size_t acc = 0;
		for (auto &r : runs)
			for (const Item &it : r->items) {
				HeaderRecord rec;
				if (enc.Encode(it.cm->msg, it.base, it.baselines, r->serverHistory, 1 << 20, out, rec)) acc += out.size();
			}
		sink = sink + acc;
	});
	const double tEncLegacy = Best(passes, [&] {
		size_t acc = 0;
		for (auto &r : runs)
			for (const Item &it : r->items)
				acc += static_cast<size_t>(legacy::Encode(it.legacyIn->view, it.legacyBase, it.legacyBaselines, legacyOut.data(),
				                                          static_cast<int>(legacyOut.size())));
		sink = sink + acc;
	});
	const double tDecNew = Best(passes, [&] {
		size_t acc = 0;
		Message dm;
		for (auto &r : runs)
			for (const Item &it : r->items) {
				uint32_t delta = 0;
				HeaderRecord drec;
				r->client.seq = it.cm->seq;
				if (DecodeMessage(it.bytes, r->client, r->clientHistory, it.baselines, gsScratch.Out(), target.Out(), dm, delta, drec))
					acc += dm.ops.size();
			}
		sink = sink + acc;
	});
	const double tDecLegacy = Best(passes, [&] {
		size_t acc = 0;
		for (auto &r : runs)
			for (const Item &it : r->items)
				if (legacy::Decode(it.legacyBytes.data(), static_cast<int>(it.legacyBytes.size()), it.legacyBase, it.legacyBaselines, scratch))
					acc += static_cast<size_t>(scratch.numEntities);
		sink = sink + acc;
	});

	// warm: the fastest of 5 codings of each message, summed; best of 3 passes; ns per message
	const auto warm = [&](auto code) {
		double best = 1e300;
		for (int rep = 0; rep < 3; rep++) {
			uint64_t sum = 0;
			const double t0 = NowNs();
			const uint64_t c0 = Ticks();
			for (auto &r : runs)
				for (const Item &it : r->items) {
					uint64_t fastest = ~uint64_t{ 0 };
					for (int k = 0; k < 5; k++) {
						const uint64_t s = Ticks();
						code(*r, it);
						fastest = std::min(fastest, Ticks() - s);
					}
					sum += fastest;
				}
			const double nsPerTick = (NowNs() - t0) / static_cast<double>(Ticks() - c0);
			best = std::min(best, static_cast<double>(sum) * nsPerTick);
		}
		return best;
	};
	const double wEncNew = warm([&](StreamRun &r, const Item &it) {
		HeaderRecord rec;
		if (enc.Encode(it.cm->msg, it.base, it.baselines, r.serverHistory, 1 << 20, out, rec)) sink = sink + out.size();
	});
	const double wEncLegacy = warm([&](StreamRun &, const Item &it) {
		sink = sink + static_cast<size_t>(legacy::Encode(it.legacyIn->view, it.legacyBase, it.legacyBaselines, legacyOut.data(),
		                                                 static_cast<int>(legacyOut.size())));
	});
	Message wdm;
	const double wDecNew = warm([&](StreamRun &r, const Item &it) {
		uint32_t delta = 0;
		HeaderRecord drec;
		r.client.seq = it.cm->seq;
		if (DecodeMessage(it.bytes, r.client, r.clientHistory, it.baselines, gsScratch.Out(), target.Out(), wdm, delta, drec))
			sink = sink + wdm.ops.size();
	});
	const double wDecLegacy = warm([&](StreamRun &, const Item &it) {
		if (legacy::Decode(it.legacyBytes.data(), static_cast<int>(it.legacyBytes.size()), it.legacyBase, it.legacyBaselines, scratch))
			sink = sink + static_cast<size_t>(scratch.numEntities);
	});

	const double n = static_cast<double>(nBench);
	printf("bytes per message: netcodec %.1f, legacy %.1f (ratio %.3f)\n", static_cast<double>(bytesNew) / n,
	       static_cast<double>(bytesLegacy) / n, static_cast<double>(bytesNew) / static_cast<double>(bytesLegacy));
	printf("  %-22s %9.1f ns/message  warm %9.1f\n", "encode netcodec", tEncNew / n, wEncNew / n);
	printf("  %-22s %9.1f ns/message  warm %9.1f\n", "encode legacy", tEncLegacy / n, wEncLegacy / n);
	printf("  %-22s %9.1f ns/message  warm %9.1f\n", "decode netcodec", tDecNew / n, wDecNew / n);
	printf("  %-22s %9.1f ns/message  warm %9.1f\n", "decode legacy", tDecLegacy / n, wDecLegacy / n);
	printf("  netcodec / legacy: encode cold %.3f warm %.3f, decode cold %.3f warm %.3f\n", tEncNew / tEncLegacy,
	       wEncNew / wEncLegacy, tDecNew / tDecLegacy, wDecNew / wDecLegacy);
	return newBad != 0 || legacyBad != 0 ? 1 : 0;
}
