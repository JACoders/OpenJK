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
#include "g_navigator.h"

//Global navigator
//CNavigator		navigator;

extern qboolean G_EntIsUnlockedDoor( int entityNum );
extern qboolean G_EntIsDoor( int entityNum );
extern qboolean G_EntIsRemovableUsable( int entNum );
extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
//For debug graphics
extern void CG_Line( vec3_t start, vec3_t end, vec3_t color, float alpha );
extern void CG_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void CG_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha );
extern qboolean FlyingCreature( gentity_t *ent );


extern vec3_t NPCDEBUG_RED;


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
	VectorCopy( point, ent->NPC->tempGoal->currentOrigin );
	
	//Copy the mins and maxs to the tempGoal
	VectorCopy( ent->mins, ent->NPC->tempGoal->mins );
	VectorCopy( ent->mins, ent->NPC->tempGoal->maxs );

	//FIXME: TESTING let's try making sure the tempGoal isn't stuck in the ground?
	if ( 0 )
	{
		trace_t	trace;
		vec3_t	bottom = {ent->NPC->tempGoal->currentOrigin[0],ent->NPC->tempGoal->currentOrigin[1],ent->NPC->tempGoal->currentOrigin[2]+ent->NPC->tempGoal->mins[2]};
		gi.trace( &trace, ent->NPC->tempGoal->currentOrigin, vec3_origin, vec3_origin, bottom, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
		if ( trace.fraction < 1.0f )
		{//in the ground, raise it up
			ent->NPC->tempGoal->currentOrigin[2] -= ent->NPC->tempGoal->mins[2]*(1.0f-trace.fraction)-0.125f;
		}
	}

	ent->NPC->tempGoal->target = NULL;
	ent->NPC->tempGoal->clipmask = ent->clipmask;
	ent->NPC->tempGoal->svFlags &= ~SVF_NAVGOAL;
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
		assert(ent->NPC->tempGoal->owner);
		ent->NPC->tempGoal->svFlags |= SVF_NAVGOAL;
	}

	ent->NPC->tempGoal->combatPoint = combatPoint;
	ent->NPC->tempGoal->enemy = targetEnt;

	ent->NPC->goalEntity = ent->NPC->tempGoal;
	ent->NPC->goalRadius = radius;
	ent->NPC->aiFlags	&= ~NPCAI_STOP_AT_LOS;

	gi.linkentity( ent->NPC->goalEntity );
}
/*
-------------------------
waypoint_testDirection
-------------------------
*/

static float waypoint_testDirection( vec3_t origin, float yaw, float minDist )
{
	vec3_t	trace_dir, test_pos;
	vec3_t	maxs, mins;
	trace_t	tr;

	//Setup the mins and max
	VectorSet( maxs, DEFAULT_MAXS_0, DEFAULT_MAXS_1, DEFAULT_MAXS_2 );
	VectorSet( mins, DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2 + STEPSIZE );

	//Get our test direction
	vec3_t	angles = { 0, yaw, 0 };
	AngleVectors( angles, trace_dir, NULL, NULL );

	//Move ahead
	VectorMA( origin, minDist, trace_dir, test_pos );

	gi.trace( &tr, origin, mins, maxs, test_pos, ENTITYNUM_NONE, ( CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_BOTCLIP ), (EG2_Collision)0, 0 );

	return ( minDist * tr.fraction );	//return actual dist completed
}

/*
-------------------------
waypoint_getRadius
-------------------------
*/

static float waypoint_getRadius( gentity_t *ent )
{
	float	minDist = MAX_RADIUS_CHECK + 1; // (unsigned int) -1;
	float	dist;

	for ( int i = 0; i < YAW_ITERATIONS; i++ )
	{
		dist = waypoint_testDirection( ent->currentOrigin, ((360.0f/YAW_ITERATIONS) * i), minDist );

		if ( dist < minDist )
			minDist = dist;
	}

	return minDist + DEFAULT_MAXS_0;
}

/*QUAKED waypoint  (0.7 0.7 0) (-20 -20 -24) (20 20 45) SOLID_OK DROP_TO_FLOOR
a place to go.

SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)
DROP_TO_FLOOR - will cause the point to auto drop to the floor

radius is automatically calculated in-world.
"targetJump" is a special edge that only guys who can jump will cross (so basically Jedi)
*/
extern int delayedShutDown;
void SP_waypoint ( gentity_t *ent )
{
		VectorSet(ent->mins, DEFAULT_MINS_0, DEFAULT_MINS_1, DEFAULT_MINS_2);
		VectorSet(ent->maxs, DEFAULT_MAXS_0, DEFAULT_MAXS_1, DEFAULT_MAXS_2);
		
		ent->contents = CONTENTS_TRIGGER;
		ent->clipmask = MASK_DEADSOLID;

		gi.linkentity( ent );

		ent->count = -1;
		ent->classname = "waypoint";

		if (ent->spawnflags&2)
		{
			ent->currentOrigin[2] += 128.0f;
		}

		if( !(ent->spawnflags&1) && G_CheckInSolid (ent, qtrue))
		{//if not SOLID_OK, and in solid
			ent->maxs[2] = CROUCH_MAXS_2;
			if(G_CheckInSolid (ent, qtrue))
			{
				gi.Printf(S_COLOR_RED"ERROR: Waypoint %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
				assert(0 && "Waypoint in solid!");
//				if (!g_entities[ENTITYNUM_WORLD].s.radius){	//not a region
//					G_Error("Waypoint %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
//				}
				delayedShutDown = level.time + 100;
				G_FreeEntity(ent);
				return;
			}
		}

		//G_SpawnString("targetJump", "", &ent->targetJump);
		ent->radius = waypoint_getRadius( ent );
		NAV::SpawnedPoint(ent);

		G_FreeEntity(ent);
		return;
}

/*QUAKED waypoint_small  (0.7 0.7 0) (-2 -2 -24) (2 2 32) SOLID_OK
SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)
DROP_TO_FLOOR - will cause the point to auto drop to the floor
*/
void SP_waypoint_small (gentity_t *ent)
{
		VectorSet(ent->mins, -2, -2, DEFAULT_MINS_2);
		VectorSet(ent->maxs, 2, 2, DEFAULT_MAXS_2);

		ent->contents = CONTENTS_TRIGGER;
		ent->clipmask = MASK_DEADSOLID;

		gi.linkentity( ent );

		ent->count = -1;
		ent->classname = "waypoint";

		if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qtrue ) )
		{
			ent->maxs[2] = CROUCH_MAXS_2;
			if ( G_CheckInSolid( ent, qtrue ) )
			{
				gi.Printf(S_COLOR_RED"ERROR: Waypoint_small %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
				assert(0);
#ifndef FINAL_BUILD
				if (!g_entities[ENTITYNUM_WORLD].s.radius){	//not a region
					G_Error("Waypoint_small %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
				}
#endif
				G_FreeEntity(ent);
				return;
			}
		}

		ent->radius = 2;	// radius
		NAV::SpawnedPoint(ent);

		G_FreeEntity(ent);
		return;
}


/*QUAKED waypoint_navgoal (0.3 1 0.3) (-20 -20 -24) (20 20 40) SOLID_OK DROP_TO_FLOOR NO_AUTO_CONNECT
A waypoint for script navgoals
Not included in navigation data

DROP_TO_FLOOR - will cause the point to auto drop to the floor
NO_AUTO_CONNECT - will not automatically connect to any other points, you must then connect it by hand


SOLID_OK - only use if placing inside solid is unavoidable in map, but may be clear in-game (ie: at the bottom of a tall, solid lift that starts at the top position)

targetname - name you would use in script when setting a navgoal (like so:)

  For example: if you give this waypoint a targetname of "console", make an NPC go to it in a script like so:

  set ("navgoal", "console");

radius - how far from the navgoal an ent can be before it thinks it reached it - default is "0" which means no radius check, just have to touch it

*/

void SP_waypoint_navgoal( gentity_t *ent )
{
	int radius = ( ent->radius ) ? (ent->radius) : 12;

	VectorSet( ent->mins, -16, -16, -24 );
	VectorSet( ent->maxs, 16, 16, 32 );
	ent->s.origin[2] += 0.125;
	if ( !(ent->spawnflags&1) && G_CheckInSolid( ent, qfalse ) )
	{
		gi.Printf(S_COLOR_RED"ERROR: Waypoint_navgoal %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
		assert(0);
#ifndef FINAL_BUILD
		if (!g_entities[ENTITYNUM_WORLD].s.radius){	//not a region
			G_Error("Waypoint_navgoal %s at %s in solid!\n", ent->targetname, vtos(ent->currentOrigin));
		}
#endif
	}
	TAG_Add( ent->targetname, NULL, ent->s.origin, ent->s.angles, radius, RTF_NAVGOAL );

	ent->classname = "navgoal";
	
	NAV::SpawnedPoint(ent, NAV::PT_GOALNODE);

	G_FreeEntity( ent );//can't do this, they need to be found later by some functions, though those could be fixed, maybe?
}

/*
-------------------------
Svcmd_Nav_f
-------------------------
*/

void Svcmd_Nav_f( void )
{
	const char	*cmd = gi.argv( 1 );

	if ( Q_stricmp( cmd, "show" ) == 0 )
	{
		cmd = gi.argv( 2 );

		if ( Q_stricmp( cmd, "all" ) == 0 )
		{
			NAVDEBUG_showNodes = !NAVDEBUG_showNodes;
			
			//NOTENOTE: This causes the two states to sync up if they aren't already
			NAVDEBUG_showCollision = NAVDEBUG_showNavGoals = 
			NAVDEBUG_showCombatPoints = NAVDEBUG_showEnemyPath = 
			NAVDEBUG_showEdges = NAVDEBUG_showNearest = NAVDEBUG_showRadius = NAVDEBUG_showNodes;		
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
		else if ( Q_stricmp( cmd, "grid" ) == 0 )
		{
			NAVDEBUG_showGrid = !NAVDEBUG_showGrid;
		}	
		else if ( Q_stricmp( cmd, "nearest" ) == 0 )
		{
			NAVDEBUG_showNearest = !NAVDEBUG_showNearest;
		}
		else if ( Q_stricmp( cmd, "lines" ) == 0 )
		{
			NAVDEBUG_showPointLines = !NAVDEBUG_showPointLines;
		}
	}
	else if ( Q_stricmp( cmd, "set" ) == 0 )
	{
		cmd = gi.argv( 2 );

		if ( Q_stricmp( cmd, "testgoal" ) == 0 )
		{
		//	NAVDEBUG_curGoal = navigator.GetNearestNode( &g_entities[0], g_entities[0].waypoint, NF_CLEAR_PATH, WAYPOINT_NONE );
		}
	}
	else if ( Q_stricmp( cmd, "goto" ) == 0 )
	{
		cmd = gi.argv( 2 );
		NAV::TeleportTo(&(g_entities[0]), cmd);
	}
	else if ( Q_stricmp( cmd, "gotonum" ) == 0 )
	{
		cmd = gi.argv( 2 );
		NAV::TeleportTo(&(g_entities[0]), atoi(cmd));
	}
	else if ( Q_stricmp( cmd, "totals" ) == 0 )
	{
		NAV::ShowStats();
	}
	else
	{
		//Print the available commands
		Com_Printf("nav - valid commands\n---\n" );
		Com_Printf("show\n - nodes\n - edges\n - testpath\n - enemypath\n - combatpoints\n - navgoals\n---\n");
		Com_Printf("goto\n ---\n" );
		Com_Printf("gotonum\n ---\n" );
		Com_Printf("totals\n ---\n" );
		Com_Printf("set\n - testgoal\n---\n" );
	}
}

//
//JWEIER ADDITIONS START

bool	navCalculatePaths	= false;

bool	NAVDEBUG_showNodes			= false;
bool	NAVDEBUG_showRadius			= false;
bool	NAVDEBUG_showEdges			= false;
bool	NAVDEBUG_showTestPath		= false;
bool	NAVDEBUG_showEnemyPath		= false;
bool	NAVDEBUG_showCombatPoints	= false;
bool	NAVDEBUG_showNavGoals		= false;
bool	NAVDEBUG_showCollision		= false;
int		NAVDEBUG_curGoal			= 0;
bool	NAVDEBUG_showGrid			= false;
bool	NAVDEBUG_showNearest		= false;
bool	NAVDEBUG_showPointLines		= false;


//
//JWEIER ADDITIONS END
