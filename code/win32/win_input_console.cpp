// #include "../server/exe_headers.h"

#include "../client/client.h"
//JLF
#include "../client/client_ui.h"


#include "../qcommon/qcommon.h"
#ifdef _JK2MP
#include "../ui/keycodes.h"
#else
#include "../client/keycodes.h"
#endif

#include "win_local.h"
#include "win_input.h"

cvar_t *inSplashMenu = NULL;
cvar_t *controllerOut = NULL;

static void HandleDebugJoystickPress(fakeAscii_t button);

static bool _UIRunning = false;

static bool IN_ControllerMustBePlugged(int controller);

// By default, cheatpad is turned on except in final build. Just change
// here to modify that.
#ifndef FINAL_BUILD
//#define DEBUG_CONTROLLER
#endif

// Controller connection globals
static signed char uiControllerNotification = -1;
bool noControllersConnected = false;
bool wasPlugged[4];

int mainControllerDelayedUnplug = 0;

PadInfo _padInfo; // gamepad thumbstick buffer

/**********************************************************
*
* CHEAT FUNCTIONS
*
**********************************************************/

bool Cheat_God( void )
{
	Cbuf_ExecuteText( EXEC_APPEND, "god\n" );
	return true;
}

void Cheat_GiveAll( void )
{
	Cbuf_ExecuteText(EXEC_APPEND, "give all\n");
}
extern bool Cheat_ChangeSaber( void );	// In wp_saber.cpp

#include "../game/anims.h"
#include "../cgame/cg_local.h"
static bool isDeathAnimation( int anim )
{
	switch( anim )
	{
	case BOTH_DEATH1:			//# First Death anim
	case BOTH_DEATH2:			//# Second Death anim
	case BOTH_DEATH3:			//# Third Death anim
	case BOTH_DEATH4:			//# Fourth Death anim
	case BOTH_DEATH5:			//# Fifth Death anim
	case BOTH_DEATH6:			//# Sixth Death anim
	case BOTH_DEATH7:			//# Seventh Death anim
	case BOTH_DEATH8:			//# 
	case BOTH_DEATH9:			//# 
	case BOTH_DEATH10:			//# 
	case BOTH_DEATH11:			//#
	case BOTH_DEATH12:			//# 
	case BOTH_DEATH13:			//# 
	case BOTH_DEATH14:			//# 
	case BOTH_DEATH14_UNGRIP:	//# Desann's end death (cin #35)
	case BOTH_DEATH14_SITUP:	//# Tavion sitting up after having been thrown (cin #23)
	case BOTH_DEATH15:			//# 
	case BOTH_DEATH16:			//# 
	case BOTH_DEATH17:			//# 
	case BOTH_DEATH18:			//# 
	case BOTH_DEATH19:			//# 
	case BOTH_DEATH20:			//# 
	case BOTH_DEATH21:			//# 
	case BOTH_DEATH22:			//# 
	case BOTH_DEATH23:			//# 
	case BOTH_DEATH24:			//# 
	case BOTH_DEATH25:			//# 
	case BOTH_DEATHFORWARD1:	//# First Death in which they get thrown forward
	case BOTH_DEATHFORWARD2:	//# Second Death in which they get thrown forward
	case BOTH_DEATHFORWARD3:	//# Tavion's falling in cin# 23
	case BOTH_DEATHBACKWARD1:	//# First Death in which they get thrown backward
	case BOTH_DEATHBACKWARD2:	//# Second Death in which they get thrown backward
	case BOTH_DEATH1IDLE:		//# Idle while close to death
	case BOTH_LYINGDEATH1:		//# Death to play when killed lying down
	case BOTH_STUMBLEDEATH1:	//# Stumble forward and fall face first death
	case BOTH_FALLDEATH1:		//# Fall forward off a high cliff and splat death - start
	case BOTH_FALLDEATH1INAIR:	//# Fall forward off a high cliff and splat death - loop
	case BOTH_FALLDEATH1LAND:	//# Fall forward off a high cliff and splat death - hit bottom
	case BOTH_DEAD1:			//# First Death finished pose
	case BOTH_DEAD2:			//# Second Death finished pose
	case BOTH_DEAD3:			//# Third Death finished pose
	case BOTH_DEAD4:			//# Fourth Death finished pose
	case BOTH_DEAD5:			//# Fifth Death finished pose
	case BOTH_DEAD6:			//# Sixth Death finished pose
	case BOTH_DEAD7:			//# Seventh Death finished pose
	case BOTH_DEAD8:			//# 
	case BOTH_DEAD9:			//# 
	case BOTH_DEAD10:			//# 
	case BOTH_DEAD11:			//#
	case BOTH_DEAD12:			//# 
	case BOTH_DEAD13:			//# 
	case BOTH_DEAD14:			//# 
	case BOTH_DEAD15:			//# 
	case BOTH_DEAD16:			//# 
	case BOTH_DEAD17:			//# 
	case BOTH_DEAD18:			//# 
	case BOTH_DEAD19:			//# 
	case BOTH_DEAD20:			//# 
	case BOTH_DEAD21:			//# 
	case BOTH_DEAD22:			//# 
	case BOTH_DEAD23:			//# 
	case BOTH_DEAD24:			//# 
	case BOTH_DEAD25:			//# 
	case BOTH_DEADFORWARD1:		//# First thrown forward death finished pose
	case BOTH_DEADFORWARD2:		//# Second thrown forward death finished pose
	case BOTH_DEADBACKWARD1:	//# First thrown backward death finished pose
	case BOTH_DEADBACKWARD2:	//# Second thrown backward death finished pose
	case BOTH_LYINGDEAD1:		//# Killed lying down death finished pose
	case BOTH_STUMBLEDEAD1:		//# Stumble forward death finished pose
	case BOTH_FALLDEAD1LAND:	//# Fall forward and splat death finished pose
	case BOTH_DEADFLOP1:		//# React to being shot from First Death finished pose
	case BOTH_DEADFLOP2:		//# React to being shot from Second Death finished pose
	case BOTH_DISMEMBER_HEAD1:	//#
	case BOTH_DISMEMBER_TORSO1:	//#
	case BOTH_DISMEMBER_LLEG:	//#
	case BOTH_DISMEMBER_RLEG:	//#
	case BOTH_DISMEMBER_RARM:	//#
	case BOTH_DISMEMBER_LARM:	//#
	case BOTH_DEATH_ROLL:		//# Death anim from a roll
	case BOTH_DEATH_FLIP:		//# Death anim from a flip
	case BOTH_DEATH_SPIN_90_R:	//# Death anim when facing 90 degrees right
	case BOTH_DEATH_SPIN_90_L:	//# Death anim when facing 90 degrees left
	case BOTH_DEATH_SPIN_180:	//# Death anim when facing backwards
	case BOTH_DEATH_LYING_UP:	//# Death anim when lying on back
	case BOTH_DEATH_LYING_DN:	//# Death anim when lying on front
	case BOTH_DEATH_FALLING_DN:	//# Death anim when falling on face
	case BOTH_DEATH_FALLING_UP:	//# Death anim when falling on back
	case BOTH_DEATH_CROUCHED:	//# Death anim when crouched
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool Cheat_WinLevel( void )
{
	// Do not do this while any UI is active. It's dangerous:
	extern int Key_GetCatcher( void );
	if( Key_GetCatcher() == KEYCATCH_UI )
		return false;

	// Also, don't do it during cutscenes:
	extern bool in_camera;
	if( in_camera || ( g_entities[0].client &&  isDeathAnimation(g_entities[0].client->ps.legsAnim)))
		return false;


	// All maps (except kor2) have a trigger item named end_level
	Cbuf_ExecuteText( EXEC_APPEND, "use end_level\n" );

	return true;
}

#ifndef XBOX_DEMO
bool Cheat_LevelSelect( void )
{
	// Set cvar that enables the level select menu:
	Cvar_SetValue( "ui_levelselect", 1 );

	return true;
}
#endif

extern bool Cheat_InfiniteForce( void ); // In wp_saber.cpp

#if YELLOW_MODE
extern bool enableYellowMode;
char yellowModeLevel = 0;
bool enableYellowStage2 = false;

bool Cheat_Yellow_Stage2( void )
{
	if(!enableYellowStage2)
		return false;

	if(IN_GetMainController() != 0)
		return false;

	if( g_entities[0].client &&  isDeathAnimation(g_entities[0].client->ps.legsAnim))
		enableYellowMode = true;
	else
		enableYellowMode = false;

	return enableYellowMode;
}

bool Cheat_Yellow_Stage1( void )
{
	if(IN_GetMainController() != 1)
		return false;

	if(yellowModeLevel	== 0)		// step one, in a cutscene
	{
		extern bool in_camera;
		if(in_camera)
		{
			yellowModeLevel++;
			return true;
		}
	}
	else if( yellowModeLevel == 1 ) // step two, during the splash screen
	{
		if(inSplashMenu->integer)
		{
			yellowModeLevel++;
			return true;
		}
	}
	else if( yellowModeLevel == 2 ) // step three, while force hud is active
	{
		if(cg.forceHUDActive)
		{
			yellowModeLevel++;
			return true;
		}
	}

	if(yellowModeLevel == 3)		// eveything is ok, now reset and move to stage 2
	{
		enableYellowStage2 = true;
		yellowModeLevel = 4;
		return true;
	}

	return false;
}

#endif // YELLOW_MODE

bool Cheat_AllForce( void )
{
	// Do not do this while the force config UI is running. That causes SERIOUS problems:
	extern bool UI_ForceConfigUIActive( void );
	if( UI_ForceConfigUIActive() )
		return false;

	// Set all Light powers to level 3:
	Cbuf_ExecuteText( EXEC_APPEND, "setForceHeal 3\nsetMindTrick 3\nsetForceProtect 3\nsetForceAbsorb 3\n" );
	// Set all Dark powers to level 3:
	Cbuf_ExecuteText( EXEC_APPEND, "setForceGrip 3\nsetForceLightning 3\nsetForceRage 3\nsetForceDrain 3\n" );

	return true;
}

bool Cheat_KungFoo( void )
{
	Cbuf_ExecuteText( EXEC_APPEND, "iknowkungfu\n");
	return qtrue;
}

//
// CHEAT DATA
//
// Every cheat is a sequence of D-pad presses, performed while the right
// thumbstick is being pressed. The cheat sequences are all length 6, and
// each cheat is tied to a void (void) function. Mapping:
// 5 - Up, 7 - Down, 8 - Left, 6 - Right
typedef bool(*xcommandC_t)(void);
struct cheat_t
{
	fakeAscii_t	buttons[6];
	xcommandC_t	function;
};

cheat_t cheats[] = {
	{ {A_JOY7, A_JOY5, A_JOY8, A_JOY6, A_JOY7, A_JOY5}, Cheat_God },
	//{ {A_JOY6, A_JOY6, A_JOY8, A_JOY8, A_JOY7, A_JOY5}, Cheat_GiveAll },
	{ {A_JOY7, A_JOY8, A_JOY5, A_JOY7, A_JOY6, A_JOY5}, Cheat_ChangeSaber },
	{ {A_JOY5, A_JOY5, A_JOY7, A_JOY7, A_JOY8, A_JOY6}, Cheat_WinLevel },
#ifndef XBOX_DEMO
	{ {A_JOY6, A_JOY8, A_JOY6, A_JOY7, A_JOY6, A_JOY5}, Cheat_LevelSelect },
#endif
	{ {A_JOY5, A_JOY7, A_JOY5, A_JOY8, A_JOY5, A_JOY6}, Cheat_InfiniteForce },
	{ {A_JOY8, A_JOY7, A_JOY6, A_JOY5, A_JOY7, A_JOY7}, Cheat_AllForce },
//	{ {A_JOY8, A_JOY7, A_JOY6, A_JOY5, A_JOY7, A_JOY8}, Cheat_KungFoo },
#if YELLOW_MODE
	{ {A_JOY5, A_JOY7, A_JOY5, A_JOY8, A_JOY6, A_JOY5}, Cheat_Yellow_Stage1 },
	{ {A_JOY5, A_JOY6, A_JOY8, A_JOY5, A_JOY7, A_JOY7}, Cheat_Yellow_Stage2 },
#endif // YELLOW_MODE
};

const int numCheats = sizeof(cheats) / sizeof(cheats[0]);

bool		enteringCheat = false;
int			cheatLength = 0;
fakeAscii_t	curCheat[6];


//If the Xbox white or black button was held for less than this amount of
//time while a selection bar was up, the user wants to use the button rather
//than reassign it.
#define MAX_WB_HOLD_TIME 500

static fakeAscii_t UIJoy2Key(fakeAscii_t button)
{
	switch(button) {
	// D-Pad
	case A_JOY7:
		return A_CURSOR_DOWN;
	case A_JOY5:
		return A_CURSOR_UP;
	case A_JOY6:
		return A_CURSOR_RIGHT;
	case A_JOY8:
		return A_CURSOR_LEFT;

	// A and B
	case A_JOY15:
		return A_MOUSE1;
	case A_JOY14:
		return A_ESCAPE;

	// X and Y
	case A_JOY16:
		return A_DELETE;
	case A_JOY13:
		return A_BACKSPACE;

	// L and R triggers
	case A_JOY11:
		return A_PAGE_UP;
	case A_JOY12:
		return A_PAGE_DOWN;

	// Start and Back
	case A_JOY4:
		return A_MOUSE1;
	case A_JOY1:
		return A_ESCAPE;

	// White and Black
	case A_JOY9:
		return A_HOME;
	case A_JOY10:
		return A_END;

	// Left and Right thumbstick - do nothing in the UI
	case A_JOY2:
		return A_SPACE;
	case A_JOY3:
		return A_SPACE;
	}

	return A_SPACE; //Invalid button.
}

struct
{
	int button;
	bool pressed;
}	uiKeyQueue[2][5] = {0};
int	uiQueueLen[2] = {0};
int uiLastKeyUpDown[2] = {0};
int uiLastKeyLeftRight[2] = {0};

void IN_UIEmptyQueue()
{
	/// If the ui is not running then this doesn't have any effect
	if (!_UIRunning)
	{
		uiQueueLen[0] = uiQueueLen[1] = 0;
		return;
	}

// BTO - No CM, bypass that logic.
//	for (int i = 0; i < ClientManager::NumClients(); i++)
	for (int i = 0; i < 1; ++i)
	{
//		ClientManager::ActivateClient(i);
		int found = 0;
		int bCancel = 0;
		for (int j = 0; j < uiQueueLen[i]; j++)
		{
			switch (uiKeyQueue[i][j].button)
			{
			case A_CURSOR_DOWN:
			case A_CURSOR_UP:
				if ( found & 2 )	// Was a left/right key pressed already?
					bCancel = 1;
				found |= 1;
				break;
			case A_CURSOR_RIGHT:
			case A_CURSOR_LEFT:
				if ( found & 1 )	// Was an up/down key already pressed?
					bCancel = 1;
				found |= 2;
				break;
			}
		}

		if (!bCancel)				// was it cancelled?
		{
			for (int j = 0; j < uiQueueLen[i]; j++)
			{
				int time = Sys_Milliseconds();
				switch (uiKeyQueue[i][j].button)
				{
				case A_CURSOR_DOWN:
				case A_CURSOR_UP:
					if (uiLastKeyLeftRight[i])
					{
						if (uiLastKeyLeftRight[i] > time)		// don't allow up/down till left/right has enough leway time
						{
							continue;
						}
					}
					uiLastKeyUpDown[i] = time + 150;			/// 250 ms sound right?
					break;
				case A_CURSOR_LEFT:
				case A_CURSOR_RIGHT:
					if (uiLastKeyUpDown[i])
					{
						if (uiLastKeyUpDown[i] > time)		// don't allow up/down till left/right has enough leway time
						{
							continue;
						}
					}
					uiLastKeyLeftRight[i] = time + 150;			/// 250 ms sound right?
					break;
				}
				Sys_QueEvent(0, SE_KEY, uiKeyQueue[i][j].button, uiKeyQueue[i][j].pressed, 0, NULL);
			}
		}
	}

	// Reset the queue
	uiQueueLen[0] = uiQueueLen[1] = 0;
}

// extern void G_DemoKeypress();
// extern void CG_SkipCredits(void);
char	lastControllerUsed = 0;

void IN_CommonJoyPress(int controller, fakeAscii_t button, bool pressed)
{
#ifdef XBOX_DEMO
	// Reset the demo timer so that we don't auto-reboot to CDX
	extern void Demo_TimerKeypress( void );
	Demo_TimerKeypress();
#endif

	lastControllerUsed	= controller;

	// Cheat system hooks. The right thumbstick button has to be held for a cheat:
	if (button == A_JOY3)
	{
		if (pressed)
		{
			// Just pressed the right thumstick in. Reset cheat detector
			enteringCheat = true;
			cheatLength = 0;
		}
		else
		{
			enteringCheat = false;
			if (cheatLength == 6)
			{
				for( int i = 0; i < numCheats; ++i)
				{
					if( memcmp( &cheats[i].buttons[0], &curCheat[0], sizeof(curCheat) ) == 0 )
					{
						if(cheats[i].function())
							S_StartLocalSound( S_RegisterSound( "sound/vehicles/x-wing/s-foil" ), CHAN_AUTO );
					}
				}
			}
		}
	}
	else if (enteringCheat && pressed)
	{
		// Handle all other buttons while entering a cheat
		if (cheatLength == 6 || (button != A_JOY5 && button != A_JOY6 && button != A_JOY7 && button != A_JOY8))
		{
			// If we press too many buttons, or anything but the D-pad, cancel entry:
			enteringCheat = false;
			cheatLength = 0;
		}
		else
		{
			// We pressed a d-pad button, we're entering a cheat, and there's still room
			curCheat[cheatLength++] = button;
		}
	}

	// Check for special cases for map hack
#ifndef FINAL_BUILD
	if (Cvar_VariableIntegerValue("cl_maphack"))
	{
		if (_UIRunning && button == A_JOY11 && pressed)
		{
			// Left trigger -> F1
			Sys_QueEvent( 0, SE_KEY, A_F1, pressed, 0, NULL );
			return;
		}
		else if (_UIRunning && button == A_JOY12 && pressed)
		{
			// Right trigger -> F2
			Sys_QueEvent( 0, SE_KEY, A_F2, pressed, 0, NULL );
			return;
		}
		else if (_UIRunning && button == A_JOY4 && pressed)
		{
			// Start button -> F3
			//IN_SetMainController(controller);
			Sys_QueEvent( 0, SE_KEY, A_F3, pressed, 0, NULL );
			return;
		}
	}
#endif

	if(inSplashMenu->integer)
	{
		// START always works, A only works if the popup isn't shown:
		if(button == A_JOY4 || (button == A_JOY15 && controllerOut->integer < 0))
		{
			Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
		}
		return;
	}

	int controllerout	= controllerOut->integer;
	if(controllerout != -1)
	{
		if(controllerout == controller && (button == A_JOY4))// || button == A_JOY15))
			Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
		return;
	}

	if(IN_GetMainController() == controller )
	{
		// Always map start button to ESCAPE
		if (!_UIRunning && button == A_JOY4 && cls.state != CA_CINEMATIC)
			Sys_QueEvent( 0, SE_KEY, A_ESCAPE, pressed, 0, NULL );

#ifdef DEBUG_CONTROLLER
		if (controller != 3)
#endif
			Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
	}

#ifdef DEBUG_CONTROLLER
	if (controller == 3 && pressed)
	{
		HandleDebugJoystickPress(button);
		return;
	}
#endif
//JLF
	
}

qboolean g_noCheckAxis = qfalse;

/**********
IN_CommonUpdate
Updates thumbstick events based on _padInfo and ui_thumbStickMode
**********/
void IN_CommonUpdate()
{
	extern int Key_GetCatcher( void );
	_UIRunning = Key_GetCatcher() == KEYCATCH_UI;

	// Even in the UI, only the main controller should be able to scroll:
	if( _UIRunning && _padInfo.padId == IN_GetMainController() )
	{
		Sys_QueEvent( 0,
					  SE_MOUSE,
					  (_padInfo.joyInfo[0].x + _padInfo.joyInfo[1].x) *  4.0f,
					  (_padInfo.joyInfo[0].y + _padInfo.joyInfo[1].y) *  -4.0f,
					  0,
					  NULL );
	}
	else if(_padInfo.padId == IN_GetMainController())
	{
		// Find out how to configure the thumbsticks
		//int thumbStickMode = Cvar_Get("ui_thumbStickMode", "0" , 0)->integer;
		int thumbStickMode = cl_thumbStickMode->integer;

		switch(thumbStickMode)
		{
		case 0:
			// Configure left thumbstick to move forward/back & strafe left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_SIDE,	 _padInfo.joyInfo[0].x * 127.0f, 0, NULL );
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_FORWARD, _padInfo.joyInfo[0].y * 127.0f, 0, NULL );

			// Configure right thumbstick for freelook
			Sys_QueEvent( 0, SE_MOUSE, _padInfo.joyInfo[1].x * 48.0f,  _padInfo.joyInfo[1].y * 48.0f, 0, NULL );
			break;
		case 1:
			// Configure left thumbstick for freelook
			Sys_QueEvent( 0, SE_MOUSE, _padInfo.joyInfo[0].x * 48.0f,  _padInfo.joyInfo[0].y * 48.0f, 0, NULL );

			// Configure right thumbstick to move forward/back & strafe left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_SIDE,	 _padInfo.joyInfo[1].x * 127.0f, 0, NULL );
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_FORWARD, _padInfo.joyInfo[1].y * 127.0f, 0, NULL );
			break;
		case 2:
			// Configure left thumbstick to move forward/back & turn left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_FORWARD, _padInfo.joyInfo[0].y * 127.0f, 0, NULL );
			Sys_QueEvent( 0, SE_MOUSE, _padInfo.joyInfo[0].x * 48.0f, 0.0f, 0, NULL );

			// Configure right thumbstick to look up/down & strafe left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_SIDE, _padInfo.joyInfo[1].x * 127.f, 0, NULL );
			Sys_QueEvent( 0, SE_MOUSE, 0.0f,  _padInfo.joyInfo[1].y * 48.0f, 0, NULL );		
			break;
		case 3:
			// Configure left thumbstick to look up/down & strafe left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_SIDE, _padInfo.joyInfo[0].x * 127.f, 0, NULL );
			Sys_QueEvent( 0, SE_MOUSE, 0.0f,  _padInfo.joyInfo[0].y * 48.0f, 0, NULL );
			
			// Configure right thumbstick to move forward/back & turn left/right
			Sys_QueEvent( 0, SE_JOYSTICK_AXIS, AXIS_FORWARD, _padInfo.joyInfo[1].y * 127.0f, 0, NULL );
			Sys_QueEvent( 0, SE_MOUSE, _padInfo.joyInfo[1].x * 48.0f, 0.0f, 0, NULL );		
			break;
		default:
			break;
		}
	}
}

void startsetMainController(int controller)
{

	IN_SetMainController(controller);
	if ( !wasPlugged[controller])
	{
		mainControllerDelayedUnplug = 1 << controller;
	}
}

/*********
IN_DisplayControllerUnplugged
*********/

static void IN_DisplayControllerUnplugged(int controller)
{
	uiControllerNotification = controller;

	bool noControllersConnected	=	!wasPlugged[0] &&
									!wasPlugged[1] &&
									!wasPlugged[2] &&
									!wasPlugged[3];

	if ( !( cls.keyCatchers & KEYCATCH_UI ) ) 
	{
		if ( cls.state == CA_ACTIVE ) 
		{
			if (controller == IN_GetMainController())
			{
				Cvar_SetValue("ControllerOutNum", controller);
				UI_SetActiveMenu( "ingame","noController" );
			}
		}
	}
	else // UI
	{
		if(inSplashMenu->integer && noControllersConnected)
		{
			Cvar_SetValue("ControllerOutNum", 4);
			UI_SetActiveMenu("ui_popup", "noController");
		}
		else if( controller == IN_GetMainController())
		{
			Cvar_SetValue("ControllerOutNum", controller);
			UI_SetActiveMenu("ui_popup", "noController");
		}
	}
// END JLF

}

/*********
IN_ClearControllerUnplugged
*********/
static void IN_ClearControllerUnplugged(void)
{
	uiControllerNotification = -1;

	//TODO Add a call to the UI that removes the controller disconnected
	// message from the screen.
//	VM_Call( uivm, UI_CONTROLLER_UNPLUGGED, false, 0);
}

qboolean CurrentStateIsInteractive()
{
	 if (cls.state == CA_UNINITIALIZED ||
			cls.state ==CA_CONNECTING||
			cls.state ==CA_CONNECTED||
			cls.state ==CA_CHALLENGING||
			cls.state ==CA_PRIMED||
			cls.state ==CA_CINEMATIC ||
			!SG_GameAllowedToSaveHere(qtrue))
		return qfalse;
	 return qtrue;
}

// Magic flag used to avoid popping up the "no controllers" dialog if
// none were present when we booted (but not from MP)
bool hadAController = false;

/*********
IN_ControllerMustBePlugged
*********/
static bool IN_ControllerMustBePlugged(int controller)
{
	if( cls.state == CA_LOADING		||
		cls.state == CA_CONNECTING	||
		cls.state == CA_CONNECTED	||
		cls.state == CA_CHALLENGING	||
		cls.state == CA_PRIMED		||
		cls.state == CA_CINEMATIC)
	{
		return false;
	}

	// If we're at the splash screen, have no controllers anymore, and there
	// was a controller ever inserted into the machine:
	extern bool Sys_QuickStart();
	if( inSplashMenu->integer &&
		!wasPlugged[0] && !wasPlugged[1] &&
		!wasPlugged[2] && !wasPlugged[3] &&
		hadAController )
		return true;

	// If we're at the splash screen, and anything else above is false
	// (we have another controller, or there's never been a controller):
	if( inSplashMenu->integer )
		return false;

	// OK. In all other cases, we need the main controller:
	return (controller == IN_GetMainController());
}


/*********
IN_PadUnplugged
*********/
void IN_PadUnplugged(int controller)
{
	if(wasPlugged[controller])
	{
		Com_Printf("\tController %d unplugged\n",controller); 
	}
//JLF moved
	wasPlugged[controller] = false;

	//IN_CheckForNoControllers();

	if(IN_ControllerMustBePlugged(controller)&& SG_GameAllowedToSaveHere(qtrue))
	{
		//If UI isn't busy, inform it about controller loss.
		if(uiControllerNotification == -1 && Cvar_VariableIntegerValue("ControllerOutNum")<0)
		{
			IN_DisplayControllerUnplugged(controller);
			mainControllerDelayedUnplug &= ~( 1<< controller);
		}
//		else
//			mainControllerDelayedUnplug = 1 << controller;

	}
	else
	{
		if ( controller == IN_GetMainController())
		{
			//store somehow for checking again later
			mainControllerDelayedUnplug = 1 << controller;
		}
	}
//	wasPlugged[controller] = false;



}

/*********
IN_PadPlugged
*********/
void IN_PadPlugged(int controller)
{
	if(!wasPlugged[controller])
	{
		Com_Printf("\tController %d plugged\n",controller); 
	}

	if(IN_ControllerMustBePlugged(controller)&& SG_GameAllowedToSaveHere(qtrue))
	{
		//If UI is dealing with this controller, tell it to stop.
		if(uiControllerNotification == controller || (_UIRunning && cls.state != CA_ACTIVE ))
		{
			IN_ClearControllerUnplugged();
		}
	}
	else
	{
		if (controller == IN_GetMainController())
		{
			//store somehow for checking again later
			mainControllerDelayedUnplug &= ~(1 << controller);
		}
	}

	wasPlugged[controller] = true;
	noControllersConnected = false;
	hadAController = true;
}

/*********
IN_GetMainController
*********/
int IN_GetMainController(void)
{
	return cls.mainGamepad;
}

/*********
IN_SetMainController
*********/
void IN_SetMainController(int id)
{
	cls.mainGamepad = id;
}

/**********************************************************
*
* DEBUGGING CODE
*
**********************************************************/

#ifdef DEBUG_CONTROLLER
static void HandleDebugJoystickPress(fakeAscii_t button)
{
	switch(button) {
	case A_JOY13: // Right pad up (yellow)
		Cbuf_ExecuteText(EXEC_APPEND, "give all\n");
		break;
	case A_JOY16: // Right pad left (blue)
		Cbuf_ExecuteText(EXEC_APPEND, "viewpos\n");
		break;
	case A_JOY14: // Right pad right (red)
		Cbuf_ExecuteText(EXEC_APPEND, "noclip\n");
		break;
	case A_JOY15: // Right pad down (green)
		Cbuf_ExecuteText(EXEC_APPEND, "god\n");
		break;
	case A_JOY4: // Start
		break;
	case A_JOY1: // back
		break;
	case A_JOY2: // Left thumbstick
		extern void Z_CompactStats(void);
		Z_CompactStats();
		break;
	case A_JOY12: // Upper right trigger
		break;
	case A_JOY8: // Left pad left
		break;
	case A_JOY6: // Left pad right
		break;
	case A_JOY5: // Left pad up
		break;
	case A_JOY7: // Left pad down
		break;
	case A_JOY11: // Upper left trigger
		break; 
	case A_JOY9: // White button
		break;
	case A_JOY10: // Black button
		break;
	}
}

#endif

