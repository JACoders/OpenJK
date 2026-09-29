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

// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"
#include "cl_cgameapi.h"
#include "cl_uiapi.h"
#ifndef _WIN32
#include <cmath>
#endif
unsigned	frame_msec;
int			old_com_frameTime;

float cl_mPitchOverride = 0.0f;
float cl_mYawOverride = 0.0f;
float cl_mSensitivityOverride = 0.0f;
qboolean cl_bUseFighterPitch = qfalse;
qboolean cl_crazyShipControls = qfalse;
cvar_t	*cl_splitScreen = NULL;
static cvar_t	*cl_splitScreenInvert[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenSensitivity[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenCmdHz[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenMoveSideAxis[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenMoveForwardAxis[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenLookYawAxis[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenLookPitchAxis[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenAttackButton[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenAltAttackButton[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenUseButton[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenJumpButton[5] = { NULL, NULL, NULL, NULL, NULL };
static cvar_t	*cl_splitScreenLocalCmds = NULL;
static const char *cl_splitScreenBindCommands[] = {
	"+forward", "+back", "+left", "+right", "+speed", "+moveleft", "+moveright", "+strafe", "+moveup", "+movedown",
	"+attack", "+altattack", "saberAttackCycle", "+use", "+button2", "invnext", "invprev", "+lookup", "+lookdown", "+mlook", "centerview",
	"weapon 1", "weapon 2", "weapon 3", "weapon 4", "weapon 5", "weapon 6", "weapon 7", "weapon 8", "weapon 13", "weapon 9", "weapon 10", "weapnext", "weapprev",
	"force_throw", "force_pull", "force_speed", "force_seeing", "+useforce", "forcenext", "forceprev",
	"force_protect", "force_absorb", "force_heal", "force_healother", "force_distract", "+force_grip", "+force_drain", "+force_lightning", "force_rage", "force_forcepowerother",
	"sensitivity", "ui_mousePitch", "movesideaxis", "moveforwardaxis", "lookyawaxis", "lookpitchaxis",
	"cl_run", "cg_autoswitch", "messagemode", "messagemode2", "voicechat", "automap_toggle", "+scores", "engage_duel", "cg_thirdperson !", "taunt", "bow", "meditate", "flourish", "gloat"
};
static cvar_t	*cl_splitScreenBindButton[5][ARRAY_LEN( cl_splitScreenBindCommands )];
static vec3_t cl_splitScreenViewangles[5];
static qboolean cl_splitScreenViewInitialized[5] = { qfalse, qfalse, qfalse, qfalse, qfalse };
static int cl_splitScreenNextCmdTime[5] = { 0, 0, 0, 0, 0 };
static int cl_splitScreenControllerAxis[5][MAX_JOYSTICK_AXIS];
static qboolean cl_splitScreenControllerButtons[5][16];
static qboolean cl_splitScreenControllerButtonPressed[5][16];
static qboolean cl_splitScreenControllerButtonReleased[5][16];
static qboolean cl_splitScreenWasDead[5];
static qboolean cl_splitScreenAttackBlockedUntilRelease[5];
static qboolean cl_splitScreenConsoleChordDown[5];

static int CL_SplitScreenPlayerForInputDevice( const char *deviceName );

#ifdef VEH_CONTROL_SCHEME_4
#define	OVERRIDE_MOUSE_SENSITIVITY 5.0f//20.0f = 180 degree turn in one mouse swipe across keyboard
#else// VEH_CONTROL_SCHEME_4
#define	OVERRIDE_MOUSE_SENSITIVITY 10.0f//20.0f = 180 degree turn in one mouse swipe across keyboard
#endif// VEH_CONTROL_SCHEME_4
/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/


kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed;
kbutton_t	in_up, in_down;

#define MAX_KBUTTONS 16

kbutton_t	in_buttons[MAX_KBUTTONS];


qboolean	in_mlooking;

void IN_Button11Down(void);
void IN_Button11Up(void);
void IN_Button10Down(void);
void IN_Button10Up(void);
void IN_Button6Down(void);
void IN_Button6Up(void);
void IN_UseGivenForce(void)
{
	char *c = Cmd_Argv(1);
	int forceNum =-1;
	int genCmdNum = 0;

	if(c) {
		forceNum = atoi(c);
	} else {
		return;
	}

	switch(forceNum) {
	case FP_DRAIN:
		IN_Button11Down();
		IN_Button11Up();
		break;
	case FP_PUSH:
		genCmdNum = GENCMD_FORCE_THROW;
		break;
	case FP_SPEED:
		genCmdNum = GENCMD_FORCE_SPEED;
		break;
	case FP_PULL:
		genCmdNum = GENCMD_FORCE_PULL;
		break;
	case FP_TELEPATHY:
		genCmdNum = GENCMD_FORCE_DISTRACT;
		break;
	case FP_GRIP:
		IN_Button6Down();
		IN_Button6Up();
		break;
	case FP_LIGHTNING:
		IN_Button10Down();
		IN_Button10Up();
		break;
	case FP_RAGE:
		genCmdNum = GENCMD_FORCE_RAGE;
		break;
	case FP_PROTECT:
		genCmdNum = GENCMD_FORCE_PROTECT;
		break;
	case FP_ABSORB:
		genCmdNum = GENCMD_FORCE_ABSORB;
		break;
	case FP_SEE:
		genCmdNum = GENCMD_FORCE_SEEING;
		break;
	case FP_HEAL:
		genCmdNum = GENCMD_FORCE_HEAL;
		break;
	case FP_TEAM_HEAL:
		genCmdNum = GENCMD_FORCE_HEALOTHER;
		break;
	case FP_TEAM_FORCE:
		genCmdNum = GENCMD_FORCE_FORCEPOWEROTHER;
		break;
	default:
		assert(0);
		break;
	}

	if(genCmdNum != 0) {
		cl.gcmdSendValue = qtrue;
		cl.gcmdValue = genCmdNum;
	}
}

void IN_MLookDown( void ) {
	in_mlooking = qtrue;
}

void IN_CenterView( void );
void IN_MLookUp( void ) {
	in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView ();
	}
}

void IN_GenCMD1( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERSWITCH;
}

void IN_GenCMD2( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ENGAGE_DUEL;
}

void IN_GenCMD3( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEAL;
}

void IN_GenCMD4( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SPEED;
}

void IN_GenCMD5( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PULL;
}

void IN_GenCMD6( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_DISTRACT;
}

void IN_GenCMD7( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_RAGE;
}

void IN_GenCMD8( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PROTECT;
}

void IN_GenCMD9( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_ABSORB;
}

void IN_GenCMD10( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEALOTHER;
}

void IN_GenCMD11( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_FORCEPOWEROTHER;
}

void IN_GenCMD12( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SEEING;
}

void IN_GenCMD13( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SEEKER;
}

void IN_GenCMD14( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_FIELD;
}

void IN_GenCMD15( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_BACTA;
}

void IN_GenCMD16( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_ELECTROBINOCULARS;
}

void IN_GenCMD17( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ZOOM;
}

void IN_GenCMD18( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SENTRY;
}

void IN_GenCMD19( void )
{
	if (Cvar_VariableIntegerValue("d_saberStanceDebug"))
	{
		Com_Printf("SABERSTANCEDEBUG: Gencmd on client set successfully.\n");
	}
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERATTACKCYCLE;
}

void IN_GenCMD20( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_THROW;
}

void IN_GenCMD21( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_JETPACK;
}

void IN_GenCMD22( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_BACTABIG;
}

void IN_GenCMD23( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_HEALTHDISP;
}

void IN_GenCMD24( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_AMMODISP;
}

void IN_GenCMD25( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_EWEB;
}

void IN_GenCMD26( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_CLOAK;
}

void IN_GenCMD27( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_TAUNT;
}

void IN_GenCMD28( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_BOW;
}

void IN_GenCMD29( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_MEDITATE;
}

void IN_GenCMD30( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FLOURISH;
}

void IN_GenCMD31( void )
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_GLOAT;
}


//toggle automap view mode
static bool g_clAutoMapMode = false;
void IN_AutoMapButton(void)
{
	g_clAutoMapMode = !g_clAutoMapMode;
}

//toggle between automap, radar, nothing
extern cvar_t *r_autoMap;
void IN_AutoMapToggle(void)
{
	Cvar_User_SetValue("cg_drawRadar", !Cvar_VariableValue("cg_drawRadar"));
	/*
	if (r_autoMap && r_autoMap->integer)
	{ //automap off, radar on
		Cvar_Set("r_autoMap", "0");
		Cvar_Set("cg_drawRadar", "1");
	}
	else if (Cvar_VariableIntegerValue("cg_drawRadar"))
	{ //radar off, automap should be off too
		Cvar_Set("cg_drawRadar", "0");
	}
	else
	{ //turn automap on
		Cvar_Set("r_autoMap", "1");
	}
	*/
}

void IN_VoiceChatButton(void)
{
	if (!cls.uiStarted)
	{ //ui not loaded so this command is useless
		return;
	}
	UIVM_SetActiveMenu( UIMENU_VOICECHAT );
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	char	*c;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}

	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}

	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( kbutton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}

	return val;
}

#define		AUTOMAP_KEY_FORWARD			1
#define		AUTOMAP_KEY_BACK			2
#define		AUTOMAP_KEY_YAWLEFT			3
#define		AUTOMAP_KEY_YAWRIGHT		4
#define		AUTOMAP_KEY_PITCHUP			5
#define		AUTOMAP_KEY_PITCHDOWN		6
#define		AUTOMAP_KEY_DEFAULTVIEW		7
static autoMapInput_t			g_clAutoMapInput;
//intercept certain keys during automap mode
static void CL_AutoMapKey(int autoMapKey, qboolean up)
{
	autoMapInput_t *data = (autoMapInput_t *)cl.mSharedMemory;

	switch (autoMapKey)
	{
	case AUTOMAP_KEY_FORWARD:
        if (up)
		{
			g_clAutoMapInput.up = 0.0f;
		}
		else
		{
			g_clAutoMapInput.up = 16.0f;
		}
		break;
	case AUTOMAP_KEY_BACK:
        if (up)
		{
			g_clAutoMapInput.down = 0.0f;
		}
		else
		{
			g_clAutoMapInput.down = 16.0f;
		}
		break;
	case AUTOMAP_KEY_YAWLEFT:
		if (up)
		{
			g_clAutoMapInput.yaw = 0.0f;
		}
		else
		{
			g_clAutoMapInput.yaw = -4.0f;
		}
		break;
	case AUTOMAP_KEY_YAWRIGHT:
		if (up)
		{
			g_clAutoMapInput.yaw = 0.0f;
		}
		else
		{
			g_clAutoMapInput.yaw = 4.0f;
		}
		break;
	case AUTOMAP_KEY_PITCHUP:
		if (up)
		{
			g_clAutoMapInput.pitch = 0.0f;
		}
		else
		{
			g_clAutoMapInput.pitch = -4.0f;
		}
		break;
	case AUTOMAP_KEY_PITCHDOWN:
		if (up)
		{
			g_clAutoMapInput.pitch = 0.0f;
		}
		else
		{
			g_clAutoMapInput.pitch = 4.0f;
		}
		break;
	case AUTOMAP_KEY_DEFAULTVIEW:
		memset(&g_clAutoMapInput, 0, sizeof(autoMapInput_t));
		g_clAutoMapInput.goToDefaults = qtrue;
		break;
	default:
		break;
	}

	memcpy(data, &g_clAutoMapInput, sizeof(autoMapInput_t));

	if (cls.cgameStarted)
	{
		CGVM_AutomapInput();
	}

	g_clAutoMapInput.goToDefaults = qfalse;
}


void IN_UpDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHUP, qfalse);
	}
	else
	{
		IN_KeyDown(&in_up);
	}
}
void IN_UpUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHUP, qtrue);
	}
	else
	{
		IN_KeyUp(&in_up);
	}
}
void IN_DownDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHDOWN, qfalse);
	}
	else
	{
		IN_KeyDown(&in_down);
	}
}
void IN_DownUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHDOWN, qtrue);
	}
	else
	{
		IN_KeyUp(&in_down);
	}
}
void IN_LeftDown(void) {IN_KeyDown(&in_left);}
void IN_LeftUp(void) {IN_KeyUp(&in_left);}
void IN_RightDown(void) {IN_KeyDown(&in_right);}
void IN_RightUp(void) {IN_KeyUp(&in_right);}
void IN_ForwardDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_FORWARD, qfalse);
	}
	else
	{
		IN_KeyDown(&in_forward);
	}
}
void IN_ForwardUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_FORWARD, qtrue);
	}
	else
	{
		IN_KeyUp(&in_forward);
	}
}
void IN_BackDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_BACK, qfalse);
	}
	else
	{
		IN_KeyDown(&in_back);
	}
}
void IN_BackUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_BACK, qtrue);
	}
	else
	{
		IN_KeyUp(&in_back);
	}
}
void IN_LookupDown(void) {IN_KeyDown(&in_lookup);}
void IN_LookupUp(void) {IN_KeyUp(&in_lookup);}
void IN_LookdownDown(void) {IN_KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {IN_KeyUp(&in_lookdown);}
void IN_MoveleftDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWLEFT, qfalse);
	}
	else
	{
		IN_KeyDown(&in_moveleft);
	}
}
void IN_MoveleftUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWLEFT, qtrue);
	}
	else
	{
		IN_KeyUp(&in_moveleft);
	}
}
void IN_MoverightDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWRIGHT, qfalse);
	}
	else
	{
		IN_KeyDown(&in_moveright);
	}
}
void IN_MoverightUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWRIGHT, qtrue);
	}
	else
	{
		IN_KeyUp(&in_moveright);
	}
}

void IN_SpeedDown(void) {IN_KeyDown(&in_speed);}
void IN_SpeedUp(void) {IN_KeyUp(&in_speed);}
void IN_StrafeDown(void) {IN_KeyDown(&in_strafe);}
void IN_StrafeUp(void) {IN_KeyUp(&in_strafe);}

void IN_Button0Down(void) {IN_KeyDown(&in_buttons[0]);}
void IN_Button0Up(void) {IN_KeyUp(&in_buttons[0]);}
void IN_Button1Down(void) {IN_KeyDown(&in_buttons[1]);}
void IN_Button1Up(void) {IN_KeyUp(&in_buttons[1]);}
void IN_Button2Down(void) {IN_KeyDown(&in_buttons[2]);}
void IN_Button2Up(void) {IN_KeyUp(&in_buttons[2]);}
void IN_Button3Down(void) {IN_KeyDown(&in_buttons[3]);}
void IN_Button3Up(void) {IN_KeyUp(&in_buttons[3]);}
void IN_Button4Down(void) {IN_KeyDown(&in_buttons[4]);}
void IN_Button4Up(void) {IN_KeyUp(&in_buttons[4]);}
void IN_Button5Down(void) //use key
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_DEFAULTVIEW, qfalse);
	}
	else
	{
		IN_KeyDown(&in_buttons[5]);
	}
}
void IN_Button5Up(void) {IN_KeyUp(&in_buttons[5]);}
void IN_Button6Down(void) {IN_KeyDown(&in_buttons[6]);}
void IN_Button6Up(void) {IN_KeyUp(&in_buttons[6]);}
void IN_Button7Down(void) {IN_KeyDown(&in_buttons[7]);}
void IN_Button7Up(void){IN_KeyUp(&in_buttons[7]);}
void IN_Button8Down(void) {IN_KeyDown(&in_buttons[8]);}
void IN_Button8Up(void) {IN_KeyUp(&in_buttons[8]);}
void IN_Button9Down(void) {IN_KeyDown(&in_buttons[9]);}
void IN_Button9Up(void) {IN_KeyUp(&in_buttons[9]);}
void IN_Button10Down(void) {IN_KeyDown(&in_buttons[10]);}
void IN_Button10Up(void) {IN_KeyUp(&in_buttons[10]);}
void IN_Button11Down(void) {IN_KeyDown(&in_buttons[11]);}
void IN_Button11Up(void) {IN_KeyUp(&in_buttons[11]);}
void IN_Button12Down(void) {IN_KeyDown(&in_buttons[12]);}
void IN_Button12Up(void) {IN_KeyUp(&in_buttons[12]);}
void IN_Button13Down(void) {IN_KeyDown(&in_buttons[13]);}
void IN_Button13Up(void) {IN_KeyUp(&in_buttons[13]);}
void IN_Button14Down(void) {IN_KeyDown(&in_buttons[14]);}
void IN_Button14Up(void) {IN_KeyUp(&in_buttons[14]);}
void IN_Button15Down(void) {IN_KeyDown(&in_buttons[15]);}
void IN_Button15Up(void) {IN_KeyUp(&in_buttons[15]);}

void IN_CenterView (void) {
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}

//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void ) {
	float	speed;

	if ( in_speed.active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		if ( cl_mYawOverride )
		{
			if ( cl_mSensitivityOverride )
			{
				cl.viewangles[YAW] -= cl_mYawOverride*cl_mSensitivityOverride*speed*cl_yawspeed->value*CL_KeyState (&in_right);
				cl.viewangles[YAW] += cl_mYawOverride*cl_mSensitivityOverride*speed*cl_yawspeed->value*CL_KeyState (&in_left);
			}
			else
			{
				cl.viewangles[YAW] -= cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&in_right);
				cl.viewangles[YAW] += cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&in_left);
			}
		}
		else
		{
			cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
			cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
		}
	}

	if ( cl_mPitchOverride )
	{
		if ( cl_mSensitivityOverride )
		{
			cl.viewangles[PITCH] -= cl_mPitchOverride*cl_mSensitivityOverride*speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
			cl.viewangles[PITCH] += cl_mPitchOverride*cl_mSensitivityOverride*speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
		}
		else
		{
			cl.viewangles[PITCH] -= cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
			cl.viewangles[PITCH] += cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
		}
	}
	else
	{
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
	}
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 46;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( in_strafe.active ) {
		side += movespeed * CL_KeyState (&in_right);
		side -= movespeed * CL_KeyState (&in_left);
	}

	side += movespeed * CL_KeyState (&in_moveright);
	side -= movespeed * CL_KeyState (&in_moveleft);


	up += movespeed * CL_KeyState (&in_up);
	up -= movespeed * CL_KeyState (&in_down);

	forward += movespeed * CL_KeyState (&in_forward);
	forward -= movespeed * CL_KeyState (&in_back);

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if (cls.cursorActive) {
		CL_UpdateCursorPosition( dx, dy );
	} else if (g_clAutoMapMode && cls.cgameStarted)
	{ //automap input
		autoMapInput_t *data = (autoMapInput_t *)cl.mSharedMemory;

		g_clAutoMapInput.yaw = dx;
		g_clAutoMapInput.pitch = dy;
		memcpy(data, &g_clAutoMapInput, sizeof(autoMapInput_t));
		CGVM_AutomapInput();

		g_clAutoMapInput.yaw = 0.0f;
		g_clAutoMapInput.pitch = 0.0f;
	}
	else if ( Key_GetCatcher( ) & KEYCATCH_UI ) {
		UIVM_MouseEvent( dx, dy );
	} else if ( Key_GetCatcher( ) & KEYCATCH_CGAME ) {
		CGVM_MouseEvent( dx, dy );
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( int axis, int value, int time ) {
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.joystickAxis[axis] = value;
}

void CL_SplitScreenSetControllerAxis( int player, int axis, int value ) {
	if ( player < 1 || player > 4 || axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		return;
	}
	if ( value < -127 || value > 127 ) {
		value = (int)Com_Clamp( -127.0f, 127.0f, value / 256.0f );
	}
	cl_splitScreenControllerAxis[player][axis] = value;
}

void CL_SplitScreenSetControllerButton( int player, int button, qboolean pressed ) {
	if ( player < 1 || player > 4 || button < 0 || button >= (int)ARRAY_LEN( cl_splitScreenControllerButtons[player] ) ) {
		return;
	}
	if ( pressed && !cl_splitScreenControllerButtons[player][button] ) {
		cl_splitScreenControllerButtonPressed[player][button] = qtrue;
	}
	if ( !pressed && cl_splitScreenControllerButtons[player][button] ) {
		cl_splitScreenControllerButtonReleased[player][button] = qtrue;
	}
	cl_splitScreenControllerButtons[player][button] = pressed;

	if ( cl_splitScreenControllerButtons[player][4] && cl_splitScreenControllerButtons[player][6] ) {
		if ( !cl_splitScreenConsoleChordDown[player] ) {
			Con_ToggleConsoleForPlayer( player );
			cl_splitScreenConsoleChordDown[player] = qtrue;
		}
	} else {
		cl_splitScreenConsoleChordDown[player] = qfalse;
	}
}

static void CL_SplitInputAxis_f( void )
{
	int player;
	int axis;
	int value;

	if ( Cmd_Argc() < 3 || Cmd_Argc() > 4 ) {
		Com_Printf( "usage: splitinput_axis <player 1-4> <axis> [value -127..127]\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	axis = atoi( Cmd_Argv( 2 ) );
	value = Cmd_Argc() == 4 ? atoi( Cmd_Argv( 3 ) ) : 0;
	value = (int)Com_Clamp( -127.0f, 127.0f, value );
	CL_SplitScreenSetControllerAxis( player, axis, value );
}

static void CL_SplitInputButton_f( void )
{
	int player;
	int button;
	int pressed;

	if ( Cmd_Argc() < 3 || Cmd_Argc() > 4 ) {
		Com_Printf( "usage: splitinput_button <player 2-4> <button> [0|1]\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	button = atoi( Cmd_Argv( 2 ) );
	pressed = Cmd_Argc() == 4 ? atoi( Cmd_Argv( 3 ) ) : 0;
	CL_SplitScreenSetControllerButton( player, button, pressed ? qtrue : qfalse );
}

static void CL_SplitInputDeviceAxis_f( void )
{
	int player;
	int axis;
	int value;
	const char *device;

	if ( Cmd_Argc() < 3 || Cmd_Argc() > 4 ) {
		Com_Printf( "usage: splitinput_device_axis <controller1|controller2|controller3> <axis> [value -127..127]\n" );
		return;
	}

	device = Cmd_Argv( 1 );
	player = CL_SplitScreenPlayerForInputDevice( device );
	axis = atoi( Cmd_Argv( 2 ) );
	value = Cmd_Argc() == 4 ? atoi( Cmd_Argv( 3 ) ) : 0;
	value = (int)Com_Clamp( -127.0f, 127.0f, value );

	if ( player < 1 || player > 4 ) {
		Com_Printf( "SplitInputSim: axis device=%s has no split-screen controller owner\n", device );
		return;
	}

	Com_Printf( "SplitInputSim: axis device=%s player=%i axis=%i value=%i\n", device, player, axis, value );
	CL_SplitScreenSetControllerAxis( player, axis, value );
}

static void CL_SplitInputDeviceButton_f( void )
{
	int player;
	int button;
	int pressed;
	const char *device;

	if ( Cmd_Argc() < 3 || Cmd_Argc() > 4 ) {
		Com_Printf( "usage: splitinput_device_button <controller1|controller2|controller3> <button> [0|1]\n" );
		return;
	}

	device = Cmd_Argv( 1 );
	player = CL_SplitScreenPlayerForInputDevice( device );
	button = atoi( Cmd_Argv( 2 ) );
	pressed = Cmd_Argc() == 4 ? atoi( Cmd_Argv( 3 ) ) : 0;

	if ( player < 1 || player > 4 ) {
		Com_Printf( "SplitInputSim: button device=%s has no split-screen controller owner\n", device );
		return;
	}

	Com_Printf( "SplitInputSim: button device=%s player=%i button=%i pressed=%i\n", device, player, button, pressed ? 1 : 0 );
	CL_SplitScreenSetControllerButton( player, button, pressed ? qtrue : qfalse );
}

static void CL_SplitInputRouteStatus_f( void )
{
	int player;
	char inputName[32];
	char modelName[MAX_QPATH];

	Com_Printf( "SplitInputRoute: cl_splitScreen=%i players=%i\n",
		cl_splitScreen ? cl_splitScreen->integer : 0,
		Cvar_VariableIntegerValue( "ui_splitScreenPlayerCount" ) );
	for ( player = 1; player <= 4; player++ ) {
		Cvar_VariableStringBuffer( va( "ui_splitScreenP%iInput", player ), inputName, sizeof( inputName ) );
		Cvar_VariableStringBuffer( va( "ui_splitScreenP%iModel", player ), modelName, sizeof( modelName ) );
		Com_Printf( "SplitInputRoute: player=%i device=%s model=%s\n", player, inputName[0] ? inputName : "<unset>", modelName[0] ? modelName : "<unset>" );
	}
	Com_Printf( "SplitInputRoute: keyboardOwner=%i controller1Owner=%i controller2Owner=%i controller3Owner=%i\n",
		CL_SplitScreenPlayerForInputDevice( "keyboard" ),
		CL_SplitScreenPlayerForInputDevice( "controller1" ),
		CL_SplitScreenPlayerForInputDevice( "controller2" ),
		CL_SplitScreenPlayerForInputDevice( "controller3" ) );
}

static void CL_SplitInputAssertModel_f( void )
{
	int player;
	char modelName[MAX_QPATH];
	const char *expected;

	if ( Cmd_Argc() < 3 ) {
		Com_Printf( "usage: splitinput_assert_model <player 1-4> <expected model>\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	expected = Cmd_ArgsFrom( 2 );
	if ( player < 1 || player > 4 ) {
		Com_Printf( "SplitInputAssertModel: FAIL invalid player=%i expected=%s\n", player, expected );
		return;
	}

	Cvar_VariableStringBuffer( va( "ui_splitScreenP%iModel", player ), modelName, sizeof( modelName ) );
	Com_Printf( "SplitInputAssertModel: %s player=%i expected=%s actual=%s\n",
		!Q_stricmp( modelName, expected ) ? "PASS" : "FAIL",
		player,
		expected,
		modelName );
}

usercmd_t CL_CreateCmd( void );
static void CL_SplitScreenCreateCmd( int player, usercmd_t *cmd );
void CL_MouseMove( usercmd_t *cmd );
void CL_CmdButtons( usercmd_t *cmd );
void CL_FinishMove( usercmd_t *cmd );

static qboolean CL_SplitInputExpectedMatches( int expected, int actual )
{
	if ( expected == -999 ) {
		return qtrue;
	}
	if ( expected == -998 ) {
		return (qboolean)( actual != 0 );
	}
	return (qboolean)( expected == actual );
}

static void CL_SplitInputAssertCmd_f( void )
{
	int player;
	int expectedForward;
	int expectedRight;
	int expectedUp;
	int expectedButtons;
	int expectedWeapon = -999;
	int expectedForce = -999;
	int expectedGeneric = -999;
	clientActive_t savedCl;
	usercmd_t cmd;
	qboolean pass = qtrue;

	if ( Cmd_Argc() < 6 ) {
		Com_Printf( "usage: splitinput_assert_cmd <player 1-4> <forward|-999|-998> <right|-999|-998> <up|-999|-998> <buttons|-999|-998> [weapon|-999|-998] [force|-999|-998] [generic|-999|-998]\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	expectedForward = atoi( Cmd_Argv( 2 ) );
	expectedRight = atoi( Cmd_Argv( 3 ) );
	expectedUp = atoi( Cmd_Argv( 4 ) );
	expectedButtons = atoi( Cmd_Argv( 5 ) );
	if ( Cmd_Argc() >= 7 ) {
		expectedWeapon = atoi( Cmd_Argv( 6 ) );
	}
	if ( Cmd_Argc() >= 8 ) {
		expectedForce = atoi( Cmd_Argv( 7 ) );
	}
	if ( Cmd_Argc() >= 9 ) {
		expectedGeneric = atoi( Cmd_Argv( 8 ) );
	}

	if ( player < 1 || player > 4 ) {
		Com_Printf( "SplitInputAssertCmd: FAIL invalid player=%i\n", player );
		return;
	}

	if ( player == 1 ) {
		cmd = CL_CreateCmd();
	} else {
		savedCl = cl;
		cl = cl_splitClients[player].active;
		CL_SplitScreenCreateCmd( player, &cmd );
		cl_splitClients[player].active = cl;
		cl = savedCl;
	}

	if ( !CL_SplitInputExpectedMatches( expectedForward, cmd.forwardmove ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedRight, cmd.rightmove ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedUp, cmd.upmove ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedButtons, cmd.buttons ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedWeapon, cmd.weapon ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedForce, cmd.forcesel ) ) {
		pass = qfalse;
	}
	if ( !CL_SplitInputExpectedMatches( expectedGeneric, cmd.generic_cmd ) ) {
		pass = qfalse;
	}

	Com_Printf( "SplitInputAssertCmd: %s player=%i expectedForward=%i actualForward=%i expectedRight=%i actualRight=%i expectedUp=%i actualUp=%i expectedButtons=%i actualButtons=%i expectedWeapon=%i actualWeapon=%i expectedForce=%i actualForce=%i expectedGeneric=%i actualGeneric=%i cmdAngles=(%.1f %.1f %.1f)\n",
		pass ? "PASS" : "FAIL",
		player,
		expectedForward,
		cmd.forwardmove,
		expectedRight,
		cmd.rightmove,
		expectedUp,
		cmd.upmove,
		expectedButtons,
		cmd.buttons,
		expectedWeapon,
		cmd.weapon,
		expectedForce,
		cmd.forcesel,
		expectedGeneric,
		cmd.generic_cmd,
		SHORT2ANGLE( cmd.angles[PITCH] ),
		SHORT2ANGLE( cmd.angles[YAW] ),
		SHORT2ANGLE( cmd.angles[ROLL] ) );
}

void CL_SplitScreenClearControllerState( int player )
{
	int axis;
	int button;

	if ( player < 1 || player > 4 ) {
		return;
	}

	for ( axis = 0; axis < MAX_JOYSTICK_AXIS; axis++ ) {
		cl_splitScreenControllerAxis[player][axis] = 0;
	}
	for ( button = 0; button < (int)ARRAY_LEN( cl_splitScreenControllerButtons[player] ); button++ ) {
		cl_splitScreenControllerButtons[player][button] = qfalse;
	}
	Com_Memset( cl_splitScreenControllerButtonPressed[player], 0,
		sizeof( cl_splitScreenControllerButtonPressed[player] ) );
	Com_Memset( cl_splitScreenControllerButtonReleased[player], 0,
		sizeof( cl_splitScreenControllerButtonReleased[player] ) );
	cl_splitScreenConsoleChordDown[player] = qfalse;
	cl_splitScreenWasDead[player] = qfalse;
	cl_splitScreenAttackBlockedUntilRelease[player] = qfalse;
}

static void CL_SplitInputClear_f( void )
{
	int player;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: splitinput_clear <player 1-4>\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	if ( player < 1 || player > 4 ) {
		Com_Printf( "splitinput_clear: invalid player %i\n", player );
		return;
	}
	CL_SplitScreenClearControllerState( player );
}

/*
=================
CL_JoystickMove
=================
*/
extern cvar_t *in_joystick;
void CL_JoystickMove( usercmd_t *cmd ) {
	float	anglespeed;

	if ( !in_joystick->integer )
	{
		return;
	}

	if ( !(in_speed.active ^ cl_run->integer) ) {
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( in_speed.active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		if ( cl_mYawOverride )
		{
			if ( cl_mSensitivityOverride )
			{
				cl.viewangles[YAW] += cl_mYawOverride * cl_mSensitivityOverride * cl.joystickAxis[AXIS_SIDE]/2.0f;
			}
			else
			{
				cl.viewangles[YAW] += cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * cl.joystickAxis[AXIS_SIDE]/2.0f;
			}
		}
		else
		{
			cl.viewangles[YAW] += anglespeed * (cl_yawspeed->value / 100.0f) * cl.joystickAxis[AXIS_SIDE];
		}
	}
	else
	{
		cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
	}

	if ( in_mlooking || cl_freelook->integer ) {
		if ( cl_mPitchOverride )
		{
			if ( cl_mSensitivityOverride )
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * cl_mSensitivityOverride * cl.joystickAxis[AXIS_FORWARD]/2.0f;
			}
			else
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * cl.joystickAxis[AXIS_FORWARD]/2.0f;
			}
		}
		else
		{
			cl.viewangles[PITCH] += anglespeed * (cl_pitchspeed->value / 100.0f) * cl.joystickAxis[AXIS_FORWARD];
		}
	} else
	{
		cmd->forwardmove = ClampChar( cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD] );
	}

	cmd->upmove = ClampChar( cmd->upmove + cl.joystickAxis[AXIS_UP] );
}

static int CL_SplitScreenAxisValue( int player, const cvar_t *axisCvar ) {
	const int axis = axisCvar->integer;

	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		return 0;
	}

	return cl_splitScreenControllerAxis[player][axis];
}

static qboolean CL_SplitScreenButtonDown( int player, int offset ) {
	if ( offset >= 0 && offset < (int)ARRAY_LEN( cl_splitScreenControllerButtons[player] ) ) {
		return cl_splitScreenControllerButtons[player][offset];
	}
	return qfalse;
}

static qboolean CL_SplitScreenButtonPressed( int player, int offset ) {
	if ( offset >= 0 && offset < (int)ARRAY_LEN( cl_splitScreenControllerButtonPressed[player] ) ) {
		return cl_splitScreenControllerButtonPressed[player][offset];
	}
	return qfalse;
}

static qboolean CL_SplitScreenCommandDown( int player, const char *command ) {
	int i;

	for ( i = 0; i < (int)ARRAY_LEN( cl_splitScreenBindCommands ); i++ ) {
		if ( Q_stricmp( cl_splitScreenBindCommands[i], command ) ) {
			continue;
		}
		return CL_SplitScreenButtonDown( player, cl_splitScreenBindButton[player][i]->integer );
	}

	return qfalse;
}

static qboolean CL_SplitScreenCommandPressed( int player, const char *command ) {
	int i;

	for ( i = 0; i < (int)ARRAY_LEN( cl_splitScreenBindCommands ); i++ ) {
		int button;
		if ( Q_stricmp( cl_splitScreenBindCommands[i], command ) ) {
			continue;
		}
		button = cl_splitScreenBindButton[player][i]->integer;
		return CL_SplitScreenButtonPressed( player, button );
	}

	return qfalse;
}

static qboolean CL_SplitScreenCommandReleased( int player, const char *command ) {
	int i;
	for ( i = 0; i < (int)ARRAY_LEN( cl_splitScreenBindCommands ); i++ ) {
		int button;
		if ( Q_stricmp( cl_splitScreenBindCommands[i], command ) ) {
			continue;
		}
		button = cl_splitScreenBindButton[player][i]->integer;
		return (qboolean)( button >= 0 &&
			button < (int)ARRAY_LEN( cl_splitScreenControllerButtonReleased[player] ) &&
			cl_splitScreenControllerButtonReleased[player][button] );
	}
	return qfalse;
}

static void CL_SplitScreenRememberControllerButtons( int player ) {
	Com_Memset( cl_splitScreenControllerButtonPressed[player], 0,
		sizeof( cl_splitScreenControllerButtonPressed[player] ) );
	Com_Memset( cl_splitScreenControllerButtonReleased[player], 0,
		sizeof( cl_splitScreenControllerButtonReleased[player] ) );
}

static int CL_SplitScreenDefaultBindForCommand( const char *command, int bindIndex ) {
	(void)bindIndex;
	if ( !Q_stricmp( command, "+attack" ) ) return 0;
	if ( !Q_stricmp( command, "+altattack" ) ) return 1;
	if ( !Q_stricmp( command, "+use" ) ) return 2;
	if ( !Q_stricmp( command, "+moveup" ) ) return 3;
	if ( !Q_stricmp( command, "+scores" ) ) return 4;
	if ( !Q_stricmp( command, "+movedown" ) ) return 7;
	if ( !Q_stricmp( command, "saberAttackCycle" ) ) return 8;
	if ( !Q_stricmp( command, "+button2" ) ) return 9;
	if ( !Q_stricmp( command, "+useforce" ) ) return 10;
	if ( !Q_stricmp( command, "weapnext" ) ) return 11;
	if ( !Q_stricmp( command, "weapprev" ) ) return 12;
	if ( !Q_stricmp( command, "forceprev" ) ) return 13;
	if ( !Q_stricmp( command, "forcenext" ) ) return 14;
	return -1;
}

static void CL_SplitScreenResetControllerBindings( int player ) {
	int bindIndex;

	if ( player < 1 || player > 4 ) {
		return;
	}
	for ( bindIndex = 0; bindIndex < (int)ARRAY_LEN( cl_splitScreenBindCommands ); bindIndex++ ) {
		Cvar_Set( cl_splitScreenBindButton[player][bindIndex]->name,
			va( "%i", CL_SplitScreenDefaultBindForCommand( cl_splitScreenBindCommands[bindIndex], bindIndex ) ) );
	}
}

static void CL_SplitScreenResetController_f( void ) {
	int player = atoi( Cmd_Argv( 1 ) );

	if ( Cmd_Argc() != 2 || player < 1 || player > 4 ) {
		Com_Printf( "usage: splitscreen_reset_controller <player 1-4>\n" );
		return;
	}
	CL_SplitScreenResetControllerBindings( player );
	Com_Printf( "Player %i controller bindings restored to defaults\n", player );
}

static int CL_SplitScreenBindIndexForCommand( const char *command ) {
	int i;

	if ( !command || !command[0] ) {
		return -1;
	}

	for ( i = 0; i < (int)ARRAY_LEN( cl_splitScreenBindCommands ); i++ ) {
		if ( !Q_stricmp( command, cl_splitScreenBindCommands[i] ) ) {
			return i;
		}
	}

	return -1;
}

static qboolean CL_SplitScreenCommandIsButtonBindable( const char *command ) {
	static const char *nonButtonCommands[] = {
		"sensitivity", "ui_mousePitch", "movesideaxis", "moveforwardaxis",
		"lookyawaxis", "lookpitchaxis", "cl_run", "cg_autoswitch",
		"+mlook", "voicechat"
	};
	int i;

	for ( i = 0; i < (int)ARRAY_LEN( nonButtonCommands ); i++ ) {
		if ( !Q_stricmp( command, nonButtonCommands[i] ) ) {
			return qfalse;
		}
	}
	return qtrue;
}

static void CL_SplitScreenBindController_f( void ) {
	int player;
	int button;
	int bindIndex;
	int i;
	const char *command;

	if ( Cmd_Argc() < 4 ) {
		Com_Printf( "usage: splitscreen_bind_controller <player 1-4> <button 0-15|-1> <command>\n" );
		return;
	}

	player = atoi( Cmd_Argv( 1 ) );
	button = atoi( Cmd_Argv( 2 ) );
	command = Cmd_ArgsFrom( 3 );

	if ( player < 1 || player > 4 || button < -1 || button > 15 ) {
		Com_Printf( "splitscreen_bind_controller: invalid player/button\n" );
		return;
	}

	bindIndex = CL_SplitScreenBindIndexForCommand( command );
	if ( bindIndex < 0 ) {
		Com_Printf( "splitscreen_bind_controller: unknown command '%s'\n", command );
		return;
	}
	if ( !CL_SplitScreenCommandIsButtonBindable( command ) ) {
		Com_Printf( "splitscreen_bind_controller: '%s' is a setting or unsupported global action, not a button binding\n", command );
		return;
	}

	if ( button >= 0 ) {
		for ( i = 0; i < (int)ARRAY_LEN( cl_splitScreenBindCommands ); i++ ) {
			if ( i != bindIndex && cl_splitScreenBindButton[player][i] && cl_splitScreenBindButton[player][i]->integer == button ) {
				Cvar_Set( cl_splitScreenBindButton[player][i]->name, "-1" );
			}
		}
	}

	Cvar_Set( cl_splitScreenBindButton[player][bindIndex]->name, va( "%i", button ) );
	Com_Printf( "Player %i controller bind: %s = %s\n", player, command, button >= 0 ? va( "JOY%i", button ) : "unbound" );
}

static void CL_SplitScreenApplyButtonBindings( int player, usercmd_t *cmd ) {
	int weapon;

	if ( CL_SplitScreenCommandPressed( player, "+scores" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "+scores" );
	}
	if ( CL_SplitScreenCommandReleased( player, "+scores" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "-scores" );
	}
	if ( CL_SplitScreenCommandPressed( player, "messagemode" ) ) {
		Con_MessageModeForPlayer( player, qfalse );
	}
	if ( CL_SplitScreenCommandPressed( player, "messagemode2" ) ) {
		Con_MessageModeForPlayer( player, qtrue );
	}
	if ( CL_SplitScreenCommandPressed( player, "weapnext" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "weapnext" );
	}
	if ( CL_SplitScreenCommandPressed( player, "weapprev" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "weapprev" );
	}
	if ( CL_SplitScreenCommandPressed( player, "invnext" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "invnext" );
	}
	if ( CL_SplitScreenCommandPressed( player, "invprev" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "invprev" );
	}
	if ( CL_SplitScreenCommandPressed( player, "forcenext" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "forcenext" );
	}
	if ( CL_SplitScreenCommandPressed( player, "forceprev" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "forceprev" );
	}
	if ( CL_SplitScreenCommandPressed( player, "automap_toggle" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "splitscreen_automap_toggle" );
	}
	if ( CL_SplitScreenCommandPressed( player, "cg_thirdperson !" ) ) {
		CL_CGameConsoleCommandForPlayer( player, "splitscreen_thirdperson_toggle" );
	}
	if ( CL_SplitScreenCommandPressed( player, "centerview" ) ) {
		const int pitchDelta = player <= 1
			? cl.snap.ps.delta_angles[PITCH]
			: cl_splitClients[player].active.snap.ps.delta_angles[PITCH];
		cl_splitScreenViewangles[player][PITCH] = -SHORT2ANGLE( pitchDelta );
		cmd->angles[PITCH] = ANGLE2SHORT( cl_splitScreenViewangles[player][PITCH] );
	}

	if ( CL_SplitScreenCommandPressed( player, "saberAttackCycle" ) ) {
		cmd->generic_cmd = GENCMD_SABERATTACKCYCLE;
	} else if ( CL_SplitScreenCommandPressed( player, "engage_duel" ) ) {
		cmd->generic_cmd = GENCMD_ENGAGE_DUEL;
	} else if ( CL_SplitScreenCommandPressed( player, "force_throw" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_THROW;
	} else if ( CL_SplitScreenCommandPressed( player, "force_pull" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_PULL;
	} else if ( CL_SplitScreenCommandPressed( player, "force_speed" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_SPEED;
	} else if ( CL_SplitScreenCommandPressed( player, "force_seeing" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_SEEING;
	} else if ( CL_SplitScreenCommandPressed( player, "force_protect" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_PROTECT;
	} else if ( CL_SplitScreenCommandPressed( player, "force_absorb" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_ABSORB;
	} else if ( CL_SplitScreenCommandPressed( player, "force_heal" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_HEAL;
	} else if ( CL_SplitScreenCommandPressed( player, "force_healother" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_HEALOTHER;
	} else if ( CL_SplitScreenCommandPressed( player, "force_distract" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_DISTRACT;
	} else if ( CL_SplitScreenCommandPressed( player, "force_rage" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_RAGE;
	} else if ( CL_SplitScreenCommandPressed( player, "force_forcepowerother" ) ) {
		cmd->generic_cmd = GENCMD_FORCE_FORCEPOWEROTHER;
	} else if ( CL_SplitScreenCommandPressed( player, "taunt" ) ) {
		cmd->generic_cmd = GENCMD_TAUNT;
	} else if ( CL_SplitScreenCommandPressed( player, "bow" ) ) {
		cmd->generic_cmd = GENCMD_BOW;
	} else if ( CL_SplitScreenCommandPressed( player, "meditate" ) ) {
		cmd->generic_cmd = GENCMD_MEDITATE;
	} else if ( CL_SplitScreenCommandPressed( player, "flourish" ) ) {
		cmd->generic_cmd = GENCMD_FLOURISH;
	} else if ( CL_SplitScreenCommandPressed( player, "gloat" ) ) {
		cmd->generic_cmd = GENCMD_GLOAT;
	}
	if ( cmd->generic_cmd ) {
		Cvar_Set( va( "cl_splitScreenP%iLastGenericCmd", player ), va( "%i", cmd->generic_cmd ) );
	}

	if ( CL_SplitScreenCommandDown( player, "+forward" ) ) {
		cmd->forwardmove = ClampChar( cmd->forwardmove + 127 );
	}
	if ( CL_SplitScreenCommandDown( player, "+back" ) ) {
		cmd->forwardmove = ClampChar( cmd->forwardmove - 127 );
	}
	if ( CL_SplitScreenCommandDown( player, "+moveleft" ) ) {
		cmd->rightmove = ClampChar( cmd->rightmove - 127 );
	}
	if ( CL_SplitScreenCommandDown( player, "+moveright" ) ) {
		cmd->rightmove = ClampChar( cmd->rightmove + 127 );
	}
	if ( CL_SplitScreenCommandDown( player, "+left" ) ) {
		if ( CL_SplitScreenCommandDown( player, "+strafe" ) ) {
			cmd->rightmove = ClampChar( cmd->rightmove - 127 );
		} else {
			cl_splitScreenViewangles[player][YAW] += 0.001f * cls.frametime * cl_splitScreenSensitivity[player]->value * 127.0f;
			cmd->angles[YAW] = ANGLE2SHORT( cl_splitScreenViewangles[player][YAW] );
		}
	}
	if ( CL_SplitScreenCommandDown( player, "+right" ) ) {
		if ( CL_SplitScreenCommandDown( player, "+strafe" ) ) {
			cmd->rightmove = ClampChar( cmd->rightmove + 127 );
		} else {
			cl_splitScreenViewangles[player][YAW] -= 0.001f * cls.frametime * cl_splitScreenSensitivity[player]->value * 127.0f;
			cmd->angles[YAW] = ANGLE2SHORT( cl_splitScreenViewangles[player][YAW] );
		}
	}
	if ( CL_SplitScreenCommandDown( player, "+lookup" ) ) {
		cl_splitScreenViewangles[player][PITCH] -= 0.001f * cls.frametime * cl_splitScreenSensitivity[player]->value * 127.0f;
		cl_splitScreenViewangles[player][PITCH] = Com_Clamp( -89.0f, 89.0f, cl_splitScreenViewangles[player][PITCH] );
		cmd->angles[PITCH] = ANGLE2SHORT( cl_splitScreenViewangles[player][PITCH] );
	}
	if ( CL_SplitScreenCommandDown( player, "+lookdown" ) ) {
		cl_splitScreenViewangles[player][PITCH] += 0.001f * cls.frametime * cl_splitScreenSensitivity[player]->value * 127.0f;
		cl_splitScreenViewangles[player][PITCH] = Com_Clamp( -89.0f, 89.0f, cl_splitScreenViewangles[player][PITCH] );
		cmd->angles[PITCH] = ANGLE2SHORT( cl_splitScreenViewangles[player][PITCH] );
	}
	if ( CL_SplitScreenCommandDown( player, "+moveup" ) ) {
		cmd->upmove = 127;
	}
	if ( CL_SplitScreenCommandDown( player, "+movedown" ) ) {
		cmd->upmove = -127;
	}
	if ( CL_SplitScreenCommandDown( player, "+speed" ) ) {
		cmd->buttons |= BUTTON_WALKING;
	}
	if ( !cl_splitScreenAttackBlockedUntilRelease[player] &&
		( CL_SplitScreenCommandDown( player, "+attack" ) ||
		CL_SplitScreenCommandPressed( player, "+attack" ) ) ) {
		cmd->buttons |= BUTTON_ATTACK;
	}
	if ( CL_SplitScreenCommandDown( player, "+altattack" ) ) {
		cmd->buttons |= BUTTON_ALT_ATTACK;
	}
	if ( CL_SplitScreenCommandDown( player, "+use" ) ) {
		cmd->buttons |= BUTTON_USE;
	}
	if ( CL_SplitScreenCommandDown( player, "+button2" ) ) {
		cmd->buttons |= BUTTON_USE_HOLDABLE;
	}
	if ( CL_SplitScreenCommandDown( player, "+useforce" ) ) {
		cmd->buttons |= BUTTON_FORCEPOWER;
	}
	if ( CL_SplitScreenCommandDown( player, "+force_grip" ) ) {
		cmd->buttons |= BUTTON_FORCEGRIP;
	}
	if ( CL_SplitScreenCommandDown( player, "+force_lightning" ) ) {
		cmd->buttons |= BUTTON_FORCE_LIGHTNING;
	}
	if ( CL_SplitScreenCommandDown( player, "+force_drain" ) ) {
		cmd->buttons |= BUTTON_FORCE_DRAIN;
	}
	for ( weapon = 1; weapon <= 13; weapon++ ) {
		if ( CL_SplitScreenCommandDown( player, va( "weapon %i", weapon ) ) ) {
			cmd->weapon = weapon;
		}
	}
}

static qboolean CL_SplitScreenPlayerUsesKeyboard( int player ) {
	char inputName[32];

	if ( !cl_splitScreen->integer ) {
		return (qboolean)( player == 1 );
	}

	Cvar_VariableStringBuffer( va( "ui_splitScreenP%iInput", player ), inputName, sizeof( inputName ) );
	return (qboolean)( !inputName[0] || !Q_stricmp( inputName, "keyboard" ) );
}

static int CL_SplitScreenPlayerForInputDevice( const char *deviceName ) {
	int player;
	char inputName[32];
	int playerCount;

	if ( !deviceName || !deviceName[0] ) {
		return 0;
	}

	playerCount = Cvar_VariableIntegerValue( "ui_splitScreenPlayerCount" );
	if ( playerCount < 1 ) {
		playerCount = 1;
	} else if ( playerCount > 4 ) {
		playerCount = 4;
	}

	for ( player = 1; player <= playerCount; player++ ) {
		Cvar_VariableStringBuffer( va( "ui_splitScreenP%iInput", player ), inputName, sizeof( inputName ) );
		if ( !Q_stricmp( inputName, deviceName ) ) {
			return player;
		}
	}

	return 0;
}

static void CL_SplitScreenCreateCmd( int player, usercmd_t *cmd ) {
	float anglespeed;
	qboolean dead;
	const int moveSide = CL_SplitScreenAxisValue( player, cl_splitScreenMoveSideAxis[player] );
	const int moveForward = CL_SplitScreenAxisValue( player, cl_splitScreenMoveForwardAxis[player] );
	const int lookYaw = CL_SplitScreenAxisValue( player, cl_splitScreenLookYawAxis[player] );
	const int lookPitch = CL_SplitScreenAxisValue( player, cl_splitScreenLookPitchAxis[player] );

	Com_Memset( cmd, 0, sizeof( *cmd ) );
	if ( CL_SplitScreenPlayerUsesKeyboard( player ) ) {
		vec3_t savedViewangles;
		vec3_t oldAngles;

		VectorCopy( cl.viewangles, savedViewangles );
		if ( !cl_splitScreenViewInitialized[player] ) {
			VectorCopy( cl.viewangles, cl_splitScreenViewangles[player] );
			cl_splitScreenViewInitialized[player] = qtrue;
		}
		VectorCopy( cl_splitScreenViewangles[player], cl.viewangles );
		VectorCopy( cl.viewangles, oldAngles );
		CL_AdjustAngles();
		CL_CmdButtons( cmd );
		CL_KeyMove( cmd );
		CL_MouseMove( cmd );
		if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
			cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
		} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
			cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
		}
		CL_FinishMove( cmd );
		VectorCopy( cl.viewangles, cl_splitScreenViewangles[player] );
		VectorCopy( savedViewangles, cl.viewangles );
		return;
	}

	if ( !in_joystick->integer ) {
		CL_SplitScreenRememberControllerButtons( player );
		return;
	}

	if ( !cl_splitScreenViewInitialized[player] ) {
		VectorCopy( cl.viewangles, cl_splitScreenViewangles[player] );
		cl_splitScreenViewInitialized[player] = qtrue;
	}

	dead = (qboolean)( cl.snap.ps.pm_type == PM_DEAD || cl.snap.ps.stats[STAT_HEALTH] <= 0 );
	if ( dead && !cl_splitScreenWasDead[player] && CL_SplitScreenCommandDown( player, "+attack" ) ) {
		cl_splitScreenAttackBlockedUntilRelease[player] = qtrue;
	}
	if ( cl_splitScreenAttackBlockedUntilRelease[player] && !CL_SplitScreenCommandDown( player, "+attack" ) ) {
		cl_splitScreenAttackBlockedUntilRelease[player] = qfalse;
	}
	cl_splitScreenWasDead[player] = dead;

	anglespeed = 0.001f * cls.frametime * cl_splitScreenSensitivity[player]->value;
	cl_splitScreenViewangles[player][YAW] += anglespeed * lookYaw;
	cl_splitScreenViewangles[player][PITCH] += anglespeed * lookPitch * ( cl_splitScreenInvert[player]->integer ? -1.0f : 1.0f );
	cl_splitScreenViewangles[player][PITCH] = Com_Clamp( -89.0f, 89.0f, cl_splitScreenViewangles[player][PITCH] );

	cmd->serverTime = cl.serverTime;
	cmd->angles[PITCH] = ANGLE2SHORT( cl_splitScreenViewangles[player][PITCH] );
	cmd->angles[YAW] = ANGLE2SHORT( cl_splitScreenViewangles[player][YAW] );
	cmd->angles[ROLL] = 0;
	cmd->forwardmove = ClampChar( -moveForward );
	cmd->rightmove = ClampChar( moveSide );
	cmd->upmove = 0;
	cmd->weapon = cl.cgameUserCmdValue;
	cmd->forcesel = cl.cgameForceSelection;
	cmd->invensel = cl.cgameInvenSelection;

	CL_SplitScreenApplyButtonBindings( player, cmd );
	CL_SplitScreenRememberControllerButtons( player );
}

static void CL_SplitScreenSendPlayerCmd( int player ) {
	usercmd_t cmd;
	int hz;
	int interval;

	if ( !cl_splitScreen->integer || !in_joystick->integer || !cl_splitScreenLocalCmds->integer || cls.state != CA_ACTIVE ) {
		return;
	}
	if ( cl_splitClients[player].enabled && cl_splitClients[player].state >= CA_CONNECTED ) {
		return;
	}

	hz = (int)Com_Clamp( 1.0f, 125.0f, cl_splitScreenCmdHz[player]->value );
	interval = 1000 / hz;
	if ( cls.realtime < cl_splitScreenNextCmdTime[player] ) {
		return;
	}
	cl_splitScreenNextCmdTime[player] = cls.realtime + interval;

	CL_SplitScreenCreateCmd( player, &cmd );
	CL_AddReliableCommand( va( "splitscreen_cmd %i %i %i %i %i %i %i %i %i %i %i %i",
		player,
		cmd.serverTime,
		cmd.angles[PITCH],
		cmd.angles[YAW],
		cmd.angles[ROLL],
		cmd.buttons,
		cmd.forwardmove,
		cmd.rightmove,
		cmd.upmove,
		cmd.weapon,
		cmd.forcesel,
		cmd.invensel ), qfalse );
}

static void CL_SplitScreenSendP2Cmd( void ) {
	int player;
	int playerCount;

	if ( !cl_splitScreen->integer || !in_joystick->integer || cls.state != CA_ACTIVE ) {
		return;
	}

	playerCount = Cvar_VariableIntegerValue( "ui_splitScreenPlayerCount" );
	if ( playerCount < 2 ) {
		playerCount = 2;
	} else if ( playerCount > 4 ) {
		playerCount = 4;
	}

	for ( player = 2; player <= playerCount; player++ ) {
		CL_SplitScreenSendPlayerCmd( player );
	}
}

/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( usercmd_t *cmd ) {
	float	mx, my;
	const float	speed = static_cast<float>(frame_msec);

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
		my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
	} else {
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}

	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	if ( mx == 0.0f && my == 0.0f )
		return;

	if ( cl_mouseAccel->value != 0.0f )
	{
		if ( cl_mouseAccelStyle->integer == 0 )
		{
			float accelSensitivity;
			float rate;

			rate = SQRTFAST( mx * mx + my * my ) / speed;

			if ( cl_mYawOverride || cl_mPitchOverride )
			{//FIXME: different people have different speed mouses,
				if ( cl_mSensitivityOverride )
				{
					//this will fuck things up for them, need to clamp
					//max input?
					accelSensitivity = cl_mSensitivityOverride;
				}
				else
				{
					accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
				}
			}
			else
			{
				accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
			}
			mx *= accelSensitivity;
			my *= accelSensitivity;

			if ( cl_showMouseRate->integer )
				Com_Printf( "rate: %f, accelSensitivity: %f\n", rate, accelSensitivity );
		}
		else
		{
			float rate[2];
			float power[2];

			// sensitivity remains pretty much unchanged at low speeds
			// cl_mouseAccel is a power value to how the acceleration is shaped
			// cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
			// NOTE: decouple the config cvars for independent acceleration setup along X and Y?

			rate[0] = fabs( mx ) / speed;
			rate[1] = fabs( my ) / speed;
			power[0] = powf( rate[0] / cl_mouseAccelOffset->value, cl_mouseAccel->value );
			power[1] = powf( rate[1] / cl_mouseAccelOffset->value, cl_mouseAccel->value );

			if ( cl_mYawOverride || cl_mPitchOverride )
			{//FIXME: different people have different speed mouses,
				if ( cl_mSensitivityOverride )
				{
					//this will fuck things up for them, need to clamp
					//max input?
					mx = cl_mSensitivityOverride * (mx + ((mx < 0) ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
					my = cl_mSensitivityOverride * (my + ((my < 0) ? -power[1] : power[1]) * cl_mouseAccelOffset->value);
				}
				else
				{
					mx = cl_sensitivity->value * (mx + ((mx < 0) ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
					my = cl_sensitivity->value * (my + ((my < 0) ? -power[1] : power[1]) * cl_mouseAccelOffset->value);
				}
			}
			else
			{
				mx = cl_sensitivity->value * (mx + ((mx < 0) ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
				my = cl_sensitivity->value * (my + ((my < 0) ? -power[1] : power[1]) * cl_mouseAccelOffset->value);
			}

			if ( cl_showMouseRate->integer )
				Com_Printf( "ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1] );
		}
	}
	else
	{
		if ( cl_mYawOverride || cl_mPitchOverride )
		{//FIXME: different people have different speed mouses,
			if ( cl_mSensitivityOverride )
			{
				//this will fuck things up for them, need to clamp
				//max input?
				mx *= cl_mSensitivityOverride;
				my *= cl_mSensitivityOverride;
			}
			else
			{
				mx *= cl_sensitivity->value;
				my *= cl_sensitivity->value;
			}
		}
		else
		{
			mx *= cl_sensitivity->value;
			my *= cl_sensitivity->value;
		}
	}

	// ingame FOV
	mx *= cl.cgameSensitivity;
	my *= cl.cgameSensitivity;

	// add mouse X/Y movement to cmd
	if ( in_strafe.active )
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	else {
		if ( cl_mYawOverride )
			cl.viewangles[YAW] -= cl_mYawOverride * mx;
		else
			cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ( (in_mlooking || cl_freelook->integer) && !in_strafe.active ) {
		// VVFIXME - This is supposed to be a CVAR
		const float cl_pitchSensitivity = 1.0f;
		const float pitch = cl_bUseFighterPitch ? m_pitchVeh->value : m_pitch->value;
		if ( cl_mPitchOverride ) {
			if ( pitch > 0 )
				cl.viewangles[PITCH] += cl_mPitchOverride * my * cl_pitchSensitivity;
			else
				cl.viewangles[PITCH] -= cl_mPitchOverride * my * cl_pitchSensitivity;
		}
		else
			cl.viewangles[PITCH] += pitch * my * cl_pitchSensitivity;
	}
	else
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
}

qboolean CL_NoUseableForce(void)
{
	if (!cls.cgameStarted)
	{ //ahh, no cgame loaded
		return qfalse;
	}

	return CGVM_NoUseableForce();
}

/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( usercmd_t *cmd ) {
	int		i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for (i = 0 ; i < MAX_KBUTTONS ; i++) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if (cmd->buttons & BUTTON_FORCEPOWER)
	{ //check for transferring a use force to a use inventory...
		if ((cmd->buttons & BUTTON_USE) || CL_NoUseableForce())
		{ //it's pushed, remap it!
			cmd->buttons &= ~BUTTON_FORCEPOWER;
			cmd->buttons |= BUTTON_USE_HOLDABLE;
		}
	}

	if ( Key_GetCatcher( ) ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( kg.anykeydown && Key_GetCatcher( ) == 0 ) {
		cmd->buttons |= BUTTON_ANY;
	}
}


/*
==============
CL_FinishMove
==============
*/
vec3_t cl_sendAngles={0};
vec3_t cl_lastViewAngles={0};
void CL_FinishMove( usercmd_t *cmd ) {
	int		i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;
	cmd->forcesel = cl.cgameForceSelection;
	cmd->invensel = cl.cgameInvenSelection;

	if (cl.gcmdSendValue)
	{
		cmd->generic_cmd = cl.gcmdValue;
		//cl.gcmdSendValue = qfalse;
		cl.gcmdSentValue = qtrue;
	}
	else
	{
		cmd->generic_cmd = 0;
	}

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	if (cl.cgameViewAngleForceTime > cl.serverTime)
	{
		cl.cgameViewAngleForce[YAW] -= SHORT2ANGLE(cl.snap.ps.delta_angles[YAW]);

		cl.viewangles[YAW] = cl.cgameViewAngleForce[YAW];
		cl.cgameViewAngleForceTime = 0;
	}

	if ( cl_crazyShipControls )
	{
		float pitchSubtract, pitchDelta, yawDelta;

		yawDelta = AngleSubtract(cl.viewangles[YAW],cl_lastViewAngles[YAW]);
		//yawDelta *= (4.0f*pVeh->m_fTimeModifier);
		cl_sendAngles[ROLL] -= yawDelta;

		float nRoll = fabs(cl_sendAngles[ROLL]);

		pitchDelta = AngleSubtract(cl.viewangles[PITCH],cl_lastViewAngles[PITCH]);
		//pitchDelta *= (2.0f*pVeh->m_fTimeModifier);
		pitchSubtract = pitchDelta * (nRoll/90.0f);
		cl_sendAngles[PITCH] += pitchDelta-pitchSubtract;

		//yaw-roll calc should be different
		if (nRoll > 90.0f)
		{
			nRoll -= 180.0f;
		}
		if (nRoll < 0.0f)
		{
			nRoll = -nRoll;
		}
		pitchSubtract = pitchDelta * (nRoll/90.0f);
		if ( cl_sendAngles[ROLL] > 0.0f )
		{
			cl_sendAngles[YAW] += pitchSubtract;
		}
		else
		{
			cl_sendAngles[YAW] -= pitchSubtract;
		}

		cl_sendAngles[PITCH] = AngleNormalize180( cl_sendAngles[PITCH] );
		cl_sendAngles[YAW] = AngleNormalize360( cl_sendAngles[YAW] );
		cl_sendAngles[ROLL] = AngleNormalize180( cl_sendAngles[ROLL] );

		for (i=0 ; i<3 ; i++) {
			cmd->angles[i] = ANGLE2SHORT(cl_sendAngles[i]);
		}
	}
	else
	{
		for (i=0 ; i<3 ; i++) {
			cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
		}
		//in case we switch to the cl_crazyShipControls
		VectorCopy( cl.viewangles, cl_sendAngles );
	}
	//always needed in for the cl_crazyShipControls
	VectorCopy( cl.viewangles, cl_lastViewAngles );
}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void ) {
	usercmd_t	cmd;
	vec3_t		oldAngles;

	if ( cl_splitScreen->integer && !CL_SplitScreenPlayerUsesKeyboard( 1 ) ) {
		CL_SplitScreenCreateCmd( 1, &cmd );
		return cmd;
	}

	VectorCopy( cl.viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles ();

	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	if ( CL_SplitScreenPlayerUsesKeyboard( 1 ) ) {
		CL_KeyMove( &cmd );
	}

	// get basic movement from mouse
	if ( CL_SplitScreenPlayerUsesKeyboard( 1 ) ) {
		CL_MouseMove( &cmd );
	}

	// get basic movement from joystick
	if ( !cl_splitScreen->integer ) {
		CL_JoystickMove( &cmd );
	}

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs(cl.viewangles[YAW] - oldAngles[YAW]), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs(cl.viewangles[PITCH] - oldAngles[PITCH]), 0 );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CA_PRIMED )
		return;

	frame_msec = com_frameTime - old_com_frameTime;

	// if running over 1000fps, act as if each frame is 1ms
	// prevents divisions by zero
	if ( frame_msec < 1 )
		frame_msec = 1;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 )
		frame_msec = 200;

	old_com_frameTime = com_frameTime;

	// generate a command for this frame
	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdNum] = CL_CreateCmd();
	CL_SplitScreenSendP2Cmd();
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( cls.state != CA_ACTIVE &&
		cls.state != CA_PRIMED &&
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( cl_lanForcePackets->integer && Sys_IsLANAddress( &clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 20 ) {
		Cvar_Set( "cl_maxpackets", "20" );
	}
	else if ( cl_maxpackets->integer > 1000 ) {
		Cvar_Set( "cl_maxpackets", "1000" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid
			|| clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.serverCommands[ clc.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		if (cl.gcmdSentValue)
		{ //hmm, just clear here, I guess.. hoping it will resolve issues with gencmd values sometimes not going through.
			cl.gcmdSendValue = qfalse;
			cl.gcmdSentValue = qfalse;
			cl.gcmdValue = 0;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	while ( clc.netchan.unsentFragments ) {
		CL_Netchan_TransmitNextFragment( &clc.netchan );
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( cls.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		CL_SplitNetSendCmds();
		return;
	}

	CL_WritePacket();
	CL_SplitNetSendCmds();
}

void CL_SplitNetSendCmds( void )
{
	int player;

	if ( !cl_splitScreen || !cl_splitScreen->integer ) {
		return;
	}

	for ( player = 2; player <= MAX_SPLITSCREEN_PLAYERS; player++ ) {
		splitScreenClient_t *split = &cl_splitClients[player];
		clientActive_t savedCl;
		clientConnection_t savedClc;
		int cmdNum;
		qboolean keyboardOwner;

		if ( !split->enabled || split->state < CA_CONNECTED ) {
			continue;
		}
		// Match the primary client's pure-server ordering: initialize cgame and
		// send the checksum command before the first usermove. Sending usercmds
		// earlier makes pure servers resend gamestate and reset validation.
		if ( !split->cgameStarted ) {
			connstate_t handshakeState = cls.state;

			// Vanilla/local servers wait for one sequenced packet after
			// connectResponse before sending gamestate. Send that empty
			// acknowledgement, but do not create a usercmd until cgame has
			// initialized and the pure checksum command has been queued.
			savedCl = cl;
			savedClc = clc;
			cl = split->active;
			clc = split->connection;
			cls.state = split->state;
			if ( CL_ReadyToSendPacket() ) {
				CL_WritePacket();
			}
			split->active = cl;
			split->connection = clc;
			cl = savedCl;
			clc = savedClc;
			cls.state = handshakeState;
			continue;
		}
		if ( split->connection.demoplaying || split->connection.netchan.remoteAddress.type == NA_BAD ) {
			continue;
		}
		if ( split->connection.netchan.unsentFragments ) {
			clientConnection_t fragmentClc = clc;
			clc = split->connection;
			CL_Netchan_TransmitNextFragment( &clc.netchan );
			split->connection = clc;
			clc = fragmentClc;
			continue;
		}

		savedCl = cl;
		savedClc = clc;
		keyboardOwner = CL_SplitScreenPlayerUsesKeyboard( player );
		if ( keyboardOwner ) {
			split->active.mouseDx[0] = savedCl.mouseDx[0];
			split->active.mouseDx[1] = savedCl.mouseDx[1];
			split->active.mouseDy[0] = savedCl.mouseDy[0];
			split->active.mouseDy[1] = savedCl.mouseDy[1];
			split->active.mouseIndex = savedCl.mouseIndex;
		}
		cl = split->active;
		clc = split->connection;

		cl.cmdNumber++;
		cmdNum = cl.cmdNumber & CMD_MASK;
		CL_SplitScreenCreateCmd( player, &cl.cmds[cmdNum] );
		if ( keyboardOwner ) {
			savedCl.mouseDx[0] = cl.mouseDx[0];
			savedCl.mouseDx[1] = cl.mouseDx[1];
			savedCl.mouseDy[0] = cl.mouseDy[0];
			savedCl.mouseDy[1] = cl.mouseDy[1];
			savedCl.mouseIndex = cl.mouseIndex;
		}

		if ( CL_ReadyToSendPacket() ) {
			CL_WritePacket();
		}

		split->active = cl;
		split->connection = clc;
		cl = savedCl;
		clc = savedClc;
	}
}

static const cmdList_t inputCmds[] =
{
	{ "centerview", "Centers view on screen", IN_CenterView, NULL },
	{ "+moveup", "Jump", IN_UpDown, NULL },
	{ "-moveup", NULL, IN_UpUp, NULL },
	{ "+movedown", "Crouch", IN_DownDown, NULL },
	{ "-movedown", NULL, IN_DownUp, NULL },
	{ "+left", "Rotate camera left", IN_LeftDown, NULL },
	{ "-left", NULL, IN_LeftUp, NULL },
	{ "+right", "Rotate camera right", IN_RightDown, NULL },
	{ "-right", NULL, IN_RightUp, NULL },
	{ "+forward", "Move forward", IN_ForwardDown, NULL },
	{ "-forward", NULL, IN_ForwardUp, NULL },
	{ "+back", "Move backward", IN_BackDown, NULL },
	{ "-back", NULL, IN_BackUp, NULL },
	{ "+lookup", "Tilt camera up", IN_LookupDown, NULL },
	{ "-lookup", NULL, IN_LookupUp, NULL },
	{ "+lookdown", "Tilt camera down", IN_LookdownDown, NULL },
	{ "-lookdown", NULL, IN_LookdownUp, NULL },
	{ "+strafe", "Hold to strafe", IN_StrafeDown, NULL },
	{ "-strafe", NULL, IN_StrafeUp, NULL },
	{ "+moveleft", "Strafe left", IN_MoveleftDown, NULL },
	{ "-moveleft", NULL, IN_MoveleftUp, NULL },
	{ "+moveright", "Strafe right", IN_MoverightDown, NULL },
	{ "-moveright", NULL, IN_MoverightUp, NULL },
	{ "+speed", "Walk or run", IN_SpeedDown, NULL },
	{ "-speed", NULL, IN_SpeedUp, NULL },
	{ "+attack", "Primary Attack", IN_Button0Down, NULL },
	{ "-attack", NULL, IN_Button0Up, NULL },
	{ "+use", "Use item", IN_Button5Down, NULL },
	{ "-use", NULL, IN_Button5Up, NULL },
	{ "+force_grip", "Hold to use grip force power", IN_Button6Down, NULL },
	{ "-force_grip", NULL, IN_Button6Up, NULL },
	{ "+altattack", "Alternate Attack", IN_Button7Down, NULL },
	{ "-altattack", NULL, IN_Button7Up, NULL },
	{ "+useforce", "Use selected force power", IN_Button9Down, NULL },
	{ "-useforce", NULL, IN_Button9Up, NULL },
	{ "+force_lightning", "Hold to use lightning force power", IN_Button10Down, NULL },
	{ "-force_lightning", NULL, IN_Button10Up, NULL },
	{ "+force_drain", "Hold to use drain force power", IN_Button11Down, NULL },
	{ "-force_drain", NULL, IN_Button11Up, NULL },
	{ "+button0", "Button 0", IN_Button0Down, NULL },
	{ "-button0", NULL, IN_Button0Up, NULL },
	{ "+button1", "Button 1", IN_Button1Down, NULL },
	{ "-button1", NULL, IN_Button1Up, NULL },
	{ "+button2", "Button 2", IN_Button2Down, NULL },
	{ "-button2", NULL, IN_Button2Up, NULL },
	{ "+button3", "Button 3", IN_Button3Down, NULL },
	{ "-button3", NULL, IN_Button3Up, NULL },
	{ "+button4", "Button 4", IN_Button4Down, NULL },
	{ "-button4", NULL, IN_Button4Up, NULL },
	{ "+button5", "Button 5", IN_Button5Down, NULL },
	{ "-button5", NULL, IN_Button5Up, NULL },
	{ "+button6", "Button 6", IN_Button6Down, NULL },
	{ "-button6", NULL, IN_Button6Up, NULL },
	{ "+button7", "Button 7", IN_Button7Down, NULL },
	{ "-button7", NULL, IN_Button7Up, NULL },
	{ "+button8", "Button 8", IN_Button8Down, NULL },
	{ "-button8", NULL, IN_Button8Up, NULL },
	{ "+button9", "Button 9", IN_Button9Down, NULL },
	{ "-button9", NULL, IN_Button9Up, NULL },
	{ "+button10", "Button 10", IN_Button10Down, NULL },
	{ "-button10", NULL, IN_Button10Up, NULL },
	{ "+button11", "Button 11", IN_Button11Down, NULL },
	{ "-button11", NULL, IN_Button11Up, NULL },
	{ "+button12", "Button 12", IN_Button12Down, NULL },
	{ "-button12", NULL, IN_Button12Up, NULL },
	{ "+button13", "Button 13", IN_Button13Down, NULL },
	{ "-button13", NULL, IN_Button13Up, NULL },
	{ "+button14", "Button 14", IN_Button14Down, NULL },
	{ "-button14", NULL, IN_Button14Up, NULL },
	{ "+button15", "Button 15", IN_Button15Down, NULL },
	{ "-button15", NULL, IN_Button15Up, NULL },
	{ "+mlook", "Hold to use mouse look", IN_MLookDown, NULL },
	{ "-mlook", NULL, IN_MLookUp, NULL },
	{ "sv_saberswitch", "Holster/activate lightsaber", IN_GenCMD1, NULL },
	{ "engage_duel", "Engage private duel", IN_GenCMD2, NULL },
	{ "force_heal", "Use heal force power", IN_GenCMD3, NULL },
	{ "force_speed", "Activate speed force power", IN_GenCMD4, NULL },
	{ "force_pull", "Use pull force power", IN_GenCMD5, NULL },
	{ "force_distract", "Activate mind trick force power", IN_GenCMD6, NULL },
	{ "force_rage", "Activate rage force power", IN_GenCMD7, NULL },
	{ "force_protect", "Activate protect force power", IN_GenCMD8, NULL },
	{ "force_absorb", "Activate absorb force power", IN_GenCMD9, NULL },
	{ "force_healother", "Use team heal force power", IN_GenCMD10, NULL },
	{ "force_forcepowerother", "Use team energize force power", IN_GenCMD11, NULL },
	{ "force_seeing", "Activate seeing force power", IN_GenCMD12, NULL },
	{ "use_seeker", "Use seeker drone item", IN_GenCMD13, NULL },
	{ "use_field", "Use forcefield item", IN_GenCMD14, NULL },
	{ "use_bacta", "Use bacta item", IN_GenCMD15, NULL },
	{ "use_electrobinoculars", "Use electro binoculars item", IN_GenCMD16, NULL },
	{ "zoom", "Use binoculars item", IN_GenCMD17, NULL },
	{ "use_sentry", "Use sentry gun item", IN_GenCMD18, NULL },
	{ "saberAttackCycle", "Switch lightsaber attack styles", IN_GenCMD19, NULL },
	{ "force_throw", "Use push force power", IN_GenCMD20, NULL },
	{ "use_jetpack", "Use jetpack item", IN_GenCMD21, NULL },
	{ "use_bactabig", "Use big bacta item", IN_GenCMD22, NULL },
	{ "use_healthdisp", "Use health dispenser item", IN_GenCMD23, NULL },
	{ "use_ammodisp", "Use ammo dispenser item", IN_GenCMD24, NULL },
	{ "use_eweb", "Use e-web item", IN_GenCMD25, NULL },
	{ "use_cloak", "Use cloaking item", IN_GenCMD26, NULL },
	{ "taunt", "Taunt", IN_GenCMD27, NULL },
	{ "bow", "Bow", IN_GenCMD28, NULL },
	{ "meditate", "Meditate", IN_GenCMD29, NULL },
	{ "flourish", "Flourish", IN_GenCMD30, NULL },
	{ "gloat", "Gloat", IN_GenCMD31, NULL },
	{ "useGivenForce", "Use specified force power", IN_UseGivenForce, NULL },
	{ "automap_button", "Show/hide automap", IN_AutoMapButton, NULL },
	{ "automap_toggle", "Show/hide radar", IN_AutoMapToggle, NULL },
	{ "voicechat", "Open voice chat menu", IN_VoiceChatButton, NULL },
	{ "splitinput_axis", "Inject a split-screen controller axis value", CL_SplitInputAxis_f, NULL },
	{ "splitinput_button", "Inject a split-screen controller button value", CL_SplitInputButton_f, NULL },
	{ "splitinput_device_axis", "Inject a device-routed split-screen controller axis value", CL_SplitInputDeviceAxis_f, NULL },
	{ "splitinput_device_button", "Inject a device-routed split-screen controller button value", CL_SplitInputDeviceButton_f, NULL },
	{ "splitinput_route_status", "Print split-screen input device routing", CL_SplitInputRouteStatus_f, NULL },
	{ "splitinput_assert_model", "Assert a split-screen player's model cvar for QA", CL_SplitInputAssertModel_f, NULL },
	{ "splitinput_assert_cmd", "Assert a generated split-screen user command for QA", CL_SplitInputAssertCmd_f, NULL },
	{ "splitinput_clear", "Clear injected split-screen controller state", CL_SplitInputClear_f, NULL },
	{ "splitscreen_bind_controller", "Bind a split-screen player's controller button to a command", CL_SplitScreenBindController_f, NULL },
	{ "splitscreen_reset_controller", "Restore a split-screen player's default controller bindings", CL_SplitScreenResetController_f, NULL },
	{ NULL, NULL, NULL, NULL }
};

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	int splitPlayer;
	cvar_t *splitScreenBindingsVersion;
	qboolean resetSplitScreenBindings;

	Cmd_AddCommandList( inputCmds );

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
	cl_splitScreen = Cvar_Get( "cl_splitScreen", "0", CVAR_ARCHIVE_ND, "Enable local split-screen command generation." );
	cl_splitScreenLocalCmds = Cvar_Get( "cl_splitScreenLocalCmds", "1", 0, "Send legacy local-server split-screen commands." );
	splitScreenBindingsVersion = Cvar_Get( "cl_splitScreenBindingsVersion", "0", CVAR_ARCHIVE_ND, "Version of the split-screen controller defaults." );
	resetSplitScreenBindings = (qboolean)( splitScreenBindingsVersion->integer < 4 );
	for ( splitPlayer = 1; splitPlayer <= 4; splitPlayer++ ) {
		int bindIndex;
		cl_splitScreenInvert[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iInvert", splitPlayer ), "0", CVAR_ARCHIVE_ND, va( "Invert Player %i controller pitch.", splitPlayer ) );
		cl_splitScreenSensitivity[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iSensitivity", splitPlayer ), "140", CVAR_ARCHIVE_ND, va( "Player %i controller look sensitivity.", splitPlayer ) );
		cl_splitScreenCmdHz[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iCmdHz", splitPlayer ), "30", CVAR_ARCHIVE_ND, va( "Player %i split-screen command rate.", splitPlayer ) );
		cl_splitScreenMoveSideAxis[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iMoveSideAxis", splitPlayer ), "0", CVAR_ARCHIVE_ND, va( "Player %i controller left/right movement axis.", splitPlayer ) );
		cl_splitScreenMoveForwardAxis[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iMoveForwardAxis", splitPlayer ), "1", CVAR_ARCHIVE_ND, va( "Player %i controller forward/back movement axis.", splitPlayer ) );
		cl_splitScreenLookYawAxis[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iLookYawAxis", splitPlayer ), "2", CVAR_ARCHIVE_ND, va( "Player %i controller look yaw axis.", splitPlayer ) );
		cl_splitScreenLookPitchAxis[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iLookPitchAxis", splitPlayer ), "3", CVAR_ARCHIVE_ND, va( "Player %i controller look pitch axis.", splitPlayer ) );
		cl_splitScreenAttackButton[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iAttackButton", splitPlayer ), "0", CVAR_ARCHIVE_ND, va( "Player %i controller attack button.", splitPlayer ) );
		cl_splitScreenAltAttackButton[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iAltAttackButton", splitPlayer ), "1", CVAR_ARCHIVE_ND, va( "Player %i controller alt attack button.", splitPlayer ) );
		cl_splitScreenUseButton[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iUseButton", splitPlayer ), "2", CVAR_ARCHIVE_ND, va( "Player %i controller use button.", splitPlayer ) );
		cl_splitScreenJumpButton[splitPlayer] = Cvar_Get( va( "cl_splitScreenP%iJumpButton", splitPlayer ), "3", CVAR_ARCHIVE_ND, va( "Player %i controller jump button.", splitPlayer ) );
		for ( bindIndex = 0; bindIndex < (int)ARRAY_LEN( cl_splitScreenBindCommands ); bindIndex++ ) {
			int defaultButtonValue;
			const char *command = cl_splitScreenBindCommands[bindIndex];

			defaultButtonValue = CL_SplitScreenDefaultBindForCommand( command, bindIndex );
			cl_splitScreenBindButton[splitPlayer][bindIndex] = Cvar_Get( va( "cl_splitScreenP%iBind%02i", splitPlayer, bindIndex ), va( "%i", defaultButtonValue ), CVAR_ARCHIVE_ND, va( "Player %i controller binding for %s.", splitPlayer, command ) );
			if ( resetSplitScreenBindings || cl_splitScreenBindButton[splitPlayer][bindIndex]->integer < -1 || cl_splitScreenBindButton[splitPlayer][bindIndex]->integer > 15 ) {
				Cvar_Set( cl_splitScreenBindButton[splitPlayer][bindIndex]->name, va( "%i", defaultButtonValue ) );
			}
		}
		Cvar_Get( va( "cl_splitScreenP%iClientNum", splitPlayer ), "-1", 0, va( "Player %i split-screen network client slot.", splitPlayer ) );
	}
	if ( resetSplitScreenBindings ) {
		Cvar_Set( splitScreenBindingsVersion->name, "4" );
	}
	Cvar_Get( "ui_splitScreenP1Name", "Padawan", CVAR_ARCHIVE_ND, "Split-screen Player 1 display name." );
	Cvar_Get( "ui_splitScreenP1Model", DEFAULT_MODEL"/default", CVAR_ARCHIVE_ND, "Split-screen Player 1 model/skin." );
	Cvar_Get( "ui_splitScreenP1Saber1", DEFAULT_SABER, CVAR_ARCHIVE_ND, "Split-screen Player 1 primary saber." );
	Cvar_Get( "ui_splitScreenP1Saber2", "none", CVAR_ARCHIVE_ND, "Split-screen Player 1 secondary saber." );
	Cvar_Get( "ui_splitScreenP1Color1", "4", CVAR_ARCHIVE_ND, "Split-screen Player 1 saber color 1." );
	Cvar_Get( "ui_splitScreenP1Color2", "3", CVAR_ARCHIVE_ND, "Split-screen Player 1 saber color 2." );
	Cvar_Get( "ui_splitScreenP1CharRed", "255", CVAR_ARCHIVE_ND, "Split-screen Player 1 custom character red channel." );
	Cvar_Get( "ui_splitScreenP1CharGreen", "255", CVAR_ARCHIVE_ND, "Split-screen Player 1 custom character green channel." );
	Cvar_Get( "ui_splitScreenP1CharBlue", "255", CVAR_ARCHIVE_ND, "Split-screen Player 1 custom character blue channel." );
	Cvar_Get( "ui_splitScreenP1ForcePowers", DEFAULT_FORCEPOWERS, CVAR_ARCHIVE_ND, "Split-screen Player 1 force power loadout." );
	Cvar_Get( "ui_splitScreenP2Name", "SplitPlayer2", CVAR_ARCHIVE_ND, "Split-screen Player 2 display name." );
	Cvar_Get( "ui_splitScreenP2Model", DEFAULT_MODEL"/default", CVAR_ARCHIVE_ND, "Split-screen Player 2 model/skin." );
	Cvar_Get( "ui_splitScreenP2Saber1", DEFAULT_SABER, CVAR_ARCHIVE_ND, "Split-screen Player 2 primary saber." );
	Cvar_Get( "ui_splitScreenP2Saber2", "none", CVAR_ARCHIVE_ND, "Split-screen Player 2 secondary saber." );
	Cvar_Get( "ui_splitScreenP2Color1", "4", CVAR_ARCHIVE_ND, "Split-screen Player 2 saber color 1." );
	Cvar_Get( "ui_splitScreenP2Color2", "3", CVAR_ARCHIVE_ND, "Split-screen Player 2 saber color 2." );
	Cvar_Get( "ui_splitScreenP2CharRed", "255", CVAR_ARCHIVE_ND, "Split-screen Player 2 custom character red channel." );
	Cvar_Get( "ui_splitScreenP2CharGreen", "255", CVAR_ARCHIVE_ND, "Split-screen Player 2 custom character green channel." );
	Cvar_Get( "ui_splitScreenP2CharBlue", "255", CVAR_ARCHIVE_ND, "Split-screen Player 2 custom character blue channel." );
	Cvar_Get( "ui_splitScreenP2ForcePowers", DEFAULT_FORCEPOWERS, CVAR_ARCHIVE_ND, "Split-screen Player 2 force power loadout." );
	Cvar_Get( "ui_splitScreenP3Name", "SplitPlayer3", CVAR_ARCHIVE_ND, "Split-screen Player 3 display name." );
	Cvar_Get( "ui_splitScreenP3Model", DEFAULT_MODEL"/default", CVAR_ARCHIVE_ND, "Split-screen Player 3 model/skin." );
	Cvar_Get( "ui_splitScreenP3Saber1", DEFAULT_SABER, CVAR_ARCHIVE_ND, "Split-screen Player 3 primary saber." );
	Cvar_Get( "ui_splitScreenP3Saber2", "none", CVAR_ARCHIVE_ND, "Split-screen Player 3 secondary saber." );
	Cvar_Get( "ui_splitScreenP3Color1", "2", CVAR_ARCHIVE_ND, "Split-screen Player 3 saber color 1." );
	Cvar_Get( "ui_splitScreenP3Color2", "3", CVAR_ARCHIVE_ND, "Split-screen Player 3 saber color 2." );
	Cvar_Get( "ui_splitScreenP3CharRed", "255", CVAR_ARCHIVE_ND, "Split-screen Player 3 custom character red channel." );
	Cvar_Get( "ui_splitScreenP3CharGreen", "255", CVAR_ARCHIVE_ND, "Split-screen Player 3 custom character green channel." );
	Cvar_Get( "ui_splitScreenP3CharBlue", "255", CVAR_ARCHIVE_ND, "Split-screen Player 3 custom character blue channel." );
	Cvar_Get( "ui_splitScreenP3ForcePowers", DEFAULT_FORCEPOWERS, CVAR_ARCHIVE_ND, "Split-screen Player 3 force power loadout." );
	Cvar_Get( "ui_splitScreenP4Name", "SplitPlayer4", CVAR_ARCHIVE_ND, "Split-screen Player 4 display name." );
	Cvar_Get( "ui_splitScreenP4Model", DEFAULT_MODEL"/default", CVAR_ARCHIVE_ND, "Split-screen Player 4 model/skin." );
	Cvar_Get( "ui_splitScreenP4Saber1", DEFAULT_SABER, CVAR_ARCHIVE_ND, "Split-screen Player 4 primary saber." );
	Cvar_Get( "ui_splitScreenP4Saber2", "none", CVAR_ARCHIVE_ND, "Split-screen Player 4 secondary saber." );
	Cvar_Get( "ui_splitScreenP4Color1", "5", CVAR_ARCHIVE_ND, "Split-screen Player 4 saber color 1." );
	Cvar_Get( "ui_splitScreenP4Color2", "3", CVAR_ARCHIVE_ND, "Split-screen Player 4 saber color 2." );
	Cvar_Get( "ui_splitScreenP4CharRed", "255", CVAR_ARCHIVE_ND, "Split-screen Player 4 custom character red channel." );
	Cvar_Get( "ui_splitScreenP4CharGreen", "255", CVAR_ARCHIVE_ND, "Split-screen Player 4 custom character green channel." );
	Cvar_Get( "ui_splitScreenP4CharBlue", "255", CVAR_ARCHIVE_ND, "Split-screen Player 4 custom character blue channel." );
	Cvar_Get( "ui_splitScreenP4ForcePowers", DEFAULT_FORCEPOWERS, CVAR_ARCHIVE_ND, "Split-screen Player 4 force power loadout." );
	Cvar_Get( "ui_splitScreenProfileTarget", "1", CVAR_ARCHIVE_ND, "Profile menu target player for split-screen setup." );
	Cvar_Get( "ui_splitScreenPlayerCount", "2", CVAR_ARCHIVE_ND, "Requested local split-screen player count." );
	Cvar_Get( "ui_splitScreenGameType", "0", CVAR_ARCHIVE_ND, "Requested local split-screen game type." );
	Cvar_Get( "ui_splitScreenMap", "mp/ffa3", CVAR_ARCHIVE_ND, "Requested local split-screen map." );
	Cvar_Get( "ui_splitScreenSessionType", "local", CVAR_ARCHIVE_ND, "Requested split-screen session type." );
	Cvar_Get( "ui_splitScreenP1Input", "keyboard", CVAR_ARCHIVE_ND, "Split-screen Player 1 input device assignment." );
	Cvar_Get( "ui_splitScreenP2Input", "controller1", CVAR_ARCHIVE_ND, "Split-screen Player 2 input device assignment." );
	Cvar_Get( "ui_splitScreenP3Input", "controller2", CVAR_ARCHIVE_ND, "Split-screen Player 3 input device assignment." );
	Cvar_Get( "ui_splitScreenP4Input", "controller3", CVAR_ARCHIVE_ND, "Split-screen Player 4 input device assignment." );
	Cvar_Get( "ui_splitScreenP2Joined", "0", CVAR_ARCHIVE_ND, "Whether split-screen Player 2 is currently joined." );
	Cvar_Get( "ui_splitScreenP3Joined", "0", CVAR_ARCHIVE_ND, "Whether split-screen Player 3 is currently joined." );
	Cvar_Get( "ui_splitScreenP4Joined", "0", CVAR_ARCHIVE_ND, "Whether split-screen Player 4 is currently joined." );
	/*
	 * Menu scripts may only assign cvars that already exist.  Register the
	 * transient setup-routing state before the front-end menus can open; a
	 * fresh home otherwise drops the setup transition on the floor.
	 */
	Cvar_Get( "ui_splitScreenConfiguring", "0", 0, "Whether a split-screen profile is being configured." );
	Cvar_Get( "ui_splitScreenMenuMode", "", 0, "Active split-screen UI composition mode." );
	Cvar_Get( "ui_splitScreenPendingSetup", "0", 0, "Deferred split-screen player setup transition." );
	Cvar_Get( "ui_splitScreenInputTarget", "0", 0, "Player targeted by the current split-screen input device." );
	Cvar_Get( "ui_splitScreenLastInputDevice", "keyboard", 0, "Most recent split-screen UI input device." );
	Cvar_Get( "ui_splitScreenLastPaint", "", 0, "Most recent split-screen UI composition path." );
}

/*
============
CL_ShutdownInput
============
*/
void CL_ShutdownInput( void ) {
	Cmd_RemoveCommandList( inputCmds );
}
