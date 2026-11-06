// The legacy (1.01 / OpenJK) snapshot coding, for the benchmark only: OpenJK's own codemp/qcommon/msg.cpp and
// huffman.cpp, driven as the engine drives them (server: SV_SendClientSnapshot, SV_WriteSnapshotToClient,
// SV_EmitPacketEntities; client: CL_ParseServerMessage, CL_ParseSnapshot, CL_ParsePacketEntities).
// No engine type appears here: states are byte images of entityState_t and playerState_t, as in netcodec.
#pragma once

#include <cstddef>
#include <cstdint>

namespace legacy {

constexpr size_t kEntityBytes = 532;
constexpr size_t kPlayerBytes = 1552;
constexpr int kMaxEntities = 1024;

struct SnapshotView {
	const uint8_t *ps = nullptr;
	const uint8_t *vps = nullptr;              // nullptr: no vehicle player state (zeros if the ps has a vehicle)
	const uint8_t *const *entities = nullptr;  // entity images in number order, the number field set
	int numEntities = 0;
};

struct Command {
	int32_t seq = 0;
	const char *text = nullptr;
};

struct MessageView {
	int32_t ack = 0;
	const Command *commands = nullptr;
	int numCommands = 0;
	bool hasSnapshot = false;
	int32_t serverTime = 0;
	int delta = 0;
	int flags = 0;
	const uint8_t *areas = nullptr;
	int numAreas = 0;
	SnapshotView snap;
};

// Snapshot written by the decoder: buffers owned by the caller.
struct SnapshotOut {
	uint8_t *ps = nullptr;             // kPlayerBytes
	uint8_t *vps = nullptr;            // kPlayerBytes
	uint8_t *entities = nullptr;       // kMaxEntities * kEntityBytes
	int numEntities = 0;
};

// Encodes one server message (ack, commands, snapshot, svc_EOF, Huffman as the engine does); returns its size in
// bytes. base: the snapshot the client acknowledged (delta from it), or nullptr. baselines: kMaxEntities images.
int Encode(const MessageView &msg, const SnapshotView *base, const uint8_t *const *baselines, uint8_t *out, int capacity);

// Decodes one message as the client does; false on a malformed message. base: the client's copy of the base.
bool Decode(const uint8_t *data, int size, const SnapshotView *base, const uint8_t *const *baselines, SnapshotOut &out);

} // namespace legacy
