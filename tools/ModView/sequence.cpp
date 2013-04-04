// Filename:-	sequence.cpp
//
// code for animation sequences...
//


#include "stdafx.h"
#include "includes.h"
#include "model.h"
//
#include "sequence.h"





LPCSTR Sequence_CreateTreeName(Sequence_t *pSequence)
{
	static CString str;

	LPCSTR psSequenceName = (AppVars.bFullPathsInSequenceTreeitems && pSequence->sNameWithPath[0])? pSequence->sNameWithPath : Sequence_GetName(pSequence,true);//->sName;

	str = va("%s ( %d..%d ) ( # = %d )",
			String_EnsureMinLength(psSequenceName, 16),
					pSequence->iStartFrame, 
						pSequence->iStartFrame + pSequence->iFrameCount - 1,
								pSequence->iFrameCount
						);

	if (pSequence->iLoopFrame != -1)
	{
		str += va(" ( loopframe %d )", pSequence->iStartFrame + pSequence->iLoopFrame);
	}

	return (LPCSTR) str;
}

void Sequence_Clear(Sequence_t *pSequence)
{
	pSequence->sName[0] = '\0';
	pSequence->sNameWithPath[0] = '\0';
	pSequence->iStartFrame	= 0;
	pSequence->iFrameCount	= 0;
	pSequence->iLoopFrame	= -1;
	pSequence->bIsDefault	= false;
}


bool Sequence_FrameIsWithin(Sequence_t *pSequence, int iFrame)
{
	if (iFrame >= pSequence->iStartFrame && iFrame < pSequence->iStartFrame + pSequence->iFrameCount)
	{
		return true;
	}

	return false;
}


// a bit of a lame function, but oh well...
//
int Sequence_GetIndex( Sequence_t *pSequenceToFind, ModelContainer_t *pContainer )
{
	for (int i=0; i<pContainer->SequenceList.size(); i++)
	{
		Sequence_t *pSequence = &pContainer->SequenceList[i];

		if (pSequence == pSequenceToFind)
		{
			return i;
		}
	}

	return -1;
}


// C++ can't overload by return only, so swap the args as well from the one below...
//
int Sequence_DeriveFromFrame( ModelContainer_t *pContainer, int iFrame )
{
	for (int i=0; i<pContainer->SequenceList.size(); i++)
	{
		Sequence_t *pSequence = &pContainer->SequenceList[i];

		if (Sequence_FrameIsWithin(pSequence, iFrame))
		{
			return i;
		}
	}

	return -1;
}

Sequence_t* Sequence_DeriveFromFrame( int iFrame, ModelContainer_t *pContainer )
{	
	for (int i=0; i<pContainer->SequenceList.size(); i++)
	{
		Sequence_t *pSequence = &pContainer->SequenceList[i];

		if (Sequence_FrameIsWithin(pSequence, iFrame))
		{
			return pSequence;
		}
	}

	return NULL;
}

int Sequence_ReturnLongestSequenceNameLength(ModelContainer_t *pContainer)
{
	int iLongestStrlen = 0;

	for (int i=0; i<pContainer->SequenceList.size(); i++)
	{
		Sequence_t *pSequence = &pContainer->SequenceList[i];

		int iThisStrlen = strlen(Sequence_GetName(pSequence,true));
		if (iLongestStrlen < iThisStrlen)
			iLongestStrlen = iThisStrlen;
	}

	return iLongestStrlen;
}


// called by external apps via remote control...
//
LPCSTR Sequence_ReturnRemoteQueryString(Sequence_t *pSequence)
{
	static char sString[1024];

	sprintf(sString,"%s %d %d %d", pSequence->sName, pSequence->iStartFrame, pSequence->iFrameCount, pSequence->iLoopFrame);
	return sString;
}



Sequence_t*	Sequence_CreateDefault(int iNumFrames)
{
	static Sequence_t Sequence;

	Sequence_Clear(&Sequence);

	strcpy(Sequence.sName,"default");
	Sequence.iFrameCount= iNumFrames;
	Sequence.bIsDefault	= true;

	return &Sequence;
}


LPCSTR Sequence_GetName(Sequence_t *pSequence, bool bUsedForDisplay /* = false */)
{
	if (!bUsedForDisplay)
	{
		return pSequence->sName;
	}

	return va("%s%s",(pSequence->iFPS<0)?"-":" ",pSequence->sName);
}

////////////////// eof //////////////////

