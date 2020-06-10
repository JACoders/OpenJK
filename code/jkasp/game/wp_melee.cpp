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

void WP_Melee( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	int			damage = ent->s.number ? (g_spskill->integer*2)+1 : 3;
	float		range = ent->s.number ? 64 : 32;

	VectorMA( muzzle, range, forwardVec, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, muzzle, mins, maxs, end, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		if ( tr.entityNum == ENTITYNUM_WORLD )
		{
			G_PlayEffect( G_EffectIndex( "melee/punch_impact" ), tr.endpos, forwardVec );
		}
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
		int dflags = DAMAGE_NO_KNOCKBACK;
		G_PlayEffect( G_EffectIndex( "melee/punch_impact" ), tr.endpos, forwardVec );
		//G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		if ( ent->NPC && (ent->NPC->aiFlags&NPCAI_HEAVY_MELEE) )
		{ //4x damage for heavy melee class
			damage *= 4;
			dflags &= ~DAMAGE_NO_KNOCKBACK;
			dflags |= DAMAGE_DISMEMBER;
		}

		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, damage, dflags, MOD_MELEE );
	}
}