/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"
#include "g_functions.h"
#include "anims.h"
#include "b_local.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "w_local.h"
#include "../cgame/cg_local.h"

vec3_t	forwardVec, vrightVec, up;
vec3_t	muzzle;

gentity_t *ent_list[MAX_GENTITIES];
extern cvar_t	*g_debugMelee;

// some naughty little things that are used cg side
int g_rocketLockEntNum = ENTITYNUM_NONE;
int g_rocketLockTime = 0;
int	g_rocketSlackTime = 0;

// Weapon Helper Functions
float weaponSpeed[WP_NUM_WEAPONS][2] =
{
	{ 0,0 },//WP_NONE,
	{ 0,0 },//WP_SABER,				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
	{ BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL },//WP_BLASTER_PISTOL,
	{ BLASTER_VELOCITY,BLASTER_VELOCITY },//WP_BLASTER,
	{ Q3_INFINITE,Q3_INFINITE },//WP_DISRUPTOR,
	{ BOWCASTER_VELOCITY,BOWCASTER_VELOCITY },//WP_BOWCASTER,
	{ REPEATER_VELOCITY,REPEATER_ALT_VELOCITY },//WP_REPEATER,
	{ DEMP2_VELOCITY,DEMP2_ALT_RANGE },//WP_DEMP2,
	{ FLECHETTE_VEL,FLECHETTE_MINE_VEL },//WP_FLECHETTE,
	{ ROCKET_VELOCITY,ROCKET_ALT_VELOCITY },//WP_ROCKET_LAUNCHER,
	{ TD_VELOCITY,TD_ALT_VELOCITY },//WP_THERMAL,
	{ 0,0 },//WP_TRIP_MINE,
	{ 0,0 },//WP_DET_PACK,
	{ CONC_VELOCITY,Q3_INFINITE },//WP_CONCUSSION,
	{ 0,0 },//WP_MELEE,			// Any ol' melee attack
	{ 0,0 },//WP_STUN_BATON,
	{ BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL },//WP_BRYAR_PISTOL,
	{ EMPLACED_VEL,EMPLACED_VEL },//WP_EMPLACED_GUN,
	{ BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL },//WP_BOT_LASER,		// Probe droid	- Laser blast
	{ 0,0 },//WP_TURRET,			// turret guns
	{ ATST_MAIN_VEL,ATST_MAIN_VEL },//WP_ATST_MAIN,
	{ ATST_SIDE_MAIN_VELOCITY,ATST_SIDE_ALT_NPC_VELOCITY },//WP_ATST_SIDE,
	{ EMPLACED_VEL,EMPLACED_VEL },//WP_TIE_FIGHTER,
	{ EMPLACED_VEL,REPEATER_ALT_VELOCITY },//WP_RAPID_FIRE_CONC,
	{ 0,0 },//WP_JAWA,
	{ TUSKEN_RIFLE_VEL,TUSKEN_RIFLE_VEL },//WP_TUSKEN_RIFLE,
	{ 0,0 },//WP_TUSKEN_STAFF,
	{ 0,0 },//WP_SCEPTER,
	{ 0,0 },//WP_NOGHRI_STICK,
};

float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire )
{
	if ( alt_fire )
	{
		return weaponSpeed[wp][1];
	}
	return weaponSpeed[wp][0];
}

//-----------------------------------------------------------------------------
void WP_TraceSetStart( const gentity_t *ent, vec3_t start, const vec3_t mins, const vec3_t maxs )
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t	tr;
	vec3_t	entMins, newstart;
	vec3_t	entMaxs;

	VectorSet( entMaxs, 5, 5, 5 );
	VectorScale( entMaxs, -1, entMins );

	if ( !ent->client )
	{
		return;
	}

	VectorCopy( ent->currentOrigin, newstart );
	newstart[2] = start[2]; // force newstart to be on the same plane as the muzzle ( start )

	gi.trace( &tr, newstart, entMins, entMaxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP, (EG2_Collision)0, 0 );

	if ( tr.startsolid || tr.allsolid )
	{
		// there is a problem here..
		return;
	}

	if ( tr.fraction < 1.0f )
	{
		VectorCopy( tr.endpos, start );
	}
}

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, gentity_t *owner, qboolean altFire )
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn();

	missile->nextthink = level.time + life;
	missile->e_ThinkFunc = thinkF_G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->owner = owner;

	Vehicle_t*	pVeh = G_IsRidingVehicle(owner);

	missile->alt_fire = altFire;

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - 10;	// move a bit on the very first frame
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
 	if (pVeh)
	{
 		missile->s.eFlags |= EF_USE_ANGLEDELTA;
	 	vectoangles(missile->s.pos.trDelta, missile->s.angles);
	  	VectorMA(missile->s.pos.trDelta, 2.0f, pVeh->m_pParentEntity->client->ps.velocity, missile->s.pos.trDelta);
	}

	VectorCopy( org, missile->currentOrigin);
	gi.linkentity( missile );

	return missile;
}


//-----------------------------------------------------------------------------
void WP_Stick( gentity_t *missile, trace_t *trace, float fudge_distance )
//-----------------------------------------------------------------------------
{
	vec3_t org, ang;

	// not moving or rotating
	missile->s.pos.trType = TR_STATIONARY;
	VectorClear( missile->s.pos.trDelta );
	VectorClear( missile->s.apos.trDelta );

	// so we don't stick into the wall
	VectorMA( trace->endpos, fudge_distance, trace->plane.normal, org );
	G_SetOrigin( missile, org );

	vectoangles( trace->plane.normal, ang );
	G_SetAngles( missile, ang );

	// I guess explode death wants me as the normal?
//	VectorCopy( trace->plane.normal, missile->pos1 );
	gi.linkentity( missile );
}

// This version shares is in the thinkFunc format
//-----------------------------------------------------------------------------
void WP_Explode( gentity_t *self )
//-----------------------------------------------------------------------------
{
	gentity_t	*attacker = self;
	vec3_t		forwardVec={0,0,1};

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	if ( !self->client )
	{
	AngleVectors( self->s.angles, forwardVec, NULL, NULL );
	}

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, forwardVec );
	}

	if ( self->owner )
	{
		attacker = self->owner;
	}
	else if ( self->activator )
	{
		attacker = self->activator;
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, 0/*don't ignore attacker*/, MOD_EXPLOSIVE_SPLASH );
	}

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_SetOrigin( self, self->currentOrigin );

	self->nextthink = level.time + 50;
	self->e_ThinkFunc = thinkF_G_FreeEntity;
}

// We need to have a dieFunc, otherwise G_Damage won't actually make us die.  I could modify G_Damage, but that entails too many changes
//-----------------------------------------------------------------------------
void WP_ExplosiveDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
//-----------------------------------------------------------------------------
{
	self->enemy = attacker;

	if ( attacker && !attacker->s.number )
	{
		// less damage when shot by player
		self->splashDamage /= 3;
		self->splashRadius /= 3;
	}

	self->s.eFlags &= ~EF_FIRING; // don't draw beam if we are dead

	WP_Explode( self );
}

bool WP_MissileTargetHint(gentity_t* shooter, vec3_t start, vec3_t out)
{
	return false;
}

int G_GetHitLocFromTrace( trace_t *trace, int mod )
{
	int hitLoc = HL_NONE;
	for (int i=0; i < MAX_G2_COLLISIONS; i++)
	{
		if ( trace->G2CollisionMap[i].mEntityNum == -1 )
		{
			break;
		}

		CCollisionRecord &coll = trace->G2CollisionMap[i];
		if ( (coll.mFlags & G2_FRONTFACE) )
		{
			G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex ), &hitLoc, coll.mCollisionPosition, NULL, NULL, mod );
			//we only want the first "entrance wound", so break
			break;
		}
	}
	return hitLoc;
}

//---------------------------------------------------------
void AddLeanOfs(const gentity_t *const ent, vec3_t point)
//---------------------------------------------------------
{
	if(ent->client)
	{
		if(ent->client->ps.leanofs)
		{
			vec3_t	right;
			//add leaning offset
			AngleVectors(ent->client->ps.viewangles, NULL, right, NULL);
			VectorMA(point, (float)ent->client->ps.leanofs, right, point);
		}
	}
}

//---------------------------------------------------------
void SubtractLeanOfs(const gentity_t *const ent, vec3_t point)
//---------------------------------------------------------
{
	if(ent->client)
	{
		if(ent->client->ps.leanofs)
		{
			vec3_t	right;
			//add leaning offset
			AngleVectors( ent->client->ps.viewangles, NULL, right, NULL );
			VectorMA( point, ent->client->ps.leanofs*-1, right, point );
		}
	}
}

//---------------------------------------------------------
void ViewHeightFix(const gentity_t *const ent)
//---------------------------------------------------------
{
	//FIXME: this is hacky and doesn't need to be here.  Was only put here to make up
	//for the times a crouch anim would be used but not actually crouching.
	//When we start calcing eyepos (SPOT_HEAD) from the tag_eyes, we won't need
	//this (or viewheight at all?)
	if ( !ent )
		return;

	if ( !ent->client || !ent->NPC )
		return;

	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 )
		return;//dead

	if ( ent->client->ps.legsAnim == BOTH_CROUCH1IDLE || ent->client->ps.legsAnim == BOTH_CROUCH1 || ent->client->ps.legsAnim == BOTH_CROUCH1WALK )
	{
		if ( ent->client->ps.viewheight!=ent->client->crouchheight + STANDARD_VIEWHEIGHT_OFFSET )
			ent->client->ps.viewheight = ent->client->crouchheight + STANDARD_VIEWHEIGHT_OFFSET;
	}
	else
	{
		if ( ent->client->ps.viewheight!=ent->client->standheight + STANDARD_VIEWHEIGHT_OFFSET )
			ent->client->ps.viewheight = ent->client->standheight + STANDARD_VIEWHEIGHT_OFFSET;
	}
}

qboolean W_AccuracyLoggableWeapon( int weapon, qboolean alt_fire, int mod )
{
	if ( mod != MOD_UNKNOWN )
	{
		switch( mod )
		{
		//standard weapons
		case MOD_BRYAR:
		case MOD_BRYAR_ALT:
		case MOD_BLASTER:
		case MOD_BLASTER_ALT:
		case MOD_DISRUPTOR:
		case MOD_SNIPER:
		case MOD_BOWCASTER:
		case MOD_BOWCASTER_ALT:
		case MOD_ROCKET:
		case MOD_ROCKET_ALT:
		case MOD_CONC:
		case MOD_CONC_ALT:
			return qtrue;
			break;
		//non-alt standard
		case MOD_REPEATER:
		case MOD_DEMP2:
		case MOD_FLECHETTE:
			return qtrue;
			break;
		//emplaced gun
		case MOD_EMPLACED:
			return qtrue;
			break;
		//atst
		case MOD_ENERGY:
		case MOD_EXPLOSIVE:
			if ( weapon == WP_ATST_MAIN || weapon == WP_ATST_SIDE )
			{
				return qtrue;
			}
			break;
		}
	}
	else if ( weapon != WP_NONE )
	{
		switch( weapon )
		{
		case WP_BRYAR_PISTOL:
		case WP_BLASTER_PISTOL:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_ROCKET_LAUNCHER:
		case WP_CONCUSSION:
			return qtrue;
			break;
		//non-alt standard
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
			if ( !alt_fire )
			{
				return qtrue;
			}
			break;
		//emplaced gun
		case WP_EMPLACED_GUN:
			return qtrue;
			break;
		//atst
		case WP_ATST_MAIN:
		case WP_ATST_SIDE:
			return qtrue;
			break;
		}
	}
	return qfalse;
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}

//---------------------------------------------------------
void CalcMuzzlePoint( gentity_t *const ent, vec3_t forwardVec, vec3_t right, vec3_t up, vec3_t muzzlePoint, float lead_in )
//---------------------------------------------------------
{
	vec3_t		org;
	mdxaBone_t	boltMatrix;

	if( !lead_in ) //&& ent->s.number != 0
	{//Not players or melee
		if( ent->client )
		{
			if ( ent->client->renderInfo.mPCalcTime >= level.time - FRAMETIME*2 )
			{//Our muzz point was calced no more than 2 frames ago
				VectorCopy(ent->client->renderInfo.muzzlePoint, muzzlePoint);
				return;
			}
		}
	}

	VectorCopy( ent->currentOrigin, muzzlePoint );

	switch( ent->s.weapon )
	{
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 16;
		VectorMA( muzzlePoint, 28, forwardVec, muzzlePoint );
		VectorMA( muzzlePoint, 6, vrightVec, muzzlePoint );
		break;

	case WP_ROCKET_LAUNCHER:
	case WP_CONCUSSION:
	case WP_THERMAL:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 2;
		break;

	case WP_BLASTER:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 1;
		if ( ent->s.number == 0 )
			VectorMA( muzzlePoint, 12, forwardVec, muzzlePoint ); // player, don't set this any lower otherwise the projectile will impact immediately when your back is to a wall
		else
			VectorMA( muzzlePoint, 2, forwardVec, muzzlePoint ); // NPC, don't set too far forwardVec otherwise the projectile can go through doors

		VectorMA( muzzlePoint, 1, vrightVec, muzzlePoint );
		break;

	case WP_SABER:
		if(ent->NPC!=NULL &&
			(ent->client->ps.torsoAnim == TORSO_WEAPONREADY2 ||
			ent->client->ps.torsoAnim == BOTH_ATTACK2))//Sniper pose
		{
			ViewHeightFix(ent);
			muzzle[2] += ent->client->ps.viewheight;//By eyes
		}
		else
		{
			muzzlePoint[2] += 16;
		}
		VectorMA( muzzlePoint, 8, forwardVec, muzzlePoint );
		VectorMA( muzzlePoint, 16, vrightVec, muzzlePoint );
		break;

	case WP_BOT_LASER:
		muzzlePoint[2] -= 16;	//
		break;
	case WP_ATST_MAIN:

		if (ent->count > 0)
		{
			ent->count = 0;
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel,
						ent->handLBolt,
						&boltMatrix, ent->s.angles, ent->s.origin, (cg.time?cg.time:level.time),
						NULL, ent->s.modelScale );
		}
		else
		{
			ent->count = 1;
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel,
						ent->handRBolt,
						&boltMatrix, ent->s.angles, ent->s.origin, (cg.time?cg.time:level.time),
						NULL, ent->s.modelScale );
		}

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );

		VectorCopy(org,muzzlePoint);

		break;
	}

	AddLeanOfs(ent, muzzlePoint);
}

// Muzzle point table...
vec3_t WP_MuzzlePoint[WP_NUM_WEAPONS] =
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{8	,	16,		0	},	// WP_SABER,
	{12,	6,		-6	},	// WP_BLASTER_PISTOL,
	{12,	6,		-6	},	// WP_BLASTER,
	{12,	6,		-6	},	// WP_DISRUPTOR,
	{12,	2,		-6	},	// WP_BOWCASTER,
	{12,	4.5,	-6	},	// WP_REPEATER,
	{12,	6,		-6	},	// WP_DEMP2,
	{12,	6,		-6	},	// WP_FLECHETTE,
	{12,	8,		-4	},	// WP_ROCKET_LAUNCHER,
	{12,	0,		-4	},	// WP_THERMAL,
	{12,	0,		-10	},	// WP_TRIP_MINE,
	{12,	0,		-4	},	// WP_DET_PACK,
	{12,	8,		-4	},	// WP_CONCUSSION,
	{0	,	8,		0	},	// WP_MELEE,
	{0,		0,		0	},	// WP_ATST_MAIN,
	{0,		0,		0	},	// WP_ATST_SIDE,
	{0	,	8,		0	},	// WP_STUN_BATON,
	{12,	6,		-6	},	// WP_BRYAR_PISTOL,
};

void WP_RocketLock( gentity_t *ent, float lockDist )
{
	// Not really a charge weapon, but we still want to delay fire until the button comes up so that we can
	//	implement our alt-fire locking stuff
	vec3_t		ang;
	trace_t		tr;

	vec3_t muzzleOffPoint, muzzlePoint, forwardVec, right, up;

	AngleVectors( ent->client->ps.viewangles, forwardVec, right, up );

	AngleVectors(ent->client->ps.viewangles, ang, NULL, NULL);

	VectorCopy( ent->client->ps.origin, muzzlePoint );
	VectorCopy(WP_MuzzlePoint[WP_ROCKET_LAUNCHER], muzzleOffPoint);

	VectorMA(muzzlePoint, muzzleOffPoint[0], forwardVec, muzzlePoint);
	VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight + muzzleOffPoint[2];

	ang[0] = muzzlePoint[0] + ang[0]*lockDist;
	ang[1] = muzzlePoint[1] + ang[1]*lockDist;
	ang[2] = muzzlePoint[2] + ang[2]*lockDist;

	gi.trace(&tr, muzzlePoint, NULL, NULL, ang, ent->client->ps.clientNum, MASK_PLAYERSOLID, (EG2_Collision)0, 0);

	if (tr.fraction != 1 && tr.entityNum < ENTITYNUM_NONE && tr.entityNum != ent->client->ps.clientNum)
	{
		gentity_t *bgEnt = &g_entities[tr.entityNum];
		if ( bgEnt && (bgEnt->s.powerups&PW_CLOAKED) )
		{
			ent->client->rocketLockIndex = ENTITYNUM_NONE;
			ent->client->rocketLockTime = 0;
		}
		else if (bgEnt && bgEnt->s.eType == ET_PLAYER )
		{
			if (ent->client->rocketLockIndex == ENTITYNUM_NONE)
			{
				ent->client->rocketLockIndex = tr.entityNum;
				ent->client->rocketLockTime = level.time;
			}
			else if (ent->client->rocketLockIndex != tr.entityNum && ent->client->rocketTargetTime < level.time)
			{
				ent->client->rocketLockIndex = tr.entityNum;
				ent->client->rocketLockTime = level.time;
			}
			else if (ent->client->rocketLockIndex == tr.entityNum)
			{
				if (ent->client->rocketLockTime == -1)
				{
					ent->client->rocketLockTime = ent->client->rocketLastValidTime;
				}
			}

			if (ent->client->rocketLockIndex == tr.entityNum)
			{
				ent->client->rocketTargetTime = level.time + 500;
			}
		}
	}
	else if (ent->client->rocketTargetTime < level.time)
	{
		ent->client->rocketLockIndex = ENTITYNUM_NONE;
		ent->client->rocketLockTime = 0;
	}
	else
	{
		if (ent->client->rocketLockTime != -1)
		{
			ent->client->rocketLastValidTime = ent->client->rocketLockTime;
		}
		ent->client->rocketLockTime = -1;
	}
}

#define VEH_HOMING_MISSILE_THINK_TIME		100
void WP_FireVehicleWeapon( gentity_t *ent, vec3_t start, vec3_t dir, vehWeaponInfo_t *vehWeapon )
{
	if ( !vehWeapon )
	{//invalid vehicle weapon
		return;
	}
	else if ( vehWeapon->bIsProjectile )
	{//projectile entity
		gentity_t	*missile;
		vec3_t		mins, maxs;

		VectorSet( maxs, vehWeapon->fWidth/2.0f,vehWeapon->fWidth/2.0f,vehWeapon->fHeight/2.0f );
		VectorScale( maxs, -1, mins );

		//make sure our start point isn't on the other side of a wall
		WP_TraceSetStart( ent, start, mins, maxs );

		//QUERY: alt_fire true or not?  Does it matter?
		missile = CreateMissile( start, dir, vehWeapon->fSpeed, 10000, ent, qfalse );
		if ( vehWeapon->bHasGravity )
		{//TESTME: is this all we need to do?
			missile->s.pos.trType = TR_GRAVITY;
		}

		missile->classname = "vehicle_proj";

		missile->damage = vehWeapon->iDamage;
		missile->splashDamage = vehWeapon->iSplashDamage;
		missile->splashRadius = vehWeapon->fSplashRadius;

		// HUGE HORRIBLE HACK
		if (ent->owner && ent->owner->s.number==0)
		{
			//Should only be for speeders - mainly for t2_trip
			if (ent->m_pVehicle->m_pVehicleInfo && ent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{
				missile->damage			*= 20.0f;
				missile->splashDamage	*= 20.0f;
				missile->splashRadius	*= 20.0f;
			}
		}

		//FIXME: externalize some of these properties?
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->clipmask = MASK_SHOT;
		//Maybe by checking flags...?
		if ( vehWeapon->bSaberBlockable )
		{
			missile->clipmask |= CONTENTS_LIGHTSABER;
		}
		/*
		if ( (vehWeapon->iFlags&VWF_KNOCKBACK) )
		{
			missile->dflags &= ~DAMAGE_DEATH_KNOCKBACK;
		}
		if ( (vehWeapon->iFlags&VWF_DISTORTION_TRAIL) )
		{
			missile->s.eFlags |= EF_DISTORTION_TRAIL;
		}
		if ( (vehWeapon->iFlags&VWF_RADAR) )
		{
			missile->s.eFlags |= EF_RADAROBJECT;
		}
		*/
		missile->s.weapon = WP_BLASTER;//does this really matter?

		// Make it easier to hit things
		VectorCopy( mins, missile->mins );
		VectorCopy( maxs, missile->maxs );
		//some slightly different stuff for things with bboxes
		if ( vehWeapon->fWidth || vehWeapon->fHeight )
		{//we assume it's a rocket-like thing
			missile->methodOfDeath = MOD_ROCKET;
			missile->splashMethodOfDeath = MOD_ROCKET;// ?SPLASH;

			// we don't want it to ever bounce
			missile->bounceCount = 0;

			missile->mass = 10;
		}
		else
		{//a blaster-laser-like thing
			missile->s.weapon = WP_BLASTER;//does this really matter?
			missile->methodOfDeath = MOD_EMPLACED;//MOD_TURBLAST; //count as a heavy weap
			missile->splashMethodOfDeath = MOD_EMPLACED;//MOD_TURBLAST;// ?SPLASH;
			// we don't want it to bounce forever
			missile->bounceCount = 8;
		}

		if ( vehWeapon->iHealth )
		{//the missile can take damage
			missile->health = vehWeapon->iHealth;
			missile->takedamage = qtrue;
			missile->contents = MASK_SHOT;
			missile->e_DieFunc = dieF_WP_ExplosiveDie;//dieF_RocketDie;
		}

		//set veh as cgame side owner for purpose of fx overrides
		if (ent->m_pVehicle && ent->m_pVehicle->m_pPilot)
		{
			missile->owner = ent->m_pVehicle->m_pPilot;
		}
		else
		{
			missile->owner = ent;
		}
		missile->s.otherEntityNum = ent->s.number;
		missile->s.otherEntityNum2 = (vehWeapon-&g_vehWeaponInfo[0]);

		if ( vehWeapon->iLifeTime )
		{//expire after a time
			if ( vehWeapon->bExplodeOnExpire )
			{//blow up when your lifetime is up
				missile->e_ThinkFunc = thinkF_WP_Explode;//FIXME: custom func?
			}
			else
			{//just remove yourself
				missile->e_ThinkFunc = thinkF_G_FreeEntity;//FIXME: custom func?
			}
			missile->nextthink = level.time + vehWeapon->iLifeTime;
		}
		if ( vehWeapon->fHoming )
		{//homing missile
			//crap, we need to set up the homing stuff like it is in MP...
			WP_RocketLock( ent, 16384 );
			if ( ent->client && ent->client->rocketLockIndex != ENTITYNUM_NONE )
			{
				int dif = 0;
				float rTime;
				rTime = ent->client->rocketLockTime;

				if (rTime == -1)
				{
					rTime = ent->client->rocketLastValidTime;
				}

				if ( !vehWeapon->iLockOnTime )
				{//no minimum lock-on time
					dif = 10;//guaranteed lock-on
				}
				else
				{
					float lockTimeInterval = vehWeapon->iLockOnTime/16.0f;
					dif = ( level.time - rTime ) / lockTimeInterval;
				}

				if (dif < 0)
				{
					dif = 0;
				}

				//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
				if ( dif >= 10 && rTime != -1 )
				{
					missile->enemy = &g_entities[ent->client->rocketLockIndex];

					if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
					{ //if enemy became invalid, died, or is on the same team, then don't seek it
						missile->spawnflags |= 1;//just to let it know it should be faster... FIXME: EXTERNALIZE
						missile->speed = vehWeapon->fSpeed;
						missile->angle = vehWeapon->fHoming;
						if ( vehWeapon->iLifeTime )
						{//expire after a time
							missile->disconnectDebounceTime = level.time + vehWeapon->iLifeTime;
							missile->lockCount = (int)(vehWeapon->bExplodeOnExpire);
						}
						missile->e_ThinkFunc = thinkF_rocketThink;
						missile->nextthink = level.time + VEH_HOMING_MISSILE_THINK_TIME;
						//FIXME: implement radar in SP?
						//missile->s.eFlags |= EF_RADAROBJECT;
					}
				}

				ent->client->rocketLockIndex = ENTITYNUM_NONE;
				ent->client->rocketLockTime = 0;
				ent->client->rocketTargetTime = 0;

				VectorCopy( dir, missile->movedir );
				missile->random = 1.0f;//FIXME: externalize?
			}
		}
	}
	else
	{//traceline
		//FIXME: implement
	}
}

void WP_VehLeadCrosshairVeh( gentity_t *camTraceEnt, vec3_t newEnd, const vec3_t dir, const vec3_t shotStart, vec3_t shotDir )
{
	//FIXME: implement from MP?
}

qboolean WP_VehCheckTraceFromCamPos( gentity_t *ent, const vec3_t shotStart, vec3_t shotDir )
{
	//FIXME: implement from MP?
	return qfalse;
}

//---------------------------------------------------------
void FireVehicleWeapon( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	Vehicle_t *pVeh = ent->m_pVehicle;

	if ( !pVeh )
	{
		return;
	}

	if (pVeh->m_iRemovedSurfaces)
	{ //can't fire when the thing is breaking apart
		return;
	}


	if (ent->owner && ent->owner->client && ent->owner->client->ps.weapon!=WP_NONE)
	{
		return;
	}

	// TODO?: If possible (probably not enough time), it would be nice if secondary fire was actually a mode switch/toggle
	// so that, for instance, an x-wing can have 4-gun fire, or individual muzzle fire. If you wanted a different weapon, you
	// would actually have to press the 2 key or something like that (I doubt I'd get a graphic for it anyways though). -AReis

	// If this is not the alternate fire, fire a normal blaster shot...
	if ( pVeh->m_pVehicleInfo &&
		(pVeh->m_pVehicleInfo->type != VH_FIGHTER || (pVeh->m_ulFlags&VEH_WINGSOPEN)) ) // NOTE: Wings open also denotes that it has already launched.
	{//fighters can only fire when wings are open
		int	weaponNum = 0, vehWeaponIndex = VEH_WEAPON_NONE;
		int	delay = 1000;
		qboolean aimCorrect = qfalse;
		qboolean linkedFiring = qfalse;

		if ( !alt_fire )
		{
			weaponNum = 0;
		}
		else
		{
			weaponNum = 1;
		}

		vehWeaponIndex = pVeh->m_pVehicleInfo->weapon[weaponNum].ID;

		if ( pVeh->weaponStatus[weaponNum].ammo <= 0 )
		{//no ammo for this weapon
			if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
			{// let the client know he's out of ammo
				int i;
				//but only if one of the vehicle muzzles is actually ready to fire this weapon
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
					{//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					if ( pVeh->m_iMuzzleTag[i] != -1
						&& pVeh->m_Muzzles[i].m_iMuzzleWait < level.time )
					{//this one would have fired, send the no ammo message
						G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
						break;
					}
				}
			}
			return;
		}

		delay = pVeh->m_pVehicleInfo->weapon[weaponNum].delay;
		aimCorrect = pVeh->m_pVehicleInfo->weapon[weaponNum].aimCorrect;
		if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 2//always linked
			|| ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 1//optionally linkable
				&& pVeh->weaponStatus[weaponNum].linked ) )//linked
		{//we're linking the primary or alternate weapons, so we'll do *all* the muzzles
			linkedFiring = qtrue;
		}

		if ( vehWeaponIndex <= VEH_WEAPON_BASE || vehWeaponIndex >= MAX_VEH_WEAPONS )
		{//invalid vehicle weapon
			return;
		}
		else
		{
			int i, numMuzzles = 0, numMuzzlesReady = 0, cumulativeDelay = 0, cumulativeAmmo = 0;
			qboolean sentAmmoWarning = qfalse;

			vehWeaponInfo_t *vehWeapon = &g_vehWeaponInfo[vehWeaponIndex];

			if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 2 )
			{//always linked weapons don't accumulate delay, just use specified delay
				cumulativeDelay = delay;
			}
			//find out how many we've got for this weapon
			for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
			{
				if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
				{//this muzzle doesn't match the weapon we're trying to use
					continue;
				}
				if ( pVeh->m_iMuzzleTag[i] != -1 && pVeh->m_Muzzles[i].m_iMuzzleWait < level.time )
				{
					numMuzzlesReady++;
				}
				if ( pVeh->m_pVehicleInfo->weapMuzzle[pVeh->weaponStatus[weaponNum].nextMuzzle] != vehWeaponIndex )
				{//Our designated next muzzle for this weapon isn't valid for this weapon (happens when ships fire for the first time)
					//set the next to this one
					pVeh->weaponStatus[weaponNum].nextMuzzle = i;
				}
				if ( linkedFiring )
				{
					cumulativeAmmo += vehWeapon->iAmmoPerShot;
					if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable != 2 )
					{//always linked weapons don't accumulate delay, just use specified delay
						cumulativeDelay += delay;
					}
				}
				numMuzzles++;
			}

			if ( linkedFiring )
			{//firing all muzzles at once
				if ( numMuzzlesReady != numMuzzles )
				{//can't fire all linked muzzles yet
					return;
				}
				else
				{//can fire all linked muzzles, check ammo
					if ( pVeh->weaponStatus[weaponNum].ammo < cumulativeAmmo )
					{//can't fire, not enough ammo
						if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
						{// let the client know he's out of ammo
							G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
						}
						return;
					}
				}
			}

			for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
			{
				if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
				{//this muzzle doesn't match the weapon we're trying to use
					continue;
				}
				if ( !linkedFiring
					&& i != pVeh->weaponStatus[weaponNum].nextMuzzle )
				{//we're only firing one muzzle and this isn't it
					continue;
				}

				// Fire this muzzle.
				if ( pVeh->m_iMuzzleTag[i] != -1 && pVeh->m_Muzzles[i].m_iMuzzleWait < level.time )
				{
					vec3_t	start, dir;

					if ( pVeh->weaponStatus[weaponNum].ammo < vehWeapon->iAmmoPerShot )
					{//out of ammo!
						if ( !sentAmmoWarning )
						{
							sentAmmoWarning = qtrue;
							if ( pVeh->m_pPilot && pVeh->m_pPilot->s.number < MAX_CLIENTS )
							{// let the client know he's out of ammo
								G_AddEvent( (gentity_t*)pVeh->m_pPilot, EV_NOAMMO, weaponNum );
							}
						}
					}
					else
					{//have enough ammo to shoot
						//do the firing
						//WP_CalcVehMuzzle(ent, i);
						VectorCopy( pVeh->m_Muzzles[i].m_vMuzzlePos, start );
						VectorCopy( pVeh->m_Muzzles[i].m_vMuzzleDir, dir );
						if ( WP_VehCheckTraceFromCamPos( ent, start, dir ) )
						{//auto-aim at whatever crosshair would be over from camera's point of view (if closer)
						}
						else if ( aimCorrect )
						{//auto-aim the missile at the crosshair if there's anything there
							trace_t trace;
							vec3_t	end;
							vec3_t	ang;
							vec3_t	fixedDir;

							if (pVeh->m_pVehicleInfo->type == VH_SPEEDER)
							{
								VectorSet(ang, 0.0f, pVeh->m_vOrientation[1], 0.0f);
							}
							else
							{
								VectorCopy(pVeh->m_vOrientation, ang);
							}
							AngleVectors( ang, fixedDir, NULL, NULL );
							//VectorMA( ent->currentOrigin, 32768, dir, end );
							VectorMA( ent->currentOrigin, 8192, dir, end );
							gi.trace( &trace, ent->currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
							if ( trace.fraction < 1.0f && !trace.allsolid && !trace.startsolid )
							{
								vec3_t newEnd;
								VectorCopy( trace.endpos, newEnd );
								WP_VehLeadCrosshairVeh( &g_entities[trace.entityNum], newEnd, fixedDir, start, dir );
							}
						}

						//play the weapon's muzzle effect if we have one
						if ( vehWeapon->iMuzzleFX )
						{
							G_PlayEffect( vehWeapon->iMuzzleFX, pVeh->m_Muzzles[i].m_vMuzzlePos, pVeh->m_Muzzles[i].m_vMuzzleDir );
						}
						WP_FireVehicleWeapon( ent, start, dir, vehWeapon );
					}

					if ( linkedFiring )
					{//we're linking the weapon, so continue on and fire all appropriate muzzles
						continue;
					}
					//else just firing one
					//take the ammo, set the next muzzle and set the delay on it
					if ( numMuzzles > 1 )
					{//more than one, look for it
						int nextMuzzle = pVeh->weaponStatus[weaponNum].nextMuzzle;
						while ( 1 )
						{
							nextMuzzle++;
							if ( nextMuzzle >= MAX_VEHICLE_MUZZLES )
							{
								nextMuzzle = 0;
							}
							if ( nextMuzzle == pVeh->weaponStatus[weaponNum].nextMuzzle )
							{//WTF?  Wrapped without finding another valid one!
								break;
							}
							if ( pVeh->m_pVehicleInfo->weapMuzzle[nextMuzzle] == vehWeaponIndex )
							{//this is the next muzzle for this weapon
								pVeh->weaponStatus[weaponNum].nextMuzzle = nextMuzzle;
								break;
							}
						}
					}//else, just stay on the one we just fired
					//set the delay on the next muzzle
					pVeh->m_Muzzles[pVeh->weaponStatus[weaponNum].nextMuzzle].m_iMuzzleWait = level.time + delay;
					//take away the ammo
					pVeh->weaponStatus[weaponNum].ammo -= vehWeapon->iAmmoPerShot;
					//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
					if ( pVeh->m_pParentEntity && ((gentity_t*)(pVeh->m_pParentEntity))->client )
					{
						((gentity_t*)(pVeh->m_pParentEntity))->client->ps.ammo[weaponNum] = pVeh->weaponStatus[weaponNum].ammo;
					}
					//done!
					//we'll get in here again next frame and try the next muzzle...
					//return;
					return;
				}
			}
			//we went through all the muzzles, so apply the cumulative delay and ammo cost
			if ( cumulativeAmmo )
			{//taking ammo one shot at a time
				//take the ammo
				pVeh->weaponStatus[weaponNum].ammo -= cumulativeAmmo;
				//NOTE: in order to send the vehicle's ammo info to the client, we copy the ammo into the first 2 ammo slots on the vehicle NPC's client->ps.ammo array
				if ( pVeh->m_pParentEntity && ((gentity_t*)(pVeh->m_pParentEntity))->client )
				{
					((gentity_t*)(pVeh->m_pParentEntity))->client->ps.ammo[weaponNum] = pVeh->weaponStatus[weaponNum].ammo;
				}
			}
			if ( cumulativeDelay )
			{//we linked muzzles so we need to apply the cumulative delay now, to each of the linked muzzles
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
					{//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					//apply the cumulative delay
					pVeh->m_Muzzles[i].m_iMuzzleWait = level.time + cumulativeDelay;
				}
			}
		}
	}
}

void WP_FireScepter( gentity_t *ent, qboolean alt_fire )
{//just a straight beam
	int			damage = 1;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt = NULL, *tent;
	float		shotRange = 8192;
	qboolean	render_impact = qtrue;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	WP_MissileTargetHint(ent, start, forwardVec);
	VectorMA( start, shotRange, forwardVec, end );

	gi.trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	traceEnt = &g_entities[tr.entityNum];

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( muzzle, tent->s.origin2 );

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			// Create a simple impact type mark that doesn't last long in the world
			G_PlayEffect( G_EffectIndex( "disruptor/flesh_impact" ), tr.endpos, tr.plane.normal );

			int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
			G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_EXTRA_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
		}
		else
		{
			G_PlayEffect( G_EffectIndex( "disruptor/wall_impact" ), tr.endpos, tr.plane.normal );
		}
	}

	/*
	shotDist = shotRange * tr.fraction;

	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( start, dist, forwardVec, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	*/
}

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
//---------------------------------------------------------
void FireWeapon( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	float alert = 256;
	Vehicle_t *pVeh = NULL;

	// track shots taken for accuracy tracking.
	ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;

	// If this is a vehicle, fire it's weapon and we're done.
	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		FireVehicleWeapon( ent, alt_fire );
		return;
	}

	// set aiming directions
	if ( ent->s.weapon == WP_DISRUPTOR && alt_fire )
	{
		if ( ent->NPC )
		{
			//snipers must use the angles they actually did their shot trace with
			AngleVectors( ent->lastAngles, forwardVec, vrightVec, up );
		}
	}
	else if ( ent->s.weapon == WP_ATST_SIDE || ent->s.weapon == WP_ATST_MAIN )
	{
		vec3_t	delta1, enemy_org1, muzzle1;
		vec3_t	angleToEnemy1;

		VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle1 );

		if ( !ent->s.number )
		{//player driving an AT-ST
			//SIGH... because we can't anticipate alt-fire, must calc muzzle here and now
			mdxaBone_t		boltMatrix;
			int				bolt;

			if ( ent->client->ps.weapon == WP_ATST_MAIN )
			{//FIXME: alt_fire should fire both barrels, but slower?
				if ( ent->alt_fire )
				{
					bolt = ent->handRBolt;
				}
				else
				{
					bolt = ent->handLBolt;
				}
			}
			else
			{// ATST SIDE weapons
				if ( ent->alt_fire )
				{
					if ( gi.G2API_GetSurfaceRenderStatus( &ent->ghoul2[ent->playerModel], "head_light_blaster_cann" ) )
					{//don't have it!
						return;
					}
					bolt = ent->genericBolt2;
				}
				else
				{
					if ( gi.G2API_GetSurfaceRenderStatus( &ent->ghoul2[ent->playerModel], "head_concussion_charger" ) )
					{//don't have it!
						return;
					}
					bolt = ent->genericBolt1;
				}
			}

			vec3_t yawOnlyAngles = {0, ent->currentAngles[YAW], 0};
			if ( ent->currentAngles[YAW] != ent->client->ps.legsYaw )
			{
				yawOnlyAngles[YAW] = ent->client->ps.legsYaw;
			}
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel, bolt, &boltMatrix, yawOnlyAngles, ent->currentOrigin, (cg.time?cg.time:level.time), NULL, ent->s.modelScale );

			// work the matrix axis stuff into the original axis and origins used.
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent->client->renderInfo.muzzlePoint );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent->client->renderInfo.muzzleDir );
			ent->client->renderInfo.mPCalcTime = level.time;

			AngleVectors( ent->client->ps.viewangles, forwardVec, vrightVec, up );
			//CalcMuzzlePoint( ent, forwardVec, vrightVec, up, muzzle, 0 );
		}
		else if ( !ent->enemy )
		{//an NPC with no enemy to auto-aim at
			VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
		}
		else
		{//NPC, auto-aim at enemy
			CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );

			VectorSubtract (enemy_org1, muzzle1, delta1);

			vectoangles ( delta1, angleToEnemy1 );
			AngleVectors (angleToEnemy1, forwardVec, vrightVec, up);
		}
	}
	else if ( ent->s.weapon == WP_BOT_LASER && ent->enemy )
	{
		vec3_t	delta1, enemy_org1, muzzle1;
		vec3_t	angleToEnemy1;

		CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );
		CalcEntitySpot( ent, SPOT_WEAPON, muzzle1 );

		VectorSubtract (enemy_org1, muzzle1, delta1);

		vectoangles ( delta1, angleToEnemy1 );
		AngleVectors (angleToEnemy1, forwardVec, vrightVec, up);
	}
	else
	{
  		if ( (pVeh = G_IsRidingVehicle( ent )) != NULL) //riding a vehicle
		{//use our muzzleDir, can't use viewangles or vehicle m_vOrientation because we may be animated to shoot left or right...
			if ((ent->s.eFlags&EF_NODRAW))//we're inside it
			{
				vec3_t	aimAngles;
				VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
				vectoangles( forwardVec, aimAngles );
				//we're only keeping the yaw
				aimAngles[PITCH] = ent->client->ps.viewangles[PITCH];
				aimAngles[ROLL] = 0;
				AngleVectors( aimAngles, forwardVec, vrightVec, up );
			}
			else
			{
				vec3_t	actorRight;
				vec3_t	actorFwd;

				VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle );
				AngleVectors(ent->currentAngles, actorFwd, actorRight, 0);

				// Aiming Left
				//-------------
				if (ent->client->ps.torsoAnim==BOTH_VT_ATL_G || ent->client->ps.torsoAnim==BOTH_VS_ATL_G)
				{
 					VectorScale(actorRight, -1.0f, forwardVec);
				}

				// Aiming Right
				//--------------
				else if (ent->client->ps.torsoAnim==BOTH_VT_ATR_G || ent->client->ps.torsoAnim==BOTH_VS_ATR_G)
				{
 					VectorCopy(actorRight, forwardVec);
				}

				// Aiming Forward
				//----------------
				else
				{
	 				VectorCopy(actorFwd, forwardVec);
				}

				// If We Have An Enemy, Fudge The Aim To Hit The Enemy
				if (ent->enemy)
				{
					vec3_t	toEnemy;
					VectorSubtract(ent->enemy->currentOrigin, ent->currentOrigin, toEnemy);
					VectorNormalize(toEnemy);
					if (DotProduct(toEnemy, forwardVec)>0.75f &&
						((ent->s.number==0 && !Q_irand(0,2)) ||		// the player has a 1 in 3 chance
						 (ent->s.number!=0 && !Q_irand(0,5))))		// other guys have a 1 in 6 chance
					{
						VectorCopy(toEnemy, forwardVec);
					}
					else
					{
						forwardVec[0] += Q_flrand(-0.1f, 0.1f);
						forwardVec[1] += Q_flrand(-0.1f, 0.1f);
						forwardVec[2] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
		}
		else
		{
			AngleVectors( ent->client->ps.viewangles, forwardVec, vrightVec, up );
		}
	}

	ent->alt_fire = alt_fire;
	if (!pVeh)
	{
		if (ent->NPC && (ent->NPC->scriptFlags&SCF_FIRE_WEAPON_NO_ANIM))
		{
		 	VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle );
			VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
			MakeNormalVectors(forwardVec, vrightVec, up);
		}
		else
		{
			CalcMuzzlePoint ( ent, forwardVec, vrightVec, up, muzzle , 0);
		}
	}

	// fire the specific weapon
	switch( ent->s.weapon )
	{
	// Player weapons
	//-----------------
	case WP_SABER:
		return;
		break;

	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		WP_FireBryarPistol( ent, alt_fire );
		break;

	case WP_BLASTER:
		WP_FireBlaster( ent, alt_fire );
		break;

	case WP_TUSKEN_RIFLE:
		if ( alt_fire )
		{
			WP_FireTuskenRifle( ent );
		}
		else
		{
			WP_Melee( ent );
		}
		break;

	case WP_DISRUPTOR:
		alert = 50; // if you want it to alert enemies, remove this
		WP_FireDisruptor( ent, alt_fire );
		break;

	case WP_BOWCASTER:
		WP_FireBowcaster( ent, alt_fire );
		break;

	case WP_REPEATER:
		WP_FireRepeater( ent, alt_fire );
		break;

	case WP_DEMP2:
		WP_FireDEMP2( ent, alt_fire );
		break;

	case WP_FLECHETTE:
		WP_FireFlechette( ent, alt_fire );
		break;

	case WP_ROCKET_LAUNCHER:
		WP_FireRocket( ent, alt_fire );
		break;

	case WP_CONCUSSION:
		WP_Concussion( ent, alt_fire );
		break;

	case WP_THERMAL:
		WP_FireThermalDetonator( ent, alt_fire );
		break;

	case WP_TRIP_MINE:
		alert = 0; // if you want it to alert enemies, remove this
		WP_PlaceLaserTrap( ent, alt_fire );
		break;

	case WP_DET_PACK:
		alert = 0; // if you want it to alert enemies, remove this
		WP_FireDetPack( ent, alt_fire );
		break;

	case WP_BOT_LASER:
		WP_BotLaser( ent );
		break;

	case WP_EMPLACED_GUN:
		// doesn't care about whether it's alt-fire or not.  We can do an alt-fire if needed
		WP_EmplacedFire( ent );
		break;

	case WP_MELEE:
		alert = 0; // if you want it to alert enemies, remove this
		if ( !alt_fire || !g_debugMelee->integer )
		{
			WP_Melee( ent );
		}
		break;

	case WP_ATST_MAIN:
		WP_ATSTMainFire( ent );
		break;

	case WP_ATST_SIDE:

		// TEMP
		if ( alt_fire )
		{
//			WP_FireRocket( ent, qfalse );
			WP_ATSTSideAltFire(ent);
		}
		else
		{
			// FIXME!
		/*	if ( ent->s.number == 0
				&& ent->client->NPC_class == CLASS_VEHICLE
				&& vehicleData[((CVehicleNPC *)ent->NPC)->m_iVehicleTypeID].type == VH_FIGHTER )
			{
				WP_ATSTMainFire( ent );
			}
			else*/
			{
				WP_ATSTSideFire(ent);
			}
		}
		break;

	case WP_TIE_FIGHTER:
		// TEMP
		WP_EmplacedFire( ent );
		break;

	case WP_RAPID_FIRE_CONC:
		// TEMP
		if ( alt_fire )
		{
			WP_FireRepeater( ent, alt_fire );
		}
		else
		{
			WP_EmplacedFire( ent );
		}
		break;

	case WP_STUN_BATON:
		WP_FireStunBaton( ent, alt_fire );
		break;

//	case WP_BLASTER_PISTOL:
	case WP_JAWA:
		WP_FireBryarPistol( ent, qfalse ); // never an alt-fire?
		break;

	case WP_SCEPTER:
		WP_FireScepter( ent, alt_fire );
		break;

	case WP_NOGHRI_STICK:
		if ( !alt_fire )
		{
			WP_FireNoghriStick( ent );
		}
		//else does melee attack/damage/func
		break;

	case WP_TUSKEN_STAFF:
	default:
		return;
		break;
	}

	if ( !ent->s.number )
	{
		if ( ent->s.weapon == WP_FLECHETTE || (ent->s.weapon == WP_BOWCASTER && !alt_fire) )
		{//these can fire multiple shots, count them individually within the firing functions
		}
		else if ( W_AccuracyLoggableWeapon( ent->s.weapon, alt_fire, MOD_UNKNOWN ) )
		{
			ent->client->sess.missionStats.shotsFired++;
		}
	}
	// We should probably just use this as a default behavior, in special cases, just set alert to false.
	if ( ent->s.number == 0 && alert > 0 )
	{
		if ( ent->client->ps.groundEntityNum == ENTITYNUM_WORLD//FIXME: check for sand contents type?
			&& ent->s.weapon != WP_STUN_BATON
			&& ent->s.weapon != WP_MELEE
			&& ent->s.weapon != WP_TUSKEN_STAFF
			&& ent->s.weapon != WP_THERMAL
			&& ent->s.weapon != WP_TRIP_MINE
			&& ent->s.weapon != WP_DET_PACK )
		{//the vibration of the shot carries through your feet into the ground
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED, qfalse, qtrue );
		}
		else
		{//an in-air alert
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED );
		}
		AddSightEvent( ent, muzzle, alert*2, AEL_DISCOVERED, 20 );
	}
}

//NOTE: Emplaced gun moved to g_emplaced.cpp

/*QUAKED misc_weapon_shooter (1 0 0) (-8 -8 -8) (8 8 8) ALTFIRE TOGGLE
ALTFIRE - fire the alt-fire of the chosen weapon
TOGGLE - keep firing until used again (fires at intervals of "wait")

"wait" - debounce time between refires (defaults to 500)
"delay" - speed of WP_THERMAL (default is 900)
"random" - ranges from 0 to random, added to wait (defaults to 0)

"target" - what to aim at (will update aim every frame if it's a moving target)

"weapon" - specify the weapon to use (default is WP_BLASTER)
	WP_BRYAR_PISTOL
	WP_BLASTER
	WP_DISRUPTOR
	WP_BOWCASTER
	WP_REPEATER
	WP_DEMP2
	WP_FLECHETTE
	WP_ROCKET_LAUNCHER
	WP_CONCUSSION
	WP_THERMAL
	WP_TRIP_MINE
	WP_DET_PACK
	WP_STUN_BATON
	WP_EMPLACED_GUN
	WP_BOT_LASER
	WP_TURRET
	WP_ATST_MAIN
	WP_ATST_SIDE
	WP_TIE_FIGHTER
	WP_RAPID_FIRE_CONC
	WP_BLASTER_PISTOL
*/
void misc_weapon_shooter_fire( gentity_t *self )
{
	FireWeapon( self, (qboolean)((self->spawnflags&1) != 0) );
	if ( (self->spawnflags&2) )
	{//repeat
		self->e_ThinkFunc = thinkF_misc_weapon_shooter_fire;
		if (self->random)
		{
			self->nextthink = level.time + self->wait + (int)(Q_flrand(0.0f, 1.0f)*self->random);
		}
		else
		{
			self->nextthink = level.time + self->wait;
		}
	}
}

void misc_weapon_shooter_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->e_ThinkFunc == thinkF_misc_weapon_shooter_fire )
	{//repeating fire, stop
		self->e_ThinkFunc = thinkF_NULL;
		self->nextthink = -1;
		return;
	}
	//otherwise, fire
	misc_weapon_shooter_fire( self );
}

void misc_weapon_shooter_aim( gentity_t *self )
{
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			self->enemy = targ;
			VectorSubtract( targ->currentOrigin, self->currentOrigin, self->client->renderInfo.muzzleDir );
			VectorCopy( targ->currentOrigin, self->pos1 );
			vectoangles( self->client->renderInfo.muzzleDir, self->client->ps.viewangles );
			SetClientViewAngle( self, self->client->ps.viewangles );
			//FIXME: don't keep doing this unless target is a moving target?
			self->nextthink = level.time + FRAMETIME;
		}
		else
		{
			self->enemy = NULL;
		}
	}
}

extern stringID_table_t WPTable[];
void SP_misc_weapon_shooter( gentity_t *self )
{
	//alloc a client just for the weapon code to use
	self->client = (gclient_t *)gi.Malloc(sizeof(gclient_t), TAG_G_ALLOC, qtrue);

	//set weapon
	self->s.weapon = self->client->ps.weapon = WP_BLASTER;
	if ( self->paintarget )
	{//use a different weapon
		self->s.weapon = self->client->ps.weapon = GetIDForString( WPTable, self->paintarget );
	}

	//set where our muzzle is
	VectorCopy( self->s.origin, self->client->renderInfo.muzzlePoint );
	//permanently updated
	self->client->renderInfo.mPCalcTime = Q3_INFINITE;

	//set up to link
	if ( self->target )
	{
        self->e_ThinkFunc = thinkF_misc_weapon_shooter_aim;
		self->nextthink = level.time + START_TIME_LINK_ENTS;
	}
	else
	{//just set aim angles
		VectorCopy( self->s.angles, self->client->ps.viewangles );
		AngleVectors( self->s.angles, self->client->renderInfo.muzzleDir, NULL, NULL );
	}

	//set up to fire when used
    self->e_UseFunc = useF_misc_weapon_shooter_use;

	if ( !self->wait )
	{
		self->wait = 500;
	}
}
