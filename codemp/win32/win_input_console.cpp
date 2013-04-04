// #include "../server/exe_headers.h"

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#ifdef _JK2MP
#include "../ui/keycodes.h"
#else
#include "../client/keycodes.h"
#endif

#include "win_local.h"
#include "win_input.h"


static void HandleDebugJoystickPress(fakeAscii_t button);

static bool _UIRunning = false;

static bool IN_ControllerMustBePlugged(int controller);

#ifdef _DEBUG
bool cheatPadEnabled = 1;
#else
bool cheatPadEnabled = 1;
#endif

// Controller connection globals
static signed char uiControllerNotification = -1;
bool noControllersConnected = false;
bool wasPlugged[4];

PadInfo _padInfo; // gamepad thumbstick buffer



//If the Xbox white or black button was held for less than this amount of
//time while a selection bar was up, the user wants to use the button rather
//than reassign it.
#define MAX_WB_HOLD_TIME 500

static fakeAscii_t UIJoy2Key(fakeAscii_t button)
{
	switch(button) {
	case A_JOY7:
		return A_CURSOR_DOWN;
	case A_JOY5:
		return A_CURSOR_UP;
	case A_JOY6:
		return A_CURSOR_RIGHT;
	case A_JOY8:
		return A_CURSOR_LEFT;
	case A_JOY15:
		return A_MOUSE1;
#ifdef _GAMECUBE
	case A_JOY16:
		return A_ESCAPE;
	case A_JOY14:
		return A_DELETE;
#else
	case A_JOY14:
		return A_ESCAPE;
	case A_JOY16:
		return A_DELETE;
	// Arbitrary choice for X button - need it for passcodes.
	case A_JOY13:
		return A_BACKSPACE;
#endif

	//left and right trigger for scrolling
	case A_JOY11:
		return A_PAGE_UP;
	case A_JOY12:
		return A_PAGE_DOWN;

	// start and back button on xbox
	case A_JOY1:
		//JLF MPMOVED
		return A_ESCAPE;
	case A_JOY2:
	case A_JOY4:
		//JLF MPMOVED
		return A_MOUSE1;
		//return button;

	case A_JOY3:
		return A_MOUSE1;
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
void IN_CommonJoyPress(int controller, fakeAscii_t button, bool pressed)
{
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
			IN_SetMainController(controller);
			Sys_QueEvent( 0, SE_KEY, A_F3, pressed, 0, NULL );
			return;
		}
	}
#endif


	if(IN_GetMainController() == controller || _UIRunning)
	{
		// Always map start button to ESCAPE
		if (!_UIRunning && button == A_JOY4 && cls.state != CA_CINEMATIC)
			Sys_QueEvent( 0, SE_KEY, A_ESCAPE, pressed, 0, NULL );

#ifndef FINAL_BUILD
		if (controller != 3 || !cheatPadEnabled)
#endif
			Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
	}

/*
	if (pressed)
	{
		G_DemoKeypress();
	}

	//Hacky!  Skip the credits if start is pressed.
	if(button == K_JOY4 && pressed) {
		CG_SkipCredits();
	}
*/

#ifndef FINAL_BUILD
	if (controller == 3 && pressed && cheatPadEnabled) {
		HandleDebugJoystickPress(button);
		return;
	}
#endif

/*
	extern int player1ControllerId;
	extern int player2ControllerId;
	extern bool checkForPlayerControllers;
	extern bool controllerUnplugged;


	// If the game isn't started yet
	if ((ClientManager::Shared().cls.cgameStarted == qfalse) &&
		(!controllerUnplugged))
	{
		// and player1 doesnt have a controllerid, then assign client 1 to this controller
		if (player1ControllerId == -1)
		{
			ClientManager::ActivateClient(0);
			if (ClientManager::ActiveController() != controller)
				ClientManager::SetActiveController(controller);
		}
		// player1 has a controller that is different then input recieved, and player2 doesnt have a controller and there are 2 clients
		else if (player1ControllerId != controller && player2ControllerId == -1 && ClientManager::NumClients() > 1)
		{
			ClientManager::ActivateClient(1);
			if (ClientManager::ActiveController() != controller)
				ClientManager::SetActiveController(controller);			
		}
	}

	if (ClientManager::ActivateByControllerId(controller))
	{

	#ifdef _XBOX
		//Check the status of the white or black buttons.
		if (button == K_JOY9) {
			ClientManager::ActiveClient().whiteButtonDown = pressed;
		} else if(button == K_JOY10) {
			ClientManager::ActiveClient().blackButtonDown = pressed;
		}

		//Ignore white/black button presses if inv/force/weap select is up. 
		//This is ugly.  It basically says return if the UI isn't running, if 
		//we got a white or black button, and if the inv/force/weapon select is 
		//running. 
		if(!_UIRunning && 
			(button == K_JOY9 ||  
			button == K_JOY10) && 
			(ClientManager::ActiveClient().cg.inventorySelectTime +  
			WEAPON_SELECT_TIME > ClientManager::ActiveClient().cg.time || 
			ClientManager::ActiveClient().cg.forcepowerSelectTime +  
			WEAPON_SELECT_TIME > ClientManager::ActiveClient().cg.time || 
			ClientManager::ActiveClient().cg.weaponSelectTime +  
			WEAPON_SELECT_TIME > ClientManager::ActiveClient().cg.time)) { 

			if(!pressed) {
				//And it just gets hackier!  Is that a word?  It is now!
				//If we've released the button and it wasn't down too long...
				if((button == K_JOY9 && 
						ClientManager::ActiveClient().whiteButtonHoldTime < 
						MAX_WB_HOLD_TIME) ||
					(button == K_JOY10 &&
						ClientManager::ActiveClient().whiteButtonHoldTime < 
						MAX_WB_HOLD_TIME)) {
					//If we've already let the button press through previously,
					//just send a release message.  Otherwise send both a press
					//and release.
					if(ClientManager::ActiveClient().keys[button].down) {
						Sys_QueEvent( 0, SE_KEY, button, false, 0, NULL );
					} else {
						Sys_QueEvent( 0, SE_KEY, button, true, 0, NULL );
						Sys_QueEvent( 0, SE_KEY, button, false, 0, NULL );
					}
				}
			}
			return; 
		} 
	#endif

		if (!_UIRunning)
		{
			Sys_QueEvent( 0, SE_KEY, button, pressed, 0, NULL );
		}
		else
		{
//			int clientNum = ClientManager::ActiveClientNum();
			int clientNum = 0; // VVFIXME
			int qL = uiQueueLen[clientNum];
			if(qL < 5) {
				uiKeyQueue[clientNum][qL].button = UIJoy2Key(button);
				uiKeyQueue[clientNum][qL].pressed = pressed;
				uiQueueLen[clientNum]++;
			}
			//Sys_QueEvent(0, SE_KEY, UIJoy2Key(button), pressed, 0, NULL);
		}
*/
/*
	}
*/

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

	// if the UI is running, then let all gamepad sticks work, else only main controller
	if(_UIRunning)
		Sys_QueEvent( 0, SE_MOUSE, _padInfo.joyInfo[1].x *  4.0f, _padInfo.joyInfo[1].y *  -4.0f, 0, NULL );
	else if(_padInfo.padId == IN_GetMainController())
	{
		// Find out how to configure the thumbsticks
		int thumbStickMode = Cvar_Get("ui_thumbStickMode", "0" , 0)->integer;

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

/*********
IN_DisplayControllerUnplugged
*********/
static void IN_DisplayControllerUnplugged(int controller)
{
	uiControllerNotification = controller;

	//TODO Add a call to the UI that draws a controller disconnected message
	// on the screen.
//	VM_Call( uivm, UI_CONTROLLER_UNPLUGGED, true, controller);
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

	if(!_UIRunning && controller == IN_GetMainController())
	{
		return true;
	}

	if(noControllersConnected)
	{
		return true;
	}

	return false;
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

	if(IN_ControllerMustBePlugged(controller))
	{
		//If UI isn't busy, inform it about controller loss.
		if(uiControllerNotification == -1)
		{
			IN_DisplayControllerUnplugged(controller);
		}
	}
	wasPlugged[controller] = false;
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

	if(IN_ControllerMustBePlugged(controller))
	{
		//If UI is dealing with this controller, tell it to stop.
		if(uiControllerNotification == controller)
		{
			IN_ClearControllerUnplugged();
		}
	}
	wasPlugged[controller] = true;
	noControllersConnected = false;
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

/*********
IN_SetThumbStickConfig
Sets the thumbstick configuration value
*********/
void IN_SetThumbStickConfig(int configValue)
{
	Cvar_Set("ui_thumbStickMode", va("%i", configValue));
}

/*********
IN_SetButtonConfig
Execs a button configuration script based on configValue
*********/
void IN_SetButtonConfig(int configValue)
{
	// Set the cvar
	Cvar_Set("ui_buttonMode", va("%i", configValue));

	// Exec the script
	char				execString[40];
	sprintf				(execString, "exec cfg\\buttonConfig%i.cfg\n", configValue);
	Cbuf_ExecuteText	(EXEC_NOW, execString);
}

/*********
IN_SetDpadConfig
Execs a dpad configuration script based on configValue
*********/
void IN_SetDpadConfig(int configValue)
{
	// Set the cvar
	Cvar_Set("ui_dpadMode", va("%i", configValue));

	// Exec the script
	char				execString[40];
	sprintf				(execString, "exec cfg\\dpadConfig%i.cfg\n", configValue);
	Cbuf_ExecuteText	(EXEC_NOW, execString);
}

/**********************************************************
*
* DEBUGGING CODE
*
**********************************************************/
bool debugSoundOff;

#ifndef FINAL_BUILD
static void HandleDebugJoystickPress(fakeAscii_t button)
{
	// Super hackalicious crap used below. Please remove this at some point.
	static int curSaberSet = 0;
	static int curPlayerSet = 0;
	static short dpadmode = 0;
	static short buttonmode = 0;
	static short thumbmode = 0;

	switch(button) {
	case A_JOY13: // Right pad up
		Cbuf_ExecuteText(EXEC_APPEND, "give all\n");
		break;
	case A_JOY16: // Right pad left
		Cbuf_ExecuteText(EXEC_APPEND, "viewpos\n");
		break;
	case A_JOY14: // Right pad right
		Cbuf_ExecuteText(EXEC_APPEND, "noclip\n");
		break;
	case A_JOY15: // Right pad down
		Cbuf_ExecuteText(EXEC_APPEND, "god\n");
		break;
	case A_JOY4: // Start
		Cvar_SetValue("m_pitch", -Cvar_VariableValue("m_pitch"));
		break;
	case A_JOY1: // back
		Cvar_SetValue("cl_autolevel", !Cvar_VariableIntegerValue("cl_autolevel"));
		break;
	case A_JOY2: // Left thumbstick
		extern void Z_CompactStats(void);
		Z_CompactStats();
		break;
	case A_JOY12: // Upper right trigger
		Cbuf_ExecuteText(EXEC_APPEND, "load current\n");
		break;
	case A_JOY8: // Left pad left
		thumbmode++;
		if(thumbmode == 4)
		{
			thumbmode = 0;
		}
		IN_SetThumbStickConfig(thumbmode);
		break;
	case A_JOY6: // Left pad right
		dpadmode++;
		if(dpadmode == 4)
		{
			dpadmode = 0;
		}
		IN_SetDpadConfig(0);
		break;
	case A_JOY5: // Left pad up
		buttonmode++;
		if(buttonmode == 4)
		{
			buttonmode = 0;
		}
		IN_SetButtonConfig(buttonmode);
		break;
	case A_JOY7: // Left pad down
		Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
		break;
	case A_JOY11: // Upper left trigger
		// VVFIXME : This is totally bootleg. The above loads current, because the
		// current SG system writes out to "current" and then uses FS to move the file
		// to the name that we specify. Which we don't support. But we can't write to
		// "current" as the SG system doesn't allow it. So we write to some arbitary
		// name, and our game ends up in current.
		Cbuf_ExecuteText(EXEC_APPEND, "save foo\n");
#if 0	// VVFIXME
		extern cvar_t *cl_safezonemask;
		if(cl_safezonemask) {
			if(cl_safezonemask->integer) {
				Cbuf_ExecuteText(EXEC_APPEND, "safezonemask 0\n");
			} else {
				Cbuf_ExecuteText(EXEC_APPEND, "safezonemask 1\n");
			}
		}
#endif
		break;
	case A_JOY9: // White button
		// Hacky. Really hacky. No, hackier than that.
		curSaberSet = (curSaberSet + 1) % 3;	// Number of xsaber strings in config file
		Cbuf_ExecuteText(EXEC_APPEND, va("vstr xsaber%d\n", curSaberSet));
		break;
	case A_JOY10: // Black button
		curPlayerSet = (curPlayerSet + 1) % 6;	// Number of xplayer strings in config file
		Cbuf_ExecuteText(EXEC_APPEND, va("vstr xplayer%d\n", curPlayerSet));
		break;
	}
}

#endif

