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

void R_BindAnimatedImage( textureBundle_t *bundle );


//////////////////////////////////////////////////////////////////////
// Singleton System
//////////////////////////////////////////////////////////////////////
CQuickSpriteSystem SQuickSprite;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickSpriteSystem::CQuickSpriteSystem() :
	mTexBundle(NULL),
	mGLStateBits(0),
	mFogIndex(-1),
	mUseFog(qfalse),
	mNextVert(0)
{
	int i;

	memset( mVerts, 0, sizeof( mVerts ) );
	memset( mFogTextureCoords, 0, sizeof( mFogTextureCoords ) );
	memset( mColors, 0, sizeof( mColors ) );

	for (i=0; i<SHADER_MAX_VERTEXES; i+=4)
	{
		// Bottom right
		mTextureCoords[i+0][0] = 1.0;
		mTextureCoords[i+0][1] = 1.0;
		// Top right
		mTextureCoords[i+1][0] = 1.0;
		mTextureCoords[i+1][1] = 0.0;
		// Top left
		mTextureCoords[i+2][0] = 0.0;
		mTextureCoords[i+2][1] = 0.0;
		// Bottom left
		mTextureCoords[i+3][0] = 0.0;
		mTextureCoords[i+3][1] = 1.0;
	}
}

CQuickSpriteSystem::~CQuickSpriteSystem()
{

}


void CQuickSpriteSystem::Flush(void)
{
	if (mNextVert==0)
	{
		return;
	}

	/*
	if (mUseFog && r_drawfog->integer == 2 &&
		mFogIndex == tr.world->globalFog)
	{ //enable hardware fog when we draw this thing if applicable -rww
		fog_t *fog = tr.world->fogs + mFogIndex;

		qglFogf(GL_FOG_MODE, GL_EXP2);
		qglFogf(GL_FOG_DENSITY, logtestExp2 / fog->parms.depthForOpaque);
		qglFogfv(GL_FOG_COLOR, fog->parms.color);
		qglEnable(GL_FOG);
	}
	*/
	//this should not be needed, since I just wait to disable fog for the surface til after surface sprites are done

	//
	// render the main pass
	//
	R_BindAnimatedImage( mTexBundle );
	GL_State(mGLStateBits);

	//
	// set arrays and lock
	//
	qglTexCoordPointer( 2, GL_FLOAT, 0, mTextureCoords );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

	qglEnableClientState( GL_COLOR_ARRAY);
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, mColors );

	qglVertexPointer (3, GL_FLOAT, 16, mVerts);

	if ( qglLockArraysEXT )
	{
		qglLockArraysEXT(0, mNextVert);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	qglDrawArrays(GL_QUADS, 0, mNextVert);

	backEnd.pc.c_vertexes += mNextVert;
	backEnd.pc.c_indexes += mNextVert;
	backEnd.pc.c_totalIndexes += mNextVert;

	//only for software fog pass (global soft/volumetric) -rww
	if (mUseFog && (r_drawfog->integer != 2 || mFogIndex != tr.world->globalFog))
	{
		fog_t *fog = tr.world->fogs + mFogIndex;

		//
		// render the fog pass
		//
		GL_Bind( tr.fogImage );
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		//
		// set arrays and lock
		//
		qglTexCoordPointer( 2, GL_FLOAT, 0, mFogTextureCoords);
//		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);	// Done above

		qglDisableClientState( GL_COLOR_ARRAY );
		qglColor4ubv((GLubyte *)&fog->colorInt);

//		qglVertexPointer (3, GL_FLOAT, 16, mVerts);	// Done above

		qglDrawArrays(GL_QUADS, 0, mNextVert);

		// Second pass from fog
		backEnd.pc.c_totalIndexes += mNextVert;
	}

	//
	// unlock arrays
	//
	if (qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	mNextVert=0;
}


void CQuickSpriteSystem::StartGroup(textureBundle_t *bundle, uint32_t glbits, int fogIndex )
{
	mNextVert = 0;

	mTexBundle = bundle;
	mGLStateBits = glbits;
	if (fogIndex != -1)
	{
		mUseFog = qtrue;
		mFogIndex = fogIndex;
	}
	else
	{
		mUseFog = qfalse;
	}

	qglDisable(GL_CULL_FACE);
}


void CQuickSpriteSystem::EndGroup(void)
{
	Flush();

	qglColor4ub(255,255,255,255);
	qglEnable(GL_CULL_FACE);
}




void CQuickSpriteSystem::Add(float *pointdata, color4ub_t color, vec2_t fog)
{
	float *curcoord;
	float *curfogtexcoord;
	uint32_t *curcolor;

	if (mNextVert>SHADER_MAX_VERTEXES-4)
	{
		Flush();
	}

	curcoord = mVerts[mNextVert];
	// This is 16*sizeof(float) because, pointdata comes from a float[16]
	memcpy(curcoord, pointdata, 16*sizeof(float));

	// Set up color
	curcolor = &mColors[mNextVert];
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;
	*curcolor++ = *(uint32_t *)color;

	if (fog)
	{
		curfogtexcoord = &mFogTextureCoords[mNextVert][0];
		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		*curfogtexcoord++ = fog[0];
		*curfogtexcoord++ = fog[1];

		mUseFog=qtrue;
	}
	else
	{
		mUseFog=qfalse;
	}

	mNextVert+=4;
}
