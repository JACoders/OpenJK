#pragma once

// tr_QuickSprite.h: interface for the CQuickSprite class.
//
//////////////////////////////////////////////////////////////////////

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
			uint32_t	mColors[SHADER_MAX_VERTEXES];
			int				mNextVert;

			void Flush(void);

public:
			CQuickSpriteSystem();
	virtual ~CQuickSpriteSystem();

			void StartGroup(textureBundle_t *bundle, uint32_t glbits, int fogIndex = -1);
			void EndGroup(void);

			void Add(float *pointdata, color4ub_t color, vec2_t fog=NULL);
};

extern CQuickSpriteSystem SQuickSprite;
