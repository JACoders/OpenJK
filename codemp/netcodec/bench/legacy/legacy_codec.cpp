// Engine-side glue of the legacy benchmark (see legacy_codec.h). Compiled like the engine (C++11, OpenJK headers).
// The loops below follow codemp/server/sv_snapshot.cpp and codemp/client/cl_parse.cpp; the engine's msg functions
// take non-const pointers but do not modify their sources, hence the const_casts.
#include "legacy_codec.h"

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

#include <cstring>

static_assert(sizeof(entityState_t) == legacy::kEntityBytes, "entityState_t layout");
static_assert(sizeof(playerState_t) == legacy::kPlayerBytes, "playerState_t layout");

namespace legacy {

namespace {

entityState_t *Ent(const uint8_t *img) { return reinterpret_cast<entityState_t *>(const_cast<uint8_t *>(img)); }
playerState_t *Ps(const uint8_t *img) { return reinterpret_cast<playerState_t *>(const_cast<uint8_t *>(img)); }

const playerState_t &ZeroPs() {
	static playerState_t zero;
	return zero;
}

// SV_EmitPacketEntities
void EmitPacketEntities(msg_t *msg, const SnapshotView *from, const SnapshotView &to, const uint8_t *const *baselines) {
	const int fromNum = from ? from->numEntities : 0;
	int newindex = 0, oldindex = 0;
	while (newindex < to.numEntities || oldindex < fromNum) {
		entityState_t *newent = newindex < to.numEntities ? Ent(to.entities[newindex]) : nullptr;
		entityState_t *oldent = oldindex < fromNum ? Ent(from->entities[oldindex]) : nullptr;
		const int newnum = newent ? newent->number : 9999;
		const int oldnum = oldent ? oldent->number : 9999;
		if (newnum == oldnum) {
			MSG_WriteDeltaEntity(msg, oldent, newent, qfalse);
			oldindex++; newindex++;
		} else if (newnum < oldnum) {
			MSG_WriteDeltaEntity(msg, Ent(baselines[newnum]), newent, qtrue);
			newindex++;
		} else {
			MSG_WriteDeltaEntity(msg, oldent, NULL, qtrue);
			oldindex++;
		}
	}
	MSG_WriteBits(msg, MAX_GENTITIES - 1, GENTITYNUM_BITS);
}

} // namespace

int Encode(const MessageView &m, const SnapshotView *base, const uint8_t *const *baselines, uint8_t *out, int capacity) {
	msg_t msg;
	MSG_Init(&msg, out, capacity);
	MSG_WriteLong(&msg, m.ack);                       // SV_SendClientSnapshot: lastClientCommand
	for (int i = 0; i < m.numCommands; i++) {         // SV_UpdateServerCommandsToClient
		MSG_WriteByte(&msg, svc_serverCommand);
		MSG_WriteLong(&msg, m.commands[i].seq);
		MSG_WriteString(&msg, m.commands[i].text);
	}
	if (m.hasSnapshot) {                              // SV_WriteSnapshotToClient
		MSG_WriteByte(&msg, svc_snapshot);
		MSG_WriteLong(&msg, m.serverTime);
		MSG_WriteByte(&msg, m.delta);
		MSG_WriteByte(&msg, m.flags);
		MSG_WriteByte(&msg, m.numAreas);
		MSG_WriteData(&msg, m.areas, m.numAreas);
		playerState_t *ps = Ps(m.snap.ps);
		playerState_t *vps = m.snap.vps ? Ps(m.snap.vps) : const_cast<playerState_t *>(&ZeroPs());
		if (base) {
			MSG_WriteDeltaPlayerstate(&msg, Ps(base->ps), ps);
			if (ps->m_iVehicleNum) {
				if (!Ps(base->ps)->m_iVehicleNum) MSG_WriteDeltaPlayerstate(&msg, NULL, vps, qtrue);
				else MSG_WriteDeltaPlayerstate(&msg, base->vps ? Ps(base->vps) : const_cast<playerState_t *>(&ZeroPs()), vps, qtrue);
			}
		} else {
			MSG_WriteDeltaPlayerstate(&msg, NULL, ps);
			if (ps->m_iVehicleNum) MSG_WriteDeltaPlayerstate(&msg, NULL, vps, qtrue);
		}
		EmitPacketEntities(&msg, base, m.snap, baselines);
	}
	MSG_WriteByte(&msg, svc_EOF);
	return msg.overflowed ? 0 : msg.cursize;
}

bool Decode(const uint8_t *data, int size, const SnapshotView *base, const uint8_t *const *baselines, SnapshotOut &out) {
	msg_t msg;
	MSG_Init(&msg, const_cast<uint8_t *>(data), size);
	msg.cursize = size;
	MSG_BeginReading(&msg);
	MSG_ReadLong(&msg);                               // CL_ParseServerMessage: reliableAcknowledge
	out.numEntities = 0;
	for (;;) {
		const int cmd = MSG_ReadByte(&msg);
		if (cmd == svc_EOF || msg.readcount > msg.cursize) break;
		if (cmd == svc_serverCommand) {
			MSG_ReadLong(&msg);
			MSG_ReadString(&msg);
		} else if (cmd == svc_snapshot) {             // CL_ParseSnapshot
			MSG_ReadLong(&msg);
			MSG_ReadByte(&msg);
			MSG_ReadByte(&msg);
			byte areamask[MAX_MAP_AREA_BYTES];
			const int len = MSG_ReadByte(&msg);
			if (len > static_cast<int>(sizeof(areamask))) return false;
			MSG_ReadData(&msg, areamask, len);
			playerState_t *ps = reinterpret_cast<playerState_t *>(out.ps);
			playerState_t *vps = reinterpret_cast<playerState_t *>(out.vps);
			if (base) {
				MSG_ReadDeltaPlayerstate(&msg, Ps(base->ps), ps);
				if (ps->m_iVehicleNum)
					MSG_ReadDeltaPlayerstate(&msg, base->vps ? Ps(base->vps) : const_cast<playerState_t *>(&ZeroPs()), vps, qtrue);
			} else {
				MSG_ReadDeltaPlayerstate(&msg, NULL, ps);
				if (ps->m_iVehicleNum) MSG_ReadDeltaPlayerstate(&msg, NULL, vps, qtrue);
			}
			// CL_ParsePacketEntities and CL_DeltaEntity
			entityState_t *ents = reinterpret_cast<entityState_t *>(out.entities);
			const int oldCount = base ? base->numEntities : 0;
			int oldindex = 0;
			int oldnum = oldindex < oldCount ? Ent(base->entities[oldindex])->number : 99999;
			const auto keep = [&](entityState_t *state) {
				if (state->number != MAX_GENTITIES - 1) out.numEntities++;
			};
			for (;;) {
				const int newnum = MSG_ReadBits(&msg, GENTITYNUM_BITS);
				if (newnum == MAX_GENTITIES - 1) break;
				if (msg.readcount > msg.cursize || out.numEntities >= kMaxEntities) return false;
				while (oldnum < newnum) {
					ents[out.numEntities] = *Ent(base->entities[oldindex]);
					keep(&ents[out.numEntities]);
					oldindex++;
					oldnum = oldindex < oldCount ? Ent(base->entities[oldindex])->number : 99999;
				}
				if (oldnum == newnum) {
					MSG_ReadDeltaEntity(&msg, Ent(base->entities[oldindex]), &ents[out.numEntities], newnum);
					keep(&ents[out.numEntities]);
					oldindex++;
					oldnum = oldindex < oldCount ? Ent(base->entities[oldindex])->number : 99999;
				} else {
					MSG_ReadDeltaEntity(&msg, Ent(baselines[newnum]), &ents[out.numEntities], newnum);
					keep(&ents[out.numEntities]);
				}
			}
			while (oldnum != 99999) {
				ents[out.numEntities] = *Ent(base->entities[oldindex]);
				keep(&ents[out.numEntities]);
				oldindex++;
				oldnum = oldindex < oldCount ? Ent(base->entities[oldindex])->number : 99999;
			}
		} else {
			return false;                             // the benchmark sends only commands and snapshots
		}
	}
	return msg.readcount <= msg.cursize;
}

} // namespace legacy
