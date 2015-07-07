/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

//is this client piloting this veh?
static QINLINE qboolean CG_Piloting(int vehNum)
{
	centity_t *veh;

	if (!vehNum)
	{
		return qfalse;
	}

	veh = &cg_entities[vehNum];

	if (veh->currentState.owner != cg.predictedPlayerState.clientNum)
	{ //the owner should be the current pilot
		return qfalse;
	}

	return qtrue;
}

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int				i;
	centity_t		*cent;
	snapshot_t		*snap;
	entityState_t	*ent;
	vec3_t			difference;
	float			dsquared;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}

	//rww - Horrible, terrible, awful hack.
	//We don't send your client entity from the server,
	//so it isn't added into the solid list from the snapshot,
	//and in addition, it has no solid data. So we will force
	//adding it in based on a hardcoded player bbox size.
	//This will cause issues if the player box size is ever
	//changed..
	if (cg_numSolidEntities < MAX_ENTITIES_IN_SNAPSHOT)
	{
		vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
		vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};
		int i, j, k;

		i = playerMaxs[0];
		if (i<1)
			i = 1;
		if (i>255)
			i = 255;

		// z is not symetric
		j = (-playerMins[2]);
		if (j<1)
			j = 1;
		if (j>255)
			j = 255;

		// and z playerMaxs can be negative...
		k = (playerMaxs[2]+32);
		if (k<1)
			k = 1;
		if (k>255)
			k = 255;

		cg_solidEntities[cg_numSolidEntities] = &cg_entities[cg.predictedPlayerState.clientNum];
		cg_solidEntities[cg_numSolidEntities]->currentState.solid = (k<<16) | (j<<8) | i;

		cg_numSolidEntities++;
	}

	dsquared = /*RMG_distancecull.value*/5000+500;
	dsquared *= dsquared;

	for(i=0;i<cg_numpermanents;i++)
	{
		cent = cg_permanents[i];
		VectorSubtract(cent->lerpOrigin, snap->ps.origin, difference);
		if (cent->currentState.eType == ET_TERRAIN ||
			((difference[0]*difference[0]) + (difference[1]*difference[1]) + (difference[2]*difference[2])) <= dsquared)
		{
			cent->currentValid = qtrue;
			if ( cent->nextState.solid )
			{
				cg_solidEntities[cg_numSolidEntities] = cent;
				cg_numSolidEntities++;
			}
		}
		else
		{
			cent->currentValid = qfalse;
		}
	}
}

static QINLINE qboolean CG_VehicleClipCheck(centity_t *ignored, trace_t *trace)
{
	if (!trace || trace->entityNum < 0 || trace->entityNum >= ENTITYNUM_WORLD)
	{ //it's alright then
		return qtrue;
	}

	if (ignored->currentState.eType != ET_PLAYER &&
		ignored->currentState.eType != ET_NPC)
	{ //can't possibly be valid then
		return qtrue;
	}

	if (ignored->currentState.m_iVehicleNum)
	{ //see if the ignore ent is a vehicle/rider - if so, see if the ent we supposedly hit is a vehicle/rider.
		//if they belong to each other, we don't want to collide them.
		centity_t *otherguy = &cg_entities[trace->entityNum];

		if (otherguy->currentState.eType != ET_PLAYER &&
			otherguy->currentState.eType != ET_NPC)
		{ //can't possibly be valid then
			return qtrue;
		}

		if (otherguy->currentState.m_iVehicleNum)
		{ //alright, both of these are either a vehicle or a player who is on a vehicle
			int index;

			if (ignored->currentState.eType == ET_PLAYER
				|| (ignored->currentState.eType == ET_NPC && ignored->currentState.NPC_class != CLASS_VEHICLE) )
			{ //must be a player or NPC riding a vehicle
				index = ignored->currentState.m_iVehicleNum;
			}
			else
			{ //a vehicle
				index = ignored->currentState.m_iVehicleNum-1;
			}

			if (index == otherguy->currentState.number)
			{ //this means we're riding or being ridden by this guy, so don't collide
				return qfalse;
			}
			else
			{//see if I'm hitting one of my own passengers
				if (otherguy->currentState.eType == ET_PLAYER
					|| (otherguy->currentState.eType == ET_NPC && otherguy->currentState.NPC_class != CLASS_VEHICLE) )
				{ //must be a player or NPC riding a vehicle
					if (otherguy->currentState.m_iVehicleNum==ignored->currentState.number)
					{ //this means we're other guy is riding the ignored ent
						return qfalse;
					}
				}
			}
		}
	}

	return qtrue;
}

/*
====================
CG_ClipMoveToEntities

====================
*/
extern void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vec3_t origin, vec3_t mins, vec3_t maxs,
										int clientNum, int tracemask,
										void (*localTrace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask)); // bg_pmove.c
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr, qboolean g2Check ) {
	int			i, x, zd, zu;
	trace_t		trace, oldTrace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;
	centity_t	*ignored = NULL;

	if (skipNumber != -1 && skipNumber != ENTITYNUM_NONE)
	{
		ignored = &cg_entities[skipNumber];
	}

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->number > MAX_CLIENTS &&
			 (ent->genericenemyindex-MAX_GENTITIES==cg.predictedPlayerState.clientNum || ent->genericenemyindex-MAX_GENTITIES==cg.predictedVehicleState.clientNum) )
//		if (ent->number > MAX_CLIENTS && cg.snap && ent->genericenemyindex && (ent->genericenemyindex-MAX_GENTITIES) == cg.snap->ps.clientNum)
		{ //rww - method of keeping objects from colliding in client-prediction (in case of ownership)
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap->CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			if (ent->eType == ET_NPC && ent->NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{ //try to dynamically adjust his bbox dynamically, if possible
				float *old = cent->m_pVehicle->m_vOrientation;
				cent->m_pVehicle->m_vOrientation = &cent->lerpAngles[0];
				BG_VehicleAdjustBBoxForOrientation(cent->m_pVehicle, cent->lerpOrigin, bmins, bmaxs,
											cent->currentState.number, MASK_PLAYERSOLID, NULL);
				cent->m_pVehicle->m_vOrientation = old;
			}

			cmodel = trap->CM_TempModel( bmins, bmaxs, 0 );
			VectorCopy( vec3_origin, angles );

			VectorCopy( cent->lerpOrigin, origin );
		}


		trap->CM_TransformedTrace ( &trace, start, end, mins, maxs, cmodel,  mask, origin, angles, 0);
		trace.entityNum = trace.fraction != 1.0 ? ent->number : ENTITYNUM_NONE;

		if (g2Check || (ignored && ignored->currentState.m_iVehicleNum))
		{
			//keep these older variables around for a bit, incase we need to replace them in the Ghoul2 Collision check
			//or in the vehicle owner trace check
			oldTrace = *tr;
		}

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;

			//rww 12-02-02
			tr->entityNum = trace.entityNum = ent->number;
		}
		if ( tr->allsolid )
		{
			if (ignored && ignored->currentState.m_iVehicleNum)
			{
				trace.entityNum = ent->number;
                if (CG_VehicleClipCheck(ignored, &trace))
				{ //this isn't our vehicle, we're really stuck
					return;
				}
				else
				{ //it's alright, keep going
					trace = oldTrace;
					*tr = trace;
				}
			}
			else
			{
				return;
			}
		}

		if (g2Check)
		{
			if (trace.entityNum == ent->number && cent->ghoul2)
			{
				CG_G2TraceCollide(&trace, mins, maxs, start, end);

				if (trace.entityNum == ENTITYNUM_NONE)
				{ //g2 trace failed, so put it back where it was.
					trace = oldTrace;
					*tr = trace;
				}
			}
		}

		if (ignored && ignored->currentState.m_iVehicleNum)
		{ //see if this is the vehicle we hit
			centity_t *hit = &cg_entities[trace.entityNum];
            if (!CG_VehicleClipCheck(ignored, &trace))
			{ //looks like it
				trace = oldTrace;
				*tr = trace;
			}
			else if (hit->currentState.eType == ET_MISSILE &&
				hit->currentState.owner == ignored->currentState.number)
			{ //hack, don't hit own missiles
				trace = oldTrace;
				*tr = trace;
			}
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask ) {
	trace_t	t;

	trap->CM_Trace ( &t, start, end, mins, maxs, 0, mask, 0);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t, qfalse);

	*result = t;
}

/*
================
CG_G2Trace
================
*/
void	CG_G2Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask ) {
	trace_t	t;

	trap->CM_Trace ( &t, start, end, mins, maxs, 0, mask, 0);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t, qtrue);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap->CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap->CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap->CM_TransformedPointContents( point, cmodel, cent->lerpOrigin, cent->lerpAngles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->ps and cg.nextFrame->ps
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap->GetCurrentCmdNumber();
		trap->GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->ps.bobCycle;
	if ( i < prev->ps.bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle(
				prev->ps.viewangles[i], next->ps.viewangles[i], f );
		}
		out->velocity[i] = prev->ps.velocity[i] +
			f * (next->ps.velocity[i] - prev->ps.velocity[i] );
	}

}

static void CG_InterpolateVehiclePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;

	out = &cg.predictedVehicleState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->vps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap->GetCurrentCmdNumber();
		trap->GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->vps.bobCycle;
	if ( i < prev->vps.bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prev->vps.bobCycle + f * ( i - prev->vps.bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->vps.origin[i] + f * (next->vps.origin[i] - prev->vps.origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle(
				prev->vps.viewangles[i], next->vps.viewangles[i], f );
		}
		out->velocity[i] = prev->vps.velocity[i] +
			f * (next->vps.velocity[i] - prev->vps.velocity[i] );
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;

	if ( !cg_predictItems.integer ) {
		return;
	}
	if ( !BG_PlayerTouchesItem( &cg.predictedPlayerState, &cent->currentState, cg.time ) ) {
		return;
	}

	if (cent->currentState.brokenLimbs)
	{ //dropped item
		return;
	}

	if (cent->currentState.eFlags & EF_ITEMPLACEHOLDER)
	{
		return;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( cgs.gametype, &cent->currentState, &cg.predictedPlayerState ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	//Currently there is no reliable way of knowing if the client has touched a certain item before another if they are next to each other, or rather
	//if the server has touched them in the same order. This results often in grabbing an item in the prediction and the server giving you the other
	//item. So for now prediction of armor, health, and ammo is disabled.
/*
	if (item->giType == IT_ARMOR)
	{ //rww - this will be stomped next update, but it's set so that we don't try to pick up two shields in one prediction and have the server cancel one
	//	cg.predictedPlayerState.stats[STAT_ARMOR] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_HEALTH)
	{ //same as above, for health
	//	cg.predictedPlayerState.stats[STAT_HEALTH] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_AMMO)
	{ //same as above, for ammo
	//	cg.predictedPlayerState.ammo[item->giTag] += item->quantity;

		//FIXME: This can't be predicted properly at the moment
		return;
	}

	if (item->giType == IT_HOLDABLE)
	{ //same as above, for holdables
	//	cg.predictedPlayerState.stats[STAT_HOLDABLE_ITEMS] |= (1 << item->giTag);
	}
*/
	// Special case for flags.
	// We don't predict touching our own flag
	// Make sure the item type is also a flag too
	if( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY ) {
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
			item->giType == IT_TEAM && item->giTag == PW_REDFLAG)
			return;
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
			item->giType == IT_TEAM && item->giTag == PW_BLUEFLAG)
			return;
	}

	if (item->giType == IT_POWERUP &&
		(item->giTag == PW_FORCE_ENLIGHTENED_LIGHT || item->giTag == PW_FORCE_ENLIGHTENED_DARK))
	{
		if (item->giTag == PW_FORCE_ENLIGHTENED_LIGHT)
		{
			if (cg.predictedPlayerState.fd.forceSide != FORCE_LIGHTSIDE)
			{
				return;
			}
		}
		else
		{
			if (cg.predictedPlayerState.fd.forceSide != FORCE_DARKSIDE)
			{
				return;
			}
		}
	}


	// grab it
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.number , &cg.predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		cg.predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if ( !cg.predictedPlayerState.ammo[ item->giTag ] ) {
			cg.predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && cg.predictedPlayerState.pm_type != PM_JETPACK && cg.predictedPlayerState.pm_type != PM_FLOAT && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap->CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap->CM_Trace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, cg_pmove.mins, cg_pmove.maxs, cmodel, -1, 0 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}

#if 0
static QINLINE void CG_EntityStateToPlayerState( entityState_t *s, playerState_t *ps )
{
	//currently unused vars commented out for speed.. only uncomment if you need them.
	ps->clientNum = s->number;
	VectorCopy( s->pos.trBase, ps->origin );
	VectorCopy( s->pos.trDelta, ps->velocity );
	ps->saberLockFrame = s->forceFrame;
	ps->legsAnim = s->legsAnim;
	ps->torsoAnim = s->torsoAnim;
	ps->legsFlip = s->legsFlip;
	ps->torsoFlip = s->torsoFlip;
	ps->clientNum = s->clientNum;
	ps->saberMove = s->saberMove;

	/*
	VectorCopy( s->apos.trBase, ps->viewangles );

	ps->fd.forceMindtrickTargetIndex = s->trickedentindex;
	ps->fd.forceMindtrickTargetIndex2 = s->trickedentindex2;
	ps->fd.forceMindtrickTargetIndex3 = s->trickedentindex3;
	ps->fd.forceMindtrickTargetIndex4 = s->trickedentindex4;

	ps->electrifyTime = s->emplacedOwner;

	ps->speed = s->speed;

	ps->genericEnemyIndex = s->genericenemyindex;

	ps->activeForcePass = s->activeForcePass;

	ps->movementDir = s->angles2[YAW];

	ps->eFlags = s->eFlags;

	ps->saberInFlight = s->saberInFlight;
	ps->saberEntityNum = s->saberEntityNum;

	ps->fd.forcePowersActive = s->forcePowersActive;

	if (s->bolt1)
	{
		ps->duelInProgress = qtrue;
	}
	else
	{
		ps->duelInProgress = qfalse;
	}

	if (s->bolt2)
	{
		ps->dualBlade = qtrue;
	}
	else
	{
		ps->dualBlade = qfalse;
	}

	ps->emplacedIndex = s->otherEntityNum2;

	ps->saberHolstered = s->saberHolstered; //reuse bool in entitystate for players differently

	ps->genericEnemyIndex = -1; //no real option for this

	//The client has no knowledge of health levels (except for the client entity)
	if (s->eFlags & EF_DEAD)
	{
		ps->stats[STAT_HEALTH] = 0;
	}
	else
	{
		ps->stats[STAT_HEALTH] = 100;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	ps->weapon = s->weapon;
	ps->groundEntityNum = s->groundEntityNum;

	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if (s->powerups & (1 << i))
		{
			ps->powerups[i] = 30;
		}
		else
		{
			ps->powerups[i] = 0;
		}
	}

	ps->loopSound = s->loopSound;
	ps->generic1 = s->generic1;
	*/
}
#endif

playerState_t cgSendPSPool[ MAX_GENTITIES ];
playerState_t *cgSendPS[MAX_GENTITIES];

//#define _PROFILE_ES_TO_PS

#ifdef _PROFILE_ES_TO_PS
int g_cgEStoPSTime = 0;
#endif

//Assign all the entity playerstate pointers to the corresponding one
//so that we can access playerstate stuff in bg code (and then translate
//it back to entitystate data)
void CG_PmoveClientPointerUpdate()
{
	int i;

	memset(&cgSendPSPool[0], 0, sizeof(cgSendPSPool));

	for ( i = 0 ; i < MAX_GENTITIES ; i++ )
	{
		cgSendPS[i] = &cgSendPSPool[i];

		// These will be invalid at this point on Xbox
		cg_entities[i].playerState = cgSendPS[i];
	}

	//Set up bg entity data
	cg_pmove.baseEnt = (bgEntity_t *)cg_entities;
	cg_pmove.entSize = sizeof(centity_t);

	cg_pmove.ghoul2 = NULL;
}

//check if local client is on an eweb
qboolean CG_UsingEWeb(void)
{
	if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN && cg.predictedPlayerState.emplacedIndex &&
		cg_entities[cg.predictedPlayerState.emplacedIndex].currentState.weapon == WP_NONE)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
extern void CG_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern	vmCvar_t		cg_showVehBounds;
pmove_t cg_vehPmove;
qboolean cg_vehPmoveSet = qfalse;

void CG_PredictPlayerState( void ) {
	int			cmdNum, current, i;
	playerState_t	oldPlayerState;
	playerState_t	oldVehicleState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;
	centity_t *pEnt;
	clientInfo_t *ci;

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
		if (CG_Piloting(cg.snap->ps.m_iVehicleNum))
		{
			cg.predictedVehicleState = cg.snap->vps;
		}
	}

	// demo playback just copies the moves
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
		{
			CG_InterpolateVehiclePlayerState(qfalse);
		}
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_noPredict.integer || g_synchronousClients.integer || CG_UsingEWeb() ) {
		CG_InterpolatePlayerState( qtrue );
		if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
		{
			CG_InterpolateVehiclePlayerState(qtrue);
		}
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;

	pEnt = &cg_entities[cg.predictedPlayerState.clientNum];
	//rww - bgghoul2
	if (cg_pmove.ghoul2 != pEnt->ghoul2) //only update it if the g2 instance has changed
	{
		if (cg.snap &&
			pEnt->ghoul2 &&
			!(cg.snap->ps.pm_flags & PMF_FOLLOW) &&
			cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
		{
			cg_pmove.ghoul2 = pEnt->ghoul2;
			cg_pmove.g2Bolts_LFoot = trap->G2API_AddBolt(pEnt->ghoul2, 0, "*l_leg_foot");
			cg_pmove.g2Bolts_RFoot = trap->G2API_AddBolt(pEnt->ghoul2, 0, "*r_leg_foot");
		}
		else
		{
			cg_pmove.ghoul2 = NULL;
		}
	}

	ci = &cgs.clientinfo[cg.predictedPlayerState.clientNum];

	//I'll just do this every frame in case the scale changes in realtime (don't need to update the g2 inst for that)
	VectorCopy(pEnt->modelScale, cg_pmove.modelScale);
	//rww end bgghoul2

	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;
	if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
	{
		oldVehicleState = cg.predictedVehicleState;
	}

	current = trap->GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap->GetUserCmd( cmdNum, &oldestCmd );
	if ( oldestCmd.serverTime > cg.snap->ps.commandTime
		&& oldestCmd.serverTime < cg.time ) {	// special check for map_restart
		if ( cg_showMiss.integer ) {
			trap->Print ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap->GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		cg.nextSnap->ps.slopeRecalcTime = cg.predictedPlayerState.slopeRecalcTime; //this is the only value we want to maintain seperately on server/client
		cg.predictedPlayerState = cg.nextSnap->ps;
		if (CG_Piloting(cg.nextSnap->ps.m_iVehicleNum))
		{
			cg.predictedVehicleState = cg.nextSnap->vps;
		}
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.snap->ps.slopeRecalcTime = cg.predictedPlayerState.slopeRecalcTime; //this is the only value we want to maintain seperately on server/client
		cg.predictedPlayerState = cg.snap->ps;
		if (CG_Piloting(cg.snap->ps.m_iVehicleNum))
		{
			cg.predictedVehicleState = cg.snap->vps;
		}
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap->Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap->Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_float = pmove_float.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	for ( i = 0 ; i < MAX_GENTITIES ; i++ )
	{
		//Written this way for optimal speed, even though it doesn't look pretty.
		//(we don't want to spend the time assigning pointers as it does take
		//a small precious fraction of time and adds up in the loop.. so says
		//the precision timer!)

		if (cg_entities[i].currentState.eType == ET_PLAYER ||
			cg_entities[i].currentState.eType == ET_NPC)
		{
			// Need a new playerState_t on Xbox
			VectorCopy( cg_entities[i].currentState.pos.trBase, cgSendPS[i]->origin );
			VectorCopy( cg_entities[i].currentState.pos.trDelta, cgSendPS[i]->velocity );
			cgSendPS[i]->saberLockFrame = cg_entities[i].currentState.forceFrame;
			cgSendPS[i]->legsAnim = cg_entities[i].currentState.legsAnim;
			cgSendPS[i]->torsoAnim = cg_entities[i].currentState.torsoAnim;
			cgSendPS[i]->legsFlip = cg_entities[i].currentState.legsFlip;
			cgSendPS[i]->torsoFlip = cg_entities[i].currentState.torsoFlip;
			cgSendPS[i]->clientNum = cg_entities[i].currentState.clientNum;
			cgSendPS[i]->saberMove = cg_entities[i].currentState.saberMove;
		}
	}

	if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
	{
		cg_entities[cg.predictedPlayerState.clientNum].playerState = &cg.predictedPlayerState;
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].playerState = &cg.predictedVehicleState;

		//use the player command time, because we are running with the player cmds (this is even the case
		//on the server)
		cg.predictedVehicleState.commandTime = cg.predictedPlayerState.commandTime;
	}

	// run cmds
	moved = qfalse;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap->GetUserCmd( cmdNum, &cg_pmove.cmd );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime )
		{
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( CG_Piloting(oldPlayerState.m_iVehicleNum) &&
			cg.predictedVehicleState.commandTime == oldVehicleState.commandTime )
		{
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showVehMiss.integer ) {
					trap->Print( "VEH PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predictedVehicleState.origin,
					cg.predictedVehicleState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

				if ( cg_showVehMiss.integer ) {
					if (!VectorCompare( oldVehicleState.origin, adjusted )) {
						trap->Print("VEH prediction error\n");
					}
				}
				VectorSubtract( oldVehicleState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showVehMiss.integer ) {
						trap->Print("VEH Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showVehMiss.integer ) {
							trap->Print("VEH Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
				//
				if ( cg_showVehMiss.integer ) {
					if (!VectorCompare( oldVehicleState.vehOrientation, cg.predictedVehicleState.vehOrientation )) {
						trap->Print("VEH orient prediction error\n");
						trap->Print("VEH pitch prediction miss: %f\n", AngleSubtract( oldVehicleState.vehOrientation[0], cg.predictedVehicleState.vehOrientation[0] ) );
						trap->Print("VEH yaw prediction miss: %f\n", AngleSubtract( oldVehicleState.vehOrientation[1], cg.predictedVehicleState.vehOrientation[1] ) );
						trap->Print("VEH roll prediction miss: %f\n", AngleSubtract( oldVehicleState.vehOrientation[2], cg.predictedVehicleState.vehOrientation[2] ) );
					}
				}
			}
		}
		else if ( !oldPlayerState.m_iVehicleNum && //don't do pred err on ps while riding veh
			cg.predictedPlayerState.commandTime == oldPlayerState.commandTime )
		{
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showMiss.integer ) {
					trap->Print( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
					cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

				if ( cg_showMiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						trap->Print("prediction error\n");
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showMiss.integer ) {
						trap->Print("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showMiss.integer ) {
							trap->Print("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

		cg_pmove.animations = bgAllAnims[pEnt->localAnimIndex].anims;
		cg_pmove.gametype = cgs.gametype;

		cg_pmove.debugMelee = cgs.debugMelee;
		cg_pmove.stepSlideFix = cgs.stepSlideFix;
		cg_pmove.noSpecMove = cgs.noSpecMove;

		cg_pmove.nonHumanoid = (pEnt->localAnimIndex > 0);

		if (cg.snap && cg.snap->ps.saberLockTime > cg.time)
		{
			centity_t *blockOpp = &cg_entities[cg.snap->ps.saberLockEnemy];

			if (blockOpp)
			{
				vec3_t lockDir, lockAng;

				VectorSubtract( blockOpp->lerpOrigin, cg.snap->ps.origin, lockDir );
				vectoangles(lockDir, lockAng);

				VectorCopy(lockAng, cg_pmove.ps->viewangles);
			}
		}

		//THIS is pretty much bad, but...
		cg_pmove.ps->fd.saberAnimLevelBase = cg_pmove.ps->fd.saberAnimLevel;
		if ( cg_pmove.ps->saberHolstered == 1 )
		{
			if ( ci->saber[0].numBlades > 0 )
			{
				cg_pmove.ps->fd.saberAnimLevelBase = SS_STAFF;
			}
			else if ( ci->saber[1].model[0] )
			{
				cg_pmove.ps->fd.saberAnimLevelBase = SS_DUAL;
			}
		}

		Pmove (&cg_pmove);

		if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum) &&
			cg.predictedPlayerState.pm_type != PM_INTERMISSION)
		{ //we're riding a vehicle, let's predict it
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			int x, zd, zu;

			if (veh->m_pVehicle)
			{ //make sure pointer is set up to go to our predicted state
				veh->m_pVehicle->m_vOrientation = &cg.predictedVehicleState.vehOrientation[0];

				//keep this updated based on what the playerstate says
				veh->m_pVehicle->m_iRemovedSurfaces = cg.predictedVehicleState.vehSurfaces;

				trap->GetUserCmd( cmdNum, &veh->m_pVehicle->m_ucmd );

				if ( veh->m_pVehicle->m_ucmd.buttons & BUTTON_TALK )
				{ //forced input if "chat bubble" is up
					veh->m_pVehicle->m_ucmd.buttons = BUTTON_TALK;
					veh->m_pVehicle->m_ucmd.forwardmove = 0;
					veh->m_pVehicle->m_ucmd.rightmove = 0;
					veh->m_pVehicle->m_ucmd.upmove = 0;
				}
				cg_vehPmove.ps = &cg.predictedVehicleState;
				cg_vehPmove.animations = bgAllAnims[veh->localAnimIndex].anims;

				memcpy(&cg_vehPmove.cmd, &veh->m_pVehicle->m_ucmd, sizeof(usercmd_t));
				/*
				cg_vehPmove.cmd.rightmove = 0; //no vehicle can move right/left
				cg_vehPmove.cmd.upmove = 0; //no vehicle can move up/down
				*/

				cg_vehPmove.gametype = cgs.gametype;
				cg_vehPmove.ghoul2 = veh->ghoul2;

				cg_vehPmove.nonHumanoid = (veh->localAnimIndex > 0);

				/*
				x = (veh->currentState.solid & 255);
				zd = (veh->currentState.solid & 255);
				zu = (veh->currentState.solid & 255) - 32;

				cg_vehPmove.mins[0] = cg_vehPmove.mins[1] = -x;
				cg_vehPmove.maxs[0] = cg_vehPmove.maxs[1] = x;
				cg_vehPmove.mins[2] = -zd;
				cg_vehPmove.maxs[2] = zu;
				*/
				//I think this was actually wrong.. just copy-pasted from id code. Oh well.
				x = (veh->currentState.solid)&255;
				zd = (veh->currentState.solid>>8)&255;
				zu = (veh->currentState.solid>>15)&255;

				zu -= 32; //I don't quite get the reason for this.
				zd = -zd;

				//z/y must be symmetrical (blah)
				cg_vehPmove.mins[0] = cg_vehPmove.mins[1] = -x;
				cg_vehPmove.maxs[0] = cg_vehPmove.maxs[1] = x;
				cg_vehPmove.mins[2] = zd;
				cg_vehPmove.maxs[2] = zu;

				VectorCopy(veh->modelScale, cg_vehPmove.modelScale);

				if (!cg_vehPmoveSet)
				{ //do all the one-time things
					cg_vehPmove.trace = CG_Trace;
					cg_vehPmove.pointcontents = CG_PointContents;
					cg_vehPmove.tracemask = MASK_PLAYERSOLID;
					cg_vehPmove.debugLevel = 0;
					cg_vehPmove.g2Bolts_LFoot = -1;
					cg_vehPmove.g2Bolts_RFoot = -1;

					cg_vehPmove.baseEnt = (bgEntity_t *)cg_entities;
					cg_vehPmove.entSize = sizeof(centity_t);

					cg_vehPmoveSet = qtrue;
				}

				cg_vehPmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;
				cg_vehPmove.pmove_fixed = pmove_fixed.integer;
				cg_vehPmove.pmove_msec = pmove_msec.integer;

				cg_entities[cg.predictedPlayerState.clientNum].playerState = &cg.predictedPlayerState;
				veh->playerState = &cg.predictedVehicleState;

				//update boarding value sent from server. boarding is not predicted, but no big deal
				veh->m_pVehicle->m_iBoarding = cg.predictedVehicleState.vehBoarding;

				Pmove(&cg_vehPmove);
				/*
				if ( !cg_paused.integer )
				{
					Com_Printf( "%d - PITCH change %4.2f\n", cg.time, AngleSubtract(veh->m_pVehicle->m_vOrientation[0],veh->m_pVehicle->m_vPrevOrientation[0]) );
				}
				*/
				if ( cg_showVehBounds.integer )
				{
					vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
					vec3_t absmin, absmax;
					VectorAdd( cg_vehPmove.ps->origin, cg_vehPmove.mins, absmin );
					VectorAdd( cg_vehPmove.ps->origin, cg_vehPmove.maxs, absmax );
					CG_Cube( absmin, absmax, NPCDEBUG_RED, 0.25 );
				}
			}
		}

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
	}

	if ( cg_showMiss.integer > 1 ) {
		trap->Print( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showMiss.integer ) {
			trap->Print( "not moved\n" );
		}
		goto revertES;
	}

	if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
	{
		CG_AdjustPositionForMover( cg.predictedVehicleState.origin,
			cg.predictedVehicleState.groundEntityNum,
			cg.physicsTime, cg.time, cg.predictedVehicleState.origin );
	}
	else
	{
		// adjust for the movement of the groundentity
		CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
			cg.predictedPlayerState.groundEntityNum,
			cg.physicsTime, cg.time, cg.predictedPlayerState.origin );
	}

	if ( cg_showMiss.integer ) {
		if (cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS) {
			trap->Print("WARNING: dropped event\n");
		}
	}

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

	if ( cg_showMiss.integer ) {
		if (cg.eventSequence > cg.predictedPlayerState.eventSequence) {
			trap->Print("WARNING: double event\n");
			cg.eventSequence = cg.predictedPlayerState.eventSequence;
		}
	}

	if (cg.predictedPlayerState.m_iVehicleNum &&
		!CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
	{ //a passenger on this vehicle, bolt them in
		centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		VectorCopy(veh->lerpAngles, cg.predictedPlayerState.viewangles);
		VectorCopy(veh->lerpOrigin, cg.predictedPlayerState.origin);
	}

revertES:
	if (CG_Piloting(cg.predictedPlayerState.m_iVehicleNum))
	{
		centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];

		if (veh->m_pVehicle)
		{
			//switch ptr back for this ent in case we stop riding it
			veh->m_pVehicle->m_vOrientation = &cgSendPS[veh->currentState.number]->vehOrientation[0];
		}

		cg_entities[cg.predictedPlayerState.clientNum].playerState = cgSendPS[cg.predictedPlayerState.clientNum];
		veh->playerState = cgSendPS[veh->currentState.number];
	}

	//copy some stuff back into the entstates to help actually "predict" them if applicable
	for ( i = 0 ; i < MAX_GENTITIES ; i++ )
	{
		if (cg_entities[i].currentState.eType == ET_PLAYER ||
			cg_entities[i].currentState.eType == ET_NPC)
		{
			cg_entities[i].currentState.torsoAnim = cgSendPS[i]->torsoAnim;
			cg_entities[i].currentState.legsAnim = cgSendPS[i]->legsAnim;
			cg_entities[i].currentState.forceFrame = cgSendPS[i]->saberLockFrame;
			cg_entities[i].currentState.saberMove = cgSendPS[i]->saberMove;
		}
	}
}
