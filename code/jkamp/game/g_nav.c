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

#include "b_local.h"
#include "g_nav.h"

extern qboolean G_EntIsUnlockedDoor( int entityNum );
extern qboolean G_EntIsDoor( int entityNum );
extern qboolean G_EntIsBreakable( int entityNum );
extern qboolean G_EntIsRemovableUsable( int entNum );
extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
//For debug graphics
//rwwFIXMEFIXME: Write these at some point for the sake of being able to debug visually
void G_Line( vec3_t start, vec3_t end, vec3_t color, float alpha )
{

}

void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha )
{

}

void G_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha )
{

}

void G_DrawEdge( vec3_t start, vec3_t end, int type )
{

}

void G_DrawNode( vec3_t origin, int type )
{

}

void G_DrawCombatPoint( vec3_t origin, int type )
{

}

void TAG_ShowTags( int flags )
{

}

qboolean FlyingCreature( gentity_t *ent )
{
	if (ent->client && ent->client->ps.gravity <= 0)
	{
		return qtrue;
	}
	return qfalse;
}

qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask );
void NAV_StoreWaypoint( gentity_t *ent );

extern vec3_t NPCDEBUG_RED;


/*
-------------------------
NPC_Blocked
-------------------------
*/


void NPC_Blocked( gentity_t *self, gentity_t *blocker )
{
	if ( self->NPC == NULL )
		return;

	//Don't do this too often
	if ( self->NPC->blockedSpeechDebounceTime > level.time )
		return;

	//Attempt to run any blocked scripts
	if ( G_ActivateBehavior( self, BSET_BLOCKED ) )
	{
		return;
	}

	//If this is one of our enemies, then just attack him
	if ( blocker->client && ( blocker->client->playerTeam == self->client->enemyTeam ) )
	{
		G_SetEnemy( self, blocker );
		return;
	}

	//Debutrap->Print( d_npcai, DEBUG_LEVEL_WARNING, "%s: Excuse me, %s %s!\n", self->targetname, blocker->classname, blocker->targetname );

	//If we're being blocked by the player, say something to them
	if ( ( blocker->s.number >= 0 && blocker->s.number < MAX_CLIENTS ) && ( ( blocker->client->playerTeam == self->client->playerTeam ) ) )
	{
		//guys in formation are not trying to get to a critical point,
		//don't make them yell at the player (unless they have an enemy and
		//are in combat because BP thinks it sounds cool during battle)
		//NOTE: only imperials, misc crewmen and hazard team have these wav files now
		//G_AddVoiceEvent( self, Q_irand(EV_BLOCKED1, EV_BLOCKED3), 0 );
	}

	self->NPC->blockedSpeechDebounceTime = level.time + MIN_BLOCKED_SPEECH_TIME + ( Q_flrand(0.0f, 1.0f) * 4000 );
	self->NPC->blockingEntNum = blocker->s.number;
}

/*
-------------------------
NPC_SetMoveGoal
-------------------------
*/

void NPC_SetMoveGoal( gentity_t *ent, vec3_t point, int radius, qboolean isNavGoal, int combatPoint, gentity_t *targetEnt )
{
	//Must be an NPC
	if ( ent->NPC == NULL )
	{
		return;
	}

	if ( ent->NPC->tempGoal == NULL )
	{//must still have a goal
		return;
	}

	//Copy the origin
	//VectorCopy( point, ent->NPC->goalPoint );	//FIXME: Make it use this, and this alone!
	VectorCopy( point, ent->NPC->tempGoal->r.currentOrigin );

	//Copy the mins and maxs to the tempGoal
	VectorCopy( ent->r.mins, ent->NPC->tempGoal->r.mins );
	VectorCopy( ent->r.mins, ent->NPC->tempGoal->r.maxs );

	ent->NPC->tempGoal->target = NULL;
	ent->NPC->tempGoal->clipmask = ent->clipmask;
	ent->NPC->tempGoal->flags &= ~FL_NAVGOAL;
	if ( targetEnt && targetEnt->waypoint >= 0 )
	{
		ent->NPC->tempGoal->waypoint = targetEnt->waypoint;
	}
	else
	{
		ent->NPC->tempGoal->waypoint = WAYPOINT_NONE;
	}
	ent->NPC->tempGoal->noWaypointTime = 0;

	if ( isNavGoal )
	{
		assert(ent->NPC->tempGoal->parent);
		ent->NPC->tempGoal->flags |= FL_NAVGOAL;
	}

	ent->NPC->tempGoal->combatPoint = combatPoint;
	ent->NPC->tempGoal->enemy = targetEnt;

	ent->NPC->goalEntity = ent->NPC->tempGoal;
	ent->NPC->goalRadius = radius;

	trap->LinkEntity( (sharedEntity_t *)ent->NPC->goalEntity );
}

/*
-------------------------
NAV_HitNavGoal
-------------------------
*/

qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t dest, int radius, qboolean flying )
{
	vec3_t	dmins, dmaxs, pmins, pmaxs;

	if ( radius & NAVGOAL_USE_RADIUS )
	{
		radius &= ~NAVGOAL_USE_RADIUS;
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
			return ( VectorLengthSquared( diff ) <= (radius*radius) );
		}
		else
		{//must hit exactly
			return ( DistanceSquared(dest, point) <= (radius*radius) );
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
NAV_ClearPathToPoint
-------------------------
*/

qboolean NAV_ClearPathToPoint( gentity_t *self, vec3_t pmins, vec3_t pmaxs, vec3_t point, int clipmask, int okToHitEntNum )
{
//	trace_t	trace;
//	return NAV_CheckAhead( self, point, trace, clipmask|CONTENTS_BOTCLIP );

	vec3_t	mins, maxs;
	trace_t	trace;

	//Test if they're even conceivably close to one another
	if ( !trap->InPVS( self->r.currentOrigin, point ) )
		return qfalse;

	if ( self->flags & FL_NAVGOAL )
	{
		if ( !self->parent )
		{
			//SHOULD NEVER HAPPEN!!!
			assert(self->parent);
			return qfalse;
		}
		VectorCopy( self->parent->r.mins, mins );
		VectorCopy( self->parent->r.maxs, maxs );
	}
	else
	{
		VectorCopy( pmins, mins );
		VectorCopy( pmaxs, maxs );
	}

	if ( self->client || ( self->flags & FL_NAVGOAL ) )
	{
		//Clients can step up things, or if this is a navgoal check, a client will be using this info
		mins[2] += STEPSIZE;

		//don't let box get inverted
		if ( mins[2] > maxs[2] )
		{
			mins[2] = maxs[2];
		}
	}

	if ( self->flags & FL_NAVGOAL )
	{
		//Trace from point to navgoal
		trap->Trace( &trace, point, mins, maxs, self->r.currentOrigin, self->parent->s.number, (clipmask|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP)&~CONTENTS_BODY, qfalse, 0, 0 );
		if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
		{//started inside do not enter, so ignore them
			clipmask &= ~CONTENTS_BOTCLIP;
			trap->Trace( &trace, point, mins, maxs, self->r.currentOrigin, self->parent->s.number, (clipmask|CONTENTS_MONSTERCLIP)&~CONTENTS_BODY, qfalse, 0, 0 );
		}

		if ( trace.startsolid || trace.allsolid )
		{
			return qfalse;
		}

		//Made it
		if ( trace.fraction == 1.0 )
		{
			return qtrue;
		}

		if ( okToHitEntNum != ENTITYNUM_NONE && trace.entityNum == okToHitEntNum )
		{
			return qtrue;
		}

		//Okay, didn't get all the way there, let's see if we got close enough:
		if ( NAV_HitNavGoal( self->r.currentOrigin, self->parent->r.mins, self->parent->r.maxs, trace.endpos, NPCS.NPCInfo->goalRadius, FlyingCreature( self->parent ) ) )
		{
			return qtrue;
		}
		else
		{
			if ( NAVDEBUG_showCollision )
			{
				if ( trace.entityNum < ENTITYNUM_WORLD && (&g_entities[trace.entityNum] != NULL) && g_entities[trace.entityNum].s.eType != ET_MOVER )
				{
					vec3_t	p1, p2;
					G_DrawEdge( point, trace.endpos, EDGE_PATH );
					VectorAdd(g_entities[trace.entityNum].r.mins, g_entities[trace.entityNum].r.currentOrigin, p1);
					VectorAdd(g_entities[trace.entityNum].r.maxs, g_entities[trace.entityNum].r.currentOrigin, p2);
					G_CubeOutline( p1, p2, FRAMETIME, 0x0000ff, 0.5 );
				}
				//FIXME: if it is a bmodel, light up the surf?
			}
		}
	}
	else
	{
		trap->Trace( &trace, self->r.currentOrigin, mins, maxs, point, self->s.number, clipmask|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0);
		if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
		{//started inside do not enter, so ignore them
			clipmask &= ~CONTENTS_BOTCLIP;
			trap->Trace( &trace, self->r.currentOrigin, mins, maxs, point, self->s.number, clipmask|CONTENTS_MONSTERCLIP, qfalse, 0, 0);
		}

		if( ( ( trace.startsolid == qfalse ) && ( trace.allsolid == qfalse ) ) && ( trace.fraction == 1.0f ) )
		{//FIXME: check for drops
			return qtrue;
		}

		if ( okToHitEntNum != ENTITYNUM_NONE && trace.entityNum == okToHitEntNum )
		{
			return qtrue;
		}

		if ( NAVDEBUG_showCollision )
		{
			if ( trace.entityNum < ENTITYNUM_WORLD && (&g_entities[trace.entityNum] != NULL) && g_entities[trace.entityNum].s.eType != ET_MOVER )
			{
				vec3_t	p1, p2;
				G_DrawEdge( self->r.currentOrigin, trace.endpos, EDGE_PATH );
				VectorAdd(g_entities[trace.entityNum].r.mins, g_entities[trace.entityNum].r.currentOrigin, p1);
				VectorAdd(g_entities[trace.entityNum].r.maxs, g_entities[trace.entityNum].r.currentOrigin, p2);
				G_CubeOutline( p1, p2, FRAMETIME, 0x0000ff, 0.5 );
			}
			//FIXME: if it is a bmodel, light up the surf?
		}
	}

	return qfalse;
}

/*
-------------------------
NAV_FindClosestWaypointForEnt
-------------------------
*/

int NAV_FindClosestWaypointForEnt( gentity_t *ent, int targWp )
{
	//FIXME: Take the target into account
	return trap->Nav_GetNearestNode( (sharedEntity_t *)ent, ent->waypoint, NF_CLEAR_PATH, targWp );
}

int NAV_FindClosestWaypointForPoint( gentity_t *ent, vec3_t point )
{
	int	bestWP;
	//FIXME: can we make this a static ent?
	gentity_t *marker = G_Spawn();

	if ( !marker )
	{
		return WAYPOINT_NONE;
	}

	G_SetOrigin( marker, point );

	VectorCopy( ent->r.mins, marker->r.mins );//stepsize?
	VectorCopy( ent->r.mins, marker->r.maxs );//crouching?

	marker->clipmask = ent->clipmask;
	marker->waypoint = WAYPOINT_NONE;

	bestWP = trap->Nav_GetNearestNode( (sharedEntity_t *)marker, marker->waypoint, NF_CLEAR_PATH, WAYPOINT_NONE );

	G_FreeEntity( marker );

	return bestWP;
}

int NAV_FindClosestWaypointForPoint2( vec3_t point )
{
	int	bestWP;
	//FIXME: can we make this a static ent?
	gentity_t *marker = G_Spawn();

	if ( !marker )
	{
		return WAYPOINT_NONE;
	}

	G_SetOrigin( marker, point );

	VectorSet( marker->r.mins, -16, -16, -6 );//includes stepsize
	VectorSet( marker->r.maxs, 16, 16, 32 );

	marker->clipmask = MASK_NPCSOLID;
	marker->waypoint = WAYPOINT_NONE;

	bestWP = trap->Nav_GetNearestNode( (sharedEntity_t *)marker, marker->waypoint, NF_CLEAR_PATH, WAYPOINT_NONE );

	G_FreeEntity( marker );

	return bestWP;
}

/*
-------------------------
NAV_ClearBlockedInfo
-------------------------
*/

void NAV_ClearBlockedInfo( gentity_t *self )
{
	self->NPC->aiFlags &= ~NPCAI_BLOCKED;
	self->NPC->blockingEntNum = ENTITYNUM_WORLD;
}

/*
-------------------------
NAV_SetBlockedInfo
-------------------------
*/

void NAV_SetBlockedInfo( gentity_t *self, int entId )
{
	self->NPC->aiFlags |= NPCAI_BLOCKED;
	self->NPC->blockingEntNum = entId;
}

/*
-------------------------
NAV_Steer
-------------------------
*/

int NAV_Steer( gentity_t *self, vec3_t dir, float distance )
{
	vec3_t	right_test, left_test;
	vec3_t	deviation;
	trace_t	tr;
	float	right_push;
	float	left_push;
	float	right_ang	= dir[YAW] + 45;
	float	left_ang	= dir[YAW] - 45;

	//Get the steering angles
	VectorCopy( dir, deviation );
	deviation[YAW] = right_ang;

	AngleVectors( deviation, right_test, NULL, NULL );

	deviation[YAW] = left_ang;

	AngleVectors( deviation, left_test, NULL, NULL );

	//Find the end positions
	VectorMA( self->r.currentOrigin, distance, right_test, right_test );
	VectorMA( self->r.currentOrigin, distance, left_test,  left_test );

	//Draw for debug purposes
	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, right_test, EDGE_PATH );
		G_DrawEdge( self->r.currentOrigin, left_test,  EDGE_PATH );
	}

	//Find the right influence
	NAV_CheckAhead( self, right_test, &tr, self->clipmask|CONTENTS_BOTCLIP );

	right_push = -45 * ( 1.0f - tr.fraction );

	//Find the left influence
	NAV_CheckAhead( self, left_test, &tr, self->clipmask|CONTENTS_BOTCLIP );

	left_push = 45 * ( 1.0f - tr.fraction );

	//Influence the mover to respond to the steering
	VectorCopy( dir, deviation );
	deviation[YAW] += ( left_push + right_push );

	return deviation[YAW];
}

/*
-------------------------
NAV_CheckAhead
-------------------------
*/

qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask )
{
	vec3_t	mins;
	float	radius;
	float	dist;
	float	tFrac;

	//Offset the step height
	VectorSet( mins, self->r.mins[0], self->r.mins[1], self->r.mins[2] + STEPSIZE );

	trap->Trace( trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, clipmask, qfalse, 0, 0 );

	if ( trace->startsolid&&(trace->contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		trap->Trace( trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, clipmask, qfalse, 0, 0 );
	}
	//Do a simple check
	if ( ( trace->allsolid == qfalse ) && ( trace->startsolid == qfalse ) && ( trace->fraction == 1.0f ) )
		return qtrue;

	//See if we're too far above
	if ( fabs( self->r.currentOrigin[2] - end[2] ) > 48 )
		return qfalse;

	//This is a work around
	radius = ( self->r.maxs[0] > self->r.maxs[1] ) ? self->r.maxs[0] : self->r.maxs[1];
	dist = Distance( self->r.currentOrigin, end );
	tFrac = 1.0f - ( radius / dist );

	if ( trace->fraction >= tFrac )
		return qtrue;

	//Do a special check for doors
	if ( trace->entityNum < ENTITYNUM_WORLD )
	{
		gentity_t	*blocker = &g_entities[trace->entityNum];

		if ( VALIDSTRING( blocker->classname ) )
		{
			if ( G_EntIsUnlockedDoor( blocker->s.number ) )
			//if ( Q_stricmp( blocker->classname, "func_door" ) == 0 )
			{
				//We're too close, try and avoid the door (most likely stuck on a lip)
				if ( DistanceSquared( self->r.currentOrigin, trace->endpos ) < MIN_DOOR_BLOCK_DIST_SQR )
					return qfalse;

				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
-------------------------
NAV_TestBypass
-------------------------
*/

static qboolean NAV_TestBypass( gentity_t *self, float yaw, float blocked_dist, vec3_t movedir )
{
	trace_t	tr;
	vec3_t	avoidAngles;
	vec3_t	block_test, block_pos;

	VectorClear( avoidAngles );
	avoidAngles[YAW] = yaw;

	AngleVectors( avoidAngles, block_test, NULL, NULL );
	VectorMA( self->r.currentOrigin, blocked_dist, block_test, block_pos );

	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, block_pos, EDGE_BLOCKED );
	}

	//See if we're clear to move in that direction
	if ( NAV_CheckAhead( self, block_pos, &tr, ( self->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
	{
		VectorCopy( block_test, movedir );

		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NAV_Bypass
-------------------------
*/

qboolean NAV_Bypass( gentity_t *self, gentity_t *blocker, vec3_t blocked_dir, float blocked_dist, vec3_t movedir )
{
	float dot, yaw, avoidRadius, arcAngle;
	vec3_t	right;

	//Draw debug info if requested
	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, blocker->r.currentOrigin, EDGE_NORMAL );
	}

	AngleVectors( self->r.currentAngles, NULL, right, NULL );

	//Get the blocked direction
	yaw = vectoyaw( blocked_dir );

	//Get the avoid radius
	avoidRadius = sqrt( ( blocker->r.maxs[0] * blocker->r.maxs[0] ) + ( blocker->r.maxs[1] * blocker->r.maxs[1] ) ) +
						sqrt( ( self->r.maxs[0] * self->r.maxs[0] ) + ( self->r.maxs[1] * self->r.maxs[1] ) );

	//See if we're inside our avoidance radius
	arcAngle = ( blocked_dist <= avoidRadius ) ? 135 : ( ( avoidRadius / blocked_dist ) * 90 );

	//FIXME: Although the below code will cause the NPC to take the "better" route, it can cause NPCs to become stuck on
	//		 one another in certain situations where both decide to take the same direction.

	//Check to see what dir the other guy is moving in (if any) and pick the opposite dir
	if ( blocker->client && !VectorCompare( blocker->client->ps.velocity, vec3_origin ) )
	{
		vec3_t blocker_movedir;
		VectorNormalize2( blocker->client->ps.velocity, blocker_movedir );
		dot = DotProduct( blocker_movedir, blocked_dir );
		if ( dot < 0.35f && dot > -0.35f )
		{//he's moving to the side of me
			vec3_t	block_pos;
			trace_t	tr;
			VectorScale( blocker_movedir, -1, blocker_movedir );
			VectorMA( self->r.currentOrigin, blocked_dist, blocker_movedir, block_pos );
			if ( NAV_CheckAhead( self, block_pos, &tr, ( self->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
			{
				VectorCopy( blocker_movedir, movedir );
				return qtrue;
			}
		}
	}

	//FIXME: this makes NPCs stack up and ping-pong like crazy.
	//			Need to keep track of this and stop trying after a while
	dot = DotProduct( blocked_dir, right );

	//Go right on the first try if that works better
	if ( dot < 0.0f )
		arcAngle *= -1;

	//Test full, best position first
	if ( NAV_TestBypass( self, AngleNormalize360( yaw + arcAngle ), blocked_dist, movedir ) )
		return qtrue;

	//Try a smaller arc
	if ( NAV_TestBypass( self, AngleNormalize360( yaw + ( arcAngle * 0.5f ) ), blocked_dist, movedir ) )
		return qtrue;

	//Try the other direction
	if ( NAV_TestBypass( self, AngleNormalize360( yaw + ( arcAngle * -1 ) ), blocked_dist, movedir ) )
		return qtrue;

	//Try the other direction more precisely
	if ( NAV_TestBypass( self, AngleNormalize360( yaw + ( ( arcAngle * -1 ) * 0.5f ) ), blocked_dist, movedir ) )
		return qtrue;

	//Unable to go around
	return qfalse;
}

/*
-------------------------
NAV_MoveBlocker
-------------------------
*/

qboolean NAV_MoveBlocker( gentity_t *self, vec3_t shove_dir )
{
	//FIXME: This is a temporary method for making blockers move

	//FIXME: This will, of course, push blockers off of cliffs, into walls and all over the place

	vec3_t	temp_dir, forward;

	vectoangles( shove_dir, temp_dir );

	temp_dir[YAW] += 45;
	AngleVectors( temp_dir, forward, NULL, NULL );

	VectorScale( forward, SHOVE_SPEED, self->client->ps.velocity );
	self->client->ps.velocity[2] += SHOVE_LIFT;

	//self->NPC->shoveDebounce = level.time + 100;

	return qtrue;
}

/*
-------------------------
NAV_ResolveBlock
-------------------------
*/

qboolean NAV_ResolveBlock( gentity_t *self, gentity_t *blocker, vec3_t blocked_dir )
{
	//Stop double waiting
	if ( ( blocker->NPC ) && ( blocker->NPC->blockingEntNum == self->s.number ) )
		return qtrue;

	//For now, just complain about it
	NPC_Blocked( self, blocker );
	NPC_FaceEntity( blocker, qtrue );

	return qfalse;
}

/*
-------------------------
NAV_TrueCollision
-------------------------
*/

qboolean NAV_TrueCollision( gentity_t *self, gentity_t *blocker, vec3_t movedir, vec3_t blocked_dir )
{
	vec3_t	velocityDir;
	float speed, dot;
	vec3_t	testPos;
	vec3_t	ptmins, ptmaxs, tmins, tmaxs;

	//TODO: Handle all ents
	if ( blocker->client == NULL )
		return qfalse;

	//Get the player's move direction and speed
	speed = VectorNormalize2( self->client->ps.velocity, velocityDir );

	//See if it's even feasible
	dot = DotProduct( movedir, velocityDir );

	if ( dot < 0.85 )
		return qfalse;

	VectorMA( self->r.currentOrigin, speed*FRAMETIME, velocityDir, testPos );

	VectorAdd( blocker->r.currentOrigin, blocker->r.mins, tmins );
	VectorAdd( blocker->r.currentOrigin, blocker->r.maxs, tmaxs );

	VectorAdd( testPos, self->r.mins, ptmins );
	VectorAdd( testPos, self->r.maxs, ptmaxs );

	if ( G_BoundsOverlap( ptmins, ptmaxs, tmins, tmaxs ) )
	{
		VectorCopy( velocityDir, blocked_dir );
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
NAV_StackedCanyon
-------------------------
*/

qboolean NAV_StackedCanyon( gentity_t *self, gentity_t *blocker, vec3_t pathDir )
{
	vec3_t	perp, cross, test;
	float	avoidRadius;
	int		extraClip = CONTENTS_BOTCLIP;
	trace_t	tr;

	PerpendicularVector( perp, pathDir );
	CrossProduct( pathDir, perp, cross );

	avoidRadius =	sqrt( ( blocker->r.maxs[0] * blocker->r.maxs[0] ) + ( blocker->r.maxs[1] * blocker->r.maxs[1] ) ) +
					sqrt( ( self->r.maxs[0] * self->r.maxs[0] ) + ( self->r.maxs[1] * self->r.maxs[1] ) );

	VectorMA( blocker->r.currentOrigin, avoidRadius, cross, test );

	trap->Trace( &tr, test, self->r.mins, self->r.maxs, test, self->s.number, self->clipmask|extraClip, qfalse, 0, 0 );
	if ( tr.startsolid&&(tr.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		extraClip &= ~CONTENTS_BOTCLIP;
		trap->Trace( &tr, test, self->r.mins, self->r.maxs, test, self->s.number, self->clipmask|extraClip, qfalse, 0, 0 );
	}

	if ( NAVDEBUG_showCollision )
	{
		vec3_t	mins, maxs;
		vec3_t	RED = { 1.0f, 0.0f, 0.0f };

		VectorAdd( test, self->r.mins, mins );
		VectorAdd( test, self->r.maxs, maxs );
		G_Cube( mins, maxs, RED, 0.25 );
	}

	if ( tr.startsolid == qfalse && tr.allsolid == qfalse )
		return qfalse;

	VectorMA( blocker->r.currentOrigin, -avoidRadius, cross, test );

	trap->Trace( &tr, test, self->r.mins, self->r.maxs, test, self->s.number, self->clipmask|extraClip, qfalse, 0, 0 );
	if ( tr.startsolid&&(tr.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		extraClip &= ~CONTENTS_BOTCLIP;
		trap->Trace( &tr, test, self->r.mins, self->r.maxs, test, self->s.number, self->clipmask|extraClip, qfalse, 0, 0 );
	}

	if ( tr.startsolid == qfalse && tr.allsolid == qfalse )
		return qfalse;

	if ( NAVDEBUG_showCollision )
	{
		vec3_t	mins, maxs;
		vec3_t	RED = { 1.0f, 0.0f, 0.0f };

		VectorAdd( test, self->r.mins, mins );
		VectorAdd( test, self->r.maxs, maxs );
		G_Cube( mins, maxs, RED, 0.25 );
	}

	return qtrue;
}

/*
-------------------------
NAV_ResolveEntityCollision
-------------------------
*/

qboolean NAV_ResolveEntityCollision( gentity_t *self, gentity_t *blocker, vec3_t movedir, vec3_t pathDir )
{
	vec3_t	blocked_dir;
	float blocked_dist;

	//Doors are ignored
	if ( G_EntIsUnlockedDoor( blocker->s.number ) )
	//if ( Q_stricmp( blocker->classname, "func_door" ) == 0 )
	{
		if ( DistanceSquared( self->r.currentOrigin, blocker->r.currentOrigin ) > MIN_DOOR_BLOCK_DIST_SQR )
			return qtrue;
	}

	VectorSubtract( blocker->r.currentOrigin, self->r.currentOrigin, blocked_dir );
	blocked_dist = VectorNormalize( blocked_dir );

	//Make sure an actual collision is going to happen
//	if ( NAV_PredictCollision( self, blocker, movedir, blocked_dir ) == qfalse )
//		return qtrue;

	//See if we can get around the blocker at all (only for player!)
	if ( blocker->s.number >= 0 && blocker->s.number < MAX_CLIENTS )
	{
		if ( NAV_StackedCanyon( self, blocker, pathDir ) )
		{
			NPC_Blocked( self, blocker );
			NPC_FaceEntity( blocker, qtrue );

			return qfalse;
		}
	}

	//First, attempt to walk around the blocker
	if ( NAV_Bypass( self, blocker, blocked_dir, blocked_dist, movedir ) )
		return qtrue;

	//Second, attempt to calculate a good move position for the blocker
	if ( NAV_ResolveBlock( self, blocker, blocked_dir ) )
		return qtrue;

	return qfalse;
}

/*
-------------------------
NAV_TestForBlocked
-------------------------
*/

qboolean NAV_TestForBlocked( gentity_t *self, gentity_t *goal, gentity_t *blocker, float distance, int *flags )
{
	if ( goal == NULL )
		return qfalse;

	if ( blocker->s.eType == ET_ITEM )
		return qfalse;

	if ( NAV_HitNavGoal( blocker->r.currentOrigin, blocker->r.mins, blocker->r.maxs, goal->r.currentOrigin, 12, qfalse ) )
	{
		*flags |= NIF_BLOCKED;

		if ( distance <= MIN_STOP_DIST )
		{
			NPC_Blocked( self, blocker );
			NPC_FaceEntity( blocker, qtrue );
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NAV_AvoidCollsion
-------------------------
*/

qboolean NAV_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info )
{
	vec3_t	movedir;
	vec3_t	movepos;

	//Clear our block info for this frame
	NAV_ClearBlockedInfo( NPCS.NPC );

	//Cap our distance
	if ( info->distance > MAX_COLL_AVOID_DIST )
	{
		info->distance = MAX_COLL_AVOID_DIST;
	}

	//Get an end position
	VectorMA( self->r.currentOrigin, info->distance, info->direction, movepos );
	VectorCopy( info->direction, movedir );

	if ( self && self->NPC && (self->NPC->aiFlags&NPCAI_NO_COLL_AVOID) )
	{//pretend there's no-one in the way
		return qtrue;
	}
	//Now test against entities
	if ( NAV_CheckAhead( self, movepos, &info->trace, CONTENTS_BODY ) == qfalse )
	{
		//Get the blocker
		info->blocker = &g_entities[ info->trace.entityNum ];
		info->flags |= NIF_COLLISION;

		//Ok to hit our goal entity
		if ( goal == info->blocker )
			return qtrue;

		//See if we're moving along with them
		//if ( NAV_TrueCollision( self, info.blocker, movedir, info.direction ) == qfalse )
		//	return qtrue;

		//Test for blocking by standing on goal
		if ( NAV_TestForBlocked( self, goal, info->blocker, info->distance, &info->flags ) == qtrue )
			return qfalse;

		//If the above function said we're blocked, don't do the extra checks
		if ( info->flags & NIF_BLOCKED )
			return qtrue;

		//See if we can get that entity to move out of our way
		if ( NAV_ResolveEntityCollision( self, info->blocker, movedir, info->pathDirection ) == qfalse )
			return qfalse;

		VectorCopy( movedir, info->direction );

		return qtrue;
	}

	//Our path is clear, just move there
	if ( NAVDEBUG_showCollision )
	{
		G_DrawEdge( self->r.currentOrigin, movepos, EDGE_PATH );
	}

	return qtrue;
}

/*
-------------------------
NAV_TestBestNode
-------------------------
*/

int NAV_TestBestNode( gentity_t *self, int startID, int endID, qboolean failEdge )
{//check only against architectrure
	vec3_t	end;
	trace_t	trace;
	vec3_t	mins;
	int		clipmask = (NPCS.NPC->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP;

	//get the position for the test choice
	trap->Nav_GetNodePosition( endID, end );

	//Offset the step height
	VectorSet( mins, self->r.mins[0], self->r.mins[1], self->r.mins[2] + STEPSIZE );

	trap->Trace( &trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, clipmask, qfalse, 0, 0 );

	if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		trap->Trace( &trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, clipmask, qfalse, 0, 0 );
	}
	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
	{//it's clear
		return endID;
	}

	//See if we're too far above
	if ( self->s.weapon != WP_SABER && fabs( self->r.currentOrigin[2] - end[2] ) > 48 )
	{
	}
	else
	{
		//This is a work around
		float	radius = ( self->r.maxs[0] > self->r.maxs[1] ) ? self->r.maxs[0] : self->r.maxs[1];
		float	dist = Distance( self->r.currentOrigin, end );
		float	tFrac = 1.0f - ( radius / dist );

		if ( trace.fraction >= tFrac )
		{//it's clear
			return endID;
		}
	}

	//Do a special check for doors
	if ( trace.entityNum < ENTITYNUM_WORLD )
	{
		gentity_t	*blocker = &g_entities[trace.entityNum];

		if VALIDSTRING( blocker->classname )
		{//special case: doors are architecture, but are dynamic, like entitites
			if ( G_EntIsUnlockedDoor( blocker->s.number ) )
			//if ( Q_stricmp( blocker->classname, "func_door" ) == 0 )
			{//it's unlocked, go for it
				//We're too close, try and avoid the door (most likely stuck on a lip)
				if ( DistanceSquared( self->r.currentOrigin, trace.endpos ) < MIN_DOOR_BLOCK_DIST_SQR )
				{
					return startID;
				}
				//we can keep heading to the door, it should open
				if ( self->s.weapon != WP_SABER && fabs( self->r.currentOrigin[2] - end[2] ) > 48 )
				{//too far above
				}
				else
				{
					return endID;
				}
			}
			else if ( G_EntIsDoor( blocker->s.number ) )
			{//a locked door!
				//path is blocked by a locked door, mark it as such if instructed to do so
				if ( failEdge )
				{
					trap->Nav_AddFailedEdge( self->s.number, startID, endID );
				}
			}
			else if ( G_EntIsBreakable( blocker->s.number ) )
			{//do same for breakable brushes/models/glass?
				//path is blocked by a breakable, mark it as such if instructed to do so
				if ( failEdge )
				{
					trap->Nav_AddFailedEdge( self->s.number, startID, endID );
				}
			}
			else if ( G_EntIsRemovableUsable( blocker->s.number ) )
			{//and removable usables
				//path is blocked by a removable usable, mark it as such if instructed to do so
				if ( failEdge )
				{
					trap->Nav_AddFailedEdge( self->s.number, startID, endID );
				}
			}
			else if ( blocker->targetname && blocker->s.solid == SOLID_BMODEL && ((blocker->r.contents&CONTENTS_MONSTERCLIP)|| (blocker->r.contents&CONTENTS_BOTCLIP)) )
			{//some other kind of do not enter entity brush that will probably be removed
				//path is blocked by a removable brushent, mark it as such if instructed to do so
				if ( failEdge )
				{
					trap->Nav_AddFailedEdge( self->s.number, startID, endID );
				}
			}
		}
	}
	//path is blocked
	//use the fallback choice
	return startID;
}

/*
-------------------------
NAV_GetNearestNode
-------------------------
*/

int NAV_GetNearestNode( gentity_t *self, int lastNode )
{
	return trap->Nav_GetNearestNode( (sharedEntity_t *)self, lastNode, NF_CLEAR_PATH, WAYPOINT_NONE );
}

/*
-------------------------
NAV_MicroError
-------------------------
*/

qboolean NAV_MicroError( vec3_t start, vec3_t end )
{
	if ( VectorCompare( start, end ) )
	{
		if ( DistanceSquared( NPCS.NPC->r.currentOrigin, start ) < (8*8) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NAV_MoveToGoal
-------------------------
*/

int	NAV_MoveToGoal( gentity_t *self, navInfo_t *info )
{
	int bestNode;
	vec3_t	origin, end;

	//Must have a goal entity to move there
	if( self->NPC->goalEntity == NULL )
		return WAYPOINT_NONE;

	//Check special player optimizations
	if ( self->NPC->goalEntity->s.number >= 0 && self->NPC->goalEntity->s.number < MAX_CLIENTS )
	{
		//If we couldn't find the point, then we won't be able to this turn
		if ( self->NPC->goalEntity->waypoint == WAYPOINT_NONE )
			return WAYPOINT_NONE;

		//NOTENOTE: Otherwise trust this waypoint for the whole frame (reduce all unnecessary calculations)
	}
	else
	{
		//Find the target's waypoint
		if ( ( self->NPC->goalEntity->waypoint = NAV_GetNearestNode( self->NPC->goalEntity, self->NPC->goalEntity->waypoint ) ) == WAYPOINT_NONE )
			return WAYPOINT_NONE;
	}

	//Find our waypoint
	if ( ( self->waypoint = NAV_GetNearestNode( self, self->lastWaypoint ) ) == WAYPOINT_NONE )
		return WAYPOINT_NONE;

	bestNode = trap->Nav_GetBestNode( self->waypoint, self->NPC->goalEntity->waypoint, NODE_NONE );

	if ( bestNode == WAYPOINT_NONE )
	{
		if ( NAVDEBUG_showEnemyPath )
		{
			vec3_t	neworigin, torigin;

			trap->Nav_GetNodePosition( self->NPC->goalEntity->waypoint, torigin );
			trap->Nav_GetNodePosition( self->waypoint, neworigin );

			G_DrawNode( torigin, NODE_GOAL );
			G_DrawNode( neworigin, NODE_GOAL );
			G_DrawNode( self->NPC->goalEntity->r.currentOrigin, NODE_START );
		}

		return WAYPOINT_NONE;
	}

	//Check this node
	bestNode = NAV_TestBestNode( self, bestNode, self->NPC->goalEntity->waypoint, qfalse );

	//trace_t	trace;

	//Get this position
	trap->Nav_GetNodePosition( bestNode, origin );
	trap->Nav_GetNodePosition( self->waypoint, end );

	//Basically, see if the path we have isn't helping
	//if ( NAV_MicroError( origin, end ) )
	//	return WAYPOINT_NONE;

	//Test the path connection from our current position to the best node
	if ( NAV_CheckAhead( self, origin, &info->trace, (self->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP ) == qfalse )
	{
		//First attempt to move to the closest point on the line between the waypoints
		G_FindClosestPointOnLineSegment( origin, end, self->r.currentOrigin, origin );

		//See if we can go there
		if ( NAV_CheckAhead( self, origin, &info->trace, (self->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP ) == qfalse )
		{
			//Just move towards our current waypoint
			bestNode = self->waypoint;
			trap->Nav_GetNodePosition( bestNode, origin );
		}
	}

	//Setup our new move information
	VectorSubtract( origin, self->r.currentOrigin, info->direction );
	info->distance = VectorNormalize( info->direction );

	VectorSubtract( end, origin, info->pathDirection );
	VectorNormalize( info->pathDirection );

	//Draw any debug info, if requested
	if ( NAVDEBUG_showEnemyPath )
	{
		vec3_t	dest, start;

		//Get the positions
		trap->Nav_GetNodePosition( self->NPC->goalEntity->waypoint, dest );
		trap->Nav_GetNodePosition( bestNode, start );

		//Draw the route
		G_DrawNode( start, NODE_START );
		G_DrawNode( dest, NODE_GOAL );
		trap->Nav_ShowPath( self->waypoint, self->NPC->goalEntity->waypoint );
	}

	return bestNode;
}

/*
-------------------------
waypoint_testDirection
-------------------------
*/

unsigned int waypoint_testDirection( vec3_t origin, float yaw, unsigned int minDist )
{
	vec3_t	trace_dir, test_pos;
	vec3_t	maxs, mins;
	vec3_t	angles;
	trace_t	tr;

	//Setup the mins and max
	VectorSet( maxs, 15, 15, DEFAULT_MAXS_2 );
	VectorSet( mins, -15, -15, DEFAULT_MINS_2 + STEPSIZE );

	//Get our test direction
	VectorSet(angles, 0, yaw, 0 );
	AngleVectors( angles, trace_dir, NULL, NULL );

	//Move ahead
//	VectorMA( origin, MAX_RADIUS_CHECK, trace_dir, test_pos );
	VectorMA( origin, minDist, trace_dir, test_pos );

	trap->Trace( &tr, origin, mins, maxs, test_pos, ENTITYNUM_NONE, ( CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP ), qfalse, 0, 0 );

	//return (unsigned int) ( (float) MAX_RADIUS_CHECK * tr.fraction );
	return (unsigned int) ( (float) minDist * tr.fraction );
}

/*
-------------------------
waypoint_getRadius
-------------------------
*/

unsigned int waypoint_getRadius( gentity_t *ent )
{
	unsigned int	minDist = MAX_RADIUS_CHECK + 1; // (unsigned int) -1;
	unsigned int	dist;
	int				i;

	for ( i = 0; i < YAW_ITERATIONS; i++ )
	{
		dist = waypoint_testDirection( ent->r.currentOrigin, ((360.0f/YAW_ITERATIONS) * i), minDist );

		if ( dist < minDist )
			minDist = dist;
	}

	return minDist;
}

/*QUAKED waypoint  (0.7 0.7 0) (-16 -16 -24) (16 16 32) SOLID_OK
a place to go.

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

radius is automatically calculated in-world.
*/
void SP_waypoint ( gentity_t *ent )
{
	if ( navCalculatePaths )
	{
		unsigned int radius;

		VectorSet(ent->r.mins, -15, -15, DEFAULT_MINS_2);
		VectorSet(ent->r.maxs, 15, 15, DEFAULT_MAXS_2);

		ent->r.contents = CONTENTS_TRIGGER;
		ent->clipmask = MASK_DEADSOLID;

		trap->LinkEntity( (sharedEntity_t *)ent );

		ent->count = -1;
		ent->classname = "waypoint";

		if( !(ent->spawnflags&1) && G_CheckInSolid (ent, qtrue))
		{//if not SOLID_OK, and in solid
			ent->r.maxs[2] = CROUCH_MAXS_2;
			if(G_CheckInSolid (ent, qtrue))
			{
				Com_Printf(S_COLOR_RED"ERROR: Waypoint %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
				assert(0 && "Waypoint in solid!");
				G_FreeEntity(ent);
				return;
			}
		}

		radius = waypoint_getRadius( ent );

		ent->health = trap->Nav_AddRawPoint( ent->r.currentOrigin, ent->spawnflags, radius );
		NAV_StoreWaypoint( ent );
		G_FreeEntity(ent);
		return;
	}

	G_FreeEntity(ent);
}

/*QUAKED waypoint_small  (0.7 0.7 0) (-2 -2 -24) (2 2 32) SOLID_OK
SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)
*/
void SP_waypoint_small (gentity_t *ent)
{
	if ( navCalculatePaths )
	{
		VectorSet(ent->r.mins, -2, -2, DEFAULT_MINS_2);
		VectorSet(ent->r.maxs, 2, 2, DEFAULT_MAXS_2);

		ent->r.contents = CONTENTS_TRIGGER;
		ent->clipmask = MASK_DEADSOLID;

		trap->LinkEntity( (sharedEntity_t *)ent );

		ent->count = -1;
		ent->classname = "waypoint";

		if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qtrue ) )
		{
			ent->r.maxs[2] = CROUCH_MAXS_2;
			if ( G_CheckInSolid( ent, qtrue ) )
			{
				Com_Printf(S_COLOR_RED"ERROR: Waypoint_small %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
				assert(0);
				G_FreeEntity(ent);
				return;
			}
		}

		ent->health = trap->Nav_AddRawPoint( ent->r.currentOrigin, ent->spawnflags, 2 );
		NAV_StoreWaypoint( ent );
		G_FreeEntity(ent);
		return;
	}

	G_FreeEntity(ent);
}


/*QUAKED waypoint_navgoal (0.3 1 0.3) (-16 -16 -24) (16 16 32) SOLID_OK
A waypoint for script navgoals
Not included in navigation data

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

radius - how far from the navgoal an ent can be before it thinks it reached it - default is "0" which means no radius check, just have to touch it
*/

void SP_waypoint_navgoal( gentity_t *ent )
{
	int radius = ( ent->radius ) ? (((int)ent->radius)|NAVGOAL_USE_RADIUS) : 12;

	VectorSet( ent->r.mins, -16, -16, -24 );
	VectorSet( ent->r.maxs, 16, 16, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		Com_Printf(S_COLOR_RED"ERROR: Waypoint_navgoal %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
		assert(0);
	}
	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, radius, RTF_NAVGOAL );

	ent->classname = "navgoal";
	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*QUAKED waypoint_navgoal_8 (0.3 1 0.3) (-8 -8 -24) (8 8 32) SOLID_OK
A waypoint for script navgoals, 8 x 8 size
Not included in navigation data

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

You CANNOT set a radius on these navgoals, they are touch-reach ONLY
*/
void SP_waypoint_navgoal_8( gentity_t *ent )
{
	VectorSet( ent->r.mins, -8, -8, -24 );
	VectorSet( ent->r.maxs, 8, 8, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		Com_Printf(S_COLOR_RED"ERROR: Waypoint_navgoal_8 %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
		assert(0);
	}

	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, 8, RTF_NAVGOAL );

	ent->classname = "navgoal";
	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*QUAKED waypoint_navgoal_4 (0.3 1 0.3) (-4 -4 -24) (4 4 32) SOLID_OK
A waypoint for script navgoals, 4 x 4 size
Not included in navigation data

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

You CANNOT set a radius on these navgoals, they are touch-reach ONLY
*/
void SP_waypoint_navgoal_4( gentity_t *ent )
{
	VectorSet( ent->r.mins, -4, -4, -24 );
	VectorSet( ent->r.maxs, 4, 4, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		Com_Printf(S_COLOR_RED"ERROR: Waypoint_navgoal_4 %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
		assert(0);
	}

	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, 4, RTF_NAVGOAL );

	ent->classname = "navgoal";
	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*QUAKED waypoint_navgoal_2 (0.3 1 0.3) (-2 -2 -24) (2 2 32) SOLID_OK
A waypoint for script navgoals, 2 x 2 size
Not included in navigation data

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

You CANNOT set a radius on these navgoals, they are touch-reach ONLY
*/
void SP_waypoint_navgoal_2( gentity_t *ent )
{
	VectorSet( ent->r.mins, -2, -2, -24 );
	VectorSet( ent->r.maxs, 2, 2, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		Com_Printf(S_COLOR_RED"ERROR: Waypoint_navgoal_2 %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
		assert(0);
	}

	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, 2, RTF_NAVGOAL );

	ent->classname = "navgoal";
	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*QUAKED waypoint_navgoal_1 (0.3 1 0.3) (-1 -1 -24) (1 1 32) SOLID_OK
A waypoint for script navgoals, 1 x 1 size
Not included in navigation data

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

You CANNOT set a radius on these navgoals, they are touch-reach ONLY
*/
void SP_waypoint_navgoal_1( gentity_t *ent )
{
	VectorSet( ent->r.mins, -1, -1, -24 );
	VectorSet( ent->r.maxs, 1, 1, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		Com_Printf(S_COLOR_RED"ERROR: Waypoint_navgoal_1 %s at %s in solid!\n", ent->targetname, vtos(ent->r.currentOrigin));
		assert(0);
	}

	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, 1, RTF_NAVGOAL );

	ent->classname = "navgoal";
	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*
-------------------------
Svcmd_Nav_f
-------------------------
*/

void Svcmd_Nav_f( void )
{
	char cmd[1024];
	trap->Argv( 1, cmd, 1024 );

	if ( Q_stricmp( cmd, "show" ) == 0 )
	{
		trap->Argv(2, cmd, 1024);

		if ( Q_stricmp( cmd, "all" ) == 0 )
		{
			NAVDEBUG_showNodes = !NAVDEBUG_showNodes;

			//NOTENOTE: This causes the two states to sync up if they aren't already
			NAVDEBUG_showCollision = NAVDEBUG_showNavGoals =
			NAVDEBUG_showCombatPoints = NAVDEBUG_showEnemyPath =
			NAVDEBUG_showEdges = NAVDEBUG_showRadius = NAVDEBUG_showNodes;
		}
		else if ( Q_stricmp( cmd, "nodes" ) == 0 )
		{
			NAVDEBUG_showNodes = !NAVDEBUG_showNodes;
		}
		else if ( Q_stricmp( cmd, "radius" ) == 0 )
		{
			NAVDEBUG_showRadius = !NAVDEBUG_showRadius;
		}
		else if ( Q_stricmp( cmd, "edges" ) == 0 )
		{
			NAVDEBUG_showEdges = !NAVDEBUG_showEdges;
		}
		else if ( Q_stricmp( cmd, "testpath" ) == 0 )
		{
			NAVDEBUG_showTestPath = !NAVDEBUG_showTestPath;
		}
		else if ( Q_stricmp( cmd, "enemypath" ) == 0 )
		{
			NAVDEBUG_showEnemyPath = !NAVDEBUG_showEnemyPath;
		}
		else if ( Q_stricmp( cmd, "combatpoints" ) == 0 )
		{
			NAVDEBUG_showCombatPoints = !NAVDEBUG_showCombatPoints;
		}
		else if ( Q_stricmp( cmd, "navgoals" ) == 0 )
		{
			NAVDEBUG_showNavGoals = !NAVDEBUG_showNavGoals;
		}
		else if ( Q_stricmp( cmd, "collision" ) == 0 )
		{
			NAVDEBUG_showCollision = !NAVDEBUG_showCollision;
		}
	}
	else if ( Q_stricmp( cmd, "set" ) == 0 )
	{
		trap->Argv( 2, cmd, 1024 );

		if ( Q_stricmp( cmd, "testgoal" ) == 0 )
		{
			NAVDEBUG_curGoal = trap->Nav_GetNearestNode( (sharedEntity_t *)&g_entities[0], g_entities[0].waypoint, NF_CLEAR_PATH, WAYPOINT_NONE );
		}
	}
	else if ( Q_stricmp( cmd, "totals" ) == 0 )
	{
		Com_Printf("Navigation Totals:\n");
		Com_Printf("------------------\n");
		Com_Printf("Total Nodes:         %d\n", trap->Nav_GetNumNodes() );
		Com_Printf("Total Combat Points: %d\n", level.numCombatPoints );
	}
	else
	{
		//Print the available commands
		Com_Printf("nav - valid commands\n---\n" );
		Com_Printf("show\n - nodes\n - edges\n - testpath\n - enemypath\n - combatpoints\n - navgoals\n---\n");
		Com_Printf("set\n - testgoal\n---\n" );
	}
}

//
//JWEIER ADDITIONS START

qboolean	navCalculatePaths	= qfalse;

qboolean	NAVDEBUG_showNodes			= qfalse;
qboolean	NAVDEBUG_showRadius			= qfalse;
qboolean	NAVDEBUG_showEdges			= qfalse;
qboolean	NAVDEBUG_showTestPath		= qfalse;
qboolean	NAVDEBUG_showEnemyPath		= qfalse;
qboolean	NAVDEBUG_showCombatPoints	= qfalse;
qboolean	NAVDEBUG_showNavGoals		= qfalse;
qboolean	NAVDEBUG_showCollision		= qfalse;
int		NAVDEBUG_curGoal			= 0;

/*
-------------------------
NAV_CalculatePaths
-------------------------
*/
#ifndef FINAL_BUILD
int fatalErrors = 0;
char *fatalErrorPointer = NULL;
char	fatalErrorString[4096];
qboolean NAV_WaypointsTooFar( gentity_t *wp1, gentity_t *wp2 )
{
	if ( Distance( wp1->r.currentOrigin, wp2->r.currentOrigin ) > 1024 )
	{
		char	temp[1024];
		int		len;
		fatalErrors++;
		if ( !wp1->targetname && !wp2->targetname )
		{
			Com_sprintf( temp, sizeof(temp), S_COLOR_RED"Waypoint conn %s->%s > 1024\n", vtos( wp1->r.currentOrigin ), vtos( wp2->r.currentOrigin ) );
		}
		else if ( !wp1->targetname )
		{
			Com_sprintf( temp, sizeof(temp), S_COLOR_RED"Waypoint conn %s->%s > 1024\n", vtos( wp1->r.currentOrigin ), wp2->targetname );
		}
		else if ( !wp2->targetname )
		{
			Com_sprintf( temp, sizeof(temp), S_COLOR_RED"Waypoint conn %s->%s > 1024\n", wp1->targetname, vtos( wp2->r.currentOrigin ) );
		}
		else
		{//they both have valid targetnames
			Com_sprintf( temp, sizeof(temp), S_COLOR_RED"Waypoint conn %s->%s > 1024\n", wp1->targetname, wp2->targetname );
		}
		len = strlen( temp );
		if ( (fatalErrorPointer-fatalErrorString)+len >= sizeof( fatalErrorString ) )
		{
			Com_Error( ERR_DROP, "%s%s%dTOO MANY FATAL NAV ERRORS!!!\n", fatalErrorString, temp, fatalErrors );
			return qtrue;
		}
		strcat( fatalErrorPointer, temp );
		fatalErrorPointer += len;
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}
#endif

static int numStoredWaypoints = 0;
//static waypointData_t *tempWaypointList=0;
static waypointData_t tempWaypointList[MAX_STORED_WAYPOINTS]; //rwwFIXMEFIXME: Need.. dynamic.. memory


void NAV_ClearStoredWaypoints(void)
{
	numStoredWaypoints = 0;
}


void NAV_StoreWaypoint( gentity_t *ent )
{
	/*
	if ( !tempWaypointList )
	{
		//tempWaypointList = (waypointData_t *) trap->Malloc(sizeof(waypointData_t)*MAX_STORED_WAYPOINTS, TAG_TEMP_WORKSPACE, qtrue);

		int size = sizeof(waypointData_t)*MAX_STORED_WAYPOINTS;
		trap->TrueMalloc((void **)&tempWaypointList, size);
		memset(tempWaypointList, 0, size);
	}
	*/

	if ( numStoredWaypoints >= MAX_STORED_WAYPOINTS )
	{
		//trap->Error( ERR_DROP, "Too many waypoints!  (%d > %d)\n", numStoredWaypoints, MAX_STORED_WAYPOINTS );
		//rwwFIXMEFIXME: commented this out so I can load some of the SP levels.
		return;
	}
	if ( ent->targetname )
	{
		Q_strncpyz( tempWaypointList[numStoredWaypoints].targetname, ent->targetname, MAX_QPATH );
	}
	if ( ent->target )
	{
		Q_strncpyz( tempWaypointList[numStoredWaypoints].target, ent->target, MAX_QPATH );
	}
	if ( ent->target2 )
	{
		Q_strncpyz( tempWaypointList[numStoredWaypoints].target2, ent->target2, MAX_QPATH );
	}
	if ( ent->target3 )
	{
		Q_strncpyz( tempWaypointList[numStoredWaypoints].target3, ent->target3, MAX_QPATH );
	}
	if ( ent->target4 )
	{
		Q_strncpyz( tempWaypointList[numStoredWaypoints].target4, ent->target4, MAX_QPATH );
	}
	tempWaypointList[numStoredWaypoints].nodeID = ent->health;

	numStoredWaypoints++;
}

int NAV_GetStoredWaypoint( char *targetname )
{
	int i;

	if ( !targetname || !targetname[0] )
	{
		return -1;
	}
	for ( i = 0; i < numStoredWaypoints; i++ )
	{
		if ( tempWaypointList[i].targetname[0] )
		{
			if ( !Q_stricmp( targetname, tempWaypointList[i].targetname ) )
			{
				return i;
			}
		}
	}
	return -1;
}

void NAV_CalculatePaths( const char *filename, int checksum )
{
	int target = -1;
	int i;

#ifdef _DISABLED
	if ( !tempWaypointList )
	{
		return;
	}
#endif //_DISABLED

#ifndef FINAL_BUILD
	fatalErrors = 0;
	memset( fatalErrorString, 0, sizeof( fatalErrorString ) );
	fatalErrorPointer = &fatalErrorString[0];
#endif
#if _HARD_CONNECT

	//Find all connections and hard connect them
	for ( i = 0; i < numStoredWaypoints; i++ )
	{
		//Find the first connection
		target = NAV_GetStoredWaypoint( tempWaypointList[i].target );

		if ( target != -1 )
		{
#ifndef FINAL_BUILD
//			if ( !NAV_WaypointsTooFar( ent, target ) )
#endif
			{
				trap->Nav_HardConnect( tempWaypointList[i].nodeID, tempWaypointList[target].nodeID );
			}
		}

		//Find a possible second connection
		target = NAV_GetStoredWaypoint( tempWaypointList[i].target2 );

		if ( target != -1 )
		{
#ifndef FINAL_BUILD
//			if ( !NAV_WaypointsTooFar( ent, target ) )
#endif
			{
				trap->Nav_HardConnect( tempWaypointList[i].nodeID, tempWaypointList[target].nodeID );
			}
		}

		//Find a possible third connection
		target = NAV_GetStoredWaypoint( tempWaypointList[i].target3 );

		if ( target != -1 )
		{
#ifndef FINAL_BUILD
//			if ( !NAV_WaypointsTooFar( ent, target ) )
#endif
			{
				trap->Nav_HardConnect( tempWaypointList[i].nodeID, tempWaypointList[target].nodeID );
			}
		}

		//Find a possible fourth connection
		target = NAV_GetStoredWaypoint( tempWaypointList[i].target4 );

		if ( target != -1 )
		{
#ifndef FINAL_BUILD
//			if ( !NAV_WaypointsTooFar( ent, target ) )
#endif
			{
				trap->Nav_HardConnect( tempWaypointList[i].nodeID, tempWaypointList[target].nodeID );
			}
		}
	}

#endif

	//Remove all waypoints now that they're done
	//trap->Free(tempWaypointList);
	/*
	trap->TrueFree((void **)&tempWaypointList);
	tempWaypointList=0;
	*/

	//Now check all blocked edges, mark failed ones
	trap->Nav_CheckBlockedEdges();

	trap->Nav_SetPathsCalculated(qfalse);
	//navigator.pathsCalculated = qfalse;

	//Calculate the paths based on the supplied waypoints
	//trap->Nav_CalculatePaths();

	//Save the resulting information
	/*
	if ( trap->Nav_Save( filename, checksum ) == qfalse )
	{
		Com_Printf("Unable to save navigations data for map \"%s\" (checksum:%d)\n", filename, checksum );
	}
	*/
#ifndef FINAL_BUILD
	if ( fatalErrors )
	{
		//Com_Error( ERR_DROP, "%s%d FATAL NAV ERRORS\n", fatalErrorString, fatalErrors );
		Com_Printf( "%s%d FATAL NAV ERRORS\n", fatalErrorString, fatalErrors );
	}
#endif
}

/*
-------------------------
NAV_Shutdown
-------------------------
*/

void NAV_Shutdown( void )
{
	trap->Nav_Free();
}

/*
-------------------------
NAV_ShowDebugInfo
-------------------------
*/

void NAV_ShowDebugInfo( void )
{
	int i;

	if ( NAVDEBUG_showNodes )
	{
		trap->Nav_ShowNodes();
	}

	if ( NAVDEBUG_showEdges )
	{
		trap->Nav_ShowEdges();
	}

	if ( NAVDEBUG_showTestPath )
	{
		//Get the nearest node to the player
		int	nearestNode = trap->Nav_GetNearestNode( (sharedEntity_t *)&g_entities[0], g_entities[0].waypoint, NF_ANY, WAYPOINT_NONE );
		int	testNode = trap->Nav_GetBestNode( nearestNode, NAVDEBUG_curGoal, NODE_NONE );
		vec3_t	dest, start;

		nearestNode = NAV_TestBestNode( &g_entities[0], nearestNode, testNode, qfalse );

		//Show the connection

		//Get the positions
		trap->Nav_GetNodePosition( NAVDEBUG_curGoal, dest );
		trap->Nav_GetNodePosition( nearestNode, start );

		G_DrawNode( start, NODE_START );
		G_DrawNode( dest, NODE_GOAL );
		trap->Nav_ShowPath( nearestNode, NAVDEBUG_curGoal );
	}

	if ( NAVDEBUG_showCombatPoints )
	{
		for ( i = 0; i < level.numCombatPoints; i++ )
		{
			G_DrawCombatPoint( level.combatPoints[i].origin, 0 );
		}
	}

	if ( NAVDEBUG_showNavGoals )
	{
		TAG_ShowTags( RTF_NAVGOAL );
	}
}

/*
-------------------------
NAV_FindPlayerWaypoint
-------------------------
*/

void NAV_FindPlayerWaypoint( int clNum )
{
	g_entities[clNum].waypoint = trap->Nav_GetNearestNode( (sharedEntity_t *)&g_entities[clNum], g_entities[clNum].lastWaypoint, NF_CLEAR_PATH, WAYPOINT_NONE );
}

//
//JWEIER ADDITIONS END
