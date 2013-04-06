#ifndef FF_MULTICOMPOUND_H
#define FF_MULTICOMPOUND_H

//#include "common_headers.h"
#include "ff_MultiEffect.h"

////-------------
///	MultiCompound
//-----------------
//	MultiCompound is a container for MultiEffect pointers.
//	It is not a single, complex effect and should not be treated as such.
//
class MultiCompound
{
public:
	typedef set<MultiEffect*> Set;
protected:
	Set mSet;
public:
	MultiCompound()
	:	mSet() 
	{}

	MultiCompound( Set &compound ) 
	:	mSet()
	{
		Add( compound );
	}

	Set& GetSet() { return mSet; }
	qboolean Add( MultiEffect *Compound );
	qboolean Add( Set &compound );

	// CImmEffect iterations
	qboolean Start();
	qboolean Stop();
	qboolean ChangeDuration( DWORD Duration );
	qboolean ChangeGain( DWORD Gain );

	// Utilities
	qboolean IsEmpty() { return qboolean( mSet.size() == 0 ); }
	qboolean operator == ( MultiCompound &compound );
	qboolean operator != ( MultiCompound &compound )
	{
		return qboolean( !( (*this) == compound ) );
	}

	// Other iterations
	qboolean IsPlaying();
	qboolean EnsurePlaying();

};

#endif // FF_MULTICOMPOUND_H