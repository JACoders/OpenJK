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

//b_goal.cpp
#include "b_local.h"
#include "icarus/Q3_Interface.h"

extern qboolean FlyingCreature( gentity_t *ent );
/*
SetGoal
*/

void SetGoal( gentity_t *goal, float rating )
{
	NPCS.NPCInfo->goalEntity = goal;
//	NPCInfo->goalEntityNeed = rating;
	NPCS.NPCInfo->goalTime = level.time;
//	NAV_ClearLastRoute(NPC);
	if ( goal )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: %s @ %s (%f)\n", goal->classname, vtos( goal->currentOrigin), rating );
	}
	else
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: NONE\n" );
	}
}


/*
NPC_SetGoal
*/

void NPC_SetGoal( gentity_t *goal, float rating )
{
	if ( goal == NPCS.NPCInfo->goalEntity )
	{
		return;
	}

	if ( !goal )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_ERROR, "NPC_SetGoal: NULL goal\n" );
		return;
	}

	if ( goal->client )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_ERROR, "NPC_SetGoal: goal is a client\n" );
		return;
	}

	if ( NPCS.NPCInfo->goalEntity )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: push %s\n", NPCInfo->goalEntity->classname );
		NPCS.NPCInfo->lastGoalEntity = NPCS.NPCInfo->goalEntity;
//		NPCInfo->lastGoalEntityNeed = NPCInfo->goalEntityNeed;
	}

	SetGoal( goal, rating );
}


/*
NPC_ClearGoal
*/

void NPC_ClearGoal( void )
{
	gentity_t	*goal;

	if ( !NPCS.NPCInfo->lastGoalEntity )
	{
		SetGoal( NULL, 0.0 );
		return;
	}

	goal = NPCS.NPCInfo->lastGoalEntity;
	NPCS.NPCInfo->lastGoalEntity = NULL;
//	NAV_ClearLastRoute(NPC);
	if ( goal->inuse && !(goal->s.eFlags & EF_NODRAW) )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_ClearGoal: pop %s\n", goal->classname );
		SetGoal( goal, 0 );//, NPCInfo->lastGoalEntityNeed
		return;
	}

	SetGoal( NULL, 0.0 );
}

/*
-------------------------
G_BoundsOverlap
-------------------------
*/

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2)
{//NOTE: flush up against counts as overlapping
	if(mins1[0]>maxs2[0])
		return qfalse;

	if(mins1[1]>maxs2[1])
		return qfalse;

	if(mins1[2]>maxs2[2])
		return qfalse;

	if(maxs1[0]<mins2[0])
		return qfalse;

	if(maxs1[1]<mins2[1])
		return qfalse;

	if(maxs1[2]<mins2[2])
		return qfalse;

	return qtrue;
}

void NPC_ReachedGoal( void )
{
//	Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "UpdateGoal: reached goal entity\n" );
	NPC_ClearGoal();
	NPCS.NPCInfo->goalTime = level.time;

//MCG - Begin
	NPCS.NPCInfo->aiFlags &= ~NPCAI_MOVING;
	NPCS.ucmd.forwardmove = 0;
	//Return that the goal was reached
	trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPCS.NPC, TID_MOVE_NAV );
//MCG - End
}
/*
ReachedGoal

id removed checks against waypoints and is now checking surfaces
*/
//qboolean NAV_HitNavGoal( vec3_t point, vec3_t mins, vec3_t maxs, gentity_t *goal, qboolean flying );
qboolean ReachedGoal( gentity_t *goal )
{
	//FIXME: For script waypoints, need a special check
/*
	int		goalWpNum;
	vec3_t	vec;
	//vec3_t	angles;
	float	delta;

	if ( goal->flags & FL_NAVGOAL )
	{//waypoint_navgoal
		return NAV_HitNavGoal( NPC->currentOrigin, NPC->mins, NPC->maxs, goal, FlyingCreature( NPC ) );
	}

	if ( goal == NPCInfo->tempGoal && !(goal->flags & FL_NAVGOAL))
	{//MUST touch waypoints, even if moving to it
		//This is odd, it just checks to see if they are on the same
		//surface and the tempGoal in in the FOV - does NOT check distance!
		// are we on same surface?

		//FIXME: NPC->waypoint reset every frame, need to find it first
		//Should we do that here?  (Still will do it only once per frame)
		if ( NPC->waypoint >= 0 && NPC->waypoint < num_waypoints )
		{
			goalWpNum = NAV_FindWaypointAt ( goal->currentOrigin );
			if ( NPC->waypoint != goalWpNum )
			{
				return qfalse;
			}
		}

		VectorSubtract ( NPCInfo->tempGoal->currentOrigin, NPC->currentOrigin, vec);
		//Who cares if it's in our FOV?!
		/*
		// is it in our FOV
		vectoangles ( vec, angles );
		delta = AngleDelta ( NPC->client->ps.viewangles[YAW], angles[YAW] );
		if ( fabs ( delta ) > NPCInfo->stats.hfov )
		{
			return qfalse;
		}
		*/

		/*
		//If in the same waypoint as tempGoal, we're there, right?
		if ( goal->waypoint >= 0 && goal->waypoint < num_waypoints )
		{
			if ( NPC->waypoint == goal->waypoint )
			{
				return qtrue;
			}
		}
		*/

/*
		if ( VectorLengthSquared( vec ) < (64*64) )
		{//Close enough
			return qtrue;
		}

		return qfalse;
	}
*/
	if ( NPCS.NPCInfo->aiFlags & NPCAI_TOUCHED_GOAL )
	{
		NPCS.NPCInfo->aiFlags &= ~NPCAI_TOUCHED_GOAL;
		return qtrue;
	}
/*
	if ( goal->s.eFlags & EF_NODRAW )
	{
		goalWpNum = NAV_FindWaypointAt( goal->currentOrigin );
		if ( NPC->waypoint == goalWpNum )
		{
			return qtrue;
		}
		return qfalse;
	}

	if(goal->client && goal->health <= 0)
	{//trying to get to dead guy
		goalWpNum = NAV_FindWaypointAt( goal->currentOrigin );
		if ( NPC->waypoint == goalWpNum )
		{
			VectorSubtract(NPC->currentOrigin, goal->currentOrigin, vec);
			vec[2] = 0;
			delta = VectorLengthSquared(vec);
			if(delta <= 800)
			{//with 20-30 of other guy's origin
				return qtrue;
			}
		}
	}
*/
	return NAV_HitNavGoal( NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, goal->r.currentOrigin, NPCS.NPCInfo->goalRadius, FlyingCreature( NPCS.NPC ) );
}

/*
static gentity_t *UpdateGoal( void )

Id removed a lot of shit here... doesn't seem to handle waypoints independently of goalentity

In fact, doesn't seem to be any waypoint info on entities at all any more?

MCG - Since goal is ALWAYS goalEntity, took out a lot of sending goal entity pointers around for no reason
*/

gentity_t *UpdateGoal( void )
{
	gentity_t	*goal;

	if ( !NPCS.NPCInfo->goalEntity )
	{
		return NULL;
	}

	if ( !NPCS.NPCInfo->goalEntity->inuse )
	{//Somehow freed it, but didn't clear it
		NPC_ClearGoal();
		return NULL;
	}

	goal = NPCS.NPCInfo->goalEntity;

	if ( ReachedGoal( goal ) )
	{
		NPC_ReachedGoal();
		goal = NULL;//so they don't keep trying to move to it
	}//else if fail, need to tell script so?

	return goal;
}
