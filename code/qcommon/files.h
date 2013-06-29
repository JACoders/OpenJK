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

#ifndef __FILES_H
#define __FILES_H



/*
   Structures local to the files_* modules.
*/



#ifdef _XBOX
#include "../goblib/goblib.h"

typedef int wfhandle_t;
#else
#include "zlib/zlib.h"
#include "unzip.h"
#endif


#define MAX_ZPATH			256
#define	BASEGAME			"base"



typedef struct fileInPack_s {
	char					*name;		// name of the file
	unsigned long			pos;		// file info position in zip
	struct	fileInPack_s*	next;		// next file in the hash
} fileInPack_t;

typedef struct {
	char			pakFilename[MAX_OSPATH];	// c:\quake3\base\asset0.pk3
#ifndef _XBOX
	unzFile			handle;
#endif
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


#define	MAX_FILE_HANDLES	16

typedef union qfile_gus {
	FILE*		o;
#ifndef _XBOX
	unzFile		z;
#endif
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

#ifdef _XBOX
	GOBHandle	ghandle;
	qboolean	gob;
	qboolean	used;
	wfhandle_t  whandle;
#endif
} fileHandleData_t;


extern fileHandleData_t	fsh[MAX_FILE_HANDLES];

extern searchpath_t	*fs_searchpaths;
extern char			fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
extern cvar_t		*fs_debug;
extern cvar_t		*fs_homepath;

#ifdef MACOS_X
// Also search the .app bundle for .pk3 files
extern cvar_t          *fs_apppath;
#endif

extern cvar_t		*fs_basepath;
extern cvar_t		*fs_basegame;
extern cvar_t		*fs_cdpath;
extern cvar_t		*fs_copyfiles;
extern cvar_t		*fs_gamedirvar;
extern int			fs_readCount;			// total bytes read
extern int			fs_loadCount;			// total files read
extern int			fs_packFiles;			// total number of files in packs


// last valid game folder used
extern char		lastValidBase[MAX_OSPATH];
extern char		lastValidGame[MAX_OSPATH];

void			FS_Startup( const char *gameName );
void			FS_CreatePath(char *OSPath);
char			*FS_BuildOSPath( const char *base, const char *game, const char *qpath );
char			*FS_BuildOSPath( const char *qpath );
fileHandle_t	FS_HandleForFile(void);
qboolean		FS_FilenameCompare( const char *s1, const char *s2 );
int				FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp );
void			FS_Shutdown( void );
void			FS_CheckInit(void);
void			FS_ReplaceSeparators( char *path );


#endif
