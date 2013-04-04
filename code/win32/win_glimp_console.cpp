// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_LogComment
** GLimp_Shutdown
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#include <assert.h>
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

#if defined(_WINDOWS) || defined(_XBOX)
#include "glw_win_dx8.h"
#elif defined(_GAMECUBE)
#include "glw_win_gc.h"
#endif


extern void WG_CheckHardwareGamma( void );

static void		GLW_InitExtensions( void );
static int		GLW_CreateWindow( void );

//
// function declaration
//
void	 QGL_EnableLogging( qboolean enable );
qboolean QGL_Init( const char *dllname );
void     QGL_Shutdown( void );
void	 GLW_Init(int width, int height, int colorbits, qboolean cdsFullscreen);
void	 GLW_Shutdown(void);


//
// variable declarations
//
glwstate_t *glw_state = NULL;


/*
** GLW_CreateWindow
**
** Responsible for creating the Alchemy window and initializing the OpenGL driver.
*/
static qboolean GLW_CreateWindow( int width, int height, int colorbits, qboolean cdsFullscreen )
{
	GLW_Init(width, height, colorbits, cdsFullscreen);
	IN_Init();

	return qtrue;
}

//--------------------------------------------
static void GLW_InitTextureCompression( void )
{
	glConfig.textureCompression = TC_NONE;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
		glConfig.textureEnvAddAvailable = qtrue;
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.textureFilterAnisotropicAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_filter_anisotropic" ) )
	{
		glConfig.textureFilterAnisotropicAvailable = qtrue;
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) )
	{
		glConfig.clampToEdgeAvailable = qtrue;
	}

	// GL_ARB_multitexture
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
		if ( qglActiveTextureARB )
		{
			qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );
			
			if ( glConfig.maxActiveTextures < 2 )
			{
				qglMultiTexCoord2fARB = NULL;
				qglActiveTextureARB = NULL;
				qglClientActiveTextureARB = NULL;
			}
		}
	}
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that attempts to load and use 
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL()
{
	char buffer[1024];

	strlwr( strcpy( buffer, OPENGL_DRIVER_NAME ) );

	//
	// load the driver and bind our function pointers to it
	// 
	if ( QGL_Init( buffer ) ) 
	{
		GLW_CreateWindow(640, 480, 24, 1);
		return qtrue;
	}

	QGL_Shutdown();

	return qfalse;
}


/*
** GLimp_EndFrame
*/
void GLimp_EndFrame (void)
{
	// don't flip if drawing to front buffer
//	if ( stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
	}
}

static void GLW_StartOpenGL( void )
{
	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL() )
	{
		Com_Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
	}
}

/*
** GLimp_Init
**
** This is the platform specific OpenGL initialization function.  It
** is responsible for loading OpenGL, initializing it, setting
** extensions, creating a window of the appropriate size, doing
** fullscreen manipulations, etc.  Its overall responsibility is
** to make sure that a functional OpenGL subsystem is operating
** when it returns to the ref.
*/
void GLimp_Init( void )
{
	// load appropriate DLL and initialize subsystem
	GLW_StartOpenGL();

	// get our config strings
	glConfig.vendor_string = (const char *) qglGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *) qglGetString (GL_RENDERER);
	glConfig.version_string = (const char *) qglGetString (GL_VERSION);
	glConfig.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);
	
	if (!glConfig.vendor_string || !glConfig.renderer_string || !glConfig.version_string || !glConfig.extensions_string)
	{
		Com_Error( ERR_FATAL, "GLimp_Init() - Invalid GL Driver\n" );
	}

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) 
	{
		glConfig.maxTextureSize = 0;
	}

	GLW_InitExtensions();
	WG_CheckHardwareGamma();
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.
*/
void GLimp_Shutdown( void )
{
	// FIXME: Brian, we need better fallbacks from partially initialized failures
	VID_Printf( PRINT_ALL, "Shutting down OpenGL subsystem\n" );

	// Set the gamma back to normal
//	GLimp_SetGamma(1.f);

	// kill input system (tied to window)
	IN_Shutdown();

	// shutdown QGL subsystem
	GLW_Shutdown();
	QGL_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) 
{
}


/*
===========================================================

SMP acceleration

===========================================================
*/

qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {
	return qfalse;
}

void *GLimp_RendererSleep( void ) {
	return NULL;
}


void GLimp_FrontEndSleep( void ) {
}


void GLimp_WakeRenderer( void *data ) {
}
