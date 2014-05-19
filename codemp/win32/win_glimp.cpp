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
#include "tr_local.h"

#include "resource.h"
#include "glw_win.h"
#include "win_local.h"
#include "qcommon/stringed_ingame.h"
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

#define	WINDOW_CLASS_NAME CLIENT_WINDOW_TITLE

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

// Whether the current hardware supports dynamic glows/flares.
extern bool g_bDynamicGlowSupported;

// Hack variable for deciding which kind of texture rectangle thing to do (for some
// reason it acts different on radeon! It's against the spec!).
bool g_bTextureRectangleHack = false;

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
		Com_Printf("...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		Com_Printf ("...WARNING: could not set the given mode (%d)\n", mode );
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

	Com_Printf ("...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits );

	// count number of PFDs
	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );

	if ( maxPFD > MAX_PFDS )
	{
		Com_Printf (S_COLOR_YELLOW  "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS );
		maxPFD = MAX_PFDS;
	}

	Com_Printf ("...%d PFDs found\n", maxPFD - 1 );

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
					Com_Printf ("...PFD %d rejected, software acceleration\n", i );
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
		{
			if ( r_verbose->integer )
			{
				Com_Printf ("...PFD %d rejected, not RGBA\n", i );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags )
		{
			if ( r_verbose->integer )
			{
				Com_Printf ("...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags );
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
			Com_Printf ("...no hardware acceleration found\n" );
			return 0;
		}
		else
		{
			Com_Printf ("...using software emulation\n" );
		}
	}
	else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
	{
		Com_Printf ("...MCD acceleration found\n" );
	}
	else
	{
		Com_Printf ("...hardware acceleration found\n" );
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
		Com_Printf ("...attempting to use stereo\n" );
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
			Com_Printf ("...GLW_ChoosePFD failed\n");
			return TRY_PFD_FAIL_SOFT;
		}
		Com_Printf ("...PIXELFORMAT %d selected\n", pixelformat );

		DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( *pPFD ), pPFD );

		if ( SetPixelFormat( glw_state.hDC, pixelformat, pPFD ) == FALSE )
		{
			Com_Printf ( "...SetPixelFormat failed\n", glw_state.hDC );
			return TRY_PFD_FAIL_SOFT;
		}

		glw_state.pixelFormatSet = qtrue;
	}

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	if ( !glw_state.hGLRC )
	{
		Com_Printf ("...creating GL context: " );
		if ( ( glw_state.hGLRC = qwglCreateContext( glw_state.hDC ) ) == 0 )
		{
			Com_Printf ( "failed\n");

			return TRY_PFD_FAIL_HARD;
		}
		Com_Printf ("succeeded\n" );

		Com_Printf ("...making context current: " );
		if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) )
		{
			qwglDeleteContext( glw_state.hGLRC );
			glw_state.hGLRC = NULL;
			Com_Printf ( "failed\n");
			return TRY_PFD_FAIL_HARD;
		}
		Com_Printf ("succeeded\n" );
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

	Com_Printf ("Initializing OpenGL driver\n" );

	//
	// get a DC for our window if we don't already have one allocated
	//
	if ( glw_state.hDC == NULL )
	{
		Com_Printf ("...getting DC: " );

		if ( ( glw_state.hDC = GetDC( tr.wv->hWnd ) ) == NULL )
		{
			Com_Printf ("failed\n" );
			return qfalse;
		}
		Com_Printf ("succeeded\n" );
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
		GLW_CreatePFD( &pfd, colorbits, depthbits, stencilbits, (qboolean)r_stereo->integer );
		if ( ( tpfd = GLW_MakeContext( &pfd ) ) != TRY_PFD_SUCCESS )
		{
			if ( tpfd == TRY_PFD_FAIL_HARD )
			{
				Com_Printf (S_COLOR_YELLOW  "...failed hard\n" );
				return qfalse;
			}

			//
			// punt if we've already tried the desktop bit depth and no stencil bits
			//
			if ( ( r_colorbits->integer == glw_state.desktopBitsPixel ) &&
				 ( stencilbits == 0 ) )
			{
				ReleaseDC( tr.wv->hWnd, glw_state.hDC );
				glw_state.hDC = NULL;

				Com_Printf ("...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}

			//
			// second attempt: desktop's color bits and no stencil
			//
			if ( colorbits > glw_state.desktopBitsPixel )
			{
				colorbits = glw_state.desktopBitsPixel;
			}
			GLW_CreatePFD( &pfd, colorbits, depthbits, 0, (qboolean)r_stereo->integer );
			if ( GLW_MakeContext( &pfd ) != TRY_PFD_SUCCESS )
			{
				if ( glw_state.hDC )
				{
					ReleaseDC( tr.wv->hWnd, glw_state.hDC );
					glw_state.hDC = NULL;
				}

				Com_Printf ("...failed to find an appropriate PIXELFORMAT\n" );

				return qfalse;
			}
		}

		/*
		** report if stereo is desired but unavailable
		*/
		if ( !( pfd.dwFlags & PFD_STEREO ) && ( r_stereo->integer != 0 ) )
		{
			Com_Printf ("...failed to select stereo pixel format\n" );
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
	DWORD			stylebits;
	int				x, y, w, h;
	DWORD			exstyle;

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
		wc.hInstance     = tr.wv->hInstance;
		wc.hIcon         = LoadIcon( tr.wv->hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground = 0;//(HBRUSH__ *)COLOR_GRAYTEXT;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = WINDOW_CLASS_NAME;

		if ( !RegisterClass( &wc ) )
		{
			Com_Error( ERR_FATAL, "GLW_CreateWindow: could not register window class, error code: 0x%x", GetLastError() );
		}
		s_classRegistered = qtrue;
//		Com_Printf ("...registered window class\n" );
	}

	//
	// create the HWND if one does not already exist
	//
	if ( !tr.wv->hWnd )
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
			exstyle = 0;//WS_EX_TOPMOST;
			stylebits = WS_SYSMENU|WS_POPUP|WS_VISIBLE;	//sysmenu gives you the icon
		}
		else
		{
			exstyle = 0;
			if ( r_noborder->integer == 0 )
			{
				stylebits = WS_SYSMENU|WINDOW_STYLE|WS_MINIMIZEBOX;
			}
			else
			{
				stylebits = WS_POPUP|WS_VISIBLE;
			}
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
			vid_xpos = ri->Cvar_Get ("vid_xpos", "", 0);
			vid_ypos = ri->Cvar_Get ("vid_ypos", "", 0);
			if ( r_centerWindow->integer == 0 )
			{
				x = vid_xpos->integer;
				y = vid_ypos->integer;
			}
			else
			{
				x = ( glw_state.desktopWidth - w ) / 2;
				y = ( glw_state.desktopHeight - h ) / 2;
			}

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

		tr.wv->hWnd = CreateWindowEx (
			 exstyle,
			 WINDOW_CLASS_NAME,
			 WINDOW_CLASS_NAME,
			 stylebits,
			 x, y, w, h,
			 NULL,
			 NULL,
			 tr.wv->hInstance,
			 NULL);

		if ( !tr.wv->hWnd )
		{
			Com_Error (ERR_FATAL, "GLW_CreateWindow() - Couldn't create window");
		}

		ShowWindow( tr.wv->hWnd, SW_SHOW );
		UpdateWindow( tr.wv->hWnd );
		Com_Printf ("...created window@%d,%d (%dx%d)\n", x, y, w, h );
	}
	else
	{
		Com_Printf ("...window already present, CreateWindowEx skipped\n" );
	}

	if ( !GLW_InitDriver( colorbits ) )
	{
		ShowWindow( tr.wv->hWnd, SW_HIDE );
		DestroyWindow( tr.wv->hWnd );
		tr.wv->hWnd = NULL;

		return qfalse;
	}

	SetForegroundWindow( tr.wv->hWnd );
	SetFocus( tr.wv->hWnd );

	return qtrue;
}

static void PrintCDSError( int value )
{
	switch ( value )
	{
	case DISP_CHANGE_RESTART:
		Com_Printf ("restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		Com_Printf ("bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		Com_Printf ("bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		Com_Printf ("DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		Com_Printf ("bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		Com_Printf ("not updated\n" );
		break;
	default:
		Com_Printf ("unknown error %d\n", value );
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
	Com_Printf ("...setting mode %d:", mode );
	if (mode == -2)
	{
		int OSwidth = GetSystemMetrics (SM_CXSCREEN);
		int OSheight = GetSystemMetrics (SM_CYSCREEN);

		// use desktop video resolution
		if( OSheight > 0 )
		{
			glConfig.vidWidth = OSwidth;
			glConfig.vidHeight = OSheight;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			Com_Printf( "Cannot determine display resolution, assuming 640x480\n" );
		}

		//TODO Aspect stuff?
		//glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, mode ) )
	{
		Com_Printf (" invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	Com_Printf (" %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[cdsFullscreen] );

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
		if (!cdsFullscreen && (colorbits == 0 || colorbits >= 15 ) )
		{
			// since I can't be bothered trying to mess around with asian codepages and MBCS stuff for a windows
			//	error box that'll only appear if something's seriously fucked then I'm going to fallback to
			//	english text when these would otherwise be used...
			//
			char sErrorHead[1024];	// ott

			extern qboolean Language_IsAsian(void);
			Q_strncpyz(sErrorHead, Language_IsAsian() ? "Low Desktop Color Depth" : ri->SE_GetString("CON_TEXT_LOW_DESKTOP_COLOUR_DEPTH"), sizeof(sErrorHead) );

			const char *psErrorBody = Language_IsAsian() ?
												"It is highly unlikely that a correct windowed\n"
												"display can be initialized with the current\n"
												"desktop display depth.  Select 'OK' to try\n"
												"anyway.  Select 'Cancel' to try a fullscreen\n"
												"mode instead."
												:
												ri->SE_GetString("CON_TEXT_TRY_ANYWAY");

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
				Com_Printf ("...using colorsbits of %d\n", colorbits );
			}
			else
			{
				Com_Printf ("WARNING:...changing depth not supported on Win95 < pre-OSR 2.x\n" );
			}
		}
		else
		{
			Com_Printf ("...using desktop display depth of %d\n", glw_state.desktopBitsPixel );
		}

		//
		// if we're already in fullscreen then just create the window
		//
		if ( glw_state.cdsFullscreen )
		{
			Com_Printf ("...already fullscreen, avoiding redundant CDS\n" );

			if ( !GLW_CreateWindow ( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) )
			{
				Com_Printf ("...restoring display settings\n" );
				ChangeDisplaySettings( 0, 0 );
				return RSERR_INVALID_MODE;
			}
		}
		//
		// need to call CDS
		//
		else
		{
			Com_Printf ("...calling CDS: " );

			// try setting the exact mode requested, because some drivers don't report
			// the low res modes in EnumDisplaySettings, but still work
			if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL )
			{
				Com_Printf ("ok\n" );

				if ( !GLW_CreateWindow ( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue) )
				{
					Com_Printf ("...restoring display settings\n" );
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

				Com_Printf ("failed, " );

				PrintCDSError( cdsRet );

				Com_Printf ("...trying next higher resolution:" );

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
					Com_Printf (" ok\n" );
					if ( !GLW_CreateWindow( glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue) )
					{
						Com_Printf ("...restoring display settings\n" );
						ChangeDisplaySettings( 0, 0 );
						return RSERR_INVALID_MODE;
					}

					glw_state.cdsFullscreen = qtrue;
				}
				else
				{
					Com_Printf (" failed, " );

					PrintCDSError( cdsRet );

					Com_Printf ("...restoring display settings\n" );
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

/*
** GLW_CheckForExtension

  Cannot use strstr directly to differentiate between (for eg) reg_combiners and reg_combiners2
*/

bool GL_CheckForExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfigExt.originalExtensionString, ext );
	if (ptr == NULL)
		return false;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

static const char *wglExtensions = NULL;

/* WGL version of the above, ASSUMES wglExtensions is non-null */
bool WGL_CheckForExtension(const char *ext)
{
	const char *ptr = Q_stristr( wglExtensions, ext );
	if (ptr == NULL)
		return false;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

//--------------------------------------------
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
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		Com_Printf ("*** IGNORING OPENGL EXTENSIONS ***\n" );
		g_bDynamicGlowSupported = false;
		ri->Cvar_Set( "r_DynamicGlow","0" );
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
		ri->Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
		{
			ri->Cvar_SetValue( "r_ext_texture_filter_anisotropic_avail", glConfig.maxTextureFilterAnisotropy );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_filter_anisotropic not found\n" );
		ri->Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qtrue;
	Com_Printf ("...using GL_EXT_texture_edge_clamp\n" );

	// WGL_EXT_swap_control
	qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
	if ( qwglSwapIntervalEXT )
	{
		Com_Printf ("...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;	// force a set next frame
	}
	else
	{
		Com_Printf ("...WGL_EXT_swap_control not found\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( GL_CheckForExtension( "GL_ARB_multitexture" ) )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				GLint glint = 0;
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glint );
				glConfig.maxActiveTextures = (int) glint;

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
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) qwglGetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) qwglGetProcAddress( "glUnlockArraysEXT" );
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
			qglCombinerParameterfvNV = ( PFNGLCOMBINERPARAMETERFVNV ) qwglGetProcAddress( "glCombinerParameterfvNV" );
			qglCombinerParameterivNV = ( PFNGLCOMBINERPARAMETERIVNV ) qwglGetProcAddress( "glCombinerParameterivNV" );
			qglCombinerParameterfNV = ( PFNGLCOMBINERPARAMETERFNV ) qwglGetProcAddress( "glCombinerParameterfNV" );
			qglCombinerParameteriNV = ( PFNGLCOMBINERPARAMETERINV ) qwglGetProcAddress( "glCombinerParameteriNV" );
			qglCombinerInputNV = ( PFNGLCOMBINERINPUTNV ) qwglGetProcAddress( "glCombinerInputNV" );
			qglCombinerOutputNV = ( PFNGLCOMBINEROUTPUTNV ) qwglGetProcAddress( "glCombinerOutputNV" );
			qglFinalCombinerInputNV = ( PFNGLFINALCOMBINERINPUTNV ) qwglGetProcAddress( "glFinalCombinerInputNV" );
			qglGetCombinerInputParameterfvNV	= ( PFNGLGETCOMBINERINPUTPARAMETERFVNV ) qwglGetProcAddress( "glGetCombinerInputParameterfvNV" );
			qglGetCombinerInputParameterivNV	= ( PFNGLGETCOMBINERINPUTPARAMETERIVNV ) qwglGetProcAddress( "glGetCombinerInputParameterivNV" );
			qglGetCombinerOutputParameterfvNV = ( PFNGLGETCOMBINEROUTPUTPARAMETERFVNV ) qwglGetProcAddress( "glGetCombinerOutputParameterfvNV" );
			qglGetCombinerOutputParameterivNV = ( PFNGLGETCOMBINEROUTPUTPARAMETERIVNV ) qwglGetProcAddress( "glGetCombinerOutputParameterivNV" );
			qglGetFinalCombinerInputParameterfvNV = ( PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV ) qwglGetProcAddress( "glGetFinalCombinerInputParameterfvNV" );
			qglGetFinalCombinerInputParameterivNV = ( PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV ) qwglGetProcAddress( "glGetFinalCombinerInputParameterivNV" );

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
		qglProgramStringARB					= (PFNGLPROGRAMSTRINGARBPROC)  qwglGetProcAddress("glProgramStringARB");
		qglBindProgramARB					= (PFNGLBINDPROGRAMARBPROC)    qwglGetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB				= (PFNGLDELETEPROGRAMSARBPROC) qwglGetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB					= (PFNGLGENPROGRAMSARBPROC)    qwglGetProcAddress("glGenProgramsARB");
		qglProgramEnvParameter4dARB			= (PFNGLPROGRAMENVPARAMETER4DARBPROC)    qwglGetProcAddress("glProgramEnvParameter4dARB");
		qglProgramEnvParameter4dvARB		= (PFNGLPROGRAMENVPARAMETER4DVARBPROC)   qwglGetProcAddress("glProgramEnvParameter4dvARB");
		qglProgramEnvParameter4fARB			= (PFNGLPROGRAMENVPARAMETER4FARBPROC)    qwglGetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB		= (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   qwglGetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4dARB		= (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)  qwglGetProcAddress("glProgramLocalParameter4dARB");
		qglProgramLocalParameter4dvARB		= (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) qwglGetProcAddress("glProgramLocalParameter4dvARB");
		qglProgramLocalParameter4fARB		= (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)  qwglGetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB		= (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) qwglGetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramEnvParameterdvARB		= (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) qwglGetProcAddress("glGetProgramEnvParameterdvARB");
		qglGetProgramEnvParameterfvARB		= (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) qwglGetProcAddress("glGetProgramEnvParameterfvARB");
		qglGetProgramLocalParameterdvARB	= (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) qwglGetProcAddress("glGetProgramLocalParameterdvARB");
		qglGetProgramLocalParameterfvARB	= (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) qwglGetProcAddress("glGetProgramLocalParameterfvARB");
		qglGetProgramivARB					= (PFNGLGETPROGRAMIVARBPROC)     qwglGetProcAddress("glGetProgramivARB");
		qglGetProgramStringARB				= (PFNGLGETPROGRAMSTRINGARBPROC) qwglGetProcAddress("glGetProgramStringARB");
		qglIsProgramARB						= (PFNGLISPROGRAMARBPROC)        qwglGetProcAddress("glIsProgramARB");

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

	// OK, so not so good to put this here, but no one else uses it!!! -AReis
	typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
	PFNWGLGETEXTENSIONSSTRINGARBPROC			qwglGetExtensionsStringARB;
	qwglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) qwglGetProcAddress("wglGetExtensionsStringARB");

	bool bHasPixelFormat = false;
	bool bHasRenderTexture = false;

	// Get the WGL extensions string.
	if ( qwglGetExtensionsStringARB )
	{
		wglExtensions = qwglGetExtensionsStringARB( glw_state.hDC );
	}

	// This externsion is used to get the wgl extension string.
	if ( wglExtensions )
	{
		// Pixel Format.
		if ( WGL_CheckForExtension( "WGL_ARB_pixel_format" ) )
		{
			qwglGetPixelFormatAttribivARB			=	(PFNWGLGETPIXELFORMATATTRIBIVARBPROC) qwglGetProcAddress("wglGetPixelFormatAttribivARB");
			qwglGetPixelFormatAttribfvARB			=	(PFNWGLGETPIXELFORMATATTRIBFVARBPROC) qwglGetProcAddress("wglGetPixelFormatAttribfvARB");
			qwglChoosePixelFormatARB				=	(PFNWGLCHOOSEPIXELFORMATARBPROC) qwglGetProcAddress("wglChoosePixelFormatARB");

			// Validate the functions we need.
			if ( !qwglGetPixelFormatAttribivARB || !qwglGetPixelFormatAttribfvARB || !qwglChoosePixelFormatARB )
			{
				Com_Printf ("...ignoring WGL_ARB_pixel_format\n" );
			}
			else
			{
				bHasPixelFormat = true;
			}
		}
		else
		{
			Com_Printf ("...ignoring WGL_ARB_pixel_format\n" );
		}

		// Offscreen pixel-buffer.
		// NOTE: VV guys can use the equivelant SetRenderTarget() with the correct texture surfaces.
		bool bWGLARBPbuffer = false;
		if ( WGL_CheckForExtension( "WGL_ARB_pbuffer" ) && bHasPixelFormat )
		{
			bWGLARBPbuffer = true;
			qwglCreatePbufferARB		=	(PFNWGLCREATEPBUFFERARBPROC) qwglGetProcAddress("wglCreatePbufferARB");
			qwglGetPbufferDCARB			=	(PFNWGLGETPBUFFERDCARBPROC) qwglGetProcAddress("wglGetPbufferDCARB");
			qwglReleasePbufferDCARB		=	(PFNWGLRELEASEPBUFFERDCARBPROC) qwglGetProcAddress("wglReleasePbufferDCARB");
			qwglDestroyPbufferARB		=	(PFNWGLDESTROYPBUFFERARBPROC) qwglGetProcAddress("wglDestroyPbufferARB");
			qwglQueryPbufferARB			=	(PFNWGLQUERYPBUFFERARBPROC) qwglGetProcAddress("wglQueryPbufferARB");

			// Validate the functions we need.
			if ( !qwglCreatePbufferARB || !qwglGetPbufferDCARB || !qwglReleasePbufferDCARB || !qwglDestroyPbufferARB || !qwglQueryPbufferARB )
			{
				bWGLARBPbuffer = false;
				Com_Printf ("...WGL_ARB_pbuffer failed\n" );
			}
		}
		else
		{
			bWGLARBPbuffer = false;
			Com_Printf ("...WGL_ARB_pbuffer not found\n" );
		}

		// Render-Texture (requires pbuffer ext (and it's dependancies of course).
		if ( WGL_CheckForExtension( "WGL_ARB_render_texture" ) && bWGLARBPbuffer )
		{
			qwglBindTexImageARB			=	(PFNWGLBINDTEXIMAGEARBPROC) qwglGetProcAddress("wglBindTexImageARB");
			qwglReleaseTexImageARB		=	(PFNWGLRELEASETEXIMAGEARBPROC) qwglGetProcAddress("wglReleaseTexImageARB");
			qwglSetPbufferAttribARB		=	(PFNWGLSETPBUFFERATTRIBARBPROC) qwglGetProcAddress("wglSetPbufferAttribARB");

			// Validate the functions we need.
			if ( !qwglCreatePbufferARB || !qwglGetPbufferDCARB || !qwglReleasePbufferDCARB || !qwglDestroyPbufferARB || !qwglQueryPbufferARB )
			{
				Com_Printf ("...ignoring WGL_ARB_render_texture\n" );
			}
			else
			{
				bHasRenderTexture = true;
			}
		}
		else
		{
			Com_Printf ("...ignoring WGL_ARB_render_texture\n" );
		}
	}

	// Find out how many general combiners they have.
	#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
	GLint iNumGeneralCombiners = 0;
	if(bNVRegisterCombiners)
		qglGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &iNumGeneralCombiners );

	// Only allow dynamic glows/flares if they have the hardware
	if ( bTexRectSupported && bARBVertexProgram && bHasRenderTexture && qglActiveTextureARB && glConfig.maxActiveTextures >= 4 &&
		( ( bNVRegisterCombiners && iNumGeneralCombiners >= 2 ) || bARBFragmentProgram ) )
	{
		g_bDynamicGlowSupported = true;
		// this would overwrite any achived setting gwg
		// ri.Cvar_Set( "r_DynamicGlow", "1" );
	}
	else
	{
		g_bDynamicGlowSupported = false;
		ri->Cvar_Set( "r_DynamicGlow","0" );
	}
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
		Com_Printf ("GLW_CheckOSVersion() - GetVersionEx failed\n" );
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
static qboolean GLW_LoadOpenGL( )
{
	qboolean cdsFullscreen;

	//
	// load the driver and bind our function pointers to it
	//
	if ( QGL_Init( "opengl32" ) )
	{
		cdsFullscreen = (qboolean)r_fullscreen->integer;

		// create the window and set up the context
		if ( !GLW_StartDriverAndSetMode( r_mode->integer, r_colorbits->integer, cdsFullscreen ) )
		{
			// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
			// try it again but with a 16-bit desktop
			if ( r_colorbits->integer != 16 ||
				 cdsFullscreen != qtrue ||
				 r_mode->integer != 3 )
			{
				if ( !GLW_StartDriverAndSetMode( 3, 16, qtrue ) )
				{
					goto fail;
				}
			}
		}

#ifdef _CRAZY_ATTRIB_DEBUG
		//I can get away with this because we don't actually use push/pop attrib anywhere else.
		qglPushAttrib(GL_ACCUM_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT|
			GL_ENABLE_BIT|GL_EVAL_BIT|GL_FOG_BIT|GL_HINT_BIT|GL_LIGHTING_BIT|GL_LINE_BIT|GL_LIST_BIT|
			GL_PIXEL_MODE_BIT|GL_POINT_BIT|GL_POLYGON_BIT|GL_POLYGON_STIPPLE_BIT|GL_SCISSOR_BIT|
			GL_STENCIL_BUFFER_BIT|GL_TEXTURE_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT);
#endif

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
	//if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SwapBuffers( glw_state.hDC );
	}

	// check logging
	QGL_EnableLogging( (qboolean)r_logFile->integer );
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

// Truncates the GL extensions string by only allowing up to 'maxExtensions' extensions in the string.
static const char *TruncateGLExtensionsString (const char *extensionsString, int maxExtensions)
{
	const char *p = extensionsString;
	const char *q;
	int numExtensions = 0;
	size_t extensionsLen = strlen (extensionsString);

	char *truncatedExtensions;

	while ( (q = strchr (p, ' ')) != NULL && numExtensions <= maxExtensions )
	{
		p = q + 1;
		numExtensions++;
	}

	if ( q != NULL )
	{
		// We still have more extensions. We'll call this the end

		extensionsLen = p - extensionsString - 1;
	}

	truncatedExtensions = (char *)ri->Hunk_Alloc (extensionsLen + 1, h_low);
	Q_strncpyz (truncatedExtensions, extensionsString, extensionsLen + 1);

	return truncatedExtensions;
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
	cvar_t *lastValidRenderer = ri->Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	cvar_t	*cv;

//	Com_Printf ("Initializing OpenGL subsystem\n" );

	//
	// check OS version to see if we can do fullscreen display changes
	//
	if ( !GLW_CheckOSVersion() )
	{
		Com_Error( ERR_FATAL, "GLimp_Init() - incorrect operating system\n" );
	}

	// save off hInstance and wndproc
	cv = ri->Cvar_Get( "win_hinstance", "", 0 );
	sscanf( cv->string, "%p", (uintptr_t *)&tr.wv->hInstance );

	cv = ri->Cvar_Get( "win_wndproc", "", 0 );
	sscanf( cv->string, "%p", (uintptr_t *)&glw_state.wndproc );

	r_allowSoftwareGL = ri->Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

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

	glConfigExt.originalExtensionString = glConfig.extensions_string;
	glConfig.extensions_string = TruncateGLExtensionsString (glConfigExt.originalExtensionString, 128);

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
	Q_strncpyz( buf, glConfig.renderer_string, sizeof(buf) );
	strlwr( buf );

	//
	// NOTE: if changing cvars, do it within this block.  This allows them
	// to be overridden when testing driver fixes, etc. but only sets
	// them to their default state when the hardware is first installed/run.
	//
	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
	{
		if (ri->Sys_LowPhysicalMemory())
		{
			ri->Cvar_Set("s_khz", "11");// this will get called before S_Init
		}
		//reset to defaults
		ri->Cvar_Set( "r_picmip", "1" );

		// Savage3D and Savage4 should always have trilinear enabled
		if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) || strstr( buf, "geforce" ) || strstr( buf, "quadro" ) )
		{
			ri->Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		}
		else
		{
			ri->Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		}

		if ( strstr( buf, "kyro" ) )
		{
			ri->Cvar_Set( "r_ext_texture_filter_anisotropic", "0");	//KYROs have it avail, but suck at it!
			ri->Cvar_Set( "r_ext_preferred_tc_method", "1");			//(Use DXT1 instead of DXT5 - same quality but much better performance on KYRO)
		}
		if ( strstr( buf, "geforce2" ) )
		{
			ri->Cvar_Set( "cg_renderToTextureFX", "0");	// slow to zero bug fix
		}

		if ( strstr( buf, "radeon 9000" ) )
		{
			ri->Cvar_Set( "cg_renderToTextureFX", "0");	// white texture bug
		}

		GLW_InitExtensions();	//get the values for test below
		//this must be a really sucky card!
		if ( (glConfig.textureCompression == TC_NONE) || (glConfig.maxActiveTextures < 2)  || (glConfig.maxTextureSize <= 512) )
		{
			ri->Cvar_Set( "r_picmip", "2");
			ri->Cvar_Set( "r_colorbits", "16");
			ri->Cvar_Set( "r_texturebits", "16");
			ri->Cvar_Set( "r_mode", "3");	//force 640
			ri->Cmd_ExecuteString ("exec low.cfg\n");	//get the rest which can be pulled in after init
		}
	}

	ri->Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );
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
//	const char *success[] = { "failed", "success" };
	int retVal;

	// FIXME: Brian, we need better fallbacks from partially initialized failures
	if ( !qwglMakeCurrent ) {
		return;
	}

	Com_Printf ("Shutting down OpenGL subsystem\n" );

	// restore gamma.  We do this first because 3Dfx's extension needs a valid OGL subsystem
	WG_RestoreGamma();

	// set current context to NULL
	if ( qwglMakeCurrent )
	{
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;

//		Com_Printf ("...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if ( glw_state.hGLRC )
	{
		retVal = qwglDeleteContext( glw_state.hGLRC ) != 0;
//		Com_Printf ("...deleting GL context: %s\n", success[retVal] );
		glw_state.hGLRC = NULL;
	}

	// release DC
	if ( glw_state.hDC )
	{
		retVal = ReleaseDC( tr.wv->hWnd, glw_state.hDC ) != 0;
//		Com_Printf ("...releasing DC: %s\n", success[retVal] );
		glw_state.hDC   = NULL;
	}

	// destroy window
	if ( tr.wv->hWnd )
	{
//		Com_Printf ("...destroying window\n" );
		ShowWindow( tr.wv->hWnd, SW_HIDE );
		DestroyWindow( tr.wv->hWnd );
		tr.wv->hWnd = NULL;
		glw_state.pixelFormatSet = qfalse;
	}

	// unregister the window class
	if ( s_classRegistered )
	{
		if ( FAILED (UnregisterClass (WINDOW_CLASS_NAME, tr.wv->hInstance)) )
		{
			Com_Error (ERR_FATAL, "GLimp_Shutdown: could not unregister window class, error code 0x%x", GetLastError());
		}
		s_classRegistered = qfalse;
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
//		Com_Printf ("...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		glw_state.cdsFullscreen = qfalse;
	}

	// shutdown QGL subsystem
	QGL_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize(void)
{
	if ( tr.wv->hWnd )
	{
		// Todo with viewlog maybe should try to unminimize but mer.
		ShowWindow( tr.wv->hWnd, SW_MINIMIZE );
	}
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
