// win_local.h: Win32-specific Quake3 header file
#pragma once
#error Deprecated file

#include "qcommon/qcommon.h"

#define DIRECTINPUT_VERSION 0x0800  //[ 0x0300 | 0x0500 | 0x0700 | 0x0800 ]
#include <dinput.h>
#include <dsound.h>

void	IN_MouseEvent (int mstate);
void	IN_RawMouseEvent( int lastX, int lastY ); // Send raw input events to the input subsystem

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

void	Sys_CreateConsole( void );
void	Sys_DestroyConsole( void );

char	*Sys_ConsoleInput (void);

qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );

// Input subsystem

void	IN_Init (void);
void	IN_Shutdown (void);
void	IN_JoystickCommands (void);

void	IN_Move (usercmd_t *cmd);
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void	IN_DeactivateWin32Mouse( void);

void	IN_Activate (qboolean active);
void	IN_Frame (void);

// window procedure
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

void Conbuf_AppendText( const char *msg );

void SNDDMA_Activate( qboolean bAppActive );
int  SNDDMA_InitDS ();

typedef struct WinVars_s {

	HINSTANCE		reflib_library;		// Handle to refresh DLL
	qboolean		reflib_active;

	HWND			hWnd;
	HINSTANCE		hInstance;
	qboolean		activeApp;
	qboolean		isMinimized;
	OSVERSIONINFO	osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned		sysMsgTime;
} WinVars_t;

extern WinVars_t	g_wv;


#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

