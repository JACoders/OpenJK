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
#include "anims.h"
#include "wp_saber.h"
#include "../cgame/cg_local.h"
#include "b_local.h"
#include "g_navigator.h"

extern Vehicle_t *G_IsRidingVehicle( gentity_t *pEnt );

//lock the owner into place relative to the cannon pos
void EWebPositionUser(gentity_t *owner, gentity_t *eweb)
{
	mdxaBone_t boltMatrix;
	vec3_t p, p2, d;
	trace_t tr;
	qboolean traceOver = qtrue;

	if ( owner->s.number < MAX_CLIENTS )
	{//extra checks
		gi.trace(&tr, owner->currentOrigin, owner->mins, owner->maxs, owner->currentOrigin, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);
		if ( tr.startsolid || tr.allsolid )
		{//crap, they're already in solid somehow, don't bother tracing over
			traceOver = qfalse;
		}
	}
	if ( traceOver )
	{//trace up
		VectorCopy( owner->currentOrigin, p2 );
		p2[2] += STEPSIZE;
		gi.trace(&tr, owner->currentOrigin, owner->mins, owner->maxs, p2, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);
		if (!tr.startsolid && !tr.allsolid )
		{
			VectorCopy( tr.endpos, p2 );
		}
		else
		{
			VectorCopy( owner->currentOrigin, p2 );
		}
	}
	//trace over
	gi.G2API_GetBoltMatrix( eweb->ghoul2, 0, eweb->headBolt, &boltMatrix,
		eweb->s.apos.trBase, eweb->currentOrigin,
		(cg.time?cg.time:level.time), NULL, eweb->s.modelScale );
	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, p );
	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, d );
	d[2] = 0;
	VectorNormalize( d );
	VectorMA( p, -44.0f, d, p );
	if ( !traceOver )
	{
		VectorCopy( p, tr.endpos );
		tr.allsolid = tr.startsolid = qfalse;
	}
	else
	{
		p[2] = p2[2];
		if ( owner->s.number < MAX_CLIENTS )
		{//extra checks
			//just see if end point is not in solid
			gi.trace(&tr, p, owner->mins, owner->maxs, p, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);
			if ( tr.startsolid || tr.allsolid )
			{//would be in solid there, so just trace over, I guess?
				gi.trace(&tr, p2, owner->mins, owner->maxs, p, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);
			}
		}
		else
		{//trace over
			gi.trace(&tr, p2, owner->mins, owner->maxs, p, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);
		}
	}
	if (!tr.startsolid && !tr.allsolid )
	{
		//trace down
		VectorCopy( tr.endpos, p );
		VectorCopy( p, p2 );
		p2[2] -= STEPSIZE;
		gi.trace(&tr, p, owner->mins, owner->maxs, p2, owner->s.number, owner->clipmask, (EG2_Collision)0, 0);

		if (!tr.startsolid && !tr.allsolid )//&& tr.fraction == 1.0f)
		{ //all clear, we can move there
			vec3_t	moveDir;
			float moveDist;
			VectorCopy( tr.endpos, p );
			VectorSubtract( p, eweb->pos4, moveDir );
			moveDist = VectorNormalize( moveDir );
			if ( moveDist > 4.0f )
			{//moved past the threshold from last position
				vec3_t	oRight;
				int		strafeAnim;

				VectorCopy( p, eweb->pos4 );//update the position
				//find out what direction he moved in
				AngleVectors( owner->currentAngles, NULL, oRight, NULL );
				if ( DotProduct( moveDir, oRight ) > 0 )
				{//moved to his right, play right strafe
					strafeAnim = BOTH_STRAFE_RIGHT1;
				}
				else
				{//moved left, play left strafe
					strafeAnim = BOTH_STRAFE_LEFT1;
				}
				NPC_SetAnim( owner, SETANIM_LEGS, strafeAnim,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}

			G_SetOrigin(owner, p);
			VectorCopy(p, owner->client->ps.origin);
			gi.linkentity( owner );
		}
	}
	//FIXME: IK the hands to the handles of the gun?
}

//===============================================
//End E-Web
//===============================================

//----------------------------------------------------------

//===============================================
//Emplaced Gun
//===============================================

// spawnflag
#define	EMPLACED_INACTIVE	1
#define EMPLACED_FACING		2
#define EMPLACED_VULNERABLE	4
#define EWEB_INVULNERABLE	4
#define EMPLACED_PLAYERUSE	8

/*QUAKED emplaced_eweb (0 0 1) (-12 -12 -24) (12 12 24) INACTIVE FACING INVULNERABLE PLAYERUSE

 INACTIVE cannot be used until used by a target_activate
 FACING - player must be facing relatively in the same direction as the gun in order to use it
 VULNERABLE - allow the gun to take damage
 PLAYERUSE - only the player makes it run its usescript

 count - how much ammo to give this gun ( default 999 )
 health - how much damage the gun can take before it blows ( default 250 )
 delay - ONLY AFFECTS NPCs - time between shots ( default 200 on hardest setting )
 wait - ONLY AFFECTS NPCs - time between bursts ( default 800 on hardest setting )
 splashdamage - how much damage a blowing up gun deals ( default 80 )
 splashradius - radius for exploding damage ( default 128 )

 scripts:
	will run usescript, painscript and deathscript
*/
//----------------------------------------------------------
void eweb_pain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, const vec3_t point, int damage, int mod,int hitLoc )
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
void eweb_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	vec3_t org;

	// turn off any firing animations it may have been doing
	self->s.frame = self->startFrame = self->endFrame = 0;
	self->svFlags &= ~(SVF_ANIMATING|SVF_PLAYER_USABLE);


	self->health = 0;
//	self->s.weapon = WP_EMPLACED_GUN; // we need to be able to switch back to the old weapon

	self->takedamage = qfalse;
	self->lastEnemy = attacker;

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

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_RadiusDamage( self->currentOrigin, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

	VectorCopy( self->currentOrigin,  org );
	org[2] += 20;

	G_PlayEffect( "emplaced/explode", org );

	// Turn the top of the eweb off.
#define TURN_OFF			0x00000100//G2SURFACEFLAG_NODESCENDANTS
	gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "eweb_damage", TURN_OFF );

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

qboolean eweb_can_be_used( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->health <= 0 )
	{
		// can't use a dead gun.
		return qfalse;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{
		return qfalse; // can't use inactive gun
	}

	if ( !activator->client )
	{
		return qfalse; // only a client can use it.
	}

	if ( self->activator )
	{
		// someone is already in the gun.
		return qfalse;
	}

	if ( other && other->client && G_IsRidingVehicle( other ) )
	{//can't use eweb when on a vehicle
		return qfalse;
	}

	if ( activator && activator->client && G_IsRidingVehicle( activator ) )
	{//can't use eweb when on a vehicle
		return qfalse;
	}

	if ( activator && activator->client && (activator->client->ps.pm_flags&PMF_DUCKED) )
	{//stand up, ya cowardly varmint!
		return qfalse;
	}

	if ( activator && activator->health <= 0 )
	{//dead men ain't got no more use fer guns...
		return qfalse;
	}

	vec3_t fwd1, fwd2;
	vec3_t	facingAngles;

	VectorAdd( self->s.angles, self->pos1, facingAngles );
	if ( activator->s.number < MAX_CLIENTS )
	{//player must be facing general direction of the turret head
		// Let's get some direction vectors for the users
		AngleVectors( activator->client->ps.viewangles, fwd1, NULL, NULL );
		fwd1[2] = 0;

		// Get the gun's direction vector
		AngleVectors( facingAngles, fwd2, NULL, NULL );
		fwd2[2] = 0;

		float dot = DotProduct( fwd1, fwd2 );

		// Must be reasonably facing the way the gun points ( 90 degrees or so ), otherwise we don't allow to use it.
		if ( dot < 0.75f )
		{
			return qfalse;
		}
	}

	if ( self->delay + 500 < level.time )
	{
		return qtrue;
	}
	return qfalse;
}

void eweb_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !eweb_can_be_used( self, other, activator ) )
	{
		return;
	}

	int	oldWeapon = activator->s.weapon;

	if ( oldWeapon == WP_SABER )
	{
		self->alt_fire = activator->client->ps.SaberActive();
	}

	// swap the users weapon with the emplaced gun and add the ammo the gun has to the player
	activator->client->ps.weapon = self->s.weapon;
	Add_Ammo( activator, WP_EMPLACED_GUN, self->count );
	activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

	// Allow us to point from one to the other
	activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
	self->activator = activator;

	G_RemoveWeaponModels( activator );

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
	if ( activator->NPC )
	{
		ChangeWeapon( activator, WP_EMPLACED_GUN );
	}
	else if ( activator->s.number == 0 )
	{
		// we don't want for it to draw the weapon select stuff
		cg.weaponSelect = WP_EMPLACED_GUN;
		CG_CenterPrint( "@SP_INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
	}

	VectorCopy( activator->currentOrigin, self->pos4 );//keep this around so we know when to make them play the strafe anim

	// the gun will track which weapon we used to have
	self->s.weapon = oldWeapon;

	// Lock the player
	activator->client->ps.eFlags |= EF_LOCKED_TO_WEAPON;
	activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
	self->activator = activator;
	self->delay = level.time; // can't disconnect from the thing for half a second

	// Let the gun be considered an enemy
	//Ugh, so much AI code seems to assume enemies are clients, maybe this shouldn't be on, but it's too late in the game to change it now without knowing what side-effects this will have
	self->svFlags |= SVF_NONNPC_ENEMY;
	self->noDamageTeam = activator->client->playerTeam;

	//FIXME: should really wait a bit after spawn and get this just once?
	self->waypoint = NAV::GetNearestNode(self);
#ifdef _DEBUG
	if ( self->waypoint == -1 )
	{
		gi.Printf( S_COLOR_RED"ERROR: no waypoint for emplaced_gun %s at %s\n", self->targetname, vtos(self->currentOrigin) );
	}
#endif

	G_Sound( self, G_SoundIndex( "sound/weapons/eweb/eweb_mount.mp3" ));

	if ( !(self->spawnflags&EMPLACED_PLAYERUSE) || activator->s.number == 0 )
	{//player-only usescript or any usescript
		// Run use script
		G_ActivateBehavior( self, BSET_USE );
	}
}

//----------------------------------------------------------
void SP_emplaced_eweb( gentity_t *ent )
{
	char name[] = "models/map_objects/hoth/eweb_model.glm";

	ent->svFlags |= SVF_PLAYER_USABLE;
	ent->contents = CONTENTS_BODY;

	if ( ent->spawnflags & EMPLACED_INACTIVE )
	{
		ent->svFlags |= SVF_INACTIVE;
	}

	VectorSet( ent->mins, -12, -12, -24 );
	VectorSet( ent->maxs, 12, 12, 24 );

	ent->takedamage = qtrue;

	if ( ( ent->spawnflags & EWEB_INVULNERABLE ))
	{
		ent->flags |= FL_GODMODE;
	}

	ent->s.radius = 80;
	ent->spawnflags |= 4; // deadsolid

	//ent->e_ThinkFunc = thinkF_NULL;
	ent->e_PainFunc = painF_eweb_pain;
	ent->e_DieFunc  = dieF_eweb_die;

	G_EffectIndex( "emplaced/explode" );
	G_EffectIndex( "emplaced/dead_smoke" );

	G_SoundIndex( "sound/weapons/eweb/eweb_aim.wav" );
	G_SoundIndex( "sound/weapons/eweb/eweb_dismount.mp3" );
	//G_SoundIndex( "sound/weapons/eweb/eweb_empty.wav" );
	G_SoundIndex( "sound/weapons/eweb/eweb_fire.wav" );
	G_SoundIndex( "sound/weapons/eweb/eweb_hitplayer.wav" );
	G_SoundIndex( "sound/weapons/eweb/eweb_hitsurface.wav" );
	//G_SoundIndex( "sound/weapons/eweb/eweb_load.wav" );
	G_SoundIndex( "sound/weapons/eweb/eweb_mount.mp3" );

	// Set up our defaults and override with custom amounts as necessary
	G_SpawnInt( "count", "999", &ent->count );
	G_SpawnInt( "health", "250", &ent->health );
	G_SpawnInt( "splashDamage", "40", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "100", &ent->splashRadius );
	G_SpawnFloat( "delay", "200", &ent->random ); // NOTE: spawning into a different field!!
	G_SpawnFloat( "wait", "800", &ent->wait );

	ent->max_health = ent->health;
	ent->dflags |= DAMAGE_CUSTOM_HUD; // dumb, but we draw a custom hud

	ent->s.modelindex = G_ModelIndex( name );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, name, ent->s.modelindex, NULL_HANDLE, NULL_HANDLE, 0, 0 );

	// Activate our tags and bones
	ent->handLBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*cannonflash" ); //muzzle bolt
	ent->headBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "cannon_Xrot" ); //for placing the owner relative to rotation
	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
	ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cannon_Yrot", qtrue );
	ent->upperLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cannon_Xrot", qtrue );
	gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_X, NEGATIVE_Y, NULL, 0, 0);
	gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->upperLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_X, NEGATIVE_Y, NULL, 0, 0);
	//gi.G2API_SetBoneAngles( &ent->ghoul2[0], "cannon_Yrot", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL);
	//set the constraints for this guy as an emplaced weapon, and his constraint angles
	//ent->s.origin2[0] = 60.0f; //60 degrees in either direction

	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	VectorCopy( ent->s.angles, ent->lastAngles );

	// store base angles for later
	VectorClear( ent->pos1 );

	ent->e_UseFunc = useF_eweb_use;
	ent->bounceCount = 1;//to distinguish it from the emplaced gun

	gi.linkentity (ent);
}

/*QUAKED emplaced_gun (0 0 1) (-24 -24 0) (24 24 64) INACTIVE x VULNERABLE PLAYERUSE

 INACTIVE cannot be used until used by a target_activate
 VULNERABLE - allow the gun to take damage
 PLAYERUSE - only the player makes it run its usescript

 count - how much ammo to give this gun ( default 999 )
 health - how much damage the gun can take before it blows ( default 250 )
 delay - ONLY AFFECTS NPCs - time between shots ( default 200 on hardest setting )
 wait - ONLY AFFECTS NPCs - time between bursts ( default 800 on hardest setting )
 splashdamage - how much damage a blowing up gun deals ( default 80 )
 splashradius - radius for exploding damage ( default 128 )

 scripts:
	will run usescript, painscript and deathscript
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

	if ( other && other->client && G_IsRidingVehicle( other ) )
	{//can't use eweb when on a vehicle
		return;
	}

	if ( activator && activator->client && G_IsRidingVehicle( activator ) )
	{//can't use eweb when on a vehicle
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
			self->alt_fire = activator->client->ps.SaberActive();
		}

		// swap the users weapon with the emplaced gun and add the ammo the gun has to the player
		activator->client->ps.weapon = self->s.weapon;
		Add_Ammo( activator, WP_EMPLACED_GUN, self->count );
		activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

		// Allow us to point from one to the other
		activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
		self->activator = activator;

		G_RemoveWeaponModels( activator );

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
		if ( activator->NPC )
		{
			ChangeWeapon( activator, WP_EMPLACED_GUN );
		}
		else if ( activator->s.number == 0 )
		{
			// we don't want for it to draw the weapon select stuff
			cg.weaponSelect = WP_EMPLACED_GUN;
			CG_CenterPrint( "@SP_INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
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
		//Ugh, so much AI code seems to assume enemies are clients, maybe this shouldn't be on, but it's too late in the game to change it now without knowing what side-effects this will have
		self->svFlags |= SVF_NONNPC_ENEMY;
		self->noDamageTeam = activator->client->playerTeam;

		// FIXME: don't do this, we'll try and actually put the player in this beast
		// move the player to the center of the gun
//		activator->contents = 0;
//		VectorCopy( self->currentOrigin, activator->client->ps.origin );

		SetClientViewAngle( activator, self->pos1 );

		//FIXME: should really wait a bit after spawn and get this just once?
		self->waypoint = NAV::GetNearestNode(self);
#ifdef _DEBUG
		if ( self->waypoint == -1 )
		{
			gi.Printf( S_COLOR_RED"ERROR: no waypoint for emplaced_gun %s at %s\n", self->targetname, vtos(self->currentOrigin) );
		}
#endif

		G_Sound( self, G_SoundIndex( "sound/weapons/emplaced/emplaced_mount.mp3" ));

		if ( !(self->spawnflags&EMPLACED_PLAYERUSE) || activator->s.number == 0 )
		{//player-only usescript or any usescript
			// Run use script
			G_ActivateBehavior( self, BSET_USE );
		}
	}
}

//----------------------------------------------------------
void emplaced_gun_pain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, const vec3_t point, int damage, int mod,int hitLoc )
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
	ugly[PITCH] = self->lastAngles[PITCH] * 0.8f + Q_flrand(-1.0f, 1.0f) * 6;
	ugly[ROLL] = Q_flrand(-1.0f, 1.0f) * 7;
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

	//ent->e_ThinkFunc = thinkF_NULL;
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
	ent->headBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*seat" );
	ent->handLBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*flash01" );
	ent->handRBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*flash02" );
	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "base_bone", qtrue );
	ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "swivel_bone", qtrue );
	gi.G2API_SetBoneAnglesIndex( &ent->ghoul2[ent->playerModel], ent->lowerLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0);

	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	VectorCopy( ent->s.angles, ent->lastAngles );

	// store base angles for later
	VectorCopy( ent->s.angles, ent->pos1 );

	ent->e_UseFunc = useF_emplaced_gun_use;
	ent->bounceCount = 0;//to distinguish it from the eweb

	gi.linkentity (ent);
}

//====================================================
//General Emplaced Weapon Funcs called in g_active.cpp
//====================================================

void G_UpdateEmplacedWeaponData( gentity_t *ent )
{
	if ( ent && ent->owner && ent->health > 0 )
	{
		gentity_t *chair = ent->owner;
		if ( chair->e_UseFunc == useF_emplaced_gun_use )//yeah, crappy way to check this, but...
		{//one that you sit in
			//take the emplaced gun's waypoint as your own
			ent->waypoint = chair->waypoint;

			//update the actual origin of the sitter
			mdxaBone_t	boltMatrix;
			vec3_t	chairAng = {0, ent->client->ps.viewangles[YAW], 0};

			// Getting the seat bolt here
			gi.G2API_GetBoltMatrix( chair->ghoul2, chair->playerModel, chair->headBolt,
					&boltMatrix, chairAng, chair->currentOrigin, (cg.time?cg.time:level.time),
					NULL, chair->s.modelScale );
			// Storing ent position, bolt position, and bolt axis
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent->client->ps.origin );
			gi.linkentity( ent );
		}
		else if ( chair->e_UseFunc == useF_eweb_use )//yeah, crappy way to check this, but...
		{//standing at an E-Web
			EWebPositionUser( ent, chair );
		}
	}
}

void ExitEmplacedWeapon( gentity_t *ent )
{
	// requesting to unlock from the weapon
	// We'll leave the gun pointed in the direction it was last facing, though we'll cut out the pitch
	if ( ent->client )
	{
		// if we are the player we will have put down a brush that blocks NPCs so that we have a clear spot to get back out.
		//gentity_t *place = G_Find( NULL, FOFS(classname), "emp_placeholder" );

		if ( ent->health > 0 )
		{//he's still alive, and we have a placeholder, so put him back
			if ( ent->owner->nextTrain )
			{
				// reset the players position
				VectorCopy( ent->owner->nextTrain->currentOrigin, ent->client->ps.origin );
				//reset ent's size to normal
				VectorCopy( ent->owner->nextTrain->mins, ent->mins );
				VectorCopy( ent->owner->nextTrain->maxs, ent->maxs );
				//free the placeholder
				G_FreeEntity( ent->owner->nextTrain );
				//re-link the ent
				gi.linkentity( ent );
			}
			else if ( ent->owner->e_UseFunc == useF_eweb_use )//yeah, crappy way to check this, but...
			{
				// so give 'em a push away from us
				vec3_t backDir, start, end;
				trace_t trace;
				gentity_t *eweb = ent->owner;
				float curRadius = 0.0f;
				float minRadius, maxRadius;
				qboolean safeExit = qfalse;

				VectorSubtract( ent->currentOrigin, eweb->currentOrigin, backDir );
				backDir[2] = 0;
				minRadius = VectorNormalize( backDir )-8.0f;

				maxRadius = (ent->maxs[0]+ent->maxs[1])*0.5f;
				maxRadius += (eweb->maxs[0]+eweb->maxs[1])*0.5f;
				maxRadius *= 1.5f;

				if ( minRadius >= maxRadius - 1.0f )
				{
					maxRadius = minRadius + 8.0f;
				}

				ent->owner = NULL;//so his trace hits me

				for ( curRadius = minRadius; curRadius <= maxRadius; curRadius += 4.0f )
				{
					VectorMA( ent->currentOrigin, curRadius, backDir, start );
					//make sure they're not in the ground
					VectorCopy( start, end );
					start[2] += 18;
					end[2] -= 18;
					gi.trace(&trace, start, ent->mins, ent->maxs, end, ent->s.number, ent->clipmask, (EG2_Collision)0, 0);
					if ( !trace.allsolid && !trace.startsolid )
					{
						G_SetOrigin( ent, trace.endpos );
						gi.linkentity( ent );
						safeExit = qtrue;
						break;
					}
				}
				//Hmm... otherwise, don't allow them to get off?
				ent->owner = eweb;
				if ( !safeExit )
				{//don't try again for a second
					ent->owner->delay = level.time + 500;
					return;
				}
			}
		}
		else if ( ent->health <= 0 )
		{
			// dead, so give 'em a push out of the chair
			vec3_t dir;
			AngleVectors( ent->owner->s.angles, NULL, dir, NULL );

			if ( rand() & 1 )
			{
				VectorScale( dir, -1, dir );
			}

			VectorMA( ent->client->ps.velocity, 75, dir, ent->client->ps.velocity );
		}
		//don't let them move towards me for a couple frames so they don't step back into me while I'm becoming solid to them
		if ( ent->s.number < MAX_CLIENTS )
		{
			if ( ent->client->ps.pm_time < 100 )
			{
				ent->client->ps.pm_time = 100;
			}
			ent->client->ps.pm_flags |= (PMF_TIME_NOFRICTION|PMF_TIME_KNOCKBACK);
		}

		if ( !ent->owner->bounceCount )
		{//not an EWeb - the overridden bone angles will remember the angle we left it at
			VectorCopy( ent->client->ps.viewangles, ent->owner->s.angles );
			ent->owner->s.angles[PITCH] = 0;
			G_SetAngles( ent->owner, ent->owner->s.angles );
			VectorCopy( ent->owner->s.angles, ent->owner->pos1 );
		}
	}

	// Remove the emplaced gun from our inventory
	ent->client->ps.stats[STAT_WEAPONS] &= ~( 1 << WP_EMPLACED_GUN );

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void CG_ChangeWeapon( int num );
	if ( ent->health <= 0 )
	{//when die, don't set weapon back on when ejected from emplaced/eweb
		//empty hands
		ent->client->ps.weapon = WP_NONE;
		if ( ent->NPC )
		{
			ChangeWeapon( ent, ent->client->ps.weapon );	// should be OK actually.
		}
		else
		{
			CG_ChangeWeapon( ent->client->ps.weapon );
		}
		if ( ent->s.number < MAX_CLIENTS )
		{
			gi.cvar_set( "cg_thirdperson", "1" );
		}
	}
	else
	{
		// when we lock or unlock from the the gun, we get our old weapon back
		ent->client->ps.weapon = ent->owner->s.weapon;

		if ( ent->NPC )
		{//BTW, if a saber-using NPC ever gets off of an emplaced gun/eweb, this will not work, look at NPC_ChangeWeapon for the proper way
			ChangeWeapon( ent, ent->client->ps.weapon );
		}
		else
		{
			G_RemoveWeaponModels( ent );
			CG_ChangeWeapon( ent->client->ps.weapon );
			if ( ent->client->ps.weapon == WP_SABER )
			{
				WP_SaberAddG2SaberModels( ent );
			}
			else
			{
				G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
			}

			if ( ent->s.number < MAX_CLIENTS )
			{
				if ( ent->client->ps.weapon == WP_SABER )
				{
					gi.cvar_set( "cg_thirdperson", "1" );
				}
				else if ( ent->client->ps.weapon != WP_SABER && cg_gunAutoFirst.integer )
				{
					gi.cvar_set( "cg_thirdperson", "0" );
				}
			}
		}

		if ( ent->client->ps.weapon == WP_SABER )
		{
			if ( ent->owner->alt_fire )
			{
				ent->client->ps.SaberActivate();
			}
			else
			{
				ent->client->ps.SaberDeactivate();
			}
		}
	}
	//set the emplaced gun/eweb's weapon back to the emplaced gun
	ent->owner->s.weapon = WP_EMPLACED_GUN;
//	gi.G2API_DetachG2Model( &ent->ghoul2[ent->playerModel] );

	ent->s.eFlags &= ~EF_LOCKED_TO_WEAPON;
	ent->client->ps.eFlags &= ~EF_LOCKED_TO_WEAPON;

	ent->owner->noDamageTeam = TEAM_FREE;
	ent->owner->svFlags &= ~SVF_NONNPC_ENEMY;
	ent->owner->delay = level.time;
	ent->owner->activator = NULL;

	if ( !ent->NPC )
	{
		// by keeping the owner, a dead npc can be pushed out of the chair without colliding with it
		ent->owner = NULL;
	}
}

void RunEmplacedWeapon( gentity_t *ent, usercmd_t **ucmd )
{
	if (( (*ucmd)->buttons & BUTTON_USE || (*ucmd)->forwardmove < 0 || (*ucmd)->upmove > 0 ) && ent->owner && ent->owner->delay + 500 < level.time )
	{
		ent->owner->s.loopSound = 0;

		if ( ent->owner->e_UseFunc == useF_eweb_use )//yeah, crappy way to check this, but...
		{
			G_Sound( ent, G_SoundIndex( "sound/weapons/eweb/eweb_dismount.mp3" ));
		}
		else
		{
			G_Sound( ent, G_SoundIndex( "sound/weapons/emplaced/emplaced_dismount.mp3" ));

		}

		ExitEmplacedWeapon( ent );
		(*ucmd)->buttons &= ~BUTTON_USE;
		if ( (*ucmd)->upmove > 0 )
		{//don't actually jump
			(*ucmd)->upmove = 0;
		}
	}
	else
	{
		// this is a crappy way to put sounds on a moving eweb....
		if ( ent->owner
			&& ent->owner->e_UseFunc == useF_eweb_use )//yeah, crappy way to check this, but...
		{
			if ( !VectorCompare( ent->client->ps.viewangles, ent->owner->movedir ))
			{
				ent->owner->s.loopSound = G_SoundIndex( "sound/weapons/eweb/eweb_aim.wav" );
				ent->owner->fly_sound_debounce_time = level.time;
			}
			else
			{
				if ( ent->owner->fly_sound_debounce_time + 100 <= level.time )
				{
					ent->owner->s.loopSound = 0;
				}
			}

			VectorCopy( ent->client->ps.viewangles, ent->owner->movedir );
		}

		// don't allow movement, weapon switching, and most kinds of button presses
		(*ucmd)->forwardmove = 0;
		(*ucmd)->rightmove = 0;
		(*ucmd)->upmove = 0;
		(*ucmd)->buttons &= (BUTTON_ATTACK|BUTTON_ALT_ATTACK);

		(*ucmd)->weapon = ent->client->ps.weapon; //WP_EMPLACED_GUN;

		if ( ent->health <= 0 )
		{
			ExitEmplacedWeapon( ent );
		}
	}
}
