/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#ifdef _GAME //including game headers on cgame is FORBIDDEN ^_^
	#include "g_local.h"
#endif

#include "bg_public.h"
#include "bg_vehicles.h"

extern float DotToSpot( vec3_t spot, vec3_t from, vec3_t fromAngles );
#ifdef _GAME //SP or gameside MP
	extern vec3_t playerMins;
	extern vec3_t playerMaxs;
	extern void ChangeWeapon( gentity_t *ent, int newWeapon );
	extern int PM_AnimLength( int index, animNumber_t anim );
#endif

extern void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags);
extern int BG_GetTime(void);
extern qboolean BG_SabersOff( playerState_t *ps );

//Alright, actually, most of this file is shared between game and cgame for MP.
//I would like to keep it this way, so when modifying for SP please keep in
//mind the bgEntity restrictions imposed. -rww

#define	STRAFERAM_DURATION	8
#define	STRAFERAM_ANGLE		8

qboolean VEH_StartStrafeRam(Vehicle_t *pVeh, qboolean Right, int Duration)
{
	return qfalse;
}

#ifdef _GAME //game-only.. for now
// Like a think or move command, this updates various vehicle properties.
qboolean Update( Vehicle_t *pVeh, const usercmd_t *pUcmd )
{
	if ( !g_vehicleInfo[VEHICLE_BASE].Update( pVeh, pUcmd ) )
	{
		return qfalse;
	}

	// See whether this vehicle should be exploding.
	if ( pVeh->m_iDieTime != 0 )
	{
		pVeh->m_pVehicleInfo->DeathUpdate( pVeh );
	}

	// Update move direction.

	return qtrue;
}
#endif //_GAME

//MP RULE - ALL PROCESSMOVECOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessMoveCommands the Vehicle.
static void ProcessMoveCommands( Vehicle_t *pVeh )
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/
	//Client sets ucmds and such for speed alterations
	float speedInc, speedIdleDec, speedIdle, speedMin, speedMax;
	playerState_t *parentPS;
//	playerState_t *pilotPS = NULL;
	int	curTime;

	parentPS = pVeh->m_pParentEntity->playerState;
	if (pVeh->m_pPilot)
	{
	//	pilotPS = pVeh->m_pPilot->playerState;
	}

	// If we're flying, make us accelerate at 40% (about half) acceleration rate, and restore the pitch
	// to origin (straight) position (at 5% increments).
	if ( pVeh->m_ulFlags & VEH_FLYING )
	{
		speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier * 0.4f;
	}
	else if ( !parentPS->m_iVehicleNum )
	{//drifts to a stop
		speedInc = 0;
		//pVeh->m_ucmd.forwardmove = 127;
	}
	else
	{
		speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
	}
	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;

#ifdef _GAME
	curTime = level.time;
#elif _CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif



	if ( (pVeh->m_pPilot /*&& (pilotPS->weapon == WP_NONE || pilotPS->weapon == WP_MELEE )*/ &&
		(pVeh->m_ucmd.buttons & BUTTON_ALT_ATTACK) && pVeh->m_pVehicleInfo->turboSpeed)
		/*||
		(parentPS && parentPS->electrifyTime > curTime && pVeh->m_pVehicleInfo->turboSpeed)*/ //make them go!
		)
	{
		if ( (parentPS && parentPS->electrifyTime > curTime) ||
			 (pVeh->m_pPilot->playerState &&
			  (pVeh->m_pPilot->playerState->weapon == WP_MELEE ||
			  (pVeh->m_pPilot->playerState->weapon == WP_SABER && BG_SabersOff( pVeh->m_pPilot->playerState ) ))) )
		{
			if ((curTime - pVeh->m_iTurboTime)>pVeh->m_pVehicleInfo->turboRecharge)
			{
				pVeh->m_iTurboTime = (curTime + pVeh->m_pVehicleInfo->turboDuration);
				if (pVeh->m_pVehicleInfo->iTurboStartFX)
				{
					int i;
					for (i=0; (i<MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i]!=-1); i++)
					{
#ifdef _GAME
						if (pVeh->m_pParentEntity &&
							pVeh->m_pParentEntity->ghoul2 &&
							pVeh->m_pParentEntity->playerState)
						{ //fine, I'll use a tempent for this, but only because it's played only once at the start of a turbo.
							vec3_t boltOrg, boltDir;
							mdxaBone_t boltMatrix;

							VectorSet(boltDir, 0.0f, pVeh->m_pParentEntity->playerState->viewangles[YAW], 0.0f);

							trap->G2API_GetBoltMatrix(pVeh->m_pParentEntity->ghoul2, 0, pVeh->m_iExhaustTag[i], &boltMatrix, boltDir, pVeh->m_pParentEntity->playerState->origin, level.time, NULL, pVeh->m_pParentEntity->modelScale);
							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltDir);
							G_PlayEffectID(pVeh->m_pVehicleInfo->iTurboStartFX, boltOrg, boltDir);
						}
#endif
					}
				}
				parentPS->speed = pVeh->m_pVehicleInfo->turboSpeed;	// Instantly Jump To Turbo Speed
			}
		}
	}

	// Slide Breaking
	if (pVeh->m_ulFlags&VEH_SLIDEBREAKING)
	{
		if (pVeh->m_ucmd.forwardmove>=0 )
		{
			pVeh->m_ulFlags &= ~VEH_SLIDEBREAKING;
		}
		parentPS->speed = 0;
	}
	else if (
		(curTime > pVeh->m_iTurboTime) &&
		!(pVeh->m_ulFlags&VEH_FLYING) &&
		pVeh->m_ucmd.forwardmove<0 &&
		fabs(pVeh->m_vOrientation[ROLL])>25.0f)
	{
		pVeh->m_ulFlags |= VEH_SLIDEBREAKING;
	}


	if ( curTime < pVeh->m_iTurboTime )
	{
		speedMax = pVeh->m_pVehicleInfo->turboSpeed;
		if (parentPS)
		{
			parentPS->eFlags |= EF_JETPACK_ACTIVE;
		}
	}
	else
	{
		speedMax = pVeh->m_pVehicleInfo->speedMax;
		if (parentPS)
		{
			parentPS->eFlags &= ~EF_JETPACK_ACTIVE;
		}
	}


	speedIdle = pVeh->m_pVehicleInfo->speedIdle;
	speedMin = pVeh->m_pVehicleInfo->speedMin;

	if ( parentPS->speed || parentPS->groundEntityNum == ENTITYNUM_NONE  ||
		 pVeh->m_ucmd.forwardmove || pVeh->m_ucmd.upmove > 0 )
	{
		if ( pVeh->m_ucmd.forwardmove > 0 && speedInc )
		{
			parentPS->speed += speedInc;
		}
		else if ( pVeh->m_ucmd.forwardmove < 0 )
		{
			if ( parentPS->speed > speedIdle )
			{
				parentPS->speed -= speedInc;
			}
			else if ( parentPS->speed > speedMin )
			{
				parentPS->speed -= speedIdleDec;
			}
		}
		// No input, so coast to stop.
		else if ( parentPS->speed > 0.0f )
		{
			parentPS->speed -= speedIdleDec;
			if ( parentPS->speed < 0.0f )
			{
				parentPS->speed = 0.0f;
			}
		}
		else if ( parentPS->speed < 0.0f )
		{
			parentPS->speed += speedIdleDec;
			if ( parentPS->speed > 0.0f )
			{
				parentPS->speed = 0.0f;
			}
		}
	}
	else
	{
		if ( !pVeh->m_pVehicleInfo->strafePerc || (0 && pVeh->m_pParentEntity->s.number < MAX_CLIENTS) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			//pVeh->m_ucmd.rightmove = 0;
		}
	}

	if ( parentPS->speed > speedMax )
	{
		parentPS->speed = speedMax;
	}
	else if ( parentPS->speed < speedMin )
	{
		parentPS->speed = speedMin;
	}

	if (parentPS && parentPS->electrifyTime > curTime)
	{
		parentPS->speed *= (pVeh->m_fTimeModifier/60.0f);
	}


	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

//MP RULE - ALL PROCESSORIENTCOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
//Oh, and please, use "< MAX_CLIENTS" to check for "player" and not
//"!s.number", this is a universal check that will work for both SP
//and MP. -rww
// ProcessOrientCommands the Vehicle.
extern void AnimalProcessOri(Vehicle_t *pVeh);
void ProcessOrientCommands( Vehicle_t *pVeh )
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/
	playerState_t *riderPS;
	playerState_t *parentPS;

	float angDif;

	if (pVeh->m_pPilot)
	{
		riderPS = pVeh->m_pPilot->playerState;
	}
	else
	{
		riderPS = pVeh->m_pParentEntity->playerState;
	}
	parentPS = pVeh->m_pParentEntity->playerState;

	//pVeh->m_vOrientation[YAW] = 0.0f;//riderPS->viewangles[YAW];
	angDif = AngleSubtract(pVeh->m_vOrientation[YAW], riderPS->viewangles[YAW]);
	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		float maxDif = pVeh->m_pVehicleInfo->turningSpeed*4.0f; //magic number hackery
		if (s < 0.0f)
		{
			s = -s;
		}
		angDif *= s/pVeh->m_pVehicleInfo->speedMax;
		if (angDif > maxDif)
		{
			angDif = maxDif;
		}
		else if (angDif < -maxDif)
		{
			angDif = -maxDif;
		}
		pVeh->m_vOrientation[YAW] = AngleNormalize180(pVeh->m_vOrientation[YAW] - angDif*(pVeh->m_fTimeModifier*0.2f));

		if (parentPS->electrifyTime > pm->cmd.serverTime)
		{ //do some crazy stuff
			pVeh->m_vOrientation[YAW] += (sin(pm->cmd.serverTime/1000.0f)*3.0f)*pVeh->m_fTimeModifier;
		}
	}

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef _GAME

extern int PM_AnimLength( int index, animNumber_t anim );

// This function makes sure that the vehicle is properly animated.
void AnimateVehicle( Vehicle_t *pVeh )
{
}

#endif //_GAME

//rest of file is shared

//NOTE NOTE NOTE NOTE NOTE NOTE
//I want to keep this function BG too, because it's fairly generic already, and it
//would be nice to have proper prediction of animations. -rww
// This function makes sure that the rider's in this vehicle are properly animated.
void AnimateRiders( Vehicle_t *pVeh )
{
	animNumber_t Anim = BOTH_VS_IDLE;
	int iFlags = SETANIM_FLAG_NORMAL;
	playerState_t *pilotPS;
	int curTime;


	// Boarding animation.
	if ( pVeh->m_iBoarding != 0 )
	{
		// We've just started moarding, set the amount of time it will take to finish moarding.
		if ( pVeh->m_iBoarding < 0 )
		{
			int iAnimLen;

			// Boarding from left...
			if ( pVeh->m_iBoarding == -1 )
			{
				Anim = BOTH_VS_MOUNT_L;
			}
			else if ( pVeh->m_iBoarding == -2 )
			{
				Anim = BOTH_VS_MOUNT_R;
			}
			else if ( pVeh->m_iBoarding == -3 )
			{
				Anim = BOTH_VS_MOUNTJUMP_L;
			}
			else if ( pVeh->m_iBoarding == VEH_MOUNT_THROW_LEFT)
			{
				Anim = BOTH_VS_MOUNTTHROW_R;
			}
			else if ( pVeh->m_iBoarding == VEH_MOUNT_THROW_RIGHT)
			{
				Anim = BOTH_VS_MOUNTTHROW_L;
			}

			// Set the delay time (which happens to be the time it takes for the animation to complete).
			// NOTE: Here I made it so the delay is actually 40% (0.4f) of the animation time.
			iAnimLen = BG_AnimLength( pVeh->m_pPilot->localAnimIndex, Anim ) * 0.4f;
			pVeh->m_iBoarding = BG_GetTime() + iAnimLen;
			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			BG_SetAnim(pVeh->m_pPilot->playerState, bgAllAnims[pVeh->m_pPilot->localAnimIndex].anims,
				SETANIM_BOTH, Anim, iFlags);
		}

		return;
	}

	if (1)
		return;

	pilotPS = pVeh->m_pPilot->playerState;

#ifdef _GAME
	curTime = level.time;
#elif _CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif

	// Going in reverse...
	if ( pVeh->m_ucmd.forwardmove < 0 && !(pVeh->m_ulFlags & VEH_SLIDEBREAKING))
	{
		Anim = BOTH_VS_REV;
	}
	else
	{
		qboolean HasWeapon	= ((pilotPS->weapon != WP_NONE) && (pilotPS->weapon != WP_MELEE));
		qboolean Attacking	= (HasWeapon && !!(pVeh->m_ucmd.buttons&BUTTON_ATTACK));
		qboolean Flying		= qfalse;
		qboolean Crashing	= qfalse;
		qboolean Right		= (pVeh->m_ucmd.rightmove>0);
		qboolean Left		= (pVeh->m_ucmd.rightmove<0);
		qboolean Turbo		= (curTime<pVeh->m_iTurboTime);
		EWeaponPose	WeaponPose	= WPOSE_NONE;


		// Remove Crashing Flag
		//----------------------
		pVeh->m_ulFlags &= ~VEH_CRASHING;


		// Put Away Saber When It Is Not Active
		//--------------------------------------

		// Don't Interrupt Attack Anims
		//------------------------------
		if (pilotPS->weaponTime>0)
		{
			return;
		}

		// Compute The Weapon Pose
		//--------------------------
		if (pilotPS->weapon==WP_BLASTER)
		{
			WeaponPose = WPOSE_BLASTER;
		}
		else if (pilotPS->weapon==WP_SABER)
		{
			if ( (pVeh->m_ulFlags&VEH_SABERINLEFTHAND) && pilotPS->torsoAnim==BOTH_VS_ATL_TO_R_S)
			{
				pVeh->m_ulFlags	&= ~VEH_SABERINLEFTHAND;
			}
			if (!(pVeh->m_ulFlags&VEH_SABERINLEFTHAND) && pilotPS->torsoAnim==BOTH_VS_ATR_TO_L_S)
			{
				pVeh->m_ulFlags	|=  VEH_SABERINLEFTHAND;
			}
			WeaponPose = (pVeh->m_ulFlags&VEH_SABERINLEFTHAND)?(WPOSE_SABERLEFT):(WPOSE_SABERRIGHT);
		}


 		if (Attacking && WeaponPose)
		{// Attack!
 			iFlags	= SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART;

			// Auto Aiming
			//===============================================
			if (!Left && !Right)		// Allow player strafe keys to override
			{
				if (pilotPS->weapon==WP_SABER && !Left && !Right)
				{
					Left = (WeaponPose==WPOSE_SABERLEFT);
					Right	= !Left;
				}
			}


			if (Left)
			{// Attack Left
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VS_ATL_G;		break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VS_ATL_S;		break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VS_ATR_TO_L_S;	break;
				default:				assert(0);
				}
			}
			else if (Right)
			{// Attack Right
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VS_ATR_G;		break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VS_ATL_TO_R_S;	break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VS_ATR_S;		break;
				default:				assert(0);
				}
			}
			else
			{// Attack Ahead
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VS_ATF_G;		break;
				default:				assert(0);
				}
			}

		}
		else if (Left && pVeh->m_ucmd.buttons&BUTTON_USE)
		{// Look To The Left Behind
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			switch(WeaponPose)
			{
			case WPOSE_SABERLEFT:	Anim = BOTH_VS_IDLE_SL;		break;
			case WPOSE_SABERRIGHT:	Anim = BOTH_VS_IDLE_SR;		break;
			default:				Anim = BOTH_VS_LOOKLEFT;
			}
		}
		else if (Right && pVeh->m_ucmd.buttons&BUTTON_USE)
		{// Look To The Right Behind
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			switch(WeaponPose)
			{
			case WPOSE_SABERLEFT:	Anim = BOTH_VS_IDLE_SL;		break;
			case WPOSE_SABERRIGHT:	Anim = BOTH_VS_IDLE_SR;		break;
			default:				Anim = BOTH_VS_LOOKRIGHT;
			}
		}
		else if (Turbo)
		{// Kicked In Turbo
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;
			Anim	= BOTH_VS_TURBO;
		}
		else if (Flying)
		{// Off the ground in a jump
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			switch(WeaponPose)
			{
			case WPOSE_NONE:		Anim = BOTH_VS_AIR;			break;
			case WPOSE_BLASTER:		Anim = BOTH_VS_AIR_G;		break;
			case WPOSE_SABERLEFT:	Anim = BOTH_VS_AIR_SL;		break;
			case WPOSE_SABERRIGHT:	Anim = BOTH_VS_AIR_SR;		break;
			default:				assert(0);
			}
		}
		else if (Crashing)
		{// Hit the ground!
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;

			switch(WeaponPose)
			{
			case WPOSE_NONE:		Anim = BOTH_VS_LAND;		break;
			case WPOSE_BLASTER:		Anim = BOTH_VS_LAND_G;		break;
			case WPOSE_SABERLEFT:	Anim = BOTH_VS_LAND_SL;		break;
			case WPOSE_SABERRIGHT:	Anim = BOTH_VS_LAND_SR;		break;
			default:				assert(0);
			}
		}
		else
		{// No Special Moves
 			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;

			if (pVeh->m_vOrientation[ROLL] <= -20)
			{// Lean Left
				switch(WeaponPose)
				{
				case WPOSE_NONE:		Anim = BOTH_VS_LEANL;			break;
				case WPOSE_BLASTER:		Anim = BOTH_VS_LEANL_G;			break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VS_LEANL_SL;		break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VS_LEANL_SR;		break;
				default:				assert(0);
				}
			}
			else if (pVeh->m_vOrientation[ROLL] >= 20)
			{// Lean Right
				switch(WeaponPose)
				{
				case WPOSE_NONE:		Anim = BOTH_VS_LEANR;			break;
				case WPOSE_BLASTER:		Anim = BOTH_VS_LEANR_G;			break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VS_LEANR_SL;		break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VS_LEANR_SR;		break;
				default:				assert(0);
				}
			}
			else
			{// No Lean
				switch(WeaponPose)
				{
				case WPOSE_NONE:		Anim = BOTH_VS_IDLE;			break;
				case WPOSE_BLASTER:		Anim = BOTH_VS_IDLE_G;			break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VS_IDLE_SL;			break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VS_IDLE_SR;			break;
				default:				assert(0);
				}
			}
		}// No Special Moves
	}// Going backwards?

	iFlags &= ~SETANIM_FLAG_OVERRIDE;
	if (pVeh->m_pPilot->playerState->torsoAnim == Anim)
	{
		pVeh->m_pPilot->playerState->torsoTimer = BG_AnimLength(pVeh->m_pPilot->localAnimIndex, Anim);
	}
	if (pVeh->m_pPilot->playerState->legsAnim == Anim)
	{
		pVeh->m_pPilot->playerState->legsTimer = BG_AnimLength(pVeh->m_pPilot->localAnimIndex, Anim);
	}
	BG_SetAnim(pVeh->m_pPilot->playerState, bgAllAnims[pVeh->m_pPilot->localAnimIndex].anims,
		SETANIM_BOTH, Anim, iFlags|SETANIM_FLAG_HOLD);
}

#ifndef _GAME
void AttachRidersGeneric( Vehicle_t *pVeh );
#endif

void G_SetSpeederVehicleFunctions( vehicleInfo_t *pVehInfo )
{
#ifdef _GAME
	pVehInfo->AnimateVehicle			=		AnimateVehicle;
	pVehInfo->AnimateRiders				=		AnimateRiders;
//	pVehInfo->ValidateBoard				=		ValidateBoard;
//	pVehInfo->SetParent					=		SetParent;
//	pVehInfo->SetPilot					=		SetPilot;
//	pVehInfo->AddPassenger				=		AddPassenger;
//	pVehInfo->Animate					=		Animate;
//	pVehInfo->Board						=		Board;
//	pVehInfo->Eject						=		Eject;
//	pVehInfo->EjectAll					=		EjectAll;
//	pVehInfo->StartDeathDelay			=		StartDeathDelay;
//	pVehInfo->DeathUpdate				=		DeathUpdate;
//	pVehInfo->RegisterAssets			=		RegisterAssets;
//	pVehInfo->Initialize				=		Initialize;
	pVehInfo->Update					=		Update;
//	pVehInfo->UpdateRider				=		UpdateRider;
#endif

	//shared
	pVehInfo->ProcessMoveCommands		=		ProcessMoveCommands;
	pVehInfo->ProcessOrientCommands		=		ProcessOrientCommands;

#ifndef _GAME //cgame prediction attachment func
	pVehInfo->AttachRiders				=		AttachRidersGeneric;
#endif
//	pVehInfo->AttachRiders				=		AttachRiders;
//	pVehInfo->Ghost						=		Ghost;
//	pVehInfo->UnGhost					=		UnGhost;
//	pVehInfo->Inhabited					=		Inhabited;
}

// Following is only in game, not in namespace

#ifdef _GAME
extern void G_AllocateVehicleObject(Vehicle_t **pVeh);
#endif


// Create/Allocate a new Animal Vehicle (initializing it as well).
void G_CreateSpeederNPC( Vehicle_t **pVeh, const char *strType )
{
#ifdef _GAME
	//these will remain on entities on the client once allocated because the pointer is
	//never stomped. on the server, however, when an ent is freed, the entity struct is
	//memset to 0, so this memory would be lost..
    G_AllocateVehicleObject(pVeh);
#else
	if (!*pVeh)
	{ //only allocate a new one if we really have to
		(*pVeh) = (Vehicle_t *) BG_Alloc( sizeof(Vehicle_t) );
	}
#endif
	memset(*pVeh, 0, sizeof(Vehicle_t));
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strType )];
}
