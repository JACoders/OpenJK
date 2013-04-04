// net_wins.c
//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "win_local.h"
#include "../xbox/XBLive.h"

static WSADATA	winsockdata;
static qboolean	winsockInitialized = qfalse;
static qboolean networkingEnabled = qfalse;

static cvar_t	*net_noudp;

static SOCKET	v_socket = INVALID_SOCKET;
static SOCKET	ip_socket = INVALID_SOCKET;
static SOCKET	ip_broadcast_socket = INVALID_SOCKET;

#define	MAX_IPS		16
static	int		numIP;
static	byte	localIP[MAX_IPS][4];

byte	broadcast_nonce[8];	//Used to uniquely identify broadcast messages bound for this server

//=============================================================================


/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void ) {
	int		code;

	code = WSAGetLastError();
	switch( code ) {
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	case WSAEHOSTUNREACH: return "WSAEHOSTUNREACH";
	default: return "NO ERROR";
	}
}

void NetadrToSockadr( netadr_t *a, struct sockaddr *s ) {
	memset( s, 0, sizeof(*s) );

	if( a->type == NA_BROADCAST ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
}


void SockadrToNetadr( struct sockaddr *s, netadr_t *a ) {
	if (s->sa_family == AF_INET) {
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
}


/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
12121212.121212121212
=============
*/
#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	\
	((struct sockaddr_ipx *)sadr)->dest = val

qboolean Sys_StringToSockaddr( const char *s, struct sockaddr *sadr ) {
	
	memset( sadr, 0, sizeof( *sadr ) );

	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	if( s[0] >= '0' && s[0] <= '9' ) {
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	} else {
		return qfalse;
	}
	
	return qtrue;
}

#undef DO

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a ) {
	struct sockaddr sadr;
	
	if ( !Sys_StringToSockaddr( s, &sadr ) ) {
		return qfalse;
	}
	
	SockadrToNetadr( &sadr, a );
	return qtrue;
}

//=============================================================================

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message )
{
	SOCKET	net_socket = ip_socket;

	struct sockaddr from;
	int		fromlen = sizeof(from);

	int ret = recvfrom( net_socket, (char *)net_message->data, net_message->maxsize, 0, &from, &fromlen );

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		if( err == WSAEWOULDBLOCK || err == WSAECONNRESET ) {
			return qfalse;
		}

		Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
		return qfalse;
	}

	memset( ((struct sockaddr_in *)&from)->sin_zero, 0, 8 );
	SockadrToNetadr( &from, net_from );

	net_message->readcount = 0;

	if( ret == net_message->maxsize ) {
		Com_Printf( "Oversize packet from %s\n", NET_AdrToString (*net_from) );
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;

}


qboolean Sys_GetBroadcastPacket( msg_t *net_message ) 
{
	SOCKET net_socket = ip_broadcast_socket;

	struct sockaddr from;
	int		fromlen = sizeof(from);

	int ret = recvfrom( net_socket, (char*)net_message->data, net_message->maxsize, 0, &from, &fromlen );

	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		if( err == WSAEWOULDBLOCK ) {//|| err == WSAECONNRESET ) {
			return qfalse;
		}

		Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
		return qfalse;
	}

	net_message->readcount = 0;
	
	if( ret == net_message->maxsize ) {
		Com_Printf("Oversized broadcast packet\n");
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

//=============================================================================

/*
==================
Sys_SendVoicePacket
==================
*/
void Sys_SendVoicePacket( int length, const void *data, netadr_t to ) {
	int				ret;
	struct sockaddr	addr;

	// check for valid packet intentions (direct send only!)
	if( to.type != NA_IP ) {
		Com_Error( ERR_FATAL, "Sys_SendVoicePacket: bad address type" );
		return;
	}

	// check we have our voice socket set up
	if( v_socket == INVALID_SOCKET ) {
		return;
	}

	NetadrToSockadr( &to, &addr );
	ret = sendto( v_socket, (const char *)data, length, 0, &addr, sizeof(addr) );

	if( ret == SOCKET_ERROR ) {
		int err = WSAGetLastError();

		// wouldblock is silent
		if( err == WSAEWOULDBLOCK ) {
			return;
		}

		Com_DPrintf( "NET_SendVoicePacket: %s\n", NET_ErrorString() );
	}
}

/*
==================
Sys_SendPacket
==================
*/
bool Sys_SendPacket( int length, const void *data, netadr_t to ) {
	int				ret;
	struct sockaddr	addr;
	SOCKET			net_socket;

	if( to.type == NA_BROADCAST ) {
		net_socket = ip_broadcast_socket;
	}
	else if( to.type == NA_IP ) {
		net_socket = ip_socket;
	}
	else {
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return false;
	}

	if( net_socket == INVALID_SOCKET) {
		return false;
	}

	NetadrToSockadr( &to, &addr );

	ret = sendto( net_socket, (const char *)data, length, 0, &addr, sizeof(addr) );

	if( ret == SOCKET_ERROR ) {
		int err = WSAGetLastError();

		// wouldblock is silent
		if( err == WSAEWOULDBLOCK ) {
			return false;
		}

		// some PPP links do not allow broadcasts and return an error
//		if( (err == WSAEADDRNOTAVAIL) && ( to.type == NA_BROADCAST )  ) {
//			return;
//		}

#ifndef FINAL_BUILD
		Com_Printf( "NET_SendPacket: %s\n", NET_ErrorString() );
#endif
		return false;
	}

	return true;
}

/*
==================
Sys_SendBroadcastPacket
==================
*/
void Sys_SendBroadcastPacket( int length, const void *data ) 
{
	// Setup the broadcast address
	netadr_t to;
	Com_Memset( &to, 0, sizeof( to ) );
	to.port = BigShort( (short)(PORT_BROADCAST) );
	to.type = NA_BROADCAST;

	Sys_SendPacket(length, data, to);
}




//=============================================================================


/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean Sys_IsLANAddress( netadr_t adr ) {
	int		i;

	if( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if( adr.type != NA_IP ) {
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	if( adr.ip[0] == 127 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 1 ) {
		return qtrue;
	}

	if ( (adr.ip[0] == 192 && adr.ip[1] == 168) || 
		 (adr.ip[0] == 10 && adr.ip[1] == 100)  ||
		 (adr.ip[0] == 172 && adr.ip[1] == 16)      )
	{
		return qtrue;
	} 

	/*
	// Class A
	if( (adr.ip[0] & 0x80) == 0x00 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] ) {
				return qtrue;
			}
		}
		// the RFC1918 class a block will pass the above test
		return qfalse;
	}

	// Class B
	if( (adr.ip[0] & 0xc0) == 0x80 ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] ) {
				return qtrue;
			}
			// also check against the RFC1918 class b blocks
			if( adr.ip[0] == 172 && localIP[i][0] == 172 && (adr.ip[1] & 0xf0) == 16 && (localIP[i][1] & 0xf0) == 16 ) {
				return qtrue;
			}
		}
		return qfalse;
	}
	*/
	//we only look at class C since ISPs and Universities are using class A but we don't want to consider them on the same LAN.

	// Class C
	for ( i = 0 ; i < numIP ; i++ ) {
		if( adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2] ) {
			return qtrue;
		}
		
		//check for both on a local lan type thing
		if( adr.ip[0] == 10 && localIP[i][0] == 10 ) 
		{
			return qtrue;
		}

		// also check against the RFC1918 class c blocks
//		if( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
//			return qtrue;
//		}
	}
	return qfalse;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP(void) {
	int i;

	for (i = 0; i < numIP; i++) {
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
	}
}


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket( char *net_interface, int port, bool broadcast ) {
	SOCKET				newsocket;
	struct sockaddr_in	address;
	qboolean			_true = qtrue;
	int					i = 1;
	int					err;

	if( net_interface ) {
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else {
		Com_Printf( "Opening IP socket: localhost:%i\n", port );
	}

	if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT ) {
			Com_Printf( "WARNING: UDP_OpenSocket: socket: %s\n", NET_ErrorString() );
		}
		return INVALID_SOCKET;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, (unsigned long *)&_true ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		return INVALID_SOCKET;
	}

	// make it broadcast capable - but only if requested. we don't want our main socket to get bogged down:
	if (!broadcast)
		i = 0;
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
		return INVALID_SOCKET;
	}
	i = 1;

	if( !net_interface || !net_interface[0] || !Q_stricmp(net_interface, "localhost") ) {
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address );
	}

	if( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( (short)port );
	}

	address.sin_family = AF_INET;

	if( bind( newsocket, (const struct sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: UDP_OpenSocket: bind: %s\n", NET_ErrorString() );
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	return newsocket;
}



/*
=====================
NET_GetLocalAddress
=====================
*/
// Xbox version supports the force option, so we can prime the
// system and hopefully be getting an IP while Com_Init() is running.
void NET_GetLocalAddress( bool force )
{
	XNADDR xnMyAddr;
	DWORD dwStatus;
	do
	{
	   // Repeat while pending; OK to do other work in this loop
	   dwStatus = XNetGetTitleXnAddr( &xnMyAddr );
	} while( dwStatus == XNET_GET_XNADDR_PENDING && force );

	// Error checking
	if( dwStatus == XNET_GET_XNADDR_NONE )
	{
		// If this wasn't the final (necessary) call, then don't worry
		if( force )
			assert(!"Error getting XBox title address.");
		return;
	}

	*(u_long*)&localIP[0] = xnMyAddr.ina.S_un.S_addr;
	*(u_long*)localIP[1] = 0;
	*(u_long*)localIP[2] = 0;
	*(u_long*)localIP[3] = 0;

	Com_Printf( "IP: %i.%i.%i.%i\n", localIP[0], localIP[1], localIP[2], localIP[3] );
}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void ) {
	cvar_t	*ip;
	int		port;
	int		i;

	ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );

	// Don't do the silly quake thing where we keep trying new port numbers.
	// We can't run more than one server per xbox. And make sure that our ip socket
	// is NOT broadcast capable, so that the game logic doesn't get confused.
	ip_socket = NET_IPSocket( ip->string, PORT_SERVER, false );
	if ( ip_socket ) {
		Cvar_SetValue( "net_port", PORT_SERVER );
		NET_GetLocalAddress( false );
		return;
	}
	
	Com_Printf( "WARNING: Couldn't allocate IP port\n");
}

void NET_OpenBroadcastIP()
{
	// This s the only socket that will be used for broadcast
	// (session discovery on system link)
	ip_broadcast_socket = NET_IPSocket(NULL, PORT_BROADCAST, true);
}


//===================================================================


/*
====================
NET_GetCvars
====================
*/
static qboolean NET_GetCvars( void ) {
	qboolean	modified;

	modified = qfalse;

	if( net_noudp && net_noudp->modified ) {
		modified = qtrue;
	}
	net_noudp = Cvar_Get( "net_noudp", "0", CVAR_LATCH | CVAR_ARCHIVE );

	return modified;
}


/*
====================
NET_Config
====================
*/
void NET_Config( qboolean enableNetworking ) {
	qboolean	modified;
	qboolean	stop;
	qboolean	start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if( net_noudp->integer ) {
		enableNetworking = qfalse;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if( enableNetworking == networkingEnabled && !modified ) {
		return;
	}

	if( enableNetworking == networkingEnabled ) {
		if( enableNetworking ) {
			stop = qtrue;
			start = qtrue;
		}
		else {
			stop = qfalse;
			start = qfalse;
		}
	}
	else {
		if( enableNetworking ) {
			stop = qfalse;
			start = qtrue;
		}
		else {
			stop = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if( stop ) {
		if ( ip_socket && ip_socket != INVALID_SOCKET ) {
			closesocket( ip_socket );
			ip_socket = INVALID_SOCKET;
		}
		if ( ip_broadcast_socket && ip_broadcast_socket != INVALID_SOCKET ) {
			closesocket( ip_broadcast_socket );
			ip_broadcast_socket = INVALID_SOCKET;
		}
	}

	if( start ) {
		if (! net_noudp->integer ) {
			NET_OpenIP();
			NET_OpenBroadcastIP();
		}
	}
}


/*
====================
NET_Init
====================
*/
void NET_Init( void ) {

	// Run NetStartup with security bypassed
	// this allows us to communicate with PCs while developing
	XNetStartupParams xnsp;
    ZeroMemory( &xnsp, sizeof(xnsp) );
    xnsp.cfgSizeOfStruct = sizeof(xnsp);

#ifndef FINAL_BUILD
	xnsp.cfgFlags |= XNET_STARTUP_BYPASS_SECURITY;
#endif

	INT err = XNetStartup( &xnsp );
	if( err != NO_ERROR ) {
		Com_Printf( "ERROR: XNetStartup Failed: %d\n", err );	
        return;
	}

	int r = WSAStartup( MAKEWORD(2,2), &winsockdata );
	if( r ) {
		Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}

	//Initialize the broadcast nonce
	XNetRandom( broadcast_nonce, sizeof(broadcast_nonce) );

	winsockInitialized = qtrue;
	Com_Printf( "Winsock Initialized\n" );

	// this is really just to get the cvars registered
	NET_GetCvars();

	NET_Config( qtrue );
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void ) {
	if ( !winsockInitialized ) {
		return;
	}

	NET_Config( qfalse );
	WSACleanup();
	winsockInitialized = qfalse;
}


/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
void NET_Sleep( int msec ) {
}


/*
====================
NET_Restart_f
====================
*/
void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}
