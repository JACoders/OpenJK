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
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//-------------------
//	DEMP2
//-------------------

//---------------------------------------------------------
static void WP_DEMP2_MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_DEMP2].damage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t *missile = CreateMissile( start, forwardVec, DEMP2_VELOCITY, 10000, ent );

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = DEMP2_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = DEMP2_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = DEMP2_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

// NOTE: this is 100% for the demp2 alt-fire effect, so changes to the visual effect will affect game side demp2 code
//--------------------------------------------------
void DEMP2_AltRadiusDamage( gentity_t *ent )
{
	float		frac = ( level.time - ent->fx_time ) / 1300.0f; // synchronize with demp2 effect
	float		dist, radius;
	gentity_t	*gent;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities, i, e;
	vec3_t		mins, maxs;
	vec3_t		v, dir;

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end
	
	radius = frac * 200.0f; // 200 is max radius...the model is aprox. 100 units tall...the fx draw code mults. this by 2.

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = ent->currentOrigin[i] - radius;
		maxs[i] = ent->currentOrigin[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		gent = entityList[ e ];

		if ( !gent->takedamage || !gent->contents )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( ent->currentOrigin[i] < gent->absmin[i] ) 
			{
				v[i] = gent->absmin[i] - ent->currentOrigin[i];
			} 
			else if ( ent->currentOrigin[i] > gent->absmax[i] ) 
			{
				v[i] = ent->currentOrigin[i] - gent->absmax[i];
			} 
			else 
			{
				v[i] = 0;
			}
		}

		// shape is an ellipsoid, so cut vertical distance in half`
		v[2] *= 0.5f;

		dist = VectorLength( v );

		if ( dist >= radius ) 
		{
			// shockwave hasn't hit them yet
			continue;
		}

		if ( dist < ent->radius )
		{
			// shockwave has already hit this thing...
			continue;
		}

		VectorCopy( gent->currentOrigin, v );
		VectorSubtract( v, ent->currentOrigin, dir);

		// push the center of mass higher than the origin so players get knocked into the air more
		dir[2] += 12;

		G_Damage( gent, ent, ent->owner, dir, ent->currentOrigin, weaponData[WP_DEMP2].altDamage, DAMAGE_DEATH_KNOCKBACK, ent->splashMethodOfDeath );
		if ( gent->takedamage && gent->client ) 
		{
			gent->s.powerups |= ( 1 << PW_SHOCKED );
			gent->client->ps.powerups[PW_SHOCKED] = level.time + 2000;
			Saboteur_Decloak( gent, Q_irand( 3000, 10000 ) );
		}
	}

	// store the last fraction so that next time around we can test against those things that fall between that last point and where the current shockwave edge is
	ent->radius = radius;

	if ( frac < 1.0f )
	{
		// shock is still happening so continue letting it expand
		ent->nextthink = level.time + 50;
	}
}


//---------------------------------------------------------
void DEMP2_AltDetonate( gentity_t *ent )
//---------------------------------------------------------
{
	G_SetOrigin( ent, ent->currentOrigin );

	// start the effects, unfortunately, I wanted to do some custom things that I couldn't easily do with the fx system, so part of it uses an event and localEntities
	G_PlayEffect( "demp2/altDetonate", ent->currentOrigin, ent->pos1 );
	G_AddEvent( ent, EV_DEMP2_ALT_IMPACT, ent->count * 2 );

	ent->fx_time = level.time;
	ent->radius = 0;
	ent->nextthink = level.time + 50;
	ent->e_ThinkFunc = thinkF_DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

//---------------------------------------------------------
static void WP_DEMP2_AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int		damage	= weaponData[WP_REPEATER].altDamage;
	int		count;
	vec3_t	start;
	trace_t	tr;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	count = ( level.time - ent->client->ps.weaponChargeTime ) / DEMP2_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 3 )
	{
		count = 3;
	}

	damage *= ( 1 + ( count * ( count - 1 )));// yields damage of 12,36,84...gives a higher bonus for longer charge

	// the shot can travel a whopping 4096 units in 1 second. Note that the shot will auto-detonate at 4096 units...we'll see if this looks cool or not
	WP_MissileTargetHint(ent, start, forwardVec);
	gentity_t *missile = CreateMissile( start, forwardVec, DEMP2_ALT_RANGE, 1000, ent, qtrue );

	// letting it know what the charge size is.
	missile->count = count;

//	missile->speed = missile->nextthink;
	VectorCopy( tr.plane.normal, missile->pos1 );

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->e_ThinkFunc = thinkF_DEMP2_AltDetonate;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2_ALT;
	missile->splashRadius = weaponData[WP_DEMP2].altSplashRadius;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
void WP_FireDEMP2( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_DEMP2_AltFire( ent );
	}
	else
	{
		WP_DEMP2_MainFire( ent );
	}
}