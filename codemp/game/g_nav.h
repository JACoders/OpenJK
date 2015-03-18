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

#pragma once

//This file is shared by the exe nav code.
//If you modify it without recompiling the exe with new code, there could be issues.

#define	WAYPOINT_NONE	-1

#define MAX_STORED_WAYPOINTS	512//???
#define MAX_WAYPOINT_REACHED_DIST_SQUARED	1024	//32 squared
#define	MAX_COLL_AVOID_DIST					128
#define	NAVGOAL_USE_RADIUS					16384	//Used to force the waypoint_navgoals with a manually set radius to actually do a DistanceSquared check, not just bounds overlap

#define	MIN_STOP_DIST 64
#define	MIN_BLOCKED_SPEECH_TIME	4000
#define	MIN_DOOR_BLOCK_DIST			16
#define	MIN_DOOR_BLOCK_DIST_SQR		( MIN_DOOR_BLOCK_DIST * MIN_DOOR_BLOCK_DIST )
#define	SHOVE_SPEED	200
#define SHOVE_LIFT	10
#define	MAX_RADIUS_CHECK	1024
#define	YAW_ITERATIONS	16


// Engine has its own copy of these (navigator.h)

#ifdef _GAME
	//rww - Rest of defines here are also shared in exe, do not modify.
	#define	__NEWCOLLECT	1

	#define _HARD_CONNECT	1

	//Node flags
	#define	NF_ANY			0
	//#define	NF_CLEAR_LOS	0x00000001
	#define NF_CLEAR_PATH	0x00000002
	#define NF_RECALC		0x00000004

	//Edge flags
	#define	EFLAG_NONE		0
	#define EFLAG_BLOCKED	0x00000001
	#define EFLAG_FAILED	0x00000002

	//Miscellaneous defines
	#define	NODE_NONE		-1
	#define	NAV_HEADER_ID	'JNV5'
	#define	NODE_HEADER_ID	'NODE'

	//this stuff is local and can be modified, don't even show it to the engine.
	extern	qboolean navCalculatePaths;

	extern	qboolean NAVDEBUG_showNodes;
	extern	qboolean NAVDEBUG_showRadius;
	extern	qboolean NAVDEBUG_showEdges;
	extern	qboolean NAVDEBUG_showTestPath;
	extern	qboolean NAVDEBUG_showEnemyPath;
	extern	qboolean NAVDEBUG_showCombatPoints;
	extern	qboolean NAVDEBUG_showNavGoals;
	extern	qboolean NAVDEBUG_showCollision;

	extern	int	 NAVDEBUG_curGoal;

	void NAV_Shutdown( void );
	void NAV_CalculatePaths( const char *filename, int checksum );
	void NAV_CalculateSquadPaths( const char *filename, int checksum );

	void NAV_ShowDebugInfo( void );

	int NAV_GetNearestNode( gentity_t *self, int lastNode );
	extern int NAV_TestBestNode( gentity_t *self, int startID, int endID, qboolean failEdge );

	qboolean NPC_GetMoveDirection( vec3_t out, float *distance );
	void NPC_MoveToGoalExt( vec3_t point );
	void NAV_FindPlayerWaypoint( int clNum );
	qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t *trace, int clipmask );
#endif
