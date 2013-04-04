// win_input.c -- win32 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

// leave this as first line for PCH reasons...
//
// #include "../server/exe_headers.h"

#include <xtl.h>
#include "glw_win_dx8.h"

#include "../client/client.h"

#include "../qcommon/qcommon.h"
#ifdef _JK2MP
#include "../ui/keycodes.h"
#else
#include "../client/keycodes.h"
#endif

#include "win_local.h"
#include "win_input.h"

#define IN_MAX_CONTROLLERS 4

void IN_UIEmptyQueue();
void IN_CheckForNoControllers();

struct inputstate_t
{
	struct controller_t
	{
		HANDLE handle;
		XINPUT_STATE state;
		XINPUT_FEEDBACK feedback;
	};
	controller_t controllers[IN_MAX_CONTROLLERS];
};

inputstate_t *in_state = NULL;



/*
=========================================================================

JOYSTICK

=========================================================================
*/
//JLF moved here for multiple access (then not used multiple times. oh well.)
extern bool noControllersConnected;
// Process all the insertions and removals, updating handles and such
void IN_ProcessChanges(DWORD dwInsert, DWORD dwRemove)
{
	for(int port = 0; port < IN_MAX_CONTROLLERS; ++port)
	{
		// Close removals.
		if((1 << port) & dwRemove)
		{
			if ( in_state->controllers[port].handle )
			{
				XInputClose( in_state->controllers[port].handle );
				in_state->controllers[port].handle = 0;
			}
			IN_PadUnplugged(port);

		}

		// Open insertions.
		if( (1 << port) & dwInsert )
		{
			in_state->controllers[port].handle = XInputOpen( XDEVICE_TYPE_GAMEPAD, port, XDEVICE_NO_SLOT, NULL );
			IN_PadPlugged(port);
		}
	}

	return;
}

/*********
IN_CheckForNoControllers()
If there are no controllers plugged in, the UI
is notified so it can display an appropriate
message.
*********/
void IN_CheckForNoControllers()
{
	extern bool noControllersConnected;
	if(!noControllersConnected)
	{
		extern bool wasPlugged[4];
		if( !wasPlugged[0] &&
			!wasPlugged[1] &&
			!wasPlugged[2] &&
			!wasPlugged[3] )
		{
			// Tell the UI that there are no controllers connected
		//	VM_Call( uivm, UI_CONTROLLER_UNPLUGGED, true, -1);
			noControllersConnected = true;
		}
	}
}

/*
=========================================================================

  RUMBLE SUPPORT

=========================================================================
*/

bool IN_RumbleAdjust(int controller, int left, int right)
{
	assert(controller >= 0 && controller < IN_MAX_CONTROLLERS);

	// Get a device handle for the controller.  This may fail.
	HANDLE handle = in_state->controllers[controller].handle;

	if (!handle) return false;
	
	XINPUT_FEEDBACK* fb = &in_state->controllers[controller].feedback;
	
	// If a prior rumble update is still pending, go away
	if (fb->Header.dwStatus == ERROR_IO_PENDING) return false;

	fb->Rumble.wLeftMotorSpeed = left;
	fb->Rumble.wRightMotorSpeed = right;
	
	return ERROR_IO_PENDING == XInputSetState(handle, fb);
}


/*
=========================================================================

=========================================================================
*/

/*
igBool IN_WindowClose(igWindow *window)
{
	SV_Shutdown ("Server quit\n");
	CL_Shutdown ();
	Com_Shutdown ();
	Sys_Quit ();
	return true;
}
*/

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown( void ) {
	IN_RumbleShutdown();

	delete in_state;
	in_state = NULL;
}


/*
===========
IN_Init
===========
*/
void IN_Init( void )
{
		in_state = new inputstate_t;

		// Initialize support for 4 gamepads
		XDEVICE_PREALLOC_TYPE xdpt[] = {
			{XDEVICE_TYPE_GAMEPAD, 4}
		};

    // Initialize the peripherals. We can only ever
	// call XInitDevices once, no matter what.
	static bool bInputInitialized = false;
	if (!bInputInitialized)
		XInitDevices( sizeof(xdpt) / sizeof(XDEVICE_PREALLOC_TYPE), xdpt );
	bInputInitialized = true;

		// Zero all of our data, including handles
		memset(in_state->controllers, 0, sizeof(in_state->controllers));

		// Find out the status of all gamepad ports, then open them
		IN_ProcessChanges( XGetDevices( XDEVICE_TYPE_GAMEPAD ), 0 );

		IN_RumbleInit();
	}

static inline float _joyAxisConvert(SHORT x)
{
	// Change scale
	float y = x / 32767.0;

	// Cheesy deadzone
	if(fabs(y) < 0.25f)
	{
		y = 0.0f;
	}

	return y;
}

// How many controls on the xbox gamepad?
#define IN_NUM_DIGITAL_BUTTONS 8
#define IN_NUM_ANALOG_BUTTONS 8
// Cutoff where the analog buttons are considered to be "pressed"
// This should be smarter.
#define IN_ANALOG_BUTTON_THRESHOLD 64

void IN_UpdateGamepad(int port)
{
	// Lookup table to convert the digital buttons to fakeAscii_t, in mask order
	const fakeAscii_t digitalXlat[IN_NUM_DIGITAL_BUTTONS] = {
		A_JOY5, // DPAD_UP
		A_JOY7, // DPAD_DOWN
		A_JOY8, // DPAD_LEFT
		A_JOY6, // DPAD_RIGHT
		A_JOY4, // Start
		A_JOY1, // Back
		A_JOY2, // Left stick
		A_JOY3  // Right stick
	};

	// Lookup table to convet the analog buttons to fakeAscii_t, in DX order
	const fakeAscii_t analogXlat[IN_NUM_ANALOG_BUTTONS] = {
		A_JOY15, // A
		A_JOY14, // B
		A_JOY16, // X
		A_JOY13, // Y
		A_JOY10, // Black
		A_JOY9,  // White
		A_JOY11, // Left trigger
		A_JOY12  // Right trigger
	};

	// Get new state
	XINPUT_STATE newState;
	XInputGetState( in_state->controllers[port].handle, &newState );

	// Get old state
	XINPUT_STATE &oldState(in_state->controllers[port].state);

	int buttonIdx;
	bool oldPressed, newPressed;

	// Check all digital buttons first
	for (buttonIdx = 0; buttonIdx < IN_NUM_DIGITAL_BUTTONS; ++buttonIdx)
	{
		oldPressed = oldState.Gamepad.wButtons & (1 << buttonIdx);
		newPressed = newState.Gamepad.wButtons & (1 << buttonIdx);

		if (oldPressed != newPressed)
			IN_CommonJoyPress(port, digitalXlat[buttonIdx], newPressed);
	}

	// Now check all analog buttons
	for (buttonIdx = 0; buttonIdx < IN_NUM_ANALOG_BUTTONS; ++buttonIdx)
	{
		oldPressed = oldState.Gamepad.bAnalogButtons[buttonIdx] > IN_ANALOG_BUTTON_THRESHOLD;
		newPressed = newState.Gamepad.bAnalogButtons[buttonIdx] > IN_ANALOG_BUTTON_THRESHOLD;

		if (oldPressed != newPressed)
			IN_CommonJoyPress(port, analogXlat[buttonIdx], newPressed);
	}

	// Update joysticks
	_padInfo.joyInfo[0].x = _joyAxisConvert(newState.Gamepad.sThumbLX);
	_padInfo.joyInfo[0].y = _joyAxisConvert(newState.Gamepad.sThumbLY);
	_padInfo.joyInfo[1].x = _joyAxisConvert(newState.Gamepad.sThumbRX);
	_padInfo.joyInfo[1].y = _joyAxisConvert(newState.Gamepad.sThumbRY);
	_padInfo.joyInfo[0].valid = _padInfo.joyInfo[1].valid = true;
	_padInfo.padId = port;

	// Copy state back
	oldState = newState;

	// Update game
	IN_CommonUpdate();
}

extern qboolean CurrentStateIsInteractive();
extern int mainControllerDelayedUnplug;

extern void startsetMainController(int controller);
extern int gLaunchController;
/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
//extern int ignoreInputTime;
extern vmCvar_t ControllerOutNum;
void IN_Frame (void)
{
	static qboolean first = qtrue;
	if (in_state)
	{
		// First, check for changes in device status (removed/inserted pads)
		DWORD dwInsert, dwRemove;
		if( XGetDeviceChanges( XDEVICE_TYPE_GAMEPAD, &dwInsert, &dwRemove ) )
		{
			IN_ProcessChanges(dwInsert, dwRemove);
		}

		if ( first )
		{
			// We only force the controller to be locked when we came from MP:
			extern bool Sys_QuickStart( void );
			if( Sys_QuickStart() )
			{
				Com_Printf("\tController %d initialized\n", gLaunchController); 
				startsetMainController(gLaunchController);

				// We're bypassing splash menu!
				Cvar_SetValue( "inSplashMenu", 0 );
			}

			// Only do this check once, no matter what:
			first = qfalse;
		}

		if ( mainControllerDelayedUnplug && CurrentStateIsInteractive() && ControllerOutNum.integer < 0)
			IN_ProcessChanges(0, mainControllerDelayedUnplug);

		// Generate callbacks for each controller that's plugged in
		for (int port = 0; port < IN_MAX_CONTROLLERS; ++port)
			if (in_state->controllers[port].handle)
				IN_UpdateGamepad(port);

		IN_UIEmptyQueue();
		IN_RumbleFrame();
	}
}
