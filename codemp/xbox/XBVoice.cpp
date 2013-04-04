//-----------------------------------------------------------------------------
// File: XBVoice.cpp
//
// Stolen from June XDK SimpleVoice sample, then butchered
//
//-----------------------------------------------------------------------------
#include "XBVoice.h"
#include "XBLive.h"
#include "..\client\client.h"
#include "../win32/snd_fx_img.h"
#include "../qcommon/qcommon.h"
#include "../cgame/cg_local.h"

#include "../qcommon/xb_settings.h"

#define _UI	// I'm going to hell.
#include "..\ui\ui_shared.h"
#undef _UI

#ifndef FINAL_BUILD
#include "../renderer/tr_font.h"
#endif

// The voice system:
CVoiceManager g_Voice;

// Port 1000 gives 0 extra port overhead on the wire - this is used the game
// Ports 1001-1255 give 2 bytes overhead on the wire
const WORD	DIRECT_PORT		  = 1001;  // I wish VDP and UDP were really different
const WORD	RELIABLE_PORT	  = 1002;  // Port for low-bandwidth reliable msgs

// Define some preset voice mask configurations
struct VOICE_MASK_PRESET
{
	char *strLabel;
	XHV_VOICE_MASK mask;
};

const VOICE_MASK_PRESET g_VoiceMasks[] =
{
	{ "None",		XHV_VOICE_MASK_NONE },
	{ "Anonymous",	XHV_VOICE_MASK_ANONYMOUS },
};
const DWORD NUM_VOICEMASKS = sizeof( g_VoiceMasks ) / sizeof( g_VoiceMasks[0] );

// Maximum number of voice streams to use for playback
const DWORD NUM_XHV_PLAYBACK_STREAMS = 2;

// Small struct used in the code to determine who gets our voice
struct VoiceReceiver
{
	int				index;		// Client's index (into xbPlayerList)
	unsigned long	dist;		// Distance to this client in worldspace
};

// We keep track of how many people (and who) can hear us
static int numVoiceTargets = 0;
static VoiceReceiver voiceTargets[MAX_ONLINE_PLAYERS];
// Non-constant for now, so we can test it:
static int maxVoiceTargets = 5;

//-----------------------------------------------------------------------------
// Name: CVoiceManager()
// Desc: Constructor
//-----------------------------------------------------------------------------
CVoiceManager::CVoiceManager()
: 
	m_DirectSock			(),
	m_bInitialized			( false ),
	m_msgVoiceData			(),
	m_VoiceTimer			( 0 ),
	m_bRunning				( false ),
	m_MuteListSize			( 0 ),
	m_MuteListTask			( NULL ),
	m_MuteState				( MUTE_IDLE ),
	m_VoiceMask				( 0 ),
	m_Channel				( CHAN_PRIMARY ),
	m_bVoiceDisabled		( false )
{
	// Nothing
}


//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize device-dependant objects
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::Initialize()
{
	// Don't do anything if we're already initialized (servers can do this)
	if( m_bInitialized )
		return S_OK;

	extern LPDSEFFECTIMAGEDESC getEffectsImageDesc(void);

	// Set up parameters for the Voice Chat engine
	XHV_RUNTIME_PARAMS xhvParams;
	xhvParams.dwMaxLocalTalkers			= 4;	// XGetPortCount();
	xhvParams.dwMaxRemoteTalkers		= MAX_ONLINE_PLAYERS - 1;	// One per box
#ifdef _DEBUG
	xhvParams.dwMaxCompressedBuffers	= 10;					// 4 buffers per local talker
#else
	xhvParams.dwMaxCompressedBuffers	= 4;					// 4 buffers per local talker
#endif
	xhvParams.dwFlags					= 0;
	xhvParams.pEffectImageDesc			= getEffectsImageDesc();
	xhvParams.dwEffectsStartIndex		= GraphVoice_Voice_0;

	// Create the engine and use this object for the callbacks
	if( FAILED( m_XHVVoiceManager.Initialize( &xhvParams ) ) )
		return E_FAIL;
	m_XHVVoiceManager.SetCallbackInterface( this );
//	m_XHVVoiceManager.SetMaxPlaybackStreamsCount( NUM_XHV_PLAYBACK_STREAMS );
	m_bInitialized = true;

	// Get the mutelist add/remove task started - only do this if on Live
	if( !logged_on || XOnlineMutelistStartup( NULL, &m_MuteListTask ) != S_OK )
		m_MuteListTask = NULL;
	m_MuteState = MUTE_IDLE;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Shutdown the whole voice system
//-----------------------------------------------------------------------------
void CVoiceManager::Shutdown()
{
	// XBL_Cleanup calls this, just in case. But if we're cleanly
	// shutting down from the menu system, we've already killed voice.
	if( !m_bInitialized )
		return;

	// This takes us out of running - and closes down all sockets
	LeaveSession();

	if( FAILED( m_XHVVoiceManager.Shutdown() ) )
		Com_Printf( "ERROR: Couldn't shutdown XHV\n" );

	// Shutdown all mutelist functionality
	if( m_MuteListTask )
	{
		// Pump task to completion
		while( XOnlineTaskContinue( m_MuteListTask ) != XONLINETASK_S_RUNNING_IDLE )
			;

		XOnlineTaskClose( m_MuteListTask );
		m_MuteListTask = NULL;
		m_MuteState = MUTE_IDLE;
	}

	m_bInitialized = false;
}


//-----------------------------------------------------------------------------
// Name: GetVoiceMask
// Desc: Returns the index of the currently set voice mask
//-----------------------------------------------------------------------------
int CVoiceManager::GetVoiceMask( void )
{
	return m_VoiceMask;
}

//-----------------------------------------------------------------------------
// Name: SetVoiceMask
// Desc: Updates the voice mask to be used
//-----------------------------------------------------------------------------
void CVoiceManager::SetVoiceMask( int maskIndex )
{
	assert( maskIndex >= 0 && maskIndex < NUM_VOICEMASKS );

	m_VoiceMask = maskIndex;
	m_XHVVoiceManager.SetVoiceMask( IN_GetMainController(), &g_VoiceMasks[m_VoiceMask].mask );
}


//-----------------------------------------------------------------------------
// Name: Tick()
// Desc: Called once per frame does any voice work
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::Tick()
{
	// VVFIXME - Can this check against m_bRunning instead?
	if( !m_bInitialized )
		return S_OK;

	// Re-build our list of voice targets:
	UpdateVoiceTargets();

	// Pump the voice engine
	m_XHVVoiceManager.DoWork();

	// Handle net messages
	ProcessDirectMessage();
	ProcessReliableMessage();

	// Make sure we send voice data at an appropriate rate
	if( m_VoiceTimer < Sys_Milliseconds() )
	{
		SendVoiceDataToAll();
	}

	// Pump the mute list add/remove task
	if( m_MuteListTask )
	{
		HRESULT hr = XOnlineTaskContinue( m_MuteListTask );

		// If we're doing something, set state. If we were doing something, transition out
		if( hr != XONLINETASK_S_RUNNING_IDLE )
			m_MuteState = MUTE_WORKING;
		else if( m_MuteState == MUTE_WORKING )
			m_MuteState = MUTE_REFRESH;
	}

	// Get a fresh copy of the user's mute list if we need to
	if( m_MuteState == MUTE_REFRESH )
	{
		UpdateMuteList();
		m_MuteState = MUTE_IDLE;
	}

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: UpdateMuteList()
// Desc: Fetches the user's mute list from the Live servers, and copies it
//		 to the local stored version. We assume that we keep our own mute flags
//		 current, and thus don't bother to update them here. (It's a waste).
//-----------------------------------------------------------------------------
void CVoiceManager::UpdateMuteList( void )
{
	if( !logged_on )
		return;

	XONLINETASK_HANDLE handle;
	HRESULT hr = XOnlineMutelistGet( IN_GetMainController(), MAX_MUTELISTUSERS, NULL, &handle, &m_MuteList[0], &m_MuteListSize );
	if( hr != S_OK )
	{
		XOnlineTaskClose( handle );
		m_MuteListSize = 0;
		return;
	}

	do
	{
		hr = XOnlineTaskContinue( handle );
	}
	while( hr == XONLINETASK_S_RUNNING );
	XOnlineTaskClose( handle );
}


//-----------------------------------------------------------------------------
// Name: JoinSession()
// Desc: Opens the reliable socket to the host, this puts us in the chat system
//       Old code then sent a Join, and then waited for an accept. And THEN
//       called StartVoice() to get voice running. I do that all now - hope
//       no other problems surface.
//-----------------------------------------------------------------------------
void CVoiceManager::JoinSession( void )
{
	// If we were already in a session, then don't do anything.
	if( m_bRunning )
		return;

	// The direct socket is a non-blocking socket on port DIRECT_PORT.
	BOOL bSuccess = m_DirectSock.Open( CXBSocket::Type_VDP );
	if( !bSuccess )
	{
		Com_Error( ERR_FATAL, "Couldn't open VDP socket: %i\n", WSAGetLastError() );
	}

	CXBSockAddr directAddr( INADDR_ANY, DIRECT_PORT );
	INT iResult = m_DirectSock.Bind( directAddr.GetPtr() );
	assert( iResult != SOCKET_ERROR );
	DWORD dwNonBlocking = 1;
	iResult = m_DirectSock.IoCtlSocket( FIONBIO, &dwNonBlocking );
	assert( iResult != SOCKET_ERROR );

	// Create a reliable socket for low-bandwidth messages that need to be
	// sent reliably.
	bSuccess = m_ReliableSock.Open( CXBSocket::Type_TCP );
	assert( bSuccess );
	CXBSockAddr reliableAddr( INADDR_ANY, RELIABLE_PORT );
	iResult = m_ReliableSock.Bind( reliableAddr.GetPtr() );
	assert( iResult != SOCKET_ERROR );
	iResult = m_ReliableSock.IoCtlSocket( FIONBIO, &dwNonBlocking );
	assert( iResult != SOCKET_ERROR );

	// Above code was in InitSockets()

	// Grab some useful things:
	bool commPresent = CommunicatorPresent();
	bool speakers = VoiceThroughSpeakers();

	// Being banned forces voice to be disabled:
	if( logged_on && !XOnlineIsUserVoiceAllowed( XBLLoggedOnUsers[ IN_GetMainController() ].xuid.dwUserFlags ) )
		m_bVoiceDisabled = true;

	// Disabled voice implies (and overrides) no speakers:
	if( m_bVoiceDisabled )
	{
		m_XHVVoiceManager.SetVoiceThroughSpeakers( FALSE );
		speakers = false;
	}

	// If not disabled, we can always listen (through speakers)
	if( !m_bVoiceDisabled )
		xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_RECV;
	else
		xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags &= ~VOICE_CAN_RECV;

	// To talk, we need a communicator, and we need to not be banned,
	// and speakers needs to be off, and voice needs to be enabled:
	if( commPresent && !speakers && !m_bVoiceDisabled )
		xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_SEND;
	else
		xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags &= ~VOICE_CAN_SEND;

	// Open up a reliable socket to the host - this will be used
	// for low-bandwidth communication. We have to wait for the connection
	// to complete before doing anything else?
	if( com_sv_running->integer )
	{
		// Start listening for new clients
		m_ReliableSock.Listen();
	}
	else
	{
		// This now triggers everyone knowing that we're ready for voice
		CXBSockAddr saHost( xbc.SrvAddr, RELIABLE_PORT );
		m_ReliableSock.Connect( saHost.GetPtr() );

		// Wait for the socket to finish connecting
		BOOL socketWritable;
		while ( !m_ReliableSock.Select( NULL, &socketWritable, NULL ) )
			;

		// We send a voiceinfo message to tell people our voice configuration
		VOICEINFO vState;
		if( m_bVoiceDisabled )
			vState = VOICEINFO_NOVOICE;
		else if( commPresent )
			vState = VOICEINFO_HAVEVOICE;
		else
			vState = VOICEINFO_SPEAKERS;
		SendVoiceInfo( vState, NULL );
	}

	// Code that used to be in StartVoice()
	m_msgVoiceData.GetMsgVoiceData().wVoicePackets = 0;
	m_VoiceTimer = Sys_Milliseconds() + VOICE_PACKET_INTERVAL;

	// Put communicator into voice chat mode
	m_XHVVoiceManager.SetProcessingMode( IN_GetMainController(), XHV_VOICECHAT_MODE );

	// If we don't have a communicator, and we're not banned, send voice to speakers
	//m_XHVVoiceManager.SetVoiceThroughSpeakers( !commPresent && !voiceBanned );

	// Finally, register everyone that's already in the game and can send or receive voice:
	for( int i = 0; i < MAX_ONLINE_PLAYERS; ++i )
	{
		if( i == xbOnlineInfo.localIndex ||
			!xbOnlineInfo.xbPlayerList[i].isActive ||
			!(xbOnlineInfo.xbPlayerList[i].flags & VOICE_CAN_RECV) )
		{
			continue;
		}

		// We should never get here as a server
		assert( !com_sv_running->integer );
		OnPlayerJoined( &xbOnlineInfo.xbPlayerList[i] );
	}

	// Reset initial channel
	m_Channel = CHAN_PRIMARY;

	// Set our voice mask from user settings:
	SetVoiceMask( Settings.voiceMask );
	// And try to re-apply the stored voice mode:
	SetVoiceOptions( Settings.voiceMode );

	m_bRunning = true;
}


//-----------------------------------------------------------------------------
// Name: LeaveSession()
// Desc: Blows away any leftover sockets, etc... Called when leaving a session
//-----------------------------------------------------------------------------
void CVoiceManager::LeaveSession( void )
{
	// Ensure that we're actually in a session to leave
	if( !m_bRunning )
		return;

	// Clean up XHV and voice state
	m_XHVVoiceManager.ClearRemoteTalkers();
	m_XHVVoiceManager.SetProcessingMode( IN_GetMainController(), XHV_INACTIVE_MODE );

	// Close down the sockets
	m_DirectSock.Close();
	m_ReliableSock.Close();

	for( int i = 0; i < MAX_CLIENTS; ++i )
	{
		if( m_ClientSockets[i].inUse )
		{
			closesocket( m_ClientSockets[i].sock );
			m_ClientSockets[i].inUse = false;
		}
	}
/*
	for( SocketList::iterator it = m_ClientSockets.begin(); 
		 it < m_ClientSockets.end();
		 ++it )
	{
		closesocket( it->sock );
	}
	m_ClientSockets.clear();
*/

	m_bRunning = false;
}


//-----------------------------------------------------------------------------
// Name: SendVoiceInfo()
// Desc: Issue a MSG_VOICEINFO from ourself (either a host or player) to 
//			another player
//-----------------------------------------------------------------------------
void CVoiceManager::SendVoiceInfo( VOICEINFO		action,
								   XBPlayerInfo*	pDestPlayer )
{
	InfoMessage msgVoiceInfo;
	MsgVoiceInfo& msg = msgVoiceInfo.GetMsgVoiceInfo();
	msg.action = action;
	msg.srcRefIndex = xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].refIndex;

	// Send the message reliably - whether it's sent to all players
	// or just to one specific player is determined by what the
	// caller passed in for pDestPlayer
	INT nBytes;
	if( pDestPlayer )
	{
		// Send the voice info message reliably to the player
		// (note that it may be relayed by the host)
		msg.dstRefIndex = pDestPlayer->refIndex;
		CXBSockAddr sa( pDestPlayer->inAddr, DIRECT_PORT );
		nBytes = SendInfoMessage( &msgVoiceInfo, &sa );
	}
	else
	{
		// Send the voice info message reliably to all players in the game
		// (note that it will definitely be relayed by the host)
		msg.dstRefIndex = -1;
		nBytes = SendInfoMessage( &msgVoiceInfo );
	}

	// This assert was removed because Send no longer is guaranteed to always work
	// If the security association times out, the number of bytes returned will 
	// NOT be equal to the size of the message.  A good thing to do here would
	// be to drop the player

	//assert( nBytes == SOCKET_ERROR || nBytes == msgVoiceInfo.GetSize() );
	(void)nBytes;
}


//-----------------------------------------------------------------------------
// Name: SendVoiceDataToAll
// Desc: Sends accumulated voice data out to other players in the game
//-----------------------------------------------------------------------------
void CVoiceManager::SendVoiceDataToAll()
{
	// Make sure we actually have data to send...
	if( m_msgVoiceData.GetMsgVoiceData().wVoicePackets > 0 )
	{
		// Send voice data via VDP directly to all other players
		INT nBytes = SendVoiceMessage( &m_msgVoiceData );

		// This assert was removed because Send no longer is guaranteed to always work
		// If the security association times out, the number of bytes returned will 
		// NOT be equal to the size of the message.  A good thing to do here would
		// be to drop the player

		//assert( nBytes == SOCKET_ERROR || nBytes == m_msgVoiceData.GetSize() );
		(void)nBytes;
	}

	m_msgVoiceData.GetMsgVoiceData().wVoicePackets = 0;
	m_VoiceTimer = Sys_Milliseconds() + VOICE_PACKET_INTERVAL;
}


//-----------------------------------------------------------------------------
// Name: LocalChatDataReady
// Desc: XHV Callback - called when a packet of voice data is ready to be
//			sent over the wire
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::LocalChatDataReady( DWORD dwPort, DWORD dwSize, PVOID pData )
{
	// If we're not in a game right now, or voice is disabled - do nothing
	if( !m_bRunning || m_bVoiceDisabled )
		return S_OK;

	MsgVoiceData& msg = m_msgVoiceData.GetMsgVoiceData();

	memcpy( msg.VoicePackets[ msg.wVoicePackets ].byData, pData, dwSize );
	msg.wVoicePackets++;

	// We've set up our voice timer such that it SHOULD cause us to send out 
	// our buffered voice data before the buffer fills up.	However, things
	// like framerate glitches, etc., could cause us to fill up before we 
	// notice the timer has fired.
	if( msg.wVoicePackets == MAX_VOICE_PER_PACKET )
	{
		SendVoiceDataToAll();
	}

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: CommunicatorStatusUpdate
// Desc: XHV Callback - called when the engine detects that the status of a
//			communicator has changed.  May not be called if a communicator
//			is quickly removed and re-inserted, but in that case there is
//			nothing the game has to do.
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::CommunicatorStatusUpdate( DWORD dwPort, XHV_VOICE_COMMUNICATOR_STATUS status )
{
	// If we're not initialized, then do nothing
	if( !m_bInitialized )
		return S_OK;

	if( status == XHV_VOICE_COMMUNICATOR_STATUS_INSERTED )
	{ // Got a new headset:
		// Let the UI know that we have a headset, so it can enable things!
		Cvar_SetValue( "ui_headset", 1 );

		// Awful UI hack. If we're on the online options screen, move off any item
		// that just became disabled.
		menuDef_t *menu = Menu_GetFocused();
		if( menu && !Q_stricmp(menu->window.name, "xbl_onlineoptions") )
		{
			VM_Call( uivm, UI_KEY_EVENT, A_CURSOR_DOWN, qtrue );	// Send a "move the cursor down"

			// Also, if we had "Speakers" selected, switch it to "Enabled"
			if( Cvar_VariableIntegerValue( "ui_voiceMode" ) == 1 )
				Cvar_SetValue( "ui_voiceMode", 2 );
		}

		// Always re-route voice to headset
		m_XHVVoiceManager.SetVoiceThroughSpeakers( FALSE );

		// Don't do anything else if banned, or voice is disabled:
		if( m_bVoiceDisabled ||
			(logged_on && !XOnlineIsUserVoiceAllowed( XBLLoggedOnUsers[ IN_GetMainController() ].xuid.dwUserFlags ) ) )
			return S_OK;

		// If we're logged onto live, update our voice flag
		if( logged_on )
			XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_VOICE, true );

		// Finally, if we're in a session, update our status and tell everyone:
		if( m_bRunning )
		{
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_RECV;
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_SEND;
			SendVoiceInfo( VOICEINFO_HAVEVOICE, NULL );
		}
	}
	else if( status == XHV_VOICE_COMMUNICATOR_STATUS_REMOVED )
	{ // Lost a headset:
		// Let the UI know that we don't have a headset, so it can disable things!
		Cvar_SetValue( "ui_headset", 0 );

		// Awful UI hack. If we're on the online options screen, move off any item
		// that just became disabled.
		menuDef_t *menu = Menu_GetFocused();
		if( menu && !Q_stricmp(menu->window.name, "xbl_onlineoptions") )
		{
			VM_Call( uivm, UI_KEY_EVENT, A_CURSOR_UP, qtrue );		// Send a "move the cursor up"

			// Also, if we had "Enabled" selected, that's no longer valid. Change ui_voiceMode:
			if( Cvar_VariableIntegerValue( "ui_voiceMode" ) == 2 )
				Cvar_SetValue( "ui_voiceMode", 1 );
		}

		// If the user pulls the headset and it was set to "speakers" or "enabled",
		// then change to "speakers"
		m_XHVVoiceManager.SetVoiceThroughSpeakers( !m_bVoiceDisabled );

		// Don't do anything else if banned, or voice is disabled:
		if( m_bVoiceDisabled ||
			(logged_on && !XOnlineIsUserVoiceAllowed( XBLLoggedOnUsers[ IN_GetMainController() ].xuid.dwUserFlags ) ) )
			return S_OK;

		// If we're logged onto live, update our voice flag
		if( logged_on )
			XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_VOICE, false );

		// Finally, if we're in a session, update our status and tell everyone:
		if( m_bRunning )
		{
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_RECV;
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags &= ~VOICE_CAN_SEND;
			SendVoiceInfo( VOICEINFO_SPEAKERS, NULL );
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: SendInfoMessage
// Desc: Sends a reliable InfoMessage, either to everyone (NULL) or a specific
//		 recipient.
//-----------------------------------------------------------------------------
int CVoiceManager::SendInfoMessage( const InfoMessage* pMsg, const CXBSockAddr* psaDest )
{
	int nBytes = 0;

	// Reliable messages are sent via TCP connection, and so should
	// only be used for low-bandwidth, low-frequency messages.
	// Consider actively throttling the amount of data sent reliably
	if( com_sv_running->integer )
	{
		// We're the host
		if( psaDest )
		{
			// This is going to one player, we can send right to them
			MatchInAddr matchInAddr( psaDest->GetInAddr() );
//			SocketList::iterator it = std::find_if( m_ClientSockets.begin(), m_ClientSockets.end(), matchInAddr );
			int cIdx;
			for( cIdx = 0; cIdx < MAX_CLIENTS; ++cIdx )
			{
				if( m_ClientSockets[cIdx].inUse && matchInAddr( m_ClientSockets[cIdx] ) )
					break;
			}
//			assert( it != m_ClientSockets.end() );
			assert( cIdx != MAX_CLIENTS );

//			nBytes += send( it->sock, (char*)pMsg, pMsg->GetSize(), 0 );
			nBytes += send( m_ClientSockets[cIdx].sock, (char*)pMsg, pMsg->GetSize(), 0 );
		}
		else
		{
			// This is going to all players, so iterate over all clients and send
			for( int i = 0; i < MAX_CLIENTS; ++i )
			{
				if( m_ClientSockets[i].inUse )
					nBytes += send( m_ClientSockets[i].sock, (char*)pMsg, pMsg->GetSize(), 0 );
			}
/*
			for( SocketList::iterator it = m_ClientSockets.begin();
				it < m_ClientSockets.end();
				++it )
			{
				nBytes += send( it->sock, (char*)pMsg, pMsg->GetSize(), 0 );
			}
*/
		}
	}
	else
	{
		// We're a client - send to host and let him forward if needed
		if( m_ReliableSock.IsOpen() )
			nBytes += m_ReliableSock.Send( pMsg, pMsg->GetSize() );
	}

	return nBytes;
}

// Utility to verify that the given player is active, can receive voice, and
// that neither of us has the other muted:
bool _playerWantsVoice( const int index )
{
	return xbOnlineInfo.xbPlayerList[index].isActive &&
		   (xbOnlineInfo.xbPlayerList[index].flags & VOICE_CAN_RECV) &&
		   !(xbOnlineInfo.xbPlayerList[index].flags & MUTED_PLAYER) &&
		   !(xbOnlineInfo.xbPlayerList[index].flags & REMOTE_MUTED);
}

// Utility to add a player index/distance pair to the voice targets list:
void _addVoiceTarget( const int index, unsigned long dist )
{
	voiceTargets[numVoiceTargets].index = index;
	voiceTargets[numVoiceTargets].dist = dist;
	++numVoiceTargets;
}

//-----------------------------------------------------------------------------
// Name: UpdateVoiceTargets
// Desc: Re-calculates the set of people that we should send voice to
//-----------------------------------------------------------------------------
void CVoiceManager::UpdateVoiceTargets( void )
{
	// Re-set the count:
	numVoiceTargets = 0;

	// ONE: Dedicated server sends to everyone or no-one, depending on channel:
	if( xbOnlineInfo.localIndex == DEDICATED_SERVER_INDEX )
	{
		if( m_Channel == CHAN_ALT )
		{
			// Button is held, add all clients to our recipient list:
			for( int i = 0; i < DEDICATED_SERVER_INDEX; ++i )
			{
				if( _playerWantsVoice( i ) )
					_addVoiceTarget( i, 0 );
			}
		}
	}
	else
	{
		// We're not the dedicated server. Now we get to all the game-type rules and such:

		// TWO: Everyone sends to the dedicated server, if there is one:
		if( _playerWantsVoice( DEDICATED_SERVER_INDEX ) )
			_addVoiceTarget( DEDICATED_SERVER_INDEX, 0 );

		// Now check all the other clients:
		for( int i = 0; i < DEDICATED_SERVER_INDEX; ++i )
		{
			// Skip any non-active players, not listening, muted either way, and ourselves
			if( !_playerWantsVoice( i ) || (i == xbOnlineInfo.localIndex) )
				continue;

			// THREE: Spectators are ALWAYS segregated from all other players:
			if( cgs.clientinfo[xbOnlineInfo.localIndex].team == TEAM_SPECTATOR ||
				cgs.clientinfo[i].team == TEAM_SPECTATOR )
			{
				if( cgs.clientinfo[i].team == cgs.clientinfo[xbOnlineInfo.localIndex].team )
					_addVoiceTarget( i, 0 );
				continue;
			}

			// FOUR: In team games, you can talk to teammates, or everyone:
			if( cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF || cgs.gametype == GT_SIEGE )
			{
				// On primary channel, only talk to teammates, on alt channel talk to all:
				if( (m_Channel == CHAN_ALT) ||
					(cgs.clientinfo[i].team == cgs.clientinfo[xbOnlineInfo.localIndex].team) )
				{
					vec3_t vecToPlayer;
					VectorSubtract( m_ClientPositions[i],
									m_ClientPositions[xbOnlineInfo.localIndex],
									vecToPlayer );
					_addVoiceTarget( i, VectorLengthSquared( vecToPlayer ) );
				}
				continue;
			}

			// FIVE: In duel, the two duelists can talk to each other:
			if( cgs.gametype == GT_DUEL )
			{
				if( cgs.clientinfo[i].team != TEAM_SPECTATOR )
					_addVoiceTarget( i, 0 );
				continue;
			}

			// SIX: In power duel...
			if( cgs.gametype == GT_POWERDUEL )
			{
				// Players can never talk to spectators:
				if( cgs.clientinfo[i].team == TEAM_SPECTATOR )
					continue;

				// Singles can talk to enemies, Double can talk to each other, plus enemy with button:
				if( cgs.clientinfo[xbOnlineInfo.localIndex].duelTeam == DUELTEAM_LONE )
					_addVoiceTarget( i, 0 );
				else if( cgs.clientinfo[i].duelTeam == DUELTEAM_DOUBLE || m_Channel == CHAN_ALT )
					_addVoiceTarget( i, 0 );

				continue;
			}

			// SEVEN: FFA is all that's left, we add everyone:
			vec3_t vecToPlayer;
			VectorSubtract( m_ClientPositions[i],
							m_ClientPositions[xbOnlineInfo.localIndex],
							vecToPlayer );
			_addVoiceTarget( i, VectorLengthSquared( vecToPlayer ) );
		}

		// Remove any, if we need to. Dedicated servers and spectators skip this step:
		if( cgs.clientinfo[xbOnlineInfo.localIndex].team != TEAM_SPECTATOR )
		{
			while( numVoiceTargets > maxVoiceTargets )
			{
				int largest = 0;
				for( int j = 1; j < numVoiceTargets; ++j )
					if( voiceTargets[j].dist > voiceTargets[largest].dist )
						largest = j;
				--numVoiceTargets;
				voiceTargets[largest].index = voiceTargets[numVoiceTargets].index;
				voiceTargets[largest].dist = voiceTargets[numVoiceTargets].dist;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: SendVoiceMessage
// Desc: Sends a voice message over the network, to everyone (NULL) or a
//		 specific recipient.
//-----------------------------------------------------------------------------
int CVoiceManager::SendVoiceMessage( const VoiceMessage* pMsg, const CXBSockAddr* psaDest )
{
	int nBytes = 0;

	// Non-reliable message - these get sent directly via VDP in all cases
	if( psaDest )
	{
		// If destined for a specific player, send straight to them
		nBytes += m_DirectSock.SendTo( pMsg, pMsg->GetSize(), psaDest->GetPtr() );
	}
	else
	{
		// Send to our current list of voice targets:
		for( int i = 0; i < numVoiceTargets; ++i )
		{
			CXBSockAddr sa( xbOnlineInfo.xbPlayerList[voiceTargets[i].index].inAddr, DIRECT_PORT );
			nBytes += m_DirectSock.SendTo( pMsg, pMsg->GetSize(), sa.GetPtr() );
		}
	}

	return nBytes;
}


//-----------------------------------------------------------------------------
// Name: ProcessDirectMessage()
// Desc: Checks to see if any direct messages are waiting on the direct socket.
//		 These can ONLY be VoiceMessages.
//		 If a message is waiting, it is routed and processed.
//		 If no messages are waiting, the function returns immediately.
//-----------------------------------------------------------------------------
void CVoiceManager::ProcessDirectMessage()
{
	if( !m_DirectSock.IsOpen() )
		return;

	// See if a network message is waiting for us
	VoiceMessage msg;
	SOCKADDR_IN saFromIn;
	INT iResult;

	// Process until no more messages are available
	do
	{
		iResult = m_DirectSock.RecvFrom( &msg, msg.GetSize(), &saFromIn );
		CXBSockAddr saFrom( saFromIn );

		// If message waiting, process it
		if( iResult != SOCKET_ERROR && iResult > 0 )
		{
			assert( iResult == msg.GetSize() );
			ProcessVoiceData( msg.GetMsgVoiceData(), saFrom );
		}
		else
		{
			assert( WSAGetLastError() == WSAEWOULDBLOCK );
		}
	} while( iResult != SOCKET_ERROR && iResult > 0 );

	return;
}


//-----------------------------------------------------------------------------
// Name: Read
// Desc: Attempts to read a message from the specified socket. Our reliable
//		 sockets are stream-oriented, so the message may come in small pieces.
//		 Fortunately, all reliable messages are the same size, so the logic is
//		 pretty simple.
//		 Returns TRUE is completely parsed.
//-----------------------------------------------------------------------------
HRESULT PendingMessage::Read( SOCKET sock )
{
	// If we have a partial message, just ask for enough data to complete it
	if( m_nBytesReceived < m_msg.GetSize() )
	{
		CHAR* pbReceive = ( (CHAR *)&m_msg ) + m_nBytesReceived;
		INT   nBytesReq = m_msg.GetSize() - m_nBytesReceived;
		INT   nBytes	= recv( sock, pbReceive, nBytesReq, 0 );

		// Check result
		if( nBytes == SOCKET_ERROR )
		{
			if( WSAGetLastError() != WSAEWOULDBLOCK )
				return E_FAIL;
		}
		else
		{
			m_nBytesReceived += nBytes;
		}
	}

	// Determine if we now have a complete message
	if( m_nBytesReceived == m_msg.GetSize() )
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}


//-----------------------------------------------------------------------------
// Name: ProcessReliableMessage()
// Desc: First checks to see if any new connections have been attempted, and if
//		 we have room, accepts them.  Then, scans all reliable client 
//		 connections to see if any have messages pending.
//		 If a message is waiting, it is routed and processed.
//		 If no messages are waiting, the function returns immediately.
//-----------------------------------------------------------------------------
void CVoiceManager::ProcessReliableMessage()
{
	if( !m_ReliableSock.IsOpen() )
		return;

	if( com_sv_running->integer )
	{
		// Process any pending socket connections
		for( ; ; )
		{
			ClientSocket cs;
			cs.sock = m_ReliableSock.Accept( &cs.sa );
			if( cs.sock == INVALID_SOCKET )
				break;

			int cIdx;
			for( cIdx = 0; cIdx < MAX_CLIENTS; ++cIdx )
			{
				if( !m_ClientSockets[cIdx].inUse )
					break;
			}
			assert( cIdx != MAX_CLIENTS );
			m_ClientSockets[cIdx] = cs;
			m_ClientSockets[cIdx].inUse = true;
//			m_ClientSockets.push_back( cs );
		}

		// Poll each of our clients for messages and timeout
//		for( SocketList::iterator it = m_ClientSockets.begin();
//			it < m_ClientSockets.end();
//			++it )
		for( int i = 0; i < MAX_CLIENTS; ++i )
		{
			if( !m_ClientSockets[i].inUse )
				continue;
			ClientSocket *it = &m_ClientSockets[i];

			// Try to parse out a message from the socket.	If message was
			// completed, process the message
			HRESULT hr = it->msgPending.Read( it->sock );
			if( FAILED( hr ) )
			{
				// We lost a connection to a client.
				Com_Printf( "WARNING: Reliable voice cnxn was lost\n" );

				// VVFIXME - This code assumes that the xbPlayerInfo for the drop is
				// still valid. It might not be, in some really outrageous situation.
				// We should figure out a solution to that.

				// Remove them from the Voice System, but let the game logic handle
				// everything else (xbOnlineInfo).
				int idx;
				for( idx = 0; idx < MAX_ONLINE_PLAYERS; ++idx )
				{
					if( xbOnlineInfo.xbPlayerList[idx].isActive &&
						xbOnlineInfo.xbPlayerList[idx].inAddr.s_addr == it->sa.sin_addr.s_addr )
						break;
				}

				if( idx == MAX_ONLINE_PLAYERS )
				{
					// This shouldn't happen (but it can - see above)
					Com_Error( ERR_FATAL, "ERROR: Couldn't find user after reliable cnxn lost\n" );
				}

				OnPlayerDisconnect( &xbOnlineInfo.xbPlayerList[idx] );
				// OK. We found them in the playerlist. Take them out of the voice system
				// This function removes the socket from the vector. I don't care if SimpleVoice
				// isn't iterator safe - I will be. =) VVFIXME - I still don't like this.
/*
				int oldIndex = it - m_ClientSockets.begin();
				OnPlayerDisconnect( &xbOnlineInfo.xbPlayerList[idx] );
				it = m_ClientSockets.begin() + oldIndex;
*/
			}
			else if( S_OK == hr )
			{
				ProcessVoiceInfo( it->msgPending.m_msg.GetMsgVoiceInfo(), CXBSockAddr( it->sa ) );
				it->msgPending.Reset();
			}
		}
	}
	else
	{
		HRESULT hr = m_msgPending.Read( m_ReliableSock.GetSocket() );
		if( FAILED( hr ) )
		{
			// This leads to calling g_Voice.Shutdown(), let that handle cleanup
			Com_Error( ERR_DROP, "@MENUS_LOST_CONNECTION" );
		}
		else if( S_OK == hr )
		{
			ProcessVoiceInfo( m_msgPending.m_msg.GetMsgVoiceInfo(), CXBSockAddr( xbc.SrvAddr, RELIABLE_PORT ) );
			m_msgPending.Reset();
		}
	}

	return;
}


//-----------------------------------------------------------------------------
// Name: ProcessVoiceInfo()
// Desc: Process the voiceport message
//-----------------------------------------------------------------------------
void CVoiceManager::ProcessVoiceInfo( const MsgVoiceInfo& msg, const CXBSockAddr& saFrom )
{
	// We can't just look at the INADDR of the sender, since this message
	// may have been relayed by the host
	int idx;
	for ( idx = 0; idx < MAX_ONLINE_PLAYERS; ++idx )
	{
		if( xbOnlineInfo.xbPlayerList[idx].isActive &&
			xbOnlineInfo.xbPlayerList[idx].refIndex == msg.srcRefIndex )
			break;
	}

	// If we get a message from an invalid player, ignore it
	if( idx == MAX_ONLINE_PLAYERS )
	{
		Com_Printf( "ERROR: VoiceInfo from invalid player!\n" );
		return;
	}

	XBPlayerInfo *srcPlyr = &xbOnlineInfo.xbPlayerList[idx];

	// This message may or may not be intended for us.	If there's no
	// destination player specified, it's meant for everyone.  Otherwise,
	// we should only process it if it's got our name on it
	if( msg.dstRefIndex == -1 ||
		msg.dstRefIndex == xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].refIndex )
	{
		switch( msg.action )
		{
		case VOICEINFO_NOVOICE:
			// This player wants no voice, or has been banned
			srcPlyr->flags &= ~VOICE_CAN_SEND;
			srcPlyr->flags &= ~VOICE_CAN_RECV;
			break;

		case VOICEINFO_SPEAKERS:
			// This player will only be receiving voice, not sending it
			srcPlyr->flags &= ~VOICE_CAN_SEND;
			srcPlyr->flags |= VOICE_CAN_RECV;

			// We still register them with XHV, to check against our mute list
			OnPlayerJoined( srcPlyr );
			break;

		case VOICEINFO_HAVEVOICE:
			// This player will be sending and receiving voice
			srcPlyr->flags |= VOICE_CAN_SEND;
			srcPlyr->flags |= VOICE_CAN_RECV;

			// Register them with XHV, and check mute lists
			OnPlayerJoined( srcPlyr );
			break;

		case VOICEINFO_ADDREMOTEMUTE:
			// We've been muted by someone
			srcPlyr->flags |= REMOTE_MUTED;
			m_XHVVoiceManager.SetRemoteMute( srcPlyr->xuid, IN_GetMainController(), TRUE );
			break;

		case VOICEINFO_REMOVEREMOTEMUTE:
			// We've been un-muted by someone
			srcPlyr->flags &= ~(REMOTE_MUTED);
			m_XHVVoiceManager.SetRemoteMute( srcPlyr->xuid, IN_GetMainController(), FALSE );
			break;

		default:
			assert( FALSE );
			break;
		}
	}

	// If the host wasn't the recipient of this message, or the message
	// is intended for all players, it's the host's responsibility to
	// relay it to all clients
	if( com_sv_running->integer )
	{
		InfoMessage msgVoiceInfo;
		msgVoiceInfo.GetMsgVoiceInfo() = msg;

		if( msg.dstRefIndex == -1 )
		{
			// Intended for everyone - send to all BUT the source
//			for( SocketList::iterator it = m_ClientSockets.begin();
//				 it < m_ClientSockets.end();
//				 ++it )
			for( int i = 0; i < MAX_CLIENTS; ++i )
			{
				if( !m_ClientSockets[i].inUse )
					continue;
				ClientSocket *it = &m_ClientSockets[i];

				if( it->sa.sin_addr.s_addr != saFrom.GetInAddr().s_addr )
				{
					CXBSockAddr saDest( it->sa );
					SendInfoMessage( &msgVoiceInfo, &saDest );
				}
			}
		}
		else if( msg.dstRefIndex != xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].refIndex )
		{
			// Intended for a specific player (not us) - Find that player and send to them
			for( int i = 0; i < MAX_ONLINE_PLAYERS; ++i )
			{
				if( xbOnlineInfo.xbPlayerList[i].isActive &&
					xbOnlineInfo.xbPlayerList[i].refIndex == msg.dstRefIndex )
				{
					CXBSockAddr saDest( xbOnlineInfo.xbPlayerList[i].inAddr, RELIABLE_PORT );
					SendInfoMessage( &msgVoiceInfo, &saDest );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Name: ProcessVoiceData
// Desc: Handles receipt of a voice data packet
//-----------------------------------------------------------------------------
void CVoiceManager::ProcessVoiceData( const MsgVoiceData& msg, const CXBSockAddr& saFrom )
{
	for( WORD i = 0; i < msg.wVoicePackets; i++ )
	{
		const VoicePacket* pPacket = &msg.VoicePackets[i];

		// We need to find the sender in the player list, so we can get their XUID
		int j;
		for( j = 0; j < MAX_ONLINE_PLAYERS; ++j )
		{
			if( xbOnlineInfo.xbPlayerList[j].isActive &&
				xbOnlineInfo.xbPlayerList[j].inAddr.s_addr == saFrom.GetInAddr().s_addr )
				break;
		}

		if( j == MAX_ONLINE_PLAYERS)
		{
			Com_Printf( "ERROR: Voice data from invalid IN_ADDR!\n" );
			return;
		}

		if( xbOnlineInfo.xbPlayerList[j].flags & MUTED_PLAYER ||
			xbOnlineInfo.xbPlayerList[j].flags & REMOTE_MUTED )
		{
			// Don't let late voice play after a mute was set
			return;
		}

		m_XHVVoiceManager.SubmitIncomingVoicePacket( xbOnlineInfo.xbPlayerList[j].xuid, (VOID*)pPacket->byData, COMPRESSED_VOICE_SIZE );
	}
}


//-----------------------------------------------------------------------------
// Name: OnPlayerJoined 
// Desc: Called whenever someone is going to start sending or receiving chat.
//		 Originally intended for first-join only, but now it re-checks mute
//		 lists and such as well.
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::OnPlayerJoined( XBPlayerInfo *plyrInfo )
{
	// Register the new player with XHV
	m_XHVVoiceManager.RegisterRemoteTalker( plyrInfo->xuid );

	// VVFIXME - fancier priority scheme needed!
	m_XHVVoiceManager.SetRemoteTalkerPriority( plyrInfo->xuid, IN_GetMainController(), XHV_PLAYBACK_PRIORITY_MAX );

	// Scan for the player in our mute list
	for( int i = 0; i < m_MuteListSize; ++i )
	{
		if( XOnlineAreUsersIdentical( &m_MuteList[i].xuid, &plyrInfo->xuid ) )
		{
			// OK. We don't like this person. Mark them muted, and tell them.
			plyrInfo->flags |= MUTED_PLAYER;
			m_XHVVoiceManager.SetMute( plyrInfo->xuid, IN_GetMainController(), TRUE );
			SendVoiceInfo( VOICEINFO_ADDREMOTEMUTE, plyrInfo );
			break;
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: OnPlayerDisconnect
// Desc: Called whenever we've detected a player disconnect. Should be called
//       before the XBPlayerInfo is blown away. This is NOT symmetric with
//		 OnPlayerJoined, which can be called many times during a game.
//-----------------------------------------------------------------------------
HRESULT CVoiceManager::OnPlayerDisconnect( XBPlayerInfo *pPlayer )
{
	// The host needs to close down the reliable channel
	if( com_sv_running->integer )
	{
		// Find the matching entry in list of reliable sockets
		MatchInAddr matchInAddr( pPlayer->inAddr );
//		SocketList::iterator it = std::find_if( m_ClientSockets.begin(), m_ClientSockets.end(), matchInAddr );
		int cIdx;
		for( cIdx = 0; cIdx < MAX_CLIENTS; ++cIdx )
		{
			if( m_ClientSockets[cIdx].inUse && matchInAddr( m_ClientSockets[cIdx] ) )
				break;
		}

		// Not finding the socket was an error before (in SimpleVoice). But if the client
		// just disconnects, we'll call this as soon as we notice the socket is hosed, and
		// again when we get the removepeer message after the net core times out the client.
//		if( it != m_ClientSockets.end() )
		if( cIdx != MAX_CLIENTS )
		{
			ClientSocket *it = &m_ClientSockets[cIdx];

			// Close the socket
			closesocket( it->sock );
//			m_ClientSockets.erase( it );
			it->inUse = false;
		}
	}

	// Notify XHV that the player is gone
	m_XHVVoiceManager.UnregisterRemoteTalker( pPlayer->xuid );

	// Don't try and send them any more voice packets
	pPlayer->flags &= ~VOICE_CAN_RECV;
	pPlayer->flags &= ~VOICE_CAN_SEND;

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: CommunicatorPresent
// Desc: Tells whether or not we have a communicator plugged in
//-----------------------------------------------------------------------------
bool CVoiceManager::CommunicatorPresent( void )
{
	XHV_LOCAL_TALKER_STATUS status;
	m_XHVVoiceManager.GetLocalTalkerStatus( IN_GetMainController(), &status );
	return status.communicatorStatus == XHV_VOICE_COMMUNICATOR_STATUS_INSERTED;
}

//-----------------------------------------------------------------------------
// Name: IsVoiceAllowed
// Desc: Do we have a communicator, and if so, should we use it?
//-----------------------------------------------------------------------------
bool CVoiceManager::IsVoiceAllowed( void )
{
	// Without a headset, or with voice disabled, we can't talk.
	if( !CommunicatorPresent() || m_bVoiceDisabled )
		return false;

	// OK. We have a headset, and we're on syslink, so it's good.
	if( !logged_on )
		return true;

	return XOnlineIsUserVoiceAllowed( XBLLoggedOnUsers[ IN_GetMainController() ].xuid.dwUserFlags );
}

//-----------------------------------------------------------------------------
// Name: SetMute
// Desc: Sets the mute state for the given user to the specified state.
//		 Updates XHV, and also our user's global mute list (if logged on).
//-----------------------------------------------------------------------------
void CVoiceManager::SetMute( XUID xuid, BOOL bMuted )
{
	// First, set the mute state in XHV
	m_XHVVoiceManager.SetMute( xuid, IN_GetMainController(), bMuted );

	// If we're logged on, we need to update our global list
	if( logged_on )
	{
		// Make the change to our mutelist
		if( bMuted )
			XOnlineMutelistAdd( IN_GetMainController(), xuid );
		else
			XOnlineMutelistRemove( IN_GetMainController(), xuid );

		// Signal that we've made a change. This forces Tick() to refresh
		// our list, just in case.
		m_MuteState = MUTE_WORKING;
	}

	// If the user is in our game currently, send them a message:
	int idx;
	for( idx = 0; idx < MAX_ONLINE_PLAYERS; ++idx )
	{
		if( xbOnlineInfo.xbPlayerList[idx].isActive &&
			XOnlineAreUsersIdentical( &xuid, &xbOnlineInfo.xbPlayerList[idx].xuid ) )
			break;
	}

	// If they're not in the game, nothing else to do
	if( idx == MAX_ONLINE_PLAYERS )
		return;

	// Update our flags, and let the other person know:
	XBPlayerInfo *plyrInfo = &xbOnlineInfo.xbPlayerList[idx];
	if( bMuted )
	{
		plyrInfo->flags |= MUTED_PLAYER;
		SendVoiceInfo( VOICEINFO_ADDREMOTEMUTE, plyrInfo );
	}
	else
	{
		plyrInfo->flags &= ~(MUTED_PLAYER);
		SendVoiceInfo( VOICEINFO_REMOVEREMOTEMUTE, plyrInfo );
	}

	return;
}

//-----------------------------------------------------------------------------
// Name: IsMuted
// Desc: Returns whether or not the specificed player is muted by the current
//		 player. This works for anyone, not just people in the current game, so
//		 the friends UI can use it.
//-----------------------------------------------------------------------------
bool CVoiceManager::IsMuted( XUID xuid )
{
	for( int i = 0; i < m_MuteListSize; ++i )
	{
		if( XOnlineAreUsersIdentical( &xuid, &m_MuteList[i].xuid ) )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Name: SetChannel
// Desc: Controls how our voice is sent out. There are only two channels:
//       CHAN_PRIMARY and CHAN_ALT, and their meaning depends on the game
//       mode, and lots of other things.
//-----------------------------------------------------------------------------
void CVoiceManager::SetChannel( VoiceChannel chan )
{
	m_Channel = chan;
}


//-----------------------------------------------------------------------------
// Name: GetVoiceDisabled
// Desc: Returns the state of the master cutoff
//-----------------------------------------------------------------------------
bool CVoiceManager::GetVoiceDisabled( void )
{
	return m_bVoiceDisabled;
}



//-----------------------------------------------------------------------------
// Name: SetVoiceOptions
// Desc: Used by the UI to find out the current voice mode:
//  0 (disabled), 1 (speakers), 2 (enabled)
//-----------------------------------------------------------------------------
int CVoiceManager::GetVoiceMode( void )
{
	if( m_bVoiceDisabled )
		return 0;
	if( m_XHVVoiceManager.GetVoiceThroughSpeakers() || !CommunicatorPresent() )
		return 1;
	return 2;
}


//-----------------------------------------------------------------------------
// Name: SetVoiceOptions
// Desc: Used by the UI to adjust voice through speakers and voice disable
// voiceMode: 0 (disabled), 1 (speakers), 2 (enabled)
//-----------------------------------------------------------------------------
void CVoiceManager::SetVoiceOptions( int voiceMode )
{
	// Do nothing if we are banned - this might not be right
	if( logged_on && !XOnlineIsUserVoiceAllowed( XBLLoggedOnUsers[ IN_GetMainController() ].xuid.dwUserFlags ) )
		return;

	// This should never happen, but...
	bool commPresent = CommunicatorPresent();
	if( !commPresent && (voiceMode == 2) )
		voiceMode = 1;

	// Store new settings:
	m_bVoiceDisabled = (voiceMode == 0);
	m_XHVVoiceManager.SetVoiceThroughSpeakers( (voiceMode == 1) );

	// Update our friend notification: we have voice if not disabled, and we have a comm:
	if( logged_on )
		XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_VOICE, (commPresent && !m_bVoiceDisabled) );

	// Update our own player information, and inform other players if we're in a session
	if( m_bRunning )
	{
		// We can receive voice as long as it's not disabled:
		if( !m_bVoiceDisabled )
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_RECV;
		else
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags &= ~VOICE_CAN_RECV;

		// We can send voice only if it's really enabled (implies that we have a comm):
		if( voiceMode == 2 )
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags |= VOICE_CAN_SEND;
		else
			xbOnlineInfo.xbPlayerList[xbOnlineInfo.localIndex].flags &= ~VOICE_CAN_SEND;

		// Send our new state to everyone, if we have a communicator, we always
		// send HAVEVOICE, for TCR reasons:
		if( commPresent && !m_bVoiceDisabled )
			SendVoiceInfo( VOICEINFO_HAVEVOICE, NULL );
		else
			SendVoiceInfo( (VOICEINFO)voiceMode, NULL );
	}
}

#ifndef FINAL_BUILD
void CVoiceManager::DrawVoiceStats( void )
{
	const float scale = 0.8f;

	int lineHeight = RE_Font_HeightPixels( 1, scale );

	// Draw the list of other players, color coded
	int yPos = 0;
	for( int i = 0; i < MAX_ONLINE_PLAYERS; ++i )
	{
		if( !xbOnlineInfo.xbPlayerList[i].isActive )
			continue;

		bool bTarget = false;
		for( int j = 0; j < numVoiceTargets; ++j )
			if( voiceTargets[j].index == i )
			{
				bTarget = true;
				break;
			}

		RE_Font_DrawString( 320, 100 + (yPos * lineHeight), xbOnlineInfo.xbPlayerList[i].name,
							bTarget ? colorGreen : colorWhite, 1, -1, scale );
		yPos++;
	}
}
#endif
