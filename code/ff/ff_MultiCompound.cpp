#include "common_headers.h"

#ifdef _IMMERSION

////------------------
///	MultiCompound::Add
//----------------------
//	Insert a single compound effect if it does not already exist.
//	Only fails when parameter is NULL.
//
qboolean MultiCompound::Add( MultiEffect *effect )
{
	return effect ? ( mSet.insert( effect ), qtrue ) : qfalse;
}

////------------------
///	MultiCompound::Add
//----------------------
//	Merge set of compound effects with current set. NULL pointers are excluded.
//	Returns false if set contains any NULL pointers. 
//
qboolean MultiCompound::Add( Set &effect )
{
	qboolean result = qtrue;

	for 
	(	Set::iterator itSet = effect.begin()
	;	itSet != effect.end()
	;	itSet++
	){
		result &= Add( *itSet );
	}

	return result;
}

////--------------------
///	MultiCompound::Start
//------------------------
//	Analogous to CImmCompoundEffect::Start. Starts all contained compound effects.
//	Returns false if any effect returns false.
//
qboolean MultiCompound::Start()
{
	qboolean result = qtrue;

	for
	(	Set::iterator itSet = mSet.begin()
	;	itSet != mSet.end()
	;	itSet++
	){
		result &= (*itSet)->Start();
	}

	return qboolean
	(	result 
	&&	mSet.size() != 0
	);
}

qboolean MultiCompound::IsPlaying()
{
	for
	(	Set::iterator itSet = mSet.begin()
	;	itSet != mSet.end()
	;	itSet++
	){
		if ( !(*itSet)->IsPlaying() )
			return qfalse;
	}

	return qtrue;
}

////----------------------------
///	MultiCompound::EnsurePlaying
//--------------------------------
//	Starts any contained compound effects if they are not currently playing.
//	Returns false if any effect returns false or any are playing.
//
qboolean MultiCompound::EnsurePlaying()
{
	qboolean result = qtrue;

	if ( !IsPlaying() )
	{
		for
		(	Set::iterator itSet = mSet.begin()
		;	itSet != mSet.end()
		;	itSet++
		){
			result &= (*itSet)->Start();
		}
	}

	return qboolean
	(	result 
	&&	mSet.size() != 0
	);
}

////-------------------
///	MultiCompound::Stop
//-----------------------
//	Analogous to CImmCompoundEffect::Stop. Stops all contained compound effects.
//	Returns false if any effect returns false. 
//
qboolean MultiCompound::Stop()
{
	qboolean result = qtrue;

	for
	(	Set::iterator itSet = mSet.begin()
	;	itSet != mSet.end()
	;	itSet++
	){
		result &= qboolean( (*itSet)->Stop() );
	}

	return qboolean
	(	result 
	&&	mSet.size() != 0
	);
}

////-----------------------------
///	MultiCompound::ChangeDuration
//---------------------------------
//	Changes duration of all compounds.
//	Returns false if any effect returns false. 
//
qboolean MultiCompound::ChangeDuration( DWORD Duration )
{
	qboolean result = qtrue;

	for
	(	Set::iterator itSet = mSet.begin()
	;	itSet != mSet.end()
	;	itSet++
	){
		result &= (*itSet)->ChangeDuration( Duration );
	}

	return qboolean
	(	result 
	&&	mSet.size() != 0
	);
}

////-------------------------
///	MultiCompound::ChangeGain
//-----------------------------
//	Changes gain of all compounds.
//	Returns false if any effect returns false. 
//
qboolean MultiCompound::ChangeGain( DWORD Gain )
{
	qboolean result = qtrue;

	for
	(	Set::iterator itSet = mSet.begin()
	;	itSet != mSet.end()
	;	itSet++
	){
		result &= (*itSet)->ChangeGain( Gain );
	}

	return qboolean
	(	result 
	&&	mSet.size() != 0
	);
}

////--------------------------
///	MultiCompound::operator ==
//------------------------------
//	Returns qtrue if the sets are EXACTLY equal, including order. This is not good
//	in general. (Fix me)
//
qboolean MultiCompound::operator == ( MultiCompound &compound )
{
	Set &other = compound.mSet;
	qboolean result = qfalse;

	if ( mSet.size() == other.size() )
	{
		for
		(	Set::iterator itSet = mSet.begin(), itOther = other.begin()
		;	itSet != mSet.end()
		//&&	itOther != other.end()	// assumed since mSet.size() == other.size()
		&&	(*itSet) == (*itOther)
		;	itSet++, itOther++
		);

		result = qboolean( itSet == mSet.end() );
	}

	return result;
}

#endif // _IMMERSION