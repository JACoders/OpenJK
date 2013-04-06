// Filename:-	fffx.cpp	(Force-Feedback FX)
//
// (Function names with "_FF_" beginnings are my internal stuff only, "FF_" beginnings are for external stuff)
//
#define INITGUID	// this will need removing if already defined in someone else's module. Only one must exist in whole game

#include "../../client/client.h"
#include "../win_local.h"
#include "ffc.h"
#include "fffx_feel.h"

// these now MUST default to NULL...
//
CFeelDevice		*g_pFeelDevice=NULL;
CFeelProject	*g_pFeelProject=NULL;
CFeelSpring		*g_pFeelSpring=NULL;


ffFX_e	ffFXLoaded[MAX_CONCURRENT_FFFXs];

//extern	HINSTANCE	global_hInstance;
//extern	HWND		cl_hwnd;
extern WinVars_t	g_wv;


extern cvar_t *in_joystick;
cvar_t	*use_ff;
cvar_t	*ff_defaultTension;


void _FF_ClearUsageArray(void);
void _FF_ClearFXSlot(int i);
void _FF_ClearCreatePlayFXSlot(int iSlotNum, ffFX_e fffx);
void _FF_CreatePlayFXSlot(int iSlotNum, ffFX_e fffx);
void _FF_PlayFXSlot(int iSlotNum);



// externally accessed
qboolean FF_IsAvailable(void)
{
	return g_pFeelDevice?TRUE:FALSE;
}

qboolean FF_IsMouse(void)
{
	if (g_pFeelDevice && (g_pFeelDevice->GetDeviceType() == FEEL_DEVICETYPE_MOUSE))
		return TRUE;

	return FALSE;
}


// 4 semi-useful CMD functions...
void CMD_FF_UseMouse(void)
{
	FF_Init(TRUE);
}

void CMD_FF_UseJoy(void)
{
	FF_Init(FALSE);
}

// arg = 0..3
//
void CMD_FF_Tension(void)
{	
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("ff_tension <tension [0..3]> (default = 1)\n");
		return;
	}

	int iTension = atoi(Cmd_Argv(1));
	if (iTension<0 || iTension>3)
	{
		Com_Printf ("ff_tension <tension [0..3]>\n");
		return;
	}

	if (g_pFeelSpring)
	{
		Com_Printf(va("Setting tension %d\n",iTension));
		Cvar_Set(ff_defaultTension->name,va("%d",iTension));
		FF_SetTension(iTension);
	}
	else
	{
		Com_Printf("No spring device\n");
	}
}


typedef struct
{
	char*	psName;
	ffFX_e	eFXNum;
}FFFX_LOOKUP;

#define FFFX_ENTRY(blah) {#blah,(ffFX_e)fffx_ ## blah}
FFFX_LOOKUP FFFX_Lookup[fffx_NUMBEROF]=
{
	FFFX_ENTRY( RandomNoise ),
	FFFX_ENTRY( AircraftCarrierTakeOff ),	// this one is pointless / dumb
	FFFX_ENTRY( BasketballDribble ),
	FFFX_ENTRY( CarEngineIdle ),
	FFFX_ENTRY( ChainsawIdle ),
	FFFX_ENTRY( ChainsawInAction ),
	FFFX_ENTRY( DieselEngineIdle ),
	FFFX_ENTRY( Jump ),
	FFFX_ENTRY( Land ),
	FFFX_ENTRY( MachineGun ),
	FFFX_ENTRY( Punched ),
	FFFX_ENTRY( RocketLaunch ),
	FFFX_ENTRY( SecretDoor ),
	FFFX_ENTRY( SwitchClick ),
	FFFX_ENTRY( WindGust ),
	FFFX_ENTRY( WindShear ),		// also pretty crap
	FFFX_ENTRY( Pistol ),
	FFFX_ENTRY( Shotgun ),
	FFFX_ENTRY( Laser1 ),
	FFFX_ENTRY( Laser2 ),
	FFFX_ENTRY( Laser3 ),
	FFFX_ENTRY( Laser4 ),
	FFFX_ENTRY( Laser5 ),
	FFFX_ENTRY( Laser6 ),
	FFFX_ENTRY( OutOfAmmo ),
	FFFX_ENTRY( LightningGun ),
	FFFX_ENTRY( Missile ),
	FFFX_ENTRY( GatlingGun ),
	FFFX_ENTRY( ShortPlasma ),
	FFFX_ENTRY( PlasmaCannon1 ),
	FFFX_ENTRY( PlasmaCannon2 ),
	FFFX_ENTRY( Cannon )
};

void CMD_FF_Play(void)
{
	if (Cmd_Argc() != 2 && Cmd_Argc() != 3)
	{
		Com_Printf ("ff_play <n> (where n = 0..%d)	||	ff_play name \"fxname\"\n",fffx_NUMBEROF-1);
		return;
	}

	ffFX_e eFX = fffx_NULL;

	if (!Q_stricmp(Cmd_Argv(1),"name"))
	{
		if (Cmd_Argc() != 3)
		{
			Com_Printf ("ff_play <n> (where n = 0..%d)	||	ff_play name \"fxname\"\n",0,fffx_NUMBEROF-1);
			return;
		}

		for (int i=0; i<fffx_NUMBEROF; i++)
		{
			if (!Q_stricmp(FFFX_Lookup[i].psName,Cmd_Argv(2)))
			{
				eFX = FFFX_Lookup[i].eFXNum;
				break;
			}
		}
	}
	else
	{
		eFX = (ffFX_e) atoi(Cmd_Argv(1));
	}

	if (eFX>=0 && eFX<fffx_NUMBEROF)
	{
		Com_Printf ("Playing FFFX # %d...(%s)\n",eFX,FFFX_Lookup[eFX].psName);
		FF_Play(eFX);
	}
}
	

// arg 0..n..10000
void CMD_FF_Spring(void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("ff_spring <0..10000>\n");
		return;
	}

	long lSpring = atoi(Cmd_Argv(1));
	if (lSpring<0 || lSpring>10000)
	{
		Com_Printf ("ff_spring <0..10000>\n");
		return;
	}

	if (g_pFeelSpring)
	{
		Com_Printf(va("Setting spring to %d\n",lSpring));
		FF_SetSpring(lSpring);
	}
	else
	{
		Com_Printf("No spring device\n");
	}
}


// Called once only during .exe lifetime...
//
void FF_Init(qboolean bTryMouseFirst)
{
	FF_Shutdown();

	Cmd_AddCommand ("ff_usemouse",	CMD_FF_UseMouse);
	Cmd_AddCommand ("ff_usejoy",	CMD_FF_UseJoy);
	Cmd_AddCommand ("ff_tension",	CMD_FF_Tension);
	Cmd_AddCommand ("ff_spring",	CMD_FF_Spring);
	Cmd_AddCommand ("ff_play",		CMD_FF_Play);

	// ====================================

	Com_Printf("\n" S_COLOR_CYAN "------- Force Feedback Initialization -------\n");

	// for the moment default to OFF until usage tables are in...
	use_ff				= Cvar_Get ("use_ff", "0", CVAR_ARCHIVE); 
	ff_defaultTension	= Cvar_Get ("ff_defaultTension", "1", CVAR_ARCHIVE); 

	// don't bother initializing if user specifically turned off force feedback...
	//	
	if (!use_ff->value)	
	{
		Com_Printf("...inhibited, not initializing\n\n");
		return;
	}

	Com_Printf("Creating feedback device:\n");

	if ( bTryMouseFirst )
	{
		CFeelMouse* m_pFeelMouse = new CFeelMouse;
		if (m_pFeelMouse)
		{
			if (m_pFeelMouse->Initialize( g_wv.hInstance, g_wv.hWnd))
			{
				g_pFeelDevice = m_pFeelMouse;				
			}
			else
			{
				delete m_pFeelMouse;
				m_pFeelMouse = NULL;
			}
		}
	}

	if (!g_pFeelDevice)
	{
		// try a general DI FF device...
		//
		CFeelDXDevice* m_pFeelDXDevice = new CFeelDXDevice;
		if (m_pFeelDXDevice)
		{
			if (m_pFeelDXDevice->Initialize( g_wv.hInstance, g_wv.hWnd))
			{
				g_pFeelDevice = m_pFeelDXDevice;
			}
			else
			{
				delete m_pFeelDXDevice;
				m_pFeelDXDevice = NULL;
			}
		}
	}
  

//	g_pFeelDevice = CFeelDevice::CreateDevice(g_wv.hInstance, g_wv.hWnd);
	if (!g_pFeelDevice)
	{
		Com_Printf("...no feedback devices found\n");		
		return;
	}
	else
	{
		_FeelInitEffects();

		for (int _i=0; _i<MAX_CONCURRENT_FFFXs; _i++)
		{
			ffFXLoaded[_i] = fffx_NULL;
		}
		
		if (g_pFeelDevice->GetDeviceType() == FEEL_DEVICETYPE_MOUSE)
		{
			Com_Printf("...found FEELit Mouse\n");		
			g_pFeelDevice->UsesWin32MouseServices(FALSE);
		}
		else if (g_pFeelDevice->GetDeviceType() == FEEL_DEVICETYPE_DIRECTINPUT)
		{
			Com_Printf("...found feedback device\n");					
			g_pFeelSpring = new CFeelSpring;
			if (!g_pFeelSpring->Initialize(		g_pFeelDevice,
												2000,	//10000,					// LONG lStiffness = FEEL_SPRING_DEFAULT_STIFFNESS
												10000,	//5000,						// DWORD dwSaturation = FEEL_SPRING_DEFAULT_SATURATION
												1000,	//0,						// DWORD dwDeadband = FEEL_SPRING_DEFAULT_DEADBAND	// must be 0..n..10000
												FEEL_EFFECT_AXIS_BOTH,				// DWORD dwfAxis = FEEL_EFFECT_AXIS_BOTH
												FEEL_SPRING_DEFAULT_CENTER_POINT,	// POINT pntCenter = FEEL_SPRING_DEFAULT_CENTER_POINT
												FEEL_EFFECT_DEFAULT_DIRECTION_X,	// LONG lDirectionX = FEEL_EFFECT_DEFAULT_DIRECTION_X
												FEEL_EFFECT_DEFAULT_DIRECTION_Y,	// LONG lDirectionY = FEEL_EFFECT_DEFAULT_DIRECTION_Y
												TRUE  // TRUE = rel coords, else screen coords // BOOL bUseDeviceCoordinates = FALSE
												)
				)
			{
				Com_Printf("...(no device return spring)\n");
				delete g_pFeelSpring;
				g_pFeelSpring = NULL;
			}
			else
			{
				Com_Printf("...device return spring ok\n");
				FF_SetTension(ff_defaultTension->integer);	// 0..3
			}
		}// if (g_pFeelDevice->GetDeviceType() == FEEL_DEVICETYPE_DIRECTINPUT)
	}
}


// call this at app shutdown... (or when switching controllers)
//
// (also called by FF_Init in case you're switching controllers so do everything
//	as if-protected)
//
void FF_Shutdown(void)
{
	// note the check first before print, since this is called from the init code
	// as well, and it'd be weird to see the sutdown string first...
	//
	if (g_pFeelSpring || g_pFeelDevice)
	{
		Com_Printf("\n" S_COLOR_CYAN "------- Force Feedback Shutdown -------\n");		
	}

	if (g_pFeelSpring)
	{
		Com_Printf("...closing return spring\n");		
		delete g_pFeelSpring;
		g_pFeelSpring = NULL;
	}

	if (g_pFeelDevice)
	{
		Com_Printf("...closing feedback device\n");		
		_FF_ClearUsageArray();
		delete g_pFeelDevice;
		g_pFeelDevice = NULL;
	}

	Cmd_RemoveCommand ("ff_usemouse");
	Cmd_RemoveCommand ("ff_usejoy");
	Cmd_RemoveCommand ("ff_tension");
	Cmd_RemoveCommand ("ff_spring");
	Cmd_RemoveCommand ("ff_play");
}









void FF_EnsurePlaying(ffFX_e fffx)
{
	if (fffx<0 || fffx>=fffx_NUMBEROF)
		return;

	// if user has specifically turned off force feedback at command line,
	//	or is not using the joystick as current input method (though this can be ignored because stick has a hands-on sensor),
	//	then forget it...
	//
	if (!use_ff->value)
		return; 			


	if (FF_IsAvailable())
	{
		// Have we already got this FF FX loaded?
		//
		for (int i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i]==fffx)
			{
				// yes, is it playing now?
				//
				if (_FeelEffectPlaying(i))				
					return;
				
				_FF_PlayFXSlot(i);
				return;
			}
		}

	// if we got this far then it ain't even loaded, so...
	//
	FF_Play(fffx);
	
	}// if (FF_IsAvailable())

}// void FF_EnsurePlaying(ffFX_e fffx)
			









// strictly speaking I suppose I should check that the current player is actually using a 
//	joystick as his current input method before sending an FF FX to it, but considering the stick
//	will only move if he's holding it then he's daft to hold it and still use the keyboard or 
//	something, so for now I'll just leave it in to avoid yet another IF statement...
//
// Since the joystick appears to run out of FX ram very quickly I'll have to use a sort of rolling
//	buffer principle I think...
//
// (watch the mid-func returns if you put anything at the end!)
//
void _FF_Play(ffFX_e fffx)
{
	int i;	

	if (fffx<0 || fffx>=fffx_NUMBEROF)
		return;

	// if user has specifically turned off force feedback at command line,
	//	or is not using the joystick as current input method (though this can be ignored because stick has a hands-on sensor),
	//	then forget it...
	//
	if (!use_ff->value)
		return;

	if (FF_IsAvailable())
	{
		// first, search for an instance of this FF FX that's already loaded, if found, start it off again...
		//
		for (i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i]==fffx)
			{
				_FF_PlayFXSlot(i);
				return;
			}
		}
		// ok, so we didn't find one of these already here, so search for any FF FX that's loaded but
		//	finished playing...
		//
		// (note that I prefer to re-use an existing one before looking for an empty slot so
		//	there's less chance of the create call failing because of lack of j/s ram)
		//
		for (i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i] != fffx_NULL)
			{
				// slot has an effect, has it finished?
				//
				if (!_FeelEffectPlaying(i))
				{
					_FF_ClearCreatePlayFXSlot(i,fffx);
					return;
				}

			}// if (lpDIEffectsCreated[i])
		}// for (i=0; i<MAX_CONCURRENT_FFFXs; i++)
		
		// At this point we haven't found an existing one to re-start, and there are no dead ones to reclaim, so
		//	we just look for an empty slot...
		//						
		for (i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i] == fffx_NULL)
			{
				_FF_CreatePlayFXSlot(i,fffx);
				return;
			}
		}

		// Hmmm, we seem to have several FF FX's playing at once and we need another. Let's just trash one of the slots
		//	and take it over anyway. I'll use the second slot in case the first is some sort of permanent engine throb
		//	or similar...
		//
		_FF_ClearCreatePlayFXSlot(1,fffx);

	}// if (FF_IsAvailable())

}// void FF_Play(ffFX_e fffx)
			

// hack-wrapper around original play() function because the stick keeps going slack!
//
void FF_Play(ffFX_e fffx)
{
	if (FF_IsAvailable())
	{
		_FF_Play(fffx);
		FF_SetTension(ff_defaultTension->integer);
	}
}





void _FF_ClearUsageArray(void)
{
	int i;

	for (i=0; i<MAX_CONCURRENT_FFFXs; i++)		
	{
		_FF_ClearFXSlot(i);		
	}		
}

// called from more than one place, subroutinised to avoid bugs if method changes...
//
void _FF_ClearFXSlot(int i)
{
	if (ffFXLoaded[i] != fffx_NULL)
	{
		_FeelClearEffect(i);
	}
	ffFXLoaded		[i]=fffx_NULL;
}




void _FF_ClearCreatePlayFXSlot(int iSlotNum, ffFX_e fffx)	
{
	// if this slot has a FF FX, zap it...
	//

	if (ffFXLoaded[iSlotNum] != fffx_NULL)
	{
		// if playing, stop it...
		//
		if (_FeelEffectPlaying(iSlotNum))
		{
			if (!_FeelStopEffect(iSlotNum))
			{
				return;
			}
		}
	}


	_FF_ClearFXSlot(iSlotNum);	// so slot is left clear if err creating next ffFX_e
	_FF_CreatePlayFXSlot(iSlotNum,fffx);
}

void _FF_CreatePlayFXSlot(int iSlotNum, ffFX_e fffx)
{
	if (!_FeelCreateEffect(iSlotNum, fffx, g_pFeelDevice))
	{
		return;
	}
	// effect created ok, so record it's type...
	//
	ffFXLoaded[iSlotNum]=fffx;
	//
	// ... and start it playing...
	//
	_FF_PlayFXSlot(iSlotNum);	
}



void _FF_PlayFXSlot(int iSlotNum)
{

#define FFFX_DURATION 1	//  1 = duration (repeat count?) (could be 'INFINITE')
#define FFFX_FLAGS    0	//	0 = special flags

	if (!_FeelStartEffect(iSlotNum, FFFX_DURATION, FFFX_FLAGS))
	{
		return;
	}

}



void FF_StopAll(void)
{
	if (FF_IsAvailable())
	{
		for (int i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i] != fffx_NULL)
			{
				/*BOOL bRes = */_FeelStopEffect(i);
			}
		}
	}
}


void FF_Stop(ffFX_e fffx)
{
	if (fffx<0 || fffx>=fffx_NUMBEROF)
		return;

	if (FF_IsAvailable())
	{
		for (int i=0; i<MAX_CONCURRENT_FFFXs; i++)
		{
			if (ffFXLoaded[i] == fffx)
			{
				/*BOOL bRes = */_FeelStopEffect(i);
			}
		}
	}
}





// 0..n..10000
//
qboolean FF_SetSpring(long lSpring)
{
	static qboolean bFXPlaying = FALSE;

	if (FF_IsAvailable())
	{
		// this plays a spring effect, and set the tension accordingly. Tension of 0 results in an effect-stop call
		//
		if (g_pFeelSpring)
		{
			if (lSpring)
			{
				if (!bFXPlaying && !g_pFeelSpring->Start())
				{
					return FALSE;
				}
				bFXPlaying = TRUE;

				static POINT p={0,0};
				g_pFeelSpring->ChangeParameters(p, lSpring);
			}
			else
			{
				if (bFXPlaying && !g_pFeelSpring->Stop())
				{
					return FALSE;
				}
				bFXPlaying = FALSE;
			}

			return TRUE;
		}
	}

	return FALSE;
}

// tension is 0 (none) to 3 (max)...
//
qboolean FF_SetTension(int iTension)
{	
	static long lSpringValues[4] = {0, 1000, 5000, 10000};

	if (iTension>3)
		iTension=3;
	if (iTension<0)
		iTension=0;	

	return FF_SetSpring(lSpringValues[iTension]);
}	

///////////////////////// eof //////////////////////////////

