#include "../client/client.h"
#include "mac_local.h"
#include <OpenTransport.h>
#include <OpenTptInternet.h>

static qboolean	gOTInited;
static EndpointRef endpoint = kOTInvalidEndpointRef;
static EndpointRef resolverEndpoint = kOTInvalidEndpointRef;

#define	MAX_IPS		16
static	int		numIP;
static	InetInterfaceInfo	sys_inetInfo[MAX_IPS];

static	TUDErr	uderr;

void RcvUDErr( void ) {
	memset( &uderr, 0, sizeof( uderr ) );
	uderr.addr.maxlen = 0;
	uderr.opt.maxlen = 0;
	OTRcvUDErr( endpoint, &uderr );
}

void HandleOTError( int err, const char *func ) {
	int		r;
	static int lastErr;

	if ( err != lastErr ) {
		Com_Printf( "%s: error %i\n", func, err );
	}
	
	// if we don't call OTLook, things wedge
	r = OTLook( endpoint );
	if ( err != lastErr ) {
		Com_DPrintf( "%s: OTLook %i\n", func, r );
	}

	switch( r ) {
	case T_UDERR:
		RcvUDErr();
		if ( err != lastErr ) {
			Com_DPrintf( "%s: OTRcvUDErr %i\n", func, uderr.error );				
		}
		break;
	default:
//		Com_Printf( "%s: Unknown OTLook error %i\n", func, r );				
		break;
	}
	lastErr = err;	// don't spew tons of messages
}

/*
=================
NotifyProc
=================
*/
pascal void NotifyProc(void* contextPtr, OTEventCode code, 
									   OTResult result, void* cookie) {
	switch( code ) {
	case T_OPENCOMPLETE:
		endpoint = cookie;
		break;
	case T_UDERR:
		RcvUDErr();
		break;
	}
}


/*
=================
GetFourByteOption
=================
*/
static OTResult GetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32   *value)
{
   OTResult err;
   TOption  option;
   TOptMgmt request;
   TOptMgmt result;
   
   /* Set up the option buffer */
   option.len  = kOTFourByteOptionSize;
   option.level= level;
   option.name = name;
   option.status = 0;
   option.value[0] = 0;// Ignored because we're getting the value.

   /* Set up the request parameter for OTOptionManagement to point
    to the option buffer we just filled out */

   request.opt.buf= (UInt8 *) &option;
   request.opt.len= sizeof(option);
   request.flags= T_CURRENT;

   /* Set up the reply parameter for OTOptionManagement. */
   result.opt.buf  = (UInt8 *) &option;
   result.opt.maxlen = sizeof(option);
   
   err = OTOptionManagement(ep, &request, &result);

   if (err == noErr) {
      switch (option.status) 
      {
         case T_SUCCESS:
         case T_READONLY:
            *value = option.value[0];
            break;
         default:
            err = option.status;
            break;
      }
   }
            
   return (err);
}


/*
=================
SetFourByteOption
=================
*/
static OTResult SetFourByteOption(EndpointRef ep,
                           OTXTILevel level,
                           OTXTIName  name,
                           UInt32   value)
{
   OTResult err;
   TOption  option;
   TOptMgmt request;
   TOptMgmt result;
   
   /* Set up the option buffer to specify the option and value to
         set. */
   option.len  = kOTFourByteOptionSize;
   option.level= level;
   option.name = name;
   option.status = 0;
   option.value[0] = value;

   /* Set up request parameter for OTOptionManagement */
   request.opt.buf= (UInt8 *) &option;
   request.opt.len= sizeof(option);
   request.flags  = T_NEGOTIATE;

   /* Set up reply parameter for OTOptionManagement. */
   result.opt.buf  = (UInt8 *) &option;
   result.opt.maxlen  = sizeof(option);

   
   err = OTOptionManagement(ep, &request, &result);

   if (err == noErr) {
      if (option.status != T_SUCCESS) 
         err = option.status;
   }
            
   return (err);
}


/*
=====================
NET_GetLocalAddress
=====================
*/
void NET_GetLocalAddress( void ) {
	OSStatus		err;

	for ( numIP = 0 ; numIP < MAX_IPS ; numIP++ ) {
		err = OTInetGetInterfaceInfo( &sys_inetInfo[ numIP ], numIP );
		if ( err ) {
			break;
		}
		Com_Printf( "LocalAddress: %i.%i.%i.%i\n",
			((byte *)&sys_inetInfo[numIP].fAddress)[0],		
			((byte *)&sys_inetInfo[numIP].fAddress)[1],		
			((byte *)&sys_inetInfo[numIP].fAddress)[2],		
			((byte *)&sys_inetInfo[numIP].fAddress)[3] );		

		Com_Printf( "Netmask: %i.%i.%i.%i\n",
			((byte *)&sys_inetInfo[numIP].fNetmask)[0],		
			((byte *)&sys_inetInfo[numIP].fNetmask)[1],		
			((byte *)&sys_inetInfo[numIP].fNetmask)[2],		
			((byte *)&sys_inetInfo[numIP].fNetmask)[3] );		
	}
}


/*
==================
Sys_InitNetworking


struct InetAddress
{
		OTAddressType	fAddressType;	// always AF_INET
		InetPort		fPort;			// Port number 
		InetHost		fHost;			// Host address in net byte order
		UInt8			fUnused[8];		// Traditional unused bytes
};
typedef struct InetAddress InetAddress;

==================
*/
void Sys_InitNetworking( void ) {
	OSStatus		err;
	OTConfiguration *config;
	TBind			bind, bindOut;
	InetAddress		in, out;
	int				i;

	Com_Printf( "----- Sys_InitNetworking -----\n" );
	// init OpenTransport	
	Com_Printf( "... InitOpenTransport()\n" );
	err = InitOpenTransport();
	if ( err != noErr ) {
		Com_Printf( "InitOpenTransport() failed\n" );
		Com_Printf( "------------------------------\n" );
		return;
	}
	
  	gOTInited = true;

	// get an endpoint
	Com_Printf( "... OTOpenEndpoint()\n" );
	config = OTCreateConfiguration( kUDPName );

#if 1
	endpoint = OTOpenEndpoint( config, 0, nil, &err); 
#else
	err = OTAsyncOpenEndpoint( config, 0, 0, NotifyProc, 0 );
	if ( !endpoint ) {
		err = 1;
	}
#endif

	if ( err != noErr ) {
		endpoint = 0;
		Com_Printf( "OTOpenEndpoint() failed\n" );
		Com_Printf( "------------------------------\n" );
		return;
	}

	// set non-blocking	
	err = OTSetNonBlocking( endpoint );
 
	// scan for a valid port in our range
	Com_Printf( "... OTBind()\n" );
	for ( i = 0 ; i < 10 ; i++ ) {
		in.fAddressType = AF_INET;
		in.fPort = PORT_SERVER + i;
		in.fHost = 0;

		bind.addr.maxlen = sizeof( in );
		bind.addr.len = sizeof( in );
		bind.addr.buf = (unsigned char *)&in;
		bind.qlen = 0;
		
		bindOut.addr.maxlen = sizeof( out );
		bindOut.addr.len = sizeof( out );
		bindOut.addr.buf = (unsigned char *)&out;
		bindOut.qlen = 0;
		
		err = OTBind( endpoint, &bind, &bindOut );
		if ( err == noErr ) {
			Com_Printf( "Opened UDP endpoint at port %i\n",
				out.fPort );
			break;
		}
	}

	if ( err != noErr ) {
		Com_Printf( "Couldn't bind a local port\n" );
	}

	// get the local address for LAN client detection
	NET_GetLocalAddress();


	// set to allow broadcasts
	err = SetFourByteOption( endpoint, INET_IP, IP_BROADCAST, T_YES );

	if ( err != noErr ) {
		Com_Printf( "IP_BROADCAST failed\n" );
	}
	
	// get an endpoint just for resolving addresses, because
	// I was having crashing problems doing it on the same endpoint
	config = OTCreateConfiguration( kUDPName );
	resolverEndpoint = OTOpenEndpoint( config, 0, nil, &err); 
	if ( err != noErr ) {
		resolverEndpoint = 0;
		Com_Printf( "OTOpenEndpoint() for resolver failed\n" );
		Com_Printf( "------------------------------\n" );
		return;
	}

	in.fAddressType = AF_INET;
	in.fPort = 0;
	in.fHost = 0;

	bind.addr.maxlen = sizeof( in );
	bind.addr.len = sizeof( in );
	bind.addr.buf = (unsigned char *)&in;
	bind.qlen = 0;
	
	bindOut.addr.maxlen = sizeof( out );
	bindOut.addr.len = sizeof( out );
	bindOut.addr.buf = (unsigned char *)&out;
	bindOut.qlen = 0;
	
	err = OTBind( resolverEndpoint, &bind, &bindOut );
		
	Com_Printf( "------------------------------\n" );
}


/*
==================
Sys_ShutdownNetworking
==================
*/
void Sys_ShutdownNetworking( void ) {
	Com_Printf( "Sys_ShutdownNetworking();\n" );

	if ( endpoint != kOTInvalidEndpointRef ) {
		OTUnbind( endpoint );
		OTCloseProvider( endpoint );
		endpoint = kOTInvalidEndpointRef;
	}
	if ( resolverEndpoint != kOTInvalidEndpointRef ) {
		OTUnbind( resolverEndpoint );
		OTCloseProvider( resolverEndpoint );
		resolverEndpoint = kOTInvalidEndpointRef;
	}
	if (gOTInited) {
		CloseOpenTransport();
		gOTInited = false;
	}
}

/*
=============
Sys_StringToAdr


Does NOT parse port numbers


idnewt
192.246.40.70
=============
*/
qboolean	Sys_StringToAdr( const char *s, netadr_t *a ) {
	OSStatus	err;
	TBind		in, out;
	InetAddress	inAddr;
	DNSAddress	dnsAddr;

	if ( !resolverEndpoint ) {
		return qfalse;
	}

	memset( &in, 0, sizeof( in ) );
	in.addr.buf = (UInt8 *) &dnsAddr;
	in.addr.len = OTInitDNSAddress(&dnsAddr, (char *)s );
	in.qlen = 0;
	
	memset( &out, 0, sizeof( out ) );
	out.addr.buf = (byte *)&inAddr;
	out.addr.maxlen = sizeof( inAddr );
	out.qlen = 0;
	                           
	err = OTResolveAddress( resolverEndpoint, &in, &out, 10000 );
	if ( err ) {
		HandleOTError( err, "Sys_StringToAdr" );
		return qfalse;
	}
	
	a->type = NA_IP;
	*(int *)a->ip = inAddr.fHost;

	return qtrue;
}

/*
==================
Sys_SendPacket
==================
*/
#define	MAX_PACKETLEN	1400
void Sys_SendPacket( int length, const void *data, netadr_t to ) {
	TUnitData	d;
	InetAddress	inAddr;
	OSStatus	err;

	if ( !endpoint ) {
		return;
	}

	if ( length > MAX_PACKETLEN ) {
		Com_Error( ERR_DROP, "Sys_SendPacket: length > MAX_PACKETLEN" );
	}

	inAddr.fAddressType = AF_INET;
	inAddr.fPort = to.port;	
	if ( to.type == NA_BROADCAST ) {
		inAddr.fHost = -1;
	} else {
		inAddr.fHost = *(int *)&to.ip;
	}
	
	memset( &d, 0, sizeof( d ) );
	
	d.addr.len = sizeof( inAddr );
	d.addr.maxlen = sizeof( inAddr );
	d.addr.buf = (unsigned char *)&inAddr;

	d.opt.len = 0;
	d.opt.maxlen = 0;
	d.opt.buf = NULL;
		
	d.udata.len = length;
	d.udata.maxlen = length;
	d.udata.buf = (unsigned char *)data;
	
	err = OTSndUData( endpoint, &d );
	if ( err ) {
		HandleOTError( err, "Sys_SendPacket" );
	}
}

/*
==================
Sys_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message ) {
	TUnitData	d;
	InetAddress	inAddr;
	OSStatus	err;
	OTFlags		flags;
	
	if ( !endpoint ) {
		return qfalse;
	}

	inAddr.fAddressType = AF_INET;
	inAddr.fPort = 0;
	inAddr.fHost = 0;

	memset( &d, 0, sizeof( d ) );

	d.addr.len = sizeof( inAddr );
	d.addr.maxlen = sizeof( inAddr );
	d.addr.buf = (unsigned char *)&inAddr;

	d.opt.len = 0;
	d.opt.maxlen = 0;
	d.opt.buf = 0;
		
	d.udata.len = net_message->maxsize;
	d.udata.maxlen = net_message->maxsize;
	d.udata.buf = net_message->data;
	
	err = OTRcvUData( endpoint, &d, &flags );
	if ( err ) {
		if ( err == kOTNoDataErr ) {
			return false;
		}
		HandleOTError( err, "Sys_GetPacket" );
		return qfalse;
	}

	net_from->type = NA_IP;
	net_from->port = inAddr.fPort;
	*(int *)net_from->ip = inAddr.fHost;

	net_message->cursize = d.udata.len;
	
	return qtrue;
}


/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
qboolean	Sys_IsLANAddress (netadr_t adr) {
	int		i;
	int		ip;
	
	if ( adr.type == NA_LOOPBACK ) {
		return qtrue;
	}

	if ( adr.type != NA_IP ) {
		return qfalse;
	}
	
	for ( ip = 0 ; ip < numIP ; ip++ ) {
		for ( i = 0 ; i < 4 ; i++ ) {
			if ( ( adr.ip[i] & ((byte *)&sys_inetInfo[ip].fNetmask)[i] )
			!= ( ((byte *)&sys_inetInfo[ip].fAddress)[i] & ((byte *)&sys_inetInfo[ip].fNetmask)[i] ) ) {
				break;
			}
		}
		if ( i == 4 ) {
			return qtrue;	// matches this subnet
		}
	}
	
	return qfalse;
}


void NET_Sleep( int i ) {
}
