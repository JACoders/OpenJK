// XBLive.h
//

#ifndef XBLIVE_H
#define XBLIVE_H


#include <xtl.h>
#include <xonline.h>
#include "match.h"
#include "../qcommon/qcommon.h"

//constants
//
#define ANY_TYPE					NULL

#define VOICE_ANY					0
#define VOICE_ON					1
#define VOICE_OFF					2

#define MINIMUM_QOS                 2   // warn of lag if join below this


// Friend states stored in clientInfo
//
enum FriendState
{
    FRIEND_NO,
    FRIEND_YES,
    FRIEND_RQST,
    FRIEND_RQST_RCV,
    FRIEND_INV,
    FRIEND_INV_RCV
};

//Possibe menu interactions for friends
typedef enum
{
	UI_F_FRIENDREQUESTED,
	UI_F_FRIENDACCEPTED,
	UI_F_FRIENDREMOVE,
	UI_F_FRIENDDECLINE,
	UI_F_FRIENDBLOCK,
	UI_F_FRIENDCANCEL,
	UI_F_GAMEREQUESTED,
	UI_F_GAMEACCEPTED,
	UI_F_GAMEDECLINE,
	UI_F_GAMECANCEL,
	UI_F_GAMEFRIENDREMOVED,
	UI_F_JOINSESSION,
	UI_F_TOGGLEONLINE
} friendChoices_t;


extern bool logged_on;  // flag defined in XBLive.cpp indicating wether we are logged on to XBox Live or not
extern XONLINE_USER			XBLLoggedOnUsers[XONLINE_MAX_LOGON_USERS];	// The users logged on to Live
extern XONLINETASK_HANDLE	logonHandle;	// The sooper handle that sometimes needs to be serviced in other files

// A few things use the CSession directly to check if we're running a server, for example
extern CSession session;

// different ways we can join a game
//
enum JoinType
{
    VIA_FRIEND_JOIN,
    VIA_FRIEND_INVITE,
    VIA_QUICKMATCH,
    VIA_OPTIMATCH,
};

// XBox Live functionality
//

#ifdef _DEBUG
const char *getXBLErrorName(HRESULT hr);
#endif

// Stages of logging in. It's a complicated process - this makes it a little easier
//
enum XBLoginState
{
	LOGIN_PASSCODE_CHECK,
	LOGIN_CONNECT,
	LOGIN_USER_ERRORS,
	LOGIN_SERVICE_ERRORS,
	LOGIN_FINISH,
};

bool	XBL_PumpLogon( void );
void	XBL_Tick();
HRESULT XBL_Init();
bool	XBL_Login(XBLoginState loginState);
void	XBL_AbortLogin(void);
HRESULT XBL_Cleanup();
HRESULT XBL_RefreshUserList();
DWORD	XBL_GetNumAccounts(bool doNow);
int		XBL_GetSelectedAccountIndex(void);
void	XBL_SetAccountIndex(const int index);
bool	XBL_SetAccountIndexByXuid(const XUID *xuid);
XONLINE_USER* XBL_GetUserInfo(const int index);
void	XBL_DisplayLobbyMenu();

const XNKID		*SysLink_GetXNKID(void);
const XNKEY		*SysLink_GetXNKEY(void);
const XNADDR	*Net_GetXNADDR( DWORD *pStatus = NULL );
bool			Net_ConnectionStatus(void);


void	Syslink_PacketEvent( msg_t *msg );

// Utility to get the current session's XNKID. Works for both server and client,
// in both Live and system link
const XNKID		*Net_GetXNKID(void);
bool			Net_XboxConnect(const XNKID *pxnkid, const XNKEY *pxnkey, const XNADDR *pxnaddr);
bool			Net_XboxDisconnect(void);

// match making functionality
//
void	XBL_MM_Tick();
void	XBL_MM_Advertise();
bool	XBL_MM_QuickMatch(ULONGLONG GameType); // Optional: X_MATCH_NULL_INTEGER to omit
// Same arguments as COptiMatchQuery::Query(), except for map, which is sent in as the name
int		XBL_MM_Find_Session(ULONGLONG GameType, // Optional: X_MATCH_NULL_INTEGER to omit
							const char *mapName, // Optional: "any" to omit
							ULONGLONG MinimumPlayers,
							ULONGLONG MaximumPlayers,
							ULONGLONG FriendlyFire, // Optional: X_MATCH_NULL_INTEGER to omit
							ULONGLONG JediMastery, // Optional: X_MATCH_NULL_INTEGER to omit
							ULONGLONG SaberOnly, // Optional: X_MATCH_NULL_INTEGER to omit
							ULONGLONG Dedicated); // Optional: X_MATCH_NULL_INTEGER to omit
DWORD	XBL_MM_RemovePlayer( bool usePrivateSlot );
DWORD	XBL_MM_AddPlayer( bool usePrivateSlot );
bool	XBL_MM_PublicSlotAvailable( void );
bool	XBL_MM_ConnectViaSessionID( const XNKID *sessionID, bool invited );
void    XBL_MM_Shutdown( bool processLogon );
bool    XBL_MM_ThisSessionIsLagging( const XNKID *sessionID );
bool	XBL_MM_IsSessionIDValid( const XNKID *sessionID );
void    XBL_MM_JoinServer( void );
void    XBL_MM_DontJoinLowQOSSever( void );
void    XBL_MM_JoinLowQOSSever( void );
void    XBL_MM_SetJoinType( JoinType );
bool	XBL_MM_CanUsePrivateSlot( void );
void    XBL_MM_SetServerJoinSlot(int);

// Now used in the UI as well...
int		mapNameToIndex(const char *mapname);
const char *mapIndexToLongName(int index);
extern const int MAP_ARRAY_SIZE;

// System Link functions used to communicate with the UI
int				Syslink_GetNumServers( void );
//const char*		Syslink_GetServerName( const int index );
const char*		Syslink_GetServerMap( const int index );
const char*		Syslink_GetServerClients( const int index );
const char*		Syslink_GetServerGametype( const int index );
int				Syslink_GetServerSaberOnly( const int index );
int				Syslink_GetServerDisableForce( const int index );
void			Syslink_SetChosenServerIndex( const int index );
void			Syslink_Listen( bool bListen );

// Optimatch functions used to communicate with the UI
int				XBL_MM_GetNumServers( void );
const char*		XBL_MM_GetServerName( const int index );
const char*		XBL_MM_GetServerMap( const int index );
const char*		XBL_MM_GetServerClients( const int index );
const char*		XBL_MM_GetServerGametype( const int index );
int				XBL_MM_GetServerSaberOnly( const int index );
int				XBL_MM_GetServerDisableForce( const int index );
int				XBL_MM_GetServerPing( const int index );
void			XBL_MM_SetChosenServerIndex( const int index );
void			XBL_MM_CancelProbing( void );

// System link analog to MM
bool	SysLink_JoinServer( void );

// friends functionality
//
void			XBL_F_Init( void );
void			XBL_F_Tick( void );
void			XBL_F_Cleanup( void );
void			XBL_F_ReleaseFriendsList( void );
void			XBL_F_GenerateFriendsList( void );
void			XBL_F_SetState( DWORD flag, bool set_flag );
bool			XBL_F_GetState( DWORD flag );
void			XBL_F_OnClientLeaveSession( void );
// Friend functions used to communicate with UI
int				XBL_F_GetNumFriends( void );
XONLINE_FRIEND* XBL_F_GetChosenFriend( void );
XONLINE_FRIEND* XBL_F_GetFriendFromName( char* );
void			XBL_F_SetChosenFriendIndex( const int index );
HRESULT			XBL_F_PerformMenuAction( friendChoices_t action );
void			XBL_F_CheckJoinableStatus( bool force );

const char*		XBL_F_GetFriendName( const int index );
int				XBL_F_GetStatusIcon( const int index );
const char*		XBL_F_GetStatusString( const int index );
const char*		XBL_F_GetStatusString2( const int index );
int				XBL_F_GetVoiceIcon( const int index );
const char*		XBL_F_GetVoiceString( const int index );

FriendState		XBL_F_GetFriendStatus( const XUID* xuid );
void			XBL_F_AddFriend( const XUID *pXuid );
void			XBL_F_RemoveFriend( const XUID *pXuid );
void            XBL_F_GetTitleString( const DWORD titleID, char* titleString );

bool			XBL_F_FriendNotice( void );
bool			XBL_F_GameNotice( void );

// player list functionality
//
void			XBL_PL_Tick(void);
void			XBL_PL_OnClientLeaveSession();
void			XBL_PL_RemoveActivePeer(XUID* xuid, int index);
// Should include XBoxCommon.h at the top, or put all of this in one file, but whatever...
struct	XBPlayerInfo;
void			XBL_PL_CheckHistoryList(XBPlayerInfo *newInfo);
void			XBL_PL_UpdatePlayerName( int index, const char *newName );

// Player functions used to communicate with UI
void			XBL_PL_Init( void );
void			XBL_PL_Cleanup( void );
int				XBL_PL_GetNumPlayers( void );
bool			XBL_PL_SetCurrentInfo( void );
void			XBL_PL_SetPlayerIndex( const int index );
const char*		XBL_PL_GetPlayerName( const int index );
int				XBL_PL_GetVoiceIcon( const int index );
int				XBL_PL_GetStatusIcon( const int index );

// Actions taken by the user via the playerlist popup window:
void			XBL_PL_ToggleMute( void );
void			XBL_PL_SendFeedBack( void );
void			XBL_PL_KickPlayer( void );
void			XBL_PL_MakeFriend( void );
void			XBL_PL_CancelFriend( void );

void			XBL_CheckServer();

// handy little buggers
//
void    charToWchar( WCHAR* wc, char* c );
void    wcharToChar( char* c, WCHAR* wc );

// stores cross title info from Single Player
//
extern BYTE     CrossTitle_InviteType;
extern XNKID    CrossTitle_TargetSessionID;
extern char     CrossTitle_FriendsGamerTag[XONLINE_GAMERTAG_SIZE];

#endif // XBLIVE_H
