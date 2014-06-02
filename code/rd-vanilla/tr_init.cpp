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

// tr_init.c -- functions that are not called every frame

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#include "tr_local.h"
#include "../rd-common/tr_common.h"
#include "tr_stl.h"
#include "../rd-common/tr_font.h"
#include "tr_WorldEffects.h"

glconfig_t	glConfig;
glstate_t	glState;

static void GfxInfo_f( void );

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;

cvar_t	*r_skipBackEnd;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_drawfog;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

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
cvar_t	*r_shadows;
cvar_t	*r_shadowRange;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showtriscolor;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
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

cvar_t	*r_fullscreen;
cvar_t	*r_noborder;
cvar_t	*r_centerWindow;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_debugStyle;

cvar_t	*r_modelpoolmegs;

/*
Ghoul2 Insert Start
*/

cvar_t	*r_noGhoul2;
cvar_t	*r_Ghoul2AnimSmooth;
cvar_t	*r_Ghoul2UnSqash;
cvar_t	*r_Ghoul2TimeBase=0;
cvar_t	*r_Ghoul2NoLerp;
cvar_t	*r_Ghoul2NoBlend;
cvar_t	*r_Ghoul2BlendMultiplier=0;
cvar_t	*r_Ghoul2UnSqashAfterSmooth;

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

// More bullshit needed for the proper modular renderer --eez
cvar_t	*sv_mapname;
cvar_t	*sv_mapChecksum;
cvar_t	*se_language;			// JKA
#ifdef JK2_MODE
cvar_t	*sp_language;			// JK2
#endif
cvar_t	*com_buildScript;

cvar_t	*r_environmentMapping;
cvar_t *r_screenshotJpegQuality;


/*
Ghoul2 Insert End
*/

#ifdef _WIN32
void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY * qglLockArraysEXT)( GLint, GLint);
void ( APIENTRY * qglUnlockArraysEXT) ( void );
#endif

#ifdef _WIN32	// GLOWXXX
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
#endif

void RE_SetLightStyle(int style, int color);

void R_Splash()
{
	image_t *pImage;

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
		// set default state
		GL_SetDefaultState();
#ifndef JK2_MODE
		R_Splash();	//get something on screen asap
#endif
		GfxInfo_f();
	}
	else
	{
		// set default state
		GL_SetDefaultState();
	}
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

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
} vidmode_t;

const vidmode_t r_vidModes[] =
{
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

	ri.Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================

RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with Hunk_FreeTempMemory()
==================
*/

byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = (byte *)ri.Z_Malloc(padwidth * height + *offset + packAlign - 1, TAG_TEMP_WORKSPACE, qfalse, 4);

	bufstart = (byte *)PADP((intptr_t) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
================== 
R_TakeScreenshot
================== 
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte *allbuf, *buffer;
	byte *srcptr, *destptr;
	byte *endline, *endmem;
	byte temp;

	int linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while(srcptr < endmem)
	{
		endline = srcptr + linelen;

		while(srcptr < endline)
		{
			temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(allbuf + offset, memcount);

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Z_Free(allbuf);
}

/* 
================== 
R_TakeScreenshotPNG
================== 
*/  
void R_TakeScreenshotPNG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer=NULL;
	size_t offset=0;
	int padlen=0;

	buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	RE_SavePNG( fileName, buffer, width, height, 3 );
	ri.Z_Free( buffer );
}

/* 
================== 
R_TakeScreenshotJPEG
================== 
*/  
void R_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.Z_Free(buffer);
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilename( char *buf, int bufSize, const char *ext ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "screenshots/shot%s%s", timeStr, ext );
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
	byte		*source, *allsource;
	byte		*src, *dst;
	size_t		offset = 0;
	int			padlen;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	Com_sprintf( checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName );

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source = allsource + offset;

	buffer = (byte *)ri.Z_Malloc(LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18, TAG_TEMP_WORKSPACE, qfalse, 4);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOTSIZE & 255;
	buffer[13] = LEVELSHOTSIZE >> 8;
	buffer[14] = LEVELSHOTSIZE & 255;
	buffer[15] = LEVELSHOTSIZE >> 8;
	buffer[16] = 24;	// pixel size

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

	ri.FS_WriteFile( checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18 );

	ri.Z_Free( buffer );
	ri.Z_Free( allsource );

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
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".tga" );

		if ( ri.FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
}

/* 
================== 
R_ScreenShotPNG_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/
void R_ScreenShotPNG_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.png", ri.Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".png" );

		if ( ri.FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshotPNG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
} 

void R_ScreenShot_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}
	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".jpg" );

		if ( ri.FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n" );
			return;
 		}
	}

	R_TakeScreenshotJPEG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent )
		Com_Printf( "Wrote %s\n", checkname );
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
	qglDisable( GL_ALPHA_TEST );
	qglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}


/*
================
R_LongString

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

/*
================
GfxInfo_f
================
*/
extern bool g_bTextureRectangleHack;

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
	const char *noborderstrings[] =
	{
		"",
		"noborder "
	};

	const char *tc_table[] = 
	{
		"None",
		"GL_S3_s3tc",
		"GL_EXT_texture_compression_s3tc",
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	R_PrintLongString(glConfig.extensions_string);
	Com_Printf ("\n");
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s%s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, r_fullscreen->integer == 0 ? noborderstrings[r_noborder->integer == 1] : noborderstrings[0] ,fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else
	{
		ri.Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			ri.Printf( PRINT_ALL, "none\n" );
		} else if ( primitives == 2 ) {
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "lightmap texture bits: %d\n", r_texturebitslm->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );
	ri.Printf( PRINT_ALL, "compressed lightmaps: %s\n", enablestrings[(r_ext_compressed_lightmaps->integer != 0 && glConfig.textureCompression != TC_NONE)] );
	ri.Printf( PRINT_ALL, "texture compression method: %s\n", tc_table[glConfig.textureCompression] );
	ri.Printf( PRINT_ALL, "anisotropic filtering: %s  ", enablestrings[(r_ext_texture_filter_anisotropic->integer != 0) && glConfig.maxTextureFilterAnisotropy] );
	if (r_ext_texture_filter_anisotropic->integer != 0 && glConfig.maxTextureFilterAnisotropy)
	{
		if (Q_isintegral(r_ext_texture_filter_anisotropic->value))
			ri.Printf( PRINT_ALL, "(%i of ", (int)r_ext_texture_filter_anisotropic->value);
		else
			ri.Printf( PRINT_ALL, "(%f of ", r_ext_texture_filter_anisotropic->value);

		if (Q_isintegral(glConfig.maxTextureFilterAnisotropy))
			ri.Printf( PRINT_ALL, "%i)\n", (int)glConfig.maxTextureFilterAnisotropy);
		else
			ri.Printf( PRINT_ALL, "%f)\n", glConfig.maxTextureFilterAnisotropy);
	}
	ri.Printf( PRINT_ALL, "Dynamic Glow: %s\n", enablestrings[r_DynamicGlow->integer] );
	if (g_bTextureRectangleHack) Com_Printf ("Dynamic Glow ATI BAD DRIVER HACK %s\n", enablestrings[g_bTextureRectangleHack] );

	if ( r_finish->integer ) {
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
	if ( r_displayRefresh ->integer ) {
		ri.Printf( PRINT_ALL, "Display refresh set to %d\n", r_displayRefresh->integer );
	}
	if (tr.world)
	{
		ri.Printf( PRINT_ALL, "Light Grid size set to (%.2f %.2f %.2f)\n", tr.world->lightGridSize[0], tr.world->lightGridSize[1], tr.world->lightGridSize[2] );
	}
}

void R_AtiHackToggle_f(void)
{
	g_bTextureRectangleHack = !g_bTextureRectangleHack;
}

/************************************************************************************************
 * R_FogDistance_f                                                                              *
 *    Console command to change the global fog opacity distance.  If you specify nothing on the *
 *    command line, it will display the current fog opacity distance.  Specifying a float       *
 *    representing the world units away the fog should be completely opaque will change the     *
 *    value.                                                                                    *
 *                                                                                              *
 * Input                                                                                        *
 *    none                                                                                      *
 *                                                                                              *
 * Output / Return                                                                              *
 *    none                                                                                      *
 *                                                                                              *
 ************************************************************************************************/
void R_FogDistance_f(void)
{
	float	distance;

	if (!tr.world)
	{
		ri.Printf(PRINT_ALL, "R_FogDistance_f: World is not initialized\n");
		return;
	}

	if (tr.world->globalFog == -1)
	{
		ri.Printf(PRINT_ALL, "R_FogDistance_f: World does not have a global fog\n");
		return;
	}

	if (ri.Cmd_Argc() <= 1)
	{
//		should not ever be 0.0
//		if (tr.world->fogs[tr.world->globalFog].tcScale == 0.0)
//		{
//			distance = 0.0;
//		}
//		else
		{
			distance = 1.0 / (8.0 * tr.world->fogs[tr.world->globalFog].tcScale);
		}

		ri.Printf(PRINT_ALL, "R_FogDistance_f: Current Distance: %.0f\n", distance);
		return;
	}

	if (ri.Cmd_Argc() != 2)
	{
		ri.Printf(PRINT_ALL, "R_FogDistance_f: Invalid number of arguments to set distance\n");
		return;
	}

	distance = atof(ri.Cmd_Argv(1));
	if (distance < 1.0) 
	{
		distance = 1.0;
	}
	tr.world->fogs[tr.world->globalFog].parms.depthForOpaque = distance;
	tr.world->fogs[tr.world->globalFog].tcScale = 1.0 / ( distance * 8 );
}

/************************************************************************************************
 * R_FogColor_f                                                                                 *
 *    Console command to change the global fog color.  Specifying nothing on the command will   *
 *    display the current global fog color.  Specifying a float R G B values between 0.0 and    *
 *    1.0 will change the fog color.                                                            *
 *                                                                                              *
 * Input                                                                                        *
 *    none                                                                                      *
 *                                                                                              *
 * Output / Return                                                                              *
 *    none                                                                                      *
 *                                                                                              *
 ************************************************************************************************/
void R_FogColor_f(void)
{
	if (!tr.world)
	{
		ri.Printf(PRINT_ALL, "R_FogColor_f: World is not initialized\n");
		return;
	}

	if (tr.world->globalFog == -1)
	{
		ri.Printf(PRINT_ALL, "R_FogColor_f: World does not have a global fog\n");
		return;
	}

	if (ri.Cmd_Argc() <= 1)
	{
		unsigned	i = tr.world->fogs[tr.world->globalFog].colorInt;

		ri.Printf(PRINT_ALL, "R_FogColor_f: Current Color: %0f %0f %0f\n",
			( (byte *)&i )[0] / 255.0,
			( (byte *)&i )[1] / 255.0,
			( (byte *)&i )[2] / 255.0);
		return;
	}

	if (ri.Cmd_Argc() != 4)
	{
		ri.Printf(PRINT_ALL, "R_FogColor_f: Invalid number of arguments to set color\n");
		return;
	}

	tr.world->fogs[tr.world->globalFog].parms.color[0] = atof(ri.Cmd_Argv(1));
	tr.world->fogs[tr.world->globalFog].parms.color[1] = atof(ri.Cmd_Argv(2));
	tr.world->fogs[tr.world->globalFog].parms.color[2] = atof(ri.Cmd_Argv(3));
	tr.world->fogs[tr.world->globalFog].colorInt = ColorBytes4 ( atof(ri.Cmd_Argv(1)) * tr.identityLight, 
			                          atof(ri.Cmd_Argv(2)) * tr.identityLight, 
			                          atof(ri.Cmd_Argv(3)) * tr.identityLight, 1.0 );
}

#ifdef _DEBUG
#define MIN_PRIMITIVES -1
#else
#define MIN_PRIMITIVES 0
#endif
#define MAX_PRIMITIVES 3

#ifdef _WIN32
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE
#else
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE | CVAR_LATCH
#endif

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

	r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compress_textures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_lightmaps = ri.Cvar_Get( "r_ext_compress_lightmaps", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_preferred_tc_method = ri.Cvar_Get( "r_ext_preferred_tc_method", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_gamma_control = ri.Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = ri.Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic", "16", CVAR_ARCHIVE );
 
	r_DynamicGlow = ri.Cvar_Get( "r_DynamicGlow", "0", CVAR_ARCHIVE );
	r_DynamicGlowPasses = ri.Cvar_Get( "r_DynamicGlowPasses", "5", CVAR_ARCHIVE );
	r_DynamicGlowDelta  = ri.Cvar_Get( "r_DynamicGlowDelta", "0.8f", CVAR_ARCHIVE );
	r_DynamicGlowIntensity = ri.Cvar_Get( "r_DynamicGlowIntensity", "1.13f", CVAR_ARCHIVE );
	r_DynamicGlowSoft = ri.Cvar_Get( "r_DynamicGlowSoft", "1", CVAR_ARCHIVE );
	r_DynamicGlowWidth = ri.Cvar_Get( "r_DynamicGlowWidth", "320", CVAR_ARCHIVE | CVAR_LATCH );
	r_DynamicGlowHeight = ri.Cvar_Get( "r_DynamicGlowHeight", "240", CVAR_ARCHIVE | CVAR_LATCH );

	r_picmip = ri.Cvar_Get ("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri.Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_colorMipLevels = ri.Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	r_detailTextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebitslm = ri.Cvar_Get( "r_texturebitslm", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stereo = ri.Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_overBrightBits = ri.Cvar_Get ("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_mapOverBrightBits = ri.Cvar_Get( "r_mapOverBrightBits", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri.Cvar_Get( "r_mode", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_noborder = ri.Cvar_Get( "r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH );
    r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = ri.Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_subdivisions = ri.Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange( r_subdivisions, 4, 80, qfalse );
	r_intensity = ri.Cvar_Get ("r_intensity", "1", CVAR_LATCH|CVAR_ARCHIVE );
	
	//
	// temporary latched variables that can only change over a restart
	//
	r_displayRefresh = ri.Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH );
	ri.Cvar_CheckRange( r_displayRefresh, 0, 240, qtrue );
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", CVAR_LATCH );
	r_singleShader = ri.Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri.Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE );
	r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = ri.Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "10", CVAR_ARCHIVE );

	r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_ARCHIVE );	//if set any lower, you lose a lot of precision in the distance
	ri.Cvar_CheckRange( r_znear, 0.001f, 10, qfalse ); // was qtrue in JA, is qfalse properly in ioq3
	r_ignoreGLErrors = ri.Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_drawSun = ri.Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "0", CVAR_ARCHIVE );
	r_finish = ri.Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = ri.Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_swapInterval = ri.Cvar_Get( "r_swapInterval", "0", SWAPINTERVAL_FLAGS );
	r_gamma = ri.Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_facePlaneCull = ri.Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_dlightStyle = ri.Cvar_Get ("r_dlightStyle", "1", CVAR_ARCHIVE);
	r_surfaceSprites = ri.Cvar_Get ("r_surfaceSprites", "1", CVAR_ARCHIVE);
	r_surfaceWeather = ri.Cvar_Get ("r_surfaceWeather", "0", CVAR_TEMP);

	r_windSpeed = ri.Cvar_Get ("r_windSpeed", "0", 0);
	r_windAngle = ri.Cvar_Get ("r_windAngle", "0", 0);
	r_windGust = ri.Cvar_Get ("r_windGust", "0", 0);
	r_windDampFactor = ri.Cvar_Get ("r_windDampFactor", "0.1", 0);
	r_windPointForce = ri.Cvar_Get ("r_windPointForce", "0", 0);
	r_windPointX = ri.Cvar_Get ("r_windPointX", "0", 0);
	r_windPointY = ri.Cvar_Get ("r_windPointY", "0", 0);

	r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );
	ri.Cvar_CheckRange( r_primitives, MIN_PRIMITIVES, MAX_PRIMITIVES, qtrue );

	r_ambientScale = ri.Cvar_Get( "r_ambientScale", "0.5", CVAR_CHEAT );
	r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_CHEAT );

	r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugStyle = ri.Cvar_Get( "r_debugStyle", "-1", CVAR_CHEAT );
	r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );

	r_nocurves = ri.Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
#ifdef JK2_MODE
	r_drawfog = ri.Cvar_Get ("r_drawfog", "1", CVAR_CHEAT );
#else
	r_drawfog = ri.Cvar_Get ("r_drawfog", "2", CVAR_CHEAT );
#endif
	r_lightmap = ri.Cvar_Get ("r_lightmap", "0", CVAR_CHEAT );
	r_portalOnly = ri.Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_skipBackEnd = ri.Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri.Cvar_Get( "r_ignore", "1", CVAR_TEMP );
	r_nocull = ri.Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri.Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri.Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri.Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri.Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showtriscolor = ri.Cvar_Get ("r_showtriscolor", "0", CVAR_ARCHIVE);
	r_showsky = ri.Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = ri.Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", 0 );
	r_shadowRange = ri.Cvar_Get( "r_shadowRange", "1000", CVAR_ARCHIVE );

/*
Ghoul2 Insert Start
*/
	r_noGhoul2 = ri.Cvar_Get( "r_noghoul2", "0", CVAR_CHEAT);
	r_Ghoul2AnimSmooth = ri.Cvar_Get( "r_ghoul2animsmooth", "0.25", 0);
	r_Ghoul2UnSqash = ri.Cvar_Get( "r_ghoul2unsquash", "1", 0);
	r_Ghoul2TimeBase = ri.Cvar_Get( "r_ghoul2timebase", "2", 0);
	r_Ghoul2NoLerp = ri.Cvar_Get( "r_ghoul2nolerp", "0", 0);
	r_Ghoul2NoBlend = ri.Cvar_Get( "r_ghoul2noblend", "0", 0);
	r_Ghoul2BlendMultiplier = ri.Cvar_Get( "r_ghoul2blendmultiplier", "1", 0);
	r_Ghoul2UnSqashAfterSmooth = ri.Cvar_Get( "r_ghoul2unsquashaftersmooth", "1", 0);

	broadsword = ri.Cvar_Get( "broadsword", "1", 0);
	broadsword_kickbones = ri.Cvar_Get( "broadsword_kickbones", "1", 0);
	broadsword_kickorigin = ri.Cvar_Get( "broadsword_kickorigin", "1", 0);
	broadsword_dontstopanim = ri.Cvar_Get( "broadsword_dontstopanim", "0", 0);
	broadsword_waitforshot = ri.Cvar_Get( "broadsword_waitforshot", "0", 0);
	broadsword_playflop = ri.Cvar_Get( "broadsword_playflop", "1", 0);
	broadsword_smallbbox = ri.Cvar_Get( "broadsword_smallbbox", "0", 0);
	broadsword_extra1 = ri.Cvar_Get( "broadsword_extra1", "0", 0);
	broadsword_extra2 = ri.Cvar_Get( "broadsword_extra2", "0", 0);
	broadsword_effcorr = ri.Cvar_Get( "broadsword_effcorr", "1", 0);
	broadsword_ragtobase = ri.Cvar_Get( "broadsword_ragtobase", "2", 0);
	broadsword_dircap = ri.Cvar_Get( "broadsword_dircap", "64", 0);

/*
Ghoul2 Insert End
*/

	sv_mapname = ri.Cvar_Get ( "mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM );
	sv_mapChecksum = ri.Cvar_Get ( "sv_mapChecksum", "", CVAR_ROM );
	se_language = ri.Cvar_Get ( "se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART );
#ifdef JK2_MODE
	sp_language = ri.Cvar_Get ( "sp_language", va("%d", SP_LANGUAGE_ENGLISH), CVAR_ARCHIVE | CVAR_NORESTART );
#endif
	com_buildScript = ri.Cvar_Get ( "com_buildScript", "0", 0 );

	r_modelpoolmegs = ri.Cvar_Get("r_modelpoolmegs", "20", CVAR_ARCHIVE);
	if (ri.LowPhysicalMemory() )
	{
		ri.Cvar_Set("r_modelpoolmegs", "0");
	}

	r_environmentMapping = ri.Cvar_Get( "r_environmentMapping", "1", CVAR_ARCHIVE );

	r_screenshotJpegQuality				= ri.Cvar_Get( "r_screenshotJpegQuality",			"95",						CVAR_ARCHIVE );

	ri.Cvar_CheckRange( r_screenshotJpegQuality, 10, 100, qtrue );

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "fontlist", R_FontList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "r_atihack", R_AtiHackToggle_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshot_png", R_ScreenShotPNG_f );
	ri.Cmd_AddCommand( "screenshot_tga", R_ScreenShotTGA_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "r_fogDistance", R_FogDistance_f);
	ri.Cmd_AddCommand( "r_fogColor", R_FogColor_f);
	ri.Cmd_AddCommand( "modelcacheinfo", RE_RegisterModels_Info_f);
	ri.Cmd_AddCommand( "imagecacheinfo", RE_RegisterImages_Info_f);
extern void R_WorldEffect_f(void);	//TR_WORLDEFFECTS.CPP
	ri.Cmd_AddCommand( "r_we", R_WorldEffect_f );
extern void R_ReloadFonts_f(void);
	ri.Cmd_AddCommand( "r_reloadfonts", R_ReloadFonts_f );
	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );
	// make sure all the commands added above are also
	// removed in R_Shutdown
}

// need to do this hackery so ghoul2 doesn't crash the game because of ITS hackery...
//
void R_ClearStuffToStopGhoul2CrashingThings(void)
{
	memset( &tr, 0, sizeof( tr ) );
}

/*
===============
R_Init
===============
*/
extern void R_InitWorldEffects();
void R_Init( void ) {	
	int	err;
	int i;

	//ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	ShaderEntryPtrs_Clear();

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

#ifdef _WIN32
	tr.wv = ri.GetWinVars();
#endif

//	Swap_Init();

#ifndef FINAL_BUILD
	if ( (intptr_t)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
#endif

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0 - tr.sawToothTable[i];

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

	R_InitFogTable();

	R_ImageLoader_Init();
	R_NoiseInit();
	R_Register();

	backEndData = (backEndData_t *) Hunk_Alloc( sizeof( backEndData_t ), qtrue );
	R_InitNextFrame();

	const color4ub_t	color = {0xff, 0xff, 0xff, 0xff};
	for(i=0;i<MAX_LIGHT_STYLES;i++)
	{
		RE_SetLightStyle(i, *(int*)color);
	}

	InitOpenGL();

	R_InitImages();
	R_InitShaders();
	R_InitSkins();
	R_ModelInit();
	R_InitWorldEffects();
	R_InitFonts();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	RestoreGhoul2InfoArray();

	//ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
extern void R_ShutdownWorldEffects(void);
void RE_Shutdown( qboolean destroyWindow, qboolean restarting ) {	

	// Need this temporarily.
#ifdef _WIN32
	tr.wv = ri.GetWinVars();
#endif

	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("shaderlist");
	ri.Cmd_RemoveCommand ("skinlist");
	ri.Cmd_RemoveCommand ("fontlist");
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("modelist" );
	ri.Cmd_RemoveCommand ("r_atihack");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("screenshot_png");
	ri.Cmd_RemoveCommand ("screenshot_tga");	
	ri.Cmd_RemoveCommand ("gfxinfo");
	ri.Cmd_RemoveCommand ("r_fogDistance");
	ri.Cmd_RemoveCommand ("r_fogColor");
	ri.Cmd_RemoveCommand ("modelcacheinfo");
	ri.Cmd_RemoveCommand ("imagecacheinfo");
	ri.Cmd_RemoveCommand ("r_we");
	ri.Cmd_RemoveCommand ("r_reloadfonts");
	ri.Cmd_RemoveCommand ("minimize");

	R_ShutdownWorldEffects();
	R_ShutdownFonts();

	if ( tr.registered ) {
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
//		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		if (destroyWindow)
		{
			R_DeleteTextures();	// only do this for vid_restart now, not during things like map load

			if ( restarting )
			{
				SaveGhoul2InfoArray();
			}
		}
	}

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();
	}
	tr.registered = qfalse;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void	RE_EndRegistration( void ) {
	R_SyncRenderThread();
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
		styleUpdated[style] = true;
	}
}

/*
=====================
tr_distortionX

DLL glue
=====================
*/

extern float tr_distortionAlpha;
extern float tr_distortionStretch;
extern qboolean tr_distortionPrePost;
extern qboolean tr_distortionNegate;

float *get_tr_distortionAlpha( void )
{
	return &tr_distortionAlpha;
}

float *get_tr_distortionStretch( void )
{
	return &tr_distortionStretch;
}

qboolean *get_tr_distortionPrePost( void )
{
	return &tr_distortionPrePost;
}

qboolean *get_tr_distortionNegate( void )
{
	return &tr_distortionNegate;
}

float g_oldRangedFog = 0.0f;
void RE_SetRangedFog( float dist )
{
	if (tr.rangedFog <= 0.0f)
	{
		g_oldRangedFog = tr.rangedFog;
	}
	tr.rangedFog = dist;
	if (tr.rangedFog == 0.0f && g_oldRangedFog)
	{ //restore to previous state if applicable
		tr.rangedFog = g_oldRangedFog;
	}
}

//bool inServer = false;
void RE_SVModelInit( void )
{
	tr.numModels = 0;
	tr.numShaders = 0;
	tr.numSkins = 0;
	R_InitImages();
	//inServer = true;
	R_InitShaders();
	//inServer = false;
	R_ModelInit();
}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern void R_LoadImage( const char *shortname, byte **pic, int *width, int *height );
extern void R_WorldEffectCommand(const char *command);
extern qboolean R_inPVS( vec3_t p1, vec3_t p2 );
extern void RE_GetModelBounds(refEntity_t *refEnt, vec3_t bounds1, vec3_t bounds2);
extern void G2API_AnimateG2Models(CGhoul2Info_v &ghoul2, int AcurrentTime,CRagDollUpdateParams *params);
extern qboolean G2API_GetRagBonePos(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale);
extern qboolean G2API_RagEffectorKick(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t velocity);
extern qboolean G2API_RagForceSolve(CGhoul2Info_v &ghoul2, qboolean force);
extern qboolean G2API_SetBoneIKState(CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params);
extern qboolean G2API_IKMove(CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params);
extern qboolean G2API_RagEffectorGoal(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos);
extern qboolean G2API_RagPCJGradientSpeed(CGhoul2Info_v &ghoul2, const char *boneName, const float speed);
extern qboolean G2API_RagPCJConstraint(CGhoul2Info_v &ghoul2, const char *boneName, vec3_t min, vec3_t max);
extern void G2API_SetRagDoll(CGhoul2Info_v &ghoul2,CRagDollParams *parms);
#ifdef G2_PERFORMANCE_ANALYSIS
extern void G2Time_ResetTimers(void);
extern void G2Time_ReportTimers(void);
#endif
extern IGhoul2InfoArray &TheGhoul2InfoArray();

#ifdef JK2_MODE
unsigned int AnyLanguage_ReadCharFromString_JK2 ( char **text, qboolean *pbIsTrailingPunctuation ) {
	return AnyLanguage_ReadCharFromString (text, pbIsTrailingPunctuation);
}
#endif

extern "C" Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *refimp ) {
	static refexport_t	re;

	ri = *refimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", 
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

#define REX(x)	re.x = RE_##x

	REX(Shutdown);

	REX(BeginRegistration);
	REX(RegisterModel);
	REX(RegisterSkin);
	REX(GetAnimationCFG);
	REX(RegisterShader);
	REX(RegisterShaderNoMip);
	re.LoadWorld = RE_LoadWorldMap;
	re.R_LoadImage = R_LoadImage;

	REX(RegisterMedia_LevelLoadBegin);
	REX(RegisterMedia_LevelLoadEnd);
	REX(RegisterMedia_GetLevel);
	REX(RegisterImages_LevelLoadEnd);
	REX(RegisterModels_LevelLoadEnd);

	REX(SetWorldVisData);

	REX(EndRegistration);

	REX(ClearScene);
	REX(AddRefEntityToScene);
	REX(GetLighting);
	REX(AddPolyToScene);
	REX(AddLightToScene);
	REX(RenderScene);
	REX(GetLighting);

	REX(SetColor);
	re.DrawStretchPic = RE_StretchPic;
	re.DrawRotatePic = RE_RotatePic;
	re.DrawRotatePic2 = RE_RotatePic2;
	REX(LAGoggles);
	REX(Scissor);

	re.DrawStretchRaw = RE_StretchRaw;
	REX(UploadCinematic);

	REX(BeginFrame);
	REX(EndFrame);

	REX(ProcessDissolve);
	REX(InitDissolve);

	REX(GetScreenShot);
	REX(TempRawImage_ReadFromFile);
	REX(TempRawImage_CleanUp);

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;
	REX(GetLightStyle);
	REX(SetLightStyle);
	REX(GetBModelVerts);
	re.WorldEffectCommand = R_WorldEffectCommand;
	REX(GetModelBounds);

	REX(SVModelInit);

	REX(RegisterFont);
	REX(Font_HeightPixels);
	REX(Font_StrLenPixels);
	REX(Font_DrawString);
	REX(Font_StrLenChars);
	re.Language_IsAsian = Language_IsAsian;
	re.Language_UsesSpaces = Language_UsesSpaces;
	re.AnyLanguage_ReadCharFromString = AnyLanguage_ReadCharFromString;
#ifdef JK2_MODE
	re.AnyLanguage_ReadCharFromString2 = AnyLanguage_ReadCharFromString_JK2;
#endif

	re.R_InitWorldEffects = R_InitWorldEffects;
	re.R_ClearStuffToStopGhoul2CrashingThings = R_ClearStuffToStopGhoul2CrashingThings;
	re.R_inPVS = R_inPVS;

	re.tr_distortionAlpha = get_tr_distortionAlpha;
	re.tr_distortionStretch = get_tr_distortionStretch;
	re.tr_distortionPrePost = get_tr_distortionPrePost;
	re.tr_distortionNegate = get_tr_distortionNegate;

	re.GetWindVector = R_GetWindVector;
	re.GetWindGusting = R_GetWindGusting;
	re.IsOutside = R_IsOutside;
	re.IsOutsideCausingPain = R_IsOutsideCausingPain;
	re.GetChanceOfSaberFizz = R_GetChanceOfSaberFizz;
	re.IsShaking = R_IsShaking;
	re.AddWeatherZone = R_AddWeatherZone;
	re.SetTempGlobalFogColor = R_SetTempGlobalFogColor;

	REX(SetRangedFog);

	re.TheGhoul2InfoArray = TheGhoul2InfoArray;

#define G2EX(x)	re.G2API_##x = G2API_##x

	G2EX(AddBolt);
	G2EX(AddBoltSurfNum);
	G2EX(AddSurface);
	G2EX(AnimateG2Models);
	G2EX(AttachEnt);
	G2EX(AttachG2Model);
	G2EX(CollisionDetect);
	G2EX(CleanGhoul2Models);
	G2EX(CopyGhoul2Instance);
	G2EX(DetachEnt);
	G2EX(DetachG2Model);
	G2EX(GetAnimFileName);
	G2EX(GetAnimFileNameIndex);
	G2EX(GetAnimFileInternalNameIndex);
	G2EX(GetAnimIndex);
	G2EX(GetAnimRange);
	G2EX(GetAnimRangeIndex);
	G2EX(GetBoneAnim);
	G2EX(GetBoneAnimIndex);
	G2EX(GetBoneIndex);
	G2EX(GetBoltMatrix);
	G2EX(GetGhoul2ModelFlags);
	G2EX(GetGLAName);
	G2EX(GetParentSurface);
	G2EX(GetRagBonePos);
	G2EX(GetSurfaceIndex);
	G2EX(GetSurfaceName);
	G2EX(GetSurfaceRenderStatus);
	G2EX(GetTime);
	G2EX(GiveMeVectorFromMatrix);
	G2EX(HaveWeGhoul2Models);
	G2EX(IKMove);
	G2EX(InitGhoul2Model);
	G2EX(IsPaused);
	G2EX(ListBones);
	G2EX(ListSurfaces);
	G2EX(LoadGhoul2Models);
	G2EX(LoadSaveCodeDestructGhoul2Info);
	G2EX(PauseBoneAnim);
	G2EX(PauseBoneAnimIndex);
	G2EX(PrecacheGhoul2Model);
	G2EX(RagEffectorGoal);
	G2EX(RagEffectorKick);
	G2EX(RagForceSolve);
	G2EX(RagPCJConstraint);
	G2EX(RagPCJGradientSpeed);
	G2EX(RemoveBolt);
	G2EX(RemoveBone);
	G2EX(RemoveGhoul2Model);
	G2EX(RemoveSurface);
	G2EX(SaveGhoul2Models);
	G2EX(SetAnimIndex);
	G2EX(SetBoneAnim);
	G2EX(SetBoneAnimIndex);
	G2EX(SetBoneAngles);
	G2EX(SetBoneAnglesIndex);
	G2EX(SetBoneAnglesMatrix);
	G2EX(SetBoneIKState);
	G2EX(SetGhoul2ModelFlags);
	G2EX(SetGhoul2ModelIndexes);
	G2EX(SetLodBias);
	//G2EX(SetModelIndexes);
	G2EX(SetNewOrigin);
	G2EX(SetRagDoll);
	G2EX(SetRootSurface);
	G2EX(SetShader);
	G2EX(SetSkin);
	G2EX(SetSurfaceOnOff);
	G2EX(SetTime);
	G2EX(StopBoneAnim);
	G2EX(StopBoneAnimIndex);
	G2EX(StopBoneAngles);
	G2EX(StopBoneAnglesIndex);
#ifdef _G2_GORE
	G2EX(AddSkinGore);
	G2EX(ClearSkinGore);
#endif

#ifdef G2_PERFORMANCE_ANALYSIS
	re.G2Time_ReportTimers = G2Time_ReportTimers;
	re.G2Time_ResetTimers = G2Time_ResetTimers;
#endif

	//Swap_Init();

	return &re;
}
