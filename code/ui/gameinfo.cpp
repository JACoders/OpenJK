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

//
// gameinfo.c
//

// *** This file is used by both the game and the user interface ***

#include "gameinfo.h"
#include "../game/weapons.h"

weaponData_t weaponData[WP_NUM_WEAPONS];
ammoData_t ammoData[AMMO_MAX];

extern void WP_LoadWeaponParms (void);

//
// Initialization - Read in files and parse into infos
//

/*
===============
GI_Init
===============
*/
void GI_Init( gameinfo_import_t *import ) {

	WP_LoadWeaponParms ();
}
