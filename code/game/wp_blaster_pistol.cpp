/*
This file is part of OpenJK.

    OpenJK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    OpenJK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenJK.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2013 OpenJK

#include "g_local.h"
#include "b_local.h"
#include "wp_saber.h"
#include "w_local.h"


//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
void WP_FireBryarPistol( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = !alt_fire ? weaponData[WP_BRYAR_PISTOL].damage : weaponData[WP_BRYAR_PISTOL].altDamage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( ent->NPC && ent->NPC->currentAim < 5 )
		{
			vec3_t	angs;

			vectoangles( forwardVec, angs );

			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				angs[PITCH] += ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
			}

			AngleVectors( angs, forwardVec, NULL, NULL );
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t	*missile = CreateMissile( start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	if ( ent->s.weapon == WP_BLASTER_PISTOL
		|| ent->s.weapon == WP_JAWA )
	{//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BRYAR_PISTOL;
	}

	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if ( ent->weaponModel[1] > 0 )
	{//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = (ent->count)?0:1;
	}
}