// Model.cpp

#include "StdAfx.h"
#include "Includes.h"
#include <assert.h>

#define sANIMATION_CFG_NAME "animation.cfg"
#define sANIMATION_PRE_NAME "animation.pre"

bool gbReportMissingASEs = true;
int giFixUpdatedASEFrameCounts = YES;


CModel::CModel()
{
	m_bCurrentUserSelection = false;
}

CModel::~CModel()
{
}

void CModel::Delete()
{
	while(m_comments != NULL)
	{
		CComment* curComment = m_comments;
		m_comments = curComment->GetNext();
		curComment->Delete();
	}
	while(m_sequences != NULL)
	{
		CSequence* curSequence = m_sequences;
		m_sequences = curSequence->GetNext();
		curSequence->Delete();
	}
	if (m_name != NULL)
	{
		free(m_name);
		m_name = NULL;
	}
	if (m_path != NULL)
	{
		free(m_path);
		m_path = NULL;
	}
	if (m_psSkelPath != NULL)
	{
		free(m_psSkelPath);
		m_psSkelPath = NULL;
	}
	if (m_psMakeSkelPath != NULL)
	{
		free(m_psMakeSkelPath);
		m_psMakeSkelPath = NULL;
	}
	m_curSequence = NULL;

	PCJList_Clear();	// not really necessary, but useful reminder

	delete this;
}

CModel* CModel::Create(CComment* comments)
{
	CModel* retval = new CModel();
	retval->Init(comments);
	return retval;
}

bool CModel::DoProperties()
{
	bool dirty = false;

//	if (IsGhoul2())
//	{
//		InfoBox("This properties page is for params not relevant for Ghoul2 models");
//	}
//	else
	{		
		CPropertySheet* propSheet = new CPropertySheet(m_name);

		CModelPropPage* propPage = new CModelPropPage();
		propPage->m_model = this;
		propPage->m_soilFlag = &dirty;
		propSheet->AddPage(propPage);

		for (int i=0; i<PCJList_GetEntries(); i++)
		{
			propPage->AddPCJEntry(PCJList_GetEntry(i));
		}

		propSheet->DoModal();

		delete propPage;
		delete propSheet;
	}

	return dirty;
}

void CModel::SetNext(CModel* next)
{
	m_next= next;
}

CModel* CModel::GetNext()
{
	return m_next;
}

bool CModel::ContainsFile(LPCSTR psFilename)
{	
	if (m_sequences == NULL)
	{
		return false;
	}
	else
	{
		CSequence* curSequence = m_sequences;
		while(curSequence)
		{
			if (stricmp(curSequence->GetPath(),psFilename)==0)
				return true;
			curSequence = curSequence->GetNext();
		}		
	}

	return false;
}

void CModel::AddSequence(CSequence* sequence)
{
	if (m_sequences == NULL)
	{
		m_sequences = sequence;
	}
	else
	{
		CSequence* curSequence = m_sequences;
		while(curSequence->GetNext() != NULL)
		{
			curSequence = curSequence->GetNext();
		}
		curSequence->SetNext(sequence);
	}
}

void CModel::DeleteSequence(CSequence* deleteSequence)
{
	// linklist is only 1-way, so we need to find the stage previous to this (if any)...
	//
	CSequence* prevSequence = NULL;
	CSequence* scanSequence = GetFirstSequence();

	while (scanSequence && scanSequence != deleteSequence)
	{
		prevSequence = scanSequence;
		scanSequence = scanSequence->GetNext();
	}
	if (scanSequence == deleteSequence)
	{
		// we found it, so was this the first sequence in the list?
		//
		if (prevSequence)
		{
			prevSequence->SetNext(scanSequence->GetNext());	// ...no
		}
		else
		{
			m_sequences = scanSequence->GetNext();			// ...yes
		}
		scanSequence->Delete();
	}
}



// func for qsort to callback, returns:
//
// <0 elem1 less than elem2 
//  0 elem1 equivalent to elem2 
// >0 elem1 greater than elem2 
//
int ModelSequenceCompareFunc( const void *arg1, const void *arg2 )
{
	CSequence *seq1 = (CSequence *) *(CSequence **)arg1;
	CSequence *seq2 = (CSequence *) *(CSequence **)arg2;

	return (seq1->m_iSequenceNumber - seq2->m_iSequenceNumber);	

/*	if (seq1->m_iSequenceNumber < seq2->m_iSequenceNumber)
		return -1;

	if (seq1->m_iSequenceNumber > seq2->m_iSequenceNumber)
		return  1;

	return 0;
	*/
}

// change the sequences around in the model till each one is in the position specified by it's member: m_iSequenceNumber
//
void CModel::ReOrderSequences()
{
	typedef vector<CSequence*> sequences_t; sequences_t sequences;

	// add sequences to list...
	//
	CSequence *curSequence = m_sequences;
	while (curSequence)
	{
		sequences.push_back(curSequence);
		curSequence = curSequence->GetNext();
	}

	// re-order sequences...
	//
	qsort( (void *)&sequences[0], (size_t)(sequences.size()), sizeof(CSequence *), ModelSequenceCompareFunc );

	// now rebuild links...
	//
	int iTotMasterSequences = GetTotMasterSequences();	// this needs to be eval'd here, you can't do it in the for-next below
	m_sequences = NULL;
	for (int i=0; i<iTotMasterSequences; i++)
	{
		curSequence = sequences[i];
		curSequence->SetNext(NULL);

		AddSequence(curSequence);
	}

	Resequence();
}

// a niceness feature so the popup anim enum dialog picker can ask which enums are already in use...
//
// (now updated to return a int instead of a bool, therefore can be used to check for duplicates)
//
int CModel::AnimEnumInUse(LPCSTR psAnimEnum)
{
	int iCount = 0;
	if (strlen(psAnimEnum))	// added for G2 models, which don't necessarily use enums yet
	{		
		CSequence *curSequence = GetFirstSequence();

		while (curSequence)
		{
			if (!strcmp(psAnimEnum,curSequence->GetEnum()))
			{
				iCount++;// return true;
			}

			// new bit, ask the additional sequences as well...
			//
			for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
			{
				if (!strcmp(psAnimEnum,curSequence->AdditionalSeqs[i]->GetEnum()))
				{
					iCount++;// return true;
				}
			}

			curSequence = curSequence->GetNext();
		}
	}
	return iCount;//false;
}

CSequence* CModel::GetFirstSequence()
{
	return m_sequences;
}

int CModel::GetTotMasterSequences()
{
	int tot = 0;
	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		tot++;
		curSequence = curSequence->GetNext();
	}
	return tot;
}

int CModel::GetTotSequences()
{
	int tot = 0;
	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		tot++;

		for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
		{
			if (curSequence->AdditionalSeqs[i]->AdditionalSequenceIsValid())
				tot++;
		}

		curSequence = curSequence->GetNext();
	}
	return tot;
}

void CModel::Resequence(bool bReScanASEFiles /* = false */)
{
	CWaitCursor wait;
	CRect Rect;
//	((CAssimilateApp*)AfxGetApp())->m_pMainWnd->GetWindowRect(&Rect);
//	CPoint Point = Rect.CenterPoint();

	CProgressCtrl *pProgress = NULL;

	if (bReScanASEFiles && ((CAssimilateApp*)AfxGetApp())->m_pMainWnd)
	{
		pProgress = new CProgressCtrl;
		bool bOK = !!pProgress->Create(	WS_CHILD|WS_VISIBLE|PBS_SMOOTH,		// DWORD dwStyle, 
									CRect(100,100,200,200),				// const RECT& rect, 
									((CAssimilateApp*)AfxGetApp())->m_pMainWnd,	// CWnd* pParentWnd, 
									1									// UINT nID 
									);
		if (!bOK)
		{
			delete pProgress;
			pProgress = NULL;
		}
	}
	
	int iTotMasterSequences = GetTotMasterSequences();
	if (pProgress)
	{			
		pProgress->SetRange(0,iTotMasterSequences);
	}
	int iSequenceNumber=0;

	int curFrame = 0;
	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		if (pProgress)
		{
			pProgress->SetPos(iSequenceNumber++);
//			pProgress->SetWindowText(va("Sequence %d/%d  (%s)",iSequenceNumber,iTotMasterSequences,curSequence->GetPath()));
			wait.Restore();
		}
		
		// mark current enums as valid or not...
		//
		curSequence->SetValidEnum(((CAssimilateApp*)AfxGetApp())->ValidEnum(curSequence->GetEnum()));
//--------		
		for (int _i=0; _i<MAX_ADDITIONAL_SEQUENCES; _i++)
		{
			CSequence *additionalSeq = curSequence->AdditionalSeqs[_i];

			additionalSeq->SetValidEnum(((CAssimilateApp*)AfxGetApp())->ValidEnum(additionalSeq->GetEnum()));
		}
//---------

		if ( bReScanASEFiles )
		{
			// new code, first of all check for changed framecounts (ie updated ASE file), and update sequence if nec...
			//
			CString nameASE = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
					nameASE+= curSequence->GetPath();

			if (!FileExists(nameASE))
			{
				if (gbCarWash_DoingScan)
				{
					strCarWashErrors += va("Model file missing: \"%s\"\n",nameASE);
				}
				else
				{
					if ( gbReportMissingASEs )
					{	
						gbReportMissingASEs = GetYesNo(va("Model file missing: \"%s\"\n\nContinue recieving this message?",nameASE));
					}
				}
			}
			else
			{
				int iStartFrame, iFrameCount, iFrameSpeed;

				iFrameCount = curSequence->GetFrameCount();	// default it in case we skip an XSI read
				iFrameSpeed = curSequence->GetFrameSpeed();	// default it in case we cache this file

				curSequence->ReadASEHeader( nameASE, iStartFrame, iFrameCount, iFrameSpeed, true);	// true = can skip XSI read

				if ( iFrameCount != curSequence->GetFrameCount() )
				{
					if (gbCarWash_DoingScan)
					{
						strCarWashErrors += va("file: \"%s\" has a framecount of %d, but .CAR file says %d\n",nameASE,iFrameCount,curSequence->GetFrameCount());
					}
					else
					{
						// don't mention it if the current count is zero, it's probably a new anim we've just added...
						//
						if ( curSequence->GetFrameCount() )
						{
							if (giFixUpdatedASEFrameCounts == YES || giFixUpdatedASEFrameCounts == NO)
							{
								CYesNoYesAllNoAll query(	va("Model file: \"%s\"",nameASE),
															"",
															va("... has a framecount of %d instead of %d as the QDT/CAR file says",iFrameCount, curSequence->GetFrameCount()),
															"",
															"",
															"Do you want me to fix this?"
															);
								giFixUpdatedASEFrameCounts = query.DoModal();
								//gbReportUpdatedASEFrameCounts = GetYesNo(va("Model file: \"%s\"\n\n... has a framecount of %d instead of %d as the QDT/CAR file says (so I'll update it).\n\nContinue recieving this message?",nameASE, iFrameCount, curSequence->GetFrameCount()));
							}
						}

						// update the sequence?...
						//
						if (giFixUpdatedASEFrameCounts == YES || giFixUpdatedASEFrameCounts == YES_ALL 
							|| !curSequence->GetFrameCount()	// update: I think this should be here?
							)
						{
							curSequence->SetFrameCount( iFrameCount );
						}
					}
				}
			}

			// findmeste:	this no longer seems to do anything under JK2, presumablt EF1-only?
#if 0
			// now try to do any auto-associate between the ASE filename base and the existing enums, 
			//	so if we find (eg) /...../...../CROUCH.ASE and we have BOTH_CROUCH then auto-set the enum to BOTH_CROUCH
			//
			CString stringASEName = nameASE;
			Filename_BaseOnly(stringASEName);	// now = (eg) "falldeath" or "injured" etc 			

			for (int i=0; ; i++)
			{
				LPCSTR p = ((CAssimilateApp*)AfxGetApp())->GetEnumEntry(i);	

				if (!p)		// EOS?
					break;

				CString stringEnum = p;

				// note, I could check stuff like "IsEnumSeperator(LPCSTR lpString)" on <p>, but you'd never
				//	have one of those enums assigned to a sequence anyway.

				char *psEnumPosAfterUnderScore = strchr(stringEnum,'_');
				if (psEnumPosAfterUnderScore++)	// check it, and skip to next char 
				{
					// does this enum match the ASE name?
					//
					if ( !stricmp( psEnumPosAfterUnderScore, stringASEName ) )
					{
						// ok, we've found a good candidate, so set it...  (no need for query-prev code, but I wanted to)
						//
						if ( strcmp( curSequence->GetEnum(), stringEnum))
						{
//							InfoBox( va("(temp notify box)\n\nEnum auto-assign of \"%s\" to ASE \"%s\"  (prev enum was \"%s\")",stringEnum,nameASE,curSequence->GetEnum()));
							curSequence->SetEnum(stringEnum);
						}
					}
				}
				else
				{						
					// this should never happen...
					//
					if (gbCarWash_DoingScan)
					{
						strCarWashErrors += va("found an anim enum with no underscore: \"%s\"\n",stringEnum);
					}
					else
					{
						ASSERT(0);
						ErrorBox(va("Error! Somehow I found an anim enum with no underscore: \"%s\"",stringEnum));
					}
				}
			}
#endif
		}

		// More bollox for Gummelt... :-)
		// now do the other freaky trick (you'd better be grateful for all this Mike!!! <g>), which is:
		//
		// If you find the substring DEATH in this (master) sequence's enum, then ensure that the first *additional*
		//	sequence of it is set to be the corresponding DEAD enum, but using the last frame only (and non-looping)
		//
		// (... or something...)
		//
		{	// keep scope local for neatness

			if ( strstr (curSequence->GetEnum(), "DEATH") )
			{
				// scan this sequence's additional sequences for a DEAD of the same basic type...
				//
				CString stringEnumDEAD = curSequence->GetEnum();

				ASSERT(!IsEnumSeperator(stringEnumDEAD));

				stringEnumDEAD.Replace("DEATH","DEAD");

				// 1st, is there even a corresponding DEAD enum in the global enum table that we can look for...
				//
				CString stringEnum;
				bool bEnumFound = false;
				for (int iEnumEntry=0; !bEnumFound; iEnumEntry++)
				{
					LPCSTR p = ((CAssimilateApp*)AfxGetApp())->GetEnumEntry(iEnumEntry);	

					if (!p)		// EOS?
						break;

					stringEnum = p;

					// note, I could check stuff like "IsEnumSeperator(LPCSTR lpString)" on <p>, but you'd never
					//	have one of those enums assigned to a sequence anyway.

					// does this enum match the one we've built?
					//
					if ( !stricmp( stringEnum, stringEnumDEAD ) )
					{
						bEnumFound = true;
					}
				}

				if ( bEnumFound )
				{
					// ok, there *is* one of these, so let's scan this sequence's additional sequences to see if we've
					//	got it...
					//
					CSequence *additionalSeq;	// outside FOR scope
					for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
					{
						additionalSeq = curSequence->AdditionalSeqs[i];

						if (additionalSeq->AdditionalSequenceIsValid())
						{
							if (!strcmp(additionalSeq->GetEnum(),stringEnum))
							{
								break;	// we've found one!
							}
						}
					}

					// if we didn't find one, NULL the ptr
					//
					if ( i == MAX_ADDITIONAL_SEQUENCES)
					{
						additionalSeq = NULL;
					}

					// did we find one? (or did it have the wrong info in?)
					//
					if ( additionalSeq == NULL // didn't find one
						|| additionalSeq->GetFrameCount()!=1
						|| additionalSeq->GetLoopFrame() !=-1
						|| additionalSeq->GetStartFrame()!= curSequence->GetFrameCount()-1
						|| additionalSeq->GetFrameSpeed()!= curSequence->GetFrameSpeed()
						)
					{
						// find a slot to add this new sequence to, or use the faulty one...
						//
						if (additionalSeq == NULL)
						{
							for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
							{
								additionalSeq = curSequence->AdditionalSeqs[i];

								if (!additionalSeq->AdditionalSequenceIsValid())
								{
									break;	// found an unused slot
								}
							}
						}

						// so have we got a slot to work with?
						//
						if ( additionalSeq == NULL )
						{
							if (gbCarWash_DoingScan)
							{
								strCarWashErrors += va( "Fuck!!!: I need an 'additional sequence' slot free in the entry: \"%s\" to generate a DEAD seq, but there isn't one spare. Edit this yourself later.\n",curSequence->GetPath());
							}
							else
							{
								ErrorBox( va( "Fuck!!!\n\nI need an 'additional sequence' slot free in the ASE:\n\n\"%s\"\n\n... to generate a DEAD seq, but there isn't one spare. Edit this yourself later.",curSequence->GetPath()));
							}
						}
						else
						{
							additionalSeq->SetStartFrame( curSequence->GetFrameCount()-1 );
							additionalSeq->SetFrameCount( 1 );
							additionalSeq->SetLoopFrame (-1 );
							additionalSeq->SetFrameSpeed( curSequence->GetFrameSpeed() );
							additionalSeq->SetEnum ( stringEnumDEAD );
						}
					}
				}
			}
		}

		curSequence->SetTargetFrame(curFrame + curSequence->GetStartFrame());	// slightly more legal than just (curFrame)

		// update: now set any additional sequences within it...
		//
		for (int i=0; i<MAX_ADDITIONAL_SEQUENCES; i++)
		{
			curSequence->AdditionalSeqs[i]->SetTargetFrame(curFrame + curSequence->AdditionalSeqs[i]->GetStartFrame());
		}

		curFrame += curSequence->GetFrameCount();
		curFrame += curSequence->GetGenLoopFrame()?1:0;	// findme:  is this right?  I hate this system
		curSequence = curSequence->GetNext();
	}
	m_totFrames = curFrame;

	ghAssimilateView->GetDocument()->SetModifiedFlag();
//	(ghAssimilateView->GetDocument()->IsModified())?OutputDebugString("modified\n"):OutputDebugString("same\n");

	if (pProgress)
	{
		delete pProgress;
		pProgress = 0;
	}
}

void CModel::AddComment(CComment* comment)
{
	if (m_curSequence != NULL)
	{
		m_curSequence->AddComment(comment);
		return;
	}
	if (m_comments == NULL)
	{
		m_comments = comment;
	}
	else
	{
		CComment* curComment = m_comments;
		while (curComment->GetNext() != NULL)
		{
			curComment = curComment->GetNext();
		}
		curComment->SetNext(comment);
	}
}

CComment* CModel::GetFirstComment()
{
	return m_comments;
}

CComment* CModel::ExtractComments()
{
	CComment* retval = m_comments;
	m_comments = NULL;
	return retval;
}

void CModel::SetName(LPCTSTR name)
{
	if (m_name != NULL)
	{
		free(m_name);
	}
	if (name == NULL)
	{
		m_name = NULL;
	}
	else
	{
		m_name = (char*)malloc(strlen(name) + 1);
		strcpy(m_name, name);
	}
}

LPCTSTR CModel::GetName()
{
	return m_name;
}

void CModel::SetPath(LPCTSTR name)
{
	if (m_path != NULL)
	{
		free(m_path);
	}
	if (name == NULL)
	{
		m_path = NULL;
	}
	else
	{
		m_path = (char*)malloc(strlen(name) + 1);
		strcpy(m_path, name);
	}
}

LPCTSTR CModel::GetPath()
{
	return m_path;
}

void CModel::DeriveName(LPCTSTR fromname)
{
	SetPath(fromname);
	if (fromname == NULL)
	{
		if (m_name != NULL)
		{
			free(m_name);
			m_name = NULL;
		}
		return;
	}
	CString name = fromname;
	int loc = name.ReverseFind('/');
	if (loc > -1)
	{
		name = name.Left(loc);
	}
	loc = name.ReverseFind('/');
	if (loc > -1)
	{
		name = name.Right(name.GetLength() - loc - 1);
	}
	SetName(name);
}

void CModel::Init(CComment* comments)
{
	m_comments = comments;
	m_next = NULL;
	m_sequences = NULL;
	m_curSequence = NULL;
	m_name = NULL;
	m_path = NULL;
	m_psSkelPath = NULL;
	m_psMakeSkelPath = NULL;
	SetOrigin(0, 0, 0);
	SetParms(-1, -1, 0, 1);
	m_iType		= TK_AS_CONVERTMDX_NOASK;
	m_bSmooth	= false;
	m_bLoseDupVerts = false;
	m_bMakeSkin	= false;
	m_fScale	= 1.0f;
	m_bIgnoreBaseDeviations = false;
	m_bSkew90	= false;
	m_bNoSkew90	= false;	
	m_bKeepMotion = false;
	m_bPreQuat = false;

	PCJList_Clear();
}

void CModel::SetConvertType(int iType)
{
	m_iType = iType;
}

int CModel::GetConvertType(void)
{
	return m_iType;
}

bool CModel::IsGhoul2(void)
{
	if (GetConvertType() == TK_AS_CONVERTMDX ||
		GetConvertType() == TK_AS_CONVERTMDX_NOASK
		)
	{
		return true;
	}

	return false;
}


void CModel::SetMakeSkin(bool bMakeSkin)
{
	m_bMakeSkin = bMakeSkin;
}

bool CModel::GetMakeSkin(void)
{
	return m_bMakeSkin;
}

void CModel::SetSmooth(bool bSmooth)
{
	m_bSmooth = bSmooth;
}

bool CModel::GetSmooth(void)
{
	return m_bSmooth;
}

void CModel::SetKeepMotion(bool bKeepMotion)
{
	m_bKeepMotion = bKeepMotion;
}

bool CModel::GetKeepMotion(void)
{
	return m_bKeepMotion;
}

void CModel::SetLoseDupVerts(bool bLoseDupVerts)
{
	m_bLoseDupVerts = bLoseDupVerts;
}

bool CModel::GetLoseDupVerts(void)
{
	return m_bLoseDupVerts;
}

void CModel::SetScale(float fScale)
{
	m_fScale = fScale;
}

float CModel::GetScale(void)
{
	return m_fScale;
}


bool CModel::GetPreQuat(void)
{
	return m_bPreQuat;
}

void CModel::SetPreQuat(bool bPreQuat)
{
	m_bPreQuat = bPreQuat;
}


void CModel::PCJList_Clear()
{
	m_vPCJList.clear();
}

void CModel::PCJList_AddEntry(LPCSTR psEntry)
{
	m_vPCJList.push_back(psEntry);
}

int	CModel::PCJList_GetEntries()
{
	return m_vPCJList.size();
}

LPCSTR CModel::PCJList_GetEntry(int iIndex)
{		
	if (iIndex < PCJList_GetEntries())
	{
		return m_vPCJList[iIndex].c_str();
	}

	assert( iIndex < PCJList_GetEntries() );
	return "";
}


// temporary!!!!!!!
void CModel::SetIgnoreBaseDeviations(bool bIgnore)
{
	m_bIgnoreBaseDeviations = bIgnore;
}

bool CModel::GetIgnoreBaseDeviations(void)
{
	return m_bIgnoreBaseDeviations;
}

void CModel::SetSkew90(bool bSkew90)
{
	m_bSkew90 = bSkew90;
}

bool CModel::GetSkew90(void)
{
	return m_bSkew90;
}

void CModel::SetNoSkew90(bool bNoSkew90)
{
	m_bNoSkew90 = bNoSkew90;
}

bool CModel::GetNoSkew90(void)
{
	return m_bNoSkew90;
}

/*
void CModel::SetSkelPath(LPCSTR psPath)
{
	if (m_psSkelPath != NULL)
	{
		free(m_psSkelPath);
	}

	if (psPath == NULL)
	{
		m_psSkelPath = NULL;
	}
	else
	{
		m_psSkelPath = (char*) malloc (strlen(psPath)+1);
		strcpy(m_psSkelPath, psPath);
	}
}

LPCSTR CModel::GetSkelPath(void)
{
	return m_psSkelPath;	// warning, may be NULL or blank
}
*/
void CModel::SetMakeSkelPath(LPCSTR psPath)
{
	if (m_psMakeSkelPath != NULL)
	{
		free(m_psMakeSkelPath);
	}

	if (psPath == NULL)
	{
		m_psMakeSkelPath = NULL;
	}
	else
	{
		m_psMakeSkelPath = (char*) malloc (strlen(psPath)+1);
		strcpy(m_psMakeSkelPath, psPath);
		strlwr(m_psMakeSkelPath);
	}
}

LPCSTR CModel::GetMakeSkelPath(void)
{
	return m_psMakeSkelPath;	// warning, may be NULL or blank
}	


void CModel::SetOrigin(int x, int y, int z)
{
	SetOriginX(x);
	SetOriginY(y);
	SetOriginZ(z);
}

void CModel::SetOriginX(int x)
{
	m_originx = x;
}

void CModel::SetOriginY(int y)
{
	m_originy = y;
}

void CModel::SetOriginZ(int z)
{
	m_originz = z;
}

int CModel::GetOriginX()
{
	return m_originx;
}

int CModel::GetOriginY()
{
	return m_originy;
}

int CModel::GetOriginZ()
{
	return m_originz;
}

void CModel::SetParms(int skipStart, int skipEnd, int totFrames, int headFrames)
{
	SetTotFrames(totFrames);
}

void CModel::SetTotFrames(int value)
{
	m_totFrames = value;
}

int CModel::GetTotFrames()
{
	return m_totFrames;
}

void CModel::SetUserSelectionBool(bool bSelected)
{
	m_bCurrentUserSelection = bSelected;
}

bool CModel::GetUserSelectionBool()
{
	return m_bCurrentUserSelection;
}

bool CModel::WriteExternal(bool bPromptForNames, bool& bCFGWritten)
{
	bCFGWritten = false;

	CString filename;
	if (bPromptForNames)
	{
		//XXXXXXXXXXXXXX
//		CFileDialog dialog(FALSE, ".cfg", /*m_name*/"D:\\Source\\StarTrek\\Code-DM\\baseef\\models\\players2\\imperial\\animation_new.cfg", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Config Data Files (*.cfg)|*.cfg|All Files (*.*)|*.*||", NULL);

		CString strInitialPrompt(ghAssimilateView->GetDocument()->GetPathName());
		Filename_RemoveFilename(strInitialPrompt);
		strInitialPrompt.Replace("/","\\");
		strInitialPrompt += "\\";
		strInitialPrompt += sANIMATION_CFG_NAME;
		
		CFileDialog dialog(FALSE, ".cfg", strInitialPrompt, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Config Data Files (*.cfg)|*.cfg|All Files (*.*)|*.*||", NULL);
		if (dialog.DoModal() != IDOK)
		{
			return false;
		}
		filename = dialog.GetPathName();	// eg. {"Q:\quake\baseq3\models\players\ste_assimilate_test\ste_assimilate_test.cfg"}
	}
	else
	{
		filename = ((CAssimilateApp*)AfxGetApp())->GetQuakeDir();
		filename+= GetPath();
		filename.MakeLower();
		filename.Replace('\\', '/');
		int loc = filename.Find(m_name);//"/root");
		if (loc>=0)
		{
			filename = filename.Left(loc+strlen(m_name));
		}
		// dup the dirname to use as the model name... (eg "/.../.../klingon" becomes "/.../.../klingon/klingon"
		//loc = filename.ReverseFind('/');		
		//filename += filename.Mid(loc);
		//filename += ".cfg";
		filename += "/"; filename += sANIMATION_CFG_NAME;	
	}
	CTxtFile* file = CTxtFile::Create(filename);
	if (file == NULL || !file->IsValid())
	{
		ErrorBox(va("Error creating file \"%s\"!",filename));
		return false;
	}

	// new bit, check for the existance of an animation.pre file, which means export this in Q3 format (rather than trek)
	//
	CString strQ3FormatCheckName(filename);
	Filename_RemoveFilename(strQ3FormatCheckName);
	strQ3FormatCheckName += "\\";
	strQ3FormatCheckName += sANIMATION_PRE_NAME;
	strQ3FormatCheckName.Replace("/","\\");

	bool bExportFormatIsQuake3Multiplayer = //FileExists(strQ3FormatCheckName);
											((CAssimilateApp*)AfxGetApp())->GetMultiPlayerMode();	

	CString strPrePend;
	if (bExportFormatIsQuake3Multiplayer)
	{
		// multi-player format, check for optional animation.pre file...
		//
		FILE *fhPRE = fopen(strQ3FormatCheckName, "rt");
		
		if (fhPRE)
		{
			//
			// read all the lines in this file and just write them straight to the output file...
			//
			char sLine[16384];
			char *psLine;
			CString strTrimmed;

			while ((psLine = fgets( sLine, sizeof(sLine), fhPRE ))!=NULL)
			{
				strTrimmed = psLine;
				strTrimmed.Replace("\n","");
				strTrimmed.TrimRight();
				strTrimmed.TrimLeft();

				file->Writeln(strTrimmed);
			}

			if (ferror(fhPRE))
			{
				ErrorBox(va("Error during reading of file \"%s\"!\n\n( this shouldn't happen )",(LPCSTR)strQ3FormatCheckName));
			}

			fclose(fhPRE);
		}		

		file->Writeln("");
		file->Writeln("//");
		file->Writeln("// Format:  targetFrame, frameCount, loopFrame, frameSpeed");
		file->Writeln("//");
	}
	else
	{
		// single-player format...
		//
		CString commentLine;
		CTime time = CTime::GetCurrentTime();
		commentLine.Format("// %s %d frames; %d sequences; updated %s", filename, m_totFrames, GetTotSequences(), time.Format("%H:%M %A, %B %d, %Y"));
		file->Writeln(commentLine);
		//
		// the Writeln functions I have to call don't handle "\n" chars properly because of being opened in binary mode
		//	(sigh), so I have to explicitly call the Writeln() functions to output CRs... :-(
		//
		file->Writeln("//");
		file->Writeln("// Format:  enum, targetFrame, frameCount, loopFrame, frameSpeed");	
		file->Writeln("//");
	}

	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		curSequence->WriteExternal(this, file, bExportFormatIsQuake3Multiplayer);
		curSequence = curSequence->GetNext();
	}
	file->Delete();

	if (HasGLA())
	{
		unlink(filename);	// zap it, since it's meaningless here (only has one seq/enum: the whole GLA)
	}
	else
	{
		bCFGWritten = true;
	}

	return true;
}

bool CModel::HasGLA()
{
	CSequence* curSequence = GetFirstSequence();
	while (curSequence)
	{
		if (curSequence->IsGLA())
		{
			return true;
		}
		curSequence = curSequence->GetNext();
	}

	return false;
}

// returns NULL else name string
//
LPCSTR CModel::GLAName(void)
{
	CSequence* curSequence = GetFirstSequence();
	while (curSequence)
	{
		if (curSequence->IsGLA())
		{
			return curSequence->GetName();
		}
		curSequence = curSequence->GetNext();
	}

	return NULL;
}


// should only be called once model is known to be in a sorted state or results are meaningless.
//
// either param can be NULL if not interested in them...
//
void CModel::GetMasterEnumBoundaryFrameNumbers(int *piFirstFrameAfterBOTH, int *piFirstFrameAfterTORSO)
{
	ENUMTYPE prevET = ET_INVALID;
	int iFirstFrameAfterBOTH = 0;
	int iFirstFrameAfterTORSO= 0;	
	
	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		ENUMTYPE thisET = curSequence->GetEnumType();

		// update any frame markers first...
		//
		if (prevET == ET_BOTH && thisET != ET_BOTH)
		{
			iFirstFrameAfterBOTH = curSequence->GetTargetFrame();
			iFirstFrameAfterTORSO= curSequence->GetTargetFrame();	// set this as well in case there are no TORSOs at all
		}

		if (prevET == ET_TORSO && thisET != ET_TORSO)
		{
			iFirstFrameAfterTORSO= curSequence->GetTargetFrame();	
		}

		prevET = thisET;

		curSequence = curSequence->GetNext();
	}
	// bug fix, if there are no leg frames at all, then we need to check if the ...AfterTORSO marker needs moving...
	//
	if (prevET == ET_BOTH)
	{
		iFirstFrameAfterBOTH = GetTotFrames();
		iFirstFrameAfterTORSO= GetTotFrames();
	}
	if (prevET == ET_TORSO)
	{		
		iFirstFrameAfterTORSO = GetTotFrames();
	}

	if (piFirstFrameAfterBOTH)
	{
		*piFirstFrameAfterBOTH = iFirstFrameAfterBOTH;
	}

	if (piFirstFrameAfterTORSO)
	{
		*piFirstFrameAfterTORSO= iFirstFrameAfterTORSO;
	}
}

void CModel::Write(CTxtFile* file)
{
	file->Write("$");
	file->Writeln(CAssimilateDoc::GetKeyword(TK_AS_GRABINIT, TABLE_QDT));

	if (!HasGLA())
	{
		if (GetScale() != 1.0f)
		{
			file->Write("$");
			file->Write(CAssimilateDoc::GetKeyword(TK_AS_SCALE, TABLE_QDT)," ");
			file->Writeln(va("%g",GetScale()));
		}

		if (GetKeepMotion())
		{
			file->Write("$");
			file->Writeln(CAssimilateDoc::GetKeyword(TK_AS_KEEPMOTION, TABLE_QDT));
		}

		for (int iPCJ=0; iPCJ < PCJList_GetEntries(); iPCJ++)
		{
			file->Write("$");
			file->Write(CAssimilateDoc::GetKeyword(TK_AS_PCJ, TABLE_QDT)," ");
			file->Writeln( PCJList_GetEntry(iPCJ) );
		}
	}

	CComment* curComment = m_comments;
	while(curComment != NULL)
	{
		curComment->Write(file);
		curComment = curComment->GetNext();
	}

	bool bFirstSeqWritten = false;
	CSequence* curSequence = m_sequences;
	while(curSequence != NULL)
	{
		curSequence->Write(file, !bFirstSeqWritten && GetPreQuat());
		curSequence = curSequence->GetNext();

		bFirstSeqWritten = true;
	}

	file->Writeln("$", CAssimilateDoc::GetKeyword(TK_AS_GRABFINALIZE, TABLE_QDT));

	if (m_path != NULL)
	{
		file->Write("$", CAssimilateDoc::GetKeyword(GetConvertType(), TABLE_QDT));
		CString path = m_path;
	
		int loc = path.Find("/base");
		if (loc > -1)
		{
			path = path.Right(path.GetLength() - loc - 5);
			loc = path.Find("/");
			path = path.Right(path.GetLength() - loc - 1);
		}
		if (!path.GetLength())	// check that some dopey artist hasn't use the name "base" on the right hand side
		{
			path = m_path;
		}

		file->Write(" ", path, " ");

		// params stuff...
		//
		if (IsGhoul2())
		{
			if	(GetMakeSkin())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_MAKESKIN,	TABLE_CONVERT), " ");
			}
			if	(GetSmooth())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_SMOOTH,	TABLE_CONVERT), " ");
			}
			if (GetLoseDupVerts())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_LOSEDUPVERTS, TABLE_CONVERT), " ");
			}
			if  (GetIgnoreBaseDeviations())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_IGNOREBASEDEVIATIONS, TABLE_CONVERT), " ");
			}
			if	(GetSkew90())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_SKEW90,	TABLE_CONVERT), " ");
			}
			if	(GetNoSkew90())
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_NOSKEW90, TABLE_CONVERT), " ");
			}
			/*
			if	(GetSkelPath() && strlen(GetSkelPath()))
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_SKEL,		TABLE_CONVERT), " ");
				file->Write(GetSkelPath(), " ");
			}
			*/
			if	(GetMakeSkelPath() && strlen(GetMakeSkelPath()))
			{
				file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_MAKESKEL,	TABLE_CONVERT), " ");
				file->Write(GetMakeSkelPath(), " ");
			}		

			// params below not used for ghoul2
		}
		else
		{
			if (giLODLevelOverride)
			{
				file->Write(va("-lod %d ",giLODLevelOverride));
			}
			//xxxxxxxxxxxxxxxxxxxxxxxxxxx

			file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_PLAYERPARMS, TABLE_CONVERT), " ");
			file->Write(0);//m_skipStart+1);	// ignore these, but don't want to update parser and have invalid prev files
			file->Space();
	//		file->Write(m_skipEnd);	// this param no longer used
	//		file->Space();
			file->Write(0);//m_skipEnd);//max upper frames);
			file->Space();
	//		file->Write(m_headFrames);	// this param no longer used
	//		file->Space();
			
		}

		//xxxxxxxxxxxxxxxxxxxxxx

		if (m_originx || m_originy || m_originz)
		{
			file->Write("-", CAssimilateDoc::GetKeyword(TK_AS_ORIGIN, TABLE_CONVERT), " ");
			file->Write(m_originx);
			file->Space();
			file->Write(m_originy);
			file->Space();
			file->Write(m_originz);
		}

		file->Writeln();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CModelPropPage property page

IMPLEMENT_DYNCREATE(CModelPropPage, CPropertyPage)

CModelPropPage::CModelPropPage() : CPropertyPage(CModelPropPage::IDD)
{
	//{{AFX_DATA_INIT(CModelPropPage)
	m_bSkew90 = FALSE;
	m_bSmooth = FALSE;	
	m_strSkelPath = _T("");
	m_iOriginX = 0;
	m_iOriginY = 0;
	m_iOriginZ = 0;
	m_fScale = 0.0f;
	m_bMakeSkin = FALSE;
	m_bLoseDupVerts = FALSE;
	m_bMakeSkel = FALSE;
	m_strNewPCJ = _T("");
	m_bKeepMotion = FALSE;
	m_bPreQuat = FALSE;
	//}}AFX_DATA_INIT

	m_PCJList.clear();
}

CModelPropPage::~CModelPropPage()
{
}

void CModelPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModelPropPage)
	DDX_Check(pDX, IDC_CHECK_SKEW90, m_bSkew90);
	DDX_Check(pDX, IDC_CHECK_SMOOTH_ALL, m_bSmooth);
	DDX_Text(pDX, IDC_EDIT_SKELPATH, m_strSkelPath);
	DDX_Text(pDX, IDC_EDIT_ORIGINX, m_iOriginX);
	DDX_Text(pDX, IDC_EDIT_ORIGINY, m_iOriginY);
	DDX_Text(pDX, IDC_EDIT_ORIGINZ, m_iOriginZ);
	DDX_Text(pDX, IDC_EDIT_SCALE, m_fScale);
	DDX_Check(pDX, IDC_CHECK_MAKESKIN, m_bMakeSkin);
	DDX_Check(pDX, IDC_CHECK_LOSEDUPVERTS, m_bLoseDupVerts);
	DDX_Check(pDX, IDC_CHECK_MAKESKEL, m_bMakeSkel);
	DDX_Text(pDX, IDC_EDIT_PCJ, m_strNewPCJ);
	DDX_Check(pDX, IDC_CHECK_KEEPMOTIONBONE, m_bKeepMotion);
	DDX_Check(pDX, IDC_CHECK_PREQUAT, m_bPreQuat);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModelPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CModelPropPage)
	ON_BN_CLICKED(IDC_CHECK_MAKESKEL, OnCheckMakeskel)
	ON_BN_CLICKED(IDC_BUTTON_DELPCJ, OnButtonDelpcj)
	ON_BN_CLICKED(IDC_BUTTON_PCJ, OnButtonPcj)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelPropPage message handlers

void CModelPropPage::OnOK() 
{
	if (!m_bMakeSkel && !m_strSkelPath.IsEmpty())
	{
		// actually, this SHOULD just cancel the OnOK and stay in the dialog (according to the windoze docs,
		//	but of course being microsoft, it doesn't bloody work, and just cancels, so I adapted the message...
		//
		if (!GetYesNo("Warning, you have a make-skeleton path entered, but 'makeskel' is OFF, this will lose that path info when this dialog closes.\n\nProceed? ('NO' is the same as clicking 'CANCEL')"))
			return;
	}
/*	
// these are all irrelevant now...
//
	m_model->SetHeadFrames(m_headFrames);
	m_model->SetSkipEnd(m_skipEnd);
	m_model->SetSkipStart(m_skipStart);
*/
	m_model->SetOriginX(m_iOriginX);
	m_model->SetOriginY(m_iOriginY);
	m_model->SetOriginZ(m_iOriginZ);

	m_model->SetSkew90(!!m_bSkew90);
	m_model->SetSmooth(!!m_bSmooth);
	m_model->SetLoseDupVerts(!!m_bLoseDupVerts);
	m_model->SetMakeSkin(!!m_bMakeSkin);
//	m_model->SetSkelPath(m_bMakeSkel?(LPCSTR)m_strSkelPath:"");
	m_model->SetMakeSkelPath(m_bMakeSkel?(LPCSTR)m_strSkelPath:"");

	m_model->SetScale(m_fScale);
	m_model->SetKeepMotion(!!m_bKeepMotion);
	m_model->SetPreQuat(!!m_bPreQuat);

	m_model->PCJList_Clear();
	for (int i=0; i<GetPCJEntries(); i++)
	{
		m_model->PCJList_AddEntry(GetPCJEntry(i));
	}


	*m_soilFlag = true;

	CPropertyPage::OnOK();
}

void CModelPropPage::PopulatePCJList(void)
{
	CListBox *pListBox = (CListBox *) GetDlgItem(IDC_LIST_PCJ);
	if (pListBox)
	{
		pListBox->ResetContent();

		for (int i = 0; i<m_PCJList.size(); i++)
		{
			pListBox->InsertString(-1, (LPCSTR) m_PCJList[i]);
		}
	}
}

BOOL CModelPropPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
/*
// these are all irrelevant now...
//		
	m_headFrames = m_model->GetHeadFrames();
	m_skipEnd = m_model->GetSkipEnd();
	m_skipStart = m_model->GetSkipStart();
	m_totFrames.Format("%d", m_model->GetTotFrames());
	m_totSequences.Format("%d", m_model->GetTotSequences());
*/
	m_iOriginX = m_model->GetOriginX();
	m_iOriginY = m_model->GetOriginY();
	m_iOriginZ = m_model->GetOriginZ();

	m_bSkew90	= m_model->GetSkew90();
	m_bSmooth	= m_model->GetSmooth();
	m_bLoseDupVerts = m_model->GetLoseDupVerts();
	m_bMakeSkin	= m_model->GetMakeSkin();
	m_strSkelPath = m_model->GetMakeSkelPath();
	if (m_strSkelPath.IsEmpty())
	{
//		m_strSkelPath = m_model->GetSkelPath();
	}
	m_bMakeSkel = (m_model->GetMakeSkelPath() && strlen(m_model->GetMakeSkelPath()))?TRUE:FALSE;

	m_fScale	= m_model->GetScale();
	m_bKeepMotion = m_model->GetKeepMotion();
	m_bPreQuat = m_model->GetPreQuat();

	PopulatePCJList();

	UpdateData(DATA_TO_DIALOG);

	HandleItemGreying();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE	
}


void CModelPropPage::DelPCJEntry(int iIndex)
{
	if (iIndex < m_PCJList.size())
	{
		m_PCJList.erase(m_PCJList.begin() + iIndex);
	}
}

void CModelPropPage::AddPCJEntry(LPCSTR psPCJName)
{
	CString strTemp(psPCJName);
			strTemp.Replace(" ","");
			strTemp.Replace("\t","");
			
	m_PCJList.push_back(strTemp);
}

int CModelPropPage::GetPCJEntries(void)
{
	return m_PCJList.size();
}

LPCSTR CModelPropPage::GetPCJEntry(int iIndex)
{		
	if (iIndex < m_PCJList.size())
	{
		return (LPCSTR) m_PCJList[iIndex];
	}

	assert(0);
	return NULL;
}

void CModelPropPage::HandleItemGreying(void)
{
	UpdateData(DIALOG_TO_DATA);
	GetDlgItem(IDC_EDIT_SCALE)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_EDIT_SKELPATH)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_STATIC_SKELPATH)->EnableWindow(m_bMakeSkel);

	GetDlgItem(IDC_PCJ_STATIC)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_LIST_PCJ)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_EDIT_PCJ)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_BUTTON_PCJ)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_BUTTON_DELPCJ)->EnableWindow(m_bMakeSkel);
	GetDlgItem(IDC_CHECK_KEEPMOTIONBONE)->EnableWindow(m_bMakeSkel);	

	Invalidate();
}

void CModelPropPage::OnCheckMakeskel() 
{
	UpdateData(DIALOG_TO_DATA);

	// first time we turn it on, we should make up a reasonable default name...
	//
	if (m_bMakeSkel && m_strSkelPath.IsEmpty())
	{
		// basically I'm just going to use the dir name as the GLA name base as well...
		//
		CString strSuggestedPath(m_model->GetPath());			// eg. "models/players/blah/root"
		int iLoc = strSuggestedPath.ReverseFind('/');
		if (iLoc>=0)
		{
			strSuggestedPath = strSuggestedPath.Left(iLoc);		// eg. "models/players/blah"

			iLoc = strSuggestedPath.ReverseFind('/');
			if (iLoc >= 0)
			{
				CString strDir(strSuggestedPath.Mid(iLoc+1));	// eg. "blah"

				strSuggestedPath += "/";
				strSuggestedPath += strDir;						// eg. "models/players/blah/blah"

				m_strSkelPath = strSuggestedPath;

				UpdateData(DATA_TO_DIALOG);
			}
		}
	}

	HandleItemGreying();	
}

void CModelPropPage::OnButtonPcj() 
{
	UpdateData(DIALOG_TO_DATA);

	if (!m_strNewPCJ.IsEmpty())
	{
		AddPCJEntry(m_strNewPCJ);
		PopulatePCJList();

		UpdateData(DIALOG_TO_DATA);
		m_strNewPCJ = "";
		UpdateData(DATA_TO_DIALOG);
	}	
}

void CModelPropPage::OnButtonDelpcj() 
{
	CListBox *pListBox = (CListBox *) GetDlgItem(IDC_LIST_PCJ);
	if (pListBox)
	{
		int iCurSel = pListBox->GetCurSel();
		if (iCurSel != LB_ERR)
		{
			DelPCJEntry(iCurSel);
			PopulatePCJList();
		}
	}
}
