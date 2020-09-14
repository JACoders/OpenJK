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
#include "ghoul2/G2.h"
#include "qcommon/cm_public.h"
#include "qcommon/MiniHeap.h"
#include "qcommon/stringed_ingame.h"
#include "sv_gameapi.h"

/*
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	len = strlen(sv.configstrings[index]);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &sv.configstrings[index][sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"\n", index,
			sv.configstrings[index] );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index < MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && client->gentity &&
			(client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
			continue;
		}
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	int		i;
	client_t	*client;

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
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevent clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}

			SV_SendConfigstring(client, index);
		}
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
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i\n", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
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
	if ( index < 0 || index >= sv_maxclients->integer ) {
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
	sharedEntity_t *svent;
	int				entnum;

	for ( entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->r.linked) {
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
SV_BoundMaxClients

===============
*/
void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "8", 0 );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}

/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = (struct client_s *)Z_Malloc (sizeof(client_t) * sv_maxclients->integer, TAG_CLIENTS, qtrue );
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
		Cvar_Set( "r_ghoul2animsmooth", "0");
		Cvar_Set( "r_ghoul2unsqashaftersmooth", "0");

	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
	SV_ChallengeInit();
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		Cvar_Set( "sv_killserver", "0" );
	}

	Cvar_Set( "sv_running", "1" );
}

/*
Ghoul2 Insert Start
*/

 void SV_InitSV(void)
{
	// clear out most of the sv struct
	memset(&sv, 0, (sizeof(sv)));
	sv.mLocalSubBSPIndex = -1;
}
/*
Ghoul2 Insert End
*/

/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = (struct client_s *)Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	Z_Free( svs.clients );

	// allocate new clients
	svs.clients = (struct client_s *)Z_Malloc ( sv_maxclients->integer * sizeof(client_t), TAG_CLIENTS, qtrue );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof(client_t) );

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );

	// allocate new snapshot entities
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
}

/*
================
SV_ClearServer
================
*/
void SV_ClearServer(void) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
		}
	}

//	CM_ClearMap();

	/*
Ghoul2 Insert Start
*/

	// nope, can't do this anymore.. sv contains entitystates with STL in them.
//	memset (&sv, 0, sizeof(sv));
 	SV_InitSV();
/*
Ghoul2 Insert End
*/
//	Com_Memset (&sv, 0, sizeof(sv));
}

/*
================
SV_TouchCGame

  touch the cgame.vm so that a pure client can load it if it's in a seperate pk3
================
*/
void SV_TouchCGame(void) {
	fileHandle_t	f;
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof(filename), "cgamex86.dll" );

	FS_FOpenFileRead( filename, &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

void SV_SendMapChange(void)
{
	int		i;

	if (svs.clients)
	{
		for (i=0 ; i<sv_maxclients->integer ; i++)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				if ( svs.clients[i].netchan.remoteAddress.type != NA_BOT ||
					svs.clients[i].demo.demorecording )
				{
					SV_SendClientMapChange( &svs.clients[i] ) ;
				}
			}
		}
	}
}

extern void SV_SendClientGameState( client_t *client );
/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots, ForceReload_e eForceReload ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	char		systemInfo[16384];
	const char	*p;

	SV_StopAutoRecordDemos();

	SV_SendMapChange();

	re->RegisterMedia_LevelLoadBegin(server, eForceReload);

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();
	svs.gameStarted = qfalse;

	Com_Printf ("------ Server Initialization ------\n");
	Com_Printf ("Server: %s\n",server);

/*
Ghoul2 Insert Start
*/
 	// de allocate the snapshot entities
	if (svs.snapshotEntities)
	{
		delete[] svs.snapshotEntities;
		svs.snapshotEntities = NULL;
	}
/*
Ghoul2 Insert End
*/

	SV_SendMapChange();

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

#ifndef DEDICATED
	// make sure all the client stuff is unloaded
	CL_ShutdownAll( qfalse );
#endif

	CM_ClearMap();

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

	re->InitSkins();
	re->InitShaders(qtrue);

	// init client structures and svs.numSnapshotEntities
	if ( !Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	SV_SendMapChange();

/*
Ghoul2 Insert Start
*/
 	// clear out those shaders, images and Models as long as this
	// isnt a dedicated server.
	/*
	if ( !com_dedicated->integer )
	{
#ifndef DEDICATED
		R_InitImages();

		R_InitShaders();

		R_ModelInit();
#endif
	}
	else
	*/
	if (com_dedicated->integer)
	{
		re->SVModelInit();
	}

	SV_SendMapChange();

	// clear pak references
	FS_ClearPakReferences(0);

/*
Ghoul2 Insert Start
*/
	// allocate the snapshot entities on the hunk
//	svs.snapshotEntities = (struct entityState_s *)Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// allocate the snapshot entities
	svs.snapshotEntities = new entityState_s[svs.numSnapshotEntities];
	// we CAN afford to do this here, since we know the STL vectors in Ghoul2 are empty
	memset(svs.snapshotEntities, 0, sizeof(entityState_t)*svs.numSnapshotEntities);

/*
Ghoul2 Insert End
*/

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set( "nextmap", "map_restart 0");
//	Cvar_Set( "nextmap", va("map %s", server) );

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// save when the server started for each client already connected
		if (svs.clients[i].state >= CS_CONNECTED) {
			svs.clients[i].oldServerTime = svs.time;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	//rww - RAGDOLL_BEGIN
	re->G2API_SetTime(sv.time,0);
	//rww - RAGDOLL_END

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	srand(Com_Milliseconds());
	sv.checksumFeed = ( ((int) rand() << 16) ^ rand() ) ^ Com_Milliseconds();
	FS_Restart( sv.checksumFeed );

	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	SV_SendMapChange();

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	time( &sv.realMapTimeStarted );
	sv.demosPruned = qfalse;

	// clear physics interaction links
	SV_ClearWorld ();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// don't allow a map_restart if game is modified
	sv_gametype->modified = qfalse;

	// run a few frames to allow everything to settle
	for ( i = 0 ;i < 3 ; i++ ) {
		//rww - RAGDOLL_BEGIN
		re->G2API_SetTime(sv.time,0);
		//rww - RAGDOLL_END
		GVM_RunFrame( sv.time );
		SV_BotFrame( sv.time );
		sv.time += 100;
		svs.time += 100;
	}
	//rww - RAGDOLL_BEGIN
	re->G2API_SetTime(sv.time,0);
	//rww - RAGDOLL_END

	// create a baseline for more efficient communications
	SV_CreateBaseline ();

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				if ( killBots ) {
					SV_DropClient( &svs.clients[i], "" );
					continue;
				}
				isBot = qtrue;
			}
			else {
				isBot = qfalse;
			}

			// connect the client again
			denied = GVM_ClientConnect( i, qfalse, isBot );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				if( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				}
				else {
					client_t		*client;
					sharedEntity_t	*ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = -1;
					client->nextSnapshotTime = svs.time;	// generate a snapshot immediately

					GVM_ClientBegin( i );
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	GVM_RunFrame( sv.time );
	SV_BotFrame( sv.time );
	sv.time += 100;
	svs.time += 100;
	//rww - RAGDOLL_BEGIN
	re->G2API_SetTime(sv.time,0);
	//rww - RAGDOLL_END

	if ( sv_pure->integer ) {
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		p = FS_LoadedPakChecksums();
		Cvar_Set( "sv_paks", p );
		if (strlen(p) == 0) {
			Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
		}
		p = FS_LoadedPakNames();
		Cvar_Set( "sv_pakNames", p );

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if ( com_dedicated->integer ) {
			SV_TouchCGame();
		}
	}
	else {
		Cvar_Set( "sv_paks", "" );
		Cvar_Set( "sv_pakNames", "" );
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = FS_ReferencedPakChecksums();
	Cvar_Set( "sv_referencedPaks", p );
	p = FS_ReferencedPakNames();
	Cvar_Set( "sv_referencedPakNames", p );

	// save systeminfo and serverinfo strings
	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Hunk_SetMark();

	/* MrE: 2000-09-13: now called in CL_DownloadsComplete
	// don't call when running dedicated
	if ( !com_dedicated->integer ) {
		// note that this is called after setting the hunk mark with Hunk_SetMark
		CL_StartHunkUsers();
	}
	*/

	for ( client_t *client = svs.clients; client - svs.clients < sv_maxclients->integer; client++) {
		// bots will not request gamestate, so it must be manually sent
		// cannot do this above where it says it will because mapname is not set at that time
		if ( client->netchan.remoteAddress.type == NA_BOT && client->demo.demorecording ) {
			SV_SendClientGameState( client );
		}
	}

	SV_BeginAutoRecordDemos();
}


/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_BotInitBotLib(void);

#ifdef DEDICATED

#define G2_VERT_SPACE_SERVER_SIZE 256
IHeapAllocator *G2VertSpaceServer = NULL;
CMiniHeap IHeapAllocator_singleton(G2_VERT_SPACE_SERVER_SIZE * 1024);


/*
================
CL_RefPrintf

DLL glue
================
*/
void QDECL SV_RefPrintf( int print_level, const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( print_level == PRINT_ALL ) {
		Com_Printf ("%s", msg);
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf (S_COLOR_YELLOW "%s", msg);		// yellow
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf (S_COLOR_RED "%s", msg);		// red
	}
}

qboolean Com_TheHunkMarkHasBeenMade(void);

//qcommon/vm.cpp
extern vm_t *currentVM;

//qcommon/cm_load.cpp
extern void *gpvCachedMapDiskImage;
extern qboolean gbUsingCachedMapDataRightNow;

static char *GetSharedMemory( void ) { return sv.mSharedMemory; }
static vm_t *GetCurrentVM( void ) { return currentVM; }
static void *CM_GetCachedMapDiskImage( void ) { return gpvCachedMapDiskImage; }
static void CM_SetCachedMapDiskImage( void *ptr ) { gpvCachedMapDiskImage = ptr; }
static void CM_SetUsingCache( qboolean usingCache ) { gbUsingCachedMapDataRightNow = usingCache; }

//server stuff D:
extern void SV_GetConfigstring( int index, char *buffer, int bufferSize );
extern void SV_SetConfigstring( int index, const char *val );

static IHeapAllocator *GetG2VertSpaceServer( void ) {
	return G2VertSpaceServer;
}

refexport_t	*re = NULL;

static void SV_InitRef( void ) {
	static refimport_t ri;
	refexport_t *ret;

	Com_Printf( "----- Initializing Renderer ----\n" );

	memset( &ri, 0, sizeof( ri ) );

	//set up the import table
	ri.Printf = SV_RefPrintf;
	ri.Error = Com_Error;
	ri.OPrintf = Com_OPrintf;
	ri.Milliseconds = Sys_Milliseconds2; //FIXME: unix+mac need this
	ri.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	ri.Hunk_FreeTempMemory = Hunk_FreeTempMemory;
	ri.Hunk_Alloc = Hunk_Alloc;
	ri.Hunk_MemoryRemaining = Hunk_MemoryRemaining;
	ri.Z_Malloc = Z_Malloc;
	ri.Z_Free = Z_Free;
	ri.Z_MemSize = Z_MemSize;
	ri.Z_MorphMallocTag = Z_MorphMallocTag;
	ri.Cmd_ExecuteString = Cmd_ExecuteString;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ArgsBuffer = Cmd_ArgsBuffer;
	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_VariableStringBuffer = Cvar_VariableStringBuffer;
	ri.Cvar_VariableString = Cvar_VariableString;
	ri.Cvar_VariableValue = Cvar_VariableValue;
	ri.Cvar_VariableIntegerValue = Cvar_VariableIntegerValue;
	ri.Sys_LowPhysicalMemory = Sys_LowPhysicalMemory;
	ri.SE_GetString = SE_GetString;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_FreeFileList = FS_FreeFileList;
	ri.FS_Read = FS_Read;
	ri.FS_ReadFile = FS_ReadFile;
	ri.FS_FCloseFile = FS_FCloseFile;
	ri.FS_FOpenFileRead = FS_FOpenFileRead;
	ri.FS_FOpenFileWrite = FS_FOpenFileWrite;
	ri.FS_FOpenFileByMode = FS_FOpenFileByMode;
	ri.FS_FileExists = FS_FileExists;
	ri.FS_FileIsInPAK = FS_FileIsInPAK;
	ri.FS_ListFiles = FS_ListFiles;
	ri.FS_Write = FS_Write;
	ri.FS_WriteFile = FS_WriteFile;
	ri.CM_BoxTrace = CM_BoxTrace;
	ri.CM_DrawDebugSurface = CM_DrawDebugSurface;
//	ri.CM_CullWorldBox = CM_CullWorldBox;
//	ri.CM_TerrainPatchIterate = CM_TerrainPatchIterate;
//	ri.CM_RegisterTerrain = CM_RegisterTerrain;
//	ri.CM_ShutdownTerrain = CM_ShutdownTerrain;
	ri.CM_ClusterPVS = CM_ClusterPVS;
	ri.CM_LeafArea = CM_LeafArea;
	ri.CM_LeafCluster = CM_LeafCluster;
	ri.CM_PointLeafnum = CM_PointLeafnum;
	ri.CM_PointContents = CM_PointContents;
	ri.Com_TheHunkMarkHasBeenMade = Com_TheHunkMarkHasBeenMade;
//	ri.S_RestartMusic = S_RestartMusic;
//	ri.SND_RegisterAudio_LevelLoadEnd = SND_RegisterAudio_LevelLoadEnd;
//	ri.CIN_RunCinematic = CIN_RunCinematic;
//	ri.CIN_PlayCinematic = CIN_PlayCinematic;
//	ri.CIN_UploadCinematic = CIN_UploadCinematic;
//	ri.CL_WriteAVIVideoFrame = CL_WriteAVIVideoFrame;

	// g2 data access
	ri.GetSharedMemory = GetSharedMemory;

	// (c)g vm callbacks
	ri.GetCurrentVM = GetCurrentVM;
//	ri.CGVMLoaded = CGVMLoaded;
//	ri.CGVM_RagCallback = CGVM_RagCallback;

	// ugly win32 backend
	ri.CM_GetCachedMapDiskImage = CM_GetCachedMapDiskImage;
	ri.CM_SetCachedMapDiskImage = CM_SetCachedMapDiskImage;
	ri.CM_SetUsingCache = CM_SetUsingCache;

	//FIXME: Might have to do something about this...
	ri.GetG2VertSpaceServer = GetG2VertSpaceServer;
	G2VertSpaceServer = &IHeapAllocator_singleton;

	ret = GetRefAPI( REF_API_VERSION, &ri );

//	Com_Printf( "-------------------------------\n");

	if ( !ret ) {
		Com_Error (ERR_FATAL, "Couldn't initialize refresh" );
	}

	re = ret;
}
#endif

void SV_Init (void) {

	time( &svs.startTime );

	SV_AddOperatorCommands ();

	// serverinfo vars
	Cvar_Get ("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get ("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	Cvar_Get ("capturelimit", "0", CVAR_SERVERINFO);

	// Get these to establish them and to make sure they have a default before the menus decide to stomp them.
	Cvar_Get ("g_maxHolocronCarry", "3", CVAR_SERVERINFO);
	Cvar_Get ("g_privateDuel", "1", CVAR_SERVERINFO );
	Cvar_Get ("g_saberLocking", "1", CVAR_SERVERINFO );
	Cvar_Get ("g_maxForceRank", "7", CVAR_SERVERINFO );
	Cvar_Get ("duel_fraglimit", "10", CVAR_SERVERINFO);
	Cvar_Get ("g_forceBasedTeams", "0", CVAR_SERVERINFO);
	Cvar_Get ("g_duelWeaponDisable", "1", CVAR_SERVERINFO);

	sv_gametype = Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH, "Server gametype value" );
	sv_needpass = Cvar_Get ("g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, "Server needs password to join" );
	Cvar_Get ("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get ("sv_privateClients", "0", CVAR_SERVERINFO, "Number of reserved client slots available with password" );
	Cvar_CheckRange( sv_privateClients, 0, MAX_CLIENTS, qtrue );
	sv_hostname = Cvar_Get ("sv_hostname", "*Jedi*", CVAR_SERVERINFO | CVAR_ARCHIVE, "The name of the server that is displayed in the serverlist" );
	sv_maxclients = Cvar_Get ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH, "Max. connected clients" );


	//cvar_t	*sv_ratePolicy;		// 1-2
	//cvar_t	*sv_clientRate;
	sv_ratePolicy = Cvar_Get( "sv_ratePolicy", "1", CVAR_ARCHIVE_ND, "Determines which policy of enforcement is used for client's \"rate\" cvar" );
	Cvar_CheckRange(sv_ratePolicy, 1, 2, qtrue);
	sv_clientRate = Cvar_Get( "sv_clientRate", "50000", CVAR_ARCHIVE_ND);
	sv_minRate = Cvar_Get ("sv_minRate", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO, "Min bandwidth rate allowed on server. Use 0 for unlimited." );
	sv_maxRate = Cvar_Get ("sv_maxRate", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO, "Max bandwidth rate allowed on server. Use 0 for unlimited." );
	sv_minPing = Cvar_Get ("sv_minPing", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get ("sv_maxPing", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get ("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, "Protect against flooding of server commands" );
	sv_floodProtectSlow = Cvar_Get ("sv_floodProtectSlow", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, "Use original method of delaying commands with flood protection" );
	// systeminfo
	Cvar_Get ("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM, "Allow cheats on server if set to 1" );
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_pure = Cvar_Get ("sv_pure", "0", CVAR_SYSTEMINFO, "Pure server" );
	Cvar_Get ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get ("sv_privatePassword", "", CVAR_TEMP );
	sv_snapsMin = Cvar_Get ("sv_snapsMin", "10", CVAR_ARCHIVE_ND ); // 1 <=> sv_snapsMax
	sv_snapsMax = Cvar_Get ("sv_snapsMax", "40", CVAR_ARCHIVE_ND ); // sv_snapsMin <=> sv_fps
	sv_snapsPolicy = Cvar_Get ("sv_snapsPolicy", "1", CVAR_ARCHIVE_ND, "Determines which policy of enforcement is used for client's \"snaps\" cvar");
	Cvar_CheckRange(sv_snapsPolicy, 0, 2, qtrue);
	sv_fps = Cvar_Get ("sv_fps", "40", CVAR_SERVERINFO, "Server frames per second" );
	sv_timeout = Cvar_Get ("sv_timeout", "200", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get ("nextmap", "", CVAR_TEMP );

	sv_allowDownload = Cvar_Get ("sv_allowDownload", "0", CVAR_SERVERINFO, "Allow clients to download mod files via UDP from the server");
	sv_master[0] = Cvar_Get ("sv_master1", MASTER_SERVER_NAME, CVAR_PROTECTED );
	sv_master[1] = Cvar_Get ("sv_master2", JKHUB_MASTER_SERVER_NAME, CVAR_PROTECTED);
	for(int index = 2; index < MAX_MASTER_SERVERS; index++)
		sv_master[index] = Cvar_Get(va("sv_master%d", index + 1), "", CVAR_ARCHIVE_ND|CVAR_PROTECTED);
	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showghoultraces = Cvar_Get ("sv_showghoultraces", "0", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get ("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get ("sv_lanForceRate", "1", CVAR_ARCHIVE_ND );

	sv_filterCommands = Cvar_Get( "sv_filterCommands", "1", CVAR_ARCHIVE );

//	sv_debugserver = Cvar_Get ("sv_debugserver", "0", 0);

	sv_autoDemo = Cvar_Get( "sv_autoDemo", "0", CVAR_ARCHIVE_ND | CVAR_SERVERINFO, "Automatically take server-side demos" );
	sv_autoDemoBots = Cvar_Get( "sv_autoDemoBots", "0", CVAR_ARCHIVE_ND, "Record server-side demos for bots" );
	sv_autoDemoMaxMaps = Cvar_Get( "sv_autoDemoMaxMaps", "0", CVAR_ARCHIVE_ND );

	sv_legacyFixes = Cvar_Get( "sv_legacyFixes", "1", CVAR_ARCHIVE );

	sv_banFile = Cvar_Get( "sv_banFile", "serverbans.dat", CVAR_ARCHIVE, "File to use to store bans and exceptions" );

	sv_maxOOBRate = Cvar_Get("sv_maxOOBRate", "1000", CVAR_ARCHIVE, "Maximum rate of handling incoming server commands" );
	sv_maxOOBRateIP = Cvar_Get("sv_maxOOBRateIP", "1", CVAR_ARCHIVE, "Maximum rate of handling incoming server commands per IP address" );
	sv_autoWhitelist = Cvar_Get("sv_autoWhitelist", "1", CVAR_ARCHIVE, "Save player IPs to allow them using server during DOS attack" );

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SV_BotInitBotLib();

	// Load saved bans
	Cbuf_AddText("sv_rehashbans\n");

	// Load IP whitelist
	SVC_LoadWhitelist();

	// Only allocated once, no point in moving it around and fragmenting
	// create a heap for Ghoul2 to use for game side model vertex transforms used in collision detection
#ifdef DEDICATED
	SV_InitRef();
#endif
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

	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\"", message );
					SV_SendServerCommand( cl, "disconnect" );
				}
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
void SV_Shutdown( char *finalmsg )
{
	if ( !com_sv_running || !com_sv_running->integer )
	{
		return;
	}

//	Com_Printf( "----- Server Shutdown -----\n" );

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ChallengeShutdown();
	SV_ShutdownGameProgs();
	svs.gameStarted = qfalse;
/*
Ghoul2 Insert Start
*/
 	// de allocate the snapshot entities
	if (svs.snapshotEntities)
	{
		delete[] svs.snapshotEntities;
		svs.snapshotEntities = NULL;
	}

	// free current level
	SV_ClearServer();
	CM_ClearMap();//jfm: add a clear here since it's commented out in clearServer.  This prevents crashing cmShaderTable on exit.

	// free server static data
	if ( svs.clients ) {
		Z_Free( svs.clients );
	}
	Com_Memset( &svs, 0, sizeof( svs ) );

	Cvar_Set( "sv_running", "0" );
	Cvar_Set("ui_singlePlayerActive", "0");

//	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	if( sv_killserver->integer != 2 )
		CL_Disconnect( qfalse );
}
