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

//-----------------------
//	Det Pack
//-----------------------

//---------------------------------------------------------
void charge_stick( gentity_t *self, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	self->s.eType = ET_GENERAL;

	// make us so we can take damage
	self->clipmask = MASK_SHOT;
	self->contents = CONTENTS_SHOTCLIP;
	self->takedamage = qtrue;
	self->health = 25;

	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet( self->maxs, 10, 10, 10 );
	VectorScale( self->maxs, -1, self->mins );

	self->activator = self->owner;
	self->owner = NULL;

	self->e_TouchFunc = touchF_NULL;
	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;

	WP_Stick( self, trace, 1.0f );
}

//---------------------------------------------------------
static void WP_DropDetPack( gentity_t *self, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	// Chucking a new one
	AngleVectors( self->client->ps.viewangles, forwardVec, vrightVec, up );
	CalcMuzzlePoint( self, forwardVec, vrightVec, up, muzzle, 0 );
	VectorNormalize( forwardVec );
	VectorMA( muzzle, -4, forwardVec, muzzle );

	VectorCopy( muzzle, start );
	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t	*missile = CreateMissile( start, forwardVec, 300, 10000, self, qfalse );

	missile->fxID = G_EffectIndex( "detpack/explosion" ); // if we set an explosion effect, explode death can use that instead

	missile->classname = "detpack";
	missile->s.weapon = WP_DET_PACK;

	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_MISSILE_STICK;
	missile->e_TouchFunc = touchF_charge_stick;

	missile->damage = weaponData[WP_DET_PACK].damage;
	missile->methodOfDeath = MOD_DETPACK;

	missile->splashDamage = weaponData[WP_DET_PACK].splashDamage;
	missile->splashRadius = weaponData[WP_DET_PACK].splashRadius;
	missile->splashMethodOfDeath = MOD_DETPACK;// ?SPLASH;

	missile->clipmask = (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP);//MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

	missile->s.radius = 30;
	VectorSet( missile->s.modelScale, 1.0f, 1.0f, 1.0f );
	gi.G2API_InitGhoul2Model( missile->ghoul2, weaponData[WP_DET_PACK].missileMdl, G_ModelIndex( weaponData[WP_DET_PACK].missileMdl ),
		NULL_HANDLE, NULL_HANDLE, 0, 0);

	AddSoundEvent( NULL, missile->currentOrigin, 128, AEL_MINOR, qtrue );
	AddSightEvent( NULL, missile->currentOrigin, 128, AEL_SUSPICIOUS, 10 );
}

//---------------------------------------------------------
void WP_FireDetPack( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( !ent || !ent->client )
	{
		return;
	}

	if ( alt_fire  )
	{
		if ( ent->client->ps.eFlags & EF_PLANTED_CHARGE )
		{
			gentity_t *found = NULL;

			// loop through all ents and blow the crap out of them!
			while (( found = G_Find( found, FOFS( classname ), "detpack" )) != NULL )
			{
				if ( found->activator == ent )
				{
					VectorCopy( found->currentOrigin, found->s.origin );
					found->e_ThinkFunc = thinkF_WP_Explode;
					found->nextthink = level.time + 100 + Q_flrand(0.0f, 1.0f) * 100;
					G_Sound( found, G_SoundIndex( "sound/weapons/detpack/warning.wav" ));

					// would be nice if this actually worked?
					AddSoundEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DANGER, qfalse, qtrue );//FIXME: are we on ground or not?
					AddSightEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DISCOVERED, 100 );
				}
			}

			ent->client->ps.eFlags &= ~EF_PLANTED_CHARGE;
		}
	}
	else
	{
		WP_DropDetPack( ent, muzzle, forwardVec );

		ent->client->ps.eFlags |= EF_PLANTED_CHARGE;
	}
}