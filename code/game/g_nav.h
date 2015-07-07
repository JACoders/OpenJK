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

#define	WAYPOINT_NONE	0

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
extern	bool NAVDEBUG_showGrid;
extern	bool NAVDEBUG_showNearest;
extern	int	 NAVDEBUG_curGoal;
extern	bool NAVDEBUG_showPointLines;


void CG_DrawNode( vec3_t origin, int type );
void CG_DrawEdge( vec3_t start, vec3_t end, int type );
void CG_DrawRadius( vec3_t origin, unsigned int radius, int type );
void CG_DrawCombatPoint( vec3_t origin, int type );

#endif //#ifndef __G_NAV_H__
