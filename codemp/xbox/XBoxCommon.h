#ifndef _XBOX_COMMON_H
#define _XBOX_COMMON_H

///////////////////////////////////////////////////////////
//
//	SHARED NETWORKING DATA
//
///////////////////////////////////////////////////////////
#include "Xtl.h"
#include "xonline.h"

// This is (MAX_CLIENTS + 1) - but a dedicated server is the only
// thing that can take the last slot - 0..11 are reserved for clients.
#define MAX_ONLINE_PLAYERS 11
#define DEDICATED_SERVER_INDEX (MAX_ONLINE_PLAYERS - 1)

#define XBOX_PORT		1000

// There are separate flags for being able to receive voice, and able to send voice.
// Because of voice through speakers, anyone has the potential to receive. By
// default, everyone does. Other people don't need to know if clients are using
// voice through speakers, except to change their icon in player/friends list.
//
// Note that VOICE_CAN_SEND implies VOICE_CAN_RECV, but not the other way around.

#define VOICE_CAN_RECV	0x01	// Can recv voice (headset or spkrs) and has their voice sockets up
#define VOICE_CAN_SEND	0x02	// Can send voice (headset)
#define MUTED_PLAYER	0x04	// We have muted this player
#define REMOTE_MUTED	0x08	// This player has us muted

// Information about the session we're currently playing in, filled in
// when joining either a live or syslink game.
struct XBLClientData_t
{
	XNKEY	    KeyExchangeKey;
	XNADDR	    HostAddress;
	XNKID	    SessionID;
	IN_ADDR     SrvAddr;
};
extern XBLClientData_t xbc;


struct XBPlayerInfo
{
    XUID    xuid;
	XNADDR	xbAddr;
	IN_ADDR	inAddr;
	char	name[XONLINE_GAMERTAG_SIZE];
	int 	refIndex;
	DWORD	flags;
	bool	isActive;
	int     friendshipStatus;

	explicit XBPlayerInfo(int refNum = -1) : refIndex(refNum), isActive(false) { }
};

struct XBOnlineInfo
{
	BYTE			localIndex;							// index into xbPlayerList
	XBPlayerInfo	xbPlayerList[MAX_ONLINE_PLAYERS];	// list of players
};
extern XBOnlineInfo xbOnlineInfo;


#define MAX_OFFLINE_HISTORY 10
#define HISTORY_OFFSET		MAX_ONLINE_PLAYERS

class XBHistoryEntry
{
public:
	XUID	xuid;
	char	name[XONLINE_GAMERTAG_SIZE];
	DWORD	flags;
	bool	isActive;
	int     friendshipStatus;

	DWORD	stamp;		// Used to locate oldest entry

	explicit XBHistoryEntry() : isActive(false) { }
};

class XBHistory
{
public:
	DWORD	stamp;
	XBHistoryEntry historyList[MAX_OFFLINE_HISTORY];

	explicit XBHistory( void ) : stamp( 0 ) { }
};
extern XBHistory xbOfflineHistory;


// All important Xbox stuff is sent inside infostrings, so we convert
// it to hex with the following utilities. (Thanks to Wolf)
#define XNADDR_STRING_LEN (2*sizeof(XNADDR)+1)
void XnAddrToString(const XNADDR *pxna, char* buffer);
void StringToXnAddr(XNADDR* pxna, const char* buffer);

#define XNKID_STRING_LEN (2*sizeof(XNKID)+1)
void XNKIDToString(const XNKID *pxnkid, char* buffer);
void StringToXNKID(XNKID* pxnkid, const char* buffer);

#define XNKEY_STRING_LEN (2*sizeof(XNKEY)+1)
void XNKEYToString(const XNKEY *pxnkey, char* buffer);
void StringToXNKEY(XNKEY* pxnkey, const char* buffer);

#define XUID_STRING_LEN (2*sizeof(XUID)+1)
void XUIDToString(const XUID *pxuid, char* buffer);
void StringToXUID(XUID* pxuid, const char* buffer);


//
// Xbox Error Popup Constants
//
// These are really UI data, but we put them here to avoid cluttering the PC headers
// even more. The error popup system needs a context when it returns a value to know
// what to do. One of these is saved off when we create the popup, and then it's
// queried when we get a response so we know what we're doing.
enum xbErrorPopupType
{
	XB_POPUP_NONE,
	XB_POPUP_CANNOT_CONNECT,
	XB_POPUP_AUTOUPDATE,
	XB_POPUP_REQUIRED_MESSAGE,
	XB_POPUP_OPTIONAL_MESSAGE,
	XB_POPUP_LOST_CONNECTION,
	XB_POPUP_INVALID_USER,
	XB_POPUP_DUPLICATE_LOGON,
	XB_POPUP_BUSY_SERVERS,
	XB_POPUP_CONFIRM_NEW_ACCOUNT,
	XB_POPUP_CONFIRM_LOGOFF,
	XB_POPUP_COM_ERROR,
	XB_POPUP_QUICKMATCH_NO_RESULTS,
	XB_POPUP_OPTIMATCH_NO_RESULTS,
	XB_POPUP_MATCHMAKING_ERROR,
	XB_POPUP_SYSTEM_LINK_LOST,
	XB_POPUP_QUIT_CONFIRM,
	XB_POPUP_QUIT_HOST_CONFIRM,
	XB_POPUP_HOST_JOIN_CONFIRM,
	XB_POPUP_CONFIRM_FRIEND_REMOVE,
	XB_POPUP_RESPAWN_NEEDED,
	XB_POPUP_CORRUPT_SETTINGS,
};
void UI_xboxErrorPopup(xbErrorPopupType popup);
void UI_xboxPopupResponse(void);

#endif