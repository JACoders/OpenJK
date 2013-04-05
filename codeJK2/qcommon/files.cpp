/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena 
 *
 * $Archive: /StarTrek/Code-Single/qcommon/files.cpp $
 *
 *****************************************************************************/


#include "../game/q_shared.h"
#include "qcommon.h"
#include "unzip.h"

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
verified to not have been modified.  A single exception is made for jk2config.cfg.  Files
can still be written out in restricted mode, so screenshots and demos are allowed.
Restricted mode can be tested by setting "+set fs_restrict 1" on the command line, even
if there is a valid product.txt under the basepath or cdpath.

If not running in restricted mode, and a file is not found in any local filesystem,
an attempt will be made to download it and save it under the base path.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the cd
path, it will be copied over to the base path.  This is a development aid to help build
test releases and to copy working sets over slow network links.


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

When building a pak file, make sure a jk2config.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================

*/

#define	BASEGAME			"base"
#define	DEMOGAME			"demo"

// every time a new demo pk3 file is built, this checksum must be updated.
// the easiest way to get it is to just run the game and see what it spits out
#define	DEMO_PAK_CHECKSUM	1431467275
#define	DEMO_PAK_MAXFILES	5174u

// if this is defined, the executable positively won't work with any paks other
// than the demo pak, even if productid is present.  This is only used for our
// last demo release to prevent the mac and linux users from using the demo
// executable with the production windows pak before the mac/linux products
// hit the shelves a little later
//#define	PRE_RELEASE_DEMO


#define MAX_ZPATH			256
#define	MAX_SEARCH_PATHS	2048
#define MAX_FILEHASH_SIZE	1024

typedef struct fileInPack_s {
	char					*name;		// name of the file
	unsigned long			pos;		// file info position in zip
	struct	fileInPack_s*	next;		// next file in the hash
} fileInPack_t;

typedef struct {
	char			pakFilename[MAX_OSPATH];	// c:\quake3\base\asset0.pk3
	unzFile			handle;
	int				checksum;
	int				numfiles;
	int				hashSize;					// hash table size (power of 2)
	fileInPack_t*	*hashTable;					// hash table
	fileInPack_t*	buildBuffer;				// buffer with the filenames etc.
} pack_t;


typedef struct {
	char		path[MAX_OSPATH];		// c:\stvoy
	char		gamedir[MAX_OSPATH];	// base
} directory_t;

typedef struct searchpath_s {
	struct searchpath_s *next;

	pack_t		*pack;		// only one of pack / dir will be non NULL
	directory_t	*dir;
} searchpath_t;

static	char		fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
static	cvar_t		*fs_debug;
static	cvar_t		*fs_basepath;
static	cvar_t		*fs_cdpath;
static	cvar_t		*fs_copyfiles;
static	cvar_t		*fs_gamedirvar;
static	cvar_t		*fs_restrict;
static	searchpath_t	*fs_searchpaths;
static	int			fs_readCount;			// total bytes read
static	int			fs_loadCount;			// total files read
static	int			fs_packFiles;			// total number of files in packs

#define	MAX_FILE_HANDLES	16

typedef union qfile_gus {
	FILE*		o;
	unzFile		z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut	file;
	qboolean	unique;
} qfile_ut;

typedef struct {
	qfile_ut	handleFiles;
	qboolean	handleSync;
	int			baseOffset;
	int			fileSize;
	int			zipFilePos;
	qboolean	zipFile;
	char		name[MAX_QPATH];
} fileHandleData_t;

fileHandleData_t	fsh[MAX_FILE_HANDLES];


//static int		fs_numServerPaks;
//static int		fs_serverPaks[MAX_SEARCH_PATHS];

// productId: This file is copyright 2000 Raven Software, and may not be duplicated except during a licensed installation of the full commercial version of Star Wars: Jedi Outcast
static byte fs_scrambledProductId[165] = {
 42, 143, 149, 190,  10, 197, 225, 133, 243,  63, 189, 182, 226,  56, 143,  17, 215,  37, 197, 218,  50, 103,  24, 235, 246, 191, 180, 149, 160, 170, 230,
 52, 176, 231,  15, 194, 236, 247, 159, 168, 132, 154,  24, 133,  67,  85,  36,  97,  99,  86, 117, 189, 212, 156, 236, 153,  68,  10, 196, 241,  39,
219, 156,  88,  93, 198, 200, 232, 142,  67,  45, 209,  53, 186, 228, 241, 162, 127, 213,  83,   7, 121,  11,  93, 123, 243, 148, 240, 229,  42,  42,
  6, 215, 239, 112, 120, 240, 244, 104,  12,  38,  47, 201, 253, 223, 208, 154,  69, 141, 157,  32, 117, 166, 146, 236,  59,  15, 223,  52,  89, 133,
 64, 201,  56, 119,  25, 211, 152, 159,  11,  92,  59, 207,  81, 123,   0, 121, 241, 116,  42,  36, 251,  51, 149,  79, 165,  12, 106, 187, 225, 203,
 99, 102,  69,  97,  81,  27, 107,  95, 164,  42,  36, 189,  94, 126
};

/*
==============
FS_Initialized
==============
*/
qboolean FS_Initialized() {
	return (qboolean)(fs_searchpaths != NULL);
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
		if (letter == PATH_SEP) letter = '/';		// damn path names	//mac and unix are different
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}

static fileHandle_t	FS_HandleForFile(void) {
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

static FILE	*FS_FileForHandle( fileHandle_t f ) {
	if ( f < 0 || f > MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of reange" );
	}
	if (fsh[f].zipFile == (int)qtrue) {
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
FS_filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelength( fileHandle_t f ) {
	int		pos;
	int		end;
	FILE*	h;

	h = FS_FileForHandle(f);
	pos = ftell (h);
	fseek (h, 0, SEEK_END);
	end = ftell (h);
	fseek (h, pos, SEEK_SET);

	return end;
}


/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
static void FS_ReplaceSeparators( char *path ) {
	char	*s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = PATH_SEP;
		}
	}
}

/*
===================
FS_BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/
char *FS_BuildOSPath( const char *base, const char *game, const char *qpath ) {
	char	temp[MAX_OSPATH];
	static char ospath[2][MAX_OSPATH];
	static int toggle;
	
	toggle ^= 1;		// flip-flop to allow two returns without clash

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
static void	FS_CreatePath (char *OSPath) {
	char	*ofs;
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return;
	}

	strlwr(OSPath);

	for (ofs = OSPath+1 ; *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir (OSPath);
			*ofs = PATH_SEP;
		}
	}
}

/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
// added extra param so behind-the-scenes copying in savegames doesn't clutter up the screen -slc
static qboolean FS_CopyFile( char *fromOSPath, char *toOSPath, qboolean qbSilent = qfalse );
static qboolean FS_CopyFile( char *fromOSPath, char *toOSPath, qboolean qbSilent ) {
	FILE	*f;
	int		len;
	byte	*buf;

	if (!qbSilent)
	{
		Com_Printf( "copy %s to %s\n", fromOSPath, toOSPath );
	}
	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return qfalse;
	}
	fseek (f, 0, SEEK_END);
	len = ftell (f);
	fseek (f, 0, SEEK_SET);
	
	buf = (unsigned char *) Z_Malloc( len, TAG_FILESYS, qfalse);
	if (fread( buf, 1, len, f ) != len)
	{
		Z_Free( buf );
		fclose(f);
		if (qbSilent){
			return qfalse;
		}
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	FS_CreatePath( toOSPath );
	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		Z_Free( buf );
		return qfalse;
	}
	if (fwrite( buf, 1, len, f ) != len)
	{
		Z_Free( buf );
		fclose(f);
		if (qbSilent){
			return qfalse;
		}
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	Z_Free( buf );

	return qtrue;
}

/*
==============
FS_FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other dll's can't just call fclose()
on files returned by FS_FOpenFile...
==============
*/
void FS_FCloseFile( fileHandle_t f ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if (fsh[f].zipFile == qtrue) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	fclose (fsh[f].handleFiles.file.o);
	memset( &fsh[f], 0, sizeof( fsh[f] ) );
}




// The following functions with "UserGen" in them were added for savegame handling, 
//	since outside functions aren't supposed to know about full paths/dirs

// "filename" is local to the current gamedir (eg "saves/blah.sav")
//
void FS_DeleteUserGenFile( const char *filename )
{
	char		*ospath;

	if ( !fs_searchpaths ) 
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	ospath = FS_BuildOSPath( fs_basepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) 
	{
		Com_Printf( "FS_DeleteUserGenFile: %s\n", ospath );
	}

	remove ( ospath );
}

// filenames are local (eg "saves/blah.sav")
//
// return: qtrue = OK
//
qboolean FS_CopyUserGenFile( const char *filename_src, const char *filename_dst )
{
	char		*ospath_src,
				*ospath_dst;

	if ( !fs_searchpaths ) 
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	// don't let sound stutter
	//S_ClearSoundBuffer(); //might need this if files are large...

	ospath_src = FS_BuildOSPath( fs_basepath->string, fs_gamedir, filename_src );
	ospath_dst = FS_BuildOSPath( fs_basepath->string, fs_gamedir, filename_dst );

	if ( fs_debug->integer ) 
	{
		Com_Printf( "FS_CopyUserGenFile: %s to %s\n", ospath_src, ospath_dst );
	}

	remove(ospath_dst);
	return (0 == rename (ospath_src, ospath_dst ));
	//return FS_CopyFile( ospath_src, ospath_dst, qtrue );
}

/*
===========
FS_SV_FOpenFileRead

===========
*/
int FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) {
  char *ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_basepath->string, filename, "" );
  // remove trailing slash
  ospath[strlen(ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileRead: %s\n", ospath );
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
  
  *fp = f;
  if (f) {
    return FS_filelength(f);
  }
  return 0;
}


/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWrite( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_basepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	//Com_DPrintf( "writing to: %s\n", ospath );
	FS_CreatePath( ospath );
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

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_basepath->string, fs_gamedir, filename );

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
			return -1;		// strings not equal
		}
	} while (c1);
	
	return 0;		// strings are equal
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
int FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE ) {
	searchpath_t	*search;
	char			*netpath;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	directory_t		*dir;
	long			hash=0;
	unz_s			*zfi;
	ZIP_FILE		*temp;
//	int				i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

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

	//
	// search through the path, one element at a time
	//

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		//
		if ( search->pack ) {
			hash = FS_HashFileName(filename, search->pack->hashSize);
		}
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {
			// disregard if it doesn't match one of the allowed pure pak files
/*			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}
*/
			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
					// found it!
					if ( uniqueFILE ) {
						// open a new file on the pakfile
						fsh[*file].handleFiles.file.z = unzReOpen (pak->pakFilename, pak->handle);
						if (fsh[*file].handleFiles.file.z == NULL) {
							Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->pakFilename);
						}
					} else {
						fsh[*file].handleFiles.file.z = pak->handle;
					}
					Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
					fsh[*file].zipFile = qtrue;
					zfi = (unz_s *)fsh[*file].handleFiles.file.z;
					// in case the file was new
					temp = zfi->file;
					// set the file position in the zip file (also sets the current file info)
					unzSetCurrentFileInfoPosition(pak->handle, pakFile->pos);
					// copy the file info into the unzip structure
					memcpy( zfi, pak->handle, sizeof(unz_s));
					// we copy this back into the structure
					zfi->file = temp;
					// open the file in the zip
					unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
					fsh[*file].zipFilePos = pakFile->pos;

					if ( fs_debug->integer ) {
						Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n", 
							filename, pak->pakFilename );
					}
					return zfi->cur_file_info.uncompressed_size;
				}
				pakFile = pakFile->next;
			} while(pakFile != NULL);
		} else if ( search->dir ) {
			// check a file in the directory tree

			// if we are running restricted, the only files we
			// will allow to come from the directory are .cfg files
			if ( fs_restrict->integer /*|| fs_numServerPaks*/ ) {
				int		l;

				l = strlen( filename );

				if ( stricmp( filename + l - 4, ".cfg" )		// for config files
					&& stricmp( filename + l - 4, ".sav" )  // for save games
					&& stricmp( filename + l - 4, ".dat" ) ) {	// for journal files
					continue;
				}
			}

			dir = search->dir;
			
			netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
			fsh[*file].handleFiles.file.o = fopen (netpath, "rb");
			if ( !fsh[*file].handleFiles.file.o ) {
				continue;
			}
			
			Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
			fsh[*file].zipFile = qfalse;
			if ( fs_debug->integer ) {
				Com_Printf( "FS_FOpenFileRead: %s (found in '%s/%s')\n", filename,
					dir->path, dir->gamedir );
			}

			// if we are getting it from the cdpath, optionally copy it
			//  to the basepath
			if ( fs_copyfiles->integer && !stricmp( dir->path, fs_cdpath->string ) ) {
				char	*copypath;

				copypath = FS_BuildOSPath( fs_basepath->string, dir->gamedir, filename );
				FS_CopyFile( netpath, copypath );
			}
			return FS_filelength (*file);
		}		
	}
	
	Com_DPrintf ("Can't find %s\n", filename);
	
	*file = 0;
	return -1;
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

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !f ) {
		return 0;
	}
	if ( f <= 0  || f >= MAX_FILE_HANDLES )
	{
		Com_Error( ERR_FATAL, "FS_Read: Invalid handle %d\n", f );
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

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

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
=================
FS_Seek

=================
*/
int FS_Seek( fileHandle_t f, long offset, int origin ) {
	int		_origin;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
		return -1;
	}

	if (fsh[f].zipFile == qtrue) {
		char	foo[65536];
		if (offset == 0 && origin == FS_SEEK_SET) {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
			return unzOpenCurrentFile(fsh[f].handleFiles.file.z);
		} else if (offset<65536) {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
			unzOpenCurrentFile(fsh[f].handleFiles.file.z);
			return FS_Read(foo, offset, f);
		} else {
			Com_Error( ERR_FATAL, "ZIP FILE FSEEK NOT YET IMPLEMENTED for big offsets(%s)\n", fsh[f].name);
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

int	FS_FileIsInPAK(const char *filename ) {
	searchpath_t	*search;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	long			hash = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
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
/*			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}
*/
			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
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
#include "..\client\client.h"
int FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t	h;
	byte*			buf;
	int				len;
	qboolean		isConfig;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	// stop sounds from repeating
	S_ClearSoundBuffer();

	buf = NULL;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = qtrue;
		if ( com_journal && com_journal->integer == 2 ) {
			int		r;

			fs_loadCount++;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );
			if ( r != sizeof( len ) ) {
				*buffer = NULL;
				return -1;
			}
			buf = (byte*) Z_Malloc( len+1, TAG_FILESYS, qfalse);
			buf[len]='\0';	// because we're not calling Z_Malloc with optional trailing 'bZeroIt' bool
			*buffer = buf;			

			r = FS_Read( buf, len, com_journalDataFile );
			if ( r != len ) {
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

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
		return -1;
	}
	
	if ( !buffer ) {
		FS_FCloseFile( h);
		return len;
	}

	fs_loadCount++;

	buf = (byte*)Z_Malloc( len+1, TAG_FILESYS, qfalse);
	buf[len]='\0';	// because we're not calling Z_Malloc with optional trailing 'bZeroIt' bool
	*buffer = buf;	

	Z_Label(buf, qpath);

	// PRECACE CHECKER!
#ifndef FINAL_BUILD
	if (com_sv_running && com_sv_running->integer && cls.state >= CA_ACTIVE) {	//com_cl_running
		if (strncmp(qpath,"menu/",5) ) {
			Com_Printf( S_COLOR_MAGENTA"FS_ReadFile: %s NOT PRECACHED!\n", qpath );
		}
	}
#endif

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
	
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	Z_Free( buffer );
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
static pack_t *FS_LoadZipFile( char *zipfile )
{
	fileInPack_t	*buildBuffer;
	pack_t			*pack;
	unzFile			uf;
	int				err;
	unz_global_info gi;
	char			filename_inzip[MAX_ZPATH];
	unz_file_info	file_info;
	int				i, len;
	long			hash;
	int				fs_numHeaderLongs;
	int				*fs_headerLongs;
	char			*namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo (uf,&gi);

	if (err != UNZ_OK)
		return NULL;

	fs_packFiles += gi.number_entry;

	len = 0;	//find the length of all filenames
	unzGoToFirstFile(uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		if ( file_info.size_filename > MAX_QPATH)
		{
			Com_Error(ERR_FATAL, "ERROR: filename length > MAX_QPATH ( strlen(%s) = %d) \n", filename_inzip, file_info.size_filename );
		}
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = (fileInPack_t *)Z_Malloc( gi.number_entry * sizeof( fileInPack_t ) + len , TAG_FILESYS, qtrue );
	namePtr = ((char *) buildBuffer) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = (int*)Z_Malloc( gi.number_entry * sizeof(int), TAG_FILESYS, qtrue );

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1) {
		if (i > gi.number_entry) {
			break;
		}
	}

	pack = (pack_t*)Z_Malloc( sizeof( pack_t ) + i * sizeof(fileInPack_t *), TAG_FILESYS, qtrue );
	memset (pack, 0, sizeof( pack_t ) + i * sizeof(fileInPack_t *));
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t **) (((char *) pack) + sizeof( pack_t ));

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );

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
		unzGetCurrentFileInfoPosition(uf, &buildBuffer[i].pos);
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	pack->checksum = LittleLong( pack->checksum );

	Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
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
		if ( !stricmp( name, list[i] ) ) {
			return nfiles;		// allready in list
		}
	}
	list[nfiles] = CopyString( name );
	nfiles++;

	return nfiles;
}

/*
===============
FS_ListFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	int				nfiles;
	char			**listCopy;
	char			*list[MAX_FOUND_FILES];
	searchpath_t	*search;
	int				i;
	int				pathLength;
	int				extensionLength;
	int				length, pathDepth;
	pack_t			*pak;
	fileInPack_t	*buildBuffer;
	char			zpath[MAX_QPATH];

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
	extensionLength = strlen( extension );
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	//
	// search through the path, one element at a time, adding to list
	//
	for (search = fs_searchpaths ; search ; search = search->next) {
		// is the element a pak file?
		if (search->pack) {
			// look through all the pak file elements
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for (i=0 ; i<pak->numfiles ; i++) {
				char	*name;
				int		zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				zpathLen = FS_ReturnPath(name, zpath, &depth);

				if ( (depth-pathDepth)>2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) ) {
					continue;
				}

				// check for extension match
				length = strlen( name );
				if ( length < extensionLength ) {
					continue;
				}

				if ( stricmp( name + length - extensionLength, extension ) ) {
					continue;
				}

				// unique the match
				nfiles = FS_AddFileToList( name + pathLength + 1, list, nfiles );
			}
		} else if (search->dir) { // scan for files in the filesystem
			char	*netpath;
			int		numSysFiles;
			char	**sysFiles;
			char	*name;

			netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
			sysFiles = Sys_ListFiles( netpath, extension, &numSysFiles, qfalse );
			for ( i = 0 ; i < numSysFiles ; i++ ) {
				// unique the match
				name = sysFiles[i];
				nfiles = FS_AddFileToList( name, list, nfiles );
			}
			Sys_FreeFileList( sysFiles );
		}		
	}

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char**)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS, qfalse);
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}


/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList( char **list ) {
	int		i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}


/*
===============
FS_AddFileToListBuf
===============
*/
static int FS_AddFileToListBuf( char *name, char *listbuf, int bufsize, int nfiles ) {
	char	*p;

	if ( nfiles == MAX_FOUND_FILES - 1 ) {
		return nfiles;
	}

	if (name[0] == '/' || name[0] == '\\') {
		name++;
	}

	p = listbuf;
	while ( *p ) {
		if ( !stricmp( name, p ) ) {
			return nfiles;		// already in list
		}
		p += strlen( p ) + 1;
	}

	if ( ( p + strlen( name ) + 2 - listbuf ) > bufsize ) {
		return nfiles;		// list is full
	}

	strcpy( p, name );
	p += strlen( p ) + 1;
	*p = 0;

	return nfiles + 1;
}

/*
================
FS_GetFileList

Returns a uniqued list of files that match the given criteria
from all search paths
================
*/
int	FS_GetModList( char *listbuf, int bufsize );
int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	int		nfiles;
	searchpath_t	*search;
	int				i;
	int				pathLength;
	int				extensionLength;
	int				length, pathDepth;
	pack_t			*pak;
	fileInPack_t	*buildBuffer;
	char			zpath[MAX_QPATH];

	if (Q_stricmp(path, "$modlist") == 0)
	{
		return FS_GetModList(listbuf, bufsize);
	}

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !path ) {
		return 0;
	}
	if ( !extension ) {
		extension = "";
	}

	pathLength = strlen( path );
	extensionLength = strlen( extension );
	nfiles = 0;
	*listbuf = 0;
	FS_ReturnPath(path, zpath, &pathDepth);
	//
	// search through the path, one element at a time, adding to list
	//
	for (search = fs_searchpaths ; search ; search = search->next) {
		// is the element a pak file?
		if (search->pack) {
			// look through all the pak file elements
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for (i=0 ; i<pak->numfiles ; i++) {
				char	*name;
				int		zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				zpathLen = FS_ReturnPath(name, zpath, &depth);

				if ( (depth-pathDepth)>2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) ) {
					continue;
				}

				// check for extension match
				length = strlen( name );
				if ( length < extensionLength || (length == (extensionLength + pathLength))) {
					continue;
				}

				if ( stricmp( name + length - extensionLength, extension ) ) {
					continue;
				}

				// unique the match
				nfiles = FS_AddFileToListBuf( name + pathLength, listbuf, bufsize, nfiles );
			}
		} else  if (search->dir) { // scan for files in the filesystem
			char	*netpath;
			int		numSysFiles;
			char	**sysFiles;
			char	*name;

			netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
			sysFiles = Sys_ListFiles( netpath, extension, &numSysFiles, qfalse );
			for ( i = 0 ; i < numSysFiles ; i++ ) {
				// unique the match
				name = sysFiles[i];
				nfiles = FS_AddFileToListBuf( name, listbuf, bufsize, nfiles );
			}
			Sys_FreeFileList( sysFiles );
		}		
	}

	return nfiles;
}

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to base with a pk3 in it

================
*/
int	FS_GetModList( char *listbuf, int bufsize ) {
	int		nMods, i, nTotal, nLen, nPaks, nPotential, nDescLen;
  char **pFiles = NULL;
  char **pPaks = NULL;
  char *name, *path;
  char descPath[MAX_OSPATH];
  fileHandle_t descHandle;

  *listbuf = 0;
  nMods = nPotential = nTotal = 0;

	pFiles = Sys_ListFiles( fs_basepath->string, ".*", &nPotential, qtrue );
	for ( i = 0 ; i < nPotential ; i++ ) {
    name = pFiles[i];
    if (Q_stricmp(name, BASEGAME) && Q_stricmpn(name, ".", 1)) {
      // ignore base
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
      nPaks = 0;
      pPaks = Sys_ListFiles(path, ".pk3", &nPaks, qfalse); 
      if (nPaks > 0) {
        nLen = strlen(name) + 1;
        // nLen is the length of the mod path
        // we need to see if there is a description available
        descPath[0] = '\0';
        strcpy(descPath, name);
        strcat(descPath, "/description.txt");
        nDescLen = FS_SV_FOpenFileRead( descPath, &descHandle);
        if ( nDescLen > 0 && descHandle) {
      		FILE *file;
      		file = FS_FileForHandle(descHandle);
			memset( descPath, 0, sizeof( descPath ) );
          nDescLen = fread(descPath, 1, 48, file);
          if (nDescLen >= 0) {
            descPath[nDescLen] = '\0';
          }
          FS_FCloseFile(descHandle);
        } else {
          strcpy(descPath, name);
        }
        nDescLen = strlen(descPath) + 1;

        if (nTotal + nLen + 1 + nDescLen + 1 < bufsize) {
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
	  	Sys_FreeFileList( pPaks );
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
============
FS_Path_f

============
*/
void FS_Path_f( void ) {
	searchpath_t	*s;
	int				i;

	Com_Printf ("Current search path:\n");
	for (s=fs_searchpaths ; s ; s=s->next) {
		if (s->pack) {
			Com_Printf ("%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles);
/*			if ( fs_numServerPaks ) {
				for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
					if ( s->pack->checksum == fs_serverPaks[i] ) {
						break;		// on the aproved list
					}
				}
				if ( i == fs_numServerPaks ) {
					Com_Printf( "    not on the pure list\n" );
				} else {
					Com_Printf( "    on the pure list\n" );
				}
			}
*/
		} else 	{
			Com_Printf ("%s/%s\n", s->dir->path, s->dir->gamedir );
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
arbitrary files during an "fs_copyfiles 1" run.
============
*/
void FS_TouchFile_f( void ) {
	fileHandle_t	f;
	int count = Cmd_Argc();

	if ( (count == 2) || (count == 3) ) {
		FS_FOpenFileRead( Cmd_Argv( 1 ), &f, qfalse );
		if ( f ) {
			FS_FCloseFile( f );
		}
		if ( count == 3 ) {
			FS_FOpenFileRead( Cmd_Argv( 2 ), &f, qfalse );
			if ( f ) {
				FS_FCloseFile( f );
			}
		}
	}
	else {
		Com_Printf( "Usage: touchFile <file> [file2] -- You gave %d args!\n", Cmd_Argc() );
	}
}

//===========================================================================


static int QDECL paksort( const void *a, const void *b ) {
	char	*aa, *bb;

	aa = *(char **)a;
	bb = *(char **)b;

	return stricmp( aa, bb );
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
	int				i;
	searchpath_t	*search;
	pack_t			*pak;
	char			*pakfile;
	int				numfiles;
	char			**pakfiles;
	char			*sorted[MAX_PAKFILES];

	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	//
	// add the directory to the search path
	//
	search = (searchpath_t *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue );
	search->dir = (directory_t*)Z_Malloc( sizeof( *search->dir ), TAG_FILESYS, qtrue );
	search->pack = 0;
	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	Z_Label(search, path);
	Z_Label(search->dir, dir);

	// find all pak files in this directory
	pakfile = FS_BuildOSPath( path, dir, "" );
	pakfile[ strlen(pakfile) - 1 ] = 0;	// strip the trailing slash

#ifdef PRE_RELEASE_DEMO
	pakfile = FS_BuildOSPath( path, dir, "asset0.pksp" );
	if ( ( pak = FS_LoadZipFile( pakfile ) ) == 0 )
		return;
	if ( (pak->numfiles^ 0x84268436u) != (DEMO_PAK_MAXFILES^ 0x84268436u))	//don't let them use the full version, even if renamed!
		return;
	search = (searchpath_t*)Z_Malloc(sizeof(searchpath_t), TAG_FILESYS, qtrue );
	search->pack = pak;
	search->dir = 0;
	search->next = fs_searchpaths;
	fs_searchpaths = search;		
#else
	pakfiles = Sys_ListFiles( pakfile, ".pk3", &numfiles, qfalse );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override asset0.pk3
	if ( numfiles > MAX_PAKFILES ) {
		numfiles = MAX_PAKFILES;
	}
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];
	}

	qsort( sorted, numfiles, 4, paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {
		pakfile = FS_BuildOSPath( path, dir, sorted[i] );
		if ( ( pak = FS_LoadZipFile( pakfile ) ) == 0 )
			continue;
		search = (searchpath_t*)Z_Malloc(sizeof(searchpath_t), TAG_FILESYS, qtrue );
		search->pack = pak;
		search->dir = 0;
		search->next = fs_searchpaths;
		fs_searchpaths = search;		
	}

	// done
	Sys_FreeFileList( pakfiles );
#endif
}

/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( void ) {
	searchpath_t	*p, *next;

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
	Cmd_RemoveCommand( "touchFile" );
}

/*
================
FS_Startup
================
*/
static void FS_Startup( const char *gameName ) {
	Com_Printf( "----- FS_Startup -----\n" );

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get ("fs_cdpath", Sys_DefaultCDPath(), CVAR_INIT );
	fs_basepath = Cvar_Get ("fs_basepath", Sys_DefaultBasePath(), CVAR_INIT );

	fs_gamedirvar = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SERVERINFO );
	fs_restrict = Cvar_Get ("fs_restrict", "", CVAR_INIT );
	Cvar_Get( "com_demo", "", CVAR_INIT );

	// set up cdpath
	if (fs_cdpath->string[0]) {
		FS_AddGameDirectory ( fs_cdpath->string, gameName );
	}

	// set up basepath
	FS_AddGameDirectory ( fs_basepath->string, gameName );

	// check for game override
	if ( fs_gamedirvar->string[0] && 
		!Q_stricmp( gameName, BASEGAME ) &&
		Q_stricmp( fs_gamedirvar->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddGameDirectory( fs_cdpath->string, fs_gamedirvar->string );
		}
		FS_AddGameDirectory( fs_basepath->string, fs_gamedirvar->string );
	}

	// add our commands
	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("dir", FS_Dir_f );
	Cmd_AddCommand ("touchFile", FS_TouchFile_f );

	// print the current search paths
	FS_Path_f();

	Com_Printf( "----------------------\n" );
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}


/*
===================
FS_SetRestrictions

Looks for product keys and restricts media add on ability
if the full version is not found
===================
*/
static void FS_SetRestrictions( void ) {
	searchpath_t	*path;

#ifndef PRE_RELEASE_DEMO
	byte	*productId;

	// if fs_restrict is set, don't even look for the id file,
	// which allows the demo release to be tested even if
	// the full game is present
	if ( !fs_restrict->integer ) {
		// look for the full game id
		FS_ReadFile( "productid.txt", (void **)&productId );
		if ( productId ) {
			// check against the hardcoded string
			unsigned int		seed, i;

			seed = 102270;
			for ( i = 0 ; i < sizeof( fs_scrambledProductId ) ; i++ ) {
#if 0
				fs_scrambledProductId[i]  = productId[i] ^ (seed&255);
				Com_Printf("%3i, ", fs_scrambledProductId[i]);
#endif
				if ( ( fs_scrambledProductId[i] ^ (seed&255) ) != productId[i] ) {
					break;
				}
				seed = (69069 * seed + 1);
			}

			FS_FreeFile( productId );

			if ( i == sizeof( fs_scrambledProductId ) ) {
				return;	// no restrictions
			}
			Com_Error( ERR_FATAL, "Invalid product identification" );
		}
	}
#endif
	Cvar_Set( "fs_restrict", "1" );
	Cvar_Set( "com_demo", "1" );

	Com_Printf( "\nRunning in restricted demo mode.\n\n" );

	// restart the filesystem with just the demo directory
	FS_Shutdown();
	FS_Startup( DEMOGAME );

	// make sure that the pak file has the header checksum we expect
	for ( path = fs_searchpaths ; path ; path = path->next ) {
		if ( path->pack ) {
			// a tiny attempt to keep the checksum from being scannable from the exe
			if ( (path->pack->checksum ^ 0x84268436u) != (DEMO_PAK_CHECKSUM ^ 0x84268436u) ) {
				Com_Error( ERR_FATAL, "Corrupted pk3: %u", path->pack->checksum );
			}
		}
	}
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
/*	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_copyfiles" );
	Com_StartupVariable( "fs_restrict" );
*/
	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
	FS_SetRestrictions();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}
}


/*
================
FS_Restart
================
*/

void FS_Restart( void ) {
	// free anything we currently have loaded
	FS_Shutdown();

	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
	FS_SetRestrictions();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}
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
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = FS_FOpenFileAppend( qpath );
		r = 0;
		break;
	default:
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}

	if ( *f ) {
		if (fsh[*f].zipFile == (int)qtrue) {
			fsh[*f].baseOffset = unztell(fsh[*f].handleFiles.file.z);
		} else {
			fsh[*f].baseOffset = ftell(fsh[*f].handleFiles.file.o);
		}
		fsh[*f].fileSize = r;
	}
	fsh[*f].handleSync = sync;

	return r;
}

int		FS_FTell( fileHandle_t f ) {
	int pos;
	if (fsh[f].zipFile == (int)qtrue) {
		pos = unztell(fsh[f].handleFiles.file.z);
	} else {
		pos = ftell(fsh[f].handleFiles.file.o);
	}
	return pos;
}

void	FS_Flush( fileHandle_t f ) {
	fflush(fsh[f].handleFiles.file.o);
}

