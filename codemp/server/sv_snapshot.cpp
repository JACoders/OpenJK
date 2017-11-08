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

#include "server.h"
#include "qcommon/cm_public.h"

/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4	sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1	svc_snapshot
4	last client reliable command
4	serverTime
1	lastframe for delta compression
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entityState_t list to the message.
=============
*/
static void SV_EmitPacketEntities( clientSnapshot_t *from, clientSnapshot_t *to, msg_t *msg ) {
	entityState_t	*oldent, *newent;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		from_num_entities;

	// generate the delta update
	if ( !from ) {
		from_num_entities = 0;
	} else {
		from_num_entities = from->num_entities;
	}

	newent = NULL;
	oldent = NULL;
	newindex = 0;
	oldindex = 0;
	while ( newindex < to->num_entities || oldindex < from_num_entities ) {
		if ( newindex >= to->num_entities ) {
			newnum = 9999;
		} else {
			newent = &svs.snapshotEntities[(to->first_entity+newindex) % svs.numSnapshotEntities];
			newnum = newent->number;
		}

		if ( oldindex >= from_num_entities ) {
			oldnum = 9999;
		} else {
			oldent = &svs.snapshotEntities[(from->first_entity+oldindex) % svs.numSnapshotEntities];
			oldnum = oldent->number;
		}

		if ( newnum == oldnum ) {
			// delta update from old position
			// because the force parm is qfalse, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteDeltaEntity (msg, oldent, newent, qfalse );
			oldindex++;
			newindex++;
			continue;
		}

		if ( newnum < oldnum ) {
			// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity (msg, &sv.svEntities[newnum].baseline, newent, qtrue );
			newindex++;
			continue;
		}

		if ( newnum > oldnum ) {
			// the old entity isn't present in the new message
			MSG_WriteDeltaEntity (msg, oldent, NULL, qtrue );
			oldindex++;
			continue;
		}
	}

	MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );	// end of packetentities
}



/*
==================
SV_WriteSnapshotToClient
==================
*/
static void SV_WriteSnapshotToClient( client_t *client, msg_t *msg ) {
	clientSnapshot_t	*frame, *oldframe;
	int					lastframe;
	int					i;
	int					snapFlags;
	int					deltaMessage;

	// this is the snapshot we are creating
	frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

	// bots never acknowledge, but it doesn't matter since the only use case is for serverside demos
	// in which case we can delta against the very last message every time
	deltaMessage = client->deltaMessage;
	if ( client->demo.isBot ) {
		client->deltaMessage = client->netchan.outgoingSequence;
	}

	// try to use a previous frame as the source for delta compressing the snapshot
	if ( deltaMessage <= 0 || client->state != CS_ACTIVE ) {
		// client is asking for a retransmit
		oldframe = NULL;
		lastframe = 0;
	} else if ( client->netchan.outgoingSequence - deltaMessage
		>= (PACKET_BACKUP - 3) ) {
		// client hasn't gotten a good message through in a long time
		Com_DPrintf ("%s: Delta request from out of date packet.\n", client->name);
		oldframe = NULL;
		lastframe = 0;
	} else if ( client->demo.demorecording && client->demo.demowaiting ) {
		// demo is waiting for a non-delta-compressed frame for this client, so don't delta compress
		oldframe = NULL;
		lastframe = 0;
	} else if ( client->demo.minDeltaFrame > deltaMessage ) {
		// we saved a non-delta frame to the demo and sent it to the client, but the client didn't ack it
		// we can't delta against an old frame that's not in the demo without breaking the demo.  so send
		// non-delta frames until the client acks.
		oldframe = NULL;
		lastframe = 0;
	} else {
		// we have a valid snapshot to delta from
		oldframe = &client->frames[ deltaMessage & PACKET_MASK ];
		lastframe = client->netchan.outgoingSequence - deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if ( oldframe->first_entity <= svs.nextSnapshotEntities - svs.numSnapshotEntities ) {
			Com_DPrintf ("%s: Delta request from out of date entities.\n", client->name);
			oldframe = NULL;
			lastframe = 0;
		}
	}

	if ( oldframe == NULL ) {
		if ( client->demo.demowaiting ) {
			// this is a non-delta frame, so we can delta against it in the demo
			client->demo.minDeltaFrame = client->netchan.outgoingSequence;
		}
		client->demo.demowaiting = qfalse;
	}

	MSG_WriteByte (msg, svc_snapshot);

	// NOTE, MRE: now sent at the start of every message from server to client
	// let the client know which reliable clientCommands we have received
	//MSG_WriteLong( msg, client->lastClientCommand );

	// send over the current server time so the client can drift
	// its view of time to try to match
	if( client->oldServerTime &&
		!( client->demo.demorecording && client->demo.isBot ) ) {
		// The server has not yet got an acknowledgement of the
		// new gamestate from this client, so continue to send it
		// a time as if the server has not restarted. Note from
		// the client's perspective this time is strictly speaking
		// incorrect, but since it'll be busy loading a map at
		// the time it doesn't really matter.
		MSG_WriteLong (msg, sv.time + client->oldServerTime);
	} else {
		MSG_WriteLong (msg, sv.time);
	}

	// what we are delta'ing from
	MSG_WriteByte (msg, lastframe);

	snapFlags = svs.snapFlagServerBit;
	if ( client->rateDelayed ) {
		snapFlags |= SNAPFLAG_RATE_DELAYED;
	}
	if ( client->state != CS_ACTIVE ) {
		snapFlags |= SNAPFLAG_NOT_ACTIVE;
	}

	MSG_WriteByte (msg, snapFlags);

	// send over the areabits
	MSG_WriteByte (msg, frame->areabytes);
	MSG_WriteData (msg, frame->areabits, frame->areabytes);

	// delta encode the playerstate
	if ( oldframe ) {
#ifdef _ONEBIT_COMBO
		MSG_WriteDeltaPlayerstate( msg, &oldframe->ps, &frame->ps, frame->pDeltaOneBit, frame->pDeltaNumBit );
#else
		MSG_WriteDeltaPlayerstate( msg, &oldframe->ps, &frame->ps );
#endif
		if (frame->ps.m_iVehicleNum)
		{ //then write the vehicle's playerstate too
			if (!oldframe->ps.m_iVehicleNum)
			{ //if last frame didn't have vehicle, then the old vps isn't gonna delta
				//properly (because our vps on the client could be anything)
#ifdef _ONEBIT_COMBO
				MSG_WriteDeltaPlayerstate( msg, NULL, &frame->vps, NULL, NULL, qtrue );
#else
				MSG_WriteDeltaPlayerstate( msg, NULL, &frame->vps, qtrue );
#endif
			}
			else
			{
#ifdef _ONEBIT_COMBO
				MSG_WriteDeltaPlayerstate( msg, &oldframe->vps, &frame->vps, frame->pDeltaOneBitVeh, frame->pDeltaNumBitVeh, qtrue );
#else
				MSG_WriteDeltaPlayerstate( msg, &oldframe->vps, &frame->vps, qtrue );
#endif
			}
		}
	} else {
#ifdef _ONEBIT_COMBO
		MSG_WriteDeltaPlayerstate( msg, NULL, &frame->ps, NULL, NULL );
#else
		MSG_WriteDeltaPlayerstate( msg, NULL, &frame->ps );
#endif
		if (frame->ps.m_iVehicleNum)
		{ //then write the vehicle's playerstate too
#ifdef _ONEBIT_COMBO
			MSG_WriteDeltaPlayerstate( msg, NULL, &frame->vps, NULL, NULL, qtrue );
#else
			MSG_WriteDeltaPlayerstate( msg, NULL, &frame->vps, qtrue );
#endif
		}
	}

	// delta encode the entities
	SV_EmitPacketEntities (oldframe, frame, msg);

	// padding for rate debugging
	if ( sv_padPackets->integer ) {
		for ( i = 0 ; i < sv_padPackets->integer ; i++ ) {
			MSG_WriteByte (msg, svc_nop);
		}
	}
}


/*
==================
SV_UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
void SV_UpdateServerCommandsToClient( client_t *client, msg_t *msg ) {
	int		i;
	int		reliableAcknowledge;

	if ( client->demo.isBot && client->demo.demorecording ) {
		reliableAcknowledge = client->demo.botReliableAcknowledge;
	} else {
		reliableAcknowledge = client->reliableAcknowledge;
	}

	// write any unacknowledged serverCommands
	for ( i = reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
		MSG_WriteByte( msg, svc_serverCommand );
		MSG_WriteLong( msg, i );
		MSG_WriteString( msg, client->reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}
	client->reliableSent = client->reliableSequence;
}

/*
=============================================================================

Build a client snapshot structure

=============================================================================
*/

typedef struct snapshotEntityNumbers_s {
	int		numSnapshotEntities;
	int		snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int QDECL SV_QsortEntityNumbers( const void *a, const void *b ) {
	int	*ea, *eb;

	ea = (int *)a;
	eb = (int *)b;

	if ( *ea == *eb ) {
		Com_Error( ERR_DROP, "SV_QsortEntityStates: duplicated entity" );
	}

	if ( *ea < *eb ) {
		return -1;
	}

	return 1;
}


/*
===============
SV_AddEntToSnapshot
===============
*/
static void SV_AddEntToSnapshot( svEntity_t *svEnt, sharedEntity_t *gEnt, snapshotEntityNumbers_t *eNums ) {
	// if we have already added this entity to this snapshot, don't add again
	if ( svEnt->snapshotCounter == sv.snapshotCounter ) {
		return;
	}
	svEnt->snapshotCounter = sv.snapshotCounter;

	// if we are full, silently discard entities
	if ( eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES ) {
		return;
	}

	eNums->snapshotEntities[ eNums->numSnapshotEntities ] = gEnt->s.number;
	eNums->numSnapshotEntities++;
}

/*
===============
SV_AddEntitiesVisibleFromPoint
===============
*/
float g_svCullDist = -1.0f;
static void SV_AddEntitiesVisibleFromPoint( vec3_t origin, clientSnapshot_t *frame,
									snapshotEntityNumbers_t *eNums, qboolean portal ) {
	int		e, i;
	sharedEntity_t *ent;
	svEntity_t	*svEnt;
	int		l;
	int		clientarea, clientcluster;
	int		leafnum;
	byte	*clientpvs;
	byte	*bitvector;
	vec3_t	difference;
	float	length, radius;

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if ( !sv.state ) {
		return;
	}

	leafnum = CM_PointLeafnum (origin);
	clientarea = CM_LeafArea (leafnum);
	clientcluster = CM_LeafCluster (leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits( frame->areabits, clientarea );

	clientpvs = CM_ClusterPVS (clientcluster);

	for ( e = 0 ; e < sv.num_entities ; e++ ) {
		ent = SV_GentityNum(e);

		// never send entities that aren't linked in
		if ( !ent->r.linked ) {
			continue;
		}

		if (ent->s.eFlags & EF_PERMANENT)
		{	// he's permanent, so don't send him down!
			continue;
		}

		if (ent->s.number != e) {
			Com_DPrintf ("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}

		// entities can be flagged to explicitly not be sent to the client
		if ( ent->r.svFlags & SVF_NOCLIENT ) {
			continue;
		}

		// entities can be flagged to be sent to only one client
		if ( ent->r.svFlags & SVF_SINGLECLIENT ) {
			if ( ent->r.singleClient != frame->ps.clientNum ) {
				continue;
			}
		}
		// entities can be flagged to be sent to everyone but one client
		if ( ent->r.svFlags & SVF_NOTSINGLECLIENT ) {
			if ( ent->r.singleClient == frame->ps.clientNum ) {
				continue;
			}
		}

		svEnt = SV_SvEntityForGentity( ent );

		// don't double add an entity through portals
		if ( svEnt->snapshotCounter == sv.snapshotCounter ) {
			continue;
		}

		// entities can request not to be sent to certain clients (NOTE: always send to ourselves)
		if ( e != frame->ps.clientNum && (ent->r.svFlags & SVF_BROADCASTCLIENTS)
			&& !(ent->r.broadcastClients[frame->ps.clientNum/32] & (1 << (frame->ps.clientNum % 32))) )
		{
			continue;
		}
		// broadcast entities are always sent, and so is the main player so we don't see noclip weirdness
		if ( (ent->r.svFlags & SVF_BROADCAST) || e == frame->ps.clientNum
			|| (ent->r.broadcastClients[frame->ps.clientNum/32] & (1 << (frame->ps.clientNum % 32))) )
		{
			SV_AddEntToSnapshot( svEnt, ent, eNums );
			continue;
		}

		if (ent->s.isPortalEnt)
		{ //rww - portal entities are always sent as well
			SV_AddEntToSnapshot( svEnt, ent, eNums );
			continue;
		}

		// ignore if not touching a PV leaf
		// check area
		if ( !CM_AreasConnected( clientarea, svEnt->areanum ) ) {
			// doors can legally straddle two areas, so
			// we may need to check another one
			if ( !CM_AreasConnected( clientarea, svEnt->areanum2 ) ) {
				continue;		// blocked by a door
			}
		}

		bitvector = clientpvs;

		// check individual leafs
		if ( !svEnt->numClusters ) {
			continue;
		}
		l = 0;
		for ( i=0 ; i < svEnt->numClusters ; i++ ) {
			l = svEnt->clusternums[i];
			if ( bitvector[l >> 3] & (1 << (l&7) ) ) {
				break;
			}
		}

		// if we haven't found it to be visible,
		// check overflow clusters that coudln't be stored
		if ( i == svEnt->numClusters ) {
			if ( svEnt->lastCluster ) {
				for ( ; l <= svEnt->lastCluster ; l++ ) {
					if ( bitvector[l >> 3] & (1 << (l&7) ) ) {
						break;
					}
				}
				if ( l == svEnt->lastCluster ) {
					continue;	// not visible
				}
			} else {
				continue;
			}
		}

		if (g_svCullDist != -1.0f)
		{ //do a distance cull check
			VectorAdd(ent->r.absmax, ent->r.absmin, difference);
			VectorScale(difference, 0.5f, difference);
			VectorSubtract(origin, difference, difference);
			length = VectorLength(difference);

			// calculate the diameter
			VectorSubtract(ent->r.absmax, ent->r.absmin, difference);
			radius = VectorLength(difference);
			if (length-radius >= g_svCullDist)
			{ //then don't add it
				continue;
			}
		}

		// add it
		SV_AddEntToSnapshot( svEnt, ent, eNums );

		// if its a portal entity, add everything visible from its camera position
		if ( ent->r.svFlags & SVF_PORTAL ) {
			if ( ent->s.generic1 ) {
				vec3_t dir;
				VectorSubtract(ent->s.origin, origin, dir);
				if ( VectorLengthSquared(dir) > (float) ent->s.generic1 * ent->s.generic1 ) {
					continue;
				}
			}
			SV_AddEntitiesVisibleFromPoint( ent->s.origin2, frame, eNums, qtrue );
		}
	}
}

/*
=============
SV_BuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, client can be something other than client->gentity
=============
*/
static void SV_BuildClientSnapshot( client_t *client ) {
	vec3_t						org;
	clientSnapshot_t			*frame;
	snapshotEntityNumbers_t		entityNumbers;
	int							i;
	sharedEntity_t				*ent;
	entityState_t				*state;
	svEntity_t					*svEnt;
	sharedEntity_t				*clent;
	playerState_t				*ps;

	// bump the counter used to prevent double adding
	sv.snapshotCounter++;

	// this is the frame we are creating
	frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

	// clear everything in this snapshot
	entityNumbers.numSnapshotEntities = 0;
	Com_Memset( frame->areabits, 0, sizeof( frame->areabits ) );

	frame->num_entities = 0;

	clent = client->gentity;
	if ( !clent || client->state == CS_ZOMBIE ) {
		return;
	}

	// grab the current playerState_t
	ps = SV_GameClientNum( client - svs.clients );
	frame->ps = *ps;
#ifdef _ONEBIT_COMBO
	frame->pDeltaOneBit = &ps->deltaOneBits;
	frame->pDeltaNumBit = &ps->deltaNumBits;
#endif

	if (ps->m_iVehicleNum)
	{ //get the vehicle's playerstate too then
		sharedEntity_t *veh = SV_GentityNum(ps->m_iVehicleNum);

		if (veh && veh->playerState)
		{ //Now VMA it and we've got ourselves a playerState
			playerState_t *vps = ((playerState_t *)VM_ArgPtr((intptr_t)veh->playerState));

            frame->vps = *vps;
#ifdef _ONEBIT_COMBO
			frame->pDeltaOneBitVeh = &vps->deltaOneBits;
			frame->pDeltaNumBitVeh = &vps->deltaNumBits;
#endif
		}
	}

	int							clientNum;
	// never send client's own entity, because it can
	// be regenerated from the playerstate
	clientNum = frame->ps.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	svEnt = &sv.svEntities[ clientNum ];
	svEnt->snapshotCounter = sv.snapshotCounter;


	// find the client's viewpoint
	VectorCopy( ps->origin, org );
	org[2] += ps->viewheight;

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SV_AddEntitiesVisibleFromPoint( org, frame, &entityNumbers, qfalse );

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort( entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
		sizeof( entityNumbers.snapshotEntities[0] ), SV_QsortEntityNumbers );

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for ( i = 0 ; i < MAX_MAP_AREA_BYTES/4 ; i++ ) {
		((int *)frame->areabits)[i] = ((int *)frame->areabits)[i] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.nextSnapshotEntities;
	for ( i = 0 ; i < entityNumbers.numSnapshotEntities ; i++ ) {
		ent = SV_GentityNum(entityNumbers.snapshotEntities[i]);
		state = &svs.snapshotEntities[svs.nextSnapshotEntities % svs.numSnapshotEntities];
		*state = ent->s;
		svs.nextSnapshotEntities++;
		// this should never hit, map should always be restarted first in SV_Frame
		if ( svs.nextSnapshotEntities >= 0x7FFFFFFE ) {
			Com_Error(ERR_FATAL, "svs.nextSnapshotEntities wrapped");
		}
		frame->num_entities++;
	}
}


/*
====================
SV_RateMsec

Return the number of msec a given size message is supposed
to take to clear, based on the current rate
====================
*/
#define	HEADER_RATE_BYTES	48		// include our header, IP header, and some overhead
static int SV_RateMsec( client_t *client, int messageSize ) {
	int		rate;
	int		rateMsec;

	// individual messages will never be larger than fragment size
	if ( messageSize > 1500 ) {
		messageSize = 1500;
	}
	rate = client->rate;
	if ( sv_maxRate->integer ) {
		if ( sv_maxRate->integer < 1000 ) {
			Cvar_Set( "sv_MaxRate", "1000" );
		}
		if ( sv_maxRate->integer < rate ) {
			rate = sv_maxRate->integer;
		}
	}
	if ( sv_minRate->integer ) {
		if ( sv_minRate->integer < 1000 ) {
			Cvar_Set( "sv_minRate", "1000" );
		}
		if ( sv_minRate->integer > rate ) {
			rate = sv_minRate->integer;
		}
	}

	rateMsec = ( messageSize + HEADER_RATE_BYTES ) * 1000 / ((int) (rate * com_timescale->value));

	return rateMsec;
}

extern void SV_WriteDemoMessage ( client_t *cl, msg_t *msg, int headerBytes );
/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void SV_SendMessageToClient( msg_t *msg, client_t *client ) {
	int			rateMsec;

	// MW - my attempt to fix illegible server message errors caused by
	// packet fragmentation of initial snapshot.
	while(client->state&&client->netchan.unsentFragments)
	{
		// send additional message fragments if the last message
		// was too large to send at once
		Com_Printf ("[ISM]SV_SendClientGameState() [1] for %s, writing out old fragments\n", client->name);
		SV_Netchan_TransmitNextFragment(&client->netchan);
	}

	// record information about the message
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg->cursize;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

	// save the message to demo.  this must happen before sending over network as that encodes the backing databuf
	if ( client->demo.demorecording && !client->demo.demowaiting ) {
		msg_t msgcopy = *msg;
		MSG_WriteByte( &msgcopy, svc_EOF );
		SV_WriteDemoMessage( client, &msgcopy, 0 );
	}

	// bots need to have their snapshots built, but
	// they query them directly without needing to be sent
	if ( client->demo.isBot ) {
		client->netchan.outgoingSequence++;
		client->demo.botReliableAcknowledge = client->reliableSent;
		return;
	}

	// send the datagram
	SV_Netchan_Transmit( client, msg );	//msg->cursize, msg->data );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every server frame
	// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=491
	// added sv_lanForceRate check
	if ( client->netchan.remoteAddress.type == NA_LOOPBACK || (sv_lanForceRate->integer && Sys_IsLANAddress (client->netchan.remoteAddress)) ) {
		client->nextSnapshotTime = svs.time + ((int) (1000.0 / sv_fps->integer * com_timescale->value));
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = SV_RateMsec( client, msg->cursize );

	if ( rateMsec < client->snapshotMsec ) {
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->snapshotMsec;
		client->rateDelayed = qfalse;
	} else {
		client->rateDelayed = qtrue;
	}

	client->nextSnapshotTime = svs.time + ((int) (rateMsec * com_timescale->value));

	// don't pile up empty snapshots while connecting
	if ( client->state != CS_ACTIVE ) {
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if ( !*client->downloadName && client->nextSnapshotTime < svs.time + ((int) (1000.0 * com_timescale->value)) ) {
			client->nextSnapshotTime = svs.time + ((int) (1000 * com_timescale->value));
		}
	}
}


/*
=======================
SV_SendClientSnapshot

Also called by SV_FinalMessage

=======================
*/
extern cvar_t	*fs_gamedirvar;
void SV_SendClientSnapshot( client_t *client ) {
	byte		msg_buf[MAX_MSGLEN];
	msg_t		msg;

	if (!client->sentGamedir)
	{ //rww - if this is the case then make sure there is an svc_setgame sent before this snap
		int i = 0;

		MSG_Init (&msg, msg_buf, sizeof(msg_buf));

		//have to include this for each message.
		MSG_WriteLong( &msg, client->lastClientCommand );

		MSG_WriteByte (&msg, svc_setgame);

		const char *gamedir = FS_GetCurrentGameDir(true);

		while (gamedir[i])
		{
			MSG_WriteByte(&msg, gamedir[i]);
			i++;
		}
		MSG_WriteByte(&msg, 0);

		// MW - my attempt to fix illegible server message errors caused by
		// packet fragmentation of initial snapshot.
		//rww - reusing this code here
		while(client->state&&client->netchan.unsentFragments)
		{
			// send additional message fragments if the last message
			// was too large to send at once
			Com_Printf ("[ISM]SV_SendClientGameState() [1] for %s, writing out old fragments\n", client->name);
			SV_Netchan_TransmitNextFragment(&client->netchan);
		}

		// record information about the message
		client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = msg.cursize;
		client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
		client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

		// send the datagram
		SV_Netchan_Transmit( client, &msg );	//msg->cursize, msg->data );

		client->sentGamedir = qtrue;
	}

	// build the snapshot
	SV_BuildClientSnapshot( client );

	if ( sv_autoDemo->integer && !client->demo.demorecording ) {
		if ( client->netchan.remoteAddress.type != NA_BOT || sv_autoDemoBots->integer ) {
			SV_BeginAutoRecordDemos();
		}
	}

	// bots need to have their snapshots built, but
	// they query them directly without needing to be sent
	if ( client->netchan.remoteAddress.type == NA_BOT && !client->demo.demorecording ) {
		return;
	}

	MSG_Init (&msg, msg_buf, sizeof(msg_buf));
	msg.allowoverflow = qtrue;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient( client, &msg );

	// send over all the relevant entityState_t
	// and the playerState_t
	SV_WriteSnapshotToClient( client, &msg );

	// Add any download data if the client is downloading
	SV_WriteDownloadToClient( client, &msg );

	// check for overflow
	if ( msg.overflowed ) {
		Com_Printf ("WARNING: msg overflowed for %s\n", client->name);
		MSG_Clear (&msg);
	}

	SV_SendMessageToClient( &msg, client );
}


/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages( void ) {
	int			i;
	client_t	*c;

	// send a message to each connected client
	for (i=0, c = svs.clients ; i < sv_maxclients->integer ; i++, c++) {
		if (!c->state) {
			continue;		// not connected
		}

		if ( svs.time < c->nextSnapshotTime ) {
			continue;		// not time yet
		}

		// send additional message fragments if the last message
		// was too large to send at once
		if ( c->netchan.unsentFragments ) {
			c->nextSnapshotTime = svs.time +
				SV_RateMsec( c, c->netchan.unsentLength - c->netchan.unsentFragmentStart );
			SV_Netchan_TransmitNextFragment( &c->netchan );
			continue;
		}

		// generate and send a new message
		SV_SendClientSnapshot( c );
	}
}

