/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// win_input.c -- win32 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"




#include "../client/client.h"
#include "win_local.h"

#ifndef NO_XINPUT
#include <Xinput.h>
#endif

typedef struct {
	int			oldButtonState;

	qboolean	mouseActive;
	qboolean	mouseInitialized;
} WinMouseVars_t;

static WinMouseVars_t s_wmv;

static int	window_center_x, window_center_y;

//
// MIDI definitions
//
static void IN_StartupMIDI( void );
static void IN_ShutdownMIDI( void );
#ifndef NO_XINPUT
void IN_UnloadXInput( void );
#endif

#define MAX_MIDIIN_DEVICES	8

typedef struct {
	int			numDevices;
	MIDIINCAPS	caps[MAX_MIDIIN_DEVICES];

	HMIDIIN		hMidiIn;
} MidiInfo_t;

static MidiInfo_t s_midiInfo;

//
// Joystick definitions
//
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V

typedef struct {
	qboolean	avail;
	int			id;			// joystick number
	JOYCAPS		jc;

	int			oldbuttonstate;
	int			oldpovstate;

	JOYINFOEX	ji;
} joystickInfo_t;

static	joystickInfo_t	joy;


cvar_t	*in_midi;
cvar_t	*in_midiport;
cvar_t	*in_midichannel;
cvar_t	*in_mididevice;

cvar_t	*in_mouse;
cvar_t	*in_joystick;
cvar_t	*in_joyBallScale;
cvar_t	*in_debugJoystick;
cvar_t	*joy_threshold;
cvar_t	*joy_xbutton;
cvar_t	*joy_ybutton;

#ifndef NO_XINPUT
cvar_t	*xin_invertThumbsticks;
cvar_t	*xin_rumbleScale;

cvar_t	*xin_invertLookX;
cvar_t	*xin_invertLookY;
#endif

qboolean	in_appactive;

// forward-referenced functions
void IN_StartupJoystick (void);
void IN_JoyMove(void);

static void MidiInfo_f( void );

/*
============================================================

RAW INPUT MOUSE CONTROL

============================================================
*/

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC	((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE	((USHORT) 0x02)
#endif

static qboolean rawMouseInitialized = qfalse;
static LONG rawDeltaX = 0;
static LONG rawDeltaY = 0;

/*
================
IN_InitRawMouse
================
*/
qboolean IN_InitRawMouse( void )
{
	RAWINPUTDEVICE Rid[1];

	Com_Printf( "Initializing raw input...\n");

	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = 0;
	Rid[0].hwndTarget = 0;

	if ( RegisterRawInputDevices( Rid, 1, sizeof(Rid[0]) ) == FALSE )
	{
		Com_Printf ("Couldn't register raw input devices\n");
		return qfalse;
	}

	Com_Printf( "Raw input initialized.\n");
	rawMouseInitialized = qtrue;
	return qtrue;
}

/*
================
IN_ShutdownRawMouse
================
*/
void IN_ShutdownRawMouse( void )
{
	if ( rawMouseInitialized )
	{
		RAWINPUTDEVICE Rid[1];

		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		Rid[0].dwFlags = RIDEV_REMOVE;
		Rid[0].hwndTarget = 0;

		if ( RegisterRawInputDevices( Rid, 1, sizeof(Rid[0]) ) == FALSE )
		{
			Com_Printf ("Couldn't un-register raw input devices\n");
		}

		rawMouseInitialized = qfalse;
	}
}

/*
================
IN_ActivateRawMouse
================
*/
void IN_ActivateRawMouse( void )
{
	rawDeltaX = rawDeltaY = 0;
}

/*
================
IN_DeactivateRawMouse
================
*/
void IN_DeactivateRawMouse( void )
{
	rawDeltaX = rawDeltaY = 0;
}

/*
================
IN_RawMouse
================
*/
void IN_RawMouse( int *mx, int *my )
{
	// force the mouse to the center, just to be consistent with default mouse behaviour
	SetCursorPos (window_center_x, window_center_y);

	*mx = rawDeltaX;
	*my = rawDeltaY;
	rawDeltaX = rawDeltaY = 0;
}

/*
================
IN_RawMouseEvent
================
*/
void IN_RawMouseEvent( int lastX, int lastY )
{
	rawDeltaX += lastX;
	rawDeltaY += lastY;
}


/*
============================================================

WIN32 MOUSE CONTROL

============================================================
*/

/*
================
IN_InitWin32Mouse
================
*/
void IN_InitWin32Mouse( void ) 
{
}

/*
================
IN_ShutdownWin32Mouse
================
*/
void IN_ShutdownWin32Mouse( void ) {
}

/*
================
IN_ActivateWin32Mouse
================
*/
void IN_ActivateWin32Mouse( void ) {
	int			x, y, width, height;
	RECT		window_rect;

	x = GetSystemMetrics (SM_XVIRTUALSCREEN);
	y = GetSystemMetrics (SM_YVIRTUALSCREEN);
	width = GetSystemMetrics (SM_CXVIRTUALSCREEN);
	height = GetSystemMetrics (SM_CYVIRTUALSCREEN);

	GetWindowRect ( g_wv.hWnd, &window_rect);
	if (window_rect.left < x)
		window_rect.left = x;
	if (window_rect.top < y)
		window_rect.top = y;
	if (window_rect.right >= width)
		window_rect.right = width-1;
	if (window_rect.bottom >= height-1)
		window_rect.bottom = height-1;
	window_center_x = (window_rect.right + window_rect.left)/2;
	window_center_y = (window_rect.top + window_rect.bottom)/2;

	SetCursorPos (window_center_x, window_center_y);

	SetCapture ( g_wv.hWnd );
	ClipCursor (&window_rect);
	while (ShowCursor (FALSE) >= 0)
		;
}

/*
================
IN_DeactivateWin32Mouse
================
*/
void IN_DeactivateWin32Mouse( void ) 
{
	ClipCursor (NULL);
	ReleaseCapture ();
	while (ShowCursor (TRUE) < 0)
		;
}

/*
================
IN_Win32Mouse
================
*/
void IN_Win32Mouse( int *mx, int *my ) {
	POINT		current_pos;

	// find mouse movement
	GetCursorPos (&current_pos);

	// force the mouse to the center, so there's room to move
	SetCursorPos (window_center_x, window_center_y);

	*mx = current_pos.x - window_center_x;
	*my = current_pos.y - window_center_y;
}


/*
============================================================

DIRECT INPUT MOUSE CONTROL

============================================================
*/

#undef DEFINE_GUID

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID(qGUID_SysMouse,   0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(qGUID_XAxis,   0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(qGUID_YAxis,   0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(qGUID_ZAxis,   0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);


#define DINPUT_BUFFERSIZE           16
#define iDirectInputCreate(a,b,c,d)	pDirectInputCreate(a,b,c,d)

HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion,
	LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);

static HINSTANCE hInstDI;

typedef struct MYDATA {
	LONG  lX;                   // X axis goes here
	LONG  lY;                   // Y axis goes here
	LONG  lZ;                   // Z axis goes here
	BYTE  bButtonA;             // One button goes here
	BYTE  bButtonB;             // Another button goes here
	BYTE  bButtonC;             // Another button goes here
	BYTE  bButtonD;             // Another button goes here
} MYDATA;

static DIOBJECTDATAFORMAT rgodf[] = {
  { &qGUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &qGUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &qGUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
};

#define NUM_OBJECTS (sizeof(rgodf) / sizeof(rgodf[0]))

static DIDATAFORMAT	df = {
	sizeof(DIDATAFORMAT),       // this structure
	sizeof(DIOBJECTDATAFORMAT), // size of object data format
	DIDF_RELAXIS,               // absolute axis coordinates
	sizeof(MYDATA),             // device data size
	NUM_OBJECTS,                // number of objects
	rgodf,                      // and here they are
};

static LPDIRECTINPUT		g_pdi;
static LPDIRECTINPUTDEVICE	g_pMouse;

void IN_DIMouse( int *mx, int *my );

/*
========================
IN_InitDIMouse
========================
*/
qboolean IN_InitDIMouse( void ) {
    HRESULT		hr;
	int			x, y;
	DIPROPDWORD	dipdw = {
		{
			sizeof(DIPROPDWORD),        // diph.dwSize
			sizeof(DIPROPHEADER),       // diph.dwHeaderSize
			0,                          // diph.dwObj
			DIPH_DEVICE,                // diph.dwHow
		},
		DINPUT_BUFFERSIZE,              // dwData
	};

	Com_Printf( "Initializing DirectInput...\n");

	if (!hInstDI) {
		hInstDI = LoadLibrary("dinput.dll");
		
		if (hInstDI == NULL) {
			Com_Printf ("Couldn't load dinput.dll\n");
			return qfalse;
		}
	}

	if (!pDirectInputCreate) {
		pDirectInputCreate = (long (__stdcall *)(struct HINSTANCE__ *,unsigned long,struct IDirectInputA ** ,struct IUnknown *))
			GetProcAddress(hInstDI,"DirectInputCreateA");

		if (!pDirectInputCreate) {
			Com_Printf ("Couldn't get DI proc addr\n");
			return qfalse;
		}
	}

	// register with DirectInput and get an IDirectInput to play with.
	hr = iDirectInputCreate( g_wv.hInstance, DIRECTINPUT_VERSION, &g_pdi, NULL);

	if (FAILED(hr)) {
		Com_Printf ("iDirectInputCreate failed\n");
		return qfalse;
	}

	// obtain an interface to the system mouse device.
	hr = g_pdi->CreateDevice( qGUID_SysMouse, &g_pMouse, NULL);

	if (FAILED(hr)) {
		Com_Printf ("Couldn't open DI mouse device\n");
		return qfalse;
	}

	// set the data format to "mouse format".
	hr = IDirectInputDevice_SetDataFormat(g_pMouse, &df);

	if (FAILED(hr)) 	{
		Com_Printf ("Couldn't set DI mouse format\n");
		return qfalse;
	}

	// set the cooperativity level.
	hr = IDirectInputDevice_SetCooperativeLevel(g_pMouse, g_wv.hWnd,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(hr)) {
		Com_Printf ("Couldn't set DI coop level\n");
		return qfalse;
	}


	// set the buffer size to DINPUT_BUFFERSIZE elements.
	// the buffer size is a DWORD property associated with the device
	hr = IDirectInputDevice_SetProperty(g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr)) {
		Com_Printf ("Couldn't set DI buffersize\n");
		return qfalse;
	}

	// clear any pending samples
	IN_DIMouse( &x, &y );
	IN_DIMouse( &x, &y );

	Com_Printf( "DirectInput initialized.\n");
	return qtrue;
}

/*
==========================
IN_ShutdownDIMouse
==========================
*/
void IN_ShutdownDIMouse( void ) 
{
    if (g_pMouse) 
	{
		IDirectInputDevice_Release(g_pMouse);
		g_pMouse = NULL;
	}
	if (g_pdi) 
	{
		IDirectInput_Release(g_pdi);
		g_pdi = NULL;
	}
	if(hInstDI)
	{
		FreeLibrary(hInstDI);
		hInstDI = NULL;
	}
}

/*
==========================
IN_ActivateDIMouse
==========================
*/
void IN_ActivateDIMouse( void ) {
	HRESULT		hr;

	if (!g_pMouse) {
		return;
	}

	// we may fail to reacquire if the window has been recreated
	hr = IDirectInputDevice_Acquire( g_pMouse );
	if (FAILED(hr)) {
		if ( !IN_InitDIMouse() ) {
			Com_Printf ("Falling back to Win32 mouse support...\n");
			Cvar_Set( "in_mouse", "-1" );
		}
	}
}

/*
==========================
IN_DeactivateDIMouse
==========================
*/
void IN_DeactivateDIMouse( void ) {
	if (!g_pMouse) {
		return;
	}
	IDirectInputDevice_Unacquire( g_pMouse );
}


/*
===================
IN_DIMouse
===================
*/
void IN_DIMouse( int *mx, int *my ) {
	DIDEVICEOBJECTDATA	od;
	DIMOUSESTATE		state;
	DWORD				dwElements;
	HRESULT				hr;
	static float		oldSysTime;

	if ( !g_pMouse ) {
		return;
	}

	// fetch new events
	for (;;)
	{
		dwElements = 1;

		hr = IDirectInputDevice_GetDeviceData(g_pMouse,
				sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			IDirectInputDevice_Acquire(g_pMouse);
			return;
		}

		/* Unable to read data or no data available */
		if ( FAILED(hr) ) {
			break;
		}

		if ( dwElements == 0 ) {
			break;
		}

		switch (od.dwOfs) {
		case DIMOFS_BUTTON0:
			if (od.dwData & 0x80)
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE1, qtrue, 0, NULL );
			else
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE1, qfalse, 0, NULL );
			break;

		case DIMOFS_BUTTON1:
			if (od.dwData & 0x80)
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE2, qtrue, 0, NULL );
			else
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE2, qfalse, 0, NULL );
			break;
			
		case DIMOFS_BUTTON2:
			if (od.dwData & 0x80)
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE3, qtrue, 0, NULL );
			else
				Sys_QueEvent( od.dwTimeStamp, SE_KEY, A_MOUSE3, qfalse, 0, NULL );
			break;
		}
	}

	// read the raw delta counter and ignore
	// the individual sample time / values
	hr = IDirectInputDevice_GetDeviceState(g_pMouse,
			sizeof(DIDEVICEOBJECTDATA), &state);
	if ( FAILED(hr) ) {
		*mx = *my = 0;
		return;
	}
	*mx = state.lX;
	*my = state.lY;
}

/*
============================================================

  MOUSE CONTROL

============================================================
*/

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse( void ) 
{
	if (!s_wmv.mouseInitialized ) {
		return;
	}
	if ( !in_mouse->integer ) 
	{
		s_wmv.mouseActive = qfalse;
		return;
	}
	if ( s_wmv.mouseActive ) 
	{
		return;
	}

	s_wmv.mouseActive = qtrue;

	if ( in_mouse->integer == 2 ) {
		IN_ActivateRawMouse();
	} else if ( in_mouse->integer != -1 ) {
		IN_ActivateDIMouse();
	}
	IN_ActivateWin32Mouse();
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse( void ) {
	if (!s_wmv.mouseInitialized ) {
		return;
	}
	if (!s_wmv.mouseActive ) {
		return;
	}
	s_wmv.mouseActive = qfalse;

	IN_DeactivateRawMouse();
	IN_DeactivateDIMouse();
	IN_DeactivateWin32Mouse();
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse( void ) 
{
	s_wmv.mouseInitialized = qfalse;

	if ( in_mouse->integer == 0 ) {
		Com_Printf ("Mouse control not active.\n");
		return;
	}

	// nt4.0 direct input is screwed up
	if ( ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT ) &&
		 ( g_wv.osversion.dwMajorVersion == 4 ) )
	{
		Com_Printf ("Disallowing DirectInput on NT 4.0\n");
		Cvar_Set( "in_mouse", "-1" );
	}

	s_wmv.mouseInitialized = qtrue;

	if ( in_mouse->integer == -1 ) {
		Com_Printf ("Skipping check for DirectInput\n");
	} else if ( in_mouse->integer == 2 ) {
		if ( IN_InitRawMouse() ) {
			return;
		}
		Com_Printf ("Falling back to Win32 mouse support...\n");
	} else {
		if ( IN_InitDIMouse() ) {
			return;
		}
		Com_Printf ("Falling back to Win32 mouse support...\n");
	}
	IN_InitWin32Mouse();
}

/*
===========
IN_MouseEvent
===========
*/
#define MAX_MOUSE_BUTTONS	5

static int mouseConvert[MAX_MOUSE_BUTTONS] =
{
	A_MOUSE1,
	A_MOUSE2,
	A_MOUSE3,
	A_MOUSE4,
	A_MOUSE5
};

void IN_MouseEvent (int mstate)
{
	int		i;

	if ( !s_wmv.mouseInitialized )
	{
		return;
	}

	// perform button actions
	for  (i = 0 ; i < MAX_MOUSE_BUTTONS ; i++ )
	{
		if ( (mstate & (1 << i)) && !(s_wmv.oldButtonState & (1 << i)) )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, mouseConvert[i], true, 0, NULL );
		}
		if ( !(mstate & (1 << i)) && (s_wmv.oldButtonState & (1 << i)) )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, mouseConvert[i], false, 0, NULL );
		}
	}	
	s_wmv.oldButtonState = mstate;
}



/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove ( void ) {
	int		mx, my;

	if ( rawMouseInitialized ) {
		IN_RawMouse( &mx, &my );
	} else if ( g_pMouse ) {
		IN_DIMouse( &mx, &my );
	} else {
		IN_Win32Mouse( &mx, &my );
	}

	if ( !mx && !my ) {
		return;
	}

	Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL );
}


/*
=========================================================================

=========================================================================
*/

/*
===========
IN_Startup
===========
*/
void IN_Startup( void ) {
	Com_Printf ("\n------- Input Initialization -------\n");
	IN_StartupMouse ();
	IN_StartupJoystick ();
	IN_StartupMIDI();
	Com_Printf ("------------------------------------\n");

	in_mouse->modified = qfalse;
	in_joystick->modified = qfalse;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void ) {
	IN_DeactivateMouse();
	IN_ShutdownRawMouse();
	IN_ShutdownDIMouse();
	IN_ShutdownMIDI();
#ifndef NO_XINPUT
	if( in_joystick && in_joystick->integer == 2 )
	{
		IN_UnloadXInput();
	}
#endif
	Cmd_RemoveCommand("midiinfo" );
}


/*
===========
IN_Init
===========
*/
void IN_Init( void ) {
	// MIDI input controler variables
	in_midi					= Cvar_Get ("in_midi",					"0",		CVAR_ARCHIVE);
	in_midiport				= Cvar_Get ("in_midiport",				"1",		CVAR_ARCHIVE);
	in_midichannel			= Cvar_Get ("in_midichannel",			"1",		CVAR_ARCHIVE);
	in_mididevice			= Cvar_Get ("in_mididevice",			"0",		CVAR_ARCHIVE);

	Cmd_AddCommand( "midiinfo", MidiInfo_f );

	// mouse variables
    in_mouse				= Cvar_Get ("in_mouse",					"-1",		CVAR_ARCHIVE|CVAR_LATCH);

	// joystick variables
	in_joystick				= Cvar_Get ("in_joystick",				"0",		CVAR_ARCHIVE|CVAR_LATCH);
	in_joyBallScale			= Cvar_Get ("in_joyBallScale",			"0.02",		CVAR_ARCHIVE);
	in_debugJoystick		= Cvar_Get ("in_debugjoystick",			"0",		CVAR_TEMP);

	joy_threshold			= Cvar_Get ("joy_threshold",			"0.15",		CVAR_ARCHIVE);

	joy_xbutton			= Cvar_Get ("joy_xbutton",			"1",		CVAR_ARCHIVE);	// treat axis as a button
	joy_ybutton			= Cvar_Get ("joy_ybutton",			"0",		CVAR_ARCHIVE);	// treat axis as a button

#ifndef NO_XINPUT
	xin_invertThumbsticks	= Cvar_Get ("xin_invertThumbsticks",	"0",		CVAR_ARCHIVE);
	xin_invertLookX			= Cvar_Get ("xin_invertLookX",			"0",		CVAR_ARCHIVE);
	xin_invertLookY			= Cvar_Get ("xin_invertLookY",			"0",		CVAR_ARCHIVE);

	xin_rumbleScale			= Cvar_Get ("xin_rumbleScale",			"1.0",		CVAR_ARCHIVE);

#endif

	IN_Startup();
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active) {
	in_appactive = active;

	if ( !active )
	{
		IN_DeactivateMouse();
	}
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void) {
	// post joystick events
	IN_JoyMove();

	if ( !s_wmv.mouseInitialized ) {
		return;
	}

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	qboolean loading = (qboolean)( cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE );

	if( !Cvar_VariableIntegerValue("r_fullscreen") && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		// temporarily deactivate if not in the game and
		// running on the desktop
		IN_DeactivateMouse ();
		return;
	}

	if( !Cvar_VariableIntegerValue("r_fullscreen") && loading ) {
		IN_DeactivateMouse ();
		return;
	}

	if ( !in_appactive ) {
		IN_DeactivateMouse ();
		return;
	}

	IN_ActivateMouse();

	// post events to the system que
	IN_MouseMove();

}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void) 
{
	s_wmv.oldButtonState = 0;
}


/*
=========================================================================

JOYSTICK

=========================================================================
*/

#ifndef NO_XINPUT

typedef struct {
	WORD wButtons;
	BYTE bLeftTrigger;
	BYTE bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
	DWORD dwPaddingReserved;
} XINPUT_GAMEPAD_EX;

typedef struct {
	DWORD dwPacketNumber;
	XINPUT_GAMEPAD_EX Gamepad;
} XINPUT_STATE_EX;

#define X360_GUIDE_BUTTON 0x0400
#define X360_LEFT_TRIGGER_MASK 0x10000
#define X360_RIGHT_TRIGGER_MASK 0x20000

static XINPUT_STATE_EX xiState;
static DWORD dwLastXIButtonState;

static HMODULE xiLibrary = NULL;

typedef DWORD (__stdcall *XIGetFuncPointer)(DWORD, XINPUT_STATE_EX *);
typedef DWORD (__stdcall *XISetFuncPointer)(DWORD, XINPUT_VIBRATION *);
XIGetFuncPointer XI_GetStateEx = NULL;
XISetFuncPointer XI_SetState = NULL;

/*
===============
IN_LoadXInput

Uses direct DLL loading as opposed to static linkage
This is because, as Ensiform pointed out, Windows 8
and Windows 7 use different XInput versions, hence
different linkage.
===============
*/

qboolean IN_LoadXInput ( void )
{
	int lastNum;
	if( xiLibrary )
		return qfalse;	// already loaded

	for(lastNum = 4; lastNum >= 3; lastNum--)		// increment as more XInput versions become supported
	{
		xiLibrary = LoadLibrary( va("XInput1_%i.dll", lastNum) );
		if( xiLibrary)
			break;
	}
	if( !xiLibrary )
	{
		Com_Printf( S_COLOR_RED"XInput not detected on your system. Please download the XBOX 360 drivers from the Microsoft home page.\n" );
		return qfalse;
	}
	
	// MEGA HACK:
	// Ordinal 100 in the XInput DLL supposedly contains a modified/improved version
	// of the XInputGetState function, with one key difference: XInputGetState does
	// not get the status of the XBOX Guide button, while XInputGetStateEx does.
	XI_GetStateEx = (XIGetFuncPointer)GetProcAddress( xiLibrary, (LPCSTR)100 );
	XI_SetState = (XISetFuncPointer)GetProcAddress( xiLibrary, "XInputSetState" );

	if( !XI_GetStateEx || !XI_SetState )
	{
		Com_Printf("^1IN_LoadXInput failed on pointer establish\n");
		IN_UnloadXInput();
		return qfalse;
	}
	return qtrue;
}

/*
===============
IN_UnloadXInput

XInput gets unloaded if we change input modes or we shut
down the game
===============
*/

void IN_UnloadXInput ( void )
{
	if( !xiLibrary )
	{
		// not loaded, so don't bother trying to unload
		return;
	}
	FreeLibrary( xiLibrary );
	xiLibrary = NULL;
}

/*
===============
IN_JoystickInitXInput

XBOX 360 controller only
===============
*/

void IN_JoystickInitXInput ( void )
{
	Com_Printf("Joystick cvar enabled -- XInput mode\n");

	if(!IN_LoadXInput())
	{
		Com_Printf("Could not load XInput -- see above error. Controller not enabled.\n");
		return;
	}

	ZeroMemory( &xiState, sizeof(XINPUT_STATE_EX) );
	dwLastXIButtonState = 0UL;

	if (XI_GetStateEx( 0, &xiState ) != ERROR_SUCCESS ) {	// only support for Controller 1 atm. If I get bored or something, 
															// I'll probably add a splitscreen mode just for lulz --eez
		Com_Printf("XBOX 360 controller not detected -- no drivers or bad connection\n");
		return;
	}

	joy.avail = qtrue;	// semi hack, we really have no use for joy. whatever, but we use this to message when connection state changes

}

#endif

/*
===============
IN_JoystickInitDInput

DirectInput only
===============
*/
void IN_JoystickInitDInput ( void )
{
	int			numdevs;
	MMRESULT	mmr;

	Com_Printf("Joystick cvar enabled -- DirectInput mode\n");

	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0)
	{
		Com_Printf ("joystick not found -- driver not present\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	mmr = 0;
	for (joy.id=0 ; joy.id<numdevs ; joy.id++)
	{
		memset (&joy.ji, 0, sizeof(joy.ji));
		joy.ji.dwSize = sizeof(joy.ji);
		joy.ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (joy.id, &joy.ji)) == JOYERR_NOERROR)
			break;
	} 

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		Com_Printf ("joystick not found -- no valid joysticks (%x)\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&joy.jc, 0, sizeof(joy.jc));
	if ((mmr = joyGetDevCaps (joy.id, &joy.jc, sizeof(joy.jc))) != JOYERR_NOERROR)
	{
		Com_Printf ("joystick not found -- invalid joystick capabilities (%x)\n", mmr); 
		return;
	}

	Com_Printf( "Joystick found.\n" );
	Com_Printf( "Pname: %s\n", joy.jc.szPname );
	Com_Printf( "OemVxD: %s\n", joy.jc.szOEMVxD );
	Com_Printf( "RegKey: %s\n", joy.jc.szRegKey );

	Com_Printf( "Numbuttons: %i / %i\n", joy.jc.wNumButtons, joy.jc.wMaxButtons );
	Com_Printf( "Axis: %i / %i\n", joy.jc.wNumAxes, joy.jc.wMaxAxes );
	Com_Printf( "Caps: 0x%x\n", joy.jc.wCaps );
	if ( joy.jc.wCaps & JOYCAPS_HASPOV ) {
		Com_Printf( "HASPOV\n" );
	} else {
		Com_Printf( "no POV\n" );
	}

	// old button and POV states default to no buttons pressed
	joy.oldbuttonstate = 0;
	joy.oldpovstate = 0;

	// mark the joystick as available
	joy.avail = qtrue; 
}

/* 
=============== 
IN_StartupJoystick 
=============== 
*/  
void IN_StartupJoystick (void) { 
	// assume no joystick
	joy.avail = qfalse; 

	if ( in_joystick->integer == 1 )
	{
		// DirectInput mode --eez
		IN_JoystickInitDInput();
	}
#ifndef NO_XINPUT
	else if ( in_joystick->integer == 2 )
	{
		// xbox 360 awesomeness
		IN_JoystickInitXInput();
	}
#endif
	else {
		Com_Printf ("Joystick is not active.\n");
		return;
	}
	
}

/*
===========
JoyToF
===========
*/
float JoyToF( int value ) {
	float	fValue;

	// move centerpoint to zero
	value -= 32768;

	// convert range from -32768..32767 to -1..1 
	fValue = (float)value / 32768.0;

	if ( fValue < -1 ) {
		fValue = -1;
	}
	if ( fValue > 1 ) {
		fValue = 1;
	}
	return fValue;
}

int JoyToI( int value ) {
	// move centerpoint to zero
	value -= 32768;

	return value;
}

int	joyDirectionKeys[16] = {
	A_CURSOR_LEFT, A_CURSOR_RIGHT,
	A_CURSOR_UP, A_CURSOR_DOWN,
	A_JOY16, A_JOY17,
	A_JOY18, A_JOY19,
	A_JOY20, A_JOY21,
	A_JOY22, A_JOY23,

	A_JOY24, A_JOY25,
	A_JOY26, A_JOY27
};

/*
===========
IN_DoDirectInput

Equivalent of IN_JoyMove for DirectInput
===========
*/

void IN_DoDirectInput( void )
{
	float	fAxisValue;
	int		i;
	DWORD	buttonstate, povstate;
	int		x, y;

	// collect the joystick data, if possible
	memset (&joy.ji, 0, sizeof(joy.ji));
	joy.ji.dwSize = sizeof(joy.ji);
	joy.ji.dwFlags = JOY_RETURNALL;

	if ( joyGetPosEx (joy.id, &joy.ji) != JOYERR_NOERROR ) {
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Com_Printf ("IN_ReadJoystick: no response\n");
		// joy.avail = false;
		return;
	}

	if ( in_debugJoystick->integer ) {
		Com_Printf( "%8x %5i %5.2f %5.2f %5.2f %5.2f %6i %6i\n", 
			joy.ji.dwButtons,
			joy.ji.dwPOV,
			JoyToF( joy.ji.dwXpos ), JoyToF( joy.ji.dwYpos ),
			JoyToF( joy.ji.dwZpos ), JoyToF( joy.ji.dwRpos ),
			JoyToI( joy.ji.dwUpos ), JoyToI( joy.ji.dwVpos ) );
	}

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = joy.ji.dwButtons;
	for ( i=0 ; i < joy.jc.wNumButtons ; i++ ) {
		if ( (buttonstate & (1<<i)) && !(joy.oldbuttonstate & (1<<i)) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_JOY0 + i, qtrue, 0, NULL );
		}
		if ( !(buttonstate & (1<<i)) && (joy.oldbuttonstate & (1<<i)) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_JOY0 + i, qfalse, 0, NULL );
		}
	}
	joy.oldbuttonstate = buttonstate;

	povstate = 0;

	// convert main joystick motion into 6 direction button bits
	for (i = 0; i < joy.jc.wNumAxes && i < 4 ; i++) {
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = JoyToF( (&joy.ji.dwXpos)[i] );
		
		if (i == 0 && !joy_xbutton->integer) {
			if ( fAxisValue < -joy_threshold->value || fAxisValue > joy_threshold->value){
				Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_SIDE, (int) -(fAxisValue*127.0), 0, NULL );
			}else{
				Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_SIDE, 0, 0, NULL );
			}
			continue;
		}
		
		if (i == 1 && !joy_ybutton->integer) {
			if ( fAxisValue < -joy_threshold->value || fAxisValue > joy_threshold->value){
				Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_FORWARD, (int) -(fAxisValue*127.0), 0, NULL );
			}else{
				Sys_QueEvent( g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_FORWARD, 0, 0, NULL );
			}
			continue;
		}
		
		if ( fAxisValue < -joy_threshold->value ) {
			povstate |= (1<<(i*2));
		} else if ( fAxisValue > joy_threshold->value ) {
			povstate |= (1<<(i*2+1));
		}
	}		

	// convert POV information from a direction into 4 button bits
	if ( joy.jc.wCaps & JOYCAPS_HASPOV ) {
		if ( joy.ji.dwPOV != JOY_POVCENTERED ) {
			if (joy.ji.dwPOV == JOY_POVFORWARD)
				povstate |= 1<<12;
			if (joy.ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 1<<13;
			if (joy.ji.dwPOV == JOY_POVRIGHT)
				povstate |= 1<<14;
			if (joy.ji.dwPOV == JOY_POVLEFT)
				povstate |= 1<<15;
		}
	}

	// determine which bits have changed and key an auxillary event for each change
	for (i=0 ; i < 16 ; i++) {
		if ( (povstate & (1<<i)) && !(joy.oldpovstate & (1<<i)) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qtrue, 0, NULL );
		}

		if ( !(povstate & (1<<i)) && (joy.oldpovstate & (1<<i)) ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, joyDirectionKeys[i], qfalse, 0, NULL );
		}
	}
	joy.oldpovstate = povstate;

	// if there is a trackball like interface, simulate mouse moves
	if ( joy.jc.wNumAxes >= 6 ) {
		x = JoyToI( joy.ji.dwUpos ) * in_joyBallScale->value;
		y = JoyToI( joy.ji.dwVpos ) * in_joyBallScale->value;
		if ( x || y ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_MOUSE, x, y, 0, NULL );
		}
	}
}

#ifndef NO_XINPUT
/*
===========
XI_ThumbFloat

Gets the percentage going one way or the other (as normalized float)
===========
*/
float QINLINE XI_ThumbFloat( signed short thumbValue )
{
	return (thumbValue < 0) ? (thumbValue / 32768.0f) : (thumbValue / 32767.0f);
}

/*
===========
XI_ApplyInversion

Inverts look up/down and look left/right appropriately
===========
*/
void XI_ApplyInversion( float *fX, float *fY )
{
	if( xin_invertLookX->integer )
		*fX *= -1.0f;
	if( xin_invertLookY->integer )
		*fY *= -1.0f;
}

#define CheckButtonStatus( xin, fakekey ) \
	if ( (xiState.Gamepad.wButtons & xin) && !(dwLastXIButtonState & xin) ) \
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, fakekey, qtrue, 0, NULL); \
	if ( !(xiState.Gamepad.wButtons & xin) && (dwLastXIButtonState & xin) ) \
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, fakekey, qfalse, 0, NULL); \
	if ( (xiState.Gamepad.wButtons & xin) ) \
		dwLastXIButtonState |= xin; \
	else \
		dwLastXIButtonState &= ~xin; \

/*
===========
IN_DoXInput

Equivalent of IN_JoyMove for XInput (xbox 360)
===========
*/

void IN_DoXInput( void )
{
	if(!joy.avail)
	{
		// Joystick not found, continue to search for it :>
		if( XI_GetStateEx(0, &xiState) == ERROR_SUCCESS )
		{
			joy.avail = qtrue;
			Com_Printf("Controller connected.\n");
		}
		else
		{
			return;
		}
	}
	else
	{
		if( XI_GetStateEx(0, &xiState) != ERROR_SUCCESS )
		{
			joy.avail = qfalse;
			Com_Printf("Controller disconnected.\n");
			return;
		}
	}

	// Now that we've dealt with the basic checks for connectivity, let's actually do the _important_ crap.
	float leftThumbX = XI_ThumbFloat(xiState.Gamepad.sThumbLX);
	float leftThumbY = XI_ThumbFloat(xiState.Gamepad.sThumbLY);
	float rightThumbX = XI_ThumbFloat(xiState.Gamepad.sThumbRX);
	float rightThumbY = XI_ThumbFloat(xiState.Gamepad.sThumbRY);
	float dX = 0, dY = 0;

	/* hi microsoft, go fuck yourself for flipping left stick's Y axis for no reason... */
	leftThumbY *= -1.0f;
	rightThumbY *= -1.0f;

	// JOYSTICKS
	// This is complete and utter trash in DirectInput, because it doesn't send like half as much crap as it should.
	if( xin_invertThumbsticks->integer )
	{
		// Left stick functions like right stick
		XI_ApplyInversion(&leftThumbX, &leftThumbY);

		// Left stick behavior
		if( abs(leftThumbX) > joy_threshold->value )	// FIXME: what does do about deadzones and sensitivity...
		{
			dX = (leftThumbX-joy_threshold->value) * in_joyBallScale->value;
		}
		if( abs(leftThumbY) > joy_threshold->value )
		{
			dY = (leftThumbY-joy_threshold->value) * in_joyBallScale->value;
		}
		
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_YAW, rightThumbX, 0, NULL);
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_PITCH, rightThumbY, 0, NULL);

		// Right stick behavior
		// Hardcoded deadzone within the gamecode itself to deal with the situation
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_SIDE, rightThumbX * 127, 0, NULL);
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_FORWARD, rightThumbY * -127, 0, NULL);
	}
	else
	{
		// Thumbsticks act as they should (right stick = camera, left stick = wasd equivalent)
		XI_ApplyInversion(&rightThumbX, &rightThumbY);

		// Left stick behavior
		// Hardcoded deadzone within the gamecode itself to deal with the situation
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_SIDE, leftThumbX * 127, 0, NULL);
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_FORWARD, leftThumbY * -127, 0, NULL);

		// Right stick behavior
		if( abs(rightThumbX) > joy_threshold->value )
		{
			float factor = abs(rightThumbX*128);
			dX = (rightThumbX-joy_threshold->value) * in_joyBallScale->value * factor;
			if(in_debugJoystick->integer)
				Com_Printf("rightThumbX: %f\tfactor: %f\tdX: %f\n", rightThumbX, factor, dX);
		}
		if( abs(rightThumbY) > joy_threshold->value )
		{
			float factor = abs(rightThumbY*128);
			dY = (rightThumbY-joy_threshold->value) * in_joyBallScale->value * factor;
			if(in_debugJoystick->integer)
				Com_Printf("rightThumbY: %f\tfactor: %f\tdX: %f\n", rightThumbY, factor, dY);
		}
		
		// ...but cap it at a reasonable amount.
		if(dX < -2.5f) dX = -2.5f;
		if(dX > 2.5f) dX = 2.5f;
		if(dY < -2.5f) dY = -2.5f;
		if(dY > 2.5f) dY = 2.5f;

		dX *= 1024;
		dY *= 1024;

		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_YAW, dX, 0, NULL);
		Sys_QueEvent(g_wv.sysMsgTime, SE_JOYSTICK_AXIS, AXIS_PITCH, dY, 0, NULL);
	}

	if ( (Key_GetCatcher() & KEYCATCH_UI) ) {
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_UP, A_CURSOR_UP );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_DOWN, A_CURSOR_DOWN );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_LEFT, A_CURSOR_LEFT );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_RIGHT, A_CURSOR_RIGHT );
		CheckButtonStatus( XINPUT_GAMEPAD_START, A_JOY4 );
		CheckButtonStatus( XINPUT_GAMEPAD_BACK, A_JOY5 );
		CheckButtonStatus( XINPUT_GAMEPAD_LEFT_THUMB, A_JOY6 );
		CheckButtonStatus( XINPUT_GAMEPAD_RIGHT_THUMB, A_JOY7 );
		CheckButtonStatus( XINPUT_GAMEPAD_LEFT_SHOULDER, A_JOY8 );
		CheckButtonStatus( XINPUT_GAMEPAD_RIGHT_SHOULDER, A_JOY9 );
		CheckButtonStatus( X360_GUIDE_BUTTON, A_JOY10 );
		CheckButtonStatus( XINPUT_GAMEPAD_A, A_ENTER );
		CheckButtonStatus( XINPUT_GAMEPAD_B, A_JOY12 );
		CheckButtonStatus( XINPUT_GAMEPAD_X, A_JOY13 );
		CheckButtonStatus( XINPUT_GAMEPAD_Y, A_JOY14 );
	}
	else {
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_UP, A_JOY0 );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_DOWN, A_JOY1 );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_LEFT, A_JOY2 );
		CheckButtonStatus( XINPUT_GAMEPAD_DPAD_RIGHT, A_JOY3 );
		CheckButtonStatus( XINPUT_GAMEPAD_START, A_JOY4 );
		CheckButtonStatus( XINPUT_GAMEPAD_BACK, A_JOY5 );
		CheckButtonStatus( XINPUT_GAMEPAD_LEFT_THUMB, A_JOY6 );
		CheckButtonStatus( XINPUT_GAMEPAD_RIGHT_THUMB, A_JOY7 );
		CheckButtonStatus( XINPUT_GAMEPAD_LEFT_SHOULDER, A_JOY8 );
		CheckButtonStatus( XINPUT_GAMEPAD_RIGHT_SHOULDER, A_JOY9 );
		CheckButtonStatus( X360_GUIDE_BUTTON, A_JOY10 );
		CheckButtonStatus( XINPUT_GAMEPAD_A, A_JOY11 );
		CheckButtonStatus( XINPUT_GAMEPAD_B, A_JOY12 );
		CheckButtonStatus( XINPUT_GAMEPAD_X, A_JOY13 );
		CheckButtonStatus( XINPUT_GAMEPAD_Y, A_JOY14 );
	}

	// extra magic required for the triggers
	if( xiState.Gamepad.bLeftTrigger && !(dwLastXIButtonState & X360_LEFT_TRIGGER_MASK) )
	{
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, A_JOY15, qtrue, 0, NULL);
	}
	else if( !xiState.Gamepad.bLeftTrigger && ( dwLastXIButtonState & X360_LEFT_TRIGGER_MASK ) )
	{
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, A_JOY15, qfalse, 0, NULL);
	}
	if( xiState.Gamepad.bLeftTrigger )
		dwLastXIButtonState |= X360_LEFT_TRIGGER_MASK;
	else
		dwLastXIButtonState &= ~X360_LEFT_TRIGGER_MASK;

	if( xiState.Gamepad.bRightTrigger && !( dwLastXIButtonState & X360_RIGHT_TRIGGER_MASK ) )
	{
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, A_JOY16, qtrue, 0, NULL);
	}
	else if( !xiState.Gamepad.bRightTrigger && ( dwLastXIButtonState & X360_RIGHT_TRIGGER_MASK ) )
	{
		Sys_QueEvent(g_wv.sysMsgTime, SE_KEY, A_JOY16, qfalse, 0, NULL);
	}
	if( xiState.Gamepad.bRightTrigger )
		dwLastXIButtonState |= X360_RIGHT_TRIGGER_MASK;
	else
		dwLastXIButtonState &= ~X360_RIGHT_TRIGGER_MASK;

	if(in_debugJoystick->integer)
		Com_Printf("buttons: \t%i\n", dwLastXIButtonState);
}
#endif

/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove( void ) 
{
	if( in_joystick->integer == 1 && joy.avail)
	{
		IN_DoDirectInput();
	}
#ifndef NO_XINPUT
	else if( in_joystick->integer == 2 )
	{
		IN_DoXInput();
	}
#endif
}

/*
=========================================================================

MIDI

=========================================================================
*/

static void MIDI_NoteOff( int note )
{
	int qkey;

	qkey = note - 60 + A_AUX0;

	if ( qkey < A_AUX0 )
	{
		return;
	}
	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qfalse, 0, NULL );
}

static void MIDI_NoteOn( int note, int velocity )
{
	int qkey;

	if ( velocity == 0 )
	{
		MIDI_NoteOff( note );
	}

	qkey = note - 60 + A_AUX0;

	if ( qkey < A_AUX0 )
	{
		return;
	}
	Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, qkey, qtrue, 0, NULL );
}

static void CALLBACK MidiInProc( HMIDIIN hMidiIn, UINT uMsg, DWORD dwInstance, 
								 DWORD dwParam1, DWORD dwParam2 )
{
	int message;

	switch ( uMsg )
	{
	case MIM_OPEN:
		break;
	case MIM_CLOSE:
		break;
	case MIM_DATA:
		message = dwParam1 & 0xff;

		// note on
		if ( ( message & 0xf0 ) == 0x90 )
		{
			if ( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer )
				MIDI_NoteOn( ( dwParam1 & 0xff00 ) >> 8, ( dwParam1 & 0xff0000 ) >> 16 );
		}
		else if ( ( message & 0xf0 ) == 0x80 )
		{
			if ( ( ( message & 0x0f ) + 1 ) == in_midichannel->integer )
				MIDI_NoteOff( ( dwParam1 & 0xff00 ) >> 8 );
		}
		break;
	case MIM_LONGDATA:
		break;
	case MIM_ERROR:
		break;
	case MIM_LONGERROR:
		break;
	}

//	Sys_QueEvent( sys_msg_time, SE_KEY, wMsg, qtrue, 0, NULL );
}

static void MidiInfo_f( void )
{
	int i;

	const char *enableStrings[] = { "disabled", "enabled" };

	Com_Printf( "\nMIDI control:       %s\n", enableStrings[in_midi->integer != 0] );
	Com_Printf( "port:               %d\n", in_midiport->integer );
	Com_Printf( "channel:            %d\n", in_midichannel->integer );
	Com_Printf( "current device:     %d\n", in_mididevice->integer );
	Com_Printf( "number of devices:  %d\n", s_midiInfo.numDevices );
	for ( i = 0; i < s_midiInfo.numDevices; i++ )
	{
		if ( i == Cvar_VariableIntegerValue( "in_mididevice" ) )
			Com_Printf( "***" );
		else
			Com_Printf( "..." );
		Com_Printf(    "device %2d:       %s\n", i, s_midiInfo.caps[i].szPname );
		Com_Printf( "...manufacturer ID: 0x%hx\n", s_midiInfo.caps[i].wMid );
		Com_Printf( "...product ID:      0x%hx\n", s_midiInfo.caps[i].wPid );

		Com_Printf( "\n" );
	}
}

static void IN_StartupMIDI( void )
{
	int i;

	if ( !Cvar_VariableIntegerValue( "in_midi" ) )
		return;

	//
	// enumerate MIDI IN devices
	//
	s_midiInfo.numDevices = midiInGetNumDevs();

	for ( i = 0; i < s_midiInfo.numDevices; i++ )
	{
		midiInGetDevCaps( i, &s_midiInfo.caps[i], sizeof( s_midiInfo.caps[i] ) );
	}

	//
	// open the MIDI IN port
	//
	if ( midiInOpen( &s_midiInfo.hMidiIn, 
		             in_mididevice->integer,
					 ( unsigned long ) MidiInProc,
					 ( unsigned long ) NULL,
					 CALLBACK_FUNCTION ) != MMSYSERR_NOERROR )
	{
		Com_Printf( "WARNING: could not open MIDI device %d: '%s'\n", in_mididevice->integer , s_midiInfo.caps[( int ) in_mididevice->value] );
		return;
	}

	midiInStart( s_midiInfo.hMidiIn );
}

static void IN_ShutdownMIDI( void )
{
	if ( s_midiInfo.hMidiIn )
	{
		midiInClose( s_midiInfo.hMidiIn );
	}
	memset( &s_midiInfo, 0, sizeof( s_midiInfo ) );
}
