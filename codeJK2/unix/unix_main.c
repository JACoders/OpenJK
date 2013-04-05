#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <mntent.h>

#include <dlfcn.h>

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"

cvar_t *nostdout;

// Structure containing functions exported from refresh DLL
refexport_t	re;

unsigned	sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = qtrue;

// =======================================================================
// General routines
// =======================================================================

void Sys_BeginProfiling( void ) {
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) 
{
	IN_Shutdown();
	IN_Init();
}

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout && nostdout->value)
        return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

void Sys_Quit (void)
{
	CL_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	_exit(0);
}

void Sys_Init(void)
{
	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);

#if id386
	Sys_SetFPCW();
#endif

#if defined __linux__
#if defined __i386__
	Cvar_Set( "arch", "linux i386" );
#elif defined __alpha__
	Cvar_Set( "arch", "linux alpha" );
#elif defined __sparc__
	Cvar_Set( "arch", "linux sparc" );
#else
	Cvar_Set( "arch", "linux unknown" );
#endif
#elif defined __sun__
#if defined __i386__
	Cvar_Set( "arch", "solaris x86" );
#elif defined __sparc__
	Cvar_Set( "arch", "solaris sparc" );
#else
	Cvar_Set( "arch", "solaris unknown" );
#endif
#elif defined __sgi__
#if defined __mips__
	Cvar_Set( "arch", "sgi mips" );
#else
	Cvar_Set( "arch", "sgi unknown" );
#endif
#else
	Cvar_Set( "arch", "unknown" );
#endif

	IN_Init();

}

void	Sys_Error( const char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
    
    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	_exit (1);

} 

void Sys_Warn (char *warning, ...)
{ 
    va_list     argptr;
    char        string[1024];
    
    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	fprintf(stderr, "Warning: %s", string);
} 

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput(void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!com_dedicated || !com_dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = qfalse;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}

/*****************************************************************************/

static void *game_library;

#ifdef __i386__
	const char *gamename = "qagamei386.so";
#elif defined __alpha__
	const char *gamename = "qagameaxp.so";
#elif defined __mips__
	const char *gamename = "qagamemips.so";
#else
#error Unknown arch
#endif

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	Com_Printf("------ Unloading %s ------\n", gamename);
	if (game_library) 
		dlclose (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
	char	*path;
	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------\n", gamename);
	Com_sprintf (name, sizeof(name), "%s/%s", curpath, gamename);

	game_library = dlopen (name, RTLD_LAZY );
	if (game_library)
		Com_DPrintf ("LoadLibrary (%s)\n",name);
	else {
		Com_Printf( "LoadLibrary(\"%s\") failed\n", name);
		Com_Printf( "...reason: '%s'\n", dlerror() );
		Com_Error( ERR_FATAL, "Couldn't load game" );
	}

	GetGameAPI = (void *)dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

/*****************************************************************************/

static void *cgame_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadCGame (void)
{
	if (cgame_library) 
		dlclose (cgame_library);
	cgame_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetCGameAPI (void)
{
	void	*(*api) (void);

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
#ifdef __i386__
	const char *cgamename = "cgamei386.so";
#elif defined __alpha__
	const char *cgamename = "cgameaxp.so";
#elif defined __mips__
	const char *cgamename = "cgamemips.so";
#else
#error Unknown arch
#endif

	Sys_UnloadCGame();

	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------\n", cgamename);

	sprintf (name, "%s/%s", curpath, cgamename);
	cgame_library = dlopen (name, RTLD_LAZY );
	if (!cgame_library)
	{
		Com_Printf ("LoadLibrary (%s)\n",name);
		Com_Error( ERR_FATAL, "Couldn't load cgame: %s", dlerror() );
	}

	api = (void *)dlsym (cgame_library, "GetCGameAPI");
	if (!api)
	{
		Com_Error( ERR_FATAL, "dlsym() failed on GetCGameAPI" );
	}

	return api();
}

/*****************************************************************************/

static void *ui_library;

/*
=================
Sys_UnloadUI
=================
*/
void Sys_UnloadUI(void)
{
	if (ui_library) 
		dlclose (ui_library);
	ui_library = NULL;
}

/*
=================
Sys_GetUIAPI

Loads the ui dll
=================
*/
void *Sys_GetUIAPI (void)
{
	void	*api;

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
#ifdef __i386__
	const char *uiname = "uii386.so";
#elif defined __alpha__
	const char *uiname = "uiaxp.so";
#elif defined __mips__
	const char *uiname = "uimips.so";
#else
#error Unknown arch
#endif

	Sys_UnloadUI();

	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------\n", uiname);

	sprintf (name, "%s/%s", curpath, uiname);
	ui_library = dlopen (name, RTLD_LAZY );
	if (!ui_library)
	{
		Com_Printf ("LoadLibrary (%s)\n",name);
		Com_Error( ERR_FATAL, "Couldn't load ui: %s", dlerror() );
	}

	api = (void *)dlsym (ui_library, "GetUIAPI");
	if (!api)
	{
		Com_Error( ERR_FATAL, "dlsym() failed on GetUIAPI" );
	}

	return api;
}

/*****************************************************************************/

void *Sys_GetRefAPI (void *parms) 
{
	return (void *)GetRefAPI(REF_API_VERSION, parms);
}

/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

typedef struct {
#if 0
	HANDLE	threadHandle;
	int		threadId;
	CRITICAL_SECTION	crit;
#endif
	FILE	*file;
	byte	*buffer;
	qboolean	eof;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) 
{
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r;

//Loop here
//	EnterCriticalSection (&stream.crit);

	// if there is any space left in the buffer, fill it up
	if ( !stream.eof ) {
		count = stream.bufferSize - (stream.threadPosition - stream.streamPosition);
		if ( count ) {
			bufferPoint = stream.threadPosition % stream.bufferSize;
			buffer = stream.bufferSize - bufferPoint;
			readCount = buffer < count ? buffer : count;
			r = fread( stream.buffer + bufferPoint, 1, readCount, stream.file );
			stream.threadPosition += r;

			if ( r != readCount )
				stream.eof = qtrue;
		}
	}

//	LeaveCriticalSection (&stream.crit);
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) 
{
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) 
{
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) 
{
	if ( stream.file ) {
		Com_Error( ERR_FATAL, "Sys_BeginStreamedFile: unclosed stream");
	}

	stream.file = f;
	stream.buffer = Z_Malloc( readAhead );
	stream.bufferSize = readAhead;
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( FILE *f ) 
{
	if ( f != stream.file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
//	EnterCriticalSection( &stream.crit );

	stream.file = NULL;
	Z_Free( stream.buffer );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, FILE *f ) 
{
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	dest = (byte *)buffer;
	remaining = size * count;

	if ( remaining <= 0 ) {
		Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
	}

	sleepCount = 0;
	while ( remaining > 0 ) {
		available = stream.threadPosition - stream.streamPosition;
		if ( !available ) {
			if (stream.eof)
				break;
			Sys_StreamThread();
			continue;
		}

		bufferPoint = stream.streamPosition % stream.bufferSize;
		bufferCount = stream.bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if ( copy > remaining ) {
			copy = remaining;
		}
		memcpy( dest, stream.buffer + bufferPoint, copy );
		stream.streamPosition += copy;
		dest += copy;
		remaining -= copy;
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( FILE *f, int offset, int origin ) {

	// halt the thread
//	EnterCriticalSection( &stream.crit );

	// clear to that point
	fseek( f, offset, origin );
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;

	// let the thread start running at the new position
//	LeaveCriticalSection( &stream.crit );
}


/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		64
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	// in vga this calls KBD_Update, under X, it calls GetEvent
	Sys_SendKeyEvents ();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = malloc( len );
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for other input devices
	IN_Frame();

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = malloc( len );
		*buf = adr;
		memcpy( buf+1, netmsg.data, netmsg.cursize );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

char *Sys_GetClipboardData(void)
{
	return NULL;
}

void	Sys_Print( const char *msg )
{
	fputs(msg, stderr);
}

int main (int argc, char **argv)
{
	int 	oldtime, newtime;
	int		len, i;
	char	*cmdline;
	void SetProgramPath(char *path);

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid(getuid());

	SetProgramPath(argv[0]);

	// merge the command line, this is kinda silly
	for (len = 1, i = 1; i < argc; i++)
		len += strlen(argv[i]) + 1;
	cmdline = malloc(len);
	*cmdline = 0;
	for (i = 1; i < argc; i++) {
		if (i > 1)
			strcat(cmdline, " ");
		strcat(cmdline, argv[i]);
	}
	Com_Init(cmdline);
	NET_Init();

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}

    while (1)
    {
		// set low precision every frame, because some system calls
		// reset it arbitrarily
		Sys_LowFPPrecision ();    

        Com_Frame ();
    }
}

#if 0
/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize-1)) - psize;

//	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
//			addr, startaddr+length, length);

	r = mprotect((char*)addr, length + startaddr - addr + psize, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

}

#endif
