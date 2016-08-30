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

#include "common_headers.h"

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "g_vehicles.h"

extern qboolean PM_ClientImpact( trace_t *trace, qboolean damageSelf );
extern qboolean PM_ControlledByPlayer( void );
extern qboolean PM_InReboundHold( int anim );
extern cvar_t *g_stepSlideFix;

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
extern qboolean PM_GroundSlideOkay( float zNormal );
extern qboolean PM_InSpecialJump( int anim );
qboolean	PM_SlideMove( float gravMod ) {
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		normal, planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	vec3_t		clipVelocity;
	int			i, j, k;
	trace_t	trace;
	vec3_t		end;
	float		time_left;
	float		into;
	vec3_t		endVelocity;
	vec3_t		endClipVelocity;
	qboolean	damageSelf = qtrue;
	int			slideMoveContents = pm->tracemask;

	if ( pm->ps->clientNum >= MAX_CLIENTS
		&& !PM_ControlledByPlayer() )
	{//a non-player client, not an NPC under player control
		if ( pml.walking //walking on the ground
			|| (pm->ps->groundEntityNum != ENTITYNUM_NONE //in air
				&& PM_InSpecialJump( pm->ps->legsAnim )//in a special jump
				&& !(pm->ps->eFlags&EF_FORCE_GRIPPED)//not being gripped
				&& !(pm->ps->pm_flags&PMF_TIME_KNOCKBACK)
				&& pm->gent
				&& pm->gent->forcePushTime < level.time) )//not being pushed
		{//
			// If we're a vehicle, ignore this if we're being driven
			if ( !pm->gent //not an game ent
				|| !pm->gent->client //not a client
				|| pm->gent->client->NPC_class != CLASS_VEHICLE//not a vehicle
				|| !pm->gent->m_pVehicle //no vehicle
				|| !pm->gent->m_pVehicle->m_pPilot//no pilot
				|| pm->gent->m_pVehicle->m_pPilot->s.number >= MAX_CLIENTS )//pilot is not the player
			{//then treat do not enter brushes as SOLID
				slideMoveContents |= CONTENTS_BOTCLIP;
			}
		}
	}

	numbumps = 4;

	VectorCopy (pm->ps->velocity, primal_velocity);
	VectorCopy (pm->ps->velocity, endVelocity);

	if ( gravMod )
	{
		if ( !(pm->ps->eFlags&EF_FORCE_GRIPPED) && !(pm->ps->eFlags&EF_FORCE_DRAINED) )
		{
			endVelocity[2] -= pm->ps->gravity * pml.frametime * gravMod;
		}
		pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane )
		{
			if ( PM_GroundSlideOkay( pml.groundTrace.plane.normal[2] ) )
			{// slide along the ground plane
				PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
					pm->ps->velocity, OVERCLIP );
			}
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane )
	{
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[0] );
		if ( !PM_GroundSlideOkay( planes[0][2] ) )
		{
			planes[0][2] = 0;
			VectorNormalize( planes[0] );
		}
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount=0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, slideMoveContents, (EG2_Collision)0, 0 );
		if ( (trace.contents&CONTENTS_BOTCLIP)
			&& (slideMoveContents&CONTENTS_BOTCLIP) )
		{//hit a do not enter brush
			if ( trace.allsolid || trace.startsolid  )//inside the botclip
			{//crap, we're in a do not enter brush, take it out for the remainder of the traces and re-trace this one right now without it
				slideMoveContents &= ~CONTENTS_BOTCLIP;
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, slideMoveContents, (EG2_Collision)0, 0 );
			}
			else if ( trace.plane.normal[2] > 0.0f )
			{//on top of a do not enter brush, it, just redo this one trace without it
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, (slideMoveContents&~CONTENTS_BOTCLIP), (EG2_Collision)0, 0 );
			}
		}

		if ( trace.allsolid )
		{// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if ( trace.fraction > 0 )
		{// actually covered some distance
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction == 1 )
		{
			 break;		// moved the entire distance
		}



		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		//Hit it
		if ( trace.surfaceFlags&SURF_NODAMAGE )
		{
			damageSelf = qfalse;
		}
		else if ( trace.entityNum == ENTITYNUM_WORLD && trace.plane.normal[2] > 0.5f )
		{//if we land on the ground, let falling damage do it's thing itself, otherwise do impact damage
			damageSelf = qfalse;
		}
		else
		{
			damageSelf = qtrue;
		}

		if ( PM_ClientImpact( &trace, damageSelf ) )
		{
			continue;
		}

		if (pm->gent->client &&
			pm->gent->client->NPC_class == CLASS_VEHICLE &&
			trace.plane.normal[2]<pm->gent->m_pVehicle->m_pVehicleInfo->maxSlope
			)
		{
			pm->ps->pm_flags |= PMF_BUMPED;
		}

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES )
		{// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		VectorCopy( trace.plane.normal, normal );

 		if ( !PM_GroundSlideOkay( normal[2] ) )
		{//wall-running
			//never push up off a sloped wall
			normal[2] = 0;
			VectorNormalize( normal );
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		if ( !(pm->ps->pm_flags&PMF_STUCK_TO_WALL) )
		{//no sliding if stuck to wall!
			for ( i = 0 ; i < numplanes ; i++ ) {
				if ( DotProduct( normal, planes[i] ) > 0.99 ) {
					VectorAdd( normal, pm->ps->velocity, pm->ps->velocity );
					break;
				}
			}
			if ( i < numplanes ) {
				continue;
			}
		}
		VectorCopy( normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = DotProduct( pm->ps->velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;		// move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity (pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity (endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct (planes[i], planes[j], dir);
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear( pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravMod ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return (qboolean)( bumpcount != 0 );
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( float gravMod )
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	vec3_t		slideMove, stepUpMove;
	trace_t		trace;
	vec3_t		up, down;
	qboolean	/*cantStepUpFwd, */isGiant = qfalse;;
	int			stepSize = STEPSIZE;

	VectorCopy (pm->ps->origin, start_o);
	VectorCopy (pm->ps->velocity, start_v);

	if ( PM_InReboundHold( pm->ps->legsAnim ) )
	{
		gravMod = 0.0f;
	}

	if ( PM_SlideMove( gravMod ) == 0 ) {
		return;		// we got exactly where we wanted to go first try
	}//else Bumped into something, see if we can step over it

	if ( pm->gent && pm->gent->client && pm->gent->client->NPC_class == CLASS_VEHICLE && pm->gent->m_pVehicle->m_pVehicleInfo->hoverHeight > 0 )
	{//Hovering vehicles don't do steps
		//FIXME: maybe make hovering vehicles go up steps, but not down them?
		return;
	}

	if ( pm->gent
		&& pm->gent->client
		&& (pm->gent->client->NPC_class == CLASS_ATST||pm->gent->client->NPC_class == CLASS_RANCOR) )
	{
		isGiant = qtrue;
		if ( pm->gent->client->NPC_class == CLASS_RANCOR )
		{
			if ( (pm->gent->spawnflags&1) )
			{
				stepSize = 64;//hack for Mutant Rancor stepping
			}
			else
			{
				stepSize = 48;//hack for Rancor stepping
			}
		}
		else
		{
			stepSize = 70;//hack for AT-ST stepping, slightly taller than a standing stormtrooper
		}
	}
	else if ( pm->maxs[2] <= 0 )
	{//short little guys can't go up steps... FIXME: just make this a flag for certain NPCs- especially ones that roll?
		stepSize = 4;
	}

	//Q3Final addition...
	VectorCopy(start_o, down);
	down[2] -= stepSize;
	pm->trace (&trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask, (EG2_Collision)0, 0);
	VectorSet(up, 0, 0, 1);
	// never step up when you still have up velocity
	if ( pm->ps->velocity[2] > 0 && (trace.fraction == 1.0 ||
			DotProduct(trace.plane.normal, up) < 0.7)) {
		return;
	}

	if ( !pm->ps->velocity[0] && !pm->ps->velocity[1] )
	{//All our velocity was cancelled sliding
		return;
	}

	VectorCopy (pm->ps->origin, down_o);
	VectorCopy (pm->ps->velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += stepSize;

	// test the player position if they were a stepheight higher

	pm->trace (&trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask, (EG2_Collision)0, 0);
	if ( trace.allsolid || trace.startsolid || trace.fraction == 0) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend can't step\n", c_pmove);
		}
		return;		// can't step up
	}

	if ( pm->debugLevel )
	{
		G_DebugLine(start_o,trace.endpos,2000,0xffffff,qtrue);
	}

//===Another slidemove forward================================================================================
	// try slidemove from this position
	VectorCopy( trace.endpos, pm->ps->origin );
	VectorCopy( start_v, pm->ps->velocity );
	/*cantStepUpFwd = */PM_SlideMove( gravMod );
//===Another slidemove forward================================================================================

	if ( pm->debugLevel )
	{
		G_DebugLine(trace.endpos,pm->ps->origin,2000,0xffffff,qtrue);
	}
	//compare the initial slidemove and this slidemove from a step up position
	VectorSubtract( down_o, start_o, slideMove );
	VectorSubtract( trace.endpos, pm->ps->origin, stepUpMove );

	if ( fabs(stepUpMove[0]) < 0.1 && fabs(stepUpMove[1]) < 0.1 && VectorLengthSquared( slideMove ) > VectorLengthSquared( stepUpMove ) )
	{
		//slideMove was better, use it
		VectorCopy (down_o, pm->ps->origin);
		VectorCopy (down_v, pm->ps->velocity);
	}
	else
	{
		qboolean skipStep = qfalse;
		// push down the final amount
		VectorCopy (pm->ps->origin, down);
		down[2] -= stepSize;
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask, (EG2_Collision)0, 0);
		if ( pm->debugLevel )
		{
			G_DebugLine(pm->ps->origin,trace.endpos,2000,0xffffff,qtrue);
		}
		if ( g_stepSlideFix->integer )
		{
			if ( pm->ps->clientNum < MAX_CLIENTS
				&& trace.plane.normal[2] < MIN_WALK_NORMAL )
			{//normal players cannot step up slopes that are too steep to walk on!
				vec3_t stepVec;
				//okay, the step up ends on a slope that it too steep to step up onto,
				//BUT:
				//If the step looks like this:
				//  (B)\__
				//        \_____(A)
				//Then it might still be okay, so we figure out the slope of the entire move
				//from (A) to (B) and if that slope is walk-upabble, then it's okay
				VectorSubtract( trace.endpos, down_o, stepVec );
				VectorNormalize( stepVec );
				if ( stepVec[2] > (1.0f-MIN_WALK_NORMAL) )
				{
					if ( pm->debugLevel )
					{
						G_DebugLine(down_o,trace.endpos,2000,0x0000ff,qtrue);
					}
					skipStep = qtrue;
				}
			}
		}

		if ( !trace.allsolid
			&& !skipStep ) //normal players cannot step up slopes that are too steep to walk on!
		{
			if ( pm->ps->clientNum
				&& isGiant
				&& g_entities[trace.entityNum].client
				&& pm->gent
				&& pm->gent->client
				&& pm->gent->client->NPC_class == CLASS_RANCOR )
			{//Rancor don't step on clients
				if ( g_stepSlideFix->integer )
				{
					VectorCopy (down_o, pm->ps->origin);
					VectorCopy (down_v, pm->ps->velocity);
				}
				else
				{
					VectorCopy (start_o, pm->ps->origin);
					VectorCopy (start_v, pm->ps->velocity);
				}
			}
			else if ( pm->ps->clientNum
				&& isGiant
				&& g_entities[trace.entityNum].client
				&& g_entities[trace.entityNum].client->playerTeam == pm->gent->client->playerTeam )
			{//AT-ST's don't step up on allies
				if ( g_stepSlideFix->integer )
				{
					VectorCopy (down_o, pm->ps->origin);
					VectorCopy (down_v, pm->ps->velocity);
				}
				else
				{
					VectorCopy (start_o, pm->ps->origin);
					VectorCopy (start_v, pm->ps->velocity);
				}
			}
			else
			{
				VectorCopy( trace.endpos, pm->ps->origin );
				if ( g_stepSlideFix->integer )
				{
					if ( trace.fraction < 1.0 )
					{
						PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
					}
				}
			}
		}
		else
		{
			if ( g_stepSlideFix->integer )
			{
				VectorCopy (down_o, pm->ps->origin);
				VectorCopy (down_v, pm->ps->velocity);
			}
		}
		if ( !g_stepSlideFix->integer )
		{
			if ( trace.fraction < 1.0 )
			{
				PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity, OVERCLIP );
			}
		}
	}

	/*
	if(cantStepUpFwd && pm->ps->origin[2] < start_o[2] + stepSize && pm->ps->origin[2] >= start_o[2])
	{//We bumped into something we could not step up
		pm->ps->pm_flags |= PMF_BLOCKED;
	}
	else
	{//We did step up, clear the bumped flag
	}
	*/
#if 0
	// if the down trace can trace back to the original position directly, don't step
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, start_o, pm->ps->clientNum, pm->tracemask);
	if ( trace.fraction == 1.0 ) {
		// use the original move
		VectorCopy (down_o, pm->ps->origin);
		VectorCopy (down_v, pm->ps->velocity);
		if ( pm->debugLevel ) {
			Com_Printf("%i:bend\n", c_pmove);
		}
	} else
#endif
	{
		// use the step move
		float	delta;

		delta = pm->ps->origin[2] - start_o[2];
		if ( delta > 2 ) {
			if ( delta < 7 ) {
				PM_AddEvent( EV_STEP_4 );
			} else if ( delta < 11 ) {
				PM_AddEvent( EV_STEP_8 );
			} else if ( delta < 15 ) {
				PM_AddEvent( EV_STEP_12 );
			} else {
				PM_AddEvent( EV_STEP_16 );
			}
		}
		if ( pm->debugLevel ) {
			Com_Printf("%i:stepped\n", c_pmove);
		}
	}
}

