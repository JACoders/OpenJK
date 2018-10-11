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

// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

static void CG_CheckClientCheckpoints(void);
static void CG_AddStrafeTrails(void);
#define _LOGTRAILS 1
#if _LOGTRAILS
static void CG_LogStrafeTrail(void);
#endif
static void CG_LastWeapon(void);

/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity( centity_t *cent ) {
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if ( cent->snapShotTime < cg.time - EVENT_VALID_MSEC ) {
		cent->previousEvent = 0;
	}

	cent->trailTime = cg.snap->serverTime;

	VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	VectorCopy (cent->currentState.angles, cent->lerpAngles);

	if (cent->currentState.eFlags & EF_G2ANIMATING)
	{ //reset the animation state
		cent->pe.torso.animationNumber = -1;
		cent->pe.legs.animationNumber = -1;
	}

#if 0
	if (cent->isRagging && (cent->currentState.eFlags & EF_DEAD))
	{
		VectorAdd(cent->lerpOrigin, cent->lerpOriginOffset, cent->lerpOrigin);
	}
#endif

	if ( cent->currentState.eType == ET_PLAYER || cent->currentState.eType == ET_NPC ) {
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
void CG_TransitionEntity( centity_t *cent ) {
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents( cent );
}


/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap ) {
	int				i;
	centity_t		*cent;
	entityState_t	*state;

	cg.snap = snap;

	if ((cg_entities[snap->ps.clientNum].ghoul2 == NULL) && trap->G2_HaveWeGhoul2Models(cgs.clientinfo[snap->ps.clientNum].ghoul2Model))
	{
		trap->G2API_DuplicateGhoul2Instance(cgs.clientinfo[snap->ps.clientNum].ghoul2Model, &cg_entities[snap->ps.clientNum].ghoul2);
		CG_CopyG2WeaponInstance(&cg_entities[snap->ps.clientNum], FIRST_WEAPON, cg_entities[snap->ps.clientNum].ghoul2);

		if (trap->G2API_AddBolt(cg_entities[snap->ps.clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[snap->ps.clientNum].noFace = qtrue;
		}
	}
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy(&cent->currentState, state, sizeof(entityState_t));
		//cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}

//JAPRO - Clientside - Autorecord Demo - Start
	if ( cg_autoRecordDemo.integer && (1<<cgs.gametype) && cg.warmup <= 0 && !cg.demoPlayback )
	{
		time_t rawtime;
		char timeBuf[256] = {0}, buf[256] = {0}, mapname[MAX_QPATH] = {0};

		time( &rawtime );
		strftime( timeBuf, sizeof( timeBuf ), "%Y-%m-%d_%H-%M-%S", gmtime( &rawtime ) );
		Q_strncpyz( mapname, cgs.mapname + 5, sizeof( mapname ) );
		COM_StripExtension( mapname, mapname, sizeof( mapname ) );
		Com_sprintf( buf, sizeof( buf ), "%s_%s_%s_%s", timeBuf, gametypeStringShort[cgs.gametype], mapname, cgs.clientinfo[cg.clientNum].name );
		Q_strstrip( buf, "\n\r;:?*<>|\"\\/ ", NULL );
		Q_CleanStr( buf );
		cg.recording = qtrue;
		trap->SendConsoleCommand( va( "stoprecord; record %s\n", buf ) );
	}
//JAPRO - Clientside - Autorecord Demo - End

}


/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
extern qboolean CG_UsingEWeb(void); //cg_predict.c
static void CG_TransitionSnapshot( void ) {
	centity_t			*cent;
	snapshot_t			*oldFrame;
	int					i;

	if ( !cg.snap ) {
		trap->Error( ERR_DROP, "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		trap->Error( ERR_DROP, "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// if we had a map_restart, set everthing with initial
	if ( !cg.snap ) {
	}

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg.predictedPlayerEntity);
	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qfalse;

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		CG_TransitionEntity( cent );

		// remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	cg.nextSnap = NULL;

	// check for playerstate transition events
	if ( oldFrame ) {
		playerState_t	*ops, *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;	// will be cleared by prediction code
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg_noPredict.integer || g_synchronousClients.integer || CG_UsingEWeb() ) {
			CG_TransitionPlayerState( ps, ops );
		}
	}

}


/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void CG_SetNextSnap( snapshot_t *snap ) {
	int					num;
	entityState_t		*es;
	centity_t			*cent;

	cg.nextSnap = snap;

	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg.predictedPlayerEntity);
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	//cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;
	//No longer want to do this, as the cg_entities[clnum] and cg.predictedPlayerEntity are one in the same.

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ )
	{
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy(&cent->nextState, es, sizeof(entityState_t));
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		cg.nextFrameTeleport = qtrue;
	} else {
		cg.nextFrameTeleport = qfalse;
	}

	// if changing follow mode, don't interpolate
	if ( cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum ) {
		cg.nextFrameTeleport = qtrue;
	}

	// if changing server restarts, don't interpolate
	if ( ( cg.nextSnap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
		cg.nextFrameTeleport = qtrue;
	}

	// sort out solid entities
	CG_BuildSolidList();

	CG_CheckClientCheckpoints();
	CG_AddStrafeTrails();
#if _LOGTRAILS
	CG_LogStrafeTrail();
#endif
	CG_LastWeapon();
}


/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean	r;
	snapshot_t	*dest;

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 ) {
		trap->Print( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n",
			cg.latestSnapshotNum, cgs.processedSnapshotNum );
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else {
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		r = trap->GetSnapshot( cgs.processedSnapshotNum, dest );

		// FIXME: why would trap->GetSnapshot return a snapshot with the same server time
		if ( cg.snap && r && dest->serverTime == cg.snap->serverTime ) {
			//[BugFix30]
			//According to dumbledore, this situation occurs when you're playing back a demo that was record when
			//the game was running in local mode.  As such, we need to skip those snaps or the demo looks laggy.
			if ( cg.demoPlayback )
			{
				continue;
			}
			//[/BugFix30]
		}

		// if it succeeded, return
		if ( r ) {
			CG_AddLagometerSnapshotInfo( dest );
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}


/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots( void ) {
	snapshot_t		*snap;
	int				n;

	// see what the latest snapshot the client system has is
	trap->GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			// this should never happen
			trap->Error( ERR_DROP, "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
		}
		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
			CG_SetInitialSnapshot( snap );
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				break;
			}

			CG_SetNextSnap( snap );


			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				trap->Error( ERR_DROP, "CG_ProcessSnapshots: Server time went backwards" );
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		trap->Error( ERR_DROP, "CG_ProcessSnapshots: cg.snap == NULL" );
	}
	if ( cg.time < cg.snap->serverTime ) {
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		trap->Error( ERR_DROP, "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}

}

static qboolean InBox(vec3_t interpOrigin, clientCheckpoint_t clientCheckpoint)
{
	if (clientCheckpoint.x1 <= interpOrigin[0] && interpOrigin[0] <= clientCheckpoint.x2 && 
		clientCheckpoint.y1 <= interpOrigin[1] && interpOrigin[1] <= clientCheckpoint.y2 && 
		clientCheckpoint.z1 <= interpOrigin[2] && interpOrigin[2] <= clientCheckpoint.z2)
		return qtrue;
	return qfalse;
}

int InterpolateTouchTime(vec3_t position, vec3_t velocity, clientCheckpoint_t clientCheckpoint)
{ //We know that last client frame, they were not touching the flag, but now they are.  Last client frame was pmoveMsec ms ago, so we only want to interp inbetween that range.
	vec3_t	interpOrigin, delta;
	int lessTime = 0;
	int serverFrametime = cgs.svfps ? (1000 / cgs.svfps) : 50;

	VectorCopy(position, interpOrigin);
	VectorScale(velocity, 0.001f, delta);//Delta is how much they travel in 1 ms.

	VectorSubtract(interpOrigin, delta, interpOrigin);//Do it once before we loop

	while (InBox(interpOrigin, clientCheckpoint)) {//This will be done a max of pml.msec times, in theory, before we are guarenteed to not be in the trigger anymore.
		lessTime++; //Add one more ms to be subtracted
		VectorSubtract(interpOrigin, delta, interpOrigin); //Keep Rewinding position by a tiny bit, that corresponds with 1ms precision (delta*0.001), since delta is per second.
		if (lessTime >= serverFrametime) { //activator->client->pmoveMsec
			break; //In theory, this should never happen, but just incase stop it here.
		}
	}

	//Com_Printf("Interpolated back by %i\n", lessTime);
	return lessTime;
}

static void CheckTouchingCheckpoint(clientCheckpoint_t clientCheckpoint) {
	float time;

	if (cg.time - cg.lastCheckPointPrintTime < 1000)
		return;
	if (!InBox(cg.snap->ps.origin, clientCheckpoint))
		return;

	time = (cg.snap->serverTime - cg.snap->ps.duelTime); //Use snapshot time instead right?
	time -= InterpolateTouchTime(cg.snap->ps.origin, cg.snap->ps.velocity, clientCheckpoint);//Other is the trigger_multiple that set this off
	time /= 1000.0f;

	if (time < 0.001f)
		time = 0.001f;

	if (cg.displacementSamples)
		CG_CenterPrint(va("^2%.3fs^4, avg ^2%i^4u, max ^2%i^4u\n\n\n\n\n\n\n\n\n\n", time, cg.displacement/cg.displacementSamples, cg.maxSpeed), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
	else
		CG_CenterPrint( va("^2%.3fs^4\n\n\n\n\n\n\n\n\n\n", time), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
	cg.lastCheckPointPrintTime = cg.time;
}

static void CG_CheckClientCheckpoints(void) {
	int i;

	if (!cg.snap)
		return;
	if (!cg.snap->ps.stats[STAT_RACEMODE] || !cg.snap->ps.duelTime)
		return;

	for (i=0; i<MAX_CLIENT_CHECKPOINTS; i++) { //optimize this..?
		if (cg.clientCheckpoints[i].isSet)
			CheckTouchingCheckpoint(cg.clientCheckpoints[i]);
	}
}








static unsigned int GetTrailColorByClientNum(int clientNum) {
	if (cgs.clientinfo[clientNum].team == TEAM_RED) {
		switch (clientNum % 9) {
			case 0: return 0xFF1493;
			case 1: return 0xB22222;
			case 2: return 0xDC143C;
			case 3: return 0xFF7F50;
			case 4: return 0xFF0000;
			case 5: return 0xFFA500;
			case 6: return 0x8B0000;
			case 7: return 0xD2691E;
			case 8: return 0xCD5C5C;
			default: return 0xCD5C5C;
		}
	}
	else if (cgs.clientinfo[clientNum].team == TEAM_BLUE) {
		switch (clientNum % 9) {
			case 0: return 0x00FFFF;
			case 1: return 0x7FFFD4;
			case 2: return 0x00DED1;
			case 3: return 0x00BFFF;
			case 4: return 0x0000FF;
			case 5: return 0x008B8B;
			case 6: return 0x483D8B;
			case 7: return 0x191970;
			case 8: return 0x87CEFA;
			default: return 0x87CEFA;
		}
	}
	else {
		switch (clientNum) {
			case 0: return 0xFFFF00;
			case 1: return 0xFF00FF;
			case 2: return 0x66CCCC;
			case 3: return 0x663366;
			case 4: return 0xCC6633;
			case 5: return 0x66FF00;
			case 6: return 0xFFFFFF;
			case 7: return 0x00CC33;
			case 8: return 0x99FFFF;
			case 9: return 0xCC66FF;
			case 10: return 0x330066;
			case 11: return 0xFFFF00;
			case 12: return 0xFFFF00;
			case 13: return 0xFFFF00;
			case 14: return 0xFFFF00;
			case 15: return 0xFFFF00;
			case 16: return 0xFFFF00;
			case 17: return 0xFFFF00;
			case 18: return 0xFFFF00;
			case 19: return 0xFFFF00;
			default: return 0xFF9900;
		}
	}
}

#if _NEWTRAILS
void CG_StrafeTrailLine( vec3_t start, vec3_t end, int time, int clientNum, int number) {
	strafeTrail_t	*trail;
	unsigned int color = GetTrailColorByClientNum(clientNum);
	int SVFPS = cg_strafeTrailFPS.integer;

	if (SVFPS < 1)
		SVFPS = 1;
	else if (SVFPS > 1000)
		SVFPS = 1000;

	trail = CG_AllocStrafeTrail();

	trail->endTime = cg.time + time;
	trail->clientNum = clientNum+1;
	VectorCopy(start, trail->start);
	VectorCopy(end, trail->end);
	//trail->radius = radius;
	trail->color = color;

	cg.drawingStrafeTrails |= (1 << clientNum);

	if (cg_scorePlums.integer && (number > 0) && (number % SVFPS == 0)) {//oh this is stupid, assume sv_fps is 30 on servers that create it i guess.
		localEntity_t	*le;
		refEntity_t		*re;
		vec3_t			angles;

		le = CG_AllocLocalEntity();
		le->leFlags = clientNum + 1;
		le->leType = LE_SCOREPLUM;
		le->startTime = cg.time;
		le->endTime = cg.time + 60*60*1000;
		le->lifeRate = 0;

		le->color[0] = color & 0xff;
		color >>= 8;
		le->color[1] = color & 0xff;
		color >>= 8;
		le->color[2] = color & 0xff;

		//le->color[3] = 1.0;

		le->color[0] /= 255;
		le->color[1] /= 255;
		le->color[2] /= 255;
		
		//le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
		le->radius = number / SVFPS;

		VectorCopy( end, le->pos.trBase );
		le->pos.trBase[2] += 8;

		re = &le->refEntity;
		re->reType = RT_SPRITE;
		re->radius = 16;

		VectorClear(angles);
		AnglesToAxis( angles, re->axis );
	}
}
#else

void CG_StrafeTrailLine( vec3_t start, vec3_t end, int time, float radius, int clientNum) {
	localEntity_t	*le;
	refEntity_t		*re;

	unsigned int color = GetTrailColorByClientNum(clientNum);

	le = CG_AllocLocalEntity();
	le->leType = LE_LINE;
	le->startTime = cg.time;
	le->endTime = cg.time + time;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->leFlags = clientNum;

	re = &le->refEntity;
	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin);
	re->shaderTime = cg.time / 1000.0f;

	re->reType = RT_LINE;
	re->radius = 0.5*radius;
	re->customShader = cgs.media.whiteShader; //trap->R_RegisterShaderNoMip("textures/colombia/canvas_doublesided");

	re->shaderTexCoord[0] = re->shaderTexCoord[1] = 1.0f;

	if (color==0)
	{
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 0xff;
	}
	else
	{
		//color = CGDEBUG_SaberColor( color );
		re->shaderRGBA[0] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[1] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[2] = color & 0xff;
//		color >>= 8;
//		re->shaderRGBA[3] = color & 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	le->color[3] = 1.0;

	//re->renderfx |= RF_DEPTHHACK;
}
#endif

static void CG_AddStrafeTrails(void) {
	int i, time;

	if (!cg_strafeTrailPlayers.integer)
		return;
	if (cg_strafeTrailLife.value <= 0.0f)
		return;
	if (cgs.restricts & RESTRICT_STRAFETRAIL)
		return;

	time = cg_strafeTrailLife.value * 1000;
	if (time <= 0)
		time = 1;
	else if (time > 60*1000*60)
		time = 60*60;

	for (i=0; i<MAX_CLIENTS; i++) {
		if (cg_strafeTrailPlayers.integer & (1<<i)) {
			centity_t *player = &cg_entities[i];
			vec3_t diff = {0};

			if (CG_IsMindTricked(player->currentState.trickedentindex,
				player->currentState.trickedentindex2,
				player->currentState.trickedentindex3,
				player->currentState.trickedentindex4,
				i))
				continue;

			if (cg_strafeTrailRacersOnly.integer && (!player->playerState->stats[STAT_RACEMODE] || !player->playerState->duelTime))
			{
				continue;
			}

			VectorSubtract(player->lerpOrigin, player->lastOrigin, diff);
			if (VectorLengthSquared(diff) < 192*192) {
				CG_StrafeTrailLine( player->lerpOrigin, player->lastOrigin, time, i, 0 );
			}
			VectorCopy( player->lerpOrigin, player->lastOrigin );
		}
	}
}

#if _LOGTRAILS
static void CG_LogStrafeTrail(void) {
	if (Q_stricmp("0", cg_logStrafeTrail.string) && cg.snap->ps.stats[11] && cg.snap->ps.duelTime) {
		centity_t *player = &cg_entities[cg.snap->ps.clientNum];
		vec3_t diff;
		char *str;
		char input[MAX_QPATH], fileName[MAX_QPATH];

		//Com_Printf("logging1 to %s\n", cg_logStrafeTrail.string);
	
		//VectorSubtract(player->lerpOrigin, player->lastOrigin, diff);
		//if (VectorLengthSquared(diff) < 192*192) {

		//store last position, if its equal then dont do this shit.
		//store some huge string and just append to it here, or use buffered..?

		if (!VectorCompare(player->lastOrigin, player->lerpOrigin) && VectorLengthSquared(diff) < 192*192) {


			Q_strncpyz( input, cg_logStrafeTrail.string, sizeof(input) );

			Q_strstrip( input, "\n\r;:?*<>|\"\\/ ", NULL );
			Q_CleanStr( input );
			Q_strlwr(fileName);//dat linux

			Com_sprintf(fileName, sizeof(fileName), "strafetrails/%s.cfg", input);
			//Q_strcat(fileName, sizeof(fileName), ".cfg");

			if (!cg.loggingStrafeTrail) {
				trap->FS_Open(fileName, &cg.strafeTrailFileHandle, FS_APPEND);
			}
			else { //Filename exists
				if (Q_stricmp(cg.logStrafeTrailFilename, fileName)) { //If we changed filename.. close old file?
					trap->FS_Close(cg.strafeTrailFileHandle);
				}
			}

			Q_strncpyz(cg.logStrafeTrailFilename, fileName, sizeof(cg.logStrafeTrailFilename));
	
			//Com_Printf("logging2 to %s\n", cg.japro.logStrafeTrailFilename);

			str = va("%i %i %i\n", (int)cg.snap->ps.origin[0], (int)cg.snap->ps.origin[1], (int)cg.snap->ps.origin[2]);

			if (cg.strafeTrailFileHandle) {
				trap->FS_Write(str, strlen(str), cg.strafeTrailFileHandle);
			}
			cg.loggingStrafeTrail  = qtrue;
		}

		VectorCopy( player->lerpOrigin, player->lastOrigin );
		return;
	}	
	
	if (cg.loggingStrafeTrail && (!Q_stricmp("0", cg_logStrafeTrail.string) || (cg.snap->ps.stats[11] && !cg.snap->ps.duelTime)))
	{
		//Com_Printf("closing to %s\n", cg.japro.logStrafeTrailFilename);
		trap->FS_Close(cg.strafeTrailFileHandle);
		cg.loggingStrafeTrail = qfalse;
	}
	
	//}
}
#endif

static QINLINE void CG_LastWeapon(void) { //Called by CG_SetNextSnap, dunno if need to use snap or can use predicted..
	if (!cg.snap)
		return;
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
		return;
	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
		return;

	if (!cg.lastWeaponSelect[0])
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon;
	if (!cg.lastWeaponSelect[1])
		cg.lastWeaponSelect[1] = cg.predictedPlayerState.weapon;

	if (cg.lastWeaponSelect[0] != cg.predictedPlayerState.weapon) { //Current does not match selected
		cg.lastWeaponSelect[1] = cg.lastWeaponSelect[0]; //Set last to current
		cg.lastWeaponSelect[0] = cg.predictedPlayerState.weapon; //Set current to selected
	}

}
