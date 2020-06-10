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
} clientStatic_t;

#define	CON_TEXTSIZE	0x30000 //was 32768
#define	NUM_CON_TIMES	4

typedef struct console_s {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens
	float	yadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

extern	clientStatic_t		cls;

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

extern  cvar_t  *cl_lanForcePackets;

extern	cvar_t	*cl_drawRecording;

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

void	CL_ServerInfoPacket( netadr_t from, msg_t *msg );
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
void Con_RunConsole (void);
void Con_DrawConsole (void);
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
void Con_Close( void );


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
e_status CIN_RunCinematic (int handle);
void CIN_DrawCinematic (int handle);
void CIN_SetExtents (int handle, int x, int y, int w, int h);
void CIN_SetLooping (int handle, qboolean loop);
void CIN_UploadCinematic(int handle);
void CIN_CloseAllVideos(void);

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
// cl_avi.c
//
qboolean CL_OpenAVIForWriting( const char *filename );
void CL_TakeVideoFrame( void );
void CL_WriteAVIVideoFrame( const byte *imageBuffer, int size );
void CL_WriteAVIAudioFrame( const byte *pcmBuffer, int size );
qboolean CL_CloseAVI( void );
qboolean CL_VideoRecording( void );
