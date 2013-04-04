#ifndef FF_CHANNELSET_H
#define FF_CHANNELSET_H

#include "ff_utils.h"
#include "ff_MultiSet.h"
#include "ff_ChannelCompound.h"

//===[FFChannelSet]===================================================/////////////
//
//	An extension to FFMultiSet that operates on a subset of its
//	elements specified by a channel. This channel may be inherent 
//	to a ChannelCompound passed as a parameter.
//
//====================================================================/////////////

class FFChannelSet : public FFMultiSet
{
public:
	typedef multimap<int, int> Channel;
protected:
	Channel mChannel;
	qboolean ParseChannels( const char *channels );
public:
	qboolean Init( FFConfigParser &config, const char *channels )
	{
		return qboolean
		(	FFMultiSet::Init( config )					// Initialize devices
		&&	ParseChannels( channels )					// Assign channels to devices
		);
	}
	void clear()
	{
		mChannel.clear();
		FFMultiSet::clear();
	}
	qboolean Register( ChannelCompound &compound, const char *name, qboolean create );

	//
	//	Optional
	//
#ifdef FF_ACCESSOR
//	Channel& GetAll() { return mChannel; }
#endif

#ifdef FF_CONSOLECOMMAND
	void Display( TNameTable &Unprocessed, TNameTable &Processed );
	static void GetDisplayTokens( TNameTable &Tokens );
#endif
};

class ChannelIterator : public multimapIterator<FFChannelSet::Channel>
{
public:
	ChannelIterator( FFChannelSet::Channel &map, int channel )
	:	multimapIterator<FFChannelSet::Channel>( map, channel )
	{}
};

#endif // FF_CHANNELSET_H