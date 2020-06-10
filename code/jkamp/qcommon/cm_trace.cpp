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

// always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
//#define ALWAYS_BBOX_VS_BBOX
// always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
//#define ALWAYS_CAPSULE_VS_CAPSULE

//#define CAPSULE_DEBUG

/*
===============================================================================

BASIC MATH

===============================================================================
*/

/*
================
RotatePoint
================
*/
void RotatePoint(vec3_t point, /*const*/ matrix3_t matrix) { // bk: FIXME
	vec3_t tvec;

	VectorCopy(point, tvec);
	point[0] = DotProduct(matrix[0], tvec);
	point[1] = DotProduct(matrix[1], tvec);
	point[2] = DotProduct(matrix[2], tvec);
}

/*
================
TransposeMatrix
================
*/
void TransposeMatrix(/*const*/ matrix3_t matrix, matrix3_t transpose) { // bk: FIXME
	int i, j;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
CreateRotationMatrix
================
*/
void CreateRotationMatrix(const vec3_t angles, matrix3_t matrix) {
	AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
	VectorInverse(matrix[1]);
}

/*
================
CM_ProjectPointOntoVector
================
*/
void CM_ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vDir, vec3_t vProj )
{
	vec3_t pVec;

	VectorSubtract( point, vStart, pVec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vDir ), vDir, vProj );
}

/*
================
CM_DistanceFromLineSquared
================
*/
float CM_DistanceFromLineSquared(vec3_t p, vec3_t lp1, vec3_t lp2, vec3_t dir) {
	vec3_t proj, t;
	int j;

	CM_ProjectPointOntoVector(p, lp1, dir, proj);
	for (j = 0; j < 3; j++)
		if ((proj[j] > lp1[j] && proj[j] > lp2[j]) ||
			(proj[j] < lp1[j] && proj[j] < lp2[j]))
			break;
	if (j < 3) {
		if (fabs(proj[j] - lp1[j]) < fabs(proj[j] - lp2[j]))
			VectorSubtract(p, lp1, t);
		else
			VectorSubtract(p, lp2, t);
		return VectorLengthSquared(t);
	}
	VectorSubtract(p, proj, t);
	return VectorLengthSquared(t);
}


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
void CM_TestBoxInBrush( traceWork_t *tw, trace_t &trace, cbrush_t *brush ) {
	int			i;
	cplane_t	*plane;
	float		dist;
	float		d1;
	cbrushside_t	*side;
	float		t;
	vec3_t		startp;

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

   if ( tw->sphere.use ) {
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for ( i = 6 ; i < brush->numsides ; i++ ) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance appropriately for radius
			dist = plane->dist + tw->sphere.radius;
			// find the closest point on the capsule to the plane
			t = DotProduct( plane->normal, tw->sphere.offset );
			if ( t > 0 )
			{
				VectorSubtract( tw->start, tw->sphere.offset, startp );
			}
			else
			{
				VectorAdd( tw->start, tw->sphere.offset, startp );
			}
			d1 = DotProduct( startp, plane->normal ) - dist;
			// if completely in front of face, no intersection
			if ( d1 > 0 ) {
				return;
			}
		}
	} else {
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for ( i = 6 ; i < brush->numsides ; i++ ) {
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance appropriately for mins/maxs
			dist = plane->dist - DotProduct( tw->offsets[ plane->signbits ], plane->normal );

			d1 = DotProduct( tw->start, plane->normal ) - dist;

			// if completely in front of face, no intersection
			if ( d1 > 0 ) {
				return;
			}
		}
	}

	// inside this brush
	trace.startsolid = trace.allsolid = qtrue;
	trace.fraction = 0;
	trace.contents = brush->contents;
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf( traceWork_t *tw, trace_t &trace, cLeaf_t *leaf, clipMap_t *local )
{
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

		CM_TestBoxInBrush( tw, trace, b );
		if ( trace.allsolid ) {
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
				trace.startsolid = trace.allsolid = qtrue;
				trace.fraction = 0;
				trace.contents = patch->contents;
				return;
			}
		}
	}
}

/*
==================
CM_TestCapsuleInCapsule

capsule inside capsule check
==================
*/
void CM_TestCapsuleInCapsule( traceWork_t *tw, trace_t &trace, clipHandle_t model ) {
	int i;
	vec3_t mins, maxs;
	vec3_t top, bottom;
	vec3_t p1, p2, tmp;
	vec3_t offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, r;

	CM_ModelBounds(model, mins, maxs);

	VectorAdd(tw->start, tw->sphere.offset, top);
	VectorSubtract(tw->start, tw->sphere.offset, bottom);
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];
	radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	offs = halfheight - radius;

	r = Square(tw->sphere.radius + radius);
	// check if any of the spheres overlap
	VectorCopy(offset, p1);
	p1[2] += offs;
	VectorSubtract(p1, top, tmp);
	if ( VectorLengthSquared(tmp) < r ) {
		trace.startsolid = trace.allsolid = qtrue;
		trace.fraction = 0;
	}
	VectorSubtract(p1, bottom, tmp);
	if ( VectorLengthSquared(tmp) < r ) {
		trace.startsolid = trace.allsolid = qtrue;
		trace.fraction = 0;
	}
	VectorCopy(offset, p2);
	p2[2] -= offs;
	VectorSubtract(p2, top, tmp);
	if ( VectorLengthSquared(tmp) < r ) {
		trace.startsolid = trace.allsolid = qtrue;
		trace.fraction = 0;
	}
	VectorSubtract(p2, bottom, tmp);
	if ( VectorLengthSquared(tmp) < r ) {
		trace.startsolid = trace.allsolid = qtrue;
		trace.fraction = 0;
	}
	// if between cylinder up and lower bounds
	if ( (top[2] >= p1[2] && top[2] <= p2[2]) ||
		(bottom[2] >= p1[2] && bottom[2] <= p2[2]) ) {
		// 2d coordinates
		top[2] = p1[2] = 0;
		// if the cylinders overlap
		VectorSubtract(top, p1, tmp);
		if ( VectorLengthSquared(tmp) < r ) {
			trace.startsolid = trace.allsolid = qtrue;
			trace.fraction = 0;
		}
	}
}

/*
==================
CM_TestBoundingBoxInCapsule

bounding box inside capsule check
==================
*/
void CM_TestBoundingBoxInCapsule( traceWork_t *tw, trace_t &trace, clipHandle_t model ) {
	vec3_t mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t *cmod;
	int i;

	// mins maxs of the capsule
	CM_ModelBounds(model, mins, maxs);

	// offset for capsule center
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1][0] > size[1][2] ) ? size[1][2]: size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet( tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius );

	// replace the capsule with the bounding box
	h = CM_TempBoxModel(tw->size[0], tw->size[1], qfalse);
	// calculate collision
	cmod = CM_ClipHandleToModel( h );
	CM_TestInLeaf( tw, trace, &cmod->leaf, &cmg );
}

/*
==================
CM_PositionTest
==================
*/
#define	MAX_POSITION_LEAFS	1024
void CM_PositionTest( traceWork_t *tw, trace_t &trace ) {
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
		CM_TestInLeaf( tw, trace, &cmg.leafs[leafs[i]], &cmg );
		if ( trace.allsolid ) {
			break;
		}
	}
}

/*
===============================================================================

TRACING

===============================================================================
*/


/*
================
CM_TraceThroughPatch
================
*/

void CM_TraceThroughPatch( traceWork_t *tw, trace_t &trace, cPatch_t *patch ) {
	float		oldFrac;

	c_patch_traces++;

	oldFrac = trace.fraction;

	CM_TraceThroughPatchCollide( tw, trace, patch->pc );

	if ( trace.fraction < oldFrac ) {
		trace.surfaceFlags = patch->surfaceFlags;
		trace.contents = patch->contents;
	}
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

	// adjust the plane distance appropriately for mins/maxs
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
				trace.contents = brush->contents;
			}
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
CM_TraceThroughLeaf
================
*/
void CM_TraceThroughLeaf( traceWork_t *tw, trace_t &trace, clipMap_t *local, cLeaf_t *leaf ) {
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

		CM_TraceThroughBrush( tw, trace, b, false );

		if ( !trace.fraction ) {
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

			CM_TraceThroughPatch( tw, trace, patch );
			if ( !trace.fraction ) {
				return;
			}
		}
	}
}

#define RADIUS_EPSILON		1.0f

/*
================
CM_TraceThroughSphere

get the first intersection of the ray with the sphere
================
*/
void CM_TraceThroughSphere( traceWork_t *tw, trace_t &trace, vec3_t origin, float radius, vec3_t start, vec3_t end ) {
	float l1, l2, length, scale, fraction;
	float /*a, */b, c, d, sqrtd;
	vec3_t v1, dir, intersection;

	// if inside the sphere
	VectorSubtract(start, origin, dir);
	l1 = VectorLengthSquared(dir);
	if (l1 < Square(radius)) {
		trace.fraction = 0;
		trace.startsolid = qtrue;
		// test for allsolid
		VectorSubtract(end, origin, dir);
		l1 = VectorLengthSquared(dir);
		if (l1 < Square(radius)) {
			trace.allsolid = qtrue;
		}
		return;
	}
	//
	VectorSubtract(end, start, dir);
	length = VectorNormalize(dir);
	//
	l1 = CM_DistanceFromLineSquared(origin, start, end, dir);
	VectorSubtract(end, origin, v1);
	l2 = VectorLengthSquared(v1);
	// if no intersection with the sphere and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON)) {
		return;
	}
	//
	//	| origin - (start + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (start[0] - origin[0]) + dir[1] * (start[1] - origin[1]) + dir[2] * (start[2] - origin[2]));
	//	c = (start[0] - origin[0])^2 + (start[1] - origin[1])^2 + (start[2] - origin[2])^2 - radius^2;
	//
	VectorSubtract(start, origin, v1);
	// dir is normalized so a = 1
	//a = 1.0f;//dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
	b = 2.0f * (dir[0] * v1[0] + dir[1] * v1[1] + dir[2] * v1[2]);
	c = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2] - (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;// * a;
	if (d > 0) {
		sqrtd = sqrtf(d);
		// = (- b + sqrtd) * 0.5f; // / (2.0f * a);
		fraction = (- b - sqrtd) * 0.5f; // / (2.0f * a);
		//
		if (fraction < 0) {
			fraction = 0;
		}
		else {
			fraction /= length;
		}
		if ( fraction < trace.fraction ) {
			trace.fraction = fraction;
			VectorSubtract(end, start, dir);
			VectorMA(start, fraction, dir, intersection);
			VectorSubtract(intersection, origin, dir);
			#ifdef CAPSULE_DEBUG
				l2 = VectorLength(dir);
				if (l2 < radius) {
					int bah = 1;
				}
			#endif
			scale = 1 / (radius+RADIUS_EPSILON);
			VectorScale(dir, scale, dir);
			VectorCopy(dir, trace.plane.normal);
			VectorAdd( tw->modelOrigin, intersection, intersection);
			trace.plane.dist = DotProduct(trace.plane.normal, intersection);
			trace.contents = CONTENTS_BODY;
		}
	}
	else if (d == 0) {
		//t1 = (- b ) / 2;
		// slide along the sphere
	}
	// no intersection at all
}

/*
================
CM_TraceThroughVerticalCylinder

get the first intersection of the ray with the cylinder
the cylinder extends halfheight above and below the origin
================
*/
void CM_TraceThroughVerticalCylinder( traceWork_t *tw, trace_t &trace, vec3_t origin, float radius, float halfheight, vec3_t start, vec3_t end) {
	float length, scale, fraction, l1, l2;
	float /*a, */b, c, d, sqrtd;
	vec3_t v1, dir, start2d, end2d, org2d, intersection;

	// 2d coordinates
	VectorSet(start2d, start[0], start[1], 0);
	VectorSet(end2d, end[0], end[1], 0);
	VectorSet(org2d, origin[0], origin[1], 0);
	// if between lower and upper cylinder bounds
	if (start[2] <= origin[2] + halfheight &&
				start[2] >= origin[2] - halfheight) {
		// if inside the cylinder
		VectorSubtract(start2d, org2d, dir);
		l1 = VectorLengthSquared(dir);
		if (l1 < Square(radius)) {
			trace.fraction = 0;
			trace.startsolid = qtrue;
			VectorSubtract(end2d, org2d, dir);
			l1 = VectorLengthSquared(dir);
			if (l1 < Square(radius)) {
				trace.allsolid = qtrue;
			}
			return;
		}
	}
	//
	VectorSubtract(end2d, start2d, dir);
	length = VectorNormalize(dir);
	//
	l1 = CM_DistanceFromLineSquared(org2d, start2d, end2d, dir);
	VectorSubtract(end2d, org2d, v1);
	l2 = VectorLengthSquared(v1);
	// if no intersection with the cylinder and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius+SURFACE_CLIP_EPSILON)) {
		return;
	}
	//
	//
	// (start[0] - origin[0] - t * dir[0]) ^ 2 + (start[1] - origin[1] - t * dir[1]) ^ 2 = radius ^ 2
	// (v1[0] + t * dir[0]) ^ 2 + (v1[1] + t * dir[1]) ^ 2 = radius ^ 2;
	// v1[0] ^ 2 + 2 * v1[0] * t * dir[0] + (t * dir[0]) ^ 2 +
	//						v1[1] ^ 2 + 2 * v1[1] * t * dir[1] + (t * dir[1]) ^ 2 = radius ^ 2
	// t ^ 2 * (dir[0] ^ 2 + dir[1] ^ 2) + t * (2 * v1[0] * dir[0] + 2 * v1[1] * dir[1]) +
	//						v1[0] ^ 2 + v1[1] ^ 2 - radius ^ 2 = 0
	//
	VectorSubtract(start, origin, v1);
	// dir is normalized so we can use a = 1
	//a = 1.0f;// * (dir[0] * dir[0] + dir[1] * dir[1]);
	b = 2.0f * (v1[0] * dir[0] + v1[1] * dir[1]);
	c = v1[0] * v1[0] + v1[1] * v1[1] - (radius+RADIUS_EPSILON) * (radius+RADIUS_EPSILON);

	d = b * b - 4.0f * c;// * a;
	if (d > 0) {
		sqrtd = sqrtf(d);
		// = (- b + sqrtd) * 0.5f;// / (2.0f * a);
		fraction = (- b - sqrtd) * 0.5f;// / (2.0f * a);
		//
		if (fraction < 0) {
			fraction = 0;
		}
		else {
			fraction /= length;
		}
		if ( fraction < trace.fraction ) {
			VectorSubtract(end, start, dir);
			VectorMA(start, fraction, dir, intersection);
			// if the intersection is between the cylinder lower and upper bound
			if (intersection[2] <= origin[2] + halfheight &&
						intersection[2] >= origin[2] - halfheight) {
				//
				trace.fraction = fraction;
				VectorSubtract(intersection, origin, dir);
				dir[2] = 0;
				#ifdef CAPSULE_DEBUG
					l2 = VectorLength(dir);
					if (l2 <= radius) {
						int bah = 1;
					}
				#endif
				scale = 1 / (radius+RADIUS_EPSILON);
				VectorScale(dir, scale, dir);
				VectorCopy(dir, trace.plane.normal);
				VectorAdd( tw->modelOrigin, intersection, intersection);
				trace.plane.dist = DotProduct(trace.plane.normal, intersection);
				trace.contents = CONTENTS_BODY;
			}
		}
	}
	else if (d == 0) {
		//t[0] = (- b ) / 2 * a;
		// slide along the cylinder
	}
	// no intersection at all
}

/*
================
CM_TraceCapsuleThroughCapsule

capsule vs. capsule collision (not rotated)
================
*/
void CM_TraceCapsuleThroughCapsule( traceWork_t *tw, trace_t &trace, clipHandle_t model ) {
	int i;
	vec3_t mins, maxs;
	vec3_t top, bottom, starttop, startbottom, endtop, endbottom;
	vec3_t offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, h;

	CM_ModelBounds(model, mins, maxs);
	// test trace bounds vs. capsule bounds
	if ( tw->bounds[0][0] > maxs[0] + RADIUS_EPSILON
		|| tw->bounds[0][1] > maxs[1] + RADIUS_EPSILON
		|| tw->bounds[0][2] > maxs[2] + RADIUS_EPSILON
		|| tw->bounds[1][0] < mins[0] - RADIUS_EPSILON
		|| tw->bounds[1][1] < mins[1] - RADIUS_EPSILON
		|| tw->bounds[1][2] < mins[2] - RADIUS_EPSILON
		) {
		return;
	}
	// top origin and bottom origin of each sphere at start and end of trace
	VectorAdd(tw->start, tw->sphere.offset, starttop);
	VectorSubtract(tw->start, tw->sphere.offset, startbottom);
	VectorAdd(tw->end, tw->sphere.offset, endtop);
	VectorSubtract(tw->end, tw->sphere.offset, endbottom);

	// calculate top and bottom of the capsule spheres to collide with
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];
	radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	offs = halfheight - radius;
	VectorCopy(offset, top);
	top[2] += offs;
	VectorCopy(offset, bottom);
	bottom[2] -= offs;
	// expand radius of spheres
	radius += tw->sphere.radius;
	// if there is horizontal movement
	if ( tw->start[0] != tw->end[0] || tw->start[1] != tw->end[1] ) {
		// height of the expanded cylinder is the height of both cylinders minus the radius of both spheres
		h = halfheight + tw->sphere.halfheight - radius;
		// if the cylinder has a height
		if ( h > 0 ) {
			// test for collisions between the cylinders
			CM_TraceThroughVerticalCylinder(tw, trace, offset, radius, h, tw->start, tw->end);
		}
	}
	// test for collision between the spheres
	CM_TraceThroughSphere(tw, trace, top, radius, startbottom, endbottom);
	CM_TraceThroughSphere(tw, trace, bottom, radius, starttop, endtop);
}

/*
================
CM_TraceBoundingBoxThroughCapsule

bounding box vs. capsule collision
================
*/
void CM_TraceBoundingBoxThroughCapsule( traceWork_t *tw, trace_t &trace, clipHandle_t model ) {
	vec3_t mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t *cmod;
	int i;

	// mins maxs of the capsule
	CM_ModelBounds(model, mins, maxs);

	// offset for capsule center
	for ( i = 0 ; i < 3 ; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1][0] > size[1][2] ) ? size[1][2]: size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet( tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius );

	// replace the capsule with the bounding box
	h = CM_TempBoxModel(tw->size[0], tw->size[1], qfalse);
	// calculate collision
	cmod = CM_ClipHandleToModel( h );
	CM_TraceThroughLeaf( tw, trace, &cmg, &cmod->leaf );
}

//=========================================================================================

/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf( traceWork_t *tw, trace_t &trace, cLeaf_t *leaf, clipMap_t *local )
{
	int			k;
	int			brushnum;
	cbrush_t	*b;
	cPatch_t	*patch;

	// trace line against all brushes in the leaf
	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ )
	{
		brushnum = local->leafbrushes[leaf->firstLeafBrush + k];

		b = &local->brushes[brushnum];
		if ( b->checkcount == local->checkcount )
		{
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = local->checkcount;

		if ( !(b->contents & tw->contents) )
		{
			continue;
		}

		CM_TraceThroughBrush( tw, trace, b, false);
		if ( !trace.fraction )
		{
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

			CM_TraceThroughPatch( tw, trace, patch );
			if ( !trace.fraction ) {
				return;
			}
		}
	}
}

/*
==================
CM_TraceThroughTree

Traverse all the contacted leafs from the start to the end position.
If the trace is a point, they will be exactly in order, but for larger
trace volumes it is possible to hit something in a later leaf with
a smaller intercept fraction.
==================
*/
void CM_TraceThroughTree( traceWork_t *tw, trace_t &trace, clipMap_t *local, int num, float p1f, float p2f, vec3_t p1, vec3_t p2) {
	cNode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace.fraction <= p1f) {
		return;		// already hit something nearer
	}

	// if < 0, we are in a leaf node
	if (num < 0) {
		CM_TraceThroughLeaf( tw, trace, local, &local->leafs[-1-num] );
		return;
	}

	//
	// find the point distances to the separating plane
	// and the offset for the size of the box
	//
	node = local->nodes + num;
	plane = node->plane;

	// adjust the plane distance appropriately for mins/maxs
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
#if 0 // bk010201 - DEAD
			// an axial brush right behind a slanted bsp plane
			// will poke through when expanded, so adjust
			// by sqrt(3)
			offset = fabs(tw->extents[0]*plane->normal[0]) +
				fabs(tw->extents[1]*plane->normal[1]) +
				fabs(tw->extents[2]*plane->normal[2]);

			offset *= 2;
			offset = tw->maxOffset;
#endif
			// this is silly
			offset = 2048;
		}
	}

	// see which sides we need to consider
	if ( t1 >= offset + 1 && t2 >= offset + 1 ) {
		CM_TraceThroughTree( tw, trace, local, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -offset - 1 && t2 < -offset - 1 ) {
		CM_TraceThroughTree( tw, trace, local, node->children[1], p1f, p2f, p1, p2 );
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

	CM_TraceThroughTree( tw, trace, local, node->children[side], p1f, midf, p1, mid );


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

	CM_TraceThroughTree( tw, trace, local, node->children[side^1], midf, p2f, mid, p2 );
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
CM_Trace
==================
*/
void CM_Trace( trace_t *trace, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, const vec3_t origin, int brushmask, int capsule, sphere_t *sphere ) {
	int			i;
	traceWork_t	tw;
	vec3_t		offset;
	cmodel_t	*cmod;
	clipMap_t	*local = 0;

	cmod = CM_ClipHandleToModel( model, &local );

	local->checkcount++;		// for multi-check avoidance

	c_traces++;				// for statistics, may be zeroed

	// fill in a default trace
	Com_Memset( &tw, 0, sizeof(tw) );
	memset(trace, 0, sizeof(*trace));
	trace->fraction = 1;	// assume it goes the entire distance until shown otherwise
	VectorCopy(origin, tw.modelOrigin);

	if (!local->numNodes) {
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

	// if a sphere is already specified
	if ( sphere ) {
		tw.sphere = *sphere;
	}
	else {
		tw.sphere.use = (qboolean)capsule;
		tw.sphere.radius = ( tw.size[1][0] > tw.size[1][2] ) ? tw.size[1][2]: tw.size[1][0];
		tw.sphere.halfheight = tw.size[1][2];
		VectorSet( tw.sphere.offset, 0, 0, tw.size[1][2] - tw.sphere.radius );
	}

	tw.maxOffset = tw.size[1][0] + tw.size[1][1] + tw.size[1][2];

	// tw.offsets[signbits] = vector to appropriately corner from origin
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
	if ( tw.sphere.use ) {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] - fabs(tw.sphere.offset[i]) - tw.sphere.radius;
				tw.bounds[1][i] = tw.end[i] + fabs(tw.sphere.offset[i]) + tw.sphere.radius;
			} else {
				tw.bounds[0][i] = tw.end[i] - fabs(tw.sphere.offset[i]) - tw.sphere.radius;
				tw.bounds[1][i] = tw.start[i] + fabs(tw.sphere.offset[i]) + tw.sphere.radius;
			}
		}
	}
	else {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] + tw.size[0][i];
				tw.bounds[1][i] = tw.end[i] + tw.size[1][i];
			} else {
				tw.bounds[0][i] = tw.end[i] + tw.size[0][i];
				tw.bounds[1][i] = tw.start[i] + tw.size[1][i];
			}
		}
	}

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2] &&
		tw.size[0][0] == 0 && tw.size[0][1] == 0 && tw.size[0][2] == 0)
	{
		if ( model && cmod->firstNode == -1)
		{
#ifdef ALWAYS_BBOX_VS_BBOX // bk010201 - FIXME - compile time flag?
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
			{
				tw.sphere.use = qfalse;
				CM_TestInLeaf( &tw, &cmod->leaf );
			}
			else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
			{
				CM_TestCapsuleInCapsule( &tw, model );
			}
			else
#endif
			if ( model == CAPSULE_MODEL_HANDLE )
			{
				if ( tw.sphere.use )
				{
					CM_TestCapsuleInCapsule( &tw, *trace, model );
				}
				else
				{
					CM_TestBoundingBoxInCapsule( &tw, *trace, model );
				}
			}
			else
			{
				CM_TestInLeaf( &tw, *trace, &cmod->leaf, local );
			}
		}
		else if (cmod->firstNode == -1)
		{
			CM_PositionTest( &tw, *trace );
		}
		else
		{
			CM_TraceThroughTree( &tw, *trace, local, cmod->firstNode, 0, 1, tw.start, tw.end );
		}
	}
	else
	{
		//
		// check for point special case
		//
		if ( tw.size[0][0] == 0 && tw.size[0][1] == 0 && tw.size[0][2] == 0 )
		{
			tw.isPoint = qtrue;
			VectorClear( tw.extents );
		}
		else
		{
			tw.isPoint = qfalse;
			tw.extents[0] = tw.size[1][0];
			tw.extents[1] = tw.size[1][1];
			tw.extents[2] = tw.size[1][2];
		}

		//
		// general sweeping through world
		//
		if ( model && cmod->firstNode == -1)
		{
#ifdef ALWAYS_BBOX_VS_BBOX
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
			{
				tw.sphere.use = qfalse;
				CM_TraceThroughLeaf( &tw, &cmod->leaf );
			}
			else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
			if ( model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
			{
				CM_TraceCapsuleThroughCapsule( &tw, model );
			}
			else
#endif
			if ( model == CAPSULE_MODEL_HANDLE )
			{
				if ( tw.sphere.use )
				{
					CM_TraceCapsuleThroughCapsule( &tw, *trace, model );
				}
				else
				{
					CM_TraceBoundingBoxThroughCapsule( &tw, *trace, model );
				}
			}
			else
			{
				CM_TraceThroughLeaf( &tw, *trace, local, &cmod->leaf );
			}
		}
		else
		{
			CM_TraceThroughTree( &tw, *trace, local, cmod->firstNode, 0, 1, tw.start, tw.end );
		}
	}

	// generate endpos from the original, unmodified start/end
	if ( trace->fraction == 1 ) {
		VectorCopy (end, trace->endpos);
	} else {
		for ( i=0 ; i<3 ; i++ ) {
			trace->endpos[i] = start[i] + trace->fraction * (end[i] - start[i]);
		}
	}

        // If allsolid is set (was entirely inside something solid), the plane is not valid.
        // If fraction == 1.0, we never hit anything, and thus the plane is not valid.
        // Otherwise, the normal on the plane should have unit length
        assert(trace->allsolid ||
               trace->fraction == 1.0 ||
               VectorLengthSquared(trace->plane.normal) > 0.9999);
}

/*
==================
CM_BoxTrace
==================
*/
void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask, int capsule ) {
	CM_Trace( results, start, end, mins, maxs, model, vec3_origin, brushmask, capsule, NULL );
}

/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
void CM_TransformedBoxTrace( trace_t *trace, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask,
						  const vec3_t origin, const vec3_t angles, int capsule ) {
	vec3_t		start_l, end_l;
	qboolean	rotated;
	vec3_t		offset;
	vec3_t		symetricSize[2];
	matrix3_t	matrix, transpose;
	int			i;
	float		halfwidth;
	float		halfheight;
	float		t;
	sphere_t	sphere;

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

	halfwidth = symetricSize[ 1 ][ 0 ];
	halfheight = symetricSize[ 1 ][ 2 ];

	sphere.use = (qboolean)capsule;
	sphere.radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	sphere.halfheight = halfheight;
	t = halfheight - sphere.radius;

	if (rotated) {
		// rotation on trace line (start-end) instead of rotating the bmodel
		// NOTE: This is still incorrect for bounding boxes because the actual bounding
		//		 box that is swept through the model is not rotated. We cannot rotate
		//		 the bounding box or the bmodel because that would make all the brush
		//		 bevels invalid.
		//		 However this is correct for capsules since a capsule itself is rotated too.
		CreateRotationMatrix(angles, matrix);
		RotatePoint(start_l, matrix);
		RotatePoint(end_l, matrix);
		// rotated sphere offset for capsule
		sphere.offset[0] = matrix[0][ 2 ] * t;
		sphere.offset[1] = -matrix[1][ 2 ] * t;
		sphere.offset[2] = matrix[2][ 2 ] * t;
	}
	else {
		VectorSet( sphere.offset, 0, 0, t );
	}

	// sweep the box through the model
	CM_Trace( trace, start_l, end_l, symetricSize[0], symetricSize[1], model, origin, brushmask, capsule, &sphere );

	// if the bmodel was rotated and there was a collision
	if ( rotated && trace->fraction != 1.0 ) {
		// rotation of bmodel collision plane
		TransposeMatrix(matrix, transpose);
		RotatePoint(trace->plane.normal, transpose);
	}

	// re-calculate the end position of the trace because the trace.endpos
	// calculated by CM_Trace could be rotated and have an offset
	trace->endpos[0] = start[0] + trace->fraction * (end[0] - start[0]);
	trace->endpos[1] = start[1] + trace->fraction * (end[1] - start[1]);
	trace->endpos[2] = start[2] + trace->fraction * (end[2] - start[2]);
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

	return(CM_CullBox(frustum, transformed));
}
