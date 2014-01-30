// win_main.c
//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "client/client.h"
#include "qcommon/qcommon.h"
#include "win32/win_local.h"
#include "win32/resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "qcommon/stringed_ingame.h"

#define MEM_THRESHOLD 128*1024*1024

/* win_shared.cpp */
void Sys_SetBinaryPath(const char *path);
char *Sys_BinaryPath(void);

void *Sys_GetBotAIAPI (void *parms ) {
	return NULL;
}

// We now expect newlines instead of always appending
// otherwise sectioned prints get messed up.
#define MAXPRINTMSG		4096
void Conbuf_AppendText( const char *pMsg )
{
	char msg[MAXPRINTMSG] = {0};
	Q_strncpyz(msg, pMsg, sizeof(msg));
	Q_StripColor(msg);
	printf("%s", msg);
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory(void) {
	static MEMORYSTATUSEX stat;
	static qboolean bAsked = qfalse;
	if (!bAsked)	// just in case it takes a little time for GlobalMemoryStatusEx() to gather stats on
	{				//	stuff we don't care about such as virtual mem etc.
		bAsked = qtrue;
		GlobalMemoryStatusEx (&stat);
	}
	return (stat.ullTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
===============
PrintMatches

===============
*/
static char g_consoleField1[256];
static char g_consoleField2[256];

static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, g_consoleField1, strlen( g_consoleField1 ) ) ) {
		printf( "    %s\n", s );
	}
}

//qboolean stdin_active = qtrue;
char *Sys_ConsoleInput(void)
{
	const char ClearLine[] = "\r                                                                               \r";

	static int	len=0;
	static bool bPendingExtended = false;

	if (!kbhit()) return NULL;

	if (len == 0) memset(g_consoleField1,0,sizeof(g_consoleField1));

	g_consoleField1[len] = getch();

	if (bPendingExtended)
	{
		switch (g_consoleField1[len])
		{
			case 'H':	//up
				strcpy(g_consoleField1, g_consoleField2);
				printf(ClearLine);
				printf("%s",g_consoleField1);
				len = strlen(g_consoleField1);
			break;

			case 'K':	//left
			break;

			case 'M':	//right
			break;

			case 'P':	//down
			break;
		}
		g_consoleField1[len] = 0;	//erase last key hit
		bPendingExtended = false;
	}
	else
	switch ((unsigned char) g_consoleField1[len])
	{
	case 0x00:	//fkey is next
	case 0xe0:	//extended = arrow keys
		g_consoleField1[len] = 0;	//erase last key hit
		bPendingExtended = true;
		break;
	case 8: // backspace
		printf("%c %c",g_consoleField1[len],g_consoleField1[len]);
		g_consoleField1[len] = 0;
		if (len > 0) len--;
		g_consoleField1[len] = 0;
		break;
	case 9:	//Tab
		if (len) {
			g_consoleField1[len] = 0;	//erase last key hit
			printf( "\n");
			// run through again, printing matches
			Cmd_CommandCompletion( PrintMatches );
			Cvar_CommandCompletion( PrintMatches );
			printf( "\n%s", g_consoleField1);
		}
		break;
	case 27: // esc
		// clear the line
		printf(ClearLine);
		len = 0;
		break;
	case '\r':	//enter
		g_consoleField1[len] = 0;	//erase last key hit
		printf("\n");
		if (len) {
			len = 0;
			strcpy(g_consoleField2, g_consoleField1);
			return g_consoleField1;
		}
		break;
	case 'v' - 'a' + 1:	// ctrl-v is paste
		g_consoleField1[len] = 0;	//erase last key hit
		char *cbd;
		cbd = Sys_GetClipboardData();
		if (cbd) {
			strncpy (&g_consoleField1[len], cbd, sizeof(g_consoleField1) );
			printf("%s",cbd);
			len += strlen(cbd);
			Z_Free( cbd );
			if (len == sizeof(g_consoleField1))
			{
				len = 0;
				return g_consoleField1;
			}
		}
		break;
	default:
		printf("%c",g_consoleField1[len]);
		len++;
		if (len == sizeof(g_consoleField1))
		{
			len = 0;
			return g_consoleField1;
		}
		break;
	}

	return NULL;
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
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
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

//	Sys_SetErrorText( text );
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

//	Sys_DestroyConsole();
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
//	Sys_DestroyConsole();
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
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( msg, "[skipnotify]", 12 ) ) {
		msg += 12;
	}
	if ( msg[0] == '*' ) {
		msg += 1;
	}
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir( const char *path ) {
	if( !CreateDirectory( path, NULL ) )
	{
		if( GetLastError( ) != ERROR_ALREADY_EXISTS )
			return qfalse;
	}
	return qtrue;
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

/* Resolves path names and determines if they are the same */
/* For use with full OS paths not quake paths */
/* Returns true if resulting paths are valid and the same, otherwise false */
bool Sys_PathCmp( const char *path1, const char *path2 ) {
	char *r1, *r2;

	r1 = _fullpath(NULL, path1, MAX_OSPATH);
	r2 = _fullpath(NULL, path2, MAX_OSPATH);

	if(r1 && r2 && !Q_stricmp(r1, r2))
	{
		free(r1);
		free(r2);
		return true;
	}

	free(r1);
	free(r2);
	return false;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **psList, int *numfiles ) {
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	intptr_t	findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, psList, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		psList[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}

static qboolean strgtr(const char *s0, const char *s1) {
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t	findhandle;
	int			flag;
	int			i;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

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

	listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

void	Sys_FreeFileList( char **psList ) {
	int		i;

	if ( !psList ) {
		return;
	}

	for ( i = 0 ; psList[i] ; i++ ) {
		Z_Free( psList[i] );
	}

	Z_Free( psList );
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
			if ( ( cliptext = (char *)GlobalLock( hClipboardData ) ) != 0 ) {
				data = (char *)Z_Malloc( GlobalSize( hClipboardData ) + 1, TAG_CLIPBOARD);
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
	int ck;

	if (len < 1)
	{ //failed to read the file (out of the pk3 if pure)
		return false;
	}

	if (FS_FileIsInPAK(name, &ck) == -1)
	{ //alright, it isn't in a pk3 anyway, so we don't need to write it.
		//this is allowable when running non-pure.
		FS_FreeFile(data);
		return true;
	}

	f = FS_FOpenFileWrite( name, qfalse );
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
				Com_Printf("Loading \"%s\" failed\n", name);
			}
		}
	}

	return dllhandle;
}

/*
=================
Sys_LoadGameDll

Used to load a development dll instead of a virtual machine
=================
*/

void * QDECL Sys_LoadLegacyGameDll( const char *name, intptr_t (QDECL **vmMain)(int, ...), intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {
	static int	lastWarning = 0;
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( intptr_t (QDECL *syscallptr)(intptr_t, ...) );
	char	*basepath;
	char	*homepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
	char	filename[MAX_QPATH];

	Com_sprintf( filename, sizeof( filename ), "%s" ARCH_STRING DLL_EXT, name );

	if (!Sys_UnpackDLL(filename))
	{
		if ( com_developer->integer )
			Com_Printf ("Sys_LoadLegacyGameDll: Failed to unpack %s" ARCH_STRING DLL_EXT " from PK3.\n", name);

		return NULL;
	}

	libHandle = LoadLibrary( filename );
	if ( !libHandle ) {
		basepath = Cvar_VariableString( "fs_basepath" );
		homepath = Cvar_VariableString( "fs_homepath" );
		cdpath = Cvar_VariableString( "fs_cdpath" );
		gamedir = Cvar_VariableString( "fs_game" );

		fn = FS_BuildOSPath( basepath, gamedir, filename );
		libHandle = LoadLibrary( fn );

		if ( !libHandle ) {
			if( homepath[0] ) {
				fn = FS_BuildOSPath( homepath, gamedir, filename );
				libHandle = LoadLibrary( fn );
			}
			if ( !libHandle ) {
				if( cdpath[0] ) {
					fn = FS_BuildOSPath( cdpath, gamedir, filename );
					libHandle = LoadLibrary( fn );
				}
				if ( !libHandle ) {
					return NULL;
				}
			}
		}
	}

	dllEntry = ( void (QDECL *)( intptr_t (QDECL *)( intptr_t, ... ) ) )GetProcAddress( libHandle, "dllEntry" );
	*vmMain = (intptr_t (QDECL *)(int,...))GetProcAddress( libHandle, "vmMain" );
	if ( !*vmMain || !dllEntry ) {
		if ( com_developer->integer )
			Com_Printf ("Sys_LoadLegacyGameDll: Entry point not found in %s" ARCH_STRING DLL_EXT ". Failed with system error code 0x%X.\n", name, GetLastError());

		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	return libHandle;
}

void *QDECL Sys_LoadGameDll( const char *name, void *(QDECL **moduleAPI)(int, ...) ) {
	HINSTANCE	libHandle;
	char	*basepath, *homepath, *cdpath, *gamedir;
	char	*fn;
	char	filename[MAX_QPATH];

	Com_sprintf( filename, sizeof( filename ), "%s"ARCH_STRING DLL_EXT, name );

	if (!Sys_UnpackDLL(filename))
	{
		if ( com_developer->integer )
			Com_Printf ("Sys_LoadGameDll: Failed to unpack %s" ARCH_STRING DLL_EXT " from PK3.\n", name);

		return NULL;
	}

	libHandle = LoadLibrary( filename );
	if ( !libHandle ) {
		basepath = Cvar_VariableString( "fs_basepath" );
		homepath = Cvar_VariableString( "fs_homepath" );
		cdpath = Cvar_VariableString( "fs_cdpath" );
		gamedir = Cvar_VariableString( "fs_game" );

		fn = FS_BuildOSPath( basepath, gamedir, filename );
		libHandle = LoadLibrary( fn );

		if ( !libHandle ) {
			if( homepath[0] ) {
				fn = FS_BuildOSPath( homepath, gamedir, filename );
				libHandle = LoadLibrary( fn );
			}
			if ( !libHandle ) {
				if( cdpath[0] ) {
					fn = FS_BuildOSPath( cdpath, gamedir, filename );
					libHandle = LoadLibrary( fn );
				}
				if ( !libHandle ) {
					return NULL;
				}
			}
		}
	}

	*moduleAPI = (void *(QDECL *)(int,...))GetProcAddress( libHandle, "GetModuleAPI" );
	if ( !*moduleAPI ) {
		if ( com_developer->integer )
			Com_Printf ("Sys_LoadGameDll: Entry point not found in %s" ARCH_STRING DLL_EXT ". Failed with system error code 0x%X.\n", name, GetLastError());

		FreeLibrary( libHandle );
		return NULL;
	}

	return libHandle;
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
static int	eventHead=0;
static int	eventTail=0;
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
	netadr_t	adr;

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
//		g_wv.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len, TAG_EVENT );
		Q_strncpyz( b, s, len );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t *)Z_Malloc( len, TAG_EVENT, qtrue );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

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
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f( void ) {
	NET_Restart();
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

void Sys_Init( void ) {
	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();		// FIXME: not in dedicated?
}


//=======================================================================
//int	totalMsec, countMsec;

#ifndef DEFAULT_BASEDIR
#	define DEFAULT_BASEDIR Sys_BinaryPath()
#endif

int main( int argc, char **argv )
{
	int		i;
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };

	// done before Com/Sys_Init since we need this for error output
	//Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < argc; i++ )
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

	NET_Init();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
//		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
//		}

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();
	}

	// never gets here
	return 0;
}
