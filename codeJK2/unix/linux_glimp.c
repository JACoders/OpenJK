/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

//#include "../qcommon/qcommon.h"
//#include "../client/keys.h"
#include "../renderer/tr_local.h"
#include "../client/client.h"

#include "unix_glw.h"

#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>

typedef enum {
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

static Display *dpy = NULL;
static int scrnum;
static Window win = 0;
static GLXContext ctx = NULL;

static qboolean autorepeaton = qtrue;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask | ButtonMotionMask )
#define X_MASK (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | StructureNotifyMask )

static qboolean        mouse_avail;
static qboolean        mouse_active;
static int   mx, my;

static cvar_t	*in_mouse;
static cvar_t	*in_dgamouse;

static cvar_t	*r_fakeFullscreen;

qboolean dgamouse = qfalse;
qboolean vidmode_ext = qfalse;

static int win_x, win_y;

static XF86VidModeModeInfo **vidmodes;
static int default_dotclock_vidmode;
static int num_vidmodes;
static qboolean vidmode_active = qfalse;

static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;    

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/

static unsigned int	keyshift[256];		// key to map to if shift held down in console
static qboolean shift_down=qfalse;

static char *XLateKey(XKeyEvent *ev, int *key)
{
	static char buf[64];
	KeySym keysym;
	static qboolean setup = qfalse;
	int i;

	*key = 0;

	XLookupString(ev, buf, sizeof buf, &keysym, 0);

// ri.Printf( PRINT_ALL, "keysym=%04X\n", (int)keysym);
	switch(keysym)
	{
		case XK_KP_Page_Up:	
		case XK_KP_9:	 *key = K_KP_PGUP; break;
		case XK_Page_Up:	 *key = K_PGUP; break;

		case XK_KP_Page_Down: 
		case XK_KP_3: *key = K_KP_PGDN; break;
		case XK_Page_Down:	 *key = K_PGDN; break;

		case XK_KP_Home: *key = K_KP_HOME; break;
		case XK_KP_7: *key = K_KP_HOME; break;
		case XK_Home:	 *key = K_HOME; break;

		case XK_KP_End:
		case XK_KP_1:	  *key = K_KP_END; break;
		case XK_End:	 *key = K_END; break;

		case XK_KP_Left: *key = K_KP_LEFTARROW; break;
		case XK_KP_4: *key = K_KP_LEFTARROW; break;
		case XK_Left:	 *key = K_LEFTARROW; break;

		case XK_KP_Right: *key = K_KP_RIGHTARROW; break;
		case XK_KP_6: *key = K_KP_RIGHTARROW; break;
		case XK_Right:	*key = K_RIGHTARROW;		break;

		case XK_KP_Down:
		case XK_KP_2: 	 *key = K_KP_DOWNARROW; break;
		case XK_Down:	 *key = K_DOWNARROW; break;

		case XK_KP_Up:   
		case XK_KP_8:    *key = K_KP_UPARROW; break;
		case XK_Up:		 *key = K_UPARROW;	 break;

		case XK_Escape: *key = K_ESCAPE;		break;

		case XK_KP_Enter: *key = K_KP_ENTER;	break;
		case XK_Return: *key = K_ENTER;		 break;

		case XK_Tab:		*key = K_TAB;			 break;

		case XK_F1:		 *key = K_F1;				break;

		case XK_F2:		 *key = K_F2;				break;

		case XK_F3:		 *key = K_F3;				break;

		case XK_F4:		 *key = K_F4;				break;

		case XK_F5:		 *key = K_F5;				break;

		case XK_F6:		 *key = K_F6;				break;

		case XK_F7:		 *key = K_F7;				break;

		case XK_F8:		 *key = K_F8;				break;

		case XK_F9:		 *key = K_F9;				break;

		case XK_F10:		*key = K_F10;			 break;

		case XK_F11:		*key = K_F11;			 break;

		case XK_F12:		*key = K_F12;			 break;

//		case XK_BackSpace: *key = K_BACKSPACE; break;
		case XK_BackSpace: *key = 8; break; // ctrl-h

		case XK_KP_Delete:
		case XK_KP_Decimal: *key = K_KP_DEL; break;
		case XK_Delete: *key = K_DEL; break;

		case XK_Pause:	*key = K_PAUSE;		 break;

		case XK_Shift_L:
		case XK_Shift_R:	*key = K_SHIFT;		break;

		case XK_Execute: 
		case XK_Control_L: 
		case XK_Control_R:	*key = K_CTRL;		 break;

		case XK_Alt_L:	
		case XK_Meta_L: 
		case XK_Alt_R:	
		case XK_Meta_R: *key = K_ALT;			break;

		case XK_KP_Begin: *key = K_KP_5;	break;

		case XK_Insert:		*key = K_INS; break;
		case XK_KP_Insert:
		case XK_KP_0: *key = K_KP_INS; break;

		case XK_KP_Multiply: *key = '*'; break;
		case XK_KP_Add:  *key = K_KP_PLUS; break;
		case XK_KP_Subtract: *key = K_KP_MINUS; break;
		case XK_KP_Divide: *key = K_KP_SLASH; break;

		default:
			*key = *(unsigned char *)buf;
			if (*key >= 'A' && *key <= 'Z')
				*key = *key - 'A' + 'a';
			break;
	} 

	return buf;
}

// ========================================================================
// makes a null cursor
// ========================================================================

static Cursor CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1/*depth*/);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
          &dummycolour,&dummycolour, 0,0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

static void install_grabs(void)
{
// inviso cursor
	XDefineCursor(dpy, win, CreateNullCursor(dpy, win));

	XGrabPointer(dpy, win,
				 False,
				 MOUSE_MASK,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime);

	XGetPointerControl(dpy, &mouse_accel_numerator, &mouse_accel_denominator,
		&mouse_threshold);

	XChangePointerControl(dpy, qtrue, qtrue, 2, 1, 0);

	if (in_dgamouse->value) {
		int MajorVersion, MinorVersion;

		if (!XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
			// unable to query, probalby not supported
			ri.Printf( PRINT_ALL, "Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		} else {
			dgamouse = qtrue;
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		}
	} else {
		XWarpPointer(dpy, None, win,
					 0, 0, 0, 0,
					 glConfig.vidWidth / 2, glConfig.vidHeight / 2);
	}

	XGrabKeyboard(dpy, win,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);

//	XSync(dpy, True);
}

static void uninstall_grabs(void)
{
	if (dgamouse) {
		dgamouse = qfalse;
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}

	XChangePointerControl(dpy, qtrue, qtrue, mouse_accel_numerator, 
		mouse_accel_denominator, mouse_threshold);

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

// inviso cursor
	XUndefineCursor(dpy, win);

//	XAutoRepeatOn(dpy);

//	XSync(dpy, True);
}

static void HandleEvents(void)
{
	int b;
	int key;
	XEvent event;
	qboolean dowarp = qfalse;
	int mwx = glConfig.vidWidth/2;
	int mwy = glConfig.vidHeight/2;
	char *p;
   
	if (!dpy)
		return;

	while (XPending(dpy)) {
		XNextEvent(dpy, &event);
		switch(event.type) {
		case KeyPress:
			p = XLateKey(&event.xkey, &key);
			if (key)
				Sys_QueEvent( 0, SE_KEY, key, qtrue, 0, NULL );
			while (*p)
				Sys_QueEvent( 0, SE_CHAR, *p++, 0, 0, NULL );
			break;
		case KeyRelease:
			XLateKey(&event.xkey, &key);
			
			Sys_QueEvent( 0, SE_KEY, key, qfalse, 0, NULL );
			break;

#if 0
		case KeyPress:
		case KeyRelease:
			key = XLateKey(&event.xkey);
			
			Sys_QueEvent( 0, SE_KEY, key, event.type == KeyPress, 0, NULL );
			if (key == K_SHIFT)
				shift_down = (event.type == KeyPress);
			if (key < 128 && (event.type == KeyPress)) {
				if (shift_down)
					key = keyshift[key];
				Sys_QueEvent( 0, SE_CHAR, key, 0, 0, NULL );
			}
#endif
			break;

		case MotionNotify:
			if (mouse_active) {
				if (dgamouse) {
					if (abs(event.xmotion.x_root) > 1)
						mx += event.xmotion.x_root * 2;
					else
						mx += event.xmotion.x_root;
					if (abs(event.xmotion.y_root) > 1)
						my += event.xmotion.y_root * 2;
					else
						my += event.xmotion.y_root;
//					ri.Printf(PRINT_ALL, "mouse (%d,%d) (root=%d,%d)\n", event.xmotion.x + win_x, event.xmotion.y + win_y, event.xmotion.x_root, event.xmotion.y_root);
				} 
				else 
				{
//					ri.Printf(PRINT_ALL, "mouse x=%d,y=%d\n", (int)event.xmotion.x - mwx, (int)event.xmotion.y - mwy);
					mx += ((int)event.xmotion.x - mwx);
					my += ((int)event.xmotion.y - mwy);
					mwx = event.xmotion.x;
					mwy = event.xmotion.y;

					if (mx || my)
						dowarp = qtrue;
				}
			}
			break;

		case ButtonPress:
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			Sys_QueEvent( 0, SE_KEY, K_MOUSE1 + b, qtrue, 0, NULL );
			break;

		case ButtonRelease:
			b=-1;
			if (event.xbutton.button == 1)
				b = 0;
			else if (event.xbutton.button == 2)
				b = 2;
			else if (event.xbutton.button == 3)
				b = 1;
			Sys_QueEvent( 0, SE_KEY, K_MOUSE1 + b, qfalse, 0, NULL );
			break;

		case CreateNotify :
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;

		case ConfigureNotify :
			win_x = event.xconfigure.x;
			win_y = event.xconfigure.y;
			break;
		}
	}

	if (dowarp) {
		/* move the mouse to the window center again */
		XWarpPointer(dpy,None,win,0,0,0,0, 
				(glConfig.vidWidth/2),(glConfig.vidHeight/2));
	}

}

void KBD_Init(void)
{
}

void KBD_Close(void)
{
}

void IN_ActivateMouse( void ) 
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		install_grabs();
		mouse_active = qtrue;
	}
}

void IN_DeactivateMouse( void ) 
{
	if (!mouse_avail || !dpy || !win)
		return;

	if (mouse_active) {
		uninstall_grabs();
		mouse_active = qfalse;
	}
}
/*****************************************************************************/

static qboolean signalcaught = qfalse;;

static void signal_handler(int sig)
{
	if (signalcaught) {
		printf("DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n", sig);
		_exit(1);
	}

	signalcaught = qtrue;
	printf("Received signal %d, exiting...\n", sig);
	GLimp_Shutdown();
	_exit(1);
}

static void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{
	if (!ctx || !dpy)
		return;
	IN_DeactivateMouse();
	XAutoRepeatOn(dpy);
	if (dpy) {
		if (ctx)
			qglXDestroyContext(dpy, ctx);
		if (win)
			XDestroyWindow(dpy, win);
		if (vidmode_active)
			XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[0]);
		XCloseDisplay(dpy);
	}
	vidmode_active = qfalse;
	dpy = NULL;
	win = 0;
	ctx = NULL;

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );

	QGL_Shutdown();
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
** GLW_StartDriverAndSetMode
*/
static qboolean GLW_StartDriverAndSetMode( const char *drivername, 
										   int mode, 
										   qboolean fullscreen )
{
	rserr_t err;

	// don't ever bother going into fullscreen with a voodoo card
#if 1	// JDC: I reenabled this
	if ( strstr( drivername, "Voodoo" ) ) {
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}
#endif

	err = GLW_SetMode( drivername, mode, fullscreen );

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
** GLW_SetMode
*/
int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen )
{
	int attrib[] = {
		GLX_RGBA,					// 0
		GLX_RED_SIZE, 4,			// 1, 2
		GLX_GREEN_SIZE, 4,			// 3, 4
		GLX_BLUE_SIZE, 4,			// 5, 6
		GLX_DOUBLEBUFFER,			// 7
		GLX_DEPTH_SIZE, 1,			// 8, 9
		GLX_STENCIL_SIZE, 1,		// 10, 11
		None
	};
// these match in the array
#define ATTR_RED_IDX 2
#define ATTR_GREEN_IDX 4
#define ATTR_BLUE_IDX 6
#define ATTR_DEPTH_IDX 9
#define ATTR_STENCIL_IDX 11
	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes attr;
	unsigned long mask;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int MajorVersion, MinorVersion;
	int actualWidth, actualHeight;
	int i;

	r_fakeFullscreen = ri.Cvar_Get( "r_fakeFullscreen", "0", CVAR_ARCHIVE);

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

	ri.Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Error couldn't open the X display\n");
		return RSERR_INVALID_MODE;
	}

	scrnum = DefaultScreen(dpy);
	root = RootWindow(dpy, scrnum);

	actualWidth = glConfig.vidWidth;
	actualHeight = glConfig.vidHeight;

	// Get video mode list
	MajorVersion = MinorVersion = 0;
	if (!XF86VidModeQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
		vidmode_ext = qfalse;
	} else {
		ri.Printf(PRINT_ALL, "Using XFree86-VidModeExtension Version %d.%d\n",
			MajorVersion, MinorVersion);
		vidmode_ext = qtrue;
	}

	if (vidmode_ext) {
		int best_fit, best_dist, dist, x, y;
		
		XF86VidModeGetAllModeLines(dpy, scrnum, &num_vidmodes, &vidmodes);

		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen && !r_fakeFullscreen->integer) {
			best_dist = 9999999;
			best_fit = -1;

			for (i = 0; i < num_vidmodes; i++) {
				if (glConfig.vidWidth > vidmodes[i]->hdisplay ||
					glConfig.vidHeight > vidmodes[i]->vdisplay)
					continue;

				x = glConfig.vidWidth - vidmodes[i]->hdisplay;
				y = glConfig.vidHeight - vidmodes[i]->vdisplay;
				dist = (x * x) + (y * y);
				if (dist < best_dist) {
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1) {
				actualWidth = vidmodes[best_fit]->hdisplay;
				actualHeight = vidmodes[best_fit]->vdisplay;

				// change to the mode
				XF86VidModeSwitchToMode(dpy, scrnum, vidmodes[best_fit]);
				vidmode_active = qtrue;

				// Move the viewport to top left
				XF86VidModeSetViewPort(dpy, scrnum, 0, 0);
			} else
				fullscreen = 0;
		}
	}


	if (!r_colorbits->value)
		colorbits = 24;
	else
		colorbits = r_colorbits->value;

	if ( !Q_stricmp( r_glDriver->string, _3DFX_DRIVER_NAME ) )
		colorbits = 16;

	if (!r_depthbits->value)
		depthbits = 24;
	else
		depthbits = r_depthbits->value;
	stencilbits = r_stencilbits->value;

	for (i = 0; i < 16; i++) {
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i) {
			// one pass, reduce
			switch (i / 4) {
			case 2 :
				if (colorbits == 24)
					colorbits = 16;
				break;
			case 1 :
				if (depthbits == 24)
					depthbits = 16;
				else if (depthbits == 16)
					depthbits = 8;
			case 3 :
				if (stencilbits == 24)
					stencilbits = 16;
				else if (stencilbits == 16)
					stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ((i % 4) == 3) { // reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}	

		if ((i % 4) == 2) { // reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1) { // reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		if (tcolorbits == 24) {
			attrib[ATTR_RED_IDX] = 8;
			attrib[ATTR_GREEN_IDX] = 8;
			attrib[ATTR_BLUE_IDX] = 8;
		} else  {
			// must be 16 bit
			attrib[ATTR_RED_IDX] = 4;
			attrib[ATTR_GREEN_IDX] = 4;
			attrib[ATTR_BLUE_IDX] = 4;
		}

		attrib[ATTR_DEPTH_IDX] = tdepthbits; // default to 24 depth
		attrib[ATTR_STENCIL_IDX] = tstencilbits;

#if 0
		ri.Printf( PRINT_DEVELOPER, "Attempting %d/%d/%d Color bits, %d depth, %d stencil display...", 
			attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
			attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);
#endif

		visinfo = qglXChooseVisual(dpy, scrnum, attrib);
		if (!visinfo) {
#if 0
			ri.Printf( PRINT_DEVELOPER, "failed\n");
#endif
			continue;
		}

#if 0
		ri.Printf( PRINT_DEVELOPER, "Successful\n");
#endif

		ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n", 
			attrib[ATTR_RED_IDX], attrib[ATTR_GREEN_IDX], attrib[ATTR_BLUE_IDX],
			attrib[ATTR_DEPTH_IDX], attrib[ATTR_STENCIL_IDX]);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if (!visinfo) {
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	/* window attributes */
	attr.background_pixel = BlackPixel(dpy, scrnum);
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = X_MASK;
	if (vidmode_active) {
		mask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore | 
			CWEventMask | CWOverrideRedirect;
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
	} else
		mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow(dpy, root, 0, 0, 
			actualWidth, actualHeight, 
			0, visinfo->depth, InputOutput,
			visinfo->visual, mask, &attr);
	XMapWindow(dpy, win);

	if (vidmode_active)
		XMoveWindow(dpy, win, 0, 0);

	// Check for DGA
	if (in_dgamouse->value) {
		if (!XF86DGAQueryVersion(dpy, &MajorVersion, &MinorVersion)) { 
			// unable to query, probalby not supported
			ri.Printf( PRINT_ALL, "Failed to detect XF86DGA Mouse\n" );
			ri.Cvar_Set( "in_dgamouse", "0" );
		} else
			ri.Printf( PRINT_ALL, "XF86DGA Mouse (Version %d.%d) initialized\n",
				MajorVersion, MinorVersion);
	}

	XFlush(dpy);

	ctx = qglXCreateContext(dpy, visinfo, NULL, True);

	qglXMakeCurrent(dpy, win, ctx);

	return RSERR_OK;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
	// Use modern texture compression extensions
	if ( strstr( glConfig.extensions_string, "ARB_texture_compression" ) && strstr( glConfig.extensions_string, "EXT_texture_compression_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC_DXT;
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
		}
	}
	// Or check for old ones
	else if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
	{
		if ( r_ext_compressed_textures->value )
		{
			glConfig.textureCompression = TC_S3TC;
			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
		}
		else
		{
			glConfig.textureCompression = TC_NONE;
			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
		}
	}
	else
	{
		glConfig.textureCompression = TC_NONE;
		ri.Printf( PRINT_ALL, "...no texture compression found\n" );
	}

#if 0
	// WGL_EXT_swap_control
	if ( strstr( glConfig.extensions_string, "WGL_EXT_swap_control" ) )
	{
		qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}
#endif

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" ) )
	{
		if ( r_ext_multitexture->value )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) dlsym( glw_state.OpenGLLib, "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
			}
			else
			{
				ri.Printf( PRINT_ALL, "...blind search for ARB_multitexture failed\n" );
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

	// GL_EXT_compiled_vertex_array
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->value )
		{
			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) dlsym( glw_state.OpenGLLib, "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) dlsym( glw_state.OpenGLLib, "glUnlockArraysEXT" );
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

}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use 
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL( const char *name )
{
	qboolean fullscreen;

	ri.Printf( PRINT_ALL, "...loading %s: ", name );

	// disable the 3Dfx splash screen and set gamma
	// we do this all the time, but it shouldn't hurt anything
	// on non-3Dfx stuff
	putenv("FX_GLIDE_NO_SPLASH=0");

	// Mesa VooDoo hacks
	putenv("MESA_GLX_FX=fullscreen\n");

	// load the QGL layer
	if ( QGL_Init( name ) ) 
	{
		fullscreen = r_fullscreen->integer;

		// create the window and set up the context
		if ( !GLW_StartDriverAndSetMode( name, r_mode->integer, fullscreen ) )
		{
			if (r_mode->integer != 3) {
				if ( !GLW_StartDriverAndSetMode( name, 3, fullscreen ) ) {
					goto fail;
				}
			} else
				goto fail;
		}

		return qtrue;
	}
	else
	{
		ri.Printf( PRINT_ALL, "failed\n" );
	}
fail:

	QGL_Shutdown();

	return qfalse;
}

/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
void GLimp_Init( void )
{
	qboolean attemptedlibGL = qfalse;
	qboolean attempted3Dfx = qfalse;
	qboolean success = qfalse;
	char	buf[1024];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	cvar_t	*cv;

	glConfig.deviceSupportsGamma = qfalse;

	InitSig();

	//
	// load and initialize the specific OpenGL driver
	//
	if ( !GLW_LoadOpenGL( r_glDriver->string ) )
	{
		if ( !Q_stricmp( r_glDriver->string, OPENGL_DRIVER_NAME ) )
		{
			attemptedlibGL = qtrue;
		}
		else if ( !Q_stricmp( r_glDriver->string, _3DFX_DRIVER_NAME ) )
		{
			attempted3Dfx = qtrue;
		}

		if ( !attempted3Dfx && !success )
		{
			attempted3Dfx = qtrue;
			if ( GLW_LoadOpenGL( _3DFX_DRIVER_NAME ) )
			{
				ri.Cvar_Set( "r_glDriver", _3DFX_DRIVER_NAME );
				r_glDriver->modified = qfalse;
				success = qtrue;
			}
		}

		// try ICD before trying 3Dfx standalone driver
		if ( !attemptedlibGL && !success )
		{
			attemptedlibGL = qtrue;
			if ( GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) )
			{
				ri.Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
				r_glDriver->modified = qfalse;
				success = qtrue;
			}
		} 
		
		if (!success)
			ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );

	}

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	//
	// chipset specific configuration
	//
	strcpy( buf, glConfig.renderer_string );
	strlwr( buf );

	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
	{
		ri.Cvar_Set( "r_picmip", "1" );
		ri.Cvar_Set( "r_twopartfog", "0" );
		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

		//
		// voodoo issues
		//
		if ( strstr( buf, "voodoo" ) && !strstr( buf, "banshee" ) )
		{
			ri.Cvar_Set( "r_fakeFullscreen", "1");
		}

		//
		// Riva128 issues
		//
		if ( strstr( buf, "riva 128" ) )
		{
			ri.Cvar_Set( "r_twopartfog", "1" );
		}

		//
		// Rage Pro issues
		//
		if ( strstr( buf, "rage pro" ) )
		{
			ri.Cvar_Set( "r_mode", "2" );
			ri.Cvar_Set( "r_twopartfog", "1" );
		}

		//
		// Permedia2 issues
		//
		if ( strstr( buf, "permedia2" ) )
		{
			ri.Cvar_Set( "r_vertexLight", "1" );
		}

		//
		// Riva TNT issues
		//
		if ( strstr( buf, "riva tnt " ) )
		{
			if ( r_texturebits->integer == 32 ||
				 ( ( r_texturebits->integer == 0 ) && glConfig.colorBits > 16 ) )
			{
				ri.Cvar_Set( "r_picmip", "1" );
			}
		}

		ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );
	}

	// initialize extensions
	GLW_InitExtensions();

	InitSig();

	return;
}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
#if 0
	int	err;

	if ( !glState.finishCalled )
		qglFinish();

	// check for errors
	if ( !gl_ignore_errors->value ) {
		if ( ( err = qglGetError() ) != GL_NO_ERROR )
		{
			ri.Error( ERR_FATAL, "GLimp_EndFrame() - glGetError() failed (0x%x)!\n", err );
		}
	}
#endif

	// don't flip if drawing to front buffer
	if ( stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		qglXSwapBuffers(dpy, win);
	}

	// check logging
	QGL_EnableLogging( r_logFile->value );

#if 0
	GLimp_LogComment( "*** RE_EndFrame ***\n" );

	// decrement log
	if ( gl_log->value )
	{
		ri.Cvar_Set( "gl_log", va("%i",gl_log->value - 1 ) );
	}
#endif
}

/*
===========================================================

SMP acceleration

===========================================================
*/

sem_t	renderCommandsEvent;
sem_t	renderCompletedEvent;
sem_t	renderActiveEvent;

void (*glimpRenderThread)( void );

void GLimp_RenderThreadWrapper( void *stub ) {
	glimpRenderThread();

#if 0
	// unbind the context before we die
	qglXMakeCurrent(dpy, None, NULL);
#endif
}


/*
=======================
GLimp_SpawnRenderThread
=======================
*/
pthread_t	renderThreadHandle;
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {

	sem_init( &renderCommandsEvent, 0, 0 );
	sem_init( &renderCompletedEvent, 0, 0 );
	sem_init( &renderActiveEvent, 0, 0 );

	glimpRenderThread = function;

	if (pthread_create( &renderThreadHandle, NULL,
		GLimp_RenderThreadWrapper, NULL)) {
		return qfalse;
	}

	return qtrue;
}

static	void	*smpData;
static	int		glXErrors;

void *GLimp_RendererSleep( void ) {
	void	*data;

#if 0
	if ( !qglXMakeCurrent(dpy, None, NULL) ) {
		glXErrors++;
	}
#endif

//	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	sem_post ( &renderCompletedEvent );

	sem_wait ( &renderCommandsEvent );

#if 0
	if ( !qglXMakeCurrent(dpy, win, ctx) ) {
		glXErrors++;
	}
#endif

//	ResetEvent( renderCompletedEvent );
//	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	sem_post ( &renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	sem_wait ( &renderCompletedEvent );

#if 0
	if ( !qglXMakeCurrent(dpy, win, ctx) ) {
		glXErrors++;
	}
#endif
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

#if 0
	if ( !qglXMakeCurrent(dpy, None, NULL) ) {
		glXErrors++;
	}
#endif

	// after this, the renderer can continue through GLimp_RendererSleep
	sem_post( &renderCommandsEvent );

	sem_wait( &renderActiveEvent );
}

/*===========================================================*/

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

void IN_Init(void)
{
	// mouse variables
    in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    in_dgamouse = Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);

	if (in_mouse->value)
		mouse_avail = qtrue;
	else
		mouse_avail = qfalse;
}

void IN_Shutdown(void)
{
	mouse_avail = qfalse;
}

void IN_MouseMove(void)
{
	if (!mouse_avail || !dpy || !win)
		return;

#if 0
	if (!dgamouse) {
		Window root, child;
		int root_x, root_y;
		int win_x, win_y;
		unsigned int mask_return;
		int mwx = glConfig.vidWidth/2;
		int mwy = glConfig.vidHeight/2;

		XQueryPointer(dpy, win, &root, &child, 
			&root_x, &root_y, &win_x, &win_y, &mask_return);

		mx = win_x - mwx;
		my = win_y - mwy;

		XWarpPointer(dpy,None,win,0,0,0,0, mwx, mwy);
	}
#endif

	if (mx || my)
		Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL );
	mx = my = 0;
}

void IN_Frame (void)
{
	if ( cls.keyCatchers || cls.state != CA_ACTIVE ) {
		// temporarily deactivate if not in the game and
		// running on the desktop
		// voodoo always counts as full screen
		if (Cvar_VariableValue ("r_fullscreen") == 0
			&& strcmp( Cvar_VariableString("r_glDriver"), _3DFX_DRIVER_NAME ) )	{
			IN_DeactivateMouse ();
			return;
		}
		if (dpy && !autorepeaton) {
			XAutoRepeatOn(dpy);
			autorepeaton = qtrue;
		}
	} else if (dpy && autorepeaton) {
		XAutoRepeatOff(dpy);
		autorepeaton = qfalse;
	}

	IN_ActivateMouse();

	// post events to the system que
	IN_MouseMove();
}

void IN_Activate(void)
{
}

void Sys_SendKeyEvents (void)
{
	XEvent event;

	if (!dpy)
		return;

	HandleEvents();
//	while (XCheckMaskEvent(dpy,KEY_MASK|MOUSE_MASK,&event))
//		HandleEvent(&event);
}

