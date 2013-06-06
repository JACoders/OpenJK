#include <dlfcn.h>
#include <sys/fcntl.h>
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "qcommon/platform.h"
#include "qcommon/files.h"

#include "sys_loadlib.h"
#ifdef DEDICATED
#include "unix_local.h"
#else
#include "sys_local.h"
#endif

static char cdPath[ MAX_OSPATH ] = { 0 };
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
=================
Sys_In_Restart_f
=================
*/
void Sys_In_Restart_f( void )
{
#ifdef DEDICATED
    IN_Shutdown();
	IN_Init();
#else
	IN_Restart( );
#endif
}

void	Sys_Init (void) {
    Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
    Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser( ) );
}

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

void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char    string[1024];
    
	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);
    
	//Sys_ErrorDialog( string );
	Sys_Print( string );
    
	Sys_Exit( 3 );
}

void Sys_Quit (void) {
    CL_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
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

char *Sys_DefaultCDPath(void)
{
	return "";
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/
extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

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
 =================
 Sys_LoadGameDll
 
 Used to load a development dll instead of a virtual machine
 =================
 */

//TODO: load mac dlls that are inside zip things inside pk3s.

void *Sys_LoadGameDll( const char *name, intptr_t (QDECL **entryPoint)(int, ...), intptr_t (QDECL *systemcalls)(intptr_t, ...) )
{
	void	*libHandle = NULL;
	void	(QDECL *dllEntry)( intptr_t (QDECL *syscallptr)(intptr_t, ...) );
	char	*basepath;
	char	*homepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
	char	filename[MAX_OSPATH];

	Com_sprintf (filename, sizeof(filename), "%s" ARCH_STRING DLL_EXT, name);

#if 0
	libHandle = Sys_LoadLibrary( filename );
#endif

	if (!libHandle) {
		//Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", filename, Sys_LibraryError() );

		basepath = Cvar_VariableString( "fs_basepath" );
		homepath = Cvar_VariableString( "fs_homepath" );
		cdpath = Cvar_VariableString( "fs_cdpath" );
		gamedir = Cvar_VariableString( "fs_game" );

		fn = FS_BuildOSPath( basepath, gamedir, filename );
		libHandle = Sys_LoadLibrary( fn );

		if ( !libHandle ) {
			Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
			if( homepath[0] ) {
				Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
				fn = FS_BuildOSPath( homepath, gamedir, filename );
				libHandle = Sys_LoadLibrary( fn );
			}
			if ( !libHandle ) {
				Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
				if( cdpath[0] ) {
					fn = FS_BuildOSPath( cdpath, gamedir, filename );
					libHandle = Sys_LoadLibrary( fn );
				}
				if ( !libHandle ) {
					Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
					// now we try base
					fn = FS_BuildOSPath( basepath, BASEGAME, filename );
					libHandle = Sys_LoadLibrary( fn );
					if ( !libHandle ) {
						if( homepath[0] ) {
							Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
							fn = FS_BuildOSPath( homepath, BASEGAME, filename );
							libHandle = Sys_LoadLibrary( fn );
						}
						if ( !libHandle ) {
							Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
							if( cdpath[0] ) {
								fn = FS_BuildOSPath( cdpath, BASEGAME, filename );
								libHandle = Sys_LoadLibrary( fn );
							}
							if ( !libHandle ) {
								Com_Printf( "Sys_LoadGameDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
								return NULL;
							}
						}
					}
				}
			}
		}
	}

	dllEntry = ( void (QDECL *)( intptr_t (QDECL *)( intptr_t, ... ) ) )Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = (intptr_t (QDECL *)(int,...))Sys_LoadFunction( libHandle, "vmMain" );
	if ( !*entryPoint || !dllEntry ) {
		Com_Printf ( "Sys_LoadGameDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError() );
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	Com_Printf ( "Sys_LoadGameDll(%s) found vmMain function at %p\n", name, *entryPoint );
	dllEntry( systemcalls );

	return libHandle;
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

#ifndef DEFAULT_BASEDIR
//TODO app bundles
//#	ifdef MACOS_X
//#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
//#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
//#	endif
#endif

int main ( int argc, char* argv[] )
{
	int		i;
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };

	// done before Com/Sys_Init since we need this for error output
	//Sys_CreateConsole();
    
	// no abort/retry/fail errors
	//SetErrorMode (SEM_FAILCRITICALERRORS);
    
	// get the initial time base
	Sys_Milliseconds();

	//Sys_InitStreamThread();

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

#if 0
	// if we find the CD, add a +set cddir xxx command line
	Sys_ScanForCD();
#endif

	Com_Init (commandLine);
    
	NET_Init();
    
#ifdef DEDICATED
    fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#else
	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, qfalse);
	}
#endif
    
	// main game loop
	while (1)
	{
#if defined __linux__ && defined DEDICATED
        Sys_ConfigureFPU();//FIXME: what's this for?
#endif
		// make sure mouse and joystick are only called once a frame
		IN_Frame();
        
		// run the game
		Com_Frame();
	}
    
	// never gets here
}
