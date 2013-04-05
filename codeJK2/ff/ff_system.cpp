#include "common_headers.h"

#ifdef _IMMERSION

//#include "ff.h"
//#include "ff_ffset.h"
//#include "ff_compound.h"
//#include "ff_system.h"

extern cvar_t *ff_developer;

////--------------
/// FFSystem::Init
//------------------
//
//
qboolean FFSystem::Init( const char *channels )
{
	// kludgy restart mechanism
	typedef struct
	{	TNameTable name;
		vector<int> channel;
	}	TRestartInfo;
	TRestartInfo restart;

	if ( mInitialized )
	{
		restart.name.resize( mHandle.size(), "" );
		restart.channel.resize( mHandle.size(), FF_CHANNEL_MAX );

		mChannel.GetRegisteredNames( restart.name );
		mHandle.GetFailedNames( restart.name );
		mHandle.GetChannels( restart.channel );

		Shutdown();
	}

	mHandle.Init();

	if ( mConfig.Init( "fffx/fffx.cfg" )			// Process config file
	&&	 mChannel.Init( mConfig, channels ) )		// Init devices
	{
		if ( restart.name.size() > 1 )
		{
			for
			(	int i = 1
			;	i < restart.name.size()
			;	i++
			){	// ignore leading device-specific set name -- (may be switching devices)
				Register( mConfig.RightOfSet( restart.name[ i ].c_str() ), restart.channel[ i ] );
			}
		}
		else
			ffShake = Register( "fffx/player/shake", FF_CHANNEL_BODY );

		mInitialized = qtrue;
	}

	return mInitialized;
}

#ifdef FF_CONSOLECOMMAND

void FFSystem::GetDisplayTokens( TNameTable &Tokens )
{
	FFChannelSet::GetDisplayTokens( Tokens );
	if ( ff_developer->integer )
	{
		Tokens.push_back( "effects" );
	}
}

void FFSystem::Display( TNameTable &Unprocessed, TNameTable &Processed )
{
	for
	(	TNameTable::iterator itName = Unprocessed.begin()
	;	itName != Unprocessed.end()
	;
	){
		if ( stricmp( "effects", (*itName).c_str() ) == 0 )
		{
			if ( ff_developer->integer )
			{
				Com_Printf( "[registered effects]\n" );

				TNameTable EffectNames;
				int total = 0;
				Channel::Set &ffSet = mChannel.GetSets();

				for
				(	int i = 0
				;	i < ffSet.size()
				;	i++
				){
					char ProductName[ FF_MAX_PATH ];
					*ProductName = 0;
					ffSet[ i ]->GetDevice()->GetProductName( ProductName, FF_MAX_PATH - 1 );
					Com_Printf( "%s...\n", ProductName );

					EffectNames.clear();
					EffectNames.resize( mHandle.size(), "" );
					ffSet[ i ]->GetRegisteredNames( EffectNames );

					for
					(	int j = 1
					;	j < EffectNames.size()
					;	j++
					){
						if ( EffectNames[ j ].length() )
							Com_Printf( "%3d) \"%s\" channel=%d\n", total++, EffectNames[ j ].c_str(), mHandle[ j ].GetChannel() );
					}
				}

				EffectNames.clear();
				EffectNames.resize( mHandle.size(), "" );
				
				if ( mHandle.GetFailedNames( EffectNames ) )
				{
					Com_Printf( "Failed Registrants...\n" );
					for
					(	int j = 1
					;	j < EffectNames.size()
					;	j++
					){
						if ( EffectNames[ j ].length() )
							Com_Printf( "%3d) \"%s\" channel=%d\n", total++, EffectNames[ j ].c_str(), mHandle[ j ].GetChannel() );
					}
				}
			}
			//else
			//{
			//	Com_Printf( "\"effects\" only available when ff_developer is set\n" );
			//}

			Processed.push_back( *itName );
			itName = Unprocessed.erase( itName );
		}
		else
		{
			itName++;
		}
	}

	mChannel.Display( Unprocessed, Processed );
}

#endif // FF_CONSOLECOMMAND

#endif // _IMMERSION


