
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

#include "../server/exe_headers.h"
#include "../client/client.h"
#include "../win32/win_local.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/fixedmap.h"
#include "../zlib/zlib.h"
#include "../qcommon/files.h"

/***********************************************
*
* WINDOWS/XBOX VERSION
*
* Build a translation table, CRC -> file name.  We have the memory.
*
************************************************/

#if defined(_WINDOWS)
#include <windows.h>
#elif defined(_XBOX)
#include <xtl.h>
#endif

struct FileInfo
{
	char* name;
	int size;
};
static VVFixedMap< FileInfo, unsigned int >* s_Files = NULL;
static byte* buffer;

HANDLE s_Mutex = INVALID_HANDLE_VALUE;

int _buildFileList(const char* path, bool insert, bool buildList)
{
	WIN32_FIND_DATA data;
	char spec[MAX_OSPATH];
	int count = 0;

	// Look for all files
	Com_sprintf(spec, sizeof(spec), "%s\\*.*", path);

	HANDLE h = FindFirstFile(spec, &data);
	while (h != INVALID_HANDLE_VALUE)
	{
		char full[MAX_OSPATH];
		Com_sprintf(full, sizeof(full), "%s\\%s", path, data.cFileName);

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Directory -- lets go recursive
			if (data.cFileName[0] != '.') {
				count += _buildFileList(full, insert, buildList);
			}
		}
		else
		{

			if(insert || buildList)
			{
				// Regular file -- add it to the table
				strlwr(full);
				unsigned int code = crc32(0, (const byte *)full, strlen(full));

				FileInfo info;
				info.name = CopyString(full);
				info.size = data.nFileSizeLow;

				if(insert)
				{
					s_Files->Insert(info, code);
				}

				if(buildList)
				{
					// get the length of the filename
					int len;
					len = strlen(info.name) + 1;

					// save the file code
					*(int*)buffer		=  code;
					buffer				+= sizeof(code);

					// save the name of the file
					strcpy((char*)buffer,info.name);
					buffer				+= len;

					// save the size of the file
					*(int*)buffer		= info.size;
					buffer				+= sizeof(info.size);
				}
			}

			count++;
		}

		// Continue the loop
		if (!FindNextFile(h, &data))
		{
			FindClose(h);
			return count;
		}
	}
	return count;
}

bool _buildFileListFromSavedList(void)
{
	// open the file up for reading
	FILE*	in;
	extern const char *Sys_RemapPath( const char *filename );
	in = fopen( Sys_RemapPath( "xbx_filelist" ), "rb" );
	if(!in)
	{
		return false;
	}

	// read in the number of files
	int count;
	if(!(fread(&count,sizeof(count),1,in)))
	{
		fclose(in);
		return false;
	}

	// allocate memory for a temp buffer
	byte*	baseAddr;
	int bufferSize;
	bufferSize	= count * ( 2 * sizeof(int) + MAX_OSPATH );
	buffer		= (byte*)Z_Malloc(bufferSize,TAG_TEMP_WORKSPACE,qtrue,32);
	baseAddr	= buffer;

	// read the rest of the file into a big buffer
	if(!(fread(buffer,bufferSize,1,in)))
	{
		fclose(in);
		Z_Free(baseAddr);
		return false;
	}

	// allocate some memory for s_Files
	s_Files = new VVFixedMap<FileInfo, unsigned int>(count);

	// loop through all the files write out the codes
	int i;
	for(i = 0; i < count; i++)
	{
		FileInfo info;
		unsigned int code;

		// read the code for the file
		code	=  *(int*)buffer;
		buffer	+= sizeof(code);

		// read the filename
		info.name = CopyString((char*)buffer);
		buffer	+= (strlen(info.name) + 1);

		// read the size of the file
		info.size	=  *(int*)buffer;
		buffer		+= sizeof(info.size);

		// save the data - optimization: don't check for dupes!
		s_Files->InsertUnsafe(info, code);
	}

	fclose(in);
	Z_Free(baseAddr);
	return true;
}

bool Sys_SaveFileCodes(void)
{
	bool ret;

	// get the number of files
	int count;
	count = _buildFileList(Sys_Cwd(), false, false);

	// open a file for writing
	FILE* out;
	out = fopen("d:\\xbx_filelist","wb");
	if(!out)
	{
		return false;
	}

	// allocate a buffer for writing
	byte*	baseAddr;
	int		bufferSize;
	
	bufferSize	= sizeof(int) + ( count * ( 2 * sizeof(int) + MAX_OSPATH ) );
	baseAddr	= (byte*)Z_Malloc(bufferSize,TAG_TEMP_WORKSPACE,qtrue,32);
	buffer		= baseAddr;

	// write the number of files to the buffer
	*(int*)buffer	=  count;
	buffer			+= sizeof(count);

	// fill up the rest of the buffer
	ret = _buildFileList(Sys_Cwd(), false, true);

	if(!ret)
	{
		// there was a problem
		fclose(out);
		Z_Free(baseAddr);
		return false;
	}

	// attempt to write out the data
	if(!(fwrite(baseAddr,bufferSize,1,out)))
	{
		// there was a problem
		fclose(out);
		Z_Free(baseAddr);
		return false;
	}

	// everything went ok
	fclose(out);
	Z_Free(baseAddr);
	return true;
}

void Sys_InitFileCodes(void)
{
	bool ret;
	int count = 0;

	Z_PushNewDeleteTag( TAG_FILELIST );

	// First: try to load an existing filecode cache
	ret = _buildFileListFromSavedList();

	// if we had trouble building the list that way
	// we need to do it by searching the files
	if( !ret )
	{
		// There was no filelist cache, make one
		if( !Sys_SaveFileCodes() )
			Com_Error( ERR_DROP, "ERROR: Couldn't create filecode cache\n" );

		// Now re-read it
		if( !_buildFileListFromSavedList() )
			Com_Error( ERR_DROP, "ERROR: Couldn't re-read filecode cache\n" );
	}
	s_Files->Sort();

	Z_PopNewDeleteTag();

	// make it thread safe
	s_Mutex = CreateMutex(NULL, FALSE, NULL);
}

void Sys_ShutdownFileCodes(void)
{
	FileInfo*	info = NULL;

	info = s_Files->Pop();
	while(info)
	{
		Z_Free(info->name);
		info->name = NULL;
		info = s_Files->Pop();
	}

	delete s_Files;
	s_Files = NULL;

	CloseHandle(s_Mutex);
}

int Sys_GetFileCode(const char* name)
{
	WaitForSingleObject(s_Mutex, INFINITE);

	// Get system level path
	char* osname = FS_BuildOSPathUnMapped(name);
	
	// Generate hash for file name
	strlwr(osname);
	unsigned int code = crc32(0, (const byte *)osname, strlen(osname));
	
	// Check if the file exists
	if (!s_Files->Find(code))
	{
		ReleaseMutex(s_Mutex);
		return -1;
	}

	ReleaseMutex(s_Mutex);
	return code;
}

const char* Sys_GetFileCodeName(int code)
{
	WaitForSingleObject(s_Mutex, INFINITE);

	FileInfo *entry = s_Files->Find(code);
	if (entry)
	{
		ReleaseMutex(s_Mutex);
		return entry->name;
	}
	
	ReleaseMutex(s_Mutex);
	return NULL;
}

int Sys_GetFileCodeSize(int code)
{
	WaitForSingleObject(s_Mutex, INFINITE);

	FileInfo *entry = s_Files->Find(code);
	if (entry)
	{
		ReleaseMutex(s_Mutex);
		return entry->size;
	}
	
	ReleaseMutex(s_Mutex);
	return -1;
}

// Quick function to re-scan for new files, update the filecode
// table, and dump the new one to disk
void Sys_FilecodeScan_f( void )
{
	// Make an updated filecode cache
	if( !Sys_SaveFileCodes() )
		Com_Error( ERR_DROP, "ERROR: Couldn't create filecode cache\n" );

	// Throw out our current list
	Sys_ShutdownFileCodes();

	// Re-init, which should use the new list we just made
	Sys_InitFileCodes();
}
