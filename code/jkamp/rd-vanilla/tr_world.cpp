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

#include "tr_local.h"

inline void Q_CastShort2Float(float *f, const short *s)
{
	*f = ((float)*s);
}


/*
=================
R_CullTriSurf

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullTriSurf( srfTriangles_t *cv ) {
	int 	boxCull;

	boxCull = R_CullLocalBox( cv->bounds );

	if ( boxCull == CULL_OUT ) {
		return qtrue;
	}
	return qfalse;
}

/*
=================
R_CullGrid

Returns true if the grid is completely culled away.
Also sets the clipped hint bit in tess
=================
*/
static qboolean	R_CullGrid( srfGridMesh_t *cv ) {
	int 	boxCull;
	int 	sphereCull;

	if ( r_nocurves->integer ) {
		return qtrue;
	}

	if ( tr.currentEntityNum != REFENTITYNUM_WORLD ) {
		sphereCull = R_CullLocalPointAndRadius( cv->localOrigin, cv->meshRadius );
	} else {
		sphereCull = R_CullPointAndRadius( cv->localOrigin, cv->meshRadius );
	}
	boxCull = CULL_OUT;

	// check for trivial reject
	if ( sphereCull == CULL_OUT )
	{
		tr.pc.c_sphere_cull_patch_out++;
		return qtrue;
	}
	// check bounding box if necessary
	else if ( sphereCull == CULL_CLIP )
	{
		tr.pc.c_sphere_cull_patch_clip++;

		boxCull = R_CullLocalBox( cv->meshBounds );

		if ( boxCull == CULL_OUT )
		{
			tr.pc.c_box_cull_patch_out++;
			return qtrue;
		}
		else if ( boxCull == CULL_IN )
		{
			tr.pc.c_box_cull_patch_in++;
		}
		else
		{
			tr.pc.c_box_cull_patch_clip++;
		}
	}
	else
	{
		tr.pc.c_sphere_cull_patch_in++;
	}

	return qfalse;
}


/*
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean	R_CullSurface( surfaceType_t *surface, shader_t *shader ) {
	srfSurfaceFace_t *sface;
	float			d;

	if ( r_nocull->integer ) {
		return qfalse;
	}

	if ( *surface == SF_GRID ) {
		return R_CullGrid( (srfGridMesh_t *)surface );
	}

	if ( *surface == SF_TRIANGLES ) {
		return R_CullTriSurf( (srfTriangles_t *)surface );
	}

	if ( *surface != SF_FACE ) {
		return qfalse;
	}

	if ( shader->cullType == CT_TWO_SIDED ) {
		return qfalse;
	}

	// face culling
	if ( !r_facePlaneCull->integer ) {
		return qfalse;
	}

	sface = ( srfSurfaceFace_t * ) surface;

	if (r_cullRoofFaces->integer)
	{ //Very slow, but this is only intended for taking shots for automap images.
		if (sface->plane.normal[2] > 0.0f &&
			sface->numPoints > 0)
		{ //it's facing up I guess
			static int i;
			static trace_t tr;
			static vec3_t basePoint;
			static vec3_t endPoint;
			static vec3_t nNormal;
			static vec3_t v;

			//The fact that this point is in the middle of the array has no relation to the
			//orientation in the surface outline.
			basePoint[0] = sface->points[sface->numPoints/2][0];
			basePoint[1] = sface->points[sface->numPoints/2][1];
			basePoint[2] = sface->points[sface->numPoints/2][2];
			basePoint[2] += 2.0f;

			//the endpoint will be 8192 units from the chosen point
			//in the direction of the surface normal

			//just go straight up I guess, for now (slight hack)
			VectorSet(nNormal, 0.0f, 0.0f, 1.0f);
			VectorMA(basePoint, 8192.0f, nNormal, endPoint);

			ri.CM_BoxTrace(&tr, basePoint, endPoint, NULL, NULL, 0, (CONTENTS_SOLID|CONTENTS_TERRAIN), qfalse);

			if (!tr.startsolid &&
				!tr.allsolid &&
				(tr.fraction == 1.0f || (tr.surfaceFlags & SURF_NOIMPACT)))
			{ //either hit nothing or sky, so this surface is near the top of the level I guess. Or the floor of a really tall room, but if that's the case we're just screwed.
				VectorSubtract(basePoint, tr.endpos, v);
				if (tr.fraction == 1.0f || VectorLength(v) < r_roofCullCeilDist->value)
				{ //ignore it if it's not close to the top, unless it just hit nothing
					//Let's try to dig back into the brush based on the negative direction of the plane,
					//and if we pop out on the other side we'll see if it's ground or not.
					i = 4;
					VectorCopy(sface->plane.normal, nNormal);
					VectorInverse(nNormal);

					while (i < 4096)
					{
						VectorMA(basePoint, i, nNormal, endPoint);
						ri.CM_BoxTrace(&tr, endPoint, endPoint, NULL, NULL, 0, (CONTENTS_SOLID|CONTENTS_TERRAIN), qfalse);
						if (!tr.startsolid &&
							!tr.allsolid &&
							tr.fraction == 1.0f)
						{ //in the clear
							break;
						}
						i++;
					}
					if (i < 4096)
					{ //Make sure we got into clearance
						VectorCopy(endPoint, basePoint);
						basePoint[2] -= 2.0f;

						//just go straight down I guess, for now (slight hack)
						VectorSet(nNormal, 0.0f, 0.0f, -1.0f);
						VectorMA(basePoint, 4096.0f, nNormal, endPoint);

						//trace a second time from the clear point in the inverse normal direction of the surface.
						//If we hit something within a set amount of units, we will assume it's a bridge type object
						//and leave it to be drawn. Otherwise we will assume it is a roof or other obstruction and
						//cull it out.
						ri.CM_BoxTrace(&tr, basePoint, endPoint, NULL, NULL, 0, (CONTENTS_SOLID|CONTENTS_TERRAIN), qfalse);

						if (!tr.startsolid &&
							!tr.allsolid &&
							(tr.fraction != 1.0f && !(tr.surfaceFlags & SURF_NOIMPACT)))
						{ //if we hit nothing or a noimpact going down then this is probably "ground".
							VectorSubtract(basePoint, tr.endpos, endPoint);
							if (VectorLength(endPoint) > r_roofCullCeilDist->value)
							{ //128 (by default) is our maximum tolerance, above that will be removed
								return qtrue;
							}
						}
					}
				}
			}
		}
	}

	d = DotProduct (tr.ori.viewOrigin, sface->plane.normal);

	// don't cull exactly on the plane, because there are levels of rounding
	// through the BSP, ICD, and hardware that may cause pixel gaps if an
	// epsilon isn't allowed here
	if ( shader->cullType == CT_FRONT_SIDED ) {
		if ( d < sface->plane.dist - 8 ) {
			return qtrue;
		}
	} else {
		if ( d > sface->plane.dist + 8 ) {
			return qtrue;
		}
	}

	return qfalse;
}

static int R_DlightFace( srfSurfaceFace_t *face, int dlightBits ) {
	float		d;
	int			i;
	dlight_t	*dl;

	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		d = DotProduct( dl->origin, face->plane.normal ) - face->plane.dist;
		if ( !VectorCompare(face->plane.normal, vec3_origin) && (d < -dl->radius || d > dl->radius) ) {
			// dlight doesn't reach the plane
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	face->dlightBits = dlightBits;
	return dlightBits;
}

static int R_DlightGrid( srfGridMesh_t *grid, int dlightBits ) {
	int			i;
	dlight_t	*dl;

	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		if ( dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
}


static int R_DlightTrisurf( srfTriangles_t *surf, int dlightBits ) {
	// FIXME: more dlight culling to trisurfs...
	surf->dlightBits = dlightBits;
	return dlightBits;
#if 0
	int			i;
	dlight_t	*dl;

	for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
		if ( ! ( dlightBits & ( 1 << i ) ) ) {
			continue;
		}
		dl = &tr.refdef.dlights[i];
		if ( dl->origin[0] - dl->radius > grid->meshBounds[1][0]
			|| dl->origin[0] + dl->radius < grid->meshBounds[0][0]
			|| dl->origin[1] - dl->radius > grid->meshBounds[1][1]
			|| dl->origin[1] + dl->radius < grid->meshBounds[0][1]
			|| dl->origin[2] - dl->radius > grid->meshBounds[1][2]
			|| dl->origin[2] + dl->radius < grid->meshBounds[0][2] ) {
			// dlight doesn't reach the bounds
			dlightBits &= ~( 1 << i );
		}
	}

	if ( !dlightBits ) {
		tr.pc.c_dlightSurfacesCulled++;
	}

	grid->dlightBits = dlightBits;
	return dlightBits;
#endif
}

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
static int R_DlightSurface( msurface_t *surf, int dlightBits ) {
	if ( *surf->data == SF_FACE ) {
		dlightBits = R_DlightFace( (srfSurfaceFace_t *)surf->data, dlightBits );
	} else if ( *surf->data == SF_GRID ) {
		dlightBits = R_DlightGrid( (srfGridMesh_t *)surf->data, dlightBits );
	} else if ( *surf->data == SF_TRIANGLES ) {
		dlightBits = R_DlightTrisurf( (srfTriangles_t *)surf->data, dlightBits );
	} else {
		dlightBits = 0;
	}

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}



#ifdef _ALT_AUTOMAP_METHOD
static bool tr_drawingAutoMap = false;
#endif
static float g_playerHeight = 0.0f;

/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface( msurface_t *surf, int dlightBits, qboolean noViewCount = qfalse )
{
	if (!noViewCount)
	{
		if ( surf->viewCount == tr.viewCount )
		{
			// already in this view, but lets make sure all the dlight bits are set
			if ( *surf->data == SF_FACE )
			{
				((srfSurfaceFace_t *)surf->data)->dlightBits |= dlightBits;
			}
			else if ( *surf->data == SF_GRID )
			{
				((srfGridMesh_t *)surf->data)->dlightBits |= dlightBits;
			}
			else if ( *surf->data == SF_TRIANGLES )
			{
				((srfTriangles_t *)surf->data)->dlightBits |= dlightBits;
			}
			return;
		}
		surf->viewCount = tr.viewCount;
		// FIXME: bmodel fog?
	}

	/*
	if (r_shadows->integer == 2)
	{
		dlightBits = R_DlightSurface( surf, dlightBits );
		//dlightBits = ( dlightBits != 0 );
		R_AddDrawSurf( surf->data, tr.shadowShader, surf->fogIndex, dlightBits );
	}
	*/
	//world shadows?

	// try to cull before dlighting or adding
#ifdef _ALT_AUTOMAP_METHOD
	if (!tr_drawingAutoMap && R_CullSurface( surf->data, surf->shader ) )
#else
	if (R_CullSurface(surf->data, surf->shader))
#endif
	{
		return;
	}

	// check for dlighting
	if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
	}

#ifdef _ALT_AUTOMAP_METHOD
	if (tr_drawingAutoMap)
	{
	//	if (g_playerHeight != g_lastHeight ||
	//		!g_lastHeightValid)
		if (*surf->data == SF_FACE)
		{ //only do this if we need to
			bool completelyTransparent = true;
			int i = 0;
			srfSurfaceFace_t *face = (srfSurfaceFace_t *)surf->data;
			byte *indices = (byte *)(face + face->ofsIndices);
			float *point;
			vec3_t color;
			float alpha;
			float e;
			bool polyStarted = false;

			while (i < face->numIndices)
			{
				point = &face->points[indices[i]][0];

				//base the color on the elevation... for now, just check the first point height
				if (point[2] < g_playerHeight)
				{
					e = point[2]-g_playerHeight;
				}
				else
				{
					e = g_playerHeight-point[2];
				}
				if (e < 0.0f)
				{
					e = -e;
				}

				//set alpha and color based on relative height of point
				alpha = e/256.0f;
				e /= 512.0f;

				//cap color
				if (e > 1.0f)
				{
					e = 1.0f;
				}
				else if (e < 0.0f)
				{
					e = 0.0f;
				}
				VectorSet(color, e, 1.0f-e, 0.0f);

				//cap alpha
				if (alpha > 1.0f)
				{
					alpha = 1.0f;
				}
				else if (alpha < 0.0f)
				{
					alpha = 0.0f;
				}

				if (alpha != 1.0f)
				{ //this point is not entirely alpha'd out, so still draw the surface
					completelyTransparent = false;
				}

				if (!completelyTransparent)
				{
					if (!polyStarted)
					{
						qglBegin(GL_POLYGON);
						polyStarted = true;
					}

					qglColor4f(color[0], color[1], color[2], 1.0f-alpha);
					qglVertex3f(point[i], point[i], point[2]);
				}

				i++;
			}

			if (polyStarted)
			{
				qglEnd();
			}
		}
	}
	else
#endif
	{
		R_AddDrawSurf( surf->data, surf->shader, surf->fogIndex, dlightBits );
	}
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces ( trRefEntity_t *ent ) {
	bmodel_t	*bmodel;
	int			clip;
	model_t		*pModel;
	int			i;

	pModel = R_GetModelByHandle( ent->e.hModel );

	bmodel = pModel->bmodel;

	clip = R_CullLocalBox( bmodel->bounds );
	if ( clip == CULL_OUT ) {
		return;
	}

	if(pModel->bspInstance)
	{ //rwwRMG - added
		R_SetupEntityLighting(&tr.refdef, ent);
	}

	//rww - Take this into account later?
//	if ( !ri.Cvar_VariableIntegerValue( "com_RMG" ) )
//	{	// don't dlight bmodels on rmg, as multiple copies of the same instance will light up
		R_DlightBmodel( bmodel, false );
//	}
//	else
//	{
//		R_DlightBmodel( bmodel, true );
//	}

	for ( i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		R_AddWorldSurface( bmodel->firstSurface + i, tr.currentEntity->dlightBits, qtrue );
	}
}

float GetQuadArea( vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4 )
{
	vec3_t	vec1, vec2, dis1, dis2;

	// Get area of tri1
	VectorSubtract( v1, v2, vec1 );
	VectorSubtract( v1, v4, vec2 );
	CrossProduct( vec1, vec2, dis1 );
	VectorScale( dis1, 0.25f, dis1 );

	// Get area of tri2
	VectorSubtract( v3, v2, vec1 );
	VectorSubtract( v3, v4, vec2 );
	CrossProduct( vec1, vec2, dis2 );
	VectorScale( dis2, 0.25f, dis2 );

	// Return addition of disSqr of each tri area
	return ( dis1[0] * dis1[0] + dis1[1] * dis1[1] + dis1[2] * dis1[2] +
				dis2[0] * dis2[0] + dis2[1] * dis2[1] + dis2[2] * dis2[2] );
}

void RE_GetBModelVerts( int bmodelIndex, vec3_t *verts, vec3_t normal )
{
	msurface_t			*surfs;
	srfSurfaceFace_t	*face;
	bmodel_t			*bmodel;
	model_t				*pModel;
	int					i;
	//	Not sure if we really need to track the best two candidates
	int					maxDist[2]={0,0};
	int					maxIndx[2]={0,0};
	int					dist = 0;
	float				dot1, dot2;

	pModel = R_GetModelByHandle( bmodelIndex );
	bmodel = pModel->bmodel;

	// Loop through all surfaces on the brush and find the best two candidates
	for ( i = 0 ; i < bmodel->numSurfaces; i++ )
	{
		surfs = bmodel->firstSurface + i;
		face = ( srfSurfaceFace_t *)surfs->data;

		// It seems that the safest way to handle this is by finding the area of the faces
		dist = GetQuadArea( face->points[0], face->points[1], face->points[2], face->points[3] );

		// Check against the highest max
		if ( dist > maxDist[0] )
		{
			// Shuffle our current maxes down
			maxDist[1] = maxDist[0];
			maxIndx[1] = maxIndx[0];

			maxDist[0] = dist;
			maxIndx[0] = i;
		}
		// Check against the second highest max
		else if ( dist >= maxDist[1] )
		{
			// just stomp the old
			maxDist[1] = dist;
			maxIndx[1] = i;
		}
	}

	// Hopefully we've found two best case candidates.  Now we should see which of these faces the viewer
	surfs = bmodel->firstSurface + maxIndx[0];
	face = ( srfSurfaceFace_t *)surfs->data;
	dot1 = DotProduct( face->plane.normal, tr.refdef.viewaxis[0] );

	surfs = bmodel->firstSurface + maxIndx[1];
	face = ( srfSurfaceFace_t *)surfs->data;
	dot2 = DotProduct( face->plane.normal, tr.refdef.viewaxis[0] );

	if ( dot2 < dot1 && dot2 < 0.0f )
	{
		i = maxIndx[1]; // use the second face
	}
	else if ( dot1 < dot2 && dot1 < 0.0f )
	{
		i = maxIndx[0]; // use the first face
	}
	else
	{ // Possibly only have one face, so may as well use the first face, which also should be the best one
		//i = rand() & 1; // ugh, we don't know which to use.  I'd hope this would never happen
		i = maxIndx[0]; // use the first face
	}

	surfs = bmodel->firstSurface + i;
	face = ( srfSurfaceFace_t *)surfs->data;

	for ( int t = 0; t < 4; t++ )
	{
		VectorCopy(	face->points[t], verts[t] );
	}
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
=============================================================
WIREFRAME AUTOMAP GENERATION SYSTEM - BEGIN
=============================================================
*/
#ifndef _ALT_AUTOMAP_METHOD
typedef struct wireframeSurfPoint_s
{
	vec3_t					xyz;
	float					alpha;
	vec3_t					color;
} wireframeSurfPoint_t;

typedef struct wireframeMapSurf_s
{
	bool					completelyTransparent;

	int						numPoints;
	wireframeSurfPoint_t	*points;

	wireframeMapSurf_s		*next;
} wireframeMapSurf_t;

typedef struct wireframeMap_s
{
    wireframeMapSurf_t		*surfs;
} wireframeMap_t;

static wireframeMap_t g_autoMapFrame;
static wireframeMapSurf_t **g_autoMapNextFree = NULL;
static bool g_autoMapValid = false; //set to true of g_autoMapFrame is valid.

//get the next available wireframe automap surface. -rww
static inline wireframeMapSurf_t *R_GetNewWireframeMapSurf(void)
{
	wireframeMapSurf_t **next = &g_autoMapFrame.surfs;

	if (g_autoMapNextFree)
	{ //save us the time of going through the entire linked list from root
		next = g_autoMapNextFree;
	}

	while (*next)
	{ //iterate through until we find the next unused one
		next = &(*next)->next;
	}

	//allocate memory for it and pass it back
	(*next) = (wireframeMapSurf_t *)Z_Malloc(sizeof(wireframeMapSurf_t), TAG_ALL, qtrue);
	g_autoMapNextFree = &(*next)->next;
	return (*next);
}

//evaluate a surface, see if it is valid for being part of the
//wireframe map render. -rww
static inline void R_EvaluateWireframeSurf(msurface_t *surf)
{
	if (*surf->data == SF_FACE)
	{
		srfSurfaceFace_t *face = (srfSurfaceFace_t *)surf->data;
		float *points = &face->points[0][0];
		int numPoints = face->numIndices;
		int *indices = (int *)((byte *)face + face->ofsIndices);
		//byte *indices = (byte *)(face + face->ofsIndices);

		if (points && numPoints > 0)
		{ //we can add it
			int i = 0;
			wireframeMapSurf_t *nextSurf = R_GetNewWireframeMapSurf();

#if 0 //doing in realtime now
			float e;

			//base the color on the elevation... for now, just check the first point height
			if (points[2] < 0.0f)
			{
				e = -points[2];
			}
			else
			{
				e = points[2];
			}
			e /= 2048.0f;
			if (e > 1.0f)
			{
				e = 1.0f;
			}
			else if (e < 0.0f)
			{
				e = 0.0f;
			}
			VectorSet(color, e, 1.0f-e, 0.0f);
#endif

			//now go through the indices and add a point for each
			nextSurf->points = (wireframeSurfPoint_t *)Z_Malloc(sizeof(wireframeSurfPoint_t)*face->numIndices, TAG_ALL, qtrue);
			nextSurf->numPoints = face->numIndices;
			while (i < face->numIndices)
			{
				points = &face->points[indices[i]][0];
				VectorCopy(points, nextSurf->points[i].xyz);

				i++;
			}
		}
	}
	else if (*surf->data == SF_TRIANGLES)
	{
		//srfTriangles_t *surfTri = (srfTriangles_t *)surf->data;
		return; //not handled
	}
	else if (*surf->data == SF_GRID)
	{
		//srfGridMesh_t *gridMesh = (srfGridMesh_t *)surf->data;
		return; //not handled
	}
	else
	{ //...unknown type?
		return;
	}
}

#if 0
//see if any surfaces on the node are facing opposite directions
//using plane normals. -rww
static inline bool R_NodeHasOppositeFaces(mnode_t *node)
{
	int c, d;
	msurface_t *surf, *surf2, **mark, **mark2;
	srfSurfaceFace_t *face, *face2;
	vec3_t normalDif;

	mark = node->firstmarksurface;
	c = node->nummarksurfaces;

	while (c--)
	{
		surf = *mark;

		if (*surf->data != SF_FACE)
		{ //if this node is not entirely comprised of faces, I guess we shouldn't check it?
			return false;
		}

		face = (srfSurfaceFace_t *)surf->data;

		//go through other surfs and compare against this surf
		d = node->nummarksurfaces;
		mark2 = node->firstmarksurface;
		while (d--)
		{
			surf2 = *mark2;

			if (*surf2->data != SF_FACE)
			{
				return false;
			}
			face2 = (srfSurfaceFace_t *)surf2->data;
			//see if this normal has a drastic angular change
			VectorSubtract(face->plane.normal, face2->plane.normal, normalDif);
			if (VectorLength(normalDif) >= 1.8f)
			{
				return true;
			}

			mark2++;
		}
		mark++;
	}

	return false;
}
#endif

//recursively called for each node to go through the surfaces on that
//node and generate the wireframe map. -rww
static inline void R_RecursiveWireframeSurf(mnode_t *node)
{
	int c;
	msurface_t *surf, **mark;

	if (!node)
	{
		return;
	}

	while (1)
	{
		if (!node ||
			node->visframe != tr.visCount)
		{ //not valid, stop this chain of recursion
			return;
		}

		if ( node->contents != -1 )
		{
			break;
		}

		R_RecursiveWireframeSurf(node->children[0]);

		node = node->children[1];
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while (c--)
	{
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_EvaluateWireframeSurf(surf);
		mark++;
	}
}

//generates a wireframe model of the map for the automap view -rww
static void R_GenerateWireframeMap(mnode_t *baseNode)
{
	int i;

	//initialize data to all 0
	memset(&g_autoMapFrame, 0, sizeof(g_autoMapFrame));

	//take the hit for this frame, mark all of these things as visible
	//so we know which are valid for automap generation, but only the
	//ones that are facing outside the world! (well, ideally.)
	for (i = 0; i < tr.world->numnodes; i++)
	{
		if (tr.world->nodes[i].contents != CONTENTS_SOLID)
		{
#if 0 //doesn't work, I take it surfs on nodes are not related to surfs on brushes
			if (!R_NodeHasOppositeFaces(&tr.world->nodes[i]))
#endif
			{
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
	}

	//now start the recursive evaluation
	R_RecursiveWireframeSurf(baseNode);
}

//clear out the wireframe map data -rww
void R_DestroyWireframeMap(void)
{
	wireframeMapSurf_t *next;
	wireframeMapSurf_t *last;

	if (!g_autoMapValid)
	{ //not valid to begin with
		return;
	}

	next = g_autoMapFrame.surfs;
	while (next)
	{
		//free memory allocated for points on this surface
		Z_Free(next->points);

		//get the next surface
		last = next;
		next = next->next;

		//free memory for this surface
		Z_Free(last);
	}

	//invalidate everything
	memset(&g_autoMapFrame, 0, sizeof(g_autoMapFrame));
	g_autoMapValid = false;
	g_autoMapNextFree = NULL;
}

//save 3d automap data to file -rww
qboolean R_WriteWireframeMapToFile(void)
{
	fileHandle_t f;
	int requiredSize = 0;
	wireframeMapSurf_t *surf = g_autoMapFrame.surfs;
	byte *out, *rOut;

	//let's go through and see how much space we're going to need to stuff all this
	//data into
    while (surf)
	{
		//memory for each point
		requiredSize += sizeof(wireframeSurfPoint_t)*surf->numPoints;

		//memory for numPoints
		requiredSize += sizeof(int);

		surf = surf->next;
	}

	if (requiredSize <= 0)
	{ //nothing to do..?
		return qfalse;
	}


	f = ri.FS_FOpenFileWrite("blahblah.bla", qtrue);
	if (!f)
	{ //can't create?
		return qfalse;
	}

	//allocate the memory we will need
    out = (byte *)Z_Malloc(requiredSize, TAG_ALL, qtrue);
	rOut = out;

	//now go through and put the data into the memory
	surf = g_autoMapFrame.surfs;
    while (surf)
	{
		memcpy(out, surf, (sizeof(wireframeSurfPoint_t)*surf->numPoints) + sizeof(int));

		//memory for each point
		out += sizeof(wireframeSurfPoint_t)*surf->numPoints;

		//memory for numPoints
		out += sizeof(int);

		surf = surf->next;
	}

	//now write the buffer, and close
	ri.FS_Write(rOut, requiredSize, f);
	Z_Free(rOut);
	ri.FS_FCloseFile(f);

	return qtrue;
}

//load 3d automap data from file -rww
qboolean R_GetWireframeMapFromFile(void)
{
	wireframeMapSurf_t *surfs, *rSurfs;
	wireframeMapSurf_t *newSurf;
	fileHandle_t f;
	int i = 0;
	int len;
	int stepBytes;

	len = ri.FS_FOpenFileRead("blahblah.bla", &f, qfalse);
	if (!f || len <= 0)
	{ //it doesn't exist
		return qfalse;
	}

	surfs = (wireframeMapSurf_t *)Z_Malloc(len, TAG_ALL, qtrue);
	rSurfs = surfs;
	ri.FS_Read(surfs, len, f);

	while (i < len)
	{
		newSurf = R_GetNewWireframeMapSurf();
		newSurf->points = (wireframeSurfPoint_t *)Z_Malloc(sizeof(wireframeSurfPoint_t)*surfs->numPoints, TAG_ALL, qtrue);

		//copy the surf data into the new surf
		//note - the surfs->points pointer is NOT pointing to valid memory, a pointer to that
		//pointer is actually what we want to use as the location of the point offsets.
		memcpy(newSurf->points, &surfs->points, sizeof(wireframeSurfPoint_t)*surfs->numPoints);
		newSurf->numPoints = surfs->numPoints;

		//the size of the point data, plus an int (the number of points)
		stepBytes = (sizeof(wireframeSurfPoint_t)*surfs->numPoints) + sizeof(int);
		i += stepBytes;

		//increment the pointer to the start of the next surface
		surfs = (wireframeMapSurf_t *)((byte *)surfs+stepBytes);
	}

	//it should end up being equal, if not something was wrong with this file.
	assert(i == len);

	ri.FS_FCloseFile(f);
	Z_Free(rSurfs);
	return qtrue;
}

//create everything, after destroying any existing data -rww
qboolean R_InitializeWireframeAutomap(void)
{
	if (r_autoMapDisable && r_autoMapDisable->integer)
	{
		return qfalse;
	}

	if (tr.world &&
		tr.world->nodes)
	{
		R_DestroyWireframeMap();
#if 0 //file load-save
		if (!R_GetWireframeMapFromFile())
		{ //first try loading the data from a file. If there is none, generate it.
			R_GenerateWireframeMap(tr.world->nodes);

			//now write it to file, since we have generated it successfully.
			R_WriteWireframeMapToFile();
		}
#else //always generate
		R_GenerateWireframeMap(tr.world->nodes);
#endif
		g_autoMapValid = true;
	}

	return (qboolean)g_autoMapValid;
}
#endif //0
/*
=============================================================
WIREFRAME AUTOMAP GENERATION SYSTEM - END
=============================================================
*/

void R_AutomapElevationAdjustment(float newHeight)
{
	g_playerHeight = newHeight;
}

#ifdef _ALT_AUTOMAP_METHOD
//adjust the player height for gradient elevation colors -rww
qboolean R_InitializeWireframeAutomap(void)
{ //yoink
	return qtrue;
}
#endif

//draw the automap with the given transformation matrix -rww
#define QUADINFINITY			16777216
static float g_lastHeight = 0.0f;
static bool g_lastHeightValid = false;
static void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits );
const void *R_DrawWireframeAutomap(const void *data)
{
	const drawBufferCommand_t *cmd = (const drawBufferCommand_t *)data;
	float e = 0.0f;
	float alpha;
	wireframeMapSurf_t *s = g_autoMapFrame.surfs;
#ifndef _ALT_AUTOMAP_METHOD
	int i;
#endif

	if (!r_autoMap || !r_autoMap->integer)
	{
		return (const void *)(cmd + 1);
	}

#ifndef _ALT_AUTOMAP_METHOD
	if (!g_autoMapValid)
	{ //data is not valid, don't draw
		return (const void *)(cmd + 1);
	}
#endif

#if 0 //instead of this method, just do the automap as a new "scene"
	//projection matrix mode
	qglMatrixMode(GL_PROJECTION);

	//store the current matrix
	qglPushMatrix();
	//translate to our proper pos/angles from identity
	qglLoadIdentity();
	qglTranslatef(pos[0], pos[1], pos[2]);
	//presumeably this is correct for compensating for quake's
	//crazy angle system.
	qglRotatef(angles[1], 0.0f, 0.0f, 1.0f);
	qglRotatef(-angles[0], 0.0f, 1.0f, 0.0f);
	qglRotatef(angles[2], 1.0f, 0.0f, 0.0f);
#endif

	//disable 2d texturing
	qglDisable( GL_TEXTURE_2D );

	//now draw the backdrop
#if 0 //this does no good sadly, because of the issue of having to clear with a second scene
	//in order for global fog clearing to work.
	if (r_autoMapBackAlpha && r_autoMapBackAlpha->value)
	{ //specify the automap background alpha
		alpha = r_autoMapBackAlpha->value;

		//cap it reasonably
		if (alpha < 0.0f)
		{
			alpha = 0.0f;
		}
		else if (alpha > 1.0f)
		{
			alpha = 1.0f;
		}
		GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA);
	}
	else
#endif
	{
		alpha = 1.0f;
		GL_State(0);
	}
	//black
	qglColor4f(0.0f, 0.0f, 0.0f, alpha);

	//draw a black backdrop
	qglPushMatrix();
	qglLoadIdentity(); //get the ident matrix

	qglBegin( GL_QUADS );
	qglVertex3f( -QUADINFINITY, QUADINFINITY, -(backEnd.viewParms.zFar-1) );
	qglVertex3f( QUADINFINITY, QUADINFINITY, -(backEnd.viewParms.zFar-1) );
	qglVertex3f( QUADINFINITY, -QUADINFINITY, -(backEnd.viewParms.zFar-1) );
	qglVertex3f( -QUADINFINITY, -QUADINFINITY, -(backEnd.viewParms.zFar-1) );
	qglEnd ();

	//pop back the viewmatrix
	qglPopMatrix();


	//set the mode to line draw
	if (r_autoMap->integer == 2)
	{ //line mode
		GL_State(GLS_POLYMODE_LINE|GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_COLOR|GLS_DEPTHMASK_TRUE);
	}
	else
	{ //fill mode
		//GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_COLOR|GLS_DEPTHMASK_TRUE);
		GL_State(GLS_DEPTHMASK_TRUE);
	}

	//set culling
	GL_Cull(CT_TWO_SIDED);

#ifndef _ALT_AUTOMAP_METHOD
	//Draw the triangles
	while (s)
	{
		//first, loop through and set the alpha on every point for this surf.
		//if the alpha ends up being completely transparent for every point, we don't even
		//need to draw it
		if (g_playerHeight != g_lastHeight ||
			!g_lastHeightValid)
		{ //only do this if we need to
			i = 0;
			s->completelyTransparent = true;
			while (i < s->numPoints)
			{
				//base the color on the elevation... for now, just check the first point height
				if (s->points[i].xyz[2] < g_playerHeight)
				{
					e = s->points[i].xyz[2]-g_playerHeight;
				}
				else
				{
					e = g_playerHeight-s->points[i].xyz[2];
				}
				if (e < 0.0f)
				{
					e = -e;
				}

				if (r_autoMap->integer != 2)
				{ //fill mode
					if (s->points[i].xyz[2] > (g_playerHeight+64.0f))
					{
						s->points[i].alpha = 1.0f;
					}
					else
					{
						s->points[i].alpha = e/256.0f;
					}
				}
				else
				{
					//set alpha and color based on relative height of point
					s->points[i].alpha = e/256.0f;
				}
				e /= 512.0f;

				//cap color
				if (e > 1.0f)
				{
					e = 1.0f;
				}
				else if (e < 0.0f)
				{
					e = 0.0f;
				}
				VectorSet(s->points[i].color, e, 1.0f-e, 0.0f);

				//cap alpha
				if (s->points[i].alpha > 1.0f)
				{
					s->points[i].alpha = 1.0f;
				}
				else if (s->points[i].alpha < 0.0f)
				{
					s->points[i].alpha = 0.0f;
				}

				if (s->points[i].alpha != 1.0f)
				{ //this point is not entirely alpha'd out, so still draw the surface
					s->completelyTransparent = false;
				}

				i++;
			}
		}

		if (s->completelyTransparent)
		{
			s = s->next;
			continue;
		}

		i = 0;
		qglBegin(GL_TRIANGLES);
		while (i < s->numPoints)
		{
			if (r_autoMap->integer == 2 || s->numPoints < 3)
			{ //line mode or not enough verts on surface
				qglColor4f(s->points[i].color[0], s->points[i].color[1], s->points[i].color[2], s->points[i].alpha);
			}
			else
			{ //fill mode
				vec3_t planeNormal;
				float fAlpha = s->points[i].alpha;
				planeNormal[0] = s->points[0].xyz[1]*(s->points[1].xyz[2]-s->points[2].xyz[2]) + s->points[1].xyz[1]*(s->points[2].xyz[2]-s->points[0].xyz[2]) + s->points[2].xyz[1]*(s->points[0].xyz[2]-s->points[1].xyz[2]);
				planeNormal[1] = s->points[0].xyz[2]*(s->points[1].xyz[0]-s->points[2].xyz[0]) + s->points[1].xyz[2]*(s->points[2].xyz[0]-s->points[0].xyz[0]) + s->points[2].xyz[2]*(s->points[0].xyz[0]-s->points[1].xyz[0]);
				planeNormal[2] = s->points[0].xyz[0]*(s->points[1].xyz[1]-s->points[2].xyz[1]) + s->points[1].xyz[0]*(s->points[2].xyz[1]-s->points[0].xyz[1]) + s->points[2].xyz[0]*(s->points[0].xyz[1]-s->points[1].xyz[1]);

				if (planeNormal[0] < 0.0f) planeNormal[0] = -planeNormal[0];
				if (planeNormal[1] < 0.0f) planeNormal[1] = -planeNormal[1];
				if (planeNormal[2] < 0.0f) planeNormal[2] = -planeNormal[2];

				/*
				if (s->points[i].xyz[2] > g_playerHeight+64.0f &&
					planeNormal[2] > 0.7f)
				{ //surface above player facing up/down directly
					fAlpha = 1.0f-planeNormal[2];
				}
				*/

				//qglColor4f(planeNormal[0], planeNormal[1], planeNormal[2], fAlpha);
				qglColor4f(s->points[i].color[0], s->points[i].color[1], 1.0f-planeNormal[2], fAlpha);
			}
			qglVertex3f(s->points[i].xyz[0], s->points[i].xyz[1], s->points[i].xyz[2]);
			i++;
		}
		qglEnd();
		s = s->next;
	}
#else
	tr_drawingAutoMap = true;
	R_RecursiveWorldNode( tr.world->nodes, 15, 0 );
	tr_drawingAutoMap = false;
#endif

	g_lastHeight = g_playerHeight;
	g_lastHeightValid = true;

#if 0 //instead of this method, just do the automap as a new "scene"
	//pop back the view matrix
	qglPopMatrix();
#endif

	//reenable 2d texturing
	qglEnable( GL_TEXTURE_2D );

	//white color/full alpha
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	return (const void *)(cmd + 1);
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits ) {

	do
	{
		int			newDlights[2];

#ifdef _ALT_AUTOMAP_METHOD
		if (tr_drawingAutoMap)
		{
			node->visframe = tr.visCount;
		}
#endif

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount)
		{
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

#ifdef _ALT_AUTOMAP_METHOD
		if ( r_nocull->integer!=1 && !tr_drawingAutoMap )
#else
		if (r_nocull->integer!=1)
#endif
		{
			int		r;

			if ( planeBits & 1 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if ( planeBits & 2 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if ( planeBits & 4 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if ( planeBits & 8 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}

		}

		if ( node->contents != -1 ) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
		if ( r_nocull->integer!=2 )
		{
			newDlights[0] = 0;
			newDlights[1] = 0;
			if ( dlightBits )
			{
				int	i;
				for ( i = 0 ; i < tr.refdef.num_dlights ; i++ )
				{
					dlight_t	*dl;
					float		dist;

					if ( dlightBits & ( 1 << i ) ) {
						dl = &tr.refdef.dlights[i];
						dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;

						if ( dist > -dl->radius ) {
							newDlights[0] |= ( 1 << i );
						}
						if ( dist < dl->radius ) {
							newDlights[1] |= ( 1 << i );
						}
					}
				}
			}
		}
		else
		{
			newDlights[0] = dlightBits;
			newDlights[1] = dlightBits;
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode (node->children[0], planeBits, newDlights[0] );

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
	} while ( 1 );

	{
		// leaf node, so add mark surfaces
		int			c;
		msurface_t	*surf, **mark;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		if ( node->mins[0] < tr.viewParms.visBounds[0][0] ) {
			tr.viewParms.visBounds[0][0] = node->mins[0];
		}
		if ( node->mins[1] < tr.viewParms.visBounds[0][1] ) {
			tr.viewParms.visBounds[0][1] = node->mins[1];
		}
		if ( node->mins[2] < tr.viewParms.visBounds[0][2] ) {
			tr.viewParms.visBounds[0][2] = node->mins[2];
		}

		if ( node->maxs[0] > tr.viewParms.visBounds[1][0] ) {
			tr.viewParms.visBounds[1][0] = node->maxs[0];
		}
		if ( node->maxs[1] > tr.viewParms.visBounds[1][1] ) {
			tr.viewParms.visBounds[1][1] = node->maxs[1];
		}
		if ( node->maxs[2] > tr.viewParms.visBounds[1][2] ) {
			tr.viewParms.visBounds[1][2] = node->maxs[2];
		}

		// add the individual surfaces
		mark = node->firstmarksurface;
		c = node->nummarksurfaces;
		while (c--) {
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_AddWorldSurface( surf, dlightBits );
			mark++;
		}
	}

}

/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf( const vec3_t p ) {
	mnode_t		*node;
	float		d;
	cplane_t	*plane;

	if ( !tr.world ) {
		Com_Error (ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while( 1 ) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}

	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS (int cluster) {
	if (!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters ) {
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
qboolean R_inPVS( const vec3_t p1, const vec3_t p2, byte *mask ) {
	int		leafnum;
	int		cluster;

	leafnum = ri.CM_PointLeafnum (p1);
	cluster = ri.CM_LeafCluster (leafnum);

	//agh, the damn snapshot mask doesn't work for this
	mask = (byte *) ri.CM_ClusterPVS (cluster);

	leafnum = ri.CM_PointLeafnum (p2);
	cluster = ri.CM_LeafCluster (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;

	return qtrue;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves (void) {
	const byte	*vis;
	mnode_t	*leaf, *parent;
	int		i;
	int		cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf( tr.viewParms.pvsOrigin );
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything
	if ( tr.viewCluster == cluster && !tr.refdef.areamaskModified
		&& !r_showcluster->modified ) {
		return;
	}

	if ( r_showcluster->modified || r_showcluster->integer ) {
		r_showcluster->modified = qfalse;
		if ( r_showcluster->integer ) {
			ri.Printf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if ( r_novis->integer || tr.viewCluster == -1 ) {
		for (i=0 ; i<tr.world->numnodes ; i++) {
			if (tr.world->nodes[i].contents != CONTENTS_SOLID) {
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	vis = R_ClusterPVS (tr.viewCluster);

	for (i=0,leaf=tr.world->nodes ; i<tr.world->numnodes ; i++, leaf++) {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters ) {
			continue;
		}

		// check general pvs
		if ( !(vis[cluster>>3] & (1<<(cluster&7))) ) {
			continue;
		}

		// check for door connection
		if ( (tr.refdef.areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) ) {
			continue;		// not visible
		}

		parent = leaf;
		do {
			if (parent->visframe == tr.visCount)
				break;
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}

/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces (void) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves ();

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );

	// perform frustum culling and add all the potentially visible surfaces
	if ( tr.refdef.num_dlights > 32 ) {
		tr.refdef.num_dlights = 32 ;
	}

	R_RecursiveWorldNode( tr.world->nodes, 15, ( 1 << tr.refdef.num_dlights ) - 1 );
}
