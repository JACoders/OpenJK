#include "client/client.h"
#include "win_local.h"

WinVars_t	g_wv;

// The only directly referenced keycode - the console key (which gives different ascii codes depending on locale)
#define CONSOLE_SCAN_CODE	0x29

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported by the OS
#endif

static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
cvar_t			*vid_xpos;			// X coordinate of window position
cvar_t			*vid_ypos;			// Y coordinate of window position

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

/*
==================
VID_AppActivate
==================
*/
static void VID_AppActivate(BOOL fActive, BOOL minimize)
{
	g_wv.isMinimized = (qboolean)minimize;

	Com_DPrintf("VID_AppActivate: %i\n", fActive );

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if (fActive && !g_wv.isMinimized )
	{
		g_wv.activeApp = qtrue;
	}
	else
	{
		g_wv.activeApp = qfalse;
	}

	// minimize/restore mouse-capture on demand
	if (!g_wv.activeApp )
	{
		IN_Activate (qfalse);
	}
	else
	{
		IN_Activate (qtrue);
	}
}

//==========================================================================


static byte virtualKeyConvert[0x92][2] =
{
	{ 0,				0				},
	{ A_MOUSE1,			A_MOUSE1		}, // VK_LBUTTON 01 Left mouse button
	{ A_MOUSE2,			A_MOUSE2		}, // VK_RBUTTON 02 Right mouse button
	{ 0,				0				}, // VK_CANCEL 03 Control-break processing
	{ A_MOUSE3,			A_MOUSE3		}, // VK_MBUTTON 04 Middle mouse button (three-button mouse)
	{ A_MOUSE4,			A_MOUSE4		}, // VK_XBUTTON1 05 Windows 2000/XP: X1 mouse button
	{ A_MOUSE5,			A_MOUSE5		}, // VK_XBUTTON2 06 Windows 2000/XP: X2 mouse button
	{ 0,				0				}, // 07 Undefined
	{ A_BACKSPACE,		A_BACKSPACE		}, // VK_BACK 08 BACKSPACE key
	{ A_TAB,			A_TAB			}, // VK_TAB 09 TAB key
	{ 0,				0				}, // 0A Reserved
	{ 0,				0				}, // 0B Reserved
	{ A_KP_5,			0				}, // VK_CLEAR 0C CLEAR key
	{ A_ENTER, 			A_KP_ENTER 		}, // VK_RETURN 0D ENTER key
	{ 0,				0				}, // 0E Undefined
	{ 0,				0				}, // 0F Undefined
	{ A_SHIFT,			A_SHIFT			}, // VK_SHIFT 10 SHIFT key
	{ A_CTRL,			A_CTRL			}, // VK_CONTROL 11 CTRL key
	{ A_ALT,			A_ALT			}, // VK_MENU 12 ALT key
	{ A_PAUSE,			A_PAUSE			}, // VK_PAUSE 13 PAUSE key
	{ A_CAPSLOCK,		A_CAPSLOCK		}, // VK_CAPITAL 14 CAPS LOCK key
	{ 0,				0				}, // VK_KANA 15 IME Kana mode
	{ 0,				0				}, // 16 Undefined
	{ 0,				0				}, // VK_JUNJA 17 IME Junja mode
	{ 0,				0				}, // VK_FINAL 18 IME final mode
	{ 0,				0				}, // VK_KANJI 19 IME Kanji mode
	{ 0,				0				}, // 1A Undefined
	{ A_ESCAPE,			A_ESCAPE		}, // VK_ESCAPE 1B ESC key
	{ 0,				0				}, // VK_CONVERT 1C IME convert
	{ 0,				0				}, // VK_NONCONVERT 1D IME nonconvert
	{ 0,				0				}, // VK_ACCEPT 1E IME accept
	{ 0,				0				}, // VK_MODECHANGE 1F IME mode change request
	{ A_SPACE,			A_SPACE			}, // VK_SPACE 20 SPACEBAR
	{ A_KP_9,			A_PAGE_UP		}, // VK_PRIOR 21 PAGE UP key
	{ A_KP_3,			A_PAGE_DOWN		}, // VK_NEXT 22 PAGE DOWN key
	{ A_KP_1,			A_END			}, // VK_END 23 END key
	{ A_KP_7,			A_HOME			}, // VK_HOME 24 HOME key
	{ A_KP_4,			A_CURSOR_LEFT	}, // VK_LEFT 25 LEFT ARROW key
	{ A_KP_8,			A_CURSOR_UP   	}, // VK_UP 26 UP ARROW key
	{ A_KP_6,			A_CURSOR_RIGHT	}, // VK_RIGHT 27 RIGHT ARROW key
	{ A_KP_2,			A_CURSOR_DOWN	}, // VK_DOWN 28 DOWN ARROW key
	{ 0,				0				}, // VK_SELECT 29 SELECT key
	{ 0,				0				}, // VK_PRINT 2A PRINT key
	{ 0,				0				}, // VK_EXECUTE 2B EXECUTE key
	{ A_PRINTSCREEN,	A_PRINTSCREEN	}, // VK_SNAPSHOT 2C PRINT SCREEN key
	{ A_KP_0,			A_INSERT		}, // VK_INSERT 2D INS key
	{ A_KP_PERIOD,		A_DELETE		}, // VK_DELETE 2E DEL key
	{ 0,				0				}, // VK_HELP 2F HELP key
	{ A_0,				A_0				}, // 30 0 key
	{ A_1,				A_1				}, // 31 1 key
	{ A_2,				A_2				}, // 32 2 key
	{ A_3,				A_3				}, // 33 3 key
	{ A_4,				A_4				}, // 34 4 key
	{ A_5,				A_5				}, // 35 5 key
	{ A_6,				A_6				}, // 36 6 key
	{ A_7,				A_7				}, // 37 7 key
	{ A_8,				A_8				}, // 38 8 key
	{ A_9,				A_9				}, // 39 9 key
	{ 0,				0				}, // 3A Undefined
	{ 0,				0				}, // 3B Undefined
	{ 0,				0				}, // 3C Undefined
	{ 0,				0				}, // 3D Undefined
	{ 0,				0				}, // 3E Undefined
	{ 0,				0				}, // 3F Undefined
	{ 0,				0				}, // 40 Undefined
	{ A_CAP_A,			A_CAP_A			}, // 41 A key
	{ A_CAP_B,			A_CAP_B			}, // 42 B key
	{ A_CAP_C,			A_CAP_C			}, // 43 C key
	{ A_CAP_D,			A_CAP_D			}, // 44 D key
	{ A_CAP_E,			A_CAP_E			}, // 45 E key
	{ A_CAP_F,			A_CAP_F			}, // 46 F key
	{ A_CAP_G,			A_CAP_G			}, // 47 G key
	{ A_CAP_H,			A_CAP_H			}, // 48 H key
	{ A_CAP_I,			A_CAP_I			}, // 49 I key
	{ A_CAP_J,			A_CAP_J			}, // 4A J key
	{ A_CAP_K,			A_CAP_K			}, // 4B K key
	{ A_CAP_L,			A_CAP_L			}, // 4C L key
	{ A_CAP_M,			A_CAP_M			}, // 4D M key
	{ A_CAP_N,			A_CAP_N			}, // 4E N key
	{ A_CAP_O,			A_CAP_O			}, // 4F O key
	{ A_CAP_P,			A_CAP_P			}, // 50 P key
	{ A_CAP_Q,			A_CAP_Q			}, // 51 Q key
	{ A_CAP_R,			A_CAP_R			}, // 52 R key
	{ A_CAP_S,			A_CAP_S			}, // 53 S key
	{ A_CAP_T,			A_CAP_T			}, // 54 T key
	{ A_CAP_U,			A_CAP_U			}, // 55 U key
	{ A_CAP_V,			A_CAP_V			}, // 56 V key
	{ A_CAP_W,			A_CAP_W			}, // 57 W key
	{ A_CAP_X,			A_CAP_X			}, // 58 X key
	{ A_CAP_Y,			A_CAP_Y			}, // 59 Y key
	{ A_CAP_Z,			A_CAP_Z			}, // 5A Z key
	{ 0,				0				}, // VK_LWIN 5B Left Windows key (Microsoft® Natural® keyboard)
	{ 0,				0				}, // VK_RWIN 5C Right Windows key (Natural keyboard)
	{ 0,				0				}, // VK_APPS 5D Applications key (Natural keyboard)
	{ 0,				0				}, // 5E Reserved
	{ 0,				0				}, // VK_SLEEP 5F Computer Sleep key
	{ A_KP_0,			A_KP_0			}, // VK_NUMPAD0 60 Numeric keypad 0 key
	{ A_KP_1,			A_KP_1			}, // VK_NUMPAD1 61 Numeric keypad 1 key
	{ A_KP_2,			A_KP_2			}, // VK_NUMPAD2 62 Numeric keypad 2 key
	{ A_KP_3,			A_KP_3			}, // VK_NUMPAD3 63 Numeric keypad 3 key
	{ A_KP_4,			A_KP_4			}, // VK_NUMPAD4 64 Numeric keypad 4 key
	{ A_KP_5,			A_KP_5			}, // VK_NUMPAD5 65 Numeric keypad 5 key
	{ A_KP_6,			A_KP_6			}, // VK_NUMPAD6 66 Numeric keypad 6 key
	{ A_KP_7,			A_KP_7			}, // VK_NUMPAD7 67 Numeric keypad 7 key
	{ A_KP_8,			A_KP_8			}, // VK_NUMPAD8 68 Numeric keypad 8 key
	{ A_KP_9,			A_KP_9			}, // VK_NUMPAD9 69 Numeric keypad 9 key
	{ A_MULTIPLY,		A_MULTIPLY		}, // VK_MULTIPLY 6A Multiply key
	{ A_KP_PLUS, 		A_KP_PLUS 		}, // VK_ADD 6B Add key
	{ 0,				0				}, // VK_SEPARATOR 6C Separator key
	{ A_KP_MINUS,		A_KP_MINUS		}, // VK_SUBTRACT 6D Subtract key
	{ A_KP_PERIOD,		A_KP_PERIOD		}, // VK_DECIMAL 6E Decimal key
	{ A_DIVIDE,			A_DIVIDE		}, // VK_DIVIDE 6F Divide key
	{ A_F1,				A_F1			}, // VK_F1 70 F1 key
	{ A_F2,				A_F2			}, // VK_F2 71 F2 key
	{ A_F3,				A_F3			}, // VK_F3 72 F3 key
	{ A_F4,				A_F4			}, // VK_F4 73 F4 key
	{ A_F5,				A_F5			}, // VK_F5 74 F5 key
	{ A_F6,				A_F6			}, // VK_F6 75 F6 key
	{ A_F7,				A_F7			}, // VK_F7 76 F7 key
	{ A_F8,				A_F8			}, // VK_F8 77 F8 key
	{ A_F9,				A_F9			}, // VK_F9 78 F9 key
	{ A_F10,			A_F10			}, // VK_F10 79 F10 key
	{ A_F11,			A_F11			}, // VK_F11 7A F11 key
	{ A_F12,			A_F12			}, // VK_F12 7B F12 key
	{ 0,				0				}, // VK_F13 7C F13 key
	{ 0,				0				}, // VK_F14 7D F14 key
	{ 0,				0				}, // VK_F15 7E F15 key
	{ 0,				0				}, // VK_F16 7F F16 key
	{ 0,				0				}, // VK_F17 80H F17 key
	{ 0,				0				}, // VK_F18 81H F18 key
	{ 0,				0				}, // VK_F19 82H F19 key
	{ 0,				0				}, // VK_F20 83H F20 key
	{ 0,				0				}, // VK_F21 84H F21 key
	{ 0,				0				}, // VK_F22 85H F22 key
	{ 0,				0				}, // VK_F23 86H F23 key
	{ 0,				0				}, // VK_F24 87H F24 key
	{ 0,				0				}, // 88 Unassigned
	{ 0,				0				}, // 89 Unassigned
	{ 0,				0				}, // 8A Unassigned
	{ 0,				0				}, // 8B Unassigned
	{ 0,				0				}, // 8C Unassigned
	{ 0,				0				}, // 8D Unassigned
	{ 0,				0				}, // 8E Unassigned
	{ 0,				0				}, // 8F Unassigned
	{ A_NUMLOCK,		A_NUMLOCK		}, // VK_NUMLOCK 90 NUM LOCK key
	{ A_SCROLLLOCK,		A_SCROLLLOCK	}  // VK_SCROLL 91
};

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
static int MapKey (ulong key, word wParam)
{
	ulong	result, scan, extended;

	// Check for the console key (hard code to the key you would expect)
	scan = ( key >> 16 ) & 0xff;
	if(scan == CONSOLE_SCAN_CODE)
	{
		return(A_CONSOLE);
	}

	// Try to convert the virtual key directly
	result = 0;
	extended = (key >> 24) & 1;
	if(wParam > 0 && wParam <= VK_SCROLL)
	{
		// yeuch, but oh well...
		//
		if ( wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9 )
		{
			bool bNumlockOn = !!(GetKeyState( VK_NUMLOCK ) & 1);
			if ( bNumlockOn )
			{
				wParam = 0x30 + (wParam - VK_NUMPAD0);	// convert to standard 0..9
			}
		}
		result = virtualKeyConvert[wParam][extended];
	}
	// Get the unshifted ascii code (if any)
	if(!result)
	{
		result = MapVirtualKey(wParam, 2) & 0xff;
	}
	// Output any debug prints
//	if(in_debug && in_debug->integer & 1)
//	{
//		Com_Printf("WM_KEY: %x : %x : %x\n", key, wParam, result);
//	}
	return(result);
}


/*
====================
MainWndProc

main window procedure
====================
*/

#define WM_BUTTON4DOWN	(WM_MOUSELAST+2)
#define WM_BUTTON4UP	(WM_MOUSELAST+3)
#define MK_BUTTON4L		0x0020
#define MK_BUTTON4R		0x0040

LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	byte code;

	if ( uMsg == MSH_MOUSEWHEEL )
	{
		if ( ( ( int ) wParam ) > 0 )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELUP, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELUP, qfalse, 0, NULL );
		}
		else
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELDOWN, qfalse, 0, NULL );
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		//
		//
		// this chunk of code theoretically only works under NT4 and Win98
		// since this message doesn't exist under Win95
		//
		if ( ( short ) HIWORD( wParam ) > 0 )
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELUP, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELUP, qfalse, 0, NULL );
		}
		else
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELDOWN, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, A_MWHEELDOWN, qfalse, 0, NULL );
		}
		break;

	case WM_CREATE:

		g_wv.hWnd = hWnd;

		vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
		vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);

		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG");

		break;
	case WM_DESTROY:
		// let sound and input know about this?
		g_wv.hWnd = NULL;
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
		break;

	case WM_ACTIVATE:
		{
			int	fActive, fMinimized;

			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			Cvar_SetValue( "com_unfocused",	(fActive == WA_INACTIVE));
			Cvar_SetValue( "com_minimized", fMinimized);

			VID_AppActivate( fActive != WA_INACTIVE, fMinimized);
			SNDDMA_Activate( (qboolean)(fActive != WA_INACTIVE && !fMinimized) );
		}
		break;

	case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;
			int		style;

			if (!Cvar_VariableIntegerValue("r_fullscreen") )
			{
				xPos = (short) LOWORD(lParam);    // horizontal position
				yPos = (short) HIWORD(lParam);    // vertical position

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLongPtr( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				Cvar_SetValue( "vid_xpos", xPos + r.left);
				Cvar_SetValue( "vid_ypos", yPos + r.top);
				vid_xpos->modified = qfalse;
				vid_ypos->modified = qfalse;
				if ( g_wv.activeApp )
				{
					IN_Activate (qtrue);
				}
			}
		}
		break;

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_BUTTON4DOWN:
	case WM_BUTTON4UP:
		{
			int	temp;

			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

		 	if (wParam & MK_BUTTON4L)
				temp |= 8;

			if (wParam & MK_BUTTON4R)
				temp |= 16;

			IN_MouseEvent (temp);
		}
		break;

	case WM_INPUT:
		{
			UINT rawSize;
			if ( GetRawInputData( (HRAWINPUT) lParam, RID_INPUT, NULL, &rawSize, sizeof(RAWINPUTHEADER) ) == -1 )
				break;

			RAWINPUT raw;
			if ( GetRawInputData( (HRAWINPUT) lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER) ) != rawSize )
				break;

			if ( ( raw.header.dwType != RIM_TYPEMOUSE ) || ( raw.data.mouse.usFlags != MOUSE_MOVE_RELATIVE ) )
				break;

			int xPosRelative = raw.data.mouse.lLastX;
			int yPosRelative = raw.data.mouse.lLastY;
			IN_RawMouseEvent( xPosRelative, yPosRelative );
		}
		break;

	case WM_SYSCOMMAND:
		if ( (wParam&0xFFF0) == SC_SCREENSAVE || (wParam&0xFFF0) == SC_MONITORPOWER || (wParam&0xFFF0) == SC_KEYMENU )
		{
			return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		if ( wParam == VK_RETURN )
		{
			if ( cl_allowAltEnter && cl_allowAltEnter->integer )
			{
				Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue("r_fullscreen") );
				Cbuf_AddText( "vid_restart\n" );
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		code = MapKey( lParam, wParam );
		if(code)
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, code, qtrue, 0, NULL );
		}
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		code = MapKey( lParam, wParam );
		if(code)
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, code, qfalse, 0, NULL );
		}
		break;

	case WM_CHAR:
		if(((lParam >> 16) & 0xff) != CONSOLE_SCAN_CODE)
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL );
		}
		// Output any debug prints
//		if(in_debug && in_debug->integer & 2)
//		{
//			Com_Printf("WM_CHAR: %x\n", wParam);
//		}
		break;

	case WM_POWERBROADCAST:
		if (wParam == PBT_APMQUERYSUSPEND)
		{
#ifndef FINAL_BUILD
			Com_Printf("Cannot go into hibernate / standby mode while game is running!\n");
#endif
			return BROADCAST_QUERY_DENY;
		}
		break;
   }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

