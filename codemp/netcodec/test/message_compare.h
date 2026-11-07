// Field-by-field comparison of messages for the tests: decoded against original (whole state images).
#pragma once

#include "netcodec/codec.h"

#include <algorithm>

namespace netcodec::trace {

[[nodiscard]] inline bool SameSnapshot(const Snapshot &a, const Snapshot &b) {
	if (!std::ranges::equal(a.ps, b.ps) || a.vps.empty() != b.vps.empty() || !std::ranges::equal(a.vps, b.vps) ||
	    a.entities.count != b.entities.count)
		return false;
	for (uint32_t i = 0; i < a.entities.count; i++)
		if (!std::ranges::equal(a.entities[i], b.entities[i])) return false;
	return true;
}

[[nodiscard]] inline bool SameBaselines(const BaselineTable &a, const BaselineTable &b) {
	for (uint32_t n = 0; n < kMaxEntities; n++)
		if (!std::ranges::equal(a[n], b[n])) return false;
	return true;
}

[[nodiscard]] inline bool SameOp(const Op &a, const Op &b) {
	if (a.type != b.type) return false;
	switch (a.type) {
	case OpType::Command: return a.seq == b.seq && a.text == b.text;
	case OpType::SetGame: return a.text == b.text;
	case OpType::GameState: {
		const GameState &x = a.gameState, &y = b.gameState;
		if (x.relSeq != y.relSeq || x.clientNum != y.clientNum || static_cast<uint32_t>(x.checksum) != static_cast<uint32_t>(y.checksum) ||
		    x.configStrings.size() != y.configStrings.size() || !SameBaselines(x.baselines, y.baselines))
			return false;
		for (size_t i = 0; i < x.configStrings.size(); i++)
			if (x.configStrings[i].index != y.configStrings[i].index || x.configStrings[i].text != y.configStrings[i].text) return false;
		return true;
	}
	case OpType::Download: return a.block == b.block && a.fileSize == b.fileSize && a.data == b.data;
	case OpType::Snapshot: return a.time == b.time && a.flags == b.flags && a.areas == b.areas && SameSnapshot(a.snap, b.snap);
	default: return true;
	}
}

[[nodiscard]] inline bool SameMessage(const Message &a, const Message &b) {
	if (a.ack != b.ack || a.ops.size() != b.ops.size()) return false;
	for (size_t i = 0; i < a.ops.size(); i++)
		if (!SameOp(a.ops[i], b.ops[i])) return false;
	return true;
}

} // namespace netcodec::trace
