//-----------------------------------------------------------------------------
// File: dbg_console_xbox.cpp
//
// Desc: Listens for string commands sent from a debug console on a
//       remote dev machine, and forwards them to the Q3 engine.
//
//       Commands are sent from the remote debug console through the debug
//       channel to the debug monitor on the Xbox machine.  The Xbox machine
//       receives the commands on a separate thread through a
//       registered command processor callback function. The callback
//       function will store commands in a buffer, and the app should
//       poll this buffer once per frame and then decipher and handle
//       the commands.
//
// Hist: 02.05.01 - Initial creation for March XDK release
//       08.21.02 - Revision and code cleanup
//       04.10.02 - Buthcered by BTO for use in JK3:JA
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include <xbdm.h>
#include <stdio.h>
#include "dbg_console_xbox.h"
#include "../client/client.h"
#include "../qcommon/qcommon.h"


// Command prefix for things sent across the dubg channel
static const CHAR g_strDebugConsoleCommandPrefix[] = "XCMD";


// Global buffer to receive remote commands from the debug console. Note that
// since this data is accessed by the app's main thread, and the debug monitor
// thread, we need to protect access with a critical section
static CHAR g_strRemoteBuf[MAXRCMDLENGTH];


// The critical section used to protect data that is shared between threads
static CRITICAL_SECTION g_CriticalSection;


// Temporary replacement for CRT string funcs, since
// we can't call CRT functions on the debug monitor
// thread right now.


//-----------------------------------------------------------------------------
// Name: dbgtolower()
// Desc: Returns lowercase of char
//-----------------------------------------------------------------------------
inline CHAR dbgtolower( CHAR ch )
{
    if( ch >= 'A' && ch <= 'Z' )
        return ch - ( 'A' - 'a' );
    else
        return ch;
}


//-----------------------------------------------------------------------------
// Name: dbgstrnicmp()
// Desc: Critical section safe string compare.
//-----------------------------------------------------------------------------
BOOL dbgstrnicmp( const CHAR* str1, const CHAR* str2, int n )
{
    while( ( dbgtolower( *str1 ) == dbgtolower( *str2 ) ) && *str1 && n > 0 )
    {
        --n;
        ++str1;
        ++str2;
    }

    return( n == 0 || dbgtolower( *str1 ) == dbgtolower( *str2 ) );
}


//-----------------------------------------------------------------------------
// Name: dbgstrcpy()
// Desc: Critical section safe string copy
//-----------------------------------------------------------------------------
VOID dbgstrcpy( CHAR* strDest, const CHAR* strSrc )
{
    while( ( *strDest++ = *strSrc++ ) != 0 );
}
    

//-----------------------------------------------------------------------------
// Name: DebugConsoleCmdProcessor()
// Desc: Command notification proc that is called by the Xbox debug monitor to
//       have us process a command.  What we'll actually attempt to do is tell
//       it to make calls to us on a separate thread, so that we can just block
//       until we're able to process a command.
//
// Note: Do NOT include newlines in the response string! To do so will confuse
//       the internal WinSock networking code used by the debug monitor API.
//-----------------------------------------------------------------------------
HRESULT __stdcall DebugConsoleCmdProcessor( const CHAR* strCommand,
                                            CHAR* strResponse, DWORD dwResponseLen,
                                            PDM_CMDCONT pdmcc )
{
    // Skip over the command prefix and the exclamation mark
    strCommand += strlen(g_strDebugConsoleCommandPrefix) + 1;

    // Check if this is the initial connect signal
    if( dbgstrnicmp( strCommand, "__connect__", 11 ) )
    {
        // If so, respond that we're connected
        lstrcpynA( strResponse, "Connected.", dwResponseLen );
        return XBDM_NOERR;
    }

	// g_strRemoteBuf needs to be protected by the critical section
	EnterCriticalSection( &g_CriticalSection );
	if( g_strRemoteBuf[0] )
	{
		// This means the application has probably stopped polling for debug commands
		dbgstrcpy( strResponse, "Cannot execute - previous command still pending" );
	}
	else
	{
		dbgstrcpy( g_strRemoteBuf, strCommand );
	}
	LeaveCriticalSection( &g_CriticalSection );

    return XBDM_NOERR;
}


//-----------------------------------------------------------------------------
// Name: DebugConsoleHandleCommands()
// Desc: Poll routine called periodically (typically every frame) by the Xbox
//       app to see if there is a command waiting to be executed, and if so,
//       execute it.
//-----------------------------------------------------------------------------
BOOL DebugConsoleHandleCommands()
{
    static BOOL bInitialized = FALSE;
    CHAR  strLocalBuf[MAXRCMDLENGTH+1]; // local copy of command

    // Initialize ourselves when we're first called.
    if( !bInitialized )
    {
        // Register our command handler with the debug monitor
        HRESULT hr = DmRegisterCommandProcessor( g_strDebugConsoleCommandPrefix, 
                                                 DebugConsoleCmdProcessor );
        if( FAILED(hr) )
            return FALSE;

        // We'll also need a critical section to protect access to g_strRemoteBuf
        InitializeCriticalSection( &g_CriticalSection );

        bInitialized = TRUE;
    }

    // If there's nothing waiting, return.
    if( !g_strRemoteBuf[0] )
        return FALSE;

    // Grab a local copy of the command received in the remote buffer
    EnterCriticalSection( &g_CriticalSection );

    lstrcpyA( strLocalBuf, g_strRemoteBuf );
    g_strRemoteBuf[0] = 0;

    LeaveCriticalSection( &g_CriticalSection );

	Cbuf_ExecuteText( EXEC_APPEND, va("%s\n", strLocalBuf) );

    return TRUE;
}

