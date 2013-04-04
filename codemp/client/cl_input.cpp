//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// cl->input.c  -- builds an intended movement command to send to the server

#include "client.h"
#ifdef _XBOX
#include "cl_input_hotswap.h"
#include "../xbox/XBVoice.h"
#include "../cgame/cg_local.h"
#include "cl_data.h"
#endif
unsigned	frame_msec;
int			old_com_frameTime;

float cl_mPitchOverride = 0.0f;
float cl_mYawOverride = 0.0f;
float cl_mSensitivityOverride = 0.0f;
qboolean cl_bUseFighterPitch = qfalse;
qboolean cl_crazyShipControls = qfalse;
#define	OVERRIDE_MOUSE_SENSITIVITY 5.0f//20.0f = 180 degree turn in one mouse swipe across keyboard
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

kbutton_t	in_buttons[16];


qboolean	in_mlooking;


#ifdef _XBOX
//HotSwapManager swapMan1(HOTSWAP_ID_WHITE);
//HotSwapManager swapMan2(HOTSWAP_ID_BLACK);


void IN_HotSwap1On(void)
{
	ClientManager::ActiveClient().swapMan1.SetDown();
}


void IN_HotSwap2On(void)
{
	ClientManager::ActiveClient().swapMan2.SetDown();
}


void IN_HotSwap1Off(void)
{
	ClientManager::ActiveClient().swapMan1.SetUp();
}


void IN_HotSwap2Off(void)
{
	ClientManager::ActiveClient().swapMan2.SetUp();
}


void CL_UpdateHotSwap(void)
{
	CM_START_LOOP();
	ClientManager::ActiveClient().swapMan1.Update();
	ClientManager::ActiveClient().swapMan2.Update();
	CM_END_LOOP();
}


bool CL_ExtendSelectTime(void)
{
	return ClientManager::ActiveClient().swapMan1.ButtonDown() ||
		   ClientManager::ActiveClient().swapMan2.ButtonDown();
}


#endif


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
		cl->gcmdSendValue = qtrue;
		cl->gcmdValue = genCmdNum;
	}
}

void IN_MLookDown( void ) {
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		ClientManager::ActiveClient().in_mlooking = qtrue;
	}
	else
#endif

	in_mlooking = qtrue;
}

void IN_MLookUp( void ) {
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		ClientManager::ActiveClient().in_mlooking = qfalse;
	}
	else
#endif

	in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView ();
	}
}

void IN_GenCMD1( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_SABERSWITCH;
}

void IN_GenCMD2( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_ENGAGE_DUEL;
}

void IN_GenCMD3( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_HEAL;
}

void IN_GenCMD4( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_SPEED;
}

void IN_GenCMD5( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_PULL;
}

void IN_GenCMD6( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_DISTRACT;
}

void IN_GenCMD7( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_RAGE;
}

void IN_GenCMD8( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_PROTECT;
}

void IN_GenCMD9( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_ABSORB;
}

void IN_GenCMD10( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_HEALOTHER;
}

void IN_GenCMD11( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_FORCEPOWEROTHER;
}

void IN_GenCMD12( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_SEEING;
}

void IN_GenCMD13( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_SEEKER;
}

void IN_GenCMD14( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_FIELD;
}

void IN_GenCMD15( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_BACTA;
}

void IN_GenCMD16( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_ELECTROBINOCULARS;
}

void IN_GenCMD17( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_ZOOM;
}

void IN_GenCMD18( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_SENTRY;
}

void IN_GenCMD19( void )
{
#ifdef _XBOX
	if (cl->snap.ps.weapon != WP_SABER)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "weapon 1");
		return;
	}
#endif

	if (Cvar_VariableIntegerValue("d_saberStanceDebug"))
	{
		Com_Printf("SABERSTANCEDEBUG: Gencmd on client set successfully.\n");
	}
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_SABERATTACKCYCLE;
}

void IN_GenCMD20( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FORCE_THROW;
}

void IN_GenCMD21( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_JETPACK;
}

void IN_GenCMD22( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_BACTABIG;
}

void IN_GenCMD23( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_HEALTHDISP;
}

void IN_GenCMD24( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_AMMODISP;
}

void IN_GenCMD25( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_EWEB;
}

void IN_GenCMD26( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_USE_CLOAK;
}

void IN_GenCMD27( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_TAUNT;
}

void IN_GenCMD28( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_BOW;
}

void IN_GenCMD29( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_MEDITATE;
}

void IN_GenCMD30( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_FLOURISH;
}

void IN_GenCMD31( void )
{
	cl->gcmdSendValue = qtrue;
	cl->gcmdValue = GENCMD_GLOAT;
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

	if (Cvar_VariableIntegerValue("cg_drawRadar"))
	{
		Cvar_Set("cg_drawRadar", "0");
	}
	else
	{
		Cvar_Set("cg_drawRadar", "1");
	}
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
	if (!uivm)
	{ //ui not loaded so this command is useless
		return;
	}
	VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_VOICECHAT );
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
	autoMapInput_t *data = (autoMapInput_t *)cl->mSharedMemory;

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

	if (cgvm)
	{
		VM_Call(cgvm, CG_AUTOMAP_INPUT, 0);
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
            IN_KeyDown(&ClientManager::ActiveClient().in_up);
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
            IN_KeyUp(&ClientManager::ActiveClient().in_up);
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
            IN_KeyDown(&ClientManager::ActiveClient().in_down);
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
            IN_KeyUp(&ClientManager::ActiveClient().in_down);
		else
#endif
		IN_KeyUp(&in_down);
	}
}

void IN_LeftDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_left);
	}
	else
#endif
	IN_KeyDown(&in_left);
}

void IN_LeftUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_left);
	}
	else
#endif
	IN_KeyUp(&in_left);
}

void IN_RightDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_right);
	}
	else
#endif
	IN_KeyDown(&in_right);
}


void IN_RightUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_right);
	}
	else
#endif
	IN_KeyUp(&in_right);
}


void IN_ForwardDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_FORWARD, qfalse);
	}
	else
	{
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyDown(&ClientManager::ActiveClient().in_forward);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyUp(&ClientManager::ActiveClient().in_forward);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyDown(&ClientManager::ActiveClient().in_back);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyUp(&ClientManager::ActiveClient().in_back);
		}
		else
#endif
		IN_KeyUp(&in_back);
	}
}

void IN_LookupDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_lookup);
	}
	else
#endif
	IN_KeyDown(&in_lookup);
}

void IN_LookupUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_lookup);
	}
	else
#endif
	IN_KeyUp(&in_lookup);
}

void IN_LookdownDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_lookdown);
	}
	else
#endif
	IN_KeyDown(&in_lookdown);
}

void IN_LookdownUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_lookdown);
	}
	else
#endif
	IN_KeyUp(&in_lookdown);
}

void IN_MoveleftDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWLEFT, qfalse);
	}
	else
	{
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyDown(&ClientManager::ActiveClient().in_moveleft);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyUp(&ClientManager::ActiveClient().in_moveleft);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyDown(&ClientManager::ActiveClient().in_moveright);
		}
		else
#endif
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
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyUp(&ClientManager::ActiveClient().in_moveright);
		}
		else
#endif
		IN_KeyUp(&in_moveright);
	}
}

#ifdef _XBOX
// BTO - State for auto-level usage. This REALLY ought to be in cl[sc ].
// Honestly, I don't want to figure out which one, especially anticipating
// split-screen. All I want for Christmas is to not do split-screen.

// MATT - looks like you got coal in your stocking this year....
//static unsigned long sLastFireTime = 0;
#endif

void IN_SpeedDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_speed);
	}
	else
#endif
	IN_KeyDown(&in_speed);
}

void IN_SpeedUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_speed);
	}
	else
#endif
	IN_KeyUp(&in_speed);
}

void IN_StrafeDown(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_strafe);
	}
	else
#endif
	IN_KeyDown(&in_strafe);
}

void IN_StrafeUp(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_strafe);
	}
	else
#endif
	IN_KeyUp(&in_strafe);
}

void IN_Button0Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[0]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[0]);
}

void IN_Button0Up(void)
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[0]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[0]);

#ifdef _XBOX	// Auto-level. Thought this was nasty, but now I sort of like it.
	ClientManager::ActiveClient().lastFireTime = Sys_Milliseconds();
#endif
}

void IN_Button1Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[1]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[1]);
}

void IN_Button1Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[1]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[1]);
}

void IN_Button2Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[2]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[2]);
}

void IN_Button2Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[2]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[2]);
}

void IN_Button3Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[3]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[3]);
}

void IN_Button3Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[3]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[3]);
}

void IN_Button4Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[4]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[4]);
}

void IN_Button4Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[4]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[4]);
}

void IN_Button5Down(void) //use key
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_DEFAULTVIEW, qfalse);
	}
	else
	{
#ifdef _XBOX
		if(ClientManager::splitScreenMode == qtrue)
		{
			IN_KeyDown(&ClientManager::ActiveClient().in_buttons[5]);
		}
		else
#endif
		IN_KeyDown(&in_buttons[5]);
	}
}

void IN_Button5Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[5]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[5]);
}

void IN_Button6Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[6]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[6]);
}

void IN_Button6Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[6]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[6]);
}

void IN_Button7Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[7]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[7]);
}

void IN_Button7Up(void)
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[7]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[7]);

#ifdef _XBOX	// Auto-level. Thought this was nasty, but now I sort of like it.
	ClientManager::ActiveClient().lastFireTime = Sys_Milliseconds();
#endif
}

void IN_Button8Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[8]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[8]);
}


void IN_Button8Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[8]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[8]);
}

void IN_Button9Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[9]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[9]);
}

void IN_Button9Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[9]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[9]);
}

void IN_Button10Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[10]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[10]);
}

void IN_Button10Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[10]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[10]);
}

void IN_Button11Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[11]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[11]);
}

void IN_Button11Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[11]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[11]);
}

void IN_Button12Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[12]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[12]);
}

void IN_Button12Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[12]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[12]);
}

void IN_Button13Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[13]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[13]);
}

void IN_Button13Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[13]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[13]);
}

void IN_Button14Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[14]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[14]);
}

void IN_Button14Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[14]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[14]);
}

void IN_Button15Down(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[15]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[15]);
}

void IN_Button15Up(void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[15]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[15]);
}

void IN_ButtonDown (void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyDown(&ClientManager::ActiveClient().in_buttons[1]);
	}
	else
#endif
	IN_KeyDown(&in_buttons[1]);
}

void IN_ButtonUp (void) 
{
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		IN_KeyUp(&ClientManager::ActiveClient().in_buttons[1]);
	}
	else
#endif
	IN_KeyUp(&in_buttons[1]);
}

void IN_CenterView (void) {
	cl->viewangles[PITCH] = -SHORT2ANGLE(cl->snap.ps.delta_angles[PITCH]);
}

#ifdef _XBOX

// If the voice toggle button (default: Y) is held for less than TAUNT_PRESS_TIME
// milliseconds - we play the character's taunt animation
#define TAUNT_PRESS_TIME	500

void IN_VoiceToggleDown(void)
{
	g_Voice.SetChannel( CHAN_ALT );
	// Remember when we pressed this. If it's held for a short amount of time, we taunt. =)
	ClientManager::ActiveClient().voiceTogglePressTime = Sys_Milliseconds();
}

void IN_VoiceToggleUp(void)
{
	g_Voice.SetChannel( CHAN_PRIMARY );
	// Check duration of hold
	if( Sys_Milliseconds() - ClientManager::ActiveClient().voiceTogglePressTime < TAUNT_PRESS_TIME )
	{
		cl->gcmdSendValue = qtrue;
		cl->gcmdValue = GENCMD_TAUNT;
	}
}
#endif


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

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( ClientManager::ActiveClient().in_speed.active ) 
		{
			speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
		} else {
			speed = 0.001 * cls.frametime;
		}

		if ( !ClientManager::ActiveClient().in_strafe.active ) 
		{
            if ( cl_mYawOverride )
			{
				cl->viewangles[YAW] -= cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&ClientManager::ActiveClient().in_right);
				cl->viewangles[YAW] += cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&ClientManager::ActiveClient().in_left);
			}
			else
			{
				cl->viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&ClientManager::ActiveClient().in_right);
				cl->viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&ClientManager::ActiveClient().in_left);
			}
		}

		if ( cl_mPitchOverride )
		{
			cl->viewangles[PITCH] -= cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&ClientManager::ActiveClient().in_lookup);
			cl->viewangles[PITCH] += cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&ClientManager::ActiveClient().in_lookdown);
		}
		else
		{
			cl->viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&ClientManager::ActiveClient().in_lookup);
			cl->viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&ClientManager::ActiveClient().in_lookdown);
		}
	}
	else 
	{
#endif // _XBOX

	if ( in_speed.active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		if ( cl_mYawOverride )
		{
			cl->viewangles[YAW] -= cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&in_right);
			cl->viewangles[YAW] += cl_mYawOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_yawspeed->value*CL_KeyState (&in_left);
		}
		else
		{
			cl->viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
			cl->viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
		}
	}

	if ( cl_mPitchOverride )
	{
		cl->viewangles[PITCH] -= cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
		cl->viewangles[PITCH] += cl_mPitchOverride*OVERRIDE_MOUSE_SENSITIVITY*speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
	}
	else
	{
		cl->viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
		cl->viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
	}

#ifdef _XBOX
	}
#endif
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

	forward = 0;
	side = 0;
	up = 0;

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( ClientManager::ActiveClient().in_speed.active ^ cl_run->integer ) {
			movespeed = 127;
			cmd->buttons &= ~BUTTON_WALKING;
		} else {
			cmd->buttons |= BUTTON_WALKING;
			movespeed = 46;
		}

		if ( ClientManager::ActiveClient().in_strafe.active ) 
		{
			side += movespeed * CL_KeyState (&ClientManager::ActiveClient().in_right);
			side -= movespeed * CL_KeyState (&ClientManager::ActiveClient().in_left);
		}

		side += movespeed * CL_KeyState (&ClientManager::ActiveClient().in_moveright);
		side -= movespeed * CL_KeyState (&ClientManager::ActiveClient().in_moveleft);


		up += movespeed * CL_KeyState (&ClientManager::ActiveClient().in_up);
		up -= movespeed * CL_KeyState (&ClientManager::ActiveClient().in_down);

		forward += movespeed * CL_KeyState (&ClientManager::ActiveClient().in_forward);
		forward -= movespeed * CL_KeyState (&ClientManager::ActiveClient().in_back);
	}
	else
	{
#endif // _XBOX

	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 46;
	}

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

#ifdef _XBOX
	}
#endif

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

#ifdef _XBOX
	// No mouse movement if the player is taunting
	if(ClientManager::splitScreenMode == qtrue && cg->predictedPlayerState.forceHandExtend == HANDEXTEND_TAUNT)
		return;
#endif
	if (g_clAutoMapMode && cgvm)
	{ //automap input
		autoMapInput_t *data = (autoMapInput_t *)cl->mSharedMemory;

		g_clAutoMapInput.yaw = dx;
		g_clAutoMapInput.pitch = dy;
		memcpy(data, &g_clAutoMapInput, sizeof(autoMapInput_t));
		VM_Call(cgvm, CG_AUTOMAP_INPUT, 1);

		g_clAutoMapInput.yaw = 0.0f;
		g_clAutoMapInput.pitch = 0.0f;
	}
	else if ( cls.keyCatchers & KEYCATCH_UI ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
	} else if (cls.keyCatchers & KEYCATCH_CGAME) {
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
	} else {
		cl->mouseDx[cl->mouseIndex] += dx;
		cl->mouseDy[cl->mouseIndex] += dy;
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
	cl->joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
extern cvar_t *in_joystick;
void CL_JoystickMove( usercmd_t *cmd ) {
/*
	if ( !in_joystick->integer )
	{
		return;
	}
*/

//	int		movespeed;
//	float	anglespeed;

/*
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( ClientManager::ActiveClient().in_speed.active ^ cl_run->integer ) {
			movespeed = 2;
		} else {
			movespeed = 1;
			cmd->buttons |= BUTTON_WALKING;
		}

		if ( ClientManager::ActiveClient().in_speed.active ) {
			anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
		} else {
			anglespeed = 0.001 * cls.frametime;
		}
	}
	else
	{
		if ( in_speed.active ^ cl_run->integer ) {
			movespeed = 2;
		} else {
			movespeed = 1;
			cmd->buttons |= BUTTON_WALKING;
		}

		if ( in_speed.active ) {
			anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
		} else {
			anglespeed = 0.001 * cls.frametime;
		}
	}
*/

/*
	if ( !in_strafe.active ) {
		if ( cl_mYawOverride )
		{
			cl->viewangles[YAW] += cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * cl->joystickAxis[AXIS_SIDE]/2.0f;
		}
		else
		{
			cl->viewangles[YAW] += anglespeed * (cl_yawspeed->value / 100.0f) * cl->joystickAxis[AXIS_SIDE];
		}
	} else
*/
	{
		cmd->rightmove = ClampChar( cmd->rightmove + cl->joystickAxis[AXIS_SIDE] );
	}

/*
	if ( in_mlooking || cl_freelook->integer ) {
		if ( cl_mPitchOverride )
		{
			cl->viewangles[PITCH] += cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * cl->joystickAxis[AXIS_FORWARD]/2.0f;
		}
		else
		{
			cl->viewangles[PITCH] += anglespeed * (cl_pitchspeed->value / 100.0f) * cl->joystickAxis[AXIS_FORWARD];
		}
	} else
*/
	{
		cmd->forwardmove = ClampChar( cmd->forwardmove + cl->joystickAxis[AXIS_FORWARD] );
	}

//	if( abs( cmd->forwardmove ) < MOVE_RUN &&
//		abs( cmd->rightmove ) < MOVE_RUN )
//		cmd->buttons |= BUTTON_WALKING;

	// Smarter run-speed detection, taking diagonals into account!
	if( (cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove) < (MOVE_RUN * MOVE_RUN) )
		cmd->buttons |= BUTTON_WALKING;

	cmd->upmove = ClampChar( cmd->upmove + cl->joystickAxis[AXIS_UP] );
}

/*
=================
CL_MouseMove
=================
*/
#ifdef _XBOX
void CL_MouseClamp(int *x, int *y)
{
	float ax = Q_fabs(*x);
	float ay = Q_fabs(*y);

	ax = (ax-10)*(3.0f/45.0f) * (ax-10) * (Q_fabs(*x) > 10);
	ay = (ay-10)*(3.0f/45.0f) * (ay-10) * (Q_fabs(*y) > 10);
	if (*x < 0)
		*x = -ax;
	else
		*x = ax;
	if (*y < 0)
		*y = -ay;
	else
		*y = ay;
}
#endif

void CL_MouseMove( usercmd_t *cmd ) {
	float	mx, my;
	float	accelSensitivity;
#ifdef _XBOX
	float   accelSensitivityY;
#endif

	float	rate;
	const float	speed = static_cast<float>(frame_msec);
	const float pitch = ClientManager::ActiveClient().cg_pitch;

#ifdef _XBOX
	const float mouseSpeedX = 0.06f;
	const float mouseSpeedY = 0.05f;

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl->mouseDx[0] + cl->mouseDx[1] ) * 0.5f * frame_msec * mouseSpeedX;
		my = ( cl->mouseDy[0] + cl->mouseDy[1] ) * 0.5f * frame_msec * mouseSpeedY;
	} else {
		int ax = cl->mouseDx[cl->mouseIndex];
		int ay = cl->mouseDy[cl->mouseIndex];
		CL_MouseClamp(&ax, &ay);
		
		mx = ax * speed * mouseSpeedX;
		my = ay * speed * mouseSpeedY;		
	}

//	extern int cg_crossHairStatus;
	const float m_hoverSensitivity = 0.4f;
	if (ClientManager::ActiveClient().cg_crossHairStatus)
	{
		mx *= m_hoverSensitivity;
		my *= m_hoverSensitivity;
	}
#else
	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl->mouseDx[0] + cl->mouseDx[1] ) * 0.5;
		my = ( cl->mouseDy[0] + cl->mouseDy[1] ) * 0.5;
	} else {
		mx = cl->mouseDx[cl->mouseIndex];
		my = cl->mouseDy[cl->mouseIndex];
	}
#endif

	cl->mouseIndex ^= 1;
	cl->mouseDx[cl->mouseIndex] = 0;
	cl->mouseDy[cl->mouseIndex] = 0;

	rate = SQRTFAST( mx * mx + my * my ) / speed;

	if ( cl_mYawOverride || cl_mPitchOverride )
	{//FIXME: different people have different speed mouses,
		if ( cl_mSensitivityOverride )
		{
			//this will fuck things up for them, need to clamp 
			//max input?
			accelSensitivity = cl_mSensitivityOverride;
			accelSensitivityY = cl_mSensitivityOverride;
		}
		else
		{
#ifdef _XBOX
			accelSensitivity = ClientManager::ActiveClient().cg_sensitivity + rate * cl_mouseAccel->value;
			accelSensitivityY = ClientManager::ActiveClient().cg_sensitivityY + rate * cl_mouseAccel->value;
#else
			accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
#endif //_XBOX
			// scale by FOV
			accelSensitivity *= cl->cgameSensitivity;
			accelSensitivityY *= cl->cgameSensitivity;
		}
	}
	else
	{
#ifdef _XBOX
		accelSensitivity = ClientManager::ActiveClient().cg_sensitivity + rate * cl_mouseAccel->value;
		accelSensitivityY = ClientManager::ActiveClient().cg_sensitivityY + rate * cl_mouseAccel->value;
#else
		accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
#endif
		// scale by FOV
		accelSensitivity *= cl->cgameSensitivity;
		accelSensitivityY *= cl->cgameSensitivity;
	}


	if ( rate && cl_showMouseRate->integer ) {
		Com_Printf( "%f : %f\n", rate, accelSensitivity );
	}

	mx *= accelSensitivity;

#ifdef _XBOX
	my *= accelSensitivityY;
#else
	my *= accelSensitivity;
#endif



	if (!mx && !my) {
#ifdef _XBOX
		// If there was a movement but no change in angles then start auto-leveling the camera
		float autolevelSpeed = 0.03f;

		// Get the current client
		ClientData &ac(ClientManager::ActiveClient());

		// Moved tests that differ in split screen up here, to make things easier to read:
		bool firingNow = (ClientManager::splitScreenMode) ?
			(ac.in_buttons[0].active || ac.in_buttons[7].active) :
			(in_buttons[0].active || in_buttons[7].active);

		if (!ac.cg_crossHairStatus &&							// Not looking at an enemy
			cl->joystickAxis[AXIS_FORWARD] &&					// Moving forward/backward
			cl->snap.ps.groundEntityNum != ENTITYNUM_NONE &&	// Not in the air
			ac.cg_autolevel &&									// Autolevel is turned on
			!firingNow &&										// Not firing a weapon
			ac.lastFireTime < Sys_Milliseconds() - 1000)		// Haven't fired recently
		{
			float normAngle = -SHORT2ANGLE(cl->snap.ps.delta_angles[PITCH]);
			// The adjustment to normAngle below is meant to add or remove some multiple
			// of 360, so that normAngle is within 180 of viewangles[PITCH]. It should
			// be correct.
			int diff = (int)(cl->viewangles[PITCH] - normAngle);
			if (diff > 180)
				normAngle += 360.0f * ((diff+180) / 360);
			else if (diff < -180)
				normAngle -= 360.0f * ((-diff+180) / 360);

			if (ac.cg_thirdPerson == 1)
			{
//				normAngle += 10;	// Removed by BTO, 2003/05/14, I hate it
				autolevelSpeed *= 1.5f;
			}
			if (cl->viewangles[PITCH] > normAngle)
			{
				cl->viewangles[PITCH] -= autolevelSpeed * speed;
				if (cl->viewangles[PITCH] < normAngle) cl->viewangles[PITCH] = normAngle;
			}
			else if (cl->viewangles[PITCH] < normAngle)
			{
				cl->viewangles[PITCH] += autolevelSpeed * speed;
				if (cl->viewangles[PITCH] > normAngle) cl->viewangles[PITCH] = normAngle;
			}
		}
#endif
		return;
	}

	// add mouse X/Y movement to cmd
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		in_strafe.active = ClientManager::ActiveClient().in_strafe.active;
		in_mlooking = ClientManager::ActiveClient().in_mlooking;
	}
#endif

	if ( in_strafe.active ) {
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	} else {
		if ( cl_mYawOverride )
		{
			cl->viewangles[YAW] -= cl_mYawOverride * mx;
		}
		else
		{
			cl->viewangles[YAW] -= m_yaw->value * mx;
		}
	}

	if ( (in_mlooking || cl_freelook->integer) && !in_strafe.active ) {
		// VVFIXME - This is supposed to be a CVAR
#ifdef _XBOX
		const float cl_pitchSensitivity = 0.5f;
#else
		const float cl_pitchSensitivity = 1.0f;
#endif
		if ( cl_mPitchOverride )
		{
			if ( pitch > 0 )
			{
				cl->viewangles[PITCH] += cl_mPitchOverride * my * cl_pitchSensitivity;
			}
			else
			{
				cl->viewangles[PITCH] -= cl_mPitchOverride * my * cl_pitchSensitivity;
			}
		}
		else
		{
			cl->viewangles[PITCH] += pitch * my * cl_pitchSensitivity;
		}
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
	}
}

qboolean CL_NoUseableForce(void)
{
	if (!cgvm)
	{ //ahh, no cgame loaded
		return qfalse;
	}

	return (qboolean)VM_Call(cgvm, CG_GET_USEABLE_FORCE);
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
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		for (i = 0 ; i < 15 ; i++) {
			if ( ClientManager::ActiveClient().in_buttons[i].active || ClientManager::ActiveClient().in_buttons[i].wasPressed ) {
				cmd->buttons |= 1 << i;
			}
			ClientManager::ActiveClient().in_buttons[i].wasPressed = qfalse;
		}
	}
	else
	{
#endif
	for (i = 0 ; i < 15 ; i++) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}
#ifdef _XBOX
	}
#endif

	if (cmd->buttons & BUTTON_FORCEPOWER)
	{ //check for transferring a use force to a use inventory...
		if ((cmd->buttons & BUTTON_USE) || CL_NoUseableForce())
		{ //it's pushed, remap it!
			cmd->buttons &= ~BUTTON_FORCEPOWER;
			cmd->buttons |= BUTTON_USE_HOLDABLE;
		}
	}

	// if this is siege make BUTTON_FORCEPOWER trigger BUTTON_USE_HOLDABLE as well
	// this assumes that no player can have both items and force powers
#ifdef _XBOX
	if( (cmd->buttons & BUTTON_FORCEPOWER ) &&	// is a force power use
		(cgs.gametype == GT_SIEGE) )				// is in siege mode
	{
		cmd->buttons |= BUTTON_USE_HOLDABLE; // set BUTTON_USE_HOLDABLE
	}
#endif

	if ( cls.keyCatchers ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
#ifdef _XBOX
	if ( ClientManager::ActiveClient().anykeydown && !cls.keyCatchers ) {
#else
	if ( kg.anykeydown && !cls.keyCatchers ) {
#endif
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
	cmd->weapon = cl->cgameUserCmdValue;
	cmd->forcesel = cl->cgameForceSelection;
	cmd->invensel = cl->cgameInvenSelection;

	if (cl->gcmdSendValue)
	{
		cmd->generic_cmd = cl->gcmdValue;
		//cl->gcmdSendValue = qfalse;
		cl->gcmdSentValue = qtrue;
	}
	else
	{
		cmd->generic_cmd = 0;
	}

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl->serverTime;
	
	if (cl->cgameViewAngleForceTime > cl->serverTime)
	{
		cl->cgameViewAngleForce[YAW] -= SHORT2ANGLE(cl->snap.ps.delta_angles[YAW]);

		cl->viewangles[YAW] = cl->cgameViewAngleForce[YAW];
		cl->cgameViewAngleForceTime = 0;
	}

	if ( cl_crazyShipControls )
	{
		float pitchSubtract, pitchDelta, yawDelta;

		yawDelta = AngleSubtract(cl->viewangles[YAW],cl_lastViewAngles[YAW]);
		//yawDelta *= (4.0f*pVeh->m_fTimeModifier);
		cl_sendAngles[ROLL] -= yawDelta;

		float nRoll = fabs(cl_sendAngles[ROLL]);

		pitchDelta = AngleSubtract(cl->viewangles[PITCH],cl_lastViewAngles[PITCH]);
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
			cmd->angles[i] = ANGLE2SHORT(cl->viewangles[i]);
		}
		//in case we switch to the cl_crazyShipControls
		VectorCopy( cl->viewangles, cl_sendAngles );
	}
	//always needed in for the cl_crazyShipControls
	VectorCopy( cl->viewangles, cl_lastViewAngles );
}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void ) {
	usercmd_t	cmd;
	vec3_t		oldAngles;

	VectorCopy( cl->viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles ();
	
	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// check to make sure the angles haven't wrapped
	if ( cl->viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl->viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl->viewangles[PITCH] > 90 ) {
		cl->viewangles[PITCH] = oldAngles[PITCH] - 90;
	} 

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs(cl->viewangles[YAW] - oldAngles[YAW]), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs(cl->viewangles[PITCH] - oldAngles[PITCH]), 0 );
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
	usercmd_t	*cmd;
	int			cmdNum;

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//		cls.state = ClientManager::ActiveClient().state;
#endif

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
        CM_START_LOOP();
		cl->cmdNumber++;
		cmdNum = cl->cmdNumber & CMD_MASK;
		cl->cmds[cmdNum] = CL_CreateCmd ();
		cmd = &cl->cmds[cmdNum];
		CM_END_LOOP();
	}
	else
	{
#endif

	cl->cmdNumber++;
	cmdNum = cl->cmdNumber & CMD_MASK;
	cl->cmds[cmdNum] = CL_CreateCmd ();
	cmd = &cl->cmds[cmdNum];

#ifdef _XBOX
	}
#endif
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
#ifdef _XBOX	// No demos on Xbox
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if (ClientManager::ActiveClient().state == CA_CINEMATIC ) {
//			return qfalse;
//		}
//	}
//	else
	if ( cls.state == CA_CINEMATIC ) {
#else
	if ( clc->demoplaying || cls.state == CA_CINEMATIC ) {
#endif
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
#ifndef _XBOX	// No downloads on Xbox
	if ( *clc->downloadTempName &&
		cls.realtime - clc->lastPacketSentTime < 50 ) {
		return qfalse;
	}
#endif

	// if we don't have a valid gamestate yet, only send
	// one packet a second
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if ( ClientManager::ActiveClient().state != CA_ACTIVE &&
//			 ClientManager::ActiveClient().state != CA_PRIMED &&
//		    cls.realtime - clc->lastPacketSentTime < 1000 ) {
//			return qfalse;
//		}
//	}
//	else
//	{
#endif
	if ( cls.state != CA_ACTIVE && 
		cls.state != CA_PRIMED && 
		cls.realtime - clc->lastPacketSentTime < 1000 ) {
		return qfalse;
	}
#ifdef _XBOX
//	}
#endif

	// send every frame for loopbacks
	if ( clc->netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( Sys_IsLANAddress( clc->netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 100 ) {
		Cvar_Set( "cl_maxpackets", "100" );
	}
	oldPacketNum = (clc->netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl->outPackets[ oldPacketNum ].p_realtime;
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
4	clc->serverCommandSequence
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
#ifdef _XBOX	// No demos on Xbox
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if(ClientManager::ActiveClient().state == CA_CINEMATIC)
//		{
//			return;
//		}
//	}
//	else
	if ( cls.state == CA_CINEMATIC ) {
#else
	if ( clc->demoplaying || cls.state == CA_CINEMATIC ) {
#endif
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl->serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc->serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc->serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc->reliableAcknowledge + 1 ; i <= clc->reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc->reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc->netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl->cmdNumber - cl->outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl->snap.valid
#ifndef _XBOX	// No demos on Xbox
			|| clc->demowaiting
#endif
			|| clc->serverMessageSequence != cl->snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc->checksumFeed;
		// also use the message acknowledge
		key ^= clc->serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc->serverCommands[ clc->serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl->cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl->cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		if (cl->gcmdSentValue)
		{ //hmm, just clear here, I guess.. hoping it will resolve issues with gencmd values sometimes not going through.
			cl->gcmdSendValue = qfalse;
			cl->gcmdSentValue = qfalse;
			cl->gcmdValue = 0;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc->netchan.outgoingSequence & PACKET_MASK;
	cl->outPackets[ packetNum ].p_realtime = cls.realtime;
	cl->outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl->outPackets[ packetNum ].p_cmdNumber = cl->cmdNumber;
	clc->lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc->netchan, &buf);	

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	while ( clc->netchan.unsentFragments ) {
		CL_Netchan_TransmitNextFragment( &clc->netchan );
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
#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue)
//	{
//		if(ClientManager::ActiveClient().state < CA_CONNECTED)
//		{
//			return;
//		}
//	}
//	else
//	{
#endif
	if ( cls.state < CA_CONNECTED ) {
		return;
	}
#ifdef _XBOX
//	}
#endif

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
        CM_START_LOOP();
		// don't send a packet if the last packet was sent too recently
		if ( !CL_ReadyToSendPacket() ) {
			if ( cl_showSend->integer ) {
				Com_Printf( ". " );
			}
			continue;
		}

		CL_WritePacket();
		CM_END_LOOP();
	}
	else
	{
#endif
	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();

#ifdef _XBOX
	}
#endif
}


/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	Cmd_AddCommand ("centerview",IN_CenterView);

	//Cmd_AddCommand ("+taunt", IN_Button3Down);//gesture
	//Cmd_AddCommand ("-taunt", IN_Button3Up);
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_Button0Down);
	Cmd_AddCommand ("-attack", IN_Button0Up);
	//Cmd_AddCommand ("+force_jump", IN_Button1Down);//force jump
	//Cmd_AddCommand ("-force_jump", IN_Button1Up);
	Cmd_AddCommand ("+use", IN_Button5Down);
	Cmd_AddCommand ("-use", IN_Button5Up);
	Cmd_AddCommand ("+force_grip", IN_Button6Down);//force grip
	Cmd_AddCommand ("-force_grip", IN_Button6Up);
	Cmd_AddCommand ("+altattack", IN_Button7Down);//altattack
	Cmd_AddCommand ("-altattack", IN_Button7Up);
	Cmd_AddCommand ("+useforce", IN_Button9Down);//active force power
	Cmd_AddCommand ("-useforce", IN_Button9Up);
	Cmd_AddCommand ("+force_lightning", IN_Button10Down);//active force power
	Cmd_AddCommand ("-force_lightning", IN_Button10Up);
	Cmd_AddCommand ("+force_drain", IN_Button11Down);//active force power
	Cmd_AddCommand ("-force_drain", IN_Button11Up);
	//buttons
	Cmd_AddCommand ("+button0", IN_Button0Down);//attack
	Cmd_AddCommand ("-button0", IN_Button0Up);
	Cmd_AddCommand ("+button1", IN_Button1Down);//force jump
	Cmd_AddCommand ("-button1", IN_Button1Up);
	Cmd_AddCommand ("+button2", IN_Button2Down);//use holdable (not used - change to use jedi power?)
	Cmd_AddCommand ("-button2", IN_Button2Up);
	Cmd_AddCommand ("+button3", IN_Button3Down);//gesture
	Cmd_AddCommand ("-button3", IN_Button3Up);
	Cmd_AddCommand ("+button4", IN_Button4Down);//walking
	Cmd_AddCommand ("-button4", IN_Button4Up);
	Cmd_AddCommand ("+button5", IN_Button5Down);//use object
	Cmd_AddCommand ("-button5", IN_Button5Up);
	Cmd_AddCommand ("+button6", IN_Button6Down);//force grip
	Cmd_AddCommand ("-button6", IN_Button6Up);
	Cmd_AddCommand ("+button7", IN_Button7Down);//altattack
	Cmd_AddCommand ("-button7", IN_Button7Up);
	Cmd_AddCommand ("+button8", IN_Button8Down);
	Cmd_AddCommand ("-button8", IN_Button8Up);
	Cmd_AddCommand ("+button9", IN_Button9Down);//active force power
	Cmd_AddCommand ("-button9", IN_Button9Up);
	Cmd_AddCommand ("+button10", IN_Button10Down);//force lightning
	Cmd_AddCommand ("-button10", IN_Button10Up);
	Cmd_AddCommand ("+button11", IN_Button11Down);//force drain
	Cmd_AddCommand ("-button11", IN_Button11Up);
	Cmd_AddCommand ("+button12", IN_Button12Down);
	Cmd_AddCommand ("-button12", IN_Button12Up);
	Cmd_AddCommand ("+button13", IN_Button13Down);
	Cmd_AddCommand ("-button13", IN_Button13Up);
	Cmd_AddCommand ("+button14", IN_Button14Down);
	Cmd_AddCommand ("-button14", IN_Button14Up);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	Cmd_AddCommand ("sv_saberswitch", IN_GenCMD1);
	Cmd_AddCommand ("engage_duel", IN_GenCMD2);
	Cmd_AddCommand ("force_heal", IN_GenCMD3);
	Cmd_AddCommand ("force_speed", IN_GenCMD4);
	Cmd_AddCommand ("force_pull", IN_GenCMD5);
	Cmd_AddCommand ("force_distract", IN_GenCMD6);
	Cmd_AddCommand ("force_rage", IN_GenCMD7);
	Cmd_AddCommand ("force_protect", IN_GenCMD8);
	Cmd_AddCommand ("force_absorb", IN_GenCMD9);
	Cmd_AddCommand ("force_healother", IN_GenCMD10);
	Cmd_AddCommand ("force_forcepowerother", IN_GenCMD11);
	Cmd_AddCommand ("force_seeing", IN_GenCMD12);
	Cmd_AddCommand ("use_seeker", IN_GenCMD13);
	Cmd_AddCommand ("use_field", IN_GenCMD14);
	Cmd_AddCommand ("use_bacta", IN_GenCMD15);
	Cmd_AddCommand ("use_electrobinoculars", IN_GenCMD16);
	Cmd_AddCommand ("zoom", IN_GenCMD17);
	Cmd_AddCommand ("use_sentry", IN_GenCMD18);
	Cmd_AddCommand ("use_jetpack", IN_GenCMD21);
	Cmd_AddCommand ("use_bactabig", IN_GenCMD22);
	Cmd_AddCommand ("use_healthdisp", IN_GenCMD23);
	Cmd_AddCommand ("use_ammodisp", IN_GenCMD24);
	Cmd_AddCommand ("use_eweb", IN_GenCMD25);
	Cmd_AddCommand ("use_cloak", IN_GenCMD26);
	Cmd_AddCommand ("taunt", IN_GenCMD27);
	Cmd_AddCommand ("bow", IN_GenCMD28);
	Cmd_AddCommand ("meditate", IN_GenCMD29);
	Cmd_AddCommand ("flourish", IN_GenCMD30);
	Cmd_AddCommand ("gloat", IN_GenCMD31);
	Cmd_AddCommand ("saberAttackCycle", IN_GenCMD19);
	Cmd_AddCommand ("force_throw", IN_GenCMD20);
#ifdef _XBOX
	Cmd_AddCommand ("+hotswap1", IN_HotSwap1On);
	Cmd_AddCommand ("+hotswap2", IN_HotSwap2On);
	Cmd_AddCommand ("-hotswap1", IN_HotSwap1Off);
	Cmd_AddCommand ("-hotswap2", IN_HotSwap2Off);

	Cmd_AddCommand ("+voicetoggle", IN_VoiceToggleDown);
	Cmd_AddCommand ("-voicetoggle", IN_VoiceToggleUp);
#endif
	Cmd_AddCommand ("useGivenForce", IN_UseGivenForce);


	Cmd_AddCommand("automap_button", IN_AutoMapButton);
	Cmd_AddCommand("automap_toggle", IN_AutoMapToggle);
	Cmd_AddCommand("voicechat", IN_VoiceChatButton);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
}
