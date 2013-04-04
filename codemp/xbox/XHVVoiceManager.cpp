//-----------------------------------------------------------------------------
// File: XHVVoiceManager.cpp
//
// Desc: Wraps the XHV voice engine and provides a simple interface to the 
//          game
//
// Hist: 05.06.03 - New for the June 2003 XDK
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "XHVVoiceManager.h"
#include <assert.h>


//-----------------------------------------------------------------------------
// Name: CXHVVoiceManager (ctor)
// Desc: Initializes member variables
//-----------------------------------------------------------------------------
CXHVVoiceManager::CXHVVoiceManager()
{
    m_pXHVEngine            = NULL;
    m_dwNumLocalTalkers     = 0;
    m_dwMaxRemoteTalkers    = 0;
    m_dwNumRemoteTalkers    = 0;
//    m_pPlayerVoiceInfo      = NULL;
    m_bVoiceThroughSpeakers = FALSE;
}



//-----------------------------------------------------------------------------
// Name: ~CXHVVoiceManager (dtor)
// Desc: Performs any final cleanup that is needed
//-----------------------------------------------------------------------------
CXHVVoiceManager::~CXHVVoiceManager()
{
    Shutdown();
}


//-----------------------------------------------------------------------------
// Name: Initialize
// Desc: Initializes the XHV voice manager
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::Initialize( XHV_RUNTIME_PARAMS* pXHVParams )
{
    assert( pXHVParams );

    // Create the engine
    HRESULT hr = XHVEngineCreate( pXHVParams, &m_pXHVEngine );
    if( FAILED( hr ) )
        return hr;

    // Enable voicechat and loopback modes only
    m_pXHVEngine->EnableProcessingMode( XHV_LOOPBACK_MODE );
    m_pXHVEngine->EnableProcessingMode( XHV_VOICECHAT_MODE );
    // m_pXHVEngine->EnableProcessingMode( XHV_SR_MODE );

    m_dwNumLocalTalkers     = pXHVParams->dwMaxLocalTalkers;
    m_dwMaxRemoteTalkers    = pXHVParams->dwMaxRemoteTalkers;

    // Register each local talker immediately, but leave them inactive
    for( DWORD i = 0; i < m_dwNumLocalTalkers; i++ )
    {
        m_pXHVEngine->RegisterLocalTalker( i );
        m_pXHVEngine->SetProcessingMode( i, XHV_INACTIVE_MODE );
    }

    // Allocate space for the PlayerVoiceInfo structs
//    m_pPlayerVoiceInfo = new PlayerVoiceInfo[ m_dwMaxRemoteTalkers ];
//    if( NULL == m_pPlayerVoiceInfo )
//    {
//        Shutdown();
//        return E_OUTOFMEMORY;
//    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: Shutdown
// Desc: Shuts down the object
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::Shutdown()
{
    if( m_pXHVEngine )
    {
        m_pXHVEngine->Release();
        m_pXHVEngine = NULL;
    }

//    if( m_pPlayerVoiceInfo )
//    {
//        delete[] m_pPlayerVoiceInfo;
//        m_pPlayerVoiceInfo = NULL;
//    }

    m_dwNumLocalTalkers     = 0;
    m_dwMaxRemoteTalkers    = 0;
    m_dwNumRemoteTalkers    = 0;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: RegisterRemoteTalker
// Desc: Registers a new remote talker
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::RegisterRemoteTalker( XUID xuidRemoteTalker )
{
    // XHV allows you to call RegisterRemoteTalker more than once with the
    // same XUID.  For convenience, we'll emulate that behavior
    if( SUCCEEDED( FindPlayerVoiceInfo( xuidRemoteTalker, NULL ) ) )
        return S_OK;

    // Verify we're within our limits
    assert( m_dwNumRemoteTalkers < m_dwMaxRemoteTalkers );

    // Register the remote talker with XHV
    HRESULT hr;
    hr = m_pXHVEngine->RegisterRemoteTalker( xuidRemoteTalker );
    if( FAILED( hr ) )
        return hr;

    // Set up a new PlayerVoiceInfo struct for the player
    PlayerVoiceInfo* pRemoteTalker = &m_pPlayerVoiceInfo[ m_dwNumRemoteTalkers ];
    pRemoteTalker->xuid = xuidRemoteTalker;
    for( DWORD i = 0; i < XGetPortCount(); i++ )
    {
        pRemoteTalker->bMuted[ i ]          = FALSE;
        pRemoteTalker->bRemoteMuted[ i ]    = FALSE;
        pRemoteTalker->priority[ i ]        = XHV_PLAYBACK_PRIORITY_NEVER;
    }

    ++m_dwNumRemoteTalkers;

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: UnregisterRemoteTalker
// Desc: Unregisters an existing remote talker
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::UnregisterRemoteTalker( XUID xuidRemoteTalker )
{
    // Find the entry to remove from our PlayerVoiceInfo list
    DWORD dwEntry;
    if( FAILED( FindPlayerVoiceInfo( xuidRemoteTalker, &dwEntry ) ) )
        return E_FAIL;

    // Remove the entry by overwriting it with the last entry in the list
    memcpy( &m_pPlayerVoiceInfo[ dwEntry ],
            &m_pPlayerVoiceInfo[ m_dwNumRemoteTalkers - 1 ],
            sizeof( PlayerVoiceInfo ) );
    --m_dwNumRemoteTalkers;

    // Unregister the remote talker with XHV
    HRESULT hr = m_pXHVEngine->UnregisterRemoteTalker( xuidRemoteTalker );
    if( FAILED( hr ) )
        return hr;

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: ClearRemoteTalkers
// Desc: Unregisters all remaining remote talkers
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::ClearRemoteTalkers()
{
    for( DWORD i = 0; i < m_dwNumRemoteTalkers; i++ )
    {
        m_pXHVEngine->UnregisterRemoteTalker( m_pPlayerVoiceInfo[ i ].xuid );
    }
    m_dwNumRemoteTalkers = 0;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: SetRemoteTalkerPriority 
// Desc: Sets the priority of a remote talker.
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::SetRemoteTalkerPriority( XUID xuidRemoteTalker, 
                                                  DWORD dwPort, 
                                                  XHV_PLAYBACK_PRIORITY priority )
{
    DWORD dwEntry;
    if( FAILED( FindPlayerVoiceInfo( xuidRemoteTalker, &dwEntry ) ) )
        return E_FAIL;

    // Set our copy of the priority
    PlayerVoiceInfo* pInfo = &m_pPlayerVoiceInfo[ dwEntry ];
    pInfo->priority[ dwPort ] = priority;

    // Recalculate priorities for the remote talker
    if( FAILED( RecalculatePriorities( pInfo ) ) )
        return E_FAIL;

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: RecalculatePriorities
// Desc: Calculates and sets the appropriate priorities of the remote talker.
//       If Voice Through Speakers is disabled, it sets the priority for each
//          local talker based upon their priority for the remote talker and 
//          their mute settings, and sets speaker priority to NEVER.
//       If Voice Through Speakers is enabled, it sets the priority for each
//          local talker to NEVER, and sets speaker priority based on the
//          following rules:
//          1) If any local talker has muted the remote talker, set to NEVER
//          2) If the remote talker has muted any local talker, set to NEVER
//          3) Otherwise, priority is the max of each local talker's priority
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::RecalculatePriorities( PlayerVoiceInfo* pInfo )
{
    if( !m_bVoiceThroughSpeakers )
    {
        // Set appropriate local talker priorities
        for( DWORD dwPort = 0; dwPort < m_dwNumLocalTalkers; dwPort++ )
        {
            XHV_PLAYBACK_PRIORITY priority = pInfo->priority[ dwPort ];
            if( pInfo->bMuted[ dwPort ] || pInfo->bRemoteMuted[ dwPort ] )
            {
                priority = XHV_PLAYBACK_PRIORITY_NEVER;
            }

            m_pXHVEngine->SetPlaybackPriority( pInfo->xuid, 
                                               dwPort, 
                                               priority );
        }

        // Set speaker priority to NEVER
        m_pXHVEngine->SetPlaybackPriority( pInfo->xuid, 
                                           XHV_PLAYBACK_TO_SPEAKERS,
                                           XHV_PLAYBACK_PRIORITY_NEVER );
    }
    else
    {
        XHV_PLAYBACK_PRIORITY priority = XHV_PLAYBACK_PRIORITY_NEVER;

        // Turn off local talker headphone output, and calculate
        // maximum priority while accounting for muting
        for( DWORD dwPort = 0; dwPort < m_dwNumLocalTalkers; dwPort++ )
        {
            m_pXHVEngine->SetPlaybackPriority( pInfo->xuid, 
                                               dwPort, 
                                               XHV_PLAYBACK_PRIORITY_NEVER );

            // If anyone has muted or been muted by the remote talker,
            // must set priority to NEVER
            if( pInfo->bMuted[ dwPort ] || pInfo->bRemoteMuted[ dwPort ] )
            {
                priority = XHV_PLAYBACK_PRIORITY_NEVER;
                break;
            }

            // Calculate maximum priority - lower values correspond 
            // to higher priority
            if( pInfo->priority[ dwPort ] < priority )
                priority = pInfo->priority[ dwPort ];
        }

        m_pXHVEngine->SetPlaybackPriority( pInfo->xuid,
                                           XHV_PLAYBACK_TO_SPEAKERS,
                                           priority );
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: SetVoiceThroughSpeakers
// Desc: Turns voice-through-speakers mode off and on
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::SetVoiceThroughSpeakers( BOOL bVoiceThroughSpeakers )
{
    m_bVoiceThroughSpeakers = bVoiceThroughSpeakers;

    // Recalculate the priorities for each remote talker
    for( DWORD i = 0; i < m_dwNumRemoteTalkers; i++ )
    {
        RecalculatePriorities( &m_pPlayerVoiceInfo[ i ] );
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: SetVoiceThroughSpeakers
// Desc: Turns voice-through-speakers mode off and on
//-----------------------------------------------------------------------------
BOOL CXHVVoiceManager::GetVoiceThroughSpeakers( VOID )
{
	return m_bVoiceThroughSpeakers;
}


//-----------------------------------------------------------------------------
// Name: SetMute
// Desc: Mutes/UnMutes a remote talker for a given local talker
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::SetMute( XUID xuidRemoteTalker, DWORD dwPort, BOOL bMuted )
{
    DWORD dwEntry;
    if( FAILED( FindPlayerVoiceInfo( xuidRemoteTalker, &dwEntry ) ) )
        return E_FAIL;

    PlayerVoiceInfo* pInfo = &m_pPlayerVoiceInfo[ dwEntry ];
    pInfo->bMuted[ dwPort ] = bMuted;
    RecalculatePriorities( pInfo );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: SetRemoteMute
// Desc: RemoteMutes/RemoteUnMutes a remote talker for a given local talker
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::SetRemoteMute( XUID xuidRemoteTalker, DWORD dwPort, BOOL bRemoteMuted )
{
    DWORD dwEntry;
    if( FAILED( FindPlayerVoiceInfo( xuidRemoteTalker, &dwEntry ) ) )
        return E_FAIL;

    PlayerVoiceInfo* pInfo = &m_pPlayerVoiceInfo[ dwEntry ];
    pInfo->bRemoteMuted[ dwPort ] = bRemoteMuted;
    RecalculatePriorities( pInfo );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: IsTalking
// Desc: Determines whether a remote talker is currently talking to a local
//          talker.
//-----------------------------------------------------------------------------
BOOL CXHVVoiceManager::IsTalking( XUID xuidRemoteTalker, DWORD dwPort )
{
    // First, see if the remote talker is currently talking
    BOOL bIsTalking = m_pXHVEngine->IsTalking( xuidRemoteTalker );

    // Although they're talking, a local player may not hear them due to
    // muting - possibly even another local talker being muted, if Voice
    // Through Speakers is on.
    if( bIsTalking )
    {
        DWORD dwEntry;
        if( FAILED( FindPlayerVoiceInfo( xuidRemoteTalker, &dwEntry ) ) )
            return FALSE;
        PlayerVoiceInfo* pInfo = &m_pPlayerVoiceInfo[ dwEntry ];

        if( !m_bVoiceThroughSpeakers )
        {
            // Voice Through Speakers OFF - check this local talker's
            // mute settings
            if( pInfo->bMuted[ dwPort ] || pInfo->bRemoteMuted[ dwPort ] )
                bIsTalking = FALSE;
        }
        else
        {
            // Voice Through Speakers ON - check every local talker's
            // mute settings
            for( DWORD i = 0; i < m_dwNumLocalTalkers; i++ )
            {
                if( pInfo->bMuted[ i ] || pInfo->bRemoteMuted[ i ] )
                    bIsTalking = FALSE;
            }
        }
    }

    return bIsTalking;
}



//-----------------------------------------------------------------------------
// Name: FindPlayerVoiceInfo
// Desc: Helper function to find an entry in the player voice info list
//-----------------------------------------------------------------------------
HRESULT CXHVVoiceManager::FindPlayerVoiceInfo( XUID xuidRemoteTalker, DWORD* pdwEntry )
{
    for( DWORD i = 0; i < m_dwNumRemoteTalkers; i++ )
    {
        if( XOnlineAreUsersIdentical( &m_pPlayerVoiceInfo[i].xuid,
                                      &xuidRemoteTalker ) )
        {
            if( pdwEntry )
            {
                *pdwEntry = i;
            }
            return S_OK;
        }
    }

    return E_FAIL;
}
