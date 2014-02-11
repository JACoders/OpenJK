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

#include "q_shared.h"
#include "qcommon.h"
#include "files.h"

/*
================
return a hash value for the filename
================
*/
long FS_HashFileName( const char *fname, int hashSize ) {
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


static FILE	*FS_FileForHandle( fileHandle_t f ) {
	if ( f < 1 || f >= MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of range" );
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
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
// added extra param so behind-the-scenes copying in savegames doesn't clutter up the screen -slc
qboolean FS_CopyFile( char *fromOSPath, char *toOSPath, qboolean qbSilent = qfalse );
qboolean FS_CopyFile( char *fromOSPath, char *toOSPath, qboolean qbSilent ) {
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

	if ( len == EOF )
	{
		fclose(f);
		if (qbSilent){
			return qfalse;
		}
		Com_Error( ERR_FATAL, "Bad file length in FS_CopyFile()" );
	}
	
	buf = (unsigned char *) Z_Malloc( len, TAG_FILESYS, qfalse);
	if (fread( buf, 1, len, f ) != (size_t) len)
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
	if (fwrite( buf, 1, len, f ) != (size_t)len)
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

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

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
qboolean FS_MoveUserGenFile( const char *filename_src, const char *filename_dst )
{
	char		*ospath_src,
				*ospath_dst;

	if ( !fs_searchpaths ) 
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	ospath_src = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename_src );
	ospath_dst = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename_dst );

	if ( fs_debug->integer ) 
	{
		Com_Printf( "FS_MoveUserGenFile: %s to %s\n", ospath_src, ospath_dst );
	}

/*	int iSlashes1=0;
	int iSlashes2=0;
	char *p;
	for (p = strchr(filename_src,'/'); p; iSlashes1++)
	{
		p = strchr(p+1,'/');
	}
	for (p = strchr(filename_dst,'/'); p; iSlashes2++)
	{
		p = strchr(p+1,'/');
	}

	if (iSlashes1 != iSlashes2)
	{
		int ret = FS_CopyFile( ospath_src, ospath_dst, qtrue );
		remove(ospath_src);
		return ret;
	}
	else
*/
	{
	remove(ospath_dst);
	return (0 == rename (ospath_src, ospath_dst ));
	}
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

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

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

#ifdef _WIN32
static bool FS_FileCacheable(const char* const filename) 
{
	extern	cvar_t	*com_buildScript;
	if (com_buildScript && com_buildScript->integer)
	{ 
		return true;
	}
	return( strchr(filename, '/') != 0 );
}
#endif

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
	void		*temp;
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
							fsh[*file].handleFiles.file.z = unzOpen (pak->pakFilename);
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
						temp = zfi->filestream;
						// set the file position in the zip file (also sets the current file info)
						unzSetOffset(pak->handle, pakFile->pos);
						// copy the file info into the unzip structure
						memcpy( zfi, pak->handle, sizeof(unz_s));
						// we copy this back into the structure
						zfi->filestream = temp;
						// open the file in the zip
						unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
						fsh[*file].zipFilePos = pakFile->pos;
						fsh[*file].zipFileLen = pakFile->len;

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
				if ( 0/*fs_restrict->integer*/ /*|| fs_numServerPaks*/ ) {
					int		l;

					l = strlen( filename );

					if ( Q_stricmp( filename + l - 4, ".cfg" )		// for config files
						&& Q_stricmp( filename + l - 4, ".sav" )  // for save games
						&& Q_stricmp( filename + l - 4, ".dat" ) ) {	// for journal files
						continue;
					}
				}

				dir = search->dir;
				
				netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );

				fsh[*file].handleFiles.file.o = fopen (netpath, "rb");
				if ( !fsh[*file].handleFiles.file.o ) {
					continue;
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
					Com_Printf( "FS_FOpenFileRead: %s (found in '%s/%s')\n", filename,
						dir->path, dir->gamedir );
				}

#ifdef _WIN32
				// if we are getting it from the cdpath, optionally copy it
				//  to the basepath
				if ( fs_copyfiles->integer && !stricmp( dir->path, fs_cdpath->string ) ) {
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
								Com_Printf( S_COLOR_CYAN"fs_copyfiles(2), Copying: %s to %s\n", netpath, copypath );
								
								FS_CreatePath( copypath );

								if (Sys_CopyFile( netpath, copypath, qtrue ))
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

				return FS_fplength(fsh[*file].handleFiles.file.o);
			}
		}
	}
	while ( bFasterToReOpenUsingNewLocalFile );
	
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

#define PK3_SEEK_BUFFER_SIZE 65536
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
#include "../client/client.h"
int FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t	h;
	byte*			buf;
	int				len;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	// stop sounds from repeating
	S_ClearSoundBuffer();

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
	*buffer = buf;	

	Z_Label(buf, qpath);

	// PRECACE CHECKER!
#ifndef FINAL_BUILD
	if (com_sv_running && com_sv_running->integer && cls.state >= CA_ACTIVE) {	//com_cl_running
		if (strncmp(qpath,"menu/",5) ) {
			Com_DPrintf( S_COLOR_MAGENTA"FS_ReadFile: %s NOT PRECACHED!\n", qpath );
		}
	}
#endif

	FS_Read (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );
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

	fs_packFiles += gi.number_entry;

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
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t **) (((char *) pack) + sizeof( pack_t ));
	for(int j = 0; j < pack->hashSize; j++) {
		pack->hashTable[j] = NULL;
	}

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
		buildBuffer[i].pos = unzGetOffset(uf);
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum( fs_headerLongs, sizeof(*fs_headerLongs) * fs_numHeaderLongs );
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

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS, qfalse );
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
void FS_FreeFileList( char **filelist ) {
	int		i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filelist ) {
		return;
	}

	for ( i = 0 ; filelist[i] ; i++ ) {
		Z_Free( filelist[i] );
	}

	Z_Free( filelist );
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

// NOTE: could prolly turn out useful for the win32 version too, but it's used only by linux and Mac OS X
//#if defined(__linux__) || defined(MACOS_X)
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

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to base with a pk3 in it

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
		// we drop "base" "." and ".."
		if (Q_stricmp(name, BASEGAME) && Q_stricmpn(name, ".", 1)) {
			// ignore base
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
			nPaks = 0;
			pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, qfalse);
			Sys_FreeFileList( pPaks );// we only use Sys_ListFiles to check wether .pk3 files are present

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
	const char	*path;
	const char	*extension;
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

Ignore case and seprator char distinctions
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
		Com_Printf( "example: fdir *t1*.bsp\n");
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
	for (s=fs_searchpaths ; s ; s=s->next) {
		if (s->pack) {
			Com_Printf ("%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles);
		} else 	{
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
	char			*pakfile;
	int				numfiles;
	char			**pakfiles;
	char			*sorted[MAX_PAKFILES];

	// this fixes the case where fs_basepath is the same as fs_cdpath
	// which happens on full installs
	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->dir && !Q_stricmp(sp->dir->path, path) && !Q_stricmp(sp->dir->gamedir, dir)) {
			return;			// we've already got this one
		}
	}

	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	//
	// add the directory to the search path
	//
	search = (searchpath_t *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue );
	search->dir = (directory_t*)Z_Malloc( sizeof( *search->dir ), TAG_FILESYS, qtrue );

	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	Z_Label(search, path);
	Z_Label(search->dir, dir);

	// find all pak files in this directory
	pakfile = FS_BuildOSPath( path, dir, "" );
	pakfile[ strlen(pakfile) - 1 ] = 0;	// strip the trailing slash

	thedir = search;

	pakfiles = Sys_ListFiles( pakfile, ".pk3", NULL, &numfiles, qfalse );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override asset0.pk3
	if ( numfiles > MAX_PAKFILES ) {
		numfiles = MAX_PAKFILES;
	}
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];
	}

	qsort( sorted, numfiles, sizeof(char*), paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {
		pakfile = FS_BuildOSPath( path, dir, sorted[i] );
		if ( ( pak = FS_LoadZipFile( pakfile ) ) == 0 )
			continue;
		search = (searchpath_t*)Z_Malloc(sizeof(searchpath_t), TAG_FILESYS, qtrue );
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
FS_Startup
================
*/
void FS_Startup( const char *gameName ) {
	const char *homePath;

	Com_Printf( "----- FS_Startup -----\n" );

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get ("fs_cdpath", "", CVAR_INIT);
	fs_basepath = Cvar_Get ("fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT);
	fs_basegame = Cvar_Get ("fs_basegame", "", CVAR_INIT );
	homePath = Sys_DefaultHomePath();
	if (!homePath || !homePath[0]) {
		homePath = fs_basepath->string;
	}
	fs_homepath = Cvar_Get ("fs_homepath", homePath, CVAR_INIT );
	fs_gamedirvar = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO );

	fs_dirbeforepak = Cvar_Get("fs_dirbeforepak", "0", CVAR_INIT);

	// add search path elements in reverse priority order
	if (fs_cdpath->string[0]) {
		FS_AddGameDirectory( fs_cdpath->string, gameName );
	}
	if (fs_basepath->string[0]) {
		FS_AddGameDirectory( fs_basepath->string, gameName );
	}
	
#ifdef MACOS_X
	fs_apppath = Cvar_Get ("fs_apppath", Sys_DefaultAppPath(), CVAR_INIT );
	// Make MacOSX also include the base path included with the .app bundle
	if (fs_apppath->string[0]) {
		FS_AddGameDirectory( fs_apppath->string, gameName );
	}
#endif

	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if (fs_homepath->string[0] && Q_stricmp(fs_homepath->string,fs_basepath->string)) {
		FS_CreatePath ( fs_homepath->string );
		FS_AddGameDirectory( fs_homepath->string, gameName );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && Q_stricmp( fs_basegame->string, gameName ) ) {
		if (fs_cdpath->string[0]) {
			FS_AddGameDirectory(fs_cdpath->string, fs_basegame->string);
		}
		if (fs_basepath->string[0]) {
			FS_AddGameDirectory(fs_basepath->string, fs_basegame->string);
		}
		if (fs_homepath->string[0] && Q_stricmp(fs_homepath->string,fs_basepath->string)) {
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
		if (fs_homepath->string[0] && Q_stricmp(fs_homepath->string,fs_basepath->string)) {
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string);
		}
	}

	// add our commands
	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("dir", FS_Dir_f );
	Cmd_AddCommand ("fdir", FS_NewDir_f );
	Cmd_AddCommand ("touchFile", FS_TouchFile_f );
	Cmd_AddCommand ("which", FS_Which_f );

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
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

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if (lastValidBase[0]) {
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_game", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart();
			Com_Error( ERR_DROP, "Invalid game folder\n" );
			return;
		}
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	if ( Q_stricmp(fs_gamedirvar->string, lastValidGame) ) {
		// skip the jampconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText ("exec " Q3CONFIG_NAME "\n");
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
qboolean FS_ConditionalRestart( void ) {
	if( fs_gamedirvar->modified ) {
		FS_Restart( );
		return qtrue;
	}
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

void FS_FilenameCompletion( const char *dir, const char *ext, qboolean stripExt, void(*callback)( const char *s ), qboolean allowNonPureFilesOnDisk ) {
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
