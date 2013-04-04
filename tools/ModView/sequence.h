// Filename:-	sequence.h
//
//  code/structs for animation sequences...
//


#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "model.h"

LPCSTR		Sequence_CreateTreeName					(Sequence_t *pSequence);
void		Sequence_Clear							(Sequence_t *pSequence);
int			Sequence_GetIndex						(Sequence_t *pSequenceToFind, ModelContainer_t *pContainer );
Sequence_t* Sequence_DeriveFromFrame				( int iFrame, ModelContainer_t *pContainer );
// C++ can't overload by return only, so swap the args as well
int			Sequence_DeriveFromFrame				( ModelContainer_t *pContainer, int iFrame );
int			Sequence_ReturnLongestSequenceNameLength(ModelContainer_t *pContainer);
LPCSTR		Sequence_ReturnRemoteQueryString		(Sequence_t *pSequence);
bool		Sequence_FrameIsWithin(Sequence_t *pSequence, int iFrame);
Sequence_t*	Sequence_CreateDefault					(int iNumFrames);
LPCSTR		Sequence_GetName						(Sequence_t *pSequence, bool bUsedForDisplay = false );

#endif	// #ifndef SEQUENCE_H

/////////////// eof /////////////

