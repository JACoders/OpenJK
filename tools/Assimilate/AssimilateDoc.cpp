// AssimilateDoc.cpp : implementation of the CAssimilateDoc class
//

#include "stdafx.h"
#include "Includes.h"
#include "BuildAll.h"
#include "sourcesafe.h"
#include "gla.h"	// just for string stuff

#include <set>
using namespace std;

#define sSAVEINFOSTRINGCHECK "(SaveInfo):"	// so comment reader can stop these damn things accumulating

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int giLODLevelOverride = 0;	// MUST default to 0

void SS_DisposingOfCurrent(LPCSTR psFileName, bool bDirty);
static bool FileUsesGLAReference(LPCSTR psFilename, LPCSTR psGLAReference);

keywordArray_t CAssimilateDoc::s_Symbols[] = 
{
	"\\",		TK_BACKSLASH,
	"/",		TK_SLASH,
	".",		TK_DOT,
	"_",		TK_UNDERSCORE,
	"-",		TK_DASH,
	"$",		TK_DOLLAR,
	NULL,		TK_EOF,
};

keywordArray_t CAssimilateDoc::s_Keywords[] = 
{
	"aseanimgrabinit",			TK_AS_GRABINIT,
	"scale",					TK_AS_SCALE,
	"keepmotion",				TK_AS_KEEPMOTION,
	"pcj",						TK_AS_PCJ,
	"aseanimgrab",				TK_AS_GRAB,	
	"aseanimgrab_gla",			TK_AS_GRAB_GLA,
	"aseanimgrabfinalize",		TK_AS_GRABFINALIZE,
	"aseanimconvert",			TK_AS_CONVERT,
	"aseanimconvertmdx",		TK_AS_CONVERTMDX,
	"aseanimconvertmdx_noask",	TK_AS_CONVERTMDX_NOASK,
	NULL,						TK_EOF,
};

keywordArray_t CAssimilateDoc::s_grabKeywords[] = 
{
	"frames",				TK_AS_FRAMES,
	"fill",					TK_AS_FILL,
	"sound",				TK_AS_SOUND,
	"action",				TK_AS_ACTION,
	"enum",					TK_AS_ENUM,
	"loop",					TK_AS_LOOP,
	"qdskipstart",			TK_AS_QDSKIPSTART,	// useful so qdata can quickly skip extra stuff without having to know the syntax of what to skip
	"qdskipstop",			TK_AS_QDSKIPSTOP,
	"additional",			TK_AS_ADDITIONAL,
	"prequat",				TK_AS_PREQUAT,
	"framespeed",			TK_AS_FRAMESPEED,	// retro hack because original format only supported frame speeds on additional sequences, not masters
	"genloopframe",			TK_AS_GENLOOPFRAME,
	NULL,					TK_EOF,
};

keywordArray_t CAssimilateDoc::s_convertKeywords[] = 
{
	"playerparms",			TK_AS_PLAYERPARMS,
	"origin",				TK_AS_ORIGIN,
	"smooth",				TK_AS_SMOOTH,
	"losedupverts",			TK_AS_LOSEDUPVERTS,
	"makeskin",				TK_AS_MAKESKIN,
	"ignorebasedeviations",	TK_AS_IGNOREBASEDEVIATIONS,	// temporary!
	"skew90",				TK_AS_SKEW90,
	"noskew90",				TK_AS_NOSKEW90,
	"skel",					TK_AS_SKEL,
	"makeskel",				TK_AS_MAKESKEL,
	NULL,					TK_EOF,
};

LPCTSTR CAssimilateDoc::GetKeyword(int token, int table)
{
	if ((table == TABLE_ANY) || (table == TABLE_QDT))
	{
		int i = 0;
		while(s_Keywords[i].m_tokenvalue != TK_EOF)
		{
			if (s_Keywords[i].m_tokenvalue == token)
			{
				return s_Keywords[i].m_keyword;
			}
			i++;
		}
	}
	if ((table == TABLE_ANY) || (table == TABLE_GRAB))
	{
		int i = 0;
		while(s_grabKeywords[i].m_tokenvalue != TK_EOF)
		{
			if (s_grabKeywords[i].m_tokenvalue == token)
			{
				return s_grabKeywords[i].m_keyword;
			}
			i++;
		}
	}
	if ((table == TABLE_ANY) || (table == TABLE_CONVERT))
	{
		int i = 0;
		while(s_convertKeywords[i].m_tokenvalue != TK_EOF)
		{
			if (s_convertKeywords[i].m_tokenvalue == token)
			{
				return s_convertKeywords[i].m_keyword;
			}
			i++;
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateDoc

IMPLEMENT_DYNCREATE(CAssimilateDoc, CDocument)

BEGIN_MESSAGE_MAP(CAssimilateDoc, CDocument)
	//{{AFX_MSG_MAP(CAssimilateDoc)
	ON_COMMAND(IDM_ADDFILES, OnAddfiles)
	ON_COMMAND(IDM_EXTERNAL, OnExternal)
	ON_COMMAND(IDM_RESEQUENCE, OnResequence)
	ON_COMMAND(IDM_BUILD, OnBuild)	
	ON_COMMAND(IDM_BUILD_MULTILOD, OnBuildMultiLOD)	
	ON_COMMAND(IDM_VALIDATE, OnValidate)		
	ON_COMMAND(IDM_CARWASH,  OnCarWash)
	ON_COMMAND(IDM_VALIDATE_MULTILOD, OnValidateMultiLOD)	
	ON_COMMAND(ID_VIEW_ANIMENUMS, OnViewAnimEnums)	
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMENUMS, OnUpdateViewAnimEnums)
	ON_COMMAND(ID_VIEW_FRAMEDETAILS, OnViewFrameDetails)	
	ON_UPDATE_COMMAND_UI(ID_VIEW_FRAMEDETAILS, OnUpdateViewFrameDetails)	
	ON_UPDATE_COMMAND_UI(IDM_RESEQUENCE, OnUpdateResequence)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_UPDATE_COMMAND_UI(IDM_EXTERNAL, OnUpdateExternal)
	ON_UPDATE_COMMAND_UI(IDM_VALIDATE, OnUpdateValidate)
	ON_UPDATE_COMMAND_UI(IDM_BUILD, OnUpdateBuild)
	ON_COMMAND(ID_EDIT_BUILDALL, OnEditBuildall)
	ON_COMMAND(IDM_EDIT_BUILDDEPENDANT, OnEditBuildDependant)
	ON_COMMAND(ID_VIEW_FRAMEDETAILSONADDITIONALSEQUENCES, OnViewFramedetailsonadditionalsequences)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FRAMEDETAILSONADDITIONALSEQUENCES, OnUpdateViewFramedetailsonadditionalsequences)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_BUILDDEPENDANT, OnUpdateEditBuilddependant)
	ON_COMMAND(ID_EDIT_LAUNCHMODVIEWONCURRENT, OnEditLaunchmodviewoncurrent)
	ON_UPDATE_COMMAND_UI(ID_EDIT_LAUNCHMODVIEWONCURRENT, OnUpdateEditLaunchmodviewoncurrent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAssimilateDoc construction/destruction

CAssimilateDoc::CAssimilateDoc()
{
	// TODO: add one-time construction code here
	Init();
}

CAssimilateDoc::~CAssimilateDoc()
{
}

BOOL CAssimilateDoc::OnNewDocument()
{
	SS_DisposingOfCurrent(m_strPathName, !!IsModified());

	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	SetTitle("Untitled");	// for some reason MFC doesn't do this from time to time

	return TRUE;
}

void CAssimilateDoc::Init()
{
	m_comments = NULL;
	m_modelList = NULL;
	m_curModel = NULL;
	m_lastModel = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateDoc serialization

CModel* CAssimilateDoc::AddModel()
{
	CModel* thisModel = CModel::Create(m_comments);
	m_comments = NULL;
	if (m_modelList == NULL)
	{
		m_modelList = thisModel;
	}
	else
	{
		CModel* curModel = m_modelList;
		while (curModel->GetNext() != NULL)
		{
			curModel = curModel->GetNext();
		}
		curModel->SetNext(thisModel);
	}
	m_curModel = thisModel;

	return m_curModel;
}

//	remember to account for Mike's m_lastModel and move it down if necessary (until I can throw it away)...
//
void CAssimilateDoc::DeleteModel(CModel *deleteModel)
{
	// linklist is only 1-way, so we need to find the stage previous to this (if any)...
	//
	CModel* prevModel = NULL;
	CModel* scanModel = GetFirstModel();

	while (scanModel && scanModel != deleteModel)
	{
		prevModel = scanModel;
		scanModel = scanModel->GetNext();
	}
	if (scanModel == deleteModel)
	{
		// we found it, so was this the first model in the list?
		//
		if (prevModel)
		{
			prevModel->SetNext(scanModel->GetNext());	// ...no
		}
		else
		{
			m_modelList = scanModel->GetNext();			// ...yes
		}
		scanModel->Delete();
	}

	// fixme: ditch this whenever possible
	// keep Mike's var up to date...
	//
	scanModel = GetFirstModel();	
	while(scanModel && scanModel->GetNext())
	{
		scanModel = scanModel->GetNext();
	}
	m_lastModel = scanModel;
}

void CAssimilateDoc::EndModel()
{
	m_lastModel = m_curModel;
	m_curModel = NULL;
}

// XSI or GLA anim grab...
//
void CAssimilateDoc::ParseGrab(CTokenizer* tokenizer, int iGrabType)
{
	if (m_curModel == NULL)
	{
		tokenizer->Error("Grab without an active model");
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	CToken* curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL, 0);
	if (curToken->GetType() != TK_IDENTIFIER)
	{
		tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
		curToken->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	CString path = curToken->GetStringValue();
	curToken->Delete();

	while(curToken != NULL)
	{
		curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
		switch(curToken->GetType())
		{
		case TK_SLASH:
		case TK_BACKSLASH:
			path += "/";
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);

			// hack for "8472" as in models/players/8472/blah.car. Arrggh!!!!!!!!!!!!!!!!!!
			//
			if (curToken->GetType() == TK_INT)
			{
				path += curToken->GetStringValue();
				curToken->Delete();
				break;
			}

			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_UNDERSCORE:
		case TK_DASH:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_DOT:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = NULL;
			break;
		case TK_SPACE:
			curToken->Delete();
			curToken = NULL;
			break;
		case TK_EOL:
			tokenizer->PutBackToken(curToken);			
			curToken = NULL;
			break;
		default:
			tokenizer->PutBackToken(curToken);
			curToken = NULL;
			break;
		}
	}

	CString enumname;
	int fill = -1;
	int loop = 0;
	CString sound;
	CString action;	
	int startFrame = 0;
	int targetFrame = 0;
	int framecount = -1;
	int framespeed = iDEFAULTSEQFRAMESPEED;
	bool bFrameSpeedFound = false;
	int iFrameSpeedFromHeader;
	//
	int iStartFrames[MAX_ADDITIONAL_SEQUENCES]={0};
	int iFrameCounts[MAX_ADDITIONAL_SEQUENCES]={0};
	int iLoopFrames [MAX_ADDITIONAL_SEQUENCES]={0};
	int iFrameSpeeds[MAX_ADDITIONAL_SEQUENCES]={0};
	CString csEnums [MAX_ADDITIONAL_SEQUENCES];
	int iAdditionalSeqNum = 0;
	bool bGenLoopFrame = false;

	bool bSomeParamsFound = false;

	curToken = tokenizer->GetToken(TKF_USES_EOL);
	switch (iGrabType)
	{
		case TK_AS_GRAB:
		{			
			while (curToken->GetType() == TK_DASH)
			{
				bSomeParamsFound = true;
				curToken->Delete();
				curToken = tokenizer->GetToken(s_grabKeywords, TKF_USES_EOL, 0);
				switch (curToken->GetType())
				{
				case TK_AS_FRAMES:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					startFrame = curToken->GetIntValue();
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					targetFrame = curToken->GetIntValue();
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					framecount = curToken->GetIntValue();
					break;
				case TK_AS_FILL:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					fill = curToken->GetIntValue();
					break;
				case TK_AS_ENUM:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_IDENTIFIER)
					{
						tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					enumname = curToken->GetStringValue();
					break;
				case TK_AS_SOUND:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_IDENTIFIER)
					{
						tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					sound = curToken->GetStringValue();
					break;
				case TK_AS_ACTION:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_IDENTIFIER)
					{
						tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					action = curToken->GetStringValue();
					break;
				case TK_AS_LOOP:
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					loop = curToken->GetIntValue();
					break;

				case TK_AS_QDSKIPSTART:
				case TK_AS_QDSKIPSTOP:
					//curToken->Delete();	// don't do this, the whole thing relies on case statements leaving one current token for the outside loop to delete
					break;

				case TK_AS_GENLOOPFRAME:
					bGenLoopFrame = true;
					break;

				case TK_AS_FRAMESPEED:	// this is still read in for compatibility, but gets overwritten lower down
					curToken->Delete();

					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					framespeed = curToken->GetIntValue();
					bFrameSpeedFound = true;
					break;

				case TK_AS_PREQUAT:					
					m_curModel->SetPreQuat(true);
					break;
				case TK_AS_ADDITIONAL:
					curToken->Delete();

					if (iAdditionalSeqNum == MAX_ADDITIONAL_SEQUENCES)
					{
						tokenizer->Error(TKERR_USERERROR, va("Trying to define > %d additional sequences for this master",MAX_ADDITIONAL_SEQUENCES));
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}

					// startframe...
					//
					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					iStartFrames[iAdditionalSeqNum] = curToken->GetIntValue();

					// framecount...
					//
					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					iFrameCounts[iAdditionalSeqNum] = curToken->GetIntValue();

					// loopframe...
					//
					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					iLoopFrames[iAdditionalSeqNum] = curToken->GetIntValue();

					// framespeed...
					//
					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_INTEGER)
					{
						tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					iFrameSpeeds[iAdditionalSeqNum] = curToken->GetIntValue();

					// enum...
					//
					curToken->Delete();
					curToken = tokenizer->GetToken(NULL, TKF_USES_EOL, 0);
					if (curToken->GetType() != TK_IDENTIFIER)
					{
						tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}
					csEnums[iAdditionalSeqNum] = curToken->GetStringValue();

					iAdditionalSeqNum++;
					break;

				default:
					tokenizer->Error(TKERR_UNEXPECTED_TOKEN, curToken->GetStringValue());
					curToken->Delete();
					tokenizer->GetToEndOfLine()->Delete();
					return;
				}
				curToken->Delete();
				curToken = tokenizer->GetToken(s_grabKeywords, TKF_USES_EOL, 0);
			}
		}
		break;

		case TK_AS_GRAB_GLA:
		{
			// no additional params permitted for this type currently...
			//
		}
		break;
	}

	path.MakeLower();
	//
	// if no extension, assume ".xsi"... (or ".gla" now)
	//
	if (!(path.GetAt(path.GetLength()-4) == '.'))
	{
		path += (iGrabType == TK_AS_GRAB)?".xsi":".gla";
	}

	if (curToken->GetType() != TK_EOL)
	{
		tokenizer->Error(TKERR_UNEXPECTED_TOKEN, curToken->GetStringValue());
		curToken->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}

	// ignore any user params about speed and frame counts, and just re-grab them from the XSI file...
	//
//	if (bSomeParamsFound)
//	{
//	}
//	else
	{
		// at this point, it must be one of the paramless entries in a .CAR file, so we need to
		//	provide the values for: startFrame, targetFrame, framecount
		//
		// read in values from the actual file, in case we need to use them...
		//
		CString nameASE = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
				nameASE+= path;
		int iStartFrame, iFrameCount;
		ReadASEHeader( nameASE, iStartFrame, iFrameCount, iFrameSpeedFromHeader, (iGrabType == TK_AS_GRAB_GLA) );

//		if (strstr(nameASE,"death16"))
//		{
//			int z=1;
//		}

		startFrame = 0;	// always
		targetFrame= 0;	// any old shite value
		framecount = iFrameCount;
		
		if (iGrabType != TK_AS_GRAB_GLA)
		{
			if (!bFrameSpeedFound)
			{
				framespeed = iFrameSpeedFromHeader;
			}
		}
	}

	curToken->Delete();

	CSequence* sequence = CSequence::_Create(bGenLoopFrame,(iGrabType == TK_AS_GRAB_GLA), path, startFrame, targetFrame, framecount, framespeed, iFrameSpeedFromHeader);
	m_curModel->AddSequence(sequence);
	sequence->AddComment(m_curModel->ExtractComments());
	sequence->DeriveName();
	if (enumname.IsEmpty())
	{
		sequence->SetEnum(sequence->GetName());
	}
	else
	{
		sequence->SetEnum(enumname);
	}
	sequence->SetFill(fill);
	sequence->SetSound(sound);
	sequence->SetAction(action);
	sequence->SetValidEnum(((CAssimilateApp*)AfxGetApp())->ValidEnum(sequence->GetEnum()));
	sequence->SetLoopFrame(loop);

	for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
	{
		sequence->AdditionalSeqs[i]->SetStartFrame(iStartFrames[i]);
		sequence->AdditionalSeqs[i]->SetFrameCount(iFrameCounts[i]);
		sequence->AdditionalSeqs[i]->SetFrameSpeed(iFrameSpeeds[i]);
		sequence->AdditionalSeqs[i]->SetLoopFrame (iLoopFrames [i]);
		sequence->AdditionalSeqs[i]->SetEnum	  (csEnums	   [i]);
		sequence->AdditionalSeqs[i]->SetValidEnum (((CAssimilateApp*)AfxGetApp())->ValidEnum(sequence->AdditionalSeqs[i]->GetEnum()));
	}
}


// return = success.  if false ret, return from caller because of error
//
bool Tokenizer_ReadPath(CString& path, CTokenizer* &tokenizer, CToken* &curToken )
{
	curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL, 0);
	if (curToken->GetType() != TK_IDENTIFIER)
	{
		tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
		curToken->Delete();
		tokenizer->GetToEndOfLine()->Delete();
		return false;
	}
	path += curToken->GetStringValue();
	curToken->Delete();

	while(curToken != NULL)
	{
		curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | (path.IsEmpty()?0:TKF_SPACETOKENS), 0);
		switch(curToken->GetType())
		{
		case TK_SLASH:
		case TK_BACKSLASH:
			path += "/";
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);

			// hack for "8472" as in models/players/8472/blah.car. Arrggh!!!!!!!!!!!!!!!!!!
			//
			if (curToken->GetType() == TK_INT)
			{
				path += curToken->GetStringValue();
				curToken->Delete();
				break;
			}

			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return false;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_UNDERSCORE:
		case TK_DASH:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return false;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_DOT:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return false;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = NULL;
			break;
		case TK_SPACE:
		case TK_EOL:
			curToken->Delete();
			curToken = NULL;
			break;
		default:
			tokenizer->PutBackToken(curToken);
			curToken = NULL;
			break;
		}
	}

	return true;
}

void CAssimilateDoc::ParseConvert(CTokenizer* tokenizer, int iTokenType)
{
	if (m_lastModel == NULL)
	{
		tokenizer->Error("Convert without an internal model");
		tokenizer->GetToEndOfLine()->Delete();
		return;
	}
	CToken* curToken = NULL;
	CString path;	

	if (!Tokenizer_ReadPath(path, tokenizer, curToken ))
		return;
/*
	while(curToken != NULL)
	{
		curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
		switch(curToken->GetType())
		{
		case TK_SLASH:
		case TK_BACKSLASH:
			path += "/";
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);

			// hack for "8472" as in models/players/8472/blah.car. Arrggh!!!!!!!!!!!!!!!!!!
			//
			if (curToken->GetType() == TK_INT)
			{
				path += curToken->GetStringValue();
				curToken->Delete();
				break;
			}

			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_UNDERSCORE:
		case TK_DASH:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			break;
		case TK_DOT:
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = tokenizer->GetToken(NULL, TKF_NUMERICIDENTIFIERSTART | TKF_USES_EOL | TKF_SPACETOKENS, 0);
			if (curToken->GetType() != TK_IDENTIFIER)
			{
				tokenizer->Error(TKERR_EXPECTED_IDENTIFIER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			path += curToken->GetStringValue();
			curToken->Delete();
			curToken = NULL;
			break;
		case TK_SPACE:
		case TK_EOL:
			curToken->Delete();
			curToken = NULL;
			break;
		default:
			tokenizer->PutBackToken(curToken);
			curToken = NULL;
			break;
		}
	}
*/
	int originx = 0;	// important to default to 0!
	int originy = 0;	//
	int originz = 0;	//
	int parm1 = 0;
	int parm2 = 0;
	int parm3 = 0;
	int parm4 = 0;

	curToken = tokenizer->GetToken();
	while(curToken->GetType() == TK_DASH)
	{
		curToken->Delete();
		curToken = tokenizer->GetToken(s_convertKeywords, 0, 0);
		switch(curToken->GetType())
		{
		case TK_AS_ORIGIN:
			curToken->Delete();
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			originx = curToken->GetIntValue();
			curToken->Delete();
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			originy = curToken->GetIntValue();
			curToken->Delete();
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			originz = curToken->GetIntValue();
			curToken->Delete();
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_PLAYERPARMS:
			curToken->Delete();
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			parm1 = curToken->GetIntValue();
			curToken->Delete();
/* this param no longer exists...
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			parm2 = curToken->GetIntValue();
			curToken->Delete();
*/			
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			parm3 = curToken->GetIntValue();
			curToken->Delete();

/*	this param no longer exists...
			curToken = tokenizer->GetToken();
			if (curToken->GetType() != TK_INTEGER)
			{
				tokenizer->Error(TKERR_EXPECTED_INTEGER, curToken->GetStringValue());
				curToken->Delete();
				tokenizer->GetToEndOfLine()->Delete();
				return;
			}
			parm4 = curToken->GetIntValue();
			curToken->Delete();
*/			parm4 = 1;

			curToken = tokenizer->GetToken();
			break;
		case TK_AS_SMOOTH:
			curToken->Delete();
			m_lastModel->SetSmooth(true);
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_LOSEDUPVERTS:
			curToken->Delete();
			m_lastModel->SetLoseDupVerts(true);
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_MAKESKIN:
			curToken->Delete();
			m_lastModel->SetMakeSkin(true);
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_IGNOREBASEDEVIATIONS:
			curToken->Delete();
			m_lastModel->SetIgnoreBaseDeviations(true);
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_SKEW90:
			curToken->Delete();
			m_lastModel->SetSkew90(true);
			curToken = tokenizer->GetToken();
			break;
		case TK_AS_NOSKEW90:
			curToken->Delete();
			m_lastModel->SetNoSkew90(true);
			curToken = tokenizer->GetToken();
			break;
/*
		case TK_AS_SKEL:
			{
				CString strSkelPath;
				curToken->Delete();

				if (!Tokenizer_ReadPath(strSkelPath, tokenizer, curToken ))
				{
					return;	
				}
				m_lastModel->SetSkelPath(strSkelPath);
				m_lastModel->SetMakeSkelPath("");
				curToken = tokenizer->GetToken();
			}
			break;
*/
		case TK_AS_MAKESKEL:
			{
				CString strMakeSkelPath;
				curToken->Delete();

				if (!Tokenizer_ReadPath(strMakeSkelPath, tokenizer, curToken))
				{
					return;
				}
				m_lastModel->SetMakeSkelPath(strMakeSkelPath);
//				m_lastModel->SetSkelPath("");
				curToken = tokenizer->GetToken();
			}
			break;
		default:
			tokenizer->Error(TKERR_UNEXPECTED_TOKEN, curToken->GetStringValue());
			curToken->Delete();
			tokenizer->GetToEndOfLine()->Delete();
			return;
		}
	}
	tokenizer->PutBackToken(curToken);
	path.MakeLower();
	m_lastModel->DeriveName(path);
	m_lastModel->SetParms(parm1, parm2, parm3, parm4);
	m_lastModel->SetOrigin(originx, originy, originz);
	m_lastModel->SetConvertType(iTokenType);
}

void CAssimilateDoc::AddComment(LPCTSTR comment)
{
	// some code to stop those damn timestamps accumulating...
	//
	if (!strnicmp(comment,sSAVEINFOSTRINGCHECK,strlen(sSAVEINFOSTRINGCHECK)))
	{
		return;
	}
	CComment* thisComment = CComment::Create(comment);
	if (m_curModel != NULL)
	{
		m_curModel->AddComment(thisComment);
		return;
	}
	if (m_comments == NULL)
	{
		m_comments = thisComment;
	}
	else
	{
		CComment* curComment = m_comments;
		while (curComment->GetNext() != NULL)
		{
			curComment = curComment->GetNext();
		}
		curComment->SetNext(thisComment);
	}
}

///////////////////////////////////////////////

#define	MAX_FOUND_FILES	0x1000
#define MAX_OSPATH MAX_PATH
#include <stdio.h>
#include <io.h>

char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	int			findhandle;
	int			flag;
	int			i;

	if ( !extension) {
		extension = "";
	}

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = strdup( strlwr(findinfo.name) );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **) malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void	Sys_FreeFileList( char **_list ) {
	int		i;

	if ( !_list ) {
		return;
	}

	for ( i = 0 ; _list[i] ; i++ ) {
		free( _list[i] );
	}

	free( _list );
}

//////////////////////////////////////////

CString strSkippedFiles;
CString strSkippedDirs;
CString strCARsFound;
int iCARsFound = 0;

void AlphaSortCARs(void)
{
	typedef set <string>	SortedStrings_t;
							SortedStrings_t SortedStrings;

	for (int i=0; i<iCARsFound; i++)
	{
		CString strThisFile = strCARsFound;
		int iLoc = strThisFile.Find("\n");
		if (iLoc>=0)
		{
			SortedStrings.insert(SortedStrings.end(), (LPCSTR)(strThisFile.Left(iLoc)));
			strCARsFound= strCARsFound.Mid(iLoc+1);
		}
	}

	// clear files-found string out, and re-enter from sorted set...
	//
	strCARsFound = "";

	for (SortedStrings_t::iterator it = SortedStrings.begin(); it != SortedStrings.end(); ++it)
	{
		strCARsFound += (*it).c_str();
		strCARsFound += "\n";
	}
}

void R_CheckCARs( LPCSTR psDir, int iScanDepth, LPCSTR psGLAReferenceItShouldInclude )
{
	((CMainFrame*)AfxGetMainWnd())->StatusMessage(va("(%d .CAR files found so far) Scanning Dir: %s",iCARsFound,psDir));

	// ignore any dir with "test" in it...
	//
	if (//strstr(psDir,"\\test")
		//||
		strstr(psDir,"\\backup")
		||
		strstr(psDir,"\\ignore_")
		)
	{
		strSkippedDirs += psDir;
		strSkippedDirs += "\n";
		return;
	}

	char	**sysFiles, **dirFiles;//, *args[5];
	int		numSysFiles, i, /*len,*/ numdirs;
	char	altname[MAX_OSPATH];
//	char	command1[MAX_OSPATH];
//	char	command2[MAX_OSPATH];

	dirFiles = Sys_ListFiles(psDir, "/", &numdirs);
	if (numdirs > 2)
	{
//		if (!iScanDepth)	// recursion limiter, to avoid scanning backup subdirs within model subdirs
		{
			for (i=2;i<numdirs;i++)
			{
				sprintf(altname, "%s\\%s", psDir, dirFiles[i]);
				//if (stricmp(altname,"q:\\send\\quake\\baseq3\\models\\players"))	// dont recurse this dir
				{
					R_CheckCARs(altname, iScanDepth+1, psGLAReferenceItShouldInclude );
				}
			}
		}
	}
//	sprintf(command1, "-targa");
//	sprintf(command2, "-outfile");
	sysFiles = Sys_ListFiles( psDir, ".car", &numSysFiles );
	for ( i=0; i<numSysFiles; i++ )
	{
		CString strThisFile = va("%s\\%s", psDir, sysFiles[i]);

		if (strstr((LPCSTR) strThisFile,"copy of "))
		{
			strSkippedFiles += strThisFile;
			strSkippedFiles += "\n";
			continue;
		}

		if (psGLAReferenceItShouldInclude && psGLAReferenceItShouldInclude [0]
			&&
			!FileUsesGLAReference(strThisFile, psGLAReferenceItShouldInclude)
			)
		{
			strSkippedFiles += strThisFile;
			strSkippedFiles += "\n";
			continue;
		}

		strCARsFound += strThisFile + "\n";
		iCARsFound++;
	/*		char	tgain[MAX_OSPATH];
			sprintf(tgain,"%s\\%s", psDir, sysFiles[i]);
			strcpy( altname, tgain );
			len = strlen( altname );
			altname[len-3] = 'j';
			altname[len-2] = 'p';
			altname[len-1] = 'g';
			args[0] = "cjpeg";
			args[1] = command2;
			args[2] = altname;
			args[3] = command1;
			args[4] = tgain;
			//printf("%s", tgain);
	*/		
			/*		len = qmain(5, args);
			if (!len) 
			{
			iNumberOf_FilesConverted++;
			iSizeOf_JPGsWritten += scGetFileLen(altname);
			iSizeOf_TGAsDeleted += scGetFileLen(tgain);
			
			  printf(" nuked!(NOT)");
			  //			remove(tgain);
			  }
			  printf("\n");
			*/
	/*		
			byte * pPixels = NULL;
			int iWidth;
			int iHeight;
			bool bRedundant = ScanTGA(tgain, &pPixels, &iWidth, &iHeight);
			if (pPixels)
			{
				free(pPixels);
			}
			
			if (bRedundant)
			{
				strTGAsWithRedundantAlpha += va("%s\n",tgain);
				iRedundantFilesFound++;
			}
	*/
	}

	Sys_FreeFileList( sysFiles );
	Sys_FreeFileList( dirFiles );
}

void CAssimilateDoc::Parse(CFile* file)
{
	Parse(file->GetFilePath());
}

void CAssimilateDoc::Parse(LPCSTR psFilename)
{	
	gbParseError = false;
	CAlertErrHandler errhandler;
	CTokenizer* tokenizer = CTokenizer::Create(TKF_NOCASEKEYWORDS | TKF_COMMENTTOKENS);
	tokenizer->SetErrHandler(&errhandler);
	tokenizer->SetSymbols(s_Symbols);
	tokenizer->SetKeywords(s_Keywords);
	tokenizer->AddParseFile(psFilename);

	extern bool gbSkipXSIRead_QuestionAsked;
	extern bool gbSkipXSIRead;
	gbSkipXSIRead_QuestionAsked = false;	// opening a new file so reset our question
	gbSkipXSIRead = false;

	int tokType = TK_UNDEFINED;
	while(tokType != TK_EOF)
	{
		CToken* curToken = tokenizer->GetToken();
		tokType = curToken->GetType();
		switch(tokType)
		{
		case TK_EOF:
			curToken->Delete();
			break;
		case TK_DOLLAR:
			curToken->Delete();
			curToken = tokenizer->GetToken();
			switch(curToken->GetType())
			{
			case TK_AS_GRABINIT:
				curToken->Delete();
				AddModel();
				break;
			case TK_AS_SCALE:
				curToken->Delete();
				curToken = tokenizer->GetToken();
				if (curToken->GetType() != TK_FLOAT && curToken->GetType() != TK_INT)
				{
					tokenizer->Error(TKERR_EXPECTED_FLOAT, curToken->GetStringValue());
					curToken->Delete();
					tokenizer->GetToEndOfLine()->Delete();
					return;
				}
				m_curModel->SetScale((curToken->GetType() == TK_FLOAT)?curToken->GetFloatValue():curToken->GetIntValue());
				curToken->Delete();				
				break;
			case TK_AS_KEEPMOTION:
				curToken->Delete();
				m_curModel->SetKeepMotion(true);
				break;
			case TK_AS_PCJ:
				curToken->Delete();
				curToken = tokenizer->GetToken();
				if (curToken->GetType() != TK_IDENTIFIER && curToken->GetType() != TK_DOLLAR)	// eg:  '$pcj pelvis'   or   '$pcj $flatten'
				{
					tokenizer->Error(TKERR_EXPECTED_STRING, curToken->GetStringValue());
					curToken->Delete();
					tokenizer->GetToEndOfLine()->Delete();
					return;
				}
				if (curToken->GetType() == TK_DOLLAR)	// read string after '$' char
				{
					curToken->Delete();
					curToken = tokenizer->GetToken();
					if (curToken->GetType() != TK_IDENTIFIER)
					{
						tokenizer->Error(TKERR_EXPECTED_STRING, curToken->GetStringValue());
						curToken->Delete();
						tokenizer->GetToEndOfLine()->Delete();
						return;
					}

					m_curModel->PCJList_AddEntry(va("$%s",curToken->GetStringValue()));
				}
				else
				{	
					m_curModel->PCJList_AddEntry(curToken->GetStringValue());
				}
				curToken->Delete();
				break;
			case TK_AS_GRAB:
				curToken->Delete();
				ParseGrab(tokenizer,TK_AS_GRAB);
				break;
			case TK_AS_GRAB_GLA:
				curToken->Delete();
				ParseGrab(tokenizer,TK_AS_GRAB_GLA);
				break;
			case TK_AS_GRABFINALIZE:
				curToken->Delete();
				EndModel();
				break;
			case TK_AS_CONVERT:
				curToken->Delete();
				ParseConvert(tokenizer,TK_AS_CONVERT);
				break;
			case TK_AS_CONVERTMDX:
				curToken->Delete();
				ParseConvert(tokenizer,TK_AS_CONVERTMDX);
				break;
			case TK_AS_CONVERTMDX_NOASK:
				curToken->Delete();
				ParseConvert(tokenizer,TK_AS_CONVERTMDX_NOASK);
				break;
			case TK_COMMENT:
				AddComment(curToken->GetStringValue());
				curToken->Delete();
				break;
			default:
				tokenizer->Error(TKERR_UNEXPECTED_TOKEN);
				curToken->Delete();
				break;
			}
			break;
		case TK_COMMENT:
			AddComment(curToken->GetStringValue());
			curToken->Delete();
			break;
		default:
			tokenizer->Error(TKERR_UNEXPECTED_TOKEN);
			curToken->Delete();
			break;
		}
	}
	tokenizer->Delete();
	UpdateAllViews(NULL, AS_NEWFILE, NULL);
	Resequence();
}

void CAssimilateDoc::Write(CFile* file)
{
	CTxtFile* outfile = CTxtFile::Create(file);

/*	// write out time/date stamp...
	//
	CString commentLine;
	CTime time = CTime::GetCurrentTime();
	commentLine.Format("// %s %s updated %s", sSAVEINFOSTRINGCHECK, file->GetFileName(), time.Format("%H:%M %A, %B %d, %Y"));
	outfile->Writeln(commentLine);
*/	
	CModel* curModel = m_modelList;
	while(curModel != NULL)
	{
		curModel->Write(outfile);
		curModel = curModel->GetNext();
	}
	CComment* curComment = m_comments;
	while(curComment != NULL)
	{
		curComment->Write(outfile);
		curComment = curComment->GetNext();
	}
	outfile->Delete();
}

void CAssimilateDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
		Write(ar.GetFile());
	}
	else
	{
		// TODO: add loading code here
		Parse(ar.GetFile());
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAssimilateDoc diagnostics

#ifdef _DEBUG
void CAssimilateDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAssimilateDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAssimilateDoc commands

void CAssimilateDoc::DeleteContents() 
{
	// TODO: Add your specialized code here and/or call the base class
	UpdateAllViews(NULL, AS_DELETECONTENTS, NULL);

	while(m_comments != NULL)
	{
		CComment* curComment = m_comments;
		m_comments = curComment->GetNext();
		curComment->Delete();
	}
	while(m_modelList != NULL)
	{
		CModel* curModel = m_modelList;
		m_modelList = curModel->GetNext();
		curModel->Delete();
	}
	m_curModel = NULL;
	m_lastModel = NULL;

	gbReportMissingASEs = true;	
	giFixUpdatedASEFrameCounts = YES;

	CDocument::DeleteContents();
}

CModel* CAssimilateDoc::GetFirstModel()
{
	return m_modelList;
}

int CAssimilateDoc::GetNumModels()
{
	int iCount = 0;
	CModel *theModel = m_modelList;

	while (theModel)
	{
		iCount++;
		theModel = theModel->GetNext();
	}

	return iCount;
}

void CAssimilateDoc::OnAddfiles() 
{
	// TODO: Add your command handler code here
	CFileDialog theDialog(true, ".xsi", NULL, OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, 
					_T("Anim Files (*.ase)(*.xsi)(*.gla)|*.ase;*.xsi;*.gla|All Files|*.*||"), NULL);
					
////////////
//	Model files (*.MDR)(*.MD3)|*.md?|
////////////
	char filenamebuffer[16384];
	filenamebuffer[0] = '\0';
	theDialog.m_ofn.lpstrFile = filenamebuffer;
	theDialog.m_ofn.nMaxFile = sizeof(filenamebuffer);

	CString 
	strInitialDir = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
	strInitialDir+= "models/players";
	strInitialDir.Replace("/","\\");

	theDialog.m_ofn.lpstrInitialDir = strInitialDir;

	int result = theDialog.DoModal();
	if (result != IDOK)
	{
		return;
	}

	CWaitCursor waitcursor;

	POSITION pos = theDialog.GetStartPosition();
	while(pos != NULL)
	{
		CString thisfile = theDialog.GetNextPathName(pos);
/*		int loc = thisfile.Find(':');
		if (loc > 0)
		{
			thisfile = thisfile.Right(thisfile.GetLength() - loc - 1);
		}
*/
		Filename_RemoveBASEQ(thisfile);
		AddFile(thisfile);		
	}
	SetModifiedFlag();
	UpdateAllViews(NULL, AS_NEWFILE, NULL);
	Resequence();	// must be AFTER UpdateAllViews
}

void CAssimilateDoc::AddFile(LPCTSTR name)
{
	CString strTemp(name);
			strTemp.MakeLower();
	if (!strstr(strTemp,"root.xsi") || GetYesNo("You're trying to add \"root.xsi\", which is inherent, you should only do this if you're making a model that has no seperate anim files\n\nProceed?"))
	{	
		// update, ignore any files with a "_1" (2,3, etc) just before the suffix (this skips LOD files)...
		//
//		int iNameLen = strlen(name);

		// (also, only check files of at least namelen "_?.ase")
		//
//		if ( iNameLen>6 && name[iNameLen-6]=='_' && isdigit(name[iNameLen-5]) )
		{
			// this is a LOD filename, so ignore it...
		}
//		else
		{
			CModel *curModel = GetCurrentUserSelectedModel();

			if (!curModel)
			{
				curModel = AddModel();	
			
				CString path = name;
				path.MakeLower();
				path.Replace('\\', '/');
				int loc = path.ReverseFind('.');
				if (loc > -1)
				{
					path = path.Left(loc);
				}
				loc = path.ReverseFind('/');
				path = path.Left(loc);
				path = path + "/root";
				curModel->DeriveName(path);
			}

			// check that we don't already have this file...
			//
			if (!curModel->ContainsFile(name))
			{
				curModel->AddSequence(CSequence::CreateFromFile(name, curModel->ExtractComments()));
			}
		}
	}
}

void CAssimilateDoc::OnExternal()
{		
	bool bCFGWritten = false;

	if (WriteCFGFiles(true,bCFGWritten))
	{
		CString strReport;	

		if (bCFGWritten)
		{		
			if (((CAssimilateApp*)AfxGetApp())->GetMultiPlayerMode())
			{
				strReport = "\n\n( CFG file written for MULTI-PLAYER format )";
			}
			else
			{
				strReport = "\n\n( CFG file written for SINGLE-PLAYER format )";
			}
		}
		else
		{
			strReport = "\n\n( CFG not needed, not written )";
		}

		InfoBox(strReport);
	}
}

// called both from the menu, and now from the Build() member...
//
bool CAssimilateDoc::WriteCFGFiles(bool bPromptForNames, bool &bCFGWritten)
{
	bCFGWritten = false;	

	CModel* curModel = m_modelList;
	while(curModel != NULL)
	{
		bool bThisCFGWritten = false;
		if (!curModel->WriteExternal(bPromptForNames,bThisCFGWritten))
		{
			return false;
		}
		if (bThisCFGWritten)
			bCFGWritten = true;

		curModel = curModel->GetNext();
	}

	return true;
}

void CAssimilateDoc::Resequence()
{
	CModel* curModel = m_modelList;
	while(curModel != NULL)
	{
		curModel->Resequence(true);
		curModel = curModel->GetNext();
	}

	SetModifiedFlag();
	UpdateAllViews(NULL, AS_FILESUPDATED, NULL);	// needed
}

void CAssimilateDoc::OnResequence() 
{
	extern bool gbSkipXSIRead_QuestionAsked;
	extern bool gbSkipXSIRead;
	gbSkipXSIRead_QuestionAsked = false;	// make it happen again
	gbSkipXSIRead = false;
	Resequence();
}

void CAssimilateDoc::OnViewAnimEnums()
{
	gbViewAnimEnums = !gbViewAnimEnums;
	UpdateAllViews(NULL, AS_FILESUPDATED, NULL);
}

void CAssimilateDoc::OnUpdateViewAnimEnums(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(gbViewAnimEnums);
}

void CAssimilateDoc::OnViewFrameDetails()
{
	gbViewFrameDetails = !gbViewFrameDetails;
	UpdateAllViews(NULL, AS_FILESUPDATED, NULL);
}

void CAssimilateDoc::OnUpdateViewFrameDetails(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(gbViewFrameDetails);
}


// 1) save qdt, 2) run qdata on it, 3) if success, save .cfg file...
//
bool CAssimilateDoc::Build(bool bAllowedToShowSuccessBox, int iLODLevel, bool bSkipSave)	// damn this stupid Serialize() crap
{
	bool bSuccess = false;

	if (Validate())	// notepad will have been launched with a textfile of errors at this point if faulty
	{
		// seems valid, so save the QDT...
		//
		giLODLevelOverride = iLODLevel;

		if (!bSkipSave)
		{
			OnFileSave();		
		}
		
		CString csQDataLocation = ((CAssimilateApp*)AfxGetApp())->GetQDataFilename();

		// hack-city!!!!!!!!!!
		//
		CModel* curModel = ghAssimilateView->GetDocument()->GetCurrentUserSelectedModel();
		if (curModel->GetPreQuat())
		{
			csQDataLocation.MakeLower();
			csQDataLocation.Replace("carcass","carcass_prequat");
		}
		
		CString params = csQDataLocation;
		
		if (!bSkipSave)
		{
			params += " -keypress";
		}
		params += " -silent";
		params += " ";
		params += m_strPathName;	// += (eg) {"Q:\quake\baseq3\models\players\ste_assimilate_test\ste_testaa.qdt"}
		Filename_AccountForLOD(params, giLODLevelOverride);

		PROCESS_INFORMATION pi;
		STARTUPINFO startupinfo;
		startupinfo.cb = sizeof(startupinfo);
		startupinfo.lpReserved = NULL;
		startupinfo.lpDesktop = NULL;
		startupinfo.lpTitle = NULL;
		startupinfo.dwFlags = 0;
		startupinfo.cbReserved2 = 0;
		startupinfo.lpReserved2 = NULL;

		params.Replace("/","\\");
		csQDataLocation.Replace("/","\\");

		LPTSTR paramsPass = params.GetBuffer(params.GetLength() + 1);

		StartWait();	// ++++++++++++++++++++++++

		if (CreateProcess(csQDataLocation, paramsPass, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupinfo, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			EndWait();	// ------------------------
			DWORD result;
			GetExitCodeProcess(pi.hProcess,&result);
			if (result)
			{
				char error[64];
				sprintf(error,"Process returned error: %d",result);
				MessageBox(NULL,error,"Build Failed",MB_OK|MB_ICONERROR);
			}
			CloseHandle(pi.hProcess);

			if (result==0)
			{
				// QData was run successfully at this point, so write the CFG file(s)...
				//
				if (iLODLevel)	// only LOD 0 writes out the CFG file
				{
					bSuccess = true;
				}
				else
				{
					bool bCFGWritten = false;
					if (WriteCFGFiles(false,bCFGWritten))	// false = no name prompt, derive automatically from model name
					{
						// success (on a plate)...
						//
						if (bAllowedToShowSuccessBox)
						{
							CString strReport("Everything seemed to go ok\n\nCAR");

							if (bCFGWritten)
							{
								strReport += " and CFG files written";
								if (((CAssimilateApp*)AfxGetApp())->GetMultiPlayerMode())
								{
									strReport += "\n\n\n\n( CFG file written for MULTI-PLAYER format )";
								}
								else
								{
									strReport += "\n\n\n\n( CFG file written for SINGLE-PLAYER format )";
								}
							}
							else
							{
								strReport += " file written";
								strReport += "\n\n\n\n( CFG file not written for GLA-referencing model )";
							}

							InfoBox(strReport);
						}
						bSuccess = true;
					}
				}
			}
		}
		else
		{
			EndWait();	// ------------------------
			MessageBox(NULL,"Could not spawn process.","Build Failed",MB_OK|MB_ICONERROR);
		}
		params.ReleaseBuffer();
	}	

	giLODLevelOverride = 0;	

	return bSuccess;
}

void CAssimilateDoc::OnBuildMultiLOD()
{
	int iErrors = 0;

	// to save time, I'll run all the validates first, and only if they're all ok will I go on to the build
	//	(which incidentally does a harmless re-validate again)
	//
	for (int i=0; i< 1+EXTRA_LOD_LEVELS; i++)
	{
		iErrors += Validate(false, i)?0:1;
	}

	// go ahead if all clear...  
	//
	// (I'll write them in reverse-LOD order so the last one written is the standard one. This should hopefully avoid
	//	any problems with the current document potentially becoming "(name)_3.qdt" from then on to MFC)
	//
	if (!iErrors)
	{
		for (int i=EXTRA_LOD_LEVELS; i>=0; i--)
		{
			Build(	!i,		// bool bAllowedToShowSuccessBox (ie only on last one)
					i,		// int iLODLevel
					false	// bool bSkipSave
					);
		}
	}
}



// 1) save qdt, 2) run qdata on it, 3) if success, save .cfg file...
//
void CAssimilateDoc::OnBuild()
{	
	Build(	true,	// bool bAllowedToShowSuccessBox, 
			0,		// int iLODLevel
			false	// bool bSkipSave
			);
}


void CAssimilateDoc::ClearModelUserSelectionBools()
{
	CModel* curModel = m_modelList;

	while (curModel)
	{
		curModel->SetUserSelectionBool(false);		
		curModel = curModel->GetNext();
	}
}

// if there's only one model loaded, return that, if none, return NULL, else if >1, return selected one, else NULL
//
CModel* CAssimilateDoc::GetCurrentUserSelectedModel()
{
	CModel *curModel = m_modelList;

	if (!curModel || !curModel->GetNext())
	{
		return curModel;		// 0 or 1 models total loaded
	}

	// more than one loaded, so find the selected one and return that...
	//
	while (curModel)
	{
		if (curModel->GetUserSelectionBool())
		{
			return curModel;
		}

		curModel = curModel->GetNext();
	}

	return NULL;	// multiple loaded, but none selected
}


static bool FileUsesGLAReference(LPCSTR psFilename, LPCSTR psGLAReference)
{
	bool bReturn = false;

	FILE *fHandle = fopen(psFilename,"rt");
	if (fHandle)
	{
		int iLen = filesize(fHandle);
		if (iLen>0)
		{
			char *psText = (char*) malloc(iLen+1);
			if (psText)
			{
				fread(psText,1,iLen,fHandle);
				psText[iLen]='\0';

				strlwr(psText);

				// this is a simple test that could be made more precise, but for now...
				//
				if ( (strstr(psText,"aseanimgrab_gla") || strstr(psText,"makeskel"))
					&&
					strstr(psText,psGLAReference)
					)
				{
					bReturn = true;
				}

				free(psText);
			}
		}
		fclose(fHandle);
	}

	return bReturn;
}



#define sASSUMEPATH "w:\\game\\base"
bool gbCarWash_YesToXSIScan;
bool gbCarWash_DoingScan = false;	// MUST default to this
bool gbQueryGoAhead = true;			// MUST defualt to this
LPCSTR gpsCARWashDirOverride = NULL;// MUST default to this
CString strCarWashErrors;
bool gbCarwashErrorsOccured;
CString strMustContainThisGLA;	// MUST be blank, else name of GLA to be present to be considered
void CAssimilateDoc::OnCarWashActual()
{
	gbCarwashErrorsOccured = false;
	CString strStartDir = gpsCARWashDirOverride?gpsCARWashDirOverride:((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
	strStartDir.Replace("/","\\");

	if (!strStartDir.GetLength())
	{			
		ErrorBox("Quake path not known at this point. Prefs not setup?");
		gbCarwashErrorsOccured = true;
		return;
//		if (!GetYesNo("Quake path not known at this point because you've not loaded anything yet\n\nShould I assume " "\"" sASSUMEPATH "\"" "?"))
//			return;
//
//		strStartDir = sASSUMEPATH;
	}

	// (this app was written so that GetQuakeDir() returns a path with a trailing slash, not nice normally, but here...)
	//
//	if (strStartDir.GetAt(strStartDir.GetLength()-1)=='\\')
//		strStartDir = strStartDir.Left(strStartDir.GetLength()-1);
	if (gpsCARWashDirOverride == NULL)
	{
		strStartDir += "models";//\\players";
	}

	if (gbQueryGoAhead)
	{
		if (!GetYesNo(va("About to scan: \"%s\\*.CAR /s\"\n\n"/*"This can take a LONG time, "*/"Proceed?",strStartDir)))
			return;
	}

	gbCarWash_YesToXSIScan = GetYesNo("Full XSI scan? ( \"NO\" will skip XSI reads, but v3.0 files are quick to read anyway now )");

	CWaitCursor wait;

	strCARsFound.Empty();
	iCARsFound = 0;
	strSkippedDirs.Empty();
	strSkippedFiles.Empty();
	
	R_CheckCARs( strStartDir, 0, strMustContainThisGLA);	//bool bBuildListOnly
	AlphaSortCARs();	// not important to alpha-sort here (during car-wash), just looks nicer
	((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");

	// ok, now ready to begin pass 2...
	//
	CString strReport;
	if (!iCARsFound)
	{			
		ASSERT(0);
		strReport = "No suitable .CAR files found for processing!\n\n";

		if (!strSkippedDirs.IsEmpty())
		{
			strReport+= "Skipped Dirs:\n\n";
			strReport+= strSkippedDirs;
		}

		if (!strSkippedFiles.IsEmpty())
		{
			strReport+= "\n\nSkipped Files:\n\n";
			strReport+= strSkippedFiles;
		}
		ErrorBox(strReport);
		gbCarwashErrorsOccured = true;
	}
	else
	{
		//----------------
		gbCarWash_DoingScan = true;
		strCarWashErrors.Empty();
		//----------------

		CString strTotalErrors;

		strReport = "Processed files:\n\n";
		for (int i=0; i<iCARsFound; i++)
		{						
			CString strThisFile = strCARsFound;
			int iLoc = strThisFile.Find("\n");
			if (iLoc>=0)
			{
				strThisFile = strThisFile.Left(iLoc);
				strCARsFound= strCARsFound.Mid(iLoc+1);

				strReport += strThisFile + "\n";

				((CMainFrame*)AfxGetMainWnd())->StatusMessage(va("Scanning File %d/%d: %s",i+1,iCARsFound,(LPCSTR)strThisFile));

				if (1)//strMustContainThisGLA.IsEmpty() || FileUsesGLAReference(strThisFile, strMustContainThisGLA))
				{
					OnNewDocument();
					Parse(strThisFile);
					if (gbParseError)
					{
						strTotalErrors += va("\nParse error in CAR file \"%s\"\n",(LPCSTR)strThisFile);					
					}
					else
					{
						OnValidate();

						if (!strCarWashErrors.IsEmpty())
						{
							// "something is wrong..."	:-)
							//
							strTotalErrors += va("\nError in file \"%s\":\n\n%s\n",(LPCSTR)strThisFile,(LPCSTR)strCarWashErrors);
						}						
						strCarWashErrors.Empty();
					}
				}
			}
			else
			{
				ASSERT(0);
				strThisFile.Insert(0,"I fucked up, the following line didn't seem to have a CR:  (tell me! -Ste)\n\n");
				ErrorBox(strThisFile);
			}
		}

		//----------------
		gbCarWash_DoingScan = false;
		//----------------


		OnNewDocument();	// trash whatever was loaded last

//		strReport = "Processed files:\n\n";
//		strReport+= strCARsFound;
		strReport+= "\n\nSkipped Dirs:\n\n";
		strReport+= strSkippedDirs;
		strReport+= "\n\nSkipped Files:\n\n";
		strReport+= strSkippedFiles;

		if (strTotalErrors.IsEmpty())
		{
			strReport.Insert(0,"(No additional errors found)\n\n");
		}
		else
		{
			strReport+= "\n\nAdditional errors will now be sent to Notepad!...";
			gbCarwashErrorsOccured = true;
		}

		if (gbQueryGoAhead || gbCarwashErrorsOccured)
		{
			InfoBox(strReport);
		}

		if (!strTotalErrors.IsEmpty())
		{
			strTotalErrors.Insert(0, "The following errors occured during CARWash...\n\n");
			SendToNotePad(strTotalErrors, "carwash_errors.txt");
		}
	}

//#define sASSUMEPATH "q:\\quake\\baseq3"
//		strTGAsWithRedundantAlpha.Insert(0,"The following files are defined as 32-bit (ie with alpha), but the alpha channel is blank (ie all 255)...\n\n");
//		SendToNotePad(strTGAsWithRedundantAlpha, "TGAs_With_Redundant_Alpha.txt");

	((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");
}

void CAssimilateDoc::OnCarWash()
{
	strMustContainThisGLA.Empty();
	OnCarWashActual();
}


// creates as temp file, then spawns notepad with it...
//
bool SendToNotePad(LPCSTR psWhatever, LPCSTR psLocalFileName)
{
	bool bReturn = false;

	LPCSTR psOutputFileName = va("%s\\%s",scGetTempPath(),psLocalFileName);

	FILE *handle = fopen(psOutputFileName,"wt");
	if (handle)
	{
		fprintf(handle,psWhatever);
		fclose(handle);

		char sExecString[MAX_PATH];

		sprintf(sExecString,"notepad %s",psOutputFileName);

		if (WinExec(sExecString,	//  LPCSTR lpCmdLine,  // address of command line
					SW_SHOWNORMAL	//  UINT uCmdShow      // window style for new application
					)
			>31	// don't ask me, Windoze just uses >31 as OK in this call.
			)
		{
			// ok...
			//
			bReturn = true;
		}
		else
		{
			ErrorBox("Unable to locate/run NOTEPAD on this machine!\n\n(let me know about this -Ste)");		
		}
	}
	else
	{
		ErrorBox(va("Unable to create file \"%s\" for notepad to use!",psOutputFileName));
	}

	return bReturn;
}


// AFX OnXxxx calls need to be void return, but the validate needs to ret a bool for elsewhere, so...
//
void CAssimilateDoc::OnValidate()
{
	Validate(!gbCarWash_DoingScan);	
}

void CAssimilateDoc::OnValidateMultiLOD()
{
	int iErrors = 0;
	for (int i=0; i< 1+EXTRA_LOD_LEVELS; i++)
	{
		iErrors += Validate(false, i)?0:1;
	}

	if (!iErrors)
	{
		InfoBox(va("Everything seems OK\n\n(Original + %d LOD levels checked)",EXTRA_LOD_LEVELS));
	}
}


// checks for things that would stop the build process, such as missing ASE files, invalid loopframes, bad anim enums, etc,
//	and writes all the faults to a text file that it displays via launching notepad
//
bool CAssimilateDoc::Validate(bool bInfoBoxAllowed, int iLODLevel)
{
	OnResequence();

	int		iNumModels = ghAssimilateView->GetDocument()->GetNumModels();
	CModel* curModel = ghAssimilateView->GetDocument()->GetCurrentUserSelectedModel();
	bool	bValidateAll = false;
	int		iFaults = 0;

	StartWait();

	if (iNumModels)
	{
		CString sOutputTextFile;

		sOutputTextFile.Format("%s\\validation_faults%s.txt",scGetTempPath(),iLODLevel?va("_LOD%d",iLODLevel):"");

		FILE *hFile = fopen(sOutputTextFile,"wt");

		if (hFile)
		{
			if (iNumModels>1)
			{
				if (!curModel)	// >1 models, but none selected, so validate all
				{
					bValidateAll = true;
				}
				else
				{
					// >1 models, 1 selected, ask if we should do all...
					//				
					bValidateAll = GetYesNo(va("Validate ALL models?\n\n( NO = model \"%s\" only )",curModel->GetName()));
				}
			}

			if (bValidateAll)
			{
				curModel = ghAssimilateView->GetDocument()->GetFirstModel();			
			}	
			
			if (iLODLevel)
			{
				fprintf(hFile,"(LOD Level %d)\n",iLODLevel);
			}

			while (curModel)
			{
				fprintf(hFile,"\nModel: \"%s\"\n\n",curModel->GetName());

				int iThisModelFaults = iFaults;

				if (	( curModel->GetMakeSkelPath()	&& strlen(curModel->GetMakeSkelPath()) )
//						||
//						( curModel->GetSkelPath()		&& strlen(curModel->GetSkelPath()) )
					)
				{
					// ... then all is fine
				}
				else
				{
					// this is an error, UNLESS you have a GLA sequence...
					//
					if (!curModel->HasGLA())
					{					
						//fprintf(hFile, "Model must have either a 'skel' or 'makeskel' path\n    ( Double-click on the model name in the treeview to edit )\n");
						fprintf(hFile, "Model must have a 'makeskel' path\n    ( Double-click on the top tree item's name (should be a folder), then click \"Makes it's own skeleton\" in the dialog )\n");
						iFaults++;
					}
				}

				//
				// validate all sequences within this model...
				//				
				int iGLACount = 0;
				int iXSICount = 0;
				CSequence* curSequence = curModel->GetFirstSequence();
				while (curSequence)
				{
					// we'll need to check these counts after checking all sequences...
					//
					if (curSequence->IsGLA())
					{
						iGLACount++;
					}
					else
					{
						iXSICount++;
					}

					#define SEQREPORT CString temp;temp.Format("Sequence \"%s\":",curSequence->GetName());while (temp.GetLength()<35)temp+=" ";

					// check 1, does the ASE file exist? (actually this is talking about XSIs/GLAs, but WTF...
					//
					CString nameASE = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
							nameASE+= curSequence->GetPath();

					Filename_AccountForLOD(nameASE, iLODLevel);

					const int iNameSpacing=35;

					if (!FileExists(nameASE))
					{
						if (!gbCarWash_DoingScan)	// because this will only duplicate reports otherwise
						{
							SEQREPORT;
							temp += va(" ASE/XSI/GLA file not found: \"%s\"\n",nameASE);
							fprintf(hFile,temp);
							iFaults++;
						}
					}

					// new check, if this is an LOD ASE it must have the same framecount as the base (non-LOD) version...
					//
					if (iLODLevel)
					{
						// read this file's framecount...
						//
						int iStartFrame, iFrameCount, iFrameSpeed;
						curSequence->ReadASEHeader( nameASE, iStartFrame, iFrameCount, iFrameSpeed);

						// read basefile's framecount...
						//
						CString baseASEname = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
								baseASEname+= curSequence->GetPath();

						int iStartFrameBASE, iFrameCountBASE, iFrameSpeedBASE;
						curSequence->ReadASEHeader( baseASEname, iStartFrameBASE, iFrameCountBASE, iFrameSpeedBASE);

						// same?...
						//
						if (iFrameCount != iFrameCountBASE)
						{
							SEQREPORT;
							temp += va(" (SERIOUS ERROR) base ASE has %d frames, but this LOD version has %d frames!\n",iFrameCountBASE,iFrameCount);
							fprintf(hFile,temp);							
							iFaults++;
						}
					}

					// check 2, is the loopframe higher than the framecount of that sequence?...
					//
					if (curSequence->GetLoopFrame() >= curSequence->GetFrameCount())
					{
						SEQREPORT;
						temp += va(" loopframe %d is illegal, max = %d\n",curSequence->GetLoopFrame(),curSequence->GetFrameCount()-1);
						fprintf(hFile,temp);							
						iFaults++;
					}

					if (!curModel->IsGhoul2())
					{
						// check 3, is the enum valid?...
						//
						if (curSequence->GetEnumType() == ET_INVALID)
						{
							SEQREPORT;
							temp += va(" invalid animation enum \"%s\"\n",curSequence->GetEnum());
							fprintf(hFile,temp);							
							iFaults++;
						}
					}

					int iEnumUsageCount = curModel->AnimEnumInUse(curSequence->GetEnum());
					if (iEnumUsageCount>1)
					{
						SEQREPORT;
						temp += va(" animation enum \"%s\" is used %d times\n",curSequence->GetEnum(),iEnumUsageCount);
						fprintf(hFile,temp);							
						iFaults++;
					}					

					// a whole bunch of checks for the additional sequences...
					//
					if (!curSequence->IsGLA())
					{
						for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
						{
							CSequence *additionalSeq = curSequence->AdditionalSeqs[i];

							#define ADDITIONALSEQREPORT CString temp;temp.Format("Sequence \"%s\": (Additional: \"%s\"):",curSequence->GetName(),additionalSeq->GetEnum());while (temp.GetLength()<60)temp+=" ";

							if (additionalSeq->AdditionalSequenceIsValid())
							{
								// check for duplicate enum names...
								//
								int iEnumUsageCount = curModel->AnimEnumInUse(additionalSeq->GetEnum());
								if (iEnumUsageCount>1)
								{
									ADDITIONALSEQREPORT;								
									temp += va(" animation enum \"%s\" is used %d times\n",additionalSeq->GetEnum(),iEnumUsageCount);
									fprintf(hFile,temp);
									iFaults++;
								}

								// additional sequences must actually have some frames...
								//
								if (additionalSeq->GetFrameCount()<=0)
								{
									ADDITIONALSEQREPORT;
									temp += va(" a frame count of %d is illegal (min = 1)\n",additionalSeq->GetFrameCount());
									fprintf(hFile,temp);
									iFaults++;
								}
								else
								{
									// the start/count range of this additional seq can't exceed it's master...
									//
									if (additionalSeq->GetStartFrame() + additionalSeq->GetFrameCount() > curSequence->GetFrameCount())
									{
										ADDITIONALSEQREPORT;
										temp += va(" illegal start/count range of %d..%d exceeds master range of %d..%d\n",
													additionalSeq->GetStartFrame(),
													additionalSeq->GetStartFrame() + additionalSeq->GetFrameCount(),
													curSequence->GetStartFrame(),
													curSequence->GetStartFrame() + curSequence->GetFrameCount()
													);
										fprintf(hFile,temp);
										iFaults++;
									}
									else
									{
										// loopframe of an additional seq must be within its own seq framecount...
										//
										if (additionalSeq->GetLoopFrame() >= additionalSeq->GetFrameCount())
										{
											ADDITIONALSEQREPORT;
											temp += va(" loopframe %d is illegal, max is %d\n",additionalSeq->GetLoopFrame(),additionalSeq->GetFrameCount()-1);
											fprintf(hFile,temp);
											iFaults++;
										}
									}
								}
							}
							else
							{
								// is this additional sequence invalid because of being just empty or being bad?...
								//
								if (strlen(additionalSeq->GetEnum()))
								{
									// it's a bad sequence (probably because of its enum being deleted from anims.h since it was saved)
									//
									ADDITIONALSEQREPORT;								
									temp += va(" this animation enum no longer exists in \"%s\"\n",sDEFAULT_ENUM_FILENAME);
									fprintf(hFile,temp);
									iFaults++;
								}
							}
						}
					}
					curSequence = curSequence->GetNext();
				}

				// special GLA/XSI checks...
				//
				{
					if (iGLACount>1)
					{
						fprintf(hFile, "Model has more than one GLA file specified\n");
						iFaults++;
					}

					if (iGLACount && iXSICount)
					{
						fprintf(hFile, "Model has both GLA and XSI files specified. Pick one method or the other\n");
						iFaults++;
					}

					if (iGLACount && (curModel->GetMakeSkelPath() && strlen(curModel->GetMakeSkelPath())) )
					{
						fprintf(hFile, "Model has both a GLA sequence and a '-makeskel' path, this is meaningless\n");
						iFaults++;
					}

					/*					
					if (iGLACount && (curModel->GetSkelPath() && strlen(curModel->GetSkelPath())) )
					{
						fprintf(hFile, "Model has both a GLA sequence and a '-skel' path, you should probably blank out the 'skel' path\n");
						iFaults++;
					}
					*/
				}

				if (iThisModelFaults == iFaults)
				{
					fprintf(hFile,"(ok)\n");	// just to be nice if reporting on >1 model...
				}
				else
				{
					fprintf(hFile,"\n(%d faults)\n",iFaults-iThisModelFaults);
				}

				curModel = curModel->GetNext();
				if (!bValidateAll)
					break;			

			}// while (curModel)

			fclose(hFile);

			if (iFaults)
			{
				// now run notepad.exe on the file we've just created...
				//
				CString sExecString;

				sExecString.Format("notepad %s",sOutputTextFile);

				if (WinExec(sExecString,	//  LPCSTR lpCmdLine,  // address of command line
							SW_SHOWNORMAL	//  UINT uCmdShow      // window style for new application
							)
					>31	// don't ask me, Windoze just uses >31 as OK in this call.
					)
				{
					// ok.
				}
				else
				{
					ErrorBox("Unable to locate/run NOTEPAD on this machine!\n\n(let me know about this -Ste)");
				}
			}
			else
			{
				if (bInfoBoxAllowed)
				{
					InfoBox("Everything ok\n\n( All files exist, enums exist, and frames seem to be valid ranges )");
				}
			}
		}// if (hFile)
		else
		{
			ErrorBox(va("Arrgh! Unable to create file '%s'!\n\n(let me know about this -Ste)",sOutputTextFile));
		}

	}// if (iNumModels)

	EndWait();

	return !iFaults;

}


void CAssimilateDoc::OnUpdateResequence(CCmdUI* pCmdUI) 
{	
	pCmdUI->Enable(!!GetNumModels());	
}

void CAssimilateDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());	
}

void CAssimilateDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());	
}

void CAssimilateDoc::OnUpdateExternal(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());		
}

void CAssimilateDoc::OnUpdateValidate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());		
}

void CAssimilateDoc::OnUpdateBuild(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());		
}



BOOL CAssimilateDoc::DoFileSave()
{	
	// sourcesafe stuff
	{
		///////////////////////////////////////////
		//
		// some stuff so I can leave the code below untouched...
		//
		LPCSTR filename = (LPCSTR) m_strPathName;
		#define Sys_Printf(blah) StatusText(blah)
		//
		///////////////////////////////////////////
		//
		// check it out first, if necessary...
		//
		if ( SS_FunctionsAvailable() )
		{
			if ( SS_IsUnderSourceControl( filename ) )
			{
				if ( SS_IsCheckedOut( filename ))
				{
					if ( !SS_IsCheckedOutByMe( filename ))
					{
						CString strCheckOuts;
						int iCount;
						
						if (SS_ListCheckOuts( filename, strCheckOuts, iCount ))
						{
							ErrorBox( va("File \"%s\" is checked out by:\n\n%s\n... so you can't save over it...\n\n... so you can't compile...\n\nTough luck matey!....(bwahahahaha!!!!!)",filename,(LPCSTR) strCheckOuts));
							return false;
						}
					}
					else
					{
						Sys_Printf ("(You own this file under SourceSafe)\n");				
					}
				}
				else
				{
					if ( GetYesNo( va("The file \"%s\"\n\n...needs to be checked out so I can save over it\n\nProceed? ('No' will abort the save)",filename) ))
					{
						if (SS_CheckOut( filename ))
						{
							Sys_Printf ("(File checked out ok)\n");				
						}
						else
						{
							ASSERT(0);	// I want to know if this ever happens
							Sys_Printf ("(Error during file checkout, aborting save\n");				
							return false;
						}
					}
					else
					{
						Sys_Printf ("(Checkout cancelled, aborting save\n");				
						return false;
					}
				}
			}
			else
			{
				Sys_Printf ("(This file is not under SourceSafe control)\n");				
			}
		}

		// now do seperate check for files that are still write-protected...
		//
		DWORD dw = GetFileAttributes( filename );

		if (dw != 0xFFFFFFFF && ( dw & FILE_ATTRIBUTE_READONLY ))
		{
			// hmmm, still write protected...
			//
			if (SS_SetupOk())
			{
				if (GetYesNo( va("The file \"%s\" is write-protected, but probably not because of SourceSafe, just as a safety thing.\n\n(Tell me if you believe this is wrong -Ste)\n\nDo you want me to un-writeprotect it so you can save over it? ('No' will abort the save)",filename )))
				{
					if ( !SetFileAttributes( filename, dw&~FILE_ATTRIBUTE_READONLY) )
					{
						ErrorBox("Failed to remove write protect, aborting...");
						return false;
					}
				}
				else
				{
					Sys_Printf ("(File was not write-enabled, aborting save)");
					return false;
				}
			}
			else
			{
				ErrorBox( va("The file \"%s\" is write-protected, but you don't appear to have SourceSafe set up properly on this machine, so I can't tell if the file is protected or just not checked out to you.\n\nIf you really want to edit this you'll have to write-enable it yourself (which I'm deliberately not offering to do for you here <g>)",filename));
			}
		}
	}

	BOOL b = CDocument::DoFileSave();

	if (b == TRUE)
	{
		// sourcesafe
		LPCSTR filename = (LPCSTR) m_strPathName;
		#define Sys_Printf(blah) StatusText(blah)
		if ( SS_FunctionsAvailable() )
		{
			if ( SS_IsUnderSourceControl( filename ))
			{
				if ( SS_IsCheckedOutByMe( filename ))
				{
//					if ( SS_CheckIn( filename ))
//					{
//						Sys_Printf("(Checked in ok)\n");
//					}
//					else
//					{
//						Sys_Printf("Error during CheckIn\n");
//					}
				}
				else
				{
					ErrorBox( va("You do not have file \"%s\" checked out",filename));
				}
			}
			else
			{
				// new bit, if it wasn't under SourceSafe, then ask if they want to add it...
				//
				if (GetYesNo(va("File \"%s\" is not under SourceSafe control, add to database?",filename)))
				{
					if ( SS_Add( filename ))
					{
						Sys_Printf("(File was added to SourceSafe Ok)\n");

						// check it out as well...
						//
						if (SS_CheckOut( filename ))
						{
							Sys_Printf ("(File checked out ok)\n");				
						}
						else
						{
							ASSERT(0);	// I want to know if this ever happens
							Sys_Printf ("( Error during file checkout! )\n");							
						}
					}
					else
					{
						ErrorBox( va("Error adding file \"%s\" to SourceSafe",filename));
					}
				}
			}
		}
	}

	StatusText(NULL);

	return b;
}

BOOL CAssimilateDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	CString strFileName = lpszPathName;

	Filename_AccountForLOD(strFileName, giLODLevelOverride);	// this is actually junk now, should lose all this LODoverride stuff

	return CDocument::OnSaveDocument(strFileName);
}


// remember these two from session to session, maybe write to registry sometime?...
//
bool	gbPreValidate = true;
CString strInitialBuildPath = "models/players";
void CAssimilateDoc::OnEditBuildDependant()
{
	CModel* curModel = m_modelList;
	if (curModel)
	{
		CString strStartDir = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();

		if (!strStartDir.GetLength())
		{
			// should never happen...
			//
			ErrorBox("Base path not known at this point. Prefs not setup?");
			return;
		}

		LPCSTR psCurrentGLAName = curModel->GetMakeSkelPath();

		if (psCurrentGLAName)
		{
			char sCurrentGLAName[1024];
			strcpy(sCurrentGLAName,psCurrentGLAName);
			strMustContainThisGLA = sCurrentGLAName;
			CBuildAll dlgBuildAll(strInitialBuildPath, gbPreValidate);

			if (dlgBuildAll.DoModal() == IDOK)
			{		
				dlgBuildAll.GetData(strInitialBuildPath, gbPreValidate);

				strStartDir += strInitialBuildPath;
				strStartDir.Replace("/","\\");
				while (strStartDir.Replace("\\\\","\\")){}	

				if (gbPreValidate)
				{
					gbQueryGoAhead = false;
					gpsCARWashDirOverride = (LPCSTR) strStartDir;
					
					OnCarWashActual();
					gbQueryGoAhead = true;
					gpsCARWashDirOverride = NULL;

					if (gbCarwashErrorsOccured)
					{
						InfoBox("Build-All aborted because of errors");
						return;
					}
				}

				((CMainFrame*)AfxGetMainWnd())->StatusMessage("Validated Ok, building...");

				//////////////////////// largely block-pasted from CarWash.... :-)

				CWaitCursor wait;

				strCARsFound.Empty();
				iCARsFound = 0;
				strSkippedDirs.Empty();
				strSkippedFiles.Empty();

				// build up a list...
				//	
				R_CheckCARs( strStartDir, 0, strMustContainThisGLA);	//bool bBuildListOnly
				AlphaSortCARs();	// important to do them in alpha-order during build, because of "_humanoid" - type dirs.
				((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");

				// ok, now ready to begin pass 2...
				//
				CString strReport;
				if (!iCARsFound)
				{			
					ASSERT(0);
					strReport = "No suitable .CAR files found for processing!\n\n";

					if (!strSkippedDirs.IsEmpty())
					{
						strReport+= "Skipped Dirs:\n\n";
						strReport+= strSkippedDirs;
					}

					if (!strSkippedFiles.IsEmpty())
					{
						strReport+= "\n\nSkipped Files:\n\n";
						strReport+= strSkippedFiles;
					}
					ErrorBox(strReport);
				}
				else
				{
					//----------------
					gbCarWash_DoingScan = true;
					strCarWashErrors.Empty();
					//----------------

					CString strTotalErrors;

					strReport = "Processed files:\n\n";
					for (int i=0; i<iCARsFound; i++)
					{
						CString strThisFile = strCARsFound;
						int iLoc = strThisFile.Find("\n");
						if (iLoc>=0)
						{
							strThisFile = strThisFile.Left(iLoc);
							strCARsFound= strCARsFound.Mid(iLoc+1);

							strReport += strThisFile + "\n";

							((CMainFrame*)AfxGetMainWnd())->StatusMessage(va("Scanning File %d/%d: %s",i+1,iCARsFound,(LPCSTR)strThisFile));

							if (1)//FileUsesGLAReference(strThisFile, sCurrentGLAName))
							{
								OnNewDocument();				
								if (OnOpenDocument_Actual(strThisFile, false))
								{
									if (gbParseError)
									{
										strTotalErrors += va("\nParse error in file \"%s\"\n",(LPCSTR)strThisFile);
										break;
									}
									else
									{
										m_strPathName = strThisFile;	// fucking stupid MFC doesn't set this!!!!!!!!!!!!!
										bool bSuccess = Build(	false,	// bool bAllowedToShowSuccessBox, 
																0,		// int iLODLevel
																true	// bool bSkipSave
																);				

										if (!strCarWashErrors.IsEmpty())
										{
											// "something is wrong..."	:-)
											//
											strTotalErrors += va("\nError in file \"%s\":\n\n%s\n",strThisFile,strCarWashErrors);
										}
										strCarWashErrors.Empty();

										if (!bSuccess)
											break;
									}
								}
								else
								{
									strTotalErrors += va("\nUnable to open file \"%s\"\n",(LPCSTR)strThisFile);
									break;
								}
							}
							else
							{
								// this CAR file doesn't use the current GLA name, so ignore it
							}
						}
						else
						{
							ASSERT(0);
							strThisFile.Insert(0,"I fucked up, the following line didn't seem to have a CR:  (tell me! -Ste)\n\n");
							ErrorBox(strThisFile);
						}
					}

					//----------------
					gbCarWash_DoingScan = false;
					//----------------


					OnNewDocument();	// trash whatever was loaded last

			//		strReport = "Processed files:\n\n";
			//		strReport+= strCARsFound;
					strReport+= "\n\nSkipped Dirs:\n\n";
					strReport+= strSkippedDirs;
					strReport+= "\n\nSkipped Files:\n\n";
					strReport+= strSkippedFiles;

					if (strTotalErrors.IsEmpty())
					{
						strReport.Insert(0,"(No additional errors found)\n\n");
					}
					else
					{
						strReport+= "\n\nAdditional errors will now be sent to Notepad!...";
					}
					InfoBox(strReport);

					if (!strTotalErrors.IsEmpty())
					{
						strTotalErrors.Insert(0, "The following errors occured during build...\n\n");
						SendToNotePad(strTotalErrors, "build_errors.txt");
					}
				}
			}
		}
		else
		{
			ErrorBox("Currently loaded model does not make a skeleton, so there are no dependants\n\n\nDUH!!");
		}
	}
	else
	{
		ErrorBox("No model loaded to build dependants of!\n\n\nDUH!!");
	}

	((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");
}

void CAssimilateDoc::OnEditBuildall() 
{
//	if (!GetYesNo("Safety Feature:  Do you really want to rebuild every CAR file in the whole game?"))
//		return;

	// validity-check...
	//
	CString strStartDir = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();	

	if (!strStartDir.GetLength())
	{
		ErrorBox("Base path not known at this point. Prefs not setup?");
		return;
	}

	CBuildAll dlgBuildAll(strInitialBuildPath, gbPreValidate);

	if (dlgBuildAll.DoModal() == IDOK)
	{		
		dlgBuildAll.GetData(strInitialBuildPath, gbPreValidate);

		strStartDir += strInitialBuildPath;
		strStartDir.Replace("/","\\");
		while (strStartDir.Replace("\\\\","\\")){}	

		if (gbPreValidate)
		{
			gbQueryGoAhead = false;
			gpsCARWashDirOverride = (LPCSTR) strStartDir;
			OnCarWash();
			gbQueryGoAhead = true;
			gpsCARWashDirOverride = NULL;

			if (gbCarwashErrorsOccured)
			{
				InfoBox("Build-All aborted because of errors");
				return;
			}
		}

		((CMainFrame*)AfxGetMainWnd())->StatusMessage("Validated Ok, building...");

		//////////////////////// largely block-pasted from CarWash.... :-)

		CWaitCursor wait;

		strCARsFound.Empty();
		iCARsFound = 0;
		strSkippedDirs.Empty();
		strSkippedFiles.Empty();

		// build up a list...
		//	
		R_CheckCARs( strStartDir, 0, "" );	//bool bBuildListOnly
		AlphaSortCARs();	// important to do them in alpha-order during build, because of "_humanoid" - type dirs.
		((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");

		// ok, now ready to begin pass 2...
		//
		CString strReport;
		if (!iCARsFound)
		{			
			ASSERT(0);
			strReport = "No suitable .CAR files found for processing!\n\n";

			if (!strSkippedDirs.IsEmpty())
			{
				strReport+= "Skipped Dirs:\n\n";
				strReport+= strSkippedDirs;
			}

			if (!strSkippedFiles.IsEmpty())
			{
				strReport+= "\n\nSkipped Files:\n\n";
				strReport+= strSkippedFiles;
			}
			ErrorBox(strReport);
		}
		else
		{
			//----------------
			gbCarWash_DoingScan = true;
			strCarWashErrors.Empty();
			//----------------

			CString strTotalErrors;

			strReport = "Processed files:\n\n";
			for (int i=0; i<iCARsFound; i++)
			{
				CString strThisFile = strCARsFound;
				int iLoc = strThisFile.Find("\n");
				if (iLoc>=0)
				{
					strThisFile = strThisFile.Left(iLoc);
					strCARsFound= strCARsFound.Mid(iLoc+1);

					strReport += strThisFile + "\n";

					((CMainFrame*)AfxGetMainWnd())->StatusMessage(va("Scanning File %d/%d: %s",i+1,iCARsFound,(LPCSTR)strThisFile));

					OnNewDocument();				
					if (OnOpenDocument_Actual(strThisFile, false))
					{
						if (gbParseError)
						{
							strTotalErrors += va("\nParse error in file \"%s\"\n",(LPCSTR)strThisFile);
							break;
						}
						else
						{
							m_strPathName = strThisFile;	// fucking stupid MFC doesn't set this!!!!!!!!!!!!!
							bool bSuccess = Build(	false,	// bool bAllowedToShowSuccessBox, 
													0,		// int iLODLevel
													true	// bool bSkipSave
													);				

							if (!strCarWashErrors.IsEmpty())
							{
								// "something is wrong..."	:-)
								//
								strTotalErrors += va("\nError in file \"%s\":\n\n%s\n",strThisFile,strCarWashErrors);
							}
							strCarWashErrors.Empty();

							if (!bSuccess)
								break;
						}
					}
					else
					{
						strTotalErrors += va("\nUnable to open file \"%s\"\n",(LPCSTR)strThisFile);
						break;
					}
				}
				else
				{
					ASSERT(0);
					strThisFile.Insert(0,"I fucked up, the following line didn't seem to have a CR:  (tell me! -Ste)\n\n");
					ErrorBox(strThisFile);
				}
			}

			//----------------
			gbCarWash_DoingScan = false;
			//----------------


			OnNewDocument();	// trash whatever was loaded last

	//		strReport = "Processed files:\n\n";
	//		strReport+= strCARsFound;
			strReport+= "\n\nSkipped Dirs:\n\n";
			strReport+= strSkippedDirs;
			strReport+= "\n\nSkipped Files:\n\n";
			strReport+= strSkippedFiles;

			if (strTotalErrors.IsEmpty())
			{
				strReport.Insert(0,"(No additional errors found)\n\n");
			}
			else
			{
				strReport+= "\n\nAdditional errors will now be sent to Notepad!...";
			}
			InfoBox(strReport);

			if (!strTotalErrors.IsEmpty())
			{
				strTotalErrors.Insert(0, "The following errors occured during build...\n\n");
				SendToNotePad(strTotalErrors, "build_errors.txt");
			}
		}
	}

	((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");
}



void SS_DisposingOfCurrent(LPCSTR psFileName, bool bDirty)
{
	if (psFileName[0])
	{
		LPCSTR filename = psFileName;	// compile laziness
		#undef Sys_Printf
		#define Sys_Printf(blah)
		if ( SS_FunctionsAvailable() )
		{
			if ( SS_IsUnderSourceControl( filename ) )
			{
				if ( SS_IsCheckedOutByMe( filename ))
				{
					if (bDirty)
					{
						// if 'need_save' then the user has clicked ok-to-lose-changes, so...
						//
						if ( GetYesNo( va("Since you've decided to lose changes on the file:\n\n\"%s\"\n\n...do you want to Undo Checkout as well?",filename)))
						{
							if (SS_UndoCheckOut( filename ))
							{
								Sys_Printf ("(Undo Checkout performed on map)\n");
							}
							else
							{
								ErrorBox("Undo Checkout failed!\n");
							}
						}
					}
					else
					{
						// if !'need_save' here then the user has saved this out already, so prompt for check in...
						//
						if ( GetYesNo( va("Since you've finished with the file:\n\n\"%s\"\n\n...do you want to do a Check In?",filename)))
						{
							if ( SS_CheckIn( filename ))
							{
								//Sys_Printf ("(CheckIn performed on map)\n");
							}
							else
							{
								ErrorBox("CheckIn failed!\n");
							}
						}
					}
				}
			}
		}
	}
}



BOOL CAssimilateDoc::OnOpenDocument_Actual(LPCTSTR lpszPathName, bool bCheckOut) 
{
	SS_DisposingOfCurrent(m_strPathName, !!IsModified());

	if (bCheckOut)
	{
		// checkout the new file?
		LPCSTR filename = lpszPathName;	// compile-laziness :-)
		if ( SS_FunctionsAvailable() )
		{
			if ( SS_IsUnderSourceControl( filename ) )
			{
				if ( SS_IsCheckedOut( filename ))
				{
					if ( !SS_IsCheckedOutByMe( filename ))
					{
						CString strCheckOuts;
						int iCount;
						
						if (SS_ListCheckOuts( filename, strCheckOuts, iCount ))
						{
							if (!GetYesNo( va("Warning: File \"%s\" is checked out by:\n\n%s\n... Continue loading? ",filename,(LPCSTR) strCheckOuts)))
							{
								return FALSE;
							}
						}
					}
					else
					{
						//InfoBox ("(You own this file under SourceSafe)\n");				
					}
				}
				else
				{
					if ( GetYesNo( va("The file \"%s\"\n\n...is under SourceSafe control, check it out now?",filename) ))
					{
						if (SS_CheckOut( filename ))
						{
							//InfoBox ("(File checked out ok)\n");				
						}
						else
						{
							WarningBox( va("( Problem encountered during check out of file \"%s\" )",filename) );
						}
					}
				}
			}
			else
			{
				//InfoBox ("(This file is not under SourceSafe control)\n");				
			}
		}
	}

	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	return TRUE;
}


BOOL CAssimilateDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	return OnOpenDocument_Actual(lpszPathName, true) ;
}

void CAssimilateDoc::OnCloseDocument() 
{
	SS_DisposingOfCurrent(m_strPathName, !!IsModified());
	
	CDocument::OnCloseDocument();
}

void CAssimilateDoc::OnViewFramedetailsonadditionalsequences() 
{
	gbViewFrameDetails_Additional = !gbViewFrameDetails_Additional;
	UpdateAllViews(NULL, AS_FILESUPDATED, NULL);
}

void CAssimilateDoc::OnUpdateViewFramedetailsonadditionalsequences(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(gbViewFrameDetails_Additional);	
}

void CAssimilateDoc::OnUpdateEditBuilddependant(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());			
}



bool RunApp(LPCSTR psAppCommand)
{
	CString strExec = psAppCommand;	// eg "start q:\\bin_nt\\behaved.exe";
			strExec.Replace("/","\\");	// otherwise it only works under NT...

	char sBatchFilename[512];
	
	GetTempPath(sizeof(sBatchFilename), sBatchFilename);
	strcat(sBatchFilename,"~temp.bat");

	FILE *handle = fopen(sBatchFilename,"wt");
	fprintf(handle,strExec);
	fprintf(handle,"\n");
	fclose(handle);

	STARTUPINFO startupinfo;
	PROCESS_INFORMATION	 ProcessInformation;

	GetStartupInfo (&startupinfo);

	BOOL ret = CreateProcess(sBatchFilename,
						//batpath,		// pointer to name of executable module 
						NULL,			// pointer to command line string
						NULL,			// pointer to process security attributes 
						NULL,			// pointer to thread security attributes 
						FALSE,			// handle inheritance flag 
						0 /*DETACHED_PROCESS*/,		// creation flags 
						NULL,			// pointer to new environment block 
						NULL,			// pointer to current directory name 
						&startupinfo,	// pointer to STARTUPINFO 
						&ProcessInformation 	// pointer to PROCESS_INFORMATION  
						);
//	remove(sBatchFilename);	// if you do this, the CreateProcess fails, presumably it needs it for a few seconds

	return !!ret;
}


void CAssimilateDoc::OnEditLaunchmodviewoncurrent() 
{
	char sExecString[MAX_PATH];

	sprintf(sExecString,"start %s.glm",Filename_WithoutExt(m_strPathName));

	if (RunApp(sExecString))
	{
		// ok...
		//		
	}
	else
	{
		ErrorBox(va("CreateProcess() call \"%s\" failed!\n\n(let me know about this -Ste)",sExecString));
	}
}

void CAssimilateDoc::OnUpdateEditLaunchmodviewoncurrent(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!!GetNumModels());
}
