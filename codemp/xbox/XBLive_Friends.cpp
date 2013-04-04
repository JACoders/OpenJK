// XBLIVE_FRIENDS.CPP
//
// wrapper to the Xbox online friends API
//

#include "..\xbox\XBLive.h"
#include "..\xbox\XBoxCommon.h"
#include "..\xbox\XBVoice.h"
#include "..\game\q_shared.h"
#include "..\qcommon\qcommon.h"
#include "..\cgame\cg_local.h"
#include "../ui/ui_shared.h"
#include "../qcommon/stringed_ingame.h"
#include "../qcommon/xb_settings.h"

extern int RE_RegisterShaderNoMip( const char *name );

//DEFINES

#define OPEN_SLOTS_JOIN_THRESHOLD	1		// the number of public slots needed to be open to make a player joinable (TCR recommends 3)
#define JOINABLE_CHECK_RATE			400		// 400 ticks (8ish seconds) between checks
#define FRIEND_DISPLAY_LIMIT		10		// only display a maximum of 10 friends at a time
#define MAX_TITLE_LENGTH			16		// allow 16 wide characters for a title's name
#define GAME_TITLE                  "Jedi Knight: Jedi Academy"

//VARIABLES

XONLINE_FRIEND		friendsList[MAX_FRIENDS];			// Our copy of the user's friends list
XONLINETASK_HANDLE	friendsGeneralTask;
XONLINETASK_HANDLE	enumerationTask;
bool				friendsInitialized	= false;
bool				generatingFriends	= false;		// flag stating that we are currently enumerating the friends list
BYTE                friendChosen        = 0;			// the friend selcted for action
int					numFriends			= 0;			// number of friends found while enumerating list
DWORD				current_state		= 0;			// flags depicting current state as perceived by friends

//PROTOTYPES
void		XBL_F_GetTitleString(const DWORD titleID, char* titleString);
HRESULT		XBL_F_Invite(void);
void		XBL_F_JoinFriendUninvited( XONLINE_FRIEND* friendPlaying);

// ******************************************************
// INITIALIZING FUNCTIONS
// *******************************************************

// Initialize the friends functionality
//
void XBL_F_Init()
{
	if( friendsInitialized )
		return;

	// We have no friends (yet)
	numFriends = 0;

	// system startup
	//
	HRESULT result = XOnlineFriendsStartup(	NULL, &friendsGeneralTask );
	if( result != S_OK )
	{
		Com_Printf("XBLive_F - Error %d in startup\n", result);
		friendsGeneralTask = NULL;
		return;
	}

	// Default state after logging in
	XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_ONLINE,		!Settings.appearOffline);
	XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_PLAYING,	false);
	XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_VOICE,		g_Voice.IsVoiceAllowed());
	XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_JOINABLE,	false);

	friendsInitialized = true;
}

// generate the list of friends
//
void XBL_F_GenerateFriendsList()
{
	if(!logged_on || generatingFriends)
		return;

	// generate the list of records
	//
	HRESULT result = XOnlineFriendsEnumerate(  IN_GetMainController(), NULL, &enumerationTask );
	if( result != S_OK )
	{
		Com_Printf("XBLive_F - Error %d using XOnlineFriendsEnumerate\n", result);
		enumerationTask = NULL;
		return;
	}

	// Reset friends count, set flag to retrieve friends during tick
	numFriends = 0;
	generatingFriends	= true;
}

// ******************************************************
// CLEANUP FUNCTIONS
// *******************************************************

// stop generating the friends list
//
void XBL_F_ReleaseFriendsList()
{
	if( !generatingFriends )
		return;

	// wait for enumeration to complete
	//
	if(enumerationTask)
	{
		HRESULT result = XOnlineFriendsEnumerateFinish( enumerationTask );
		if( IS_ERROR(result) )
		{
			Com_Printf("XBLive_F - Error %d using XOnlineFriendsEnumerateFinish\n", result);
		}

		// finish any related tasks
		//
		do
		{
			result = XOnlineTaskContinue( enumerationTask );
			if( IS_ERROR(result) )
			{
				Com_Printf("XBLive_F - Error %d enumerating friends list\n", result);
			}
		}
		while( result != XONLINETASK_S_SUCCESS );

		XOnlineTaskClose(enumerationTask);
		enumerationTask = NULL;
	}

	// List isn't valid anymore, so no friends, and tell tick to stop generating
	generatingFriends	= false;
	numFriends = 0;
}

// shutdown and cleanup friends functionality
//
void XBL_F_Cleanup()
{
	HRESULT result;

    XBL_F_ReleaseFriendsList();

    // finish off incomplete tasks
	//
	if(friendsGeneralTask)
	{
		do 
		{
			result = XOnlineTaskContinue( friendsGeneralTask );
			if( IS_ERROR(result) )
			{
				Com_Printf("XBLive_F - Error %d finishing off friends general tasks\n", result);
				XOnlineTaskClose(friendsGeneralTask);
				friendsGeneralTask = NULL;
				break;
			}
		}
		while( result != XONLINETASK_S_RUNNING_IDLE );
		XOnlineTaskClose(friendsGeneralTask);
		friendsGeneralTask = NULL;
	}
    friendsInitialized	= false;
}

// ******************************************************
// FUNCTIONS THE MENU SYSTEM USES TO COMMUNICATE TO FRIENDS
// *******************************************************

//Return the number of friends found while enumerating list
int	XBL_F_GetNumFriends(void)
{
	return numFriends;
}

// Set the higlighted friend for use when friend is selected
void XBL_F_SetChosenFriendIndex(const int index)
{
	// Sanity check - this does get called before we're done enumerating some times!
	assert( (index >= 0 && index < numFriends) || !numFriends );

	// SOF2 had checks for less than zero - probably forgot to do a
	// Menu_SetFeederSelection before opening the menu.
	friendChosen = index;

	// The UI calls this when the selection in the listbox moves. Set the status lines:
	Cvar_Set( "fl_voiceLine", XBL_F_GetVoiceString( index ) );
	Cvar_Set( "fl_statusLine", XBL_F_GetStatusString( index ) );
	Cvar_Set( "fl_statusLine2", XBL_F_GetStatusString2( index ) );
}

XONLINE_FRIEND *XBL_F_GetChosenFriend( void )
{
	if( friendChosen >= 0 && friendChosen < numFriends )
		return &friendsList[friendChosen];
	return NULL;
}

// Handle a players attempt to join a friends game from an invite when they are potentially using another title
//
HRESULT XBL_F_JoinGameFromInvite()
{
    HRESULT result;

    result = XOnlineFriendsAnswerGameInvite( IN_GetMainController(), &friendsList[friendChosen], XONLINE_GAMEINVITE_YES); // writes invite to HD if needed
    if( result != S_OK )
    {
        assert(!"faliure to accept invite");
        // report error
    }

    // figure out if you're playing the same game as your friend
    //
    BOOL playingSameGame = XOnlineTitleIdIsSameTitle( friendsList[friendChosen].dwTitleID );

    // if you are
    //
    if( playingSameGame )
    {
	    XBL_MM_ConnectViaSessionID( &friendsList[friendChosen].sessionID, true );
        return S_OK;
    }

    // else if its a different game return the code to show popup
    //
    return -1;
}

// Handle a players attempt to join a friends game when they are potentially using another title
HRESULT XBL_F_JoinGame()
{
	// Figure out if you're playing the same game as your friend
	BOOL playingSameGame = XOnlineTitleIdIsSameTitle( friendsList[friendChosen].dwTitleID );

	// If you are
	if( playingSameGame )
	{
		XBL_MM_ConnectViaSessionID( &friendsList[friendChosen].sessionID, false );
		return S_OK;
	}

	// It's a different game, write out the info:
	HRESULT result = XOnlineFriendsJoinGame( IN_GetMainController(), &friendsList[friendChosen] );
	assert( result == S_OK );

	// Show insert disk popup
	return -1;
}

// This handle all possbile actions that the user can perform on a selected friend
//
HRESULT XBL_F_PerformMenuAction(friendChoices_t action)
{
	HRESULT result = S_OK;

    switch(action)
	{
	case UI_F_FRIENDREQUESTED:
		result = XOnlineFriendsRequest(IN_GetMainController(), friendsList[friendChosen].xuid);
		break;

	case UI_F_FRIENDACCEPTED:
		result = XOnlineFriendsAnswerRequest(IN_GetMainController(), &friendsList[friendChosen], XONLINE_REQUEST_YES);
		break;

	case UI_F_FRIENDREMOVE:
		    result = XOnlineFriendsRemove(IN_GetMainController(), &friendsList[friendChosen]);
		    break;

	case UI_F_FRIENDDECLINE:
		result = XOnlineFriendsAnswerRequest(IN_GetMainController(), &friendsList[friendChosen], XONLINE_REQUEST_NO);
		break;

	case UI_F_FRIENDBLOCK:
		result = XOnlineFriendsAnswerRequest(IN_GetMainController(), &friendsList[friendChosen], XONLINE_REQUEST_BLOCK);
		break;

	case UI_F_FRIENDCANCEL:
		result = XOnlineFriendsRemove(IN_GetMainController(), &friendsList[friendChosen]);
		break;

	case UI_F_GAMEREQUESTED:
		result = XBL_F_Invite();
		break;

	case UI_F_GAMEACCEPTED:
        result = XBL_F_JoinGameFromInvite();
		break;

	case UI_F_GAMEDECLINE:
		result = XOnlineFriendsAnswerGameInvite(IN_GetMainController(), &friendsList[friendChosen], XONLINE_GAMEINVITE_NO);
		break;

	case UI_F_GAMEFRIENDREMOVED:
		result = XOnlineFriendsAnswerGameInvite(IN_GetMainController(), &friendsList[friendChosen], XONLINE_GAMEINVITE_REMOVE);
		break;

	case UI_F_GAMECANCEL:
		result = XOnlineFriendsRevokeGameInvite(IN_GetMainController(), *Net_GetXNKID(), 1, &friendsList[friendChosen]);
		break;

	case UI_F_JOINSESSION:
        result = XBL_F_JoinGame();
		break;

	case UI_F_TOGGLEONLINE:
		if(current_state & XONLINE_FRIENDSTATE_FLAG_ONLINE)
			XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_ONLINE, false);
		else
			XBL_F_SetState(XONLINE_FRIENDSTATE_FLAG_ONLINE, true);
		break;
	}
	return result;
}


// Returns the gamertag of the given friend - used by UI listbox renderer
const char *XBL_F_GetFriendName( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return "";

	return friendsList[index].szGamertag;
}

// Returns the voice icon to use in the friends list for the given friend
int XBL_F_GetVoiceIcon( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return -1;

	// Friend has no voice?
	if( !(friendsList[index].dwFriendState & XONLINE_FRIENDSTATE_FLAG_VOICE) )
		return -1;

	// They have voice. Just check if they're muted.
	if( g_Voice.IsMuted( friendsList[index].xuid ) )
		return RE_RegisterShaderNoMip( "gfx/mp/voice_mute_icon" );
	else
		return RE_RegisterShaderNoMip( "gfx/mp/voice_on_icon" );
}

// Returns the full voice info status line for the given friend
const char *XBL_F_GetVoiceString( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return "";

	// Never display voice info for an offline friend
	if( !(friendsList[index].dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE) )
		return "";

	// Friend has no voice?
	if( !(friendsList[index].dwFriendState & XONLINE_FRIENDSTATE_FLAG_VOICE) )
		return va( "%s %s", SE_GetString( "MENUS_VOICE" ), SE_GetString( "MENUS_OFF" ) );

	// OK. Friend has voice. Just check if they're muted.
	if( g_Voice.IsMuted( friendsList[index].xuid ) )
		return va( "%s %s", SE_GetString( "MENUS_VOICE" ), SE_GetString( "MENUS_MUTED" ) );
	else
		return va( "%s %s", SE_GetString( "MENUS_VOICE" ), SE_GetString( "MENUS_ON" ) );
}

// Returns the status icon to use in the friends list for the given friend
int XBL_F_GetStatusIcon( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return -1;

	// We'll be using this a lot...
	const XONLINE_FRIEND *curFriend = &friendsList[index];

	// In order of importance

	// Player has received a request to a game
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE )
		return RE_RegisterShaderNoMip( "gfx/mp/game_received_icon" );

	// Player has received a request to be a friend
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST )
		return RE_RegisterShaderNoMip( "gfx/mp/friend_received_icon" );

	// Player has sent invite to game
	if( (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTINVITE) &&
		!(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED) && 
		!(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED) )
		return RE_RegisterShaderNoMip( "gfx/mp/game_sent_icon" );

	// Player has sent friend request
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST )
		return RE_RegisterShaderNoMip( "gfx/mp/friend_sent_icon" );

	// Friend is online, or online and playing
	if( (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING) ||
		(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE) )
		return RE_RegisterShaderNoMip( "gfx/mp/online_icon" );

	// Friend is offline
	return -1;
}

// Returns the online/offline status line for the given friend
const char *XBL_F_GetStatusString( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return "";

	// Some things we'lll be reusing a bit:
	const XONLINE_FRIEND *curFriend = &friendsList[index];
	char titleString[MAX_TITLENAME_LEN+1];
	XBL_F_GetTitleString( curFriend->dwTitleID, titleString );

	// In order of importance

	// Friend is currently playing some game
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING )
		return va( SE_GetString( "MENUS_PLAYING" ), titleString );

	// Friend is online - not in a session
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE )
		return va( SE_GetString( "MENUS_AVAILABLE_IN" ), titleString );

	// Friend is offline
	return SE_GetString( "MENUS_OFFLINE" );
}

// Returns the special case status info for the given friend
const char *XBL_F_GetStatusString2( const int index )
{
	// Invalid index?
	if( index < 0 || index >= numFriends )
		return "";

	// Some things we'lll be reusing a bit:
	const XONLINE_FRIEND *curFriend = &friendsList[index];
	char titleString[MAX_TITLENAME_LEN+1];
	XBL_F_GetTitleString( curFriend->dwTitleID, titleString );

	// In order of importance

	// Player has received a game invite fom the indicated friend
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE )
		return va( SE_GetString( "MENUS_WANTS_TO_PLAY" ), titleString );	// Cleared ui_gameInvite (now xbl_hudGame)

	// Player has received a request to be a friend
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST )
		return SE_GetString( "MENUS_WANTS_TO_BE_FRIEND" );	// Cleared ui_friendInvite

	// Player has sent invite to game
	if( (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTINVITE) &&
		!(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED) &&
		!(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED) )
		return SE_GetString( "MENUS_INVITED_TO_PLAY" );

	// Player has sent friend request
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST )
		return SE_GetString( "MENUS_YOU_ASKED_TO_BE_FRIEND" );

	// No special state:
	return "";
}

// Determines the friend status of a given user. Used by the player list
// code to set the state of each user, which is then used to determine icons.
FriendState XBL_F_GetFriendStatus( const XUID *xuid )
{
	XONLINE_FRIEND *curFriend = NULL;
	for( int i = 0; i < numFriends; i++ )
	{
		if( XOnlineAreUsersIdentical( &friendsList[i].xuid, xuid ) )
		{
			curFriend = &friendsList[i];
			break;
		}
	}

	if( !curFriend )
		return FRIEND_NO;

	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST )
	{
		return FRIEND_RQST;
	}

	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST )
	{
		return FRIEND_RQST_RCV;
	}

	return FRIEND_YES;
}

// Send a friend invitation to this user.
void XBL_F_AddFriend( const XUID *pXuid )
{
	// Send the request to the servers - VVFIXME - handle errors here?
	HRESULT hr = XOnlineFriendsRequest( IN_GetMainController(), *pXuid );
}

// Cancel a friend invitation we may have sent to this user, and/or
// remove them from our friend list.
void XBL_F_RemoveFriend( const XUID *pXuid )
{
	XONLINE_FRIEND *curFriend = NULL;
	for( int i = 0; i < numFriends; i++ )
	{
		if( XOnlineAreUsersIdentical( &friendsList[i].xuid, pXuid ) )
		{
			curFriend = &friendsList[i];
			break;
		}
	}

	// Make sure we found them in our list
	if( !curFriend )
		return;

	// OK. Remove them from the list
	XOnlineFriendsRemove( IN_GetMainController(), curFriend );
}

// ******************************************************
// SUPPORT FUNCTIONS FOR MANAGING FRIENDS
// *******************************************************

// ******************************************************
// PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC ** PUBLIC
// *******************************************************
// functionality to be run every game tick
//
void XBL_F_Tick()
{
	if( !friendsInitialized )
		return;

	HRESULT result;

	// Check if we are in a session with enough open public slots to make us joinable
	XBL_F_CheckJoinableStatus( false );

	// Process the general task, required during gameplay to handle invitations
	if( friendsGeneralTask )
	{
		result = XOnlineTaskContinue( friendsGeneralTask );
		if( IS_ERROR(result) )
		{
			Com_Printf("XBLive_F - Error %d running friends general tasks\n", result);
			XOnlineTaskClose(friendsGeneralTask);
			friendsGeneralTask = NULL;
		}
	}

	// If the UI is currently displaying the friends list we must continue to update it
	if( generatingFriends && enumerationTask )
	{
		// Keep enumerating
		result = XOnlineTaskContinue( enumerationTask );
		if( IS_ERROR(result) )
		{
			Com_Printf("XBLive_F - Error %d enumerating friends list\n", result);
		}

		// Do we have all the results?
		if( result == XONLINETASK_S_RESULTS_AVAIL )
		{
			// Copy the list into our buffer
			numFriends = XOnlineFriendsGetLatest(IN_GetMainController(), MAX_FRIENDS, &friendsList[0] );

			// Also update the strings about the currently selected friend:
			if( friendChosen >= 0 && friendChosen < numFriends )
			{
				Cvar_Set( "fl_voiceLine", XBL_F_GetVoiceString( friendChosen ) );
				Cvar_Set( "fl_statusLine", XBL_F_GetStatusString( friendChosen ) );
				Cvar_Set( "fl_statusLine2", XBL_F_GetStatusString2( friendChosen ) );
			}
		}
	}
}

// Change our state - set or remove a notification flag
void XBL_F_SetState( DWORD flag, bool set_flag )
{
	if( set_flag )
		current_state |= flag;
	else
		current_state &= ~flag;

	HRESULT hr =  XOnlineNotificationSetState(IN_GetMainController(), current_state, *Net_GetXNKID(), 0, NULL );
	assert( hr == S_OK );

	// VVFIXME - Move this shite into the calling code
	//jsw//Hack for the menus
	if(flag == XONLINE_FRIENDSTATE_FLAG_ONLINE)
	{
		if(set_flag)
			Cvar_Set( "ui_xblivefriendonline", SE_GetString("XBL_ONLINE") );
		else
			Cvar_Set( "ui_xblivefriendonline", SE_GetString("XBL_OFFLINE") );
	}
}

// Just a way to get our own state information (things that are in our friend state)
bool XBL_F_GetState( DWORD flag )
{
	return current_state & flag;
}

// Invite a friend to join the current game
HRESULT XBL_F_Invite(void)
{
	return XOnlineFriendsGameInvite( IN_GetMainController(), *Net_GetXNKID(), 1, &friendsList[friendChosen] );
}

//	we need to change our client friend states to show that we're not in a session
//
void XBL_F_OnClientLeaveSession()
{
	XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_JOINABLE, false );	
	XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_PLAYING, false );
}

bool XBL_F_FriendNotice( void )
{
	return (logged_on && XOnlineGetNotification( IN_GetMainController(), XONLINE_NOTIFICATION_FRIEND_REQUEST ));
}

bool XBL_F_GameNotice( void )
{
	return (logged_on && XOnlineGetNotification( IN_GetMainController(), XONLINE_NOTIFICATION_GAME_INVITE ));
}

// Get friend details based on gamertag
//
XONLINE_FRIEND* XBL_F_GetFriendFromName( char* name )
{
    for( int i = 0; i < numFriends; i++ )
    {
        if( !strcmp( friendsList[i].szGamertag, name ) )
        {
            return &friendsList[i];
        }
    }

	return NULL;
}


// ******************************************************
// PRIVATE ** PRIVATE ** PRIVATE ** PRIVATE ** PRIVATE ** PRIVATE
// *******************************************************

//Get the title string of game being played
void XBL_F_GetTitleString(const DWORD titleID, char* titleString)
{
	// Is their title ID valid?
	if( !titleID )
	{
		strcpy( titleString, "" );
		return;
	}

	WCHAR	playingWstr[MAX_TITLENAME_LEN+1];
	int		attempt = 0;

	// MS suggests polling to get the string:
	do
	{
		attempt++;
		XOnlineFriendsGetTitleName( titleID, XC_LANGUAGE_ENGLISH, MAX_TITLE_LENGTH, playingWstr );
	} while( playingWstr[0] == 0 && attempt < 10000 );

	// If we got something, use it, else just print the title ID
	if( attempt < 10000 )
		wcharToChar( titleString, playingWstr );
	else
		sprintf( titleString, "%d", titleID );
}

// periodically update your joinability status (your session has X+ public slots open)
//
void XBL_F_CheckJoinableStatus( bool force )
{
	// if not currently in a session, leave
	//
	if( !(current_state & XONLINE_FRIENDSTATE_FLAG_PLAYING) )
		return;	

	static int pass = 0;

	// if its time to check...
	//
	if(	++pass > JOINABLE_CHECK_RATE || force )
	{
		// Just count how many clients we have valid info for, and compare to maxclients!
		// Need a special case for dedicated host though:
		extern cvar_t *sv_maxclients;
		int numClients = 0;
		int maxClients = com_dedicated->integer ? sv_maxclients->integer : cgs.maxclients;
		for( int i = 0; i < maxClients; ++i )
			if( xbOnlineInfo.xbPlayerList[i].isActive )
				numClients++;

		// Set our joinable state based on the number of empty slots.
		XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_JOINABLE, numClients < maxClients );

		pass = 0;
    }	
}
