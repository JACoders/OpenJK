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

// client.h -- primary header for client

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "rd-common/tr_public.h"
#include "keys.h"
#include "snd_public.h"
#include "game/bg_public.h"
#include "cgame/cg_public.h"
#include "ui/ui_public.h"

#define	RETRANSMIT_TIMEOUT	3000	// time between connection packet retransmits

// file full of random crap that gets used to create ja_guid
#define QKEY_FILE "jakey"
#define QKEY_SIZE 2048

// Wind
extern vec3_t cl_windVec;

// snapshots are a view of the server at a given time
typedef struct clSnapshot_s {
	qboolean		valid;			// cleared if delta parsing was invalid
	int				snapFlags;		// rate delayed and dropped commands

	int				serverTime;		// server time the message is valid for (in msec)

	int				messageNum;		// copied from netchan->incoming_sequence
	int				deltaNum;		// messageNum the delta is from
	int				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
	playerState_t	ps;						// complete information about the current player at this time
	playerState_t	vps; //vehicle I'm riding's playerstate (if applicable) -rww

	int				numEntities;			// all of the entities that need to be presented
	int				parseEntitiesNum;		// at the time of this snapshot

	int				serverCommandNum;		// execute all commands up to this before
											// making the snapshot current
} clSnapshot_t;



/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

typedef struct outPacket_s {
	int		p_cmdNumber;		// cl.cmdNumber when packet was sent
	int		p_serverTime;		// usercmd->serverTime when packet was sent
	int		p_realtime;			// cls.realtime when packet was sent
} outPacket_t;

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original
#define	MAX_PARSE_ENTITIES	( PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES )

extern int g_console_field_width;

typedef struct clientActive_s {
	int			timeoutcount;		// it requres several frames in a timeout condition
									// to disconnect, preventing debugging breaks from
									// causing immediate disconnects on continue
	clSnapshot_t	snap;			// latest received from server

	int			serverTime;			// may be paused during play
	int			oldServerTime;		// to prevent time from flowing bakcwards
	int			oldFrameServerTime;	// to check tournament restarts
	int			serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
									// this value changes as net lag varies
	qboolean	extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	qboolean	newSnapshots;		// set on parse of any valid packet

	gameState_t	gameState;			// configstrings
	char		mapname[MAX_QPATH];	// extracted from CS_SERVERINFO

	int			parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	int			mouseDx[2], mouseDy[2];	// added to by mouse events
	int			mouseIndex;
	int			joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	// cgame communicates a few values to the client system
	int			cgameUserCmdValue;	// current weapon to add to usercmd_t
	vec3_t		cgameViewAngleForce;
	int			cgameViewAngleForceTime;
	float		cgameSensitivity;

	int			cgameForceSelection;
	int			cgameInvenSelection;

	qboolean	gcmdSendValue;
	qboolean	gcmdSentValue;
	byte		gcmdValue;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmdNumber;			// incremented each frame, because multiple
									// frames may need to be packed into a single packet

	outPacket_t	outPackets[PACKET_BACKUP];	// information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			serverId;			// included in each client message so the server
												// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t	snapshots[PACKET_BACKUP];

	entityState_t	entityBaselines[MAX_GENTITIES];	// for delta compression when not in previous frame

	entityState_t	parseEntities[MAX_PARSE_ENTITIES];

	char			*mSharedMemory;
} clientActive_t;

extern	clientActive_t		cl;

#define MAX_HEIGHTMAP_SIZE	16000

typedef struct
{
	int			mType;
	int			mSide;
	vec3_t		mOrigin;

} rmAutomapSymbol_t;

#define	MAX_AUTOMAP_SYMBOLS	512

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/


typedef struct clientConnection_s {

	int			clientNum;
	int			lastPacketSentTime;			// for retransmits during connection
	int			lastPacketTime;				// for timeouts

	netadr_t	serverAddress;
	int			connectTime;				// for connection retransmits
	int			connectPacketCount;			// for display on connection dialog
	char		serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog

	int			challenge;					// from the server to use for connecting
	int			checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int			reliableSequence;
	int			reliableAcknowledge;		// the last one the server has executed
	char		reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int			serverMessageSequence;

	// reliable messages received from server
	int			serverCommandSequence;
	int			lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char		serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// file transfer from server
	fileHandle_t download;
	char		downloadTempName[MAX_OSPATH];
	char		downloadName[MAX_OSPATH];
	int			downloadNumber;
	int			downloadBlock;	// block we are waiting for
	int			downloadCount;	// how many bytes we got
	int			downloadSize;	// how many bytes we got
	char		downloadList[MAX_INFO_STRING]; // list of paks we need to download
	qboolean	downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak
	qboolean	downloadMenuActive;
	qboolean	downloadWaitingOnUser;
	qboolean	downloadFinished;
	int			downloadTime;

	// demo information
	char		demoName[MAX_QPATH];
	qboolean	spDemoRecording;
	qboolean	demorecording;
	qboolean	demoplaying;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	firstDemoFrameSkipped;
	fileHandle_t	demofile;

	int			timeDemoFrames;		// counter of rendered frames
	int			timeDemoStart;		// cls.realtime before first frame
	int			timeDemoBaseTime;	// each frame will be at this time + frameNum * 50

	float		aviVideoFrameRemainder;
	float		aviSoundFrameRemainder;

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t	netchan;
} clientConnection_t;

extern	clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct ping_s {
	netadr_t	adr;
	int			start;
	int			time;
	char		info[MAX_INFO_STRING];
} ping_t;

typedef struct serverInfo_s {
	netadr_t	adr;
	char	  	hostName[MAX_NAME_LENGTH];
	char	  	mapName[MAX_NAME_LENGTH];
	char	  	game[MAX_NAME_LENGTH];
	int			netType;
	int			gameType;
	int		  	clients;
	int		  	maxClients;
	int			minPing;
	int			maxPing;
	int			ping;
	qboolean	visible;
	qboolean	needPassword;
	int			trueJedi;
	int			weaponDisable;
	int			forceDisable;
	int			humans, bots;
} serverInfo_t;

typedef struct clientStatic_s {
	connstate_t	state;				// connection status

	char		servername[MAX_OSPATH];		// name of server from original connect (used by reconnect)

	// when the server clears the hunk, all of these must be restarted
	qboolean	rendererStarted;
	qboolean	soundStarted;
	qboolean	soundRegistered;
	qboolean	uiStarted;
	qboolean	cgameStarted;

	int			framecount;
	int			frametime;			// msec since last frame

	int			realtime;			// ignores pause
	int			realFrametime;		// ignoring pause, so console always works

	int			numlocalservers;
	serverInfo_t	localServers[MAX_OTHER_SERVERS];

	int			numglobalservers;
	serverInfo_t  globalServers[MAX_GLOBAL_SERVERS];
	// additional global servers
	int			numGlobalServerAddresses;
	netadr_t		globalServerAddresses[MAX_GLOBAL_SERVERS];

	int			numfavoriteservers;
	serverInfo_t	favoriteServers[MAX_OTHER_SERVERS];

	int pingUpdateSource;		// source currently pinging or updating

	// update server info
	netadr_t	updateServer;
	char		updateChallenge[MAX_TOKEN_CHARS];
	char		updateInfoString[MAX_INFO_STRING];

	netadr_t	authorizeServer;

	// rendering info
	glconfig_t	glconfig;
	qhandle_t	charSetShader;
	qhandle_t	whiteShader;
	qhandle_t	consoleShader;
	int			consoleFont;

	// Cursor
	qboolean	cursorActive;
	qhandle_t	cursorShader;
	int			cursorX;
	int			cursorY;

	// Engine menu
	int			menuFont;

	// cl_ratioFix: how much narrower than 640 wide the screen's 640x480 is, 1 if not at all
	float		widthRatioCoef;
} clientStatic_t;

#define	CON_TEXTSIZE	0x30000 //was 32768
#define	NUM_CON_TIMES	4

typedef union {
	struct {
		unsigned char	color;
		char			character;
		unsigned char	fade;		// 0 drawn solid, up to 255 see-through (chat from far away)
		unsigned char	unused;
	} f;
	unsigned int	compare;
} conChar_t;

typedef struct {
	qboolean	initialized;

	conChar_t	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		rowwidth;		// timestamp, text and line wrap character
	int		totallines;		// total lines in console scrollback

	int		charWidth;		// Scaled console character width
	int		charHeight;		// Scaled console character height

	float	xadjust;		// for wide aspect screens
	float	yadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;

	unsigned char	fade;	// what CL_ConsolePrint fades the text with, set by Con_SetPrintFade
} console_t;

extern	clientStatic_t		cls;

extern	cvar_t	*cl_ratioFix;

extern	refexport_t		*re;		// interface to refresh .dll

//
// cvars
//
extern	cvar_t	*cl_nodelta;
extern	cvar_t	*cl_debugMove;
extern	cvar_t	*cl_noprint;
extern	cvar_t	*cl_timegraph;
extern	cvar_t	*cl_maxpackets;
extern	cvar_t	*cl_packetdup;
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showSend;
extern	cvar_t	*cl_timeNudge;
extern	cvar_t	*cl_showTimeDelta;
extern	cvar_t	*cl_freezeDemo;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;
extern	cvar_t	*cl_run;
extern	cvar_t	*cl_crouch;
extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_sensitivity;
extern	cvar_t	*cl_freelook;

extern	cvar_t	*cl_mouseAccel;
extern	cvar_t	*cl_mouseAccelOffset;
extern	cvar_t	*cl_mouseAccelStyle;
extern	cvar_t	*cl_showMouseRate;

extern	cvar_t	*m_pitchVeh;
extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;
extern	cvar_t	*m_filter;

extern	cvar_t	*cl_timedemo;
extern	cvar_t	*cl_aviFrameRate;
extern	cvar_t	*cl_aviMotionJpeg;
extern	cvar_t	*cl_avi2GBLimit;

extern	cvar_t	*cl_forceavidemo;

extern	cvar_t	*cl_activeAction;

extern	cvar_t	*cl_allowDownload;
extern	cvar_t	*cl_allowAltEnter;
extern	cvar_t	*cl_conXOffset;
extern	cvar_t	*cl_inGameVideo;

extern	cvar_t	*cl_consoleKeys;
extern	cvar_t	*cl_consoleUseScanCode;
extern	cvar_t	*cl_consoleTildeKey;
extern	cvar_t	*cl_consoleShiftRequirement;

extern  cvar_t  *cl_lanForcePackets;

extern	cvar_t	*cl_drawRecording;

extern	cvar_t	*cl_reconnectArgs;

//=================================================

//
// cl_main
//

void CL_Init (void);
void CL_FlushMemory(void);
void CL_ShutdownAll( qboolean shutdownRef );
void CL_AddReliableCommand( const char *cmd, qboolean isDisconnectCmd );

void CL_StartHunkUsers( void );

qboolean CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean CL_GetDefaultState( int index, entityState_t *state );
qboolean CL_GetServerCommand( int serverCommandNumber );
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale, float mPitchOverride, float mYawOverride, float mSensitivityOverride, int fpSel, int invenSel );

void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_Vid_Restart_f( void );
void CL_Snd_Restart_f (void);
void CL_StartDemoLoop( void );
void CL_NextDemo( void );
void CL_ReadDemoMessage( void );

void CL_InitDownloads(void);
void CL_NextDownload(void);

void CL_GetPing( int n, char *buf, int buflen, int *pingtime );
void CL_GetPingInfo( int n, char *buf, int buflen );
void CL_ClearPing( int n );
int CL_GetPingQueueCount( void );

void CL_InitRef( void );

int CL_ServerStatus( const char *serverAddress, char *serverStatusString, int maxLen );

qboolean CL_CheckPaused(void);

void CL_DrawEngineMenus( void );
void CL_UpdateCursorPosition( int dx, int dy );
void CL_CursorButton( int key );

//
// cl_input
//
typedef struct kbutton_s {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

void CL_InitInput (void);
void CL_ShutdownInput(void);
void CL_SendCmd (void);
void CL_ClearState (void);

void CL_WritePacket( void );

float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString( int keynum/*, qboolean bTranslate */ ); //note: translate is only called for menu display not configs

//
// cl_parse.c
//
extern int cl_connectedToPureServer;
extern int cl_connectedToCheatServer;

void CL_SystemInfoChanged( void );
void CL_ParseServerMessage( msg_t *msg );

//====================================================================

void	CL_ServerInfoPacket( const netadr_t *from, msg_t *msg );
void	CL_LocalServers_f( void );
void	CL_GlobalServers_f( void );
void	CL_FavoriteServers_f( void );
void	CL_Ping_f( void );
qboolean CL_UpdateVisiblePings_f( int source );


//
// console
//

void Con_CheckResize (void);
void Con_Init (void);
void Con_Shutdown(void);
void Con_Clear_f (void);
void Con_ToggleConsole_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_SetPrintFade( int fade );
void Con_RunConsole (void);
void Con_DrawConsole (void);
void Con_FontsLost( void );
const char *Con_FontName( int index );
int Con_NumFonts( void );
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
void Con_Close( void );
qboolean Con_MouseMove( int dx, int dy );
qboolean Con_MouseKey( int key, qboolean down, qboolean shift );
qboolean Con_KeyEvent( int key, int lower, qboolean ctrl, qboolean shift );
qboolean Con_CharEvent( int ch );
qboolean Con_FindEscape( void );
void Con_MouseReset( void );


//
// cl_scrn.c
//
void	SCR_Init (void);
void	SCR_UpdateScreen (void);

void	SCR_DebugGraph (float value, int color);

int		SCR_GetBigStringWidth( const char *str );	// returns in virtual 640x480 coordinates

void	SCR_FillRect( float x, float y, float width, float height,
					 const float *color );
void	SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void	SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname );

void	SCR_DrawBigString( int x, int y, const char *s, float alpha, qboolean noColorEscape );			// draws a string with embedded color control characters with fade
void	SCR_DrawBigStringColor( int x, int y, const char *s, vec4_t color, qboolean noColorEscape );	// ignores embedded color control characters
void	SCR_DrawSmallStringExt( int x, int y, const char *string, float *setColor, qboolean forceColor, qboolean noColorEscape );
const float *Con_TypedColor( const char *p );
void	SCR_DrawChar( int x, int y, float size, int ch );
void	SCR_DrawSmallChar( int x, int y, int ch );


//
// cl_cin.c
//

void CL_PlayCinematic_f( void );
void SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status CIN_StopCinematic(int handle);

// cl_cin_video.cpp
typedef struct cinVideo_s cinVideo_t;
cinVideo_t *CIN_VideoOpen( const char *name );
void CIN_VideoClose( cinVideo_t *v );
int CIN_VideoWidth( const cinVideo_t *v );
int CIN_VideoHeight( const cinVideo_t *v );
byte *CIN_VideoBuffer( const cinVideo_t *v );
qboolean CIN_VideoUpdate( cinVideo_t *v, double time, qboolean silent, qboolean *newFrame );
void CIN_VideoRewind( cinVideo_t *v );

e_status CIN_RunCinematic (int handle);
void CIN_DrawCinematic (int handle);
void CIN_SetExtents (int handle, int x, int y, int w, int h);
void CIN_SetLooping (int handle, qboolean loop);
void CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);
qboolean CIN_PlayingWithSound( void );

//
// cl_cgame.c
//
void CL_InitCGame( void );
void CL_ShutdownCGame( void );
qboolean CL_GameCommand( void );
void CL_CGameRendering( stereoFrame_t stereo );
void CL_SetCGameTime( void );
void CL_FirstSnapshot( void );
void CL_ShaderStateChanged(void);

//
// cl_ui.c
//
void CL_InitUI( void );
void CL_ShutdownUI( void );
int Key_GetCatcher( void );
void Key_SetCatcher( int catcher );
void LAN_LoadCachedServers();
void LAN_SaveServersToCache();


//
// cl_net_chan.c
//
void CL_Netchan_Transmit( netchan_t *chan, msg_t* msg);	//int length, const byte *data );
void CL_Netchan_TransmitNextFragment( netchan_t *chan );
qboolean CL_Netchan_Process( netchan_t *chan, msg_t *msg );

//
// cl_modelmanager.cpp
//
#define KEYCATCH_MODELMANAGER	0x0010	// engine only, the model manager owns keyboard and mouse

void CL_ModelManager_Init( void );
void CL_ModelManager_Shutdown( void );
void CL_ModelManager_f( void );
qboolean CL_ModelManager_Active( void );
void CL_ModelManager_Close( void );
void CL_ModelManager_OpenForStory( void );
void CL_ModelManager_KeyEvent( int key, qboolean down );
void CL_ModelManager_CharEvent( int ch );
void CL_ModelManager_MouseEvent( int dx, int dy );
void CL_ModelManager_Escape( void );
void CL_ModelManager_Frame( void );
qboolean CL_ModelManager_FilterEntity( const refEntity_t *ent );
void CL_ModelManager_RenderScene( const refdef_t *fd );
qboolean CL_ModelManager_Camera( vec3_t origin, vec3_t angles );
qboolean CL_ModelManager_ServerPrint( const char *text );
void CL_ModelManager_Draw( void );

//
// cl_npcmanager.cpp
//
#define KEYCATCH_NPCMANAGER		0x0020	// engine only, the NPC manager owns keyboard and mouse
#define KEYCATCH_SHADERMANAGER	0x0040	// engine only, the shader manager owns keyboard and mouse
#define KEYCATCH_EFFECTMANAGER	0x0080	// engine only, the effect manager owns keyboard and mouse
#define KEYCATCH_STORYTELLER	0x0100	// engine only, the storyteller owns keyboard and mouse
#define KEYCATCH_CINEDITOR		0x0200	// engine only, the camera editor owns keyboard and mouse
#define KEYCATCH_MAPEDITOR		0x0400	// engine only, the map editor owns keyboard and mouse
#define KEYCATCH_SOUNDMANAGER	0x0800	// engine only, the sound or the music manager owns keyboard and mouse
#define KEYCATCH_RADIALMENU		0x1000	// engine only, the radial menus' settings own keyboard and mouse
#define KEYCATCH_LANGUAGES		0x2000	// engine only, the languages window owns keyboard and mouse
#define KEYCATCH_CONSETTINGS	0x4000	// engine only, the console settings window owns keyboard and mouse
#define KEYCATCH_ANIMMANAGER	0x8000	// engine only, the animation manager owns keyboard and mouse
#define KEYCATCH_PRESETS		0x10000	// engine only, the presets own keyboard and mouse
#define KEYCATCH_FEEDBACK		0x20000	// engine only, the feedback window owns keyboard and mouse
#define KEYCATCH_ASSETMANAGER	0x40000	// engine only, the asset manager owns keyboard and mouse
#define KEYCATCH_ENGINETOOLS	( KEYCATCH_MODELMANAGER | KEYCATCH_NPCMANAGER | KEYCATCH_SHADERMANAGER | KEYCATCH_EFFECTMANAGER | KEYCATCH_MAPEDITOR | KEYCATCH_STORYTELLER | KEYCATCH_CINEDITOR | KEYCATCH_SOUNDMANAGER | KEYCATCH_RADIALMENU | KEYCATCH_LANGUAGES | KEYCATCH_CONSETTINGS | KEYCATCH_ANIMMANAGER | KEYCATCH_PRESETS | KEYCATCH_FEEDBACK | KEYCATCH_ASSETMANAGER )	// mods never see or clear these

void CL_NpcManager_Init( void );
void CL_NpcManager_Shutdown( void );
void CL_NpcManager_f( void );
const char *CL_NpcManager_OutgoingChat( const char *cmd );
int CL_NpcManager_ListTime( void );

// cl_npcwalk.cpp
#define NW_MAX_ROUTE_POINTS	32
qboolean CL_NpcWalk_Entity( entityState_t *es, int serverTime );
void CL_NpcWalk_Frame( void );
void CL_NpcWalk_Reset( void );
void CL_NpcWalk_Hook( char **argv, int argc );
void CL_NpcWalk_GroupHook( char **argv, int argc );
int CL_LastCommandTime( void );
qboolean CL_NpcWalk_CanSend( void );
void CL_Storyteller_AddToScene( const refdef_t *fd );
int CL_NpcWalk_Target( const char *target );
void CL_RpCinematic_ShotWalks( const char *name, qboolean local );
void CL_NpcWalk_GroupSpot( const vec3_t spot, const vec3_t offset, vec3_t out );
void CL_NpcWalk_GroupMiddle( const vec3_t *starts, int count, vec3_t middle );
void CL_NpcWalk_Command( void );
typedef struct {
	int				num;		// the NPC's entity number
	const char		*route;		// the saved route's name, "" when not told
	qboolean		loop, driver;
	float			speed;
	const vec3_t	*pts;		// where it set off, then the route's spots
	int				numPts;
	vec3_t			pos;		// where it is now
} npcWalkInfo_t;
int CL_NpcWalk_NumWalks( void );
qboolean CL_NpcWalk_GetWalk( int index, npcWalkInfo_t *out );
const char *CL_NpcWalk_RouteOf( int num );
void CL_NpcWalk_ListRead( int listTime );
qboolean CL_NpcWalk_SaveRoute( const char *name, const vec3_t *pts, int numPts, float speed );
int CL_NpcWalk_LoadRoute( const char *name, vec3_t *pts, int maxPts, float *speed );
qboolean CL_NpcWalk_NpcOrigin( int num, vec3_t out );
int CL_NpcWalk_Targets( const char *target, int *nums, int max );
float CL_NpcWalk_PaceSpeed( qboolean run );
int CL_NpcWalk_RouteMs( const vec3_t *pts, int numPts, float speed );
qboolean CL_NpcWalk_RouteAt( const vec3_t *pts, int numPts, float speed, int t, vec3_t pos, float *yaw );
void CL_NpcWalk_Pose( int num, const vec3_t pos, float yaw, int pace );
void CL_NpcWalk_ClearPoses( void );
qboolean CL_NpcWalk_PoseOf( int num, vec3_t out );
qboolean CL_NpcWalk_PoseYaw( int num, float *yaw );
void CL_NpcWalk_PosePlayer( playerState_t *ps );
// what a cinematic's recording has someone doing, beyond where they are
typedef struct {
	int			legsAnim, torsoAnim;
	qboolean	legsFlip, torsoFlip;
	int			forcePowersActive;
	int			saberHolstered;
	int			saberMove;
} npcPoseState_t;
void CL_NpcWalk_PoseEvent( int num, int event, int parm );
void CL_NpcWalk_PoseEx( int num, const vec3_t pos, const vec3_t angles, int pace, qboolean free, const npcPoseState_t *st, qboolean tilt );
int CL_NpcWalk_Tilts( vec3_t *pos, vec3_t *angles, int max );
int CL_EmotePreview_Count( void );
const char *CL_EmotePreview_Name( int index );
const char *CL_EmotePreview_Anim( const char *emote );
qboolean CL_Cinematic_RecordKey( int key );
qboolean CL_NpcManager_Active( void );
void CL_NpcManager_Frame( void );
void CL_NpcManager_Close( void );
void CL_NpcManager_OpenForStory( qboolean manage );
void CL_NpcManager_KeyEvent( int key, qboolean down );
void CL_NpcManager_CharEvent( int ch );
void CL_NpcManager_MouseEvent( int dx, int dy );
void CL_NpcManager_Escape( void );
qboolean CL_NpcManager_ServerPrint( const char *text );
void CL_NpcManager_RequestList( void );
qboolean CL_NpcManager_Waiting( void );
int CL_NpcManager_NumNpcs( void );
qboolean CL_NpcManager_GetNpc( int index, int *num, const char **type, const char **name );
qboolean CL_NpcManager_NpcOrigin( int num, vec3_t out );
const char *CL_NpcManager_SpawnCommand( const char *type, qboolean vehicle, const char *targetname, const char *team,
	const char *model, const char *scale, const vec3_t pos, int yaw );
qboolean CL_NpcManager_RenderScene( const refdef_t *fd );
qboolean CL_NpcManager_Camera( vec3_t origin, vec3_t angles );
qboolean CL_NpcManager_FilterEntity( const refEntity_t *ent );
void CL_NpcManager_Draw( void );

//
// cl_favorites.cpp: the managers' favorites, by list ("models", "effects", "shaders")
//
qboolean CL_Fav_Has( const char *list, const char *name );
qboolean CL_Fav_Toggle( const char *list, const char *name );
int CL_Fav_Count( const char *list );
const char *CL_Fav_Star( qboolean on );

//
// cl_shadermanager.cpp
//
void CL_ShaderManager_Init( void );
void CL_ShaderManager_Shutdown( void );
void CL_ShaderManager_f( void );
qhandle_t CL_ShaderManager_RegisterModel( const char *name );
qhandle_t CL_ShaderManager_RegisterSkin( const char *name );
void CL_ShaderManager_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
void CL_ShaderManager_AddEntity( const refEntity_t *ent );
qboolean CL_ShaderManager_RenderScene( const refdef_t *fd );
qboolean CL_ShaderManager_FilterEntity( const refEntity_t *ent );
qboolean CL_ShaderManager_Active( void );
void CL_ShaderManager_Close( void );
void CL_ShaderManager_OpenForStory( int tab );
void CL_ShaderManager_KeyEvent( int key, qboolean down );
void CL_ShaderManager_CharEvent( int ch );
void CL_ShaderManager_MouseEvent( int dx, int dy );
void CL_ShaderManager_Escape( void );
void CL_ShaderManager_Frame( void );
void CL_ShaderManager_ServerPrint( const char *text );
void CL_ShaderManager_SkyLook( void );
void CL_ShaderManager_SkyHook( char **argv, int argc );
void CL_ShaderManager_Draw( void );
const char *CL_ShaderManager_SkinName( qhandle_t h );
qboolean CL_ShaderManager_Camera( vec3_t origin, vec3_t angles );
void CL_ShaderManager_AddEffect( qhandle_t shader, const vec3_t mins, const vec3_t maxs, float radius );
// the Mine tab's player shaders, this player's and those others shared, each on their own
// effects, sabers and shells only
int CL_ShaderManager_Owner( const vec3_t org, int entNum );
qhandle_t CL_ShaderManager_OwnerShader( int owner, qhandle_t shader );
qhandle_t CL_ShaderManager_MineFor( qhandle_t shader, const vec3_t org, int entNum );
const refEntity_t *CL_ShaderManager_MineEntity( const refEntity_t *ent, refEntity_t *copy );
int CL_ShaderManager_ShaderCount( void );
const char *CL_ShaderManager_ShaderName( int i );
qboolean CL_ShaderManager_IsSky( const char *name );
qboolean CL_ShaderManager_Grows( const char *name );
void CL_ShaderManager_RequestOverlays( void );
int CL_ShaderManager_NumOverlays( void );
const char *CL_ShaderManager_OverlayShader( int i );
int CL_ShaderManager_NumRemaps( void );
const char *CL_ShaderManager_RemapFrom( int i );
const char *CL_ShaderManager_RemapTo( int i );
void CL_ModelManager_RequestList( void );
int CL_ModelManager_NumPlaced( void );
const char *CL_ModelManager_PlacedName( int i );
void CL_SoundManager_RequestList( void );
int CL_SoundManager_NumLoops( void );
const char *CL_SoundManager_LoopPath( int i );
void CL_AssetManager_Init( void );
void CL_AssetManager_Shutdown( void );
void CL_AssetManager_Close( void );
void CL_AssetManager_Frame( void );
void CL_AssetManager_KeyEvent( int key, qboolean down );
void CL_AssetManager_CharEvent( int ch );
void CL_AssetManager_MouseEvent( int dx, int dy );
void CL_AssetManager_Escape( void );
void CL_AssetManager_Draw( void );
void CL_ShaderManager_MineHook( char **argv, int argc, const char *message );

//
// cl_effectmanager.cpp
//
void CL_EffectManager_Init( void );
void CL_EffectManager_Shutdown( void );
void CL_EffectManager_f( void );
qboolean CL_EffectManager_Active( void );
void CL_EffectManager_Close( void );
void CL_EffectManager_OpenForStory( void );
void CL_EffectManager_RequestList( void );
int CL_EffectManager_NumActive( void );
qboolean CL_EffectManager_GetActive( int index, int *num, const char **path, vec3_t origin );
void CL_EffectManager_KeyEvent( int key, qboolean down );
void CL_EffectManager_CharEvent( int ch );
void CL_EffectManager_MouseEvent( int dx, int dy );
void CL_EffectManager_Escape( void );
void CL_EffectManager_Frame( void );
void CL_EffectManager_AddEntity( const refEntity_t *ent );
qboolean CL_EffectManager_FilterEntity( const refEntity_t *ent );
qboolean CL_EffectManager_RenderScene( const refdef_t *fd );
qboolean CL_EffectManager_Camera( vec3_t origin, vec3_t angles );
qboolean CL_EffectManager_ServerPrint( const char *text );
void CL_EffectManager_Draw( void );
float CL_EffectManager_FxScale( const char *effect, const vec3_t org );
void CL_EffectManager_OpenForCinematic( void );
qboolean CL_Cinematic_CaptureEffect( const char *cmd, float scale );
qboolean CL_Cinematic_CaptureSound( const char *cmd );
void CL_SoundManager_OpenForCinematic( void );
qboolean CL_Cinematic_PickingEffect( void );
void CL_EffectManager_ScaleHook( char **argv, int argc, const char *message );

// cl_mapeditor.cpp
void CL_MapEditor_Init( void );
void CL_MapEditor_Shutdown( void );
void CL_MapEditor_f( void );
void CL_MapEditor_Open( void );
void CL_MapEditor_Close( void );
qboolean CL_MapEditor_Active( void );
void CL_MapEditor_OpenForStory( void );
qboolean CL_Storyteller_ChatLine( void );
qboolean CL_Storyteller_ServerPrint( const char *text );
void CL_Storyteller_Hook( char **argv, int argc, const char *message );
void CL_Storyteller_TriggerUsed( const char *name );
int CL_Storyteller_UseLock( const char *name, const char **text );
const char *CL_MapEditor_AimedUseTrigger( void );
void CL_MapEditor_PickForStory( int kind );
void CL_Storyteller_TriggerPicked( const char *value );
void CL_RpCinematic_AnimHook( char **argv, int argc );
void CL_RpCinematic_SnapEntity( entityState_t *es );
void CL_RpCinematic_SnapPlayer( playerState_t *ps );
qboolean CL_MapEditor_ServerPrint( const char *text );
void CL_MapEditor_KeyEvent( int key, qboolean down );
void CL_MapEditor_CharEvent( int ch );
void CL_MapEditor_MouseEvent( int dx, int dy );
void CL_MapEditor_Escape( void );
void CL_MapEditor_Frame( void );
void CL_MapEditor_Draw( void );
void CL_MapEditor_Sounds( const vec3_t ear );
void CL_MapEditor_AddToScene( const refdef_t *fd );
qboolean CL_MapEditor_RenderScene( const refdef_t *fd );
qboolean CL_MapEditor_Camera( vec3_t origin, vec3_t angles );
qboolean CL_MapEditor_FilterEntity( const refEntity_t *ent );
qboolean CL_MapEditor_Hide2D( float x, float y, float w, float h );
int CL_MapEditor_LightStyle( int style, int color );

// cl_rphooks.cpp
qboolean CL_RpHooks_ServerCommand( qboolean *replaced );
qboolean CL_RpHooks_AllowCommand( const char *cmd );
int CL_RpHooks_LongestWord( const char *text );
void CL_RpHooks_Init( void );
void CL_RpHooks_Shutdown( void );
void CL_RpHooks_Disconnect( void );
void CL_RpHooks_Draw( void );
void CL_RpHooks_DrawSubtitles( void );
void CL_RpHooks_ChatSubtitle( void );
// the features sent as cmd: hooks, which only players with this client see: off until turned on
// in a tool's TOGGLE EXPERIMENTAL, for the session, or with rpexperimental, for good
#define RP_EXPERIMENTAL_LABEL	"TOGGLE EXPERIMENTAL"
#define RP_EXPERIMENTAL_WARNING	"These features are a work in progress. They should not be used on Temple, and can only be viewed by the people currently in-game with OpenJK-rpmod installed and up to date."
#define RP_EXPERIMENTAL_OFF		"Experimental: turn it on with " RP_EXPERIMENTAL_LABEL
qboolean CL_RpHooks_Experimental( void );
void CL_MapEditor_Hook( char **argv, int argc, const char *message );
void CL_RpHooks_SetExperimental( qboolean on );
void CL_RpHooks_Unescape( char *s );
const char *CL_RpHooks_NpcTalkCommand( const char *npc, const char *name, const char *color, const char *emote, const char *text );

// cl_rpcinematic.cpp
void CL_RpCinematic_Init( void );
void CL_RpCinematic_Shutdown( void );
void CL_RpCinematic_Reset( void );
void CL_RpCinematic_Stop( void );
void CL_RpCinematic_Hook( char **argv, int argc );
qboolean CL_RpCinematic_Active( void );
qboolean CL_RpCinematic_Subtitles( void );
qboolean CL_RpCinematic_Hide2D( float x, float y, float w, float h );
qboolean CL_RpCinematic_RenderScene( const refdef_t *fd );
qboolean CL_RpCinematic_Camera( vec3_t origin, vec3_t angles );
const refEntity_t *CL_RpCinematic_Entity( const refEntity_t *ent, refEntity_t *copy );
void CL_RpCinematic_RestartAnim( int num );

// cl_animmanager.cpp
void CL_AnimManager_Init( void );
void CL_AnimManager_Shutdown( void );
void CL_AnimManager_f( void );
qboolean CL_AnimManager_Active( void );
void CL_AnimManager_Close( void );
void CL_AnimManager_KeyEvent( int key, qboolean down );
void CL_AnimManager_CharEvent( int ch );
void CL_AnimManager_MouseEvent( int dx, int dy );
void CL_AnimManager_Escape( void );
void CL_AnimManager_Frame( void );
void CL_AnimManager_Draw( void );
void CL_AnimManager_Entity( const refEntity_t *ent );
void CL_AnimManager_Hook( char **argv, int argc, const char *message );

// cl_rendermenu.cpp: the camera follows right behind with the camera settings on
void CL_CameraSettings_Frame( void );
// and the RENDERER page's effects show as Vulkan and Post Effects are picked on it
void CL_RenderMenu_Frame( void );

// cl_adminflags.cpp
void CL_AdminFlags_Init( void );
void CL_AdminFlags_Reset( void );
void CL_AdminFlags_Command( void );
void CL_AdminFlags_ServerPrint( const char *text );
void CL_AdminFlags_Draw( void );

// cl_discord.cpp
void CL_StripAllColors( char *text );
void CL_Discord_Init( void );
void CL_Discord_Frame( void );
void CL_Discord_Shutdown( void );

// cl_quickconnect.cpp
void CL_QuickConnect_Init( void );
void CL_QuickConnect_Frame( void );
qboolean CL_QuickConnect_Escape( void );
qboolean CL_QuickConnect_InfoResponse( const netadr_t *from, const char *info );

// cl_presets.cpp
void CL_Presets_Init( void );
void CL_Presets_Shutdown( void );
void CL_Presets_Close( void );
void CL_Presets_KeyEvent( int key, qboolean down );
void CL_Presets_MouseEvent( int dx, int dy );
void CL_Presets_CharEvent( int ch );
void CL_Presets_Escape( void );
void CL_Presets_Draw( void );
void CL_RpCinematic_Draw( void );
void CL_RpCinematic_Frame( void );
qboolean CL_RpCinematic_ShotHook( const char *name, char *hook, int size, int *ms );
void CL_RpCinematic_Edit( const char *name, qboolean fromStoryteller );
void CL_RpCinematic_EditorClose( void );
qboolean CL_RpCinematic_EditorActive( void );
void CL_RpCinematic_EditorKeyEvent( int key, qboolean down );
void CL_RpCinematic_EditorCharEvent( int ch );
void CL_RpCinematic_EditorMouseEvent( int dx, int dy );
void CL_RpCinematic_EditorEscape( void );
void CL_Cinematic_Hook( char **argv, int argc, const char *message );
int CL_Cinematic_PlayMs( const char *name );

// cl_storyteller.cpp
// cl_soundmanager.cpp
void CL_SoundManager_Init( void );
void CL_SoundManager_Shutdown( void );
void CL_SoundManager_f( void );
void CL_MusicManager_f( void );
qboolean CL_SoundManager_Active( void );
qboolean CL_MusicManager_Active( void );
void CL_SoundManager_Close( void );
void CL_SoundManager_OpenForStory( void );
void CL_MusicManager_OpenForStory( void );
void CL_SoundManager_KeyEvent( int key, qboolean down );
void CL_SoundManager_CharEvent( int ch );
void CL_SoundManager_MouseEvent( int dx, int dy );
void CL_SoundManager_Escape( void );
void CL_SoundManager_Frame( void );
qboolean CL_SoundManager_ServerPrint( const char *text );
void CL_SoundManager_Draw( void );

// cl_radialmenu.cpp
void CL_RadialMenu_Init( void );
void CL_RadialMenu_Shutdown( void );
qboolean CL_RadialMenu_Active( void );
qboolean CL_RadialMenu_MouseEvent( int dx, int dy );
qboolean CL_RadialMenu_KeyDown( int key );
qboolean CL_RadialMenu_Escape( void );
void CL_RadialMenu_Frame( void );
void CL_RadialMenu_Draw( void );
void CL_RadialMenu_CloseSettings( void );
void CL_RadialMenu_KeyEvent( int key, qboolean down );
void CL_RadialMenu_CharEvent( int ch );
void CL_RadialMenu_SettingsMouseEvent( int dx, int dy );
void CL_RadialMenu_SettingsEscape( void );
void CL_RadialMenu_DrawSettings( void );

// cl_consettings.cpp
void CL_ConSettings_Init( void );
void CL_ConSettings_Shutdown( void );
qboolean CL_ConSettings_Active( void );
void CL_ConSettings_Close( void );
void CL_ConSettings_Draw( void );
void CL_ConSettings_KeyEvent( int key, qboolean down );
void CL_ConSettings_CharEvent( int ch );
void CL_ConSettings_MouseEvent( int dx, int dy );
void CL_ConSettings_Escape( void );
qhandle_t CL_ConSettings_Background( void );
void CL_ConSettings_RendererRestarted( void );

// cl_feedback.cpp
void CL_Feedback_Init( void );
void CL_Feedback_Shutdown( void );
void CL_Feedback_Frame( void );
qboolean CL_Feedback_Active( void );
void CL_Feedback_Close( void );
void CL_Feedback_Draw( void );
void CL_Feedback_KeyEvent( int key, qboolean down );
void CL_Feedback_CharEvent( int ch );
void CL_Feedback_MouseEvent( int dx, int dy );
void CL_Feedback_Escape( void );
void CL_Feedback_RendererRestarted( void );
qboolean CL_Feedback_ServerPrint( const char *text );
void CL_Feedback_Disconnect( void );

// cl_languages.cpp
void CL_Languages_Init( void );
void CL_Languages_Shutdown( void );
qboolean CL_Languages_Active( void );
void CL_Languages_Close( void );
void CL_Languages_Frame( void );
void CL_Languages_Draw( void );
void CL_Languages_KeyEvent( int key, qboolean down );
void CL_Languages_CharEvent( int ch );
void CL_Languages_MouseEvent( int dx, int dy );
void CL_Languages_Escape( void );
const char *CL_Languages_OutgoingChat( const char *cmd );

//
// cl_chatsplit.cpp
//
void CL_ChatSplit_Init( void );
int Con_ModernInput( void );	// cl_console.cpp: con_modernInput
void CL_ChatSplit_Frame( void );
qboolean CL_ChatSplit_Forward( void );
void CL_ChatCaps_Text( const char *cmd, char *text );
qboolean CL_ChatCaps_Forward( char *out, int size );

//
// cl_saberlight.cpp
//
void CL_NpcNames_Init( void );
void CL_NpcNames_Disconnect( void );
void CL_NpcNames_Frame( void );
void CL_NpcNames_Draw( void );
void CL_NpcNames_View( const refdef_t *fd );
void CL_NpcNames_Hook( char **argv, int argc, const char *message );

void CL_SaberLight_Init( void );
void CL_SaberLight_Shutdown( void );
void CL_SaberLight_RendererRestarted( void );
void CL_SaberLight_Entity( const refEntity_t *ent );
qboolean CL_SaberLight_GameLight( const vec3_t org, float radius, float r, float g, float b );
void CL_SaberLight_ClearScene( void );
void CL_SaberLight_RenderScene( void );
int CL_ShaderManager_NearestPlayer( const vec3_t org );	// cl_shadermanager.cpp
void CL_Languages_ServerCommand( void );
const char *CL_Languages_Filter( const char *text );

// cl_saberstyle.cpp
void CL_SaberStyle_Init( void );
qboolean CL_SaberStyle_Cycle( void );
void CL_SaberStyle_Frame( void );

// cl_emotepreview.cpp
void CL_EmotePreview_Init( void );
void CL_EmotePreview_Shutdown( void );
void CL_EmotePreview_Command( const char *cmd );
void CL_EmotePreview_Frame( void );
void CL_EmotePreview_Draw( void );
const char *CL_GlmBounds( const char *path, vec3_t mins, vec3_t maxs );

// cl_rpadmin.cpp
void CL_RpAdmin_Init( void );
void CL_RpAdmin_Disconnect( void );
qboolean CL_RpAdmin_Allow( const char *tool, const char *command );
qboolean CL_RpAdmin_ServerPrint( const char *text );
qboolean CL_RpAdmin_IsAdmin( void );
void CL_RpAdmin_AskQuietly( void );
void CL_RpAdmin_Frame( void );

// cl_update.cpp
void CL_Update_Init( void );
void CL_Update_Shutdown( void );
// cl_rendermenu.cpp: the RENDERER page in the Setup menu
void CL_RenderMenu_Init( void );
void CL_RenderMenu_Shutdown( void );
void CL_Update_Frame( void );
void CL_Update_Draw( void );

void CL_Storyteller_Init( void );
void CL_Storyteller_Shutdown( void );
void CL_Storyteller_f( void );
qboolean CL_Storyteller_Active( void );
void CL_Storyteller_Close( void );
void CL_Storyteller_KeyEvent( int key, qboolean down );
void CL_Storyteller_CharEvent( int ch );
void CL_Storyteller_MouseEvent( int dx, int dy );
void CL_Storyteller_Escape( void );
void CL_Storyteller_Frame( void );
void CL_Storyteller_Draw( void );
void CL_Storyteller_Open( void );
void CL_Storyteller_ShotSaved( const char *name );
qboolean CL_Storyteller_Capture( const char *cmd );
qboolean CL_Storyteller_Picking( void );

//
// cl_fighterassist.cpp
//
void CL_FighterAssist_Init( void );
void CL_FighterAssist_Shutdown( void );
void CL_FighterAssist_FilterCmd( usercmd_t *cmd, qboolean boost );

//
// cl_chatrange.cpp
//
void CL_ChatRange_Init( void );
void CL_ChatRange_Shutdown( void );
void CL_ChatRange_ServerCommand( void );
void CL_ChatRange_OutgoingCommand( const char *cmd );
qboolean CL_ChatSenderDistance( const char *line, float *distance );

//
// cl_whisper.cpp
//
void CL_Whisper_Init( void );
void CL_Whisper_ServerCommand( void );

//
// cl_cgame.cpp: the client's own commands for the cgame
//
int CL_QueueCgameCommand( const char *text );
int CL_LastCgameCommand( void );
void CL_ResetCgameCommands( void );

//
// cl_loadpk3.cpp
//
void CL_LoadPk3_Init( void );
void CL_LoadPk3_Frame( void );
void CL_LoadPk3_Attached( void *ghoul2, int entityNum );
void CL_EmotePreview_PlayerAnims( void *ghoul2, int legsAnim, int torsoAnim );
int S_ForgetMissing( void );
void CL_ChatRange_CgamePrint( const char *text );

//
// cl_avi.c
//
qboolean CL_OpenAVIForWriting( const char *filename );
void CL_TakeVideoFrame( void );
void CL_WriteAVIVideoFrame( const byte *imageBuffer, int size );
void CL_WriteAVIAudioFrame( const byte *pcmBuffer, int size );
qboolean CL_CloseAVI( void );
qboolean CL_VideoRecording( void );
