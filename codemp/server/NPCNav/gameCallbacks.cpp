//rww - callbacks the navigation system needs to make to the game code.
#include "../../game/q_shared.h"
#include "../../game/g_public.h"
#include "../server.h"

qboolean GNavCallback_NAV_ClearPathToPoint( sharedEntity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEntNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_CLEARPATHTOPOINT, self->s.number, pmins, pmaxs, point, clipmask, okToHitEntNum);
}

qboolean GNavCallback_NPC_ClearLOS( sharedEntity_t *ent, const vec3_t end )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_CLEARLOS, ent->s.number, end);
}

int GNavCallback_NAVNEW_ClearPathBetweenPoints(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int ignore, int clipmask)
{
	return VM_Call(gvm, GAME_NAV_CLEARPATHBETWEENPOINTS, start, end, mins, maxs, ignore, clipmask);
}

qboolean GNavCallback_NAV_CheckNodeFailedForEnt( sharedEntity_t *ent, int nodeNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_CHECKNODEFAILEDFORENT, ent->s.number, nodeNum);
}

qboolean GNavCallback_G_EntIsUnlockedDoor( int entityNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_ENTISUNLOCKEDDOOR, entityNum);
}

qboolean GNavCallback_G_EntIsDoor( int entityNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_ENTISDOOR, entityNum);
}

qboolean GNavCallback_G_EntIsBreakable( int entityNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_ENTISBREAKABLE, entityNum);
}

qboolean GNavCallback_G_EntIsRemovableUsable( int entNum )
{
	return (qboolean)VM_Call(gvm, GAME_NAV_ENTISREMOVABLEUSABLE, entNum);
}

void GNavCallback_CP_FindCombatPointWaypoints( void )
{
	VM_Call(gvm, GAME_NAV_FINDCOMBATPOINTWAYPOINTS);
}
