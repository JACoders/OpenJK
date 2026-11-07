// netcodec: whole-message codec of the new protocol (server to client): server commands, gamestate, download
// blocks and snapshots (player state, vehicle player state, entities), entropy-coded with static trained tables.
//
// States are byte images of the engine structures (entityState_t, playerState_t): the codec reads and writes the
// networked fields at the offsets of the generated schema, so the engine passes its own structures unchanged.
// Field values follow the legacy widths of the schema (an unsigned field keeps its low bits, a signed one is
// sign-extended), except booleans and user* fields, coded by their C type.
//
// This header is the codec's internal C++ interface (tests, benchmarks, tools); the engine will use a smaller header
// over the same views (step 6 part E).
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rans.h"

namespace netcodec {

inline constexpr size_t kEntityBytes = 532;    // sizeof(entityState_t), checked against the schema
inline constexpr size_t kPlayerBytes = 1552;   // sizeof(playerState_t)
inline constexpr uint32_t kMaxEntities = 1024; // entity numbers are below this (GENTITYNUM_BITS = 10)

using EntityState = std::array<uint8_t, kEntityBytes>;
using PlayerState = std::array<uint8_t, kPlayerBytes>;

// entityState_t::number, at offset 0 (not a networked field): the codec takes an entity's number from it, and the
// decoder writes it
[[nodiscard]] inline uint32_t EntityNumber(std::span<const uint8_t> state) noexcept {
	uint32_t n = 0;
	std::memcpy(&n, state.first(4).data(), 4);
	return n;
}
inline void SetEntityNumber(std::span<uint8_t> state, uint32_t n) noexcept { std::memcpy(state.first(4).data(), &n, 4); }

// The views below read the engine's own arrays in place (no copy): the entity states of a snapshot, its player
// states, the entity baselines.

// A snapshot's entities in a ring of entity states (the engine's circular svs.snapshotEntities and
// cl.parseEntities), sorted by number: entity i is state (first + i) mod (ring.size() / kEntityBytes). A plain array
// is the ring of its own count of states, first 0.
struct EntityRing {
	std::span<const uint8_t> ring;   // whole entity states, one after the other
	uint32_t first = 0;              // below the ring's state count
	uint32_t count = 0;              // at most the ring's state count

	[[nodiscard]] std::span<const uint8_t> operator[](uint32_t i) const noexcept {
		const uint32_t size = ToU32(ring.size() / kEntityBytes);
		const uint32_t k = first + i;
		return ring.subspan((k < size ? k : k - size) * kEntityBytes, kEntityBytes);
	}
};

// The entity baselines by number (the engine's sv.svEntities[n].baseline, cl.entityBaselines[n]): the state of
// number n at byte n * stride; a number without a baseline holds the zero state, and a baseline's number field is
// its number (so number 0 has none, as in the engine). Empty: no baselines.
struct BaselineTable {
	std::span<const uint8_t> bytes;
	size_t stride = kEntityBytes;

	[[nodiscard]] std::span<const uint8_t> operator[](uint32_t n) const noexcept {   // the zero state when empty
		static constexpr std::array<uint8_t, kEntityBytes> kZero{};
		return bytes.empty() ? std::span<const uint8_t>(kZero) : bytes.subspan(n * stride, kEntityBytes);
	}
};

// A snapshot, as views of the caller's storage.
struct Snapshot {
	std::span<const uint8_t> ps;    // playerState_t
	std::span<const uint8_t> vps;   // the vehicle's playerState_t; empty when ps.m_iVehicleNum is 0
	EntityRing entities;
};

struct ConfigString {
	uint16_t index = 0;
	std::string text;
};

struct GameState {
	int64_t relSeq = 0;
	std::vector<ConfigString> configStrings;   // sorted by index
	BaselineTable baselines;
	int64_t clientNum = 0;
	int64_t checksum = 0;
};

enum class OpType : uint8_t { Nop = 1, GameState = 2, Command = 3, Snapshot = 4, SetGame = 5, Download = 6, MapChange = 7 };

// One operation of a server message. Only the members of its type are used. The snapshot and the gamestate's
// baselines are views: the storage they point to must outlive the message.
struct Op {
	OpType type = OpType::Nop;
	int64_t seq = 0;               // Command
	std::string text;              // Command, SetGame
	GameState gameState;           // GameState
	uint32_t block = 0;            // Download
	int64_t fileSize = 0;          // Download, block 0 only
	std::vector<uint8_t> data;     // Download
	int64_t time = 0;              // Snapshot: server time
	uint32_t flags = 0;            // Snapshot
	std::vector<uint8_t> areas;    // Snapshot
	Snapshot snap;                 // Snapshot
};

struct Message {
	int64_t ack = 0;
	std::vector<Op> ops;
};

// What both sides keep about a snapshot message, for the next messages coded against it.
struct HeaderRecord {
	int64_t ack = 0;
	std::optional<int64_t> time;           // set when the message had a snapshot
	int64_t cmdSeq = 0;                    // the client holds every server command up to this one
	// the snapshot this message was coded against (second-order predictions), or nullptr: both sides must give the
	// same (the engine: only while it holds it, fewer than PACKET_BACKUP messages back)
	const Snapshot *older = nullptr;
};

// The base a message is coded against: delta = sequence distance (1 to 255), 0 = no base.
struct Base {
	uint32_t delta = 0;
	const Snapshot *snap = nullptr;
	const HeaderRecord *header = nullptr;
};

// The texts of the server commands, by sequence number, as each side holds them (the server for retransmission, the
// client once received). A command is coded against an earlier one that the client is known to hold: in the same
// message, or at most kCommandWindow sequence numbers back and not after the base's HeaderRecord::cmdSeq. Both sides
// must keep at least the last 2 * kCommandWindow + 2 commands (retransmitted commands may run ahead of the window).
inline constexpr int64_t kCommandWindow = 127;

struct CommandHistory {
	CommandHistory() = default;
	CommandHistory(const CommandHistory &) = delete;
	CommandHistory &operator=(const CommandHistory &) = delete;
	CommandHistory(CommandHistory &&) = delete;
	CommandHistory &operator=(CommandHistory &&) = delete;
	virtual ~CommandHistory() = default;
	// the text, valid until the history changes; a null data() if not held (16 bytes: returned in registers)
	[[nodiscard]] virtual std::string_view Find(int64_t seq) const = 0;
};

class MessageEncoder {
public:
	// a message coded larger than this is also coded with its entity and player-state fields raw (escape), and the
	// smaller of the two is sent; also when the coded form does not fit in maxBytes
	static constexpr size_t kEscapeTrial = 1024;

	// Codes msg against base; baselines: the entity baselines of the current gamestate (a gamestate op inside the
	// message gives the baselines of the ops after it); history: the server's commands. Returns false if a value is
	// outside the model (a bug) or the output does not fit in maxBytes.
	[[nodiscard]] bool Encode(const Message &msg, const Base &base, const BaselineTable &baselines,
	                          const CommandHistory &history, size_t maxBytes, std::vector<uint8_t> &out, HeaderRecord &record);

private:
	std::vector<rans::Encoder::Op> ops_;   // record buffer of the rANS encoder, reused between messages
	std::vector<uint8_t> scratch_;         // output buffer, reused between messages
};

// Looks up the base of a delta: returns false if the client does not hold it.
struct BaseLookup {
	BaseLookup() = default;
	BaseLookup(const BaseLookup &) = delete;
	BaseLookup &operator=(const BaseLookup &) = delete;
	BaseLookup(BaseLookup &&) = delete;
	BaseLookup &operator=(BaseLookup &&) = delete;
	virtual ~BaseLookup() = default;
	[[nodiscard]] virtual bool Find(uint32_t delta, const Snapshot *&snap, const HeaderRecord *&header) const = 0;
};

// Where the decoder writes: the baselines (by number, as BaselineTable) and the snapshot (player states, and the
// entities from ring state `first` on, wrapping; at most maxEntities, the engine's MAX_SNAPSHOT_ENTITIES, or the
// message is malformed). The decoded message's snapshot and gamestate are views of this storage.
struct BaselineTableOut {
	std::span<uint8_t> bytes;
	size_t stride = kEntityBytes;
};
struct SnapshotOut {
	std::span<uint8_t> ps;    // kPlayerBytes
	std::span<uint8_t> vps;   // kPlayerBytes, written when the player is in a vehicle
	std::span<uint8_t> ring;  // whole entity states
	uint32_t first = 0;
	uint32_t maxEntities = kMaxEntities;
};

// Schema, for tests and tools: offsets of the entity fields in codec order, and of every player-state field
// (the union of the player, pilot and vehicle tables, sorted by name).
[[nodiscard]] std::span<const uint16_t> EntityFieldOffsets() noexcept;
[[nodiscard]] std::span<const uint16_t> PlayerFieldOffsets() noexcept;

// Decodes one message. baselines: the current baselines; gamestate: where a gamestate's baselines go (cleared, then
// written; the ops after it use them; the engine passes the same array as baselines); snapshot: where the snapshot
// goes (one per message at most); history: the commands the client holds. Returns false on malformed input (the
// message must then be dropped; a gamestate may have been written already).
[[nodiscard]] bool DecodeMessage(std::span<const uint8_t> bytes, const BaseLookup &lookup, const CommandHistory &history,
                                 const BaselineTable &baselines, const BaselineTableOut &gamestate, const SnapshotOut &snapshot,
                                 Message &msg, uint32_t &delta, HeaderRecord &record);

// Training (tools). symbols[ctx][symbol]: the symbols the encoder codes, per context (a context that is never coded
// has no entry); rawBits[ctx]: the raw bits coded right after a symbol of the context. CountMessage adds one message
// (same arguments and result as Encode, no output). Quantize turns the counts of one context into the frequencies of
// its table (freq.size() symbols).
struct SymbolCounts {
	std::vector<std::vector<uint64_t>> symbols;
	std::vector<uint64_t> rawBits;
};
[[nodiscard]] SymbolCounts NewSymbolCounts();
[[nodiscard]] bool CountMessage(const Message &msg, const Base &base, const BaselineTable &baselines,
                                const CommandHistory &history, HeaderRecord &record, SymbolCounts &counts);
void Quantize(std::span<const uint64_t> counts, std::span<uint16_t> freq) noexcept;

} // namespace netcodec
