// Filename:-	CommArea.cpp
//
// contains low-level code for shared-memory inter-program comms
//
#include "stdafx.h"
#include "includes.h"
//#include <winbase.h>
//
#include "CommArea.h"



#define sCOMMAREA_NAME		"COMMAREA_MODVIEW"
#define iCOMMAREA_VERSION	1

typedef enum
{
	cst_READY=0,	// nothing pending
	cst_SERVERREQ,	// server has request for client
	cst_CLIENTREQ,	// client has request for server
	cst_CLIENTERR,	// client failed a request, check PassedData.sError[]
	cst_SERVERERR,	// server failed a request, check PassedData.sError[]
	cst_WAIT,		// raised by either side while filling in details of stuff
	//
	// these last 2 I could probably get rid of and replace by just setting back to cst_READY, but for now...
	//
	cst_CLIENTOK,	// client completed a request	
	cst_SERVEROK,	// server completed a request

} CommStatus_e;


typedef struct
{
	// message passing... (arb. buffer sizes, can be increased if you bump the version number as well)
	//
	char			sCommand[1024];	// used to pass text command (case insensitive)
	byte			bData[65536];	// 
	int				iDataSize;
	// other...
	//
	char			sError[4096];	// used for reporting problems

} PassedData_t;


typedef struct
{
	// verification fields...
	//
	int				iVersion;		// version of this struct, should match in client and server
	int				iSize;			// size of this struct, belt and braces safety
	//
	// the semaphore field...
	//
	CommStatus_e	eStatus;

	// message passing... (arb. buffer sizes, can be increased if you bump the version number as well)
	//
	PassedData_t	PassedData;

} CommArea_t;


static CommArea_t	CommArea_USEONLYDURINGINIT, *gpMappedCommArea = NULL;
static HANDLE		hFileMap	= NULL;
static bool			bIAmServer	= false;

// special copy that I can put all recieved data into so that the acknowledging a message doesn't let the other
//	app zap the results with a new message while you're busy copying the data elsewhere...
//
static PassedData_t	LocalData;



// by having a common string at the front of my error strings I can allow the caller to detect
//	whether they're messages produced by me or by Windows (and allow for stripping the common piece if desired)
//
#define sERROR_COMMAREAUNINITIALISED	"CommArea: function called while comms not initialised, or init failed"
#define sERROR_COMMANDSTRINGTOOLONG		"CommArea: command string too long"
#define sERROR_COMMANDDATATOOLONG		"CommArea: command data too long"
#define sERROR_BADCOMMANDACKTIME		"CommArea: command acknowledge called but no command was pending"
#define sERROR_BADCOMMANDCLEARTIME		"CommArea: command-clear called while no error or ack pending - commands lost?"
#define sERROR_NOTASKPENDING			"CommArea: error report attempted while task not pending"
#define sERROR_BUSY						"CommArea: busy"


static PassedData_t *CommArea_LocaliseData(void)
{
	LocalData = gpMappedCommArea->PassedData;
	return &LocalData;
}


// buffer-size query for caller-app to setup args legally (if wanting to send big commands and unsure of space)...
//
int CommArea_GetMaxDataSize(void)
{
	return sizeof(LocalData.bData);
}
int CommArea_GetMaxCommandStrlen(void)
{
	return sizeof(LocalData.sCommand);
}
// getting this size wrong while sending error strings will never cause problems, they're just clipped
//
int CommArea_GetMaxErrorStrlen(void)
{
	return sizeof(LocalData.sError);
}


void CommArea_ShutDown(void)
{
	if (gpMappedCommArea)
	{
		UnmapViewOfFile( gpMappedCommArea );
		gpMappedCommArea = NULL;
	}
	 
	if (hFileMap)
	{
		CloseHandle(hFileMap);
		hFileMap = NULL;
	}
}


static LPCSTR CommArea_MapViewOfFile(void)
{
	gpMappedCommArea = (CommArea_t *) MapViewOfFile(hFileMap,			// HANDLE hFileMappingObject,  // file-mapping object to map into 
													FILE_MAP_ALL_ACCESS,// DWORD dwDesiredAccess,      // access mode
													0,					// DWORD dwFileOffsetHigh,     // high-order 32 bits of file offset
													0,					// DWORD dwFileOffsetLow,      // low-order 32 bits of file offset
													0					// DWORD dwNumberOfBytesToMap  // number of bytes to map
													);

	return gpMappedCommArea ? NULL : SystemErrorString();
}




// return is either error message, or NULL for success...
//
LPCSTR CommArea_ServerInitOnceOnly(void)
{
	LPCSTR psError = NULL;

	hFileMap = CreateFileMapping(	INVALID_HANDLE_VALUE,	// HANDLE hFile	 // handle to file to map
									NULL,					// LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
									PAGE_READWRITE,			// DWORD flProtect,	// protection for mapping object
									0,						// DWORD dwMaximumSizeHigh,   // high-order 32 bits of object size
									sizeof(CommArea_USEONLYDURINGINIT),		// DWORD dwMaximumSizeLow,    // low-order 32 bits of object size
									sCOMMAREA_NAME			// LPCTSTR lpName             // name of file-mapping object
									);

	DWORD dwError = GetLastError();
	if (hFileMap == NULL || dwError == ERROR_ALREADY_EXISTS)
	{
		psError = SystemErrorString(dwError);
	}
	else
	{
		// ok so far, so get a map view of it...
		//
		psError = CommArea_MapViewOfFile();

		if (!psError)
		{
			// Yeeehaaa....  let's init this baby...
			//
			ZEROMEMPTR(gpMappedCommArea);

			gpMappedCommArea->iVersion	= iCOMMAREA_VERSION;
			gpMappedCommArea->iSize		= sizeof(*gpMappedCommArea);

			bIAmServer = true;
		}
	}

	if (psError)
	{
		CommArea_ShutDown();
	}

	return psError;
}
 


// return is either error message, or NULL for success...
//
LPCSTR CommArea_ClientInitOnceOnly(void)
{
	LPCSTR psError = NULL;

	hFileMap = OpenFileMapping(	FILE_MAP_ALL_ACCESS,	// DWORD dwDesiredAccess,  // access mode
								true,					// BOOL bInheritHandle,    // inherit flag
								sCOMMAREA_NAME			// LPCTSTR lpName          // pointer to name of file-mapping object
								);

	if (!hFileMap)
	{
		psError = SystemErrorString();
	}
	else
	{
		// ok so far, so get a map view of it...
		//
		psError = CommArea_MapViewOfFile();

		if (!psError)
		{
			// let's give it a quick check for version differences...
			//
			static char sError[1024];

			if (gpMappedCommArea->iVersion != iCOMMAREA_VERSION)
			{
				sprintf(sError,"CommArea version # mismatch, found %d but expected %d!",gpMappedCommArea->iVersion, iCOMMAREA_VERSION);
				psError = sError;
			}
			else
			{
				if (gpMappedCommArea->iSize != sizeof(*gpMappedCommArea))
				{
					sprintf(sError,"CommArea struct size mismatch, found %d but expected %d!",gpMappedCommArea->iSize, sizeof(*gpMappedCommArea));
					psError = sError;
				}
				else
				{
					// Yeeehaaa....  everything ok...
					//
			  		bIAmServer = false;	// unnec. but clearer
				}
			}
		}
	}

	if (psError)
	{
		CommArea_ShutDown();
	}

	return psError;
}



// can be safely called even when init failed...
//
bool CommArea_IsIdle(void)
{
	if (!gpMappedCommArea)
		return true;

	return	gpMappedCommArea->eStatus == cst_READY
			/*
			|| 
			gpMappedCommArea->eStatus == cst_CLIENTOK ||
			gpMappedCommArea->eStatus == cst_SERVEROK
			*/
			;
}



// call to query if any new commands are waiting for us, and return local (non-shared) ptrs if so.
//
// return = command string, else NULL for none
//
// This can be safely called even if the OnceOnlyInit call failed
//
LPCSTR CommArea_IsCommandWaiting(byte **ppbDataPassback, int *piDatasizePassback)
{
	assert(ppbDataPassback);
	assert(piDatasizePassback);

	if (gpMappedCommArea && gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTREQ : cst_SERVERREQ) )
	{
		// make local (non-shared) copy of the data so we can access via ptrs without next command overwriting it...
		//
		CommArea_LocaliseData();
		*ppbDataPassback	= &LocalData.bData[0];
		*piDatasizePassback	=  LocalData.iDataSize;
		return LocalData.sCommand;
	}

	return NULL;
}


// call this only (and always) after you send a command to tell you when the other app has finished responding to it
//
// return = response string (may be blank), else NULL for none  (optional data fields are filled in if supplied)
//
LPCSTR CommArea_IsAckWaiting(byte **ppbDataPassback /* = NULL */, int *piDatasizePassback /* = NULL */)
{		
	assert(gpMappedCommArea);

	if (gpMappedCommArea && gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTOK : cst_SERVEROK) )
	{
		// make local (non-shared) copy of the data so we can access via ptrs without next command overwriting it...
		//
		CommArea_LocaliseData();
		if ( ppbDataPassback)
			*ppbDataPassback	= &LocalData.bData[0];
		if ( piDatasizePassback)
			*piDatasizePassback	= LocalData.iDataSize;
		return LocalData.sCommand;
	}

	return NULL;
}

// check outgoing data for size-exceeding, and copy to send buffer if everything ok...
//
// return = errmess or NULL for ok
//
static LPCSTR CommArea_SetupAndLegaliseOutgoingData(LPCSTR psCommand, byte *pbData, int iDataSize)
{
	if (!gpMappedCommArea)
		return sERROR_COMMAREAUNINITIALISED;

	if (psCommand && strlen(psCommand) > sizeof(gpMappedCommArea->PassedData.sCommand)-1)
		return sERROR_COMMANDSTRINGTOOLONG;

	if (pbData && iDataSize > sizeof(gpMappedCommArea->PassedData.bData))
		return sERROR_COMMANDDATATOOLONG;


	// seems ok, so copy it to outgoing...
	//
	CommStatus_e ePrevStatus =	gpMappedCommArea->eStatus;
	gpMappedCommArea->eStatus = cst_WAIT;
	{
		if (psCommand)
		{
			strcpy(gpMappedCommArea->PassedData.sCommand,psCommand);
		}
		else
		{
			strcpy(gpMappedCommArea->PassedData.sCommand,"");
		}

		if (pbData)
		{
			memcpy(gpMappedCommArea->PassedData.bData, pbData, iDataSize);
		}
		else
		{
			//ZEROMEM(gpMappedCommArea->PassedData.bData);	// not actually necessary
		}

		gpMappedCommArea->PassedData.iDataSize = iDataSize;
	}
	gpMappedCommArea->eStatus = ePrevStatus;

	return NULL;
}

// you can ignore the return code from this if you only call it sensibly, ie when you know you've just completed a
//	command task
//
// NOTE: if there's an error then the acknowledge is NOT sent!!!!
//
LPCSTR CommArea_CommandAck(LPCSTR psCommand /* = NULL */, byte *pbData /* = NULL */, int iDataSize /* = 0 */)
{
	assert(gpMappedCommArea );

	if (gpMappedCommArea && gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTREQ : cst_SERVERREQ) )
	{
		LPCSTR psError = CommArea_SetupAndLegaliseOutgoingData(psCommand, pbData, iDataSize);
		if (psError)
		{
			assert(0);
			return psError;
		}

		gpMappedCommArea->eStatus = (bIAmServer ? cst_SERVEROK : cst_CLIENTOK);
		return NULL;
	}
	
	assert(0);
	return sERROR_BADCOMMANDACKTIME;
}


// Call this to query for any pending error messages from the other program
//
// return = error, else NULL for none
//
// This can be safely called even if the OnceOnlyInit call failed
//
LPCSTR CommArea_IsErrorWaiting(void)
{		
	if (gpMappedCommArea && gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTERR : cst_SERVERERR) )
	{
		return CommArea_LocaliseData()->sError;
	}

	return NULL;
}


// Only call this after you've sent a command, then received either an acknowledgement or an error
//
// You can ignore the return error from this if you only call it sensibly, ie when you know there was an error
//	that wanted displaying...
//
LPCSTR CommArea_CommandClear(void)
{
	assert(gpMappedCommArea );		

	if (gpMappedCommArea && 
			(
			gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTERR : cst_SERVERERR)
			||
			gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTOK : cst_SERVEROK)
			)
		)
	{
		gpMappedCommArea->eStatus = cst_READY;
		return NULL;
	}

	return sERROR_BADCOMMANDCLEARTIME;
}


// Call to report an error, this should only be done on receipt of a command that your app had a problem obeying...
//
// You can ignore the return code from this if you only call it sensibly, ie when you know you're just reporting an error
//
// NOTE: if there's an error (like you're calling this at the wrong time) then this error is NOT sent!!!!
//
LPCSTR CommArea_CommandError(LPCSTR psError)
{
	assert(gpMappedCommArea);

	if (gpMappedCommArea && gpMappedCommArea->eStatus == (bIAmServer ? cst_CLIENTREQ : cst_SERVERREQ) )
	{
		gpMappedCommArea->eStatus = cst_WAIT;
		{
			strncpy(gpMappedCommArea->PassedData.sError, psError, sizeof(gpMappedCommArea->PassedData.sError)-1);
			gpMappedCommArea->PassedData.sError[sizeof(gpMappedCommArea->PassedData.sError)-1] = '\0';
		}
		gpMappedCommArea->eStatus = (bIAmServer ? cst_SERVERERR : cst_CLIENTERR);
		return NULL;
	}

	return sERROR_NOTASKPENDING;
}


// return NULL = success, else error message...  (pbData can be NULL if desired, ditto iDataSize)
//
LPCSTR CommArea_IssueCommand(LPCSTR psCommand, byte *pbData /* = NULL */, int iDataSize /* = 0 */)
{
	if (!CommArea_IsIdle())
		return sERROR_BUSY;

	assert(gpMappedCommArea);
	if (!gpMappedCommArea)
		return sERROR_COMMAREAUNINITIALISED;

	LPCSTR psError = CommArea_SetupAndLegaliseOutgoingData(psCommand, pbData, iDataSize);
	if (psError)
	{
		assert(0);
		return psError;
	}

	gpMappedCommArea->eStatus = (bIAmServer ? cst_SERVERREQ : cst_CLIENTREQ);
	return NULL;
}


////////////////////// eof ////////////////


