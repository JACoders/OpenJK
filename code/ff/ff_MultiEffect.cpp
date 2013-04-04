#include "common_headers.h"

#ifdef _IMMERSION

////--------------------------
///	MultiEffect::GetStartDelay
//------------------------------
//	Determines the shortest start delay.
//
qboolean MultiEffect::GetStartDelay( DWORD &StartDelay )
{
	StartDelay = MAXDWORD;
	qboolean result = qtrue;

	int i,max;
	for
	(	i = 0, max = GetNumberOfContainedEffects()
	;	i < max
	;	i++
	){
		DWORD CurrentStartDelay;
		CImmEffect* pIE = GetContainedEffect( i );
		if ( pIE
		&&	 pIE->GetStartDelay( CurrentStartDelay )
		){
			StartDelay = Min( StartDelay, CurrentStartDelay );
		}
		else
		{
			result = qfalse;
		}
	}

	return qboolean
	(	result 
	&&	max > 0
	);
}

////------------------------
///	MultiEffect::GetDelayEnd
//----------------------------
//	Computes end of earliest start delay. Compare this value with ::GetTickCount()
//	to determine if any component waveform started playing on the device.
//
qboolean MultiEffect::GetDelayEnd( DWORD &DelayEnd )
{
	DelayEnd = MAXDWORD;
	qboolean result = qtrue;

	int i,max;
	for
	(	i = 0, max = GetNumberOfContainedEffects()
	;	i < max
	;	i++
	){
		DWORD StartDelay;
		CImmEffect* pIE = GetContainedEffect( i );
		if ( pIE 
		&&	 pIE->GetStartDelay( StartDelay )
		){
			DelayEnd = Min( DelayEnd, StartDelay + pIE->m_dwLastStarted );
		}
		else
		{
			result = qfalse;
		}
	}

	return qboolean
	(	result 
	&&	max > 0
	);
}

////---------------------------
///	MultiEffect::ChangeDuration
//-------------------------------
//	Analogous to CImmEffect::ChangeDuration. Changes duration of all component effects.
//	Returns false if any effect returns false. Attempts to change duration of all effects
//	regardless of individual return values.
//
qboolean MultiEffect::ChangeDuration( DWORD Duration )
{
	DWORD CurrentDuration;
	qboolean result = GetDuration( CurrentDuration );

	if ( result )
	{
		DWORD RelativeDuration = Duration - CurrentDuration;

		int i,max;
		for
		(	i = 0, max = GetNumberOfContainedEffects()
		;	i < max
		;	i++
		){
			IMM_ENVELOPE Envelope = {0};
			CImmEffect* pIE = GetContainedEffect( i );
			result &= qboolean
			(	pIE
			&&	pIE->GetDuration( CurrentDuration )
			&&	pIE->ChangeDuration( CurrentDuration + RelativeDuration )
			&&	(	!pIE->GetEnvelope( &Envelope )
				||	(	Envelope.dwAttackTime = ( CurrentDuration ? (DWORD)((float)Envelope.dwAttackTime * (float)Duration / (float)CurrentDuration) : 0 )
					,	Envelope.dwFadeTime = ( CurrentDuration ? (DWORD)((float)Envelope.dwFadeTime * (float)Duration / (float)CurrentDuration) : 0 )
					,	pIE->ChangeEnvelope( &Envelope )
					)
				)
			);
		}

		result &= qboolean( max > 0 );
	}

	return result;
}

////-----------------------
///	MultiEffect::ChangeGain
//---------------------------
//	Analogous to CImmEffect::ChangeGain. Changes gain of all component effects.
//	Returns false if any effect returns false. Attempts to change gain of all effects
//	regardless of individual return values.
//
qboolean MultiEffect::ChangeGain( DWORD Gain )
{
	DWORD CurrentGain;
	qboolean result = GetGain( CurrentGain );

	if ( result )
	{
		DWORD RelativeGain = Gain - CurrentGain;

		int i,max;
		for
		(	i = 0, max = GetNumberOfContainedEffects()
		;	i < max
		;	i++
		){
			CImmEffect* pIE = GetContainedEffect( i );
			result &= qboolean
			(	pIE
			&&	pIE->GetGain( CurrentGain )
			&&	pIE->ChangeGain( CurrentGain + RelativeGain )
			);
		}

		result &= qboolean( max > 0 );
	}

	return result;
}

////----------------------
///	MultiEffect::GetStatus
//--------------------------
//	Analogous to CImmEffect::GetStatus. ORs all status flags from all component effects.
//	Returns false if any effect returns false. Attempts to get status of all effects
//	regardless of individual return values.
//
qboolean MultiEffect::GetStatus( DWORD &Status )
{
	Status = 0;
	qboolean result = qtrue;

	int i,max;
	for
	(	i = 0, max = GetNumberOfContainedEffects()
	;	i < max
	;	i++
	){
		DWORD CurrentStatus;
		CImmEffect* pIE = GetContainedEffect( i );
		if ( pIE
		&&	 pIE->GetStatus( &CurrentStatus )
		){
			Status |= CurrentStatus;
		}
		else
		{
			result = qfalse;
		}
	}

	return qboolean
	(	result
	&&	max > 0
	);
}

qboolean MultiEffect::ChangeStartDelay( DWORD StartDelay )
{
	DWORD CurrentStartDelay;
	qboolean result = GetStartDelay( CurrentStartDelay );

	if ( result )
	{
		DWORD RelativeStartDelay = StartDelay - CurrentStartDelay;
		
		int i,max;
		for
		(	i = 0, max = GetNumberOfContainedEffects()
		;	i < max
		;	i++
		){
			CImmEffect* pIE = GetContainedEffect( i );
			result &= qboolean
			(	pIE
			&&	pIE->GetStartDelay( CurrentStartDelay )
			&&	pIE->ChangeStartDelay( CurrentStartDelay + RelativeStartDelay )
			);
		}

		result &= qboolean( max > 0 );
	}

	return result;
}

qboolean MultiEffect::GetDuration( DWORD &Duration )
{
	Duration = 0;
	qboolean result = qtrue;

	int i,max;
	for
	(	i = 0, max = GetNumberOfContainedEffects()
	;	i < max
	;	i++
	){
		DWORD CurrentDuration;
		CImmEffect* pIE = GetContainedEffect( i );
		if ( pIE
		&&	 pIE->GetDuration( CurrentDuration )
		){
			Duration = Max( Duration, CurrentDuration );
		}
		else
		{
			result = qfalse;
		}
	}

	return qboolean
	(	result
	&&	max > 0
	);
}

qboolean MultiEffect::GetGain( DWORD &Gain )
{
	Gain = 0;
	qboolean result = qtrue;

	int i,max;
	for
	(	i = 0, max = GetNumberOfContainedEffects()
	;	i < max
	;	i++
	){
		DWORD CurrentGain;
		CImmEffect* pIE = GetContainedEffect( i );
		if ( pIE
		&&	 pIE->GetGain( CurrentGain )
		){
			Gain = Max( Gain, CurrentGain );
		}
		else
		{
			result = qfalse;
		}
	}

	return qboolean
	(	result
	&&	max > 0
	);
}

#endif // _IMMERSION