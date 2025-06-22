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

//g_items.h

#ifndef __ITEMS_H__
#define __ITEMS_H__

// Items enums
enum
{
ITM_NONE,

ITM_SABER_PICKUP,
ITM_BRYAR_PISTOL_PICKUP,
ITM_BLASTER_PICKUP,
ITM_DISRUPTOR_PICKUP,
ITM_BOWCASTER_PICKUP,
ITM_REPEATER_PICKUP,
ITM_DEMP2_PICKUP,
ITM_FLECHETTE_PICKUP,
ITM_ROCKET_LAUNCHER_PICKUP,
ITM_THERMAL_DET_PICKUP,
ITM_TRIP_MINE_PICKUP,
ITM_DET_PACK_PICKUP,
ITM_STUN_BATON_PICKUP,

ITM_BOT_LASER_PICKUP,
ITM_EMPLACED_GUN_PICKUP,
ITM_TURRET_PICKUP,
ITM_MELEE,
ITM_ATST_MAIN_PICKUP,
ITM_ATST_SIDE_PICKUP,
ITM_TIE_FIGHTER_PICKUP,
ITM_RAPID_FIRE_CONC_PICKUP,
ITM_BLASTER_PISTOL_PICKUP,

ITM_AMMO_FORCE_PICKUP,
ITM_AMMO_BLASTER_PICKUP,
ITM_AMMO_POWERCELL_PICKUP,
ITM_AMMO_METAL_BOLTS_PICKUP,
ITM_AMMO_ROCKETS_PICKUP,
ITM_AMMO_EMPLACED_PICKUP,
ITM_AMMO_THERMAL_PICKUP,
ITM_AMMO_TRIPMINE_PICKUP,
ITM_AMMO_DETPACK_PICKUP,

ITM_FORCE_HEAL_PICKUP,
ITM_FORCE_LEVITATION_PICKUP,
ITM_FORCE_SPEED_PICKUP,
ITM_FORCE_PUSH_PICKUP,
ITM_FORCE_PULL_PICKUP,
ITM_FORCE_TELEPATHY_PICKUP,
ITM_FORCE_GRIP_PICKUP,
ITM_FORCE_LIGHTNING_PICKUP,
ITM_FORCE_SABERTHROW_PICKUP,

ITM_BATTERY_PICKUP,
ITM_SEEKER_PICKUP,
ITM_SHIELD_PICKUP,
ITM_BACTA_PICKUP,
ITM_DATAPAD_PICKUP,
ITM_BINOCULARS_PICKUP,
ITM_SENTRY_GUN_PICKUP,
ITM_LA_GOGGLES_PICKUP,

ITM_MEDPAK_PICKUP,
ITM_SHIELD_SM_PICKUP,
ITM_SHIELD_LRG_PICKUP,
ITM_GOODIE_KEY_PICKUP,
ITM_SECURITY_KEY_PICKUP,

ITM_NUM_ITEMS
};

// Inventory item enums
enum //# item_e
{
	INV_ELECTROBINOCULARS,
	INV_BACTA_CANISTER,
	INV_SEEKER,
	INV_LIGHTAMP_GOGGLES,
	INV_SENTRY,
	//# #eol
	INV_GOODIE_KEY,	// don't want to include keys in the icarus list
	INV_SECURITY_KEY,

	INV_MAX						// Be sure to update MAX_INVENTORY
};

#endif
