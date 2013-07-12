
#include "../game/q_shared.h"
#include "qcommon.h"
#include "files.h"
#include "../win32/win_file.h"
#include "../zlib/zlib.h"



static	cvar_t		*fs_openorder;


// Zlib Tech Ref says decompression should use about 44kb.  I'll
// go with 64kb as a safety factor...
#define ZI_STACKSIZE (64*1024)

static char* zi_stackTop = NULL;
static char* zi_stackBase = NULL;



//GOB stuff
//===========================================================================

struct gi_handleTable
{
	wfhandle_t file;
	bool used;
};

static gi_handleTable *gi_handles = NULL;
static int gi_cacheHandle = 0;

static GOBFSHandle gi_open(GOBChar* name, GOBAccessType type)
{
	if (type != GOBACCESS_READ) return (GOBFSHandle)0xFFFFFFFF;

	int f;
	for (f = 0; f < MAX_FILE_HANDLES; ++f)
	{
		if (!gi_handles[f].used) break;
	}

	if (f == MAX_FILE_HANDLES) return (GOBFSHandle)0xFFFFFFFF;
	
	gi_handles[f].file = WF_Open(name, true, strstr(name, "assets.gob") ? true : false);
	if (gi_handles[f].file < 0) return (GOBFSHandle)0xFFFFFFFF;
	gi_handles[f].used = true;

	return (GOBFSHandle)f;
}

static GOBBool gi_close(GOBFSHandle* handle)
{
	WF_Close(gi_handles[(int)*handle].file);
	gi_handles[(int)*handle].used = false;
	return GOB_TRUE;
}

static GOBInt32 gi_read(GOBFSHandle handle, GOBVoid* buffer, GOBInt32 size)
{
	return WF_Read(buffer, size, gi_handles[(int)handle].file);
}

static GOBInt32 gi_seek(GOBFSHandle handle, GOBInt32 offset, GOBSeekType type)
{
	int _type;
	switch (type) {
	case GOBSEEK_START: _type = SEEK_SET; break;
	case GOBSEEK_CURRENT: _type = SEEK_CUR; break;
	case GOBSEEK_END: _type = SEEK_END; break;
	default: assert(0); _type = SEEK_SET; break;
	}

	return WF_Seek(offset, _type, gi_handles[(int)handle].file);
}

static GOBVoid* gi_alloc(GOBUInt32 size)
{
	return Z_Malloc(size, TAG_FILESYS, qfalse, 32);
}

static GOBVoid gi_free(GOBVoid* ptr)
{
	Z_Free(ptr);
}

static GOBBool cache_open(GOBUInt32 size)
{
	for (gi_cacheHandle = 0; gi_cacheHandle < MAX_FILE_HANDLES; ++gi_cacheHandle)
	{
		if (!gi_handles[gi_cacheHandle].used) break;
	}

	if (gi_cacheHandle == MAX_FILE_HANDLES) return GOB_FALSE;
	
	gi_handles[gi_cacheHandle].file = WF_Open("z:\\jedi.swap", false, true);
	if (gi_handles[gi_cacheHandle].file < 0) return GOB_FALSE;

	if (!WF_Resize(size, gi_handles[gi_cacheHandle].file))
	{
		WF_Close(gi_handles[gi_cacheHandle].file);
		return GOB_FALSE;
	}

	gi_handles[gi_cacheHandle].used = true;

	return GOB_TRUE;
}

static GOBBool cache_close(GOBVoid)
{
	WF_Close(gi_handles[gi_cacheHandle].file);
	gi_handles[gi_cacheHandle].used = false;
	return GOB_TRUE;
}

static GOBInt32 cache_read(GOBVoid* buffer, GOBInt32 size)
{
	return WF_Read(buffer, size, gi_handles[gi_cacheHandle].file);
}

static GOBInt32 cache_write(GOBVoid* buffer, GOBInt32 size)
{
	return WF_Write(buffer, size, gi_handles[gi_cacheHandle].file);
}

static GOBInt32 cache_seek(GOBInt32 offset)
{
	return WF_Seek(offset, SEEK_SET, gi_handles[gi_cacheHandle].file);
}

static voidpf zi_alloc(voidpf opaque, uInt items, uInt size)
{
	voidpf ret = zi_stackTop;
	
	zi_stackTop += items * size;
	assert(zi_stackTop < zi_stackBase + ZI_STACKSIZE);

	return ret;
}

static void zi_free(voidpf opaque, voidpf address)
{
}

static GOBInt32 gi_decompress_zlib(GOBVoid* source, GOBUInt32 sourceLen, 
	GOBVoid* dest, GOBUInt32* destLen)
{
	// Copied and modified version of zlib's uncompress()...

	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;

	stream.next_out = (Bytef*)dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	stream.zalloc = zi_alloc;
	stream.zfree = zi_free;
	zi_stackTop = zi_stackBase;

	err = inflateInit(&stream);
	if (err != Z_OK) return err;

	err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		inflateEnd(&stream);
		return err == Z_OK ? Z_BUF_ERROR : err;
	}
	*destLen = stream.total_out;

	err = inflateEnd(&stream);
	return err;
}

GOBInt32 gi_decompress_null(GOBVoid* source, GOBUInt32 sourceLen, 
	GOBVoid* dest, GOBUInt32* destLen)
{
	if (sourceLen > *destLen) return -1;
	*destLen = sourceLen;

	memcpy(dest, source, sourceLen);
	return 0;
}

#ifdef GOB_PROFILE
static GOBVoid gi_profileread(GOBUInt32 code)
{
	code = LittleLong(code);
	Sys_Log("gob-prof.dat", &code, sizeof(code), true);
}
#endif

//===========================================================================




static void FS_CheckUsed(fileHandle_t f)
{
	if (!fsh[f].used)
	{
		Com_Error( ERR_FATAL, "Filesystem call attempting to use invalid handle\n" );
	}
}


int FS_filelength( fileHandle_t f )
{
	FS_CheckInit();
	FS_CheckUsed(f);
	
	if (fsh[f].gob)
	{
		GOBUInt32 cur, end, crap;
		GOBSeek(fsh[f].ghandle, 0, GOBSEEK_CURRENT, &cur);
		GOBSeek(fsh[f].ghandle, 0, GOBSEEK_END, &end);
		GOBSeek(fsh[f].ghandle, cur, GOBSEEK_START, &crap);
		
		return end;
	}
	else
	{
		int pos = WF_Tell(fsh[f].whandle);
		WF_Seek(0, SEEK_END, fsh[f].whandle);
		int end = WF_Tell(fsh[f].whandle);
		WF_Seek(pos, SEEK_SET, fsh[f].whandle);

		return end;
	}
}


void FS_FCloseFile( fileHandle_t f )
{
	FS_CheckInit();
	FS_CheckUsed(f);

	if (fsh[f].gob)
		GOBClose(fsh[f].ghandle);
	else
		WF_Close(fsh[f].whandle);

	fsh[f].used = qfalse;
}


fileHandle_t FS_FOpenFileWrite( const char *filename )
{
	FS_CheckInit();
	
	fileHandle_t f = FS_HandleForFile();

	char* osname = FS_BuildOSPath( filename );
	fsh[f].whandle = WF_Open(osname, false, false);
	if (fsh[f].whandle >= 0)
	{
		fsh[f].used = qtrue;
		fsh[f].gob = qfalse;
		return f;
	}

	return 0;
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

static int FS_FOpenFileReadOS( const char *filename, fileHandle_t f )
{
	if (Sys_GetFileCode(filename) != -1)
	{
		char* osname = FS_BuildOSPath( filename );
		fsh[f].whandle = WF_Open(osname, true, false);
		if (fsh[f].whandle >= 0)
		{
			fsh[f].used = qtrue;
			fsh[f].gob = qfalse;
			return FS_filelength(f);
		}
	}
	return -1;
}


/*
===================
FS_BuildGOBPath

Qpath may have either forward or backwards slashes
===================
*/
static char *FS_BuildGOBPath(const char *qpath )
{
	static char path[2][MAX_OSPATH];
	static int toggle;
	
	toggle ^= 1;		// flip-flop to allow two returns without clash

	if (qpath[0] == '\\' || qpath[0] == '/')
	{
		Com_sprintf( path[toggle], sizeof( path[0] ), ".%s", qpath );
	}
	else
	{
		Com_sprintf( path[toggle], sizeof( path[0] ), ".\\%s", qpath );
	}

//	FS_ReplaceSeparators( path[toggle], '\\' );
	FS_ReplaceSeparators( path[toggle] );
	
	return path[toggle];
}


static int FS_FOpenFileReadGOB( const char *filename, fileHandle_t f )
{
	char* gobname = FS_BuildGOBPath( filename );
	if (GOBOpen(gobname, &fsh[f].ghandle) == GOBERR_OK)
	{
		fsh[f].used = qtrue;
		fsh[f].gob = qtrue;
		return FS_filelength(f);
	}
	return -1;
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
int FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE )
{
	FS_CheckInit();
	
	if ( file == NULL ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'file' parameter passed\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	*file = FS_HandleForFile();

	int len;
	
	if (fs_openorder->integer == 0)
	{
		// Release mode -- read from GOB first
		len = FS_FOpenFileReadGOB(filename, *file);
		if (len < 0) len = FS_FOpenFileReadOS(filename, *file);
	}
	else
	{
		// Debug mode -- external files override GOB
		len = FS_FOpenFileReadOS(filename, *file);
		if (len < 0) len = FS_FOpenFileReadGOB(filename, *file);
	}

	if (len >= 0) return len;

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
int FS_Read( void *buffer, int len, fileHandle_t f )
{
	FS_CheckInit();
	FS_CheckUsed(f);

	if ( !f )
	{
		return 0;
	}
	
	if ( f <= 0  || f >= MAX_FILE_HANDLES )
	{
		Com_Error( ERR_FATAL, "FS_Read: Invalid handle %d\n", f );
	}

	if (fsh[f].gob)
	{
		GOBUInt32 size = GOBRead(buffer, len, fsh[f].ghandle);
		if (size == GOB_INVALID_SIZE)
		{
#if defined(FINAL_BUILD)
			/*
			extern void ERR_DiscFail(bool);
			ERR_DiscFail(false);
			*/
#else
			Com_Error( ERR_FATAL, "Failed to read from GOB" );
#endif
		}
		return size;
	}
	else
	{
		return WF_Read(buffer, len, fsh[f].whandle);
	}
}

/*
	MP has FS_Read2 which is supposed to do some extra logic.
	We don't care, and just call FS_Read()
*/
int FS_Read2( void *buffer, int len, fileHandle_t f )
{
	return FS_Read(buffer, len, f);
}

/*
=================
FS_Write
=================
*/
int FS_Write( const void *buffer, int len, fileHandle_t f )
{
	FS_CheckInit();
	FS_CheckUsed(f);

	if ( !f )
	{
		return 0;
	}
	
	if ( f <= 0  || f >= MAX_FILE_HANDLES )
	{
		Com_Error( ERR_FATAL, "FS_Read: Invalid handle %d\n", f );
	}

	if (fsh[f].gob)
	{
		Com_Error( ERR_FATAL, "FS_Write: Cannot write to GOB files %d\n", f );
	}
	else
	{
		return WF_Write(buffer, len, fsh[f].whandle);
	}

	return 0;
}


/*
=================
FS_Seek

=================
*/
int FS_Seek( fileHandle_t f, long offset, int origin )
{
	FS_CheckInit();
	FS_CheckUsed(f);

	if (fsh[f].gob)
	{
		int _origin;
		switch( origin ) {
		case FS_SEEK_CUR: _origin = GOBSEEK_CURRENT; break;
		case FS_SEEK_END: _origin = GOBSEEK_END; break;
		case FS_SEEK_SET: _origin = GOBSEEK_START; break;
		default:
			_origin = GOBSEEK_CURRENT;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		GOBUInt32 pos;
		GOBSeek(fsh[f].ghandle, offset, _origin, &pos);
		return pos;
	}
	else
	{
		int _origin;
		switch( origin ) {
		case FS_SEEK_CUR: _origin = SEEK_CUR; break;
		case FS_SEEK_END: _origin = SEEK_END; break;
		case FS_SEEK_SET: _origin = SEEK_SET; break;
		default:
			_origin = SEEK_CUR;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		return WF_Seek(offset, _origin, fsh[f].whandle);
	}
}


/*
=================
FS_Access
=================
*/
qboolean FS_Access( const char *filename )
{
	GOBBool status;
	
	FS_CheckInit();

	char* gobname = FS_BuildGOBPath( filename );
	if (GOBAccess(gobname, &status) != GOBERR_OK || status != GOB_TRUE)
	{
		return Sys_GetFileCode( filename ) != -1;
	}

	return qtrue;
}


/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

#ifdef _JK2MP
int	FS_FileIsInPAK(const char *filename, int *pChecksum)
#else
int	FS_FileIsInPAK(const char *filename)
#endif
{
	FS_CheckInit();

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	GOBBool exists;
	GOBAccess(const_cast<GOBChar*>(filename), &exists);

#ifdef _JK2MP
	*pChecksum = 0;
#endif

	return exists ? 1 : -1;
}

/*
============
FS_ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_ReadFile( const char *qpath, void **buffer )
{
	FS_CheckInit();
	
	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	// stop sounds from repeating
	S_ClearSoundBuffer();

	fileHandle_t h;
	int len = FS_FOpenFileRead( qpath, &h, qfalse );
	if ( h == 0 )
	{
		if ( buffer ) *buffer = NULL;
		return -1;
	}
	
	if ( !buffer )
	{
		FS_FCloseFile(h);
		return len;
	}

	// assume temporary....
	byte* buf = (byte*)Z_Malloc( len+1, TAG_TEMP_WORKSPACE, qfalse, 32);
	buf[len]='\0';

//	Z_Label(buf, qpath);

	FS_Read(buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	*buffer = buf;
	return len;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer )
{
	FS_CheckInit();

	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	Z_Free( buffer );
}


int	FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	FS_CheckInit();

	if (mode != FS_READ)
	{
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}
	
	return FS_FOpenFileRead( qpath, f, qtrue );
}


int	FS_FTell( fileHandle_t f )
{
	FS_CheckInit();
	FS_CheckUsed(f);

	if (fsh[f].gob)
	{
		GOBUInt32 pos;
		GOBSeek(fsh[f].ghandle, 0, GOBSEEK_CURRENT, &pos);
		return pos;
	}
	else
	{
		return WF_Tell(fsh[f].whandle);
	}
}

/*
================
FS_Startup
================
*/
void FS_Startup( const char *gameName )
{
	Com_Printf( "----- FS_Startup -----\n" );

	fs_openorder = Cvar_Get( "fs_openorder", "0", 0 );
	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get ("fs_cdpath", Sys_DefaultCDPath(), CVAR_INIT );	
	fs_basepath = Cvar_Get ("fs_basepath", Sys_DefaultBasePath(), CVAR_INIT );
	fs_gamedirvar = Cvar_Get ("fs_game", "base", CVAR_INIT|CVAR_SERVERINFO );
	fs_restrict = Cvar_Get ("fs_restrict", "", CVAR_INIT );

	gi_handles = new gi_handleTable[MAX_FILE_HANDLES];
	for (int f = 0; f < MAX_FILE_HANDLES; ++f)
	{
		fsh[f].used = false;
		gi_handles[f].used = false;
	}

	zi_stackBase = (char*)Z_Malloc(ZI_STACKSIZE, TAG_FILESYS, qfalse);

	GOBMemoryFuncSet mem;
	mem.alloc = gi_alloc;
	mem.free = gi_free;
	
	GOBFileSysFuncSet file;
	file.close = gi_close;
	file.open = gi_open;
	file.read = gi_read;
	file.seek = gi_seek;
	file.write = NULL;
	
	GOBCacheFileFuncSet cache;
	cache.close = cache_close;
	cache.open = cache_open;
	cache.read = cache_read;
	cache.seek = cache_seek;
	cache.write = cache_write;

	GOBCodecFuncSet codec = {
		2, // codecs
		{
			{ // Codec 0 - zlib
				'z', GOB_INFINITE_RATIO, // tag, ratio (ratio is meaningless for decomp)
				NULL,
				gi_decompress_zlib,
			},
			{ // Codec 1 - null
				'0', GOB_INFINITE_RATIO, // tag, ratio (ratio is meaningless for decomp)
				NULL,
				gi_decompress_null,
			},
		}
	};

	if (
		GOBInit(&mem, &file, &codec, NULL)
		!= GOBERR_OK)
	{
		Com_Error( ERR_FATAL, "Could not initialize GOB" );
	}

	char* archive = FS_BuildOSPath( "assets" );
	if (GOBArchiveOpen(archive, GOBACCESS_READ, GOB_FALSE, GOB_TRUE) != GOBERR_OK)
	{
#if defined(FINAL_BUILD)
		/*
		extern void ERR_DiscFail(bool);
		ERR_DiscFail(false);
		*/
#else
		//Com_Error( ERR_FATAL, "Could not initialize GOB" );
		Cvar_Set("fs_openorder", "1");
#endif
	}
	
	GOBSetCacheSize(1);
	GOBSetReadBufferSize(32 * 1024);

#ifdef GOB_PROFILE
	GOBProfileFuncSet profile = {
		gi_profileread
	};
	GOBSetProfileFuncs(&profile);
	GOBStartProfile();
#endif

	Com_Printf( "----------------------\n" );
}

/*
============================

DIRECTORY SCANNING FUCNTIONS

============================
*/

#define	MAX_FOUND_FILES	0x1000

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
//	list[nfiles] = CopyString( name );
	list[nfiles] = (char *) Z_Malloc( strlen(name) + 1, TAG_LISTFILES, qfalse );
	strcpy(list[nfiles], name);
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
char **FS_ListFiles( const char *path, const char *extension, int *numfiles )
{
	char			*netpath;
	int				numSysFiles;
	char			**sysFiles;
	char			*name;
	int				nfiles = 0;
	char			**listCopy;
	char			*list[MAX_FOUND_FILES];
	int				i;

	FS_CheckInit();

	if ( !path ) {
		*numfiles = 0;
		return NULL;
	}

	// We don't do any fancy searchpath magic here, it's all in the meta-file
	// that Sys_ListFiles will return
	netpath = FS_BuildOSPath( path );
#ifdef _JK2MP
	sysFiles = Sys_ListFiles( netpath, extension, NULL, &numSysFiles, qfalse );
#else
	sysFiles = Sys_ListFiles( netpath, extension, &numSysFiles, qfalse );
#endif
	for ( i = 0 ; i < numSysFiles ; i++ ) {
		// unique the match
		name = sysFiles[i];
		nfiles = FS_AddFileToList( name, list, nfiles );
	}
	Sys_FreeFileList( sysFiles );

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char**)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_LISTFILES, qfalse);
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
void FS_FreeFileList( char **filelist )
{
	int		i;

	FS_CheckInit();

	if ( !filelist ) {
		return;
	}

	for ( i = 0 ; filelist[i] ; i++ ) {
		Z_Free( filelist[i] );
	}

	Z_Free( filelist );
}

/*
===============
FS_AddFileToListBuf
===============
*/
static int FS_AddFileToListBuf( char *name, char *listbuf, int bufsize, int nfiles )
{
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
int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize )
{
	int				nfiles = 0;
	int				i;
	char			*netpath;
	int				numSysFiles;
	char			**sysFiles;
	char			*name;

	FS_CheckInit();

	if ( !path ) {
		return 0;
	}
	if ( !extension ) {
		extension = "";
	}

	// Prime the file list buffer
	listbuf[0] = '\0';
	netpath = FS_BuildOSPath( path );
#ifdef _JK2MP
	sysFiles = Sys_ListFiles( netpath, extension, NULL, &numSysFiles, qfalse );
#else
	sysFiles = Sys_ListFiles( netpath, extension, &numSysFiles, qfalse );
#endif
	for ( i = 0 ; i < numSysFiles ; i++ ) {
		// unique the match
		name = sysFiles[i];
		nfiles = FS_AddFileToListBuf( name, listbuf, bufsize, nfiles );
	}
	Sys_FreeFileList( sysFiles );

	return nfiles;
}

/*
=================
 Filesytem STUBS
=================
*/

qboolean FS_ConditionalRestart(int checksumFeed)
{
	return qfalse;
}

void FS_ClearPakReferences(int flags)
{
	return;
}

const char *FS_LoadedPakNames(void)
{
	return "";
}

const char *FS_ReferencedPakNames(void)
{
	return "";
}

void FS_SetRestrictions(void)
{
	return;
}

#ifdef _JK2MP
void FS_Restart(int checksumFeed)
#else
void FS_Restart(void)
#endif
{
	return;
}

qboolean FS_FileExists(const char *file)
{
	assert(!"FS_FileExists not implemented on Xbox");
	return qfalse;
}

void FS_UpdateGamedir(void)
{
	return;
}

void FS_PureServerSetReferencedPaks(const char *pakSums, const char *pakNames)
{
	return;
}

void FS_PureServerSetLoadedPaks(const char *pakSums, const char *pakNames)
{
	return;
}

const char *FS_ReferencedPakChecksums(void)
{
	return "";
}

const char *FS_LoadedPakChecksums(void)
{
	return "";
}
