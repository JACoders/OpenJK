//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// cl_main.c  -- client main loop

#include "client.h"
#include "../qcommon/stringed_ingame.h"
#include <limits.h>
#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "cl_data.h"
#include "snd_local_console.h"
#include "../xbox/XBLive.h"
#include "../xbox/XBoxCommon.h"
#else
#include "snd_local.h"
#endif

//rwwRMG - added:
//#include "..\qcommon\cm_local.h"
//#include "..\qcommon\cm_landscape.h"

#if !defined(G2_H_INC)
	#include "..\ghoul2\G2_local.h"
#endif

#if !defined (MINIHEAP_H_INC)
#include "../qcommon/miniheap.h"
#endif

#ifdef _DONETPROFILE_
#include "../qcommon/INetProfile.h"
#endif

#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
#include "../renderer/tr_local.h"
#endif

#ifdef _XBOX
#include "../renderer/tr_local.h"
#endif

cvar_t	*cl_nodelta;
cvar_t	*cl_debugMove;

cvar_t	*cl_noprint;
cvar_t	*cl_motd;

cvar_t	*rcon_client_password;
cvar_t	*rconAddress;

cvar_t	*cl_timeout;
cvar_t	*cl_maxpackets;
cvar_t	*cl_packetdup;
cvar_t	*cl_timeNudge;
cvar_t	*cl_showTimeDelta;
cvar_t	*cl_freezeDemo;

cvar_t	*cl_shownet;
cvar_t	*cl_showSend;
cvar_t	*cl_timedemo;
cvar_t	*cl_avidemo;
cvar_t	*cl_forceavidemo;

cvar_t	*cl_freelook;
cvar_t	*cl_sensitivity;

cvar_t	*cl_mouseAccel;
cvar_t	*cl_showMouseRate;

cvar_t	*m_pitchVeh;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;
cvar_t	*m_filter;

#ifdef _XBOX
//MAP HACK
cvar_t	*cl_mapname;
#endif

cvar_t	*cl_activeAction;

cvar_t	*cl_motdString;

cvar_t	*cl_allowDownload;
cvar_t	*cl_allowAltEnter;
cvar_t	*cl_conXOffset;
cvar_t	*cl_inGameVideo;

cvar_t	*cl_serverStatusResendTime;
cvar_t	*cl_trn;
cvar_t	*cl_framerate;

cvar_t	*cl_autolodscale;

vec3_t cl_windVec;

#ifdef USE_CD_KEY
char	cl_cdkey[34] = "                                ";


#endif	// USE_CD_KEY

// MATT - changed this to a pointer for splitscreen client swapping
//clientActive_t		g_cl;
clientActive_t		 *cl = NULL;	//&g_cl;

//clientConnection_t	g_clc;
clientConnection_t	*clc = NULL;	//&g_clc;

clientStatic_t		cls;
vm_t				*cgvm;

bool preserveTextures = false;

// Structure containing functions exported from refresh DLL
refexport_t	re;

CMiniHeap *G2VertSpaceClient = 0;

#if defined __USEA3D && defined __A3D_GEOM
	void hA3Dg_ExportRenderGeom (refexport_t *incoming_re);
#endif

extern void SV_BotFrame( int time );
void CL_CheckForResend( void );
void CL_ShowIP_f(void);
//void CL_ServerStatus_f(void);
//void CL_ServerStatusResponse( netadr_t from, msg_t *msg );

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_AddReliableCommand

The given command will be transmitted to the server, and is gauranteed to
not have future usercmd_t executed before it is executed
======================
*/
void CL_AddReliableCommand( const char *cmd ) {
	int		index;

	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	if ( clc->reliableSequence - clc->reliableAcknowledge > MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "Client command overflow" );
	}
	clc->reliableSequence++;
	index = clc->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( clc->reliableCommands[ index ], cmd, sizeof( clc->reliableCommands[ index ] ) );
}

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand( void ) {
	int r, index, l;

	r = clc->reliableSequence - ((int)(random()) * 5);
	index = clc->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	l = strlen(clc->reliableCommands[ index ]);
	if ( l >= MAX_STRING_CHARS - 1 ) {
		l = MAX_STRING_CHARS - 2;
	}
	clc->reliableCommands[ index ][ l ] = '\n';
	clc->reliableCommands[ index ][ l+1 ] = '\0';
}

/*
======================
CL_MakeMonkeyDoLaundry
======================
*/
void CL_MakeMonkeyDoLaundry( void ) {
	if ( Sys_MonkeyShouldBeSpanked() ) {
		if ( !(cls.framecount & 255) ) {
			if ( random() < 0.1 ) {
				CL_ChangeReliableCommand();
			}
		}
	}
}

//======================================================================

/*
=====================
CL_ShutdownAll
=====================
*/
void CL_ShutdownAll(void) {

#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
	//so it doesn't barf on shutdown saying refentities belong to each other
	tr.refdef.num_entities = 0;
#endif

	// clear sounds
	S_DisableSounds();
	// shutdown CGame
	CL_ShutdownCGame();

	if ( !com_dedicated->integer )
	{
		// shutdown UI
		CL_ShutdownUI();

		// shutdown the renderer
		if ( re.Shutdown ) {
			re.Shutdown( qfalse );		// don't destroy window or context
		}
	}
	else
	{
		VM_Call( uivm, UI_SHUTDOWN );
	}

#ifndef _XBOX
	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
#endif
	cls.rendererStarted = qfalse;
	cls.soundRegistered = qfalse;
}


#ifdef _XBOX
//To avoid fragmentation, we want everything free by this point.
//Much of this probably violates DLL boundaries, so it's done on
//Xbox only.
extern void R_DestroyWireframeMap(void);
extern void Sys_IORequestQueueClear(void);
extern void AS_FreePartial(void);
extern void Cvar_Defrag(void);
extern void R_ModelFree(void);
extern void Ghoul2InfoArray_Free(void);
extern void CM_Free(void);
extern void G_ClPtrClear(void);
extern void NPC_NPCPtrsClear(void);
extern void Sys_StreamRequestQueueClear(void);
extern void RemoveAllWP(void);
extern void BG_ClearVehicleParseParms(void);
extern void NAV_ClearStoredWaypoints(void);
extern void ClearTheBonePool(void);
extern void IN_KillRumbleScripts(void);
extern int NumMiscEnts;

// Various client-side siege state:
extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;
extern int cgSiegeRoundBeganTime;
extern int cgSiegeRoundCountTime;
extern int cg_siegeDeathTime;
extern int g_siegeRespawnCheck;
extern qboolean cg_vehPmoveSet;

namespace cgame
{
	extern int bgNumAnimEvents;
	extern void BG_ClearVehicleLoadInfo(void);
};

void CL_ClearLastLevel(void)
{
	Z_TagFree(TAG_UI_ALLOC);
	Z_TagFree(TAG_CG_UI_ALLOC);
	Z_TagFree(TAG_BG_ALLOC);
	CM_Free();
	R_DestroyWireframeMap();
	Ghoul2InfoArray_Free();
	CM_FreeShaderText();
	R_ModelFree();
	Sys_IORequestQueueClear();
	Sys_StreamRequestQueueClear();
	AS_FreePartial();
	Cvar_Defrag();
	G_ClPtrClear();
	NPC_NPCPtrsClear();
	RemoveAllWP();
	BG_ClearVehicleParseParms();
	cgame::BG_ClearVehicleLoadInfo();
	NAV_ClearStoredWaypoints();
	R_DeleteTextures();
	ClearTheBonePool();
	IN_KillRumbleScripts();			// Trying to fix rumble when quitting - wrong place?

	NumMiscEnts = 0;
	cgame::bgNumAnimEvents = 1;
	cg_vehPmoveSet = qfalse;

	cgSiegeRoundState = 0;
	cgSiegeRoundTime = 0;
	cgSiegeRoundBeganTime = 0;
	cgSiegeRoundCountTime = 0;
	cg_siegeDeathTime = 0;
	g_siegeRespawnCheck = 0;
}
#endif


/*
=================
CL_FlushMemory

Called by CL_MapLoading, CL_Connect_f, CL_PlayDemo_f, and CL_ParseGamestate the only
ways a client gets into a game
Also called by Com_Error
=================
*/
void CL_FlushMemory( void ) {

	// shutdown all the client stuff
	CL_ShutdownAll();

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		// clear collision map data
		CM_ClearMap();
		// clear the whole hunk
		Hunk_Clear();

		//clear everything else to avoid fragmentation
#ifdef _XBOX
		CL_ClearLastLevel();

		ClientManager::ClientActiveRelocate( false );

#ifdef _DEBUG
		//Useful for memory debugging.  Please don't delete.  Comment out if
		//necessary.
		extern void Z_DisplayLevelMemory(int, int, int);
		extern void Z_Details_f(void);
		extern void Z_TagPointers(memtag_t);
		Z_DisplayLevelMemory(0, 0, 0);
		Z_Details_f();
		#endif
#endif
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	CL_StartHunkUsers();
}

/*
=====================
CL_MapLoading

A local server is starting to load a map, so update the
screen to let the user know about it, then dump all client
memory on the hunk from cgame, ui, and renderer
=====================
*/
void CL_MapLoading( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}

	// Set this to localhost.
	Cvar_Set( "cl_currentServerAddress", "Localhost");

	Con_Close();
	cls.keyCatchers = 0;

	// if we are already connected to the local host, stay connected
	if ( cls.state >= CA_CONNECTED && !Q_stricmp( cls.servername, "localhost" ) )
	{
		CM_START_LOOP();

		cls.state = CA_CONNECTED;		// so the connect screen is drawn
		Com_Memset( clc->serverMessage, 0, sizeof( clc->serverMessage ) );
		Com_Memset( &cl->gameState, 0, sizeof( cl->gameState ) );
		clc->lastPacketSentTime = -9999;
		CM_END_LOOP();

		SCR_UpdateScreen();
	}
	else
	{
		// clear nextmap so the cinematic shutdown doesn't execute it
		Cvar_Set( "nextmap", "" );
		CL_Disconnect( qtrue, qfalse );	// Special flag to not delete textures - we need them to draw the connect screen!
		Q_strncpyz( cls.servername, "localhost", sizeof(cls.servername) );

		CM_START_LOOP();

		cls.state = CA_CHALLENGING;		// so the connect screen is drawn
		cls.keyCatchers = 0;
		SCR_UpdateScreen();
		clc->connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls.servername, &clc->serverAddress);

		CM_END_LOOP();

		// we don't need a challenge on the localhost

		CL_CheckForResend();
	}
}

/*
=====================
CL_ClearState

Called before parsing a gamestate
=====================
*/
void CL_ClearState (void) {

//	S_StopAllSounds();

 	Com_Memset( cl, 0, sizeof( *cl ) );
}


/*
=====================
CL_Disconnect

Called when a connection, demo, or cinematic is being terminated.
Goes from a connected state to either a menu state or a console state
Sends a disconnect message to the server
This is also called on Com_Error and Com_Quit, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect( qboolean showMainMenu, qboolean deleteTextures ) {
	if ( !com_cl_running || !com_cl_running->integer ) {
		return;
	}

#ifdef _XBOX
	Cvar_Set("r_norefresh", "0");
#endif

	// shutting down the client so enter full screen ui mode
	Cvar_Set("r_uiFullScreen", "1");

	if ( uivm && showMainMenu ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}

	SCR_StopCinematic ();
	S_ClearSoundBuffer();

#ifdef _XBOX
//	extern qboolean RE_RegisterImages_LevelLoadEnd(void);
//	RE_RegisterImages_LevelLoadEnd();
	if( deleteTextures && !preserveTextures )
		R_DeleteTextures();
#endif

	// send a disconnect message to the server
	// send it a few times in case one is dropped
#ifdef _XBOX
	CM_START_LOOP();
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif
	if ( cls.state >= CA_CONNECTED ) {
		CL_AddReliableCommand( "disconnect" );
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
	}

	CL_ClearState ();

	// wipe the client connection
	Com_Memset( clc, 0, sizeof( *clc ) );

	cls.state = CA_DISCONNECTED;
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//			ClientManager::ActiveClient().state = cls.state;
	CM_END_LOOP();
#endif

	// not connected to a pure server anymore
	cl_connectedToPureServer = qfalse;
	cl_connectedGAME = 0;
	cl_connectedCGAME = 0;
	cl_connectedUI = 0;
}


/*
===================
CL_ForwardCommandToServer

adds the current command line as a clientCommand
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void CL_ForwardCommandToServer( const char *string ) {
	char	*cmd;

	cmd = Cmd_Argv(0);

	// ignore key up commands
	if ( cmd[0] == '-' ) {
		return;
	}

#ifdef _XBOX	// No demos on Xbox
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if(ClientManager::ActiveClient().state < CA_CONNECTED || cmd[0] == '+' ) {
//			return;
//		}
//	}
//	else
	if (cls.state < CA_CONNECTED || cmd[0] == '+' ) {
#else
	if (clc->demoplaying || cls.state < CA_CONNECTED || cmd[0] == '+' ) {
#endif
		Com_Printf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( string );
	} else {
		CL_AddReliableCommand( cmd );
	}
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f( void ) {
	SCR_StopCinematic();
	Cvar_Set("ui_singlePlayerActive", "0");

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if ( ClientManager::ActiveClient().state != CA_DISCONNECTED && ClientManager::ActiveClient().state != CA_CINEMATIC ) {
//			Com_Error (ERR_DISCONNECT, "Disconnected from server");
//		}
//	}
//	else
#endif
	if ( cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC ) {
		Com_Error (ERR_DISCONNECT, "Disconnected from server");
	}
}


/*
================
CL_Reconnect_f

================
*/
void CL_Reconnect_f( void ) {
	if ( !strlen( cls.servername ) || !strcmp( cls.servername, "localhost" ) ) {
		Com_Printf( "Can't reconnect to localhost.\n" );
		return;
	}
	Cvar_Set("ui_singlePlayerActive", "0");
	Cbuf_AddText( va("connect %s\n", cls.servername ) );
}

/*
================
CL_Connect_f

================
*/
void CL_Connect_f( void ) {
	char	*server;

	if ( !Cvar_VariableValue("fs_restrict") && !Sys_CheckCD() )
	{
		Com_Error( ERR_NEED_CD, SE_GetString("CON_TEXT_NEED_CD") ); //"Game CD not in drive" );		
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: connect [server]\n");
		return;	
	}

	Cvar_Set("ui_singlePlayerActive", "0");

	// clear any previous "server full" type messages
	clc->serverMessage[0] = 0;

	server = Cmd_Argv (1);

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		// if running a local server, kill it
		SV_Shutdown( "Server quit\n" );
	}

	// make sure a local server is killed
	Cvar_Set( "sv_killserver", "1" );
	preserveTextures = true; //Don't allow textures to get trashed by CL_Disconnect
	SV_Frame( 0 );
	preserveTextures = false;

	// Hmm. Don't delete textures here either. They should get thrown out when
	// we finally connect in CL_DownloadsComplete? Hope so.
	CL_Disconnect( qtrue, qfalse );
	Con_Close();

	/* MrE: 2000-09-13: now called in CL_DownloadsComplete
	CL_FlushMemory( );
	*/

	Q_strncpyz( cls.servername, server, sizeof(cls.servername) );

	if (!NET_StringToAdr( cls.servername, &clc->serverAddress) ) {
		Com_Printf ("Bad server address\n");
		cls.state = CA_DISCONNECTED;
		return;
	}
	if (clc->serverAddress.port == 0) {
		clc->serverAddress.port = BigShort( PORT_SERVER );
	}
#ifndef FINAL_BUILD
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", cls.servername,
		clc->serverAddress.ip[0], clc->serverAddress.ip[1],
		clc->serverAddress.ip[2], clc->serverAddress.ip[3],
		BigShort( clc->serverAddress.port ) );
#endif

	// if we aren't playing on a lan, we need to authenticate
	// with the cd key
	if ( NET_IsLocalAddress( clc->serverAddress ) ) {
		cls.state = CA_CHALLENGING;
	} else {
		cls.state = CA_CONNECTING;
	}

	cls.keyCatchers = 0;
	clc->connectTime = -99999;	// CL_CheckForResend() will fire immediately
	clc->connectPacketCount = 0;

	// server connection string
	Cvar_Set( "cl_currentServerAddress", server );
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f( void ) {
	char	message[1024];
	int		i;
	netadr_t	to;

	if ( !rcon_client_password->string ) {
		Com_Printf ("You must set 'rcon_password' before\n"
					"issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	strcat (message, "rcon ");

	strcat (message, rcon_client_password->string);
	strcat (message, " ");

	for (i=1 ; i<Cmd_Argc() ; i++) {
		strcat (message, Cmd_Argv(i));
		strcat (message, " ");
	}

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif

	if ( cls.state >= CA_CONNECTED ) {
		to = clc->netchan.remoteAddress;
	} else {
		if (!strlen(rconAddress->string)) {
			Com_Printf ("You must either be connected,\n"
						"or set the 'rconAddress' cvar\n"
						"to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rconAddress->string, &to);
		if (to.port == 0) {
			to.port = BigShort (PORT_SERVER);
		}
	}
	
	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
}

/*
=================
CL_SendPureChecksums
=================
*/
void CL_SendPureChecksums( void ) {
#ifndef _XBOX
	const char *pChecksums;
	char cMsg[MAX_INFO_VALUE];
	int i;

	// if we are pure we need to send back a command with our referenced pk3 checksums
	pChecksums = FS_ReferencedPakPureChecksums();

	// "cp"
	// "Yf"
	Com_sprintf(cMsg, sizeof(cMsg), "Yf ");
	Q_strcat(cMsg, sizeof(cMsg), pChecksums);
	for (i = 0; i < 2; i++) {
		cMsg[i] += 10;
	}
	CL_AddReliableCommand( cMsg );
#endif
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer( void ) {
	CL_AddReliableCommand( va("vdr") );
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
extern bool g_nOverrideChecked;
void CL_Vid_Restart_f( void ) {
	//rww - sort of nasty, but when a user selects a mod
	//from the menu all it does is a vid_restart, so we
	//have to check for new net overrides for the mod then.
#ifndef _XBOX	// No mods on Xbox
	g_nOverrideChecked = false;
#endif

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CL_ShutdownUI();
	// shutdown the CGame
	CL_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences( FS_UI_REF | FS_CGAME_REF );
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart( clc->checksumFeed );

	cls.rendererStarted = qfalse;
	cls.uiStarted = qfalse;
	cls.cgameStarted = qfalse;
	cls.soundRegistered = qfalse;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );

	// if not running a server clear the whole hunk
	if ( !com_sv_running->integer ) {
		CM_ClearMap();
		// clear the whole hunk
		Hunk_Clear();
	}
	else {
		// clear all the client data on the hunk
		Hunk_ClearToMark();
	}

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CL_StartHunkUsers();

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif

	// start the cgame if connected
	if ( cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC ) {
		cls.cgameStarted = qtrue;
		CL_InitCGame();
		// send pure checksums
		CL_SendPureChecksums();
	}
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
// extern void S_UnCacheDynamicMusic( void );
void CL_Snd_Restart_f( void ) {
	S_Shutdown();
	S_Init();

//	S_FreeAllSFXMem();			// These two removed by BTO (VV)
//	S_UnCacheDynamicMusic();	// S_Shutdown() already does this!

//	CL_Vid_Restart_f();

	extern qboolean	s_soundMuted;
	s_soundMuted = qfalse;		// we can play again

	extern void S_RestartMusic( void );
	S_RestartMusic();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f( void ) {
	Com_Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f( void ) {
	Com_Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f( void ) {
	int		i;
	int		ofs;

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//        if ( ClientManager::ActiveClient().state != CA_ACTIVE ) {
//			Com_Printf( "Not connected to a server.\n");
//			return;
//		}
//	}
//	else
#endif
	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n");
		return;
	}

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		ofs = cl->gameState.stringOffsets[ i ];
		if ( !ofs ) {
			continue;
		}
		Com_Printf( "%4i: %s\n", i, cl->gameState.stringData + ofs );
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f( void ) {
	Com_Printf( "--------- Client Information ---------\n" );
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		Com_Printf( "state: %i\n", ClientManager::ActiveClient().state );
//	else
#endif
	Com_Printf( "state: %i\n", cls.state );
	Com_Printf( "Server: %s\n", cls.servername );
	Com_Printf ("User info settings:\n");
	Info_Print( Cvar_InfoString( CVAR_USERINFO ) );
	Com_Printf( "--------------------------------------\n" );
}


//====================================================================

/*
=================
CL_DownloadsComplete

Called when all downloading has been completed
=================
*/
void CL_DownloadsComplete( void ) {

	// if we downloaded files we need to restart the file system
#ifndef _XBOX	// No downloads on Xbox
	if (clc->downloadRestart) {
		clc->downloadRestart = qfalse;

		FS_Restart(clc->checksumFeed); // We possibly downloaded a pak, restart the file system to load it

		// inform the server so we get new gamestate info
		CL_AddReliableCommand( "donedl" );

		// by sending the donenl command we request a new gamestate
		// so we don't want to load stuff yet
		return;
	}
#endif

	// let the client game init and load data
	cls.state = CA_LOADING;

#ifdef _XBOX
//		if(ClientManager::splitScreenMode == qtrue)
//			ClientManager::ActiveClient().state = cls.state;
#endif

	// Pump the loop, this may change gamestate!
	Com_EventLoop();

	// if the gamestate was changed by calling Com_EventLoop
	// then we loaded everything already and we don't want to do it again.
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if(ClientManager::ActiveClient().state != CA_LOADING) {
//			return;
//		}
//	}
//	else
#endif
	if ( cls.state != CA_LOADING ) {
		return;
	}

	// starting to load a map so we get out of full screen ui mode
	Cvar_Set("r_uiFullScreen", "0");

	// flush client memory and start loading stuff
	// this will also (re)load the UI
	// if this is a local client then only the client part of the hunk
	// will be cleared, note that this is done after the hunk mark has been set
#ifdef _XBOX
	if((ClientManager::ActiveClientNum() == 0) || (ClientManager::splitScreenMode == false))
#endif
	CL_FlushMemory();

	// initialize the CGame
	cls.cgameStarted = qtrue;
	CL_InitCGame();

	// set pure checksums
	CL_SendPureChecksums();

	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/*
=================
CL_BeginDownload

Requests a file to download from the server.  Stores it in the current
game directory.
=================
*/

#ifndef _XBOX	// No downloads on Xbox

void CL_BeginDownload( const char *localName, const char *remoteName ) {

	Com_DPrintf("***** CL_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

	Q_strncpyz ( clc->downloadName, localName, sizeof(clc->downloadName) );
	Com_sprintf( clc->downloadTempName, sizeof(clc->downloadTempName), "%s.tmp", localName );

	// Set so UI gets access to it
	Cvar_Set( "cl_downloadName", remoteName );
	Cvar_Set( "cl_downloadSize", "0" );
	Cvar_Set( "cl_downloadCount", "0" );
	Cvar_SetValue( "cl_downloadTime", (float) cls.realtime );

	clc->downloadBlock = 0; // Starting new file
	clc->downloadCount = 0;

	CL_AddReliableCommand( va("download %s", remoteName) );
}

/*
=================
CL_NextDownload

A download completed or failed
=================
*/
void CL_NextDownload(void) {
	char *s;
	char *remoteName, *localName;

	// We are looking to start a download here
	if (*clc->downloadList) {
		s = clc->downloadList;

		// format is:
		//  @remotename@localname@remotename@localname, etc.

		if (*s == '@')
			s++;
		remoteName = s;
		
		if ( (s = strchr(s, '@')) == NULL ) {
			CL_DownloadsComplete();
			return;
		}

		*s++ = 0;
		localName = s;
		if ( (s = strchr(s, '@')) != NULL )
			*s++ = 0;
		else
			s = localName + strlen(localName); // point at the nul byte

		CL_BeginDownload( localName, remoteName );

		clc->downloadRestart = qtrue;

		// move over the rest
		memmove( clc->downloadList, s, strlen(s) + 1);

		return;
	}

	CL_DownloadsComplete();
}

#endif	// XBOX		- No downloads on Xbox

/*
=================
CL_InitDownloads

After receiving a valid game state, we valid the cgame and local zip files here
and determine if we need to download them
=================
*/
void CL_InitDownloads(void) {
#ifndef _XBOX
  char missingfiles[1024];
	
	if ( !cl_allowDownload->integer )
	{
		// autodownload is disabled on the client
		// but it's possible that some referenced files on the server are missing
		if (FS_ComparePaks( missingfiles, sizeof( missingfiles ), qfalse ) )
		{      
			// NOTE TTimo I would rather have that printed as a modal message box
			//   but at this point while joining the game we don't know wether we will successfully join or not
			Com_Printf( "\nWARNING: You are missing some files referenced by the server:\n%s"
				"You might not be able to join the game\n"
				"Go to the setting menu to turn on autodownload, or get the file elsewhere\n\n", missingfiles );
		}
	}
	else if ( FS_ComparePaks( clc->downloadList, sizeof( clc->downloadList ) , qtrue ) ) {
		
		Com_Printf("Need paks: %s\n", clc->downloadList );
		
		if ( *clc->downloadList ) {
			// if autodownloading is not enabled on the server
			cls.state = CA_CONNECTED;
#ifdef _XBOX
//		if(ClientManager::splitScreenMode == qtrue)
//			ClientManager::ActiveClient().state = cls.state;
#endif
			CL_NextDownload();
			return;
		}
		
	}
#endif
	CL_DownloadsComplete();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
#define MAX_CONNECT_RETRIES		5

void CL_CheckForResend( void ) {
	int		port;
	char	info[MAX_INFO_STRING];
	char	data[MAX_INFO_STRING];

	CM_START_LOOP();

	// resend if we haven't gotten a reply yet
	if ( cls.state != CA_CONNECTING && cls.state != CA_CHALLENGING ) {
		return;
	}

	if ( cls.realtime - clc->connectTime < RETRANSMIT_TIMEOUT ) {
		return;
	}

	clc->connectTime = cls.realtime;	// for retransmit requests
	clc->connectPacketCount++;

	if( clc->connectPacketCount > MAX_CONNECT_RETRIES )
	{
		Com_Error( ERR_DROP, "@MENUS_LOST_CONNECTION" );
		return;
	}

	switch ( cls.state ) {
	case CA_CONNECTING:
		// requesting a challenge
		NET_OutOfBandPrint(NS_CLIENT, clc->serverAddress, "getchallenge");
		break;
		
	case CA_CHALLENGING:
		// sending back the challenge
		port = (int) Cvar_VariableValue ("net_qport");

		Q_strncpyz( info, Cvar_InfoString( CVAR_USERINFO ), sizeof( info ) );
		Info_SetValueForKey( info, "protocol", va("%i", PROTOCOL_VERSION ) );
		if(ClientManager::splitScreenMode == qtrue)
			Info_SetValueForKey( info, "qport", va("%i", ClientManager::ActivePort()) );
		else
			Info_SetValueForKey( info, "qport", va("%i", port ) );
		Info_SetValueForKey( info, "challenge", va("%i", clc->challenge ) );

		// Send Xbox stuff to host
		// This stuff needs to be parsed in SV_DirectConnect(). SOF2 sent a raw
		// XBPlayerInfo, which changed net traffic type. I'm just sending what I
		// need to, and doing text encode to avoid other changes.

		// Send our Xbox Address
		char sxnaddr[XNADDR_STRING_LEN];
		XnAddrToString(Net_GetXNADDR(), sxnaddr);
		Info_SetValueForKey(info, "xnaddr", sxnaddr);

		// Send our XUID if we're logged on and it's good
		if (logged_on)
		{
			XONLINE_USER *pUser = &XBLLoggedOnUsers[ IN_GetMainController() ];
			if (pUser && pUser->hr == S_OK)
			{
				char sxuid[XUID_STRING_LEN];
				XUIDToString(&pUser->xuid, sxuid);
				Info_SetValueForKey(info, "xuid", sxuid);
			}
		}

		// If we're allowed to take a private slot (ie, we joined a friend or got invited):
		if (XBL_MM_CanUsePrivateSlot())
			Info_SetValueForKey(info, "xbps", "1");

		sprintf(data, "connect \"%s\"", info );
		NET_OutOfBandData( NS_CLIENT, clc->serverAddress, (unsigned char *)data, strlen(data) );

		// the most current userinfo has been sent, so watch for any
		// newer changes to userinfo variables
		ClientManager::ActiveClient().cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;

	default:
		Com_Error( ERR_FATAL, "CL_CheckForResend: bad cls.state" );
	}

	CM_END_LOOP();
}


/*
===================
CL_DisconnectPacket

Sometimes the server can drop the client and the netchan based
disconnect can be lost.  If the client continues to send packets
to the server, the server will send out of band disconnect packets
to the client so it doesn't have to wait for the full timeout period.
===================
*/
void CL_DisconnectPacket( netadr_t from ) {
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue) {
//		if(ClientManager::ActiveClient().state < CA_AUTHORIZING ) {
//			return;
//		}
//	}
//	else
#endif
	if ( cls.state < CA_AUTHORIZING ) {
		return;
	}

	// if not from our server, ignore it
	if ( !NET_CompareAdr( from, clc->netchan.remoteAddress ) ) {
		return;
	}

	// if we have received packets within three seconds, ignore it
	// (it might be a malicious spoof)
	if ( cls.realtime - clc->lastPacketTime < 3000 ) {
		return;
	}

	// drop the connection (FIXME: connection dropped dialog)
	Com_Printf( "Server disconnected for unknown reason\n" );

#ifdef _XBOX
	Net_XboxDisconnect();
#endif

	CL_Disconnect( qtrue );
}


#ifndef MAX_STRINGED_SV_STRING
#define MAX_STRINGED_SV_STRING 1024
#endif
static void CL_CheckSVStringEdRef(char *buf, const char *str)
{ //I don't really like doing this. But it utilizes the system that was already in place.
	int i = 0;
	int b = 0;
	int strLen = 0;
	qboolean gotStrip = qfalse;

	if (!str || !str[0])
	{
		if (str)
		{
			strcpy(buf, str);
		}
		return;
	}

	strcpy(buf, str);

	strLen = strlen(str);

	if (strLen >= MAX_STRINGED_SV_STRING)
	{
		return;
	}

	while (i < strLen && str[i])
	{
		gotStrip = qfalse;

		if (str[i] == '@' && (i+1) < strLen)
		{
			if (str[i+1] == '@' && (i+2) < strLen)
			{
				if (str[i+2] == '@' && (i+3) < strLen)
				{ //@@@ should mean to insert a stringed reference here, so insert it into buf at the current place
					char stripRef[MAX_STRINGED_SV_STRING];
					int r = 0;

					while (i < strLen && str[i] == '@')
					{
						i++;
					}

					while (i < strLen && str[i] && str[i] != ' ' && str[i] != ':' && str[i] != '.' && str[i] != '\n')
					{
						stripRef[r] = str[i];
						r++;
						i++;
					}
					stripRef[r] = 0;

					buf[b] = 0;
					Q_strcat(buf, MAX_STRINGED_SV_STRING, SE_GetString(va("MP_SVGAME_%s", stripRef)));
					b = strlen(buf);
				}
			}
		}

		if (!gotStrip)
		{
			buf[b] = str[i];
			b++;
		}
		i++;
	}

	buf[b] = 0;
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char	*s;
	char	*c;

	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );	// skip the -1

	s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	c = Cmd_Argv(0);

	Com_DPrintf ("CL packet %s: %s\n", NET_AdrToString(from), c);

	// challenge from the server we are connecting to
	if ( !Q_stricmp(c, "challengeResponse") ) {
		if ( cls.state != CA_CONNECTING ) {
			Com_Printf( "Unwanted challenge response received.  Ignored.\n" );
		} else {
			// start sending challenge repsonse instead of challenge request packets
			clc->challenge = atoi(Cmd_Argv(1));
			cls.state = CA_CHALLENGING;
			clc->connectPacketCount = 0;
			clc->connectTime = -99999;

			// take this address as the new server address.  This allows
			// a server proxy to hand off connections to multiple servers
			clc->serverAddress = from;
			Com_DPrintf ("challengeResponse: %d\n", clc->challenge);
		}
		return;
	}

	// server connection
	if ( !Q_stricmp(c, "connectResponse") ) {
		if ( cls.state >= CA_CONNECTED ) {
			Com_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		if ( cls.state != CA_CHALLENGING ) {
			Com_Printf ("connectResponse packet while not connecting.  Ignored.\n");
			return;
		}
		if ( !NET_CompareBaseAdr( from, clc->serverAddress ) ) {
			Com_Printf( "connectResponse from a different address.  Ignored.\n" );
			Com_Printf( "%s should have been %s\n", NET_AdrToString( from ), 
				NET_AdrToString( clc->serverAddress ) );
			return;
		}

		if(ClientManager::splitScreenMode == qtrue)
			Netchan_Setup (NS_CLIENT, &clc->netchan, from, ClientManager::ActivePort() );
		else
			Netchan_Setup (NS_CLIENT, &clc->netchan, from, Cvar_VariableValue( "net_qport" ) );

		cls.state = CA_CONNECTED;
		clc->lastPacketSentTime = -9999;		// send first packet immediately
		return;
	}

	// server responding to a get playerlist
	if ( !Q_stricmp(c, "statusResponse") ) {
		assert( 0 );
//		CL_ServerStatusResponse( from, msg );
		return;
	}

	// a disconnect message from the server, which will happen if the server
	// dropped the connection but it is still getting packets from us
	if (!Q_stricmp(c, "disconnect")) {
		CL_DisconnectPacket( from );
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "echo") ) {
		NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv(1) );
		return;
	}

	// cd check
	if ( !Q_stricmp(c, "keyAuthorize") ) {
		// we don't use these now, so dump them on the floor
		return;
	}

	// echo request from server
	if ( !Q_stricmp(c, "print") ) 
	{
		char sTemp[MAX_STRINGED_SV_STRING];

		s = MSG_ReadString( msg );
		CL_CheckSVStringEdRef(sTemp, s);
		Q_strncpyz( clc->serverMessage, sTemp, sizeof( clc->serverMessage ) );
		Com_Printf( "%s", sTemp );
		return;
	}

	// New for Xbox - server responds with various xCommands to tell us to shut up and go away:
	if ( !Q_stricmp(c, "xFull") )
	{
		Com_Error( ERR_DROP, "@MENUS_SERVER_FULL" );
		return;
	}

	if ( !Q_stricmp(c, "xProt") || !Q_stricmp(c, "xToosoon") )
	{
		Com_Error( ERR_DROP, "@MENUS_LOST_CONNECTION" );
		return;
	}

	Com_DPrintf ("Unknown connectionless packet command.\n");
}


/*
=================
CL_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int		headerBytes;

	clc->lastPacketTime = cls.realtime;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif

	if ( cls.state < CA_CONNECTED ) {
		return;		// can't be a valid sequenced packet
	}

	if ( msg->cursize < 4 ) {
		Com_Printf ("%s: Runt packet\n",NET_AdrToString( from ));
		return;
	}

	//
	// packet from server
	//
	if ( !NET_CompareAdr( from, clc->netchan.remoteAddress ) ) {
		Com_DPrintf ("%s:sequenced packet without connection\n"
			,NET_AdrToString( from ) );
		// FIXME: send a client disconnect?
		return;
	}

	if (!CL_Netchan_Process( &clc->netchan, msg) ) {
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in 
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc->serverMessageSequence = LittleLong( *(int *)msg->data );

	clc->lastPacketTime = cls.realtime;
	CL_ParseServerMessage( msg );

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
#ifndef _XBOX	// No demos on Xbox
	if ( clc->demorecording && !clc->demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
#endif
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout( void ) {
	//
	// check timeout
	//
#ifdef _XBOX
/*
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( ( !cl_paused->integer || !sv_paused->integer ) 
			&& ClientManager::ActiveClient().state >= CA_CONNECTED && ClientManager::ActiveClient().state != CA_CINEMATIC
			&& cls.realtime - clc->lastPacketTime > cl_timeout->value*1000) {
			if (++cl->timeoutcount > 5) {	// timeoutcount saves debugger
				const char *psTimedOut = SE_GetString("MP_SVGAME_SERVER_CONNECTION_TIMED_OUT");
				Com_Printf ("\n%s\n",psTimedOut);
				Com_Error(ERR_DROP, psTimedOut);
				//CL_Disconnect( qtrue );
				return;
			}
		} else {
			cl->timeoutcount = 0;
		}
	}
	else
	{
*/
#endif
	if ( ( !cl_paused->integer || !sv_paused->integer ) 
		&& cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC
	    && cls.realtime - clc->lastPacketTime > cl_timeout->value*1000) {
		if (++cl->timeoutcount > 5) {	// timeoutcount saves debugger
//			const char *psTimedOut = SE_GetString("MP_SVGAME_SERVER_CONNECTION_TIMED_OUT");
//			Com_Printf ("\n%s\n",psTimedOut);
			Com_Error(ERR_DROP, "@MENUS_LOST_CONNECTION");
			//CL_Disconnect( qtrue );
			return;
		}
	} else {
		cl->timeoutcount = 0;
	}
#ifdef _XBOX
//	}
#endif
}


//============================================================================

void CL_PrepareUserInfoClientData()
{
	int clientnum = ClientManager::ActiveClientNum();
	Cvar_Set ( "model", ClientManager::ActiveClient().model );
	Cvar_Set ( "char_color_red", ClientManager::ActiveClient().char_color_red );
	Cvar_Set ( "char_color_green", ClientManager::ActiveClient().char_color_green );
	Cvar_Set ( "char_color_blue", ClientManager::ActiveClient().char_color_blue );

	Cvar_Set ( "saber1", ClientManager::ActiveClient().saber1 );
 	Cvar_Set ( "saber2", ClientManager::ActiveClient().saber2 );

	Cvar_Set ( "color1", ClientManager::ActiveClient().color1 );
	Cvar_Set ( "color2", ClientManager::ActiveClient().color2 );

	Cvar_Set ( "g_saber_color", ClientManager::ActiveClient().saber_color1 );
	Cvar_Set ( "g_saber2_color", ClientManager::ActiveClient().saber_color2 );
	Cvar_Set ( "forcePowers", ClientManager::ActiveClient().forcePowers);

	Cvar_Set ( "name", ClientManager::ActiveClient().autoName);
}

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo( void ) {
	// don't add reliable commands when not yet connected

	if ( cls.state < CA_CHALLENGING ) {
		return;
	}
	// don't overflow the reliable command buffer when paused
	if ( cl_paused->integer ) {
		return;
	}

	CM_START_LOOP();

	// send a reliable userinfo update if needed
	if ( ClientManager::ActiveClient().cvar_modifiedFlags & CVAR_USERINFO ) {
		CL_PrepareUserInfoClientData();
		ClientManager::ActiveClient().cvar_modifiedFlags &= ~CVAR_USERINFO;
		
		CL_AddReliableCommand( va("userinfo \"%s\"", Cvar_InfoString( CVAR_USERINFO ) ) );
	}

	CM_END_LOOP();
}


void CL_CheckDeferedCmds()
{
	// don't add reliable commands when not yet connected

	if ( cls.state < CA_CHALLENGING ) {
		return;
	}
	// don't overflow the reliable command buffer when paused
	if ( cl_paused->integer ) {
		return;
	}

	CM_START_LOOP();
	ClientManager::ClientFeedDeferedScript();
	CM_END_LOOP();
}


static void CL_UpdateTeamCount( void )
{
	int red		= 0;
	int blue	= 0;
	int i		= 0;

	for( ; i < cgs.maxclients; i++)
	{
		if(cgs.clientinfo[i].infoValid)
		{
			if(cgs.clientinfo[i].team == 3 )	// spectator
			{
				continue;
			}
			if(cgs.clientinfo[i].team == 1 || cgs.clientinfo[i].duelTeam == 2)
			{
				red++;
			}
			else if(cgs.clientinfo[i].team == 2 || cgs.clientinfo[i].duelTeam == 1)
			{
				blue++;
			}
		}
	}

	Cvar_Set("blueTeamCount", va("(%d)",blue));
	Cvar_Set("redTeamCount", va("(%d)",red));
}

extern CMiniHeap *G2VertSpaceServer;

/*
==================
CL_Frame

==================
*/
static unsigned int frameCount;
static float avgFrametime=0.0;
extern void SE_CheckForLanguageUpdates(void);
void CL_Frame ( int msec ) {

	if ( !com_cl_running->integer ) {
		// If a client isn't running, then we're running a dedicated
		// server - we still need the UI.
		S_Update();

		SCR_UpdateScreen();
		return;
	}

	SE_CheckForLanguageUpdates();	// will take zero time to execute unless language changes, then will reload strings.
									//	of course this still doesn't work for menus...

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif

	if ( cls.state == CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_UI )
		&& !com_sv_running->integer ) {
		// if disconnected, bring up the menu
		S_StopAllSounds();
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
		S_StartBackgroundTrack("music/mp/MP_action4.mp3","",0);
	}

	// if recording an avi, lock to a fixed fps
#ifndef _XBOX
	if ( cl_avidemo->integer && msec) {
		// save the current screen
		if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
			if (cl_avidemo->integer > 0) {
				Cbuf_ExecuteText( EXEC_NOW, "screenshot silent\n" );
			} else {
				Cbuf_ExecuteText( EXEC_NOW, "screenshot_tga silent\n" );
			}
		}
		// fixed time for next frame'
		msec = (1000 / abs(cl_avidemo->integer)) * com_timescale->value;
		if (msec == 0) {
			msec = 1;
		}
	}
#endif

	CL_MakeMonkeyDoLaundry();

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	// Always calculate framerate, bias the LOD if low
	avgFrametime+=msec;
	float framerate = 1000.0f*(1.0/(avgFrametime/32.0f));
	static int lodFrameCount = 0;
	int bias = Cvar_VariableIntegerValue("r_lodbias");
	if(!(frameCount&0x1f))
	{
        if(cl_framerate->integer)
		{
			char mess[256];
			sprintf(mess,"Frame rate=%f LOD=%d\n\n",framerate,bias);
			Com_PrintfAlways(mess);
		}
		avgFrametime=0.0f;

		if(ClientManager::splitScreenMode == qtrue)
		{
			// If splitscreen mode drops below 20FPS, pull down the LOD bias
			// below 15FPS, drop it again
			if(framerate < 20.0f && bias == 0)
			{
				bias++;
				Cvar_SetValue("r_lodbias", bias);
				lodFrameCount = -1;
			}
			if(framerate < 15.0f)
			{
				bias++;
				if(bias > 2)
					bias = 2;
				Cvar_SetValue("r_lodbias", bias);
				lodFrameCount = -1;
			}
		}
		else
		{
			// If non-splitscreen drops below 30FPS, pull down the LOD bias
			if(framerate < 30.0f && bias == 0)
			{
				bias++;
				Cvar_SetValue("r_lodbias", bias);
				lodFrameCount = -1;
			}
			if(framerate < 20.0f)
			{
				bias++;
				if(bias > 2)
					bias = 2;
				Cvar_SetValue("r_lodbias", bias);
				lodFrameCount = -1;
			}
		}

		lodFrameCount++;
		if(lodFrameCount==5 && bias > 0)
		{
			bias--;
			Cvar_SetValue("r_lodBias", bias);
			lodFrameCount = 0;
		}
	}
	frameCount++;

	cls.realtime += cls.frametime;

#ifdef _DONETPROFILE_
	if(cls.state==CA_ACTIVE)
	{
		ClReadProf().IncTime(cls.frametime);
	}
#endif

	if ( cl_timegraph->integer ) {
		SCR_DebugGraph ( cls.realFrametime * 0.25, 0 );
	}

#ifdef _XBOX
	//Check on the hot swappable button states.
	CL_UpdateHotSwap();
#endif

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	//JLF
	CL_CheckDeferedCmds();
	
	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CL_SendCmd();

	// resend a connection request if necessary
	CL_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update audio
	S_Update();

	// advance local effects for next frame
	SCR_RunCinematic();

//	Con_RunConsole();

	// reset the heap for Ghoul2 vert transform space gameside
	if (G2VertSpaceServer)
	{
		G2VertSpaceServer->ResetHeap();
	}

	CL_UpdateTeamCount();

	cls.framecount++;
}


//============================================================================

/*
================
CL_RefPrintf

DLL glue
================
*/
#define	MAXPRINTMSG	4096
void QDECL CL_RefPrintf( int print_level, const char *fmt, ...) {
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	if ( print_level == PRINT_ALL ) {
		Com_Printf ("%s", msg);
	} else if ( print_level == PRINT_WARNING ) {
		Com_Printf (S_COLOR_YELLOW "%s", msg);		// yellow
	} else if ( print_level == PRINT_DEVELOPER ) {
		Com_DPrintf (S_COLOR_RED "%s", msg);		// red
	}
}



/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef( void ) {
	if ( !re.Shutdown ) {
		return;
	}
	re.Shutdown( qtrue );
	Com_Memset( &re, 0, sizeof( re ) );
}

/*
============
CL_InitRenderer
============
*/
void CL_InitRenderer( void ) {
	// this sets up the renderer and calls R_Init
	re.BeginRegistration( &cls.glconfig );

	// load character sets
//	cls.charSetShader = re.RegisterShaderNoMip("gfx/2d/charsgrid_med");

	cls.whiteShader = re.RegisterShader( "white" );
//	cls.consoleShader = re.RegisterShader( "console" );
	g_console_field_width = cls.glconfig.vidWidth / SMALLCHAR_WIDTH - 2;
	kg.g_consoleField.widthInChars = g_console_field_width;
}

/*
============================
CL_StartHunkUsers

After the server has cleared the hunk, these will need to be restarted
This is the only place that any of these functions are called from
============================
*/
void CL_StartHunkUsers( void ) {
	if (!com_cl_running) {
		return;
	}

	if ( !com_cl_running->integer ) {
		return;
	}

	if ( !cls.rendererStarted ) {
		cls.rendererStarted = qtrue;
		CL_InitRenderer();
	}

	if ( !cls.soundStarted ) {
		cls.soundStarted = qtrue;
		S_Init();
	}

	if ( !cls.soundRegistered ) {
		cls.soundRegistered = qtrue;
#ifdef _XBOX
		S_BeginRegistration(ClientManager::NumClients());
#else
		S_BeginRegistration();
#endif
	}

	if ( !cls.uiStarted ) {
		cls.uiStarted = qtrue;
		CL_InitUI();
	}
}

/*
============
CL_InitRef
============
*/
void CL_InitRef( void ) {
	refexport_t	*ret;

	ret = GetRefAPI( REF_API_VERSION );

#if defined __USEA3D && defined __A3D_GEOM
	hA3Dg_ExportRenderGeom (ret);
#endif

//	Com_Printf( "-------------------------------\n");

	if ( !ret ) {
		Com_Error (ERR_FATAL, "Couldn't initialize refresh" );
	}

	re = *ret;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set( "cl_paused", "0" );
}


//===========================================================================================

#define MODEL_CHANGE_DELAY 5000
int gCLModelDelay = 0;

void CL_SetModel_f( void ) {
	char	*arg;
	char	name[256];

	arg = Cmd_Argv( 1 );
	if (arg[0])
	{
		/*
		//If you wanted to be foolproof you would put this on the server I guess. But that
		//tends to put things out of sync regarding cvar status. And I sort of doubt someone
		//is going to write a client and figure out the protocol so that they can annoy people
		//by changing models real fast.
		int curTime = Com_Milliseconds();
		if (gCLModelDelay > curTime)
		{
			Com_Printf("You can only change your model every %i seconds.\n", (MODEL_CHANGE_DELAY/1000));
			return;
		}
		
		gCLModelDelay = curTime + MODEL_CHANGE_DELAY;
		*/
		//rwwFIXMEFIXME: This is currently broken and doesn't seem to work for connecting clients
		Cvar_Set( "model", arg );
	}
	else
	{
		Cvar_VariableStringBuffer( "model", name, sizeof(name) );
		Com_Printf("model is set to %s\n", name);
	}
}

void CL_SetForcePowers_f( void ) {
	return;
}

#define G2_VERT_SPACE_CLIENT_SIZE 256

/*
====================
CL_Init
====================
*/
void CL_Init( void ) {
//	Com_Printf( "----- Client Initialization -----\n" );

	Con_Init ();	

#ifdef _XBOX
	CM_START_LOOP();

//	if (!ClientManager::ActiveClient().m_cl) ClientManager::ActiveClient().m_cl = new clientActive_t;
	
	CL_ClearState ();

//	if(ClientManager::splitScreenMode == qtrue)
//		ClientManager::ActiveClient().state = CA_DISCONNECTED;
//	else
        cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;
	CM_END_LOOP();
#else
	CL_ClearState ();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;
#endif
	CL_InitInput ();

	//
	// register our variables
	//
	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );
	cl_motd = Cvar_Get ("cl_motd", "1", 0);

//	cl_timeout = Cvar_Get ("cl_timeout", "200", 0);
	cl_timeout = Cvar_Get ("cl_timeout", "20", 0);

	cl_timeNudge = Cvar_Get ("cl_timeNudge", "0", CVAR_TEMP );
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
	cl_showSend = Cvar_Get ("cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get ("cl_showTimeDelta", "0", CVAR_TEMP );
	cl_freezeDemo = Cvar_Get ("cl_freezeDemo", "0", CVAR_TEMP );
	rcon_client_password = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	cl_activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );

	cl_timedemo = Cvar_Get ("timedemo", "0", 0);
	cl_avidemo = Cvar_Get ("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get ("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get ("rconAddress", "", 0);

	cl_yawspeed = Cvar_Get ("cl_yawspeed", "140", CVAR_ARCHIVE);
	cl_pitchspeed = Cvar_Get ("cl_pitchspeed", "140", CVAR_ARCHIVE);
	cl_anglespeedkey = Cvar_Get ("cl_anglespeedkey", "1.5", CVAR_ARCHIVE);

	cl_maxpackets = Cvar_Get ("cl_maxpackets", "30", CVAR_ARCHIVE );
	cl_packetdup = Cvar_Get ("cl_packetdup", "1", CVAR_ARCHIVE );

	cl_run = Cvar_Get ("cl_run", "1", CVAR_ARCHIVE);
	cl_sensitivity = Cvar_Get ("sensitivity", "5", CVAR_ARCHIVE);
#ifdef _XBOX
	cl_sensitivity = Cvar_Get ("sensitivity", "2", CVAR_ARCHIVE);

	Cvar_Get ("sensitivityY", "2", CVAR_ARCHIVE);

	CM_START_LOOP();

	ClientManager::ActiveClient().cg_sensitivity = cl_sensitivity->value;
	ClientManager::ActiveClient().cg_sensitivityY = Cvar_VariableValue ("sensitivityY");

	CM_END_LOOP();

#endif//_XBOX
	cl_mouseAccel = Cvar_Get ("cl_mouseAccel", "0", CVAR_ARCHIVE);
	cl_freelook = Cvar_Get( "cl_freelook", "1", CVAR_ARCHIVE );

	cl_showMouseRate = Cvar_Get ("cl_showmouserate", "0", 0);
	cl_framerate	= Cvar_Get ("cl_framerate", "0", CVAR_TEMP);
	cl_allowDownload = Cvar_Get ("cl_allowDownload", "0", CVAR_ARCHIVE);
	cl_allowAltEnter = Cvar_Get ("cl_allowAltEnter", "0", CVAR_ARCHIVE);

	cl_autolodscale = Cvar_Get( "cl_autolodscale", "1", CVAR_ARCHIVE );

	cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
#ifdef MACOS_X
        // In game video is REALLY slow in Mac OS X right now due to driver slowness
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "0", CVAR_ARCHIVE);
#else
	cl_inGameVideo = Cvar_Get ("r_inGameVideo", "1", CVAR_ARCHIVE);
#endif

	cl_serverStatusResendTime = Cvar_Get ("cl_serverStatusResendTime", "750", 0);

	m_pitchVeh = Cvar_Get ("m_pitchVeh", "-0.022", CVAR_ARCHIVE);

	m_yaw = Cvar_Get ("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get ("m_forward", "0.25", CVAR_ARCHIVE);
	m_side = Cvar_Get ("m_side", "0.25", CVAR_ARCHIVE);
#ifdef MACOS_X
        // Input is jittery on OS X w/o this
	m_filter = Cvar_Get ("m_filter", "1", CVAR_ARCHIVE);
#else
	m_filter = Cvar_Get ("m_filter", "0", CVAR_ARCHIVE);
#endif

	cl_motdString = Cvar_Get( "cl_motdString", "", CVAR_ROM );

	Cvar_Get( "cl_maxPing", "800", CVAR_ARCHIVE );


	// userinfo
	Cvar_Get ("name", "Padawan", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );
	Cvar_Get ("rate", "4000", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("model", "kyle/default", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

#ifdef _XBOX
	// Temp CVar to store UI selected models
//	Cvar_Get ("UImodel", "kyle/default", CVAR_ARCHIVE | CVAR_PROFILE );
#endif

//	Cvar_Get ("model2", "kyle/default", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE );
	Cvar_Get ("forcepowers", "7-1-032330000000001333", CVAR_USERINFO | CVAR_ARCHIVE );


	
//JLF forcepowers
#ifdef _XBOX
//	Cvar_Get ("forcepowers2", "7-1-032330000000001333", CVAR_USERINFO2 | CVAR_ARCHIVE );
	Cvar_Get ("forcePowersProfile", "7-1-032330000000001333", CVAR_USERINFO | CVAR_ARCHIVE );
#endif
	Cvar_Get ("g_redTeam", "Empire", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get ("g_blueTeam", "Rebellion", CVAR_SERVERINFO | CVAR_ARCHIVE);

	Cvar_Get ("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("color2", "4", CVAR_USERINFO | CVAR_ARCHIVE );


	Cvar_Get ("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("teamtask", "0", CVAR_USERINFO );
	Cvar_Get ("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE );
	Cvar_Get ("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	Cvar_Get ("password", "", CVAR_USERINFO);
	Cvar_Get ("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE );

	//default sabers
	Cvar_Get ("saber1",  "single_1", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

	Cvar_Get ("saber2",  "none", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

//	Cvar_Get ("saber12",  "single_1", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE2 );
//	Cvar_Get ("saber22",  "none", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE2 );

	//skin color
	Cvar_Get ("char_color_red",  "255", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

	Cvar_Get ("char_color_green",  "255", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

	Cvar_Get ("char_color_blue",  "255", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_PROFILE );

	

	//skin color2
//	Cvar_Get ("char_color_red2",  "255", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE2 );
//	Cvar_Get ("char_color_green2",  "255", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE2 );
//	Cvar_Get ("char_color_blue2",  "255", CVAR_USERINFO2 | CVAR_ARCHIVE | CVAR_PROFILE2 );

	// cgame might not be initialized before menu is used
	Cvar_Get ("cg_viewsize", "100", CVAR_ARCHIVE );

	//
	// register our commands
	//
#ifndef _XBOX
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("globalservers", CL_GlobalServers_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("demo", CL_PlayDemo_f);
	Cmd_AddCommand ("stoprecord", CL_StopRecord_f);
#endif
	Cmd_AddCommand ("configstrings", CL_Configstrings_f);
	Cmd_AddCommand ("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand ("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("showip", CL_ShowIP_f );
	Cmd_AddCommand ("fs_openedList", CL_OpenedPK3List_f );
	Cmd_AddCommand ("fs_referencedList", CL_ReferencedPK3List_f );
	Cmd_AddCommand ("model", CL_SetModel_f );
	Cmd_AddCommand ("forcepowers", CL_SetForcePowers_f );

	CL_InitRef();

	SCR_Init ();

	Cbuf_Execute ();

	Cvar_Set( "cl_running", "1" );

	G2VertSpaceClient = new CMiniHeap(G2_VERT_SPACE_CLIENT_SIZE * 1024);

#ifdef _XBOX
	extern void CIN_Init(void);
	Com_Printf( "Initializing Cinematics...\n");
	CIN_Init();
#endif
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown( void ) {
	static qboolean recursive = qfalse;
	
	//Com_Printf( "----- CL_Shutdown -----\n" );

	if ( recursive ) {
		printf ("recursive CL_Shutdown shutdown\n");
		return;
	}
	recursive = qtrue;

	if (G2VertSpaceClient)
	{
		delete G2VertSpaceClient;
		G2VertSpaceClient = 0;
	}

	CL_Disconnect( qtrue );

	CL_ShutdownRef();	//must be before shutdown all so the images get dumped in RE_Shutdown

	// RJ: added the shutdown all to close down the cgame (to free up some memory, such as in the fx system)
	CL_ShutdownAll();

	S_Shutdown();
	//CL_ShutdownUI();

	Cmd_RemoveCommand ("cmd");
	Cmd_RemoveCommand ("configstrings");
	Cmd_RemoveCommand ("userinfo");
	Cmd_RemoveCommand ("snd_restart");
	Cmd_RemoveCommand ("vid_restart");
	Cmd_RemoveCommand ("disconnect");
	Cmd_RemoveCommand ("record");
	Cmd_RemoveCommand ("demo");
	Cmd_RemoveCommand ("cinematic");
	Cmd_RemoveCommand ("stoprecord");
	Cmd_RemoveCommand ("connect");
	Cmd_RemoveCommand ("localservers");
	Cmd_RemoveCommand ("globalservers");
	Cmd_RemoveCommand ("rcon");
	Cmd_RemoveCommand ("ping");
	Cmd_RemoveCommand ("serverstatus");
	Cmd_RemoveCommand ("showip");
	Cmd_RemoveCommand ("model");
	Cmd_RemoveCommand ("forcepowers");

	Cvar_Set( "cl_running", "0" );

	recursive = qfalse;

	Com_Memset( &cls, 0, sizeof( cls ) );

	//Com_Printf( "-----------------------\n" );

}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void) {
	Sys_ShowIP();
}
