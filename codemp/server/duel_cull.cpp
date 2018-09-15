#include "qcommon/qcommon.h"
#include "duel_cull.h"

static qboolean isPlayer(sharedEntity_t *ent) {
	if (ent->s.eType == ET_PLAYER)
		return qtrue;

	return qfalse;
}

static qboolean isNPC(sharedEntity_t *ent) {
	if (ent->s.eType == ET_NPC)
		return qtrue;

	return qfalse;
}

static qboolean isMover(sharedEntity_t *ent) {
	if (ent->s.eType == ET_MOVER)
		return qtrue;

	return qfalse;
}

sharedEntity_t *flatten(sharedEntity_t *ent) {
	//Com_Printf("flatten\n");
	if (ent->s.eType == ET_MISSILE) {
		//Com_Printf("ET_MISSILE\n");
		return SV_GentityNum(ent->r.ownerNum);
	}
	if (ent->s.eType == ET_EVENTS + EV_GENERAL_SOUND) {
		//Com_Printf("EV_GENERAL_SOUND\n");
		//return SV_GentityNum(ent->s.otherEntityNum);
		return ent;
		//ent->s.owner
	}
	if (ent->s.eType == ET_EVENTS + EV_SABER_HIT) {
		//Com_Printf("EV_SABER_HIT\n");
		return SV_GentityNum(ent->s.otherEntityNum2 == ENTITYNUM_NONE ? ent->s.otherEntityNum : ent->s.otherEntityNum2);
	}
	if (ent->s.eType == ET_EVENTS + EV_SHIELD_HIT) {
		//Com_Printf("EV_SHIELD_HIT\n");
		return SV_GentityNum(ent->s.otherEntityNum);
	}
	// some of EV_SABER_BLOCK's are not owned
	if (ent->s.eType == ET_EVENTS + EV_SABER_BLOCK) {
		//Com_Printf("EV_SABER_BLOCK\n");
		return SV_GentityNum(ent->s.otherEntityNum2 == ENTITYNUM_NONE ? ent->s.otherEntityNum : ent->s.otherEntityNum2);
	}
	if (ent->s.eFlags & EF_PLAYER_EVENT) {
		//return SV_GentityNum(ent->s.otherEntityNum);
#if 0
		Com_Printf("EV_SABER_ATTACK: ent: %i, singleClient: %i\n",
			SV_NumForGentity(ent),
			ent->r.singleClient
		);
#endif
		return SV_GentityNum(ent->r.singleClient);
	}
	if (ent->s.eType == ET_EVENTS + EV_PLAYER_TELEPORT_IN
		|| ent->s.eType == ET_EVENTS + EV_PLAYER_TELEPORT_OUT) {
		//Com_Printf("EV_PLAYER_TELEPORT_X\n");
		return SV_GentityNum(ent->s.clientNum);
	}
	if ((ent->s.event & ~EV_EVENT_BITS) == EV_GRENADE_BOUNCE) {
		return SV_GentityNum(ent->r.ownerNum);
	}
	//Com_Printf("END FLATTEN\n");
	return ent;
}

static playerState_t *GetPS(sharedEntity_t *ent) {
	return SV_GameClientNum(ent->s.number);
}

static qboolean isDueling(sharedEntity_t *ent) {
	if (isPlayer(flatten(ent)) && GetPS(flatten(ent))->duelInProgress)
		return qtrue;

	return qfalse;
}

static qboolean isActor(sharedEntity_t *ent) {
	if (!isMover(ent) && (isPlayer(flatten(ent)) || isNPC(flatten(ent))))
		return qtrue;

	return qfalse;
}

static qboolean isDuelOpponent(sharedEntity_t *A, sharedEntity_t *B) { //wtf void??
	auto a = flatten(A);
	auto b = flatten(B);

	if (isDueling(a) && isDueling(b) && (a == b || a->playerState->duelIndex == SV_NumForGentity(b)))
		return qtrue;

	return qfalse;
}

qboolean DuelCull(sharedEntity_t *ent, sharedEntity_t *touch) { //figure something out for smooth collision?

	if (!sv_snapShotDuelCull->integer)
		return qfalse;

	if (isActor(ent) && isActor(touch)) {
		if (!isDueling(ent) && !isDueling(touch)) { //2 players in ffa
			return qfalse; //don't cull
		}
		else if (!isDueling(ent) && isDueling(touch)) { //they're dueling and we're not
			return qfalse; //don't cull
		}
		else if (isDuelOpponent(ent, touch)) { //we're in a duel and this is our opponent
			return qfalse; //don't cull
		}
		return qtrue; //cull everything else
	}

	return qfalse;
}
