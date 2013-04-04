//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "server.h"

/*
Ghoul2 Insert Start
*/
#if !defined(TR_LOCAL_H)
	#include "../renderer/tr_local.h"
#endif

#if !defined (MINIHEAP_H_INC)
#include "../qcommon/MiniHeap.h"
#endif

#include "../qcommon/stringed_ingame.h"

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#endif

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	int		len, i;
	int		maxChunkSize = MAX_STRING_CHARS - 24;
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
			if ( client->state < CS_PRIMED ) {
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}

			len = strlen( val );
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
					Q_strncpyz( buf, &val[sent], maxChunkSize );

					SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd, index, buf );

					sent += (maxChunkSize - 1);
					remaining -= (maxChunkSize - 1);
				}
			} else {
				// standard cs, just send it
				SV_SendServerCommand( client, "cs %i \"%s\"\n", index, val );
			}
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
================
SV_AddConfigstring

================
*/
int SV_AddConfigstring (const char *name, int start, int max)
{
	int		i;
	
	if (!name || !name[0])
	{
		return 0;
	}

	if (name[0] == '/' || name[0] == '\\')
	{
#if _DEBUG
		Com_DPrintf( "WARNING: Leading slash on '%s'\n", name);
#endif
		name++;

		if (!name[0])
		{
			return 0;
		}
	}

	for (i=1 ; i < max ; i++)
	{
		if (sv.configstrings[start+i][0] == 0)
		{	// Didn't find it
			SV_SetConfigstring ((start+i), name);
			break;
		}
		else if (!Q_stricmp(sv.configstrings[start+i], name))
		{
			return i;
		}
	}

	return 0;

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
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
		Cvar_Set( "r_ghoul2animsmooth", "0");
		Cvar_Set( "r_ghoul2unsqashaftersmooth", "0");

	} else {
		// we don't need nearly as many when playing locally
//		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
		svs.numSnapshotEntities = sv_maxclients->integer * 8 * 64;
	}
	svs.initialized = qtrue;

	Cvar_Set( "sv_running", "1" );

}

/*
Ghoul2 Insert Start
*/

 void SV_InitSV(void)
{
	// clear out most of the sv struct
	memset(&sv, 0, (sizeof(sv)));
/*
	sv.mLocalSubBSPIndex = -1;
*/
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
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
	} else {
		// we don't need nearly as many when playing locally
//		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
		svs.numSnapshotEntities = sv_maxclients->integer * 8 * 64;
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

	if (Cvar_VariableValue( "vm_cgame" ))
	{
		Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", "cgame" );
	}
	else
	{
		Com_sprintf( filename, sizeof(filename), "cgamex86.dll" );
	}
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
				if ( svs.clients[i].netchan.remoteAddress.type != NA_BOT ) 
				{
					SV_SendClientMapChange( &svs.clients[i] ) ;
				}
			}
		}	
	}
}

void R_SVModelInit();


#ifdef _XBOX
//To avoid fragmentation, we want everything free by this point.
//Much of this probably violates DLL boundaries, so it's done on
//Xbox only.
extern void NAV_Free(void);
extern void CL_ClearLastLevel(void);
extern int checkminimumplayers_time;
extern void G_ClearVehicles(void);

namespace game 
{
	extern void BG_ClearVehicleLoadInfo(void);
}

extern int g_duelPrintTimer;

void SV_ClearLastLevel(void)
{
	CL_ClearLastLevel();
	game::BG_ClearVehicleLoadInfo();
	NAV_Free();
	checkminimumplayers_time = 0;
	G_ClearVehicles();

	int i;
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			Z_Free( sv.configstrings[i] );
			sv.configstrings[i] = NULL;
		}
	}

	g_duelPrintTimer	= 0;
}
#endif


/*
================
SV_DedicatedSpawn

Clean up and init the needed data for dedicated server mode
================
*/
namespace ui
{
extern void Menus_OpenByName(const char *p);
extern void Menus_CloseAll( void );
}

void SV_DedicatedSpawn(char* map)
{
	//turn off dedicated flag so textures will load
	Cvar_Set("dedicated", "0");

	//clear and restart the render system
	extern void CL_InitRenderer( void );
	CL_InitRenderer();

	// Register sounds, so that muting is turned off and UI sounds work
	cls.soundRegistered = qtrue;
	S_BeginRegistration(ClientManager::NumClients());

	//clear out & init ghoul2 system
//	G2API_CleanG2(G2_CLEAN_ALL);

//	com_serverGhoulInit = true;
//	com_serverGhoulShutdown = false;

	//reload menu stuff
	VM_Call( uivm, UI_INIT, 0 );
	VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_DEDICATED );

	//jsw//clear out the socket buffers for voice in case they got filled while loading
//	g_Voice.EmptyVoiceBuffer();
//	g_Voice.StartVoice();
//	g_Voice.VerifyPlayerList();
	
	//reset dedicated flag so no other textures will load
	Cvar_Set( "dedicated", "1");
}

void SV_FixBrokenRules( void )
{
	int gt = Cvar_VariableIntegerValue( "g_gametype" );
	if( gt == GT_DUEL || gt == GT_POWERDUEL )
	{
		Cvar_SetValue( "fraglimit", 1 );
		Cvar_SetValue( "timelimit", 0 );
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
extern CMiniHeap *G2VertSpaceServer;
extern CMiniHeap CMiniHeap_singleton;

extern void RE_RegisterMedia_LevelLoadBegin(const char *psMapName, ForceReload_e eForceReload);
void SV_SpawnServer( char *server, qboolean killBots, ForceReload_e eForceReload ) {
	int			i;
	int			checksum;
	qboolean	isBot;
	char		systemInfo[16384];
	const char	*p;

	SV_SendMapChange();

	RE_RegisterMedia_LevelLoadBegin(server, eForceReload);

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

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

#ifdef _XBOX
	// disable vsync during load for speed
	qglDisable(GL_VSYNC);
#endif

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

#ifndef DEDICATED
	// make sure all the client stuff is unloaded
	CL_ShutdownAll();
#endif

	CM_ClearMap();

#ifdef _XBOX
	R_DeleteTextures();
#endif

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

#ifdef _XBOX
	SV_ClearLastLevel();
	ClientManager::ActivateClient(0);
#endif

	R_InitSkins();
	R_InitShaders(qtrue);

	// This was in SV_DedicatedSpawn, but it gets in the way of my memory maps:
	if( com_dedicated->integer )
	{
		// Textures have been blown away - need to kill font system so it
		// will re-register shaders when UI re-scans menu files below:
		extern void R_ShutdownFonts( void );
		R_ShutdownFonts();
	}

	ClientManager::ClientActiveRelocate( !com_dedicated->integer && !ClientManager::splitScreenMode );

#if defined(_XBOX) && !defined(FINAL_BUILD)
	//Useful for memory debugging.  Please don't delete.  Comment out if
	//necessary.
	extern void Z_DisplayLevelMemory(int, int, int);
	extern void Z_Details_f(void);
	extern void Z_TagPointers(memtag_t);
	Z_DisplayLevelMemory(0, 0, 0);
	Z_TagPointers( TAG_ALL );
	Z_Details_f();
#endif

	// init client structures and svs.numSnapshotEntities 
	if ( !Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// Do dedicated server-specific startup
	if ( com_dedicated->integer )
	{
		SV_DedicatedSpawn(server);
	}

	// Xbox - Correct various problems with broken rules settings when people
	// change gametype in-game, etc...
	SV_FixBrokenRules();

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
		R_SVModelInit();
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

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString("");
	}

	//rww - RAGDOLL_BEGIN
	G2API_SetTime(svs.time,0);
	//rww - RAGDOLL_END

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	srand(Com_Milliseconds());
	sv.checksumFeed = ( ((int) rand() << 16) ^ rand() ) ^ Com_Milliseconds();
	FS_Restart( sv.checksumFeed );

#ifdef _XBOX
	CL_StartHunkUsers();
	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );
//	RE_LoadWorldMap(va("maps/%s.bsp", server));

	// Start up voice system if it isn't running yet. (ie, if we're on syslink)
	if( !logged_on )
		g_Voice.Initialize();
#else
	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );
#endif

	SV_SendMapChange();

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

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
		G2API_SetTime(svs.time,0);
		//rww - RAGDOLL_END
		VM_Call( gvm, GAME_RUN_FRAME, svs.time );
		SV_BotFrame( svs.time );
		svs.time += 100;
	}
	//rww - RAGDOLL_BEGIN
	G2API_SetTime(svs.time,0);
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
			denied = (char *)VM_ExplicitArgPtr( gvm, VM_Call( gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
//				SV_DropClient( &svs.clients[i], denied );
				SV_DropClient( &svs.clients[i], "@MENUS_LOST_CONNECTION" );
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

					VM_Call( gvm, GAME_CLIENT_BEGIN, i );
				}
			}
		}
	}	

	// run another frame to allow things to look at all the players
	VM_Call( gvm, GAME_RUN_FRAME, svs.time );
	SV_BotFrame( svs.time );
	svs.time += 100;
	//rww - RAGDOLL_BEGIN
	G2API_SetTime(svs.time,0);
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

	// Xbox - Dedicated servers need to do extra work here. Most of this is done in
	// cl_parse normally, but that never runs in this case:
	if ( com_dedicated->integer )
	{
		// Normally, we start advertising when we get the first snapshot.
		// Do it now. This is also necessary so that Net_GetXNKID works below.
		XBL_MM_Advertise();

		// We need to put ourselves into the playerlist.
		xbOnlineInfo.localIndex = DEDICATED_SERVER_INDEX;

		XBPlayerInfo *plyrInfo = &xbOnlineInfo.xbPlayerList[DEDICATED_SERVER_INDEX];
		memset( plyrInfo, 0, sizeof(XBPlayerInfo) );

		// We get the first refIndex
		plyrInfo->refIndex = svs.clientRefNum++;

		// Address information
		plyrInfo->xbAddr = *Net_GetXNADDR( NULL );
		XNetXnAddrToInAddr( &plyrInfo->xbAddr, Net_GetXNKID(), &plyrInfo->inAddr );

		// Gamertag and XUID
		Q_strncpyz( plyrInfo->name, Cvar_VariableString("name"), sizeof(plyrInfo->name) );
		XONLINE_USER *pUser;
		if (logged_on && (pUser = &XBLLoggedOnUsers[ IN_GetMainController() ]) && (pUser->hr == S_OK))
			plyrInfo->xuid = pUser->xuid;
		else
			plyrInfo->xuid.qwUserID = plyrInfo->refIndex;

		plyrInfo->isActive = true;

		// Start up the voice chat session
		g_Voice.JoinSession();

		// And mark ourselves as playing, so that others can join our game:
		XBL_F_SetState( XONLINE_FRIENDSTATE_FLAG_PLAYING, true );
	}
}


/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_BotInitBotLib(void);

void SV_Init (void) {
	SV_AddOperatorCommands ();

	// serverinfo vars
	Cvar_Get ("dmflags", "0", CVAR_SERVERINFO);
	Cvar_Get ("fraglimit", "20", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	
	// Get these to establish them and to make sure they have a default before the menus decide to stomp them.
//	Cvar_Get ("g_maxHolocronCarry", "3", CVAR_SERVERINFO);
	Cvar_Get ("g_privateDuel", "1", CVAR_SERVERINFO );
	Cvar_Get ("g_saberLocking", "1", CVAR_SERVERINFO );
	Cvar_Get ("g_maxForceRank", "6", CVAR_SERVERINFO );
	Cvar_Get ("duel_fraglimit", "10", CVAR_SERVERINFO);
	Cvar_Get ("g_forceBasedTeams", "0", CVAR_SERVERINFO);
	Cvar_Get ("g_duelWeaponDisable", "1", CVAR_SERVERINFO);

	// Fix up g_duelWeaponDisable so it's always correct:
	int weaponDisable = 0;
	for( int i = 0; i < WP_NUM_WEAPONS; i++ )
	{
		if( i != WP_SABER )
			weaponDisable |= (1<<i);
	}
	Cvar_SetValue( "g_duelWeaponDisable", weaponDisable );

	sv_gametype = Cvar_Get ("g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH );
	sv_needpass = Cvar_Get ("g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM );
	Cvar_Get ("sv_keywords", "", CVAR_SERVERINFO);
	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO | CVAR_ROM);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get ("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get ("sv_hostname", "*Jedi*", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_maxclients = Cvar_Get ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
	sv_maxRate = Cvar_Get ("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_minPing = Cvar_Get ("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get ("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get ("sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_allowAnonymous = Cvar_Get ("sv_allowAnonymous", "0", CVAR_SERVERINFO);

	// Live/SystemLink/SplitScreen/BotMatch
	xb_gameType = Cvar_Get("xb_gameType", "0", CVAR_ARCHIVE);

	// systeminfo
	Cvar_Get ("sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
#ifndef DLL_ONLY // bk010216 - for DLL-only servers
	sv_pure = Cvar_Get ("sv_pure", "1", CVAR_SYSTEMINFO );
#else
	sv_pure = Cvar_Get ("sv_pure", "0", CVAR_SYSTEMINFO | CVAR_INIT | CVAR_ROM );
#endif
	Cvar_Get ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get ("sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get ("sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get ("sv_timeout", "200", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get ("nextmap", "", CVAR_TEMP );

#ifndef _XBOX	// No master or downloads on Xbox
	sv_allowDownload = Cvar_Get ("sv_allowDownload", "0", CVAR_SERVERINFO);
	sv_master[0] = Cvar_Get ("sv_master1", MASTER_SERVER_NAME, 0 );
	sv_master[1] = Cvar_Get ("sv_master2", "", CVAR_ARCHIVE );
	sv_master[2] = Cvar_Get ("sv_master3", "", CVAR_ARCHIVE );
	sv_master[3] = Cvar_Get ("sv_master4", "", CVAR_ARCHIVE );
	sv_master[4] = Cvar_Get ("sv_master5", "", CVAR_ARCHIVE );
#endif
	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showghoultraces = Cvar_Get ("sv_showghoultraces", "0", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get ("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);

//	sv_debugserver = Cvar_Get ("sv_debugserver", "0", 0);

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();

	// init the botlib here because we need the pre-compiler in the UI
	SV_BotInitBotLib();

#ifdef _XBOX
	svs.clientRefNum = 0;
#endif
	// Only allocated once, no point in moving it around and fragmenting
	// create a heap for Ghoul2 to use for game side model vertex transforms used in collision detection
	G2VertSpaceServer = &CMiniHeap_singleton;
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
bool dontSendFinalMessage = false;
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;

	if(dontSendFinalMessage) {
		return;
	}
	
	dontSendFinalMessage = true;
	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\"", message );
					SV_SendServerCommand( cl, "disconnect @MENUS_LOST_CONNECTION" );
				}
				// force a snapshot to be sent
				cl->nextSnapshotTime = -1;
				SV_SendClientSnapshot( cl );
			}
		}
	}
	dontSendFinalMessage = false;
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

	//Replaced com_errorEntered check with a recursion check inside
	//SV_FinalMessage.  How else can the clients know to disconnect?
	if ( svs.clients/* && !com_errorEntered*/ ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
#ifndef _XBOX	// No master on Xbox
	SV_MasterShutdown();
#endif
	SV_ShutdownGameProgs();
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

#ifdef _XBOX
	// If we were advertising on Live, remove the listing. This also unregisters
	// the server's key. SysLink keys are never unregistered, so we don't do anything
	// special here for them.
	if ( logged_on )
		XBL_MM_Shutdown( true );

	// Leave the voice session
	g_Voice.LeaveSession();

	// Tear down voice now if we're on system link (Live keeps it active)
	if( !logged_on )
		g_Voice.Shutdown();

	if( logged_on )
	{
		XBL_F_OnClientLeaveSession();
		XBL_PL_OnClientLeaveSession();
	}

	// Wipe our player list - this is important
	memset( &xbOnlineInfo, 0, sizeof(xbOnlineInfo) );
#endif

	// disconnect any local clients
	CL_Disconnect( qfalse );
}

