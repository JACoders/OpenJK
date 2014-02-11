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
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//---------------------------------------------------------
void WP_FireStunBaton( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end, start;

	G_Sound( ent, G_SoundIndex( "sound/weapons/baton/fire" ));

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	VectorMA( start, STUN_BATON_RANGE, forwardVec, end );

	VectorSet( maxs, 5, 5, 5 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, start, mins, maxs, end, ent->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP, (EG2_Collision)0, 0 );

	if ( tr.entityNum >= ENTITYNUM_WORLD || tr.entityNum < 0 )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( tr_ent && tr_ent->takedamage && tr_ent->client )
	{
		G_PlayEffect( "stunBaton/flesh_impact", tr.endpos, tr.plane.normal );

		// TEMP!
//		G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		tr_ent->client->ps.powerups[PW_SHOCKED] = level.time + 1500;

		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, weaponData[WP_STUN_BATON].damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
	else if ( tr_ent->svFlags & SVF_GLASS_BRUSH || ( tr_ent->svFlags & SVF_BBRUSH && tr_ent->material == 12 )) // material grate...we are breaking a grate!
	{
		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, 999, DAMAGE_NO_KNOCKBACK, MOD_MELEE ); // smash that puppy
	}
}