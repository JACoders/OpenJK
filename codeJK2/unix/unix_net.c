// unix_net.c

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#ifdef NeXT
#include <libc.h>
#endif

static cvar_t	*noudp;

netadr_t	net_local_adr;

int			ip_socket;
int			ipx_socket;

#define	MAX_IPS		16
static	int		numIP;
static	byte	localIP[MAX_IPS][4];

int NET_Socket (char *net_interface, int port);
char *NET_ErrorString (void);

//=============================================================================

void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
	memset (s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int *)&s->sin_addr = -1;
	}
	else if (a->type == NA_IP)
	{
		s->sin_family = AF_INET;

		*(int *)&s->sin_addr = *(int *)&a->ip;
		s->sin_port = a->port;
	}
}

void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a)
{
	*(int *)&a->ip = *(int *)&s->sin_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
Sys_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean	Sys_StringToSockaddr (const char *s, struct sockaddr *sadr)
{
	struct hostent	*h;
	char	*colon;
	
	memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	
	((struct sockaddr_in *)sadr)->sin_port = 0;
	
	if ( s[0] >= '0' && s[0] <= '9')
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else
	{
		if (! (h = gethostbyname(s)) )
			return qfalse;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return qtrue;
}

/*
=============
Sys_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	Sys_StringToAdr (const char *s, netadr_t *a)
{
	struct sockaddr_in sadr;
	
	if (!Sys_StringToSockaddr (s, (struct sockaddr *)&sadr))
		return qfalse;
	
	SockadrToNetadr (&sadr, a);

	return qtrue;
}


//=============================================================================

qboolean	Sys_GetPacket (netadr_t *net_from, msg_t *net_message)
{
	int 	ret;
	struct sockaddr_in	from;
	int		fromlen;
	int		net_socket;
	int		protocol;
	int		err;

	for (protocol = 0 ; protocol < 2 ; protocol++)
	{
		if (protocol == 0)
			net_socket = ip_socket;
		else
			net_socket = ipx_socket;

		if (!net_socket)
			continue;

		fromlen = sizeof(from);
		ret = recvfrom (net_socket, net_message->data, net_message->maxsize
			, 0, (struct sockaddr *)&from, &fromlen);

		SockadrToNetadr (&from, net_from);

		if (ret == -1)
		{
			err = errno;

			if (err == EWOULDBLOCK || err == ECONNREFUSED)
				continue;
			Com_Printf ("NET_GetPacket: %s from %s\n", NET_ErrorString(),
						NET_AdrToString(*net_from));
			continue;
		}

		if (ret == net_message->maxsize)
		{
			Com_Printf ("Oversize packet from %s\n", NET_AdrToString (*net_from));
			continue;
		}

		net_message->cursize = ret;
		return qtrue;
	}

	return qfalse;
}

//=============================================================================

void	Sys_SendPacket( int length, const void *data, netadr_t to )
{
	int		ret;
	struct sockaddr_in	addr;
	int		net_socket;

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_socket;
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_socket;
	}
	else if (to.type == NA_IPX)
	{
		net_socket = ipx_socket;
	}
	else if (to.type == NA_BROADCAST_IPX)
	{
		net_socket = ipx_socket;
	}
	else {
		Com_Error (ERR_FATAL, "NET_SendPacket: bad address type");
		return;
	}

	if (!net_socket)
		return;

	NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr) );
	if (ret == -1)
	{
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
	if ( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// FIXME: ipx?

	if ( adr.type == NA_IP ) {
		for ( i = 0 ; i < numIP ; i++ ) {
			// assuming a class C network, which may not be smart...
			if ( adr.ip[0] == localIP[i][0]
				&& adr.ip[1] == localIP[i][1]
				&& adr.ip[2] == localIP[i][2] ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress( void ) {
	char				hostname[256];
	struct hostent		*hostInfo;
	int					error;
	char				*p;
	int					ip;
	int					n;

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

	numIP = 0;
	while( ( p = hostInfo->h_addr_list[numIP++] ) != NULL && numIP < MAX_IPS ) {
		ip = ntohl( *(int *)p );
		localIP[ numIP ][0] = p[0];
		localIP[ numIP ][1] = p[1];
		localIP[ numIP ][2] = p[2];
		localIP[ numIP ][3] = p[3];
		Com_Printf( "IP: %i.%i.%i.%i\n", ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff, ( ip >> 8 ) & 0xff, ip & 0xff );
	}
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

	ip = Cvar_Get ("net_ip", "localhost", 0);

	port = Cvar_Get("net_port", va("%i", PORT_SERVER), 0)->value;

	for ( i = 0 ; i < 10 ; i++ ) {
		ip_socket = NET_IPSocket (ip->string, port + i);
		if ( ip_socket ) {
			Cvar_SetValue( "net_port", port + i );
			NET_GetLocalAddress();
			return;
		}
	}
	Com_Error (ERR_FATAL, "Couldn't allocate IP port");
}


/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	noudp = Cvar_Get ("net_noudp", "0", 0);
	// open sockets
	if (! noudp->value) {
		NET_OpenIP ();
	}
}


/*
====================
NET_Socket
====================
*/
int NET_IPSocket (char *net_interface, int port)
{
	int newsocket;
	struct sockaddr_in address;
	qboolean _qtrue = qtrue;
	int	i = 1;

	if ( net_interface ) {
		Com_Printf("Opening IP socket: %s:%i\n", net_interface, port );
	} else {
		Com_Printf("Opening IP socket: localhost:%i\n", port );
	}

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		Com_Printf ("ERROR: UDP_OpenSocket: socket: %s", NET_ErrorString());
		return 0;
	}

	// make it non-blocking
	if (ioctl (newsocket, FIONBIO, &_qtrue) == -1)
	{
		Com_Printf ("ERROR: UDP_OpenSocket: ioctl FIONBIO:%s\n", NET_ErrorString());
		return 0;
	}

	// make it broadcast capable
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) == -1)
	{
		Com_Printf ("ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", NET_ErrorString());
		return 0;
	}

	if (!net_interface || !net_interface[0] || !stricmp(net_interface, "localhost"))
		address.sin_addr.s_addr = INADDR_ANY;
	else
		Sys_StringToSockaddr (net_interface, (struct sockaddr *)&address);

	if (port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);

	address.sin_family = AF_INET;

	if( bind (newsocket, (void *)&address, sizeof(address)) == -1)
	{
		Com_Printf ("ERROR: UDP_OpenSocket: bind: %s\n", NET_ErrorString());
		close (newsocket);
		return 0;
	}

	return newsocket;
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	if (ip_socket) {
		close(ip_socket);
		ip_socket = 0;
	}
}


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

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern qboolean stdin_active;

	if (!ip_socket || !com_dedicated->integer)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	if (stdin_active)
		FD_SET(0, &fdset); // stdin is processed too
	FD_SET(ip_socket, &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(ip_socket+1, &fdset, NULL, NULL, &timeout);
}

