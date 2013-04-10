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
#ifdef __linux__ // rb010123
#include <mntent.h>
#endif
#include <dlfcn.h>

#ifdef __linux__
#include <fpu_control.h> // bk001213 - force dumps on divide by zero
#endif


#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "../qcommon/platform.h"

#include "linux_local.h" // bk001204

cvar_t *nostdout;

// Structure containing functions exported from refresh DLL
refexport_t	re;

unsigned	sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = qtrue;

// =======================================================================
// General routines
// =======================================================================

// bk001207 
#define MEM_THRESHOLD 96*1024*1024
/*
==================
Sys_LowPhysicalMemory()
==================
*/
qboolean Sys_LowPhysicalMemory() {
  //MEMORYSTATUS stat;
  //GlobalMemoryStatus (&stat);
  //return (stat.dwTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
  return qfalse; // bk001207 - FIXME
}

/*
==================
Sys_FunctionCmp
==================
*/
int Sys_FunctionCmp(void *f1, void *f2) {
	return qtrue;
}

/*
==================
Sys_FunctionCheckSum
==================
*/
int Sys_FunctionCheckSum(void *f1) {
	return 0;
}

/*
==================
Sys_MonkeyShouldBeSpanked
==================
*/
int Sys_MonkeyShouldBeSpanked( void ) {
	return 0;
}

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

// bk010104 - added for abstraction
void Sys_Exit( int ex ) {
#ifdef NDEBUG // regular behavior
  // We can't do this 
  //  as long as GL DLL's keep installing with atexit...
  //exit(ex);
  _exit(ex);
#else
  // Give me a backtrace on error exits.
  assert( ex == 0 );
  exit(ex);
#endif
}


void Sys_Quit (void) {
  CL_Shutdown ();
  fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
  Sys_Exit(0);
}

void Sys_Init(void)
{
	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);

#if defined __linux__
#if defined __i386__
	Cvar_Set( "arch", "linux i386" );
#elif defined __alpha__
	Cvar_Set( "arch", "linux alpha" );
#elif defined __sparc__
	Cvar_Set( "arch", "linux sparc" );
#elif defined __FreeBSD__

#if defined __i386__ // FreeBSD
        Cvar_Set( "arch", "freebsd i386" );
#elif defined __alpha__
        Cvar_Set( "arch", "freebsd alpha" );
#else
        Cvar_Set( "arch", "freebsd unknown" );
#endif // FreeBSD

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

	Cvar_Set( "username", Sys_GetCurrentUser() );

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
    fprintf(stderr, "Sys_Error: %s\n", string);
    
    Sys_Exit( 1 ); // bk010104 - use single exit point.
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

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
  // bk001206 - verbose error reporting
  const char* err; // rb010123 - now const
  if ( !dllHandle ) {
    Com_Printf("Sys_UnloadDll(NULL)\n");
    return;
  }
  dlclose( dllHandle );
  err = dlerror();
  if ( err != NULL )
    Com_Printf ( "Sys_UnloadGame failed on dlclose: \"%s\"!\n", err );
}


/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
=================
*/
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

void *Sys_LoadDll( const char *name, 
		   int (**entryPoint)(int, ...),
		   int (*systemcalls)(int, ...) ) 
{
  void *libHandle;
  void	(*dllEntry)( int (*syscallptr)(int, ...) );
  char	curpath[MAX_OSPATH];
  char	fname[MAX_OSPATH];
  //char	loadname[MAX_OSPATH];
  char	*basepath;
  char	*cdpath;
  char	*gamedir;
  char	*fn;
  const char*  err = NULL; // bk001206 // rb0101023 - now const

  // bk001206 - let's have some paranoia
  assert( name );

  getcwd(curpath, sizeof(curpath));
#if defined __i386__
#ifndef NDEBUG
  snprintf (fname, sizeof(fname), "%si386-debug.%s", name, DLL_EXT); // bk010205 - different DLL name
#else
  snprintf (fname, sizeof(fname), "%si386.%s", name, DLL_EXT);
#endif
#elif defined __x86_64__
#ifndef NDEBUG
  snprintf (fname, sizeof(fname), "%sx86_64-debug.%s", name, DLL_EXT); // bk010205 - different DLL name
#else
  snprintf (fname, sizeof(fname), "%sx86_64.%s", name, DLL_EXT);
#endif
#elif defined __powerpc__   //rcg010207 - PPC support.
  snprintf (fname, sizeof(fname), "%sppc.%s", name, DLL_EXT);
#elif defined __axp__
  snprintf (fname, sizeof(fname), "%saxp.%s", name, DLL_EXT);
#elif defined __mips__
  snprintf (fname, sizeof(fname), "%smips.%s", name, DLL_EXT);
#else
#error Unknown arch
#endif

// bk001129 - was RTLD_LAZY 
#define Q_RTLD    RTLD_NOW

#if 0 // bk010205 - was NDEBUG // bk001129 - FIXME: what is this good for?
  // bk001206 - do not have different behavior in builds
  Q_strncpyz(loadname, curpath, sizeof(loadname));
  // bk001129 - from cvs1.17 (mkv)
  Q_strcat(loadname, sizeof(loadname), "/");
  
  Q_strcat(loadname, sizeof(loadname), fname);
  Com_Printf( "Sys_LoadDll(%s)... \n", loadname );
  libHandle = dlopen( loadname, Q_RTLD );
  //if ( !libHandle ) {
  // bk001206 - report any problem
  //Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", loadname, dlerror() );
#endif // bk010205 - do not load from installdir

  basepath = Cvar_VariableString( "fs_basepath" );
  cdpath = Cvar_VariableString( "fs_cdpath" );
  gamedir = Cvar_VariableString( "fs_game" );
  
  fn = FS_BuildOSPath( basepath, gamedir, fname );
  // bk001206 - verbose
  Com_Printf( "Sys_LoadDll(%s)... \n", fn );
  
  // bk001129 - from cvs1.17 (mkv), was fname not fn
  libHandle = dlopen( fn, Q_RTLD );
 
#ifndef NDEBUG
  if (libHandle == NULL)  Com_Printf("Failed to open DLL\n");
#endif
 
  if ( !libHandle ) {
    if( cdpath[0] ) {
      // bk001206 - report any problem
      Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, dlerror() );
      
      fn = FS_BuildOSPath( cdpath, gamedir, fname );
      libHandle = dlopen( fn, Q_RTLD );
      if ( !libHandle ) {
	// bk001206 - report any problem
	Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, dlerror() );
      }
      else
	Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn );
    }
    else
      Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn );
    
    if ( !libHandle ) {
#ifdef NDEBUG // bk001206 - in debug abort on failure
      Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed dlopen() completely!\n", name  );
#else
      Com_Printf ( "Sys_LoadDll(%s) failed dlopen() completely!\n", name );
#endif
      return NULL;
    }
  }
  // bk001206 - no different behavior
  //#ifndef NDEBUG }
  //else Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", loadname );
  //#endif

  dllEntry = (void (*)(int (*)(int,...))) dlsym( libHandle, "dllEntry" ); 
  if (!dllEntry)
  {
     err = dlerror();
     Com_Printf("Sys_LoadDLL(%s) failed dlsym(dllEntry): \"%s\" ! \n",name,err);
  }
  //int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
  *entryPoint = (int(*)(int,...))dlsym( libHandle, "vmMain" );
  if (!*entryPoint)
     err = dlerror();
  if ( !*entryPoint || !dllEntry ) {
#ifdef NDEBUG // bk001206 - in debug abort on failure
    Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed dlsym(vmMain): \"%s\" !\n", name, err );
#else
    Com_Printf ( "Sys_LoadDll(%s) failed dlsym(vmMain): \"%s\" !\n", name, err );
#endif
    dlclose( libHandle );
    err = dlerror();
    if ( err != NULL )
      Com_Printf ( "Sys_LoadDll(%s) failed dlcose: \"%s\"\n", name, err );
    return NULL;
  }
  Com_Printf ( "Sys_LoadDll(%s) found **vmMain** at  %p  \n", name, *entryPoint ); // bk001212
  dllEntry( systemcalls );
  Com_Printf ( "Sys_LoadDll(%s) succeeded!\n", name );
  return libHandle;
}


#if 0 // bk010215 - scheduled for full deletion
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
void Sys_UnloadGame (void) {
  // bk001206 - this code is never used
  assert(0);

  Com_Printf("------ Unloading %s ------\n", gamename);
  if (game_library) {
    dlclose (game_library);
    game_library = NULL;
  }
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
	//char	*path; // bk001204 - unused

  // bk001206 - this code is never used
  assert(0);

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
  // bk001206 - this code is never used
  assert(0);
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

  // bk001206 - this code is never used
  assert(0);

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
  // bk001206 - this code is never used
  assert(0);
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
	void	*(*api)(void);

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

	// bk001206 - this code is never used
	assert(0);
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

	api = (void *(*)(void))dlsym (ui_library, "GetUIAPI");
	if (!api)
	{
		Com_Error( ERR_FATAL, "dlsym() failed on GetUIAPI" );
	}

	return api();
}

/*****************************************************************************/

static void *botlib_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadBotLib (void)
{
  // bk001206 - this code is never used
  assert(0);
	if (botlib_library) 
		dlclose (botlib_library);
	botlib_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetBotLibAPI (void *parms )
{
	void	*(*GetBotLibAPI) (void *);
	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
#ifdef __i386__
	const char *botlibname = "qaboti386.so";
#elif defined __alpha__
	const char *botlibname = "qabotaxp.so";
#elif defined __mips__
	const char *botlibname = "qabotmips.so";
#else
#error Unknown arch
#endif
	// bk001129 - this code is never used
	assert(0);

	Sys_UnloadBotLib();
      
	getcwd(curpath, sizeof(curpath));

	Com_Printf("------- Loading %s -------\n", botlibname);
    
	sprintf (name, "%s/%s", curpath, botlibname);\
	// bk001129 - was  RTLD_LAZY
	botlib_library = dlopen (name, RTLD_NOW );
	if (!botlib_library)
	{
		Com_Printf ("LoadLibrary (%s)\n",name);
		Com_Error( ERR_FATAL, "Couldn't load botlib: %s", dlerror() );
	}

	GetBotLibAPI = (void *)dlsym (botlib_library, "GetBotLibAPI");
	if (!GetBotLibAPI)
	{
		Sys_UnloadBotLib ();
		Com_Error( ERR_FATAL, "dlsym() failed on GetBotLibAPI" );
	}

	// bk001129 - this is a signature mismatch
	return GetBotLibAPI (parms);
}

void *Sys_GetBotAIAPI (void *parms ) {
	return NULL;
}

/*****************************************************************************/
#endif // bk010215


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}

#else

typedef struct {
	fileHandle_t file;
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

	// if there is any space left in the buffer, fill it up
	if ( !stream.eof ) {
		count = stream.bufferSize - (stream.threadPosition - stream.streamPosition);
		if ( count ) {
			bufferPoint = stream.threadPosition % stream.bufferSize;
			buffer = stream.bufferSize - bufferPoint;
			readCount = buffer < count ? buffer : count;
			r = FS_Read ( stream.buffer + bufferPoint, readCount, stream.file );
			stream.threadPosition += r;

			if ( r != readCount )
				stream.eof = qtrue;
		}
	}
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
	stream.buffer = Z_Malloc( readAhead,TAG_FILESYS,qfalse );
	stream.bufferSize = readAhead;
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) 
{
	if ( f != stream.file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}

	stream.file = 0;
	Z_Free( stream.buffer );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) 
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
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
	// clear to that point
	FS_Seek( f, offset, origin );
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

// bk000306: upped this from 64
#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
// bk000306: initialize
int		eventHead = 0;
int             eventTail = 0;
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

	// bk000305 - was missing
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
	  Com_Printf("Sys_QueEvent: overflow\n");
	  // we are discarding an event, but don't leak memory
	  if ( ev->evPtr ) {
	    Z_Free( ev->evPtr );
	  }
	  eventTail++;
	}

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
		b = (char *)Z_Malloc( len,TAG_EVENT,qfalse );
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
		buf = (netadr_t *)Z_Malloc( len,TAG_EVENT,qfalse );
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

qboolean Sys_CheckCD( void ) {
	return qtrue;
}

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


void    Sys_ConfigureFPU() { // bk001213 - divide by zero
#ifdef __linux2__
#ifdef __i386
#ifndef NDEBUG
  // bk0101022 - enable FPE's in debug mode
  static int fpu_word = _FPU_DEFAULT & ~(_FPU_MASK_ZM | _FPU_MASK_IM);
  int current = 0;
  _FPU_GETCW(current);
  if ( current!=fpu_word) {
#if 0
    Com_Printf("FPU Control 0x%x (was 0x%x)\n", fpu_word, current );
    _FPU_SETCW( fpu_word );
    _FPU_GETCW( current );
    assert(fpu_word==current);
#endif
  }
#else // NDEBUG
  static int fpu_word = _FPU_DEFAULT;
  _FPU_SETCW( fpu_word );
#endif // NDEBUG
#endif // __i386 
#endif // __linux
}


void Sys_PrintBinVersion( const char* name ) {
  char* date = __DATE__;
  char* time = __TIME__;
  char* sep = "==============================================================";
  fprintf( stdout, "\n\n%s\n", sep );
#ifdef DEDICATED
  fprintf( stdout, "Linux Quake3 Dedicated Server [%s %s]\n", date, time );  
#else
  fprintf( stdout, "Linux Quake3 Full Executable  [%s %s]\n", date, time );  
#endif
  fprintf( stdout, " local install: %s\n", name );
  fprintf( stdout, "%s\n\n", sep );
}

void Sys_ParseArgs( int argc, char* argv[] ) {

  if ( argc==2 ) {
    if ( (!strcmp( argv[1], "--version" ))
	 || ( !strcmp( argv[1], "-v" )) )
    {
      Sys_PrintBinVersion( argv[0] );
      Sys_Exit(0);
    }
  }
}

#include "../client/client.h"
extern clientStatic_t	cls;

int main ( int argc, char* argv[] )
{
  // int 	oldtime, newtime; // bk001204 - unused
	int		len, i;
	char	*cmdline;
	void Sys_SetDefaultCDPath(const char *path);

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid(getuid());

	Sys_ParseArgs( argc, argv ); 	// bk010104 - added this for support

	Sys_SetDefaultCDPath(argv[0]);

	// merge the command line, this is kinda silly
	for (len = 1, i = 1; i < argc; i++)
		len += strlen(argv[i]) + 1;
	cmdline = (char *)malloc(len);
	*cmdline = 0;
	for (i = 1; i < argc; i++) {
		if (i > 1)
			strcat(cmdline, " ");
		strcat(cmdline, argv[i]);
	}

	// bk000306 - clear queues
	memset( &eventQue[0], 0, MAX_QUED_EVENTS*sizeof(sysEvent_t) ); 
	memset( &sys_packetReceived[0], 0, MAX_MSGLEN*sizeof(byte) );

	Com_Init(cmdline);
	NET_Init();

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	}

    while (1) {
#ifdef __linux__
      Sys_ConfigureFPU();
#endif
        Com_Frame ();
    }
}
