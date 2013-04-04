//-----------------------------------------------------------------------------
// File: XHVVoiceManager.h
//
// Desc: Wraps the XHV voice engine and provides a simple interface to the 
//          game
//
// Hist: 05.06.03 - New for the June 2003 XDK
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _XHVVOICEMANAGER_H_
#define _XHVVOICEMANAGER_H_

#include <xtl.h>
#include <xonline.h>
#include <xhv.h>


//-----------------------------------------------------------------------------
// Name: struct PlayerVoiceInfo
// Desc: Used to keep track of voice-related details about each player
//-----------------------------------------------------------------------------
struct PlayerVoiceInfo
{
    XUID                    xuid;
    XHV_PLAYBACK_PRIORITY   priority[4];
    BOOL                    bMuted[4];
    BOOL                    bRemoteMuted[4];
};

class CXHVVoiceManager
{
public:
    CXHVVoiceManager();
    ~CXHVVoiceManager();

    HRESULT Initialize( XHV_RUNTIME_PARAMS* pXHVParams );
    HRESULT Shutdown();

    // These functions are simple passthrough functions to m_pXHVEngine:
    HRESULT SetCallbackInterface( ITitleXHV* pTitleXHV )
        { return m_pXHVEngine->SetCallbackInterface( pTitleXHV ); }
    HRESULT DoWork()
        { return m_pXHVEngine->DoWork(); }
    HRESULT SetVoiceMask( DWORD dwPort, const XHV_VOICE_MASK* pVoiceMask )
        { return m_pXHVEngine->SetVoiceMask( dwPort, pVoiceMask ); }
    HRESULT GetLocalTalkerStatus( DWORD dwPort, XHV_LOCAL_TALKER_STATUS* pLocalTalkerStatus )
        { return m_pXHVEngine->GetLocalTalkerStatus( dwPort, pLocalTalkerStatus ); }
    HRESULT SetMaxPlaybackStreamsCount( DWORD dwStreamsCount )
        { return m_pXHVEngine->SetMaxPlaybackStreamsCount( dwStreamsCount ); }
    HRESULT SubmitIncomingVoicePacket( XUID xuidRemoteTalker, VOID* pvData, DWORD dwSize )
        { return m_pXHVEngine->SubmitIncomingVoicePacket( xuidRemoteTalker, pvData, dwSize ); }
    HRESULT SetProcessingMode( DWORD dwPort, XHV_PROCESSING_MODE processingMode )
        { return m_pXHVEngine->SetProcessingMode( dwPort, processingMode ); }

    // These functions wrap handling remote talkers, so that we can manage
    // the list of PlayerVoiceInfo structs
    HRESULT RegisterRemoteTalker( XUID xuidRemoteTalker );
    HRESULT UnregisterRemoteTalker( XUID xuidRemoteTalker );
    HRESULT ClearRemoteTalkers();
    HRESULT SetRemoteTalkerPriority( XUID xuidRemoteTalker, DWORD dwPort, XHV_PLAYBACK_PRIORITY priority );

    // These functions handle higher-level logical operations such as
    // muting and voice-through-speaker control
    HRESULT SetVoiceThroughSpeakers( BOOL bVoiceThroughSpeakers );
	BOOL	GetVoiceThroughSpeakers( VOID );
    HRESULT SetMute( XUID xuidRemoteTalker, DWORD dwPort, BOOL bMuted );
    HRESULT SetRemoteMute( XUID xuidRemoteTalker, DWORD dwPort, BOOL bRemoteMuted );
    BOOL    IsTalking( XUID xuidRemoteTalker, DWORD dwPort );

protected:
    HRESULT FindPlayerVoiceInfo( XUID xuidRemoteTalker, DWORD* pdwEntry );
    HRESULT RecalculatePriorities( PlayerVoiceInfo* pInfo );

    PXHVENGINE          m_pXHVEngine;
    DWORD               m_dwNumLocalTalkers;
    DWORD               m_dwMaxRemoteTalkers;
    DWORD               m_dwNumRemoteTalkers;
    PlayerVoiceInfo		m_pPlayerVoiceInfo[10];	// Avoid calling new!

    BOOL                m_bVoiceThroughSpeakers;
};

#endif // _XHVVOICEMANAGER_H_