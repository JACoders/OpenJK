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

#include "g_headers.h"

#include "b_local.h"
#include "g_local.h"
#include "wp_saber.h"
#include "w_local.h"
#include "g_functions.h"

// Emplaced Gun
//---------------------------------------------------------
void WP_EmplacedFire( gentity_t *ent )
//---------------------------------------------------------
{
	float damage = weaponData[WP_EMPLACED_GUN].damage * ( ent->NPC ? 0.1f : 1.0f );
	float vel = EMPLACED_VEL * ( ent->NPC ? 0.4f : 1.0f );

	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent );

	missile->classname = "emplaced_proj";
	missile->s.weapon = WP_EMPLACED_GUN;

	missile->damage = damage; 
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EMPLACED;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// do some weird switchery on who the real owner is, we do this so the projectiles don't hit the gun object
	missile->owner = ent->owner;

	VectorSet( missile->maxs, EMPLACED_SIZE, EMPLACED_SIZE, EMPLACED_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	// alternate wpMuzzles
	ent->fxID = !ent->fxID;
}