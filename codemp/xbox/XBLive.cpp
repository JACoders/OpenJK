// XBLive.cpp
//

#include "xtl.h"
#include "XBLive.h"
#include "..\client\client.h"
#include "..\game\q_shared.h"
#include "..\qcommon\qcommon.h"
#include "..\xbox\xboxcommon.h"
#include "..\xbox\XBVoice.h"
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"

#include "../qcommon/xb_settings.h"

#define _UI	// I'm going to hell.
#include "..\ui\ui_shared.h"
#undef _UI

#include "..\qcommon\stringed_ingame.h"


#define SERVICE_COUNT	2		// number of services we currently wish to log into
#define CONNECTED_TICKS 20      // number of ticks between connection tests


XONLINETASK_HANDLE			logonHandle;
XONLINE_USER				XBLAccountusers[XONLINE_MAX_STORED_ONLINE_USERS] = {0};		// The accounts that are on stored on the console / mu
XONLINE_USER				XBLLoggedOnUsers[XONLINE_MAX_LOGON_USERS] = {0};			// The users that want to logon onto xbox live
DWORD						XBLservices[ SERVICE_COUNT ] = {0};							// desired services
bool						logged_on = false;											// flag to determine if we are logged onto xbox live
bool						xbl_inited = false;											// flag to determine if XBL is initialized
static DWORD				numOfXBLiveAccounts = 0;									// Number of current xbox live accounts found on machine
static int					selectedAccountIndex = 0;									// The selected account index into XBLAccountusers array that user wants to login with

// Moved here from XBVoice - makes more sense, I think
XBOnlineInfo xbOnlineInfo;

/*

Crazy insane lookup table of XBL error codes

*/
#ifdef _DEBUG
const char *getXBLErrorName(HRESULT hr)
{
	switch( hr )
	{
		case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
		case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
//		case WSACONNRESET: return "WSACONNRESET";

		case XONLINE_E_OVERFLOW: return "XONLINE_E_OVERFLOW";
		case XONLINE_E_NO_SESSION: return "XONLINE_E_NO_SESSION";
		case XONLINE_E_USER_NOT_LOGGED_ON: return "XONLINE_E_USER_NOT_LOGGED_ON";
		case XONLINE_E_NO_GUEST_ACCESS: return "XONLINE_E_NO_GUEST_ACCESS";
		case XONLINE_E_NOT_INITIALIZED: return "XONLINE_E_NOT_INITIALIZED";
		case XONLINE_E_NO_USER: return "XONLINE_E_NO_USER";
		case XONLINE_E_INTERNAL_ERROR: return "XONLINE_E_INTERNAL_ERROR";
		case XONLINE_E_OUT_OF_MEMORY: return "XONLINE_E_OUT_OF_MEMORY";
		case XONLINE_E_TASK_BUSY: return "XONLINE_E_TASK_BUSY";
		case XONLINE_E_SERVER_ERROR: return "XONLINE_E_SERVER_ERROR";
		case XONLINE_E_IO_ERROR: return "XONLINE_E_IO_ERROR";
		case XONLINE_E_BAD_CONTENT_TYPE: return "XONLINE_E_BAD_CONTENT_TYPE";
		case XONLINE_E_USER_NOT_PRESENT: return "XONLINE_E_USER_NOT_PRESENT";
		case XONLINE_E_PROTOCOL_MISMATCH: return "XONLINE_E_PROTOCOL_MISMATCH";
		case XONLINE_E_INVALID_SERVICE_ID: return "XONLINE_E_INVALID_SERVICE_ID";
		case XONLINE_E_INVALID_REQUEST: return "XONLINE_E_INVALID_REQUEST";

		case XONLINE_E_LOGON_NO_NETWORK_CONNECTION: return "XONLINE_E_LOGON_NO_NETWORK_CONNECTION";

		case XONLINE_S_LOGON_CONNECTION_ESTABLISHED: return "XONLINE_S_LOGON_CONNECTION_ESTABLISHED";

		case XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE: return "XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE";
		case XONLINE_E_LOGON_UPDATE_REQUIRED: return "XONLINE_E_LOGON_UPDATE_REQUIRED";
		case XONLINE_E_LOGON_SERVERS_TOO_BUSY: return "XONLINE_E_LOGON_SERVERS_TOO_BUSY";
		case XONLINE_E_LOGON_CONNECTION_LOST: return "XONLINE_E_LOGON_CONNECTION_LOST";
		case XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON: return "XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON";
		case XONLINE_E_LOGON_INVALID_USER: return "XONLINE_E_LOGON_INVALID_USER";

		case XONLINE_E_LOGON_SERVICE_NOT_REQUESTED: return "XONLINE_E_LOGON_SERVICE_NOT_REQUESTED";
		case XONLINE_E_LOGON_SERVICE_NOT_AUTHORIZED: return "XONLINE_E_LOGON_SERVICE_NOT_AUTHORIZED";
		case XONLINE_E_LOGON_SERVICE_TEMPORARILY_UNAVAILABLE: return "XONLINE_E_LOGON_SERVICE_TEMPORARILY_UNAVAILABLE";

		case XONLINE_S_LOGON_USER_HAS_MESSAGE: return "XONLINE_S_LOGON_USER_HAS_MESSAGE";

		case XONLINE_E_LOGON_USER_ACCOUNT_REQUIRES_MANAGEMENT: return "XONLINE_E_LOGON_USER_ACCOUNT_REQUIRES_MANAGEMENT";

		case XONLINE_S_LOGON_COMMIT_USER_CHANGE: return "XONLINE_S_LOGON_COMMIT_USER_CHANGE";
		case XONLINE_S_LOGON_USER_CHANGE_COMPLETE: return "XONLINE_S_LOGON_USER_CHANGE_COMPLETE";

		case XONLINE_E_LOGON_CHANGE_USER_FAILED: return "XONLINE_E_LOGON_CHANGE_USER_FAILED";

		case XONLINE_E_LOGON_MU_NOT_MOUNTED: return "XONLINE_E_LOGON_MU_NOT_MOUNTED";
		case XONLINE_E_LOGON_MU_IO_ERROR: return "XONLINE_E_LOGON_MU_IO_ERROR";
		case XONLINE_E_LOGON_NOT_LOGGED_ON: return "XONLINE_E_LOGON_NOT_LOGGED_ON";

		case XONLINE_E_NOTIFICATION_BAD_CONTENT_TYPE: return "XONLINE_E_NOTIFICATION_BAD_CONTENT_TYPE";
		case XONLINE_E_NOTIFICATION_REQUEST_TOO_SMALL: return "XONLINE_E_NOTIFICATION_REQUEST_TOO_SMALL";
		case XONLINE_E_NOTIFICATION_INVALID_MESSAGE_TYPE: return "XONLINE_E_NOTIFICATION_INVALID_MESSAGE_TYPE";
		case XONLINE_E_NOTIFICATION_NO_ADDRESS: return "XONLINE_E_NOTIFICATION_NO_ADDRESS";
		case XONLINE_E_NOTIFICATION_INVALID_PUID: return "XONLINE_E_NOTIFICATION_INVALID_PUID";
		case XONLINE_E_NOTIFICATION_NO_CONNECTION: return "XONLINE_E_NOTIFICATION_NO_CONNECTION";
		case XONLINE_E_NOTIFICATION_SEND_FAILED: return "XONLINE_E_NOTIFICATION_SEND_FAILED";
		case XONLINE_E_NOTIFICATION_RECV_FAILED: return "XONLINE_E_NOTIFICATION_RECV_FAILED";
		case XONLINE_E_NOTIFICATION_MESSAGE_TRUNCATED: return "XONLINE_E_NOTIFICATION_MESSAGE_TRUNCATED";
		case XONLINE_E_NOTIFICATION_SERVER_BUSY: return "XONLINE_E_NOTIFICATION_SERVER_BUSY";
		case XONLINE_E_NOTIFICATION_LIST_FULL: return "XONLINE_E_NOTIFICATION_LIST_FULL";
		case XONLINE_E_NOTIFICATION_BLOCKED: return "XONLINE_E_NOTIFICATION_BLOCKED";
		case XONLINE_E_NOTIFICATION_FRIEND_PENDING: return "XONLINE_E_NOTIFICATION_FRIEND_PENDING";
		case XONLINE_E_NOTIFICATION_FLUSH_TICKETS: return "XONLINE_E_NOTIFICATION_FLUSH_TICKETS";
		case XONLINE_E_NOTIFICATION_TOO_MANY_REQUESTS: return "XONLINE_E_NOTIFICATION_TOO_MANY_REQUESTS";
		case XONLINE_E_NOTIFICATION_USER_ALREADY_EXISTS: return "XONLINE_E_NOTIFICATION_USER_ALREADY_EXISTS";
		case XONLINE_E_NOTIFICATION_USER_NOT_FOUND: return "XONLINE_E_NOTIFICATION_USER_NOT_FOUND";
		case XONLINE_E_NOTIFICATION_OTHER_LIST_FULL: return "XONLINE_E_NOTIFICATION_OTHER_LIST_FULL";
		case XONLINE_E_NOTIFICATION_SELF: return "XONLINE_E_NOTIFICATION_SELF";
		case XONLINE_E_NOTIFICATION_SAME_TITLE: return "XONLINE_E_NOTIFICATION_SAME_TITLE";
		case XONLINE_E_NOTIFICATION_NO_TASK: return "XONLINE_E_NOTIFICATION_NO_TASK";

		case XONLINE_E_MATCH_INVALID_SESSION_ID: return "XONLINE_E_MATCH_INVALID_SESSION_ID";
		case XONLINE_E_MATCH_INVALID_TITLE_ID: return "XONLINE_E_MATCH_INVALID_TITLE_ID";
		case XONLINE_E_MATCH_INVALID_DATA_TYPE: return "XONLINE_E_MATCH_INVALID_DATA_TYPE";
		case XONLINE_E_MATCH_REQUEST_TOO_SMALL: return "XONLINE_E_MATCH_REQUEST_TOO_SMALL";
		case XONLINE_E_MATCH_REQUEST_TRUNCATED: return "XONLINE_E_MATCH_REQUEST_TRUNCATED";
		case XONLINE_E_MATCH_INVALID_SEARCH_REQ: return "XONLINE_E_MATCH_INVALID_SEARCH_REQ";
		case XONLINE_E_MATCH_INVALID_OFFSET: return "XONLINE_E_MATCH_INVALID_OFFSET";
		case XONLINE_E_MATCH_INVALID_ATTR_TYPE: return "XONLINE_E_MATCH_INVALID_ATTR_TYPE";
		case XONLINE_E_MATCH_INVALID_VERSION: return "XONLINE_E_MATCH_INVALID_VERSION";
		case XONLINE_E_MATCH_OVERFLOW: return "XONLINE_E_MATCH_OVERFLOW";
		case XONLINE_E_MATCH_INVALID_RESULT_COL: return "XONLINE_E_MATCH_INVALID_RESULT_COL";
		case XONLINE_E_MATCH_INVALID_STRING: return "XONLINE_E_MATCH_INVALID_STRING";
		case XONLINE_E_MATCH_STRING_TOO_LONG: return "XONLINE_E_MATCH_STRING_TOO_LONG";
		case XONLINE_E_MATCH_BLOB_TOO_LONG: return "XONLINE_E_MATCH_BLOB_TOO_LONG";
		case XONLINE_E_MATCH_INVALID_ATTRIBUTE_ID: return "XONLINE_E_MATCH_INVALID_ATTRIBUTE_ID";
		case XONLINE_E_MATCH_SESSION_ALREADY_EXISTS: return "XONLINE_E_MATCH_SESSION_ALREADY_EXISTS";
		case XONLINE_E_MATCH_CRITICAL_DB_ERR: return "XONLINE_E_MATCH_CRITICAL_DB_ERR";
		case XONLINE_E_MATCH_NOT_ENOUGH_COLUMNS: return "XONLINE_E_MATCH_NOT_ENOUGH_COLUMNS";
		case XONLINE_E_MATCH_PERMISSION_DENIED: return "XONLINE_E_MATCH_PERMISSION_DENIED";
		case XONLINE_E_MATCH_INVALID_PART_SCHEME: return "XONLINE_E_MATCH_INVALID_PART_SCHEME";
		case XONLINE_E_MATCH_INVALID_PARAM: return "XONLINE_E_MATCH_INVALID_PARAM";
		case XONLINE_E_MATCH_DATA_TYPE_MISMATCH: return "XONLINE_E_MATCH_DATA_TYPE_MISMATCH";
		case XONLINE_E_MATCH_SERVER_ERROR: return "XONLINE_E_MATCH_SERVER_ERROR";
		case XONLINE_E_MATCH_NO_USERS: return "XONLINE_E_MATCH_NO_USERS";
		case XONLINE_E_MATCH_INVALID_BLOB: return "XONLINE_E_MATCH_INVALID_BLOB";

		case XONLINE_S_OFFERING_NEW_CONTENT: return "XONLINE_S_OFFERING_NEW_CONTENT";
		case XONLINE_S_OFFERING_NO_NEW_CONTENT: return "XONLINE_S_OFFERING_NO_NEW_CONTENT";

		case XONLINE_E_OFFERING_BAD_REQUEST: return "XONLINE_E_OFFERING_BAD_REQUEST";
		case XONLINE_E_OFFERING_INVALID_USER: return "XONLINE_E_OFFERING_INVALID_USER";
		case XONLINE_E_OFFERING_INVALID_OFFER_ID: return "XONLINE_E_OFFERING_INVALID_OFFER_ID";
		case XONLINE_E_OFFERING_INELIGIBLE_FOR_OFFER: return "XONLINE_E_OFFERING_INELIGIBLE_FOR_OFFER";
		case XONLINE_E_OFFERING_OFFER_EXPIRED: return "XONLINE_E_OFFERING_OFFER_EXPIRED";
		case XONLINE_E_OFFERING_SERVICE_UNREACHABLE: return "XONLINE_E_OFFERING_SERVICE_UNREACHABLE";
		case XONLINE_E_OFFERING_PURCHASE_BLOCKED: return "XONLINE_E_OFFERING_PURCHASE_BLOCKED";
		case XONLINE_E_OFFERING_PURCHASE_DENIED: return "XONLINE_E_OFFERING_PURCHASE_DENIED";
		case XONLINE_E_OFFERING_BILLING_SERVER_ERROR: return "XONLINE_E_OFFERING_BILLING_SERVER_ERROR";
		case XONLINE_E_OFFERING_OFFER_NOT_CANCELABLE: return "XONLINE_E_OFFERING_OFFER_NOT_CANCELABLE";
		case XONLINE_E_OFFERING_NOTHING_TO_CANCEL: return "XONLINE_E_OFFERING_NOTHING_TO_CANCEL";
		case XONLINE_E_OFFERING_ALREADY_OWN_MAX: return "XONLINE_E_OFFERING_ALREADY_OWN_MAX";
		case XONLINE_E_OFFERING_NO_CHARGE: return "XONLINE_E_OFFERING_NO_CHARGE";
		case XONLINE_E_OFFERING_PERMISSION_DENIED: return "XONLINE_E_OFFERING_PERMISSION_DENIED";
		case XONLINE_E_OFFERING_NAME_TAKEN: return "XONLINE_E_OFFERING_NAME_TAKEN";

		case XONLINE_E_BILLING_AUTHORIZATION_FAILED: return "XONLINE_E_BILLING_AUTHORIZATION_FAILED";
		case XONLINE_E_BILLING_CREDIT_CARD_EXPIRED: return "XONLINE_E_BILLING_CREDIT_CARD_EXPIRED";
		case XONLINE_E_BILLING_NON_ACTIVE_ACCOUNT: return "XONLINE_E_BILLING_NON_ACTIVE_ACCOUNT";
		case XONLINE_E_BILLING_INVALID_PAYMENT_INSTRUMENT_STATUS: return "XONLINE_E_BILLING_INVALID_PAYMENT_INSTRUMENT_STATUS";

		case XONLINE_E_UODB_KEY_ALREADY_EXISTS: return "XONLINE_E_UODB_KEY_ALREADY_EXISTS";

		case XONLINE_E_MSGSVR_INVALID_REQUEST: return "XONLINE_E_MSGSVR_INVALID_REQUEST";

		case XONLINE_E_FEEDBACK_NULL_TARGET: return "XONLINE_E_FEEDBACK_NULL_TARGET";
		case XONLINE_E_FEEDBACK_BAD_TYPE: return "XONLINE_E_FEEDBACK_BAD_TYPE";
		case XONLINE_E_FEEDBACK_CANNOT_LOG: return "XONLINE_E_FEEDBACK_CANNOT_LOG";

		case XONLINE_E_STAT_BAD_REQUEST: return "XONLINE_E_STAT_BAD_REQUEST";
		case XONLINE_E_STAT_INVALID_TITLE_OR_LEADERBOARD: return "XONLINE_E_STAT_INVALID_TITLE_OR_LEADERBOARD";
		case XONLINE_E_STAT_TOO_MANY_SPECS: return "XONLINE_E_STAT_TOO_MANY_SPECS";
		case XONLINE_E_STAT_TOO_MANY_STATS: return "XONLINE_E_STAT_TOO_MANY_STATS";
		case XONLINE_E_STAT_USER_NOT_FOUND: return "XONLINE_E_STAT_USER_NOT_FOUND";
		case XONLINE_E_STAT_SET_FAILED_0: return "XONLINE_E_STAT_SET_FAILED_0";
		case XONLINE_E_STAT_PERMISSION_DENIED: return "XONLINE_E_STAT_PERMISSION_DENIED";
		case XONLINE_E_STAT_LEADERBOARD_WAS_RESET: return "XONLINE_E_STAT_LEADERBOARD_WAS_RESET";
		case XONLINE_E_STAT_INVALID_ATTACHMENT: return "XONLINE_E_STAT_INVALID_ATTACHMENT";
		case XONLINE_S_STAT_CAN_UPLOAD_ATTACHMENT: return "XONLINE_S_STAT_CAN_UPLOAD_ATTACHMENT";

		case XONLINE_E_STORAGE_INVALID_REQUEST: return "XONLINE_E_STORAGE_INVALID_REQUEST";
		case XONLINE_E_STORAGE_ACCESS_DENIED: return "XONLINE_E_STORAGE_ACCESS_DENIED";
		case XONLINE_E_STORAGE_FILE_IS_TOO_BIG: return "XONLINE_E_STORAGE_FILE_IS_TOO_BIG";
		case XONLINE_E_STORAGE_FILE_NOT_FOUND: return "XONLINE_E_STORAGE_FILE_NOT_FOUND";
		case XONLINE_E_STORAGE_INVALID_ACCESS_TOKEN: return "XONLINE_E_STORAGE_INVALID_ACCESS_TOKEN";
		case XONLINE_E_STORAGE_CANNOT_FIND_PATH: return "XONLINE_E_STORAGE_CANNOT_FIND_PATH";
		case XONLINE_E_STORAGE_FILE_IS_ELSEWHERE: return "XONLINE_E_STORAGE_FILE_IS_ELSEWHERE";
		case XONLINE_E_STORAGE_INVALID_STORAGE_PATH: return "XONLINE_E_STORAGE_INVALID_STORAGE_PATH";
		case XONLINE_E_STORAGE_INVALID_FACILITY: return "XONLINE_E_STORAGE_INVALID_FACILITY";

		default:
			return "***UNKNOWN_XTL_ERROR***";
	}
}
#endif


/*
=====================

Conversion to/from strings for X-structs, stolen from Wolf

=====================
*/
static void DataToHexString(const BYTE* pData, int dataLen, char* pBuffer)
{
	char* pOrig = pBuffer;
	memset(pBuffer, 0, 2*dataLen+1); //The hex version of each byte is 2 chars
	for(int i = 0; i < dataLen; i++)
	{
		sprintf(pBuffer,  "%02x\0", pData[i]);
		pBuffer+=2;
	}
}

static void HexStringToData(const char* pBuffer, int bufferLen, BYTE* pData)
{
	memset(pData, 0, bufferLen/2);
	
	char sByte[3];
	for(int i = 0; i < bufferLen; i+=2)
	{
		sByte[0] = pBuffer[0];
		sByte[1] = pBuffer[1];
		sByte[2] = '\0';

		BYTE b = strtol(sByte, NULL, 16);

		*pData = b;
		pData++;
		pBuffer+=2;
	}
}

void XnAddrToString(const XNADDR *pxna, char *buffer)
{
	DataToHexString((const BYTE *)pxna, sizeof(XNADDR), buffer);
}

void StringToXnAddr(XNADDR* pxna, const char* buffer)
{
	int len = strlen(buffer);
	assert( (len + 1) == XNADDR_STRING_LEN );
	HexStringToData(buffer, len, (BYTE *)pxna);
}

void XNKIDToString(const XNKID *pxnkid, char* buffer)
{
	DataToHexString((const BYTE *)pxnkid, sizeof(XNKID), buffer);
}

void StringToXNKID(XNKID* pxnkid, const char* buffer)
{
	int len = strlen(buffer);
	assert( (len + 1) == XNKID_STRING_LEN );
	HexStringToData(buffer, len, (BYTE *)pxnkid);
}

void XNKEYToString(const XNKEY *pxnkey, char* buffer)
{
	DataToHexString((const BYTE *)pxnkey, sizeof(XNKEY), buffer);
}

void StringToXNKEY(XNKEY* pxnkey, const char* buffer)
{
	int len = strlen(buffer);
	assert( (len + 1) == XNKEY_STRING_LEN );
	HexStringToData(buffer, len, (BYTE *)pxnkey);
}

void XUIDToString(const XUID *pxuid, char* buffer)
{
	DataToHexString((const BYTE *)pxuid, sizeof(XUID), buffer);
}

void StringToXUID(XUID* pxuid, const char* buffer)
{
	int len = strlen(buffer);
	assert( (len + 1) == XUID_STRING_LEN );
	HexStringToData(buffer, len, (BYTE *)pxuid);
}


/*
==================

System Link quick n' dirty utilities

==================
*/

// We keep a single static copy of this crap, just for system link games.
// We should probably be reusing session, but I'm lazy, and that's ugly.
struct XBLSysLinkData_t
{
	XNKEY	    xnkey;
	XNKID	    xnkid;
	bool		initialized;
	bool		listening;
	int			chosen;

	XBLSysLinkData_t(void) : initialized(false), listening(false) { }

	void Initialize(void)
	{
		if (initialized)
			return;

		INT err = XNetCreateKey(&xnkid, &xnkey);
		assert( !err );

		err = XNetRegisterKey(&xnkid, &xnkey);
		assert( !err );

		initialized = true;
	}
};

static XBLSysLinkData_t	sysLink;
static bool				sysLinkInitialized = false;

const XNKID *SysLink_GetXNKID(void)
{
	sysLink.Initialize();
	return &sysLink.xnkid;
}

const XNKEY *SysLink_GetXNKEY(void)
{
	sysLink.Initialize();
	return &sysLink.xnkey;
}

// Handles broadcast packets received from system link servers advertising their existence
//
void Syslink_PacketEvent( msg_t *msg )
{
	// We only deal with these if we're listening for servers
	if( !sysLink.listening )
		return;

	// Throw away packets that are too small, or don't have the -1 "header":
	if ( msg->cursize < 4 || *(int *)msg->data != -1 )
		return;

	// Start pulling data from the packet:
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );	// skip the -1

	// Grab the command:
	char *s = MSG_ReadStringLine( msg );
	Cmd_TokenizeString( s );
	char *c = Cmd_Argv(0);

	// The only valid command is infoResponse (though this is no longer an accurate term):
	if ( Q_stricmp(c, "infoResponse") != 0)
		return;

	// Grab the real payload:
	char *infoString = MSG_ReadString( msg );
	// Decode the XNADDR first, so we can check for dupes:
	XNADDR tempAddr;
	StringToXnAddr( &tempAddr, Info_ValueForKey(infoString, "xnaddr") );

	// Find a slot for it in the localServers array:
	for ( int i = 0 ; i < MAX_OTHER_SERVERS ; i++ )
	{
		// Empty slot - this ordering is ok, as we always fill in order:
		if ( !cls.localServers[i].lastUpdate )
			break;

		// Whenever we get a dupe, update the info:
		if ( memcmp( &tempAddr, &cls.localServers[i].HostAddress, sizeof(XNADDR) ) == 0 )
			break;
	}

	// Silently fail if there are too many servers on the LAN:
	if ( i == MAX_OTHER_SERVERS )
		return;

	// Get the slot that we'll be filling in:
	serverInfo_t *server = &cls.localServers[i];

	// Increase the list size if this isn't a dupe:
	if( !server->lastUpdate )
		cls.numlocalservers++;

	Q_strncpyz(server->mapName, Info_ValueForKey(infoString, "mapname"), sizeof(server->mapName));
	server->gameType = atoi(Info_ValueForKey(infoString, "gametype"));
	server->saberOnly = atoi(Info_ValueForKey(infoString, "wdisable"));
	server->forceDisable = atoi(Info_ValueForKey(infoString, "fdisable"));
	server->clients = atoi(Info_ValueForKey(infoString, "clients"));
	server->maxClients = atoi(Info_ValueForKey(infoString, "sv_maxclients"));

	// Re-stamp the entry with current time:
	server->lastUpdate = Sys_Milliseconds();

	StringToXnAddr(&server->HostAddress, Info_ValueForKey(infoString, "xnaddr"));
	StringToXNKID(&server->SessionID, Info_ValueForKey(infoString, "xnkid"));
	StringToXNKEY(&server->KeyExchangeKey, Info_ValueForKey(infoString, "xnkey"));
}

// Used by the UI to control whether or not we listen to system link advert packets:
void Syslink_Listen( bool bListen )
{
	sysLink.listening = bListen;

	// Mark all slots as unused (invisible) when we start listening:
	if( bListen )
	{
		cls.numlocalservers = 0;
		for( int i = 0; i < MAX_OTHER_SERVERS; ++i )
			cls.localServers[i].lastUpdate = 0;
	}
}

void Syslink_SetChosenServerIndex( const int index )
{
	sysLink.chosen = index;
}

//=============================================================================


// Centralized function for handling connection setup and storage of server
// info into xbc. ALL connections should come through here at some point.
//
bool Net_XboxConnect(const XNKID *pxnkid, const XNKEY *pxnkey, const XNADDR *pxnaddr)
{
	// 1) Register the session key
	if ( XNetRegisterKey( pxnkid, pxnkey ) )
	{
		Com_Error( ERR_FATAL, "Unable to register key for connect\n" );
	}

	// 2) Convert the address to a winsock usable format
	IN_ADDR pseudoIP;
	if ( XNetXnAddrToInAddr( pxnaddr, pxnkid, &pseudoIP ) )
	{
		Com_Error( ERR_FATAL, "Couldn't convert XNADDR to IN_ADDR for connect\n" );
	}

	// We're going to connect, store this information off for reference
	memcpy(&xbc.HostAddress,	pxnaddr,	sizeof(XNADDR));
	memcpy(&xbc.KeyExchangeKey,	pxnkey,		sizeof(XNKEY));
	memcpy(&xbc.SessionID,		pxnkid,		sizeof(XNKID));
	memcpy(&xbc.SrvAddr,		&pseudoIP,	sizeof(IN_ADDR));

	// Issue the connect command
	Cbuf_ExecuteText( EXEC_APPEND, va( "connect %i.%i.%i.%i\n", pseudoIP.S_un.S_un_b.s_b1, 
																pseudoIP.S_un.S_un_b.s_b2,
																pseudoIP.S_un.S_un_b.s_b3,
																pseudoIP.S_un.S_un_b.s_b4 ) );

	// Start up voice system as well - Live does this when you login
	if( !logged_on )
		g_Voice.Initialize();

	return true;
}

// Tears down the current client connection. Should ONLY be called by non-servers,
// and ONLY when we're really leaving the session. We can't verify much though -
// as we can be called after SV_Shutdown has been called (see Com_Error for an example).
bool Net_XboxDisconnect(void)
{
	XNetUnregisterKey( &xbc.SessionID );
	memset(&xbc.SessionID, 0, sizeof(XNKID));

	// Tell voice that we left the game
	g_Voice.LeaveSession();

	// If we're playing system link, shutdown voice entirely, Live needs to keep it up
	if( !logged_on )
		g_Voice.Shutdown();

	if ( logged_on )
	{
		XBL_F_OnClientLeaveSession();
		XBL_PL_OnClientLeaveSession();
	}

	// Blow away our player list and such. Servers also do this in SV_Shutdown()
	memset( &xbOnlineInfo, 0, sizeof(xbOnlineInfo) );

	return true;
}

// Translates the UI's indexes into server structs, as there can be holes in the list
serverInfo_t *SysLink_GetServer( int index )
{
	if( index < 0 || index >= cls.numlocalservers )
		return NULL;

	for( int i = 0; i < MAX_OTHER_SERVERS; ++i )
	{
		// Is this entry valid?
		if( cls.localServers[i].lastUpdate )
		{
			if( index == 0 )
				return &cls.localServers[i];
			index--;
		}
	}

	// Should never happen
	return NULL;
}

// System Link analog to the XBL_MM_Join_Server function. UI used to issue the connect
// directly, but we need to do key management, so we've moved the code over here.
bool SysLink_JoinServer( void )
{
	if( sysLink.chosen < 0 || sysLink.chosen >= cls.numlocalservers )
		return false;

	serverInfo_t *server = SysLink_GetServer( sysLink.chosen );
	if( !server )
		return false;

	return Net_XboxConnect(&server->SessionID, &server->KeyExchangeKey, &server->HostAddress);
}


// Gets your XNADDR. Used by syslink, and lots of other people.
const XNADDR *Net_GetXNADDR( DWORD *pStatus /* = NULL */)
{
	static XNADDR hostAddr;
	DWORD dwStatus;

	do
	{
	   dwStatus = XNetGetTitleXnAddr( &hostAddr );
	} while ( dwStatus == XNET_GET_XNADDR_PENDING );

	// Error checking
	if ( dwStatus == XNET_GET_XNADDR_NONE )
	{
		assert(!"Error getting XBox title address.");
	}

	if (pStatus)
		*pStatus = dwStatus;

	return &hostAddr;
}

// Gets the XNKID for the current session - works for anyone
const XNKID *Net_GetXNKID(void)
{
	// Clients ALWAYS store the session ID in xbc. Servers use either their syslink
	// copy, or the one in the matchmaking structure.
	if (!com_sv_running->integer)
		return &xbc.SessionID;
	else if (logged_on)
		return &session.SessionID;
	else
		return SysLink_GetXNKID();
}


// Sets the account index into the array
//
void XBL_SetAccountIndex(const int index)
{
	selectedAccountIndex = index;
}

// Used by the code that handles stored invitations
bool XBL_SetAccountIndexByXuid(const XUID *xuid)
{
	for( int i = 0; i < numOfXBLiveAccounts; ++i )
	{
		if( XOnlineAreUsersIdentical( &XBLAccountusers[i].xuid, xuid ) )
		{
			selectedAccountIndex = i;
			return true;
		}
	}

	// Possible. Imagine someone accepts an invite with an MU account, then pulls it.
	// There's even time to go to the dash and delete the account.
	return false;
}

// Return the selected account index of the XBL user
//
int XBL_GetSelectedAccountIndex(void)
{
	return selectedAccountIndex;
}

#define ACCOUNT_CHECK_FREQUENCY 300

// Returns the total number of accounts found on the console
//
DWORD XBL_GetNumAccounts( bool doNow )
{
    // this is continuously called by the sign-in menu so use that to update resident accounts
    // in case someone plugs in an MMU while on that screen - oh, just dont do it every frame!
    //
    static int pass = 0;
    if( doNow )
        pass = ACCOUNT_CHECK_FREQUENCY;

    if( ++pass >= ACCOUNT_CHECK_FREQUENCY)
    {
        HRESULT result = XOnlineGetUsers( XBLAccountusers, &numOfXBLiveAccounts );
        pass=0;
    }

	return numOfXBLiveAccounts;
}

// Returns theuser info based on index of array
//
XONLINE_USER* XBL_GetUserInfo(const int index)
{
	if(index >= 0 && index < numOfXBLiveAccounts)
		return &XBLAccountusers[index];
	else
		return NULL;
}


// Refresh the list of XBox Live users found on this system
//
HRESULT XBL_RefreshUserList()
{
    // Wait for any inserted MUs to mount - may contain user info
	//
    while ( XGetDeviceEnumerationStatus() == XDEVICE_ENUMERATION_BUSY ) {}

	// XOnlineStartup calls XNetStartup and WSAStartup 
	// already done in Net_Init() via main()
	//
	XONLINE_STARTUP_PARAMS xosp = { 0 };
	HRESULT result = XOnlineStartup( &xosp );

	if( result < S_OK )
	{
		Com_Printf("XBLive - Failed at start (XOnlineStartup) \n");
		return result;
	}

    // Get accounts stored on the hard disk and XMUs
	//
    result = XOnlineGetUsers( XBLAccountusers, &numOfXBLiveAccounts );

	if( result < S_OK )
	{
		Com_Printf("XBLive - Error trying to find accounts\n");
		numOfXBLiveAccounts = 0;
	}

    return result;
}




// Set up the XBox Live functionality
//
HRESULT XBL_Init()
{
	if( xbl_inited ) // already inited
	{
		Com_Printf("XBLive is already initialized\n");
		return S_OK;
	}

	HRESULT result = XBL_RefreshUserList();

	if( result < S_OK )
	{
		Com_Printf("XBLive - XBL_Init failed!\n");
	}
	else
	{
		xbl_inited = true;
	}

	memset(&xbOnlineInfo, 0, sizeof(XBOnlineInfo));

	return result;
}


// Log onto Microsoft server. This is a long and complicated procedure.
// We break it up into stages, and tell the function where we want to resume.
//
bool XBL_Login( XBLoginState loginState )
{
	assert( !logged_on );

	// Stage ONE: Make sure that we have a valid account picked, then check for passcode.
	if( loginState <= LOGIN_PASSCODE_CHECK )
	{
		// Shouldn't happen:
		assert( numOfXBLiveAccounts > 0 );

		// Selected index should be valid. Does it need a passcode?
		if( XBLAccountusers[selectedAccountIndex].dwUserOptions & XONLINE_USER_OPTION_REQUIRE_PASSCODE )
		{
			// OK. Display the passcode popup
			Menus_ActivateByName( "xbox_passcode" );
			return false;
		}

	}

	// Stage TWO: call XOnlineLogon, and check for system errors
	if( loginState <= LOGIN_CONNECT )
	{
		// Select which services we want to log into
		XBLservices[0] = XONLINE_MATCHMAKING_SERVICE;
		XBLservices[1] = XONLINE_FEEDBACK_SERVICE;

		// XOnlineLogon() allows a list of up to 4 players (1 per controller).
		// The list must be a one-to-one match of controller to user in order
		// for the online system to recognize which user is using which controller.
		ZeroMemory( &XBLLoggedOnUsers, sizeof( XBLLoggedOnUsers ) );

		// Put the user in the right port?
		XBLLoggedOnUsers[ IN_GetMainController() ] = XBLAccountusers[selectedAccountIndex];
		HRESULT result = XOnlineLogon( XBLLoggedOnUsers, XBLservices, SERVICE_COUNT, NULL, &logonHandle);

		// Check return value
		if( result == S_OK )
		{
			// OK. It worked. Carry on...
		}
		else if( result == XONLINE_E_LOGON_NO_NETWORK_CONNECTION )
		{
			// No connection. Show the popup.
			UI_xboxErrorPopup( XB_POPUP_CANNOT_CONNECT );
			return false;
		}
		else
		{
			// Should never happen
			assert( 0 );
		}

		//
		// We could break into two stages at this point, if we need to. Doesn't seem necessary
		//

		// Wait for the logon process to complete
		do
		{
			result = XOnlineTaskContinue( logonHandle );
		} 
		while( result == XONLINETASK_S_RUNNING );

		// OK. What happened?
		result = XOnlineLogonTaskGetResults( logonHandle );	
		switch( result )
		{
			case XONLINE_S_LOGON_CONNECTION_ESTABLISHED:
				// No system errors. Still need to check users and services below.
				break;

			case XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE:
				// Network troubleshooter popup
				UI_xboxErrorPopup( XB_POPUP_CANNOT_CONNECT );
				return false;

			case XONLINE_E_LOGON_CONNECTION_LOST:
				// Network troubleshooter popup
				UI_xboxErrorPopup( XB_POPUP_CANNOT_CONNECT );
				return false;

			case XONLINE_E_LOGON_INVALID_USER:
				// Account management popup
				UI_xboxErrorPopup( XB_POPUP_INVALID_USER );
				return false;

			case XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON:
				// Kicked by dupe popup
				UI_xboxErrorPopup( XB_POPUP_DUPLICATE_LOGON );
				return false;

			case XONLINE_E_LOGON_SERVERS_TOO_BUSY:
				// Busy servers popup
				UI_xboxErrorPopup( XB_POPUP_BUSY_SERVERS );
				return false;

			case XONLINE_E_LOGON_UPDATE_REQUIRED:
				// Update required popup
				UI_xboxErrorPopup( XB_POPUP_AUTOUPDATE );
				return false;

			default:
				assert( 0 );
				break;
		}
	}

	// Stage THREE: Call XOnlineGetLogonUsers(), and check for user errors
	if( loginState <= LOGIN_USER_ERRORS )
	{
		XONLINE_USER *updatedList = XOnlineGetLogonUsers();

		// Should never happen:
		assert( updatedList );

		// Copy the results
		memcpy(XBLLoggedOnUsers, updatedList, sizeof(XONLINE_USER) * XONLINE_MAX_LOGON_USERS);

		// Check for user problems
		for(int i = 0; i < XONLINE_MAX_LOGON_USERS; i++)
		{
			if( XBLLoggedOnUsers[i].xuid.qwUserID != 0 )
			{
				if( XBLLoggedOnUsers[i].hr == S_OK)
				{
					// No problem with this user. We only have one, so we could move on, but whatever...
					continue;
				}
				if( XBLLoggedOnUsers[i].hr == XONLINE_S_LOGON_USER_HAS_MESSAGE )
				{
					// Optional message popup - force UI to not finish login
					UI_xboxErrorPopup( XB_POPUP_OPTIONAL_MESSAGE );
					return false;
				}
				else if( XBLLoggedOnUsers[i].hr == XONLINE_E_LOGON_USER_ACCOUNT_REQUIRES_MANAGEMENT )
				{
					// Required management popup - force UI to not finish login
					UI_xboxErrorPopup( XB_POPUP_REQUIRED_MESSAGE );
					return false;
				}
				else
				{
					// This should never happen
					assert( 0 );
				}
			}
		}
	}

	// Stage FOUR: Check for service errors
	if( loginState <= LOGIN_SERVICE_ERRORS )
	{
		// VVFIXME - Need to check for these and handle them appropriately
		// Also, above code needs to check for user restrictions (voice banning)
	}

	// Stage FIVE: Finish logging in. Fire up other systems
	if( loginState <= LOGIN_FINISH )
	{
		logged_on = true;

		// We initialize voice above, in Net_XboxConnect(). But if we're logging
		// into live, we need to do it here, becuase we need to check for our voice
		// headset when we set initial presence info. Gar!
		g_Voice.Initialize();

		XBL_F_Init();
	
		//Set the player name to the Xbox live account name
		Q_strncpyz(ClientManager::ActiveClient().autoName, 
				XBLLoggedOnUsers[ IN_GetMainController() ].szGamertag,
				STRING_SIZE);
		Cvar_SetValue( "xbl_liveconnected", 1 );

		// OK. We're responsible for switching menus, too
		Menus_CloseAll();
		Menus_ActivateByName( "xbl_lobbymenu" );
	}

	return true;
}

// This is to cleanup logging in when an error is encountered
void XBL_AbortLogin(void)
{
	if(logonHandle)
	{
		XOnlineTaskClose(logonHandle);
		logonHandle = NULL;
	}
}

// shutdown xbox live functionality
//
HRESULT XBL_Cleanup()
{
	if(!logged_on)
		return S_OK;

    XBL_F_Cleanup();
	g_Voice.Shutdown();

	XBL_MM_Shutdown( false );

	if(logonHandle)
	{
		XOnlineTaskClose(logonHandle);
		logonHandle = NULL;
	}

	int result = XOnlineCleanup();
	if( result == S_OK )
	{
		logged_on = false;
		xbl_inited = false;
	}
	else
	{
		Com_Printf("XBLive - couldn't log off - failed to cleanup\n");
	}

	//friends stuff clean up
	Cvar_Set("ui_gameinvite", "");
	Cvar_Set("ui_friendinvite", "");
	Cvar_Set("xbl_friendnotifiation", "");

	return result;
}

// Simple wrapper around XNetGetEthernetLinkStatus()
bool Net_ConnectionStatus( void )
{
	return (bool)(XNetGetEthernetLinkStatus());
}

//  Set a CVAR flagging wether or not we currently have network connection
//
void XBL_Check_For_Net_Connection()
{
    static DWORD stateBefore = XNET_ETHERNET_LINK_ACTIVE;
    static int counter = CONNECTED_TICKS;

	// Check status every 20 frames (a little more than 1Hz)
	if(++counter > CONNECTED_TICKS)
	{
 		counter = 0;

        DWORD stateNow = XNetGetEthernetLinkStatus();
		Cvar_SetValue( "xb_netConnected", stateNow );

		// Did we just lose our connection? We need to take various actions depending on where we are:
		bool lostConnection = ((stateNow != stateBefore) && (stateNow == 0));
		if( !lostConnection )
		{
			stateBefore = stateNow;
			return;
		}

		// Grab the current menu (if there is one):
		menuDef_t *menu = Menu_GetFocused();
		// Grab our xb_gameType, we'll need it:
		int xbGT = Cvar_VariableIntegerValue( "xb_gameType" );

		if( menu && !Q_stricmp(menu->window.name, "main") )
		{
			// If we're on the main menu, and SystemLink is highlighted, move focus.
			// I need to do stupid tricks here, or the UI calls traps that fail!
			itemDef_t *item = Menu_GetFocusedItem( menu );
			if( item && !Q_stricmp(item->window.name, "linkButton") )
				VM_Call( uivm, UI_KEY_EVENT, A_CURSOR_UP, qtrue );	// Send a "move the cursor up"
		}
		else if( cls.state == CA_DISCONNECTED && xbGT == 2 )
		{
			// We're in the front-end, and we've chosen system link:
			UI_xboxErrorPopup( XB_POPUP_SYSTEM_LINK_LOST );
		}
		else if( cls.state == CA_DISCONNECTED && logged_on )
		{
			// We're in the front-end, and we've chosen Live! We wait until after login,
			// because the login screen is where we prompt people to go to the troubleshooter:
			UI_xboxErrorPopup( XB_POPUP_SYSTEM_LINK_LOST );
		}
		else if( cls.state > CA_DISCONNECTED && xbGT >= 2 )
		{
			// If we're playing (or started connecting) to SysLink or Live, bomb out:
			Com_Error( ERR_DROP, "@MENUS_SYSTEM_LINK_LOST" );
		}

        stateBefore = stateNow;
	}
}

bool XBL_PumpLogon( void )
{
	if(logonHandle)
	{
		// Pump logon
		HRESULT result = XOnlineTaskContinue( logonHandle );
		result = XOnlineLogonTaskGetResults( logonHandle );

#ifdef _DEBUG
		if ( FAILED( result ) )
			Com_Printf("TaskContinue(logonHandle) failed: %s\n", getXBLErrorName(result));
#endif

		if( result == XONLINE_S_LOGON_CONNECTION_ESTABLISHED || result == S_OK )
		{
			return true;
		}
		else if( result == XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON )
		{
			XBL_Cleanup();
			Com_Error( ERR_DROP, "@MENUS_XBOX_DUPLICATE_LOGON" );
//				UI_xboxErrorPopup( XB_POPUP_DUPLICATE_LOGON );
			return false;
		}
		else
		{
			XBL_Cleanup();
			Com_Error( ERR_DROP, "@MENUS_XBOX_LOST_CONNECTION" );
//				UI_xboxErrorPopup( XB_POPUP_LOST_CONNECTION );
			return false;
		}
	}

	return true;
}

void UI_HandleXBLogonError(const int result);
// run once every game cycle
//
void XBL_Tick()
{
    // check if this xbox has a network connection
    //
	XBL_Check_For_Net_Connection();

	if( logged_on )
	{
		if( !XBL_PumpLogon() )
			return;

		// tick friends
		//
		XBL_F_Tick();	

		// tick matchmaking
		//
		XBL_MM_Tick();

	}

	// tick player list - this needs to happen in system link too!
	//
	XBL_PL_Tick();

	// Tick Voice - do this elsewhere?
	g_Voice.Tick();

	// If we're currently listening for system link games, throw out any
	// that we haven't heard from in some time:
	if( sysLink.listening )
	{
		for( int i = 0; i < MAX_OTHER_SERVERS; ++i )
		{
			if( cls.localServers[i].lastUpdate &&
				cls.localServers[i].lastUpdate + 8000 < Sys_Milliseconds() )
			{
				cls.localServers[i].lastUpdate = 0;
				cls.numlocalservers--;
			}
		}
	}
}


//----------------------------------------------------------------------------
// Name: XBL_CheckServer()
// Desc: checks server we are trying to connect to
//----------------------------------------------------------------------------
void XBL_CheckServer()
{
	// VVFIXME - Does this work? At all? What the hell?

	//don't test if in sys link OR server OR not considered playing yet
	if(!logged_on || com_sv_running->integer ||!XBL_F_GetState(XONLINE_FRIENDSTATE_FLAG_PLAYING))
		return;
	static int timer = 0;
	if(timer < Sys_Milliseconds())
	{
		timer = Sys_Milliseconds() + 4000;
		if(!XBL_MM_IsSessionIDValid(Net_GetXNKID()))
	       Com_Error( ERR_DROP, SE_GetString("XBL_GAME_SESSION_NOT_AVAIL") );
	}
}

//-----------------------------------------------------------------------------
// Xbox Error Popup support stuff
//-----------------------------------------------------------------------------

// What popup is active at the moment?
static xbErrorPopupType sPopup = XB_POPUP_NONE;

// Creates the requested popup, then displays it.
// Establishes context so proper action will be taken on a selection.
void UI_xboxErrorPopup(xbErrorPopupType popup)
{
	// Set our context
	sPopup = popup;

	Cvar_Set( "xb_PopupTitle", "" );

	// Set the menu cvars
	switch( popup )
	{
		case XB_POPUP_CANNOT_CONNECT:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_CANNOT_CONNECT" );
			break;
		case XB_POPUP_AUTOUPDATE:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_REQUIRED_UPDATE" );
			break;
		case XB_POPUP_REQUIRED_MESSAGE:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_REQUIRED_MESSAGE" );
			break;
		case XB_POPUP_OPTIONAL_MESSAGE:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_OPTIONAL_MESSAGE" );
			break;
		case XB_POPUP_LOST_CONNECTION:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_LOST_CONNECTION" );
			break;
		case XB_POPUP_INVALID_USER:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_ACCOUNT_NOT_CURRENT" );
			break;
		case XB_POPUP_DUPLICATE_LOGON:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_DUPLICATE_LOGON" );
			break;
		case XB_POPUP_BUSY_SERVERS:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_LIVE_BUSY" );
			break;
		case XB_POPUP_CONFIRM_NEW_ACCOUNT:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_NEW_ACCOUNT_CONFIRM" );
			break;
		case XB_POPUP_CONFIRM_LOGOFF:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_SIGN_OUT" );
			break;
		case XB_POPUP_COM_ERROR:
			Cvar_Set( "xb_errMessage", Cvar_VariableString( "com_errorMessage" ) );
			break;
		case XB_POPUP_QUICKMATCH_NO_RESULTS:
		case XB_POPUP_OPTIMATCH_NO_RESULTS:
			// Special case. These don't use the normal popup.
			sPopup = XB_POPUP_NONE;
			Menus_ActivateByName( "no_servers_popup" );
			return;	// <- Not a break!
		case XB_POPUP_MATCHMAKING_ERROR:
			Cvar_Set( "xb_errMessage", "@MENUS_XBOX_MATCHMAKING_ERROR" );
			break;
		case XB_POPUP_SYSTEM_LINK_LOST:
			Cvar_Set( "xb_errMessage", "@MENUS_SYSTEM_LINK_LOST" );
			break;
		case XB_POPUP_QUIT_CONFIRM:
			Cvar_Set( "xb_errMessage", "@MENUS_QUIT_CURRENT_HOSTED_GAME_STATUS_LOST");
			Cvar_Set( "xb_PopupTitle",  "@MENUS_QUIT" );
			break;
		case XB_POPUP_QUIT_HOST_CONFIRM:
			Cvar_Set( "xb_errMessage", "@MENUS_QUIT_CURRENT_HOSTED_GAME_AND");
			Cvar_Set( "xb_PopupTitle",  "@MENUS_QUIT" );
			break;
		case XB_POPUP_HOST_JOIN_CONFIRM:
			// Message here depends on whether or not we're the server:
			if( com_sv_running->integer )
				Cvar_Set( "xb_errMessage", "@MENUS_HOST_JOINING_OTHER" );
			else
				Cvar_Set( "xb_errMessage", "@MENUS_CLIENT_JOINING_OTHER" );
			break;
		case XB_POPUP_CONFIRM_FRIEND_REMOVE:
			Cvar_Set( "xb_errMessage", va( SE_GetString( "MENUS_CONFIRM_FRIEND_REMOVE" ), Cvar_VariableString( "fl_selectedName" ) ) );
			break;
		case XB_POPUP_RESPAWN_NEEDED:
			Cvar_Set( "xb_errMessage" , "@MENUS_RESPAWN_NEEDED");
			break;
		case XB_POPUP_CORRUPT_SETTINGS:
			Cvar_Set( "xb_errMessage", "@MENUS_CORRUPT_SETTINGS" );
			break;

		default:
			Com_Error( ERR_FATAL, "ERROR: Invalid popup type %i\n", popup );
	}

	// Display the menu
	Menus_ActivateByName( "xbox_error_popup" );
}

// Accepts a response to the currently dislpayed popup.
// Does whatever is necessary based on which popup was visible, and what the response was.
void UI_xboxPopupResponse( void )
{
	if( sPopup == XB_POPUP_NONE )
		Com_Error( ERR_FATAL, "ERROR: Got a popup response with no valid context\n" );

	int response = Cvar_VariableIntegerValue( "xb_errResponse" );

	extern void Sys_Reboot( const char *reason );

	// Remove remnant kruft:
	if( response == 2 ) // && sPopup != XB_POPUP_OPTIONAL_MESSAGE )
	{
		// Do nothing.
		return;
	}

	switch( sPopup )
	{
		case XB_POPUP_CANNOT_CONNECT:
			if( response == 0 )			// A
			{
				// Won't return
				Sys_Reboot( "net_config" );
			}
			else if( response == 1 )	// B
			{
				XBL_AbortLogin();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
				return;
			}
			break;

		case XB_POPUP_AUTOUPDATE:
			if( response == 0 )			// A
			{
				// Won't return
				XOnlineTitleUpdate( 0 );
			}
			else						// B
			{
				XBL_AbortLogin();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			break;

		case XB_POPUP_REQUIRED_MESSAGE:
			if( response == 0 )			// A
			{
				// Won't return
				Sys_Reboot( "manage_account" );
			}
			else						// B
			{
				XBL_AbortLogin();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			break;

		case XB_POPUP_OPTIONAL_MESSAGE:
			if( response == 0 )			// A
			{
				// Won't return
				Sys_Reboot( "manage_account" );
			}
			else if( response == 1 )	// B
			{
				// Resume logging in - special case as we can re-trigger a popup:
				Menus_CloseByName( "xbox_error_popup" );
				sPopup = XB_POPUP_NONE;
				XBL_Login( LOGIN_SERVICE_ERRORS );
				return;
			}
			break;

		case XB_POPUP_LOST_CONNECTION:
			if( response == 0 )			// A
			{
				XBL_Cleanup();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			else if( response == 1 )	// B
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_INVALID_USER:
			if( response == 0 )			// A
			{
				// Won't return
				Sys_Reboot( "manage_account" );
			}
			else						// B
			{
				XBL_AbortLogin();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			break;

		case XB_POPUP_DUPLICATE_LOGON:
			if( response == 0 )			// A
			{
				XBL_Cleanup();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			else if( response == 1 )	// B
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_BUSY_SERVERS:
			if( response == 0 )			// A
			{
				// Try again - special case because we could re-trigger a popup
				XBL_AbortLogin();
				Menus_CloseByName( "xbox_error_popup" );
				sPopup = XB_POPUP_NONE;
				XBL_Login( LOGIN_CONNECT );
				return;
			}
			else						// B
			{
				XBL_AbortLogin();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			break;

		case XB_POPUP_CONFIRM_NEW_ACCOUNT:
			if( response == 0 )			// A
			{
				// Won't return
				Sys_Reboot( "new_account" );
			}
			else						// B
			{
				// Close popup, leave user on account screen:
				Menus_CloseByName( "xbox_error_popup" );
			}
			break;

		case XB_POPUP_CONFIRM_LOGOFF:
			if( response == 0)			// A
			{
				// User wants to sign out
				XBL_Cleanup();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			else						// B
			{
				// The popup is gone, make sure that we're back on the lobby
				Menus_CloseAll();
				Menus_ActivateByName( "xbl_lobbymenu" );
			}
			break;
	
		case XB_POPUP_COM_ERROR:
			if( response == 0 )			// A
			{
				//XBL_Cleanup();
				Menus_CloseAll();
				if( logged_on )
					Menus_ActivateByName( "xbl_lobbymenu" );
				else
					Menus_ActivateByName( "main" );
			}
			else if( response == 1 )	// B
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_QUICKMATCH_NO_RESULTS:
		case XB_POPUP_OPTIMATCH_NO_RESULTS:
			// These don't use the normal popup menu
			assert( 0 );
			break;

		case XB_POPUP_MATCHMAKING_ERROR:
			if( response == 0 )			// A
			{
				// Some kind of fatal error with the quickmatch or optimatch
				// Force the user back to the lobby
				Menus_CloseAll();
				Menus_ActivateByName( "xbl_lobbymenu" );
			}
			else
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_SYSTEM_LINK_LOST:
			if( response == 0 )			// A
			{
				XBL_Cleanup();
				Menus_CloseAll();
				Menus_ActivateByName( "main" );
			}
			else if( response == 1 )	// B
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_QUIT_CONFIRM:
			if( response == 0 )			// A continue
			{
				itemDef_t it;
				it.parent = Menu_GetFocused();
				Item_RunScript(&it, "uiScript Leave");
//				Cbuf_ExecuteText( EXEC_APPEND, "disconnect\n" );
//				trap_Key_SetCatcher( KEYCATCH_UI );
//				Menus_CloseAll();
				return;
			}
			else
			{
				Menus_CloseByName( "xbox_error_popup" );
			}
			break;

		case XB_POPUP_QUIT_HOST_CONFIRM:
			if( response == 0 )			// A continue
			{	
				itemDef_t it;
				it.parent = Menu_GetFocused();
				Item_RunScript(&it, "uiScript Leave");
			//	Cbuf_ExecuteText( EXEC_APPEND, "disconnect\n" );
			//	trap_Key_SetCatcher( KEYCATCH_UI );
			//	Menus_CloseAll();
				return;
			}
			else
			{
				Menus_CloseByName( "xbox_error_popup" );
			}
			break;

		case XB_POPUP_HOST_JOIN_CONFIRM:
			if( response == 0 )			// A continue
			{
				// User wants to join. Now we check for lag:
				XONLINE_FRIEND* curFriend = XBL_F_GetChosenFriend();

				if( curFriend && XBL_MM_ThisSessionIsLagging( &curFriend->sessionID ) )
				{
					Menus_CloseByName( "xbox_error_popup" );
					Menus_ActivateByName( "slow_server_popup" );
				}
				else
				{
					// No lag. This function is designed for the case when the
					// user agrees to join a low-ping server, but it does what we want:
					Menus_CloseByName( "xbox_error_popup" );
					XBL_MM_JoinLowQOSSever();
				}
			}
			else
			{
				// User decided not to continue joining
				XBL_MM_DontJoinLowQOSSever();
				Menus_CloseByName( "xbox_error_popup" );
			}
			break;

		case XB_POPUP_CONFIRM_FRIEND_REMOVE:
			if( response == 0 )			// A confirm
			{
				// Remove them from the friends list
				XBL_F_PerformMenuAction(UI_F_FRIENDREMOVE);
				Menus_CloseByName( "xbox_error_popup" );
			}
			else
			{
				// Do nothing
				Menus_CloseByName( "xbox_error_popup" );
			}
			break;

		case XB_POPUP_RESPAWN_NEEDED:
			if( response == 0 )			// A
			{
				Menus_CloseAll();
				Menus_OpenByName("ingame");
			}
			else if( response == 1 )	// B
			{
				// Invalid response!
				return;
			}
			break;

		case XB_POPUP_CORRUPT_SETTINGS:
			if( response == 0 )			// A
			{
				Settings.Delete();
				Menus_CloseByName( "xbox_error_popup" );
				extern void XB_Startup( XBStartupState startupState );
				XB_Startup( STARTUP_COMBINED_SPACE_CHECK );
				return;
			}
			else
			{
				// Invalid response!
				return;
			}
			break;

		default:
			Com_Error( ERR_FATAL, "ERROR: Invalid popup type %i\n", sPopup );
	}

	// If we get here, the user gave a valid response to our popup. Clear the context:
	sPopup = XB_POPUP_NONE;
}
