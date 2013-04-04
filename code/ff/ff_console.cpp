/*
 * Stubs to allow linking with FF_ fnuctions declared.
 * Brian Osman
 */

//JLFRUMBLE includes modified to avoid typename collision field_t MPSKIPPED
#ifdef _JK2MP
#include "../namespace_begin.h"
#endif
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/keycodes.h"
//#include "../client/client.h"
#include "../client/fffx.h"
#include "../win32/win_input.h"
#ifdef _JK2MP
#include "../namespace_end.h"
#endif

void FF_StopAll(void)
{
	Com_Printf("FF_StopAll: Please implement.\n");
	// Do nothing
}

void FF_Stop(ffFX_e effect)
{
	Com_Printf("FF_Stop: Please implement fffx_id = %i\n",effect);
	// Do nothing
}

void FF_EnsurePlaying(ffFX_e effect)
{
	Com_Printf("FF_EnsurePlaying: Please implement fffx_id = %i\n",effect);
	// Do nothing
}

void FF_Play(ffFX_e effect)
{
	int s;	// script id
	static int const_rumble[2] = {-1}; // script id for constant rumble
	int client;

	// super huge switch for rumble effects
	switch(effect)
	{
	case fffx_AircraftCarrierTakeOff:
	case fffx_BasketballDribble:
	case fffx_CarEngineIdle:
	case fffx_ChainsawIdle:
	case fffx_ChainsawInAction:
	case fffx_DieselEngineIdle:
	case fffx_Jump:
		s = IN_CreateRumbleScript(IN_GetMainController(), 2, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 50000, 10000, 200);
			IN_AddRumbleState(s, 0, 0, 10);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_Land:
		s = IN_CreateRumbleScript(IN_GetMainController(), 2, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 50000, 10000, 200);
			IN_AddRumbleState(s, 0, 0, 10);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_MachineGun:
		s = IN_CreateRumbleScript(IN_GetMainController(), 2, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 56000, 20000, 230);
			IN_AddRumbleState(s, 0, 0, 10);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_Punched:
	case fffx_RocketLaunch:

	case fffx_SecretDoor:
	case fffx_SwitchClick:	// used by saber
		s = IN_CreateRumbleScript(IN_GetMainController(), 1, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 30000, 10000, 120);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_WindGust:
	case fffx_WindShear:
	case fffx_Pistol:
		s = IN_CreateRumbleScript(IN_GetMainController(), 2, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 50000, 10000, 200);
			IN_AddRumbleState(s, 0, 0, 10);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_Shotgun:
	case fffx_Laser1:
	case fffx_Laser2:
	case fffx_Laser3:
	case fffx_Laser4:
	case fffx_Laser5:
	case fffx_Laser6:
	case fffx_OutOfAmmo:
	case fffx_LightningGun:
	case fffx_Missile:
	case fffx_GatlingGun:
		s = IN_CreateRumbleScript(IN_GetMainController(), 2, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 39000, 0, 220);
			IN_AddRumbleState(s, 0, 0, 10);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_ShortPlasma:
	case fffx_PlasmaCannon1:
	case fffx_PlasmaCannon2:
	case fffx_Cannon:
	case fffx_FallingShort:
	case fffx_FallingMedium:
		s = IN_CreateRumbleScript(IN_GetMainController(), 1, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 25000,10000, 230);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_FallingFar:
		s = IN_CreateRumbleScript(IN_GetMainController(), 1, true);
		if (s != -1)
		{
			IN_AddRumbleState(s, 32000,10000, 230);
			IN_ExecuteRumbleScript(s);
		}
		break;
	case fffx_StartConst:
		client = IN_GetMainController();
		if(const_rumble[client] == -1)
		{
			const_rumble[client] = IN_CreateRumbleScript(IN_GetMainController(), 9, true);
			if (const_rumble[client] != -1)
			{
				IN_AddEffectFade4(const_rumble[client], 0,0, 50000, 0, 2000);
				IN_AddRumbleState(const_rumble[client], 50000, 0, 300);
				IN_AddEffectFade4(const_rumble[client], 50000,50000, 0, 0, 1000);
				IN_ExecuteRumbleScript(const_rumble[client]);
			}
		}
		break;
	case fffx_StopConst:
		client = IN_GetMainController();
		if (const_rumble[client] == -1)
			return;
		IN_KillRumbleScript(const_rumble[client]);
		const_rumble[client] = -1;
		break;
	default:
		Com_Printf("No rumble script is defined for fffx_id = %i\n",effect);
		break;
	}
}

/*********
FF_XboxShake

intensity	- speed of rumble
duration	- length of rumble
*********/
#define	FF_SH_MIN_MOTOR_SPEED		20000
#define	FF_SH_MOTOR_SPEED_MODIFIER	(65535 - FF_SH_MIN_MOTOR_SPEED)
void FF_XboxShake(float intensity, int duration)
{
	int s;
	s = IN_CreateRumbleScript(IN_GetMainController(), 1, true);
	if (s != -1)
	{
		int speed;
		// figure out the speed
		speed = (FF_SH_MIN_MOTOR_SPEED) + (FF_SH_MOTOR_SPEED_MODIFIER * intensity);
		
		// Add the state and execute
		IN_AddRumbleState(s, speed, speed, duration);
		IN_ExecuteRumbleScript(s);
	}
}

/*********
FF_XboxDamage

damage	- Amount of damage
xpos	- x position for the damage ( -1.0 - 1.0 )

The following function various the rumble based upon
the amount of damage and the position of the damage.
*********/
#define FF_DA_MIN_MOTOR_SPEED		20000	// use this to vary the minimum intensity
#define FF_DA_MOTOR_SPEED_MODIFIER	(65535 - FF_DA_MIN_MOTOR_SPEED)
void FF_XboxDamage(int damage, float xpos)
{
	int s;
	s = IN_CreateRumbleScript(IN_GetMainController(), 1, true);
	if (s != -1)
	{
		int leftMotorSpeed;
		int rightMotorSpeed;
		int duration;
		float per;

		duration = 175;

		// how much damage?
		if(damage > 100)
		{
			per = 1.0;
		}
		else
		{
			per = damage/100;
		}
		
		if(xpos >= -0.2 && xpos <= 0.2)	// damge to center
		{
			leftMotorSpeed = rightMotorSpeed = (FF_DA_MIN_MOTOR_SPEED)+(FF_DA_MOTOR_SPEED_MODIFIER * per);
		}
		else if(xpos > 0.2)	// damage to right
		{
			rightMotorSpeed = (FF_DA_MIN_MOTOR_SPEED)+(FF_DA_MOTOR_SPEED_MODIFIER * per);
			leftMotorSpeed = 0;
		}
		else	// damage to left
		{
			leftMotorSpeed = (FF_DA_MIN_MOTOR_SPEED)+(FF_DA_MOTOR_SPEED_MODIFIER * per);;
			rightMotorSpeed = 0;
		}
		
		// Add the state and execute
		IN_AddRumbleState(s, leftMotorSpeed, rightMotorSpeed, duration);
		IN_ExecuteRumbleScript(s);
	}
}

