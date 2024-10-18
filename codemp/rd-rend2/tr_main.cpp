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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"
#include "tr_weather.h"

#include <string.h> // memcpy

#include "ghoul2/g2_local.h"

trGlobals_t		tr;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


refimport_t	ri;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

/*
================
R_CompareVert
================
*/
qboolean R_CompareVert(srfVert_t * v1, srfVert_t * v2, qboolean checkST)
{
	int             i;

	for (i = 0; i < 3; i++)
	{
		if (floor(v1->xyz[i] + 0.1) != floor(v2->xyz[i] + 0.1))
		{
			return qfalse;
		}

		if (checkST && ((v1->st[0] != v2->st[0]) || (v1->st[1] != v2->st[1])))
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
=============
R_CalcNormalForTriangle
=============
*/
void R_CalcNormalForTriangle(vec3_t normal, const vec3_t v0, const vec3_t v1, const vec3_t v2)
{
	vec3_t          udir, vdir;

	// compute the face normal based on vertex points
	VectorSubtract(v2, v0, udir);
	VectorSubtract(v1, v0, vdir);
	CrossProduct(udir, vdir, normal);

	VectorNormalize(normal);
}

/*
=============
R_CalcTangentsForTriangle
http://members.rogers.com/deseric/tangentspace.htm
=============
*/
void R_CalcTangentsForTriangle(vec3_t tangent, vec3_t bitangent,
							   const vec3_t v0, const vec3_t v1, const vec3_t v2,
							   const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	int             i;
	vec3_t          planes[3];
	vec3_t          u, v;

	for(i = 0; i < 3; i++)
	{
		VectorSet(u, v1[i] - v0[i], t1[0] - t0[0], t1[1] - t0[1]);
		VectorSet(v, v2[i] - v0[i], t2[0] - t0[0], t2[1] - t0[1]);

		VectorNormalize(u);
		VectorNormalize(v);

		CrossProduct(u, v, planes[i]);
	}

	//So your tangent space will be defined by this :
	//Normal = Normal of the triangle or Tangent X Bitangent (careful with the cross product,
	// you have to make sure the normal points in the right direction)
	//Tangent = ( dp(Fx(s,t)) / ds,  dp(Fy(s,t)) / ds, dp(Fz(s,t)) / ds )   or     ( -Bx/Ax, -By/Ay, - Bz/Az )
	//Bitangent =  ( dp(Fx(s,t)) / dt,  dp(Fy(s,t)) / dt, dp(Fz(s,t)) / dt )  or     ( -Cx/Ax, -Cy/Ay, -Cz/Az )

	// tangent...
	tangent[0] = -planes[0][1] / planes[0][0];
	tangent[1] = -planes[1][1] / planes[1][0];
	tangent[2] = -planes[2][1] / planes[2][0];
	VectorNormalize(tangent);

	// bitangent...
	bitangent[0] = -planes[0][2] / planes[0][0];
	bitangent[1] = -planes[1][2] / planes[1][0];
	bitangent[2] = -planes[2][2] / planes[2][0];
	VectorNormalize(bitangent);
}




/*
=============
R_CalcTangentSpace
=============
*/
void R_CalcTangentSpace(vec3_t tangent, vec3_t bitangent, vec3_t normal,
						const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	vec3_t          cp, u, v;
	vec3_t          faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[0] = -cp[1] / cp[0];
		bitangent[0] = -cp[2] / cp[0];
	}

	u[0] = v1[1] - v0[1];
	v[0] = v2[1] - v0[1];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[1] = -cp[1] / cp[0];
		bitangent[1] = -cp[2] / cp[0];
	}

	u[0] = v1[2] - v0[2];
	v[0] = v2[2] - v0[2];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[2] = -cp[1] / cp[0];
		bitangent[2] = -cp[2] / cp[0];
	}

	VectorNormalize(tangent);
	VectorNormalize(bitangent);

	// compute the face normal based on vertex points
	if ( normal[0] == 0.0f && normal[1] == 0.0f && normal[2] == 0.0f )
	{
		VectorSubtract(v2, v0, u);
		VectorSubtract(v1, v0, v);
		CrossProduct(u, v, faceNormal);
	}
	else
	{
		VectorCopy(normal, faceNormal);
	}

	VectorNormalize(faceNormal);

#if 1
	// Gram-Schmidt orthogonalize
	//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA(tangent, -DotProduct(faceNormal, tangent), faceNormal, tangent);
	VectorNormalize(tangent);

	// compute the cross product B=NxT
	//CrossProduct(normal, tangent, bitangent);
#else
	// normal, compute the cross product N=TxB
	CrossProduct(tangent, bitangent, normal);
	VectorNormalize(normal);

	if(DotProduct(normal, faceNormal) < 0)
	{
		//VectorInverse(normal);
		//VectorInverse(tangent);
		//VectorInverse(bitangent);

		// compute the cross product T=BxN
		CrossProduct(bitangent, faceNormal, tangent);

		// compute the cross product B=NxT
		//CrossProduct(normal, tangent, bitangent);
	}
#endif

	VectorCopy(faceNormal, normal);
}

void R_CalcTangentSpaceFast(vec3_t tangent, vec3_t bitangent, vec3_t normal,
						const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2)
{
	vec3_t          cp, u, v;
	vec3_t          faceNormal;

	VectorSet(u, v1[0] - v0[0], t1[0] - t0[0], t1[1] - t0[1]);
	VectorSet(v, v2[0] - v0[0], t2[0] - t0[0], t2[1] - t0[1]);

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[0] = -cp[1] / cp[0];
		bitangent[0] = -cp[2] / cp[0];
	}

	u[0] = v1[1] - v0[1];
	v[0] = v2[1] - v0[1];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[1] = -cp[1] / cp[0];
		bitangent[1] = -cp[2] / cp[0];
	}

	u[0] = v1[2] - v0[2];
	v[0] = v2[2] - v0[2];

	CrossProduct(u, v, cp);
	if(fabs(cp[0]) > 10e-6)
	{
		tangent[2] = -cp[1] / cp[0];
		bitangent[2] = -cp[2] / cp[0];
	}

	VectorNormalizeFast(tangent);
	VectorNormalizeFast(bitangent);

	// compute the face normal based on vertex points
	VectorSubtract(v2, v0, u);
	VectorSubtract(v1, v0, v);
	CrossProduct(u, v, faceNormal);

	VectorNormalizeFast(faceNormal);

#if 0
	// normal, compute the cross product N=TxB
	CrossProduct(tangent, bitangent, normal);
	VectorNormalizeFast(normal);

	if(DotProduct(normal, faceNormal) < 0)
	{
		VectorInverse(normal);
		//VectorInverse(tangent);
		//VectorInverse(bitangent);

		CrossProduct(normal, tangent, bitangent);
	}

	VectorCopy(faceNormal, normal);
#else
	// Gram-Schmidt orthogonalize
		//tangent[a] = (t - n * Dot(n, t)).Normalize();
	VectorMA(tangent, -DotProduct(faceNormal, tangent), faceNormal, tangent);
	VectorNormalizeFast(tangent);
#endif

	VectorCopy(faceNormal, normal);
}

/*
http://www.terathon.com/code/tangent.html
*/
void R_CalcTexDirs(vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2,
				   const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3)
{
	float			x1, x2, y1, y2, z1, z2;
	float			s1, s2, t1, t2, r;

	x1 = v2[0] - v1[0];
	x2 = v3[0] - v1[0];
	y1 = v2[1] - v1[1];
	y2 = v3[1] - v1[1];
	z1 = v2[2] - v1[2];
	z2 = v3[2] - v1[2];

	s1 = w2[0] - w1[0];
	s2 = w3[0] - w1[0];
	t1 = w2[1] - w1[1];
	t2 = w3[1] - w1[1];

	r = 1.0f / (s1 * t2 - s2 * t1);

	VectorSet(sdir, (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
	VectorSet(tdir, (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
}

void R_CalcTbnFromNormalAndTexDirs(vec3_t tangent, vec3_t bitangent, vec3_t normal, vec3_t sdir, vec3_t tdir)
{
	vec3_t n_cross_t;
	float n_dot_t, handedness;

	// Gram-Schmidt orthogonalize
	n_dot_t = DotProduct(normal, sdir);
	VectorMA(sdir, -n_dot_t, normal, tangent);
	VectorNormalize(tangent);

	// Calculate handedness
	CrossProduct(normal, sdir, n_cross_t);
	handedness = (DotProduct(n_cross_t, tdir) < 0.0f) ? -1.0f : 1.0f;

	// Calculate bitangent
	CrossProduct(normal, tangent, bitangent);
	VectorScale(bitangent, handedness, bitangent);
}

qboolean R_CalcTangentVectors(srfVert_t * dv[3])
{
	int             i;
	float           bb, s, t;
	vec3_t          bary;


	/* calculate barycentric basis for the triangle */
	bb = (dv[1]->st[0] - dv[0]->st[0]) * (dv[2]->st[1] - dv[0]->st[1]) - (dv[2]->st[0] - dv[0]->st[0]) * (dv[1]->st[1] - dv[0]->st[1]);
	if(fabs(bb) < 0.00000001f)
		return qfalse;

	/* do each vertex */
	for(i = 0; i < 3; i++)
	{
		vec3_t bitangent, nxt;

		// calculate s tangent vector
		s = dv[i]->st[0] + 10.0f;
		t = dv[i]->st[1];
		bary[0] = ((dv[1]->st[0] - s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] = ((dv[2]->st[0] - s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] = ((dv[0]->st[0] - s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		dv[i]->tangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		dv[i]->tangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		dv[i]->tangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(dv[i]->tangent, dv[i]->xyz, dv[i]->tangent);
		VectorNormalize(dv[i]->tangent);

		// calculate t tangent vector
		s = dv[i]->st[0];
		t = dv[i]->st[1] + 10.0f;
		bary[0] = ((dv[1]->st[0] - s) * (dv[2]->st[1] - t) - (dv[2]->st[0] - s) * (dv[1]->st[1] - t)) / bb;
		bary[1] = ((dv[2]->st[0] - s) * (dv[0]->st[1] - t) - (dv[0]->st[0] - s) * (dv[2]->st[1] - t)) / bb;
		bary[2] = ((dv[0]->st[0] - s) * (dv[1]->st[1] - t) - (dv[1]->st[0] - s) * (dv[0]->st[1] - t)) / bb;

		bitangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		bitangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		bitangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(bitangent, dv[i]->xyz, bitangent);
		VectorNormalize(bitangent);

		// store bitangent handedness
		CrossProduct(dv[i]->normal, dv[i]->tangent, nxt);
		dv[i]->tangent[3] = (DotProduct(nxt, bitangent) < 0.0f) ? -1.0f : 1.0f;

		// debug code
		//% Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%     stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}

	return qtrue;
}

/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLocalBox(vec3_t localBounds[2]) {
#if 0
	int		i, j;
	vec3_t	transformed[8];
	float	dists[8];
	vec3_t	v;
	cplane_t	*frust;
	int			anyBack;
	int			front, back;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// transform into world space
	for (i = 0 ; i < 8 ; i++) {
		v[0] = bounds[i&1][0];
		v[1] = bounds[(i>>1)&1][1];
		v[2] = bounds[(i>>2)&1][2];

		VectorCopy( tr.ori.origin, transformed[i] );
		VectorMA( transformed[i], v[0], tr.ori.axis[0], transformed[i] );
		VectorMA( transformed[i], v[1], tr.ori.axis[1], transformed[i] );
		VectorMA( transformed[i], v[2], tr.ori.axis[2], transformed[i] );
	}

	// check against frustum planes
	anyBack = 0;
	for (i = 0 ; i < 4 ; i++) {
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for (j = 0 ; j < 8 ; j++) {
			dists[j] = DotProduct(transformed[j], frust->normal);
			if ( dists[j] > frust->dist ) {
				front = 1;
				if ( back ) {
					break;		// a point is in front
				}
			} else {
				back = 1;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
#else
	int             j;
	vec3_t          transformed;
	vec3_t          v;
	vec3_t          worldBounds[2];

	if(r_nocull->integer)
	{
		return CULL_CLIP;
	}

	// transform into world space
	ClearBounds(worldBounds[0], worldBounds[1]);

	for(j = 0; j < 8; j++)
	{
		v[0] = localBounds[j & 1][0];
		v[1] = localBounds[(j >> 1) & 1][1];
		v[2] = localBounds[(j >> 2) & 1][2];

		R_LocalPointToWorld(v, transformed);

		AddPointToBounds(transformed, worldBounds[0], worldBounds[1]);
	}

	return R_CullBox(worldBounds);
#endif
}

/*
=================
R_CullBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullBox(vec3_t worldBounds[2]) {
	int             i;
	cplane_t       *frust;
	qboolean        anyClip;
	int             r, numPlanes;

	numPlanes = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4;

	// check against frustum planes
	anyClip = qfalse;
	for(i = 0; i < numPlanes; i++)
	{
		frust = &tr.viewParms.frustum[i];

		r = BoxOnPlaneSide(worldBounds[0], worldBounds[1], frust);

		if(r == 2)
		{
			// completely outside frustum
			return CULL_OUT;
		}
		if(r == 3)
		{
			anyClip = qtrue;
		}
	}

	if(!anyClip)
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}

/*
** R_CullLocalPointAndRadius
*/
int R_CullLocalPointAndRadius( const vec3_t pt, float radius )
{
	vec3_t transformed;

	R_LocalPointToWorld( pt, transformed );

	return R_CullPointAndRadius( transformed, radius );
}

/*
** R_CullPointAndRadius
*/
int R_CullPointAndRadiusEx( const vec3_t pt, float radius, const cplane_t* frustum, int numPlanes )
{
	int		i;
	float	dist;
	const cplane_t	*frust;
	qboolean mightBeClipped = qfalse;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// check against frustum planes
	for (i = 0 ; i < numPlanes ; i++)
	{
		frust = &frustum[i];

		dist = DotProduct( pt, frust->normal) - frust->dist;
		if ( dist < -radius )
		{
			return CULL_OUT;
		}
		else if ( dist <= radius )
		{
			mightBeClipped = qtrue;
		}
	}

	if ( mightBeClipped )
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}

/*
** R_CullPointAndRadius
*/
int R_CullPointAndRadius( const vec3_t pt, float radius )
{
	return R_CullPointAndRadiusEx(pt, radius, tr.viewParms.frustum, (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4);
}

/*
=================
R_LocalNormalToWorld

=================
*/
void R_LocalNormalToWorld (const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.ori.axis[0][0] + local[1] * tr.ori.axis[1][0] + local[2] * tr.ori.axis[2][0];
	world[1] = local[0] * tr.ori.axis[0][1] + local[1] * tr.ori.axis[1][1] + local[2] * tr.ori.axis[2][1];
	world[2] = local[0] * tr.ori.axis[0][2] + local[1] * tr.ori.axis[1][2] + local[2] * tr.ori.axis[2][2];
}

/*
=================
R_LocalPointToWorld

=================
*/
void R_LocalPointToWorld (const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.ori.axis[0][0] + local[1] * tr.ori.axis[1][0] + local[2] * tr.ori.axis[2][0] + tr.ori.origin[0];
	world[1] = local[0] * tr.ori.axis[0][1] + local[1] * tr.ori.axis[1][1] + local[2] * tr.ori.axis[2][1] + tr.ori.origin[1];
	world[2] = local[0] * tr.ori.axis[0][2] + local[1] * tr.ori.axis[1][2] + local[2] * tr.ori.axis[2][2] + tr.ori.origin[2];
}

/*
=================
R_WorldToLocal

=================
*/
void R_WorldToLocal (const vec3_t world, vec3_t local) {
	local[0] = DotProduct(world, tr.ori.axis[0]);
	local[1] = DotProduct(world, tr.ori.axis[1]);
	local[2] = DotProduct(world, tr.ori.axis[2]);
}

/*
==========================
R_TransformModelToClip

==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelViewMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] =
			src[0] * modelViewMatrix[ i + 0 * 4 ] +
			src[1] * modelViewMatrix[ i + 1 * 4 ] +
			src[2] * modelViewMatrix[ i + 2 * 4 ] +
			1 * modelViewMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		dst[i] =
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
R_TransformClipToWindow

==========================
*/
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}


/*
==========================
myGlMultMatrix

==========================
*/
void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms,
					   orientationr_t *ori )
{
	vec3_t	delta;
	float	axisLength;

	if ( ent->e.reType != RT_MODEL || ent == &tr.worldEntity ) {
		*ori = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, ori->origin );

	VectorCopy( ent->e.axis[0], ori->axis[0] );
	VectorCopy( ent->e.axis[1], ori->axis[1] );
	VectorCopy( ent->e.axis[2], ori->axis[2] );

	ori->modelMatrix[0] = ori->axis[0][0];
	ori->modelMatrix[4] = ori->axis[1][0];
	ori->modelMatrix[8] = ori->axis[2][0];
	ori->modelMatrix[12] = ori->origin[0];

	ori->modelMatrix[1] = ori->axis[0][1];
	ori->modelMatrix[5] = ori->axis[1][1];
	ori->modelMatrix[9] = ori->axis[2][1];
	ori->modelMatrix[13] = ori->origin[1];

	ori->modelMatrix[2] = ori->axis[0][2];
	ori->modelMatrix[6] = ori->axis[1][2];
	ori->modelMatrix[10] = ori->axis[2][2];
	ori->modelMatrix[14] = ori->origin[2];

	ori->modelMatrix[3] = 0;
	ori->modelMatrix[7] = 0;
	ori->modelMatrix[11] = 0;
	ori->modelMatrix[15] = 1;

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->ori.origin, ori->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	} else {
		axisLength = 1.0f;
	}

	ori->viewOrigin[0] = DotProduct( delta, ori->axis[0] ) * axisLength;
	ori->viewOrigin[1] = DotProduct( delta, ori->axis[1] ) * axisLength;
	ori->viewOrigin[2] = DotProduct( delta, ori->axis[2] ) * axisLength;
}

/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
static void R_RotateForViewer(orientationr_t *ori, viewParms_t *viewParms)
{
	float	viewerMatrix[16];
	vec3_t	origin;

	*ori = {};
	ori->axis[0][0] = 1.0f;
	ori->axis[1][1] = 1.0f;
	ori->axis[2][2] = 1.0f;
	VectorCopy(viewParms->ori.origin, ori->viewOrigin);

	// transform by the camera placement
	VectorCopy(viewParms->ori.origin, origin);

	viewerMatrix[0] = viewParms->ori.axis[0][0];
	viewerMatrix[4] = viewParms->ori.axis[0][1];
	viewerMatrix[8] = viewParms->ori.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = viewParms->ori.axis[1][0];
	viewerMatrix[5] = viewParms->ori.axis[1][1];
	viewerMatrix[9] = viewParms->ori.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = viewParms->ori.axis[2][0];
	viewerMatrix[6] = viewParms->ori.axis[2][1];
	viewerMatrix[10] = viewParms->ori.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix(viewerMatrix, s_flipMatrix, ori->modelViewMatrix);
	Matrix16Identity(ori->modelMatrix);
}

/*
** SetFarClip
*/
static void R_SetFarClip( viewParms_t *viewParms, const trRefdef_t *refdef )
{
	float	farthestCornerDistance = 0;
	int		i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if (refdef != NULL)
	{
		if (refdef->rdflags & RDF_NOWORLDMODEL) {
			// override the zfar then
			if (refdef->rdflags & RDF_AUTOMAP)
				viewParms->zFar = 32768.0f;
			else
				viewParms->zFar = 2048.0f;
			return;
		}
	}

	//
	// set far clipping planes dynamically
	//
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;
		float distance;

		if ( i & 1 )
		{
			v[0] = viewParms->visBounds[0][0];
		}
		else
		{
			v[0] = viewParms->visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = viewParms->visBounds[0][1];
		}
		else
		{
			v[1] = viewParms->visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = viewParms->visBounds[0][2];
		}
		else
		{
			v[2] = viewParms->visBounds[1][2];
		}

		distance = DistanceSquared( viewParms->ori.origin, v );

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}

	// Bring in the zFar to the distanceCull distance
	// The sky renders at zFar so need to move it out a little
	// ...and make sure there is a minimum zfar to prevent problems
	viewParms->zFar = Com_Clamp(2048.0f, tr.distanceCull * (1.732), sqrtf( farthestCornerDistance ));
}

/*
=================
R_SetupFrustum

Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
the projection matrix.
=================
*/
void R_SetupFrustum (viewParms_t *dest, float xmin, float xmax, float ymax, float zProj, float zFar, float stereoSep)
{
	vec3_t ofsorigin;
	float oppleg, adjleg, length;
	int i;

	if(stereoSep == 0 && xmin == -xmax)
	{
		// symmetric case can be simplified
		VectorCopy(dest->ori.origin, ofsorigin);

		length = sqrt(xmax * xmax + zProj * zProj);
		oppleg = xmax / length;
		adjleg = zProj / length;

		VectorScale(dest->ori.axis[0], oppleg, dest->frustum[0].normal);
		VectorMA(dest->frustum[0].normal, adjleg, dest->ori.axis[1], dest->frustum[0].normal);

		VectorScale(dest->ori.axis[0], oppleg, dest->frustum[1].normal);
		VectorMA(dest->frustum[1].normal, -adjleg, dest->ori.axis[1], dest->frustum[1].normal);
	}
	else
	{
		// In stereo rendering, due to the modification of the projection matrix, dest->ori.origin is not the
		// actual origin that we're rendering so offset the tip of the view pyramid.
		VectorMA(dest->ori.origin, stereoSep, dest->ori.axis[1], ofsorigin);

		oppleg = xmax + stereoSep;
		length = sqrt(oppleg * oppleg + zProj * zProj);
		VectorScale(dest->ori.axis[0], oppleg / length, dest->frustum[0].normal);
		VectorMA(dest->frustum[0].normal, zProj / length, dest->ori.axis[1], dest->frustum[0].normal);

		oppleg = xmin + stereoSep;
		length = sqrt(oppleg * oppleg + zProj * zProj);
		VectorScale(dest->ori.axis[0], -oppleg / length, dest->frustum[1].normal);
		VectorMA(dest->frustum[1].normal, -zProj / length, dest->ori.axis[1], dest->frustum[1].normal);
	}

	length = sqrt(ymax * ymax + zProj * zProj);
	oppleg = ymax / length;
	adjleg = zProj / length;

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[2].normal);
	VectorMA(dest->frustum[2].normal, adjleg, dest->ori.axis[2], dest->frustum[2].normal);

	VectorScale(dest->ori.axis[0], oppleg, dest->frustum[3].normal);
	VectorMA(dest->frustum[3].normal, -adjleg, dest->ori.axis[2], dest->frustum[3].normal);

	for (i=0 ; i<4 ; i++) {
		dest->frustum[i].type = PLANE_NON_AXIAL;
		dest->frustum[i].dist = DotProduct (ofsorigin, dest->frustum[i].normal);
		SetPlaneSignbits( &dest->frustum[i] );
	}

	if (zFar != 0.0f)
	{
		vec3_t farpoint;

		VectorMA(ofsorigin, zFar, dest->ori.axis[0], farpoint);
		VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);

		dest->frustum[4].type = PLANE_NON_AXIAL;
		dest->frustum[4].dist = DotProduct (farpoint, dest->frustum[4].normal);
		SetPlaneSignbits( &dest->frustum[4] );
		dest->flags |= VPF_FARPLANEFRUSTUM;
	}
}

/*
===============
R_SetupProjection
===============
*/
void R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum)
{
	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = r_stereoSeparation->value;

	/*
	 * offset the view origin of the viewer for stereo rendering
	 * by setting the projection matrix appropriately.
	 */

	if(stereoSep != 0)
	{
		if(dest->stereoFrame == STEREO_LEFT)
			stereoSep = zProj / stereoSep;
		else if(dest->stereoFrame == STEREO_RIGHT)
			stereoSep = zProj / -stereoSep;
		else
			stereoSep = 0;
	}

	ymax = zProj * tan(dest->fovY * M_PI / 360.0f);
	ymin = -ymax;

	xmax = zProj * tan(dest->fovX * M_PI / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	dest->projectionMatrix[0] = 2 * zProj / width;
	dest->projectionMatrix[4] = 0;
	dest->projectionMatrix[8] = (xmax + xmin + 2 * stereoSep) / width;
	dest->projectionMatrix[12] = 2 * zProj * stereoSep / width;

	dest->projectionMatrix[1] = 0;
	dest->projectionMatrix[5] = 2 * zProj / height;
	dest->projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	dest->projectionMatrix[13] = 0;

	dest->projectionMatrix[3] = 0;
	dest->projectionMatrix[7] = 0;
	dest->projectionMatrix[11] = -1;
	dest->projectionMatrix[15] = 0;

	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	if(computeFrustum)
		R_SetupFrustum(dest, xmin, xmax, ymax, zProj, zFar, stereoSep);
}

/*
===============
R_SetupProjectionZ

Sets the z-component transformation part in the projection matrix
===============
*/
void R_SetupProjectionZ(viewParms_t *dest)
{
	float zNear, zFar, depth;

	zNear   = dest->zNear;
	zFar	= dest->zFar;

	depth	= zFar - zNear;

	dest->projectionMatrix[2] = 0;
	dest->projectionMatrix[6] = 0;
	dest->projectionMatrix[10] = -( zFar + zNear ) / depth;
	dest->projectionMatrix[14] = -2 * zFar * zNear / depth;

	if (dest->isPortal)
	{
		float	plane[4];
		float	plane2[4];
		vec4_t q, c;

		// transform portal plane into camera space
		plane[0] = dest->portalPlane.normal[0];
		plane[1] = dest->portalPlane.normal[1];
		plane[2] = dest->portalPlane.normal[2];
		plane[3] = dest->portalPlane.dist;

		plane2[0] = -DotProduct (dest->ori.axis[1], plane);
		plane2[1] = DotProduct (dest->ori.axis[2], plane);
		plane2[2] = -DotProduct (dest->ori.axis[0], plane);
		plane2[3] = DotProduct (plane, dest->ori.origin) - plane[3];

		// Lengyel, Eric. "Modifying the Projection Matrix to Perform Oblique Near-plane Clipping".
		// Terathon Software 3D Graphics Library, 2004. http://www.terathon.com/code/oblique.html
		q[0] = (SGN(plane2[0]) + dest->projectionMatrix[8]) / dest->projectionMatrix[0];
		q[1] = (SGN(plane2[1]) + dest->projectionMatrix[9]) / dest->projectionMatrix[5];
		q[2] = -1.0f;
		q[3] = (1.0f + dest->projectionMatrix[10]) / dest->projectionMatrix[14];

		VectorScale4(plane2, 2.0f / DotProduct4(plane2, q), c);

		dest->projectionMatrix[2]  = c[0];
		dest->projectionMatrix[6]  = c[1];
		dest->projectionMatrix[10] = c[2] + 1.0f;
		dest->projectionMatrix[14] = c[3];
	}
}

/*
===============
R_SetupProjectionOrtho
===============
*/
void R_SetupProjectionOrtho(viewParms_t *dest, const vec3_t viewBounds[2])
{
	float xmin, xmax, ymin, ymax, znear, zfar;
	//viewParms_t *dest = &tr.viewParms;
	int i;
	vec3_t pop;

	// Quake3:   Projection:
	//
	//    Z  X   Y  Z
	//    | /    | /
	//    |/     |/
	// Y--+      +--X

	xmin  =  viewBounds[0][1];
	xmax  =  viewBounds[1][1];
	ymin  = -viewBounds[1][2];
	ymax  = -viewBounds[0][2];
	znear =  viewBounds[0][0];
	zfar  =  viewBounds[1][0];

	dest->projectionMatrix[0]  = 2 / (xmax - xmin);
	dest->projectionMatrix[4]  = 0;
	dest->projectionMatrix[8]  = 0;
	dest->projectionMatrix[12] = (xmax + xmin) / (xmax - xmin);

	dest->projectionMatrix[1]  = 0;
	dest->projectionMatrix[5]  = 2 / (ymax - ymin);
	dest->projectionMatrix[9]  = 0;
	dest->projectionMatrix[13] = (ymax + ymin) / (ymax - ymin);

	dest->projectionMatrix[2]  = 0;
	dest->projectionMatrix[6]  = 0;
	dest->projectionMatrix[10] = -2 / (zfar - znear);
	dest->projectionMatrix[14] = -(zfar + znear) / (zfar - znear);

	dest->projectionMatrix[3]  = 0;
	dest->projectionMatrix[7]  = 0;
	dest->projectionMatrix[11] = 0;
	dest->projectionMatrix[15] = 1;

	VectorScale(dest->ori.axis[1],  1.0f, dest->frustum[0].normal);
	VectorMA(dest->ori.origin, viewBounds[0][1], dest->frustum[0].normal, pop);
	dest->frustum[0].dist = DotProduct(pop, dest->frustum[0].normal);

	VectorScale(dest->ori.axis[1], -1.0f, dest->frustum[1].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][1], dest->frustum[1].normal, pop);
	dest->frustum[1].dist = DotProduct(pop, dest->frustum[1].normal);

	VectorScale(dest->ori.axis[2],  1.0f, dest->frustum[2].normal);
	VectorMA(dest->ori.origin, viewBounds[0][2], dest->frustum[2].normal, pop);
	dest->frustum[2].dist = DotProduct(pop, dest->frustum[2].normal);

	VectorScale(dest->ori.axis[2], -1.0f, dest->frustum[3].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][2], dest->frustum[3].normal, pop);
	dest->frustum[3].dist = DotProduct(pop, dest->frustum[3].normal);

	VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);
	VectorMA(dest->ori.origin, -viewBounds[1][0], dest->frustum[4].normal, pop);
	dest->frustum[4].dist = DotProduct(pop, dest->frustum[4].normal);

	for (i = 0; i < 5; i++)
	{
		dest->frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits (&dest->frustum[i]);
	}

	dest->flags |= VPF_FARPLANEFRUSTUM;
}

/*
=================
R_MirrorPoint
=================
*/
void R_MirrorPoint (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	VectorSubtract( in, surface->origin, local );

	VectorClear( transformed );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(local, surface->axis[i]);
		VectorMA( transformed, d, camera->axis[i], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

void R_MirrorVector (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	float	d;

	VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(in, surface->axis[i]);
		VectorMA( out, d, camera->axis[i], out );
	}
}


/*
=============
R_PlaneForSurface
=============
*/
void R_PlaneForSurface (surfaceType_t *surfType, cplane_t *plane) {
	srfBspSurface_t	*tri;
	srfPoly_t		*poly;
	srfVert_t		*v1, *v2, *v3;
	vec4_t			plane4;

	if (!surfType) {
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType) {
	case SF_FACE:
		*plane = ((srfBspSurface_t *)surfType)->cullPlane;
		return;
	case SF_TRIANGLES:
		tri = (srfBspSurface_t *)surfType;
		v1 = tri->verts + tri->indexes[0];
		v2 = tri->verts + tri->indexes[1];
		v3 = tri->verts + tri->indexes[2];
		PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t *)surfType;
		PlaneFromPoints( plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[3];
		return;
	default:
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/
qboolean R_GetPortalOrientations(const msurface_t *surf, int entityNum,
							 orientation_t *surface, orientation_t *camera,
							 vec3_t pvsOrigin, qboolean *mirror ) {
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;
	vec3_t		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( surf->data, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD ) {
		const trRefEntity_t *currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( currentEntity, &tr.viewParms, &tr.ori );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.ori.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.ori.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
	PerpendicularVector( surface->axis[1], surface->axis[0] );
	CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) {
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] &&
			e->e.oldorigin[1] == e->e.origin[1] &&
			e->e.oldorigin[2] == e->e.origin[2] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );

		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = (tr.refdef.time/1000.0f) * e->e.frame;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		}
		else if ( e->e.skinNum ) {
			d = e->e.skinNum;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

static qboolean IsMirror( const msurface_t *surface, int entityNum )
{
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( surface->data, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != REFENTITYNUM_WORLD )
	{
		const trRefEntity_t *currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( currentEntity, &tr.viewParms, &tr.ori );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.ori.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.ori.origin );
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
	{
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] &&
			e->e.oldorigin[1] == e->e.origin[1] &&
			e->e.oldorigin[2] == e->e.origin[2] )
		{
			return qtrue;
		}

		return qfalse;
	}
	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const msurface_t *surface, int entityNum, vec4_t clipDest[128], int *numVertices ) {
	float shortest = 100000000;
	int numTriangles;
	vec4_t clip, eye;
	int i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	// TODO: Check if set properly here already
	//R_RotateForViewer(&tr.viewParms.world, &tr.viewParms);

	RB_BeginSurface(surface->shader, 0, 0 );
	rb_surfaceTable[ *surface->data ](surface->data);

	if ( tess.numVertexes > 128 )
	{
		// Don't bother trying, just assume it's off-screen and make it look bad. Besides, artists
		// shouldn't be using this many vertices on a mirror surface anyway :)
		return qtrue;
	}

	*numVertices = tess.numVertexes;

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip( tess.xyz[i], tr.viewParms.world.modelViewMatrix, tr.viewParms.projectionMatrix, eye, clip );
		VectorCopy4(clip, clipDest[i]);

		for ( j = 0; j < 3; j++ )
		{
			if ( clip[j] >= clip[3] )
			{
				pointFlags |= (1 << (j*2));
			}
			else if ( clip[j] <= -clip[3] )
			{
				pointFlags |= ( 1 << (j*2+1));
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd )
	{
		return qtrue;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	numTriangles = tess.numIndexes / 3;

	for ( i = 0; i < tess.numIndexes; i += 3 )
	{
		vec3_t normal, tNormal;

		float len;

		VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.ori.origin, normal );

		len = VectorLengthSquared( normal );			// lose the sqrt
		if ( len < shortest )
		{
			shortest = len;
		}

		R_VboUnpackNormal(tNormal, tess.normal[tess.indexes[i]]);

		if ( DotProduct( normal, tNormal ) >= 0 )
		{
			numTriangles--;
		}
	}
	if ( !numTriangles )
	{
		return qtrue;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( surface, entityNum) )
	{
		return qfalse;
	}

	if ( shortest > (tess.shader->portalRange*tess.shader->portalRange) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
========================
R_MirrorViewBySurface

Returns qtrue if another view has been rendered
========================
*/
qboolean R_MirrorViewBySurface (msurface_t *surface, int entityNum) {
	vec4_t			clipDest[128];
	int				numVertices;
	viewParms_t		newParms;
	viewParms_t		oldParms;
	orientation_t	surfaceOri, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) {
		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer || (r_fastsky->integer == 1) ) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen(surface, entityNum, clipDest, &numVertices ) ) {
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;
	newParms.zFar = 0.0f;
	newParms.zNear = r_znear->value;
	newParms.flags &= ~VPF_FARPLANEFRUSTUM;
	if ( !R_GetPortalOrientations(surface, entityNum, &surfaceOri, &camera,
		newParms.pvsOrigin, &newParms.isMirror ) ) {
		return qfalse;		// bad portal, no portalentity
	}

	if (newParms.isMirror)
		newParms.flags |= VPF_NOVIEWMODEL;

	// Calculate window coordinates of this surface to get tight fitting scissor rectangle
	int viewportWidth = oldParms.viewportWidth;
	int viewportHeight = oldParms.viewportHeight;
	float viewportCenterX = oldParms.viewportX + 0.5f * viewportWidth;
	float viewportCenterY = oldParms.viewportY + 0.5f * viewportHeight;

	/*int minRectX = INT_MAX;
	int minRectY = INT_MAX;
	int maxRectX = 0;
	int maxRectY = 0;

	for ( int i = 0; i < numVertices; i++ )
	{
		float ndcX;
		float ndcY;
		float wdcX;
		float wdcY;
		int floorX;
		int floorY;
		int ceilX;
		int ceilY;

		if ( clipDest[i][0] <= -clipDest[i][3] )
			ndcX = -1.0f;
		else if ( clipDest[i][0] >= clipDest[i][3] )
			ndcX = 1.0f;
		else
			ndcX = clipDest[i][0] / clipDest[i][3];

		if ( clipDest[i][1] <= -clipDest[i][3] )
			ndcY = -1.0f;
		else if ( clipDest[i][1] >= clipDest[i][3] )
			ndcY = 1.0f;
		else
			ndcY = clipDest[i][1] / clipDest[i][3];

		wdcX = 0.5f * viewportWidth * ndcX + viewportCenterX;
		wdcY = 0.5f * viewportHeight * ndcY + viewportCenterY;

		floorX = (int)wdcX;
		floorY = (int)wdcY;

		ceilX = (int)(wdcX + 0.5f);
		ceilY = (int)(wdcY + 0.5f);

		minRectX = Q_min(minRectX, floorX);
		minRectY = Q_min(minRectY, floorY);
		maxRectX = Q_max(maxRectX, ceilX);
		maxRectY = Q_max(maxRectY, ceilY);
	}

	minRectX = Q_max(minRectX, 0);
	minRectY = Q_max(minRectY, 0);
	maxRectX = Q_min(maxRectX, oldParms.viewportX + viewportWidth);
	maxRectY = Q_min(maxRectY, oldParms.viewportY + viewportHeight);

	newParms.scissorX = minRectX;
	newParms.scissorY = minRectY;
	newParms.scissorWidth = maxRectX - minRectX;
	newParms.scissorHeight = maxRectY - minRectY;*/

	R_MirrorPoint (oldParms.ori.origin, &surfaceOri, &camera, newParms.ori.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );

	R_MirrorVector (oldParms.ori.axis[0], &surfaceOri, &camera, newParms.ori.axis[0]);
	R_MirrorVector (oldParms.ori.axis[1], &surfaceOri, &camera, newParms.ori.axis[1]);
	R_MirrorVector (oldParms.ori.axis[2], &surfaceOri, &camera, newParms.ori.axis[2]);

	// OPTIMIZE further: restrict the viewport and set up the view and projection
	// matrices so they only draw into the tightest screen-space aligned box required
	// to fill the restricted viewport.

	tr.viewCount++;
	tr.viewParms = newParms;
	R_RotateForViewer(&tr.viewParms.world, &tr.viewParms);
	R_SetupProjection(&tr.viewParms, tr.viewParms.zNear, tr.viewParms.zFar, qtrue);

	R_MarkLeaves();

	// clear out the visible min/max
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);
	int planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;
	// set vis bounds
	R_RecursiveWorldNode(tr.world->nodes, planeBits, 0, 0);
	R_SetFarClip(&tr.viewParms, nullptr);
	R_SetupProjectionZ(&tr.viewParms);

	// render the mirror view
	tr.viewParms.currentViewParm = tr.numCachedViewParms;
	tr.viewParms.viewParmType = VPT_PORTAL;
	Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &tr.viewParms, sizeof(viewParms_t));
	tr.numCachedViewParms++;

	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent ) {
	int				i, j;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
===============
R_Radix
===============
*/
static QINLINE void R_Radix( int byte, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];
  int           i;
  unsigned char *sortKey = NULL;
  unsigned char *end = NULL;

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  end = sortKey + ( size * sizeof( drawSurf_t ) );
  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) )
    ++count[ *sortKey ];

  index[ 0 ] = 0;

  for( i = 1; i < 256; ++i )
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];

  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + byte;
  for( i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) )
    dest[ index[ *sortKey ]++ ] = source[ i ];
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];
#ifdef Q3_LITTLE_ENDIAN
  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
#else
  R_Radix( 3, size, source, scratch );
  R_Radix( 2, size, scratch, source );
  R_Radix( 1, size, source, scratch );
  R_Radix( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
}

//==========================================================================================

bool R_IsPostRenderEntity ( const trRefEntity_t *refEntity )
{
	return (refEntity->e.renderfx & RF_DISTORTION) ||
			(refEntity->e.renderfx & RF_FORCEPOST) ||
			(refEntity->e.renderfx & RF_FORCE_ENT_ALPHA);
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( uint32_t sort, int *entityNum, shader_t **shader, int *cubemap, int *postRender )
{
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & QSORT_SHADERNUM_MASK ];
	*postRender = (sort >> QSORT_POSTRENDER_SHIFT ) & QSORT_POSTRENDER_MASK;
	*entityNum = (sort >> QSORT_ENTITYNUM_SHIFT) & QSORT_ENTITYNUM_MASK;
	*cubemap = (sort >> QSORT_CUBEMAP_SHIFT ) & QSORT_CUBEMAP_MASK;
}

uint32_t R_CreateSortKey(int entityNum, int sortedShaderIndex, int cubemapIndex, int postRender)
{
	uint32_t key = 0;

	key |= (sortedShaderIndex & QSORT_SHADERNUM_MASK) << QSORT_SHADERNUM_SHIFT;
	key |= (cubemapIndex & QSORT_CUBEMAP_MASK) << QSORT_CUBEMAP_SHIFT;
	key |= (postRender & QSORT_POSTRENDER_MASK) << QSORT_POSTRENDER_SHIFT;
	key |= (entityNum & QSORT_ENTITYNUM_MASK) << QSORT_ENTITYNUM_SHIFT;

	return key;
}

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf(
	surfaceType_t *surface,
	int entityNum,
	shader_t *shader,
	int fogIndex,
	int dlightMap,
	int postRender,
	int cubemap)
{
	int index;
	drawSurf_t *surf;

	if (tr.refdef.rdflags & RDF_NOFOG)
	{
		fogIndex = 0;
	}

	if ( (shader->surfaceFlags & SURF_FORCESIGHT) && !(tr.refdef.rdflags & RDF_ForceSightOn) )
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

	if (tr.viewParms.flags & VPF_DEPTHSHADOW &&
		(postRender == qtrue || shader->sort != SS_OPAQUE))
	{
		return;
	}

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	surf = tr.refdef.drawSurfs + index;
	surf->surface = surface;

	if (tr.viewParms.flags & VPF_DEPTHSHADOW &&
		shader->useSimpleDepthShader == qtrue)
	{
		surf->sort = R_CreateSortKey(entityNum, tr.defaultShader->sortedIndex, 0, 0);
		surf->dlightBits = 0;
		surf->fogIndex = 0;
	}
	else
	{
		surf->sort = R_CreateSortKey(entityNum, shader->sortedIndex, cubemap, postRender);
		surf->dlightBits = dlightMap;
		surf->fogIndex = fogIndex;
	}

	tr.refdef.numDrawSurfs++;
}

/*
=================
R_SortAndSubmitDrawSurfs
=================
*/
void R_SortAndSubmitDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	// it is possible for some views to not have any surfaces
	if (numDrawSurfs < 1)
		return;

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS )
			numDrawSurfs = MAX_DRAWSURFS;

	R_RadixSort( drawSurfs, numDrawSurfs );

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

static void R_AddEntitySurface(const trRefdef_t *refdef, trRefEntity_t *ent, int entityNum)
{
	shader_t		*shader;

	ent->needDlights = qfalse;

	//
	// the weapon model must be handled special --
	// we don't want the hacked weapon position showing in
	// mirrors, because the true body position will already be drawn
	//
	if ( (ent->e.renderfx & RF_FIRST_PERSON) && (tr.viewParms.flags & VPF_NOVIEWMODEL)) {
		return;
	}

	// simple generated models, like sprites and beams, are not culled
	switch ( ent->e.reType ) {
	case RT_PORTALSURFACE:
		break;		// don't draw anything
	case RT_SPRITE:
	case RT_BEAM:
	case RT_ORIENTED_QUAD:
	case RT_ELECTRICITY:
	case RT_LINE:
	case RT_ORIENTEDLINE:
	case RT_CYLINDER:
	case RT_SABER_GLOW:
		// self blood sprites, talk balloons, etc should not be drawn in the primary
		// view.  We can't just do this check for all entities, because md3
		// entities may still want to cast shadows from them
		if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
			return;
		}
		shader = R_GetShaderByHandle( ent->e.customShader );
		R_AddDrawSurf(
			&entitySurface,
			entityNum,
			shader,
			R_SpriteFogNum(ent),
			0,
			R_IsPostRenderEntity(ent),
			0 /* cubeMap */ );
		break;

	case RT_MODEL:
		// we must set up parts of tr.ori for model culling
		R_RotateForEntity( ent, &tr.viewParms, &tr.ori );

		tr.currentModel = R_GetModelByHandle( ent->e.hModel );
		if (!tr.currentModel) {
			R_AddDrawSurf(
				&entitySurface,
				entityNum,
				tr.defaultShader,
				0,
				0,
				R_IsPostRenderEntity(ent),
				0/* cubeMap */ );
		} else {
			switch ( tr.currentModel->type ) {
			case MOD_MESH:
				R_AddMD3Surfaces( ent, entityNum );
				break;
			case MOD_MDR:
				R_MDRAddAnimSurfaces( ent, entityNum );
				break;
			case MOD_IQM:
				R_AddIQMSurfaces( ent, entityNum );
				break;
			case MOD_BRUSH:
				R_AddBrushModelSurfaces( ent, entityNum );
				break;
			case MOD_MDXM:
				if (ent->e.ghoul2)
					R_AddGhoulSurfaces(ent, entityNum);
				break;
			case MOD_BAD:		// null model axis
				if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal ) {
					break;
				}

				if ( ent->e.ghoul2 &&
					G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)) )
				{
					R_AddGhoulSurfaces(ent, entityNum);
					break;
				}

				// FIX ME: always draw null axis model instead of rejecting the drawcall
				if (tr.currentModel->dataSize > 0)
					R_AddDrawSurf(
						&entitySurface,
						entityNum,
						tr.defaultShader,
						0,
						0,
						R_IsPostRenderEntity(ent),
						0 /* cubeMap */ );
				break;
			default:
				ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
				break;
			}
		}
		break;
	case RT_ENT_CHAIN:
		shader = R_GetShaderByHandle(ent->e.customShader);
		R_AddDrawSurf(
			&entitySurface,
			entityNum,
			shader,
			R_SpriteFogNum(ent),
			false,
			R_IsPostRenderEntity(ent),
			0 /* cubeMap */ );
		break;
	default:
		ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
	}
}

/*
=============
R_AddEntitySurfaces
=============
*/
static void R_AddEntitySurfaces(const trRefdef_t *refdef)
{
	if ( !r_drawentities->integer )
	{
		return;
	}

	int entityStart = 0;
	int numEntities = refdef->num_entities;
	if (tr.world && tr.world->skyboxportal)
	{
		if (tr.viewParms.isSkyPortal)
		{
			// Stop after skyportal entities
			numEntities = tr.skyPortalEntities;
		}
		else
		{
			// Skip skyportal entities
			entityStart = tr.skyPortalEntities;
		}
	}

	for (int i = entityStart; i < numEntities; i++)
	{
		trRefEntity_t *ent = refdef->entities + i;
		R_AddEntitySurface(refdef, ent, i);
	}
}


/*
====================
R_GenerateDrawSurfs
====================
*/
void R_GenerateDrawSurfs( viewParms_t *viewParms, trRefdef_t *refdef ) {

	// TODO: Get rid of this
	if (viewParms->viewParmType == VPT_PLAYER_SHADOWS)
	{
		int entityNum = viewParms->targetFboLayer;
		trRefEntity_t *ent = refdef->entities + entityNum;
		R_AddEntitySurface(refdef, ent, entityNum);
		return;
	}

	R_AddWorldSurfaces(viewParms, refdef);

	R_AddEntitySurfaces(refdef);

	R_AddPolygonSurfaces(refdef);

	if ( tr.viewParms.viewParmType > VPT_POINT_SHADOWS && tr.world )
	{
		R_AddWeatherSurfaces();
	}
}

/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon( int color, int numPoints, float *points ) {
	// FIXME: implement this
#if 0
	int		i;

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color&1, (color>>1)&1, (color>>2)&1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
#endif
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics( void ) {
	if ( !r_debugSurface->integer ) {
		return;
	}

	R_IssuePendingRenderCommands();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri.CM_DrawDebugSurface( R_DebugPolygon );
}


/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView (viewParms_t *parms) {

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	tr.refdef.fistDrawSurf = tr.refdef.numDrawSurfs;

	R_GenerateDrawSurfs(&tr.viewParms, &tr.refdef);

	R_SortAndSubmitDrawSurfs( tr.refdef.drawSurfs + tr.refdef.fistDrawSurf, tr.refdef.numDrawSurfs - tr.refdef.fistDrawSurf);

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}


void R_RenderDlightCubemaps(const refdef_t *fd)
{
	int i;

	for (i = 0; i < tr.refdef.num_dlights; i++)
	{
		viewParms_t		shadowParms;
		int j;

		Com_Memset( &shadowParms, 0, sizeof( shadowParms ) );

		shadowParms.viewportX = 0;
		shadowParms.viewportY = 0;
		shadowParms.viewportWidth = DSHADOW_MAP_SIZE;
		shadowParms.viewportHeight = DSHADOW_MAP_SIZE;
		shadowParms.isPortal = qfalse;
		shadowParms.isMirror = qfalse;

		shadowParms.fovX = 90;
		shadowParms.fovY = 90;

		shadowParms.flags = VPF_DEPTHSHADOW | VPF_NOVIEWMODEL | VPF_POINTSHADOW;
		shadowParms.zFar = tr.refdef.dlights[i].radius;
		shadowParms.zNear = 1.0f;

		VectorCopy( tr.refdef.dlights[i].origin, shadowParms.ori.origin );

		for (j = 0; j < 6; j++)
		{
			switch(j)
			{
				case 0:
					// -X
					VectorSet( shadowParms.ori.axis[0], -1,  0,  0);
					VectorSet( shadowParms.ori.axis[1],  0,  0, -1);
					VectorSet( shadowParms.ori.axis[2],  0,  1,  0);
					break;
				case 1:
					// +X
					VectorSet( shadowParms.ori.axis[0],  1,  0,  0);
					VectorSet( shadowParms.ori.axis[1],  0,  0,  1);
					VectorSet( shadowParms.ori.axis[2],  0,  1,  0);
					break;
				case 2:
					// -Y
					VectorSet( shadowParms.ori.axis[0],  0, -1,  0);
					VectorSet( shadowParms.ori.axis[1],  1,  0,  0);
					VectorSet( shadowParms.ori.axis[2],  0,  0, -1);
					break;
				case 3:
					// +Y
					VectorSet( shadowParms.ori.axis[0],  0,  1,  0);
					VectorSet( shadowParms.ori.axis[1],  1,  0,  0);
					VectorSet( shadowParms.ori.axis[2],  0,  0,  1);
					break;
				case 4:
					// -Z
					VectorSet( shadowParms.ori.axis[0],  0,  0, -1);
					VectorSet( shadowParms.ori.axis[1],  1,  0,  0);
					VectorSet( shadowParms.ori.axis[2],  0,  1,  0);
					break;
				case 5:
					// +Z
					VectorSet( shadowParms.ori.axis[0],  0,  0,  1);
					VectorSet( shadowParms.ori.axis[1], -1,  0,  0);
					VectorSet( shadowParms.ori.axis[2],  0,  1,  0);
					break;
			}

			shadowParms.targetFbo = tr.shadowCubeFbo[i*6+j];
			shadowParms.targetFboLayer = 0;

			R_RenderView(&shadowParms);

			R_IssuePendingRenderCommands();

			tr.refdef.numDrawSurfs = 0;
		}
	}
}

void R_SetOrientationOriginAndAxis(
	orientationr_t& orientation,
	const vec3_t origin,
	const vec3_t forward,
	const vec3_t left,
	const vec3_t up)
{
	VectorCopy(origin, orientation.origin);
	VectorCopy(forward, orientation.axis[0]);
	VectorCopy(left, orientation.axis[1]);
	VectorCopy(up, orientation.axis[2]);
}

void R_SetOrientationOriginAndAxis(
	orientationr_t& orientation,
	const vec3_t origin,
	const matrix3_t axis)
{
	R_SetOrientationOriginAndAxis(orientation, origin, axis[0], axis[1], axis[2]);
}

void R_SetupViewParmsForOrthoRendering(
	int viewportWidth,
	int viewportHeight,
	FBO_t *fbo,
	viewParmFlags_t viewParmFlags,
	const orientationr_t& orientation,
	const vec3_t viewBounds[2])
{
	viewParms_t& viewParms = tr.viewParms;

	viewParms = {};
	viewParms.viewportWidth  = viewportWidth;
	viewParms.viewportHeight = viewportHeight;
	viewParms.flags = viewParmFlags;
	viewParms.targetFbo = fbo;
	viewParms.zFar = viewBounds[1][0];
	VectorCopy(orientation.origin, viewParms.ori.origin);
	for (int i = 0; i < 3; ++i)
		VectorCopy(orientation.axis[i], viewParms.ori.axis[i]);

	VectorCopy(orientation.origin, viewParms.pvsOrigin);

	tr.viewCount++;

	viewParms.frameSceneNum = tr.frameSceneNum;
	viewParms.frameCount = tr.frameCount;

	R_RotateForViewer(&viewParms.world, &viewParms);
	R_SetupProjectionOrtho(&viewParms, viewBounds);
}

void R_SetupPshadowMaps(trRefdef_t *refdef)
{
	viewParms_t		shadowParms;
	int i;

	// first, make a list of shadows
	for ( i = 0; i < refdef->num_entities; i++)
	{
		trRefEntity_t *ent = &refdef->entities[i];

		if((ent->e.renderfx & (RF_FIRST_PERSON | RF_NOSHADOW)))
			continue;

		//if((ent->e.renderfx & RF_THIRD_PERSON))
			//continue;

		if (ent->e.reType == RT_MODEL)
		{
			model_t *model = R_GetModelByHandle( ent->e.hModel );
			pshadow_t shadow;
			float radius = 0.0f;
			float scale = 1.0f;
			vec3_t diff;
			int j;

			if (!model)
				continue;

			if (ent->e.nonNormalizedAxes)
			{
				scale = VectorLength( ent->e.axis[0] );
			}

			switch (model->type)
			{
				case MOD_MDXM:
				case MOD_BAD:
				{
					if (ent->e.ghoul2 && G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)))
					{
						// scale the radius if needed
						float largestScale = ent->e.modelScale[0];
						if (ent->e.modelScale[1] > largestScale)
							largestScale = ent->e.modelScale[1];
						if (ent->e.modelScale[2] > largestScale)
							largestScale = ent->e.modelScale[2];
						if (!largestScale)
							largestScale = 1;
						radius = ent->e.radius * largestScale * 1.2;
					}
				}
				break;

				default:
					break;
			}

			if (!radius)
				continue;

			// Cull entities that are behind the viewer by more than lightRadius
			VectorSubtract(ent->e.lightingOrigin, refdef->vieworg, diff);
			if (DotProduct(diff, refdef->viewaxis[0]) < -r_pshadowDist->value)
				continue;

			memset(&shadow, 0, sizeof(shadow));

			shadow.numEntities = 1;
			shadow.entityNums[0] = i;
			shadow.viewRadius = radius;
			shadow.lightRadius = r_pshadowDist->value;
			VectorCopy(ent->e.lightingOrigin, shadow.viewOrigin);
			shadow.sort = DotProduct(diff, diff) / (radius * radius);
			VectorCopy(ent->e.origin, shadow.entityOrigins[0]);
			shadow.entityRadiuses[0] = radius;

			for (j = 0; j < MAX_CALC_PSHADOWS; j++)
			{
				pshadow_t swap;

				if (j + 1 > refdef->num_pshadows)
				{
					refdef->num_pshadows = j + 1;
					refdef->pshadows[j] = shadow;
					break;
				}

				// sort shadows by distance from camera divided by radius
				// FIXME: sort better
				if (refdef->pshadows[j].sort <= shadow.sort)
					continue;

				swap = refdef->pshadows[j];
				refdef->pshadows[j] = shadow;
				shadow = swap;
			}
		}
	}

	// cap number of drawn pshadows
	if (refdef->num_pshadows > MAX_DRAWN_PSHADOWS)
	{
		refdef->num_pshadows = MAX_DRAWN_PSHADOWS;
	}

	// next, fill up the rest of the shadow info
	for ( i = 0; i < refdef->num_pshadows; i++)
	{
		pshadow_t *shadow = &refdef->pshadows[i];
		vec3_t up;
		vec3_t ambientLight, directedLight, lightDir;

		VectorSet(lightDir, 0.57735f, 0.57735f, 0.57735f);

		R_LightForPoint(shadow->viewOrigin, ambientLight, directedLight, lightDir);
#if 1
		lightDir[2] = 0.0f;
		VectorNormalize(lightDir);
		VectorSet(lightDir, lightDir[0] * 0.3f, lightDir[1] * 0.3f, 1.0f);
		VectorNormalize(lightDir);
#else
		// sometimes there's no light
		if (DotProduct(lightDir, lightDir) < 0.9f)
			VectorSet(lightDir, 0.0f, 0.0f, 1.0f);
#endif
		if (shadow->viewRadius * 3.0f > shadow->lightRadius)
		{
			shadow->lightRadius = shadow->viewRadius * 3.0f;
		}

		VectorMA(shadow->viewOrigin, shadow->viewRadius, lightDir, shadow->lightOrigin);

		// make up a projection, up doesn't matter
		VectorScale(lightDir, -1.0f, shadow->lightViewAxis[0]);
		VectorSet(up, 0, 0, -1);

		if ( fabs(DotProduct(up, shadow->lightViewAxis[0])) > 0.9f )
		{
			VectorSet(up, -1, 0, 0);
		}

		CrossProduct(shadow->lightViewAxis[0], up, shadow->lightViewAxis[1]);
		VectorNormalize(shadow->lightViewAxis[1]);
		CrossProduct(shadow->lightViewAxis[0], shadow->lightViewAxis[1], shadow->lightViewAxis[2]);

		VectorCopy(shadow->lightViewAxis[0], shadow->cullPlane.normal);
		shadow->cullPlane.dist = DotProduct(shadow->cullPlane.normal, shadow->lightOrigin);
		shadow->cullPlane.type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&shadow->cullPlane);
	}
}

void R_RenderCubemapSide(int cubemapIndex, int cubemapSide, bool bounce)
{
	refdef_t refdef = {};
	float oldColorScale = tr.refdef.colorScale;

	VectorCopy(tr.cubemaps[cubemapIndex].origin, refdef.vieworg);
	refdef.fov_x = 90;
	refdef.fov_y = 90;
	refdef.width = tr.renderCubeFbo[cubemapSide]->width;
	refdef.height = tr.renderCubeFbo[cubemapSide]->height;
	refdef.x = 0;
	refdef.y = 0;

	switch (cubemapSide)
	{
	case 0:
		// +X
		VectorSet(refdef.viewaxis[0], 1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, 1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 1:
		// -X
		VectorSet(refdef.viewaxis[0], -1, 0, 0);
		VectorSet(refdef.viewaxis[1], 0, 0, -1);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 2:
		// +Y
		VectorSet(refdef.viewaxis[0], 0, 1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, 1);
		break;
	case 3:
		// -Y
		VectorSet(refdef.viewaxis[0], 0, -1, 0);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, 0, -1);
		break;
	case 4:
		// +Z
		VectorSet(refdef.viewaxis[0], 0, 0, 1);
		VectorSet(refdef.viewaxis[1], -1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	case 5:
		// -Z
		VectorSet(refdef.viewaxis[0], 0, 0, -1);
		VectorSet(refdef.viewaxis[1], 1, 0, 0);
		VectorSet(refdef.viewaxis[2], 0, -1, 0);
		break;
	}

	RE_BeginFrame(STEREO_CENTER);

	RE_BeginScene(&refdef);

	tr.refdef.colorScale = 1.0f;

	for (int i = 0; i < tr.numCachedViewParms; i++)
	{
		if (!tr.cachedViewParms[i].targetFbo)
		{
			tr.cachedViewParms[i].targetFbo = tr.renderCubeFbo[cubemapSide];
			tr.cachedViewParms[i].targetFboLayer = 0;
			tr.cachedViewParms[i].flags |= VPF_NOVIEWMODEL;
			if (!bounce)
				tr.cachedViewParms[i].flags |= VPF_NOCUBEMAPS;
		}
		R_RenderView(&tr.cachedViewParms[i]);
		R_IssuePendingRenderCommands();
		tr.refdef.numDrawSurfs = 0;
	}

	RE_EndScene();

	R_NewFrameSync();
}

void R_SetupViewParms(const trRefdef_t *refdef)
{
	tr.viewCount++;
	Com_Memset(&tr.viewParms, 0, sizeof(viewParms_t));
	tr.viewParms.viewportX = refdef->x;

	// Shoud be just refef->y but this flips the menu orientation for models, so its actually needed like this
	if (!tr.world)
		tr.viewParms.viewportY = glConfig.vidHeight - (refdef->y + refdef->height);
	else
		tr.viewParms.viewportY = refdef->y;

	tr.viewParms.viewportWidth = refdef->width;
	tr.viewParms.viewportHeight = refdef->height;
	tr.viewParms.zNear = r_znear->value;

	tr.viewParms.fovX = refdef->fov_x;
	tr.viewParms.fovY = refdef->fov_y;

	VectorCopy(refdef->vieworg, tr.viewParms.ori.origin);
	VectorCopy(refdef->viewaxis[0], tr.viewParms.ori.axis[0]);
	VectorCopy(refdef->viewaxis[1], tr.viewParms.ori.axis[1]);
	VectorCopy(refdef->viewaxis[2], tr.viewParms.ori.axis[2]);

	VectorCopy(refdef->vieworg, tr.viewParms.pvsOrigin);

	R_RotateForViewer(&tr.viewParms.world, &tr.viewParms);
	R_SetupProjection(&tr.viewParms, tr.viewParms.zNear, tr.viewParms.zFar, qtrue);

	if (tr.world)
	{
		R_MarkLeaves();

		// clear out the visible min/max
		ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);
		int planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;
		// set vis bounds
		R_RecursiveWorldNode(tr.world->nodes, planeBits, 0, 0);
	}

	R_SetFarClip(&tr.viewParms, refdef);
	R_SetupProjectionZ(&tr.viewParms);
}

qboolean R_AddPortalView(const trRefdef_t *refdef)
{
	if (!tr.world)
		return qfalse;

	for (int i = 0; i < tr.world->numWorldSurfaces; i++)
	{
		if (tr.world->surfacesViewCount[i] != tr.viewCount)
			continue;

		msurface_t	*surface = tr.world->surfaces + i;
		if (surface->shader->sort != SS_PORTAL) {
			continue;
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if (R_MirrorViewBySurface(surface, REFENTITYNUM_WORLD)) {
			return qtrue;		// only one mirror view at a time
		}
	}

	for (int i = 0; i < tr.world->numMergedSurfaces; i++)
	{
		if (tr.world->mergedSurfacesViewCount[i] != tr.viewCount)
			continue;

		msurface_t	*surface = tr.world->mergedSurfaces + i;
		if (surface->shader->sort != SS_PORTAL) {
			continue;
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if (R_MirrorViewBySurface(surface, REFENTITYNUM_WORLD)) {
			return qtrue;		// only one mirror view at a time
		}
	}

	for (int i = 0; i < refdef->num_entities; i++)
	{
		trRefEntity_t *ent = &refdef->entities[i];
		switch (ent->e.reType) {
		case RT_PORTALSURFACE:
			break;		// don't draw anything
		case RT_SPRITE:
		case RT_BEAM:
		case RT_ORIENTED_QUAD:
		case RT_ELECTRICITY:
		case RT_LINE:
		case RT_ORIENTEDLINE:
		case RT_CYLINDER:
		case RT_SABER_GLOW:
			break;

		case RT_MODEL:

			// we must set up parts of tr.ori for model culling
			R_RotateForEntity(ent, &tr.viewParms, &tr.ori);

			tr.currentModel = R_GetModelByHandle(ent->e.hModel);
			if (!tr.currentModel) {
				continue;
			}
			else {
				switch (tr.currentModel->type) {
				case MOD_BRUSH:
				{
					//R_AddBrushModelSurfaces(ent, i);
					bmodel_t *bmodel = tr.currentModel->data.bmodel;
					world_t *world = R_GetWorld(bmodel->worldIndex);
					for (int j = 0; j < bmodel->numSurfaces; j++) {
						int surf = bmodel->firstSurface + j;

						// TODO: use pvs of misc_bsp models
						msurface_t	*surface = world->surfaces + surf;
						if (surface->shader->sort != SS_PORTAL) {
							continue;
						}

						// if the mirror was completely clipped away, we may need to check another surface
						if (R_MirrorViewBySurface(surface, i)) {
							return qtrue;		// only one mirror view at a time
						}
					}
				}
				break;
				case MOD_MESH:
				case MOD_MDR:
				case MOD_IQM:
				case MOD_MDXM:
				case MOD_BAD:
				default:
					break;
				}
			}
			break;
		case RT_ENT_CHAIN:
			break;
		default:
			break;
		}
	}

	return qfalse;
}

static float CalcSplit(float n, float f, float i, float m)
{
	return (n * pow(f / n, i / m) + (f - n) * i / m) / 2.0f;
}

void R_GatherFrameViews(trRefdef_t *refdef)
{
	int mainFlags = 0;
	// skyportal view
	if (tr.world && tr.world->skyboxportal)
	{
		tr.viewCount++;
		tr.viewParms = tr.skyPortalParms;
		R_RotateForViewer(&tr.viewParms.world, &tr.viewParms);
		R_SetupProjection(&tr.viewParms, tr.viewParms.zNear, tr.viewParms.zFar, qtrue);

		VectorCopy(tr.skyPortalParms.pvsOrigin, tr.viewParms.pvsOrigin);
		R_MarkLeaves();

		// clear out the visible min/max
		ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);
		int planeBits = (tr.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 31 : 15;
		// set vis bounds
		R_RecursiveWorldNode(tr.world->nodes, planeBits, 0, 0);
		R_SetFarClip(&tr.viewParms, refdef);
		R_SetupProjectionZ(&tr.viewParms);

		tr.viewParms.currentViewParm = tr.numCachedViewParms;
		tr.viewParms.viewParmType = VPT_SKYPORTAL;
		Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &tr.viewParms, sizeof(viewParms_t));
		tr.numCachedViewParms++;
	}

	VectorCopy(refdef->vieworg, tr.viewParms.pvsOrigin);
	if (tr.world)
		R_MarkLeaves();

	if (!(refdef->rdflags & RDF_NOWORLDMODEL))
	{
		// dlight shadowmaps
		if (refdef->num_dlights && r_dlightMode->integer >= 2)
		{
			for (int i = 0; i < refdef->num_dlights; i++)
			{
				viewParms_t		shadowParms;
				int j;

				Com_Memset(&shadowParms, 0, sizeof(shadowParms));

				shadowParms.viewportX = 0;
				shadowParms.viewportY = 0;
				shadowParms.viewportWidth = DSHADOW_MAP_SIZE;
				shadowParms.viewportHeight = DSHADOW_MAP_SIZE;
				shadowParms.isPortal = qfalse;
				shadowParms.isMirror = qfalse;

				shadowParms.fovX = 90;
				shadowParms.fovY = 90;

				shadowParms.flags = VPF_DEPTHSHADOW | VPF_NOVIEWMODEL | VPF_POINTSHADOW;
				shadowParms.zFar = refdef->dlights[i].radius;
				shadowParms.zNear = 1.0f;

				VectorCopy(refdef->dlights[i].origin, shadowParms.ori.origin);

				for (j = 0; j < 6; j++)
				{
					switch (j)
					{
					case 0:
						// -X
						VectorSet(shadowParms.ori.axis[0], -1, 0, 0);
						VectorSet(shadowParms.ori.axis[1], 0, 0, -1);
						VectorSet(shadowParms.ori.axis[2], 0, 1, 0);
						break;
					case 1:
						// +X
						VectorSet(shadowParms.ori.axis[0], 1, 0, 0);
						VectorSet(shadowParms.ori.axis[1], 0, 0, 1);
						VectorSet(shadowParms.ori.axis[2], 0, 1, 0);
						break;
					case 2:
						// -Y
						VectorSet(shadowParms.ori.axis[0], 0, -1, 0);
						VectorSet(shadowParms.ori.axis[1], 1, 0, 0);
						VectorSet(shadowParms.ori.axis[2], 0, 0, -1);
						break;
					case 3:
						// +Y
						VectorSet(shadowParms.ori.axis[0], 0, 1, 0);
						VectorSet(shadowParms.ori.axis[1], 1, 0, 0);
						VectorSet(shadowParms.ori.axis[2], 0, 0, 1);
						break;
					case 4:
						// -Z
						VectorSet(shadowParms.ori.axis[0], 0, 0, -1);
						VectorSet(shadowParms.ori.axis[1], 1, 0, 0);
						VectorSet(shadowParms.ori.axis[2], 0, 1, 0);
						break;
					case 5:
						// +Z
						VectorSet(shadowParms.ori.axis[0], 0, 0, 1);
						VectorSet(shadowParms.ori.axis[1], -1, 0, 0);
						VectorSet(shadowParms.ori.axis[2], 0, 1, 0);
						break;
					}

					shadowParms.targetFbo = tr.shadowCubeFbo[i * 6 + j];
					shadowParms.targetFboLayer = 0;

					shadowParms.currentViewParm = tr.numCachedViewParms;
					shadowParms.viewParmType = VPT_POINT_SHADOWS;

					R_RotateForViewer(&shadowParms.world, &shadowParms);
					R_SetupProjection(&shadowParms, shadowParms.zNear, shadowParms.zFar, qtrue);
					R_SetupProjectionZ(&shadowParms);

					Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &shadowParms, sizeof(viewParms_t));
					tr.numCachedViewParms++;
				}
			}
		}

		// pshadow shadowmaps
		if (r_shadows->integer == 4)
		{
			R_SetupPshadowMaps(refdef);

			for (int i = 0; i < tr.refdef.num_pshadows; i++)
			{
				pshadow_t *shadow = &tr.refdef.pshadows[i];

				tr.viewParms.viewportX = 0;
				tr.viewParms.viewportY = 0;
				tr.viewParms.viewportWidth = PSHADOW_MAP_SIZE;
				tr.viewParms.viewportHeight = PSHADOW_MAP_SIZE;
				tr.viewParms.isPortal = qfalse;
				tr.viewParms.isMirror = qfalse;

				tr.viewParms.fovX = 90;
				tr.viewParms.fovY = 90;

				tr.viewParms.targetFbo = tr.pshadowFbos[i];
				tr.viewParms.targetFboLayer = shadow->entityNums[0];

				tr.viewParms.flags = (viewParmFlags_t)(VPF_DEPTHSHADOW | VPF_NOVIEWMODEL);
				tr.viewParms.viewParmType = VPT_PLAYER_SHADOWS;
				tr.viewParms.zFar = shadow->lightRadius;

				VectorCopy(shadow->lightOrigin, tr.viewParms.ori.origin);

				VectorCopy(shadow->lightViewAxis[0], tr.viewParms.ori.axis[0]);
				VectorCopy(shadow->lightViewAxis[1], tr.viewParms.ori.axis[1]);
				VectorCopy(shadow->lightViewAxis[2], tr.viewParms.ori.axis[2]);

				{
					tr.viewCount++;
					tr.viewParms.frameSceneNum = tr.frameSceneNum;
					tr.viewParms.frameCount = tr.frameCount;

					// set viewParms.world
					R_RotateForViewer(&tr.viewParms.world, &tr.viewParms);

					{
						float xmin, xmax, ymin, ymax, znear, zfar;
						viewParms_t *dest = &tr.viewParms;
						vec3_t pop;

						xmin = ymin = -shadow->viewRadius;
						xmax = ymax = shadow->viewRadius;
						znear = 0;
						zfar = shadow->lightRadius;

						dest->projectionMatrix[0] = 2 / (xmax - xmin);
						dest->projectionMatrix[4] = 0;
						dest->projectionMatrix[8] = (xmax + xmin) / (xmax - xmin);
						dest->projectionMatrix[12] = 0;

						dest->projectionMatrix[1] = 0;
						dest->projectionMatrix[5] = 2 / (ymax - ymin);
						dest->projectionMatrix[9] = (ymax + ymin) / (ymax - ymin);	// normally 0
						dest->projectionMatrix[13] = 0;

						dest->projectionMatrix[2] = 0;
						dest->projectionMatrix[6] = 0;
						dest->projectionMatrix[10] = 2 / (zfar - znear);
						dest->projectionMatrix[14] = 0;

						dest->projectionMatrix[3] = 0;
						dest->projectionMatrix[7] = 0;
						dest->projectionMatrix[11] = 0;
						dest->projectionMatrix[15] = 1;

						VectorScale(dest->ori.axis[1], 1.0f, dest->frustum[0].normal);
						VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[0].normal, pop);
						dest->frustum[0].dist = DotProduct(pop, dest->frustum[0].normal);

						VectorScale(dest->ori.axis[1], -1.0f, dest->frustum[1].normal);
						VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[1].normal, pop);
						dest->frustum[1].dist = DotProduct(pop, dest->frustum[1].normal);

						VectorScale(dest->ori.axis[2], 1.0f, dest->frustum[2].normal);
						VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[2].normal, pop);
						dest->frustum[2].dist = DotProduct(pop, dest->frustum[2].normal);

						VectorScale(dest->ori.axis[2], -1.0f, dest->frustum[3].normal);
						VectorMA(dest->ori.origin, -shadow->viewRadius, dest->frustum[3].normal, pop);
						dest->frustum[3].dist = DotProduct(pop, dest->frustum[3].normal);

						VectorScale(dest->ori.axis[0], -1.0f, dest->frustum[4].normal);
						VectorMA(dest->ori.origin, -shadow->lightRadius, dest->frustum[4].normal, pop);
						dest->frustum[4].dist = DotProduct(pop, dest->frustum[4].normal);

						for (int j = 0; j < 5; j++)
						{
							dest->frustum[j].type = PLANE_NON_AXIAL;
							SetPlaneSignbits(&dest->frustum[j]);
						}

						dest->flags |= VPF_FARPLANEFRUSTUM;
					}

					tr.viewParms.currentViewParm = tr.numCachedViewParms;
					Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &tr.viewParms, sizeof(viewParms_t));
					tr.numCachedViewParms++;
				}
			}

		}

		// sun shadowmaps
		if (r_sunlightMode->integer && r_depthPrepass->value && (r_forceSun->integer || tr.sunShadows))
		{
			vec3_t lightViewAxis[3];
			vec3_t lightOrigin;
			float splitZNear, splitZFar, splitBias;
			float viewZNear, viewZFar;
			vec3_t lightviewBounds[2];

			viewZNear = r_shadowCascadeZNear->value;
			viewZFar = r_shadowCascadeZFar->value;
			splitBias = r_shadowCascadeZBias->value;

			for (int level = 0; level < 3; level++)
			{
				switch (level)
				{
				case 0:
				default:
					splitZNear = viewZNear;
					splitZFar = CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
					break;
				case 1:
					splitZNear = CalcSplit(viewZNear, viewZFar, 1, 3) + splitBias;
					splitZFar = CalcSplit(viewZNear, viewZFar, 2, 3) + splitBias;
					break;
				case 2:
					splitZNear = CalcSplit(viewZNear, viewZFar, 2, 3) + splitBias;
					splitZFar = viewZFar;
					break;
				}

				VectorCopy(refdef->vieworg, lightOrigin);
				// Make up a projection
				VectorScale(refdef->sunDir, -1.0f, lightViewAxis[0]);
				// Use world up as light view up
				VectorSet(lightViewAxis[2], 0, 0, 1);

				// Check if too close to parallel to light direction
				if (fabs(DotProduct(lightViewAxis[2], lightViewAxis[0])) > 0.9f)
				{
					// Use world left as light view up
					VectorSet(lightViewAxis[2], 0, 1, 0);
				}

				// clean axes
				CrossProduct(lightViewAxis[2], lightViewAxis[0], lightViewAxis[1]);
				VectorNormalize(lightViewAxis[1]);
				CrossProduct(lightViewAxis[0], lightViewAxis[1], lightViewAxis[2]);

				// Create bounds for light projection using slice of view projection
				{
					ClearBounds(lightviewBounds[0], lightviewBounds[1]);

					vec3_t point, base;
					float lx, ly, radius;
					vec3_t splitCenter, frustrumPoint0, frustrumPoint7;

					VectorSet(splitCenter, 0.f, 0.f, 0.f);

					// add view near plane
					lx = splitZNear * tan(refdef->fov_x * M_PI / 360.0f);
					ly = splitZNear * tan(refdef->fov_y * M_PI / 360.0f);
					VectorMA(refdef->vieworg, splitZNear, refdef->viewaxis[0], base);

					VectorMA(base, lx, refdef->viewaxis[1], point);
					VectorMA(point, ly, refdef->viewaxis[2], point);
					VectorCopy(point, frustrumPoint0);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, -lx, refdef->viewaxis[1], point);
					VectorMA(point, ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, lx, refdef->viewaxis[1], point);
					VectorMA(point, -ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, -lx, refdef->viewaxis[1], point);
					VectorMA(point, -ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					// add view far plane
					lx = splitZFar * tan(refdef->fov_x * M_PI / 360.0f);
					ly = splitZFar * tan(refdef->fov_y * M_PI / 360.0f);
					VectorMA(refdef->vieworg, splitZFar, refdef->viewaxis[0], base);

					VectorMA(base, lx, refdef->viewaxis[1], point);
					VectorMA(point, ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, -lx, refdef->viewaxis[1], point);
					VectorMA(point, ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, lx, refdef->viewaxis[1], point);
					VectorMA(point, -ly, refdef->viewaxis[2], point);
					VectorAdd(point, splitCenter, splitCenter);

					VectorMA(base, -lx, refdef->viewaxis[1], point);
					VectorMA(point, -ly, refdef->viewaxis[2], point);
					VectorCopy(point, frustrumPoint7);
					VectorAdd(point, splitCenter, splitCenter);

					VectorScale(splitCenter, 1.0f / 8.0f, splitCenter);
					radius = Distance(frustrumPoint0, frustrumPoint7) / 2.0f;
					lightviewBounds[0][0] = -radius;
					lightviewBounds[0][1] = -radius;
					lightviewBounds[0][2] = -radius;
					lightviewBounds[1][0] = radius;
					lightviewBounds[1][1] = radius;
					lightviewBounds[1][2] = radius;

					VectorCopy(splitCenter, lightOrigin);
				}

				orientationr_t orientation = {};
				R_SetOrientationOriginAndAxis(orientation, lightOrigin, lightViewAxis);

				R_SetupViewParmsForOrthoRendering(
					tr.sunShadowFbo[level]->width,
					tr.sunShadowFbo[level]->height,
					tr.sunShadowFbo[level],
					VPF_DEPTHSHADOW | VPF_DEPTHCLAMP | VPF_ORTHOGRAPHIC | VPF_NOVIEWMODEL | VPF_SHADOWCASCADES,
					orientation,
					lightviewBounds);

				// Moving the Light in Texel-Sized Increments
				// from http://msdn.microsoft.com/en-us/library/windows/desktop/ee416324%28v=vs.85%29.aspx
				static float worldUnitsPerTexel = 2.0f * lightviewBounds[1][0] / (float)tr.sunShadowFbo[level]->width;
				static float invWorldUnitsPerTexel = tr.sunShadowFbo[level]->width / (2.0f * lightviewBounds[1][0]);

				tr.viewParms.world.modelViewMatrix[12] = floorf(tr.viewParms.world.modelViewMatrix[12] * invWorldUnitsPerTexel);
				tr.viewParms.world.modelViewMatrix[13] = floorf(tr.viewParms.world.modelViewMatrix[13] * invWorldUnitsPerTexel);
				tr.viewParms.world.modelViewMatrix[14] = floorf(tr.viewParms.world.modelViewMatrix[14] * invWorldUnitsPerTexel);

				tr.viewParms.world.modelViewMatrix[12] *= worldUnitsPerTexel;
				tr.viewParms.world.modelViewMatrix[13] *= worldUnitsPerTexel;
				tr.viewParms.world.modelViewMatrix[14] *= worldUnitsPerTexel;

				Matrix16Multiply(
					tr.viewParms.projectionMatrix,
					tr.viewParms.world.modelViewMatrix,
					refdef->sunShadowMvp[level]);

				tr.viewParms.currentViewParm = tr.numCachedViewParms;
				tr.viewParms.viewParmType = VPT_SUN_SHADOWS;
				Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &tr.viewParms, sizeof(viewParms_t));
				tr.numCachedViewParms++;
			}
			mainFlags |= VPF_USESUNLIGHT;
		}
	}

	// main view
	{
		R_SetupViewParms(refdef);
		if (R_AddPortalView(refdef))
		{
			// this is a debug option to see exactly what is being mirrored
			if (r_portalOnly->integer)
				return;

			R_SetupViewParms(refdef);
		}

		tr.viewParms.stereoFrame = STEREO_CENTER; // FIXME
		tr.viewParms.flags = mainFlags;

		tr.viewParms.currentViewParm = tr.numCachedViewParms;
		tr.viewParms.viewParmType = VPT_MAIN;
		Com_Memcpy(&tr.cachedViewParms[tr.numCachedViewParms], &tr.viewParms, sizeof(viewParms_t));
		tr.numCachedViewParms++;
	}
}
