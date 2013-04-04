// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
//#define PRE_RELEASE_TADEMO

#include "../ghoul2/G2.h"
#include "ui_local.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/game_version.h"
#include "ui_force.h"
#include "../cgame/animtable.h" //we want this to be compiled into the module because we access it in the shared module.
#include "../game/bg_saga.h"

#include "..\cgame\holocronicons.h"

#ifdef _XBOX
#include "../qcommon/qcommon.h"
#include "..\xbox\xboxcommon.h"
#include "../qcommon/xb_settings.h"
#endif

extern void IN_DisplayControllerUnplugged( int controller );

extern void UI_SaberAttachToChar( itemDef_t *item );
extern const char *SE_GetString( const char *psPackageAndStringReference );

char *forcepowerDesc[NUM_FORCE_POWERS] = 
{
"@MENUS_OF_EFFECT_JEDI_ONLY_NEFFECT",
"@MENUS_DURATION_IMMEDIATE_NAREA",
"@MENUS_DURATION_5_SECONDS_NAREA",
"@MENUS_DURATION_INSTANTANEOUS",
"@MENUS_INSTANTANEOUS_EFFECT_NAREA",
"@MENUS_DURATION_VARIABLE_20",
"@MENUS_DURATION_INSTANTANEOUS_NAREA",
"@MENUS_OF_EFFECT_LIVING_PERSONS",
"@MENUS_DURATION_VARIABLE_10",
"@MENUS_DURATION_VARIABLE_NAREA",
"@MENUS_DURATION_CONTINUOUS_NAREA",
"@MENUS_OF_EFFECT_JEDI_ALLIES_NEFFECT",
"@MENUS_EFFECT_JEDI_ALLIES_NEFFECT",
"@MENUS_VARIABLE_NAREA_OF_EFFECT",
"@MENUS_EFFECT_NAREA_OF_EFFECT",
"@SP_INGAME_FORCE_SABER_OFFENSE_DESC",
"@SP_INGAME_FORCE_SABER_DEFENSE_DESC",
"@SP_INGAME_FORCE_SABER_THROW_DESC"
};

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#include "../renderer/modelmem.h"

#include "../xbox/XBLive.h"
#include "../xbox/XBoxCommon.h"
#include "../xbox/XBVoice.h"
#include <xonline.h>
extern XONLINE_ACCEPTED_GAMEINVITE *Sys_AcceptedInvite( void );

// Used to check that only the client that started the UI can control things
int uiClientNum = 0;
int uiclientInputClosed =0;
int storedClientInputClosed =0;
qboolean uiControllerMenu = qfalse;
int unpluggedcontrol2= -1;


#define PAUSE_DELAY  5
int gDelayedPause = 0;

//JLF 
int gScrollAccum = 0;
int gScrollDelta = 0;

#define ARROW_SPACE 8

char uglyMug1 = 0;
char uglyMug2 = 0;

#define TEXTSCROLLDESCRETESTEP 50

void UpdateNextBotSlot( void );
void UpdatePrevBotSlot( void );

#define LISTBUFSIZE 10240
#define filepathlength 120

struct playerProfile_t
{
	char	listBuf[LISTBUFSIZE];			//	The list of file names read in

	// For scrolling through file names 
	int				currentLine;		//	Index to currentSaveFileComments[] currently highlighted
	int				fileCnt;		//	Number of save files read in
	char			*modelName;
};

playerProfile_t s_playerProfile;

void resetProfileFileCount()
{
	s_playerProfile.fileCnt = -1;

};

#define MAX_PROFILEFILES	8
#define COMMENTSIZE 64

typedef struct 
{
	char currentProfileName[COMMENTSIZE];						// file name of savegame
	char currentProfileComments[COMMENTSIZE];	// file comment
	char currentProfileDateTimeString[COMMENTSIZE];	// file time and date
	time_t currentProfileDateTime;
} profileData_t;

static profileData_t s_ProfileData[MAX_PROFILEFILES];

void ReadSaveDirectoryProfiles(void);


int UI_GetMaxForceRank(void)
{
	char info[MAX_INFO_VALUE];
	info[0] = 0;
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
	return atoi(Info_ValueForKey(info, "g_maxForceRank"));
}

#ifdef _XBOX
// VIRTUAL KEYBOARD DEFINES ETC
//
// Warning: These next values must work out so that there are at least 2 columns and/or
// 2 rows. Otherwise you will not be able to compile because of divide by zero errors.
// Not to mention the ugly keyboard you'd be designing

#define SKB_NUM_LETTERS  (36)
#define SKB_NUM_COLS  (10) // must be > 1 and < SKB_NUM_LETTERS-1
#define SKB_NUM_ROWS  ((SKB_NUM_LETTERS%SKB_NUM_COLS)?(SKB_NUM_LETTERS/SKB_NUM_COLS+1):(SKB_NUM_LETTERS/SKB_NUM_COLS))
#define SKB_TOP  (225)
#define SKB_BOT  (350)
#define SKB_LEFT (100)
#define SKB_RIGHT  (540)
#define SKB_STRING_LENGTH (10)
#define SKB_STRING_TOP (150)
#define SKB_STRING_LEFT (200)
#define SKB_SPACE_H ((SKB_RIGHT-SKB_LEFT)/(SKB_NUM_COLS-1))
#define SKB_SPACE_V ((SKB_BOT-SKB_TOP)/(SKB_NUM_ROWS-1))
#define SKB_ACCEPT_NAME ("skb_accept")
#define SKB_DELETE_NAME ("skb_delete")
#define SKB_KEYBOARD_NAME ("skb_keyboard")
#define SKB_OK_X (390)
#define SKB_OK_Y (400)
#define SKB_BACKSPACE_X (250)
#define SKB_BACKSPACE_Y (400)

#define SKB_PULSE_LARGE (2.80f)
#define SKB_PULSE_SMALL (2.25f)
#define SKB_PULSE_SPEED (0.025f)

char *letters[SKB_NUM_LETTERS] = {
			"0", "1", "2", "3",
			"4", "5", "6", "7",
			"8", "9",		
			"A", "B", "C", "D",
			"E", "F", "G", "H",
			"I", "J", "K", "L",
			"M", "N", "O", "P",
			"Q", "R", "S", "T",
			"U", "V", "W", "X",
			"Y", "Z",	};

typedef struct
{
	short activeKey;
	short curStringPos;
	short curCol;
	short curRow;
	float pulse_size;
	bool pulse_up;
} softkeyboardDef_t;

softkeyboardDef_t skb;

static qboolean	UI_SoftKeyboard_HandleKey(int flags, float *special, int key);
static qboolean	UI_SoftKeyboardDelete_HandleKey(int flags, float *special, int key);
static qboolean	UI_SoftKeyboardAccept_HandleKey(int flags, float *special, int key);
static void		UI_SoftKeyboardInit();
static void		UI_SoftKeyboardDelete();
static void		UI_SoftKeyboardAccept();
static void		UI_SoftKeyboard_Draw();
static void		UI_SoftKeyboardDelete_Draw();
static void		UI_SoftKeyboardAccept_Draw();
static void		UI_DrawVoteDesc();
static void		UI_DrawPlayerKickDesc();
static void UI_DrawInvisibleVoteListener();
static int UI_MapCountByCurrentGameType();
static void UI_UpdateMoves( void );
static const char *UI_SelectedTeamHead(int index, int *actual);

#endif //_XBOX


/*
===============
UI_XB_BotScript
Handle all UI script calls for our fake scrolling bot list screen
===============
*/
#include "../namespace_begin.h"
extern void Menu_SetItemText(const menuDef_t *menu,const char *itemName, const char *text);
extern void Menu_SetItemBackground(const menuDef_t *menu, const char *itemName, const char *background);
qboolean Item_SetFocus(itemDef_t *item, float x, float y);
#include "../namespace_end.h"

// Dirty little function to get the number of clients that are GOING to
// be in the game, while we're in the UI - used for bot config and such
int UI_NumClients( void )
{
	return trap_Cvar_VariableValue("ui_dedicated") ? 0 : ClientManager::NumClients();
}

void UI_XB_BotScript(char **args, const char *name)
{
	// Currently visible settings, before commiting back to cvars
	static int botValues[16];
	static int numBlue, numRed;
	static int visibleRows, curOffset;
	static char labels[4][4];	// Labels for each item
	static bool teamGame;

	if ( !String_Parse(args, &name) || !name )
	{
		return;
	}

	else if(Q_stricmp(name, "Init") == 0)
	{
		// How many bots of each color?
		numBlue = trap_Cvar_VariableValue( "ui_numBlueBots" );
		numRed = trap_Cvar_VariableValue( "ui_numRedBots" );

		// Is this a team game?
		int gameType = trap_Cvar_VariableValue( "ui_netGameType" );
		teamGame = (gameType == 2) || (gameType == 3) || (gameType == 5);

		// Fetch all the current ui_blueteam and ui_redteam cvars:
		int i;
		for(i = 0; i < numBlue; ++i)
		{
			int bot = trap_Cvar_VariableValue( va("ui_blueteam%i", i+1) );
			if (bot > 1)
				botValues[i] = bot - 2;
			else
				botValues[i] = 0;
		}
		for(i = 0; i < numRed; ++i)
		{
			int bot = trap_Cvar_VariableValue( va("ui_redteam%i", i+1) );
			if (bot > 1)
				botValues[numBlue+i] = bot - 2;
			else
				botValues[numBlue+i] = 0;
		}

		// How many rows of scrolling joy do we need?
		visibleRows = (numBlue+numRed < 4) ? numBlue+numRed : 4;
		trap_Cvar_SetValue( "xb_botRows", visibleRows );

		// For every visible row, set the label, current value, and portrait:
		menuDef_t *menu = Menu_GetFocused();

		for(i = 0; i < visibleRows; ++i)
		{
			// Label
			Com_sprintf( labels[i], sizeof(labels[0]), "%i:", i+1 );
			Menu_SetItemText(menu, va("bot%i",i), labels[i]);
			// Value
			trap_Cvar_SetValue( va("ui_bot%i", i), botValues[i] );
		}

		// curOffset is the index (in botValues) of the first visible row
		curOffset = 0;
	}
	else if(Q_stricmp(name, "Confirm") == 0)
	{
		int i;

		// First, grab all the changes from the currently visible controls
		for(i = 0; i < visibleRows; ++i)
			botValues[curOffset+i] = trap_Cvar_VariableValue( va("ui_bot%i",i) );

		// Now, commit all botValues entries to the ui_blueteam and ui_redteam cvars
		for(i = 0; i < numBlue; ++i)
			trap_Cvar_SetValue( va("ui_blueteam%i",i+1), botValues[i]+2 );
		for(i = 0; i < numRed; ++i)
			trap_Cvar_SetValue( va("ui_redteam%i",i+1), botValues[numBlue+i]+2 );
	}

	else if(Q_stricmp(name, "ScrollUp") == 0)
	{
		menuDef_t *menu = Menu_GetFocused();

		// We fell off the top of the multi-list, go to the bottom of the list:
		if (curOffset == 0)
		{
			goto jumptobottom;	// I'm sorry. Really. - BTO
		}

		// Copy changes to last entry into botValues. This can only happen on the fourth row
		botValues[curOffset+3] = trap_Cvar_VariableValue( "ui_bot3" );

		// Propagate changes down
		int i;
		for(i = 3; i >= 1; --i)
		{
			// Set new label
			Com_sprintf( labels[i], sizeof(labels[0]), "%i:", curOffset+i );
			// Copy value
			trap_Cvar_SetValue( va("ui_bot%i",i), trap_Cvar_VariableValue( va("ui_bot%i",i-1) ) );
		}

		// Fill in new first item
		Com_sprintf( labels[0], sizeof(labels[0]), "%i:", curOffset );
		trap_Cvar_SetValue( "ui_bot0", botValues[curOffset-1] );

		// Adjust our offset
		curOffset--;

		// Put focus back on the first item
		Item_SetFocus(Menu_FindItemByName( menu, "bot0" ), 0, 0);
	}
	else if(Q_stricmp(name, "ScrollDown") == 0)
	{
		menuDef_t *menu = Menu_GetFocused();

		// We fell off the bottom of the multi-list, go to the top of the list:
		if (curOffset+4 >= numRed+numBlue)
		{
			goto jumptotop;	// I swear! I'll never do it again! - BTO
		}

		// Copy changes to first entry into botValues. This can only happen on the first row
		botValues[curOffset] = trap_Cvar_VariableValue( "ui_bot0" );

		// Propagate changes up
		int i;
		for(i = 0; i <= 2; ++i)
		{
			// Set new label
			Com_sprintf( labels[i], sizeof(labels[0]), "%i:", curOffset+i+2 );
			// Copy value
			trap_Cvar_SetValue( va("ui_bot%i",i), trap_Cvar_VariableValue( va("ui_bot%i",i+1) ) );
		}

		// Fill in new last item
		Com_sprintf( labels[3], sizeof(labels[0]), "%i:", curOffset+5 );
		trap_Cvar_SetValue( "ui_bot3", botValues[curOffset+4] );

		// Adjust our offset
		curOffset++;

		// Put focus back on the last item
		Item_SetFocus(Menu_FindItemByName( menu, "bot3" ), 0, 0);
	}
	else if(Q_stricmp(name, "JumpToTop") == 0)
	{
jumptotop:
		menuDef_t *menu = Menu_GetFocused();
		int i;

		for(i = 0; i < visibleRows; ++i)
		{
			// Save off old value
			botValues[curOffset+i] = trap_Cvar_VariableValue( va("ui_bot%i",i) );

			// Update label
			Com_sprintf( labels[i], sizeof(labels[0]), "%i:", i+1 );
			// Update value
			trap_Cvar_SetValue( va("ui_bot%i", i), botValues[i] );
		}

		// Reset curOffset
		curOffset = 0;

		// Jump to the first item
		Item_SetFocus(Menu_FindItemByName( menu, "bot0" ), 0, 0);
	}
	else if(Q_stricmp(name, "JumpToBottom") == 0)
	{
jumptobottom:
		menuDef_t *menu = Menu_GetFocused();
		int i;
		int newOffset = (numBlue + numRed) - visibleRows;

		for(i = 0; i < visibleRows; ++i)
		{
			// Save off old value
			botValues[curOffset+i] = trap_Cvar_VariableValue( va("ui_bot%i",i) );

			// Update label
			Com_sprintf( labels[i], sizeof(labels[0]), "%i:", newOffset+i+1 );
			// Update value
			trap_Cvar_SetValue( va("ui_bot%i", i), botValues[newOffset+i] );
		}

		// Adjust curOffset
		curOffset = newOffset;

		// Jump to the last visible item
		Item_SetFocus(Menu_FindItemByName( menu, va("bot%i",visibleRows-1) ), 0, 0);
	}
	else if(Q_stricmp(name, "UpdateImages") == 0)
	{
		// This gets called by the action scripts of the multis, so commit to botValues:
		int i;
		for(i = 0; i < visibleRows; ++i)
			botValues[curOffset+i] = trap_Cvar_VariableValue( va("ui_bot%i",i) );

		// Make sure that all img# fields are set correctly
		menuDef_t *menu = Menu_GetFocused();
		for(i = 0; i < visibleRows; ++i)
		{
			// Update image
			const char *botModel = Info_ValueForKey( UI_GetBotInfoByNumber( botValues[curOffset+i] ), "model" );
			const char *teamStr;
			if (!teamGame)
				teamStr = "default";
			else if (curOffset+i < numBlue)
				teamStr = "blue";
			else
				teamStr = "red";
			Menu_SetItemBackground(menu, va("img%i",i), va("models/players/%s/icon_%s", botModel, teamStr) );
		}
	}
	else if(Q_stricmp(name, "checkBlue") == 0)
	{
		// This script is NOT run while the other data in this function is valid!
		int red = trap_Cvar_VariableValue("ui_numRedBots");
		int blue = trap_Cvar_VariableValue("ui_numBlueBots");
		// Number of client slots, minus however many local clients:
		int maxBots = trap_Cvar_VariableValue("ui_publicSlots") +
					  trap_Cvar_VariableValue("ui_privateSlots") -
					  UI_NumClients();

		// Check for overflow/underflow, and wrap to the right value:
		if(red+blue > maxBots)
			trap_Cvar_SetValue("ui_numBlueBots", 0);
		else if(blue < 0)
			trap_Cvar_SetValue("ui_numBlueBots", maxBots - red );
	}
	else if(Q_stricmp(name, "checkRed") == 0)
	{
		// This script is NOT run while the other data in this function is valid!
		int red = trap_Cvar_VariableValue("ui_numRedBots");
		int blue = trap_Cvar_VariableValue("ui_numBlueBots");
		// Number of client slots, minus however many local clients:
		int maxBots = trap_Cvar_VariableValue("ui_publicSlots") +
					  trap_Cvar_VariableValue("ui_privateSlots") -
					  UI_NumClients();

		// Check for overflow/underflow, and wrap to the right value:
		if(red+blue > maxBots)
			trap_Cvar_SetValue("ui_numRedBots", 0);
		else if(red < 0)
			trap_Cvar_SetValue("ui_numRedBots", maxBots - blue );
	}
	else if(Q_stricmp(name, "dedOnUpdateBots") == 0)
	{
		int deOn	= trap_Cvar_VariableValue("ui_dedicated");
		int botMin	= trap_Cvar_VariableValue("bot_minplayers");

		if(deOn && botMin > 0)
		{
			Cvar_SetValue("bot_minplayers", botMin - 1);
			UpdatePrevBotSlot();
		}
		else if(!deOn && botMin > 0)
		{
			Cvar_SetValue("bot_minplayers", botMin + 1);
			UpdateNextBotSlot();
		}
		else if(!deOn && botMin == 0)
		{
			Cvar_SetValue("bot_minplayers", botMin + 1);
		}
		else
		{
			UpdateNextBotSlot();
		}
	}
	else if(Q_stricmp(name, "dedChange") == 0)
	{
		// This script is NOT run while the other data in this function is valid!
		int red = trap_Cvar_VariableValue("ui_numRedBots");
		int blue = trap_Cvar_VariableValue("ui_numBlueBots");
		int pubSlots = trap_Cvar_VariableValue("ui_publicSlots");
		int privSlots = trap_Cvar_VariableValue("ui_privateSlots");
		int localClients = UI_NumClients();

		// If we just turned off dedicated, then re-clamp slots to 8
		if(localClients && (pubSlots + privSlots > 8))
		{
			// Remove only private slots, if possible
			if(pubSlots <= 8)
			{
				privSlots = 8 - pubSlots;
				trap_Cvar_SetValue("ui_privateSlots", privSlots);
			}
			else
			{
				// Remove all private, and then clamp public
				privSlots = 0;
				pubSlots = 8;
				trap_Cvar_SetValue("ui_privateSlots", privSlots);
				trap_Cvar_SetValue("ui_publicSlots", pubSlots);
			}
		}

		// Number of client slots, minus however many local clients:
		int maxBots = pubSlots + privSlots - localClients;

		// Make sure that bots won't overflow:
		if(red+blue > maxBots)
		{
			// Yup, too many bots now. Remove from whichever side has more:
			if(red > blue)
				trap_Cvar_SetValue("ui_numRedBots", maxBots - blue);
			else
				trap_Cvar_SetValue("ui_numBlueBots", maxBots - red);
		}

		// Likewise, minimum players:
//		if(trap_Cvar_VariableValue("bot_minplayers") > maxBots)
//			trap_Cvar_SetValue("bot_minplayers", maxBots);
	}
}

/*
===============
UI_XBL_PlayerListScript
Handle all UI script calls for player list
===============
*/
void UI_XBL_PlayerListScript(char **args, const char *name)
{
	if ( !String_Parse(args, &name) || !name )
	{
		return;
	}
	//
	// General player-list management
	//
	else if(Q_stricmp(name, "init") == 0)
	{
		XBL_PL_Init();
		Menu_SetFeederSelection(NULL, FEEDER_XBL_PLAYERS, 0, NULL);
	}
	else if(Q_stricmp(name, "shutdown") == 0)
	{
		XBL_PL_Cleanup();
	}
	else if (Q_stricmp(name, "popup") == 0)
	{
		// Copy selected player's info into the cvars
		if (!XBL_PL_SetCurrentInfo())
			return;

		// Show the popup
		Menus_ActivateByName("playerlist_popup");
	}
	//
	// User-specific actions
	//
	else if (Q_stricmp(name, "friendAdd") == 0)
	{
		XBL_PL_MakeFriend();
	}
	else if (Q_stricmp(name, "friendRemove") == 0)
	{
		XBL_PL_CancelFriend();
	}
	else if (Q_stricmp(name, "toggleMute") == 0)
	{
		XBL_PL_ToggleMute();
	}
	else if (Q_stricmp(name, "sendFeedback") == 0)
	{
		XBL_PL_SendFeedBack();
	}
	else if (Q_stricmp(name, "kick") == 0)
	{
		XBL_PL_KickPlayer();
	}
}

void UI_JoinSession()
{
	XBL_MM_SetJoinType( VIA_FRIEND_JOIN );
	HRESULT result = XBL_F_PerformMenuAction(UI_F_JOINSESSION);

	// Return value other than S_OK means they're playing a different game
	if( result != S_OK )
	{
		// Try to get the right title name:
		char titleString[MAX_TITLENAME_LEN+1] = { 0 };
		XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();
		if( curFriend )
			XBL_F_GetTitleString( curFriend->dwTitleID, titleString );

		// Need to to localization here, so we can splice the titleString in:
		if( titleString[0] )
			trap_Cvar_Set( "ui_swapMessage", va( SE_GetString("MENUS_INSERT_DISC_NAMED"), titleString ) );
		else
			trap_Cvar_Set( "ui_swapMessage", "@MENUS_INSERT_DISC_UNKNOWN" );
		Menus_ActivateByName("swap_disks_popup");
    }
}

void UI_JoinInvite()
{
	XBL_MM_SetJoinType( VIA_FRIEND_INVITE );
	HRESULT result = XBL_F_PerformMenuAction(UI_F_GAMEACCEPTED);

    // Return value other than S_OK means they're playing a different game
    if( result != S_OK )
    {
		// Try to get the right title name:
		char titleString[MAX_TITLENAME_LEN+1] = { 0 };
		XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();
		if( curFriend )
			XBL_F_GetTitleString( curFriend->dwTitleID, titleString );

		if( titleString[0] )
			trap_Cvar_Set( "ui_swapMessage", va( SE_GetString("MENUS_INSERT_DISC_NAMED"), titleString ) );
		else
			trap_Cvar_Set( "ui_swapMessage", "@MENUS_INSERT_DISC_UNKNOWN" );

		Menus_ActivateByName("swap_disks_popup");
	}
}

void RunVoteScript(const char* name)
{
	if (Q_stricmp(name, "voteMap") == 0) {
		int nextmap = trap_Cvar_VariableValue("vote_nextmap");
		if(nextmap != 0)
		{
			trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote nextmap\n") );

		}
		else if (ui_currentNetMap.integer >=0 && ui_currentNetMap.integer < uiInfo.mapCount) {
			trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote map %s\n",uiInfo.mapList[ui_currentNetMap.integer].mapLoadName) );
		}
	} else if (Q_stricmp(name, "voteKick") == 0) {
		if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
			//trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote kick \"%s\"\n",uiInfo.playerNames[uiInfo.playerIndex]) );
			trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote clientkick \"%i\"\n",uiInfo.playerIndexes[uiInfo.playerIndex]) );
		}
	} else if (Q_stricmp(name, "voteGame") == 0) {
		if (ui_netGameType.integer >= 0 && ui_netGameType.integer < uiInfo.numGameTypes) {
			trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote g_gametype %i\n",uiInfo.gameTypes[ui_netGameType.integer].gtEnum) );
		}
	} else if (Q_stricmp(name, "voteLeader") == 0) {
		if (uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount) {
			trap_Cmd_ExecuteText( EXEC_APPEND, va("callteamvote leader \"%s\"\n",uiInfo.teamNames[uiInfo.teamIndex]) );
		}
	}
}

/*
===============
UI_XBL_HandleFriendsPopUp
Determines which friend pop up to activate based on friend state.
===============
*/
void UI_XBL_HandleFriendsPopUp( void )
{
	// Get the chosen friend
	const XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();

	if( !curFriend )
		return;

	// Is the friend's game joinable?
	bool joinAvail = (bool)(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_JOINABLE);

	// We can invite if we're playing...
	bool inviteAvail = XBL_F_GetState( XONLINE_FRIENDSTATE_FLAG_PLAYING );

	// Are we in the same game as this friend?
	bool sameSession = (memcmp( &curFriend->sessionID, Net_GetXNKID(), sizeof(XNKID) ) == 0);

	// Double check that we're REALLY in a game
	uiClientState_t	cstate;
	trap_GetClientState( &cstate );
	if( cstate.connState != CA_ACTIVE && !com_dedicated->integer )
		inviteAvail = false;

	// If we're in the same game, neither option makes sense
	if( sameSession )
		inviteAvail = joinAvail = false;

	// Put the friend's name into the cvar used to display it in all the popups
	trap_Cvar_Set( "fl_selectedName", curFriend->szGamertag );

	// Based on current friendstates activate correct popup

	// Player has received a game invite fom the indicated friend (Accept/Decline/Remove)
	if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE &&
		!sameSession )
		Menus_ActivateByName("xbf_ReceivedInvite_popup");

	// We sent an invite, didn't hear back. Player is online and playing (joinable). (Cancel/Join/Remove)
	else if( (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTINVITE) &&
			 !(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED) && 
			 !(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED) && 
			 (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING) &&
			 joinAvail )
		Menus_ActivateByName("xbf_SentInviteCanJoin_popup");

	// We sent an invite, didn't hear back. Player is idle, offline, or in our game (not joinable). (Cancel/Remove)
	else if( (curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTINVITE) && 
			 !(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED) && 
			 !(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED) && 
			 !(curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING) )
		Menus_ActivateByName("xbf_SentInviteNoJoin_popup");

	// Player has received a friend request. (Accept/Decline/Block)
	else if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST )
		Menus_ActivateByName("xbf_ReceivedRequest_popup");

	// Player has sent a friend request. (Revoke)
	else if( curFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST )
		Menus_ActivateByName("xbf_SentRequest_popup");

	// Now, there are four more options, for all combinations of join and invite being available.
	// It really doesn't matter if the user is online or offline.

	// Player is joinable, and we have the option to invite (Invite/Join/Remove)
	else if( joinAvail && inviteAvail )
		Menus_ActivateByName("xbf_BasicInviteJoin_popup");

	// Player is joinable, but we can't send an invitation (Join/Remove)
	else if( joinAvail && !inviteAvail )
		Menus_ActivateByName("xbf_BasicJoin_popup");

	// Player can be invited, but we can't join their game (Invite/Remove)
	else if( !joinAvail && inviteAvail )
		Menus_ActivateByName("xbf_BasicInvite_popup");

	// We can't invite, can't join (Remove)
	else
		Menus_ActivateByName("xbf_Basic_popup");
}

/*
===============
UI_XBL_FriendsListScript
Handle all UI script calls for friends list
===============
*/
void UI_XBL_FriendsListScript(char **args, const char *name)
{
	if ( !String_Parse(args, &name) || !name )
	{
		return;
	}
	//
	// General friends-list management
	//
	else if(Q_stricmp(name, "init") == 0)
	{
		// VVFIXME - Initialize selection or something? - generate isn't instant!
		XBL_F_GenerateFriendsList();
		Menu_SetFeederSelection(NULL, FEEDER_XBL_FRIENDS, 0, NULL);
	}
	else if(Q_stricmp(name, "shutdown") == 0)
	{
		XBL_F_ReleaseFriendsList();
	}
	else if (Q_stricmp(name, "popup") == 0)
	{
		// Display the correct popup, depending on many factors
		UI_XBL_HandleFriendsPopUp();
	}
	//
	// Respond to friend requests
	//
	else if (Q_stricmp(name, "accept") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_FRIENDACCEPTED);
	}
	else if (Q_stricmp(name, "decline") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_FRIENDDECLINE);
	}
	else if (Q_stricmp(name, "block") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_FRIENDBLOCK);
	}
	else if (Q_stricmp(name, "blockInit") == 0)
	{
		// Need to set xb_errMessage to be the properly formatted string
		// containing the gamertag of the player:
		Cvar_Set( "xb_errMessage", va( SE_GetString( "MENUS_CONFIRM_BLOCK" ), Cvar_VariableString( "fl_selectedName" ) ) );
	}

	//
	// Remove an existing friend or cancel a pending friend request
	//
	else if (Q_stricmp(name, "remove") == 0)
	{
		// Destructive action, need to get confirmation:
		UI_xboxErrorPopup( XB_POPUP_CONFIRM_FRIEND_REMOVE );
//		XBL_F_PerformMenuAction(UI_F_FRIENDREMOVE);
	}
	else if (Q_stricmp(name, "cancel") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_FRIENDCANCEL);
	}

	//
	// Invite someone to play, or cancel a pending invitation
	//
	else if (Q_stricmp(name, "invite") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_GAMEREQUESTED);
	}
	else if(Q_stricmp(name, "uninvite") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_GAMECANCEL);
	}

	//
	// Respond to a game invitation - three stages
	//
	else if (Q_stricmp(name, "acceptInvite") == 0)
	{
		XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();

		if( com_sv_running->integer || cls.state == CA_ACTIVE )
		{
			// We're already playing/hosting, get confirmation:
			XBL_MM_SetJoinType( VIA_FRIEND_INVITE );
			UI_xboxErrorPopup( XB_POPUP_HOST_JOIN_CONFIRM );
		}
		else if( curFriend && XBL_MM_ThisSessionIsLagging( &curFriend->sessionID ) )
		{
			// We're in the front-end, but the server is slow:
			XBL_MM_SetJoinType(VIA_FRIEND_INVITE);
			Menus_ActivateByName( "slow_server_popup" );
		}
		else
		{
			// Front-end, server is fast:
			UI_JoinInvite();
		}
	}
	else if (Q_stricmp(name, "declineInvite") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_GAMEDECLINE);
	}
	else if (Q_stricmp(name, "removeInviter") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_GAMEFRIENDREMOVED);
	}

	//
	// Join someone else's game uninvited
	//
	else if (Q_stricmp(name, "join") == 0)
	{
        XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();

		if( com_sv_running->integer || cls.state == CA_ACTIVE )
		{
			// We're already playing/hosting, get confirmation:
			XBL_MM_SetJoinType( VIA_FRIEND_JOIN );
			UI_xboxErrorPopup( XB_POPUP_HOST_JOIN_CONFIRM );
		}
		else if( curFriend && XBL_MM_ThisSessionIsLagging( &curFriend->sessionID ) )
        {
			// We're in the front-end, but the server is slow:
            XBL_MM_SetJoinType(VIA_FRIEND_JOIN);
			Menus_ActivateByName( "slow_server_popup" );
        }
        else
        {
			// Front-end, server is fast:
            UI_JoinSession();
        }
	}

	// Appear online/offline
	else if(Q_stricmp(name, "toggleOnline") == 0)
	{
		XBL_F_PerformMenuAction(UI_F_TOGGLEONLINE);		
	}
}

// Function to allow the server to switch maps and such. Most of this code is
// based on CheckVote() in g_main.c
void UI_ServerMap( char **args, const char *name )
{
	if ( !String_Parse(args, &name) || !name )
	{
		return;
	}

	// update - Makes sure that the newMap variable is valid for the newGameType
	else if (Q_stricmp(name, "update") == 0)
	{
		// Get current game type:
		int newGameType = trap_Cvar_VariableValue( "ui_newGameType" );
		const char *newMap;

		switch (newGameType)
		{
		case 0: case 6:
			newMap = "mp/ffa1"; break;
		case 3: case 4:
			newMap = "mp/duel1"; break;
		case 7:
			newMap = "mp/siege_hoth"; break;
		case 8:
			newMap = "mp/ctf1"; break;
		default:
			assert( 0 );
		}

		// Update the map var
		trap_Cvar_Set( "ui_newMap", newMap );
	}
	else if (Q_stricmp(name, "confirm") == 0)
	{
		// Executes the map switch, lots of code stolen from CheckVote and such

		int botMin		= trap_Cvar_VariableValue( "bot_minplayers" );
		int tempBotMin	= trap_Cvar_VariableValue( "temp_bot_minplayers" );

		if( tempBotMin != botMin )
		{
			trap_Cvar_SetValue("bot_minplayers", tempBotMin);
		}

		// Grab the target values from the cvars:
		int newGameType = trap_Cvar_VariableValue( "ui_newGameType" );
		char newMap[64];
		trap_Cvar_VariableStringBuffer( "ui_newMap", newMap, sizeof(newMap) );

		int gameType = trap_Cvar_VariableValue( "g_gametype" );

		// If we're switching gametypes...
		if (newGameType != gameType)
			Cbuf_ExecuteText( EXEC_APPEND, va("g_gametype %d\n", newGameType) );

		// And then we always send the map change command:
		Cbuf_ExecuteText( EXEC_APPEND, va("map %s\n", newMap) );

		Cbuf_ExecuteText( EXEC_APPEND, "kick allbots\n");
	}
}

static void UI_SwitchForceSide(int val, int min, int max);
const char *UI_TeamName(int team);

void UI_XB_ForceScript( char **args, const char *name )
{
	if ( !String_Parse(args, &name) || !name )
	{
		return;
	}

	// init - sets initial cvars for the first force screen
	else if (Q_stricmp(name, "init") == 0)
	{
		char	info[MAX_INFO_STRING];
		trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

		int ingameChangingForce = 0;
		int team = trap_Cvar_VariableValue( "xb_joinTeam" );
		if ( team == 4)
		{
			ingameChangingForce = 1;
			team = ClientManager::ActiveClient().myTeam;
		}




		int forceTeams = atoi(Info_ValueForKey(info, "g_forceBasedTeams"));
		int gametype = atoi(Info_ValueForKey(info, "g_gametype"));
		uiMaxRank = atoi(Info_ValueForKey(info, "g_maxForceRank"));

		// If team specific powers are on, make sure that we restrict the user:
		if (forceTeams && (gametype >= GT_TEAM))
			trap_Cvar_SetValue( "ui_disableForceSideChange", 1 );
		else
			trap_Cvar_SetValue( "ui_disableForceSideChange", 0 );

		
		int forceSide;
		// If we're not playing a team game, just put them on light side
/*		if (gametype < GT_TEAM) // No teams - light powers
			forceSide = 0;
		else if (team == 1)		// Blue team - light powers
			forceSide = 1;
		else					// Red team - dark powers
			forceSide = 2;
*/
//JLF NEW
		if (forceTeams)
		{
			if (  team == TEAM_RED)
				ClientManager::ActiveClient().forceSide = FORCE_DARKSIDE; 
			else
				ClientManager::ActiveClient().forceSide = FORCE_LIGHTSIDE; 
		}

		uiForceSide = ClientManager::ActiveClient().forceSide;
		forceSide = uiForceSide;

		// uiForceSide = 1;
		UI_UpdateForcePowers();

		uiForceSide = forceSide;

		// Set the force side cvar - unless we came from the in-game menu directly,
		// in which case we override and don't change this:
		if (ingameChangingForce)
		{
			trap_Cvar_SetValue("ui_forceSideCvar",uiForceSide);
			forceSide = trap_Cvar_VariableValue( "ui_forceSideCvar" );
		}
		else
		{
			
/*
		// If we're not playing a team game, just put them on light side
			if (gametype < GT_TEAM) // No teams - light powers
				forceSide = uiForceSide;
			else if (team == TEAM_BLUE)		// Blue team - light powers
				forceSide = 1;
			else					// Red team - dark powers
				forceSide = 2;
*/
			trap_Cvar_SetValue( "ui_forceSideCvar", forceSide);

//			uiForceSide = forceSide;

		}



		if ( gametype < GT_TEAM)
		{
			uiForcePowersRank[FP_TEAM_HEAL] = 0;
			uiForcePowersRank[FP_TEAM_FORCE] = 0;
		}


		
	//	Cvar_SetValue("ui_forceConfigCvar",ClientManager::ActiveClient().forceConfig);
			// Update the configuration feeder
		Cvar_SetValue("ui_forceConfigCvar", 0 );
		ClientManager::ActiveClient().forceConfig = 0;


		UI_FeederSelection(FEEDER_FORCECFG, trap_Cvar_VariableValue("ui_forceConfigCvar"), NULL); 

		UI_ForceSide_HandleKey(0, 0, A_CURSOR_RIGHT, (forceSide==1)?2:1, 1, 2, UI_FORCE_SIDE);
		UI_SwitchForceSide(forceSide, 1, 2);
	//	trap_Cvar_Set( "ui_forceConfigModified", "1" );

	}
	// Confirm - make changes to force powers, (also sets team, indirectly)
	else if (Q_stricmp(name, "confirm") == 0)
	{
		// Similar to the setForce uiScript, but more automatic:

		// What team are we joining?
		int team = trap_Cvar_VariableValue( "xb_joinTeam" );
		int myTeam = ClientManager::ActiveClient().myTeam;
		int gametype = trap_Cvar_VariableValue( "g_gametype" );

		gTouchedForce[ClientManager::ActiveClientNum()] = qtrue;

//JLF
	
		if (team == 4)
		{
			// No change, came directly from in-game menu without team picker:
//JLFTESTXXX
		//	if ( myTeam != TEAM_SPECTATOR )
		//		UI_UpdateClientForcePowers( UI_TeamName(myTeam) );	// Will cause respawn?
		//	else
				UI_UpdateClientForcePowers( NULL );					// Just update powers
		}
		else if (team == 1)
		{
			// Meaning depends on whether or not we're playing powerduel
			if (gametype == GT_POWERDUEL)
			{
				UI_UpdateClientForcePowers( NULL );
				Cbuf_ExecuteText( EXEC_APPEND, "duelteam single\n" );	// Join singles team
			}
			else
				UI_UpdateClientForcePowers( "red" );					// Join red team
			//	UI_UpdateClientForcePowers( "blue" );					// Join blue team
			
		}
		else if (team == 2)
		{
			// Meaning depends on whether or not we're playing powerduel
			if (gametype == GT_POWERDUEL)
			{
				UI_UpdateClientForcePowers( NULL );
				Cbuf_ExecuteText( EXEC_APPEND, "duelteam double\n" );	// Join doubles team
			}
			else
				UI_UpdateClientForcePowers( "blue" );					// Join blue team
			//	UI_UpdateClientForcePowers( "red" );					// Join red team
		}
		else if (team == 0)
		{
			// Non-team game
			UI_UpdateClientForcePowers( "free" );
		}

		ClientManager::ActiveClient().forceSide = Cvar_VariableValue("ui_forceSideCvar");
	
//		if (trap_Cvar_VariableValue("ui_forceConfigModified"))
//		{
//			ClientManager::ActiveClient().forceConfig = Cvar_VariableValue("ui_forceConfigCvar");
//		}
			
	}
}

/*
	MASTER Startup function for saved games, invite checks, etc...
	Modeled after XBL_Login
*/
void XB_Startup( XBStartupState startupState )
{
	if( startupState <= STARTUP_LOAD_SETTINGS )
	{
		// Cheap trick to prime the _initLD function in win_main_console -
		// so that Load will return true if user chose not to save settings in SP:
		Sys_AcceptedInvite();

		bool bSuccess = Settings.Load();
		if( !bSuccess )
		{
			// Odd. If saving was disabled, then Load will appear to work.
			UI_xboxErrorPopup( XB_POPUP_CORRUPT_SETTINGS );
			return;
		}
	}

	if( startupState <= STARTUP_COMBINED_SPACE_CHECK )
	{
		// Gah. This should ALWAYS work, but if it doesn't, we just disable settings
		// to thwart the cosmic rays that allowed it to happen!
		if( !Settings.Save() )
			Settings.Disable();
	}

	if( startupState <= STARTUP_INVITE_CHECK )
	{
		// Last real stage in MP:
		// Restore all settings (from file or defaults):
		Settings.SetAll();

		// Called when the first MP menu is opened, to possibly put us on the fast-track
		// to joining a Live game. If so, we immediately open the account menu.
		XONLINE_ACCEPTED_GAMEINVITE *pInvite = Sys_AcceptedInvite();

		// If there's no invite, do nothing here (common case)
		if (!pInvite)
			return;

		// There's an invite waiting. Now we try to do some automation:

		// First, start up Xbox Live. Let that fail:
		if (XBL_Init() != S_OK)
			return;

		// Next, we try to locate the user that accepted the invite,
		// this can also fail:
		if (!XBL_SetAccountIndexByXuid( &pInvite->xuidAcceptedFriend ) )
			return;

		// Now start the popup-filled login sequence:
		XBL_Login( LOGIN_PASSCODE_CHECK );

		// Fix for various bugs where xb_gameType wasn't getting set in this
		// code-path - caused much UI to stop working. Doh.
		Cvar_SetValue( "xb_gameType", 3 );
	}
}



void UI_TransitionVoteMenu( char **args, const char *name )
{
	char nextmenu[32]; 
	if(cgs.voteTime <= 0)
		trap_Cvar_VariableStringBuffer("call_vote_menu_name", nextmenu, sizeof(nextmenu));
	else if(!cgs.votePlaced)
		trap_Cvar_VariableStringBuffer("vote_menu_name", nextmenu, sizeof(nextmenu));
	else
        trap_Cvar_VariableStringBuffer("alreadyvote_menu_name", nextmenu, sizeof(nextmenu));

	Menus_OpenByName(nextmenu);
}
#endif	// _XBOX

void UI_AssignGameType(char **args, const char *name)
{
	if ( !String_Parse(args, &name) || !name )
	{
		Com_Printf("Need a gametype argument");
		return;
	}

	if(Q_stricmp(name, "FFA") == 0)
		ui_netGameType.integer = 0;
	else if(Q_stricmp(name, "Duel") == 0)
		ui_netGameType.integer = 1;
	else if(Q_stricmp(name, "PowerDuel") == 0)
		ui_netGameType.integer = 2;
	else if(Q_stricmp(name, "TeamFFA") == 0)
		ui_netGameType.integer = 3;
	else if(Q_stricmp(name, "Siege") == 0)
		ui_netGameType.integer = 4;
	else if(Q_stricmp(name, "CTF") == 0)
		ui_netGameType.integer = 5;
	else
		Com_Printf("Bad gametype: %s", name);

//	trap_Cvar_Set( "ui_netGameType", va("%d", ui_netGameType.integer));
//	trap_Cvar_Set( "ui_actualnetGameType", va("%d", uiInfo.gameTypes[ui_netGameType.integer].gtEnum));
//	trap_Cvar_Set( "ui_currentNetMap", "0");
//	UI_MapCountByGameType(qfalse);
}


// Movedata Sounds
typedef enum
{
	MDS_NONE = 0,
	MDS_FORCE_JUMP,
	MDS_ROLL,
	MDS_SABER,
	MDS_MOVE_SOUNDS_MAX
};

typedef enum
{
	MD_ACROBATICS = 0,
	MD_SINGLE_FAST,
	MD_SINGLE_MEDIUM,
	MD_SINGLE_STRONG,
	MD_DUAL_SABERS,
	MD_SABER_STAFF,
	MD_MOVE_TITLE_MAX
};

// Some hard coded badness
// At some point maybe this should be externalized to a .dat file
char *datapadMoveTitleData[MD_MOVE_TITLE_MAX] =
{
"@MENUS_ACROBATICS",
"@MENUS_SINGLE_FAST",
"@MENUS_SINGLE_MEDIUM",
"@MENUS_SINGLE_STRONG",
"@MENUS_DUAL_SABERS",
"@MENUS_SABER_STAFF",
};

char *datapadMoveTitleBaseAnims[MD_MOVE_TITLE_MAX] =
{
"BOTH_RUN1",
"BOTH_SABERFAST_STANCE",
"BOTH_STAND2",
"BOTH_SABERSLOW_STANCE",
"BOTH_SABERDUAL_STANCE",
"BOTH_SABERSTAFF_STANCE",
};

#define MAX_MOVES 16

typedef struct 
{
	char	*title;	
	char	*desc;	
	char	*anim;
	short	sound;
} datpadmovedata_t;

static datpadmovedata_t datapadMoveData[MD_MOVE_TITLE_MAX][MAX_MOVES] = 
{
// Acrobatics
"@MENUS_FORCE_JUMP1",				"@MENUS_FORCE_JUMP1_DESC",			"BOTH_FORCEJUMP1",				MDS_FORCE_JUMP,
"@MENUS_FORCE_FLIP",				"@MENUS_FORCE_FLIP_DESC",			"BOTH_FLIP_F",					MDS_FORCE_JUMP,
"@MENUS_ROLL",						"@MENUS_ROLL_DESC",					"BOTH_ROLL_F",					MDS_ROLL,
"@MENUS_BACKFLIP_OFF_WALL",			"@MENUS_BACKFLIP_OFF_WALL_DESC",	"BOTH_WALL_FLIP_BACK1",			MDS_FORCE_JUMP,
"@MENUS_SIDEFLIP_OFF_WALL",			"@MENUS_SIDEFLIP_OFF_WALL_DESC",	"BOTH_WALL_FLIP_RIGHT",			MDS_FORCE_JUMP,
"@MENUS_WALL_RUN",					"@MENUS_WALL_RUN_DESC",				"BOTH_WALL_RUN_RIGHT",			MDS_FORCE_JUMP,
"@MENUS_WALL_GRAB_JUMP",			"@MENUS_WALL_GRAB_JUMP_DESC",		"BOTH_FORCEWALLREBOUND_FORWARD",MDS_FORCE_JUMP,
"@MENUS_RUN_UP_WALL_BACKFLIP",		"@MENUS_RUN_UP_WALL_BACKFLIP_DESC",	"BOTH_FORCEWALLRUNFLIP_START",	MDS_FORCE_JUMP,
"@MENUS_JUMPUP_FROM_KNOCKDOWN",		"@MENUS_JUMPUP_FROM_KNOCKDOWN_DESC","BOTH_KNOCKDOWN3",				MDS_NONE,
"@MENUS_JUMPKICK_FROM_KNOCKDOWN",	"@MENUS_JUMPKICK_FROM_KNOCKDOWN_DESC","BOTH_KNOCKDOWN2",			MDS_NONE,
"@MENUS_ROLL_FROM_KNOCKDOWN",		"@MENUS_ROLL_FROM_KNOCKDOWN_DESC",	"BOTH_KNOCKDOWN1",				MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,

//Single Saber, Fast Style
"@MENUS_STAB_BACK",					"@MENUS_STAB_BACK_DESC",			"BOTH_A2_STABBACK1",			MDS_SABER,
"@MENUS_LUNGE_ATTACK",				"@MENUS_LUNGE_ATTACK_DESC",			"BOTH_LUNGE2_B__T_",			MDS_SABER,
"@MENUS_FAST_ATTACK_KATA",			"@MENUS_FAST_ATTACK_KATA_DESC",		"BOTH_A1_SPECIAL",				MDS_SABER,
"@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP,
"@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP,
"@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,

//Single Saber, Medium Style
"@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER,
"@MENUS_FLIP_ATTACK",				"@MENUS_FLIP_ATTACK_DESC",			"BOTH_JUMPFLIPSLASHDOWN1",		MDS_FORCE_JUMP,
"@MENUS_MEDIUM_ATTACK_KATA",		"@MENUS_MEDIUM_ATTACK_KATA_DESC",	"BOTH_A2_SPECIAL",				MDS_SABER,
"@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP,
"@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP,
"@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,

//Single Saber, Strong Style
"@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER,
"@MENUS_JUMP_ATTACK",				"@MENUS_JUMP_ATTACK_DESC",			"BOTH_FORCELEAP2_T__B_",		MDS_FORCE_JUMP,
"@MENUS_STRONG_ATTACK_KATA",		"@MENUS_STRONG_ATTACK_KATA_DESC",	"BOTH_A3_SPECIAL",				MDS_SABER,
"@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN",				MDS_FORCE_JUMP,
"@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP,
"@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,

//Dual Sabers
"@MENUS_SLASH_BACK",				"@MENUS_SLASH_BACK_DESC",			"BOTH_ATTACK_BACK",				MDS_SABER,
"@MENUS_FLIP_FORWARD_ATTACK",		"@MENUS_FLIP_FORWARD_ATTACK_DESC",	"BOTH_JUMPATTACK6",				MDS_FORCE_JUMP,
"@MENUS_DUAL_SABERS_TWIRL",			"@MENUS_DUAL_SABERS_TWIRL_DESC",	"BOTH_SPINATTACK6",				MDS_SABER,
"@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_DUAL",				MDS_FORCE_JUMP,
"@MENUS_DUAL_SABER_BARRIER",		"@MENUS_DUAL_SABER_BARRIER_DESC",	"BOTH_A6_SABERPROTECT",			MDS_SABER,
"@MENUS_DUAL_STAB_FRONT_BACK",		"@MENUS_DUAL_STAB_FRONT_BACK_DESC", "BOTH_A6_FB",					MDS_SABER,
"@MENUS_DUAL_STAB_LEFT_RIGHT",		"@MENUS_DUAL_STAB_LEFT_RIGHT_DESC", "BOTH_A6_LR",					MDS_SABER,
"@MENUS_CARTWHEEL",					"@MENUS_CARTWHEEL_DESC",			"BOTH_ARIAL_RIGHT",				MDS_FORCE_JUMP,
"@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB_DESC",		"BOTH_ROLL_STAB",				MDS_SABER,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,

// Saber Staff
"@MENUS_STAB_BACK",					"@MENUS_STAB_BACK_DESC",			"BOTH_A2_STABBACK1",			MDS_SABER,
"@MENUS_BACK_FLIP_ATTACK",			"@MENUS_BACK_FLIP_ATTACK_DESC",		"BOTH_JUMPATTACK7",				MDS_FORCE_JUMP,
"@MENUS_SABER_STAFF_TWIRL",			"@MENUS_SABER_STAFF_TWIRL_DESC",	"BOTH_SPINATTACK7",				MDS_SABER,
"@MENUS_ATTACK_ENEMYONGROUND",		"@MENUS_ATTACK_ENEMYONGROUND_DESC", "BOTH_STABDOWN_STAFF",			MDS_FORCE_JUMP,
"@MENUS_SPINNING_KATA",				"@MENUS_SPINNING_KATA_DESC",		"BOTH_A7_SOULCAL",				MDS_SABER,
"@MENUS_KICK1",						"@MENUS_KICK1_DESC",				"BOTH_A7_KICK_F",				MDS_FORCE_JUMP,
"@MENUS_JUMP_KICK",					"@MENUS_JUMP_KICK_DESC",			"BOTH_A7_KICK_F_AIR",			MDS_FORCE_JUMP,
"@MENUS_BUTTERFLY_ATTACK",			"@MENUS_BUTTERFLY_ATTACK_DESC",		"BOTH_BUTTERFLY_FR1",			MDS_SABER,
"@MENUS_BOTH_ROLL_STAB",			"@MENUS_BOTH_ROLL_STAB2_DESC",		"BOTH_ROLL_STAB",				MDS_SABER,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
NULL, NULL, 0,	MDS_NONE,
};


/*
================
vmMain

This is the only way control passes into the module.
!!! This MUST BE THE VERY FIRST FUNCTION compiled into the .qvm file !!!
================
*/
vmCvar_t  ui_debug;
vmCvar_t  ui_initialized;
vmCvar_t	ui_char_color_red;
vmCvar_t	ui_char_color_green;
vmCvar_t	ui_char_color_blue;
vmCvar_t	ui_PrecacheModels;
vmCvar_t	ui_char_anim;

//JLF Menu progression
vmCvar_t	ui_menuProgression;
//vmCvar_t	ui_menuClient;

//controller menu
vmCvar_t	ControllerOutNum	;


vmCvar_t   ui_respawnneeded;

//END JLFCALLOUT

void _UI_Init( qboolean );
void _UI_Shutdown( void );
void _UI_KeyEvent( int key, qboolean down );
void _UI_MouseEvent( int dx, int dy );
void _UI_Refresh( int realtime );
qboolean _UI_IsFullscreen( void );
void UI_SetSiegeTeams(void);
extern qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel );
void UI_SiegeSetCvarsForClass(siegeClass_t *scl);
int UI_SiegeClassNum(siegeClass_t *scl);
void UI_UpdateCvarsForClass(const int team,const baseClass,const int index);
void	UI_UpdateSiegeStatusIcons(void);
void UI_ClampMaxPlayers(void);
static void UI_CheckServerName( void );
static qboolean UI_CheckPassword( void );
static void UI_JoinServer( void );

#include "../namespace_begin.h"
// Functions in BG or ui_shared
void Menu_ShowGroup (menuDef_t *menu, char *itemName, qboolean showFlag);
void Menu_ItemDisable(menuDef_t *menu, char *name,int disableFlag);
int Menu_ItemsMatchingGroup(menuDef_t *menu, const char *name);
itemDef_t *Menu_GetMatchingItemByNumber(menuDef_t *menu, int index, const char *name);

int BG_GetUIPortrait(const int team, const short classIndex, const short cntIndex);
char *BG_GetUIPortraitFile(const int team, const short classIndex, const short cntIndex);

siegeClass_t *BG_GetClassOnBaseClass(const int team, const short classIndex, const short cntIndex);

int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
  switch ( command ) {
	  case UI_GETAPIVERSION:
		  return UI_API_VERSION;

	  case UI_INIT:
		  _UI_Init(arg0);
		  return 0;

	  case UI_SHUTDOWN:
		  _UI_Shutdown();
		  return 0;

	  case UI_KEY_EVENT:
		  _UI_KeyEvent( arg0, arg1 );
		  return 0;

	  case UI_MOUSE_EVENT:
		  _UI_MouseEvent( arg0, arg1 );
		  return 0;

	  case UI_REFRESH:
		  _UI_Refresh( arg0 );
		  return 0;

	  case UI_IS_FULLSCREEN:
		  return _UI_IsFullscreen();

	  case UI_SET_ACTIVE_MENU:
		  _UI_SetActiveMenu( arg0 );
		  return 0;

	  case UI_CONSOLE_COMMAND:
		  return UI_ConsoleCommand(arg0);

	  case UI_DRAW_CONNECT_SCREEN:
		  UI_DrawConnectScreen( arg0 );
		  return 0;
	  case UI_HASUNIQUECDKEY: // mod authors need to observe this
	    return qtrue; // bk010117 - change this to qfalse for mods!
	  case UI_MENU_RESET:
		  Menu_Reset();
		  return 0;
	}

	return -1;
}
#include "../namespace_end.h"

siegeClassDesc_t g_UIClassDescriptions[MAX_SIEGE_CLASSES];
siegeTeam_t *siegeTeam1 = NULL;
siegeTeam_t *siegeTeam2 = NULL;
int g_UIGloballySelectedSiegeClass = -1;

//Cut down version of the stuff used in the game code
//This is just the bare essentials of what we need to load animations properly for ui ghoul2 models.
//This function doesn't need to be sync'd with the BG_ version in bg_panimate.c unless some sort of fundamental change
//is made. Just make sure the variables/functions accessed in ui_shared.c exist in both modules.
qboolean	UIPAFtextLoaded = qfalse;
animation_t	uiHumanoidAnimations[MAX_TOTALANIMATIONS]; //humanoid animations are the only ones that are statically allocated.

#include "../namespace_begin.h"
bgLoadedAnim_t bgAllAnims[MAX_ANIM_FILES];
int uiNumAllAnims = 1; //start off at 0, because 0 will always be assigned to humanoid.
#include "../namespace_end.h"

animation_t *UI_AnimsetAlloc(void)
{
	assert (uiNumAllAnims < MAX_ANIM_FILES);
	bgAllAnims[uiNumAllAnims].anims = (animation_t *) BG_Alloc(sizeof(animation_t)*MAX_TOTALANIMATIONS);

	return bgAllAnims[uiNumAllAnims].anims;
}

/*
======================
UI_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc

======================
*/
#include "../namespace_begin.h"
//static char UIPAFtext[60000];
int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid) 
{
	char		*text_p;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	int			usedIndex = -1;
	int			nextIndex = uiNumAllAnims;

	fileHandle_t	f;
	int				animNum;

	if (!isHumanoid)
	{
		i = 1;
		while (i < uiNumAllAnims)
		{ //see if it's been loaded already
			if (!Q_stricmp(bgAllAnims[i].filename, filename))
			{
				animset = bgAllAnims[i].anims;
				return i; //alright, we already have it.
			}
			i++;
		}

		//Looks like it has not yet been loaded. Allocate space for the anim set if we need to, and continue along.
		if (!animset)
		{
			if (strstr(filename, "players/_humanoid/"))
			{ //then use the static humanoid set.
				animset = uiHumanoidAnimations;
				isHumanoid = qtrue;
				nextIndex = 0;
			}
			else
			{
				animset = UI_AnimsetAlloc();

				if (!animset)
				{
					assert(!"Anim set alloc failed!");
					return -1;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		assert(animset);
	}
#endif

	char *UIPAFtext = NULL;

	// load the file
	if (!UIPAFtextLoaded || !isHumanoid)
	{ //rww - We are always using the same animation config now. So only load it once.
		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		if (len <= 0)
			return -1;
/*
		if ( (len <= 0) || (len >= sizeof( UIPAFtext ) - 1) ) 
		{
			if (len > 0)
			{
				Com_Error(ERR_DROP, "%s exceeds the allowed ui-side animation buffer!", filename);
			}
			return -1;
		}
*/

		UIPAFtext = (char *) Z_Malloc( len+1, TAG_TEMP_WORKSPACE, qfalse, 4 );

		trap_FS_Read( UIPAFtext, len, f );
		UIPAFtext[len] = 0;
		trap_FS_FCloseFile( f );
	}
	else
	{
		return 0; //humanoid index
	}

	// parse the text
	text_p = UIPAFtext;
	skip = 0;	// quiet the compiler warning

	//FIXME: have some way of playing anims backwards... negative numFrames?

	//initialize anim array so that from 0 to MAX_ANIMATIONS, set default values of 0 1 0 100
	for(i = 0; i < MAX_ANIMATIONS; i++)
	{
		animset[i].firstFrame = 0;
		animset[i].numFrames = 0;
		animset[i].loopFrames = -1;
		animset[i].frameLerp = 100;
//		animset[i].initialLerp = 100;
	}

	// read information for each frame
	while(1) 
	{
		token = COM_Parse( (const char **)(&text_p) );

		if ( !token || !token[0]) 
		{
			break;
		}

		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{
//#ifndef FINAL_BUILD
#ifdef _DEBUG
			//Com_Printf(S_COLOR_RED"WARNING: Unknown token %s in %s\n", token, filename);
#endif
			continue;
		}

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) 
		{
			break;
		}
		animset[animNum].firstFrame = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) 
		{
			break;
		}
		animset[animNum].numFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) 
		{
			break;
		}
		animset[animNum].loopFrames = atoi( token );

		token = COM_Parse( (const char **)(&text_p) );
		if ( !token ) 
		{
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) 
		{
			fps = 1;//Don't allow divide by zero error
		}
		if ( fps < 0 )
		{//backwards
			animset[animNum].frameLerp = floor(1000.0f / fps);
		}
		else
		{
			animset[animNum].frameLerp = ceil(1000.0f / fps);
		}

//		animset[animNum].initialLerp = ceil(1000.0f / fabs(fps));
	}

	if (isHumanoid)
	{
		bgAllAnims[0].anims = animset;
		strcpy(bgAllAnims[0].filename, filename);
		UIPAFtextLoaded = qtrue;

		usedIndex = 0;
	}
	else
	{
		bgAllAnims[nextIndex].anims = animset;
		strcpy(bgAllAnims[nextIndex].filename, filename);

		usedIndex = nextIndex;

		if (nextIndex)
		{ //don't bother increasing the number if this ended up as a humanoid load.
			uiNumAllAnims++;
		}
		else
		{
			UIPAFtextLoaded = qtrue;
			usedIndex = 0;
		}
	}

	Z_Free( UIPAFtext );

	return usedIndex;
}

//menuDef_t *Menus_FindByName(const char *p);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);

#include "../namespace_end.h"

void UpdateForceUsed();

char holdSPString[MAX_STRING_CHARS]={0};
char holdSPString2[MAX_STRING_CHARS]={0};

uiInfo_t uiInfo;

static void UI_BuildFindPlayerList(qboolean force);
static int UI_MapCountByGameType(qboolean singlePlayer);
static int UI_HeadCountByColor( void );
static void UI_ParseGameInfo(const char *teamFile);
static const char *UI_SelectedMap(int index, int *actual);
static int UI_GetIndexFromSelection(int actual);
static void UI_SiegeClassCnt( const int team );


int ProcessNewUI( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );
int	uiSkinColor=TEAM_FREE;
int	uiHoldSkinColor=TEAM_FREE;	// Stores the skin color so that in non-team games, the player screen remembers the team you chose, in case you're coming back from the force powers screen.

static const char *skillLevels[] = {
  "SKILL1",//"I Can Win",
  "SKILL2",//"Bring It On",
  "SKILL3",//"Hurt Me Plenty",
  "SKILL4",//"Hardcore",
  "SKILL5"//"Nightmare"
};
static const int numSkillLevels = sizeof(skillLevels) / sizeof(const char*);



static const char *teamArenaGameTypes[] = {
	"FFA",
	"Holocron",
	"JediMaster",
	"Duel",
	"PowerDuel",
	"SP",
	"Team FFA",
	"Siege",
	"CTF",
	"CTY",
	"TeamTournament"
};
static int const numTeamArenaGameTypes = sizeof(teamArenaGameTypes) / sizeof(const char*);



static char* netnames[] = {
	"???",
	"UDP",
	"IPX",
	NULL
};

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

const char *UI_GetStringEdString(const char *refSection, const char *refName);

const char *UI_TeamName(int team)  {
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

// returns either string or NULL for OOR...
//
static const char *GetCRDelineatedString( const char *psStripFileRef, const char *psStripStringRef, int iIndex)
{
	static char sTemp[256];
	const char *psList = UI_GetStringEdString(psStripFileRef, psStripStringRef);
	char *p;

	while (iIndex--)
	{
		psList = strchr(psList,'\n');
		if (!psList){
			return NULL;	// OOR
		}
		psList++;
	}

	strcpy(sTemp,psList);
	p = strchr(sTemp,'\n');
	if (p) {
		*p = '\0';
	}

	return sTemp;
}


static const char *GetMonthAbbrevString( int iMonth )
{
	const char *p = GetCRDelineatedString("MP_INGAME","MONTHS", iMonth);
	
	return p ? p : "Jan";	// sanity
}




static const int numNetSources = 3;	// now hard-entered in StringEd file
static const char *GetNetSourceString(int iSource)
{
	const char *p = GetCRDelineatedString("MP_INGAME","NET_SOURCES", iSource);

	return p ? p : "??";
}




void AssetCache() {
	int n;
	//if (Assets.textFont == NULL) {
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
//	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
//	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	// Icons for various server settings.
//	uiInfo.uiDC.Assets.needPass = trap_R_RegisterShaderNoMip( "gfx/menus/needpass" );
//	uiInfo.uiDC.Assets.noForce = trap_R_RegisterShaderNoMip( "gfx/menus/noforce" );
//	uiInfo.uiDC.Assets.forceRestrict = trap_R_RegisterShaderNoMip( "gfx/menus/forcerestrict" );
//	uiInfo.uiDC.Assets.saberOnly = trap_R_RegisterShaderNoMip( "gfx/menus/saberonly" );
//	uiInfo.uiDC.Assets.trueJedi = trap_R_RegisterShaderNoMip( "gfx/menus/truejedi" );
							
	for( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}
}

void _UI_DrawSides(float x, float y, float w, float h, float size) {
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size) {
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  _UI_DrawTopBottom(x, y, width, height, size);
  _UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}

#include "../namespace_begin.h"
int MenuFontToHandle(int iMenuFont)
{
	switch (iMenuFont)
	{
		case 1: return uiInfo.uiDC.Assets.qhSmallFont;
		case 2: return uiInfo.uiDC.Assets.qhMediumFont;
		case 3: return uiInfo.uiDC.Assets.qhBigFont;
		case 4: return uiInfo.uiDC.Assets.qhSmall2Font;
	}

	return uiInfo.uiDC.Assets.qhMediumFont;	// 0;
}
#include "../namespace_end.h"

int Text_Width(const char *text, float scale, int iMenuFont) 
{	
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_StrLenPixels(text, iFontIndex, scale);
}

int Text_Height(const char *text, float scale, int iMenuFont) 
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_HeightPixels(iFontIndex, scale);
}

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style, int iMenuFont)
{
	int iStyleOR = 0;

	int iFontIndex = MenuFontToHandle(iMenuFont);
	//
	// kludge.. convert JK2 menu styles to SOF2 printstring ctrl codes...
	//	
	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = (int)STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = (int)STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow
	}

	trap_R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}


void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int iMenuFont) 
{
	Text_Paint(x, y, scale, color, text, 0, limit, style, iMenuFont);

	// now print the cursor as well...  (excuse the braces, it's for porting C++ to C)
	//
	{
		char sTemp[1024];
		int iCopyCount = limit ? min(strlen(text), limit) : strlen(text);
			iCopyCount = min(iCopyCount,cursorPos);
			iCopyCount = min(iCopyCount,sizeof(sTemp));

			// copy text into temp buffer for pixel measure...
			//			
			strncpy(sTemp,text,iCopyCount);
					sTemp[iCopyCount] = '\0';

			{
				int iFontIndex = MenuFontToHandle( iMenuFont );	
				int iNextXpos  = trap_R_Font_StrLenPixels(sTemp, iFontIndex, scale );

				Text_Paint(x+iNextXpos, y, scale, color, va("%c",cursor), 0, limit, style|ITEM_TEXTSTYLE_BLINK, iMenuFont);
			}
	}
}


// maxX param is initially an X limit, but is also used as feedback. 0 = text was clipped to fit within, else maxX = next pos
//
static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit, int iMenuFont) 
{
	// this is kinda dirty, but...
	//
	int iFontIndex = MenuFontToHandle(iMenuFont);
	
	//float fMax = *maxX;
	int iPixelLen = trap_R_Font_StrLenPixels(text, iFontIndex, scale);
	if (x + iPixelLen > *maxX)
	{
		// whole text won't fit, so we need to print just the amount that does...
		//  Ok, this is slow and tacky, but only called occasionally, and it works...
		//
		char sTemp[4096]={0};	// lazy assumption
		const char *psText = text;
		char *psOut = &sTemp[0];
		char *psOutLastGood = psOut;
		unsigned int uiLetter;

		while (*psText && (x + trap_R_Font_StrLenPixels(sTemp, iFontIndex, scale)<=*maxX) 
			   && psOut < &sTemp[sizeof(sTemp)-1]	// sanity
				)
		{
			int iAdvanceCount;
			psOutLastGood = psOut;
			
			uiLetter = trap_AnyLanguage_ReadCharFromString(psText, &iAdvanceCount, NULL);
			psText += iAdvanceCount;

			if (uiLetter > 255)
			{
				*psOut++ = uiLetter>>8;
				*psOut++ = uiLetter&0xFF;
			}
			else
			{
				*psOut++ = uiLetter&0xFF;
			}
		}
		*psOutLastGood = '\0';

		*maxX = 0;	// feedback
		Text_Paint(x, y, scale, color, sTemp, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
	else
	{
		// whole text fits fine, so print it all...
		//
		*maxX = x + iPixelLen;	// feedback the next position, as the caller expects		
		Text_Paint(x, y, scale, color, text, adjust, limit, ITEM_TEXTSTYLE_NORMAL, iMenuFont);
	}
}


void UI_ShowPostGame(qboolean newHigh) {
	trap_Cvar_Set ("cg_cameraOrbit", "0");
	trap_Cvar_Set("cg_thirdPerson", "0");
#ifdef _XBOX
	ClientManager::ActiveClient().cg_thirdPerson = 0;
#endif
	trap_Cvar_Set( "sv_killserver", "1" );
	uiInfo.soundHighScore = newHigh;
  _UI_SetActiveMenu(UIMENU_POSTGAME);
}
/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic(qhandle_t image, int w, int h) {
  int x, y;
  x = (SCREEN_WIDTH - w) / 2;
  y = (SCREEN_HEIGHT - h) / 2;
  UI_DrawHandlePic(x, y, w, h, image);
}

int frameCount = 0;
int startTime;

vmCvar_t	ui_rankChange;
static void UI_BuildPlayerList();
char parsedFPMessage[1024];

#include "../namespace_begin.h"
extern int FPMessageTime;
#include "../namespace_end.h"

void Text_PaintCenter(float x, float y, float scale, vec4_t color, const char *text, float adjust, int iMenuFont);

const char *UI_GetStringEdString(const char *refSection, const char *refName)
{
	static char text[1024]={0};

	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
}

#define	UI_FPS_FRAMES	4
void _UI_Refresh( int realtime )
{
	// Severy hackery in split screen. We only draw the UI once - and we always
	// do it on the second pass (when client 1 calls this function). But we need
	// to temporarily set the active client to be the one that opened the UI in
	// the first place.
	if (ClientManager::splitScreenMode && cls.state == CA_ACTIVE)
	{
		if (ClientManager::ActiveClientNum() == 0)
			return;

		ClientManager::ActivateClient( uiClientNum );
	}

	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	//if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	if( !com_dedicated->integer )
	{
		trap_G2API_SetTime(realtime, 0);
		trap_G2API_SetTime(realtime, 1);
	}
	//ghoul2 timer must be explicitly updated during ui rendering.

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}

	if (gDelayedPause > 0)
	{
		gDelayedPause--;
		if ( gDelayedPause <= 0 )
		{
			trap_Cvar_Set( "cl_paused", "1" );
		}
	}

	UI_UpdateCvars();

	if (Menu_Count() > 0) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
//		UI_DoServerRefresh();
		// refresh server status
//		UI_BuildServerStatus(qfalse);
		// refresh find player list
#ifndef _XBOX
		UI_BuildFindPlayerList(qfalse);
#endif
	} 
#ifndef _XBOX	
	// draw cursor
	UI_SetColor( NULL );
	if (Menu_Count() > 0) {
		UI_DrawHandlePic( uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 48, 48, uiInfo.uiDC.Assets.cursor);
	}
#endif

#ifndef NDEBUG
	if (uiInfo.uiDC.debug)
	{
		// cursor coordinates
		//FIXME
		//UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	if (ui_rankChange.integer)
	{
		FPMessageTime = realtime + 3000;

		if (!parsedFPMessage[0] /*&& uiMaxRank > ui_rankChange.integer*/)
		{
			const char *printMessage = UI_GetStringEdString("MP_INGAME", "SET_NEW_RANK");

			int i = 0;
			int p = 0;
			int linecount = 0;

			while (printMessage[i] && p < 1024)
			{
				parsedFPMessage[p] = printMessage[i];
				p++;
				i++;
				linecount++;

				if (linecount > 64 && printMessage[i] == ' ')
				{
					parsedFPMessage[p] = '\n';
					p++;
					linecount = 0;
				}
			}
			parsedFPMessage[p] = '\0';
		}

//JLF this line was commented out (it seems like a bad idea
		if (uiMaxRank > ui_rankChange.integer)
		{
			uiMaxRank = ui_rankChange.integer;
			uiForceRank = uiMaxRank;

			/*
			while (x < NUM_FORCE_POWERS)
			{
				//For now just go ahead and clear force powers upon rank change
				uiForcePowersRank[x] = 0;
				x++;
			}
			uiForcePowersRank[FP_LEVITATION] = 1;
			uiForceUsed = 0;
			*/

			//Use BG_LegalizedForcePowers and transfer the result into the UI force settings

//JLF NEW
			if (ClientManager::NumClients()==2)
			{
				ClientManager::ActivateClient(1);
				UI_UpdateForcePowers();
				UI_ReadLegalForce();
				UI_UpdateClientForcePowers(NULL);
				ClientManager::ActivateClient(0);
			}

//JLF NEW
			UI_UpdateForcePowers();

			UI_ReadLegalForce();
//JLF NEW
			UI_UpdateClientForcePowers(NULL);
			
		}

		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABER_OFFENSE] < 1)
		{
			uiForcePowersRank[FP_SABER_OFFENSE] = 1;
		}
		if (ui_freeSaber.integer && uiForcePowersRank[FP_SABER_DEFENSE] < 1)
		{
			uiForcePowersRank[FP_SABER_DEFENSE] = 1;
		}
		trap_Cvar_Set("ui_rankChange", "0");

		//remember to update the force power count after changing the max rank
		UpdateForceUsed();
	}

	if (ui_freeSaber.integer)
	{
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 0;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 0;
	}
	else
	{
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 1;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 1;
	}

	/*
	if (parsedFPMessage[0] && FPMessageTime > realtime)
	{
		vec4_t txtCol;
		int txtStyle = ITEM_TEXTSTYLE_SHADOWED;

		if ((FPMessageTime - realtime) < 2000)
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = (((float)FPMessageTime - (float)realtime)/2000);

			txtStyle = 0;
		}
		else
		{
			txtCol[0] = colorWhite[0];
			txtCol[1] = colorWhite[1];
			txtCol[2] = colorWhite[2];
			txtCol[3] = colorWhite[3];
		}

		Text_Paint(10, 0, 1, txtCol, parsedFPMessage, 0, 1024, txtStyle, FONT_MEDIUM);
	}
	*/
	//For now, don't bother.

	// Undo the split screen hackery from the top. If we're in split screen,
	// then we've stomped active client with the id of whoever opened the UI
	if (ClientManager::splitScreenMode && cls.state == CA_ACTIVE)
		ClientManager::ActivateClient( 1 );
}

/*
=================
_UI_Shutdown
=================
*/
#include "../namespace_begin.h"
void UI_CleanupGhoul2(void);
#include "../namespace_end.h"

void _UI_Shutdown( void ) {
//	trap_LAN_SaveCachedServers();
	UI_CleanupGhoul2();
}

char *defaultMenu = NULL;

char *GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
//	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return defaultMenu;
	}

	char *buf = (char *) Z_Malloc( len+1, TAG_TEMP_WORKSPACE, qfalse, 4 );
	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	//COM_Compress(buf);
  return buf;

}

qboolean Asset_Parse(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {

		memset(&token, 0, sizeof(pc_token_t));

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}			
			//trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.qhMediumFont = trap_R_RegisterFont(token.string);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmallFont = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "small2Font") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.smallFont);
			uiInfo.uiDC.Assets.qhSmall2Font = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			//trap_R_RegisterFont(token, pointSize, &uiInfo.uiDC.Assets.bigFont);
			uiInfo.uiDC.Assets.qhBigFont = trap_R_RegisterFont(token.string);
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) 
		{
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr))
			{
				Com_Printf(S_COLOR_YELLOW,"Bad 1st parameter for keyword 'cursor'");
				return qfalse;
			}
//			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		// gradientbar
//		if (Q_stricmp(token.string, "gradientbar") == 0) {
//			if (!trap_PC_ReadToken(handle, &token)) {
//				return qfalse;
//			}
//			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(token.string);
//			continue;
//		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( token.string );
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

		if (Q_stricmp(token.string, "moveRollSound") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.moveRollSound = trap_S_RegisterSound( token.string );
			}
			continue;
		}

		if (Q_stricmp(token.string, "moveJumpSound") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.moveJumpSound = trap_S_RegisterSound( token.string );
			}

			continue;
		}
		if (Q_stricmp(token.string, "datapadmoveSaberSound1") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound1 = trap_S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound2") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound2 = trap_S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound3") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound3 = trap_S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound4") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound4 = trap_S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound5") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound5 = trap_S_RegisterSound( token.string );
			}

			continue;
		}

		if (Q_stricmp(token.string, "datapadmoveSaberSound6") == 0) 
		{
			if (trap_PC_ReadToken(handle,&token))
			{
				uiInfo.uiDC.Assets.datapadmoveSaberSound6 = trap_S_RegisterSound( token.string );
			}

			continue;
		}


		// precaching various sound files used in the menus
		if (Q_stricmp(token.string, "precacheSound") == 0)
		{
			const char *tempStr;
			if (PC_Script_Parse(handle, &tempStr)) 
			{
				char *soundFile;
				do
				{
					soundFile = COM_ParseExt(&tempStr, qfalse);	
					if (soundFile[0] != 0 && soundFile[0] != ';') {
						trap_S_RegisterSound( soundFile);
					}
				} while (soundFile[0]);
			}
			continue;
		}
	}
	return qfalse;
}


void UI_Report() {
  String_Report();
  //Font_Report();

}

#include "../namespace_begin.h"
void UIC_SaveMenuFile( const char *filename, int menuLen );
bool UIC_LoadMenuFile( const char *filename, int menuLen );
#include "../namespace_end.h"

// Global flag - used by various parse routines to tell us that we can't
// make a UIC file, because the .menu parsing has side-effects that are
// too complicated:
bool gDoNotMakeUic;

void UI_ParseMenu(const char *menuFile) {
	// If this isn't a final build version, grab the size of the .menu file.
	// UIC_LoadMenuFile will verify that the one stored in the .uic matches.
	// Not as good as a CRC or timestamp, but it's all we've really got:
	int menuLen = 0;
#ifndef FINAL_BUILD
	menuLen = FS_ReadFile( menuFile, NULL );
#endif

	// First, if there's a uic file, load that instead! Woot Woot!
	char uicName[MAX_QPATH];
	COM_StripExtension( menuFile, uicName );
	void FS_ReplaceSeparators( char *path );
	FS_ReplaceSeparators( uicName );
	if( UIC_LoadMenuFile( va("%s.uic", uicName), menuLen ) )
		return;

	// Nope, do the old-fashioned thing
	int handle;
	pc_token_t token;

	// Need to know if menu had an assetGlobalDef - those can't be converted to UIC
	gDoNotMakeUic = false;
	int menusCreated = 0;

	//Com_Printf("Parsing menu file: %s\n", menuFile);

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		return;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				gDoNotMakeUic = true;	// Assetglobaldef registers various assets
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
			// Keep track, just in case someone puts multiple menuDefs in a single .menu
			menusCreated++;
		}
	}
	trap_PC_FreeSource(handle);

	// If there wasn't an assetGlobalDef, then we can build a compiled version
	// of this menu (including source menu file's length:
	if( !gDoNotMakeUic && (menusCreated == 1)  )
		UIC_SaveMenuFile( va("d:\\base\\%s.uic", uicName), menuLen );
}

qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;
    
		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu(token.string); 
	}
	return qfalse;
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t token;
	int handle;
	int start;

	start = trap_Milliseconds();

	trap_PC_LoadGlobalDefines ( "ui/jk2mp/menudef.h" );

	handle = trap_PC_LoadSource( menuFile );
	if (!handle) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		handle = trap_PC_LoadSource( "ui/jk2mpmenus.txt" );
		if (!handle) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ) );
		}
	}

	if (reset) {
		Menu_Reset();
	}

	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			break;
		if( token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

//	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

	trap_PC_FreeSource( handle );

	trap_PC_RemoveAllGlobalDefines ( );
}

void UI_Load() {
	char *menuSet;
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();

	if (menu && menu->window.name) {
		strcpy(lastName, menu->window.name);
	}
	else
	{
		lastName[0] = 0;
	}

	if (uiInfo.inGameLoad)
	{
		menuSet= "ui/jk2mpingame.txt";
	}
	else
	{
		menuSet= UI_Cvar_VariableString("ui_menuFilesMP");
	}
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jk2mpmenus.txt";
	}

	String_Init();

#ifdef PRE_RELEASE_TADEMO
	UI_ParseGameInfo("demogameinfo.txt");
#else
	UI_ParseGameInfo("ui/jk2mp/gameinfo.txt");
#endif
	UI_LoadArenas();
	UI_LoadBots();

	UI_LoadMenus(menuSet, qtrue);
	Menus_CloseAll();
	Menus_ActivateByName(lastName);

}

static const char *handicapValues[] = {"None","95","90","85","80","75","70","65","60","55","50","45","40","35","30","25","20","15","10","5",NULL};

static void UI_DrawHandicap(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
  int i, h;

  h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
  i = 20 - h / 5;

  Text_Paint(rect->x, rect->y, scale, color, handicapValues[i], 0, 0, textStyle, iMenuFont);
}

static void UI_DrawClanName(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
  Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_teamName"), 0, 0, textStyle, iMenuFont);
}


static void UI_SetCapFragLimits(qboolean uiVars) {
	int cap = 5;
	int frag = 10;

	if (uiVars) {
		trap_Cvar_Set("ui_captureLimit", va("%d", cap));
		trap_Cvar_Set("ui_fragLimit", va("%d", frag));
	} else {
		trap_Cvar_Set("capturelimit", va("%d", cap));
		trap_Cvar_Set("fraglimit", va("%d", frag));
	}
}

static const char* UI_GetGameTypeName(int gtEnum)
{
	switch ( gtEnum ) 
	{
	case GT_FFA:
		return UI_GetStringEdString("MENUS", "FREE_FOR_ALL");//"Free For All";
	case GT_HOLOCRON:
		return UI_GetStringEdString("MENUS", "HOLOCRON_FFA");//"Holocron FFA";
	case GT_JEDIMASTER:
		return UI_GetStringEdString("MENUS", "SAGA");//"Jedi Master";??
	case GT_SINGLE_PLAYER:
		return UI_GetStringEdString("MENUS", "SAGA");//"Team FFA";
	case GT_DUEL:
		return UI_GetStringEdString("MENUS", "DUEL");//"Team FFA";
	case GT_POWERDUEL:
		return UI_GetStringEdString("MENUS", "POWERDUEL");//"Team FFA";
	case GT_TEAM:
		return UI_GetStringEdString("MENUS", "TEAM_FFA");//"Team FFA";
	case GT_SIEGE:
		return UI_GetStringEdString("MENUS", "SIEGE");//"Siege";
	case GT_CTF:
		return UI_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");//"Capture the Flag";
	case GT_CTY:
		return UI_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");//"Capture the Ysalamiri";
	}
	return UI_GetStringEdString("MENUS", "SAGA");//"Team FFA";
}



// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) 
{
  Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.gameTypes[ui_gameType.integer].gtEnum), 0, 0, textStyle, iMenuFont);
}

static void UI_DrawNetGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) 
{
	if (ui_netGameType.integer < 0 || ui_netGameType.integer >= uiInfo.numGameTypes) 
	{
		trap_Cvar_Set("ui_netGameType", "0");
		trap_Cvar_Set("ui_actualNetGameType", "0");
	}
	Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.gameTypes[ui_netGameType.integer].gtEnum) , 0, 0, textStyle, iMenuFont);
}

static void UI_DrawAutoSwitch(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
#ifdef _XBOX
	int switchVal = ClientManager::ActiveClient().cg_autoswitch;
#else
	int switchVal = trap_Cvar_VariableValue("cg_autoswitch");
#endif //_XBOX
	const char *switchString = "AUTOSWITCH1";
	const char *stripString = NULL;

	switch(switchVal)
	{
	case 2:
		switchString = "AUTOSWITCH2";
		break;
	case 3:
		switchString = "AUTOSWITCH3";
		break;
	case 0:
		switchString = "AUTOSWITCH0";
		break;
	default:
		break;
	}

	stripString = UI_GetStringEdString("MP_INGAME", (char *)switchString);

	if (stripString)
	{
		Text_Paint(rect->x, rect->y, scale, color, stripString, 0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawJoinGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont)
{
	if (ui_joinGameType.integer < 0 || ui_joinGameType.integer > uiInfo.numJoinGameTypes)
	{
		trap_Cvar_Set("ui_joinGameType", "0");
	}

	Text_Paint(rect->x, rect->y, scale, color, UI_GetGameTypeName(uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum) , 0, 0, textStyle, iMenuFont);
}



static int UI_TeamIndexFromName(const char *name) {
  int i;

  if (name && *name) {
    for (i = 0; i < uiInfo.teamCount; i++) {
      if (Q_stricmp(name, uiInfo.teamList[i].teamName) == 0) {
        return i;
      }
    }
  } 

  return 0;

}

static void UI_DrawClanLogo(rectDef_t *rect, float scale, vec4_t color) {
  int i;
  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
  if (i >= 0 && i < uiInfo.teamCount) {
  	trap_R_SetColor( color );

		if (uiInfo.teamList[i].teamIcon == -1) {
      uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
      uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
      uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
		}

  	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
    trap_R_SetColor(NULL);
  }
}

static void UI_DrawSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
  int i;
	i = trap_Cvar_VariableValue( "g_spSkill" );
  if (i < 1 || i > numSkillLevels) {
    i = 1;
  }
  Text_Paint(rect->x, rect->y, scale, color, (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[i-1]),0, 0, textStyle, iMenuFont);
}


static void UI_DrawGenericNum(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int type,int iMenuFont) 
{
	int i;
	char s[256];

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	Com_sprintf(s, sizeof(s), "%i\0", val);
	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);
}

static void UI_DrawForceMastery(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char *s;

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	s = (char *)UI_GetStringEdString("MP_INGAME", forceMasteryLevels[val]);
	Text_Paint(rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont);
}


static void UI_DrawSkinColor(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char s[256];

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	switch(val)
	{
	case TEAM_RED:
		trap_SP_GetStringTextString("MENUS_TEAM_RED", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Red\0");
		break;
	case TEAM_BLUE:
		trap_SP_GetStringTextString("MENUS_TEAM_BLUE", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Blue\0");
		break;
	default:
		trap_SP_GetStringTextString("MENUS_DEFAULT", s, sizeof(s));
//		Com_sprintf(s, sizeof(s), "Default\0");
		break;
	}

	Text_Paint(rect->x, rect->y, scale, color, s, 0, 0, textStyle, iMenuFont);
}


//JLF
#ifdef _XBOX
static void UI_SwitchForceSide(int val, int min, int max)
{
	int i;
	char s[256];
	menuDef_t *menu;
	
	char info[MAX_INFO_VALUE];

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	/*
	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch(ClientManager::ActiveClient().myTeam)
		{
		case TEAM_RED:
			uiForceSide = FORCE_DARKSIDE;
			break;
		case TEAM_BLUE:
			uiForceSide = FORCE_LIGHTSIDE;
			break;
		default:
			break;
		}
	}
	*/


	if (val == FORCE_LIGHTSIDE)
	{
		trap_SP_GetStringTextString("MENUS_FORCEDESC_LIGHT",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));

		}
		menu = Menus_FindByName("ingame_playerforce");

		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}

		menu = Menus_FindByName("ingame_forcepoints");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}
	else
	{
		trap_SP_GetStringTextString("MENUS_FORCEDESC_DARK",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
		menu = Menus_FindByName("ingame_playerforce");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
		menu = Menus_FindByName("ingame_forcepoints");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}


}
#endif

static void UI_DrawForceSide(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char s[256];
	menuDef_t *menu;
	
	char info[MAX_INFO_VALUE];

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch(ClientManager::ActiveClient().myTeam)
		{
		case TEAM_RED:
			uiForceSide = FORCE_DARKSIDE;
			color[0] = 0.2;
			color[1] = 0.2;
			color[2] = 0.2;
			break;
		case TEAM_BLUE:
			uiForceSide = FORCE_LIGHTSIDE;
			color[0] = 0.2;
			color[1] = 0.2;
			color[2] = 0.2;
			break;
		default:
			break;
		}
	}

	if (val == FORCE_LIGHTSIDE)
	{
		trap_SP_GetStringTextString("MENUS_FORCEDESC_LIGHT",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));

		}
		menu = Menus_FindByName("ingame_playerforce");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers_team", qfalse);

			Menu_ShowItemByName(menu, "lightpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}
	else
	{
		trap_SP_GetStringTextString("MENUS_FORCEDESC_DARK",s, sizeof(s));
		menu = Menus_FindByName("forcealloc");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
		menu = Menus_FindByName("ingame_playerforce");
		if (menu)
		{
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers_team", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);

			Menu_ShowItemByName(menu, "darkpowers_team", qtrue);//(ui_gameType.integer >= GT_TEAM));
		}
	}


	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);

}

qboolean UI_HasSetSaberOnly( void )
{
	char	info[MAX_INFO_STRING];
	int i = 0;
	int wDisable = 0;
	int	gametype = 0;

	gametype = atoi(Info_ValueForKey(info, "g_gametype"));

	if ( gametype == GT_JEDIMASTER )
	{ //set to 0 
		return qfalse;
	}

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	if (gametype == GT_DUEL || gametype == GT_POWERDUEL)
	{
		wDisable = atoi(Info_ValueForKey(info, "g_duelWeaponDisable"));
	}
	else
	{
		wDisable = atoi(Info_ValueForKey(info, "g_weaponDisable"));
	}

	while (i < WP_NUM_WEAPONS)
	{
		if (!(wDisable & (1 << i)) &&
			i != WP_SABER && i != WP_NONE)
		{
			return qfalse;
		}

		i++;
	}

	return qtrue;
}

static qboolean UI_AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

qboolean UI_TrueJediEnabled( void )
{
	char	info[MAX_INFO_STRING];
	int		gametype = 0, disabledForce = 0, trueJedi = 0;
	qboolean saberOnly = qfalse, allForceDisabled = qfalse;

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
	//if (ui_forcePowerDisable.integer)
	//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
	disabledForce = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));
	allForceDisabled = UI_AllForceDisabled(disabledForce);
	gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	saberOnly = UI_HasSetSaberOnly();

	if ( gametype == GT_HOLOCRON 
		|| gametype == GT_JEDIMASTER 
		|| saberOnly 
		|| allForceDisabled )
	{
		trueJedi = 0;
	}
	else
	{
		trueJedi = atoi( Info_ValueForKey( info, "g_jediVmerc" ) );
	}
	return (trueJedi != 0);
}

static void UI_DrawJediNonJedi(rectDef_t *rect, float scale, vec4_t color, int textStyle, int val, int min, int max, int iMenuFont)
{
	int i;
	char s[256];
	//menuDef_t *menu;
	
	char info[MAX_INFO_VALUE];

	i = val;
	if (i < min || i > max) 
	{
		i = min;
	}

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if ( !UI_TrueJediEnabled() )
	{//true jedi mode is not on, do not draw this button type
		return;
	}

	if ( val == FORCE_NONJEDI )
	{
		trap_SP_GetStringTextString("MENUS_NO",s, sizeof(s));
	}
	else
	{
		trap_SP_GetStringTextString("MENUS_YES",s, sizeof(s));
	}

	Text_Paint(rect->x, rect->y, scale, color, s,0, 0, textStyle, iMenuFont);
}

static void UI_DrawTeamName(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int textStyle, int iMenuFont) {
  int i;
  i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));
  if (i >= 0 && i < uiInfo.teamCount) {
    Text_Paint(rect->x, rect->y, scale, color, va("%s: %s", (blue) ? "Blue" : "Red", uiInfo.teamList[i].teamName),0, 0, textStyle, iMenuFont);
  }
}

static void UI_DrawTeamMember(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int num, int textStyle, int iMenuFont) 
{
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = trap_Cvar_VariableValue(va(blue ? "ui_blueteam%i" : "ui_redteam%i", num));
	const char *text;
	int maxcl = trap_Cvar_VariableValue( "sv_maxClients" );
	vec4_t finalColor;
	int numval = num;

	numval *= 2;

	if (blue)
	{
		numval -= 1;
	}

	finalColor[0] = color[0];
	finalColor[1] = color[1];
	finalColor[2] = color[2];
	finalColor[3] = color[3];

	if (numval > maxcl)
	{
		finalColor[0] *= 0.5;
		finalColor[1] *= 0.5;
		finalColor[2] *= 0.5;

		value = -1;
	}

	if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_SIEGE)
	{
		if (value > 1 )
		{
			value = 1;
		}
	}

	if (value <= 1) {
		if (value == -1)
		{
			//text = "Closed";
			text = UI_GetStringEdString("MENUS", "CLOSED");
		}
		else
		{
			//text = "Human";
			text = UI_GetStringEdString("MENUS", "HUMAN");
		}
	} else {
		value -= 2;
		if (value >= UI_GetNumBots()) {
			value = 1;
		}
		text = UI_GetBotNameByNumber(value);
	}

  Text_Paint(rect->x, rect->y, scale, finalColor, text, 0, 0, textStyle, iMenuFont);
}

static void UI_DrawEffects(rectDef_t *rect, float scale, vec4_t color) 
{
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiSaberColorShaders[uiInfo.effectsColor]);
}

static void UI_DrawMapPreview(rectDef_t *rect, float scale, vec4_t color, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set("ui_currentNetMap", "0");
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set("ui_currentMap", "0");
		}
		map = 0;
	}

	if (uiInfo.mapList[map].levelShot == -1) {
		uiInfo.mapList[map].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[map].imageName);
	}

	if (uiInfo.mapList[map].levelShot > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("menu/art/unknownmap_mp"));
	}
}						 


static void UI_DrawMapTimeToBeat(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	int minutes, seconds, time;
	if (ui_currentMap.integer < 0 || ui_currentMap.integer > uiInfo.mapCount) {
		ui_currentMap.integer = 0;
		trap_Cvar_Set("ui_currentMap", "0");
	}

	time = uiInfo.mapList[ui_currentMap.integer].timeToBeat[uiInfo.gameTypes[ui_gameType.integer].gtEnum];

	minutes = time / 60;
	seconds = time % 60;

  Text_Paint(rect->x, rect->y, scale, color, va("%02i:%02i", minutes, seconds), 0, 0, textStyle, iMenuFont);
}

static void UI_SetForceDisabled(int force)
{
	int i = 0;

	if (force)
	{
		while (i < NUM_FORCE_POWERS)
		{
			if (force & (1 << i))
			{
				uiForcePowersDisabled[i] = qtrue;

				if (i != FP_LEVITATION && i != FP_SABER_OFFENSE && i != FP_SABER_DEFENSE)
				{
					uiForcePowersRank[i] = 0;
				}
				else
				{
					if (i == FP_LEVITATION)
					{
						uiForcePowersRank[i] = 1;
					}
					else
					{
						uiForcePowersRank[i] = 3;
					}
				}
			}
			else
			{
				uiForcePowersDisabled[i] = qfalse;
			}
			i++;
		}
	}
	else
	{
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			uiForcePowersDisabled[i] = qfalse;
			i++;
		}
	}
}
// The game type on create server has changed - make the HUMAN/BOTS fields active 
#ifndef _XBOX
void UpdateBotButtons(void)
{
	menuDef_t *menu;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return;
	}

	if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_SIEGE)
	{
		Menu_ShowItemByName(menu, "humanbotfield", qfalse);
		Menu_ShowItemByName(menu, "humanbotnonfield", qtrue);
	}
	else
	{
		Menu_ShowItemByName(menu, "humanbotfield", qtrue);
		Menu_ShowItemByName(menu, "humanbotnonfield", qfalse);
	}

}
#endif // _XBOX

void UpdateForceStatus()
{
	menuDef_t *menu;

	// Currently we don't make a distinction between those that wish to play Jedi of lower than maximum skill.
/*	if (ui_forcePowerDisable.integer)
	{
		uiForceRank = 0;
		uiForceAvailable = 0;
		uiForceUsed = 0;
	}
	else
	{
		uiForceRank = uiMaxRank;
		uiForceUsed = 0;
		uiForceAvailable = forceMasteryPoints[uiForceRank];
	}
*/
	
	menu = Menus_FindByName("ingame_player");
	if (menu)
	{
		char	info[MAX_INFO_STRING];
		int		disabledForce = 0;
		qboolean trueJedi = qfalse, allForceDisabled = qfalse;

		trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

		//already have serverinfo at this point for stuff below. Don't bother trying to use ui_forcePowerDisable.
		//if (ui_forcePowerDisable.integer)
		//if (atoi(Info_ValueForKey(info, "g_forcePowerDisable")))
		disabledForce = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));
		allForceDisabled = UI_AllForceDisabled(disabledForce);
		trueJedi = UI_TrueJediEnabled();

		if ( !trueJedi || allForceDisabled )
		{
			Menu_ShowItemByName(menu, "jedinonjedi", qfalse);
		}
		else
		{
			Menu_ShowItemByName(menu, "jedinonjedi", qtrue);
		}
		if ( allForceDisabled == qtrue || (trueJedi && uiJediNonJedi == FORCE_NONJEDI) )
		{	// No force stuff
			Menu_ShowItemByName(menu, "noforce", qtrue);
			Menu_ShowItemByName(menu, "yesforce", qfalse);
			// We don't want the saber explanation to say "configure saber attack 1" since we can't.
			Menu_ShowItemByName(menu, "sabernoneconfigme", qfalse);
		}
		else
		{
			UI_SetForceDisabled(disabledForce);
			Menu_ShowItemByName(menu, "noforce", qfalse);
			Menu_ShowItemByName(menu, "yesforce", qtrue);
		}

		//Moved this to happen after it's done with force power disabling stuff
		if (uiForcePowersRank[FP_SABER_OFFENSE] > 0 || ui_freeSaber.integer)
		{	// Show lightsaber stuff.
			Menu_ShowItemByName(menu, "nosaber", qfalse);
			Menu_ShowItemByName(menu, "yessaber", qtrue);
		}
		else
		{
			Menu_ShowItemByName(menu, "nosaber", qtrue);
			Menu_ShowItemByName(menu, "yessaber", qfalse);
		}

		// The leftmost button should be "apply" unless you are in spectator, where you can join any team.
		if (ClientManager::ActiveClient().myTeam != TEAM_SPECTATOR)
		{
			Menu_ShowItemByName(menu, "playerapply", qtrue);
			Menu_ShowItemByName(menu, "playerforcejoin", qfalse);
			Menu_ShowItemByName(menu, "playerforcered", qtrue);
			Menu_ShowItemByName(menu, "playerforceblue", qtrue);
			Menu_ShowItemByName(menu, "playerforcespectate", qtrue);
		}
		else
		{
			// Set or reset buttons based on choices
			if (atoi(Info_ValueForKey(info, "g_gametype")) >= GT_TEAM)
			{	// This is a team-based game.
				Menu_ShowItemByName(menu, "playerforcespectate", qtrue);
				
				// This is disabled, always show both sides from spectator.
				if ( 0 && atoi(Info_ValueForKey(info, "g_forceBasedTeams")))
				{	// Show red or blue based on what side is chosen.
					if (uiForceSide==FORCE_LIGHTSIDE)
					{
						Menu_ShowItemByName(menu, "playerforcered", qfalse);
						Menu_ShowItemByName(menu, "playerforceblue", qtrue);
					}
					else if (uiForceSide==FORCE_DARKSIDE)
					{
						Menu_ShowItemByName(menu, "playerforcered", qtrue);
						Menu_ShowItemByName(menu, "playerforceblue", qfalse);
					}
					else
					{
						Menu_ShowItemByName(menu, "playerforcered", qtrue);
						Menu_ShowItemByName(menu, "playerforceblue", qtrue);
					}
				}
				else
				{
					Menu_ShowItemByName(menu, "playerforcered", qtrue);
					Menu_ShowItemByName(menu, "playerforceblue", qtrue);
				}
			}
			else
			{
				Menu_ShowItemByName(menu, "playerforcered", qfalse);
				Menu_ShowItemByName(menu, "playerforceblue", qfalse);
			}

			Menu_ShowItemByName(menu, "playerapply", qfalse);
			Menu_ShowItemByName(menu, "playerforcejoin", qtrue);
			Menu_ShowItemByName(menu, "playerforcespectate", qtrue);
		}
	}


	if ( !UI_TrueJediEnabled() )
	{// Take the current team and force a skin color based on it.
		char	info[MAX_INFO_STRING];

		int team;
		char * progressionvalue = Cvar_VariableString("ui_menuProgression");

		if ((Q_stricmp(progressionvalue, "ingamemenu") == 0))  
			team = ClientManager::ActiveClient().myTeam;
		else
			team =(int)(trap_Cvar_VariableValue("xb_joinTeam"));
			
		switch(team )
		{
		case TEAM_RED:
			uiSkinColor = TEAM_RED;
			uiInfo.effectsColor = SABER_RED;
			break;
		case TEAM_BLUE:
			uiSkinColor = TEAM_BLUE;
			uiInfo.effectsColor = SABER_BLUE;
			break;
		default:
			trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

			if (atoi(Info_ValueForKey(info, "g_gametype")) >= GT_TEAM)
			{
				uiSkinColor = TEAM_FREE;
			}
			else	// A bit of a hack so non-team games will remember which skin set you chose in the player menu
			{
				uiSkinColor = uiHoldSkinColor;
			}
			break;
		}
	}
}


static const char *UI_EnglishMapName(const char *map) {
	int i;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (Q_stricmp(map, uiInfo.mapList[i].mapLoadName) == 0) {
			return uiInfo.mapList[i].mapName;
		}
	}
	return "";
}

static const char *UI_AIFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return uiInfo.aliasList[j].ai;
		}
	}
	return "Kyle";
}

static void UI_NextOpponent() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
  int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i++;
	if (i >= uiInfo.teamCount) {
		i = 0;
	}
	if (i == j) {
		i++;
		if ( i >= uiInfo.teamCount) {
			i = 0;
		}
	}
 	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void UI_PriorOpponent() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
  int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i--;
	if (i < 0) {
		i = uiInfo.teamCount - 1;
	}
	if (i == j) {
		i--;
		if ( i < 0) {
			i = uiInfo.teamCount - 1;
		}
	}
 	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void	UI_DrawPlayerLogo(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));

	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawPlayerLogoMetal(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawPlayerLogoName(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogo(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogoMetal(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogoName(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
 	trap_R_SetColor( NULL );
}

static void UI_DrawAllMapsSelection(rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net, int iMenuFont) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map >= 0 && map < uiInfo.mapCount) {
	  Text_Paint(rect->x, rect->y, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle, iMenuFont);
	}
}

static void UI_DrawOpponentName(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
  Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_opponentName"), 0, 0, textStyle, iMenuFont);
}

static int UI_OwnerDrawWidth(int ownerDraw, float scale) {
	int i, h, value, findex, iUse = 0;
	const char *text;
	const char *s = NULL;


  switch (ownerDraw) {
    case UI_HANDICAP:
			  h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
				i = 20 - h / 5;
				s = handicapValues[i];
      break;
    case UI_SKIN_COLOR:
		switch(uiSkinColor)
		{
		case TEAM_RED:
//			s = "Red";
			s = (char *)UI_GetStringEdString("MENUS", "TEAM_RED");
			break;
		case TEAM_BLUE:
//			s = "Blue";
			s = (char *)UI_GetStringEdString("MENUS", "TEAM_BLUE");
			break;
		default:
//			s = "Default";
			s = (char *)UI_GetStringEdString("MENUS", "DEFAULT");
			break;
		}
		break;
    case UI_FORCE_SIDE:
		i = uiForceSide;
		if (i < 1 || i > 2) {
			i = 1;
		}

		if (i == FORCE_LIGHTSIDE)
		{
//			s = "Light";
			s = (char *)UI_GetStringEdString("MENUS", "FORCEDESC_LIGHT");
		}
		else
		{
//			s = "Dark";
			s = (char *)UI_GetStringEdString("MENUS", "FORCEDESC_DARK");
		}
		break;
    case UI_JEDI_NONJEDI:
		i = uiJediNonJedi;
		if (i < 0 || i > 1)
		{
			i = 0;
		}

		if (i == FORCE_NONJEDI)
		{
//			s = "Non-Jedi";
			s = (char *)UI_GetStringEdString("MENUS", "NO");
		}
		else
		{
//			s = "Jedi";
			s = (char *)UI_GetStringEdString("MENUS", "YES");
		}
		break;
    case UI_FORCE_RANK:
		i = uiForceRank;
		if (i < 1 || i > MAX_FORCE_RANK) {
			i = 1;
		}

		s = (char *)UI_GetStringEdString("MP_INGAME", forceMasteryLevels[i]);
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		i = uiForcePowersRank[findex];

		if (i < 0 || i > NUM_FORCE_POWER_LEVELS-1)
		{
			i = 0;
		}

		s = va("%i", uiForcePowersRank[findex]);
		break;
    case UI_CLANNAME:
				s = UI_Cvar_VariableString("ui_teamName");
      break;
    case UI_GAMETYPE:
				s = uiInfo.gameTypes[ui_gameType.integer].gameType;
      break;
    case UI_SKILL:
				i = trap_Cvar_VariableValue( "g_spSkill" );
				if (i < 1 || i > numSkillLevels) {
					i = 1;
				}
			  s = (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[i-1]);
      break;
    case UI_BLUETEAMNAME:
			  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_blueTeam"));
			  if (i >= 0 && i < uiInfo.teamCount) {
			    s = va("%s: %s", (char *)UI_GetStringEdString("MENUS", "TEAM_BLUE"), uiInfo.teamList[i].teamName);
			  }
      break;
    case UI_REDTEAMNAME:
			  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_redTeam"));
			  if (i >= 0 && i < uiInfo.teamCount) {
			    s = va("%s: %s",  (char *)UI_GetStringEdString("MENUS", "TEAM_RED"), uiInfo.teamList[i].teamName);
			  }
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
			if (ownerDraw <= UI_BLUETEAM5)
			{
			  iUse = ownerDraw-UI_BLUETEAM1 + 1;
			}
			else
			{
			  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
			}

			value = trap_Cvar_VariableValue(va("ui_blueteam%i", iUse));
			if (value <= 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 1;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", iUse, text);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
			if (ownerDraw <= UI_REDTEAM5)
			{
			  iUse = ownerDraw-UI_REDTEAM1 + 1;
			}
			else
			{
			  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
			}

			value = trap_Cvar_VariableValue(va("ui_redteam%i", iUse));
			if (value <= 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 1;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", iUse, text);
      break;
		case UI_TIER:
			break;
		case UI_TIER_MAPNAME:
			break;
		case UI_TIER_GAMETYPE:
			break;
		case UI_ALLMAPS_SELECTION:
			break;
		case UI_OPPONENT_NAME:
			break;
    default:
      break;
  }

	if (s) {
		return Text_Width(s, scale, 0);
	}
	return 0;
}

static void UI_DrawBotName(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont) 
{
	int value = uiInfo.botIndex;
	const char *text = "";
	if (value >= UI_GetNumBots()) {
		value = 0;
	}
	text = UI_GetBotNameByNumber(value);
	Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle,iMenuFont);
}

static void UI_DrawBotSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont) 
{
	if (uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels) 
	{
		Text_Paint(rect->x, rect->y, scale, color, (char *)UI_GetStringEdString("MP_INGAME", (char *)skillLevels[uiInfo.skillIndex]), 0, 0, textStyle,iMenuFont);
	}
}

static void UI_DrawRedBlue(rectDef_t *rect, float scale, vec4_t color, int textStyle,int iMenuFont) 
{
	Text_Paint(rect->x, rect->y, scale, color, (uiInfo.redBlue == 0) ? UI_GetStringEdString("MP_INGAME","RED") : UI_GetStringEdString("MP_INGAME","BLUE"), 0, 0, textStyle,iMenuFont);
}

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vec4_t color) {
 	trap_R_SetColor( color );
	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
 	trap_R_SetColor( NULL );
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList() {
	uiClientState_t	cs;
	int		n, count, team, team2, playerTeamNumber;
	char	info[MAX_INFO_STRING];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi(Info_ValueForKey(info, "tl"));
	team = atoi(Info_ValueForKey(info, "t"));
	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;
	for( n = 0; n < count; n++ ) {
		trap_GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if (info[0]) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
			Q_CleanStr( uiInfo.playerNames[uiInfo.playerCount] );
			if(uiInfo.playerCount == 0) {
				Cvar_Set("vote_cantkick", va(UI_GetStringEdString("MP_SVGAME", "CANT_KICK"), uiInfo.playerNames[0]));
			}
			uiInfo.playerIndexes[uiInfo.playerCount] = n;
			uiInfo.playerCount++;
			team2 = atoi(Info_ValueForKey(info, "t"));
			if (team2 == team && n != uiInfo.playerNumber) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
				Q_CleanStr( uiInfo.teamNames[uiInfo.myTeamCount] );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if (uiInfo.playerNumber == n) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	if (!uiInfo.teamLeader) {
		trap_Cvar_Set("cg_selectedPlayer", va("%d", playerTeamNumber));
	}

	n = trap_Cvar_VariableValue("cg_selectedPlayer");
	if (n < 0 || n > uiInfo.myTeamCount) {
		n = 0;
	}


	if (n < uiInfo.myTeamCount) {
		trap_Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[n]);
	}
	else
	{
		trap_Cvar_Set("cg_selectedPlayerName", "Everyone");
	}

	if (!team || team == TEAM_SPECTATOR || !uiInfo.teamLeader)
	{
		n = uiInfo.myTeamCount;
		trap_Cvar_Set("cg_selectedPlayer", va("%d", n));
		trap_Cvar_Set("cg_selectedPlayerName", "N/A");
	}
}


static void UI_DrawSelectedPlayer(rectDef_t *rect, float scale, vec4_t color, int textStyle, int iMenuFont) {
	if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
  Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("cg_selectedPlayerName"), 0, 0, textStyle, iMenuFont);
}

/*
=================
UI_Version
=================
*/
static void UI_Version(rectDef_t *rect, float scale, vec4_t color, int iMenuFont) 
{
	int width;
	
	width = uiInfo.uiDC.textWidth(Q3_VERSION, scale, iMenuFont);

	uiInfo.uiDC.drawText(rect->x - width, rect->y, scale, color, Q3_VERSION, 0, 0, 0, iMenuFont);
}

/*
=================
UI_OwnerDraw
=================
*/
// FIXME: table drive
//
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle,int iMenuFont) 
{
	rectDef_t rect;
	int findex;
	int drawRank = 0, iUse = 0;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

  switch (ownerDraw) 
  {
    case UI_HANDICAP:
      UI_DrawHandicap(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_SKIN_COLOR:
      UI_DrawSkinColor(&rect, scale, color, textStyle, uiSkinColor, TEAM_FREE, TEAM_BLUE, iMenuFont);
      break;
	case UI_FORCE_SIDE:
      UI_DrawForceSide(&rect, scale, color, textStyle, uiForceSide, 1, 2, iMenuFont);
      break;
	case UI_JEDI_NONJEDI:
      UI_DrawJediNonJedi(&rect, scale, color, textStyle, uiJediNonJedi, 0, 1, iMenuFont);
      break;
    case UI_FORCE_POINTS:
      UI_DrawGenericNum(&rect, scale, color, textStyle, uiForceAvailable, 1, forceMasteryPoints[MAX_FORCE_RANK], ownerDraw,iMenuFont);
      break;
	case UI_FORCE_MASTERY_SET:
      UI_DrawForceMastery(&rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont);
      break;
    case UI_FORCE_RANK:
      UI_DrawForceMastery(&rect, scale, color, textStyle, uiForceRank, 0, MAX_FORCE_RANK, iMenuFont);
      break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:

//		uiForceRank
/*
		uiForceUsed
		// Only fields for white stars
		if (uiForceUsed<3)
		{
		    Menu_ShowItemByName(menu, "lightpowers_team", qtrue);
		}
		else if (uiForceUsed<6)
		{
		    Menu_ShowItemByName(menu, "lightpowers_team", qtrue);
		}
*/

		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		if (uiForcePowerDarkLight[findex] && uiForceSide != uiForcePowerDarkLight[findex])
		{
			color[0] *= 0.5;
			color[1] *= 0.5;
			color[2] *= 0.5;
		}
/*		else if (uiForceRank < UI_ForceColorMinRank[bgForcePowerCost[findex][FORCE_LEVEL_1]])
		{
			color[0] *= 0.5;
			color[1] *= 0.5;
			color[2] *= 0.5;
		}
*/		drawRank = uiForcePowersRank[findex];

		UI_DrawForceStars(&rect, scale, color, textStyle, findex, drawRank, 0, NUM_FORCE_POWER_LEVELS-1);
		break;
    case UI_EFFECTS:
      UI_DrawEffects(&rect, scale, color);
      break;
    case UI_PLAYERMODEL:
      //UI_DrawPlayerModel(&rect);
      break;
    case UI_CLANNAME:
      UI_DrawClanName(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_CLANLOGO:
      UI_DrawClanLogo(&rect, scale, color);
      break;
    case UI_GAMETYPE:
      UI_DrawGameType(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_NETGAMETYPE:
      UI_DrawNetGameType(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_AUTOSWITCHLIST:
      UI_DrawAutoSwitch(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_MAPPREVIEW:
      UI_DrawMapPreview(&rect, scale, color, qtrue);
      break;
    case UI_MAP_TIMETOBEAT:
      UI_DrawMapTimeToBeat(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_SKILL:
      UI_DrawSkill(&rect, scale, color, textStyle, iMenuFont);
      break;
    case UI_TOTALFORCESTARS:
//      UI_DrawTotalForceStars(&rect, scale, color, textStyle);
      break;
    case UI_BLUETEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qtrue, textStyle, iMenuFont);
      break;
    case UI_REDTEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qfalse, textStyle, iMenuFont);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
	if (ownerDraw <= UI_BLUETEAM5)
	{
	  iUse = ownerDraw-UI_BLUETEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_DrawTeamMember(&rect, scale, color, qtrue, iUse, textStyle, iMenuFont);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
	if (ownerDraw <= UI_REDTEAM5)
	{
	  iUse = ownerDraw-UI_REDTEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_DrawTeamMember(&rect, scale, color, qfalse, iUse, textStyle, iMenuFont);
      break;
	case UI_OPPONENTMODEL:
		//UI_DrawOpponent(&rect);
		break;
	case UI_PLAYERLOGO:
		UI_DrawPlayerLogo(&rect, color);
		break;
	case UI_PLAYERLOGO_METAL:
		UI_DrawPlayerLogoMetal(&rect, color);
		break;
	case UI_PLAYERLOGO_NAME:
		UI_DrawPlayerLogoName(&rect, color);
		break;
	case UI_OPPONENTLOGO:
		UI_DrawOpponentLogo(&rect, color);
		break;
	case UI_OPPONENTLOGO_METAL:
		UI_DrawOpponentLogoMetal(&rect, color);
		break;
	case UI_OPPONENTLOGO_NAME:
		UI_DrawOpponentLogoName(&rect, color);
		break;
	case UI_ALLMAPS_SELECTION:
		UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qtrue, iMenuFont);
		break;
	case UI_MAPS_SELECTION:
		UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qfalse, iMenuFont);
		break;
	case UI_OPPONENT_NAME:
		UI_DrawOpponentName(&rect, scale, color, textStyle, iMenuFont);
		break;
	case UI_BOTNAME:
		UI_DrawBotName(&rect, scale, color, textStyle,iMenuFont);
		break;
	case UI_BOTSKILL:
		UI_DrawBotSkill(&rect, scale, color, textStyle,iMenuFont);
		break;
	case UI_REDBLUE:
		UI_DrawRedBlue(&rect, scale, color, textStyle,iMenuFont);
		break;
	case UI_CROSSHAIR:
		UI_DrawCrosshair(&rect, scale, color);
		break;
	case UI_SELECTEDPLAYER:
		UI_DrawSelectedPlayer(&rect, scale, color, textStyle, iMenuFont);
		break;
	case UI_VERSION:
		UI_Version(&rect, scale, color, iMenuFont);
		break;

#ifdef _XBOX
	case UI_PLAYERKICK_DESC:
		UI_DrawPlayerKickDesc();
		break;
	case UI_VOTE_DESC:
		UI_DrawVoteDesc();
		break;
	case UI_VOTE_LISTENER:
		UI_DrawInvisibleVoteListener();
		break;
	case UI_SOFT_KEYBOARD:
		UI_SoftKeyboard_Draw();
		break;
	case UI_SOFT_KEYBOARD_ACCEPT:
		UI_SoftKeyboardAccept_Draw();
		break;
	case UI_SOFT_KEYBOARD_DELETE:
		UI_SoftKeyboardDelete_Draw();
		break;
#endif
	default:
		break;
  }

}

static qboolean UI_OwnerDrawVisible(int flags) {
	qboolean vis = qtrue;

	while (flags) {

		if (flags & UI_SHOW_FFA) {
			if (trap_Cvar_VariableValue("g_gametype") != GT_FFA && trap_Cvar_VariableValue("g_gametype") != GT_HOLOCRON && trap_Cvar_VariableValue("g_gametype") != GT_JEDIMASTER) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}

		if (flags & UI_SHOW_NOTFFA) {
			if (trap_Cvar_VariableValue("g_gametype") == GT_FFA || trap_Cvar_VariableValue("g_gametype") == GT_HOLOCRON || trap_Cvar_VariableValue("g_gametype") != GT_JEDIMASTER) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}

		if (flags & UI_SHOW_LEADER) {
			// these need to show when this client can give orders to a player or a group
			if (!uiInfo.teamLeader) {
				vis = qfalse;
			} else {
				// if showing yourself
				if (ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber) { 
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		} 
		if (flags & UI_SHOW_NOTLEADER) {
			// these need to show when this client is assigning their own status or they are NOT the leader
			if (uiInfo.teamLeader) {
				// if not showing yourself
				if (!(ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber)) { 
					vis = qfalse;
				}
				// these need to show when this client can give orders to a player or a group
			}
			flags &= ~UI_SHOW_NOTLEADER;
		} 
		if (flags & UI_SHOW_FAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer != AS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		} 
		if (flags & UI_SHOW_NOTFAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer == AS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		} 
		if (flags & UI_SHOW_ANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		} 
		if (flags & UI_SHOW_ANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		} 
		if (flags & UI_SHOW_NETANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		} 
		if (flags & UI_SHOW_NETANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		} 
		if (flags & UI_SHOW_NEWHIGHSCORE) {
			if (uiInfo.newHighScoreTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			} else {
				if (uiInfo.soundHighScore) {
					if (trap_Cvar_VariableValue("sv_killserver") == 0) {
						// wait on server to go down before playing sound
						//trap_S_StartLocalSound(uiInfo.newHighScoreSound, CHAN_ANNOUNCER);
						uiInfo.soundHighScore = qfalse;
					}
				}
			}
			flags &= ~UI_SHOW_NEWHIGHSCORE;
		} 
		if (flags & UI_SHOW_NEWBESTTIME) {
			if (uiInfo.newBestTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NEWBESTTIME;
		} 
		if (flags & UI_SHOW_DEMOAVAILABLE) {
			if (!uiInfo.demoAvailable) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		} else {
			flags = 0;
		}
	}
  return vis;
}

static qboolean UI_Handicap_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
    int h;
    h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
		if (key == A_MOUSE2) {
	    h -= 5;
		} else {
	    h += 5;
		}
    if (h > 100) {
      h = 5;
    } else if (h < 0) {
			h = 100;
		}
  	trap_Cvar_Set( "handicap", va( "%i", h) );
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_Effects_HandleKey(int flags, float *special, int key) {
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		
		if ( !UI_TrueJediEnabled() )
		{
			int team = ClientManager::ActiveClient().myTeam;
					
			if (team == TEAM_RED || team==TEAM_BLUE)
			{
				return qfalse;
			}
		}
				
		if (key == A_MOUSE2) {
			uiInfo.effectsColor--;
		} else {
			uiInfo.effectsColor++;
		}
		
		if( uiInfo.effectsColor > 5 ) {
			uiInfo.effectsColor = 0;
		} else if (uiInfo.effectsColor < 0) {
			uiInfo.effectsColor = 5;
		}
		
		trap_Cvar_SetValue( "color1", /*uitogamecode[uiInfo.effectsColor]*/uiInfo.effectsColor );
		return qtrue;
	}
	return qfalse;
}

#include "../namespace_begin.h"
extern void	Item_RunScript(itemDef_t *item, const char *s);		//from ui_shared;
#include "../namespace_end.h"

// For hot keys on the chat main menu.
static qboolean UI_Chat_Main_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "attack");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "defend");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "request");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "reply");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "spot");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "tactics");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Attack_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "att_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "att_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "att_03");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Defend_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "def_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "def_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "def_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "def_04");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Request_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "req_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "req_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "req_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "req_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "req_05");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "req_06");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Reply_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "rep_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "rep_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "rep_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "rep_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "rep_05");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Spot_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "spot_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "spot_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "spot_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "spot_04");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

// For hot keys on the chat main menu.
static qboolean UI_Chat_Tactical_HandleKey(int key) 
{
	menuDef_t *menu;
	itemDef_t *item;

	menu = Menu_GetFocused();	

	if (!menu)
	{
		return (qfalse);
	}

	if ((key == A_1) || ( key == A_PLING))
	{
		item = Menu_FindItemByName(menu, "tac_01");
	}
	else if ((key == A_2) || ( key == A_AT))
	{
		item = Menu_FindItemByName(menu, "tac_02");
	}
	else if ((key == A_3) || ( key == A_HASH))
	{
		item = Menu_FindItemByName(menu, "tac_03");
	}
	else if ((key == A_4) || ( key == A_STRING))
	{
		item = Menu_FindItemByName(menu, "tac_04");
	}
	else if ((key == A_5) || ( key == A_PERCENT))
	{
		item = Menu_FindItemByName(menu, "tac_05");
	}
	else if ((key == A_6) || ( key == A_CARET))
	{
		item = Menu_FindItemByName(menu, "tac_06");
	}
	else
	{
		return (qfalse);
	}

	if (item)
	{
	    Item_RunScript(item, item->action);
	}

	return (qtrue);
}

static qboolean UI_GameType_HandleKey(int flags, float *special, int key, qboolean resetMap) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int oldCount = UI_MapCountByGameType(qtrue);

		// hard coded mess here
		if (key == A_MOUSE2) {
			ui_gameType.integer--;
			if (ui_gameType.integer == 2) {
				ui_gameType.integer = 1;
			} else if (ui_gameType.integer < 2) {
				ui_gameType.integer = uiInfo.numGameTypes - 1;
			}
		} else {
			ui_gameType.integer++;
			if (ui_gameType.integer >= uiInfo.numGameTypes) {
				ui_gameType.integer = 1;
			} else if (ui_gameType.integer == 2) {
				ui_gameType.integer = 3;
			}
		}
    
		trap_Cvar_Set("ui_gameType", va("%d", ui_gameType.integer));
		UI_SetCapFragLimits(qtrue);
#ifndef _XBOX
		UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
#endif
		if (resetMap && oldCount != UI_MapCountByGameType(qtrue)) {
	  	trap_Cvar_Set( "ui_currentMap", "0");
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, NULL);
		}
    return qtrue;
  }
  return qfalse;
}

// If we're in the solo menu, don't let them see siege maps.
static qboolean UI_InSoloMenu( void )
{
	menuDef_t *menu;
	itemDef_t *item;
	char *name = "solo_gametypefield";

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return (qfalse);
	}

	item = Menu_FindItemByName(menu, name);
	if (item)
	{
		return qtrue;
	}

	return (qfalse);
}

static qboolean UI_NetGameType_HandleKey(int flags, float *special, int key) 
{
#ifdef _XBOX
  if (key == A_CURSOR_RIGHT || key == A_CURSOR_LEFT)
#else
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
#endif
  {

#ifdef _XBOX
	if (key == A_CURSOR_LEFT) 
#else
	if (key == A_MOUSE2) 
#endif
	{
		ui_netGameType.integer--;
		if (UI_InSoloMenu())
		{
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_SIEGE)
			{
				ui_netGameType.integer--;
			}
		}
	} 
	else 
	{
		ui_netGameType.integer++;
		if (UI_InSoloMenu())
		{
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_SIEGE)
			{
				ui_netGameType.integer++;
			}
		}
	}

    if (ui_netGameType.integer < 0) 
	{
		ui_netGameType.integer = uiInfo.numGameTypes - 1;
	} 
	else if (ui_netGameType.integer >= uiInfo.numGameTypes) 
	{
		ui_netGameType.integer = 0;
    } 

  	trap_Cvar_Set( "ui_netGameType", va("%d", ui_netGameType.integer));
  	trap_Cvar_Set( "ui_actualnetGameType", va("%d", uiInfo.gameTypes[ui_netGameType.integer].gtEnum));
  	trap_Cvar_Set( "ui_currentNetMap", "0");
	UI_MapCountByGameType(qfalse);
		Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, NULL);
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_AutoSwitch_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
#ifdef _XBOX
	  int switchVal = ClientManager::ActiveClient().cg_autoswitch;
#else
	  int switchVal = trap_Cvar_VariableValue("cg_autoswitch");
#endif //_XBOX

		if (key == A_MOUSE2) {
			switchVal--;
		} else {
			switchVal++;
		}

    if (switchVal < 0)
	{
		switchVal = 2;
	}
	else if (switchVal >= 3)
	{
      switchVal = 0;
    } 

#ifdef _XBOX
	ClientManager::ActiveClient().cg_autoswitch = switchVal;
#endif
  	trap_Cvar_Set( "cg_autoswitch", va("%i", switchVal));
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_Skill_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
  	int i = trap_Cvar_VariableValue( "g_spSkill" );

		if (key == A_MOUSE2) {
	    i--;
		} else {
	    i++;
		}

    if (i < 1) {
			i = numSkillLevels;
		} else if (i > numSkillLevels) {
      i = 1;
    }

    trap_Cvar_Set("g_spSkill", va("%i", i));
    return qtrue;
  }
  return qfalse;
}


static qboolean UI_TeamName_HandleKey(int flags, float *special, int key, qboolean blue) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
    int i;
    i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));

		if (key == A_MOUSE2) {
	    i--;
		} else {
	    i++;
		}

    if (i >= uiInfo.teamCount) {
      i = 0;
    } else if (i < 0) {
			i = uiInfo.teamCount - 1;
		}

    trap_Cvar_Set( (blue) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[i].teamName);

    return qtrue;
  }
  return qfalse;
}

static qboolean UI_TeamMember_HandleKey(int flags, float *special, int key, qboolean blue, int num) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va(blue ? "ui_blueteam%i" : "ui_redteam%i", num);
		int value = trap_Cvar_VariableValue(cvar);
		int maxcl = trap_Cvar_VariableValue( "sv_maxClients" );
		int numval = num;

		numval *= 2;

		if (blue)
		{
			numval -= 1;
		}

		if (numval > maxcl)
		{
			return qfalse;
		}

		if (value < 1)
		{
			value = 1;
		}

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		/*if (ui_actualNetGameType.integer >= GT_TEAM) {
			if (value >= uiInfo.characterCount + 2) {
				value = 0;
			} else if (value < 0) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {*/
			if (value >= UI_GetNumBots() + 2) {
				value = 1;
			} else if (value < 1) {
				value = UI_GetNumBots() + 2 - 1;
			}
		//}

		trap_Cvar_Set(cvar, va("%i", value));
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_OpponentName_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			UI_PriorOpponent();
		} else {
			UI_NextOpponent();
		}
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_BotName_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
//		int game = trap_Cvar_VariableValue("g_gametype");
		int value = uiInfo.botIndex;

		if (key == A_MOUSE2) {
			value--;
		} else {
			value++;
		}

		/*
		if (game >= GT_TEAM) {
			if (value >= uiInfo.characterCount + 2) {
				value = 0;
			} else if (value < 0) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {
		*/
			if (value >= UI_GetNumBots()/* + 2*/) {
				value = 0;
			} else if (value < 0) {
				value = UI_GetNumBots()/* + 2*/ - 1;
			}
		//}
		uiInfo.botIndex = value;
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_BotSkill_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			uiInfo.skillIndex--;
		} else {
			uiInfo.skillIndex++;
		}
		if (uiInfo.skillIndex >= numSkillLevels) {
			uiInfo.skillIndex = 0;
		} else if (uiInfo.skillIndex < 0) {
			uiInfo.skillIndex = numSkillLevels-1;
		}
    return qtrue;
  }
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		if (key == A_MOUSE2) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap_Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair)); 
		return qtrue;
	}
	return qfalse;
}



static qboolean UI_SelectedPlayer_HandleKey(int flags, float *special, int key) {
  if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) {
		int selected;

		UI_BuildPlayerList();
		if (!uiInfo.teamLeader) {
			return qfalse;
		}
		selected = trap_Cvar_VariableValue("cg_selectedPlayer");
		
		if (key == A_MOUSE2) {
			selected--;
		} else {
			selected++;
		}

		if (selected > uiInfo.myTeamCount) {
			selected = 0;
		} else if (selected < 0) {
			selected = uiInfo.myTeamCount;
		}

		if (selected == uiInfo.myTeamCount) {
		 	trap_Cvar_Set( "cg_selectedPlayerName", "Everyone");
		} else {
		 	trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[selected]);
		}
	 	trap_Cvar_Set( "cg_selectedPlayer", va("%d", selected));
	}
	return qfalse;
}

/*
static qboolean UI_VoiceChat_HandleKey(int flags, float *special, int key)
{

	qboolean ret = qfalse;

	switch(key)
	{
		case A_1:
		case A_KP_1:
			ret = qtrue;
			break;
		case A_2:
		case A_KP_2:
			ret = qtrue;
			break;

	}
  
	return ret;
}
*/


#ifdef _XBOX
static qboolean UI_XboxPasscode_HandleKey(int flags, float *special, int key)
{
	static BYTE passcode[XONLINE_PASSCODE_LENGTH];
	int passcodeState = trap_Cvar_VariableValue( "xb_passcodeState" );

	// If the user hasn't entered a full passcode yet
	if (passcodeState >= 0 && passcodeState <= 3)
	{
		switch (key)
		{
			// Undo our stupid UI joy2key mapping that was done in the input system
			case A_CURSOR_DOWN:
				passcode[passcodeState++] = XONLINE_PASSCODE_DPAD_DOWN; break;
			case A_CURSOR_LEFT:
				passcode[passcodeState++] = XONLINE_PASSCODE_DPAD_LEFT; break;
			case A_CURSOR_RIGHT:
				passcode[passcodeState++] = XONLINE_PASSCODE_DPAD_RIGHT; break;
			case A_CURSOR_UP:
				passcode[passcodeState++] = XONLINE_PASSCODE_DPAD_UP; break;
			case A_PAGE_UP:
				passcode[passcodeState++] = XONLINE_PASSCODE_GAMEPAD_LEFT_TRIGGER; break;
			case A_PAGE_DOWN:
				passcode[passcodeState++] = XONLINE_PASSCODE_GAMEPAD_RIGHT_TRIGGER; break;
			case A_DELETE:
				passcode[passcodeState++] = XONLINE_PASSCODE_GAMEPAD_X; break;
			case A_BACKSPACE:
				passcode[passcodeState++] = XONLINE_PASSCODE_GAMEPAD_Y; break;
			default:
				// No other button (including "A") does anything here
				return qfalse;
		}

		// User has incremented passcodeState - change the cvar, and we're done
		trap_Cvar_Set( "xb_passcodeState", va("%d", passcodeState) );
		return qtrue;
	}

	// If the user has a full passcode on screen:
	if (passcodeState == 4)
	{
		// Pressing "A" tests the code. Every other button does nothing
		if (key != A_MOUSE1)
			return qfalse;

		// Test the passcode
		XONLINE_USER *pUser = XBL_GetUserInfo( XBL_GetSelectedAccountIndex() );
		if (memcmp(pUser->passcode, passcode, sizeof(passcode)) == 0)
		{
			// Success - resume logging in
			Menus_CloseByName( "xbox_passcode" );
			XBL_Login( LOGIN_CONNECT );
			return qtrue;
		}
		else
		{
			// Wrong - set state to invalid - so menu changes
			trap_Cvar_Set( "xb_passcodeState", "5" );
			return qtrue;
		}
	}

	// If the user had already entered an invalid passcode
	if (passcodeState == 5)
	{
		// Pressing "A" brings them back to the beginning, to try again.
		// All other buttons do nothing
		if (key != A_MOUSE1)
			return qfalse;

		trap_Cvar_Set( "xb_passcodeState", "0" );
		return qtrue;
	}

	// No other state is valid!
	assert( 0 );
	return qfalse;
}
#endif


static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	int findex, iUse = 0;

  switch (ownerDraw) {
    case UI_HANDICAP:
      return UI_Handicap_HandleKey(flags, special, key);
      break;
    case UI_SKIN_COLOR:
      return UI_SkinColor_HandleKey(flags, special, key, uiSkinColor, TEAM_FREE, TEAM_BLUE, ownerDraw);
      break;
    case UI_FORCE_SIDE:
      return UI_ForceSide_HandleKey(flags, special, key, uiForceSide, 1, 2, ownerDraw);
      break;
    case UI_JEDI_NONJEDI:
      return UI_JediNonJedi_HandleKey(flags, special, key, uiJediNonJedi, 0, 1, ownerDraw);
      break;
	case UI_FORCE_MASTERY_SET:
      return UI_ForceMaxRank_HandleKey(flags, special, key, uiForceRank, 1, MAX_FORCE_RANK, ownerDraw);
      break;
    case UI_FORCE_RANK:
		break;		
	case UI_CHAT_MAIN:
		return UI_Chat_Main_HandleKey(key);
		break;
	case UI_CHAT_ATTACK:
		return UI_Chat_Attack_HandleKey(key);
		break;
	case UI_CHAT_DEFEND:
		return UI_Chat_Defend_HandleKey(key);
		break;
	case UI_CHAT_REQUEST:
		return UI_Chat_Request_HandleKey(key);
		break;
	case UI_CHAT_REPLY:
		return UI_Chat_Reply_HandleKey(key);
		break;
	case UI_CHAT_SPOT:
		return UI_Chat_Spot_HandleKey(key);
		break;
	case UI_CHAT_TACTICAL:
		return UI_Chat_Tactical_HandleKey(key);
		break;
	case UI_FORCE_RANK_HEAL:
	case UI_FORCE_RANK_LEVITATION:
	case UI_FORCE_RANK_SPEED:
	case UI_FORCE_RANK_PUSH:
	case UI_FORCE_RANK_PULL:
	case UI_FORCE_RANK_TELEPATHY:
	case UI_FORCE_RANK_GRIP:
	case UI_FORCE_RANK_LIGHTNING:
	case UI_FORCE_RANK_RAGE:
	case UI_FORCE_RANK_PROTECT:
	case UI_FORCE_RANK_ABSORB:
	case UI_FORCE_RANK_TEAM_HEAL:
	case UI_FORCE_RANK_TEAM_FORCE:
	case UI_FORCE_RANK_DRAIN:
	case UI_FORCE_RANK_SEE:
	case UI_FORCE_RANK_SABERATTACK:
	case UI_FORCE_RANK_SABERDEFEND:
	case UI_FORCE_RANK_SABERTHROW:
		findex = (ownerDraw - UI_FORCE_RANK)-1;
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		return UI_ForcePowerRank_HandleKey(flags, special, key, uiForcePowersRank[findex], 0, NUM_FORCE_POWER_LEVELS-1, ownerDraw);
		break;
    case UI_EFFECTS:
      return UI_Effects_HandleKey(flags, special, key);
      break;
    case UI_GAMETYPE:
      return UI_GameType_HandleKey(flags, special, key, qtrue);
      break;
    case UI_NETGAMETYPE:
      return UI_NetGameType_HandleKey(flags, special, key);
      break;
    case UI_AUTOSWITCHLIST:
      return UI_AutoSwitch_HandleKey(flags, special, key);
      break;
    case UI_SKILL:
      return UI_Skill_HandleKey(flags, special, key);
      break;
    case UI_BLUETEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qtrue);
      break;
    case UI_REDTEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qfalse);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
		case UI_BLUETEAM6:
		case UI_BLUETEAM7:
		case UI_BLUETEAM8:
	if (ownerDraw <= UI_BLUETEAM5)
	{
	  iUse = ownerDraw-UI_BLUETEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-274; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}

      UI_TeamMember_HandleKey(flags, special, key, qtrue, iUse);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
		case UI_REDTEAM6:
		case UI_REDTEAM7:
		case UI_REDTEAM8:
	if (ownerDraw <= UI_REDTEAM5)
	{
	  iUse = ownerDraw-UI_REDTEAM1 + 1;
	}
	else
	{
	  iUse = ownerDraw-277; //unpleasent hack because I don't want to move up all the UI_BLAHTEAM# defines
	}
      UI_TeamMember_HandleKey(flags, special, key, qfalse, iUse);
      break;
		case UI_OPPONENT_NAME:
			UI_OpponentName_HandleKey(flags, special, key);
			break;
		case UI_BOTNAME:
			return UI_BotName_HandleKey(flags, special, key);
			break;
		case UI_BOTSKILL:
			return UI_BotSkill_HandleKey(flags, special, key);
			break;
		case UI_REDBLUE:
			UI_RedBlue_HandleKey(flags, special, key);
			break;
		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey(flags, special, key);
			break;
		case UI_SELECTEDPLAYER:
			UI_SelectedPlayer_HandleKey(flags, special, key);
			break;
	//	case UI_VOICECHAT:
	//		UI_VoiceChat_HandleKey(flags, special, key);
	//		break;
#ifdef _XBOX
		case UI_XBOX_PASSCODE:
			UI_XboxPasscode_HandleKey(flags, special, key);
			break;
		case UI_SOFT_KEYBOARD:
			return UI_SoftKeyboard_HandleKey(flags, special, key);
			break;
		case UI_SOFT_KEYBOARD_DELETE:
			return UI_SoftKeyboardDelete_HandleKey(flags, special, key);
			break;
		case UI_SOFT_KEYBOARD_ACCEPT:
			return UI_SoftKeyboardAccept_HandleKey(flags, special, key);
			break;
#endif
    default:
      break;
  }

  return qfalse;
}


static float UI_GetValue(int ownerDraw) {
  return 0;
}

static qboolean UI_SetNextMap(int actual, int index) {
	int i;
	for (i = actual + 1; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
			return qtrue;
		}
	}
	return qfalse;
}


static void UI_StartSkirmish(qboolean next) {
	int i, k, g, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS];

	temp = trap_Cvar_VariableValue( "g_gametype" );
	trap_Cvar_Set("ui_gameType", va("%i", temp));

	if (next) {
		int actual;
		int index = trap_Cvar_VariableValue("ui_mapIndex");
	 	UI_MapCountByGameType(qtrue);
		UI_SelectedMap(index, &actual);
		if (UI_SetNextMap(actual, index)) {
		} else {
			UI_GameType_HandleKey(0, 0, A_MOUSE1, qfalse);
			UI_MapCountByGameType(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, "skirmish");
		}
	}

	g = uiInfo.gameTypes[ui_gameType.integer].gtEnum;
	trap_Cvar_SetValue( "g_gametype", g );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentMap.integer].mapLoadName) );
	skill = trap_Cvar_VariableValue( "g_spSkill" );
	trap_Cvar_Set("ui_scoreMap", uiInfo.mapList[ui_currentMap.integer].mapName);

	k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));

	trap_Cvar_Set("ui_singlePlayerActive", "1");

	// set up sp overrides, will be replaced on postgame
	temp = trap_Cvar_VariableValue( "capturelimit" );
	trap_Cvar_Set("ui_saveCaptureLimit", va("%i", temp));
	temp = trap_Cvar_VariableValue( "fraglimit" );
	trap_Cvar_Set("ui_saveFragLimit", va("%i", temp));
	temp = trap_Cvar_VariableValue( "duel_fraglimit" );
	trap_Cvar_Set("ui_saveDuelLimit", va("%i", temp));

	UI_SetCapFragLimits(qfalse);

	temp = trap_Cvar_VariableValue( "cg_drawTimer" );
	trap_Cvar_Set("ui_drawTimer", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_doWarmup" );
	trap_Cvar_Set("ui_doWarmup", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_friendlyFire" );
	trap_Cvar_Set("ui_friendlyFire", va("%i", temp));
	temp = trap_Cvar_VariableValue( "sv_maxClients" );
	trap_Cvar_Set("ui_maxClients", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_warmup" );
	trap_Cvar_Set("ui_Warmup", va("%i", temp));
	temp = trap_Cvar_VariableValue( "sv_pure" );
	trap_Cvar_Set("ui_pure", va("%i", temp));

	trap_Cvar_Set("cg_cameraOrbit", "0");
	trap_Cvar_Set("cg_thirdPerson", "0");
#ifdef _XBOX
	ClientManager::ActiveClient().cg_thirdPerson = 0;
#endif
	trap_Cvar_Set("cg_drawTimer", "1");
	trap_Cvar_Set("g_doWarmup", "1");
	trap_Cvar_Set("g_warmup", "15");
	trap_Cvar_Set("sv_pure", "0");
	trap_Cvar_Set("g_friendlyFire", "0");
	trap_Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
	trap_Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));

	if (trap_Cvar_VariableValue("ui_recordSPDemo")) {
		Com_sprintf(buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, g);
		trap_Cvar_Set("ui_recordSPDemoName", buff);
	}

	delay = 500;

	if (g == GT_DUEL || g == GT_POWERDUEL) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap_Cvar_Set("sv_maxClients", va("%d", temp));
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %f "", %i \n", uiInfo.mapList[ui_currentMap.integer].opponentName, skill, delay);
		trap_Cmd_ExecuteText( EXEC_APPEND, buff );
	} else if (g == GT_HOLOCRON || g == GT_JEDIMASTER) {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap_Cvar_Set("sv_maxClients", va("%d", temp));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_HOLOCRON) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers-1; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_HOLOCRON) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	} else {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap_Cvar_Set("sv_maxClients", va("%d", temp));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers-1; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	if (g >= GT_TEAM ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
}

static void UI_Update(const char *name) {
	int	val = trap_Cvar_VariableValue(name);

	if (Q_stricmp(name, "s_khz") == 0) 
	{
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		return;
	}

 	if (Q_stricmp(name, "ui_SetName") == 0) {
		trap_Cvar_Set( "name", UI_Cvar_VariableString("ui_Name"));
 	} else if (Q_stricmp(name, "ui_setRate") == 0) {
		float rate = trap_Cvar_VariableValue("rate");
		if (rate >= 5000) {
			trap_Cvar_Set("cl_maxpackets", "30");
			trap_Cvar_Set("cl_packetdup", "1");
		} else if (rate >= 4000) {
			trap_Cvar_Set("cl_maxpackets", "15");
			trap_Cvar_Set("cl_packetdup", "2");		// favor less prediction errors when there's packet loss
		} else {
			trap_Cvar_Set("cl_maxpackets", "15");
			trap_Cvar_Set("cl_packetdup", "1");		// favor lower bandwidth
		}
 	} 
	else if (Q_stricmp(name, "ui_GetName") == 0) 
	{
		trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString("name"));
	}
	else if (Q_stricmp(name, "ui_r_colorbits") == 0) 
	{
		switch (val) 
		{
			case 0:
				trap_Cvar_SetValue( "ui_r_depthbits", 0 );
				break;

			case 16:
				trap_Cvar_SetValue( "ui_r_depthbits", 16 );
				break;

			case 32:
				trap_Cvar_SetValue( "ui_r_depthbits", 24 );
				break;
		}
	} 
	else if (Q_stricmp(name, "ui_r_lodbias") == 0) 
	{
		switch (val) 
		{
			case 0:
				trap_Cvar_SetValue( "ui_r_subdivisions", 4 );
				break;
			case 1:
				trap_Cvar_SetValue( "ui_r_subdivisions", 12 );
				break;

			case 2:
				trap_Cvar_SetValue( "ui_r_subdivisions", 20 );
				break;
		}
	} 
	else if (Q_stricmp(name, "ui_r_glCustom") == 0) 
	{
		switch (val) 
		{
			case 0:	// high quality

				trap_Cvar_SetValue( "ui_r_fullScreen", 1 );
				trap_Cvar_SetValue( "ui_r_subdivisions", 4 );
				trap_Cvar_SetValue( "ui_r_lodbias", 0 );
				trap_Cvar_SetValue( "ui_r_colorbits", 32 );
				trap_Cvar_SetValue( "ui_r_depthbits", 24 );
				trap_Cvar_SetValue( "ui_r_picmip", 0 );
				trap_Cvar_SetValue( "ui_r_mode", 4 );
				trap_Cvar_SetValue( "ui_r_texturebits", 32 );
				trap_Cvar_SetValue( "ui_r_fastSky", 0 );
				trap_Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//trap_Cvar_SetValue( "ui_cg_shadows", 2 );//stencil
				trap_Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 1: // normal 
				trap_Cvar_SetValue( "ui_r_fullScreen", 1 );
				trap_Cvar_SetValue( "ui_r_subdivisions", 4 );
				trap_Cvar_SetValue( "ui_r_lodbias", 0 );
				trap_Cvar_SetValue( "ui_r_colorbits", 0 );
				trap_Cvar_SetValue( "ui_r_depthbits", 24 );
				trap_Cvar_SetValue( "ui_r_picmip", 1 );
				trap_Cvar_SetValue( "ui_r_mode", 3 );
				trap_Cvar_SetValue( "ui_r_texturebits", 0 );
				trap_Cvar_SetValue( "ui_r_fastSky", 0 );
				trap_Cvar_SetValue( "ui_r_inGameVideo", 1 );
				//trap_Cvar_SetValue( "ui_cg_shadows", 2 );
				trap_Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;

			case 2: // fast

				trap_Cvar_SetValue( "ui_r_fullScreen", 1 );
				trap_Cvar_SetValue( "ui_r_subdivisions", 12 );
				trap_Cvar_SetValue( "ui_r_lodbias", 1 );
				trap_Cvar_SetValue( "ui_r_colorbits", 0 );
				trap_Cvar_SetValue( "ui_r_depthbits", 0 );
				trap_Cvar_SetValue( "ui_r_picmip", 2 );
				trap_Cvar_SetValue( "ui_r_mode", 3 );
				trap_Cvar_SetValue( "ui_r_texturebits", 0 );
				trap_Cvar_SetValue( "ui_r_fastSky", 1 );
				trap_Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//trap_Cvar_SetValue( "ui_cg_shadows", 1 );
				trap_Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;

			case 3: // fastest

				trap_Cvar_SetValue( "ui_r_fullScreen", 1 );
				trap_Cvar_SetValue( "ui_r_subdivisions", 20 );
				trap_Cvar_SetValue( "ui_r_lodbias", 2 );
				trap_Cvar_SetValue( "ui_r_colorbits", 16 );
				trap_Cvar_SetValue( "ui_r_depthbits", 16 );
				trap_Cvar_SetValue( "ui_r_mode", 3 );
				trap_Cvar_SetValue( "ui_r_picmip", 3 );
				trap_Cvar_SetValue( "ui_r_texturebits", 16 );
				trap_Cvar_SetValue( "ui_r_fastSky", 1 );
				trap_Cvar_SetValue( "ui_r_inGameVideo", 0 );
				//trap_Cvar_SetValue( "ui_cg_shadows", 0 );
				trap_Cvar_Set( "ui_r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		}
	}
}

int gUISelectedMap = 0;

/*
===============
UI_DeferMenuScript

Return true if the menu script should be deferred for later
===============
*/
static qboolean UI_DeferMenuScript ( char **args )
{
	const char* name;

	// Whats the reason for being deferred?
	if (!String_Parse( (char**)args, &name)) 
	{
		return qfalse;
	}

	// Handle the custom cases
	if ( !Q_stricmp ( name, "VideoSetup" ) )
	{
		const char* warningMenuName;
		qboolean	deferred;

		// No warning menu specified
		if ( !String_Parse( (char**)args, &warningMenuName) )
		{
			return qfalse;
		}

		// Defer if the video options were modified
		deferred = trap_Cvar_VariableValue ( "ui_r_modified" ) ? qtrue : qfalse;

		if ( deferred )
		{
			// Open the warning menu
			Menus_OpenByName(warningMenuName);
		}

		return deferred;
	}
	else if ( !Q_stricmp ( name, "RulesBackout" ) )
	{
		qboolean deferred;
		
		deferred = trap_Cvar_VariableValue ( "ui_rules_backout" ) ? qtrue : qfalse ;

		trap_Cvar_Set ( "ui_rules_backout", "0" );

		return deferred;
	}

	return qfalse;
}

#ifdef _XBOX
static void AddUIClient(void)
{
	if (ClientManager::NumClients() == 1)
	{
		ClientManager::splitScreenMode = qtrue;
		ClientManager::AddClient();
		ClientManager::ActivateClient(1);
		ClientManager::SetMainClient(1);
		ClientManager::ClientForceInit(1);
	//	ClientManager::SetActiveController(1);

		// Restore this client's settings from the Settings struct
		Settings.SetAll();
	}
}

//extern void Client_Free(void);
//extern void CL_FreeKeyBindings(void);
void RemoveUIClient(void)
{
	if (ClientManager::NumClients() == 2)
	{
		//remove client if there are two

		ClientManager::ActivateClient(1);
		//Client_Free();
		//CL_FreeKeyBindings();

		ClientManager::splitScreenMode = qfalse;
		ClientManager::Resize(1);
		ClientManager::ActivateClient(0);
		ClientManager::SetMainClient(0);
	}
}
#endif // _XBOX

/*
=================
UI_UpdateVideoSetup

Copies the temporary user interface version of the video cvars into
their real counterparts.  This is to create a interface which allows 
you to discard your changes if you did something you didnt want
=================
*/
#ifndef _XBOX
void UI_UpdateVideoSetup ( void )
{
	trap_Cvar_Set ( "r_mode", UI_Cvar_VariableString ( "ui_r_mode" ) );
	trap_Cvar_Set ( "r_fullscreen", UI_Cvar_VariableString ( "ui_r_fullscreen" ) );
	trap_Cvar_Set ( "r_colorbits", UI_Cvar_VariableString ( "ui_r_colorbits" ) );
	trap_Cvar_Set ( "r_lodbias", UI_Cvar_VariableString ( "ui_r_lodbias" ) );
	trap_Cvar_Set ( "r_picmip", UI_Cvar_VariableString ( "ui_r_picmip" ) );
	trap_Cvar_Set ( "r_texturebits", UI_Cvar_VariableString ( "ui_r_texturebits" ) );
	trap_Cvar_Set ( "r_texturemode", UI_Cvar_VariableString ( "ui_r_texturemode" ) );
	trap_Cvar_Set ( "r_detailtextures", UI_Cvar_VariableString ( "ui_r_detailtextures" ) );
	trap_Cvar_Set ( "r_ext_compress_textures", UI_Cvar_VariableString ( "ui_r_ext_compress_textures" ) );
	trap_Cvar_Set ( "r_depthbits", UI_Cvar_VariableString ( "ui_r_depthbits" ) );
	trap_Cvar_Set ( "r_subdivisions", UI_Cvar_VariableString ( "ui_r_subdivisions" ) );
	trap_Cvar_Set ( "r_fastSky", UI_Cvar_VariableString ( "ui_r_fastSky" ) );
	trap_Cvar_Set ( "r_inGameVideo", UI_Cvar_VariableString ( "ui_r_inGameVideo" ) );
	trap_Cvar_Set ( "r_allowExtensions", UI_Cvar_VariableString ( "ui_r_allowExtensions" ) );
	trap_Cvar_Set ( "cg_shadows", UI_Cvar_VariableString ( "ui_cg_shadows" ) );
	trap_Cvar_Set ( "ui_r_modified", "0" );

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
}
#endif // _XBOX

/*
=================
UI_GetVideoSetup

Retrieves the current actual video settings into the temporary user
interface versions of the cvars.
=================
*/
#ifndef _XBOX
void UI_GetVideoSetup ( void )
{
	// Make sure the cvars are registered as read only.
	trap_Cvar_Register ( NULL, "ui_r_glCustom",				"4", CVAR_ROM|CVAR_INTERNAL|CVAR_ARCHIVE );

	trap_Cvar_Register ( NULL, "ui_r_mode",					"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_fullscreen",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_colorbits",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_lodbias",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_picmip",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_texturebits",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_texturemode",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_detailtextures",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_ext_compress_textures","0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_depthbits",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_subdivisions",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_fastSky",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_inGameVideo",			"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_allowExtensions",		"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_cg_shadows",				"0", CVAR_ROM|CVAR_INTERNAL );
	trap_Cvar_Register ( NULL, "ui_r_modified",				"0", CVAR_ROM|CVAR_INTERNAL );
	
	// Copy over the real video cvars into their temporary counterparts
	trap_Cvar_Set ( "ui_r_mode",		UI_Cvar_VariableString ( "r_mode" ) );
	trap_Cvar_Set ( "ui_r_colorbits",	UI_Cvar_VariableString ( "r_colorbits" ) );
	trap_Cvar_Set ( "ui_r_fullscreen",	UI_Cvar_VariableString ( "r_fullscreen" ) );
	trap_Cvar_Set ( "ui_r_lodbias",		UI_Cvar_VariableString ( "r_lodbias" ) );
	trap_Cvar_Set ( "ui_r_picmip",		UI_Cvar_VariableString ( "r_picmip" ) );
	trap_Cvar_Set ( "ui_r_texturebits", UI_Cvar_VariableString ( "r_texturebits" ) );
	trap_Cvar_Set ( "ui_r_texturemode", UI_Cvar_VariableString ( "r_texturemode" ) );
	trap_Cvar_Set ( "ui_r_detailtextures", UI_Cvar_VariableString ( "r_detailtextures" ) );
	trap_Cvar_Set ( "ui_r_ext_compress_textures", UI_Cvar_VariableString ( "r_ext_compress_textures" ) );
	trap_Cvar_Set ( "ui_r_depthbits", UI_Cvar_VariableString ( "r_depthbits" ) );
	trap_Cvar_Set ( "ui_r_subdivisions", UI_Cvar_VariableString ( "r_subdivisions" ) );
	trap_Cvar_Set ( "ui_r_fastSky", UI_Cvar_VariableString ( "r_fastSky" ) );
	trap_Cvar_Set ( "ui_r_inGameVideo", UI_Cvar_VariableString ( "r_inGameVideo" ) );
	trap_Cvar_Set ( "ui_r_allowExtensions", UI_Cvar_VariableString ( "r_allowExtensions" ) );
	trap_Cvar_Set ( "ui_cg_shadows", UI_Cvar_VariableString ( "cg_shadows" ) );
	trap_Cvar_Set ( "ui_r_modified", "0" );
}
#endif // _XBOX

// If the game type is siege, hide the addbot button. I would have done a cvar text on that item,
// but it already had one on it.
#ifndef _XBOX
static void UI_SetBotButton ( void )
{
	int gameType = trap_Cvar_VariableValue( "g_gametype" );
	int server;
	menuDef_t *menu;
	itemDef_t *item;
	char *name = "addBot";

	server = trap_Cvar_VariableValue( "sv_running" );

	// If in siege or a client, don't show add bot button
	if ((gameType==GT_SIEGE) || (server==0))	// If it's not siege, don't worry about it
	{
		menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

		if (!menu)
		{
			return;
		}

		item = Menu_FindItemByName(menu, name);
		if (item)
		{
			Menu_ShowItemByName(menu, name, qfalse);
		}
	}
}
#endif // _XBOX

// Update the model cvar and everything is good.
static void UI_UpdateCharacterCvars ( void )
{
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	trap_Cvar_VariableStringBuffer("ui_char_model", model, sizeof(model));
	trap_Cvar_VariableStringBuffer("ui_char_skin_head", head, sizeof(head));
	trap_Cvar_VariableStringBuffer("ui_char_skin_torso", torso, sizeof(torso));
	trap_Cvar_VariableStringBuffer("ui_char_skin_legs", legs, sizeof(legs));

	Com_sprintf( skin, sizeof( skin ), "%s/%s|%s|%s", 
										model, 
										head, 
										torso, 
										legs 
				);

	trap_Cvar_Set ( "model", skin );

	trap_Cvar_Set ( "char_color_red", UI_Cvar_VariableString ( "ui_char_color_red" ) );
	trap_Cvar_Set ( "char_color_green", UI_Cvar_VariableString ( "ui_char_color_green" ) );
	trap_Cvar_Set ( "char_color_blue", UI_Cvar_VariableString ( "ui_char_color_blue" ) );
	trap_Cvar_Set ( "ui_selectedModelIndex", "-1");

	strcpy(ClientManager::ActiveClient().model, skin);
	strcpy(ClientManager::ActiveClient().char_color_red, UI_Cvar_VariableString ( "ui_char_color_red" ));
	strcpy(ClientManager::ActiveClient().char_color_green, UI_Cvar_VariableString ( "ui_char_color_green" ));
	strcpy(ClientManager::ActiveClient().char_color_blue, UI_Cvar_VariableString ( "ui_char_color_blue" ));




}

static void UI_GetCharacterCvars ( void )
{
	char *model;
	char *skin;
	int i;

	trap_Cvar_Set ( "ui_char_color_red", UI_Cvar_VariableString ( "char_color_red" ) );
	trap_Cvar_Set ( "ui_char_color_green", UI_Cvar_VariableString ( "char_color_green" ) );
	trap_Cvar_Set ( "ui_char_color_blue", UI_Cvar_VariableString ( "char_color_blue" ) );

	model = UI_Cvar_VariableString ( "model" );

//NEW CLIENTDATA CODE

	trap_Cvar_Set ( "ui_char_color_red", ClientManager::ActiveClient().char_color_red );
	trap_Cvar_Set ( "ui_char_color_green", ClientManager::ActiveClient().char_color_green );
	trap_Cvar_Set ( "ui_char_color_blue", ClientManager::ActiveClient().char_color_blue );

	model = ClientManager::ActiveClient().model;
//END

	skin = strrchr(model,'/');
	if (skin && strchr(model,'|'))	//we have a multipart custom jedi
	{
		char skinhead[MAX_QPATH];
		char skintorso[MAX_QPATH];
		char skinlower[MAX_QPATH];
		char *p2;

		*skin=0;
		skin++;
		//now get the the individual files

		//advance to second
		p2 = strchr(skin, '|'); 
		assert(p2);
		*p2=0;
		p2++;
		strcpy (skinhead, skin);


		//advance to third
		skin = strchr(p2, '|');
		assert(skin);
		*skin=0;
		skin++;
		strcpy (skintorso,p2);

		strcpy (skinlower,skin);



		trap_Cvar_Set("ui_char_model", model);
		trap_Cvar_Set("ui_char_skin_head", skinhead);
		trap_Cvar_Set("ui_char_skin_torso", skintorso);
		trap_Cvar_Set("ui_char_skin_legs", skinlower);

		for (i = 0; i < uiInfo.playerSpeciesCount; i++)
		{
			if ( !stricmp(model, uiInfo.playerSpecies[i].Name) )
			{
				uiInfo.playerSpeciesIndex = i;
				break;
			}
		}
	}
	else
	{
		model = UI_Cvar_VariableString ( "ui_char_model" );
		for (i = 0; i < uiInfo.playerSpeciesCount; i++)
		{
			if ( !stricmp(model, uiInfo.playerSpecies[i].Name) )
			{
				uiInfo.playerSpeciesIndex = i;
				return;	//FOUND IT, don't fall through
			}
		}
		//nope, didn't find it.
		uiInfo.playerSpeciesIndex = 0;//jic
		trap_Cvar_Set("ui_char_model", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name);
		trap_Cvar_Set("ui_char_skin_head", "head_a1");
		trap_Cvar_Set("ui_char_skin_torso","torso_a1");
		trap_Cvar_Set("ui_char_skin_legs", "lower_a1");
	}
}

#ifdef	_XBOX
#define	SIEGE_OBJ_XOFFSET		178.0f
#define	SIEGE_OBJ_YOFFSET		82.0f
#define	SIEGE_OBJ_MULT			0.8f
#endif //_XBOX

void UI_SetSiegeObjectiveGraphicPos(menuDef_t *menu,const char *itemName,const char *cvarName)
{
	itemDef_t	*item;
	char		cvarBuf[1024];
	const char	*holdVal;
	char		*holdBuf;

	item = Menu_FindItemByName(menu, itemName);

	if (item)
	{
		// get cvar data
		trap_Cvar_VariableStringBuffer(cvarName, cvarBuf, sizeof(cvarBuf));
		
		holdBuf = cvarBuf;
		if (String_Parse(&holdBuf,&holdVal))
		{
			item->window.rectClient.x = atof(holdVal);
			if (String_Parse(&holdBuf,&holdVal))
			{
				item->window.rectClient.y = atof(holdVal);
				if (String_Parse(&holdBuf,&holdVal))
				{
					item->window.rectClient.w = atof(holdVal);
					if (String_Parse(&holdBuf,&holdVal))
					{
						item->window.rectClient.h = atof(holdVal);

#ifdef _XBOX
						item->window.rectClient.x += SIEGE_OBJ_XOFFSET;
						item->window.rectClient.y += SIEGE_OBJ_YOFFSET;
						item->window.rectClient.w *= SIEGE_OBJ_MULT;
						item->window.rectClient.h *= SIEGE_OBJ_MULT;

						item->window.rect.x = item->window.rectClient.x;
						item->window.rect.y = item->window.rectClient.y;

						item->window.rect.w = item->window.rectClient.w;
						item->window.rect.h = item->window.rectClient.h;
#else
						item->window.rect.x = item->window.rectClient.x;
						item->window.rect.y = item->window.rectClient.y;

						item->window.rect.w = item->window.rectClient.w;
						item->window.rect.h = item->window.rectClient.h;
#endif //_XBOX
					}
				}
			}
		}
	}
}

void UI_FindCurrentSiegeTeamClass( void )
{
	menuDef_t *menu;
	int myTeam = ClientManager::ActiveClient().myTeam;
	char *itemname;
	itemDef_t *item;
	int	baseClass;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	if (( myTeam != TEAM_RED ) && ( myTeam != TEAM_BLUE ))
	{
		return;
	}

	// If the player is on a team, 
	if ( myTeam == TEAM_RED )
	{			
		itemDef_t *item;
		item = (itemDef_t *) Menu_FindItemByName(menu, "onteam1" );
		if (item)
		{
		    Item_RunScript(item, item->action);
		}
	}
	else if ( myTeam == TEAM_BLUE )
	{			
		itemDef_t *item;
		item = (itemDef_t *) Menu_FindItemByName(menu, "onteam2" );
		if (item)
		{
		    Item_RunScript(item, item->action);
		}
	}	


	baseClass = (int)trap_Cvar_VariableValue("ui_siege_class");

	// Find correct class button and activate it.
	if (baseClass == SPC_INFANTRY)
	{
		itemname = "class1_button";
	}
	else if (baseClass == SPC_HEAVY_WEAPONS)
	{
		itemname = "class2_button";
	}
	else if (baseClass == SPC_DEMOLITIONIST)
	{
		itemname = "class3_button";
	}
	else if (baseClass == SPC_VANGUARD)
	{
		itemname = "class4_button";
	}
	else if (baseClass == SPC_SUPPORT)
	{
		itemname = "class5_button";
	}
	else if (baseClass == SPC_SUPPORT)
	{
		itemname = "class5_button";
	}
	else if (baseClass == SPC_JEDI)
	{
		itemname = "class6_button";
	}
	else 
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, itemname );
	if (item)
	{
		Item_RunScript(item, item->action);
	}

}

void UI_UpdateSiegeObjectiveGraphics( void )
{
	menuDef_t *menu;
	int	teamI,objI;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	// Hiding a bunch of fields because the opening section of the siege menu was getting too long
	Menu_ShowGroup(menu,"class_button",qfalse);
	Menu_ShowGroup(menu,"class_count",qfalse);
	Menu_ShowGroup(menu,"feeders",qfalse);
	Menu_ShowGroup(menu,"classdescription",qfalse);
	Menu_ShowGroup(menu,"minidesc",qfalse);
	Menu_ShowGroup(menu,"obj_longdesc",qfalse);
	Menu_ShowGroup(menu,"objective_pic",qfalse);
	Menu_ShowGroup(menu,"stats",qfalse);
	Menu_ShowGroup(menu,"forcepowerlevel",qfalse);

	// Get objective icons for each team
	for (teamI=1;teamI<3;teamI++)
	{
		for (objI=1;objI<8;objI++)
		{
			Menu_SetItemBackground(menu,va("tm%i_icon%i",teamI,objI),va("*team%i_objective%i_mapicon",teamI,objI));
			Menu_SetItemBackground(menu,va("tm%i_l_icon%i",teamI,objI),va("*team%i_objective%i_mapicon",teamI,objI));
		}
	}

	// Now get their placement on the map
	for (teamI=1;teamI<3;teamI++)
	{
		for (objI=1;objI<8;objI++)
		{
			UI_SetSiegeObjectiveGraphicPos(menu,va("tm%i_icon%i",teamI,objI),va("team%i_objective%i_mappos",teamI,objI));
		}
	}

}

saber_colors_t TranslateSaberColor( const char *name );

static void UI_UpdateSaberCvars ( int playerNum )
{
	saber_colors_t colorI;

	trap_Cvar_Set ( "saber1", UI_Cvar_VariableString ( "ui_saber" ) );
	trap_Cvar_Set ( "saber2", UI_Cvar_VariableString ( "ui_saber2" ) );

	colorI = TranslateSaberColor( UI_Cvar_VariableString ( "ui_saber_color" ) );
	trap_Cvar_Set ( "color1", va("%d",colorI));
	trap_Cvar_Set ( "g_saber_color", UI_Cvar_VariableString ( "ui_saber_color" ));

	strcpy(ClientManager::ActiveClient().color1, va("%d",colorI));

	colorI = TranslateSaberColor( UI_Cvar_VariableString ( "ui_saber2_color" ) );
	trap_Cvar_Set ( "color2", va("%d",colorI) );
	trap_Cvar_Set ( "g_saber2_color", UI_Cvar_VariableString ( "ui_saber2_color" ));

	strcpy(ClientManager::ActiveClient().color2, va("%d",colorI));

	strcpy(ClientManager::ActiveClient().saber1, UI_Cvar_VariableString ( "ui_saber" ));
	strcpy(ClientManager::ActiveClient().saber2, UI_Cvar_VariableString ( "ui_saber2" ));

	strcpy(ClientManager::ActiveClient().saber_color1, UI_Cvar_VariableString ( "ui_saber_color" ));
	strcpy(ClientManager::ActiveClient().saber_color2, UI_Cvar_VariableString ( "ui_saber2_color" ));

	ClientManager::ActiveClient().cvar_modifiedFlags |= CVAR_USERINFO;

}

// More hard coded goodness for the menus.
static void UI_SetSaberBoxesandHilts (void)
{
	menuDef_t *menu;
	itemDef_t *item;
	qboolean	getBig = qfalse;
	char sType[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return;
	}

	trap_Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "dual", sType ) != 0 )
	{
//		trap_Cvar_Set("ui_saber", "single_1");
//		trap_Cvar_Set("ui_saber2", "single_1");
		getBig = qtrue;
	}

	else if (Q_stricmp( "staff", sType ) != 0 )
	{
//		trap_Cvar_Set("ui_saber", "dual_1");
//		trap_Cvar_Set("ui_saber2", "none");
		getBig = qtrue;
	}

	if (!getBig)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box2middle" );

	if(item)
	{
		item->window.rect.x = 212;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box2bottom" );

	if(item)
	{
		item->window.rect.x = 212;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box3middle" );

	if(item)
	{
		item->window.rect.x = 418;
		item->window.rect.y = 126;
		item->window.rect.w = 219;
		item->window.rect.h = 44;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "box3bottom" );

	if(item)
	{
		item->window.rect.x = 418;
		item->window.rect.y = 170;
		item->window.rect.w = 219;
		item->window.rect.h = 60;
	}
}

//extern qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel );
extern qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin );
#include "../namespace_begin.h"
extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name,int *runTimeLength );
extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );
#include "../namespace_end.h"

static void UI_UpdateSaberType( void )
{
	char sType[MAX_QPATH];
	trap_Cvar_VariableStringBuffer( "ui_saber_type", sType, sizeof(sType) );

	if ( Q_stricmp( "single", sType ) == 0 ||
		Q_stricmp( "staff", sType ) == 0 )
	{
		trap_Cvar_Set( "ui_saber2", "none" );
	}
}

static void UI_UpdateSaberHilt( qboolean secondSaber )
{
	menuDef_t *menu;
	itemDef_t *item;
	char model[MAX_QPATH];
	char modelPath[MAX_QPATH];
	char skinPath[MAX_QPATH];
	char *itemName;
	char *saberCvarName;
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu (either video or ingame video, I would assume)

	if (!menu)
	{
		return;
	}

	if ( secondSaber )
	{
		itemName = "saber2";
		saberCvarName = "ui_saber2";
	}
	else
	{
		itemName = "saber";
		saberCvarName = "ui_saber";
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, itemName );

	if(!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateSaberHilt: Could not find item (%s) in menu (%s)", itemName, menu->window.name);
	}

	trap_Cvar_VariableStringBuffer( saberCvarName, model, sizeof(model) );

	item->text = model;
	//read this from the sabers.cfg
	if ( UI_SaberModelForSaber( model, modelPath ) )
	{//successfully found a model
		ItemParse_asset_model_go( item, modelPath, &animRunLength );//set the model
		//get the customSkin, if any
		//COM_StripExtension( modelPath, skinPath );
		//COM_DefaultExtension( skinPath, sizeof( skinPath ), ".skin" );
		if ( UI_SaberSkinForSaber( model, skinPath ) )
		{
			ItemParse_model_g2skin_go( item, skinPath );//apply the skin
		}
		else
		{
			ItemParse_model_g2skin_go( item, NULL );//apply the skin
		}
	}
}

static void UI_UpdateSaberColor( qboolean secondSaber )
{
}

extern char * SaberColorToString(saber_colors_t color);

static void UI_GetSaberCvars ( void )
{
//	trap_Cvar_Set ( "ui_saber_type", UI_Cvar_VariableString ( "g_saber_type" ) );
	trap_Cvar_Set ( "ui_saber", UI_Cvar_VariableString ( "saber1" ) );
	trap_Cvar_Set ( "ui_saber2", UI_Cvar_VariableString ( "saber2" ));

	trap_Cvar_Set("g_saber_color", SaberColorToString(trap_Cvar_VariableValue("color1")));
	trap_Cvar_Set("g_saber2_color", SaberColorToString(trap_Cvar_VariableValue("color2")));

	trap_Cvar_Set ( "ui_saber_color", UI_Cvar_VariableString ( "g_saber_color" ) );
	trap_Cvar_Set ( "ui_saber2_color", UI_Cvar_VariableString ( "g_saber2_color" ) );

//JLF NEW CLIENTDATA ACCESS
	trap_Cvar_Set ( "ui_saber", ClientManager::ActiveClient().saber1);
	trap_Cvar_Set ( "ui_saber2", ClientManager::ActiveClient().saber2);

    
	trap_Cvar_Set("g_saber_color", SaberColorToString(atoi(ClientManager::ActiveClient().color1)));
	trap_Cvar_Set("g_saber2_color",SaberColorToString(atoi(ClientManager::ActiveClient().color2)));

	trap_Cvar_Set ( "ui_saber_color", UI_Cvar_VariableString ( "g_saber_color" ) );
	trap_Cvar_Set ( "ui_saber2_color", UI_Cvar_VariableString ( "g_saber2_color" ) );

}


//extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );
#include "../namespace_begin.h"
extern qboolean ItemParse_model_g2anim_go( itemDef_t *item, const char *animName );
#include "../namespace_end.h"
//extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name );

void UI_UpdateCharacterSkin( void )
{
	menuDef_t *menu;
	itemDef_t *item;
	char skin[MAX_QPATH];
	char model[MAX_QPATH];
	char head[MAX_QPATH];
	char torso[MAX_QPATH];
	char legs[MAX_QPATH];

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacterSkin: Could not find item (character) in menu (%s)", menu->window.name);
	}

	trap_Cvar_VariableStringBuffer("ui_char_model", model, sizeof(model));
	trap_Cvar_VariableStringBuffer("ui_char_skin_head", head, sizeof(head));
	trap_Cvar_VariableStringBuffer("ui_char_skin_torso", torso, sizeof(torso));
	trap_Cvar_VariableStringBuffer("ui_char_skin_legs", legs, sizeof(legs));

	Com_sprintf( skin, sizeof( skin ), "models/players/%s/|%s|%s|%s", 
										model, 
										head, 
										torso, 
										legs 
				);

	ModelMem.SetUISkin( skin );

	ItemParse_model_g2skin_go( item, skin );
}

static void UI_ResetCharacterListBoxes( void )
{

	itemDef_t *item;
	menuDef_t *menu;
	listBoxDef_t *listPtr;

	menu = Menu_GetFocused();

	if (menu)
	{
		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "headlistbox");
		if (item)
		{
			listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "torsolistbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "lowerlistbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}

		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "colorbox");
		if (item)
		{
			listPtr = (listBoxDef_t*)item->typeData;
			if( listPtr )
			{
				listPtr->cursorPos = 0;
			}
			item->cursorPos = 0;
		}
	}
}

//Rewritten to use ui_botInfos instead.  Cleaner and makes player names
//and bot names consistent.
const char *UI_ModelNameToPlayerName( const char *model )
{
	static char s_Name[64];
	char *p;

	// Make a copy first:
	Q_strncpyz( s_Name, model, 64 );

	// Remove the slash, and color/default crap:
	if( p = strstr(s_Name, "/"))
		* p = 0;

	// Special cases:
	if( Q_stricmp( s_Name, "alora2" ) == 0 )
		strcpy( s_Name, "Alora" );
	else if( Q_stricmp( s_Name, "tavion_new" ) == 0 )
		strcpy( s_Name, "Tavion" );
	else if( Q_stricmp( s_Name, "tavion" ) == 0 )
		strcpy( s_Name, "Tavion" );
	else if( Q_stricmp( s_Name, "lando" ) == 0 )
		strcpy( s_Name, "Lando" );
	else if( strstr( s_Name, "jedi_" ) )
		strcpy( s_Name, "Jaden" );
	else {
		//Standard case.  Loop through ui_botInfos and try to match the model.
		for(int i=0; i<UI_GetNumBots(); i++) {
			const char *info = Info_ValueForKey(UI_GetBotInfoByNumber(i), 
					"model");
			if(!Q_stricmp(info, s_Name)) {
				strcpy(s_Name, Info_ValueForKey(UI_GetBotInfoByNumber(i), 
							"name"));
				break;
			}
		}

		assert(i != UI_GetNumBots());
	}

	// Now, check against the rest of the player names on the server, until we find one we can use:
	uiClientState_t	cs;
	trap_GetClientState( &cs );
	char info[MAX_INFO_STRING];
	int myNum;
	for( myNum = 0; myNum < MAX_CLIENTS; myNum++ )
	{
		bool taken = false;
		char numberedName[64];
		strcpy( numberedName, s_Name );
		if( myNum )
			strcat( numberedName, va(" %d", myNum+1) );

		// Is this name taken?
		for( int i = 0; i < MAX_CLIENTS; ++i )
		{
			if( i == cs.clientNum )
				continue;

			trap_GetConfigString( CS_PLAYERS + i, info, MAX_INFO_STRING );
			if( !info[0] )
				continue;

			if( Q_stricmp( numberedName, Info_ValueForKey(info, "n") ) == 0 )
			{
				taken = true;
				break;
			}
		}

		// If no one else has this variation, let's use it
		if( !taken )
		{
			strcpy( s_Name, numberedName );
			return s_Name;
		}
	}

	// We should never get here
	assert( 0 );
	return "Padawan";
}

static void UI_SetPlayerName(void)
{
	// Set the player model Cvar to the UImodel Cvar value
//	trap_Cvar_Set( "model", Cvar_VariableString("UImodel"));
	if( Cvar_VariableValue( "xb_gameType" ) != 3 ) {
		extern        vmCvar_t        g_gametype;
		if(cgs.gametype == GT_SIEGE) {
			Q_strncpyz(ClientManager::ActiveClient().autoName, 
					UI_ModelNameToPlayerName(
						bgSiegeClasses[g_UIGloballySelectedSiegeClass].forcedModel),
					STRING_SIZE);
		} else {
			Q_strncpyz(ClientManager::ActiveClient().autoName, UI_ModelNameToPlayerName( ClientManager::ActiveClient().model), STRING_SIZE );
		}
	}

	ClientManager::ActiveClient().cvar_modifiedFlags |= CVAR_USERINFO;
}


void UI_closeInGame()
{
#ifdef _XBOX
	UI_SetPlayerName();
#endif
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Key_ClearStates();
	trap_Cvar_Set( "cl_paused", "0" );
	Menus_CloseAll();
	ClientManager::ClearAllClientInputs();
}


void UI_setClientInputClosed(int input)
{
	uiclientInputClosed = input;
	int currentclient;
	currentclient = ClientManager::ActiveClientNum();
	ClientManager::ActivateClient(0);
	if (ClientManager::ActiveClient().controller == uiclientInputClosed)
		uiclientInputClosed = 2+4+8;
	if (ClientManager::splitScreenMode)
	{
		ClientManager::ActivateClient(1);
		if (ClientManager::ActiveClient().controller == uiclientInputClosed)
			uiclientInputClosed = 1+4+8;
	}
	ClientManager::ActivateClient(currentclient);
	
}


//JLF
qboolean UI_testMenuProgression(char **args)
{
	const char *menuname;
	char *progressionvalue;
	qboolean back = qfalse;
	static int saberback;
	itemDef_t item;
	if ( !String_Parse(args, &menuname) )
	{
		return qfalse;
	}
	
	progressionvalue = Cvar_VariableString("ui_menuProgression");

	//ingame menu
	if ((Q_stricmp(progressionvalue, "ingamemenu") == 0))  //straight off menu
	{
		if (Q_stricmp(menuname, "characterModelSelection") == 0)
		{
			Menus_CloseByName("characterModelSelection");
			Menus_ActivateByName("characterSetup");
		}
		else if (Q_stricmp(menuname, "characterSelect") == 0)
		{
			Menus_CloseByName("characterSelect");
			Menus_ActivateByName("characterSetup");
		}
		else if ((Q_stricmp(menuname, "characterSaberCustom") == 0))
		{
			Menus_CloseByName("characterSaberCustom");
			trap_Cvar_Set("ui_respawnneeded", "1");
			Menus_ActivateByName("characterSetup");
		}
		else if ((Q_stricmp(menuname, "ingame_forcepoints") == 0))
		{
			itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
			Item_RunScript(itemp, "uiScript	xb_force confirm ");			
			Menus_CloseByName("ingame_forcepoints");
			trap_Cvar_Set("ui_respawnneeded", "1");
			Menus_ActivateByName("characterSetup");
		}

	}

	//join game 1 player
	else if ((Q_stricmp(progressionvalue, "joingame1player") == 0))  //join 1 player
	{
		if ((Q_stricmp(menuname, "characterModelSelection") == 0)||(Q_stricmp(menuname, "characterSelect") == 0))
		{
			if (UI_GetMaxForceRank() == 0) // skip the force set up
			{
				//open the saber screen
				Menus_ActivateByName("characterSaberCustom");
			}
			else
			{
				//open the force screens
				Menus_ActivateByName("ingame_playerforce");
			}
		}
		
		else if ((Q_stricmp(menuname, "ingame_forcepoints") == 0))
		{

			itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
			Item_RunScript(itemp, "uiScript	xb_force confirm ");			
			Menus_CloseByName("ingame_forcepoints");
			//trap_Cvar_Set("ui_respawnneeded", "1");
			Menus_ActivateByName("characterSaberCustom");

			/*
			if (uiForcePowersRank[FP_SABER_OFFENSE]<1)
			{
				itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
				Item_RunScript(itemp, " 	uiScript	xb_force confirm ; 	uiScript closeingame");//was at first(uiScript	setForce none ;delay		1 ;)
			}
			else
			{
				Menus_CloseByName("ingame_forcepoints");
				Menus_ActivateByName("characterSaberCustom");
				
			}
			*/
		}

		else if ((Q_stricmp(menuname, "characterSaberCustom") == 0))
		{
			itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
			Item_RunScript(itemp, "	uiScript xb_force confirm ; 	uiScript closeingame");	//uiScript	setForce none ; delay		1 ;		
		}
		
		
			
	}

	//join game 2 player split screen
	else if ((Q_stricmp(progressionvalue, "joingame2player") == 0))  //join 2 player
	{

		if ((Q_stricmp(menuname, "characterModelSelection") == 0)||(Q_stricmp(menuname, "characterSelect") == 0))
		{
			if (UI_GetMaxForceRank() == 0) // skip the force set up
			{

				Menus_ActivateByName("characterSaberCustom");
			}
			else
			{
				//open the force screens
				Menus_ActivateByName("ingame_playerforce");
				
			}
		}
		
		else if ((Q_stricmp(menuname, "ingame_forcepoints") == 0))
		{
			Menus_CloseByName("ingame_forcepoints");
			if (uiForcePowersRank[FP_SABER_OFFENSE]<1)
			{
				if ( uiClientNum == 0)
				{
					Menus_ActivateByName("ingame_Player2");
					uiClientNum = 1;
					ClientManager::ActivateClient(1);
				}
				else
				{
					itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
					Item_RunScript(itemp, "uiScript	xb_force confirm ; 	uiScript closeingame");//uiScript	setForce none ; delay		1 ;	
				}
			}
			else //saber does exist
			{
				Menus_ActivateByName("characterSaberCustom");
				
			}
		}

		else if ((Q_stricmp(menuname, "characterSaberCustom") == 0))
		{
			if ( uiClientNum == 0)
			{
				itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
				Item_RunScript(itemp, "uiScript	xb_force confirm ; 	close characterSaberCustom ; open ingame_Player2");//uiScript	setForce none ; delay		1 ;	
		
			}
			else // client 1
			{
				itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
				Item_RunScript(itemp, "	uiScript	xb_force confirm ; 	uiScript closeingame"); //uiScript	setForce none ; delay		1 ;

			}
		}

		else if ((Q_stricmp(menuname, "ingame_Player2") == 0))
		{
			Menus_ActivateByName("characterModelSelection");
		}
	}
	else if ((Q_stricmp(progressionvalue, "ChangeTeam") == 0))  //changeteam
	{
		if (Q_stricmp(menuname, "ingame_ChangeTeam") == 0)
		{
			int team = trap_Cvar_VariableValue("xb_joinTeam");
			int gametype = trap_Cvar_VariableValue("g_gametype");
			if ( team == ClientManager::ActiveClient().myTeam)
			{
				UI_closeInGame();
				if( gametype == GT_POWERDUEL )
				//	trap_Cmd_ExecuteText( EXEC_APPEND, va("duelteam %s\n", (team == 1) ? "single" : "double") );
					AddDeferedCommand( va("duelteam %s\n", (team == 1) ? "single" : "double") );
				else
				{
					if (team == 1)//red
					//	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n","forcered"));
						AddDeferedCommand(  va("team %s\n","forcered") );
					else if (team == 2) //blue
					//	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n","forceblue"));
						AddDeferedCommand(  va("team %s\n","forceblue") );
						
					else
					//	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n",UI_TeamName(team)));
						AddDeferedCommand(  va("team %s\n",UI_TeamName(team)) );

				}
			}
			else 
			{
				if ( gametype == GT_SIEGE)
				{
					Menus_CloseAll();
					Menus_OpenByName("ingame_siegecharacter");
				}
				else
				{
					int forcebasedteams = trap_Cvar_VariableValue("g_forceBasedTeams");
					if ( forcebasedteams == 0)
					{
						UI_closeInGame();
						if( gametype == GT_POWERDUEL )
						//	trap_Cmd_ExecuteText( EXEC_APPEND, va("duelteam %s\n", (team == 1) ? "single" : "double") );
							AddDeferedCommand( va("duelteam %s\n", (team == 1) ? "single" : "double") );
						else
						//	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n",UI_TeamName(team)));
							AddDeferedCommand(  va("team %s\n",UI_TeamName(team)));
					}
					else
					{
						if (UI_GetMaxForceRank() == 0) // skip the force set up
						{
							UI_closeInGame();
							if( gametype == GT_POWERDUEL )
							//	trap_Cmd_ExecuteText( EXEC_APPEND, va("duelteam %s\n", (team == 1) ? "single" : "double") );
								AddDeferedCommand( va("duelteam %s\n", (team == 1) ? "single" : "double") );
							else
							//	trap_Cmd_ExecuteText( EXEC_APPEND, va("team %s\n",UI_TeamName(team)));
								AddDeferedCommand( va("team %s\n",UI_TeamName(team)));
						}
						else
						{
						//	UI_xboxErrorPopup( XB_POPUP_FORCE_CONFIGURE_CONFIRM);							
							Menus_CloseAll();
							Menus_OpenByName("ingame_playerforce");
						}
					}
				}
			}
		}
		if (Q_stricmp(menuname, "ingame_forcePoints") == 0)
		{
			itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
			Item_RunScript(itemp, "	uiScript	xb_force confirm ; 	uiScript closeingame");//uiScript	setForce none ; delay		1 ;
		}
		
	}
	

	else
		return qfalse;

	return qtrue;
}



#define MAX_SABER_HILTS	64

char *saberSingleHiltInfo [MAX_SABER_HILTS];
char *saberStaffHiltInfo [MAX_SABER_HILTS];

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName );
void UI_SaberGetHiltInfo( char *singleHilts[MAX_SABER_HILTS], char *staffHilts[MAX_SABER_HILTS] );


void setArrowX(itemDef_t * arrowcontrol, int xloc)
{
	
	arrowcontrol->window.rect.x = xloc;

}


static void UI_UpdateCharacter( qboolean changedModel )
{
	menuDef_t *menu;
	itemDef_t *item;
	char modelPath[MAX_QPATH];
	int	animRunLength;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	item = (itemDef_t *) Menu_FindItemByName(menu, "character");

	if (!item)
	{
		Com_Error( ERR_FATAL, "UI_UpdateCharacter: Could not find item (character) in menu (%s)", menu->window.name);
	}

	ItemParse_model_g2anim_go( item, ui_char_anim.string );

	Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
	ModelMem.SetUIName( modelPath );
	ItemParse_asset_model_go( item, modelPath, &animRunLength );

	if ( changedModel )
	{//set all skins to first skin since we don't know you always have all skins
		//FIXME: could try to keep the same spot in each list as you swtich models
		UI_FeederSelection(FEEDER_PLAYER_SKIN_HEAD, 0, item);	//fixme, this is not really the right item!!
		UI_FeederSelection(FEEDER_PLAYER_SKIN_TORSO, 0, item);
		UI_FeederSelection(FEEDER_PLAYER_SKIN_LEGS, 0, item);
		UI_FeederSelection(FEEDER_COLORCHOICES, 0, item);
	}
	UI_UpdateCharacterSkin();
}

static void UI_RunMenuScript(char **args) 
{
	const char *name, *name2;
	char buff[1024];
	float cvarValue;

	if (String_Parse(args, &name)) 
	{
		if (Q_stricmp(name, "StartServer") == 0) 
		{
			int i, added = 0;
			float skill;
			int warmupTime = 0;
			int doWarmup = 0;

#ifdef _XBOX
			ClientManager::ActivateClient(0);
#endif

#ifdef _XBOX
			int botMin	= trap_Cvar_VariableValue("bot_minplayers");

			trap_Cvar_SetValue("temp_bot_minplayers", botMin);

			// We use two cvars all the time, though in syslink, only public will be >0
			trap_Cvar_SetValue("sv_maxclients",
							   trap_Cvar_VariableValue("ui_publicSlots")+trap_Cvar_VariableValue("ui_privateSlots"));
			trap_Cvar_SetValue("sv_privateClients", trap_Cvar_VariableValue("ui_privateSlots"));
#endif
			trap_Cvar_Set("cg_thirdPerson", "0");
#ifdef _XBOX
			ClientManager::ActiveClient().cg_thirdPerson = 0;
#endif
			trap_Cvar_Set("cg_cameraOrbit", "0");
			// for Solo games I set this to 1 in the menu and don't want it stomped here,
			// this cvar seems to be reset to 0 in all the proper places so... -dmv
		//	trap_Cvar_Set("ui_singlePlayerActive", "0");


			if (ui_dedicated.integer)
			{
				trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
				trap_Cvar_Set( "cl_running", "0" );
			}
			else
			{
				trap_Cvar_SetValue( "dedicated", 0 );
				trap_Cvar_Set( "cl_running", "1" );
			}

			trap_Cvar_SetValue( "g_gametype", Com_Clamp( 0, 8, uiInfo.gameTypes[ui_netGameType.integer].gtEnum ) );
			trap_Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
			trap_Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			skill = trap_Cvar_VariableValue( "g_spSkill" );

			//Cap the warmup values in case the user tries a dumb setting.
			warmupTime = trap_Cvar_VariableValue( "g_warmup" );
			doWarmup = trap_Cvar_VariableValue( "g_doWarmup" );

			if (doWarmup && warmupTime < 1)
			{
				trap_Cvar_Set("g_doWarmup", "0");
			}
			if (warmupTime < 5)
			{
				trap_Cvar_Set("g_warmup", "5");
			}
			if (warmupTime > 120)
			{
				trap_Cvar_Set("g_warmup", "120");
			}

			if (trap_Cvar_VariableValue( "g_gametype" ) == GT_DUEL ||
				trap_Cvar_VariableValue( "g_gametype" ) == GT_POWERDUEL)
			{ //always set fraglimit 1 when starting a duel game
				trap_Cvar_Set("fraglimit", "1");
			}

			// Bot match and split screen don't allow voting:
			if (trap_Cvar_VariableValue( "xb_gameType" ) < 2)
			{
				trap_Cvar_SetValue( "g_allowVote", 0 );
			}

#ifdef _XBOX
			// If we're playing siege, make sure that bot_minplayers gets set to zero
			if (ui_actualNetGameType.integer == GT_SIEGE)
				trap_Cvar_SetValue( "bot_minplayers", 0 );

			// The Xbox bot interface allows uneven teams and such. Slightly different logic here:
			int maxcl = trap_Cvar_VariableValue( "sv_maxclients" );
			int numRed = trap_Cvar_VariableValue( "ui_numRedBots" );
			int numBlue = trap_Cvar_VariableValue( "ui_numBlueBots" );

			// Sanity check
			assert( numRed + numBlue + UI_NumClients() <= maxcl );

			// Add all the blue bots, then the red ones
			for (i = 0; i < numBlue; ++i)
			{
				int bot = trap_Cvar_VariableValue( va("ui_blueteam%i", i+1) );
				if (bot < 2)
					bot = 2;

				if (ui_actualNetGameType.integer >= GT_TEAM)
					Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Blue");
				else
					Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
				trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			}

			// We should never be adding red bots in a non-team game, but...
			for (i = 0; i < numRed; ++i)
			{
				int bot = trap_Cvar_VariableValue( va("ui_redteam%i", i+1) );
				if (bot < 2)
					bot = 2;

				if (ui_actualNetGameType.integer >= GT_TEAM)
					Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Red");
				else
					Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
				trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			}
#else // _XBOX
			for (i = 0; i < PLAYERS_PER_TEAM; i++) 
			{
				int bot = trap_Cvar_VariableValue( va("ui_blueteam%i", i+1));
				int maxcl = trap_Cvar_VariableValue( "sv_maxClients" );

				if (bot > 1) 
				{
					int numval = i+1;

					numval *= 2;

					numval -= 1;

					if (numval <= maxcl)
					{
						if (ui_actualNetGameType.integer >= GT_TEAM) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Blue");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
						}
						trap_Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				bot = trap_Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot > 1) {
					int numval = i+1;

					numval *= 2;

					if (numval <= maxcl)
					{
						if (ui_actualNetGameType.integer >= GT_TEAM) {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f %s\n", UI_GetBotNameByNumber(bot-2), skill, "Red");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot \"%s\" %f \n", UI_GetBotNameByNumber(bot-2), skill);
						}
						trap_Cmd_ExecuteText( EXEC_APPEND, buff );
						added++;
					}
				}
				if (added >= maxcl)
				{ //this means the client filled up all their slots in the UI with bots. So stretch out an extra slot for them, and then stop adding bots.
					trap_Cvar_Set("sv_maxClients", va("%i", added+1));
					break;
				}
			}
#endif // _XBOX
		} else if (Q_stricmp(name, "loadArenas") == 0) {
			UI_LoadArenas();
			UI_MapCountByGameType(qfalse);
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, gUISelectedMap, "createserver");
			uiForceRank = trap_Cvar_VariableValue("g_maxForceRank");
		} else if (Q_stricmp(name, "clearError") == 0) {
			trap_Cvar_Set("com_errorMessage", "");
		} else if (Q_stricmp(name, "JoinServer") == 0) {
			UI_JoinServer();
		} else if (Q_stricmp(name, "Quit") == 0) {
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cmd_ExecuteText( EXEC_NOW, "quit");
		}
#ifdef _XBOX
		else if (Q_stricmp(name, "addclient") == 0) {
			AddUIClient();
		}
		else if (Q_stricmp(name, "removeclient") == 0) {
			RemoveUIClient();
		}
#endif
		else if (Q_stricmp(name, "Leave") == 0) 
		{
			// Were we running a dedicated server?
			if (com_dedicated->integer)
			{
				// Marks us as no longer playing or joinable:
				XBL_F_OnClientLeaveSession();

				trap_Cvar_SetValue( "cl_paused", 0 );
				trap_Key_SetCatcher( KEYCATCH_UI );
				trap_Cvar_SetValue( "cl_running", 1 );
				SV_Shutdown( "Server quit\n" );

				CL_ShutdownUI();
				extern void RE_Shutdown( qboolean destroyWindow );
				RE_Shutdown( qfalse );
				CL_Disconnect( qtrue );

				trap_Cvar_SetValue( "dedicated", 0 );
				trap_Cvar_SetValue( "ui_dedicated", 0 );
				CL_FlushMemory();

				Menus_CloseAll();
				if (logged_on)
					Menus_ActivateByName("xbl_lobbymenu");
				else
					Menus_ActivateByName("main");
				return;
			}

#ifdef _XBOX
			ClientManager::ActivateClient(0);
			trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			if (ClientManager::splitScreenMode == qtrue)
			{
				ClientManager::ActivateClient(1);
				trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			}
#else
			trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
#endif

			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("main");
		} 
		else if (Q_stricmp(name, "getsaberhiltinfo") == 0) 
		{
			UI_SaberGetHiltInfo(saberSingleHiltInfo,saberStaffHiltInfo);
		}
		else if (Q_stricmp(name, "closeingame") == 0) {
#ifdef _XBOX
//			ModelMem.ExitUI();
#endif
			UI_closeInGame();
			
		}
		else if( Q_stricmp(name, "VoteScript") == 0)
		{
			const char *name;

			if (String_Parse(args, &name))
			{
				RunVoteScript(name);
			}
		}
		else if( Q_stricmp(name, "SetSabers") == 0)
		{
			Cvar_Set("ui_saber", ClientManager::ActiveClient().saber1);
			Cvar_Set("ui_saber2", ClientManager::ActiveClient().saber2);
			Cvar_Set("ui_saber_color", ClientManager::ActiveClient().saber_color1);
			Cvar_Set("ui_saber2_color", ClientManager::ActiveClient().saber_color2);

			if(strstr(ClientManager::ActiveClient().saber1, "dual"))
			{
				Cvar_Set("ui_saber_type", "staff");
			}
			else if(strstr(ClientManager::ActiveClient().saber2, "single"))
			{
				Cvar_Set("ui_saber_type", "dual");
			}
			else
			{
				Cvar_Set("ui_saber_type", "single");
			}

		}
		else if(Q_stricmp(name, "setUglyMug") == 0)
		{
			menuDef_t*	menu	= Menu_GetFocused();
			itemDef_t*	item	= Menu_FindItemByName(menu, "headlist");

			if(!item)
				return;

			listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

			if(ClientManager::splitScreenMode)
			{
				if(ClientManager::ActiveClientNum() == 0)
				{
					item->cursorPos	= uglyMug1;
					listPtr->cursorPos	= uglyMug1;
				}
				else
				{
					item->cursorPos	= uglyMug2;
					listPtr->cursorPos	= uglyMug2;
				}
			}
			else
			{
				item->cursorPos	= uglyMug1;
				listPtr->cursorPos	= uglyMug1;
			}

			int numberOfHeads	= 43;
			int	width			= 6;
			int	startPos		= 0;
			int cutoff			= 24;
			if(item->cursorPos > numberOfHeads || item->cursorPos < 0)
			{
				startPos = 0;
			}
			else
			{
				while(startPos < numberOfHeads)
				{
					if(item->cursorPos < startPos)
					{
						startPos	-= width;
						break;
					}
					else if( item->cursorPos == startPos)
					{
						break;
					}
					startPos	+=	width;
				}
			}

			if(startPos > cutoff)
				listPtr->startPos	= cutoff;
			else
				listPtr->startPos = startPos;
		}
		 else if (Q_stricmp(name, "addBot") == 0) {
			if (trap_Cvar_VariableValue("g_gametype") >= GT_TEAM) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot \"%s\" %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot \"%s\" %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			}
		}
/*
		else if (Q_stricmp(name, "orders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					strcpy(buff, orders);
					trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				} else {
					int i;
					for (i = 0; i < uiInfo.myTeamCount; i++) {
						if (Q_stricmp(UI_Cvar_VariableString("name"), uiInfo.teamNames[i]) == 0) {
							continue;
						}
						strcpy(buff, orders);
						trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamNames[i]) );
						trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
					}
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrdersTeam") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer == uiInfo.myTeamCount) {
					trap_Cmd_ExecuteText( EXEC_APPEND, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");

				if (selectedPlayer == uiInfo.myTeamCount)
				{
					selectedPlayer = -1;
					strcpy(buff, orders);
					trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, selectedPlayer) );
				}
				else
				{
					strcpy(buff, orders);
					trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
				}
				trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );

				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		}
*/
		else if (Q_stricmp(name, "setForce") == 0)
		{
			const char *teamArg;

			if (String_Parse(args, &teamArg))
			{
				if ( Q_stricmp( "none", teamArg ) == 0 )
				{
					UI_UpdateClientForcePowers(NULL);
				}
				else if ( Q_stricmp( "same", teamArg ) == 0 )
				{//stay on current team
					int myTeam = ClientManager::ActiveClient().myTeam;
					if ( myTeam != TEAM_SPECTATOR )
					{
						UI_UpdateClientForcePowers(UI_TeamName(myTeam));//will cause him to respawn, if it's been 5 seconds since last one
					}
					else
					{
						UI_UpdateClientForcePowers(NULL);//just update powers
					}
				}
				else
				{
					UI_UpdateClientForcePowers(teamArg);
				}
			}
			else
			{
				UI_UpdateClientForcePowers(NULL);
			}
		}
		else if (Q_stricmp(name, "setsiegeclassandteam") == 0)
		{
			int team = (int)trap_Cvar_VariableValue("ui_holdteam");
			int oldteam = (int)trap_Cvar_VariableValue("ui_startsiegeteam");
			qboolean	goTeam = qtrue;
			char	newclassString[512];
			char	startclassString[512];

			trap_Cvar_VariableStringBuffer( "ui_mySiegeClass", newclassString, sizeof(newclassString) );
			trap_Cvar_VariableStringBuffer( "ui_startsiegeclass", startclassString, sizeof(startclassString) );
				
			// Was just a spectator - is still just a spectator
			if ((oldteam == team) && (oldteam == 3))
			{
				goTeam = qfalse;
			}
			// If new team and class match old team and class, just return to the game.
			else if ((oldteam == team))
			{	// Classes match?
				if (g_UIGloballySelectedSiegeClass != -1)
				{	
					if (!strcmp(startclassString,bgSiegeClasses[g_UIGloballySelectedSiegeClass].name))
					{
						goTeam = qfalse;
					}
				}
			}

			if (goTeam)
			{
				if (team == 1)	// Team red
				{
					trap_Cvar_Set("ui_team", va("%d", team));
				}
				else if (team == 2)	// Team blue
				{
					trap_Cvar_Set("ui_team", va("%d", team));
				}
				else if (team == 3)	// Team spectator
				{
					trap_Cvar_Set("ui_team", va("%d", team));
				}

				if (g_UIGloballySelectedSiegeClass != -1)
				{
					trap_Cmd_ExecuteText( EXEC_APPEND, va("siegeclass \"%s\"\n", bgSiegeClasses[g_UIGloballySelectedSiegeClass].name) );
				}
			}
		} 
		else if (Q_stricmp(name, "setMovesListDefault") == 0) 
		{
			uiInfo.movesTitleIndex = 2;
		}
		else if (Q_stricmp(name, "resetMovesList") == 0) 
		{
			menuDef_t *menu;
			menu = Menus_FindByName("rules_moves");
			//update saber models
			if (menu)
			{
				itemDef_t *item  = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					UI_SaberAttachToChar( item );
				}
			}

			trap_Cvar_Set( "ui_move_desc", " " );
		}
		else if (Q_stricmp(name, "resetcharacterlistboxes") == 0) 
		{
			UI_ResetCharacterListBoxes();
		}
		else if (Q_stricmp(name, "setMoveCharacter") == 0) 
		{
			itemDef_t *item;
			menuDef_t *menu;
			modelDef_t *modelPtr;
			int	animRunLength;

			UI_GetCharacterCvars();

			uiInfo.movesTitleIndex = 0;

			menu = Menus_FindByName("rules_moves");

			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
				if (item)
				{
					modelPtr = (modelDef_t*)item->typeData;
					if (modelPtr)
					{
						char modelPath[MAX_QPATH];

						uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
						ItemParse_model_g2anim_go( item,  uiInfo.movesBaseAnim );
						uiInfo.moveAnimTime = 0 ;

						Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
						ItemParse_asset_model_go( item, modelPath, &animRunLength);

						UI_UpdateCharacterSkin();
						UI_SaberAttachToChar( item );
					}
				}
			} 
		}
		else if (Q_stricmp(name, "character") == 0) 
		{
#ifdef _XBOX
//			ModelMem.EnterUI();
#endif
			UI_UpdateCharacter( qfalse );
		}
		else if (Q_stricmp(name, "characterchanged") == 0) 
		{
			UI_UpdateCharacter( qtrue );
		}
		else if (Q_stricmp(name, "updatecharcvars") == 0
			|| (Q_stricmp(name, "updatecharmodel") == 0) )
		{
			UI_UpdateCharacterCvars();
		}
		else if (Q_stricmp(name, "getcharcvars") == 0) 
		{
			UI_GetCharacterCvars();
		}
		else if (Q_stricmp(name, "char_skin") == 0) 
		{
			UI_UpdateCharacterSkin();
		}
		else if (Q_stricmp(name, "setui_dualforcepower") == 0) 
		{
			int forcePowerDisable = trap_Cvar_VariableValue("g_forcePowerDisable");
			int	i, forceBitFlag=0;

			// Turn off all powers but a few
			for (i=0;i<NUM_FORCE_POWERS;i++)
			{
				if ((i != FP_LEVITATION) &&
					(i != FP_PUSH) &&
					(i != FP_PULL) &&
					(i != FP_SABERTHROW) &&
					(i != FP_SABER_DEFENSE) &&
					(i != FP_SABER_OFFENSE))
				{
					forceBitFlag |= (1<<i);
				}
			}


			if (forcePowerDisable==0)
			{
				trap_Cvar_Set("ui_dualforcepower", "0");
			}
			else if (forcePowerDisable==forceBitFlag)
			{
				trap_Cvar_Set("ui_dualforcepower", "2");
			}
			else 
			{
				trap_Cvar_Set("ui_dualforcepower", "1");
			}

		}
		else if (Q_stricmp(name, "dualForcePowers") == 0) 
		{
			int	dualforcePower,i, forcePowerDisable;
			dualforcePower = trap_Cvar_VariableValue("ui_dualforcepower");

			if (dualforcePower==0)	// All force powers
			{
				forcePowerDisable = 0;
			}
			else if (dualforcePower==1)	// Remove All force powers
			{
				// It was set to something, so might as well make sure it got all flags set.
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					forcePowerDisable |= (1<<i);
				}
			}
			else if (dualforcePower==2)	// Limited force powers
			{
				forcePowerDisable = 0;						
				
				// Turn off all powers but a few
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					if ((i != FP_LEVITATION) &&
						(i != FP_PUSH) &&
						(i != FP_PULL) &&
						(i != FP_SABERTHROW) &&
						(i != FP_SABER_DEFENSE) &&
						(i != FP_SABER_OFFENSE))
					{
						forcePowerDisable |= (1<<i);
					}
				}
			}

			trap_Cvar_Set("g_forcePowerDisable", va("%i",forcePowerDisable));
		}
		else if (Q_stricmp(name, "forcePowersDisable") == 0) 
		{
			int	forcePowerDisable,i;

			forcePowerDisable = trap_Cvar_VariableValue("g_forcePowerDisable");

			// It was set to something, so might as well make sure it got all flags set.
			if (forcePowerDisable)
			{
				for (i=0;i<NUM_FORCE_POWERS;i++)
				{
					forcePowerDisable |= (1<<i);
				}

				trap_Cvar_Set("g_forcePowerDisable", va("%i",forcePowerDisable));
			}

		} 
		else if (Q_stricmp(name, "weaponDisable") == 0) 
		{
			int	weaponDisable,i;
			const char *cvarString;

			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_DUEL ||
				uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_POWERDUEL)
			{
				cvarString = "g_duelWeaponDisable";
			}
			else
			{
				cvarString = "g_weaponDisable";
			}

			weaponDisable = trap_Cvar_VariableValue(cvarString);

			// It was set to something, so might as well make sure it got all flags set.
			if (weaponDisable)
			{
				for (i=0;i<WP_NUM_WEAPONS;i++)
				{
					if (i!=WP_SABER)
					{
						weaponDisable |= (1<<i);
					}
				}

				trap_Cvar_Set(cvarString, va("%i",weaponDisable));
			}
		} 
		// If this is siege, change all the bots to humans, because we faked it earlier 
		//  swapping humans for bots on the menu
		else if (Q_stricmp(name, "setSiegeNoBots") == 0) 
		{
			int blueValue,redValue,i;

			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_SIEGE)
			{
                //hmm, I guess I'll set bot_minplayers to 0 here too. -rww
				trap_Cvar_Set("bot_minplayers", "0");

				for (i=1;i<9;i++)
				{
					blueValue = trap_Cvar_VariableValue(va("ui_blueteam%i",i ));
					if (blueValue>1)
					{
						trap_Cvar_Set(va("ui_blueteam%i",i ), "1");
					}

					redValue = trap_Cvar_VariableValue(va("ui_redteam%i",i ));
					if (redValue>1)
					{
						trap_Cvar_Set(va("ui_redteam%i",i ), "1");
					}

				}
			}
		}
		else if (Q_stricmp(name, "updateForceStatus") == 0)
		{
			UpdateForceStatus();
		}
		else if (Q_stricmp(name, "update") == 0) 
		{
			if (String_Parse(args, &name2)) 
			{
				UI_Update(name2);
			}
		}
		else if (Q_stricmp(name, "getsabercvars") == 0) 
		{
			UI_GetSaberCvars();
		}
		else if (Q_stricmp(name, "setsaberboxesandhilts") == 0) 
		{
			UI_SetSaberBoxesandHilts();
		}
		else if (Q_stricmp(name, "saber_type") == 0) 
		{
			UI_UpdateSaberType();
		}
		else if (Q_stricmp(name, "saber_hilt") == 0) 
		{
			UI_UpdateSaberHilt( qfalse );
		}
		else if (Q_stricmp(name, "saber_color") == 0) 
		{
			UI_UpdateSaberColor( qfalse );
		}
		else if (Q_stricmp(name, "setscreensaberhilt") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap_Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberhilt1") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut1");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap_Cvar_Set( "ui_saber", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberhilt2") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut2");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap_Cvar_Set( "ui_saber2", saberSingleHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "setscreensaberstaff") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "hiltbut_staves");
				if (item)
				{
					if (saberSingleHiltInfo[item->cursorPos])
					{
						trap_Cvar_Set( "ui_saber", saberStaffHiltInfo[item->cursorPos] );
					}
				}
			}
		}
		else if (Q_stricmp(name, "saber2_hilt") == 0) 
		{
			UI_UpdateSaberHilt( qtrue );
		}
		else if (Q_stricmp(name, "saber2_color") == 0) 
		{
			UI_UpdateSaberColor( qtrue );
		}
		else if (Q_stricmp(name, "updatesabercvars") == 0) 
		{
		//	UI_UpdateSaberCvars(trap_Cvar_VariableValue("ui_menuClient"));
			UI_UpdateSaberCvars(uiClientNum);

		}
		else if (Q_stricmp(name, "updatesiegeobjgraphics") == 0) 
		{
			int team = (int)trap_Cvar_VariableValue("ui_team");
			trap_Cvar_Set("ui_holdteam", va("%d", team));

			UI_UpdateSiegeObjectiveGraphics();

			// Set up the objective complete icons
			itemDef_t*	item;
			menuDef_t*	menu	= Menu_GetFocused();
			if(!strcmp(Cvar_Get("mapname","",0)->string, "mp/siege_hoth"))
			{
				// team 1
				item	= Menu_FindItemByName(menu, "Comp_tm1_obj1");
				item->window.rect.x	= 130;
				item->window.rect.y	= 137;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj2");
				item->window.rect.x	= 130;
				item->window.rect.y	= 163;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj3");
				item->window.rect.x	= 130;
				item->window.rect.y	= 189;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj4");
				item->window.rect.x	= 130;
				item->window.rect.y	= 215;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj5");
				item->window.rect.x	= 130;
				item->window.rect.y	= 241;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj6");
				item->window.rect.x	= 130;
				item->window.rect.y	= 267;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				// team 2
				item	= Menu_FindItemByName(menu, "Comp_tm2_obj1");
				item->window.rect.x	= 488;
				item->window.rect.y	= 137;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj2");
				item->window.rect.x	= 488;
				item->window.rect.y	= 163;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj3");
				item->window.rect.x	= 488;
				item->window.rect.y	= 189;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj4");
				item->window.rect.x	= 488;
				item->window.rect.y	= 215;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj5");
				item->window.rect.x	= 488;
				item->window.rect.y	= 241;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj6");
				item->window.rect.x	= 488;
				item->window.rect.y	= 267;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

			}
			else
			{
				// team 2
				item	= Menu_FindItemByName(menu, "Comp_tm2_obj1");
				item->window.rect.x	= 130;
				item->window.rect.y	= 137;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj2");
				item->window.rect.x	= 130;
				item->window.rect.y	= 163;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj3");
				item->window.rect.x	= 130;
				item->window.rect.y	= 189;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj4");
				item->window.rect.x	= 130;
				item->window.rect.y	= 215;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj5");
				item->window.rect.x	= 130;
				item->window.rect.y	= 241;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm2_obj6");
				item->window.rect.x	= 130;
				item->window.rect.y	= 267;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				// team 1
				item	= Menu_FindItemByName(menu, "Comp_tm1_obj1");
				item->window.rect.x	= 488;
				item->window.rect.y	= 137;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj2");
				item->window.rect.x	= 488;
				item->window.rect.y	= 163;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj3");
				item->window.rect.x	= 488;
				item->window.rect.y	= 189;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj4");
				item->window.rect.x	= 488;
				item->window.rect.y	= 215;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj5");
				item->window.rect.x	= 488;
				item->window.rect.y	= 241;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;

				item	= Menu_FindItemByName(menu, "Comp_tm1_obj6");
				item->window.rect.x	= 488;
				item->window.rect.y	= 267;
				item->window.rect.w	= 24;
				item->window.rect.h	= 24;
			}
		}
		else if (Q_stricmp(name, "setsiegeobjbuttons") == 0) 
		{
			const char *itemArg;
			const char *cvarLitArg;
			const char *cvarNormalArg;
			char	string[512];
			char	string2[512];
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				// Set the new item to the background 
				if (String_Parse(args, &itemArg))
				{

					// Set the old button to it's original background
					trap_Cvar_VariableStringBuffer( "currentObjMapIconItem", string, sizeof(string) );
					item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, string);
					if (item)
					{
							// A cvar holding the name of a cvar - how crazy is that?
							trap_Cvar_VariableStringBuffer( "currentObjMapIconBackground", string, sizeof(string) );
							trap_Cvar_VariableStringBuffer( string, string2, sizeof(string2) );
							Menu_SetItemBackground(menu, item->window.name, string2);

							// Re-enable this button
							Menu_ItemDisable(menu,(char *) item->window.name, qfalse);
					}

					// Set the new item to the given background
					item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, itemArg);
					if (item)
					{	// store item name
						trap_Cvar_Set("currentObjMapIconItem",	 item->window.name);
						if (String_Parse(args, &cvarNormalArg))
						{	// Store normal background
							trap_Cvar_Set("currentObjMapIconBackground", cvarNormalArg);
							// Get higlight background
							if (String_Parse(args, &cvarLitArg))
							{	// set hightlight background
								trap_Cvar_VariableStringBuffer( cvarLitArg, string, sizeof(string) );
								Menu_SetItemBackground(menu, item->window.name, string);
								// Disable button
								Menu_ItemDisable(menu,(char *) item->window.name, qtrue);
							}
						}
					}
				}
			}


		}
#ifdef _XBOX
		else if (Q_stricmp(name, "xbx_changesiegemenu") == 0)
		{
			const char*	val;

			// close all menus
			Menus_CloseAll();

			// should we be setting stuff up for class select, or
			// just returning to the game
			if( trap_Cvar_VariableValue("ui_siegeSelect") )
			{
				char*	mapname;
				char	info[MAX_INFO_VALUE];

				if (trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) ))
				{
					mapname = Info_ValueForKey( info, "mapname" );
				}

				if (!mapname || !mapname[0])
				{
					return;
				}

				if(String_Parse(args, &val))
				{
					int	num	= atoi(val);

					if(strcmp(mapname, "mp/siege_hoth"))
					{
						if(num == 1)
							num	= 2;
						else
							num = 1;
					}

					// set the team cvars
					trap_Cvar_Set("ui_team", va("%d",num));
					trap_Cvar_Set("ui_holdteam", va("%d",num));

					// update the class count
					UI_SiegeClassCnt(num);

					// open the class section menu
					Menus_OpenByName("ingame_siegecharacter");
				}

				// reset the siege select cvar
				trap_Cvar_Set("ui_siegeSelect", "0");
			}
		}
#endif //_XBOX
		else if (Q_stricmp(name, "updatesiegeclasscnt") == 0) 
		{
			const char *teamArg;

			if (String_Parse(args, &teamArg))
			{
					UI_SiegeClassCnt(atoi(teamArg));
			}
		}
		else if (Q_stricmp(name, "updatesiegecvars") == 0) 
		{
			int team,baseClass; 
	
			team = (int)trap_Cvar_VariableValue("ui_holdteam");
			baseClass = (int)trap_Cvar_VariableValue("ui_siege_class");

			UI_UpdateCvarsForClass(team, baseClass, 0);

		}
		// Save current team and class
		else if (Q_stricmp(name, "setteamclassicons") == 0)
		{
			int team = (int)trap_Cvar_VariableValue("ui_holdteam");
			char	classString[512];

			trap_Cvar_VariableStringBuffer( "ui_mySiegeClass", classString, sizeof(classString) );

			trap_Cvar_Set("ui_startsiegeteam", va("%d", team));
		 	trap_Cvar_Set( "ui_startsiegeclass", classString);

			// If player is already on a team, set up icons to show it.
			UI_FindCurrentSiegeTeamClass();

		}
		else if (Q_stricmp(name, "updatesiegeweapondesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_weapons_feed");
				if (item)
				{
					char	info[MAX_INFO_VALUE];
					trap_Cvar_VariableStringBuffer( va("ui_class_weapondesc%i", item->cursorPos), info, sizeof(info) );
					trap_Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegeinventorydesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_inventory_feed");
				if (item)
				{
					char info[MAX_INFO_VALUE];
					trap_Cvar_VariableStringBuffer( va("ui_class_itemdesc%i", item->cursorPos), info, sizeof(info) );
					trap_Cvar_Set( "ui_itemforceinvdesc", info );
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegeforcedesc") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
				if (item)
				{
					int i;
					char info[MAX_STRING_CHARS];

					trap_Cvar_VariableStringBuffer( va("ui_class_power%i", item->cursorPos), info, sizeof(info) );

					//count them up
					for (i=0;i< NUM_FORCE_POWERS;i++)
					{
						if (!strcmp(HolocronIcons[i],info))
						{
							trap_Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
						}
					}
				}
			}
		}
		else if (Q_stricmp(name, "resetitemdescription") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{

				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "itemdescription");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						listPtr->startPos = 0;
						listPtr->cursorPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if (Q_stricmp(name, "resetsiegelistboxes") == 0)
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "description");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_weapons_feed");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_inventory_feed");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}

				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						listPtr->startPos = 0;
					}
					item->cursorPos = 0;
				}
			}
		}
		else if (Q_stricmp(name, "updatesiegestatusicons") == 0) 
		{
			UI_UpdateSiegeStatusIcons();
		}
		else if (Q_stricmp(name, "setcurrentNetMap") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;

			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "maplist");
				if (item)
				{
					listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
					if (listPtr)
					{
						trap_Cvar_Set("ui_currentNetMap", va("%d",listPtr->cursorPos));
					}
				}
			}
		}
		else if (Q_stricmp(name, "resetmaplist") == 0) 
		{
			menuDef_t *menu;
			itemDef_t *item;
		
			menu = Menu_GetFocused();	// Get current menu
			if (menu)
			{
				item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "maplist");
				if (item)
				{
					uiInfo.uiDC.feederSelection(item->special, item->cursorPos, item);
				}
			}
		}
		else if (Q_stricmp(name, "clampmaxplayers") == 0)
		{
			UI_ClampMaxPlayers();
		}
#ifdef _XBOX
		// XBL uiScript commands
		else if (Q_stricmp(name, "initaccountlist") == 0)
		{
			// Make sure that things are up and running
			XBL_Init();
			XBL_GetNumAccounts( true );
			Menu_SetFeederSelection( NULL, FEEDER_XBL_ACCOUNTS, 0, NULL );
		}

		else if (Q_stricmp(name, "account") == 0)
		{
			// Followed by either "create" or "choose"
			const char *accountAction;
			if (!String_Parse(args, &accountAction) || !accountAction)
				return;

			// For creating a new account:
			if ((Q_stricmp(accountAction, "create") == 0) ||
				!XBL_GetNumAccounts( false ))
			{
				// L1-1-15
				// Net troubleshooter must be an option if no connection is found
				// when user tries to make a new account:
				if( !Net_ConnectionStatus() )
					UI_xboxErrorPopup( XB_POPUP_CANNOT_CONNECT );
				else
					UI_xboxErrorPopup( XB_POPUP_CONFIRM_NEW_ACCOUNT );
			}
			else if (Q_stricmp(accountAction, "choose") == 0)
			{
				// We've already called SetAccountIndex somewhere
				// We don't even check for return value - this just kicks off a
				// giant sequence of popups and such, ending with the lobby
				XBL_Login( LOGIN_PASSCODE_CHECK );
			}
		}
		else if (Q_stricmp(name, "logofflive") == 0)
		{
			// User is already logged on - is trying to back out. Get confirmation
			UI_xboxErrorPopup( XB_POPUP_CONFIRM_LOGOFF );
		}
		else if (Q_stricmp(name, "quickmatch") == 0)
		{
			// Run the query. This will issue a connect if possible, otherwise it will
			// display the correct popup:
			XBL_MM_QuickMatch((ui_optiGameType.integer >= 0) ? ui_optiGameType.integer : X_MATCH_NULL_INTEGER);
		}
		else if (Q_stricmp(name, "optimatch") == 0)
		{
			// Followed by "query", "create", "updatemin", "updatemax"
			const char *optiAction;
			if (!String_Parse(args, &optiAction) || !optiAction)
				return;
	
			if (Q_stricmp(optiAction, "query") == 0)
			{
			    // Run the query
			    XBL_MM_Find_Session(
				    (ui_optiGameType.integer >= 0) ? ui_optiGameType.integer : X_MATCH_NULL_INTEGER,
				    ui_optiCurrentMap.string,
				    ui_optiMinPlayers.integer,
				    ui_optiMaxPlayers.integer,
				    (ui_optiFriendlyFire.integer >= 0) ? ui_optiFriendlyFire.integer : X_MATCH_NULL_INTEGER,
				    (ui_optiJediMastery.integer >= 0) ? ui_optiJediMastery.integer : X_MATCH_NULL_INTEGER,
					(ui_optiSaberOnly.integer >= 0) ? ui_optiSaberOnly.integer : X_MATCH_NULL_INTEGER,
					(ui_optiDedicated.integer >= 0) ? ui_optiDedicated.integer : X_MATCH_NULL_INTEGER );

			    if (!XBL_MM_GetNumServers())
			    {
				    // If there are no results, display the popup rather than switching menus
				    UI_xboxErrorPopup( XB_POPUP_OPTIMATCH_NO_RESULTS );
			    }
			    else
			    {
				    // Everything else is automatic. XBL_MM_Tick is getting pings for us,
				    // results already exists and will be pulled by the listbox drawing code.
				    Menus_CloseAll();
				    Menus_OpenByName("optimatch_results");
			    }
			}
			else if (Q_stricmp(optiAction, "updatemin") == 0)
			{
				// If user is asking for a non-dedicated server, this should be in [0..7], else [0..9]
				int maxVal = (trap_Cvar_VariableValue( "ui_optiDedicated" ) == 0) ? 7 : 9;
				int minPlayers = trap_Cvar_VariableValue( "ui_optiMinPlayers" );
				int maxPlayers = trap_Cvar_VariableValue( "ui_optiMaxPlayers" );

				// Clamp to maximum value
				if( minPlayers > maxVal )
					minPlayers = maxVal;
				// Clamp to minimum (zero):
				if( minPlayers < 0 )
					minPlayers = 0;

				// If we increased to above current maximum players, increase that:
				if( minPlayers > maxPlayers )
					maxPlayers = minPlayers;

				// Re-set the variables:
				trap_Cvar_SetValue( "ui_optiMinPlayers", minPlayers );
				trap_Cvar_SetValue( "ui_optiMaxPlayers", maxPlayers );
			}
			else if (Q_stricmp(optiAction, "updatemax") == 0)
			{
				// If user is asking for a non-dedicated server, this should be in [1..7], else [0..11]
				int maxVal = (trap_Cvar_VariableValue( "ui_optiDedicated" ) == 0) ? 7 : 9;
				int minPlayers = trap_Cvar_VariableValue( "ui_optiMinPlayers" );
				int maxPlayers = trap_Cvar_VariableValue( "ui_optiMaxPlayers" );

				// Clamp to maximum value
				if( maxPlayers > maxVal )
					maxPlayers = maxVal;
				// Clamp to minimum (zero):
				if( maxPlayers < 0 )
					maxPlayers = 0;

				// If we reduced to below current minimum players, reduce that:
				if( maxPlayers < minPlayers )
					minPlayers = maxPlayers;

				// Re-set the variables:
				trap_Cvar_SetValue( "ui_optiMinPlayers", minPlayers );
				trap_Cvar_SetValue( "ui_optiMaxPlayers", maxPlayers );
			}
			else if (Q_stricmp(optiAction, "updateded") == 0)
			{
				int minPlayers = trap_Cvar_VariableValue( "ui_optiMinPlayers" );
				int maxPlayers = trap_Cvar_VariableValue( "ui_optiMaxPlayers" );

				// If user changed to only wanting non-dedicated servers,
				// then we have to re-clamp the min/max fields:
				if( trap_Cvar_VariableValue( "ui_optiDedicated" ) != 0 )
					return;

				if( maxPlayers > 7 )
					trap_Cvar_SetValue( "ui_optiMaxPlayers", 7 );
				if( minPlayers > 7 )
					trap_Cvar_SetValue( "ui_optiMinPlayers", 7 );
			}
		}
		else if (Q_stricmp(name, "haltoptimatch") == 0)
		{
			// Cancels probing of QoS from results once we've backed out or started joining a server
			XBL_MM_CancelProbing();
		}
		else if (Q_stricmp(name, "copyQueryToGame") == 0)
		{
			// User ran an Optimatch or Quickmatch and got no results. Copy the query values
			// over so that the game they host (if they just hit the A button again) will be right.
			// This should be simple, alas...

			// Easy things first:
			if( ui_optiFriendlyFire.integer >= 0 )
				trap_Cvar_SetValue( "g_friendlyfire", ui_optiFriendlyFire.integer );
			if( ui_optiJediMastery.integer >= 0 )
				trap_Cvar_SetValue( "ui_maxForceRank", ui_optiJediMastery.integer );
			if( ui_optiSaberOnly.integer >= 0 )
				trap_Cvar_SetValue( "g_weaponDisable", ui_optiSaberOnly.integer );

			// Now the hard part:
			bool pickedMap = (Q_stricmp(ui_optiCurrentMap.string, "any") != 0);
			bool pickedGame = (ui_optiGameType.integer != -1);

			// No preferences? They get defaults
			if( !pickedMap && !pickedGame )
				return;

			// If they only specified a gametype, but no map, then get their setting, and pick a suitable map:
			if( pickedGame && !pickedMap )
			{
				int gt, mapIndex;

				switch( ui_optiGameType.integer )
				{
					case 0:	// FFA
						gt = 0;
						mapIndex = 15;
						break;
					case 3:	// Duel
						gt = 1;
						mapIndex = 5;
						break;
					case 4:	// PowerDuel
						gt = 2;
						mapIndex = 5;
						break;
					case 6:	// TFFA
						gt = 3;
						mapIndex = 15;
						break;
					case 7:	// Siege
						gt = 4;
						mapIndex = 20;
						break;
					case 8:	// CTF
						gt = 5;
						mapIndex = 0;
						break;
				}
				trap_Cvar_SetValue( "ui_netGametype", gt );
				trap_Cvar_SetValue( "ui_currentNetMap", mapIndex );
				return;
			}

			// If they only picked a map, but no gametype, then pick a gametype that supports that map:
			if( pickedMap && !pickedGame )
			{
				int mapIndex = mapNameToIndex( ui_optiCurrentMap.string );
				int gt;

				if( mapIndex >= 0 && mapIndex <= 4 )
					gt = 5;	// CTF
				else if( mapIndex >= 5 && mapIndex <= 14 )
					gt = 1;	// Duel
				else if( mapIndex >= 15 && mapIndex <= 19 )
					gt = 0;	// FFA
				else
					gt = 4;	// Siege

				trap_Cvar_SetValue( "ui_netGametype", gt );
				trap_Cvar_SetValue( "ui_currentNetMap", mapIndex );
				return;
			}

			// They picked both a map and a gametype. We assume that it's a legal combo:
			int gt;

			switch( ui_optiGameType.integer )
			{
				case 0: gt = 0; break;
				case 3: gt = 1; break;
				case 4: gt = 2; break;
				case 6: gt = 3; break;
				case 7: gt = 4; break;
				case 8: gt = 5; break;
			}
			trap_Cvar_SetValue( "ui_netGametype", gt );
			trap_Cvar_SetValue( "ui_currentNetMap", mapNameToIndex( ui_optiCurrentMap.string ) );
		}
		else if (Q_stricmp(name, "xboxErrorResponse") == 0)
		{
			// User closed the Xbox Error Popup in some way. Do TheRightThing(TM)
			UI_xboxPopupResponse();
		}
		else if (Q_stricmp(name, "singleplayer") == 0)
		{
			extern void Sys_Reboot( const char *reason );
			Sys_Reboot("singleplayer");
		}
		else if (Q_stricmp(name, "plyrList") == 0)
		{
			// Handles all player list functionality as secondary commands
			UI_XBL_PlayerListScript(args, name);
		}
		else if (Q_stricmp(name, "friendsList") == 0)
		{
			// Handles all friends list functionality as secondary commands
			UI_XBL_FriendsListScript(args, name);
		}
		else if (Q_stricmp(name, "getOnlineOptions") == 0)
		{
			// Retrieve all relevant options for UI display:

			// Appear offline:
			Cvar_SetValue( "ui_appearOffline", XBL_F_GetState( XONLINE_FRIENDSTATE_FLAG_ONLINE ) ? 0 : 1 );

			// Current voice mask:
			Cvar_SetValue( "ui_voiceMask", g_Voice.GetVoiceMask() );

			// Current voice mode:
			Cvar_SetValue( "ui_voiceMode", g_Voice.GetVoiceMode() );

			// Is there a headset plugged in? Used to disable voicemask, etc...
			Cvar_SetValue( "ui_headset", g_Voice.CommunicatorPresent() ? 1 : 0 );
		}
		else if (Q_stricmp(name, "setOnlineOptions") == 0)
		{
			// Update various options from the ones in the UI:

			// Appear offline:
			if( logged_on )
			{
				Settings.appearOffline = Cvar_VariableIntegerValue( "ui_appearOffline" );
				XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_ONLINE, !Settings.appearOffline );
			}

			// Voice mask:
			Settings.voiceMask = Cvar_VariableIntegerValue( "ui_voiceMask" );
			g_Voice.SetVoiceMask( Settings.voiceMask );

			// Other voice options (speakers/disabled):
			Settings.voiceMode = Cvar_VariableIntegerValue( "ui_voiceMode" );
			g_Voice.SetVoiceOptions( Settings.voiceMode );

			Settings.Save();
		}
		else if (Q_stricmp(name, "xblUpdateGameType") == 0)
		{
			// Replacement for complicated gametype ownerdraw
			int gt = trap_Cvar_VariableValue( "ui_netGameType" );

			// Make sure that the currently selected map is valid for this gametype:
			switch( gt )
			{
			case 0: case 3:	// FFA, Team FFA
				trap_Cvar_SetValue( "ui_currentNetMap", 15 );
				break;
			case 1: case 2:	// Duel, PowerDuel
				trap_Cvar_SetValue( "ui_currentNetMap", 5 );
				break;
			case 4:			// Siege
				trap_Cvar_SetValue( "ui_currentNetMap", 20 );
				break;
			case 5:			// CTF
				trap_Cvar_SetValue( "ui_currentNetMap", 0 );
				break;
			}

			// FFA and duel are non-team, make sure there are no red bots:
			if (gt == 0 || gt == 1)
				trap_Cvar_SetValue( "ui_numRedBots", 0 );

			// Siege has no bots at all:
			if (gt == 4)
			{
				trap_Cvar_SetValue( "ui_numRedBots", 0 );
				trap_Cvar_SetValue( "ui_numBlueBots", 0 );
			}

			// Power duel requires at least three people:
			if (gt == 2)
			{
				int pubSlots = trap_Cvar_VariableValue( "ui_publicSlots" );
				int privSlots = trap_Cvar_VariableValue( "ui_privateSlots" );

				if (pubSlots + privSlots < 3)
					trap_Cvar_SetValue( "ui_publicSlots", 3 - privSlots );
			}

			// Update actualnetgametype, like ownerdraw does:
		  	trap_Cvar_SetValue( "ui_actualnetGameType", uiInfo.gameTypes[gt].gtEnum);
		}
		else if (Q_stricmp(name, "xblUpdateBotSlotsNext") == 0)
		{
			UpdateNextBotSlot();
		}
		else if (Q_stricmp(name, "xblUpdateBotSlotsPrev") == 0)
		{
			UpdatePrevBotSlot();
		}
		else if (Q_stricmp(name, "xblUpdatePublicSlots") == 0)
		{
			// This no longer supports wrapping - there's too many ambiguous
			// situations. Instead, we clamp values at the min/maximum:
			int pubSlots = trap_Cvar_VariableValue( "ui_publicSlots" );
			int privSlots = trap_Cvar_VariableValue( "ui_privateSlots" );
			int maxSlots = trap_Cvar_VariableValue( "ui_dedicated" ) ? 10 : 8;
			int gt = trap_Cvar_VariableValue( "ui_netGameType" );

			// Clamp underflow:
			if (pubSlots < 0)
				pubSlots = 0;

			// Clamp underflow based on minimum slots needed for gametype (2, except in powerduel):
			if (pubSlots + privSlots < 2 && gt != 2)
				pubSlots = 2 - privSlots;
			else if (pubSlots + privSlots < 3 && gt == 2)
				pubSlots = 3 - privSlots;

			// Clamp overflow:
			if (pubSlots > maxSlots)
				pubSlots = maxSlots;

			// Reduce other field if necessary:
			if (pubSlots + privSlots > maxSlots)
				privSlots = maxSlots - pubSlots;

			// If we decreased too much, decrease min players
//			if (pubSlots + privSlots < trap_Cvar_VariableValue( "bot_minplayers" ))
//				trap_Cvar_SetValue( "bot_minplayers", pubSlots + privSlots );

			// Also need to check if current bot counts are now too high:
			int red = trap_Cvar_VariableValue( "ui_numRedBots" );
			int blue = trap_Cvar_VariableValue( "ui_numBlueBots" );
			int maxBots = pubSlots + privSlots - UI_NumClients();
			if (red + blue > maxBots)
			{
				// Decrease red only, if possible, otherwise, both
				if (blue <= maxBots)
					trap_Cvar_SetValue( "ui_numRedBots", maxBots - blue );
				else
				{
					trap_Cvar_SetValue( "ui_numRedBots", 0 );
					trap_Cvar_SetValue( "ui_numBlueBots", maxBots );
				}
			}

			// Commit changes to slot counts:
			trap_Cvar_SetValue( "ui_publicSlots", pubSlots );
			trap_Cvar_SetValue( "ui_privateSlots", privSlots );
		}
		else if (Q_stricmp(name, "xblUpdatePrivateSlots") == 0)
		{
			// This no longer supports wrapping - there's too many ambiguous
			// situations. Instead, we clamp values at the min/maximum:
			int pubSlots = trap_Cvar_VariableValue( "ui_publicSlots" );
			int privSlots = trap_Cvar_VariableValue( "ui_privateSlots" );
			int maxSlots = trap_Cvar_VariableValue( "ui_dedicated" ) ? 10 : 8;
			int gt = trap_Cvar_VariableValue( "ui_netGameType" );

			// Clamp underflow:
			if (privSlots < 0)
				privSlots = 0;

			// Clamp underflow based on minimum slots needed for gametype (2, except in powerduel):
			if (pubSlots + privSlots < 2 && gt != 2)
				privSlots = 2 - pubSlots;
			else if (pubSlots + privSlots < 3 && gt == 2)
				privSlots = 3 - pubSlots;

			// Clamp overflow:
			if (privSlots > maxSlots)
				privSlots = maxSlots;

			// Reduce other field if necessary:
			if (pubSlots + privSlots > maxSlots)
				pubSlots = maxSlots - privSlots;

			// If we decreased too much, decrease min players
//			if (pubSlots + privSlots < trap_Cvar_VariableValue( "bot_minplayers" ))
//				trap_Cvar_SetValue( "bot_minplayers", pubSlots + privSlots );

			// Also need to check if current bot counts are now too high:
			int red = trap_Cvar_VariableValue( "ui_numRedBots" );
			int blue = trap_Cvar_VariableValue( "ui_numBlueBots" );
			int maxBots = pubSlots + privSlots - UI_NumClients();
			if (red + blue > maxBots)
			{
				// Decrease red only, if possible, otherwise, both
				if (blue <= maxBots)
					trap_Cvar_SetValue( "ui_numRedBots", maxBots - blue );
				else
				{
					trap_Cvar_SetValue( "ui_numRedBots", 0 );
					trap_Cvar_SetValue( "ui_numBlueBots", maxBots );
				}
			}

			// Commit changes to slot counts:
			trap_Cvar_SetValue( "ui_publicSlots", pubSlots );
			trap_Cvar_SetValue( "ui_privateSlots", privSlots );
		}
		else if (Q_stricmp(name, "xblUpdateMinPlayers") == 0)
		{
			assert( 0 );
/*
			int minPlayers = trap_Cvar_VariableValue( "bot_minplayers" );
			int pubSlots = trap_Cvar_VariableValue( "ui_publicSlots" );
			int privSlots = trap_Cvar_VariableValue( "ui_privateSlots" );

			// Underflow, pick current maximum value:
			if (minPlayers < 0)
				trap_Cvar_SetValue( "bot_minplayers", pubSlots + privSlots );

			// Overflow, wrap to zero:
			if (minPlayers > pubSlots + privSlots)
				trap_Cvar_Set( "bot_minplayers", "0" );
*/
		}
		else if (Q_stricmp(name, "xblUpdateForceMastery") == 0)
		{
			// Pilfered code from UI_ForceMaxRank_HandleKey()
			int num = trap_Cvar_VariableValue( "ui_maxForceRank" );

			uiMaxRank = num;
			trap_Cvar_SetValue( "g_maxForceRank", num );

			// Hack:
			if( !num )
			{
				int forcePowerDisable;

				for( int i = 0; i < NUM_FORCE_POWERS; i++ )
					forcePowerDisable |= (1<<i);

				trap_Cvar_Set("g_forcePowerDisable", va("%i",forcePowerDisable));
			}
			else
				trap_Cvar_SetValue( "g_forcePowerDisable", 0 );

			// The update force used will remove overallocated powers automatically.
			UpdateForceUsed();
			gTouchedForce[ClientManager::ActiveClientNum()] = qtrue;
		}
#endif
//JLF uiscripts
		else if (Q_stricmp(name, "setarrow")==0)
		{
			const char *controlName ;
			const char *arrowControlName ;
			const char * controlText;
			int textwidth;
			int startx;
			itemDef_t * item;
			itemDef_t * arrowControl;
			menuDef_t *menu;
			menu = Menu_GetFocused();
			
			String_Parse(args, &controlName);
			String_Parse(args, &arrowControlName);
			//get the textwidth from control
			if (menu)
			{
				itemDef_t *item;
				item = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, controlName);
				if (*(item->text) == '@')	// string reference
				{		
					controlText = SE_GetString( &(item->text[1]) );
				}
				else
					controlText = item->text;
				textwidth = uiInfo.uiDC.textWidth( controlText, item->textscale, item->iMenuFont );
				startx = item->window.rect.x;
				arrowControl = (itemDef_s *) Menu_FindItemByName((menuDef_t *) menu, arrowControlName);
				setArrowX(arrowControl, textwidth + startx+ ARROW_SPACE);
			}
			

		}
	





#ifdef _XBOX
		
		else if (Q_stricmp(name, "setUIClient")==0)
		{
			const char *clientnum;
			if (!String_Parse(args, &clientnum))
				return;
			uiClientNum = atoi(clientnum);

#ifdef _XBOX
			UI_SetPlayerName();
#endif

			ClientManager::ActivateClient(uiClientNum);
		}
		else if ((Q_stricmp(name, "setClientInputClosed")==0))
		{
			const char * clientcontrollerInputClosed;
			if (!String_Parse(args, &clientcontrollerInputClosed))
				return;
			storedClientInputClosed = uiclientInputClosed;
			uiclientInputClosed = atoi(clientcontrollerInputClosed);
			if ( uiclientInputClosed == 0)
			{
				if (clientcontrollerInputClosed[0] != '0')
				{
					uiclientInputClosed = (int)trap_Cvar_VariableValue(clientcontrollerInputClosed);
					UI_setClientInputClosed(uiclientInputClosed);
				}
			}

		}
		else if ((Q_stricmp(name, "restoreClientInputClosed")==0))
		{
			uiclientInputClosed = storedClientInputClosed;
		}
		else if ((Q_stricmp(name, "closeControllerMenu")==0))
		{	
			Cvar_Set("ControllerOutNum","-1");
			Menus_CloseByName("noController");
		}
		else if (Q_stricmp(name, "setForceSide") == 0)
		{

			cvarValue = trap_Cvar_VariableValue("ui_forceSideCvar");

			Cvar_SetValue("ui_forceConfigCvar", 0 );
			UI_FeederSelection(FEEDER_FORCECFG, 0, NULL); 
			
			UI_ForceSide_HandleKey(0, 0, A_CURSOR_RIGHT, (cvarValue==1)? 2:1 , 1 , 2 ,UI_FORCE_SIDE);
		
			UI_SwitchForceSide(cvarValue, 1, 2);
		}

		else if (Q_stricmp(name, "setForceConfig") == 0)
		{
			cvarValue = trap_Cvar_VariableValue("ui_forceConfigCvar");
			UI_FeederSelection(FEEDER_FORCECFG, cvarValue, NULL); 
		}

		else if (Q_stricmp(name, "setupForceScreen") == 0)
		{
			cvarValue = trap_Cvar_VariableValue("ui_forceSideCvar");
			UI_SwitchForceSide(cvarValue, 1, 2);
		}
		else if (Q_stricmp(name, "testMenuProgression") == 0)
		{
			UI_testMenuProgression(args);
		}
		else if (Q_stricmp(name, "spectate") == 0)
		{
			AddDeferedCommand(va("team s\n"));
		//	trap_Cmd_ExecuteText( EXEC_APPEND,  va("team s\n"));
		}
				//	show		typebut_single
			//	hide		typebut_dual
			//	hide		typebut_staff
			//	setfocus	typebut_single
		else if (Q_stricmp(name, "saberCustomFocus") == 0)
		{
			menuDef_t*	menu			= Menu_GetFocused();
			itemDef_t*	item;
			const char*	ui_saber_type	= Cvar_Get("ui_saber_type", "single", 0)->string;
			if(Q_stricmp(ui_saber_type, "single") == 0)
			{
				Menu_ShowItemByName(menu, "typebut_single", qtrue);
				item	= Menu_FindItemByName(menu, "typebut_single");
				Item_SetFocus(item, 0, 0);
				Item_RunScript(item, item->onFocus);
				
				Menu_ShowItemByName(menu, "typebut_dual", qfalse);
				Menu_ShowItemByName(menu, "typebut_staff", qfalse);
			}
			else if(Q_stricmp(ui_saber_type, "dual") == 0)
			{
				Menu_ShowItemByName(menu, "typebut_dual", qtrue);
				item	= Menu_FindItemByName(menu, "typebut_dual");
				Item_SetFocus(item, 0, 0);
				Item_RunScript(item, item->onFocus);

				Menu_ShowItemByName(menu, "typebut_single", qfalse);
				
				Menu_ShowItemByName(menu, "typebut_staff", qfalse);
				
			}
			else if(Q_stricmp(ui_saber_type, "staff") == 0)
			{
				Menu_ShowItemByName(menu, "typebut_staff", qtrue);
				item	= Menu_FindItemByName(menu, "typebut_staff");
				Item_SetFocus(item, 0, 0);
				Item_RunScript(item, item->onFocus);

				Menu_ShowItemByName(menu, "typebut_single", qfalse);
				Menu_ShowItemByName(menu, "typebut_dual", qfalse);
				
			}
			else
			{
				Cvar_Set("ui_saber_type", "single");
				Menu_ShowItemByName(menu, "typebut_single", qtrue);
				item	= Menu_FindItemByName(menu, "typebut_single");
				Item_SetFocus(item, 0, 0);
				Item_RunScript(item, item->onFocus);

				Menu_ShowItemByName(menu, "typebut_dual", qfalse);
				Menu_ShowItemByName(menu, "typebut_staff", qfalse);
			}
		}
		else if (Q_stricmp(name, "setplayertext") == 0)
		{
			if (! ClientManager::splitScreenMode )
				Cvar_Set("playertext", "");
			else if ( ClientManager::ActiveClientNum() ==0)
				Cvar_Set("playertext", "@MENUS_P1");
			else
				Cvar_Set("playertext", "@MENUS_P2");
		}


// JLF Menu Progression
		else if (Q_stricmp(name, "startMenuProgression") == 0)
		{
			const char * chain;
			if (!String_Parse(args, &chain))
				return;

			if (Q_stricmp(chain, "Join") == 0)
			{		
			//	trap_Cvar_SetValue( "ui_menuClient", 0 );
			//	uiClientNum = 0;
				//test game type
				if ( ClientManager::splitScreenMode)
				{
					Cvar_Set("ui_menuProgression", "joingame2player");
					if ( trap_Cvar_VariableValue( "spectateSelected" ) ==1)
					{
						if (uiClientNum == 0)
						{
							Menus_CloseAll();
							Menus_OpenByName("ingame_Player2");
					//		player1spectate = qtrue;
						}
						else
						{
							itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
							Item_RunScript(itemp, " 	uiScript	xb_force confirm ; 	uiScript closeingame");
						}

					}
				//	else
				//	{
				//		if ( uiClientNum == 0)
				//		{
				//			player1spectate = qfalse;
				//		}
				//	}
						
					
						

				}
				else
				{
					Cvar_Set("ui_menuProgression", "joingame1player");
					if ( trap_Cvar_VariableValue( "spectateSelected" ) ==1)
					{
						UI_closeInGame();
					}

				}
			}		
			else if (Q_stricmp(chain, "ChangeTeam") == 0)
			{
				Cvar_Set ("ui_menuProgression", "ChangeTeam");
			}
		}

// END menu Progression

		else if (Q_stricmp(name, "showhideingamepause") == 0)
		{
			int gametype = trap_Cvar_VariableValue( "g_gametype" );

			// Duel or Siege - no explicit control of spectating (siege has other controls)
			if( gametype == GT_DUEL || gametype == GT_SIEGE )
			{
				Cvar_SetValue( "IngameJoinTeamButtonOn", 0 );
				Cvar_SetValue( "IngameJoinGameButtonOn", 0 );
				Cvar_SetValue( "SpectateOn", 0 );
				return;
			}

			// Team game (including PowerDuel), but not Siege - show the Change Team button
			if( gametype == GT_TEAM || gametype == GT_CTF || gametype == GT_POWERDUEL )
			{
				Cvar_SetValue( "IngameJoinTeamButtonOn", 1 );
				Cvar_SetValue( "IngameJoinGameButtonOn", 0 );
				Cvar_SetValue( "SpectateOn", 0 );
				return;
			}

			// FFA - Show one of the buttons:
			if( gametype == GT_FFA )
			{
				// No change team button:
				Cvar_SetValue( "IngameJoinTeamButtonOn", 0 );

				// If we're a spectator now, show "Join Game":
				if (ClientManager::ActiveClient().isSpectating)
				{
					Cvar_SetValue( "SpectateOn", 0 );
					Cvar_SetValue( "IngameJoinGameButtonOn", 1 );
				}
				else
				{
					Cvar_SetValue( "SpectateOn", 1 );
					Cvar_SetValue( "IngameJoinGameButtonOn", 0 );
				}

				return;
			}

			// Should never reach here. What game *are* we playing?
			assert( 0 );
		}
		else if (Q_stricmp(name, "testbot") == 0)
		{
			int pubslots =trap_Cvar_VariableValue( "ui_publicSlots" );
			int privslots =trap_Cvar_VariableValue( "ui_privateSlots" );
			int bot_min = trap_Cvar_VariableValue( "bot_minplayers" );
			if ( pubslots == bot_min && privslots == 0) //we have a bot game
			{
				int gametype = trap_Cvar_VariableValue("g_gametype");
				if ( gametype == GT_FFA || gametype == GT_DUEL)
				{
					itemDef_t * itemp = Menu_FindItemByName(Menu_GetFocused(), "title");
					Item_RunScript(itemp, " delay 1 ; setcvar xb_joinTeam 0 ; uiscript startMenuProgression Join ; setcvar xb_returnMenu ingame_player ; setCvar xb_saberAcceptMenu ingame_playerforce ; close all ; open	characterModelSelection ");
				}
			}
		}
		else if (Q_stricmp(name, "testrespawnneeded") == 0)
		{
			int respawnneeded = trap_Cvar_VariableValue("ui_respawnneeded");
			if ( respawnneeded && cgs.gametype != GT_DUEL && cgs.gametype != GT_POWERDUEL )
			{
				UI_xboxErrorPopup( XB_POPUP_RESPAWN_NEEDED );
				Cvar_Set("ui_respawnneeded","0");
			}
			else
			{
				Menus_CloseAll();
				Menus_OpenByName("ingame");
			}
				
		}
		

		else if (Q_stricmp(name, "genericpopup") == 0)
		{
			const char *menuid;
			String_Parse(args, &menuid);
			if(Q_stricmp(menuid, "quitconfirm") == 0)
			{

				UI_xboxErrorPopup( XB_POPUP_QUIT_CONFIRM );
			}
		
		}	

		else if (Q_stricmp(name, "quitgame") == 0)
		{
			//is the server
			if (com_sv_running->integer)
			{
				if (trap_Cvar_VariableValue( "xb_gameType" ) ==0)//bot match
					UI_xboxErrorPopup( XB_POPUP_QUIT_CONFIRM );
				else if(ClientManager::splitScreenMode == qtrue)
					UI_xboxErrorPopup( XB_POPUP_QUIT_CONFIRM );
				else
					UI_xboxErrorPopup( XB_POPUP_QUIT_HOST_CONFIRM );
			}
			else
				UI_xboxErrorPopup( XB_POPUP_QUIT_CONFIRM );

		}
		
//JLF END		
		else if (Q_stricmp(name, "getControls") == 0)
		{
			int clNum = ClientManager::ActiveClientNum();

			// Copy basic controls screen vars into ui ones

			// Inverted aim:
			Cvar_SetValue( "ui_mousePitch", Settings.invertAim[clNum] ? 0 : 1 );

			// Thumbsticks:
			Cvar_SetValue( "ui_thumbStickMode", Settings.thumbstickMode[clNum] );

			// Buttons:
			if( Settings.buttonMode[clNum] == 0 )
				Cvar_Set( "ui_buttonconfig", "weaponsbias" );
			else if( Settings.buttonMode[clNum] == 1 )
				Cvar_Set( "ui_buttonconfig", "forcebias" );
			else
				Cvar_Set( "ui_buttonconfig", "southpaw" );

			// Triggers:
			if( Settings.triggerMode[clNum] == 0 )
				Cvar_Set( "ui_triggerconfig", "default" );
			else
				Cvar_Set( "ui_triggerconfig", "southpaw" );
		}
		else if (Q_stricmp(name, "setControls") == 0)
		{
			int clNum = ClientManager::ActiveClientNum();

			// Inverted aim:
			Settings.invertAim[clNum] = !Cvar_VariableIntegerValue( "ui_mousePitch" );
			ClientManager::ActiveClient().cg_pitch = Settings.invertAim[clNum] ? 0.022 : -0.022;

			// Thumbsticks:
			Settings.thumbstickMode[clNum] = Cvar_VariableValue( "ui_thumbStickMode" );

			// Buttons:
			if( Q_stricmp( Cvar_VariableString( "ui_buttonconfig" ), "weaponsbias" ) == 0 )
				Settings.buttonMode[clNum] = 0;
			else if( Q_stricmp( Cvar_VariableString( "ui_buttonconfig" ), "forcebias" ) == 0 )
				Settings.buttonMode[clNum] = 1;
			else
				Settings.buttonMode[clNum] = 2;
			Cbuf_ExecuteText( EXEC_APPEND, va("exec cfg/uibuttonConfig%d.cfg\n", Settings.buttonMode[clNum]) );

			// Triggers:
			if( Q_stricmp( Cvar_VariableString( "ui_triggerconfig" ), "default" ) == 0 )
				Settings.triggerMode[clNum] = 0;
			else
				Settings.triggerMode[clNum] = 1;
			Cbuf_ExecuteText( EXEC_APPEND, va("exec cfg/triggersConfig%d.cfg\n", Settings.triggerMode[clNum]) );

			Settings.Save();
		}
		else if (Q_stricmp(name, "getsettingscvars") == 0)
		{
			// Fetches everything on the advanced controls screen:
			Cvar_SetValue( "ui_autolevel", ClientManager::ActiveClient().cg_autolevel );
			Cvar_SetValue( "ui_autoswitch",  ClientManager::ActiveClient().cg_autoswitch );

			if( ClientManager::ActiveClientNum() == 0 )
				Cvar_SetValue( "ui_useRumble", Cvar_VariableIntegerValue( "in_useRumble" ) );
			else
				Cvar_SetValue( "ui_useRumble", Cvar_VariableIntegerValue( "in_useRumble2" ) );
			
			Cvar_Set( "ui_sensitivity", va("%f", ClientManager::ActiveClient().cg_sensitivity) );
			Cvar_Set( "ui_sensitivityY", va("%f", ClientManager::ActiveClient().cg_sensitivityY) );
		}
		else if (Q_stricmp(name, "updatesettingscvars") == 0)
		{
			int clNum = ClientManager::ActiveClientNum();

			Settings.autolevel[clNum] = Cvar_VariableIntegerValue( "ui_autolevel" );
			ClientManager::ActiveClient().cg_autolevel = Settings.autolevel[clNum];

			Settings.autoswitch[clNum] = Cvar_VariableValue( "ui_autoswitch" );
			ClientManager::ActiveClient().cg_autoswitch = Settings.autoswitch[clNum];

			if( ClientManager::ActiveClientNum() == 0 )
			{
				Settings.rumble[0] = Cvar_VariableIntegerValue( "ui_useRumble" );
				Cvar_SetValue( "in_useRumble", Settings.rumble[0] );
			}
			else
			{
				Settings.rumble[1] = Cvar_VariableIntegerValue( "ui_useRumble" );
				Cvar_SetValue( "in_useRumble2", Settings.rumble[1] );
			}

			Settings.sensitivityX[clNum] = Cvar_VariableValue( "ui_sensitivity" );
			ClientManager::ActiveClient().cg_sensitivity = Settings.sensitivityX[clNum];

			Settings.sensitivityY[clNum] = Cvar_VariableValue( "ui_sensitivityY" );
			ClientManager::ActiveClient().cg_sensitivityY = Settings.sensitivityY[clNum];

			Settings.Save();
		}
		else if (Q_stricmp(name, "syslink") == 0)
		{
			// A system link command, either "listen" or "halt"
			const char *tokenAction;
			if (!String_Parse(args, &tokenAction) || !tokenAction)
				return;

			if( Q_stricmp(tokenAction, "listen") == 0)
			{
				Syslink_Listen( true );
				Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
			}
			else if( Q_stricmp(tokenAction, "halt") == 0)
			{
				Syslink_Listen( false );
			}
		}
		else if (Q_stricmp(name, "updatevolume") == 0)
		{
			// Store all settings from the audio page:
			Settings.effectsVolume = Cvar_VariableValue( "s_effects_volume" );
			Settings.musicVolume = Cvar_VariableValue( "s_music_volume" );
			Settings.voiceVolume = Cvar_VariableValue( "s_voice_volume" );
			Settings.brightness = Cvar_VariableValue( "s_brightness_volume" );

			extern void GLimp_SetGamma(float);
			GLimp_SetGamma(Cvar_VariableValue( "s_brightness_volume" ) / 5.0f);

			Settings.Save();
		}
		else if (Q_stricmp(name, "brightnessChanged") == 0)
		{
			extern void GLimp_SetGamma(float);
			GLimp_SetGamma(Cvar_VariableValue( "s_brightness_volume" ) / 5.0f);
		}

		else if (Q_stricmp(name, "softkeyboardinit") == 0)
		{
			UI_SoftKeyboardInit();
		}

		else if (Q_stricmp(name, "continueSlowJoin") == 0) 
        {
            XBL_MM_JoinLowQOSSever();      
        }
		else if (Q_stricmp(name, "cancelSlowJoin") == 0) 
        {
            XBL_MM_DontJoinLowQOSSever();      
        }
		else if (Q_stricmp(name, "xbStartup") == 0)
		{
			// Called when the first MP menu is opened. Does various boot-time things:
			// Load settings file, check for invite, etc...

			// If we've already done this once, don't do it again (otherwise
			// the user can't back out once they have an invite):
			static bool doneOnce = false;
			if (doneOnce)
				return;
			doneOnce = true;

			XB_Startup( STARTUP_LOAD_SETTINGS );
		}
		else if (Q_stricmp(name, "joinStoredInvite") == 0)
		{
			// Called when the XBL lobby menu is opened, continuing the above
			// sequence. This should actually join the pending invite game,
			// if there is one.
			XONLINE_ACCEPTED_GAMEINVITE *pInvite = Sys_AcceptedInvite();

			// If there's no invite, do nothing here (common case)
			if (!pInvite)
				return;

			// If we've already done this once, don't do it again (otherwise
			// the user can't avoid joining once they have an invite):
			static bool doneOnce = false;
			if (doneOnce)
				return;
			doneOnce = true;

			// There's an invite waiting. Now we try to connect:
			bool invited = (pInvite->InvitingFriend.dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE);
			XBL_MM_SetJoinType( invited ? VIA_FRIEND_INVITE : VIA_FRIEND_JOIN );
			XBL_MM_ConnectViaSessionID( &pInvite->InvitingFriend.sessionID , invited );
		}
		else if (Q_stricmp(name, "xb_bot") == 0)
		{
			// Handles all the uiScripts for our fake-scrolling bot screen
			UI_XB_BotScript(args, name);
		}
		else if (Q_stricmp(name, "serverMap") == 0)
		{
			// Handles all uiScript commands for the server side map switching interface
			UI_ServerMap(args, name);
		}
		else if (Q_stricmp(name, "xb_force") == 0)
		{
			// Handles juggling of force configuration related cvars and such
			UI_XB_ForceScript(args, name);
		}
		else if (Q_stricmp(name, "initmaps") == 0) {
			UI_LoadArenas();
			UI_MapCountByCurrentGameType();
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, "createserver");
		}
		else if (Q_stricmp(name, "transitionVoteMenu") == 0)
		{
			// Handles transition from ingame pause menu to vote menu. 
			UI_TransitionVoteMenu(args, name);
		}
		else if(Q_stricmp(name, "assignGameType") == 0)
		{
			UI_AssignGameType(args, name);
		}
		else if(Q_stricmp(name, "placevote") == 0)
		{
			if(String_Parse(args,&name))
			{
				if(Q_stricmp(name,"no") == 0)
				{
                    trap_Cmd_ExecuteText( EXEC_APPEND, va("vote no\n") );
					cgs.votePlaced = true;
				}
				else if(Q_stricmp(name,"yes") == 0)
				{
                    trap_Cmd_ExecuteText( EXEC_APPEND, va("vote yes\n") );
					cgs.votePlaced = true;
				}
				else if(Q_stricmp(name,"null") == 0)
				{
					cgs.votePlaced = true;
				}
				else
				{
					Com_Printf("unknown argument: placevote %s\n", name);
				}
			}
			Com_Printf("no argument: placevote\n");
		}
		else if(Q_stricmp(name, "resetscroll") == 0)
		{
			menuDef_t *menu = Menu_GetFocused();
			if(!menu)
				return;
			for( int i = 0; i < menu->itemCount; ++i )
			{
				if( menu->items[i]->type == ITEM_TYPE_TEXTSCROLL )
				{
					textScrollDef_t *scrollPtr = (textScrollDef_t*)menu->items[i]->typeData;
					scrollPtr->startPos = 0;
				}
			}
		}
		else if(Q_stricmp(name, "updatemoves") == 0)
		{
			UI_UpdateMoves();
		}
#endif
		else 
		{
			Com_Printf("unknown UI script %s\n", name);
		}
	}
}

static void UI_GetTeamColor(vec4_t *color) {
}

#include "../namespace_begin.h"
int BG_SiegeCountBaseClass(const int team, const short classIndex);
#include "../namespace_end.h"

static void UI_SiegeClassCnt( const int team )
{
	UI_SetSiegeTeams();

	trap_Cvar_Set("ui_infantry_cnt", va("%d", BG_SiegeCountBaseClass(team,0)));
	trap_Cvar_Set("ui_vanguard_cnt", va("%d", BG_SiegeCountBaseClass(team,1)));
	trap_Cvar_Set("ui_support_cnt", va("%d", BG_SiegeCountBaseClass(team,2)));
	trap_Cvar_Set("ui_jedi_cnt", va("%d", BG_SiegeCountBaseClass(team,3)));
	trap_Cvar_Set("ui_demo_cnt", va("%d", BG_SiegeCountBaseClass(team,4)));
	trap_Cvar_Set("ui_heavy_cnt", va("%d", BG_SiegeCountBaseClass(team,5)));

}

void UI_ClampMaxPlayers(void)
{
	char	buf[32];
	// min checks
	//
	if( uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_DUEL ) //DUEL
	{
		if( trap_Cvar_VariableValue("sv_maxClients") < 2 )
		{
			trap_Cvar_Set("sv_maxClients", "2");
		}
	}
	else if( uiInfo.gameTypes[ui_netGameType.integer].gtEnum == GT_POWERDUEL ) // POWER DUEL
	{
		if( trap_Cvar_VariableValue("sv_maxClients") < 3 )
		{
			trap_Cvar_Set("sv_maxClients", "3");
		}
	}

	
	// max check for all game types
	if( trap_Cvar_VariableValue("sv_maxClients") > MAX_CLIENTS )
	{
		sprintf(buf,"%d",MAX_CLIENTS);
		trap_Cvar_Set("sv_maxClients", buf);
	}

}

void	UI_UpdateSiegeStatusIcons(void)
{
    menuDef_t *menu = Menu_GetFocused();
	int i;

	menu = Menu_GetFocused();	// Get current menu

	if (!menu)
	{
		return;
	}

	for (i=0;i<7;i++)
	{

		Menu_SetItemBackground(menu,va("wpnicon0%d",i), va("*ui_class_weapon%d",i));
	}

	for (i=0;i<7;i++)
	{
		Menu_SetItemBackground(menu,va("itemicon0%d",i), va("*ui_class_item%d",i));
	}

	for (i=0;i<10;i++)
	{
		Menu_SetItemBackground(menu,va("forceicon0%d",i), va("*ui_class_power%d",i));
	}

	for (i=10;i<15;i++)
	{
		Menu_SetItemBackground(menu,va("forceicon%d",i), va("*ui_class_power%d",i));
	}

}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType(qboolean singlePlayer) {

	int i, c, game;
	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ui_gameType.integer].gtEnum : uiInfo.gameTypes[ui_netGameType.integer].gtEnum;

	if (game == GT_SINGLE_PLAYER) {
		game++;
	} 
	if (game == GT_TEAM) {
		game = GT_FFA;
	}
	if (game == GT_HOLOCRON || game == GT_JEDIMASTER) {
		game = GT_FFA;
	}

	for (i = 0; i < uiInfo.mapCount; i++) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << game)) {
			if (singlePlayer) {
				if (!(uiInfo.mapList[i].typeBits & (1 << GT_SINGLE_PLAYER))) {
					continue;
				}
			}
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

qboolean UI_hasSkinForBase(const char *base, const char *team) {
	char	test[1024];
	fileHandle_t	f;
	
	Com_sprintf( test, sizeof( test ), "models/players/%s/%s/lower_default.skin", base, team );
	trap_FS_FOpenFile(test, &f, FS_READ);
	if (f != 0) {
		trap_FS_FCloseFile(f);
		return qtrue;
	}
	Com_sprintf( test, sizeof( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );
	trap_FS_FOpenFile(test, &f, FS_READ);
	if (f != 0) {
		trap_FS_FCloseFile(f);
		return qtrue;
	}
	return qfalse;
}

/*
==================
UI_HeadCountByColor
==================
*/
static int UI_HeadCountByColor() {
	int i, c;
	char *teamname;

	c = 0;

	switch(uiSkinColor)
	{
		case TEAM_BLUE:
			teamname = "/blue";
			break;
		case TEAM_RED:
			teamname = "/red";
			break;
		default:
			teamname = "/default";
	}

	// Count each head with this color
	for (i=0; i<uiInfo.q3HeadCount; i++)
	{
		if (uiInfo.q3HeadNames[i] && strstr(uiInfo.q3HeadNames[i], teamname))
		{
			c++;
		}
	}
	return c;
}

/*
==================
UI_JoinServer
==================
*/
static void UI_JoinServer( void )
{
	trap_Cvar_Set("cg_thirdPerson", "0");
	ClientManager::ActiveClient().cg_thirdPerson = 0;

	trap_Cvar_Set("cg_cameraOrbit", "0");
	trap_Cvar_Set("ui_singlePlayerActive", "0");
	if (logged_on)
	{ // Live server
		XBL_MM_JoinServer();
	}
	else
	{ // System link
		SysLink_JoinServer();
	}	
}

int UI_SiegeClassNum(siegeClass_t *scl)
{
	int i = 0;

	while (i < bgNumSiegeClasses)
	{
		if (&bgSiegeClasses[i] == scl)
		{
			return i;
		}
		i++;
	}

	return 0;
}

void UI_SetSiegeTeams(void)
{
	char			info[MAX_INFO_VALUE];
	char			*mapname = NULL;
	char			levelname[MAX_QPATH];
	char			btime[1024];
	char			teams[2048];
	char			teamInfo[MAX_SIEGE_INFO_SIZE];
	char			team1[1024];
	char			team2[1024];
	int				len = 0;
	int				gametype;
	fileHandle_t	f;

	//Get the map name from the server info
	if (trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) ))
	{
		mapname = Info_ValueForKey( info, "mapname" );
	}

	if (!mapname || !mapname[0])
	{
		return;
	}

	gametype = atoi(Info_ValueForKey(info, "g_gametype"));

	//If the server we are connected to is not siege we cannot choose a class anyway
	if (gametype != GT_SIEGE)
	{
		return;
	}

	Com_sprintf(levelname, sizeof(levelname), "maps/%s.siege", mapname);

	if (!levelname || !levelname[0])
	{
		return;
	}

	len = trap_FS_FOpenFile(levelname, &f, FS_READ);

	if (!f || len >= MAX_SIEGE_INFO_SIZE)
	{
		return;
	}

	trap_FS_Read(siege_info, len, f);
	siege_info[len] = 0;	//ensure null terminated

	trap_FS_FCloseFile(f);

	//Found the .siege file

	if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
	{
		char buf[1024];

		trap_Cvar_VariableStringBuffer("cg_siegeTeam1", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			strcpy(team1, buf);
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team1", team1);
		}

		trap_Cvar_VariableStringBuffer("cg_siegeTeam2", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			strcpy(team2, buf);
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team2", team2);
		}
	}
	else
	{
		return;
	}

	//Set the team themes so we know what classes to make available for selection
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, btime);
		}
	}
	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, btime);
		}
	}

	siegeTeam1 = BG_SiegeFindThemeForTeam(SIEGETEAM_TEAM1);
	siegeTeam2 = BG_SiegeFindThemeForTeam(SIEGETEAM_TEAM2);

	//set the default description for the default selection
	if (!siegeTeam1 || !siegeTeam1->classes[0])
	{
		Com_Error(ERR_DROP, "Error loading teams in UI");
	}

	Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM1, 0, NULL);
	Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);
}

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount(float feederID) 
{
	int team,baseClass,count=0,i; 
	static char info[MAX_STRING_CHARS];

#ifdef _XBOX 
//JLF 
	static bool firstProfileListRequest = true;
#endif

	
	switch ( (int)feederID )
	{

		case FEEDER_SABER_SINGLE_INFO:

			for (i=0;i<MAX_SABER_HILTS;i++)
			{
				if (saberSingleHiltInfo[i])
				{
					count++;
				}
				else
				{//done
					break;
				}
			}
			return count;

		case FEEDER_SABER_STAFF_INFO:

			for (i=0;i<MAX_SABER_HILTS;i++)
			{
				if (saberStaffHiltInfo[i])
				{
					count++;
				}
				else
				{//done
					break;
				}
			}
			return count;

		case FEEDER_Q3HEADS:
			return UI_HeadCountByColor();

		case FEEDER_SIEGE_TEAM1:
			if (!siegeTeam1)
			{
				UI_SetSiegeTeams();
				if (!siegeTeam1)
				{
					return 0;
				}
			}
			return siegeTeam1->numClasses;
		case FEEDER_SIEGE_TEAM2:
			if (!siegeTeam2)
			{
				UI_SetSiegeTeams();
				if (!siegeTeam2)
				{
					return 0;
				}
			}
			return siegeTeam2->numClasses;

		case FEEDER_FORCECFG:
			if (uiForceSide == FORCE_LIGHTSIDE)
			{
				return uiInfo.forceConfigCount-uiInfo.forceConfigLightIndexBegin;
			}
			else
			{
				return uiInfo.forceConfigLightIndexBegin+1;
			}
			//return uiInfo.forceConfigCount;

		case FEEDER_MAPS:
		case FEEDER_ALLMAPS:
			return UI_MapCountByCurrentGameType() + 1;
			// Add one for the "Next Map" option
	
		case FEEDER_SERVERS:
			return Syslink_GetNumServers();
	
		case FEEDER_PLAYER_LIST:
			if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) 
			{
				uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
				UI_BuildPlayerList();
			}

			// We subtract one, to prevent including the server. But if the
			// server is dedicated, he won't show up in this count/list anyway:
			if (xbOnlineInfo.xbPlayerList[DEDICATED_SERVER_INDEX].isActive)
				return uiInfo.playerCount;
			else
				return uiInfo.playerCount - 1;

		case FEEDER_TEAM_LIST:
			if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) 
			{
				uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
				UI_BuildPlayerList();
			}
			return uiInfo.myTeamCount;

		case FEEDER_MOVES :

			for (i=0;i<MAX_MOVES;i++)
			{
				if (datapadMoveData[uiInfo.movesTitleIndex][i].title)
				{
					count++;
				}
			}

			return count;

		case FEEDER_MOVES_TITLES :
			return (MD_MOVE_TITLE_MAX);

		case FEEDER_PLAYER_SPECIES:
			return uiInfo.playerSpeciesCount;

		case FEEDER_PLAYER_SKIN_HEAD:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount;

		case FEEDER_PLAYER_SKIN_TORSO:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount;

		case FEEDER_PLAYER_SKIN_LEGS:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount;

		case FEEDER_COLORCHOICES:
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount;

		case FEEDER_SIEGE_BASE_CLASS:		
			team = (int)trap_Cvar_VariableValue("ui_team");
			baseClass = (int)trap_Cvar_VariableValue("ui_siege_class");

			if ((team == SIEGETEAM_TEAM1) || 
				(team == SIEGETEAM_TEAM2)) 
			{
				// Is it a valid base class?
				if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
				{
					return (BG_SiegeCountBaseClass( team, baseClass ));
				}
			}
			return 0;

		// Get the count of weapons
		case FEEDER_SIEGE_CLASS_WEAPONS:	
			//count them up
			for (i=0;i< WP_NUM_WEAPONS;i++)
			{
				trap_Cvar_VariableStringBuffer( va("ui_class_weapon%i", i), info, sizeof(info) );
				if (stricmp(info,"gfx/2d/select")!=0)
				{
					count++;
				}
			}
#ifdef _XBOX
			//count them up
			for (i=0;i< HI_NUM_HOLDABLE;i++)
			{
				trap_Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
				// A hack so health and ammo dispenser icons don't show up.
				if ((stricmp(info,"gfx/2d/select")!=0) && (stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
					(stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
				{
					count++;
				}
			}

			//count them up
			for (i=0;i< NUM_FORCE_POWERS;i++)
			{
				trap_Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
				if (stricmp(info,"gfx/2d/select")!=0)
				{
					count++;
				}
			}
#endif
			return count;

		// Get the count of inventory
		case FEEDER_SIEGE_CLASS_INVENTORY:	
			//count them up
			for (i=0;i< HI_NUM_HOLDABLE;i++)
			{
				trap_Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
				// A hack so health and ammo dispenser icons don't show up.
				if ((stricmp(info,"gfx/2d/select")!=0) && (stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
					(stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
				{
					count++;
				}
			}
			return count;

		// Get the count of force powers
		case FEEDER_SIEGE_CLASS_FORCE:	
			//count them up
			for (i=0;i< NUM_FORCE_POWERS;i++)
			{
				trap_Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
				if (stricmp(info,"gfx/2d/select")!=0)
				{
					count++;
				}
			}
			return count;

#ifdef _XBOX
		// Get the count of xbl accounts:
		case FEEDER_XBL_ACCOUNTS:
			{
				int numAccounts = XBL_GetNumAccounts( false );
//				int curSelection = XBL_GetSelectedAccountIndex();

				// Is our current selection invalid? Fix it.
//				if( curSelection && (curSelection >= numAccounts) )
//					Menu_SetFeederSelection( NULL, FEEDER_XBL_ACCOUNTS, numAccounts-1, NULL );
				return numAccounts;
			}

		// Number of active players, plus number in history list, plus one for divider
		case FEEDER_XBL_PLAYERS:
			return XBL_PL_GetNumPlayers() + 1;

		// Number of friends
		case FEEDER_XBL_FRIENDS:
			return XBL_F_GetNumFriends();

		// Number of results from an optimatch query
		case FEEDER_XBL_SERVERS:
			return XBL_MM_GetNumServers();

		//JLF
		case FEEDER_PROFILES :
		{
			if (s_playerProfile.fileCnt == -1 || firstProfileListRequest)
			{
				firstProfileListRequest = false;
				ReadSaveDirectoryProfiles();	//refresh
			//	UI_HandleLoadSelection();
			}
			return s_playerProfile.fileCnt;
		}	
	
#endif
	}

	return 0;
}

static const char *UI_SelectedMap(int index, int *actual) {
	int i, c;
	c = 0;
	*actual = 0;

	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (c == index) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			} else {
				c++;
			}
		}
	}
	return "";
}

/*
==================
UI_HeadCountByColor
==================
*/
static const char *UI_SelectedTeamHead(int index, int *actual) {
	char *teamname;
	int i,c=0;

	switch(uiSkinColor)
	{
		case TEAM_BLUE:
			teamname = "/blue";
			break;
		case TEAM_RED:
			teamname = "/red";
			break;
		default:
			teamname = "/default";
			break;
	}

	// Count each head with this color

	for (i=0; i<uiInfo.q3HeadCount; i++)
	{
		if (uiInfo.q3HeadNames[i] && strstr(uiInfo.q3HeadNames[i], teamname))
		{
			if (c==index)
			{
				*actual = i;
				return uiInfo.q3HeadNames[i];
			}
			else
			{
				c++;
			}
		}
	}
	return "";
}


static int UI_GetIndexFromSelection(int actual) {
	int i, c;
	c = 0;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (i == actual) {
				return c;
			}
				c++;
		}
	}
  return 0;
}

static const char *UI_FeederItemText(float feederID, int index, int column, 
									 qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3) {
	static char info[MAX_STRING_CHARS]; // don't change this size without changing the sizes inside the SaberProperName calls
	static char hostname[1024];
	static char clientBuff[32];
	static char needPass[32];
	static int lastColumn = -1;
	static int lastTime = 0;
	*handle1 = *handle2 = *handle3 = -1;

	if (feederID == FEEDER_SABER_SINGLE_INFO)
	{
		//char *saberProperName=0;
		UI_SaberProperNameForSaber( saberSingleHiltInfo[index], info );
		return info;
	}
	else if	(feederID == FEEDER_SABER_STAFF_INFO)
	{
		//char *saberProperName=0;
		UI_SaberProperNameForSaber( saberStaffHiltInfo[index], info );
		return info;
	}
	else if (feederID == FEEDER_Q3HEADS) {
		int actual;
		return UI_SelectedTeamHead(index, &actual);
	} 
	else if (feederID == FEEDER_SIEGE_TEAM1)
	{
		return ""; //nothing I guess, the description part can cover this
		/*
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return "";
			}
		}

		if (siegeTeam1->classes[index])
		{
			return siegeTeam1->classes[index]->name;
		}
		return "";
		*/
	}
	else if (feederID == FEEDER_SIEGE_TEAM2)
	{
		return ""; //nothing I guess, the description part can cover this
		/*
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return "";
			}
		}

		if (siegeTeam2->classes[index])
		{
			return siegeTeam2->classes[index]->name;
		}
		return "";
		*/
	}
	else if (feederID == FEEDER_FORCECFG) {
		if (index >= 0 && index < uiInfo.forceConfigCount) {
			if (index == 0)
			{ //always show "custom"
				return uiInfo.forceConfigNames[index];
			}
			else
			{
				if (uiForceSide == FORCE_LIGHTSIDE)
				{
					index += uiInfo.forceConfigLightIndexBegin;
					if (index < 0)
					{
						return NULL;
					}
					if (index >= uiInfo.forceConfigCount)
					{
						return NULL;
					}
					return uiInfo.forceConfigNames[index];
				}
				else if (uiForceSide == FORCE_DARKSIDE)
				{
					index += uiInfo.forceConfigDarkIndexBegin;
					if (index < 0)
					{
						return NULL;
					}
					if (index > uiInfo.forceConfigLightIndexBegin)
					{ //dark gets read in before light
						return NULL;
					}
					if (index >= uiInfo.forceConfigCount)
					{
						return NULL;
					}
					return uiInfo.forceConfigNames[index];
				}
				else
				{
					return NULL;
				}
			}
		}
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual;
		// The first index is for our hacked-in Next Map option. 
		if(index == 0)
			return "@MENUS_NEXT_MAP";
		else
			index--;
		return UI_SelectedMap(index, &actual);
	} else if (feederID == FEEDER_SERVERS) {
		if (index >= 0 && index < Syslink_GetNumServers())
		{
			switch (column)
			{
				case 0:	// Map
					return Syslink_GetServerMap( index );
				case 1:	// Players
					return Syslink_GetServerClients( index );
				case 2:	// Game
					return Syslink_GetServerGametype( index );
				case 3:	// Saber-Only Icon
					*handle1 = Syslink_GetServerSaberOnly( index );
					return "";
				case 4:	// Disable Force Icon
					*handle1 = Syslink_GetServerDisableForce( index );
					return "";
			}
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (xbOnlineInfo.xbPlayerList[DEDICATED_SERVER_INDEX].isActive)
		{
			// Dedicated server, this means that there is a full player list
			if (index >= 0 && index < uiInfo.playerCount) {
				return uiInfo.playerNames[index];
			}
		}
		else
		{
			// Non-dedicated server, so they'll be first player. Skip them:
			if (index >= 0 && index < uiInfo.playerCount - 1) {
				return uiInfo.playerNames[index + 1];
			}
		}
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (index >= 0 && index < uiInfo.myTeamCount) {
			return uiInfo.teamNames[index];
		}
	} else if (feederID == FEEDER_MOVES) 
	{
		return datapadMoveData[uiInfo.movesTitleIndex][index].title;
	}
	else if (feederID == FEEDER_MOVES_TITLES) 
	{
		return datapadMoveTitleData[index];
	}
	else if (feederID == FEEDER_PLAYER_SPECIES) 
	{
		return uiInfo.playerSpecies[index].Name;
	} 
	else if (feederID == FEEDER_LANGUAGES) 
	{
		return 0;
	} 
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount) 
		{
			*handle1 = trap_R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorShader[index]);
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorShader[index];
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount) 
		{
			*handle1 = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index];
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount) 
		{
			*handle1 = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoNames[index]));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoNames[index];
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount) 
		{
			*handle1 = trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[index]));
			return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[index];
		}
	}
	else if (feederID == FEEDER_SIEGE_BASE_CLASS)
	{
		return ""; 
	}
	else if (feederID == FEEDER_SIEGE_CLASS_WEAPONS)
	{
		return ""; 
	}
#ifdef _XBOX
	else if (feederID == FEEDER_XBL_ACCOUNTS)
	{
		// VVFIXME - SOF2 keeps track of old number of accounts, to force a
		// refresh when someone yanks an MU. Probably necessary
		int numEntries = XBL_GetNumAccounts( false );
		if (index >= 0 && index < numEntries)
		{
			XONLINE_USER *pUser = XBL_GetUserInfo( index );
			if (pUser)
			{
				static char displayName[XONLINE_GAMERTAG_SIZE];
				strcpy( displayName, pUser->szGamertag );
				return displayName;
			}
		}
	}
	else if (feederID == FEEDER_XBL_PLAYERS)
	{
		int numEntries = XBL_PL_GetNumPlayers() + 1;

		if (index >= 0 && index < numEntries)
		{
			if (column == 0)
				return XBL_PL_GetPlayerName( index );
			else if (column == 1)
			{
				*handle1 = XBL_PL_GetStatusIcon( index );
				return "";
			}
			else if (column == 2)
			{
				*handle1 = XBL_PL_GetVoiceIcon( index );
				return "";
			}
			else
				return "";
		}
	}
	else if (feederID == FEEDER_XBL_FRIENDS)
	{
		if (index >= 0 && index < XBL_F_GetNumFriends())
		{
			if (column == 0)
				return XBL_F_GetFriendName( index );
			else if (column == 1)
			{
				*handle1 = XBL_F_GetStatusIcon( index );
				return "";
			}
			else if (column == 2)
			{
				*handle1 = XBL_F_GetVoiceIcon( index );
				return "";
			}
			else
				return "";
		}
	}
	else if (feederID == FEEDER_XBL_SERVERS)
	{
		// We handle the optimatch results listbox separately from the rest
		// of the UI server browser code. It's just nasty otherwise.
		if (index >= 0 && index < XBL_MM_GetNumServers())
		{
			switch (column)
			{
				case 0:	// Host
					return XBL_MM_GetServerName( index );
				case 1:	// Map
					return XBL_MM_GetServerMap( index );
				case 2:	// Players
					return XBL_MM_GetServerClients( index );
				case 3:	// Game
					return XBL_MM_GetServerGametype( index );
				case 4:	// Saber Only
					*handle1 = XBL_MM_GetServerSaberOnly( index );
					return "";
				case 5:	// Disable Force
					*handle1 = XBL_MM_GetServerDisableForce( index );
					return "";
				case 6:	// QoS
					*handle1 = XBL_MM_GetServerPing( index );
					return "";
			}
		}
	}

	//JLF
	else if (feederID == FEEDER_PROFILES)
	{
		if (column == 0)
		{
			return s_ProfileData[index].currentProfileName;
		}
	}

#endif

	return "";
}


static qhandle_t UI_FeederItemImage(float feederID, int index) {
	int	validCnt,i;
	static char info[MAX_STRING_CHARS];

	if (feederID == FEEDER_SABER_SINGLE_INFO) 
	{
		return 0;
	}
	else if (feederID == FEEDER_SABER_STAFF_INFO)
	{
		return 0;
	}
	else if (feederID == FEEDER_Q3HEADS) 
	{
		int actual;
		UI_SelectedTeamHead(index, &actual);
		index = actual;

		if (index >= 0 && index < uiInfo.q3HeadCount)
		{ //we want it to load them as it draws them, like the TA feeder
		      //return uiInfo.q3HeadIcons[index];
			int selModel = trap_Cvar_VariableValue("ui_selectedModelIndex");

			if (selModel != -1)
			{
				if (uiInfo.q3SelectedHead != selModel)
				{
					uiInfo.q3SelectedHead = selModel;
					//UI_FeederSelection(FEEDER_Q3HEADS, uiInfo.q3SelectedHead);
					Menu_SetFeederSelection(NULL, FEEDER_Q3HEADS, selModel, NULL);
				}
			}

			if (!uiInfo.q3HeadIcons[index])
			{ //this isn't the best way of doing this I guess, but I didn't want a whole seperate string array
			  //for storing shader names. I can't just replace q3HeadNames with the shader name, because we
			  //print what's in q3HeadNames and the icon name would look funny.
				char iconNameFromSkinName[256];
				int i = 0;
				int skinPlace;

				i = strlen(uiInfo.q3HeadNames[index]);

				while (uiInfo.q3HeadNames[index][i] != '/')
				{
					i--;
				}

				i++;
				skinPlace = i; //remember that this is where the skin name begins

				//now, build a full path out of what's in q3HeadNames, into iconNameFromSkinName
				Com_sprintf(iconNameFromSkinName, sizeof(iconNameFromSkinName), "models/players/%s", uiInfo.q3HeadNames[index]);

				i = strlen(iconNameFromSkinName);

				while (iconNameFromSkinName[i] != '/')
				{
					i--;
				}
				
				i++;
				iconNameFromSkinName[i] = 0; //terminate, and append..
				Q_strcat(iconNameFromSkinName, 256, "icon_");

				//and now, for the final step, append the skin name from q3HeadNames onto the end of iconNameFromSkinName
				i = strlen(iconNameFromSkinName);

				while (uiInfo.q3HeadNames[index][skinPlace])
				{
					iconNameFromSkinName[i] = uiInfo.q3HeadNames[index][skinPlace];
					i++;
					skinPlace++;
				}
				iconNameFromSkinName[i] = 0;

				//and now we are ready to register (thankfully this will only happen once)
				uiInfo.q3HeadIcons[index] = trap_R_RegisterShaderNoMip(iconNameFromSkinName);
			}
			return uiInfo.q3HeadIcons[index];
		}
    }
	else if (feederID == FEEDER_SIEGE_TEAM1) 
	{
		if (!siegeTeam1)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam1)
			{
				return 0;
			}
		}

		if (siegeTeam1->classes[index])
		{
			return siegeTeam1->classes[index]->uiPortraitShader;
		}
		return 0;
	}
	else if (feederID == FEEDER_SIEGE_TEAM2) 
	{
		if (!siegeTeam2)
		{
			UI_SetSiegeTeams();
			if (!siegeTeam2)
			{
				return 0;
			}
		}

		if (siegeTeam2->classes[index])
		{
			return siegeTeam2->classes[index]->uiPortraitShader;
		}
		return 0;
	}
	else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS) 
	{
		int actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount) {
			if (uiInfo.mapList[index].levelShot == -1) {
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount) 
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadIcons[index];
			return trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]));
		}
	} 
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount) 
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoIcons[index];
			return trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoNames[index]));
		}
	} 
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount) 
		{
			//return uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegIcons[index];
			return trap_R_RegisterShaderNoMip(va("models/players/%s/icon_%s", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].Name, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[index]));
		}
	} 
	else if (feederID == FEEDER_COLORCHOICES)
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount) 
		{
			return trap_R_RegisterShaderNoMip( uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorShader[index]);
		}
	}

	else if ( feederID == FEEDER_SIEGE_BASE_CLASS)
	{
		int team,baseClass; 

		team = (int)trap_Cvar_VariableValue("ui_team");
		baseClass = (int)trap_Cvar_VariableValue("ui_siege_class");

		if ((team == SIEGETEAM_TEAM1) || 
			(team == SIEGETEAM_TEAM2)) 
		{
			// Is it a valid base class?
			if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
			{
				if (index >= 0) 
				{
					return(BG_GetUIPortrait(team, baseClass, index));
				}
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_WEAPONS)
	{
		validCnt = 0;
		//count them up
		for (i=0;i< WP_NUM_WEAPONS;i++)
		{
			trap_Cvar_VariableStringBuffer( va("ui_class_weapon%i", i), info, sizeof(info) );
			if (stricmp(info,"gfx/2d/select")!=0)
			{
				if (validCnt == index)
				{
					return(trap_R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}

#ifdef _XBOX
		//count them up
		for (i=0;i< HI_NUM_HOLDABLE;i++)
		{
			trap_Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
			// A hack so health and ammo dispenser icons don't show up.
			if ((stricmp(info,"gfx/2d/select")!=0) && (stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
				(stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
			{
				if (validCnt == index)
				{
					return(trap_R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}

		//count them up
		for (i=0;i< NUM_FORCE_POWERS;i++)
		{
			trap_Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
			if (stricmp(info,"gfx/2d/select")!=0)
			{
				if (validCnt == index)
				{
					return(trap_R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
#endif


	}
	else if ( feederID == FEEDER_SIEGE_CLASS_INVENTORY)
	{
		validCnt = 0;
		//count them up
		for (i=0;i< HI_NUM_HOLDABLE;i++)
		{
			trap_Cvar_VariableStringBuffer( va("ui_class_item%i", i), info, sizeof(info) );
			// A hack so health and ammo dispenser icons don't show up.
			if ((stricmp(info,"gfx/2d/select")!=0) && (stricmp(info,"gfx/hud/i_icon_healthdisp")!=0) &&
				(stricmp(info,"gfx/hud/i_icon_ammodisp")!=0))
			{
				if (validCnt == index)
				{
					return(trap_R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
	}
	else if ( feederID == FEEDER_SIEGE_CLASS_FORCE)
	{
		int slotI=0;
		static char info2[MAX_STRING_CHARS];
		menuDef_t *menu;
		itemDef_t *item;


		validCnt = 0;


		menu = Menu_GetFocused();	// Get current menu
		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "base_class_force_feed");
			if (item)
			{
				listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
				if (listPtr)
				{
					slotI = listPtr->startPos;
				}
			}
		}

		//count them up
		for (i=0;i< NUM_FORCE_POWERS;i++)
		{
			trap_Cvar_VariableStringBuffer( va("ui_class_power%i", i), info, sizeof(info) );
			if (stricmp(info,"gfx/2d/select")!=0)
			{
				if (validCnt == index)
				{
					trap_Cvar_VariableStringBuffer( va("ui_class_powerlevel%i", validCnt), info2, sizeof(info2) );

					trap_Cvar_Set(va("ui_class_powerlevelslot%i", index-slotI), info2);
					return(trap_R_RegisterShaderNoMip(info));
				}
				validCnt++;
			}
		}
	}

  return 0;
}



//called every time a class is selected from a feeder, sets info
//for shaders to be displayed in the menu about the class -rww
extern qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType );
void UI_SiegeSetCvarsForClass(siegeClass_t *scl)
{
	int i = 0;
	int count = 0;
	char shader[MAX_QPATH];

	//let's clear the things out first
	while (i < WP_NUM_WEAPONS)
	{
		trap_Cvar_Set(va("ui_class_weapon%i", i), "gfx/2d/select");
		i++;
	}
	//now for inventory items
	i = 0;
	while (i < HI_NUM_HOLDABLE)
	{
		trap_Cvar_Set(va("ui_class_item%i", i), "gfx/2d/select");
		i++;
	}
	//now for force powers
	i = 0;
	while (i < NUM_FORCE_POWERS)
	{
		trap_Cvar_Set(va("ui_class_power%i", i), "gfx/2d/select");
		i++;
	}

	//now health and armor
	trap_Cvar_Set("ui_class_health", "0");
	trap_Cvar_Set("ui_class_armor", "0");

	trap_Cvar_Set("ui_class_icon", "");

	if (!scl)
	{ //no select?
		return;
	}

	//set cvars for which weaps we have
	i = 0;
	trap_Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
	while (i < WP_NUM_WEAPONS)
	{

		if (scl->weapons & (1<<i))
		{

			if (i == WP_SABER)
			{ //we want to see what kind of saber they have, and set the cvar based on that
				char saberType[1024];

				if (scl->saber1[0] &&
					scl->saber2[0])
				{
					strcpy(saberType, "gfx/hud/w_icon_duallightsaber");
				} //fixme: need saber data access on ui to determine if staff, "gfx/hud/w_icon_saberstaff"
				else
				{
					char buf[1024];
					if (scl->saber1[0] && UI_SaberTypeForSaber(scl->saber1, buf))
					{
						if ( !Q_stricmp( buf, "SABER_STAFF" ) )
						{
							strcpy(saberType, "gfx/hud/w_icon_saberstaff");
						}
						else
						{
							strcpy(saberType, "gfx/hud/w_icon_lightsaber");
						}
					}
					else
					{
						strcpy(saberType, "gfx/hud/w_icon_lightsaber");
					}
				}

				trap_Cvar_Set(va("ui_class_weapon%i", count), saberType);
				trap_Cvar_Set(va("ui_class_weapondesc%i", count), "@MENUS_AN_ELEGANT_WEAPON_FOR");
				count++;
				trap_Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
			}
			else
			{
				gitem_t *item = BG_FindItemForWeapon( i );
				trap_Cvar_Set(va("ui_class_weapon%i", count), item->icon);
				trap_Cvar_Set(va("ui_class_weapondesc%i", count), item->description);
				count++;
				trap_Cvar_Set(va("ui_class_weapondesc%i", count), " ");	// Blank it out to start with
			}
		}

		i++;
	}

	//now for inventory items
	i = 0;
	count = 0;

	while (i < HI_NUM_HOLDABLE)
	{
		if (scl->invenItems & (1<<i))
		{
			gitem_t *item = BG_FindItemForHoldable(i);
			trap_Cvar_Set(va("ui_class_item%i", count), item->icon);
			trap_Cvar_Set(va("ui_class_itemdesc%i", count), item->description);
			count++;
		}
		else
		{
			trap_Cvar_Set(va("ui_class_itemdesc%i", count), " ");
		}
		i++;
	}

	//now for force powers
	i = 0;
	count = 0;

	while (i < NUM_FORCE_POWERS)
	{
		trap_Cvar_Set(va("ui_class_powerlevel%i", i), "0");	// Zero this out to start.
		if (i<9)
		{
			trap_Cvar_Set(va("ui_class_powerlevelslot%i", i), "0");	// Zero this out to start.
		}

		if (scl->forcePowerLevels[i])
		{
			trap_Cvar_Set(va("ui_class_powerlevel%i", count), va("%i",scl->forcePowerLevels[i]));
			trap_Cvar_Set(va("ui_class_power%i", count), HolocronIcons[i]);
			count++;
		}

		i++;
	}

	//now health and armor
	trap_Cvar_Set("ui_class_health", va("%i", scl->maxhealth));
	trap_Cvar_Set("ui_class_armor", va("%i", scl->maxarmor));
	trap_Cvar_Set("ui_class_speed", va("%3.2f", scl->speed));

	//now get the icon path based on the shader index
	if (scl->classShader)
	{
		trap_R_ShaderNameFromIndex(shader, scl->classShader);
	}
	else
	{ //no shader
		shader[0] = 0;
	}
	trap_Cvar_Set("ui_class_icon", shader);
}

int g_siegedFeederForcedSet = 0;

void UI_UpdateCvarsForClass(const int team,const baseClass,const int index)
{
	siegeClass_t *holdClass=0;
	char *holdBuf;

	// Is it a valid team
	if ((team == SIEGETEAM_TEAM1) || 
		(team == SIEGETEAM_TEAM2)) 
	{

		// Is it a valid base class?
		if ((baseClass >= SPC_INFANTRY) && (baseClass < SPC_MAX))
		{
			// A valid index?
			if ((index>=0) && (index < BG_SiegeCountBaseClass( team, baseClass )))
			{
				if (!g_siegedFeederForcedSet)
				{
					holdClass = BG_GetClassOnBaseClass( team, baseClass, index);					
					if (holdClass)	//clicked a valid item
					{
						g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(holdClass);
						trap_Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);
						g_siegedFeederForcedSet = 1;
						Menu_SetFeederSelection(NULL, FEEDER_SIEGE_BASE_CLASS, -1, NULL);
						UI_SiegeSetCvarsForClass(holdClass);

						holdBuf = BG_GetUIPortraitFile(team, baseClass, index);
						if (holdBuf)
						{
							trap_Cvar_Set("ui_classPortrait",holdBuf);
						}
					}
				}
				g_siegedFeederForcedSet = 0;
			}
			else
			{
				trap_Cvar_Set("ui_classDesc", " ");
			}
		}
	}

}


qboolean UI_FeederSelection(float feederFloat, int index, itemDef_t *item) 
{
	static char info[MAX_STRING_CHARS];
	const int feederID = feederFloat;

	if (feederID == FEEDER_Q3HEADS) 
	{
		int actual;
		UI_SelectedTeamHead(index, &actual);
		uiInfo.q3SelectedHead = index;
		trap_Cvar_Set("ui_selectedModelIndex", va("%i", index));
		if(ClientManager::splitScreenMode)
		{
			if(ClientManager::ActiveClientNum() == 0)
			{
				uglyMug1	= index;
			}
			else
			{
				uglyMug2 = index;
			}
		}
		else
		{
			uglyMug1	= index;
		}

		index = actual;
		if (index >= 0 && index < uiInfo.q3HeadCount) 
		{
//#ifdef _XBOX
//			trap_Cvar_Set( "UImodel", uiInfo.q3HeadNames[index]);
//#else
			trap_Cvar_Set( "model", uiInfo.q3HeadNames[index]);
//#endif
			strcpy(ClientManager::ActiveClient().model, uiInfo.q3HeadNames[index]);
			ClientManager::ActiveClient().modelIndex = index;
			ClientManager::ActiveClient().cvar_modifiedFlags |= CVAR_USERINFO;
		}
	} 
	else if (feederID == FEEDER_MOVES) 
	{
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		menu = Menus_FindByName("rulesMenu_moves");

		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = (modelDef_t*)item->typeData;
				if (modelPtr)
				{
					char modelPath[MAX_QPATH];
					int animRunLength;

					ItemParse_model_g2anim_go( item,  datapadMoveData[uiInfo.movesTitleIndex][index].anim );

					Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );
					UI_UpdateCharacterSkin();

					uiInfo.moveAnimTime = uiInfo.uiDC.realTime + animRunLength;

					if (datapadMoveData[uiInfo.movesTitleIndex][index].anim)
					{

						// Play sound for anim
						if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_FORCE_JUMP)
						{
							trap_S_StartLocalSound( uiInfo.uiDC.Assets.moveJumpSound, CHAN_LOCAL );
						}
						else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_ROLL)
						{
							trap_S_StartLocalSound( uiInfo.uiDC.Assets.moveRollSound, CHAN_LOCAL );
						}
						else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_SABER)
						{
							// Randomly choose one sound
							int soundI = Q_irand( 1, 6 );
							sfxHandle_t *soundPtr;
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound1;
							if (soundI == 2)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound2;
							}
							else if (soundI == 3)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound3;
							}
							else if (soundI == 4)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound4;
							}
							else if (soundI == 5)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound5;
							}
							else if (soundI == 6)
							{
								soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound6;
							}

							trap_S_StartLocalSound( *soundPtr, CHAN_LOCAL );
						}

						if (datapadMoveData[uiInfo.movesTitleIndex][index].desc)
						{
							trap_Cvar_Set( "ui_move_desc", datapadMoveData[uiInfo.movesTitleIndex][index].desc);
						}
					}
					UI_SaberAttachToChar( item );
				}
			}
		}
	} 
	else if (feederID == FEEDER_MOVES_TITLES) 
	{
		itemDef_t *item;
		menuDef_t *menu;
		modelDef_t *modelPtr;

		uiInfo.movesTitleIndex = index;
		uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
		menu = Menus_FindByName("rulesMenu_moves");

		if (menu)
		{
			item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
			if (item)
			{
				modelPtr = (modelDef_t*)item->typeData;
				if (modelPtr)
				{
					char modelPath[MAX_QPATH];
					int	animRunLength;

					uiInfo.movesBaseAnim = datapadMoveTitleBaseAnims[uiInfo.movesTitleIndex];
					ItemParse_model_g2anim_go( item,  uiInfo.movesBaseAnim );

					Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
					ItemParse_asset_model_go( item, modelPath, &animRunLength );

					UI_UpdateCharacterSkin();

				}
			}
		}
	}
	else if (feederID == FEEDER_SIEGE_TEAM1)
	{
		if (!g_siegedFeederForcedSet)
		{
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(siegeTeam1->classes[index]);
			trap_Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass(siegeTeam1->classes[index]);
		}
		g_siegedFeederForcedSet = 0;
	}
	else if (feederID == FEEDER_SIEGE_TEAM2)
	{
		if (!g_siegedFeederForcedSet)
		{
			g_UIGloballySelectedSiegeClass = UI_SiegeClassNum(siegeTeam2->classes[index]);
			trap_Cvar_Set("ui_classDesc", g_UIClassDescriptions[g_UIGloballySelectedSiegeClass].desc);

			//g_siegedFeederForcedSet = 1;
			//Menu_SetFeederSelection(NULL, FEEDER_SIEGE_TEAM2, -1, NULL);

			UI_SiegeSetCvarsForClass(siegeTeam2->classes[index]);
		}
		g_siegedFeederForcedSet = 0;
	}
	else if (feederID == FEEDER_FORCECFG) 
	{
		int newindex = index;

		if (uiForceSide == FORCE_LIGHTSIDE)
		{
			newindex += uiInfo.forceConfigLightIndexBegin;
			if (newindex >= uiInfo.forceConfigCount)
			{
				return qfalse;
			}
		}
		else
		{ //else dark
			newindex += uiInfo.forceConfigDarkIndexBegin;
			if (newindex >= uiInfo.forceConfigCount || newindex > uiInfo.forceConfigLightIndexBegin)
			{ //dark gets read in before light
				return qfalse;
			}
		}

//		if (ClientManager::ActiveClientNum()==0)
//		{
			if (index >= 0 && index < uiInfo.forceConfigCount) 
			{
				//	UI_ForceConfigHandle(uiInfo.forceConfigSelected, index);
				//	uiInfo.forceConfigSelected = index;

				UI_ForceConfigHandle(ClientManager::ActiveClient().forceConfig, index);
				ClientManager::ActiveClient().forceConfig = index;	

			}
//		} 
//		else
//		{
//			if (index >= 0 && index < uiInfo.forceConfigCount) 
//			{
//					UI_ForceConfigHandle(uiInfo.forceConfigSelected2, index);
//					uiInfo.forceConfigSelected2 = index;
//			}

//		}
	}
	else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) 
	{
		int actual, map;
		const char *checkValid = NULL;

		if(index == 0)
		{
			trap_Cvar_SetValue("vote_nextmap", 1);
			return qtrue;
		}
		else
		{
			trap_Cvar_SetValue("vote_nextmap", 0);
			index--;
		}

		map = (feederID == FEEDER_ALLMAPS) ? ui_currentNetMap.integer : ui_currentMap.integer;
		checkValid = UI_SelectedMap(index, &actual);

		if (!checkValid || !checkValid[0])
		{ //this isn't a valid map to select, so reselect the current
			index = ui_mapIndex.integer;
			UI_SelectedMap(index, &actual);
		}

		trap_Cvar_Set("ui_mapIndex", va("%d", index));
		gUISelectedMap = index;
		ui_mapIndex.integer = index;

		if (feederID == FEEDER_MAPS) {
			ui_currentMap.integer = actual;
			trap_Cvar_Set("ui_currentMap", va("%d", actual));
		} else {
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set("ui_currentNetMap", va("%d", actual));
		}

	} else if (feederID == FEEDER_SERVERS) {
		Syslink_SetChosenServerIndex( index );
	} else if (feederID == FEEDER_PLAYER_LIST) {
		// If the server is dedicated, then we have a full list, and don't adjust the index:
		if (xbOnlineInfo.xbPlayerList[DEDICATED_SERVER_INDEX].isActive)
			uiInfo.playerIndex = index;
		else	// Otherwise, we skip the first client, which is actually the server:
			uiInfo.playerIndex = index + 1;
	} else if (feederID == FEEDER_TEAM_LIST) {
		uiInfo.teamIndex = index;
	}
	else if (feederID == FEEDER_COLORCHOICES) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorCount)
		{
			Item_RunScript(item, uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].ColorActionText[index]);
		}
	}
	else if (feederID == FEEDER_PLAYER_SKIN_HEAD) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadCount)
		{
			trap_Cvar_Set("ui_char_skin_head", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinHeadNames[index]);
		}
	} 
	else if (feederID == FEEDER_PLAYER_SKIN_TORSO) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoCount)
		{
			trap_Cvar_Set("ui_char_skin_torso", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinTorsoNames[index]);
		}
	} 
	else if (feederID == FEEDER_PLAYER_SKIN_LEGS) 
	{
		if (index >= 0 && index < uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegCount)
		{
			trap_Cvar_Set("ui_char_skin_legs", uiInfo.playerSpecies[uiInfo.playerSpeciesIndex].SkinLegNames[index]);
		}
	} 
	else if (feederID == FEEDER_PLAYER_SPECIES) 
	{
		uiInfo.playerSpeciesIndex = index;
	} 
	else if (feederID == FEEDER_LANGUAGES) 
	{
		uiInfo.languageCountIndex = index;
	} 
	else if (  feederID == FEEDER_SIEGE_BASE_CLASS )
	{
		int team,baseClass; 

		team = (int)trap_Cvar_VariableValue("ui_team");
		baseClass = (int)trap_Cvar_VariableValue("ui_siege_class");

		UI_UpdateCvarsForClass(team, baseClass, index);
	}
	else if (feederID == FEEDER_SIEGE_CLASS_WEAPONS) 
	{
//		trap_Cvar_VariableStringBuffer( va("ui_class_weapondesc%i", index), info, sizeof(info) );
//		trap_Cvar_Set( "ui_itemforceinvdesc", info );
	} 
	else if (feederID == FEEDER_SIEGE_CLASS_INVENTORY) 
	{
//		trap_Cvar_VariableStringBuffer( va("ui_class_itemdesc%i", index), info, sizeof(info) );
//		trap_Cvar_Set( "ui_itemforceinvdesc", info );
	} 
	else if (feederID == FEEDER_SIEGE_CLASS_FORCE) 
	{
		int i;
//		int validCnt = 0;

		trap_Cvar_VariableStringBuffer( va("ui_class_power%i", index), info, sizeof(info) );

		//count them up
		for (i=0;i< NUM_FORCE_POWERS;i++)
		{
			if (!strcmp(HolocronIcons[i],info))
			{
				trap_Cvar_Set( "ui_itemforceinvdesc", forcepowerDesc[i] );
			}
		}
	} 
#ifdef _XBOX
	else if (feederID == FEEDER_XBL_ACCOUNTS)
	{
		XBL_SetAccountIndex( index );
	}
	else if (feederID == FEEDER_XBL_PLAYERS)
	{
		XBL_PL_SetPlayerIndex( index );
	}
	else if (feederID == FEEDER_XBL_FRIENDS)
	{
		XBL_F_SetChosenFriendIndex( index );
	}
	else if (feederID == FEEDER_XBL_SERVERS)
	{
		XBL_MM_SetChosenServerIndex( index );
	}
	else if (feederID == FEEDER_PROFILES)
	{
		trap_Cvar_Set("ui_profileNameSelect",s_ProfileData[index].currentProfileName);
	}


#endif

	return qtrue;
}


static qboolean GameType_Parse(char **p, qboolean join) {
	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	if (join) {
		uiInfo.numJoinGameTypes = 0;
	} else {
		uiInfo.numGameTypes = 0;
	}

	while ( 1 ) {
		token = COM_ParseExt((const char **)p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// two tokens per line, character name and sex
			if (join) {
				if (!String_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gameType) || !Int_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gtEnum)) {
					return qfalse;
				}
			} else {
				if (!String_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gameType) || !Int_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gtEnum)) {
					return qfalse;
				}
			}
    
			if (join) {
				if (uiInfo.numJoinGameTypes < MAX_GAMETYPES) {
					uiInfo.numJoinGameTypes++;
				} else {
					Com_Printf("Too many net game types, last one replace!\n");
				}		
			} else {
				if (uiInfo.numGameTypes < MAX_GAMETYPES) {
					uiInfo.numGameTypes++;
				} else {
					Com_Printf("Too many game types, last one replace!\n");
				}		
			}
     
			token = COM_ParseExt((const char **)p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}
	return qfalse;
}

static qboolean MapList_Parse(char **p) {
	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	uiInfo.mapCount = 0;

	while ( 1 ) {
		token = COM_ParseExt((const char **)p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapName) || !String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapLoadName) 
				||!Int_Parse(p, &uiInfo.mapList[uiInfo.mapCount].teamMembers) ) {
				return qfalse;
			}

			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].opponentName)) {
				return qfalse;
			}

			uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

			while (1) {
				token = COM_ParseExt((const char **)p, qtrue);
				if (token[0] >= '0' && token[0] <= '9') {
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << (token[0] - 0x030));
					if (!Int_Parse(p, &uiInfo.mapList[uiInfo.mapCount].timeToBeat[token[0] - 0x30])) {
						return qfalse;
					}
				} else {
					break;
				} 
			}

  		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
			uiInfo.mapList[uiInfo.mapCount].levelShot = trap_R_RegisterShaderNoMip(va("levelshots/%s_small", uiInfo.mapList[uiInfo.mapCount].mapLoadName));

			if (uiInfo.mapCount < MAX_MAPS) {
				uiInfo.mapCount++;
			} else {
				Com_Printf("Too many maps, last one replaced!\n");
			}
		}
	}
	return qfalse;
}

static void UI_ParseGameInfo(const char *teamFile) {
	char	*token;
	char *p;
	char *buff = NULL;
	//int mode = 0; TTimo: unused

	buff = GetMenuBuffer(teamFile);
	if (!buff) {
		return;
	}

	p = buff;

	while ( 1 ) {
		token = COM_ParseExt( (const char **)(&p), qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "gametypes") == 0) {

			if (GameType_Parse(&p, qfalse)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "joingametypes") == 0) {

			if (GameType_Parse(&p, qtrue)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "maps") == 0) {
			// start a new menu
			MapList_Parse(&p);
		}

	}

	Z_Free( buff );
}

static void UI_Pause(qboolean b) {
	if (b) {
		// pause the game and set the ui keycatcher
	  trap_Cvar_Set( "cl_paused", "1" );
	  S_StopSounds();
		trap_Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}

/*
=================
UI_LoadForceConfig_List
=================
Looks in the directory for force config files (.fcf) and loads the name in
*/
void UI_LoadForceConfig_List( void )
{
	int			numfiles = 0;
	char		filelist[2048];
	char		configname[128];
	char		*fileptr = NULL;
	int			j = 0;
	int			filelen = 0;
	qboolean	lightSearch = qfalse;

	uiInfo.forceConfigCount = 0;
	Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), "Custom");
	uiInfo.forceConfigCount++;
	//Always reserve index 0 as the "custom" config

nextSearch:
	if (lightSearch)
	{ //search light side folder
		numfiles = trap_FS_GetFileList("forcecfg/light", "fcf", filelist, 2048 );
		uiInfo.forceConfigLightIndexBegin = uiInfo.forceConfigCount-1;
	}
	else
	{ //search dark side folder
		numfiles = trap_FS_GetFileList("forcecfg/dark", "fcf", filelist, 2048 );
		uiInfo.forceConfigDarkIndexBegin = uiInfo.forceConfigCount-1;
	}

	fileptr = filelist;

	for (j=0; j<numfiles && uiInfo.forceConfigCount < MAX_FORCE_CONFIGS;j++,fileptr+=filelen+1)
	{
		filelen = strlen(fileptr);
		COM_StripExtension(fileptr, configname);

		if (lightSearch)
		{
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qtrue; //light side config
		}
		else
		{
			uiInfo.forceConfigSide[uiInfo.forceConfigCount] = qfalse; //dark side config
		}

		Com_sprintf( uiInfo.forceConfigNames[uiInfo.forceConfigCount], sizeof(uiInfo.forceConfigNames[uiInfo.forceConfigCount]), configname);
		uiInfo.forceConfigCount++;
	}

	if (!lightSearch)
	{
		lightSearch = qtrue;
		goto nextSearch;
	}
}


/*
=================
bIsImageFile
builds path and scans for valid image extentions
=================
*/
static qboolean bIsImageFile(const char* dirptr, const char* skinname)
{
	char fpath[MAX_QPATH];
	int f;

#ifdef _XBOX
	Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.dds", dirptr, skinname);
#else
	Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.jpg", dirptr, skinname);
#endif
	trap_FS_FOpenFile(fpath, &f, FS_READ);
#if !defined(_XBOX) || defined(_DEBUG)
	if (!f)
	{ //not there, try png
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.png", dirptr, skinname);
		trap_FS_FOpenFile(fpath, &f, FS_READ);
	}
	if (!f)
	{ //not there, try tga
		Com_sprintf(fpath, MAX_QPATH, "models/players/%s/icon_%s.tga", dirptr, skinname);
		trap_FS_FOpenFile(fpath, &f, FS_READ);
	}
#endif
	if (f) 
	{
		trap_FS_FCloseFile(f);
		return qtrue;
	}

	return qfalse;
}


/*
=================
PlayerModel_BuildList
=================
*/
static bool _loadCachedQ3ModelList( void )
{
	// New, improved Xbox version - loads the cached version of this file!
	FILE *cacheFile = fopen( "d:\\ui_headcache", "rb" );
	if( !cacheFile )
		return false;

	// Read in the number of entries:
	if( !fread( &uiInfo.q3HeadCount, sizeof(uiInfo.q3HeadCount), 1, cacheFile ) )
	{
		fclose( cacheFile );
		return false;
	}

	// Read in the head names:
	if( !fread( &uiInfo.q3HeadNames[0][0], sizeof(uiInfo.q3HeadNames[0]) * uiInfo.q3HeadCount, 1, cacheFile ) )
	{
		fclose( cacheFile );
		return false;
	}

	memset( &uiInfo.q3HeadIcons[0], 0, sizeof(uiInfo.q3HeadIcons) );
	fclose( cacheFile );
	return true;
}

static void _saveCachedQ3ModelList( void )
{
	FILE *cacheFile = fopen( "d:\\ui_headcache", "wb" );
	if( !cacheFile )
		return;

	// Write out numer of entries:
	if( !fwrite( &uiInfo.q3HeadCount, sizeof(uiInfo.q3HeadCount), 1, cacheFile ) )
	{
		fclose( cacheFile );
		return;
	}

	// Write out the head names:
	if( !fwrite( &uiInfo.q3HeadNames[0][0], sizeof(uiInfo.q3HeadNames[0]) * uiInfo.q3HeadCount, 1, cacheFile ) )
	{
		fclose( cacheFile );
		return;
	}

	fclose( cacheFile );
	return;
}

static void UI_BuildQ3Model_List( void )
{
	// First, try to reuse a cache:
	if( _loadCachedQ3ModelList() )
		return;

	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[64];
	char*	dirptr;
	char*	fileptr;
	char*	check;
	int		i;
	int		j, k, p, s;
	int		dirlen;
	int		filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);
		
		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;
			
		if(strstr(dirptr, "rebel_pilot") > 0)
			continue;

		numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), "skin", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && uiInfo.q3HeadCount < MAX_Q3PLAYERMODELS;j++,fileptr+=filelen+1)
		{
			int skinLen = 0;

			filelen = strlen(fileptr);

			COM_StripExtension(fileptr,skinname);

			skinLen = strlen(skinname);
			k = 0;
			while (k < skinLen && skinname[k] && skinname[k] != '_')
			{
				k++;
			}
			if (skinname[k] == '_')
			{
				p = 0;

				while (skinname[k])
				{
					skinname[p] = skinname[k];
					k++;
					p++;
				}
				skinname[p] = '\0';
			}

			/*
#ifdef _XBOX
			Com_sprintf(fpath, 2048, "models/players/%s/icon%s.dds", dirptr, skinname);
#else
			Com_sprintf(fpath, 2048, "models/players/%s/icon%s.jpg", dirptr, skinname);
#endif

			trap_FS_FOpenFile(fpath, &f, FS_READ);

			if (f)
			*/
			check = &skinname[1];
			if (bIsImageFile(dirptr, check))
			{ //if it exists
				qboolean iconExists = qfalse;

				//trap_FS_FCloseFile(f);

				if (skinname[0] == '_')
				{ //change character to append properly
					skinname[0] = '/';
				}

				s = 0;

				while (s < uiInfo.q3HeadCount)
				{ //check for dupes
					if (!Q_stricmp(va("%s%s", dirptr, skinname), uiInfo.q3HeadNames[s]))
					{
						iconExists = qtrue;
						break;
					}
					s++;
				}

				if (iconExists)
				{
					continue;
				}

				Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), va("%s%s", dirptr, skinname));
				uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = 0;//trap_R_RegisterShaderNoMip(fpath);
				//rww - we are now registering them as they are drawn like the TA feeder, so as to decrease UI load time.
			}

			if (uiInfo.q3HeadCount >= MAX_Q3PLAYERMODELS)
			{
				return;
			}
		}
	}	

	// All right, we just did a lot of work, let's save it:
	_saveCachedQ3ModelList();
}

void UI_SiegeInit(void)
{
	//Load the player class types
	BG_SiegeLoadClasses(g_UIClassDescriptions);

	if (!bgNumSiegeClasses)
	{ //We didn't find any?!
		Com_Error(ERR_DROP, "Couldn't find any player classes for Siege");
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if (!bgNumSiegeTeams)
	{ //React same as with classes.
		Com_Error(ERR_DROP, "Couldn't find any player teams for Siege");
	}
}

/*
=================
UI_ParseColorData
=================
*/
//static qboolean UI_ParseColorData(char* buf, playerSpeciesInfo_t &species)
static qboolean UI_ParseColorData(char* buf, playerSpeciesInfo_t *species,char*	file)
{
	const char	*token;
	const char	*p;

	p = buf;
	COM_BeginParseSession(file);
	species->ColorCount = 0;

	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );	//looking for the shader
		if ( token[0] == 0 )
		{
			return species->ColorCount;
		}
		Q_strncpyz( species->ColorShader[species->ColorCount], token, sizeof(species->ColorShader[0]) );

		token = COM_ParseExt( &p, qtrue );	//looking for action block {
		if ( token[0] != '{' )
		{
			return qfalse;
		}

		assert(!species->ColorActionText[species->ColorCount][0]);
		token = COM_ParseExt( &p, qtrue );	//looking for action commands
		while (token[0] != '}')
		{
			if ( token[0] == 0)
			{	//EOF
				return qfalse;
			}
			assert(species->ColorCount < sizeof(species->ColorActionText)/sizeof(species->ColorActionText[0]) );
			Q_strcat(species->ColorActionText[species->ColorCount], sizeof(species->ColorActionText[0]), token);
			Q_strcat(species->ColorActionText[species->ColorCount], sizeof(species->ColorActionText[0]), " ");
			token = COM_ParseExt( &p, qtrue );	//looking for action commands or final }
		}
		species->ColorCount++;	//next color please
	}
	return qtrue;//never get here
}

/*
=================
UI_BuildPlayerModel_List
=================
*/
static void UI_BuildPlayerModel_List( qboolean inGameLoad )
{
	int		numdirs;
	char	dirlist[2048];
	char*	dirptr;
	int		dirlen;
	int		i;
	int		j;


	uiInfo.playerSpeciesCount = 0;
	uiInfo.playerSpeciesIndex = 0;
	memset(uiInfo.playerSpecies, 0, sizeof (uiInfo.playerSpecies) );

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs; i++,dirptr+=dirlen+1)
	{
		char	filelist[2048];
		char*	fileptr;
		int		filelen;
		int f = 0;
		char fpath[2048];

		dirlen = strlen(dirptr);
		
		if (dirlen)
		{
			if (dirptr[dirlen-1]=='/')
				dirptr[dirlen-1]='\0';
		}
		else
		{
			continue;
		}

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;
			
		Com_sprintf(fpath, 2048, "models/players/%s/PlayerChoice.txt", dirptr);
		filelen = trap_FS_FOpenFile(fpath, &f, FS_READ);

		if (f)
		{ 
			char buffer[2048];
			char	skinname[64];
			int		numfiles;
			int		iSkinParts=0;

			trap_FS_Read(&buffer, filelen, f);
			trap_FS_FCloseFile(f);
			buffer[filelen] = 0;	//ensure trailing NULL

			//record this species
			Q_strncpyz( uiInfo.playerSpecies[uiInfo.playerSpeciesCount].Name, dirptr, sizeof(uiInfo.playerSpecies[0].Name) );

			if (!UI_ParseColorData(buffer,&uiInfo.playerSpecies[uiInfo.playerSpeciesCount],fpath))
			{
				Com_Printf(S_COLOR_RED"UI_BuildPlayerModel_List: Errors parsing '%s'\n", fpath);
			}

			numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), ".skin", filelist, 2048 );
			fileptr  = filelist;
			for (j=0; j<numfiles; j++,fileptr+=filelen+1)
			{
				if (trap_Cvar_VariableValue("fs_copyfiles") > 0 )
				{
					trap_FS_FOpenFile(va("models/players/%s/%s",dirptr,fileptr), &f, FS_READ);
					if (f) trap_FS_FCloseFile(f);
				}

				filelen = strlen(fileptr);
				COM_StripExtension(fileptr,skinname);

				if (bIsImageFile(dirptr, skinname))
				{ //if it exists
					if (strnicmp(skinname,"head_",5) == 0)
					{
						if (uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinHeadCount < MAX_PLAYERMODELS) 
						{
							Q_strncpyz(
								uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinHeadNames[uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinHeadCount++], 
								skinname, 
								sizeof(uiInfo.playerSpecies[0].SkinHeadNames[0])
								);
							iSkinParts |= 1<<0;
						}
					} else
					if (strnicmp(skinname,"torso_",6) == 0)
					{
						if (uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinTorsoCount < MAX_PLAYERMODELS) 
						{
							Q_strncpyz(uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinTorsoNames[uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinTorsoCount++], 
								skinname, 
								sizeof(uiInfo.playerSpecies[0].SkinTorsoNames[0])
								);
							iSkinParts |= 1<<1;
						}
					} else
					if (strnicmp(skinname,"lower_",6) == 0)
					{
						if (uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinLegCount < MAX_PLAYERMODELS) 
						{
							Q_strncpyz(uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinLegNames[uiInfo.playerSpecies[uiInfo.playerSpeciesCount].SkinLegCount++], 
								skinname, 
								sizeof(uiInfo.playerSpecies[0].SkinLegNames[0]) 
								);
								iSkinParts |= 1<<2;
						}
					}
					
				}
			}
			if (iSkinParts != 7)
			{	//didn't get a skin for each, then skip this model.
				memset(&uiInfo.playerSpecies[uiInfo.playerSpeciesCount], 0, sizeof(uiInfo.playerSpecies[uiInfo.playerSpeciesCount]));//undo the colors
				continue;
			}
			uiInfo.playerSpeciesCount++;
			if (!inGameLoad && ui_PrecacheModels.integer)
			{
				int g2Model;
				void *ghoul2 = 0;
				Com_sprintf( fpath, sizeof( fpath ), "models/players/%s/model.glm", dirptr );
				g2Model = trap_G2API_InitGhoul2Model(&ghoul2, fpath, 0, 0, 0, 0, 0);
				if (g2Model >= 0)
				{
//					trap_G2API_RemoveGhoul2Model( &ghoul2, 0 );
					trap_G2API_CleanGhoul2Models (&ghoul2);
				}
			}
//			if (uiInfo.playerSpeciesCount >= MAX_PLAYERMODELS)
			if (uiInfo.playerSpeciesCount >= 8)
			{
				return;
			}
		}
	}	

}

/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad ) {
	const char *menuSet;
	int start;

	//register this freakin thing now
	vmCvar_t siegeTeamSwitch;
	trap_Cvar_Register(&siegeTeamSwitch, "g_siegeTeamSwitch", "1", CVAR_SERVERINFO|CVAR_ARCHIVE);

	// Get the list of possible languages
//	uiInfo.languageCount = trap_SP_GetNumLanguages();	// this does a dir scan, so use carefully

	uiInfo.inGameLoad = inGameLoad;

	//initialize all these cvars to "0"
	UI_SiegeSetCvarsForClass(NULL);

	UI_SiegeInit();

	UI_UpdateForcePowers();

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}


  //UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.RegisterFont = &trap_R_RegisterFont;
	uiInfo.uiDC.Font_StrLenPixels = trap_R_Font_StrLenPixels;
	uiInfo.uiDC.Font_StrLenChars = trap_R_Font_StrLenChars;
	uiInfo.uiDC.Font_HeightPixels = trap_R_Font_HeightPixels;
	uiInfo.uiDC.Font_DrawString = trap_R_Font_DrawString;
	uiInfo.uiDC.Language_IsAsian = trap_Language_IsAsian;
	uiInfo.uiDC.Language_UsesSpaces = trap_Language_UsesSpaces;
	uiInfo.uiDC.AnyLanguage_ReadCharFromString = trap_AnyLanguage_ReadCharFromString;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.deferScript = &UI_DeferMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error; 
	uiInfo.uiDC.Print = &Com_Printf; 
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;

	Init_Display(&uiInfo.uiDC);

	UI_BuildPlayerModel_List(inGameLoad);

	String_Init();
  
//	uiInfo.uiDC.cursor	= trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "white" );

	AssetCache();

	start = trap_Milliseconds();

  uiInfo.teamCount = 0;
  uiInfo.characterCount = 0;
  uiInfo.aliasCount = 0;

	UI_ParseGameInfo("ui/jk2mp/gameinfo.txt");

	menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jk2mpmenus.txt";
	}

#if 1
	if (inGameLoad)
	{
		UI_LoadMenus("ui/jk2mpingame.txt", qtrue);
	}
	else if (!ui_bypassMainMenuLoad.integer)
	{
		UI_LoadMenus(menuSet, qtrue);
	}
#else //this was adding quite a giant amount of time to the load time
	UI_LoadMenus(menuSet, qtrue);
	UI_LoadMenus("ui/jk2mpingame.txt", qtrue);
#endif
	
	trap_Cvar_Register(NULL, "ui_name", UI_Cvar_VariableString("name"), CVAR_INTERNAL );	//get this now, jic the menus change again trying to setName before getName

	Menus_CloseAll();

#ifndef _XBOX	// We no longer maintain ui_currentMap, this just causes problems
	UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
#endif

	UI_BuildQ3Model_List();
	UI_LoadBots();

	UI_LoadForceConfig_List();

	UI_InitForceShaders();

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = /*gamecodetoui[*/(int)trap_Cvar_VariableValue("color1");//-1];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");

	trap_Cvar_Set("ui_mousePitchVeh", (trap_Cvar_VariableValue("m_pitchVeh") >= 0) ? "0" : "1");

	trap_Cvar_Register(NULL, "debug_protocol", "", 0 );

	trap_Cvar_Set("ui_actualNetGameType", va("%d", ui_netGameType.integer));

#ifdef _XBOX
	trap_Cvar_Set("ui_siegeSelect","1");
#endif

}

//#ifdef _XBOX
//#include "../namespace_begin.h"
//extern void UpdateDemoTimer();
//#include "../namespace_end.h"
//#endif

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down ) {
	int storedclient;
	int menuActiveClient;
	int closedClients;

	// Hack: If we're a dedicated server, then we want the X button to be "hold-to-talk"
	if( com_dedicated->integer && key == A_DELETE )
	{
		g_Voice.SetChannel( down ? CHAN_ALT : CHAN_PRIMARY );
	}
	
  if (Menu_Count() > 0) {
    menuDef_t *menu = Menu_GetFocused();
		if (menu) {
//JLF
#ifdef _XBOX

//			UpdateDemoTimer();
			storedclient = ClientManager::ActiveClientNum();
		//	menuActiveClient =  Cvar_VariableIntegerValue("ui_menuClient");
			menuActiveClient =  uiClientNum;
			if ( storedclient != menuActiveClient)
				ClientManager::ActivateClient(menuActiveClient);
#endif
			//check to see if controller is blocked
			closedClients = uiclientInputClosed;//Cvar_VariableIntegerValue("clientInputClosed");
//JLF merciless hack
			if (strcmp("noController",menu->window.name)!=0)
			{
				uiControllerMenu = qfalse;
				if ( closedClients & 0x1 && uiClientNum == 0)//client 0 closed
					return;
				if ( closedClients& 0x2 && uiClientNum == 1)//client1 closed
					return;
			}
			else
			{
				uiControllerMenu = qtrue;
			}
			
		//	if (closedClients && uiClientNum)
		//		return;
		//	if ( ClientManager::ActiveController() & Cvar_VariableIntegerValue("clientInputClosed"))
		//		return;

		//	if (key == A_ESCAPE && down && !Menus_AnyFullScreenVisible()) {
		//		Menus_CloseAll();
		//	} else {
				Menu_HandleKey(menu, key, down );
		//	}
//JLF			
			if ( storedclient != menuActiveClient)
				ClientManager::ActivateClient( storedclient);

		} else {
			uiControllerMenu = qfalse;
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
		}

  }

  //if ((s > 0) && (s != menu_null_sound)) {
	//  trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
  //}
}


/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0)
		uiInfo.uiDC.cursorx = 0;
	else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH)
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0)
		uiInfo.uiDC.cursory = 0;
	else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT)
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;

	gScrollAccum += dy;
	gScrollDelta =0;

	if (gScrollAccum > 	TEXTSCROLLDESCRETESTEP)
	{
		gScrollDelta =1;
		gScrollAccum =0;
	}
	else if (gScrollAccum <0)
	{
		gScrollDelta = -1;
		gScrollAccum = TEXTSCROLLDESCRETESTEP;
	}
	


  if (Menu_Count() > 0) {
    //menuDef_t *menu = Menu_GetFocused();
    //Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
  }

}

void UI_LoadNonIngame() {
	const char *menuSet = UI_Cvar_VariableString("ui_menuFilesMP");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/jk2mpmenus.txt";
	}
	UI_LoadMenus(menuSet, qfalse);
	uiInfo.inGameLoad = qfalse;
}

extern void S_StopSounds( void );

void _UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];
//JLF
#ifdef _XBOX
	static qboolean firstmenu = qtrue;
#endif

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
  if (Menu_Count() > 0) {
		vec3_t v;
		v[0] = v[1] = v[2] = 0;

		// XBOX - Remember which client started the UI!

		//ignore player2 join message
		if (UIMENU_PLAYERCONFIG == menu)
		{
			if (ClientManager::ActiveClientNum())
				return;
		}

		if (menu != UIMENU_NOCONTROLLERINGAME && menu != UIMENU_NOCONTROLLER)
			uiClientNum = ClientManager::ActiveClientNum();

	  switch ( menu ) {
	  case UIMENU_NONE:
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

		  return;
	  case UIMENU_MAIN:
		{
			trap_Key_SetCatcher( KEYCATCH_UI );
			
			Menus_CloseAll();

			if (logged_on)
				Menus_ActivateByName("xbl_lobbymenu");
			else
				Menus_ActivateByName("main");

			trap_Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));
			
			if (buf[0]) 
			{
				// Display Xbox popups after an ERR_DROP. But there's a special case
				// if the Com_Error was for getting disconnected from Live, allowing
				// the user to go to the dashboard/troubleshooter:
				extern bool bComErrorLostConnection;
				if( bComErrorLostConnection )
				{
					UI_xboxErrorPopup( XB_POPUP_CANNOT_CONNECT );
					bComErrorLostConnection = false;
				}
				else
					UI_xboxErrorPopup( XB_POPUP_COM_ERROR );
			}

			if( ControllerOutNum.integer >= 0 )
			{
				IN_DisplayControllerUnplugged( ControllerOutNum.integer );
			}

			return;
		}

	  case UIMENU_TEAM:
			trap_Key_SetCatcher( KEYCATCH_UI );
      Menus_ActivateByName("team");
		  return;
	  case UIMENU_POSTGAME:
			//trap_Cvar_Set( "sv_killserver", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			if (uiInfo.inGameLoad) {
//				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName("endofgame");
		  //UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
		  return;
	  case UIMENU_INGAME:
			S_StopSounds();
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();

//#ifdef _XBOX
			// Set the UImodel Cvar to be the current player's model Cvar
//			trap_Cvar_Set("UImodel", Cvar_VariableString("model"));
//#endif

			trap_Cvar_Set("ui_menuProgression","ingamemenu");

		//	Cvar_SetValue("ui_menuClient",ClientManager::ActiveClientNum());

			Menus_CloseAll();
			Menus_ActivateByName("ingame");
		  return;
	  case UIMENU_PLAYERCONFIG:
			{
				menuDef_t * thismenu;
				thismenu =Menus_FindByName("ingame_player");
				if (!( thismenu->window.flags & WINDOW_VISIBLE) )
				{
						 
					gDelayedPause = PAUSE_DELAY;

						
//#ifdef _XBOX
					// Set the UImodel Cvar to be the current player's model Cvar
//					trap_Cvar_Set("UImodel", Cvar_VariableString("model"));
//#endif
					trap_Key_SetCatcher( KEYCATCH_UI );
					UI_BuildPlayerList();
					Menus_CloseAll();
					Menus_ActivateByName("ingame_player");
					UpdateForceUsed();
				}
			}
		  return;
	  case UIMENU_PLAYERFORCE:
		 // trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame_playerforce");
			UpdateForceUsed();
		  return;
	  case UIMENU_SIEGEMESSAGE:
		 // trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("siege_popmenu");
		  return;
	  case UIMENU_SIEGEOBJECTIVES:
		 // trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_siegeobjectives");
			return;
	  case UIMENU_VOICECHAT:
		 // trap_Cvar_Set( "cl_paused", "1" );
			// No chatin non-siege games.
		 
		  	if (trap_Cvar_VariableValue( "g_gametype" ) < GT_TEAM)
			{
				return;
			}
				 
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_voicechat");
			return;
	  case UIMENU_CLOSEALL:
			Menus_CloseAll();
			return;
	  case UIMENU_CLASSSEL:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("ingame_siegeclass");
		  return;
	  case UIMENU_DEDICATED:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("dedicated");
		  return;

	  case UIMENU_NOCONTROLLERINGAME:
		//	trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
		//	trap_Cvar_Set("ui_menuProgression","ingamemenu");
		//	Menus_CloseAll();
		//	Menus_ActivateByName("ingame");
			Menus_ActivateByName("noController");
			uiControllerMenu = qtrue;
		  return;

	  case UIMENU_NOCONTROLLER:
			Menus_ActivateByName("noController");
			uiControllerMenu = qtrue;
		  return;
	  }
  }
}

qboolean _UI_IsFullscreen( void ) {
	return Menus_AnyFullScreenVisible();
}



static connstate_t	lastConnState;
static char			lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB", 
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB", 
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %2d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%2d min %2d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%2d sec", time );
	}
}

void Text_PaintCenter(float x, float y, float scale, vec4_t color, const char *text, float adjust, int iMenuFont) {
	int len = Text_Width(text, scale, iMenuFont);
	Text_Paint(x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, iMenuFont);
}


static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale, int iMenuFont) {
	char sDownLoading[256];
	char sEstimatedTimeLeft[256];
	char sTransferRate[256];
	char sOf[20];
	char sCopied[256];
	char sSec[20];
	//
	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int leftWidth;
	const char *s;

	vec4_t colorLtGreyAlpha = {0, 0, 0, .5};

	UI_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, colorLtGreyAlpha );

	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 0);	// "Downloading:"
	strcpy(sDownLoading,s?s:"");
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 1);	// "Estimated time left:"
	strcpy(sEstimatedTimeLeft,s?s:"");
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 2);	// "Transfer rate:"
	strcpy(sTransferRate,s?s:"");
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 3);	// "of"
	strcpy(sOf,s?s:"");
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 4);	// "copied"
	strcpy(sCopied,s?s:"");
	s = GetCRDelineatedString("MENUS","DOWNLOAD_STUFF", 5);	// "sec."
	strcpy(sSec,s?s:"");

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor(colorWhite);

	Text_PaintCenter(centerPoint, yStart + 112, scale, colorWhite, sDownLoading, 0, iMenuFont);
	Text_PaintCenter(centerPoint, yStart + 192, scale, colorWhite, sEstimatedTimeLeft, 0, iMenuFont);
	Text_PaintCenter(centerPoint, yStart + 248, scale, colorWhite, sTransferRate, 0, iMenuFont);

	if (downloadSize > 0) {
		s = va( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	} else {
		s = downloadName;
	}

	Text_PaintCenter(centerPoint, yStart+136, scale, colorWhite, s, 0, iMenuFont);

	UI_ReadableSize( dlSizeBuf,		sizeof dlSizeBuf,		downloadCount );
	UI_ReadableSize( totalSizeBuf,	sizeof totalSizeBuf,	downloadSize );

	if (downloadCount < 4096 || !downloadTime) {
		Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0, iMenuFont);
		Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
	} else {
		if ((uiInfo.uiDC.realTime - downloadTime) / 1000) {
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		} else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if (downloadSize && xferRate) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf, 
				(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, dlTimeBuf, 0, iMenuFont);
			Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
		} else {
			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0, iMenuFont);
			if (downloadSize) {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s %s %s)", dlSizeBuf, sOf, totalSizeBuf, sCopied), 0, iMenuFont);
			} else {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s %s)", dlSizeBuf, sCopied), 0, iMenuFont);
			}
		}

		if (xferRate) {
			Text_PaintCenter(leftWidth, yStart+272, scale, colorWhite, va("%s/%s", xferRateBuf,sSec), 0, iMenuFont);
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay )
{
	// This function no longer does anything during overlay mode - CG_DrawInformation
	// does everything in that situation. We could stop calling this, but whatever...
//	if( overlay )
//		return;

	const char *s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];
	float yStart = 130;

	char sStringEdTemp[256];

	menuDef_t *menu = Menus_FindByName("Connect");

	if ( menu )
	{
		// Controls drawing of the map pic frames
		Cvar_SetValue( "cx_overlay", overlay );
		Menu_Paint(menu, qtrue);
	}

	// New: during overlay mode, we draw first, just to lay down the background,
	// then cgame takes over drawing the mappics and such
	if ( overlay )
		return;

	// see what information we should display
	trap_GetClientState( &cstate );

	info[0] = '\0';
	if( trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) ) )
	{
		// Loading... <Map Name>
		int mapIndex = mapNameToIndex( Info_ValueForKey( info, "mapname" ) );
		if( mapIndex == MAP_ARRAY_SIZE )
			s = va( SE_GetString( "MENUS_LOADING_MAPNAME" ), "" );
		else
			s = va( SE_GetString( "MENUS_LOADING_MAPNAME" ), mapIndexToLongName( mapIndex ) );
		Text_PaintCenter(320, yStart, 1.0, colorWhite, s, 0, FONT_MEDIUM);
	}

	// Either "Starting Up" or "Connecting to Host"
	if (!Q_stricmp(cstate.servername,"localhost"))
		s = SE_GetString( "MENUS_STARTING_UP" );
	else
		s = SE_GetString( "MENUS_CONNECTING_TO" );
	Text_PaintCenter(320, yStart + 48, 1.0, colorWhite, s, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);

#ifndef _XBOX
	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		{
			trap_SP_GetStringTextString("MENUS_AWAITING_CONNECTION", sStringEdTemp, sizeof(sStringEdTemp));
			s = va(/*"Awaiting connection...%i"*/sStringEdTemp, cstate.connectPacketCount);
		}
		break;
	case CA_CHALLENGING:
		{
			trap_SP_GetStringTextString("MENUS_AWAITING_CHALLENGE", sStringEdTemp, sizeof(sStringEdTemp));
			s = va(/*"Awaiting challenge...%i"*/sStringEdTemp, cstate.connectPacketCount);
		}
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

			trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
			if (*downloadName) {
				UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale, FONT_MEDIUM );
				return;
			}
		}
		trap_SP_GetStringTextString("MENUS_AWAITING_GAMESTATE", sStringEdTemp, sizeof(sStringEdTemp));
		s = /*"Awaiting gamestate..."*/sStringEdTemp;
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	if (Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(320, yStart + 80, 1.0, colorWhite, s, 0, FONT_MEDIUM);
	}
	// password required / connection rejected information goes here
#endif
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

vmCvar_t	ui_ffa_fraglimit;
vmCvar_t	ui_ffa_timelimit;

vmCvar_t	ui_tourney_fraglimit;
vmCvar_t	ui_tourney_timelimit;

vmCvar_t	ui_selectedModelIndex;
vmCvar_t	ui_char_model;
vmCvar_t	ui_char_skin_head;
vmCvar_t	ui_char_skin_torso;
vmCvar_t	ui_char_skin_legs;

vmCvar_t	ui_saber_type;
vmCvar_t	ui_saber;
vmCvar_t	ui_saber2;
vmCvar_t	ui_saber_color;
vmCvar_t	ui_saber2_color;

//JLF Force
vmCvar_t	ui_forceSideCvar;
//vmCvar_t	forceSideCvar1;
//vmCvar_t	forceSideCvar2;

vmCvar_t	ui_forceConfigCvar;
//vmCvar_t	forceConfigCvar1;
//vmCvar_t	forceConfigCvar2;


vmCvar_t	ui_team_fraglimit;
vmCvar_t	ui_team_timelimit;
vmCvar_t	ui_team_friendly;

vmCvar_t	ui_ctf_capturelimit;
vmCvar_t	ui_ctf_timelimit;
vmCvar_t	ui_ctf_friendly;

vmCvar_t	ui_arenasFile;
vmCvar_t	ui_botsFile;
vmCvar_t	ui_spSkill;

vmCvar_t	ui_browserMaster;
vmCvar_t	ui_browserGameType;
vmCvar_t	ui_browserSortKey;
vmCvar_t	ui_browserShowFull;
vmCvar_t	ui_browserShowEmpty;

vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;

vmCvar_t	ui_server1;
vmCvar_t	ui_server2;
vmCvar_t	ui_server3;
vmCvar_t	ui_server4;
vmCvar_t	ui_server5;
vmCvar_t	ui_server6;
vmCvar_t	ui_server7;
vmCvar_t	ui_server8;
vmCvar_t	ui_server9;
vmCvar_t	ui_server10;
vmCvar_t	ui_server11;
vmCvar_t	ui_server12;
vmCvar_t	ui_server13;
vmCvar_t	ui_server14;
vmCvar_t	ui_server15;
vmCvar_t	ui_server16;

vmCvar_t	ui_redteam;
vmCvar_t	ui_redteam1;
vmCvar_t	ui_redteam2;
vmCvar_t	ui_redteam3;
vmCvar_t	ui_redteam4;
vmCvar_t	ui_redteam5;
vmCvar_t	ui_redteam6;
vmCvar_t	ui_redteam7;
vmCvar_t	ui_redteam8;
vmCvar_t	ui_blueteam;
vmCvar_t	ui_blueteam1;
vmCvar_t	ui_blueteam2;
vmCvar_t	ui_blueteam3;
vmCvar_t	ui_blueteam4;
vmCvar_t	ui_blueteam5;
vmCvar_t	ui_blueteam6;
vmCvar_t	ui_blueteam7;
vmCvar_t	ui_blueteam8;
vmCvar_t	ui_teamName;
vmCvar_t	ui_dedicated;
vmCvar_t	ui_gameType;
vmCvar_t	ui_netGameType;
vmCvar_t	ui_actualNetGameType;
vmCvar_t	ui_joinGameType;
#ifdef _XBOX
vmCvar_t	ui_optiGameType;
vmCvar_t	ui_optiCurrentMap;
vmCvar_t	ui_optiMinPlayers;
vmCvar_t	ui_optiMaxPlayers;
vmCvar_t	ui_optiFriendlyFire;
vmCvar_t	ui_optiJediMastery;
vmCvar_t	ui_optiSaberOnly;
vmCvar_t	ui_optiDedicated;
#endif
vmCvar_t	ui_netSource;
vmCvar_t	ui_serverFilterType;
vmCvar_t	ui_opponentName;
vmCvar_t	ui_menuFiles;
vmCvar_t	ui_currentMap;
vmCvar_t	ui_currentNetMap;
vmCvar_t	ui_mapIndex;
vmCvar_t	ui_currentOpponent;
vmCvar_t	ui_selectedPlayer;
vmCvar_t	ui_selectedPlayerName;
vmCvar_t	ui_lastServerRefresh_0;
vmCvar_t	ui_lastServerRefresh_1;
vmCvar_t	ui_lastServerRefresh_2;
vmCvar_t	ui_lastServerRefresh_3;
vmCvar_t	ui_singlePlayerActive;
vmCvar_t	ui_scoreAccuracy;
vmCvar_t	ui_scoreImpressives;
vmCvar_t	ui_scoreExcellents;
vmCvar_t	ui_scoreCaptures;
vmCvar_t	ui_scoreDefends;
vmCvar_t	ui_scoreAssists;
vmCvar_t	ui_scoreGauntlets;
vmCvar_t	ui_scoreScore;
vmCvar_t	ui_scorePerfect;
vmCvar_t	ui_scoreTeam;
vmCvar_t	ui_scoreBase;
vmCvar_t	ui_scoreTimeBonus;
vmCvar_t	ui_scoreSkillBonus;
vmCvar_t	ui_scoreShutoutBonus;
vmCvar_t	ui_scoreTime;
vmCvar_t	ui_captureLimit;
vmCvar_t	ui_fragLimit;
vmCvar_t	ui_findPlayer;
vmCvar_t	ui_hudFiles;
vmCvar_t	ui_recordSPDemo;
vmCvar_t	ui_realCaptureLimit;
vmCvar_t	ui_realWarmUp;
vmCvar_t	ui_serverStatusTimeOut;
vmCvar_t	se_language;
vmCvar_t	ui_bypassMainMenuLoad;

#ifdef _XBOX
vmCvar_t	ui_profileNameSelect;
#endif //_XBOX

// bk001129 - made static to avoid aliasing
static cvarTable_t		cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_selectedModelIndex, "ui_selectedModelIndex", "16", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_char_model,			"ui_char_model",		"jedi_tf",CVAR_ROM|CVAR_INTERNAL},
	{ &ui_char_skin_head,		"ui_char_skin_head",	"head_a1",CVAR_ROM|CVAR_INTERNAL},
	{ &ui_char_skin_torso,		"ui_char_skin_torso",	"torso_a1",CVAR_ROM|CVAR_INTERNAL}, 
	{ &ui_char_skin_legs,		"ui_char_skin_legs",	"lower_a1",CVAR_ROM|CVAR_INTERNAL}, 

	{ &ui_char_anim,			"ui_char_anim",			"BOTH_WALK1",CVAR_ROM|CVAR_INTERNAL}, 

	{ &ui_saber_type,			"ui_saber_type",		"single",CVAR_ROM|CVAR_INTERNAL},
	{ &ui_saber,				"ui_saber",				"single_1",CVAR_ROM|CVAR_INTERNAL}, 
	{ &ui_saber2,				"ui_saber2",			"none",CVAR_ROM|CVAR_INTERNAL}, 
	{ &ui_saber_color,			"ui_saber_color",		"yellow",CVAR_ROM|CVAR_INTERNAL},
	{ &ui_saber2_color,			"ui_saber2_color",		"yellow",CVAR_ROM|CVAR_INTERNAL},

	{ &ui_char_color_red,		"ui_char_color_red",	"255", CVAR_ROM|CVAR_INTERNAL}, 
	{ &ui_char_color_green,		"ui_char_color_green",	"255", CVAR_ROM|CVAR_INTERNAL}, 
	{ &ui_char_color_blue,		"ui_char_color_blue",	"255", CVAR_ROM|CVAR_INTERNAL}, 

	{ &ui_PrecacheModels,		"ui_PrecacheModels",	"0", CVAR_ARCHIVE}, 

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE|CVAR_INTERNAL },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_debug, "ui_debug", "0", CVAR_TEMP|CVAR_INTERNAL },
	{ &ui_initialized, "ui_initialized", "0", CVAR_TEMP|CVAR_INTERNAL },
	{ &ui_teamName, "ui_teamName", "Empire", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_opponentName, "ui_opponentName", "Rebellion", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_rankChange, "ui_rankChange", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_freeSaber, "ui_freeSaber", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_forcePowerDisable, "ui_forcePowerDisable", "0", CVAR_ARCHIVE|CVAR_INTERNAL },

//JLF
	{ &ui_forceSideCvar, "ui_forceSideCvar", "1", CVAR_ARCHIVE },
//	{ &forceSideCvar1, "forceSideCvar1", "1", 0 },
//	{ &forceSideCvar2, "forceSideCvar2", "1", 0 },

	{ &ui_forceConfigCvar, "ui_forceConfigCvar", "0", CVAR_ARCHIVE },
//	{ &forceConfigCvar1, "forceConfigCvar1", "0", CVAR_ARCHIVE },
//	{ &forceConfigCvar2, "forceConfigCvar2", "0", CVAR_ARCHIVE },



	{ &ui_redteam, "ui_redteam", "Empire", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam, "ui_blueteam", "Rebellion", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_dedicated, "ui_dedicated", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_gameType, "ui_gametype", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_joinGameType, "ui_joinGametype", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_netGameType, "ui_netGametype", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_actualNetGameType, "ui_actualNetGametype", "3", CVAR_ARCHIVE|CVAR_INTERNAL },
#ifdef _XBOX
	{ &ui_optiGameType, "ui_optiGameType", "0", CVAR_ARCHIVE },
	{ &ui_optiCurrentMap, "ui_optiCurrentMap", "0", CVAR_ARCHIVE },
	{ &ui_optiMinPlayers, "ui_optiMinPlayers", "0", CVAR_ARCHIVE },
	{ &ui_optiMaxPlayers, "ui_optiMaxPlayers", "0", CVAR_ARCHIVE },
	{ &ui_optiFriendlyFire, "ui_optiFriendlyFire", "0", CVAR_ARCHIVE },
	{ &ui_optiJediMastery, "ui_optiJediMastery", "0", CVAR_ARCHIVE },
	{ &ui_optiSaberOnly, "ui_optiSaberOnly", "0", CVAR_ARCHIVE },
	{ &ui_optiDedicated, "ui_optiDedicated", "0", CVAR_ARCHIVE },
	{ &ui_profileNameSelect,"ui_profileNameSelect","darkwing",CVAR_ARCHIVE },
#endif
	{ &ui_redteam1, "ui_redteam1", "1", CVAR_ARCHIVE|CVAR_INTERNAL }, //rww - these used to all default to 0 (closed).. I changed them to 1 (human)
	{ &ui_redteam2, "ui_redteam2", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam3, "ui_redteam3", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam4, "ui_redteam4", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam5, "ui_redteam5", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam6, "ui_redteam6", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam7, "ui_redteam7", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_redteam8, "ui_redteam8", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam1, "ui_blueteam1", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam2, "ui_blueteam2", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam3, "ui_blueteam3", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam4, "ui_blueteam4", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam5, "ui_blueteam5", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam6, "ui_blueteam6", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam7, "ui_blueteam7", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_blueteam8, "ui_blueteam8", "1", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_netSource, "ui_netSource", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_menuFiles, "ui_menuFilesMP", "ui/jk2mpmenus.txt", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_currentMap, "ui_currentMap", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_currentNetMap, "ui_currentNetMap", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_mapIndex, "ui_mapIndex", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_currentOpponent, "ui_currentOpponent", "0", CVAR_ARCHIVE|CVAR_INTERNAL },
	{ &ui_selectedPlayer, "cg_selectedPlayer", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_selectedPlayerName, "cg_selectedPlayerName", "", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_lastServerRefresh_0, "ui_lastServerRefresh_0", "", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_lastServerRefresh_1, "ui_lastServerRefresh_1", "", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_lastServerRefresh_2, "ui_lastServerRefresh_2", "", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_lastServerRefresh_3, "ui_lastServerRefresh_3", "", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_INTERNAL},
	{ &ui_scoreAccuracy, "ui_scoreAccuracy", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreImpressives, "ui_scoreImpressives", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreExcellents, "ui_scoreExcellents", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreCaptures, "ui_scoreCaptures", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreDefends, "ui_scoreDefends", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreAssists, "ui_scoreAssists", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreGauntlets, "ui_scoreGauntlets", "0",CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreScore, "ui_scoreScore", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scorePerfect, "ui_scorePerfect", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreTeam, "ui_scoreTeam", "0 to 0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreBase, "ui_scoreBase", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreTime, "ui_scoreTime", "00:00", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreTimeBonus, "ui_scoreTimeBonus", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreSkillBonus, "ui_scoreSkillBonus", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_scoreShutoutBonus, "ui_scoreShutoutBonus", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_fragLimit, "ui_fragLimit", "10", CVAR_INTERNAL},
	{ &ui_captureLimit, "ui_captureLimit", "5", CVAR_INTERNAL},
	{ &ui_findPlayer, "ui_findPlayer", "Kyle", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_realWarmUp, "g_warmup", "20", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &ui_realCaptureLimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE| CVAR_INTERNAL | CVAR_NORESTART},
	{ &ui_serverStatusTimeOut, "ui_serverStatusTimeOut", "7000", CVAR_ARCHIVE|CVAR_INTERNAL},
	{ &se_language, "se_language","english", CVAR_ARCHIVE | CVAR_NORESTART},	//text (string ed)

	{ &ui_bypassMainMenuLoad, "ui_bypassMainMenuLoad", "0", CVAR_INTERNAL },
//JLF menu progression
	{&ui_menuProgression, "ui_menuProgression", "",0},
//	{&ui_menuClient, "ui_menuClient", "0",0},
	{ &ControllerOutNum,	"ControllerOutNum", "-1", 0}, 
	{ &ui_respawnneeded,	"ui_respawnneeded", "0", 0}, 



};

// bk001129 - made static to avoid aliasing
static int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}

//JLF
#ifdef _XBOX //xbox version
//for the xbox reading the save directory will consist of 
//iterating through the save game folders

void ReadSaveDirectoryProfiles (void)
{
	char	*holdChar;

	// Clear out save data
	memset(s_ProfileData,0,sizeof(s_ProfileData));
	s_playerProfile.fileCnt = 0;
//	Cvar_Set("ui_profileDesc", "" );	// Blank out comment 
//	Cvar_Set("ui_SelectionOK", "0" );

//	Cvar_Set("ui_ResumeOK", "0" );
	holdChar = s_playerProfile.listBuf;
	XGAME_FIND_DATA SaveGameData;
	HANDLE searchhandle;
	HANDLE profileHandle;
	BOOL retval;
	char psLocalFilename[filepathlength];

 // At least one; count up the rest
	DWORD dwCount = 1;

	char saveGameName[filepathlength];
		
	

    // Any saves?
	searchhandle = XFindFirstSaveGame( "U:\\", &SaveGameData );
	if ( searchhandle != INVALID_HANDLE_VALUE )
    do
	{
		//get the name of the file		
		wcstombs(saveGameName, SaveGameData.szSaveGameName, filepathlength);
		strcpy( holdChar, saveGameName);
		

		if	( Q_stricmp("current",saveGameName)!=0 )
		{
			time_t result;
			if (Q_stricmp("auto",saveGameName)==0)
			{
//				Cvar_Set("ui_ResumeOK", "1" );
			}
			else
			{	

				// Is this a valid file??? & Get comment of file
				//create full path name
				// create the path for the screenshot file
				strcpy (psLocalFilename , SaveGameData.szSaveGameDirectory);
				strcat (psLocalFilename , "JK3PF.xsv");


				//find out if the file is there

				profileHandle = NULL;
				profileHandle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
					OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);
				if ( profileHandle!= INVALID_HANDLE_VALUE)
				{

//				result = ui.SG_GetSaveGameComment(saveGameName, s_savedata[s_savegame.saveFileCnt].currentSaveFileComments, s_savedata[s_savegame.saveFileCnt].currentSaveFileMap);
//				if (result != 0) // ignore Bad save game 
				
//					strcpy(s_ProfileData[s_playerProfile.fileCnt].currentSaveFileComments,s_ProfileData[s_playerProfile.fileCnt].currentSaveFileMap);
					strcpy(s_ProfileData[s_playerProfile.fileCnt].currentProfileName,saveGameName);
//					s_ProfileData[s_playerProfile.fileCnt].currentSaveFileDateTime = result;
//					holdChar += strlen(holdChar)+1;
					
//					struct tm *localTime;
//					localTime = localtime( &result );
///					strcpy(s_ProfileData[s_playerProfile.fileCnt].currentSaveFileDateTimeString,asctime( localTime ) );
					s_playerProfile.fileCnt++;
					CloseHandle(profileHandle);
					if (s_playerProfile.fileCnt == MAX_PROFILEFILES)
					{
						break;
					}
				}
			}
		}
		
		retval =XFindNextSaveGame( searchhandle, &SaveGameData );
	}while(retval);
    
}


/*
=================
UI_SoftKeyboard
=================
*/
static void UI_SoftKeyboardInit()
{
	char strtmp[] = "";

	trap_Cvar_Set("ui_profileNameSelect", strtmp);

	skb.activeKey=0;
	skb.curCol=0;
	skb.curRow=0;
	skb.curStringPos=0;
	skb.pulse_size = SKB_PULSE_SMALL;
	skb.pulse_up = true;
}

static void UI_SoftKeyboardDelete()
{
	char strtmp[SKB_STRING_LENGTH+1];
	trap_Cvar_VariableStringBuffer("ui_profileNameSelect", strtmp, SKB_STRING_LENGTH+1);
	if(skb.curStringPos > 0)
	{
		strtmp[skb.curStringPos] = 0;	// should already be 0, but let's be safe
		skb.curStringPos--;
		strtmp[skb.curStringPos] = 0;
	}
	trap_Cvar_Set("ui_profileNameSelect", strtmp);
}

static void UI_SoftKeyboardAccept()
{
	char strtmp[SKB_STRING_LENGTH+1];
	trap_Cvar_VariableStringBuffer("ui_profileNameSelect", strtmp, SKB_STRING_LENGTH+1);

	if(Q_stricmp(strtmp,"") )
	{

		menuDef_t *menu = Menu_GetFocused();
		itemDef_t *item	= Menu_FindItemByName(menu, SKB_ACCEPT_NAME);
		if (menu->onAccept) 
		{
			Item_RunScript(item, menu->onAccept);
		}
	}
}

static qboolean UI_SoftKeyboardDelete_HandleKey(int flags, float *special, int key) 
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item;
	switch(key)
	{
	case A_CURSOR_UP:
		skb.curRow = SKB_NUM_ROWS - 1;
		if((skb.curRow * SKB_NUM_COLS + skb.curCol) >= SKB_NUM_LETTERS)
			skb.curRow--;
		item = Menu_FindItemByName(menu, SKB_KEYBOARD_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_CURSOR_DOWN:
		skb.curRow = 0;
		item = Menu_FindItemByName(menu, SKB_KEYBOARD_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_CURSOR_LEFT:
	case A_CURSOR_RIGHT:
		skb.curCol = SKB_NUM_COLS/2;
		item = Menu_FindItemByName(menu, SKB_ACCEPT_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_MOUSE1:
		UI_SoftKeyboardDelete();
		break;
	default:
		// We didn't handle this keypress.
		return qfalse;
		break;
	}
	skb.activeKey = skb.curRow * SKB_NUM_COLS + skb.curCol;
	return qtrue;
}

static qboolean UI_SoftKeyboardAccept_HandleKey(int flags, float *special, int key) 
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item;
	switch(key)
	{
	case A_CURSOR_UP:
		skb.curRow = SKB_NUM_ROWS - 1;
		if((skb.curRow * SKB_NUM_COLS + skb.curCol) >= SKB_NUM_LETTERS)
			skb.curRow--;
		item = Menu_FindItemByName(menu, SKB_KEYBOARD_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_CURSOR_DOWN:
		skb.curRow = 0;
		item = Menu_FindItemByName(menu, SKB_KEYBOARD_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_CURSOR_LEFT:
	case A_CURSOR_RIGHT:
		skb.curCol = SKB_NUM_COLS/2-1;
		item = Menu_FindItemByName(menu, SKB_DELETE_NAME);
		Item_SetFocus(item, 0, 0);
		break;
	case A_MOUSE1:
		UI_SoftKeyboardAccept();
		break;
	default:
		// We didn't handle this keypress.
		return qfalse;
		break;
	}
	skb.activeKey = skb.curRow * SKB_NUM_COLS + skb.curCol;
	return qtrue;
}


static qboolean UI_SoftKeyboard_HandleKey(int flags, float *special, int key) 
{
	char strtmp[SKB_STRING_LENGTH+1];
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item;

// If the user pressed A (mouse 1), just add a letter to our string and return
	if(key == A_MOUSE1)
	{
		trap_Cvar_VariableStringBuffer("ui_profileNameSelect", strtmp, SKB_STRING_LENGTH+1);
		if(skb.curStringPos < SKB_STRING_LENGTH)
		{
			strtmp[skb.curStringPos] = letters[skb.activeKey][0];
			skb.curStringPos++;
			strtmp[skb.curStringPos] = 0;
		}
		trap_Cvar_Set("ui_profileNameSelect", strtmp);
		return qtrue;
	}

// Assuming the user pressed the D-pad, adjust the current row and column,
// and the associated active key position.
	switch(key)
	{
	case A_CURSOR_UP:
		skb.curRow-=1;
		break;
	case A_CURSOR_DOWN:
		skb.curRow+=1;
		break;
	case A_CURSOR_LEFT:
		skb.curCol-=1;
		break;
	case A_CURSOR_RIGHT:
		skb.curCol+=1;
		break;
	default:
		// We didn't handle this keypress.
		return qfalse;
		break;
	}
	skb.activeKey = skb.curRow * SKB_NUM_COLS + skb.curCol;

// Now make sure that the new active key is actually on the keyboard
// This means that the row and columns must be within bounds, and we
// must be on a letter (not on an empty space)
	if(skb.activeKey < 0 || skb.activeKey >=SKB_NUM_LETTERS || skb.curCol >= SKB_NUM_COLS || skb.curCol < 0)
	{
		switch(key)
		{
		case A_CURSOR_UP:
			// Wrap to the Accept/Backspace buttons
			skb.curRow = 0;
			if(skb.curCol < SKB_NUM_COLS/2)
				item = Menu_FindItemByName(menu, SKB_DELETE_NAME);
			else
				item = Menu_FindItemByName(menu, SKB_ACCEPT_NAME);
			Item_SetFocus(item, 0, 0);
			break;

		case A_CURSOR_DOWN:
			// Wrap to the Accept/Backspace buttons
			skb.curRow--;
			if(skb.curCol < SKB_NUM_COLS/2)
				item = Menu_FindItemByName(menu, SKB_DELETE_NAME);
			else
				item = Menu_FindItemByName(menu, SKB_ACCEPT_NAME);
			Item_SetFocus(item, 0, 0);
			break;

		case A_CURSOR_LEFT:
			// Wrap to the right side of the KB
			if(skb.curRow == SKB_NUM_ROWS-1)
				skb.curCol = SKB_NUM_LETTERS % SKB_NUM_COLS - 1;
			else
				skb.curCol = SKB_NUM_COLS - 1;
			break;

		case A_CURSOR_RIGHT:
			// Wrap to the left side of the KB
			skb.curCol=0;
			break;
		default:
			break;
		}
		skb.activeKey = skb.curRow * SKB_NUM_COLS + skb.curCol;
	}

	return qtrue;
}



vec4_t skb_color_unfocus = {0.7f, 0.7f, 0.8f, 1.0f};
vec4_t skb_color_focus = {0.78f, 0.471f, 0.161f, 1.0f};

static void UI_SoftKeyboardAccept_Draw()
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item = Menu_FindItemByName(menu, SKB_ACCEPT_NAME);
	int x = SKB_OK_X;
	int y = SKB_OK_Y;
	x -= Text_Width("OK", 1.0f, 2) / 2;
	y -= Text_Height("OK", 1.0f, 2) / 2;
	if(item->window.flags & WINDOW_HASFOCUS)
		Text_Paint(x, y, 1.0f, skb_color_focus, "OK", 1000, 0, 0, 4);
	else
		Text_Paint(x, y, 1.0f, skb_color_unfocus, "OK", 1000, 0, 0, 4);
}

static void UI_SoftKeyboardDelete_Draw()
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item = Menu_FindItemByName(menu, SKB_DELETE_NAME);
	int x = SKB_BACKSPACE_X;
	int y = SKB_BACKSPACE_Y;
	x -= Text_Width("Backspace", 1.0f, 2) / 2;
	y -= Text_Height("Backspace", 1.0f, 2) / 2;
	if(item->window.flags & WINDOW_HASFOCUS)
        Text_Paint(x, y, 1.0f, skb_color_focus, "Backspace", 1000, 0, 0, 4);
	else
        Text_Paint(x, y, 1.0f, skb_color_unfocus, "Backspace", 1000, 0, 0, 4);
}

static void UI_SoftKeyboard_Draw()
{
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *item = Menu_FindItemByName(menu, SKB_KEYBOARD_NAME);

//draw each letter on the screen at the appropriate coordinates
	int x,y;
	float size;
	vec4_t *color;
	for(int cl=0; cl<SKB_NUM_LETTERS; cl++)
	{
		if(skb.activeKey == cl && (item->window.flags & WINDOW_HASFOCUS))
		{
			color = &skb_color_focus;
			if(skb.pulse_up)
			{
				skb.pulse_size += SKB_PULSE_SPEED;
				if(skb.pulse_size > SKB_PULSE_LARGE)
					skb.pulse_up = false;
			}
			else
			{
				skb.pulse_size -= SKB_PULSE_SPEED;
				if(skb.pulse_size < SKB_PULSE_SMALL)
					skb.pulse_up = true;
			}
			size = skb.pulse_size;
		}
		else
		{
			color = &skb_color_unfocus;
			size = 1.0f;
		}

		x = (cl%SKB_NUM_COLS) * SKB_SPACE_H + SKB_LEFT;
		x -= (Text_Width(letters[cl], size, FONT_MEDIUM)) / 2;
		y = (cl/SKB_NUM_COLS) * SKB_SPACE_V + SKB_TOP;
		y -= ((Text_Height(letters[cl], size, 2)) / 2) * 1.5;

		Text_Paint(x, y, size, *color, letters[cl], 1000, 0, 0, 4);
	}

	char strtmp[SKB_STRING_LENGTH + 1];
	trap_Cvar_VariableStringBuffer("ui_profileNameSelect", strtmp, SKB_STRING_LENGTH+1);
	Text_Paint(SKB_STRING_LEFT, SKB_STRING_TOP, 1.5f, skb_color_unfocus, strtmp, 1000, 0, 0, 4);
}

static void UI_DrawInvisibleVoteListener()
{
	if(cgs.voteTime > 0)
		Menus_OpenByName("vote_alreadycalled");
}

static void UI_DrawVoteDesc()
{
	char votedesc1[64] = "";
	char *votedesc2;
	char *voteparam = "";

	strcat(votedesc1, cgs.voteCaller);
	strcat(votedesc1, " proposed a vote");

	if (strncmp(cgs.voteString, "map_restart", 11)==0)
	{
		votedesc2 = "to restart the game.";
	}
	else if (strncmp(cgs.voteString, "vstr nextmap",12)==0)
	{
		votedesc2 = "to proceed to the next map.";
	}
	else if (strncmp(cgs.voteString, "g_doWarmup",10)==0)
	{
		votedesc2 = "to play a warmup game.";
	}
	else if (strncmp(cgs.voteString, "g_gametype",10)==0)
	{
		votedesc2 = "to change to the following gametype:";
		voteparam = cgs.voteString+11; 
	}
	else if (strncmp(cgs.voteString, "map", 3)==0)
	{
		votedesc2 = "to switch to the following map:";
		voteparam = cgs.voteString+4;
	}
	else if (strncmp(cgs.voteString, "kick", 4)==0)
	{
		votedesc2 = "to kick the following player:";
		voteparam = cgs.voteString+5;
	}
	else
	{
		votedesc2 = "to DEFAULT MESSAGE.";
		voteparam = "DEFAULT MESSAGE";
	}
	menuDef_t *currmenu = Menu_GetFocused();
	itemDef_t *thisitem = Menu_FindItemByName(currmenu, "current_vote_desc");
	Window *thiswindow = &(thisitem->window);
	int font = thisitem->iMenuFont;
	float size = thisitem->textscale;
	int x,y;

	x = thiswindow->rect.x + thiswindow->rect.w/2;
	x -= (Text_Width(votedesc1, size, font) / 2);
	y = thiswindow->rect.y;
	Text_Paint(x, y, size, thiswindow->foreColor, votedesc1, 1000, 0, 0, font);

	x = thiswindow->rect.x + thiswindow->rect.w/2;
	x -= (Text_Width(votedesc2, size, font) / 2);
	y = thiswindow->rect.y + ((Text_Height(votedesc1, size, font) / 2) * 2.0);
	Text_Paint(x, y, size, thiswindow->foreColor, votedesc2, 1000, 0, 0, font);

	if(strcmp(voteparam, "") != 0)
	{
		x = thiswindow->rect.x + thiswindow->rect.w/2;
		x -= (Text_Width(voteparam, size, font) / 2);
		y = thiswindow->rect.y + thiswindow->rect.h;
		Text_Paint(x, y, size, thiswindow->foreColor, voteparam, 1000, 0, 0, font);
	}
}

static void UI_DrawPlayerKickDesc()
{
	if(!xbOnlineInfo.xbPlayerList[DEDICATED_SERVER_INDEX].isActive)
	{
		//disable the listbox if there is only one player, the server
		if(uiInfo.playerCount > 1)
			trap_Cvar_SetValue("ui_showPlayerListbox", 1);
		else
			trap_Cvar_SetValue("ui_showPlayerListbox", 0);
	}
	else
	{
		// when in a dedicated server, there always must be at least one client
		// so always display the listbox
		trap_Cvar_SetValue("ui_showPlayerListbox", 1);
	}
}


/*
==================
UI_MapCountByCurrentGameType
==================
*/
static int UI_MapCountByCurrentGameType() {
	char info[MAX_INFO_STRING];
	int i, c, game;
	c = 0;

	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	game = atoi(Info_ValueForKey(info, "g_gametype"));

	if (game == GT_SINGLE_PLAYER) {
		game++;
	} 
	if (game == GT_TEAM) {
		game = GT_FFA;
	}
	if (game == GT_HOLOCRON || game == GT_JEDIMASTER) {
		game = GT_FFA;
	}

	for (i = 0; i < uiInfo.mapCount; i++) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << game)) {
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

/*
==========
UI_UpdateMoves()
==========
*/
static void UI_UpdateMoves( void )
{
	uiInfo.movesTitleIndex	= (short)trap_Cvar_VariableValue("ui_move_title");
	if(uiInfo.movesTitleIndex > 5 || uiInfo.movesTitleIndex < 0)
		uiInfo.movesTitleIndex = 0;

	short index	= (short)trap_Cvar_VariableValue("ui_moves");
	if(index > 15 || index < 0)
		index = 0;


	itemDef_t *item;
	menuDef_t *menu;
	modelDef_t *modelPtr;

	menu = Menus_FindByName("rules_moves");

	if (menu)
	{
		item = (itemDef_t *) Menu_FindItemByName((menuDef_t *) menu, "character");
		if (item)
		{
			modelPtr = (modelDef_t*)item->typeData;
			if (modelPtr)
			{
				char modelPath[MAX_QPATH];
				int animRunLength;

				ItemParse_model_g2anim_go( item,  datapadMoveData[uiInfo.movesTitleIndex][index].anim );

				Com_sprintf( modelPath, sizeof( modelPath ), "models/players/%s/model.glm", UI_Cvar_VariableString ( "ui_char_model" ) );
				ItemParse_asset_model_go( item, modelPath, &animRunLength );
				UI_UpdateCharacterSkin();

				uiInfo.moveAnimTime = uiInfo.uiDC.realTime + animRunLength;

				if (datapadMoveData[uiInfo.movesTitleIndex][index].anim)
				{

					// Play sound for anim
					if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_FORCE_JUMP)
					{
						trap_S_StartLocalSound( uiInfo.uiDC.Assets.moveJumpSound, CHAN_LOCAL );
					}
					else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_ROLL)
					{
						trap_S_StartLocalSound( uiInfo.uiDC.Assets.moveRollSound, CHAN_LOCAL );
					}
					else if (datapadMoveData[uiInfo.movesTitleIndex][index].sound == MDS_SABER)
					{
						// Randomly choose one sound
						int soundI = Q_irand( 1, 6 );
						sfxHandle_t *soundPtr;
						soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound1;
						if (soundI == 2)
						{
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound2;
						}
						else if (soundI == 3)
						{
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound3;
						}
						else if (soundI == 4)
						{
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound4;
						}
						else if (soundI == 5)
						{
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound5;
						}
						else if (soundI == 6)
						{
							soundPtr = &uiInfo.uiDC.Assets.datapadmoveSaberSound6;
						}

						trap_S_StartLocalSound( *soundPtr, CHAN_LOCAL );
					}

					if (datapadMoveData[uiInfo.movesTitleIndex][index].desc)
					{
						trap_Cvar_Set( "ui_move_desc", datapadMoveData[uiInfo.movesTitleIndex][index].desc);
					}
				}
				UI_SaberAttachToChar( item );
			}
		}
	}
}
void UpdatePrevBotSlot( void )
{


	int botSlot		= trap_Cvar_VariableValue("bot_minplayers");
	int humSlot		= trap_Cvar_VariableValue("ui_publicSlots");
	int gametype	= trap_Cvar_VariableValue("xb_gameType");
	int dedicated	= trap_Cvar_VariableValue("ui_dedicated");
	int	end			= 10;
	int	start		= 0;
	int type		= trap_Cvar_VariableValue("ui_netGameType");
	int calltype	= trap_Cvar_VariableValue("xblBotSlotCallType");

	menuDef_t*	parent	= Menu_GetFocused();

	bool	isAdvancedRules = !strcmp(parent->window.name, "advanced_rules_menu");
		

	if(!dedicated)
	{
		end			= 8;
		start		= 1;
	}

	switch(gametype)
	{
	case 0:	// bot match
		if(isAdvancedRules || calltype)
			break;
		start		= 2;
		if(type == 2 && botSlot == start)
			trap_Cvar_Set("bot_minplayers", va("%d", start + 1));
		else if( botSlot == end)
			trap_Cvar_Set("bot_minplayers", va("%d", start));
		break;
	case 1:	// splitscreen
		if(isAdvancedRules)
			break;
		start		= 2;
		if(type == 2 && botSlot == start)
			trap_Cvar_Set("bot_minplayers", va("%d", start + 1));
		else if( botSlot == end)
			trap_Cvar_Set("bot_minplayers", va("%d", start));
		break;
	case 2:	// system link
	case 3:	// xbox live
		if(!isAdvancedRules)
			break;
		if(botSlot == end)
			trap_Cvar_Set("bot_minplayers", va("%d",start));
		break;
	default:
		break;
	}

}
void UpdateNextBotSlot( void )
{

	int botSlot		= trap_Cvar_VariableValue("bot_minplayers");
	int humSlot		= trap_Cvar_VariableValue("ui_publicSlots");
	int gametype	= trap_Cvar_VariableValue("xb_gameType");
	int dedicated	= trap_Cvar_VariableValue("ui_dedicated");
	int	end			= 10;
	int	start		= 0;
	int type		= trap_Cvar_VariableValue("ui_netGameType");
	int calltype	= trap_Cvar_VariableValue("xblBotSlotCallType");

	menuDef_t*	parent	= Menu_GetFocused();
	bool	isAdvancedRules = !strcmp(parent->window.name, "advanced_rules_menu");

	if(!dedicated)
	{
		end			= 8;
		start		= 1;
	}

	switch(gametype)
	{
	case 0:	// bot match
		if(isAdvancedRules || calltype)
			break;

		start		= 2;
		if(botSlot == start)
			trap_Cvar_Set("bot_minplayers", va("%d", end));
		break;
	case 1:	// splitscreen
		if(isAdvancedRules)
			break;

		start		= 2;
		if(botSlot == start)
			trap_Cvar_Set("bot_minplayers", va("%d", end));
		break;
	case 2:	// system link
	case 3:	// xbox live
		if(!isAdvancedRules || ( calltype && botSlot == start ))
			break;
		if(botSlot > humSlot)
			trap_Cvar_Set("bot_minplayers", va("%d", humSlot));
		else if(botSlot == start)
			trap_Cvar_Set("bot_minplayers", va("%d", humSlot));
		break;
	default:
		break;
	}
}
#endif


