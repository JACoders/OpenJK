// XBLive_PL.cpp
//
// Player List functionality for XBox Live
//


#include "XBLive.h"
#include "XBoxCommon.h"
#include "..\cgame\cg_local.h"
#include "../qcommon/qcommon.h"
#include "XBVoice.h"
#include "../server/server.h"
#include "../ui/ui_shared.h"

extern int RE_RegisterShaderNoMip( const char *name );
extern const char *SE_GetString( const char *psPackageAndStringReference );

// Icon names that make up the cycle when someone is talking:
const char *XBL_SPEAKINGCYCLE_PL[] =
{
	"gfx/mp/speaking_1_icon",
	"gfx/mp/speaking_2_icon",
	"gfx/mp/voice_on_icon",
};
#define	MAX_SPEAK_CYCLE 3
#define SPEAK_CYCLE_DELAY 10

//DEFINES
#define		PLAYERSLIST_UPDATE_RATE 60

//VARIABLES

int			playerIndex			= 0;	// Player index chosen from the list
int			selectedIndex		= 0;	// Index of currently selected player - if the list changes during popup
XUID		selectedXuid;				// XUID of selected player, in case the list REALLY changes
WCHAR		selectedGamertag[XONLINE_GAMERTAG_SIZE];	// g-tag of selected player, "
int			speakCycleIndex		= 0;	// index of current frame in speak cycle animation
int			speakCycleDelay		= 0;	// frames since last cycle increment
XBHistory	xbOfflineHistory;
bool		playerListActive = false;	// Determine if play list is active
BYTE		tickCount			= 0;	// Amount of tick counts that have passed

// ******************************************************
// INITIALIZING FUNCTIONS
// *******************************************************
void XBL_PL_Init(void)
{
	playerListActive = true;
	tickCount = PLAYERSLIST_UPDATE_RATE;
	XBL_F_GenerateFriendsList();
}

// ******************************************************
// CLEANUP FUNCTIONS
// *******************************************************
void XBL_PL_Cleanup(void)
{
	playerListActive = false;
	XBL_F_ReleaseFriendsList();
}

// when this client leaves the session
//
void XBL_PL_OnClientLeaveSession()
{
	memset( &xbOfflineHistory, 0, sizeof(xbOfflineHistory) );
}

// **********************************************************
// FUNCTIONS THE MENU SYSTEM USES TO COMMUNICATE TO PLAYERS *
// **********************************************************

// Internally, the current selection is stored in playerIndex
// This value is absolute - all mapping from pseudo-values in the UI is
// done in the set/get functions. Values for playerIndex:
// -1							: Divider bar
// 0 .. (MAX_ONLINE_PLAYERS-1)	: Active players (should never be ours)
// MAX_ONLINE_PLAYERS .. +10	: History entries
//
// HISTORY_OFFSET is defined to be MAX_ONLINE_PLAYERS for ease of use

// The UI has a different set of values, because they need to map to
// the entries in the listbox directly. Thus, indexes coming from the UI
// will be:
// 0 .. (clients-2)	: Players, other than ourselves
// (clients-1)		: Divider
// clients .. +10	: History

// Get the number of active players --- makes me sad way of doing it :(
int XBL_PL_GetNumActivePlayers( void )
{
	int count = 0;
	// Loop through the player lists and test the active flag
	for(int i=0; i<MAX_ONLINE_PLAYERS; i++)
	{
		if(xbOnlineInfo.xbPlayerList[i].isActive)
			count++;
	}

	return count - 1; //do not count yourself
}

// Get the number of active entries in the history list
int XBL_PL_GetNumActiveHistory( void )
{
	int count = 0;
	// Loop through the player lists and test the active flag
	for(int i = 0; i < MAX_OFFLINE_HISTORY; i++)
	{
		if(xbOfflineHistory.historyList[i].isActive)
			count++;
	}

	return count;
}

// Get the total number of players in the player list (active+history-you)
int XBL_PL_GetNumPlayers( void )
{
	return XBL_PL_GetNumActivePlayers() + XBL_PL_GetNumActiveHistory();
}


// Converts a UI index into an index into the xbPlayerList array
int XBL_PL_IndexToPlayerIndexOnline(const int index)
{
	int count = 0;

	for(int i=0; i<MAX_ONLINE_PLAYERS; i++)
	{
		// Scan through all active players other than ourself
		if(xbOnlineInfo.xbPlayerList[i].isActive && xbOnlineInfo.localIndex != i)
		{
			if(count == index)
				return i;
			count++;
		}
	}

	return -1;
}

// Converts a pseudo-UI index into an index into the xbOfflineHistory array
int XBL_PL_IndexToPlayerIndexHistory(const int index)
{
	int count = 0;

	for(int i=0; i<MAX_OFFLINE_HISTORY; i++)
	{
		// Scan through all active entries in the history list
		if(xbOfflineHistory.historyList[i].isActive)
		{
			// When playerIndex points into the history list, it is offset so we know
			if(count == index)
				return (i + HISTORY_OFFSET);
			count++;
		}
	}

	return -1;
}

// Very handy utility, converts a UI player list index (from the feeder)
// into a valid playerIndex (as above)
int XBL_PL_IndexToPlayerIndex(const int index)
{
	int numActive = XBL_PL_GetNumActivePlayers();

	if(index < numActive)
	{
		// It's a low (current player) index
		return XBL_PL_IndexToPlayerIndexOnline( index );
	}
	else if(index == numActive)
	{
		// It's the index of the divider
		return -1;
	}
	else
	{
		// It's a history index - shift down
		return XBL_PL_IndexToPlayerIndexHistory( index - (numActive + 1) );
	}
}


// Set the stored playerIndex from the UI's index into the player list
void XBL_PL_SetPlayerIndex(const int index)
{
	playerIndex = XBL_PL_IndexToPlayerIndex( index );
}


// This sets all the cvars used in the playerlist popup window
bool XBL_PL_SetCurrentInfo( void )
{
	// We always set the date, regardless:
	SYSTEMTIME time;
	GetSystemTime(&time);
	Cvar_Set("pl_sessionDate", va("%d/%d/%d", time.wMonth, time.wDay, time.wYear) );

	if(playerIndex >= 0 && playerIndex < MAX_ONLINE_PLAYERS)
	{
		// Sanity check - dedicated server is in last slot, has no clientinfo
		assert( xbOnlineInfo.xbPlayerList[playerIndex].isActive );

		// Selected player is in the current game's players list
		if( xbOnlineInfo.xbPlayerList[playerIndex].isActive )
		{
			// And they're active. That's good. OK, copy things into cvars
			Cvar_Set( "pl_selectedName", xbOnlineInfo.xbPlayerList[playerIndex].name );

			if(xbOnlineInfo.xbPlayerList[playerIndex].flags & MUTED_PLAYER)
				Cvar_Set("pl_selectedMute", "1");
			else
				Cvar_Set("pl_selectedMute", "0");

			// Only show the friend-related options if logged into live:
			if( logged_on )
			{
				if( xbOnlineInfo.xbPlayerList[playerIndex].friendshipStatus == FRIEND_RQST )
					Cvar_Set("pl_selectedFriend", "1");
				else if( xbOnlineInfo.xbPlayerList[playerIndex].friendshipStatus == FRIEND_YES )
					Cvar_Set("pl_selectedFriend", "2");
				else
					Cvar_Set("pl_selectedFriend", "0");
			}
			else
			{
				Cvar_Set("pl_selectedFriend", "-1");
			}

			if(com_sv_running->integer) {
				if(svs.clients[playerIndex].state == CS_ACTIVE) {
					Cvar_Set("pl_allowKick", "1");
				} else {
					Cvar_Set("pl_allowKick", "0");
				}
			} else {
				Cvar_Set("pl_allowKick", "0");
			}


			// Remember this index, so that if people join/leave, we don't use the popup
			// to do something to the wrong player!
			selectedIndex = playerIndex;
			// Remember the XUID and gamertag, so that we can many things even if the
			// list does change while we're in the popup
			selectedXuid = xbOnlineInfo.xbPlayerList[selectedIndex].xuid;
			charToWchar( selectedGamertag, xbOnlineInfo.xbPlayerList[selectedIndex].name );
			return true;
		}
	}
	else if(playerIndex >= HISTORY_OFFSET && playerIndex < HISTORY_OFFSET + MAX_OFFLINE_HISTORY)
	{
		int index = playerIndex - HISTORY_OFFSET;

		// Sanity check
		assert( xbOfflineHistory.historyList[index].isActive );

		if(xbOfflineHistory.historyList[index].isActive)
		{
			// OK. Copy things into cvars
			Cvar_Set( "pl_selectedName", xbOfflineHistory.historyList[index].name );

			if(xbOfflineHistory.historyList[index].flags & MUTED_PLAYER)
				Cvar_Set("pl_selectedMute", "1");
			else
				Cvar_Set("pl_selectedMute", "0");

			// Only show the friend-related options if logged into live:
			if( logged_on )
			{
				if(xbOfflineHistory.historyList[index].friendshipStatus == FRIEND_RQST)
					Cvar_Set("pl_selectedFriend", "1");
				else if(xbOfflineHistory.historyList[index].friendshipStatus == FRIEND_YES)
					Cvar_Set("pl_selectedFriend", "2");
				else
					Cvar_Set("pl_selectedFriend", "0");
			}
			else
			{
				Cvar_Set("pl_selectedFriend", "-1");
			}

			// Can never kick someone that isn't playing
			Cvar_Set( "pl_allowKick", "0" );

			// Remember this index, (with the OFFSET!) so that if people join/leave,
			// we don't use the popup to do something to the wrong player!
			selectedIndex = playerIndex;
			// Remember the XUID and gamertag, so that we can many things even if the
			// list does change while we're in the popup
			selectedXuid = xbOfflineHistory.historyList[index].xuid;
			charToWchar( selectedGamertag, xbOfflineHistory.historyList[index].name );
			return true;
		}
	}

	//set to all 0 by default
	Cvar_Set("pl_selectedName", "");
	Cvar_Set("pl_selectedMute", "0");
	Cvar_Set("pl_selectedFriend", "0");
	Cvar_Set("pl_allowKick", "0");
	return false;
}


// Different from SOF2 version. Returns the name of the player at location
// `index' in the UI's player list. Handles player/history/divider.
const char *XBL_PL_GetPlayerName(const int index)
{
	int pIndex = XBL_PL_IndexToPlayerIndex( index );

	if(pIndex < 0)
	{
		// The divider - VVFIXME
		return SE_GetString( "MENUS_RECENT_PLAYERS" );
	}
	else if(pIndex < MAX_ONLINE_PLAYERS)
	{
		// It's an active player
		return xbOnlineInfo.xbPlayerList[pIndex].name;
	}
	else
	{
		// It's a history index
		return xbOfflineHistory.historyList[pIndex - HISTORY_OFFSET].name;
	}
}

// Gets the shader index for the voice icon to display (-1 for none)
int XBL_PL_GetVoiceIcon(const int index)
{
	int pIndex = XBL_PL_IndexToPlayerIndex( index );

	if(pIndex < 0)
	{
		// Divider
		return -1;
	}
	else if(pIndex < MAX_ONLINE_PLAYERS)
	{
		// Active player
		if(xbOnlineInfo.xbPlayerList[pIndex].flags & MUTED_PLAYER)
			return RE_RegisterShaderNoMip( "gfx/mp/voice_mute_icon" );
		else if(g_Voice.IsTalking(xbOnlineInfo.xbPlayerList[pIndex].xuid))
			return RE_RegisterShaderNoMip( XBL_SPEAKINGCYCLE_PL[speakCycleIndex] );
		else if(xbOnlineInfo.xbPlayerList[pIndex].flags & VOICE_CAN_SEND)
			return RE_RegisterShaderNoMip( "gfx/mp/voice_on_icon" );
//		else if(xbOnlineInfo.xbPlayerList[pIndex].flags & VOICE_CAN_RECV)
//			return RE_RegisterShaderNoMip( "gfx/mp/voice_speakers_icon" );
		else
			return -1;
	}
	else
	{
		// History player
		if(xbOfflineHistory.historyList[pIndex - HISTORY_OFFSET].flags & MUTED_PLAYER)
			return RE_RegisterShaderNoMip( "gfx/mp/voice_mute_icon" );
		// VVFIXME - Removed these. I don't think history players should have "current" voice status
//		else if(xbOfflineHistory.historyList[pIndex - HISTORY_OFFSET].flags & VOICE_ENABLED)
//			return XBL_ICONSASCIIVALUE_PL[XBL_PL_ICON_VOICEON];
		else
			return -1;
	}
}

// Gets the current status icon to display
int XBL_PL_GetStatusIcon(const int index)
{
	int pIndex = XBL_PL_IndexToPlayerIndex( index );

	if(pIndex < 0)
	{
		// Divider
		return -1;
	}
	else if(pIndex < MAX_ONLINE_PLAYERS)
	{
		// Active player
		if( xbOnlineInfo.xbPlayerList[pIndex].friendshipStatus == FRIEND_RQST_RCV)
			return RE_RegisterShaderNoMip( "gfx/mp/friend_received_icon" );
		else if( xbOnlineInfo.xbPlayerList[pIndex].friendshipStatus == FRIEND_RQST)
			return RE_RegisterShaderNoMip( "gfx/mp/friend_sent_icon" );
		else if( xbOnlineInfo.xbPlayerList[pIndex].friendshipStatus == FRIEND_YES)
			return RE_RegisterShaderNoMip( "gfx/mp/online_icon" );
		else
			return -1;
	}
	else
	{
		// History player
		if( xbOfflineHistory.historyList[pIndex - HISTORY_OFFSET].friendshipStatus == FRIEND_RQST)
			return RE_RegisterShaderNoMip( "gfx/mp/friend_sent_icon" );
		else
			return -1;
	}
}


// Quick functions for validating the stored playerIndex value before we do anything
static bool validStoredActiveIndex( void )
{
	return ( selectedIndex >= 0 &&
			 selectedIndex < MAX_ONLINE_PLAYERS &&
			 xbOnlineInfo.xbPlayerList[selectedIndex].isActive &&
			 XOnlineAreUsersIdentical( &xbOnlineInfo.xbPlayerList[selectedIndex].xuid, &selectedXuid ) );
}

static bool validStoredHistoryIndex( void )
{
	int hIndex = selectedIndex - HISTORY_OFFSET;
	return ( hIndex >= 0 &&
			 hIndex < MAX_OFFLINE_HISTORY &&
			 xbOfflineHistory.historyList[hIndex].isActive &&
			 XOnlineAreUsersIdentical( &xbOfflineHistory.historyList[hIndex].xuid, &selectedXuid ) );
}

// Toggle mute status for the player we stored in selectedIndex.
// VVFIXME - This could be more clever, and not depend on index still being valid.
void XBL_PL_ToggleMute( void )
{
	// Offset index, if this is a history player
	int hIndex = selectedIndex - HISTORY_OFFSET;

	if( validStoredActiveIndex() )
	{
		// Active player
		if( xbOnlineInfo.xbPlayerList[selectedIndex].flags & MUTED_PLAYER )
		{
			// Was muted. Un-mute them.
			g_Voice.SetMute(xbOnlineInfo.xbPlayerList[selectedIndex].xuid, false);
			xbOnlineInfo.xbPlayerList[selectedIndex].flags &= ~MUTED_PLAYER;
			Cvar_Set( "pl_selectedMute", "0" );
		}
		else
		{
			// Wasn't muted. Mute them.
			g_Voice.SetMute(xbOnlineInfo.xbPlayerList[selectedIndex].xuid, true);
			xbOnlineInfo.xbPlayerList[selectedIndex].flags |= MUTED_PLAYER;
			Cvar_Set( "pl_selectedMute", "1" );
		}
	}
	else if( validStoredHistoryIndex() )
	{
		if( xbOfflineHistory.historyList[hIndex].flags & MUTED_PLAYER )
		{
			// Was muted. Un-mute them.
			g_Voice.SetMute( xbOfflineHistory.historyList[hIndex].xuid, false );
			xbOfflineHistory.historyList[hIndex].flags &= ~MUTED_PLAYER;
			Cvar_Set( "pl_selectedMute", "0" );
		}
		else
		{
			// Wasn't muted. Mute them.
			g_Voice.SetMute( xbOfflineHistory.historyList[hIndex].xuid, true );
			xbOfflineHistory.historyList[hIndex].flags |= MUTED_PLAYER;
			Cvar_Set( "pl_selectedMute", "1" );
		}
	}
}

// Send feedback about the player we stored in selectedIndex.
// Feedback type is pulled from the cvar set by the UI.
void XBL_PL_SendFeedBack( void )
{
	// All necessary information is stored away, no vaildation needed
	XONLINETASK_HANDLE		feedbackTask;
	XONLINE_FEEDBACK_TYPE	feedback = (XONLINE_FEEDBACK_TYPE)Cvar_VariableIntegerValue("pl_feedbackType");
	XONLINE_FEEDBACK_PARAMS	feedbackParams;
	feedbackParams.lpStringParam = &selectedGamertag[0];

	HRESULT hr = XOnlineFeedbackSend( IN_GetMainController(), selectedXuid, feedback, &feedbackParams, NULL, &feedbackTask );
	if( hr != S_OK )
		return;

	do
	{
		hr = XOnlineTaskContinue( feedbackTask );
	} while( hr != XONLINETASK_S_SUCCESS );

	XOnlineTaskClose( feedbackTask );
}

// Kick a player via the player list (they've been stored in playerIndex already)
void XBL_PL_KickPlayer( void )
{
	// Sanity checks
	assert( com_sv_running->integer &&
			selectedIndex >= 0 &&
			selectedIndex < DEDICATED_SERVER_INDEX );

	// Make sure they're still around, and it's still the same person
	// This action will ALWAYS require validation
	if( !validStoredActiveIndex() )
		return;

	client_t *cl = &svs.clients[selectedIndex];

	SV_DropClient( cl, "@MENUS_YOU_WERE_KICKED" );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

// Send a friend request to a player via the player list
// (They've already been stored in playerIndex)
void XBL_PL_MakeFriend( void )
{
	// Selected user doesn't need to be valid anymore, we just need the XUID
	XBL_F_AddFriend( &selectedXuid );
	// Update, we've sent a request, this will change the button displayed
	Cvar_Set( "pl_selectedFriend", "1" );
}

// Cancel a friend request or remove a friend from our friend list, via the player list
// (They've already been stored in playerIndex)
void XBL_PL_CancelFriend( void )
{
	// Selected user doesn't need to be valid anymore, we just need the XUID
	XBL_F_RemoveFriend( &selectedXuid );
	// We've canceled, this will change button displayed
	Cvar_Set( "pl_selectedFriend", "0" );
}

// **********************************************************
// PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC *
// **********************************************************

void XBL_PL_Tick(void)
{
	if(playerListActive)
	{
		speakCycleDelay++;
		if(speakCycleDelay > SPEAK_CYCLE_DELAY)
		{
			speakCycleDelay = 0;
			speakCycleIndex++;
			if(speakCycleIndex >= MAX_SPEAK_CYCLE)
				speakCycleIndex = 0;
		}

		// Remaining code only applies if we're on Xbox Live
		if( !logged_on )
			return;

		if( ++tickCount < PLAYERSLIST_UPDATE_RATE )
			return;

		tickCount = 0; // reset

		// Scan through player list and update friendstatus
		for( int i = 0; i < MAX_ONLINE_PLAYERS; i++)
		{
			if(xbOnlineInfo.xbPlayerList[i].isActive)
				xbOnlineInfo.xbPlayerList[i].friendshipStatus = XBL_F_GetFriendStatus(&xbOnlineInfo.xbPlayerList[i].xuid);
		}
		// Scan through history list and update friendstatus
		for( int i = 0; i < MAX_OFFLINE_HISTORY; i++)
		{
			if(xbOfflineHistory.historyList[i].isActive)
				xbOfflineHistory.historyList[i].friendshipStatus = XBL_F_GetFriendStatus(&xbOfflineHistory.historyList[i].xuid);
		}
	}
}

//call when another client leaves the game
void XBL_PL_RemoveActivePeer(XUID* xuid, int index)
{
	// Don't re-add them if they're already in:
	int i;
	for(i = 0; i < MAX_OFFLINE_HISTORY; i++)
	{
		if(xbOfflineHistory.historyList[i].isActive && XOnlineAreUsersIdentical(xuid, &xbOfflineHistory.historyList[i].xuid))
			return;
	}

	// Search for inactive slot, or oldest active slot:
	int oldestIndex = 0;
	DWORD oldestStamp = xbOfflineHistory.historyList[0].stamp;
	for( i = 0; i < MAX_OFFLINE_HISTORY; ++i )
	{
		// Inactive slots win
		if( !xbOfflineHistory.historyList[i].isActive )
			break;

		// Older?
		if( xbOfflineHistory.historyList[i].stamp < oldestStamp )
		{
			oldestStamp = xbOfflineHistory.historyList[i].stamp;
			oldestIndex = i;
		}
	}

	// Grab the entry:
	XBHistoryEntry* entry;
	if( i == MAX_OFFLINE_HISTORY )
		entry = &xbOfflineHistory.historyList[oldestIndex];
	else
		entry = &xbOfflineHistory.historyList[i];

	memcpy(&entry->xuid, xuid, sizeof(XUID));
	if(index >= 0)
	{
		memcpy(entry->name, &xbOnlineInfo.xbPlayerList[index].name, sizeof(char) * XONLINE_GAMERTAG_SIZE);
		entry->friendshipStatus = xbOnlineInfo.xbPlayerList[index].friendshipStatus;
		entry->flags = xbOnlineInfo.xbPlayerList[index].flags;
	}
	entry->isActive = true;

	// Re-stamp this entry, advance timestamp:
	entry->stamp = xbOfflineHistory.stamp++;
}

void XBL_PL_CheckHistoryList(XBPlayerInfo *newInfo)
{
	if(!logged_on)
		return;
	for(int i = 0; i < MAX_OFFLINE_HISTORY; i++)
	{
		if(xbOfflineHistory.historyList[i].isActive && XOnlineAreUsersIdentical(&newInfo->xuid, &xbOfflineHistory.historyList[i].xuid))
		{
			xbOfflineHistory.historyList[i].isActive = false;
		}
	}
}

// Called when a user's userinfo changes, and their name needs to be updated:
void XBL_PL_UpdatePlayerName( int index, const char *newName )
{
	// People should never be able to change their names while logged into Live!
	// Fuck it. Making this work is impossible, given the number of inane name
	// changes that come from the server.

	// Just copy it over:
	Q_strncpyz( xbOnlineInfo.xbPlayerList[index].name, newName, sizeof(xbOnlineInfo.xbPlayerList[index].name) );
}
