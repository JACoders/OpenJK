#ifndef FF_SYSTEM_H
#define FF_SYSTEM_H

//#include "ff_utils.h"
//#include "ff_compound.h"
//#include "ff_ffset.h"
//#include "ff_configparser.h"

#include "ff_ConfigParser.h"
#include "ff_ChannelSet.h"
#include "ff_HandleTable.h"

//===[FFSystem]=======================================================/////////////
//
//	The main system for a single user with multiple channels for
//	multiple simultaneous devices. All this is factored and 'classy'
//	with the intent to make it more readable and easy to track bugs.
//
//	That's the intent, at least.
//
//====================================================================/////////////

class FFSystem
{
public:
	typedef FFConfigParser Config;
	typedef FFChannelSet Channel;
	typedef FFHandleTable Handle;
protected:
	Config mConfig;
	Channel mChannel;
	Handle mHandle;
	qboolean mInitialized;
	ffHandle_t ffShake;
public:
	qboolean Init( const char *channels );
	void Shutdown()
	{
		mInitialized = qfalse;
		mHandle.clear();
		mChannel.clear();
		mConfig.Clear();
	}
	ffHandle_t Register( const char *name, int channel, qboolean notfound = qtrue, qboolean create = qtrue )
	{
		ffHandle_t result = FF_HANDLE_NULL;
		if ( name && name[ 0 ] )
		{
			ChannelCompound compound( channel );
			mChannel.Register( compound, name, create );
			result = mHandle.Convert( compound, name, notfound );
		}
		return result;
	}
	qboolean StopAll()
	{
		return mChannel.StopAll();
	}
	const char* GetName( ffHandle_t ff )
	{
		return mHandle.GetName( ff );
	}
	qboolean Stop( ffHandle_t ff )
	{
		return mHandle[ ff ].Stop();
	}
	qboolean Play( ffHandle_t ff )
	{
		return mHandle[ ff ].Start();
	}
	qboolean EnsurePlaying( ffHandle_t ff )
	{
		return mHandle[ ff ].EnsurePlaying();
	}
	qboolean Shake( int intensity, int duration, qboolean ensure = qtrue )
	{
		ChannelCompound &Compound = mHandle[ ffShake ];
		Compound.ChangeDuration( duration );
		Compound.ChangeGain( intensity );
		return ensure 
		?	EnsurePlaying( ffShake ) 
		:	Compound.Start()
		;
	}
	qboolean IsInitialized() { return mInitialized;	}
	qboolean IsPlaying( ffHandle_t ff )
	{
		return mHandle[ ff ].IsPlaying();
	}
	qboolean ChangeGain( ffHandle_t ff, DWORD gain )
	{
		return mHandle[ ff ].ChangeGain( gain );
	}

	//
	//	Optional
	//
//	qboolean Lookup( set<ffHandle_t> &result, const char *name )
//	{
//		set<MultiEffect*> effect;
//		return
//			mChannel.Lookup( effect, name )
//		&&	mHandle.Lookup( result, effect, name );
//	}

#ifdef FF_ACCESSOR
//	Channel& GetChannels() { return mChannel; }				// for CMD_FF_Info
	Handle& GetHandles() { return mHandle; }				// for CMD_FF_Info
#endif

#ifdef FF_CONSOLECOMMAND
	void Display( TNameTable &Unprocessed, TNameTable &Processed );
	void GetDisplayTokens( TNameTable &Tokens );
#endif
};

#endif // FF_SYSTEM_H