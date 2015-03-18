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

#ifdef _GAME //we only want a few of these functions for BG

extern float DotToSpot( vec3_t spot, vec3_t from, vec3_t fromAngles );
extern vec3_t playerMins;
extern vec3_t playerMaxs;

extern int PM_AnimLength( int index, animNumber_t anim );

extern void Vehicle_SetAnim(gentity_t *ent,int setAnimParts,int anim,int setAnimFlags, int iBlend);
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void G_VehicleTrace( trace_t *results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs, const vec3_t end, int passEntityNum, int contentmask );

// Update death sequence.
static void DeathUpdate( Vehicle_t *pVeh )
{
	if ( level.time >= pVeh->m_iDieTime )
	{
		// If the vehicle is not empty.
		if ( pVeh->m_pVehicleInfo->Inhabited( pVeh ) )
		{
			pVeh->m_pVehicleInfo->EjectAll( pVeh );
		}
		else
		{
			// Waste this sucker.
		}

		// Die now...
/*		else
		{
			vec3_t	mins, maxs, bottom;
			trace_t	trace;

			if ( pVeh->m_pVehicleInfo->explodeFX )
			{
				G_PlayEffect( pVeh->m_pVehicleInfo->explodeFX, parent->currentOrigin );
				//trace down and place mark
				VectorCopy( parent->currentOrigin, bottom );
				bottom[2] -= 80;
				trap->trace( &trace, parent->currentOrigin, vec3_origin, vec3_origin, bottom, parent->s.number, CONTENTS_SOLID );
				if ( trace.fraction < 1.0f )
				{
					VectorCopy( trace.endpos, bottom );
					bottom[2] += 2;
					G_PlayEffect( "ships/ship_explosion_mark", trace.endpos );
				}
			}

			parent->takedamage = qfalse;//so we don't recursively damage ourselves
			if ( pVeh->m_pVehicleInfo->explosionRadius > 0 && pVeh->m_pVehicleInfo->explosionDamage > 0 )
			{
				VectorCopy( parent->mins, mins );
				mins[2] = -4;//to keep it off the ground a *little*
				VectorCopy( parent->maxs, maxs );
				VectorCopy( parent->currentOrigin, bottom );
				bottom[2] += parent->mins[2] - 32;
				trap->trace( &trace, parent->currentOrigin, mins, maxs, bottom, parent->s.number, CONTENTS_SOLID );
				G_RadiusDamage( trace.endpos, NULL, pVeh->m_pVehicleInfo->explosionDamage, pVeh->m_pVehicleInfo->explosionRadius, NULL, MOD_EXPLOSIVE );//FIXME: extern damage and radius or base on fuel
			}

			parent->e_ThinkFunc = thinkF_G_FreeEntity;
			parent->nextthink = level.time + FRAMETIME;
		}*/
	}
}

// Like a think or move command, this updates various vehicle properties.
static qboolean Update( Vehicle_t *pVeh, const usercmd_t *pUcmd )
{
	return g_vehicleInfo[VEHICLE_BASE].Update( pVeh, pUcmd );
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
	float fWalkSpeedMax;
	int		curTime;
	bgEntity_t *parent = pVeh->m_pParentEntity;
	playerState_t *parentPS = parent->playerState;

#ifdef _GAME
	curTime = level.time;
#elif defined(_CGAME)
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	curTime = pm->cmd.serverTime;
#endif

	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;
	speedMax = pVeh->m_pVehicleInfo->speedMax;

	speedIdle = pVeh->m_pVehicleInfo->speedIdle;
//	speedIdleAccel = pVeh->m_pVehicleInfo->accelIdle * pVeh->m_fTimeModifier;
	speedMin = pVeh->m_pVehicleInfo->speedMin;



	if ( pVeh->m_pPilot /*&& (pilotPS->weapon == WP_NONE || pilotPS->weapon == WP_MELEE )*/ &&
		(pVeh->m_ucmd.buttons & BUTTON_ALT_ATTACK) && pVeh->m_pVehicleInfo->turboSpeed )
	{
		if ((curTime - pVeh->m_iTurboTime)>pVeh->m_pVehicleInfo->turboRecharge)
		{
			pVeh->m_iTurboTime = (curTime + pVeh->m_pVehicleInfo->turboDuration);
			parentPS->speed = pVeh->m_pVehicleInfo->turboSpeed;	// Instantly Jump To Turbo Speed
		}
	}

	if ( curTime < pVeh->m_iTurboTime )
	{
		speedMax = pVeh->m_pVehicleInfo->turboSpeed;
	}
	else
	{
		speedMax = pVeh->m_pVehicleInfo->speedMax;
	}

	if ( !parentPS->m_iVehicleNum  )
	{//drifts to a stop
		speedInc = speedIdle * pVeh->m_fTimeModifier;
		VectorClear( parentPS->moveDir );
		//m_ucmd.forwardmove = 127;
		parentPS->speed = 0;
	}
	else
	{
		speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
	}

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
		if ( pVeh->m_ucmd.forwardmove < 0 )
		{
			pVeh->m_ucmd.forwardmove = 0;
		}
		if ( pVeh->m_ucmd.upmove < 0 )
		{
			pVeh->m_ucmd.upmove = 0;
		}

		//pVeh->m_ucmd.rightmove = 0;

		/*if ( !pVeh->m_pVehicleInfo->strafePerc
			|| (!g_speederControlScheme->value && !parent->s.number) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			pVeh->m_ucmd.rightmove = 0;
		}*/
	}

	fWalkSpeedMax = speedMax * 0.275f;
	if ( curTime>pVeh->m_iTurboTime && (pVeh->m_ucmd.buttons & BUTTON_WALKING) && parentPS->speed > fWalkSpeedMax )
	{
		parentPS->speed = fWalkSpeedMax;
	}
	else if ( parentPS->speed > speedMax )
	{
		parentPS->speed = speedMax;
	}
	else if ( parentPS->speed < speedMin )
	{
		parentPS->speed = speedMin;
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
// ProcessOrientCommands the Vehicle.
static void ProcessOrientCommands( Vehicle_t *pVeh )
{
	/********************************************************************************/
	/*	BEGIN	Here is where make sure the vehicle is properly oriented.	BEGIN	*/
	/********************************************************************************/
	bgEntity_t *parent = pVeh->m_pParentEntity;
	playerState_t *parentPS, *riderPS;

	bgEntity_t *rider = NULL;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}

	if ( !rider )
	{
		rider = parent;
	}



	parentPS = parent->playerState;

	if (rider)
	{
		float angDif;
		riderPS = rider->playerState;
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
		}
	}


/*	speed = VectorLength( parentPS->velocity );

	// If the player is the rider...
	if ( rider->s.number < MAX_CLIENTS )
	{//FIXME: use the vehicle's turning stat in this calc
		pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
	}
	else
	{
		float turnSpeed = pVeh->m_pVehicleInfo->turningSpeed;
		if ( !pVeh->m_pVehicleInfo->turnWhenStopped
			&& !parentPS->speed )//FIXME: or !pVeh->m_ucmd.forwardmove?
		{//can't turn when not moving
			//FIXME: or ramp up to max turnSpeed?
			turnSpeed = 0.0f;
		}
		if (rider->s.eType == ET_NPC)
		{//help NPCs out some
			turnSpeed *= 2.0f;
			if (parentPS->speed > 200.0f)
			{
				turnSpeed += turnSpeed * parentPS->speed/200.0f*0.05f;
			}
		}
		turnSpeed *= pVeh->m_fTimeModifier;

		//default control scheme: strafing turns, mouselook aims
		if ( pVeh->m_ucmd.rightmove < 0 )
		{
			pVeh->m_vOrientation[YAW] += turnSpeed;
		}
		else if ( pVeh->m_ucmd.rightmove > 0 )
		{
			pVeh->m_vOrientation[YAW] -= turnSpeed;
		}

		if ( pVeh->m_pVehicleInfo->malfunctionArmorLevel && pVeh->m_iArmor <= pVeh->m_pVehicleInfo->malfunctionArmorLevel )
		{//damaged badly
		}
	}*/

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

void AnimalProcessOri(Vehicle_t *pVeh)
{
	ProcessOrientCommands(pVeh);
}

#ifdef _GAME //back to our game-only functions
static void AnimateVehicle( Vehicle_t *pVeh )
{
	animNumber_t	Anim = BOTH_VT_IDLE;
	int				iFlags = SETANIM_FLAG_NORMAL, iBlend = 300;
	gentity_t *		pilot = (gentity_t *)pVeh->m_pPilot;
	gentity_t *		parent = (gentity_t *)pVeh->m_pParentEntity;
	float			fSpeedPercToMax;

	// We're dead (boarding is reused here so I don't have to make another variable :-).
	if ( parent->health <= 0 )
	{
		/*
		if ( pVeh->m_iBoarding != -999 )	// Animate the death just once!
		{
			pVeh->m_iBoarding = -999;
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			// FIXME! Why do you keep repeating over and over!!?!?!? Bastard!
			//Vehicle_SetAnim( parent, SETANIM_LEGS, BOTH_VT_DEATH1, iFlags, iBlend );
		}
		*/
		return;
	}

	// If they're bucking, play the animation and leave...
	if ( parent->client->ps.legsAnim == BOTH_VT_BUCK )
	{
		// Done with animation? Erase the flag.
		if ( parent->client->ps.legsTimer <= 0 )
		{
			pVeh->m_ulFlags &= ~VEH_BUCKING;
		}
		else
		{
			return;
		}
	}
	else if ( pVeh->m_ulFlags & VEH_BUCKING )
	{
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
		Anim = BOTH_VT_BUCK;
		iBlend = 500;
		Vehicle_SetAnim( parent, SETANIM_LEGS, BOTH_VT_BUCK, iFlags, iBlend );
		return;
	}

	// Boarding animation.
	if ( pVeh->m_iBoarding != 0 )
	{
		// We've just started boarding, set the amount of time it will take to finish boarding.
		if ( pVeh->m_iBoarding < 0 )
		{
			int iAnimLen;

			// Boarding from left...
			if ( pVeh->m_iBoarding == -1 )
			{
				Anim = BOTH_VT_MOUNT_L;
			}
			else if ( pVeh->m_iBoarding == -2 )
			{
				Anim = BOTH_VT_MOUNT_R;
			}
			else if ( pVeh->m_iBoarding == -3 )
			{
				Anim = BOTH_VT_MOUNT_B;
			}

			// Set the delay time (which happens to be the time it takes for the animation to complete).
			// NOTE: Here I made it so the delay is actually 70% (0.7f) of the animation time.
			iAnimLen = BG_AnimLength( parent->localAnimIndex, Anim ) * 0.7f;
			pVeh->m_iBoarding = level.time + iAnimLen;

			// Set the animation, which won't be interrupted until it's completed.
			// TODO: But what if he's killed? Should the animation remain persistant???
			iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;

			Vehicle_SetAnim( parent, SETANIM_LEGS, Anim, iFlags, iBlend );
			if (pilot)
			{
				Vehicle_SetAnim(pilot, SETANIM_BOTH, Anim, iFlags, iBlend );
			}
			return;
		}
		// Otherwise we're done.
		else if ( pVeh->m_iBoarding <= level.time )
		{
			pVeh->m_iBoarding = 0;
		}
	}

	// Percentage of maximum speed relative to current speed.
	//float fSpeed = VectorLength( client->ps.velocity );
	fSpeedPercToMax = parent->client->ps.speed / pVeh->m_pVehicleInfo->speedMax;


	// Going in reverse...
	if ( fSpeedPercToMax < -0.01f )
	{
		Anim = BOTH_VT_WALK_REV;
		iBlend = 600;
	}
	else
	{
		qboolean		Turbo		= (fSpeedPercToMax>0.0f && level.time<pVeh->m_iTurboTime);
		qboolean		Walking		= (fSpeedPercToMax>0.0f && ((pVeh->m_ucmd.buttons&BUTTON_WALKING) || fSpeedPercToMax<=0.275f));
		qboolean		Running		= (fSpeedPercToMax>0.275f);


		// Remove Crashing Flag
		//----------------------
		pVeh->m_ulFlags &= ~VEH_CRASHING;

		if (Turbo)
		{// Kicked In Turbo
			iBlend	= 50;
			iFlags	= SETANIM_FLAG_OVERRIDE;
			Anim	= BOTH_VT_TURBO;
		}
		else
		{// No Special Moves
			iBlend	= 300;
 			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;
			Anim	= (Walking)?(BOTH_VT_WALK_FWD  ):((Running)?(BOTH_VT_RUN_FWD  ):(BOTH_VT_IDLE1));
		}
	}
	Vehicle_SetAnim( parent, SETANIM_LEGS, Anim, iFlags, iBlend );
}

//rwwFIXMEFIXME: This is all going to have to be predicted I think, or it will feel awful
//and lagged
// This function makes sure that the rider's in this vehicle are properly animated.
static void AnimateRiders( Vehicle_t *pVeh )
{
	animNumber_t Anim = BOTH_VT_IDLE;
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 500;
	gentity_t *pilot = (gentity_t *)pVeh->m_pPilot;
	gentity_t *parent = (gentity_t *)pVeh->m_pParentEntity;
	playerState_t *pilotPS;
	float fSpeedPercToMax;

	pilotPS = pVeh->m_pPilot->playerState;

	// Boarding animation.
	if ( pVeh->m_iBoarding != 0 )
	{
		return;
	}

	// Percentage of maximum speed relative to current speed.
	fSpeedPercToMax = parent->client->ps.speed / pVeh->m_pVehicleInfo->speedMax;

	// Going in reverse...
	if (0)
	{
		Anim = BOTH_VT_WALK_REV;
		iBlend = 600;
	}
	else
	{
		qboolean		HasWeapon	= ((pilotPS->weapon != WP_NONE) && (pilotPS->weapon != WP_MELEE));
		qboolean		Attacking	= (HasWeapon && !!(pVeh->m_ucmd.buttons&BUTTON_ATTACK));
		qboolean		Right		= (pVeh->m_ucmd.rightmove>0);
		qboolean		Left		= (pVeh->m_ucmd.rightmove<0);
		qboolean		Turbo		= (fSpeedPercToMax>0.0f && level.time<pVeh->m_iTurboTime);
		qboolean		Walking		= (fSpeedPercToMax>0.0f && ((pVeh->m_ucmd.buttons&BUTTON_WALKING) || fSpeedPercToMax<=0.275f));
		qboolean		Running		= (fSpeedPercToMax>0.275f);
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
			if ( (pVeh->m_ulFlags&VEH_SABERINLEFTHAND) && pilotPS->torsoAnim==BOTH_VT_ATL_TO_R_S)
			{
				pVeh->m_ulFlags	&= ~VEH_SABERINLEFTHAND;
			}
			if (!(pVeh->m_ulFlags&VEH_SABERINLEFTHAND) && pilotPS->torsoAnim==BOTH_VT_ATR_TO_L_S)
			{
				pVeh->m_ulFlags	|=  VEH_SABERINLEFTHAND;
			}
			WeaponPose = (pVeh->m_ulFlags&VEH_SABERINLEFTHAND)?(WPOSE_SABERLEFT):(WPOSE_SABERRIGHT);
		}


 		if (Attacking && WeaponPose)
		{// Attack!
			iBlend	= 100;
 			iFlags	= SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART;

			if (Turbo)
			{
				Right = qtrue;
				Left = qfalse;
			}

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
				case WPOSE_BLASTER:		Anim = BOTH_VT_ATL_G;		break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VT_ATL_S;		break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VT_ATR_TO_L_S;	break;
				default:				assert(0);
				}
			}
			else if (Right)
			{// Attack Right
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VT_ATR_G;		break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VT_ATL_TO_R_S;	break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VT_ATR_S;		break;
				default:				assert(0);
				}
			}
			else
			{// Attack Ahead
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VT_ATF_G;		break;
				default:				assert(0);
				}
			}
		}
		else if (Turbo)
		{// Kicked In Turbo
			iBlend	= 50;
			iFlags	= SETANIM_FLAG_OVERRIDE;
			Anim	= BOTH_VT_TURBO;
		}
		else
		{// No Special Moves
			iBlend	= 300;
 			iFlags	= SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLDLESS;

			if (WeaponPose==WPOSE_NONE)
			{
				if (Walking)
				{
					Anim = BOTH_VT_WALK_FWD;
				}
				else if (Running)
				{
					Anim = BOTH_VT_RUN_FWD;
				}
				else
				{
					Anim = BOTH_VT_IDLE1;//(Q_irand(0,1)==0)?(BOTH_VT_IDLE):(BOTH_VT_IDLE1);
				}
			}
			else
			{
				switch(WeaponPose)
				{
				case WPOSE_BLASTER:		Anim = BOTH_VT_IDLE_G;			break;
				case WPOSE_SABERLEFT:	Anim = BOTH_VT_IDLE_SL;			break;
				case WPOSE_SABERRIGHT:	Anim = BOTH_VT_IDLE_SR;			break;
				default:				assert(0);
				}
			}
		}// No Special Moves
	}

	Vehicle_SetAnim( pilot, SETANIM_BOTH, Anim, iFlags, iBlend );
}
#endif //_GAME

#ifdef _CGAME
void AttachRidersGeneric( Vehicle_t *pVeh );
#endif

//on the client this function will only set up the process command funcs
void G_SetAnimalVehicleFunctions( vehicleInfo_t *pVehInfo )
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
	pVehInfo->DeathUpdate				=		DeathUpdate;
//	pVehInfo->RegisterAssets			=		RegisterAssets;
//	pVehInfo->Initialize				=		Initialize;
	pVehInfo->Update					=		Update;
//	pVehInfo->UpdateRider				=		UpdateRider;
#endif //_GAME
	pVehInfo->ProcessMoveCommands		=		ProcessMoveCommands;
	pVehInfo->ProcessOrientCommands		=		ProcessOrientCommands;

#ifdef _CGAME //cgame prediction attachment func
	pVehInfo->AttachRiders				=		AttachRidersGeneric;
#endif
//	pVehInfo->AttachRiders				=		AttachRiders;
//	pVehInfo->Ghost						=		Ghost;
//	pVehInfo->UnGhost					=		UnGhost;
//	pVehInfo->Inhabited					=		Inhabited;
}

#ifdef _GAME
extern void G_AllocateVehicleObject(Vehicle_t **pVeh);
#endif

// Create/Allocate a new Animal Vehicle (initializing it as well).
//this is a BG function too in MP so don't un-bg-compatibilify it -rww
void G_CreateAnimalNPC( Vehicle_t **pVeh, const char *strAnimalType )
{
	// Allocate the Vehicle.
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
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strAnimalType )];
}
