// Filename:-	wintalk.cpp
//
// a module containing code for talking to other windows apps via a shared memory space...
//
#include "stdafx.h"
#include "includes.h"
//#include <winbase.h>
#include "CommArea.h"
//
#include "wintalk.h"


extern bool Document_ModelLoadPrimary(LPCSTR psFilename);


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
static void TrimFirstSpace(char *psString)
{
	*((char*)FindWhitespace(psString))='\0';
}

// every case here must either do CommArea_CommandAck(...) or CommArea_CommandError(...),
//	failure to do this is amazingly bad.
// 
// return TRUE = app exit requested
//
static bool HandleCommands(LPCSTR psString, byte *pbCommandData, int iCommandDataSize)
{
	static bool bAppExitWanted = false;

	if (bAppExitWanted)
		return true;	// refuse to listen to any more commands

#define IF_ARG(string)	if (!strncmp(psArg,string,strlen(string)))
#define NEXT_ARG		SkipWhitespace(FindWhitespace(psArg))
#define READ_ARG(_arg,_destbuffer)  {strncpy(_destbuffer,_arg,sizeof(_destbuffer)-1);_destbuffer[sizeof(_destbuffer)-1]='\0';TrimFirstSpace(_destbuffer);}

	LPCSTR psArg = psString;
		
	IF_ARG("model_loadprimary")
	{
		psArg = NEXT_ARG;

		if (Document_ModelLoadPrimary( psArg ))
		{
			CommArea_CommandAck(va("%d",AppVars.hModelLastLoaded));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("quit")
	{
		CommArea_CommandAck();
		bAppExitWanted = true;
	}
	else
	IF_ARG("model_loadbolton")	// <modelhandle to bolt to> <name of bolt point> <filename>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;
		
		char sBoltName[1024];
		READ_ARG(psArg,sBoltName);		

		psArg = NEXT_ARG;	// psFullPathedFilename

		if (Model_LoadBoltOn(psArg, hModel, sBoltName, true, true))	// bBoltIsBone, bBoltReplacesAllExisting
		{
			CommArea_CommandAck(va("%d",AppVars.hModelLastLoaded));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_addbolton")	// <modelhandle to bolt to> <name of bolt point> <filename>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;
		
		char sBoltName[1024];
		READ_ARG(psArg,sBoltName);		

		psArg = NEXT_ARG;	// psFullPathedFilename

		if (Model_LoadBoltOn(psArg, hModel, sBoltName, true, false))	// bBoltIsBone, bBoltReplacesAllExisting
		{
			CommArea_CommandAck(va("%d",AppVars.hModelLastLoaded));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_deletebolton")	// <modelhandle of bolted thing to delete>
	{
		psArg = NEXT_ARG;
		
		ModelHandle_t hModelBoltOn = (ModelHandle_t) atoi(psArg);
		
		if (Model_DeleteBoltOn(hModelBoltOn))
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_loadsurfacebolton")	// <modelhandle to bolt to> <name of bolt point> <filename>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;
		
		char sBoltName[1024];
		READ_ARG(psArg,sBoltName);		

		psArg = NEXT_ARG;	// psFullPathedFilename

		if (Model_LoadBoltOn(psArg, hModel, sBoltName, false, true))	// bBoltIsBone, bBoltReplacesAllExisting
		{
			CommArea_CommandAck(va("%d",AppVars.hModelLastLoaded));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_addsurfacebolton")	// <modelhandle to bolt to> <name of bolt point> <filename>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;
		
		char sBoltName[1024];
		READ_ARG(psArg,sBoltName);		

		psArg = NEXT_ARG;	// psFullPathedFilename

		if (Model_LoadBoltOn(psArg, hModel, sBoltName, false, false))	// bBoltIsBone, bBoltReplacesAllExisting
		{
			CommArea_CommandAck(va("%d",AppVars.hModelLastLoaded));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_getnumbonealiases")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		int iAliases = Model_GetNumBoneAliases(hModel);
		CommArea_CommandAck(va("%d",iAliases));
	}
	else
	IF_ARG("model_getbonealias")		// <modelhandle> <%d = index>  (answer is "realname" "aliasname")
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		int iAliasNum = atoi(psArg);

		string strReal,strAlias;
		if (Model_GetBoneAliasPair(hModel, iAliasNum, strReal, strAlias))
		{
			CommArea_CommandAck(va("%s %s",strReal.c_str(),strAlias.c_str()));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_getnumsequences")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		int iSequences = Model_GetNumSequences(hModel);
		CommArea_CommandAck(va("%d",iSequences));
	}
	else
	IF_ARG("model_getsequence")		// <modelhandle> <%d = sequencenum>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		int iSequenceNum = atoi(psArg);

		LPCSTR psString = Model_GetSequenceString(hModel, iSequenceNum);

		if (psString)
		{
			CommArea_CommandAck(va("%s",psString));
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_locksequence")	// <modelhandle> <%d = sequencenum>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		int iSequenceNum = atoi(psArg);

		bool bOk = Model_Sequence_Lock(hModel, iSequenceNum, true);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_locksequence_secondary")	// <modelhandle> <%d = sequencenum>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		int iSequenceNum = atoi(psArg);

		bool bOk = Model_Sequence_Lock(hModel, iSequenceNum, false);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_unlocksequences")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		bool bOk = Model_Sequence_UnLock(hModel, true);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_unlocksequences_secondary")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		bool bOk = Model_Sequence_UnLock(hModel, false);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_bonename_secondarystart")	// <modelhandle> <%s = bonename>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		bool bOk = Model_SetSecondaryAnimStart(hModel, psArg);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_clear_secondarystart")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		bool bOk = Model_SetSecondaryAnimStart(hModel, -1);

		if (bOk)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_highlightbone")	// <modelhandle> <%s = bonename, else "#all" / "#none" / "#aliased">
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;		

		bool bSuccess = false;

		if (!stricmp(psArg,"#aliased"))
		{
			bSuccess = Model_SetBoneHighlight(hModel, iITEMHIGHLIGHT_ALIASED);
		}
		else
		if (!stricmp(psArg,"#all"))
		{
			bSuccess = Model_SetBoneHighlight(hModel, iITEMHIGHLIGHT_ALL);
		}
		else
		if (!stricmp(psArg,"#none"))
		{
			bSuccess = Model_SetBoneHighlight(hModel, iITEMHIGHLIGHT_NONE);
		}
		else
		{
			bSuccess = Model_SetBoneHighlight(hModel, psArg);
		}

		if (bSuccess)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("model_highlightsurface")	// <modelhandle> <%s = surfacename, else "#all" or "#none">
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		psArg = NEXT_ARG;

		bool bSuccess = false;

		if (!stricmp(psArg,"#tags"))
		{
			bSuccess = Model_SetSurfaceHighlight(hModel, iITEMHIGHLIGHT_ALL_TAGSURFACES);
		}
		else
		if (!stricmp(psArg,"#all"))
		{
			bSuccess = Model_SetSurfaceHighlight(hModel, iITEMHIGHLIGHT_ALL);
		}
		else
		if (!stricmp(psArg,"#none"))
		{
			bSuccess = Model_SetSurfaceHighlight(hModel, iITEMHIGHLIGHT_NONE);
		}
		else
		{
			bSuccess = Model_SetSurfaceHighlight(hModel, psArg);
		}

		if (bSuccess)
		{
			CommArea_CommandAck();
		}
		else
		{
			CommArea_CommandError(va("ModView: Failed command: \"%s\"", psString));
		}
	}
	else
	IF_ARG("modeltree_getrootsurface")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		// for this command, just send back whatever the answer is without validating...
		//
		HTREEITEM hTreeItem = ModelTree_GetRootSurface(hModel);
		
		CommArea_CommandAck(va("%d",hTreeItem));
	}
	else				
	IF_ARG("modeltree_getrootbone")	// <modelhandle>
	{
		psArg = NEXT_ARG;

		ModelHandle_t hModel = (ModelHandle_t) atoi(psArg);

		// for this command, just send back whatever the answer is without validating...
		//
		HTREEITEM hTreeItem = ModelTree_GetRootBone(hModel);

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
	IF_ARG("errorbox_disable")
	{
		gbErrorBox_Inhibit = true;
		CommArea_CommandAck();
	}
	else
	IF_ARG("errorbox_enable")
	{
		gbErrorBox_Inhibit = false;
		CommArea_CommandAck();
	}
	else
	IF_ARG("getlasterror")
	{
		CommArea_CommandAck(ModView_GetLastError());
	}
	else
	IF_ARG("startanimwrap")	// must be BEFORE "startanim" because of strncmp()!
	{
		Model_StartAnim(true);
		CommArea_CommandAck();
	}
	else
	IF_ARG("startanim")
	{
		Model_StartAnim();
		CommArea_CommandAck();
	}
	else
	IF_ARG("stopanim")
	{
		Model_StopAnim();
		CommArea_CommandAck();
	}
	else
	{
		// unknown command...
		//		
		CommArea_CommandError(va("ModView: Unknown command \"%s\"", psString));
	}

	return bAppExitWanted;
}




static bool bDoNotEnterHandler = false;

// return TRUE = app exit wanted!!!
//
bool WinTalk_HandleMessages(void)
{	
	bool bAppExitWanted = false;
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
				bAppExitWanted = HandleCommands(psString, pbCommandData, iCommandDataSize);
			}
			else
			if ((psString = CommArea_IsErrorWaiting()) != NULL)
			{
				assert(0);	// i don't think we should ever get here, but just in case...
				ErrorBox(va("ModView: Other program reported an error:\n\n\"%s\"",psString));
				CommArea_CommandClear();
			}
		}

		bDoNotEnterHandler = false;
	}

	return bAppExitWanted;
}


// return is success/fail
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

	bDoNotEnterHandler = false;
	{

		CommArea_IssueCommand(psCommand, pbData, iDataSize);

		// wait until command is acknowledged or has failed...  (you may want a sanity check timeout?)
		//
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

	}
	bDoNotEnterHandler = false;

	return !bError;
}






////////////////////// eof ////////////////


