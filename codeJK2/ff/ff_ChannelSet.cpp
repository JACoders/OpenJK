#include "common_headers.h"

#ifdef _IMMERSION

extern const char *gChannelName[];

////---------------------------
/// FFChannelSet::ParseChannels
//-------------------------------
//	This is the worst hack of a parser ever devised.
//
qboolean FFChannelSet::ParseChannels( const char *channels )
{
	if ( !channels )
		return qfalse;

	int channel;
	const char *pos;

	for
	(	pos = channels
	;	pos && sscanf( pos, "%d", &channel ) == 1
	;
	){
		int device;
		char *endpos;
		endpos = strchr( pos, ';' );

		if ( channel >= 0 && channel < FF_CHANNEL_MAX )
		{
			for
			(	pos = strchr( pos, ',' )
			;	pos && ( !endpos || pos < endpos ) && sscanf( pos, " ,%d", &device ) == 1
			;	pos = strchr( pos + 1, ',' )
			){
				if ( device >= 0 && device < mSet.size() )
				{
					for
					(	ChannelIterator itChannel( mChannel, channel )
					;	itChannel != mChannel.end()
					&&	(**itChannel).second != device	// found duplicate
					;	++itChannel
					);

					// Don't allow duplicates
					if ( itChannel == mChannel.end() )
					{
						FFChannelSet::Channel::value_type Value( channel, device );
						Value.second = device;
						mChannel.insert( Value );
					}
				}
			}
		}

		pos = ( endpos ? endpos + 1 : NULL);

	}

	// FIX ME -- return qfalse if there is a parse error
	return qtrue;
}

////----------------------
/// FFChannelSet::Register
//--------------------------
//
//	Assumptions:
//	*	'compound' is empty of effects and contains the desired channel prior to entry.
//
//	Parameters:
//	*	compound: its channel parameter is an input. its effect set is filled with registered
//		- effects. 'compound' should not contain any effects prior to this function call.
//	*	name: effect name to register in each FFSet on the channel
//	*	create: qtrue if FFSet should create the effect, qfalse if it should just look it up.
//
qboolean FFChannelSet::Register( ChannelCompound &compound, const char *name, qboolean create )
{
	for
	(	ChannelIterator itChannel( mChannel, compound.GetChannel() )
	;	itChannel != mChannel.end()
	;	++itChannel
	){
		MultiEffect *Effect;
		Effect = mSet[ (**itChannel).second ]->Register( name, create );
		if ( Effect )
			compound.GetSet().insert( Effect );
	}

	return qboolean( compound.GetSet().size() != 0 );
}

#ifdef FF_CONSOLECOMMAND

void FFChannelSet::GetDisplayTokens( TNameTable &Tokens )
{
	FFMultiSet::GetDisplayTokens( Tokens );
	Tokens.push_back( "channels" );
	Tokens.push_back( "devices" );
}


void FFChannelSet::Display( TNameTable &Unprocessed, TNameTable &Processed )
{
	FFMultiSet::Display( Unprocessed, Processed );

	for
	(	TNameTable::iterator itName = Unprocessed.begin()
	;	itName != Unprocessed.end()
	;
	){
		if ( stricmp( "channels", (*itName).c_str() ) == 0 )
		{
			Com_Printf( "[available channels]\n" );

			for
			(	int i = 0
			;	i < FF_CHANNEL_MAX
			;	i++
			){
				Com_Printf( "%d) %s  devices:", i, gChannelName[ i ] );
				for
				(	ChannelIterator itChannel( mChannel, i )
				;	itChannel != mChannel.end()
				;	++itChannel
				){
					Com_Printf( " %d", (**itChannel).second );
				}
				Com_Printf( "\n" );
			}

			Processed.push_back( *itName );
			itName = Unprocessed.erase( itName );
		}
		else if ( stricmp( "devices", (*itName).c_str() ) == 0 )
		{
			Com_Printf( "[initialized devices]\n" );

			for
			(	int i = 0
			;	i < mDevices->GetNumDevices()
			;	i++
			){
				char ProductName[ FF_MAX_PATH ];
				ProductName[ 0 ] = 0;
				mDevices->GetDevice( i )->GetProductName( ProductName, FF_MAX_PATH - 1 );
				Com_Printf( "%d) %s\n", i, ProductName );
			}

			Processed.push_back( *itName );
			itName = Unprocessed.erase( itName );
		}
		else
		{
			itName++;
		}
	}
}

#endif // FF_CONSOLECOMMAND

#endif // _IMMERSION