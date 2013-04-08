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

#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"

#ifdef _XBOX
// Get font functions with default arguments that we need below
#include "tr_font.h"
#endif

#define	REF_API_VERSION		9

//
// these are the functions exported by the refresh module
//
#ifdef _XBOX
template <class T> class SPARC;
#endif
typedef struct {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void	(*Shutdown)( qboolean destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void	(*BeginRegistration)( glconfig_t *config );
	qhandle_t (*RegisterModel)( const char *name );
	qhandle_t (*RegisterSkin)( const char *name );
	int		  (*GetAnimationCFG)(const char *psCFGFilename, char *psDest, int iDestSize);
	qhandle_t (*RegisterShader)( const char *name );
	qhandle_t (*RegisterShaderNoMip)( const char *name );
	void	(*LoadWorld)( const char *name );

	// these two functions added to help with the new model alloc scheme...
	//
	void	(*RegisterMedia_LevelLoadBegin)(const char *psMapName, ForceReload_e eForceReload, qboolean bAllowScreenDissolve);
	void	(*RegisterMedia_LevelLoadEnd)(void);

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
#ifdef _XBOX
	void	(*SetWorldVisData)( SPARC<byte> *vis );
#else
	void	(*SetWorldVisData)( const byte *vis );
#endif

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void	(*EndRegistration)( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void	(*ClearScene)( void );
	void	(*AddRefEntityToScene)( const refEntity_t *re );
	void	(*AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts );
	void	(*AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void	(*RenderScene)( const refdef_t *fd );
	qboolean(*GetLighting)( const vec3_t org, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);

	void	(*SetColor)( const float *rgba );	// NULL = 1,1,1,1
	void	(*DrawStretchPic) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white
	void	(*DrawRotatePic) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white
	void	(*DrawRotatePic2) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white
	void	(*LAGoggles)(void);
	void	(*Scissor) ( float x, float y, float w, float h);	// 0 = white

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void	(*UploadCinematic) (int cols, int rows, const byte *data, int client, qboolean dirty);

	void	(*BeginFrame)( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void	(*EndFrame)( int *frontEndMsec, int *backEndMsec );

	qboolean (*ProcessDissolve)(void);
	qboolean (*InitDissolve)(qboolean bForceCircularExtroWipe);


	// for use with save-games mainly...
	void	(*GetScreenShot)(byte *data, int w, int h);

	// this is so you can get access to raw pixels from a graphics format (TGA/JPG/BMP etc), 
	//	currently only the save game uses it (to make raw shots for the autosaves)
	//
	byte*	(*TempRawImage_ReadFromFile)(const char *psLocalFilename, int *piWidth, int *piHeight, byte *pbReSampleBuffer, qboolean qbVertFlip);
	void	(*TempRawImage_CleanUp)();

	//misc stuff
	int		(*MarkFragments)( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	//model stuff
	void	(*LerpTag)( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, 
					 float frac, const char *tagName );
	void	(*ModelBounds)( qhandle_t model, vec3_t mins, vec3_t maxs );

	void	(*GetLightStyle)(int style, color4ub_t color);
	void	(*SetLightStyle)(int style, int color);

	void	(*GetBModelVerts)( int bmodelIndex, vec3_t *vec, vec3_t normal );
	void	(*WorldEffectCommand)(const char *command);

	int		(*RegisterFont)(const char *name);
#ifdef _XBOX	// No default arguments through function pointers.
	int		Font_HeightPixels(const int index, const float scale = 1.0f)
	{
		return RE_Font_HeightPixels(index, scale);
	}
	int		Font_StrLenPixels(const char *s, const int index, const float scale = 1.0f)
	{
		return RE_Font_StrLenPixels(s, index, scale);
	}
	void	Font_DrawString(int x, int y, const char *s, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float scale = 1.0f)
	{
		return RE_Font_DrawString(x, y, s, rgba, iFontHandle, iMaxPixelWidth, scale);
	}
#else
	int		(*Font_HeightPixels)(const int index, const float scale);
	int		(*Font_StrLenPixels)(const char *s, const int index, const float scale);
	void	(*Font_DrawString)(int x, int y, const char *s, const float *rgba, const int iFontHandle, int iMaxPixelWidth, const float scale);
#endif
	int		(*Font_StrLenChars) (const char *s);
	qboolean (*Language_IsAsian) (void);
	qboolean (*Language_UsesSpaces) (void);
	unsigned int (*AnyLanguage_ReadCharFromString)( char *psText, int * piAdvanceCount, qboolean *pbIsTrailingPunctuation /* = NULL */);
	unsigned int (*AnyLanguage_ReadCharFromString2)( char **psText, qboolean *pbIsTrailingPunctuation /* = NULL */);
} refexport_t;


// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
refexport_t*GetRefAPI( int apiVersion );

#endif	// __TR_PUBLIC_H
