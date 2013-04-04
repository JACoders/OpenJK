// Filename:-	textures.h
//

#ifndef TEXTURES_H
#define TEXTURES_H


typedef struct
{
	char		sName[MAX_QPATH];	// game-relative path (without extension)
	char		sExt[20];			// ".tga" etc (extra space for stuff like ".shader" maybe?)
	bool		bIsDefault;			// false in all but one cases
	byte		*pPixels;           // actual texture bitmap
	int			iWidth, iHeight;	// size of TextureData
	GLuint		gluiBind;			// id the texture is actually bound to (may be zero if file was missing)
	GLuint		gluiDesiredBind;	// id the texture would be bound to if present (used for refresh purposes)
	int			iUsageCount;		
	FILETIME	ft;
	bool		bFTValid;
} Texture_t;


void Texture_SetFilter(void);
void TextureList_SetFilter(void);
void TextureList_OnceOnlyInit(void);
void TextureList_DeleteAll(void);
int  TextureList_GetMip(void);
void TextureList_ReMip(int iMIPLevel);
void TextureList_Refresh(void);
TextureHandle_t TextureHandle_ForName(LPCSTR psLocalTexturePath);
GLuint Texture_GetGLBind( TextureHandle_t thHandle);
Texture_t *Texture_GetTextureData( TextureHandle_t thHandle );
int  Texture_Load( LPCSTR psLocalTexturePath, bool bInhibitStatus = false );


void FakeCvars_Shutdown(void);
void FakeCvars_OnceOnlyInit(void);

void OnceOnly_GLVarsInit(void);

#endif	// #ifndef TEXTURES_H



//////////////////// eof //////////////////

