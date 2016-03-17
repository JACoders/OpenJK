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

#include "cm_local.h"

/*
===============================================================================

POSITION TESTING

===============================================================================
*/

/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush( traceWork_t *tw, cbrush_t *brush ) {
	int			i;
	cplane_t	*plane;
	float		dist;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides) {
		return;
	}

	// special test for axial
	if ( tw->bounds[0][0] > brush->bounds[1][0]
		|| tw->bounds[0][1] > brush->bounds[1][1]
		|| tw->bounds[0][2] > brush->bounds[1][2]
		|| tw->bounds[1][0] < brush->bounds[0][0]
		|| tw->bounds[1][1] < brush->bounds[0][1]
		|| tw->bounds[1][2] < brush->bounds[0][2]
		) {
		return;
	}

	// the first six planes are the axial planes, so we only
	// need to test the remainder
	for ( i = 6 ; i < brush->numsides ; i++ ) {
		side = brush->sides + i;
		plane = side->plane;

		// adjust the plane distance apropriately for mins/maxs
		dist = plane->dist - DotProduct( tw->offsets[ plane->signbits ], plane->normal );

		d1 = DotProduct( tw->start, plane->normal ) - dist;

		// if completely in front of face, no intersection
		if ( d1 > 0 ) {
			return;
		}
	}

	// inside this brush
	tw->trace.startsolid = tw->trace.allsolid = qtrue;
	tw->trace.fraction = 0;
	tw->trace.contents = brush->contents;
}

/*
================
CM_PlaneCollision

  Returns false for a quick getout
================
*/

bool CM_PlaneCollision(traceWork_t *tw, cbrushside_t *side)
{
	float			dist, f;
	float			d1, d2;
	cplane_t		*plane = side->plane;

	// adjust the plane distance apropriately for mins/maxs
	dist = plane->dist - DotProduct( tw->offsets[ plane->signbits ], plane->normal );

	d1 = DotProduct( tw->start, plane->normal ) - dist;
	d2 = DotProduct( tw->end, plane->normal ) - dist;

	if (d2 > 0.0f)
	{
		// endpoint is not in solid
		tw->getout = true;
	}
	if (d1 > 0.0f)
	{
		// startpoint is not in solid
		tw->startout = true;
	}

	// if completely in front of face, no intersection with the entire brush
	if ((d1 > 0.0f) && ( (d2 >= SURFACE_CLIP_EPSILON) || (d2 >= d1) ) )
	{
		return(false);
	}

	// if it doesn't cross the plane, the plane isn't relevent
	if ((d1 <= 0.0f) && (d2 <= 0.0f))
	{
		return(true);
	}
	// crosses face
	if (d1 > d2)
	{	// enter
		f = (d1 - SURFACE_CLIP_EPSILON);
		if ( f < 0.0f )
		{
			f = 0.0f;
			if (f > tw->enterFrac)
			{
				tw->enterFrac = f;
				tw->clipplane = plane;
				tw->leadside = side;
			}
		}
		else if (f > tw->enterFrac * (d1 - d2) )
		{
			tw->enterFrac = f / (d1 - d2);
			tw->clipplane = plane;
			tw->leadside = side;
		}
	}
	else
	{	// leave
		f = (d1 + SURFACE_CLIP_EPSILON);
		if ( f < (d1 - d2) )
		{
			f = 1.0f;
			if (f < tw->leaveFrac)
			{
				tw->leaveFrac = f;
			}
		}
		else if (f > tw->leaveFrac * (d1 - d2) )
		{
			tw->leaveFrac = f / (d1 - d2);
		}
	}
	return(true);
}

/*
================
CM_TraceThroughBrush
================
*/
void CM_TraceThroughBrush( traceWork_t *tw, trace_t &trace, cbrush_t *brush, bool infoOnly )
{
	int				i;
	cbrushside_t	*side;

	tw->enterFrac = -1.0f;
	tw->leaveFrac = 1.0f;
	tw->clipplane = NULL;

	if ( !brush->numsides )
	{
		return;
	}

	tw->getout = false;
	tw->startout = false;
	tw->leadside = NULL;

	//
	// compare the trace against all planes of the brush
	// find the latest time the trace crosses a plane towards the interior
	// and the earliest time the trace crosses a plane towards the exterior
	//
	for (i = 0; i < brush->numsides; i++)
	{
		side = brush->sides + i;

		if(!CM_PlaneCollision(tw, side))
		{
			return;
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!tw->startout)
	{
		if(!infoOnly)
		{
			// original point was inside brush
			trace.startsolid = qtrue;
			if (!tw->getout)
			{
				trace.allsolid = qtrue;
				trace.fraction = 0.0f;
			}
		}
		tw->enterFrac = 0.0f;
		return;
	}

	if (tw->enterFrac < tw->leaveFrac)
	{
		if ((tw->enterFrac > -1.0f) && (tw->enterFrac < trace.fraction))
		{
			if (tw->enterFrac < 0.0f)
			{
				tw->enterFrac = 0.0f;
			}
			if(!infoOnly)
			{
				trace.fraction = tw->enterFrac;
				trace.plane = *tw->clipplane;
				trace.surfaceFlags = cmg.shaders[tw->leadside->shaderNum].surfaceFlags;
//				tw->trace.sideNum = tw->leadside - cmg.brushsides;
				trace.contents = brush->contents;
			}
		}
	}
}

/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf( traceWork_t *tw, cLeaf_t *leaf, clipMap_t *local ) {
	int			k;
	int			brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	// test box position against all brushes in the leaf
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = local->leafbrushes[leaf->firstLeafBrush+k];
		b = &local->brushes[brushnum];
		if (b->checkcount == local->checkcount) {
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = local->checkcount;

		if ( !(b->contents & tw->contents)) {
			continue;
		}

		CM_TestBoxInBrush( tw, b );
		if ( tw->trace.allsolid ) {
			return;
		}
	}

	// test against all patches
#ifdef BSPC
	if (1) {
#else
	if ( !cm_noCurves->integer ) {
#endif //BSPC
		for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) {
			patch = local->surfaces[ local->leafsurfaces[ leaf->firstLeafSurface + k ] ];

			if ( !patch ) {
				continue;
			}
			if ( patch->checkcount == local->checkcount ) {
				continue;	// already checked this brush in another leaf
			}
			patch->checkcount = local->checkcount;

			if ( !(patch->contents & tw->contents)) {
				continue;
			}

			if ( CM_PositionTestInPatchCollide( tw, patch->pc ) ) {
				tw->trace.startsolid = tw->trace.allsolid = qtrue;
				tw->trace.fraction = 0;
				tw->trace.contents = patch->contents;
				return;
			}
		}
	}
}

/*
==================
CM_PositionTest
==================
*/
#define	MAX_POSITION_LEAFS	1024
void CM_PositionTest( traceWork_t *tw ) {
	int		leafs[MAX_POSITION_LEAFS];
	int		i;
	leafList_t	ll;

	// identify the leafs we are touching
	VectorAdd( tw->start, tw->size[0], ll.bounds[0] );
	VectorAdd( tw->start, tw->size[1], ll.bounds[1] );

	for (i=0 ; i<3 ; i++) {
		ll.bounds[0][i] -= 1;
		ll.bounds[1][i] += 1;
	}

	ll.count = 0;
	ll.maxcount = MAX_POSITION_LEAFS;
	ll.list = leafs;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = qfalse;

	cmg.checkcount++;

	CM_BoxLeafnums_r( &ll, 0 );


	cmg.checkcount++;

	// test the contents of the leafs
	for (i=0 ; i < ll.count ; i++) {
		CM_TestInLeaf( tw, &cmg.leafs[leafs[i]], &cmg );
		if ( tw->trace.allsolid ) {
			break;
		}
	}
}

/*
===============================================================================

BOX TRACING

===============================================================================
*/


/*
================
CM_TraceThroughPatch
================
*/

void CM_TraceThroughPatch( traceWork_t *tw, cPatch_t *patch ) {
	float		oldFrac;

	c_patch_traces++;

	oldFrac = tw->trace.fraction;

	CM_TraceThroughPatchCollide( tw, patch->pc );

	if ( tw->trace.fraction < oldFrac ) {
		tw->trace.surfaceFlags = patch->surfaceFlags;
		tw->trace.contents = patch->contents;
	}
}


/*
================
CM_TraceThroughBrush
================
*/
void CM_TraceThroughBrush( traceWork_t *tw, cbrush_t *brush ) {
	int			i;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterFrac, leaveFrac;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	enterFrac = -1.0;
	leaveFrac = 1.0;
	clipplane = NULL;

	if ( !brush->numsides ) {
		return;
	}

	// I'm not sure if test is strictly correct.  Are all
	// bboxes axis aligned?  Do I care?  It seems to work
	// good enough...
	if ( tw->bounds[0][0] > brush->bounds[1][0]
		|| tw->bounds[0][1] > brush->bounds[1][1]
		|| tw->bounds[0][2] > brush->bounds[1][2]
		|| tw->bounds[1][0] < brush->bounds[0][0]
		|| tw->bounds[1][1] < brush->bounds[0][1]
		|| tw->bounds[1][2] < brush->bounds[0][2]
		) {
		return;
	}

	c_brush_traces++;

	getout = qfalse;
	startout = qfalse;

	leadside = NULL;

	//
	// compare the trace against all planes of the brush
	// find the latest time the trace crosses a plane towards the interior
	// and the earliest time the trace crosses a plane towards the exterior
	//
	for (i=0 ; i<brush->numsides ; i++) {
		side = brush->sides + i;
		plane = side->plane;

		// adjust the plane distance apropriately for mins/maxs
		dist = plane->dist - DotProduct( tw->offsets[ plane->signbits ], plane->normal );

		d1 = DotProduct( tw->start, plane->normal ) - dist;
		d2 = DotProduct( tw->end, plane->normal ) - dist;

		if (d2 > 0) {
			getout = qtrue;	// endpoint is not in solid
		}
		if (d1 > 0) {
			startout = qtrue;
		}

		// if completely in front of face, no intersection with the entire brush
		if (d1 > 0 && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 )  ) {
			return;
		}

		// if it doesn't cross the plane, the plane isn't relevent
		if (d1 <= 0 && d2 <= 0 ) {
			continue;
		}

		// crosses face
		if (d1 > d2) {	// enter
			f = (d1-SURFACE_CLIP_EPSILON) / (d1-d2);
			if ( f < 0 ) {
				f = 0;
			}
			if (f > enterFrac) {
				enterFrac = f;
				clipplane = plane;
				leadside = side;
			}
		} else {	// leave
			f = (d1+SURFACE_CLIP_EPSILON) / (d1-d2);
			if ( f > 1 ) {
				f = 1;
			}
			if (f < leaveFrac) {
				leaveFrac = f;
			}
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!startout) {	// original point was inside brush
		tw->trace.startsolid = qtrue;
		tw->trace.contents |= brush->contents;	//note, we always want to know the contents of something we're inside of
		if (!getout)
		{	//endpoint was inside brush
			tw->trace.allsolid = qtrue;
			tw->trace.fraction = 0;
		}
		return;
	}

	if (enterFrac < leaveFrac) {
		if (enterFrac > -1 && enterFrac < tw->trace.fraction) {
			if (enterFrac < 0) {
				enterFrac = 0;
			}
			tw->trace.fraction = enterFrac;
			tw->trace.plane = *clipplane;
			tw->trace.surfaceFlags = cmg.shaders[leadside->shaderNum].surfaceFlags;
			tw->trace.contents = brush->contents;
		}
	}
}

/*
================
CM_GenericBoxCollide
================
*/

bool CM_GenericBoxCollide(const vec3pair_t abounds, const vec3pair_t bbounds)
{
	int		i;

	// Check for completely no intersection
	for(i = 0; i < 3; i++)
	{
		if(abounds[1][i] < bbounds[0][i])
		{
			return(false);
		}
		if(abounds[0][i] > bbounds[1][i])
		{
			return(false);
		}
	}
	return(true);
}

/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf( traceWork_t *tw, cLeaf_t *leaf, clipMap_t *local ) {
	int			k;
	int			brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	// trace line against all brushes in the leaf
	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ ) {
		brushnum = local->leafbrushes[leaf->firstLeafBrush+k];

		b = &local->brushes[brushnum];
		if ( b->checkcount == local->checkcount ) {
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = local->checkcount;

		if ( !(b->contents & tw->contents) ) {
			continue;
		}

		//if (b->contents & CONTENTS_PLAYERCLIP) continue;

		CM_TraceThroughBrush( tw, b );
		if ( !tw->trace.fraction ) {
			return;
		}
	}

	// trace line against all patches in the leaf
#ifdef BSPC
	if (1) {
#else
	if ( !cm_noCurves->integer ) {
#endif
		for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) {
			patch = local->surfaces[ local->leafsurfaces[ leaf->firstLeafSurface + k ] ];
			if ( !patch ) {
				continue;
			}
			if ( patch->checkcount == local->checkcount ) {
				continue;	// already checked this patch in another leaf
			}
			patch->checkcount = local->checkcount;

			if ( !(patch->contents & tw->contents) ) {
				continue;
			}

			CM_TraceThroughPatch( tw, patch );
			if ( !tw->trace.fraction ) {
				return;
			}
		}
	}
}

//=========================================================================================

/*
==================
CM_TraceThroughTree

Traverse all the contacted leafs from the start to the end position.
If the trace is a point, they will be exactly in order, but for larger
trace volumes it is possible to hit something in a later leaf with
a smaller intercept fraction.
==================
*/
void CM_TraceThroughTree( traceWork_t *tw, clipMap_t *local, int num, float p1f, float p2f, vec3_t p1, vec3_t p2) {
	cNode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	vec3_t		mid;
	int			side;
	float		midf;

	if (tw->trace.fraction <= p1f) {
		return;		// already hit something nearer
	}

	// if < 0, we are in a leaf node
	if (num < 0) {
		CM_TraceToLeaf( tw, &local->leafs[-1-num], local );
		return;
	}

	//
	// find the point distances to the separating plane
	// and the offset for the size of the box
	//
	node = local->nodes + num;
	plane = node->plane;

#if 0
	// uncomment this to test against every leaf in the world for debugging
CM_TraceThroughTree( tw, local, node->children[0], p1f, p2f, p1, p2 );
CM_TraceThroughTree( tw, local, node->children[1], p1f, p2f, p1, p2 );
return;
#endif

	// adjust the plane distance apropriately for mins/maxs
	if ( plane->type < 3 ) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = tw->extents[plane->type];
	} else {
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if ( tw->isPoint ) {
			offset = 0;
		} else {
			// an axial brush right behind a slanted bsp plane
			// will poke through when expanded, so adjust
			// by sqrt(3)
			offset = fabs(tw->extents[0]*plane->normal[0]) +
				fabs(tw->extents[1]*plane->normal[1]) +
				fabs(tw->extents[2]*plane->normal[2]);

			offset *= 2;
#if 0
CM_TraceThroughTree( tw, local, node->children[0], p1f, p2f, p1, p2 );
CM_TraceThroughTree( tw, local, node->children[1], p1f, p2f, p1, p2 );
return;
#endif
			offset = tw->maxOffset;
			offset = 2048;
		}
	}

	// see which sides we need to consider
	if ( t1 >= offset + 1 && t2 >= offset + 1 ) {
		CM_TraceThroughTree( tw, local, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -offset - 1 && t2 < -offset - 1 ) {
		CM_TraceThroughTree( tw, local, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	// put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
	if ( t1 < t2 ) {
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
		frac = (t1 - offset + SURFACE_CLIP_EPSILON)*idist;
	} else if (t1 > t2) {
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - SURFACE_CLIP_EPSILON)*idist;
		frac = (t1 + offset + SURFACE_CLIP_EPSILON)*idist;
	} else {
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if ( frac < 0 ) {
		frac = 0;
	}
	if ( frac > 1 ) {
		frac = 1;
	}

	midf = p1f + (p2f - p1f)*frac;

	mid[0] = p1[0] + frac*(p2[0] - p1[0]);
	mid[1] = p1[1] + frac*(p2[1] - p1[1]);
	mid[2] = p1[2] + frac*(p2[2] - p1[2]);

	CM_TraceThroughTree( tw, local, node->children[side], p1f, midf, p1, mid );


	// go past the node
	if ( frac2 < 0 ) {
		frac2 = 0;
	}
	if ( frac2 > 1 ) {
		frac2 = 1;
	}

	midf = p1f + (p2f - p1f)*frac2;

	mid[0] = p1[0] + frac2*(p2[0] - p1[0]);
	mid[1] = p1[1] + frac2*(p2[1] - p1[1]);
	mid[2] = p1[2] + frac2*(p2[2] - p1[2]);

	CM_TraceThroughTree( tw, local, node->children[side^1], midf, p2f, mid, p2 );
}

void CM_CalcExtents(const vec3_t start, const vec3_t end, const traceWork_t *tw, vec3pair_t bounds)
{
	int		i;

	for ( i = 0 ; i < 3 ; i++ )
	{
		if ( start[i] < end[i] )
		{
			bounds[0][i] = start[i] + tw->size[0][i];
			bounds[1][i] = end[i] + tw->size[1][i];
		}
		else
		{
			bounds[0][i] = end[i] + tw->size[0][i];
			bounds[1][i] = start[i] + tw->size[1][i];
		}
	}
}

//======================================================================

/*
==================
CM_BoxTrace
==================
*/
void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask) {
	int			i;
	traceWork_t	tw;
	vec3_t		offset;
	cmodel_t	*cmod;
	clipMap_t	*local = 0;

	cmod = CM_ClipHandleToModel( model, &local );

	local->checkcount++;		// for multi-check avoidance

	c_traces++;				// for statistics, may be zeroed

	// fill in a default trace
	memset( &tw, 0, sizeof(tw) - sizeof(tw.trace.G2CollisionMap));
	tw.trace.fraction = 1;	// assume it goes the entire distance until shown otherwise

	if (!local->numNodes) {
		*results = tw.trace;
		return;	// map not loaded, shouldn't happen
	}

	// allow NULL to be passed in for 0,0,0
	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	// set basic parms
	tw.contents = brushmask;

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		tw.size[0][i] = mins[i] - offset[i];
		tw.size[1][i] = maxs[i] - offset[i];
		tw.start[i] = start[i] + offset[i];
		tw.end[i] = end[i] + offset[i];
	}

	tw.maxOffset = tw.size[1][0] + tw.size[1][1] + tw.size[1][2];

	// tw.offsets[signbits] = vector to apropriate corner from origin
	tw.offsets[0][0] = tw.size[0][0];
	tw.offsets[0][1] = tw.size[0][1];
	tw.offsets[0][2] = tw.size[0][2];

	tw.offsets[1][0] = tw.size[1][0];
	tw.offsets[1][1] = tw.size[0][1];
	tw.offsets[1][2] = tw.size[0][2];

	tw.offsets[2][0] = tw.size[0][0];
	tw.offsets[2][1] = tw.size[1][1];
	tw.offsets[2][2] = tw.size[0][2];

	tw.offsets[3][0] = tw.size[1][0];
	tw.offsets[3][1] = tw.size[1][1];
	tw.offsets[3][2] = tw.size[0][2];

	tw.offsets[4][0] = tw.size[0][0];
	tw.offsets[4][1] = tw.size[0][1];
	tw.offsets[4][2] = tw.size[1][2];

	tw.offsets[5][0] = tw.size[1][0];
	tw.offsets[5][1] = tw.size[0][1];
	tw.offsets[5][2] = tw.size[1][2];

	tw.offsets[6][0] = tw.size[0][0];
	tw.offsets[6][1] = tw.size[1][1];
	tw.offsets[6][2] = tw.size[1][2];

	tw.offsets[7][0] = tw.size[1][0];
	tw.offsets[7][1] = tw.size[1][1];
	tw.offsets[7][2] = tw.size[1][2];


	//
	// calculate bounds
	//
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( tw.start[i] < tw.end[i] ) {
			tw.bounds[0][i] = tw.start[i] + tw.size[0][i];
			tw.bounds[1][i] = tw.end[i] + tw.size[1][i];
		} else {
			tw.bounds[0][i] = tw.end[i] + tw.size[0][i];
			tw.bounds[1][i] = tw.start[i] + tw.size[1][i];
		}
	}

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2]) {
		if ( model ) {
			CM_TestInLeaf( &tw, &cmod->leaf, local );
		} else {
			CM_PositionTest( &tw );
		}
	} else {
		//
		// check for point special case
		//
		if ( tw.size[0][0] == 0 && tw.size[0][1] == 0 && tw.size[0][2] == 0 ) {
			tw.isPoint = qtrue;
			VectorClear( tw.extents );
		} else {
			tw.isPoint = qfalse;
			tw.extents[0] = tw.size[1][0];
			tw.extents[1] = tw.size[1][1];
			tw.extents[2] = tw.size[1][2];
		}

		//
		// general sweeping through world
		//
		if ( model ) {
			CM_TraceToLeaf( &tw, &cmod->leaf, local );
		} else {
			CM_TraceThroughTree( &tw, local, 0, 0, 1, tw.start, tw.end );
		}
	}

	// generate endpos from the original, unmodified start/end
	if ( tw.trace.fraction == 1 ) {
		VectorCopy (end, tw.trace.endpos);
	} else {
		for ( i=0 ; i<3 ; i++ ) {
			tw.trace.endpos[i] = start[i] + tw.trace.fraction * (end[i] - start[i]);
		}
	}

	*results = tw.trace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles) {
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;
	vec3_t		offset;
	vec3_t		symetricSize[2];
	int			i;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
		start_l[i] = start[i] + offset[i];
		end_l[i] = end[i] + offset[i];
	}

	// subtract origin offset
	VectorSubtract( start_l, origin, start_l );
	VectorSubtract( end_l, origin, end_l );

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE &&
		(angles[0] || angles[1] || angles[2]) ) {
		rotated = qtrue;
	} else {
		rotated = qfalse;
	}

	if (rotated) {
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	CM_BoxTrace( &trace, start_l, end_l, symetricSize[0], symetricSize[1], model, brushmask);

	if ( rotated && trace.fraction != 1.0 ) {
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	*results = trace;
}

/*
=================
CM_CullBox

Returns true if culled out
=================
*/

bool CM_CullBox(const cplane_t *frustum, const vec3_t transformed[8])
{
	int				i, j;
	const cplane_t	*frust;

	// check against frustum planes
	for (i=0, frust=frustum; i<4 ; i++, frust++)
	{
		for (j=0 ; j<8 ; j++)
		{
			if (DotProduct(transformed[j], frust->normal) > frust->dist)
			{	// a point is in front
				break;
			}
		}

		if (j == 8)
		{	// all points were behind one of the planes
			return true;
		}
	}
	return false;
}

/*
=================
CM_CullWorldBox

Returns true if culled out
=================
*/

bool CM_CullWorldBox (const cplane_t *frustum, const vec3pair_t bounds)
{
	int			i;
	vec3_t		transformed[8];

	for (i = 0 ; i < 8 ; i++)
	{
		transformed[i][0] = bounds[i & 1][0];
		transformed[i][1] = bounds[(i >> 1) & 1][1];
		transformed[i][2] = bounds[(i >> 2) & 1][2];
	}

	//rwwFIXMEFIXME: Was not ! before. But that seems the way it should be and it works that way. Why?
	return(!CM_CullBox(frustum, transformed));
}
