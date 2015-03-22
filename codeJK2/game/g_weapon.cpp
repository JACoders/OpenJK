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

#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "anims.h"
#include "b_local.h"
#include "w_local.h"

vec3_t	wpFwd, wpVright, wpUp;
vec3_t	wpMuzzle;

gentity_t *ent_list[MAX_GENTITIES];

// some naughty little things that are used cg side
int g_rocketLockEntNum = ENTITYNUM_NONE;
int g_rocketLockTime = 0;
int	g_rocketSlackTime = 0;

// Weapon Helper Functions

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
	newstart[2] = start[2]; // force newstart to be on the same plane as the wpMuzzle ( start )

	gi.trace( &tr, newstart, entMins, entMaxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP, G2_NOCOLLIDE, 0 );

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

	missile->alt_fire = altFire;

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - 10;	// move a bit on the very first frame
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
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
	vec3_t		wpFwd;

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	AngleVectors( self->s.angles, wpFwd, NULL, NULL );

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, wpFwd );
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
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, attacker, MOD_EXPLOSIVE_SPLASH );
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
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_ROCKET_LAUNCHER:
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
void CalcMuzzlePoint( gentity_t *const ent, vec3_t wpFwd, vec3_t right, vec3_t wpUp, vec3_t muzzlePoint, float lead_in )
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
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 16;
		VectorMA( muzzlePoint, 28, wpFwd, muzzlePoint );
		VectorMA( muzzlePoint, 6, wpVright, muzzlePoint );
		break;

	case WP_ROCKET_LAUNCHER:
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
			VectorMA( muzzlePoint, 12, wpFwd, muzzlePoint ); // player, don't set this any lower otherwise the projectile will impact immediately when your back is to a wall
		else
			VectorMA( muzzlePoint, 2, wpFwd, muzzlePoint ); // NPC, don't set too far wpFwd otherwise the projectile can go through doors

		VectorMA( muzzlePoint, 1, wpVright, muzzlePoint );
		break;

	case WP_SABER:
		if(ent->NPC!=NULL &&
			(ent->client->ps.torsoAnim == TORSO_WEAPONREADY2 ||
			ent->client->ps.torsoAnim == BOTH_ATTACK2))//Sniper pose
		{
			ViewHeightFix(ent);
			wpMuzzle[2] += ent->client->ps.viewheight;//By eyes
		}
		else
		{
			muzzlePoint[2] += 16;
		}
		VectorMA( muzzlePoint, 8, wpFwd, muzzlePoint );
		VectorMA( muzzlePoint, 16, wpVright, muzzlePoint );
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

//---------------------------------------------------------
void FireWeapon( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	float alert = 256;

	// track shots taken for accuracy tracking.
	ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;

	// set aiming directions
	if ( ent->s.weapon == WP_DISRUPTOR && alt_fire )
	{
		if ( ent->NPC )
		{
			//snipers must use the angles they actually did their shot trace with
			AngleVectors( ent->lastAngles, wpFwd, wpVright, wpUp );
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

			AngleVectors( ent->client->ps.viewangles, wpFwd, wpVright, wpUp );
			//CalcMuzzlePoint( ent, wpFwd, vright, wpUp, wpMuzzle, 0 );
		}
		else if ( !ent->enemy )
		{//an NPC with no enemy to auto-aim at
			VectorCopy( ent->client->renderInfo.muzzleDir, wpFwd );
		}
		else
		{//NPC, auto-aim at enemy
			CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );

			VectorSubtract (enemy_org1, muzzle1, delta1);

			vectoangles ( delta1, angleToEnemy1 );
			AngleVectors (angleToEnemy1, wpFwd, wpVright, wpUp);
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
		AngleVectors (angleToEnemy1, wpFwd, wpVright, wpUp);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, wpFwd, wpVright, wpUp );
	}

	ent->alt_fire = alt_fire;
	CalcMuzzlePoint ( ent, wpFwd, wpVright, wpUp, wpMuzzle , 0);

	// fire the specific weapon
	switch( ent->s.weapon )
	{
	// Player weapons
	//-----------------
	case WP_SABER:
		return;
		break;

	case WP_BRYAR_PISTOL:
		WP_FireBryarPistol( ent, alt_fire );
		break;

	case WP_BLASTER:
		WP_FireBlaster( ent, alt_fire );
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
		WP_Melee( ent );
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
			if ( ent->s.number == 0 && ent->client->ps.vehicleModel )
			{
				WP_ATSTMainFire( ent );
			}
			else
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

	case WP_BLASTER_PISTOL: // enemy version
		WP_FireBryarPistol( ent, qfalse ); // never an alt-fire?
		break;

//	case WP_TRICORDER:
//		WP_TricorderScan( ent, alt_fire );
//		break;

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
		AddSoundEvent( ent, wpMuzzle, alert, AEL_DISCOVERED );
		AddSightEvent( ent, wpMuzzle, alert*2, AEL_DISCOVERED, 20 );
	}
}

// spawnflag
#define	EMPLACED_INACTIVE	1
#define EMPLACED_FACING		2
#define EMPLACED_VULNERABLE	4

//----------------------------------------------------------

/*QUAKED emplaced_gun (0 0 1) (-24 -24 0) (24 24 64) INACTIVE FACING VULNERABLE

 INACTIVE cannot be used until used by a target_activate
 FACING - player must be facing relatively in the same direction as the gun in order to use it
 VULNERABLE - allow the gun to take damage

 count - how much ammo to give this gun ( default 999 )
 health - how much damage the gun can take before it blows ( default 250 )
 delay - ONLY AFFECTS NPCs - time between shots ( default 200 on hardest setting )
 wait - ONLY AFFECTS NPCs - time between bursts ( default 800 on hardest setting )
 splashdamage - how much damage a blowing up gun deals ( default 80 )
 splashradius - radius for exploding damage ( default 128 )
*/

//----------------------------------------------------------
void emplaced_gun_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	vec3_t fwd1, fwd2;

	if ( self->health <= 0 )
	{
		// can't use a dead gun.
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{
		return; // can't use inactive gun
	}

	if ( !activator->client )
	{
		return; // only a client can use it.
	}

	if ( self->activator )
	{
		// someone is already in the gun.
		return;
	}

	// We'll just let the designers duke this one out....I mean, as to whether they even want to limit such a thing.
	if ( self->spawnflags & EMPLACED_FACING )
	{
		// Let's get some direction vectors for the users
		AngleVectors( activator->client->ps.viewangles, fwd1, NULL, NULL );

		// Get the guns direction vector
		AngleVectors( self->pos1, fwd2, NULL, NULL );

		float dot = DotProduct( fwd1, fwd2 );

		// Must be reasonably facing the way the gun points ( 90 degrees or so ), otherwise we don't allow to use it.
		if ( dot < 0.0f )
		{
			return;
		}
	}

	// don't allow using it again for half a second
	if ( self->delay + 500 < level.time )
	{
		int	oldWeapon = activator->s.weapon;

		if ( oldWeapon == WP_SABER )
		{
			self->alt_fire = activator->client->ps.saberActive;
		}

		// swap the users weapon with the emplaced gun and add the ammo the gun has to the player
		activator->client->ps.weapon = self->s.weapon;
		Add_Ammo( activator, WP_EMPLACED_GUN, self->count );
		activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

		// Allow us to point from one to the other
		activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
		self->activator = activator;

		if ( activator->weaponModel >= 0 )
		{
			// rip that gun out of their hands....
			gi.G2API_RemoveGhoul2Model( activator->ghoul2, activator->weaponModel );
			activator->weaponModel = -1;
		}

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
		if ( activator->NPC )
		{
			if ( activator->weaponModel >= 0 )
			{
				// rip that gun out of their hands....
				gi.G2API_RemoveGhoul2Model( activator->ghoul2, activator->weaponModel );
				activator->weaponModel = -1;

// Doesn't work?
//				activator->maxs[2] += 35; // make it so you can potentially shoot their head
//				activator->s.radius += 10; // increase ghoul radius so we can collide with the enemy more accurately
//				gi.linkentity( activator );
			}

			ChangeWeapon( activator, WP_EMPLACED_GUN );
		}
		else if ( activator->s.number == 0 )
		{
			// we don't want for it to draw the weapon select stuff
			cg.weaponSelect = WP_EMPLACED_GUN;
			CG_CenterPrint( "@INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
		}
		// Since we move the activator inside of the gun, we reserve a solid spot where they were standing in order to be able to get back out without being in solid
		if ( self->nextTrain )
		{//you never know
			G_FreeEntity( self->nextTrain );
		}
		self->nextTrain = G_Spawn();
		//self->nextTrain->classname = "emp_placeholder";
		self->nextTrain->contents = CONTENTS_MONSTERCLIP|CONTENTS_PLAYERCLIP;//hmm... playerclip too now that we're doing it for NPCs?
		G_SetOrigin( self->nextTrain, activator->client->ps.origin );
		VectorCopy( activator->mins, self->nextTrain->mins );
		VectorCopy( activator->maxs, self->nextTrain->maxs );
		gi.linkentity( self->nextTrain );

		//need to inflate the activator's mins/maxs since the gunsit anim puts them outside of their bbox
		VectorSet( activator->mins, -24, -24, -24 );
		VectorSet( activator->maxs, 24, 24, 40 );

		// Move the activator into the center of the gun.  For NPC's the only way the can get out of the gun is to die.
		VectorCopy( self->s.origin, activator->client->ps.origin );
		activator->client->ps.origin[2] += 30; // move them up so they aren't standing in the floor
		gi.linkentity( activator );

		// the gun will track which weapon we used to have
		self->s.weapon = oldWeapon;

		// Lock the player
		activator->client->ps.eFlags |= EF_LOCKED_TO_WEAPON;
		activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
		self->activator = activator;
		self->delay = level.time; // can't disconnect from the thing for half a second

		// Let the gun be considered an enemy
		self->svFlags |= SVF_NONNPC_ENEMY;
		self->noDamageTeam = activator->client->playerTeam;

		// FIXME: don't do this, we'll try and actually put the player in this beast
		// move the player to the center of the gun
//		activator->contents = 0;
//		VectorCopy( self->currentOrigin, activator->client->ps.origin );

		SetClientViewAngle( activator, self->pos1 );

		//FIXME: should really wait a bit after spawn and get this just once?
		self->waypoint = NAV_FindClosestWaypointForEnt( self, WAYPOINT_NONE );
#ifdef _DEBUG
		if ( self->waypoint == -1 )
		{
			gi.Printf( S_COLOR_RED"ERROR: no waypoint for emplaced_gun %s at %s\n", self->targetname, vtos(self->currentOrigin) );
		}
#endif

		G_Sound( self, G_SoundIndex( "sound/weapons/emplaced/emplaced_mount.mp3" ));
	}
}

//----------------------------------------------------------
void emplaced_gun_pain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc )
{
	if ( self->health <= 0 )
	{
		// play pain effect?
	}
	else
	{
		if ( self->paintarget )
		{
			G_UseTargets2( self, self->activator, self->paintarget );
		}

		// Don't do script if dead
		G_ActivateBehavior( self, BSET_PAIN );
	}
}

//----------------------------------------------------------
void emplaced_blow( gentity_t *ent )
{
	ent->e_DieFunc = dieF_NULL;
	emplaced_gun_die( ent, ent->lastEnemy, ent->lastEnemy, 0, MOD_UNKNOWN );
}

//----------------------------------------------------------
void emplaced_gun_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	vec3_t org;

	// turn off any firing animations it may have been doing
	self->s.frame = self->startFrame = self->endFrame = 0;
	self->svFlags &= ~SVF_ANIMATING;

	self->health = 0;
//	self->s.weapon = WP_EMPLACED_GUN; // we need to be able to switch back to the old weapon

	self->takedamage = qfalse;
	self->lastEnemy = attacker;

	// we defer explosion so the player has time to get out
	if ( self->e_DieFunc )
	{
		self->e_ThinkFunc = thinkF_emplaced_blow;
		self->nextthink = level.time + 3000; // don't blow for a couple of seconds
		return;
	}

	if ( self->activator && self->activator->client )
	{
		if ( self->activator->NPC )
		{
			vec3_t right;

			// radius damage seems to throw them, but add an extra bit to throw them away from the weapon
			AngleVectors( self->currentAngles, NULL, right, NULL );
			VectorMA( self->activator->client->ps.velocity, 140, right, self->activator->client->ps.velocity );
			self->activator->client->ps.velocity[2] = -100;

			// kill them
			self->activator->health = 0;
			self->activator->client->ps.stats[STAT_HEALTH] = 0;
		}

		// kill the players emplaced ammo, cheesy way to keep the gun from firing
		self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] = 0;
	}

	self->e_PainFunc = painF_NULL;
	self->e_ThinkFunc = thinkF_NULL;

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_RadiusDamage( self->currentOrigin, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

	// when the gun is dead, add some ugliness to it.
	vec3_t ugly;

	ugly[YAW] = 4;
	ugly[PITCH] = self->lastAngles[PITCH] * 0.8f + crandom() * 6;
	ugly[ROLL] = crandom() * 7;
	gi.G2API_SetBoneAnglesIndex( &self->ghoul2[self->playerModel], self->lowerLumbarBone, ugly, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );

	VectorCopy( self->currentOrigin,  org );
	org[2] += 20;

	G_PlayEffect( "emplaced/explode", org );

	// create some persistent smoke by using a dynamically created fx runner
	gentity_t *ent = G_Spawn();

	if ( ent )
	{
		ent->delay = 200;
		ent->random = 100;

		ent->fxID = G_EffectIndex( "emplaced/dead_smoke" );

		ent->e_ThinkFunc = thinkF_fx_runner_think;
		ent->nextthink = level.time + 50;

		// move up above the gun origin
		VectorCopy( self->currentOrigin, org );
		org[2] += 35;
		G_SetOrigin( ent, org );
		VectorCopy( org, ent->s.origin );

		VectorSet( ent->s.angles, -90, 0, 0 ); // up
		G_SetAngles( ent, ent->s.angles );

		gi.linkentity( ent );
	}

	G_ActivateBehavior( self, BSET_DEATH );
}

//----------------------------------------------------------
void SP_emplaced_gun( gentity_t *ent )
{
	char name[] = "models/map_objects/imp_mine/turret_chair.glm";

	ent->svFlags |= SVF_PLAYER_USABLE;
	ent->contents = CONTENTS_BODY;//CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP;//CONTENTS_SOLID;

	if ( ent->spawnflags & EMPLACED_INACTIVE )
	{
		ent->svFlags |= SVF_INACTIVE;
	}

	VectorSet( ent->mins, -30, -30, -5 );
	VectorSet( ent->maxs, 30, 30, 60 );

	ent->takedamage = qtrue;

	if ( !( ent->spawnflags & EMPLACED_VULNERABLE ))
	{
		ent->flags |= FL_GODMODE;
	}

	ent->s.radius = 110;
	ent->spawnflags |= 4; // deadsolid

	ent->e_ThinkFunc = thinkF_NULL;
	ent->e_PainFunc = painF_emplaced_gun_pain;
	ent->e_DieFunc  = dieF_emplaced_gun_die;

	G_EffectIndex( "emplaced/explode" );
	G_EffectIndex( "emplaced/dead_smoke" );

	G_SoundIndex( "sound/weapons/emplaced/emplaced_mount.mp3" );
	G_SoundIndex( "sound/weapons/emplaced/emplaced_dismount.mp3" );
	G_SoundIndex( "sound/weapons/emplaced/emplaced_move_lp.wav" );

	// Set up our defaults and override with custom amounts as necessary
	G_SpawnInt( "count", "999", &ent->count );
	G_SpawnInt( "health", "250", &ent->health );
	G_SpawnInt( "splashDamage", "80", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "128", &ent->splashRadius );
	G_SpawnFloat( "delay", "200", &ent->random ); // NOTE: spawning into a different field!!
	G_SpawnFloat( "wait", "800", &ent->wait );

	ent->max_health = ent->health;
	ent->dflags |= DAMAGE_CUSTOM_HUD; // dumb, but we draw a custom hud

	ent->s.modelindex = G_ModelIndex( name );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, name, ent->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );

	// Activate our tags and bones
	ent->headBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*seat" );
	ent->handLBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*flash01" );
	ent->handRBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*flash02" );
	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "base_bone", qtrue );
	ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[0], "swivel_bone", qtrue );
	gi.G2API_SetBoneAngles( &ent->ghoul2[0], "swivel_bone", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0);

	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	VectorCopy( ent->s.angles, ent->lastAngles );

	// store base angles for later
	VectorCopy( ent->s.angles, ent->pos1 );

	ent->e_UseFunc = useF_emplaced_gun_use;

	gi.linkentity (ent);
}
