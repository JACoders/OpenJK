// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"

//seems to be a compiler bug, it doesn't clean out the #ifdefs between dif-compiles
//or something, so the headers spew errors on these defs from the previous compile.
//this fixes that. -rww
#ifdef _JK2MP
//get rid of all the crazy defs we added for this file
#undef currentAngles
#undef currentOrigin
#undef mins
#undef maxs
#undef legsAnimTimer
#undef torsoAnimTimer
#undef bool
#undef false
#undef true

#undef sqrtf
#undef Q_flrand

#undef MOD_EXPLOSIVE
#endif

#ifdef _JK2 //SP does not have this preprocessor for game like MP does
#ifndef _JK2MP
#define _JK2MP
#endif
#endif

#ifndef _JK2MP //if single player
#ifndef QAGAME //I don't think we have a QAGAME define
#define QAGAME //but define it cause in sp we're always in the game
#endif
#endif

#ifdef QAGAME //including game headers on cgame is FORBIDDEN ^_^
#include "g_local.h"
#elif defined _JK2MP
#include "bg_public.h"
#endif

#ifndef _JK2MP
#include "g_functions.h"
#include "g_vehicles.h"
#include "..\game\wp_saber.h"
#else
#include "bg_vehicles.h"
#endif

#ifdef _JK2MP
//this is really horrible, but it works! just be sure not to use any locals or anything
//with these names (exluding bool, false, true). -rww
#define currentAngles r.currentAngles
#define currentOrigin r.currentOrigin
#define mins r.mins
#define maxs r.maxs
#define legsAnimTimer legsTimer
#define torsoAnimTimer torsoTimer
#define bool qboolean
#define false qfalse
#define true qtrue

//#define sqrtf sqrt
#define Q_flrand flrand

#define MOD_EXPLOSIVE MOD_SUICIDE
#else
#define bgEntity_t gentity_t
extern void NPC_SetAnim(gentity_t	*ent,int setAnimParts,int anim,int setAnimFlags, int iBlend);
#endif

extern float DotToSpot( vec3_t spot, vec3_t from, vec3_t fromAngles );
#ifdef QAGAME //SP or gameside MP
extern vmCvar_t	cg_thirdPersonAlpha;
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern cvar_t	*g_speederControlScheme;
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int PM_AnimLength( int index, animNumber_t anim );
#endif

#ifdef _JK2MP

#include "../namespace_begin.h"

extern void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int BG_GetTime(void);
extern qboolean BG_SabersOff( playerState_t *ps );
#endif

//Alright, actually, most of this file is shared between game and cgame for MP.
//I would like to keep it this way, so when modifying for SP please keep in
//mind the bgEntity restrictions imposed. -rww

#define	STRAFERAM_DURATION	8
#define	STRAFERAM_ANGLE		8


#ifndef _JK2MP
bool	VEH_StartStrafeRam(Vehicle_t *pVeh, bool Right)
{
	if (!(pVeh->m_ulFlags&VEH_STRAFERAM))
	{
		float	speed = VectorLength(pVeh->m_pParentEntity->client->ps.velocity);
		if (speed>400.0f)
		{
			// Compute Pos3
			//--------------
			vec3_t	right;
			AngleVectors(pVeh->m_vOrientation, 0, right, 0);
 			VectorMA(pVeh->m_pParentEntity->client->ps.velocity, (Right)?( speed):(-speed), right, pVeh->m_pParentEntity->pos3);

			pVeh->m_ulFlags				|= VEH_STRAFERAM;
			parentPS->hackingTime		 = (Right)?(STRAFERAM_DURATION):(-STRAFERAM_DURATION);

			if (pVeh->m_iSoundDebounceTimer<level.time && Q_irand(0,1)==0)
			{
				int	shiftSound = Q_irand(1,4);
				switch (shiftSound)
				{
				case 1: shiftSound=pVeh->m_pVehicleInfo->soundShift1; break;
				case 2: shiftSound=pVeh->m_pVehicleInfo->soundShift2; break;
				case 3: shiftSound=pVeh->m_pVehicleInfo->soundShift3; break;
				case 4: shiftSound=pVeh->m_pVehicleInfo->soundShift4; break;
				}
				if (shiftSound)
				{
					pVeh->m_iSoundDebounceTimer = level.time + Q_irand(1000, 4000);
					G_SoundIndexOnEnt( pVeh->m_pParentEntity, CHAN_AUTO, shiftSound);
				}
			}
			return true;
		}
	}
	return false;
}
#else
bool	VEH_StartStrafeRam(Vehicle_t *pVeh, bool Right, int Duration)
{
	return false;
}
#endif


#ifdef QAGAME //game-only.. for now
// Like a think or move command, this updates various vehicle properties.
bool Update( Vehicle_t *pVeh, const usercmd_t *pUcmd )
{
	if ( !g_vehicleInfo[VEHICLE_BASE].Update( pVeh, pUcmd ) )
	{
		return false;
	}

	// See whether this vehicle should be exploding.
	if ( pVeh->m_iDieTime != 0 )
	{
		pVeh->m_pVehicleInfo->DeathUpdate( pVeh );
	}

	// Update move direction.
#ifndef _JK2MP //this makes prediction unhappy, and rightfully so.
	gentity_t *parent = (gentity_t *)pVeh->m_pParentEntity;

	if ( pVeh->m_ulFlags & VEH_FLYING )
	{
		vec3_t vVehAngles;
		VectorSet(vVehAngles, 0, pVeh->m_vOrientation[YAW], 0 );
		AngleVectors( vVehAngles, parent->client->ps.moveDir, NULL, NULL );
	}
	else
	{
		vec3_t vVehAngles;
		VectorSet(vVehAngles, pVeh->m_vOrientation[PITCH], pVeh->m_vOrientation[YAW], 0 );
		AngleVectors( vVehAngles, parent->client->ps.moveDir, NULL, NULL );
	}

	// Check For A Strafe Ram
	//------------------------
	if (!(pVeh->m_ulFlags&VEH_STRAFERAM) && !(pVeh->m_ulFlags&VEH_FLYING))
	{
		// Started A Strafe
		//------------------
		if (pVeh->m_ucmd.rightmove && !parentPS->hackingTime)
		{
			parentPS->hackingTime = (pVeh->m_ucmd.rightmove>0)?(level.time):(-1*level.time);
		}

		// Ended A Strafe
		//----------------
		else if (!pVeh->m_ucmd.rightmove && parentPS->hackingTime)
		{
			// If It Was A Short Burst, Start The Strafe Ram
			//-----------------------------------------------
			if ((level.time - abs(parentPS->hackingTime))<300)
			{
				if (!VEH_StartStrafeRam(pVeh, (parentPS->hackingTime>0)))
				{
					parentPS->hackingTime = 0;
				}
			}

			// Otherwise, Clear The Timer
			//----------------------------
			else
			{
				parentPS->hackingTime = 0;
			}
		}
	}

	// If Currently In A StrafeRam, Check To See If It Is Done (Timed Out)
	//---------------------------------------------------------------------
  	else if (!parentPS->hackingTime)
	{
		pVeh->m_ulFlags &=~VEH_STRAFERAM;
	}


	// Exhaust Effects Start And Stop When The Accelerator Is Pressed
	//----------------------------------------------------------------
	if (pVeh->m_pVehicleInfo->iExhaustFX)
	{
		// Start It On Each Exhaust Bolt
		//-------------------------------
		if (pVeh->m_ucmd.forwardmove && !(pVeh->m_ulFlags&VEH_ACCELERATORON))
		{
			pVeh->m_ulFlags |= VEH_ACCELERATORON;
			for (int i=0; (i<MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i]!=-1); i++)
			{
				G_PlayEffect(pVeh->m_pVehicleInfo->iExhaustFX, parent->playerModel, pVeh->m_iExhaustTag[i], parent->s.number, parent->currentOrigin, 1,	qtrue);
			}
		}

		// Stop It On Each Exhaust Bolt
		//------------------------------
		else if (!pVeh->m_ucmd.forwardmove && (pVeh->m_ulFlags&VEH_ACCELERATORON))
		{
			pVeh->m_ulFlags &=~VEH_ACCELERATORON;
			for (int i=0; (i<MAX_VEHICLE_EXHAUSTS && pVeh->m_iExhaustTag[i]!=-1); i++)
			{
				G_StopEffect(pVeh->m_pVehicleInfo->iExhaustFX, parent->playerModel, pVeh->m_iExhaustTag[i], parent->s.number);
			}
		}
	}

	if (!(pVeh->m_ulFlags&VEH_ARMORLOW) && (pVeh->m_iArmor <= pVeh->m_pVehicleInfo->armor/3))
	{
		pVeh->m_ulFlags |= VEH_ARMORLOW;

	}

	// Armor Gone Effects (Fire)
	//---------------------------
	if (pVeh->m_pVehicleInfo->iArmorGoneFX)
	{
		if (!(pVeh->m_ulFlags&VEH_ARMORGONE) && (pVeh->m_iArmor <= 0))
		{
			pVeh->m_ulFlags |= VEH_ARMORGONE;
			G_PlayEffect(pVeh->m_pVehicleInfo->iArmorGoneFX, parent->playerModel, parent->crotchBolt, parent->s.number, parent->currentOrigin, 1, qtrue);
			parent->s.loopSound = G_SoundIndex( "sound/vehicles/common/fire_lp.wav" );
		}
	}
#endif

	return true;
}
#endif //QAGAME

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
	float speedInc, speedIdleDec, speedIdle, speedIdleAccel, speedMin, speedMax;
	playerState_t *parentPS;
	playerState_t *pilotPS = NULL;
	int	curTime;

#ifdef _JK2MP
	parentPS = pVeh->m_pParentEntity->playerState;
	if (pVeh->m_pPilot)
	{
		pilotPS = pVeh->m_pPilot->playerState;
	}
#else
	parentPS = &pVeh->m_pParentEntity->client->ps;
	if (pVeh->m_pPilot)
	{
		pilotPS = &pVeh->m_pPilot->client->ps;
	}
#endif


	// If we're flying, make us accelerate at 40% (about half) acceleration rate, and restore the pitch
	// to origin (straight) position (at 5% increments). 
	if ( pVeh->m_ulFlags & VEH_FLYING ) 
	{
		speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier * 0.4f;
	}
#ifdef _JK2MP
	else if ( !parentPS->m_iVehicleNum )
#else
	else if ( !pVeh->m_pVehicleInfo->Inhabited( pVeh ) )
#endif
	{//drifts to a stop
		speedInc = 0;
		//pVeh->m_ucmd.forwardmove = 127;
	}
	else
	{
		speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
	}
	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;

#ifndef _JK2MP//SP
	curTime = level.time;
#elif QAGAME//MP GAME
	curTime = level.time;
#elif CGAME//MP CGAME
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
#ifdef QAGAME
						if (pVeh->m_pParentEntity &&
							pVeh->m_pParentEntity->ghoul2 &&
							pVeh->m_pParentEntity->playerState)
						{ //fine, I'll use a tempent for this, but only because it's played only once at the start of a turbo.
							vec3_t boltOrg, boltDir;
							mdxaBone_t boltMatrix;

							VectorSet(boltDir, 0.0f, pVeh->m_pParentEntity->playerState->viewangles[YAW], 0.0f);

							trap_G2API_GetBoltMatrix(pVeh->m_pParentEntity->ghoul2, 0, pVeh->m_iExhaustTag[i], &boltMatrix, boltDir, pVeh->m_pParentEntity->playerState->origin, level.time, NULL, pVeh->m_pParentEntity->modelScale);
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
		if (pVeh->m_ucmd.forwardmove>=0 
#ifndef _JK2MP
			|| ((level.time - pVeh->m_pParentEntity->lastMoveTime)>500)
#endif
			)
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
	speedIdleAccel = pVeh->m_pVehicleInfo->accelIdle * pVeh->m_fTimeModifier;
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
		if ( !pVeh->m_pVehicleInfo->strafePerc 
#ifdef _JK2MP
			|| (0 && pVeh->m_pParentEntity->s.number < MAX_CLIENTS) )
#else
			|| (!g_speederControlScheme->value && !pVeh->m_pParentEntity->s.number) )
#endif
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
#ifdef _JK2MP //temp hack til mp speeder controls are sorted -rww
extern void AnimalProcessOri(Vehicle_t *pVeh);
#endif
void ProcessOrientCommands( Vehicle_t *pVeh )
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/
	playerState_t *riderPS;
	playerState_t *parentPS;

#ifdef _JK2MP
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

#else
 	gentity_t *rider = pVeh->m_pParentEntity->owner;
	if ( !rider || !rider->client )
	{
		riderPS = &pVeh->m_pParentEntity->client->ps;
	}
	else
	{
		riderPS = &rider->client->ps;
	}
	parentPS = &pVeh->m_pParentEntity->client->ps;

	if (pVeh->m_ulFlags & VEH_FLYING)
	{
		pVeh->m_vOrientation[YAW] += pVeh->m_vAngularVelocity;
	}
	else if (
		(pVeh->m_ulFlags & VEH_SLIDEBREAKING) ||	// No Angles Control While Out Of Control 
		(pVeh->m_ulFlags & VEH_OUTOFCONTROL) 		// No Angles Control While Out Of Control 
		)
	{
		// Any ability to change orientation?
	}
	else if (
		(pVeh->m_ulFlags & VEH_STRAFERAM)			// No Angles Control While Strafe Ramming
		)
	{
		if (parentPS->hackingTime>0)
		{
			parentPS->hackingTime--;
			pVeh->m_vOrientation[ROLL] += (parentPS->hackingTime<( STRAFERAM_DURATION/2))?(-STRAFERAM_ANGLE):( STRAFERAM_ANGLE);
		}
		else if (pVeh->hackingTime<0)
		{
			parentPS->hackingTime++;
			pVeh->m_vOrientation[ROLL] += (parentPS->hackingTime>(-STRAFERAM_DURATION/2))?( STRAFERAM_ANGLE):(-STRAFERAM_ANGLE);
		}
	}
	else
	{
		pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
	}
#endif

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef QAGAME

extern void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int PM_AnimLength( int index, animNumber_t anim );

// This function makes sure that the vehicle is properly animated.
void AnimateVehicle( Vehicle_t *pVeh )
{
}

#endif //QAGAME

//rest of file is shared

#ifndef	_JK2MP
extern void CG_ChangeWeapon( int num );
#endif


#ifndef _JK2MP
extern void G_StartMatrixEffect( gentity_t *ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f, int spinTime = 0 );
#endif


//NOTE NOTE NOTE NOTE NOTE NOTE
//I want to keep this function BG too, because it's fairly generic already, and it
//would be nice to have proper prediction of animations. -rww
// This function makes sure that the rider's in this vehicle are properly animated.
void AnimateRiders( Vehicle_t *pVeh )
{
	animNumber_t Anim = BOTH_VS_IDLE;
	float fSpeedPercToMax;
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 300;
	playerState_t *pilotPS;
	playerState_t *parentPS;
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
				iBlend = 0;
				Anim = BOTH_VS_MOUNTTHROW_R;
			}
			else if ( pVeh->m_iBoarding == VEH_MOUNT_THROW_RIGHT)
			{
				iBlend = 0;
				Anim = BOTH_VS_MOUNTTHROW_L;
			}

			// Set the delay time (which happens to be the time it takes for the animation to complete).
			// NOTE: Here I made it so the delay is actually 40% (0.4f) of the animation time.
#ifdef _JK2MP
			iAnimLen = BG_AnimLength( pVeh->m_pPilot->localAnimIndex, Anim ) * 0.4f;
			pVeh->m_iBoarding = BG_GetTime() + iAnimLen;
#else
 			iAnimLen = PM_AnimLength( pVeh->m_pPilot->client->clientInfo.animFileIndex, Anim );// * 0.4f;
			if (pVeh->m_iBoarding!=VEH_MOUNT_THROW_LEFT && pVeh->m_iBoarding!=VEH_MOUNT_THROW_RIGHT)
			{
				pVeh->m_iBoarding = level.time + (iAnimLen*0.4f);
			}
			else
			{
				pVeh->m_iBoarding = level.time + iAnimLen;
			}
#endif
			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			
#ifdef _JK2MP
			BG_SetAnim(pVeh->m_pPilot->playerState, bgAllAnims[pVeh->m_pPilot->localAnimIndex].anims,
				SETANIM_BOTH, Anim, iFlags, iBlend);
#else
			NPC_SetAnim( pVeh->m_pPilot, SETANIM_BOTH, Anim, iFlags, iBlend );
			if (pVeh->m_pOldPilot)
			{
				iAnimLen = PM_AnimLength( pVeh->m_pPilot->client->clientInfo.animFileIndex, BOTH_VS_MOUNTTHROWEE);
				NPC_SetAnim( pVeh->m_pOldPilot, SETANIM_BOTH, BOTH_VS_MOUNTTHROWEE, iFlags, iBlend );
			}
#endif
		}

#ifndef _JK2MP
		if (pVeh->m_pOldPilot && pVeh->m_pOldPilot->client->ps.torsoAnimTimer<=0)
		{
			if (Q_irand(0, player->count)==0)
			{
				player->count++;
 				player->lastEnemy = pVeh->m_pOldPilot;
				G_StartMatrixEffect(player, MEF_LOOK_AT_ENEMY|MEF_NO_RANGEVAR|MEF_NO_VERTBOB|MEF_NO_SPIN, 1000);
			}

 			gentity_t*	oldPilot = pVeh->m_pOldPilot;
			pVeh->m_pVehicleInfo->Eject(pVeh, pVeh->m_pOldPilot, qtrue);		// will set pointer to zero

			// Kill Him
			//----------
			oldPilot->client->noRagTime = -1;	// no ragdoll for you
			G_Damage(oldPilot, pVeh->m_pPilot, pVeh->m_pPilot, pVeh->m_pPilot->currentAngles, pVeh->m_pPilot->currentOrigin, 1000, 0, MOD_CRUSH);

			// Compute THe Throw Direction As Backwards From The Vehicle's Velocity
			//----------------------------------------------------------------------
			vec3_t		throwDir;
			VectorScale(pVeh->m_pParentEntity->client->ps.velocity, -1.0f, throwDir);
			VectorNormalize(throwDir);
			throwDir[2] += 0.3f;	// up a little

			// Now Throw Him Out
			//-------------------
			G_Throw(oldPilot, throwDir, VectorLength(pVeh->m_pParentEntity->client->ps.velocity)/10.0f);
			NPC_SetAnim(oldPilot, SETANIM_BOTH, BOTH_DEATHBACKWARD1, SETANIM_FLAG_OVERRIDE, iBlend );
		}
#endif

		return;
	}

#ifdef _JK2MP //fixme
	if (1) return;
#endif

#ifdef _JK2MP
	pilotPS = pVeh->m_pPilot->playerState;
	parentPS = pVeh->m_pPilot->playerState;
#else
	pilotPS = &pVeh->m_pPilot->client->ps;
	parentPS = &pVeh->m_pParentEntity->client->ps;
#endif

#ifndef _JK2MP//SP
	curTime = level.time;
#elif QAGAME//MP GAME
	curTime = level.time;
#elif CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif

	// Percentage of maximum speed relative to current speed.
	fSpeedPercToMax = parentPS->speed / pVeh->m_pVehicleInfo->speedMax;

	// Going in reverse...
#ifdef _JK2MP
	if ( pVeh->m_ucmd.forwardmove < 0 && !(pVeh->m_ulFlags & VEH_SLIDEBREAKING))
#else
	if ( fSpeedPercToMax < -0.018f && !(pVeh->m_ulFlags & VEH_SLIDEBREAKING))
#endif
	{
		Anim = BOTH_VS_REV;
		iBlend = 500;
	}
	else 
	{
		bool		HasWeapon	= ((pilotPS->weapon != WP_NONE) && (pilotPS->weapon != WP_MELEE));
		bool		Attacking	= (HasWeapon && !!(pVeh->m_ucmd.buttons&BUTTON_ATTACK));
#ifdef _JK2MP //fixme: flying tends to spaz out a lot
		bool		Flying		= false;
		bool		Crashing	= false;
#else
		bool		Flying		= !!(pVeh->m_ulFlags & VEH_FLYING);
		bool		Crashing	= !!(pVeh->m_ulFlags & VEH_CRASHING);
#endif
		bool		Right		= (pVeh->m_ucmd.rightmove>0);
		bool		Left		= (pVeh->m_ucmd.rightmove<0);
		bool		Turbo		= (curTime<pVeh->m_iTurboTime);
		EWeaponPose	WeaponPose	= WPOSE_NONE;


		// Remove Crashing Flag
		//----------------------
		pVeh->m_ulFlags &= ~VEH_CRASHING;


		// Put Away Saber When It Is Not Active
		//--------------------------------------
#ifndef _JK2MP
		if (HasWeapon && (Turbo || (pilotPS->weapon==WP_SABER && !pilotPS->SaberActive())))
		{
			if (pVeh->m_pPilot->s.number<MAX_CLIENTS)
			{
				CG_ChangeWeapon(WP_NONE);
			}

			pVeh->m_pPilot->client->ps.weapon = WP_NONE;
			G_RemoveWeaponModels(pVeh->m_pPilot);
		}
#endif

		// Don't Interrupt Attack Anims
		//------------------------------
#ifdef _JK2MP
		if (pilotPS->weaponTime>0)
		{
			return;
		}
#else		
		if (pilotPS->torsoAnim>=BOTH_VS_ATL_S && pilotPS->torsoAnim<=BOTH_VS_ATF_G)
		{
			float		bodyCurrent	  = 0.0f;
			int			bodyEnd		  = 0;
			if (!!gi.G2API_GetBoneAnimIndex(&pVeh->m_pPilot->ghoul2[pVeh->m_pPilot->playerModel], pVeh->m_pPilot->rootBone, level.time, &bodyCurrent, NULL, &bodyEnd, NULL, NULL, NULL))
			{
				if (bodyCurrent<=((float)(bodyEnd)-1.5f))
				{
					return;
				}
			}
		}
#endif

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
			iBlend	= 100;
 			iFlags	= SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART;
	
			// Auto Aiming
			//===============================================
			if (!Left && !Right)		// Allow player strafe keys to override
			{
#ifndef _JK2MP
				if (pVeh->m_pPilot->enemy)
				{
					vec3_t	toEnemy;
					float	toEnemyDistance;
					vec3_t	actorRight;
					float	actorRightDot;

					VectorSubtract(pVeh->m_pPilot->currentOrigin, pVeh->m_pPilot->enemy->currentOrigin, toEnemy);
					toEnemyDistance = VectorNormalize(toEnemy);

					AngleVectors(pVeh->m_pParentEntity->currentAngles, 0, actorRight, 0);
					actorRightDot = DotProduct(toEnemy, actorRight);

	 				if (fabsf(actorRightDot)>0.5f || pilotPS->weapon==WP_SABER)
					{
						Left	= (actorRightDot>0.0f);
						Right	= !Left;
					}
					else
					{
						Right = Left = false;
					}
				}
				else 
#endif
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
			iBlend	= 400;
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
			iBlend	= 400;
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
			iBlend	= 50;
			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;
			Anim	= BOTH_VS_TURBO;
		}
		else if (Flying)
		{// Off the ground in a jump
			iBlend	= 800;
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
			iBlend	= 100;
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
			iBlend	= 300;
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

#ifdef _JK2MP
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
		SETANIM_BOTH, Anim, iFlags|SETANIM_FLAG_HOLD, iBlend);
#else
	NPC_SetAnim( pVeh->m_pPilot, SETANIM_BOTH, Anim, iFlags, iBlend );
#endif
}

#ifndef QAGAME
void AttachRidersGeneric( Vehicle_t *pVeh );
#endif

void G_SetSpeederVehicleFunctions( vehicleInfo_t *pVehInfo )
{
#ifdef QAGAME
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

#ifndef QAGAME //cgame prediction attachment func
	pVehInfo->AttachRiders				=		AttachRidersGeneric;
#endif
//	pVehInfo->AttachRiders				=		AttachRiders;
//	pVehInfo->Ghost						=		Ghost;
//	pVehInfo->UnGhost					=		UnGhost;
//	pVehInfo->Inhabited					=		Inhabited;
}

// Following is only in game, not in namespace
#ifdef _JK2MP
#include "../namespace_end.h"
#endif

#ifdef QAGAME
extern void G_AllocateVehicleObject(Vehicle_t **pVeh);
#endif

#ifdef _JK2MP
#include "../namespace_begin.h"
#endif

// Create/Allocate a new Animal Vehicle (initializing it as well).
void G_CreateSpeederNPC( Vehicle_t **pVeh, const char *strType )
{
#ifdef _JK2MP
#ifdef QAGAME
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
#else
	// Allocate the Vehicle.
	(*pVeh) = (Vehicle_t *) gi.Malloc( sizeof(Vehicle_t), TAG_G_ALLOC, qtrue );
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strType )];
#endif
}

#ifdef _JK2MP

#include "../namespace_end.h"

//get rid of all the crazy defs we added for this file
#undef currentAngles
#undef currentOrigin
#undef mins
#undef maxs
#undef legsAnimTimer
#undef torsoAnimTimer
#undef bool
#undef false
#undef true

#undef sqrtf
#undef Q_flrand

#undef MOD_EXPLOSIVE
#endif
