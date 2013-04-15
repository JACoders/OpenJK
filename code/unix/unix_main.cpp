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

#include <dlfcn.h>

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "sys/sys_local.h"
#include "sys/sys_loadlib.h"

cvar_t *nostdout;

unsigned	sys_frame_time;

uid_t saved_euid;

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
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

	game_library = Sys_LoadLibrary (name );
	if (game_library)
		Com_DPrintf ("LoadLibrary (%s)\n",name);
	else {
		Com_Printf( "LoadLibrary(\"%s\") failed\n", name);
		Com_Printf( "...reason: '%s'\n", dlerror() );
		Com_Error( ERR_FATAL, "Couldn't load game" );
	}

	GetGameAPI = (void *)Sys_LoadFunction (game_library, "GetGameAPI");
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
void * Sys_LoadCgame( int (**entryPoint)(int, ...), int (*systemcalls)(int, ...) )
{
	void	(*dllEntry)( int (*syscallptr)(int, ...) );
    
	dllEntry = ( void (*)( int (*)( int, ... ) ) )Sys_LoadFunction( game_library, "dllEntry" );
	*entryPoint = (int (*)(int,...))Sys_LoadFunction( game_library, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		Sys_UnloadLibrary( game_library );
		return NULL;
	}
    
	dllEntry( systemcalls );
	return game_library;
}

int main (int argc, char **argv)
{
	int 	oldtime, newtime;
	int		len, i;
	char	*cmdline;
	void SetProgramPath(char *path);


	// get the initial time base
	Sys_Milliseconds();

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
#ifdef _DEBUG
		if (!g_wv.activeApp)
		{
			Sleep(50);
		}
#endif // _DEBUG
        
        // make sure mouse and joystick are only called once a frame
		IN_Frame();

        Com_Frame ();
    }
}