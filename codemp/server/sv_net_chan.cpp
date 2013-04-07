//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "server.h"

// TTimo: unused, commenting out to make gcc happy
#if 1
/*
==============
SV_Netchan_Encode

	// first four bytes of the data are always:
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Encode( client_t *client, msg_t *msg ) {
	long reliableAcknowledge, i, index;
	byte key, *string;
        int	srdc, sbit, soob;
        
	if ( msg->cursize < SV_ENCODE_START ) {
		return;
	}

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;
        
        msg->bit = 0;
        msg->readcount = 0;
        msg->oob = (qboolean)0;
        
	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = (qboolean)soob;
        msg->bit = sbit;
        msg->readcount = srdc;
        
	string = (byte *)client->lastClientCommandString;
	index = 0;
	// xor the client challenge with the netchan sequence number
	key = client->challenge ^ client->netchan.outgoingSequence;
	for (i = SV_ENCODE_START; i < msg->cursize; i++) {
		// modify the key with the last received and with this message acknowledged client command
		if (!string[index])
			index = 0;
		if (/*string[index] > 127 ||*/	// eurofix: remove this so we can chat in european languages...	-ste
			string[index] == '%') 
		{
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

/*
==============
SV_Netchan_Decode

	// first 12 bytes of the data are always:
	long serverId;
	long messageAcknowledge;
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Decode( client_t *client, msg_t *msg ) {
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, *string;

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;
        
        msg->oob = (qboolean)0;
        
        serverId = MSG_ReadLong(msg);
	messageAcknowledge = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = (qboolean)soob;
        msg->bit = sbit;
        msg->readcount = srdc;
        
	string = (byte *)client->reliableCommands[ reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ];
	index = 0;
	//
	key = client->challenge ^ serverId ^ messageAcknowledge;
	for (i = msg->readcount + SV_DECODE_START; i < msg->cursize; i++) {
		// modify the key with the last sent and acknowledged server command
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
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment( netchan_t *chan ) {
	Netchan_TransmitNextFragment( chan );
}


/*
===============
SV_Netchan_Transmit
================
*/

//extern byte chksum[65536];
void SV_Netchan_Transmit( client_t *client, msg_t *msg) {	//int length, const byte *data ) {
//	int i;
	MSG_WriteByte( msg, svc_EOF );
//	for(i=SV_ENCODE_START;i<msg->cursize;i++) {
//		chksum[i-SV_ENCODE_START] = msg->data[i];
//	}
//	Huff_Compress( msg, SV_ENCODE_START );
	SV_Netchan_Encode( client, msg );
	Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process( client_t *client, msg_t *msg ) {
	int ret;
//	int i;
	ret = Netchan_Process( &client->netchan, msg );
	if (!ret)
		return qfalse;
	SV_Netchan_Decode( client, msg );
//	Huff_Decompress( msg, SV_DECODE_START );
//	for(i=SV_DECODE_START+msg->readcount;i<msg->cursize;i++) {
//		if (msg->data[i] != chksum[i-(SV_DECODE_START+msg->readcount)]) {
//			Com_Error(ERR_DROP,"bad\n");
//		}
//	}
	return qtrue;
}

