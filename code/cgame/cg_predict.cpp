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

// cg_predict.c -- this file generates cg.predicted_player_state by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"

#include "../game/g_vehicles.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) 
{
	int			i;
	centity_t	*cent;
	vec3_t		difference;
	float		dsquared;

	cg_numSolidEntities = 0;

	if(!cg.snap)
	{
		return;
	}

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) 
	{
		if ( cg.snap->entities[ i ].number < ENTITYNUM_WORLD )
		{
			cent = &cg_entities[ cg.snap->entities[ i ].number ];

			if ( cent->gent != NULL && cent->gent->s.solid ) 
			{
				cg_solidEntities[cg_numSolidEntities] = cent;
				cg_numSolidEntities++;
			}
		}
	}

	dsquared = 5000+500;
	dsquared *= dsquared;

	for(i=0;i<cg_numpermanents;i++)
	{
		cent = cg_permanents[i];
		VectorSubtract(cent->lerpOrigin, cg.snap->ps.origin, difference);
		if (cent->currentState.eType == ET_TERRAIN ||
			((difference[0]*difference[0]) + (difference[1]*difference[1]) + (difference[2]*difference[2])) <= dsquared)
		{
			cent->currentValid = qtrue;
			if ( cent->nextState && cent->nextState->solid ) 
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

/*
====================
CG_ClipMoveToEntities

====================
*/
void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->eType == ET_PUSH_TRIGGER ) {
			continue;
		}
		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = cgi_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );

			//Hmm... this would cause traces against brush movers to snap at 20fps (as with the third person camera)... 
			//Let's use the lerpOrigin for now and see if it breaks anything...
			//EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, origin );
			VectorCopy( cent->lerpOrigin, origin );

		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = cgi_CM_TempBoxModel( bmins, bmaxs );//, cent->gent->contents );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		cgi_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 const int skipNumber, const int mask, const EG2_Collision eG2TraceType/*=G2_NOCOLLIDE*/, const int useLod/*=0*/) {
	trace_t	t;

	cgi_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/

#define USE_SV_PNT_CONTENTS (1)

#if USE_SV_PNT_CONTENTS
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	return gi.pointcontents(point,passEntityNum );
}
#else
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = cgi_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = cgi_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= cgi_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}
#endif

void CG_SetClientViewAngles( vec3_t angles, qboolean overrideViewEnt )
{
	if ( cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD || overrideViewEnt )
	{//don't clamp angles when looking through a viewEntity
		for( int i = 0; i < 3; i++ ) 
		{
			cg.predicted_player_state.viewangles[i] = angles[i];
			cg.predicted_player_state.delta_angles[i] = 0;
			cg.snap->ps.viewangles[i] = angles[i];
			cg.snap->ps.delta_angles[i] = 0;
			g_entities[0].client->pers.cmd_angles[i] = ANGLE2SHORT(angles[i]);
		}
		cgi_SetUserCmdAngles( angles[PITCH], angles[YAW], angles[ROLL] );
	}
}

extern qboolean PM_AdjustAnglesToGripper( gentity_t *gent, usercmd_t *cmd );
extern qboolean PM_AdjustAnglesForSpinningFlip( gentity_t *ent, usercmd_t *ucmd, qboolean anglesOnly );
extern qboolean G_CheckClampUcmd( gentity_t *ent, usercmd_t *ucmd );
extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
qboolean CG_CheckModifyUCmd( usercmd_t *cmd, vec3_t viewangles )
{
	qboolean overridAngles = qfalse;
	if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{//controlling something else
		memset( cmd, 0, sizeof( usercmd_t ) );
		/*
		//to keep pointing in same dir, need to set cmd.angles
		cmd->angles[PITCH] = ANGLE2SHORT( cg.snap->ps.viewangles[PITCH] ) - cg.snap->ps.delta_angles[PITCH];
		cmd->angles[YAW] = ANGLE2SHORT( cg.snap->ps.viewangles[YAW] ) - cg.snap->ps.delta_angles[YAW];
		cmd->angles[ROLL] = 0;
		*/
		VectorCopy( g_entities[0].pos4, viewangles );
		overridAngles = qtrue;
		//CG_SetClientViewAngles( g_entities[cg.snap->ps.viewEntity].client->ps.viewangles, qtrue );
	}
	else if ( G_IsRidingVehicle( &g_entities[0] ) )
	{
		overridAngles = qtrue;
		/*
		int vehIndex = g_entities[0].owner->client->ps.vehicleIndex;
		if ( vehIndex != VEHICLE_NONE 
			&& (vehicleData[vehIndex].type == VH_FIGHTER || (vehicleData[vehIndex].type == VH_SPEEDER )) )
		{//in vehicle flight mode
			float speed = VectorLength( cg.snap->ps.velocity );
			if ( !speed || cg.snap->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				cmd->rightmove = 0;
				cmd->angles[PITCH] = 0;
				cmd->angles[YAW] = ANGLE2SHORT( cg.snap->ps.viewangles[YAW] ) - cg.snap->ps.delta_angles[YAW];
				CG_SetClientViewAngles( cg.snap->ps.viewangles, qfalse );
			}
		}
		*/
	}

	if ( &g_entities[0] && g_entities[0].client )
	{
		if ( !PM_AdjustAnglesToGripper( &g_entities[0], cmd ) )
		{
			if ( PM_AdjustAnglesForSpinningFlip( &g_entities[0], cmd, qtrue ) )
			{
				CG_SetClientViewAngles( g_entities[0].client->ps.viewangles, qfalse );
				if ( viewangles )
				{
					VectorCopy( g_entities[0].client->ps.viewangles, viewangles );
					overridAngles = qtrue;
				}
			}
		}
		else
		{
			CG_SetClientViewAngles( g_entities[0].client->ps.viewangles, qfalse );
			if ( viewangles )
			{
				VectorCopy( g_entities[0].client->ps.viewangles, viewangles );
				overridAngles = qtrue;
			}
		}
		if ( G_CheckClampUcmd( &g_entities[0], cmd ) )
		{
			CG_SetClientViewAngles( g_entities[0].client->ps.viewangles, qfalse );
			if ( viewangles )
			{
				VectorCopy( g_entities[0].client->ps.viewangles, viewangles );
				overridAngles = qtrue;
			}
		}
	}
	return overridAngles;
}

qboolean CG_OnMovingPlat( playerState_t *ps )
{
	if ( ps->groundEntityNum != ENTITYNUM_NONE )
	{
		entityState_t *es = &cg_entities[ps->groundEntityNum].currentState;
		if ( es->eType == ET_MOVER )
		{//on a mover
			if ( es->pos.trType != TR_STATIONARY )
			{
				if ( es->pos.trType != TR_LINEAR_STOP && es->pos.trType != TR_NONLINEAR_STOP )
				{//a constant mover
					if ( !VectorCompare( vec3_origin, es->pos.trDelta ) )
					{//is moving
						return qtrue;
					}
				}
				else
				{//a linear-stop mover
					if ( es->pos.trTime+es->pos.trDuration > cg.time )
					{//still moving
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}
/*
========================
CG_InterpolatePlayerState

Generates cg.predicted_player_state by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;
	qboolean		skip = qfalse;
	vec3_t			oldOrg;

	out = &cg.predicted_player_state;
	prev = cg.snap;
	next = cg.nextSnap;

	VectorCopy(out->origin,oldOrg);
	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = cgi_GetCurrentCmdNumber();
		cgi_GetUserCmd( cmdNum, &cmd );

		skip = CG_CheckModifyUCmd( &cmd, out->viewangles );

		if ( !skip )
		{
			//NULL so that it doesn't execute a block of code that must be run from game
			PM_UpdateViewAngles( out, &cmd, NULL );
		}
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) 
	{
		return;
	}

	if (!( !next || next->serverTime <= prev->serverTime ) )
	{

		f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );
		
		i = next->ps.bobCycle;
		if ( i < prev->ps.bobCycle ) 
		{
			i += 256;		// handle wraparound
		}
		out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

		for ( i = 0 ; i < 3 ; i++ ) 
		{
			out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i] );
			if ( !grabAngles ) 
			{
				out->viewangles[i] = LerpAngle( 
					prev->ps.viewangles[i], next->ps.viewangles[i], f );
			}
			out->velocity[i] = prev->ps.velocity[i] + 
				f * (next->ps.velocity[i] - prev->ps.velocity[i] );
		}
	}

	bool onPlat=false;
	centity_t	*pent=0;
	if (out->groundEntityNum>0)
	{
		pent=&cg_entities[out->groundEntityNum];
		if (pent->currentState.eType == ET_MOVER ) 

		{
			onPlat=true;
		}
	}

	if (
		cg.validPPS && 
		cg_smoothPlayerPos.value>0.0f && 
		cg_smoothPlayerPos.value<1.0f &&
		!onPlat
		)
	{
		// 0 = no smoothing, 1 = no movement
		for (i=0;i<3;i++)
		{
			out->origin[i]=cg_smoothPlayerPos.value*(oldOrg[i]-out->origin[i])+out->origin[i];
		}
	}
	else if (onPlat&&cg_smoothPlayerPlat.value>0.0f&&cg_smoothPlayerPlat.value<1.0f)
	{
//		if (cg.frametime<150)
//		{
		assert(pent);
		vec3_t	p1,p2,vel;
		float lerpTime;


		EvaluateTrajectory( &pent->currentState.pos,cg.snap->serverTime, p1 );
		if ( cg.nextSnap &&cg.nextSnap->serverTime > cg.snap->serverTime && pent->nextState) 
		{
			EvaluateTrajectory( &pent->nextState->pos,cg.nextSnap->serverTime, p2 );
			lerpTime=float(cg.nextSnap->serverTime - cg.snap->serverTime);
		}
		else
		{
			EvaluateTrajectory( &pent->currentState.pos,cg.snap->serverTime+50, p2 );
			lerpTime=50.0f;
		}

		float accel=cg_smoothPlayerPlatAccel.value*cg.frametime/lerpTime;

		if (accel>20.0f)
		{
			accel=20.0f;
		}

		for (i=0;i<3;i++)
		{
			vel[i]=accel*(p2[i]-p1[i]);
		}

		VectorAdd(out->origin,vel,out->origin);

		if (cg.validPPS && 
			cg_smoothPlayerPlat.value>0.0f && 
			cg_smoothPlayerPlat.value<1.0f
			)
		{
			// 0 = no smoothing, 1 = no movement
			for (i=0;i<3;i++)
			{
				out->origin[i]=cg_smoothPlayerPlat.value*(oldOrg[i]-out->origin[i])+out->origin[i];
			}
		}
//		}
	}
}

/*
===================
CG_TouchItem
===================
*/
void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_PlayerTouchesItem( &cg.predicted_player_state, &cent->currentState, cg.time ) ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( &cent->currentState, &cg.predicted_player_state ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// grab it
	AddEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex , &cg.predicted_player_state);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		int ammotype = weaponData[item->giTag].ammoIndex;
		cg.predicted_player_state.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if ( !cg.predicted_player_state.ammo[ ammotype] ) {
			cg.predicted_player_state.ammo[ ammotype ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
Only called for the last command
=========================
*/
void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predicted_player_state.pm_type == PM_SPECTATOR );

	if ( cg.predicted_player_state.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->eType != ET_PUSH_TRIGGER && ent->eType != ET_TELEPORT_TRIGGER ) {
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = cgi_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		cgi_CM_BoxTrace( &trace, cg.predicted_player_state.origin, cg.predicted_player_state.origin, 
			cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else {
			// we hit this push trigger
			if ( spectator ) {
				continue;
			}

			VectorCopy( ent->origin2, cg.predicted_player_state.velocity );
		}
	}
}


/*
=================
CG_PredictPlayerState

Generates cg.predicted_player_state for the current cg.time
cg.predicted_player_state is guaranteed to be valid after exiting.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new refdef will usually have exactly one new usercmd over the last,
but we have to simulate all unacknowledged commands since the last snapshot
received.  This means that on an internet connection, quite a few
pmoves may be issued each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
extern	qboolean	player_locked;
void CG_PredictPlayerState( void ) {
	int			cmdNum, current;
	playerState_t	oldPlayerState;

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predicted_player_state is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predicted_player_state = cg.snap->ps;
	}


	if ( 1 )//cg_timescale.value >= 1.0f )
	{
		// demo playback just copies the moves
		/*
		if ( (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
			CG_InterpolatePlayerState( qfalse );
			return;
		}
		*/

		// non-predicting local movement will grab the latest angles
		CG_InterpolatePlayerState( qtrue );
		return;
	}

	// prepare for pmove
	//FIXME: is this bad???
	cg_pmove.gent = NULL;
	cg_pmove.ps = &cg.predicted_player_state;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	cg_pmove.tracemask = MASK_PLAYERSOLID;
	cg_pmove.noFootsteps = 0;//( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predicted_player_state;

	// if we are too far out of date, just freeze
	cmdNum = cg.snap->cmdNum;
	current = cgi_GetCurrentCmdNumber();

	if ( current - cmdNum >= CMD_BACKUP ) {
		return;	
	}

	// get the most recent information we have
	cg.predicted_player_state = cg.snap->ps;

	// we should always be predicting at least one frame
	if ( cmdNum >= current )	{
		return;	
	}

	// run cmds
	do {
		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cmdNum == current - 1 ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predicted_player_state.origin, 
					cg.predicted_player_state.groundEntityNum, cg.oldTime, adjusted );

				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
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

		// if the command can't be gotten because it is
		// too far out of date, the frame is invalid
		// this should never happen, because we check ranges at
		// the top of the function
		cmdNum++;
		if ( !cgi_GetUserCmd( cmdNum, &cg_pmove.cmd ) ) {
			break;
		}

		gentity_t *ent = &g_entities[0];//cheating and dirty, I know, but this is a SP game so prediction can cheat
		if ( player_locked ||
			(ent && !ent->s.number&&ent->aimDebounceTime>level.time) ||
			(ent && ent->client && ent->client->ps.pm_time && (ent->client->ps.pm_flags&PMF_TIME_KNOCKBACK)) ||
			(ent && ent->forcePushTime > level.time) ) 
		{//lock out player control unless dead
			//VectorClear( cg_pmove.cmd.angles );
			cg_pmove.cmd.forwardmove = 0;
			cg_pmove.cmd.rightmove = 0;
			cg_pmove.cmd.buttons = 0;
			cg_pmove.cmd.upmove = 0;
		}
		CG_CheckModifyUCmd( &cg_pmove.cmd, NULL );
		//FIXME: prediction on clients in timescale results in jerky positional translation
		Pmove( &cg_pmove );
		
		// add push trigger movement effects
		CG_TouchTriggerPrediction();

	} while ( cmdNum < current );

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predicted_player_state.origin, 
		cg.predicted_player_state.groundEntityNum, 
		cg.time, cg.predicted_player_state.origin );

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predicted_player_state, &oldPlayerState );
}


