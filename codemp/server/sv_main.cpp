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

#include "server.h"

#include "ghoul2/ghoul2_shared.h"
#include "sv_gameapi.h"

serverStatic_t	svs;				// persistant server info
server_t		sv;					// local server

cvar_t	*sv_snapsMin;			// minimum snapshots/sec a client can request, also limited by sv_snapsMax
cvar_t	*sv_snapsMax;			// maximum snapshots/sec a client can request, also limited by sv_fps
cvar_t	*sv_snapsPolicy;		// 0-2
cvar_t	*sv_fps = NULL;				// time rate for running non-clients
cvar_t	*sv_timeout;			// seconds without any message
cvar_t	*sv_zombietime;			// seconds to sink messages after disconnect
cvar_t	*sv_rconPassword;		// password for remote server commands
cvar_t	*sv_privatePassword;	// password for the privateClient slots
cvar_t	*sv_maxclients;
cvar_t	*sv_privateClients;		// number of clients reserved for password
cvar_t	*sv_hostname;
cvar_t	*sv_allowDownload;
cvar_t	*sv_master[MAX_MASTER_SERVERS];		// master server ip address
cvar_t	*sv_reconnectlimit;		// minimum seconds between connect messages
cvar_t	*sv_showghoultraces;	// report ghoul2 traces
cvar_t	*sv_showloss;			// report when usercmds are lost
cvar_t	*sv_padPackets;			// add nop bytes to messages
cvar_t	*sv_killserver;			// menu system can set to 1 to shut server down
cvar_t	*sv_mapname;
cvar_t	*sv_mapChecksum;
cvar_t	*sv_serverid;
cvar_t	*sv_ratePolicy;		// 1-2
cvar_t	*sv_clientRate;
cvar_t	*sv_minRate;
cvar_t	*sv_maxRate;
cvar_t	*sv_minPing;
cvar_t	*sv_maxPing;
cvar_t	*sv_gametype;
cvar_t	*sv_pure;
cvar_t	*sv_floodProtect;
cvar_t	*sv_floodProtectSlow;
cvar_t	*sv_lanForceRate; // dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
cvar_t	*sv_needpass;
cvar_t	*sv_filterCommands; // strict filtering on commands (1: strip ['\r', '\n'], 2: also strip ';')
cvar_t	*sv_autoDemo;
cvar_t	*sv_autoDemoBots;
cvar_t	*sv_autoDemoMaxMaps;
cvar_t	*sv_legacyFixes;
cvar_t	*sv_banFile;
cvar_t	*sv_maxOOBRate;
cvar_t	*sv_maxOOBRateIP;
cvar_t	*sv_autoWhitelist;

serverBan_t serverBans[SERVER_MAXBANS];
int serverBansCount = 0;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
===============
SV_ExpandNewlines

Converts newlines to "\n" so a line prints nicer
===============
*/
char	*SV_ExpandNewlines( char *in ) {
	static	char	string[1024];
	size_t	l;

	l = 0;
	while ( *in && l < sizeof(string) - 3 ) {
		if ( *in == '\n' ) {
			string[l++] = '\\';
			string[l++] = 'n';
		} else {
			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand( client_t *client, const char *cmd ) {
	int		index, i;

	// do not send commands until the gamestate has been sent
	if ( client->state < CS_PRIMED ) {
		return;
	}

	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if ( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 ) {
		Com_Printf( "===== pending server commands =====\n" );
		for ( i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
			Com_Printf( "cmd %5d: %s\n", i, client->reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
		}
		Com_Printf( "cmd %5d: %s\n", i, cmd );
		SV_DropClient( client, "Server command overflow" );
		return;
	}
	index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( client->reliableCommands[ index ], cmd, sizeof( client->reliableCommands[ index ] ) );
}


/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A NULL client will broadcast to all clients
=================
*/
void QDECL SV_SendServerCommand(client_t *cl, const char *fmt, ...) {
	va_list		argptr;
	byte		message[MAX_MSGLEN];
	client_t	*client;
	int			j;

	va_start (argptr,fmt);
	Q_vsnprintf((char *)message, sizeof(message), fmt, argptr);
	va_end (argptr);

	// Fix to http://aluigi.altervista.org/adv/q3msgboom-adv.txt
	// The actual cause of the bug is probably further downstream
	// and should maybe be addressed later, but this certainly
	// fixes the problem for now
	if ( strlen ((char *)message) > 1022 ) {
		return;
	}

	if ( cl != NULL ) {
		SV_AddServerCommand( cl, (char *)message );
		return;
	}

	// hack to echo broadcast prints to console
	if ( com_dedicated->integer && !Q_strncmp( (char *)message, "print", 5) ) {
		Com_Printf ("broadcast: %s\n", SV_ExpandNewlines((char *)message) );
	}

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < sv_maxclients->integer ; j++, client++) {
		SV_AddServerCommand( client, (char *)message );
	}
}


/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/
#define NEW_RESOLVE_DURATION		86400000 //24 hours
static int g_lastResolveTime[MAX_MASTER_SERVERS];

static inline bool SV_MasterNeedsResolving(int server, int time)
{ //refresh every so often regardless of if the actual address was modified -rww
	if (g_lastResolveTime[server] > time)
	{ //time flowed backwards?
		return true;
	}

	if ((time-g_lastResolveTime[server]) > NEW_RESOLVE_DURATION)
	{ //it's time again
		return true;
	}

	return false;
}

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
#define	HEARTBEAT_MSEC	300*1000
#define	HEARTBEAT_GAME	"QuakeArena-1"
void SV_MasterHeartbeat( void ) {
	static netadr_t	adr[MAX_MASTER_SERVERS];
	int			i;
	int			time;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if ( !com_dedicated || com_dedicated->integer != 2 ) {
		return;		// only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if ( svs.time < svs.nextHeartbeatTime ) {
		return;
	}
	svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

	//we need to use this instead of svs.time since svs.time resets over map changes (or rather
	//every time the game restarts), and we don't really need to resolve every map change
	time = Com_Milliseconds();

	// send to group masters
	for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		if ( !sv_master[i]->string[0] ) {
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if ( sv_master[i]->modified || SV_MasterNeedsResolving(i, time) ) {
			sv_master[i]->modified = qfalse;

			g_lastResolveTime[i] = time;

			Com_Printf( "Resolving %s\n", sv_master[i]->string );
			if ( !NET_StringToAdr( sv_master[i]->string, &adr[i] ) ) {
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf( "Couldn't resolve address: %s\n", sv_master[i]->string );
				Cvar_Set( sv_master[i]->name, "" );
				sv_master[i]->modified = qfalse;
				continue;
			}
			if ( !strstr( ":", sv_master[i]->string ) ) {
				adr[i].port = BigShort( PORT_MASTER );
			}
			Com_Printf( "%s resolved to %s\n", sv_master[i]->string, NET_AdrToString(adr[i]) );

			SVC_WhitelistAdr( adr[i] );
		}

		Com_Printf ("Sending heartbeat to %s\n", sv_master[i]->string );
		// this command should be changed if the server info / status format
		// ever incompatably changes
		NET_OutOfBandPrint( NS_SERVER, adr[i], "heartbeat %s\n", HEARTBEAT_GAME );
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown( void ) {
	// send a hearbeat right now
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// send it again to minimize chance of drops
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat();

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_BucketForAddress

Find or allocate a bucket for an address
================
*/
#include <map>

static leakyBucket_t *SVC_BucketForAddress( netadr_t address, int burst, int period, int now ) {
	static std::map<int, leakyBucket_t> bucketMap;
	static int lastGC = 0;

	if (address.type != NA_IP) {
		return NULL;
	}

	// Consider following spoofed dos scenario:
	//
	// Packets coming at a steady rate, IPs are never the
	// same. The garbage collector will perform
	// packet_rate * (1 + gc_rate * period / 1000)
	// iterations per second (where rates are in Hz)
	//
	// Theoretical minimum is packet_rate because buckets need to be
	// removed at the same rate they are created. It can be achieved
	// with another map, at the cost of higher constants.
	//
	// gc_rate at 1/s will keep the overhead relatively low.
	// Memory consumption is packet_rate * (period / 1000 + 1 / gc_rate)

	if (lastGC + 1000 < now) {
		lastGC = now;

		auto it = bucketMap.begin();

		while (it != bucketMap.end()) {
			int interval = now - it->second.lastTime;

			if (interval > it->second.burst * period || interval < 0) {
				it = bucketMap.erase(it);
			} else {
				++it;
			}
		}
	}

	return &bucketMap[address.ipi];
}

/*
================
SVC_RateLimit
================
*/
qboolean SVC_RateLimit( leakyBucket_t *bucket, int burst, int period, int now ) {
	qboolean	block = qfalse;

	if ( bucket != NULL ) {
		int interval = now - bucket->lastTime;
		int expired = interval / period;
		int expiredRemainder = interval % period;

		if ( expired > bucket->burst || interval < 0 ) {
			bucket->burst = 0;
			bucket->lastTime = now;
		} else {
			bucket->burst -= expired;
			bucket->lastTime = now - expiredRemainder;
		}

		if ( bucket->burst < burst ) {
			bucket->burst++;
		} else {
			block = qtrue;
		}
	}

	return block;
}

/*
================
SVC_RateLimitAddress

Rate limit for a particular address
================
*/
qboolean SVC_RateLimitAddress( netadr_t from, int burst, int period, int now ) {
	leakyBucket_t *bucket = SVC_BucketForAddress( from, burst, period, now );

	return SVC_RateLimit( bucket, burst, period, now );
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status( netadr_t from ) {
	char	player[1024];
	char	status[MAX_MSGLEN];
	int		i;
	client_t	*cl;
	playerState_t	*ps;
	int		statusLength;
	int		playerLength;
	char	infostring[MAX_INFO_STRING];

	// ignore if we are in single player
	/*
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER ) {
		return;
	}
	*/

	// A maximum challenge length of 128 should be more than plenty.
	if(strlen(Cmd_Argv(1)) > 128)
		return;

	Q_strncpyz( infostring, Cvar_InfoString( CVAR_SERVERINFO ), sizeof( infostring ) );

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey( infostring, "challenge", Cmd_Argv(1) );

	status[0] = 0;
	statusLength = 0;

	for (i=0 ; i < sv_maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if ( cl->state >= CS_CONNECTED ) {
			ps = SV_GameClientNum( i );
			Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[PERS_SCORE], cl->ping, cl->name);
			playerLength = strlen(player);
			if (statusLength + playerLength >= (int)sizeof(status) ) {
				break;		// can't hold any more
			}
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint( NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status );
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info( netadr_t from ) {
	int		i, count, humans, wDisable;
	char	*gamedir;
	char	infostring[MAX_INFO_STRING];

	// ignore if we are in single player
	/*
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")) {
		return;
	}
	*/

	if (Cvar_VariableValue("ui_singlePlayerActive"))
	{
		return;
	}

	/*
	 * Check whether Cmd_Argv(1) has a sane length. This was not done in the original Quake3 version which led
	 * to the Infostring bug discovered by Luigi Auriemma. See http://aluigi.altervista.org/ for the advisory.
	 */

	// A maximum challenge length of 128 should be more than plenty.
	if(strlen(Cmd_Argv(1)) > 128)
		return;

	// don't count privateclients
	count = humans = 0;
	for ( i = sv_privateClients->integer ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
			if ( svs.clients[i].netchan.remoteAddress.type != NA_BOT ) {
				humans++;
			}
		}
	}

	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey( infostring, "challenge", Cmd_Argv(1) );

	Info_SetValueForKey( infostring, "protocol", va("%i", PROTOCOL_VERSION) );
	Info_SetValueForKey( infostring, "hostname", sv_hostname->string );
	Info_SetValueForKey( infostring, "mapname", sv_mapname->string );
	Info_SetValueForKey( infostring, "clients", va("%i", count) );
	Info_SetValueForKey( infostring, "g_humanplayers", va("%i", humans) );
	Info_SetValueForKey( infostring, "sv_maxclients",
		va("%i", sv_maxclients->integer - sv_privateClients->integer ) );
	Info_SetValueForKey( infostring, "gametype", va("%i", sv_gametype->integer ) );
	Info_SetValueForKey( infostring, "needpass", va("%i", sv_needpass->integer ) );
	Info_SetValueForKey( infostring, "truejedi", va("%i", Cvar_VariableIntegerValue( "g_jediVmerc" ) ) );
	if ( sv_gametype->integer == GT_DUEL || sv_gametype->integer == GT_POWERDUEL )
	{
		wDisable = Cvar_VariableIntegerValue( "g_duelWeaponDisable" );
	}
	else
	{
		wDisable = Cvar_VariableIntegerValue( "g_weaponDisable" );
	}
	Info_SetValueForKey( infostring, "wdisable", va("%i", wDisable ) );
	Info_SetValueForKey( infostring, "fdisable", va("%i", Cvar_VariableIntegerValue( "g_forcePowerDisable" ) ) );
	//Info_SetValueForKey( infostring, "pure", va("%i", sv_pure->integer ) );
	Info_SetValueForKey( infostring, "autodemo", va("%i", sv_autoDemo->integer ) );

	if( sv_minPing->integer ) {
		Info_SetValueForKey( infostring, "minPing", va("%i", sv_minPing->integer) );
	}
	if( sv_maxPing->integer ) {
		Info_SetValueForKey( infostring, "maxPing", va("%i", sv_maxPing->integer) );
	}
	gamedir = Cvar_VariableString( "fs_game" );
	if( *gamedir ) {
		Info_SetValueForKey( infostring, "game", gamedir );
	}

	NET_OutOfBandPrint( NS_SERVER, from, "infoResponse\n%s", infostring );
}

/*
================
SVC_FlushRedirect

================
*/
void SV_FlushRedirect( char *outputbuf ) {
	NET_OutOfBandPrint( NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf );
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand( netadr_t from, msg_t *msg ) {
	qboolean	valid;
	char		remaining[1024];
	// TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	// (OOB messages are the bottleneck here)
#define	SV_OUTPUTBUF_LENGTH	(1024 - 16)
	char		sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	char		*cmd_aux;

	if ( !strlen( sv_rconPassword->string ) ||
		strcmp (Cmd_Argv(1), sv_rconPassword->string) ) {
		valid = qfalse;
		Com_Printf ("Bad rcon from %s: %s\n", NET_AdrToString (from), Cmd_ArgsFrom(2) );
	} else {
		valid = qtrue;
		Com_Printf ("Rcon from %s: %s\n", NET_AdrToString (from), Cmd_ArgsFrom(2) );
	}

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	Com_BeginRedirect (sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if ( !strlen( sv_rconPassword->string ) ) {
		Com_Printf ("No rconpassword set.\n");
	} else if ( !valid ) {
		Com_Printf ("Bad rconpassword.\n");
	} else {
		remaining[0] = 0;

		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux = Cmd_Cmd();
		cmd_aux+=4;
		while(cmd_aux[0]==' ')
			cmd_aux++;
		while(cmd_aux[0] && cmd_aux[0]!=' ') // password
			cmd_aux++;
		while(cmd_aux[0]==' ')
			cmd_aux++;

		Q_strcat( remaining, sizeof(remaining), cmd_aux);

		Cmd_ExecuteString (remaining);
	}

	Com_EndRedirect ();
}

// dos protection whitelist

#define WHITELIST_FILE			"ipwhitelist.dat"

#include <set>

static std::set<int32_t>	svc_whitelist;

void SVC_LoadWhitelist( void ) {
	fileHandle_t f;
	int32_t *data = NULL;
	int len = FS_SV_FOpenFileRead(WHITELIST_FILE, &f);

	if (len <= 0) {
		return;
	}

	data = (int32_t *)Z_Malloc(len, TAG_TEMP_WORKSPACE);

	FS_Read(data, len, f);
	FS_FCloseFile(f);

	len /= sizeof(int32_t);

	for (int i = 0; i < len; i++) {
		svc_whitelist.insert(data[i]);
	}

	Z_Free(data);
	data = NULL;
}

void SVC_WhitelistAdr( netadr_t adr ) {
	fileHandle_t f;

	if (adr.type != NA_IP) {
		return;
	}

	if (!svc_whitelist.insert(adr.ipi).second) {
		return;
	}

	Com_DPrintf("Whitelisting %s\n", NET_AdrToString(adr));

	f = FS_SV_FOpenFileAppend(WHITELIST_FILE);
	if (!f) {
		Com_Printf("Couldn't open " WHITELIST_FILE ".\n");
		return;
	}

	FS_Write(adr.ip, sizeof(adr.ip), f);
	FS_FCloseFile(f);
}

static qboolean SVC_IsWhitelisted( netadr_t adr ) {
	if (adr.type == NA_IP) {
		return (qboolean)(svc_whitelist.find(adr.ipi) != svc_whitelist.end());
	} else {
		return qtrue;
	}
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	static	leakyBucket_t	bucket;
	static	int	dropped = 0;
	static	int	lastMsg = 0;
	int		now = Sys_Milliseconds();
	char	*s;
	char	*c;

	if (sv_maxOOBRateIP->integer) {
		int rate = Com_Clampi(1, 1000, sv_maxOOBRateIP->integer);
		int period = 1000 / rate;
		int burst = 10 * rate;

		if (SVC_RateLimitAddress(from, burst, period, now)) {
			if (com_developer && com_developer->integer) {
				Com_Printf("SV_ConnectionlessPacket: Rate limit from %s exceeded, dropping request\n", NET_AdrToString(from));
			}
			return;
		}
	}

	if (sv_maxOOBRate->integer) {
		int rate = Com_Clampi(1, 1000, sv_maxOOBRate->integer);
		int period = 1000 / rate;

		if (SVC_IsWhitelisted(from)) {
			rate *= 2;
		}

		if (SVC_RateLimit(&bucket, rate, period, now)) {
			dropped++;
			return;
		}
	}

	if (dropped > 0 && lastMsg + 5000 < now) {
		Com_Printf("SV_ConnectionlessPacket: Rate limit exceeded, dropped %d requests\n", dropped);
		dropped = 0;
		lastMsg = now;
	}

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );		// skip the -1 marker

	if (!Q_strncmp("connect", (const char *)&msg->data[4], 7)) {
		Huff_Decompress(msg, 12);
	}

	s = MSG_ReadStringLine( msg );
	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);
	if ( com_developer->integer ) {
		Com_Printf( "SV packet %s : %s\n", NET_AdrToString( from ), c );
	}

	if (!Q_stricmp(c, "getstatus")) {
		SVC_Status( from  );
	} else if (!Q_stricmp(c, "getinfo")) {
		SVC_Info( from );
	} else if (!Q_stricmp(c, "getchallenge")) {
		SV_GetChallenge( from );
	} else if (!Q_stricmp(c, "connect")) {
		SV_DirectConnect( from );
	} else if (!Q_stricmp(c, "ipAuthorize")) {
		// ...
	} else if (!Q_stricmp(c, "rcon")) {
		SVC_RemoteCommand( from, msg );
	} else if (!Q_stricmp(c, "disconnect")) {
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	} else {
		if ( com_developer->integer ) {
			Com_Printf( "bad connectionless packet from %s:\n%s\n",
				NET_AdrToString( from ), s );
		}
	}
}


//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent( netadr_t from, msg_t *msg ) {
	int			i;
	client_t	*cl;
	int			qport;

	// check for connectionless packet (0xffffffff) first
	if ( msg->cursize >= 4 && *(int *)msg->data == -1) {
		SV_ConnectionlessPacket( from, msg );
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );				// sequence number
	qport = MSG_ReadShort( msg ) & 0xffff;

	// find which client the message is from
	for (i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if (cl->state == CS_FREE) {
			continue;
		}
		if ( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) ) {
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport) {
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port) {
			Com_Printf( "SV_ReadPackets: fixing up a translated port\n" );
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SV_Netchan_Process(cl, msg)) {
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE) {
				cl->lastPacketTime = svs.time;	// don't timeout
				SV_ExecuteClientMessage( cl, msg );
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint( NS_SERVER, from, "disconnect" );
}


/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings( void ) {
	int			i, j;
	client_t	*cl;
	int			total, count;
	int			delta;
	playerState_t	*ps;

	for (i=0 ; i < sv_maxclients->integer ; i++) {
		cl = &svs.clients[i];
		if ( cl->state != CS_ACTIVE ) {
			cl->ping = 999;
			continue;
		}
		if ( !cl->gentity ) {
			cl->ping = 999;
			continue;
		}
		if ( cl->gentity->r.svFlags & SVF_BOT ) {
			cl->ping = 0;
			continue;
		}

		total = 0;
		count = 0;
		for ( j = 0 ; j < PACKET_BACKUP ; j++ ) {
			if ( cl->frames[j].messageAcked <= 0 ) {
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count) {
			cl->ping = 999;
		} else {
			cl->ping = total/count;
			if ( cl->ping > 999 ) {
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		ps = SV_GameClientNum( i );
		ps->ping = cl->ping;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts( void ) {
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.time - 1000 * sv_timeout->integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		// message times may be wrong across a changelevel
		if (cl->lastPacketTime > svs.time) {
			cl->lastPacketTime = svs.time;
		}

		if (cl->state == CS_ZOMBIE
		&& cl->lastPacketTime < zombiepoint) {
			Com_DPrintf( "Going from CS_ZOMBIE to CS_FREE for %s\n", cl->name );
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		if ( cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint) {
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if ( ++cl->timeoutCount > 5 ) {
				SV_DropClient (cl, "timed out");
				cl->state = CS_FREE;	// don't bother with zombie state
			}
		} else {
			cl->timeoutCount = 0;
		}
	}
}


/*
==================
SV_CheckPaused
==================
*/
qboolean SV_CheckPaused( void ) {
	int		count;
	client_t	*cl;
	int		i;

	if ( !cl_paused->integer ) {
		return qfalse;
	}

	// only pause if there is just a single client connected
	count = 0;
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT ) {
			count++;
		}
	}

	if ( count > 1 ) {
		// don't pause
		if (sv_paused->integer)
			Cvar_Set("sv_paused", "0");
		return qfalse;
	}

	if (!sv_paused->integer)
		Cvar_Set("sv_paused", "1");
	return qtrue;
}

/*
==================
SV_CheckCvars
==================
*/
void SV_CheckCvars( void ) {
	static int lastModHostname = -1, lastModFramerate = -1, lastModSnapsMin = -1, lastModSnapsMax = -1;
	static int lastModSnapsPolicy = -1, lastModRatePolicy = -1, lastModClientRate = -1;
	static int lastModMaxRate = -1, lastModMinRate = -1;
	qboolean changed = qfalse;

	if ( sv_hostname->modificationCount != lastModHostname ) {
		char hostname[MAX_INFO_STRING];
		char *c = hostname;
		lastModHostname = sv_hostname->modificationCount;

		strcpy( hostname, sv_hostname->string );
		while( *c )
		{
			if ( (*c == '\\') || (*c == ';') || (*c == '"'))
			{
				*c = '.';
				changed = qtrue;
			}
			c++;
		}
		if( changed )
		{
			Cvar_Set("sv_hostname", hostname );
		}
	}

	// check limits on client "rate" values based on server settings
	if ( sv_clientRate->modificationCount != lastModClientRate ||
		 sv_minRate->modificationCount != lastModMinRate ||
		 sv_maxRate->modificationCount != lastModMaxRate ||
		 sv_ratePolicy->modificationCount != lastModRatePolicy )
	{
		sv_clientRate->modificationCount = lastModClientRate;
		sv_maxRate->modificationCount = lastModMaxRate;
		sv_minRate->modificationCount = lastModMinRate;
		sv_ratePolicy->modificationCount = lastModRatePolicy;

		if (sv_ratePolicy->integer == 1)
		{
			// NOTE: what if server sets some dumb sv_clientRate value?
			client_t *cl = NULL;
			int i = 0;

			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
				// if the client is on the same subnet as the server and we aren't running an
				// internet public server, assume they don't need a rate choke
				if (Sys_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1) {
					cl->rate = 100000;	// lans should not rate limit
				}
				else {
					int val = sv_clientRate->integer;
					if (val != cl->rate) {
						cl->rate = val;
					}
				}
			}
		}
		else if (sv_ratePolicy->integer == 2)
		{
			// NOTE: what if server sets some dumb sv_clientRate value?
			client_t *cl = NULL;
			int i = 0;

			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
				// if the client is on the same subnet as the server and we aren't running an
				// internet public server, assume they don't need a rate choke
				if (Sys_IsLANAddress(cl->netchan.remoteAddress) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1) {
					cl->rate = 100000;	// lans should not rate limit
				}
				else {
					int val = cl->rate;
					if (!val) {
						val = sv_maxRate->integer;
					}
					val = Com_Clampi( 1000, 90000, val );
					val = Com_Clampi( sv_minRate->integer, sv_maxRate->integer, val );
					if (val != cl->rate) {
						cl->rate = val;
					}
				}
			}
		}
	}

	// check limits on client "snaps" value based on server framerate and snapshot rate
	if ( sv_fps->modificationCount != lastModFramerate ||
		 sv_snapsMin->modificationCount != lastModSnapsMin ||
		 sv_snapsMax->modificationCount != lastModSnapsMax ||
		 sv_snapsPolicy->modificationCount != lastModSnapsPolicy )
	{
		lastModFramerate = sv_fps->modificationCount;
		lastModSnapsMin = sv_snapsMin->modificationCount;
		lastModSnapsMax = sv_snapsMax->modificationCount;
		lastModSnapsPolicy = sv_snapsPolicy->modificationCount;

		if (sv_snapsPolicy->integer == 1)
		{
			client_t *cl = NULL;
			int i = 0;

			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
				int val = 1000 / sv_fps->integer;
				if (val != cl->snapshotMsec) {
					// Reset last sent snapshot so we avoid desync between server frame time and snapshot send time
					cl->nextSnapshotTime = -1;
					cl->snapshotMsec = val;
				}
			}
		}
		else if (sv_snapsPolicy->integer == 2)
		{
			client_t *cl = NULL;
			int i = 0;
			int minSnaps = Com_Clampi(1, sv_snapsMax->integer, sv_snapsMin->integer); // between 1 and sv_snapsMax ( 1 <-> 40 )
			int maxSnaps = Q_min(sv_fps->integer, sv_snapsMax->integer); // can't produce more than sv_fps snapshots/sec, but can send less than sv_fps snapshots/sec

			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
				int val = 1000 / Com_Clampi(minSnaps, maxSnaps, cl->wishSnaps);
				if (val != cl->snapshotMsec) {
					// Reset last sent snapshot so we avoid desync between server frame time and snapshot send time
					cl->nextSnapshotTime = -1;
					cl->snapshotMsec = val;
				}
			}
		}
	}
}

/*
==================
SV_FrameMsec
Return time in millseconds until processing of the next server frame.
==================
*/
int SV_FrameMsec()
{
	if(sv_fps)
	{
		int frameMsec;

		frameMsec = 1000.0f / sv_fps->value;

		if(frameMsec < sv.timeResidual)
			return 0;
		else
			return frameMsec - sv.timeResidual;
	}
	else
		return 1;
}

/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame( int msec ) {
	int		frameMsec;
	int		startTime;

	// the menu kills the server with this cvar
	if ( sv_killserver->integer ) {
		SV_Shutdown ("Server was killed.\n");
		Cvar_Set( "sv_killserver", "0" );
		return;
	}

	if ( !com_sv_running->integer ) {
		return;
	}

	// allow pause if only the local client is connected
	if ( SV_CheckPaused() ) {
		return;
	}

	// if it isn't time for the next frame, do nothing
	if ( sv_fps->integer < 1 ) {
		Cvar_Set( "sv_fps", "10" );
	}
	frameMsec = 1000 / sv_fps->integer * com_timescale->value;
	// don't let it scale below 1ms
	if(frameMsec < 1)
	{
		Cvar_Set("timescale", va("%f", sv_fps->integer / 1000.0f));
		frameMsec = 1;
	}

	sv.timeResidual += msec;

	if (!com_dedicated->integer) SV_BotFrame( sv.time + sv.timeResidual );

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if ( svs.time > 0x70000000 ) {
		SV_Shutdown( "Restarting server due to time wrapping" );
		Cbuf_AddText( va( "map %s\n", Cvar_VariableString( "mapname" ) ) );
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if ( svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities ) {
		SV_Shutdown( "Restarting server due to numSnapshotEntities wrapping" );
		Cbuf_AddText( va( "map %s\n", Cvar_VariableString( "mapname" ) ) );
		return;
	}

	if( sv.restartTime && sv.time >= sv.restartTime ) {
		sv.restartTime = 0;
		Cbuf_AddText( "map_restart 0\n" );
		return;
	}

	// update infostrings if anything has been changed
	if ( cvar_modifiedFlags & CVAR_SERVERINFO ) {
		SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if ( cvar_modifiedFlags & CVAR_SYSTEMINFO ) {
		SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}

	if ( com_speeds->integer ) {
		startTime = Sys_Milliseconds ();
	} else {
		startTime = 0;	// quite a compiler warning
	}

	// update ping based on the all received frames
	SV_CalcPings();

	if (com_dedicated->integer) SV_BotFrame( sv.time );

	// run the game simulation in chunks
	while ( sv.timeResidual >= frameMsec ) {
		sv.timeResidual -= frameMsec;
		svs.time += frameMsec;
		sv.time += frameMsec;

		// let everything in the world think and move
		GVM_RunFrame( sv.time );
	}

	//rww - RAGDOLL_BEGIN
	re->G2API_SetTime(sv.time,0);
	//rww - RAGDOLL_END

	if ( com_speeds->integer ) {
		time_game = Sys_Milliseconds () - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();

	// send messages back to the clients
	SV_SendClientMessages();

	SV_CheckCvars();

	// send a heartbeat to the master if needed
	SV_MasterHeartbeat();
}

//============================================================================

