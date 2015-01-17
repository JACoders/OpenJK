#if !defined(_WIN32)
#include <dlfcn.h>
#include <unistd.h>
#ifdef DEDICATED
#include <sys/fcntl.h>
#endif
#endif
#include "qcommon/qcommon.h"
#include "sys_local.h"
#include "sys_loadlib.h"
#include "sys_public.h"
#include <inttypes.h>

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };

cvar_t *com_minimized;
cvar_t *com_unfocused;

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

sysEvent_t Sys_GetEvent( void ) {
	sysEvent_t	ev;
	char		*s;
#if !defined(_JK2EXE)
	netadr_t	adr;
	msg_t		netmsg;
#endif

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

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

#if !defined(_JK2EXE)
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
#endif

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
==================
Sys_GetClipboardData
==================
*/
char *Sys_GetClipboardData( void ) {
#ifdef DEDICATED
	return NULL;
#else
	if ( !SDL_HasClipboardText() )
		return NULL;

	char *cbText = SDL_GetClipboardText();
	size_t len = strlen( cbText ) + 1;

	char *buf = (char *)Z_Malloc( len, TAG_CLIPBOARD );
	Q_strncpyz( buf, cbText, len );

	SDL_free( cbText );
	return buf;
#endif
}

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

/*
=================
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath(void)
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
=================
Sys_DefaultInstallPath
=================
*/
char *Sys_DefaultInstallPath(void)
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath(void)
{
	return Sys_BinaryPath();
}

// Cicular buffer of characters. Be careful, there is no null terminator.
// You're expected to use the console log length to know where the end
// of the string is.
#define MAX_CONSOLE_LOG_SIZE (65535)
static char consoleLog[MAX_CONSOLE_LOG_SIZE];

// Where to start writing the next string
static int consoleLogWriteHead;

// Length of buffer
static int consoleLogLength;

// We now expect newlines instead of always appending
// otherwise sectioned prints get messed up.
#define MAXPRINTMSG		4096
void Conbuf_AppendText( const char *pMsg )
{
	char msg[MAXPRINTMSG] = {0};
	Q_strncpyz(msg, pMsg, sizeof(msg));
	Q_StripColor(msg);
	printf("%s", msg);

	// Add to log
	for ( int i = 0; msg[i]; i++ )
	{
		consoleLog[consoleLogWriteHead] = msg[i];

		consoleLogWriteHead = (consoleLogWriteHead + 1) % MAX_CONSOLE_LOG_SIZE;

		consoleLogLength++;
		if ( consoleLogLength > MAX_CONSOLE_LOG_SIZE )
		{
			consoleLogLength = MAX_CONSOLE_LOG_SIZE;
		}
	}
}

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
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init( void ) {
	Cmd_AddCommand ("in_restart", IN_Restart);
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser() );

	com_unfocused = Cvar_Get( "com_unfocused", "0", CVAR_ROM );
	com_minimized = Cvar_Get( "com_minimized", "0", CVAR_ROM );

	Sys_PlatformInit();
}

void Sys_Exit( int ex ) __attribute__((noreturn));
void Sys_Exit( int ex ) {
#ifndef DEDICATED
	SDL_Quit( );
#endif

    exit(ex);
}

#if !defined(DEDICATED)
static void Sys_ErrorDialog( const char *error )
{
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars
	char crashLogPath[64];

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime
	Com_sprintf( crashLogPath, sizeof( crashLogPath ), "crashlog-%s.txt", timeStr );

	const char *errorMessage = va( "%s\nWrite crash log to %s?\n", error, crashLogPath );

	const SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
	};

	SDL_MessageBoxData messageBoxData = {};
	messageBoxData.buttons = buttons;
	messageBoxData.numbuttons = ARRAY_LEN( buttons );
	messageBoxData.flags = SDL_MESSAGEBOX_ERROR;
	messageBoxData.message = errorMessage;
	messageBoxData.title = "Error";

	int button;
	if ( SDL_ShowMessageBox( &messageBoxData, &button ) < 0 )
	{
		// Bad things happened
		return;
	}

	if ( button == 1 )
	{
		FILE *fp = fopen( crashLogPath, "w" );

		if ( consoleLogLength == MAX_CONSOLE_LOG_SIZE )
		{
			fwrite( consoleLog + consoleLogWriteHead, MAX_CONSOLE_LOG_SIZE - consoleLogWriteHead, 1, fp );
		}

		fwrite( consoleLog, consoleLogWriteHead, 1, fp );

		fclose( fp );
	}
}
#endif

void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char    string[1024];

	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	Sys_Print( string );
#if !defined(DEDICATED)
	Sys_ErrorDialog( string );
#endif

	Sys_Exit( 3 );
}

void Sys_Quit (void) {
	IN_Shutdown();

	Com_ShutdownZoneMemory();
	Com_ShutdownHunkMemory();

	Sys_PlatformQuit();

    Sys_Exit(0);
}

/*
=================
Sys_UnloadDll
=================
*/
void Sys_UnloadDll( void *dllHandle )
{
	if( !dllHandle )
	{
		Com_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
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
		char libPath[MAX_OSPATH];
		const char *topDir = Sys_BinaryPath();

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

#if defined(MACOS_X) && !defined(_JK2EXE)
void *Sys_LoadMachOBundle( const char *name )
{
	if ( !FS_LoadMachOBundle(name) )
		return NULL;

	char *homepath = Cvar_VariableString( "fs_homepath" );
	char *gamedir = Cvar_VariableString( "fs_game" );
	char dllName[MAX_QPATH];

	Com_sprintf( dllName, sizeof(dllName), "%s_pk3" DLL_EXT, name );

	//load the unzipped library
	char *fn = FS_BuildOSPath( homepath, gamedir, dllName );

	void    *libHandle = Sys_LoadLibrary( fn );

	if ( libHandle != NULL ) {
		Com_Printf( "Loaded pk3 bundle %s.\n", name );
	}

	return libHandle;
}
#endif

enum SearchPathFlag
{
	SEARCH_PATH_MOD		= 1 << 0,
	SEARCH_PATH_BASE	= 1 << 1,
	SEARCH_PATH_ROOT	= 1 << 2
};

static void *Sys_LoadDllFromPaths( const char *filename, const char *gamedir, const char **searchPaths,
									int numPaths, uint32_t searchFlags, const char *callerName )
{
	char *fn;
	void *libHandle;

	if ( searchFlags & SEARCH_PATH_MOD )
	{
		for ( int i = 0; i < numPaths; i++ )
		{
			const char *libDir = searchPaths[i];
			if ( !libDir[0] )
				continue;

			fn = FS_BuildOSPath( libDir, gamedir, filename );
			libHandle = Sys_LoadLibrary( fn );
			if ( libHandle )
				return libHandle;

			Com_Printf( "%s(%s) failed: \"%s\"\n", callerName, fn, Sys_LibraryError() );
		}
	}

	if ( searchFlags & SEARCH_PATH_BASE )
	{
		for ( int i = 0; i < numPaths; i++ )
		{
			const char *libDir = searchPaths[i];
			if ( !libDir[0] )
				continue;

			fn = FS_BuildOSPath( libDir, BASEGAME, filename );
			libHandle = Sys_LoadLibrary( fn );
			if ( libHandle )
				break;

			Com_Printf( "%s(%s) failed: \"%s\"\n", callerName, fn, Sys_LibraryError() );
		}
	}

	if ( searchFlags & SEARCH_PATH_ROOT )
	{
		for ( int i = 0; i < numPaths; i++ )
		{
			const char *libDir = searchPaths[i];
			if ( !libDir[0] )
				continue;

			fn = va( "%s/%s", libDir, filename );
			libHandle = Sys_LoadLibrary( fn );
			if ( libHandle )
				break;

			Com_Printf( "%s(%s) failed: \"%s\"\n", callerName, fn, Sys_LibraryError() );
		}
	}
		
	return NULL;
}

void *Sys_LoadLegacyGameDll( const char *name, VMMainProc **vmMain, SystemCallProc *systemcalls )
{
	void	*libHandle = NULL;
	char	filename[MAX_OSPATH];

	Com_sprintf (filename, sizeof(filename), "%s" ARCH_STRING DLL_EXT, name);

	if (!Sys_UnpackDLL(filename))
	{
		Com_DPrintf( "Sys_LoadLegacyGameDll: Failed to unpack %s from PK3.\n", filename );
		return NULL;
	}

#if defined(MACOS_X) && !defined(_JK2EXE)
    //First, look for the old-style mac .bundle that's inside a pk3
    //It's actually zipped, and the zipfile has the same name as 'name'
    libHandle = Sys_LoadMachOBundle( name );
#endif

	if (!libHandle) {
		//Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", filename, Sys_LibraryError() );

		char *basepath = Cvar_VariableString( "fs_basepath" );
		char *homepath = Cvar_VariableString( "fs_homepath" );
		char *cdpath = Cvar_VariableString( "fs_cdpath" );
		char *gamedir = Cvar_VariableString( "fs_game" );
#ifdef MACOS_X
        char *apppath = Cvar_VariableString( "fs_apppath" );
#endif

		const char *searchPaths[] = {
			basepath,
			homepath,
#ifdef MACOS_X
			apppath,
#endif
			cdpath,
		};
		int numPaths = ARRAY_LEN( searchPaths );

		libHandle = Sys_LoadDllFromPaths( filename, gamedir, searchPaths, numPaths, SEARCH_PATH_BASE | SEARCH_PATH_MOD, __FUNCTION__ );
		if ( !libHandle )
			return NULL;
	}

	typedef void QDECL DllEntryProc( SystemCallProc *syscallptr );

	DllEntryProc *dllEntry = (DllEntryProc *)Sys_LoadFunction( libHandle, "dllEntry" );
	*vmMain = (VMMainProc *)Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*vmMain || !dllEntry ) {
		Com_DPrintf ( "Sys_LoadGameDll(%s) failed to find vmMain function:\n...%s!\n", name, Sys_LibraryError() );
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	Com_DPrintf ( "Sys_LoadGameDll(%s) found vmMain function at 0x%" PRIxPTR "\n", name, *vmMain );
	dllEntry( systemcalls );

	return libHandle;
}

void *Sys_LoadSPGameDll( const char *name, GetGameAPIProc **GetGameAPI )
{
	void	*libHandle = NULL;
	char	filename[MAX_OSPATH];

	assert( GetGameAPI );

	Com_sprintf (filename, sizeof(filename), "%s" ARCH_STRING DLL_EXT, name);

#if defined(MACOS_X) && !defined(_JK2EXE)
    //First, look for the old-style mac .bundle that's inside a pk3
    //It's actually zipped, and the zipfile has the same name as 'name'
    libHandle = Sys_LoadMachOBundle( name );
#endif

	if (!libHandle) {
		char *basepath = Cvar_VariableString( "fs_basepath" );
		char *homepath = Cvar_VariableString( "fs_homepath" );
		char *cdpath = Cvar_VariableString( "fs_cdpath" );
		char *gamedir = Cvar_VariableString( "fs_game" );
#ifdef MACOS_X
        char *apppath = Cvar_VariableString( "fs_apppath" );
#endif

		const char *searchPaths[] = {
			basepath,
			homepath,
#ifdef MACOS_X
			apppath,
#endif
			cdpath,
		};
		int numPaths = ARRAY_LEN( searchPaths );

		libHandle = Sys_LoadDllFromPaths( filename, gamedir, searchPaths, numPaths,
											SEARCH_PATH_BASE | SEARCH_PATH_MOD | SEARCH_PATH_ROOT,
											__FUNCTION__ );
		if ( !libHandle )
			return NULL;
	}

	*GetGameAPI = (GetGameAPIProc *)Sys_LoadFunction( libHandle, "GetGameAPI" );
	if ( !*GetGameAPI ) {
		Com_DPrintf ( "%s(%s) failed to find GetGameAPI function:\n...%s!\n", __FUNCTION__, name, Sys_LibraryError() );
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	return libHandle;
}

void *Sys_LoadGameDll( const char *name, GetModuleAPIProc **moduleAPI )
{
	void	*libHandle = NULL;
	char	filename[MAX_OSPATH];

	Com_sprintf (filename, sizeof(filename), "%s" ARCH_STRING DLL_EXT, name);

	if (!Sys_UnpackDLL(filename))
	{
		Com_DPrintf( "Sys_LoadGameDll: Failed to unpack %s from PK3.\n", filename );
		return NULL;
	}

#if defined(MACOS_X) && !defined(_JK2EXE)
    //First, look for the old-style mac .bundle that's inside a pk3
    //It's actually zipped, and the zipfile has the same name as 'name'
    libHandle = Sys_LoadMachOBundle( name );
#endif

	if (!libHandle) {
		//Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", filename, Sys_LibraryError() );

		char *basepath = Cvar_VariableString( "fs_basepath" );
		char *homepath = Cvar_VariableString( "fs_homepath" );
		char *cdpath = Cvar_VariableString( "fs_cdpath" );
		char *gamedir = Cvar_VariableString( "fs_game" );
#ifdef MACOS_X
        char *apppath = Cvar_VariableString( "fs_apppath" );
#endif

		const char *searchPaths[] = {
			basepath,
			homepath,
#ifdef MACOS_X
			apppath,
#endif
			cdpath,
		};
		int numPaths = ARRAY_LEN( searchPaths );

		libHandle = Sys_LoadDllFromPaths( filename, gamedir, searchPaths, numPaths, SEARCH_PATH_BASE | SEARCH_PATH_MOD, __FUNCTION__ );
		if ( !libHandle )
			return NULL;
	}

	*moduleAPI = (GetModuleAPIProc *)Sys_LoadFunction( libHandle, "GetModuleAPI" );
	if ( !*moduleAPI ) {
		Com_DPrintf ( "Sys_LoadGameDll(%s) failed to find GetModuleAPI function:\n...%s!\n", name, Sys_LibraryError() );
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	return libHandle;
}

#ifdef MACOS_X
/*
 =================
 Sys_StripAppBundle

 Discovers if passed dir is suffixed with the directory structure of a Mac OS X
 .app bundle. If it is, the .app directory structure is stripped off the end and
 the result is returned. If not, dir is returned untouched.
 =================
 */
char *Sys_StripAppBundle( char *dir )
{
	static char cwd[MAX_OSPATH];

	Q_strncpyz(cwd, dir, sizeof(cwd));
	if(strcmp(Sys_Basename(cwd), "MacOS"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	if(strcmp(Sys_Basename(cwd), "Contents"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	if(!strstr(Sys_Basename(cwd), ".app"))
		return dir;
	Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
	return cwd;
}
#endif

#ifndef DEFAULT_BASEDIR
#	ifdef MACOS_X
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
#	endif
#endif

int main ( int argc, char* argv[] )
{
	int		i;
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };

	// get the initial time base
	Sys_Milliseconds();

#ifdef MACOS_X
	// This is passed if we are launched by double-clicking
	if ( argc >= 2 && Q_strncmp ( argv[1], "-psn", 4 ) == 0 )
		argc = 1;
#endif

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

	Com_Init (commandLine);

	NET_Init();

#if defined(DEDICATED) && !defined(_WIN32)
    fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	// main game loop
	while (1)
	{
		bool shouldSleep = false;

#if !defined(_JK2EXE)
		if ( com_dedicated->integer )
		{
			shouldSleep = true;
		}
#endif

#if !defined(DEDICATED)
		if ( com_minimized->integer )
		{
			shouldSleep = true;
		}
#endif

		if ( shouldSleep )
		{
			Sys_Sleep( 5 );
		}

		// make sure mouse and joystick are only called once a frame
		IN_Frame();

		// run the game
		Com_Frame();
	}

	// never gets here
}
