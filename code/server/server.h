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

// server.h

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../game/g_public.h"
#include "../game/bg_public.h"

#ifndef SERVER_H
#define SERVER_H


//=============================================================================

//#define	PERS_SCORE				0		// !!! MUST NOT CHANGE, SERVER AND
										// GAME BOTH REFERENCE !!!
//rww - this won't do.. I need to include bg_public.h in the exe elsewhere.
//I'm including it here instead so we can have our PERS_SCORE value. And have
//it be the proper enum value.

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

typedef struct {
	serverState_t	state;
	int				serverId;			// changes each server start
	int				snapshotCounter;	// incremented for each snapshot built
	int				time;				// all entities are correct for this time		// These 2 saved out
	int				timeResidual;		// <= 1000 / sv_frame->value					//   during savegame.
	float			timeResidualFraction;	// fraction of a msec accumulated
	int				nextFrameTime;		// when time > nextFrameTime, process world		// this doesn't get used anywhere! -Ste
	char			*configstrings[MAX_CONFIGSTRINGS];
	//
	// be careful, Jake's code uses the 'svEntities' field as a marker to memset-this-far-only inside SV_InitSV()!!!!!
	//
	char			*entityParsePoint;	// used during game VM init

	int				mLocalSubBSPIndex;
	int				mLocalSubBSPModelOffset;
	char			*mLocalSubBSPEntityParsePoint;

	svEntity_t		svEntities[MAX_GENTITIES];
} server_t;



typedef struct {
	int				areabytes;
	byte			areabits[MAX_MAP_AREA_BYTES];		// portalarea visibility bits
	playerState_t	ps;
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


typedef struct client_s {
	clientState_t	state;
	char			userinfo[MAX_INFO_STRING];		// name, etc

	char			*reliableCommands[MAX_RELIABLE_COMMANDS];
	int				reliableSequence;
	int				reliableAcknowledge;

	int				gamestateMessageNum;	// netchan->outgoingSequence of gamestate

	usercmd_t		lastUsercmd;
	int				lastMessageNum;		// for delta compression
	int				cmdNum;				// command number last executed
	int				lastClientCommand;	// reliable client message sequence
	gentity_t		*gentity;			// SV_GentityNum(clientnum)
	char			name[MAX_NAME_LENGTH];			// extracted from userinfo, high bits masked
	byte			*download;			// file being downloaded
	int				downloadsize;		// total bytes (can't use EOF because of paks)
	int				downloadcount;		// bytes sent
	int				deltaMessage;		// frame last client usercmd message
	int				lastPacketTime;		// sv.time when packet was last received
	int				lastConnectTime;	// sv.time when connection started
	qboolean		droppedCommands;	// true if enough pakets to pass the cl_packetdup were dropped
	int				timeoutCount;		// must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t	frames[PACKET_BACKUP];	// updates can be delta'd from here
	netchan_t		netchan;
} client_t;

//=============================================================================


typedef struct {
	netadr_t	adr;
	int			challenge;
	int			time;
} challenge_t;

// this structure will be cleared only when the game dll changes
typedef struct {
	qboolean	initialized;				// sv_init has completed
	client_t	*clients;					// [sv_maxclients->integer];
	int			numSnapshotEntities;		// sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
	int			nextSnapshotEntities;		// next snapshotEntities to use
	entityState_t	*snapshotEntities;		// [numSnapshotEntities]
} serverStatic_t;

//=============================================================================

extern	serverStatic_t	svs;				// persistant server info across maps
extern	server_t		sv;					// cleared each map

extern	game_export_t	*ge;

extern	cvar_t	*sv_fps;
extern	cvar_t	*sv_timeout;
extern	cvar_t	*sv_zombietime;
extern	cvar_t	*sv_reconnectlimit;
extern	cvar_t	*sv_showloss;
extern	cvar_t	*sv_killserver;
extern	cvar_t	*sv_mapname;
extern	cvar_t	*sv_spawntarget;
extern	cvar_t	*sv_mapChecksum;
extern	cvar_t	*sv_serverid;
extern  cvar_t	*sv_testsave;
extern  cvar_t	*sv_compress_saved_games;

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage (char *message);
void QDECL SV_SendServerCommand( client_t *cl, const char *fmt, ...);


void SV_AddOperatorCommands (void);
void SV_RemoveOperatorCommands (void);


//
// sv_init.c
//
void SV_SetConfigstring( int index, const char *val );
void SV_GetConfigstring( int index, char *buffer, int bufferSize );

void SV_SetUserinfo( int index, const char *val );
void SV_GetUserinfo( int index, char *buffer, int bufferSize );

void SV_SpawnServer( const char *server, ForceReload_e eForceReload, qboolean bAllowScreenDissolve );


//
// sv_client.c
//
void SV_DirectConnect( netadr_t from );

void SV_ExecuteClientMessage( client_t *cl, msg_t *msg );
void SV_UserinfoChanged( client_t *cl );

void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd, SavedGameJustLoaded_e eSavedGameJustLoaded );
void SV_DropClient( client_t *drop, const char *reason );

void SV_ExecuteClientCommand( client_t *cl, const char *s );
void SV_ClientThink (client_t *cl, usercmd_t *cmd);


//
// sv_snapshot.c
//
void SV_AddServerCommand( client_t *client, const char *cmd );
void SV_SendMessageToClient( msg_t *msg, client_t *client );
void SV_SendClientMessages( void );
void SV_SendClientSnapshot( client_t *client );



//
// sv_game.c
//
gentity_t	*SV_GentityNum( int num );
svEntity_t	*SV_SvEntityForGentity( gentity_t *gEnt );
gentity_t	*SV_GEntityForSvEntity( svEntity_t *svEnt );
void		SV_InitGameProgs (void);
void		SV_ShutdownGameProgs (qboolean shutdownCin);
qboolean	SV_inPVS (const vec3_t p1, const vec3_t p2);


//============================================================
//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity( gentity_t *ent );
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity( gentity_t *ent );
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid


clipHandle_t SV_ClipHandleForEntity( const gentity_t *ent );


void SV_SectorList_f( void );


int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, gentity_t **elist, int maxcount );
// fills in a table of entity pointers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.


int SV_PointContents( const vec3_t p, int passEntityNum );
// returns the CONTENTS_* value from the world and all entities at the given point.

/*
Ghoul2 Insert Start
*/
void SV_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
			  const int passEntityNum, const int contentmask, const EG2_Collision eG2TraceType = G2_NOCOLLIDE, const int useLod = 0);
/*
Ghoul2 Insert End
*/
// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum is explicitly excluded from clipping checks (normally ENTITYNUM_NONE)



///////////////////////////////////////////////
//
// sv_savegame.cpp
//
void SV_LoadGame_f(void);
void SV_LoadTransition_f(void);
void SV_SaveGame_f(void);
void SV_WipeGame_f(void);
qboolean SV_TryLoadTransition( const char *mapname );
qboolean SG_WriteSavegame(const char *psPathlessBaseName, qboolean qbAutosave);
qboolean SG_ReadSavegame(const char *psPathlessBaseName);
void SG_WipeSavegame(const char *psPathlessBaseName);
qboolean SG_Append(unsigned int chid, const void *data, int length);
int SG_Read			(unsigned int chid, void *pvAddress, int iLength, void **ppvAddressPtr = NULL);
int SG_ReadOptional	(unsigned int chid, void *pvAddress, int iLength, void **ppvAddressPtr = NULL);
void SG_Shutdown();
void SG_TestSave(void);
//
// note that this version number does not mean that a savegame with the same version can necessarily be loaded,
//	since anyone can change any loadsave-affecting structure somewhere in a header and change a chunk size. 
// What it's used for is for things like mission pack etc if we need to distinguish "street-copy" savegames from
//	any new enhanced ones that need to ask for new chunks during loading.
//
#define iSAVEGAME_VERSION 1
int SG_Version(void);	// call this to know what version number a successfully-opened savegame file was
//
extern SavedGameJustLoaded_e eSavedGameJustLoaded;
extern qboolean qbLoadTransition;
//
///////////////////////////////////////////////

#ifdef JK2_MODE
// glue
class cStrings
{
private:
	unsigned int	Flags;
	char			*Reference;
	
public:
					 cStrings(unsigned int initFlags = 0, char *initReference = NULL);
	virtual			~cStrings(void);

	virtual void	Clear(void);

	void			SetFlags(unsigned int newFlags);
	void			SetReference(char *newReference);

	unsigned int	GetFlags(void) { return Flags; }
	char			*GetReference(void) { return Reference; }

	virtual bool	UnderstandToken(int token, char *data );
	virtual bool	Load(char *&Data, int &Size );
};


class cStringsSingle : public cStrings
{
private:
	char			*Text;

	virtual void	Clear(void);
	void			SetText(const char *newText);

public:
					 cStringsSingle(unsigned int initFlags = 0, char *initReference = NULL);
	virtual			~cStringsSingle();

	char			*GetText(void) { return Text; }

	virtual bool	UnderstandToken(int token, char *data );
};
#endif

#endif	// #ifndef SERVER_H
