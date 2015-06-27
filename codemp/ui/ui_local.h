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
#include "rd-common/tr_types.h"
#include "ui_public.h"
#include "keycodes.h"
#include "game/bg_public.h"
#include "ui_shared.h"

// ui_cvar.c
#define XCVAR_PROTO
	#include "ui_xcvar.h"
#undef XCVAR_PROTO
void UI_RegisterCvars( void );
void UI_UpdateCvars( void );

#define MAX_FORCE_CONFIGS		128
#define MAX_SABER_HILTS			256 //64

//
// ui_main.c
//
qboolean UI_FeederSelection( float feederID, int index, itemDef_t *item );
void UI_Report( void );
void UI_Load( void );
void UI_LoadMenus( const char *menuFile, qboolean reset );
void UI_LoadArenas( void );
void UI_LoadForceConfig_List( void );

//
// ui_players.c
//

//FIXME ripped from cg_local.h
typedef struct lerpFrame_s {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct playerInfo_s {
	// model info
	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	lerpFrame_t		legs;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	lerpFrame_t		torso;

//	qhandle_t		headModel;
//	qhandle_t		headSkin;

	animation_t		animations[MAX_TOTALANIMATIONS];

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	vec3_t			flashDlightColor;
	int				muzzleFlashTime;

	// currently in use drawing parms
	vec3_t			viewAngles;
	vec3_t			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;

	int				pendingTorsoAnim;
	int				legsAnimationTimer;

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;
} playerInfo_t;

// new ui stuff
#define MAX_ALIASES				64
#define MAX_TEAMS				64
#define MAX_GAMETYPES			16
#define MAX_MAPS				512 // 128
#define PLAYERS_PER_TEAM		8 //5
#define MAX_PINGREQUESTS		32
#define MAX_DISPLAY_SERVERS		2048
#define MAX_SERVERSTATUS_LINES	128
#define MAX_SERVERSTATUS_TEXT	4096 //1024
#define MAX_FOUNDPLAYER_SERVERS	16
#define TEAM_MEMBERS			8//5
#define MAPS_PER_TIER			3
#define MAX_TIERS				16
#define MAX_MODS				64
#define MAX_DEMOS				2048 // 256
#define MAX_MOVIES				2048 // 256
#define MAX_Q3PLAYERMODELS		1024 //256

#define DEMO_DIRECTORY "demos"
#define DEMO_EXTENSION "dm_"
#define MAX_DEMOLIST (MAX_DEMOS * MAX_QPATH)

#define MAX_SCROLLTEXT_SIZE		4096
#define MAX_SCROLLTEXT_LINES	64

typedef struct aliasInfo_s {
	const char *name;
	const char *ai;
	const char *action;
} aliasInfo_t;

typedef struct teamInfo_s {
	const char *teamName;
	const char *imageName;
	const char *teamMembers[TEAM_MEMBERS];
	qhandle_t teamIcon;
	qhandle_t teamIcon_Metal;
	qhandle_t teamIcon_Name;
	int cinematic;
} teamInfo_t;

typedef struct gameTypeInfo_s {
	const char *gameType;
	int gtEnum;
} gameTypeInfo_t;

typedef struct mapInfo_s {
	const char *mapName;
	const char *mapLoadName;
	const char *imageName;
	const char *opponentName;
	int teamMembers;
	int typeBits;
	int cinematic;
	int timeToBeat[MAX_GAMETYPES];
	qhandle_t levelShot;
	qboolean active;
} mapInfo_t;

typedef struct tierInfo_s {
	const char *tierName;
	const char *maps[MAPS_PER_TIER];
	int gameTypes[MAPS_PER_TIER];
	qhandle_t mapHandles[MAPS_PER_TIER];
} tierInfo_t;

typedef struct serverFilter_s {
	const char *description;
	const char *basedir;
} serverFilter_t;

typedef struct pinglist_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

typedef struct serverStatus_s {
	pinglist_t pingList[MAX_PINGREQUESTS];
	int		numqueriedservers;
	int		currentping;
	int		nextpingtime;
	int		maxservers;
	int		refreshtime;
	int		numServers;
	int		sortKey;
	int		sortDir;
	int		lastCount;
	qboolean refreshActive;
	int		currentServer;
	int		displayServers[MAX_DISPLAY_SERVERS];
	int		numDisplayServers;
	int		numPlayersOnServers;
	int		nextDisplayRefresh;
	int		nextSortTime;
	qhandle_t currentServerPreview;
	int		currentServerCinematic;
	int		motdLen;
	int		motdWidth;
	int		motdPaintX;
	int		motdPaintX2;
	int		motdOffset;
	int		motdTime;
	char	motd[MAX_STRING_CHARS];
} serverStatus_t;

typedef struct pendingServer_s {
	char		adrstr[MAX_ADDRESSLENGTH];
	char		name[MAX_ADDRESSLENGTH];
	int			startTime;
	int			serverNum;
	qboolean	valid;
} pendingServer_t;

typedef struct pendingServerStatus_s {
	int num;
	pendingServer_t server[MAX_SERVERSTATUSREQUESTS];
} pendingServerStatus_t;

typedef struct serverStatusInfo_s {
	char address[MAX_ADDRESSLENGTH];
	char *lines[MAX_SERVERSTATUS_LINES][4];
	char text[MAX_SERVERSTATUS_TEXT];
	char pings[MAX_CLIENTS * 3];
	int numLines;
} serverStatusInfo_t;

typedef struct modInfo_s {
	const char *modName;
	const char *modDescr;
} modInfo_t;

#define SKIN_LENGTH			16
#define ACTION_BUFFER_SIZE	128

typedef struct {
	char name[SKIN_LENGTH];
} skinName_t;

typedef struct {
	char shader[MAX_QPATH];
	char actionText[ACTION_BUFFER_SIZE];
} playerColor_t;

typedef struct playerSpeciesInfo_s {
	char		Name[MAX_QPATH];
	int			SkinHeadCount;
	int			SkinHeadMax;
	skinName_t	*SkinHead;
	int			SkinTorsoCount;
	int			SkinTorsoMax;
	skinName_t	*SkinTorso;
	int			SkinLegCount;
	int			SkinLegMax;
	skinName_t	*SkinLeg;
	int			ColorMax;
	int			ColorCount;
	playerColor_t	*Color;
} playerSpeciesInfo_t;

typedef struct uiInfo_s {
	displayContextDef_t		uiDC;

	int						characterCount;
	int						botIndex;

	int						aliasCount;
	aliasInfo_t				aliasList[MAX_ALIASES];

	int						teamCount;
	teamInfo_t				teamList[MAX_TEAMS];

	int						numGameTypes;
	gameTypeInfo_t			gameTypes[MAX_GAMETYPES];

	int						numJoinGameTypes;
	gameTypeInfo_t			joinGameTypes[MAX_GAMETYPES];

	int						redBlue;
	int						playerCount;
	int						myTeamCount;
	int						teamIndex;
	int						playerRefresh;
	int						playerIndex;
	int						playerNumber;
	qboolean				teamLeader;
	char					playerNames[MAX_CLIENTS][MAX_NETNAME];
	char					teamNames[MAX_CLIENTS][MAX_TEAMNAME];
	int						teamClientNums[MAX_CLIENTS];

	int						playerIndexes[MAX_CLIENTS]; //so we can vote-kick by index

	int						mapCount;
	mapInfo_t				mapList[MAX_MAPS];


	int						tierCount;
	tierInfo_t				tierList[MAX_TIERS];

	int						skillIndex;

	modInfo_t				modList[MAX_MODS];
	int						modCount;
	int						modIndex;

	char					demoList[MAX_DEMOS][MAX_QPATH];
	int						demoCount;
	int						demoIndex;
	int						loadedDemos;

	const char				*movieList[MAX_MOVIES];
	int						movieCount;
	int						movieIndex;
	int						previewMovie;

	char					scrolltext[MAX_SCROLLTEXT_SIZE];
	const char				*scrolltextLine[MAX_SCROLLTEXT_LINES];
	int						scrolltextLineCount;

	serverStatus_t			serverStatus;

	// for the showing the status of a server
	char					serverStatusAddress[MAX_ADDRESSLENGTH];
	serverStatusInfo_t		serverStatusInfo;
	int						nextServerStatusRefresh;

	// to retrieve the status of server to find a player
	pendingServerStatus_t	pendingServerStatus;
	char					findPlayerName[MAX_STRING_CHARS];
	char					foundPlayerServerAddresses[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	char					foundPlayerServerNames[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	int						currentFoundPlayerServer;
	int						numFoundPlayerServers;
	int						nextFindPlayerRefresh;

	int						currentCrosshair;

	int						q3HeadCount;
	char					q3HeadNames[MAX_Q3PLAYERMODELS][64];
	qhandle_t				q3HeadIcons[MAX_Q3PLAYERMODELS];
	int						q3SelectedHead;

	int						forceConfigCount;
	int						forceConfigSelected;
	char					forceConfigNames[MAX_FORCE_CONFIGS][128];
	qboolean				forceConfigSide[MAX_FORCE_CONFIGS]; //true if it's a light side config, false if dark side
	int						forceConfigDarkIndexBegin; //mark the index number dark configs start at
	int						forceConfigLightIndexBegin; //mark the index number light configs start at

	qboolean				inGameLoad;

	int						playerSpeciesMax;
	int						playerSpeciesCount;
	playerSpeciesInfo_t		*playerSpecies;
	int						playerSpeciesIndex;

	short					movesTitleIndex;
	const char				*movesBaseAnim;
	int						moveAnimTime;

	int						languageCount;
	int						languageCountIndex;
} uiInfo_t;
extern uiInfo_t uiInfo;

qboolean	UI_ConsoleCommand( int realTime );
void		UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader );
void		UI_FillRect( float x, float y, float width, float height, const float *color );
char		*UI_Cvar_VariableString( const char *var_name );


//
// ui_gameinfo.c
//

int UI_GetNumBots( void );
void UI_LoadBots( void );
char *UI_GetBotNameByNumber( int num );


//
// ui_saber.c
//

qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel );
qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType );


// new ui

extern uiImport_t *trap;
