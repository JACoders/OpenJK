#include <dlfcn.h>
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include "sys_loadlib.h"
#include "sys_local.h"

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };

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

static void *game_library;

/*
 =================
 Sys_UnloadGame
 =================
 */
void Sys_UnloadGame (void)
{
	Com_Printf("------ Unloading Game ------\n");
	if (game_library)
		Sys_UnloadLibrary (game_library);
	game_library = NULL;
}

/*
 =================
 Sys_GetGameAPI
 
 Loads the game dll
 =================
 */
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	
	const char	*basepath;
	const char	*cdpath;
	const char	*gamedir;
	const char	*homepath;
#ifdef MACOS_X
    const char  *apppath;
#endif
	const char	*fn;
	
#ifdef JK2_MODE
	const char *gamename = "jospgame" ARCH_STRING DLL_EXT;
#else
	const char *gamename = "jagame" ARCH_STRING DLL_EXT;
#endif
	
	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without calling Sys_UnloadGame");
	
	// check the current debug directory first for development purposes
	homepath = Cvar_VariableString( "fs_homepath" );
	basepath = Cvar_VariableString( "fs_basepath" );
	cdpath = Cvar_VariableString( "fs_cdpath" );
	gamedir = Cvar_VariableString( "fs_game" );
#ifdef MACOS_X
    apppath = Cvar_VariableString( "fs_apppath" );
#endif
	
	fn = FS_BuildOSPath( basepath, gamedir, gamename );
	
	game_library = Sys_LoadLibrary( fn );
	
//First try in mod directories. basepath -> homepath -> cdpath
	if (!game_library) {
		if (homepath[0]) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( homepath, gamedir, gamename);
			game_library = Sys_LoadLibrary( fn );
		}
	}
	
#ifdef MACOS_X
    if (!game_library) {
		if( apppath[0] ) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( apppath, gamedir, gamename );
			game_library = Sys_LoadLibrary( fn );
		}
	}
#endif
    
	if (!game_library) {
		if( cdpath[0] ) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( cdpath, gamedir, gamename );
			game_library = Sys_LoadLibrary( fn );
		}
	}
	
//Now try in base. basepath -> homepath -> cdpath
	if (!game_library) {
		Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
		
		fn = FS_BuildOSPath( basepath, OPENJKGAME, gamename );
		game_library = Sys_LoadLibrary( fn );
	}
	
	if (!game_library) {
		if ( homepath[0] ) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( homepath, OPENJKGAME, gamename );
			game_library = Sys_LoadLibrary( fn );
		}
	}
    
#ifdef MACOS_X
    if (!game_library) {
		if( apppath[0] ) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( apppath, OPENJKGAME, gamename );
			game_library = Sys_LoadLibrary( fn );
		}
	}
#endif
	
	if (!game_library) {
		if( cdpath[0] ) {
			Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			
			fn = FS_BuildOSPath( cdpath, OPENJKGAME, gamename );
			game_library = Sys_LoadLibrary( fn );
		}
	}
//Still couldn't find it.
	if (!game_library) {
		Com_Printf( "Sys_GetGameAPI(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
		Com_Error( ERR_FATAL, "Couldn't load game" );
	}
	
	
	Com_Printf ( "Sys_GetGameAPI(%s): succeeded ...\n", fn );

	GetGameAPI = (void *(*)(void *))Sys_LoadFunction (game_library, "GetGameAPI");
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
    
	dllEntry = ( void (*)( intptr_t (*)( intptr_t, ... ) ) )Sys_LoadFunction( game_library, "dllEntry" );
	*entryPoint = (intptr_t (*)(int,...))Sys_LoadFunction( game_library, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		Sys_UnloadLibrary( game_library );
		return NULL;
	}
    
	dllEntry( systemcalls );
	return game_library;
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

int main (int argc, char **argv)
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

	Com_Init(commandLine);
		
    while (1)
    {
		// set low precision every frame, because some system calls
		// reset it arbitrarily
#ifdef _DEBUG
//		if (!g_wv.activeApp)
//		{
//			Sleep(50);
//		}
#endif // _DEBUG
        
        // make sure mouse and joystick are only called once a frame
		IN_Frame();
		
        Com_Frame ();
    }
}
