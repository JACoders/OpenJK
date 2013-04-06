#ifndef MULTIEFFECT_H
#define MULTIEFFECT_H

//#include "common_headers.h"
//#include "ifc.h"

////-----------
///	MultiEffect
//---------------
//	CImmCompoundEffect makes no assumption that its contained effects form a more
//	complex, single effect. MultiEffect makes this assumption and provides member
//	functions available in CImmEffect to operate on this "complex" effect.
//
//	Do not instantiate. (Do not call constructor)
//	Instead, cast existing CImmCompoundEffect* to MultiEffect*
//	Utility functions are specific to the needs of this system.
//
class MultiEffect : public CImmCompoundEffect
{
public:
	// dummy constructor
	MultiEffect() : CImmCompoundEffect( NULL, 0, NULL ) {}	// Never call (cast instead)

	// CImmEffect extensions
	qboolean GetStatus( DWORD &Status );
	qboolean GetStartDelay( DWORD &StartDelay );
	qboolean GetDuration( DWORD &Duration );
	qboolean GetGain( DWORD &Gain );
	qboolean ChangeDuration( DWORD Duration );
	qboolean ChangeGain( DWORD Gain );
	qboolean ChangeStartDelay( DWORD StartDelay );

	// utility functions
	qboolean GetDelayEnd( DWORD &DelayEnd );
	qboolean IsBeyondStartDelay()
	{
		DWORD DelayEnd;
		return qboolean
		(	GetDelayEnd( DelayEnd )
		&&	DelayEnd < ::GetTickCount()		// Does not account for counter overflow.
		);
	}
	qboolean IsPlaying()
	{
		DWORD Status; 
		return qboolean( GetStatus( Status ) && (Status & IMM_STATUS_PLAYING) );
	}

	// CImmCompoundEffect overrides
	qboolean Start( DWORD dwIterations = IMM_EFFECT_DONT_CHANGE, DWORD dwFlags = 0 )
	{
		return qboolean
		(	IsBeyondStartDelay()
		&&	CImmCompoundEffect::Start( dwIterations, dwFlags )
		);
	}
};

#endif // MULTIEFFECT_H