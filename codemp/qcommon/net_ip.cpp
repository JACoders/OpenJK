//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#ifdef _WIN32
	#include <winsock.h>

	typedef int socklen_t;

	#undef EAGAIN
	#undef EADDRNOTAVAIL
	#undef EAFNOSUPPORT
	#undef ECONNRESET

	#define EAGAIN WSAEWOULDBLOCK
	#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
	#define EAFNOSUPPORT WSAEAFNOSUPPORT
	#define ECONNRESET WSAECONNRESET

	#define socketError WSAGetLastError( )

	static WSADATA	winsockdata;
	static qboolean	winsockInitialized = qfalse;
#else

#if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
        // needed for socklen_t on OSX 10.2
#        define _BSD_SOCKLEN_T_
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef MACOS_X
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>         // for 'struct sockaddr_dl'
#endif

#ifdef __sun
#include <sys/filio.h>
#endif

typedef int SOCKET;
#define INVALID_SOCKET                -1
#define SOCKET_ERROR                        -1
#define closesocket                                close
#define ioctlsocket                                ioctl
#define socketError                                errno

#endif

static qboolean usingSocks = qfalse;
static qboolean networkingEnabled = qfalse;

static cvar_t	*net_enabled;
static cvar_t	*net_forcenonlocal;

static cvar_t	*net_socksEnabled;
static cvar_t	*net_socksServer;
static cvar_t	*net_socksPort;
static cvar_t	*net_socksUsername;
static cvar_t	*net_socksPassword;

static cvar_t	*net_ip;
static cvar_t	*net_port;

static struct sockaddr	socksRelayAddr;

static SOCKET	ip_socket = INVALID_SOCKET;
static SOCKET	socks_socket = INVALID_SOCKET;

#define	MAX_IPS		16
static	int		numIP;
static	byte	localIP[MAX_IPS][4];

//=============================================================================

/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void ) {
#ifdef _WIN32
	switch( socketError ) {
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
#else
	return strerror ( socketError );
#endif
}

static void NetadrToSockadr( netadr_t *a, struct sockaddr *s ) {
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

static void SockadrToNetadr( struct sockaddr *s, netadr_t *a ) {
	if (s->sa_family == AF_INET) {
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
}

/*
=============
Sys_StringToSockaddr
=============
*/
static qboolean Sys_StringToSockaddr( const char *s, struct sockaddr *sadr )
{
	struct hostent	*h;

	memset( sadr, 0, sizeof( *sadr ) );

	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	if( s[0] >= '0' && s[0] <= '9' )
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else
	{
		if( ( h = gethostbyname( s ) ) == 0 )
			return qfalse;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}

	return qtrue;
}

/*
=============
Sys_StringToAdr
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
#ifdef _DEBUG
int	recvfromCount;
#endif

qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message ) {
	int ret, err;
	socklen_t fromlen;
	struct sockaddr from;

	if ( ip_socket == INVALID_SOCKET ) {
		return qfalse;
	}

	fromlen = sizeof( from );
#ifdef _DEBUG
	recvfromCount++;		// performance check
#endif
	ret = recvfrom( ip_socket, (char *)net_message->data, net_message->maxsize, 0, (struct sockaddr *)&from, &fromlen );

	if ( ret == SOCKET_ERROR ) {
		err = socketError;

		if( err == EAGAIN || err == ECONNRESET )
			return qfalse;

		Com_Printf( "NET_GetPacket: %s\n", NET_ErrorString() );
		return qfalse;
	}

	memset( ((struct sockaddr_in *)&from)->sin_zero, 0, 8 );

	if ( usingSocks && memcmp( &from, &socksRelayAddr, fromlen ) == 0 ) {
		if ( ret < 10 || net_message->data[0] != 0 || net_message->data[1] != 0 || net_message->data[2] != 0 || net_message->data[3] != 1 ) {
			return qfalse;
		}
		net_from->type = NA_IP;
		net_from->ip[0] = net_message->data[4];
		net_from->ip[1] = net_message->data[5];
		net_from->ip[2] = net_message->data[6];
		net_from->ip[3] = net_message->data[7];
		net_from->port = *(short *)&net_message->data[8];
		net_message->readcount = 10;
	}
	else {
		SockadrToNetadr( &from, net_from );
		net_message->readcount = 0;
	}

	if( ret >= net_message->maxsize ) {
		Com_Printf( "Oversize packet from %s\n", NET_AdrToString (*net_from) );
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

//=============================================================================

static char socksBuf[4096];

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, const void *data, netadr_t to ) {
	int				ret;
	struct sockaddr	addr;

	if ( to.type != NA_BROADCAST && to.type != NA_IP ) {
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return;
	}

	if ( ip_socket == INVALID_SOCKET ) {
		return;
	}

	NetadrToSockadr( &to, &addr );

	if( usingSocks && to.type == NA_IP ) {
		socksBuf[0] = 0;	// reserved
		socksBuf[1] = 0;
		socksBuf[2] = 0;	// fragment (not fragmented)
		socksBuf[3] = 1;	// address type: IPV4
		*(int *)&socksBuf[4] = ((struct sockaddr_in *)&addr)->sin_addr.s_addr;
		*(short *)&socksBuf[8] = ((struct sockaddr_in *)&addr)->sin_port;
		memcpy( &socksBuf[10], data, length );
		ret = sendto( ip_socket, socksBuf, length+10, 0, &socksRelayAddr, sizeof(socksRelayAddr) );
	}
	else {
		ret = sendto( ip_socket, (const char *)data, length, 0, &addr, sizeof(addr) );
	}
	if( ret == SOCKET_ERROR ) {
		int err = socketError;

		// wouldblock is silent
		if( err == EAGAIN ) {
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if( err == EADDRNOTAVAIL && to.type == NA_BROADCAST ) {
			return;
		}

		Com_Printf( "NET_SendPacket: %s\n", NET_ErrorString() );
	}
}

//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean Sys_IsLANAddress( netadr_t adr ) {
	if ( !net_forcenonlocal )
		net_forcenonlocal = Cvar_Get( "net_forcenonlocal", "0", 0 );

	if ( net_forcenonlocal && net_forcenonlocal->integer )
		return qfalse;

	if( adr.type == NA_LOOPBACK )
		return qtrue;

	if( adr.type != NA_IP )
		return qfalse;

	// RFC1918:
	// 10.0.0.0        -   10.255.255.255  (10/8 prefix)
	// 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
	// 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
	if ( adr.ip[0] == 10 )
		return qtrue;
	if ( adr.ip[0] == 172 && (adr.ip[1]&0xf0) == 16 )
		return qtrue;
	if ( adr.ip[0] == 192 && adr.ip[1] == 168 )
		return qtrue;

	if ( adr.ip[0] == 127 )
		return qtrue;

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte

	// Class C
	for ( int i=0; i<numIP; i++ ) {
		if( memcmp( adr.ip, localIP[i], 3 ) == 0 )
			return qtrue;
	}
	return qfalse;
}

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP(void) {
	for ( int i=0; i<numIP; i++ )
		Com_Printf( "IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3] );
}

//=============================================================================

/*
====================
NET_IPSocket
====================
*/
SOCKET NET_IPSocket( char *net_interface, int port, int *err ) {
	SOCKET				newsocket;
	struct sockaddr_in	address;
	u_long				_true = 1;
	int					i = 1;

	*err = 0;

	if( net_interface ) {
		Com_Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	}
	else {
		Com_Printf( "Opening IP socket: localhost:%i\n", port );
	}

	if( ( newsocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == INVALID_SOCKET ) {
		*err = socketError;
		Com_Printf( "WARNING: NET_IPSocket: socket: %s\n", NET_ErrorString() );
		return newsocket;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IPSocket: ioctl FIONBIO: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	// make it broadcast capable
	if( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IPSocket: setsockopt SO_BROADCAST: %s\n", NET_ErrorString() );
	}

	if( !net_interface || !net_interface[0] || !Q_stricmp(net_interface, "localhost") ) {
		memset( &address, 0, sizeof( address ) );
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		if ( !Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address ) ) {
			closesocket( newsocket );
			return INVALID_SOCKET;
		}
	}

	if( port == PORT_ANY ) {
		address.sin_port = 0;
	}
	else {
		address.sin_port = htons( (short)port );
	}

	if( bind( newsocket, (const struct sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_Printf( "WARNING: NET_IPSocket: bind: %s\n", NET_ErrorString() );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}

	return newsocket;
}

/*
====================
NET_OpenSocks
====================
*/
void NET_OpenSocks( int port ) {
	struct sockaddr_in	address;
	int					err;
	struct hostent		*h;
	int					len;
	qboolean			rfc1929;
	unsigned char		buf[64];

	usingSocks = qfalse;

	Com_Printf( "Opening connection to SOCKS server.\n" );

	if ( ( socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) {
		err = socketError;
		Com_Printf( "WARNING: NET_OpenSocks: socket: %s\n", NET_ErrorString() );
		return;
	}

	h = gethostbyname( net_socksServer->string );
	if ( h == NULL ) {
		err = socketError;
		Com_Printf( "WARNING: NET_OpenSocks: gethostbyname: %s\n", NET_ErrorString() );
		return;
	}
	if ( h->h_addrtype != AF_INET ) {
		Com_Printf( "WARNING: NET_OpenSocks: gethostbyname: address type was not AF_INET\n" );
		return;
	}
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *(int *)h->h_addr_list[0];
	address.sin_port = htons( (short)net_socksPort->integer );

	if ( connect( socks_socket, (struct sockaddr *)&address, sizeof( address ) ) == SOCKET_ERROR ) {
		err = socketError;
		Com_Printf( "NET_OpenSocks: connect: %s\n", NET_ErrorString() );
		return;
	}

	// send socks authentication handshake
	if ( *net_socksUsername->string || *net_socksPassword->string ) {
		rfc1929 = qtrue;
	}
	else {
		rfc1929 = qfalse;
	}

	buf[0] = 5;		// SOCKS version
	// method count
	if ( rfc1929 ) {
		buf[1] = 2;
		len = 4;
	}
	else {
		buf[1] = 1;
		len = 3;
	}
	buf[2] = 0;		// method #1 - method id #00: no authentication
	if ( rfc1929 ) {
		buf[2] = 2;		// method #2 - method id #02: username/password
	}
	if ( send( socks_socket, (const char *)buf, len, 0 ) == SOCKET_ERROR ) {
		err = socketError;
		Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, (char *)buf, 64, 0 );
	if ( len == SOCKET_ERROR ) {
		err = socketError;
		Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if ( len != 2 || buf[0] != 5 ) {
		Com_Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	switch( buf[1] ) {
	case 0:	// no authentication
		break;
	case 2: // username/password authentication
		break;
	default:
		Com_Printf( "NET_OpenSocks: request denied\n" );
		return;
	}

	// do username/password authentication if needed
	if ( buf[1] == 2 ) {
		int		ulen;
		int		plen;

		// build the request
		ulen = strlen( net_socksUsername->string );
		plen = strlen( net_socksPassword->string );

		buf[0] = 1;		// username/password authentication version
		buf[1] = ulen;
		if ( ulen ) {
			memcpy( &buf[2], net_socksUsername->string, ulen );
		}
		buf[2 + ulen] = plen;
		if ( plen ) {
			memcpy( &buf[3 + ulen], net_socksPassword->string, plen );
		}

		// send it
		if ( send( socks_socket, (const char *)buf, 3 + ulen + plen, 0 ) == SOCKET_ERROR ) {
			err = socketError;
			Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
			return;
		}

		// get the response
		len = recv( socks_socket, (char *)buf, 64, 0 );
		if ( len == SOCKET_ERROR ) {
			err = socketError;
			Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
			return;
		}
		if ( len != 2 || buf[0] != 1 ) {
			Com_Printf( "NET_OpenSocks: bad response\n" );
			return;
		}
		if ( buf[1] != 0 ) {
			Com_Printf( "NET_OpenSocks: authentication failed\n" );
			return;
		}
	}

	// send the UDP associate request
	buf[0] = 5;		// SOCKS version
	buf[1] = 3;		// command: UDP associate
	buf[2] = 0;		// reserved
	buf[3] = 1;		// address type: IPV4
	*(int *)&buf[4] = INADDR_ANY;
	*(short *)&buf[8] = htons( (short)port );		// port
	if ( send( socks_socket, (const char *)buf, 10, 0 ) == SOCKET_ERROR ) {
		err = socketError;
		Com_Printf( "NET_OpenSocks: send: %s\n", NET_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, (char *)buf, 64, 0 );
	if( len == SOCKET_ERROR ) {
		err = socketError;
		Com_Printf( "NET_OpenSocks: recv: %s\n", NET_ErrorString() );
		return;
	}
	if( len < 2 || buf[0] != 5 ) {
		Com_Printf( "NET_OpenSocks: bad response\n" );
		return;
	}
	// check completion code
	if( buf[1] != 0 ) {
		Com_Printf( "NET_OpenSocks: request denied: %i\n", buf[1] );
		return;
	}
	if( buf[3] != 1 ) {
		Com_Printf( "NET_OpenSocks: relay address is not IPV4: %i\n", buf[3] );
		return;
	}
	memset( &socksRelayAddr, 0, sizeof( socksRelayAddr ) );
	((struct sockaddr_in *)&socksRelayAddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&socksRelayAddr)->sin_addr.s_addr = *(int *)&buf[4];
	((struct sockaddr_in *)&socksRelayAddr)->sin_port = *(short *)&buf[8];
	memset( ((struct sockaddr_in *)&socksRelayAddr)->sin_zero, 0, 8 );

	usingSocks = qtrue;
}

/*
=====================
NET_GetLocalAddress
=====================
*/
#ifdef MACOS_X
// Don't do a forward mapping from the hostname of the machine to the IP.  The reason is that we might have obtained an IP address from DHCP and there might not be any name registered for the machine.  On Mac OS X, the machine name defaults to 'localhost' and NetInfo has 127.0.0.1 listed for this name.  Instead, we want to get a list of all the IP network interfaces on the machine.
// This code adapted from OmniNetworking.

#ifdef _SIZEOF_ADDR_IFREQ
	// tjw: OSX 10.4 does not have sa_len
	#define IFR_NEXT(ifr)	\
	((struct ifreq *) ((char *) ifr + _SIZEOF_ADDR_IFREQ(*ifr)))
#else
	// tjw: assume that once upon a time some version did have sa_len
	#define IFR_NEXT(ifr)	\
	((struct ifreq *) ((char *) (ifr) + sizeof(*(ifr)) + \
	max(0, (int) (ifr)->ifr_addr.sa_len - (int) sizeof((ifr)->ifr_addr))))
#endif


void NET_GetLocalAddress( void ) {
	struct ifreq requestBuffer[MAX_IPS], *linkInterface, *inetInterface;
	struct ifconf ifc;
	struct ifreq ifr;
	struct sockaddr_dl *sdl;
	int interfaceSocket;
	int family;

	// Set this early so we can just return if there is an error
	numIP = 0;

	ifc.ifc_len = sizeof(requestBuffer);
	ifc.ifc_buf = (caddr_t)requestBuffer;

	// Since we get at this info via an ioctl, we need a temporary little socket.
	// This will only get AF_INET interfaces, but we probably don't care about
	// anything else.  If we do end up caring later, we should add a
	// ONAddressFamily and at a -interfaces method to it.
	family = AF_INET;
	if ((interfaceSocket = socket(family, SOCK_DGRAM, 0)) < 0) {
		Com_Printf("NET_GetLocalAddress: Unable to create temporary socket, errno = %d\n", errno);
		return;
	}

	if (ioctl(interfaceSocket, SIOCGIFCONF, &ifc) != 0) {
		Com_Printf("NET_GetLocalAddress: Unable to get list of network interfaces, errno = %d\n", errno);
		return;
	}

	linkInterface = (struct ifreq *) ifc.ifc_buf;
	while ((char *) linkInterface < &ifc.ifc_buf[ifc.ifc_len]) {
		unsigned int nameLength;

		// The ioctl returns both the entries having the address (AF_INET)
		// and the link layer entries (AF_LINK).  The AF_LINK entry has the
		// link layer address which contains the interface type.  This is the
		// only way I can see to get this information.  We cannot assume that
		// we will get bot an AF_LINK and AF_INET entry since the interface
		// may not be configured.  For example, if you have a 10Mb port on
		// the motherboard and a 100Mb card, you may not configure the
		// motherboard port.

		// For each AF_LINK entry...
		if (linkInterface->ifr_addr.sa_family == AF_LINK) {
			// if there is a matching AF_INET entry
			inetInterface = (struct ifreq *) ifc.ifc_buf;
			while ((char *) inetInterface < &ifc.ifc_buf[ifc.ifc_len]) {
				if (inetInterface->ifr_addr.sa_family == AF_INET &&
						!strncmp(inetInterface->ifr_name, linkInterface->ifr_name,
							sizeof(linkInterface->ifr_name))) {

					for (nameLength = 0; nameLength < IFNAMSIZ; nameLength++)
						if (!linkInterface->ifr_name[nameLength])
							break;

					sdl = (struct sockaddr_dl *)&linkInterface->ifr_addr;
					// Skip loopback interfaces
					if (sdl->sdl_type != IFT_LOOP) {
						// Get the local interface address
						strncpy(ifr.ifr_name, inetInterface->ifr_name, sizeof(ifr.ifr_name));
						if (ioctl(interfaceSocket, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
							Com_Printf("NET_GetLocalAddress: Unable to get local address "
									"for interface '%s', errno = %d\n", inetInterface->ifr_name, errno);
						} else {
							struct sockaddr_in *sin;
							int ip;

							sin = (struct sockaddr_in *)&ifr.ifr_addr;

							ip = ntohl(sin->sin_addr.s_addr);
							localIP[ numIP ][0] = (ip >> 24) & 0xff;
							localIP[ numIP ][1] = (ip >> 16) & 0xff;
							localIP[ numIP ][2] = (ip >>  8) & 0xff;
							localIP[ numIP ][3] = (ip >>  0) & 0xff;
							Com_Printf( "IP: %i.%i.%i.%i (%s)\n",
									localIP[ numIP ][0], localIP[ numIP ][1],
									localIP[ numIP ][2], localIP[ numIP ][3],
									inetInterface->ifr_name);
							numIP++;
						}
					}

					// We will assume that there is only one AF_INET entry per AF_LINK entry.
					// What happens when we have an interface that has multiple IP addresses, or
					// can that even happen?
					// break;
				}
				inetInterface = IFR_NEXT(inetInterface);
			}
		}
		linkInterface = IFR_NEXT(linkInterface);
	}

	close(interfaceSocket);
}
#else
void NET_GetLocalAddress( void )
{
	char				hostname[256];
	struct hostent		*hostInfo;
	int					error;
	char				*p;
	int					ip;
	int					n;

    // Set this early so we can just return if there is an error
	numIP = 0;

	if( gethostname( hostname, 256 ) == SOCKET_ERROR ) {
		error = socketError;
		return;
	}

	hostInfo = gethostbyname( hostname );
	if( !hostInfo ) {
		error = socketError;
		return;
	}

	Com_Printf( "Hostname: %s\n", hostInfo->h_name );
	n = 0;
	while( ( p = hostInfo->h_aliases[n++] ) != NULL ) {
		Com_Printf( "Alias: %s\n", p );
	}

	if ( hostInfo->h_addrtype != AF_INET ) {
		return;
	}

	while( ( p = hostInfo->h_addr_list[numIP] ) != NULL && numIP < MAX_IPS ) {
		ip = ntohl( *(int *)p );
		localIP[ numIP ][0] = p[0];
		localIP[ numIP ][1] = p[1];
		localIP[ numIP ][2] = p[2];
		localIP[ numIP ][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
		numIP++;
	}
}
#endif

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void )
{
	int port = net_port->integer;
	int err;

	NET_GetLocalAddress();

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one

	if ( net_enabled->integer & NET_ENABLEV4 ) {
		for ( int i=0 ; i < 10 ; i++ ) {
			ip_socket = NET_IPSocket( net_ip->string, port + i, &err );
			if ( ip_socket != INVALID_SOCKET ) {
				Cvar_SetValue( "net_port", port + i );

				if ( net_socksEnabled->integer )
					NET_OpenSocks( port + i );
				break;
			}
			else {
				if ( err == EAFNOSUPPORT )
					break;
			}
		}
		if ( ip_socket == INVALID_SOCKET )
			Com_Printf( "WARNING: Couldn't bind to a v4 ip address.\n");
	}
}

//===================================================================

/*
====================
NET_GetCvars
====================
*/
static qboolean NET_GetCvars( void ) {
	int	modified = 0;

	net_enabled = Cvar_Get( "net_enabled", "1", CVAR_LATCH | CVAR_ARCHIVE );
	modified = net_enabled->modified;
	net_enabled->modified = qfalse;

	net_forcenonlocal = Cvar_Get( "net_forcenonlocal", "0", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_forcenonlocal->modified;
	net_forcenonlocal->modified = qfalse;

	net_ip = Cvar_Get( "net_ip", "localhost", CVAR_LATCH );
	modified += net_ip->modified;
	net_ip->modified = qfalse;

	net_port = Cvar_Get( "net_port", XSTRING( PORT_SERVER ), CVAR_LATCH );
	modified += net_port->modified;
	net_port->modified = qfalse;

	net_socksEnabled = Cvar_Get( "net_socksEnabled", "0", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_socksEnabled->modified;
	net_socksEnabled->modified = qfalse;

	net_socksServer = Cvar_Get( "net_socksServer", "", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_socksServer->modified;
	net_socksServer->modified = qfalse;

	net_socksPort = Cvar_Get( "net_socksPort", "1080", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_socksPort->modified;
	net_socksPort->modified = qfalse;

	net_socksUsername = Cvar_Get( "net_socksUsername", "", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_socksUsername->modified;
	net_socksUsername->modified = qfalse;

	net_socksPassword = Cvar_Get( "net_socksPassword", "", CVAR_LATCH | CVAR_ARCHIVE );
	modified += net_socksPassword->modified;
	net_socksPassword->modified = qfalse;

	return modified ? qtrue : qfalse;
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

	if ( !net_enabled->integer )
		enableNetworking = qfalse;

	// if enable state is the same and no cvars were modified, we have nothing to do
	if ( enableNetworking == networkingEnabled && !modified )
		return;

	if ( enableNetworking == networkingEnabled ) {
		if ( enableNetworking ) {
			stop = qtrue;
			start = qtrue;
		}
		else {
			stop = qfalse;
			start = qfalse;
		}
	}
	else {
		if ( enableNetworking ) {
			stop = qfalse;
			start = qtrue;
		}
		else {
			stop = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if ( stop ) {
		if ( ip_socket != INVALID_SOCKET ) {
			closesocket( ip_socket );
			ip_socket = INVALID_SOCKET;
		}

		if ( socks_socket != INVALID_SOCKET ) {
			closesocket( socks_socket );
			socks_socket = INVALID_SOCKET;
		}
	}

	if ( start ) {
		if ( net_enabled->integer )
			NET_OpenIP();
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init( void ) {
#ifdef _WIN32
	int r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
	if( r ) {
		Com_Printf( "WARNING: Winsock initialization failed, returned %d\n", r );
		return;
	}

	winsockInitialized = qtrue;
	Com_Printf( "Winsock Initialized\n" );
#endif

	NET_Config( qtrue );

	Cmd_AddCommand ("net_restart", NET_Restart_f );
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void ) {
	if ( !networkingEnabled ) {
		return;
	}

	NET_Config( qfalse );
#ifdef _WIN32
	WSACleanup();
	winsockInitialized = qfalse;
#endif
}

/*
====================
NET_Sleep

sleeps msec or until net socket is ready
====================
*/
void NET_Sleep( int msec ) {
#ifndef _WIN32
	struct timeval timeout;
	fd_set	fdset;
	extern qboolean stdin_active;

	if ( !com_dedicated->integer )
		return; // we're not a server, just run full speed

	if ( ip_socket == -1 )
		return;

	FD_ZERO(&fdset);
	if (stdin_active)
		FD_SET(0, &fdset); // stdin is processed too
	FD_SET(ip_socket, &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(ip_socket+1, &fdset, NULL, NULL, &timeout);
#endif
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart_f( void ) {
	NET_Config( qtrue );
}
