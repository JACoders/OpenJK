/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include <inttypes.h>

#include "bg_public.h"
#include "g_local.h"
#include "game/bg_public.h"

//
// Cvar callbacks
//

static void UpdateLegacyFixesConfigstring( legacyFixes_t legacyFix, qboolean enabled ) {
	char sLegacyFixes[32];
	trap->GetConfigstring(CS_LEGACY_FIXES, sLegacyFixes, sizeof(sLegacyFixes));

	uint32_t legacyFixes = strtoul(sLegacyFixes, NULL, 0);
	if (enabled) {
		legacyFixes |= (1 << legacyFix);
	} else {
		legacyFixes &= ~(1 << legacyFix);
	}
	trap->SetConfigstring(CS_LEGACY_FIXES, va("%" PRIu32, legacyFixes));
}

static void CVU_FixSaberMoveData(void) {
	BG_FixSaberMoveData();
	UpdateLegacyFixesConfigstring(LEGACYFIX_SABERMOVEDATA, g_fixSaberMoveData.integer);
}

static void CVU_FixRunWalkAnims(void) {
	UpdateLegacyFixesConfigstring(LEGACYFIX_RUNWALKANIMS, g_fixRunWalkAnims.integer);
}

static void CVU_FixWeaponAttackAnim(void) {
	BG_FixWeaponAttackAnim();
	UpdateLegacyFixesConfigstring(LEGACYFIX_WEAPONATTACKANIM, g_fixWeaponAttackAnim.integer);
}

//
// Cvar table
//

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	uint32_t	cvarFlags;
	qboolean	trackChange; // announce if value changes
} cvarTable_t;

#define XCVAR_DECL
	#include "g_xcvar.h"
#undef XCVAR_DECL

static const cvarTable_t gameCvarTable[] = {
	#define XCVAR_LIST
		#include "g_xcvar.h"
	#undef XCVAR_LIST
};
static const size_t gameCvarTableSize = ARRAY_LEN( gameCvarTable );

void G_RegisterCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void G_UpdateCvars( void ) {
	size_t i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();

				if ( cv->trackChange )
					trap->SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string ) );
			}
		}
	}
}
