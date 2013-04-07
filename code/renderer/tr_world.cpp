// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "tr_local.h"

#ifdef VV_LIGHTING
#include "tr_lightmanager.h"
#endif

#ifdef _XBOX
#include "../qcommon/sparc.h"
#endif

static bool lookingForWorstLeaf = false;

#ifdef _XBOX
static bool GetCoordsForLeaf(int leafNum, vec3_t coords)
{
	srfSurfaceFace_t *face;
	msurface_t *surf;
	int i;

	for(i=0; i<tr.world->leafs[leafNum].nummarksurfaces; i++) {
		surf = *(tr.world->marksurfaces + 
				tr.world->leafs[leafNum].firstMarkSurfNum + i);
		
		if(!surf->data || *surf->data != SF_FACE) {
			continue;
		}

		face = (srfSurfaceFace_t*)surf->data;
		Q_CastShort2Float(&coords[0], (short*)(face->srfPoints + 0));
		Q_CastShort2Float(&coords[1], (short*)(face->srfPoints + 1));
		Q_CastShort2Float(&coords[2], (short*)(face->srfPoints + 2));
		return true;
	}

	return false;
}
#endif

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

	if ( tr.currentEntityNum != TR_WORLDENT ) {
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

	if ( r_nocull->integer==1 ) {
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
	d = DotProduct (tr.or.viewOrigin, sface->plane.normal);

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


#ifndef VV_LIGHTING
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
#endif // VV_LIGHTING

#ifndef VV_LIGHTING
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
#endif // VV_LIGHTING


#ifndef VV_LIGHTING
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
#endif // VV_LIGHTING

/*
====================
R_DlightSurface

The given surface is going to be drawn, and it touches a leaf
that is touched by one or more dlights, so try to throw out
more dlights if possible.
====================
*/
#ifndef VV_LIGHTING
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
#endif // VV_LIGHTING



/*
======================
R_AddWorldSurface
======================
*/
#ifdef VV_LIGHTING
void R_AddWorldSurface( msurface_t *surf, int dlightBits, qboolean noViewCount ) {
#else
static void R_AddWorldSurface( msurface_t *surf, int dlightBits, qboolean noViewCount = qfalse ) {
#endif
	/*
	if ( surf->viewCount == tr.viewCount ) {
		return;		// already in this view
	}
	*/

	//rww - changed this to be like sof2mp's so RMG will look right.
	//Will this affect anything that is non-rmg?

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

//	surf->viewCount = tr.viewCount;
	// FIXME: bmodel fog?

	// try to cull before dlighting or adding
	if ( R_CullSurface( surf->data, surf->shader ) ) {
		return;
	}

	// check for dlighting
	if ( dlightBits ) {
#ifdef VV_LIGHTING
		dlightBits = VVLightMan.R_DlightSurface( surf, dlightBits );
#else
		dlightBits = R_DlightSurface( surf, dlightBits );
#endif
		dlightBits = ( dlightBits != 0 );
	}

	R_AddDrawSurf( surf->data, surf->shader, surf->fogIndex, dlightBits );
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
	{
#ifdef VV_LIGHTING
		VVLightMan.R_SetupEntityLighting(&tr.refdef, ent);
#else
		R_SetupEntityLighting(&tr.refdef, ent);
#endif
	}

#ifdef VV_LIGHTING
	VVLightMan.R_DlightBmodel( bmodel, qfalse );
#else
	R_DlightBmodel( bmodel, qfalse );
#endif

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

#ifdef _XBOX
float GetQuadArea( unsigned short v1[3], unsigned short v2[3], unsigned short v3[3], unsigned short v4[3])
{
	vec3_t fv1;
	vec3_t fv2;
	vec3_t fv3;
	vec3_t fv4;

	for(int i=0; i<3; i++) {
		Q_CastShort2Float(&fv1[i], (short*)&v1[i]);
		Q_CastShort2Float(&fv2[i], (short*)&v2[i]);
		Q_CastShort2Float(&fv3[i], (short*)&v3[i]);
		Q_CastShort2Float(&fv4[i], (short*)&v4[i]);
	}

	return GetQuadArea(fv1, fv2, fv3, fv4);
}
#endif

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
#ifdef _XBOX
		int nextSurfPoint = NEXT_SURFPOINT(face->flags);
		dist = GetQuadArea( face->srfPoints, face->srfPoints + nextSurfPoint, 
				face->srfPoints + nextSurfPoint * 2, face->srfPoints +
						 nextSurfPoint * 3 );
#else
		dist = GetQuadArea( face->points[0], face->points[1], face->points[2], face->points[3] );
#endif

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

#ifdef _XBOX
	int nextSurfPoint = NEXT_SURFPOINT(face->flags);
	for ( int t = 0; t < 4; t++ )
	{
		Q_CastShort2Float(&verts[t][0], (short*)(face->srfPoints + nextSurfPoint * t + 0));
		Q_CastShort2Float(&verts[t][1], (short*)(face->srfPoints + nextSurfPoint * t + 1));
		Q_CastShort2Float(&verts[t][2], (short*)(face->srfPoints + nextSurfPoint * t + 2));
	}
#else
	for ( int t = 0; t < 4; t++ )
	{
		VectorCopy(	face->points[t], verts[t] );
	}
#endif
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
#ifndef VV_LIGHTING
static void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits ) {

	do {
		int			newDlights[2];

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if ( r_nocull->integer!=1 ) {
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
#endif // VV_LIGHTING


/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf( vec3_t p ) {
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
#ifdef _XBOX
		plane = tr.world->planes + node->planeNum;
#else
		plane = node->plane;
#endif
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

#ifdef _XBOX
	return tr.world->vis->Decompress(cluster * tr.world->clusterBytes,
			tr.world->numClusters);
#else
	return tr.world->vis + cluster * tr.world->clusterBytes;
#endif
}

/*
=================
R_inPVS
=================
*/
#ifdef _XBOX
qboolean R_inPVS( vec3_t p1, vec3_t p2 ) {
	mleaf_s *leaf;
	byte	*vis;

	leaf = (mleaf_s*)R_PointInLeaf( p1 );
	vis = (byte*)CM_ClusterPVS( leaf->cluster );
	leaf = (mleaf_s*)R_PointInLeaf( p2 );

	if ( !vis || (!(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7)))) ) {
		return qfalse;
	}
	return qtrue;
}
#else // _XBOX

qboolean R_inPVS( vec3_t p1, vec3_t p2 ) {
	mnode_t *leaf;
	byte	*vis;

	leaf = R_PointInLeaf( p1 );
	vis = CM_ClusterPVS( leaf->cluster );
	leaf = R_PointInLeaf( p2 );

	if ( !(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))) ) {
		return qfalse;
	}
	return qtrue;
}
#endif // _XBOX

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
#ifdef _XBOX
void R_MarkLeaves (mleaf_s *leafOverride) {
	const byte	*vis;
	mleaf_s	*leaf;
   	mnode_s	*parent;
	int		i;
	int		cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	if(!leafOverride) {
		leaf = (mleaf_s*)R_PointInLeaf( tr.viewParms.pvsOrigin );
	} else {
		leaf = leafOverride;
	}
	cluster = leaf->cluster;

	assert(leaf->contents != -1);

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	if ( tr.viewCluster == cluster && !tr.refdef.areamaskModified ) {
		return;
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
	
	for (i=0,leaf=tr.world->leafs ; i<tr.world->numleafs ; i++, leaf++) {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters ) {
			continue;
		}

		// check general pvs
		if ( !(vis[cluster>>3] & (1<<(cluster&7))) ) {
			continue;
		}

		// check for door connection
		if (!lookingForWorstLeaf &&
			   (tr.refdef.areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) ) {
			continue;		// not visible
		}

		parent = (mnode_t*)leaf;
		assert(leaf->contents != -1);
		do {
			if (parent->visframe == tr.visCount)
				break;
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}
#else // _XBOX

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
			VID_Printf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
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
#endif


/*
=============
R_AddWorldSurfaces
=============
*/
#ifdef _XBOX
void R_AddWorldSurfaces (void) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	tr.currentEntityNum = TR_WORLDENT;//ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );

	// perform frustum culling and add all the potentially visible surfaces
	if ( VVLightMan.num_dlights > MAX_DLIGHTS ) {
		VVLightMan.num_dlights = MAX_DLIGHTS ;
	}

	VVLightMan.R_RecursiveWorldNode( tr.world->nodes, 15, ( 1 << VVLightMan.num_dlights ) - 1 );
}

#else // _XBOX

void R_AddWorldSurfaces (void) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	tr.currentEntityNum = TR_WORLDENT;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves ();

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );

	// perform frustum culling and add all the potentially visible surfaces
	if ( tr.refdef.num_dlights > 32 ) {
		tr.refdef.num_dlights = 32 ;
	}

	R_RecursiveWorldNode( tr.world->nodes, 31, ( 1 << tr.refdef.num_dlights ) - 1 );
}

#endif // _XBOX
