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

// client.h -- primary header for client
#pragma once

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../rd-common/tr_public.h"
#include "keys.h"
#include "snd_public.h"
#include "../cgame/cg_public.h"

// snapshots are a view of the server at a given time
typedef struct {
	qboolean		valid;			// cleared if delta parsing was invalid
	int				snapFlags;		// rate delayed and dropped commands

	int				serverTime;		// server time the message is valid for (in msec)

	int				messageNum;		// copied from netchan->incoming_sequence
	int				deltaNum;		// messageNum the delta is from
	int				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
	playerState_t	ps;						// complete information about the current player at this time

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

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define MAX_PARSE_ENTITIES	512

extern int g_console_field_width;

typedef struct {
	int			timeoutcount;

	clSnapshot_t	frame;			// latest received from server

	int			serverTime;
	int			oldServerTime;		// to prevent time from flowing bakcwards
	int			oldFrameServerTime;	// to check tournament restarts
	int			serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
									// this value changes as net lag varies
	qboolean	extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	qboolean	newSnapshots;		// set on parse, cleared when CL_AdjustTimeDelta looks at it

	gameState_t	gameState;			// configstrings
	char		mapname[MAX_QPATH];	// extracted from CS_SERVERINFO

	int			parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	int			mouseDx[2], mouseDy[2];	// added to by mouse events
	int			mouseIndex;
	int			joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	int			cgameUserCmdValue;	// current weapon to add to usercmd_t
	float		cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmdNumber;			// incremented each frame, because multiple
									// frames may need to be packed into a single packet

	int			packetTime[PACKET_BACKUP];	// cls.realtime sent, for calculating pings
	int			packetCmdNumber[PACKET_BACKUP];	// cmdNumber when packet was sent

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	// these are just parsed out of the configstrings for convenience
	int			serverId;

	// non-gameserver infornamtion
	int			cinematictime;		// cls.realtime for first cinematic frame (FIXME: NO LONGER USED!, but I wasn't sure if I could remove it because of struct sizes assumed elsewhere? -Ste)

	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t	frames[PACKET_BACKUP];

	entityState_t	parseEntities[MAX_PARSE_ENTITIES];

	//DJC added - making force powers in single player work like those in
	//multiplayer.  This makes hot swapping code more portable.
	qboolean		gcmdSendValue;
	byte			gcmdValue;
} clientActive_t;

extern	clientActive_t		cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, or connect to a different server

A connection can be to either a server through the network layer,
or just a streaming cinematic.

=============================================================================
*/


typedef struct {		
	int			lastPacketSentTime;			// for retransmits
	int			lastPacketTime;
	char		servername[MAX_OSPATH];		// name of server from original connect
	netadr_t	serverAddress;
	int			connectTime;		// for connection retransmits
	int			connectPacketCount;	// for display on connection dialog

	int			challenge;			// from the server to use for connecting

	int			reliableSequence;
	int			reliableAcknowledge;
	char		*reliableCommands[MAX_RELIABLE_COMMANDS];

	// reliable messages received from server
	int			serverCommandSequence;
	char		*serverCommands[MAX_RELIABLE_COMMANDS];

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

typedef struct {
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
	float		frametimeFraction;	// fraction of a msec since last frame

	int			realtime;			// ignores pause
	float		realtimeFraction;	// fraction of a msec accumulated
	int			realFrametime;		// ignoring pause, so console always works

	// update server info
	char		updateInfoString[MAX_INFO_STRING];

	// rendering info
	glconfig_t	glconfig;
	qhandle_t	charSetShader;
	qhandle_t	whiteShader;
	qhandle_t	consoleShader;
} clientStatic_t;

#define	CON_TEXTSIZE	0x30000 //was 32768
#define	NUM_CON_TIMES	4

typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens
	float	yadjust;		

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

extern	clientStatic_t		cls;

//=============================================================================

extern	refexport_t		re;		// interface to refresh .dll


//
// cvars
//
extern	cvar_t	*cl_nodelta;
extern	cvar_t	*cl_debugMove;
extern	cvar_t	*cl_noprint;
extern	cvar_t	*cl_timegraph;
extern	cvar_t	*cl_packetdup;
extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_timeNudge;
extern	cvar_t	*cl_showTimeDelta;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;
extern	cvar_t	*cl_run;
extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_sensitivity;
extern	cvar_t	*cl_freelook;

extern	cvar_t	*cl_mouseAccel;
extern	cvar_t	*cl_showMouseRate;

extern	cvar_t	*cl_allowAltEnter;

extern	cvar_t	*cl_inGameVideo;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;
extern	cvar_t	*m_filter;

extern	cvar_t	*cl_activeAction;

extern	cvar_t	*cl_consoleKeys;

//=================================================

//
// cl_main
//

void CL_Init (void);

void CL_AddReliableCommand( const char *cmd );

void CL_Disconnect_f (void);
void CL_Vid_Restart_f( void );
void CL_Snd_Restart_f (void);

qboolean CL_CheckPaused(void);

//
// cl_input
//
typedef struct {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_ClearState (void);
void CL_ReadPackets (void);
void CL_UpdateHotSwap(void);
bool CL_ExtendSelectTime(void);

void CL_WritePacket( void );
void IN_CenterView (void);

float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString( int keynum/*, qboolean bTranslate*/ ); //note: translate is only called for menu display not configs

//
// cl_parse.c
//
extern int cl_connectedToCheatServer;

void CL_SystemInfoChanged( void );
void CL_ParseServerMessage( msg_t *msg );

//====================================================================

//
// console
//
void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
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
void	SCR_DrawBigChar( int x, int y, int ch );
void	SCR_DrawSmallChar( int x, int y, int ch );

#ifdef JK2_MODE
void	SCR_PrecacheScreenshot();
#endif

//
// cl_cin.c
//
void CL_PlayCinematic_f( void );
void CL_PlayInGameCinematic_f(void);
qboolean CL_CheckPendingCinematic(void);
qboolean CL_IsRunningInGameCinematic(void);
qboolean CL_InGameCinematicOnStandBy(void);
void SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic( qboolean bAllowRefusal = qfalse );

int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */);
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
qboolean CL_InitCGameVM( void *gameLibrary );
void CL_InitCGame( void );
void CL_ShutdownCGame( void );
qboolean CL_GameCommand( void );
void CL_CGameRendering( stereoFrame_t stereo );
void CL_SetCGameTime( void );
void CL_FirstSnapshot( void );


//
// cl_ui.c
//
void CL_InitUI( void );
void CL_ShutdownUI( void );
void CL_GenericMenu_f(void);
void CL_DataPad_f(void);
void CL_EndScreenDissolve_f(void);
int Key_GetCatcher( void );
void Key_SetCatcher( int catcher );
