//-----------------------------------------------------------------------------
// File: dbg_console_xbox.h
//
// Desc: Header file for communicating with a remote debug console. Please read
//       the comments in the dbg_console_xbox.cpp file for more info
//       on the API.
//
//       This header defines the following arrays:
//
//          g_RemoteCommands -  This is the list of commands your application provides.
//                              Note that "help" and "set" are provided automatically
//                              This is implemented in DebugCmd.cpp
//
//          g_RemoteVariables - This is a list of variables that your application
//                              exposes. They can be examined and modified by the
//                              remote debug console with the "set" command. 
//                              This is implemented in DebugChannel.cpp
//
// Hist: 02.05.01 - Initial creation for March XDK release
//       08.21.02 - Revision and code cleanup
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef DEBUGCMD_H
#define DEBUGCMD_H

#define MAXRCMDLENGTH       256                 // Size of the remote cmd buffer

// Handle any remote commands that have been sent - this should be called
// periodically by the application
BOOL DebugConsoleHandleCommands();

#endif // DEBUGCMD_H

