#include <SDL.h>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../rd-vanilla/tr_local.h"
#include "sdl_qgl.h"
#include "../sys/sys_local.h"

//static SDL_Window *window = NULL;

static float displayAspect;
cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_sdlDriver;

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

static SDL_Window *screen = NULL;
static SDL_GLContext opengl_context;

bool g_bTextureRectangleHack = false;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglCombinerParameterfvNV) (GLenum pname,const GLfloat *params);
void (APIENTRYP qglCombinerParameterivNV) (GLenum pname,const GLint *params);
void (APIENTRYP qglCombinerParameterfNV) (GLenum pname,GLfloat param);
void (APIENTRYP qglCombinerParameteriNV) (GLenum pname,GLint param);
void (APIENTRYP qglCombinerInputNV) (GLenum stage,GLenum portion,GLenum variable,GLenum input,GLenum mapping,
		GLenum componentUsage);
void (APIENTRYP qglCombinerOutputNV) (GLenum stage,GLenum portion,GLenum abOutput,GLenum cdOutput,GLenum sumOutput,
		GLenum scale, GLenum bias,GLboolean abDotProduct,GLboolean cdDotProduct,
		GLboolean muxSum);

void (APIENTRYP qglFinalCombinerInputNV) (GLenum variable,GLenum input,GLenum mapping,GLenum componentUsage);
void (APIENTRYP qglGetCombinerInputParameterfvNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerInputParameterivNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLint *params);
void (APIENTRYP qglGetCombinerOutputParameterfvNV) (GLenum stage,GLenum portion,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerOutputParameterivNV) (GLenum stage,GLenum portion,GLenum pname,GLint *params);
void (APIENTRYP qglGetFinalCombinerInputParameterfvNV) (GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetFinalCombinerInputParameterivNV) (GLenum variable,GLenum pname,GLint *params);

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

void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize(void)
{
	SDL_MinimizeWindow( screen );
}

void GLimp_EndFrame( void )
{
	SDL_GL_SwapWindow(screen);
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = (SDL_Rect *)a;
	SDL_Rect *modeB = (SDL_Rect *)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs( aspectA - displayAspect );
	float aspectDiffB = fabs( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static bool GLimp_DetectAvailableModes(void)
{
	int i;
	char buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect modes[ 128 ];
	int numModes = 0;

	int display = SDL_GetWindowDisplayIndex( screen );
	SDL_DisplayMode windowMode;

	if( SDL_GetWindowDisplayMode( screen, &windowMode ) < 0 )
	{
		Com_Printf( "Couldn't get window display mode, no resolutions detected (%s).\n", SDL_GetError() );
		return false;
	}

	int numDisplayModes = SDL_GetNumDisplayModes( display );
	for( i = 0; i < numDisplayModes; i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( display, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			Com_Printf( "Display supports any resolution\n" );
			return true;
		}

		if( windowMode.format != mode.format )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	if( numModes > 1 )
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			Com_Printf( "Skipping mode %ux%x, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		Com_Printf( "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}

	return true;
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	const char *glstring;
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	//int samples;
	int i = 0;
	//SDL_Surface *icon = NULL;
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	Com_Printf( "Initializing OpenGL display\n");

	if ( r_allowResize->integer && !fullscreen )
		flags |= SDL_WINDOW_RESIZABLE;

	/*icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);*/

	// If a window exists, note its display index
	if( screen != NULL )
		display = SDL_GetWindowDisplayIndex( screen );

	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		Com_Printf( "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		Com_Printf( "Cannot determine display aspect, assuming 1.333\n" );
	}

	Com_Printf( "...setting mode %d:", mode );

	if (mode == -2)
	{
		// use desktop video resolution
		if( desktopMode.h > 0 )
		{
			glConfig.vidWidth = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			Com_Printf( "Cannot determine display resolution, assuming 640x480\n" );
		}

		//glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, /*&glConfig.windowAspect,*/ mode ) )
	{
		Com_Printf( " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	Com_Printf( " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	// Center window
	if( r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode.w / 2 ) - ( glConfig.vidWidth / 2 );
		y = ( desktopMode.h / 2 ) - ( glConfig.vidHeight / 2 );
	}

	// Destroy existing state if it exists
	if( opengl_context != NULL )
	{
		SDL_GL_DeleteContext( opengl_context );
		opengl_context = NULL;
	}

	if( screen != NULL )
	{
		SDL_GetWindowPosition( screen, &x, &y );
		Com_DPrintf( "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( screen );
		screen = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig.isFullscreen = qfalse;
	}

	colorBits = r_colorbits->value;
	if ((!colorBits) || (colorBits >= 32))
		colorBits = 24;

	if (!r_depthbits->value)
		depthBits = 24;
	else
		depthBits = r_depthbits->value;

	stencilBits = r_stencilbits->value;
	//samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1 :
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3 :
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3)
		{ // reduce colorBits
			if (testColorBits == 24)
				testColorBits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthBits
			if (testDepthBits == 24)
				testDepthBits = 16;
			else if (testDepthBits == 16)
				testDepthBits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilBits
			if (testStencilBits == 24)
				testStencilBits = 16;
			else if (testStencilBits == 16)
				testStencilBits = 8;
			else
				testStencilBits = 0;
		}

		if (testColorBits == 24)
			perChannelColorBits = 8;
		else
			perChannelColorBits = 4;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (perChannelColorBits == 4)
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );

		/*SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );*/

		if(r_stereo->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
			SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

		if( ( screen = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y,
				glConfig.vidWidth, glConfig.vidHeight, flags ) ) == NULL )
		{
			Com_DPrintf( "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

		if( fullscreen )
		{
			SDL_DisplayMode mode;

			switch( testColorBits )
			{
				case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
				case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
				default: Com_DPrintf( "testColorBits is %d, can't fullscreen\n", testColorBits ); continue;
			}

			mode.w = glConfig.vidWidth;
			mode.h = glConfig.vidHeight;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue( "r_displayRefresh" );
			mode.driverdata = NULL;

			if( SDL_SetWindowDisplayMode( screen, &mode ) < 0 )
			{
				Com_DPrintf( "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}

		SDL_SetWindowTitle( screen, CLIENT_WINDOW_TITLE );
		//SDL_SetWindowIcon( screen, icon );

		if( ( opengl_context = SDL_GL_CreateContext( screen ) ) == NULL )
		{
			Com_Printf( "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ) );
			continue;
		}

		SDL_GL_SetSwapInterval( r_swapInterval->integer );

		glConfig.colorBits = testColorBits;
		glConfig.depthBits = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		Com_Printf( "Using %d color bits, %d depth, %d stencil display.\n",
				glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
		break;
	}

	/*SDL_FreeSurface( icon );*/

	if (!GLimp_DetectAvailableModes())
	{
		return RSERR_UNKNOWN;
	}

	glstring = (char *) qglGetString (GL_RENDERER);
	Com_Printf( "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		const char *driverName;

		if (SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			Com_Printf( "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n",
					SDL_GetError());
			return qfalse;
		}

		if (!SDL_GetNumVideoDrivers())
		{
			Com_Printf( "SDL_GetNumVideoDrivers( ) FAILED (%s)\n",
					SDL_GetError());
			return qfalse;
		}

		//
		// TODO: Prompt the user to choose a specific video driver.
		driverName = SDL_GetVideoDriver( 0 );
		Com_Printf( "SDL using driver \"%s\"\n", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}

	if (fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		Com_Printf( "Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = GLimp_SetMode(mode, fullscreen, noborder);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			Com_Printf( "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			Com_Printf( "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;
		case RSERR_UNKNOWN:
			Com_Printf( "...ERROR: no display modes could be found.\n" );
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

/*
** GLW_CheckForExtension

  Cannot use strstr directly to differentiate between (for eg) reg_combiners and reg_combiners2
*/

bool GL_CheckForExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return false;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

static void GLW_InitTextureCompression( void )
{
	bool newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = GL_CheckForExtension("ARB_texture_compression") && GL_CheckForExtension("EXT_texture_compression_s3tc");
	old_tc = GL_CheckForExtension("GL_S3_s3tc");

	if ( old_tc )
	{
		Com_Printf ("...GL_S3_s3tc available\n" );
	}

	if ( newer_tc )
	{
		Com_Printf ("...GL_EXT_texture_compression_s3tc available\n" );
	}

	if ( !r_ext_compressed_textures->value )
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		Com_Printf ("...ignoring texture compression\n" );
	}
	else if ( !old_tc && !newer_tc )
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		Com_Printf ("...no supported texture compression method found\n" );
		Com_Printf (".....ignoring texture compression\n" );
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if ( r_ext_preferred_tc_method->integer == TC_NONE )
		{
			// No preference, so pick the best
			if ( newer_tc )
			{
				Com_Printf ("...no tc preference specified\n" );
				Com_Printf (".....using GL_EXT_texture_compression_s3tc\n" );
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				Com_Printf ("...no tc preference specified\n" );
				Com_Printf (".....using GL_S3_s3tc\n" );
				glConfig.textureCompression = TC_S3TC;
			}
		}
		else
		{
			// User has specified a preference, now see if this request can be honored
			if ( old_tc && newer_tc )
			{
				// both are avaiable, so we can use the desired tc method
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					Com_Printf ("...using preferred tc method, GL_S3_s3tc\n" );
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					Com_Printf ("...using preferred tc method, GL_EXT_texture_compression_s3tc\n" );
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			}
			else
			{
				// Both methods are not available, so this gets trickier
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					// Preferring to user older compression
					if ( old_tc )
					{
						Com_Printf ("...using GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf ("...preferred tc method, GL_S3_s3tc not available\n" );
						Com_Printf (".....falling back to GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if ( newer_tc )
					{
						Com_Printf ("...using GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf ("...preferred tc method, GL_EXT_texture_compression_s3tc not available\n" );
						Com_Printf (".....falling back to GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
===============
GLimp_InitExtensions
===============
*/
extern bool g_bDynamicGlowSupported;
static void GLimp_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		Com_Printf ("*** IGNORING OPENGL EXTENSIONS ***\n" );
		g_bDynamicGlowSupported = false;
		ri.Cvar_Set( "r_DynamicGlow","0" );
		return;
	}

	Com_Printf ("Initializing OpenGL extensions\n" );

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( GL_CheckForExtension( "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			Com_Printf ("...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			Com_Printf ("...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_env_add not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.maxTextureFilterAnisotropy = 0;
	if ( GL_CheckForExtension( "EXT_texture_filter_anisotropic" ) )
	{
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF	//can't include glext.h here ... sigh
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy );
		Com_Printf ("...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer > 1 )
		{
			Com_Printf ("...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
		ri.Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
		{
			ri.Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_filter_anisotropic not found\n" );
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qtrue;
	Com_Printf ("...using GL_EXT_texture_edge_clamp\n" );

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( GL_CheckForExtension( "GL_ARB_multitexture" ) )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) SDL_GL_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					Com_Printf ("...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					Com_Printf ("...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			Com_Printf ("...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( GL_CheckForExtension( "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->integer )
		{
			Com_Printf ("...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				Com_Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_compiled_vertex_array not found\n" );
	}

	bool bNVRegisterCombiners = false;
	// Register Combiners.
	if ( GL_CheckForExtension( "GL_NV_register_combiners" ) )
	{
		// NOTE: This extension requires multitexture support (over 2 units).
		if ( glConfig.maxActiveTextures >= 2 )
		{
			bNVRegisterCombiners = true;
			// Register Combiners function pointer address load.	- AReis
			// NOTE: VV guys will _definetly_ not be able to use regcoms. Pixel Shaders are just as good though :-)
			// NOTE: Also, this is an nVidia specific extension (of course), so fragment shaders would serve the same purpose
			// if we needed some kind of fragment/pixel manipulation support.
			qglCombinerParameterfvNV = (PFNGLCOMBINERPARAMETERFVNV)SDL_GL_GetProcAddress( "glCombinerParameterfvNV" );
			qglCombinerParameterivNV = (PFNGLCOMBINERPARAMETERIVNV)SDL_GL_GetProcAddress( "glCombinerParameterivNV" );
			qglCombinerParameterfNV = (PFNGLCOMBINERPARAMETERFNV)SDL_GL_GetProcAddress( "glCombinerParameterfNV" );
			qglCombinerParameteriNV = (PFNGLCOMBINERPARAMETERINV)SDL_GL_GetProcAddress( "glCombinerParameteriNV" );
			qglCombinerInputNV = (PFNGLCOMBINERINPUTNV)SDL_GL_GetProcAddress( "glCombinerInputNV" );
			qglCombinerOutputNV = (PFNGLCOMBINEROUTPUTNV)SDL_GL_GetProcAddress( "glCombinerOutputNV" );
			qglFinalCombinerInputNV = (PFNGLFINALCOMBINERINPUTNV)SDL_GL_GetProcAddress( "glFinalCombinerInputNV" );
			qglGetCombinerInputParameterfvNV	= (PFNGLGETCOMBINERINPUTPARAMETERFVNV)SDL_GL_GetProcAddress( "glGetCombinerInputParameterfvNV" );
			qglGetCombinerInputParameterivNV	= (PFNGLGETCOMBINERINPUTPARAMETERIVNV)SDL_GL_GetProcAddress( "glGetCombinerInputParameterivNV" );
			qglGetCombinerOutputParameterfvNV = (PFNGLGETCOMBINEROUTPUTPARAMETERFVNV)SDL_GL_GetProcAddress( "glGetCombinerOutputParameterfvNV" );
			qglGetCombinerOutputParameterivNV = (PFNGLGETCOMBINEROUTPUTPARAMETERIVNV)SDL_GL_GetProcAddress( "glGetCombinerOutputParameterivNV" );
			qglGetFinalCombinerInputParameterfvNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV)SDL_GL_GetProcAddress( "glGetFinalCombinerInputParameterfvNV" );
			qglGetFinalCombinerInputParameterivNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV)SDL_GL_GetProcAddress( "glGetFinalCombinerInputParameterivNV" );

			// Validate the functions we need.
			if ( !qglCombinerParameterfvNV || !qglCombinerParameterivNV || !qglCombinerParameterfNV || !qglCombinerParameteriNV || !qglCombinerInputNV ||
				 !qglCombinerOutputNV || !qglFinalCombinerInputNV || !qglGetCombinerInputParameterfvNV || !qglGetCombinerInputParameterivNV ||
				 !qglGetCombinerOutputParameterfvNV || !qglGetCombinerOutputParameterivNV || !qglGetFinalCombinerInputParameterfvNV || !qglGetFinalCombinerInputParameterivNV )
			{
				bNVRegisterCombiners = false;
				qglCombinerParameterfvNV = NULL;
				qglCombinerParameteriNV = NULL;
				Com_Printf ("...GL_NV_register_combiners failed\n" );
			}
		}
		else
		{
			bNVRegisterCombiners = false;
			Com_Printf ("...ignoring GL_NV_register_combiners\n" );
		}
	}
	else
	{
		bNVRegisterCombiners = false;
		Com_Printf ("...GL_NV_register_combiners not found\n" );
	}

	// NOTE: Vertex and Fragment Programs are very dependant on each other - this is actually a
	// good thing! So, just check to see which we support (one or the other) and load the shared
	// function pointers. ARB rocks!

	// Vertex Programs.
	bool bARBVertexProgram = false;
	if ( GL_CheckForExtension( "GL_ARB_vertex_program" ) )
	{
		bARBVertexProgram = true;
	}
	else
	{
		bARBVertexProgram = false;
		Com_Printf ("...GL_ARB_vertex_program not found\n" );
	}

	// Fragment Programs.
	bool bARBFragmentProgram = false;
	if ( GL_CheckForExtension( "GL_ARB_fragment_program" ) )
	{
		bARBFragmentProgram = true;
	}
	else
	{
		bARBFragmentProgram = false;
		Com_Printf ("...GL_ARB_fragment_program not found\n" );
	}

	// If we support one or the other, load the shared function pointers.
	if ( bARBVertexProgram || bARBFragmentProgram )
	{
		qglProgramStringARB					= (PFNGLPROGRAMSTRINGARBPROC)  SDL_GL_GetProcAddress("glProgramStringARB");
		qglBindProgramARB					= (PFNGLBINDPROGRAMARBPROC)    SDL_GL_GetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB				= (PFNGLDELETEPROGRAMSARBPROC) SDL_GL_GetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB					= (PFNGLGENPROGRAMSARBPROC)    SDL_GL_GetProcAddress("glGenProgramsARB");
		qglProgramEnvParameter4dARB			= (PFNGLPROGRAMENVPARAMETER4DARBPROC)    SDL_GL_GetProcAddress("glProgramEnvParameter4dARB");
		qglProgramEnvParameter4dvARB		= (PFNGLPROGRAMENVPARAMETER4DVARBPROC)   SDL_GL_GetProcAddress("glProgramEnvParameter4dvARB");
		qglProgramEnvParameter4fARB			= (PFNGLPROGRAMENVPARAMETER4FARBPROC)    SDL_GL_GetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB		= (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   SDL_GL_GetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4dARB		= (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)  SDL_GL_GetProcAddress("glProgramLocalParameter4dARB");
		qglProgramLocalParameter4dvARB		= (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) SDL_GL_GetProcAddress("glProgramLocalParameter4dvARB");
		qglProgramLocalParameter4fARB		= (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)  SDL_GL_GetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB		= (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) SDL_GL_GetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramEnvParameterdvARB		= (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) SDL_GL_GetProcAddress("glGetProgramEnvParameterdvARB");
		qglGetProgramEnvParameterfvARB		= (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) SDL_GL_GetProcAddress("glGetProgramEnvParameterfvARB");
		qglGetProgramLocalParameterdvARB	= (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) SDL_GL_GetProcAddress("glGetProgramLocalParameterdvARB");
		qglGetProgramLocalParameterfvARB	= (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) SDL_GL_GetProcAddress("glGetProgramLocalParameterfvARB");
		qglGetProgramivARB					= (PFNGLGETPROGRAMIVARBPROC)     SDL_GL_GetProcAddress("glGetProgramivARB");
		qglGetProgramStringARB				= (PFNGLGETPROGRAMSTRINGARBPROC) SDL_GL_GetProcAddress("glGetProgramStringARB");
		qglIsProgramARB						= (PFNGLISPROGRAMARBPROC)        SDL_GL_GetProcAddress("glIsProgramARB");

		// Validate the functions we need.
		if ( !qglProgramStringARB || !qglBindProgramARB || !qglDeleteProgramsARB || !qglGenProgramsARB ||
			 !qglProgramEnvParameter4dARB || !qglProgramEnvParameter4dvARB || !qglProgramEnvParameter4fARB ||
             !qglProgramEnvParameter4fvARB || !qglProgramLocalParameter4dARB || !qglProgramLocalParameter4dvARB ||
             !qglProgramLocalParameter4fARB || !qglProgramLocalParameter4fvARB || !qglGetProgramEnvParameterdvARB ||
             !qglGetProgramEnvParameterfvARB || !qglGetProgramLocalParameterdvARB || !qglGetProgramLocalParameterfvARB ||
             !qglGetProgramivARB || !qglGetProgramStringARB || !qglIsProgramARB )
		{
			bARBVertexProgram = false;
			bARBFragmentProgram = false;
			qglGenProgramsARB = NULL;	//clear ptrs that get checked
			qglProgramEnvParameter4fARB = NULL;
			Com_Printf ("...ignoring GL_ARB_vertex_program\n" );
			Com_Printf ("...ignoring GL_ARB_fragment_program\n" );
		}
	}

	// Figure out which texture rectangle extension to use.
	bool bTexRectSupported = false;
	if ( Q_stricmpn( glConfig.vendor_string, "ATI Technologies",16 )==0
		&& Q_stricmpn( glConfig.version_string, "1.3.3",5 )==0
		&& glConfig.version_string[5] < '9' ) //1.3.34 and 1.3.37 and 1.3.38 are broken for sure, 1.3.39 is not
	{
		g_bTextureRectangleHack = true;
	}

	if ( GL_CheckForExtension( "GL_NV_texture_rectangle" ) || GL_CheckForExtension( "GL_EXT_texture_rectangle" ) )
	{
		bTexRectSupported = true;
	}

	// Find out how many general combiners they have.
	#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
	GLint iNumGeneralCombiners = 0;
	if(bNVRegisterCombiners)
		qglGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &iNumGeneralCombiners );

	// Only allow dynamic glows/flares if they have the hardware
	if ( bTexRectSupported && bARBVertexProgram && qglActiveTextureARB && glConfig.maxActiveTextures >= 4 &&
		( ( bNVRegisterCombiners && iNumGeneralCombiners >= 2 ) || bARBFragmentProgram ) )
	{
		g_bDynamicGlowSupported = true;
		// this would overwrite any achived setting gwg
		// ri.Cvar_Set( "r_DynamicGlow", "1" );
	}
	else
	{
		g_bDynamicGlowSupported = false;
		ri.Cvar_Set( "r_DynamicGlow","0" );
	}
}

void 		GLimp_Init( void )
{
	ri.Cvar_Get( "r_restartOnResize", "1", CVAR_ARCHIVE );
	ri.Cvar_Get( "r_resizeDelay", "1000", CVAR_ARCHIVE );
	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE );

	/*	if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		ri.Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		ri.Cvar_Set( "r_picmap", "1" );
		ri.Cvar_Set( "r_texturebits", "0" );
		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		ri.Cvar_Set( "r_fullscreen", "0" );
		ri.Cvar_Set( "r_centerWindow", "0" );
		ri.Cvar_Set( "com_abnormalExit", "0" );
		}*/

	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer))
		goto success;

	// Try again, this time in a platform specific "safe mode"
	/* Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse))
		goto success; */

	/*	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		Com_Printf( "Setting r_mode %d failed, falling back on r_mode %d\n",
				r_mode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse))
			goto success;
			}*/

	// Nothing worked, give up
	Com_Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:
	// This values force the UI to disable driver selection
	//	glConfig.driverType = GLDRV_ICD;
	//	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = (qboolean)(!r_ignorehwgamma->integer &&
		SDL_SetWindowBrightness( screen, 1.0f ) >= 0);

	// get our config strings
    glConfig.vendor_string = (const char *) qglGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *) qglGetString (GL_RENDERER);
	glConfig.version_string = (const char *) qglGetString (GL_VERSION);
	glConfig.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 )
	{
		glConfig.maxTextureSize = 0;
	}

	// initialize extensions
	GLimp_InitExtensions( );

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init( screen );
}

/*
===============
GLimp_Shutdown
===============
*/
void 		GLimp_Shutdown( void )
{
	ri.IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

void		GLimp_EnableLogging( qboolean enable )
{
}

void 		GLimp_LogComment( const char *comment )
{
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	Uint16 table[3][256];
	int i, j;
	
	if( !glConfig.deviceSupportsGamma || r_ignorehwgamma->integer > 0 )
		return;
	
	for (i = 0; i < 256; i++)
	{
		table[0][i] = ( ( ( Uint16 ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( Uint16 ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( Uint16 ) blue[i] ) << 8 ) | blue[i];
	}
	
#ifdef _WIN32
#include <windows.h>
	
	// Win2K and newer put this odd restriction on gamma ramps...
	{
		OSVERSIONINFO	vinfo;
		
		vinfo.dwOSVersionInfoSize = sizeof( vinfo );
		GetVersionEx( &vinfo );
		if( vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			ri.Printf( PRINT_DEVELOPER, "performing gamma clamp.\n" );
			for( j = 0 ; j < 3 ; j++ )
			{
				for( i = 0 ; i < 128 ; i++ )
				{
					if( table[ j ] [ i] > ( ( 128 + i ) << 8 ) )
						table[ j ][ i ] = ( 128 + i ) << 8;
				}
				
				if( table[ j ] [127 ] > 254 << 8 )
					table[ j ][ 127 ] = 254 << 8;
			}
		}
	}
#endif
	
	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i-1])
				table[j][i] = table[j][i-1];
		}
	}
	
	SDL_SetWindowGammaRamp(screen, table[0], table[1], table[2]);
}
