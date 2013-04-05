
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
#include "win_local.h"
#include "../qcommon/qcommon.h"

#if defined(_WINDOWS)
#include <windows.h>
#elif defined(_XBOX)
#include <xtl.h>
#endif

extern void Z_SetNewDeleteTemporary(bool);

#define STREAM_SLOW_READ 0

#include "../client/snd_local_console.h"

#include <deque>

extern HANDLE Sys_FileStreamMutex;
extern const char* Sys_GetFileCodeName(int code);
extern int Sys_GetFileCodeSize(int code);

#define STREAM_MAX_OPEN 48
struct StreamInfo
{
	volatile HANDLE file;
	volatile bool used;
	volatile bool error;
	volatile bool opening;
	volatile bool reading;
};
static StreamInfo* s_Streams = NULL;

enum IORequestType
{
	IOREQ_OPEN,
	IOREQ_READ,
	IOREQ_SHUTDOWN,
};

struct IORequest
{
	IORequestType type;
	streamHandle_t handle;
	DWORD data[3];
};
typedef std::deque<IORequest> requestqueue_t;
requestqueue_t* s_IORequestQueue = NULL;

HANDLE s_Thread = INVALID_HANDLE_VALUE;
HANDLE s_QueueMutex = INVALID_HANDLE_VALUE;
HANDLE s_QueueLen = INVALID_HANDLE_VALUE;


static DWORD WINAPI _streamThread(LPVOID)
{
	for (;;)
	{
		IORequest req;
		DWORD bytes;
		StreamInfo* strm;

		// Wait for the IO queue to fill
		WaitForSingleObject(s_QueueLen, INFINITE);
		
		// Grab the next IO request
		WaitForSingleObject(s_QueueMutex, INFINITE);
		assert(!s_IORequestQueue->empty());
		req = s_IORequestQueue->front();
		s_IORequestQueue->pop_front();
		ReleaseMutex(s_QueueMutex);

		// Process request
		switch (req.type)
		{
		case IOREQ_OPEN:
			strm = &s_Streams[req.handle];
			assert(strm->used);
		
			{
				const char* name = Sys_GetFileCodeName(req.data[0]);

				WaitForSingleObject(Sys_FileStreamMutex, INFINITE);
			
				strm->file = 
					CreateFile(name, GENERIC_READ, 
					FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			
				ReleaseMutex(Sys_FileStreamMutex);
			}

			strm->error = (strm->file == INVALID_HANDLE_VALUE);
			strm->opening = false;
			break;

		case IOREQ_READ:
			strm = &s_Streams[req.handle];
			assert(strm->used);
		
			WaitForSingleObject(Sys_FileStreamMutex, INFINITE);

#			if STREAM_SLOW_READ
			Sleep(200);
#			endif

			strm->error = 
				(SetFilePointer(strm->file, req.data[2], 0, FILE_BEGIN) != req.data[2] ||
				ReadFile(strm->file, (void*)req.data[0], req.data[1], &bytes, NULL) == 0);
			
			ReleaseMutex(Sys_FileStreamMutex);

			strm->reading = false;
			break;

		case IOREQ_SHUTDOWN:
			ExitThread(0);
			break;
		}
	}

	return TRUE;
}


static void _sendIORequest(const IORequest& req)
{
	// Add request to queue
	WaitForSingleObject(s_QueueMutex, INFINITE);
	Z_SetNewDeleteTemporary(true);
	s_IORequestQueue->push_back(req);
	Z_SetNewDeleteTemporary(false);
	ReleaseMutex(s_QueueMutex);

	// Let IO thread know it has one more pending request
	ReleaseSemaphore(s_QueueLen, 1, NULL);
}

void Sys_IORequestQueueClear(void)
{
	WaitForSingleObject(s_QueueMutex, INFINITE);
	delete s_IORequestQueue;
	s_IORequestQueue = new requestqueue_t;
	ReleaseMutex(s_QueueMutex);
}

void Sys_StreamInit(void)
{
	// Create array for storing open streams
	s_Streams = (StreamInfo*)Z_Malloc(
		STREAM_MAX_OPEN * sizeof(StreamInfo), TAG_FILESYS, qfalse);
	for (int i = 0; i < STREAM_MAX_OPEN; ++i)
	{
		s_Streams[i].used = false;
	}

	// Create queue to hold requests for IO thread
	s_IORequestQueue = new requestqueue_t;

	// Create a thread to service IO
	s_QueueMutex = CreateMutex(NULL, FALSE, NULL);
	s_QueueLen = CreateSemaphore(NULL, 0, STREAM_MAX_OPEN * 3, NULL);
	s_Thread = CreateThread(NULL, 64*1024, _streamThread, 0, 0, NULL);
}

void Sys_StreamShutdown(void)
{
	// Tell the IO thread to shutdown
	IORequest req;
	req.type = IOREQ_SHUTDOWN;
	_sendIORequest(req);

	// Wait for thread to close
	WaitForSingleObject(s_Thread, INFINITE);
	
	// Kill IO thread
	CloseHandle(s_Thread);
	CloseHandle(s_QueueLen);
	CloseHandle(s_QueueMutex);

	// Remove queue of IO requests
	delete s_IORequestQueue;
	
	// Remove streaming table
	Z_Free(s_Streams);
}

static streamHandle_t GetFreeHandle(void)
{
	for (streamHandle_t i = 1; i < STREAM_MAX_OPEN; ++i)
	{
		if (!s_Streams[i].used) return i;
	}
	
	// handle 0 is invalid by convention
	return 0;
}

int Sys_StreamOpen(int code, streamHandle_t *handle)
{
	// Find a free handle
	*handle = GetFreeHandle();
	if (*handle == 0)
	{
		return -1;
	}

	// Find the file size
	int size = Sys_GetFileCodeSize(code);
	if (size < 0)
	{
		*handle = 0;
		return -1;
	}

	// Init stream data
	s_Streams[*handle].used = true;
	s_Streams[*handle].opening = true;
	s_Streams[*handle].reading = false;
	s_Streams[*handle].error = false;

	// Send an open request to the thread
	IORequest req;
	req.type = IOREQ_OPEN;
	req.handle = *handle;
	req.data[0] = code;
	_sendIORequest(req);

	// Return file size
	return size;
}

bool Sys_StreamRead(void* buffer, int size, int pos, streamHandle_t handle)
{
	assert((unsigned int)buffer % 32 == 0);

	// Handle must be valid.  Do not allow multiple reads.
	if (!s_Streams[handle].used || s_Streams[handle].reading) return false;

	// Ready to read
	s_Streams[handle].reading = true;
	s_Streams[handle].error = false;
	
	// Request IO threading reading
	IORequest req;
	req.type = IOREQ_READ;
	req.handle = handle;
	req.data[0] = (DWORD)buffer;
	req.data[1] = size;
	req.data[2] = pos;
	_sendIORequest(req);
	
	return true;
}

bool Sys_StreamIsReading(streamHandle_t handle)
{
	return s_Streams[handle].used && s_Streams[handle].reading;
}

bool Sys_StreamIsError(streamHandle_t handle)
{
	return s_Streams[handle].used && s_Streams[handle].error;
}

void Sys_StreamClose(streamHandle_t handle)
{
	if (s_Streams[handle].used)
	{
		// Block until read is done
		while (s_Streams[handle].opening || s_Streams[handle].reading);
		
		// Close the file
		CloseHandle(s_Streams[handle].file);
		s_Streams[handle].used = false;
	}
}
