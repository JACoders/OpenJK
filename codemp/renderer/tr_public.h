#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "cgame/tr_types.h"

#define	REF_API_VERSION		8

//
// these are the functions exported by the refresh module
//
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
	qhandle_t (*RegisterShader)( const char *name );
	qhandle_t (*RegisterShaderNoMip)( const char *name );
	const char *(*ShaderNameFromIndex)( int index );
	void	(*LoadWorld)( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void	(*SetWorldVisData)( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void	(*EndRegistration)( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void	(*ClearScene)( void );
	void	(*ClearDecals) ( void );
	void	(*AddRefEntityToScene)( const refEntity_t *re );
	void	(*AddMiniRefEntityToScene)( const miniRefEntity_t *re );
	void	(*AddPolyToScene)( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
	void	(*AddDecalToScene)(qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary );
	int		(*LightForPoint)( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
#ifndef VV_LIGHTING
	void	(*AddLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
	void	(*AddAdditiveLightToScene)( const vec3_t org, float intensity, float r, float g, float b );
#endif
	void	(*RenderScene)( const refdef_t *fd );

	void	(*SetColor)( const float *rgba );	// NULL = 1,1,1,1
	void	(*DrawStretchPic) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, qhandle_t hShader );	// 0 = white
	void	(*DrawRotatePic) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white
	void	(*DrawRotatePic2) ( float x, float y, float w, float h, 
		float s1, float t1, float s2, float t2, float a1, qhandle_t hShader );	// 0 = white

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
	void	(*UploadCinematic) (int cols, int rows, const byte *data, int client, qboolean dirty);

	void	(*BeginFrame)( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void	(*EndFrame)( int *frontEndMsec, int *backEndMsec );


	int		(*MarkFragments)( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int		(*LerpTag)( orientation_t *tag,  qhandle_t model, int startFrame, int endFrame, 
					 float frac, const char *tagName );
	void	(*ModelBounds)( qhandle_t model, vec3_t mins, vec3_t maxs );

#ifdef __USEA3D
	void    (*A3D_RenderGeometry) (void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus);
#endif

	qhandle_t (*RegisterFont)( const char *fontName );
	int		(*Font_StrLenPixels) (const char *text, const int iFontIndex, const float scale);
	int		(*Font_StrLenChars) (const char *text);
	int		(*Font_HeightPixels)(const int iFontIndex, const float scale);
	void	(*Font_DrawString)(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale);
	qboolean (*Language_IsAsian)(void);
	qboolean (*Language_UsesSpaces)(void);
	unsigned int (*AnyLanguage_ReadCharFromString)( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation/* = NULL*/ );

	void	(*RemapShader)(const char *oldShader, const char *newShader, const char *offsetTime);
	qboolean (*GetEntityToken)( char *buffer, int size );
	qboolean (*inPVS)( const vec3_t p1, const vec3_t p2, byte *mask );

	void (*GetLightStyle)(int style, color4ub_t color);
	void (*SetLightStyle)(int style, int color);

	void	(*GetBModelVerts)( int bmodelIndex, vec3_t *vec, vec3_t normal );

	void (*TakeVideoFrame)( int h, int w, byte* captureBuffer, byte *encodeBuffer, qboolean motionJpeg );
} refexport_t;


// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
refexport_t*GetRefAPI( int apiVersion );

#endif	// __TR_PUBLIC_H
