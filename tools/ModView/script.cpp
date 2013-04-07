// Filename:-	script.cpp
//
// file to control model-loading scripts (scripts are purely a viewer convenience for multiple-bolt stuff)
//


#include "stdafx.h"
#include "includes.h"
#include "ModViewTreeView.h"
#include "glm_code.h"
#include "R_Model.h"
#include "R_Surface.h"
#include "textures.h"
#include "TEXT.H"
#include "sequence.h"
#include "model.h"
#include "GenericParser2.h"
//
#include "script.h"




CGenericParser2	theParser;



LPCSTR	Script_GetExtension(void)
{
	return sSCRIPT_EXTENSION;
}


LPCSTR	Script_GetFilter(bool bStandAlone /* = true */)
{
	static char sFilterString[1024];

	strcpy(sFilterString, "ModView script files (*" sSCRIPT_EXTENSION ")|*" sSCRIPT_EXTENSION "|");

	strcat(sFilterString,bStandAlone?"|":"");
	return sFilterString;
}


/*
bool CString_ExtractLine(CString &strSrc, CString &strDst)
{
	int iLoc = strSrc.Find("\n");
	if (iLoc!= -1)
	{
		strDst = strSrc.Left(iLoc);
		strSrc = strSrc.Mid(iLoc+1);
	}

	return (iLoc!=-1);
}

bool CString_ExtractUntil(CString &strSrc, CString &strDst, char cSeperator)
{
	int iLoc = strSrc.Find(cSeperator);
	if (iLoc!= -1)
	{
		strDst = strSrc.Left(iLoc);
		strSrc = strSrc.Mid(iLoc+1);
	}

	return (iLoc!=-1);
}
*/

typedef map<ModelContainer_t*, string>	HandleXref_t;
										HandleXref_t HandleXRefs;

static CString strHandlesSoFar;		// semicolon-delineated string of all handle names, for quick ifdef check

static void Script_Clear()
{
	HandleXRefs.clear();
	strHandlesSoFar = ';';	// initial state = ';' so all finds can check ";<name;"
}


// generates a unique handlename based on the input suggestion...
//
static LPCSTR GenerateHandleName(LPCSTR psIdealBaseName)
{
	static CString strReturn;

	int iAlternative = 0;

	do
	{
		strReturn = psIdealBaseName;
		strReturn.MakeLower();

		if (iAlternative++)
			strReturn += va("_%d",iAlternative);
	}
	while (strHandlesSoFar.Find(va(";%s;",(LPCSTR) strReturn)) != -1);	// note ';' at both ends of find() arg

	strHandlesSoFar += strReturn;
	strHandlesSoFar += ";";	// doesn't really matter what this is, as long as each name gets seperated

	return (LPCSTR) strReturn;
}


static void R_ModelContainer_WriteOptional(ModelContainer_t* pContainer, void *pvData, bool bIsBolt )
{
	FILE *fhText = (FILE *) pvData;

	if (Model_GetG2SurfaceRootOverride(pContainer) != -1)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_SURFACENAME_ROOTOVERRIDE, Model_GetSurfaceName(pContainer->hModel, Model_GetG2SurfaceRootOverride(pContainer->hModel)));
	}		

	// name of secondary anim start bone MUST be written out before iSequenceLockNumber_Secondary, or reading won't work...
	//
	if (Model_SecondaryAnimLockingActive(pContainer))
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_BONENAME_SECONDARYSTART, Model_GetBoneName(pContainer->hModel, Model_GetSecondaryAnimStart(pContainer->hModel)));
	}

	if (pContainer->iSequenceLockNumber_Primary != -1)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_LOCKSEQUENCE,	Model_Sequence_GetName(pContainer->hModel, pContainer->iSequenceLockNumber_Primary));
	}

	if (pContainer->iSequenceLockNumber_Secondary != -1)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_LOCKSEQUENCE_SECONDARY, Model_Sequence_GetName(pContainer->hModel, pContainer->iSequenceLockNumber_Secondary));
	}

	if (pContainer->bSeqMultiLock_Primary_Active)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");		
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_MULTISEQ_PRIMARYLOCK, sANY_NONBLANK_STRING);
	}

	if (pContainer->bSeqMultiLock_Secondary_Active)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");		
		fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_MULTISEQ_SECONDARYLOCK, sANY_NONBLANK_STRING);
	}

	// group...
	if (pContainer->SeqMultiLock_Primary.size())
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\n",sSCRIPTKEYWORD_MULTISEQ_PRIMARYLIST);
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "{\n");

		for (int i=0; i<pContainer->SeqMultiLock_Primary.size(); i++)
		{				
			LPCSTR psSeqName = Model_Sequence_GetName(pContainer, pContainer->SeqMultiLock_Primary[i]);

			if (psSeqName)
			{			
				fprintf(fhText, bIsBolt?"\t\t\t":"\t\t");
				fprintf(fhText, "name%d\t\"%s\"\n",i,psSeqName);
			}
		}

		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "}\n");
	}

	// group...
	if (pContainer->SeqMultiLock_Secondary.size())
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\n",sSCRIPTKEYWORD_MULTISEQ_SECONDARYLIST);
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "{\n");

		for (int i=0; i<pContainer->SeqMultiLock_Secondary.size(); i++)
		{				
			LPCSTR psSeqName = Model_Sequence_GetName(pContainer, pContainer->SeqMultiLock_Secondary[i]);

			if (psSeqName)
			{			
				fprintf(fhText, bIsBolt?"\t\t\t":"\t\t");
				fprintf(fhText, "name%d\t\"%s\"\n",i,psSeqName);
			}
		}

		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "}\n");
	}	

	if (pContainer->SkinSets.size())
	{
		if (!pContainer->strCurrentSkinFile.empty() && !pContainer->strCurrentSkinEthnic.empty())
		{
			fprintf(fhText, bIsBolt?"\t\t":"\t");
			fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_SKINFILE, pContainer->strCurrentSkinFile.c_str());

			fprintf(fhText, bIsBolt?"\t\t":"\t");
			fprintf(fhText, "%s\t\t\"%s\"\n",sSCRIPTKEYWORD_ETHNIC,	pContainer->strCurrentSkinEthnic.c_str());
		}
	}

	if (pContainer->OldSkinSets.size())
	{
		if (!pContainer->strCurrentSkinFile.empty())
		{
			fprintf(fhText, bIsBolt?"\t\t":"\t");
			fprintf(fhText, "%s\t\"%s\"\n",sSCRIPTKEYWORD_OLDSKINFILE, pContainer->strCurrentSkinFile.c_str());
		}
	}

	// write out all surfaces that are ON that weren't defaulted to ON (ie user-changed)...
	//
	int 
	iNumSurfaces = Model_GetNumSurfacesDifferentFromDefault(pContainer,SURF_ON);
	if (iNumSurfaces)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\n",sSCRIPTKEYWORD_SURFACES_ON);
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "{\n");

		for (int i=0; i<iNumSurfaces; i++)
		{
			LPCSTR psSurfaceName = Model_GetSurfaceDifferentFromDefault(pContainer,SURF_ON,i);
			if (psSurfaceName)
			{
				fprintf(fhText, bIsBolt?"\t\t\t":"\t\t");
				fprintf(fhText, "name%d\t\"%s\"\n",i,psSurfaceName );
			}
		}
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "}\n");
	}

	// write out all surfaces that are OFF that weren't defaulted to OFF (ie user-changed)...
	//
	iNumSurfaces = Model_GetNumSurfacesDifferentFromDefault(pContainer,SURF_OFF);
	if (iNumSurfaces)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\n",sSCRIPTKEYWORD_SURFACES_OFF);
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "{\n");

		for (int i=0; i<iNumSurfaces; i++)
		{
			LPCSTR psSurfaceName = Model_GetSurfaceDifferentFromDefault(pContainer,SURF_OFF,i);
			if (psSurfaceName)
			{
				fprintf(fhText, bIsBolt?"\t\t\t":"\t\t");
				fprintf(fhText, "name%d\t\"%s\"\n",i, psSurfaceName);
			}
		}
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "}\n");
	}

	// write out all surfaces that are OFF+NOCHILDREN (ie user-changed)...
	//
	iNumSurfaces = Model_GetNumSurfacesDifferentFromDefault(pContainer,SURF_NO_DESCENDANTS);
	if (iNumSurfaces)
	{
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "%s\n",sSCRIPTKEYWORD_SURFACES_OFFNOCHILDREN);
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "{\n");

		for (int i=0; i<iNumSurfaces; i++)
		{
			LPCSTR psSurfaceName = Model_GetSurfaceDifferentFromDefault(pContainer,SURF_NO_DESCENDANTS,i);
			if (psSurfaceName)
			{
				fprintf(fhText, bIsBolt?"\t\t\t":"\t\t");
				fprintf(fhText, "name%d\t\"%s\"\n",i, psSurfaceName);
			}
		}
		fprintf(fhText, bIsBolt?"\t\t":"\t");
		fprintf(fhText, "}\n");
	}
}

static void R_ModelContainer_CallBack_WriteBolts(ModelContainer_t* pContainer, void *pvData )
{
	FILE *fhText = (FILE *) pvData;
	
	CString strHandleName = GenerateHandleName(Filename_WithoutPath(Filename_WithoutExt(pContainer->sLocalPathName)));

	HandleXRefs[ pContainer ] = strHandleName;	

	// main model or bolton?
	if (pContainer->pBoneBolt_ParentContainer == NULL && pContainer->pSurfaceBolt_ParentContainer == NULL)
	{
		// parent...
		//
		/* eg:

		name	"mainwpn"
		modelfile	"models/weapons/m4/m4.glm"

		*/

		fprintf(fhText, "\t%s\t\t\"%s\"\n",sSCRIPTKEYWORD_NAME, (LPCSTR) strHandleName);
		fprintf(fhText, "\t%s\t\"%s\"\n",sSCRIPTKEYWORD_MODELFILE, (LPCSTR) pContainer->sLocalPathName);

		R_ModelContainer_WriteOptional(pContainer, pvData, false );
	}
	else
	{
		// bolton...(surface or bone)
		//
		/* eg:

		boltmodel
		{
			name	"ar buffer"
			modelfile	"models/test/m4/buffer/buffer.glm"
			parent	"mainwpn"
			bolttobone	"gun"
			locksequence "right_hand_idle"
		}

		*/

		fprintf(fhText, "\t%s\n",sSCRIPTKEYWORD_BOLTMODEL);
		fprintf(fhText, "\t{\n");
		fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_NAME,			(LPCSTR) strHandleName);
		fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_MODELFILE,	(LPCSTR) pContainer->sLocalPathName);		
		//
		// we're bolted to either a bone or a surface (never both), so...
		//
		if (pContainer->pBoneBolt_ParentContainer)
		{
			fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_PARENT,		HandleXRefs[pContainer->pBoneBolt_ParentContainer].c_str());
			fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_BOLTTOBONE,	Model_GetBoltName(pContainer->pBoneBolt_ParentContainer,pContainer->iBoneBolt_ParentBoltIndex, true));	// bBoltIsBone
		}
		else
		{
			fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_PARENT,			HandleXRefs[pContainer->pSurfaceBolt_ParentContainer].c_str());
			fprintf(fhText, "\t\t%s\t\"%s\"\n",sSCRIPTKEYWORD_BOLTTOSURFACE,	Model_GetBoltName(pContainer->pSurfaceBolt_ParentContainer,pContainer->iSurfaceBolt_ParentBoltIndex, false));	// bBoltIsBone
		}

		R_ModelContainer_WriteOptional(pContainer, pvData, true );
		
		fprintf(fhText, "\t}\n");
	}

	fprintf(fhText,"\n");
}






bool Script_Write(LPCSTR psFullPathedFilename)
{
	FILE *fhText = NULL;

	if (Model_Loaded())
	{
		Script_Clear();

		fhText = fopen(psFullPathedFilename,"wt");

		if (fhText)
		{
			fprintf(fhText,va("%s\n{\n",sSCRIPTKEYWORD_LOADMODEL));

			R_ModelContainer_Apply(&AppVars.Container, R_ModelContainer_CallBack_WriteBolts, fhText);

			fprintf(fhText,"}\n");

			fprintf(fhText,"\n");	// cosmetic

			fclose(fhText);
		}
		// DT EDIT
		/*
		else
		{
			ErrorBox( va("Couldn't open file: %s\n", psFullPathedFilename));
			return false;
		}
		*/

	}

	return !!fhText;
}



// search for certain optional arguments for the model that's just been loaded (whether bolt or primary)..
//
// return = errmess, else NULL for ok
//
static LPCSTR Script_Parse_ReadOptionalArgs(ModelContainer_t *pContainer, CGPGroup *pParseGroup)
{
	LPCSTR psError = NULL;

	// I know this code has loads of spurious string constructors, but I don't care. It's nice and readable,
	//	and adds no user-noticeable overhead to file reading...
	//
	do
	{
		string strSurfaceName = pParseGroup->FindPairValue(sSCRIPTKEYWORD_SURFACENAME_ROOTOVERRIDE, "");
		if (strSurfaceName.length())
		{
			if (!Model_SetG2SurfaceRootOverride	(pContainer, strSurfaceName.c_str()))
			{
				psError = va("Failed to set model root-surface override \"%s\"",strSurfaceName.c_str());
			}
		}

		// name of secondary anim start bone MUST be written out before iSequenceLockNumber_Secondary, or reading won't work...
		//
		string strBoneName = pParseGroup->FindPairValue(sSCRIPTKEYWORD_BONENAME_SECONDARYSTART, "");
		if (strBoneName.length())
		{
			if (!Model_SetSecondaryAnimStart(pContainer->hModel, strBoneName.c_str()))
			{
				psError = va("Failed to set secondary anim start bone \"%s\"",strBoneName.c_str());
				break;
			}
		}

		// "locksequence"...?
		//
		string strSequence = pParseGroup->FindPairValue(sSCRIPTKEYWORD_LOCKSEQUENCE, "");
		if (strSequence.length())
		{				
			if (!Model_Sequence_Lock(pContainer->hModel,strSequence.c_str(),true))	// errors displayed internally
			{
				psError = "Failed to lock primary sequence";
				break;
			}
		}

		// "locksequence_secondary"...?
		//
		strSequence = pParseGroup->FindPairValue(sSCRIPTKEYWORD_LOCKSEQUENCE_SECONDARY, "");
		if (strSequence.length())
		{			
			if (!Model_Sequence_Lock(pContainer->hModel,strSequence.c_str(),false))	// errors displayed internally
			{
				psError = "Failed to lock secondary sequence";
				break;
			}
		}		


		// "skinfile"...?
		//
		string strSkinFile = pParseGroup->FindPairValue(sSCRIPTKEYWORD_SKINFILE, "");
		if (strSkinFile.length())
		{
			string strEthnic = pParseGroup->FindPairValue(sSCRIPTKEYWORD_ETHNIC, "");
			if (strEthnic.length())
			{
				if (!Skins_ApplyEthnic( pContainer, strSkinFile.c_str(), strEthnic.c_str(),false/*findmeste true*/,true))
				{
					psError = va("Failed to apply ethnic \"%s\" of skin \"%s\"",strSkinFile.c_str(), strEthnic.c_str());
					break;
				}
			}
		}

		// "oldskinfile"...?
		//
		string strOldSkinFile = pParseGroup->FindPairValue(sSCRIPTKEYWORD_OLDSKINFILE, "");
		if (strOldSkinFile.length())
		{
			if (!OldSkins_Apply( pContainer, strOldSkinFile.c_str() ))
			{
				psError = va("Failed to apply skin \"%s\"",strOldSkinFile.c_str());
				break;
			}
		}

		// multisequence primary and secondary locking...
		//
		string 
		strLock = pParseGroup->FindPairValue(sSCRIPTKEYWORD_MULTISEQ_PRIMARYLOCK, "");
		if (strLock.length())
		{
			Model_MultiSeq_SetActive(pContainer->hModel, true, true);
		}

		strLock = pParseGroup->FindPairValue(sSCRIPTKEYWORD_MULTISEQ_SECONDARYLOCK, "");
		if (strLock.length())
		{
			Model_MultiSeq_SetActive(pContainer->hModel, false, true);
		}

		// multisequence primary and secondary lists...
		//
		CGPGroup* 
		pSubGroup = pParseGroup->FindSubGroup(sSCRIPTKEYWORD_MULTISEQ_PRIMARYLIST);
		if (pSubGroup)
		{
			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{			
				// string strJunk = (*it).first;	// eg 'name0'
				string strSequence = pValue->GetTopValue();

				int iSeqNum = Model_Sequence_IndexForName(pContainer, strSequence.c_str());
				if (iSeqNum != -1)
				{
					Model_MultiSeq_Add( pContainer->hModel, iSeqNum, true, false);
				}

				pValue = pValue->GetNext();
			}
		}
		pSubGroup = pParseGroup->FindSubGroup(sSCRIPTKEYWORD_MULTISEQ_SECONDARYLIST);
		if (pSubGroup)
		{
			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{
				// string strJunk = (*it).first;	// eg 'name0'
				string strSequence = pValue->GetTopValue();

				int iSeqNum = Model_Sequence_IndexForName(pContainer, strSequence.c_str());
				if (iSeqNum != -1)
				{
					Model_MultiSeq_Add( pContainer->hModel, iSeqNum, false, false);
				}

				pValue = pValue->GetNext();
			}
		}

		// now read in any surfaces that were set ON, OFF, or OFF+NO CHILDREN by the user...
		//		
		pSubGroup = pParseGroup->FindSubGroup(sSCRIPTKEYWORD_SURFACES_ON);
		if (pSubGroup)
		{
			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{			
				// string strJunk = (*it).first;	// eg 'name0'
				string strSurface = pValue->GetTopValue();

				Model_GLMSurface_SetStatus( pContainer->hModel, strSurface.c_str(), SURF_ON);

				pValue = pValue->GetNext();
			}
		}

		pSubGroup = pParseGroup->FindSubGroup(sSCRIPTKEYWORD_SURFACES_OFF);
		if (pSubGroup)
		{
			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{			
				// string strJunk = (*it).first;	// eg 'name0'
				string strSurface = pValue->GetTopValue();

				Model_GLMSurface_SetStatus( pContainer->hModel, strSurface.c_str(), SURF_OFF);

				pValue = pValue->GetNext();
			}
		}

		pSubGroup = pParseGroup->FindSubGroup(sSCRIPTKEYWORD_SURFACES_OFFNOCHILDREN);
		if (pSubGroup)
		{
			CGPValue *pValue = pSubGroup->GetPairs();
			while (pValue)
			{			
				// string strJunk = (*it).first;	// eg 'name0'
				string strSurface = pValue->GetTopValue();

				Model_GLMSurface_SetStatus( pContainer->hModel, strSurface.c_str(), SURF_NO_DESCENDANTS);

				pValue = pValue->GetNext();
			}
		}

		// ...
	}
	while(0);


	if (psError)
	{
		static string	strErrors;
						strErrors = psError;
						strErrors.insert(0,"Script_Parse_ReadOptionalArgs(): ");

		return strErrors.c_str();
	}

	return NULL;
}


static LPCSTR Script_Parse_BoltModel(CGPGroup *pParseGroup)
{
	LPCSTR psError = NULL;

	do
	{
		// "name"...
		//
		string strHandle = pParseGroup->FindPairValue(sSCRIPTKEYWORD_NAME, "");
		if (! (strHandle.length()))
		{
			psError = "No '" sSCRIPTKEYWORD_NAME "' keyword found!";
			break;
		}

		// "modelfile"...
		//
		string strModelFile = pParseGroup->FindPairValue(sSCRIPTKEYWORD_MODELFILE, "");
		if (! (strModelFile.length()))
		{
			psError = "No '" sSCRIPTKEYWORD_MODELFILE "' keyword found!";
			break;
		}

		// "parent"...
		//
		string strParentHandle = pParseGroup->FindPairValue(sSCRIPTKEYWORD_PARENT, "");
		if (! (strParentHandle.length()))
		{
			psError = "No '" sSCRIPTKEYWORD_PARENT "' keyword found!";
			break;
		}

		// "bolttobone" or "bolttosurface"
		//
		string strBoltToBone	= pParseGroup->FindPairValue(sSCRIPTKEYWORD_BOLTTOBONE, "");
		string strBoltToSurface	= pParseGroup->FindPairValue(sSCRIPTKEYWORD_BOLTTOSURFACE, "");
		if ( !(strBoltToBone.length()) && !(strBoltToSurface.length()) )
		{
			psError = "No '" sSCRIPTKEYWORD_BOLTTOBONE "' or '" sSCRIPTKEYWORD_BOLTTOSURFACE "' keyword found!";
			break;
		}

		// check parent handle is valid...
		//
		ModelContainer_t *pParentContainer = NULL;
		for (HandleXref_t::iterator it = HandleXRefs.begin(); it!=HandleXRefs.end(); it++)
		{
			pParentContainer = (*it).first;
			string sThisName = (*it).second;

			if (sThisName == strParentHandle)
				break;
		}
		if (pParentContainer == NULL)
		{
			psError = va("Unknown parent handle \"%s\"",strParentHandle.c_str());
			break;
		}


		// read model in...
		//
		char *psFullPathedFileName = va("%s%s",gamedir,strModelFile.c_str());
		//
		// bolt to bone?
		//
		if (strBoltToBone.length())
		{
			if (!Model_LoadBoltOn(psFullPathedFileName, pParentContainer->hModel, strBoltToBone.c_str(), true, false))	// bBoltIsBone, bBoltReplacesAllExisting
			{
				psError = va("Failed to load bonebolt-model \"%s\"",psFullPathedFileName);
				break;
			}
		}
		else
		{
			// bolt to surface...
			//
			if (!Model_LoadBoltOn(psFullPathedFileName, pParentContainer->hModel, strBoltToSurface.c_str(), false, false))	// bBoltIsBone, bBoltReplacesAllExisting
			{
				psError = va("Failed to load surfacebolt-model \"%s\"",psFullPathedFileName);
				break;
			}
		}

		// loaded ok...
		//
		ModelContainer_t *pThisContainer = ModelContainer_FindFromModelHandle( AppVars.hModelLastLoaded );
		HandleXRefs[pThisContainer] = strHandle;
		
		Script_Parse_ReadOptionalArgs(pThisContainer, pParseGroup);
	}
	while(0);


	if (psError)
	{
		static string	strErrors;
						strErrors = psError;
						strErrors.insert(0,"Script_Parse_BoltModel(): ");

		return strErrors.c_str();
	}

	return NULL;
}



static LPCSTR Script_Parse_LoadModel(CGPGroup *pParseGroup)
{
	LPCSTR psError = NULL;

	do
	{
		// "name"...
		//
		string strHandle = pParseGroup->FindPairValue(sSCRIPTKEYWORD_NAME, "");
		if (! (strHandle.length()))
		{
			psError = "No '" sSCRIPTKEYWORD_NAME "' keyword found!";
			break;
		}

		// "modelfile"...
		//
		string strModelFile = pParseGroup->FindPairValue(sSCRIPTKEYWORD_MODELFILE, "");
		if (! (strModelFile.length()))
		{
			psError = "No '" sSCRIPTKEYWORD_MODELFILE "' keyword found!";
			break;
		}


		// read model in... (need to make local version of this to pass, va() gets used too much during load
		//
		extern bool Document_ModelLoadPrimary(LPCSTR psFilename);
		//char *psFullPathModelName = va("%s%s",gamedir,strModelFile.c_str());
		char sFullPathModelName[MAX_PATH];
		strncpy(sFullPathModelName,va("%s%s",gamedir,strModelFile.c_str()),sizeof(sFullPathModelName)-1);
		sFullPathModelName[sizeof(sFullPathModelName)-1]='\0';

		if (!Document_ModelLoadPrimary( sFullPathModelName ))
		{
			psError = va("Failed to load primary model %s",sFullPathModelName);
			break;
		}

		// loaded ok...
		//
		ModelContainer_t *pThisContainer = &AppVars.Container;
		HandleXRefs[pThisContainer] = strHandle;			
		
		psError = Script_Parse_ReadOptionalArgs(pThisContainer, pParseGroup);
		if (psError)
			break;

		// now check for boltons...
		//
		CGPGroup *pSubGroup = pParseGroup->GetSubGroups();
		while (pSubGroup)
		{			
			string strSubGroupType = pSubGroup->GetName();

			if (strSubGroupType == sSCRIPTKEYWORD_BOLTMODEL)
			{
				if ((psError = Script_Parse_BoltModel(pSubGroup))!=NULL)
					break;
			}

			pSubGroup = pSubGroup->GetNext();
		}
		if (psError)
			break;
	}
	while(0);


	if (psError)
	{
		static string	strErrors;
						strErrors = psError;
						strErrors.insert(0,"Script_Parse_LoadModel(): ");

		return strErrors.c_str();
	}

	return NULL;
}


bool Script_Read(LPCSTR psFullPathedFilename)
{
	LPCSTR psError = NULL;
	char *psData = NULL;	
	
	int iSize = LoadFile(psFullPathedFilename, (void**)&psData);
	if (iSize != -1)
	{
		SetQdirFromPath( psFullPathedFilename );

		Script_Clear();

		char *psDataPtr = psData;
		
		// parse it...
		//
		if (theParser.Parse(&psDataPtr, true))
		{
			CGPGroup	*pFileGroup = theParser.GetBaseParseGroup();

			CGPGroup *pParseGroup_LoadModel = pFileGroup->FindSubGroup(sSCRIPTKEYWORD_LOADMODEL);//, true);

			if (pParseGroup_LoadModel)
			{
				// special optional arg for NPC->MVS hackiness...
				//
				string strBaseDir = pParseGroup_LoadModel->FindPairValue(sSCRIPTKEYWORD_BASEDIR, "");
				if (!strBaseDir.empty())
				{
					SetQdirFromPath( strBaseDir.c_str() );
				}
				
				psError = Script_Parse_LoadModel(pParseGroup_LoadModel);
			}
			else
			{
				psError = "Unable to find keyword '" sSCRIPTKEYWORD_LOADMODEL "'";
			}
		}
		else
		{
			psError = va("{} - Brace mismatch error in file \"%s\"!",psFullPathedFilename);
		}
	}
	else
	{
		psError = "File not found";
	}

	SAFEFREE(psData);

	if (psError)
	{
		ErrorBox(va("Error while reading script file: \"%s\":\n\n%s",psFullPathedFilename,psError));
	}

	return !psError;
}


/////////////////// eof /////////////////


