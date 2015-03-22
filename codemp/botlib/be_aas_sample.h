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
 * name:		be_aas_sample.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_sample.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#pragma once

#ifdef AASINTERN
void AAS_InitAASLinkHeap(void);
void AAS_InitAASLinkedEntities(void);
void AAS_FreeAASLinkHeap(void);
void AAS_FreeAASLinkedEntities(void);
aas_face_t *AAS_AreaGroundFace(int areanum, vec3_t point);
aas_face_t *AAS_TraceEndFace(aas_trace_t *trace);
aas_plane_t *AAS_PlaneFromNum(int planenum);
aas_link_t *AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum);
aas_link_t *AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype);
qboolean AAS_PointInsideFace(int facenum, vec3_t point, float epsilon);
qboolean AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon);
void AAS_UnlinkFromAreas(aas_link_t *areas);
#endif //AASINTERN

//returns the mins and maxs of the bounding box for the given presence type
void AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
//returns the cluster the area is in (negative portal number if the area is a portal)
int AAS_AreaCluster(int areanum);
//returns the presence type(s) of the area
int AAS_AreaPresenceType(int areanum);
//returns the presence type(s) at the given point
int AAS_PointPresenceType(vec3_t point);
//returns the result of the trace of a client bbox
aas_trace_t AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent);
//stores the areas the trace went through and returns the number of passed areas
int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);
//returns the areas the bounding box is in
int AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
//return area information
int AAS_AreaInfo( int areanum, aas_areainfo_t *info );
//returns the area the point is in
int AAS_PointAreaNum(vec3_t point);
//
int AAS_PointReachabilityAreaIndex( vec3_t point );
//returns the plane the given face is in
void AAS_FacePlane(int facenum, vec3_t normal, float *dist);
