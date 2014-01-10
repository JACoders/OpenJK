// unix_net.c

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> // bk001204

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#ifdef MACOS_X
#import <sys/sockio.h>
#import <net/if.h>
#import <net/if_types.h>

#import <arpa/inet.h>         // for inet_ntoa()
#import <net/if_dl.h>         // for 'struct sockaddr_dl'
#endif

static qboolean networkingEnabled = qfalse;

static cvar_t	*net_noudp;
static cvar_t	*net_forcenonlocal;

static int		ip_socket = -1;

#define	MAX_IPS		16
static	int		numIP;
static	byte	localIP[MAX_IPS][4];

//=============================================================================

/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString (void)
{
	int		code;

	code = errno;
	return strerror (code);
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
static qboolean Sys_StringToSockaddr (const char *s, struct sockaddr *sadr)
{
	struct hostent	*h;

	memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;

	((struct sockaddr_in *)sadr)->sin_port = 0;

	if ( s[0] >= '0' && s[0] <= '9')
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
qboolean	Sys_StringToAdr (const char *s, netadr_t *a)
{
	struct sockaddr sadr;
	
	if (!Sys_StringToSockaddr (s, &sadr))
		return qfalse;
	
	SockadrToNetadr (&sadr, a);

	return qtrue;
}


//=============================================================================

qboolean	Sys_GetPacket (netadr_t *net_from, msg_t *net_message)
{
	int 	ret;
	struct sockaddr	from;
	socklen_t	fromlen;
	int		err;

	fromlen = sizeof(from);
	ret = recvfrom (ip_socket, net_message->data, net_message->maxsize, 0, (struct sockaddr *)&from, &fromlen);

	if (ret == -1)
	{
		err = errno;

		if ( err == EAGAIN || err == ECONNRESET )
			return qfalse;

		Com_Printf ("NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AdrToString(*net_from));
		return qfalse;
	}

	memset( ((struct sockaddr_in *)&from)->sin_zero, 0, 8 );

	SockadrToNetadr (&from, net_from);
	net_message->readcount = 0;

	if (ret == net_message->maxsize) {
		Com_Printf ("Oversize packet from %s\n", NET_AdrToString (*net_from));
		return qfalse;
	}

	net_message->cursize = ret;
	return qtrue;
}

//=============================================================================

void	Sys_SendPacket( int length, const void *data, netadr_t to )
{
	int				ret;
	struct sockaddr	addr;

	if ( to.type != NA_BROADCAST && to.type != NA_IP ) {
		Com_Error( ERR_FATAL, "Sys_SendPacket: bad address type" );
		return;
	}

	if ( ip_socket == -1 )
		return;

	NetadrToSockadr (&to, &addr);

	ret = sendto (ip_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr) );
	if (ret == -1)
	{
		int err = errno;

		// wouldblock is silent
		if( err == EAGAIN ) {
			return;
		}

		// some PPP links do not allow broadcasts and return an error
		if( err == EADDRNOTAVAIL && to.type == NA_BROADCAST ) {
			return;
		}

		Com_Printf ("NET_SendPacket ERROR: %s to %s\n", NET_ErrorString(),
				NET_AdrToString (to));
	}
}


//=============================================================================

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean	Sys_IsLANAddress (netadr_t adr) {
	int		i;

	if (!net_forcenonlocal)
	{
		net_forcenonlocal = Cvar_Get( "net_forcenonlocal", "0", 0 );
	}

	if (net_forcenonlocal && net_forcenonlocal->integer)
	{
		return qfalse;
	}

	if( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if( adr.type != NA_IP ) {
		return qfalse;
	}

	// choose which comparison to use based on the class of the address being tested
	// any local adresses of a different class than the address being tested will fail based on the first byte
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
		// also check against the RFC1918 class c blocks
		if( adr.ip[0] == 192 && localIP[i][0] == 192 && adr.ip[1] == 168 && localIP[i][1] == 168 ) {
			return qtrue;
		}
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

/*
=====================
NET_GetLocalAddress
=====================
*/
#ifdef MACOS_X
// Don't do a forward mapping from the hostname of the machine to the IP.  The reason is that we might have obtained an IP address from DHCP and there might not be any name registered for the machine.  On Mac OS X, the machine name defaults to 'localhost' and NetInfo has 127.0.0.1 listed for this name.  Instead, we want to get a list of all the IP network interfaces on the machine.
// This code adapted from OmniNetworking.

#define IFR_NEXT(ifr)	\
    ((struct ifreq *) ((char *) (ifr) + sizeof(*(ifr)) + \
      MAX(0, (int) (ifr)->ifr_addr.sa_len - (int) sizeof((ifr)->ifr_addr))))

void NET_GetLocalAddress( void ) {
        struct ifreq requestBuffer[MAX_IPS], *linkInterface, *inetInterface;
        struct ifconf ifc;
        struct ifreq ifr;
        struct sockaddr_dl *sdl;
        int interfaceSocket;
        int family;

        //Com_Printf("NET_GetLocalAddress: Querying for network interfaces\n");

        // Set this early so we can just return if there is an error
	numIP = 0;

        ifc.ifc_len = sizeof(requestBuffer);
        ifc.ifc_buf = (caddr_t)requestBuffer;

        // Since we get at this info via an ioctl, we need a temporary little socket.  This will only get AF_INET interfaces, but we probably don't care about anything else.  If we do end up caring later, we should add a ONAddressFamily and at a -interfaces method to it.
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

            // The ioctl returns both the entries having the address (AF_INET) and the link layer entries (AF_LINK).  The AF_LINK entry has the link layer address which contains the interface type.  This is the only way I can see to get this information.  We cannot assume that we will get bot an AF_LINK and AF_INET entry since the interface may not be configured.  For example, if you have a 10Mb port on the motherboard and a 100Mb card, you may not configure the motherboard port.

            // For each AF_LINK entry...
            if (linkInterface->ifr_addr.sa_family == AF_LINK) {
                // if there is a matching AF_INET entry
                inetInterface = (struct ifreq *) ifc.ifc_buf;
                while ((char *) inetInterface < &ifc.ifc_buf[ifc.ifc_len]) {
                    if (inetInterface->ifr_addr.sa_family == AF_INET &&
                        !strncmp(inetInterface->ifr_name, linkInterface->ifr_name, sizeof(linkInterface->ifr_name))) {

                        for (nameLength = 0; nameLength < IFNAMSIZ; nameLength++)
                            if (!linkInterface->ifr_name[nameLength])
                                break;

                        sdl = (struct sockaddr_dl *)&linkInterface->ifr_addr;
                        // Skip loopback interfaces
                        if (sdl->sdl_type != IFT_LOOP) {
                            // Get the local interface address
                            strncpy(ifr.ifr_name, inetInterface->ifr_name, sizeof(ifr.ifr_name));
                            if (ioctl(interfaceSocket, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
                                Com_Printf("NET_GetLocalAddress: Unable to get local address for interface '%s', errno = %d\n", inetInterface->ifr_name, errno);
                            } else {
                                struct sockaddr_in *sin;
                                int ip;

                                sin = (struct sockaddr_in *)&ifr.ifr_addr;

                                ip = ntohl(sin->sin_addr.s_addr);
                                localIP[ numIP ][0] = (ip >> 24) & 0xff;
                                localIP[ numIP ][1] = (ip >> 16) & 0xff;
                                localIP[ numIP ][2] = (ip >>  8) & 0xff;
                                localIP[ numIP ][3] = (ip >>  0) & 0xff;
                                Com_Printf( "IP: %i.%i.%i.%i (%s)\n", localIP[ numIP ][0], localIP[ numIP ][1], localIP[ numIP ][2], localIP[ numIP ][3], inetInterface->ifr_name);
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
void NET_GetLocalAddress( void ) {
	char				hostname[256];
	struct hostent		*hostInfo;
	// int					error; // bk001204 - unused
	char				*p;
	int					ip;
	int					n;

    // Set this early so we can just return if there is an error
	numIP = 0;

	if ( gethostname( hostname, 256 ) == -1 ) {
		return;
	}

	hostInfo = gethostbyname( hostname );
	if ( !hostInfo ) {
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
NET_IPSocket
====================
*/
int NET_IPSocket (char *net_interface, int port)
{
	int newsocket;
	struct sockaddr_in address;
	u_long _true = 1;
	int	i = 1;
	int err;

	if ( net_interface ) {
		Com_Printf("Opening IP socket: %s:%i\n", net_interface, port );
	} else {
		Com_Printf("Opening IP socket: localhost:%i\n", port );
	}

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		err = errno;
		if ( err != EAFNOSUPPORT )
			Com_Printf ("WARNING: NET_IPSocket: socket: %s", NET_ErrorString());
		return -1;
	}

	// make it non-blocking
	if (ioctl (newsocket, FIONBIO, &_true) == -1)
	{
		Com_Printf ("WARNING: NET_IPSocket: ioctl FIONBIO:%s\n", NET_ErrorString());
		close( newsocket );
		return -1;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		Com_Printf ("WARNING: NET_IPSocket: setsockopt SO_BROADCAST:%s\n", NET_ErrorString());
	}

	if (!net_interface || !net_interface[0] || !Q_stricmp(net_interface, "localhost")) {
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		if ( !Sys_StringToSockaddr( net_interface, (struct sockaddr *)&address ) ) {
			close( newsocket );
			return -1;
		}
	}

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	if( bind (newsocket, (const sockaddr *)&address, sizeof(address)) == -1)
	{
		Com_Printf ("WARNING: NET_IPSocket: bind: %s\n", NET_ErrorString());
		close( newsocket );
		return -1;
	}

	return newsocket;
}

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	cvar_t	*ip;
	int		port;
	int		i;

	ip = Cvar_Get ("net_ip", "localhost", CVAR_LATCH);
	port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH)->value;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = NET_IPSocket (ip->string, port + i);
		if ( ip_socket != -1 ) {
			Cvar_SetValue( "net_port", port + i );
			NET_GetLocalAddress();
			return;
		}
	}
	Com_Printf( "WARNING: Couldn't allocate IP port\n");
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

	if( net_forcenonlocal && net_forcenonlocal->modified ) {
		modified = qtrue;
	}
	net_forcenonlocal = Cvar_Get( "net_forcenonlocal", "0", CVAR_LATCH | CVAR_ARCHIVE );

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
		if ( ip_socket && ip_socket != -1 ) {
			close( ip_socket );
			ip_socket = -1;
		}
	}

	if( start ) {
		if (! net_noudp->integer ) {
			NET_OpenIP();
		}
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	// this is really just to get the cvars registered
	NET_GetCvars();

	NET_Config( qtrue );
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	if ( !networkingEnabled ) {
		return;
	}

	NET_Config( qfalse );
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
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
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart( void ) {
	NET_Config( networkingEnabled );
}
