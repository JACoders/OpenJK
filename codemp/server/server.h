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

#pragma once

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "game/g_public.h"
#include "game/bg_public.h"
#include "rd-common/tr_public.h"

//=============================================================================

#define	PERS_SCORE				0		// !!! MUST NOT CHANGE, SERVER AND
										// GAME BOTH REFERENCE !!!

#define	MAX_ENT_CLUSTERS	16

typedef struct svEntity_s {
	struct worldSector_s *worldSector;
	struct svEntity_s *nextEntityInWorldSector;

	entityState_t	baseline;		// for delta compression of initial sighting
	int			numClusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			lastCluster;		// if all the clusters don't fit in clusternums
	int			areanum, areanum2;
	int			snapshotCounter;	// used to prevent double adding from portal views
} svEntity_t;

typedef enum {
	SS_DEAD,			// no map loaded
	SS_LOADING,			// spawning level entities
	SS_GAME				// actively running
} serverState_t;

typedef struct server_s {
	serverState_t	state;
	qboolean		restarting;			// if true, send configstring changes during SS_LOADING
	int				serverId;			// changes each server start
	int				restartedServerId;	// serverId before a map_restart
	int				checksumFeed;		//
	int				snapshotCounter;	// incremented for each snapshot built
	int				timeResidual;		// <= 1000 / sv_frame->value
	int				nextFrameTime;		// when time > nextFrameTime, process world
	char			*configstrings[MAX_CONFIGSTRINGS];
	svEntity_t		svEntities[MAX_GENTITIES];

	char			*entityParsePoint;	// used during game VM init

	// the game virtual machine will update these on init and changes
	sharedEntity_t	*gentities;
	int				gentitySize;
	int				num_entities;		// current number, <= MAX_GENTITIES

	playerState_t	*gameClients;
	int				gameClientSize;		// will be > sizeof(playerState_t) due to game private data

	int				restartTime;
	int				time;

	//rwwRMG - added:
	int				mLocalSubBSPIndex;
	int				mLocalSubBSPModelOffset;
	char			*mLocalSubBSPEntityParsePoint;

	char			*mSharedMemory;

	time_t			realMapTimeStarted;	// time the current map was started
	qboolean		demosPruned; // whether or not existing demos were cleaned up already
} server_t;

typedef struct clientSnapshot_s {
	int				areabytes;
	byte			areabits[MAX_MAP_AREA_BYTES];		// portalarea visibility bits
	playerState_t	ps;
	playerState_t	vps; //vehicle I'm riding's playerstate (if applicable) -rww
#ifdef _ONEBIT_COMBO
	int				*pDeltaOneBit;
	int				*pDeltaOneBitVeh;
	int				*pDeltaNumBit;
	int				*pDeltaNumBitVeh;
#endif
	int				num_entities;
	int				first_entity;		// into the circular sv_packet_entities[]
										// the entities MUST be in increasing state number
										// order, otherwise the delta compression will fail
	int				messageSent;		// time the message was transmitted
	int				messageAcked;		// time the message was acked
	int				messageSize;		// used to rate drop packets
} clientSnapshot_t;

typedef enum {
	CS_FREE,		// can be reused for a new connection
	CS_ZOMBIE,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	CS_CONNECTED,	// has been assigned to a client_t, but no gamestate yet
	CS_PRIMED,		// gamestate has been sent, but client hasn't sent a usercmd
	CS_ACTIVE		// client is fully in game
} clientState_t;


// struct to hold demo data for a single demo
typedef struct {
	char		demoName[MAX_OSPATH];
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is sent
	int			minDeltaFrame;	// the first non-delta frame stored in the demo.  cannot delta against frames older than this
	fileHandle_t	demofile;
	qboolean	isBot;
	int			botReliableAcknowledge; // for bots, need to maintain a separate reliableAcknowledge to record server messages into the demo file
} demoInfo_t;


typedef struct client_s {
	clientState_t	state;
	char			userinfo[MAX_INFO_STRING];		// name, etc

	qboolean		sentGamedir; //see if he has been sent an svc_setgame

	char			reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];
	int				reliableSequence;		// last added reliable message, not necesarily sent or acknowledged yet
	int				reliableAcknowledge;	// last acknowledged reliable message
	int				reliableSent;			// last sent reliable message, not necesarily acknowledged yet
	int				messageAcknowledge;

	int				gamestateMessageNum;	// netchan->outgoingSequence of gamestate
	int				challenge;

	usercmd_t		lastUsercmd;
	int				lastMessageNum;		// for delta compression
	int				lastClientCommand;	// reliable client message sequence
	char			lastClientCommandString[MAX_STRING_CHARS];
	sharedEntity_t	*gentity;			// SV_GentityNum(clientnum)
	char			name[MAX_NAME_LENGTH];			// extracted from userinfo, high bits masked

	// downloading
	char			downloadName[MAX_QPATH]; // if not empty string, we are downloading
	fileHandle_t	download;			// file being downloaded
 	int				downloadSize;		// total bytes (can't use EOF because of paks)
 	int				downloadCount;		// bytes sent
	int				downloadClientBlock;	// last block we sent to the client, awaiting ack
	int				downloadCurrentBlock;	// current block number
	int				downloadXmitBlock;	// last block we xmited
	unsigned char	*downloadBlocks[MAX_DOWNLOAD_WINDOW];	// the buffers for the download blocks
	int				downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qboolean		downloadEOF;		// We have sent the EOF block
	int				downloadSendTime;	// time we last got an ack from the client

	int				deltaMessage;		// frame last client usercmd message
	int				lastReliableTime;	// svs.time when reliable command was last received
	int				lastPacketTime;		// svs.time when packet was last received
	int				lastConnectTime;	// svs.time when connection started
	int				nextSnapshotTime;	// send another snapshot when svs.time >= nextSnapshotTime
	qboolean		rateDelayed;		// true if nextSnapshotTime was set based on rate instead of snapshotMsec
	int				timeoutCount;		// must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t	frames[PACKET_BACKUP];	// updates can be delta'd from here
	int				ping;
	int				rate;				// bytes / second
	int				snapshotMsec;		// requests a snapshot every snapshotMsec unless rate choked
	int				wishSnaps;			// requested snapshot/sec rate
	int				pureAuthentic;
	qboolean		gotCP; // TTimo - additional flag to distinguish between a bad pure checksum, and no cp command at all
	netchan_t		netchan;

	int				lastUserInfoChange; //if > svs.time && count > x, deny change -rww
	int				lastUserInfoCount; //allow a certain number of changes within a certain time period -rww

	int				oldServerTime;
	qboolean		csUpdated[MAX_CONFIGSTRINGS];

	demoInfo_t		demo;
} client_t;

//=============================================================================


// this structure will be cleared only when the game dll changes
typedef struct serverStatic_s {
	qboolean	initialized;				// sv_init has completed

	int			time;						// will be strictly increasing across level changes
	time_t		startTime;					// time since epoch the executable was started

	int			snapFlagServerBit;			// ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t	*clients;					// [sv_maxclients->integer];
	int			numSnapshotEntities;		// sv_maxclients->integer*PACKET_BACKUP*MAX_SNAPSHOT_ENTITIES
	int			nextSnapshotEntities;		// next snapshotEntities to use
	entityState_t	*snapshotEntities;		// [numSnapshotEntities]
	int			nextHeartbeatTime;
	netadr_t	redirectAddress;			// for rcon return messages

	netadr_t	authorizeAddress;			// for rcon return messages

	qboolean	gameStarted;				// gvm is loaded
} serverStatic_t;

#define SERVER_MAXBANS	1024
// Structure for managing bans
typedef struct serverBan_s {
	netadr_t ip;
	// For a CIDR-Notation type suffix
	int subnet;

	qboolean isexception;
} serverBan_t;

//=============================================================================

extern	serverStatic_t	svs;				// persistant server info across maps
extern	server_t		sv;					// cleared each map

//FIXME: dedi server probably can't have this..
extern	refexport_t		*re;					// interface to refresh .dll

extern	cvar_t	*sv_snapsMin;
extern	cvar_t	*sv_snapsMax;
extern	cvar_t	*sv_snapsPolicy;
extern	cvar_t	*sv_fps;
extern	cvar_t	*sv_timeout;
extern	cvar_t	*sv_zombietime;
extern	cvar_t	*sv_rconPassword;
extern	cvar_t	*sv_privatePassword;
extern	cvar_t	*sv_allowDownload;
extern	cvar_t	*sv_maxclients;
extern	cvar_t	*sv_privateClients;
extern	cvar_t	*sv_hostname;
extern	cvar_t	*sv_master[MAX_MASTER_SERVERS];
extern	cvar_t	*sv_reconnectlimit;
extern	cvar_t	*sv_showghoultraces;
extern	cvar_t	*sv_showloss;
extern	cvar_t	*sv_padPackets;
extern	cvar_t	*sv_killserver;
extern	cvar_t	*sv_mapname;
extern	cvar_t	*sv_mapChecksum;
extern	cvar_t	*sv_serverid;
extern	cvar_t	*sv_ratePolicy;
extern	cvar_t	*sv_clientRate;
extern	cvar_t	*sv_minRate;
extern	cvar_t	*sv_maxRate;
extern	cvar_t	*sv_minPing;
extern	cvar_t	*sv_maxPing;
extern	cvar_t	*sv_gametype;
extern	cvar_t	*sv_pure;
extern	cvar_t	*sv_floodProtect;
extern	cvar_t	*sv_floodProtectSlow;
extern	cvar_t	*sv_lanForceRate;
extern	cvar_t	*sv_needpass;
extern	cvar_t	*sv_filterCommands;
extern	cvar_t	*sv_autoDemo;
extern	cvar_t	*sv_autoDemoBots;
extern	cvar_t	*sv_autoDemoMaxMaps;
extern	cvar_t	*sv_legacyFixes;
extern	cvar_t	*sv_banFile;
extern	cvar_t	*sv_maxOOBRate;
extern	cvar_t	*sv_maxOOBRateIP;
extern	cvar_t	*sv_autoWhitelist;

extern	serverBan_t serverBans[SERVER_MAXBANS];
extern	int serverBansCount;

//===========================================================

//
// sv_main.c
//
typedef struct leakyBucket_s leakyBucket_t;
struct leakyBucket_s {
	int					lastTime;
	unsigned short		burst;
};

extern leakyBucket_t outboundLeakyBucket;

qboolean SVC_RateLimit( leakyBucket_t *bucket, int burst, int period, int now );
qboolean SVC_RateLimitAddress( netadr_t from, int burst, int period, int now );
void SVC_LoadWhitelist( void );
void SVC_WhitelistAdr( netadr_t adr );
void SV_FinalMessage (char *message);
void QDECL SV_SendServerCommand( client_t *cl, const char *fmt, ...);


void SV_AddOperatorCommands (void);
void SV_RemoveOperatorCommands (void);


void SV_MasterHeartbeat (void);
void SV_MasterShutdown (void);




//
// sv_init.c
//
void SV_SetConfigstring( int index, const char *val );
void SV_GetConfigstring( int index, char *buffer, int bufferSize );
void SV_UpdateConfigstrings( client_t *client );

void SV_SetUserinfo( int index, const char *val );
void SV_GetUserinfo( int index, char *buffer, int bufferSize );

void SV_ChangeMaxClients( void );
void SV_SpawnServer( char *server, qboolean killBots, ForceReload_e eForceReload );



//
// sv_challenge.cpp
//
void SV_ChallengeInit();
void SV_ChallengeShutdown();
int SV_CreateChallenge(netadr_t from);
qboolean SV_VerifyChallenge(int receivedChallenge, netadr_t from);

//
// sv_client.c
//
void SV_GetChallenge( netadr_t from );

void SV_DirectConnect( netadr_t from );

void SV_SendClientMapChange( client_t *client );
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg );
void SV_UserinfoChanged( client_t *cl );

void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd );
void SV_DropClient( client_t *drop, const char *reason );

void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK );
void SV_ClientThink (client_t *cl, usercmd_t *cmd);

void SV_WriteDownloadToClient( client_t *cl , msg_t *msg );

//
// sv_ccmds.c
//
void SV_Heartbeat_f( void );
void SV_RecordDemo( client_t *cl, char *demoName );
void SV_StopRecordDemo( client_t *cl );
void SV_AutoRecordDemo( client_t *cl );
void SV_StopAutoRecordDemos();
void SV_BeginAutoRecordDemos();

//
// sv_snapshot.c
//
void SV_AddServerCommand( client_t *client, const char *cmd );
void SV_UpdateServerCommandsToClient( client_t *client, msg_t *msg );
void SV_WriteFrameToClient (client_t *client, msg_t *msg);
void SV_SendMessageToClient( msg_t *msg, client_t *client );
void SV_SendClientMessages( void );
void SV_SendClientSnapshot( client_t *client );

//
// sv_game.c
//
int	SV_NumForGentity( sharedEntity_t *ent );
sharedEntity_t *SV_GentityNum( int num );
playerState_t *SV_GameClientNum( int num );
svEntity_t	*SV_SvEntityForGentity( sharedEntity_t *gEnt );
sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt );
void		SV_InitGameProgs ( void );
void		SV_ShutdownGameProgs ( void );
qboolean	SV_inPVS (const vec3_t p1, const vec3_t p2);

//
// sv_bot.c
//
void		SV_BotFrame( int time );
int			SV_BotAllocateClient(void);
void		SV_BotFreeClient( int clientNum );

void		SV_BotInitCvars(void);
int			SV_BotGetSnapshotEntity( int client, int ent );
int			SV_BotGetConsoleMessage( int client, char *buf, int size );

void *Bot_GetMemoryGame(int size);
void Bot_FreeMemoryGame(void *ptr);

int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void BotImport_DebugPolygonDelete(int id);

//============================================================
//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity( sharedEntity_t *ent );
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity( sharedEntity_t *ent );
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


clipHandle_t SV_ClipHandleForEntity( const sharedEntity_t *ent );


void SV_SectorList_f( void );


int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.


int SV_PointContents( const vec3_t p, int passEntityNum );
// returns the CONTENTS_* value from the world and all entities at the given point.


void SV_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask, int capsule, int traceFlags, int useLod );
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum is explicitly excluded from clipping checks (normally ENTITYNUM_NONE)


void SV_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, int capsule );
// clip to a specific entity

//
// sv_net_chan.c
//
void SV_Netchan_Transmit( client_t *client, msg_t *msg);	//int length, const byte *data );
void SV_Netchan_TransmitNextFragment( netchan_t *chan );
qboolean SV_Netchan_Process( client_t *client, msg_t *msg );
