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

#include "b_local.h"
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"

//-----------------------
//	Golan Arms Flechette
//-----------------------

//---------------------------------------------------------
static void WP_FlechetteMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs, start;
	gentity_t	*missile;
	float		damage = weaponData[WP_FLECHETTE].damage, vel = FLECHETTE_VEL;

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	// If we aren't the player, we will cut the velocity and damage of the shots
	if ( ent->s.number )
	{
		damage *= 0.75f;
		vel *= 0.5f;
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	for ( int i = 0; i < FLECHETTE_SHOTS; i++ )
	{
		vectoangles( wpFwd, angs );

		if ( i == 0 && ent->s.number == 0 )
		{
			// do nothing on the first shot for the player, this one will hit the crosshairs
		}
		else
		{
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		missile = CreateMissile( start, fwd, vel, 10000, ent );

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;

		VectorSet( missile->maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->maxs, -1, missile->mins );

		missile->damage = damage;

//		if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			missile->flags |= FL_OVERCHARGED;
//		}
			
		missile->dflags = (DAMAGE_DEATH_KNOCKBACK|DAMAGE_EXTRA_KNOCKBACK);
		
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(1,2);

		missile->s.eFlags |= EF_BOUNCE_SHRAPNEL;
		ent->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
void prox_mine_think( gentity_t *ent )
//---------------------------------------------------------
{
	int			count;
	qboolean	blow = qfalse;

	// if it isn't time to auto-explode, do a small proximity check
	if ( ent->delay > level.time )
	{
		count = G_RadiusList( ent->currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list );

		for ( int i = 0; i < count; i++ )
		{
			if ( ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->activator->s.number )
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if ( blow )
	{
//		G_Sound( ent, G_SoundIndex( "sound/weapons/flechette/warning.wav" ));
		ent->e_ThinkFunc = thinkF_WP_Explode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//---------------------------------------------------------
void prox_mine_stick( gentity_t *self, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	// turn us into a generic entity so we aren't running missile code
	self->s.eType = ET_GENERAL;

	self->s.modelindex = G_ModelIndex("models/weapons2/golan_arms/prox_mine.md3");
	self->e_TouchFunc = touchF_NULL;

	self->contents = CONTENTS_SOLID;
	self->takedamage = qtrue;
	self->health = 5;
	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet( self->maxs, 5, 5, 5 );
	VectorScale( self->maxs, -1, self->mins );

	self->activator = self->owner;
	self->owner = NULL;

	WP_Stick( self, trace );
	
	self->e_ThinkFunc = thinkF_prox_mine_think;
	self->nextthink = level.time + 450;

	// sticks for twenty seconds, then auto blows.
	self->delay = level.time + 20000;

	gi.linkentity( self );
}
/* Old Flechette alt-fire code....
//---------------------------------------------------------
static void WP_FlechetteProxMine( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, FLECHETTE_MINE_VEL, 10000, ent, qtrue );

	missile->fxID = G_EffectIndex( "flechette/explosion" );

	missile->classname = "proxMine";
	missile->s.weapon = WP_FLECHETTE;

	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_MISSILE_STICK;
	missile->e_TouchFunc = touchF_prox_mine_stick;

	missile->damage = FLECHETTE_MINE_DAMAGE;
	missile->methodOfDeath = MOD_EXPLOSIVE;

	missile->splashDamage = FLECHETTE_MINE_SPLASH_DAMAGE;
	missile->splashRadius = FLECHETTE_MINE_SPLASH_RADIUS;
	missile->splashMethodOfDeath = MOD_EXPLOSIVE_SPLASH;

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 0; 
}
*/
//----------------------------------------------
void WP_flechette_alt_blow( gentity_t *ent )
//----------------------------------------------
{
	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin ); // Not sure if this is even necessary, but correct origins are cool?

	G_RadiusDamage( ent->currentOrigin, ent->owner, ent->splashDamage, ent->splashRadius, NULL, MOD_EXPLOSIVE_SPLASH );
	G_PlayEffect( "flechette/alt_blow", ent->currentOrigin );

	G_FreeEntity( ent );
}

//------------------------------------------------------------------------------
static void WP_CreateFlechetteBouncyThing( vec3_t start, vec3_t fwd, gentity_t *self )
//------------------------------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( start, fwd, 950 + random() * 700, 1500 + random() * 2000, self, qtrue );
	
	missile->e_ThinkFunc = thinkF_WP_flechette_alt_blow;

	missile->s.weapon = WP_FLECHETTE;
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet( missile->mins, -3.0f, -3.0f, -3.0f );
	VectorSet( missile->maxs, 3.0f, 3.0f, 3.0f );
	missile->clipmask = MASK_SHOT;
	missile->clipmask &= ~CONTENTS_CORPSE;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_BOUNCE_HALF;

	missile->damage = weaponData[WP_FLECHETTE].altDamage;
	missile->dflags = 0;
	missile->splashDamage = weaponData[WP_FLECHETTE].splashDamage;
	missile->splashRadius = weaponData[WP_FLECHETTE].splashRadius;

	missile->svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT;

	VectorCopy( start, missile->pos2 );
}

//---------------------------------------------------------
static void WP_FlechetteAltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t 	dir, fwd, start, angs;

	vectoangles( wpFwd, angs );
	VectorCopy( wpMuzzle, start );

	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	for ( int i = 0; i < 2; i++ )
	{
		VectorCopy( angs, dir );

		dir[PITCH] -= random() * 4 + 8; // make it fly upwards
		dir[YAW] += crandom() * 2;
		AngleVectors( dir, fwd, NULL, NULL );

		WP_CreateFlechetteBouncyThing( start, fwd, self );
		self->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
void WP_FireFlechette( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_FlechetteAltFire( ent );
	}
	else
	{
		WP_FlechetteMainFire( ent );
	}
}