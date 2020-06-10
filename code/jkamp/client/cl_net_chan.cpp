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

#include "client.h"

// TTimo: unused, commenting out to make gcc happy
#if 1
/*
==============
CL_Netchan_Encode

	// first 12 bytes of the data are always:
	long serverId;
	long messageAcknowledge;
	long reliableAcknowledge;

==============
*/
static void CL_Netchan_Encode( msg_t *msg ) {
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, *string;

	if ( msg->cursize <= CL_ENCODE_START ) {
		return;
	}

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;

        msg->bit = 0;
        msg->readcount = 0;
        msg->oob = (qboolean)0;

        serverId = MSG_ReadLong(msg);
	messageAcknowledge = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = (qboolean)soob;
        msg->bit = sbit;
        msg->readcount = srdc;

	string = (byte *)clc.serverCommands[ reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ];
	index = 0;
	//
	key = clc.challenge ^ serverId ^ messageAcknowledge;
	for (i = CL_ENCODE_START; i < msg->cursize; i++) {
		// modify the key with the last received now acknowledged server command
		if (!string[index])
			index = 0;
		if (/*string[index] > 127 || */	// eurofix: remove this so we can chat in european languages...	-ste
			string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->data + i) = (*(msg->data + i)) ^ key;
	}
}

/*
==============
CL_Netchan_Decode

	// first four bytes of the data are always:
	long reliableAcknowledge;

==============
*/
static void CL_Netchan_Decode( msg_t *msg ) {
	long reliableAcknowledge, i, index;
	byte key, *string;
        int	srdc, sbit, soob;

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;

        msg->oob = (qboolean)0;

	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = (qboolean)soob;
        msg->bit = sbit;
        msg->readcount = srdc;

	string = (unsigned char *)clc.reliableCommands[ reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ];
	index = 0;
	// xor the client challenge with the netchan sequence number (need something that changes every message)
	key = clc.challenge ^ LittleLong( *(unsigned *)msg->data );
	for (i = msg->readcount + CL_DECODE_START; i < msg->cursize; i++) {
		// modify the key with the last sent and with this message acknowledged client command
		if (!string[index])
			index = 0;
		if (/*string[index] > 127 || */	// eurofix: remove this so we can chat in european languages...	-ste
			string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		// decode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}
#endif

/*
=================
CL_Netchan_TransmitNextFragment
=================
*/
void CL_Netchan_TransmitNextFragment( netchan_t *chan ) {
	Netchan_TransmitNextFragment( chan );
}

//byte chksum[65536];

/*
===============
CL_Netchan_Transmit
================
*/
void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg ) {
//	int i;
	MSG_WriteByte( msg, clc_EOF );
//	for(i=CL_ENCODE_START;i<msg->cursize;i++) {
//		chksum[i-CL_ENCODE_START] = msg->data[i];
//	}

//	Huff_Compress( msg, CL_ENCODE_START );
	CL_Netchan_Encode( msg );
	Netchan_Transmit( chan, msg->cursize, msg->data );
}

extern 	int oldsize;
int newsize = 0;

/*
=================
CL_Netchan_Process
=================
*/
qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg ) {
	int ret;
//	int i;
//	static		int newsize = 0;

	ret = Netchan_Process( chan, msg );
	if (!ret)
		return qfalse;
	CL_Netchan_Decode( msg );
//	Huff_Decompress( msg, CL_DECODE_START );
//	for(i=CL_DECODE_START+msg->readcount;i<msg->cursize;i++) {
//		if (msg->data[i] != chksum[i-(CL_DECODE_START+msg->readcount)]) {
//			Com_Error(ERR_DROP,"bad %d v %d\n", msg->data[i], chksum[i-(CL_DECODE_START+msg->readcount)]);
//		}
//	}
	newsize += msg->cursize;
//	Com_Printf("saved %d to %d (%d%%)\n", (oldsize>>3), newsize, 100-(newsize*100/(oldsize>>3)));
	return qtrue;
}
