/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../qcommon/sstring.h"	// to get Gil's string class, because MS's doesn't compile properly in here
#include "stv_version.h"

#ifdef _XBOX
#include "../win32/win_file.h"
#include "../ui/ui_splash.h"
#endif

#ifndef FINAL_BUILD
#include "platform.h"
#endif

#define	MAXPRINTMSG	4096

#define MAX_NUM_ARGVS	50

int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

#ifndef _XBOX
static fileHandle_t	logfile;
static fileHandle_t	speedslog;
static fileHandle_t	camerafile;
fileHandle_t	com_journalFile;
fileHandle_t	com_journalDataFile;		// config files are written here
#endif

cvar_t	*com_viewlog;
cvar_t	*com_speeds;
cvar_t	*com_developer;
cvar_t	*com_timescale;
cvar_t	*com_fixedtime;
cvar_t	*com_maxfps;
cvar_t	*com_sv_running;
cvar_t	*com_cl_running;
cvar_t	*com_logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*com_showtrace;
cvar_t	*com_terrainPhysics;
cvar_t	*com_version;
cvar_t	*com_buildScript;	// for automated data building scripts
cvar_t	*cl_paused;
cvar_t	*sv_paused;
cvar_t	*com_skippingcin;
cvar_t	*com_speedslog;		// 1 = buffer log, 2 = flush after each print

// Support for JK2 binaries --eez
cvar_t	*com_jk2;			// searches for jk2gamex86.dll instead of jagamex86.dll

#ifdef G2_PERFORMANCE_ANALYSIS
cvar_t	*com_G2Report;
#endif


// com_speeds times
int		time_game;
int		time_frontend;		// renderer frontend time
int		time_backend;		// renderer backend time

int		timeInTrace;
int		timeInPVSCheck;
int		numTraces;

int			com_frameTime;
int			com_frameMsec;
int			com_frameNumber = 0;

qboolean	com_errorEntered;
qboolean	com_fullyInitialized = qfalse;

char	com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f( void );
//JLF
//void G_DemoFrame();

//============================================================================

#ifndef _XBOX
static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)( char *buffer );

void Com_BeginRedirect (char *buffer, int buffersize, void (*flush)( char *) )
{
	if (!buffer || !buffersize || !flush)
		return;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	if ( rd_flush ) {
		rd_flush(rd_buffer);
	}

	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}
#ifndef FINAL_BUILD
#define OUTPUT_TO_BUILD_WINDOW
#endif
#endif	//not xbox

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsprintf_s (msg,fmt,argptr);
	va_end (argptr);

	if ( rd_buffer ) {
		if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1)) {
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat (rd_buffer, strlen(rd_buffer), msg);
		return;
	}

	CL_ConsolePrint( msg );

	// echo to dedicated console and early console
	Sys_Print( msg );

#ifdef OUTPUT_TO_BUILD_WINDOW
	OutputDebugString(msg);
#endif

	// logfile
	if ( com_logfile && com_logfile->integer ) {
		if ( !logfile ) {
			logfile = FS_FOpenFileWrite( "qconsole.log" );
			if ( com_logfile->integer > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}
		}
		if ( logfile ) {
			FS_Write(msg, strlen(msg), logfile);
		}
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf( const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if ( !com_developer || !com_developer->integer ) {
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start (argptr,fmt);
	vsprintf_s (msg,fmt,argptr);
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}

void Com_WriteCam ( const char *text )
{
#ifndef _XBOX
	static	char	mapname[MAX_QPATH];
	// camerafile
	if ( !camerafile ) 
	{
		extern	cvar_t	*sv_mapname;

		//NOTE: always saves in working dir if using one...
		Com_sprintf( mapname, MAX_QPATH, "maps/%s_cam.map", sv_mapname->string );
		camerafile = FS_FOpenFileWrite( mapname );
	}

	if ( camerafile ) 
	{
		FS_Printf( camerafile, "%s", text );
	}

	Com_Printf( "%s\n", mapname );
#endif
}

void Com_FlushCamFile()
{
#ifndef _XBOX
	if (!camerafile)
	{
		// nothing to flush, right?
		Com_Printf("No cam file available\n");
		return;
	}
	FS_ForceFlush(camerafile);
	FS_FCloseFile (camerafile);
	camerafile = 0;

	static	char	flushedMapname[MAX_QPATH];
	extern	cvar_t	*sv_mapname;
	Com_sprintf( flushedMapname, MAX_QPATH, "maps/%s_cam.map", sv_mapname->string );
	Com_Printf("flushed all cams to %s\n", flushedMapname);
#endif
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/

void SG_WipeSavegame(const char *name);	// pretty sucky, but that's how SoF did it...<g>
void SG_Shutdown();
//void SCR_UnprecacheScreenshot();

void QDECL Com_Error( int code, const char *fmt, ... ) {
	va_list		argptr;

#if defined(_WIN32) && defined(_DEBUG)
	if ( code != ERR_DISCONNECT ) {
//		if (com_noErrorInterrupt && !com_noErrorInterrupt->integer) 
		{
			__asm {
				int 0x03
			}
		}
	}
#endif

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {
		code = ERR_FATAL;
	}

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	
	com_errorEntered = qtrue;

	//reset some game stuff here
//	SCR_UnprecacheScreenshot();

	va_start (argptr,fmt);
	vsprintf_s (com_errorMessage,fmt,argptr);
	va_end (argptr);	

	if ( code != ERR_DISCONNECT ) {
		Cvar_Get("com_errorMessage", "", CVAR_ROM);	//give com_errorMessage a default so it won't come back to life after a resetDefaults
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	SG_Shutdown();				// close any file pointers
	if ( code == ERR_DISCONNECT ) {
		SV_Shutdown("Disconnect");
		CL_Disconnect();
		CL_FlushMemory();
		CL_StartHunkUsers();
		com_errorEntered = qfalse;
		throw ("DISCONNECTED\n");
	} else if ( code == ERR_DROP ) {
		// If loading/saving caused the crash/error - delete the temp file
		SG_WipeSavegame("current");	// delete file

		SV_Shutdown (va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect();
		CL_FlushMemory();
		CL_StartHunkUsers();
		Com_Printf (S_COLOR_RED"********************\n"S_COLOR_MAGENTA"ERROR: %s\n"S_COLOR_RED"********************\n", com_errorMessage);
		com_errorEntered = qfalse;
		throw ("DROPPED\n");
	} else {
		CL_Shutdown ();
		SV_Shutdown (va(S_COLOR_RED"Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void ) {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown ("Server quit\n");
		CL_Shutdown ();
		Com_Shutdown ();
	}
	Sys_Quit ();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define	MAX_CONSOLE_LINES	32
int		com_numConsoleLines;
char	*com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *commandLine ) {
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( *commandLine == '+' || *commandLine == '\n' ) {
			if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of openjk_sp.cfg
===================
*/
qboolean Com_SafeMode( void ) {
	int		i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmp( Cmd_Argv(0), "safe" )
			|| !Q_stricmp( Cmd_Argv(0), "cvar_restart" ) ) {
			com_consoleLines[i][0] = 0;
			return qtrue;
		}
	}
	return qfalse;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	int		i;
	char	*s;
	cvar_t	*cv;

	for (i=0 ; i < com_numConsoleLines ; i++) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv(0), "set" ) ) {
			continue;
		}

		s = Cmd_Argv(1);
		if ( !match || !Q_stricmp( s, match ) ) {
			Cvar_Set( s, Cmd_Argv(2) );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns qtrue if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands( void ) {
	int		i;
	qboolean	added;

	added = qfalse;
	// quote every token, so args with semicolons can work
	for (i=0 ; i < com_numConsoleLines ; i++) {
		if ( !com_consoleLines[i] || !com_consoleLines[i][0] ) {
			continue;
		}

		// set commands won't override menu startup
		if ( Q_stricmpn( com_consoleLines[i], "set", 3 ) ) {
			added = qtrue;
		}
		Cbuf_AddText( com_consoleLines[i] );
		Cbuf_AddText( "\n" );
	}

	return added;
}


//============================================================================


void Info_Print( const char *s ) {
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}

/*
============
Com_StringContains
============
*/
char *Com_StringContains(char *str1, char *str2, int casesensitive) {
	int len, i, j;

	len = strlen(str1) - strlen(str2);
	for (i = 0; i <= len; i++, str1++) {
		for (j = 0; str2[j]; j++) {
			if (casesensitive) {
				if (str1[j] != str2[j]) {
					break;
				}
			}
			else {
				if (toupper(str1[j]) != toupper(str2[j])) {
					break;
				}
			}
		}
		if (!str2[j]) {
			return str1;
		}
	}
	return NULL;
}

/*
============
Com_Filter
============
*/
int Com_Filter(char *filter, char *name, int casesensitive) {
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') {
					break;
				}
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (strlen(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) {
					return qfalse;
				}
				name = ptr + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) {
					return qfalse;
				}
			}
			else {
				if (toupper(*filter) != toupper(*name)) {
					return qfalse;
				}
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}



/*
=================
Com_InitHunkMemory
=================
*/
void Com_InitHunkMemory( void )
{
	Hunk_Clear();

//	Cmd_AddCommand( "meminfo", Z_Details_f );
}

// I'm leaving this in just in case we ever need to remember where's a good place to hook something like this in.
//
void Com_ShutdownHunkMemory(void)
{
}


/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void ) 
{
}



/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void ) 
{
	Z_TagFree(TAG_HUNKALLOC);	
	Z_TagFree(TAG_HUNKMISCMODELS);
}



/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void Hunk_Clear( void ) 
{
	Z_TagFree(TAG_HUNKALLOC);
	Z_TagFree(TAG_HUNKMISCMODELS);

	extern void CIN_CloseAllVideos();
				CIN_CloseAllVideos();

	extern void R_ClearStuffToStopGhoul2CrashingThings(void);
				R_ClearStuffToStopGhoul2CrashingThings();
}






/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

#define	MAX_PUSHED_EVENTS	64
int			com_pushedEventsHead, com_pushedEventsTail;
sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void ) {
	sysEvent_t	ev;

	// get an event from the system
		ev = Sys_GetEvent();

	return ev;
}

/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
	sysEvent_t		*ev;
	static int		printedWarning;

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = qtrue;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = qfalse;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t	Com_GetEvent( void ) {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf ) {
	int		t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer ) {
		t1 = Sys_Milliseconds ();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer ) {
		t2 = Sys_Milliseconds ();
		msec = t2 - t1;
		if ( com_speeds->integer == 3 ) {
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop( void ) {
	sysEvent_t	ev;
	netadr_t	evFrom;
	byte		bufData[MAX_MSGLEN];
	msg_t		buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evTime );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE:
			CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
			Cbuf_AddText( (char *)ev.evPtr );
			Cbuf_AddText( "\n" );
			break;
		case SE_PACKET:
			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
			if ( com_sv_running->integer ) {
				Com_RunAndTimeServerPacket( &evFrom, &buf );
			} else {
				CL_PacketEvent( evFrom, &buf );
			}
			break;
		}

		// free any block data
		if ( ev.evPtr ) {
			Z_Free( ev.evPtr );
		}
	}
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds (void) {
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f (void) {
	if ( Cmd_Argc() > 1 ) {
		Com_Error( ERR_DROP, "Testing drop error" );
	} else {
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f (void) {
	float	s;
	int		start, now;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "freeze <seconds>\n" );
		return;
	}
	s = atof( Cmd_Argv(1) );

	start = Com_Milliseconds();

	while ( 1 ) {
		now = Com_Milliseconds();
		if ( ( now - start ) * 0.001 > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void ) {
	* ( int * ) 0 = 0x12345678;
}

/*
=================
Com_Init
=================
*/
extern void Com_InitZoneMemory();
extern void R_InitWorldEffects();
void Com_Init( char *commandLine ) {
	char	*s;

	Com_Printf( "%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__ );

	try {
		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine( commandLine );

		Swap_Init ();
		Cbuf_Init ();

		Com_InitZoneMemory();

		Cmd_Init ();
		Cvar_Init ();

		// get the commandline cvars set
		Com_StartupVariable( NULL );

		com_jk2 = Cvar_Get( "com_jk2", "0", CVAR_INIT );

		// done early so bind command exists
		CL_InitKeyCommands();

		FS_InitFilesystem ();	//uses z_malloc
		R_InitWorldEffects();   // this doesn't do much but I want to be sure certain variables are intialized.
		
		Cbuf_AddText ("exec default.cfg\n");

		// skip the openjk_sp.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText ("exec "Q3CONFIG_NAME"\n");
		}

		Cbuf_AddText ("exec autoexec.cfg\n");
		
		Cbuf_Execute ();

		// override anything from the config files with command line args
		Com_StartupVariable( NULL );
		
		// allocate the stack based hunk allocator
		Com_InitHunkMemory();

		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;
		
		//
		// init commands and vars
		//
		Cmd_AddCommand ("quit", Com_Quit_f);
		Cmd_AddCommand ("writeconfig", Com_WriteConfig_f );
		
		com_maxfps = Cvar_Get ("com_maxfps", "125", CVAR_ARCHIVE);
		
		com_developer = Cvar_Get ("developer", "0", CVAR_TEMP );
		com_logfile = Cvar_Get ("logfile", "0", CVAR_TEMP );
		com_speedslog = Cvar_Get ("speedslog", "0", CVAR_TEMP );
		
		com_timescale = Cvar_Get ("timescale", "1", CVAR_CHEAT );
		com_fixedtime = Cvar_Get ("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get ("com_showtrace", "0", CVAR_CHEAT);
		com_terrainPhysics = Cvar_Get ("com_terrainPhysics", "1", CVAR_CHEAT);
		com_viewlog = Cvar_Get( "viewlog", "0", CVAR_TEMP );
		com_speeds = Cvar_Get ("com_speeds", "0", 0);
		
#ifdef G2_PERFORMANCE_ANALYSIS
		com_G2Report = Cvar_Get("com_G2Report", "0", 0);
#endif

		cl_paused	   = Cvar_Get ("cl_paused", "0", CVAR_ROM);
		sv_paused	   = Cvar_Get ("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get ("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get ("cl_running", "0", CVAR_ROM);
		com_skippingcin = Cvar_Get ("skippingCinematic", "0", CVAR_ROM);
		com_buildScript = Cvar_Get( "com_buildScript", "0", 0 );
		
		if ( com_developer && com_developer->integer ) {
			Cmd_AddCommand ("error", Com_Error_f);
			Cmd_AddCommand ("crash", Com_Crash_f );
			Cmd_AddCommand ("freeze", Com_Freeze_f);
		}
		
		s = va("%s %s %s", Q3_VERSION, CPUSTRING, __DATE__ );
		com_version = Cvar_Get ("version", s, CVAR_ROM | CVAR_SERVERINFO );

#ifndef __NO_JK2
		if(com_jk2 && com_jk2->integer)
		{
			JK2SP_Init();
		}
		else
#endif
		SE_Init();	// Initialize StringEd
	
		Sys_Init();	// this also detects CPU type, so I can now do this CPU check below...

		Netchan_Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
//	VM_Init();
		SV_Init();
		
		CL_Init();

#ifdef _XBOX
		// Experiment. Sound memory never gets freed, move it earlier. This
		// will also let us play movies sooner, if we need to.
		extern void CL_StartSound(void);
		CL_StartSound();
#endif

		Sys_ShowConsole( com_viewlog->integer, qfalse );
		
		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();

		// add + commands from command line
//#ifndef _XBOX
		if ( !Com_AddStartupCommands() ) {
//#ifdef NDEBUG
			// if the user didn't give any commands, run default action
//			if ( !com_dedicated->integer ) 
			{
				Cbuf_AddText ("cinematic openinglogos\n");
//				if( !com_introPlayed->integer ) {
//					Cvar_Set( com_introPlayed->name, "1" );
//					Cvar_Set( "nextmap", "cinematic intro" );
//				}
			}
//#endif	
		}
//#endif
		com_fullyInitialized = qtrue;
		Com_Printf ("--- Common Initialization Complete ---\n");

//HACKERY FOR THE DEUTSCH		
		//if ( (Cvar_VariableIntegerValue("ui_iscensored") == 1) 	//if this was on before, set it again so it gets its flags
		//	)
		//{
		//	Cvar_Get( "ui_iscensored",   "1", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT|CVAR_NORESTART);
		//	Cvar_Set( "ui_iscensored",   "1");	//just in case it was archived
		//	// NOTE : I also create this in UI_Init()
		//	Cvar_Get( "g_dismemberment", "0", CVAR_ARCHIVE|CVAR_ROM|CVAR_INIT|CVAR_CHEAT);
		//	Cvar_Set( "g_dismemberment", "0");	//just in case it was archived
		//}
	}

	catch (const char* reason) {
		Sys_Error ("Error during initialization %s", reason);
	}

}

//==================================================================

void Com_WriteConfigToFile( const char *filename ) {
	fileHandle_t	f;

	f = FS_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf ("Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf (f, "// generated by OpenJK, do not modify\n");
	Key_WriteBindings (f);
	Cvar_WriteVariables (f);
	FS_FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void ) {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !(cvar_modifiedFlags & CVAR_ARCHIVE ) ) {
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_WriteConfigToFile( Q3CONFIG_NAME );
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void ) {
	char	filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/


int Com_ModifyMsec( int msec, float &fraction ) 
{
	int		clampTime;

	fraction=0.0f;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) 
	{
		msec = com_fixedtime->integer;
	} 
	else if ( com_timescale->value ) 
	{
		fraction=(float)msec;
		fraction*=com_timescale->value;
		msec=(int)floor(fraction);
		fraction-=(float)msec;
	}
	
	// don't let it scale below 1 msec
	if ( msec < 1 ) 
	{
		msec = 1;
		fraction=0.0f;
	}

	if ( com_skippingcin->integer ) {
		// we're skipping ahead so let it go a bit faster
		clampTime = 500;
	} else {
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime ) {
		msec = clampTime;
		fraction=0.0f;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
static vec3_t corg;
static vec3_t cangles;
static bool bComma;
void Com_SetOrgAngles(vec3_t org,vec3_t angles)
{
	VectorCopy(org,corg);
	VectorCopy(angles,cangles);
}

#ifdef G2_PERFORMANCE_ANALYSIS
void G2Time_ResetTimers(void);
void G2Time_ReportTimers(void);
#endif

#pragma warning (disable: 4701)	//local may have been used without init (timing info vars)
void Com_Frame( void ) {
try 
{
	int		timeBeforeFirstEvents, timeBeforeServer, timeBeforeEvents, timeBeforeClient, timeAfter;
	int		msec, minMsec;
	static int	lastTime;

	// write config file if anything changed
#ifndef _XBOX
	Com_WriteConfiguration(); 

	// if "viewlog" has been modified, show or hide the log console
	if ( com_viewlog->modified ) {
		Sys_ShowConsole( com_viewlog->integer, qfalse );
		com_viewlog->modified = qfalse;
	}
#endif

	//
	// main event loop
	//
	if ( com_speeds->integer ) {
		timeBeforeFirstEvents = Sys_Milliseconds ();
	}

	// we may want to spin here if things are going too fast
	if ( com_maxfps->integer > 0 ) {
		minMsec = 1000 / com_maxfps->integer;
	} else {
		minMsec = 1;
	}
	do {
		com_frameTime = Com_EventLoop();
		if ( lastTime > com_frameTime ) {
			lastTime = com_frameTime;		// possible on first frame
		}
		msec = com_frameTime - lastTime;
	} while ( msec < minMsec );
	Cbuf_Execute ();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	float fractionMsec=0.0f;
	msec = Com_ModifyMsec( msec, fractionMsec);
	
	//
	// server side
	//
	if ( com_speeds->integer ) {
		timeBeforeServer = Sys_Milliseconds ();
	}

	SV_Frame (msec, fractionMsec);


	//
	// client system
	//
#ifdef _XBOX
//	extern void G_DemoFrame();

//	G_DemoFrame();
	extern bool TestDemoTimer();
	extern void PlayDemo();
	if ( TestDemoTimer())
	{
		PlayDemo();
	}


#endif
//	if ( !com_dedicated->integer ) 
	{
		//
		// run event loop a second time to get server to client packets
		// without a frame of latency
		//
		if ( com_speeds->integer ) {
			timeBeforeEvents = Sys_Milliseconds ();
		}
		Com_EventLoop();
		Cbuf_Execute ();


		//
		// client side
		//
		if ( com_speeds->integer ) {
			timeBeforeClient = Sys_Milliseconds ();
		}

		CL_Frame (msec, fractionMsec);

		if ( com_speeds->integer ) {
			timeAfter = Sys_Milliseconds ();
		}
	}


	//
	// report timing information
	//
	if ( com_speeds->integer ) {
		int			all, sv, ev, cl;

		all = timeAfter - timeBeforeServer;
		sv = timeBeforeEvents - timeBeforeServer;
		ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf("fr:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i tr:%3i pvs:%3i rf:%3i bk:%3i\n", 
					com_frameNumber, all, sv, ev, cl, time_game, timeInTrace, timeInPVSCheck, time_frontend, time_backend);

#ifndef _XBOX
		// speedslog
		if ( com_speedslog && com_speedslog->integer )
		{
			if(!speedslog)
			{
				speedslog = FS_FOpenFileWrite("speeds.log");
				FS_Write("data={\n", strlen("data={\n"), speedslog);
				bComma=false;
				if ( com_speedslog->integer > 1 ) 
				{
					// force it to not buffer so we get valid
					// data even if we are crashing
					FS_ForceFlush(logfile);
				}
			}
			if (speedslog)
			{
				char		msg[MAXPRINTMSG];

				if(bComma)
				{
					FS_Write(",\n", strlen(",\n"), speedslog);
					bComma=false;
				}
				FS_Write("{", strlen("{"), speedslog);
				Com_sprintf(msg,sizeof(msg),
							"%8.4f,%8.4f,%8.4f,%8.4f,%8.4f,%8.4f,",corg[0],corg[1],corg[2],cangles[0],cangles[1],cangles[2]);
				FS_Write(msg, strlen(msg), speedslog);
				Com_sprintf(msg,sizeof(msg),
					"%i,%3i,%3i,%3i,%3i,%3i,%3i,%3i,%3i,%3i}", 
					com_frameNumber, all, sv, ev, cl, time_game, timeInTrace, timeInPVSCheck, time_frontend, time_backend);
				FS_Write(msg, strlen(msg), speedslog);
				bComma=true;
			}
		}
#endif

		timeInTrace = timeInPVSCheck = 0;
	}

	//
	// trace optimization tracking
	//
	if ( com_showtrace->integer ) {
		extern	int c_traces, c_brush_traces, c_patch_traces;
		extern	int	c_pointcontents;

		/*
		Com_Printf( "%4i non-sv_traces, %4i sv_traces, %4i ms, ave %4.2f ms\n", c_traces - numTraces, numTraces, timeInTrace, (float)timeInTrace/(float)numTraces );
		timeInTrace = numTraces = 0;
		c_traces = 0;
		*/
		
		Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
			c_brush_traces, c_patch_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_patch_traces = 0;
		c_pointcontents = 0;
	}

	com_frameNumber++;
}//try
	catch (const char* reason) {
		Com_Printf (reason);
		return;			// an ERR_DROP was thrown
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	if (com_G2Report && com_G2Report->integer)
	{
		G2Time_ReportTimers();
	}

	G2Time_ResetTimers();
#endif
}

#pragma warning (default: 4701)	//local may have been used without init

/*
=================
Com_Shutdown
=================
*/
extern void CM_FreeShaderText(void);
void Com_Shutdown (void) {
	CM_ClearMap();

	CM_FreeShaderText();

	if (logfile) {
		FS_FCloseFile (logfile);
		logfile = 0;
	}

	if (speedslog) {
		FS_Write("\n};", strlen("\n};"), speedslog);
		FS_FCloseFile (speedslog);
		speedslog = 0;
	}

	if (camerafile) {
		FS_FCloseFile (camerafile);
		camerafile = 0;
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}

#ifndef __NO_JK2
	if(com_jk2 && com_jk2->integer)
	{
		JK2SP_Shutdown();
	}
	else
#endif
	SE_ShutDown();//close the string packages

	extern void Netchan_Shutdown();
	Netchan_Shutdown();
}

/*
============
ParseTextFile
============
*/

bool Com_ParseTextFile(const char *file, class CGenericParser2 &parser, bool cleanFirst)
{
	fileHandle_t	f;
	int				length = 0;
	char			*buf = 0, *bufParse = 0;

	length = FS_FOpenFileByMode( file, &f, FS_READ );
	if (!f || !length)		
	{
		return false;
	}

	buf = new char [length + 1];
	FS_Read( buf, length, f );
	buf[length] = 0;

	bufParse = buf;
	parser.Parse(&bufParse, cleanFirst);
	delete[] buf;

	FS_FCloseFile( f );

	return true;
}

void Com_ParseTextFileDestroy(class CGenericParser2 &parser)
{
	parser.Clean();
}

CGenericParser2 *Com_ParseTextFile(const char *file, bool cleanFirst, bool writeable)
{
	fileHandle_t	f;
	int				length = 0;
	char			*buf = 0, *bufParse = 0;
	CGenericParser2 *parse;

	length = FS_FOpenFileByMode( file, &f, FS_READ );
	if (!f || !length)		
	{
		return 0;
	}

	buf = new char [length + 1];
	FS_Read( buf, length, f );
	FS_FCloseFile( f );
	buf[length] = 0;

	bufParse = buf;

	parse = new CGenericParser2;
	if (!parse->Parse(&bufParse, cleanFirst, writeable))
	{
		delete parse;
		parse = 0;
	}

	delete[] buf;

	return parse;
}

