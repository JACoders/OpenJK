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

//	Temp melee attack damage routine
//---------------------------------------------------------
void WP_Melee( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	int			damage = ent->s.number ? (g_spskill->integer*2)+1 : 3;
	float		range = ent->s.number ? 64 : 32;

	VectorMA( wpMuzzle, range, wpFwd, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, wpMuzzle, mins, maxs, end, ent->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( ent->client && !PM_DroidMelee( ent->client->NPC_class ) )
	{
		if ( ent->s.number || ent->alt_fire )
		{
			damage *= Q_irand( 2, 3 );
		}
		else
		{
			damage *= Q_irand( 1, 2 );
		}
	}

	if ( tr_ent && tr_ent->takedamage )
	{
		G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
}