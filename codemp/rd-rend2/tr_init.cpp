/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "G2_local.h"

bool g_bDynamicGlowSupported = false;		// Not used. Put here to keep *_glimp from whining at us. --eez

glconfig_t  glConfig;
glRefConfig_t glRefConfig;
qboolean    textureFilterAnisotropic = qfalse;
int         maxAnisotropy = 0;
float       displayAspect = 0.0f;

glstate_t	glState;

static void GfxInfo_f( void );
static void GfxMemInfo_f( void );

cvar_t  *com_altivec;

cvar_t	*se_language;

cvar_t	*r_flareSize;
cvar_t	*r_flareFade;
cvar_t	*r_flareCoeff;

cvar_t	*r_displayRefresh;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;
cvar_t	*r_zproj;
cvar_t	*r_stereoSeparation;

cvar_t	*r_skipBackEnd;

cvar_t	*r_stereo;
cvar_t	*r_anaglyphMode;

cvar_t	*r_greyscale;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;
cvar_t	*r_autolodscalevalue;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_max_anisotropy;
cvar_t	*r_ext_preferred_tc_method;

cvar_t  *r_ext_draw_range_elements;
cvar_t  *r_ext_multi_draw_arrays;
cvar_t  *r_ext_framebuffer_object;
cvar_t  *r_ext_texture_float;
cvar_t  *r_arb_half_float_pixel;
cvar_t  *r_ext_framebuffer_multisample;
cvar_t  *r_arb_seamless_cube_map;
cvar_t  *r_arb_vertex_type_2_10_10_10_rev;

cvar_t  *r_mergeMultidraws;
cvar_t  *r_mergeLeafSurfaces;

cvar_t  *r_cameraExposure;

cvar_t  *r_softOverbright;

cvar_t  *r_hdr;
cvar_t  *r_floatLightmap;
cvar_t  *r_postProcess;

cvar_t  *r_toneMap;
cvar_t  *r_forceToneMap;
cvar_t  *r_forceToneMapMin;
cvar_t  *r_forceToneMapAvg;
cvar_t  *r_forceToneMapMax;

cvar_t  *r_autoExposure;
cvar_t  *r_forceAutoExposure;
cvar_t  *r_forceAutoExposureMin;
cvar_t  *r_forceAutoExposureMax;

cvar_t  *r_srgb;

cvar_t  *r_depthPrepass;
cvar_t  *r_ssao;

cvar_t  *r_normalMapping;
cvar_t  *r_specularMapping;
cvar_t  *r_deluxeMapping;
cvar_t  *r_parallaxMapping;
cvar_t  *r_cubeMapping;
cvar_t  *r_deluxeSpecular;
cvar_t  *r_specularIsMetallic;
cvar_t  *r_baseSpecular;
cvar_t  *r_baseGloss;
cvar_t  *r_recalcMD3Normals;
cvar_t  *r_mergeLightmaps;
cvar_t  *r_dlightMode;
cvar_t  *r_pshadowDist;
cvar_t  *r_imageUpsample;
cvar_t  *r_imageUpsampleMaxSize;
cvar_t  *r_imageUpsampleType;
cvar_t  *r_genNormalMaps;
cvar_t  *r_forceSun;
cvar_t  *r_forceSunMapLightScale;
cvar_t  *r_forceSunLightScale;
cvar_t  *r_forceSunAmbientScale;
cvar_t  *r_sunlightMode;
cvar_t  *r_drawSunRays;
cvar_t  *r_sunShadows;
cvar_t  *r_shadowFilter;
cvar_t  *r_shadowMapSize;
cvar_t  *r_shadowCascadeZNear;
cvar_t  *r_shadowCascadeZFar;
cvar_t  *r_shadowCascadeZBias;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t	*r_drawBuffer;
cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_roundImagesDown;
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

cvar_t	*r_fullscreen;
cvar_t  *r_noborder;
cvar_t	*r_centerWindow;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;
cvar_t	*r_customPixelAspect;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;

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

cvar_t	*r_marksOnTriangleMeshes;

cvar_t	*r_aviMotionJpegQuality;
cvar_t	*r_screenshotJpegQuality;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;


extern void	RB_SetGL2D (void);
void R_Splash()
{
	image_t *pImage = R_FindImageFile( "menu/splash", IMGTYPE_COLORALPHA, IMGFLAG_NONE);
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 640, 480, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );
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
	char renderer_buffer[1024];

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
		GLint		temp;
		
		GLimp_Init();
		GLimp_InitExtraExtensions();

		strcpy( renderer_buffer, glConfig.renderer_string );
		Q_strlwr( renderer_buffer );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 ) 
		{
			glConfig.maxTextureSize = 0;
		}

		// set default state
		GL_SetDefaultState();
		R_Splash();	//get something on screen asap
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
void GL_CheckErrs( char *file, int line ) {
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

	ri->Error( ERR_FATAL, "GL_CheckErrors: %s in %s at line %d", s , file, line);
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480",		640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480 (wide)",856,	480,	1 }
};
static int	s_numVidModes = ARRAY_LEN( r_vidModes );

qboolean R_GetModeInfo( int *width, int *height, int mode ) {
	vidmode_t	*vm;
	float			pixelAspect;

	if ( mode < -1 ) {
		return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	} else {
		vm = &r_vidModes[mode];

		*width  = vm->width;
		*height = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	ri->Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri->Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri->Printf( PRINT_ALL, "\n" );
}


/* 
============================================================================== 
 
						SCREEN SHOTS 

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

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

Return value must be freed with ri->Hunk_FreeTempMemory()
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
	buffer = (byte *)ri->Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);
	
	bufstart = (byte*)(PADP((intptr_t) buffer + *offset, packAlign));
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);
	
	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;
	
	return buffer;
}

/* 
================== 
RB_TakeScreenshot
================== 
*/  
void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
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

	ri->FS_WriteFile(fileName, buffer, memcount + 18);

	ri->Hunk_FreeTempMemory(allbuf);
}

/*
================== 
R_TakeScreenshotPNG
================== 
*/
void RB_TakeScreenshotPNG( int x, int y, int width, int height, char *fileName ) {
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	buffer = RB_ReadPixels( x, y, width, height, &offset, &padlen );
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SavePNG( fileName, buffer, width, height, 3 );
	ri->Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri->Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
	const screenshotCommand_t	*cmd;
	
	cmd = (const screenshotCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	switch( cmd->format ) {
		case SSF_JPEG:
			RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
		case SSF_TGA:
			RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
		case SSF_PNG:
			RB_TakeScreenshotPNG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName );
			break;
	}

	return (const void *)(cmd + 1);	
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, screenshotFormat_t format ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = (screenshotCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->format = format;
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

	buffer = (byte *)ri->Hunk_AllocateTempMemory(LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18);
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

	ri->FS_WriteFile( checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18 );

	ri->Hunk_FreeTempMemory( buffer );
	ri->Hunk_FreeTempMemory( allsource );

	ri->Printf( PRINT_ALL, "Wrote %s\n", checkname );
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

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.tga", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".tga" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SSF_TGA );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

void R_ScreenShotPNG_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.png", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".png" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SSF_PNG );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

void R_ScreenShotJPEG_f (void) {
	char checkname[MAX_OSPATH] = {0};
	qboolean silent = qfalse;

	if ( !strcmp( ri->Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri->Cmd_Argv(1), "silent" ) )
		silent = qtrue;

	if ( ri->Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, sizeof( checkname ), "screenshots/%s.jpg", ri->Cmd_Argv( 1 ) );
	}
	else {
		// timestamp the file
		R_ScreenshotFilename( checkname, sizeof( checkname ), ".jpg" );

		if ( ri->FS_FileExists( checkname ) ) {
			Com_Printf( "ScreenShot: Couldn't create a file\n"); 
			return;
 		}
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, SSF_JPEG );

	if ( !silent )
		ri->Printf (PRINT_ALL, "Wrote %s\n", checkname);
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t	*cmd;
	byte				*cBuf;
	size_t				memcount, linelen;
	int				padwidth, avipadwidth, padlen, avipadlen;
	GLint packAlign;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	cmd = (const videoFrameCommand_t *)data;
	
	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	avipadlen = avipadwidth - linelen;

	cBuf = (byte*)(PADP(cmd->captureBuffer, packAlign));
		
	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
		GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg)
	{
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
			r_aviMotionJpegQuality->integer,
			cmd->width, cmd->height, cBuf, padlen);
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else
	{
		byte *lineend, *memend;
		byte *srcptr, *destptr;
	
		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;
		
		// swap R and B and remove line paddings
		while(srcptr < memend)
		{
			lineend = srcptr + linelen;
			while(srcptr < lineend)
			{
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}
			
			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;
			
			srcptr += padlen;
		}
		
		ri->CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);	
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

	//qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	glState.vertexAttribsState = 0;
	glState.vertexAttribPointersSet = 0;
	glState.currentProgram = 0;
	qglUseProgramObjectARB(0);

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glState.currentVBO = NULL;
	glState.currentIBO = NULL;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

	if (glRefConfig.seamlessCubeMap)
		qglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

/*
================
R_PrintLongString

Workaround for ri->Printf's 1024 characters buffer limit.
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
		ri->Printf( PRINT_ALL, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
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

	ri->Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri->Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri->Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri->Printf( PRINT_ALL, "GL_EXTENSIONS: " );
	R_PrintLongString( glConfig.extensions_string );
	ri->Printf( PRINT_ALL, "\n" );
	ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri->Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
	ri->Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri->Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		ri->Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else
	{
		ri->Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		ri->Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri->Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	ri->Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri->Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri->Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri->Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri->Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri->Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri->Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	if ( r_vertexLight->integer )
	{
		ri->Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( r_displayRefresh ->integer ) {
		ri->Printf( PRINT_ALL, "Display refresh set to %d\n", r_displayRefresh->integer );
	}
	if ( r_finish->integer ) {
		ri->Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

/*
================
GfxMemInfo_f
================
*/
void GfxMemInfo_f( void ) 
{
	switch (glRefConfig.memInfo)
	{
		case MI_NONE:
		{
			ri->Printf(PRINT_ALL, "No extension found for GPU memory info.\n");
		}
		break;
		case MI_NVX:
		{
			int value;

			qglGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX: %ikb\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTION_COUNT_NVX: %i\n", value);

			qglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &value);
			ri->Printf(PRINT_ALL, "GPU_MEMORY_INFO_EVICTED_MEMORY_NVX: %ikb\n", value);
		}
		break;
		case MI_ATI:
		{
			// GL_ATI_meminfo
			int value[4];

			qglGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "VBO_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "TEXTURE_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);

			qglGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, &value[0]);
			ri->Printf(PRINT_ALL, "RENDERBUFFER_FREE_MEMORY_ATI: %ikb total %ikb largest aux: %ikb total %ikb largest\n", value[0], value[1], value[2], value[3]);
		}
		break;
	}
}

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
	com_altivec = ri->Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);

	//
	// latched and archived variables
	//
	r_allowExtensions = ri->Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = ri->Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = ri->Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri->Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri->Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_preferred_tc_method = ri->Cvar_Get( "r_ext_preferred_tc_method", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_ext_draw_range_elements = ri->Cvar_Get( "r_ext_draw_range_elements", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_multi_draw_arrays = ri->Cvar_Get( "r_ext_multi_draw_arrays", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_framebuffer_object = ri->Cvar_Get( "r_ext_framebuffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_float = ri->Cvar_Get( "r_ext_texture_float", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_half_float_pixel = ri->Cvar_Get( "r_arb_half_float_pixel", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_framebuffer_multisample = ri->Cvar_Get( "r_ext_framebuffer_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_seamless_cube_map = ri->Cvar_Get( "r_arb_seamless_cube_map", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_arb_vertex_type_2_10_10_10_rev = ri->Cvar_Get( "r_arb_vertex_type_2_10_10_10_rev", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri->Cvar_Get( "r_ext_texture_filter_anisotropic",
			"0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_max_anisotropy = ri->Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );

	r_picmip = ri->Cvar_Get ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_roundImagesDown = ri->Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri->Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	ri->Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_detailTextures = ri->Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri->Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri->Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_stencilbits = ri->Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri->Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multisample = ri->Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri->Cvar_CheckRange( r_ext_multisample, 0, 4, qtrue );
	r_overBrightBits = ri->Cvar_Get ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignorehwgamma = ri->Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri->Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH );
	r_fullscreen = ri->Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	r_noborder = ri->Cvar_Get("r_noborder", "0", CVAR_ARCHIVE);
	r_centerWindow = ri->Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_customwidth = ri->Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri->Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri->Cvar_Get( "r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri->Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = ri->Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri->Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = ri->Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	ri->Cvar_CheckRange( r_subdivisions, 4, 80, qfalse );
	r_stereo = ri->Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_greyscale = ri->Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri->Cvar_CheckRange(r_greyscale, 0, 1, qfalse);

	r_softOverbright = ri->Cvar_Get( "r_softOverbright", "1", CVAR_ARCHIVE | CVAR_LATCH );

	r_hdr = ri->Cvar_Get( "r_hdr", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_floatLightmap = ri->Cvar_Get( "r_floatLightmap", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_postProcess = ri->Cvar_Get( "r_postProcess", "1", CVAR_ARCHIVE );

	r_toneMap = ri->Cvar_Get( "r_toneMap", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_forceToneMap = ri->Cvar_Get( "r_forceToneMap", "0", CVAR_CHEAT );
	r_forceToneMapMin = ri->Cvar_Get( "r_forceToneMapMin", "-8.0", CVAR_CHEAT );
	r_forceToneMapAvg = ri->Cvar_Get( "r_forceToneMapAvg", "-2.0", CVAR_CHEAT );
	r_forceToneMapMax = ri->Cvar_Get( "r_forceToneMapMax", "0.0", CVAR_CHEAT );

	r_autoExposure = ri->Cvar_Get( "r_autoExposure", "1", CVAR_ARCHIVE );
	r_forceAutoExposure = ri->Cvar_Get( "r_forceAutoExposure", "0", CVAR_CHEAT );
	r_forceAutoExposureMin = ri->Cvar_Get( "r_forceAutoExposureMin", "-2.0", CVAR_CHEAT );
	r_forceAutoExposureMax = ri->Cvar_Get( "r_forceAutoExposureMax", "2.0", CVAR_CHEAT );

	r_cameraExposure = ri->Cvar_Get( "r_cameraExposure", "0", CVAR_CHEAT );

	r_srgb = ri->Cvar_Get( "r_srgb", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_depthPrepass = ri->Cvar_Get( "r_depthPrepass", "1", CVAR_ARCHIVE );
	r_ssao = ri->Cvar_Get( "r_ssao", "0", CVAR_LATCH | CVAR_ARCHIVE );

	r_normalMapping = ri->Cvar_Get( "r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_specularMapping = ri->Cvar_Get( "r_specularMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_deluxeMapping = ri->Cvar_Get( "r_deluxeMapping", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_parallaxMapping = ri->Cvar_Get( "r_parallaxMapping", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_cubeMapping = ri->Cvar_Get( "r_cubeMapping", "0", CVAR_ARCHIVE | CVAR_LATCH ); 
   	r_deluxeSpecular = ri->Cvar_Get( "r_deluxeSpecular", "0.3", CVAR_ARCHIVE | CVAR_LATCH ); 
   	r_specularIsMetallic = ri->Cvar_Get( "r_specularIsMetallic", "0", CVAR_ARCHIVE | CVAR_LATCH ); 
   	r_baseSpecular = ri->Cvar_Get( "r_baseSpecular", "0.04", CVAR_ARCHIVE | CVAR_LATCH ); 
   	r_baseGloss = ri->Cvar_Get( "r_baseGloss", "0.3", CVAR_ARCHIVE | CVAR_LATCH );
	r_dlightMode = ri->Cvar_Get( "r_dlightMode", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_pshadowDist = ri->Cvar_Get( "r_pshadowDist", "128", CVAR_ARCHIVE );
	r_recalcMD3Normals = ri->Cvar_Get( "r_recalcMD3Normals", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_mergeLightmaps = ri->Cvar_Get( "r_mergeLightmaps", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsample = ri->Cvar_Get( "r_imageUpsample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsampleMaxSize = ri->Cvar_Get( "r_imageUpsampleMaxSize", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_imageUpsampleType = ri->Cvar_Get( "r_imageUpsampleType", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_genNormalMaps = ri->Cvar_Get( "r_genNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH );

	r_forceSun = ri->Cvar_Get( "r_forceSun", "0", CVAR_CHEAT );
	r_forceSunMapLightScale = ri->Cvar_Get( "r_forceSunMapLightScale", "1.0", CVAR_CHEAT );
	r_forceSunLightScale = ri->Cvar_Get( "r_forceSunLightScale", "1.0", CVAR_CHEAT );
	r_forceSunAmbientScale = ri->Cvar_Get( "r_forceSunAmbientScale", "0.5", CVAR_CHEAT );
	r_drawSunRays = ri->Cvar_Get( "r_drawSunRays", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_sunlightMode = ri->Cvar_Get( "r_sunlightMode", "1", CVAR_ARCHIVE | CVAR_LATCH );

	r_sunShadows = ri->Cvar_Get( "r_sunShadows", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowFilter = ri->Cvar_Get( "r_shadowFilter", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowMapSize = ri->Cvar_Get( "r_shadowMapSize", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowCascadeZNear = ri->Cvar_Get( "r_shadowCascadeZNear", "4", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowCascadeZFar = ri->Cvar_Get( "r_shadowCascadeZFar", "3072", CVAR_ARCHIVE | CVAR_LATCH );
	r_shadowCascadeZBias = ri->Cvar_Get( "r_shadowCascadeZBias", "-320", CVAR_ARCHIVE | CVAR_LATCH );

	//
	// temporary latched variables that can only change over a restart
	//
	r_fullbright = ri->Cvar_Get ("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT );
	r_mapOverBrightBits = ri->Cvar_Get ("r_mapOverBrightBits", "2", CVAR_LATCH );
	r_intensity = ri->Cvar_Get ("r_intensity", "1", CVAR_LATCH );
	r_singleShader = ri->Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );
	r_displayRefresh = ri->Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH );
	ri->Cvar_CheckRange( r_displayRefresh, 0, 240, qtrue );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri->Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
	r_lodbias = ri->Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = ri->Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = ri->Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	r_autolodscalevalue	= ri->Cvar_Get( "r_autolodscalevalue", "0", CVAR_ROM );
	ri->Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_zproj = ri->Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
	r_stereoSeparation = ri->Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
	r_ignoreGLErrors = ri->Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri->Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = ri->Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = ri->Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
	r_dynamiclight = ri->Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = ri->Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
	r_finish = ri->Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = ri->Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_swapInterval = ri->Cvar_Get( "r_swapInterval", "0",
					SWAPINTERVAL_FLAGS );
	r_markcount = ri->Cvar_Get( "r_markcount", "100", CVAR_ARCHIVE );
	r_gamma = ri->Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_facePlaneCull = ri->Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_ambientScale = ri->Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = ri->Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	r_anaglyphMode = ri->Cvar_Get("r_anaglyphMode", "0", CVAR_ARCHIVE);
	r_mergeMultidraws = ri->Cvar_Get("r_mergeMultidraws", "1", CVAR_ARCHIVE);
	r_mergeLeafSurfaces = ri->Cvar_Get("r_mergeLeafSurfaces", "1", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri->Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = ri->Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri->Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri->Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri->Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = ri->Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri->Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = ri->Cvar_Get ("r_lightmap", "0", 0 );
	r_portalOnly = ri->Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = ri->Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri->Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);
	r_flareCoeff = ri->Cvar_Get ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

	r_skipBackEnd = ri->Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri->Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = ri->Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = ri->Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri->Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri->Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = ri->Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri->Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri->Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri->Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = ri->Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri->Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri->Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri->Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri->Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = ri->Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = ri->Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = ri->Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri->Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri->Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = ri->Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = ri->Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri->Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = ri->Cvar_Get( "cg_shadows", "1", 0 );

	r_marksOnTriangleMeshes = ri->Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

	r_aviMotionJpegQuality = ri->Cvar_Get("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri->Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_maxpolys = ri->Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri->Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
	r_noPrecacheGLA						= ri->Cvar_Get( "r_noPrecacheGLA",					"0",						CVAR_CHEAT );
#endif
	r_noServerGhoul2					= ri->Cvar_Get( "r_noserverghoul2",					"0",						CVAR_CHEAT );
	r_Ghoul2AnimSmooth					= ri->Cvar_Get( "r_ghoul2animsmooth",				"0.3",						CVAR_NONE );
	r_Ghoul2UnSqashAfterSmooth			= ri->Cvar_Get( "r_ghoul2unsqashaftersmooth",		"1",						CVAR_NONE );
	broadsword							= ri->Cvar_Get( "broadsword",						"0",						CVAR_NONE );
	broadsword_kickbones				= ri->Cvar_Get( "broadsword_kickbones",				"1",						CVAR_NONE );
	broadsword_kickorigin				= ri->Cvar_Get( "broadsword_kickorigin",			"1",						CVAR_NONE );
	broadsword_dontstopanim				= ri->Cvar_Get( "broadsword_dontstopanim",			"0",						CVAR_NONE );
	broadsword_waitforshot				= ri->Cvar_Get( "broadsword_waitforshot",			"0",						CVAR_NONE );
	broadsword_playflop					= ri->Cvar_Get( "broadsword_playflop",				"1",						CVAR_NONE );
	broadsword_smallbbox				= ri->Cvar_Get( "broadsword_smallbbox",				"0",						CVAR_NONE );
	broadsword_extra1					= ri->Cvar_Get( "broadsword_extra1",				"0",						CVAR_NONE );
	broadsword_extra2					= ri->Cvar_Get( "broadsword_extra2",				"0",						CVAR_NONE );
	broadsword_effcorr					= ri->Cvar_Get( "broadsword_effcorr",				"1",						CVAR_NONE );
	broadsword_ragtobase				= ri->Cvar_Get( "broadsword_ragtobase",				"2",						CVAR_NONE );
	broadsword_dircap					= ri->Cvar_Get( "broadsword_dircap",				"64",						CVAR_NONE );
/*
Ghoul2 Insert End
*/

	se_language = ri->Cvar_Get ( "se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART );

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri->Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri->Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri->Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri->Cmd_AddCommand( "fontlist", R_FontList_f );
	ri->Cmd_AddCommand( "modellist", R_Modellist_f );
	ri->Cmd_AddCommand( "modelist", R_ModeList_f );
	ri->Cmd_AddCommand( "screenshot", R_ScreenShotJPEG_f );
	ri->Cmd_AddCommand( "screenshot_png", R_ScreenShotPNG_f );
	ri->Cmd_AddCommand( "screenshot_tga", R_ScreenShotTGA_f );
	ri->Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri->Cmd_AddCommand( "minimize", GLimp_Minimize );
	ri->Cmd_AddCommand( "gfxmeminfo", GfxMemInfo_f );
}

void R_InitQueries(void)
{
	if (!glRefConfig.occlusionQuery)
		return;

	if (r_drawSunRays->integer)
		qglGenQueriesARB(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
}

void R_ShutDownQueries(void)
{
	if (!glRefConfig.occlusionQuery)
		return;

	if (r_drawSunRays->integer)
		qglDeleteQueriesARB(ARRAY_LEN(tr.sunFlareQuery), tr.sunFlareQuery);
}

void RE_SetLightStyle (int style, int color);

/*
===============
R_Init
===============
*/
void R_Init( void ) {	
	int	err;
	int i;
	byte *ptr;

	ri->Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
	Com_Memset( &tess, 0, sizeof( tess ) );
	
#ifdef _WIN32
	tr.wv = (WinVars_t *)ri->GetWinVars();
#endif

//	Swap_Init();

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

	R_InitFogTable();

	R_NoiseInit();
	R_ImageLoader_Init();
	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = (byte*)ri->Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);
	R_InitNextFrame();

	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
	{
		RE_SetLightStyle (i, -1);
	}

	InitOpenGL();

	R_InitImages();

	if (glRefConfig.framebufferObject)
		FBO_Init();

	GLSL_InitGPUShaders();

	R_InitVBOs();

	R_InitShaders (qfalse);

	R_InitSkins();

	R_InitFonts();

	R_ModelInit();

	R_InitDecals();

	R_InitQueries();


	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri->Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	// print info
	GfxInfo_f();
	ri->Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

	ri->Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri->Cmd_RemoveCommand ("modellist");
	ri->Cmd_RemoveCommand ("screenshot");
	ri->Cmd_RemoveCommand ("screenshot_png");
	ri->Cmd_RemoveCommand ("screenshot_tga");
	ri->Cmd_RemoveCommand ("imagelist");
	ri->Cmd_RemoveCommand ("shaderlist");
	ri->Cmd_RemoveCommand ("skinlist");
	ri->Cmd_RemoveCommand ("fontlist");
	ri->Cmd_RemoveCommand ("gfxinfo");
	ri->Cmd_RemoveCommand("minimize");
	ri->Cmd_RemoveCommand( "modelist" );
	ri->Cmd_RemoveCommand( "shaderstate" );
	ri->Cmd_RemoveCommand( "gfxmeminfo" );


	if ( tr.registered ) {
		R_IssuePendingRenderCommands();
		R_ShutDownQueries();
		if (glRefConfig.framebufferObject)
			FBO_Shutdown();
		R_DeleteTextures();
		R_ShutdownVBOs();
		GLSL_ShutdownGPUShaders();
	}

	R_ShutdownFonts();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		Com_Memset( &glConfig, 0, sizeof( glConfig ) );
		Com_Memset( &glState, 0, sizeof( glState ) );
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_IssuePendingRenderCommands();
	if (!ri->Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

// HACK
extern qboolean gG2_GBMNoReconstruct;
extern qboolean gG2_GBMUseSPMethod;
static void G2API_BoltMatrixReconstruction( qboolean reconstruct ) { gG2_GBMNoReconstruct = (qboolean)!reconstruct; }
static void G2API_BoltMatrixSPMethod( qboolean spMethod ) { gG2_GBMUseSPMethod = spMethod; }

static float GetDistanceCull( void ) { return 6000/*tr.distanceCull*/; }

extern void R_SVModelInit( void ); //tr_model.cpp

static void GetRealRes( int *w, int *h ) {
	*w = glConfig.vidWidth;
	*h = glConfig.vidHeight;
}

extern IGhoul2InfoArray &TheGhoul2InfoArray();
const CGhoul2Info NullG2;

// STUBS, REPLACEME
qboolean stub_InitializeWireframeAutomap() { return qtrue; }

void RE_GetLightStyle(int style, color4ub_t color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_GetLightStyle: %d is out of range", style );
		return;
	}

	*(int *)color = *(int *)styleColors[style];
}

void RE_SetLightStyle(int style, int color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_SetLightStyle: %d is out of range", style );
		return;
	}

	if (*(int*)styleColors[style] != color)
	{
		*(int *)styleColors[style] = color;
	}
}

void stub_RE_GetBModelVerts (int bModel, vec3_t *vec, float *normal) {}
void stub_RE_WorldEffectCommand ( const char *cmd ){}
void stub_RE_AddWeatherZone ( vec3_t mins, vec3_t maxs ) {}
static void RE_SetRefractionProperties ( float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate ) { }

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" {
Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t	re;

	assert( rimp );
	ri = rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri->Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", 
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterServerModel = RE_RegisterServerModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterServerSkin = RE_RegisterServerSkin;
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
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
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
	re.GetBModelVerts = stub_RE_GetBModelVerts;

	re.SetRangedFog = RE_SetRangedFog;
	re.SetRefractionProperties = RE_SetRefractionProperties;
	re.GetDistanceCull = GetDistanceCull;
	re.GetRealRes = GetRealRes;
	// R_AutomapElevationAdjustment
	re.InitializeWireframeAutomap = stub_InitializeWireframeAutomap;
	re.AddWeatherZone = stub_RE_AddWeatherZone;
	re.WorldEffectCommand = stub_RE_WorldEffectCommand;
	// RE_InitRendererTerrain
	re.RegisterMedia_LevelLoadBegin = C_LevelLoadBegin;
	re.RegisterMedia_LevelLoadEnd = C_LevelLoadEnd;
	re.RegisterMedia_GetLevel = C_GetLevel;
	re.RegisterImages_LevelLoadEnd = C_Images_LevelLoadEnd;
	re.RegisterModels_LevelLoadEnd = C_Models_LevelLoadEnd;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	re.InitSkins							= R_InitSkins;
	re.InitShaders							= R_InitShaders;
	re.SVModelInit							= R_SVModelInit;
	re.HunkClearCrap						= RE_HunkClearCrap;

	re.G2API_AddBolt						= G2API_AddBolt;
	re.G2API_AddBoltSurfNum					= G2API_AddBoltSurfNum;
	re.G2API_AddSurface						= G2API_AddSurface;
	re.G2API_AnimateG2ModelsRag				= G2API_AnimateG2ModelsRag;
	re.G2API_AttachEnt						= G2API_AttachEnt;
	re.G2API_AttachG2Model					= G2API_AttachG2Model;
	re.G2API_AttachInstanceToEntNum			= G2API_AttachInstanceToEntNum;
	re.G2API_AbsurdSmoothing				= G2API_AbsurdSmoothing;
	re.G2API_BoltMatrixReconstruction		= G2API_BoltMatrixReconstruction;
	re.G2API_BoltMatrixSPMethod				= G2API_BoltMatrixSPMethod;
	re.G2API_CleanEntAttachments			= G2API_CleanEntAttachments;
	re.G2API_CleanGhoul2Models				= G2API_CleanGhoul2Models;
	re.G2API_ClearAttachedInstance			= G2API_ClearAttachedInstance;
	re.G2API_CollisionDetect				= G2API_CollisionDetect;
	re.G2API_CollisionDetectCache			= G2API_CollisionDetectCache;
	re.G2API_CopyGhoul2Instance				= G2API_CopyGhoul2Instance;
	re.G2API_CopySpecificG2Model			= G2API_CopySpecificG2Model;
	re.G2API_DetachG2Model					= G2API_DetachG2Model;
	re.G2API_DoesBoneExist					= G2API_DoesBoneExist;
	re.G2API_DuplicateGhoul2Instance		= G2API_DuplicateGhoul2Instance;
	re.G2API_FreeSaveBuffer					= G2API_FreeSaveBuffer;
	re.G2API_GetAnimFileName				= G2API_GetAnimFileName;
	re.G2API_GetAnimFileNameIndex			= G2API_GetAnimFileNameIndex;
	re.G2API_GetAnimRange					= G2API_GetAnimRange;
	re.G2API_GetBoltMatrix					= G2API_GetBoltMatrix;
	re.G2API_GetBoneAnim					= G2API_GetBoneAnim;
	re.G2API_GetBoneIndex					= G2API_GetBoneIndex;
	re.G2API_GetGhoul2ModelFlags			= G2API_GetGhoul2ModelFlags;
	re.G2API_GetGLAName						= G2API_GetGLAName;
	re.G2API_GetParentSurface				= G2API_GetParentSurface;
	re.G2API_GetRagBonePos					= G2API_GetRagBonePos;
	re.G2API_GetSurfaceIndex				= G2API_GetSurfaceIndex;
	re.G2API_GetSurfaceName					= G2API_GetSurfaceName;
	re.G2API_GetSurfaceOnOff				= G2API_GetSurfaceOnOff;
	re.G2API_GetSurfaceRenderStatus			= G2API_GetSurfaceRenderStatus;
	re.G2API_GetTime						= G2API_GetTime;
	re.G2API_Ghoul2Size						= G2API_Ghoul2Size;
	re.G2API_GiveMeVectorFromMatrix			= G2API_GiveMeVectorFromMatrix;
	re.G2API_HasGhoul2ModelOnIndex			= G2API_HasGhoul2ModelOnIndex;
	re.G2API_HaveWeGhoul2Models				= G2API_HaveWeGhoul2Models;
	re.G2API_IKMove							= G2API_IKMove;
	re.G2API_InitGhoul2Model				= G2API_InitGhoul2Model;
	re.G2API_IsPaused						= G2API_IsPaused;
	re.G2API_ListBones						= G2API_ListBones;
	re.G2API_ListSurfaces					= G2API_ListSurfaces;
	re.G2API_LoadGhoul2Models				= G2API_LoadGhoul2Models;
	re.G2API_LoadSaveCodeDestructGhoul2Info	= G2API_LoadSaveCodeDestructGhoul2Info;
	re.G2API_OverrideServerWithClientData	= G2API_OverrideServerWithClientData;
	re.G2API_PauseBoneAnim					= G2API_PauseBoneAnim;
	re.G2API_PrecacheGhoul2Model			= G2API_PrecacheGhoul2Model;
	re.G2API_RagEffectorGoal				= G2API_RagEffectorGoal;
	re.G2API_RagEffectorKick				= G2API_RagEffectorKick;
	re.G2API_RagForceSolve					= G2API_RagForceSolve;
	re.G2API_RagPCJConstraint				= G2API_RagPCJConstraint;
	re.G2API_RagPCJGradientSpeed			= G2API_RagPCJGradientSpeed;
	re.G2API_RemoveBolt						= G2API_RemoveBolt;
	re.G2API_RemoveBone						= G2API_RemoveBone;
	re.G2API_RemoveGhoul2Model				= G2API_RemoveGhoul2Model;
	re.G2API_RemoveGhoul2Models				= G2API_RemoveGhoul2Models;
	re.G2API_RemoveSurface					= G2API_RemoveSurface;
	re.G2API_ResetRagDoll					= G2API_ResetRagDoll;
	re.G2API_SaveGhoul2Models				= G2API_SaveGhoul2Models;
	re.G2API_SetBoltInfo					= G2API_SetBoltInfo;
	re.G2API_SetBoneAngles					= G2API_SetBoneAngles;
	re.G2API_SetBoneAnglesIndex				= G2API_SetBoneAnglesIndex;
	re.G2API_SetBoneAnglesMatrix			= G2API_SetBoneAnglesMatrix;
	re.G2API_SetBoneAnglesMatrixIndex		= G2API_SetBoneAnglesMatrixIndex;
	re.G2API_SetBoneAnim					= G2API_SetBoneAnim;
	re.G2API_SetBoneAnimIndex				= G2API_SetBoneAnimIndex;
	re.G2API_SetBoneIKState					= G2API_SetBoneIKState;
	re.G2API_SetGhoul2ModelIndexes			= G2API_SetGhoul2ModelIndexes;
	re.G2API_SetGhoul2ModelFlags			= G2API_SetGhoul2ModelFlags;
	re.G2API_SetLodBias						= G2API_SetLodBias;
	re.G2API_SetNewOrigin					= G2API_SetNewOrigin;
	re.G2API_SetRagDoll						= G2API_SetRagDoll;
	re.G2API_SetRootSurface					= G2API_SetRootSurface;
	re.G2API_SetShader						= G2API_SetShader;
	re.G2API_SetSkin						= G2API_SetSkin;
	re.G2API_SetSurfaceOnOff				= G2API_SetSurfaceOnOff;
	re.G2API_SetTime						= G2API_SetTime;
	re.G2API_SkinlessModel					= G2API_SkinlessModel;
	re.G2API_StopBoneAngles					= G2API_StopBoneAngles;
	re.G2API_StopBoneAnglesIndex			= G2API_StopBoneAnglesIndex;
	re.G2API_StopBoneAnim					= G2API_StopBoneAnim;
	re.G2API_StopBoneAnimIndex				= G2API_StopBoneAnimIndex;

	#ifdef _G2_GORE
	re.G2API_GetNumGoreMarks				= G2API_GetNumGoreMarks;
	re.G2API_AddSkinGore					= G2API_AddSkinGore;
	re.G2API_ClearSkinGore					= G2API_ClearSkinGore;
	#endif // _SOF2

	/*
	Ghoul2 Insert End
	*/

	re.LoadDataImage						= R_LoadDataImage;
	re.InvertImage							= R_InvertImage;
	re.Resample								= R_Resample;
	re.LoadImageJA							= R_LoadImage;
	//re.CreateAutomapImage					= R_CreateAutomapImage;
	re.SavePNG								= RE_SavePNG;

	re.TheGhoul2InfoArray					= TheGhoul2InfoArray;
	// G2VertSpaceServer

	return &re;
}
}