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

/*****************************************************************************
 * name:		be_aas_debug.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_debug.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/
#pragma once

//clear the shown debug lines
void AAS_ClearShownDebugLines(void);
//
void AAS_ClearShownPolygons(void);
//show a debug line
void AAS_DebugLine(vec3_t start, vec3_t end, int color);
//show a permenent line
void AAS_PermanentLine(vec3_t start, vec3_t end, int color);
//show a permanent cross
void AAS_DrawPermanentCross(vec3_t origin, float size, int color);
//draw a cross in the plane
void AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color);
//show a bounding box
void AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs);
//show a face
void AAS_ShowFace(int facenum);
//show an area
void AAS_ShowArea(int areanum, int groundfacesonly);
//
void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly);
//draw a cros
void AAS_DrawCross(vec3_t origin, float size, int color);
//print the travel type
void AAS_PrintTravelType(int traveltype);
//draw an arrow
void AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor);
//visualize the given reachability
void AAS_ShowReachability(struct aas_reachability_s *reach);
//show the reachable areas from the given area
void AAS_ShowReachableAreas(int areanum);
