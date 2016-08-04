/*
===========================================================================
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

#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "b_local.h"

extern qboolean G_EntIsUnlockedDoor( int entityNum );
extern qboolean FlyingCreature( gentity_t *ent );

#define	MIN_DOOR_BLOCK_DIST			16
#define	MIN_DOOR_BLOCK_DIST_SQR		( MIN_DOOR_BLOCK_DIST * MIN_DOOR_BLOCK_DIST )
/*
-------------------------
NAV_HitNavGoal
-------------------------
*/

qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t dest, int radius, qboolean flying )
{
	vec3_t	dmins, dmaxs, pmins, pmaxs;

	if ( radius )
	{
		//NOTE:  This needs to do a DistanceSquared on navgoals that had
		//			a radius manually set! We can't do the smaller navgoals against
		//			walls to get around this because player-sized traces to them
		//			from angles will not work... - MCG
		if ( !flying )
		{//Allow for a little z difference
			vec3_t	diff;
			VectorSubtract( point, dest, diff );
			if ( fabs(diff[2]) <= 24 )
			{
				diff[2] = 0;
			}
			return (qboolean)( VectorLengthSquared( diff ) <= (radius*radius) );
		}
		else
		{//must hit exactly
			return (qboolean)( DistanceSquared(dest, point) <= (radius*radius) );
		}
		//There is probably a better way to do this, either by preserving the original
		//		mins and maxs of the navgoal and doing this check ONLY if the radius
		//		is non-zero (like the original implementation) or some boolean to
		//		tell us to do this check rather than the fake bbox overlap check...
	}
	else
	{
		//Construct a dummy bounding box from our radius value
		VectorSet( dmins, -radius, -radius, -radius );
		VectorSet( dmaxs,  radius,  radius,  radius );

		//Translate it
		VectorAdd( dmins, dest, dmins );
		VectorAdd( dmaxs, dest, dmaxs );

		//Translate the starting box
		VectorAdd( point, mins, pmins );
		VectorAdd( point, maxs, pmaxs );

		//See if they overlap
		return G_BoundsOverlap( pmins, pmaxs, dmins, dmaxs );
	}
}

/*
-------------------------
NAV_CheckAhead
-------------------------
*/

qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t &trace, int clipmask )
{
	vec3_t	mins;

	//Offset the step height
	VectorSet( mins, self->mins[0], self->mins[1], self->mins[2] + STEPSIZE );

	gi.trace( &trace, self->currentOrigin, mins, self->maxs, end, self->s.number, clipmask, (EG2_Collision)0, 0 );

	if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		gi.trace( &trace, self->currentOrigin, mins, self->maxs, end, self->s.number, clipmask, (EG2_Collision)0, 0 );
	}
	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
		return qtrue;

	//See if we're too far above
	if ( fabs( self->currentOrigin[2] - end[2] ) > 48 )
		return qfalse;

	//This is a work around
	float	radius = ( self->maxs[0] > self->maxs[1] ) ? self->maxs[0] : self->maxs[1];
	float	dist = Distance( self->currentOrigin, end );
	float	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	//Do a special check for doors
	if ( trace.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t	*blocker = &g_entities[trace.entityNum];

		if VALIDSTRING( blocker->classname )
		{
			if ( G_EntIsUnlockedDoor( blocker->s.number ) )
			//if ( Q_stricmp( blocker->classname, "func_door" ) == 0 )
			{
				//We're too close, try and avoid the door (most likely stuck on a lip)
				if ( DistanceSquared( self->currentOrigin, trace.endpos ) < MIN_DOOR_BLOCK_DIST_SQR )
					return qfalse;

				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_ClearPathToGoal
-------------------------
*/

qboolean NPC_ClearPathToGoal( vec3_t dir, gentity_t *goal )
{
	trace_t	trace;

	//FIXME: What does do about area portals?  THIS IS BROKEN
	//if ( gi.inPVS( NPC->currentOrigin, goal->currentOrigin ) == qfalse )
	//	return qfalse;

	//Look ahead and see if we're clear to move to our goal position
	if ( NAV_CheckAhead( NPC, goal->currentOrigin, trace, ( NPC->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
	{
		//VectorSubtract( goal->currentOrigin, NPC->currentOrigin, dir );
		return qtrue;
	}

	if (!FlyingCreature(NPC))
	{
		//See if we're too far above
		if ( fabs( NPC->currentOrigin[2] - goal->currentOrigin[2] ) > 48 )
			return qfalse;
	}

	//This is a work around
	float	radius = ( NPC->maxs[0] > NPC->maxs[1] ) ? NPC->maxs[0] : NPC->maxs[1];
	float	dist = Distance( NPC->currentOrigin, goal->currentOrigin );
	float	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	//See if we're looking for a navgoal
	if ( goal->svFlags & SVF_NAVGOAL )
	{
		//Okay, didn't get all the way there, let's see if we got close enough:
		if ( NAV_HitNavGoal( trace.endpos, NPC->mins, NPC->maxs, goal->currentOrigin, NPCInfo->goalRadius, FlyingCreature( NPC ) ) )
		{
			//VectorSubtract(goal->currentOrigin, NPC->currentOrigin, dir);
			return qtrue;
		}
	}

	return qfalse;
}

qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist )
{
	vec3_t	mins, end;
	trace_t	trace;

	VectorMA( self->currentOrigin, dist, dir, end );

	//Offset the step height
	VectorSet( mins, self->mins[0], self->mins[1], self->mins[2] + STEPSIZE );

	gi.trace( &trace, self->currentOrigin, mins, self->maxs, end, self->s.number, CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );

	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
	{
		return qtrue;
	}

	return qfalse;
}

qboolean NAV_MoveDirSafe( gentity_t *self, usercmd_t *cmd, float distScale = 1.0f )
{
	vec3_t	moveDir;

	if ( !self || !self->client )
	{
		return qtrue;
	}
	if ( !self->client->ps.speed )
	{
		return qtrue;
	}
	if ( FlyingCreature( self ) )
	{
		return qtrue;
	}
	if ( VectorCompare( self->client->ps.moveDir, vec3_origin ) )
	{//no movedir, build from cmd
		if ( !cmd->forwardmove && !cmd->rightmove )
		{//not moving at all
			return qtrue;
		}
		vec3_t fwd, right, fwdAngs = {0, self->currentAngles[YAW], 0};
		AngleVectors( fwdAngs, fwd, right, NULL );
		VectorScale( fwd, cmd->forwardmove, fwd );
		VectorScale( right, cmd->rightmove, right );
		VectorAdd( fwd, right, moveDir );
		VectorNormalize( moveDir );
	}
	else
	{
		VectorCopy( self->client->ps.moveDir, moveDir );
	}
	return (NAV_DirSafe( self, moveDir, (self->client->ps.speed/10.0f)*distScale ));
}
