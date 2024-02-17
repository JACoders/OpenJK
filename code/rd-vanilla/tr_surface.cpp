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

// tr_surf.c

#include "../server/exe_headers.h"

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/


//============================================================================


/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow( int verts, int indexes ) {
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES
		&& tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		Com_Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		Com_Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum );
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 ) {
	vec3_t		normal;
	int			ndx;

	RB_CHECKOVERFLOW( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx+1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx+1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx+1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx+2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx+2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx+2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx+3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx+3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx+3][2] = origin[2] + left[2] - up[2];


	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.ori.axis[0], normal );

	tess.normal[ndx][0] = tess.normal[ndx+1][0] = tess.normal[ndx+2][0] = tess.normal[ndx+3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx+1][1] = tess.normal[ndx+2][1] = tess.normal[ndx+3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx+1][2] = tess.normal[ndx+2][2] = tess.normal[ndx+3][2] = normal[2];

	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx+1][0][0] = tess.texCoords[ndx+1][1][0] = s2;
	tess.texCoords[ndx+1][0][1] = tess.texCoords[ndx+1][1][1] = t1;

	tess.texCoords[ndx+2][0][0] = tess.texCoords[ndx+2][1][0] = s2;
	tess.texCoords[ndx+2][0][1] = tess.texCoords[ndx+2][1][1] = t2;

	tess.texCoords[ndx+3][0][0] = tess.texCoords[ndx+3][1][0] = s1;
	tess.texCoords[ndx+3][0][1] = tess.texCoords[ndx+3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	byteAlias_t *baSource = (byteAlias_t *)color, *baDest;
	baDest = (byteAlias_t *)&tess.vertexColors[ndx + 0];
	baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[ndx + 1];
	baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[ndx + 2];
	baDest->ui = baSource->ui;
	baDest = (byteAlias_t *)&tess.vertexColors[ndx + 3];
	baDest->ui = baSource->ui;

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void ) {
	vec3_t		left, up;
	float		radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;
	if ( backEnd.currentEntity->e.rotation == 0 ) {
		VectorScale( backEnd.viewParms.ori.axis[1], radius, left );
		VectorScale( backEnd.viewParms.ori.axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;

		ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.ori.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.ori.axis[2], left );

		VectorScale( backEnd.viewParms.ori.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.ori.axis[1], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

/*
=======================
RB_SurfaceOrientedQuad
=======================
*/
static void RB_SurfaceOrientedQuad( void )
{
	vec3_t	left, up;
	float	radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;
	MakeNormalVectors( backEnd.currentEntity->e.axis[0], left, up );

	if ( backEnd.currentEntity->e.rotation == 0 )
	{
		VectorScale( left, radius, left );
		VectorScale( up, radius, up );
	}
	else
	{
		vec3_t	tempLeft, tempUp;
		float	s, c;
		float	ang;

		ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		// Use a temp so we don't trash the values we'll need later
		VectorScale( left, c * radius, tempLeft );
		VectorMA( tempLeft, -s * radius, up, tempLeft );

		VectorScale( up, c * radius, tempUp );
		VectorMA( tempUp, s * radius, left, up ); // no need to use the temp anymore, so copy into the dest vector ( up )

		// This was copied for safekeeping, we're done, so we can move it back to left
		VectorCopy( tempLeft, left );
	}

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

/*
==============
RB_SurfaceLine
==============
*/
//
//	Values for a proper line render primitive...
//		Width
//		STScale (how many times to loop a texture)
//		alpha
//		RGB
//
//  Values for proper line object...
//		lifetime
//		dscale
//		startalpha, endalpha
//		startRGB, endRGB
//

static void DoLine( const vec3_t start, const vec3_t end, const vec3_t up, float spanWidth )
{
	float		spanWidth2;
	int			vbase;

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	spanWidth2 = -spanWidth;

	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];// * 0.25;//wtf??not sure why the code would be doing this
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];// * 0.25;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];// * 0.25;
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];// * 0.25;
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void DoLine2( const vec3_t start, const vec3_t end, const vec3_t up, float spanWidth, float spanWidth2, const float tcStart, const float tcEnd )
{
	int			vbase;

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = tcStart;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];// * 0.25;//wtf??not sure why the code would be doing this
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];// * 0.25;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];// * 0.25;
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];// * 0.25;
	tess.numVertexes++;

	VectorMA( start, -spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = tcStart;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = tcEnd;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	VectorMA( end, -spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = tcEnd;//backEnd.currentEntity->e.shaderTexCoord[1];
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = backEnd.currentEntity->e.shaderRGBA[3];
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

//-----------------
// RB_SurfaceLine
//-----------------
static void RB_SurfaceLine( void )
{
	refEntity_t *e;
	vec3_t		right;
	vec3_t		start, end;
	vec3_t		v1, v2;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, end );
	VectorCopy( e->origin, start );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.ori.origin, v1 );
	VectorSubtract( end, backEnd.viewParms.ori.origin, v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	DoLine( start, end, right, e->radius);
}

/*
==============
RB_SurfaceCylinder
==============
*/

#define NUM_CYLINDER_SEGMENTS 40

// e->origin holds the bottom point
// e->oldorigin holds the top point
// e->radius holds the radius

// If a cylinder has a tapered end that has a very small radius, the engine converts it to a cone.  Not a huge savings, but the texture mapping is slightly better
//	and it uses half as many indicies as the cylinder version
//-------------------------------------
static void RB_SurfaceCone( void )
//-------------------------------------
{
	static vec3_t points[NUM_CYLINDER_SEGMENTS];
	vec3_t		vr, vu, midpoint;
	vec3_t		tapered, base;
	float		detail, length;
	int			i;
	int			segments;
	refEntity_t *e;

	e = &backEnd.currentEntity->e;

	//Work out the detail level of this cylinder
	VectorAdd( e->origin, e->oldorigin, midpoint );
	VectorScale(midpoint, 0.5, midpoint);		// Average start and end

	VectorSubtract( midpoint, backEnd.viewParms.ori.origin, midpoint );
	length = VectorNormalize( midpoint );

	// this doesn't need to be perfect....just a rough compensation for zoom level is enough
	length *= (backEnd.viewParms.fovX / 90.0f);

	detail = 1 - ((float) length / 2048 );
	segments = NUM_CYLINDER_SEGMENTS * detail;

	// 3 is the absolute minimum, but the pop between 3-8 is too noticeable
	if ( segments < 8 )
	{
		segments = 8;
	}

	if ( segments > NUM_CYLINDER_SEGMENTS )
	{
		segments = NUM_CYLINDER_SEGMENTS;
	}

	// Get the direction vector
	MakeNormalVectors( e->axis[0], vr, vu );

	// we only need to rotate around the larger radius, the smaller radius get's welded
	if ( e->radius < e->backlerp )
	{
		VectorScale( vu, e->backlerp, vu );
		VectorCopy( e->origin, base );
		VectorCopy( e->oldorigin, tapered );
	}
	else
	{
		VectorScale( vu, e->radius, vu );
		VectorCopy( e->origin, tapered );
		VectorCopy( e->oldorigin, base );
	}


	// Calculate the step around the cylinder
	detail = 360.0f / (float)segments;

	for ( i = 0; i < segments; i++ )
	{
		// ring
		RotatePointAroundVector( points[i], e->axis[0], vu, detail * i );
		VectorAdd( points[i], base, points[i] );
	}

	// Calculate the texture coords so the texture can wrap around the whole cylinder
	detail = 1.0f / (float)segments;

	RB_CHECKOVERFLOW( 2 * (segments+1), 3 * segments ); // this isn't 100% accurate

	int vbase = tess.numVertexes;

	for ( i = 0; i < segments; i++ )
	{
 		VectorCopy( points[i], tess.xyz[tess.numVertexes] );
		tess.texCoords[tess.numVertexes][0][0] = detail * i;
		tess.texCoords[tess.numVertexes][0][1] = 1.0f;
		tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
		tess.numVertexes++;

		// We could add this vert once, but using the given texture mapping method, we need to generate different texture coordinates
		VectorCopy( tapered, tess.xyz[tess.numVertexes] );
		tess.texCoords[tess.numVertexes][0][0] = detail * i + detail * 0.5f; // set the texture coordinates to the point half-way between the untapered ends....but on the other end of the texture
		tess.texCoords[tess.numVertexes][0][1] = 0.0f;
		tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
		tess.numVertexes++;
	}

	// last point has the same verts as the first, but does not share the same tex coords, so we have to duplicate it
 	VectorCopy( points[0], tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = detail * i;
	tess.texCoords[tess.numVertexes][0][1] = 1.0f;
	tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
	tess.numVertexes++;

 	VectorCopy( tapered, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = detail * i + detail * 0.5f;
	tess.texCoords[tess.numVertexes][0][1] = 0.0f;
	tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
	tess.numVertexes++;

	// do the welding
	for ( i = 0; i < segments; i++ )
	{
		tess.indexes[tess.numIndexes++] = vbase;
		tess.indexes[tess.numIndexes++] = vbase + 1;
		tess.indexes[tess.numIndexes++] = vbase + 2;

		vbase += 2;
	}
}

//-------------------------------------
static void RB_SurfaceCylinder( void )
//-------------------------------------
{
	static vec3_t lower_points[NUM_CYLINDER_SEGMENTS], upper_points[NUM_CYLINDER_SEGMENTS];
	vec3_t		vr, vu, midpoint, v1;
	float		detail, length;
	int			i;
	int			segments;
	refEntity_t *e;

	e = &backEnd.currentEntity->e;

	// check for tapering
	if ( !( e->radius < 0.3f && e->backlerp < 0.3f) && ( e->radius < 0.3f || e->backlerp < 0.3f ))
	{
		// One end is sufficiently tapered to consider changing it to a cone
		RB_SurfaceCone();
		return;
	}

	//Work out the detail level of this cylinder
	VectorAdd( e->origin, e->oldorigin, midpoint );
	VectorScale(midpoint, 0.5, midpoint);		// Average start and end

	VectorSubtract( midpoint, backEnd.viewParms.ori.origin, midpoint );
	length = VectorNormalize( midpoint );

	// this doesn't need to be perfect....just a rough compensation for zoom level is enough
	length *= (backEnd.viewParms.fovX / 90.0f);

	detail = 1 - ((float) length / 2048 );
	segments = NUM_CYLINDER_SEGMENTS * detail;

	// 3 is the absolute minimum, but the pop between 3-8 is too noticeable
	if ( segments < 8 )
	{
		segments = 8;
	}

	if ( segments > NUM_CYLINDER_SEGMENTS )
	{
		segments = NUM_CYLINDER_SEGMENTS;
	}

	//Get the direction vector
	MakeNormalVectors( e->axis[0], vr, vu );

	VectorScale( vu, e->radius, v1 );	// size1
	VectorScale( vu, e->backlerp, vu );	// size2

	// Calculate the step around the cylinder
	detail = 360.0f / (float)segments;

	for ( i = 0; i < segments; i++ )
	{
		//Upper ring
		RotatePointAroundVector( upper_points[i], e->axis[0], vu, detail * i );
		VectorAdd( upper_points[i], e->origin, upper_points[i] );

		//Lower ring
		RotatePointAroundVector( lower_points[i], e->axis[0], v1, detail * i );
		VectorAdd( lower_points[i], e->oldorigin, lower_points[i] );
	}

	// Calculate the texture coords so the texture can wrap around the whole cylinder
	detail = 1.0f / (float)segments;

	RB_CHECKOVERFLOW( 2 * (segments+1), 6 * segments ); // this isn't 100% accurate

	int vbase = tess.numVertexes;

	for ( i = 0; i < segments; i++ )
	{
 		VectorCopy( upper_points[i], tess.xyz[tess.numVertexes] );
		tess.texCoords[tess.numVertexes][0][0] = detail * i;
		tess.texCoords[tess.numVertexes][0][1] = 1.0f;
		tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
		tess.numVertexes++;

		VectorCopy( lower_points[i], tess.xyz[tess.numVertexes] );
		tess.texCoords[tess.numVertexes][0][0] = detail * i;
		tess.texCoords[tess.numVertexes][0][1] = 0.0f;
		tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
		tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
		tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
		tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
		tess.numVertexes++;
	}

	// last point has the same verts as the first, but does not share the same tex coords, so we have to duplicate it
 	VectorCopy( upper_points[0], tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = detail * i;
	tess.texCoords[tess.numVertexes][0][1] = 1.0f;
	tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
	tess.numVertexes++;

 	VectorCopy( lower_points[0], tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = detail * i;
	tess.texCoords[tess.numVertexes][0][1] = 0.0f;
	tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
	tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
	tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
	tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
	tess.numVertexes++;

	// glue the verts
	for ( i = 0; i < segments; i++ )
	{
		tess.indexes[tess.numIndexes++] = vbase;
		tess.indexes[tess.numIndexes++] = vbase + 1;
		tess.indexes[tess.numIndexes++] = vbase + 2;

		tess.indexes[tess.numIndexes++] = vbase + 2;
		tess.indexes[tess.numIndexes++] = vbase + 1;
		tess.indexes[tess.numIndexes++] = vbase + 3;

		vbase += 2;
	}
}

static vec3_t sh1, sh2;
static int f_count;

// Up front, we create a random "shape", then apply that to each line segment...and then again to each of those segments...kind of like a fractal
//----------------------------------------------------------------------------
static void CreateShape()
//----------------------------------------------------------------------------
{
	VectorSet( sh1, 0.66f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
				0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f,
				0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f );

	// it seems to look best to have a point on one side of the ideal line, then the other point on the other side.
	VectorSet( sh2, 0.33f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
					-sh1[1] + Q_flrand(-1.0f, 1.0f) * 0.02f,	// forcing point to be on the opposite side of the line -- right
					-sh1[2] + Q_flrand(-1.0f, 1.0f) * 0.02f );// up
}

//----------------------------------------------------------------------------
static void ApplyShape( vec3_t start, vec3_t end, vec3_t right, float sradius, float eradius, int count, float startPerc, float endPerc )
//----------------------------------------------------------------------------
{
	vec3_t	point1, point2, fwd;
	vec3_t	rt, up;
	float	perc, dis;

    if ( count < 1 )
	{
		// done recursing
		DoLine2( start, end, right, sradius, eradius, startPerc, endPerc );
		return;
	}

    CreateShape();

	VectorSubtract( end, start, fwd );
	dis = VectorNormalize( fwd ) * 0.7f;
	MakeNormalVectors( fwd, rt, up );

	perc = sh1[0];

	VectorScale( start, perc, point1 );
	VectorMA( point1, 1.0f - perc, end, point1 );
	VectorMA( point1, dis * sh1[1], rt, point1 );
	VectorMA( point1, dis * sh1[2], up, point1 );

	// do a quick and dirty interpolation of the radius at that point
	float rads1, rads2;

	rads1 = sradius * 0.666f + eradius * 0.333f;
	rads2 = sradius * 0.333f + eradius * 0.666f;

	// recursion
    ApplyShape( start, point1, right, sradius, rads1, count - 1, startPerc, startPerc * 0.666f + endPerc * 0.333f );

	perc = sh2[0];

	VectorScale( start, perc, point2 );
	VectorMA( point2, 1.0f - perc, end, point2 );
	VectorMA( point2, dis * sh2[1], rt, point2 );
	VectorMA( point2, dis * sh2[2], up, point2 );

	// recursion
    ApplyShape( point2, point1, right, rads1, rads2, count - 1, startPerc * 0.333f + endPerc * 0.666f, startPerc * 0.666f + endPerc * 0.333f );
	ApplyShape( point2, end, right, rads2, eradius, count - 1, startPerc * 0.333f + endPerc * 0.666f, endPerc );
}

//----------------------------------------------------------------------------
static void DoBoltSeg( vec3_t start, vec3_t end, vec3_t right, float radius )
//----------------------------------------------------------------------------
{
	refEntity_t *e;
	vec3_t fwd, old;
	vec3_t cur, off={10,10,10};
	vec3_t rt, up;
	vec3_t temp;
	int		i;
    float dis, oldPerc = 0.0f, perc, oldRadius, newRadius;

	e = &backEnd.currentEntity->e;

	VectorSubtract( end, start, fwd );
	dis = VectorNormalize( fwd );

	if (dis > 2000)	//freaky long
	{
//		ri.Printf( PRINT_WARNING, "DoBoltSeg: insane distance.\n" );
		dis = 2000;
	}
	MakeNormalVectors( fwd, rt, up );

	VectorCopy( start, old );

	newRadius = oldRadius = radius;

    for ( i = 16; i <= dis; i+= 16 )
	{
		// because of our large step size, we may not actually draw to the end.  In this case, fudge our percent so that we are basically complete
		if ( i + 16 > dis )
		{
			perc = 1.0f;
		}
		else
		{
			// percentage of the amount of line completed
			perc = (float)i / dis;
		}

		// create our level of deviation for this point
		VectorScale( fwd, Q_crandom(&e->frame) * 3.0f, temp );				// move less in fwd direction, chaos also does not affect this
		VectorMA( temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], rt, temp );	// move more in direction perpendicular to line, angles is really the chaos
		VectorMA( temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], up, temp );	// move more in direction perpendicular to line

		// track our total level of offset from the ideal line
		VectorAdd( off, temp, off );

        // Move from start to end, always adding our current level of offset from the ideal line
		//	Even though we are adding a random offset.....by nature, we always move from exactly start....to end
		VectorAdd( start, off, cur );
		VectorScale( cur, 1.0f - perc, cur );
		VectorMA( cur, perc, end, cur );

		if ( e->renderfx & RF_TAPERED )
		{
			// This does pretty close to perfect tapering since apply shape interpolates the old and new as it goes along.
			//	by using one minus the square, the radius stays fairly constant, then drops off quickly at the very point of the bolt
			oldRadius = radius * (1.0f-oldPerc*oldPerc);
			newRadius = radius * (1.0f-perc*perc);
		}

		// Apply the random shape to our line seg to give it some micro-detail-jaggy-coolness.
		ApplyShape( cur, old, right, newRadius, oldRadius, 2 - r_lodbias->integer, 0, 1 );

		// randomly split off to create little tendrils, but don't do it too close to the end and especially if we are not even of the forked variety
        if (( e->renderfx & RF_FORKED ) && f_count > 0 && Q_random(&e->frame) > 0.93f && (1.0f - perc) > 0.8f )
		{
			vec3_t newDest;

			f_count--;

			// Pick a point somewhere between the current point and the final endpoint
			VectorAdd( cur, e->oldorigin, newDest );
			VectorScale( newDest, 0.5f, newDest );

			// And then add some crazy offset
			for ( int t = 0; t < 3; t++ )
			{
				newDest[t] += Q_crandom(&e->frame) * 80;
			}

			// we could branch off using OLD and NEWDEST, but that would allow multiple forks...whereas, we just want simpler brancing
            DoBoltSeg( cur, newDest, right, newRadius );
		}

		// Current point along the line becomes our new old attach point
		VectorCopy( cur, old );
		oldPerc = perc;
	}
}

//------------------------------------------
static void RB_SurfaceElectricity()
//------------------------------------------
{
	refEntity_t *e;
	vec3_t		right, fwd;
	vec3_t		start, end;
	vec3_t		v1, v2;
	float		radius, perc = 1.0f, dis;

	e = &backEnd.currentEntity->e;
	radius = e->radius;

	VectorCopy( e->origin, start );

	VectorSubtract( e->oldorigin, start, fwd );
	dis = VectorNormalize( fwd );

	// see if we should grow from start to end
	if ( e->renderfx & RF_GROW )
	{
		perc = 1.0f - ( e->endTime - tr.refdef.time ) / e->angles[1]/*duration*/;

		if ( perc > 1.0f )
		{
			perc = 1.0f;
		}
		else if ( perc < 0.0f )
		{
			perc = 0.0f;
		}
	}

	VectorMA( start, perc * dis, fwd, e->oldorigin );
	VectorCopy( e->oldorigin, end );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.ori.origin, v1 );
	VectorSubtract( end, backEnd.viewParms.ori.origin, v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	// allow now more than three branches on branch type electricity
	f_count = 3;
    DoBoltSeg( start, end, right, radius );
}

/*
=============
RB_SurfacePolychain
=============
*/
/* // we could try to do something similar to this to get better normals into the tess for these types of surfs.  As it stands, any shader pass that
//	requires a normal ( env map ) will not work properly since the normals seem to essentially be random garbage.
void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;
	vec3_t	a,b,normal={1,0,0};

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	if ( p->numVerts >= 3 )
	{
		VectorSubtract( p->verts[0].xyz, p->verts[1].xyz, a );
		VectorSubtract( p->verts[2].xyz, p->verts[1].xyz, b );
		CrossProduct( a,b, normal );
		VectorNormalize( normal );
	}

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		VectorCopy( normal, tess.normal[numv] );
		*(int *)&tess.vertexColors[numv] = *(int *)p->verts[ i ].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}
*/
void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		byteAlias_t *baDest = (byteAlias_t *)&tess.vertexColors[numv++],
			*baSource = (byteAlias_t *)&p->verts[ i ].modulate;
		baDest->i = baSource->i;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

inline static uint32_t ComputeFinalVertexColor( const byte *colors ) {
	int k;
	byteAlias_t result;
	uint32_t r, g, b;

	for ( k=0; k<4; k++ )
		result.b[k] = colors[k];

	if ( tess.shader->lightmapIndex[0] != LIGHTMAP_BY_VERTEX )
		return result.ui;

	if ( r_fullbright->integer ) {
		result.b[0] = 255;
		result.b[1] = 255;
		result.b[2] = 255;
		return result.ui;
	}
	// an optimization could be added here to compute the style[0] (which is always the world normal light)
	r = g = b = 0;
	for( k=0; k<MAXLIGHTMAPS; k++ ) {
		if ( tess.shader->styles[k] < LS_UNUSED ) {
			byte *styleColor = styleColors[tess.shader->styles[k]];

			r += (uint32_t)(*colors++) * (uint32_t)(*styleColor++);
			g += (uint32_t)(*colors++) * (uint32_t)(*styleColor++);
			b += (uint32_t)(*colors++) * (uint32_t)(*styleColor);
			colors++;
		}
		else
			break;
	}
	result.b[0] = Com_Clamp( 0, 255, r >> 8 );
	result.b[1] = Com_Clamp( 0, 255, g >> 8 );
	result.b[2] = Com_Clamp( 0, 255, b >> 8 );

	return result.ui;
}

/*
=============
RB_SurfaceTriangles
=============
*/
void RB_SurfaceTriangles( srfTriangles_t *srf ) {
	int			i, k;
	drawVert_t	*dv;
	float		*xyz, *normal, *texCoords;
	byte		*color;
	int			dlightBits;

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	RB_CHECKOVERFLOW( srf->numVerts, srf->numIndexes );

	for ( i = 0 ; i < srf->numIndexes ; i += 3 ) {
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}
	tess.numIndexes += srf->numIndexes;

	dv = srf->verts;
	xyz = tess.xyz[ tess.numVertexes ];
	normal = tess.normal[ tess.numVertexes ];
	texCoords = tess.texCoords[ tess.numVertexes ][0];
	color = tess.vertexColors[ tess.numVertexes ];

	for ( i = 0 ; i < srf->numVerts ; i++, dv++)
	{
		xyz[0] = dv->xyz[0];
		xyz[1] = dv->xyz[1];
		xyz[2] = dv->xyz[2];
		xyz += 4;

		//if ( needsNormal )
		{
			normal[0] = dv->normal[0];
			normal[1] = dv->normal[1];
			normal[2] = dv->normal[2];
		}
		normal += 4;

		texCoords[0] = dv->st[0];
		texCoords[1] = dv->st[1];

		for(k=0;k<MAXLIGHTMAPS;k++)
		{
			if (tess.shader->lightmapIndex[k] >= 0)
			{
				texCoords[2+(k*2)] = dv->lightmap[k][0];
				texCoords[2+(k*2)+1] = dv->lightmap[k][1];
			}
			else
			{	// can't have an empty slot in the middle, so we are done
				break;
			}
		}
		texCoords += NUM_TEX_COORDS*2;

		*(unsigned *)color = ComputeFinalVertexColor((byte *)dv->color);
		color += 4;
	}

	for ( i = 0 ; i < srf->numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i] = dlightBits;
	}

	tess.numVertexes += srf->numVerts;
}



/*
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam( void )
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	int	i;
	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	e = &backEnd.currentEntity->e;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS ; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
//		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	switch(e->skinNum)
	{
	case 1://Green
		qglColor3f( 0, 1, 0 );
		break;
	case 2://Blue
		qglColor3f( 0.5, 0.5, 1 );
		break;
	case 0://red
	default:
		qglColor3f( 1, 0, 0 );
		break;
	}

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		qglVertex3fv( start_points[ i % NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[ i % NUM_BEAM_SEGS] );
	}
	qglEnd();
}


//------------------
// DoSprite
//------------------
static void DoSprite( vec3_t origin, float radius, float rotation )
{
	float	s, c;
	float	ang;
	vec3_t	left, up;

	ang = M_PI * rotation / 180.0f;
	s = sin( ang );
	c = cos( ang );

	VectorScale( backEnd.viewParms.ori.axis[1], c * radius, left );
	VectorMA( left, -s * radius, backEnd.viewParms.ori.axis[2], left );

	VectorScale( backEnd.viewParms.ori.axis[2], c * radius, up );
	VectorMA( up, s * radius, backEnd.viewParms.ori.axis[1], up );

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

//------------------
// RB_SurfaceSaber
//------------------
static void RB_SurfaceSaberGlow()
{
	vec3_t		end;
	refEntity_t *e;

	e = &backEnd.currentEntity->e;

	// Render the glow part of the blade
	for ( float i = e->saberLength; i > 0; i -= e->radius * 0.65f )
	{
		VectorMA( e->origin, i, e->axis[0], end );

		DoSprite( end, e->radius, 0.0f );//Q_flrand(0.0f, 1.0f) * 360.0f );
		e->radius += 0.017f;
	}

	// Big hilt sprite
	// Please don't kill me Pat...I liked the hilt glow blob, but wanted a subtle pulse.:)  Feel free to ditch it if you don't like it.  --Jeff
	// Please don't kill me Jeff...  The pulse is good, but now I want the halo bigger if the saber is shorter...  --Pat
	DoSprite( e->origin, 5.5f + Q_flrand(0.0f, 1.0f) * 0.25f, 0.0f );//Q_flrand(0.0f, 1.0f) * 360.0f );
}

/*
** LerpMeshVertexes
*/
static void LerpMeshVertexes (md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale, newXyzScale;
	float	oldNormalScale, newNormalScale;
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4)
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )
			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4)
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

			VectorNormalize (outNormal);
		}
	}
}

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh(md3Surface_t *surface) {
	int				j;
	float			backlerp;
	int				*triangles;
	float			*texCoords;
	int				indexes;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles*3 );

	LerpMeshVertexes (surface, backlerp);

	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[Bob + j] = Doug + triangles[j];
	}
	tess.numIndexes += indexes;

	texCoords = (float *) ((byte *)surface + surface->ofsSt);

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j*2+0];
		tess.texCoords[Doug + j][0][1] = texCoords[j*2+1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}


/*
==============
RB_SurfaceFace
==============
*/
void RB_SurfaceFace( srfSurfaceFace_t *surf ) {
	int			i, j, k;
	unsigned int *indices;
	glIndex_t	*tessIndexes;
	float		*v;
	float		*normal;
	int			ndx;
	int			Bob;
	int			numPoints;
	int			dlightBits;
	byteAlias_t	ba;

	RB_CHECKOVERFLOW( surf->numPoints, surf->numIndices );

	dlightBits = surf->dlightBits;
	tess.dlightBits |= dlightBits;

	indices = ( unsigned * ) ( ( ( char  * ) surf ) + surf->ofsIndices );

	Bob = tess.numVertexes;
	tessIndexes = tess.indexes + tess.numIndexes;
	for ( i = surf->numIndices-1 ; i >= 0  ; i-- ) {
		tessIndexes[i] = indices[i] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	v = surf->points[0];

	ndx = tess.numVertexes;

	numPoints = surf->numPoints;

	//if ( tess.shader->needsNormal )
	{
		normal = surf->plane.normal;
		for ( i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ ) {
			VectorCopy( normal, tess.normal[ndx] );
		}
	}

	for ( i = 0, v = surf->points[0], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++ ) {
		VectorCopy( v, tess.xyz[ndx]);
		tess.texCoords[ndx][0][0] = v[3];
		tess.texCoords[ndx][0][1] = v[4];
		for(k=0;k<MAXLIGHTMAPS;k++)
		{
			if (tess.shader->lightmapIndex[k] >= 0)
			{
				tess.texCoords[ndx][k+1][0] = v[VERTEX_LM+(k*2)];
				tess.texCoords[ndx][k+1][1] = v[VERTEX_LM+(k*2)+1];
			}
			else
			{
				break;
			}
		}
		ba.ui = ComputeFinalVertexColor( (byte *)&v[VERTEX_COLOR] );
		for ( j=0; j<4; j++ )
			tess.vertexColors[ndx][j] = ba.b[j];
		tess.vertexDlightBits[ndx] = dlightBits;
	}

	tess.numVertexes += surf->numPoints;
}


static float LodErrorForVolume( vec3_t local, float radius ) {
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.ori.axis[0][0] + local[1] * backEnd.ori.axis[1][0] +
		local[2] * backEnd.ori.axis[2][0] + backEnd.ori.origin[0];
	world[1] = local[0] * backEnd.ori.axis[0][1] + local[1] * backEnd.ori.axis[1][1] +
		local[2] * backEnd.ori.axis[2][1] + backEnd.ori.origin[1];
	world[2] = local[0] * backEnd.ori.axis[0][2] + local[1] * backEnd.ori.axis[1][2] +
		local[2] * backEnd.ori.axis[2][2] + backEnd.ori.origin[2];

	VectorSubtract( world, backEnd.viewParms.ori.origin, world );
	d = DotProduct( world, backEnd.viewParms.ori.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
void RB_SurfaceGrid( srfGridMesh_t *cv ) {
	int		i, j, k;
	float	*xyz;
	float	*texCoords;
	float	*normal;
	unsigned char *color;
	drawVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int		*vDlightBits;

	dlightBits = cv->dlightBits;
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < cv->width-1 ; i++ ) {
		if ( cv->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < cv->height-1 ; i++ ) {
		if ( cv->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	rows = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum );
			} else {
				break;
			}
		} while ( 1 );

		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[numVertexes];
		normal = tess.normal[numVertexes];
		texCoords = tess.texCoords[numVertexes][0];
		color = ( unsigned char * ) &tess.vertexColors[numVertexes];
		vDlightBits = &tess.vertexDlightBits[numVertexes];

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = cv->verts + heightTable[ used + i ] * cv->width
					+ widthTable[ j ];

				xyz[0] = dv->xyz[0];
				xyz[1] = dv->xyz[1];
				xyz[2] = dv->xyz[2];
				xyz += 4;

				texCoords[0] = dv->st[0];
				texCoords[1] = dv->st[1];

				for(k=0;k<MAXLIGHTMAPS;k++)
				{
					texCoords[2+(k*2)]= dv->lightmap[k][0];
					texCoords[2+(k*2)+1]= dv->lightmap[k][1];
				}
				texCoords += NUM_TEX_COORDS*2;

//				if ( needsNormal )
				{
					normal[0] = dv->normal[0];
					normal[1] = dv->normal[1];
					normal[2] = dv->normal[2];
				}
				normal += 4;
				*(unsigned *)color = ComputeFinalVertexColor((byte *)dv->color);
				color += 4;
				*vDlightBits++ = dlightBits;
			}
		}

		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++) {
					int		v1, v2, v3, v4;

					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes] = v2;
					tess.indexes[numIndexes+1] = v3;
					tess.indexes[numIndexes+2] = v1;

					tess.indexes[numIndexes+3] = v1;
					tess.indexes[numIndexes+4] = v3;
					tess.indexes[numIndexes+5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}

#define LATHE_SEG_STEP	10
#define BEZIER_STEP		0.05f	// must be in the range of 0 to 1

// FIXME: This function is horribly expensive
static void RB_SurfaceLathe()
{
	refEntity_t *e;
	vec2_t		pt, oldpt, l_oldpt;
	vec2_t		pt2, oldpt2, l_oldpt2;
	float		bezierStep, latheStep;
    float		temp, mu, mum1;
	float		mum13, mu3, group1, group2;
	float		s, c, d = 1.0f, pain = 0.0f;
	int			i, t, vbase;

	e = &backEnd.currentEntity->e;

	if ( e->endTime && e->endTime > backEnd.refdef.time )
	{
		d = 1.0f - ( e->endTime - backEnd.refdef.time ) / 1000.0f;
	}

	if ( e->frame && e->frame + 1000 > backEnd.refdef.time )
	{
		pain = ( backEnd.refdef.time - e->frame ) / 1000.0f;
//		pain *= pain;
		pain = ( 1.0f - pain ) * 0.08f;
	}

	VectorSet2( l_oldpt, e->axis[0][0], e->axis[0][1] );

	// do scalability stuff...r_lodbias 0-3
	int lod = r_lodbias->integer + 1;
	if ( lod > 4 )
	{
		lod = 4;
	}
	if ( lod < 1 )
	{
		lod = 1;
	}
	bezierStep = BEZIER_STEP * lod;
	latheStep = LATHE_SEG_STEP * lod;

	// Do bezier profile strip, then lathe this around to make a 3d model
	for ( mu = 0.0f; mu <= 1.01f * d; mu += bezierStep )
	{
		// Four point curve
		mum1	= 1 - mu;
		mum13	= mum1 * mum1 * mum1;
		mu3		= mu * mu * mu;
		group1	= 3 * mu * mum1 * mum1;
		group2	= 3 * mu * mu *mum1;

		// Calc the current point on the curve
		for ( i = 0; i < 2; i++ )
		{
			l_oldpt2[i] = mum13 * e->axis[0][i] + group1 * e->axis[1][i] + group2 * e->axis[2][i] + mu3 * e->oldorigin[i];
		}

		VectorSet2( oldpt, l_oldpt[0], 0 );
		VectorSet2( oldpt2, l_oldpt2[0], 0 );

		// lathe patch section around in a complete circle
		for ( t = latheStep; t <= 360; t += latheStep )
		{
			VectorSet2( pt, l_oldpt[0], 0 );
			VectorSet2( pt2, l_oldpt2[0], 0 );

			s = sin( DEG2RAD( t ));
			c = cos( DEG2RAD( t ));

			// rotate lathe points
//c -s 0
//s  c 0
//0  0 1
			temp = c * pt[0] - s * pt[1];
			pt[1] = s * pt[0] + c * pt[1];
			pt[0] = temp;
			temp = c * pt2[0] - s * pt2[1];
			pt2[1] = s * pt2[0] + c * pt2[1];
			pt2[0] = temp;

			RB_CHECKOVERFLOW( 4, 6 );

			vbase = tess.numVertexes;

			// Actually generate the necessary verts
			VectorSet( tess.normal[tess.numVertexes], oldpt[0], oldpt[1], l_oldpt[1] );
			VectorAdd( e->origin, tess.normal[tess.numVertexes], tess.xyz[tess.numVertexes] );
			VectorNormalize( tess.normal[tess.numVertexes] );
			i = oldpt[0] * 0.1f + oldpt[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = (t-latheStep)/360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu-bezierStep + cos( i + backEnd.refdef.floatTime ) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet( tess.normal[tess.numVertexes], oldpt2[0], oldpt2[1], l_oldpt2[1] );
			VectorAdd( e->origin, tess.normal[tess.numVertexes], tess.xyz[tess.numVertexes] );
			VectorNormalize( tess.normal[tess.numVertexes] );
			i = oldpt2[0] * 0.1f + oldpt2[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = (t-latheStep) / 360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu + cos( i + backEnd.refdef.floatTime ) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet( tess.normal[tess.numVertexes], pt[0], pt[1], l_oldpt[1] );
			VectorAdd( e->origin, tess.normal[tess.numVertexes], tess.xyz[tess.numVertexes] );
			VectorNormalize( tess.normal[tess.numVertexes] );
			i = pt[0] * 0.1f + pt[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = t/360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu-bezierStep + cos( i + backEnd.refdef.floatTime ) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet( tess.normal[tess.numVertexes], pt2[0], pt2[1], l_oldpt2[1] );
			VectorAdd( e->origin, tess.normal[tess.numVertexes], tess.xyz[tess.numVertexes] );
			VectorNormalize( tess.normal[tess.numVertexes] );
			i = pt2[0] * 0.1f + pt2[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = t/360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu + cos( i + backEnd.refdef.floatTime ) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = vbase;
			tess.indexes[tess.numIndexes++] = vbase + 1;
			tess.indexes[tess.numIndexes++] = vbase + 3;

			tess.indexes[tess.numIndexes++] = vbase + 3;
			tess.indexes[tess.numIndexes++] = vbase + 2;
			tess.indexes[tess.numIndexes++] = vbase;

			// Shuffle new points to old
			VectorCopy2( pt, oldpt );
			VectorCopy2( pt2, oldpt2 );
		}

		// shuffle lathe points
		VectorCopy2( l_oldpt2, l_oldpt );
	}
}

#define DISK_DEF	4
#define TUBE_DEF	6

static void RB_SurfaceClouds()
{
	// Disk definition
	float diskStripDef[DISK_DEF] = {
				0.0f,
				0.4f,
				0.7f,
				1.0f };

	float diskAlphaDef[DISK_DEF] = {
				1.0f,
				1.0f,
				0.4f,
				0.0f };

	float diskCurveDef[DISK_DEF] = {
				0.0f,
				0.0f,
				0.008f,
				0.02f };

	// tube definition
	float tubeStripDef[TUBE_DEF] = {
				0.0f,
				0.05f,
				0.1f,
				0.5f,
				0.7f,
				1.0f };

	float tubeAlphaDef[TUBE_DEF] = {
				0.0f,
				0.45f,
				1.0f,
				1.0f,
				0.45f,
				0.0f };

	float tubeCurveDef[TUBE_DEF] = {
				0.0f,
				0.004f,
				0.006f,
				0.01f,
				0.006f,
				0.0f };

	refEntity_t *e;
	vec3_t		pt, oldpt;
	vec3_t		pt2, oldpt2;
	float		latheStep = 30.0f;
	float		s, c, temp;
	float		*stripDef, *alphaDef, *curveDef, ct;
	int			i, t, vbase;

	e = &backEnd.currentEntity->e;

	// select which type we shall be doing
	if ( e->renderfx & RF_GROW ) // doing tube type
	{
		ct = TUBE_DEF;
		stripDef = tubeStripDef;
		alphaDef = tubeAlphaDef;
		curveDef = tubeCurveDef;
		e->backlerp *= -1; // needs to be reversed
	}
	else
	{
		ct = DISK_DEF;
		stripDef = diskStripDef;
		alphaDef = diskAlphaDef;
		curveDef = diskCurveDef;
	}

	// do the strip def, then lathe this around to make a 3d model
	for ( i = 0; i < ct - 1; i++ )
	{
		VectorSet( oldpt,	(stripDef[i]	* (e->radius - e->rotation)) + e->rotation,	0, curveDef[i] * e->radius * e->backlerp );
		VectorSet( oldpt2,	(stripDef[i+1]	* (e->radius - e->rotation)) + e->rotation,	0, curveDef[i+1] * e->radius * e->backlerp );

		// lathe section around in a complete circle
		for ( t = latheStep; t <= 360; t += latheStep )
		{
			// rotate every time except last seg
			if ( t < 360.0f )
			{
				VectorCopy( oldpt, pt );
				VectorCopy( oldpt2, pt2 );

				s = sin( DEG2RAD( latheStep ));
				c = cos( DEG2RAD( latheStep ));

				// rotate lathe points
				temp = c * pt[0] - s * pt[1];	// c -s 0
				pt[1] = s * pt[0] + c * pt[1];	// s  c 0
				pt[0] = temp;					// 0  0 1

				temp = c * pt2[0] - s * pt2[1];	 // c -s 0
				pt2[1] = s * pt2[0] + c * pt2[1];// s  c 0
				pt2[0] = temp;					 // 0  0 1
			}
			else
			{
				// just glue directly to the def points.
				VectorSet( pt,	(stripDef[i]	* (e->radius - e->rotation)) + e->rotation,	0, curveDef[i] * e->radius * e->backlerp );
				VectorSet( pt2,	(stripDef[i+1]	* (e->radius - e->rotation)) + e->rotation,	0, curveDef[i+1] * e->radius * e->backlerp );
			}

			RB_CHECKOVERFLOW( 4, 6 );

			vbase = tess.numVertexes;

			// Actually generate the necessary verts
			VectorAdd( e->origin, oldpt, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
			tess.vertexColors[tess.numVertexes][1] =
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd( e->origin, oldpt2, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
			tess.vertexColors[tess.numVertexes][1] =
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i+1];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd( e->origin, pt, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
			tess.vertexColors[tess.numVertexes][1] =
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd( e->origin, pt2, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
			tess.vertexColors[tess.numVertexes][1] =
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i+1];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = vbase;
			tess.indexes[tess.numIndexes++] = vbase + 1;
			tess.indexes[tess.numIndexes++] = vbase + 3;

			tess.indexes[tess.numIndexes++] = vbase + 3;
			tess.indexes[tess.numIndexes++] = vbase + 2;
			tess.indexes[tess.numIndexes++] = vbase;

			// Shuffle new points to old
			VectorCopy2( pt, oldpt );
			VectorCopy2( pt2, oldpt2 );
		}
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis( void ) {
	GL_Bind( tr.whiteImage );
	GL_State( GLS_DEFAULT );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
void RB_SurfaceEntity( surfaceType_t *surfType ) {
	switch( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_ORIENTED_QUAD:
		RB_SurfaceOrientedQuad();
		break;
	case RT_LINE:
		RB_SurfaceLine();
		break;
	case RT_ELECTRICITY:
		RB_SurfaceElectricity();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_SABER_GLOW:
		RB_SurfaceSaberGlow();
		break;
	case RT_CYLINDER:
		RB_SurfaceCylinder();
		break;
	case RT_LATHE:
		RB_SurfaceLathe();
		break;
	case RT_CLOUDS:
		RB_SurfaceClouds();
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
	return;
}

void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}


/*
==================
RB_TestZFlare

This is called at surface tesselation time
==================
*/
static bool RB_TestZFlare( vec3_t point) {
	int				i;
	vec4_t			eye, clip, normalized, window;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip( point, backEnd.ori.modelMatrix,
		backEnd.viewParms.projectionMatrix, eye, clip );

	// check to see if the point is completely off screen
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( clip[i] >= clip[3] || clip[i] <= -clip[3] ) {
			return qfalse;
		}
	}

	R_TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );

	if ( window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth
		|| window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight ) {
		return qfalse;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

//do test
	float			depth = 0.0f;
	bool			visible;
	float			screenZ;

	// read back the z buffer contents
	if ( r_flares->integer !=1 ) {	//skipping the the z-test
		return true;
	}
	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;
	qglReadPixels( backEnd.viewParms.viewportX + window[0],backEnd.viewParms.viewportY + window[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );

	screenZ = backEnd.viewParms.projectionMatrix[14] /
		( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

	visible = ( -eye[2] - -screenZ ) < 24;
	return visible;
}

void RB_SurfaceFlare( srfFlare_t *surf ) {
	vec3_t		left, up;
	float		radius;
	byte		color[4];
	vec3_t		dir;
	vec3_t		origin;
	float		d, dist;

	if ( !r_flares->integer ) {
		return;
	}

	if (!RB_TestZFlare( surf->origin ) ) {
		return;
	}

	// calculate the xyz locations for the four corners
	VectorMA( surf->origin, 3, surf->normal, origin );
	float* snormal = surf->normal;

	VectorSubtract( origin, backEnd.viewParms.ori.origin, dir );
	dist = VectorNormalize( dir );

	d = -DotProduct( dir, snormal );
	if ( d < 0 ) {
		d = -d;
	}

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	color[0] = d * 255;
	color[1] = d * 255;
	color[2] = d * 255;
	color[3] = 255;	//only gets used if the shader has cgen exact_vertex!

	radius = tess.shader->portalRange ? tess.shader->portalRange: 30;
	if (dist < 512.0f)
	{
		radius = radius * dist / 512.0f;
	}
	if (radius<5.0f)
	{
		radius = 5.0f;
	}
	VectorScale( backEnd.viewParms.ori.axis[1], radius, left );
	VectorScale( backEnd.viewParms.ori.axis[2], radius, up );
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( origin, left, up, color );
}


void RB_SurfaceDisplayList( srfDisplayList_t *surf ) {
	// all appropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
	qglCallList( surf->listNum );
}

void RB_SurfaceSkip( void *surf ) {
}

void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD,
	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP,
	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(void*))RB_SurfaceTriangles,	// SF_TRIANGLES,
	(void(*)(void*))RB_SurfacePolychain,	// SF_POLY,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MD3,
/*
Ghoul2 Insert Start
*/

	(void(*)(void*))RB_SurfaceGhoul,		// SF_MDX,
/*
Ghoul2 Insert End
*/

	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void(*)(void*))RB_SurfaceDisplayList	// SF_DISPLAY_LIST
};
