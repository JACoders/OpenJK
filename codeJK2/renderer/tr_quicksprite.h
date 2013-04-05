// this include must remain at the top of every CPP file
//#include "../game/q_math.h"
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
			unsigned long	mGLStateBits;
			unsigned long	mFogColor;
			qboolean		mUseFog;
			vec4_t			mVerts[SHADER_MAX_VERTEXES];
			unsigned int	mIndexes[SHADER_MAX_VERTEXES];			// Ideally this would be static, cause it never changes
			vec2_t			mTextureCoords[SHADER_MAX_VERTEXES];	// Ideally this would be static, cause it never changes
			vec2_t			mFogTextureCoords[SHADER_MAX_VERTEXES];
			unsigned long	mColors[SHADER_MAX_VERTEXES];
			int				mNextVert;
			qboolean		mTurnCullBackOn;

			void Flush(void);

public:
			CQuickSpriteSystem(void);
			~CQuickSpriteSystem(void);

			void StartGroup(textureBundle_t *bundle, unsigned long glbits, unsigned long fogcolor=0x00000000);
			void EndGroup(void);

			void Add(float *pointdata, color4ub_t color, vec2_t fog=NULL);
};

extern CQuickSpriteSystem SQuickSprite;


#endif // !defined(AFX_TR_QUICKSPRITE_H__6F05EB85_A1ED_4537_9EC0_9F5D82A5D99A__INCLUDED_)


