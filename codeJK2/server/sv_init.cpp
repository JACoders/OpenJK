
// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

#include "../client/snd_music.h"	// didn't want to put this in snd_local because of rebuild times etc.
#include "server.h"

/*
Ghoul2 Insert Start
*/
#if !defined(TR_LOCAL_H)
	#include "../renderer/tr_local.h"
#endif

#if !defined (MINIHEAP_H_INC)
	#include "../qcommon/miniheap.h"
#endif

void CM_CleanLeafCache(void);

CMiniHeap *G2VertSpaceServer = NULL;
/*
Ghoul2 Insert End
*/


/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_SetConfigstring: bad index %i\n", index);
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	Z_Free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME ) {
		SV_SendServerCommand( NULL, "cs %i \"%s\"\n", index, val );
	}
}



/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_GetConfigstring: bad index %i\n", index);
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= 1 ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i\n", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[ index ].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= 1 ) {
		Com_Error (ERR_DROP, "SV_GetUserinfo: bad index %i\n", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline( void ) {
	gentity_t			*svent;
	int				entnum;	

	for ( entnum = 0; entnum < ge->num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->inuse) {
			continue;
		}
		if (!svent->linked) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[entnum].baseline = svent->s;
	}
}




/*
===============
SV_Startup

Called when a game is about to begin
===============
*/
void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}

	const int iSize = sizeof(client_t) * 1;
	svs.clients = (struct client_s *) Z_Malloc (iSize, TAG_CLIENTS, qfalse );
	memset(svs.clients,0,iSize);
	svs.numSnapshotEntities = 1 * 4 * 64;
	svs.initialized = qtrue;

	Cvar_Set( "sv_running", "1" );
}


/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
================
*/
void SV_SpawnServer( char *server, ForceReload_e eForceReload, qboolean bAllowScreenDissolve )
{
	int			i;
	int			checksum;

	RE_RegisterMedia_LevelLoadBegin( server, eForceReload, bAllowScreenDissolve );


	Cvar_SetValue( "cl_paused", 0 );
	Cvar_Set( "timescale", "1" );//jic we were skipping

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf ("------ Server Initialization ------\n%s\n", com_version->string);
	Com_Printf ("Server: %s\n",server);	

	// init client structures and svs.numSnapshotEntities 
	if ( !Cvar_VariableIntegerValue("sv_running") ) {
		SV_Startup();
	}		

	// don't let sound stutter and dump all stuff on the hunk
	CL_MapLoading();

	Hunk_Clear();
 	// clear out those shaders, images and Models
	R_InitImages();
	R_InitShaders();
	R_ModelInit();

	// create a heap for Ghoul2 to use for game side model vertex transforms used in collision detection
	if (!G2VertSpaceServer)
	{
		static const int MiniHeapSize=128 * 1024; // maxsize of ghoul2 miniheap
		G2VertSpaceServer	= new CMiniHeap(MiniHeapSize);
	}

	if (svs.snapshotEntities)
	{
		Z_Free(svs.snapshotEntities);
	}
	// allocate the snapshot entities 
	svs.snapshotEntities = (entityState_t *) Z_Malloc (sizeof(entityState_t)*svs.numSnapshotEntities, TAG_CLIENTS, qtrue );

	Music_SetLevelName(server);

	// toggle the server bit so clients can detect that a
	// server has changed
//!@	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set( "nextmap", va("map %s", server) );

	// wipe the entire per-level structure
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
		}
	}

	memset (&sv, 0, sizeof(sv));


	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	sv.time = 1000;
	G2API_SetTime(sv.time,G2T_SV_TIME);

	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld ();
	
	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// run a few frames to allow everything to settle
	for ( i = 0 ;i < 3 ; i++ ) {
		ge->RunFrame( sv.time );
		sv.time += 100;
		G2API_SetTime(sv.time,G2T_SV_TIME);
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline ();

	for (i=0 ; i<1 ; i++) {
		// clear all time counters, because we have reset sv.time
		svs.clients[i].lastPacketTime = 0;
		svs.clients[i].lastConnectTime = 0;
		svs.clients[i].nextSnapshotTime = 0;

		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			// connect the client again
			denied = ge->ClientConnect( i, qfalse, eNO/*qfalse*/ );	// firstTime = qfalse, qbFromSavedGame
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				svs.clients[i].state = CS_CONNECTED;
				// when we get the next packet from a connected client,
				// the new gamestate will be sent
			}
		}
	}	

	// run another frame to allow things to look at all connected clients
	ge->RunFrame( sv.time );
	sv.time += 100;
	G2API_SetTime(sv.time,G2T_SV_TIME);


	// save systeminfo and serverinfo strings
	SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString( CVAR_SYSTEMINFO ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;
	
	// send a heartbeat now so the master will get up to date info
	svs.nextHeartbeatTime = -9999999;

	Hunk_SetMark();
	
	Com_Printf ("-----------------------------------\n");
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init (void) {
	SV_AddOperatorCommands ();

	// serverinfo vars
	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);

	// systeminfo
	Cvar_Get ("helpUsObi", "0", CVAR_SYSTEMINFO );
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_fps = Cvar_Get ("sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get ("sv_timeout", "120", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get ("nextmap", "", CVAR_TEMP );
	sv_spawntarget = Cvar_Get ("spawntarget", "", 0 );

	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);
	sv_testsave = Cvar_Get ("sv_testsave", "0", 0);
	sv_compress_saved_games = Cvar_Get ("sv_compress_saved_games", "1", 0);

}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;
	
	SV_SendServerCommand( NULL, "print \"%s\"", message );
	SV_SendServerCommand( NULL, "disconnect" );

	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < 1 ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// force a snapshot to be sent
				cl->nextSnapshotTime = -1;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg ) {
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown -----\n" );

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_ShutdownGameProgs();

	if (svs.snapshotEntities)
	{
		Z_Free(svs.snapshotEntities);
		svs.snapshotEntities = NULL;
	}
	// remove allocated space for ghoul2 game side model transforms
	delete G2VertSpaceServer;
	G2VertSpaceServer = NULL;

	// free current level
	memset( &sv, 0, sizeof( sv ) );

	// free server static data
	if ( svs.clients ) {
		Z_Free( svs.clients );
	}
	memset( &svs, 0, sizeof( svs ) );

	// Ensure we free any memory used by the leaf cache.
	CM_CleanLeafCache();

	Cvar_Set( "sv_running", "0" );

	Com_Printf( "---------------------------\n" );
}

