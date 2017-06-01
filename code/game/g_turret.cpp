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

#include "g_local.h"
#include "g_functions.h"
#include "b_local.h"
#include "../cgame/cg_local.h"

extern	cvar_t	*g_spskill;

void G_SetEnemy( gentity_t *self, gentity_t *enemy );
void finish_spawning_turret( gentity_t *base );
void ObjectDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
//special routine for tracking angles between client and server -rww
void turret_SetBoneAngles(gentity_t *ent, const char *bone, const vec3_t angles);

#define	ARM_ANGLE_RANGE		60
#define	HEAD_ANGLE_RANGE	90

#define SPF_TURRETG2_TURBO		4
#define SPF_TURRETG2_LEAD_ENEMY	8

#define name "models/map_objects/imp_mine/turret_canon.glm"
#define name2 "models/map_objects/imp_mine/turret_damage.md3"
#define name3 "models/map_objects/wedge/laser_cannon_model.glm"

//------------------------------------------------------------------------------------------------------------
void TurretPain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, const vec3_t point, int damage, int mod, int hitLoc )
//------------------------------------------------------------------------------------------------------------
{
	vec3_t dir;

	VectorSubtract( point, self->currentOrigin, dir );
	VectorNormalize( dir );

	if ( mod == MOD_DEMP2 || mod == MOD_DEMP2_ALT )
	{
		// DEMP2 makes the turret stop shooting for a bit..and does extra feedback
		self->attackDebounceTime = level.time + 800 + Q_flrand(0.0f, 1.0f) * 500;
		G_PlayEffect( "sparks/spark_exp_nosnd", point, dir );
	}

	if ( !self->enemy )
	{//react to being hit
		G_SetEnemy( self, attacker );
	}

	G_PlayEffect( "sparks/spark_exp_nosnd", point, dir );
}

//------------------------------------------------------------------------------------------------------------
void turret_die ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
//------------------------------------------------------------------------------------------------------------
{
	vec3_t	forward = { 0,0,-1 }, pos;

	// Turn off the thinking of the base & use it's targets
	self->e_ThinkFunc = thinkF_NULL;
	self->e_UseFunc = useF_NULL;

	// clear my data
	self->e_DieFunc  = dieF_NULL;
	self->takedamage = qfalse;
	self->health = 0;
	self->s.loopSound = 0;

	// hack the effect angle so that explode death can orient the effect properly
	if ( self->spawnflags & 2 )
	{
		VectorSet( forward, 0, 0, 1 );
	}

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );

	if ( self->spawnflags & SPF_TURRETG2_TURBO )
	{
		G_PlayEffect( G_EffectIndex( "explosions/fighter_explosion2" ), self->currentOrigin, self->currentAngles );
	}
	else
	{
		if ( self->fxID > 0 )
		{
			VectorMA( self->currentOrigin, 12, forward, pos );
			G_PlayEffect( self->fxID, pos, forward );
		}
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, attacker, MOD_UNKNOWN );
	}

	if ( self->s.eFlags & EF_SHADER_ANIM )
	{
		self->s.frame = 1; // black
	}

	self->s.weapon = 0; // crosshair code uses this to mark crosshair red

	if ( self->s.modelindex2 )
	{
		// switch to damage model if we should
		self->s.modelindex = self->s.modelindex2;

		VectorCopy( self->currentAngles, self->s.apos.trBase );
		VectorClear( self->s.apos.trDelta );

		if ( self->target )
		{
			G_UseTargets( self, attacker );
		}
	}
	else
	{
		ObjectDie( self, inflictor, attacker, damage, meansOfDeath );
	}
}

//start an animation on model_root both server side and client side
void TurboLaser_SetBoneAnim(gentity_t *eweb, int startFrame, int endFrame)
{
	//set info on the entity so it knows to start the anim on the client next snapshot.
	//eweb->s.eFlags |= EF_G2ANIMATING;

	if (eweb->s.torsoAnim == startFrame && eweb->s.legsAnim == endFrame)
	{ //already playing this anim, let's flag it to restart
		//eweb->s.torsoFlip = !eweb->s.torsoFlip;
	}
	else
	{
		eweb->s.torsoAnim = startFrame;
		eweb->s.legsAnim = endFrame;
	}

	//now set the animation on the server ghoul2 instance.
	assert(&eweb->ghoul2[0]);
	gi.G2API_SetBoneAnim(&eweb->ghoul2[0], "model_root", startFrame, endFrame,
		(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND), 1.0f, level.time, -1, 100);
}

#define START_DIS 15

extern void WP_FireTurboLaserMissile( gentity_t *ent, vec3_t start, vec3_t dir );

//----------------------------------------------------------------
static void turret_fire ( gentity_t *ent, vec3_t start, vec3_t dir )
//----------------------------------------------------------------
{
	vec3_t		org, ang;
	gentity_t	*bolt;

	if ( (gi.pointcontents( start, ent->s.number )&MASK_SHOT) )
	{
		return;
	}

	VectorMA( start, -START_DIS, dir, org ); // dumb....

	if ( ent->random )
	{
		vectoangles( dir, ang );
		ang[PITCH] += Q_flrand( -ent->random, ent->random );
		ang[YAW] += Q_flrand( -ent->random, ent->random );
		AngleVectors( ang, dir, NULL, NULL );
	}

	vectoangles(dir, ang);

	if ( (ent->spawnflags&SPF_TURRETG2_TURBO) )
	{
		//muzzle flash
		G_PlayEffect( G_EffectIndex( "turret/turb_muzzle_flash" ), org, ang );
		G_SoundOnEnt( ent, CHAN_LESS_ATTEN, "sound/vehicles/weapons/turbolaser/fire1" );

		WP_FireTurboLaserMissile( ent, start, dir );
		if ( ent->alt_fire )
		{
			TurboLaser_SetBoneAnim( ent, 2, 3 );
		}
		else
		{
			TurboLaser_SetBoneAnim( ent, 0, 1 );
		}
	}
	else
	{
		G_PlayEffect( "blaster/muzzle_flash", org, dir );

		bolt = G_Spawn();

		bolt->classname = "turret_proj";
		bolt->nextthink = level.time + 10000;
		bolt->e_ThinkFunc = thinkF_G_FreeEntity;
		bolt->s.eType = ET_MISSILE;
		bolt->s.weapon = WP_BLASTER;
		bolt->owner = ent;
		bolt->damage = ent->damage;
		bolt->dflags = DAMAGE_NO_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;		// Don't push them around, or else we are constantly re-aiming
		bolt->splashDamage = 0;
		bolt->splashRadius = 0;
		bolt->methodOfDeath = MOD_ENERGY;
		bolt->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		bolt->trigger_formation = qfalse;		// don't draw tail on first frame

		VectorSet( bolt->maxs, 1.5, 1.5, 1.5 );
		VectorScale( bolt->maxs, -1, bolt->mins );
		bolt->s.pos.trType = TR_LINEAR;
		bolt->s.pos.trTime = level.time;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorScale( dir, 1100, bolt->s.pos.trDelta );
		SnapVector( bolt->s.pos.trDelta );		// save net bandwidth
		VectorCopy( start, bolt->currentOrigin);
	}
}

//-----------------------------------------------------
void turret_head_think( gentity_t *self )
//-----------------------------------------------------
{
	// if it's time to fire and we have an enemy, then gun 'em down!  pushDebounce time controls next fire time
	if ( self->enemy && self->pushDebounceTime < level.time && self->attackDebounceTime < level.time )
	{
		// set up our next fire time
		self->pushDebounceTime = level.time + self->wait;

		vec3_t		fwd, org;
		mdxaBone_t	boltMatrix;

		// Getting the flash bolt here
		gi.G2API_GetBoltMatrix( self->ghoul2,
					0,
					(self->spawnflags&SPF_TURRETG2_TURBO) ? ( (self->alt_fire ? gi.G2API_AddBolt( &self->ghoul2[0], "*muzzle2" ) : gi.G2API_AddBolt( &self->ghoul2[0], "*muzzle1" )) ) : gi.G2API_AddBolt( &self->ghoul2[0], "*flash03" ),
					&boltMatrix,
					self->currentAngles,
					self->currentOrigin,
					level.time,
					NULL,
					self->modelScale );
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->alt_fire = (qboolean)!self->alt_fire;
		}

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );

		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, fwd );
		}
		else
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, fwd );
		}

		VectorMA( org, START_DIS, fwd, org );

		turret_fire( self, org, fwd );
		self->fly_sound_debounce_time = level.time;//used as lastShotTime
	}
}

//-----------------------------------------------------
static void turret_aim( gentity_t *self )
//-----------------------------------------------------
{
	vec3_t	enemyDir, org, org2;
	vec3_t	desiredAngles, setAngle;
	float	diffYaw = 0.0f, diffPitch = 0.0f;
	float	maxYawSpeed		= ( self->spawnflags & SPF_TURRETG2_TURBO ) ? 30.0f : 14.0f;
	float	maxPitchSpeed	= ( self->spawnflags & SPF_TURRETG2_TURBO ) ? 15.0f : 3.0f;

	// move our gun base yaw to where we should be at this time....
	EvaluateTrajectory( &self->s.apos, level.time, self->currentAngles );
	self->currentAngles[YAW] = AngleNormalize360( self->currentAngles[YAW] );
	self->speed = AngleNormalize360( self->speed );

	if ( self->enemy )
	{
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		if ( self->enemy->client )
		{
			VectorCopy( self->enemy->client->renderInfo.eyePoint, org );
		}
		else
		{
			VectorCopy( self->enemy->currentOrigin, org );
		}
		if ( self->spawnflags & 2 )
		{
			org[2] -= 15;
		}
		else
		{
			org[2] -= 5;
		}
		mdxaBone_t	boltMatrix;

		// Getting the "eye" here
		gi.G2API_GetBoltMatrix( self->ghoul2,
					0,
					(self->spawnflags&SPF_TURRETG2_TURBO) ? ( (self->alt_fire ? gi.G2API_AddBolt( &self->ghoul2[0], "*muzzle2" ) : gi.G2API_AddBolt( &self->ghoul2[0], "*muzzle1" )) ) : gi.G2API_AddBolt( &self->ghoul2[0], "*flash03" ),
					&boltMatrix,
					self->currentAngles,
					self->s.origin,
					level.time,
					NULL,
					self->modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org2 );

		VectorSubtract( org, org2, enemyDir );
		vectoangles( enemyDir, desiredAngles );

		diffYaw = AngleSubtract( self->currentAngles[YAW], desiredAngles[YAW] );
		diffPitch = AngleSubtract( self->speed, desiredAngles[PITCH] );
	}
	else
	{
		// no enemy, so make us slowly sweep back and forth as if searching for a new one
//		diffYaw = sin( level.time * 0.0001f + self->count ) * 5.0f;	// don't do this for now since it can make it go into walls.
	}

	if ( diffYaw )
	{
		// cap max speed....
		if ( fabs(diffYaw) > maxYawSpeed )
		{
			diffYaw = ( diffYaw >= 0 ? maxYawSpeed : -maxYawSpeed );
		}

		// ...then set up our desired yaw
		VectorSet( setAngle, 0.0f, diffYaw, 0.0f );

		VectorCopy( self->currentAngles, self->s.apos.trBase );
		VectorScale( setAngle,- 5, self->s.apos.trDelta );
		self->s.apos.trTime = level.time;
		self->s.apos.trType = TR_LINEAR;
	}

	if ( diffPitch )
	{
		if ( fabs(diffPitch) > maxPitchSpeed )
		{
			// cap max speed
			self->speed += (diffPitch > 0.0f) ? -maxPitchSpeed : maxPitchSpeed;
		}
		else
		{
			// small enough, so just add half the diff so we smooth out the stopping
			self->speed -= ( diffPitch );//desiredAngles[PITCH];
		}

		// Note that this is NOT interpolated, so it will be less smooth...On the other hand, it does use Ghoul2 to blend, so it may smooth it out a bit?
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			if ( self->spawnflags & 2 )
			{
				VectorSet( desiredAngles, 0.0f, 0.0f, -self->speed );
			}
			else
			{
				VectorSet( desiredAngles, 0.0f, 0.0f, self->speed );
			}
			turret_SetBoneAngles(self, "pitch", desiredAngles);
		}
		else
		{
			// Note that this is NOT interpolated, so it will be less smooth...On the other hand, it does use Ghoul2 to blend, so it may smooth it out a bit?
			if ( self->spawnflags & 2 )
			{
				VectorSet( desiredAngles, self->speed, 0.0f, 0.0f );
			}
			else
			{
				VectorSet( desiredAngles, -self->speed, 0.0f, 0.0f );
			}
			gi.G2API_SetBoneAngles( &self->ghoul2[0], "Bone_body", desiredAngles,
							BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time );
		}
	}

	if ( diffYaw || diffPitch )
	{
		self->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );
	}
	else
	{
		self->s.loopSound = 0;
	}
}

//-----------------------------------------------------
static void turret_turnoff( gentity_t *self )
//-----------------------------------------------------
{
	if ( self->enemy == NULL )
	{
		// we don't need to turnoff
		return;
	}

	if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
	{
		TurboLaser_SetBoneAnim( self, 4, 5 );
	}

	// shut-down sound
	G_Sound( self, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));

	// make turret play ping sound for 5 seconds
	self->aimDebounceTime = level.time + 5000;

	// Clear enemy
	self->enemy = NULL;
}

//-----------------------------------------------------
static qboolean turret_find_enemies( gentity_t *self )
//-----------------------------------------------------
{
	// HACK for t2_wedge!!!
	if ( self->spawnflags & SPF_TURRETG2_TURBO )
		return qfalse;

	qboolean	found = qfalse;
	int			count;
	float		bestDist = self->radius * self->radius;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;
	gentity_t	*entity_list[MAX_GENTITIES], *target, *bestTarget = NULL;

	if ( self->aimDebounceTime > level.time ) // time since we've been shut off
	{
		// We were active and alert, i.e. had an enemy in the last 3 secs
		if ( self->painDebounceTime < level.time )
		{
			G_Sound(self, G_SoundIndex( "sound/chars/turret/ping.wav" ));
			self->painDebounceTime = level.time + 1000;
		}
	}

	VectorCopy( self->currentOrigin, org2 );
	if ( self->spawnflags & 2 )
	{
		org2[2] += 20;
	}
	else
	{
		org2[2] -= 20;
	}

	count = G_RadiusList( org2, self->radius, self, qtrue, entity_list );

	for ( int i = 0; i < count; i++ )
	{
		target = entity_list[i];

		if ( !target->client )
		{
			// only attack clients
			continue;
		}
		if ( target == self || !target->takedamage || target->health <= 0 || ( target->flags & FL_NOTARGET ))
		{
			continue;
		}
		if ( target->client->playerTeam == self->noDamageTeam )
		{
			// A bot we don't want to shoot
			continue;
		}
		if ( !gi.inPVS( org2, target->currentOrigin ))
		{
			continue;
		}

		VectorCopy( target->client->renderInfo.eyePoint, org );

		if ( self->spawnflags & 2 )
		{
			org[2] -= 15;
		}
		else
		{
			org[2] += 5;
		}

		trace_t	tr;
		gi.trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

		if ( !tr.allsolid && !tr.startsolid && ( tr.fraction == 1.0 || tr.entityNum == target->s.number ))
		{
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract( target->currentOrigin, self->currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < bestDist )// all things equal, keep current
			{
				if ( self->attackDebounceTime < level.time )
				{
					// We haven't fired or acquired an enemy in the last 2 seconds-start-up sound
					G_Sound( self, G_SoundIndex( "sound/chars/turret/startup.wav" ));

					// Wind up turrets for a bit
					self->attackDebounceTime = level.time + 1400;
				}

				bestTarget = target;
				bestDist = enemyDist;
				found = qtrue;
			}
		}
	}

	if ( found )
	{
		if ( !self->enemy )
		{//just aquired one
			AddSoundEvent( bestTarget, self->currentOrigin, 256, AEL_DISCOVERED );
			AddSightEvent( bestTarget, self->currentOrigin, 512, AEL_DISCOVERED, 20 );
		}
		G_SetEnemy( self, bestTarget );
		if ( VALIDSTRING( self->target2 ))
		{
			G_UseTargets2( self, self, self->target2 );
		}
	}

	return found;
}

//-----------------------------------------------------
void turret_base_think( gentity_t *self )
//-----------------------------------------------------
{
	qboolean	turnOff = qtrue;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;

	self->nextthink = level.time + FRAMETIME;

	if ( self->spawnflags & 1 )
	{
		// not turned on
		turret_turnoff( self );
		turret_aim( self );

		// No target
		self->flags |= FL_NOTARGET;
		return;
	}
	else
	{
		// I'm all hot and bothered
		self->flags &= ~FL_NOTARGET;
	}

	if ( !self->enemy )
	{
		if ( turret_find_enemies( self ))
		{
			turnOff = qfalse;
		}
	}
	else
	{
		if ( self->enemy->health > 0 )
		{
			// enemy is alive
			VectorSubtract( self->enemy->currentOrigin, self->currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < self->radius * self->radius )
			{
				// was in valid radius
				if ( gi.inPVS( self->currentOrigin, self->enemy->currentOrigin ) )
				{
					// Every now and again, check to see if we can even trace to the enemy
					trace_t tr;

					if ( self->enemy->client )
					{
						VectorCopy( self->enemy->client->renderInfo.eyePoint, org );
					}
					else
					{
						VectorCopy( self->enemy->currentOrigin, org );
					}
					VectorCopy( self->currentOrigin, org2 );
					if ( self->spawnflags & 2 )
					{
						org2[2] += 10;
					}
					else
					{
						org2[2] -= 10;
					}
					gi.trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

					if ( self->spawnflags & SPF_TURRETG2_TURBO || ( !tr.allsolid && !tr.startsolid && tr.entityNum == self->enemy->s.number ) )
					{
						turnOff = qfalse;	// Can see our enemy
					}
				}
			}
		}

		turret_head_think( self );
	}

	if ( turnOff )
	{
		if ( self->bounceCount < level.time ) // bounceCount is used to keep the thing from ping-ponging from on to off
		{
			turret_turnoff( self );
		}
	}
	else
	{
		// keep our enemy for a minimum of 2 seconds from now
		self->bounceCount = level.time + 2000 + Q_flrand(0.0f, 1.0f) * 150;
	}

	turret_aim( self );
}

//-----------------------------------------------------------------------------
void turret_base_use( gentity_t *self, gentity_t *other, gentity_t *activator )
//-----------------------------------------------------------------------------
{
	// Toggle on and off
	self->spawnflags = (self->spawnflags ^ 1);

	if (( self->s.eFlags & EF_SHADER_ANIM ) && ( self->spawnflags & 1 )) // Start_Off
	{
		self->s.frame = 1; // black
	}
	else
	{
		self->s.frame = 0; // glow
	}
}

//special routine for tracking angles between client and server -rww
void turret_SetBoneAngles(gentity_t *ent, const char *bone, const vec3_t angles)
{
	/*
	int *thebone = &ent->s.boneIndex1;
	int *firstFree = NULL;
	int i = 0;
	int boneIndex = G_BoneIndex(bone);
	int flags;
	Eorientations up, right, forward;
	vec3_t *boneVector = &ent->s.boneAngles1;
	vec3_t *freeBoneVec = NULL;

	while (thebone)
	{
		if (!*thebone && !firstFree)
		{ //if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			firstFree = thebone;
			freeBoneVec = boneVector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{ //this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			boneVector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			boneVector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			boneVector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			boneVector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{ //didn't find it, create it
		if (!firstFree)
		{ //no free bones.. can't do a thing then.
			Com_Printf("WARNING: NPC has no free bone indexes\n");
			return;
		}

		thebone = firstFree;

		*thebone = boneIndex;
		boneVector = freeBoneVec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *boneVector);
*/
	//Now set the angles on our server instance if we have one.

	if ( !ent->ghoul2.size() )
	{
		return;
	}

	int flags = BONE_ANGLES_POSTMULT;
	Eorientations up, right, forward;
	up = POSITIVE_Y;
	right = NEGATIVE_Z;
	forward = NEGATIVE_X;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	//ent->s.boneOrient = ((forward)|(right<<3)|(up<<6));

	gi.G2API_SetBoneAngles( &ent->ghoul2[0], bone, angles, flags, up,
							right, forward, NULL, 100, level.time );
}

void turret_set_models( gentity_t *self, qboolean dying )
{
	if ( dying )
	{
		if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->s.modelindex = G_ModelIndex( name2 );
			self->s.modelindex2 = G_ModelIndex( name );
		}

		gi.G2API_RemoveGhoul2Model( self->ghoul2, 0 );
		/*G_KillG2Queue( self->s.number );
		self->s.modelGhoul2 = 0;

		gi.G2API_InitGhoul2Model( &self->ghoul2,
									name2,
									0, //base->s.modelindex,
									//note, this is not the same kind of index - this one's referring to the actual
									//index of the model in the g2 instance, whereas modelindex is the index of a
									//configstring -rww
									0,
									0,
									0,
									0);
		*/
	}
	else
	{
		if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->s.modelindex = G_ModelIndex( name );
			self->s.modelindex2 = G_ModelIndex( name2 );
			//set the new onw
			gi.G2API_InitGhoul2Model( self->ghoul2,
										name,
										0, //base->s.modelindex,
										//note, this is not the same kind of index - this one's referring to the actual
										//index of the model in the g2 instance, whereas modelindex is the index of a
										//configstring -rww
										0,
										0,
										0,
										0);
		}
		else
		{
			self->s.modelindex = G_ModelIndex( name3 );
			//set the new onw
			gi.G2API_InitGhoul2Model( self->ghoul2,
										name3,
										0, //base->s.modelindex,
										//note, this is not the same kind of index - this one's referring to the actual
										//index of the model in the g2 instance, whereas modelindex is the index of a
										//configstring -rww
										0,
										0,
										0,
										0);
		}

		/*self->s.modelGhoul2 = 1;
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{//larger
			self->s.g2radius = 128;
		}
		else
		{
			self->s.g2radius = 80;
		}*/

		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{//different pitch bone and muzzle flash points
			turret_SetBoneAngles(self, "pitch", vec3_origin);
			//self->genericValue11 = gi.G2API_AddBolt( self->ghoul2, 0, "*muzzle1" );
			//self->genericValue12 = gi.G2API_AddBolt( self->ghoul2, 0, "*muzzle2" );
		}
		else
		{
			turret_SetBoneAngles(self, "Bone_body", vec3_origin);
			//self->genericValue11 = gi.G2API_AddBolt( self->ghoul2, 0, "*flash03" );
		}
	}
}

/*QUAKED misc_turret (1 0 0) (-8 -8 -22) (8 8 0) START_OFF UPSIDE_DOWN TURBO
Turret that hangs from the ceiling, will aim and shoot at enemies

  START_OFF - Starts off
  UPSIDE_DOWN - make it rest on a surface/floor instead of hanging from the ceiling
  TURBO - Big-ass, Boxy Death Star Turbo Laser version

  radius - How far away an enemy can be for it to pick it up (default 512)
  wait	- Time between shots (default 150 ms)
  dmg	- How much damage each shot does (default 5)
  health - How much damage it can take before exploding (default 100)

  splashDamage - How much damage the explosion does
  splashRadius - The radius of the explosion
  NOTE: If either of the above two are 0, it will not make an explosion

  targetname - Toggles it on/off
  target - What to use when destroyed
  target2 - What to use when it decides to start shooting at an enemy

  team - team that is not targeted by and does not take damage from this turret
	"player",
	"enemy",	(default)
	"neutral"
*/
//-----------------------------------------------------
void SP_misc_turret( gentity_t *base )
//-----------------------------------------------------
{
	/*base->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/turret_canon.glm" );
	base->s.modelindex2 = G_ModelIndex( "models/map_objects/imp_mine/turret_damage.md3" );
	base->playerModel = gi.G2API_InitGhoul2Model( base->ghoul2, "models/map_objects/imp_mine/turret_canon.glm", base->s.modelindex );
	base->s.radius = 80.0f;*/
	turret_set_models( base, qfalse );

	gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "Bone_body", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );
	base->torsoBolt = gi.G2API_AddBolt( &base->ghoul2[base->playerModel], "*flash03" );

	finish_spawning_turret( base );

	if (( base->spawnflags & 1 )) // Start_Off
	{
		base->s.frame = 1; // black
	}
	else
	{
		base->s.frame = 0; // glow
	}
	base->s.eFlags |= EF_SHADER_ANIM;
}

//-----------------------------------------------------
void finish_spawning_turret( gentity_t *base )
{
	vec3_t		fwd;

	if ( base->spawnflags & 2 )
	{
		base->s.angles[ROLL] += 180;
		base->s.origin[2] -= 22.0f;
	}

	G_SetAngles( base, base->s.angles );
	AngleVectors( base->currentAngles, fwd, NULL, NULL );

	G_SetOrigin(base, base->s.origin);

	base->noDamageTeam = TEAM_ENEMY;

	base->s.eType = ET_GENERAL;

	if ( base->team && base->team[0] )
	{
		base->noDamageTeam = (team_t)GetIDForString( TeamTable, base->team );
		base->team = NULL;
	}

	// Set up our explosion effect for the ExplodeDeath code....
	base->fxID = G_EffectIndex( "turret/explode" );
	G_EffectIndex( "sparks/spark_exp_nosnd" );

	base->e_UseFunc = useF_turret_base_use;
	base->e_PainFunc = painF_TurretPain;

	// don't start working right away
	base->e_ThinkFunc = thinkF_turret_base_think;
	base->nextthink = level.time + FRAMETIME * 5;

	// this is really the pitch angle.....
	base->speed = 0;

	G_SpawnFloat( "shotspeed", "0", &base->mass );
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 4000;
		}

		if ( !base->health )
		{
			base->health = 2000;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 32768;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 500;// + Q_flrand(0.0f, 1.0f) * 500;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 200;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 500;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 10;
		}

		VectorSet( base->s.modelScale, 2.0f, 2.0f, 2.0f );
		VectorSet( base->maxs, 128.0f, 128.0f, 120.0f );
		VectorSet( base->mins, -128.0f, -128.0f, -120.0f );

		// Cull Radius.
		base->s.radius = 256;

		//start in "off" anim
		TurboLaser_SetBoneAnim( base, 4, 5 );

		// Make sure it doesn't do sparks and such when saber contacts with it.
		base->flags = FL_DMG_BY_HEAVY_WEAP_ONLY;
		base->takedamage = qfalse;
		base->contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;

		base->noDamageTeam = TEAM_NEUTRAL;
		base->team = NULL;
	}
	else
	{
		// this is a random time offset for the no-enemy-search-around-mode
		base->count = Q_flrand(0.0f, 1.0f) * 9000;

		if ( !base->health )
		{
			base->health = 100;
		}

		// search radius
		if ( !base->radius )
		{
			base->radius = 512;
		}

		// How quickly to fire
		if ( !base->wait )
		{
			base->wait = 150 + Q_flrand(0.0f, 1.0f) * 55;
		}

		if ( !base->splashDamage )
		{
			base->splashDamage = 10;
		}

		if ( !base->splashRadius )
		{
			base->splashRadius = 25;
		}

		// how much damage each shot does
		if ( !base->damage )
		{
			base->damage = 5;
		}

		if ( base->spawnflags & 2 )
		{//upside-down, invert mins and maxe
			VectorSet( base->maxs, 10.0f, 10.0f, 30.0f );
			VectorSet( base->mins, -10.0f, -10.0f, 0.0f );
		}
		else
		{
			VectorSet( base->maxs, 10.0f, 10.0f, 0.0f );
			VectorSet( base->mins, -10.0f, -10.0f, -30.0f );
		}

		base->takedamage = qtrue;
		base->contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;
	}

	// Precache special FX and moving sounds
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		G_EffectIndex( "turret/turb_muzzle_flash" );
		G_EffectIndex( "turret/turb_shot" );
		G_EffectIndex( "turret/turb_impact" );
		//FIXME: Turbo Laser Cannon sounds!
		G_SoundIndex( "sound/vehicles/weapons/turbolaser/turn.wav" );
		G_EffectIndex( "explosions/fighter_explosion2" );
		RegisterItem( FindItemForWeapon( WP_TIE_FIGHTER ));
	}
	else
	{
		// Precache moving sounds
		G_SoundIndex( "sound/chars/turret/startup.wav" );
		G_SoundIndex( "sound/chars/turret/shutdown.wav" );
		G_SoundIndex( "sound/chars/turret/ping.wav" );
		G_SoundIndex( "sound/chars/turret/move.wav" );
	}

	base->max_health = base->health;
	base->e_DieFunc  = dieF_turret_die;

	base->material = MAT_METAL;

	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		RegisterItem( FindItemForWeapon( WP_TURRET ));

		base->svFlags |= SVF_NO_TELEPORT|SVF_SELF_ANIMATING;
	}
	else
	{
		// Register this so that we can use it for the missile effect
		RegisterItem( FindItemForWeapon( WP_BLASTER ));

		base->svFlags |= SVF_NO_TELEPORT|SVF_NONNPC_ENEMY|SVF_SELF_ANIMATING;
	}

	// But set us as a turret so that we can be identified as a turret
	base->s.weapon = WP_TURRET;

	gi.linkentity( base );
}

/*QUAKED misc_ns_turret (1 0 0) (-8 -8 -32) (8 8 29) START_OFF
NS turret that only hangs from the ceiling, will aim and shoot at enemies

  START_OFF - Starts off

  radius - How far away an enemy can be for it to pick it up (default 512)
  wait	- Time between shots (default 150 ms)
  dmg	- How much damage each shot does (default 5)
  health - How much damage it can take before exploding (default 100)

  splashDamage - How much damage the explosion does
  splashRadius - The radius of the explosion
  NOTE: If either of the above two are 0, it will not make an explosion

  targetname - Toggles it on/off
  target - What to use when destroyed

  team - team that is not targeted by and does not take damage from this turret
	"player",
	"enemy",	(default)
	"neutral"
*/
//-----------------------------------------------------
void SP_misc_ns_turret( gentity_t *base )
//-----------------------------------------------------
{
	base->s.modelindex = G_ModelIndex( "models/map_objects/nar_shaddar/turret/turret.glm" );
	base->s.modelindex2 = G_ModelIndex( "models/map_objects/imp_mine/turret_damage.md3" ); // FIXME!
	base->playerModel = gi.G2API_InitGhoul2Model( base->ghoul2, "models/map_objects/nar_shaddar/turret/turret.glm", base->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
	base->s.radius = 80.0f;

	gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "Bone_body", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );
	base->torsoBolt = gi.G2API_AddBolt( &base->ghoul2[base->playerModel], "*flash02" );

	finish_spawning_turret( base );
}

//--------------------------------------

void laser_arm_fire (gentity_t *ent)
{
	vec3_t	start, end, fwd, rt, up;
	trace_t	trace;

	if ( ent->attackDebounceTime < level.time && ent->alt_fire )
	{
		// If I'm firing the laser and it's time to quit....then quit!
		ent->alt_fire = qfalse;
//		ent->e_ThinkFunc = thinkF_NULL;
//		return;
	}

	ent->nextthink = level.time + FRAMETIME;

	// If a fool gets in the laser path, fry 'em
	AngleVectors( ent->currentAngles, fwd, rt, up );

	VectorMA( ent->currentOrigin, 20, fwd, start );
	//VectorMA( start, -6, rt, start );
	//VectorMA( start, -3, up, start );
	VectorMA( start, 4096, fwd, end );

	gi.trace( &trace, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SHOT, (EG2_Collision)0, 0 );//ignore
	ent->fly_sound_debounce_time = level.time;//used as lastShotTime

	// Only deal damage when in alt-fire mode
	if ( trace.fraction < 1.0 && ent->alt_fire )
	{
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{
			gentity_t *hapless_victim = &g_entities[trace.entityNum];
			if ( hapless_victim && hapless_victim->takedamage && ent->damage )
			{
				G_Damage( hapless_victim, ent, ent->nextTrain->activator, fwd, trace.endpos, ent->damage, DAMAGE_IGNORE_TEAM, MOD_UNKNOWN );
			}
		}
	}

	if ( ent->alt_fire )
	{
//		CG_FireLaser( start, trace.endpos, trace.plane.normal, ent->nextTrain->startRGBA, qfalse );
	}
	else
	{
//		CG_AimLaser( start, trace.endpos, trace.plane.normal );
	}
}

void laser_arm_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	vec3_t	newAngles;

	self->activator = activator;
	switch( self->count )
	{
	case 0:
	default:
		//Fire
		//gi.Printf("FIRE!\n");
//		self->lastEnemy->lastEnemy->e_ThinkFunc = thinkF_laser_arm_fire;
//		self->lastEnemy->lastEnemy->nextthink = level.time + FRAMETIME;
		//For 3 seconds
		self->lastEnemy->lastEnemy->alt_fire = qtrue; // Let 'er rip!
		self->lastEnemy->lastEnemy->attackDebounceTime = level.time + self->lastEnemy->lastEnemy->wait;
		G_Sound(self->lastEnemy->lastEnemy, G_SoundIndex("sound/chars/l_arm/fire.wav"));
		break;
	case 1:
		//Yaw left
		//gi.Printf("LEFT...\n");
		VectorCopy( self->lastEnemy->currentAngles, newAngles );
		newAngles[1] += self->speed;
		G_SetAngles( self->lastEnemy, newAngles );
//		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, LARM_FOFS, LARM_ROFS, LARM_UOFS );
		G_Sound( self->lastEnemy, G_SoundIndex( "sound/chars/l_arm/move.wav" ) );
		break;
	case 2:
		//Yaw right
		//gi.Printf("RIGHT...\n");
		VectorCopy( self->lastEnemy->currentAngles, newAngles );
		newAngles[1] -= self->speed;
		G_SetAngles( self->lastEnemy, newAngles );
//		bolt_head_to_arm( self->lastEnemy, self->lastEnemy->lastEnemy, LARM_FOFS, LARM_ROFS, LARM_UOFS );
		G_Sound( self->lastEnemy, G_SoundIndex( "sound/chars/l_arm/move.wav" ) );
		break;
	case 3:
		//pitch up
		//gi.Printf("UP...\n");
		//FIXME: Clamp
		VectorCopy( self->lastEnemy->lastEnemy->currentAngles, newAngles );
		newAngles[0] -= self->speed;
		if ( newAngles[0] < -45 )
		{
			newAngles[0] = -45;
		}
		G_SetAngles( self->lastEnemy->lastEnemy, newAngles );
		G_Sound( self->lastEnemy->lastEnemy, G_SoundIndex( "sound/chars/l_arm/move.wav" ) );
		break;
	case 4:
		//pitch down
		//gi.Printf("DOWN...\n");
		//FIXME: Clamp
		VectorCopy( self->lastEnemy->lastEnemy->currentAngles, newAngles );
		newAngles[0] += self->speed;
		if ( newAngles[0] > 90 )
		{
			newAngles[0] = 90;
		}
		G_SetAngles( self->lastEnemy->lastEnemy, newAngles );
		G_Sound( self->lastEnemy->lastEnemy, G_SoundIndex( "sound/chars/l_arm/move.wav" ) );
		break;
	}
}
/*QUAKED misc_laser_arm (1 0 0) (-8 -8 -8) (8 8 8)

What it does when used depends on it's "count" (can be set by a script)
	count:
		0 (default) - Fire in direction facing
		1 turn left
		2 turn right
		3 aim up
		4 aim down

  speed - How fast it turns (degrees per second, default 30)
  dmg	- How much damage the laser does 10 times a second (default 5 = 50 points per second)
  wait  - How long the beam lasts, in seconds (default is 3)

  targetname - to use it
  target - What thing for it to be pointing at to start with

  "startRGBA" - laser color, Red Green Blue Alpha, range 0 to 1 (default  1.0 0.85 0.15 0.75 = Yellow-Orange)
*/
void laser_arm_start (gentity_t *base)
{
	vec3_t	armAngles;
	vec3_t	headAngles;

	base->e_ThinkFunc = thinkF_NULL;
	//We're the base, spawn the arm and head
	gentity_t *arm = G_Spawn();
	gentity_t *head = G_Spawn();

	VectorCopy( base->s.angles, armAngles );
	VectorCopy( base->s.angles, headAngles );
	if ( base->target && base->target[0] )
	{//Start out pointing at something
		gentity_t *targ = G_Find( NULL, FOFS(targetname), base->target );
		if ( !targ )
		{//couldn't find it!
			Com_Printf(S_COLOR_RED "ERROR : laser_arm can't find target %s!\n", base->target);
		}
		else
		{//point at it
			vec3_t	dir, angles;

			VectorSubtract(targ->currentOrigin, base->s.origin, dir );
			vectoangles( dir, angles );
			armAngles[1] = angles[1];
			headAngles[0] = angles[0];
			headAngles[1] = angles[1];
		}
	}

	//Base
	//Base does the looking for enemies and pointing the arm and head
	G_SetAngles( base, base->s.angles );
	//base->s.origin[2] += 4;
	G_SetOrigin(base, base->s.origin);
	gi.linkentity(base);
	//FIXME: need an actual model
	base->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_base.md3");
	base->s.eType = ET_GENERAL;
	G_SpawnVector4( "startRGBA", "1.0 0.85 0.15 0.75", (float *)&base->startRGBA );
	//anglespeed - how fast it can track the player, entered in degrees per second, so we divide by FRAMETIME/1000
	if ( !base->speed )
	{
		base->speed = 3.0f;
	}
	else
	{
		base->speed *= FRAMETIME/1000.0f;
	}
	base->e_UseFunc = useF_laser_arm_use;
	base->nextthink = level.time + FRAMETIME;

	//Arm
	//Does nothing, not solid, gets removed when head explodes
	G_SetOrigin( arm, base->s.origin );
	gi.linkentity(arm);
	G_SetAngles( arm, armAngles );
//	bolt_head_to_arm( arm, head, LARM_FOFS, LARM_ROFS, LARM_UOFS );
	arm->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_arm.md3");

	//Head
	//Fires when enemy detected, animates, can be blown up
	//Need to normalize the headAngles pitch for the clamping later
	if ( headAngles[0] < -180 )
	{
		headAngles[0] += 360;
	}
	else if ( headAngles[0] > 180 )
	{
		headAngles[0] -= 360;
	}
	G_SetAngles( head, headAngles );
	head->s.modelindex = G_ModelIndex("models/mapobjects/dn/laser_head.md3");
	head->s.eType = ET_GENERAL;
//	head->svFlags |= SVF_BROADCAST;// Broadcast to all clients
	VectorSet( head->mins, -8, -8, -8 );
	VectorSet( head->maxs, 8, 8, 8 );
	head->contents = CONTENTS_BODY;
	gi.linkentity(head);

	//dmg
	if ( !base->damage )
	{
		head->damage = 5;
	}
	else
	{
		head->damage = base->damage;
	}
	base->damage = 0;
	//lifespan of beam
	if ( !base->wait )
	{
		head->wait = 3000;
	}
	else
	{
		head->wait = base->wait * 1000;
	}
	base->wait = 0;

	//Precache firing and explode sounds
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_SoundIndex("sound/chars/l_arm/fire.wav");
	G_SoundIndex("sound/chars/l_arm/move.wav");

	//Link them up
	base->lastEnemy = arm;
	arm->lastEnemy = head;
	head->owner = arm;
	arm->nextTrain = head->nextTrain = base;

	// The head should always think, since it will be either firing a damage laser or just a target laser
	head->e_ThinkFunc = thinkF_laser_arm_fire;
	head->nextthink = level.time + FRAMETIME;
	head->alt_fire = qfalse; // Don't do damage until told to
}

void SP_laser_arm (gentity_t *base)
{
	base->e_ThinkFunc = thinkF_laser_arm_start;
	base->nextthink = level.time + START_TIME_LINK_ENTS;
}

//--------------------------
// PERSONAL ASSAULT SENTRY
//--------------------------

#define PAS_DAMAGE	2

//-----------------------------------------------------------------------------
void pas_use( gentity_t *self, gentity_t *other, gentity_t *activator )
//-----------------------------------------------------------------------------
{
	// Toggle on and off
	self->spawnflags = (self->spawnflags ^ 1);

	if ( self->spawnflags & 1 )
	{
		self->nextthink = 0; // turn off and do nothing
		self->e_ThinkFunc = thinkF_NULL;
	}
	else
	{
		self->nextthink = level.time + 50;
		self->e_ThinkFunc = thinkF_pas_think;
	}
}

//----------------------------------------------------------------
void pas_fire( gentity_t *ent )
//----------------------------------------------------------------
{
	vec3_t		fwd, org;
	mdxaBone_t	boltMatrix;

	// Getting the flash bolt here
	gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel,
				ent->torsoBolt,
				&boltMatrix, ent->currentAngles, ent->s.origin, (cg.time?cg.time:level.time),
				NULL, ent->s.modelScale );

	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, fwd );

	G_PlayEffect( "turret/muzzle_flash", org, fwd );

	gentity_t	*bolt;

	bolt = G_Spawn();

	bolt->classname = "turret_proj";
	bolt->nextthink = level.time + 10000;
	bolt->e_ThinkFunc = thinkF_G_FreeEntity;
	bolt->s.eType = ET_MISSILE;
	bolt->s.weapon = WP_TURRET;
	bolt->owner = ent;
	bolt->damage = PAS_DAMAGE;
	bolt->dflags = DAMAGE_NO_KNOCKBACK;		// Don't push them around, or else we are constantly re-aiming
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_ENERGY;
	bolt->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	VectorSet( bolt->maxs, 1, 1, 1 );
	VectorScale( bolt->maxs, -1, bolt->mins );

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( org, bolt->s.pos.trBase );
	VectorScale( fwd, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );		// save net bandwidth
	VectorCopy( org, bolt->currentOrigin);
}

//-----------------------------------------------------
static qboolean pas_find_enemies( gentity_t *self )
//-----------------------------------------------------
{
	qboolean	found = qfalse;
	int			count;
	float		bestDist = self->radius * self->radius;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;
	gentity_t	*entity_list[MAX_GENTITIES], *target;

	if ( self->aimDebounceTime > level.time ) // time since we've been shut off
	{
		// We were active and alert, i.e. had an enemy in the last 3 secs
		if ( self->painDebounceTime < level.time )
		{
			G_Sound(self, G_SoundIndex( "sound/chars/turret/ping.wav" ));
			self->painDebounceTime = level.time + 1000;
		}
	}

	mdxaBone_t	boltMatrix;

	// Getting the "eye" here
	gi.G2API_GetBoltMatrix( self->ghoul2, self->playerModel,
				self->torsoBolt,
				&boltMatrix, self->currentAngles, self->s.origin, (cg.time?cg.time:level.time),
				NULL, self->s.modelScale );

	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org2 );

	count = G_RadiusList( org2, self->radius, self, qtrue, entity_list );

	for ( int i = 0; i < count; i++ )
	{
		target = entity_list[i];

		if ( !target->client )
		{
			continue;
		}
		if ( target == self || !target->takedamage || target->health <= 0 || ( target->flags & FL_NOTARGET ))
		{
			continue;
		}
		if ( target->client->playerTeam == self->noDamageTeam )
		{
			// A bot we don't want to shoot
			continue;
		}
		if ( !gi.inPVS( org2, target->currentOrigin ))
		{
			continue;
		}

		if ( target->client )
		{
			VectorCopy( target->client->renderInfo.eyePoint, org );
			org[2] -= 15;
		}
		else
		{
			VectorCopy( target->currentOrigin, org );
		}

		trace_t	tr;
		gi.trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

		if ( !tr.allsolid && !tr.startsolid && ( tr.fraction == 1.0 || tr.entityNum == target->s.number ))
		{
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract( target->currentOrigin, self->currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( target->s.number ) // don't do this for the player
			{
				G_StartFlee( target, self, self->currentOrigin, AEL_DANGER, 3000, 5000 );
			}

			if ( enemyDist < bestDist )// all things equal, keep current
			{
				if ( self->attackDebounceTime + 2000 < level.time )
				{
					// We haven't fired or acquired an enemy in the last 2 seconds-start-up sound
					G_Sound( self, G_SoundIndex( "sound/chars/turret/startup.wav" ));

					// Wind up turrets for a bit
					self->attackDebounceTime = level.time + 900 + Q_flrand(0.0f, 1.0f) * 200;
				}

				G_SetEnemy( self, target );
				bestDist = enemyDist;
				found = qtrue;
			}
		}
	}

	if ( found && VALIDSTRING( self->target2 ))
	{
		G_UseTargets2( self, self, self->target2 );
	}

	return found;
}

//---------------------------------
void pas_adjust_enemy( gentity_t *ent )
//---------------------------------
{
	qboolean keep = qtrue;

	if ( ent->enemy->health <= 0 )
	{
		keep = qfalse;
	}
	else// if ( Q_flrand(0.0f, 1.0f) > 0.5f )
	{
		// do a trace every now and then.
		mdxaBone_t	boltMatrix;
		vec3_t		org, org2;

		// Getting the "eye" here
		gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel,
					ent->torsoBolt,
					&boltMatrix, ent->currentAngles, ent->s.origin, (cg.time?cg.time:level.time),
					NULL, ent->s.modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org2 );

		if ( ent->enemy->client )
		{
			VectorCopy( ent->enemy->client->renderInfo.eyePoint, org );
			org[2] -= 15;
		}
		else
		{
			VectorCopy( ent->enemy->currentOrigin, org );
		}

		trace_t	tr;
		gi.trace( &tr, org2, NULL, NULL, org, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

		if ( tr.allsolid || tr.startsolid || tr.entityNum != ent->enemy->s.number )
		{
			// trace failed
			keep = qfalse;
		}
	}

	if ( keep )
	{
		ent->bounceCount = level.time + 500 + Q_flrand(0.0f, 1.0f) * 150;
	}
	else if ( ent->bounceCount < level.time ) // don't ping pong on and off
	{
		ent->enemy = NULL;
		// shut-down sound
		G_Sound( ent, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));

		// make turret play ping sound for 5 seconds
		ent->aimDebounceTime = level.time + 5000;
	}
}

//---------------------------------
void pas_think( gentity_t *ent )
//---------------------------------
{
	if ( !ent->damage )
	{
		// let us do our animation, then we are good to go in terms of pounding the crap out of enemies.
		ent->damage = 1;
		gi.G2API_SetBoneAnimIndex( &ent->ghoul2[ent->playerModel], ent->rootBone, 0, 11, BONE_ANIM_OVERRIDE_FREEZE, 0.8f, cg.time, -1, -1 );
		ent->nextthink = level.time + 1200;
		return;
	}

	if ( !ent->count )
	{
		// turrets that have no ammo may as well do nothing
		return;
	}

	ent->nextthink = level.time + FRAMETIME;

	if ( ent->enemy )
	{
		// make sure that the enemy is still valid
		pas_adjust_enemy( ent );
	}

	if ( !ent->enemy )
	{
		pas_find_enemies( ent );
	}

	qboolean	moved = qfalse;
	float		diffYaw = 0.0f, diffPitch = 0.0f;
	vec3_t		enemyDir, org;
	vec3_t		frontAngles, backAngles;
	vec3_t		desiredAngles;

	ent->speed = AngleNormalize360( ent->speed );
	ent->random = AngleNormalize360( ent->random );

	if ( ent->enemy )
	{
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		if ( ent->enemy->client )
		{
			VectorCopy( ent->enemy->client->renderInfo.eyePoint, org );
			org[2] -= 40;
		}
		else
		{
			VectorCopy( ent->enemy->currentOrigin, org );
		}

		VectorSubtract( org, ent->currentOrigin, enemyDir );
		vectoangles( enemyDir, desiredAngles );

		diffYaw = AngleSubtract( ent->speed, desiredAngles[YAW] );
		diffPitch = AngleSubtract( ent->random, desiredAngles[PITCH] );
	}
	else
	{
		// no enemy, so make us slowly sweep back and forth as if searching for a new one
		diffYaw = sin( level.time * 0.0001f + ent->count ) * 2.0f;
	}

	if ( fabs(diffYaw) > 0.25f )
	{
		moved = qtrue;

		if ( fabs(diffYaw) > 10.0f )
		{
			// cap max speed
			ent->speed += (diffYaw > 0.0f) ? -10.0f : 10.0f;
		}
		else
		{
			// small enough
			ent->speed -= diffYaw;
		}
	}


	if ( fabs(diffPitch) > 0.25f )
	{
		moved = qtrue;

		if ( fabs(diffPitch) > 4.0f )
		{
			// cap max speed
			ent->random += (diffPitch > 0.0f) ? -4.0f : 4.0f;
		}
		else
		{
			// small enough
			ent->random -= diffPitch;
		}
	}

	// the bone axes are messed up, so hence some dumbness here
	VectorSet( frontAngles, -ent->random, 0.0f, 0.0f );
	VectorSet( backAngles, 0.0f, 0.0f, ent->speed - ent->s.angles[YAW] );

	gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "bone_barrel", frontAngles,
						BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, NEGATIVE_X, NULL,100,cg.time);
	gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "bone_gback", frontAngles,
						BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, NEGATIVE_X, NULL,100,cg.time);
	gi.G2API_SetBoneAngles( &ent->ghoul2[ent->playerModel], "bone_hinge", backAngles,
						BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL,100,cg.time);

	if ( moved )
	{
	//ent->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );
	}
	else
	{
		ent->s.loopSound = 0;
	}

	if ( ent->enemy && ent->attackDebounceTime < level.time && Q_flrand(0.0f, 1.0f) > 0.3f )
	{
		ent->count--;

		if ( ent->count )
		{
			pas_fire( ent );
			ent->fly_sound_debounce_time = level.time;//used as lastShotTime
		}
		else
		{
			ent->nextthink = 0;
			G_Sound( ent, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
		}
	}
}

/*QUAKED misc_sentry_turret (1 0 0) (-16 -16 0) (16 16 24) START_OFF RESERVED
personal assault sentry, like the ones you can carry in your inventory

  RESERVED - do no use this flag for anything, does nothing..etc.

  radius - How far away an enemy can be for it to pick it up (default 512)
  count - number of shots before thing deactivates. -1 = infinite, default 150
  health - How much damage it can take before exploding (default 50)

  splashDamage - How much damage the explosion does
  splashRadius - The radius of the explosion
  NOTE: If either of the above two are 0, it will not make an explosion

  target - What to use when destroyed
  target2 - What to use when it decides to fire at an enemy

  team - team that does not take damage from this item
	"player",
	"enemy",
	"neutral"

*/

//---------------------------------
void SP_PAS( gentity_t *base )
//---------------------------------
{
	base->classname = "PAS";
	G_SetOrigin( base, base->s.origin );
	G_SetAngles( base, base->s.angles );

	base->speed = base->s.angles[YAW];

	base->s.modelindex = G_ModelIndex( "models/items/psgun.glm" );
	base->playerModel = gi.G2API_InitGhoul2Model( base->ghoul2, "models/items/psgun.glm", base->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
	base->s.radius = 30.0f;
	VectorSet( base->s.modelScale, 1.0f, 1.0f, 1.0f );

	base->rootBone = gi.G2API_GetBoneIndex( &base->ghoul2[base->playerModel], "model_root", qtrue );
	gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "bone_hinge", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );
	gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "bone_gback", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );
	gi.G2API_SetBoneAngles( &base->ghoul2[base->playerModel], "bone_barrel", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 );

	base->torsoBolt = gi.G2API_AddBolt( &base->ghoul2[base->playerModel], "*flash02" );

	base->s.eType = ET_GENERAL;

	if ( !base->radius )
	{
		base->radius = 512;
	}

	if ( base->count == 0 )
	{
		// give ammo
		base->count = 150;
	}

	base->e_UseFunc = useF_pas_use;

	base->damage = 0; // start animation flag

	base->contents = CONTENTS_SHOTCLIP|CONTENTS_CORPSE;//for certain traces
	VectorSet( base->mins, -8, -8, 0 );
	VectorSet( base->maxs, 8, 8, 18 );

	if ( !(base->spawnflags & 1 )) // START_OFF
	{
		base->nextthink = level.time + 1000; // we aren't starting off, so start working right away
		base->e_ThinkFunc = thinkF_pas_think;
	}

	// Set up our explosion effect for the ExplodeDeath code....
	base->fxID = G_EffectIndex( "turret/explode" );
	G_EffectIndex( "sparks/spark_exp_nosnd" );

	if ( !base->health )
	{
		base->health = 50;
	}
	base->max_health = base->health;

	base->takedamage = qtrue;
	base->e_PainFunc = painF_TurretPain;
	base->e_DieFunc  = dieF_turret_die;

	// hack this flag on so that when it calls the turret die code, it will orient the effect up
	// HACK
	//--------------------------------------
	base->spawnflags |= 2;

	// Use this for our missile effect
	RegisterItem( FindItemForWeapon( WP_TURRET ));
	base->s.weapon = WP_TURRET;

	base->svFlags |= SVF_NONNPC_ENEMY;

	base->noDamageTeam = TEAM_NEUTRAL;
	if ( base->team && base->team[0] )
	{
		base->noDamageTeam = (team_t)GetIDForString( TeamTable, base->team );
		base->team = NULL;
	}

	gi.linkentity( base );
}

//------------------------------------------------------------------------
qboolean place_portable_assault_sentry( gentity_t *self, vec3_t origin, vec3_t angs )
//------------------------------------------------------------------------
{
	vec3_t		fwd, pos;
	vec3_t		mins, maxs;
	trace_t		tr;
	gentity_t	*pas;

	VectorSet( maxs, 9, 9, 0 );
	VectorScale( maxs, -1, mins );

	angs[PITCH] = 0;
	angs[ROLL] = 0;
	AngleVectors( angs, fwd, NULL, NULL );

	// and move a consistent distance away from us so we don't have the dumb thing spawning inside of us.
	VectorMA( origin, 30, fwd, pos );
	gi.trace( &tr, origin, NULL, NULL, pos, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	// find the ground
	tr.endpos[2] += 20;
	VectorCopy( tr.endpos, pos );
	pos[2] -= 64;

	gi.trace( &tr, tr.endpos, mins, maxs, pos, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	// check for a decent surface, meaning mostly flat...should probably also check surface parms so we don't set us down on lava or something.
	if ( !tr.startsolid && !tr.allsolid && tr.fraction < 1.0f && tr.plane.normal[2] > 0.9f && tr.entityNum >= ENTITYNUM_WORLD )
	{
		// Then spawn us if it seems cool.
		pas = G_Spawn();

		if ( pas )
		{
			VectorCopy( tr.endpos, pas->s.origin );
			SP_PAS( pas );

			pas->contents |= CONTENTS_PLAYERCLIP; // player placed ones can block players but not npcs

			pas->e_UseFunc = useF_NULL; // placeable ones never need to be used

			// we don't hurt us or anyone who belongs to the same team as us.
			if ( self->client )
			{
				pas->noDamageTeam = self->client->playerTeam;
			}

			G_Sound( self, G_SoundIndex( "sound/player/use_sentry" ));
			pas->activator = self;
			return qtrue;
		}
	}
	return qfalse;
}


//-------------
// ION CANNON
//-------------


//----------------------------------------
void ion_cannon_think( gentity_t *self )
//----------------------------------------
{
	if ( self->spawnflags & 2 )
	{
		if ( self->count )
		{
			// still have bursts left, so keep going
			self->count--;
		}
		else
		{
			// done with burst, so wait delay amount, plus a random bit
			self->nextthink = level.time + ( self->delay + Q_flrand(-1.0f, 1.0f) * self->random );
			self->count = Q_irand(0,5); // 0-5 bursts

			// Not firing this time
			return;
		}
	}

	if ( self->fxID )
	{
		vec3_t		fwd, org;
		mdxaBone_t	boltMatrix;

		// Getting the flash bolt here
		gi.G2API_GetBoltMatrix( self->ghoul2, self->playerModel,
					self->torsoBolt,
					&boltMatrix, self->s.angles, self->s.origin, (cg.time?cg.time:level.time),
					NULL, self->s.modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, fwd );

		G_PlayEffect( self->fxID, org, fwd );
	}

	if ( self->target2 )
	{
		// If we have a target2 fire it off in sync with our gun firing
		G_UseTargets2( self, self, self->target2 );
	}

	gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone, 0, 8, BONE_ANIM_OVERRIDE_FREEZE, 0.6f, cg.time, -1, -1 );
	self->nextthink = level.time + self->wait + Q_flrand(-1.0f, 1.0f) * self->random;
}

//----------------------------------------------------------------------------------------------
void ion_cannon_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
//----------------------------------------------------------------------------------------------
{
	vec3_t org;

	// dead, so nuke the ghoul model and put in the damage md3 version
	if ( self->playerModel >= 0 )
	{
		gi.G2API_RemoveGhoul2Model( self->ghoul2, self->playerModel );
	}
	self->s.modelindex = self->s.modelindex2;
	self->s.modelindex2 = 0;

		// Turn off the thinking of the base & use it's targets
	self->e_ThinkFunc = thinkF_NULL;
	self->e_UseFunc = useF_NULL;

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	// clear my data
	self->e_DieFunc  = dieF_NULL;
	self->takedamage = qfalse;
	self->health = 0;

	self->takedamage = qfalse;//stop chain reaction runaway loops
	self->s.loopSound = 0;

	// not solid anymore
	self->contents = 0;

	VectorCopy( self->currentOrigin, self->s.pos.trBase );

	VectorCopy( self->currentOrigin, org );
	org[2] += 20;
	G_PlayEffect( "env/ion_cannon_explosion", org );

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius,
				attacker, MOD_UNKNOWN );
	}

	gi.linkentity( self );
}

//----------------------------------------------------------------------------
void ion_cannon_use( gentity_t *self, gentity_t *other, gentity_t *activator )
//----------------------------------------------------------------------------
{
	// toggle
	if ( self->e_ThinkFunc == thinkF_NULL )
	{
		// start thinking now
		self->e_ThinkFunc = thinkF_ion_cannon_think;
		self->nextthink = level.time + FRAMETIME; // fires right on being used
	}
	else
	{
		self->e_ThinkFunc = thinkF_NULL;
	}
}

/*QUAKED misc_ion_cannon (1 0 0) (-280 -280 0) (280 280 640) START_OFF BURSTS SHIELDED
Huge ion cannon, like the ones at the rebel base on Hoth.

  START_OFF - Starts off
  BURSTS - adds more variation, shots come out in bursts
  SHIELDED - cannon is shielded, any kind of shot bounces off.

  wait	- How fast it shoots (default 1500 ms between shots, can't be less than 500 ms)
  random - milliseconds wait variation (default 400 ms...up to plus or minus .4 seconds)
  delay	- Number of milliseconds between bursts (default 6000 ms, can't be less than 1000 ms, only works when BURSTS checked)

  health - default 2000
  splashDamage - how much damage to do when it dies, must be greater than 0 to actually work
  splashRadius - damage radius, must be greater than 0 to actually work

  targetname - Toggles it on/off
  target - What to use when destroyed
  target2 - What to use when it fires a shot.
*/
//-----------------------------------------------------
void SP_misc_ion_cannon( gentity_t *base )
//-----------------------------------------------------
{
	G_SetAngles( base, base->s.angles );

	G_SetOrigin(base, base->s.origin);

	base->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/ion_cannon.glm" );
	base->playerModel = gi.G2API_InitGhoul2Model( base->ghoul2, "models/map_objects/imp_mine/ion_cannon.glm", base->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
	base->s.radius = 320.0f;
	VectorSet( base->s.modelScale, 2.0f, 2.0f, 2.0f );

	base->rootBone = gi.G2API_GetBoneIndex( &base->ghoul2[base->playerModel], "model_root", qtrue );
	base->torsoBolt = gi.G2API_AddBolt( &base->ghoul2[base->playerModel], "*flash02" );

	// register damage model
	base->s.modelindex2 = G_ModelIndex( "models/map_objects/imp_mine/ion_cannon_damage.md3" );

	base->e_UseFunc = useF_ion_cannon_use;

	// How quickly to fire
	if ( base->wait == 0.0f )
	{
		base->wait = 1500.0f;
	}
	else if ( base->wait < 500.0f )
	{
		base->wait = 500.0f;
	}

	if ( base->random == 0.0f )
	{
		base->random = 400.0f;
	}

	if ( base->delay == 0 )
	{
		base->delay = 6000;
	}
	else if ( base->delay < 1000 )
	{
		base->delay = 1000;
	}

	// we only take damage from a heavy weapon class missile
	base->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;

	if ( base->spawnflags & 4 )//shielded
	{
		base->flags |= FL_SHIELDED; //technically, this would only take damage from a lightsaber, but the other flag just above would cancel that out too.
	}

	G_SpawnInt( "health", "2000", &base->health );
	base->e_DieFunc = dieF_ion_cannon_die;
	base->takedamage = qtrue;

	// Start Off?
	if ( base->spawnflags & 1 )
	{
		base->e_ThinkFunc = thinkF_NULL;
	}
	else
	{
		// start thinking now, otherwise, we'll wait until we are used
		base->e_ThinkFunc = thinkF_ion_cannon_think;
		base->nextthink = level.time + base->wait + Q_flrand(-1.0f, 1.0f) * base->random;
	}

	// Bursts?
	if ( base->spawnflags & 2 )
	{
		base->count = Q_irand(0,5); // 0-5 bursts
	}

	// precache
	base->fxID = G_EffectIndex( "env/ion_cannon" );

	// Set up our explosion effect for the ExplodeDeath code....
	G_EffectIndex( "env/ion_cannon_explosion" );

	base->contents = CONTENTS_BODY;

	VectorSet( base->mins, -141.0f, -148.0f, 0.0f );
	VectorSet( base->maxs, 142.0f, 135.0f, 245.0f );

	gi.linkentity( base );
}


//-----------------------------------------------------
void spotlight_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->e_ThinkFunc == thinkF_NULL )
	{
		// start thinking now, otherwise, we'll wait until we are used
		self->e_ThinkFunc = thinkF_spotlight_think;
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		self->e_ThinkFunc = thinkF_NULL;
		self->s.eFlags &= ~EF_ALT_FIRING;
	}
}

//-----------------------------------------------------
void spotlight_think( gentity_t *ent )
{
	vec3_t		dir, end;
	trace_t		tr;

	// dumb hack flag so that we can draw an interpolated light cone cgame side.
	ent->s.eFlags |= EF_ALT_FIRING;

	VectorSubtract( ent->enemy->currentOrigin, ent->currentOrigin, dir );
	VectorNormalize( dir );
	vectoangles( dir, ent->s.apos.trBase );
	ent->s.apos.trType = TR_INTERPOLATE;

	VectorMA( ent->currentOrigin, 2048, dir, end ); // just pick some max trace distance
	gi.trace( &tr, ent->currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, CONTENTS_SOLID, (EG2_Collision)0, 0 );

	ent->radius = tr.fraction * 2048.0f;

	if ( tr.fraction < 1 )
	{
		if ( DistanceSquared( tr.endpos, g_entities[0].currentOrigin ) < 140 * 140 )
		{
			// hit player--use target2
			G_UseTargets2( ent, &g_entities[0], ent->target2 );

#ifndef FINAL_BUILD
			if ( g_developer->integer == PRINT_DEVELOPER )
			{
				Com_Printf( S_COLOR_MAGENTA "Spotlight hit player at time: %d!!!\n", level.time );
			}
#endif
		}
	}

	ent->nextthink = level.time + 50;
}

//-----------------------------------------------------
void spotlight_link( gentity_t *ent )
{
	gentity_t *target = 0;

	target = G_Find( target, FOFS(targetname), ent->target );

	if ( !target )
	{
		Com_Printf( S_COLOR_RED "ERROR: spotlight_link: bogus target %s\n", ent->target );
		G_FreeEntity( ent );
		return;
	}

	ent->enemy = target;

	// Start Off?
	if ( ent->spawnflags & 1 )
	{
		ent->e_ThinkFunc = thinkF_NULL;
		ent->s.eFlags &= ~EF_ALT_FIRING;
	}
	else
	{
		// start thinking now, otherwise, we'll wait until we are used
		ent->e_ThinkFunc = thinkF_spotlight_think;
		ent->nextthink = level.time + FRAMETIME;
	}
}

/*QUAKED misc_spotlight (1 0 0) (-10 -10 0) (10 10 10) START_OFF
model="models/map_objects/imp_mine/spotlight.md3"
Search spotlight that must be targeted at a func_train or other entity
Uses its target2 when it detects the player

  START_OFF - Starts off

  targetname - Toggles it on/off
  target - What to point at
  target2 - What to use when detects player
*/

//-----------------------------------------------------
void SP_misc_spotlight( gentity_t *base )
//-----------------------------------------------------
{
	if ( !base->target )
	{
		Com_Printf( S_COLOR_RED "ERROR: misc_spotlight must have a target\n" );
		G_FreeEntity( base );
		return;
	}

	G_SetAngles( base, base->s.angles );
	G_SetOrigin( base, base->s.origin );

	base->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/spotlight.md3" );

	G_SpawnInt( "health", "300", &base->health );

	// Set up our lightcone effect, though we will have it draw cgame side so it looks better
	G_EffectIndex( "env/light_cone" );

	base->contents = CONTENTS_BODY;
	base->e_UseFunc = useF_spotlight_use;

	// the thing we need to target may not have spawned yet, so try back in a bit
	base->e_ThinkFunc = thinkF_spotlight_link;
	base->nextthink = level.time + 100;

	gi.linkentity( base );
}

/*QUAKED misc_panel_turret (0 0 1) (-8 -8 -12) (8 8 16) HEALTH
Creates a turret that, when the player uses a panel, takes control of this turret and adopts the turret view

  HEALTH - gun turret has health and displays a hud with its current health

"target" - thing to use when player enters the turret view
"target2" - thing to use when player leaves the turret view
"target3" - thing to use when it dies.

  radius - the max yaw range in degrees, (default 90) which means you can move 90 degrees on either side of the start angles.
  random - the max pitch range in degrees, (default 60) which means you can move 60 degrees above or below the start angles.
  delay - time between shots, in milliseconds (default 200).
  damage - amount of damage shots do, (default 50).
  speed - missile speed, (default 3000)

  heatlh - how much heatlh the thing has, (default 200) only works if HEALTH is checked, otherwise it can't be destroyed.
*/

extern gentity_t	*player;
extern qboolean		G_ClearViewEntity( gentity_t *ent );
extern void			G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
extern gentity_t	*CreateMissile( vec3_t org, vec3_t dir, float vel, int life, gentity_t *owner, qboolean altFire = qfalse );

void panel_turret_shoot( gentity_t *self, vec3_t org, vec3_t dir)
{
	gentity_t *missile = CreateMissile( org, dir, self->speed, 10000, self );

	missile->classname = "b_proj";
	missile->s.weapon = WP_TIE_FIGHTER;

	VectorSet( missile->maxs, 9, 9, 9 );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->bounceCount = 0;

	missile->damage = self->damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	G_SoundOnEnt( self, CHAN_AUTO, "sound/movers/objects/ladygun_fire" );

	VectorMA( org, 32, dir, org );
	org[2] -= 4;
	G_PlayEffect( "ships/imp_blastermuzzleflash", org, dir );
}

//-----------------------------------------
void misc_panel_turret_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	if ( self->target3 )
	{
		G_UseTargets2( self, player, self->target3 );
	}

	// FIXME: might need some other kind of logic or functionality in here??
	G_UseTargets2( self, player, self->target2 );
	G_ClearViewEntity( player );
	cg.overrides.active &= ~CG_OVERRIDE_FOV;
	cg.overrides.fov = 0;
}

//-----------------------------------------
void panel_turret_think( gentity_t *self )
{
	// Ensure that I am the viewEntity
	if ( player && player->client && player->client->ps.viewEntity == self->s.number )
	{
		usercmd_t *ucmd = &player->client->usercmd;

		// We are the viewEnt so update our new viewangles based on the sum of our start angles and the ucmd angles
		for ( int i = 0; i < 3; i++ )
		{
			// convert our base angle to a short, add with the usercmd.angle ( a short ), then switch use back to a real angle
			self->s.apos.trBase[i] = AngleNormalize180( SHORT2ANGLE( ucmd->angles[i] + ANGLE2SHORT( self->s.angles[i] ) + self->pos3[i] ));
		}

		// Only clamp if we have a PITCH clamp
		if ( self->random != 0.0f )
		// Angle clamping -- PITCH
		{
			if ( self->s.apos.trBase[PITCH] > self->random ) // random is PITCH
			{
				self->pos3[PITCH] += ANGLE2SHORT( AngleNormalize180( self->random - self->s.apos.trBase[PITCH]));
				self->s.apos.trBase[PITCH] = self->random;
			}
			else if ( self->s.apos.trBase[PITCH] < -self->random )
			{
				self->pos3[PITCH] -= ANGLE2SHORT( AngleNormalize180( self->random + self->s.apos.trBase[PITCH]));
				self->s.apos.trBase[PITCH] = -self->random;
			}
		}

		// Only clamp if we have a YAW clamp
		if ( self->radius != 0.0f )
		{
			float yawDif = AngleSubtract( self->s.apos.trBase[YAW], self->s.angles[YAW] );

			// Angle clamping -- YAW
			if (  yawDif > self->radius ) // radius is YAW
			{
				self->pos3[YAW] += ANGLE2SHORT( self->radius - yawDif );
				self->s.apos.trBase[YAW] = AngleNormalize180( self->s.angles[YAW] + self->radius );
			}
			else if ( yawDif < -self->radius ) // radius is YAW
			{
				self->pos3[YAW] -= ANGLE2SHORT( self->radius + yawDif );
				self->s.apos.trBase[YAW] = AngleNormalize180( self->s.angles[YAW] - self->radius );
			}
		}

		// Let cgame interpolation smooth out the angle changes
		self->s.apos.trType = TR_INTERPOLATE;
		self->s.pos.trType	= TR_INTERPOLATE; // not really moving, but this fixes an interpolation bug in cg_ents.

		// Check for backing out of turret
		if ( ( self->useDebounceTime < level.time ) && ((ucmd->buttons & BUTTON_USE) || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove) )
		{
			self->useDebounceTime = level.time + 200;

			G_UseTargets2( self, player, self->target2 );
			G_ClearViewEntity( player );
			G_Sound( player, self->soundPos2 );

			cg.overrides.active &= ~CG_OVERRIDE_FOV;
			cg.overrides.fov = 0;
			if ( ucmd->upmove > 0 )
			{//stop player from doing anything for a half second after
				player->aimDebounceTime = level.time + 500;
			}

			// can be drawn
//			self->s.eFlags &= ~EF_NODRAW;
		}
		else
		{
			// don't draw me when being looked through
//			self->s.eFlags |= EF_NODRAW;
//			self->s.modelindex = 0;

			// we only need to think when we are being used
			self->nextthink = level.time + 50;

			cg.overrides.active |= CG_OVERRIDE_FOV;
			cg.overrides.fov = 90;
		}

		if ( ucmd->buttons & BUTTON_ATTACK || ucmd->buttons & BUTTON_ALT_ATTACK )
		{
			if ( self->attackDebounceTime < level.time )
			{
				vec3_t dir, pt;

				AngleVectors( self->s.apos.trBase, dir, NULL, NULL );

				VectorCopy( self->currentOrigin, pt );
				pt[2] -= 4;
				panel_turret_shoot( self, pt, dir );

				self->attackDebounceTime = level.time + self->delay;
			}
		}
	}
}

//-----------------------------------------
void panel_turret_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// really only usable by the player
	if ( !activator || !activator->client || activator->s.number )
	{
		return;
	}

	if ( self->useDebounceTime > level.time )
	{
		// can't use it again right away.
		return;
	}

	if ( self->spawnflags & 1 ) // health...presumably the lady luck gun
	{
		G_Sound( self, G_SoundIndex( "sound/movers/objects/ladygun_on" ));
	}

	self->useDebounceTime = level.time + 200;

	// Compensating for the difference between the players view at the time of use and the start angles that the gun object has
	self->pos3[PITCH]	= -activator->client->usercmd.angles[PITCH];
	self->pos3[YAW]		= -activator->client->usercmd.angles[YAW];
	self->pos3[ROLL]	= 0;

	// set me as view entity
	G_UseTargets2( self, activator, self->target );
	G_SetViewEntity( activator, self );

	G_Sound( activator, self->soundPos1 );

	self->e_ThinkFunc = thinkF_panel_turret_think;
//	panel_turret_think( self );
	self->nextthink = level.time + 150;
}

//-----------------------------------------
void SP_misc_panel_turret( gentity_t *self )
{
	G_SpawnFloat( "radius", "90", &self->radius );	// yaw
	G_SpawnFloat( "random", "60", &self->random );	// pitch
	G_SpawnFloat( "speed" , "3000", &self->speed );
	G_SpawnInt( "delay", "200", &self->delay );
	G_SpawnInt( "damage", "50", &self->damage );

	VectorSet( self->pos3, 0.0f, 0.0f, 0.0f );

	if ( self->spawnflags & 1 ) // heatlh
	{
		self->takedamage = qtrue;
		self->contents = CONTENTS_SHOTCLIP;
		G_SpawnInt( "health", "200", &self->health );

		self->max_health = self->health;
		self->dflags |= DAMAGE_CUSTOM_HUD; // dumb, but we draw a custom hud
		G_SoundIndex( "sound/movers/objects/ladygun_on" );
	}

	self->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/ladyluck_gun.md3" );

	self->soundPos1 = G_SoundIndex( "sound/movers/camera_on.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/camera_off.mp3" );

	G_SoundIndex( "sound/movers/objects/ladygun_fire" );
	G_EffectIndex("ships/imp_blastermuzzleflash");

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );

	VectorSet( self->mins, -8, -8, -12 );
	VectorSet( self->maxs, 8, 8, 0 );
	self->contents = CONTENTS_SOLID;

	self->s.weapon = WP_TURRET;

	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));
	gi.linkentity( self );

	self->e_UseFunc = useF_panel_turret_use;
	self->e_DieFunc = dieF_misc_panel_turret_die;
}

#undef name
#undef name2
#undef name3
