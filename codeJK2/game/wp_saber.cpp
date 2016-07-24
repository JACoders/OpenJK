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

#include "g_headers.h"

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "../../code/qcommon/tri_coll_test.h"

#define MAX_SABER_VICTIMS 16
static int		victimEntityNum[MAX_SABER_VICTIMS];
static float	totalDmg[MAX_SABER_VICTIMS];
static vec3_t	dmgDir[MAX_SABER_VICTIMS];
static vec3_t	dmgSpot[MAX_SABER_VICTIMS];
static float	dmgFraction[MAX_SABER_VICTIMS];
static int		hitLoc[MAX_SABER_VICTIMS];
static qboolean	hitDismember[MAX_SABER_VICTIMS];
static int		hitDismemberLoc[MAX_SABER_VICTIMS];
static vec3_t	saberHitLocation, saberHitNormal={0,0,1.0};
static float	saberHitFraction;
static float	sabersCrossed;
static int		saberHitEntity;
static int		numVictims = 0;

#define SABER_PITCH_HACK 90


extern cvar_t	*g_timescale;
extern cvar_t	*g_dismemberment;

extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern qboolean		G_ClearViewEntity( gentity_t *ent );
extern void			G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
extern qboolean G_ControlledByPlayer( gentity_t *self );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void CG_ChangeWeapon( int num );
extern void G_AngerAlert( gentity_t *self );
extern void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward );
extern int G_CheckLedgeDive( gentity_t *self, float checkDist, vec3_t checkVel, qboolean tryOpposite, qboolean tryPerp );
extern void G_BounceMissile( gentity_t *ent, trace_t *trace );
extern qboolean G_PointInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone );
extern void WP_FireDreadnoughtBeam( gentity_t *ent );
extern void G_MissileImpacted( gentity_t *ent, gentity_t *other, vec3_t impactPos, vec3_t normal, int hitLoc=HL_NONE );
extern evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist = 0.0f );
extern int PM_PickAnim( gentity_t *self, int minAnim, int maxAnim );
extern void NPC_SetPainEvent( gentity_t *self );
extern qboolean PM_SwimmingAnim( int anim );
extern qboolean PM_InAnimForSaberMove( int anim, int saberMove );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SaberInAttack( int move );
extern qboolean PM_SaberInTransition( int move );
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransitionAny( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SpinningSaberAnim( int anim );
extern int PM_SaberBounceForAttack( int move );
extern int PM_BrokenParryForAttack( int move );
extern int PM_KnockawayForParry( int move );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean PM_SaberInIdle( int move );
extern qboolean PM_SaberInReflect( int move );
extern qboolean PM_InSpecialJump( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern int PM_PowerLevelForSaberAnim( playerState_t *ps );
extern void PM_VelocityForSaberMove( playerState_t *ps, vec3_t throwDir );
extern qboolean PM_VelocityForBlockedMove( playerState_t *ps, vec3_t throwDir );
extern int Jedi_ReCalcParryTime( gentity_t *self, evasionType_t evasionType );
extern qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc );
extern void Jedi_PlayDeflectSound( gentity_t *self );
extern void Jedi_PlayBlockedPushSound( gentity_t *self );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void Jedi_Ambush( gentity_t *self );
extern qboolean Jedi_SaberBusy( gentity_t *self );

void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
void WP_SaberInFlightReflectCheck( gentity_t *self, usercmd_t *ucmd  );

void WP_SaberDrop( gentity_t *self, gentity_t *saber );
qboolean WP_SaberLose( gentity_t *self, vec3_t throwDir );
void WP_SaberReturn( gentity_t *self, gentity_t *saber );
void WP_SaberBlock( gentity_t *saber, vec3_t hitloc, qboolean missleBlock );
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
void ForceThrow( gentity_t *self, qboolean pull );
qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );

extern cvar_t	*g_saberAutoBlocking;
extern cvar_t	*g_saberRealisticCombat;
extern int g_crosshairEntNum;

int		g_saberFlashTime = 0;
vec3_t	g_saberFlashPos = {0,0,0};

int forcePowerNeeded[NUM_FORCE_POWERS] =
{
	0,//FP_HEAL,//instant
	10,//FP_LEVITATION,//hold/duration
	50,//FP_SPEED,//duration
	15,//FP_PUSH,//hold/duration
	15,//FP_PULL,//hold/duration
	20,//FP_TELEPATHY,//instant
	1,//FP_GRIP,//hold/duration - FIXME: 30?
	1,//FP_LIGHTNING,//hold/duration
	20,//FP_SABERTHROW,
	1,//FP_SABER_DEFENSE,
	0,//FP_SABER_OFFENSE,
	//NUM_FORCE_POWERS
};

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] =
{
	JUMP_VELOCITY,//normal jump
	420,
	590,
	840
};

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] =
{
	32,//normal jump (+stepheight+crouchdiff = 66)
	96,//(+stepheight+crouchdiff = 130)
	192,//(+stepheight+crouchdiff = 226)
	384//(+stepheight+crouchdiff = 418)
};

float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] =
{
	66,//normal jump (32+stepheight(18)+crouchdiff(24) = 74)
	130,//(96+stepheight(18)+crouchdiff(24) = 138)
	226,//(192+stepheight(18)+crouchdiff(24) = 234)
	418//(384+stepheight(18)+crouchdiff(24) = 426)
};

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	384,//256,
	448,//384,
	512
};

float forcePushCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	0.8f,
	0.6f
};

float forcePullCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	1.0f,
	0.8f
};

float forceSpeedValue[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	0.75f,
	0.5f,
	0.25f
};

float forceSpeedRangeMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//none
	30.0f,
	45.0f,
	60.0f
};

float forceSpeedFOVMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//none
	20.0f,
	30.0f,
	40.0f
};

int forceGripDamage[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	0,
	6,
	9
};

int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	5000,
	10000,
	15000
};

//NOTE: keep in synch with table below!!!
int saberThrowDist[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	256,
	400,
	400
};

//NOTE: keep in synch with table above!!!
int saberThrowDistSquared[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	65536,
	160000,
	160000
};

int parryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	1000000,//if don't even have defense, can't use defense!
	300,
	150,
	50
};

float saberAnimSpeedMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//if don't even have offense, can't use offense!
	0.75f,
	1.0f,
	2.0f
};
//SABER INITIALIZATION======================================================================

void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *psWeaponModel )
{
	if (!psWeaponModel)
	{
		assert (psWeaponModel);
		return;
	}
	if ( ent && ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{//hack for galakmech, no weaponmodel
		ent->weaponModel = -1;
		return;
	}

	char weaponModel[MAX_QPATH];
	Q_strncpyz(weaponModel, psWeaponModel, sizeof(weaponModel));
	if (char *spot = (char*)Q_stristr(weaponModel, ".md3") ) {
        *spot = 0;
		spot = (char*)Q_stristr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
		if (!spot&&!Q_stristr(weaponModel, "noweap"))
		{
			Q_strcat (weaponModel, sizeof(weaponModel), "_w");
		}
		Q_strcat (weaponModel, sizeof(weaponModel), ".glm");	//and change to ghoul2
	}

	if ( ent->playerModel == -1 )
	{
		return;
	}
	// give us a sabre model
	ent->weaponModel = gi.G2API_InitGhoul2Model(ent->ghoul2, weaponModel, G_ModelIndex( weaponModel ), NULL_HANDLE, NULL_HANDLE, 0, 0);
	if ( ent->weaponModel != -1 )
	{
		// attach it to the hand
		gi.G2API_AttachG2Model(&ent->ghoul2[ent->weaponModel], &ent->ghoul2[ent->playerModel],
					ent->handRBolt, ent->playerModel);
		// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
		gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel], "*flash");
	  	//gi.G2API_SetLodBias( &ent->ghoul2[ent->weaponModel], 0 );
	}
}

//----------------------------------------------------------
void G_Throw( gentity_t *targ, vec3_t newDir, float push )
//----------------------------------------------------------
{
	vec3_t	kvel;
	float	mass;

	if ( targ->physicsBounce > 0 )	//overide the mass
	{
		mass = targ->physicsBounce;
	}
	else
	{
		mass = 200;
	}

	if ( g_gravity->value > 0 )
	{
		VectorScale( newDir, g_knockback->value * (float)push / mass * 0.8, kvel );
		kvel[2] = newDir[2] * g_knockback->value * (float)push / mass * 1.5;
	}
	else
	{
		VectorScale( newDir, g_knockback->value * (float)push / mass, kvel );
	}

	if ( targ->client )
	{
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );
	}
	else if ( targ->s.pos.trType != TR_STATIONARY && targ->s.pos.trType != TR_LINEAR_STOP && targ->s.pos.trType != TR_NONLINEAR_STOP )
	{
		VectorAdd( targ->s.pos.trDelta, kvel, targ->s.pos.trDelta );
		VectorCopy( targ->currentOrigin, targ->s.pos.trBase );
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if ( targ->client && !targ->client->ps.pm_time )
	{
		int		t;

		t = push * 2;

		if ( t < 50 )
		{
			t = 50;
		}
		if ( t > 200 )
		{
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

int WP_SetSaberModel( gclient_t *client, class_t npcClass )
{
	if ( client )
	{
		switch ( npcClass )
		{
		case CLASS_DESANN://Desann
			client->ps.saberModel = "models/weapons2/saber_desann/saber_w.glm";
			break;
		case CLASS_LUKE://Luke
			client->ps.saberModel = "models/weapons2/saber_luke/saber_w.glm";
			break;
		case CLASS_KYLE://Kyle NPC and player
			client->ps.saberModel = "models/weapons2/saber/saber_w.glm";
			break;
		default://reborn and tavion and everyone else
			client->ps.saberModel = "models/weapons2/saber_reborn/saber_w.glm";
			break;
		}
		return ( G_ModelIndex( client->ps.saberModel ) );
	}
	else
	{
		switch ( npcClass )
		{
		case CLASS_DESANN://Desann
			return ( G_ModelIndex( "models/weapons2/saber_desann/saber_w.glm" ) );
			break;
		case CLASS_LUKE://Luke
			return ( G_ModelIndex( "models/weapons2/saber_luke/saber_w.glm" ) );
			break;
		case CLASS_KYLE://Kyle NPC and player
			return ( G_ModelIndex( "models/weapons2/saber/saber_w.glm" ) );
			break;
		default://reborn and tavion and everyone else
			return ( G_ModelIndex( "models/weapons2/saber_reborn/saber_w.glm" ) );
			break;
		}
	}
}

void WP_SaberInitBladeData( gentity_t *ent )
{
	gentity_t *saberent;

	if ( ent->client )
	{
		VectorClear( ent->client->renderInfo.muzzlePoint );
		VectorClear( ent->client->renderInfo.muzzlePointOld );
		//VectorClear( ent->client->renderInfo.muzzlePointNext );
		VectorClear( ent->client->renderInfo.muzzleDir );
		VectorClear( ent->client->renderInfo.muzzleDirOld );
		//VectorClear( ent->client->renderInfo.muzzleDirNext );
		ent->client->ps.saberLengthOld = ent->client->ps.saberLength = 0;
		ent->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		ent->client->ps.saberLockTime = 0;
		if ( ent->s.number )
		{
			if ( ent->client->NPC_class == CLASS_DESANN )
			{
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_4;
			}
			else if ( ent->client->NPC_class == CLASS_TAVION )
			{
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_5;
			}
			else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && (ent->NPC->rank == RANK_CIVILIAN || ent->NPC->rank == RANK_LT_JG) )
			{//grunt and fencer always uses quick attacks
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_1;
			}
			else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && (ent->NPC->rank == RANK_CREWMAN || ent->NPC->rank == RANK_ENSIGN) )
			{//acrobat & force-users always use medium attacks
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_2;
			}
			else if ( ent->client->playerTeam == TEAM_ENEMY && ent->client->NPC_class == CLASS_SHADOWTROOPER )
			{//shadowtroopers
				ent->client->ps.saberAnimLevel = Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 );
			}
			else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && ent->NPC->rank == RANK_LT )
			{//boss always starts with strong attacks
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_3;
			}
			else if ( ent->client->NPC_class == CLASS_KYLE )
			{
				ent->client->ps.saberAnimLevel = g_entities[0].client->ps.saberAnimLevel;
			}
			else
			{//?
				ent->client->ps.saberAnimLevel = Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 );
			}
		}
		else
		{
			if ( !ent->client->ps.saberAnimLevel )
			{//initialize, but don't reset
				ent->client->ps.saberAnimLevel = FORCE_LEVEL_2;
			}
			cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel;
			if ( ent->client->sess.missionStats.weaponUsed[WP_SABER] <= 0 )
			{//let missionStats know that we actually do have the saber, even if we never use it
				ent->client->sess.missionStats.weaponUsed[WP_SABER] = 1;
			}
		}
		ent->client->ps.saberAttackChainCount = 0;
		if ( ent->client->NPC_class == CLASS_DESANN )
		{//longer saber
			ent->client->ps.saberLengthMax = 48;
		}
		else if ( ent->client->NPC_class == CLASS_REBORN )
		{//shorter saber
			ent->client->ps.saberLengthMax = 32;
		}
		else
		{//standard saber length
			ent->client->ps.saberLengthMax = 40;
		}

		if ( ent->client->ps.saberEntityNum <= 0 || ent->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
		{
			saberent = G_Spawn();
			ent->client->ps.saberEntityNum = saberent->s.number;
			saberent->classname = "lightsaber";

			saberent->s.eType = ET_GENERAL;
			saberent->svFlags = SVF_USE_CURRENT_ORIGIN;
			saberent->s.weapon = WP_SABER;
			saberent->owner = ent;
			saberent->s.otherEntityNum = ent->s.number;

			saberent->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;
			saberent->contents = CONTENTS_LIGHTSABER;//|CONTENTS_SHOTCLIP;

			VectorSet( saberent->mins, -3.0f, -3.0f, -3.0f );
			VectorSet( saberent->maxs, 3.0f, 3.0f, 3.0f );
			saberent->mass = 10;//necc?

			saberent->s.eFlags |= EF_NODRAW;
			saberent->svFlags |= SVF_NOCLIENT;
/*
Ghoul2 Insert Start
*/
			//FIXME: get saberModel from NPCs.cfg for NPCs?
			saberent->s.modelindex = WP_SetSaberModel( ent->client, ent->client->NPC_class );
			gi.G2API_InitGhoul2Model( saberent->ghoul2, ent->client->ps.saberModel, saberent->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt( &saberent->ghoul2[0], "*flash" );
			//gi.G2API_SetLodBias( &saberent->ghoul2[0], 0 );

/*
Ghoul2 Insert End
*/

			ent->client->ps.saberInFlight = qfalse;
			ent->client->ps.saberEntityDist = 0;
			ent->client->ps.saberEntityState = SES_LEAVING;

			ent->client->ps.saberMove = 0;

			//FIXME: need a think function to create alerts when turned on or is on, etc.
		}
	}
	else
	{
		ent->client->ps.saberEntityNum = ENTITYNUM_NONE;
		ent->client->ps.saberInFlight = qfalse;
		ent->client->ps.saberEntityDist = 0;
		ent->client->ps.saberEntityState = SES_LEAVING;
	}
}

void WP_SaberUpdateOldBladeData( gentity_t *ent )
{
	if ( ent->client )
	{
		VectorCopy( ent->client->renderInfo.muzzlePoint, ent->client->renderInfo.muzzlePointOld );
		VectorCopy( ent->client->renderInfo.muzzleDir, ent->client->renderInfo.muzzleDirOld );
		if ( ent->client->ps.saberLengthOld <= 0 && ent->client->ps.saberLength > 0 )
		{//just turned on
			//do sound event
			vec3_t	saberOrg;
			VectorCopy( g_entities[ent->client->ps.saberEntityNum].currentOrigin, saberOrg );
			AddSoundEvent( ent, saberOrg, 256, AEL_SUSPICIOUS );
		}
		ent->client->ps.saberLengthOld = ent->client->ps.saberLength;
	}
}



//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
int WPDEBUG_SaberColor( saber_colors_t saberColor )
{
	switch( (int)(saberColor) )
	{
		case SABER_RED:
			return 0x000000ff;
			break;
		case SABER_ORANGE:
			return 0x000088ff;
			break;
		case SABER_YELLOW:
			return 0x0000ffff;
			break;
		case SABER_GREEN:
			return 0x0000ff00;
			break;
		case SABER_BLUE:
			return 0x00ff0000;
			break;
		case SABER_PURPLE:
			return 0x00ff00ff;
			break;
		default:
			return 0x00ffffff;//white
			break;
	}
}

qboolean WP_GetSaberDeflectionAngle( gentity_t *attacker, gentity_t *defender )
{
	vec3_t	temp, att_SaberBase, att_StartPos, saberMidNext, att_HitDir, att_HitPos, def_BladeDir;
	float	att_SaberHitLength, hitDot;

	if ( !attacker || !attacker->client || attacker->client->ps.saberInFlight || attacker->client->ps.saberLength <= 0 )
	{
		return qfalse;
	}
	if ( !defender || !defender->client || defender->client->ps.saberInFlight || defender->client->ps.saberLength <= 0 )
	{
		return qfalse;
	}

	attacker->client->ps.saberBounceMove = LS_NONE;

	//get the attacker's saber base pos at time of impact
	VectorSubtract( attacker->client->renderInfo.muzzlePoint, attacker->client->renderInfo.muzzlePointOld, temp );
	VectorMA( attacker->client->renderInfo.muzzlePointOld, saberHitFraction, temp, att_SaberBase );

	//get the position along the length of the blade where the hit occured
	att_SaberHitLength = Distance( saberHitLocation, att_SaberBase )/attacker->client->ps.saberLength;

	//now get the start of that midpoint in the swing and the actual impact point in the swing (shouldn't the latter just be saberHitLocation?)
	VectorMA( attacker->client->renderInfo.muzzlePointOld, att_SaberHitLength, attacker->client->renderInfo.muzzleDirOld, att_StartPos );
	VectorMA( attacker->client->renderInfo.muzzlePoint, att_SaberHitLength, attacker->client->renderInfo.muzzleDir, saberMidNext );
	VectorSubtract( saberMidNext, att_StartPos, att_HitDir );
	VectorMA( att_StartPos, saberHitFraction, att_HitDir, att_HitPos );
	VectorNormalize( att_HitDir );

	//get the defender's saber dir at time of impact
	VectorSubtract( defender->client->renderInfo.muzzleDirOld, defender->client->renderInfo.muzzleDir, temp );
	VectorMA( defender->client->renderInfo.muzzleDirOld, saberHitFraction, temp, def_BladeDir );

	//now compare
	hitDot = DotProduct( att_HitDir, def_BladeDir );
	if ( hitDot < 0.25f && hitDot > -0.25f )
	{//hit pretty much perpendicular, pop straight back
		attacker->client->ps.saberBounceMove = PM_SaberBounceForAttack( attacker->client->ps.saberMove );
		return qfalse;
	}
	else
	{//a deflection
		vec3_t	att_Right, att_Up, att_DeflectionDir;
		float	swingRDot, swingUDot;

		//get the direction of the deflection
		VectorScale( def_BladeDir, hitDot, att_DeflectionDir );
		//get our bounce straight back direction
		VectorScale( att_HitDir, -1.0f, temp );
		//add the bounce back and deflection
		VectorAdd( att_DeflectionDir, temp, att_DeflectionDir );
		//normalize the result to determine what direction our saber should bounce back toward
		VectorNormalize( att_DeflectionDir );

		//need to know the direction of the deflectoin relative to the attacker's facing
		VectorSet( temp, 0, attacker->client->ps.viewangles[YAW], 0 );//presumes no pitch!
		AngleVectors( temp, NULL, att_Right, att_Up );
		swingRDot = DotProduct( att_Right, att_DeflectionDir );
		swingUDot = DotProduct( att_Up, att_DeflectionDir );

		if ( swingRDot > 0.25f )
		{//deflect to right
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_TR;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_BR;
			}
			else
			{//deflect horizontally
				attacker->client->ps.saberBounceMove = LS_D1__R;
			}
		}
		else if ( swingRDot < -0.25f )
		{//deflect to left
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_TL;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_BL;
			}
			else
			{//deflect horizontally
				attacker->client->ps.saberBounceMove = LS_D1__L;
			}
		}
		else
		{//deflect in middle
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_T_;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_B_;
			}
			else
			{//deflect horizontally?  Well, no such thing as straight back in my face, so use top
				if ( swingRDot > 0 )
				{
					attacker->client->ps.saberBounceMove = LS_D1_TR;
				}
				else if ( swingRDot < 0 )
				{
					attacker->client->ps.saberBounceMove = LS_D1_TL;
				}
				else
				{
					attacker->client->ps.saberBounceMove = LS_D1_T_;
				}
			}
		}
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			gi.Printf( S_COLOR_BLUE"%s deflected from %s to %s\n", attacker->targetname, saberMoveData[attacker->client->ps.saberMove].name, saberMoveData[attacker->client->ps.saberBounceMove].name );
		}
#endif
		return qtrue;
	}
}


void WP_SaberClearDamageForEntNum( int entityNum )
{
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		if ( entityNum )
		{
			Com_Printf( "clearing damage for entnum %d\n", entityNum );
		}
	}
#endif// FINAL_BUILD
	if ( g_saberRealisticCombat->integer > 1 )
	{
		return;
	}
	for ( int i = 0; i < numVictims; i++ )
	{
		if ( victimEntityNum[i] == entityNum )
		{
			totalDmg[i] = 0;//no damage
			hitLoc[i] = HL_NONE;
			hitDismemberLoc[i] = HL_NONE;
			hitDismember[i] = qfalse;
			victimEntityNum[i] = ENTITYNUM_NONE;//like we never hit him
		}
	}
}

extern float damageModifier[];
extern float hitLocHealthPercentage[];
qboolean WP_SaberApplyDamage( gentity_t *ent, float baseDamage, int baseDFlags, qboolean brokenParry )
{
	qboolean	didDamage = qfalse;
	gentity_t	*victim;
	int			dFlags = baseDFlags;
	float		maxDmg;


	if ( !numVictims )
	{
		return qfalse;
	}
	for ( int i = 0; i < numVictims; i++ )
	{
		dFlags = baseDFlags|DAMAGE_DEATH_KNOCKBACK|DAMAGE_NO_HIT_LOC;
		if ( victimEntityNum[i] != ENTITYNUM_NONE && &g_entities[victimEntityNum[i]] != NULL )
		{	// Don't bother with this damage if the fraction is higher than the saber's fraction
			if ( dmgFraction[i] < saberHitFraction || brokenParry )
			{
				victim = &g_entities[victimEntityNum[i]];
				if ( !victim )
				{
					continue;
				}

				if ( victim->e_DieFunc == dieF_maglock_die )
				{//*sigh*, special check for maglocks
					vec3_t testFrom;
					if ( ent->client->ps.saberInFlight )
					{
						VectorCopy( g_entities[ent->client->ps.saberEntityNum].currentOrigin, testFrom );
					}
					else
					{
						VectorCopy( ent->currentOrigin, testFrom );
					}
					testFrom[2] = victim->currentOrigin[2];
					trace_t testTrace;
					gi.trace( &testTrace, testFrom, vec3_origin, vec3_origin, victim->currentOrigin, ent->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
					if ( testTrace.entityNum != victim->s.number )
					{//can only damage maglocks if have a clear trace to the thing's origin
						continue;
					}
				}
				if ( totalDmg[i] > 0 )
				{//actually want to do *some* damage here
					if ( victim->s.weapon == WP_SABER && victim->client && !g_saberRealisticCombat->integer )
					{//dmg vs other saber fighters is modded by hitloc and capped
						totalDmg[i] *= damageModifier[hitLoc[i]];
						if ( hitLoc[i] == HL_NONE )
						{
							maxDmg = 33*baseDamage;
						}
						else
						{
							maxDmg = 50*hitLocHealthPercentage[hitLoc[i]]*baseDamage;//*victim->client->ps.stats[STAT_MAX_HEALTH]*2.0f;
						}
						if ( maxDmg < totalDmg[i] )
						{
							totalDmg[i] = maxDmg;
						}
						//dFlags |= DAMAGE_NO_HIT_LOC;
					}
					//clamp the dmg
					if ( victim->s.weapon != WP_SABER )
					{//clamp the dmg between 25 and maxhealth
						/*
						if ( totalDmg[i] > victim->max_health )
						{
							totalDmg[i] = victim->max_health;
						}
						else */if ( totalDmg[i] < 25 )
						{
							totalDmg[i] = 25;
						}
						if ( totalDmg[i] > 100 )//+(50*g_spskill->integer) )
						{//clamp using same adjustment as in NPC_Begin
							totalDmg[i] = 100;//+(50*g_spskill->integer);
						}
					}
					else
					{//clamp the dmg between 5 and 100
						if ( !victim->s.number && totalDmg[i] > 50 )
						{//never do more than half full health damage to player
							//prevents one-hit kills
							totalDmg[i] = 50;
						}
						else if ( totalDmg[i] > 100 )
						{
							totalDmg[i] = 100;
						}
						else
						{
							if ( totalDmg[i] < 5 )
							{
								totalDmg[i] = 5;
							}
						}
					}

					if ( totalDmg[i] > 0 )
					{
						didDamage = qtrue;
						if( victim->client )
						{
							if ( victim->client->ps.pm_time > 0 && victim->client->ps.pm_flags & PMF_TIME_KNOCKBACK && victim->client->ps.velocity[2] > 0 )
							{//already being knocked around
								dFlags |= DAMAGE_NO_KNOCKBACK;
							}
							if ( g_dismemberment->integer >= 11381138 || g_saberRealisticCombat->integer )
							{
								dFlags |= DAMAGE_DISMEMBER;
								if ( hitDismember[i] )
								{
									victim->client->dismembered = qfalse;
								}
							}
							else if ( hitDismember[i] )
							{
								dFlags |= DAMAGE_DISMEMBER;
							}
							if ( baseDamage <= 1.0f )
							{//very mild damage
								if ( victim->s.number == 0 || victim->client->ps.weapon == WP_SABER || victim->client->NPC_class == CLASS_GALAKMECH )
								{//if it's the player or a saber-user, don't kill them with this blow
									dFlags |= DAMAGE_NO_KILL;
								}
							}
						}
						else
						{
							if ( victim->takedamage )
							{//some other breakable thing
								//create a flash here
								g_saberFlashTime = level.time-50;
								VectorCopy( dmgSpot[i], g_saberFlashPos );
							}
						}
						//victim->hitLoc = hitLoc[i];

						dFlags |= DAMAGE_NO_KNOCKBACK;//okay, let's try no knockback whatsoever...
						dFlags &= ~DAMAGE_DEATH_KNOCKBACK;
						if ( g_saberRealisticCombat->integer )
						{
							dFlags |= DAMAGE_NO_KNOCKBACK;
							dFlags &= ~DAMAGE_DEATH_KNOCKBACK;
							dFlags &= ~DAMAGE_NO_KILL;
						}
						if ( ent->client && !ent->s.number )
						{
							switch( hitLoc[i] )
							{
							case HL_FOOT_RT:
							case HL_FOOT_LT:
							case HL_LEG_RT:
							case HL_LEG_LT:
								ent->client->sess.missionStats.legAttacksCnt++;
								break;
							case HL_WAIST:
							case HL_BACK_RT:
							case HL_BACK_LT:
							case HL_BACK:
							case HL_CHEST_RT:
							case HL_CHEST_LT:
							case HL_CHEST:
								ent->client->sess.missionStats.torsoAttacksCnt++;
								break;
							case HL_ARM_RT:
							case HL_ARM_LT:
							case HL_HAND_RT:
							case HL_HAND_LT:
								ent->client->sess.missionStats.armAttacksCnt++;
								break;
							default:
								ent->client->sess.missionStats.otherAttacksCnt++;
								break;
							}
						}
						G_Damage( victim, ent, ent, dmgDir[i], dmgSpot[i], ceil(totalDmg[i]), dFlags, MOD_SABER, hitDismemberLoc[i] );
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							gi.Printf( S_COLOR_RED"damage: %4.2f, hitLoc %d\n", totalDmg[i], hitLoc[i] );
						}
#endif
						//do the effect
						//G_PlayEffect( G_EffectIndex( "blood_sparks" ), dmgSpot[i], dmgDir[i] );
						if ( ent->s.number == 0 )
						{
							AddSoundEvent( victim->owner, dmgSpot[i], 256, AEL_DISCOVERED );
							AddSightEvent( victim->owner, dmgSpot[i], 512, AEL_DISCOVERED, 50 );
						}
						if ( ent->client )
						{
							if ( ent->enemy && ent->enemy == victim )
							{//just so Jedi knows that he hit his enemy
								ent->client->ps.saberEventFlags |= SEF_HITENEMY;
							}
							else
							{
								ent->client->ps.saberEventFlags |= SEF_HITOBJECT;
							}
						}
					}
				}
			}
		}
	}
	return didDamage;
}

void WP_SaberDamageAdd( float trDmg, int trVictimEntityNum, vec3_t trDmgDir, vec3_t trDmgSpot, float dmg, float fraction, int trHitLoc, qboolean trDismember, int trDismemberLoc )
{
	int curVictim = 0;
	int i;

	if ( trVictimEntityNum < 0 || trVictimEntityNum >= ENTITYNUM_WORLD )
	{
		return;
	}
	if ( trDmg * dmg < 10.0f )
	{//too piddly an amount of damage to really count?
		//FIXME: but already did the effect, didn't we... sigh...
		//return;
	}
	if ( trDmg )
	{//did some damage to something
		for ( i = 0; i < numVictims; i++ )
		{
			if ( victimEntityNum[i] == trVictimEntityNum )
			{//already hit this guy before
				curVictim = i;
				break;
			}
		}
		if ( i == numVictims )
		{//haven't hit his guy before
			if ( numVictims + 1 >= MAX_SABER_VICTIMS )
			{//can't add another victim at this time
				return;
			}
			//add a new victim to the list
			curVictim = numVictims;
			victimEntityNum[numVictims++] = trVictimEntityNum;
		}

		float addDmg = trDmg*dmg;
		if ( trHitLoc!=HL_NONE && (hitLoc[curVictim]==HL_NONE||hitLocHealthPercentage[trHitLoc]>hitLocHealthPercentage[hitLoc[curVictim]]) )
		{//this hitLoc is more critical than the previous one this frame
			hitLoc[curVictim] = trHitLoc;
		}

		totalDmg[curVictim] += addDmg;
		if ( !VectorLengthSquared( dmgDir[curVictim] ) )
		{
			VectorCopy( trDmgDir, dmgDir[curVictim] );
		}
		if ( !VectorLengthSquared( dmgSpot[curVictim] ) )
		{
			VectorCopy( trDmgSpot, dmgSpot[curVictim] );
		}

		// Make sure we keep track of the fraction.  Why?
		// Well, if the saber hits something that stops it, the damage isn't done past that point.
		dmgFraction[curVictim] = fraction;
		if ( (trDismemberLoc != HL_NONE && hitDismemberLoc[curVictim] == HL_NONE)
			|| (!hitDismember[curVictim] && trDismember) )
		{//either this is the first dismember loc we got or we got a loc before, but it wasn't a dismember loc, so take the new one
			hitDismemberLoc[curVictim] = trDismemberLoc;
		}
		if ( trDismember )
		{//we scored a dismemberment hit...
			hitDismember[curVictim] = trDismember;
		}
	}
}

/*
WP_SabersIntersect

Breaks the two saber paths into 2 tris each and tests each tri for the first saber path against each of the other saber path's tris

FIXME: subdivide the arc into a consistant increment
FIXME: test the intersection to see if the sabers really did intersect (weren't going in the same direction and/or passed through same point at different times)?
*/
extern qboolean tri_tri_intersect(vec3_t V0,vec3_t V1,vec3_t V2,vec3_t U0,vec3_t U1,vec3_t U2);
qboolean WP_SabersIntersect( gentity_t *ent1, gentity_t *ent2, qboolean checkDir )
{
	vec3_t	saberBase1, saberTip1, saberBaseNext1, saberTipNext1;
	vec3_t	saberBase2, saberTip2, saberBaseNext2, saberTipNext2;
	vec3_t	dir;

	/*
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_GREEN"Doing precise saber intersection check\n" );
	}
#endif
	*/

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.saberLength <= 0 || ent2->client->ps.saberLength <= 0 )
	{
		return qfalse;
	}

	//if ( ent1->client->ps.saberInFlight )
	{
		VectorCopy( ent1->client->renderInfo.muzzlePointOld, saberBase1 );
		VectorCopy( ent1->client->renderInfo.muzzlePoint, saberBaseNext1 );

		VectorSubtract( ent1->client->renderInfo.muzzlePoint, ent1->client->renderInfo.muzzlePointOld, dir );
		VectorNormalize( dir );
		VectorMA( saberBaseNext1, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext1 );

		VectorMA( saberBase1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDirOld, saberTip1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDir, saberTipNext1 );

		VectorSubtract( saberTipNext1, saberTip1, dir );
		VectorNormalize( dir );
		VectorMA( saberTipNext1, SABER_EXTRAPOLATE_DIST, dir, saberTipNext1 );
	}
	/*
	else
	{
		VectorCopy( ent1->client->renderInfo.muzzlePoint, saberBase1 );
		VectorCopy( ent1->client->renderInfo.muzzlePointNext, saberBaseNext1 );
		VectorMA( saberBase1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDir, saberTip1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDirNext, saberTipNext1 );
	}
	*/

	//if ( ent2->client->ps.saberInFlight )
	{
		VectorCopy( ent2->client->renderInfo.muzzlePointOld, saberBase2 );
		VectorCopy( ent2->client->renderInfo.muzzlePoint, saberBaseNext2 );

		VectorSubtract( ent2->client->renderInfo.muzzlePoint, ent2->client->renderInfo.muzzlePointOld, dir );
		VectorNormalize( dir );
		VectorMA( saberBaseNext2, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext2 );

		VectorMA( saberBase2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDirOld, saberTip2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDir, saberTipNext2 );

		VectorSubtract( saberTipNext2, saberTip2, dir );
		VectorNormalize( dir );
		VectorMA( saberTipNext2, SABER_EXTRAPOLATE_DIST, dir, saberTipNext2 );
	}
	/*
	else
	{
		VectorCopy( ent2->client->renderInfo.muzzlePoint, saberBase2 );
		VectorCopy( ent2->client->renderInfo.muzzlePointNext, saberBaseNext2 );
		VectorMA( saberBase2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDir, saberTip2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDirNext, saberTipNext2 );
	}
	*/
	if ( checkDir )
	{//check the direction of the two swings to make sure the sabers are swinging towards each other
		vec3_t saberDir1, saberDir2;

		VectorSubtract( saberTipNext1, saberTip1, saberDir1 );
		VectorSubtract( saberTipNext2, saberTip2, saberDir2 );
		VectorNormalize( saberDir1 );
		VectorNormalize( saberDir2 );
		if ( DotProduct( saberDir1, saberDir2 ) > 0.6f )
		{//sabers moving in same dir, probably didn't actually hit
			return qfalse;
		}
		//now check orientation of sabers, make sure they're not parallel or close to it
		float dot = DotProduct( ent1->client->renderInfo.muzzleDir, ent2->client->renderInfo.muzzleDir );
		if ( dot > 0.9f || dot < -0.9f )
		{//too parallel to really block effectively?
			return qfalse;
		}
	}

	if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberBaseNext2 ) )
	{
		return qtrue;
	}
	if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberTipNext2 ) )
	{
		return qtrue;
	}
	if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberBaseNext2 ) )
	{
		return qtrue;
	}
	if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberTipNext2 ) )
	{
		return qtrue;
	}
	return qfalse;
}

float WP_SabersDistance( gentity_t *ent1, gentity_t *ent2 )
{
	vec3_t	saberBaseNext1, saberTipNext1, saberPoint1;
	vec3_t	saberBaseNext2, saberTipNext2, saberPoint2;

	/*
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_GREEN"Doing precise saber intersection check\n" );
	}
#endif
	*/

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.saberLength <= 0 || ent2->client->ps.saberLength <= 0 )
	{
		return qfalse;
	}

	//if ( ent1->client->ps.saberInFlight )
	{
		VectorCopy( ent1->client->renderInfo.muzzlePoint, saberBaseNext1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDir, saberTipNext1 );
	}
	/*
	else
	{
		VectorCopy( ent1->client->renderInfo.muzzlePointNext, saberBaseNext1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDirNext, saberTipNext1 );
	}
	*/

	//if ( ent2->client->ps.saberInFlight )
	{
		VectorCopy( ent2->client->renderInfo.muzzlePoint, saberBaseNext2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDir, saberTipNext2 );
	}
	/*
	else
	{
		VectorCopy( ent2->client->renderInfo.muzzlePointNext, saberBaseNext2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDirNext, saberTipNext2 );
	}
	*/

	float sabersDist = ShortestLineSegBewteen2LineSegs( saberBaseNext1, saberTipNext1, saberBaseNext2, saberTipNext2, saberPoint1, saberPoint2 );

	//okay, this is a super hack, but makes saber collisions look better from the player point of view
	/*
	if ( sabersDist < 16.0f )
	{
		vec3_t	saberDistDir, saberMidPoint, camLookDir;

		VectorSubtract( saberPoint2, saberPoint1, saberDistDir );
		VectorMA( saberPoint1, 0.5f, saberDistDir, saberMidPoint );
		VectorSubtract( saberMidPoint, cg.refdef.vieworg, camLookDir );
		VectorNormalize( saberDistDir );
		VectorNormalize( camLookDir );
		float dot = fabs(DotProduct( camLookDir, saberDistDir ));
		sabersDist -= 8.0f*dot;
		if ( sabersDist < 0.0f )
		{
			sabersDist = 0.0f;
		}
	}
	*/

#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 2 )
	{
		G_DebugLine( saberPoint1, saberPoint2, FRAMETIME, 0x00ffffff, qtrue );
	}
#endif
	return sabersDist;
}

qboolean WP_SabersIntersection( gentity_t *ent1, gentity_t *ent2, vec3_t intersect )
{
	vec3_t	saberBaseNext1, saberTipNext1, saberPoint1;
	vec3_t	saberBaseNext2, saberTipNext2, saberPoint2;

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.saberLength <= 0 || ent2->client->ps.saberLength <= 0 )
	{
		return qfalse;
	}

	VectorCopy( ent1->client->renderInfo.muzzlePoint, saberBaseNext1 );
	VectorMA( saberBaseNext1, ent1->client->ps.saberLength, ent1->client->renderInfo.muzzleDir, saberTipNext1 );

	VectorCopy( ent2->client->renderInfo.muzzlePoint, saberBaseNext2 );
	VectorMA( saberBaseNext2, ent2->client->ps.saberLength, ent2->client->renderInfo.muzzleDir, saberTipNext2 );

	ShortestLineSegBewteen2LineSegs( saberBaseNext1, saberTipNext1, saberBaseNext2, saberTipNext2, saberPoint1, saberPoint2 );
	VectorAdd( saberPoint1, saberPoint2, intersect );
	VectorScale( intersect, 0.5, intersect );
	return qtrue;
}

char *hit_blood_sparks = "blood_sparks";
char *hit_sparks = "saber_cut";

extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
qboolean WP_SaberDamageEffects( trace_t *tr, const vec3_t start, float length, float dmg, vec3_t dmgDir, vec3_t bladeDir, int enemyTeam )
{

	int			hitEntNum[MAX_G2_COLLISIONS] = {ENTITYNUM_NONE};
	float		hitEntDmgAdd[MAX_G2_COLLISIONS] = {0};
	float		hitEntDmgSub[MAX_G2_COLLISIONS] = {0};
	vec3_t		hitEntPoint[MAX_G2_COLLISIONS];
	vec3_t		hitEntDir[MAX_G2_COLLISIONS];
	float		hitEntStartFrac[MAX_G2_COLLISIONS] = {0};
	int			trHitLoc = HL_NONE;
	int			trDismemberLoc = HL_NONE;
	qboolean	trDismember = qfalse;
	int			i,z;
	int			numHitEnts = 0;
	float		distFromStart,doDmg;
	char		*hitEffect;
	gentity_t	*hitEnt;

	for (z=0; z < MAX_G2_COLLISIONS; z++)
	{
		if ( tr->G2CollisionMap[z].mEntityNum == -1 )
		{//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
			continue;//break;//
		}

		CCollisionRecord &coll	= tr->G2CollisionMap[z];
		//distFromStart			= Distance( start, coll.mCollisionPosition );
		distFromStart			= coll.mDistance;

		/*
		//FIXME: (distFromStart/length) is not guaranteed to be from 0 to 1... *sigh*...
		if ( length && saberHitFraction < 1.0f && (distFromStart/length) < 1.0f && (distFromStart/length) > saberHitFraction )
		{//a saber was hit before this point, don't count it
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer )
			{
				gi.Printf( S_COLOR_MAGENTA"rejecting G2 collision- %4.2f farther than saberHitFraction %4.2f\n", (distFromStart/length), saberHitFraction  );
			}
#endif
			continue;
		}
		*/

		for ( i = 0; i < numHitEnts; i++ )
		{
			if ( hitEntNum[i] != ENTITYNUM_NONE )
			{//we hit this ent before
				//we'll want to add this dist
				hitEntDmgAdd[i] = distFromStart;
				break;
			}
		}
		if ( i == numHitEnts )
		{//first time we hit this ent
			if ( numHitEnts == MAX_G2_COLLISIONS )
			{//hit too many damn ents!
				continue;
			}
			hitEntNum[numHitEnts] = coll.mEntityNum;
			if ( !coll.mFlags )
			{//hmm, we came out first, so we must have started inside
				//we'll want to subtract this dist
				hitEntDmgAdd[numHitEnts] = distFromStart;
			}
			else
			{//we're entering the model
				//we'll want to subtract this dist
				hitEntDmgSub[numHitEnts] = distFromStart;
			}
			//keep track of how far in the damage was done
			hitEntStartFrac[numHitEnts] = hitEntDmgSub[numHitEnts]/length;
			//remember the entrance point
			VectorCopy( coll.mCollisionPosition, hitEntPoint[numHitEnts] );
			//remember the entrance dir
			VectorCopy( coll.mCollisionNormal, hitEntDir[numHitEnts] );
			VectorNormalize( hitEntDir[numHitEnts] );

			//do the effect

			//FIXME: check material rather than team?
			hitEnt = &g_entities[hitEntNum[numHitEnts]];
			hitEffect = hit_blood_sparks;
			if ( hitEnt != NULL )
			{
				if ( hitEnt->client )
				{
					class_t npc_class = hitEnt->client->NPC_class;
					if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE || npc_class == CLASS_REMOTE ||
					     npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					     npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
					     npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
					{
						hitEffect = hit_sparks;
					}
				}
				else
				{
					hitEffect = hit_sparks;
				}
			}

			//FIXME: play less if damage is less?
			G_PlayEffect( hitEffect, coll.mCollisionPosition, coll.mCollisionNormal );

			//Get the hit location based on surface name
			if ( (hitLoc[hitEntNum[numHitEnts]] == HL_NONE && trHitLoc == HL_NONE)
				|| (hitDismemberLoc[hitEntNum[numHitEnts]] == HL_NONE && trDismemberLoc == HL_NONE)
				|| (!hitDismember[hitEntNum[numHitEnts]] && !trDismember) )
			{//no hit loc set for this ent this damage cycle yet
				//FIXME: find closest impact surf *first* (per ent), then call G_GetHitLocFromSurfName?
				trDismember = G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex ), &trHitLoc, coll.mCollisionPosition, dmgDir, bladeDir, MOD_SABER );
				trDismemberLoc = trHitLoc;
			}
			numHitEnts++;
		}
	}

	//now go through all the ents we hit and do the damage
	for ( i = 0; i < numHitEnts; i++ )
	{
		doDmg = dmg;
		if ( hitEntNum[i] != ENTITYNUM_NONE )
		{
			if ( doDmg < 10 )
			{//base damage is less than 10
				if ( hitEntNum[i] != 0 )
				{//not the player
					hitEnt = &g_entities[hitEntNum[i]];
					if ( !hitEnt->client || (hitEnt->client->ps.weapon!=WP_SABER&&hitEnt->client->NPC_class!=CLASS_GALAKMECH&&hitEnt->client->playerTeam==enemyTeam) )
					{//did *not* hit a jedi and did *not* hit the player
						//make sure the base damage is high against non-jedi, feels better
						doDmg = 10;
					}
				}
			}
			if ( !hitEntDmgAdd[i] && !hitEntDmgSub[i] )
			{//spent entire time in model
				//NOTE: will we even get a collision then?
				doDmg *= length;
			}
			else if ( hitEntDmgAdd[i] && hitEntDmgSub[i] )
			{//we did enter and exit
				doDmg *= hitEntDmgAdd[i] - hitEntDmgSub[i];
			}
			else if ( !hitEntDmgAdd[i] )
			{//we didn't exit, just entered
				doDmg *= length - hitEntDmgSub[i];
			}
			else if ( !hitEntDmgSub[i] )
			{//we didn't enter, only exited
				doDmg *= hitEntDmgAdd[i];
			}
			if ( doDmg > 0 )
			{
				WP_SaberDamageAdd( 1.0, hitEntNum[i], hitEntDir[i], hitEntPoint[i], ceil(doDmg), hitEntStartFrac[i], trHitLoc, trDismember, trDismemberLoc );
			}
		}
	}
	return (numHitEnts>0);
}

void WP_SaberKnockaway( gentity_t *attacker, trace_t *tr )
{
	WP_SaberDrop( attacker, &g_entities[attacker->client->ps.saberEntityNum] );
	G_Sound( &g_entities[attacker->client->ps.saberEntityNum], G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
	G_PlayEffect( "saber_block", tr->endpos );
	saberHitFraction = tr->fraction;
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_MAGENTA"WP_SaberKnockaway: saberHitFraction %4.2f\n", saberHitFraction );
	}
#endif
	VectorCopy( tr->endpos, saberHitLocation );
	saberHitEntity = tr->entityNum;
	g_saberFlashTime = level.time-50;
	VectorCopy( saberHitLocation, g_saberFlashPos );

	//FIXME: make hitEnt play an attack anim or some other special anim when this happens
	//gentity_t *hitEnt = &g_entities[tr->entityNum];
	//NPC_SetAnim( hitEnt, SETANIM_BOTH, BOTH_KNOCKSABER, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
}

#define SABER_COLLISION_DIST 6//was 2//was 4//was 8//was 16
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
qboolean WP_SaberDamageForTrace( int ignore, vec3_t start, vec3_t end, float dmg,
								vec3_t bladeDir, qboolean noGhoul, int attackStrength,
								qboolean extrapolate = qtrue )
{
	trace_t		tr;
	vec3_t		dir;
	int			mask = (MASK_SHOT|CONTENTS_LIGHTSABER);
	gentity_t	*attacker = &g_entities[ignore];

	vec3_t		end2;
	VectorCopy( end, end2 );
	if ( extrapolate )
	{
		//NOTE: since we can no longer use the predicted point, extrapolate the trace some.
		//		this may allow saber hits that aren't actually hits, but it doesn't look too bad
		vec3_t diff;
		VectorSubtract( end, start, diff );
		VectorNormalize( diff );
		VectorMA( end2, SABER_EXTRAPOLATE_DIST, diff, end2 );
	}

	if ( !noGhoul )
	{
		if ( !attacker->s.number || (attacker->client&&(attacker->client->playerTeam==TEAM_PLAYER||attacker->client->NPC_class==CLASS_SHADOWTROOPER||attacker->client->NPC_class==CLASS_TAVION||attacker->client->NPC_class==CLASS_DESANN) ) )//&&attackStrength==FORCE_LEVEL_3)
		{//player,. player allies, shadowtroopers, tavion and desann use larger traces
			vec3_t	traceMins = {-2,-2,-2}, traceMaxs = {2,2,2};
			gi.trace( &tr, start, traceMins, traceMaxs, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
		/*
		else if ( !attacker->s.number )
		{
			vec3_t	traceMins = {-1,-1,-1}, traceMaxs = {1,1,1};
			gi.trace( &tr, start, traceMins, traceMaxs, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
		*/
		else
		{//reborn use smaller traces
			gi.trace( &tr, start, NULL, NULL, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
	}
	else
	{
		gi.trace( &tr, start, NULL, NULL, end2, ignore, mask, G2_NOCOLLIDE, 10 );
	}


#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 1 )
	{
		if ( attacker != NULL && attacker->client != NULL )
		{
			G_DebugLine(start, end2, FRAMETIME, WPDEBUG_SaberColor( attacker->client->ps.saberColor ), qtrue);
		}
	}
#endif

	if ( tr.entityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}

	if ( tr.entityNum == ENTITYNUM_WORLD )
	{
		return qtrue;
	}

	if ( &g_entities[tr.entityNum] )
	{
		gentity_t *hitEnt = &g_entities[tr.entityNum];
		gentity_t *owner = g_entities[tr.entityNum].owner;
		if ( hitEnt->contents & CONTENTS_LIGHTSABER )
		{
			if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
			{//thrown saber hit something
				 if ( owner
					 && owner->s.number
					 && owner->client
					 && owner->NPC
					 && owner->health > 0
					 && ( owner->client->NPC_class == CLASS_TAVION
							/*|| (owner->client->NPC_class == CLASS_SHADOWTROOPER && !Q_irand( 0, g_spskill->integer*3 ))
							|| (Q_irand( -5, owner->NPC->rank ) > RANK_CIVILIAN && !Q_irand( 0, g_spskill->integer*3 ))*/ )
					 )
				 {//Tavion can toss a blocked thrown saber aside
					WP_SaberKnockaway( attacker, &tr );
					Jedi_PlayDeflectSound( owner );
					return qfalse;
				 }
			}
			//FIXME: take target FP_SABER_DEFENSE and attacker FP_SABER_OFFENSE into account here somehow?
			qboolean sabersIntersect = WP_SabersIntersect( attacker, owner, qfalse );//qtrue );
			float sabersDist;
			if ( attacker && attacker->client && attacker->client->ps.saberInFlight
				&& owner && owner->s.number == 0 && (g_saberAutoBlocking->integer||attacker->client->ps.saberBlockingTime>level.time) )//NPC flying saber hit player's saber bounding box
			{//players have g_saberAutoBlocking, do the more generous check against flying sabers
				//FIXME: instead of hitting the player's saber bounding box
				//and picking an anim afterwards, have him use AI similar
				//to the AI the jedi use for picking a saber melee block...?
				sabersDist = 0;
			}
			else
			{//sabers must actually collide with the attacking saber
				sabersDist = WP_SabersDistance( attacker, owner );
				if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
				{
					sabersDist /= 2.0f;
					if ( sabersDist <= 16.0f )
					{
						sabersIntersect = qtrue;
					}
				}
#ifndef FINAL_BUILD
				if ( d_saberCombat->integer > 1 )
				{
					gi.Printf( "sabersDist: %4.2f\n", sabersDist );
				}
#endif//FINAL_BUILD
			}
			if ( sabersCrossed == -1 || sabersCrossed > sabersDist )
			{
				sabersCrossed = sabersDist;
			}
			float	collisionDist;
			if ( g_saberRealisticCombat->integer )
			{
				collisionDist = SABER_COLLISION_DIST;
			}
			else
			{
				collisionDist = SABER_COLLISION_DIST+6+g_spskill->integer*4;
			}
			if ( owner && owner->client && (attacker != NULL)
				&& (sabersDist > collisionDist )//|| !InFront( attacker->currentOrigin, owner->currentOrigin, owner->client->ps.viewangles, 0.35f ))
				&& !sabersIntersect )//was qtrue, but missed too much?
			{//swing came from behind and/or was not stopped by a lightsaber
				//re-try the trace without checking for lightsabers
				gi.trace ( &tr, start, NULL, NULL, end2, ignore, mask&~CONTENTS_LIGHTSABER, G2_NOCOLLIDE, 10 );
				if ( tr.entityNum == ENTITYNUM_WORLD )
				{
 					return qtrue;
				}
				if ( tr.entityNum == ENTITYNUM_NONE || &g_entities[tr.entityNum] == NULL )
				{//didn't hit the owner
					/*
					if ( attacker
						&& attacker->client
						&& (PM_SaberInAttack( attacker->client->ps.saberMove ) || PM_SaberInStart( attacker->client->ps.saberMove ))
						&& DistanceSquared( tr.endpos, owner->currentOrigin ) < 10000 )
					{
						if ( owner->NPC
							&& !owner->client->ps.saberInFlight
							&& owner->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
							&& !Jedi_SaberBusy( owner ) )
						{//owner parried, just make sure they're saber is in the right spot - only does this if they're not already doing something with saber
							if ( g_spskill->integer && (g_spskill->integer > 1 || Q_irand( 0, 1 )))
							{//if on easy, they don't cheat like this, if on medium, they cheat 50% of the time, if on hard, they always cheat
								//FIXME: also take into account the owner's FP_DEFENSE?
								if ( Q_irand( 0, owner->NPC->rank ) >= RANK_CIVILIAN )
								{//lower-rank Jedi aren't as good blockers
									vec3_t attDir;
									VectorSubtract( end2, start, attDir );
									VectorNormalize( attDir );
									Jedi_SaberBlockGo( owner, owner->NPC->last_ucmd, start, attDir, NULL );
								}
							}
						}
					}
					*/
  					return qfalse;	// Exit, but we didn't hit the wall.
				}
#ifndef FINAL_BUILD
				if ( d_saberCombat->integer > 1 )
				{
					if ( !attacker->s.number )
					{
						gi.Printf( S_COLOR_MAGENTA"%d saber hit owner through saber %4.2f, dist = %4.2f\n", level.time, saberHitFraction, sabersDist );
					}
				}
#endif//FINAL_BUILD
				hitEnt = &g_entities[tr.entityNum];
				owner = g_entities[tr.entityNum].owner;
			}
			else
			{//hit a lightsaber
				if ( (tr.fraction < saberHitFraction || tr.startsolid)
					&& sabersDist < (8.0f+g_spskill->value)*4.0f// 50.0f//16.0f
					&& (sabersIntersect || sabersDist < (4.0f+g_spskill->value)*2.0f) )//32.0f) )
				{	// This saber hit closer than the last one.
					if ( (tr.allsolid || tr.startsolid) && owner && owner->client )
					{//tr.fraction will be 0, unreliable... so calculate actual
						float dist = Distance( start, end2 );
						if ( dist )
						{
							float hitFrac = WP_SabersDistance( attacker, owner )/dist;
							if ( hitFrac > 1.0f )
							{//umm... minimum distance between sabers was longer than trace...?
								hitFrac = 1.0f;
							}
							if ( hitFrac < saberHitFraction )
							{
								saberHitFraction = hitFrac;
							}
						}
						else
						{
							saberHitFraction = 0.0f;
						}
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer > 1 )
						{
							if ( !attacker->s.number )
							{
								gi.Printf( S_COLOR_GREEN"%d saber hit saber dist %4.2f allsolid %4.2f\n", level.time, sabersDist, saberHitFraction );
							}
						}
#endif//FINAL_BUILD
					}
					else
					{
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer > 1 )
						{
							if ( !attacker->s.number )
							{
								gi.Printf( S_COLOR_BLUE"%d saber hit saber dist %4.2f, frac %4.2f\n", level.time, sabersDist, saberHitFraction );
							}
							saberHitFraction = tr.fraction;
						}
#endif//FINAL_BUILD
					}
#ifndef FINAL_BUILD
					if ( d_saberCombat->integer )
					{
						gi.Printf( S_COLOR_MAGENTA"hit saber: saberHitFraction %4.2f, allsolid %d, startsolid %d\n", saberHitFraction, tr.allsolid, tr.startsolid );
					}
#endif//FINAL_BUILD
					VectorCopy(tr.endpos, saberHitLocation);
					saberHitEntity = tr.entityNum;
				}
				/*
				if ( owner
					&& owner->client
					&& attacker
					&& attacker->client
					&& (PM_SaberInAttack( attacker->client->ps.saberMove ) || PM_SaberInStart( attacker->client->ps.saberMove ))
					&& DistanceSquared( tr.endpos, owner->currentOrigin ) < 10000 )
				{
					if ( owner->NPC
						&& !owner->client->ps.saberInFlight
						&& owner->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
						&& !Jedi_SaberBusy( owner ) )
					{//owner parried, just make sure they're saber is in the right spot - only does this if they're not already doing something with saber
						if ( g_spskill->integer && (g_spskill->integer > 1 || Q_irand( 0, 1 )))
						{//if on easy, they don't cheat like this, if on medium, they cheat 50% of the time, if on hard, they always cheat
							//FIXME: also take into account the owner's FP_DEFENSE?
							if ( Q_irand( 0, owner->NPC->rank ) >= RANK_CIVILIAN )
							{//lower-rank Jedi aren't as good blockers
								vec3_t attDir;
								VectorSubtract( end2, start, attDir );
								VectorNormalize( attDir );
								Jedi_SaberBlockGo( owner, owner->NPC->last_ucmd, start, attDir, NULL );
							}
						}
					}
				}
				*/
  				return qfalse;	// Exit, but we didn't hit the wall.
			}
		}
		else
		{
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				if ( !attacker->s.number )
				{
					gi.Printf( S_COLOR_RED"%d saber hit owner directly %4.2f\n", level.time, saberHitFraction );
				}
			}
#endif//FINAL_BUILD
		}

		if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
		{//thrown saber hit something
			if ( ( hitEnt && hitEnt->client && hitEnt->health > 0 && ( hitEnt->client->NPC_class == CLASS_DESANN || hitEnt->client->NPC_class == CLASS_LUKE || (hitEnt->client->NPC_class == CLASS_GALAKMECH&&hitEnt->client->ps.powerups[PW_GALAK_SHIELD] > 0) ) ) ||
				 ( owner && owner->client && owner->health > 0 && ( owner->client->NPC_class == CLASS_DESANN || owner->client->NPC_class == CLASS_LUKE || (owner->client->NPC_class==CLASS_GALAKMECH&&owner->client->ps.powerups[PW_GALAK_SHIELD] > 0) ) ) )
			{//Luke and Desann slap thrown sabers aside
				//FIXME: control the direction of the thrown saber... if hit Galak's shield, bounce directly away from his origin?
				WP_SaberKnockaway( attacker, &tr );
				if ( hitEnt->client )
				{
					Jedi_PlayDeflectSound( hitEnt );
				}
				else
				{
					Jedi_PlayDeflectSound( owner );
				}
  				return qfalse;	// Exit, but we didn't hit the wall.
			}
		}

		if ( hitEnt->takedamage )
		{
			//no team damage: if ( !hitEnt->client || attacker == NULL || !attacker->client || (hitEnt->client->playerTeam != attacker->client->playerTeam) )
			{
				//multiply the damage by the total distance of the swipe
				VectorSubtract( end2, start, dir );
				float len = VectorNormalize( dir );//VectorLength( dir );
				if ( noGhoul || !hitEnt->ghoul2.size() )
				{//we weren't doing a ghoul trace
					char	*hitEffect;
					if ( dmg >= 1.0 && hitEnt->bmodel )
					{
						dmg = 1.0;
					}
					if ( len > 1 )
					{
						dmg *= len;
					}
#ifndef FINAL_BUILD
					if ( d_saberCombat->integer > 1 )
					{
						if ( !(hitEnt->contents & CONTENTS_LIGHTSABER) )
						{
							gi.Printf( S_COLOR_GREEN"Hit ent, but no ghoul collisions\n" );
						}
					}
#endif
					float	trFrac, dmgFrac;
					if ( tr.allsolid )
					{//totally inside them
						trFrac = 1.0;
						dmgFrac = 0.0;
					}
					else if ( tr.startsolid )
					{//started inside them
						//we don't know how much was inside, we know it's less than all, so use half?
						trFrac = 0.5;
						dmgFrac = 0.0;
					}
					else
					{//started outside them and hit them
						//yeah. this doesn't account for coming out the other wide, but we can worry about that later (use ghoul2)
						trFrac = (1.0f - tr.fraction);
						dmgFrac = tr.fraction;
					}
					WP_SaberDamageAdd( trFrac, tr.entityNum, dir, tr.endpos, dmg, dmgFrac, HL_NONE, qfalse, HL_NONE );
					if ( !tr.allsolid && !tr.startsolid )
					{
						VectorScale( dir, -1, dir );
					}
					//FIXME: don't do blood sparks on non-living things
					hitEffect = hit_blood_sparks;
					if ( hitEnt != NULL )
					{
						if ( hitEnt->client )
						{
							class_t npc_class = hitEnt->client->NPC_class;
							if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
								 npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
								 npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
							     npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
							{
								hitEffect = hit_sparks;
							}
						}
						else
						{
							hitEffect = hit_sparks;
						}
					}

					G_PlayEffect( hitEffect, tr.endpos, dir );//"saber_cut"
				}
				else
				{//we were doing a ghoul trace
					if ( !WP_SaberDamageEffects( &tr, start, len, dmg, dir, bladeDir, attacker->client->enemyTeam ) )
					{//didn't hit a ghoul ent
						/*
						if ( && hitEnt->ghoul2.size() )
						{//it was a ghoul2 model so we should have hit it
							return qfalse;
						}
						*/
					}
				}
			}
		}
	}

	return qfalse;
}

typedef enum
{
	LOCK_FIRST = 0,
	LOCK_TOP = LOCK_FIRST,
	LOCK_DIAG_TR,
	LOCK_DIAG_TL,
	LOCK_DIAG_BR,
	LOCK_DIAG_BL,
	LOCK_R,
	LOCK_L,
	LOCK_RANDOM
} sabersLockMode_t;

#define LOCK_IDEAL_DIST_TOP 32.0f
#define LOCK_IDEAL_DIST_CIRCLE 48.0f
extern void PM_SetAnimFrame( gentity_t *gent, int frame, qboolean torso, qboolean legs );
extern qboolean ValidAnimFileIndex ( int index );
qboolean WP_SabersCheckLock2( gentity_t *attacker, gentity_t *defender, sabersLockMode_t lockMode )
{
	animation_t *anim;
	int		attAnim, defAnim, advance = 0;
	float	attStart = 0.5f;
	float	idealDist = 48.0f;
	//MATCH ANIMS
	if ( lockMode == LOCK_RANDOM )
	{
		lockMode = (sabersLockMode_t)Q_irand( (int)LOCK_FIRST, (int)(LOCK_RANDOM)-1 );
	}
	switch ( lockMode )
	{
	case LOCK_TOP:
		attAnim = BOTH_BF2LOCK;
		defAnim = BOTH_BF1LOCK;
		attStart = 0.5f;
		idealDist = LOCK_IDEAL_DIST_TOP;
		break;
	case LOCK_DIAG_TR:
		attAnim = BOTH_CCWCIRCLELOCK;
		defAnim = BOTH_CWCIRCLELOCK;
		attStart = 0.5f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	case LOCK_DIAG_TL:
		attAnim = BOTH_CWCIRCLELOCK;
		defAnim = BOTH_CCWCIRCLELOCK;
		attStart = 0.5f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	case LOCK_DIAG_BR:
		attAnim = BOTH_CWCIRCLELOCK;
		defAnim = BOTH_CCWCIRCLELOCK;
		attStart = 0.85f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	case LOCK_DIAG_BL:
		attAnim = BOTH_CCWCIRCLELOCK;
		defAnim = BOTH_CWCIRCLELOCK;
		attStart = 0.85f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	case LOCK_R:
		attAnim = BOTH_CWCIRCLELOCK;
		defAnim = BOTH_CCWCIRCLELOCK;
		attStart = 0.75f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	case LOCK_L:
		attAnim = BOTH_CCWCIRCLELOCK;
		defAnim = BOTH_CWCIRCLELOCK;
		attStart = 0.75f;
		idealDist = LOCK_IDEAL_DIST_CIRCLE;
		break;
	default:
		return qfalse;
		break;
	}
	NPC_SetAnim( attacker, SETANIM_BOTH, attAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC_SetAnim( defender, SETANIM_BOTH, defAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if( ValidAnimFileIndex( attacker->client->clientInfo.animFileIndex ) )
	{
		anim = &level.knownAnimFileSets[attacker->client->clientInfo.animFileIndex].animations[attAnim];
		advance = floor( anim->numFrames*attStart );
		PM_SetAnimFrame( attacker, anim->firstFrame + advance, qtrue, qtrue );
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			Com_Printf( "%s starting saber lock, anim = %s, %d frames to go!\n", attacker->NPC_type, animTable[attAnim].name, anim->numFrames-advance );
		}
#endif
	}
	if( ValidAnimFileIndex( defender->client->clientInfo.animFileIndex ) )
	{
		anim = &level.knownAnimFileSets[defender->client->clientInfo.animFileIndex].animations[defAnim];
		PM_SetAnimFrame( defender, anim->firstFrame + advance, qtrue, qtrue );//was anim->firstFrame + anim->numFrames - advance, but that's wrong since they are matched anims
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			Com_Printf( "%s starting saber lock, anim = %s, %d frames to go!\n", defender->NPC_type, animTable[defAnim].name, advance );
		}
#endif
	}
	VectorClear( attacker->client->ps.velocity );
	VectorClear( defender->client->ps.velocity );
	attacker->client->ps.saberLockTime = defender->client->ps.saberLockTime = level.time + SABER_LOCK_TIME;
	attacker->client->ps.legsAnimTimer = attacker->client->ps.torsoAnimTimer = defender->client->ps.legsAnimTimer = defender->client->ps.torsoAnimTimer = SABER_LOCK_TIME;
	//attacker->client->ps.weaponTime = defender->client->ps.weaponTime = SABER_LOCK_TIME;
	attacker->client->ps.saberLockEnemy = defender->s.number;
	defender->client->ps.saberLockEnemy = attacker->s.number;

	//MATCH ANGLES
	//FIXME: if zDiff in elevation, make lower look up and upper look down and move them closer?
	float defPitchAdd = 0, zDiff = ((attacker->currentOrigin[2]+attacker->client->standheight)-(defender->currentOrigin[2]+defender->client->standheight));
	if ( zDiff > 24 )
	{
		defPitchAdd = -30;
	}
	else if ( zDiff < -24 )
	{
		defPitchAdd = 30;
	}
	else
	{
		defPitchAdd = zDiff/24.0f*-30.0f;
	}
	if ( attacker->NPC && defender->NPC )
	{//if 2 NPCs, just set pitch to 0
		attacker->client->ps.viewangles[PITCH] = -defPitchAdd;
		defender->client->ps.viewangles[PITCH] = defPitchAdd;
	}
	else
	{//if a player is involved, clamp player's pitch and match NPC's to player
		if ( !attacker->s.number )
		{
			//clamp to defPitch
			if ( attacker->client->ps.viewangles[PITCH] > -defPitchAdd + 10 )
			{
				attacker->client->ps.viewangles[PITCH] = -defPitchAdd + 10;
			}
			else if ( attacker->client->ps.viewangles[PITCH] < -defPitchAdd-10 )
			{
				attacker->client->ps.viewangles[PITCH] = -defPitchAdd-10;
			}
			//clamp to sane numbers
			if ( attacker->client->ps.viewangles[PITCH] > 50 )
			{
				attacker->client->ps.viewangles[PITCH] = 50;
			}
			else if ( attacker->client->ps.viewangles[PITCH] < -50 )
			{
				attacker->client->ps.viewangles[PITCH] = -50;
			}
			defender->client->ps.viewangles[PITCH] = attacker->client->ps.viewangles[PITCH]*-1;
			defPitchAdd = defender->client->ps.viewangles[PITCH];
		}
		else if ( !defender->s.number )
		{
			//clamp to defPitch
			if ( defender->client->ps.viewangles[PITCH] > defPitchAdd + 10 )
			{
				defender->client->ps.viewangles[PITCH] = defPitchAdd + 10;
			}
			else if ( defender->client->ps.viewangles[PITCH] < defPitchAdd-10 )
			{
				defender->client->ps.viewangles[PITCH] = defPitchAdd-10;
			}
			//clamp to sane numbers
			if ( defender->client->ps.viewangles[PITCH] > 50 )
			{
				defender->client->ps.viewangles[PITCH] = 50;
			}
			else if ( defender->client->ps.viewangles[PITCH] < -50 )
			{
				defender->client->ps.viewangles[PITCH] = -50;
			}
			defPitchAdd = defender->client->ps.viewangles[PITCH];
			attacker->client->ps.viewangles[PITCH] = defender->client->ps.viewangles[PITCH]*-1;
		}
	}
	vec3_t	attAngles, defAngles, defDir;
	VectorSubtract( defender->currentOrigin, attacker->currentOrigin, defDir );
	VectorCopy( attacker->client->ps.viewangles, attAngles );
	attAngles[YAW] = vectoyaw( defDir );
	SetClientViewAngle( attacker, attAngles );
	defAngles[PITCH] = attAngles[PITCH]*-1;
	defAngles[YAW] = AngleNormalize180( attAngles[YAW] + 180);
	defAngles[ROLL] = 0;
	SetClientViewAngle( defender, defAngles );

	//MATCH POSITIONS
	vec3_t	newOrg;
	/*
	idealDist -= fabs(defPitchAdd)/8.0f;
	*/
	float scale = VectorLength( attacker->s.modelScale );
	if ( scale )
	{
		idealDist += 8*(scale-1.0f);
	}
	scale = VectorLength( defender->s.modelScale );
	if ( scale )
	{
		idealDist += 8*(scale-1.0f);
	}

	float diff = VectorNormalize( defDir ) - idealDist;//diff will be the total error in dist
	//try to move attacker half the diff towards the defender
	VectorMA( attacker->currentOrigin, diff*0.5f, defDir, newOrg );
	trace_t	trace;
	gi.trace( &trace, attacker->currentOrigin, attacker->mins, attacker->maxs, newOrg, attacker->s.number, attacker->clipmask, G2_NOCOLLIDE, 0 );
	if ( !trace.startsolid && !trace.allsolid )
	{
		G_SetOrigin( attacker, trace.endpos );
		gi.linkentity( attacker );
	}
	//now get the defender's dist and do it for him too
	vec3_t	attDir;
	VectorSubtract( attacker->currentOrigin, defender->currentOrigin, attDir );
	diff = VectorNormalize( attDir ) - idealDist;//diff will be the total error in dist
	//try to move defender all of the remaining diff towards the attacker
	VectorMA( defender->currentOrigin, diff, attDir, newOrg );
	gi.trace( &trace, defender->currentOrigin, defender->mins, defender->maxs, newOrg, defender->s.number, defender->clipmask, G2_NOCOLLIDE, 0 );
	if ( !trace.startsolid && !trace.allsolid )
	{
		G_SetOrigin( defender, trace.endpos );
		gi.linkentity( defender );
	}

	//DONE!

	return qtrue;
}

qboolean WP_SabersCheckLock( gentity_t *ent1, gentity_t *ent2 )
{
	if ( ent1->client->playerTeam == ent2->client->playerTeam )
	{
		return qfalse;
	}
	if ( ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}
	if ( ent1->painDebounceTime > level.time-1000 || ent2->painDebounceTime > level.time-1000 )
	{//can't saberlock if you're not ready
		return qfalse;
	}
	if ( fabs( ent1->currentOrigin[2]-ent2->currentOrigin[2]) > 18 )
	{
		return qfalse;
	}
	float dist = DistanceSquared(ent1->currentOrigin,ent2->currentOrigin);
	if ( dist < 64 || dist > 6400 )//( dist < 128 || dist > 2304 )
	{//between 8 and 80 from each other//was 16 and 48
		return qfalse;
	}
	if ( !InFOV( ent1, ent2, 40, 180 ) || !InFOV( ent2, ent1, 40, 180 ) )
	{
		return qfalse;
	}
	//Check for certain anims that *cannot* lock
	//FIXME: there should probably be a whole *list* of these, but I'll put them in as they come up
	if ( ent1->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent1->client->ps.torsoAnimTimer > 300 )
	{//can't lock when saber is behind you
		return qfalse;
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent2->client->ps.torsoAnimTimer > 300 )
	{//can't lock when saber is behind you
		return qfalse;
	}
	//BR to TL lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A5_BR_TL )
	{//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A5_BR_TL )
	{//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BR );
	}
	//BL to TR lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A5_BL_TR )
	{//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A5_BL_TR )
	{//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BL );
	}
	//L to R lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A5__L__R )
	{//ent1 is attacking l to r
		return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		*/
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A5__L__R )
	{//ent2 is attacking l to r
		return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		*/
	}
	//R to L lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A5__R__L )
	{//ent1 is attacking r to l
		return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		*/
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A5__R__L )
	{//ent2 is attacking r to l
		return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		*/
	}
	//TR to BL lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A5_TR_BL )
	{//ent1 is attacking diagonally
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		*/
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A5_TR_BL )
	{//ent2 is attacking diagonally
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BL );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		*/
	}

	//TL to BR lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A5_TL_BR )
	{//ent1 is attacking diagonally
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		*/
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A5_TL_BR )
	{//ent2 is attacking diagonally
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BR );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		*/
	}
	//T to B lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A5_T__B_ )
	{//ent1 is attacking top-down
		/*
		if ( ent2->client->ps.torsoAnim == BOTH_P1_S1_T_ ||
			ent2->client->ps.torsoAnim == BOTH_K1_S1_T_ )
		*/
		{//ent2 is blocking at top
			return WP_SabersCheckLock2( ent1, ent2, LOCK_TOP );
		}
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A5_T__B_ )
	{//ent2 is attacking top-down
		/*
		if ( ent1->client->ps.torsoAnim == BOTH_P1_S1_T_ ||
			ent1->client->ps.torsoAnim == BOTH_K1_S1_T_ )
		*/
		{//ent1 is blocking at top
			return WP_SabersCheckLock2( ent2, ent1, LOCK_TOP );
		}
	}
	/*
	if ( !Q_irand( 0, 10 ) )
	{
		return WP_SabersCheckLock2( ent1, ent2, LOCK_RANDOM );
	}
	*/
	return qfalse;
}

qboolean WP_SaberParry( gentity_t *victim, gentity_t *attacker )
{
	if ( !victim || !victim->client || !attacker )
	{
		return qfalse;
	}
	if ( victim->s.number || g_saberAutoBlocking->integer || victim->client->ps.saberBlockingTime > level.time )
	{//either an NPC or a player who is blocking
		if ( !PM_SaberInTransitionAny( victim->client->ps.saberMove )
			&& !PM_SaberInBounce( victim->client->ps.saberMove )
			&& !PM_SaberInKnockaway( victim->client->ps.saberMove ) )
		{//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberBlockNonRandom( victim, saberHitLocation, qfalse );
		}
		victim->client->ps.saberEventFlags |= SEF_PARRIED;

		//since it was parried, take away any damage done
		//FIXME: what if the damage was done before the parry?
		WP_SaberClearDamageForEntNum( victim->s.number );

		//tell the victim to get mad at me
		if ( victim->enemy != attacker && victim->client->playerTeam != attacker->client->playerTeam )
		{//they're not mad at me and they're not on my team
			G_ClearEnemy( victim );
			G_SetEnemy( victim, attacker );
		}
		return qtrue;
	}
	return qfalse;
}

qboolean WP_BrokenParryKnockDown( gentity_t *victim )
{
	if ( !victim || !victim->client )
	{
		return qfalse;
	}
	if ( victim->client->ps.saberMove == LS_PARRY_UP
		|| victim->client->ps.saberMove == LS_PARRY_UR
		|| victim->client->ps.saberMove == LS_PARRY_UL
		|| victim->client->ps.saberMove == LS_H1_BR
		|| victim->client->ps.saberMove == LS_H1_B_
		|| victim->client->ps.saberMove == LS_H1_BL )
	{//knock their asses down!
		int knockAnim = BOTH_KNOCKDOWN1;
		if ( PM_CrouchAnim( victim->client->ps.legsAnim ) )
		{
			knockAnim = BOTH_KNOCKDOWN4;
		}
		NPC_SetAnim( victim, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		G_AddEvent( victim, EV_PAIN, victim->health );
		return qtrue;
	}
	return qfalse;
}
/*
---------------------------------------------------------
void WP_SaberDamageTrace( gentity_t *ent )

  Constantly trace from the old blade pos to new, down the saber beam and do damage

  FIXME: if the dot product of the old muzzle dir and the new muzzle dir is < 0.75, subdivide it and do multiple traces so we don't flatten out the arc!
---------------------------------------------------------
*/
#define MAX_SABER_SWING_INC 0.33f
void WP_SaberDamageTrace( gentity_t *ent )
{
	vec3_t		mp1, mp2, md1, md2, baseOld, baseNew, baseDiff, endOld, endNew, bladePointOld, bladePointNew;
	float		tipDmgMod = 1.0f;
	float		baseDamage;
	int			baseDFlags = 0;
	qboolean	hit_wall = qfalse;
	qboolean	brokenParry = qfalse;

	for ( int ven = 0; ven < MAX_SABER_VICTIMS; ven++ )
	{
		victimEntityNum[ven] = ENTITYNUM_NONE;
	}
	memset( totalDmg, 0, sizeof( totalDmg) );
	memset( dmgDir, 0, sizeof( dmgDir ) );
	memset( dmgSpot, 0, sizeof( dmgSpot ) );
	memset( dmgFraction, 0, sizeof( dmgFraction ) );
	memset( hitLoc, HL_NONE, sizeof( hitLoc ) );
	memset( hitDismemberLoc, HL_NONE, sizeof( hitDismemberLoc ) );
	memset( hitDismember, qfalse, sizeof( hitDismember ) );
	numVictims = 0;
	VectorClear(saberHitLocation);
	VectorClear(saberHitNormal);
	saberHitFraction = 1.0;	// Closest saber hit.  The saber can do no damage past this point.
	saberHitEntity = ENTITYNUM_NONE;
	sabersCrossed = -1;

	if ( !ent->client )
	{
		return;
	}

	if ( !ent->s.number )
	{//player never uses these
		ent->client->ps.saberEventFlags &= ~SEF_EVENTS;
	}

	if ( ent->client->ps.saberLength <= 1 )//cen get down to 1 when in a wall
	{//saber is not on
		return;
	}

	if ( VectorCompare( ent->client->renderInfo.muzzlePointOld, vec3_origin ) || VectorCompare( ent->client->renderInfo.muzzleDirOld, vec3_origin ) )
	{
		//just started up the saber?
		return;
	}

	int saberContents = gi.pointcontents( ent->client->renderInfo.muzzlePoint, ent->client->ps.saberEntityNum );
	if ( (saberContents&CONTENTS_WATER)||
		(saberContents&CONTENTS_SLIME)||
		(saberContents&CONTENTS_LAVA) )
	{//um... turn off?  Or just set length to 1?
		//FIXME: short-out effect/sound?
		ent->client->ps.saberActive = qfalse;
		return;
	}
	else if ( saberContents&CONTENTS_OUTSIDE )
	{
		if ( (level.worldFlags&WF_RAINING) )
		{
			//add steam in rain
			if ( Q_flrand( 0, 500 ) < ent->client->ps.saberLength )
			{
				vec3_t	end, normal = {0,0,1};//FIXME: opposite of rain angles?
				VectorMA( ent->client->renderInfo.muzzlePoint, ent->client->ps.saberLength*Q_flrand(0, 1), ent->client->renderInfo.muzzleDir, end );
				G_PlayEffect( "saber/fizz", end, normal );
			}
		}
	}

	//FIXMEFIXMEFIXME: When in force speed (esp. lvl 3), need to interpolate this because
	//		we animate so much faster that the arc is pretty much flat...

	int entPowerLevel;
	if ( !ent->s.number && (ent->client->ps.forcePowersActive&(1<<FP_SPEED)) )
	{
		entPowerLevel = FORCE_LEVEL_3;
	}
	else
	{
		entPowerLevel = PM_PowerLevelForSaberAnim( &ent->client->ps );
	}

	if ( ent->client->ps.saberInFlight )
	{//flying sabers are much more deadly
		//unless you're dead
		if ( ent->health <= 0 && !g_saberRealisticCombat->integer )
		{//so enemies don't keep trying to block it
			//FIXME: still do damage, just not to humanoid clients who should try to avoid it
			//baseDamage = 0.0f;
			return;
		}
		//or unless returning
		else if ( ent->client->ps.saberEntityState == SES_RETURNING )
		{//special case, since we're returning, chances are if we hit something
			//it's going to be butt-first.  So do less damage.
			baseDamage = 0.1f;
		}
		else
		{
			if ( !ent->s.number )
			{//cheat for player
				baseDamage = 10.0f;
			}
			else
			{
				baseDamage = 2.5f;
			}
		}
		//Use old to current since can't predict it
		VectorCopy( ent->client->renderInfo.muzzlePointOld, mp1 );
		VectorCopy( ent->client->renderInfo.muzzleDirOld, md1 );
		VectorCopy( ent->client->renderInfo.muzzlePoint, mp2 );
		VectorCopy( ent->client->renderInfo.muzzleDir, md2 );
	}
	else
	{
		if ( ent->client->ps.saberMove == LS_READY )
		{//just do effects
			if ( g_saberRealisticCombat->integer < 2 )
			{//don't kill with this hit
				baseDFlags = DAMAGE_NO_KILL;
			}
			baseDamage = 0;
		}
		else if ( ent->client->ps.saberLockTime > level.time )
		{//just do effects
			baseDamage = 0;
		}
		else if ( ent->client->ps.saberBlocked > BLOCKED_NONE
				 || ( !PM_SaberInAttack( ent->client->ps.saberMove )
					  && !PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
					  && !PM_SaberInTransitionAny( ent->client->ps.saberMove )
					)
				)
		{//don't do damage if parrying/reflecting/bouncing/deflecting or not actually attacking or in a transition to/from/between attacks
			baseDamage = 0;
		}
		else
		{//okay, in a saberMove that does damage
			//make sure we're in the right anim
			if ( !PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
				&& !PM_InAnimForSaberMove( ent->client->ps.torsoAnim, ent->client->ps.saberMove ) )
			{//forced into some other animation somehow, like a pain or death?
				baseDamage = 0;
			}
			else if ( ent->client->ps.weaponstate == WEAPON_FIRING && ent->client->ps.saberBlocked == BLOCKED_NONE &&
				( PM_SaberInAttack(ent->client->ps.saberMove) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) || PM_SpinningSaberAnim(ent->client->ps.torsoAnim) || entPowerLevel > FORCE_LEVEL_2 ) )
			{//normal attack swing swinging/spinning (or if using strong set), do normal damage
				//FIXME: more damage for higher attack power levels?
				//		More damage based on length/color of saber?
				//FIXME: Desann does double damage?
				if ( g_saberRealisticCombat->integer )
				{
					switch ( entPowerLevel )
					{
					case FORCE_LEVEL_3:
						baseDamage = 10.0f;
						break;
					case FORCE_LEVEL_2:
						baseDamage = 5.0f;
						break;
					default:
					case FORCE_LEVEL_1:
						baseDamage = 2.5f;
						break;
					}
				}
				else
				{
					baseDamage = 2.5f * (float)entPowerLevel;
				}
			}
			else
			{//saber is transitioning, defending or idle, don't do as much damage
				//FIXME: strong attacks and returns should do damage and be unblockable
				if ( g_timescale->value < 1.0 )
				{//in slow mo or force speed, we need to do damage during the transitions
					if ( g_saberRealisticCombat->integer )
					{
						switch ( entPowerLevel )
						{
						case FORCE_LEVEL_3:
							baseDamage = 10.0f;
							break;
						case FORCE_LEVEL_2:
							baseDamage = 5.0f;
							break;
						default:
						case FORCE_LEVEL_1:
							baseDamage = 2.5f;
							break;
						}
					}
					else
					{
						baseDamage = 2.5f * (float)entPowerLevel;
					}
				}
				else// if ( !ent->s.number )
				{//I have to do *some* damage in transitions or else you feel like a total gimp
					baseDamage = 0.1f;
				}
				/*
				else
				{
					baseDamage = 0;//was 1.0f;//was 0.25
				}
				*/
			}
		}

		//Use current to next since can predict it
		//FIXME: if they're closer than the saber blade start, we don't want the
		//		arm to pass through them without any damage... so check the radius
		//		and push them away (do pain & knockback)
		//FIXME: if going into/coming from a parry/reflection or going into a deflection, don't use old mp & dir?  Otherwise, deflections will cut through?
		//VectorCopy( ent->client->renderInfo.muzzlePoint, mp1 );
		//VectorCopy( ent->client->renderInfo.muzzleDir, md1 );
		//VectorCopy( ent->client->renderInfo.muzzlePointNext, mp2 );
		//VectorCopy( ent->client->renderInfo.muzzleDirNext, md2 );
		//prediction was causing gaps in swing (G2 problem) so *don't* predict
		VectorCopy( ent->client->renderInfo.muzzlePointOld, mp1 );
		VectorCopy( ent->client->renderInfo.muzzleDirOld, md1 );
		VectorCopy( ent->client->renderInfo.muzzlePoint, mp2 );
		VectorCopy( ent->client->renderInfo.muzzleDir, md2 );

		//NOTE: this is a test, may not be necc, as I can still swing right through someone without hitting them, somehow...
		//see if anyone is so close that they're within the dist from my origin to the start of the saber
		if ( ent->health > 0 && !ent->client->ps.saberLockTime )
		{
			trace_t trace;
			gi.trace( &trace, ent->currentOrigin, vec3_origin, vec3_origin, mp1, ent->s.number, (MASK_SHOT&~(CONTENTS_CORPSE|CONTENTS_ITEM)), G2_NOCOLLIDE, 0 );
			if ( trace.entityNum < ENTITYNUM_WORLD && (trace.entityNum > 0||ent->client->NPC_class == CLASS_DESANN) )//NPCs don't push player away, unless it's Desann
			{//a valid ent
				gentity_t *traceEnt = &g_entities[trace.entityNum];
				if ( traceEnt
					&& traceEnt->client
					&& traceEnt->health > 0
					&& traceEnt->client->playerTeam != ent->client->playerTeam
					&& !PM_InKnockDown( &traceEnt->client->ps ) )
				{//enemy client, push them away
					if ( !traceEnt->client->ps.saberLockTime && !traceEnt->message )
					{//don't push people in saberlock or with security keys
						vec3_t hitDir;
						VectorSubtract( trace.endpos, ent->currentOrigin, hitDir );
						float totalDist = Distance( mp1, ent->currentOrigin );
						float knockback = (totalDist-VectorNormalize( hitDir ))/totalDist * 200.0f;
						hitDir[2] = 0;
						//FIXME: do we need to call G_Throw?  Seems unfair to put actual knockback on them, stops the attack
						//G_Throw( traceEnt, hitDir, knockback );
						VectorMA( traceEnt->client->ps.velocity, knockback, hitDir, traceEnt->client->ps.velocity );
						traceEnt->client->ps.pm_time = 200;
						traceEnt->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							gi.Printf( "%s pushing away %s at %s\n", ent->NPC_type, traceEnt->NPC_type, vtos( traceEnt->client->ps.velocity ) );
						}
#endif
					}
				}
			}
		}
	}

	if ( g_saberRealisticCombat->integer > 1 )
	{//always do damage, and lots of it
		if ( g_saberRealisticCombat->integer > 2 )
		{//always do damage, and lots of it
			baseDamage = 25.0f;
		}
		else if ( baseDamage > 0.1f )
		{//only do super damage if we would have done damage according to normal rules
			baseDamage = 25.0f;
		}
	}
	else if ( (!ent->s.number&&ent->client->ps.forcePowersActive&(1<<FP_SPEED)) )
	{
		baseDamage *= (1.0f-g_timescale->value);
	}
	// Get the old state of the blade
	VectorCopy( mp1, baseOld );
	VectorMA( baseOld, ent->client->ps.saberLength, md1, endOld );
	// Get the future state of the blade
	VectorCopy( mp2, baseNew );
	VectorMA( baseNew, ent->client->ps.saberLength, md2, endNew );

	sabersCrossed = -1;
	if ( VectorCompare2( baseOld, baseNew ) && VectorCompare2( endOld, endNew ) )
	{
		hit_wall = WP_SaberDamageForTrace( ent->s.number, mp2, endNew, baseDamage*4, md2, qfalse, entPowerLevel, qfalse );
	}
	else
	{
		float aveLength, step = 8, stepsize = 8;
		vec3_t	ma1, ma2, md2ang, curBase1, curBase2;
		int	xx;
		//do the trace at the base first
		hit_wall = WP_SaberDamageForTrace( ent->s.number, baseOld, baseNew, baseDamage, md2, qfalse, entPowerLevel );

		//if hit a saber, shorten rest of traces to match
		if ( saberHitFraction < 1.0 )
		{
			//adjust muzzleDir...
			vec3_t ma1, ma2;
			vectoangles( md1, ma1 );
			vectoangles( md2, ma2 );
			for ( xx = 0; xx < 3; xx++ )
			{
				md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], saberHitFraction );
			}
			AngleVectors( md2ang, md2, NULL, NULL );
			//shorten the base pos
			VectorSubtract( mp2, mp1, baseDiff );
			VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
			VectorMA( baseNew, ent->client->ps.saberLength, md2, endNew );
		}

		//If the angle diff in the blade is high, need to do it in chunks of 33 to avoid flattening of the arc
		float dirInc, curDirFrac;
		if ( PM_SaberInAttack( ent->client->ps.saberMove )
			|| PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
			|| PM_SpinningSaberAnim( ent->client->ps.torsoAnim )
			|| PM_InSpecialJump( ent->client->ps.torsoAnim )
			|| (g_timescale->value<1.0f&&PM_SaberInTransitionAny( ent->client->ps.saberMove )) )
		{
			curDirFrac = DotProduct( md1, md2 );
		}
		else
		{
			curDirFrac = 1.0f;
		}
		//NOTE: if saber spun at least 180 degrees since last damage trace, this is not reliable...!
		if ( fabs(curDirFrac) < 1.0f - MAX_SABER_SWING_INC )
		{//the saber blade spun more than 33 degrees since the last damage trace
			curDirFrac = dirInc = 1.0f/((1.0f - curDirFrac)/MAX_SABER_SWING_INC);
		}
		else
		{
			curDirFrac = 1.0f;
			dirInc = 0.0f;
		}
		qboolean hit_saber = qfalse;

		vectoangles( md1, ma1 );
		vectoangles( md2, ma2 );

		vec3_t curMD1, curMD2;//, mdDiff, dirDiff;
		//VectorSubtract( md2, md1, mdDiff );
		VectorCopy( md1, curMD2 );
		VectorCopy( baseOld, curBase2 );

		while ( 1 )
		{
			VectorCopy( curMD2, curMD1 );
			VectorCopy( curBase2, curBase1 );
			if ( curDirFrac >= 1.0f )
			{
				VectorCopy( md2, curMD2 );
				VectorCopy( baseNew, curBase2 );
			}
			else
			{
				for ( xx = 0; xx < 3; xx++ )
				{
					md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], curDirFrac );
				}
				AngleVectors( md2ang, curMD2, NULL, NULL );
				//VectorMA( md1, curDirFrac, mdDiff, curMD2 );
				VectorSubtract( baseNew, baseOld, baseDiff );
				VectorMA( baseOld, curDirFrac, baseDiff, curBase2 );
			}
			// Move up the blade in intervals of stepsize
			for ( step = stepsize; step < ent->client->ps.saberLength && step < ent->client->ps.saberLengthOld; step+=12 )
			{
				VectorMA( curBase1, step, curMD1, bladePointOld );
				VectorMA( curBase2, step, curMD2, bladePointNew );
				if ( WP_SaberDamageForTrace( ent->s.number, bladePointOld, bladePointNew, baseDamage, curMD2, qfalse, entPowerLevel ) )
				{
					hit_wall = qtrue;
				}

				//if hit a saber, shorten rest of traces to match
				if ( saberHitFraction < 1.0 )
				{
					//adjust muzzle endpoint
					VectorSubtract( mp2, mp1, baseDiff );
					VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
					VectorMA( baseNew, ent->client->ps.saberLength, curMD2, endNew );
					//adjust muzzleDir...
					vec3_t curMA1, curMA2;
					vectoangles( curMD1, curMA1 );
					vectoangles( curMD2, curMA2 );
					for ( xx = 0; xx < 3; xx++ )
					{
						md2ang[xx] = LerpAngle( curMA1[xx], curMA2[xx], saberHitFraction );
					}
					AngleVectors( md2ang, curMD2, NULL, NULL );
					/*
					VectorSubtract( curMD2, curMD1, dirDiff );
					VectorMA( curMD1, saberHitFraction, dirDiff, curMD2 );
					*/
					hit_saber = qtrue;
				}
				if (hit_wall)
				{
					break;
				}
			}
			if ( hit_wall || hit_saber )
			{
				break;
			}
			if ( curDirFrac >= 1.0f )
			{
				break;
			}
			else
			{
				curDirFrac += dirInc;
				if ( curDirFrac >= 1.0f )
				{
					curDirFrac = 1.0f;
				}
			}
		}

		//do the trace at the end last
		//Special check- adjust for length of blade not being a multiple of 12
		aveLength = (ent->client->ps.saberLengthOld + ent->client->ps.saberLength)/2;
		if ( step > aveLength )
		{//less dmg if the last interval was not stepsize
			tipDmgMod = (stepsize-(step-aveLength))/stepsize;
		}
		//NOTE: since this is the tip, we do not extrapolate the extra 16
		if ( WP_SaberDamageForTrace( ent->s.number, endOld, endNew, tipDmgMod*baseDamage, md2, qfalse, entPowerLevel, qfalse ) )
		{
			hit_wall = qtrue;
		}
	}

	if ( (saberHitFraction < 1.0f||(sabersCrossed>=0&&sabersCrossed<=32.0f)) && (ent->client->ps.weaponstate == WEAPON_FIRING || ent->client->ps.saberInFlight) )
	{// The saber (in-hand) hit another saber, mano.
		qboolean inFlightSaberBlocked = qfalse;
		qboolean collisionResolved = qfalse;
		qboolean deflected = qfalse;

		gentity_t *hitEnt = &g_entities[saberHitEntity];
		gentity_t *hitOwner = NULL;
		int hitOwnerPowerLevel = FORCE_LEVEL_0;

		if ( hitEnt )
		{
			hitOwner = hitEnt->owner;
		}
		if ( hitOwner && hitOwner->client )
		{
			hitOwnerPowerLevel = PM_PowerLevelForSaberAnim( &hitOwner->client->ps );
			if ( entPowerLevel == FORCE_LEVEL_3 && PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
			{//a special "unblockable" attack
				if ( hitOwner->client->NPC_class == CLASS_DESANN
					|| hitOwner->client->NPC_class == CLASS_TAVION
					|| hitOwner->client->NPC_class == CLASS_LUKE )
				{//these masters can even block unblockables (stops cheap kills)
					entPowerLevel = FORCE_LEVEL_2;
				}
			}
		}

		//FIXME: check for certain anims, facing, etc, to make them lock into a sabers-locked pose
		//SEF_LOCKED

		if ( ent->client->ps.saberInFlight &&
			ent->client->ps.saberActive &&
			ent->client->ps.saberEntityNum != ENTITYNUM_NONE &&
			ent->client->ps.saberEntityState != SES_RETURNING )
		{//saber was blocked, return it
			inFlightSaberBlocked = qtrue;
		}

		//FIXME: based on strength, position and angle of attack & defense, decide if:
		//	defender and attacker lock sabers
		//	*defender's parry should hold and attack bounces (or deflects, based on angle of sabers)
		//	*defender's parry is somewhat broken and both bounce (or deflect)
		//	*defender's parry is broken and they bounce while attacker's attack deflects or carries through (especially if they're dead)
		//	defender is knocked down and attack goes through

		//Check deflections and broken parries
		if ( hitOwner && hitOwner->health > 0 && ent->health > 0 //both are alive
			&& !inFlightSaberBlocked && hitOwner->client && !hitOwner->client->ps.saberInFlight && !ent->client->ps.saberInFlight//both have sabers in-hand
			&& ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
			&& ent->client->ps.saberLockTime < level.time
			&& hitOwner->client->ps.saberLockTime < level.time )
		{//2 in-hand sabers hit
			//FIXME: defender should not parry or block at all if not in a saber anim... like, if in a roll or knockdown...
			if ( baseDamage )
			{//there is damage involved, not just effects
				qboolean entAttacking = qfalse;
				qboolean hitOwnerAttacking = qfalse;
				qboolean entDefending = qfalse;
				qboolean hitOwnerDefending = qfalse;

				if ( PM_SaberInAttack( ent->client->ps.saberMove ) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
				{
					entAttacking = qtrue;
				}
				else if ( entPowerLevel > FORCE_LEVEL_2 )
				{
					if ( PM_SaberInAttack( ent->client->ps.saberMove ) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) || PM_SaberInTransitionAny( ent->client->ps.saberMove ) )
					{
						entAttacking = qtrue;
					}
				}
				if ( PM_SaberInParry( ent->client->ps.saberMove )
					|| ent->client->ps.saberMove == LS_READY )
				{
					entDefending = qtrue;
				}

				if ( PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) )
				{
					hitOwnerAttacking = qtrue;
				}
				else if ( hitOwnerPowerLevel > FORCE_LEVEL_2 )
				{
					if ( PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) || PM_SaberInTransitionAny( hitOwner->client->ps.saberMove ) )
					{
						hitOwnerAttacking = qtrue;
					}
				}
				if ( PM_SaberInParry( hitOwner->client->ps.saberMove )
					 || hitOwner->client->ps.saberMove == LS_READY )
				{
					hitOwnerDefending = qtrue;
				}

				if ( entAttacking
					&& hitOwnerAttacking
					&& ( entPowerLevel == hitOwnerPowerLevel
						|| (entPowerLevel > FORCE_LEVEL_2 && hitOwnerPowerLevel > FORCE_LEVEL_2 )
						|| (entPowerLevel < FORCE_LEVEL_3 && hitOwnerPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_2 && Q_irand( 0, 2 ))
						|| (entPowerLevel < FORCE_LEVEL_2 && hitOwnerPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_1 && Q_irand( 0, 1 ))
						|| (hitOwnerPowerLevel < FORCE_LEVEL_3 && entPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_2 && !Q_irand( 0, 1 ))
						|| (hitOwnerPowerLevel < FORCE_LEVEL_2 && entPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_1 && !Q_irand( 0, 1 )))
					&& WP_SabersCheckLock( ent, hitOwner ) )
				{
					collisionResolved = qtrue;
				}
				else if ( hitOwnerAttacking
					&& entDefending
					&& !Q_irand( 0, 2 )
					&& (ent->client->ps.saberMove != LS_READY || (hitOwnerPowerLevel-ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE]) < Q_irand( -6, 0 ) )
					&& ((hitOwnerPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )||
						(hitOwnerPowerLevel < FORCE_LEVEL_2 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_1 )||
						(hitOwnerPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_0 && !Q_irand( 0, (hitOwnerPowerLevel-ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE]+1)*2 )))
					&& WP_SabersCheckLock( hitOwner, ent ) )
				{
					collisionResolved = qtrue;
				}
				else if ( entAttacking && hitOwnerDefending )
				{//I'm attacking hit, they're parrying
					qboolean activeDefense = (hitOwner->s.number||g_saberAutoBlocking->integer||hitOwner->client->ps.saberBlockingTime > level.time);
					if ( !Q_irand( 0, 2 )
						&& activeDefense
						&& (hitOwner->client->ps.saberMove != LS_READY || (entPowerLevel-hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]) < Q_irand( -6, 0 ) )
						&& ( ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )
							|| ( entPowerLevel < FORCE_LEVEL_2 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_1 )
							|| ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_0 && !Q_irand( 0, (entPowerLevel-hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]+1)*2 )) )
						&& WP_SabersCheckLock( ent, hitOwner ) )
					{
						collisionResolved = qtrue;
					}
					else if ( saberHitFraction < 1.0f )
					{//an actual collision
						if ( entPowerLevel < FORCE_LEVEL_3 && activeDefense )
						{//strong attacks cannot be deflected
							//based on angle of attack & angle of defensive saber, see if I should deflect off in another dir rather than bounce back
							deflected = WP_GetSaberDeflectionAngle( ent, hitOwner );
							//just so Jedi knows that he was blocked
							ent->client->ps.saberEventFlags |= SEF_BLOCKED;
						}
						//base parry breaks on animation (saber attack level), not FP_SABER_OFFENSE
						if ( entPowerLevel < FORCE_LEVEL_3
							//&& ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_3//if you have high saber offense, you cannot have your attack knocked away, regardless of what style you're using?
							&& hitOwner->client->ps.saberAnimLevel != FORCE_LEVEL_5
							&& activeDefense
							&& (hitOwnerPowerLevel > FORCE_LEVEL_2||(hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_2&&Q_irand(0,hitOwner->client->ps.saberAnimLevel))) )
						{//knockaways can make fast-attacker go into a broken parry anim if the ent is using fast or med (but not Tavion)
							//make me parry
							WP_SaberParry( hitOwner, ent );
							//turn the parry into a knockaway
							hitOwner->client->ps.saberBounceMove = PM_KnockawayForParry( hitOwner->client->ps.saberBlocked );
							//make them go into a broken parry
							ent->client->ps.saberBounceMove = PM_BrokenParryForAttack( ent->client->ps.saberMove );
							ent->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							if ( ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
								&& (ent->s.number||g_saberRealisticCombat->integer) )//&& !Q_irand( 0, 3 ) )
							{//knocked the saber right out of his hand! (never happens to player)
								//Get a good velocity to send the saber in based on my parry move
								vec3_t	throwDir;
								if ( !PM_VelocityForBlockedMove( &hitOwner->client->ps, throwDir ) )
								{
									PM_VelocityForSaberMove( &ent->client->ps, throwDir );
								}
								WP_SaberLose( ent, throwDir );
							}
							//just so Jedi knows that he was blocked
							ent->client->ps.saberEventFlags |= SEF_BLOCKED;
#ifndef FINAL_BUILD
							if ( d_saberCombat->integer )
							{
								gi.Printf( S_COLOR_RED"%s knockaway %s's attack, new move = %s, anim = %s\n", hitOwner->NPC_type, ent->NPC_type, saberMoveData[ent->client->ps.saberBounceMove].name, animTable[saberMoveData[ent->client->ps.saberBounceMove].animToUse].name );
							}
#endif
						}
						else if ( entPowerLevel > FORCE_LEVEL_2
							|| !activeDefense
							|| (!deflected && Q_irand( 0, PM_PowerLevelForSaberAnim( &ent->client->ps ) - hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]/*PM_PowerLevelForSaberAnim( &hitOwner->client->ps )*/ ) > 0 ) )
						{//broke their parry altogether
							if ( entPowerLevel > FORCE_LEVEL_2 || Q_irand( 0, ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] - hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] ) )
							{//chance of continuing with the attack (not bouncing back)
								ent->client->ps.saberEventFlags &= ~SEF_BLOCKED;
								ent->client->ps.saberBounceMove = LS_NONE;
								brokenParry = qtrue;
							}
							//do some time-consuming saber-knocked-aside broken parry anim
							hitOwner->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							hitOwner->client->ps.saberBounceMove = LS_NONE;
							if ( hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
								&& (ent->s.number||g_saberRealisticCombat->integer)
								&& !Q_irand( 0, 2 ) )
							{//knocked the saber right out of his hand!
								//get the right velocity for my attack direction
								vec3_t	throwDir;
								PM_VelocityForSaberMove( &ent->client->ps, throwDir );
								WP_SaberLose( hitOwner, throwDir );
								if ( (ent->client->ps.saberAnimLevel == FORCE_LEVEL_3 && !Q_irand(0,3) )
									|| ( ent->client->ps.saberAnimLevel==FORCE_LEVEL_4&&!Q_irand(0,1) ) )
								{// a strong attack
									if ( WP_BrokenParryKnockDown( hitOwner ) )
									{
										hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
										hitOwner->client->ps.saberBounceMove = LS_NONE;
									}
								}
							}
							else
							{
								if ( (ent->client->ps.saberAnimLevel == FORCE_LEVEL_3 && !Q_irand(0,5) )
									|| ( ent->client->ps.saberAnimLevel==FORCE_LEVEL_4&&!Q_irand(0,3) ) )
								{// a strong attack
									if ( WP_BrokenParryKnockDown( hitOwner ) )
									{
										hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
										hitOwner->client->ps.saberBounceMove = LS_NONE;
									}
								}
							}
#ifndef FINAL_BUILD
							if ( d_saberCombat->integer )
							{
								if ( ent->client->ps.saberEventFlags&SEF_BLOCKED )
								{
									gi.Printf( S_COLOR_RED"%s parry broken (bounce/deflect)!\n", hitOwner->targetname );
								}
								else
								{
									gi.Printf( S_COLOR_RED"%s parry broken (follow-through)!\n", hitOwner->targetname );
								}
							}
#endif
						}
						else
						{
							WP_SaberParry( hitOwner, ent );
							if ( PM_SaberInBounce( ent->client->ps.saberMove ) //FIXME: saberMove not set until pmove!
								&& activeDefense
								&& hitOwner->client->ps.saberAnimLevel != FORCE_LEVEL_1 && hitOwner->client->ps.saberAnimLevel != FORCE_LEVEL_5
								&& hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )
							{//attacker bounced off, and defender has ability to do knockaways, so do one unless we're using fast attacks
								//turn the parry into a knockaway
								hitOwner->client->ps.saberBounceMove = PM_KnockawayForParry( hitOwner->client->ps.saberBlocked );
							}
							else if ( (ent->client->ps.saberAnimLevel == FORCE_LEVEL_3 && !Q_irand(0,6) )
									|| ( ent->client->ps.saberAnimLevel==FORCE_LEVEL_4 && !Q_irand(0,3) ) )
							{// a strong attack can sometimes do a knockdown
								//HMM... maybe only if they're moving backwards?
								if ( WP_BrokenParryKnockDown( hitOwner ) )
								{
									hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
									hitOwner->client->ps.saberBounceMove = LS_NONE;
								}
							}
						}
						collisionResolved = qtrue;
					}
				}
				/*
				else if ( entDefending && hitOwnerAttacking )
				{//I'm parrying, they're attacking
					if ( hitOwnerPowerLevel < FORCE_LEVEL_3 )
					{//strong attacks cannot be deflected
						//based on angle of attack & angle of defensive saber, see if I should deflect off in another dir rather than bounce back
						deflected = WP_GetSaberDeflectionAngle( hitOwner, ent );
						//just so Jedi knows that he was blocked
						hitOwner->client->ps.saberEventFlags |= SEF_BLOCKED;
					}
					//FIXME: base parry breaks on animation (saber attack level), not FP_SABER_OFFENSE
					if ( hitOwnerPowerLevel > FORCE_LEVEL_2 || (!deflected && Q_irand( 0, PM_PowerLevelForSaberAnim( &hitOwner->client->ps ) - ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] ) > 0 ) )
					{//broke my parry altogether
						if ( hitOwnerPowerLevel > FORCE_LEVEL_2 || Q_irand( 0, hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] - ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] ) )
						{//chance of continuing with the attack (not bouncing back)
							hitOwner->client->ps.saberEventFlags &= ~SEF_BLOCKED;
							hitOwner->client->ps.saberBounceMove = LS_NONE;
						}
						//do some time-consuming saber-knocked-aside broken parry anim
						ent->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							if ( hitOwner->client->ps.saberEventFlags&SEF_BLOCKED )
							{
								gi.Printf( S_COLOR_RED"%s parry broken (bounce/deflect)!\n", ent->targetname );
							}
							else
							{
								gi.Printf( S_COLOR_RED"%s parry broken (follow-through)!\n", ent->targetname );
							}
						}
#endif
					}
					else
					{
						WP_SaberParry( ent, hitOwner );
					}
					collisionResolved = qtrue;
				}
				*/
				else
				{//some other kind of in-hand saber collision
				}
			}
		}
		else
		{//some kind of in-flight collision
		}

		if ( saberHitFraction < 1.0f )
		{
			if ( !collisionResolved && baseDamage )
			{//some other kind of in-hand saber collision
				//handle my reaction
				if ( !ent->client->ps.saberInFlight
					&& ent->client->ps.saberLockTime < level.time )
				{//my saber is in hand
					if ( ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
					{
						if ( PM_SaberInAttack( ent->client->ps.saberMove ) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) ||
							(entPowerLevel > FORCE_LEVEL_2&&!PM_SaberInIdle(ent->client->ps.saberMove)&&!PM_SaberInParry(ent->client->ps.saberMove)&&!PM_SaberInReflect(ent->client->ps.saberMove)) )
						{//in the middle of attacking
							if ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->health > 0 )
							{//don't deflect/bounce in strong attack or when enemy is dead
								WP_GetSaberDeflectionAngle( ent, hitOwner );
								ent->client->ps.saberEventFlags |= SEF_BLOCKED;
								//since it was blocked/deflected, take away any damage done
								//FIXME: what if the damage was done before the parry?
								WP_SaberClearDamageForEntNum( hitOwner->s.number );
							}
						}
						else
						{//saber collided when not attacking, parry it
							//since it was blocked/deflected, take away any damage done
							//FIXME: what if the damage was done before the parry?
							WP_SaberClearDamageForEntNum( hitOwner->s.number );
							/*
							if ( ent->s.number || g_saberAutoBlocking->integer || ent->client->ps.saberBlockingTime > level.time )
							{//either an NPC or a player who has blocking
								if ( !PM_SaberInTransitionAny( ent->client->ps.saberMove ) && !PM_SaberInBounce( ent->client->ps.saberMove ) )
								{//I'm not attacking, in transition or in a bounce, so play a parry
									//just so Jedi knows that he parried something
									WP_SaberBlockNonRandom( ent, saberHitLocation, qfalse );
								}
								ent->client->ps.saberEventFlags |= SEF_PARRIED;
							}
							*/
						}
					}
					else
					{
						//since it was blocked/deflected, take away any damage done
						//FIXME: what if the damage was done before the parry?
						WP_SaberClearDamageForEntNum( hitOwner->s.number );
					}
				}
				else
				{//nothing happens to *me* when my inFlight saber hits something
				}
				//handle their reaction
				if ( hitOwner
					&& hitOwner->health > 0
					&& hitOwner->client
					&& !hitOwner->client->ps.saberInFlight
					&& hitOwner->client->ps.saberLockTime < level.time )
				{//their saber is in hand
					if ( PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) ||
						(hitOwner->client->ps.saberAnimLevel > FORCE_LEVEL_2&&!PM_SaberInIdle(hitOwner->client->ps.saberMove)&&!PM_SaberInParry(hitOwner->client->ps.saberMove)&&!PM_SaberInReflect(hitOwner->client->ps.saberMove)) )
					{//in the middle of attacking
						/*
						if ( hitOwner->client->ps.saberAnimLevel < FORCE_LEVEL_3 )
						{//don't deflect/bounce in strong attack
							WP_GetSaberDeflectionAngle( hitOwner, ent );
							hitOwner->client->ps.saberEventFlags |= SEF_BLOCKED;
						}
						*/
					}
					else
					{//saber collided when not attacking, parry it
						if ( !PM_SaberInBrokenParry( hitOwner->client->ps.saberMove ) )
						{//not currently in a broken parry
							if ( !WP_SaberParry( hitOwner, ent ) )
							{//FIXME: hitOwner can't parry, do some time-consuming saber-knocked-aside broken parry anim?
								//hitOwner->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							}
						}
					}
				}
				else
				{//nothing happens to *hitOwner* when their inFlight saber hits something
				}
			}

			//collision must have been handled by now
			//Set the blocked attack bounce value in saberBlocked so we actually play our saberBounceMove anim
			if ( ent->client->ps.saberEventFlags & SEF_BLOCKED )
			{
				if ( ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
				{
					ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
				}
			}
			/*
			if ( hitOwner && hitOwner->client->ps.saberEventFlags & SEF_BLOCKED )
			{
				hitOwner->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
			*/
		}

		if ( saberHitFraction < 1.0f || collisionResolved )
		{//either actually hit or locked
			if ( ent->client->ps.saberLockTime < level.time )
			{
				if ( inFlightSaberBlocked )
				{//FIXME: never hear this sound
					G_Sound( &g_entities[ent->client->ps.saberEntityNum], G_SoundIndex( va( "sound/weapons/saber/saberbounce%d.wav", Q_irand(1,3) ) ) );
				}
				else
				{
					if ( deflected )
					{
						G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberbounce%d.wav", Q_irand(1,3) ) ) );
					}
					else
					{
						G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
					}
				}
				G_PlayEffect( "saber_block", saberHitLocation, saberHitNormal );
			}
			// Set the little screen flash - only when an attack is blocked
			g_saberFlashTime = level.time-50;
			VectorCopy( saberHitLocation, g_saberFlashPos );
		}

		if ( saberHitFraction < 1.0f )
		{
			if ( inFlightSaberBlocked )
			{//we threw a saber and it was blocked, do any effects, etc.
				int	knockAway = 5;
				if ( hitEnt
					&& hitOwner
					&& hitOwner->client
					&& (PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) || PM_SpinningSaberAnim( hitOwner->client->ps.torsoAnim )) )
				{//if hit someone who was in an attack or spin anim, more likely to have in-flight saber knocked away
					if ( hitOwnerPowerLevel > FORCE_LEVEL_2 )
					{//string attacks almost always knock it aside!
						knockAway = 1;
					}
					else
					{//33% chance
						knockAway = 2;
					}
				}
				if ( !Q_irand( 0, knockAway ) || //random
						( hitOwner && hitOwner->client &&
							(hitOwner->client->NPC_class==CLASS_DESANN||hitOwner->client->NPC_class==CLASS_TAVION||hitOwner->client->NPC_class==CLASS_LUKE)
						) //or if blocked by a Boss character FIXME: or base on defense level?
					)//FIXME: player should not auto-block a flying saber, let him override the parry with an attack to knock the saber from the air, rather than this random chance
				{//knock it aside and turn it off
					G_PlayEffect( "saber_cut", saberHitLocation, saberHitNormal );
					if ( hitEnt )
					{
						vec3_t newDir;

						VectorSubtract( g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin, newDir );
						VectorNormalize( newDir );
						G_ReflectMissile( ent, &g_entities[ent->client->ps.saberEntityNum], newDir );
					}
					Jedi_PlayDeflectSound( hitOwner );
					WP_SaberDrop( ent, &g_entities[ent->client->ps.saberEntityNum] );
				}
				else
				{
					if ( !Q_irand( 0, 2 ) && hitEnt )
					{
						vec3_t newDir;
						VectorSubtract( g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin, newDir );
						VectorNormalize( newDir );
						G_ReflectMissile( ent, &g_entities[ent->client->ps.saberEntityNum], newDir );
					}
					WP_SaberReturn( ent, &g_entities[ent->client->ps.saberEntityNum] );
				}
			}
		}
	}

	if ( ent->client->ps.saberLockTime > level.time
		&& ent->s.number < ent->client->ps.saberLockEnemy
		&& !Q_irand( 0, 3 ) )
	{//need to make some kind of effect
		vec3_t	hitNorm = {0,0,1};
		if ( WP_SabersIntersection( ent, &g_entities[ent->client->ps.saberLockEnemy], g_saberFlashPos ) )
		{
			if ( Q_irand( 0, 10 ) )
			{
				G_PlayEffect( "saber_block", g_saberFlashPos, hitNorm );
			}
			else
			{
				g_saberFlashTime = level.time-50;
				G_PlayEffect( "saber_cut", g_saberFlashPos, hitNorm );
			}
			G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
		}
	}

	if ( WP_SaberApplyDamage( ent, baseDamage, baseDFlags, brokenParry ) )
	{//actually did damage to something
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			gi.Printf( "base damage was %4.2f\n", baseDamage );
		}
#endif
		G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberhit%d.wav", Q_irand( 1, 3 ) ) ) );
	}

	if ( hit_wall )
	{
		//just so Jedi knows that he hit a wall
		ent->client->ps.saberEventFlags |= SEF_HITWALL;
		if ( ent->s.number == 0 )
		{
			AddSoundEvent( ent, ent->currentOrigin, 128, AEL_DISCOVERED );
			AddSightEvent( ent, ent->currentOrigin, 256, AEL_DISCOVERED, 50 );
		}
	}
}

//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================

/*
================
WP_SaberImpact

================
*/
void WP_SaberImpact( gentity_t *owner, gentity_t *saber, trace_t *trace )
{
	gentity_t		*other;

	other = &g_entities[trace->entityNum];

	if ( other->takedamage && (other->svFlags&SVF_BBRUSH) )
	{//a breakable brush?  break it!
		vec3_t dir;
		VectorCopy( saber->s.pos.trDelta, dir );
		VectorNormalize( dir );

		int dmg = other->health*2;
		if ( other->health > 50 && dmg > 20 && !(other->svFlags&SVF_GLASS_BRUSH) )
		{
			dmg = 20;
		}
		G_Damage( other, owner, saber, dir, trace->endpos, dmg, 0, MOD_SABER );
		G_PlayEffect( "saber_cut", trace->endpos, dir );
		if ( owner->s.number == 0 )
		{
			AddSoundEvent( owner, trace->endpos, 256, AEL_DISCOVERED );
			AddSightEvent( owner, trace->endpos, 512, AEL_DISCOVERED, 50 );
		}
		return;
	}

	if ( saber->s.pos.trType == TR_LINEAR )
	{
		//hit a wall? send it back
		WP_SaberReturn( saber->owner, saber );
	}

	if ( other && !other->client && (other->contents&CONTENTS_LIGHTSABER) )//&& other->s.weapon == WP_SABER )
	{//2 in-flight sabers collided!
		//Big flash
		//FIXME: bigger effect/sound?
		//FIXME: STILL DOESNT WORK!!!
		G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
		G_PlayEffect( "saber_block", trace->endpos );
		g_saberFlashTime = level.time-50;
		VectorCopy( trace->endpos, g_saberFlashPos );
	}

	if ( owner && owner->s.number == 0 && owner->client )
	{
		//Add the event
		if ( owner->client->ps.saberLength > 0 )
		{//saber is on, very suspicious
			AddSoundEvent( owner, saber->currentOrigin, 128, AEL_DISCOVERED );
			AddSightEvent( owner, saber->currentOrigin, 256, AEL_DISCOVERED, 50 );
		}
		else
		{//saber is off, not as suspicious
			AddSoundEvent( owner, saber->currentOrigin, 128, AEL_SUSPICIOUS );
			AddSightEvent( owner, saber->currentOrigin, 256, AEL_SUSPICIOUS );
		}
	}

	// check for bounce
	if ( !other->takedamage && ( saber->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) )
	{
		// Check to see if there is a bounce count
		if ( saber->bounceCount ) {
			// decrement number of bounces and then see if it should be done bouncing
			if ( --saber->bounceCount <= 0 ) {
				// He (or she) will bounce no more (after this current bounce, that is).
				saber->s.eFlags &= ~(EF_BOUNCE | EF_BOUNCE_HALF);
				if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
				{
					WP_SaberDrop( saber->owner, saber );
				}
				return;
			}
			else
			{//bounced and still have bounces left
				if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
				{//under telekinetic control
					if ( !gi.inPVS( saber->currentOrigin, owner->client->renderInfo.handRPoint ) )
					{//not in the PVS of my master
						saber->bounceCount -= 25;
					}
				}
			}
		}

		if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
		{
			//don't home for a few frames so we can get around this thing
			trace_t	bounceTr;
			vec3_t	end;
			float owner_dist = Distance( owner->client->renderInfo.handRPoint, saber->currentOrigin );

			VectorMA( saber->currentOrigin, 10, trace->plane.normal, end );
			gi.trace( &bounceTr, saber->currentOrigin, saber->mins, saber->maxs, end, owner->s.number, saber->clipmask, G2_NOCOLLIDE, 0 );
			VectorCopy( bounceTr.endpos, saber->currentOrigin );
			if ( owner_dist > 0 )
			{
				if ( owner_dist > 50 )
				{
					owner->client->ps.saberEntityDist = owner_dist-50;
				}
				else
				{
					owner->client->ps.saberEntityDist = 0;
				}
			}
			return;
		}

		G_BounceMissile( saber, trace );

		if ( saber->s.pos.trType == TR_GRAVITY )
		{//bounced
			//play a bounce sound
			G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
			//change rotation
			VectorCopy( saber->currentAngles, saber->s.apos.trBase );
			saber->s.apos.trType = TR_LINEAR;
			saber->s.apos.trTime = level.time;
			VectorSet( saber->s.apos.trDelta, Q_irand( -300, 300 ), Q_irand( -300, 300 ), Q_irand( -300, 300 ) );
		}
		//see if we stopped
		else if ( saber->s.pos.trType == TR_STATIONARY )
		{//stopped
			//play a bounce sound
			G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
			//stop rotation
			VectorClear( saber->s.apos.trDelta );
			saber->currentAngles[0] = SABER_PITCH_HACK;
			VectorCopy( saber->currentAngles, saber->s.apos.trBase );
			//remember when it fell so it can return automagically
			saber->aimDebounceTime = level.time;
		}
	}
	else if ( other->client && other->health > 0
		&& ( other->client->NPC_class == CLASS_DESANN || other->client->NPC_class == CLASS_TAVION || other->client->NPC_class == CLASS_LUKE || ( other->client->NPC_class == CLASS_GALAKMECH && other->client->ps.powerups[PW_GALAK_SHIELD] > 0 ) ) )
	{//Luke, Desann and Tavion slap thrown sabers aside
		WP_SaberDrop( owner, saber );
		G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
		G_PlayEffect( "saber_block", trace->endpos );
		g_saberFlashTime = level.time-50;
		VectorCopy( trace->endpos, g_saberFlashPos );
		//FIXME: make Luke/Desann/Tavion play an attack anim or some other special anim when this happens
		Jedi_PlayDeflectSound( other );
	}
}

extern float G_PointDistFromLineSegment( const vec3_t start, const vec3_t end, const vec3_t from );
void WP_SaberInFlightReflectCheck( gentity_t *self, usercmd_t *ucmd  )
{
	gentity_t	*ent;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*missile_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e;
	int			ent_count = 0;
	int			radius = 180;
	vec3_t		center, forward;
	vec3_t		tip;
	vec3_t		up = {0,0,1};

	if ( self->NPC && (self->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{//don't react to things flying at me...
		return;
	}
	//sanity checks: make sure we actually have a saberent
	if ( self->client->ps.weapon != WP_SABER )
	{
		return;
	}
	if ( !self->client->ps.saberInFlight )
	{
		return;
	}
	if ( !self->client->ps.saberLength )
	{
		return;
	}
	if ( self->client->ps.saberEntityNum == ENTITYNUM_NONE )
	{
		return;
	}
	gentity_t *saberent = &g_entities[self->client->ps.saberEntityNum];
	if ( !saberent )
	{
		return;
	}
	//okay, enough damn sanity checks

	VectorCopy( saberent->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	//FIXME: check visibility?
	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( ent->client || ent->s.weapon != WP_SABER )
			{//FIXME: wake up bad guys?
				continue;
			}
			if ( ent->s.eFlags & EF_NODRAW )
			{
				continue;
			}
			if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
			{//not a lightsaber
				continue;
			}
		}
		else
		{//FIXME: make exploding missiles explode?
			if ( ent->s.pos.trType == TR_STATIONARY )
			{//nothing you can do with a stationary missile
				continue;
			}
			if ( ent->splashDamage || ent->splashRadius )
			{//can't deflect exploding missiles
				if ( DistanceSquared( ent->currentOrigin, center ) < 256 )//16 squared
				{
					G_MissileImpacted( ent, saberent, ent->currentOrigin, up );
				}
				continue;
			}
		}

		//don't deflect it if it's not within 16 units of the blade
		VectorMA( self->client->renderInfo.muzzlePoint, self->client->ps.saberLength, self->client->renderInfo.muzzleDir, tip );

		if( G_PointDistFromLineSegment( self->client->renderInfo.muzzlePoint, tip, ent->currentOrigin ) > 32 )
		{
			continue;
		}
		// ok, we are within the radius, add us to the incoming list
		missile_list[ent_count] = ent;
		ent_count++;

	}

	if ( ent_count )
	{
		vec3_t	fx_dir;
		// we are done, do we have any to deflect?
		if ( ent_count )
		{
			for ( int x = 0; x < ent_count; x++ )
			{
				if ( missile_list[x]->s.weapon == WP_SABER )
				{//just send it back
					if ( missile_list[x]->owner && missile_list[x]->owner->client && missile_list[x]->owner->client->ps.saberActive && missile_list[x]->s.pos.trType == TR_LINEAR && missile_list[x]->owner->client->ps.saberEntityState != SES_RETURNING )
					{//it's on and being controlled
						//FIXME: prevent it from damaging me?
						WP_SaberReturn( missile_list[x]->owner, missile_list[x] );
						VectorNormalize2( missile_list[x]->s.pos.trDelta, fx_dir );
						G_PlayEffect( "saber_block", missile_list[x]->currentOrigin, fx_dir );
						if ( missile_list[x]->owner->client->ps.saberInFlight && self->client->ps.saberInFlight )
						{
							G_Sound( missile_list[x], G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
							g_saberFlashTime = level.time-50;
							gentity_t *saber = &g_entities[self->client->ps.saberEntityNum];
							vec3_t	org;
							VectorSubtract( missile_list[x]->currentOrigin, saber->currentOrigin, org );
							VectorMA( saber->currentOrigin, 0.5, org, org );
							VectorCopy( org, g_saberFlashPos );
						}
					}
				}
				else
				{//bounce it
					if ( self->client && !self->s.number )
					{
						self->client->sess.missionStats.saberBlocksCnt++;
					}

					G_ReflectMissile( self, missile_list[x], forward );
					//do an effect
					VectorNormalize2( missile_list[x]->s.pos.trDelta, fx_dir );
					G_PlayEffect( "blaster/deflect", missile_list[x]->currentOrigin, fx_dir );
				}
			}
		}
	}
}

qboolean WP_SaberValidateEnemy( gentity_t *self, gentity_t *enemy )
{
	if ( !enemy )
	{
		return qfalse;
	}

	if ( !enemy || enemy == self || !enemy->inuse || !enemy->client )
	{//not valid
		return qfalse;
	}

	if ( enemy->health <= 0 )
	{//corpse
		return qfalse;
	}

	if ( DistanceSquared( self->client->renderInfo.handRPoint, enemy->currentOrigin ) > saberThrowDistSquared[self->client->ps.forcePowerLevel[FP_SABERTHROW]] )
	{//too far
		return qfalse;
	}

	if ( (!InFront( enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.0f) || !G_ClearLOS( self, self->client->renderInfo.eyePoint, enemy ) )
		&& ( DistanceHorizontalSquared( enemy->currentOrigin, self->currentOrigin ) > 65536 || fabs(enemy->currentOrigin[2]-self->currentOrigin[2]) > 384 )  )
	{//(not in front or not clear LOS) & greater than 256 away
		return qfalse;
	}

	if ( enemy->client->playerTeam == self->client->playerTeam )
	{//on same team
		return qfalse;
	}

	//LOS?

	return qtrue;
}

float WP_SaberRateEnemy( gentity_t *enemy, vec3_t center, vec3_t forward, float radius )
{
	float rating;
	vec3_t	dir;

	VectorSubtract( enemy->currentOrigin, center, dir );
	rating = (1.0f-(VectorNormalize( dir )/radius));
	rating *= DotProduct( forward, dir );
	return rating;
}

gentity_t *WP_SaberFindEnemy( gentity_t *self, gentity_t *saber )
{
//FIXME: should be a more intelligent way of doing this, like auto aim?
//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t	*ent, *bestEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		center, mins, maxs, fwdangles, forward;
	int			i, e;
	float		radius = 400;
	float		rating, bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	VectorCopy( saber->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	if ( WP_SaberValidateEnemy( self, self->enemy ) )
	{
		bestEnt = self->enemy;
		bestRating = WP_SaberRateEnemy( bestEnt, center, forward, radius );
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	if ( !numListedEntities )
	{//should we clear the enemy?
		return bestEnt;
	}

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if ( ent == self || ent == saber || ent == bestEnt )
		{
			continue;
		}
		if ( !WP_SaberValidateEnemy( self, ent ) )
		{//doesn't meet criteria of valid look enemy (don't check current since we would have done that before this func's call
			continue;
		}
		if ( !gi.inPVS( self->currentOrigin, ent->currentOrigin ) )
		{//not even potentially visible
			continue;
		}
		if ( !G_ClearLOS( self, self->client->renderInfo.eyePoint, ent ) )
		{//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		rating = WP_SaberRateEnemy( ent, center, forward, radius );
		if ( rating > bestRating )
		{
			bestEnt = ent;
			bestRating = rating;
		}
	}
	return bestEnt;
}

void WP_RunSaber( gentity_t *self, gentity_t *saber )
{
	vec3_t		origin, oldOrg;
	trace_t		tr;

	VectorCopy( saber->currentOrigin, oldOrg );
	// get current position
	EvaluateTrajectory( &saber->s.pos, level.time, origin );
	// get current angles
	EvaluateTrajectory( &saber->s.apos, level.time, saber->currentAngles );

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	int clipmask = saber->clipmask;
	if ( !self || !self->client || self->client->ps.saberLength <= 0 )
	{//don't keep hitting other sabers when turned off
		clipmask &= ~CONTENTS_LIGHTSABER;
	}
	gi.trace( &tr, saber->currentOrigin, saber->mins, saber->maxs, origin,
		saber->owner ? saber->owner->s.number : ENTITYNUM_NONE, clipmask, G2_NOCOLLIDE, 0 );

	VectorCopy( tr.endpos, saber->currentOrigin );

	if ( self->client->ps.saberActive )
	{
		if ( self->client->ps.saberInFlight || (self->client->ps.weaponTime&&!Q_irand( 0, 100 )) )
		{//make enemies run from a lit saber in flight or from me when I'm attacking
			if ( !Q_irand( 0, 10 ) )
			{//not so often...
				AddSightEvent( self, saber->currentOrigin, self->client->ps.saberLength*3, AEL_DANGER, 100 );
			}
		}
	}

	if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	gi.linkentity( saber );

	//touch push triggers?

	if ( tr.fraction != 1 )
	{
		WP_SaberImpact( self, saber, &tr );
	}

	if ( saber->s.pos.trType == TR_LINEAR )
	{//home
		//figure out where saber should be
		vec3_t	forward, saberHome, saberDest, fwdangles = {0};

		VectorCopy( self->client->ps.viewangles, fwdangles );
		if ( self->s.number )
		{
			fwdangles[0] -= 8;
		}
		else if ( cg.renderingThirdPerson )
		{
			fwdangles[0] -= 5;
		}

		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1
			|| self->client->ps.saberEntityState == SES_RETURNING
			|| VectorCompare( saber->s.pos.trDelta, vec3_origin ) )
		{//control if it's returning or just starting
			float	saberSpeed = 500;//FIXME: based on force level?
			float	dist;
			gentity_t *enemy = NULL;

			AngleVectors( fwdangles, forward, NULL, NULL );

			if ( self->client->ps.saberEntityDist < 100 )
			{//make the saber head to my hand- the bolt it was attached to
				VectorCopy( self->client->renderInfo.handRPoint, saberHome );
			}
			else
			{//aim saber from eyes
				VectorCopy( self->client->renderInfo.eyePoint, saberHome );
			}
			VectorMA( saberHome, self->client->ps.saberEntityDist, forward, saberDest );

			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 && self->client->ps.saberEntityState == SES_LEAVING )
			{//max level
				//pick an enemy
				enemy = WP_SaberFindEnemy( self, saber );
				if ( enemy )
				{//home in on enemy
					float enemyDist = Distance( self->client->renderInfo.handRPoint, enemy->currentOrigin );
					VectorCopy( enemy->currentOrigin, saberDest );
					saberDest[2] += enemy->maxs[2]/2.0f;//FIXME: when in a knockdown anim, the saber float above them... do we care?
					self->client->ps.saberEntityDist = enemyDist;
				}
			}


			//Make the saber head there
			VectorSubtract( saberDest, saber->currentOrigin, saber->s.pos.trDelta );
			dist = VectorNormalize( saber->s.pos.trDelta );
			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 && self->client->ps.saberEntityState == SES_LEAVING && !enemy )
			{
				if ( dist < 200 )
				{
					saberSpeed = 400 - (dist*2);
				}
			}
			else if ( self->client->ps.saberEntityState == SES_LEAVING && dist < 50 )
			{
				saberSpeed = dist * 2 + 30;
				if ( (enemy && dist > enemy->maxs[0]) || (!enemy && dist > 24) )
				{//auto-tracking an enemy and we can't hit him
					if ( saberSpeed < 120 )
					{//clamp to a minimum speed
						saberSpeed = 120;
					}
				}
			}
			/*
			if ( self->client->ps.saberEntityState == SES_RETURNING )
			{//FIXME: if returning, move faster?
				saberSpeed = 800;
				if ( dist < 200 )
				{
					saberSpeed -= 400 - (dist*2);
				}
			}
			*/
			VectorScale( saber->s.pos.trDelta, saberSpeed, saber->s.pos.trDelta );
			//SnapVector( saber->s.pos.trDelta );	// save net bandwidth
			VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}
		else
		{
			VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}

		//if it's heading back, point it's base at us
		if ( self->client->ps.saberEntityState == SES_RETURNING )
		{
			fwdangles[0] += SABER_PITCH_HACK;
			VectorCopy( fwdangles, saber->s.apos.trBase );
			saber->s.apos.trTime = level.time;
			saber->s.apos.trType = TR_INTERPOLATE;
			VectorClear( saber->s.apos.trDelta );
		}
	}
}


qboolean WP_SaberLaunch( gentity_t *self, gentity_t *saber, qboolean thrown )
{//FIXME: probably need a debounce time
	vec3_t	saberMins={-3.0f,-3.0f,-3.0f};
	vec3_t	saberMaxs={3.0f,3.0f,3.0f};
	trace_t	trace;

	if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
	{
		if ( !WP_ForcePowerUsable( self, FP_SABERTHROW, 20 ) )
		{
			return qfalse;
		}
	}
	else
	{
		if ( !WP_ForcePowerUsable( self, FP_SABERTHROW, 0 ) )
		{
			return qfalse;
		}
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't saber throw when zoomed in or in cinematic
		return qfalse;
	}
	//make sure it won't start in solid
	gi.trace( &trace, self->client->renderInfo.handRPoint, saberMins, saberMaxs, self->client->renderInfo.handRPoint, saber->s.number, MASK_SOLID, G2_NOCOLLIDE, 0 );
	if ( trace.startsolid || trace.allsolid )
	{
		return qfalse;
	}
	//make sure I'm not throwing it on the other side of a door or wall or whatever
	gi.trace( &trace, self->currentOrigin, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, G2_NOCOLLIDE, 0 );
	if ( trace.startsolid || trace.allsolid || trace.fraction < 1.0f )
	{
		return qfalse;
	}

	if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
	{//at max skill, the cost increases as keep it out
		WP_ForcePowerStart( self, FP_SABERTHROW, 10 );
	}
	else
	{
		WP_ForcePowerStart( self, FP_SABERTHROW, 0 );
	}
	//draw it
	saber->s.eFlags &= ~EF_NODRAW;
	saber->svFlags |= SVF_BROADCAST;
	saber->svFlags &= ~SVF_NOCLIENT;

	//place it
	VectorCopy( self->client->renderInfo.handRPoint, saber->currentOrigin );//muzzlePoint
	VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
	saber->s.pos.trTime = level.time;
	saber->s.pos.trType = TR_LINEAR;
	VectorClear( saber->s.pos.trDelta );
	gi.linkentity( saber );

	//spin it
	VectorClear( saber->s.apos.trBase );
	saber->s.apos.trTime = level.time;
	saber->s.apos.trType = TR_LINEAR;
	if ( self->health > 0 && thrown )
	{//throwing it
		saber->s.apos.trBase[1] = self->client->ps.viewangles[1];
		saber->s.apos.trBase[0] = SABER_PITCH_HACK;
	}
	else
	{//dropping it
		vectoangles( self->client->renderInfo.muzzleDir, saber->s.apos.trBase );
	}
	VectorClear( saber->s.apos.trDelta );

	switch ( self->client->ps.forcePowerLevel[FP_SABERTHROW] )
	{//FIXME: make a table?
	default:
	case FORCE_LEVEL_1:
		saber->s.apos.trDelta[1] = 600;
		break;
	case FORCE_LEVEL_2:
		saber->s.apos.trDelta[1] = 800;
		break;
	case FORCE_LEVEL_3:
		saber->s.apos.trDelta[1] = 1200;
		break;
	}

	//Take it out of my hand
	self->client->ps.saberInFlight = qtrue;
	self->client->ps.saberEntityState = SES_LEAVING;
	self->client->ps.saberEntityDist = saberThrowDist[self->client->ps.forcePowerLevel[FP_SABERTHROW]];
	self->client->ps.saberThrowTime = level.time;
	//if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
	{
		self->client->ps.forcePowerDebounce[FP_SABERTHROW] = level.time + 1000;//so we can keep it out for a minimum amount of time
	}

	//if it's not active, turn it on
	self->client->ps.saberActive = qtrue;

	//turn on the saber trail
	self->client->saberTrail.inAction = qtrue;
	self->client->saberTrail.duration = 150;

	//reset the mins
	VectorCopy( saberMins, saber->mins );
	VectorCopy( saberMaxs, saber->maxs );
	saber->contents = 0;//CONTENTS_LIGHTSABER;
	saber->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;

	// remove the ghoul2 sabre model on the player
	if ( self->weaponModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model(self->ghoul2, self->weaponModel);
		self->weaponModel = -1;
	}

	return qtrue;
}

qboolean WP_SaberLose( gentity_t *self, vec3_t throwDir )
{
	if ( !self || !self->client || self->client->ps.saberEntityNum <= 0 )
	{//WTF?!!  We lost it already?
		return qfalse;
	}
	gentity_t *dropped = &g_entities[self->client->ps.saberEntityNum];
	if ( !self->client->ps.saberInFlight )
	{//not alreay in air
		//make it so we can throw it
		self->client->ps.forcePowersKnown |= (1<<FP_SABERTHROW);
		self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_1;
		//throw it
		if ( !WP_SaberLaunch( self, dropped, qfalse ) )
		{//couldn't throw it
			return qfalse;
		}
	}
	if ( self->client->ps.saberActive )
	{//on
		//drop it instantly
		WP_SaberDrop( self, dropped );
	}
	//optionally give it some thrown velocity
	if ( throwDir && !VectorCompare( throwDir, vec3_origin ) )
	{
		VectorCopy( throwDir, dropped->s.pos.trDelta );
	}
	//don't pull it back on the next frame
	if ( self->NPC )
	{
		self->NPC->last_ucmd.buttons &= ~BUTTON_ATTACK;
	}
	return qtrue;
}

void WP_SaberCatch( gentity_t *self, gentity_t *saber, qboolean switchToSaber )
{//FIXME: probably need a debounce time
	if ( self->health > 0 && !PM_SaberInBrokenParry( self->client->ps.saberMove ) && self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{
		//don't draw it
		saber->s.eFlags |= EF_NODRAW;
		saber->svFlags &= SVF_BROADCAST;
		saber->svFlags |= SVF_NOCLIENT;

		//take off any gravity stuff if we'd dropped it
		saber->s.pos.trType = TR_LINEAR;
		saber->s.eFlags &= ~EF_BOUNCE_HALF;

		//Put it in my hand
		self->client->ps.saberInFlight = qfalse;
		self->client->ps.saberEntityState = SES_LEAVING;

		//turn off the saber trail
		self->client->saberTrail.inAction = qfalse;
		self->client->saberTrail.duration = 75;

		//reset its contents/clipmask
		saber->contents = CONTENTS_LIGHTSABER;// | CONTENTS_SHOTCLIP;
		saber->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		//play catch sound
		G_Sound( saber, G_SoundIndex( "sound/weapons/saber/saber_catch.wav" ) );
		//FIXME: if an NPC, don't turn it back on if no enemy or enemy is dead...
		//if it's not our current weapon, make it our current weapon
		if ( self->client->ps.weapon == WP_SABER )
		{
			G_CreateG2AttachedWeaponModel( self, self->client->ps.saberModel );
		}
		if ( switchToSaber )
		{
			if ( self->client->ps.weapon != WP_SABER )
			{
				CG_ChangeWeapon( WP_SABER );
			}
			else
			{//if it's not active, turn it on
				self->client->ps.saberActive = qtrue;
			}
		}
	}
}


void WP_SaberReturn( gentity_t *self, gentity_t *saber )
{
	if ( PM_SaberInBrokenParry( self->client->ps.saberMove ) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
	{
		return;
	}
	if ( self && self->client )
	{//still alive and stuff
		//FIXME: when it's returning, flies butt first, but seems to do a lot of damage when going through people... hmm...
		self->client->ps.saberEntityState = SES_RETURNING;
		//turn down the saber trail
		self->client->saberTrail.inAction = qfalse;
		self->client->saberTrail.duration = 75;
	}
	if ( !(saber->s.eFlags&EF_BOUNCE) )
	{
		saber->s.eFlags |= EF_BOUNCE;
		saber->bounceCount = 300;
	}
}


void WP_SaberDrop( gentity_t *self, gentity_t *saber )
{
	saber->s.eFlags &= ~EF_BOUNCE;
	saber->bounceCount = 0;
	//make it fall
	saber->s.pos.trType = TR_GRAVITY;
	//make it bounce some
	saber->s.eFlags |= EF_BOUNCE_HALF;
	//make it spin
	VectorCopy( saber->currentAngles, saber->s.apos.trBase );
	saber->s.apos.trType = TR_LINEAR;
	saber->s.apos.trTime = level.time;
	VectorSet( saber->s.apos.trDelta, Q_irand( -300, 300 ), saber->s.apos.trDelta[1], Q_irand( -300, 300 ) );
	if ( !saber->s.apos.trDelta[1] )
	{
		saber->s.apos.trDelta[1] = Q_irand( -300, 300 );
	}
	//force it to be ready to return
	self->client->ps.saberEntityDist = 0;
	self->client->ps.saberEntityState = SES_RETURNING;
	//turn it off
	self->client->ps.saberActive = qfalse;
	//turn off the saber trail
	self->client->saberTrail.inAction = qfalse;
	self->client->saberTrail.duration = 75;
	//play the saber turning off sound
	if ( self->client->playerTeam == TEAM_PLAYER )
	{
		G_SoundOnEnt( saber, CHAN_AUTO, "sound/weapons/saber/saberoff.wav" );
	}
	else
	{
		G_SoundOnEnt( saber, CHAN_AUTO, "sound/weapons/saber/enemy_saber_off.wav" );
	}

	if ( self->health <= 0 )
	{//owner is dead!
		saber->s.time = level.time;//will make us free ourselves after a time
	}
}


void WP_SaberPull( gentity_t *self, gentity_t *saber )
{
	if ( PM_SaberInBrokenParry( self->client->ps.saberMove ) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
	{
		return;
	}
	if ( self->health > 0 )
	{
		//take off gravity
		saber->s.pos.trType = TR_LINEAR;
		//take off bounce
		saber->s.eFlags &= EF_BOUNCE_HALF;
		//play sound
		G_Sound( self, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
	}
}


// Check if we are throwing it, launch it if needed, update position if needed.
void WP_SaberThrow( gentity_t *self, usercmd_t *ucmd )
{
	vec3_t			saberDiff;
	trace_t			tr;
	//static float	SABER_SPEED = 10;

	gentity_t *saberent;

	if ( self->client->ps.saberEntityNum <= 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
	{//WTF?!!  We lost it?
		return;
	}

	saberent = &g_entities[self->client->ps.saberEntityNum];

	VectorSubtract( self->client->renderInfo.handRPoint, saberent->currentOrigin, saberDiff );

	//is our saber in flight?
	if ( !self->client->ps.saberInFlight )
	{//saber is not in flight right now
		if ( self->client->ps.weapon != WP_SABER )
		{//don't even have it out
			return;
		}
		else if ( ucmd->buttons & BUTTON_ALT_ATTACK && !(self->client->ps.pm_flags&PMF_ALT_ATTACK_HELD) )
		{//still holding it, not still holding attack from a previous throw, so throw it.
			if ( !(self->client->ps.saberEventFlags&SEF_INWATER) && WP_SaberLaunch( self, saberent, qtrue ) )
			{
				if ( self->client && !self->s.number )
				{
					self->client->sess.missionStats.saberThrownCnt++;
				}
				//need to recalc this because we just moved it
				VectorSubtract( self->client->renderInfo.handRPoint, saberent->currentOrigin, saberDiff );
			}
			else
			{//couldn't throw it
				return;
			}
		}
		else
		{//holding it, don't want to throw it, go away.
			return;
		}
	}
	else
	{//inflight
		//is our saber currently on it's way back to us?
		if ( self->client->ps.saberEntityState == SES_RETURNING )
		{//see if we're close enough to pick it up
			if ( VectorLengthSquared( saberDiff ) <= 256 )//16 squared//G_BoundsOverlap( self->absmin, self->absmax, saberent->absmin, saberent->absmax ) )//
			{//caught it
				vec3_t	axisPoint;
				trace_t	trace;
				VectorCopy( self->currentOrigin, axisPoint );
				axisPoint[2] = self->client->renderInfo.handRPoint[2];
				gi.trace( &trace, axisPoint, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, G2_NOCOLLIDE, 0 );
				if ( !trace.startsolid && trace.fraction >= 1.0f )
				{//our hand isn't through a wall
					WP_SaberCatch( self, saberent, qtrue );
					NPC_SetAnim( self, SETANIM_TORSO, TORSO_HANDRETRACT1, SETANIM_FLAG_OVERRIDE );
				}
				return;
			}
		}

		if ( saberent->s.pos.trType != TR_STATIONARY )
		{//saber is in flight, lerp it
			WP_RunSaber( self, saberent );
		}
		else
		{//it fell on the ground
			if ( self->health <= 0 && level.time > saberent->s.time + 5000 )
			{//make us free ourselves after a time
				G_FreeEntity( saberent );
				self->client->ps.saberEntityNum = ENTITYNUM_NONE;
				return;
			}
			if ( (!self->s.number && level.time - saberent->aimDebounceTime > 15000)
				|| (self->s.number && level.time - saberent->aimDebounceTime > 5000) )
			{//(only for player) been missing for 15 seconds, automagicially return
				WP_SaberCatch( self, saberent, qfalse );
				return;
			}
		}
	}

	//are we still trying to use the saber?
	if ( self->client->ps.weapon != WP_SABER )
	{//switched away
		if ( !self->client->ps.saberInFlight )
		{//wasn't throwing saber
			return;
		}
		else if ( saberent->s.pos.trType == TR_LINEAR )
		{//switched away while controlling it, just drop the saber
			WP_SaberDrop( self, saberent );
			return;
		}
		else
		{//it's on the ground, see if it's inside us (touching)
			if ( G_PointInBounds( saberent->currentOrigin, self->absmin, self->absmax ) )
			{//it's in us, pick it up automatically
				WP_SaberPull( self, saberent );
			}
		}
	}
	else if ( saberent->s.pos.trType != TR_LINEAR )
	{//weapon is saber and not flying
		if ( self->client->ps.saberInFlight )
		{//we dropped it
			if ( ucmd->buttons & BUTTON_ATTACK )//|| self->client->ps.weaponstate == WEAPON_RAISING )//ucmd->buttons & BUTTON_ALT_ATTACK ||
			{//we actively want to pick it up or we just switched to it, so pull it back
				gi.trace( &tr, saberent->currentOrigin, saberent->mins, saberent->maxs, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, G2_NOCOLLIDE, 0 );

				if ( tr.allsolid || tr.startsolid || tr.fraction < 1.0f )
				{//can't pick it up yet, no LOS
					return;
				}
				//clear LOS, pick it up
				WP_SaberPull( self, saberent );
			}
			else
			{//see if it's inside us (touching)
				if ( G_PointInBounds( saberent->currentOrigin, self->absmin, self->absmax ) )
				{//it's in us, pick it up automatically
					WP_SaberPull( self, saberent );
				}
			}
		}
	}
	else if ( self->health <= 0 && self->client->ps.saberInFlight )
	{//we died, drop it
		WP_SaberDrop( self, saberent );
		return;
	}
	else if ( !self->client->ps.saberActive && self->client->ps.saberEntityState != SES_RETURNING )
	{//we turned it off, drop it
		WP_SaberDrop( self, saberent );
		return;
	}

	//TODO: if deactivate saber in flight, should it drop?

	if ( saberent->s.pos.trType != TR_LINEAR )
	{//don't home
		return;
	}

	float saberDist = VectorLength( saberDiff );
	if ( self->client->ps.saberEntityState == SES_LEAVING )
	{//saber still flying forward
		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
		{//still holding it out
			if ( !(ucmd->buttons&BUTTON_ALT_ATTACK) && self->client->ps.forcePowerDebounce[FP_SABERTHROW] < level.time )
			{//done throwing, return to me
				if ( self->client->ps.saberActive )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
			else if ( level.time - self->client->ps.saberThrowTime >= 100 )
			{
				if ( WP_ForcePowerAvailable( self, FP_SABERTHROW, 1 ) )
				{
					WP_ForcePowerDrain( self, FP_SABERTHROW, 1 );
					self->client->ps.saberThrowTime = level.time;
				}
				else
				{//out of force power, return to me
					WP_SaberReturn( self, saberent );
				}
			}
		}
		else
		{
			if ( !(ucmd->buttons&BUTTON_ALT_ATTACK) && self->client->ps.forcePowerDebounce[FP_SABERTHROW] < level.time )
			{//not holding button and has been out at least 1 second, return to me
				if ( self->client->ps.saberActive )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
			else if ( level.time - self->client->ps.saberThrowTime > 3000
				|| (self->client->ps.forcePowerLevel[FP_SABERTHROW]==FORCE_LEVEL_1&&saberDist>=self->client->ps.saberEntityDist) )
			{//been out too long, or saber throw 1 went too far, return to me
				if ( self->client->ps.saberActive )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
		}
	}
	if ( self->client->ps.saberEntityState == SES_RETURNING )
	{
		if ( self->client->ps.saberEntityDist > 0 )
		{
			self->client->ps.saberEntityDist -= 25;
		}
		if ( self->client->ps.saberEntityDist < 0 )
		{
			self->client->ps.saberEntityDist = 0;
		}
		else if ( saberDist < self->client->ps.saberEntityDist )
		{//if it's coming back to me, never push it away
			self->client->ps.saberEntityDist = saberDist;
		}
	}
}


//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
int WP_MissileBlockForBlock( int saberBlock )
{
	switch( saberBlock )
	{
	case BLOCKED_UPPER_RIGHT:
		return BLOCKED_UPPER_RIGHT_PROJ;
		break;
	case BLOCKED_UPPER_LEFT:
		return BLOCKED_UPPER_LEFT_PROJ;
		break;
	case BLOCKED_LOWER_RIGHT:
		return BLOCKED_LOWER_RIGHT_PROJ;
		break;
	case BLOCKED_LOWER_LEFT:
		return BLOCKED_LOWER_LEFT_PROJ;
		break;
	case BLOCKED_TOP:
		return BLOCKED_TOP_PROJ;
		break;
	}
	return saberBlock;
}

void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock )
{
	vec3_t diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;

	if ( self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING )
	{//don't block while changing weapons
		return;
	}
	//NPCs don't auto-block
	if ( !missileBlock && self->s.number != 0 && self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		return;
	}

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	VectorNormalize( diff );

	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	//FIXME: take torsoAngles into account?
	if ( zdiff > -5 )//0 )//40 )
	{
		if ( rightdot > 0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else if ( zdiff > -22 )//-20 )//20 )
	{
		if ( zdiff < -10 )//30 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			//NPC should duck, but NPC should never get here
		}
		if ( rightdot > 0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{//FIXME: this looks really weird if the shot is too low!
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else
	{
		if ( rightdot >= 0 )
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		if ( !self->s.number )
		{
			gi.Printf( "EyeZ: %4.2f  HitZ: %4.2f  zdiff: %4.2f  rdot: %4.2f\n", self->client->renderInfo.eyePoint[2], hitloc[2], zdiff, rightdot );
			switch ( self->client->ps.saberBlocked )
			{
			case BLOCKED_TOP:
				gi.Printf( "BLOCKED_TOP\n" );
				break;
			case BLOCKED_UPPER_RIGHT:
				gi.Printf( "BLOCKED_UPPER_RIGHT\n" );
				break;
			case BLOCKED_UPPER_LEFT:
				gi.Printf( "BLOCKED_UPPER_LEFT\n" );
				break;
			case BLOCKED_LOWER_RIGHT:
				gi.Printf( "BLOCKED_LOWER_RIGHT\n" );
				break;
			case BLOCKED_LOWER_LEFT:
				gi.Printf( "BLOCKED_LOWER_LEFT\n" );
				break;
			default:
				break;
			}
		}
	}
#endif

	if ( missileBlock )
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
	}

	if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		int parryReCalcTime = Jedi_ReCalcParryTime( self, EVASION_PARRY );
		if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
}

void WP_SaberBlock( gentity_t *saber, vec3_t hitloc, qboolean missileBlock )
{
	gentity_t *playerent;
	vec3_t diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;

	if (saber && saber->owner)
	{
		playerent = saber->owner;
		if (!playerent->client)
		{
			return;
		}
		if ( playerent->client->ps.weaponstate == WEAPON_DROPPING ||
			playerent->client->ps.weaponstate == WEAPON_RAISING )
		{//don't block while changing weapons
			return;
		}
	}
	else
	{	// Bad entity passed.
		return;
	}

	//temporarily disabling auto-blocking for NPCs...
	if ( !missileBlock && playerent->s.number != 0 && playerent->client->ps.saberBlocked != BLOCKED_NONE )
	{
		return;
	}

	VectorSubtract(hitloc, playerent->currentOrigin, diff);
	VectorNormalize(diff);

	fwdangles[1] = playerent->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff) + Q_flrand(-0.2f,0.2f);
	zdiff = hitloc[2] - playerent->currentOrigin[2] + Q_irand(-8,8);

	// Figure out what quadrant the block was in.
	if (zdiff > 24)
	{	// Attack from above
		if (Q_irand(0,1))
		{
			playerent->client->ps.saberBlocked = BLOCKED_TOP;
		}
		else
		{
			playerent->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
	}
	else if (zdiff > 13)
	{	// The upper half has three viable blocks...
		if (rightdot > 0.25)
		{	// In the right quadrant...
			if (Q_irand(0,1))
			{
				playerent->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
			}
			else
			{
				playerent->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
			}
		}
		else
		{
			switch(Q_irand(0,3))
			{
			case 0:
				playerent->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
				break;
			case 1:
			case 2:
				playerent->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
				break;
			case 3:
				playerent->client->ps.saberBlocked = BLOCKED_TOP;
				break;
			}
		}
	}
	else
	{	// The lower half is a bit iffy as far as block coverage.  Pick one of the "low" ones at random.
		if (Q_irand(0,1))
		{
			playerent->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			playerent->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

	if ( missileBlock )
	{
		playerent->client->ps.saberBlocked = WP_MissileBlockForBlock( playerent->client->ps.saberBlocked );
	}
}

void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd  )
{
	float		dist;
	gentity_t	*ent, *incoming = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e;
	float		closestDist, radius = 256;
	vec3_t		forward, dir, missile_dir, fwdangles = {0};
	trace_t		trace;
	vec3_t		traceTo, entDir;


	if ( self->client->ps.weapon != WP_SABER )
	{
		return;
	}

	if ( self->client->ps.saberInFlight )
	{
		return;
	}

	if ( self->client->ps.forcePowersActive&(1<<FP_LIGHTNING) )
	{//can't block while zapping
		return;
	}

	if ( self->client->ps.forcePowersActive&(1<<FP_PUSH) )
	{//can't block while shoving
		return;
	}

	if ( self->client->ps.forcePowersActive&(1<<FP_GRIP) )
	{//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		return;
	}

	if ( self->health <= 0 )
	{//dead don't try to block (NOTE: actual deflection happens in missile code)
		return;
	}

	if ( PM_InKnockDown( &self->client->ps ) )
	{//can't block when knocked down
		return;
	}

	if ( !self->client->ps.saberLength )
	{
		if ( self->s.number == 0 )
		{//player doesn't auto-activate
			return;
		}
	}

	if ( !self->s.number )
	{//don't do this if already attacking!
		if ( ucmd->buttons & BUTTON_ATTACK
			|| PM_SaberInAttack( self->client->ps.saberMove )
			|| PM_SaberInSpecialAttack( self->client->ps.torsoAnim )
			|| PM_SaberInTransitionAny( self->client->ps.saberMove ))
		{
			return;
		}
	}

	if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
		return;
	}

	if ( !self->s.number && !g_saberAutoBlocking->integer && self->client->ps.saberBlockingTime<level.time )
	{
		return;
	}

	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = self->currentOrigin[i] - radius;
		maxs[i] = self->currentOrigin[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	closestDist = radius;

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE && !(ent->s.eFlags&EF_MISSILE_STICK) )
		{//not a normal projectile
			if ( ent->client || ent->s.weapon != WP_SABER )
			{//FIXME: wake up bad guys?
				continue;
			}
			if ( ent->s.eFlags & EF_NODRAW )
			{
				continue;
			}
			if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
			{//not a lightsaber
				//FIXME: what about general objects that are small in size- like rocks, etc...
				continue;
			}
			//a lightsaber.. make sure it's on and inFlight
			if ( !ent->owner || !ent->owner->client )
			{
				continue;
			}
			if ( !ent->owner->client->ps.saberInFlight )
			{//not in flight
				continue;
			}
			if ( ent->owner->client->ps.saberLength <= 0 )
			{//not on
				continue;
			}
			if ( ent->owner->health <= 0 && !g_saberRealisticCombat->integer )
			{//it's not doing damage, so ignore it
				continue;
			}
		}
		else
		{
			if ( ent->s.pos.trType == TR_STATIONARY && !self->s.number )
			{//nothing you can do with a stationary missile if you're the player
				continue;
			}
		}

		float		dot1, dot2;
		//see if they're in front of me
		VectorSubtract( ent->currentOrigin, self->currentOrigin, dir );
		dist = VectorNormalize( dir );
		//FIXME: handle detpacks, proximity mines and tripmines
		if ( ent->s.weapon == WP_THERMAL )
		{//thermal detonator!
			if ( self->NPC && dist < ent->splashRadius )
			{
				if ( dist < ent->splashRadius &&
					ent->nextthink < level.time + 600 &&
					ent->count &&
					self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
						(ent->s.pos.trType == TR_STATIONARY||
						ent->s.pos.trType == TR_INTERPOLATE||
						(dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE||
						!WP_ForcePowerUsable( self, FP_PUSH, 0 )) )
				{//TD is close enough to hurt me, I'm on the ground and the thing is at rest or behind me and about to blow up, or I don't have force-push so force-jump!
					//FIXME: sometimes this might make me just jump into it...?
					self->client->ps.forceJumpCharge = 480;
				}
				else
				{//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
					ForceThrow( self, qfalse );
				}
			}
			continue;
		}
		else if ( ent->splashDamage && ent->splashRadius )
		{//exploding missile
			//FIXME: handle tripmines and detpacks somehow...
			//			maybe do a force-gesture that makes them explode?
			//			But what if we're within it's splashradius?
			if ( !self->s.number )
			{//players don't auto-handle these at all
				continue;
			}
			else
			{
				if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
				{//a placed explosive like a tripmine or detpack
					if ( InFOV( ent->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 90, 90 ) )
					{//in front of me
						if ( G_ClearLOS( self, ent ) )
						{//can see it
							vec3_t throwDir;
							//make the gesture
							ForceThrow( self, qfalse );
							//take it off the wall and toss it
							ent->s.pos.trType = TR_GRAVITY;
							ent->s.eType = ET_MISSILE;
							ent->s.eFlags &= ~EF_MISSILE_STICK;
							ent->s.eFlags |= EF_BOUNCE_HALF;
							AngleVectors( ent->currentAngles, throwDir, NULL, NULL );
							VectorMA( ent->currentOrigin, ent->maxs[0]+4, throwDir, ent->currentOrigin );
							VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
							VectorScale( throwDir, 300, ent->s.pos.trDelta );
							ent->s.pos.trDelta[2] += 150;
							VectorMA( ent->s.pos.trDelta, 800, dir, ent->s.pos.trDelta );
							ent->s.pos.trTime = level.time;		// move a bit on the very first frame
							VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
							ent->owner = self;
							// make it explode, but with less damage
							ent->splashDamage /= 3;
							ent->splashRadius /= 3;
							ent->e_ThinkFunc = thinkF_WP_Explode;
							ent->nextthink = level.time + Q_irand( 500, 3000 );
						}
					}
				}
				else if ( dist < ent->splashRadius &&
				self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
					(DotProduct( dir, forward ) < SABER_REFLECT_MISSILE_CONE||
					!WP_ForcePowerUsable( self, FP_PUSH, 0 )) )
				{//NPCs try to evade it
					self->client->ps.forceJumpCharge = 480;
				}
				else
				{//else, try to force-throw it away
					//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
					ForceThrow( self, qfalse );
				}
			}
			//otherwise, can't block it, so we're screwed
			continue;
		}

		if ( ent->s.weapon != WP_SABER )
		{//only block shots coming from behind
			if ( (dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE )
				continue;
		}
		else if ( !self->s.number )
		{//player never auto-blocks thrown sabers
			continue;
		}//NPCs always try to block sabers coming from behind!

		//see if they're heading towards me
		VectorCopy( ent->s.pos.trDelta, missile_dir );
		VectorNormalize( missile_dir );
		if ( (dot2 = DotProduct( dir, missile_dir )) > 0 )
			continue;

		//FIXME: must have a clear trace to me, too...
		if ( dist < closestDist )
		{
			VectorCopy( self->currentOrigin, traceTo );
			traceTo[2] = self->absmax[2] - 4;
			gi.trace( &trace, ent->currentOrigin, ent->mins, ent->maxs, traceTo, ent->s.number, ent->clipmask, G2_NOCOLLIDE, 0 );
			if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
			{//okay, try one more check
				VectorNormalize2( ent->s.pos.trDelta, entDir );
				VectorMA( ent->currentOrigin, radius, entDir, traceTo );
				gi.trace( &trace, ent->currentOrigin, ent->mins, ent->maxs, traceTo, ent->s.number, ent->clipmask, G2_NOCOLLIDE, 0 );
				if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
				{//can't hit me, ignore it
					continue;
				}
			}
			if ( self->s.number != 0 )
			{//An NPC
				if ( self->NPC && !self->enemy && ent->owner )
				{
					if ( ent->owner->health >= 0 && (!ent->owner->client || ent->owner->client->playerTeam != self->client->playerTeam) )
					{
						G_SetEnemy( self, ent->owner );
					}
				}
			}
			//FIXME: if NPC, predict the intersection between my current velocity/path and the missile's, see if it intersects my bounding box (+/-saberLength?), don't try to deflect unless it does?
			closestDist = dist;
			incoming = ent;
		}
	}

	if ( incoming )
	{
		if ( self->NPC && !G_ControlledByPlayer( self ) )
		{
			if ( Jedi_WaitingAmbush( self ) )
			{
				Jedi_Ambush( self );
			}
			if ( Jedi_SaberBlockGo( self, &self->NPC->last_ucmd, NULL, NULL, incoming ) != EVASION_NONE )
			{//make sure to turn on your saber if it's not on
				self->client->ps.saberActive = qtrue;
			}
		}
		else//player
		{
			WP_SaberBlockNonRandom( self, incoming->currentOrigin, qtrue );
			if ( incoming->owner && incoming->owner->client && (!self->enemy || self->enemy->s.weapon != WP_SABER) )//keep enemy jedi over shooters
			{
				self->enemy = incoming->owner;
				NPC_SetLookTarget( self, incoming->owner->s.number, level.time+1000 );
			}
		}
	}
}


//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================


void WP_SetSaberMove(gentity_t *self, short blocked)
{
	self->client->ps.saberBlocked = blocked;
}

extern void CG_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha );
void WP_SaberUpdate( gentity_t *self, usercmd_t *ucmd )
{
	//float	swap;
	float	minsize = 16;

	if(0)//	if ( self->s.number != 0 )
	{//for now only the player can do this		// not anymore
		return;
	}

	if ( !self->client )
	{
		return;
	}

	if ( self->client->ps.saberEntityNum < 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
	{//never got one
		return;
	}

	// Check if we are throwing it, launch it if needed, update position if needed.
	WP_SaberThrow(self, ucmd);


	//vec3_t saberloc;
	//vec3_t sabermins={-8,-8,-8}, sabermaxs={8,8,8};

	gentity_t *saberent;

	if (self->client->ps.saberEntityNum <= 0)
	{//WTF?!!  We lost it?
		return;
	}

	saberent = &g_entities[self->client->ps.saberEntityNum];

	//FIXME: Based on difficulty level/jedi saber combat skill, make this bounding box fatter/smaller
	if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{//we're blocking, increase min size
		minsize = 32;
	}

	//is our saber in flight?
	if ( !self->client->ps.saberInFlight )
	{	// It isn't, which means we can update its position as we will.
		if ( !self->client->ps.saberActive || PM_InKnockDown( &self->client->ps ) )
		{//can't block if saber isn't on
			VectorClear(saberent->mins);
			VectorClear(saberent->maxs);
			G_SetOrigin(saberent, self->currentOrigin);
		}
		else if ( self->client->ps.saberBlocking == BLK_TIGHT || self->client->ps.saberBlocking == BLK_WIDE )
		{//FIXME: keep bbox in front of player, even when wide?
			vec3_t	saberOrg;
			if ( ( (self->s.number&&!Jedi_SaberBusy(self)&&!g_saberRealisticCombat->integer) || (self->s.number == 0 && self->client->ps.saberBlocking == BLK_WIDE && (g_saberAutoBlocking->integer||self->client->ps.saberBlockingTime>level.time)) )
				&& self->client->ps.weaponTime <= 0 )
			{//full-size blocking for non-attacking player with g_saberAutoBlocking on
				vec3_t saberang={0,0,0}, fwd, sabermins={-8,-8,-8}, sabermaxs={8,8,8};

				saberang[YAW] = self->client->ps.viewangles[YAW];
				AngleVectors( saberang, fwd, NULL, NULL );

				VectorMA( self->currentOrigin, 12, fwd, saberOrg );

				VectorAdd( self->mins, sabermins, saberent->mins );
				VectorAdd( self->maxs, sabermaxs, saberent->maxs );

				saberent->contents = CONTENTS_LIGHTSABER;

				G_SetOrigin( saberent, saberOrg );
			}
			else
			{
				vec3_t	saberTip;
				VectorMA( self->client->renderInfo.muzzlePoint, self->client->ps.saberLength, self->client->renderInfo.muzzleDir, saberTip );
				VectorMA( self->client->renderInfo.muzzlePoint, self->client->ps.saberLength*0.5, self->client->renderInfo.muzzleDir, saberOrg );
				for ( int i = 0; i < 3; i++ )
				{
					if ( saberTip[i] > self->client->renderInfo.muzzlePoint[i] )
					{
						saberent->maxs[i] = saberTip[i] - saberOrg[i] + 8;//self->client->renderInfo.muzzlePoint[i];
						saberent->mins[i] = self->client->renderInfo.muzzlePoint[i] - saberOrg[i] - 8;
					}
					else //if ( saberTip[i] < self->client->renderInfo.muzzlePoint[i] )
					{
						saberent->maxs[i] = self->client->renderInfo.muzzlePoint[i] - saberOrg[i] + 8;
						saberent->mins[i] = saberTip[i] - saberOrg[i] - 8;//self->client->renderInfo.muzzlePoint[i];
					}
					if ( self->client->ps.weaponTime > 0 || self->s.number || g_saberAutoBlocking->integer || self->client->ps.saberBlockingTime > level.time )
					{//if attacking or blocking (or an NPC), inflate to a minimum size
						if ( saberent->maxs[i] < minsize )
						{
							saberent->maxs[i] = minsize;
						}
						if ( saberent->mins[i] > -minsize )
						{
							saberent->mins[i] = -minsize;
						}
					}
				}
				saberent->contents = CONTENTS_LIGHTSABER;
				G_SetOrigin( saberent, saberOrg );
			}
		}
		/*
		else if (self->client->ps.saberBlocking == BLK_WIDE)
		{	// Assuming that we are not swinging, the saber's bounding box should be around the player.
			vec3_t saberang={0,0,0}, fwd;

			saberang[YAW] = self->client->ps.viewangles[YAW];
			AngleVectors( saberang, fwd, NULL, NULL );

			VectorMA(self->currentOrigin, 12, fwd, saberloc);

			VectorAdd(self->mins, sabermins, saberent->mins);
			VectorAdd(self->maxs, sabermaxs, saberent->maxs);

			saberent->contents = CONTENTS_LIGHTSABER;

			G_SetOrigin( saberent, saberloc);
		}
		else if (self->client->ps.saberBlocking == BLK_TIGHT)
		{	// If the player is swinging, the bbox is around just the saber
			VectorCopy(self->client->renderInfo.muzzlePoint, sabermins);
			// Put the limits of the bbox around the saber size.
			VectorMA(sabermins, self->client->ps.saberLength, self->client->renderInfo.muzzleDir, sabermaxs);

			// Now make the mins into mins and the maxs into maxs
			if (sabermins[0] > sabermaxs[0])
			{
				swap = sabermins[0];
				sabermins[0] = sabermaxs[0];
				sabermaxs[0] = swap;
			}
			if (sabermins[1] > sabermaxs[1])
			{
				swap = sabermins[1];
				sabermins[1] = sabermaxs[1];
				sabermaxs[1] = swap;
			}
			if (sabermins[2] > sabermaxs[2])
			{
				swap = sabermins[2];
				sabermins[2] = sabermaxs[2];
				sabermaxs[2] = swap;
			}

			// Now the loc is halfway between the (absolute) mins and maxs
			VectorAdd(sabermins, sabermaxs, saberloc);
			VectorScale(saberloc, 0.5, saberloc);

			// Finally, turn the mins and maxs, which are absolute, into relative mins and maxs.
			VectorSubtract(sabermins, saberloc, saberent->mins);
			VectorSubtract(sabermaxs, saberloc, saberent->maxs);

			saberent->contents = CONTENTS_LIGHTSABER;// | CONTENTS_SHOTCLIP;

			G_SetOrigin( saberent, saberloc);
		}
		*/
		else
		{	// Otherwise there is no blocking possible.
			VectorClear(saberent->mins);
			VectorClear(saberent->maxs);
			G_SetOrigin(saberent, self->currentOrigin);
		}
		saberent->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		gi.linkentity(saberent);
	}
	else
	{
		WP_SaberInFlightReflectCheck( self, ucmd );
	}
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 2 )
	{
		CG_CubeOutline( saberent->absmin, saberent->absmax, 50, WPDEBUG_SaberColor( self->client->ps.saberColor ), 1 );
	}
#endif
}


//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
extern gentity_t *TossClientItems( gentity_t *self );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void WP_DropWeapon( gentity_t *dropper, vec3_t velocity )
{
	if ( !dropper || !dropper->client )
	{
		return;
	}
	int	replaceWeap = WP_NONE;
	int oldWeap = dropper->s.weapon;
	gentity_t *weapon = TossClientItems( dropper );
	if ( oldWeap == WP_THERMAL && dropper->NPC )
	{//Hmm, maybe all NPCs should go into melee?  Not too many, though, or they mob you and look silly
		replaceWeap = WP_MELEE;
	}
	if (dropper->ghoul2.IsValid()&& dropper->weaponModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model( dropper->ghoul2, dropper->weaponModel );
		dropper->weaponModel = -1;
	}
	//FIXME: does this work on the player?
	dropper->client->ps.stats[STAT_WEAPONS] |= ( 1 << replaceWeap );
	if ( !dropper->s.number )
	{
		if ( oldWeap == WP_THERMAL )
		{
			dropper->client->ps.ammo[weaponData[oldWeap].ammoIndex] -= weaponData[oldWeap].energyPerShot;
		}
		else
		{
			dropper->client->ps.stats[STAT_WEAPONS] &= ~( 1 << oldWeap );
		}
		CG_ChangeWeapon( replaceWeap );
	}
	else
	{
		dropper->client->ps.stats[STAT_WEAPONS] &= ~( 1 << oldWeap );
	}
	ChangeWeapon( dropper, replaceWeap );
	dropper->s.weapon = replaceWeap;
	if ( dropper->NPC )
	{
		dropper->NPC->last_ucmd.weapon = replaceWeap;
	}
	if ( weapon != NULL && velocity && !VectorCompare( velocity, vec3_origin ) )
	{//weapon should have a direction to it's throw
		VectorScale( velocity, 3, weapon->s.pos.trDelta );//NOTE: Presumes it is moving already...?
		if ( weapon->s.pos.trDelta[2] < 150 )
		{//this is presuming you don't want them to drop the weapon down on you...
			weapon->s.pos.trDelta[2] = 150;
		}
		//FIXME: gets stuck inside it's former owner...
		weapon->forcePushTime = level.time + 600; // let the push effect last for 600 ms
	}
}

void WP_KnockdownTurret( gentity_t *self, gentity_t *pas )
{
	//knock it over
	VectorCopy( pas->currentOrigin, pas->s.pos.trBase );
	pas->s.pos.trType = TR_LINEAR_STOP;
	pas->s.pos.trDuration = 250;
	pas->s.pos.trTime = level.time;
	pas->s.pos.trDelta[2] = ( 12.0f / ( pas->s.pos.trDuration * 0.001f ) );

	VectorCopy( pas->currentAngles, pas->s.apos.trBase );
	pas->s.apos.trType = TR_LINEAR_STOP;
	pas->s.apos.trDuration = 250;
	pas->s.apos.trTime = level.time;
	//FIXME: pick pitch/roll that always tilts it directly away from pusher
	pas->s.apos.trDelta[PITCH] = ( 100.0f / ( pas->s.apos.trDuration * 0.001f ) );

	//kill it
	pas->count = 0;
	pas->nextthink = -1;
	G_Sound( pas, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
	//push effect?
	pas->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

void WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty )
{
	int parts;
	qboolean runningResist = qfalse;

	if ( !self || self->health <= 0 || !self->client || !pusher || !pusher->client )
	{
		return;
	}
	if ( (!self->s.number || self->client->NPC_class == CLASS_DESANN || self->client->NPC_class == CLASS_LUKE)
		&& (VectorLengthSquared( self->client->ps.velocity ) > 10000 || self->client->ps.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 ) )
	{
		runningResist = qtrue;
	}
	if ( !runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_SpinningSaberAnim( self->client->ps.legsAnim )
		&& !PM_FlippingAnim( self->client->ps.legsAnim )
		&& !PM_RollingAnim( self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !PM_CrouchAnim( self->client->ps.legsAnim ))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}
	NPC_SetAnim( self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( !noPenalty )
	{
		if ( !runningResist )
		{
			VectorClear( self->client->ps.velocity );
			//still stop them from attacking or moving for a bit, though
			//FIXME: maybe push just a little (like, slide)?
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_FORCE_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
	Jedi_PlayBlockedPushSound( self );
}

void WP_ForceKnockdown( gentity_t *self, gentity_t *pusher, qboolean pull, qboolean strongKnockdown, qboolean breakSaberLock )
{
	if ( !self || !self->client || !pusher || !pusher->client )
	{
		return;
	}

	//break out of a saberLock?
	if ( breakSaberLock )
	{
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}

	if ( self->health > 0 )
	{
		if ( !self->s.number )
		{
			NPC_SetPainEvent( self );
		}
		else
		{
			GEntity_PainFunc( self, pusher, pusher, self->currentOrigin, 0, MOD_MELEE );
		}
		vec3_t	pushDir;
		if ( pull )
		{
			VectorSubtract( pusher->currentOrigin, self->currentOrigin, pushDir );
		}
		else
		{
			VectorSubtract( self->currentOrigin, pusher->currentOrigin, pushDir );
		}
		G_CheckLedgeDive( self, 72, pushDir, qfalse, qfalse );

		if ( !PM_SpinningSaberAnim( self->client->ps.legsAnim )
			&& !PM_FlippingAnim( self->client->ps.legsAnim )
			&& !PM_RollingAnim( self->client->ps.legsAnim )
			&& !PM_InKnockDown( &self->client->ps ) )
		{
			int knockAnim = BOTH_KNOCKDOWN1;//default knockdown
			if ( pusher->client->NPC_class == CLASS_DESANN && self->client->NPC_class != CLASS_LUKE )
			{//desann always knocks down, unless you're Luke
				strongKnockdown = qtrue;
			}
			if ( !self->s.number
				&& !strongKnockdown
				&& ( (!pull&&(self->client->ps.forcePowerLevel[FP_PUSH]>FORCE_LEVEL_1||!g_spskill->integer)) || (pull&&(self->client->ps.forcePowerLevel[FP_PULL]>FORCE_LEVEL_1||!g_spskill->integer)) )	)
			{//player only knocked down if pushed *hard*
				if ( self->s.weapon == WP_SABER )
				{//temp HACK: these are the only 2 pain anims that look good when holding a saber
					knockAnim = PM_PickAnim( self, BOTH_PAIN2, BOTH_PAIN3 );
				}
				else
				{
					knockAnim = PM_PickAnim( self, BOTH_PAIN1, BOTH_PAIN19 );
				}
			}
			else if ( PM_CrouchAnim( self->client->ps.legsAnim ) )
			{//crouched knockdown
				knockAnim = BOTH_KNOCKDOWN4;
			}
			else
			{//plain old knockdown
				vec3_t pLFwd, pLAngles = {0,self->client->ps.viewangles[YAW],0};
				vec3_t sFwd, sAngles = {0,pusher->client->ps.viewangles[YAW],0};
				AngleVectors( pLAngles, pLFwd, NULL, NULL );
				AngleVectors( sAngles, sFwd, NULL, NULL );
				if ( DotProduct( sFwd, pLFwd ) > 0.2f )
				{//pushing him from behind
					//FIXME: check to see if we're aiming below or above the waist?
					if ( pull )
					{
						knockAnim = BOTH_KNOCKDOWN1;
					}
					else
					{
						knockAnim = BOTH_KNOCKDOWN3;
					}
				}
				else
				{//pushing him from front
					if ( pull )
					{
						knockAnim = BOTH_KNOCKDOWN3;
					}
					else
					{
						knockAnim = BOTH_KNOCKDOWN1;
					}
				}
			}
			if ( knockAnim == BOTH_KNOCKDOWN1 && strongKnockdown )
			{//push *hard*
				knockAnim = BOTH_KNOCKDOWN2;
			}
			NPC_SetAnim( self, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			if ( self->s.number )
			{//randomize getup times
				int addTime = Q_irand( -300, 1000 );
				self->client->ps.legsAnimTimer += addTime;
				self->client->ps.torsoAnimTimer += addTime;
			}
			//
			if ( pusher->NPC && pusher->enemy == self )
			{//pushed pushed down his enemy
				G_AddVoiceEvent( pusher, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 3000 );
				pusher->NPC->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
	}
	self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

void ForceThrow( gentity_t *self, qboolean pull )
{//FIXME: pass in a target ent so we (an NPC) can push/pull just one targeted ent.
	//shove things in front of you away
	float		dist;
	gentity_t	*ent, *forwardEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	float		dot1, cone;
	trace_t		tr;
	int			anim, hold, soundIndex, cost, actualCost;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-throw while leaning
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_PUSH] > level.time )//self->client->ps.powerups[PW_FORCE_PUSH] > level.time )
	{//already pushing- now you can't haul someone across the room, sorry
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{
		if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
		{//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
	}

	if ( self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoAnimTimer > 400)
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoAnimTimer > 900)
		|| (self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoAnimTimer > 500)
		|| (self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoAnimTimer > 300)
		|| (self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoAnimTimer > 500) )
	{//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}
	if ( pull )
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	if ( !radius )
	{//no ability to do this yet
		return;
	}

	if ( pull )
	{
		cost = forcePowerNeeded[FP_PULL];
		if ( !WP_ForcePowerUsable( self, FP_PULL, cost ) )
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 200ms after this
		anim = BOTH_FORCEPULL;
		soundIndex = G_SoundIndex( "sound/weapons/force/pull.wav" );
		hold = 200;
	}
	else
	{
		cost = forcePowerNeeded[FP_PUSH];
		if ( !WP_ForcePowerUsable( self, FP_PUSH, cost ) )
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 1 second after this
		anim = BOTH_FORCEPUSH;
		soundIndex = G_SoundIndex( "sound/weapons/force/push.wav" );
		hold = 650;
	}

	int parts = SETANIM_TORSO;
	if ( !PM_InKnockDown( &self->client->ps ) )
	{
		if ( self->client->ps.saberLockTime > level.time )
		{
			self->client->ps.saberLockTime = 0;
			self->painDebounceTime = level.time + 2000;
			hold += 1000;
			parts = SETANIM_BOTH;
		}
		else if ( !VectorLengthSquared( self->client->ps.velocity ) && !(self->client->ps.pm_flags&PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		hold = floor( hold*g_timescale->value );
	}
	self->client->ps.weaponTime = hold;//was 1000, but want to swing sooner
	//do effect... FIXME: build-up or delay this until in proper part of anim
	self->client->ps.powerups[PW_FORCE_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;

	G_Sound( self, soundIndex );

	VectorCopy( self->client->ps.viewangles, fwdangles );
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	if ( pull )
	{
		cone = forcePullCone[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		cone = forcePushCone[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	if ( cone >= 1.0f )
	{//must be pointing right at them
		VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
		gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, G2_NOCOLLIDE, 0 );//was MASK_SHOT, changed to match crosshair trace
		/*
		//FIXME: can't just return, need to be able to push missiles
		if ( tr.entityNum >= ENTITYNUM_WORLD )
		{//no-one right in front of self, so short out
			return;
		}
		*/
		forwardEnt = &g_entities[tr.entityNum];
	}

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if ( ent->owner == self && ent->s.weapon != WP_THERMAL )//can push your own thermals
			continue;
		if ( !(ent->inuse) )
			continue;
		if ( ent->NPC && ent->NPC->scriptFlags & SCF_NO_FORCE )
		{
			if ( ent->s.weapon == WP_SABER )
			{//Hmm, should jedi do the resist behavior?  If this is on, perhaps it's because of a cinematic?
				WP_ResistForcePush( ent, self, qtrue );
			}
			continue;
		}
		if ( (ent->flags&FL_FORCE_PULLABLE_ONLY) && !pull )
		{//simple HACK: cannot force-push ammo rack items (because they may start in solid)
			continue;
		}
		//FIXME: don't push it if I already pushed it a little while ago
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( cone >= 1.0f )
			{//must be pointing right at them
				if ( ent != forwardEnt )
				{//must be the person I'm looking right at
					if ( ent->client && !pull
						&& ent->client->ps.forceGripEntityNum == self->s.number
						&& (self->s.eFlags&EF_FORCE_GRIPPED) )
					{//this is the guy that's force-gripping me, use a wider cone regardless of force power level
					}
					else
					{
						continue;
					}
				}
			}
			if ( ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )//|| !(ent->flags&FL_DROPPED_ITEM) )//was only dropped items
			{
				//FIXME: need pushable objects
				if ( ent->s.eFlags & EF_NODRAW )
				{
					continue;
				}
				if ( !ent->client )
				{
					if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
					{//not a lightsaber
						if ( !(ent->svFlags&SVF_GLASS_BRUSH) )
						{//and not glass
							if ( Q_stricmp( "func_door", ent->classname ) != 0 || !(ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
							{//not a force-usable door
								if ( Q_stricmp( "func_static", ent->classname ) != 0 || (!(ent->spawnflags&1/*F_PUSH*/)&&!(ent->spawnflags&2/*F_PULL*/)) )
								{//not a force-usable func_static
									if ( Q_stricmp( "limb", ent->classname ) )
									{//not a limb
										if ( ent->s.weapon == WP_TURRET && !Q_stricmp( "PAS", ent->classname ) && ent->s.apos.trType == TR_STATIONARY )
										{//can knock over placed turrets
											if ( !self->s.number || self->enemy != ent )
											{//only NPCs who are actively mad at this turret can push it over
												continue;
											}
										}
										else
										{
											continue;
										}
									}
								}
							}
							else if ( ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2 )
							{//not at rest
								continue;
							}
						}
					}
					//continue;
				}
				else if ( ent->client->NPC_class == CLASS_MARK1 )
				{//can't push Mark1 unless push 3
					if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
					{
						continue;
					}
				}
				else if ( ent->client->NPC_class == CLASS_GALAKMECH || ent->client->NPC_class == CLASS_ATST )
				{//can't push ATST or Galak
					continue;
				}
				else if ( ent->s.weapon == WP_EMPLACED_GUN )
				{//FIXME: maybe can pull them out?
					continue;
				}
				else if ( ent->client->playerTeam == self->client->playerTeam && self->enemy && self->enemy != ent )
				{//can't accidently push a teammate while in combat
					continue;
				}
			}
			else if ( ent->s.eType == ET_ITEM
				&& ent->item
				&& ent->item->giType == IT_HOLDABLE
				&& ent->item->giTag == INV_SECURITY_KEY )
				//&& (ent->flags&FL_DROPPED_ITEM) ???
			{//dropped security keys can't be pushed?  But placed ones can...?  does this make any sense?
				if ( !pull || self->s.number )
				{//can't push, NPC's can't do anything to it
					continue;
				}
				else
				{
					if ( g_crosshairEntNum != ent->s.number )
					{//player can pull it if looking *right* at it
						if ( cone >= 1.0f )
						{//we did a forwardEnt trace
							if ( forwardEnt != ent )
							{//must be pointing right at them
								continue;
							}
						}
						else
						{//do a forwardEnt trace
							VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
							gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, G2_NOCOLLIDE, 0 );//was MASK_SHOT, changed to match crosshair trace
							if ( tr.entityNum != ent->s.number )
							{//last chance
								continue;
							}
						}
					}
				}
			}
		}
		else
		{
			if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
			{//can't force-push/pull stuck missiles (detpacks, tripmines)
				continue;
			}
			if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
			{//only thermal detonators can be pushed once stopped
				continue;
			}
		}

		//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ )
		{
			if ( center[i] < ent->absmin[i] )
			{
				v[i] = ent->absmin[i] - center[i];
			} else if ( center[i] > ent->absmax[i] )
			{
				v[i] = center[i] - ent->absmax[i];
			} else
			{
				v[i] = 0;
			}
		}

		VectorSubtract( ent->absmax, ent->absmin, size );
		VectorMA( ent->absmin, 0.5, size, ent_org );

		//see if they're in front of me
		VectorSubtract( ent_org, center, dir );
		VectorNormalize( dir );
		if ( cone < 1.0f )
		{//must be within the forward cone
			if ( ent->client && !pull
				&& ent->client->ps.forceGripEntityNum == self->s.number
				&& self->s.eFlags&EF_FORCE_GRIPPED )
			{//this is the guy that's force-gripping me, use a wider cone regardless of force power level
				if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
					continue;
			}
			else if ( ent->s.eType == ET_MISSILE )//&& ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )
			{//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
				if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
					continue;
			}
			else if ( (dot1 = DotProduct( dir, forward )) < cone )
			{
				continue;
			}
		}
		else if ( ent->s.eType == ET_MISSILE )
		{//a missile and we're at force level 1... just use a small cone, but not ridiculously small
			if ( (dot1 = DotProduct( dir, forward )) < 0.75f )
			{
				continue;
			}
		}//else is an NPC or brush entity that our forward trace would have to hit

		dist = VectorLength( v );

		//Now check and see if we can actually deflect it
		//method1
		//if within a certain range, deflect it
		if ( ent->s.eType == ET_MISSILE && cone >= 1.0f )
		{//smaller radius on missile checks at force push 1
			if ( dist >= 192 )
			{
				continue;
			}
		}
		else if ( dist >= radius )
		{
			continue;
		}

		//in PVS?
		if ( !ent->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.eyePoint ) )
		{//must be in PVS
			continue;
		}

		if ( ent != forwardEnt )
		{//don't need to trace against forwardEnt again
		//really should have a clear LOS to this thing...
			gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_OPAQUE|CONTENTS_SOLID, G2_NOCOLLIDE, 0 );//was MASK_SHOT, but changed to match above trace and crosshair trace
			if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
			{//must have clear LOS
				continue;
			}
		}

		// ok, we are within the radius, add us to the incoming list
		push_list[ent_count] = ent;
		ent_count++;
	}

	if ( ent_count )
	{
		for ( int x = 0; x < ent_count; x++ )
		{
			if ( push_list[x]->client )
			{
				vec3_t	pushDir;
				float	knockback = pull?0:200;

//FIXMEFIXMEFIXMEFIXMEFIXME: extern a lot of this common code when I have the time!!!

				//First, if this is the player we're push/pulling, see if he can counter it
				if ( !push_list[x]->s.number )
				{//player
					if ( push_list[x]->health > 0 //alive
						&& push_list[x]->client //client
						&& push_list[x]->client->ps.torsoAnim != BOTH_FORCEGRIP_HOLD// BOTH_FORCEGRIP1//wasn't trying to grip anyone
						&& (self->client->NPC_class != CLASS_DESANN || !Q_irand( 0, 2 ) )//only 30% chance of resisting a Desann push
						&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE//on the ground
						&& !PM_InKnockDown( &push_list[x]->client->ps )//not knocked down already
						&& push_list[x]->client->ps.saberLockTime < level.time//not involved in a saberLock
						&& push_list[x]->client->ps.weaponTime < level.time//not attacking or otherwise busy
						&& (push_list[x]->client->ps.weapon == WP_SABER||push_list[x]->client->ps.weapon == WP_MELEE) )//using saber or fists
					{//trying to push or pull the player!
						if ( push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time//player was pushing/pulling too
							||( pull && Q_irand( 0, (push_list[x]->client->ps.forcePowerLevel[FP_PULL] - self->client->ps.forcePowerLevel[FP_PULL])*2+1 ) > 0 )//player's pull is high enough
							||( !pull && Q_irand( 0, (push_list[x]->client->ps.forcePowerLevel[FP_PUSH] - self->client->ps.forcePowerLevel[FP_PUSH])*2+1 ) > 0 ) )//player's push is high enough
						{//player's force push/pull is high enough to try to stop me
							if ( InFront( self->currentOrigin, push_list[x]->client->renderInfo.eyePoint, push_list[x]->client->ps.viewangles, 0.3f ) )
							{//I'm in front of player
								WP_ResistForcePush( push_list[x], self, qfalse );
								push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
								push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
								continue;
							}
						}
					}
				}
				else if ( push_list[x]->client && Jedi_WaitingAmbush( push_list[x] ) )
				{
					WP_ForceKnockdown( push_list[x], self, pull, qtrue, qfalse );
					continue;
				}


				//okay, everyone else (or player who couldn't resist it)...
				if ( ((self->s.number == 0 && Q_irand( 0, 2 ) ) || Q_irand( 0, 2 ) ) && push_list[x]->client && push_list[x]->health > 0 //a living client
						&& push_list[x]->client->ps.weapon == WP_SABER //Jedi
						&& push_list[x]->health > 0 //alive
						&& (self->client->NPC_class != CLASS_DESANN || !Q_irand( 0, 2 ) )//only 30% chance of resisting a Desann push
						&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE //on the ground
						&& InFront( self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles, 0.3f ) //I'm in front of him
						&& ( push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time ||//he's pushing too
								(push_list[x]->s.number != 0 && push_list[x]->client->ps.weaponTime < level.time)//not the player and not attacking (NPC jedi auto-defend against pushes)
						   )
					)
				{//Jedi don't get pushed, they resist as long as they aren't already attacking and are on the ground
					if ( push_list[x]->client->ps.saberLockTime > level.time )
					{//they're in a lock
						if ( push_list[x]->client->ps.saberLockEnemy != self->s.number )
						{//they're not in a lock with me
							continue;
						}
						else if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 ||
							push_list[x]->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
						{//they're in a lock with me, but my push is too weak
							continue;
						}
						else
						{//we will knock them down
							self->painDebounceTime = 0;
							self->client->ps.weaponTime = 500;
							if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
							{
								self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
							}
						}
					}
					if ( !pull && self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 && !Q_irand(0,2) &&
							push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
					{//a level 3 push can even knock down a jedi
						if ( PM_InKnockDown( &push_list[x]->client->ps ) )
						{//can't knock them down again
							continue;
						}
						WP_ForceKnockdown( push_list[x], self, pull, qfalse, qtrue );
					}
					else
					{
						WP_ResistForcePush( push_list[x], self, qfalse );
					}
				}
				else
				{
					//UGH: FIXME: for enemy jedi, they should probably always do force pull 3, and not your weapon (if player?)!
					//shove them
					if ( push_list[x]->s.number && push_list[x]->message )
					{//an NPC who has a key
						//don't push me... FIXME: maybe can pull the key off me?
						WP_ForceKnockdown( push_list[x], self, pull, qfalse, qfalse );
						continue;
					}
					if ( pull )
					{
						VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, pushDir );
						if ( self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_1
							&& push_list[x]->s.weapon != WP_SABER
							&& push_list[x]->s.weapon != WP_MELEE
							&& push_list[x]->s.weapon != WP_THERMAL )
						{//yank the weapon - NOTE: level 1 just knocks them down, not take weapon
							//FIXME: weapon yank anim if not a knockdown?
							if ( InFront( self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles, 0.0f ) )
							{//enemy has to be facing me, too...
								WP_DropWeapon( push_list[x], pushDir );
							}
						}
						knockback += VectorNormalize( pushDir );
						if ( knockback > 200 )
						{
							knockback = 200;
						}
						if ( self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_3 )
						{//maybe just knock them down
							knockback /= 3;
						}
					}
					else
					{
						VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
						knockback -= VectorNormalize( pushDir );
						if ( knockback < 100 )
						{
							knockback = 100;
						}
						//scale for push level
						if ( self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2 )
						{//maybe just knock them down
							knockback /= 3;
						}
						else if ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
						{//super-hard push
							//Hmm, maybe in this case can even nudge/knockdown a jedi?  Especially if close?
							//knockback *= 5;
						}
					}
					//actually push/pull the enemy
					G_Throw( push_list[x], pushDir, knockback );
					//make it so they don't actually hurt me when pulled at me...
					push_list[x]->forcePuller = self->s.number;

					if ( push_list[x]->client->ps.velocity[2] < knockback )
					{
						push_list[x]->client->ps.velocity[2] = knockback;
					}

					if ( push_list[x]->health > 0 )
					{//target is still alive
						if ( (push_list[x]->s.number||(cg.renderingThirdPerson&&!cg.zoomMode)) //NPC or 3rd person player
							&& ((!pull&&self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2 && push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1) //level 1 push
								|| (pull && self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_2 && push_list[x]->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_1)) )//level 1 pull
						{//NPC or third person player (without force push/pull skill), and force push/pull level is at 1
							WP_ForceKnockdown( push_list[x], self, pull, (!pull&&knockback>150), qfalse );
						}
						else if ( !push_list[x]->s.number )
						{//player, have to force an anim on him
							WP_ForceKnockdown( push_list[x], self, pull, (!pull&&knockback>150), qfalse );
						}
						else
						{//NPC and force-push/pull at level 2 or higher
							WP_ForceKnockdown( push_list[x], self, pull, (!pull&&knockback>100), qfalse );
						}
					}
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				}
			}
			else if ( push_list[x]->s.weapon == WP_SABER && (push_list[x]->contents&CONTENTS_LIGHTSABER) )
			{//a thrown saber, just send it back
				/*
				if ( pull )
				{//steal it?
				}
				else */if ( push_list[x]->owner && push_list[x]->owner->client && push_list[x]->owner->client->ps.saberActive && push_list[x]->s.pos.trType == TR_LINEAR && push_list[x]->owner->client->ps.saberEntityState != SES_RETURNING )
				{//it's on and being controlled
					//FIXME: prevent it from damaging me?
					if ( self->s.number == 0 || Q_irand( 0, 2 ) )
					{//certain chance of throwing it aside and turning it off?
						//give it some velocity away from me
						//FIXME: maybe actually push or pull it?
						if ( Q_irand( 0, 1 ) )
						{
							VectorScale( right, -1, right );
						}
						G_ReflectMissile( self, push_list[x], right );
						//FIXME: isn't turning off!!!
						WP_SaberDrop( push_list[x]->owner, push_list[x] );
					}
					else
					{
						WP_SaberReturn( push_list[x]->owner, push_list[x] );
					}
					//different effect?
				}
			}
			else if ( push_list[x]->s.eType == ET_MISSILE
				&& push_list[x]->s.pos.trType != TR_STATIONARY
				&& (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				vec3_t dir2Me;
				VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, dir2Me );
				float dot = DotProduct( push_list[x]->s.pos.trDelta, dir2Me );
				if ( pull )
				{//deflect rather than reflect?
				}
				else
				{
					if ( push_list[x]->s.eFlags&EF_MISSILE_STICK )
					{//caught a sticky in-air
						push_list[x]->s.eType = ET_MISSILE;
						push_list[x]->s.eFlags &= ~EF_MISSILE_STICK;
						push_list[x]->s.eFlags |= EF_BOUNCE_HALF;
						push_list[x]->splashDamage /= 3;
						push_list[x]->splashRadius /= 3;
						push_list[x]->e_ThinkFunc = thinkF_WP_Explode;
						push_list[x]->nextthink = level.time + Q_irand( 500, 3000 );
					}
					if ( dot >= 0 )
					{//it's heading towards me
						G_ReflectMissile( self, push_list[x], forward );
					}
					else
					{
						VectorScale( push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta );
					}
					//deflect sound
					//G_Sound( push_list[x], G_SoundIndex( va("sound/weapons/blaster/reflect%d.wav", Q_irand( 1, 3 ) ) ) );
					//push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				}
			}
			else if ( push_list[x]->svFlags & SVF_GLASS_BRUSH )
			{//break the glass
				trace_t tr;
				vec3_t	pushDir;
				float	damage = 800;

				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}

				if ( pull )
				{
					VectorSubtract( self->client->renderInfo.eyePoint, tr.endpos, pushDir );
				}
				else
				{
					VectorSubtract( tr.endpos, self->client->renderInfo.eyePoint, pushDir );
				}
				/*
				VectorSubtract( push_list[x]->absmax, push_list[x]->absmin, size );
				VectorMA( push_list[x]->absmin, 0.5, size, center );
				if ( pull )
				{
					VectorSubtract( self->client->renderInfo.eyePoint, center, pushDir );
				}
				else
				{
					VectorSubtract( center, self->client->renderInfo.eyePoint, pushDir );
				}
				*/
				damage -= VectorNormalize( pushDir );
				if ( damage < 200 )
				{
					damage = 200;
				}
				VectorScale( pushDir, damage, pushDir );

				G_Damage( push_list[x], self, self, pushDir, tr.endpos, damage, 0, MOD_UNKNOWN );
			}
			else if ( !Q_stricmp( "func_static", push_list[x]->classname ) )
			{//force-usable func_static
				if ( !pull && (push_list[x]->spawnflags&1/*F_PUSH*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
				else if ( pull && (push_list[x]->spawnflags&2/*F_PULL*/) )
				{
					GEntity_UseFunc( push_list[x], self, self );
				}
			}
			else if ( !Q_stricmp( "func_door", push_list[x]->classname ) && (push_list[x]->spawnflags&2/*MOVER_FORCE_ACTIVATE*/) )
			{//push/pull the door
				vec3_t	pos1, pos2;

				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}

				if ( VectorCompare( vec3_origin, push_list[x]->s.origin ) )
				{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
					VectorSubtract( push_list[x]->absmax, push_list[x]->absmin, size );
					VectorMA( push_list[x]->absmin, 0.5, size, center );
					if ( (push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS1 )
					{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos1, center );
					}
					else if ( !(push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS2 )
					{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
						VectorSubtract( center, push_list[x]->pos2, center );
					}
					VectorAdd( center, push_list[x]->pos1, pos1 );
					VectorAdd( center, push_list[x]->pos2, pos2 );
				}
				else
				{//actually has an origin, pos1 and pos2 are absolute
					VectorCopy( push_list[x]->currentOrigin, center );
					VectorCopy( push_list[x]->pos1, pos1 );
					VectorCopy( push_list[x]->pos2, pos2 );
				}

				if ( Distance( pos1, self->client->renderInfo.eyePoint ) < Distance( pos2, self->client->renderInfo.eyePoint ) )
				{//pos1 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
				}
				else
				{//pos2 is closer
					if ( push_list[x]->moverState == MOVER_POS1 )
					{//at the farthest pos
						if ( !pull )
						{//trying to push, but already at farthest point, so screw it
							continue;
						}
					}
					else if ( push_list[x]->moverState == MOVER_POS2 )
					{//at closest pos
						if ( pull )
						{//trying to pull, but already at closest point, so screw it
							continue;
						}
					}
				}
				GEntity_UseFunc( push_list[x], self, self );
			}
			else if ( push_list[x]->s.eType == ET_MISSILE/*thermal resting on ground*/
				|| push_list[x]->s.eType == ET_ITEM
				|| push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp( "limb", push_list[x]->classname ) == 0 )
			{//general object, toss it
				vec3_t	pushDir, kvel;
				float	knockback = pull?0:200;
				float	mass = 200;
				if ( pull )
				{
					if ( push_list[x]->s.eType == ET_ITEM )
					{//pull it to a little higher point
						vec3_t	adjustedOrg;
						VectorCopy( self->currentOrigin, adjustedOrg );
						adjustedOrg[2] += self->maxs[2]/3;
						VectorSubtract( adjustedOrg, push_list[x]->currentOrigin, pushDir );
					}
					else
					{
						VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, pushDir );
					}
					knockback += VectorNormalize( pushDir );
					if ( knockback > 200 )
					{
						knockback = 200;
					}
					if ( push_list[x]->s.eType == ET_ITEM
						&& push_list[x]->item
						&& push_list[x]->item->giType == IT_HOLDABLE
						&& push_list[x]->item->giTag == INV_SECURITY_KEY )
					{//security keys are pulled with less enthusiasm
						if ( knockback > 100 )
						{
							knockback = 100;
						}
					}
					else if ( knockback > 200 )
					{
						knockback = 200;
					}
				}
				else
				{
					//HMM, if I have an auto-enemy & he's in front of me, push it toward him?
					VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
					knockback -= VectorNormalize( pushDir );
					if ( knockback < 100 )
					{
						knockback = 100;
					}
				}
				//FIXME: if pull a FL_FORCE_PULLABLE_ONLY, clear the flag, assuming it's no longer in solid?  or check?
				VectorCopy( push_list[x]->currentOrigin, push_list[x]->s.pos.trBase );
				push_list[x]->s.pos.trTime = level.time;								// move a bit on the very first frame
				if ( push_list[x]->s.pos.trType != TR_INTERPOLATE )
				{//don't do this to rolling missiles
					push_list[x]->s.pos.trType = TR_GRAVITY;
				}

				if ( push_list[x]->e_ThinkFunc == thinkF_G_RunObject && push_list[x]->physicsBounce )
				{
					mass = push_list[x]->physicsBounce;
				}
				if ( mass < 50 )
				{//???
					mass = 50;
				}
				if ( g_gravity->value > 0 )
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass * 0.8, kvel );
					kvel[2] = pushDir[2] * g_knockback->value * knockback / mass * 1.5;
				}
				else
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass, kvel );
				}
				VectorAdd( push_list[x]->s.pos.trDelta, kvel, push_list[x]->s.pos.trDelta );
				if ( g_gravity->value > 0 )
				{
					if ( push_list[x]->s.pos.trDelta[2] < knockback )
					{
						push_list[x]->s.pos.trDelta[2] = knockback;
					}
				}
				//no trDuration?
				if ( push_list[x]->e_ThinkFunc != thinkF_G_RunObject )
				{//objects spin themselves?
					//spin it
					//FIXME: messing with roll ruins the rotational center???
					push_list[x]->s.apos.trTime = level.time;
					push_list[x]->s.apos.trType = TR_LINEAR;
					VectorClear( push_list[x]->s.apos.trDelta );
					push_list[x]->s.apos.trDelta[1] = Q_irand( -800, 800 );
				}

				if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
				{//make sure it runs it's physics
					push_list[x]->e_ThinkFunc = thinkF_LimbThink;
					push_list[x]->nextthink = level.time + FRAMETIME;
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				if ( push_list[x]->item && push_list[x]->item->giTag == INV_SECURITY_KEY )
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_DISCOVERED );//security keys are more important
				}
				else
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_SUSPICIOUS );//hmm... or should this always be discovered?
				}
			}
			else if ( push_list[x]->s.weapon == WP_TURRET
				&& !Q_stricmp( "PAS", push_list[x]->classname )
				&& push_list[x]->s.apos.trType == TR_STATIONARY )
			{//a portable turret
				WP_KnockdownTurret( self, push_list[x] );
			}
		}
		if ( pull )
		{
			if ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
			{//at level 3, can pull multiple, so it costs more
				actualCost = forcePowerNeeded[FP_PUSH]*ent_count;
				if ( actualCost > 50 )
				{
					actualCost = 50;
				}
				else if ( actualCost < cost )
				{
					actualCost = cost;
				}
			}
			else
			{
				actualCost = cost;
			}
			WP_ForcePowerStart( self, FP_PULL, actualCost );
		}
		else
		{
			if ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
			{//at level 3, can push multiple, so costs more
				actualCost = forcePowerNeeded[FP_PUSH]*ent_count;
				if ( actualCost > 50 )
				{
					actualCost = 50;
				}
				else if ( actualCost < cost )
				{
					actualCost = cost;
				}
			}
			else if ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_1 )
			{//at level 3, can push multiple, so costs more
				actualCost = floor(forcePowerNeeded[FP_PUSH]*ent_count/1.5f);
				if ( actualCost > 50 )
				{
					actualCost = 50;
				}
				else if ( actualCost < cost )
				{
					actualCost = cost;
				}
			}
			else
			{
				actualCost = cost;
			}
			WP_ForcePowerStart( self, FP_PUSH, actualCost );
		}
	}
	else
	{//didn't push or pull anything?  don't penalize them too much
		if ( pull )
		{
			WP_ForcePowerStart( self, FP_PULL, 5 );
		}
		else
		{
			WP_ForcePowerStart( self, FP_PUSH, 5 );
		}
	}
	if ( self->NPC )
	{//NPCs can push more often
		//FIXME: vary by rank and game skill?
		self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + 200;
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
	}
}

void ForceSpeed( gentity_t *self, int duration )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_SPEED, 0 ) )
	{
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	if ( !self->s.number && in_camera )
	{//player can't use force powers in cinematic
		return;
	}
	WP_ForcePowerStart( self, FP_SPEED, 0 );
	if ( duration )
	{
		self->client->ps.forcePowerDuration[FP_SPEED] = level.time + duration;
	}
	G_Sound( self, G_SoundIndex( "sound/weapons/force/speed.wav" ) );
}

void ForceHeal( gentity_t *self )
{
	if ( self->health <= 0 || self->client->ps.stats[STAT_MAX_HEALTH] <= self->health )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_HEAL, 20 ) )
	{//must have enough force power for at least 5 points of health
		return;
	}

	if ( self->painDebounceTime > level.time || (self->client->ps.weaponTime&&self->client->ps.weapon!=WP_NONE) )
	{//can't initiate a heal while taking pain or attacking
		return;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	if ( !self->s.number && in_camera )
	{//player can't use force powers in cinematic
		return;
	}
	/*
	if ( self->client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2 )
	{//instant heal
		//no more than available force power
		int max = self->client->ps.forcePower;
		if ( max > MAX_FORCE_HEAL )
		{//no more than max allowed
			max = MAX_FORCE_HEAL;
		}
		if ( max > self->client->ps.stats[STAT_MAX_HEALTH] - self->health )
		{//no more than what's missing
			max = self->client->ps.stats[STAT_MAX_HEALTH] - self->health;
		}
		self->health += max;
		WP_ForcePowerDrain( self, FP_HEAL, max );
		G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d.mp3", Q_irand( 1, 4 ) ) );
	}
	else
	*/
	{
		//start health going up
		//NPC_SetAnim( self, SETANIM_TORSO, ?, SETANIM_FLAG_OVERRIDE );
		WP_ForcePowerStart( self, FP_HEAL, 0 );
		if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 )
		{//must meditate
			//FIXME: holster weapon (select WP_NONE?)
			//FIXME: BOTH_FORCEHEAL_START
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
			self->client->ps.torsoAnimTimer = self->client->ps.legsAnimTimer = FORCE_HEAL_INTERVAL*MAX_FORCE_HEAL + 2000;//???
			if ( self->client->ps.saberActive )
			{
				self->client->ps.saberActive = qfalse;//turn off saber when meditating
				if ( self->client->playerTeam == TEAM_PLAYER )
				{
					G_SoundOnEnt( self, CHAN_WEAPON, "sound/weapons/saber/saberoff.wav" );
				}
				else
				{
					G_SoundOnEnt( self, CHAN_WEAPON, "sound/weapons/saber/enemy_saber_off.wav" );
				}
			}
		}
		else
		{//just a quick gesture
			/*
			//Can't get an anim that looks good...
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEHEAL_QUICK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
			*/
		}
	}

	//FIXME: always play healing effect
	G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/heal.mp3" );
}

extern void NPC_PlayConfusionSound( gentity_t *self );
extern void NPC_Jedi_PlayConfusionSound( gentity_t *self );
qboolean WP_CheckBreakControl( gentity_t *self )
{
	if ( !self )
	{
		return qfalse;
	}
	if ( !self->s.number )
	{//player
		if ( self->client && self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
		{//control-level
			if ( self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD )
			{//we are in a viewentity
				gentity_t *controlled = &g_entities[self->client->ps.viewEntity];
				if ( controlled->NPC && controlled->NPC->controlledTime > level.time )
				{//it is an NPC we controlled
					//clear it and return
					G_ClearViewEntity( self );
					return qtrue;
				}
			}
		}
	}
	else
	{//NPC
		if ( self->NPC && self->NPC->controlledTime > level.time )
		{//being controlled
			gentity_t *controller = &g_entities[0];
			if ( controller->client && controller->client->ps.viewEntity == self->s.number )
			{//we are being controlled by player
				if ( controller->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
				{//control-level mind trick
					//clear the control and return
					G_ClearViewEntity( controller );
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

void ForceTelepathy( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	qboolean	targetLive = qfalse;

	if ( WP_CheckBreakControl( self ) )
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}
	//FIXME: if mind trick 3 and aiming at an enemy need more force power
	if ( !WP_ForcePowerUsable( self, FP_TELEPATHY, 0 ) )
	{
		return;
	}

	if ( self->client->ps.weaponTime >= 800 )
	{//just did one!
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	if ( !self->s.number && in_camera )
	{//player can't use force powers in cinematic
		return;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );
	VectorMA( self->client->renderInfo.eyePoint, 2048, forward, end );

	//Cause a distraction if enemy is not fighting
	gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_BODY, G2_NOCOLLIDE, 0 );
	if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
	{
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if( traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
	{
		return;
	}

	if ( traceEnt && traceEnt->client  )
	{
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
		case CLASS_ATST://much too big to grip!
		//no droids either
		case CLASS_PROBE:
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_PROTOCOL:
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if ( targetLive && traceEnt->NPC )
	{//hit an organic non-player
		if ( G_ActivateBehavior( traceEnt, BSET_MINDTRICK ) )
		{//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		else if ( traceEnt->client->playerTeam != self->client->playerTeam )
		{//an enemy
			int override = 0;
			if ( (traceEnt->NPC->scriptFlags&SCF_NO_MIND_TRICK) )
			{
				if ( traceEnt->client->NPC_class == CLASS_GALAKMECH )
				{
					G_AddVoiceEvent( NPC, Q_irand( EV_CONFUSE1, EV_CONFUSE3 ), Q_irand( 3000, 5000 ) );
				}
			}
			else if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
			{//control them, even jedi
				G_SetViewEntity( self, traceEnt );
				traceEnt->NPC->controlledTime = level.time + 30000;
			}
			else if ( traceEnt->s.weapon != WP_SABER )
			{//haha!  Jedi aren't easily confused!
				if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 )
				{//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					override = 50;
					if ( self->client->ps.forcePower < 50 )
					{
						return;
					}
					if ( traceEnt->s.weapon != WP_NONE )
					{//don't charm people who aren't capable of fighting... like ugnaughts and droids
						if ( traceEnt->enemy )
						{
							G_ClearEnemy( traceEnt );
						}
						if ( traceEnt->NPC )
						{
							//traceEnt->NPC->tempBehavior = BS_FOLLOW_LEADER;
							traceEnt->client->leader = self;
						}
						//FIXME: maybe pick an enemy right here?
						team_t	saveTeam = traceEnt->client->enemyTeam;
						traceEnt->client->enemyTeam = traceEnt->client->playerTeam;
						traceEnt->client->playerTeam = saveTeam;
						//FIXME: need a *charmed* timer on this...?  Or do TEAM_PLAYERS assume that "confusion" means they should switch to team_enemy when done?
						traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]];
					}
				}
				else
				{//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds
					NPC_PlayConfusionSound( traceEnt );
					if ( traceEnt->enemy )
					{
						G_ClearEnemy( traceEnt );
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound( traceEnt );
			}
			WP_ForcePowerStart( self, FP_TELEPATHY, override );
		}
		else if ( traceEnt->client->playerTeam == self->client->playerTeam )
		{//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if ( traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC!=NULL && !(traceEnt->NPC->scriptFlags&SCF_NO_RESPONSE) )
			{
				NPC_UseResponse( traceEnt, self, qfalse );
				WP_ForcePowerStart( self, FP_TELEPATHY, 1 );
			}
		}//NOTE: no effect on TEAM_NEUTRAL?
		vec3_t	eyeDir;
		AngleVectors( traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL );
		VectorNormalize( eyeDir );
		G_PlayEffect( "force_touch", traceEnt->client->renderInfo.eyePoint, eyeDir );

		//make sure this plays and that you cannot press fire for about 1 second after this
		//FIXME: BOTH_FORCEMINDTRICK or BOTH_FORCEDISTRACT
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
		//FIXME: build-up or delay this until in proper part of anim
	}
	else
	{
		if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && tr.fraction * 2048 > 64 )
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffect( G_EffectIndex( "force_touch" ), tr.endpos, tr.plane.normal );
			//FIXME: these events don't seem to always be picked up...?
			AddSoundEvent( self, tr.endpos, 512, AEL_SUSPICIOUS, qtrue );
			AddSightEvent( self, tr.endpos, 512, AEL_SUSPICIOUS, 50 );
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
}

void ForceGrip( gentity_t *self )
{//FIXME: make enemy Jedi able to use this
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt = NULL;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force grip when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-grip while leaning
		return;
	}

	if ( self->client->ps.forceGripEntityNum <= ENTITYNUM_WORLD )
	{//already gripping
		if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
		{
			self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
		}
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_GRIP, 0 ) )
	{//can't use it right now
		return;
	}

	if ( self->client->ps.forcePower < 26 )
	{//need 20 to start, 6 to hold it for any decent amount of time...
		return;
	}

	if ( self->client->ps.weaponTime )
	{//busy
		return;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	//Cause choking anim + health drain in ent in front of me
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );
	VectorMA( self->client->renderInfo.handLPoint, FORCE_GRIP_DIST, forward, end );

	if ( self->enemy && (self->s.number || InFront( self->enemy->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 0.2f ) ) )
	{//NPCs can always lift enemy since we assume they're looking at them, players need to be facing the enemy
		if ( gi.inPVS( self->enemy->currentOrigin, self->client->renderInfo.eyePoint ) )
		{//must be in PVS
			gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, self->enemy->currentOrigin, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
			if ( tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number )
			{//must have clear LOS
				traceEnt = self->enemy;
			}
		}
	}
	if ( !traceEnt )
	{//okay, trace straight ahead and see what's there
		gi.trace( &tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
		if ( tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
		{
			return;
		}

		traceEnt = &g_entities[tr.entityNum];
	}

	if ( !traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE) )
	{
		return;
	}

	if ( traceEnt->client )
	{
		if ( traceEnt->client->ps.forceJumpZStart )
		{//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
			G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand( 3000, 5000 ) );
			return;
			break;
		case CLASS_ATST://much too big to grip!
			return;
		//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE://?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			break;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			break;
		case CLASS_DESANN://Desann cannot be gripped, he just pushes you back instantly
			Jedi_PlayDeflectSound( traceEnt );
			ForceThrow( traceEnt, qfalse );
			return;
			break;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_TAVION:
		case CLASS_JEDI:
		case CLASS_LUKE:
			if ( traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
			{
				Jedi_PlayDeflectSound( traceEnt );
				ForceThrow( traceEnt, qfalse );
				return;
			}
			break;
		default:
			break;
		}
		if ( traceEnt->s.weapon == WP_EMPLACED_GUN )
		{//FIXME: maybe can pull them out?
			return;
		}
		if ( self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam )
		{//can't accidently grip your teammate in combat
			return;
		}
	}
	else
	{//can't grip non-clients... right?
		return;
	}
	WP_ForcePowerStart( self, FP_GRIP, 20 );
	//FIXME: rule out other things?
	//FIXME: Jedi resist, like the push and pull?
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	//if ( traceEnt->client )
	{
		G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
		if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER )
		{//if we pick up & carry, drop their weap
			if ( traceEnt->s.weapon )
			{
				if ( traceEnt->s.weapon != WP_SABER )
				{
					WP_DropWeapon( traceEnt, NULL );
				}
				else
				{
					//turn it off?
					traceEnt->client->ps.saberActive = qfalse;
					G_SoundOnEnt( traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.wav" );
				}
			}
		}
		//else FIXME: need a one-armed choke if we're not on a high enough level to make them drop their gun
		VectorCopy( traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg );
	}
	/*
	else
	{
		VectorCopy( traceEnt->currentOrigin, self->client->ps.forceGripOrg );
	}
	*/
	self->client->ps.forceGripOrg[2] += 48;//FIXME: define?
	if ( self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
	{//just a duration
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 5000;
		traceEnt->s.loopSound = G_SoundIndex( "sound/weapons/force/grip.mp3" );
	}
	else
	{
		if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2 )
		{//lifting sound?  or always?
		}
		//if ( traceEnt->s.number )
		{//picks them up for a second first
			self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		}
		/*
		else
		{//player should take damage right away
			self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		}
		*/
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/grip.mp3" );
	}
}

void ForceLightning( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force lightning when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-lightning while leaning
		return;
	}
	if ( self->client->ps.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING, 0 ) )
	{
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_LIGHTNING] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	//Shoot lightning from hand
	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	else
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/lightning.wav" );
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
	{//short burst
		//G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/lightning.wav" );
	}
	else
	{//holding it
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/lightning2.wav" );
	}

	//FIXME: build-up or delay this until in proper part of anim
	self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
	WP_ForcePowerStart( self, FP_LIGHTNING, self->client->ps.torsoAnimTimer );
}

void ForceLightningDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, float dist, float dot, vec3_t impactPoint )
{
	if( traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
	{
		return;
	}

	if ( traceEnt && traceEnt->takedamage )
	{
		if ( !traceEnt->client || traceEnt->client->playerTeam != self->client->playerTeam || self->enemy == traceEnt || traceEnt->enemy == self )
		{//an enemy or object
			int	dmg;
			if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
			{//more damage if closer and more in front
				dmg = 1;
				if ( dist < 100 )
				{
					dmg += 2;
				}
				else if ( dist < 200 )
				{
					dmg += 1;
				}
				if ( dot > 0.9f )
				{
					dmg += 2;
				}
				else if ( dot > 0.7f )
				{
					dmg += 1;
				}
			}
			else
			{
				dmg = Q_irand( 1, 3 );//*self->client->ps.forcePowerLevel[FP_LIGHTNING];
			}
			if ( traceEnt->client && traceEnt->health > 0 && ( traceEnt->client->NPC_class == CLASS_DESANN || traceEnt->client->NPC_class == CLASS_LUKE ) )
			{//Luke and Desann can shield themselves from the attack
				//FIXME: shield effect or something?
				int parts;
				if ( traceEnt->client->ps.groundEntityNum != ENTITYNUM_NONE && !PM_SpinningSaberAnim( traceEnt->client->ps.legsAnim ) && !PM_FlippingAnim( traceEnt->client->ps.legsAnim ) && !PM_RollingAnim( traceEnt->client->ps.legsAnim ) )
				{//if on a surface and not in a spin or flip, play full body resist
					parts = SETANIM_BOTH;
				}
				else
				{//play resist just in torso
					parts = SETANIM_TORSO;
				}
				NPC_SetAnim( traceEnt, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				Jedi_PlayDeflectSound( traceEnt );
				dmg = 0;
			}
			else if ( traceEnt->s.weapon == WP_SABER )
			{
				if ( Q_irand( 0, 1 ) )
				{//jedi less likely to be damaged
					dmg = 0;
				}
				else
				{
					dmg = 1;
				}
			}
			if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAK )
			{
				if ( traceEnt->client->ps.powerups[PW_GALAK_SHIELD] )
				{//has shield up
					dmg = 0;
				}
			}
			G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_ELECTROCUTE );
			if ( traceEnt->client )
			{
				if ( !Q_irand( 0, 2 ) )
				{
					G_Sound( traceEnt, G_SoundIndex( va( "sound/weapons/force/lightninghit%d.wav", Q_irand( 1, 3 ) ) ) );
				}
				traceEnt->s.powerups |= ( 1 << PW_SHOCKED );

				// If we are dead or we are a bot, we can do the full version
				class_t npc_class = traceEnt->client->NPC_class;
				if ( traceEnt->health <= 0 || ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE ||
					 npc_class == CLASS_MOUSE || npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_REMOTE ||
					 npc_class == CLASS_R5D2 || npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 ||
					 npc_class == CLASS_MARK2 || npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST ) ||
					 npc_class == CLASS_SENTRY )
				{
					traceEnt->client->ps.powerups[PW_SHOCKED] = level.time + 4000;
				}
				else //short version
				{
					traceEnt->client->ps.powerups[PW_SHOCKED] = level.time + 500;
				}
			}
		}
	}
}

void ForceShootLightning( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && cg.zoomMode )
	{//can't force lightning when zoomed in
		return;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	//FIXME: if lightning hits water, do water-only-flagged radius damage from that point
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = 512, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->currentOrigin, center );
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for ( e = 0 ; e < numListedEntities ; e++ )
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->owner == self && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ )
			{
				if ( center[i] < traceEnt->absmin[i] )
				{
					v[i] = traceEnt->absmin[i] - center[i];
				} else if ( center[i] > traceEnt->absmax[i] )
				{
					v[i] = center[i] - traceEnt->absmax[i];
				} else
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->absmax, traceEnt->absmin, size );
			VectorMA( traceEnt->absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius )
			{
				continue;
			}

			//in PVS?
			if ( !traceEnt->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.handLPoint ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace( &tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			//FIXME: maybe add up the ents and do more damage the less ents there are
			//		as if we're spreading out the damage?
			ForceLightningDamage( self, traceEnt, dir, dist, dot, ent_org );
		}

	}
	else
	{//trace-line
		int ignore = self->s.number;
		int traces = 0;
		vec3_t	start;

		VectorCopy( self->client->renderInfo.handLPoint, start );
		VectorMA( self->client->renderInfo.handLPoint, 2048, forward, end );

		while ( traces < 10 )
		{//need to loop this in case we hit a Jedi who dodges the shot
			gi.trace( &tr, start, vec3_origin, vec3_origin, end, ignore, MASK_SHOT, G2_RETURNONHIT, 0 );
			if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
			{
				return;
			}

			traceEnt = &g_entities[tr.entityNum];
			//NOTE: only NPCs do this auto-dodge
			if ( traceEnt && traceEnt->s.number && traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC
			{//FIXME: need a more reliable way to know we hit a jedi?
				if ( !Jedi_DodgeEvasion( traceEnt, self, &tr, HL_NONE ) )
				{//act like we didn't even hit him
					VectorCopy( tr.endpos, start );
					ignore = tr.entityNum;
					traces++;
					continue;
				}
			}
			//a Jedi is not dodging this shot
			break;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceLightningDamage( self, traceEnt, forward, 0, 0, tr.endpos );
	}
}

void ForceJumpCharge( gentity_t *self, usercmd_t *ucmd )
{
	float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && cg.zoomMode )
	{//can't force jump when zoomed in
		return;
	}

	//need to play sound
	if ( !self->client->ps.forceJumpCharge )
	{//FIXME: this should last only as long as the actual charge-up
		G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jumpbuild.wav" );
	}
	//Increment
	self->client->ps.forceJumpCharge += forceJumpChargeInterval;

	//clamp to max strength for current level
	if ( self->client->ps.forceJumpCharge > forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]] )
	{
		self->client->ps.forceJumpCharge = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]];
	}

	//clamp to max available force power
	if ( self->client->ps.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[FP_LEVITATION] > self->client->ps.forcePower )
	{//can't use more than you have
		self->client->ps.forceJumpCharge = self->client->ps.forcePower*forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);
	}
	//FIXME: a simple tap should always do at least a normal height's jump?
}

int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd )
{
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy( self->client->ps.viewangles, view );
	view[0] = 0;
	AngleVectors( view, forward, right, NULL );
	if ( ucmd->forwardmove && ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if ( ucmd->rightmove > 0 )
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if ( ucmd->forwardmove || ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 100;
		}
		else if ( ucmd->forwardmove < 0 )
		{
			pushFwd = -100;
		}
		else if ( ucmd->rightmove > 0 )
		{
			pushRt = 100;
		}
		else if ( ucmd->rightmove < 0 )
		{
			pushRt = -100;
		}
	}
	VectorMA( self->client->ps.velocity, pushFwd, forward, jumpVel );
	VectorMA( self->client->ps.velocity, pushRt, right, jumpVel );
	jumpVel[2] += self->client->ps.forceJumpCharge;//forceJumpStrength;
	if ( pushFwd > 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_FORWARD;
	}
	else if ( pushFwd < 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_BACKWARD;
	}
	else if ( pushRt > 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_RIGHT;
	}
	else if ( pushRt < 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_LEFT;
	}
	else
	{//FIXME: jump straight up anim
		return FJ_UP;
	}
}

void ForceJump( gentity_t *self, usercmd_t *ucmd )
{
	if ( self->client->ps.forcePowerDuration[FP_LEVITATION] > level.time )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_LEVITATION, 0 ) )
	{
		return;
	}
	if ( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return;
	}
	if ( self->client->ps.pm_flags&PMF_JUMP_HELD )
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force jump when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}

	G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );

	float forceJumpChargeInterval = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]]/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	int anim;
	vec3_t	jumpVel;

	switch( WP_GetVelocityForForceJump( self, jumpVel, ucmd ) )
	{
	case FJ_FORWARD:
		if ( self->NPC && self->NPC->rank != RANK_CREWMAN && self->NPC->rank <= RANK_LT_JG )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMP1;
		}
		else
		{
			anim = BOTH_FLIP_F;
		}
		break;
	case FJ_BACKWARD:
		if ( self->NPC && self->NPC->rank != RANK_CREWMAN && self->NPC->rank <= RANK_LT_JG )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPBACK1;
		}
		else
		{
			anim = BOTH_FLIP_B;
		}
		break;
	case FJ_RIGHT:
		if ( self->NPC && self->NPC->rank != RANK_CREWMAN && self->NPC->rank <= RANK_LT_JG )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPRIGHT1;
		}
		else
		{
			anim = BOTH_FLIP_R;
		}
		break;
	case FJ_LEFT:
		if ( self->NPC && self->NPC->rank != RANK_CREWMAN && self->NPC->rank <= RANK_LT_JG )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPLEFT1;
		}
		else
		{
			anim = BOTH_FLIP_L;
		}
		break;
	default:
	case FJ_UP:
		anim = BOTH_JUMP1;
		break;
	}

	int	parts = SETANIM_BOTH;
	if ( self->client->ps.weaponTime )
	{//FIXME: really only care if we're in a saber attack anim.. maybe trail length?
		parts = SETANIM_LEGS;
	}

	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	//FIXME: sound effect
	self->client->ps.forceJumpZStart = self->currentOrigin[2];//remember this for when we land
	VectorCopy( jumpVel, self->client->ps.velocity );
	//wasn't allowing them to attack when jumping, but that was annoying
	//self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

	WP_ForcePowerStart( self, FP_LEVITATION, self->client->ps.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[FP_LEVITATION] );
	//self->client->ps.forcePowerDuration[FP_LEVITATION] = level.time + self->client->ps.weaponTime;
	self->client->ps.forceJumpCharge = 0;
}

void WP_ForcePowerRegenerate( gentity_t *self, int overrideAmt )
{
	if ( !self->client )
	{
		return;
	}

	if ( self->client->ps.forcePower < self->client->ps.forcePowerMax )
	{
		if ( overrideAmt )
		{
			self->client->ps.forcePower += overrideAmt;
		}
		else
		{
			self->client->ps.forcePower++;
		}
		if ( self->client->ps.forcePower > self->client->ps.forcePowerMax )
		{
			self->client->ps.forcePower = self->client->ps.forcePowerMax;
		}
	}
}

void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	if ( self->NPC )
	{//For now, NPCs have infinite force power
		return;
	}
	//take away the power
	int	drain = overrideAmt;
	if ( !drain )
	{
		drain = forcePowerNeeded[forcePower];
	}
	if ( !drain )
	{
		return;
	}
	self->client->ps.forcePower -= drain;
	if ( self->client->ps.forcePower < 0 )
	{
		self->client->ps.forcePower = 0;
	}
}

void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	int	duration = 0;

	//FIXME: debounce some of these

	//and it in
	//set up duration time
	switch( (int)forcePower )
	{
	case FP_HEAL:
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.forceHealCount = 0;
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_SPEED:
		//duration is always 5 seconds, player time
		duration = ceil(FORCE_SPEED_DURATION*forceSpeedValue[self->client->ps.forcePowerLevel[FP_SPEED]]);//FIXME: because the timescale scales down (not instant), this doesn't end up being exactly right...
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/speedloop.wav" );
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		duration = 1000;
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_LIGHTNING:
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	default:
		break;
	}
	if ( duration )
	{
		self->client->ps.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.forcePowerDuration[forcePower] = 0;
	}
	self->client->ps.forcePowerDebounce[forcePower] = 0;

	WP_ForcePowerDrain( self, forcePower, overrideAmt );

	if ( !self->s.number )
	{
		self->client->sess.missionStats.forceUsed[(int)forcePower]++;
	}
}

qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	if ( forcePower == FP_LEVITATION )
	{
		return qtrue;
	}
	int	drain = overrideAmt?overrideAmt:forcePowerNeeded[forcePower];
	if ( !drain )
	{
		return qtrue;
	}
	if ( self->client->ps.forcePower < drain )
	{
		//G_AddEvent( self, EV_NOAMMO, 0 );
		return qfalse;
	}
	return qtrue;
}

extern void CG_PlayerLockedWeaponSpeech( int jumping );
qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{

	if ( !(self->client->ps.forcePowersKnown & ( 1 << forcePower )) )
	{//don't know this power
		return qfalse;
	}

	if ( self->client->ps.forcePowerLevel[forcePower] <= 0 )
	{//can't use this power
		return qfalse;
	}

	if( self->flags & FL_LOCK_PLAYER_WEAPONS ) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
	{
		CG_PlayerLockedWeaponSpeech( qfalse );
		return qfalse;
	}

	if ( (self->client->ps.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		return qfalse;
	}
	/*
	if ( !self->client->ps.forcePowerLevel[(int)(forcePower)] )
	{
		return qfalse;
	}
	*/
	if ( self->client->NPC_class == CLASS_ATST )
	{//Doh!  No force powers in an AT-ST!
		return qfalse;
	}
	if ( self->client->ps.vehicleModel != 0 )
	{//Doh!  No force powers when flying a vehicle!
		return qfalse;
	}
	if ( self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD )
	{//Doh!  No force powers when controlling an NPC
		return qfalse;
	}
	if ( self->client->ps.eFlags & EF_LOCKED_TO_WEAPON )
	{//Doh!  No force powers when in an emplaced gun!
		return qfalse;
	}

	return WP_ForcePowerAvailable( self, forcePower, overrideAmt );
}

void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower )
{
	gentity_t	*gripEnt;

	self->client->ps.forcePowersActive &= ~( 1 << forcePower );

	switch( (int)forcePower )
	{
	case FP_HEAL:
		//if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_3 )
		{//wasn't an instant heal and heal is now done
			if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 )
			{//if in meditation pose, must come out of it
				//FIXME: BOTH_FORCEHEAL_STOP
				if ( self->client->ps.legsAnim == BOTH_FORCEHEAL_START )
				{
					NPC_SetAnim( self, SETANIM_LEGS, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				if ( self->client->ps.torsoAnim == BOTH_FORCEHEAL_START )
				{
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
				self->client->ps.saberBlocked = BLOCKED_NONE;
			}
		}
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowerDebounce[FP_LEVITATION] = 0;
		break;
	case FP_SPEED:
		if ( !self->s.number )
		{//player using force speed
			if ( g_timescale->value != 1.0 )
			{
				gi.cvar_set("timescale", "1");
			}
		}
		//FIXME: reset my current anim, keeping current frame, but with proper anim speed
		//		otherwise, the anim will continue playing at high speed
		self->s.loopSound = 0;
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		if ( self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD )
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];
			if ( gripEnt )
			{
				gripEnt->s.loopSound = 0;
				if ( gripEnt->client )
				{
					gripEnt->client->ps.eFlags &= ~EF_FORCE_GRIPPED;
					if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//sanity-cap the velocity
						float gripVel = VectorNormalize( gripEnt->client->ps.velocity );
						if ( gripVel > 500.0f )
						{
							gripVel = 500.0f;
						}
						VectorScale( gripEnt->client->ps.velocity, gripVel, gripEnt->client->ps.velocity );
					}

					//FIXME: they probably dropped their weapon, should we make them flee?  Or should AI handle no-weapon behavior?
					if ( gripEnt->health > 0 )
					{
						int	holdTime = 500;
						G_AddEvent( gripEnt, EV_WATER_CLEAR, 0 );
						if ( gripEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time )
						{//they probably pushed out of it
							holdTime = 0;
						}
						else if ( gripEnt->s.weapon == WP_SABER )
						{//jedi recover faster
							holdTime = self->client->ps.forcePowerLevel[FP_GRIP]*200;
						}
						else
						{
							holdTime = self->client->ps.forcePowerLevel[FP_GRIP]*500;
						}
						//stop the anims soon, keep them locked in place for a bit
						if ( gripEnt->client->ps.torsoAnim == BOTH_CHOKE1 || gripEnt->client->ps.torsoAnim == BOTH_CHOKE3 )
						{//stop choking anim on torso
							if ( gripEnt->client->ps.torsoAnimTimer > holdTime )
							{
								gripEnt->client->ps.torsoAnimTimer = holdTime;
							}
						}
						if ( gripEnt->client->ps.legsAnim == BOTH_CHOKE1 || gripEnt->client->ps.legsAnim == BOTH_CHOKE3 )
						{//stop choking anim on legs
							gripEnt->client->ps.legsAnimTimer = 0;
							if ( holdTime )
							{
								//lock them in place for a bit
								gripEnt->client->ps.pm_time = gripEnt->client->ps.torsoAnimTimer;
								gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								if ( gripEnt->s.number )
								{//NPC
									gripEnt->painDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
								else
								{//player
									gripEnt->aimDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
							}
						}
						if ( gripEnt->NPC )
						{
							if ( !(gripEnt->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
							{//not falling to their death
								gripEnt->NPC->nextBStateThink = level.time + holdTime;
							}
							//if still alive after stopped gripping, let them wake others up
							G_AngerAlert( gripEnt );
						}
					}
				}
				else
				{
					gripEnt->s.eFlags &= ~EF_FORCE_GRIPPED;
					if ( gripEnt->s.eType == ET_MISSILE )
					{//continue normal movement
						if ( gripEnt->s.weapon == WP_THERMAL )
						{
							gripEnt->s.pos.trType = TR_INTERPOLATE;
						}
						else
						{
							gripEnt->s.pos.trType = TR_LINEAR;//FIXME: what about gravity-effected projectiles?
						}
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						gripEnt->s.pos.trTime = level.time;
					}
					else
					{//drop it
						gripEnt->e_ThinkFunc = thinkF_G_RunObject;
						gripEnt->nextthink = level.time + FRAMETIME;
						gripEnt->s.pos.trType = TR_GRAVITY;
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						gripEnt->s.pos.trTime = level.time;
					}
				}
			}
			self->s.loopSound = 0;
			self->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
		}
		if ( self->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD )
		{
			NPC_SetAnim( self, BOTH_FORCEGRIP_RELEASE, BOTH_FORCELIGHTNING_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		break;
	case FP_LIGHTNING:
		if ( self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START )
		{
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.forcePowerDebounce[FP_LIGHTNING] = level.time + 3000;//FIXME: define?
		}
		else
		{//stop the looping sound
			self->client->ps.forcePowerDebounce[FP_LIGHTNING] = level.time + 1000;//FIXME: define?
			self->s.loopSound = 0;
		}
		break;
	default:
		break;
	}
}

extern qboolean PM_ForceJumpingUp( gentity_t *gent );
static void WP_ForcePowerRun( gentity_t *self, forcePowers_t forcePower, usercmd_t *cmd )
{
	float				speed, newSpeed;
	gentity_t			*gripEnt;
	vec3_t				angles, dir, gripOrg, gripEntOrg;
	float				dist;
	extern usercmd_t	ucmd;

	switch( (int)forcePower )
	{
	case FP_HEAL:
		if ( self->client->ps.forceHealCount >= MAX_FORCE_HEAL || self->health >= self->client->ps.stats[STAT_MAX_HEALTH] )
		{//fully healed or used up all 25
			if ( !Q3_TaskIDPending( self, TID_CHAN_VOICE ) )
			{
				G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d.mp3", Q_irand( 1, 4 ) ) );
			}
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_3 && ( (cmd->buttons&BUTTON_ATTACK) || (cmd->buttons&BUTTON_ALT_ATTACK) || self->painDebounceTime > level.time || (self->client->ps.weaponTime&&self->client->ps.weapon!=WP_NONE) ) )
		{//attacked or was hit while healing...
			//stop healing
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 && ( cmd->rightmove || cmd->forwardmove || cmd->upmove > 0 ) )
		{//moved while healing... FIXME: also, in WP_ForcePowerStart, stop healing if any other force power is used
			//stop healing
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerDebounce[FP_HEAL] < level.time )
		{//time to heal again
			if ( WP_ForcePowerAvailable( self, forcePower, 4 ) )
			{//have available power
				self->health++;
				self->client->ps.forceHealCount++;
				if ( self->client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2 )
				{
					self->client->ps.forcePowerDebounce[FP_HEAL] = level.time + 50;
				}
				else
				{
					self->client->ps.forcePowerDebounce[FP_HEAL] = level.time + FORCE_HEAL_INTERVAL;
				}
				WP_ForcePowerDrain( self, forcePower, 4 );
			}
			else
			{//stop
				WP_ForcePowerStop( self, forcePower );
			}
		}
		break;
	case FP_LEVITATION:
		if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceJumpZStart )
		{//done with jump
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			if ( PM_ForceJumpingUp( self ) )
			{//holding jump in air
				if ( cmd->upmove > 10 )
				{//still trying to go up
					if ( WP_ForcePowerAvailable( self, FP_LEVITATION, 1 ) )
					{
						if ( self->client->ps.forcePowerDebounce[FP_LEVITATION] < level.time )
						{
							WP_ForcePowerDrain( self, FP_LEVITATION, 5 );
							self->client->ps.forcePowerDebounce[FP_LEVITATION] = level.time + 100;
						}
						self->client->ps.forcePowersActive |= ( 1 << FP_LEVITATION );
						self->client->ps.forceJumpCharge = 1;//just used as a flag for the player, cleared when he lands
					}
					else
					{//cut the jump short
						WP_ForcePowerStop( self, forcePower );
					}
				}
				else
				{//cut the jump short
					WP_ForcePowerStop( self, forcePower );
				}
			}
			else
			{
				WP_ForcePowerStop( self, forcePower );
			}
		}
		break;
	case FP_SPEED:
		speed = forceSpeedValue[self->client->ps.forcePowerLevel[FP_SPEED]];
		if ( !self->s.number )
		{//player using force speed
			gi.cvar_set("timescale", va("%4.2f", speed));
			if ( g_timescale->value > speed )
			{
				newSpeed = g_timescale->value - 0.05;
				if ( newSpeed < speed )
				{
					newSpeed = speed;
				}
				gi.cvar_set("timescale", va("%4.2f", newSpeed));
			}
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		if ( !WP_ForcePowerAvailable( self, FP_GRIP, 0 )
			|| (self->client->ps.forcePowerLevel[FP_GRIP]>FORCE_LEVEL_1&&!self->s.number&&!(cmd->buttons&BUTTON_FORCEGRIP)) )
		{
			WP_ForcePowerStop( self, FP_GRIP );
			return;
		}
		else if ( self->client->ps.forceGripEntityNum >= 0 && self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD )
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];

			if ( !gripEnt || (gripEnt->health <= 0&&gripEnt->takedamage) )//FIXME: what about things that never had health or lose takedamage when they die?
			{//either invalid ent, or dead ent
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_1
				&& gripEnt->client
				&& gripEnt->client->ps.groundEntityNum == ENTITYNUM_NONE )
			{
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( gripEnt->s.weapon == WP_SABER && gripEnt->NPC && gripEnt->client && gripEnt->client->ps.forcePowersKnown&(1<<FP_PUSH) && !Q_irand( 0, 100-(gripEnt->NPC->stats.evasion*10)-(g_spskill->integer*10) ) )
			{//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
				ForceThrow( gripEnt, qfalse );
				//FIXME: I need to go into some pushed back anim...
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else
			{
				if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
				{//holding it
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				//get their org
				VectorCopy( self->client->ps.viewangles, angles );
				angles[0] -= 10;
				AngleVectors( angles, dir, NULL, NULL );
				if ( gripEnt->client )
				{//move
					VectorCopy( gripEnt->client->renderInfo.headPoint, gripEntOrg );
				}
				else
				{
					VectorCopy( gripEnt->currentOrigin, gripEntOrg );
				}

				//how far are they
				dist = Distance( self->client->renderInfo.handLPoint, gripEntOrg );
				if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2 &&
					(!InFront( gripEntOrg, self->client->renderInfo.handLPoint, self->client->ps.viewangles, 0.3f ) ||
						DistanceSquared( gripEntOrg, self->client->renderInfo.handLPoint ) > FORCE_GRIP_DIST_SQUARED))
				{//must face them
					WP_ForcePowerStop( self, FP_GRIP );
					return;
				}

				//check for lift or carry
				if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
				{//carry
					//cap dist
					if ( dist > 256 )
					{
						dist = 256;
					}
					else if ( dist < 128 )
					{
						dist = 128;
					}
					VectorMA( self->client->renderInfo.handLPoint, dist, dir, gripOrg );
				}
				else if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
				{//just lift
					VectorCopy( self->client->ps.forceGripOrg, gripOrg );
				}
				else
				{
					VectorCopy( gripEnt->currentOrigin, gripOrg );
				}
				//now move them
				if ( gripEnt->client )
				{
					if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//level 1 just holds them
						VectorSubtract( gripOrg, gripEntOrg, gripEnt->client->ps.velocity );
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
						{//level 2 just lifts them
							float gripDist = VectorNormalize( gripEnt->client->ps.velocity )/3.0f;
							if ( gripDist < 5.0f )
							{
								gripDist = 5.0f;
							}
							VectorScale( gripEnt->client->ps.velocity, (gripDist*gripDist), gripEnt->client->ps.velocity );
						}
					}
					//stop them from thinking
					gripEnt->client->ps.pm_time = 2000;
					gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
					if ( gripEnt->NPC )
					{
						if ( !(gripEnt->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
						{//not falling to their death
							gripEnt->NPC->nextBStateThink = level.time + 2000;
						}
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
						{//level 1 just holds them
							vectoangles( dir, angles );
							gripEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW]+180);
							gripEnt->NPC->desiredPitch = -angles[PITCH];
							SaveNPCGlobals();
							SetNPCGlobals( gripEnt );
							NPC_UpdateAngles( qtrue, qtrue );
							gripEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
							gripEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
							gripEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
							RestoreNPCGlobals();
							//FIXME: why does he turn back to his original angles once he dies or is let go?
						}
					}
					else if ( !gripEnt->s.number )
					{
						//vectoangles( dir, angles );
						//gripEnt->client->ps.viewangles[0] = -angles[0];
						//gripEnt->client->ps.viewangles[1] = AngleNormalize180(angles[YAW]+180);
						gripEnt->enemy = self;
						NPC_SetLookTarget( gripEnt, self->s.number, level.time+1000 );
					}

					gripEnt->client->ps.eFlags |= EF_FORCE_GRIPPED;
					//dammit!  Make sure that saber stays off!
					gripEnt->client->ps.saberActive = qfalse;
				}
				else
				{//move
					if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//level 1 just holds them
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						VectorSubtract( gripOrg, gripEntOrg, gripEnt->s.pos.trDelta );
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
						{//level 2 just lifts them
							VectorScale( gripEnt->s.pos.trDelta, 10, gripEnt->s.pos.trDelta );
						}
						gripEnt->s.pos.trType = TR_LINEAR;
						gripEnt->s.pos.trTime = level.time;
					}

					gripEnt->s.eFlags |= EF_FORCE_GRIPPED;
				}

				//Shouldn't this be discovered?
				//AddSightEvent( self, gripOrg, 128, AEL_DANGER, 20 );
				AddSightEvent( self, gripOrg, 128, AEL_DISCOVERED, 20 );

				if ( self->client->ps.forcePowerDebounce[FP_GRIP] < level.time )
				{
					//GEntity_PainFunc( gripEnt, self, self, gripOrg, 0, MOD_CRUSH );
					gripEnt->painDebounceTime = 0;
					G_Damage( gripEnt, self, self, dir, gripOrg, forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]], DAMAGE_NO_ARMOR, MOD_CRUSH );//MOD_???
					if ( gripEnt->s.number )
					{
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
						{//do damage faster at level 3
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 150, 750 );
						}
						else
						{
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 250, 1000 );
						}
					}
					else
					{//player takes damage faster
						self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 100, 600 );
					}
					if ( forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]] > 0 )
					{//no damage at level 1
						WP_ForcePowerDrain( self, FP_GRIP, 3 );
					}
				}
				else
				{
					//WP_ForcePowerDrain( self, FP_GRIP, 0 );
					if ( !gripEnt->enemy )
					{
						G_SetEnemy( gripEnt, self );
					}
				}
				if ( gripEnt->client && gripEnt->health > 0 )
				{
					int anim = BOTH_CHOKE3; //left-handed choke
					if ( gripEnt->client->ps.weapon == WP_NONE || gripEnt->client->ps.weapon == WP_MELEE )
					{
						anim = BOTH_CHOKE1; //two-handed choke
					}
					if ( self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
					{//still on ground, only set anim on torso
						NPC_SetAnim( gripEnt, SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					else
					{//in air, set on whole body
						NPC_SetAnim( gripEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					gripEnt->painDebounceTime = level.time + 2000;
				}
			}
		}
		break;
	case FP_LIGHTNING:
		if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( cmd->buttons & BUTTON_FORCE_LIGHTNING )
			{//holding it keeps it going
				self->client->ps.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
				if ( self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START )
				{
					if ( !self->client->ps.torsoAnimTimer )
					{
						NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
				}
				else
				{
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
			}
		}
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) )
		{
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			ForceShootLightning( self );
			WP_ForcePowerDrain( self, forcePower, 0 );
		}
		break;
	default:
		break;
	}
}

void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd )
{
	qboolean	usingForce = qfalse;
	int			i;
	//see if any force powers are running
	if ( !self )
	{
		return;
	}
	if ( !self->client )
	{
		return;
	}

	if ( self->health <= 0 )
	{//if dead, deactivate any active force powers
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( self->client->ps.forcePowerDuration[i] || (self->client->ps.forcePowersActive&( 1 << i )) )
			{
				WP_ForcePowerStop( self, (forcePowers_t)i );
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		return;
	}

	if ( !self->s.number )
	{//player uses different kind of force-jump
	}
	else
	{
		/*
		if ( ucmd->buttons & BUTTON_FORCEJUMP )
		{//just charging up
			ForceJumpCharge( self, ucmd );
		}
		else */
		if ( self->client->ps.forceJumpCharge )
		{//let go of charge button, have charge
			//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.
			if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& !PM_SwimmingAnim( self->client->ps.legsAnim ) )
			{//FIXME: stop sound?
				//self->client->ps.forceJumpCharge = 0;
				//FIXME: actually, we want this to still be cleared... don't clear it if the button isn't being pressed, but clear it if not holding button and not on ground.
			}
			else
			{//still on ground, so jump
				ForceJump( self, ucmd );
				return;
			}
		}
	}

	if ( ucmd->buttons & BUTTON_FORCEGRIP )
	{
		ForceGrip( self );
	}

	if ( ucmd->buttons & BUTTON_FORCE_LIGHTNING )
	{
		ForceLightning( self );
	}

	for ( i = 0; i < NUM_FORCE_POWERS; i++ )
	{
		if ( self->client->ps.forcePowerDuration[i] )
		{
			if ( self->client->ps.forcePowerDuration[i] < level.time )
			{
				if ( (self->client->ps.forcePowersActive&( 1 << i )) )
				{//turn it off
					WP_ForcePowerStop( self, (forcePowers_t)i );
				}
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		if ( (self->client->ps.forcePowersActive&( 1 << i )) )
		{
			usingForce = qtrue;
			WP_ForcePowerRun( self, (forcePowers_t)i, ucmd );
		}
	}
	if ( self->client->ps.saberInFlight )
	{//don't regen force power while throwing saber
		if ( self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( &g_entities[self->client->ps.saberEntityNum] != NULL && g_entities[self->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR )
			{//fell to the ground and we're trying to pull it back
				usingForce = qtrue;
			}
		}
	}
	if ( !usingForce )
	{//when not using the force, regenerate at 10 points per second
		if ( self->client->ps.forcePowerRegenDebounceTime < level.time )
		{
			WP_ForcePowerRegenerate( self, 0 );
			self->client->ps.forcePowerRegenDebounceTime = level.time + 100;
		}
	}
}

void WP_InitForcePowers( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return;
	}

	if( ent->client->NPC_class == CLASS_TAVION ||
		ent->client->NPC_class == CLASS_REBORN ||
		ent->client->NPC_class == CLASS_DESANN ||
		ent->client->NPC_class == CLASS_SHADOWTROOPER ||
		ent->client->NPC_class == CLASS_JEDI ||
		ent->client->NPC_class == CLASS_LUKE )
	{//an NPC jedi
		ent->client->ps.forcePower = ent->client->ps.forcePowerMax = FORCE_POWER_MAX;
		ent->client->ps.forcePowerRegenDebounceTime = 0;
		ent->client->ps.forceGripEntityNum = ENTITYNUM_NONE;

		if ( ent->client->NPC_class == CLASS_DESANN )
		{
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_GRIP )|( 1 << FP_LIGHTNING)|( 1 << FP_SABERTHROW)|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
		}
		else if ( ent->client->NPC_class == CLASS_LUKE )
		{
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_SABERTHROW)|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
		}
		else if ( ent->client->NPC_class == CLASS_TAVION || ( ent->client->NPC_class == CLASS_JEDI && ent->NPC->rank == RANK_COMMANDER ) )
		{//Tavia or trainer Jedi
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_SABERTHROW)|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			if ( ent->client->NPC_class == CLASS_TAVION )
			{
				ent->client->ps.forcePowersKnown |= ( 1 << FP_LIGHTNING)|( 1 << FP_GRIP );
				ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_2;
				ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;
			}
		}
		else if ( ent->client->NPC_class == CLASS_SHADOWTROOPER )
		{//Shadow Trooper
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_SABERTHROW)|( 1 << FP_GRIP )|( 1 << FP_LIGHTNING)|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
		}
		else if ( ent->NPC->rank == RANK_LT || ent->client->NPC_class == CLASS_JEDI )
		{//Reborn Boss or ally Jedi
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_SABERTHROW)|( 1 << FP_GRIP )|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			if ( ent->client->NPC_class != CLASS_JEDI )
			{
				ent->client->ps.forcePowersKnown |= ( 1 << FP_GRIP );
				ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;
			}
		}
		else if ( ent->NPC->rank == RANK_LT_JG )
		{//Reborn Fencer
			ent->client->ps.forcePowersKnown = ( 1 << FP_PUSH )|( 1 << FP_SABERTHROW )|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_2;//FIXME: maybe make him only use it in defense- to throw away grenades
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_2;
		}
		else if ( ent->NPC->rank == RANK_ENSIGN )
		{//Force User
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
		}
		else if ( ent->NPC->rank == RANK_CREWMAN )
		{//Acrobat
			ent->client->ps.forcePowersKnown = ( 1 << FP_LEVITATION )|( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
		}
		else if ( ent->NPC->rank == RANK_CIVILIAN )
		{//Grunt (NOTE: grunt turns slower and has less health)
			ent->client->ps.forcePowersKnown = ( 1 << FP_SPEED)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
	}
	else
	{//player
		ent->client->ps.forcePowersKnown = ( 1 << FP_HEAL )|( 1 << FP_LEVITATION )|( 1 << FP_SPEED )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_TELEPATHY )|( 1 << FP_GRIP )|( 1 << FP_LIGHTNING)|( 1 << FP_SABERTHROW)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE );
		ent->client->ps.forcePower = ent->client->ps.forcePowerMax = FORCE_POWER_MAX;
		ent->client->ps.forcePowerRegenDebounceTime = 0;
		ent->client->ps.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_2;
		ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
		ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_1;
		ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_1;
		ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
		ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_2;
		ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_1;
		ent->client->ps.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_2;
		ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
		ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
		if ( ent->NPC )
		{//???
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_3;
		}
		else
		{
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;
		}
		ent->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
	}
}
