/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"


world_t *R_GetWorld(int worldIndex)
{
	if (worldIndex == -1)
	{
		return tr.world;
	}
	else
	{
		return tr.bspModels[worldIndex];
	}
}

/*
================
R_CullSurface

Tries to cull surfaces before they are lighted or
added to the sorting list.
================
*/
static qboolean	R_CullSurface( msurface_t *surf, int entityNum ) {
	if ( r_nocull->integer || surf->cullinfo.type == CULLINFO_NONE) {
		return qfalse;
	}

	if ( *surf->data == SF_GRID && r_nocurves->integer ) {
		return qtrue;
	}

	if (surf->cullinfo.type & CULLINFO_PLANE)
	{
		// Only true for SF_FACE, so treat like its own function
		float			d;
		cullType_t ct;

		if ( !r_facePlaneCull->integer ) {
			return qfalse;
		}

		ct = surf->shader->cullType;

		if (ct == CT_TWO_SIDED)
		{
			return qfalse;
		}

		if (tr.viewParms.flags & (VPF_DEPTHSHADOW) && tr.viewParms.flags & (VPF_SHADOWCASCADES))
		{
			if (ct == CT_FRONT_SIDED)
			{
				ct = CT_BACK_SIDED;
			}
			else if (ct == CT_BACK_SIDED)
			{
				ct = CT_FRONT_SIDED;
			}
		}

		// do proper cull for orthographic projection
		if (tr.viewParms.flags & VPF_ORTHOGRAPHIC) {
			d = DotProduct(tr.viewParms.ori.axis[0], surf->cullinfo.plane.normal);
			if ( ct == CT_FRONT_SIDED ) {
				if (d > 0)
					return qtrue;
			} else {
				if (d < 0)
					return qtrue;
			}
			return qfalse;
		}

		d = DotProduct (tr.ori.viewOrigin, surf->cullinfo.plane.normal);

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( ct == CT_FRONT_SIDED ) {
			if ( d < surf->cullinfo.plane.dist - 8 ) {
				return qtrue;
			}
		} else {
			if ( d > surf->cullinfo.plane.dist + 8 ) {
				return qtrue;
			}
		}

		return qfalse;
	}

	if (surf->cullinfo.type & CULLINFO_SPHERE)
	{
		int 	sphereCull;

		if ( entityNum != REFENTITYNUM_WORLD ) {
			sphereCull = R_CullLocalPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
		} else {
			sphereCull = R_CullPointAndRadius( surf->cullinfo.localOrigin, surf->cullinfo.radius );
		}

		if ( sphereCull == CULL_OUT )
		{
			return qtrue;
		}
	}

	if (surf->cullinfo.type & CULLINFO_BOX)
	{
		int boxCull;

		if ( entityNum != REFENTITYNUM_WORLD ) {
			boxCull = R_CullLocalBox( surf->cullinfo.bounds );
		} else {
			boxCull = R_CullBox( surf->cullinfo.bounds );
		}

		if ( boxCull == CULL_OUT )
		{
			return qtrue;
		}
	}

	return qfalse;
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
	float       d;
	int         i;
	dlight_t    *dl;

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			d = DotProduct( dl->origin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -dl->radius || d > dl->radius ) {
				// dlight doesn't reach the plane
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if ( dl->origin[0] - dl->radius > surf->cullinfo.bounds[1][0]
				|| dl->origin[0] + dl->radius < surf->cullinfo.bounds[0][0]
				|| dl->origin[1] - dl->radius > surf->cullinfo.bounds[1][1]
				|| dl->origin[1] + dl->radius < surf->cullinfo.bounds[0][1]
				|| dl->origin[2] - dl->radius > surf->cullinfo.bounds[1][2]
				|| dl->origin[2] + dl->radius < surf->cullinfo.bounds[0][2] ) {
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
			if ( ! ( dlightBits & ( 1 << i ) ) ) {
				continue;
			}
			dl = &tr.refdef.dlights[i];
			if (!SpheresIntersect(dl->origin, dl->radius, surf->cullinfo.localOrigin, surf->cullinfo.radius))
			{
				// dlight doesn't reach the bounds
				dlightBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES:
		case SF_VBO_MESH:
			((srfBspSurface_t *)surf->data)->dlightBits = dlightBits;
			break;

		default:
			dlightBits = 0;
			break;
	}

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	} else {
		tr.pc.c_dlightSurfacesCulled++;
	}

	return dlightBits;
}

/*
====================
R_PshadowSurface

Just like R_DlightSurface, cull any we can
====================
*/
static int R_PshadowSurface( msurface_t *surf, int pshadowBits ) {
	float       d;
	int         i;
	pshadow_t    *ps;

	if ( surf->cullinfo.type & CULLINFO_PLANE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			d = DotProduct( ps->lightOrigin, surf->cullinfo.plane.normal ) - surf->cullinfo.plane.dist;
			if ( d < -ps->lightRadius || d > ps->lightRadius ) {
				// pshadow doesn't reach the plane
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_BOX )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if ( ps->lightOrigin[0] - ps->lightRadius > surf->cullinfo.bounds[1][0]
				|| ps->lightOrigin[0] + ps->lightRadius < surf->cullinfo.bounds[0][0]
				|| ps->lightOrigin[1] - ps->lightRadius > surf->cullinfo.bounds[1][1]
				|| ps->lightOrigin[1] + ps->lightRadius < surf->cullinfo.bounds[0][1]
				|| ps->lightOrigin[2] - ps->lightRadius > surf->cullinfo.bounds[1][2]
				|| ps->lightOrigin[2] + ps->lightRadius < surf->cullinfo.bounds[0][2]
				|| BoxOnPlaneSide(surf->cullinfo.bounds[0], surf->cullinfo.bounds[1], &ps->cullPlane) == 2 ) {
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	if ( surf->cullinfo.type & CULLINFO_SPHERE )
	{
		for ( i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
			if ( ! ( pshadowBits & ( 1 << i ) ) ) {
				continue;
			}
			ps = &tr.refdef.pshadows[i];
			if (!SpheresIntersect(ps->viewOrigin, ps->viewRadius, surf->cullinfo.localOrigin, surf->cullinfo.radius)
				|| DotProduct( surf->cullinfo.localOrigin, ps->cullPlane.normal ) - ps->cullPlane.dist < -surf->cullinfo.radius)
			{
				// pshadow doesn't reach the bounds
				pshadowBits &= ~( 1 << i );
			}
		}
	}

	switch(*surf->data)
	{
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES:
		case SF_VBO_MESH:
			((srfBspSurface_t *)surf->data)->pshadowBits = pshadowBits;
			break;

		default:
			pshadowBits = 0;
			break;
	}

	if ( pshadowBits ) {
		//tr.pc.c_dlightSurfaces++;
	}

	return pshadowBits;
}


/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface(
	msurface_t *surf,
	const trRefEntity_t *entity,
	int entityNum,
	int dlightBits,
	int pshadowBits)
{
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if ( R_CullSurface( surf, entityNum ) ) {
		return;
	}

	// check for dlighting
	// TODO: implement dlight culling for non worldspawn surfaces
	if ( dlightBits ) {
		if (entityNum != REFENTITYNUM_WORLD)
			dlightBits = (1 << tr.refdef.num_dlights) - 1;
		else
			dlightBits = R_DlightSurface( surf, dlightBits );
	}

	// set pshadows
	if ( pshadowBits ) {
		R_PshadowSurface( surf, pshadowBits );
	}

	bool isPostRenderEntity = false;
	if ( entityNum != REFENTITYNUM_WORLD )
	{
		assert(entity);
		isPostRenderEntity = R_IsPostRenderEntity(entity);
	}

	R_AddDrawSurf( surf->data, entityNum, surf->shader, surf->fogIndex,
			dlightBits, isPostRenderEntity, surf->cubemapIndex );

	for ( int i = 0, numSprites = surf->numSurfaceSprites;
			i < numSprites; ++i )
	{
		srfSprites_t *sprites = surf->surfaceSprites + i;
		R_AddDrawSurf((surfaceType_t *)sprites, entityNum, sprites->shader,
				surf->fogIndex, dlightBits, isPostRenderEntity, 0);
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
void R_AddBrushModelSurfaces ( trRefEntity_t *ent, int entityNum ) {
	model_t *pModel = R_GetModelByHandle( ent->e.hModel );
	bmodel_t *bmodel = pModel->data.bmodel;
	int clip = R_CullLocalBox( bmodel->bounds );
	if ( clip == CULL_OUT ) {
		return;
	}

	if (!(tr.viewParms.flags & VPF_DEPTHSHADOW))
		R_DlightBmodel( bmodel, ent );

	world_t *world = R_GetWorld(bmodel->worldIndex);
	for ( int i = 0 ; i < bmodel->numSurfaces ; i++ ) {
		int surf = bmodel->firstSurface + i;

		if (world->surfacesViewCount[surf] != tr.viewCount)
		{
			world->surfacesViewCount[surf] = tr.viewCount;
			R_AddWorldSurface(world->surfaces + surf, ent, entityNum, ent->needDlights, 0);
		}
	}
}

float GetQuadArea(vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4)
{
	vec3_t	vec1, vec2, dis1, dis2;

	// Get area of tri1
	VectorSubtract(v1, v2, vec1);
	VectorSubtract(v1, v4, vec2);
	CrossProduct(vec1, vec2, dis1);
	VectorScale(dis1, 0.25f, dis1);

	// Get area of tri2
	VectorSubtract(v3, v2, vec1);
	VectorSubtract(v3, v4, vec2);
	CrossProduct(vec1, vec2, dis2);
	VectorScale(dis2, 0.25f, dis2);

	// Return addition of disSqr of each tri area
	return (dis1[0] * dis1[0] + dis1[1] * dis1[1] + dis1[2] * dis1[2] +
		dis2[0] * dis2[0] + dis2[1] * dis2[1] + dis2[2] * dis2[2]);
}

void RE_GetBModelVerts(int bmodelIndex, vec3_t *verts, vec3_t normal)
{
	int					surf;
	srfBspSurface_t		*face;
	//	Not sure if we really need to track the best two candidates
	int					maxDist[2] = { 0,0 };
	int					maxIndx[2] = { 0,0 };
	int					dist = 0;
	float				dot1, dot2;

	model_t *pModel = R_GetModelByHandle(bmodelIndex);
	bmodel_t *bmodel = pModel->data.bmodel;
	world_t *world = R_GetWorld(bmodel->worldIndex);

	// Loop through all surfaces on the brush and find the best two candidates
	for (int i = 0; i < bmodel->numSurfaces; i++)
	{
		surf = bmodel->firstSurface + i;
		face = (srfBspSurface_t *)(world->surfaces + surf)->data;

		// It seems that the safest way to handle this is by finding the area of the faces
		dist = GetQuadArea(face->verts[0].xyz, face->verts[1].xyz, face->verts[2].xyz, face->verts[3].xyz);

		// Check against the highest max
		if (dist > maxDist[0])
		{
			// Shuffle our current maxes down
			maxDist[1] = maxDist[0];
			maxIndx[1] = maxIndx[0];

			maxDist[0] = dist;
			maxIndx[0] = i;
		}
		// Check against the second highest max
		else if (dist >= maxDist[1])
		{
			// just stomp the old
			maxDist[1] = dist;
			maxIndx[1] = i;
		}
	}

	// Hopefully we've found two best case candidates.  Now we should see which of these faces the viewer

	surf = bmodel->firstSurface + maxIndx[0];
	face = (srfBspSurface_t *)(world->surfaces + surf)->data;
	dot1 = DotProduct(face->cullPlane.normal, tr.refdef.viewaxis[0]);

	surf = bmodel->firstSurface + maxIndx[1];
	face = (srfBspSurface_t *)(world->surfaces + surf)->data;
	dot2 = DotProduct(face->cullPlane.normal, tr.refdef.viewaxis[0]);

	if (dot2 < dot1 && dot2 < 0.0f)
	{
		surf = bmodel->firstSurface + maxIndx[1]; // use the second face
	}
	else if (dot1 < dot2 && dot1 < 0.0f)
	{
		surf = bmodel->firstSurface + maxIndx[0]; // use the first face
	}
	else
	{ // Possibly only have one face, so may as well use the first face, which also should be the best one
		//i = rand() & 1; // ugh, we don't know which to use.  I'd hope this would never happen
		surf = bmodel->firstSurface + maxIndx[0]; // use the first face
	}
	face = (srfBspSurface_t *)(world->surfaces + surf)->data;

	for (int t = 0; t < 4; t++)
	{
		VectorCopy(face->verts[t].xyz, verts[t]);
	}
}

void RE_SetRangedFog ( float range )
{
	tr.rangedFog = range;
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/


/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits, int pshadowBits )
{
	do {
		int			newDlights[2];
		unsigned int newPShadows[2];

		// if the node wasn't marked as potentially visible, exit
		// pvs is skipped for depth shadows
		if (!(tr.viewParms.flags & VPF_DEPTHSHADOW) &&
				node->visCounts[tr.visIndex] != tr.visCounts[tr.visIndex]) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if ( !r_nocull->integer ) {
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

			if ( planeBits & 16 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[4]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~16;			// all descendants will also be in front
				}
			}
		}

		if ( node->contents != -1 ) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
		newDlights[0] = 0;
		newDlights[1] = 0;
		if ( dlightBits ) {
			for ( int i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
				if ( !(dlightBits & (1 << i)) ) {
					continue;
				}

				dlight_t *dl = &tr.refdef.dlights[i];
				float dist = DotProduct(dl->origin, node->plane->normal) - node->plane->dist;

				if ( dist > -dl->radius ) {
					newDlights[0] |= ( 1 << i );
				}
				if ( dist < dl->radius ) {
					newDlights[1] |= ( 1 << i );
				}
			}
		}

		newPShadows[0] = 0;
		newPShadows[1] = 0;
		if ( pshadowBits ) {
			for ( int i = 0 ; i < tr.refdef.num_pshadows ; i++ ) {
				if ( !(pshadowBits & (1 << i)) ) {
					continue;
				}

				pshadow_t *shadow = &tr.refdef.pshadows[i];
				float dist = DotProduct(shadow->lightOrigin, node->plane->normal) - node->plane->dist;

				if ( dist > -shadow->lightRadius ) {
					newPShadows[0] |= ( 1 << i );
				}

				if ( dist < shadow->lightRadius ) {
					newPShadows[1] |= ( 1 << i );
				}
			}
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode (node->children[0], planeBits, newDlights[0], newPShadows[0] );

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
		pshadowBits = newPShadows[1];
	} while ( 1 );

	{
		// leaf node, so add mark surfaces
		int			c;
		int surf, *view;

		tr.pc.c_leafs++;

		// add to z buffer bounds
		tr.viewParms.visBounds[0][0] = MIN(node->mins[0], tr.viewParms.visBounds[0][0]);
		tr.viewParms.visBounds[0][1] = MIN(node->mins[1], tr.viewParms.visBounds[0][1]);
		tr.viewParms.visBounds[0][2] = MIN(node->mins[2], tr.viewParms.visBounds[0][2]);

		tr.viewParms.visBounds[1][0] = MAX(node->maxs[0], tr.viewParms.visBounds[1][0]);
		tr.viewParms.visBounds[1][1] = MAX(node->maxs[1], tr.viewParms.visBounds[1][1]);
		tr.viewParms.visBounds[1][2] = MAX(node->maxs[2], tr.viewParms.visBounds[1][2]);

		// add merged and unmerged surfaces
		if (tr.world->viewSurfaces && !r_nocurves->integer)
			view = tr.world->viewSurfaces + node->firstmarksurface;
		else
			view = tr.world->marksurfaces + node->firstmarksurface;

		c = node->nummarksurfaces;
		while (c--) {
			// just mark it as visible, so we don't jump out of the cache derefencing the surface
			surf = *view;
			if (surf < 0)
			{
				if (tr.world->mergedSurfacesViewCount[-surf - 1] != tr.viewCount)
				{
					tr.world->mergedSurfacesViewCount[-surf - 1]  = tr.viewCount;
					tr.world->mergedSurfacesDlightBits[-surf - 1] = dlightBits;
					tr.world->mergedSurfacesPshadowBits[-surf - 1] = pshadowBits;
				}
				else
				{
					tr.world->mergedSurfacesDlightBits[-surf - 1] |= dlightBits;
					tr.world->mergedSurfacesPshadowBits[-surf - 1] |= pshadowBits;
				}
			}
			else
			{
				if (tr.world->surfacesViewCount[surf] != tr.viewCount)
				{
					tr.world->surfacesViewCount[surf] = tr.viewCount;
					tr.world->surfacesDlightBits[surf] = dlightBits;
					tr.world->surfacesPshadowBits[surf] = pshadowBits;
				}
				else
				{
					tr.world->surfacesDlightBits[surf] |= dlightBits;
					tr.world->surfacesPshadowBits[surf] |= pshadowBits;
				}
			}
			view++;
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
		ri.Error (ERR_DROP, "R_PointInLeaf: bad model");
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
	if (!tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters ) {
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
	if ( !(mask[cluster>>3] & (1<<(cluster&7))) )
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
void R_MarkLeaves( void )
{
	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	mnode_t	*leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	int cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	for (int i = 0; i < MAX_VISCOUNTS; i++)
	{
		// if the areamask or r_showcluster was modified, invalidate all visclusters
		// this caused doors to open into undrawn areas
		if (tr.refdef.areamaskModified || r_showcluster->modified)
		{
			tr.visClusters[i] = -2;
		}
		else if (tr.visClusters[i] == cluster)
		{
			if (tr.visClusters[i] != tr.visClusters[tr.visIndex] && r_showcluster->integer)
			{
				ri.Printf(PRINT_ALL, "found cluster:%i  area:%i  index:%i\n",
					cluster, leaf->area, i);
			}

			tr.visIndex = i;
			return;
		}
	}

	tr.visIndex = (tr.visIndex + 1) % MAX_VISCOUNTS;
	tr.visCounts[tr.visIndex]++;
	tr.visClusters[tr.visIndex] = cluster;

	if ( r_showcluster->modified || r_showcluster->integer ) {
		r_showcluster->modified = qfalse;
		if ( r_showcluster->integer ) {
			ri.Printf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
		}
	}

	const byte *vis = R_ClusterPVS(tr.visClusters[tr.visIndex]);

	int i;
	for (i = 0, leaf = (tr.world->nodes + tr.world->numDecisionNodes); i < (tr.world->numnodes - tr.world->numDecisionNodes); i++, leaf++) {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters ) {
			continue;
		}

		// check general pvs
		if ( !(vis[cluster>>3] & (1<<(cluster&7))) ) {
			continue;
		}

		// Handle skyportal draws
		byte *areamask = tr.viewParms.isSkyPortal == qtrue ? tr.skyPortalAreaMask : tr.refdef.areamask;

		// check for door connection
		if ( (areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) ) {
			continue;		// not visible
		}

		mnode_t *parent = leaf;
		do {
			if (parent->visCounts[tr.visIndex] == tr.visCounts[tr.visIndex]) {
				break;
			}

			parent->visCounts[tr.visIndex] = tr.visCounts[tr.visIndex];
			parent = parent->parent;
		} while (parent);
	}
}


/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces( viewParms_t *viewParms, trRefdef_t *refdef ) {
	int planeBits, dlightBits, pshadowBits;

	if ( !r_drawworld->integer ) {
		return;
	}

	if ( refdef->rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	// determine which leaves are in the PVS / areamask
	if (!(viewParms->flags & VPF_DEPTHSHADOW)) {
		R_MarkLeaves();
	}

	// clear out the visible min/max
	ClearBounds(viewParms->visBounds[0], viewParms->visBounds[1]);

	// perform frustum culling and flag all the potentially visible surfaces
	refdef->num_dlights = Q_min(refdef->num_dlights, 32);
	refdef->num_pshadows = Q_min(refdef->num_pshadows, 32);

	planeBits = (viewParms->flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;

	if ( viewParms->flags & VPF_DEPTHSHADOW )
	{
		dlightBits = 0;
		pshadowBits = 0;
	}
	else
	{
		dlightBits = (1 << refdef->num_dlights) - 1;
		if (r_shadows->integer == 4)
			pshadowBits = (1 << refdef->num_pshadows) - 1;
		else
			pshadowBits = 0;
	}

	R_RecursiveWorldNode(tr.world->nodes, planeBits, dlightBits, pshadowBits);

	// now add all the potentially visible surfaces
	R_RotateForEntity(&tr.worldEntity, &tr.viewParms, &tr.ori);

	for (int i = 0; i < tr.world->numWorldSurfaces; i++)
	{
		if (tr.world->surfacesViewCount[i] != tr.viewCount)
			continue;

		R_AddWorldSurface(
			tr.world->surfaces + i,
			nullptr,
			REFENTITYNUM_WORLD,
			tr.world->surfacesDlightBits[i],
			tr.world->surfacesPshadowBits[i]);
	}

	for (int i = 0; i < tr.world->numMergedSurfaces; i++)
	{
		if (tr.world->mergedSurfacesViewCount[i] != tr.viewCount)
			continue;

		R_AddWorldSurface(
			tr.world->mergedSurfaces + i,
			nullptr,
			REFENTITYNUM_WORLD,
			tr.world->mergedSurfacesDlightBits[i],
			tr.world->mergedSurfacesPshadowBits[i]);
	}
}
