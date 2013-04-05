#ifndef FF_CHANNELCOMPOUND_H
#define FF_CHANNELCOMPOUND_H

#include "ff_MultiCompound.h"

////---------------
///	ChannelCompound
//-------------------
//	Stored in THandleTable. This class associates MultiCompound with some arbitrary 'channel.'
//	Further, this class assumes that its MultiEffects have the same name and are probably
//	initialized on different devices. None of this is enforced at this time.
//
class ChannelCompound : public MultiCompound
{
protected:
	int mChannel;
public:
	ChannelCompound( int channel = FF_CHANNEL_MAX ) 
	:	MultiCompound()
	{
		mChannel = 
		(	(channel >= 0 && channel < FF_CHANNEL_MAX)
		?	channel
		:	FF_CHANNEL_MAX
		);
	}

	ChannelCompound( Set &compound, int channel = FF_CHANNEL_MAX )
	:	MultiCompound( compound )
	{
		mChannel = 
		(	(channel >= 0 && channel < FF_CHANNEL_MAX)
		?	channel
		:	FF_CHANNEL_MAX
		);
	}

	int GetChannel() 
	{
		return mChannel;
	}
	const char *GetName()
	{
		return mSet.size() 
		?	(*mSet.begin())->GetName()
		:	NULL
		;
	}
	qboolean operator == ( ChannelCompound &channelcompound )
	{
		return qboolean
		(	mChannel == channelcompound.mChannel
		&&	(*(MultiCompound*)this) == *(MultiCompound*)&channelcompound
		);
	}
	qboolean operator != ( ChannelCompound &channelcompound )
	{
		return qboolean( !( (*this) == channelcompound ) );
	}
};

typedef vector<ChannelCompound> THandleTable;

#endif // FF_CHANNELCOMPOUND_H
