#pragma once

/*
   Structures local to the files_* modules.
*/

#include "unzip.h"

#define	BASEGAME			"base"

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
	unsigned long			len;		// uncompress file size
	struct	fileInPack_s*	next;		// next file in the hash
} fileInPack_t;

typedef struct {
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

typedef struct {
	char		path[MAX_OSPATH];		// c:\jediacademy\gamedata
	char		fullpath[MAX_OSPATH];	// c:\jediacademy\gamedata\base
	char		gamedir[MAX_OSPATH];	// base
} directory_t;

typedef struct searchpath_s {
	struct searchpath_s *next;

	pack_t		*pack;		// only one of pack / dir will be non NULL
	directory_t	*dir;
} searchpath_t;


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
	qboolean	streamed;
	char		name[MAX_ZPATH];
} fileHandleData_t;


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
void			FS_CheckInit(void);
void			FS_ReplaceSeparators( char *path );
void			FS_FreePak( pack_t *thepak );
