// netcodec: whole-message codec of the new protocol (server to client): server commands, gamestate, download
// blocks and snapshots (player state, vehicle player state, entities), entropy-coded with static trained tables.
//
// States are byte images of the engine structures (entityState_t, playerState_t): the codec reads and writes the
// networked fields at the offsets of the generated schema, so the engine passes its own structures unchanged.
// Field values follow the legacy widths of the schema (an unsigned field keeps its low bits, a signed one is
// sign-extended), except booleans and user* fields, coded by their C type.
//
// This header is the codec's internal C++ interface (tests, benchmarks, and the engine glue of plan step 7, which
// will expose a smaller header to the engine).
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "rans.h"

namespace netcodec {

inline constexpr size_t kEntityBytes = 532;    // sizeof(entityState_t), checked against the schema
inline constexpr size_t kPlayerBytes = 1552;   // sizeof(playerState_t)
inline constexpr uint32_t kMaxEntities = 1024; // entity numbers are below this (GENTITYNUM_BITS = 10)

using EntityState = std::array<uint8_t, kEntityBytes>;
using PlayerState = std::array<uint8_t, kPlayerBytes>;

struct Entity {
	uint16_t number = 0;
	EntityState state{};
};

struct Snapshot {
	PlayerState ps{};
	bool hasVehicle = false;       // a vehicle player state follows (ps.m_iVehicleNum != 0)
	PlayerState vps{};
	std::vector<Entity> entities;  // sorted by number
};

struct ConfigString {
	uint16_t index = 0;
	std::string text;
};

struct GameState {
	int64_t relSeq = 0;
	std::vector<ConfigString> configStrings;   // sorted by index
	std::vector<Entity> baselines;             // sorted by number
	int64_t clientNum = 0;
	int64_t checksum = 0;
};

enum class OpType : uint8_t { Nop = 1, GameState = 2, Command = 3, Snapshot = 4, SetGame = 5, Download = 6, MapChange = 7 };

// One operation of a server message. Only the members of its type are used.
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
	const Snapshot *older = nullptr;       // the snapshot this message was coded against (second-order prediction)
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
	[[nodiscard]] virtual const std::string *Find(int64_t seq) const = 0;   // nullptr if not held
};

class MessageEncoder {
public:
	// Codes msg against base; baselines: the entity baselines of the current gamestate (updated by a gamestate op
	// inside the message for the ops after it); history: the server's commands. Returns false if a value is outside
	// the model (a bug) or the output does not fit in maxBytes.
	[[nodiscard]] bool Encode(const Message &msg, const Base &base, const std::vector<Entity> &baselines,
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

// Schema, for tests and tools: offsets of the entity fields in codec order, and of every player-state field
// (the union of the player, pilot and vehicle tables, sorted by name).
[[nodiscard]] std::span<const uint16_t> EntityFieldOffsets() noexcept;
[[nodiscard]] std::span<const uint16_t> PlayerFieldOffsets() noexcept;

// Decodes one message. baselines: in, the current baselines; out, replaced if the message has a gamestate; history:
// the commands the client holds. Returns false on malformed input (the message must then be dropped).
[[nodiscard]] bool DecodeMessage(std::span<const uint8_t> bytes, const BaseLookup &lookup, const CommandHistory &history,
                                 std::vector<Entity> &baselines, Message &msg, uint32_t &delta, HeaderRecord &record);

// Training (tools). symbols[ctx][symbol]: the symbols the encoder codes, per context (a context that is never coded
// has no entry); rawBits[ctx]: the raw bits coded right after a symbol of the context. CountMessage adds one message
// (same arguments and result as Encode, no output). Quantize turns the counts of one context into the frequencies of
// its table (freq.size() symbols).
struct SymbolCounts {
	std::vector<std::vector<uint64_t>> symbols;
	std::vector<uint64_t> rawBits;
};
[[nodiscard]] SymbolCounts NewSymbolCounts();
[[nodiscard]] bool CountMessage(const Message &msg, const Base &base, const std::vector<Entity> &baselines,
                                const CommandHistory &history, HeaderRecord &record, SymbolCounts &counts);
void Quantize(std::span<const uint64_t> counts, std::span<uint16_t> freq) noexcept;

} // namespace netcodec
