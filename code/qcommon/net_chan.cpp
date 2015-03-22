/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "q_shared.h"
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
#define MAX_LOOPDATA            16 * 1024

#if (MAX_PACKETLEN > MAX_MSGLEN)
#error MAX_PACKETLEN must be <= MAX_MSGLEN
#endif
#if (MAX_LOOPDATA > MAX_MSGLEN)
#error MAX_LOOPDATA must be <= MAX_MSGLEN
#endif

#define	FRAGMENT_SIZE			(MAX_PACKETLEN - 100)
#define	PACKET_HEADER			10			// two ints and a short

#define	FRAGMENT_BIT	(1<<31)

cvar_t		*showpackets;
cvar_t		*showdrop;
cvar_t		*qport;

static const char *netsrcString[2] = {
	"client",
	"server"
};

typedef struct {
	char loopData[MAX_LOOPDATA];
	int			get, send;
} loopback_t;

static loopback_t	*loopbacks = NULL;


/*
===============
Netchan_Init

===============
*/
void Netchan_Init( int port ) {
	if (!loopbacks)
	{
		loopbacks = (loopback_t*) Z_Malloc(sizeof(loopback_t) * 2, TAG_NEWDEL, qtrue);
	}

	port &= 0xffff;
	showpackets = Cvar_Get ("showpackets", "0", CVAR_TEMP );
	showdrop = Cvar_Get ("showdrop", "0", CVAR_TEMP );
	qport = Cvar_Get ("net_qport", va("%i", port), CVAR_INIT );
}

void Netchan_Shutdown()
{
	if (loopbacks)
	{
		Z_Free(loopbacks);
		loopbacks = 0;
	}
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
	//int			qport;
	int			fragmentStart, fragmentLength;
	qboolean	fragmented;

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
		/*qport = */MSG_ReadShort( msg );
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
			chan->fragmentLength + fragmentLength > (int)sizeof( chan->fragmentBuffer ) ) {
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

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	//If read and write positions are the same, nothing left to read.
	if (loop->get == loop->send)
		return qfalse;

	//Get read position.  Wrap if too close to end.
	i = loop->get;
	if(i > MAX_LOOPDATA - 4) {
		i = 0;
	}

	//Get length of packet.
	byteAlias_t *ba = (byteAlias_t *)&loop->loopData[i];
	const int length = ba->i;
	i += 4;

	//See if entire packet is at end of buffer or part is at the beginning.
	if(i + length <= MAX_LOOPDATA) {
		//Everything fits, full copy.
		memcpy (net_message->data, loop->loopData + i, length);
		net_message->cursize = length;
		i += length;
		loop->get = i;
	} else {
		//Doesn't all fit, partial copy
		const int copyToEnd = MAX_LOOPDATA - i;
		memcpy (net_message->data, loop->loopData + i, copyToEnd);
		memcpy ((char*)net_message->data + copyToEnd,
				loop->loopData, length - copyToEnd);
		net_message->cursize = length;
		loop->get = length - copyToEnd;
	}

	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;

	return qtrue;
}


void NET_SendLoopPacket (netsrc_t sock, int length, const void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	//Make sure there is enough free space in the buffer.
#ifdef _DEBUG
	int freeSpace;
	if(loop->send >= loop->get) {
		freeSpace = MAX_LOOPDATA - (loop->send - loop->get);
	} else {
		freeSpace = loop->get - loop->send;
	}

	assert(freeSpace > length);
#endif // _DEBUG

	//Get write position.  Wrap around if too close to end.
	i = loop->send;
	if(i > MAX_LOOPDATA - 4) {
		i = 0;
	}

	//Write length of packet.
	byteAlias_t *ba = (byteAlias_t *)&loop->loopData[i];
	ba->i = length;
	i += 4;

	//See if the whole packet will fit on the end of the buffer or if we
	//need to write part of it back at the beginning.
	if(i + length <= MAX_LOOPDATA) {
		//Everything fits, full copy.
		memcpy (loop->loopData + i, data, length);
		i += length;
		loop->send = i;
	} else {
		//Doesn't all fit, partial copy
		int copyToEnd = MAX_LOOPDATA - i;
		memcpy(loop->loopData + i, data, copyToEnd);
		memcpy(loop->loopData, (char*)data + copyToEnd, length - copyToEnd);
		loop->send = length - copyToEnd;
	}
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
	Q_vsnprintf( string+4, sizeof(string)-4, format, argptr );
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

