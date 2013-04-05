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
#include "glw_win.h"
#include "win_local.h"
#include "resource.h"	//JFM: to get icon

extern void WG_CheckHardwareGamma( void );
extern void WG_RestoreGamma( void );

typedef enum {
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

#define TRY_PFD_SUCCESS		0
#define TRY_PFD_FAIL_SOFT	1
#define TRY_PFD_FAIL_HARD	2

#define	WINDOW_CLASS_NAME	"Jedi Knight 2"

static void		GLW_InitExtensions( void );
static rserr_t	GLW_SetMode( int mode, 
							 int colorbits, 
							 qboolean cdsFullscreen );

static qboolean s_classRegistered = qfalse;

//
// function declaration
//
void	 QGL_EnableLogging( qboolean enable );
qboolean QGL_Init( const char *dllname );
void     QGL_Shutdown( void );

//
// variable declarations
//
glwstate_t glw_state;

cvar_t	*r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
cvar_t	*r_maskMinidriver;		// allow a different dll name to be treated as if it were opengl32.dll



/*
** GLW_StartDriverAndSetMode
*/
static qboolean GLW_StartDriverAndSetMode( int mode, 
										   int colorbits,
										   qboolean cdsFullscreen )
{
	rserr_t err;

	err = GLW_SetMode( mode, colorbits, cdsFullscreen );

	switch ( err )
	{
	case RSERR_INVALID_FULLSCREEN:
		ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/*
** ChoosePFD
**
** Helper function that replaces ChoosePixelFormat.
*/
#define MAX_PFDS 256

static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD )
{
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
	int maxPFD = 0;
	int i;
	int bestMatch = 0;

	ri.Printf( PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits );

	// count number of PFDs
	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );
	if ( maxPFD > MAX_PFDS )
	{
		ri.Printf( PRINT_WARNING, "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS );
		maxPFD = MAX_PFDS;
	}

	ri.Printf( PRINT_ALL, "...%d PFDs found\n", maxPFD - 1 );

	// grab information
	for ( i = 1; i <= maxPFD; i++ )
	{
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );
	}

	// look for a best match
	for ( i = 1; i <= maxPFD; i++ )
	{
		//
		// make sure this has hardware acceleration
		//
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) 
		{
			if ( !r_allowSoftwareGL->integer )
			{
				if ( r_verbose->integer )
				{
					ri.Printf( PRINT_ALL, "...PFD %d rejected, software acceleration\n", i );
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
		{
			if ( r_verbose->integer )
			{
				ri.Printf( PRINT_ALL, "...PFD %d rejected, not RGBA\n", i );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) 
		{
			if ( r_verbose->integer )
			{
				ri.Printf( PRINT_ALL, "...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags );
			}
			continue;
		}

		// verify enough bits
		if ( pfds[i].cDepthBits < 15 )
		{
			continue;
		}
		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) )
		{
			continue;
		}

		//
		// selection criteria (in order of priority):
		// 
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//
		if ( bestMatch )
		{
			// check stereo
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
			
			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}

			// check color
			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
			{
				// prefer perfect match
				if ( pfds[i].cColorBits == pPFD->cColorBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
			{
				// prefer perfect match
				if ( pfds[i].cDepthBits == pPFD->cDepthBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check stencil
			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
			{
				// prefer perfect match
				if ( pfds[i].cStencilBits == pPFD->cStencilBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) && 
					 ( pPFD->cStencilBits > 0 ) )
				{
					bestMatch = i;
					continue;
				}
			}
		}
		else
		{
			bestMatch = i;
		}
	}
	
	if ( !bestMatch )
		return 0;

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 )
	{
		if ( !r_allowSoftwareGL->integer )
		{
			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
			return 0;
		}
		else
		{
			ri.Printf( PRINT_ALL, "...using software emulation\n" );
		}
	}
	else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
	{
		ri.Printf( PRINT_ALL, "...MCD acceleration found\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	return bestMatch;
}

/*
** void GLW_CreatePFD
**
** Helper function zeros out then fills in a PFD
*/
static void GLW_CreatePFD( PIXELFORMATDESCRIPTOR *pPFD, int colorbits, int depthbits, int stencilbits, qboolean stereo )
{
    PIXELFORMATDESCRIPTOR src = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer	
		8,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };

	src.cColorBits = colorbits;
	src.cDepthBits = depthbits;
	src.cStencilBits = stencilbits;

	if ( stereo )
	{
		ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
		src.dwFlags |= PFD_STEREO;
		glConfig.stereoEnabled = qtrue;
	}
	else
	{
		glConfig.stereoEnabled = qfalse;
	}

	*pPFD = src;
}

/*
** GLW_MakeContext
*/
static int GLW_MakeContext( PIXELFORMATDESCRIPTOR *pPFD )
{
	int pixelformat;

	//
	// don't putz around with pixelformat if it's already set (e.g. this is a soft
	// reset of the graphics system)
	//
	if ( !glw_state.pixelFormatSet )
	{
		//
		// choose, set, and describe our desired pixel format.  If we're
		// using a minidriver then we need to bypass the GDI functions,
		// otherwise use the GDI functions.
		//
		if ( ( pixelformat = GLW_ChoosePFD( glw_state.hDC, pPFD ) ) == 0 )
		{
			ri.Printf( PRINT_ALL, "...GLW_ChoosePFD failed\n");
			return TRY_PFD_FAIL_SOFT;
		}
		ri.Printf( PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelformat );

		DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( *pPFD ), pPFD );

		if ( SetPixelFormat( glw_state.hDC, pixelformat, pPFD ) == FALSE )
		{
			ri.Printf (PRINT_ALL, "...SetPixelFormat failed\n", glw_state.hDC );
			return TRY_PFD_FAIL_SOFT;
		}

		glw_state.pixelFormatSet = qtrue;
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	if ( !glw_state.hGLRC )
	{
		ri.Printf( PRINT_ALL, "...creating GL context: " );
		if ( ( glw_state.hGLRC = qwglCreateContext( glw_state.hDC ) ) == 0 )
		{
			ri.Printf (PRINT_ALL, "failed\n");

			return TRY_PFD_FAIL_HARD;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );

		ri.Printf( PRINT_ALL, "...making context current: " );
		if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) )
		{
			qwglDeleteContext( glw_state.hGLRC );
			glw_state.hGLRC = NULL;
			ri.Printf (PRINT_ALL, "failed\n");
			return TRY_PFD_FAIL_HARD;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );
	}

	return TRY_PFD_SUCCESS;
}


/*
** GLW_InitDriver
**
** - get a DC if one doesn't exist
** - create an HGLRC if one doesn't exist
*/
static qboolean GLW_InitDriver( int colorbits )
{
	int		tpfd;
	int		depthbits, stencilbits;
    static PIXELFORMATDESCRIPTOR pfd;		// save between frames since 'tr' gets cleared

	ri.Printf( PRINT_ALL, "Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if ( glw_state.hDC == NULL )
	{
		ri.Printf( PRINT_ALL, "...getting DC: " );

		if ( ( glw_state.hDC = GetDC( g_wv.hWnd ) ) == NULL )
		{
			ri.Printf( PRINT_ALL, "failed\n" );
			return qfalse;
		}
		ri.Printf( PRINT_ALL, "succeeded\n" );
	}

	if ( colorbits == 0 )
	{
		colorbits = glw_state.desktopBitsPixel;
	}

	//
	// implicitly assume Z-buffer depth == desktop color depth
	//
	if ( r_depthbits->integer == 0 ) {
		if ( colorbits > 16 ) {
			depthbits = 24;
		} else {
			depthbits = 16;
		}
	} else {
		depthbits = r_depthbits->integer;
	}

	//
	// do not allow stencil if Z-buffer depth likely won't contain it
	//
	stencilbits = r_stencilbits->integer;
	if ( depthbits < 24 )
	{
		stencilbits = 0;
	}

	//
	// make two attempts to set the PIXELFORMAT
	//

	//
	// first attempt: r_colorbits, depthbits, and r_stencilbits
	//
	if ( !glw_state.pixelFormatSet )
	{
		GLW_CreatePFD( &pfd, colorbits, depthbits, stencilbits, r_stereo->integer );
		if ( ( tpfd = GLW_MakeContext( &pfd ) ) != TRY_PFD_SUCCESS )
		{
			if ( tpfd == TRY_PFD_FAIL_HARD )
			{
				ri.Printf( PRINT_WARNING, "...failed hard\n" );
				return qfalse;
			}

			//
			// punt if we've already tried the desktop bit depth and no stencil bits
			//
			if ( ( r_colorbits->integer == glw_state.desktopBitsPixel ) &&
				 ( stencilbits == 0 ) )
			{
				ReleaseDC( g_wv.hWnd, glw_state.hDC );
				glw_state.hDC = NULL;

				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}

			//
			// second attempt: desktop's color bits and no stencil
			//
			if ( colorbits > glw_state.desktopBitsPixel )
			{
				colorbits = glw_state.desktopBitsPixel;
			}
			GLW_CreatePFD( &pfd, colorbits, depthbits, 0, r_stereo->integer );
			if ( GLW_MakeContext( &pfd ) != TRY_PFD_SUCCESS )
			{
				if ( glw_state.hDC )
				{
					ReleaseDC( g_wv.hWnd, glw_state.hDC );
					glw_state.hDC = NULL;
				}

				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}
		}

		/*
		** report if stereo is desired but unavailable
		*/
		if ( !( pfd.dwFlags & PFD_STEREO ) && ( r_stereo->integer != 0 ) ) 
		{
			ri.Printf( PRINT_ALL, "...failed to select stereo pixel format\n" );
			glConfig.stereoEnabled = qfalse;
		}
	}

	/*
	** store PFD specifics 
	*/
	glConfig.colorBits = ( int ) pfd.cColorBits;
	glConfig.depthBits = ( int ) pfd.cDepthBits;
	glConfig.stencilBits = ( int ) pfd.cStencilBits;

	return qtrue;
}

/*
** GLW_CreateWindow
**
** Responsible for creating the Win32 window and initializing the OpenGL driver.
*/
#define	WINDOW_STYLE	(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE)
static qboolean GLW_CreateWindow( int width, int height, int colorbits, qboolean cdsFullscreen )
{
	RECT			r;
	cvar_t			*vid_xpos, *vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	//
	// register the window class if necessary
	//
	if ( !s_classRegistered )
	{
		WNDCLASS wc;

		memset( &wc, 0, sizeof( wc ) );

		wc.style         = 0;
		wc.lpfnWndProc   = (WNDPROC) glw_state.wndproc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = g_wv.hInstance;
	    wc.hIcon         = LoadIcon (g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));	//jfm: to get icon
		wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground = (struct HBRUSH__ *)COLOR_GRAYTEXT;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if ( !RegisterClass( &wc ) )
		{
			ri.Error( ERR_FATAL, "GLW_CreateWindow: could not register window class" );
		}
		s_classRegistered = qtrue;
		ri.Printf( PRINT_ALL, "...registered window class\n" );
	}

	//
	// create the HWND if one does not already exist
	//
	if ( !g_wv.hWnd )
	{
		//
		// compute width and height
		//
		r.left = 0;
		r.top = 0;
		r.right  = width;
		r.bottom = height;

		if ( cdsFullscreen )
		{
			exstyle = WS_EX_TOPMOST;
			stylebits = WS_SYSMENU|WS_POPUP|WS_VISIBLE;	//sysmenu gives you the icon
		}
		else
		{
			exstyle = 0;
			stylebits = WS_SYSMENU|WINDOW_STYLE|WS_MINIMIZEBOX;
			AdjustWindowRect (&r, stylebits, FALSE);
		}

		w = r.right - r.left;
		h = r.bottom - r.top;

		if ( cdsFullscreen )
		{
			x = 0;
			y = 0;
		}
		else
		{
			vid_xpos = ri.Cvar_Get ("vid_xpos", "", 0);
			vid_ypos = ri.Cvar_Get ("vid_ypos", "", 0);
			x = vid_xpos->integer;
			y = vid_ypos->integer;

			// adjust window coordinates if necessary 
			// so that the window is completely on screen
			if ( x < 0 )
				x = 0;
			if ( y < 0 )
				y = 0;

			if ( w < glw_state.desktopWidth &&
				 h < glw_state.desktopHeight )
			{
				if ( x + w > glw_state.desktopWidth )
					x = ( glw_state.desktopWidth - w );
				if ( y + h > glw_state.desktopHeight )
					y = ( glw_state.desktopHeight - h );
			}
		}

		g_wv.hWnd = CreateWindowEx (
			 exstyle, 
			 WINDOW_CLASS_NAME,	//class
			 WINDOW_CLASS_NAME,	//window title
			 stylebits,
			 x, y, w, h,
			 NULL,
			 NULL,
			 g_wv.hInstance,
			 NULL);

		if ( !g_wv.hWnd )
		{
			ri.Error (ERR_FATAL, "GLW_CreateWindow() - Couldn't create window");
		}
	
		ShowWindow( g_wv.hWnd, SW_SHOW );
		UpdateWindow( g_wv.hWnd );
		ri.Printf( PRINT_ALL, "...created window@%d,%d (%dx%d)\n", x, y, w, h );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...window already present, CreateWindowEx skipped\n" );
	}

	if ( !GLW_InitDriver( colorbits ) )
	{
		ShowWindow( g_wv.hWnd, SW_HIDE );
		DestroyWindow( g_wv.hWnd );
		g_wv.hWnd = NULL;

		return qfalse;
	}

	SetForegroundWindow( g_wv.hWnd );
	SetFocus( g_wv.hWnd );

	return qtrue;
}

static void PrintCDSError( int value )
{
	switch ( value )
	{
	case DISP_CHANGE_RESTART:
		ri.Printf( PRINT_ALL, "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		ri.Printf( PRINT_ALL, "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		ri.Printf( PRINT_ALL, "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		ri.Printf( PRINT_ALL, "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		ri.Printf( PRINT_ALL, "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		ri.Printf( PRINT_ALL, "not updated\n" );
		break;
	default:
		ri.Printf( PRINT_ALL, "unknown error %d\n", value );
		break;
	}
}

/*
** GLW_SetMode
*/
static rserr_t GLW_SetMode( int mode, 
							int colorbits, 
							qboolean cdsFullscreen )
{
	HDC hDC;
	const char *win_fs[] = { "W", "FS" };
	int		cdsRet;
	DEVMODE dm;
		
	//
	// print out informational messages
	//
	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[cdsFullscreen] );

	//
	// check our desktop attributes
	//
	hDC = GetDC( GetDesktopWindow() );
	glw_state.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	glw_state.desktopWidth = GetDeviceCaps( hDC, HORZRES );
	glw_state.desktopHeight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	//
	// verify desktop bit depth
	//
	if ( glw_state.desktopBitsPixel < 15 || glw_state.desktopBitsPixel == 24 )
	{
		if ( colorbits == 0 || ( !cdsFullscreen && colorbits >= 15 ) )
		{	
			// since I can't be bothered trying to mess around with asian codepages and MBCS stuff for a windows
			//	error box that'll only appear if something's seriously fucked then I'm going to fallback to
			//	english text when these would otherwise be used...
			//
			char sErrorHead[1024];	// ott

			extern qboolean Language_IsAsian(void);
			Q_strncpyz(sErrorHead, Language_IsAsian() ? "Low Desktop Color Depth" : SP_GetStringTextString("CON_TEXT_LOW_DESKTOP_COLOUR_DEPTH"), sizeof(sErrorHead) );

			const char *psErrorBody = Language_IsAsian() ?
												"It is highly unlikely that a correct windowed\n"
												"display can be initialized with the current\n"
												"desktop display depth.  Select 'OK' to try\n"
												"anyway.  Select 'Cancel' to try a fullscreen\n"
												"mode instead."
												:
												SP_GetStringTextString("CON_TEXT_TRY_ANYWAY");

			if ( MessageBox( NULL, 							
						psErrorBody,
						sErrorHead,
						MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK )
			{
				return RSERR_INVALID_MODE;
			}
		}
	}

	// do a CDS if needed
	if ( cdsFullscreen )
	{
		memset( &dm, 0, sizeof( dm ) );
		
		dm.dmSize = sizeof( dm );
		
		dm.dmPelsWidth  = glConfig.vidWidth;
		dm.dmPelsHeight = glConfig.vidHeight;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( r_displayRefresh->integer != 0 )
		{
			dm.dmDisplayFrequency = r_displayRefresh->integer;
			dm.dmFields |= DM_DISPLAYFREQUENCY;
		}
		
		// try to change color depth if possible
		if ( colorbits != 0 )
		{
			if ( glw_state.allowdisplaydepthchange )
			{
				dm.dmBitsPerPel = colorbits;
				dm.dmFields |= DM_BITSPERPEL;
				ri.Printf( PRINT_ALL, "...using colorsbits of %d\n", colorbits );
			}
			else
			{
				ri.Printf( PRINT_ALL, "WARNING:...changing depth not supported on Win95 < pre-OSR 2.x\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...using desktop display depth of %d\n", glw_state.desktopBitsPixel );
		}

		//
		// if we're already in fullscreen then just create the window
		//
		if ( glw_state.cdsFullscreen )
		{
			ri.Printf( PRINT_ALL, "...already fullscreen, avoiding redundant CDS\n" );

			if ( !GLW_CreateWindow ( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) )
			{
				ri.Printf( PRINT_ALL, "...restoring display settings\n" );
				ChangeDisplaySettings( 0, 0 );
				return RSERR_INVALID_MODE;
			}
		}
		//
		// need to call CDS
		//
		else
		{
			ri.Printf( PRINT_ALL, "...calling CDS: " );
			
			// try setting the exact mode requested, because some drivers don't report
			// the low res modes in EnumDisplaySettings, but still work
			if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL )
			{
				ri.Printf( PRINT_ALL, "ok\n" );

				if ( !GLW_CreateWindow ( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue) )
				{
					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );
					return RSERR_INVALID_MODE;
				}
				
				glw_state.cdsFullscreen = qtrue;
			}
			else
			{
				//
				// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
				//
				DEVMODE		devmode;
				int			modeNum;

				ri.Printf( PRINT_ALL, "failed, " );
				
				PrintCDSError( cdsRet );
				
				ri.Printf( PRINT_ALL, "...trying next higher resolution:" );
				
				// we could do a better matching job here...
				for ( modeNum = 0 ; ; modeNum++ ) {
					if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
						modeNum = -1;
						break;
					}
					if ( devmode.dmPelsWidth >= glConfig.vidWidth
						&& devmode.dmPelsHeight >= glConfig.vidHeight
						&& devmode.dmBitsPerPel >= 15 ) {
						break;
					}
				}
				
				if ( modeNum != -1 && ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL )
				{
					ri.Printf( PRINT_ALL, " ok\n" );
					if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue) )
					{
						ri.Printf( PRINT_ALL, "...restoring display settings\n" );
						ChangeDisplaySettings( 0, 0 );
						return RSERR_INVALID_MODE;
					}
					
					glw_state.cdsFullscreen = qtrue;
				}
				else
				{
					ri.Printf( PRINT_ALL, " failed, " );
					
					PrintCDSError( cdsRet );
					
					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );
					
/*				jfm:  i took out the following code to allow fallback to mode 3, with this code it goes half windowed and just doesn't work.
					glw_state.cdsFullscreen = qfalse;
					glConfig.isFullscreen = qfalse;
					if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse) )
					{
						return RSERR_INVALID_MODE;
					}
*/
					return RSERR_INVALID_FULLSCREEN;
				}
			}
		}
	}
	else
	{
		if ( glw_state.cdsFullscreen )
		{
			ChangeDisplaySettings( 0, 0 );
		}

		glw_state.cdsFullscreen = qfalse;
		if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse ) )
		{
			return RSERR_INVALID_MODE;
		}
	}

	//
	// success, now check display frequency, although this won't be valid on Voodoo(2)
	//
	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) )
	{
		glConfig.displayFrequency = dm.dmDisplayFrequency;
	}

	// NOTE: this is overridden later on standalone 3Dfx drivers
	glConfig.isFullscreen = cdsFullscreen;

	return RSERR_OK;
}

//--------------------------------------------
static void GLW_InitTextureCompression( void )
{
	qboolean newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = ( strstr( glConfig.extensions_string, "ARB_texture_compression" )
		&& strstr( glConfig.extensions_string, "EXT_texture_compression_s3tc" )) ? qtrue : qfalse;
	old_tc = ( strstr( glConfig.extensions_string, "GL_S3_s3tc" )) ? qtrue : qfalse;

	if ( old_tc )
	{
		ri.Printf( PRINT_ALL, "...GL_S3_s3tc available\n" );
	}

	if ( newer_tc )
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc available\n" );
	}

	if ( !r_ext_compressed_textures->value )
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		ri.Printf( PRINT_ALL, "...ignoring texture compression\n" );
	}
	else if ( !old_tc && !newer_tc )
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		ri.Printf( PRINT_ALL, "...no supported texture compression method found\n" );
		ri.Printf( PRINT_ALL, ".....ignoring texture compression\n" );
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if ( r_ext_preferred_tc_method->integer == TC_NONE )
		{
			// No preference, so pick the best
			if ( newer_tc )
			{
				ri.Printf( PRINT_ALL, "...no tc preference specified\n" );
				ri.Printf( PRINT_ALL, ".....using GL_EXT_texture_compression_s3tc\n" );
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				ri.Printf( PRINT_ALL, "...no tc preference specified\n" );
				ri.Printf( PRINT_ALL, ".....using GL_S3_s3tc\n" );
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
					ri.Printf( PRINT_ALL, "...using preferred tc method, GL_S3_s3tc\n" );
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					ri.Printf( PRINT_ALL, "...using preferred tc method, GL_EXT_texture_compression_s3tc\n" );
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
						ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored 
						ri.Printf( PRINT_ALL, "...preferred tc method, GL_S3_s3tc not available\n" );
						ri.Printf( PRINT_ALL, ".....falling back to GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if ( newer_tc )
					{
						ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored 
						ri.Printf( PRINT_ALL, "...preferred tc method, GL_EXT_texture_compression_s3tc not available\n" );
						ri.Printf( PRINT_ALL, ".....falling back to GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
** GLW_InitExtensions
*/
cvar_t *r_ATI_NPATCH_available = NULL;
static void GLW_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.textureFilterAnisotropicAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_filter_anisotropic" ) )
	{
		glConfig.textureFilterAnisotropicAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "1" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) )
	{
		glConfig.clampToEdgeAvailable = qtrue;
		ri.Printf( PRINT_ALL, "...Using GL_EXT_texture_edge_clamp\n" );
	}

	// WGL_EXT_swap_control
	qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
	if ( qwglSwapIntervalEXT )
	{
		ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;	// force a set next frame
	}
	else
	{
		ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->integer )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) qwglGetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) qwglGetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				ri.Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_point_parameters" ) )
	{
		if ( r_ext_compiled_vertex_array->integer || 1)
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_point_parameters\n" );
			qglPointParameterfEXT = ( void ( APIENTRY * )( GLenum, GLfloat) ) qwglGetProcAddress( "glPointParameterfEXT" );
			qglPointParameterfvEXT = ( void ( APIENTRY * )( GLenum, GLfloat *) ) qwglGetProcAddress( "glPointParameterfvEXT" );
			if (!qglPointParameterfEXT || !qglPointParameterfvEXT) 
			{
				ri.Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_point_parameters\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_point_parameters not found\n" );
	}

#ifdef _NPATCH
	// GL_ATI_pn_triangles
	qglPNTrianglesiATI = NULL;
	r_ATI_NPATCH_available = ri.Cvar_Get( "r_ATI_NPATCH_available", "0",CVAR_ROM );
/*	if ( strstr( glConfig.extensions_string, "GL_ATI_pn_triangles" ) )
	{
		ri.Cvar_Set( "r_ATI_NPATCH_available", "1" );
		if ( r_ati_pn_triangles->integer )
		{	
			ri.Printf( PRINT_ALL, "...using GL_ATI_pn_triangles\n" );
			qglPNTrianglesiATI = ( void ( APIENTRY * )( GLenum, GLint ) ) qwglGetProcAddress( "glPNTrianglesiATI" );
			if (!qglPNTrianglesiATI) {
				ri.Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ATI_pn_triangles\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ATI_pn_triangles not found\n" );
	}
*/
#endif // _NPATCH
}

/*
** GLW_CheckOSVersion
*/
static qboolean GLW_CheckOSVersion( void )
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = qfalse;

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			glw_state.allowdisplaydepthchange = qtrue;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				glw_state.allowdisplaydepthchange = qtrue;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					glw_state.allowdisplaydepthchange = qtrue;
				}
			}
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "GLW_CheckOSVersion() - GetVersionEx failed\n" );
		return qfalse;
	}

	return qtrue;
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
	qboolean cdsFullscreen;

	strlwr( strcpy( buffer, OPENGL_DRIVER_NAME ) );

	//
	// load the driver and bind our function pointers to it
	// 
	if ( QGL_Init( buffer ) ) 
	{
		cdsFullscreen = r_fullscreen->integer;

		// create the window and set up the context
		if ( !GLW_StartDriverAndSetMode( r_mode->integer, r_colorbits->integer, cdsFullscreen ) )
		{
			// if we're on a 24/32-bit desktop and we're going fullscreen
			// try it again but with a 16-bit desktop
			if ( r_colorbits->integer != 16 ||
				 cdsFullscreen != (int)qtrue ||
				 r_mode->integer != 3 )
			{
				if ( !GLW_StartDriverAndSetMode( 3, 16, qtrue ) )
				{
					goto fail;
				}
			}
		}
		return qtrue;
	}
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** GLimp_EndFrame
*/
void GLimp_EndFrame (void)
{
	//
	// swapinterval stuff
	//
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;

		if ( !glConfig.stereoEnabled ) {	// why?
			if ( qwglSwapIntervalEXT ) {
				qwglSwapIntervalEXT( r_swapInterval->integer );
			}
		}
	}


	// don't flip if drawing to front buffer
	if ( stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SwapBuffers( glw_state.hDC );
	}

	// check logging
	QGL_EnableLogging( r_logFile->integer );
}

static void GLW_StartOpenGL( void )
{
	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL() )
	{
		ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
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
	char	buf[MAX_STRING_CHARS];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	cvar_t	*cv;

	ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

	//
	// check OS version to see if we can do fullscreen display changes
	//
	if ( !GLW_CheckOSVersion() )
	{
		ri.Error( ERR_FATAL, "GLimp_Init() - incorrect operating system\n" );
	}

	// save off hInstance and wndproc
	cv = ri.Cvar_Get( "win_hinstance", "", 0 );
	sscanf( cv->string, "%i", (int *)&g_wv.hInstance );

	cv = ri.Cvar_Get( "win_wndproc", "", 0 );
	sscanf( cv->string, "%i", (int *)&glw_state.wndproc );

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

	// load appropriate DLL and initialize subsystem
	GLW_StartOpenGL();

	// get our config strings
	glConfig.vendor_string = (const char *) qglGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *) qglGetString (GL_RENDERER);
	glConfig.version_string = (const char *) qglGetString (GL_VERSION);
	glConfig.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);
	
	if (!glConfig.vendor_string || !glConfig.renderer_string || !glConfig.version_string || !glConfig.extensions_string)
	{
		ri.Error( ERR_FATAL, "GLimp_Init() - Invalid GL Driver\n" );
	}

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) 
	{
		glConfig.maxTextureSize = 0;
	}

	//
	// chipset specific configuration
	//
	strcpy( buf, glConfig.renderer_string );
	strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
extern qboolean Sys_LowPhysicalMemory();
	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
	{
		if (Sys_LowPhysicalMemory())
		{
			ri.Cvar_Set("s_khz", "11");// this will get called before S_Init
			ri.Cvar_Set("cg_VariantSoundCap", "2");
			ri.Cvar_Set("s_allowDynamicMusic","0");
		}
		//reset to defaults
		ri.Cvar_Set( "r_picmip", "1" );
		
		if ( strstr( buf, "matrox" )) {
            ri.Cvar_Set( "r_allowExtensions", "0");			
		}
		else
		// Savage3D and Savage4 should always have trilinear enabled
		if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) || strstr( buf, "geforce" ))
		{
			ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		}
		else
		{
			ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		}

		if ( strstr( buf, "kyro" ) )	
		{
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0");	//KYROs have it avail, but suck at it!
			ri.Cvar_Set( "r_ext_preferred_tc_method", "1");			//(Use DXT1 instead of DXT5 - same quality but much better performance on KYRO)
		}

		GLW_InitExtensions();
		
		//this must be a really sucky card!
		if ( (glConfig.textureCompression == TC_NONE) || (glConfig.maxActiveTextures < 2)  || (glConfig.maxTextureSize <= 512) )
		{
			ri.Cvar_Set( "r_picmip", "2");
			ri.Cvar_Set( "r_colorbits", "16");
			ri.Cvar_Set( "r_texturebits", "16");
			ri.Cvar_Set( "r_mode", "3");	//force 640
			Cmd_ExecuteString ("exec low.cfg\n");	//get the rest which can be pulled in after init
		}
	}
	
	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

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
//	const char *strings[] = { "soft", "hard" };
	const char *success[] = { "failed", "success" };
	int retVal;

	// FIXME: Brian, we need better fallbacks from partially initialized failures
	if ( !qwglMakeCurrent ) {
		return;
	}

	ri.Printf( PRINT_ALL, "Shutting down OpenGL subsystem\n" );

	// restore gamma.  We do this first because 3Dfx's extension needs a valid OGL subsystem
	WG_RestoreGamma();

	// set current context to NULL
	if ( qwglMakeCurrent )
	{
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;

		ri.Printf( PRINT_ALL, "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if ( glw_state.hGLRC )
	{
		retVal = qwglDeleteContext( glw_state.hGLRC ) != 0;
		ri.Printf( PRINT_ALL, "...deleting GL context: %s\n", success[retVal] );
		glw_state.hGLRC = NULL;
	}

	// release DC
	if ( glw_state.hDC )
	{
		retVal = ReleaseDC( g_wv.hWnd, glw_state.hDC ) != 0;
		ri.Printf( PRINT_ALL, "...releasing DC: %s\n", success[retVal] );
		glw_state.hDC   = NULL;
	}

	// destroy window
	if ( g_wv.hWnd )
	{
		ri.Printf( PRINT_ALL, "...destroying window\n" );
		ShowWindow( g_wv.hWnd, SW_HIDE );
		DestroyWindow( g_wv.hWnd );
		g_wv.hWnd = NULL;
		glw_state.pixelFormatSet = qfalse;
	}

	// close the r_logFile
	if ( glw_state.log_fp )
	{
		fclose( glw_state.log_fp );
		glw_state.log_fp = 0;
	}

	// reset display settings
	if ( glw_state.cdsFullscreen )
	{
		ri.Printf( PRINT_ALL, "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		glw_state.cdsFullscreen = qfalse;
	}

	// shutdown QGL subsystem
	QGL_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) 
{
	if ( glw_state.log_fp ) {
		fprintf( glw_state.log_fp, "%s", comment );
	}
}


/*
===========================================================

SMP acceleration

===========================================================
*/

HANDLE	renderCommandsEvent;
HANDLE	renderCompletedEvent;
HANDLE	renderActiveEvent;

void (*glimpRenderThread)( void );

void GLimp_RenderThreadWrapper( void ) {
	glimpRenderThread();

	// unbind the context before we die
	qwglMakeCurrent( glw_state.hDC, NULL );
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
HANDLE	renderThreadHandle;
int		renderThreadId;
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {

	renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   (unsigned long *) &renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

static	void	*smpData;
static	int		wglErrors;

void *GLimp_RendererSleep( void ) {
	void	*data;

	if ( !qwglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( renderCompletedEvent );

	WaitForSingleObject( renderCommandsEvent, INFINITE );

	if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}

	ResetEvent( renderCompletedEvent );
	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	WaitForSingleObject( renderCompletedEvent, INFINITE );

	if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	if ( !qwglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent( renderCommandsEvent );

	WaitForSingleObject( renderActiveEvent, INFINITE );
}

