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
#endif

#ifdef QAGAME //we only want a few of these functions for BG

extern float DotToSpot( vec3_t spot, vec3_t from, vec3_t fromAngles );
extern vmCvar_t	cg_thirdPersonAlpha;
extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern cvar_t	*g_speederControlScheme;
extern void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int PM_AnimLength( int index, animNumber_t anim );
extern void Vehicle_SetAnim(gentity_t *ent,int setAnimParts,int anim,int setAnimFlags, int iBlend);
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void G_VehicleTrace( trace_t *results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs, const vec3_t end, int passEntityNum, int contentmask );

static void RegisterAssets( Vehicle_t *pVeh )
{
	//atst uses turret weapon
#ifdef _JK2MP
	RegisterItem(BG_FindItemForWeapon(WP_TURRET));
#else
	// PUT SOMETHING HERE...
#endif

	//call the standard RegisterAssets now
	g_vehicleInfo[VEHICLE_BASE].RegisterAssets( pVeh );
}

// Like a think or move command, this updates various vehicle properties.
/*
static bool Update( Vehicle_t *pVeh, const usercmd_t *pUcmd )
{
	return g_vehicleInfo[VEHICLE_BASE].Update( pVeh, pUcmd );
}
*/

// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
static bool Board( Vehicle_t *pVeh, bgEntity_t *pEnt )
{
	if ( !g_vehicleInfo[VEHICLE_BASE].Board( pVeh, pEnt ) )
		return false;

	// Set the board wait time (they won't be able to do anything, including getting off, for this amount of time).
	pVeh->m_iBoarding = level.time + 1500;

	return true;
}
#endif //QAGAME

#ifdef _JK2MP
#include "../namespace_begin.h"
#endif

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
	float fWalkSpeedMax;
	bgEntity_t *parent = pVeh->m_pParentEntity;
#ifdef _JK2MP
	playerState_t *parentPS = parent->playerState;
#else
	playerState_t *parentPS = &parent->client->ps;
#endif

	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;
	speedMax = pVeh->m_pVehicleInfo->speedMax;

	speedIdle = pVeh->m_pVehicleInfo->speedIdle;
	speedIdleAccel = pVeh->m_pVehicleInfo->accelIdle * pVeh->m_fTimeModifier;
	speedMin = pVeh->m_pVehicleInfo->speedMin;

#ifdef _JK2MP
	if ( !parentPS->m_iVehicleNum  )
#else
	if ( !pVeh->m_pVehicleInfo->Inhabited( pVeh ) )
#endif
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

		pVeh->m_ucmd.rightmove = 0;

		/*if ( !pVeh->m_pVehicleInfo->strafePerc 
			|| (!g_speederControlScheme->value && !parent->s.number) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			pVeh->m_ucmd.rightmove = 0;
		}*/
	}

	if (parentPS && parentPS->electrifyTime > pm->cmd.serverTime)
	{
		speedMax *= 0.5f;
	}

	fWalkSpeedMax = speedMax * 0.275f;
	if ( pVeh->m_ucmd.buttons & BUTTON_WALKING && parentPS->speed > fWalkSpeedMax )
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

	if (parentPS->stats[STAT_HEALTH] <= 0)
	{ //don't keep moving while you're dying!
		parentPS->speed = 0;
	}

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

#ifdef _JK2MP
void WalkerYawAdjust(Vehicle_t *pVeh, playerState_t *riderPS, playerState_t *parentPS)
{
	float angDif = AngleSubtract(pVeh->m_vOrientation[YAW], riderPS->viewangles[YAW]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		float maxDif = pVeh->m_pVehicleInfo->turningSpeed*1.5f; //magic number hackery

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

/*
void WalkerPitchAdjust(Vehicle_t *pVeh, playerState_t *riderPS, playerState_t *parentPS)
{
	float angDif = AngleSubtract(pVeh->m_vOrientation[PITCH], riderPS->viewangles[PITCH]);

	if (parentPS && parentPS->speed)
	{
		float s = parentPS->speed;
		float maxDif = pVeh->m_pVehicleInfo->turningSpeed*0.8f; //magic number hackery

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
		pVeh->m_vOrientation[PITCH] = AngleNormalize360(pVeh->m_vOrientation[PITCH] - angDif*(pVeh->m_fTimeModifier*0.2f));
	}
}
*/
#endif

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
	float speed;
	bgEntity_t *parent = pVeh->m_pParentEntity;
	playerState_t *parentPS, *riderPS;

#ifdef _JK2MP
	bgEntity_t *rider = NULL;
	if (parent->s.owner != ENTITYNUM_NONE)
	{
		rider = PM_BGEntForNum(parent->s.owner); //&g_entities[parent->r.ownerNum];
	}
#else
	gentity_t *rider = parent->owner;
#endif

#ifdef _JK2MP
	if ( !rider )
#else
	if ( !rider || !rider->client )
#endif
	{
		rider = parent;
	}

#ifdef _JK2MP
	parentPS = parent->playerState;
	riderPS = rider->playerState;
#else
	parentPS = &parent->client->ps;
	riderPS = &rider->client->ps;
#endif

	speed = VectorLength( parentPS->velocity );

	// If the player is the rider...
	if ( rider->s.number < MAX_CLIENTS )
	{//FIXME: use the vehicle's turning stat in this calc
#ifdef _JK2MP
		WalkerYawAdjust(pVeh, riderPS, parentPS);
		//FighterPitchAdjust(pVeh, riderPS, parentPS);
		pVeh->m_vOrientation[PITCH] = riderPS->viewangles[PITCH];
#else
		pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
		pVeh->m_vOrientation[PITCH] = riderPS->viewangles[PITCH];
#endif
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
#ifdef _JK2MP
		if (rider->s.eType == ET_NPC)
#else
		if ( !rider || rider->NPC )
#endif
		{//help NPCs out some
			turnSpeed *= 2.0f;
#ifdef _JK2MP
			if (parentPS->speed > 200.0f)
#else
			if ( parent->client->ps.speed > 200.0f )
#endif
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
	}

	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef QAGAME //back to our game-only functions
// This function makes sure that the vehicle is properly animated.
static void AnimateVehicle( Vehicle_t *pVeh )
{
	animNumber_t Anim = BOTH_STAND1; 
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 300;
	gentity_t *parent = (gentity_t *)pVeh->m_pParentEntity;
	float fSpeedPercToMax;

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

// Following is redundant to g_vehicles.c
//	if ( pVeh->m_iBoarding )
//	{
//		//we have no boarding anim
//		if (pVeh->m_iBoarding < level.time)
//		{ //we are on now
//			pVeh->m_iBoarding = 0;
//		}
//		else
//		{
//			return;
//		}
//	}

	// Percentage of maximum speed relative to current speed.
	//float fSpeed = VectorLength( client->ps.velocity );
	fSpeedPercToMax = parent->client->ps.speed / pVeh->m_pVehicleInfo->speedMax; 

	// If we're moving...
	if ( fSpeedPercToMax > 0.0f ) //fSpeedPercToMax >= 0.85f )
	{  
		float fYawDelta;

		iBlend = 300;
		iFlags = SETANIM_FLAG_OVERRIDE;
		fYawDelta = pVeh->m_vPrevOrientation[YAW] - pVeh->m_vOrientation[YAW];

		// NOTE: Mikes suggestion for fixing the stuttering walk (left/right) is to maintain the
		// current frame between animations. I have no clue how to do this and have to work on other
		// stuff so good luck to him :-p AReis

		// If we're walking (or our speed is less than .275%)...
		if ( ( pVeh->m_ucmd.buttons & BUTTON_WALKING ) || fSpeedPercToMax < 0.275f )
		{ 
			// Make them lean if we're turning.
			/*if ( fYawDelta < -0.0001f )
			{
				Anim = BOTH_VT_WALK_FWD_L;
			}
			else if ( fYawDelta > 0.0001 )
			{
				Anim = BOTH_VT_WALK_FWD_R;
			}
			else*/
			{
				Anim = BOTH_WALK1;
			}
		}
		// otherwise we're running.
		else
		{
			// Make them lean if we're turning.
			/*if ( fYawDelta < -0.0001f )
			{
				Anim = BOTH_VT_RUN_FWD_L;
			}
			else if ( fYawDelta > 0.0001 )
			{
				Anim = BOTH_VT_RUN_FWD_R;
			}
			else*/
			{
				Anim = BOTH_RUN1;
			}
		}
	}
	else
	{
		// Going in reverse...
		if ( fSpeedPercToMax < -0.018f )
		{
			iFlags = SETANIM_FLAG_NORMAL;
			Anim = BOTH_WALKBACK1;
			iBlend = 500;
		}
		else
		{
			//int iChance = Q_irand( 0, 20000 ); 

			// Every once in a while buck or do a different idle...
			iFlags = SETANIM_FLAG_NORMAL | SETANIM_FLAG_RESTART | SETANIM_FLAG_HOLD; 
			iBlend = 600;
#ifdef _JK2MP
			if (parent->client->ps.m_iVehicleNum)
#else
			if ( pVeh->m_pVehicleInfo->Inhabited( pVeh ) )
#endif
			{//occupado
				Anim = BOTH_STAND1;
			}
			else
			{//wide open for you, baby
				Anim = BOTH_STAND2;
			}
		}
	}

	Vehicle_SetAnim( parent, SETANIM_LEGS, Anim, iFlags, iBlend );
}

//rwwFIXMEFIXME: This is all going to have to be predicted I think, or it will feel awful
//and lagged
#endif //QAGAME

#ifndef QAGAME
void AttachRidersGeneric( Vehicle_t *pVeh );
#endif

//on the client this function will only set up the process command funcs
void G_SetWalkerVehicleFunctions( vehicleInfo_t *pVehInfo )
{
#ifdef QAGAME
	pVehInfo->AnimateVehicle			=		AnimateVehicle;
//	pVehInfo->AnimateRiders				=		AnimateRiders;
//	pVehInfo->ValidateBoard				=		ValidateBoard;
//	pVehInfo->SetParent					=		SetParent;
//	pVehInfo->SetPilot					=		SetPilot;
//	pVehInfo->AddPassenger				=		AddPassenger;
//	pVehInfo->Animate					=		Animate;
	pVehInfo->Board						=		Board;
//	pVehInfo->Eject						=		Eject;
//	pVehInfo->EjectAll					=		EjectAll;
//	pVehInfo->StartDeathDelay			=		StartDeathDelay;
//	pVehInfo->DeathUpdate				=		DeathUpdate;
	pVehInfo->RegisterAssets			=		RegisterAssets;
//	pVehInfo->Initialize				=		Initialize;
//	pVehInfo->Update					=		Update;
//	pVehInfo->UpdateRider				=		UpdateRider;
#endif //QAGAME
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
//this is a BG function too in MP so don't un-bg-compatibilify it -rww
void G_CreateWalkerNPC( Vehicle_t **pVeh, const char *strAnimalType )
{
	// Allocate the Vehicle.
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
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strAnimalType )];
#else
	(*pVeh) = (Vehicle_t *) gi.Malloc( sizeof(Vehicle_t), TAG_G_ALLOC, qtrue );
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strAnimalType )];
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
