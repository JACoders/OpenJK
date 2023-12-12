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
// tr_surf.c
#include "tr_local.h"
#include "tr_weather.h"

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
	if ((tess.numVertexes + verts) < SHADER_MAX_VERTEXES &&
			(tess.numIndexes + indexes) < SHADER_MAX_INDEXES)
	{
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
}

void RB_CheckVBOandIBO(VBO_t *vbo, IBO_t *ibo)
{
	if (vbo != glState.currentVBO ||
			ibo != glState.currentIBO ||
			tess.multiDrawPrimitives >= MAX_MULTIDRAW_PRIMITIVES)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );

		R_BindVBO(vbo);
		R_BindIBO(ibo);
	}

	if (vbo != backEndData->currentFrame->dynamicVbo &&
		ibo != backEndData->currentFrame->dynamicIbo)
	{
		tess.useInternalVBO = qfalse;
	}

	if ( ibo != backEndData->currentFrame->dynamicIbo )
	{
		tess.externalIBO = ibo;
	}
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 ) {
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

	tess.normal[ndx] =
	tess.normal[ndx+1] =
	tess.normal[ndx+2] =
	tess.normal[ndx+3] = R_VboPackNormal(normal);

	// standard square texture coordinates
	VectorSet2(tess.texCoords[ndx  ][0], s1, t1);
	VectorSet2(tess.texCoords[ndx  ][1], s1, t1);

	VectorSet2(tess.texCoords[ndx+1][0], s2, t1);
	VectorSet2(tess.texCoords[ndx+1][1], s2, t1);

	VectorSet2(tess.texCoords[ndx+2][0], s2, t2);
	VectorSet2(tess.texCoords[ndx+2][1], s2, t2);

	VectorSet2(tess.texCoords[ndx+3][0], s1, t2);
	VectorSet2(tess.texCoords[ndx+3][1], s1, t2);

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	VectorCopy4(color, tess.vertexColors[ndx]);
	VectorCopy4(color, tess.vertexColors[ndx+1]);
	VectorCopy4(color, tess.vertexColors[ndx+2]);
	VectorCopy4(color, tess.vertexColors[ndx+3]);

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}


/*
==============
RB_InstantQuad

based on Tess_InstantQuad from xreal
==============
*/
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4])
{
//	GLimp_LogComment("--- RB_InstantQuad2 ---\n");					// FIXME: REIMPLEMENT (wasn't implemented in ioq3 to begin with) --eez

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;
	tess.minIndex = 0;
	tess.maxIndex = 3;
	tess.useInternalVBO = qtrue;

	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD0);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD0, NULL);

	R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex);

	RB_CommitInternalBufferData();

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.minIndex = 0;
	tess.maxIndex = 0;
	tess.useInternalVBO = qfalse;
}


void RB_InstantQuad(vec4_t quadVerts[4])
{
	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix4x4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

void RB_InstantTriangle() 
{
	qglDrawArrays(GL_TRIANGLES, 0, 3);
}



/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void ) {
	vec3_t		left, up;
	float		radius;
	float			colors[4];
	trRefEntity_t	*ent = backEnd.currentEntity;

	// calculate the xyz locations for the four corners
	radius = ent->e.radius;
	if ( ent->e.rotation == 0 ) {
		VectorScale( backEnd.viewParms.ori.axis[1], radius, left );
		VectorScale( backEnd.viewParms.ori.axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;
		
		ang = M_PI * ent->e.rotation / 180;
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

	VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);

	RB_AddQuadStamp( ent->e.origin, left, up, colors );
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
	float	color[4];

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;
//	MakeNormalVectors( backEnd.currentEntity->e.axis[0], left, up );
	VectorCopy( backEnd.currentEntity->e.axis[1], left );
	VectorCopy( backEnd.currentEntity->e.axis[2], up );

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

	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, color);

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, color);
}

/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		tess.vertexColors[numv][0] = p->verts[ i ].modulate[0] / 255.0f;
		tess.vertexColors[numv][1] = p->verts[ i ].modulate[1] / 255.0f;
		tess.vertexColors[numv][2] = p->verts[ i ].modulate[2] / 255.0f;
		tess.vertexColors[numv][3] = p->verts[ i ].modulate[3] / 255.0f;

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

static void RB_SurfaceVertsAndIndexes( int numVerts, srfVert_t *verts, int numIndexes, glIndex_t *indexes, int dlightBits, int pshadowBits)
{
	int             i;
	glIndex_t      *inIndex;
	srfVert_t      *dv;
	float          *xyz, *texCoords, *lightCoords;
	uint32_t        *lightdir;
	uint32_t        *normal;
	uint32_t        *tangent;
	glIndex_t      *outIndex;
	float          *color;
	gpuFrame_t		*currentFrame = backEndData->currentFrame;

	RB_CheckVBOandIBO(currentFrame->dynamicVbo, currentFrame->dynamicIbo);

	RB_CHECKOVERFLOW( numVerts, numIndexes );

	inIndex = indexes;
	outIndex = &tess.indexes[ tess.numIndexes ];
	for ( i = 0 ; i < numIndexes ; i++ ) {
		*outIndex++ = tess.numVertexes + *inIndex++;
	}
	tess.numIndexes += numIndexes;

	if ( tess.shader->vertexAttribs & ATTR_POSITION )
	{
		dv = verts;
		xyz = tess.xyz[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, xyz+=4 )
			VectorCopy(dv->xyz, xyz);
	}

	if ( tess.shader->vertexAttribs & ATTR_NORMAL )
	{
		dv = verts;
		normal = &tess.normal[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, normal++ )
			*normal = R_VboPackNormal(dv->normal);
	}

	if ( tess.shader->vertexAttribs & ATTR_TANGENT )
	{
		dv = verts;
		tangent = &tess.tangent[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, tangent++ )
			*tangent = R_VboPackTangent(dv->tangent);
	}

	if ( tess.shader->vertexAttribs & ATTR_TEXCOORD0 )
	{
		dv = verts;
		texCoords = tess.texCoords[ tess.numVertexes ][0];
		for ( i = 0 ; i < numVerts ; i++, dv++, texCoords+=NUM_TESS_TEXCOORDS*2 )
			VectorCopy2(dv->st, texCoords);
	}

	for (int tc = 0; tc < MAXLIGHTMAPS; ++tc)
	{
		if ( tess.shader->vertexAttribs & (ATTR_TEXCOORD1 + tc) )
		{
			dv = verts;
			lightCoords = tess.texCoords[ tess.numVertexes ][1 + tc];
			for ( i = 0 ; i < numVerts ; i++, dv++, lightCoords+=NUM_TESS_TEXCOORDS*2 )
				VectorCopy2(dv->lightmap[tc], lightCoords);
		}
	}

	if ( tess.shader->vertexAttribs & ATTR_COLOR )
	{
		dv = verts;
		color = tess.vertexColors[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, color+=4 )
			VectorCopy4(dv->vertexColors[0], color);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
	{
		dv = verts;
		lightdir = &tess.lightdir[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightdir++ )
			*lightdir = R_VboPackNormal(dv->lightdir);
	}

#if 0  // nothing even uses vertex dlightbits
	for ( i = 0 ; i < numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
	}
#endif

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	tess.numVertexes += numVerts;
}

static qboolean RB_SurfaceVbo(
		VBO_t *vbo, IBO_t *ibo, int numVerts, int numIndexes, int firstIndex,
		int minIndex, int maxIndex, int dlightBits, int pshadowBits, qboolean shaderCheck)
{
	int i, mergeForward, mergeBack;
	GLvoid *firstIndexOffset, *lastIndexOffset;

	if (!vbo || !ibo)
	{
		return qfalse;
	}

	if (shaderCheck &&
			(ShaderRequiresCPUDeforms(tess.shader) ||
			 tess.shader->isSky ||
			 tess.shader->isPortal))
	{
		return qfalse;
	}

	RB_CheckVBOandIBO(vbo, ibo);

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	// merge this into any existing multidraw primitives
	mergeForward = -1;
	mergeBack = -1;
	firstIndexOffset = BUFFER_OFFSET(firstIndex * sizeof(glIndex_t));
	lastIndexOffset  = BUFFER_OFFSET((firstIndex + numIndexes) * sizeof(glIndex_t));

	if (r_mergeMultidraws->integer)
	{
		i = 0;

		if (r_mergeMultidraws->integer == 1)
		{
			// lazy merge, only check the last primitive
			if (tess.multiDrawPrimitives)
			{
				i = tess.multiDrawPrimitives - 1;
			}
		}

		for (; i < tess.multiDrawPrimitives; i++)
		{
			if (tess.multiDrawLastIndex[i] == firstIndexOffset)
			{
				mergeBack = i;
			}

			if (lastIndexOffset == tess.multiDrawFirstIndex[i])
			{
				mergeForward = i;
			}
		}
	}

	if (mergeBack != -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes;
		tess.multiDrawLastIndex[mergeBack]   = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], minIndex);
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack == -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeForward] += numIndexes;
		tess.multiDrawFirstIndex[mergeForward]  = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[mergeForward]   = tess.multiDrawFirstIndex[mergeForward] + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawMinIndex[mergeForward] = MIN(tess.multiDrawMinIndex[mergeForward], minIndex);
		tess.multiDrawMaxIndex[mergeForward] = MAX(tess.multiDrawMaxIndex[mergeForward], maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack != -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawLastIndex[mergeBack]   = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], MIN(tess.multiDrawMinIndex[mergeForward], minIndex));
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], MAX(tess.multiDrawMaxIndex[mergeForward], maxIndex));
		tess.multiDrawPrimitives--;

		if (mergeForward != tess.multiDrawPrimitives)
		{
			tess.multiDrawNumIndexes[mergeForward] = tess.multiDrawNumIndexes[tess.multiDrawPrimitives];
			tess.multiDrawFirstIndex[mergeForward] = tess.multiDrawFirstIndex[tess.multiDrawPrimitives];
		}
		backEnd.pc.c_multidrawsMerged += 2;
	}
	else if (mergeBack == -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[tess.multiDrawPrimitives] = numIndexes;
		tess.multiDrawFirstIndex[tess.multiDrawPrimitives] = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[tess.multiDrawPrimitives] = (glIndex_t *)lastIndexOffset;
		tess.multiDrawMinIndex[tess.multiDrawPrimitives] = minIndex;
		tess.multiDrawMaxIndex[tess.multiDrawPrimitives] = maxIndex;
		tess.multiDrawPrimitives++;
	}

	backEnd.pc.c_multidraws++;

	tess.numIndexes  += numIndexes;
	tess.numVertexes += numVerts;

	return qtrue;
}

/*
=============
RB_SurfaceBSPTriangles
=============
*/
static void RB_SurfaceBSPTriangles( srfBspSurface_t *srf ) {
	if( RB_SurfaceVbo (srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes,
				srf->firstIndex, srf->minIndex, srf->maxIndex, srf->dlightBits, srf->pshadowBits, qtrue ) )
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
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
	shaderProgram_t *sp = &tr.textureColorShader;
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

	// FIXME: Quake3 doesn't use this, so I never tested it
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.minIndex = 0;
	tess.maxIndex = 0;

	for ( i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		VectorCopy(start_points[ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
		VectorCopy(end_points  [ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
	}

	for ( i = 0; i < NUM_BEAM_SEGS; i++ ) {
		tess.indexes[tess.numIndexes++] =       i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;

		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  + (i + 1) * 2;
	}

	tess.minIndex = 0;
	tess.maxIndex = tess.numVertexes;
	tess.useInternalVBO = qtrue;

	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	RB_UpdateVBOs(ATTR_POSITION);
	
	GLSL_VertexAttribsState(ATTR_POSITION, NULL);
	GLSL_BindProgram(sp);
		
	GLSL_SetUniformMatrix4x4(sp, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
					
	GLSL_SetUniformVec4(sp, UNIFORM_COLOR, colorRed);

	R_DrawElementsVBO(tess.numIndexes, tess.firstIndex, tess.minIndex, tess.maxIndex);

	RB_CommitInternalBufferData();

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.minIndex = 0;
	tess.maxIndex = 0;
	tess.useInternalVBO = qfalse;
}

//------------------
// DoSprite
//------------------
static void DoSprite( vec3_t origin, float radius, float rotation ) 
{
	float	s, c;
	float	ang;
	vec3_t	left, up;
	float	color[4];
	
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

	VectorScale4(backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, color);

	RB_AddQuadStamp( origin, left, up, color );
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

		DoSprite( end, e->radius, 0.0f );//random() * 360.0f );
		e->radius += 0.017f;
	}

	// Big hilt sprite
	// Please don't kill me Pat...I liked the hilt glow blob, but wanted a subtle pulse.:)  Feel free to ditch it if you don't like it.  --Jeff
	// Please don't kill me Jeff...  The pulse is good, but now I want the halo bigger if the saber is shorter...  --Pat
	DoSprite( e->origin, 5.5f + Q_flrand(0.0f, 1.0f) * 0.25f, 0.0f );//random() * 360.0f );
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
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = 0;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = 1;//backEnd.currentEntity->e.shaderTexCoord[1];
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void DoLine2(const vec3_t start, const vec3_t end, const vec3_t up, float spanWidth, float spanWidth2, const float tcStart = 0.0f, const float tcEnd = 1.0f)
{
	int			vbase;

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = tcStart;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( start, -spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = tcStart;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = tcEnd;//backEnd.currentEntity->e.shaderTexCoord[1];
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, -spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;//backEnd.currentEntity->e.shaderTexCoord[0];
	tess.texCoords[tess.numVertexes][0][1] = tcEnd;//backEnd.currentEntity->e.shaderTexCoord[1];
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

#ifndef REND2_SP
static void DoLine_Oriented( const vec3_t start, const vec3_t end, const vec3_t up, float spanWidth )
{
	float		spanWidth2;
	int			vbase;

	vbase = tess.numVertexes;

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = backEnd.currentEntity->e.data.line.stscale;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 1;
	tess.texCoords[tess.numVertexes][0][1] = backEnd.currentEntity->e.data.line.stscale;
	VectorScale4 (backEnd.currentEntity->e.shaderRGBA, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void RB_SurfaceOrientedLine(void)
{
	refEntity_t *e;
	vec3_t		right;
	vec3_t		start, end;

	e = &backEnd.currentEntity->e;

	VectorCopy(e->oldorigin, end);
	VectorCopy(e->origin, start);

	// compute side vector
	VectorNormalize(e->axis[1]);
	VectorCopy(e->axis[1], right);
	DoLine_Oriented(start, end, right, e->data.line.width*0.5);
}
#endif

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

#define NUM_CYLINDER_SEGMENTS 32

// FIXME: use quad stamp?
static void DoCylinderPart(polyVert_t *verts)
{
	int			vbase;
	int			i;

	RB_CHECKOVERFLOW( 4, 6 );

	vbase = tess.numVertexes;

	for (i=0; i<4; i++)
	{
		VectorCopy( verts->xyz, tess.xyz[tess.numVertexes] );
		tess.texCoords[tess.numVertexes][0][0] = verts->st[0];
		tess.texCoords[tess.numVertexes][0][1] = verts->st[1];
		VectorScale4 (verts->modulate, 1.0f / 255.0f, tess.vertexColors[tess.numVertexes]);
		tess.numVertexes++;
		verts++;
	}	

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 3;
	tess.indexes[tess.numIndexes++] = vbase;
}

// e->origin holds the bottom point
// e->oldorigin holds the top point
// e->radius holds the radius

static void RB_SurfaceCylinder( void )
{
	static polyVert_t	lower_points[NUM_CYLINDER_SEGMENTS], upper_points[NUM_CYLINDER_SEGMENTS], verts[4];
	vec3_t		vr, vu, midpoint, v1;
	float		detail, length;
	int			i;
	int			segments;
	refEntity_t *e;
	int			nextSegment;

	e = &backEnd.currentEntity->e;

	//Work out the detail level of this cylinder
	VectorAdd( e->origin, e->oldorigin, midpoint );
	VectorScale(midpoint, 0.5f, midpoint);		// Average start and end

	VectorSubtract( midpoint, backEnd.viewParms.ori.origin, midpoint );
	length = VectorNormalize( midpoint );

	// this doesn't need to be perfect....just a rough compensation for zoom level is enough
	length *= (backEnd.viewParms.fovX / 90.0f);

	detail = 1 - ((float) length / 1024 );
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
	VectorScale( vu, e->rotation, vu );	// size2

	// Calculate the step around the cylinder
	detail = 360.0f / (float)segments;

	for ( i = 0; i < segments; i++ )
	{
		//Upper ring
		RotatePointAroundVector( upper_points[i].xyz, e->axis[0], vu, detail * i );
		VectorAdd( upper_points[i].xyz, e->origin, upper_points[i].xyz );
		
		//Lower ring
		RotatePointAroundVector( lower_points[i].xyz, e->axis[0], v1, detail * i );
		VectorAdd( lower_points[i].xyz, e->oldorigin, lower_points[i].xyz );
	}
	
	// Calculate the texture coords so the texture can wrap around the whole cylinder
	detail = 1.0f / (float)segments;

	for ( i = 0; i < segments; i++ )
	{
		if ( i + 1 < segments )
			nextSegment = i + 1;
		else
			nextSegment = 0;

 		VectorCopy( upper_points[i].xyz, verts[0].xyz );
		verts[0].st[1] = 1.0f;
		verts[0].st[0] = detail * i;
		verts[0].modulate[0] = (byte)(e->shaderRGBA[0]);
		verts[0].modulate[1] = (byte)(e->shaderRGBA[1]);
		verts[0].modulate[2] = (byte)(e->shaderRGBA[2]);
		verts[0].modulate[3] = (byte)(e->shaderRGBA[3]);

		VectorCopy( lower_points[i].xyz, verts[1].xyz );
		verts[1].st[1] = 0.0f;
		verts[1].st[0] = detail * i;
		verts[1].modulate[0] = (byte)(e->shaderRGBA[0]);
		verts[1].modulate[1] = (byte)(e->shaderRGBA[1]);
		verts[1].modulate[2] = (byte)(e->shaderRGBA[2]);
		verts[1].modulate[3] = (byte)(e->shaderRGBA[3]);

		VectorCopy( lower_points[nextSegment].xyz, verts[2].xyz );
		verts[2].st[1] = 0.0f;
		verts[2].st[0] = detail * ( i + 1 );
		verts[2].modulate[0] = (byte)(e->shaderRGBA[0]);
		verts[2].modulate[1] = (byte)(e->shaderRGBA[1]);
		verts[2].modulate[2] = (byte)(e->shaderRGBA[2]);
		verts[2].modulate[3] = (byte)(e->shaderRGBA[3]);

		VectorCopy( upper_points[nextSegment].xyz, verts[3].xyz );
		verts[3].st[1] = 1.0f;
		verts[3].st[0] = detail * ( i + 1 );
		verts[3].modulate[0] = (byte)(e->shaderRGBA[0]);
		verts[3].modulate[1] = (byte)(e->shaderRGBA[1]);
		verts[3].modulate[2] = (byte)(e->shaderRGBA[2]);
		verts[3].modulate[3] = (byte)(e->shaderRGBA[3]);

		DoCylinderPart(verts);
	}
}

static vec3_t sh1, sh2;
static float f_count;

#define LIGHTNING_RECURSION_LEVEL 1 // was 2

// these functions are pretty crappy in terms of returning a nice range of rnd numbers, but it's probably good enough?
/*static int Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

static float Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

static float Q_crandom( int *seed ) {
	return 2.0F * ( Q_random( seed ) - 0.5f );
}
*/
// Up front, we create a random "shape", then apply that to each line segment...and then again to each of those segments...kind of like a fractal
//----------------------------------------------------------------------------
static void CreateShape()
//----------------------------------------------------------------------------
{
#ifndef REND2_SP
	VectorSet( sh1, 0.66f + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
				0.07f + Q_flrand(-1.0f, 1.0f) * 0.025f,
				0.07f + Q_flrand(-1.0f, 1.0f) * 0.025f );

	// it seems to look best to have a point on one side of the ideal line, then the other point on the other side.
	VectorSet( sh2, 0.33f + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
					-sh1[1] + Q_flrand(-1.0f, 1.0f) * 0.02f,	// forcing point to be on the opposite side of the line -- right
					-sh1[2] + Q_flrand(-1.0f, 1.0f) * 0.02f );// up
#else
	VectorSet(sh1, 0.66f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
		0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f,
		0.08f + Q_flrand(-1.0f, 1.0f) * 0.02f);

	// it seems to look best to have a point on one side of the ideal line, then the other point on the other side.
	VectorSet(sh2, 0.33f,// + Q_flrand(-1.0f, 1.0f) * 0.1f,	// fwd
		-sh1[1] + Q_flrand(-1.0f, 1.0f) * 0.02f,	// forcing point to be on the opposite side of the line -- right
		-sh1[2] + Q_flrand(-1.0f, 1.0f) * 0.02f);// up
#endif
}

//----------------------------------------------------------------------------
static void ApplyShape( vec3_t start, vec3_t end, vec3_t right, float sradius, float eradius, int count, float startPerc = 0.0f, float endPerc = 1.0f)
//----------------------------------------------------------------------------
{
	vec3_t	point1, point2, fwd;
	vec3_t	rt, up;
	float	perc, dis;

    if ( count < 1 )
	{
		// done recursing
		DoLine2( start, end, right, sradius, eradius, startPerc, endPerc);
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
#ifndef REND2_SP
    ApplyShape( start, point1, right, sradius, rads1, count - 1 );
#else
    ApplyShape( start, point1, right, sradius, rads1, count - 1, startPerc, startPerc * 0.666f + endPerc * 0.333f);
#endif

	perc = sh2[0];

	VectorScale( start, perc, point2 );
	VectorMA( point2, 1.0f - perc, end, point2 );
	VectorMA( point2, dis * sh2[1], rt, point2 );
	VectorMA( point2, dis * sh2[2], up, point2 );
    
	// recursion
#ifndef REND2_SP
    ApplyShape( point2, point1, right, rads1, rads2, count - 1 );
	ApplyShape( point2, end, right, rads2, eradius, count - 1 );
#else
	ApplyShape(point2, point1, right, rads1, rads2, count - 1, startPerc * 0.333f + endPerc * 0.666f, startPerc * 0.666f + endPerc * 0.333f);
	ApplyShape(point2, end, right, rads2, eradius, count - 1, startPerc * 0.333f + endPerc * 0.666f, endPerc);
#endif
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

#ifdef REND2_SP
	if (dis > 2000)	//freaky long
	{
		//		ri.Printf( PRINT_WARNING, "DoBoltSeg: insane distance.\n" );
		dis = 2000;
	}
#endif

	MakeNormalVectors( fwd, rt, up );

	VectorCopy( start, old );
    
#ifndef REND2_SP
	oldRadius = newRadius = radius;
	for (i = 20; i <= dis; i += 20)
	{
		// because of our large step size, we may not actually draw to the end.  In this case, fudge our percent so that we are basically complete
		if (i + 20 > dis)
#else
	newRadius = oldRadius = radius;
	for (i = 16; i <= dis; i += 16)
	{
		// because of our large step size, we may not actually draw to the end.  In this case, fudge our percent so that we are basically complete
		if (i + 16 > dis)
#endif
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
#ifndef REND2_SP
		VectorMA( temp, Q_crandom(&e->frame) * 7.0f * e->axis[0][0], rt, temp );	// move more in direction perpendicular to line, angles is really the chaos
		VectorMA( temp, Q_crandom(&e->frame) * 7.0f * e->axis[0][0], up, temp );	// move more in direction perpendicular to line
#else
		VectorMA(temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], rt, temp);	// move more in direction perpendicular to line, angles is really the chaos
		VectorMA(temp, Q_crandom(&e->frame) * 7.0f * e->angles[0], up, temp);	// move more in direction perpendicular to line
#endif

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
#ifndef REND2_SP
        ApplyShape( cur, old, right, newRadius, oldRadius, LIGHTNING_RECURSION_LEVEL );
#else
		ApplyShape(cur, old, right, newRadius, oldRadius, 2 - r_lodbias->integer, 0, 1);
#endif
  
		// randomly split off to create little tendrils, but don't do it too close to the end and especially if we are not even of the forked variety
        if ( ( e->renderfx & RF_FORKED ) && f_count > 0 && Q_random(&e->frame) > 0.94f && radius * (1.0f - perc) > 0.2f )
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
#ifndef REND2_SP
		perc = 1.0f - ( e->axis[0][2]/*endTime*/ - tr.refdef.time ) / e->axis[0][1]/*duration*/;
#else
		perc = 1.0f - (e->endTime - tr.refdef.time) / e->angles[1]/*duration*/;
#endif
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
#ifdef REND2_SP
	f_count = 3;
#endif
    DoBoltSeg( start, end, right, radius );
}

//================================================================================

#if 0
/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(vec4_t *normals, unsigned int count)
{
//    assert(count);
        
#if idppc
    {
        register float half = 0.5;
        register float one  = 1.0;
        float *components = (float *)normals;
        
        // Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
        // runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
        // refinement step to get a little more precision.  This seems to yeild results
        // that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
        // (That is, for the given input range of about 0.6 to 2.0).
        do {
            float x, y, z;
            float B, y0, y1;
            
            x = components[0];
            y = components[1];
            z = components[2];
            components += 4;
            B = x*x + y*y + z*z;

#ifdef __GNUC__            
            asm("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
			y0 = __frsqrte(B);
#endif
            y1 = y0 + half*y0*(one - B*y0*y0);

            x = x * y1;
            y = y * y1;
            components[-4] = x;
            z = z * y1;
            components[-3] = y;
            components[-2] = z;
        } while(count--);
    }
#else // No assembly version for this architecture, or C_ONLY defined
	// given the input, it's safe to call VectorNormalizeFast
    while (count--) {
        VectorNormalizeFast(normals[0]);
        normals++;
    }
#endif

}
#endif



/*
** LerpMeshVertexes
*/
#if 0
#if idppc_altivec
static void LerpMeshVertexes_altivec(md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale QALIGN(16);
	float   newXyzScale QALIGN(16);
	float	oldNormalScale QALIGN(16);
	float newNormalScale QALIGN(16);
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
		vector signed short newNormalsVec0;
		vector signed short newNormalsVec1;
		vector signed int newNormalsIntVec;
		vector float newNormalsFloatVec;
		vector float newXyzScaleVec;
		vector unsigned char newNormalsLoadPermute;
		vector unsigned char newNormalsStorePermute;
		vector float zero;
		
		newNormalsStorePermute = vec_lvsl(0,(float *)&newXyzScaleVec);
		newXyzScaleVec = *(vector float *)&newXyzScale;
		newXyzScaleVec = vec_perm(newXyzScaleVec,newXyzScaleVec,newNormalsStorePermute);
		newXyzScaleVec = vec_splat(newXyzScaleVec,0);		
		newNormalsLoadPermute = vec_lvsl(0,newXyz);
		newNormalsStorePermute = vec_lvsr(0,outXyz);
		zero = (vector float)vec_splat_s8(0);
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			newNormalsLoadPermute = vec_lvsl(0,newXyz);
			newNormalsStorePermute = vec_lvsr(0,outXyz);
			newNormalsVec0 = vec_ld(0,newXyz);
			newNormalsVec1 = vec_ld(16,newXyz);
			newNormalsVec0 = vec_perm(newNormalsVec0,newNormalsVec1,newNormalsLoadPermute);
			newNormalsIntVec = vec_unpackh(newNormalsVec0);
			newNormalsFloatVec = vec_ctf(newNormalsIntVec,0);
			newNormalsFloatVec = vec_madd(newNormalsFloatVec,newXyzScaleVec,zero);
			newNormalsFloatVec = vec_perm(newNormalsFloatVec,newNormalsFloatVec,newNormalsStorePermute);
			//outXyz[0] = newXyz[0] * newXyzScale;
			//outXyz[1] = newXyz[1] * newXyzScale;
			//outXyz[2] = newXyz[2] * newXyzScale;

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

			vec_ste(newNormalsFloatVec,0,outXyz);
			vec_ste(newNormalsFloatVec,4,outXyz);
			vec_ste(newNormalsFloatVec,8,outXyz);
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

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
}
#endif
#endif

static void LerpMeshVertexes_scalar(mdvSurface_t *surf, float backlerp)
{
#if 0
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

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
#endif
	float *outXyz;
	uint32_t *outNormal;
	mdvVertex_t *newVerts;
	int		vertNum;

	newVerts = surf->verts + backEnd.currentEntity->e.frame * surf->numVerts;

	outXyz =    tess.xyz[tess.numVertexes];
	outNormal = &tess.normal[tess.numVertexes];

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			vec3_t normal;

			VectorCopy(newVerts->xyz,    outXyz);
			VectorCopy(newVerts->normal, normal);

			*outNormal = R_VboPackNormal(normal);

			newVerts++;
			outXyz += 4;
			outNormal++;
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//

		mdvVertex_t *oldVerts;

		oldVerts = surf->verts + backEnd.currentEntity->e.oldframe * surf->numVerts;

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			vec3_t normal;

			VectorLerp(newVerts->xyz,    oldVerts->xyz,    backlerp, outXyz);
			VectorLerp(newVerts->normal, oldVerts->normal, backlerp, normal);
			VectorNormalize(normal);

			*outNormal = R_VboPackNormal(normal);

			newVerts++;
			oldVerts++;
			outXyz += 4;
			outNormal++;
		}
	}

}

static void LerpMeshVertexes(mdvSurface_t *surf, float backlerp)
{
#if 0
#if idppc_altivec
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		LerpMeshVertexes_altivec( surf, backlerp );
		return;
	}
#endif // idppc_altivec
#endif
	LerpMeshVertexes_scalar( surf, backlerp );
}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(mdvSurface_t *surface) {
	int				j;
	float			backlerp;
	mdvSt_t			*texCoords;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numIndexes );

	LerpMeshVertexes (surface, backlerp);

	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < surface->numIndexes ; j++) {
		tess.indexes[Bob + j] = Doug + surface->indexes[j];
	}
	tess.numIndexes += surface->numIndexes;

	texCoords = surface->st;

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j].st[0];
		tess.texCoords[Doug + j][0][1] = texCoords[j].st[1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;

}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceBSPFace( srfBspSurface_t *srf ) {
	if( RB_SurfaceVbo(srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes,
				srf->firstIndex, srf->minIndex, srf->maxIndex, srf->dlightBits, srf->pshadowBits, qtrue ) )
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
}


static float	LodErrorForVolume( vec3_t local, float radius ) {
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
static void RB_SurfaceBSPGrid( srfBspSurface_t *srf ) {
	int		i, j;
	float	*xyz;
	float	*texCoords, *lightCoords[MAXLIGHTMAPS];
	uint32_t *normal;
	uint32_t *tangent;
	float   *color;
	uint32_t *lightdir;
	srfVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int     pshadowBits;
	//int		*vDlightBits;

	if( RB_SurfaceVbo (srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes,
				srf->firstIndex, srf->minIndex, srf->maxIndex, srf->dlightBits, srf->pshadowBits, qtrue ) )
	{
		return;
	}

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	pshadowBits = srf->pshadowBits;
	tess.pshadowBits |= pshadowBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( srf->lodOrigin, srf->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < srf->width-1 ; i++ ) {
		if ( srf->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = srf->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < srf->height-1 ; i++ ) {
		if ( srf->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = srf->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
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
		normal = &tess.normal[numVertexes];
		tangent = &tess.tangent[numVertexes];
		texCoords = tess.texCoords[numVertexes][0];
		for (int tc = 0; tc < MAXLIGHTMAPS; ++tc)
			lightCoords[tc] = tess.texCoords[numVertexes][1 + tc];
		color = tess.vertexColors[numVertexes];
		lightdir = &tess.lightdir[numVertexes];
		//vDlightBits = &tess.vertexDlightBits[numVertexes];

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = srf->verts + heightTable[ used + i ] * srf->width
					+ widthTable[ j ];

				if ( tess.shader->vertexAttribs & ATTR_POSITION )
				{
					VectorCopy(dv->xyz, xyz);
					xyz += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_NORMAL )
				{
					*normal++ = R_VboPackNormal(dv->normal);
				}

				if ( tess.shader->vertexAttribs & ATTR_TANGENT )
				{
					*tangent++ = R_VboPackTangent(dv->tangent);
				}

				if ( tess.shader->vertexAttribs & ATTR_TEXCOORD0 )
				{
					VectorCopy2(dv->st, texCoords);
					texCoords += NUM_TESS_TEXCOORDS*2;
				}

				for (int tc = 0; tc < MAXLIGHTMAPS; ++tc)
				{
					if ( tess.shader->vertexAttribs & (ATTR_TEXCOORD1 + tc) )
					{
						VectorCopy2(dv->lightmap[tc], lightCoords[tc]);
						lightCoords[tc] += NUM_TESS_TEXCOORDS*2;
					}
				}

				if ( tess.shader->vertexAttribs & ATTR_COLOR )
				{
					VectorCopy4(dv->vertexColors[0], color);
					color += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
				{
					*lightdir++ = R_VboPackNormal(dv->lightdir);
				}

				//*vDlightBits++ = dlightBits;
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

#ifdef REND2_SP
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

	if (e->endTime && e->endTime > backEnd.refdef.time)
	{
		d = 1.0f - (e->endTime - backEnd.refdef.time) / 1000.0f;
	}

	if (e->frame && e->frame + 1000 > backEnd.refdef.time)
	{
		pain = (backEnd.refdef.time - e->frame) / 1000.0f;
		//		pain *= pain;
		pain = (1.0f - pain) * 0.08f;
	}

	VectorSet2(l_oldpt, e->axis[0][0], e->axis[0][1]);

	// do scalability stuff...r_lodbias 0-3
	int lod = r_lodbias->integer + 1;
	if (lod > 4)
	{
		lod = 4;
	}
	if (lod < 1)
	{
		lod = 1;
	}
	bezierStep = BEZIER_STEP * lod;
	latheStep = LATHE_SEG_STEP * lod;

	// Do bezier profile strip, then lathe this around to make a 3d model
	for (mu = 0.0f; mu <= 1.01f * d; mu += bezierStep)
	{
		// Four point curve
		mum1 = 1 - mu;
		mum13 = mum1 * mum1 * mum1;
		mu3 = mu * mu * mu;
		group1 = 3 * mu * mum1 * mum1;
		group2 = 3 * mu * mu *mum1;

		// Calc the current point on the curve
		for (i = 0; i < 2; i++)
		{
			l_oldpt2[i] = mum13 * e->axis[0][i] + group1 * e->axis[1][i] + group2 * e->axis[2][i] + mu3 * e->oldorigin[i];
		}

		VectorSet2(oldpt, l_oldpt[0], 0);
		VectorSet2(oldpt2, l_oldpt2[0], 0);

		// lathe patch section around in a complete circle
		for (t = latheStep; t <= 360; t += latheStep)
		{
			VectorSet2(pt, l_oldpt[0], 0);
			VectorSet2(pt2, l_oldpt2[0], 0);

			s = sin(DEG2RAD(t));
			c = cos(DEG2RAD(t));

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

			RB_CHECKOVERFLOW(4, 6);

			vbase = tess.numVertexes;

			vec3_t normal;

			// Actually generate the necessary verts
			VectorSet(normal, oldpt[0], oldpt[1], l_oldpt[1]);
			VectorAdd(e->origin, normal, tess.xyz[tess.numVertexes]);
			VectorNormalize(normal);
			tess.normal[tess.numVertexes] = R_VboPackNormal(normal);
			i = oldpt[0] * 0.1f + oldpt[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = (t - latheStep) / 360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu - bezierStep + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet(normal, oldpt2[0], oldpt2[1], l_oldpt2[1]);
			VectorAdd(e->origin, normal, tess.xyz[tess.numVertexes]);
			VectorNormalize(normal);
			tess.normal[tess.numVertexes] = R_VboPackNormal(normal);
			i = oldpt2[0] * 0.1f + oldpt2[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = (t - latheStep) / 360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet(normal, pt[0], pt[1], l_oldpt[1]);
			VectorAdd(e->origin, normal, tess.xyz[tess.numVertexes]);
			VectorNormalize(normal);
			tess.normal[tess.numVertexes] = R_VboPackNormal(normal);
			i = pt[0] * 0.1f + pt[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = t / 360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu - bezierStep + cos(i + backEnd.refdef.floatTime) * pain;
			tess.vertexColors[tess.numVertexes][0] = e->shaderRGBA[0];
			tess.vertexColors[tess.numVertexes][1] = e->shaderRGBA[1];
			tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[2];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorSet(normal, pt2[0], pt2[1], l_oldpt2[1]);
			VectorAdd(e->origin, normal, tess.xyz[tess.numVertexes]);
			VectorNormalize(normal);
			tess.normal[tess.numVertexes] = R_VboPackNormal(normal);
			i = pt2[0] * 0.1f + pt2[1] * 0.1f;
			tess.texCoords[tess.numVertexes][0][0] = t / 360.0f;
			tess.texCoords[tess.numVertexes][0][1] = mu + cos(i + backEnd.refdef.floatTime) * pain;
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
			VectorCopy2(pt, oldpt);
			VectorCopy2(pt2, oldpt2);
		}

		// shuffle lathe points
		VectorCopy2(l_oldpt2, l_oldpt);
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
	if (e->renderfx & RF_GROW) // doing tube type
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
	for (i = 0; i < ct - 1; i++)
	{
		VectorSet(oldpt, (stripDef[i] * (e->radius - e->rotation)) + e->rotation, 0, curveDef[i] * e->radius * e->backlerp);
		VectorSet(oldpt2, (stripDef[i + 1] * (e->radius - e->rotation)) + e->rotation, 0, curveDef[i + 1] * e->radius * e->backlerp);

		// lathe section around in a complete circle
		for (t = latheStep; t <= 360; t += latheStep)
		{
			// rotate every time except last seg
			if (t < 360.0f)
			{
				VectorCopy(oldpt, pt);
				VectorCopy(oldpt2, pt2);

				s = sin(DEG2RAD(latheStep));
				c = cos(DEG2RAD(latheStep));

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
				VectorSet(pt, (stripDef[i] * (e->radius - e->rotation)) + e->rotation, 0, curveDef[i] * e->radius * e->backlerp);
				VectorSet(pt2, (stripDef[i + 1] * (e->radius - e->rotation)) + e->rotation, 0, curveDef[i + 1] * e->radius * e->backlerp);
			}

			RB_CHECKOVERFLOW(4, 6);

			vbase = tess.numVertexes;

			// Actually generate the necessary verts
			VectorAdd(e->origin, oldpt, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
				tess.vertexColors[tess.numVertexes][1] =
				tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd(e->origin, oldpt2, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
				tess.vertexColors[tess.numVertexes][1] =
				tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i + 1];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd(e->origin, pt, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
				tess.vertexColors[tess.numVertexes][1] =
				tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			VectorAdd(e->origin, pt2, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = tess.xyz[tess.numVertexes][0] * 0.1f;
			tess.texCoords[tess.numVertexes][0][1] = tess.xyz[tess.numVertexes][1] * 0.1f;
			tess.vertexColors[tess.numVertexes][0] =
				tess.vertexColors[tess.numVertexes][1] =
				tess.vertexColors[tess.numVertexes][2] = e->shaderRGBA[0] * alphaDef[i + 1];
			tess.vertexColors[tess.numVertexes][3] = e->shaderRGBA[3];
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = vbase;
			tess.indexes[tess.numIndexes++] = vbase + 1;
			tess.indexes[tess.numIndexes++] = vbase + 3;

			tess.indexes[tess.numIndexes++] = vbase + 3;
			tess.indexes[tess.numIndexes++] = vbase + 2;
			tess.indexes[tess.numIndexes++] = vbase;

			// Shuffle new points to old
			VectorCopy2(pt, oldpt);
			VectorCopy2(pt2, oldpt2);
		}
	}
}
#endif

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
	// FIXME: implement this
#if 0
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
#endif
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity( surfaceType_t *surfType ) {
	switch( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_ORIENTED_QUAD:
		RB_SurfaceOrientedQuad();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_ELECTRICITY:
		RB_SurfaceElectricity();
		break;
	case RT_LINE:
		RB_SurfaceLine();
		break;
	case RT_SABER_GLOW:
		RB_SurfaceSaberGlow();
		break;
	case RT_CYLINDER:
		RB_SurfaceCylinder();
		break;
#ifndef REND2_SP
	case RT_ORIENTEDLINE:
		RB_SurfaceOrientedLine();
		break;
	case RT_ENT_CHAIN:
		{
			static trRefEntity_t tempEnt = *backEnd.currentEntity;

			//rww - if not static then currentEntity is garbage because
			//this is a local. This was not static in sof2.. but I guess
			//they never check ce.renderfx so it didn't show up.

			const int start = backEnd.currentEntity->e.uRefEnt.uMini.miniStart;
			const int count = backEnd.currentEntity->e.uRefEnt.uMini.miniCount;
			assert(count > 0);
			backEnd.currentEntity = &tempEnt;
			
			assert(backEnd.currentEntity->e.renderfx >= 0);

			for (int i = 0, j = start; i < count; i++, j++)
			{
				backEnd.currentEntity->e = backEnd.refdef.entities[j].e;
				assert(backEnd.currentEntity->e.renderfx >= 0);

				RB_SurfaceEntity(surfType);
			}
		}
		break;
#else
	case RT_LATHE:
		RB_SurfaceLathe();
		break;
	case RT_CLOUDS:
		RB_SurfaceClouds();
		break;
#endif
	default:
		RB_SurfaceAxis();
		break;
	}
	// FIX ME: just a testing hack. Pretty sure we can merge all of these
	tess.shader->entityMergable = qtrue;
}

static void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

static void RB_SurfaceFlare(srfFlare_t *surf)
{
	if (r_flares->integer)
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal);
}

static void RB_SurfaceVBOMesh(srfBspSurface_t * srf)
{
	RB_SurfaceVbo (srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes, srf->firstIndex,
			srf->minIndex, srf->maxIndex, srf->dlightBits, srf->pshadowBits, qfalse );
}

void RB_SurfaceVBOMDVMesh(srfVBOMDVMesh_t * surface)
{
	int i, mergeForward, mergeBack;
	GLvoid *firstIndexOffset, *lastIndexOffset;

	GLimp_LogComment("--- RB_SurfaceVBOMDVMesh ---\n");

	if(!surface->vbo || !surface->ibo)
		return;

	if (glState.currentVBO != surface->vbo)
	{
		RB_EndSurface();
	}

	//FIXME: Implement GPU vertex interpolation instead!
	if (backEnd.currentEntity->e.oldframe != 0 || backEnd.currentEntity->e.frame != 0)
	{
		RB_SurfaceMesh(surface->mdvSurface);
		return;
	}

	R_BindVBO(surface->vbo);
	R_BindIBO(surface->ibo);

	tess.useInternalVBO = qfalse;
	tess.externalIBO = surface->ibo;

	// tess.dlightBits is already set in the renderloop

	// merge this into any existing multidraw primitives
	mergeForward = -1;
	mergeBack = -1;
	firstIndexOffset = BUFFER_OFFSET(surface->indexOffset * sizeof(glIndex_t));
	lastIndexOffset = BUFFER_OFFSET(surface->numIndexes * sizeof(glIndex_t));

	if (r_mergeMultidraws->integer)
	{
		i = 0;

		if (r_mergeMultidraws->integer == 1)
		{
			// lazy merge, only check the last primitive
			if (tess.multiDrawPrimitives)
			{
				i = tess.multiDrawPrimitives - 1;
			}
		}

		for (; i < tess.multiDrawPrimitives; i++)
		{
			if (tess.multiDrawLastIndex[i] == firstIndexOffset)
			{
				mergeBack = i;
			}

			if (lastIndexOffset == tess.multiDrawFirstIndex[i])
			{
				mergeForward = i;
			}
		}
	}

	if (mergeBack != -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += surface->numIndexes;
		tess.multiDrawLastIndex[mergeBack] = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], surface->minIndex);
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], surface->maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack == -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeForward] += surface->numIndexes;
		tess.multiDrawFirstIndex[mergeForward] = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[mergeForward] = tess.multiDrawFirstIndex[mergeForward] + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawMinIndex[mergeForward] = MIN(tess.multiDrawMinIndex[mergeForward], surface->minIndex);
		tess.multiDrawMaxIndex[mergeForward] = MAX(tess.multiDrawMaxIndex[mergeForward], surface->maxIndex);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack != -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += surface->numIndexes + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawLastIndex[mergeBack] = tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack];
		tess.multiDrawMinIndex[mergeBack] = MIN(tess.multiDrawMinIndex[mergeBack], MIN(tess.multiDrawMinIndex[mergeForward], surface->minIndex));
		tess.multiDrawMaxIndex[mergeBack] = MAX(tess.multiDrawMaxIndex[mergeBack], MAX(tess.multiDrawMaxIndex[mergeForward], surface->maxIndex));
		tess.multiDrawPrimitives--;

		if (mergeForward != tess.multiDrawPrimitives)
		{
			tess.multiDrawNumIndexes[mergeForward] = tess.multiDrawNumIndexes[tess.multiDrawPrimitives];
			tess.multiDrawFirstIndex[mergeForward] = tess.multiDrawFirstIndex[tess.multiDrawPrimitives];
		}
		backEnd.pc.c_multidrawsMerged += 2;
	}
	else if (mergeBack == -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[tess.multiDrawPrimitives] = surface->numIndexes;
		tess.multiDrawFirstIndex[tess.multiDrawPrimitives] = (glIndex_t *)firstIndexOffset;
		tess.multiDrawLastIndex[tess.multiDrawPrimitives] = (glIndex_t *)lastIndexOffset;
		tess.multiDrawMinIndex[tess.multiDrawPrimitives] = surface->minIndex;
		tess.multiDrawMaxIndex[tess.multiDrawPrimitives] = surface->maxIndex;
		tess.multiDrawPrimitives++;
	}

	backEnd.pc.c_multidraws++;

	tess.numIndexes += surface->numIndexes;
	tess.numVertexes += surface->numVerts;
}

static void RB_SurfaceSkip( void *surf ) {
}

static void RB_SurfaceSprites( srfSprites_t *surf )
{
	if ( !r_surfaceSprites->integer  || surf->numSprites == 0)
		return;

	RB_EndSurface();

	// TODO: Do we want a 2-level lod system where far away sprites are
	// just flat surfaces?
	
	// TODO: Check which pass (z-prepass/shadow/forward) we're rendering for?
	shader_t *shader = surf->shader;
	shaderStage_t *firstStage = shader->stages[0];
	shaderProgram_t *programGroup = firstStage->glslShaderGroup;
	const surfaceSprite_t *ss = surf->sprite;

	uint32_t shaderFlags = 0;
	if ( surf->alphaTestType != ALPHA_TEST_NONE )
		shaderFlags |= SSDEF_ALPHA_TEST;

	if ( ss->type == SURFSPRITE_ORIENTED )
		shaderFlags |= SSDEF_FACE_CAMERA;

	if (ss->facing == SURFSPRITE_FACING_UP)
		shaderFlags |= SSDEF_FACE_UP;

	if (ss->facing == SURFSPRITE_FACING_NORMAL)
		shaderFlags |= SSDEF_FLATTENED;

	if (ss->type == SURFSPRITE_EFFECT || ss->type == SURFSPRITE_WEATHERFX)
		shaderFlags |= SSDEF_FX_SPRITE;

	if ((firstStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE))
		shaderFlags |= SSDEF_ADDITIVE;

	if (surf->fogIndex > 0 && r_drawfog->integer)
		shaderFlags |= SSDEF_USE_FOG;

	shaderProgram_t *program = programGroup + shaderFlags;
	assert(program->uniformBlocks & (1 << UNIFORM_BLOCK_SURFACESPRITE));

	UniformDataWriter uniformDataWriter;
	uniformDataWriter.Start(program);
	
	// FIXME: Use entity block for this
	uniformDataWriter.SetUniformMatrix4x4(
		UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	uniformDataWriter.SetUniformInt(
		UNIFORM_ALPHA_TEST_TYPE, surf->alphaTestType);

	if (surf->fogIndex != -1)
		uniformDataWriter.SetUniformInt(UNIFORM_FOGINDEX, surf->fogIndex - 1);

	Allocator& frameAllocator = *backEndData->perFrameMemory;

	SamplerBindingsWriter samplerBindingsWriter;
	samplerBindingsWriter.AddAnimatedImage(&firstStage->bundle[0], TB_COLORMAP);

	const GLuint currentFrameUbo = backEndData->currentFrame->ubo;
	const GLuint currentSpriteUbo = shader->spriteUbo;
	const UniformBlockBinding uniformBlockBindings[] = {
		{ currentSpriteUbo, ss->spriteUboOffset, UNIFORM_BLOCK_SURFACESPRITE },
		{ currentFrameUbo, tr.sceneUboOffset, UNIFORM_BLOCK_SCENE },
		{ currentFrameUbo, tr.cameraUboOffsets[tr.viewParms.currentViewParm], UNIFORM_BLOCK_CAMERA },
		{ currentFrameUbo, tr.fogsUboOffset, UNIFORM_BLOCK_FOGS }
	};

	uint32_t numBindings;
	UniformData *spriteUniformData = uniformDataWriter.Finish(frameAllocator);
	SamplerBinding *spriteSamplerBinding = samplerBindingsWriter.Finish(
		frameAllocator, &numBindings);

	int numDrawIndicesUndrawn = surf->numIndices;
	int baseVertex = surf->baseVertex;
	while (numDrawIndicesUndrawn > 5)
	{
		int drawIndices = numDrawIndicesUndrawn > 98298 ? 98298 : numDrawIndicesUndrawn;

		DrawItem item = {};
		item.renderState.stateBits = firstStage->stateBits;
		item.renderState.cullType = CT_TWO_SIDED;
		item.renderState.depthRange = DepthRange{ 0.0f, 1.0f };
		item.program = program;
		item.ibo = surf->ibo;

		item.uniformData = spriteUniformData;

		item.samplerBindings = spriteSamplerBinding;
		item.numSamplerBindings = numBindings;

		DrawItemSetVertexAttributes(
			item, surf->attributes, surf->numAttributes, frameAllocator);
		DrawItemSetUniformBlockBindings(
			item, uniformBlockBindings, frameAllocator);

		item.draw.type = DRAW_COMMAND_INDEXED;
		item.draw.primitiveType = GL_TRIANGLES;
		item.draw.numInstances = 1;
		item.draw.params.indexed.indexType = GL_UNSIGNED_SHORT;
		item.draw.params.indexed.firstIndex = 0;
		item.draw.params.indexed.numIndices = drawIndices;
		item.draw.params.indexed.baseVertex = baseVertex;

		tess.externalIBO = surf->ibo;

		uint32_t RB_CreateSortKey(const DrawItem& item, int stage, int layer);
		uint32_t key = RB_CreateSortKey(item, 0, surf->shader->sort);
		RB_AddDrawItem(backEndData->currentPass, key, item);

		numDrawIndicesUndrawn -= drawIndices;
		baseVertex += ((98298 / 6) * 4);
	}
}

void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD, 
	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP, 
	(void(*)(void*))RB_SurfaceBSPFace,		// SF_FACE,
	(void(*)(void*))RB_SurfaceBSPGrid,		// SF_GRID,
	(void(*)(void*))RB_SurfaceBSPTriangles,	// SF_TRIANGLES,
	(void(*)(void*))RB_SurfacePolychain,	// SF_POLY,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MDV,
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceGhoul,		// SF_MDX,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void(*)(void*))RB_SurfaceVBOMesh,	    // SF_VBO_MESH,
	(void(*)(void*))RB_SurfaceVBOMDVMesh,   // SF_VBO_MDVMESH
	(void(*)(void*))RB_SurfaceSprites,      // SF_SPRITES
	(void(*)(void*))RB_SurfaceWeather,      // SF_WEATHER
};
