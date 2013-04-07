#ifndef __FILES_H
#define __FILES_H

/*
   Structures local to the files_* modules.
*/

#ifdef _XBOX
#include "../goblib/goblib.h"

typedef int wfhandle_t;
#else
#include "../zlib32/zip.h"
#include "unzip.h"
#endif

#define	BASEGAME			"base"
#define	DEMOGAME			"demo"

// every time a new demo pk3 file is built, this checksum must be updated.
// the easiest way to get it is to just run the game and see what it spits out
#define	DEMO_PAK_CHECKSUM	437558517u

// if this is defined, the executable positively won't work with any paks other
// than the demo pak, even if productid is present.  This is only used for our
// last demo release to prevent the mac and linux users from using the demo
// executable with the production windows pak before the mac/linux products
// hit the shelves a little later
// NOW defined in build files
//#define PRE_RELEASE_TADEMO

#define MAX_ZPATH			256
#define	MAX_SEARCH_PATHS	4096
#define MAX_FILEHASH_SIZE	1024

typedef struct fileInPack_s {
	char					*name;		// name of the file
	unsigned long			pos;		// file info position in zip
	struct	fileInPack_s*	next;		// next file in the hash
} fileInPack_t;

typedef struct {
	char			pakFilename[MAX_OSPATH];	// c:\quake3\base\pak0.pk3
	char			pakBasename[MAX_OSPATH];	// pak0
	char			pakGamename[MAX_OSPATH];	// base
#ifndef _XBOX
	unzFile			handle;						// handle to zip file
#endif
	int				checksum;					// regular checksum
	int				pure_checksum;				// checksum for pure
	int				numfiles;					// number of files in pk3
	int				referenced;					// referenced file flags
	int				hashSize;					// hash table size (power of 2)
	fileInPack_t*	*hashTable;					// hash table
	fileInPack_t*	buildBuffer;				// buffer with the filenames etc.
} pack_t;

typedef struct {
	char		path[MAX_OSPATH];		// c:\jk2
	char		gamedir[MAX_OSPATH];	// base
} directory_t;

typedef struct searchpath_s {
	struct searchpath_s *next;

	pack_t		*pack;		// only one of pack / dir will be non NULL
	directory_t	*dir;
} searchpath_t;


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
	qboolean	streamed;
	char		name[MAX_ZPATH];

#ifdef _XBOX
	GOBHandle	ghandle;
	qboolean	gob;
	qboolean	used;
	wfhandle_t  whandle;
#endif
} fileHandleData_t;


extern char			fs_gamedir[MAX_OSPATH];	// this will be a single file name with no separators
extern cvar_t		*fs_debug;
extern cvar_t		*fs_homepath;
extern cvar_t		*fs_basepath;
extern cvar_t		*fs_basegame;
extern cvar_t		*fs_cdpath;
extern cvar_t		*fs_copyfiles;
extern cvar_t		*fs_gamedirvar;
extern cvar_t		*fs_restrict;
extern cvar_t		*fs_dirbeforepak; //rww - when building search path, keep directories at top and insert pk3's under them
extern searchpath_t	*fs_searchpaths;
extern int			fs_readCount;			// total bytes read
extern int			fs_loadCount;			// total files read
extern int			fs_loadStack;			// total files in memory
extern int			fs_packFiles;			// total number of files in packs

extern int			fs_fakeChkSum;
extern int			fs_checksumFeed;

extern fileHandleData_t	fsh[MAX_FILE_HANDLES];

extern qboolean		initialized;

// never load anything from pk3 files that are not present at the server when pure
extern int		fs_numServerPaks;
extern int		fs_serverPaks[MAX_SEARCH_PATHS];				// checksums
extern char		*fs_serverPakNames[MAX_SEARCH_PATHS];			// pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
extern int		fs_numServerReferencedPaks;
extern int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];			// checksums
extern char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];		// pk3 names

// last valid game folder used
extern char		lastValidBase[MAX_OSPATH];
extern char		lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
extern FILE*	missingFiles;
#endif


void			FS_Startup( const char *gameName );
qboolean		FS_CreatePath(char *OSPath);
char			*FS_BuildOSPath( const char *base, const char *game, const char *qpath );
char			*FS_BuildOSPath( const char *qpath );
fileHandle_t	FS_HandleForFile(void);
qboolean		FS_FilenameCompare( const char *s1, const char *s2 );
int				FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp );
void			FS_Shutdown( void );
void			FS_SetRestrictions(void);
void			FS_CheckInit(void);
void			FS_ReplaceSeparators( char *path );

#endif
