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
#include "w_local.h"

//	Bot Laser
//---------------------------------------------------------
void WP_BotLaser( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( muzzle, forwardVec, BRYAR_PISTOL_VEL, 10000, ent );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = BRYAR_PISTOL_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}