#include <dlfcn.h>
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "qcommon/platform.h"
#include "qcommon/files.h"

#include "sys_loadlib.h"

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

void Sys_SetDefaultCDPath(const char *path)
{
	Q_strncpyz(cdPath, path, sizeof(cdPath));
}

char *Sys_DefaultCDPath(void)
{
        return cdPath;
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
	char	*fn, *basepath, *homepath, *cdpath, *gamedir;
    
	if(!useSystemLib || !(dllhandle = Sys_LoadLibrary(name)))
	{
		if ( !dllhandle ) {
			basepath = Cvar_VariableString( "fs_basepath" );
			homepath = Cvar_VariableString( "fs_homepath" );
			cdpath = Cvar_VariableString( "fs_cdpath" );
			gamedir = Cvar_VariableString( "fs_game" );
            
			fn = FS_BuildOSPath( basepath, gamedir, name );
			dllhandle = Sys_LoadLibrary( fn );
            
			if ( !dllhandle ) {
				if( homepath[0] ) {
					fn = FS_BuildOSPath( homepath, gamedir, name );
					dllhandle = Sys_LoadLibrary( fn );
				}
				if ( !dllhandle ) {
					if( cdpath[0] ) {
						fn = FS_BuildOSPath( cdpath, gamedir, name );
						dllhandle = Sys_LoadLibrary( fn );
					}
					if ( !dllhandle ) {
						return NULL;
					}
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

void *Sys_LoadGameDll( const char *name, int (**entryPoint)(int, ...), int (*systemcalls)(int, ...) ) 
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
  libHandle = Sys_LoadLibrary( loadname );
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
  libHandle = Sys_LoadLibrary( fn );
 
#ifndef NDEBUG
  if (libHandle == NULL)  Com_Printf("Failed to open DLL\n");
#endif
 
  if ( !libHandle ) {
    if( cdpath[0] ) {
      // bk001206 - report any problem
      Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );

      fn = FS_BuildOSPath( cdpath, gamedir, fname );
      libHandle = Sys_LoadLibrary( fn );
      if ( !libHandle ) {
	// bk001206 - report any problem
	Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
      }
      else
	Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn );
    }
    else
      {
#ifdef NDEBUG // bk001206 - in debug abort on failure
//	Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
#else
	Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
#endif
	//return NULL;
      }
  }

  gamedir = BASEGAME;

  fn = FS_BuildOSPath( basepath, gamedir, fname );
  // bk001206 - verbose
  Com_Printf( "Sys_LoadDll(%s)... \n", fn );

  // bk001129 - from cvs1.17 (mkv), was fname not fn
  libHandle = Sys_LoadLibrary( fn );

#ifndef NDEBUG
  if (libHandle == NULL)  Com_Printf("Failed to open DLL\n");
#endif

  if ( !libHandle ) {
    if( cdpath[0] ) {
      // bk001206 - report any problem
      Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );

      fn = FS_BuildOSPath( cdpath, gamedir, fname );
      libHandle = Sys_LoadLibrary( fn );
      if ( !libHandle ) {
	// bk001206 - report any problem
	Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
      }
      else
	Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", fn );
    }
    else
      {
#ifdef NDEBUG // bk001206 - in debug abort on failure
//	Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
#else
	Com_Printf( "Sys_LoadDll(%s) failed: \"%s\"\n", fn, Sys_LibraryError() );
#endif
	return NULL;
      }
  }
  // bk001206 - no different behavior
  //#ifndef NDEBUG }
  //else Com_Printf ( "Sys_LoadDll(%s): succeeded ...\n", loadname );
  //#endif

  dllEntry = (void (*)(int (*)(int,...))) Sys_LoadFunction( libHandle, "dllEntry" ); 
  if (!dllEntry)
  {
     err = Sys_LibraryError();
     Com_Printf("Sys_LoadDLL(%s) failed dlsym(dllEntry): \"%s\" ! \n",name,err);
  }
  //int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  )
  *entryPoint = (int(*)(int,...))Sys_LoadFunction( libHandle, "vmMain" );
  if (!*entryPoint)
     err = Sys_LibraryError();
  if ( !*entryPoint || !dllEntry ) {
#ifdef NDEBUG // bk001206 - in debug abort on failure
//    Com_Error ( ERR_FATAL, "Sys_LoadDll(%s) failed dlsym(vmMain): \"%s\" !\n", name, err );
#else
    Com_Printf ( "Sys_LoadDll(%s) failed dlsym(vmMain): \"%s\" !\n", name, err );
#endif
    Sys_UnloadLibrary( libHandle );
    err = Sys_LibraryError();
    if ( err != NULL )
      Com_Printf ( "Sys_LoadDll(%s) failed dlcose: \"%s\"\n", name, err );
    return NULL;
  }
  Com_Printf ( "Sys_LoadDll(%s) found **vmMain** at  %p  \n", name, *entryPoint ); // bk001212
  dllEntry( systemcalls );
  Com_Printf ( "Sys_LoadDll(%s) succeeded!\n", name );
  return libHandle;
}
