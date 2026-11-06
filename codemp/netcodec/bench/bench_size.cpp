// netcodec_bench_size: coded size of the corpus against the bytes the server sent (the engine's own coding, as
// recorded in the demos), with the encoding time. Same selection and groups as the measurements of plan step 5: the
// first message of a demo (the gamestate the client rebuilt) is left out; "snapshot-only" messages have one snapshot
// operation and nothing else.
// Usage: netcodec_bench_size <trace.bin> ...   (train.bin, holdout.bin, captures.bin)
#include "demo_trace.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace netcodec;

namespace {

struct Sizes {
	std::vector<std::pair<uint32_t, uint32_t>> v;   // coded, engine
	uint64_t coded = 0, engine = 0;

	void Add(uint32_t c, uint32_t e) {
		v.emplace_back(c, e);
		coded += c;
		engine += e;
	}
};

using Groups = std::map<std::string, Sizes>;

void Report(const char *title, Groups &groups) {
	std::printf("\n  %s\n    %-44s %8s %8s %5s %5s | %8s %6s\n", title, "", "n", "mean", "p95", "max", "engine", "ratio");
	for (auto &[key, g] : groups) {
		std::ranges::sort(g.v);
		const auto n = static_cast<double>(g.v.size());
		const double mean = static_cast<double>(g.coded) / n, em = static_cast<double>(g.engine) / n;
		std::printf("    %-44.44s %8zu %8.1f %5u %5u | %8.1f %6.3f\n", key.c_str(), g.v.size(), mean,
		            g.v[static_cast<size_t>(n * 0.95)].first, g.v.back().first, em, mean / em);
	}
}

bool SnapshotOnly(const Message &m) noexcept {
	return std::ranges::all_of(m.ops, [](const Op &op) { return op.type == OpType::Snapshot; }) && !m.ops.empty();
}

bool GameStateOnly(const Message &m) noexcept {
	return std::ranges::all_of(m.ops, [](const Op &op) { return op.type == OpType::GameState; }) && !m.ops.empty();
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 2) { std::fprintf(stderr, "usage: %s trace.bin ...\n", argv[0]); return 2; }
	Groups all, snaps, byType, byInterval, byAge, byMap;
	MessageEncoder enc;
	std::vector<uint8_t> out;
	size_t refused = 0, coded = 0;
	std::chrono::steady_clock::duration encodeTime{};
	for (int a = 1; a < argc; a++) {
		trace::DemoTraceFile in(argv[a]);
		if (!in.Ok()) return 2;
		trace::DemoStream s;
		while (in.Next(s)) {
			const std::string set = s.Meta(0);
			const std::string type = set == "capture" ? s.Meta(1) : s.Meta(2);
			char interval[32];
			std::snprintf(interval, sizeof interval, " %.1f", set == "capture" ? 50.0 : std::strtod(s.Meta(5).c_str(), nullptr));
			refused += trace::ServerLoop(s, [&](const trace::DemoMessage &m, const Base &base, const std::vector<Entity> &baselines,
			                                    const CommandHistory &history, HeaderRecord &rec) {
				const auto t0 = std::chrono::steady_clock::now();
				const bool ok = enc.Encode(m.msg, base, baselines, history, 1 << 20, out, rec);
				encodeTime += std::chrono::steady_clock::now() - t0;
				coded++;
				if (!ok || GameStateOnly(m.msg)) return ok;
				const auto n = ToU32(out.size());
				all[set].Add(n, m.engineBytes);
				if (!SnapshotOnly(m.msg)) return true;
				snaps[set].Add(n, m.engineBytes);
				byType[set + " " + type].Add(n, m.engineBytes);
				byInterval[set + interval].Add(n, m.engineBytes);
				char age[16];
				std::snprintf(age, sizeof age, " %u", std::min(m.delta, 9U));
				byAge[set + age].Add(n, m.engineBytes);
				if (set == "holdout") byMap[s.Meta(3)].Add(n, m.engineBytes);
				return true;
			});
		}
		if (!in.Ok()) return 2;
	}
	std::printf("messages coded %zu, refused %zu; encoding %.0f ns per message\n", coded, refused,
	            static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(encodeTime).count()) / static_cast<double>(coded));
	for (const auto &[set, g] : all)
		std::printf("  all messages but the gamestate, %-8s coded %" PRIu64 " bytes, engine %" PRIu64 ", ratio %.3f\n", set.c_str(),
		            g.coded, g.engine,
		            static_cast<double>(g.coded) / static_cast<double>(g.engine));
	Report("snapshot-only messages by set", snaps);
	Report("by set and game type", byType);
	Report("by set and snapshot interval (ms)", byInterval);
	Report("by set and base age (0 = no base)", byAge);
	Report("held-out set by map", byMap);
	return refused != 0 ? 1 : 0;
}
