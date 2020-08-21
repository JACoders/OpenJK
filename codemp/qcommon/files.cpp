/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

/*****************************************************************************

 * name:		files.cpp
 *
 * desc:		file code
 *
 *****************************************************************************/

#include "qcommon/qcommon.h"

#ifndef DEDICATED
#ifndef FINAL_BUILD
#include "client/client.h"
#endif
#endif
#include <minizip/unzip.h>

#if defined(_WIN32)
#include <windows.h>
#endif

// for rmdir
#if defined (_MSC_VER)
	#include <direct.h>
#else
	#include <unistd.h>
#endif

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created relative to the base path, so base path should usually be writable.

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "cd path" for game content.


The "base game" is the directory under the paths where data comes from by default, and
can be either "baseq3" or "demoq3".

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
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes.  Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory
cd path + current game's zip files
cd path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory
cd path + base game's zip file
cd path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory
cd path + BASEGAME's zip file
cd path + BASEGAME's directory

server download, to be written to home path + current game's directory


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

When building a pak file, make sure a q3config.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================

*/

#define MAX_ZPATH			256
#define	MAX_SEARCH_PATHS	4096
#define MAX_FILEHASH_SIZE	1024

typedef struct fileInPack_s {
	char					*name;		// name of the file
	unsigned long			pos;		// file info position in zip
	unsigned long			len;		// uncompress file size
	struct	fileInPack_s*	next;		// next file in the hash
} fileInPack_t;

typedef struct pack_s {
	char			pakPathname[MAX_OSPATH];	// c:\jediacademy\gamedata\base
	char			pakFilename[MAX_OSPATH];	// c:\jediacademy\gamedata\base\assets0.pk3
	char			pakBasename[MAX_OSPATH];	// assets0
	char			pakGamename[MAX_OSPATH];	// base
	unzFile			handle;						// handle to zip file
	int				checksum;					// regular checksum
	int				pure_checksum;				// checksum for pure
	int				numfiles;					// number of files in pk3
	int				referenced;					// referenced file flags
	int				hashSize;					// hash table size (power of 2)
	fileInPack_t*	*hashTable;					// hash table
	fileInPack_t*	buildBuffer;				// buffer with the filenames etc.
} pack_t;

typedef struct directory_s {
	char		path[MAX_OSPATH];		// c:\jediacademy\gamedata
	char		fullpath[MAX_OSPATH];	// c:\jediacademy\gamedata\base
	char		gamedir[MAX_OSPATH];	// base
} directory_t;

typedef struct searchpath_s {
	struct searchpath_s *next;

	pack_t		*pack;		// only one of pack / dir will be non NULL
	directory_t	*dir;
} searchpath_t;

static char		fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
static cvar_t		*fs_debug;
static cvar_t		*fs_homepath;

#ifdef MACOS_X
// Also search the .app bundle for .pk3 files
static cvar_t          *fs_apppath;
#endif

static cvar_t		*fs_basepath;
static cvar_t		*fs_basegame;
static cvar_t		*fs_cdpath;
static cvar_t		*fs_copyfiles;
static cvar_t		*fs_gamedirvar;
static cvar_t		*fs_dirbeforepak; //rww - when building search path, keep directories at top and insert pk3's under them
static searchpath_t	*fs_searchpaths;
static int			fs_readCount;			// total bytes read
static int			fs_loadCount;			// total files read
static int			fs_packFiles = 0;		// total number of files in packs

static int			fs_fakeChkSum;
static int			fs_checksumFeed;

typedef union qfile_gus {
	FILE*		o;
	unzFile		z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut	file;
	qboolean	unique;
} qfile_ut;

typedef struct fileHandleData_s {
	qfile_ut	handleFiles;
	qboolean	handleSync;
	int			fileSize;
	int			zipFilePos;
	int			zipFileLen;
	qboolean	zipFile;
	char		name[MAX_ZPATH];
} fileHandleData_t;

static fileHandleData_t	fsh[MAX_FILE_HANDLES];

// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
// wether we did a reorder on the current search path when joining the server
static qboolean fs_reordered = qfalse;

// never load anything from pk3 files that are not present at the server when pure
static int		fs_numServerPaks = 0;
static int		fs_serverPaks[MAX_SEARCH_PATHS];				// checksums
static char		*fs_serverPakNames[MAX_SEARCH_PATHS];			// pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static int		fs_numServerReferencedPaks;
static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];			// checksums
static char	*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];		// pk3 names

#if defined(_WIN32)
// temporary files - store them in a circular buffer. We're pretty
// much guaranteed to not need more than 8 temp files at a time.
static int		fs_temporaryFileWriteIdx = 0;
static char		fs_temporaryFileNames[8][MAX_OSPATH];
#endif

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE*		missingFiles = NULL;
#endif

/* C99 defines __func__ */
#if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2 || _MSC_VER >= 1300
#    define __func__ __FUNCTION__
#  else
#    define __func__ "(unknown)"
#  endif
#endif

/*
==============
FS_Initialized
==============
*/

qboolean FS_Initialized( void ) {
	return (qboolean)(fs_searchpaths != NULL);
}

static void FS_AssertInitialised( void ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
}

/*
=================
FS_PakIsPure
=================
*/
qboolean FS_PakIsPure( pack_t *pack ) {
	int i;

	if ( fs_numServerPaks ) {
    // NOTE TTimo we are matching checksums without checking the pak names
    //   this means you can have the same pk3 as the server under a different name, you will still get through sv_pure validation
    //   (what happens when two pk3's have the same checkums? is it a likely situation?)
    //   also, if there's a wrong checksumed pk3 and autodownload is enabled, the checksum will be appended to the downloaded pk3 name
		for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
			// FIXME: also use hashed file names
			if ( pack->checksum == fs_serverPaks[i] ) {
				return qtrue;		// on the aproved list
			}
		}
		return qfalse;	// not on the pure server pak list
	}
	return qtrue;
}

/*
================
return a hash value for the filename
================
*/
static long FS_HashFileName( const char *fname, int hashSize ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}

static fileHandle_t FS_HandleForFile(void) {
	int		i;

	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o == NULL ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
	return 0;
}

static FILE	*FS_FileForHandle( fileHandle_t f ) {
	if ( f < 1 || f >= MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of range" );
	}
	if (fsh[f].zipFile == qtrue) {
		Com_Error( ERR_DROP, "FS_FileForHandle: can't get FILE on zip file" );
	}
	if ( ! fsh[f].handleFiles.file.o ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: NULL" );
	}

	return fsh[f].handleFiles.file.o;
}

void	FS_ForceFlush( fileHandle_t f ) {
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf( file, NULL, _IONBF, 0 );
}

/*
================
FS_fplength
================
*/

long FS_fplength(FILE *h)
{
	long		pos;
	long		end;

	pos = ftell(h);
	if ( pos == EOF )
		return EOF;

	fseek(h, 0, SEEK_END);
	end = ftell(h);
	fseek(h, pos, SEEK_SET);

	return end;
}

/*
================
FS_filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelength( fileHandle_t f ) {
	FILE	*h;

	h = FS_FileForHandle(f);

	if(h == NULL)
		return EOF;
	else
		return FS_fplength(h);
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
char *FS_BuildOSPath( const char *qpath ) {
	char	temp[MAX_OSPATH];
	static char ospath[4][MAX_OSPATH];
	static int toggle;

	int nextToggle = (toggle + 1)&3;	// allows four returns without clash (increased from 2 during fs_copyfiles 2 enhancement)
	toggle = nextToggle;

	// Fix for filenames that are given to FS with a leading "/" (/botfiles/Foo)
	if (qpath[0] == '\\' || qpath[0] == '/')
		qpath++;

	Com_sprintf( temp, sizeof(temp), "/base/%s", qpath );
	FS_ReplaceSeparators( temp );
	Com_sprintf( ospath[toggle], sizeof( ospath[0] ), "%s%s", fs_basepath->string, temp );

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
	if ( ofs ) {
		ofs++;
	}

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
=================
FS_CheckFilenameIsMutable

ERR_FATAL if trying to maniuplate a file with the platform library, or pk3 extension
=================
*/
static void FS_CheckFilenameIsMutable( const char *filename, const char *function )
{
	// Check if the filename ends with the library, or pk3 extension
	if( COM_CompareExtension( filename, DLL_EXT )
		|| COM_CompareExtension( filename, ".pk3" ) )
	{
		Com_Error( ERR_FATAL, "%s: Not allowed to manipulate '%s' due "
			"to %s extension", function, filename, COM_GetExtension( filename ) );
	}
}

/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
void FS_CopyFile( char *fromOSPath, char *toOSPath ) {
	FILE	*f;
	int		len;
	byte	*buf;

	FS_CheckFilenameIsMutable( fromOSPath, __func__ );

	Com_Printf( "copy %s to %s\n", fromOSPath, toOSPath );

	if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat")) {
		Com_Printf( "Ignoring journal files\n");
		return;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek (f, 0, SEEK_END);
	len = ftell (f);
	fseek (f, 0, SEEK_SET);

	if ( len == EOF )
	{
		fclose( f );
		Com_Error( ERR_FATAL, "Bad file length in FS_CopyFile()" );
	}

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = (unsigned char *)malloc( len );
	if (fread( buf, 1, len, f ) != (unsigned)len)
	{
		fclose( f );
		free ( buf );
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if( FS_CreatePath( toOSPath ) ) {
		free ( buf );
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		free ( buf );
		return;
	}
	if (fwrite( buf, 1, len, f ) != (unsigned)len)
	{
		fclose( f );
		free ( buf );
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}

/*
===========
FS_Remove

===========
*/
void FS_Remove( const char *osPath ) {
	FS_CheckFilenameIsMutable( osPath, __func__ );

	remove( osPath );
}

/*
===========
FS_HomeRemove

===========
*/
void FS_HomeRemove( const char *homePath ) {
	FS_CheckFilenameIsMutable( homePath, __func__ );

	remove( FS_BuildOSPath( fs_homepath->string,
			fs_gamedir, homePath ) );
}

/*
===========
FS_Rmdir

Removes a directory, optionally deleting all files under it
===========
*/
void FS_Rmdir( const char *osPath, qboolean recursive ) {
	FS_CheckFilenameIsMutable( osPath, __func__ );

	if ( recursive ) {
		int numfiles;
		int i;
		char **filesToRemove = Sys_ListFiles( osPath, "", NULL, &numfiles, qfalse );
		for ( i = 0; i < numfiles; i++ ) {
			char fileOsPath[MAX_OSPATH];
			Com_sprintf( fileOsPath, sizeof( fileOsPath ), "%s/%s", osPath, filesToRemove[i] );
			FS_Remove( fileOsPath );
		}
		FS_FreeFileList( filesToRemove );

		char **directoriesToRemove = Sys_ListFiles( osPath, "/", NULL, &numfiles, qfalse );
		for ( i = 0; i < numfiles; i++ ) {
			if ( !Q_stricmp( directoriesToRemove[i], "." ) || !Q_stricmp( directoriesToRemove[i], ".." ) ) {
				continue;
			}
			char directoryOsPath[MAX_OSPATH];
			Com_sprintf( directoryOsPath, sizeof( directoryOsPath ), "%s/%s", osPath, directoriesToRemove[i] );
			FS_Rmdir( directoryOsPath, qtrue );
		}
		FS_FreeFileList( directoriesToRemove );
	}

	rmdir( osPath );
}

/*
===========
FS_HomeRmdir

Removes a directory, optionally deleting all files under it
===========
*/
void FS_HomeRmdir( const char *homePath, qboolean recursive ) {
	FS_CheckFilenameIsMutable( homePath, __func__ );

	FS_Rmdir( FS_BuildOSPath( fs_homepath->string,
					fs_gamedir, homePath ), recursive );
}

/*
================
FS_FileInPathExists

Tests if path and file exists
================
*/
qboolean FS_FileInPathExists(const char *testpath)
{
	FILE *filep;

	filep = fopen(testpath, "rb");

	if(filep)
	{
		fclose(filep);
		return qtrue;
	}

	return qfalse;
}

/*
================
FS_FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
================
*/
qboolean FS_FileExists( const char *file )
{
	return FS_FileInPathExists(FS_BuildOSPath(fs_homepath->string, fs_gamedir, file));
}

/*
================
FS_SV_FileExists

Tests if the file exists
================
*/
qboolean FS_SV_FileExists( const char *file )
{
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, file, "");
	testpath[strlen(testpath)-1] = '\0';

	return FS_FileInPathExists(testpath);
}

/*
===========
FS_SV_FOpenFileWrite

===========
*/
fileHandle_t FS_SV_FOpenFileWrite( const char *filename ) {
	char *ospath;
	fileHandle_t	f;

	FS_AssertInitialised();

	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	ospath[strlen(ospath)-1] = '\0';

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileWrite: %s\n", ospath );
	}

	FS_CheckFilenameIsMutable( ospath, __func__ );

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

/*
===========
FS_SV_FOpenFileAppend

===========
*/
fileHandle_t FS_SV_FOpenFileAppend( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	FS_AssertInitialised();

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	ospath[strlen(ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileAppend: %s\n", ospath );
	}

	FS_CheckFilenameIsMutable( ospath, __func__ );

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[f].handleSync = qfalse;

	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}

	return f;
}

/*
===========
FS_SV_FOpenFileRead
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) {
	char *ospath;
	fileHandle_t	f = 0;

	FS_AssertInitialised();

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	// search homepath
	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	// remove trailing slash
	ospath[strlen(ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath );
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o)
	{
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
		}

		if ( !fsh[f].handleFiles.file.o )
		{
			f = 0;
		}
	}

	if (!fsh[f].handleFiles.file.o)
	{
		// search cd path
		ospath = FS_BuildOSPath( fs_cdpath->string, filename, "" );
		ospath[strlen(ospath)-1] = '\0';

		if (fs_debug->integer)
		{
			Com_Printf( "FS_SV_FOpenFileRead (fs_cdpath) : %s\n", ospath );
		}

		fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
		fsh[f].handleSync = qfalse;

		if ( !fsh[f].handleFiles.file.o )
		{
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
FS_SV_Rename

===========
*/
void FS_SV_Rename( const char *from, const char *to, qboolean safe ) {
	char			*from_ospath, *to_ospath;

	FS_AssertInitialised();

	// don't let sound stutter
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, from, "" );
	to_ospath = FS_BuildOSPath( fs_homepath->string, to, "" );
	from_ospath[strlen(from_ospath)-1] = '\0';
	to_ospath[strlen(to_ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath );
	}

	if ( safe ) {
		FS_CheckFilenameIsMutable( to_ospath, __func__ );
	}

	if (rename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFile ( from_ospath, to_ospath );
		FS_Remove ( from_ospath );
	}
}

/*
===========
FS_Rename

===========
*/
void FS_Rename( const char *from, const char *to ) {
	char			*from_ospath, *to_ospath;

	FS_AssertInitialised();

	// don't let sound stutter
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	to_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_Rename: %s --> %s\n", from_ospath, to_ospath );
	}

	FS_CheckFilenameIsMutable( to_ospath, __func__ );

	if (rename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFile ( from_ospath, to_ospath );
		FS_Remove ( from_ospath );
	}
}

/*
===========
FS_FCloseFile

Close a file.

There are three cases handled:

  * normal file: closed with fclose.

  * file in pak3 archive: subfile is closed with unzCloseCurrentFile, but the
    minizip handle to the pak3 remains open.

  * file in pak3 archive, opened with "unique" flag: This file did not use
    the system minizip handle to the pak3 file, but its own dedicated one.
    The dedicated handle is closed with unzClose.

===========
*/
void FS_FCloseFile( fileHandle_t f ) {
	FS_AssertInitialised();

	if (fsh[f].zipFile == qtrue) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if (fsh[f].handleFiles.file.o) {
		fclose (fsh[f].handleFiles.file.o);
	}
	Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWrite( const char *filename, qboolean safe ) {
	char			*ospath;
	fileHandle_t	f;

	FS_AssertInitialised();

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if ( safe ) {
		FS_CheckFilenameIsMutable( ospath, __func__ );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

/*
===========
FS_FOpenFileAppend

===========
*/
fileHandle_t FS_FOpenFileAppend( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	FS_AssertInitialised();

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	FS_CheckFilenameIsMutable( ospath, __func__ );

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

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

Ignore case and separator char distinctions
===========
*/
qboolean FS_FilenameCompare( const char *s1, const char *s2 ) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if (c1 != c2) {
			return qtrue;		// strings not equal
		}
	} while (c1);

	return qfalse;		// strings are equal
}

/*
===========
FS_IsExt

Return qtrue if ext matches file extension filename
===========
*/

qboolean FS_IsExt(const char *filename, const char *ext, int namelen)
{
	int extlen;

	extlen = strlen(ext);

	if(extlen > namelen)
		return qfalse;

	filename += namelen - extlen;

	return (qboolean)!Q_stricmp(filename, ext);
}

/*
===========
FS_IsDemoExt

Return qtrue if filename has a demo extension
===========
*/

#define DEMO_EXTENSION "dm_"
qboolean FS_IsDemoExt(const char *filename, int namelen)
{
	const char *ext_test;

	ext_test = strrchr(filename, '.');
	if(ext_test && !Q_stricmpn(ext_test + 1, DEMO_EXTENSION, ARRAY_LEN(DEMO_EXTENSION) - 1))
	{
		int protocol = atoi(ext_test + ARRAY_LEN(DEMO_EXTENSION));

		if(protocol == PROTOCOL_VERSION)
			return qtrue;
	}

	return qfalse;
}

#ifdef _WIN32

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

bool Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ )
{
	FILETIME ftFinalFile, ftDataFile;

	if (Sys_GetFileTime(psFinalFileName, ftFinalFile) && Sys_GetFileTime(psDataFileName, ftDataFile))
	{
		// timer res only accurate to within 2 seconds on FAT, so can't do exact compare...
		//
		//LONG l = CompareFileTime( &ftFinalFile, &ftDataFile );
		if (  (fabs((double)(ftFinalFile.dwLowDateTime - ftDataFile.dwLowDateTime)) <= 20000000 ) &&
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

#endif // _WIN32

bool FS_FileCacheable(const char* const filename)
{
	extern	cvar_t	*com_buildScript;
	if (com_buildScript && com_buildScript->integer)
	{
		return true;
	}
	return( strchr(filename, '/') != 0 );
}

/*
===========
FS_FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
extern qboolean		com_fullyInitialized;

long FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE ) {
	searchpath_t	*search;
	char			*netpath;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	directory_t		*dir;
	long			hash;
	//unz_s			*zfi;
	//void			*temp;
	int				l;
	bool			isUserConfig = false;

	hash = 0;

	FS_AssertInitialised();

	if ( file == NULL ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'file' parameter passed\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		*file = 0;
		return -1;
	}

	// make sure the q3key file is only readable by the quake3.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if( com_fullyInitialized && strstr( filename, "q3key" ) ) {
		*file = 0;
		return -1;
	}

	isUserConfig = !Q_stricmp( filename, "autoexec.cfg" ) || !Q_stricmp( filename, Q3CONFIG_CFG );

	//
	// search through the path, one element at a time
	//

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	// this new bool is in for an optimisation, if you (eg) opened a BSP file under fs_copyfiles==2,
	//	then it triggered a copy operation to update your local HD version, then this will re-open the
	//	file handle on your local version, not the net build. This uses a bit more CPU to re-do the loop
	//	logic, but should read faster than accessing the net version a second time.
	//
	qboolean bFasterToReOpenUsingNewLocalFile = qfalse;

	do
	{
		bFasterToReOpenUsingNewLocalFile = qfalse;

		for ( search = fs_searchpaths ; search ; search = search->next ) {
			//
			if ( search->pack ) {
				hash = FS_HashFileName(filename, search->pack->hashSize);
			}
			// is the element a pak file?
			if ( search->pack && search->pack->hashTable[hash] ) {
				// disregard if it doesn't match one of the allowed pure pak files
				if ( !FS_PakIsPure(search->pack) ) {
					continue;
				}

				// autoexec.cfg and openjk.cfg can only be loaded outside of pk3 files.
				if ( isUserConfig ) {
					continue;
				}

				// look through all the pak file elements
				pak = search->pack;
				pakFile = pak->hashTable[hash];
				do {
					// case and separator insensitive comparisons
					if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
						// found it!

						// mark the pak as having been referenced and mark specifics on cgame and ui
						// shaders, txt, arena files  by themselves do not count as a reference as
						// these are loaded from all pk3s
						// from every pk3 file..

						// The x86.dll suffixes are needed in order for sv_pure to continue to
						// work on non-x86/windows systems...

						l = strlen( filename );
						if ( !(pak->referenced & FS_GENERAL_REF)) {
							if( !FS_IsExt(filename, ".shader", l) &&
							    !FS_IsExt(filename, ".txt", l) &&
							    !FS_IsExt(filename, ".str", l) &&
							    !FS_IsExt(filename, ".cfg", l) &&
							    !FS_IsExt(filename, ".config", l) &&
							    !FS_IsExt(filename, ".bot", l) &&
							    !FS_IsExt(filename, ".arena", l) &&
							    !FS_IsExt(filename, ".menu", l) &&
							    !FS_IsExt(filename, ".fcf", l) &&
							    Q_stricmp(filename, "jampgamex86.dll") != 0 &&
							    //Q_stricmp(filename, "vm/qagame.qvm") != 0 &&
							    !strstr(filename, "levelshots"))
							{
								pak->referenced |= FS_GENERAL_REF;
							}
						}

						if (!(pak->referenced & FS_CGAME_REF))
						{
							if ( Q_stricmp( filename, "cgame.qvm" ) == 0 ||
									Q_stricmp( filename, "cgamex86.dll" ) == 0 )
							{
								pak->referenced |= FS_CGAME_REF;
							}
						}

						if (!(pak->referenced & FS_UI_REF))
						{
							if ( Q_stricmp( filename, "ui.qvm" ) == 0 ||
									Q_stricmp( filename, "uix86.dll" ) == 0 )
							{
								pak->referenced |= FS_UI_REF;
							}
						}

						if ( uniqueFILE ) {
							// open a new file on the pakfile
							fsh[*file].handleFiles.file.z = unzOpen (pak->pakFilename);
							if (fsh[*file].handleFiles.file.z == NULL) {
								Com_Error (ERR_FATAL, "Couldn't open %s", pak->pakFilename);
							}
						} else {
							fsh[*file].handleFiles.file.z = pak->handle;
						}
						Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
						fsh[*file].zipFile = qtrue;

						// set the file position in the zip file (also sets the current file info)
						unzSetOffset(fsh[*file].handleFiles.file.z, pakFile->pos);

						// open the file in the zip
						unzOpenCurrentFile(fsh[*file].handleFiles.file.z);

#if 0
						zfi = (unz_s *)fsh[*file].handleFiles.file.z;
						// in case the file was new
						temp = zfi->filestream;
						// set the file position in the zip file (also sets the current file info)
						unzSetOffset(pak->handle, pakFile->pos);
						// copy the file info into the unzip structure
						Com_Memcpy( zfi, pak->handle, sizeof(unz_s) );
						// we copy this back into the structure
						zfi->filestream = temp;
						// open the file in the zip
						unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
#endif
						fsh[*file].zipFilePos = pakFile->pos;
						fsh[*file].zipFileLen = pakFile->len;

						if ( fs_debug->integer ) {
							Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n",
								filename, pak->pakFilename );
						}
	#ifndef DEDICATED
	#ifndef FINAL_BUILD
						// Check for unprecached files when in game but not in the menus
						if((cls.state == CA_ACTIVE) && !(Key_GetCatcher( ) & KEYCATCH_UI))
						{
							Com_Printf(S_COLOR_YELLOW "WARNING: File %s not precached\n", filename);
						}
	#endif
	#endif // DEDICATED
						return pakFile->len;
					}
					pakFile = pakFile->next;
				} while(pakFile != NULL);
			} else if ( search->dir ) {
				// check a file in the directory tree

				// if we are running restricted, the only files we
				// will allow to come from the directory are .cfg files
				l = strlen( filename );
		  // FIXME TTimo I'm not sure about the fs_numServerPaks test
		  // if you are using FS_ReadFile to find out if a file exists,
		  //   this test can make the search fail although the file is in the directory
		  // I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
		  // turned out I used FS_FileExists instead
				if ( fs_numServerPaks ) {
					if ( !FS_IsExt( filename, ".cfg", l ) &&		// for config files
					    !FS_IsExt( filename, ".fcf", l ) &&		// force configuration files
					    !FS_IsExt( filename, ".menu", l ) &&		// menu files
					    !FS_IsExt( filename, ".game", l ) &&		// menu files
					    !FS_IsExt( filename, ".dat", l ) &&		// for journal files
					    !FS_IsDemoExt( filename, l ) ) {			// demos
						continue;
					}
				}

				dir = search->dir;

				netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
				fsh[*file].handleFiles.file.o = fopen (netpath, "rb");
				if ( !fsh[*file].handleFiles.file.o ) {
					continue;
				}

				if ( !FS_IsExt( filename, ".cfg", l ) &&		// for config files
					!FS_IsExt( filename, ".fcf", l ) &&		// force configuration files
					!FS_IsExt( filename, ".menu", l ) &&		// menu files
					!FS_IsExt( filename, ".game", l ) &&		// menu files
					!FS_IsExt( filename, ".dat", l ) &&		// for journal files
					!FS_IsDemoExt( filename, l ) ) {			// demos
					fs_fakeChkSum = Q_flrand(0.0f, 1.0f);
				}
#ifdef _WIN32
				// if running with fs_copyfiles 2, and search path == local, then we need to fail to open
				//	if the time/date stamp != the network version (so it'll loop round again and use the network path,
				//	which comes later in the search order)
				//
				if ( fs_copyfiles->integer == 2 && fs_cdpath->string[0] && !Q_stricmp( dir->path, fs_basepath->string )
					&& FS_FileCacheable(filename) )
				{
					if ( Sys_FileOutOfDate( netpath, FS_BuildOSPath( fs_cdpath->string, dir->gamedir, filename ) ))
					{
						fclose(fsh[*file].handleFiles.file.o);
						fsh[*file].handleFiles.file.o = 0;
						continue;	//carry on to find the cdpath version.
					}
				}
#endif
				Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
				fsh[*file].zipFile = qfalse;
				if ( fs_debug->integer ) {
					Com_Printf( "FS_FOpenFileRead: %s (found in '%s%c%s')\n", filename,
						dir->path, PATH_SEP, dir->gamedir );
				}

#ifdef _WIN32
				// if we are getting it from the cdpath, optionally copy it
				//  to the basepath
				if ( fs_copyfiles->integer && !Q_stricmp( dir->path, fs_cdpath->string ) ) {
					char	*copypath;

					copypath = FS_BuildOSPath( fs_basepath->string, dir->gamedir, filename );
					switch ( fs_copyfiles->integer )
					{
						default:
						case 1:
						{
							FS_CopyFile( netpath, copypath );
						}
						break;

						case 2:
						{

							if (FS_FileCacheable(filename) )
							{
								// maybe change this to Com_DPrintf?   On the other hand...
								//
								Com_Printf( "fs_copyfiles(2), Copying: %s to %s\n", netpath, copypath );

								FS_CreatePath( copypath );

								bool bOk = true;
								if (!CopyFile( netpath, copypath, FALSE ))
								{
									DWORD dwAttrs = GetFileAttributes(copypath);
									SetFileAttributes(copypath, dwAttrs & ~FILE_ATTRIBUTE_READONLY);
									bOk = !!CopyFile( netpath, copypath, FALSE );
								}

								if (bOk)
								{
									// clear this handle and setup for re-opening of the new local copy...
									//
									bFasterToReOpenUsingNewLocalFile = qtrue;
									fclose(fsh[*file].handleFiles.file.o);
									fsh[*file].handleFiles.file.o = NULL;
								}
							}
						}
						break;
					}
				}
#endif
				if (bFasterToReOpenUsingNewLocalFile)
				{
					break;	// and re-read the local copy, not the net version
				}

	#ifndef DEDICATED
	#ifndef FINAL_BUILD
				// Check for unprecached files when in game but not in the menus
				if((cls.state == CA_ACTIVE) && !(Key_GetCatcher( ) & KEYCATCH_UI))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: File %s not precached\n", filename);
				}
	#endif
	#endif // dedicated
				return FS_fplength(fsh[*file].handleFiles.file.o);
			}
		}
	}
	while ( bFasterToReOpenUsingNewLocalFile );

	Com_DPrintf ("Can't find %s\n", filename);
#ifdef FS_MISSING
	if (missingFiles) {
		fprintf(missingFiles, "%s\n", filename);
	}
#endif
	*file = 0;
	return -1;
}

// This is a bit of a hack but it is used for other OS'/arch to still be acceptable with pure servers.
// Intentionally looking for x86.dll because this is all that exists in pk3s.
qboolean FS_FindPureDLL(const char *name)
{
	char dllName[MAX_OSPATH];
	fileHandle_t h;

	if(!fs_searchpaths)
		Com_Error(ERR_FATAL, "Filesystem call made without initialization");

	if ( !Cvar_VariableValue( "sv_pure" ) )
		return qtrue;

	Com_sprintf(dllName, sizeof(dllName), "%sx86.dll", name);

	if(FS_FOpenFileRead(dllName, &h, qtrue) > 0)
	{
		FS_FCloseFile( h );
		return qtrue;
	}
	return qfalse;
}

/*
=================
FS_Read

Properly handles partial reads
=================
*/
int FS_Read( void *buffer, int len, fileHandle_t f ) {
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	FS_AssertInitialised();

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;
	fs_readCount += len;

	if (fsh[f].zipFile == qfalse) {
		remaining = len;
		tries = 0;
		while (remaining) {
			block = remaining;
			read = fread (buf, 1, block, fsh[f].handleFiles.file.o);
			if (read == 0) {
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if (!tries) {
					tries = 1;
				} else {
					return len-remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if (read == -1) {
				Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");
			}

			remaining -= read;
			buf += read;
		}
		return len;
	} else {
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
	}
}

/*
=================
FS_Write

Properly handles partial writes
=================
*/
int FS_Write( const void *buffer, int len, fileHandle_t h ) {
	int		block, remaining;
	int		written;
	byte	*buf;
	int		tries;
	FILE	*f;

	FS_AssertInitialised();

	if ( !h ) {
		return 0;
	}

	f = FS_FileForHandle(h);
	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while (remaining) {
		block = remaining;
		written = fwrite (buf, 1, block, f);
		if (written == 0) {
			if (!tries) {
				tries = 1;
			} else {
				Com_Printf( "FS_Write: 0 bytes written\n" );
				return 0;
			}
		}

		if (written == -1) {
			Com_Printf( "FS_Write: -1 bytes written\n" );
			return 0;
		}

		remaining -= written;
		buf += written;
	}
	if ( fsh[h].handleSync ) {
		fflush( f );
	}
	return len;
}

void QDECL FS_Printf( fileHandle_t h, const char *fmt, ... ) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	FS_Write(msg, strlen(msg), h);
}

#define PK3_SEEK_BUFFER_SIZE 65536
/*
=================
FS_Seek

=================
*/
int FS_Seek( fileHandle_t f, long offset, int origin ) {
	int		_origin;

	FS_AssertInitialised();

	if (fsh[f].zipFile == qtrue) {
		//FIXME: this is really, really crappy
		//(but better than what was here before)
		byte	buffer[PK3_SEEK_BUFFER_SIZE];
		int		remainder;
		int		currentPosition = FS_FTell( f );

		// change negative offsets into FS_SEEK_SET
		if ( offset < 0 ) {
			switch( origin ) {
				case FS_SEEK_END:
					remainder = fsh[f].zipFileLen + offset;
					break;

				case FS_SEEK_CUR:
					remainder = currentPosition + offset;
					break;

				case FS_SEEK_SET:
				default:
					remainder = 0;
					break;
			}

			if ( remainder < 0 ) {
				remainder = 0;
			}

			origin = FS_SEEK_SET;
		} else {
			if ( origin == FS_SEEK_END ) {
				remainder = fsh[f].zipFileLen - currentPosition + offset;
			} else {
				remainder = offset;
			}
		}

		switch( origin ) {
			case FS_SEEK_SET:
				if ( remainder == currentPosition ) {
					return offset;
				}
				unzSetOffset(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
				unzOpenCurrentFile(fsh[f].handleFiles.file.z);
				//fallthrough

			case FS_SEEK_END:
			case FS_SEEK_CUR:
				while( remainder > PK3_SEEK_BUFFER_SIZE ) {
					FS_Read( buffer, PK3_SEEK_BUFFER_SIZE, f );
					remainder -= PK3_SEEK_BUFFER_SIZE;
				}
				FS_Read( buffer, remainder, f );
				return offset;

			default:
				Com_Error( ERR_FATAL, "Bad origin in FS_Seek" );
				return -1;
		}
	} else {
		FILE *file;
		file = FS_FileForHandle(f);
		switch( origin ) {
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		return fseek( file, offset, _origin );
	}
}

/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

int	FS_FileIsInPAK(const char *filename, int *pChecksum ) {
	searchpath_t	*search;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	long			hash = 0;

	FS_AssertInitialised();

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		//
		if (search->pack) {
			hash = FS_HashFileName(filename, search->pack->hashSize);
		}
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {
			// disregard if it doesn't match one of the allowed pure pak files
			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
					if (pChecksum) {
						*pChecksum = pak->pure_checksum;
					}
					return 1;
				}
				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}
	return -1;
}

/*
============
FS_ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
long FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t	h;
	byte*			buf;
	qboolean		isConfig;
	long				len;

	FS_AssertInitialised();

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = qtrue;
		if ( com_journal && com_journal->integer == 2 ) {
			int		r;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );
			if ( r != sizeof( len ) ) {
				if (buffer != NULL) *buffer = NULL;
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len) {
				if (buffer == NULL) {
					return 1;			// hack for old journal files
				}
				*buffer = NULL;
				return -1;
			}
			if (buffer == NULL) {
				return len;
			}

			buf = (unsigned char *)Hunk_AllocateTempMemory(len+1);
			*buffer = buf;

			r = FS_Read( buf, len, com_journalDataFile );
			if ( r != len ) {
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

			fs_loadCount++;

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = qfalse;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, qfalse );
	if ( h == 0 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		return -1;
	}

	if ( !buffer ) {
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		FS_FCloseFile( h);
		return len;
	}

	fs_loadCount++;

	buf = (byte*)Z_Malloc( len+1, TAG_FILESYS, qfalse);
	buf[len]='\0';	// because we're not calling Z_Malloc with optional trailing 'bZeroIt' bool
	*buffer = buf;

//	Z_Label(buf, qpath);

	FS_Read (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && com_journal && com_journal->integer == 1 ) {
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalDataFile );
		FS_Write( buf, len, com_journalDataFile );
		FS_Flush( com_journalDataFile );
	}
	return len;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer ) {
	FS_AssertInitialised();
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	Z_Free( buffer );
}

/*
============
FS_WriteFile

Filename are reletive to the quake search path
============
*/
void FS_WriteFile( const char *qpath, const void *buffer, int size ) {
	fileHandle_t f;

	FS_AssertInitialised();

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
==========================================================================

ZIP FILE LOADING

==========================================================================
*/

/*
=================
FS_LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
static pack_t *FS_LoadZipFile( const char *zipfile, const char *basename )
{
	fileInPack_t	*buildBuffer;
	pack_t			*pack;
	unzFile			uf;
	int				err;
	unz_global_info gi;
	char			filename_inzip[MAX_ZPATH];
	unz_file_info	file_info;
	int				len;
	size_t			i;
	long			hash;
	int				fs_numHeaderLongs;
	int				*fs_headerLongs;
	char			*namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo (uf,&gi);

	if (err != UNZ_OK)
		return NULL;

	len = 0;
	unzGoToFirstFile(uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = (struct fileInPack_s *)Z_Malloc( (gi.number_entry * sizeof( fileInPack_t )) + len, TAG_FILESYS, qtrue );
	namePtr = ((char *) buildBuffer) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = (int *)Z_Malloc( ( gi.number_entry + 1 ) * sizeof(int), TAG_FILESYS, qtrue );
	fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong( fs_checksumFeed );

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1) {
		if (i > gi.number_entry) {
			break;
		}
	}

	pack = (pack_t *)Z_Malloc( sizeof( pack_t ) + i * sizeof(fileInPack_t *), TAG_FILESYS, qtrue );
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t **) (((char *) pack) + sizeof( pack_t ));
	for(int j = 0; j < pack->hashSize; j++) {
		pack->hashTable[j] = NULL;
	}

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
	Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );

	// strip .pk3 if needed
	if ( strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + strlen( pack->pakBasename ) - 4, ".pk3" ) ) {
		pack->pakBasename[strlen( pack->pakBasename ) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		if (file_info.uncompressed_size > 0) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		Q_strlwr( filename_inzip );
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		strcpy( buildBuffer[i].name, filename_inzip );
		namePtr += strlen(filename_inzip) + 1;
		// store the file position in the zip
		buildBuffer[i].pos = unzGetOffset(uf);
		buildBuffer[i].len = file_info.uncompressed_size;
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum( &fs_headerLongs[ 1 ], sizeof(*fs_headerLongs) * ( fs_numHeaderLongs - 1 ) );
	pack->pure_checksum = Com_BlockChecksum( fs_headerLongs, sizeof(*fs_headerLongs) * fs_numHeaderLongs );
	pack->checksum = LittleLong( pack->checksum );
	pack->pure_checksum = LittleLong( pack->pure_checksum );

	Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

/*
=================
FS_FreePak

Frees a pak structure and releases all associated resources
=================
*/

void FS_FreePak(pack_t *thepak)
{
	unzClose(thepak->handle);
	Z_Free(thepak->buildBuffer);
	Z_Free(thepak);
}

/*
=================
FS_GetZipChecksum

Compares whether the given pak file matches a referenced checksum
=================
*/
qboolean FS_CompareZipChecksum(const char *zipfile)
{
	pack_t *thepak;
	int index, checksum;

	thepak = FS_LoadZipFile(zipfile, "");

	if(!thepak)
		return qfalse;

	checksum = thepak->checksum;
	FS_FreePak(thepak);

	for(index = 0; index < fs_numServerReferencedPaks; index++)
	{
		if(checksum == fs_serverReferencedPaks[index])
			return qtrue;
	}

	return qfalse;
}

/*
=================================================================================

DIRECTORY SCANNING FUNCTIONS

=================================================================================
*/

#define	MAX_FOUND_FILES	0x1000

static int FS_ReturnPath( const char *zname, char *zpath, int *depth ) {
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while(zname[at] != 0)
	{
		if (zname[at]=='/' || zname[at]=='\\') {
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy(zpath, zname);
	zpath[len] = 0;
	*depth = newdep;

	return len;
}

/*
==================
FS_AddFileToList
==================
*/
static int FS_AddFileToList( char *name, char *list[MAX_FOUND_FILES], int nfiles ) {
	int		i;

	if ( nfiles == MAX_FOUND_FILES - 1 ) {
		return nfiles;
	}
	for ( i = 0 ; i < nfiles ; i++ ) {
		if ( !Q_stricmp( name, list[i] ) ) {
			return nfiles;		// allready in list
		}
	}
	list[nfiles] = CopyString( name );
	nfiles++;

	return nfiles;
}

/*
===============
FS_ListFilteredFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
char **FS_ListFilteredFiles( const char *path, const char *extension, char *filter, int *numfiles ) {
	int				nfiles;
	char			**listCopy;
	char			*list[MAX_FOUND_FILES];
	searchpath_t	*search;
	int				i;
	int				pathLength;
	int				extensionLength;
	int				length, pathDepth, temp;
	pack_t			*pak;
	fileInPack_t	*buildBuffer;
	char			zpath[MAX_ZPATH];

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !path ) {
		*numfiles = 0;
		return NULL;
	}
	if ( !extension ) {
		extension = "";
	}

	pathLength = strlen( path );
	if ( path[pathLength-1] == '\\' || path[pathLength-1] == '/' ) {
		pathLength--;
	}
	extensionLength = strlen( extension );
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	//
	// search through the path, one element at a time, adding to list
	//
	for (search = fs_searchpaths ; search ; search = search->next) {
		// is the element a pak file?
		if (search->pack) {

			//ZOID:  If we are pure, don't search for files on paks that
			// aren't on the pure list
			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for (i = 0; i < pak->numfiles; i++) {
				char	*name;
				int		zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				//
				if (filter) {
					// case insensitive
					if (!Com_FilterPath( filter, name, qfalse ))
						continue;
					// unique the match
					nfiles = FS_AddFileToList( name, list, nfiles );
				}
				else {

					zpathLen = FS_ReturnPath(name, zpath, &depth);

					if ( (depth-pathDepth)>2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) ) {
						continue;
					}

					// check for extension match
					length = strlen( name );
					if ( length < extensionLength ) {
						continue;
					}

					if ( Q_stricmp( name + length - extensionLength, extension ) ) {
						continue;
					}
					// unique the match

					temp = pathLength;
					if (pathLength) {
						temp++;		// include the '/'
					}
					nfiles = FS_AddFileToList( name + temp, list, nfiles );
				}
			}
		} else if (search->dir) { // scan for files in the filesystem
			char	*netpath;
			int		numSysFiles;
			char	**sysFiles;
			char	*name;

			// don't scan directories for files if we are pure or restricted
			if ( fs_numServerPaks &&
				 (!extension || Q_stricmp(extension, "fcf")) )
			{
				//rww - allow scanning for fcf files outside of pak even if pure
			    continue;
		    }
			else
			{
				netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
				sysFiles = Sys_ListFiles( netpath, extension, filter, &numSysFiles, qfalse );
				for ( i = 0 ; i < numSysFiles ; i++ ) {
					// unique the match
					name = sysFiles[i];
					nfiles = FS_AddFileToList( name, list, nfiles );
				}
				Sys_FreeFileList( sysFiles );
			}
		}
	}

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

	return listCopy;
}

/*
=================
FS_ListFiles
=================
*/
char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	return FS_ListFilteredFiles( path, extension, NULL, numfiles );
}

/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList( char **fileList ) {
	//rwwRMG - changed to fileList to not conflict with list type
	int		i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !fileList ) {
		return;
	}

	for ( i = 0 ; fileList[i] ; i++ ) {
		Z_Free( fileList[i] );
	}

	Z_Free( fileList );
}


/*
================
FS_GetFileList
================
*/
int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	int		nFiles, i, nTotal, nLen;
	char **pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if (Q_stricmp(path, "$modlist") == 0) {
		return FS_GetModList(listbuf, bufsize);
	}

	pFiles = FS_ListFiles(path, extension, &nFiles);

	for (i =0; i < nFiles; i++) {
		nLen = strlen(pFiles[i]) + 1;
		if (nTotal + nLen + 1 < bufsize) {
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}
		else {
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList(pFiles);

	return nFiles;
}

/*
=======================
Sys_ConcatenateFileLists

mkv: Naive implementation. Concatenates three lists into a
     new list, and frees the old lists from the heap.
bk001129 - from cvs1.17 (mkv)

FIXME TTimo those two should move to common.c next to Sys_ListFiles
=======================
 */
static unsigned int Sys_CountFileList(char **fileList)
{
	int i = 0;

	if (fileList)
	{
		while (*fileList)
		{
			fileList++;
			i++;
		}
	}
	return i;
}

static char** Sys_ConcatenateFileLists( char **list0, char **list1, char **list2 )
{
	int totalLength = 0;
	char** cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);
	totalLength += Sys_CountFileList(list2);

	/* Create new list. */
	dst = cat = (char **)Z_Malloc( ( totalLength + 1 ) * sizeof( char* ), TAG_FILESYS, qtrue );

	/* Copy over lists. */
	if (list0) {
		for (src = list0; *src; src++, dst++)
			*dst = *src;
	}
	if (list1) {
		for (src = list1; *src; src++, dst++)
			*dst = *src;
	}
	if (list2) {
		for (src = list2; *src; src++, dst++)
			*dst = *src;
	}

	// Terminate the list
	*dst = NULL;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if (list0) Z_Free( list0 );
	if (list1) Z_Free( list1 );
	if (list2) Z_Free( list2 );

	return cat;
}
//#endif

// For base game mod listing
const char *SE_GetString( const char *psPackageAndStringReference );

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to base with a pk3 in it
The directories are searched in base path, cd path and home path
================
*/
int	FS_GetModList( char *listbuf, int bufsize ) {
	int		nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char **pFiles = NULL;
	char **pPaks = NULL;
	char *name, *path;
	char descPath[MAX_OSPATH];
	fileHandle_t descHandle;

	int dummy;
	char **pFiles0 = NULL;
	char **pFiles1 = NULL;
	char **pFiles2 = NULL;
	qboolean bDrop = qfalse;

	*listbuf = 0;
	nMods = nPotential = nTotal = 0;

	pFiles0 = Sys_ListFiles( fs_homepath->string, NULL, NULL, &dummy, qtrue );
	pFiles1 = Sys_ListFiles( fs_basepath->string, NULL, NULL, &dummy, qtrue );
	pFiles2 = Sys_ListFiles( fs_cdpath->string, NULL, NULL, &dummy, qtrue );
	// we searched for mods in the three paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles = Sys_ConcatenateFileLists( pFiles0, pFiles1, pFiles2 );
	nPotential = Sys_CountFileList(pFiles);

	for ( i = 0 ; i < nPotential ; i++ ) {
		name = pFiles[i];
		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if (i!=0) {
			bDrop = qfalse;
			for(j=0; j<i; j++)
			{
				if (Q_stricmp(pFiles[j],name)==0) {
					// this one can be dropped
					bDrop = qtrue;
					break;
				}
			}
		}
		if (bDrop) {
			continue;
		}
		// we drop "." and ".."
		if (Q_stricmpn(name, ".", 1)) {
			// now we need to find some .pk3 files to validate the mod
			// NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			//   so it could be in base path, cd path or home path
			//   we will try each three of them here (yes, it's a bit messy)
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
			nPaks = 0;
			pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, qfalse);
			Sys_FreeFileList( pPaks ); // we only use Sys_ListFiles to check wether .pk3 files are present

			/* Try on cd path */
			if( nPaks <= 0 ) {
				path = FS_BuildOSPath( fs_cdpath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
				Sys_FreeFileList( pPaks );
			}

			/* try on home path */
			if ( nPaks <= 0 )
			{
				path = FS_BuildOSPath( fs_homepath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
				Sys_FreeFileList( pPaks );
			}

			if (nPaks > 0) {
				bool isBase = !Q_stricmp( name, BASEGAME );
				nLen = isBase ? 1 : strlen(name) + 1;
				// nLen is the length of the mod path
				// we need to see if there is a description available
				descPath[0] = '\0';
				strcpy(descPath, name);
				strcat(descPath, "/description.txt");
				nDescLen = FS_SV_FOpenFileRead( descPath, &descHandle );
				if ( nDescLen > 0 && descHandle) {
					FILE *file;
					file = FS_FileForHandle(descHandle);
					Com_Memset( descPath, 0, sizeof( descPath ) );
					nDescLen = fread(descPath, 1, 48, file);
					if (nDescLen >= 0) {
						descPath[nDescLen] = '\0';
					}
					FS_FCloseFile(descHandle);
				} else if ( isBase ) {
					strcpy(descPath, SE_GetString("MENUS_JEDI_ACADEMY"));
				} else {
					strcpy(descPath, name);
				}
				nDescLen = strlen(descPath) + 1;

				if (nTotal + nLen + 1 + nDescLen + 1 < bufsize) {
					if ( isBase )
						strcpy(listbuf, "");
					else
						strcpy(listbuf, name);
					listbuf += nLen;
					strcpy(listbuf, descPath);
					listbuf += nDescLen;
					nTotal += nLen + nDescLen;
					nMods++;
				}
				else {
					break;
				}
			}
		}
	}
	Sys_FreeFileList( pFiles );

	return nMods;
}

//============================================================================

/*
================
FS_Dir_f
================
*/
void FS_Dir_f( void ) {
	char	*path;
	char	*extension;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 ) {
		Com_Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( Cmd_Argc() == 2 ) {
		path = Cmd_Argv( 1 );
		extension = "";
	} else {
		path = Cmd_Argv( 1 );
		extension = Cmd_Argv( 2 );
	}

	Com_Printf( "Directory of %s %s\n", path, extension );
	Com_Printf( "---------------\n" );

	dirnames = FS_ListFiles( path, extension, &ndirs );

	for ( i = 0; i < ndirs; i++ ) {
		Com_Printf( "%s\n", dirnames[i] );
	}
	FS_FreeFileList( dirnames );
}

/*
===========
FS_ConvertPath
===========
*/
void FS_ConvertPath( char *s ) {
	while (*s) {
		if ( *s == '\\' || *s == ':' ) {
			*s = '/';
		}
		s++;
	}
}

/*
===========
FS_PathCmp

Ignore case and separator char distinctions
===========
*/
int FS_PathCmp( const char *s1, const char *s2 ) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if (c1 < c2) {
			return -1;		// strings not equal
		}
		if (c1 > c2) {
			return 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
FS_SortFileList
================
*/
void FS_SortFileList(char **filelist, int numfiles) {
	int i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = (char **)Z_Malloc( ( numfiles + 1 ) * sizeof( *sortedlist ), TAG_FILESYS, qtrue );
	sortedlist[0] = NULL;
	numsortedfiles = 0;
	for (i = 0; i < numfiles; i++) {
		for (j = 0; j < numsortedfiles; j++) {
			if (FS_PathCmp(filelist[i], sortedlist[j]) < 0) {
				break;
			}
		}
		for (k = numsortedfiles; k > j; k--) {
			sortedlist[k] = sortedlist[k-1];
		}
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Com_Memcpy(filelist, sortedlist, numfiles * sizeof( *filelist ) );
	Z_Free(sortedlist);
}

/*
================
FS_NewDir_f
================
*/
void FS_NewDir_f( void ) {
	char	*filter;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "usage: fdir <filter>\n" );
		Com_Printf( "example: fdir *ffa*.bsp\n");
		return;
	}

	filter = Cmd_Argv( 1 );

	Com_Printf( "---------------\n" );

	dirnames = FS_ListFilteredFiles( "", "", filter, &ndirs );

	FS_SortFileList(dirnames, ndirs);

	for ( i = 0; i < ndirs; i++ ) {
		FS_ConvertPath(dirnames[i]);
		Com_Printf( "%s\n", dirnames[i] );
	}
	Com_Printf( "%d files listed\n", ndirs );
	FS_FreeFileList( dirnames );
}

/*
============
FS_Path_f

============
*/
void FS_Path_f( void ) {
	searchpath_t	*s;
	int				i;

	Com_Printf ("Current search path:\n");
	for (s = fs_searchpaths; s; s = s->next) {
		if (s->pack) {
			Com_Printf ("%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles);
			if ( fs_numServerPaks ) {
				if ( !FS_PakIsPure(s->pack) ) {
					Com_Printf( "    not on the pure list\n" );
				} else {
					Com_Printf( "    on the pure list\n" );
				}
			}
		} else {
			Com_Printf ("%s%c%s\n", s->dir->path, PATH_SEP, s->dir->gamedir );
		}
	}

	Com_Printf( "\n" );
	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o ) {
			Com_Printf( "handle %i: %s\n", i, fsh[i].name );
		}
	}
}

/*
============
FS_TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void FS_TouchFile_f( void ) {
	fileHandle_t	f;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: touchFile <file>\n" );
		return;
	}

	FS_FOpenFileRead( Cmd_Argv( 1 ), &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

/*
============
FS_Which_f
============
*/
void FS_Which_f( void ) {
	searchpath_t	*search;
	char		*filename;

	filename = Cmd_Argv(1);

	if ( !filename[0] ) {
		Com_Printf( "Usage: which <file>\n" );
		return;
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		return;
	}

	// just wants to see if file is there
	for ( search=fs_searchpaths; search; search=search->next ) {
		if ( search->pack ) {
			long hash = FS_HashFileName( filename, search->pack->hashSize );

			// is the element a pak file?
			if ( search->pack->hashTable[hash]) {
				// look through all the pak file elements
				pack_t* pak = search->pack;
				fileInPack_t* pakFile = pak->hashTable[hash];

				do {
					// case and separator insensitive comparisons
					if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
						// found it!
						Com_Printf( "File \"%s\" found in \"%s\"\n", filename, pak->pakFilename );
						return;
					}

					pakFile = pakFile->next;
				} while ( pakFile != NULL );
			}
		} else if (search->dir) {
			directory_t* dir = search->dir;

			char* netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
			FILE* filep = fopen(netpath, "rb");

			if ( filep ) {
				fclose( filep );

				char buf[MAX_OSPATH];
				Com_sprintf( buf, sizeof( buf ), "%s%c%s", dir->path, PATH_SEP, dir->gamedir );
				FS_ReplaceSeparators( buf );
				Com_Printf( "File \"%s\" found at \"%s\"\n", filename, buf );
				return;
			}
		}
	}

	Com_Printf( "File not found: \"%s\"\n", filename );
}

//===========================================================================

static int QDECL paksort( const void *a, const void *b ) {
	char	*aa, *bb;

	aa = *(char **)a;
	bb = *(char **)b;

	return FS_PathCmp( aa, bb );
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads the zip headers
================
*/
#define	MAX_PAKFILES	1024
static void FS_AddGameDirectory( const char *path, const char *dir ) {
	searchpath_t	*sp;
	int				i;
	searchpath_t	*search;
	searchpath_t	*thedir;
	pack_t			*pak;
	char			curpath[MAX_OSPATH + 1], *pakfile;
	int				numfiles;
	char			**pakfiles;
	char			*sorted[MAX_PAKFILES];

	// this fixes the case where fs_basepath is the same as fs_cdpath
	// which happens on full installs
	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->dir && Sys_PathCmp(sp->dir->path, path) && !Q_stricmp(sp->dir->gamedir, dir)) {
			return;			// we've already got this one
		}
	}

	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	// find all pak files in this directory
	Q_strncpyz(curpath, FS_BuildOSPath(path, dir, ""), sizeof(curpath));
	curpath[strlen(curpath) - 1] = '\0';	// strip the trailing slash

	//
	// add the directory to the search path
	//
	search = (struct searchpath_s *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue);
	search->dir = (directory_t *)Z_Malloc( sizeof( *search->dir ), TAG_FILESYS, qtrue );

	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->fullpath, curpath, sizeof( search->dir->fullpath ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	thedir = search;

	pakfiles = Sys_ListFiles( curpath, ".pk3", NULL, &numfiles, qfalse );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override pak0.pk3
	if ( numfiles > MAX_PAKFILES ) {
		numfiles = MAX_PAKFILES;
	}
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];
	}

	qsort( sorted, numfiles, sizeof(char*), paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {
		pakfile = FS_BuildOSPath( path, dir, sorted[i] );
		if ( ( pak = FS_LoadZipFile( pakfile, sorted[i] ) ) == 0 )
			continue;
		Q_strncpyz(pak->pakPathname, curpath, sizeof(pak->pakPathname));
		// store the game name for downloading
		Q_strncpyz(pak->pakGamename, dir, sizeof(pak->pakGamename));

		fs_packFiles += pak->numfiles;

		search = (searchpath_s *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue);
		search->pack = pak;

		if (fs_dirbeforepak && fs_dirbeforepak->integer && thedir)
		{
			searchpath_t *oldnext = thedir->next;
			thedir->next = search;

			while (oldnext)
			{
				search->next = oldnext;
				search = search->next;
				oldnext = oldnext->next;
			}
		}
		else
		{
			search->next = fs_searchpaths;
			fs_searchpaths = search;
		}
	}

	// done
	Sys_FreeFileList( pakfiles );
}

/*
================
FS_idPak
================
*/
qboolean FS_idPak( char *pak, char *base ) {
	int i;

	for (i = 0; i < NUM_ID_PAKS; i++) {
		if ( !FS_FilenameCompare(pak, va("%s/assets%d", base, i)) ) {
			break;
		}
	}
	if (i < NUM_ID_PAKS) {
		return qtrue;
	}
	return qfalse;
}

/*
================
FS_CheckDirTraversal

Check whether the string contains stuff like "../" to prevent directory traversal bugs
and return qtrue if it does.
================
*/

qboolean FS_CheckDirTraversal(const char *checkdir)
{
	if(strstr(checkdir, "../") || strstr(checkdir, "..\\"))
		return qtrue;

	return qfalse;
}

/*
================
FS_ComparePaks

if dlstring == qtrue

Returns a list of pak files that we should download from the server. They all get stored
in the current gamedir and an FS_Restart will be fired up after we download them all.

The string is the format:

@remotename@localname [repeat]

static int		fs_numServerReferencedPaks;
static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
static char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];

----------------
dlstring == qfalse

we are not interested in a download string format, we want something human-readable
(this is used for diagnostics while connecting to a pure server)
================
*/
qboolean FS_ComparePaks( char *neededpaks, int len, qboolean dlstring ) {
	searchpath_t	*sp;
	qboolean havepak;
	char *origpos = neededpaks;
	int i;

	if ( !fs_numServerReferencedPaks ) {
		return qfalse; // Server didn't send any pack information along
	}

	*neededpaks = 0;

	for ( i = 0 ; i < fs_numServerReferencedPaks ; i++ ) {
		// Ok, see if we have this pak file
		havepak = qfalse;

		// never autodownload any of the id paks
		if ( FS_idPak(fs_serverReferencedPakNames[i], "base") || FS_idPak(fs_serverReferencedPakNames[i], "missionpack") ) {
			continue;
		}

		// Make sure the server cannot make us write to non-quake3 directories.
		if(FS_CheckDirTraversal(fs_serverReferencedPakNames[i]))
		{
			Com_Printf("WARNING: Invalid download name %s\n", fs_serverReferencedPakNames[i]);
			continue;
		}

		for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
			if ( sp->pack && sp->pack->checksum == fs_serverReferencedPaks[i] ) {
				havepak = qtrue; // This is it!
				break;
			}
		}

		if ( !havepak && fs_serverReferencedPakNames[i] && *fs_serverReferencedPakNames[i] ) {
			// Don't got it

			if (dlstring)
			{
				// We need this to make sure we won't hit the end of the buffer or the server could
				// overwrite non-pk3 files on clients by writing so much crap into neededpaks that
				// Q_strcat cuts off the .pk3 extension.

				origpos += strlen(origpos);

				// Remote name
				Q_strcat( neededpaks, len, "@");
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
				Q_strcat( neededpaks, len, ".pk3" );

				// Local name
				Q_strcat( neededpaks, len, "@");
				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) ) {
					char st[MAX_ZPATH];
					// We already have one called this, we need to download it to another name
					// Make something up with the checksum in it
					Com_sprintf( st, sizeof( st ), "%s.%08x.pk3", fs_serverReferencedPakNames[i], fs_serverReferencedPaks[i] );
					Q_strcat( neededpaks, len, st );
				} else {
					Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
					Q_strcat( neededpaks, len, ".pk3" );
				}

				// Find out whether it might have overflowed the buffer and don't add this file to the
				// list if that is the case.
				if(strlen(origpos) + (origpos - neededpaks) >= (unsigned)(len - 1))
				{
					*origpos = '\0';
					break;
				}
			}
			else
			{
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
				Q_strcat( neededpaks, len, ".pk3" );
				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) )
				{
					Q_strcat( neededpaks, len, " (local file exists with wrong checksum)");
				}
				Q_strcat( neededpaks, len, "\n");
			}
		}
	}
	if ( *neededpaks ) {
		return qtrue;
	}

	return qfalse; // We have them all
}

/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( qboolean closemfp ) {
	searchpath_t	*p, *next;
	int	i;

#if defined(_WIN32)
	// Delete temporary files
	fs_temporaryFileWriteIdx = 0;
	for ( size_t i = 0; i < ARRAY_LEN(fs_temporaryFileNames); i++ )
	{
		if (fs_temporaryFileNames[i][0] != '\0')
		{
			if ( !DeleteFile(fs_temporaryFileNames[i]) )
			{
				DWORD error = GetLastError();
				Com_DPrintf("FS_Shutdown: failed to delete '%s'. "
							"Win32 error code: 0x08x",
							fs_temporaryFileNames[i],
							error);
			}

			fs_temporaryFileNames[i][0] = '\0';
		}
	}
#endif

	for(i = 0; i < MAX_FILE_HANDLES; i++) {
		if (fsh[i].fileSize) {
			FS_FCloseFile(i);
		}
	}

	// free everything
	for ( p = fs_searchpaths ; p ; p = next ) {
		next = p->next;

		if ( p->pack ) {
			FS_FreePak( p->pack );
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

#ifdef FS_MISSING
	if (closemfp) {
		fclose(missingFiles);
	}
#endif
}

//rww - add search paths in for received svc_setgame
//Ensiform - this is so wrong rww
void FS_UpdateGamedir(void)
{
	if ( fs_gamedirvar->string[0] && Q_stricmp( fs_gamedirvar->string, BASEGAME ) )
	{
		if (fs_cdpath->string[0])
		{
			FS_AddGameDirectory(fs_cdpath->string, fs_gamedirvar->string);
		}
		if (fs_basepath->string[0])
		{
			FS_AddGameDirectory(fs_basepath->string, fs_gamedirvar->string);
		}
		if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string))
		{
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string);
		}
	}
}

/*
================
FS_ReorderPurePaks
NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
  this can lead to misleading situations, see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
================
*/
static void FS_ReorderPurePaks()
{
	searchpath_t *s;
	int i;
	searchpath_t **p_insert_index, // for linked list reordering
		**p_previous; // when doing the scan

	// only relevant when connected to pure server
	if ( !fs_numServerPaks )
		return;

	fs_reordered = qfalse;

	p_insert_index = &fs_searchpaths; // we insert in order at the beginning of the list
	for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
		p_previous = p_insert_index; // track the pointer-to-current-item
		for (s = *p_insert_index; s; s = s->next) {
			// the part of the list before p_insert_index has been sorted already
			if (s->pack && fs_serverPaks[i] == s->pack->checksum) {
				fs_reordered = qtrue;
				// move this element to the insert list
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				// increment insert list
				p_insert_index = &s->next;
				break; // iterate to next server pack
			}
			p_previous = &s->next;
		}
	}
}

/**
	@brief Mount the asset archives (.pk3) and register commands.

	Mounts in this order the archives from:

	1.  <fs_cdpath>/<gameName>/
	2.  <fs_basepath>/<gameName>/
	3.  <fs_apppath>/<gameName>/ (Mac Only)
	4.  <fs_homepath>/<gameName>/
	5.  <fs_cdpath>/<fs_basegame>/
	6.  <fs_basepath>/<fs_basegame>/
	7.  <fs_homepath>/<fs_basegame>/
	8.  <fs_cdpath>/<fs_game>/
	9.  <fs_basepath>/<fs_game>/
	10. <fs_homepath>/<fs_game>/

	@param gameName Name of the default folder (i.e. always BASEGAME = "base" in OpenJK)
*/
void FS_Startup( const char *gameName ) {
	const char *homePath;

	Com_Printf( "----- FS_Startup -----\n" );

	fs_packFiles = 0;

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get ("fs_cdpath", "", CVAR_INIT|CVAR_PROTECTED, "(Read Only) Location for development files" );
	fs_basepath = Cvar_Get ("fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT|CVAR_PROTECTED, "(Read Only) Location for game files" );
	fs_basegame = Cvar_Get ("fs_basegame", "", CVAR_INIT );
	homePath = Sys_DefaultHomePath();
	if (!homePath || !homePath[0]) {
		homePath = fs_basepath->string;
	}
	fs_homepath = Cvar_Get ("fs_homepath", homePath, CVAR_INIT|CVAR_PROTECTED, "(Read/Write) Location for user generated files" );
	fs_gamedirvar = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO, "Mod directory" );

	fs_dirbeforepak = Cvar_Get("fs_dirbeforepak", "0", CVAR_INIT|CVAR_PROTECTED, "Prioritize directories before paks if not pure" );

	// add search path elements in reverse priority order (lowest priority first)
	if (fs_cdpath->string[0]) {
		FS_AddGameDirectory( fs_cdpath->string, gameName );
	}
	if (fs_basepath->string[0]) {
		FS_AddGameDirectory( fs_basepath->string, gameName );
	}

#ifdef MACOS_X
	fs_apppath = Cvar_Get ("fs_apppath", Sys_DefaultAppPath(), CVAR_INIT|CVAR_PROTECTED, "(Read Only) Location of OSX .app bundle" );
	// Make MacOSX also include the base path included with the .app bundle
	if (fs_apppath->string[0]) {
		FS_AddGameDirectory( fs_apppath->string, gameName );
	}
#endif

	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
		FS_CreatePath ( fs_homepath->string );
		FS_AddGameDirectory ( fs_homepath->string, gameName );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && Q_stricmp( fs_basegame->string, gameName ) ) {
		if (fs_cdpath->string[0]) {
			FS_AddGameDirectory(fs_cdpath->string, fs_basegame->string);
		}
		if (fs_basepath->string[0]) {
			FS_AddGameDirectory(fs_basepath->string, fs_basegame->string);
		}
		if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
			FS_AddGameDirectory(fs_homepath->string, fs_basegame->string);
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && Q_stricmp( fs_gamedirvar->string, gameName ) ) {
		if (fs_cdpath->string[0]) {
			FS_AddGameDirectory(fs_cdpath->string, fs_gamedirvar->string);
		}
		if (fs_basepath->string[0]) {
			FS_AddGameDirectory(fs_basepath->string, fs_gamedirvar->string);
		}
		if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string);
		}
	}

	// add our commands
	Cmd_AddCommand ("path", FS_Path_f, "Lists search paths" );
	Cmd_AddCommand ("dir", FS_Dir_f, "Lists a folder" );
	Cmd_AddCommand ("fdir", FS_NewDir_f, "Lists a folder with filters" );
	Cmd_AddCommand ("touchFile", FS_TouchFile_f, "Touches a file" );
	Cmd_AddCommand ("which", FS_Which_f, "Determines which search path a file was loaded from" );

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=506
	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );

#ifdef FS_MISSING
	if (missingFiles == NULL) {
		missingFiles = fopen( "\\missing.txt", "ab" );
	}
#endif
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/*
=====================
FS_LoadedPakChecksums

Returns a space separated string containing the checksums of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( !search->pack ) {
			continue;
		}

		Q_strcat( info, sizeof( info ), va("%i ", search->pack->checksum ) );
	}

	return info;
}

/*
=====================
FS_LoadedPakNames

Returns a space separated string containing the names of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakNames( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( !search->pack ) {
			continue;
		}

		if (*info) {
			Q_strcat(info, sizeof( info ), " " );
		}
		Q_strcat( info, sizeof( info ), search->pack->pakBasename );
	}

	return info;
}

/*
=====================
FS_LoadedPakPureChecksums

Returns a space separated string containing the pure checksums of all loaded pk3 files.
Servers with sv_pure use these checksums to compare with the checksums the clients send
back to the server.
=====================
*/
const char *FS_LoadedPakPureChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( !search->pack ) {
			continue;
		}

		Q_strcat( info, sizeof( info ), va("%i ", search->pack->pure_checksum ) );
	}

	return info;
}

/*
=====================
FS_ReferencedPakChecksums

Returns a space separated string containing the checksums of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
const char *FS_ReferencedPakChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;


	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack ) {
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME))) {
				Q_strcat( info, sizeof( info ), va("%i ", search->pack->checksum ) );
			}
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakPureChecksums

Returns a space separated string containing the pure checksums of all referenced pk3 files.
Servers with sv_pure set will get this string back from clients for pure validation

The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
=====================
*/
const char *FS_ReferencedPakPureChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;
	int nFlags, numPaks, checksum;

	info[0] = 0;

	checksum = fs_checksumFeed;
	numPaks = 0;
	for (nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1) {
		if (nFlags & FS_GENERAL_REF) {
			// add a delimter between must haves and general refs
			//Q_strcat(info, sizeof(info), "@ ");
			info[strlen(info)+1] = '\0';
			info[strlen(info)+2] = '\0';
			info[strlen(info)] = '@';
			info[strlen(info)] = ' ';
		}
		for ( search = fs_searchpaths ; search ; search = search->next ) {
			// is the element a pak file and has it been referenced based on flag?
			if ( search->pack && (search->pack->referenced & nFlags)) {
				Q_strcat( info, sizeof( info ), va("%i ", search->pack->pure_checksum ) );
				if (nFlags & (FS_CGAME_REF | FS_UI_REF)) {
					break;
				}
				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
		}
		if (fs_fakeChkSum != 0) {
			// only added if a non-pure file is referenced
			Q_strcat( info, sizeof( info ), va("%i ", fs_fakeChkSum ) );
		}
	}
	// last checksum is the encoded number of referenced pk3s
	checksum ^= numPaks;
	Q_strcat( info, sizeof( info ), va("%i ", checksum ) );

	return info;
}

/*
=====================
FS_ReferencedPakNames

Returns a space separated string containing the names of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
const char *FS_ReferencedPakNames( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	// we want to return ALL pk3's from the fs_game path
	// and referenced one's from base
	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack ) {
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME))) {
				if (*info) {
					Q_strcat(info, sizeof( info ), " " );
				}
				Q_strcat( info, sizeof( info ), search->pack->pakGamename );
				Q_strcat( info, sizeof( info ), "/" );
				Q_strcat( info, sizeof( info ), search->pack->pakBasename );
			}
		}
	}

	return info;
}

/*
=====================
FS_ClearPakReferences
=====================
*/
void FS_ClearPakReferences( int flags ) {
	searchpath_t *search;

	if ( !flags ) {
		flags = -1;
	}
	for ( search = fs_searchpaths; search; search = search->next ) {
		// is the element a pak file and has it been referenced?
		if ( search->pack ) {
			search->pack->referenced &= ~flags;
		}
	}
}


/*
=====================
FS_PureServerSetLoadedPaks

If the string is empty, all data sources will be allowed.
If not empty, only pk3 files that match one of the space
separated checksums will be checked for files, with the
exception of .cfg and .dat files.
=====================
*/
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames ) {
	int		i, c, d;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();
	if ( c > MAX_SEARCH_PATHS ) {
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerPaks = c;

	for ( i = 0 ; i < c ; i++ ) {
		fs_serverPaks[i] = atoi( Cmd_Argv( i ) );
	}

	if (fs_numServerPaks) {
		Com_DPrintf( "Connected to a pure server.\n" );
	}
	else
	{
		if (fs_reordered)
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
			// force a restart to make sure the search order will be correct
			Com_DPrintf( "FS search reorder is required\n" );
			FS_Restart(fs_checksumFeed);
			return;
		}
	}

	for ( i = 0 ; i < c ; i++ ) {
		if (fs_serverPakNames[i]) {
			Z_Free(fs_serverPakNames[i]);
		}
		fs_serverPakNames[i] = NULL;
	}
	if ( pakNames && *pakNames ) {
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();
		if ( d > MAX_SEARCH_PATHS ) {
			d = MAX_SEARCH_PATHS;
		}

		for ( i = 0 ; i < d ; i++ ) {
			fs_serverPakNames[i] = CopyString( Cmd_Argv( i ) );
		}
	}
}

/*
=====================
FS_PureServerSetReferencedPaks

The checksums and names of the pk3 files referenced at the server
are sent to the client and stored here. The client will use these
checksums to see if any pk3 files need to be auto-downloaded.
=====================
*/
void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames ) {
	int		i, c, d = 0;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();
	if ( c > MAX_SEARCH_PATHS ) {
		c = MAX_SEARCH_PATHS;
	}

	for ( i = 0 ; i < c ; i++ ) {
		fs_serverReferencedPaks[i] = atoi( Cmd_Argv( i ) );
	}

	for (i = 0 ; i < (int)ARRAY_LEN(fs_serverReferencedPakNames); i++)
	{
		if(fs_serverReferencedPakNames[i])
			Z_Free(fs_serverReferencedPakNames[i]);

		fs_serverReferencedPakNames[i] = NULL;
	}

	if ( pakNames && *pakNames ) {
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();
		if ( d > c ) {
			d = c;
		}

		for ( i = 0 ; i < d ; i++ ) {
			fs_serverReferencedPakNames[i] = CopyString( Cmd_Argv( i ) );
		}
	}

	// ensure that there are as many checksums as there are pak names.
	if(d < c)
		c = d;

	fs_numServerReferencedPaks = c;
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

	if(!FS_FilenameCompare(Cvar_VariableString("fs_game"), BASEGAME))
		Cvar_Set("fs_game", "");

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "mpdefault.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load mpdefault.cfg" );
		// bk001208 - SafeMode see below, FIXME?
	}

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

#if defined(_WIN32)
	Com_Memset(fs_temporaryFileNames, 0, sizeof(fs_temporaryFileNames));
#endif

  // bk001208 - SafeMode see below, FIXME?
}

/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed ) {

	// free anything we currently have loaded
	FS_Shutdown(qfalse);

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences(0);

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "mpdefault.cfg", NULL ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if (lastValidBase[0]) {
			FS_PureServerSetLoadedPaks("", "");
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_game", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart(checksumFeed);
			Com_Error( ERR_DROP, "Invalid game folder\n" );
			return;
		}
		Com_Error( ERR_FATAL, "Couldn't load mpdefault.cfg" );
	}

	if ( Q_stricmp(fs_gamedirvar->string, lastValidGame) ) {
		// skip the jampconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText ("exec " Q3CONFIG_CFG "\n");
		}
	}

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

}

/*
=================
FS_ConditionalRestart
restart if necessary
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		return qtrue;
	}
#if 0
	if(fs_gamedirvar->modified)
	{
		if(FS_FilenameCompare(lastValidGame, fs_gamedirvar->string) &&
				(*lastValidGame || FS_FilenameCompare(fs_gamedirvar->string, BASEGAME)) &&
				(*fs_gamedirvar->string || FS_FilenameCompare(lastValidGame, BASEGAME)))
		{
			FS_Restart(checksumFeed);
			//Cvar_Restart(qtrue);
			return qtrue;
		}
		else
			fs_gamedirvar->modified = qfalse;
	}

	if(checksumFeed != fs_checksumFeed)
		FS_Restart(checksumFeed);
	else if(fs_numServerPaks && !fs_reordered)
		FS_ReorderPurePaks();
#endif
	return qfalse;
}

/*
========================================================================================

Handle based file calls for virtual machines

========================================================================================
*/

int		FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	int		r;
	qboolean	sync;

	sync = qfalse;

	switch( mode ) {
	case FS_READ:
		r = FS_FOpenFileRead( qpath, f, qtrue );
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite( qpath );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = FS_FOpenFileAppend( qpath );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
	default:
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}

	if (!f) {
		return r;
	}

	if ( *f ) {
		fsh[*f].fileSize = r;
	}
	fsh[*f].handleSync = sync;

	return r;
}

int		FS_FTell( fileHandle_t f ) {
	int pos;
	if (fsh[f].zipFile == qtrue) {
		pos = unztell(fsh[f].handleFiles.file.z);
	} else {
		pos = ftell(fsh[f].handleFiles.file.o);
	}
	return pos;
}

void	FS_Flush( fileHandle_t f ) {
	fflush(fsh[f].handleFiles.file.o);
}

void FS_FilenameCompletion( const char *dir, const char *ext, qboolean stripExt, callbackFunc_t callback, qboolean allowNonPureFilesOnDisk ) {
	int nfiles;
	char **filenames, filename[MAX_STRING_CHARS];

	filenames = FS_ListFilteredFiles( dir, ext, NULL, &nfiles );

	FS_SortFileList( filenames, nfiles );

	// pass all the files to callback (FindMatches)
	for ( int i=0; i<nfiles; i++ ) {
		FS_ConvertPath( filenames[i] );
		Q_strncpyz( filename, filenames[i], MAX_STRING_CHARS );

		if ( stripExt )
			COM_StripExtension( filename, filename, sizeof( filename ) );

		callback( filename );
	}
	FS_FreeFileList( filenames );
}

const char *FS_GetCurrentGameDir(bool emptybase)
{
	if(fs_gamedirvar->string[0])
		return fs_gamedirvar->string;

	return emptybase ? "" : BASEGAME;
}

#ifdef MACOS_X
bool FS_LoadMachOBundle( const char *name )
{
	int     len;
	void    *data;
	fileHandle_t    f;
	char    *fn;
	unzFile dll;
	byte* buf;
	char    dllName[MAX_QPATH];
	char    *tempName;
	unz_file_info   zfi;

	//read zipped bundle from pk3
	len = FS_ReadFile(name, &data);

	if (len < 1) {
		return false;
	}

	//write temporary file of zipped bundle to e.g. uixxxxxx
	//unique filename to avoid any clashes
	Com_sprintf( dllName, sizeof(dllName), "%sXXXXXX", name );

	tempName = mktemp( dllName );

	f = FS_FOpenFileWrite( dllName );

	if ( !f )
	{
		FS_FreeFile(data);
		return false;
	}

	if (FS_Write( data, len, f ) < len)
	{
		FS_FreeFile(data);
		return false;
	}

	FS_FCloseFile( f );
	FS_FreeFile(data);

	//unzOpen zipped bundle, find the dylib, and try to write it
	fn = FS_BuildOSPath( fs_homepath->string, fs_gamedir, dllName );

	dll = unzOpen( fn );

	Com_sprintf (dllName, sizeof(dllName), "%s.bundle/Contents/MacOS/%s", name, name);

	if (unzLocateFile(dll, dllName, 0) != UNZ_OK)
	{
		unzClose(dll);
		remove( fn );
		return false;
	}

	unzOpenCurrentFile( dll );

	Com_sprintf( dllName, sizeof(dllName), "%s_pk3" DLL_EXT, name );

	f = FS_FOpenFileWrite( dllName, qfalse );

	if ( !f )
	{
		unzCloseCurrentFile( dll );
		unzClose( dll );
		remove( fn );
		return false;
	}

	unzGetCurrentFileInfo( dll, &zfi, NULL, 0, NULL, 0, NULL, 0 );

	len = zfi.uncompressed_size;

	buf = (byte*)Z_Malloc( len+1, TAG_FILESYS, qfalse);

	if (unzReadCurrentFile( dll, buf, len ) < len)
	{
		FS_FCloseFile( f );
		unzCloseCurrentFile( dll );
		unzClose( dll );
		return false;
	}

	if (FS_Write(buf, len, f) < len)
	{
		FS_FCloseFile( f );
		unzCloseCurrentFile( dll );
		unzClose( dll );
		return false;
	}

	FS_FCloseFile( f );
	unzCloseCurrentFile( dll );
	unzClose( dll );
	Z_Free( buf );

	//remove temporary zipped bundle
	remove( fn );

	return true;
}
#endif

qboolean FS_WriteToTemporaryFile( const void *data, size_t dataLength, char **tempFilePath )
{
#if defined(_WIN32)
	DWORD error;
	TCHAR tempPath[MAX_PATH];
	DWORD tempPathResult = GetTempPath(MAX_PATH, tempPath);
	if ( tempPathResult )
	{
		TCHAR tempFileName[MAX_PATH];
		UINT tempFileNameResult = GetTempFileName(tempPath, "OJK", 0, tempFileName);
		if ( tempFileNameResult )
		{
			HANDLE file = CreateFile(
				tempFileName, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if ( file != INVALID_HANDLE_VALUE )
			{
				DWORD bytesWritten = 0;
				if (WriteFile(file, data, dataLength, &bytesWritten, NULL))
				{
					int deletesRemaining = ARRAY_LEN(fs_temporaryFileNames);

					CloseHandle(file);

					while ( deletesRemaining > 0 &&
							fs_temporaryFileNames[fs_temporaryFileWriteIdx][0] != '\0' )
					{
						// Delete old temporary file as we need to
						if ( DeleteFile(fs_temporaryFileNames[fs_temporaryFileWriteIdx]) )
						{
							break;
						}

						error = GetLastError();
						Com_DPrintf("FS_WriteToTemporaryFile failed for '%s'. "
									"Win32 error code: 0x%08x\n",
									fs_temporaryFileNames[fs_temporaryFileWriteIdx],
									error);

						// Failed to delete, possibly because DLL was still in use. This can
						// happen when running a listen server and you continually restart
						// the map. The game DLL is reloaded, but cgame and ui DLLs are not.
						fs_temporaryFileWriteIdx =
							(fs_temporaryFileWriteIdx + 1) % ARRAY_LEN(fs_temporaryFileNames);
						deletesRemaining--;
					}

					// If this happened, then all slots are used and we some how have 8 DLLs
					// loaded at once?!
					assert(deletesRemaining > 0);

					Q_strncpyz(fs_temporaryFileNames[fs_temporaryFileWriteIdx],
								tempFileName, sizeof(fs_temporaryFileNames[0]));
					fs_temporaryFileWriteIdx =
						(fs_temporaryFileWriteIdx + 1) % ARRAY_LEN(fs_temporaryFileNames);

					if ( tempFilePath )
					{
						size_t fileNameLen = strlen(tempFileName);
						*tempFilePath = (char *)Z_Malloc(fileNameLen + 1, TAG_FILESYS);
						Q_strncpyz(*tempFilePath, tempFileName, fileNameLen + 1);
					}

					return qtrue;
				}
				else
				{
					error = GetLastError();
					Com_DPrintf("FS_WriteToTemporaryFile failed to write '%s'. "
								"Win32 error code: 0x%08x\n",
								tempFileName, error);
				}
			}
			else
			{
				error = GetLastError();
				Com_DPrintf("FS_WriteToTemporaryFile failed to create '%s'. "
							"Win32 error code: 0x%08x\n",
							tempFileName, error);
			}
		}
		else
		{
			error = GetLastError();
			Com_DPrintf("FS_WriteToTemporaryFile failed to generate temporary file name. "
						"Win32 error code: 0x%08x\n", error);
		}
	}
	else
	{
		error = GetLastError();
		Com_DPrintf("FS_WriteToTemporaryFile failed to get temporary file folder. "
						"Win32 error code: 0x%08x\n", error);
	}
#endif

	return qfalse;
}
