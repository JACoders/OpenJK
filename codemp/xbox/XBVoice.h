//-----------------------------------------------------------------------------
// File: XBVoice.h
//
// Stolen from June XDK SimpleVoice sample, then butchered
//
//-----------------------------------------------------------------------------
#ifndef _XBVOICE_H
#define _XBVOICE_H

#pragma warning( disable: 4786 )
#include <cassert>
#include <vector>
#include <algorithm>
#include <xtl.h>
#include <xonline.h>
#include <xhv.h>
#include "xbsocket.h"
#include "xbSockAddr.h"
#include "XHVVoiceManager.h"
#include "XBoxCommon.h"
#include "../win32/win_input.h"
#include "../game/q_shared.h"

// Which "channel" are we talking on - right now it's just team or everyone.
// And in a non-team game, it doesn't matter.
enum VoiceChannel
{
	CHAN_PRIMARY,
	CHAN_ALT
};


// Sending out one network packet for each 20ms packet worth of voice would
// waste a lot of bandwidth in packet overhead.  Instead, we'll coalesce many
// voice packets together, and send at regular intervals
const int	VOICE_PACKET_INTERVAL	= 100;		// 100ms = 10 times/s
const DWORD	MAX_VOICE_PER_PACKET	= 5;		// 5 packets per player = 100ms

//
// Message payloads - Packed to minimize network traffic
//
#pragma pack( push )
#pragma pack( 1 )

enum VOICEINFO
{
	VOICEINFO_NOVOICE,			// We have voice turned off, or have been banned
	VOICEINFO_SPEAKERS,			// We can receive voice, but can't send it
	VOICEINFO_HAVEVOICE,		// We have an active communicator (CAN_SEND & CAN_RECV)
	VOICEINFO_ADDREMOTEMUTE,
	VOICEINFO_REMOVEREMOTEMUTE,
};

// The VOICEINFO message is a little bit strange - since it
// must be sent reliably, it gets relayed by the host.	This
// means we need to identify the source and destination players
// inside the packet itself.  We use refIndex indices.
struct MsgVoiceInfo
{
	VOICEINFO	action;				// Action requested
	int			srcRefIndex;		// refIndex of sending source player
	int			dstRefIndex;		// refIndex of destination player
};

// This is dependent on the time per voice packet:
// CompressedSize = 2 + ( TimeInMs * 8 / 20 )
// 20ms -> 10 Bytes
// 40ms -> 18 Bytes
// 60ms -> 26 Bytes
// 80ms -> 34 Bytes
// 100ms -> 42 Bytes
#define COMPRESSED_VOICE_SIZE 10
struct VoicePacket
{
	BYTE byData[COMPRESSED_VOICE_SIZE];
};

struct MsgVoiceData
{
	WORD		wVoicePackets;
	VoicePacket VoicePackets[MAX_VOICE_PER_PACKET];
};
#pragma pack( pop )


//-----------------------------------------------------------------------------
// Name: class InfoMessage
// Desc: Informational voice message object sent between players and hosts
//-----------------------------------------------------------------------------
class InfoMessage
{
	MsgVoiceInfo	m_VoiceInfo;

public:

	explicit InfoMessage( ) { }
	~InfoMessage() {}

	int GetSize() const { return sizeof(*this); }
	MsgVoiceInfo&	 GetMsgVoiceInfo() { return m_VoiceInfo; }
};

//-----------------------------------------------------------------------------
// Name: class VoiceMessage
// Desc: Message object containing voice packets, sent between players
//-----------------------------------------------------------------------------
class VoiceMessage
{
	WORD			dataSize;		// Needed for winsock to realize there's no actual data
	MsgVoiceData	m_VoiceData;

public:
	explicit VoiceMessage( ) : dataSize( 0 ) { }
	~VoiceMessage() {}

	int	GetSize() const { return sizeof(*this); }
	MsgVoiceData&	 GetMsgVoiceData() { return m_VoiceData; }
};


//-----------------------------------------------------------------------------
// Name: class PendingMessage
// Desc: Used for parsing Messages out of a TCP stream.  Since fragments
//			of a given message could come in individually, they must be
//			buffered until complete
//-----------------------------------------------------------------------------
class PendingMessage
{
public:
	PendingMessage() : m_nBytesReceived( 0 ) { }

	HRESULT Read( SOCKET sock );   // Read message from socket
	VOID Reset() { m_nBytesReceived = 0; }

	InfoMessage	m_msg;				// Buffer space for message
	INT			m_nBytesReceived;	// # of bytes received in message
};


//-----------------------------------------------------------------------------
// Name: struct ClientSocket
// Desc: Keeps track of socket and associated data for reliable client
//			connections
//-----------------------------------------------------------------------------
struct ClientSocket
{
	SOCKET			sock;		// Socket
	SOCKADDR_IN		sa;			// Address of client
	PendingMessage	msgPending; // Pending message
	bool			inUse;		// Is this socket active?
};


//-----------------------------------------------------------------------------
// Name: class CVoiceManager
// Desc: Main class that provides all voice functionality
//-----------------------------------------------------------------------------
class CVoiceManager : public ITitleXHV
{
//	typedef std::vector< ClientSocket > SocketList;

	CXBSocket		m_DirectSock;			// Direct socket for voice data
	CXBSocket		m_ReliableSock;			// Reliable socket (or listen socket for host)
//	SocketList		m_ClientSockets;		// Server's reliable sockets
	ClientSocket	m_ClientSockets[MAX_CLIENTS];	// Server's reliable sockets

	PendingMessage	m_msgPending;			// Pending message for client
	VoiceMessage	m_msgVoiceData;			// Voice data packet for buffering voice

	int				m_VoiceTimer;			// Time of last voice send
	bool			m_bRunning;				// Are we in a game?
	bool			m_bInitialized;			// TRUE if we've initialized
	bool			m_bVoiceDisabled;		// Master cutoff switch

	CXHVVoiceManager m_XHVVoiceManager;		// Voice Chat engine

	int				m_VoiceMask;			// Which voice mask to use
	VoiceChannel	m_Channel;				// Who are we talking to?

	enum MuteState
	{
		MUTE_IDLE,
		MUTE_WORKING,
		MUTE_REFRESH
	};

	MuteState				m_MuteState;			// Simple state to refresh mute list after work
	XONLINE_MUTELISTUSER	m_MuteList[MAX_MUTELISTUSERS];	// Local copy of the user's mute list
	DWORD					m_MuteListSize;			// Number of (valid) entries in mute list
	XONLINETASK_HANDLE		m_MuteListTask;			// Task handle for mute list operations

	vec3_t			m_ClientPositions[MAX_CLIENTS];	// X,Y,Z of each client in super low-res (-128..128)

public:
	// ITitleXHV callback functions
	STDMETHODIMP CommunicatorStatusUpdate( DWORD dwPort, XHV_VOICE_COMMUNICATOR_STATUS status );
	STDMETHODIMP LocalChatDataReady( DWORD dwPort, DWORD dwSize, PVOID pData );

	// Highest level interface
	CVoiceManager();
	HRESULT	Initialize();
	void	Shutdown();
	HRESULT	Tick();

	// For entering and leaving the current game's chat
	void JoinSession( void );
	void LeaveSession( void );

	// Called by each client when a player joins or leaves the game
	HRESULT OnPlayerJoined( XBPlayerInfo *plyrInfo );
	HRESULT OnPlayerDisconnect( XBPlayerInfo *pPlayer );

	// Do we have a voice communicator at the moment?
	bool	CommunicatorPresent( void );
	// Do we have a communicator, and are we allowed to use it?
	bool	IsVoiceAllowed( void );

	// Are we listening to voice through our speakers? Simple pass-through
	BOOL	VoiceThroughSpeakers( void ) { return m_XHVVoiceManager.GetVoiceThroughSpeakers(); }

	// Pass through to XHV stuff:
	BOOL	IsTalking( XUID xuid ) { return m_XHVVoiceManager.IsTalking(xuid, IN_GetMainController()); }

	// Set muting state for the specified player, update our global mute list
	void	SetMute( XUID xuid, BOOL bMuted );

	// If the specified player muted?
	bool	IsMuted( XUID xuid );

	// Voice mask interface - get or set the current voice mask index
	int		GetVoiceMask( void );
	void	SetVoiceMask( int maskIndex );

	// Master switch interface
	bool	GetVoiceDisabled( void );

	// Controls both major options, with built-in error checking:
	void	SetVoiceOptions( int voiceMode );
	int		GetVoiceMode( void );

	// For setting our active channel
	void	SetChannel( VoiceChannel chan );

	// Re-calculate our list of voice targets:
	void	UpdateVoiceTargets( void );

	// Used to update our (staggered update) client positions for voice proximity:
	void	SetClientPosition( int index, short pos[3] )
	{
		m_ClientPositions[index][0] = pos[0];
		m_ClientPositions[index][1] = pos[1];
		m_ClientPositions[index][2] = pos[2];
	}

#ifndef FINAL_BUILD
	// For debugging purposes:
	void DrawVoiceStats( void );
#endif

private:
	// Send messages
	void SendVoiceInfo( VOICEINFO action, XBPlayerInfo *pDestPlayer );
	void SendVoiceDataToAll();
	int  SendVoiceMessage( const VoiceMessage* pMsg, const CXBSockAddr* psaDest = NULL );
	int  SendInfoMessage( const InfoMessage* pMsg, const CXBSockAddr* psaDest = NULL );

	// Receive messages
	void ProcessDirectMessage();
	void ProcessReliableMessage();

	// Process incoming messages
	void ProcessVoiceInfo( const MsgVoiceInfo&, const CXBSockAddr& );
	void ProcessVoiceData( const MsgVoiceData&, const CXBSockAddr& );

	// Manage the user's mute list
	void UpdateMuteList( void );
};


//-----------------------------------------------------------------------------
// Name: class MatchInAddr
// Desc: Predicate functor used to match on IN_ADDRs in player lists
//-----------------------------------------------------------------------------
struct MatchInAddr
{
	IN_ADDR ia;
	explicit MatchInAddr( const CXBSockAddr& sa ) : ia( sa.GetInAddr() ) { }
	explicit MatchInAddr( const in_addr& addr ) : ia( addr ) { }
	bool operator()( const XBPlayerInfo& playerInfo )
	{
		return playerInfo.inAddr.s_addr == ia.s_addr;
	}
	bool operator()( const ClientSocket& cs )
	{
		return cs.sa.sin_addr.s_addr == ia.s_addr;
	}
};

// The voice system:
extern CVoiceManager g_Voice;

#endif // _XBVOICE_H
