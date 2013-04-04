#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <float.h>
#include <stdio.h>
#include "../game/g_public.h"
#include <xonline.h>
#include "glw_win_dx8.h"
#include "../qcommon/xb_settings.h"

#ifdef _XBOX
#include <IO.h>
#define NEWDECL __cdecl

#ifndef FINAL_BUILD
#include "dbg_console_xbox.h"
#endif

#endif

int gLaunchController = 0;

extern int eventHead, eventTail;
extern sysEvent_t eventQue[MAX_QUED_EVENTS];
extern byte		sys_packetReceived[MAX_MSGLEN];

void *NEWDECL operator new(size_t size)
{
	return Z_Malloc(size, TAG_NEWDEL, qfalse);
}


void *NEWDECL operator new[](size_t size)
{
	return Z_Malloc(size, TAG_NEWDEL, qfalse);
}


void NEWDECL operator delete[](void *ptr)
{
	if (ptr)
		Z_Free(ptr);
}


void NEWDECL operator delete(void *ptr)
{
	if (ptr)
		Z_Free(ptr);
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
extern void Sys_In_Restart_f(void);
extern void Sys_Net_Restart_f(void);
void Sys_Init( void ) 
{
	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);
}

#ifdef XBOX_DEMO
// When we're a demo, we're not running from D:\, so we need some hacks:
char demoBasePath[64];
#endif

char *Sys_Cwd( void )
{
	static char cwd[MAX_OSPATH];

#ifdef XBOX_DEMO
	strcpy( cwd, demoBasePath );
#else
	strcpy(cwd, "d:");
#endif

	return cwd;
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
}



/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char *error, ... ) {
        va_list                argptr;
        char                text[256];

        va_start (argptr, error);
        vsprintf (text, error, argptr);
        va_end (argptr);

#ifdef _GAMECUBE
        printf(text);
#else
        OutputDebugString(text);
#endif

#if 0 // UN-PORT
        Com_ShutdownZoneMemory();
        Com_ShutdownHunkMemory();
#endif

        exit (1);
}


/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
        sysEvent_t        ev;

        // return if we have data
        if ( eventHead > eventTail ) {
                eventTail++;
                return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
        }

        // check for network packets
        msg_t                netmsg;
        MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );

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


void Sys_Print(const char *msg)
{
#ifdef _GAMECUBE
	printf(msg);
#else
	OutputDebugString(msg);
#endif
}

/*
==============
Sys_Log
==============
*/
void Sys_Log( const char *file, const char *msg ) {
	Sys_Log(file, msg, strlen(msg), strchr(msg, '\n') ? true : false);
}

/*
==============
Sys_Log
==============
*/
void Sys_Log( const char *file, const void *buffer, int size, bool flush ) {
#ifndef FINAL_BUILD
	static bool unableToLog = false;

	// Once we've failed to write to the log files once, bail out.
	// This lets us put release builds on DVD without recompiling.
	if (unableToLog)
		return;

	struct FileInfo
	{
		char name[MAX_QPATH];
		FILE *handle;
	};

	const int LOG_MAX_FILES = 4;
	static FileInfo files[LOG_MAX_FILES];
	static int num_files = 0;

	FileInfo* cur = NULL;
	for (int f = 0; f < num_files; ++f)
	{
		if (!stricmp(file, files[f].name))
		{
			cur = &files[f];
			break;
		}
	}

	if (cur == NULL)
	{
		if (num_files >= LOG_MAX_FILES)
		{
			Sys_Print("Too many log files!\n");
			return;
		}

		cur = &files[num_files++];
		strcpy(cur->name, file);
		cur->handle = NULL;
	}

	char fullname[MAX_QPATH];
	sprintf(fullname, "d:\\%s", cur->name);
	if (!cur->handle)
	{
		cur->handle = fopen(fullname, "wb");
		if (cur->handle == NULL)
		{
			Sys_Print("Unable to open log file!\n");
			unableToLog = true;
			return;
		}
	}

	if (size == 1) fputc(*(char*)buffer, cur->handle);
	else fwrite(buffer, size, 1, cur->handle);

	if (flush)
	{
		fflush(cur->handle);
	}
#endif
}

#ifdef _XBOX
HANDLE Sys_FileStreamMutex = INVALID_HANDLE_VALUE;
#endif

void Win_Init(void)
{
#ifdef _XBOX
	Sys_FileStreamMutex = CreateMutex(NULL, FALSE, NULL);
#endif
}

/*
=====================

XBE SWITCHING SUPPORT

=====================
*/

#ifdef XBOX_DEMO
// Filled in when we're launched from CDX
LD_DEMO demoLaunchData;
bool demoLaunchDataValid = false;

// If we were launched by the user, or someone has pressed a key
// then the timer should not count down during movies/loading:
bool demoTimerAlways = false;
int demoTimer = 0;
#endif

// Takes a filename (relative to ".") and pre-pends the right path. This
// is needed for demos where the game won't be running from D:
const char *Sys_RemapPath( const char *filename )
{
#ifdef XBOX_DEMO
	return va( "%s\\%s", demoBasePath, filename );
#else
	return va( "D:\\%s", filename );
#endif
}

// Despite what you may think, this function actually just returns
// a value telling you if you *should* quick-boot -- ie skip intro
// cinematics and such. Only supposed to XGetLaunchInfo once per
// boot, so we cache the results.
//
// This function always gets called at startup, so we also use it
// to retrieve the launch info for a demo
#define LAUNCH_MAGIC "J3D1"
bool Sys_QuickStart( void )
{
	static bool retVal = false;
	static bool initialized = false;

	if( initialized )
		return retVal;

	initialized = true;

#ifdef XBOX_DEMO
	// Default to D:\, this gets replaced below if CDX started us
	strcpy( demoBasePath, "D:" );

	gLaunchController = 0;	// Irrelevant in demo
	retVal = false;			// We never come from MP (eg), so always false
	DWORD launchType;

	DWORD result = XGetLaunchInfo( &launchType, (LAUNCH_DATA *) &demoLaunchData );
	if( result == ERROR_SUCCESS && launchType == LDT_TITLE )
	{
		// We were launched by CDX:
		demoLaunchDataValid = true;

		// How we were launched affects timer behavior:
		demoTimerAlways = (demoLaunchData.dwRunmode == XLDEMO_RUNMODE_KIOSKMODE);

		// Need to re-map paths, as D:\\ doesn't work now:
		Q_strncpyz( demoBasePath, demoLaunchData.szLaunchedXBE, sizeof(demoBasePath), qtrue );

		// Find our executable name in the path, and truncate the string there:
		char *pXBE = strstr( demoBasePath, "\\default.xbe" );
		if( !pXBE )
			Com_Error( ERR_FATAL, "Error re-mapping D drive\n" );
		*pXBE = 0;

		// Fix the video path:
		extern char XBOX_VIDEO_PATH[64];
		strcpy( XBOX_VIDEO_PATH, demoBasePath );
		strcat( XBOX_VIDEO_PATH, "\\base\\video\\" );
	}

	return retVal;
#else
	DWORD launchType;
	LAUNCH_DATA ld;

	if( (XGetLaunchInfo( &launchType, &ld ) != ERROR_SUCCESS) ||
		(launchType != LDT_TITLE) ||
		strcmp((const char *)&ld.Data[1], LAUNCH_MAGIC) )
		return (retVal = false);
	
	gLaunchController = ld.Data[0];

	// Magic number to disable settings/saving
	if( ld.Data[5] == 0x42 )
		Settings.Disable();

	return (retVal = true);
#endif
}

extern int IN_GetMainController(void);
extern void SP_DrawMPLoadScreen(void);

// Takes an extra parameter so that the accepted invite code can pass
// in the XONLINE_ACCEPTED_INVITE to be copied into launch data.
//
// For the demo, the only valid reason is "demo", and pData should be NULL
void Sys_Reboot( const char *reason, const void *pData )
{
#ifdef XBOX_DEMO
	if( pData || !demoLaunchDataValid || Q_stricmp( reason, "demo" ) != 0 )
		Com_Error( ERR_DROP, "Invalid Sys_Reboot call\n" );

	// Kill off the sound and stream threads:
	S_Shutdown();
	extern void Sys_StreamShutdown(void);
	Sys_StreamShutdown();

	// Now return to CDX
	XLaunchNewImage( demoLaunchData.szLauncherXBE, (LAUNCH_DATA *) &demoLaunchData );
	// Should never return!
#else
	LAUNCH_DATA ld;
	const char *path = NULL;
	int controller;

	memset( &ld, 0, sizeof(ld) );
	controller = IN_GetMainController();
	ld.Data[0] = (byte) controller;
	Com_Printf("\tController %d Passed\n",controller); 

	if (!Q_stricmp(reason, "multiplayer"))
	{
		path = "d:\\jamp.xbe";
		SP_DrawMPLoadScreen();

		// Set a magic number if saving is disabled
		if( Settings.IsDisabled() )
			ld.Data[1] = 0x42;

		// Flag that there is no invite in the launch data:
		ld.Data[2] = 0;
	}
	else if (!Q_stricmp(reason, "invite"))
	{
		path = "d:\\jamp.xbe";
		SP_DrawMPLoadScreen();

		// Set a magic number if saving is disabled
		if( Settings.IsDisabled() )
			ld.Data[1] = 0x42;

		// Flag that we're including an invite with the launch data:
		ld.Data[2] = 1;

		memcpy( &ld.Data[3], pData, sizeof(XONLINE_ACCEPTED_GAMEINVITE) );
	}
	else
	{
		Com_Error( ERR_FATAL, "Unknown reboot code %s\n", reason );
	}

	// Title should not be doing ANYTHING in the background.
	// Shutting down sound ensures that the sound thread is gone
	S_Shutdown();
	// Similarly, kill off the streaming thread
	extern void Sys_StreamShutdown(void);
	Sys_StreamShutdown();

	// Keep the loading screen up while we reboot!
	glw_state->device->PersistDisplay();

	XLaunchNewImage(path, &ld);

	// This function should not return!
	Com_Error( ERR_FATAL, "ERROR: XLaunchNewImage returned\n" );
#endif
}

static XONLINE_ACCEPTED_GAMEINVITE acceptedGameInvite;

// Used to check for the presence of an accepted invite for our game on the HD.
// Can only return true on the FIRST call.
bool Sys_InviteExists( void )
{
#ifdef XBOX_DEMO
	return false;
#else
	static bool initialized = false;
	if( initialized )
		return false;
	initialized = true;

	// If we just came from the MP XBE, don't auto-reboot again. That's just silly.
	if( Sys_QuickStart() )
		return false;

	// Try to retrieve an invitation from the HD (this requires that we start XOnline):
	XOnlineStartup( NULL );
	HRESULT hr = XOnlineFriendsGetAcceptedGameInvite( &acceptedGameInvite );
	XOnlineCleanup();

	return (hr == S_OK);
#endif
}

// Reboot to MP to join the game that we have an invite for:
void Sys_JoinInvite( void )
{
	// Aha. Well, XTL seems to blow this away now, so we need to copy it to launch_data. Bleh.
	Sys_Reboot( "invite", &acceptedGameInvite );
	// Never returns!
}

#ifdef XBOX_DEMO
// Timer code for the demo:
static int lastTime = 0;
static bool demoTimerPaused = false;

// Notify the demo timer of a keypress, which resets the timer, and ensures
// that the timer no longer runs during FMV/loading (if it was before)
void Demo_TimerKeypress( void )
{
	demoTimerAlways = false;

	// Reset the timer:
	demoTimer = demoLaunchData.dwTimeout;

	// Stamp the diff-timer
	lastTime = Sys_Milliseconds();
}

// Update the timer, and check to see if we should reboot:
void Demo_TimerUpdate( void )
{
	// Handle first call correctly
	if( !lastTime )
	{
		demoTimer = demoLaunchData.dwTimeout;
		lastTime = Sys_Milliseconds();
	}

	int newTime = Sys_Milliseconds();
	int diffTime = newTime - lastTime;
	lastTime = newTime;

	// If the timer isn't supposed to run, and we're "paused", don't update:
	extern bool in_camera;
	if( !demoTimerAlways && (demoTimerPaused || in_camera) )
		return;

	// If we weren't even launched by CDX in the first place, or were given
	// a zero timeout, do nothing:
	if( !demoLaunchDataValid || !demoLaunchData.dwTimeout )
		return;

	// Time ran out?
	if( demoTimer < diffTime )
		Sys_Reboot( "demo", NULL );

	demoTimer -= diffTime;
}

// Pause/unpause the demo timer when entering/exiting non-interactive state:
void Demo_TimerPause( bool bPaused )
{
	// Always stamp the timer right before we change state:
	Demo_TimerUpdate();

	demoTimerPaused = bPaused;
}

#endif

/*
==================
WinMain

==================
*/
#if defined (_XBOX)
int __cdecl main()
#elif defined (_GAMECUBE)
int main(int argc, char* argv[])
#endif
{
//	Z_SetFreeOSMem();

	// I'm going to kill someone. This should not be necessary. No, really.
	Direct3D_SetPushBufferSize(1024*1024, 128*1024);

	// get the initial time base
	Sys_Milliseconds();

	// Need to fetch this stuff REALLY early so that path re-mappnig works
	// for renderer startup. Bleh.
	Sys_QuickStart();

	Win_Init();
	Com_Init( "" );

	// Run one frame, to finish loading (calls CL_StartHunkUsers)...
	IN_Frame();
	Com_Frame();

	extern void G_AllocGentities( void );
	G_AllocGentities();

	// And then quickly copy all the planet binks to the Z: drive
	extern void Sys_BinkCopyInit(void);
	Sys_BinkCopyInit();

	// main game loop
	while( 1 ) {
		IN_Frame();
		Com_Frame();

		// Poll debug console for new commands
#ifndef FINAL_BUILD
		DebugConsoleHandleCommands();
#endif
	}

	return 0;
}


char *Sys_GetClipboardData(void) { return NULL; }

void Sys_StartProcess(char *, qboolean) {}

void Sys_OpenURL(char *, int) {}

void Sys_Quit(void) {}

void Sys_ShowConsole(int, int) {}

void Sys_Mkdir(const char *) {}

int Sys_LowPhysicalMemory(void) { return 0; }

void Sys_FreeFileList(char **filelist)
{
	// All strings in a file list are allocated at once, so we just need to
	// do two frees, one for strings, one for the pointers.
	if ( filelist )
	{
		if ( filelist[0] )
			Z_Free( filelist[0] );

		Z_Free( filelist );
	}
}

#ifdef _JK2MP
char** Sys_ListFiles(const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs)
#else
char** Sys_ListFiles(const char *directory, const char *extension, int *numfiles, qboolean wantsubs)
#endif
{
#ifdef _JK2MP
	// MP has extra filter paramter. We don't support that.
	if (filter)
	{
		assert(!"Sys_ListFiles doesn't support filter on console!");
		return NULL;
	}
#endif

	// Hax0red console version of Sys_ListFiles. We mangle our arguments to get a standard filename
	// That file should exist, and contain the list of files that meet this search criteria.
	char	listFilename[MAX_OSPATH];
	char	*listFile, *curFile, *end;
	int		nfiles;
	char	**retList;

	// S00per hack
#ifdef XBOX_DEMO
	const char *basePath = Sys_RemapPath( "base\\" );
	if (strstr(directory, basePath))
		directory += strlen( basePath );
#else
	if (strstr(directory, "d:\\base\\"))
		directory += 8;
#endif

	if (!extension)
	{
		extension = "";
	}
	else if (extension[0] == '/' && extension[1] == 0)
	{
		// Passing a slash as extension will find directories
		extension = "dir";
	}
	else if (extension[0] == '.')
	{
		// Skip over leading .
		extension++;
	}

	// Build our filename
	Com_sprintf(listFilename, sizeof(listFilename), "%s\\_console_%s_list_", directory, extension);
	if (FS_ReadFile( listFilename, (void**)&listFile ) <= 0)
	{
		if(listFile) {
			FS_FreeFile(listFile);
		}
		Com_Printf( "WARNING: List file %s not found\n", listFilename );
		if (numfiles)
			*numfiles = 0;
		return NULL;
	}

	// Do a first pass to count number of files in the list
	nfiles = 0;
	curFile = listFile;
	while (true)
	{
		// Find end of line
		end = strchr(curFile, '\r');
		if (end)
		{
			// Should have a \n next -- skip them both
			end += 2;
		}
		else
		{
			end = strchr(curFile, '\n');
			if (end) end++;
			else end = curFile + strlen(curFile);
		}

		// Is the line empty?  If so, we're done.
		if (!curFile || !curFile[0]) break;
		++nfiles;

		// Advance to next line
		curFile = end;
	}

	// Fill in caller's pointer for number of files found
	if (numfiles) *numfiles = nfiles;

	// Did we find any files at all?
	if (nfiles == 0)
	{
		FS_FreeFile(listFile);
		return NULL;
	}

	// Allocate a file list, and quick string pool, but use LISTFILES
	retList = (char **) Z_Malloc( ( nfiles + 1 ) * sizeof( *retList ), TAG_LISTFILES, qfalse);
	// Our string pool is actually slightly too large, but it's temporary, and that's better
	// than slightly too small
	char *stringPool = (char *) Z_Malloc( strlen(listFile) + 1, TAG_LISTFILES, qfalse );

	// Now go through the list of files again, and fill in the list to be returned
	nfiles = 0;
	curFile = listFile;
	while (true)
	{
		// Find end of line
		end = strchr(curFile, '\r');
		if (end)
		{
			// Should have a \n next -- skip them both
			*end++ = '\0';
			*end++ = '\0';
		}
		else
		{
			end = strchr(curFile, '\n');
			if (end) *end++ = '\0';
			else end = curFile + strlen(curFile);
		}

		// Is the line empty?  If so, we're done.
		int curStrSize = strlen(curFile);
		if (curStrSize < 1)
		{
			retList[nfiles] = NULL;
			break;
		}

		// Alloc a small copy
		//retList[nfiles++] = CopyString( curFile );
		retList[nfiles++] = stringPool;
		strcpy(stringPool, curFile);
		stringPool += (curStrSize + 1);

		// Advance to next line
		curFile = end;
	}

	// Free the special file's buffer
	FS_FreeFile( listFile );

	return retList;
}

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame( void ) {
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
#ifndef _JK2MP
void *Sys_GetGameAPI (void *parms)
{
	extern game_export_t *GetGameAPI( game_import_t *import );
	return GetGameAPI((game_import_t *)parms);
}
#endif

/*
=================
Sys_LoadCgame

Used to hook up a development dll
=================
*/
// void * Sys_LoadCgame( void ) 
#ifndef _JK2MP
void * Sys_LoadCgame( int (**entryPoint)(int, ...), int (*systemcalls)(int, ...) )
{
	extern void CG_PreInit();
	extern void cg_dllEntry( int (*syscallptr)( int arg,... ) );
	extern int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7 );
	cg_dllEntry(systemcalls);
	*entryPoint = (int (*)(int,...))vmMain;
//	CG_PreInit();
	return 0;
}
#endif

/* VVFIXME: More stubs */
qboolean Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ )
{
	return qfalse;
}

qboolean Sys_CopyFile(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, qboolean bOverwrite)
{
	return qfalse;
}

qboolean Sys_CheckCD( void )
{
	return qtrue;
}
