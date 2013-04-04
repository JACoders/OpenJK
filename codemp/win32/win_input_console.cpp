// #include "../server/exe_headers.h"

#include "../client/client.h"

//JLF
//#include "../client/client_ui.h"

#include "../qcommon/qcommon.h"
#ifdef _JK2MP
#include "../ui/keycodes.h"
#else
#include "../client/keycodes.h"
#endif

#include "win_local.h"
#include "win_input.h"

#include "../cgame/cg_local.h"
#include "../client/cl_data.h"

#include "../qcommon/xb_settings.h"

static void HandleDebugJoystickPress(fakeAscii_t button);

static bool _UIRunning = false;

static bool IN_ControllerMustBePlugged(int controller);

// By default, cheatpad is turned on except in final build. Just change
// here to modify that.
#ifndef FINAL_BUILD
//#define DEBUG_CONTROLLER
#endif

// Controller connection globals
signed char uiControllerNotification = -1;
extern int unpluggedcontrol2;


bool noControllersConnected = false;
bool wasPlugged[4];

int mainControllerDelayedUnplug = 0;


PadInfo _padInfo; // gamepad thumbstick buffer



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
	for (int i = 0; i < ClientManager::NumClients(); i++)
//	for (int i = 0; i < 1; ++i)
	{
		ClientManager::ActivateClient(i);
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
extern int uiClientNum;
extern int uiclientInputClosed;
extern qboolean uiControllerMenu;
extern vmCvar_t ControllerOutNum;

// extern void G_DemoKeypress();
// extern void CG_SkipCredits(void);
void IN_CommonJoyPress(int controller, fakeAscii_t button, bool pressed)
{
	//int clientsClosedBitField;
	int activeClient;
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
		//	IN_SetMainController(controller);
			Sys_QueEvent( 0, SE_KEY, A_F3, pressed, 0, NULL );
			return;
		}
	}
#endif


	// Don't allow any input to pass through if an important controller is
	// disconnected
	int controllerout	= ControllerOutNum.integer;
	if(controllerout != -1)
	{
		if( (controllerout == controller) && (button == A_JOY4) )
			Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
		return;
	}

//JLF
	if (_UIRunning && cls.state != CA_ACTIVE && pressed )
	{
	int clientnum = ClientManager::ActiveClientNum();
	int client1controller;
		if ( ClientManager::splitScreenMode == qtrue)
	{
		int clientnum = ClientManager::ActiveClientNum();
		ClientManager::ActivateClient(0);
		client1controller = ClientManager::ActiveClient().controller;
		if ( controller != client1controller)
		{
			ClientManager::ActivateClient(1);
			ClientManager::SetActiveController(controller);
		}

				ClientManager::SetActiveController(controller);

	ClientManager::ActivateClient(clientnum);
	}
	}

//	clientsClosedBitField = Cvar_Get("clientInputClosed", "0" , 0)->integer;

	activeClient = ClientManager::ActiveClientNum();
	ClientManager::ActivateClient(0);

	if ((uiclientInputClosed & 0x1) && controller == ClientManager::ActiveController()&& UIJoy2Key(button) !=A_ESCAPE)
	{
		ClientManager::ActivateClient(activeClient);
		return;
	}
	else
	{
		if (ClientManager::splitScreenMode == qtrue)
		{ 
			ClientManager::ActivateClient(1);
			if ((uiclientInputClosed & 0x02) && controller == ClientManager::ActiveController())
			{
				ClientManager::ActivateClient(activeClient);
				return;
			}
		}
		else
		{
			if (controller != ClientManager::ActiveController())
			{
				ClientManager::ActivateClient(activeClient);
				return;
			}
		}
	}
	ClientManager::ActivateClient(activeClient);

	if (!uiControllerMenu)
	{
		if (_UIRunning && cls.state == CA_ACTIVE )
		{	if (ClientManager::ActiveClientNum()!= uiClientNum)
			{
				return;
			}
			if (ClientManager::ActiveController() != controller)
			{
				return;
			}
		}
	}

//END JLF

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		// Always map start button to ESCAPE
	//	if (!_UIRunning && controller != ClientManager::Controller(0) && controller != ClientManager::Controller(1))
		if (!_UIRunning && controller != ClientManager::ActiveController())
			return;

		if (!_UIRunning && button == A_JOY4 && cls.state != CA_CINEMATIC)
			Sys_QueEvent( 0, SE_KEY, A_ESCAPE, pressed, 0, NULL );

		Sys_QueEvent( 0, SE_KEY, _UIRunning ? UIJoy2Key(button) : button, pressed, 0, NULL );
	}
	else
#endif // _XBOX


	if(IN_GetMainController() == controller || _UIRunning)
//	if(ClientManager::ActiveController() == controller || _UIRunning)
	
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
	// Splitscreen mode has to have other controllers work
	else if(_padInfo.padId == IN_GetMainController() || ClientManager::splitScreenMode == qtrue)
	{
		// Find out how to configure the thumbsticks
		//int thumbStickMode = Cvar_Get("ui_thumbStickMode", "0" , 0)->integer;
		//int thumbStickMode = ClientManager::ActiveClient().cg_thumbsticks;
		int thumbStickMode = Settings.thumbstickMode[ClientManager::ActiveClientNum()];

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



extern void UI_SetActiveMenu( const char* menuname,const char *menuID );
/*********
IN_DisplayControllerUnplugged
*********/
void IN_DisplayControllerUnplugged(int controller)
{

	int activeclient = ClientManager::ActiveClientNum();

	if ( !( cls.keyCatchers & KEYCATCH_UI ) ) 
	{
		if ( cls.state == CA_ACTIVE ) 
		{

			Cvar_SetValue("ControllerOutNum", controller);
			uiControllerNotification = controller;
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOCONTROLLERINGAME);

		}
		else //picks up controller out while quiting
		{
			if (controller == IN_GetMainController())
			{
				mainControllerDelayedUnplug = 1 << controller;
			}
		}
	}
	else // UI
	{	
		{
			if (ClientManager::splitScreenMode)
			{
				Cvar_SetValue("ControllerOutNum", controller);
				uiControllerNotification = controller;
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOCONTROLLERINGAME);
			}
			else if (controller == IN_GetMainController())
			{
					Cvar_SetValue("ControllerOutNum", controller);
					uiControllerNotification = controller;
					VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOCONTROLLER);
			}
			
		}
	}
// END JLF
}

void CheckForSecondPrompt( void )
{
	if(unpluggedcontrol2 != -1 && ControllerOutNum.integer == -1)
		IN_DisplayControllerUnplugged(unpluggedcontrol2);
}


qboolean CurrentStateIsInteractive()
{
	 if (cls.state == CA_UNINITIALIZED ||
			cls.state ==CA_CONNECTING||
			cls.state ==CA_CONNECTED||
			cls.state ==CA_CHALLENGING||
			cls.state ==CA_PRIMED||
			cls.state ==CA_CINEMATIC  ||
			cls.state ==CA_LOADING)
		return qfalse;
	 return qtrue;
}


/*********
IN_ClearControllerUnplugged
*********/
static void IN_ClearControllerUnplugged(void)
{
	uiControllerNotification = -1;
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

	if(	ClientManager::splitScreenMode == qtrue )
	{
		if( controller == IN_GetMainController() )
			return true;
			
		if( cls.state == CA_ACTIVE && ClientManager::Controller(1) == controller ) 
			return true;
	}
	else if( controller == IN_GetMainController() )
	{
		return true;
	}

	return false;
}



extern void IN_CheckForNoControllers();
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
	Cvar_SetValue("ControllersConnectedCount", wasPlugged[0] + wasPlugged[1] + wasPlugged[2] + wasPlugged[3]);

	//IN_CheckForNoControllers();

	if(IN_ControllerMustBePlugged(controller)/*&& SG_GameAllowedToSaveHere(qtrue)*/)
	{
		//If UI isn't busy, inform it about controller loss.
		if(uiControllerNotification == -1 && Cvar_VariableIntegerValue("ControllerOutNum")<0)
		{
			mainControllerDelayedUnplug &= ~( 1<< controller);
			IN_DisplayControllerUnplugged(controller);
			
		}
		else
		{
			if(ClientManager::splitScreenMode == qtrue)
				if (controller != uiControllerNotification)
					unpluggedcontrol2 = controller;
		}
	}
	else
	{
		if ((noControllersConnected && _UIRunning) || controller == IN_GetMainController() || ( cls.state != CA_DISCONNECTED && ClientManager::splitScreenMode))
		{
			//store somehow for checking again later
			mainControllerDelayedUnplug = 1 << controller;
		}
	}


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

	if(IN_ControllerMustBePlugged(controller)/*&& SG_GameAllowedToSaveHere(qtrue)*/)
	{
		//If UI is dealing with this controller, tell it to stop.
		if((uiControllerNotification == controller)|| (_UIRunning && cls.state != CA_ACTIVE ))
		{
			IN_ClearControllerUnplugged();
		}
		if (unpluggedcontrol2 == controller)
			unpluggedcontrol2 = -1;

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
	Cvar_SetValue("ControllersConnectedCount", wasPlugged[0]+wasPlugged[1] + wasPlugged[2]+wasPlugged[3]);
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
	ClientManager::SetActiveController(id);
	ClientManager::ActivateByControllerId(id);
	ClientManager::SetMainClient(ClientManager::ActiveClientNum());
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

