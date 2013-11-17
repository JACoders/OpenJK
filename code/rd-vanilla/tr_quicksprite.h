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

// this include must remain at the top of every CPP file
#include "tr_local.h"

// tr_QuickSprite.h: interface for the CQuickSprite class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TR_QUICKSPRITE_H__6F05EB85_A1ED_4537_9EC0_9F5D82A5D99A__INCLUDED_)
#define AFX_TR_QUICKSPRITE_H__6F05EB85_A1ED_4537_9EC0_9F5D82A5D99A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CQuickSpriteSystem
{
private:
			textureBundle_t	*mTexBundle;
			uint32_t	mGLStateBits;
			int				mFogIndex;
			qboolean		mUseFog;
			vec4_t			mVerts[SHADER_MAX_VERTEXES];
			vec2_t			mTextureCoords[SHADER_MAX_VERTEXES];	// Ideally this would be static, cause it never changes
			vec2_t			mFogTextureCoords[SHADER_MAX_VERTEXES];
			uint32_t		mColors[SHADER_MAX_VERTEXES];
			int				mNextVert;
			qboolean		mTurnCullBackOn;

			void Flush(void);

public:
			CQuickSpriteSystem(void);
			~CQuickSpriteSystem(void);

			void StartGroup(textureBundle_t *bundle, uint32_t glbits, int fogIndex = -1);
			void EndGroup(void);

			void Add(float *pointdata, color4ub_t color, vec2_t fog=NULL);
};

extern CQuickSpriteSystem SQuickSprite;


#endif


