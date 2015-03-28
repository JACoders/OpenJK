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

#ifndef __G_NAV_H__
#define __G_NAV_H__

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

extern	bool navCalculatePaths;

extern	bool NAVDEBUG_showNodes;
extern	bool NAVDEBUG_showRadius;
extern	bool NAVDEBUG_showEdges;
extern	bool NAVDEBUG_showTestPath;
extern	bool NAVDEBUG_showEnemyPath;
extern	bool NAVDEBUG_showCombatPoints;
extern	bool NAVDEBUG_showNavGoals;
extern	bool NAVDEBUG_showCollision;

extern	int	 NAVDEBUG_curGoal;

void NAV_Shutdown( void );
void NAV_CalculatePaths( const char *filename, int checksum );
void NAV_CalculateSquadPaths( const char *filename, int checksum );

void NAV_ShowDebugInfo( void );

int NAV_GetNearestNode( gentity_t *self, int lastNode );
extern int NAV_TestBestNode( gentity_t *self, int startID, int endID, qboolean failEdge );

qboolean NPC_GetMoveDirection( vec3_t out, float *distance );
void NPC_MoveToGoalExt( vec3_t point );
void NAV_FindPlayerWaypoint( void );
qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t &trace, int clipmask );

void CG_DrawNode( vec3_t origin, int type );
void CG_DrawEdge( vec3_t start, vec3_t end, int type );
void CG_DrawRadius( vec3_t origin, unsigned int radius, int type );
void CG_DrawCombatPoint( vec3_t origin, int type );

#endif //#ifndef __G_NAV_H__
