
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */
#include "../cgame/cg_local.h"
#include "../server/exe_headers.h"

#include "win_local.h"
#include "win_input.h"


//MB #include "../client/cl_data.h"
#include "../game/q_shared.h"

extern qboolean G_ActivePlayerNormal(void);
static int rumble_timer = 0;

cvar_t*	in_shaking_rumble;	// 1 - shaking rumble on, 0 - shaking rumble off

struct rumblestate_t
{
	int timeToStop;
	
	// Right motor speed on Xbox, action type on Gamecube
	int arg1;

	// Left motor speed on Xbox, secondary action type on Gamecube
	int arg2;
};

struct rumblestate_special_t
{
	int code;
	int arg1;
	int arg2;
};

struct rumblescript_t
{
	int nextStateAt;
	
	int controller;
	
	int currentState;
	int usedStates;
	int numStates;

	bool autoDelete;
	rumblestate_t *states;
};

struct rumblestatus_t
{
	bool changed;
	bool killed;
	bool paused;
	int timePaused;
};

#define MAX_RUMBLE_STATES		10
#define MAX_RUMBLE_SCRIPTS		10
#define MAX_RUMBLE_CONTROLLERS	4

// In rumblestate, highest speed for each side takes precidence
// Number of rumble states is fairly small, so a plain array will work fine
static rumblestatus_t rumbleStatus[MAX_RUMBLE_CONTROLLERS];
static rumblescript_t rumbleScripts[MAX_RUMBLE_SCRIPTS];

cvar_t* in_useRumble = NULL;

bool IN_usingRumble( void )
{
	return in_useRumble->integer;
}


// Creates a rumble script with numStates
// Returns -1 on no more room, otherwise an identifier to use for scripts
int IN_CreateRumbleScript(int controller, int numStates, bool deleteWhenFinished)
{
	if (!IN_usingRumble()) return -1;

	if (controller <= -1 || controller >= MAX_RUMBLE_CONTROLLERS) return -1;
	assert (numStates > 0 && numStates < MAX_RUMBLE_STATES);
	
	int i;
	for (i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
	{
		if (rumbleScripts[i].states == 0)
			break;
	}

	if (i == MAX_RUMBLE_SCRIPTS) 
		return -1;		// Ran out of scripts
	

	rumbleScripts[i].autoDelete = deleteWhenFinished;
	rumbleScripts[i].controller = controller;
	rumbleScripts[i].currentState = 0;
	rumbleScripts[i].nextStateAt = 0;
	rumbleScripts[i].numStates = numStates;
	rumbleScripts[i].usedStates = 0;
	rumbleScripts[i].states = new rumblestate_t[numStates];
	memset(rumbleScripts[i].states, 0, sizeof(rumblestate_t) * numStates);
	return i;
}

// A negative time will last until you kill it explicitly
// Returns index, used to kill or change a state in a script
int IN_AddRumbleStateFull(int whichScript, int arg1, int arg2, int timeInMs)
{
	if (!IN_usingRumble()) return -1;

	assert(whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);
	assert(rumbleScripts[whichScript].usedStates < rumbleScripts[whichScript].numStates);
	
	// Get the current state
	rumblescript_t *curScript = &rumbleScripts[whichScript];
	rumblestate_t *curState = &curScript->states[curScript->usedStates];

	curState->arg1 = arg1;
	curState->arg2 = arg2;

	curState->timeToStop = timeInMs;
	return curScript->usedStates++;
}

int IN_AddRumbleState(int whichScript, int leftSpeed, int rightSpeed, int timeInMs)
{
	return IN_AddRumbleStateFull(whichScript, leftSpeed, rightSpeed, timeInMs);
}

int IN_AddRumbleStateSpecial(int whichScript, int action, int arg1, int arg2)
{
	if (!IN_usingRumble()) return -1;
	
	assert(whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);
	assert(rumbleScripts[whichScript].usedStates < rumbleScripts[whichScript].numStates);
	
	// Get the current state
	rumblescript_t *curScript = &rumbleScripts[whichScript];
	rumblestate_special_t *curState = (rumblestate_special_t*)&curScript->states[curScript->usedStates];

	curState->code = action;
	curState->arg1 = arg1;
	curState->arg2 = arg2;
	return curScript->usedStates++;
}

int IN_AddEffectFade4(int whichScript, int startLeft, int startRight, 
					  int endLeft, int endRight, int timeInMs)
{
	const int fadeSmoothness = 50;		// number of ms between updates, smaller is smoother

	int e = IN_AddRumbleState(whichScript, startLeft, startRight, fadeSmoothness);	// Lasts for fadeSmoothness ms

	if (startLeft < endLeft)		// Fade increases
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_LEFT, e, 
			(endLeft - startLeft) * fadeSmoothness / timeInMs);	
	}
	else
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_DEC_LEFT, e, 
			(startLeft - endLeft) * fadeSmoothness / timeInMs);	
	}

	if (startRight < endRight)
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_RIGHT, e, 
			(endRight - startRight) * fadeSmoothness / timeInMs);
	}
	else
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_DEC_RIGHT, e, 
			(startRight - endRight) * fadeSmoothness / timeInMs);
	}

	return IN_AddRumbleStateSpecial(whichScript, IN_CMD_GOTO_XTIMES, 
		e, timeInMs / fadeSmoothness);
}

int IN_AddEffectFadeExp6(int whichScript, int startLeft, int startRight, 
						 int endLeft, int endRight, char factor, int timeInMs)
{
	const int fadeSmoothness = 10;		// number of ms between updates, smaller is smoother

	int state = IN_AddRumbleState(whichScript, startLeft, startRight, fadeSmoothness);	// Lasts for fadeSmoothness ms

	if (startLeft < endLeft)		// Fade increases
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_LEFT, state, 
			(endLeft - startLeft) * fadeSmoothness / timeInMs - 
			(factor / 2) * ( 1 - timeInMs / fadeSmoothness)
			);	
	}
	else
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_DEC_LEFT, state, 
			(startLeft - endLeft) * fadeSmoothness / timeInMs - 
			(factor / 2) * ( 1 - timeInMs / fadeSmoothness)
			);	
	}

	if (startRight < endRight)
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_RIGHT, state, 
			(endRight - startRight) * fadeSmoothness / timeInMs - 
			(factor / 2) * ( 1 - timeInMs / fadeSmoothness)
			);	
	}
	else
	{
		IN_AddRumbleStateSpecial(whichScript, IN_CMD_DEC_RIGHT, state,
			(startRight - endRight) * fadeSmoothness / timeInMs - 
			(factor / 2) * ( 1 - timeInMs / fadeSmoothness)
			);	
	}

	IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_ARG2, state + 1, factor);
	IN_AddRumbleStateSpecial(whichScript, IN_CMD_INC_ARG2, state + 2, factor);
	return IN_AddRumbleStateSpecial(whichScript, IN_CMD_GOTO_XTIMES, 
		state, timeInMs / fadeSmoothness);
}

// Kills a rumble state based on index
void IN_KillRumbleState(int whichScript, int index)
{
	if (!IN_usingRumble()) return;

	assert( whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);
	assert( index < rumbleScripts[whichScript].numStates );

	rumbleScripts[whichScript].states[index].timeToStop = 0;
	rumbleStatus[rumbleScripts[whichScript].controller].changed = true;
}

// Stops the script, if script has autodelete on then it will get deleted, otherwise it will only stop
void IN_KillRumbleScript(int whichScript)
{
	if (!IN_usingRumble()) return;

	assert (whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);

	rumbleScripts[whichScript].nextStateAt = 0;
	if (rumbleScripts[whichScript].autoDelete)
	{
		if (rumbleScripts[whichScript].states)
			delete [] rumbleScripts[whichScript].states;
		rumbleScripts[whichScript].states = 0;
	}

	rumbleStatus[rumbleScripts[whichScript].controller].changed = true;
}

// Stops Rumbling for specific controller
void IN_KillRumbleScripts(int controller)
{
	if (!IN_usingRumble()) return;
	if (controller <= -1 || controller >= MAX_RUMBLE_CONTROLLERS) return;
	if (rumbleStatus[controller].killed == true) return;

	for (int i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
	{
		if (rumbleScripts[i].controller == controller)
			IN_KillRumbleScript(i);
	}

	rumbleStatus[controller].killed = IN_RumbleAdjust(controller, 0, 0);
}

// Stops Rumbling on all controllers
void IN_KillRumbleScripts( void )
{
	if (!IN_usingRumble()) return;

	for (int i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
		IN_KillRumbleScript(i);

	for (int j = 0; j < MAX_RUMBLE_CONTROLLERS; j++)
	{
		if (!rumbleStatus[j].killed)
		{
			rumbleStatus[j].killed = IN_RumbleAdjust(j, 0, 0);
		}
	}
}

void IN_DeleteRumbleScript(int whichScript)
{
	if (!IN_usingRumble()) return;

	assert (whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);

	if (rumbleScripts[whichScript].states)
		delete [] rumbleScripts[whichScript].states;
	rumbleScripts[whichScript].nextStateAt = 0;
	rumbleScripts[whichScript].states = 0;

	rumbleStatus[rumbleScripts[whichScript].controller].changed = true;
}

int IN_RunSpecialScript(int whichScript)
{
	rumblestate_special_t *sp = (rumblestate_special_t*)&rumbleScripts[whichScript].states[rumbleScripts[whichScript].currentState];
	switch (sp->code)
	{
		// updates the current state pointer
		// uses arg1
	case IN_CMD_GOTO:
		rumbleScripts[whichScript].currentState = sp->arg1;
		return rumbleScripts[whichScript].states[sp->arg1].timeToStop;
		break;	
		// does a goto, and decreases count of arg2, until 0
	case IN_CMD_GOTO_XTIMES:
		if (--sp->arg2 >= 0)
		{
			rumbleScripts[whichScript].currentState = sp->arg1;
			return rumbleScripts[whichScript].states[sp->arg1].timeToStop;
		}
		else		// Go onto next cmd
		{
			if (!IN_AdvanceToNextState(whichScript))
				return -2;	// Done
			return -1;
		}
		break;

		// Decreasae Arg2 of a State,		sp->arg1 = state, sp->arg2 = amount to decrease arg2 of state by
	case IN_CMD_DEC_ARG2:
		{
			rumblestate_special_t *temp = (rumblestate_special_t*)&rumbleScripts[whichScript].states[sp->arg1];
			temp->arg2 -= sp->arg2;
		}
		break;

		// Increase Arg2 of a State,		sp->arg1 = state, sp->arg2 = amount to increase arg2 of state by
	case IN_CMD_INC_ARG2:
		{
			rumblestate_special_t *temp = (rumblestate_special_t*)&rumbleScripts[whichScript].states[sp->arg1];
			temp->arg2 += sp->arg2;
		}
		break;

		// Decreasae Arg1 of a State,		sp->arg1 = state, sp->arg2 = amount to decrease arg1 of state by
	case IN_CMD_DEC_ARG1:
		{
			rumblestate_special_t *temp = (rumblestate_special_t*)&rumbleScripts[whichScript].states[sp->arg1];
			temp->arg1 -= sp->arg2;
		}
		break;

		// Increase Arg2 of a State,		sp->arg1 = state, sp->arg2 = amount to increase arg1 of state by
	case IN_CMD_INC_ARG1:
		{
			rumblestate_special_t *temp = (rumblestate_special_t*)&rumbleScripts[whichScript].states[sp->arg1];
			temp->arg1 += sp->arg2;
		}
		break;

	case IN_CMD_DEC_LEFT:
		rumbleScripts[whichScript].states[sp->arg1].arg2 -=  sp->arg2;
		if (rumbleScripts[whichScript].states[sp->arg1].arg2 < 0)
			rumbleScripts[whichScript].states[sp->arg1].arg2 = 0;
		if (rumbleScripts[whichScript].currentState >= rumbleScripts[whichScript].usedStates - 1)
			return -2;	// Done
		return rumbleScripts[whichScript].states[++rumbleScripts[whichScript].currentState].timeToStop;
		break;

	case IN_CMD_DEC_RIGHT:
		rumbleScripts[whichScript].states[sp->arg1].arg1 -=  sp->arg2;
		if (rumbleScripts[whichScript].states[sp->arg1].arg1 < 0)
			rumbleScripts[whichScript].states[sp->arg1].arg1 = 0;
		if (rumbleScripts[whichScript].currentState >= rumbleScripts[whichScript].usedStates - 1)
			return -2;	// Done
		return rumbleScripts[whichScript].states[++rumbleScripts[whichScript].currentState].timeToStop;
		break;

	case IN_CMD_INC_LEFT:
		rumbleScripts[whichScript].states[sp->arg1].arg2 +=  sp->arg2;
		if (rumbleScripts[whichScript].states[sp->arg1].arg2 > 65534)
			rumbleScripts[whichScript].states[sp->arg1].arg2 = 65534;
		if (rumbleScripts[whichScript].currentState >= rumbleScripts[whichScript].usedStates - 1)
			return -2;	// Done
		return rumbleScripts[whichScript].states[++rumbleScripts[whichScript].currentState].timeToStop;
		break;

	case IN_CMD_INC_RIGHT:
		rumbleScripts[whichScript].states[sp->arg1].arg1 += sp->arg2;
		if (rumbleScripts[whichScript].states[sp->arg1].arg1 > 65534)
			rumbleScripts[whichScript].states[sp->arg1].arg1 = 65534;
		if (rumbleScripts[whichScript].currentState >= rumbleScripts[whichScript].usedStates - 1)
			return -2;	// Done
		return rumbleScripts[whichScript].states[++rumbleScripts[whichScript].currentState].timeToStop;
		break;
	}
	return 0;
}

int IN_Time()
{
	//mb return ClientManager::ActiveClient().cg.time;
	return cg.time;
}

int testTime;

void IN_ExecuteRumbleScript(int whichScript)
{
	if (!IN_usingRumble()) return;

	assert (whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS);
	
	// Can't execute an empty script???
	assert (rumbleScripts[whichScript].usedStates > 0);

	rumbleScripts[whichScript].currentState = 0;
	int cmd = rumbleScripts[whichScript].states[rumbleScripts[whichScript].currentState].timeToStop;
	if (cmd < 0)
	{
		cmd = IN_RunSpecialScript(whichScript);
	}
	
	rumbleScripts[whichScript].nextStateAt = -1;//IN_Time() + cmd;
	
	rumbleStatus[rumbleScripts[whichScript].controller].changed = true;
	rumbleStatus[rumbleScripts[whichScript].controller].killed = false;

	testTime = IN_Time();
}



void IN_PauseRumbling(int controller)
{ 
	if (!IN_usingRumble()) return;
	if (controller <= -1 || controller >= MAX_RUMBLE_CONTROLLERS) return;
	if (rumbleStatus[controller].paused == true) return;

	rumbleStatus[controller].timePaused = IN_Time();
	rumbleStatus[controller].paused = IN_RumbleAdjust(controller, 0, 0);
	IN_KillRumbleScripts();
}

void IN_UnPauseRumbling(int controller)
{
	if (!IN_usingRumble()) return;
	if (controller <= -1 || controller >= MAX_RUMBLE_CONTROLLERS) return;
	
	// can't unpause a control that wasn't paused
	if (rumbleStatus[controller].paused == false) return;

	int cur_time = IN_Time();
	for (int i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
	{
		if (rumbleScripts[i].controller == controller)
		{
			if (rumbleScripts[i].nextStateAt == 0) continue;
			// update the time to stop based on how long it was paused
			rumbleScripts[i].nextStateAt += (cur_time - rumbleStatus[controller].timePaused);
		}
	}

	rumbleStatus[controller].paused = false; 
	rumbleStatus[controller].changed = true;
	rumbleStatus[controller].killed = false;

	IN_KillRumbleScripts();
}

void IN_TogglePauseRumbling(int controller)
{ 
	if (!IN_usingRumble()) return;
	if (controller <= -1 || controller >= MAX_RUMBLE_CONTROLLERS) return;
	if (rumbleStatus[controller].paused)
		IN_UnPauseRumbling(controller);
	else
		IN_PauseRumbling(controller);
}

// Pauses rumbling on all controllers
void IN_PauseRumbling( void )
{ 
	if (!IN_usingRumble()) return;
	for (int i = 0; i < MAX_RUMBLE_CONTROLLERS; i++)
		IN_PauseRumbling(i);
}

// UnPauses rumbling on all controllers
void IN_UnPauseRumbling( void )
{ 
	if (!IN_usingRumble()) return;
	for (int i = 0; i < MAX_RUMBLE_CONTROLLERS; i++)
		IN_UnPauseRumbling(i);
}

// Toggles Pausing on all controllers
void IN_TogglePauseRumbling( void )
{ 
	if (!IN_usingRumble()) return;
	for (int i = 0; i < MAX_RUMBLE_CONTROLLERS; i++)
		IN_TogglePauseRumbling(i);
}

// Returns false when the end of the script is reached
bool IN_AdvanceToNextState(int whichScript)
{
	assert( whichScript >= 0 && whichScript < MAX_RUMBLE_SCRIPTS );
	
	if (rumbleScripts[whichScript].currentState >= rumbleScripts[whichScript].usedStates - 1)
	{
		// Script is at its end, so kill it( which deletes only if autodelete
		IN_KillRumbleScript(whichScript);
		return false;
	}

	// Advance a state
	rumbleScripts[whichScript].currentState++;

	int cmd = rumbleScripts[whichScript].states[rumbleScripts[whichScript].currentState].timeToStop;
	while (cmd < 0)
	{
		cmd = IN_RunSpecialScript(whichScript);
		if (cmd == -1) return true;
		if (cmd == -2) return false;
	}

	rumbleScripts[whichScript].nextStateAt = IN_Time() + cmd;
	return true;
}

// Max rumble takes precidence
// Other possibility is some kind of sum of all the speeds 
// Call this once a frame, to update the controller based on the rumble states
extern qboolean _UI_IsFullscreen( void );
void IN_UpdateRumbleFromStates()
{
	if(_UI_IsFullscreen())
	{
		IN_KillRumbleScripts();
		return;
	}


	int i;
	int value[MAX_RUMBLE_CONTROLLERS][2];		
	int cur_time = IN_Time();
	bool canKillScripts = false;

	memset(value, 0, sizeof(int)*MAX_RUMBLE_CONTROLLERS*2);
	for (i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
	{
		// If rumble is paused on current controller than skip this rumble state
		if ( rumbleStatus[rumbleScripts[i].controller].paused) continue;

//*mb	ClientManager::ActivateByControllerId(rumbleScripts[i].controller);
		if ( !IN_usingRumble() ) 
		{
			IN_KillRumbleScript(i);
			continue;
		}
/*mb
		if (!ClientManager::ActiveGentity() || !G_ActivePlayerNormal())
		{
			IN_KillRumbleScript(i);
			continue;
		}
*/
		// Unset state so skip
		if ( rumbleScripts[i].nextStateAt == 0) continue;

		canKillScripts	= true;

		if ( rumbleScripts[i].nextStateAt == -1)
		{
			int cmd = rumbleScripts[i].states[rumbleScripts[i].currentState].timeToStop;
			rumbleScripts[i].nextStateAt = cur_time + cmd;
		}

		// Time is up on this rumble state
		if ( rumbleScripts[i].nextStateAt < cur_time) 
		{
			// If timeToStop is < cur_time and > 0 then end this state otherwise (negative number) always rumble 
			if (rumbleScripts[i].nextStateAt > 0) 
			{	
				rumbleStatus[rumbleScripts[i].controller].changed = true;
				rumbleStatus[rumbleScripts[i].controller].killed = false;
				if (!IN_AdvanceToNextState(i))		// Returns false if reached the end of script
					continue;
			}
		}

		rumblescript_t *curScript = &rumbleScripts[i];

		if (value[curScript->controller][0] < curScript->states[curScript->currentState].arg2)
			value[curScript->controller][0] = curScript->states[curScript->currentState].arg2;
		if (value[curScript->controller][1] < curScript->states[curScript->currentState].arg1)
			value[curScript->controller][1] = curScript->states[curScript->currentState].arg1;
	}
	
	// Go through the 4 controller ports
	for (i = 0; i < MAX_RUMBLE_CONTROLLERS; i++) 
	{
		// paused, so do nothing for this controller
		if ( rumbleStatus[i].paused) continue;

		// Only update the actual hardware if a state has changed
		if (!rumbleStatus[i].changed) continue;
		
		IN_RumbleAdjust(i, value[i][0], value[i][1]);
		
		// State has changed
		rumbleStatus[i].changed = false;
	}

	if(canKillScripts)
	{
		if( (cur_time - rumble_timer) > 5000 )
			IN_KillRumbleScripts();
	}
	else
	{
		rumble_timer	= cur_time;
	}

}



/*
==================
IN_RumbleInit
==================
*/
void IN_RumbleInit (void) {
	memset(&rumbleStatus, 0, sizeof(rumblestatus_t)*MAX_RUMBLE_CONTROLLERS);
	memset(&rumbleScripts, 0, sizeof(rumblescript_t)*MAX_RUMBLE_SCRIPTS);

	in_useRumble = Cvar_Get( "in_useRumble", "1", 0 );
	in_shaking_rumble	= Cvar_Get("in_shaking_rumble", "1", 0);
}


/*
==================
IN_RumbleShutdown
==================
*/
void IN_RumbleShutdown (void) {
	for (int i = 0; i < MAX_RUMBLE_SCRIPTS; i++)
	{
		if (rumbleScripts[i].states)
			delete [] rumbleScripts[i].states;
		rumbleScripts[i].states = 0;
		rumbleScripts[i].nextStateAt = 0;
	}
}


/*
==================
IN_RumbleFrame
==================
*/
void IN_RumbleFrame (void)
{
	// Check to see if we need to pause rumbling
	if(cl_paused->integer && !rumbleStatus[IN_GetMainController()].paused)
	{
		IN_PauseRumbling(IN_GetMainController());
	}
	else if(!cl_paused->integer && rumbleStatus[IN_GetMainController()].paused)
	{
		IN_UnPauseRumbling(IN_GetMainController());
	}
	
	// Update the states
	IN_UpdateRumbleFromStates();
}
