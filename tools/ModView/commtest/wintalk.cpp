// Filename:-	wintalk.cpp
//
// a module containing code for talking to other windows apps via a shared memory space...
//
#include "stdafx.h"
#include <assert.h>
#include "commtestDoc.h"
#include "commtestview.h"
//#include <winbase.h>
#include "../tools/modview/CommArea.h"
//
#include "../tools/modview/wintalk.h"



#ifndef ZEROMEM
#define ZEROMEM(blah) memset(&blah,0,sizeof(blah))
#endif



void ErrorBox(const char *sString)
{
	MessageBox( NULL, sString, "Error",		MB_OK|MB_ICONERROR|MB_TASKMODAL );		
}
void InfoBox(const char *sString)
{
	MessageBox( NULL, sString, "Info",		MB_OK|MB_ICONINFORMATION|MB_TASKMODAL );		
}
void WarningBox(const char *sString)
{
	MessageBox( NULL, sString, "Warning",	MB_OK|MB_ICONWARNING|MB_TASKMODAL );
}

char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[16][1024];
	static index = 0;

	index = (++index)&15;
	
	va_start (argptr, format);
	vsprintf (string[index], format,argptr);
	va_end (argptr);

	return string[index];	
}


// this'll return a string of up to the first 4095 chars of a system error message...
//
LPCSTR SystemErrorString(DWORD dwError)
{
	static char sString[4096];

	LPVOID lpMsgBuf=0;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);		

	ZEROMEM(sString);
	strncpy(sString,(LPCSTR) lpMsgBuf,sizeof(sString)-1);

	LocalFree( lpMsgBuf ); 

	return sString;
}


//void SystemErrorBox(DWORD dwError)

extern HWND g_hWnd;
#define GetYesNo(psQuery)	(!!(MessageBox(g_hWnd,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))


//////////////////////////////////////////////////////////////////////////////


static LPCSTR FindWhitespace(LPCSTR psString)
{
	while (*psString && !isspace(*psString)) psString++;
	return psString;
}
static LPCSTR SkipWhitespace(LPCSTR psString)
{
	while (isspace(*psString)) psString++;
	return psString;
}



// every case here must either do CommArea_CommandAck(...) or CommArea_CommandError(...),
//	failure to do this is amazingly bad!!!
// 
static void HandleCommands(LPCSTR psString, byte *pbCommandData, int iCommandDataSize)
{
/*
#define IF_ARG(string)	if (!strncmp(psArg,string,strlen(string)))
#define NEXT_ARG		SkipWhitespace(FindWhitespace(psArg))

	LPCSTR psArg = psString;
		
	IF_ARG("model_loadprimary")
	{
		psArg = NEXT_ARG;

		if (Document_ModelLoadPrimary( psArg ))
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"model_loadprimary %s\"\"", psArg));
		}
	}
	else				
	IF_ARG("modeltree_getrootsurface")
	{
		// for this command, just send back whatever the answer is without validating...
		//
		HTREEITEM hTreeItem = ModelTree_GetRootSurface(Model_GetPrimaryHandle());
		
		CommArea_CommandAck(va("%d",hTreeItem));
	}
	else				
	IF_ARG("modeltree_getrootbone")
	{
		// for this command, just send back whatever the answer is without validating...
		//
		HTREEITEM hTreeItem = ModelTree_GetRootBone(Model_GetPrimaryHandle());

		CommArea_CommandAck(va("%d",hTreeItem));
	}
	else
	// this version MUST be the first of the two, or the shorter one will early-match even a long command
	IF_ARG("modeltree_getitemtextpure")	// "...pure" will skip stuff like "////" for disabled surfaces
	{
		psArg = NEXT_ARG;

		HTREEITEM	hTreeItem	= (HTREEITEM) atoi(psArg);
		LPCSTR		psText		= ModelTree_GetItemText(hTreeItem,true);

		CommArea_CommandAck(psText);
	}
	else
	IF_ARG("modeltree_getitemtext")
	{
		psArg = NEXT_ARG;

		HTREEITEM	hTreeItem	= (HTREEITEM) atoi(psArg);
		LPCSTR		psText		= ModelTree_GetItemText(hTreeItem);

		CommArea_CommandAck(psText);
	}
	else
	IF_ARG("modeltree_getchilditem")
	{
		psArg = NEXT_ARG;

		HTREEITEM	hTreeItem = (HTREEITEM) atoi(psArg);
					hTreeItem = ModelTree_GetChildItem(hTreeItem);

		CommArea_CommandAck(va("%d",hTreeItem));
	}
	else
	IF_ARG("modeltree_getnextsiblingitem")
	{
		psArg = NEXT_ARG;

		HTREEITEM	hTreeItem = (HTREEITEM) atoi(psArg);
					hTreeItem = ModelTree_GetNextSiblingItem(hTreeItem);

		CommArea_CommandAck(va("%d",hTreeItem));
	}
	else
	{
		// unknown command...
		//		
		CommArea_CommandError(va("ModView: Unknown command \"%s\"", psString));
	}
*/

	InfoBox(va("CommsTest: Command recieved: \"%s\"",psString));

	if (GetYesNo("CommsTest: Report as error? ('NO' will report success)"))
	{
		CommArea_CommandError(va("CommsTest: Failed to complete task '%s'", psString));
	}
	else
	{					
		CommArea_CommandAck();
	}
}


static bool bDoNotEnterHandler = false;	// Modal dialogue boxes can cause problems with re-entrance
void WinTalk_HandleMessages(void)
{	
	if (!bDoNotEnterHandler)
	{
		bDoNotEnterHandler = true;

		if (!CommArea_IsIdle())
		{
			CWaitCursor wait;

			byte	*pbCommandData = NULL;
			int		iCommandDataSize;
			LPCSTR	psString;
			
			if ((psString = CommArea_IsCommandWaiting( &pbCommandData, &iCommandDataSize )) != NULL)
			{
				HandleCommands(psString, pbCommandData, iCommandDataSize);
			}
			else
			if ((psString = CommArea_IsErrorWaiting()) != NULL)
			{
				assert(0);	// I don't think we should ever get here, but just in case...
				ErrorBox(va("CommTest: Other program reported an error:\n\n\"%s\"",psString));
				CommArea_CommandClear();
			}
		}

		bDoNotEnterHandler = false;
	}
}


// return is success / fail
//
bool WinTalk_IssueCommand(	LPCSTR psCommand, 
							byte *pbData,				// optional extra command data (current max = 64k)
							int iDataSize,				// sizeof above
							LPCSTR *ppsResultPassback,	// optional result passback if expecting an answer string
							byte **ppbDataPassback,		// optional data passback if expecting a data result
							int *piDataSizePassback		// optional size of above if you need it supplying
							)
{
	bool bError = false;

	while (!CommArea_IsIdle())
	{
		WinTalk_HandleMessages();
	}

	bDoNotEnterHandler = true;
	{
		CommArea_IssueCommand(psCommand, pbData, iDataSize);

		// wait until command is acknowledged or has failed...  (you may want a sanity check timeout?)
		//
		CWaitCursor wait;

		while (1)
		{
			LPCSTR psReply;

			if ((psReply = CommArea_IsAckWaiting(ppbDataPassback,piDataSizePassback)) != NULL)
			{
				if ( ppsResultPassback)
					*ppsResultPassback = psReply;

				bError = false;
				break;
			}

			if ((psReply = CommArea_IsErrorWaiting()) != NULL)
			{
				ErrorBox(va("Other program reported an error:\n\n\"%s\"",psReply));			
				bError = true;
				break;
			}

			Sleep(0);	// needed to avoid hogging all CPU time :-)
		}

		// you MUST do this...
		//
		CommArea_CommandClear();



/*		{
			gpCommTestView->GetTreeCtrl().DeleteAllItems();
			gpCommTestView->GetTreeCtrl().InsertItem("hello");
		}
*/
	}
	bDoNotEnterHandler = false;

	return !bError;
}





////////////////////// eof ////////////////


