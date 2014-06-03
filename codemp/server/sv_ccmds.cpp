#include "server.h"
#include "qcommon/stringed_ingame.h"
#include "server/sv_gameapi.h"
#include "qcommon/game_version.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

const char *SV_GetStringEdString(char *refSection, char *refName)
{
	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a stringed reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the stringed reference indication and dealt with
	//properly.
	static char text[1024]={0};
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

/*
==================
SV_GetPlayerByHandle

Returns the player with player id or name from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByHandle( void ) {
	client_t	*cl;
	int			i;
	char		*s;
	char		cleanName[64];

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "No player specified.\n" );
		return NULL;
	}

	s = Cmd_Argv(1);

	// Check whether this is a numeric player handle
	for(i = 0; s[i] >= '0' && s[i] <= '9'; i++);

	if(!s[i])
	{
		int plid = atoi(s);

		// Check for numeric playerid match
		if(plid >= 0 && plid < sv_maxclients->integer)
		{
			cl = &svs.clients[plid];

			if(cl->state)
				return cl;
		}
	}

	// check for a name match
	for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( !cl->state ) {
			continue;
		}
		if ( !Q_stricmp( cl->name, s ) ) {
			return cl;
		}

		Q_strncpyz( cleanName, cl->name, sizeof(cleanName) );
		Q_StripColor( cleanName );
		//Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, s ) ) {
			return cl;
		}
	}

	Com_Printf( "Player %s is not on the server\n", s );

	return NULL;
}

/*
==================
SV_GetPlayerByNum

Returns the player with idnum from Cmd_Argv(1)
==================
*/
static client_t *SV_GetPlayerByNum( void ) {
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return NULL;
	}

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "No player specified.\n" );
		return NULL;
	}

	s = Cmd_Argv(1);

	for (i = 0; s[i]; i++) {
		if (s[i] < '0' || s[i] > '9') {
			Com_Printf( "Bad slot number: %s\n", s);
			return NULL;
		}
	}
	idnum = atoi( s );
	if ( idnum < 0 || idnum >= sv_maxclients->integer ) {
		Com_Printf( "Bad client slot: %i\n", idnum );
		return NULL;
	}

	cl = &svs.clients[idnum];
	if ( !cl->state ) {
		Com_Printf( "Client %i is not active\n", idnum );
		return NULL;
	}
	return cl;
}

//=========================================================

/*
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f( void ) {
	char		*cmd = NULL, *map = NULL;
	qboolean	killBots=qfalse, cheat=qfalse;
	char		expanded[MAX_QPATH] = {0}, mapname[MAX_QPATH] = {0};

	map = Cmd_Argv(1);
	if ( !map )
		return;

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	if (strchr (map, '\\') ) {
		Com_Printf ("Can't have mapnames with a \\\n");
		return;
	}

	Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
	if ( FS_ReadFile (expanded, NULL) == -1 ) {
		Com_Printf ("Can't find map %s\n", expanded);
		return;
	}

	// force latched values to get set
	Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH );

	cmd = Cmd_Argv(0);
	if ( !Q_stricmpn( cmd, "devmap", 6 ) ) {
		cheat = qtrue;
		killBots = qtrue;
	} else {
		cheat = qfalse;
		killBots = qfalse;
	}

	// save the map name here cause on a map restart we reload the jampconfig.cfg
	// and thus nuke the arguments of the map command
	Q_strncpyz(mapname, map, sizeof(mapname));

	ForceReload_e eForceReload = eForceReload_NOTHING;	// default for normal load

//	if ( !Q_stricmp( cmd, "devmapbsp") ) {	// not relevant in MP codebase
//		eForceReload = eForceReload_BSP;
//	}
//	else
	if ( !Q_stricmp( cmd, "devmapmdl") ) {
		eForceReload = eForceReload_MODELS;
	}
	else
	if ( !Q_stricmp( cmd, "devmapall") ) {
		eForceReload = eForceReload_ALL;
	}

	// start up the map
	SV_SpawnServer( mapname, killBots, eForceReload );

	// set the cheat value
	// if the level was started with "map <levelname>", then
	// cheats will not be allowed.  If started with "devmap <levelname>"
	// then cheats will be allowed
	Cvar_Set( "sv_cheats", cheat ? "1" : "0" );
}


/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f( void ) {
	int			i;
	client_t	*client;
	char		*denied;
	qboolean	isBot;
	int			delay;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId ) {
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( sv.restartTime ) {
		return;
	}

	if (Cmd_Argc() > 1 ) {
		delay = atoi( Cmd_Argv(1) );
	}
	else {
		delay = 5;
	}
	if( delay ) {
		sv.restartTime = sv.time + delay * 1000;
		SV_SetConfigstring( CS_WARMUP, va("%i", sv.restartTime) );
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified || sv_gametype->modified ) {
		char	mapname[MAX_QPATH];

		Com_Printf( "variable change -- restarting.\n" );
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );

		SV_SpawnServer( mapname, qfalse, eForceReload_NOTHING );
		return;
	}

	SV_StopAutoRecordDemos();

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	time( &sv.realMapTimeStarted );

	// if a map_restart occurs while a client is changing maps, we need
	// to give them the correct time so that when they finish loading
	// they don't violate the backwards time check in cl_cgame.c
	for (i=0 ; i<sv_maxclients->integer ; i++) {
		if (svs.clients[i].state == CS_PRIMED) {
			svs.clients[i].oldServerTime = sv.restartTime;
		}
	}

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.restarting = qtrue;

	SV_RestartGame();

	// run a few frames to allow everything to settle
	for ( i = 0 ;i < 3 ; i++ ) {
		GVM_RunFrame( sv.time );
		sv.time += 100;
		svs.time += 100;
	}

	sv.state = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for (i=0 ; i<sv_maxclients->integer ; i++) {
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if ( client->state < CS_CONNECTED) {
			continue;
		}

		if ( client->netchan.remoteAddress.type == NA_BOT ) {
			isBot = qtrue;
		} else {
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = GVM_ClientConnect( i, qfalse, isBot );
		if ( denied ) {
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, denied );
			Com_Printf( "SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i ); // bk010125
			continue;
		}

		if(client->state == CS_ACTIVE)
			SV_ClientEnterWorld(client, &client->lastUsercmd);
		else
		{
			// If we don't reset client->lastUsercmd and are restarting during map load,
			// the client will hang because we'll use the last Usercmd from the previous map,
			// which is wrong obviously.
			SV_ClientEnterWorld(client, NULL);
		}
	}

	// run another frame to allow things to look at all the players
	GVM_RunFrame( sv.time );
	sv.time += 100;
	svs.time += 100;

	SV_BeginAutoRecordDemos();
}

//===============================================================

/*
==================
SV_KickBlankPlayers
==================
*/
static void SV_KickBlankPlayers( void ) {
	client_t	*cl;
	int			i;
	char		cleanName[64];

	// make sure server is running
	if ( !com_sv_running->integer ) {
		return;
	}

	// check for a name match
	for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( !cl->state ) {
			continue;
		}
		if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
			continue;
		}
		if ( !Q_stricmp( cl->name, "" ) ) {
			SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
			cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			continue;
		}

		Q_strncpyz( cleanName, cl->name, sizeof(cleanName) );
		Q_StripColor( cleanName );
		//Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, "" ) ) {
			SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
			cl->lastPacketTime = svs.time;	// in case there is a funny zombie
		}
	}
}

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f( void ) {
	client_t	*cl;
	int			i;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: kick <player name>\nkick all = kick everyone\nkick allbots = kick all bots\n");
		return;
	}

	if (!Q_stricmp(Cmd_Argv(1), "Padawan"))
	{ //if you try to kick the default name, also try to kick ""
		SV_KickBlankPlayers();
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		if ( !Q_stricmp(Cmd_Argv(1), "all") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
					continue;
				}
				SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		else if ( !Q_stricmp(Cmd_Argv(1), "allbots") ) {
			for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
				if ( !cl->state ) {
					continue;
				}
				if( cl->netchan.remoteAddress.type != NA_BOT ) {
					continue;
				}
				SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
				cl->lastPacketTime = svs.time;	// in case there is a funny zombie
			}
		}
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		Com_Printf("Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
==================
SV_KickBots_f

Kick all bots off of the server
==================
*/
static void SV_KickBots_f( void ) {
	client_t	*cl;
	int			i;

	// make sure server is running
	if( !com_sv_running->integer ) {
		Com_Printf("Server is not running.\n");
		return;
	}

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if( !cl->state ) {
			continue;
		}

		if( cl->netchan.remoteAddress.type != NA_BOT ) {
			continue;
		}

		SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
		cl->lastPacketTime = svs.time; // in case there is a funny zombie
	}
}
/*
==================
SV_KickAll_f

Kick all users off of the server
==================
*/
static void SV_KickAll_f( void ) {
	client_t *cl;
	int i;

	// make sure server is running
	if( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if( !cl->state ) {
			continue;
		}

		if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
			continue;
		}

		SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
		cl->lastPacketTime = svs.time; // in case there is a funny zombie
	}
}

/*
==================
SV_KickNum_f

Kick a user off of the server
==================
*/
static void SV_KickNum_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: %s <client number>\n", Cmd_Argv(0));
		return;
	}

	cl = SV_GetPlayerByNum();
	if ( !cl ) {
		return;
	}
	if( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		Com_Printf("Cannot kick host player\n");
		return;
	}

	SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","WAS_KICKED"));	// "was kicked" );
	cl->lastPacketTime = svs.time;	// in case there is a funny zombie
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f( void )
{
	int				i, humans, bots;
	client_t		*cl;
	playerState_t	*ps;
	const char		*s;
	int				ping;
	char			state[32];
	qboolean		avoidTruncation = qfalse;

	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() > 1 )
	{
		if (!Q_stricmp("notrunc", Cmd_Argv(1)))
		{
			avoidTruncation = qtrue;
		}
	}

	humans = bots = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if ( svs.clients[i].netchan.remoteAddress.type != NA_BOT ) {
				humans++;
			}
			else {
				bots++;
			}
		}
	}

#if defined(_WIN32)
#define STATUS_OS "Windows"
#elif defined(__linux__)
#define STATUS_OS "Linux"
#elif defined(MACOS_X)
#define STATUS_OS "OSX"
#else
#define STATUS_OS "Unknown"
#endif

	const char *ded_table[] =
	{
		"listen",
		"lan dedicated",
		"public dedicated",
	};

	char hostname[MAX_HOSTNAMELENGTH]={0};

	Q_strncpyz(hostname, sv_hostname->string, sizeof(hostname));
	Q_StripColor(hostname);

	Com_Printf ("hostname: %s^7\n", hostname );
	Com_Printf ("version : %s %i\n", VERSION_STRING_DOTTED, PROTOCOL_VERSION );
	Com_Printf ("game    : %s\n", FS_GetCurrentGameDir() );
	Com_Printf ("udp/ip  : %s:%i os(%s) type(%s)\n", Cvar_VariableString("net_ip"), Cvar_VariableIntegerValue("net_port"), STATUS_OS, ded_table[com_dedicated->integer]);
	Com_Printf ("map     : %s gametype(%i)\n", sv_mapname->string, sv_gametype->integer );
	Com_Printf ("players : %i humans, %i bots (%i max)\n", humans, bots, sv_maxclients->integer - sv_privateClients->integer);

	Com_Printf ("num score ping name            lastmsg address               qport rate\n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ----- -----\n");
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}

		if (cl->state == CS_CONNECTED)
		{
			strcpy(state, "CNCT ");
		}
		else if (cl->state == CS_ZOMBIE)
		{
			strcpy(state, "ZMBI ");
		}
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_sprintf(state, sizeof(state), "%4i", ping);
		}

		ps = SV_GameClientNum( i );
		s = NET_AdrToString( cl->netchan.remoteAddress );

		if (!avoidTruncation)
		{
			Com_Printf ("%3i %5i %s %-15.15s %7i %21s %5i %5i\n",
				i,
				ps->persistant[PERS_SCORE],
				state,
				cl->name,
				svs.time - cl->lastPacketTime,
				s,
				cl->netchan.qport,
				cl->rate
				);
		}
		else
		{
			Com_Printf ("%3i %5i %s %s %7i %21s %5i %5i\n",
				i,
				ps->persistant[PERS_SCORE],
				state,
				cl->name,
				svs.time - cl->lastPacketTime,
				s,
				cl->netchan.qport,
				cl->rate
				);
		}
	}
	Com_Printf ("\n");
}

char	*SV_ExpandNewlines( char *in );
#define SVSAY_PREFIX "Server^7\x19: "

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f(void) {
	char	text[MAX_SAY_TEXT] = {0};

	if( !com_dedicated->integer ) {
		Com_Printf( "Server is not dedicated.\n" );
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc () < 2 ) {
		return;
	}

	Cmd_ArgsBuffer( text, sizeof(text) );

	Com_Printf ("broadcast: chat \""SVSAY_PREFIX"%s\\n\"\n", SV_ExpandNewlines((char *)text) );
	SV_SendServerCommand(NULL, "chat \""SVSAY_PREFIX"%s\"\n", text);
}

const char *forceToggleNamePrints[NUM_FORCE_POWERS] = {
	"HEAL",
	"JUMP",
	"SPEED",
	"PUSH",
	"PULL",
	"MINDTRICK",
	"GRIP",
	"LIGHTNING",
	"DARK RAGE",
	"PROTECT",
	"ABSORB",
	"TEAM HEAL",
	"TEAM REPLENISH",
	"DRAIN",
	"SEEING",
	"SABER OFFENSE",
	"SABER DEFENSE",
	"SABER THROW",
};

static void SV_ForceToggle_f( void ) {
	int bits = Cvar_VariableIntegerValue("g_forcePowerDisable");
	int i, val;
	char *s;
	const char *enablestrings[] =
	{
		"Disabled",
		"Enabled"
	};

	// make sure server is running
	if( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		for(i = 0; i < NUM_FORCE_POWERS; i++ ) {
			Com_Printf ("%i - %s - Status: %s\n", i, forceToggleNamePrints[i], enablestrings[!(bits & (1<<i))]);
		}
		Com_Printf ("Example usage: forcetoggle 3(toggles PUSH)\n");
		return;
	}

	s = Cmd_Argv(1);

	if( Q_isanumber( s ) ) {
		val = atoi(s);
		if( val >= 0 && val < NUM_FORCE_POWERS) {
			bits ^= (1 << val);
			Cvar_SetValue("g_forcePowerDisable", bits);
			Com_Printf ("%s has been %s.\n", forceToggleNamePrints[val], (bits & (1<<val)) ? "disabled" : "enabled");
		}
		else {
			Com_Printf ("Specified a power that does not exist.\nExample usage: forcetoggle 3\n(toggles PUSH)\n");
		}
	}
	else {
		for(i = 0; i < NUM_FORCE_POWERS; i++ ) {
			Com_Printf ("%i - %s - Status: %s\n", i, forceToggleNamePrints[i], enablestrings[!(bits & (1<<i))]);
		}
		Com_Printf ("Specified a power that does not exist.\nExample usage: forcetoggle 3\n(toggles PUSH)\n");
	}
}

/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f( void ) {
	svs.nextHeartbeatTime = -9999999;
}

/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f( void ) {
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	Com_Printf ("Server info settings:\n");
	Info_Print ( Cvar_InfoString( CVAR_SERVERINFO ) );
}

/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f( void ) {
	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	Com_Printf ("System info settings:\n");
	Info_Print ( Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
}

/*
===========
SV_DumpUser_f

Examine all a users info strings FIXME: move to game
===========
*/
static void SV_DumpUser_f( void ) {
	client_t	*cl;

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	cl = SV_GetPlayerByHandle();
	if ( !cl ) {
		return;
	}

	Com_Printf( "userinfo\n" );
	Com_Printf( "--------\n" );
	Info_Print( cl->userinfo );
}

/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f( void ) {
	SV_Shutdown( "killserver" );
}

void SV_WriteDemoMessage ( client_t *cl, msg_t *msg, int headerBytes ) {
	int		len, swlen;

	// write the packet sequence
	len = cl->netchan.outgoingSequence;
	swlen = LittleLong( len );
	FS_Write( &swlen, 4, cl->demo.demofile );

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong( len );
	FS_Write( &swlen, 4, cl->demo.demofile );
	FS_Write( msg->data + headerBytes, len, cl->demo.demofile );
}

void SV_StopRecordDemo( client_t *cl ) {
	int		len;

	if ( !cl->demo.demorecording ) {
		Com_Printf( "Client %d is not recording a demo.\n", cl - svs.clients );
		return;
	}

	// finish up
	len = -1;
	FS_Write (&len, 4, cl->demo.demofile);
	FS_Write (&len, 4, cl->demo.demofile);
	FS_FCloseFile (cl->demo.demofile);
	cl->demo.demofile = 0;
	cl->demo.demorecording = qfalse;
	Com_Printf ("Stopped demo for client %d.\n", cl - svs.clients);
}

// stops all recording demos
void SV_StopAutoRecordDemos() {
	if ( svs.clients && sv_autoDemo->integer ) {
		for ( client_t *client = svs.clients; client - svs.clients < sv_maxclients->integer; client++ ) {
			if ( client->demo.demorecording) {
				SV_StopRecordDemo( client );
			}
		}
	}
}

/*
====================
SV_StopRecording_f

stop recording a demo
====================
*/
void SV_StopRecord_f( void ) {
	int		i;

	client_t *cl = NULL;
	if ( Cmd_Argc() == 2 ) {
		int clIndex = atoi( Cmd_Argv( 1 ) );
		if ( clIndex < 0 || clIndex >= sv_maxclients->integer ) {
			Com_Printf( "Unknown client number %d.\n", clIndex );
			return;
		}
		cl = &svs.clients[clIndex];
	} else {
		for (i = 0; i < sv_maxclients->integer; i++) {
			if ( svs.clients[i].demo.demorecording ) {
				cl = &svs.clients[i];
				break;
			}
		}
		if ( cl == NULL ) {
			Com_Printf( "No demo being recorded.\n" );
			return;
		}
	}
	SV_StopRecordDemo( cl );
}

/*
==================
SV_DemoFilename
==================
*/
void SV_DemoFilename( char *buf, int bufSize ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "demo%s", timeStr );
}

// defined in sv_client.cpp
extern void SV_CreateClientGameStateMessage( client_t *client, msg_t* msg );

void SV_RecordDemo( client_t *cl, char *demoName ) {
	char		name[MAX_OSPATH];
	byte		bufData[MAX_MSGLEN];
	msg_t		msg;
	int			len;

	if ( cl->demo.demorecording ) {
		Com_Printf( "Already recording.\n" );
		return;
	}

	if ( cl->state != CS_ACTIVE ) {
		Com_Printf( "Client is not active.\n" );
		return;
	}

	// open the demo file
	Q_strncpyz( cl->demo.demoName, demoName, sizeof( cl->demo.demoName ) );
	Com_sprintf( name, sizeof( name ), "demos/%s.dm_%d", cl->demo.demoName, PROTOCOL_VERSION );
	Com_Printf( "recording to %s.\n", name );
	cl->demo.demofile = FS_FOpenFileWrite( name );
	if ( !cl->demo.demofile ) {
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	cl->demo.demorecording = qtrue;

	// don't start saving messages until a non-delta compressed message is received
	cl->demo.demowaiting = qtrue;

	cl->demo.isBot = ( cl->netchan.remoteAddress.type == NA_BOT ) ? qtrue : qfalse;
	cl->demo.botReliableAcknowledge = cl->reliableSent;

	// write out the gamestate message
	MSG_Init( &msg, bufData, sizeof( bufData ) );

	// NOTE, MRE: all server->client messages now acknowledge
	int tmp = cl->reliableSent;
	SV_CreateClientGameStateMessage( cl, &msg );
	cl->reliableSent = tmp;

	// finished writing the client packet
	MSG_WriteByte( &msg, svc_EOF );

	// write it to the demo file
	len = LittleLong( cl->netchan.outgoingSequence - 1 );
	FS_Write( &len, 4, cl->demo.demofile );

	len = LittleLong( msg.cursize );
	FS_Write( &len, 4, cl->demo.demofile );
	FS_Write( msg.data, msg.cursize, cl->demo.demofile );

	// the rest of the demo file will be copied from net messages
}

void SV_AutoRecordDemo( client_t *cl ) {
	char demoName[MAX_OSPATH];
	char demoFolderName[MAX_OSPATH];
	char demoFileName[MAX_OSPATH];
	char *demoNames[] = { demoFolderName, demoFileName };
	char date[MAX_OSPATH];
	char folderDate[MAX_OSPATH];
	char demoPlayerName[MAX_NAME_LENGTH];
	time_t rawtime;
	struct tm * timeinfo;
	time( &rawtime );
	timeinfo = localtime( &rawtime );
	strftime( date, sizeof( date ), "%Y-%m-%d_%H-%M-%S", timeinfo );
	timeinfo = localtime( &sv.realMapTimeStarted );
	strftime( folderDate, sizeof( folderDate ), "%Y-%m-%d_%H-%M-%S", timeinfo );
	Q_strncpyz( demoPlayerName, cl->name, sizeof( demoPlayerName ) );
	Q_CleanStr( demoPlayerName );
	Com_sprintf( demoFileName, sizeof( demoFileName ), "%d %s %s %s",
			cl - svs.clients, demoPlayerName, Cvar_VariableString( "mapname" ), date );
	Com_sprintf( demoFolderName, sizeof( demoFolderName ), "%s %s", Cvar_VariableString( "mapname" ), folderDate );
	// sanitize filename
	for ( char **start = demoNames; start - demoNames < (ptrdiff_t)ARRAY_LEN( demoNames ); start++ ) {
		Q_strstrip( *start, "\n\r;:.?*<>|\\/\"", NULL );
	}
	Com_sprintf( demoName, sizeof( demoName ), "autorecord/%s/%s", demoFolderName, demoFileName );
	SV_RecordDemo( cl, demoName );
}

static time_t SV_ExtractTimeFromDemoFolder( char *folder ) {
	size_t timeLen = strlen( "0000-00-00_00-00-00" );
	if ( strlen( folder ) < timeLen ) {
		return 0;
	}
	struct tm timeinfo;
	timeinfo.tm_isdst = 0;
	int numMatched = sscanf( folder + ( strlen(folder) - timeLen ), "%4d-%2d-%2d_%2d-%2d-%2d",
		&timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec);
	if ( numMatched < 6 ) {
		// parsing failed
		return 0;
	}
	timeinfo.tm_year -= 1900;
	return mktime( &timeinfo );
}

static int QDECL SV_DemoFolderTimeComparator( const void *arg1, const void *arg2 ) {
	return SV_ExtractTimeFromDemoFolder( (char *)arg2 ) - SV_ExtractTimeFromDemoFolder( (char *)arg1 );
}

// starts demo recording on all active clients
void SV_BeginAutoRecordDemos() {
	if ( sv_autoDemo->integer ) {
		for ( client_t *client = svs.clients; client - svs.clients < sv_maxclients->integer; client++ ) {
			if ( client->state == CS_ACTIVE && !client->demo.demorecording ) {
				if ( client->netchan.remoteAddress.type != NA_BOT || sv_autoDemoBots->integer ) {
					SV_AutoRecordDemo( client );
				}
			}
		}
		if ( sv_autoDemoMaxMaps->integer > 0 ) {
			char fileList[MAX_QPATH * 500];
			char autorecordDirList[500][MAX_QPATH];
			int autorecordDirListCount = 0;
			char *fileName;
			int i;
			int numFiles = FS_GetFileList( "demos/autorecord", "/", fileList, sizeof( fileList ) );

			fileName = fileList;
			for ( i = 0; i < numFiles; i++ )
			{
				if ( Q_stricmp( fileName, "." ) && Q_stricmp( fileName, ".." ) ) {
					Q_strncpyz( autorecordDirList[autorecordDirListCount++], fileName, MAX_QPATH );
				}
				fileName += strlen( fileName ) + 1;
			}
			qsort( autorecordDirList, autorecordDirListCount, sizeof( autorecordDirList[0] ), SV_DemoFolderTimeComparator );
			for ( i = sv_autoDemoMaxMaps->integer; i < autorecordDirListCount; i++ ) {
				FS_HomeRmdir( va( "demos/autorecord/%s", autorecordDirList[i] ), qtrue );
			}
		}
	}
}

// code is a merge of the cl_main.cpp function of the same name and SV_SendClientGameState in sv_client.cpp
static void SV_Record_f( void ) {
	char		demoName[MAX_OSPATH];
	char		name[MAX_OSPATH];
	int			i;
	char		*s;
	client_t	*cl;

	if ( svs.clients == NULL ) {
		Com_Printf( "cannot record server demo - null svs.clients\n" );
		return;
	}

	if ( Cmd_Argc() > 3 ) {
		Com_Printf( "record <demoname> <clientnum>\n" );
		return;
	}


	if ( Cmd_Argc() == 3 ) {
		int clIndex = atoi( Cmd_Argv( 2 ) );
		if ( clIndex < 0 || clIndex >= sv_maxclients->integer ) {
			Com_Printf( "Unknown client number %d.\n", clIndex );
			return;
		}
		cl = &svs.clients[clIndex];
	} else {
		for ( i=0, cl=svs.clients ; i < sv_maxclients->integer ; i++, cl++ )
		{
			if ( !cl->state )
			{
				continue;
			}

			if ( cl->demo.demorecording )
			{
				continue;
			}

			if ( cl->state == CS_ACTIVE )
			{
				break;
			}
		}
	}

	if (cl - svs.clients >= sv_maxclients->integer) {
		Com_Printf( "No active client could be found.\n" );
		return;
	}

	if ( cl->demo.demorecording ) {
		Com_Printf( "Already recording.\n" );
		return;
	}

	if ( cl->state != CS_ACTIVE ) {
		Com_Printf( "Client is not active.\n" );
		return;
	}

	if ( Cmd_Argc() >= 2 ) {
		s = Cmd_Argv( 1 );
		Q_strncpyz( demoName, s, sizeof( demoName ) );
		Com_sprintf( name, sizeof( name ), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION );
	} else {
		// timestamp the file
		SV_DemoFilename( demoName, sizeof( demoName ) );

		Com_sprintf (name, sizeof(name), "demos/%s.dm_%d", demoName, PROTOCOL_VERSION );

		if ( FS_FileExists( name ) ) {
			Com_Printf( "Record: Couldn't create a file\n");
			return;
 		}
	}

	SV_RecordDemo( cl, demoName );
}

//===========================================================

/*
==================
SV_CompleteMapName
==================
*/
static void SV_CompleteMapName( char *args, int argNum ) {
	if ( argNum == 2 )
		Field_CompleteFilename( "maps", "bsp", qtrue, qfalse );
}

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands( void ) {
	static qboolean	initialized;

	if ( initialized ) {
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("kickbots", SV_KickBots_f);
	Cmd_AddCommand ("kickall", SV_KickAll_f);
	Cmd_AddCommand ("kicknum", SV_KickNum_f);
	Cmd_AddCommand ("clientkick", SV_KickNum_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("systeminfo", SV_Systeminfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);
	Cmd_AddCommand ("map_restart", SV_MapRestart_f);
	Cmd_AddCommand ("sectorlist", SV_SectorList_f);
	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "map", SV_CompleteMapName );
	Cmd_AddCommand ("devmap", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "devmap", SV_CompleteMapName );
//	Cmd_AddCommand ("devmapbsp", SV_Map_f);	// not used in MP codebase, no server BSP_cacheing
	Cmd_AddCommand ("devmapmdl", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "devmapmdl", SV_CompleteMapName );
	Cmd_AddCommand ("devmapall", SV_Map_f);
	Cmd_SetCommandCompletionFunc( "devmapall", SV_CompleteMapName );
	Cmd_AddCommand ("killserver", SV_KillServer_f);
	Cmd_AddCommand ("svsay", SV_ConSay_f);

	Cmd_AddCommand ("forcetoggle", SV_ForceToggle_f);
	Cmd_AddCommand ("svrecord", SV_Record_f);
	Cmd_AddCommand ("svstoprecord", SV_StopRecord_f);
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands( void ) {
#if 0
	// removing these won't let the server start again
	Cmd_RemoveCommand ("heartbeat");
	Cmd_RemoveCommand ("kick");
	Cmd_RemoveCommand ("banUser");
	Cmd_RemoveCommand ("banClient");
	Cmd_RemoveCommand ("status");
	Cmd_RemoveCommand ("serverinfo");
	Cmd_RemoveCommand ("systeminfo");
	Cmd_RemoveCommand ("dumpuser");
	Cmd_RemoveCommand ("map_restart");
	Cmd_RemoveCommand ("sectorlist");
	Cmd_RemoveCommand ("svsay");
#endif
}

