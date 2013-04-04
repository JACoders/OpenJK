#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <float.h>
#include <stdio.h>
#include "../game/g_public.h"
#include "../xbox/XBLive.h"

#include "../qcommon/files.h"
#include "win_file.h"
#include "../renderer/tr_local.h"
#include "glw_win_dx8.h"

#include "../qcommon/xb_settings.h"

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"

#include <IO.h>
#define NEWDECL __cdecl

#ifndef FINAL_BUILD
#include "dbg_console_xbox.h"
#endif

#endif

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




char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

#ifdef _XBOX
	strcpy(cwd, "d:");
#endif

#ifdef _GAMECUBE
	strcpy(cwd, ".");
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
#define MAX_POLL_RATE	15
sysEvent_t Sys_GetEvent( void ) {
    sysEvent_t        ev;

	// return if we have data
	if(ClientManager::splitScreenMode == qtrue)
	{
		if(ClientManager::ActiveClient().eventHead > ClientManager::ActiveClient().eventTail ) {
			ClientManager::ActiveClient().eventTail++;
			return ClientManager::ActiveClient().eventQue[ (ClientManager::ActiveClient().eventTail - 1) & MASK_QUED_EVENTS ];
		}
	}
	else
	{
		if ( eventHead > eventTail ) {
            eventTail++;
       	    return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	    }
	}

    // check for network packets
	msg_t                netmsg;
	netadr_t	adr;

	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t *) Z_Malloc(len, TAG_EVENT, qfalse, 4);
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	//Check for broadcast messages
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetBroadcastPacket ( &netmsg ) )
	{
		// copy out to a seperate buffer for qeueing
		int len = netmsg.cursize - netmsg.readcount;
		char *buf = (char *) Z_Malloc(len, TAG_EVENT, qfalse, 4);
		memcpy( buf, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_BROADCAST_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if(ClientManager::splitScreenMode == qtrue)
	{
		if(ClientManager::ActiveClient().eventHead > ClientManager::ActiveClient().eventTail ) {
			ClientManager::ActiveClient().eventTail++;
			return ClientManager::ActiveClient().eventQue[ (ClientManager::ActiveClient().eventTail - 1) & MASK_QUED_EVENTS ];
		}
	}
	else
	{
	    if ( eventHead > eventTail ) {
			eventTail++;
			return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
    	}
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

Crappy full-screen texture drawing code from SP

*/

/*********
SP_DrawTexture
*********/
void SP_DrawTexture(void* pixels, float width, float height, float vShift)
{
	if (!pixels)
	{
		// Ug.  We were not even able to load the error message texture.
		return;
	}
	
	// Create a texture from the buffered file
	GLuint texid;
	qglGenTextures(1, &texid);
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DDS1_EXT, width, height, 0, GL_DDS1_EXT, GL_UNSIGNED_BYTE, pixels);

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Reset every GL state we've got.  Who knows what state
	// the renderer could be in when this function gets called.
	qglColor3f(1.f, 1.f, 1.f);
#ifdef _XBOX
	if(glw_state->isWidescreen)
		qglViewport(0, 0, 720, 480);
	else
#endif
	qglViewport(0, 0, 640, 480);

	GLboolean alpha = qglIsEnabled(GL_ALPHA_TEST);
	qglDisable(GL_ALPHA_TEST);

	GLboolean blend = qglIsEnabled(GL_BLEND);
	qglDisable(GL_BLEND);

	GLboolean cull = qglIsEnabled(GL_CULL_FACE);
	qglDisable(GL_CULL_FACE);

	GLboolean depth = qglIsEnabled(GL_DEPTH_TEST);
	qglDisable(GL_DEPTH_TEST);

	GLboolean fog = qglIsEnabled(GL_FOG);
	qglDisable(GL_FOG);

	GLboolean lighting = qglIsEnabled(GL_LIGHTING);
	qglDisable(GL_LIGHTING);

	GLboolean offset = qglIsEnabled(GL_POLYGON_OFFSET_FILL);
	qglDisable(GL_POLYGON_OFFSET_FILL);

	GLboolean scissor = qglIsEnabled(GL_SCISSOR_TEST);
	qglDisable(GL_SCISSOR_TEST);

	GLboolean stencil = qglIsEnabled(GL_STENCIL_TEST);
	qglDisable(GL_STENCIL_TEST);

	GLboolean texture = qglIsEnabled(GL_TEXTURE_2D);
	qglEnable(GL_TEXTURE_2D);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
#ifdef _XBOX
	if(glw_state->isWidescreen)
        qglOrtho(0, 720, 0, 480, 0, 1);
	else
#endif
	qglOrtho(0, 640, 0, 480, 0, 1);
	
	qglMatrixMode(GL_TEXTURE0);
	qglLoadIdentity();
	qglMatrixMode(GL_TEXTURE1);
	qglLoadIdentity();

	qglActiveTextureARB(GL_TEXTURE0_ARB);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);

	memset(&tess, 0, sizeof(tess));

	// Draw the error message
	qglBeginFrame();

/*	if (!SP_LicenseDone)
	{
		// clear the screen if we haven't done the
		// license yet...
		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);
	}
*/
	float x1, x2, y1, y2;
#ifdef _XBOX
	if(glw_state->isWidescreen)
	{
		x1 = 0;
		x2 = 720;
		y1 = 0;
		y2 = 480;
	}
	else {
#endif
	x1 = 0;
	x2 = 640;
	y1 = 0;
	y2 = 480;
#ifdef _XBOX
	}
#endif

	y1 += vShift;
	y2 += vShift;

	qglBeginEXT (GL_TRIANGLE_STRIP, 4, 0, 0, 4, 0);
		qglTexCoord2f( 0,  0 );
		qglVertex2f(x1, y1);
		qglTexCoord2f( 1 ,  0 );
		qglVertex2f(x2, y1);
		qglTexCoord2f( 0, 1 );
		qglVertex2f(x1, y2);
		qglTexCoord2f( 1, 1 );
		qglVertex2f(x2, y2);
	qglEnd();
	
	qglEndFrame();
	qglFlush();

	// Restore (most) of the render states we reset
	if (alpha) qglEnable(GL_ALPHA_TEST);
	else qglDisable(GL_ALPHA_TEST);

	if (blend) qglEnable(GL_BLEND);
	else qglDisable(GL_BLEND);

	if (cull) qglEnable(GL_CULL_FACE);
	else qglDisable(GL_CULL_FACE);

	if (depth) qglEnable(GL_DEPTH_TEST);
	else qglDisable(GL_DEPTH_TEST);

	if (fog) qglEnable(GL_FOG);
	else qglDisable(GL_FOG);

	if (lighting) qglEnable(GL_LIGHTING);
	else qglDisable(GL_LIGHTING);

	if (offset) qglEnable(GL_POLYGON_OFFSET_FILL);
	else qglDisable(GL_POLYGON_OFFSET_FILL);

	if (scissor) qglEnable(GL_SCISSOR_TEST);
	else qglDisable(GL_SCISSOR_TEST);

	if (stencil) qglEnable(GL_STENCIL_TEST);
	else qglDisable(GL_STENCIL_TEST);

	if (texture) qglEnable(GL_TEXTURE_2D);
	else qglDisable(GL_TEXTURE_2D);

	// Kill the texture
	qglDeleteTextures(1, &texid);
}


/*********
SP_GetLanguageExt

Retuns the extension for the current language, or
english if the language is unknown.
*********/
char* SP_GetLanguageExt()
{
	switch(XGetLanguage())
	{
	case XC_LANGUAGE_ENGLISH:
		return "EN";
//	case XC_LANGUAGE_JAPANESE:
//		return "JA";
	case XC_LANGUAGE_GERMAN:
		return "GE";
//	case XC_LANGUAGE_SPANISH:
//		return "SP";
//	case XC_LANGUAGE_ITALIAN:
//		return "IT";
//	case XC_LANGUAGE_KOREAN:
//		return "KO";
//	case XC_LANGUAGE_TCHINESE:
//		return "CH";
//	case XC_LANGUAGE_PORTUGUESE:
//		return "PO";
	case XC_LANGUAGE_FRENCH:
		return "FR";
	default:
		return "EN";
	}
}

/*********
SP_LoadFile
*********/
void* SP_LoadFile(const char* name)
{
	wfhandle_t h = WF_Open(name, true, false);
	if (h < 0) return NULL;

	if (WF_Seek(0, SEEK_END, h))
	{
		WF_Close(h);
		return NULL;
	}

	int len = WF_Tell(h);
	
	if (WF_Seek(0, SEEK_SET, h))
	{
		WF_Close(h);
		return NULL;
	}

	void *buf = Z_Malloc(len, TAG_TEMP_WORKSPACE, false, 32);

	if (WF_Read(buf, len, h) != len)
	{
		Z_Free(buf);
		WF_Close(h);
		return NULL;
	}

	WF_Close(h);

	return buf;
}

/*********
SP_LoadFileWithLanguage

Loads a screen with the appropriate language
*********/
void *SP_LoadFileWithLanguage(const char *name)
{
	char fullname[MAX_QPATH];
	void *buffer = NULL;
	char *ext;

	// get the language extension
	ext = SP_GetLanguageExt();

	// creat the fullpath name and try to load the texture
	sprintf(fullname, "%s_%s.dds", name, ext);
	buffer = SP_LoadFile(fullname);

	if (!buffer)
	{
		sprintf(fullname, "%s.dds", name);
		buffer = SP_LoadFile(fullname);
	}

	return buffer;
}

/*
SP_DrawSPLoadScreen

Draws the single player loading screen - used when skipping the logo movies
*/
void SP_DrawSPLoadScreen( void )
{
	// Load the texture:
	void *image = SP_LoadFileWithLanguage("d:\\base\\media\\LoadSP");

	if( image )
	{
		SP_DrawTexture(image, 512, 512, 0);
		Z_Free(image);
	}
}

/*
ERR_DiscFail

Draws the damaged/dirty disc message, looping forever
*/
void ERR_DiscFail(bool poll)
{
	// Load the texture:
	void *image = SP_LoadFileWithLanguage("d:\\base\\media\\DiscErr");

	if( image )
	{
		SP_DrawTexture(image, 512, 512, 0);
		Z_Free(image);
	}

	for (;;)
	{
		extern void S_Update_(void);
		S_Update_();
	}
}

/*****************************************************************************/

/*
=====================

XBE SWITCHING SUPPORT

=====================
*/
#define LAUNCH_MAGIC "J3D1"
void Sys_Reboot( const char *reason )
{
	LAUNCH_DATA ld;
	const char *path = NULL;

	memset( &ld, 0, sizeof(ld) );

	if (!Q_stricmp(reason, "new_account"))
	{
		PLD_LAUNCH_DASHBOARD pDash	= (PLD_LAUNCH_DASHBOARD) &ld;
		pDash->dwReason				= XLD_LAUNCH_DASHBOARD_NEW_ACCOUNT_SIGNUP;
		path						= NULL;
	}
	else if (!Q_stricmp(reason, "net_config"))
	{
		PLD_LAUNCH_DASHBOARD pDash	= (PLD_LAUNCH_DASHBOARD) &ld;
		pDash->dwReason				= XLD_LAUNCH_DASHBOARD_NETWORK_CONFIGURATION;
		path						= NULL;
	}
	else if (!Q_stricmp(reason, "manage_account"))
	{
		PLD_LAUNCH_DASHBOARD pDash	= (PLD_LAUNCH_DASHBOARD) &ld;
		pDash->dwReason				= XLD_LAUNCH_DASHBOARD_ACCOUNT_MANAGEMENT;
		path						= NULL;
	}
	else if (!Q_stricmp(reason, "singleplayer"))
	{
		SP_DrawSPLoadScreen();
		glw_state->device->PersistDisplay();
		path = "d:\\default.xbe";
		ld.Data[0] = IN_GetMainController();
		strcpy((char *)&ld.Data[1], LAUNCH_MAGIC);
		if( Settings.IsDisabled() )
			ld.Data[5] = 0x42;
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

	XLaunchNewImage(path, &ld);

	// This function should not return!
	Com_Error( ERR_FATAL, "ERROR: XLaunchNewImage returned\n" );
}

static LAUNCH_DATA s_ld;

// Run-once function to make sure that ld is filled in.
// Call this from any function that needs to use s_ld:
static void _initLD( void )
{
	static bool initialized = false;

	if( !initialized )
	{
		initialized = true;

		DWORD launchType;
		if( XGetLaunchInfo( &launchType, &s_ld ) != ERROR_SUCCESS ||
			launchType != LDT_TITLE )
			memset( &s_ld, 0, sizeof(s_ld) );

		if( s_ld.Data[1] == 0x42 )
			Settings.Disable();
	}
}

int Sys_GetLaunchController( void )
{
	_initLD();

	return s_ld.Data[0];
}

// Used to check for the presence of an accepted invite for our game on the HD.
// This actually looks in the launch_data, because the SP game absorbs it, then
// copies it back to the LD before rebooting. Bleh.
XONLINE_ACCEPTED_GAMEINVITE *Sys_AcceptedInvite( void )
{
	_initLD();

	// Flag to indicate whether or not we had an invite:
	if( !s_ld.Data[2] )
		return NULL;

	// OK. The SP XBE should have just copied the invite to the LD:
	return (XONLINE_ACCEPTED_GAMEINVITE *) &s_ld.Data[3];
}

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
	// I'm going to kill someone. This should not be necessary. No, really.
	Direct3D_SetPushBufferSize(1024*1024, 128*1024);

	// get the initial time base
	Sys_Milliseconds();

	Win_Init();
	Com_Init( "" );

	//Start sound early.  The STL inside will allocate memory and we don't
	//want that memory in the middle of the zone.
	if ( !cls.soundRegistered ) {
		cls.soundRegistered = qtrue;
		S_BeginRegistration(ClientManager::NumClients());
	}

//	NET_Init();
	// At this point, we NEED our local address:
	extern void NET_GetLocalAddress( bool force );
	NET_GetLocalAddress( true );

	// A sample does this, seems un-necessary though:
//	XNetGetBroadcastVersionStatus( TRUE );

	// Check for a pending invitation on the HD. We call this now to
	// force the result to be retrieved and cached inside the func.
	Sys_AcceptedInvite();

	// main game loop
	while( 1 ) {
		/*
		extern void PrintMem(void);
		PrintMem();
		*/
		IN_Frame();
		Com_Frame();

		// Do any XBL stuff
//		XBL_Tick();

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
	if (strstr(directory, "d:\\base\\"))
		directory += 8;

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
#ifndef FINAL_BUILD
		Com_Printf( "WARNING: List file %s not found\n", listFilename );
#endif
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
	CG_PreInit();
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
