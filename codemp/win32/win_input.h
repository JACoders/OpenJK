#pragma once

bool IN_ControllersChanged(int inserted[], int removed[]);

#if defined (_XBOX ) || defined (_GAMECUBE)
#define _USE_RUMBLE
#endif

bool	IN_AnyButtonPressed(void);

void	IN_enableRumble( void );
void	IN_disableRumble( void );
bool	IN_usingRumble( void );

int		IN_CreateRumbleScript(int controller, int numStates, bool deleteWhenFinished);
void	IN_DeleteRumbleScript(int whichScript);
void	IN_KillRumbleScript(int whichScript);
void	IN_ExecuteRumbleScript(int whichScript);

bool	IN_AdvanceToNextState(int whichScript);

void	IN_KillRumbleScripts(int controller);
void	IN_KillRumbleScripts( void );

#define IN_CMD_GOTO_XTIMES	-5
#define IN_CMD_GOTO			-6

#define IN_CMD_DEC_ARG2		-7
#define IN_CMD_INC_ARG2		-8
#define IN_CMD_DEC_ARG1		-9
#define IN_CMD_INC_ARG1		-10

#ifdef _XBOX
	#define IN_CMD_DEC_LEFT		-70
	#define IN_CMD_DEC_RIGHT	-71
	#define IN_CMD_INC_LEFT		-72
	#define IN_CMD_INC_RIGHT	-73
#endif


#if defined (_XBOX)			// ----- XBOX --------

int IN_AddRumbleState(int whichScript, int leftSpeed, int rightSpeed, int timeInMs);
int IN_AddEffectFade4(int whichScript, int startLeft, int startRight, int endLeft, int endRight, int timeInMs);
int IN_AddEffectFadeExp6(int whichScript, int startLeft, int startRight, int endLeft, int endRight, char factor, int timeInMs);

#elif defined (_GAMECUBE)	// ---- GAME CUBE ----

#define IN_GCACTION_START		1
#define IN_GCACTION_STOP		2
#define IN_GCACTION_STOPHARD	3
int IN_AddRumbleState(int whichScript, int action, int timeInMs, int arg = 0);

#endif						// ------END IF-------

int		IN_AddRumbleStateSpecial(int whichScript, int action, int arg1, int arg2);
void	IN_KillRumbleState(int whichScript, int index);

void	IN_PauseRumbling(int controller);
void	IN_PauseRumbling( void );

void	IN_UnPauseRumbling(int controller);
void	IN_UnPauseRumbling( void );

void	IN_TogglePauseRumbling(int controller);
void	IN_TogglePauseRumbling( void );
int		IN_GetMainController();
void	IN_SetMainController(int id);

void	IN_PadUnplugged(int controller);
void	IN_PadPlugged(int controller);

void	IN_CommonJoyPress(int controller, fakeAscii_t button, bool pressed);
void	IN_CommonUpdate(void);

#define IN_MAX_JOYSTICKS 2
// Stores gamepad joystick info
struct JoystickInfo
{
	bool valid;
	float x, y;
};

// Stores gamepad id and joysick info
struct PadInfo
{
	JoystickInfo joyInfo[2];
	int padId;
};

// Buffer for gamepad info
extern PadInfo _padInfo;

bool IN_RumbleAdjust(int controller, int left, int right);
void IN_RumbleInit (void);
void IN_RumbleShutdown (void);
void IN_RumbleFrame (void);
