// XBLive_MM.CPP
//
// Wrappers and extra utilities based around the matchsim generated code.
// Interface is borrowed from the SOF2 code, but modified quite a bit.
//


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SERVER SIDE

#include "xblive.h"
#include "xonline.h"
#include "match.h"
#include "xboxcommon.h"
#include "..\server\server.h"
#include "../ui/ui_local.h"
#include "../qcommon/cm_local.h"
#include "../client/client.h" 


// All servers are rated from 1 to 4
#define MAX_QOS		4


// External tools for servers to grab info from when advertising:
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_maxForceRank;
extern qboolean HasSetSaberOnly(void);
extern int RE_RegisterShaderNoMip( const char *name );


// Hardcoded list of map names, so that current map can be an index rather than string
// This needs to stay up-to-date.
const char	*mapArray[] = {
	"mp/ctf1",
	"mp/ctf2",
	"mp/ctf3",
	"mp/ctf4",
	"mp/ctf5",
	"mp/duel1",
	"mp/duel2",
	"mp/duel3",
	"mp/duel4",
	"mp/duel5",
	"mp/duel6",
	"mp/duel7",
	"mp/duel8",
	"mp/duel9",
	"mp/duel10",
	"mp/ffa1",
	"mp/ffa2",
	"mp/ffa3",
	"mp/ffa4",
	"mp/ffa5",
	"mp/siege_hoth",
	"mp/siege_desert",
	"mp/siege_korriban",
};
const int MAP_ARRAY_SIZE = sizeof(mapArray) / sizeof(mapArray[0]);

// Long names of all the maps. This should be done some other way.
const char	*mapLongArray[] = {
	"@MENUS_IMPERIAL_DROP_ZONE_ABR",
	"@MENUS_HOTH_WASTELAND_ABR",
	"@MENUS_YAVIN_HILLTOPS_ABR",
	"@MENUS_CORUSCANT_STREETS_ABR",
	"@MENUS_FACTORY_ABR",
	"@MENUS_BESPIN_COURTYARD_ABR",
	"@MENUS_GENERATOR_ROOM_ABR",
	"@MENUS_IMPERIAL_SHAFT_ABR",
	"@MENUS_IMPERIAL_CONTROL_ROOM_ABR",
	"@MENUS_TASPIR_LANDING_ABR",
	"@MENUS_YAVIN_TRAINING_AREA_ABR",
	"@MENUS_RANCOR_PIT_ABR",
	"@MENUS_ABANDONED_CITY_ABR",
	"@MENUS_HOTH_CANYON_ABR",
	"@MENUS_VJUN_FUEL_PROCESSING_ABR",
	"@MENUS_VJUN_SENTINEL_ABR",
	"@MENUS_KORRIBAN_TOMBS_ABR",
	"@MENUS_TATOOINE_CITY_ABR",
	"@MENUS_RIFT_SANCTUARY_ABR",
	"@MENUS_TASPIR_ABR",
	"@MENUS_HOTH_ATTACK_LOWER_ABR",
	"@MENUS_DESERT_RESCUE_LOWER_ABR",
	"@MENUS_KORRIBAN_VALLEY_LOWER_ABR",
};

const char *gameTypeArray[] = {
	"@MENUS_FREE_FOR_ALL_ABR",
	"",
	"",
	"@MENUS_DUEL_ABR",
	"@MENUS_POWERDUEL_ABR",
	"",
	"@MENUS_TEAM_FFA_ABR",
	"@MENUS_SIEGE_ABR",
	"@MENUS_CAPTURE_THE_FLAG_ABR",
	"",
};
const int GAMETYPE_ARRAY_SIZE = sizeof(gameTypeArray) / sizeof(gameTypeArray[0]);

// Our session, assuming that we're a server
CSession				session;	// from match.h

// Our two query objects, one used for optimatch and quickmatch,
// the other for joining games via session ID
COptiMatchQuery			query;		// from match.h
CJoinSessionByIDQuery	queryByID;	// from match.h

// Info about the session we're currently in
XBLClientData_t	xbc;
JoinType		joinVia;

// The currently selected optimatch game (from the UI's list)
int				optiMatchIndex = 0;

// make a wide char string from a char string
//
void charToWchar( WCHAR* wc, char* c)
{
	assert( wc );

	if(!c)
	{
		wc[0] = 0;
		return;
	}

	wsprintfW(wc, L"%hs", c);
}
 
// make a char string from a widechar string
//
void wcharToChar( char* c, WCHAR* wc )
{
	assert(c && wc);

	sprintf(c, "%ls", wc);
}


// Easy conversion between map name and index values

// Returns MAP_ARRAY_SIZE if map isn't found
int mapNameToIndex(const char *mapname)
{
	for ( int index = 0; index < MAP_ARRAY_SIZE; ++index)
		if (!stricmp(mapname, mapArray[index]))
			return index;
	return MAP_ARRAY_SIZE;
}

// Returns pointer to the table, don't stomp. NULL if not valid
const char *mapIndexToName(int index)
{
	if (index >= 0 && index < MAP_ARRAY_SIZE)
		return mapArray[index];
	return NULL;
}

// Returns pointer to the table, don't stomp. NULL if not valid
const char *mapIndexToLongName(int index)
{
	if (index >= 0 && index < MAP_ARRAY_SIZE)
		return mapLongArray[index];
	return NULL;
}

// Returns pointer to the table, don't stomp. NULL if not valid
const char *gameTypeIndexToName(int index)
{
	if (index >= 0 && index < GAMETYPE_ARRAY_SIZE)
		return gameTypeArray[index];
	return NULL;
}


// increment the player count advertised on this session
//
DWORD XBL_MM_AddPlayer( bool usePrivateSlot )
{
	if (!logged_on || !session.Exists())
		return -1;

	// SOF2 had a really bad system that checked for server's friends. We do what
	// MS says is good - all invites/joins go into private, everyone else goes public.
    if( usePrivateSlot && session.PrivateOpen )
    {
		session.PrivateFilled++;
		session.PrivateOpen--;
		session.TotalPlayers++;
    }
    else
    {
		// Either joined via matchmaking, or we don't have any more private slots
		// Give them a public slot.
		if ( session.PublicOpen )
		{
			session.PublicFilled++;
			session.PublicOpen--;
			session.TotalPlayers++;
		}
		else
		{
			assert( 0 );
		}
    }

	session.Update();

    return 0;
}

// decrement the player count advertised on this session
//
DWORD XBL_MM_RemovePlayer( bool usePrivateSlot )
{
	if(!logged_on || !session.Exists())
		return -1;

	// The value we're given only tells us if they were eligble for a private slot when
	// they joined, but they may have actually ended up in a public slot. As such, this
	// can get pretty wacky. Oh well.
	if ( usePrivateSlot && session.PrivateFilled )
	{
		session.PrivateFilled--;
		session.PrivateOpen++;
		session.TotalPlayers--;
	}
	else
	{
		if ( session.PublicFilled )
		{
			session.PublicFilled--;
			session.PublicOpen++;
			session.TotalPlayers--;
		}
		else
		{
			assert( 0 );
		}
	}

	session.Update();

	return 0;
}

// Is there a public slot available? SV_DirectConnect needs to know so it can reject
// clients when several people try to connect at once:
bool XBL_MM_PublicSlotAvailable( void )
{
	return session.PublicOpen;
}

// SOF2 had some silly two-stage thing. They stored off the session parms here, then used
// a couple globals to delay advertisement until later. I'm going to try and avoid that
void XBL_MM_Init_Session()
{
	// Fill in # of slots. OpenPublic is total slots, minus one for server, minus # reserved for private
	session.PrivateFilled		= 0;
	session.PrivateOpen			= sv_privateClients->integer;
	session.PublicFilled		= (com_dedicated->integer ? 0 : 1);	// Non-dedicated server fills a slot
	session.PublicOpen			= sv_maxclients->integer - (session.PrivateOpen + session.PublicFilled);
	session.TotalPlayers		= session.PublicFilled;

	// Get current map index, and gametype
	int index = mapNameToIndex( sv_mapname->string );
	if (index == MAP_ARRAY_SIZE)
	{
		Com_Error( ERR_FATAL, "Bad map name: %s\n", sv_mapname->string );
	}
	session.CurrentMap = index;
	session.GameType = sv_gametype->integer;

	// Copy the host's gamertag to the session name
	XONLINE_USER* pHostAccount = XBL_GetUserInfo( XBL_GetSelectedAccountIndex() );
	WCHAR sessionName[XONLINE_GAMERTAG_SIZE] = { 0 };
	if ( pHostAccount )
	{
		charToWchar( sessionName, pHostAccount->szGamertag );
	}
	else
	{
		charToWchar( sessionName, "unknown" );
	}
	session.SetSessionName( sessionName );

	// All other game options:
	session.FriendlyFire	= g_friendlyFire.integer;
	session.JediMastery		= g_maxForceRank.integer;
	session.SaberOnly		= HasSetSaberOnly();
	session.Dedicated		= com_dedicated->integer;

	// Actually create the session. If we don't call Process immediately, it explodes
	HRESULT hr = session.Create();
	if (hr != S_OK)
	{
		Com_Error( ERR_DROP, "Failed to create session: 0x%x\n", hr );
	}

	do
	{
		if( !XBL_PumpLogon() )
			return;
		hr = session.Process();
	} while ( session.IsCreating() );

	// VVFIXME - Better error handling
	if ( !session.Exists() )
	{
		Com_Error( ERR_DROP, "Failed to create session #2: 0x%x\n", hr );
	}

	// Fix for a bug. Server was using Notification API before advertising, so for
	// the first few seconds, had the wrong sessionID, and thus couldn't invite.
	// Force an update now:
	XBL_F_CheckJoinableStatus( true );
}

void XBL_MM_Update_Session()
{
	// VVFIXME - Do we need to ensure that slot counts are right?
	// Our gamertag hasn't changed (I hope) so we leave that alone.

	// Get current map index, and gametype
	int index = mapNameToIndex( sv_mapname->string );
	if (index == MAP_ARRAY_SIZE)
	{
		Com_Error( ERR_FATAL, "Bad map name: %s\n", sv_mapname->string );
	}
	session.CurrentMap = index;
	session.GameType = sv_gametype->integer;

	// All other game options:
	session.FriendlyFire	= g_friendlyFire.integer;
	session.JediMastery		= g_maxForceRank.integer;
	session.SaberOnly		= HasSetSaberOnly();
	session.Dedicated		= com_dedicated->integer;

	// Update the advertised session info
	session.Update();
}

// Ensure that our session is being advertised correctly
// Will update map name after a level change, etc...
void XBL_MM_Advertise()
{
	// If not on xboxlive dont post server
	if(!logged_on)
		return;

	if ( session.Exists() )
	{
		// Our session is already being advertised, update it
		XBL_MM_Update_Session();
	}
	else
	{
		// Brand new session
        XBL_MM_Init_Session();
	}
}


// Finish off and tidy up match making
void XBL_MM_Shutdown( bool processLogon )
{
	// We're killing our session, no matter what
	session.Delete();

	// XDK code to finish this off, while not letting the logon task expire,
	// except if we're already leaving live, where we can get into a recursive
	// com_error situation:
	HRESULT hr;
	do
	{
		if( processLogon && !XBL_PumpLogon() )
			return;
		hr = session.Process();
	} while ( session.IsDeleting() );
}


// Search for and join a good online server
// GameType is optional - X_MATCH_NULL_INTEGER to omit
bool XBL_MM_QuickMatch(ULONGLONG GameType)
{
	// Reuse our optimatch query object. VVFIXME - needs to be torn down first?

	HRESULT hr = query.Query(
		GameType,
		X_MATCH_NULL_INTEGER,		// CurrentMap
		0,							// MinimumPlayers
		8,							// MaximumPlayers
		X_MATCH_NULL_INTEGER,		// FriendlyFire
		X_MATCH_NULL_INTEGER,		// JediMastery
		X_MATCH_NULL_INTEGER,		// SaberOnly
		X_MATCH_NULL_INTEGER);		// Dedicated
	if ( FAILED( hr ) )
	{
		UI_xboxErrorPopup( XB_POPUP_MATCHMAKING_ERROR );
		return false;
	}
	// Keep servicing the query until it completes.
	// The logon task must also be serviced in order to remain connected. 
	do
	{
		if( !XBL_PumpLogon() )
			return false;
		hr = query.Process();
	} while( query.IsRunning() );

	if( !query.Succeeded() )
	{
		UI_xboxErrorPopup( XB_POPUP_MATCHMAKING_ERROR );
		return false;
	}

	// VVFIXME - Need to do probing, and pick best session, not just the first one
	if (!query.Results.Size())
	{
		UI_xboxErrorPopup( XB_POPUP_QUICKMATCH_NO_RESULTS );
		return false;
	}

	COptiMatchResult &server(query.Results[0]);

	XBL_MM_SetJoinType( VIA_QUICKMATCH );
	// VVFIXME - Experiment, leave this out so that the screen doesn't get trashed
	// right as we connect?
//	Menus_CloseAll();
	Net_XboxConnect(&server.SessionID, &server.KeyExchangeKey, &server.HostAddress);
/*
    // warn of lag and join(or not) via ui
    //
    if( cls.globalServers[bestSesh].ping < MINIMUM_QOS )
    {
        joinServerSlot = bestSesh;
	    Cvar_Set(CVAR_UI_XBOXREBOOTTITLE, " ");
        Cvar_Set(CVAR_UI_XBOXREBOOTMESSAGE, StringTable_Get(XSTR_GAMEPLAY_AFFECTED_BY_NETWORK));
		//Menus_ActivateByName("ingame_small_bgd");
        Menus_CloseByName("quickmatch_popup");
        Menus_ActivateByName("xblive_slow_warning");
       return false;
    }
    XBL_MM_JoinServer( bestSesh );
*/
	return true;
}


// work out the value of a servers QoS
//
int getQoSValue( XNQOSINFO* qosInfo )
{
	if ( !qosInfo )
		return 0;	// Sentinel for an as-yet-unknown ping

	WORD avgPing = qosInfo->wRttMedInMsecs;
	if (avgPing < 150)
		return 3;
	else if (avgPing < 250)
		return 2;
	else
		return 1;
}


// Find currently running sessions
int XBL_MM_Find_Session(ULONGLONG GameType, // Optional: X_MATCH_NULL_INTEGER to omit
						const char *mapName, // Optional: "any" to omit
						ULONGLONG MinimumPlayers,
						ULONGLONG MaximumPlayers,
						ULONGLONG FriendlyFire, // Optional: X_MATCH_NULL_INTEGER to omit
						ULONGLONG JediMastery, // Optional: X_MATCH_NULL_INTEGER to omit
						ULONGLONG SaberOnly, // Optional: X_MATCH_NULL_INTEGER to omit
						ULONGLONG Dedicated) // Optional: X_MATCH_NULL_INTEGER to omit
{
	// Kill off a previous query that's still running
	query.Cancel();

	ULONGLONG CurrentMap = mapNameToIndex( mapName );
	if( CurrentMap == MAP_ARRAY_SIZE )
		CurrentMap = X_MATCH_NULL_INTEGER;

	HRESULT hr = query.Query(
		GameType,
		CurrentMap,
		MinimumPlayers,
		MaximumPlayers,
		FriendlyFire,
		JediMastery,
		SaberOnly,
		Dedicated);
	if ( FAILED( hr ) )
	{
		Com_Error( ERR_DROP, "@MENUS_XBOX_LOST_CONNECTION" );
	}
	// Keep servicing the query until it completes.
	// The logon task must also be serviced in order 
	// to remain connected. 
	do
	{
		if( !XBL_PumpLogon() )
			return 0;
		hr = query.Process();
	} while( query.IsRunning() );

	if( !query.Succeeded() )
	{
		Com_Error( ERR_DROP, "@MENUS_XBOX_LOST_CONNECTION" );
	}

	if (!query.Results.Size())
	{
		// VVFIXME - handle search that returns no results
		Com_Printf("No games found in query\n");
		return 0;
	}

	// The above gets all results, and does initial (can we connect) probing.
	// We wait for that, then begin real probing here, which is done async.
	query.Probe();
	optiMatchIndex = 0;

	return query.Results.Size();
}


// Joins the specified server from the optimatch results list
void XBL_MM_JoinServer( void )
{
	// Sanity check
	if( optiMatchIndex < 0 || optiMatchIndex >= query.Results.Size() )
		return;

	COptiMatchResult *res = &query.Results[ optiMatchIndex ];

	XBL_MM_SetJoinType( VIA_OPTIMATCH );
	Net_XboxConnect( &res->SessionID, &res->KeyExchangeKey, &res->HostAddress );

	query.Cancel();
}


// Single function to run a query by session ID. Several other functions
// use this to get various results (or join servers)
static HRESULT runSessionIDQuery( const XNKID *sessionID )
{
	HRESULT hr;
	hr = queryByID.Query( *(ULONGLONG *)sessionID );
	if ( FAILED( hr ) )
	{
		Com_Error( ERR_FATAL, "Session ID query failed with 0x%x\n", hr );
	}

	// Keep servicing the query until it completes.
	// The logon task must also be serviced in order to remain connected. 
	do
	{
		if( !XBL_PumpLogon() )
			return 0;
		hr = queryByID.Process();
	} while( queryByID.IsRunning() );

	if ( !queryByID.Succeeded() )
	{
		Com_Error( ERR_FATAL, "Session ID query failed #2 with 0x%x\n", hr );
	}

	return hr;
}

// Returns true if the sessions traffic performs lower than minimum levels for invite
bool XBL_MM_ThisSessionIsLagging( const XNKID *sessionID )
{
	// DO the query first...
	runSessionIDQuery( sessionID );

	// This happens if it's a different title, for example.
	if ( !queryByID.Results.Size() )
	{
		return false;
	}

	// Start probing
	queryByID.Probe();

	// Keep probing, and keep the logon task serviced.
	// VVFIXME - Timeout here?
	HRESULT hr;
	do
	{
		if( !XBL_PumpLogon() )
			return false;
		hr = queryByID.Process();
	} while( queryByID.IsProbing() );

	return getQoSValue( queryByID.Results[0].pQosInfo ) < MINIMUM_QOS;
}

namespace ui
{
extern void Menus_CloseAll( void );
}

// Connnect to a game we have the session ID for
bool XBL_MM_ConnectViaSessionID( const XNKID *sessionID, bool invited )
{
	// Do the query...
	runSessionIDQuery( sessionID );

	// No results? (Should only be one)
	if ( !queryByID.Results.Size() )
	{
		return false;
	}

	CJoinSessionByIDResult &server(queryByID.Results[0]);
	if (server.PrivateOpen < 1 && server.PublicOpen < 1)
	{
		// No slots available
		return false;
	}

	// OK. If we're currently hosting a dedicated server, we need to clean things up:
	if( com_dedicated->integer )
	{
		// Marks us as no longer joinable or playing:
		XBL_F_OnClientLeaveSession();

		// Code copied from "Leave" in ui_main.c
		Cvar_SetValue( "cl_paused", 0 );
		Key_SetCatcher( KEYCATCH_UI );
		Cvar_SetValue( "cl_running", 1 );
		SV_Shutdown( "Server quit\n" );

		CL_ShutdownUI();
		extern void RE_Shutdown( qboolean destroyWindow );
		RE_Shutdown( qfalse );
		CL_Disconnect( qtrue );

		Cvar_SetValue( "dedicated", 0 );
		Cvar_SetValue( "ui_dedicated", 0 );
		CL_FlushMemory();

		ui::Menus_CloseAll();
	}
	else if( cls.state == CA_ACTIVE )
	{
		// If we were already playing, we need to disconnect earlier than normal,
		// to clean up XBL stuff. Don't delete textures, we need to draw the connect screen:
		CL_Disconnect( qtrue, qfalse );
		Net_XboxDisconnect();
	}

	Net_XboxConnect(&server.SessionID, &server.KeyExchangeKey, &server.HostAddress);

 	return true;
}

// Check if session exists
bool XBL_MM_IsSessionIDValid(const XNKID *sessionID)
{
	// Do the query...
	runSessionIDQuery( sessionID );

	// Did we get any hits?
	return bool( queryByID.Results.Size() );
}

// Remember how we started connecting to a session, in case it turns out to suck
void XBL_MM_SetJoinType(JoinType way)
{
    joinVia = way;
}

// Are we eligible for a private slot in the game we're connecting to?
bool XBL_MM_CanUsePrivateSlot( void )
{
	return ((joinVia == VIA_FRIEND_JOIN) ||
			(joinVia == VIA_FRIEND_INVITE));
}

extern void  UI_JoinSession();
extern void  UI_JoinInvite();

//  Join a server even after recieving a message saying it was low QoS 
void XBL_MM_JoinLowQOSSever()
{
	if( joinVia == VIA_FRIEND_JOIN )
	{
		UI_JoinSession();
	}

	else if( joinVia == VIA_FRIEND_INVITE)
	{
		UI_JoinInvite();
	}

/*
	else if( joinVia == VIA_OPTIMATCH )
	{
		XBL_MM_JoinServer( joinServerSlot );
	}

	else if( joinVia == VIA_QUICKMATCH )
	{
		XBL_MM_JoinServer( joinServerSlot );
	}
*/
}

//  Bail out of joining a server after recieving a message saying it was low QoS 
void XBL_MM_DontJoinLowQOSSever()
{
	// If we were originally invited, make sure to decline the invitation
	if( joinVia == VIA_FRIEND_INVITE )
	{
		HRESULT result = XBL_F_PerformMenuAction(UI_F_GAMEDECLINE);
	}

	// All other scenarios require no extra work
}

// run this code every game tick
// will only be called while logged on
//
void XBL_MM_Tick()
{
	// VVFIXME - SOF2 re-advertised after some crazy timeout.

	// New version just ticks the session object as well, it does nothing if it's not real
	HRESULT hr = session.Process();
#ifdef _DEBUG
	if ( FAILED( hr ) )
		Com_Printf("session.Process() failed: %s\n", getXBLErrorName(hr));
#endif

	// The only time we need to do async work on the query is when probing for QoS
	if ( query.IsProbing() )
	{
		query.Process();
	}
}

//
// Big pile of functions that the UI uses to pull system link results for display
//
int Syslink_GetNumServers( void )
{
	return cls.numlocalservers;
}

extern serverInfo_t *SysLink_GetServer( int index );

const char *Syslink_GetServerMap( const int index )
{
	serverInfo_t *s = SysLink_GetServer( index );

	int mapIndex = mapNameToIndex( s->mapName );
	if( mapIndex == MAP_ARRAY_SIZE )
		return "";

	return mapIndexToLongName( mapIndex );
}

const char *Syslink_GetServerClients( const int index )
{
	serverInfo_t *s = SysLink_GetServer( index );

	return va( "%d/%d", s->clients, s->maxClients );
}

const char *Syslink_GetServerGametype( const int index )
{
	serverInfo_t *s = SysLink_GetServer( index );

	return gameTypeIndexToName( s->gameType );
}

int Syslink_GetServerSaberOnly( const int index )
{
	serverInfo_t *s = SysLink_GetServer( index );

	if( s->saberOnly )
		return RE_RegisterShaderNoMip( "gfx/mp/saber_only" );
	else
		return -1;
}

int Syslink_GetServerDisableForce( const int index )
{
	serverInfo_t *s = SysLink_GetServer( index );

	if( s->forceDisable )
		return RE_RegisterShaderNoMip( "gfx/mp/noforce_bw" );
	else
		return -1;
}

//
// Big pile of functions that the UI uses to pull optimatch results for display
//
int XBL_MM_GetNumServers( void )
{
	return query.Results.Size();
}

const char *XBL_MM_GetServerName( const int index )
{
	static char retVal[XATTRIB_SESSION_NAME_MAX_LEN+1];

	wcharToChar( retVal, query.Results[index].SessionName );
	return retVal;
}

const char *XBL_MM_GetServerMap( const int index )
{
	return mapIndexToLongName( query.Results[index].CurrentMap );
}

const char *XBL_MM_GetServerClients( const int index )
{
	int openSlots = query.Results[index].PublicOpen + query.Results[index].PrivateOpen;
	int filledSlots = query.Results[index].PublicFilled + query.Results[index].PrivateFilled;
	return va( "%d/%d", filledSlots, filledSlots + openSlots );
}

const char *XBL_MM_GetServerGametype( const int index )
{
	return gameTypeIndexToName( query.Results[index].GameType );
}

int XBL_MM_GetServerSaberOnly( const int index )
{
	if( query.Results[index].SaberOnly )
		return RE_RegisterShaderNoMip( "gfx/mp/saber_only" );
	else
		return -1;
}

int XBL_MM_GetServerDisableForce( const int index )
{
	if( query.Results[index].JediMastery == 0 )
		return RE_RegisterShaderNoMip( "gfx/mp/noforce_bw" );
	else
		return -1;
}

int XBL_MM_GetServerPing( const int index )
{
	int pingVal = getQoSValue( query.Results[index].pQosInfo );

	switch( pingVal )
	{
		case 1:	// Bad
			return RE_RegisterShaderNoMip( "gfx/mp/dot_red" );
		case 2:	// Average
			return RE_RegisterShaderNoMip( "gfx/mp/dot_yellow" );
		case 3:	// Good
			return RE_RegisterShaderNoMip( "gfx/mp/dot_green" );
		case 0:	// Unknown
		default:
			return -1;
	}
}

void XBL_MM_SetChosenServerIndex( const int index )
{
	// Just save this away:
	optiMatchIndex = index;

	// Also set the cvar that UI uses to draw the host's gamertag in the status line
	Cvar_Set( "xbl_gamertag", XBL_MM_GetServerName( index ) );
}

// Used by the UI when the user picks a server to join, or backs out of the results screen
void XBL_MM_CancelProbing( void )
{
	query.Cancel();
}
