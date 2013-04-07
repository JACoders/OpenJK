
#include "../game/q_shared.h"
#include "qcommon.h"

/*

packet header
-------------
4	outgoing sequence.  high bit will be set if this is a fragmented message
4	acknowledge sequence
[2	qport (only for client to server)]
[2	fragment start byte]
[2	fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.

*/


#define	MAX_PACKETLEN			(MAX_MSGLEN)	//(1400)		// max size of a network packet

#if (MAX_PACKETLEN > MAX_MSGLEN)
#error MAX_PACKETLEN must be <= MAX_MSGLEN
#endif

#define	FRAGMENT_SIZE			(MAX_PACKETLEN - 100)
#define	PACKET_HEADER			10			// two ints and a short

#define	FRAGMENT_BIT	(1<<31)

cvar_t		*showpackets;
cvar_t		*showdrop;
cvar_t		*qport;

static char *netsrcString[2] = {
	"client",
	"server"
};

/*
===============
Netchan_Init

===============
*/
void Netchan_Init( int port ) {
	
	port &= 0xffff;
	showpackets = Cvar_Get ("showpackets", "0", CVAR_TEMP );
	showdrop = Cvar_Get ("showdrop", "0", CVAR_TEMP );
	qport = Cvar_Get ("qport", va("%i", port), CVAR_INIT );
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport ) {
	memset (chan, 0, sizeof(*chan));
	
	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;
}


/*
==============
Netchan_ScramblePacket

A probably futile attempt to make proxy hacking somewhat
more difficult.

XOR's everything in the buffer after the first header
bytes with a sequence based on the value of the first int.
Used for both scrambling and descrambling.
==============
*/
static void Netchan_ScramblePacket( msg_t *buf ) {
	unsigned	seed;
	int			i, c;

	seed = LittleLong( *(unsigned *)buf->data );

	// byte xor the data after the header with
	// a "random" sequence
	c = buf->cursize;
	for (i = PACKET_HEADER ; i < c ; i++) {
		seed = (69069 * seed + 1);
		buf->data[i] ^= seed;
	}
}


/*
===============
Netchan_Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void Netchan_Transmit( netchan_t *chan, int length, const byte *data ) {
	msg_t		send;
	byte		send_buf[MAX_PACKETLEN];
	int			fragmentStart, fragmentLength;

	fragmentStart = 0;		// stop warning message
	fragmentLength = 0;

	// fragment large reliable messages
	if ( length >= FRAGMENT_SIZE ) {
		fragmentStart = 0;
		do {
			// write the packet header
			MSG_Init (&send, send_buf, sizeof(send_buf));

			MSG_WriteLong( &send, chan->outgoingSequence | FRAGMENT_BIT );
			MSG_WriteLong( &send, chan->incomingSequence );

			// send the qport if we are a client
			if ( chan->sock == NS_CLIENT ) {
				MSG_WriteShort( &send, qport->integer );
			}

			// copy the reliable message to the packet first
			fragmentLength = FRAGMENT_SIZE;
			if ( fragmentStart  + fragmentLength > length ) {
				fragmentLength = length - fragmentStart;
			}

			MSG_WriteShort( &send, fragmentStart );
			MSG_WriteShort( &send, fragmentLength );
			MSG_WriteData( &send, data + fragmentStart, fragmentLength );

			// XOR scramble all data in the packet after the header
			Netchan_ScramblePacket( &send );

			// send the datagram
			NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );

			if ( showpackets->integer ) {
				Com_Printf ("%s send %4i : s=%i ack=%i fragment=%i,%i\n"
					, netsrcString[ chan->sock ]
					, send.cursize
					, chan->outgoingSequence - 1
					, chan->incomingSequence
					, fragmentStart, fragmentLength);
			}

			fragmentStart += fragmentLength;
			// this exit condition is a little tricky, because a packet
			// that is exactly the fragment length still needs to send
			// a second packet of zero length so that the other side
			// can tell there aren't more to follow
		} while ( fragmentStart != length || fragmentLength == FRAGMENT_SIZE );

		chan->outgoingSequence++;
		return;
	}

	// write the packet header
	MSG_Init (&send, send_buf, sizeof(send_buf));

	MSG_WriteLong( &send, chan->outgoingSequence );
	MSG_WriteLong( &send, chan->incomingSequence );
	chan->outgoingSequence++;

	// send the qport if we are a client
	if ( chan->sock == NS_CLIENT ) {
		MSG_WriteShort( &send, qport->integer );
	}

	MSG_WriteData( &send, data, length );

	// XOR scramble all data in the packet after the header
	Netchan_ScramblePacket( &send );

	// send the datagram
	NET_SendPacket( chan->sock, send.cursize, send.data, chan->remoteAddress );

	if ( showpackets->integer ) {
		Com_Printf( "%s send %4i : s=%i ack=%i\n"
			, netsrcString[ chan->sock ]
			, send.cursize
			, chan->outgoingSequence - 1
			, chan->incomingSequence );
	}
}

/*
=================
Netchan_Process

Returns qfalse if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
qboolean Netchan_Process( netchan_t *chan, msg_t *msg ) {
	int			sequence, sequence_ack;
	int			qport;
	int			fragmentStart, fragmentLength;
	qboolean	fragmented;

	// XOR unscramble all data in the packet after the header
	Netchan_ScramblePacket( msg );

	// get sequence numbers		
	MSG_BeginReading( msg );
	sequence = MSG_ReadLong( msg );
	sequence_ack = MSG_ReadLong( msg );

	// check for fragment information
	if ( sequence & FRAGMENT_BIT ) {
		sequence &= ~FRAGMENT_BIT;
		fragmented = qtrue;
	} else {
		fragmented = qfalse;
	}

	// read the qport if we are a server
	if ( chan->sock == NS_SERVER ) {
		qport = MSG_ReadShort( msg );
	}

	// read the fragment information
	if ( fragmented ) {
		fragmentStart = MSG_ReadShort( msg );
		fragmentLength = MSG_ReadShort( msg );
	} else {
		fragmentStart = 0;		// stop warning message
		fragmentLength = 0;
	}

	if ( showpackets->integer ) {
		if ( fragmented ) {
			Com_Printf( "%s recv %4i : s=%i ack=%i fragment=%i,%i\n"
				, netsrcString[ chan->sock ]
				, msg->cursize
				, sequence
				, sequence_ack
				, fragmentStart, fragmentLength );
		} else {
			Com_Printf( "%s recv %4i : s=%i ack=%i\n"
				, netsrcString[ chan->sock ]
				, msg->cursize
				, sequence
				, sequence_ack );
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if ( sequence <= chan->incomingSequence ) {
		if ( showdrop->integer || showpackets->integer ) {
			Com_Printf( "%s:Out of order packet %i at %i\n"
				, NET_AdrToString( chan->remoteAddress )
				,  sequence
				, chan->incomingSequence );
		}
		return qfalse;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - (chan->incomingSequence+1);
	if ( chan->dropped > 0 ) {
		if ( showdrop->integer || showpackets->integer ) {
			Com_Printf( "%s:Dropped %i packets at %i\n"
			, NET_AdrToString( chan->remoteAddress )
			, chan->dropped
			, sequence );
		}
	}
	

	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence 
	//
	if ( fragmented ) {
		// make sure we 
		if ( sequence != chan->fragmentSequence ) {
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if ( fragmentStart != chan->fragmentLength ) {
			if ( showdrop->integer || showpackets->integer ) {
				Com_Printf( "%s:Dropped a message fragment\n"
				, NET_AdrToString( chan->remoteAddress )
				, sequence);
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return qfalse;
		}

		// copy the fragment to the fragment buffer
		if ( fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > sizeof( chan->fragmentBuffer ) ) {
			if ( showdrop->integer || showpackets->integer ) {
				Com_Printf ("%s:illegal fragment length\n"
				, NET_AdrToString (chan->remoteAddress ) );
			}
			return qfalse;
		}

		memcpy( chan->fragmentBuffer + chan->fragmentLength, 
			msg->data + msg->readcount, fragmentLength );

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if ( fragmentLength == FRAGMENT_SIZE ) {
			return qfalse;
		}

		if ( chan->fragmentLength > msg->maxsize ) {
			Com_Printf( "%s:fragmentLength %i > msg->maxsize\n"
				, NET_AdrToString (chan->remoteAddress ),
				chan->fragmentLength );
			return qfalse;
		}

		// copy the full message over the partial fragment

		// make sure the sequence number is still there
		*(int *)msg->data = LittleLong( sequence );

		memcpy( msg->data + 4, chan->fragmentBuffer, chan->fragmentLength );
		msg->cursize = chan->fragmentLength + 4;
		chan->fragmentLength = 0;
		msg->readcount = 4;	// past the sequence number

		return qtrue;
	}

	//
	// the message can now be read from the current message pointer
	//
	chan->incomingSequence = sequence;
	chan->incomingAcknowledged = sequence_ack;

	return qtrue;
}


//==============================================================================

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return qfalse;

	if (a.type == NA_LOOPBACK)
		return qtrue;

	Com_Printf ("NET_CompareBaseAdr: bad address type\n");
	return qfalse;
}

const char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];

	if (a.type == NA_LOOPBACK) {
		Com_sprintf (s, sizeof(s), "loopback");
	}

	return s;
}


qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return qfalse;

	if (a.type == NA_LOOPBACK)
		return qtrue;

	Com_Printf ("NET_CompareAdr: bad address type\n");
	return qfalse;
}


qboolean	NET_IsLocalAddress( netadr_t adr ) {
	return adr.type == NA_LOOPBACK;
}



/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define	MAX_LOOPBACK	16

typedef struct {
	byte	data[MAX_PACKETLEN];
	int		datalen;
} loopmsg_t;

typedef struct {
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];


qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return qfalse;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return qtrue;

}


void NET_SendLoopPacket (netsrc_t sock, int length, const void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================


void NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to ) {

	// sequenced packets are shown in netchan, so just show oob
	if ( showpackets->integer && *(int *)data == -1 )	{
		Com_Printf ("send packet %4i\n", length);
	}

	if ( to.type == NA_LOOPBACK ) {
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}
}

/*
===============
NET_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void QDECL NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, const char *format, ... ) {
	va_list		argptr;
	char		string[MAX_MSGLEN];

	// set the header
	string[0] = (char) 0xff;
	string[1] = (char) 0xff;
	string[2] = (char) 0xff;
	string[3] = (char) 0xff;

	va_start( argptr, format );
	vsprintf( string+4, format, argptr );
	va_end( argptr );

	// send the datagram
	NET_SendPacket( sock, strlen( string ), string, adr );
}



/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
=============
*/
qboolean	NET_StringToAdr( const char *s, netadr_t *a ) {
	if (!strcmp (s, "localhost")) {
		memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return qtrue;
	}

	a->type = NA_BAD;
	return qfalse;
}

