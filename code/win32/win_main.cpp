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

// win_main.h

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <crtdbg.h>

// The following macros set and clear, respectively, given bits
// of the C runtime library debug flag, as specified by a bitmask.

#ifdef   _DEBUG
#define  SET_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#else
#define  SET_CRT_DEBUG_FIELD(a)   ((void) 0)
#define  CLEAR_CRT_DEBUG_FIELD(a) ((void) 0)
#endif

#define MEM_THRESHOLD 128*1024*1024

static char		sys_cmdline[MAX_STRING_CHARS];

void Sys_SetBinaryPath(const char *path);
char *Sys_BinaryPath(void);

/*
==================
Sys_GetFileTime()
==================
*/
bool Sys_GetFileTime(LPCSTR psFileName, FILETIME &ft)
{
	bool bSuccess = false;
	HANDLE hFile = INVALID_HANDLE_VALUE;	

	hFile = CreateFile(	psFileName,	// LPCTSTR lpFileName,          // pointer to name of the file
						GENERIC_READ,			// DWORD dwDesiredAccess,       // access (read-write) mode
						FILE_SHARE_READ,		// DWORD dwShareMode,           // share mode
						NULL,					// LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// pointer to security attributes
						OPEN_EXISTING,			// DWORD dwCreationDisposition,  // how to create
						FILE_FLAG_NO_BUFFERING,// DWORD dwFlagsAndAttributes,   // file attributes
						NULL					// HANDLE hTemplateFile          // handle to file with attributes to 
						);

	if (hFile != INVALID_HANDLE_VALUE)
	{			
		if (GetFileTime(hFile,	// handle to file
						NULL,	// LPFILETIME lpCreationTime
						NULL,	// LPFILETIME lpLastAccessTime
						&ft		// LPFILETIME lpLastWriteTime
						)
			)
		{
			bSuccess = true;
		}

		CloseHandle(hFile);
	}

	return bSuccess;
}


/*
==================
Sys_FileOutOfDate
==================
*/
qboolean Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ )
{
	FILETIME ftFinalFile, ftDataFile;

	if (Sys_GetFileTime(psFinalFileName, ftFinalFile) && Sys_GetFileTime(psDataFileName, ftDataFile))
	{
		// timer res only accurate to within 2 seconds on FAT, so can't do exact compare...
		//
		//LONG l = CompareFileTime( &ftFinalFile, &ftDataFile );
		if (  ( abs( long( ftFinalFile.dwLowDateTime - ftDataFile.dwLowDateTime) ) <= 20000000 ) &&
				  ftFinalFile.dwHighDateTime == ftDataFile.dwHighDateTime				
			)
		{
			return false;	// file not out of date, ie use it.
		}
		return true;	// flag return code to copy over a replacement version of this file
	}


	// extra error check, report as suspicious if you find a file locally but not out on the net.,.
	//
	if (com_developer->integer)
	{
		if (!Sys_GetFileTime(psDataFileName, ftDataFile))
		{
			Com_Printf( "Sys_FileOutOfDate: reading %s but it's not on the net!\n", psFinalFileName);
		}
	}

	return false;
}

/*
==================
Sys_CopyFile
==================
*/
qboolean Sys_CopyFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, qboolean bOverWrite)
{
	qboolean bOk = qtrue;
	if (!CopyFile( lpExistingFileName, lpNewFileName, !bOverWrite ) && bOverWrite)
	{
		DWORD dwAttrs = GetFileAttributes(lpNewFileName);
		SetFileAttributes(lpNewFileName, dwAttrs & ~FILE_ATTRIBUTE_READONLY);
		bOk = CopyFile( lpExistingFileName, lpNewFileName, FALSE );
	}
	return bOk;
}

/*
==================
Sys_LowPhysicalMemory
==================
*/
qboolean Sys_LowPhysicalMemory() 
{
	static MEMORYSTATUSEX stat;
	static qboolean bAsked = qfalse;
	static cvar_t* sys_lowmem = Cvar_Get( "sys_lowmem", "0", 0 );

	if (!bAsked)	// just in case it takes a little time for GlobalMemoryStatus() to gather stats on
	{				//	stuff we don't care about such as virtual mem etc.
		bAsked = qtrue;
		GlobalMemoryStatusEx (&stat);
	}
	if (sys_lowmem->integer)
	{
		return qtrue;
	}
	return (stat.ullTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}


/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();
  	Com_ShutdownZoneMemory();
  	Com_ShutdownHunkMemory();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();
  	Com_ShutdownZoneMemory();
  	Com_ShutdownHunkMemory();

	exit (0);
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( void ) {
	return "";
}

/*
==============
Sys_DefaultBasePath
==============
*/
char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	int			findhandle;
	int			flag;
	int			i;

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **) Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_LISTFILES, qfalse);
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void	Sys_FreeFileList( char **filelist ) {
	int		i;

	if ( !filelist ) {
		return;
	}

	for ( i = 0 ; filelist[i] ; i++ ) {
		Z_Free( filelist[i] );
	}

	Z_Free( filelist );
}

//========================================================

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = (char *) GlobalLock( hClipboardData ) ) != 0 ) {
				data = (char *) Z_Malloc( GlobalSize( hClipboardData ) + 1, TAG_CLIPBOARD, qfalse);
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData )+1 );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( (struct HINSTANCE__ *)dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

//make sure the dll can be opened by the file system, then write the
//file back out again so it can be loaded is a library. If the read
//fails then the dll is probably not in the pk3 and we are running
//a pure server -rww
bool Sys_UnpackDLL(const char *name)
{
	void *data;
	fileHandle_t f;
	int len = FS_ReadFile(name, &data);

	if (len < 1)
	{ //failed to read the file (out of the pk3 if pure)
		return false;
	}

	if (FS_FileIsInPAK(name) == -1)
	{ //alright, it isn't in a pk3 anyway, so we don't need to write it.
		//this is allowable when running non-pure.
		FS_FreeFile(data);
		return true;
	}

	f = FS_FOpenFileWrite( name );
	if ( !f )
	{ //can't open for writing? Might be in use.
		//This is possibly a malicious user attempt to circumvent dll
		//replacement so we won't allow it.
		FS_FreeFile(data);
		return false;
	}

	if (FS_Write( data, len, f ) < len)
	{ //Failed to write the full length. Full disk maybe?
		FS_FreeFile(data);
		return false;
	}

	FS_FCloseFile( f );
	FS_FreeFile(data);

	return true;
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void *Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void *dllhandle = NULL;

	if(useSystemLib)
		Com_Printf("Trying to load \"%s\"...\n", name);
	
	if(!useSystemLib || !(dllhandle = Sys_LoadLibrary(name)))
	{
		const char *topDir;
		char libPath[MAX_OSPATH];
        
		topDir = Sys_BinaryPath();
        
		if(!*topDir)
			topDir = ".";
        
		Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, topDir);
		Com_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);
        
		if(!(dllhandle = Sys_LoadLibrary(libPath)))
		{
			const char *basePath = Cvar_VariableString("fs_basepath");
			
			if(!basePath || !*basePath)
				basePath = ".";
			
			if(FS_FilenameCompare(topDir, basePath))
			{
				Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
				Com_sprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				dllhandle = Sys_LoadLibrary(libPath);
			}
			
			if(!dllhandle)
			{
				const char *cdPath = Cvar_VariableString("fs_cdpath");

				if(!basePath || !*basePath)
					basePath = ".";

				if(FS_FilenameCompare(topDir, cdPath))
				{
					Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, cdPath);
					Com_sprintf(libPath, sizeof(libPath), "%s%c%s", cdPath, PATH_SEP, name);
					dllhandle = Sys_LoadLibrary(libPath);
				}

				if(!dllhandle)
				{
					Com_Printf("Loading \"%s\" failed\n", name);
				}
			}
		}
	}
	
	return dllhandle;
}


/*
========================================================================

GAME DLL

========================================================================
*/
static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame( void ) {
	if ( !game_library ) {
		return;
	}
	if ( !FreeLibrary (game_library) ) {
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	}
	game_library = NULL;
}

/*
=================
Sys_UnloadGamePending
This function is kind of redundant in Windows, but the extra
function is needed because of Linux/Mac version being different.
=================
*/
void Sys_UnloadGamePending() {
	Sys_UnloadGame();
}

/*
=================
Sys_DelayedUnloadGame
=================
*/
void Sys_DelayedUnloadGame()
{
	HINSTANCE save = game_library;
	Sys_UnloadGame();
	game_library = save;
}

/*
=================
Sys_RetrieveDLL

OpenJK Function.
Retrieve the DLL using fs_game
=================
*/

static HINSTANCE Sys_RetrieveDLL( const char *gamename, const char *debugdir )
{
	char	cwd[MAX_OSPATH];
	char	name[MAX_OSPATH];

	HINSTANCE retVal;

	cvar_t *moddir = Cvar_Get("fs_game", "", CVAR_INIT|CVAR_SERVERINFO);

	// First search path: mod dir, debug/release folders
	_getcwd(cwd, sizeof(cwd));
	Com_sprintf(name, sizeof(name), "%s/%s/%s/%s", cwd, moddir->string, debugdir, gamename);
	retVal = LoadLibrary(name);
	if(retVal)
		goto successful;

	// Second search path: mod dir
	Com_sprintf(name, sizeof(name), "%s/%s/%s", cwd, moddir->string, gamename);
	retVal = LoadLibrary(name);
	if(retVal)
		goto successful;

	// Third/last search path: gamedata folder
	Com_sprintf(name, sizeof(name), "%s/%s", cwd, gamename);
	retVal = LoadLibrary(name);

successful:
	Com_DPrintf("LoadLibrary (%s)\n", name);
	return retVal;
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

	const char *gamename;
	if(Cvar_VariableIntegerValue("com_jk2"))
	{
		gamename = "jk2game" ARCH_STRING DLL_EXT;
	}
	else
	{
		gamename = "jagame" ARCH_STRING DLL_EXT;
	}

#if id386
#ifdef NDEBUG
	const char *debugdir = "release";
#elif MEM_DEBUG
	const char *debugdir = "shdebug";
#else
	const char *debugdir = "debug";
#endif	//NDEBUG
#elif idx64
#ifdef NDEBUG
	const char *debugdir = "release64";
#elif MEM_DEBUG
	const char *debugdir = "shdebug64";
#else
	const char *debugdir = "debug64";
#endif	//NDEBUG
#elif defined _M_ALPHA
#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif //NDEBUG
#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	game_library = Sys_RetrieveDLL(gamename, debugdir);
	if(!game_library)
	{
		char *buf;

		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL );

		Com_Printf( "LoadLibrary(\"%s\") failed\n", gamename);
		Com_Printf( "...reason: '%s'\n", buf );
		Com_Error( ERR_FATAL, "Couldn't load game" );
	}

	GetGameAPI = (void *(*)(void *))GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}
	return GetGameAPI (parms);
}


/*
=================
Sys_LoadCgame

Used to hook up a development dll
=================
*/
void * Sys_LoadCgame( intptr_t (**entryPoint)(int, ...), intptr_t (*systemcalls)(intptr_t, ...) ) 
{
	void	(*dllEntry)( intptr_t (*syscallptr)(intptr_t, ...) );

	dllEntry = ( void (*)( intptr_t (*)( intptr_t, ... ) ) )GetProcAddress( game_library, "dllEntry" ); 
	*entryPoint = (intptr_t (*)(int,...))GetProcAddress( game_library, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( game_library );
		return NULL;
	}

	dllEntry( systemcalls );
	return game_library;
}

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
	HANDLE	threadHandle;
	int		threadId;
	CRITICAL_SECTION	crit;
	fileHandle_t	file;
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
void Sys_StreamThread( void ) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r;

	while (1) {
		Sleep( 10 );
		EnterCriticalSection (&stream.crit);

		// if there is any space left in the buffer, fill it up
		while ( !stream.eof ) {
			count = stream.bufferSize - (stream.threadPosition - stream.streamPosition);
			if ( !count ) {
				break;
			}

			bufferPoint = stream.threadPosition % stream.bufferSize;
			buffer = stream.bufferSize - bufferPoint;
			readCount = buffer < count ? buffer : count;

			r = FS_Read( stream.buffer + bufferPoint, readCount, stream.file );
			stream.threadPosition += r;

			if ( r != readCount ) {
				stream.eof = qtrue;
				break;
			}
		}

		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) {

	InitializeCriticalSection ( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   (unsigned long *) &stream.threadId);
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
	if ( stream.file ) {
		Sys_EndStreamedFile( stream.file );
	}

	stream.file = f;
	stream.buffer = (unsigned char *) Z_Malloc( readAhead );
	stream.bufferSize = readAhead;
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;

	// let the thread start running
	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) {
	if ( f != stream.file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection( &stream.crit );

	stream.file = 0;
	Z_Free( stream.buffer );

}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
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
			if ( stream.eof ) {
				break;
			}
			if ( sleepCount == 1 ) {
				Com_DPrintf( "Sys_StreamedRead: waiting\n" );
			}
			if ( ++sleepCount > 100 ) {
				Com_Error( ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep( 10 );
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

	// halt the thread
	EnterCriticalSection( &stream.crit );

	// clear to that point
	FS_Seek( f, offset, origin );
	stream.streamPosition = 0;
	stream.threadPosition = 0;
	stream.eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection( &stream.crit );
}

#endif


/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
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
    MSG			msg;
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if ( !GetMessage (&msg, NULL, 0, 0) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = (char *) Z_Malloc( len, TAG_EVENT, qfalse);
		Q_strncpyz( b, s, len );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

#if MEM_DEBUG
void SH_Register(void);
#endif

void Sys_Init( void ) {
	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
#if MEM_DEBUG
	SH_Register();
#endif

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("This game requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("This game doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();		// FIXME: not in dedicated?
}



// do a quick mem test to check for any potential future mem problems...
//
static void QuickMemTest(void)
{
//	if (!Sys_LowPhysicalMemory())
	{
		const int iMemTestMegs = 128;	// useful search label
		// special test, 
		void *pvData = malloc(iMemTestMegs * 1024 * 1024);
		if (pvData)
		{
			free(pvData);
		}
		else
		{
			// err...
			//
			LPCSTR psContinue = re.Language_IsAsian() ? 
								"Your machine failed to allocate %dMB in a memory test, which may mean you'll have problems running this game all the way through.\n\nContinue anyway?"
								: 
								SE_GetString("CON_TEXT_FAILED_MEMTEST");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

			#define GetYesNo(psQuery)	(!!(MessageBox(NULL,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
			if (!GetYesNo(va(psContinue,iMemTestMegs)))
			{
				LPCSTR psNoMem = re.Language_IsAsian() ?
								"Insufficient memory to run this game!\n"
								:
								SE_GetString("CON_TEXT_INSUFFICIENT_MEMORY");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

				Com_Error( ERR_FATAL, psNoMem );
			}
		}
	}
}

/* Begin Sam Lantinga Public Domain 4/13/98 */

static void UnEscapeQuotes(char *arg)
{
	char *last = NULL;

	while (*arg) {
		if (*arg == '"' && (last != NULL && *last == '\\')) {
			char *c_curr = arg;
			char *c_last = last;

			while (*c_curr) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;

	argc = last_argc = 0;
	for (bufp = cmdline; *bufp;) {
		/* Skip leading whitespace */
		while (isspace(*bufp)) {
			++bufp;
		}
		/* Skip over argument */
		if (*bufp == '"') {
			++bufp;
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			lastp = bufp;
			while (*bufp && (*bufp != '"' || *lastp == '\\')) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && !isspace(*bufp)) {
				++bufp;
			}
		}
		if (*bufp) {
			if (argv) {
				*bufp = '\0';
			}
			++bufp;
		}

		/* Strip out \ from \" sequences */
		if (argv && last_argc != argc) {
			UnEscapeQuotes(argv[last_argc]);
		}
		last_argc = argc;
	}
	if (argv) {
		argv[argc] = NULL;
	}
	return (argc);
}

/* End Sam Lantinga Public Domain 4/13/98 */

//=======================================================================
//int	totalMsec, countMsec;

#ifndef DEFAULT_BASEDIR
#	define DEFAULT_BASEDIR Sys_BinaryPath()
#endif

int main ( int argc, char **argv )
{
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };
//	int			startTime, endTime;

   SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF );
//   _CrtSetBreakAlloc(34804);

	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

	Sys_InitStreamThread();

	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( int i = 1; i < argc; i++ )
	{
		const bool containsSpaces = (strchr(argv[i], ' ') != NULL);
		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	Com_Init( commandLine );

	QuickMemTest();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized ) {
			Sleep( 5 );
		}

#ifdef _DEBUG
		if (!g_wv.activeApp)
		{
			Sleep(50);
		}
#endif // _DEBUG

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();
	}
}

/*
==================
WinMain

==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}

	/* Begin Sam Lantinga Public Domain 4/13/98 */

	TCHAR *text = GetCommandLine();
	char *cmdline = _strdup(text);
	if ( cmdline == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	int    argc = ParseCommandLine(cmdline, NULL);
	char **argv = (char **)alloca(sizeof(char *) * argc + 1);
	if ( argv == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	ParseCommandLine(cmdline, argv);

	/* End Sam Lantinga Public Domain 4/13/98 */

	g_wv.hInstance = hInstance;

	/* Begin Sam Lantinga Public Domain 4/13/98 */

	main(argc, argv);

	free(cmdline);

	/* End Sam Lantinga Public Domain 4/13/98 */

	// never gets here
	return 0;
}
