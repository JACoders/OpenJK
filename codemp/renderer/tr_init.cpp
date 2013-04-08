//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

#ifndef DEDICATED
#if !defined __TR_WORLDEFFECTS_H
	#include "tr_WorldEffects.h"
#endif
#endif //!DEDICATED

#include "tr_font.h"

#if !defined (MINIHEAP_H_INC)
	#include "qcommon/MiniHeap.h"

#include "ghoul2/G2_local.h"
#endif


//#ifdef __USEA3D
//// Defined in snd_a3dg_refcommon.c
//void RE_A3D_RenderGeometry (void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus);
//#endif

#define G2_VERT_SPACE_SERVER_SIZE 256
CMiniHeap *G2VertSpaceServer = NULL;
CMiniHeap CMiniHeap_singleton(G2_VERT_SPACE_SERVER_SIZE * 1024);

#ifndef DEDICATED
glconfig_t	glConfig;
glstate_t	glState;
static void GfxInfo_f( void );

#endif 


cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;

cvar_t	*r_skipBackEnd;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
// rjr - removed for hacking cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;
cvar_t	*r_autolodscalevalue;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_drawfog;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_cullRoofFaces; //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
cvar_t	*r_roofCullCeilDist; //ceiling distance cull tolerance -rww
cvar_t	*r_roofCullFloorDist; //floor distance cull tolerance -rww
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_autoMap; //automap renderside toggle for debugging -rww
cvar_t	*r_autoMapBackAlpha; //alpha of automap bg -rww
cvar_t	*r_autoMapDisable; //don't calc it (since it's slow in debug) -rww

cvar_t	*r_dlightStyle;
cvar_t	*r_surfaceSprites;
cvar_t	*r_surfaceWeather;

cvar_t	*r_windSpeed;
cvar_t	*r_windAngle;
cvar_t	*r_windGust;
cvar_t	*r_windDampFactor;
cvar_t	*r_windPointForce;
cvar_t	*r_windPointX;
cvar_t	*r_windPointY;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_compressed_lightmaps;
cvar_t	*r_ext_preferred_tc_method;
cvar_t	*r_ext_gamma_control;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;

cvar_t	*r_DynamicGlow;
cvar_t	*r_DynamicGlowPasses;
cvar_t	*r_DynamicGlowDelta;
cvar_t	*r_DynamicGlowIntensity;
cvar_t	*r_DynamicGlowSoft;
cvar_t	*r_DynamicGlowWidth;
cvar_t	*r_DynamicGlowHeight;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_stereo;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t	*r_texturebitslm;

cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_shadowRange;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_markcount;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen = 0;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;

cvar_t	*r_overBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

cvar_t	*r_modelpoolmegs;

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
cvar_t	*r_noPrecacheGLA;
#endif

cvar_t	*r_noServerGhoul2;
cvar_t	*r_Ghoul2AnimSmooth=0;
cvar_t	*r_Ghoul2UnSqashAfterSmooth=0;
//cvar_t	*r_Ghoul2UnSqash;
//cvar_t	*r_Ghoul2TimeBase=0; from single player
//cvar_t	*r_Ghoul2NoLerp;
//cvar_t	*r_Ghoul2NoBlend;
//cvar_t	*r_Ghoul2BlendMultiplier=0;

cvar_t	*broadsword=0;
cvar_t	*broadsword_kickbones=0;
cvar_t	*broadsword_kickorigin=0;
cvar_t	*broadsword_playflop=0;
cvar_t	*broadsword_dontstopanim=0;
cvar_t	*broadsword_waitforshot=0;
cvar_t	*broadsword_smallbbox=0;
cvar_t	*broadsword_extra1=0;
cvar_t	*broadsword_extra2=0;

cvar_t	*broadsword_effcorr=0;
cvar_t	*broadsword_ragtobase=0;
cvar_t	*broadsword_dircap=0;

/*
Ghoul2 Insert End
*/

#ifndef DEDICATED
void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY * qglLockArraysEXT)( GLint, GLint);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

void ( APIENTRY * qglPointParameterfEXT)( GLenum, GLfloat);
void ( APIENTRY * qglPointParameterfvEXT)( GLenum, GLfloat *);

//3d textures -rww
void ( APIENTRY * qglTexImage3DEXT) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void ( APIENTRY * qglTexSubImage3DEXT) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);


// Declare Register Combiners function pointers.
PFNGLCOMBINERPARAMETERFVNV				qglCombinerParameterfvNV = NULL;
PFNGLCOMBINERPARAMETERIVNV				qglCombinerParameterivNV = NULL;
PFNGLCOMBINERPARAMETERFNV				qglCombinerParameterfNV = NULL;
PFNGLCOMBINERPARAMETERINV				qglCombinerParameteriNV = NULL;
PFNGLCOMBINERINPUTNV					qglCombinerInputNV = NULL;
PFNGLCOMBINEROUTPUTNV					qglCombinerOutputNV = NULL;
PFNGLFINALCOMBINERINPUTNV				qglFinalCombinerInputNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERFVNV		qglGetCombinerInputParameterfvNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERIVNV		qglGetCombinerInputParameterivNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNV		qglGetCombinerOutputParameterfvNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNV		qglGetCombinerOutputParameterivNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV	qglGetFinalCombinerInputParameterfvNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV	qglGetFinalCombinerInputParameterivNV = NULL;

#ifdef _WIN32
// Declare Pixel Format function pointers.
PFNWGLGETPIXELFORMATATTRIBIVARBPROC		qwglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC		qwglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC			qwglChoosePixelFormatARB = NULL;

// Declare Pixel Buffer function pointers.
PFNWGLCREATEPBUFFERARBPROC				qwglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC				qwglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC			qwglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC				qwglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC				qwglQueryPbufferARB = NULL;

// Declare Render-Texture function pointers.
PFNWGLBINDTEXIMAGEARBPROC				qwglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC			qwglReleaseTexImageARB = NULL;
PFNWGLSETPBUFFERATTRIBARBPROC			qwglSetPbufferAttribARB = NULL;
#endif

// Declare Vertex and Fragment Program function pointers.
PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC qglBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC qglProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC qglProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC qglProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC qglProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC qglProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC qglProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC qglGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC qglGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC qglGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC qglGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB = NULL;
PFNGLISPROGRAMARBPROC qglIsProgramARB = NULL;

void RE_SetLightStyle(int style, int color);

void RE_GetBModelVerts( int bmodelIndex, vec3_t *verts, vec3_t normal );

#endif // !DEDICATED

static void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral )
{
	if ( shouldBeIntegral )
	{
		if ( ( int ) cv->value != cv->integer )
		{
			Com_Printf (S_COLOR_YELLOW  "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
			Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal )
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
		Cvar_Set( cv->name, va( "%f", minVal ) );
	}
	else if ( cv->value > maxVal )
	{
		Com_Printf (S_COLOR_YELLOW  "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
		Cvar_Set( cv->name, va( "%f", maxVal ) );
	}
}

#ifndef DEDICATED

void R_Splash()
{
	image_t *pImage;
/*	const char* s = Cvar_VariableString("se_language");
	if (stricmp(s,"english"))
	{
		pImage = R_FindImageFile( "menu/splash_eur", qfalse, qfalse, qfalse, GL_CLAMP);
	}
	else
	{
		pImage = R_FindImageFile( "menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
	}
*/
	pImage = R_FindImageFile( "menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
	extern void	RB_SetGL2D (void);
	RB_SetGL2D();	
	if (pImage )
	{//invalid paths?
		GL_Bind( pImage );
	}
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	const int width = 640;
	const int height = 480;
	const float x1 = 320 - width / 2;
	const float x2 = 320 + width / 2;
	const float y1 = 240 - height / 2;
	const float y2 = 240 + height / 2;


	qglBegin (GL_TRIANGLE_STRIP);
		qglTexCoord2f( 0,  0 );
		qglVertex2f(x1, y1);
		qglTexCoord2f( 1 ,  0 );
		qglVertex2f(x2, y1);
		qglTexCoord2f( 0, 1 );
		qglVertex2f(x1, y2);
		qglTexCoord2f( 1, 1 );
		qglVertex2f(x2, y2);
	qglEnd();

	GLimp_EndFrame();
}

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 )
	{
		GLimp_Init();
		// print info the first time only
		GL_SetDefaultState();
		R_Splash();	//get something on screen asap
		GfxInfo_f();
	}
	else
	{
		// set default state
		GL_SetDefaultState();
	}
	// init command buffers and SMP
	R_InitCommandBuffers();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
    int		err;
    char	s[64];

    err = qglGetError();
    if ( err == GL_NO_ERROR ) {
        return;
    }
    if ( r_ignoreGLErrors->integer ) {
        return;
    }
    switch( err ) {
        case GL_INVALID_ENUM:
            strcpy( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            strcpy( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            strcpy( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            strcpy( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            strcpy( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            strcpy( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            Com_sprintf( s, sizeof(s), "%i", err);
            break;
    }

    Com_Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}

#endif //!DEDICATED

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
} vidmode_t;

const vidmode_t r_vidModes[] = {
    { "Mode  0: 320x240",		320,	240 },
    { "Mode  1: 400x300",		400,	300 },
    { "Mode  2: 512x384",		512,	384 },
    { "Mode  3: 640x480",		640,	480 },
    { "Mode  4: 800x600",		800,	600 },
    { "Mode  5: 960x720",		960,	720 },
    { "Mode  6: 1024x768",		1024,	768 },
    { "Mode  7: 1152x864",		1152,	864 },
    { "Mode  8: 1280x1024",		1280,	1024 },
    { "Mode  9: 1600x1200",		1600,	1200 },
    { "Mode 10: 2048x1536",		2048,	1536 },
    { "Mode 11: 856x480 (wide)", 856,	 480 },
    { "Mode 12: 2400x600(surround)",2400,600 }
};
static const int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, int mode ) {
	const vidmode_t	*vm;

    if ( mode < -1 ) {
        return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		return qtrue;
	}

	vm = &r_vidModes[mode];

    *width  = vm->width;
    *height = vm->height;

    return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	Com_Printf ("\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		Com_Printf ("%s\n", r_vidModes[i].description );
	}
	Com_Printf ("\n" );
}

/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 
#ifndef DEDICATED
/* 
================== 
R_TakeScreenshot
================== 
*/  
void R_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;
	int			i, c, temp;

	buffer = (unsigned char *)Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3+18);

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for (i=18 ; i<c ; i+=3) {
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	FS_WriteFile( fileName, buffer, c );

	Hunk_FreeTempMemory( buffer );
}

/* 
================== 
R_TakeScreenshot
================== 
*/  
void R_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;

	buffer = (unsigned char *)Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*4);

	qglReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer ); 

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 4 );
	}

	FS_WriteFile( fileName, buffer, 1 );		// create path
	SaveJPG( fileName, 95, glConfig.vidWidth, glConfig.vidHeight, buffer);

	Hunk_FreeTempMemory( buffer );
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilename( int lastNumber, char *fileName, const char *psExt ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999%s",psExt );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i%s"
		, a, b, c, d, psExt );
}

/*
====================
R_LevelShot

levelshots are specialized 256*256 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
#define LEVELSHOTSIZE 256
static void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	Com_sprintf( checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName );

	source = (unsigned char *)Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = (unsigned char *)Hunk_AllocateTempMemory( LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOTSIZE & 255;
	buffer[13] = LEVELSHOTSIZE >> 8;
	buffer[14] = LEVELSHOTSIZE & 255;
	buffer[15] = LEVELSHOTSIZE >> 8;
	buffer[16] = 24;	// pixel size

	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source ); 

	// resample from source
	xScale = glConfig.vidWidth / (4.0*LEVELSHOTSIZE);
	yScale = glConfig.vidHeight / (3.0*LEVELSHOTSIZE);
	for ( y = 0 ; y < LEVELSHOTSIZE ; y++ ) {
		for ( x = 0 ; x < LEVELSHOTSIZE ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * LEVELSHOTSIZE + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, LEVELSHOTSIZE * LEVELSHOTSIZE * 3 );
	}

	FS_WriteFile( checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18 );

	Hunk_FreeTempMemory( buffer );
	Hunk_FreeTempMemory( source );

	Com_Printf ("Wrote %s\n", checkname );
}

/* 
================== 
R_ScreenShotTGA_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/  
void R_ScreenShotTGA_f (void) {
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname, ".tga" );

			if (!FS_FileExists( checkname ))
			{
				break; // file doesn't exist
			}
		}

		if ( lastNumber >= 9999 ) {
			Com_Printf ( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}


	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent ) {
		Com_Printf ( "Wrote %s\n", checkname);
	}
} 

//jpeg  vession
void R_ScreenShot_f (void) {
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}
	if ( !strcmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname, ".jpg" );

			if (!FS_FileExists( checkname ))
			{
				break; // file doesn't exist
			}
		}

		if ( lastNumber == 10000 ) {
			Com_Printf ( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}


	R_TakeScreenshotJPEG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent ) {
		Com_Printf ( "Wrote %s\n", checkname);
	}
} 

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
}


/*
================
GfxInfo_f
================
*/
extern bool g_bTextureRectangleHack;

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string) {
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0)
	{
		Q_strncpyz(buffer, p, sizeof (buffer) );
		Com_Printf( "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

void GfxInfo_f( void ) 
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	const char *tc_table[] = 
	{
		"None",
		"GL_S3_s3tc",
		"GL_EXT_texture_compression_s3tc",
	};

	Com_Printf ("\nGL_VENDOR: %s\n", glConfig.vendor_string );
	Com_Printf ("GL_RENDERER: %s\n", glConfig.renderer_string );
	Com_Printf ("GL_VERSION: %s\n", glConfig.version_string );
	R_PrintLongString( glConfig.extensions_string );
	Com_Printf ("\n");
	Com_Printf ("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	Com_Printf ("GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures );
	Com_Printf ("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	Com_Printf ("MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		Com_Printf ("%d\n", glConfig.displayFrequency );
	}
	else
	{
		Com_Printf ("N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		Com_Printf ("GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		Com_Printf ("GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		Com_Printf ("rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			Com_Printf ("none\n" );
		} else if ( primitives == 2 ) {
			Com_Printf ("single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			Com_Printf ("multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			Com_Printf ("multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	Com_Printf ("texturemode: %s\n", r_textureMode->string );
	Com_Printf ("picmip: %d\n", r_picmip->integer );
	Com_Printf ("texture bits: %d\n", r_texturebits->integer );
	Com_Printf ("lightmap texture bits: %d\n", r_texturebitslm->integer );
	Com_Printf ("multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	Com_Printf ("compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	Com_Printf ("texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	Com_Printf ("compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );
	Com_Printf ("compressed lightmaps: %s\n", enablestrings[(r_ext_compressed_lightmaps->integer != 0 && glConfig.textureCompression != TC_NONE)] );
	Com_Printf ("texture compression method: %s\n", tc_table[glConfig.textureCompression] );
	Com_Printf ("anisotropic filtering: %s  ", enablestrings[(r_ext_texture_filter_anisotropic->integer != 0) && glConfig.maxTextureFilterAnisotropy] );
		Com_Printf ("(%f of %f)\n", r_ext_texture_filter_anisotropic->value, glConfig.maxTextureFilterAnisotropy );
	Com_Printf ("Dynamic Glow: %s\n", enablestrings[r_DynamicGlow->integer] );
	if (g_bTextureRectangleHack) Com_Printf ("Dynamic Glow ATI BAD DRIVER HACK %s\n", enablestrings[g_bTextureRectangleHack] );

	if ( r_finish->integer ) {
		Com_Printf ("Forcing glFinish\n" );
	}
	if ( r_displayRefresh ->integer ) {
		Com_Printf ("Display refresh set to %d\n", r_displayRefresh->integer );
	}
	if (tr.world)
	{
		Com_Printf ("Light Grid size set to (%.2f %.2f %.2f)\n", tr.world->lightGridSize[0], tr.world->lightGridSize[1], tr.world->lightGridSize[2] );
	}
}

void R_AtiHackToggle_f(void)
{
	g_bTextureRectangleHack = !g_bTextureRectangleHack;
}

#endif // !DEDICATED
/*
===============
R_Register
===============
*/
void R_Register( void ) 
{
	//
	// latched and archived variables
	//
	r_allowExtensions = Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = Cvar_Get( "r_ext_compress_textures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_lightmaps = Cvar_Get( "r_ext_compress_lightmaps", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_preferred_tc_method = Cvar_Get( "r_ext_preferred_tc_method", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_gamma_control = Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
#ifdef __linux__ // broken on linux
	r_ext_texture_env_add = Cvar_Get( "r_ext_texture_env_add", "0", CVAR_ARCHIVE | CVAR_LATCH);
#else
	r_ext_texture_env_add = Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
#endif
	r_ext_texture_filter_anisotropic = Cvar_Get( "r_ext_texture_filter_anisotropic", "16", CVAR_ARCHIVE );

	r_DynamicGlow = Cvar_Get( "r_DynamicGlow", "0", CVAR_ARCHIVE );
	r_DynamicGlowPasses = Cvar_Get( "r_DynamicGlowPasses", "5", CVAR_CHEAT );
	r_DynamicGlowDelta  = Cvar_Get( "r_DynamicGlowDelta", "0.8f", CVAR_CHEAT );
	r_DynamicGlowIntensity = Cvar_Get( "r_DynamicGlowIntensity", "1.13f", CVAR_CHEAT );
	r_DynamicGlowSoft = Cvar_Get( "r_DynamicGlowSoft", "1", CVAR_CHEAT );
	r_DynamicGlowWidth = Cvar_Get( "r_DynamicGlowWidth", "320", CVAR_CHEAT | CVAR_LATCH );
	r_DynamicGlowHeight = Cvar_Get( "r_DynamicGlowHeight", "240", CVAR_CHEAT | CVAR_LATCH );

	r_picmip = Cvar_Get ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	AssertCvarRange( r_picmip, 0, 16, qtrue );
	r_detailTextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebitslm = Cvar_Get( "r_texturebitslm", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stereo = Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH );
#ifdef __linux__
	r_stencilbits = Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
#else
	r_stencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
#endif
	r_depthbits = Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_overBrightBits = Cvar_Get ("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignorehwgamma = Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_customwidth = Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);

	//
	// temporary latched variables that can only change over a restart
	//
	r_displayRefresh = Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH );
	AssertCvarRange( r_displayRefresh, 0, 200, qtrue );
	r_fullbright = Cvar_Get ("r_fullbright", "0", CVAR_CHEAT );
	r_intensity = Cvar_Get ("r_intensity", "1", CVAR_LATCH );
	r_singleShader = Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE );
	r_lodbias = Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_autolodscalevalue = Cvar_Get( "r_autolodscalevalue", "0", CVAR_ROM );

	r_flares = Cvar_Get ("r_flares", "1", CVAR_ARCHIVE );
	r_znear = Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	AssertCvarRange( r_znear, 0.001f, 200, qtrue );
	r_ignoreGLErrors = Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
	r_dynamiclight = Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
// rjr - removed for hacking r_dlightBacks = Cvar_Get( "r_dlightBacks", "1", CVAR_CHEAT );
	r_finish = Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
	r_markcount = Cvar_Get( "r_markcount", "100", CVAR_ARCHIVE );
#ifdef __MACOS__
	r_gamma = Cvar_Get( "r_gamma", "1.2", CVAR_ARCHIVE );
#else
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
#endif
	r_facePlaneCull = Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_cullRoofFaces = Cvar_Get ("r_cullRoofFaces", "0", CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullCeilDist = Cvar_Get ("r_roofCullCeilDist", "256", CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullFloorDist = Cvar_Get ("r_roofCeilFloorDist", "128", CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww

	r_primitives = Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_ambientScale = Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	r_autoMap = Cvar_Get( "r_autoMap", "0", CVAR_ARCHIVE ); //automap renderside toggle for debugging -rww
	r_autoMapBackAlpha = Cvar_Get( "r_autoMapBackAlpha", "0", 0 ); //alpha of automap bg -rww
	r_autoMapDisable = Cvar_Get( "r_autoMapDisable", "1", 0 );

	//
	// temporary variables that can change at any time
	//
	r_showImages = Cvar_Get( "r_showImages", "0", CVAR_CHEAT );

	r_debugLight = Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );

	r_dlightStyle = Cvar_Get ("r_dlightStyle", "1", CVAR_TEMP);
	r_surfaceSprites = Cvar_Get ("r_surfaceSprites", "1", CVAR_TEMP);
	r_surfaceWeather = Cvar_Get ("r_surfaceWeather", "0", CVAR_TEMP);

	r_windSpeed = Cvar_Get ("r_windSpeed", "0", 0);
	r_windAngle = Cvar_Get ("r_windAngle", "0", 0);
	r_windGust = Cvar_Get ("r_windGust", "0", 0);
	r_windDampFactor = Cvar_Get ("r_windDampFactor", "0.1", 0);
	r_windPointForce = Cvar_Get ("r_windPointForce", "0", 0);
	r_windPointX = Cvar_Get ("r_windPointX", "0", 0);
	r_windPointY = Cvar_Get ("r_windPointY", "0", 0);

	r_nocurves = Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_drawfog = Cvar_Get ("r_drawfog", "2", CVAR_CHEAT );
	r_lightmap = Cvar_Get ("r_lightmap", "0", CVAR_CHEAT );
	r_portalOnly = Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_skipBackEnd = Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = Cvar_Get( "r_lodscale", "5", 0 );
	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_lockpvs = Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = Cvar_Get( "cg_shadows", "1", 0 );
	r_shadowRange = Cvar_Get( "r_shadowRange", "1000", 0 );

	r_maxpolys = Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);
/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
	r_noPrecacheGLA = Cvar_Get( "r_noPrecacheGLA", "0", CVAR_CHEAT);
#endif

	r_noServerGhoul2 = Cvar_Get( "r_noserverghoul2", "0", CVAR_CHEAT);

	r_Ghoul2AnimSmooth = Cvar_Get( "r_ghoul2animsmooth", "0.3", 0 );
	r_Ghoul2UnSqashAfterSmooth = Cvar_Get( "r_ghoul2unsqashaftersmooth", "1", 0 );

	broadsword = Cvar_Get( "broadsword", "0", 0);
	broadsword_kickbones = Cvar_Get( "broadsword_kickbones", "1", 0);
	broadsword_kickorigin = Cvar_Get( "broadsword_kickorigin", "1", 0);
	broadsword_dontstopanim = Cvar_Get( "broadsword_dontstopanim", "0", 0);
	broadsword_waitforshot = Cvar_Get( "broadsword_waitforshot", "0", 0);
	broadsword_playflop = Cvar_Get( "broadsword_playflop", "1", 0);
	broadsword_smallbbox = Cvar_Get( "broadsword_smallbbox", "0", 0);
	broadsword_extra1 = Cvar_Get( "broadsword_extra1", "0", 0);
	broadsword_extra2 = Cvar_Get( "broadsword_extra2", "0", 0);
	broadsword_effcorr = Cvar_Get( "broadsword_effcorr", "1", 0);
	broadsword_ragtobase = Cvar_Get( "broadsword_ragtobase", "2", 0);
	broadsword_dircap = Cvar_Get( "broadsword_dircap", "64", 0);
/*
Ghoul2 Insert End
*/
extern qboolean Sys_LowPhysicalMemory();
	r_modelpoolmegs = Cvar_Get("r_modelpoolmegs", "20", CVAR_ARCHIVE);
	if (Sys_LowPhysicalMemory() )
	{
		Cvar_Set("r_modelpoolmegs", "0");
	}

	// make sure all the commands added here are also
	// removed in R_Shutdown
#ifndef DEDICATED
	Cmd_AddCommand( "imagelist", R_ImageList_f );
	Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	Cmd_AddCommand( "skinlist", R_SkinList_f );
	Cmd_AddCommand( "fontlist", R_FontList_f );
	Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	Cmd_AddCommand( "screenshot_tga", R_ScreenShotTGA_f );
	Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	Cmd_AddCommand( "r_atihack", R_AtiHackToggle_f );
	Cmd_AddCommand( "r_we", R_WorldEffect_f);
	Cmd_AddCommand( "imagecacheinfo", RE_RegisterImages_Info_f);
#endif
	Cmd_AddCommand( "modellist", R_Modellist_f );
	Cmd_AddCommand( "modelist", R_ModeList_f );
	Cmd_AddCommand( "modelcacheinfo", RE_RegisterModels_Info_f);

}


/*
===============
R_Init
===============
*/
extern void R_InitWorldEffects(void); //tr_WorldEffects.cpp
void R_Init( void ) {	
	int i;
	byte *ptr;

//	Com_Printf ("----- R_Init -----\n" );
	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
#ifndef DEDICATED
	Com_Memset( &tess, 0, sizeof( tess ) );
#endif

//	Swap_Init();

#ifndef DEDICATED
#ifndef FINAL_BUILD
	if ( (int)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned (%x)\n",(int)tess.xyz & 15 );
	}
#endif
#endif
	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}
#ifndef DEDICATED
	R_InitFogTable();

	R_NoiseInit();
#endif
	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = (byte *)Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);
#ifndef DEDICATED
	R_ToggleSmpFrame();

	for(i = 0; i < MAX_LIGHT_STYLES; i++)
	{
		RE_SetLightStyle(i, -1);
	}
	InitOpenGL();

	R_InitImages();
	R_InitShaders(qfalse);
	R_InitSkins();

	R_TerrainInit(); //rwwRMG - added

	R_InitFonts();
#endif
	R_ModelInit();
	G2VertSpaceServer = &CMiniHeap_singleton;
#ifndef DEDICATED
	R_InitDecals ( );

	R_InitWorldEffects();

	int	err = qglGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf ( "glGetError() = 0x%x\n", err);
#endif
//	Com_Printf ("----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

//	Com_Printf ("RE_Shutdown( %i )\n", destroyWindow );

	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("shaderlist");
	Cmd_RemoveCommand ("skinlist");
	Cmd_RemoveCommand ("fontlist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("screenshot_tga");
	Cmd_RemoveCommand ("gfxinfo");
	Cmd_RemoveCommand ("r_atihack");
	Cmd_RemoveCommand ("r_we");
	Cmd_RemoveCommand ("imagecacheinfo");
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("modelist");
	Cmd_RemoveCommand ("modelcacheinfo");
#ifndef DEDICATED

	if ( r_DynamicGlow && r_DynamicGlow->integer )
	{
		// Release the Glow Vertex Shader.
		if ( tr.glowVShader )
		{
			qglDeleteProgramsARB( 1, &tr.glowVShader );
		}

		// Release Pixel Shader.
		if ( tr.glowPShader )
		{
			if ( qglCombinerParameteriNV  )
			{
				// Release the Glow Regcom call list.
				qglDeleteLists( tr.glowPShader, 1 );
			}
			else if ( qglGenProgramsARB )
			{
				// Release the Glow Fragment Shader.
				qglDeleteProgramsARB( 1, &tr.glowPShader );
			}
		}

		// Release the scene glow texture.
		qglDeleteTextures( 1, &tr.screenGlow );

		// Release the scene texture.
		qglDeleteTextures( 1, &tr.sceneImage );

		// Release the blur texture.
		qglDeleteTextures( 1, &tr.blurImage );
	}

	R_TerrainShutdown(); //rwwRMG - added

	R_ShutdownFonts();
	if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		if (destroyWindow)
		{
			R_DeleteTextures();		// only do this for vid_restart now, not during things like map load
		}
	}

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();
	}
#endif //!DEDICATED

	tr.registered = qfalse;
}

#ifndef DEDICATED

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if (!Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

void RE_GetLightStyle(int style, color4ub_t color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_GetLightStyle: %d is out of range", (int)style );
		return;
	}

	*(int *)color = *(int *)styleColors[style];
}

void RE_SetLightStyle(int style, int color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_SetLightStyle: %d is out of range", (int)style );
		return;
	}

	if (*(int*)styleColors[style] != color)
	{
		*(int *)styleColors[style] = color;
	}
}

#endif //!DEDICATED
/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI ( int apiVersion ) {
	static refexport_t	re;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		Com_Printf ( "Mismatched REF_API_VERSION: expected %i, got %i\n", 
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;
#ifndef DEDICATED
	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.ShaderNameFromIndex = RE_ShaderNameFromIndex;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.DrawRotatePic = RE_RotatePic;
	re.DrawRotatePic2 = RE_RotatePic2;

	re.ClearScene = RE_ClearScene;
	re.ClearDecals = RE_ClearDecals;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddMiniRefEntityToScene = RE_AddMiniRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.AddDecalToScene = RE_AddDecalToScene;
	re.LightForPoint = R_LightForPoint;
#ifndef VV_LIGHTING
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
#endif
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.Font_StrLenPixels = RE_Font_StrLenPixels;
	re.Font_StrLenChars = RE_Font_StrLenChars;
	re.Font_HeightPixels = RE_Font_HeightPixels;
	re.Font_DrawString = RE_Font_DrawString;
	re.Language_IsAsian = Language_IsAsian;
	re.Language_UsesSpaces = Language_UsesSpaces;
	re.AnyLanguage_ReadCharFromString = AnyLanguage_ReadCharFromString;

	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.GetLightStyle = RE_GetLightStyle;
	re.SetLightStyle = RE_SetLightStyle;

	re.GetBModelVerts = RE_GetBModelVerts;
#endif //!DEDICATED
	return &re;
}

