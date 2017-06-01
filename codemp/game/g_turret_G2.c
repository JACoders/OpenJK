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
#include "ghoul2/G2.h"
#include "qcommon/q_shared.h"

void G_SetEnemy( gentity_t *self, gentity_t *enemy );
void finish_spawning_turretG2( gentity_t *base );
void ObjectDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void turretG2_base_use( gentity_t *self, gentity_t *other, gentity_t *activator );


#define SPF_TURRETG2_CANRESPAWN	4
#define SPF_TURRETG2_TURBO		8
#define SPF_TURRETG2_LEAD_ENEMY	16
#define SPF_SHOWONRADAR			32

#define	ARM_ANGLE_RANGE		60
#define	HEAD_ANGLE_RANGE	90

#define name "models/map_objects/imp_mine/turret_canon.glm"
#define name2 "models/map_objects/imp_mine/turret_damage.md3"
#define name3 "models/map_objects/wedge/laser_cannon_model.glm"

//special routine for tracking angles between client and server -rww
void G2Tur_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles)
{
	int *thebone = &ent->s.boneIndex1;
	int *firstFree = NULL;
	int i = 0;
	int boneIndex = G_BoneIndex(bone);
	int flags, up, right, forward;
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

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	flags = BONE_ANGLES_POSTMULT;
	up = POSITIVE_Y;
	right = NEGATIVE_Z;
	forward = NEGATIVE_X;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = ((forward)|(right<<3)|(up<<6));

	trap->G2API_SetBoneAngles( ent->ghoul2,
					0,
					bone,
					angles,
					flags,
					up,
					right,
					forward,
					NULL,
					100,
					level.time );
}

void turretG2_set_models( gentity_t *self, qboolean dying )
{
	if ( dying )
	{
		if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->s.modelindex = G_ModelIndex( name2 );
			self->s.modelindex2 = G_ModelIndex( name );
		}

		trap->G2API_RemoveGhoul2Model( &self->ghoul2, 0 );
		G_KillG2Queue( self->s.number );
		self->s.modelGhoul2 = 0;
		/*
		trap->G2API_InitGhoul2Model( &self->ghoul2,
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
			trap->G2API_InitGhoul2Model( &self->ghoul2,
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
			trap->G2API_InitGhoul2Model( &self->ghoul2,
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

		self->s.modelGhoul2 = 1;
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{//larger
			self->s.g2radius = 128;
		}
		else
		{
			self->s.g2radius = 80;
		}

		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{//different pitch bone and muzzle flash points
			G2Tur_SetBoneAngles(self, "pitch", vec3_origin);
			self->genericValue11 = trap->G2API_AddBolt( self->ghoul2, 0, "*muzzle1" );
			self->genericValue12 = trap->G2API_AddBolt( self->ghoul2, 0, "*muzzle2" );
		}
		else
		{
			G2Tur_SetBoneAngles(self, "Bone_body", vec3_origin);
			self->genericValue11 = trap->G2API_AddBolt( self->ghoul2, 0, "*flash03" );
		}
	}
}

//------------------------------------------------------------------------------------------------------------
void TurretG2Pain( gentity_t *self, gentity_t *attacker, int damage )
//------------------------------------------------------------------------------------------------------------
{
	if (self->paintarget && self->paintarget[0])
	{
		if (self->genericValue8 < level.time)
		{
			G_UseTargets2(self, self, self->paintarget);
			self->genericValue8 = level.time + self->genericValue4;
		}
	}

	if ( attacker->client && attacker->client->ps.weapon == WP_DEMP2 )
	{
		self->attackDebounceTime = level.time + 2000 + Q_flrand(0.0f, 1.0f) * 500;
		self->painDebounceTime = self->attackDebounceTime;
	}
	if ( !self->enemy )
	{//react to being hit
		G_SetEnemy( self, attacker );
	}
	//self->s.health = self->health;
	//mmm..yes..bad.
}

//------------------------------------------------------------------------------------------------------------
void turretG2_die ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
//------------------------------------------------------------------------------------------------------------
{
	vec3_t	forward = { 0,0,-1 }, pos;

	// Turn off the thinking of the base & use it's targets
	//self->think = NULL;
	self->use = NULL;

	// clear my data
	self->die  = NULL;
	self->pain = NULL;
	self->takedamage = qfalse;
	self->s.health = self->health = 0;
	self->s.loopSound = 0;
	self->s.shouldtarget = qfalse;
	//self->s.owner = MAX_CLIENTS; //not owned by any client

	// hack the effect angle so that explode death can orient the effect properly
	if ( self->spawnflags & 2 )
	{
		VectorSet( forward, 0, 0, 1 );
	}

//	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );

	VectorMA( self->r.currentOrigin, 12, forward, pos );
	G_PlayEffect( EFFECT_EXPLOSION_TURRET, pos, forward );

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		G_RadiusDamage( self->r.currentOrigin,
						attacker,
						self->splashDamage,
						self->splashRadius,
						attacker,
						NULL,
						MOD_UNKNOWN );
	}

	if ( self->s.eFlags & EF_SHADER_ANIM )
	{
		self->s.frame = 1; // black
	}

	self->s.weapon = 0; // crosshair code uses this to mark crosshair red

	if ( self->s.modelindex2 )
	{
		// switch to damage model if we should
		turretG2_set_models( self, qtrue );

		VectorCopy( self->r.currentAngles, self->s.apos.trBase );
		VectorClear( self->s.apos.trDelta );

		if ( self->target )
		{
			G_UseTargets( self, attacker );
		}

		if (self->spawnflags & SPF_TURRETG2_CANRESPAWN)
		{//respawn
			if (self->health < 1 && !self->genericValue5)
			{ //we are dead, set our respawn delay if we have one
				self->genericValue5 = level.time + self->count;
			}
		}
	}
	else
	{
		ObjectDie( self, inflictor, attacker, damage, meansOfDeath );
	}
}

#define START_DIS 15

//start an animation on model_root both server side and client side
void TurboLaser_SetBoneAnim(gentity_t *eweb, int startFrame, int endFrame)
{
	//set info on the entity so it knows to start the anim on the client next snapshot.
	eweb->s.eFlags |= EF_G2ANIMATING;

	if (eweb->s.torsoAnim == startFrame && eweb->s.legsAnim == endFrame)
	{ //already playing this anim, let's flag it to restart
		eweb->s.torsoFlip = !eweb->s.torsoFlip;
	}
	else
	{
		eweb->s.torsoAnim = startFrame;
		eweb->s.legsAnim = endFrame;
	}

	//now set the animation on the server ghoul2 instance.
	assert(eweb->ghoul2);
	trap->G2API_SetBoneAnim(eweb->ghoul2, 0, "model_root", startFrame, endFrame,
		(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND), 1.0f, level.time, -1, 100);
}

extern void WP_FireTurboLaserMissile( gentity_t *ent, vec3_t start, vec3_t dir );
//----------------------------------------------------------------
static void turretG2_fire ( gentity_t *ent, vec3_t start, vec3_t dir )
//----------------------------------------------------------------
{
	vec3_t		org, ang;
	gentity_t	*bolt;

	if ( (trap->PointContents( start, ent->s.number )&MASK_SHOT) )
	{
		return;
	}

	VectorMA( start, -START_DIS, dir, org ); // dumb....

	if ( ent->random )
	{
		vectoangles( dir, ang );
		ang[PITCH] += flrand( -ent->random, ent->random );
		ang[YAW] += flrand( -ent->random, ent->random );
		AngleVectors( ang, dir, NULL, NULL );
	}

	vectoangles(dir, ang);

	if ( (ent->spawnflags&SPF_TURRETG2_TURBO) )
	{
		//muzzle flash
		G_PlayEffectID( ent->genericValue13, org, ang );
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
		G_PlayEffectID( G_EffectIndex("blaster/muzzle_flash"), org, ang );
		bolt = G_Spawn();

		bolt->classname = "turret_proj";
		bolt->nextthink = level.time + 10000;
		bolt->think = G_FreeEntity;
		bolt->s.eType = ET_MISSILE;
		bolt->s.weapon = WP_BLASTER;
		bolt->r.ownerNum = ent->s.number;
		bolt->damage = ent->damage;
		bolt->alliedTeam = ent->alliedTeam;
		bolt->teamnodmg = ent->teamnodmg;
		bolt->dflags = (DAMAGE_NO_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS);		// Don't push them around, or else we are constantly re-aiming
		bolt->splashDamage = ent->splashDamage;
		bolt->splashRadius = ent->splashDamage;
		bolt->methodOfDeath = MOD_TARGET_LASER;//MOD_ENERGY;
		bolt->splashMethodOfDeath = MOD_TARGET_LASER;//MOD_ENERGY;
		bolt->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		//bolt->trigger_formation = qfalse;		// don't draw tail on first frame

		VectorSet( bolt->r.maxs, 1.5, 1.5, 1.5 );
		VectorScale( bolt->r.maxs, -1, bolt->r.mins );
		bolt->s.pos.trType = TR_LINEAR;
		bolt->s.pos.trTime = level.time;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorScale( dir, ent->mass, bolt->s.pos.trDelta );
		SnapVector( bolt->s.pos.trDelta );		// save net bandwidth
		VectorCopy( start, bolt->r.currentOrigin);
	}
}

void turretG2_respawn( gentity_t *self )
{
	self->use = turretG2_base_use;
	self->pain = TurretG2Pain;
	self->die  = turretG2_die;
	self->takedamage = qtrue;
	self->s.shouldtarget = qtrue;
	//self->s.owner = MAX_CLIENTS; //not owned by any client
	if ( self->s.eFlags & EF_SHADER_ANIM )
	{
		self->s.frame = 0; // normal
	}
	self->s.weapon = WP_TURRET; // crosshair code uses this to mark crosshair red

	turretG2_set_models( self, qfalse );
	self->s.health = self->health = self->genericValue6;
	if (self->maxHealth) {
		G_ScaleNetHealth(self);
	}
	self->genericValue5 = 0;//clear this now
}

//-----------------------------------------------------
void turretG2_head_think( gentity_t *self )
//-----------------------------------------------------
{
	// if it's time to fire and we have an enemy, then gun 'em down!  pushDebounce time controls next fire time
	if ( self->enemy
		&& self->setTime < level.time
		&& self->attackDebounceTime < level.time )
	{
		vec3_t		fwd, org;
		mdxaBone_t	boltMatrix;

		// set up our next fire time
		self->setTime = level.time + self->wait;

		// Getting the flash bolt here
		trap->G2API_GetBoltMatrix( self->ghoul2,
					0,
					(self->alt_fire?self->genericValue12:self->genericValue11),
					&boltMatrix,
					self->r.currentAngles,
					self->r.currentOrigin,
					level.time,
					NULL,
					self->modelScale );
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->alt_fire = !self->alt_fire;
		}

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
		//BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Y, fwd );
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, fwd );
		}
		else
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, fwd );
		}

		VectorMA( org, START_DIS, fwd, org );

		turretG2_fire( self, org, fwd );
		self->fly_sound_debounce_time = level.time;//used as lastShotTime
	}
}

//-----------------------------------------------------
static void turretG2_aim( gentity_t *self )
//-----------------------------------------------------
{
	vec3_t	enemyDir, org, org2;
	vec3_t	desiredAngles, setAngle;
	float	diffYaw = 0.0f, diffPitch = 0.0f;
	float	maxYawSpeed = (self->spawnflags&SPF_TURRETG2_TURBO)?30.0f:14.0f;
	float	maxPitchSpeed = (self->spawnflags&SPF_TURRETG2_TURBO)?15.0f:3.0f;

	// move our gun base yaw to where we should be at this time....
	BG_EvaluateTrajectory( &self->s.apos, level.time, self->r.currentAngles );
	self->r.currentAngles[YAW] = AngleNormalize360( self->r.currentAngles[YAW] );
	self->speed = AngleNormalize360( self->speed );

	if ( self->enemy )
	{
		mdxaBone_t	boltMatrix;
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		if ( self->enemy->client )
		{
			VectorCopy( self->enemy->client->renderInfo.eyePoint, org );
		}
		else
		{
			VectorCopy( self->enemy->r.currentOrigin, org );
		}
		if ( self->spawnflags & 2 )
		{
			org[2] -= 15;
		}
		else
		{
			org[2] -= 5;
		}

		if ( (self->spawnflags&SPF_TURRETG2_LEAD_ENEMY) )
		{//we want to lead them a bit
			vec3_t diff, velocity;
			float dist;
			VectorSubtract( org, self->s.origin, diff );
			dist = VectorNormalize( diff );
			if ( self->enemy->client )
			{
				VectorCopy( self->enemy->client->ps.velocity, velocity );
			}
			else
			{
				VectorCopy( self->enemy->s.pos.trDelta, velocity );
			}
			VectorMA( org, (dist/self->mass), velocity, org );
		}

		// Getting the "eye" here
		trap->G2API_GetBoltMatrix( self->ghoul2,
					0,
					(self->alt_fire?self->genericValue12:self->genericValue11),
					&boltMatrix,
					self->r.currentAngles,
					self->s.origin,
					level.time,
					NULL,
					self->modelScale );

		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org2 );

		VectorSubtract( org, org2, enemyDir );
		vectoangles( enemyDir, desiredAngles );

		diffYaw = AngleSubtract( self->r.currentAngles[YAW], desiredAngles[YAW] );
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

		VectorCopy( self->r.currentAngles, self->s.apos.trBase );
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
			G2Tur_SetBoneAngles(self, "pitch", desiredAngles);
		}
		else
		{
			if ( self->spawnflags & 2 )
			{
				VectorSet( desiredAngles, self->speed, 0.0f, 0.0f );
			}
			else
			{
				VectorSet( desiredAngles, -self->speed, 0.0f, 0.0f );
			}
			G2Tur_SetBoneAngles(self, "Bone_body", desiredAngles);
		}
		/*
		trap->G2API_SetBoneAngles( self->ghoul2,
						0,
						"Bone_body",
						desiredAngles,
						BONE_ANGLES_POSTMULT,
						POSITIVE_Y,
						POSITIVE_Z,
						POSITIVE_X,
						NULL,
						100,
						level.time );
						*/
	}

	if ( diffYaw || diffPitch )
	{//FIXME: turbolaser sounds
		if ( (self->spawnflags&SPF_TURRETG2_TURBO) )
		{
			self->s.loopSound = G_SoundIndex( "sound/vehicles/weapons/turbolaser/turn.wav" );
		}
		else
		{
			self->s.loopSound = G_SoundIndex( "sound/chars/turret/move.wav" );
		}
	}
	else
	{
		self->s.loopSound = 0;
	}
}

//-----------------------------------------------------
static void turretG2_turnoff( gentity_t *self )
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
	if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
	{
		G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
	}

	// make turret play ping sound for 5 seconds
	self->aimDebounceTime = level.time + 5000;

	// Clear enemy
	self->enemy = NULL;
}

//-----------------------------------------------------
static qboolean turretG2_find_enemies( gentity_t *self )
//-----------------------------------------------------
{
	qboolean	found = qfalse;
	int			i, count;
	float		bestDist = self->radius * self->radius;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;
	qboolean	foundClient = qfalse;
	gentity_t	*entity_list[MAX_GENTITIES], *target, *bestTarget = NULL;

	if ( self->aimDebounceTime > level.time ) // time since we've been shut off
	{
		// We were active and alert, i.e. had an enemy in the last 3 secs
		if ( self->painDebounceTime < level.time )
		{
			if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
			{
				G_Sound(self, CHAN_BODY, G_SoundIndex( "sound/chars/turret/ping.wav" ));
			}
			self->painDebounceTime = level.time + 1000;
		}
	}

	VectorCopy( self->r.currentOrigin, org2 );
	if ( self->spawnflags & 2 )
	{
		org2[2] += 20;
	}
	else
	{
		org2[2] -= 20;
	}

	count = G_RadiusList( org2, self->radius, self, qtrue, entity_list );

	for ( i = 0; i < count; i++ )
	{
		trace_t	tr;
		target = entity_list[i];

		if ( !target->client )
		{
			// only attack clients
			if ( !(target->flags&FL_BBRUSH)//not a breakable brush
				|| !target->takedamage//is a bbrush, but invincible
				|| (target->NPC_targetname&&self->targetname&&Q_stricmp(target->NPC_targetname,self->targetname)!=0) )//not in invicible bbrush, but can only be broken by an NPC that is not me
			{
				continue;
			}
			//else: we will shoot at bbrushes!
		}
		if ( target == self || !target->takedamage || target->health <= 0 || ( target->flags & FL_NOTARGET ))
		{
			continue;
		}
		if ( target->client && target->client->sess.sessionTeam == TEAM_SPECTATOR )
		{
			continue;
		}
		if ( target->client && target->client->tempSpectate >= level.time )
		{
			continue;
		}
		if ( self->alliedTeam )
		{
			if ( target->client )
			{
				if ( target->client->sess.sessionTeam == self->alliedTeam )
				{
					// A bot/client/NPC we don't want to shoot
					continue;
				}
			}
			else if ( target->teamnodmg == self->alliedTeam )
			{
				// An ent we don't want to shoot
				continue;
			}
		}
		if ( !trap->InPVS( org2, target->r.currentOrigin ))
		{
			continue;
		}

		if ( target->client )
		{
			VectorCopy( target->client->renderInfo.eyePoint, org );
		}
		else
		{
			VectorCopy( target->r.currentOrigin, org );
		}

		if ( self->spawnflags & 2 )
		{
			org[2] -= 15;
		}
		else
		{
			org[2] += 5;
		}

		trap->Trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, qfalse, 0, 0 );

		if ( !tr.allsolid && !tr.startsolid && ( tr.fraction == 1.0 || tr.entityNum == target->s.number ))
		{
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < bestDist || (target->client && !foundClient))// all things equal, keep current
			{
				if ( self->attackDebounceTime < level.time )
				{
					// We haven't fired or acquired an enemy in the last 2 seconds-start-up sound
					if ( !(self->spawnflags&SPF_TURRETG2_TURBO) )
					{
						G_Sound( self, CHAN_BODY, G_SoundIndex( "sound/chars/turret/startup.wav" ));
					}

					// Wind up turrets for a bit
					self->attackDebounceTime = level.time + 1400;
				}

				bestTarget = target;
				bestDist = enemyDist;
				found = qtrue;
				if ( target->client )
				{//prefer clients over non-clients
					foundClient = qtrue;
				}
			}
		}
	}

	if ( found )
	{
		/*
		if ( !self->enemy )
		{//just aquired one
			AddSoundEvent( bestTarget, self->r.currentOrigin, 256, AEL_DISCOVERED );
			AddSightEvent( bestTarget, self->r.currentOrigin, 512, AEL_DISCOVERED, 20 );
		}
		*/
		G_SetEnemy( self, bestTarget );
		if ( VALIDSTRING( self->target2 ))
		{
			G_UseTargets2( self, self, self->target2 );
		}
	}

	return found;
}

//-----------------------------------------------------
void turretG2_base_think( gentity_t *self )
//-----------------------------------------------------
{
	qboolean	turnOff = qtrue;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;

	self->nextthink = level.time + FRAMETIME;

	if ( self->health <= 0 )
	{//dead
		if (self->spawnflags & SPF_TURRETG2_CANRESPAWN)
		{//can respawn
			if ( self->genericValue5 && self->genericValue5 < level.time )
			{ //we are dead, see if it's time to respawn
				turretG2_respawn( self );
			}
		}
		return;
	}
	else if ( self->spawnflags & 1 )
	{// not turned on
		turretG2_turnoff( self );
		turretG2_aim( self );

		// No target
		self->flags |= FL_NOTARGET;
		return;
	}
	else
	{
		// I'm all hot and bothered
		self->flags &= ~FL_NOTARGET;
	}

	if ( self->enemy )
	{
		if ( self->enemy->health < 0
			|| !self->enemy->inuse )
		{
			self->enemy = NULL;
		}
	}

	if ( self->last_move_time < level.time )
	{//MISNOMER: used a enemy recalcing debouncer
		if ( turretG2_find_enemies( self ) )
		{//found one
			turnOff = qfalse;
			if ( self->enemy && self->enemy->client )
			{//hold on to clients for a min of 3 seconds
				self->last_move_time = level.time + 3000;
			}
			else
			{//hold less
				self->last_move_time = level.time + 500;
			}
		}
	}

	if ( self->enemy != NULL )
	{
		if ( self->enemy->client && self->enemy->client->sess.sessionTeam == TEAM_SPECTATOR )
		{//don't keep going after spectators
			self->enemy = NULL;
		}
		else if ( self->enemy->client && self->enemy->client->tempSpectate >= level.time )
		{//don't keep going after spectators
			self->enemy = NULL;
		}
		else
		{//FIXME: remain single-minded or look for a new enemy every now and then?
			// enemy is alive
			VectorSubtract( self->enemy->r.currentOrigin, self->r.currentOrigin, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < self->radius * self->radius )
			{
				// was in valid radius
				if ( trap->InPVS( self->r.currentOrigin, self->enemy->r.currentOrigin ) )
				{
					// Every now and again, check to see if we can even trace to the enemy
					trace_t tr;

					if ( self->enemy->client )
					{
						VectorCopy( self->enemy->client->renderInfo.eyePoint, org );
					}
					else
					{
						VectorCopy( self->enemy->r.currentOrigin, org );
					}
					VectorCopy( self->r.currentOrigin, org2 );
					if ( self->spawnflags & 2 )
					{
						org2[2] += 10;
					}
					else
					{
						org2[2] -= 10;
					}
					trap->Trace( &tr, org2, NULL, NULL, org, self->s.number, MASK_SHOT, qfalse, 0, 0 );

					if ( !tr.allsolid && !tr.startsolid && tr.entityNum == self->enemy->s.number )
					{
						turnOff = qfalse;	// Can see our enemy
					}
				}
			}

		}
	}

	if ( turnOff )
	{
		if ( self->bounceCount < level.time ) // bounceCount is used to keep the thing from ping-ponging from on to off
		{
			turretG2_turnoff( self );
		}
	}
	else
	{
		// keep our enemy for a minimum of 2 seconds from now
		self->bounceCount = level.time + 2000 + Q_flrand(0.0f, 1.0f) * 150;
	}

	turretG2_aim( self );
	if ( !turnOff )
	{
		turretG2_head_think( self );
	}
}

//-----------------------------------------------------------------------------
void turretG2_base_use( gentity_t *self, gentity_t *other, gentity_t *activator )
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


/*QUAKED misc_turretG2 (1 0 0) (-8 -8 -22) (8 8 0) START_OFF UPSIDE_DOWN CANRESPAWN TURBO LEAD SHOWRADAR

Turret that hangs from the ceiling, will aim and shoot at enemies

  START_OFF - Starts off
  UPSIDE_DOWN - make it rest on a surface/floor instead of hanging from the ceiling
  CANRESPAWN - will respawn after being killed (use count)
  TURBO - Big-ass, Boxy Death Star Turbo Laser version
  LEAD - Turret will aim ahead of moving targets ("lead" them)
  SHOWRADAR - show on radar

  radius - How far away an enemy can be for it to pick it up (default 512)
  wait	- Time between shots (default 150 ms)
  dmg	- How much damage each shot does (default 5)
  health - How much damage it can take before exploding (default 100)
  count - if CANRESPAWN spawnflag, decides how long it is before gun respawns (in ms) - defaults to 20000 (20 seconds)

  paintarget - target to fire off upon being hurt
  painwait - ms to wait between firing off pain targets

  random - random error (in degrees) of projectile direction when it comes out of the muzzle (default is 2)

  shotspeed - the speed of the missile it fires travels at (default is 1100 for regular turrets, 20000 for TURBOLASERS)

  splashDamage - How much damage the explosion does
  splashRadius - The radius of the explosion

  targetname - Toggles it on/off
  target - What to use when destroyed
  target2 - What to use when it decides to start shooting at an enemy

  showhealth - set to 1 to show health bar on this entity when crosshair is over it

  teamowner - crosshair shows green for this team, red for opposite team
	0 - none
	1 - red
	2 - blue

  alliedTeam - team that this turret won't target
	0 - none
	1 - red
	2 - blue

  teamnodmg - team that turret does not take damage from
	0 - none
	1 - red
	2 - blue

  customscale - custom scaling size. 100 is normal size, 1024 is the max scaling. this will change the bounding box size, so be careful of starting in solid!

"icon" - icon that represents the objective on the radar
*/
//-----------------------------------------------------
void SP_misc_turretG2( gentity_t *base )
//-----------------------------------------------------
{
	int customscaleVal;
	char* s;

	turretG2_set_models( base, qfalse );

	G_SpawnInt("painwait", "0", &base->genericValue4);
	base->genericValue8 = 0;

	G_SpawnInt("customscale", "0", &customscaleVal);
	base->s.iModelScale = customscaleVal;
	if (base->s.iModelScale)
	{
		if (base->s.iModelScale > 1023)
		{
			base->s.iModelScale = 1023;
		}
		base->modelScale[0] = base->modelScale[1] = base->modelScale[2] = base->s.iModelScale/100.0f;
	}

	G_SpawnString( "icon", "", &s );
	if (s && s[0])
	{
		// We have an icon, so index it now.  We are reusing the genericenemyindex
		// variable rather than adding a new one to the entity state.
		base->s.genericenemyindex = G_IconIndex(s);
	}

	finish_spawning_turretG2( base );

	if (( base->spawnflags & 1 )) // Start_Off
	{
		base->s.frame = 1; // black
	}
	else
	{
		base->s.frame = 0; // glow
	}
	if ( !(base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		base->s.eFlags |= EF_SHADER_ANIM;
	}

	if (base->spawnflags & SPF_SHOWONRADAR)
	{
		base->s.eFlags |= EF_RADAROBJECT;
	}
#undef name
#undef name2
#undef name3
}

//-----------------------------------------------------
void finish_spawning_turretG2( gentity_t *base )
{
	vec3_t		fwd;
	int			t;

	if ( (base->spawnflags&2) )
	{
		base->s.angles[ROLL] += 180;
		base->s.origin[2] -= 22.0f;
	}

	G_SetAngles( base, base->s.angles );
	AngleVectors( base->r.currentAngles, fwd, NULL, NULL );

	G_SetOrigin(base, base->s.origin);

	base->s.eType = ET_GENERAL;

	if ( base->team && base->team[0] && //level.gametype == GT_SIEGE &&
		!base->teamnodmg)
	{
		base->teamnodmg = atoi(base->team);
	}
	base->team = NULL;

	// Set up our explosion effect for the ExplodeDeath code....
	G_EffectIndex( "turret/explode" );
	G_EffectIndex( "sparks/spark_exp_nosnd" );

	base->use = turretG2_base_use;
	base->pain = TurretG2Pain;

	// don't start working right away
	base->think = turretG2_base_think;
	base->nextthink = level.time + FRAMETIME * 5;

	// this is really the pitch angle.....
	base->speed = 0;

	// respawn time defaults to 20 seconds
	if ( (base->spawnflags&SPF_TURRETG2_CANRESPAWN) && !base->count )
	{
		base->count = 20000;
	}

	G_SpawnFloat( "shotspeed", "0", &base->mass );
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 20000;
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
			base->wait = 1000;// + Q_flrand(0.0f, 1.0f) * 500;
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
			base->damage = 500;
		}

		if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
		{
			VectorSet( base->r.maxs, 64.0f, 64.0f, 30.0f );
			VectorSet( base->r.mins, -64.0f, -64.0f, -30.0f );
		}
		//start in "off" anim
		TurboLaser_SetBoneAnim( base, 4, 5 );
		if ( level.gametype == GT_SIEGE )
		{//FIXME: designer-specified?
			//FIXME: put on other entities, too, particularly siege objectives and bbrushes...
			base->s.eFlags2 |= EF2_BRACKET_ENTITY;
		}
	}
	else
	{
		if ( !base->random )
		{//error worked into projectile direction
			base->random = 2.0f;
		}

		if ( !base->mass )
		{//misnomer: speed of projectile
			base->mass = 1100;
		}

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
		{//upside-down, invert r.mins and maxe
			VectorSet( base->r.maxs, 10.0f, 10.0f, 30.0f );
			VectorSet( base->r.mins, -10.0f, -10.0f, 0.0f );
		}
		else
		{
			VectorSet( base->r.maxs, 10.0f, 10.0f, 0.0f );
			VectorSet( base->r.mins, -10.0f, -10.0f, -30.0f );
		}
	}

	//stash health off for respawn.  NOTE: cannot use maxhealth because that might not be set if not showing the health bar
	base->genericValue6 = base->health;

	G_SpawnInt( "showhealth", "0", &t );
	if (t)
	{ //a non-0 maxhealth value will mean we want to show the health on the hud
		base->maxHealth = base->health;
		G_ScaleNetHealth(base);
		base->s.shouldtarget = qtrue;
		//base->s.owner = MAX_CLIENTS; //not owned by any client
	}

	if (base->s.iModelScale)
	{ //let's scale the bbox too...
		float fScale = base->s.iModelScale/100.0f;
		VectorScale(base->r.mins, fScale, base->r.mins);
		VectorScale(base->r.maxs, fScale, base->r.maxs);
	}

	// Precache special FX and moving sounds
	if ( (base->spawnflags&SPF_TURRETG2_TURBO) )
	{
		base->genericValue13 = G_EffectIndex( "turret/turb_muzzle_flash" );
		base->genericValue14 = G_EffectIndex( "turret/turb_shot" );
		base->genericValue15 = G_EffectIndex( "turret/turb_impact" );
		//FIXME: Turbo Laser Cannon sounds!
		G_SoundIndex( "sound/vehicles/weapons/turbolaser/turn.wav" );
	}
	else
	{
		G_SoundIndex( "sound/chars/turret/startup.wav" );
		G_SoundIndex( "sound/chars/turret/shutdown.wav" );
		G_SoundIndex( "sound/chars/turret/ping.wav" );
		G_SoundIndex( "sound/chars/turret/move.wav" );
	}

	base->r.contents = CONTENTS_BODY|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_SHOTCLIP;

	//base->max_health = base->health;
	base->takedamage = qtrue;
	base->die  = turretG2_die;

	base->material = MAT_METAL;
	//base->r.svFlags |= SVF_NO_TELEPORT|SVF_NONNPC_ENEMY|SVF_SELF_ANIMATING;

	// Register this so that we can use it for the missile effect
	RegisterItem( BG_FindItemForWeapon( WP_BLASTER ));

	// But set us as a turret so that we can be identified as a turret
	base->s.weapon = WP_TURRET;

	trap->LinkEntity( (sharedEntity_t *)base );
}
