
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

#include "../game/q_shared.h"
#include "win_file.h"
#include "../qcommon/qcommon.h"

#ifdef _XBOX
#include <Xtl.h>
#endif

#ifdef _WINDOWS
#include <windows.h>
#endif


struct FileTable
{
	bool m_bUsed;
	bool m_bErrorsFatal;
	HANDLE m_Handle;
};

FileTable* s_FileTable = NULL;
const int WF_MAX_OPEN_FILES = 8;

void WF_Init(void)
{
	assert(!s_FileTable);

	s_FileTable = new FileTable[WF_MAX_OPEN_FILES];

	for (wfhandle_t i = 0; i < WF_MAX_OPEN_FILES; ++i)
	{
		s_FileTable[i].m_bUsed = false;
	}
}

void WF_Shutdown(void)
{
	assert(s_FileTable);
	
	for (wfhandle_t i = 0; i < WF_MAX_OPEN_FILES; ++i)
	{
		if (s_FileTable[i].m_bUsed)
		{
			WF_Close(i);
		}
	}

	delete [] s_FileTable;
	s_FileTable = NULL;
}

static wfhandle_t WF_GetFreeHandle(void)
{
	for (int i = 0; i < WF_MAX_OPEN_FILES; ++i)
	{
		if (!s_FileTable[i].m_bUsed)
		{
			return i;
		}
	}

	return -1;
}

int WF_Open(const char* name, bool read, bool aligned)
{
	wfhandle_t handle = WF_GetFreeHandle();
	if (handle == -1) return -1;
	
	s_FileTable[handle].m_Handle = 
		CreateFile(name, read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ, 0, 
		read ? OPEN_EXISTING : OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL |(aligned ? FILE_FLAG_NO_BUFFERING : 0) , 0);

	if (s_FileTable[handle].m_Handle != INVALID_HANDLE_VALUE)
	{
		s_FileTable[handle].m_bUsed = true;
		
		// errors are fatal on game partition
		s_FileTable[handle].m_bErrorsFatal = (name[0] == 'D' || name[0] == 'd');
		
		return handle;
	}
	
	return -1;
}

void WF_Close(wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	CloseHandle(s_FileTable[handle].m_Handle);
	s_FileTable[handle].m_bUsed = false;
}

int WF_Read(void* buffer, int len, wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	DWORD bytes;
	if (!ReadFile(s_FileTable[handle].m_Handle, buffer, len, &bytes, 0) &&
		s_FileTable[handle].m_bErrorsFatal)
	{
#if defined(FINAL_BUILD)
		extern void ERR_DiscFail(bool);
		ERR_DiscFail(false);
#else
		assert(0);
#endif
	}

	return bytes;
}

int WF_Write(const void* buffer, int len, wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	DWORD bytes;
	WriteFile(s_FileTable[handle].m_Handle, buffer, len, &bytes, 0);
	return bytes;
}

int WF_Seek(int offset, int origin, wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	switch (origin)
	{
	case SEEK_CUR: origin = FILE_CURRENT; break;
	case SEEK_END: origin = FILE_END; break;
	case SEEK_SET: origin = FILE_BEGIN; break;
	default: assert(false);
	}

	return SetFilePointer(s_FileTable[handle].m_Handle, offset, 0, origin) < 0;
}

int WF_Tell(wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	return SetFilePointer(s_FileTable[handle].m_Handle, 0, 0, FILE_CURRENT);
}

int WF_Resize(int size, wfhandle_t handle)
{
	assert(handle >= 0 && handle < WF_MAX_OPEN_FILES && 
		s_FileTable[handle].m_bUsed);

	SetFilePointer(s_FileTable[handle].m_Handle, size, NULL, FILE_BEGIN);
	return SetEndOfFile(s_FileTable[handle].m_Handle);
}
