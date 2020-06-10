/*
===========================================================================
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include <SDL.h>
#include "qcommon/qcommon.h"
#include "qcommon/q_shared.h"
#include "client/client.h"
#include "sys/sys_local.h"

static cvar_t *in_keyboardDebug     = NULL;

static SDL_Joystick *stick = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

cvar_t *in_joystick          		= NULL;
static cvar_t *in_joystickThreshold = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;

static SDL_Window *SDL_window = NULL;

#define CTRL(a) ((a)-'a'+1)

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, fakeAscii_t key, qboolean down )
{
	if( down )
		Com_Printf( "+ " );
	else
		Com_Printf( "  " );

	Com_Printf( "Scancode: 0x%02x(%s) Sym: 0x%02x(%s)",
			keysym->scancode, SDL_GetScancodeName( keysym->scancode ),
			keysym->sym, SDL_GetKeyName( keysym->sym ) );

	if( keysym->mod & KMOD_LSHIFT )   Com_Printf( " KMOD_LSHIFT" );
	if( keysym->mod & KMOD_RSHIFT )   Com_Printf( " KMOD_RSHIFT" );
	if( keysym->mod & KMOD_LCTRL )    Com_Printf( " KMOD_LCTRL" );
	if( keysym->mod & KMOD_RCTRL )    Com_Printf( " KMOD_RCTRL" );
	if( keysym->mod & KMOD_LALT )     Com_Printf( " KMOD_LALT" );
	if( keysym->mod & KMOD_RALT )     Com_Printf( " KMOD_RALT" );
	if( keysym->mod & KMOD_LGUI )     Com_Printf( " KMOD_LGUI" );
	if( keysym->mod & KMOD_RGUI )     Com_Printf( " KMOD_RGUI" );
	if( keysym->mod & KMOD_NUM )      Com_Printf( " KMOD_NUM" );
	if( keysym->mod & KMOD_CAPS )     Com_Printf( " KMOD_CAPS" );
	if( keysym->mod & KMOD_MODE )     Com_Printf( " KMOD_MODE" );
	if( keysym->mod & KMOD_RESERVED ) Com_Printf( " KMOD_RESERVED" );

	Com_Printf( " Q:0x%02x(%s)\n", key, Key_KeynumToString( key ) );
}

#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static qboolean IN_IsConsoleKey( fakeAscii_t key, int character )
{
	typedef struct consoleKey_s
	{
		enum
		{
			QUAKE_KEY,
			CHARACTER
		} type;

		union
		{
			fakeAscii_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if( cl_consoleKeys->modified )
	{
		const char *text_p;
        char *token;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		COM_BeginParseSession("cl_consoleKeys");
		while( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			token = COM_Parse( &text_p );
			if( !token[ 0 ] )
				break;

			if( strlen( token ) == 4 )
				charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = consoleKey_t::CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = consoleKey_t::QUAKE_KEY;
				c->u.key = (fakeAscii_t)Key_StringToKeynum( token );

				// 0 isn't a key
				if( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if( key == character )
		key = A_NULL;

	for( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch( c->type )
		{
			case consoleKey_t::QUAKE_KEY:
				if( key && c->u.key == key )
					return qtrue;
				break;

			case consoleKey_t::CHARACTER:
				if( c->u.character == character )
					return qtrue;
				break;
		}
	}

	return qfalse;
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static bool IN_NumLockEnabled( void )
{
#if defined(_WIN32)
	return (GetKeyState( VK_NUMLOCK ) & 1) != 0;
#else
	// @fixme : doesn't give proper state if numlock is on before app startup
	return (SDL_GetModState() & KMOD_NUM) != 0;
#endif
}

static void IN_TranslateNumpad( SDL_Keysym *keysym, fakeAscii_t *key )
{
	if ( IN_NumLockEnabled() )
	{
		switch ( keysym->sym )
		{
		case SDLK_KP_0:
			keysym->scancode = SDL_SCANCODE_0;
			keysym->sym = SDLK_0;
			*key = A_0;
			break;
		case SDLK_KP_1:
			keysym->scancode = SDL_SCANCODE_1;
			keysym->sym = SDLK_1;
			*key = A_1;
			break;
		case SDLK_KP_2:
			keysym->scancode = SDL_SCANCODE_2;
			keysym->sym = SDLK_2;
			*key = A_2;
			break;
		case SDLK_KP_3:
			keysym->scancode = SDL_SCANCODE_3;
			keysym->sym = SDLK_3;
			*key = A_3;
			break;
		case SDLK_KP_4:
			keysym->scancode = SDL_SCANCODE_4;
			keysym->sym = SDLK_4;
			*key = A_4;
			break;
		case SDLK_KP_5:
			keysym->scancode = SDL_SCANCODE_5;
			keysym->sym = SDLK_5;
			*key = A_5;
			break;
		case SDLK_KP_6:
			keysym->scancode = SDL_SCANCODE_6;
			keysym->sym = SDLK_6;
			*key = A_6;
			break;
		case SDLK_KP_7:
			keysym->scancode = SDL_SCANCODE_7;
			keysym->sym = SDLK_7;
			*key = A_7;
			break;
		case SDLK_KP_8:
			keysym->scancode = SDL_SCANCODE_8;
			keysym->sym = SDLK_8;
			*key = A_8;
			break;
		case SDLK_KP_9:
			keysym->scancode = SDL_SCANCODE_9;
			keysym->sym = SDLK_9;
			*key = A_9;
			break;
		default:
			break;
		}
	}
}

/*
===============
IN_TranslateSDLToJKKey
===============
*/
static fakeAscii_t IN_TranslateSDLToJKKey( SDL_Keysym *keysym, qboolean down ) {
	fakeAscii_t key = A_NULL;

	if ( keysym->sym >= A_LOW_A && keysym->sym <= A_LOW_Z )
		key = (fakeAscii_t)(A_CAP_A + (keysym->sym - A_LOW_A));
	else if ( keysym->sym >= A_LOW_AGRAVE && keysym->sym <= A_LOW_THORN && keysym->sym != A_DIVIDE )
		key = (fakeAscii_t)(A_CAP_AGRAVE + (keysym->sym - A_LOW_AGRAVE));
	else if ( keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE )
		key = (fakeAscii_t)keysym->sym;
	else
	{
		IN_TranslateNumpad( keysym, &key );

		switch( keysym->sym )
		{
			case SDLK_PAGEUP:       key = A_PAGE_UP;       break;
			case SDLK_KP_9:         key = A_KP_9;          break;
			case SDLK_PAGEDOWN:     key = A_PAGE_DOWN;     break;
			case SDLK_KP_3:         key = A_KP_3;          break;
			case SDLK_KP_7:         key = A_KP_7;          break;
			case SDLK_HOME:         key = A_HOME;          break;
			case SDLK_KP_1:         key = A_KP_1;          break;
			case SDLK_END:          key = A_END;           break;
			case SDLK_KP_4:         key = A_KP_4;          break;
			case SDLK_LEFT:         key = A_CURSOR_LEFT;   break;
			case SDLK_KP_6:         key = A_KP_6;          break;
			case SDLK_RIGHT:        key = A_CURSOR_RIGHT;  break;
			case SDLK_KP_2:         key = A_KP_2;          break;
			case SDLK_DOWN:         key = A_CURSOR_DOWN;   break;
			case SDLK_KP_8:         key = A_KP_8;          break;
			case SDLK_UP:           key = A_CURSOR_UP;     break;
			case SDLK_ESCAPE:       key = A_ESCAPE;        break;
			case SDLK_KP_ENTER:     key = A_KP_ENTER;      break;
			case SDLK_RETURN:       key = A_ENTER;         break;
			case SDLK_TAB:          key = A_TAB;           break;
			case SDLK_F1:           key = A_F1;            break;
			case SDLK_F2:           key = A_F2;            break;
			case SDLK_F3:           key = A_F3;            break;
			case SDLK_F4:           key = A_F4;            break;
			case SDLK_F5:           key = A_F5;            break;
			case SDLK_F6:           key = A_F6;            break;
			case SDLK_F7:           key = A_F7;            break;
			case SDLK_F8:           key = A_F8;            break;
			case SDLK_F9:           key = A_F9;            break;
			case SDLK_F10:          key = A_F10;           break;
			case SDLK_F11:          key = A_F11;           break;
			case SDLK_F12:          key = A_F12;           break;

			case SDLK_BACKSPACE:    key = A_BACKSPACE;     break;
			case SDLK_KP_PERIOD:    key = A_KP_PERIOD;     break;
			case SDLK_DELETE:       key = A_DELETE;        break;
			case SDLK_PAUSE:        key = A_PAUSE;         break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       key = A_SHIFT;         break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        key = A_CTRL;          break;

			case SDLK_RALT:
			case SDLK_LALT:         key = A_ALT;           break;

			case SDLK_KP_5:         key = A_KP_5;          break;
			case SDLK_INSERT:       key = A_INSERT;        break;
			case SDLK_KP_0:         key = A_KP_0;          break;
			case SDLK_KP_MULTIPLY:  key = A_STAR;          break;
			case SDLK_KP_PLUS:      key = A_KP_PLUS;       break;
			case SDLK_KP_MINUS:     key = A_KP_MINUS;      break;
			case SDLK_KP_DIVIDE:    key = A_FORWARD_SLASH; break;

			case SDLK_SCROLLLOCK:   key = A_SCROLLLOCK;    break;
			case SDLK_NUMLOCKCLEAR: key = A_NUMLOCK;       break;
			case SDLK_CAPSLOCK:     key = A_CAPSLOCK;      break;

			case L'\u00D7':			key = A_MULTIPLY;		break;
			case L'\u00E0':			key = A_LOW_AGRAVE;		break;
			case L'\u00E1':			key = A_LOW_AACUTE;		break;
			case L'\u00E2':			key = A_LOW_ACIRCUMFLEX; break;
			case L'\u00E3':			key = A_LOW_ATILDE;		break;
			case L'\u00E4':			key = A_LOW_ADIERESIS;	break;
			case L'\u00E5':			key = A_LOW_ARING;		break;
			case L'\u00E6':			key = A_LOW_AE;			break;
			case L'\u00E7':			key = A_LOW_CCEDILLA;	break;
			case L'\u00E8':			key = A_LOW_EGRAVE;		break;
			case L'\u00E9':			key = A_LOW_EACUTE;		break;
			case L'\u00EA':			key = A_LOW_ECIRCUMFLEX; break;
			case L'\u00EB':			key = A_LOW_EDIERESIS;	break;
			case L'\u00EC':			key = A_LOW_IGRAVE;		break;
			case L'\u00ED':			key = A_LOW_IACUTE;		break;
			case L'\u00EE':			key = A_LOW_ICIRCUMFLEX; break;
			case L'\u00EF':			key = A_LOW_IDIERESIS;	break;
			case L'\u00F0':			key = A_LOW_ETH;		break;
			case L'\u00F1':			key = A_LOW_NTILDE;		break;
			case L'\u00F2':			key = A_LOW_OGRAVE;		break;
			case L'\u00F3':			key = A_LOW_OACUTE;		break;
			case L'\u00F4':			key = A_LOW_OCIRCUMFLEX; break;
			case L'\u00F5':			key = A_LOW_OTILDE;		break;
			case L'\u00F6':			key = A_LOW_ODIERESIS;	break;
			case L'\u00F7':			key = A_DIVIDE;			break;
			case L'\u00F8':			key = A_LOW_OSLASH;		break;
			case L'\u00F9':			key = A_LOW_UGRAVE;		break;
			case L'\u00FA':			key = A_LOW_UACUTE;		break;
			case L'\u00FB':			key = A_LOW_UCIRCUMFLEX; break;
			case L'\u00FC':			key = A_LOW_UDIERESIS;	break;
			case L'\u00FD':			key = A_LOW_YACUTE;		break;
			case L'\u00FE':			key = A_LOW_THORN;		break;
			case L'\u00FF':			key = A_LOW_YDIERESIS;	break;

			default:
				break;
		}
	}

	if( in_keyboardDebug->integer )
		IN_PrintKey( keysym, key, down );

	if ( cl_consoleUseScanCode->integer )
	{
		if ( keysym->scancode == SDL_SCANCODE_GRAVE )
		{
			SDL_Keycode translated = SDL_GetKeyFromScancode( SDL_SCANCODE_GRAVE );

			if ( (translated != SDLK_CARET) || (translated == SDLK_CARET && (keysym->mod & KMOD_SHIFT)) )
			{
				// Console keys can't be bound or generate characters
				key = A_CONSOLE;
			}
		}
	}
	else
	{
		if ( IN_IsConsoleKey( key, 0 ) )
		{
			// Console keys can't be bound or generate characters
			key = A_CONSOLE;
		}
	}

	return key;
}

/*
===============
IN_GobbleMotionEvents
===============
*/
static void IN_GobbleMotionEvents( void )
{
	SDL_Event dummy[ 1 ];
	int val = 0;

	// Gobble any mouse motion events
	SDL_PumpEvents( );
	while( ( val = SDL_PeepEvents( dummy, 1, SDL_GETEVENT,
		SDL_MOUSEMOTION, SDL_MOUSEMOTION ) ) > 0 ) { }

	if ( val < 0 )
		Com_Printf( "IN_GobbleMotionEvents failed: %s\n", SDL_GetError( ) );
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	if( !mouseActive )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		SDL_SetWindowGrab( SDL_window, SDL_TRUE );

		IN_GobbleMotionEvents( );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !Cvar_VariableIntegerValue("r_fullscreen") )
	{
		if( in_nograb->modified || !mouseActive )
		{
			if( in_nograb->integer )
				SDL_SetWindowGrab( SDL_window, SDL_FALSE );
			else
				SDL_SetWindowGrab( SDL_window, SDL_TRUE );

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( void )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if( !Cvar_VariableIntegerValue("r_fullscreen") )
		SDL_ShowCursor( 1 );

	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
		IN_GobbleMotionEvents( );

		SDL_SetWindowGrab( SDL_window, SDL_FALSE );
		SDL_SetRelativeMouseMode( SDL_FALSE );

		// Don't warp the mouse unless the cursor is within the window
		if( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_MOUSE_FOCUS )
			SDL_WarpMouseInWindow( SDL_window, cls.glconfig.vidWidth / 2, cls.glconfig.vidHeight / 2 );

		mouseActive = qfalse;
	}
}

// We translate axes movement into keypresses
static int joy_keys[16] = {
	A_CURSOR_LEFT, A_CURSOR_RIGHT,
	A_CURSOR_UP, A_CURSOR_DOWN,
	A_JOY16, A_JOY17,
	A_JOY18, A_JOY19,
	A_JOY20, A_JOY21,
	A_JOY22, A_JOY23,
	A_JOY24, A_JOY25,
	A_JOY26, A_JOY27
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static int hat_keys[16] = {
	A_JOY28, A_JOY29,
	A_JOY30, A_JOY31,
	A_JOY24, A_JOY25,
	A_JOY26, A_JOY27,
	A_JOY20, A_JOY21,
	A_JOY22, A_JOY23,
	A_JOY16, A_JOY17,
	A_JOY18, A_JOY19
};


struct stick_state_s
{
	qboolean buttons[16];  // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	int oldaaxes[MAX_JOYSTICK_AXIS];
	unsigned int oldhats;
} stick_state;

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick( void )
{
	int i = 0;
	int total = 0;
	char buf[16384] = "";

	if (stick != NULL)
		SDL_JoystickClose(stick);

	stick = NULL;
	memset(&stick_state, '\0', sizeof (stick_state));

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
		{
			Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible joysticks\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
		Q_strcat(buf, sizeof(buf), "\n");
	}

	Cvar_Get( "in_availableJoysticks", buf, CVAR_ROM );

	if( !in_joystick->integer ) {
		Com_DPrintf( "Joystick is not active.\n" );
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		return;
	}

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", CVAR_ARCHIVE_ND );
	if( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
		Cvar_Set( "in_joystickNo", "0" );

	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "0", CVAR_ARCHIVE_ND );

	in_joystickThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE_ND );

	stick = SDL_JoystickOpen( in_joystickNo->integer );

	if (stick == NULL) {
		Com_DPrintf( "No joystick opened.\n" );
		return;
	}

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:       %s\n", SDL_JoystickNameForIndex(in_joystickNo->integer) );
	Com_DPrintf( "Axes:       %d\n", SDL_JoystickNumAxes(stick) );
	Com_DPrintf( "Hats:       %d\n", SDL_JoystickNumHats(stick) );
	Com_DPrintf( "Buttons:    %d\n", SDL_JoystickNumButtons(stick) );
	Com_DPrintf( "Balls:      %d\n", SDL_JoystickNumBalls(stick) );
	Com_DPrintf( "Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No" );
	Com_DPrintf( "Threshold: %f\n", in_joystickThreshold->value );

	SDL_JoystickEventState(SDL_QUERY);
}

void IN_Init( void *windowData )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
		return;
	}

	SDL_window = (SDL_Window *)windowData;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

	// joystick variables
	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE_ND );

	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE_ND|CVAR_LATCH );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE_ND );

	SDL_StartTextInput( );

	mouseAvailable = (qboolean)( in_mouse->value != 0 );
	if ( in_mouse->integer == 2 ) {
		Com_DPrintf( "Not using raw mouse input\n" );
		SDL_SetHint( "SDL_MOUSE_RELATIVE_MODE_WARP", "1" );
	}
	else {
		Com_DPrintf( "Using raw mouse input\n" );
		SDL_SetHint( "SDL_MOUSE_RELATIVE_MODE_WARP", "0" );
	}
	IN_DeactivateMouse( );

	int appState = SDL_GetWindowFlags( SDL_window );
	Cvar_SetValue( "com_unfocused", ( appState & SDL_WINDOW_INPUT_FOCUS ) == 0 );
	Cvar_SetValue( "com_minimized", ( appState & SDL_WINDOW_MINIMIZED ) != 0 );

	IN_InitJoystick( );
	Com_DPrintf( "------------------------------------\n" );
}

uint8_t ConvertUTF32ToExpectedCharset( uint32_t utf32 )
{
	switch ( utf32 )
	{
		// Cyrillic characters - mapped to Windows-1251 encoding
		case 0x0410: return 192;
		case 0x0411: return 193;
		case 0x0412: return 194;
		case 0x0413: return 195;
		case 0x0414: return 196;
		case 0x0415: return 197;
		case 0x0416: return 198;
		case 0x0417: return 199;
		case 0x0418: return 200;
		case 0x0419: return 201;
		case 0x041A: return 202;
		case 0x041B: return 203;
		case 0x041C: return 204;
		case 0x041D: return 205;
		case 0x041E: return 206;
		case 0x041F: return 207;
		case 0x0420: return 208;
		case 0x0421: return 209;
		case 0x0422: return 210;
		case 0x0423: return 211;
		case 0x0424: return 212;
		case 0x0425: return 213;
		case 0x0426: return 214;
		case 0x0427: return 215;
		case 0x0428: return 216;
		case 0x0429: return 217;
		case 0x042A: return 218;
		case 0x042B: return 219;
		case 0x042C: return 220;
		case 0x042D: return 221;
		case 0x042E: return 222;
		case 0x042F: return 223;
		case 0x0430: return 224;
		case 0x0431: return 225;
		case 0x0432: return 226;
		case 0x0433: return 227;
		case 0x0434: return 228;
		case 0x0435: return 229;
		case 0x0436: return 230;
		case 0x0437: return 231;
		case 0x0438: return 232;
		case 0x0439: return 233;
		case 0x043A: return 234;
		case 0x043B: return 235;
		case 0x043C: return 236;
		case 0x043D: return 237;
		case 0x043E: return 238;
		case 0x043F: return 239;
		case 0x0440: return 240;
		case 0x0441: return 241;
		case 0x0442: return 242;
		case 0x0443: return 243;
		case 0x0444: return 244;
		case 0x0445: return 245;
		case 0x0446: return 246;
		case 0x0447: return 247;
		case 0x0448: return 248;
		case 0x0449: return 249;
		case 0x044A: return 250;
		case 0x044B: return 251;
		case 0x044C: return 252;
		case 0x044D: return 253;
		case 0x044E: return 254;
		case 0x044F: return 255;

		// Eastern european characters - polish, czech, etc use Windows-1250 encoding
		case 0x0160: return 138;
		case 0x015A: return 140;
		case 0x0164: return 141;
		case 0x017D: return 142;
		case 0x0179: return 143;
		case 0x0161: return 154;
		case 0x015B: return 156;
		case 0x0165: return 157;
		case 0x017E: return 158;
		case 0x017A: return 159;
		case 0x0141: return 163;
		case 0x0104: return 165;
		case 0x015E: return 170;
		case 0x017B: return 175;
		case 0x0142: return 179;
		case 0x0105: return 185;
		case 0x015F: return 186;
		case 0x013D: return 188;
		case 0x013E: return 190;
		case 0x017C: return 191;
		case 0x0154: return 192;
		case 0x00C1: return 193;
		case 0x00C2: return 194;
		case 0x0102: return 195;
		case 0x00C4: return 196;
		case 0x0139: return 197;
		case 0x0106: return 198;
		case 0x00C7: return 199;
		case 0x010C: return 200;
		case 0x00C9: return 201;
		case 0x0118: return 202;
		case 0x00CB: return 203;
		case 0x011A: return 204;
		case 0x00CD: return 205;
		case 0x00CE: return 206;
		case 0x010E: return 207;
		case 0x0110: return 208;
		case 0x0143: return 209;
		case 0x0147: return 210;
		case 0x00D3: return 211;
		case 0x00D4: return 212;
		case 0x0150: return 213;
		case 0x00D6: return 214;
		case 0x0158: return 216;
		case 0x016E: return 217;
		case 0x00DA: return 218;
		case 0x0170: return 219;
		case 0x00DC: return 220;
		case 0x00DD: return 221;
		case 0x0162: return 222;
		case 0x00DF: return 223;
		case 0x0155: return 224;
		case 0x00E1: return 225;
		case 0x00E2: return 226;
		case 0x0103: return 227;
		case 0x00E4: return 228;
		case 0x013A: return 229;
		case 0x0107: return 230;
		case 0x00E7: return 231;
		case 0x010D: return 232;
		case 0x00E9: return 233;
		case 0x0119: return 234;
		case 0x00EB: return 235;
		case 0x011B: return 236;
		case 0x00ED: return 237;
		case 0x00EE: return 238;
		case 0x010F: return 239;
		case 0x0111: return 240;
		case 0x0144: return 241;
		case 0x0148: return 242;
		case 0x00F3: return 243;
		case 0x00F4: return 244;
		case 0x0151: return 245;
		case 0x00F6: return 246;
		case 0x0159: return 248;
		case 0x016F: return 249;
		case 0x00FA: return 250;
		case 0x0171: return 251;
		case 0x00FC: return 252;
		case 0x00FD: return 253;
		case 0x0163: return 254;
		case 0x02D9: return 255;

		default: return (uint8_t)utf32;
	}
}

/*
===============
IN_ProcessEvents
===============
*/
void SNDDMA_Activate( qboolean activate );
static void IN_ProcessEvents( void )
{
	SDL_Event e;
	fakeAscii_t key = A_NULL;
	static fakeAscii_t lastKeyDown = A_NULL;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
			return;

	while( SDL_PollEvent( &e ) )
	{
		switch( e.type )
		{
			case SDL_KEYDOWN:
				key = IN_TranslateSDLToJKKey( &e.key.keysym, qtrue );
				if ( key != A_NULL )
					Sys_QueEvent( 0, SE_KEY, key, qtrue, 0, NULL );

				if ( key == A_BACKSPACE )
					Sys_QueEvent( 0, SE_CHAR, CTRL('h'), qfalse, 0, NULL);
				else if ( kg.keys[A_CTRL].down && key >= A_CAP_A && key <= A_CAP_Z )
					Sys_QueEvent( 0, SE_CHAR, CTRL(tolower(key)), qfalse, 0, NULL );

				lastKeyDown = key;
				break;

			case SDL_KEYUP:
				key = IN_TranslateSDLToJKKey( &e.key.keysym, qfalse );
				if( key != A_NULL )
					Sys_QueEvent( 0, SE_KEY, key, qfalse, 0, NULL );

				lastKeyDown = A_NULL;
				break;

			case SDL_TEXTINPUT:
				if( lastKeyDown != A_CONSOLE )
				{
					char *c = e.text.text;

					// Quick and dirty UTF-8 to UTF-32 conversion
					while( *c )
					{
						uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
						if( utf32 != 0 )
						{
							if( IN_IsConsoleKey( A_NULL, utf32 ) )
							{
								Sys_QueEvent( 0, SE_KEY, A_CONSOLE, qtrue, 0, NULL );
								Sys_QueEvent( 0, SE_KEY, A_CONSOLE, qfalse, 0, NULL );
							}
							else
							{
								uint8_t encoded = ConvertUTF32ToExpectedCharset( utf32 );
								Sys_QueEvent( 0, SE_CHAR, encoded, 0, 0, NULL );
							}
						}
					}
				}
				break;

			case SDL_MOUSEMOTION:
				if ( mouseActive )
				{
					if ( !e.motion.xrel && !e.motion.yrel )
						break;
					Sys_QueEvent( 0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					unsigned short b;
					switch( e.button.button )
					{
						case SDL_BUTTON_LEFT:	b = A_MOUSE1;     break;
						case SDL_BUTTON_MIDDLE:	b = A_MOUSE3;     break;
						case SDL_BUTTON_RIGHT:	b = A_MOUSE2;     break;
						case SDL_BUTTON_X1:		b = A_MOUSE4;     break;
						case SDL_BUTTON_X2:		b = A_MOUSE5;     break;
						default: b = A_AUX0 + ( e.button.button - 6 ) % 32; break;
					}
					Sys_QueEvent( 0, SE_KEY, b,
						( e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse ), 0, NULL );
				}
				break;

			case SDL_MOUSEWHEEL:
				if( e.wheel.y > 0 )
				{
					Sys_QueEvent( 0, SE_KEY, A_MWHEELUP, qtrue, 0, NULL );
					Sys_QueEvent( 0, SE_KEY, A_MWHEELUP, qfalse, 0, NULL );
				}
				else if( e.wheel.y < 0 )
				{
					Sys_QueEvent( 0, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL );
					Sys_QueEvent( 0, SE_KEY, A_MWHEELDOWN, qfalse, 0, NULL );
				}
				break;

			case SDL_QUIT:
				Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
					{
						Cvar_SetValue( "com_unfocused", 1 );
						SNDDMA_Activate( qfalse );
						break;
					}

					case SDL_WINDOWEVENT_FOCUS_GAINED:
					{
						Cvar_SetValue( "com_unfocused", 0 );
						SNDDMA_Activate( qtrue );
						break;
					}
				}
				break;

			default:
				break;
		}
	}
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove( void )
{
	unsigned int axes = 0;
	unsigned int hats = 0;
	int total = 0;
	int i = 0;

	if (!stick)
		return;

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		for (i = 0; i < total; i++)
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
				balldx *= 2;
			if (abs(balldy) > 1)
				balldy *= 2;
			Sys_QueEvent( 0, SE_MOUSE, balldx, balldy, 0, NULL );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > (int)ARRAY_LEN(stick_state.buttons))
			total = ARRAY_LEN(stick_state.buttons);
		for (i = 0; i < total; i++)
		{
			qboolean pressed = (qboolean)(SDL_JoystickGetButton(stick, i) != 0);
			if (pressed != stick_state.buttons[i])
			{
				Sys_QueEvent( 0, SE_KEY, A_JOY1 + i, pressed, 0, NULL );
				stick_state.buttons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4) total = 4;
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldhats)
	{
		for( i = 0; i < 4; i++ ) {
			if( ((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldhats)[i] ) {
				// release event
				switch( ((Uint8 *)&stick_state.oldhats)[i] ) {
					case SDL_HAT_UP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					default:
						break;
				}
				// press event
				switch( ((Uint8 *)&hats)[i] ) {
					case SDL_HAT_UP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (in_joystickUseAnalog->integer)
		{
			if (total > MAX_JOYSTICK_AXIS) total = MAX_JOYSTICK_AXIS;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) abs(axis) ) / 32767.0f;

				if( f < in_joystickThreshold->value ) axis = 0;

				if ( axis != stick_state.oldaaxes[i] )
				{
					Sys_QueEvent( 0, SE_JOYSTICK_AXIS, i, axis, 0, NULL );
					stick_state.oldaaxes[i] = axis;
				}
			}
		}
		else
		{
			if (total > 16) total = 16;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) axis ) / 32767.0f;
				if( f < -in_joystickThreshold->value ) {
					axes |= ( 1 << ( i * 2 ) );
				} else if( f > in_joystickThreshold->value ) {
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if (axes != stick_state.oldaxes)
	{
		for( i = 0; i < 16; i++ ) {
			if( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) ) {
				Sys_QueEvent( 0, SE_KEY, joy_keys[i], qtrue, 0, NULL );
			}

			if( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) ) {
				Sys_QueEvent( 0, SE_KEY, joy_keys[i], qfalse, 0, NULL );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}


void IN_Frame (void) {
	qboolean loading;

	IN_JoyMove( );

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = (qboolean)( cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE );

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !cls.glconfig.isFullscreen && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( );
	}
	else if( !( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window not got focus
		IN_DeactivateMouse( );
	}
	else
		IN_ActivateMouse( );

	IN_ProcessEvents( );
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick( void )
{
	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
		return;

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void IN_Shutdown( void ) {
	SDL_StopTextInput( );

	IN_DeactivateMouse( );
	mouseAvailable = qfalse;

	IN_ShutdownJoystick( );

	SDL_window = NULL;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
	IN_ShutdownJoystick( );
	IN_Init( SDL_window );
}
