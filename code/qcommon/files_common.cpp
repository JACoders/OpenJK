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

/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena 
 *
 *
 *****************************************************************************/


#include "q_shared.h"
#include "qcommon.h"
#include "files.h"

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of 
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_QPATH is 64 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created relative to the base path, so base path should usually be writable.

The "cd path" is the path to an alternate hierarchy that will be searched if a file
is not located in the base path.  A user can do a partial install that copies some
data to a base path created on their hard drive and leave the rest on the cd.  Files
are never writen to the cd path.  It defaults to a value set by the installer, like
"e:\quake3", but it can be overridden with "+set ds_cdpath g:\quake3".

If a user runs the game directly from a CD, the base path would be on the CD.  This
should still function correctly, but all file writes will fail (harmlessly).


The "base game" is the directory under the paths where data comes from by default, and
can be either "base" or "demo".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "asset0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

After the paths are initialized, quake will look for the product.txt file.  If not
found and verified, the game will run in restricted mode.  In restricted mode, only 
files contained in demo/asset0.pk3 will be available for loading, and only if the zip header is
verified to not have been modified.  A single exception is made for jaconfig.cfg.  Files
can still be written out in restricted mode, so screenshots and demos are allowed.
Restricted mode can be tested by setting "+set fs_restrict 1" on the command line, even
if there is a valid product.txt under the basepath or cdpath.

If not running in restricted mode, and a file is not found in any local filesystem,
an attempt will be made to download it and save it under the base path.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the cd
path, it will be copied over to the base path.  This is a development aid to help build
test releases and to copy working sets over slow network links.
(If set to 2, copying will only take place if the two filetimes are NOT EQUAL)


The qpath "sound/newstuff/test.wav" would be searched for in the following places:

base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory
base path + base game's zip files
base path + base game's directory
cd path + base game's zip files
cd path + base game's directory
server download, to be written to base path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.




TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.


How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before FS_Restart()?



Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistance?

When building a pak file, make sure a jaconfig.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================

*/

char		fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
cvar_t		*fs_debug;
cvar_t		*fs_homepath;

#ifdef MACOS_X
// Also search the .app bundle for .pk3 files
cvar_t          *fs_apppath;
#endif

cvar_t		*fs_basepath;
cvar_t		*fs_basegame;
cvar_t		*fs_cdpath;
cvar_t		*fs_copyfiles;
cvar_t		*fs_gamedirvar;
cvar_t		*fs_dirbeforepak; //rww - when building search path, keep directories at top and insert pk3's under them
searchpath_t	*fs_searchpaths;
int			fs_readCount;			// total bytes read
int			fs_loadCount;			// total files read
int			fs_packFiles;			// total number of files in packs

qboolean initialized = qfalse;

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];



fileHandleData_t	fsh[MAX_FILE_HANDLES];

void FS_CheckInit(void)
{
	if (!initialized)
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
}


/*
==============
FS_Initialized
==============
*/

qboolean FS_Initialized() {
	return (qboolean)(fs_searchpaths != NULL);
}



fileHandle_t	FS_HandleForFile(void) {
	int		i;

	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o == NULL ) {
			return i;
		}
	}

	Com_Printf( "FS_HandleForFile: all handles taken:\n" );
	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		Com_Printf( "%d. %s\n", i, fsh[i].name);
	}
	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
	return 0;
}


/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void FS_ReplaceSeparators( char *path ) {
	char	*s;
	qboolean lastCharWasSep = qfalse;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			if ( !lastCharWasSep ) {
				*s = PATH_SEP;
				lastCharWasSep = qtrue;
			} else {
				memmove (s, s + 1, strlen (s));
			}
		} else {
			lastCharWasSep = qfalse;
		}
	}
}

/*
===================
FS_BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/

char *FS_BuildOSPath( const char *qpath )
{
	char	temp[MAX_OSPATH];
	static char ospath[2][MAX_OSPATH];
	static int toggle;
	
	toggle ^= 1;		// flip-flop to allow two returns without clash

	// Fix for filenames that are given to FS with a leading "/" (/botfiles/Foo)
	if (qpath[0] == '\\' || qpath[0] == '/')
		qpath++;

	Com_sprintf( temp, sizeof(temp), "/%s/%s", fs_gamedirvar->string, qpath );

	FS_ReplaceSeparators( temp );	
	Com_sprintf( ospath[toggle], sizeof( ospath[0] ), "%s%s", 
		fs_basepath->string, temp );
	
	return ospath[toggle];
}

char *FS_BuildOSPath( const char *base, const char *game, const char *qpath ) {
	char	temp[MAX_OSPATH];
	static char ospath[4][MAX_OSPATH];
	static int toggle;
	
	int nextToggle = (toggle + 1)&3;	// allows four returns without clash (increased from 2 during fs_copyfiles 2 enhancement)
	toggle = nextToggle;
	
	if( !game || !game[0] ) {
		game = fs_gamedir;
	}

	Com_sprintf( temp, sizeof(temp), "/%s/%s", game, qpath );
	FS_ReplaceSeparators( temp );	
	Com_sprintf( ospath[toggle], sizeof( ospath[0] ), "%s%s", base, temp );
	
	return ospath[toggle];
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
qboolean FS_CreatePath (char *OSPath) {
	char	*ofs;
	char	path[MAX_OSPATH];
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return qtrue;
	}

	Q_strncpyz( path, OSPath, sizeof( path ) );
	FS_ReplaceSeparators( path );

	// Skip creation of the root directory as it will always be there
	ofs = strchr( path, PATH_SEP );
	ofs++;

	for (; ofs != NULL && *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {
			// create the directory
			*ofs = 0;
			if (!Sys_Mkdir (path)) {
				Com_Error( ERR_FATAL, "FS_CreatePath: failed to create path \"%s\"",
					path );
			}
			*ofs = PATH_SEP;
		}
	}
	return qfalse;
}



/*
===========
FS_SV_FOpenFileRead

===========
*/
int FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) {
	char *ospath;
	fileHandle_t	f = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	// remove trailing slash
  ospath[strlen(ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileRead: %s\n", ospath );
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		// NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
		if (Q_stricmp(fs_homepath->string,fs_basepath->string))
		{
			// search basepath
			ospath = FS_BuildOSPath( fs_basepath->string, filename, "" );
			ospath[strlen(ospath)-1] = '\0';
			
			if ( fs_debug->integer )
			{
				Com_Printf( "FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath );
			}
			
			fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
			fsh[f].handleSync = qfalse;
			
			if ( !fsh[f].handleFiles.file.o )
			{
				f = 0;
			}
		}
	}
	
	if (!fsh[f].handleFiles.file.o) {
		// search cd path
		ospath = FS_BuildOSPath( fs_cdpath->string, filename, "" );
		ospath[strlen(ospath)-1] = '\0';
		
		if (fs_debug->integer)
		{
			Com_Printf( "FS_SV_FOpenFileRead (fs_cdpath) : %s\n", ospath );
		}
		
		fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
		fsh[f].handleSync = qfalse;
		
		if( !fsh[f].handleFiles.file.o ) {
			f = 0;
		}
	}
  
  *fp = f;
  if (f) {
    return FS_filelength(f);
  }
  return 0;
}




/*
===========
FS_FOpenFileAppend

===========
*/
fileHandle_t FS_FOpenFileAppend( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	FS_CreatePath( ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}


/*
===========
FS_FilenameCompare

Ignore case and seprator char distinctions
===========
*/
qboolean FS_FilenameCompare( const char *s1, const char *s2 ) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( Q_islower(c1) ) {
			c1 -= ('a' - 'A');
		}
		if ( Q_islower(c2) ) {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}
		
		if (c1 != c2) {
			return -1;		// strings not equal
		}
	} while (c1);
	
	return 0;		// strings are equal
}

					
#define	MAXPRINTMSG	4096
void QDECL FS_Printf( fileHandle_t h, const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	FS_Write(msg, strlen(msg), h);
}




/*
============
FS_WriteFile

Filename are relative to the quake search path
============
*/
void FS_WriteFile( const char *qpath, const void *buffer, int size ) {
	fileHandle_t f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !buffer ) {
		Com_Error( ERR_FATAL, "FS_WriteFile: NULL parameter" );
	}

	f = FS_FOpenFileWrite( qpath );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", qpath );
		return;
	}

	FS_Write( buffer, size, f );

	FS_FCloseFile( f );
}








/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( void ) {
	searchpath_t	*p, *next;
	int	i;

	for(i = 0; i < MAX_FILE_HANDLES; i++) {
		if (fsh[i].fileSize) {
			FS_FCloseFile(i);
		}
	}

	// free everything
	for ( p = fs_searchpaths ; p ; p = next ) {
		next = p->next;

		if ( p->pack ) {
			unzClose(p->pack->handle);
			Z_Free( p->pack->buildBuffer );
			Z_Free( p->pack );
		}
		if ( p->dir ) {
			Z_Free( p->dir );
		}
		Z_Free( p );
	}

	// any FS_ calls will now be an error until reinitialized
	fs_searchpaths = NULL;

	Cmd_RemoveCommand( "path" );
	Cmd_RemoveCommand( "dir" );
	Cmd_RemoveCommand( "fdir" );
	Cmd_RemoveCommand( "touchFile" );
	Cmd_RemoveCommand( "which" );

	initialized = qfalse;
}



/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void ) {
	// allow command line parms to override our defaults
	// we don't have to specially handle this, because normal command
	// line variable sets happen before the filesystem
	// has been initialized
	//
	// UPDATE: BTO (VV)
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_copyfiles" );
	Com_StartupVariable( "fs_dirbeforepak" );
#ifdef MACOS_X
	Com_StartupVariable( "fs_apppath" );
#endif

	// try to start up normally
	FS_Startup( BASEGAME );
	initialized = qtrue;

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}




void	FS_Flush( fileHandle_t f ) {
	fflush(fsh[f].handleFiles.file.o);
}

