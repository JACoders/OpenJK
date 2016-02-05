/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// cl_parse.c  -- parse a message received from the server

#include "client.h"
#include "cl_cgameapi.h"
#include "qcommon/stringed_ingame.h"

#ifdef USE_INTERNAL_ZLIB
#include "zlib/zlib.h"
#else
#include <zlib.h>
#endif

static char hiddenCvarVal[128];

char *svc_strings[256] = {
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
	"svc_setgame",
	"svc_mapchange",
};

void SHOWNET( msg_t *msg, char *s) {
	if ( cl_shownet->integer >= 2) {
		Com_Printf ("%3i:%s\n", msg->readcount-1, s);
	}
}

/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old,
					 qboolean unchanged) {
	entityState_t	*state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES-1)];

	if ( unchanged )
	{
		*state = *old;
	}
	else
	{
		MSG_ReadDeltaEntity( msg, old, state, newnum );
	}

	if ( state->number == (MAX_GENTITIES-1) ) {
		return;		// entity was delta removed
	}
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe) {
	int			newnum;
	entityState_t	*oldstate;
	int			oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = NULL;
	if (!oldframe) {
		oldnum = 99999;
	} else {
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while ( 1 ) {
		// read the entity index number
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum == (MAX_GENTITIES-1) ) {
			break;
		}

		if ( msg->readcount > msg->cursize ) {
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: end of message");
		}

		while ( oldnum < newnum ) {
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum) {
			// delta from previous state
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum ) {
			// delta from baseline
			if ( cl_shownet->integer == 3 ) {
				Com_Printf ("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[newnum], qfalse );
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != 99999 ) {
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->integer == 3 ) {
			Com_Printf ("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

		oldindex++;

		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}


/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot( msg_t *msg ) {
	int			len;
	clSnapshot_t	*old;
	clSnapshot_t	newSnap;
	int			deltaNum;
	int			oldMessageNum;
	int			i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset (&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = clc.serverCommandSequence;

	newSnap.serverTime = MSG_ReadLong( msg );

	// if we were just unpaused, we can only *now* really let the
	// change come into effect or the client hangs.
	cl_paused->modified = qfalse;

	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;		// uncompressed frame
		old = NULL;
		clc.demowaiting = qfalse;	// we can start recording now
	} else {
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if ( !old->valid ) {
			// should never happen
			Com_Printf ("Delta from invalid frame (not supposed to happen!).\n");
			while ( ( newSnap.deltaNum & PACKET_MASK ) != ( newSnap.messageNum & PACKET_MASK ) && !old->valid ) {
				newSnap.deltaNum++;
				old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
			}
			if ( old->valid ) {
				Com_Printf ("Found more recent frame to delta from.\n");
			}
		}
		if ( !old->valid ) {
			Com_Printf ("Failed to find more recent frame to delta from.\n");
		} else if ( old->messageNum != newSnap.deltaNum ) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf ("Delta frame too old.\n");
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
			Com_DPrintf ("Delta parseEntitiesNum too old.\n");
		} else {
			newSnap.valid = qtrue;	// valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if((unsigned)len > sizeof(newSnap.areamask))
	{
		Com_Error (ERR_DROP,"CL_ParseSnapshot: Invalid size %d for areamask", len);
		return;
	}

	MSG_ReadData( msg, &newSnap.areamask, len);

	// read playerinfo
	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
		if (newSnap.ps.m_iVehicleNum)
		{ //this means we must have written our vehicle's ps too
			MSG_ReadDeltaPlayerstate( msg, &old->vps, &newSnap.vps, qtrue );
		}
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
		if (newSnap.ps.m_iVehicleNum)
		{ //this means we must have written our vehicle's ps too
			MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.vps, qtrue );
		}
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid ) {
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}
	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ ) {
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) {
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if (cl_shownet->integer == 3) {
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
		cl.snap.deltaNum, cl.snap.ping );
	}

	cl.newSnapshots = qtrue;
}


/*
================
CL_ParseSetGame

rww - Update fs_game, this message is so we can use the ext_data
*_overrides.txt files for mods.
================
*/
void MSG_CheckNETFPSFOverrides(qboolean psfOverrides);
void FS_UpdateGamedir(void);
void CL_ParseSetGame( msg_t *msg )
{
	char newGameDir[MAX_QPATH];
	int i = 0;
	char next;

	while (i < MAX_QPATH)
	{
		next = MSG_ReadByte( msg );

		if (next)
		{ //if next is 0 then we have finished reading to the end of the message
			newGameDir[i] = next;
		}
		else
		{
			break;
		}
		i++;
	}
	newGameDir[i] = 0;

	if(FS_CheckDirTraversal(newGameDir))
	{
		Com_Printf(S_COLOR_YELLOW "WARNING: Server sent invalid fs_game value %s\n", newGameDir);
		return;
	}

	if(!FS_FilenameCompare(newGameDir, BASEGAME))
		Cvar_Set("fs_game", "");
	else
		Cvar_Set("fs_game", newGameDir);

	if(!(Cvar_Flags("fs_game") & CVAR_MODIFIED))
		return;

	//Update the search path for the mod dir
	FS_UpdateGamedir();

	//Now update the overrides manually
	MSG_CheckNETFPSFOverrides(qfalse);
	MSG_CheckNETFPSFOverrides(qtrue);
}


//=====================================================================

int cl_connectedToPureServer;
int cl_connectedToCheatServer;

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_SystemInfoChanged( void ) {
	char			*systemInfo;
	const char		*s, *t;
	char			key[BIG_INFO_KEY];
	char			value[BIG_INFO_VALUE];
	qboolean		gameSet;

	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SYSTEMINFO ];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this new serverId
	cl.serverId = atoi( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	// don't set any vars when playing a demo
	if ( clc.demoplaying ) {
		return;
	}

	s = Info_ValueForKey( systemInfo, "sv_cheats" );
	cl_connectedToCheatServer = atoi( s );
	if ( !cl_connectedToCheatServer )
	{
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey( systemInfo, "sv_paks" );
	t = Info_ValueForKey( systemInfo, "sv_pakNames" );
	FS_PureServerSetLoadedPaks( s, t );

	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	FS_PureServerSetReferencedPaks( s, t );

	gameSet = qfalse;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while ( s ) {
		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}
		// ehw!
		if ( !Q_stricmp( key, "fs_game" ) ) {
			if(FS_CheckDirTraversal(value))
			{
				Com_Printf(S_COLOR_YELLOW "WARNING: Server sent invalid fs_game value %s\n", value);
				continue;
			}

			if(!FS_FilenameCompare(value, BASEGAME))
			{
				Q_strncpyz(value, "", sizeof(value));
			}

			gameSet = qtrue;
		}
		Cvar_Server_Set( key, value );
	}
	// if game folder should not be set and it is set at the client side
	if ( !gameSet && *Cvar_VariableString("fs_game") ) {
		Cvar_Set( "fs_game", "" );
	}
	cl_connectedToPureServer = Cvar_VariableValue( "sv_pure" );
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate( msg_t *msg ) {
	int				i;
	entityState_t	*es;
	int				newnum;
	entityState_t	nullstate;
	int				cmd;
	char			*s;

	Con_Close();

	clc.connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	cl.gameState.dataCount = 1;	// leave a 0 at the beginning for uninitialized configstrings
	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			int		len, start;

			start = msg->readcount;

			i = MSG_ReadShort( msg );
			if ( i < 0 || i >= MAX_CONFIGSTRINGS ) {
				Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
			}
			s = MSG_ReadBigString( msg );

			if (cl_shownet->integer >= 2)
			{
				Com_Printf("%3i: %d: %s\n", start, i, s);
			}

			/*
			if (i == CS_SERVERINFO)
			{ //get the special value here
				char *f = strstr(s, "g_debugMelee");
				if (f)
				{
					while (*f && *f != '\\')
					{ //find the \ after it
						f++;
					}
					if (*f == '\\')
					{ //got it
						int i = 0;

						f++;
						while (*f && *f != '\\' && i < 128)
						{
							hiddenCvarVal[i] = *f;
							i++;
							f++;
						}
						hiddenCvarVal[i] = 0;

						//resume here
						s = f;
					}
				}
			}
			*/

			len = strlen( s );

			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
				Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
			}

			// append it to the gameState string buffer
			cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "Baseline number out of range: %i", newnum );
			}
			Com_Memset (&nullstate, 0, sizeof(nullstate));
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		} else {
			Com_Error( ERR_DROP, "CL_ParseGamestate: bad command byte" );
		}
	}

	clc.clientNum = MSG_ReadLong(msg);
	// read the checksum feed
	clc.checksumFeed = MSG_ReadLong( msg );

	// Throw away the info for the old RMG system.
	MSG_ReadShort (msg);


	// parse serverId and other cvars
	CL_SystemInfoChanged();

	// reinitialize the filesystem if the game directory has changed
	if( FS_ConditionalRestart( clc.checksumFeed ) ) {
		// don't set to true because we yet have to start downloading
		// enabling this can cause double loading of a map when connecting to
		// a server which has a different game directory set
		//clc.downloadRestart = qtrue;
	}

	// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
	// cgame
	CL_InitDownloads();

	// make sure the game starts
	Cvar_Set( "cl_paused", "0" );
}


//=====================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload ( msg_t *msg ) {
	int		size;
	unsigned char data[MAX_MSGLEN];
	uint16_t block;

	if (!*clc.downloadTempName) {
		Com_Printf("Server sending download, but no download was requested\n");
		CL_AddReliableCommand("stopdl", qfalse);
		return;
	}

	// read the data
	block = MSG_ReadShort ( msg );

	if ( !block && !clc.downloadBlock )
	{
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong ( msg );

		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if (clc.downloadSize < 0)
		{
			Com_Error(ERR_DROP, "%s", MSG_ReadString( msg ) );
			return;
		}
	}

	size = /*(unsigned short)*/MSG_ReadShort ( msg );
	if (size < 0 || size > (int)sizeof(data))
	{
		Com_Error(ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk", size);
		return;
	}

	MSG_ReadData( msg, data, size );

	if((clc.downloadBlock & 0xFFFF) != block)
	{
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n", (clc.downloadBlock & 0xFFFF), block);
		return;
	}

	// open the file if not opened yet
	if (!clc.download)
	{
		clc.download = FS_SV_FOpenFileWrite( clc.downloadTempName );

		if (!clc.download) {
			Com_Printf( "Could not create %s\n", clc.downloadTempName );
			CL_AddReliableCommand( "stopdl", qfalse );
			CL_NextDownload();
			return;
		}
	}

	if (size)
		FS_Write( data, size, clc.download );

	CL_AddReliableCommand( va("nextdl %d", clc.downloadBlock), qfalse );
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );

	if (!size) { // A zero length block means EOF
		if (clc.download) {
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename ( clc.downloadTempName, clc.downloadName, qfalse );
		}

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload ();
	}
}

int CL_GetValueForHidden(const char *s)
{ //string arg here just in case I want to add more sometime and make a lookup table
	return atoi(hiddenCvarVal);
}

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString( msg_t *msg ) {
	char	*s;
	int		seq;
	int		index;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed stored it off
	if ( clc.serverCommandSequence >= seq ) {
		return;
	}
	clc.serverCommandSequence = seq;

	index = seq & (MAX_RELIABLE_COMMANDS-1);
	/*
	if (s[0] == 'c' && s[1] == 's' && s[2] == ' ' && s[3] == '0' && s[4] == ' ')
	{ //yes.. we seem to have an incoming server info.
		char *f = strstr(s, "g_debugMelee");
		if (f)
		{
			while (*f && *f != '\\')
			{ //find the \ after it
				f++;
			}
			if (*f == '\\')
			{ //got it
				int i = 0;

				f++;
				while (*f && *f != '\\' && i < 128)
				{
					hiddenCvarVal[i] = *f;
					i++;
					f++;
				}
				hiddenCvarVal[i] = 0;

				//don't worry about backing over beginning of string I guess,
				//we already know we successfully strstr'd the initial string
				//which exceeds this length.
				//MSG_ReadString appears to just return a static buffer so I
				//can stomp over its contents safely.
				f--;
				*f = '\"';
				f--;
				*f = ' ';
				f--;
				*f = '0';
				f--;
				*f = ' ';
				f--;
				*f = 's';
				f--;
				*f = 'c';

				//the normal configstring gets to start here...
				s = f;
			}
		}
	}
	*/
	Q_strncpyz( clc.serverCommands[ index ], s, sizeof( clc.serverCommands[ index ] ) );
}


/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( msg_t *msg ) {
	int			cmd;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf ("%i ",msg->cursize);
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf ("------------------\n");
	}

	MSG_Bitstream(msg);

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong( msg );
	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	//
	// parse the message
	//
	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			Com_Error (ERR_DROP,"CL_ParseServerMessage: read past end of server message");
			break;
		}

		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF) {
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( !svc_strings[cmd] ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount-1, cmd );
			} else {
				SHOWNET( msg, svc_strings[cmd] );
			}
		}

	// other commands
		switch ( cmd ) {
		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;
		case svc_nop:
			break;
		case svc_serverCommand:
			CL_ParseCommandString( msg );
			break;
		case svc_gamestate:
			CL_ParseGamestate( msg );
			break;
		case svc_snapshot:
			CL_ParseSnapshot( msg );
			break;
		case svc_setgame:
			CL_ParseSetGame( msg );
			break;
		case svc_download:
			CL_ParseDownload( msg );
			break;
		case svc_mapchange:
			if ( cls.cgameStarted )
				CGVM_MapChange();
			break;
		}
	}
}
