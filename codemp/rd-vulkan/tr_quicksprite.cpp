/*
===========================================================================
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

// tr_QuickSprite.cpp: implementation of the CQuickSpriteSystem class.
//
//////////////////////////////////////////////////////////////////////
#include "tr_local.h"

#include "tr_quicksprite.h"

//////////////////////////////////////////////////////////////////////
// Singleton System
//////////////////////////////////////////////////////////////////////
CQuickSpriteSystem SQuickSprite;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickSpriteSystem::CQuickSpriteSystem() :
	vk_pipeline(0),
	mTexBundle(NULL),
	mFogIndex(-1),
	mUseFog(qfalse)
{
	uint32_t i;

	memset(mFogTextureCoords, 0, sizeof(mFogTextureCoords));

	for (i = 0; i < SHADER_MAX_VERTEXES; i += 4)
	{
		// Bottom right
		mTextureCoords[i + 0][0] = 1.0;
		mTextureCoords[i + 0][1] = 1.0;
		// Top right
		mTextureCoords[i + 1][0] = 1.0;
		mTextureCoords[i + 1][1] = 0.0;
		// Top left
		mTextureCoords[i + 2][0] = 0.0;
		mTextureCoords[i + 2][1] = 0.0;
		// Bottom left
		mTextureCoords[i + 3][0] = 0.0;
		mTextureCoords[i + 3][1] = 1.0;
	}
}

CQuickSpriteSystem::~CQuickSpriteSystem()
{

}

void CQuickSpriteSystem::Flush( void )
{
	if (tess.numIndexes == 0)
		return;

	vk_select_texture(0);
	R_BindAnimatedImage(mTexBundle);

	tess.svars.texcoordPtr[0] = mTextureCoords;
	
	vk_bind_pipeline(vk_pipeline);
	vk_bind_index();
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0 | TESS_ST0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);

	//only for software fog pass (global soft/volumetric) -rww
	//if (mUseFog && (r_drawfog->integer != 2 || mFogIndex != tr.world->globalFog))

	// surface sprite fogs are rendered without fog collapse
	if (mUseFog)
	{
		uint32_t pipeline = vk.std_pipeline.fog_pipelines[0][tess.shader->fogPass - 1][2][tess.shader->polygonOffset];
		const fog_t *fog;
		int			i;

		fog = tr.world->fogs + mFogIndex;

		for (i = 0; i < tess.numVertexes; i++) {
			*(int*)&tess.svars.colors[0][i] = fog->colorInt;
		}

		RB_CalcFogTexCoords((float*)mFogTextureCoords);
		tess.svars.texcoordPtr[0] = mFogTextureCoords;

		vk_bind(tr.fogImage);
		vk_bind_pipeline(pipeline);
		vk_bind_geometry(TESS_ST0 | TESS_RGBA0);
		vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);
	}

	tess.numVertexes = 0;
	tess.numIndexes = 0;
}

void CQuickSpriteSystem::StartGroup( const textureBundle_t *bundle, uint32_t pipeline, int fogIndex )
{
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	vk_pipeline = pipeline;
	mTexBundle = bundle;
	if (fogIndex != -1)
	{
		mUseFog = qtrue;
		mFogIndex = fogIndex;
	}
	else
	{
		mUseFog = qfalse;
	}
}

void CQuickSpriteSystem::EndGroup( void )
{
	Flush();

	//qglColor4ub(255,255,255,255);
}

void CQuickSpriteSystem::Add( float *pointdata, color4ub_t color, vec2_t fog )
{
	float *curcoord;
	float *curfogtexcoord;
	uint32_t i;

	if (tess.numVertexes > SHADER_MAX_VERTEXES - 4)
		Flush();

	curcoord = tess.xyz[tess.numVertexes];
	// This is 16*sizeof(float) because, pointdata comes from a float[16]
	memcpy(curcoord, pointdata, 16 * sizeof(float));

	// Set up color
	for (i = 0; i < 4; i++) {
		memcpy(tess.svars.colors[0][tess.numVertexes + i], color, sizeof(color4ub_t));
	}

	for (i = 0; i < 6; i++) {
		tess.indexes[tess.numIndexes] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + 3;
		tess.indexes[tess.numIndexes + 3] = tess.numVertexes + 3;
		tess.indexes[tess.numIndexes + 4] = tess.numVertexes + 1;
		tess.indexes[tess.numIndexes + 5] = tess.numVertexes + 2;
	}

	if (fog)
	{
		curfogtexcoord = &mFogTextureCoords[tess.numVertexes][0];
		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		mUseFog = qtrue;
	}
	else
	{
		mUseFog = qfalse;
	}

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}