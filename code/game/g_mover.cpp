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

#include "bg_public.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"
#include "objectives.h"
#include "g_local.h"

#include "../icarus/IcarusInterface.h"


int	BMS_START = 0;
int	BMS_MID = 1;
int	BMS_END = 2;

extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
void CalcTeamDoorCenter ( gentity_t *ent, vec3_t center );
void InitMover( gentity_t *ent ) ;

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void MatchTeam( gentity_t *teamLeader, int moverState, int time );
extern qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);

typedef struct {
	gentity_t	*ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
} pushed_t;
pushed_t	pushed[MAX_GENTITIES], *pushed_p;


/*
-------------------------
G_SetLoopSound
-------------------------
*/

void G_PlayDoorLoopSound( gentity_t *ent )
{
	if ( VALIDSTRING( ent->soundSet ) == false )
		return;

	sfxHandle_t	sfx = CAS_GetBModelSound( ent->soundSet, BMS_MID );

	if ( sfx == -1 )
	{
		ent->s.loopSound = 0;
		return;
	}

	ent->s.loopSound = sfx;
}

/*
-------------------------
G_PlayDoorSound
-------------------------
*/

void G_PlayDoorSound( gentity_t *ent, int type )
{
	if ( VALIDSTRING( ent->soundSet ) == false )
		return;

	sfxHandle_t	sfx = CAS_GetBModelSound( ent->soundSet, type );

	if ( sfx == -1 )
		return;

	vec3_t	doorcenter;
	CalcTeamDoorCenter( ent, doorcenter );
	if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
	{
		AddSoundEvent( ent->activator, doorcenter, 128, AEL_MINOR, qfalse, qtrue );
	}

	G_AddEvent( ent, EV_BMODEL_SOUND, sfx );
}

/*
============
G_TestEntityPosition

============
*/
gentity_t	*G_TestEntityPosition( gentity_t *ent ) {
	trace_t	tr;
	int		mask;

	if ( (ent->client && ent->health <= 0) || !ent->clipmask )
	{//corpse or something with no clipmask
		mask = MASK_SOLID;
	}
	else
	{
		mask = ent->clipmask;
	}
	if ( ent->client )
	{
		gi.trace( &tr, ent->client->ps.origin, ent->mins, ent->maxs, ent->client->ps.origin, ent->s.number, mask, (EG2_Collision)0, 0 );
	}
	else
	{
		if ( ent->s.eFlags & EF_MISSILE_STICK )//Arggh, this is dumb...but when it used the bbox, it was pretty much always in solid when it is riding something..which is wrong..so I changed it to basically be a point contents check
		{
			gi.trace( &tr, ent->s.pos.trBase, vec3_origin, vec3_origin, ent->s.pos.trBase, ent->s.number, mask, (EG2_Collision)0, 0 );
		}
		else
		{
			gi.trace( &tr, ent->s.pos.trBase, ent->mins, ent->maxs, ent->s.pos.trBase, ent->s.number, mask, (EG2_Collision)0, 0 );
		}
	}

	if (tr.startsolid)
		return &g_entities[ tr.entityNum ];

	return NULL;
}


/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
extern qboolean G_OkayToRemoveCorpse( gentity_t *self );

qboolean	G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {
	vec3_t		forward, right, up;
	vec3_t		org, org2, move2;
	gentity_t	*block;

	/*
	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
		check->s.groundEntityNum != pusher->s.number ) {
		return qfalse;
	}
	*/

	// save off the old position
	if (pushed_p > &pushed[MAX_GENTITIES]) {
		G_Error( "pushed_p > &pushed[MAX_GENTITIES]" );
	}
	pushed_p->ent = check;
	VectorCopy (check->s.pos.trBase, pushed_p->origin);
	VectorCopy (check->s.apos.trBase, pushed_p->angles);
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy (check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;

	// we need this for pushing things later
	VectorSubtract (vec3_origin, amove, org);
	AngleVectors (org, forward, right, up);

	// try moving the contacted entity
	VectorAdd (check->s.pos.trBase, move, check->s.pos.trBase);
	if (check->client) {
		// make sure the client's view rotates when on a rotating mover
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
	}

	// figure movement due to the pusher's amove
	VectorSubtract (check->s.pos.trBase, pusher->currentOrigin, org);
	org2[0] = DotProduct (org, forward);
	org2[1] = -DotProduct (org, right);
	org2[2] = DotProduct (org, up);
	VectorSubtract (org2, org, move2);
	VectorAdd (check->s.pos.trBase, move2, check->s.pos.trBase);
	if ( check->client ) {
		VectorAdd (check->client->ps.origin, move, check->client->ps.origin);
		VectorAdd (check->client->ps.origin, move2, check->client->ps.origin);
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
	}

	/*
	if ( check->client && check->health <= 0 && check->contents == CONTENTS_CORPSE )
	{//sigh... allow pushing corpses into walls... problem is, they'll be stuck in there... maybe just remove them?
		return qtrue;
	}
	*/
	block = G_TestEntityPosition( check );
	if (!block) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->currentOrigin );
		} else {
			VectorCopy( check->s.pos.trBase, check->currentOrigin );
		}
		gi.linkentity (check);
		return qtrue;
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( (pushed_p-1)->origin, check->s.pos.trBase);
	if ( check->client ) {
		VectorCopy( (pushed_p-1)->origin, check->client->ps.origin);
	}
	VectorCopy( (pushed_p-1)->angles, check->s.apos.trBase );
	block = G_TestEntityPosition (check);
	if ( !block ) {
		check->s.groundEntityNum = ENTITYNUM_NONE;
		pushed_p--;
		return qtrue;
	}

	// blocked
	if ( pusher->damage )
	{//Do damage
		if ( (pusher->spawnflags&MOVER_CRUSHER)//a crusher
 			&& check->s.clientNum >= MAX_CLIENTS//not the player
			&& check->client //NPC
			&& check->health <= 0 //dead
			&& G_OkayToRemoveCorpse( check ) )//okay to remove him
		{//crusher stuck on a non->player corpse that does not have a key and is not running a script
			G_FreeEntity( check );
		}
		else
		{
			G_Damage(check, pusher, pusher->activator, move, check->currentOrigin, pusher->damage, 0, MOD_CRUSH );
		}
	}

	return qfalse;
}


/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	qboolean	notMoving;
	int			i, e;
	int			listedEntities;
	vec3_t		mins, maxs;
	vec3_t		pusherMins, pusherMaxs, totalMins, totalMaxs;
	pushed_t	*p;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*check;

	*obstacle = NULL;


	if ( !pusher->bmodel )
	{//misc_model_breakable
		VectorAdd( pusher->currentOrigin, pusher->mins, pusherMins );
		VectorAdd( pusher->currentOrigin, pusher->maxs, pusherMaxs );
	}

	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->currentAngles[0] || pusher->currentAngles[1] || pusher->currentAngles[2]
		|| amove[0] || amove[1] || amove[2] )
	{
		float		radius;

		radius = RadiusFromBounds( pusher->mins, pusher->maxs );
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = pusher->currentOrigin[i] + move[i] - radius;
			maxs[i] = pusher->currentOrigin[i] + move[i] + radius;
			totalMins[i] = mins[i] - move[i];
			totalMaxs[i] = maxs[i] - move[i];
		}
	}
	else
	{
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = pusher->absmin[i] + move[i];
			maxs[i] = pusher->absmax[i] + move[i];
		}

		VectorCopy( pusher->absmin, totalMins );
		VectorCopy( pusher->absmax, totalMaxs );
		for (i=0 ; i<3 ; i++)
		{
			if ( move[i] > 0 )
			{
				totalMaxs[i] += move[i];
			}
			else
			{
				totalMins[i] += move[i];
			}
		}
	}

	// unlink the pusher so we don't get it in the entityList
	gi.unlinkentity( pusher );

	listedEntities = gi.EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->currentOrigin, move, pusher->currentOrigin );
	VectorAdd( pusher->currentAngles, amove, pusher->currentAngles );
	gi.linkentity( pusher );

	notMoving = (qboolean)(VectorCompare( vec3_origin, move )&&VectorCompare( vec3_origin, amove ));

	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = entityList[ e ];

		if (( check->s.eFlags & EF_MISSILE_STICK ) && (notMoving || check->s.groundEntityNum < 0 || check->s.groundEntityNum >= ENTITYNUM_NONE ))
		{
			// special case hack for sticky things, destroy it if we aren't attached to the thing that is moving, but the moving thing is pushing us
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		// only push items and players
		if ( check->s.eType != ET_ITEM )
		{
			//FIXME: however it allows items to be pushed through stuff, do same for corpses?
			if ( check->s.eType != ET_PLAYER )
			{
				if ( !( check->s.eFlags & EF_MISSILE_STICK ))
				{
					// cannot be pushed by this mover
					continue;
				}
			}
			/*
			else if ( check->health <= 0 )
			{//For now, don't push on dead players
				continue;
			}
			*/
			else if ( !pusher->bmodel )
			{
				vec3_t	checkMins, checkMaxs;

				VectorAdd( check->currentOrigin, check->mins, checkMins );
				VectorAdd( check->currentOrigin, check->maxs, checkMaxs );

				if ( G_BoundsOverlap( checkMins, checkMaxs, pusherMins, pusherMaxs ) )
				{//They're inside me already, no push - FIXME: we're testing a moves spot, aren't we, so we could have just moved inside them?
					continue;
				}
			}
		}


		if ( check->maxs[0] - check->mins[0] <= 0 &&
				check->maxs[1] - check->mins[1] <= 0 &&
				check->maxs[2] - check->mins[2] <= 0 )
		{//no size, don't push
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->absmin[0] >= maxs[0]
			|| check->absmin[1] >= maxs[1]
			|| check->absmin[2] >= maxs[2]
			|| check->absmax[0] <= mins[0]
			|| check->absmax[1] <= mins[1]
			|| check->absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if ( G_TestEntityPosition( check ) != pusher )
			{
				continue;
			}
		}

		if ( ((pusher->spawnflags&2)&&!Q_stricmp("func_breakable",pusher->classname))
			||((pusher->spawnflags&16)&&!Q_stricmp("func_static",pusher->classname)) )
		{//ugh, avoid stricmp with a unique flag
			//Damage on impact
			if ( pusher->damage )
			{//Do damage
				G_Damage( check, pusher, pusher->activator, move, check->currentOrigin, pusher->damage, 0, MOD_CRUSH );
				if ( pusher->health >= 0 && pusher->takedamage && !(pusher->spawnflags&1) )
				{//do some damage to me, too
					G_Damage( pusher, check, pusher->activator, move, pusher->s.pos.trBase, floor(pusher->damage/4.0f), 0, MOD_CRUSH );
				}
			}
		}
		// really need a flag like MOVER_TOUCH that calls the ent's touch function here, instead of this stricmp crap
		else if ( (pusher->spawnflags&2) && !Q_stricmp( "func_rotating", pusher->classname ) )
		{
			GEntity_TouchFunc( pusher, check, NULL );
			continue;	// don't want it blocking so skip past it
		}

		vec3_t oldOrg;

		VectorCopy( check->s.pos.trBase, oldOrg );

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) )
		{
			// the mover wasn't blocked
			if ( check->s.eFlags & EF_MISSILE_STICK )
			{
				if ( !VectorCompare( oldOrg, check->s.pos.trBase ))
				{
					// and the rider was actually pushed, so interpolate position change to smooth out the ride
					check->s.pos.trType = TR_INTERPOLATE;
					continue;
				}
				//else..the mover wasn't blocked & we are riding the mover & but the rider did not move even though the mover itself did...so
				//	drop through and let it blow up the rider up
			}
			else
			{
				continue;
			}
		}

		// the move was blocked even after pushing this rider
		if ( check->s.eFlags & EF_MISSILE_STICK )
		{
			// so nuke 'em so they don't block us anymore
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}

		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p=pushed_p-1 ; p>=pushed ; p-- ) {
			VectorCopy (p->origin, p->ent->s.pos.trBase);
			VectorCopy (p->angles, p->ent->s.apos.trBase);
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy (p->origin, p->ent->client->ps.origin);
			}
			gi.linkentity (p->ent);
		}
		return qfalse;
	}

	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( gentity_t *ent ) {
	vec3_t		move, amove;
	gentity_t	*part, *obstacle;
	vec3_t		origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for (part = ent ; part ; part=part->teamchain)
	{
		// get current position
		part->s.eFlags &= ~EF_BLOCKED_MOVER;
		EvaluateTrajectory( &part->s.pos, level.time, origin );
		EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->currentOrigin, move );
		VectorSubtract( angles, part->currentAngles, amove );
		if ( !G_MoverPush( part, move, amove, &obstacle ) )
		{
			break;	// move was blocked
		}
	}

	if (part)
	{
		// if the pusher has a "blocked" function, call it
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain )
		{
			//Push up time so it doesn't wiggle when blocked
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			EvaluateTrajectory( &part->s.pos, level.time, part->currentOrigin );
			EvaluateTrajectory( &part->s.apos, level.time, part->currentAngles );
			gi.linkentity( part );
			part->s.eFlags |= EF_BLOCKED_MOVER;
		}

		if ( ent->e_BlockedFunc != blockedF_NULL )
		{// this check no longer necessary, done internally below, but it's here for reference
			GEntity_BlockedFunc( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain )
	{
		// call the reached function if time is at or past end point
		if ( part->s.pos.trType == TR_LINEAR_STOP ||
			part->s.pos.trType == TR_NONLINEAR_STOP )
		{
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration )
			{
				GEntity_ReachedFunc( part );
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

/*	if ( ent->classname && Q_stricmp("misc_turret", ent->classname ) == 0 )
	{
		rebolt_turret( ent );
	}
*/
	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
CalcTeamDoorCenter

Finds all the doors of a team and returns their center position
*/

void CalcTeamDoorCenter ( gentity_t *ent, vec3_t center )
{
	vec3_t		slavecenter;
	gentity_t	*slave;

	//Start with our center
	VectorAdd(ent->mins, ent->maxs, center);
	VectorScale(center, 0.5, center);
	for ( slave = ent->teamchain ; slave ; slave = slave->teamchain )
	{
		//Find slave's center
		VectorAdd(slave->mins, slave->maxs, slavecenter);
		VectorScale(slavecenter, 0.5, slavecenter);
		//Add that to our own, find middle
		VectorAdd(center, slavecenter, center);
		VectorScale(center, 0.5, center);
	}
}

/*
===============
SetMoverState
===============
*/
void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vec3_t			delta;
	float			f;

	ent->moverState = moverState;

	ent->s.pos.trTime = time;

	if ( ent->s.pos.trDuration <= 0 )
	{//Don't allow divide by zero!
		ent->s.pos.trDuration = 1;
	}

	switch( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		if ( ent->alt_fire )
		{
			ent->s.pos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.pos.trType = TR_NONLINEAR_STOP;
		}
		ent->s.eFlags &= ~EF_BLOCKED_MOVER;
		break;
	case MOVER_2TO1:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		if ( ent->alt_fire )
		{
			ent->s.pos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.pos.trType = TR_NONLINEAR_STOP;
		}
		ent->s.eFlags &= ~EF_BLOCKED_MOVER;
		break;
	}
	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
	gi.linkentity( ent );
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t		*slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		SetMoverState( slave, (moverState_t) moverState, time );
	}
}



/*
================
ReturnToPos1
================
*/
void ReturnToPos1( gentity_t *ent ) {
	ent->e_ThinkFunc = thinkF_NULL;
	ent->nextthink = 0;
	ent->s.time = level.time;

	MatchTeam( ent, MOVER_2TO1, level.time );

	// starting sound
	G_PlayDoorLoopSound( ent );
	G_PlayDoorSound( ent, BMS_START );	//??
}


/*
================
Reached_BinaryMover
================
*/

void Reached_BinaryMover( gentity_t *ent )
{
	// stop the looping sound
	ent->s.loopSound = 0;

	if ( ent->moverState == MOVER_1TO2 )
	{//reached open
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}

		// play sound
		G_PlayDoorSound( ent, BMS_END );

		if ( ent->wait < 0 )
		{//Done for good
			ent->e_ThinkFunc = thinkF_NULL;
			ent->nextthink = -1;
			ent->e_UseFunc = useF_NULL;
		}
		else
		{
			// return to pos1 after a delay
			ent->e_ThinkFunc = thinkF_ReturnToPos1;
			if(ent->spawnflags & 8)
			{//Toggle, keep think, wait for next use?
				ent->nextthink = -1;
			}
			else
			{
				ent->nextthink = level.time + ent->wait;
			}
		}

		// fire targets
		if ( !ent->activator )
		{
			ent->activator = ent;
		}
		G_UseTargets2( ent, ent->activator, ent->opentarget );
	}
	else if ( ent->moverState == MOVER_2TO1 )
	{//closed
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}
		// play sound
		G_PlayDoorSound( ent, BMS_END );

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster )
		{
			gi.AdjustAreaPortalState( ent, qfalse );
		}
		G_UseTargets2( ent, ent->activator, ent->closetarget );
	}
	else
	{
		G_Error( "Reached_BinaryMover: bad moverState" );
	}
}


/*
================
Use_BinaryMover_Go
================
*/
void Use_BinaryMover_Go( gentity_t *ent )
{
	int		total;
	int		partial;
//	gentity_t	*other = ent->enemy;
	gentity_t	*activator = ent->activator;

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 )
	{
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		vec3_t	doorcenter;
		CalcTeamDoorCenter( ent, doorcenter );
		if ( ent->activator && ent->activator->client && ent->activator->client->playerTeam == TEAM_PLAYER )
		{
			AddSightEvent( ent->activator, doorcenter, 256, AEL_MINOR, 1 );
		}

		// starting sound
		G_PlayDoorLoopSound( ent );
		G_PlayDoorSound( ent, BMS_START );
		ent->s.time = level.time;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			gi.AdjustAreaPortalState( ent, qtrue );
		}
		G_UseTargets( ent, ent->activator );
		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2 ) {
		//have to do this because the delay sets our think to Use_BinaryMover_Go
		ent->e_ThinkFunc = thinkF_ReturnToPos1;
		if ( ent->spawnflags & 8 )
		{//TOGGLE doors don't use wait!
			ent->nextthink = level.time + FRAMETIME;
		}
		else
		{
			ent->nextthink = level.time + ent->wait;
		}
		G_UseTargets2( ent, ent->activator, ent->target2 );
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 )
	{
		total = ent->s.pos.trDuration-50;
		if ( ent->s.pos.trType == TR_NONLINEAR_STOP )
		{
			vec3_t curDelta;
			VectorSubtract( ent->currentOrigin, ent->pos1, curDelta );
			float fPartial = VectorLength( curDelta )/VectorLength( ent->s.pos.trDelta );
			VectorScale( ent->s.pos.trDelta, fPartial, curDelta );
			fPartial /= ent->s.pos.trDuration;
			fPartial /= 0.001f;
			fPartial = acos( fPartial );
			fPartial = RAD2DEG( fPartial );
			fPartial = (90.0f - fPartial)/90.0f*ent->s.pos.trDuration;
			partial = total - floor( fPartial );
		}
		else
		{
			partial = level.time - ent->s.pos.trTime;//ent->s.time;
		}

		if ( partial > total ) {
			partial = total;
		}
		ent->s.pos.trTime = level.time - ( total - partial );//ent->s.time;

		MatchTeam( ent, MOVER_1TO2, ent->s.pos.trTime );

		G_PlayDoorSound( ent, BMS_START );

		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 )
	{
		total = ent->s.pos.trDuration-50;
		if ( ent->s.pos.trType == TR_NONLINEAR_STOP )
		{
			vec3_t curDelta;
			VectorSubtract( ent->currentOrigin, ent->pos2, curDelta );
			float fPartial = VectorLength( curDelta )/VectorLength( ent->s.pos.trDelta );
			VectorScale( ent->s.pos.trDelta, fPartial, curDelta );
			fPartial /= ent->s.pos.trDuration;
			fPartial /= 0.001f;
			fPartial = acos( fPartial );
			fPartial = RAD2DEG( fPartial );
			fPartial = (90.0f - fPartial)/90.0f*ent->s.pos.trDuration;
			partial = total - floor( fPartial );
		}
		else
		{
			partial = level.time - ent->s.pos.trTime;//ent->s.time;
		}
		if ( partial > total ) {
			partial = total;
		}

		ent->s.pos.trTime = level.time - ( total - partial );//ent->s.time;
		MatchTeam( ent, MOVER_2TO1, ent->s.pos.trTime );

		G_PlayDoorSound( ent, BMS_START );

		return;
	}
}

void UnLockDoors(gentity_t *const ent)
{
	//noise?
	//go through and unlock the door and all the slaves
	gentity_t	*slave = ent;
	do
	{	// want to allow locked toggle doors, so keep the targetname
		if( !(slave->spawnflags & MOVER_TOGGLE) )
		{
			slave->targetname = NULL;//not usable ever again
		}
		slave->spawnflags &= ~MOVER_LOCKED;
		slave->s.frame = 1;//second stage of anim
		slave = slave->teamchain;
	} while  ( slave );
}
void LockDoors(gentity_t *const ent)
{
	//noise?
	//go through and lock the door and all the slaves
	gentity_t	*slave = ent;
	do
	{
		slave->spawnflags |= MOVER_LOCKED;
		slave->s.frame = 0;//first stage of anim
		slave = slave->teamchain;
	} while  ( slave );
}
/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	int	key;
	const char *text;

	if ( ent->e_UseFunc == useF_NULL )
	{//I cannot be used anymore, must be a door with a wait of -1 that's opened.
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE )
	{
		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	if ( ent->svFlags & SVF_INACTIVE )
	{
		return;
	}

	if ( ent->spawnflags & MOVER_LOCKED )
	{//a locked door, unlock it
		UnLockDoors(ent);
		return;
	}

	if ( ent->spawnflags & MOVER_GOODIE )
	{
		if ( ent->fly_sound_debounce_time > level.time )
		{
			return;
		}
		else
		{
			key = INV_GoodieKeyCheck( activator );
			if (key)
			{//activator has a goodie key, remove it
				activator->client->ps.inventory[key]--;
				G_Sound( activator, G_SoundIndex( "sound/movers/goodie_pass.wav" ) );
				// once the goodie mover has been used, it no longer requires a goodie key
				ent->spawnflags &= ~MOVER_GOODIE;
			}
			else
			{	//don't have a goodie key
				G_Sound( activator, G_SoundIndex( "sound/movers/goodie_fail.wav" ) );
				ent->fly_sound_debounce_time = level.time + 5000;
				text = "cp @SP_INGAME_NEED_KEY_TO_OPEN";
				//FIXME: temp message, only on certain difficulties?, graphic instead of text?
				gi.SendServerCommand( 0, text );
				return;
			}
		}
	}

	G_ActivateBehavior(ent,BSET_USE);

	G_SetEnemy( ent, other );
	ent->activator = activator;
	if(ent->delay)
	{
		ent->e_ThinkFunc = thinkF_Use_BinaryMover_Go;
		ent->nextthink = level.time + ent->delay;
	}
	else
	{
		Use_BinaryMover_Go(ent);
	}
}



/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMoverTrData( gentity_t *ent )
{
	vec3_t		move;
	float		distance;

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( ! ent->speed )
	{
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 )
	{
		ent->s.pos.trDuration = 1;
	}
}

void InitMover( gentity_t *ent )
{
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 )
	{
		if ( strstr( ent->model2, ".glm" ))
		{
			ent->s.modelindex2 = G_ModelIndex( ent->model2 );
			ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, ent->model2, ent->s.modelindex2, NULL_HANDLE, NULL_HANDLE, 0, 0 );
			if ( ent->playerModel >= 0 )
			{
				ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
			}

			ent->s.radius = 120;
		}
		else
		{
			ent->s.modelindex2 = G_ModelIndex( ent->model2 );
		}
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->e_UseFunc	   = useF_Use_BinaryMover;
	ent->e_ReachedFunc = reachedF_Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->svFlags = SVF_USE_CURRENT_ORIGIN;
	if ( ent->spawnflags & MOVER_INACTIVE )
	{// Make it inactive
		ent->svFlags |= SVF_INACTIVE;
	}
	if(ent->spawnflags & MOVER_PLAYER_USE)
	{//Can be used by the player's BUTTON_USE
		ent->svFlags |= SVF_PLAYER_USABLE;
	}
	ent->s.eType = ET_MOVER;
	VectorCopy( ent->pos1, ent->currentOrigin );
	gi.linkentity( ent );

	InitMoverTrData( ent );
}


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

/*
================
Blocked_Door
================
*/
void Blocked_Door( gentity_t *ent, gentity_t *other ) {

	// remove anything other than a client -- no longer the case

	// don't remove security keys or goodie keys
	if ( (other->s.eType == ET_ITEM) && (other->item->giTag >= INV_GOODIE_KEY && other->item->giTag <= INV_SECURITY_KEY) )
	{
		// should we be doing anything special if a key blocks it... move it somehow..?
	}
	// if your not a client, or your a dead client remove yourself...
	else if ( other->s.number && (!other->client || (other->client && other->health <= 0 && other->contents == CONTENTS_CORPSE && !other->message)) )
	{
		if ( !IIcarusInterface::GetIcarus()->IsRunning( other->m_iIcarusID ) /*!other->taskManager || !other->taskManager->IsRunning()*/ )
		{
			// if an item or weapon can we do a little explosion..?
			G_FreeEntity( other );
			return;
		}
	}

	if ( ent->damage )
	{
		if ( (ent->spawnflags&MOVER_CRUSHER)//a crusher
 			&& other->s.clientNum >= MAX_CLIENTS//not the player
			&& other->client //NPC
			&& other->health <= 0 //dead
			&& G_OkayToRemoveCorpse( other ) )//okay to remove him
		{//crusher stuck on a non->player corpse that does not have a key and is not running a script
			G_FreeEntity( other );
		}
		else
		{
			G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
		}
	}
	if ( ent->spawnflags & MOVER_CRUSHER ) {
		return;		// crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( ent, ent, other );
}


/*
================
Touch_DoorTrigger
================
*/

void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if ( ent->svFlags & SVF_INACTIVE )
	{
		return;
	}

	if ( ent->owner->spawnflags & MOVER_LOCKED )
	{//don't even try to use the door if it's locked
		return;
	}

	if ( ent->owner->moverState != MOVER_1TO2 )
	{//Door is not already opening
		//if ( ent->owner->moverState == MOVER_POS1 || ent->owner->moverState == MOVER_2TO1 )
		//{//only check these if closed or closing

		//If door is closed, opening or open, check this
		Use_BinaryMover( ent->owner, ent, other );
	}

	/*
	//Old style
	if ( ent->owner->moverState != MOVER_1TO2 ) {
		Use_BinaryMover( ent->owner, ent, other );
	}
	*/
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *ent )
{
	gentity_t		*other;
	vec3_t		mins, maxs;
	int			i, best;

	// set all of the slaves as shootable
	if ( ent->takedamage )
	{
		for ( other = ent ; other ; other = other->teamchain )
		{
			other->takedamage = qtrue;
		}
	}

	// find the bounds of everything on the team
	VectorCopy (ent->absmin, mins);
	VectorCopy (ent->absmax, maxs);

	for (other = ent->teamchain ; other ; other=other->teamchain) {
		AddPointToBounds (other->absmin, mins, maxs);
		AddPointToBounds (other->absmax, mins, maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn ();
	VectorCopy (mins, other->mins);
	VectorCopy (maxs, other->maxs);
	other->owner = ent;
	other->contents = CONTENTS_TRIGGER;
	other->e_TouchFunc = touchF_Touch_DoorTrigger;
	gi.linkentity (other);
	other->classname = "trigger_door";

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( gentity_t *ent )
{
	MatchTeam( ent, ent->moverState, level.time );
}

qboolean G_EntIsDoor( int entityNum )
{
	if ( entityNum < 0 || entityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	gentity_t *ent = &g_entities[entityNum];
	if ( ent && !Q_stricmp( "func_door", ent->classname ) )
	{//blocked by a door
		return qtrue;
	}
	return qfalse;
}

gentity_t *G_FindDoorTrigger( gentity_t *ent )
{
	gentity_t *owner = NULL;
	gentity_t *door = ent;
	if ( door->flags & FL_TEAMSLAVE )
	{//not the master door, get the master door
		while ( door->teammaster && (door->flags&FL_TEAMSLAVE))
		{
			door = door->teammaster;
		}
	}
	if ( door->targetname )
	{//find out what is targeting it
		//FIXME: if ent->targetname, check what kind of trigger/ent is targetting it?  If a normal trigger (active, etc), then it's okay?
		while ( (owner = G_Find( owner, FOFS( target ), door->targetname )) != NULL )
		{
			if ( owner && (owner->contents&CONTENTS_TRIGGER) )
			{
				return owner;
			}
		}
		owner = NULL;
		while ( (owner = G_Find( owner, FOFS( target2 ), door->targetname )) != NULL )
		{
			if ( owner && (owner->contents&CONTENTS_TRIGGER) )
			{
				return owner;
			}
		}
	}

	owner = NULL;
	while ( (owner = G_Find( owner, FOFS( classname ), "trigger_door" )) != NULL )
	{
		if ( owner->owner == door )
		{
			return owner;
		}
	}

	return NULL;
}

qboolean G_TriggerActive( gentity_t *self );
qboolean G_EntIsUnlockedDoor( int entityNum )
{
	if ( entityNum < 0 || entityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	if ( G_EntIsDoor( entityNum ) )
	{
		gentity_t *ent = &g_entities[entityNum];
		gentity_t *owner = NULL;
		if ( ent->flags & FL_TEAMSLAVE )
		{//not the master door, get the master door
			while ( ent->teammaster && (ent->flags&FL_TEAMSLAVE))
			{
				ent = ent->teammaster;
			}
		}
		if ( ent->targetname )
		{//find out what is targetting it
			owner = NULL;
			//FIXME: if ent->targetname, check what kind of trigger/ent is targetting it?  If a normal trigger (active, etc), then it's okay?
			while ( (owner = G_Find( owner, FOFS( target ), ent->targetname )) != NULL )
			{
				if ( !Q_stricmp( "trigger_multiple", owner->classname )
				  || !Q_stricmp( "trigger_once", owner->classname ) )//FIXME: other triggers okay too?
				{
					if ( G_TriggerActive( owner ) )
					{
						return qtrue;
					}
				}
			}
			owner = NULL;
			while ( (owner = G_Find( owner, FOFS( target2 ), ent->targetname )) != NULL )
			{
				if ( !Q_stricmp( "trigger_multiple", owner->classname ) )//FIXME: other triggers okay too?
				{
					if ( G_TriggerActive( owner ) )
					{
						return qtrue;
					}
				}
			}
			return qfalse;
		}
		else
		{//check the door's auto-created trigger instead
			owner = G_FindDoorTrigger( ent );
			if ( owner && (owner->svFlags&SVF_INACTIVE) )
			{//owning auto-created trigger is inactive
				return qfalse;
			}
		}
		if ( !(ent->svFlags & SVF_INACTIVE) && //assumes that the reactivate trigger isn't right next to the door!
			!ent->health &&
			!(ent->spawnflags & MOVER_PLAYER_USE) &&
			!(ent->spawnflags & MOVER_FORCE_ACTIVATE) &&
			!(ent->spawnflags & MOVER_LOCKED))
			//FIXME: what about MOVER_GOODIE?
		{
			return qtrue;
		}
	}
	return qfalse;
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN FORCE_ACTIVATE CRUSHER TOGGLE LOCKED GOODIE PLAYER_USE INACTIVE
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
FORCE_ACTIVATE	Can only be activated by a force push or pull
CRUSHER		?
TOGGLE		wait in both the start and end states for a trigger event - does NOT work on Trek doors.
LOCKED		Starts locked, with the shader animmap at the first frame and inactive.  Once used, the animmap changes to the second frame and the door operates normally.  Note that you cannot use the door again after this.
GOODIE		Door will not work unless activator has a "goodie key" in his inventory
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

"target"     Door fires this when it starts moving from it's closed position to its open position.
"opentarget" Door fires this after reaching its "open" position
"target2"    Door fires this when it starts moving from it's open position to its closed position.
"closetarget" Door fires this after reaching its "closed" position
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"delay"		when used, how many seconds to wait before moving - default is none
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default, set to negative for no damage)
"color"		constantLight color
"light"		constantLight radius
"health"	if set, the door must be shot open
"sounds" - sound door makes when opening/closing
"linear"	set to 1 and it will move linearly rather than with acceleration (default is 0)
0 - no sound (default)
*/
void SP_func_door (gentity_t *ent)
{
	vec3_t	abs_movedir;
	float	distance;
	vec3_t	size;
	float	lip;

	ent->e_BlockedFunc = blockedF_Blocked_Door;

	if ( ent->spawnflags & MOVER_GOODIE )
	{
		G_SoundIndex( "sound/movers/goodie_fail.wav" );
		G_SoundIndex( "sound/movers/goodie_pass.wav" );
	}

	// default speed of 400
	if (!ent->speed)
		ent->speed = 400;

	// default wait of 2 seconds
	if (!ent->wait)
		ent->wait = 2;
	ent->wait *= 1000;

	ent->delay *= 1000;

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );
	if ( ent->damage < 0 )
	{
		ent->damage = 0;
	}

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	gi.SetBrushModel( ent, ent->model );
	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->maxs, ent->mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// if "start_open", reverse position 1 and 2
	if ( ent->spawnflags & 1 )
	{
		vec3_t	temp;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );
	}

	if ( ent->spawnflags & MOVER_LOCKED )
	{//a locked door, set up as locked until used directly
		ent->s.eFlags |= EF_SHADER_ANIM;//use frame-controlled shader anim
		ent->s.frame = 0;//first stage of anim
	}
	InitMover( ent );

	ent->nextthink = level.time + FRAMETIME;

	if ( !(ent->flags&FL_TEAMSLAVE) )
	{
		int health;

		G_SpawnInt( "health", "0", &health );

		if ( health )
		{
			ent->takedamage = qtrue;
		}

		if ( !(ent->spawnflags&MOVER_LOCKED) && (ent->targetname || health || ent->spawnflags & MOVER_PLAYER_USE || ent->spawnflags & MOVER_FORCE_ACTIVATE) )
		{
			// non touch/shoot doors
			ent->e_ThinkFunc = thinkF_Think_MatchTeam;
		}
		else
		{//locked doors still spawn a trigger
			ent->e_ThinkFunc = thinkF_Think_SpawnNewDoorTrigger;
		}
	}
}

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->owner->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->owner, ent, other );
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t	*trigger;
	vec3_t	tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->e_TouchFunc = touchF_Touch_PlatCenterTrigger;
	trigger->contents = CONTENTS_TRIGGER;
	trigger->owner = ent;

	tmin[0] = ent->pos1[0] + ent->mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->mins[2];

	tmax[0] = ent->pos1[0] + ent->maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ent->pos1[0] + (ent->mins[0] + ent->maxs[0]) *0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ent->pos1[1] + (ent->mins[1] + ent->maxs[1]) *0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy (tmin, trigger->mins);
	VectorCopy (tmax, trigger->maxs);

	gi.linkentity (trigger);
}


/*QUAKED func_plat (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat (gentity_t *ent) {
	float		lip, height;

//	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
//	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");

	VectorClear (ent->s.angles);

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	gi.SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = (ent->maxs[2] - ent->mins[2]) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	ent->pos1[2] -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->e_TouchFunc = touchF_Touch_Plat;

	ent->e_BlockedFunc = blockedF_Blocked_Door;

	ent->owner = ent;	// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger(ent);
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( gentity_t *ent ) {
	vec3_t		abs_movedir;
	float		distance;
	vec3_t		size;
	float		lip;

//	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");

	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	gi.SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract( ent->maxs, ent->mins, size );
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA (ent->pos1, distance, ent->movedir, ent->pos2);

	if (ent->health) {
		// shootable button
		ent->takedamage = qtrue;
	} else {
		// touchable button
		ent->e_TouchFunc = touchF_Touch_Button;
	}

	InitMover( ent );
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON		1
#define TRAIN_TOGGLE		2
#define TRAIN_BLOCK_STOPS	4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *ent ) {

	if ( ent->spawnflags & 2048 )
	{
		// this tie fighter hack is done for doom_detention, where the shooting gallery takes place. let them draw again when they start moving
		ent->s.eFlags &= ~EF_NODRAW;
	}

	ent->s.pos.trTime = level.time;
	if ( ent->alt_fire )
	{
		ent->s.pos.trType = TR_LINEAR_STOP;
	}
	else
	{
		ent->s.pos.trType = TR_NONLINEAR_STOP;
	}
}

/*
===============
Reached_Train
===============
*/
void Reached_Train( gentity_t *ent ) {
	gentity_t		*next;
	float			speed;
	vec3_t			move;
	float			length;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		//FIXME: can we go backwards- say if we are toggle-able?
		return;		// just stop
	}

	// fire all other targets
	G_UseTargets( next, ent );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
	VectorCopy( next->s.origin, ent->pos1 );
	VectorCopy( next->nextTrain->s.origin, ent->pos2 );

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
	} else {
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if ( speed < 1 ) {
		speed = 1;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->s.pos.trDuration = length * 1000 / speed;

	// looping sound
/*
	if ( VALIDSTRING( next->soundSet ) )
	{
		ent->s.loopSound = CAS_GetBModelSound( next->soundSet, BMS_MID );//ent->soundLoop;
	}
*/
	G_PlayDoorLoopSound( ent );

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	if (( next->spawnflags & 1 ))
	{
		vec3_t angs;

		vectoangles( move, angs );
		AnglesSubtract( angs, ent->currentAngles, angs );

		for ( int i = 0; i < 3; i++ )
		{
			AngleNormalize360( angs[i] );
		}
		VectorCopy( ent->currentAngles, ent->s.apos.trBase );
		VectorScale( angs, 0.5f, ent->s.apos.trDelta );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 2000;
		if ( ent->alt_fire )
		{
			ent->s.apos.trType = TR_LINEAR_STOP;
		}
		else
		{
			ent->s.apos.trType = TR_NONLINEAR_STOP;
		}
	}
	else
	{
		if (( next->spawnflags & 4 ))
		{//yaw
			vec3_t angs;

			vectoangles( move, angs );
			AnglesSubtract( angs, ent->currentAngles, angs );

			for ( int i = 0; i < 3; i++ )
			{
				AngleNormalize360( angs[i] );
			}
			VectorCopy( ent->currentAngles, ent->s.apos.trBase );
			ent->s.apos.trDelta[YAW] = angs[YAW] * 0.5f;
			if (( next->spawnflags & 8 ))
			{//roll, too
				ent->s.apos.trDelta[ROLL] = angs[YAW] * -0.1f;
			}
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 2000;
			if ( ent->alt_fire )
			{
				ent->s.apos.trType = TR_LINEAR_STOP;
			}
			else
			{
				ent->s.apos.trType = TR_NONLINEAR_STOP;
			}
		}
	}

	// This is for the tie fighter shooting gallery on doom detention, you could see them waiting under the bay, but the architecture couldn't easily be changed..
	if (( next->spawnflags & 2 ))
	{
		ent->s.eFlags |= EF_NODRAW; // make us invisible until we start moving again
	}

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait )
	{
		ent->nextthink = level.time + next->wait * 1000;
		ent->e_ThinkFunc = thinkF_Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
	else if (!( next->spawnflags & 2 ))
	{
		// we aren't waiting to start, so let us draw right away
		ent->s.eFlags &= ~EF_NODRAW;
	}
}

void TrainUse( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	Reached_Train( ent );
}

/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent ) {
	gentity_t		*path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS(targetname), ent->target );
	if ( !ent->nextTrain ) {
		gi.Printf( "func_train at %s with an unfound target\n", vtos(ent->absmin) );
		//Free me?`
		return;
	}

	//FIXME: this can go into an infinite loop if last path_corner doesn't link to first
	//path_corner, like so:
	// t1---->t2---->t3
	//         ^      |
	//          \_____|
	start = NULL;
	next = NULL;
	int iterations = 2000;	//max attempts to find our path start
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if (!iterations--)
		{
			G_Error( "Think_SetupTrainTargets:  last path_corner doesn't link back to first on func_train(%s)", vtos(ent->absmin) );
		}
		if (!start)
		{
			start = path;
		}
		if ( !path->target ) {
//			gi.Printf( "Train corner at %s without a target\n", vtos(path->s.origin) );
			//end of path
			break;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS(targetname), path->target );
			if ( !next ) {
//				gi.Printf( "Train corner at %s without a target path_corner\n", vtos(path->s.origin) );
				//end of path
				break;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		if ( next )
		{
			path->nextTrain = next;
		}
		else
		{
			break;
		}
	}

	if ( !ent->targetname || (ent->spawnflags&1) /*start on*/)
	{
		// start the train moving from the first corner
		Reached_Train( ent );
	}
	else
	{
		G_SetOrigin( ent, ent->s.origin );
	}
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TURN_TRAIN INVISIBLE YAW_TRAIN ROLL_TRAIN

TURN_TRAIN	func_train moving on this path will turn to face the next path_corner within 2 seconds
INVISIBLE - train will become invisible ( but still solid ) when it reaches this path_corner.
		It will become visible again at the next path_corner that does not have this option checked

Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		gi.Printf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in
	VectorCopy(self->s.origin, self->currentOrigin);
}


void func_train_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	if ( self->target3 )
	{
		G_UseTargets2( self, self, self->target3 );
	}

	//HACKHACKHACKHACK
	G_PlayEffect( "explosions/fighter_explosion2", self->currentOrigin );
	G_FreeEntity( self );
	//HACKHACKHACKHACK
}

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS x x LOOP PLAYER_USE INACTIVE TIE

LOOP - if it's a ghoul2 model, it will just loop the animation
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used
TIE  flying Tie-fighter hack, should be made more flexible so other things can use this if needed

A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		default 100
"dmg"		default	2
"health"	default 0
"noise"		looping sound to play when the train is in motion
"targetname" will not start until used
"target"	next path corner
"target3" what to use when it breaks
"color"		constantLight color
"light"		constantLight radius
"linear" set to 1 and it will move linearly rather than with acceleration (default is 0)
"startframe" default 0...ghoul2 animation start frame
"endframe" default 0...ghoul2 animation end frame
*/
void SP_func_train (gentity_t *self) {
	VectorClear (self->s.angles);

	if (self->spawnflags & TRAIN_BLOCK_STOPS) {
		self->damage = 0;
	} else {
		if (!self->damage) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		gi.Printf ("func_train without a target at %s\n", vtos(self->absmin));
		G_FreeEntity( self );
		return;
	}

	char *noise;

	G_SpawnInt( "startframe", "0", &self->startFrame );
	G_SpawnInt( "endframe", "0", &self->endFrame );

	if ( G_SpawnString( "noise", "", &noise ) )
	{
		if ( VALIDSTRING( noise ) )
		{
			self->s.loopSound = cgi_S_RegisterSound( noise );//G_SoundIndex( noise );
		}
	}

	gi.SetBrushModel( self, self->model );
	InitMover( self );

	if ( self->spawnflags & 2048 ) // TIE HACK
	{
		//HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
		self->s.modelindex2 = G_ModelIndex( "models/map_objects/ships/tie_fighter.md3" );
		G_EffectIndex( "explosions/fighter_explosion2" );

		self->contents = CONTENTS_SHOTCLIP;
		self->takedamage = qtrue;
		VectorSet( self->maxs, 112, 112, 112 );
		VectorSet( self->mins, -112, -112, -112 );
		self->e_DieFunc  = dieF_func_train_die;
		gi.linkentity( self );
		//HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
	}

	if ( self->targetname )
	{
		self->e_UseFunc = useF_TrainUse;
	}

	self->e_ReachedFunc = reachedF_Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + START_TIME_LINK_ENTS;
	self->e_ThinkFunc = thinkF_Think_SetupTrainTargets;


	if ( self->playerModel >= 0 && self->spawnflags & 32 ) // loop...dunno, could support it on other things, but for now I need it for the glider...so...kill the flag
	{
		self->spawnflags &= ~32; // once only

		gi.G2API_SetBoneAnim( &self->ghoul2[self->playerModel], "model_root", self->startFrame, self->endFrame, BONE_ANIM_OVERRIDE_LOOP, 1.0f + Q_flrand(-1.0f, 1.0f) * 0.1f, 0, -1, -1 );
		self->endFrame = 0; // don't allow it to do anything with the animation function in G_main
	}
}

/*
===============================================================================

STATIC

===============================================================================
*/

/*QUAKED func_static (0 .5 .8) ? F_PUSH F_PULL SWITCH_SHADER CRUSHER IMPACT SOLITARY PLAYER_USE INACTIVE BROADCAST
F_PUSH		Will be used when you Force-Push it
F_PULL		Will be used when you Force-Pull it
SWITCH_SHADER	Toggle the shader anim frame between 1 and 2 when used
CRUSHER		Make it do damage when it's blocked
IMPACT		Make it do damage when it hits any entity
SOLITARY	Can only be pushed when directly under crosshair, when pushed you shall push nothing else BUT me.
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used
BROADCAST   don't ever use this, it's evil

A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius
"dmg"		how much damage to do when it crushes (use with CRUSHER spawnflag default is 2)
"linear" set to 1 and it will move linearly rather than with acceleration (default is 0)
"NPC_targetname" if set up to be push/pullable, only this NPC can push/pull it (for the player, use "player")
*/
void SP_func_static( gentity_t *ent )
{
	gi.SetBrushModel( ent, ent->model );

	VectorCopy( ent->s.origin, ent->pos1 );
	VectorCopy( ent->s.origin, ent->pos2 );

	InitMover( ent );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->e_UseFunc = useF_func_static_use;
	ent->e_ReachedFunc = reachedF_NULL;


	if( ent->spawnflags & 2048 )
	{								   // yes this is very very evil, but for now (pre-alpha) it's a solution
		ent->svFlags |= SVF_BROADCAST; // I need to rotate something that is huge and it's touching too many area portals...
	}

	if ( ent->spawnflags & 4/*SWITCH_SHADER*/ )
	{
		ent->s.eFlags |= EF_SHADER_ANIM;//use frame-controlled shader anim
		ent->s.frame = 0;//first stage of anim
		ent->spawnflags &= ~4;//this is the CRUSHER spawnflag!  remove it!
	}
	if ( ent->spawnflags & 8 )
	{//!!! 8 is NOT the crusher spawnflag, 4 is!!!
		ent->spawnflags &= ~8;
		ent->spawnflags |= MOVER_CRUSHER;
		if ( !ent->damage )
		{
			ent->damage = 2;
		}
	}
	gi.linkentity( ent );

	if (level.mBSPInstanceDepth)
	{	// this means that this guy will never be updated, moved, changed, etc.
		ent->s.eFlags = EF_PERMANENT;
	}
}

void func_static_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior( self, BSET_USE );

	if ( self->spawnflags & 4/*SWITCH_SHADER*/ )
	{
		self->s.frame = self->s.frame ? 0 : 1;//toggle frame
	}
	G_UseTargets( self, activator );
}

/*
===============================================================================

ROTATING

===============================================================================
*/
void func_rotating_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
//	vec3_t spot;
//	gentity_t	*tent;
	if( !other->client ) // don't want to disintegrate items or weapons, etc...
	{
		return;
	}
	// yeah, this is a bit hacky...
	if(	self->s.apos.trType != TR_STATIONARY && !(other->flags & FL_DISINTEGRATED) ) // only damage if it's moving
	{
//		VectorCopy( self->currentOrigin, spot );
//		tent = G_TempEntity( spot, EV_DISINTEGRATION );
//		tent->s.eventParm = PW_DISRUPTION;
//		tent->owner = self;
		// let G_Damage call the fx instead, beside, this way you can disintegrate a corpse this way
		G_Sound( other, G_SoundIndex( "sound/effects/energy_crackle.wav" ) );
		G_Damage( other, self, self, NULL, NULL, 10000, DAMAGE_NO_KNOCKBACK, MOD_SNIPER );
	}
}


void func_rotating_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if(	self->s.apos.trType == TR_LINEAR )
	{
		self->s.apos.trType = TR_STATIONARY;
		// stop the sound if it stops moving
		self->s.loopSound = 0;
		// play stop sound too?
		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_END ));
		}
	}
	else
	{
		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
			self->s.loopSound = CAS_GetBModelSound( self->soundSet, BMS_MID );
			if ( self->s.loopSound < 0 )
			{
				self->s.loopSound = 0;
			}
		}
		self->s.apos.trType = TR_LINEAR;
	}
}


/*QUAKED func_rotating (0 .5 .8) ? START_ON TOUCH_KILL X_AXIS Y_AXIS x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
TOUCH_KILL  Inflicts massive damage upon touching it, disitegrates bodies
INACTIVE	must be used by a target_activate before it can be used

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_rotating (gentity_t *ent) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}


	ent->s.apos.trType = TR_STATIONARY;
	if( ent->spawnflags & 1 )	// start on
	{
		ent->s.apos.trType = TR_LINEAR;
	}
	// set the axis of rotation
	if ( ent->spawnflags & 4 ) {
		ent->s.apos.trDelta[2] = ent->speed;
	} else if ( ent->spawnflags & 8 ) {
		ent->s.apos.trDelta[0] = ent->speed;
	} else {
		ent->s.apos.trDelta[1] = ent->speed;
	}

	if (!ent->damage) {
		ent->damage = 2;
	}


	gi.SetBrushModel( ent, ent->model );
	InitMover( ent );

	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_func_rotating_use;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->currentAngles );
	if( ent->spawnflags & 2 )
	{
		ent->e_TouchFunc = touchF_func_rotating_touch;
		G_SoundIndex( "sound/effects/energy_crackle.wav" );
	}

	gi.linkentity( ent );
}


/*
===============================================================================

BOBBING

===============================================================================
*/

void func_bobbing_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// Toggle our move state
	if ( self->s.pos.trType == TR_SINE )
	{
		self->s.pos.trType = TR_INTERPOLATE;

		// Save off roughly where we were
		VectorCopy( self->currentOrigin, self->s.pos.trBase );
		// Store the current phase value so we know how to start up where we left off.
		self->radius = ( level.time - self->s.pos.trTime ) / (float)self->s.pos.trDuration;
	}
	else
	{
		self->s.pos.trType = TR_SINE;

		// Set the time based on the saved phase value
		self->s.pos.trTime = level.time - self->s.pos.trDuration * self->radius;
		// Always make sure we are starting with a fresh base
		VectorCopy( self->s.origin, self->s.pos.trBase );
	}
}

/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS START_OFF x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

Normally bobs on the Z axis
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
"targetname" turns on/off when used
*/
void SP_func_bobbing (gentity_t *ent) {
	float		height;
	float		phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	gi.SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// set the axis of bobbing
	if ( ent->spawnflags & 1 ) {
		ent->s.pos.trDelta[0] = height;
	} else if ( ent->spawnflags & 2 ) {
		ent->s.pos.trDelta[1] = height;
	} else {
		ent->s.pos.trDelta[2] = height;
	}

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;

	if ( ent->spawnflags & 4 ) // start_off
	{
		ent->s.pos.trType = TR_INTERPOLATE;

		// Now use the phase to calculate where it should be at the start.
		ent->radius = phase;
		phase = (float)sin( phase * M_PI * 2 );
		VectorMA( ent->s.pos.trBase, phase, ent->s.pos.trDelta, ent->s.pos.trBase );

		if ( ent->targetname )
		{
			ent->e_UseFunc = useF_func_bobbing_use;
		}
	}
	else
	{
		ent->s.pos.trType = TR_SINE;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ? x x x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum(gentity_t *ent) {
	float		freq;
	float		length;
	float		phase;
	float		speed;

	G_SpawnFloat( "speed", "30", &speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	gi.SetBrushModel( ent, ent->model );

	// find pendulum length
	length = fabs( ent->mins[2] );
	if ( length < 8 ) {
		length = 8;
	}

	freq = 1 / ( M_PI * 2 ) * sqrt( g_gravity->value / ( 3 * length ) );

	ent->s.pos.trDuration = ( 1000 / freq );

	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	ent->s.apos.trDuration = 1000 / freq;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;

	ent->s.apos.trDelta[2] = speed;
}

/*
===============================================================================

WALL

===============================================================================
*/

//static -slc
void use_wall( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior(ent,BSET_USE);

	// Not there so make it there
	//if (!(ent->contents & CONTENTS_SOLID))
	if (!ent->count) // off
	{
		ent->svFlags &= ~SVF_NOCLIENT;
		ent->s.eFlags &= ~EF_NODRAW;
		ent->count = 1;
		//NOTE: reset the brush model because we need to get *all* the contents back
		//ent->contents |= CONTENTS_SOLID;
		gi.SetBrushModel( ent, ent->model );
		if ( !(ent->spawnflags&1) )
		{//START_OFF doesn't effect area portals
			gi.AdjustAreaPortalState( ent, qfalse );
		}
	}
	// Make it go away
	else
	{
		//NOTE: MUST do this BEFORE clearing contents, or you may not open the area portal!!!
		if ( !(ent->spawnflags&1) )
		{//START_OFF doesn't effect area portals
			gi.AdjustAreaPortalState( ent, qtrue );
		}
		ent->contents = 0;
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->count = 0; // off
	}
}

#define FUNC_WALL_OFF	1
#define FUNC_WALL_ANIM	2


/*QUAKED func_wall (0 .5 .8) ? START_OFF AUTOANIMATE x x x x PLAYER_USE INACTIVE
PLAYER_USE	Player can use it with the use button
INACTIVE	must be used by a target_activate before it can be used

A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"modelAngles" md3 model's angles <pitch yaw roll> (in addition to any rotation on the part of the brush entity itself)
"color"		constantLight color
"light"		constantLight radius

START_OFF - the wall will not be there
AUTOANIMATE - if a model is used it will animate
*/
void SP_func_wall( gentity_t *ent )
{
	gi.SetBrushModel( ent, ent->model );

	VectorCopy( ent->s.origin, ent->pos1 );
	VectorCopy( ent->s.origin, ent->pos2 );

	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->currentOrigin );

	// count is used as an on/off switch (start it on)
	ent->count = 1;

	// it must be START_OFF
	if (ent->spawnflags & FUNC_WALL_OFF)
	{
		ent->spawnContents = ent->contents;
		ent->contents = 0;
		ent->svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		// turn it off
		ent->count = 0;
	}

	if (!(ent->spawnflags & FUNC_WALL_ANIM))
	{
		ent->s.eFlags |= EF_ANIM_ALLFAST;
	}
	ent->e_UseFunc = useF_use_wall;

	gi.linkentity (ent);

}


void security_panel_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}
	if ( INV_SecurityKeyCheck( activator, self->message ) )
	{//congrats!
		gi.SendServerCommand( 0, "cp @SP_INGAME_SECURITY_KEY_UNLOCKEDDOOR" );
		//use targets
		G_UseTargets( self, activator );
		//take key
		INV_SecurityKeyTake( activator, self->message );
		if ( activator->ghoul2.size() )
		{
			gi.G2API_SetSurfaceOnOff( &activator->ghoul2[activator->playerModel], "l_arm_key", 0x00000002 );
		}
		//FIXME: maybe set frame on me to have key sticking out?
		//self->s.frame = 1;
		//play sound
		G_Sound( self, self->soundPos2 );
		//unusable
		self->e_UseFunc = useF_NULL;
	}
	else
	{//failure sound/display
		if ( activator->message )
		{//have a key, just the wrong one
			gi.SendServerCommand( 0, "cp @SP_INGAME_INCORRECT_KEY" );
		}
		else
		{//don't have a key at all
			gi.SendServerCommand( 0, "cp @SP_INGAME_NEED_SECURITY_KEY" );
		}
		G_UseTargets2( self, activator, self->target2 );
		//FIXME: change display?  Maybe shader animmap change?
		//play sound
		G_Sound( self, self->soundPos1 );
	}
}

/*QUAKED misc_security_panel (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x INACTIVE
model="models/map_objects/kejim/sec_panel.md3"
  A model that just sits there and opens when a player uses it and has right key

INACTIVE - Start off, has to be activated to be usable

"message"	name of the key player must have
"target"	thing to use when successfully opened
"target2"	thing to use when player uses the panel without the key
*/
void SP_misc_security_panel ( gentity_t *self )
{
	self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/sec_panel.md3" );
	self->soundPos1 = G_SoundIndex( "sound/movers/sec_panel_fail.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/sec_panel_pass.mp3" );
	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	VectorSet( self->mins, -8, -8, -8 );
	VectorSet( self->maxs, 8, 8, 8 );
	self->contents = CONTENTS_SOLID;
	gi.linkentity( self );

	//NOTE: self->message is the key
	self->svFlags |= SVF_PLAYER_USABLE;
	if(self->spawnflags & 128)
	{
		self->svFlags |= SVF_INACTIVE;
	}
	self->e_UseFunc = useF_security_panel_use;
}
