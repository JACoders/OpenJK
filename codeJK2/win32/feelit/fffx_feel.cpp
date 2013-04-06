// Filename:-	fffx_Feel.cpp	(Force-Feedback FX)

#include "../../client/client.h"
//#include "stdafx.h"
//#include "resource.h"
#include "fffx_feel.h"

extern cvar_t* js_ffmult;


#define MAX_EFFECTS_IN_COMPOUND 3    // This needs to be at least 3 for now.  I can add array bounds checking later

CFeelEffect* g_pEffects[MAX_CONCURRENT_FFFXs][MAX_EFFECTS_IN_COMPOUND];

void _FeelInitEffects()
{
	for (int i = 0; i < MAX_CONCURRENT_FFFXs; i++)
	{
		for (int j = 0; j < MAX_EFFECTS_IN_COMPOUND; j++)
		{
			g_pEffects[i][j] = NULL;
		}
	}
}

BOOL _FeelCreateEffect(int iSlotNum, ffFX_e fffx, CFeelDevice* pFeelDevice)
{
	BOOL success = TRUE;
	FEELIT_ENVELOPE envelope;
	envelope.dwSize = sizeof(FEELIT_ENVELOPE);

	switch (fffx)
	{
	case fffx_RandomNoise:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												7500 * js_ffmult->value,	// magnitude
												95,	// period
												10000,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_SawtoothDown);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												8300 * js_ffmult->value,	// magnitude
												160,	// period
												10000,	// duration
												9000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][2] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][2]))->InitializePolar(pFeelDevice,
												8300 * js_ffmult->value,	// magnitude
												34,	// period
												10000,	// duration
												31000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
								
			success = FALSE;
		break;
	case fffx_AircraftCarrierTakeOff:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 600000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 750000;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												0,	// angle
												2500,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope

			success = FALSE;
		break;
	case fffx_BasketballDribble:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 40000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 30000;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												18000,	// angle
												150,	// duration
												5000 * js_ffmult->value,	// magnitude
												&envelope))// envelope

			success = FALSE;
		break;
	case fffx_CarEngineIdle:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												2500 * js_ffmult->value,	// magnitude
												50,	// period
												10000,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_ChainsawIdle:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												3600 * js_ffmult->value,	// magnitude
												60,	// period
												1000,	// duration
												9000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												4000 * js_ffmult->value,	// magnitude
												100,	// period
												1000,	// duration
												18000,	// angle
												4000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_ChainsawInAction:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												6700 * js_ffmult->value,	// magnitude
												60,	// period
												1000,	// duration
												9000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												100,	// period
												1000,	// duration
												18000,	// angle
												5000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][2] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][2]))->InitializePolar(pFeelDevice,
												10000 * js_ffmult->value,	// magnitude
												340,	// period
												1000,	// duration
												18000,	// angle
												4000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_DieselEngineIdle:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_SawtoothDown);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												2000 * js_ffmult->value,	// magnitude
												250,	// period
												10000,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												4000 * js_ffmult->value,	// magnitude
												125,	// period
												10000,	// duration
												18000,	// angle
												1500,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Jump:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												500,	// period
												300,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Land:
		envelope.dwAttackLevel = 6000;
		envelope.dwAttackTime = 200000;
		envelope.dwFadeLevel = 3000;
		envelope.dwFadeTime = 50000;

		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												1000 * js_ffmult->value,	// magnitude
												750,	// period
												250,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_MachineGun:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												3500 * js_ffmult->value,	// magnitude
												70,	// period
												1000,	// duration
												0,	// angle
												2500,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Punched:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 0;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 50000;

		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												8000 * js_ffmult->value,	// magnitude
												130,	// period
												70,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_RocketLaunch:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 200000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 0;

		g_pEffects[iSlotNum][1] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												18000,	// angle
												400,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope
			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 300000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 100000;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												0,	// angle
												1000,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope
			success = FALSE;
		break;
	case fffx_SecretDoor:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 400000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 0;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												0,	// angle
												400,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope

			success = FALSE;
		break;
	case fffx_SwitchClick:
		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												18000,	// angle
												50,	// duration
												7000 * js_ffmult->value,	// magnitude
												NULL))// envelope

			success = FALSE;
		break;
	case fffx_WindGust:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												1000,	// period
												500,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_WindShear:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 1500000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 500000;

		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_SawtoothDown);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												10000 * js_ffmult->value,	// magnitude
												80,	// period
												2000,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_Pistol:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												8500 * js_ffmult->value,	// magnitude
												130,	// period
												50,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Shotgun:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												100,	// period
												100,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 100000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 100000;

		g_pEffects[iSlotNum][1] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												0,	// angle
												300,	// duration
												7000 * js_ffmult->value,	// magnitude
												&envelope))// envelope
			success = FALSE;
		break;
	case fffx_Laser1:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												18000,	// angle
												2000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Laser2:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												0,	// angle
												3000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Laser3:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												9000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Laser4:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												7000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												9000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Laser5:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Laser6:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												25,	// period
												1000,	// duration
												0,	// angle
												2000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_OutOfAmmo:
		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												18000,	// angle
												10,	// duration
												6000 * js_ffmult->value,	// magnitude
												NULL))// envelope

			success = FALSE;
		break;
	case fffx_LightningGun:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												1500 * js_ffmult->value,	// magnitude
												250,	// period
												1000,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope

			success = FALSE;
		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												50,	// period
												1000,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_Missile:
		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 500000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 200000;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												0,	// angle
												250,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope

			success = FALSE;
		break;
	case fffx_GatlingGun:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_SawtoothDown);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												100,	// period
												1000,	// duration
												0,	// angle
												1000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;
		break;
	case fffx_ShortPlasma:
		envelope.dwAttackLevel = 7000;
		envelope.dwAttackTime = 250000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 0;

		g_pEffects[iSlotNum][0] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												0,	// angle
												250,	// duration
												0,	// magnitude
												&envelope))// envelope

			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 0;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 250000;

		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												30,	// period
												250,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_PlasmaCannon1:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												5000 * js_ffmult->value,	// magnitude
												500,	// period
												400,	// duration
												18000,	// angle
												-5000,	// offset
												0,	// phase
												NULL))	// envelope

			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 250000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 0;

		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												6000 * js_ffmult->value,	// magnitude
												30,	// period
												250,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_PlasmaCannon2:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												4000 * js_ffmult->value,	// magnitude
												1000,	// period
												800,	// duration
												18000,	// angle
												-4000,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 500000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 0;

		g_pEffects[iSlotNum][1] = new CFeelPeriodic(GUID_Feel_Sine);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												8000 * js_ffmult->value,	// magnitude
												35,	// period
												500,	// duration
												0,	// angle
												0,	// offset
												0,	// phase
												&envelope))	// envelope
			success = FALSE;
		break;
	case fffx_Cannon:
		g_pEffects[iSlotNum][0] = new CFeelPeriodic(GUID_Feel_Square);
		if (!((CFeelPeriodic*)(g_pEffects[iSlotNum][0]))->InitializePolar(pFeelDevice,
												8000 * js_ffmult->value,	// magnitude
												100,	// period
												100,	// duration
												18000,	// angle
												0,	// offset
												0,	// phase
												NULL))	// envelope
			success = FALSE;

		envelope.dwAttackLevel = 0;
		envelope.dwAttackTime = 100000;
		envelope.dwFadeLevel = 0;
		envelope.dwFadeTime = 100000;

		g_pEffects[iSlotNum][1] = new CFeelConstant;
		if (!((CFeelConstant*)(g_pEffects[iSlotNum][1]))->InitializePolar(pFeelDevice,
												0,	// angle
												300,	// duration
												10000 * js_ffmult->value,	// magnitude
												&envelope))// envelope
			success = FALSE;
		break;
	}// switch (fffx)

	// if any effect in the compound failed to initialize, dump the lot
	if (!success)
	{
		for (int i = 0; i < MAX_EFFECTS_IN_COMPOUND; i++)
		{
			if (g_pEffects[iSlotNum][i])
			{
				delete g_pEffects[iSlotNum][i];
				g_pEffects[iSlotNum][i] = NULL;
			}
		}
	}

	return success;
}

BOOL _FeelStartEffect(int iSlotNum, DWORD dwIterations, DWORD dwFlags)
{
	BOOL success = TRUE;

	for (int i = 0; i < MAX_EFFECTS_IN_COMPOUND; i++)
	{
		if (g_pEffects[iSlotNum][i])
		{
			if (!g_pEffects[iSlotNum][i]->Start(dwIterations, dwFlags))
				success = FALSE;
		}
	}

	return success;
}

BOOL _FeelEffectPlaying(int iSlotNum)
{
	DWORD	dwFlags;	
	dwFlags = 0;

	// check to see if any effect within the compound is still playing
	for (int i = 0; i < MAX_EFFECTS_IN_COMPOUND; i++)
	{
		if (g_pEffects[iSlotNum][i])
		{
			g_pEffects[iSlotNum][i]->GetStatus(&dwFlags);
			if (dwFlags & FEELIT_FSTATUS_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

BOOL _FeelStopEffect(int iSlotNum)
{
	BOOL success = TRUE;

	for (int i = 0; i < MAX_EFFECTS_IN_COMPOUND; i++)
	{
		if (g_pEffects[iSlotNum][i])
		{
			if (!g_pEffects[iSlotNum][i]->Stop())
				success = FALSE;
		}
	}

	return success;
}

BOOL _FeelClearEffect(int iSlotNum)
{
	for (int i = 0; i < MAX_EFFECTS_IN_COMPOUND; i++)
	{
		if (g_pEffects[iSlotNum][i])
		{
			delete g_pEffects[iSlotNum][i];
			g_pEffects[iSlotNum][i] = NULL;
		}
	}

	return TRUE;
}


