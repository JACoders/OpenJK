// common.c -- misc functions used in client and server

//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "GenericParser2.h"
#include "stringed_ingame.h"
#include "qcommon/game_version.h"
#include "../server/NPCNav/navigator.h"

#define	MAXPRINTMSG	4096

FILE *debuglogfile;
fileHandle_t logfile;
fileHandle_t	com_journalFile;			// events are written here
fileHandle_t	com_journalDataFile;		// config files are written here

cvar_t	*com_viewlog;
cvar_t	*com_speeds;
cvar_t	*com_developer;
cvar_t	*com_dedicated;
cvar_t	*com_timescale;
cvar_t	*com_fixedtime;
cvar_t	*com_dropsim;		// 0.0 to 1.0, simulated packet drops
cvar_t	*com_journal;
cvar_t	*com_maxfps;
cvar_t	*com_timedemo;
cvar_t	*com_sv_running;
cvar_t	*com_cl_running;
cvar_t	*com_logfile;		// 1 = buffer log, 2 = flush after each print
cvar_t	*com_showtrace;

cvar_t	*com_optvehtrace;

#ifdef G2_PERFORMANCE_ANALYSIS
cvar_t	*com_G2Report;
#endif

cvar_t	*com_terrainPhysics; //rwwRMG - added

cvar_t	*com_version;
cvar_t	*com_buildScript;	// for automated data building scripts
cvar_t	*com_bootlogo;
cvar_t	*cl_paused;
cvar_t	*sv_paused;
cvar_t	*com_cameraMode;
cvar_t	*com_unfocused;
cvar_t	*com_minimized;
cvar_t  *com_homepath;

cvar_t	*com_RMG;

#ifdef _DEBUG
cvar_t	*vm_legacy;
#endif

// com_speeds times
int		time_game;
int		time_frontend;		// renderer frontend time
int		time_backend;		// renderer backend time

int			com_frameTime;
int			com_frameMsec;
int			com_frameNumber;

qboolean	com_errorEntered = qfalse;
qboolean	com_fullyInitialized = qfalse;

char	com_errorMessage[MAXPRINTMSG] = {0};

void Com_WriteConfig_f( void );

//============================================================================

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

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the appropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void QDECL Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean opening_qconsole = qfalse;

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( rd_buffer ) {
		if ((strlen (msg) + strlen(rd_buffer)) > (size_t)(rd_buffersize - 1)) {
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat(rd_buffer, rd_buffersize, msg);
    // TTimo nooo .. that would defeat the purpose
		//rd_flush(rd_buffer);			
		//*rd_buffer = 0;
		return;
	}

#ifndef DEDICATED
	CL_ConsolePrint( msg );
#endif

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer ) {
    // TTimo: only open the qconsole.log if the filesystem is in an initialized state
    //   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if ( !logfile && FS_Initialized() && !opening_qconsole ) {
			struct tm *newtime;
			time_t aclock;

			opening_qconsole = qtrue;

			time( &aclock );
			newtime = localtime( &aclock );

			logfile = FS_FOpenFileWrite( "qconsole.log" );

			if ( logfile ) {
				Com_Printf( "logfile opened on %s\n", asctime( newtime ) );
				if ( com_logfile->integer > 1 ) {
					// force it to not buffer so we get valid
					// data even if we are crashing
					FS_ForceFlush(logfile);
				}
			}
			else {
				Com_Printf( "Opening qconsole.log failed!\n" );
				Cvar_SetValue( "logfile", 0 );
			}
		}
		opening_qconsole = qfalse;
		if ( logfile && FS_Initialized()) {
			FS_Write(msg, strlen(msg), logfile);
		}
	}


#if defined(_WIN32) && defined(_DEBUG)
	if ( *msg )
	{
		OutputDebugString ( Q_CleanStr(msg) );
		OutputDebugString ("\n");
	}
#endif
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
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);
	
	Com_Printf ("%s", msg);
}

// Outputs to the VC / Windows Debug window (only in debug compile)
void QDECL Com_OPrintf( const char *fmt, ...) 
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);
#ifdef _WIN32
	OutputDebugString(msg);
#else
	printf("%s", msg);
#endif
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the appropriate things.
=============
*/
void QDECL Com_Error( int code, const char *fmt, ... ) {
	va_list		argptr;
	static int	lastErrorTime;
	static int	errorCount;
	int			currentTime;

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	com_errorEntered = qtrue;

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {
		code = ERR_FATAL;
	}

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERR_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	va_start (argptr,fmt);
	Q_vsnprintf (com_errorMessage,sizeof(com_errorMessage), fmt,argptr);
	va_end (argptr);

	if ( code != ERR_DISCONNECT && code != ERR_NEED_CD ) {
		Cvar_Get("com_errorMessage", "", CVAR_ROM);	//give com_errorMessage a default so it won't come back to life after a resetDefaults
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	if ( code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT || code == ERR_DROP || code == ERR_NEED_CD ) {
		throw code;
	} else {
		CL_Shutdown ();
		SV_Shutdown (va("Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown ();

	Sys_Error ("%s", com_errorMessage);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the appropriate things.
=============
*/
void Com_Quit_f( void ) {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown ("Server quit\n");
		CL_Shutdown ();
		Com_Shutdown ();
		FS_Shutdown(qtrue);
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
	int inq = 0;
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		if (*commandLine == '"') {
			inq = !inq;
		}
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( (*commandLine == '+' && !inq) || *commandLine == '\n'  || *commandLine == '\r' ) {
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
skip loading of jampconfig.cfg
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
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	for (int i=0 ; i < com_numConsoleLines ; i++) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv(0), "set" ) ) {
			continue;
		}

		char *s = Cmd_Argv(1);

		if ( !match || !strcmp( s, match ) )
			Cvar_User_Set( s, Cmd_Argv( 2 ) );
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
	char	key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
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
			Com_Memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s ", key);

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
int Com_Filter(char *filter, char *name, int casesensitive)
{
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i, found;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?') break;
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if (strlen(buf)) {
				ptr = Com_StringContains(name, buf, casesensitive);
				if (!ptr) return qfalse;
				name = ptr + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[' && *(filter+1) == '[') {
			filter++;
		}
		else if (*filter == '[') {
			filter++;
			found = qfalse;
			while(*filter && !found) {
				if (*filter == ']' && *(filter+1) != ']') break;
				if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
					if (casesensitive) {
						if (*name >= *filter && *name <= *(filter+2)) found = qtrue;
					}
					else {
						if (toupper(*name) >= toupper(*filter) &&
							toupper(*name) <= toupper(*(filter+2))) found = qtrue;
					}
					filter += 3;
				}
				else {
					if (casesensitive) {
						if (*filter == *name) found = qtrue;
					}
					else {
						if (toupper(*filter) == toupper(*name)) found = qtrue;
					}
					filter++;
				}
			}
			if (!found) return qfalse;
			while(*filter) {
				if (*filter == ']' && *(filter+1) != ']') break;
				filter++;
			}
			filter++;
			name++;
		}
		else {
			if (casesensitive) {
				if (*filter != *name) return qfalse;
			}
			else {
				if (toupper(*filter) != toupper(*name)) return qfalse;
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}

/*
============
Com_FilterPath
============
*/
int Com_FilterPath(char *filter, char *name, int casesensitive)
{
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for (i = 0; i < MAX_QPATH-1 && filter[i]; i++) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		}
		else {
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for (i = 0; i < MAX_QPATH-1 && name[i]; i++) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		}
		else {
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Com_Filter(new_filter, new_name, casesensitive);
}

/*
============
Com_HashKey
============
*/
int Com_HashKey(char *string, int maxlen) {
	int register hash, i;

	hash = 0;
	for (i = 0; i < maxlen && string[i] != '\0'; i++) {
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

/*
================
Com_RealTime
================
*/
int Com_RealTime(qtime_t *qtime) {
	time_t t;
	struct tm *tms;

	t = time(NULL);
	if (!qtime)
		return t;
	tms = localtime(&t);
	if (tms) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}


/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
#define	MAX_PUSHED_EVENTS	            1024
// bk001129 - init, also static
static int		com_pushedEventsHead = 0;
static int             com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void ) {
	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get ("journal", "0", CVAR_INIT);
	if ( !com_journal->integer ) {
		return;
	}

	if ( com_journal->integer == 1 ) {
		Com_Printf( "Journaling events\n");
		com_journalFile = FS_FOpenFileWrite( "journal.dat" );
		com_journalDataFile = FS_FOpenFileWrite( "journaldata.dat" );
	} else if ( com_journal->integer == 2 ) {
		Com_Printf( "Replaying journaled events\n");
		FS_FOpenFileRead( "journal.dat", &com_journalFile, qtrue );
		FS_FOpenFileRead( "journaldata.dat", &com_journalDataFile, qtrue );
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		Cvar_Set( "com_journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t	Com_GetRealEvent( void ) {
	int			r;
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 ) {
		r = FS_Read( &ev, sizeof(ev), com_journalFile );
		if ( r != sizeof(ev) ) {
			Com_Error( ERR_FATAL, "Error reading from journal file" );
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = Z_Malloc( ev.evPtrLength, TAG_EVENT, qtrue );
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );
			if ( r != ev.evPtrLength ) {
				Com_Error( ERR_FATAL, "Error reading from journal file" );
			}
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 ) {
			r = FS_Write( &ev, sizeof(ev), com_journalFile );
			if ( r != sizeof(ev) ) {
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );
				if ( r != ev.evPtrLength ) {
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void ) {
  // clear the static buffer array
  // this requires SE_NONE to be accepted as a valid but NOP event
  memset( com_pushedEvents, 0, sizeof(com_pushedEvents) );
  // reset counters while we are at it
  // beware: GetEvent might still return an SE_NONE from the buffer
  com_pushedEventsHead = 0;
  com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
	sysEvent_t		*ev;
	static int printedWarning = 0; // bk001129 - init, bk001204 - explicit int

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
		  // bk001129 - was ev.evTime
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
			break;
        case SE_NONE:
            break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, (qboolean)ev.evValue2, ev.evTime );
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
			if ( ((char *)ev.evPtr)[0] == '\\' || ((char *)ev.evPtr)[0] == '/' ) 
			{
				Cbuf_AddText( (char *)ev.evPtr+1 );	
			}
			else
			{
				Cbuf_AddText( (char *)ev.evPtr );
			}
			Cbuf_AddText( "\n" );
			break;
		case SE_PACKET:
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if ( com_dropsim->value > 0 ) {
				static int seed;

				if ( Q_random( &seed ) < com_dropsim->value ) {
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t *)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof( evFrom );

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ( (unsigned)buf.cursize > (unsigned)buf.maxsize ) {
				Com_Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			Com_Memcpy( buf.data, (byte *)((netadr_t *)ev.evPtr + 1), buf.cursize );
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

	return 0;	// never reached
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
	* ( volatile int * ) 0 = 0x12345678;
}

#ifdef MEM_DEBUG
	void SH_Register(void);
#endif

/*
==================
Com_ExecuteCfg
==================
*/

void Com_ExecuteCfg(void)
{
	Cbuf_ExecuteText(EXEC_NOW, "exec mpdefault.cfg\n");
	Cbuf_Execute(); // Always execute after exec to prevent text buffer overflowing

	if(!Com_SafeMode())
	{
		// skip the q3config.cfg and autoexec.cfg if "safe" is on the command line
		Cbuf_ExecuteText(EXEC_NOW, "exec " Q3CONFIG_CFG "\n");
		Cbuf_Execute();
		Cbuf_ExecuteText(EXEC_NOW, "exec autoexec.cfg\n");
		Cbuf_Execute();
	}
}

/*
=================
Com_InitRand
Seed the random number generator, if possible with an OS supplied random seed.
=================
*/
static void Com_InitRand(void)
{
	unsigned int seed;

	if(Sys_RandomBytes((byte *) &seed, sizeof(seed)))
		srand(seed);
	else
		srand(time(NULL));
}

/*
=================
Com_ErrorString
Error string for the given error code (from Com_Error).
=================
*/
static const char *Com_ErrorString ( int code )
{
	switch ( code )
	{
		case ERR_DISCONNECT:
		// fallthrough
		case ERR_SERVERDISCONNECT:
			return "DISCONNECTED";

		case ERR_DROP:
			return "DROPPED";

		case ERR_NEED_CD:
			return "NEED CD";

		default:
			return "UNKNOWN";
	}
}

/*
=================
Com_CatchError
Handles freeing up of resources when Com_Error is called.
=================
*/
static void Com_CatchError ( int code )
{
	if ( code == ERR_DISCONNECT || code == ERR_SERVERDISCONNECT ) {
		SV_Shutdown( "Server disconnected" );
		CL_Disconnect( qtrue );
		CL_FlushMemory(  );
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	} else if ( code == ERR_DROP ) {
		Com_Printf ("********************\n"
					"ERROR: %s\n"
					"********************\n", com_errorMessage);
		SV_Shutdown (va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect( qtrue );
		CL_FlushMemory( );
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	} else if ( code == ERR_NEED_CD ) {
		SV_Shutdown( "Server didn't have CD" );
		if ( com_cl_running && com_cl_running->integer ) {
			CL_Disconnect( qtrue );
			CL_FlushMemory( );
		} else {
			Com_Printf("Server didn't have CD\n" );
		}
		// make sure we can get at our local stuff
		FS_PureServerSetLoadedPaks( "", "" );
		com_errorEntered = qfalse;
	}
}

/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine ) {
	char	*s;

	Com_Printf( "%s %s %s\n", JK_VERSION, PLATFORM_STRING, __DATE__ );

	try
	{
		// initialize the weak pseudo-random number generator for use later.
		Com_InitRand();

		// do this before anything else decides to push events
		Com_InitPushEvent();

		Cvar_Init ();

		navigator.Init();

		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine( commandLine );

	//	Swap_Init ();
		Cbuf_Init ();

		Com_InitZoneMemory();

		Cmd_Init ();

		// override anything from the config files with command line args
		Com_StartupVariable( NULL );

		// Seed the random number generator
		Rand_Init(Sys_Milliseconds(true));

		// get the developer cvar set as early as possible
		com_developer = Cvar_Get("developer", "0", CVAR_TEMP);

		// done early so bind command exists
		CL_InitKeyCommands();

		com_homepath = Cvar_Get("com_homepath", "", CVAR_INIT);

		FS_InitFilesystem ();

		Com_InitJournaling();

		Com_ExecuteCfg();

		// override anything from the config files with command line args
		Com_StartupVariable( NULL );

	  // get dedicated here for proper hunk megs initialization
	#ifdef DEDICATED
		com_dedicated = Cvar_Get ("dedicated", "2", CVAR_ROM);
		Cvar_CheckRange( com_dedicated, 1, 2, qtrue );
	#else
		//OJKFIXME: Temporarily disabled dedicated server when not using the dedicated server binary.
		//			Issue is the server not having a renderer when not using ^^^^^
		//				and crashing in SV_SpawnServer calling re.RegisterMedia_LevelLoadBegin
		//			Until we fully remove the renderer from the server, the client executable
		//				will not have dedicated support capabilities.
		//			Use the dedicated server package.
		com_dedicated = Cvar_Get ("_dedicated", "0", CVAR_ROM|CVAR_INIT|CVAR_PROTECTED);
	//	Cvar_CheckRange( com_dedicated, 0, 2, qtrue );
	#endif
		// allocate the stack based hunk allocator
		Com_InitHunkMemory();

		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;

		//
		// init commands and vars
		//
		com_maxfps = Cvar_Get ("com_maxfps", "125", CVAR_ARCHIVE);

		com_logfile = Cvar_Get ("logfile", "0", CVAR_TEMP );

		com_timescale = Cvar_Get ("timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO );
		com_fixedtime = Cvar_Get ("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get ("com_showtrace", "0", CVAR_CHEAT);

		com_terrainPhysics = Cvar_Get ("com_terrainPhysics", "1", CVAR_CHEAT);

		com_dropsim = Cvar_Get ("com_dropsim", "0", CVAR_CHEAT);
		com_viewlog = Cvar_Get( "viewlog", "0", 0 );
		com_speeds = Cvar_Get ("com_speeds", "0", 0);
		com_timedemo = Cvar_Get ("timedemo", "0", 0);
		com_cameraMode = Cvar_Get ("com_cameraMode", "0", CVAR_CHEAT);

		com_unfocused = Cvar_Get( "com_unfocused", "0", CVAR_ROM );
		com_minimized = Cvar_Get( "com_minimized", "0", CVAR_ROM );

		com_optvehtrace = Cvar_Get("com_optvehtrace", "0", 0);

		cl_paused = Cvar_Get ("cl_paused", "0", CVAR_ROM);
		sv_paused = Cvar_Get ("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get ("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get ("cl_running", "0", CVAR_ROM);
		com_buildScript = Cvar_Get( "com_buildScript", "0", 0 );

#ifdef G2_PERFORMANCE_ANALYSIS
		com_G2Report = Cvar_Get("com_G2Report", "0", 0);
#endif

		com_RMG = Cvar_Get("RMG", "0", 0);

		Cvar_Get ("RMG_seed", "0", 0);
		Cvar_Get ("RMG_time", "day", 0);
		Cvar_Get ("RMG_soundset", "", 0);

		Cvar_Get ("RMG_textseed", "0", CVAR_SYSTEMINFO|CVAR_ARCHIVE);
		Cvar_Get ("RMG_map", "small", CVAR_ARCHIVE|CVAR_SYSTEMINFO);
		Cvar_Get ("RMG_timefile", "day", CVAR_ARCHIVE);
		Cvar_Get ("RMG_terrain", "grassyhills", CVAR_ARCHIVE);

		Cvar_Get ("RMG_sky", "", CVAR_SYSTEMINFO );
		Cvar_Get ("RMG_fog", "", CVAR_SYSTEMINFO );
		Cvar_Get ("RMG_weather", "", CVAR_SYSTEMINFO|CVAR_SERVERINFO|CVAR_CHEAT );
		Cvar_Get ("RMG_instances", "colombia", CVAR_SYSTEMINFO );
		Cvar_Get ("RMG_miscents", "deciduous", 0);
		Cvar_Get ("RMG_music", "music/dm_kam1", 0);
		Cvar_Get ("RMG_mission", "ctf", CVAR_SYSTEMINFO );
		Cvar_Get ("RMG_course", "standard", CVAR_SYSTEMINFO );
		Cvar_Get ("RMG_distancecull", "5000", CVAR_CHEAT );

		com_bootlogo = Cvar_Get( "com_bootlogo", "1", CVAR_ARCHIVE);

#ifdef _DEBUG
		vm_legacy = Cvar_Get( "vm_legacy", "0", 0 );
#endif

		if ( com_dedicated->integer ) {
			if ( !com_viewlog->integer ) {
				Cvar_Set( "viewlog", "1" );
			}
		}

		if ( com_developer && com_developer->integer ) {
			Cmd_AddCommand ("error", Com_Error_f);
			Cmd_AddCommand ("crash", Com_Crash_f );
			Cmd_AddCommand ("freeze", Com_Freeze_f);
		}
		Cmd_AddCommand ("quit", Com_Quit_f);
		Cmd_AddCommand ("changeVectors", MSG_ReportChangeVectors_f );
		Cmd_AddCommand ("writeconfig", Com_WriteConfig_f );

		s = va("%s %s %s", JK_VERSION_OLD, PLATFORM_STRING, __DATE__ );
		com_version = Cvar_Get ("version", s, CVAR_ROM | CVAR_SERVERINFO );

		SE_Init();

		Sys_Init();
		Netchan_Init( Com_Milliseconds() & 0xffff );	// pick a port value that should be nice and random
		SV_Init();

		com_dedicated->modified = qfalse;
		if ( !com_dedicated->integer ) {
			CL_Init();
			Sys_ShowConsole( com_viewlog->integer, qfalse );
		}

		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();


		// add + commands from command line
		if ( !Com_AddStartupCommands() ) 
		{
			// if the user didn't give any commands, run default action
			if ( !com_dedicated->integer ) 
			{
				if ( com_bootlogo->integer )
				{
					Cbuf_AddText ("cinematic openinglogos.roq\n");
				}
			}
		}

		// start in full screen ui mode
		Cvar_Set("r_uiFullScreen", "1");

		CL_StartHunkUsers();

		// make sure single player is off by default
		Cvar_Set("ui_singlePlayerActive", "0");

#ifdef MEM_DEBUG
		SH_Register();
#endif

		com_fullyInitialized = qtrue;
		Com_Printf ("--- Common Initialization Complete ---\n");	
	}
	catch ( int code )
	{
		Com_CatchError (code);
		Sys_Error ("Error during initialization: %s", Com_ErrorString (code));
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

	FS_Printf (f, "// generated by OpenJK MP, do not modify\n");
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

	Com_WriteConfigToFile( Q3CONFIG_CFG );
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

	if(!COM_CompareExtension(filename, ".cfg"))
	{
		Com_Printf( "Com_WriteConfig_f: Only the \".cfg\" extension is supported by this command!\n" );
		return;
	}

	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec( int msec ) {
	int		clampTime;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) {
		msec = com_fixedtime->integer;
	} else if ( com_timescale->value ) {
		msec *= com_timescale->value;
	} else if (com_cameraMode->integer) {
		msec *= com_timescale->value;
	}
	
	// don't let it scale below 1 msec
	if ( msec < 1 && com_timescale->value) {
		msec = 1;
	}

	if ( com_dedicated->integer ) {
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if ( com_sv_running->integer && msec > 500 ) {
			Com_Printf( "Hitch warning: %i msec frame time\n", msec );
		}
		clampTime = 5000;
	} else 
	if ( !com_sv_running->integer ) {
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	} else {
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if ( msec > clampTime ) {
		msec = clampTime;
	}

	return msec;
}

#ifdef G2_PERFORMANCE_ANALYSIS
#include "qcommon/timing.h"
void G2Time_ResetTimers(void);
void G2Time_ReportTimers(void);
extern timing_c G2PerformanceTimer_PreciseFrame;
extern int G2Time_PreciseFrame;
#endif

/*
=================
Com_Frame
=================
*/
void Com_Frame( void ) {

	try
	{
#ifdef G2_PERFORMANCE_ANALYSIS
		G2PerformanceTimer_PreciseFrame.Start();
#endif
		int		msec, minMsec;
		static int	lastTime = 0;
 
		int		timeBeforeFirstEvents;
		int           timeBeforeServer;
		int           timeBeforeEvents;
		int           timeBeforeClient;
		int           timeAfter;


		// bk001204 - init to zero.
		//  also:  might be clobbered by `longjmp' or `vfork'
		timeBeforeFirstEvents =0;
		timeBeforeServer =0;
		timeBeforeEvents =0;
		timeBeforeClient = 0;
		timeAfter = 0;

		// write config file if anything changed
		Com_WriteConfiguration(); 

		// if "viewlog" has been modified, show or hide the log console
		if ( com_viewlog->modified ) {
			if ( !com_dedicated->value ) {
				Sys_ShowConsole( com_viewlog->integer, qfalse );
			}
			com_viewlog->modified = qfalse;
		}

		//
		// main event loop
		//
		if ( com_speeds->integer ) {
			timeBeforeFirstEvents = Sys_Milliseconds ();
		}

		// we may want to spin here if things are going too fast
		if ( !com_dedicated->integer && com_maxfps->integer > 0 && !com_timedemo->integer ) {
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
		msec = Com_ModifyMsec( msec );

		//
		// server side
		//
		if ( com_speeds->integer ) {
			timeBeforeServer = Sys_Milliseconds ();
		}

		SV_Frame( msec );

		// if "dedicated" has been modified, start up
		// or shut down the client system.
		// Do this after the server may have started,
		// but before the client tries to auto-connect
		if ( com_dedicated->modified ) {
			// get the latched value
			Cvar_Get( "_dedicated", "0", 0 );
			com_dedicated->modified = qfalse;
			if ( !com_dedicated->integer ) {
				CL_Init();
				Sys_ShowConsole( com_viewlog->integer, qfalse );
				CL_StartHunkUsers();	//fire up the UI!
			} else {
				CL_Shutdown();
				Sys_ShowConsole( 1, qtrue );
			}
		}

		//
		// client system
		//
		if ( !com_dedicated->integer ) {
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

			CL_Frame( msec );

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

			Com_Printf ("frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n", 
						 com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend );
		}	

		//
		// trace optimization tracking
		//
		if ( com_showtrace->integer ) {
	
			extern	int c_traces, c_brush_traces, c_patch_traces;
			extern	int	c_pointcontents;

			Com_Printf ("%4i traces  (%ib %ip) %4i points\n", c_traces,
				c_brush_traces, c_patch_traces, c_pointcontents);
			c_traces = 0;
			c_brush_traces = 0;
			c_patch_traces = 0;
			c_pointcontents = 0;
		}

		com_frameNumber++;
	}
	catch (int code) {
		Com_CatchError (code);
		Com_Printf ("%s\n", Com_ErrorString (code));
		return;
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_PreciseFrame += G2PerformanceTimer_PreciseFrame.End();

	if (com_G2Report && com_G2Report->integer)
	{
		G2Time_ReportTimers();
	}

	G2Time_ResetTimers();
#endif
}

/*
=================
Com_Shutdown
=================
*/
void MSG_shutdownHuffman();
void Com_Shutdown (void) 
{
	CM_ClearMap();

	if (logfile) {
		FS_FCloseFile (logfile);
		logfile = 0;
		com_logfile->integer = 0;//don't open up the log file again!!
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}

	MSG_shutdownHuffman();
/*
	// Only used for testing changes to huffman frequency table when tuning.
	{
		extern float Huff_GetCR(void);
		char mess[256];
		sprintf(mess,"Eff. CR = %f\n",Huff_GetCR());
		OutputDebugString(mess);
	}
*/
}

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t *edit ) {
	edit->buffer[0] = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

static const char *completionString;
static char shortestMatch[MAX_TOKEN_CHARS];
static int	matchCount;
// field we are working on, passed to Field_AutoComplete(&g_consoleCommand for instance)
static field_t *completionField;

/*
===============
FindMatches

===============
*/
static void FindMatches( const char *s ) {
	int		i;

	if ( Q_stricmpn( s, completionString, strlen( completionString ) ) ) {
		return;
	}
	matchCount++;
	if ( matchCount == 1 ) {
		Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
		return;
	}

	// cut shortestMatch to the amount common with s
	for ( i = 0 ; s[i] ; i++ ) {
		if ( tolower(shortestMatch[i]) != tolower(s[i]) ) {
			shortestMatch[i] = 0;
			break;
		}
	}
	if (!s[i])
	{
		shortestMatch[i] = 0;
	}
}

/*
===============
PrintMatches

===============
*/
static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( S_COLOR_GREY"Cmd  "S_COLOR_WHITE"%s\n", s );
	}
}

/*
===============
PrintArgMatches

===============
*/
#if 0
// This is here for if ever commands with other argument completion
static void PrintArgMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( S_COLOR_WHITE"  %s\n", s );
	}
}
#endif

#ifndef DEDICATED
/*
===============
PrintKeyMatches

===============
*/
static void PrintKeyMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( S_COLOR_GREY"Key  "S_COLOR_WHITE"%s\n", s );
	}
}
#endif

/*
===============
PrintFileMatches

===============
*/
static void PrintFileMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( S_COLOR_GREY"File "S_COLOR_WHITE"%s\n", s );
	}
}

/*
===============
PrintCvarMatches

===============
*/
static void PrintCvarMatches( const char *s ) {
	char value[TRUNCATE_LENGTH] = {0};

	if ( !Q_stricmpn( s, shortestMatch, (int)strlen( shortestMatch ) ) ) {
		Com_TruncateLongString( value, Cvar_VariableString( s ) );
		Com_Printf( S_COLOR_GREY"Cvar "S_COLOR_WHITE"%s = "S_COLOR_GREY"\""S_COLOR_WHITE"%s"S_COLOR_GREY"\""S_COLOR_WHITE"\n", s, value );
	}
}

/*
===============
Field_FindFirstSeparator
===============
*/
static char *Field_FindFirstSeparator( char *s ) {
	for ( size_t i=0; i<strlen( s ); i++ ) {
		if ( s[i] == ';' )
			return &s[ i ];
	}

	return NULL;
}

/*
===============
Field_Complete
===============
*/
static qboolean Field_Complete( void ) {
	int completionOffset;

	if ( matchCount == 0 )
		return qtrue;

	completionOffset = strlen( completionField->buffer ) - strlen( completionString );

	Q_strncpyz( &completionField->buffer[completionOffset], shortestMatch, sizeof( completionField->buffer ) - completionOffset );

	completionField->cursor = strlen( completionField->buffer );

	if ( matchCount == 1 ) {
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		completionField->cursor++;
		return qtrue;
	}

	Com_Printf( "%c%s\n", CONSOLE_PROMPT_CHAR, completionField->buffer );

	return qfalse;
}

#ifndef DEDICATED
/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( void )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	Key_KeynameCompletion( FindMatches );

	if( !Field_Complete( ) )
		Key_KeynameCompletion( PrintKeyMatches );
}
#endif

/*
===============
Field_CompleteFilename
===============
*/
void Field_CompleteFilename( const char *dir, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk )
{
	matchCount = 0;
	shortestMatch[ 0 ] = 0;

	FS_FilenameCompletion( dir, ext, stripExt, FindMatches, allowNonPureFilesOnDisk );

	if ( !Field_Complete() )
		FS_FilenameCompletion( dir, ext, stripExt, PrintFileMatches, allowNonPureFilesOnDisk );
}

/*
===============
Field_CompleteCommand
===============
*/
void Field_CompleteCommand( char *cmd, qboolean doCommands, qboolean doCvars )
{
	int completionArgument = 0;

	// Skip leading whitespace and quotes
	cmd = Com_SkipCharset( cmd, " \"" );

	Cmd_TokenizeStringIgnoreQuotes( cmd );
	completionArgument = Cmd_Argc();

	// If there is trailing whitespace on the cmd
	if ( *(cmd + strlen( cmd )-1) == ' ' ) {
		completionString = "";
		completionArgument++;
	}
	else
		completionString = Cmd_Argv( completionArgument - 1 );

	if ( completionArgument > 1 ) {
		const char *baseCmd = Cmd_Argv( 0 );
		char *p;

#ifndef DEDICATED
		if ( baseCmd[0] == '\\' || baseCmd[0] == '/' )
			baseCmd++;
#endif

		if( ( p = Field_FindFirstSeparator( cmd ) ) )
			Field_CompleteCommand( p + 1, qtrue, qtrue ); // Compound command
		else
			Cmd_CompleteArgument( baseCmd, cmd, completionArgument ); 
	}
	else {
		if ( completionString[0] == '\\' || completionString[0] == '/' )
			completionString++;

		matchCount = 0;
		shortestMatch[ 0 ] = 0;

		if ( strlen( completionString ) == 0 )
			return;

		if ( doCommands )
			Cmd_CommandCompletion( FindMatches );

		if ( doCvars )
			Cvar_CommandCompletion( FindMatches );

		if ( !Field_Complete() ) {
			// run through again, printing matches
			if ( doCommands )
				Cmd_CommandCompletion( PrintMatches );

			if ( doCvars )
				Cvar_CommandCompletion( PrintCvarMatches );
		}
	}
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete( field_t *field ) {
	if ( !field || !field->buffer[0] )
		return;

	completionField = field;

	Field_CompleteCommand( completionField->buffer, qtrue, qtrue );
}

//rwwRMG: Inserted:
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

/*
==================
Com_RandomBytes

fills string array with len radom bytes, peferably from the OS randomizer
==================
*/
void Com_RandomBytes( byte *string, int len )
{
	int i;

	if( Sys_RandomBytes( string, len ) )
		return;

	Com_Printf( "Com_RandomBytes: using weak randomization\n" );
	for( i = 0; i < len; i++ )
		string[i] = (unsigned char)( rand() % 255 );
}
