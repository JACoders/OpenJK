// Owning storage behind the codec's views, for the tests, benchmarks and tools: a snapshot, a baseline table, and
// the decoder's output. A view stays valid while its storage lives and keeps its size.
#pragma once

#include "netcodec/codec.h"

#include <algorithm>
#include <span>
#include <vector>

namespace netcodec::trace {

static_assert(sizeof(EntityState) == kEntityBytes, "entity states are contiguous");

[[nodiscard]] inline std::span<const uint8_t> Bytes(const std::vector<EntityState> &v) noexcept {
	return { reinterpret_cast<const uint8_t *>(v.data()), v.size() * kEntityBytes };
}
[[nodiscard]] inline std::span<uint8_t> Bytes(std::vector<EntityState> &v) noexcept {
	return { reinterpret_cast<uint8_t *>(v.data()), v.size() * kEntityBytes };
}

struct SnapshotData {
	PlayerState ps{};
	PlayerState vps{};
	bool hasVehicle = false;               // a vehicle player state (ps.m_iVehicleNum != 0)
	std::vector<EntityState> entities;     // sorted by number, the number at offset 0

	[[nodiscard]] Snapshot View() const noexcept {
		return Snapshot{ .ps = ps,
		                 .vps = hasVehicle ? std::span<const uint8_t>(vps) : std::span<const uint8_t>(),
		                 .entities = EntityRing{ .ring = Bytes(entities), .first = 0, .count = ToU32(entities.size()) } };
	}

	void Assign(const Snapshot &s) {
		std::ranges::copy(s.ps, ps.begin());
		hasVehicle = !s.vps.empty();
		if (hasVehicle) std::ranges::copy(s.vps, vps.begin());
		entities.resize(s.entities.count);
		for (uint32_t i = 0; i < s.entities.count; i++) std::ranges::copy(s.entities[i], entities[i].begin());
	}
};

// baselines by number: kMaxEntities states, zero where there is none
struct BaselineData {
	std::vector<EntityState> table = std::vector<EntityState>(kMaxEntities);

	[[nodiscard]] BaselineTable View() const noexcept { return BaselineTable{ .bytes = Bytes(table), .stride = kEntityBytes }; }
	[[nodiscard]] BaselineTableOut Out() noexcept { return BaselineTableOut{ .bytes = Bytes(table), .stride = kEntityBytes }; }

	void Assign(const BaselineTable &t) {
		for (uint32_t n = 0; n < kMaxEntities; n++) std::ranges::copy(t[n], table[n].begin());
	}
};

// where the tests decode a snapshot
struct DecodeTarget {
	PlayerState ps{};
	PlayerState vps{};
	std::vector<EntityState> ring = std::vector<EntityState>(kMaxEntities);

	[[nodiscard]] SnapshotOut Out() noexcept {
		return SnapshotOut{ .ps = ps, .vps = vps, .ring = Bytes(ring), .first = 0, .maxEntities = kMaxEntities };
	}
};

} // namespace netcodec::trace
