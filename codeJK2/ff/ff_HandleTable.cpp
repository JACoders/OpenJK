#include "common_headers.h"

#ifdef _IMMERSION

////----------------------
/// FFHandleTable::Convert
//--------------------------
//
//
ffHandle_t FFHandleTable::Convert( ChannelCompound &compound, const char *name, qboolean create )
{
	ffHandle_t ff = FF_HANDLE_NULL;

	//	Reserve a handle for effects that failed to create.
	//	Rerouting channels to other devices may cause an effect to become lost.
	//	This assumes that FF_Register is always called with legitimate effect names.
	//	See CMD_FF_Play on how to handle possibly-bogus user input.
	//	(It does not call this function)
	if ( compound.GetSet().size() )
		ff = Convert( compound );
	else
	{
		for
		(	FFHandleTable::RegFail::iterator itRegFail = mRegFail.begin()
		;	itRegFail != mRegFail.end()
		&&	(*itRegFail).second != name
		;	itRegFail++
		);

		ff = 
		(	itRegFail != mRegFail.end()
		?	(*itRegFail).first
		:	FF_HANDLE_NULL
		);
	}

	if ( ff == FF_HANDLE_NULL )
	{
		mVector.push_back( compound );
		ff = mVector.size() - 1;

		// Remember effect name for future 'ff_restart' calls.
		if ( create && !compound.GetSet().size() )
			mRegFail[ ff ] = name;
	}

	return ff;
}

////----------------------
/// FFHandleTable::Convert
//--------------------------
//	Looks for 'compound' in the table.
//
//	Assumes:
//	*	'compound' is non-empty
//
//	Returns:
//		ffHandle_t
//
ffHandle_t FFHandleTable::Convert( ChannelCompound &compound )
{
	for
	(	int i = 1
	;	i < mVector.size()
	&&	mVector[ i ] != compound
	;	i++
	);

	return
	(	i < mVector.size()
	?	i
	:	FF_HANDLE_NULL
	);
}

////-----------------------------
/// FFHandleTable::GetFailedNames
//---------------------------------
//
//
qboolean FFHandleTable::GetFailedNames( TNameTable &NameTable )
{
	for
	(	RegFail::iterator itRegFail = mRegFail.begin()
	;	itRegFail != mRegFail.end()
	;	itRegFail++
	){
		NameTable[ (*itRegFail).first ] = (*itRegFail).second;
	}

	return qboolean( mRegFail.size() != 0 );
}

////--------------------------
/// FFHandleTable::GetChannels
//------------------------------
//
//
qboolean FFHandleTable::GetChannels( vector<int> &channel )
{
	//ASSERT( channel.size() >= mVector.size() );

	for
	(	int i = 1
	;	i < mVector.size()
	;	i++
	){
		channel[ i ] = mVector[ i ].GetChannel();
	}

	return qtrue;
}

const char *FFHandleTable::GetName( ffHandle_t ff )
{
	const char *result = NULL;

	if ( !mVector[ ff ].IsEmpty() )
	{
		result = mVector[ ff ].GetName();
	}
	else
	{
		RegFail::iterator itRegFail = mRegFail.find( ff );
		if ( itRegFail != mRegFail.end() )
			result = (*itRegFail).second.c_str();
	}

	return result;
}

#endif // _IMMERSION