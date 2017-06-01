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

#define sqrtf sqrt
#define Q_flrand flrand

#define MOD_EXPLOSIVE MOD_SUICIDE
#else
#define bgEntity_t gentity_t
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
extern void G_VehicleTrace( trace_t *results, const vec3_t start, const vec3_t tMins, const vec3_t tMaxs, const vec3_t end, int passEntityNum, int contentmask );
#endif

extern qboolean BG_UnrestrainedPitchRoll( playerState_t *ps, Vehicle_t *pVeh );

#ifdef _JK2MP

#include "../namespace_begin.h"

extern void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern int BG_GetTime(void);
#endif

#include "b_local.h"
extern void BG_ExternThisSoICanRecompileInDebug( Vehicle_t *pVeh, playerState_t *riderPS );

//this stuff has got to be predicted, so..
bool BG_FighterUpdate(Vehicle_t *pVeh, const usercmd_t *pUcmd, vec3_t trMins, vec3_t trMaxs, float gravity,
					  void (*traceFunc)( trace_t *results, const vec3_t start, const vec3_t lmins, const vec3_t lmaxs, const vec3_t end, int passEntityNum, int contentMask ))
{
	vec3_t		bottom;
	playerState_t *parentPS;
	//qboolean	isDead = qfalse;
#ifdef QAGAME //don't do this on client
	// Make sure the riders are not visible or collidable.
	pVeh->m_pVehicleInfo->Ghost( pVeh, pVeh->m_pPilot );
#endif


#ifdef _JK2MP
	parentPS = pVeh->m_pParentEntity->playerState;
#else
	parentPS = &pVeh->m_pParentEntity->client->ps;
#endif

	if (!parentPS)
	{
		Com_Error(ERR_DROP, "NULL PS in BG_FighterUpdate (%s)", pVeh->m_pVehicleInfo->name);
		return false;
	}

	// If we have a pilot, take out gravity (it's a flying craft...).
	if ( pVeh->m_pPilot )
	{
		parentPS->gravity = 0;
#ifndef _JK2MP //don't need this flag in mp, I.. guess
		pVeh->m_pParentEntity->svFlags |= SVF_CUSTOM_GRAVITY;
#endif
	}
	else
	{
#ifndef _JK2MP //don't need this flag in mp, I.. guess
		pVeh->m_pParentEntity->svFlags &= ~SVF_CUSTOM_GRAVITY;
#else //in MP set grav back to normal gravity
		if (pVeh->m_pVehicleInfo->gravity)
		{
			parentPS->gravity = pVeh->m_pVehicleInfo->gravity;
		}
		else
		{ //it doesn't have gravity specified apparently
			parentPS->gravity = gravity;
		}
#endif
	}

#ifdef _JK2MP
	//isDead = (qboolean)((parentPS->eFlags&EF_DEAD)!=0);
#else
	//isDead = (parentPS->stats[STAT_HEALTH] <= 0 );
#endif

	/*
	if ( isDead ||
		(pVeh->m_pVehicleInfo->surfDestruction &&
			pVeh->m_iRemovedSurfaces ) )
	{//can't land if dead or spiralling out of control
		pVeh->m_LandTrace.fraction = 1.0f;
		pVeh->m_LandTrace.contents = pVeh->m_LandTrace.surfaceFlags = 0;
		VectorClear( pVeh->m_LandTrace.plane.normal );
		pVeh->m_LandTrace.allsolid = qfalse;
		pVeh->m_LandTrace.startsolid = qfalse;
	}
	else
	{
	*/
	//argh, no, I need to have a way to see when they impact the ground while damaged. -rww

		// Check to see if the fighter has taken off yet (if it's a certain height above ground).
		VectorCopy( parentPS->origin, bottom );
		bottom[2] -= pVeh->m_pVehicleInfo->landingHeight;

		traceFunc( &pVeh->m_LandTrace, parentPS->origin, trMins, trMaxs, bottom, pVeh->m_pParentEntity->s.number, (MASK_NPCSOLID&~CONTENTS_BODY) );
	//}

	return true;
}

#ifdef QAGAME //ONLY in SP or on server, not cgame

// Like a think or move command, this updates various vehicle properties.
static bool Update( Vehicle_t *pVeh, const usercmd_t *pUcmd )
{
	assert(pVeh->m_pParentEntity);
	if (!BG_FighterUpdate(pVeh, pUcmd, ((gentity_t *)pVeh->m_pParentEntity)->mins,
		((gentity_t *)pVeh->m_pParentEntity)->maxs,
#ifdef _JK2MP
		g_gravity.value,
#else
		g_gravity->value,
#endif
		G_VehicleTrace))
	{
		return false;
	}

	if ( !g_vehicleInfo[VEHICLE_BASE].Update( pVeh, pUcmd ) )
	{
		return false;
	}

	return true;
}

// Board this Vehicle (get on). The first entity to board an empty vehicle becomes the Pilot.
static bool Board( Vehicle_t *pVeh, bgEntity_t *pEnt )
{
	if ( !g_vehicleInfo[VEHICLE_BASE].Board( pVeh, pEnt ) )
		return false;

	// Set the board wait time (they won't be able to do anything, including getting off, for this amount of time).
	pVeh->m_iBoarding = level.time + 1500;

	return true;
}

// Eject an entity from the vehicle.
static bool Eject( Vehicle_t *pVeh, bgEntity_t *pEnt, qboolean forceEject )
{
	if ( g_vehicleInfo[VEHICLE_BASE].Eject( pVeh, pEnt, forceEject ) )
	{
		return true;
	}

	return false;
}

#endif //end game-side only

//method of decrementing the given angle based on the given taking variable frame times into account
static float PredictedAngularDecrement(float scale, float timeMod, float originalAngle)
{
	float fixedBaseDec = originalAngle*0.05f;
	float r = 0.0f;

	if (fixedBaseDec < 0.0f)
	{
		fixedBaseDec = -fixedBaseDec;
	}

	fixedBaseDec *= (1.0f+(1.0f-scale));

	if (fixedBaseDec < 0.1f)
	{ //don't increment in incredibly small fractions, it would eat up unnecessary bandwidth.
		fixedBaseDec = 0.1f;
	}

	fixedBaseDec *= (timeMod*0.1f);
	if (originalAngle > 0.0f)
	{ //subtract
		r = (originalAngle-fixedBaseDec);
		if (r < 0.0f)
		{
			r = 0.0f;
		}
	}
	else if (originalAngle < 0.0f)
	{ //add
		r = (originalAngle+fixedBaseDec);
		if (r > 0.0f)
		{
			r = 0.0f;
		}
	}

	return r;
}

#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver
qboolean FighterIsInSpace( gentity_t *gParent )
{
	if ( gParent
		&& gParent->client
		&& gParent->client->inSpaceIndex
		&& gParent->client->inSpaceIndex < ENTITYNUM_WORLD )
	{
		return qtrue;
	}
	return qfalse;
}
#endif

qboolean FighterOverValidLandingSurface( Vehicle_t *pVeh )
{
	if ( pVeh->m_LandTrace.fraction < 1.0f //ground present
		&& pVeh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE )//flat enough
		//FIXME: also check for a certain surface flag ... "landing zones"?
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS )
{
	if ( FighterOverValidLandingSurface( pVeh )
		&& !parentPS->speed )//stopped
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLanding( Vehicle_t *pVeh, playerState_t *parentPS )
{

	if ( FighterOverValidLandingSurface( pVeh )
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver
		&& pVeh->m_pVehicleInfo->Inhabited( pVeh )//has to have a driver in order to be capable of landing
#endif
		&& (pVeh->m_ucmd.forwardmove < 0||pVeh->m_ucmd.upmove<0) //decelerating or holding crouch button
		&& parentPS->speed <= MIN_LANDING_SPEED )//going slow enough to start landing - was using pVeh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterIsLaunching( Vehicle_t *pVeh, playerState_t *parentPS )
{

	if ( FighterOverValidLandingSurface( pVeh )
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver
		&& pVeh->m_pVehicleInfo->Inhabited( pVeh )//has to have a driver in order to be capable of landing
#endif
		&& pVeh->m_ucmd.upmove > 0 //trying to take off
		&& parentPS->speed <= 200.0f )//going slow enough to start landing - was using pVeh->m_pVehicleInfo->speedIdle, but that's still too fast
	{
		return qtrue;
	}
	return qfalse;
}

qboolean FighterSuspended( Vehicle_t *pVeh, playerState_t *parentPS )
{
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver
	if (!pVeh->m_pPilot//empty
		&& !parentPS->speed//not moving
		&& pVeh->m_ucmd.forwardmove <= 0//not trying to go forward for whatever reason
		&& pVeh->m_pParentEntity != NULL
		&& (((gentity_t *)pVeh->m_pParentEntity)->spawnflags&2) )//SUSPENDED spawnflag is on
	{
		return qtrue;
	}
	return qfalse;
#elif defined CGAME
	return qfalse;
#endif
}

#ifdef CGAME
extern void trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ); //cg_syscalls.c
extern sfxHandle_t trap_S_RegisterSound( const char *sample); //cg_syscalls.c
#endif

//MP RULE - ALL PROCESSMOVECOMMANDS FUNCTIONS MUST BE BG-COMPATIBLE!!!
//If you really need to violate this rule for SP, then use ifdefs.
//By BG-compatible, I mean no use of game-specific data - ONLY use
//stuff available in the MP bgEntity (in SP, the bgEntity is #defined
//as a gentity, but the MP-compatible access restrictions are based
//on the bgEntity structure in the MP codebase) -rww
// ProcessMoveCommands the Vehicle.
#define FIGHTER_MIN_TAKEOFF_FRACTION 0.7f
static void ProcessMoveCommands( Vehicle_t *pVeh )
{
	/************************************************************************************/
	/*	BEGIN	Here is where we move the vehicle (forward or back or whatever). BEGIN	*/
	/************************************************************************************/

	//Client sets ucmds and such for speed alterations
	float speedInc, speedIdleDec, speedIdle, speedIdleAccel, speedMin, speedMax;
	bgEntity_t *parent = pVeh->m_pParentEntity;
	qboolean isLandingOrLaunching = qfalse;
#ifndef _JK2MP//SP
	int curTime = level.time;
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

#ifdef _JK2MP
	playerState_t *parentPS = parent->playerState;
#else
	playerState_t *parentPS = &parent->client->ps;
#endif

#ifdef _JK2MP
	if ( parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME )
	{//Going to Hyperspace
		//totally override movement
		float timeFrac = ((float)(curTime-parentPS->hyperSpaceTime))/HYPERSPACE_TIME;
		if ( timeFrac < HYPERSPACE_TELEPORT_FRAC )
		{//for first half, instantly jump to top speed!
			if ( !(parentPS->eFlags2&EF2_HYPERSPACE) )
			{//waiting to face the right direction, do nothing
				parentPS->speed = 0.0f;
			}
			else
			{
				if ( parentPS->speed < HYPERSPACE_SPEED )
				{//just started hyperspace
//MIKE: This is going to play the sound twice for the predicting client, I suggest using
//a predicted event or only doing it game-side. -rich
#ifdef QAGAME//MP GAME-side
					//G_EntitySound( ((gentity_t *)(pVeh->m_pParentEntity)), CHAN_LOCAL, pVeh->m_pVehicleInfo->soundHyper );
#elif defined CGAME//MP CGAME-side
					trap_S_StartSound( NULL, pm->ps->clientNum, CHAN_LOCAL, pVeh->m_pVehicleInfo->soundHyper );
#endif
				}

				parentPS->speed = HYPERSPACE_SPEED;
			}
		}
		else
		{//slow from top speed to 200...
			parentPS->speed = 200.0f + ((1.0f-timeFrac)*(1.0f/HYPERSPACE_TELEPORT_FRAC)*(HYPERSPACE_SPEED-200.0f));
			//don't mess with acceleration, just pop to the high velocity
			if ( VectorLength( parentPS->velocity ) < parentPS->speed )
			{
				VectorScale( parentPS->moveDir, parentPS->speed, parentPS->velocity );
			}
		}
		return;
	}
#endif

	if ( pVeh->m_iDropTime >= curTime )
	{//no speed, just drop
		parentPS->speed = 0.0f;
		parentPS->gravity = 800;
		return;
	}

	isLandingOrLaunching = (qboolean)(FighterIsLanding( pVeh, parentPS )||FighterIsLaunching( pVeh, parentPS ));

	// If we are hitting the ground, just allow the fighter to go up and down.
	if ( isLandingOrLaunching//going slow enough to start landing
		&& (pVeh->m_ucmd.forwardmove<=0||pVeh->m_LandTrace.fraction<=FIGHTER_MIN_TAKEOFF_FRACTION) )//not trying to accelerate away already (or: you are trying to, but not high enough off the ground yet)
	{//FIXME: if start to move forward and fly over something low while still going relatively slow, you may try to land even though you don't mean to...
		//float fInvFrac = 1.0f - pVeh->m_LandTrace.fraction;

		if ( pVeh->m_ucmd.upmove > 0 )
		{
#ifdef _JK2MP
			if ( parentPS->velocity[2] <= 0
				&& pVeh->m_pVehicleInfo->soundTakeOff )
			{//taking off for the first time
#ifdef QAGAME//MP GAME-side
				G_EntitySound( ((gentity_t *)(pVeh->m_pParentEntity)), CHAN_AUTO, pVeh->m_pVehicleInfo->soundTakeOff );
#endif
			}
#endif
			parentPS->velocity[2] += pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;// * ( /*fInvFrac **/ 1.5f );
		}
		else if ( pVeh->m_ucmd.upmove < 0 )
		{
			parentPS->velocity[2] -= pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;// * ( /*fInvFrac **/ 1.8f );
		}
		else if ( pVeh->m_ucmd.forwardmove < 0 )
		{
			if ( pVeh->m_LandTrace.fraction != 0.0f )
			{
				parentPS->velocity[2] -= pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
			}

			if ( pVeh->m_LandTrace.fraction <= FIGHTER_MIN_TAKEOFF_FRACTION )
			{
				//pVeh->m_pParentEntity->client->ps.velocity[0] *= pVeh->m_LandTrace.fraction;
				//pVeh->m_pParentEntity->client->ps.velocity[1] *= pVeh->m_LandTrace.fraction;

				//remember to always base this stuff on the time modifier! otherwise, you create
				//framerate-dependancy issues and break prediction in MP -rww
				//parentPS->velocity[2] *= pVeh->m_LandTrace.fraction;
				//it's not an angle, but hey
				parentPS->velocity[2] = PredictedAngularDecrement(pVeh->m_LandTrace.fraction, pVeh->m_fTimeModifier*5.0f, parentPS->velocity[2]);

				parentPS->speed = 0;
			}
		}

		// Make sure they don't pitch as they near the ground.
		//pVeh->m_vOrientation[PITCH] *= 0.7f;
		pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.7f, pVeh->m_fTimeModifier*10.0f, pVeh->m_vOrientation[PITCH]);

		return;
	}

	if ( (pVeh->m_ucmd.upmove > 0) && pVeh->m_pVehicleInfo->turboSpeed )
	{
		if ((curTime - pVeh->m_iTurboTime)>pVeh->m_pVehicleInfo->turboRecharge)
		{
			pVeh->m_iTurboTime = (curTime + pVeh->m_pVehicleInfo->turboDuration);
			if (pVeh->m_pVehicleInfo->iTurboStartFX)
			{
				int i;
				for (i=0; i<MAX_VEHICLE_EXHAUSTS; i++)
				{
					if (pVeh->m_iExhaustTag[i]==-1)
					{
						break;
					}
					#ifndef _JK2MP//SP
						G_PlayEffect(pVeh->m_pVehicleInfo->iTurboStartFX, pVeh->m_pParentEntity->playerModel, pVeh->m_iExhaustTag[i], pVeh->m_pParentEntity->s.number, pVeh->m_pParentEntity->currentOrigin );
					#else
						//TODO: MP Play Effect?
					#endif
				}
			}
			//NOTE: turbo sound can't be part of effect if effect is played on every muzzle!
			if ( pVeh->m_pVehicleInfo->soundTurbo )
			{
#ifndef _JK2MP//SP
				G_SoundIndexOnEnt( pVeh->m_pParentEntity, CHAN_AUTO, pVeh->m_pVehicleInfo->soundTurbo );
#elif defined QAGAME//MP GAME-side
				G_EntitySound( ((gentity_t *)(pVeh->m_pParentEntity)), CHAN_AUTO, pVeh->m_pVehicleInfo->soundTurbo );
#elif defined CGAME//MP CGAME-side
				//trap_S_StartSound( NULL, pVeh->m_pParentEntity->s.number, CHAN_AUTO, pVeh->m_pVehicleInfo->soundTurbo );
#endif
			}
		}
	}
	speedInc = pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
	if ( curTime < pVeh->m_iTurboTime )
	{//going turbo speed
		speedMax = pVeh->m_pVehicleInfo->turboSpeed;
		//double our acceleration
		speedInc *= 2.0f;
		//force us to move forward
		pVeh->m_ucmd.forwardmove = 127;
#ifdef _JK2MP//SP can cheat and just check m_iTurboTime directly... :)
		//add flag to let cgame know to draw the iTurboFX effect
		parentPS->eFlags |= EF_JETPACK_ACTIVE;
#endif
	}
	/*
	//FIXME: if turbotime is up and we're waiting for it to recharge, should our max speed drop while we recharge?
	else if ( (curTime - pVeh->m_iTurboTime)<3000 )
	{//still waiting for the recharge
		speedMax = pVeh->m_pVehicleInfo->speedMax*0.75;
	}
	*/
	else
	{//normal max speed
		speedMax = pVeh->m_pVehicleInfo->speedMax;
#ifdef _JK2MP//SP can cheat and just check m_iTurboTime directly... :)
		if ( (parentPS->eFlags&EF_JETPACK_ACTIVE) )
		{//stop cgame from playing the turbo exhaust effect
			parentPS->eFlags &= ~EF_JETPACK_ACTIVE;
		}
#endif
	}
	speedIdleDec = pVeh->m_pVehicleInfo->decelIdle * pVeh->m_fTimeModifier;
	speedIdle = pVeh->m_pVehicleInfo->speedIdle;
	speedIdleAccel = pVeh->m_pVehicleInfo->accelIdle * pVeh->m_fTimeModifier;
	speedMin = pVeh->m_pVehicleInfo->speedMin;

	if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_HEAVY)) )
	{//engine has taken heavy damage
		speedMax *= 0.8f;//at 80% speed
	}
	else if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_LIGHT)) )
	{//engine has taken light damage
		speedMax *= 0.6f;//at 60% speed
	}

	if (pVeh->m_iRemovedSurfaces
		|| parentPS->electrifyTime>=curTime)
	{ //go out of control
		parentPS->speed += speedInc;
		//Why set forwardmove?  PMove code doesn't use it... does it?
		pVeh->m_ucmd.forwardmove = 127;
	}
#ifdef QAGAME //well, the thing is always going to be inhabited if it's being predicted!
	else if ( FighterSuspended( pVeh, parentPS ) )
	{
		parentPS->speed = 0;
		pVeh->m_ucmd.forwardmove = 0;
	}
	else if ( !pVeh->m_pVehicleInfo->Inhabited( pVeh )
		&& parentPS->speed > 0 )
	{//pilot jumped out while we were moving forward (not landing or landed) so just keep the throttle locked
		//Why set forwardmove?  PMove code doesn't use it... does it?
		pVeh->m_ucmd.forwardmove = 127;
	}
#endif
	else if ( ( parentPS->speed || parentPS->groundEntityNum == ENTITYNUM_NONE  ||
		 pVeh->m_ucmd.forwardmove || pVeh->m_ucmd.upmove > 0 ) && pVeh->m_LandTrace.fraction >= 0.05f )
	{
		if ( pVeh->m_ucmd.forwardmove > 0 && speedInc )
		{
			parentPS->speed += speedInc;
			pVeh->m_ucmd.forwardmove = 127;
		}
		else if ( pVeh->m_ucmd.forwardmove < 0
			|| pVeh->m_ucmd.upmove < 0 )
		{//decelerating or braking
			if ( pVeh->m_ucmd.upmove < 0 )
			{//braking (trying to land?), slow down faster
				if ( pVeh->m_ucmd.forwardmove )
				{//decelerator + brakes
					speedInc += pVeh->m_pVehicleInfo->braking;
					speedIdleDec += pVeh->m_pVehicleInfo->braking;
				}
				else
				{//just brakes
					speedInc = speedIdleDec = pVeh->m_pVehicleInfo->braking;
				}
			}
			if ( parentPS->speed > speedIdle )
			{
				parentPS->speed -= speedInc;
			}
			else if ( parentPS->speed > speedMin )
			{
				if ( FighterOverValidLandingSurface( pVeh ) )
				{//there's ground below us and we're trying to slow down, slow down faster
					parentPS->speed -= speedInc;
				}
				else
				{
					parentPS->speed -= speedIdleDec;
					if ( parentPS->speed < MIN_LANDING_SPEED )
					{//unless you can land, don't drop below the landing speed!!!  This way you can't come to a dead stop in mid-air
						parentPS->speed = MIN_LANDING_SPEED;
					}
				}
			}
			if ( pVeh->m_pVehicleInfo->type == VH_FIGHTER )
			{
				pVeh->m_ucmd.forwardmove = 127;
			}
			else if ( speedMin >= 0 )
			{
				pVeh->m_ucmd.forwardmove = 0;
			}
		}
		//else not accel, decel or braking
		else if ( pVeh->m_pVehicleInfo->throttleSticks )
		{//we're using a throttle that sticks at current speed
			if ( parentPS->speed <= MIN_LANDING_SPEED )
			{//going less than landing speed
				if ( FighterOverValidLandingSurface( pVeh ) )
				{//close to ground and not going very fast
					//slow to a stop if within landing height and not accel/decel/braking
					if ( parentPS->speed > 0 )
					{//slow down
						parentPS->speed -= speedIdleDec;
					}
					else if ( parentPS->speed < 0 )
					{//going backwards, slow down
						parentPS->speed += speedIdleDec;
					}
				}
				else
				{//not over a valid landing surf, but going too slow
					//speed up to idle speed if not over a valid landing surf and not accel/decel/braking
					if ( parentPS->speed < speedIdle )
					{
						parentPS->speed += speedIdleAccel;
						if ( parentPS->speed > speedIdle )
						{
							parentPS->speed = speedIdle;
						}
					}
				}
			}
		}
		else
		{//then speed up or slow down to idle speed
			//accelerate to cruising speed only, otherwise, just coast
			// If they've launched, apply some constant motion.
			if ( (pVeh->m_LandTrace.fraction >= 1.0f //no ground
					|| pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE )//or can't land on ground below us
				&& speedIdle > 0 )
			{//not above ground and have an idle speed
				//float fSpeed = pVeh->m_pParentEntity->client->ps.speed;
				if ( parentPS->speed < speedIdle )
				{
					parentPS->speed += speedIdleAccel;
					if ( parentPS->speed > speedIdle )
					{
						parentPS->speed = speedIdle;
					}
				}
				else if ( parentPS->speed > 0 )
				{//slow down
					parentPS->speed -= speedIdleDec;

					if ( parentPS->speed < speedIdle )
					{
						parentPS->speed = speedIdle;
					}
				}
			}
			else//either close to ground or no idle speed
			{//slow to a stop if no idle speed or within landing height and not accel/decel/braking
				if ( parentPS->speed > 0 )
				{//slow down
					parentPS->speed -= speedIdleDec;
				}
				else if ( parentPS->speed < 0 )
				{//going backwards, slow down
					parentPS->speed += speedIdleDec;
				}
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

#ifndef _JK2MP
		if ( !pVeh->m_pVehicleInfo->strafePerc || (!g_speederControlScheme->value && !pVeh->m_pParentEntity->s.number) )
		{//if in a strafe-capable vehicle, clear strafing unless using alternate control scheme
			pVeh->m_ucmd.rightmove = 0;
		}
#endif
	}

#if 1//This is working now, but there are some transitional jitters... Rich?
//STRAFING==============================================================================
	if ( pVeh->m_pVehicleInfo->strafePerc
#ifdef QAGAME//only do this check on GAME side, because if it's CGAME, it's being predicted, and it's only predicted if the local client is the driver
		&& pVeh->m_pVehicleInfo->Inhabited( pVeh )//has to have a driver in order to be capable of landing
#endif
		&& !pVeh->m_iRemovedSurfaces
		&& parentPS->electrifyTime<curTime
		&& (pVeh->m_LandTrace.fraction >= 1.0f//no grounf
			||pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE//can't land here
			||parentPS->speed>MIN_LANDING_SPEED)//going too fast to land
		&& pVeh->m_ucmd.rightmove )
	{//strafe
		vec3_t vAngles, vRight;
		float strafeSpeed = (pVeh->m_pVehicleInfo->strafePerc*speedMax)*5.0f;
		VectorCopy( pVeh->m_vOrientation, vAngles );
		vAngles[PITCH] = vAngles[ROLL] = 0;
		AngleVectors( vAngles, NULL, vRight, NULL );

		if ( pVeh->m_ucmd.rightmove > 0 )
		{//strafe right
			//FIXME: this will probably make it possible to cheat and
			//		go faster than max speed if you keep turning and strafing...
			if ( pVeh->m_fStrafeTime > -MAX_STRAFE_TIME )
			{//can strafe right for 2 seconds
				float curStrafeSpeed = DotProduct( parentPS->velocity, vRight );
				if ( curStrafeSpeed > 0.0f )
				{//if > 0, already strafing right
					strafeSpeed -= curStrafeSpeed;//so it doesn't add up
				}
				if ( strafeSpeed > 0 )
				{
					VectorMA( parentPS->velocity, strafeSpeed*pVeh->m_fTimeModifier, vRight, parentPS->velocity );
				}
				pVeh->m_fStrafeTime -= 50*pVeh->m_fTimeModifier;
			}
		}
		else
		{//strafe left
			if ( pVeh->m_fStrafeTime < MAX_STRAFE_TIME )
			{//can strafe left for 2 seconds
				float curStrafeSpeed = DotProduct( parentPS->velocity, vRight );
				if ( curStrafeSpeed < 0.0f )
				{//if < 0, already strafing left
					strafeSpeed += curStrafeSpeed;//so it doesn't add up
				}
				if ( strafeSpeed > 0 )
				{
					VectorMA( parentPS->velocity, -strafeSpeed*pVeh->m_fTimeModifier, vRight, parentPS->velocity );
				}
				pVeh->m_fStrafeTime += 50*pVeh->m_fTimeModifier;
			}
		}
		//strafing takes away from forward speed?  If so, strafePerc above should use speedMax
		//parentPS->speed *= (1.0f-pVeh->m_pVehicleInfo->strafePerc);
	}
	else//if ( pVeh->m_fStrafeTime )
	{
		if ( pVeh->m_fStrafeTime > 0 )
		{
			pVeh->m_fStrafeTime -= 50*pVeh->m_fTimeModifier;
			if ( pVeh->m_fStrafeTime < 0 )
			{
				pVeh->m_fStrafeTime = 0.0f;
			}
		}
		else if ( pVeh->m_fStrafeTime < 0 )
		{
			pVeh->m_fStrafeTime += 50*pVeh->m_fTimeModifier;
			if ( pVeh->m_fStrafeTime > 0 )
			{
				pVeh->m_fStrafeTime = 0.0f;
			}
		}
	}
//STRAFING==============================================================================
#endif

	if ( parentPS->speed > speedMax )
	{
		parentPS->speed = speedMax;
	}
	else if ( parentPS->speed < speedMin )
	{
		parentPS->speed = speedMin;
	}

#ifdef QAGAME//FIXME: get working in GAME and CGAME
	if ((pVeh->m_vOrientation[PITCH]*0.1f) > 10.0f)
	{ //pitched downward, increase speed more and more based on our tilt
		if ( FighterIsInSpace( (gentity_t *)parent ) )
		{//in space, do nothing with speed base on pitch...
		}
		else
		{
			//really should only do this when on a planet
			float mult = pVeh->m_vOrientation[PITCH]*0.1f;
			if (mult < 1.0f)
			{
				mult = 1.0f;
			}
			parentPS->speed = PredictedAngularDecrement(mult, pVeh->m_fTimeModifier*10.0f, parentPS->speed);
		}
	}

	if (pVeh->m_iRemovedSurfaces
		|| parentPS->electrifyTime>=curTime)
	{ //going down
		if ( FighterIsInSpace( (gentity_t *)parent ) )
		{//we're in a valid trigger_space brush
			//simulate randomness
			if ( !(parent->s.number&3) )
			{//even multiple of 3, don't do anything
				parentPS->gravity = 0;
			}
			else if ( !(parent->s.number&2) )
			{//even multiple of 2, go up
				parentPS->gravity = -500.0f;
				parentPS->velocity[2] = 80.0f;
			}
			else
			{//odd number, go down
				parentPS->gravity = 500.0f;
				parentPS->velocity[2] = -80.0f;
			}
		}
		else
		{//over a planet
			parentPS->gravity = 500.0f;
			parentPS->velocity[2] = -80.0f;
		}
	}
	else if ( FighterSuspended( pVeh, parentPS ) )
	{
		parentPS->gravity = 0;
	}
	else if ( (!parentPS->speed||parentPS->speed < speedIdle) && pVeh->m_ucmd.upmove <= 0 )
	{//slowing down or stopped and not trying to take off
		if ( FighterIsInSpace( (gentity_t *)parent ) )
		{//we're in space, stopping doesn't make us drift downward
			if ( FighterOverValidLandingSurface( pVeh ) )
			{//well, there's something below us to land on, so go ahead and lower us down to it
				parentPS->gravity = (speedIdle - parentPS->speed)/4;
			}
		}
		else
		{//over a planet
			parentPS->gravity = (speedIdle - parentPS->speed)/4;
		}
	}
	else
	{
		parentPS->gravity = 0;
	}
#else//FIXME: get above checks working in GAME and CGAME
	parentPS->gravity = 0;
#endif

	/********************************************************************************/
	/*	END Here is where we move the vehicle (forward or back or whatever). END	*/
	/********************************************************************************/
}

extern void BG_VehicleTurnRateForSpeed( Vehicle_t *pVeh, float speed, float *mPitchOverride, float *mYawOverride );
static void FighterWingMalfunctionCheck( Vehicle_t *pVeh, playerState_t *parentPS )
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed( pVeh, parentPS->speed, &mPitchOverride, &mYawOverride );
	//check right wing damage
	if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_RIGHT_HEAVY)) )
	{//right wing has taken heavy damage
		pVeh->m_vOrientation[ROLL] += (sin( pVeh->m_ucmd.serverTime*0.001 )+1.0f)*pVeh->m_fTimeModifier*mYawOverride*50.0f;
	}
	else if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_RIGHT_LIGHT)) )
	{//right wing has taken light damage
		pVeh->m_vOrientation[ROLL] += (sin( pVeh->m_ucmd.serverTime*0.001 )+1.0f)*pVeh->m_fTimeModifier*mYawOverride*12.5f;
	}

	//check left wing damage
	if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_LEFT_HEAVY)) )
	{//left wing has taken heavy damage
		pVeh->m_vOrientation[ROLL] -= (sin( pVeh->m_ucmd.serverTime*0.001 )+1.0f)*pVeh->m_fTimeModifier*mYawOverride*50.0f;
	}
	else if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_LEFT_LIGHT)) )
	{//left wing has taken light damage
		pVeh->m_vOrientation[ROLL] -= (sin( pVeh->m_ucmd.serverTime*0.001 )+1.0f)*pVeh->m_fTimeModifier*mYawOverride*12.5f;
	}

}

static void FighterNoseMalfunctionCheck( Vehicle_t *pVeh, playerState_t *parentPS )
{
	float mPitchOverride = 1.0f;
	float mYawOverride = 1.0f;
	BG_VehicleTurnRateForSpeed( pVeh, parentPS->speed, &mPitchOverride, &mYawOverride );
	//check nose damage
	if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_FRONT_HEAVY)) )
	{//nose has taken heavy damage
		//pitch up and down over time
		pVeh->m_vOrientation[PITCH] += sin( pVeh->m_ucmd.serverTime*0.001 )*pVeh->m_fTimeModifier*mPitchOverride*50.0f;
	}
	else if ( (parentPS->brokenLimbs&(1<<SHIPSURF_DAMAGE_FRONT_LIGHT)) )
	{//nose has taken heavy damage
		//pitch up and down over time
		pVeh->m_vOrientation[PITCH] += sin( pVeh->m_ucmd.serverTime*0.001 )*pVeh->m_fTimeModifier*mPitchOverride*20.0f;
	}
}

static void FighterDamageRoutine( Vehicle_t *pVeh, bgEntity_t *parent, playerState_t *parentPS, playerState_t *riderPS, qboolean isDead )
{
 	if ( !pVeh->m_iRemovedSurfaces )
	{//still in one piece
		if ( pVeh->m_pParentEntity && isDead )
		{//death spiral
			pVeh->m_ucmd.upmove = 0;
			//FIXME: don't bias toward pitching down when not in space
			/*
			if ( FighterIsInSpace( pVeh->m_pParentEntity ) )
			{
			}
			else
			*/
			if ( !(pVeh->m_pParentEntity->s.number%3) )
			{//NOT everyone should do this
				pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
				if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
				{
					if ( pVeh->m_vOrientation[PITCH] > 60.0f )
					{
						pVeh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if ( !(pVeh->m_pParentEntity->s.number%2) )
			{
				pVeh->m_vOrientation[PITCH] -= pVeh->m_fTimeModifier;
				if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
				{
					if ( pVeh->m_vOrientation[PITCH] > -60.0f )
					{
						pVeh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			if ( (pVeh->m_pParentEntity->s.number%2) )
			{
				pVeh->m_vOrientation[YAW] += pVeh->m_fTimeModifier;
				pVeh->m_vOrientation[ROLL] += pVeh->m_fTimeModifier*4.0f;
			}
			else
			{
				pVeh->m_vOrientation[YAW] -= pVeh->m_fTimeModifier;
				pVeh->m_vOrientation[ROLL] -= pVeh->m_fTimeModifier*4.0f;
			}
		}
		return;
	}

	//if we get into here we have at least one broken piece
	pVeh->m_ucmd.upmove = 0;

	//if you're off the ground and not suspended, pitch down
	//FIXME: not in space!
	if ( pVeh->m_LandTrace.fraction >= 0.1f )
	{
		if ( !FighterSuspended( pVeh, parentPS ) )
		{
			//pVeh->m_ucmd.forwardmove = 0;
			//FIXME: don't bias towards pitching down when in space...
			if ( !(pVeh->m_pParentEntity->s.number%2) )
			{//NOT everyone should do this
				pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
				if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
				{
					if ( pVeh->m_vOrientation[PITCH] > 60.0f )
					{
						pVeh->m_vOrientation[PITCH] = 60.0f;
					}
				}
			}
			else if ( !(pVeh->m_pParentEntity->s.number%3) )
			{
				pVeh->m_vOrientation[PITCH] -= pVeh->m_fTimeModifier;
				if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
				{
					if ( pVeh->m_vOrientation[PITCH] > -60.0f )
					{
						pVeh->m_vOrientation[PITCH] = -60.0f;
					}
				}
			}
			//else: just keep going forward
		}
	}
#ifdef QAGAME
	if ( pVeh->m_LandTrace.fraction < 1.0f )
	{ //if you land at all when pieces of your ship are missing, then die
		gentity_t *parent = (gentity_t *)pVeh->m_pParentEntity;
		gentity_t *killer = parent;
#ifdef _JK2MP//only have this info in MP...
		if (parent->client->ps.otherKiller < ENTITYNUM_WORLD &&
			parent->client->ps.otherKillerTime > level.time)
		{
			gentity_t *potentialKiller = &g_entities[parent->client->ps.otherKiller];

			if (potentialKiller->inuse && potentialKiller->client)
			{ //he's valid I guess
				killer = potentialKiller;
			}
		}
#endif
		G_Damage(parent, killer, killer, vec3_origin, parent->client->ps.origin, 99999, DAMAGE_NO_ARMOR, MOD_SUICIDE);
	}
#endif

	if ( ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) ||
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D)) &&
		((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) ||
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)) )
	{ //wings on both side broken
		float factor = 2.0f;
		if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) &&
			(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F) &&
			(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) &&
			(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D))
		{ //all wings broken
			factor *= 2.0f;
		}

		if ( !(pVeh->m_pParentEntity->s.number%4)||!(pVeh->m_pParentEntity->s.number%5) )
		{//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] += (pVeh->m_fTimeModifier*factor); //do some spiralling
	}
	else if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) ||
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D))
	{ //left wing broken
		float factor = 2.0f;
		if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) &&
			(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D))
		{ //if both are broken..
			factor *= 2.0f;
		}

		if ( !(pVeh->m_pParentEntity->s.number%4)||!(pVeh->m_pParentEntity->s.number%5) )
		{//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] += factor*pVeh->m_fTimeModifier;
	}
	else if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) ||
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
	{ //right wing broken
		float factor = 2.0f;
		if ((pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) &&
			(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F))
		{ //if both are broken..
			factor *= 2.0f;
		}

		if ( !(pVeh->m_pParentEntity->s.number%4)||!(pVeh->m_pParentEntity->s.number%5) )
		{//won't yaw, so increase roll factor
			factor *= 4.0f;
		}

		pVeh->m_vOrientation[ROLL] -= factor*pVeh->m_fTimeModifier;
	}
}

#ifdef _JK2MP
void FighterYawAdjust(Vehicle_t *pVeh, playerState_t *riderPS, playerState_t *parentPS)
{
	float angDif = AngleSubtract(pVeh->m_vOrientation[YAW], riderPS->viewangles[YAW]);

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
		pVeh->m_vOrientation[YAW] = AngleNormalize180(pVeh->m_vOrientation[YAW] - angDif*(pVeh->m_fTimeModifier*0.2f));
	}
}

void FighterPitchAdjust(Vehicle_t *pVeh, playerState_t *riderPS, playerState_t *parentPS)
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

	bgEntity_t *parent = pVeh->m_pParentEntity;
	playerState_t *parentPS, *riderPS;
	float angleTimeMod;
#ifdef QAGAME
	const float groundFraction = 0.1f;
#endif
	float	curRoll = 0.0f;
	qboolean isDead = qfalse;
	qboolean isLandingOrLanded = qfalse;
#ifndef _JK2MP//SP
	int curTime = level.time;
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

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
	isDead = (qboolean)((parentPS->eFlags&EF_DEAD)!=0);
#else
	parentPS = &parent->client->ps;
	riderPS = &rider->client->ps;
	isDead = (qboolean)(parentPS->stats[STAT_HEALTH] <= 0 );
#endif

#ifdef _JK2MP
	if ( parentPS->hyperSpaceTime
		&& (curTime - parentPS->hyperSpaceTime) < HYPERSPACE_TIME )
	{//Going to Hyperspace
		VectorCopy( riderPS->viewangles, pVeh->m_vOrientation );
		VectorCopy( riderPS->viewangles, parentPS->viewangles );
		return;
	}
#endif

	if ( pVeh->m_iDropTime >= curTime )
	{//you can only YAW during this
		parentPS->viewangles[YAW] = pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
		return;
	}

	angleTimeMod = pVeh->m_fTimeModifier;

	if ( isDead || parentPS->electrifyTime>=curTime ||
		(pVeh->m_pVehicleInfo->surfDestruction &&
		pVeh->m_iRemovedSurfaces &&
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_C) &&
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_D) &&
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_E) &&
		(pVeh->m_iRemovedSurfaces & SHIPSURF_BROKEN_F)) )
	{ //do some special stuff for when all the wings are torn off
		FighterDamageRoutine(pVeh, parent, parentPS, riderPS, isDead);
		pVeh->m_vOrientation[ROLL] = AngleNormalize180( pVeh->m_vOrientation[ROLL] );
		return;
	}

	if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
	{
		pVeh->m_vOrientation[ROLL] = PredictedAngularDecrement(0.95f, angleTimeMod*2.0f, pVeh->m_vOrientation[ROLL]);
	}

	isLandingOrLanded = (qboolean)(FighterIsLanding( pVeh, parentPS )||FighterIsLanded( pVeh, parentPS ));

	if (!isLandingOrLanded)
	{ //don't do this stuff while landed.. I guess. I don't want ships spinning in place, looks silly.
		int m = 0;
		float aVelDif;
		float dForVel;

		FighterWingMalfunctionCheck( pVeh, parentPS );

		while (m < 3)
		{
			aVelDif = pVeh->m_vFullAngleVelocity[m];

			if (aVelDif != 0.0f)
			{
				dForVel = (aVelDif*0.1f)*pVeh->m_fTimeModifier;
				if (dForVel > 1.0f || dForVel < -1.0f)
				{
					pVeh->m_vOrientation[m] += dForVel;
					pVeh->m_vOrientation[m] = AngleNormalize180(pVeh->m_vOrientation[m]);
					if (m == PITCH)
					{ //don't pitch downward into ground even more.
						if (pVeh->m_vOrientation[m] > 90.0f && (pVeh->m_vOrientation[m]-dForVel) < 90.0f)
						{
							pVeh->m_vOrientation[m] = 90.0f;
							pVeh->m_vFullAngleVelocity[m] = -pVeh->m_vFullAngleVelocity[m];
						}
					}
					if (riderPS)
					{
						riderPS->viewangles[m] = pVeh->m_vOrientation[m];
					}
					pVeh->m_vFullAngleVelocity[m] -= dForVel;
				}
				else
				{
					pVeh->m_vFullAngleVelocity[m] = 0.0f;
				}
			}

			m++;
		}
	}
	else
	{ //clear decr/incr angles once landed.
		VectorClear(pVeh->m_vFullAngleVelocity);
	}

	curRoll = pVeh->m_vOrientation[ROLL];

	// If we're landed, we shouldn't be able to do anything but take off.
	if ( isLandingOrLanded //going slow enough to start landing
		&& !pVeh->m_iRemovedSurfaces
		&& parentPS->electrifyTime<curTime)//not spiraling out of control
	{
		if ( parentPS->speed > 0.0f )
		{//Uh... what?  Why?
			if ( pVeh->m_LandTrace.fraction < 0.3f )
			{
				pVeh->m_vOrientation[PITCH] = 0.0f;
			}
			else
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.83f, angleTimeMod*10.0f, pVeh->m_vOrientation[PITCH]);
			}
		}
		if ( pVeh->m_LandTrace.fraction > 0.1f
			|| pVeh->m_LandTrace.plane.normal[2] < MIN_LANDING_SLOPE )
		{//off the ground, at least (or not on a valid landing surf)
			// Dampen the turn rate based on the current height.
#ifdef _JK2MP
			FighterYawAdjust(pVeh, riderPS, parentPS);
#else
			pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];//*pVeh->m_LandTrace.fraction;
#endif
		}
	}
	else if ( (pVeh->m_iRemovedSurfaces||parentPS->electrifyTime>=curTime)//spiralling out of control
		&& (!(pVeh->m_pParentEntity->s.number%4)||!(pVeh->m_pParentEntity->s.number%5)) )
	{//no yaw control
	}
	else if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS && parentPS->speed > 0.0f )//&& !( pVeh->m_ucmd.forwardmove > 0 && pVeh->m_LandTrace.fraction != 1.0f ) )
	{
		if ( BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
		{
			VectorCopy( riderPS->viewangles, pVeh->m_vOrientation );
			VectorCopy( riderPS->viewangles, parentPS->viewangles );
#ifdef _JK2MP
			//BG_ExternThisSoICanRecompileInDebug( pVeh, riderPS );
#endif

			curRoll = pVeh->m_vOrientation[ROLL];

			FighterNoseMalfunctionCheck( pVeh, parentPS );

			//VectorCopy( pVeh->m_vOrientation, parentPS->viewangles );
		}
		else
		{
			/*
			float fTurnAmt[3];
			//PITCH
			fTurnAmt[PITCH] = riderPS->viewangles[PITCH] * 0.08f;
			//YAW
			fTurnAmt[YAW] = riderPS->viewangles[YAW] * 0.065f;
			fTurnAmt[YAW] *= fTurnAmt[YAW];
			// Dampen the turn rate based on the current height.
			if ( riderPS->viewangles[YAW] < 0 )
			{//must keep it negative because squaring a negative makes it positive
				fTurnAmt[YAW] = -fTurnAmt[YAW];
			}
			fTurnAmt[YAW] *= pVeh->m_LandTrace.fraction;
			//ROLL
			fTurnAmt[2] = 0.0f;
			*/

			//Actal YAW
#ifdef _JK2MP
			FighterYawAdjust(pVeh, riderPS, parentPS);
#else
			pVeh->m_vOrientation[YAW] = riderPS->viewangles[YAW];
#endif

			// If we are not hitting the ground, allow the fighter to pitch up and down.
			if ( !FighterOverValidLandingSurface( pVeh )
				|| parentPS->speed > MIN_LANDING_SPEED )
			//if ( ( pVeh->m_LandTrace.fraction >= 1.0f || pVeh->m_ucmd.forwardmove != 0 ) && pVeh->m_LandTrace.fraction >= 0.0f )
			{
				float fYawDelta;

#ifdef _JK2MP
				FighterPitchAdjust(pVeh, riderPS, parentPS);
#else
				pVeh->m_vOrientation[PITCH] = riderPS->viewangles[PITCH];
#endif

				FighterNoseMalfunctionCheck( pVeh, parentPS );

				// Adjust the roll based on the turn amount and dampen it a little.
				fYawDelta = AngleSubtract(pVeh->m_vOrientation[YAW], pVeh->m_vPrevOrientation[YAW]); //pVeh->m_vOrientation[YAW] - pVeh->m_vPrevOrientation[YAW];
				if ( fYawDelta > 8.0f )
				{
					fYawDelta = 8.0f;
				}
				else if ( fYawDelta < -8.0f )
				{
					fYawDelta = -8.0f;
				}
				curRoll -= fYawDelta;

				curRoll = PredictedAngularDecrement(0.93f, angleTimeMod*2.0f, curRoll);
				//cap it reasonably
				//NOTE: was hardcoded to 40.0f, now using extern data
				if ( pVeh->m_pVehicleInfo->rollLimit != -1 )
				{
					if (curRoll > pVeh->m_pVehicleInfo->rollLimit )
					{
						curRoll = pVeh->m_pVehicleInfo->rollLimit;
					}
					else if (curRoll < -pVeh->m_pVehicleInfo->rollLimit)
					{
						curRoll = -pVeh->m_pVehicleInfo->rollLimit;
					}
				}
			}
		}
	}

	// If you are directly impacting the ground, even out your pitch.
	if ( isLandingOrLanded )
	{//only if capable of landing
		if ( !isDead
			&& parentPS->electrifyTime<curTime
			&& (!pVeh->m_pVehicleInfo->surfDestruction || !pVeh->m_iRemovedSurfaces ) )
		{//not crashing or spiralling out of control...
			if ( pVeh->m_vOrientation[PITCH] > 0 )
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.2f, angleTimeMod*10.0f, pVeh->m_vOrientation[PITCH]);
			}
			else
			{
				pVeh->m_vOrientation[PITCH] = PredictedAngularDecrement(0.75f, angleTimeMod*10.0f, pVeh->m_vOrientation[PITCH]);
			}
		}
	}


/*
//NOTE: all this is redundant now since we have the FighterDamageRoutine func...
#ifdef _JK2MP //...yeah. Need to send armor across net for prediction to work.
	if ( isDead )
#else
	if ( pVeh->m_iArmor <= 0 )
#endif
	{//going to explode
		//FIXME: maybe make it erratically jerk or spin or start and stop?
#ifndef _JK2MP
		if ( g_speederControlScheme->value > 0 || !rider || rider->s.number )
#else
		if (1)
#endif
		{
			pVeh->m_ucmd.rightmove = Q_irand( -64, 64 );
		}
		else
		{
			pVeh->m_ucmd.rightmove = 0;
		}
		pVeh->m_ucmd.forwardmove = Q_irand( -32, 127 );
		pVeh->m_ucmd.upmove = Q_irand( -127, 127 );
		pVeh->m_vOrientation[YAW] += Q_flrand( -10, 10 );
		pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
		if ( pVeh->m_vOrientation[PITCH] > 60.0f )
		{
			pVeh->m_vOrientation[PITCH] = 60.0f;
		}
		if ( pVeh->m_LandTrace.fraction != 0.0f )
		{
			parentPS->velocity[2] -= pVeh->m_pVehicleInfo->acceleration * pVeh->m_fTimeModifier;
		}
	}
*/
	// If no one is in this vehicle and it's up in the sky, pitch it forward as it comes tumbling down.
#ifdef QAGAME //never gonna happen on client anyway, we can't be getting predicted unless the predicting client is boarded
 	if ( !pVeh->m_pVehicleInfo->Inhabited( pVeh )
		&& pVeh->m_LandTrace.fraction >= groundFraction
		&& !FighterIsInSpace( (gentity_t *)parent )
		&& !FighterSuspended( pVeh, parentPS ) )
	{
		pVeh->m_ucmd.upmove = 0;
		//pVeh->m_ucmd.forwardmove = 0;
		pVeh->m_vOrientation[PITCH] += pVeh->m_fTimeModifier;
		if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
		{
			if ( pVeh->m_vOrientation[PITCH] > 60.0f )
			{
				pVeh->m_vOrientation[PITCH] = 60.0f;
			}
		}
	}
#endif

	if ( !pVeh->m_fStrafeTime )
	{//use that roll
		pVeh->m_vOrientation[ROLL] = curRoll;
		//NOTE: this seems really backwards...
		if ( pVeh->m_vOrientation[ROLL] )
		{ //continually adjust the yaw based on the roll..
			if ( (pVeh->m_iRemovedSurfaces||parentPS->electrifyTime>=curTime)//spiralling out of control
				&& (!(pVeh->m_pParentEntity->s.number%4)||!(pVeh->m_pParentEntity->s.number%5)) )
			{//leave YAW alone
			}
			else
			{
				if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
				{
					pVeh->m_vOrientation[YAW] -= ((pVeh->m_vOrientation[ROLL])*0.05f)*pVeh->m_fTimeModifier;
				}
			}
		}
	}
	else
	{//add in strafing roll
		float strafeRoll = (pVeh->m_fStrafeTime/MAX_STRAFE_TIME)*pVeh->m_pVehicleInfo->rollLimit;//pVeh->m_pVehicleInfo->bankingSpeed*
		float strafeDif = AngleSubtract(strafeRoll, pVeh->m_vOrientation[ROLL]);
		pVeh->m_vOrientation[ROLL] += (strafeDif*0.1f)*pVeh->m_fTimeModifier;
		if ( !BG_UnrestrainedPitchRoll( riderPS, pVeh ) )
		{//cap it reasonably
			if ( pVeh->m_pVehicleInfo->rollLimit != -1
				&& !pVeh->m_iRemovedSurfaces
				&& parentPS->electrifyTime<curTime)
			{
				if (pVeh->m_vOrientation[ROLL] > pVeh->m_pVehicleInfo->rollLimit )
				{
					pVeh->m_vOrientation[ROLL] = pVeh->m_pVehicleInfo->rollLimit;
				}
				else if (pVeh->m_vOrientation[ROLL] < -pVeh->m_pVehicleInfo->rollLimit)
				{
					pVeh->m_vOrientation[ROLL] = -pVeh->m_pVehicleInfo->rollLimit;
				}
			}
		}
	}

	if (pVeh->m_pVehicleInfo->surfDestruction)
	{
		FighterDamageRoutine(pVeh, parent, parentPS, riderPS, isDead);
	}
	pVeh->m_vOrientation[ROLL] = AngleNormalize180( pVeh->m_vOrientation[ROLL] );
	/********************************************************************************/
	/*	END	Here is where make sure the vehicle is properly oriented.	END			*/
	/********************************************************************************/
}

#ifdef QAGAME //ONLY in SP or on server, not cgame

extern void PM_SetAnim(pmove_t	*pm,int setAnimParts,int anim,int setAnimFlags, int blendTime);

// This function makes sure that the vehicle is properly animated.
static void AnimateVehicle( Vehicle_t *pVeh )
{
	int Anim = -1;
	int iFlags = SETANIM_FLAG_NORMAL, iBlend = 300;
	qboolean isLanding = qfalse, isLanded = qfalse;
#ifdef _JK2MP
	playerState_t *parentPS = pVeh->m_pParentEntity->playerState;
#else
	playerState_t *parentPS = &pVeh->m_pParentEntity->client->ps;
#endif
#ifndef _JK2MP//SP
	//nothing
#elif defined QAGAME//MP GAME
	int curTime = level.time;
#elif defined CGAME//MP CGAME
	//FIXME: pass in ucmd?  Not sure if this is reliable...
	int curTime = pm->cmd.serverTime;
#endif

#ifdef _JK2MP
	if ( parentPS->hyperSpaceTime
		&& curTime - parentPS->hyperSpaceTime < HYPERSPACE_TIME )
	{//Going to Hyperspace
		//close the wings (FIXME: makes sense on X-Wing, not Shuttle?)
		if ( pVeh->m_ulFlags & VEH_WINGSOPEN )
		{
			pVeh->m_ulFlags &= ~VEH_WINGSOPEN;
			Anim = BOTH_WINGS_CLOSE;
		}
	}
	else
#endif
	{
		isLanding = FighterIsLanding( pVeh, parentPS );
		isLanded = FighterIsLanded( pVeh, parentPS );

		// if we're above launch height (way up in the air)...
		if ( !isLanding && !isLanded )
		{
			if ( !( pVeh->m_ulFlags & VEH_WINGSOPEN ) )
			{
				pVeh->m_ulFlags |= VEH_WINGSOPEN;
				pVeh->m_ulFlags &= ~VEH_GEARSOPEN;
				Anim = BOTH_WINGS_OPEN;
			}
		}
		// otherwise we're below launch height and still taking off.
		else
		{
			if ( (pVeh->m_ucmd.forwardmove < 0 || pVeh->m_ucmd.upmove < 0||isLanded)
				&& pVeh->m_LandTrace.fraction <= 0.4f
				&& pVeh->m_LandTrace.plane.normal[2] >= MIN_LANDING_SLOPE )
			{//already landed or trying to land and close to ground
				// Open gears.
				if ( !( pVeh->m_ulFlags & VEH_GEARSOPEN ) )
				{
#ifdef _JK2MP
					if ( pVeh->m_pVehicleInfo->soundLand )
					{//just landed?
#ifdef QAGAME//MP GAME-side
						G_EntitySound( ((gentity_t *)(pVeh->m_pParentEntity)), CHAN_AUTO, pVeh->m_pVehicleInfo->soundLand );
#elif defined CGAME//MP CGAME-side
						//trap_S_StartSound( NULL, pVeh->m_pParentEntity->s.number, CHAN_AUTO, pVeh->m_pVehicleInfo->soundLand );
#endif
					}
#endif
					pVeh->m_ulFlags |= VEH_GEARSOPEN;
					Anim = BOTH_GEARS_OPEN;
				}
			}
			else
			{//trying to take off and almost halfway off the ground
				// Close gears (if they're open).
				if ( pVeh->m_ulFlags & VEH_GEARSOPEN )
				{
					pVeh->m_ulFlags &= ~VEH_GEARSOPEN;
					Anim = BOTH_GEARS_CLOSE;
					//iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
				}
				// If gears are closed, and we are below launch height, close the wings.
				else
				{
					if ( pVeh->m_ulFlags & VEH_WINGSOPEN )
					{
						pVeh->m_ulFlags &= ~VEH_WINGSOPEN;
						Anim = BOTH_WINGS_CLOSE;
					}
				}
			}
		}
	}

	if ( Anim != -1 )
	{
		#ifdef _JK2MP
			BG_SetAnim(pVeh->m_pParentEntity->playerState, bgAllAnims[pVeh->m_pParentEntity->localAnimIndex].anims,
				SETANIM_BOTH, Anim, iFlags, iBlend);
		#else
			NPC_SetAnim( pVeh->m_pParentEntity, SETANIM_BOTH, Anim, iFlags, iBlend );
		#endif
	}
}

// This function makes sure that the rider's in this vehicle are properly animated.
static void AnimateRiders( Vehicle_t *pVeh )
{
}

#endif //game-only

#ifndef QAGAME
void AttachRidersGeneric( Vehicle_t *pVeh );
#endif

void G_SetFighterVehicleFunctions( vehicleInfo_t *pVehInfo )
{
#ifdef QAGAME //ONLY in SP or on server, not cgame
	pVehInfo->AnimateVehicle			=		AnimateVehicle;
	pVehInfo->AnimateRiders				=		AnimateRiders;
//	pVehInfo->ValidateBoard				=		ValidateBoard;
//	pVehInfo->SetParent					=		SetParent;
//	pVehInfo->SetPilot					=		SetPilot;
//	pVehInfo->AddPassenger				=		AddPassenger;
//	pVehInfo->Animate					=		Animate;
	pVehInfo->Board						=		Board;
	pVehInfo->Eject						=		Eject;
//	pVehInfo->EjectAll					=		EjectAll;
//	pVehInfo->StartDeathDelay			=		StartDeathDelay;
//	pVehInfo->DeathUpdate				=		DeathUpdate;
//	pVehInfo->RegisterAssets			=		RegisterAssets;
//	pVehInfo->Initialize				=		Initialize;
	pVehInfo->Update					=		Update;
//	pVehInfo->UpdateRider				=		UpdateRider;
#endif //game-only
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
void G_CreateFighterNPC( Vehicle_t **pVeh, const char *strType )
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
#else
	(*pVeh) = (Vehicle_t *) gi.Malloc( sizeof(Vehicle_t), TAG_G_ALLOC, qtrue );
#endif
	(*pVeh)->m_pVehicleInfo = &g_vehicleInfo[BG_VehicleGetIndex( strType )];
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
