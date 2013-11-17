/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// tr_main.c -- main control flow for each frame

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#include "tr_local.h"

#if !defined(G2_H_INC)
	#include "../ghoul2/G2.h"
#endif

void R_AddTerrainSurfaces(void);

trGlobals_t		tr;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

refimport_t ri;

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t	entitySurface = SF_ENTITY;

/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLocalBox (const vec3_t bounds[2]) {
	int		i, j;
	vec3_t	transformed[8];
	float	dists[8];
	vec3_t	v;
	cplane_t	*frust;
	int			anyBack;
	int			front, back;

	if ( r_nocull->integer==1 ) {
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
	for (i = 0 ; i < 5 ; i++) {
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
int R_CullPointAndRadius( const vec3_t pt, float radius )
{
	int		i;
	float	dist;
	cplane_t	*frust;
	qboolean mightBeClipped = qfalse;

	if ( r_nocull->integer==1 ) {
		return CULL_CLIP;
	}

	// check against frustum planes
#ifndef __NO_JK2
	if( com_jk2 && com_jk2->integer )
	{
		// They used 4 frustrum planes in JK2, and 5 in JKA --eez
		for (i = 0 ; i < 4 ; i++) 
		{
			frust = &tr.viewParms.frustum[i];

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
	}
	else
	{
#endif
	for (i = 0 ; i < 5 ; i++) 
	{
		frust = &tr.viewParms.frustum[i];

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
#ifndef __NO_JK2
	}
#endif

	if ( mightBeClipped )
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
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

float preTransEntMatrix[16];

void R_InvertMatrix(float *sourcemat, float *destmat)
{
	int i, j, temp=0;

    for (i = 0; i < 3; i++)
	{
        for (j = 0; j < 3; j++)
		{
            destmat[j*4 + i] = sourcemat[temp++];
		}
	}
    for (i = 0; i < 3; i++)
	{
		temp = i*4;
        destmat[temp+3]=0;		// destmat[destmat[i][3]=0;
        for (j = 0; j < 3; j++)
		{
            destmat[temp+3]-=destmat[temp+j]*sourcemat[j*4+3];		// dest->matrix[i][3]-=dest->matrix[i][j]*src->matrix[j][3];
		}
	}
}

/*
=================
R_WorldNormalToEntity 

=================
*/
void R_WorldNormalToEntity (const vec3_t worldvec, vec3_t entvec) 
{
	entvec[0] = -worldvec[0] * preTransEntMatrix[0] - worldvec[1] * preTransEntMatrix[4] + worldvec[2] * preTransEntMatrix[8];
	entvec[1] = -worldvec[0] * preTransEntMatrix[1] - worldvec[1] * preTransEntMatrix[5] + worldvec[2] * preTransEntMatrix[9];
	entvec[2] = -worldvec[0] * preTransEntMatrix[2] - worldvec[1] * preTransEntMatrix[6] + worldvec[2] * preTransEntMatrix[10];
}

/*
=================
R_WorldPointToEntity 

=================
*/
/*void R_WorldPointToEntity (vec3_t worldvec, vec3_t entvec)
{
	entvec[0] = worldvec[0] * preTransEntMatrix[0] + worldvec[1] * preTransEntMatrix[4] + worldvec[2] * preTransEntMatrix[8]+preTransEntMatrix[12];
	entvec[1] = worldvec[0] * preTransEntMatrix[1] + worldvec[1] * preTransEntMatrix[5] + worldvec[2] * preTransEntMatrix[9]+preTransEntMatrix[13];
	entvec[2] = worldvec[0] * preTransEntMatrix[2] + worldvec[1] * preTransEntMatrix[6] + worldvec[2] * preTransEntMatrix[10]+preTransEntMatrix[14];
}
*/

/*
=================
R_WorldToLocal

=================
*/
void R_WorldToLocal (vec3_t world, vec3_t local) {
	local[0] = DotProduct(world, tr.ori.axis[0]);
	local[1] = DotProduct(world, tr.ori.axis[1]);
	local[2] = DotProduct(world, tr.ori.axis[2]);
}

/*
==========================
R_TransformModelToClip

==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] = 
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
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

	window[0] = 0.5 * ( 1.0 + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5 * ( 1.0 + normalized[1] ) * view->viewportHeight;
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
					   orientationr_t *ori ) {
//	float	glMatrix[16];
	vec3_t	delta;
	float	axisLength;

	if ( ent->e.reType != RT_MODEL ) {
		*ori = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, ori->origin );

	VectorCopy( ent->e.axis[0], ori->axis[0] );
	VectorCopy( ent->e.axis[1], ori->axis[1] );
	VectorCopy( ent->e.axis[2], ori->axis[2] );

	preTransEntMatrix[0] = ori->axis[0][0];
	preTransEntMatrix[4] = ori->axis[1][0];
	preTransEntMatrix[8] = ori->axis[2][0];
	preTransEntMatrix[12] = ori->origin[0];

	preTransEntMatrix[1] = ori->axis[0][1];
	preTransEntMatrix[5] = ori->axis[1][1];
	preTransEntMatrix[9] = ori->axis[2][1];
	preTransEntMatrix[13] = ori->origin[1];

	preTransEntMatrix[2] = ori->axis[0][2];
	preTransEntMatrix[6] = ori->axis[1][2];
	preTransEntMatrix[10] = ori->axis[2][2];
	preTransEntMatrix[14] = ori->origin[2];

	preTransEntMatrix[3] = 0;
	preTransEntMatrix[7] = 0;
	preTransEntMatrix[11] = 0;
	preTransEntMatrix[15] = 1;

	myGlMultMatrix( preTransEntMatrix, viewParms->world.modelMatrix, ori->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->ori.origin, ori->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0 / axisLength;
		}
	} else {
		axisLength = 1.0;
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
void R_RotateForViewer (void) 
{
	float	viewerMatrix[16];
	vec3_t	origin;

	memset (&tr.ori, 0, sizeof(tr.ori));
	tr.ori.axis[0][0] = 1;
	tr.ori.axis[1][1] = 1;
	tr.ori.axis[2][2] = 1;
	VectorCopy (tr.viewParms.ori.origin, tr.ori.viewOrigin);

	// transform by the camera placement
	VectorCopy( tr.viewParms.ori.origin, origin );

	viewerMatrix[0] = tr.viewParms.ori.axis[0][0];
	viewerMatrix[4] = tr.viewParms.ori.axis[0][1];
	viewerMatrix[8] = tr.viewParms.ori.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = tr.viewParms.ori.axis[1][0];
	viewerMatrix[5] = tr.viewParms.ori.axis[1][1];
	viewerMatrix[9] = tr.viewParms.ori.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = tr.viewParms.ori.axis[2][0];
	viewerMatrix[6] = tr.viewParms.ori.axis[2][1];
	viewerMatrix[10] = tr.viewParms.ori.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, tr.ori.modelMatrix );

	tr.viewParms.world = tr.ori;

}

/*
** SetFarClip
*/
static void SetFarClip( void )
{
	float	farthestCornerDistance = 0;
	int		i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
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
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		distance = DistanceSquared(tr.viewParms.ori.origin, v);

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}
	// Bring in the zFar to the distanceCull distance
	// The sky renders at zFar so need to move it out a little
	// ...and make sure there is a minimum zfar to prevent problems
	tr.viewParms.zFar = Com_Clamp(2048.0f, tr.distanceCull * (1.732), sqrtf( farthestCornerDistance ));
}


/*
===============
R_SetupProjection
===============
*/
void R_SetupProjection( void ) {
	float	xmin, xmax, ymin, ymax;
	float	width, height, depth;
	float	zNear, zFar;

	// dynamically compute far clip plane distance
	SetFarClip();

	//
	// set up projection matrix
	//
	zNear	= r_znear->value;
	zFar	= tr.viewParms.zFar;

	ymax = zNear * tan( tr.refdef.fov_y * M_PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( tr.refdef.fov_x * M_PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zFar - zNear;

	tr.viewParms.projectionMatrix[0] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = ( xmax + xmin ) / width;	// normally 0
	tr.viewParms.projectionMatrix[12] = 0;

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -( zFar + zNear ) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
}

/*
=================
R_SetupFrustum

Setup that culling frustum planes for the current view
=================
*/
void R_SetupFrustum (void) {
	int		i;
	float	xs, xc;
	float	ang;

	ang = tr.viewParms.fovX / 180 * M_PI * 0.5;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.ori.axis[0], xs, tr.viewParms.frustum[0].normal );
	VectorMA( tr.viewParms.frustum[0].normal, xc, tr.viewParms.ori.axis[1], tr.viewParms.frustum[0].normal );

	VectorScale( tr.viewParms.ori.axis[0], xs, tr.viewParms.frustum[1].normal );
	VectorMA( tr.viewParms.frustum[1].normal, -xc, tr.viewParms.ori.axis[1], tr.viewParms.frustum[1].normal );

	ang = tr.viewParms.fovY / 180 * M_PI * 0.5;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.ori.axis[0], xs, tr.viewParms.frustum[2].normal );
	VectorMA( tr.viewParms.frustum[2].normal, xc, tr.viewParms.ori.axis[2], tr.viewParms.frustum[2].normal );

	VectorScale( tr.viewParms.ori.axis[0], xs, tr.viewParms.frustum[3].normal );
	VectorMA( tr.viewParms.frustum[3].normal, -xc, tr.viewParms.ori.axis[2], tr.viewParms.frustum[3].normal );


	// this is the far plane
	VectorScale( tr.viewParms.ori.axis[0],-1.0f, tr.viewParms.frustum[4].normal );

	for (i=0 ; i<5 ; i++) {
		tr.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[i].dist = DotProduct (tr.viewParms.ori.origin, tr.viewParms.frustum[i].normal);
		if (i==4)
		{
			// far plane does not go through the view point, it goes alot farther..
			tr.viewParms.frustum[i].dist -= tr.distanceCull*1.02f; // a little slack so we don't cull stuff 
		}
		SetPlaneSignbits( &tr.viewParms.frustum[i] );
	}
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
	srfTriangles_t	*tri;
	srfGridMesh_t *grid;
	srfPoly_t		*poly;
	drawVert_t		*v1, *v2, *v3;
	vec4_t			plane4;

	if (!surfType) {
		memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType) {
	case SF_FACE:
		*plane = ((srfSurfaceFace_t *)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri = (srfTriangles_t *)surfType;
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
	case SF_GRID: 
		grid = (srfGridMesh_t *)surfType;
		v1 = &grid->verts[0];
		v2 = &grid->verts[1];
		v3 = &grid->verts[2];
		PlaneFromPoints( plane4, v3->xyz, v2->xyz, v1->xyz );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	default:
		memset (plane, 0, sizeof(*plane));
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
qboolean R_GetPortalOrientations( drawSurf_t *drawSurf, int entityNum, 
							 orientation_t *surface, orientation_t *camera,
							 vec3_t pvsOrigin, qboolean *mirror ) {
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;
	vec3_t		transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != TR_WORLDENT ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.ori );

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
		if ( e->e.frame ) {
			// continuous rotate
			d = (tr.refdef.time/1000.0f) * e->e.frame;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		} else if (e->e.skinNum){
			// bobbing rotate
			//d = 4 * sin( tr.refdef.time * 0.003 );
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

static qboolean IsMirror( const drawSurf_t *drawSurf, int entityNum )
{
	int			i;
	cplane_t	originalPlane, plane;
	trRefEntity_t	*e;
	float		d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != TR_WORLDENT ) 
	{
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.ori );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.ori.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.ori.origin );
	} 
	else 
	{
		plane = originalPlane;
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
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[128] ) {
	float shortest = 1000000000;
	int entityNum;
	int numTriangles;
	shader_t *shader;
	int		fogNum;
	int dlighted;
	vec4_t clip, eye;
	int i;
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	R_RotateForViewer();

	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		int j;
		unsigned int pointFlags = 0;

		R_TransformModelToClip( tess.xyz[i], tr.ori.modelMatrix, tr.viewParms.projectionMatrix, eye, clip );

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
		vec3_t normal;
		float dot;
		float len;

		VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.ori.origin, normal );

		len = VectorLengthSquared( normal );			// lose the sqrt
		if ( len < shortest )
		{
			shortest = len;
		}

		if ( ( dot = DotProduct( normal, tess.normal[tess.indexes[i]] ) ) >= 0 )
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
	if ( IsMirror( drawSurf, entityNum ) )
	{
		return qfalse;
	}

	if ( shortest > (tess.shader->portalRange * tess.shader->portalRange))
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
int	recursivePortalCount;
qboolean R_MirrorViewBySurface (drawSurf_t *drawSurf, int entityNum) {
	vec4_t			clipDest[128];
	viewParms_t		newParms;
	viewParms_t		oldParms;
	orientation_t	surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) 
	{
		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer || r_fastsky->integer ) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
		return qfalse;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = qtrue;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera, 
		newParms.pvsOrigin, &newParms.isMirror ) ) {
		return qfalse;		// bad portal, no portalentity
	}

	R_MirrorPoint (oldParms.ori.origin, &surface, &camera, newParms.ori.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
	
	R_MirrorVector (oldParms.ori.axis[0], &surface, &camera, newParms.ori.axis[0]);
	R_MirrorVector (oldParms.ori.axis[1], &surface, &camera, newParms.ori.axis[1]);
	R_MirrorVector (oldParms.ori.axis[2], &surface, &camera, newParms.ori.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView (&newParms);

	tr.viewParms = oldParms;
	
	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent ) {
	int				i;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	if ( tr.refdef.doLAGoggles )
	{
		return tr.world->numfogs;
	}

	int partialFog = 0;
	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		if ( ent->e.origin[0] - ent->e.radius >= fog->bounds[0][0] 
			&& ent->e.origin[0] + ent->e.radius <= fog->bounds[1][0] 
			&& ent->e.origin[1] - ent->e.radius >= fog->bounds[0][1]
			&& ent->e.origin[1] + ent->e.radius <= fog->bounds[1][1] 
			&& ent->e.origin[2] - ent->e.radius >= fog->bounds[0][2]
			&& ent->e.origin[2] + ent->e.radius <= fog->bounds[1][2] ) 
		{//totally inside it
			return i;
			break;
		}
		if ( ( ent->e.origin[0] - ent->e.radius >= fog->bounds[0][0] && ent->e.origin[1] - ent->e.radius >= fog->bounds[0][1] && ent->e.origin[2] - ent->e.radius >= fog->bounds[0][2] &&
			ent->e.origin[0] - ent->e.radius <= fog->bounds[1][0] && ent->e.origin[1] - ent->e.radius <= fog->bounds[1][1] && ent->e.origin[2] - ent->e.radius <= fog->bounds[1][2] ) ||
			( ent->e.origin[0] + ent->e.radius >= fog->bounds[0][0] && ent->e.origin[1] + ent->e.radius >= fog->bounds[0][1] && ent->e.origin[2] + ent->e.radius >= fog->bounds[0][2] &&
			ent->e.origin[0] + ent->e.radius <= fog->bounds[1][0] && ent->e.origin[1] + ent->e.radius <= fog->bounds[1][1] && ent->e.origin[2] + ent->e.radius <= fog->bounds[1][2] ) )
		{//partially inside it
			if ( tr.refdef.fogIndex == i || R_FogParmsMatch( tr.refdef.fogIndex, i ) )
			{//take new one only if it's the same one that the viewpoint is in
				return i;
				break;
			}
			else if ( !partialFog )
			{//first partialFog
				partialFog = i;
			}
		}
	}

	return partialFog;
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

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( const surfaceType_t *surface, const shader_t *shader, int fogIndex, int dlightMap ) 
{
	int			index;

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;

	if ( tr.refdef.doLAGoggles )
	{
		fogIndex = tr.world->numfogs;
	}

	if ( (shader->surfaceFlags & SURF_FORCESIGHT) && !(tr.refdef.rdflags & RDF_ForceSightOn) )
	{	//if shader is only seen with ForceSight and we don't have ForceSight on, then don't draw
		return;
	}

	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = (shader->sortedIndex << QSORT_SHADERNUM_SHIFT) 
		| tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = (surfaceType_t *)surface;
	tr.refdef.numDrawSurfs++;
}

/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap ) {
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1) ];
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & (MAX_ENTITIES-1);
	*dlightMap = sort & 3;
}

/*
=================
R_SortDrawSurfs
=================
*/
void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;

	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort( drawSurfs, numDrawSurfs );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for ( int i = 0 ; i < numDrawSurfs ; i++ ) {
		R_DecomposeSort( (drawSurfs+i)->sort, &entityNum, &shader, &fogNum, &dlighted );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			Com_Error (ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( (drawSurfs+i), entityNum) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;		// only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

/*
=============
R_AddEntitySurfaces
=============
*/
void R_AddEntitySurfaces (void) {
	trRefEntity_t	*ent;
	shader_t		*shader;

	if ( !r_drawentities->integer ) {
		return;
	}

	for ( tr.currentEntityNum = 0; 
	      tr.currentEntityNum < tr.refdef.num_entities; 
		  tr.currentEntityNum++ ) {
		ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = qfalse;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		if ((ent->e.renderfx & RF_ALPHA_FADE))
		{
			// we need to make sure this is not sorted before the world..in fact we 
			// want this to be sorted quite late...like how about last.
			// I don't want to use the highest bit, since no doubt someone fumbled
			// handling that as an unsigned quantity somewhere
			tr.shiftedEntityNum |= 0x80000000;
		}
		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in 
		// mirrors, because the true body position will already be drawn
		//
		if ( (ent->e.renderfx & RF_FIRST_PERSON) && tr.viewParms.isPortal) {
			continue;
		}


		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType ) {
		case RT_PORTALSURFACE:
			break;		// don't draw anything
		case RT_SPRITE:
		case RT_ORIENTED_QUAD:
		case RT_BEAM:
		case RT_CYLINDER:
		case RT_LATHE:
		case RT_CLOUDS:
		case RT_LINE:
		case RT_ELECTRICITY:
		case RT_SABER_GLOW:
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
				continue;
			}
			shader = R_GetShaderByHandle( ent->e.customShader );
			R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0 );
			break;

		case RT_MODEL:
			// we must set up parts of tr.or for model culling
			R_RotateForEntity( ent, &tr.viewParms, &tr.ori );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if (!tr.currentModel) {
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
			} else {
				switch ( tr.currentModel->type ) {
				case MOD_MESH:
					R_AddMD3Surfaces( ent );
					break;
				case MOD_BRUSH:
					R_AddBrushModelSurfaces( ent );
					break;
/*
Ghoul2 Insert Start
*/

				case MOD_MDXM:
  					R_AddGhoulSurfaces( ent);
  					break;
				case MOD_BAD:		// null model axis
					if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal)
					{
						if (!(ent->e.renderfx & RF_SHADOW_ONLY))
						{
						break;
					}
					}

  					if (ent->e.ghoul2 && G2API_HaveWeGhoul2Models(*((CGhoul2Info_v *)ent->e.ghoul2)))
  					{
  						R_AddGhoulSurfaces( ent);
  						break;
  					}

					R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, false );
					break;
/*
Ghoul2 Insert End
*/
					
				default:
					Com_Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
					break;
				}
			}
			break;
		default:
			Com_Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
		}
	}

}


/*
====================
R_GenerateDrawSurfs
====================
*/
void R_GenerateDrawSurfs( void ) {
	R_AddWorldSurfaces ();

	R_AddPolygonSurfaces();

	R_AddTerrainSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection ();

	R_AddEntitySurfaces ();
}

/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon( int color, int numPoints, float *points ) {
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

	// the render thread can't make callbacks to the main thread
	//R_SyncRenderThread();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri.CM_DrawDebugSurface( R_DebugPolygon );
}

qboolean R_FogParmsMatch( int fog1, int fog2 )
{
	for ( int i = 0; i < 2; i++ )
	{
		if ( tr.world->fogs[fog1].parms.color[i] != tr.world->fogs[fog2].parms.color[i] )
		{
			return qfalse;
		}
	}
	return qtrue;
}

void R_SetViewFogIndex (void)
{
	if ( tr.world->numfogs > 1 )
	{//more than just the LA goggles
		fog_t *fog;
		int contents = ri.SV_PointContents( tr.refdef.vieworg, 0 );
		if ( (contents&CONTENTS_FOG) )
		{//only take a tr.refdef.fogIndex if the tr.refdef.vieworg is actually *in* that fog brush (assumption: checks pointcontents for any CONTENTS_FOG, not that particular brush...)
			for ( tr.refdef.fogIndex = 1 ; tr.refdef.fogIndex < tr.world->numfogs ; tr.refdef.fogIndex++ ) 
			{
				fog = &tr.world->fogs[tr.refdef.fogIndex]; 
				if ( tr.refdef.vieworg[0] >= fog->bounds[0][0]
					&& tr.refdef.vieworg[1] >= fog->bounds[0][1]
					&& tr.refdef.vieworg[2] >= fog->bounds[0][2]
					&& tr.refdef.vieworg[0] <= fog->bounds[1][0]
					&& tr.refdef.vieworg[1] <= fog->bounds[1][1]
					&& tr.refdef.vieworg[2] <= fog->bounds[1][2] ) 
				{
					break;
				}
			}
			if ( tr.refdef.fogIndex == tr.world->numfogs ) 
			{
				tr.refdef.fogIndex = 0;
			}
		}
		else
		{
			tr.refdef.fogIndex = 0;
		}
	}
	else
	{
		tr.refdef.fogIndex = 0;
	}
}
void RE_SetLightStyle(int style, int colors );

/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView (viewParms_t *parms) {
	int		firstDrawSurf;

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	if (r_debugStyle->integer >= 0)
	{
		int			i;
		color4ub_t	whitecolor = {0xff, 0xff, 0xff, 0xff};
		color4ub_t	blackcolor = {0x00, 0x00, 0x00, 0xff};

		for(i=0;i<MAX_LIGHT_STYLES;i++)
		{
			RE_SetLightStyle(i,  *(int*)blackcolor);
		}
		RE_SetLightStyle(r_debugStyle->integer,  *(int*)whitecolor);
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer ();

	R_SetupFrustum ();

	if (!(tr.refdef.rdflags & RDF_NOWORLDMODEL)) 
	{	// Trying to do this with no world is not good.
		R_SetViewFogIndex ();
	}

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );

	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}