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

#include "g_local.h"
#include "g_functions.h"

extern int	G_FindConfigstringIndex( const char *name, int start, int max, qboolean create );

#define FX_ENT_RADIUS 32

extern int	BMS_START;
extern int	BMS_MID;
extern int	BMS_END;

//----------------------------------------------------------

/*QUAKED fx_runner (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF ONESHOT DAMAGE
Runs the specified effect, can also be targeted at an info_notnull to orient the effect

	STARTOFF - effect starts off, toggles on/off when used
	ONESHOT - effect fires only when used
	DAMAGE - does radius damage around effect every "delay" milliseonds

	"fxFile" - name of the effect file to play
	"target" - direction to aim the effect in, otherwise defaults to up
	"target2" - uses its target2 when the fx gets triggered
	"delay"  - how often to call the effect, don't over-do this ( default 200 )
	"random" - random amount of time to add to delay, ( default 0, 200 = 0ms to 200ms )
	"splashRadius" - only works when damage is checked ( default 16 )
	"splashDamage" - only works when damage is checked ( default 5 )
	"soundset"	- bmodel set to use, plays start sound when toggled on, loop sound while on ( doesn't play on a oneshot), and a stop sound when turned off
*/
#define FX_RUNNER_RESERVED 0x800000

//----------------------------------------------------------
void fx_runner_think( gentity_t *ent )
{
	vec3_t temp;

	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin );
	EvaluateTrajectory( &ent->s.apos, level.time, ent->currentAngles );

	// call the effect with the desired position and orientation
	G_AddEvent( ent, EV_PLAY_EFFECT, ent->fxID );

	// Assume angles, we'll do a cross product on the other end to finish up
	AngleVectors( ent->currentAngles, ent->pos3, NULL, NULL );
	MakeNormalVectors( ent->pos3, ent->pos4, temp ); // there IS a reason this is done...it's so that it doesn't break every effect in the game...

	ent->nextthink = level.time + ent->delay + random() * ent->random;

	if ( ent->spawnflags & 4 ) // damage
	{
		G_RadiusDamage( ent->currentOrigin, ent, ent->splashDamage, ent->splashRadius, ent, MOD_UNKNOWN );
	}

	if ( ent->target2 )
	{
		// let our target know that we have spawned an effect
		G_UseTargets2( ent, ent, ent->target2 );
	}

	if ( !(ent->spawnflags & 2 ) && !ent->s.loopSound ) // NOT ONESHOT...this is an assy thing to do
	{
		if ( VALIDSTRING( ent->soundSet ) == true )
		{
			ent->s.loopSound = CAS_GetBModelSound( ent->soundSet, BMS_MID );

			if ( ent->s.loopSound < 0 )
			{
				ent->s.loopSound = 0;
			}
		}
	}

}

//----------------------------------------------------------
void fx_runner_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->spawnflags & 2 ) // ONESHOT
	{
		// call the effect with the desired position and orientation, as a safety thing,
		//	make sure we aren't thinking at all.
		fx_runner_think( self );
		self->nextthink = -1;

		if ( self->target2 )
		{
			// let our target know that we have spawned an effect
			G_UseTargets2( self, self, self->target2 );
		}

		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
		}
	}
	else
	{
		// ensure we are working with the right think function
		self->e_ThinkFunc = thinkF_fx_runner_think;

		// toggle our state
		if ( self->nextthink == -1 )
		{
			// NOTE: we fire the effect immediately on use, the fx_runner_think func will set
			//	up the nextthink time.
			fx_runner_think( self );

			if ( VALIDSTRING( self->soundSet ) == true )
			{
				G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
				self->s.loopSound = CAS_GetBModelSound( self->soundSet, BMS_MID );

				if ( self->s.loopSound < 0 )
				{
					self->s.loopSound = 0;
				}
			}
		}
		else
		{
			// turn off for now
			self->nextthink = -1;

			if ( VALIDSTRING( self->soundSet ) == true )
			{
				G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_END ));
				self->s.loopSound = 0;
			}
		}
	}
}

//----------------------------------------------------------
void fx_runner_link( gentity_t *ent )
{
	vec3_t	dir;

	if ( ent->target )
	{
		// try to use the target to override the orientation
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target );

		if ( !target )
		{
			// Bah, no good, dump a warning, but continue on and use the UP vector
			Com_Printf( "fx_runner_link: target specified but not found: %s\n", ent->target );
			Com_Printf( "  -assuming UP orientation.\n" );
		}
		else
		{
			// Our target is valid so let's override the default UP vector
			VectorSubtract( target->s.origin, ent->s.origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ent->s.angles );
		}
	}

	// don't really do anything with this right now other than do a check to warn the designers if the target2 is bogus
	if ( ent->target2 )
	{
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target2 );

		if ( !target )
		{
			// Target2 is bogus, but we can still continue
			Com_Printf( "fx_runner_link: target2 was specified but is not valid: %s\n", ent->target2 );
		}
	}

	G_SetAngles( ent, ent->s.angles );

	if ( ent->spawnflags & 1 || ent->spawnflags & 2 ) // STARTOFF || ONESHOT
	{
		// We won't even consider thinking until we are used
		ent->nextthink = -1;
	}
	else
	{
		if ( VALIDSTRING( ent->soundSet ) == true )
		{
			ent->s.loopSound = CAS_GetBModelSound( ent->soundSet, BMS_MID );

			if ( ent->s.loopSound < 0 )
			{
				ent->s.loopSound = 0;
			}
		}

		// Let's get to work right now!
		ent->e_ThinkFunc = thinkF_fx_runner_think;
		ent->nextthink = level.time + 200; // wait a small bit, then start working
	}

	// make us useable if we can be targeted
	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_fx_runner_use;
	}
}

//----------------------------------------------------------
void SP_fx_runner( gentity_t *ent )
{
	// Get our defaults
	G_SpawnInt( "delay", "200", &ent->delay );
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnInt( "splashRadius", "16", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "5", &ent->splashDamage );

	if ( !G_SpawnAngleHack( "angle", "0", ent->s.angles ))
	{
		// didn't have angles, so give us the default of up
		VectorSet( ent->s.angles, -90, 0, 0 );
	}

	if ( !ent->fxFile )
	{
		gi.Printf( S_COLOR_RED"ERROR: fx_runner %s at %s has no fxFile specified\n", ent->targetname, vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}

	// Try and associate an effect file, unfortunately we won't know if this worked or not
	//	until the CGAME trys to register it...
	ent->fxID = G_EffectIndex( ent->fxFile );

	ent->s.eType = ET_MOVER;

	// Give us a bit of time to spawn in the other entities, since we may have to target one of 'em
	ent->e_ThinkFunc = thinkF_fx_runner_link;
	ent->nextthink = level.time + 400;

	// Save our position and link us up!
	G_SetOrigin( ent, ent->s.origin );

	VectorSet( ent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( ent->maxs, -1, ent->mins );

	gi.linkentity( ent );
}


/*QUAKED fx_snow (1 0 0) (-16 -16 -16) (16 16 16)
This world effect will spawn snow globally into the level.

"count" the number of snow particles (default of 1000)
*/
//----------------------------------------------------------
void SP_CreateSnow( gentity_t *ent )
{
	char	temp[256];

	G_SpawnInt( "count", "1000", &ent->count );

	cvar_t *r_weatherScale = gi.cvar( "r_weatherScale", "1", CVAR_ARCHIVE );

	if ( r_weatherScale->value > 0.0f )
	{
		sprintf( temp, "snow init %i", (int)( ent->count * r_weatherScale->value ));

		G_FindConfigstringIndex( temp, CS_WORLD_FX, MAX_WORLD_FX, qtrue );

		level.worldFlags |= WF_SNOWING;
	}
}

/*QUAKED fx_rain (1 0 0) (-16 -16 -16) (16 16 16)
This world effect will spawn rain globally into the level.

"count" the number of rain particles (default of 500)
*/
//----------------------------------------------------------
void SP_CreateRain( gentity_t *ent )
{
	char	temp[256];

	G_SpawnInt( "count", "500", &ent->count );

	cvar_t *r_weatherScale = gi.cvar( "r_weatherScale", "1", CVAR_ARCHIVE );

	if ( r_weatherScale->value > 0.0f )
	{
		sprintf( temp, "rain init %i", (int)( ent->count * r_weatherScale->value ));

		G_FindConfigstringIndex( temp, CS_WORLD_FX, MAX_WORLD_FX, qtrue );

		level.worldFlags |= WF_RAINING;
	}
}


//-----------------
// Explosion Trail
//-----------------

//----------------------------------------------------------
void fx_explosion_trail_think( gentity_t *ent )
{
	vec3_t	origin;
	trace_t	tr;

	if ( ent->spawnflags & 1 ) // gravity
	{
		ent->s.pos.trType = TR_GRAVITY;
	}
	else
	{
		ent->s.pos.trType = TR_LINEAR;
	}

	EvaluateTrajectory( &ent->s.pos, level.time, origin );

	gi.trace( &tr, ent->currentOrigin, vec3_origin, vec3_origin, origin,
				ent->owner ? ent->owner->s.number : ENTITYNUM_NONE, ent->clipmask, G2_RETURNONHIT, 10 );

	if ( tr.fraction < 1.0f )
	{
		// never explode or bounce on sky
		if ( !( tr.surfaceFlags & SURF_NOIMPACT ))
		{
			if ( ent->splashDamage && ent->splashRadius )
			{
				G_RadiusDamage( tr.endpos, ent, ent->splashDamage, ent->splashRadius, ent, MOD_EXPLOSIVE_SPLASH );
			}
		}

		if ( ent->fullName )
		{
			// fxFile2....in other words, impact fx
			G_PlayEffect( ent->fullName, tr.endpos, tr.plane.normal );
		}

		if ( VALIDSTRING( ent->soundSet ) == true )
		{
			G_AddEvent( ent, EV_BMODEL_SOUND, CAS_GetBModelSound( ent->soundSet, BMS_END ));
		}

		G_FreeEntity( ent );
		return;
	}

	G_RadiusDamage( origin, ent, ent->damage, ent->radius, ent, MOD_EXPLOSIVE_SPLASH );

	// call the effect with the desired position and orientation
	G_PlayEffect( ent->fxID, origin, ent->currentAngles );

	ent->nextthink = level.time + 50;
	gi.linkentity( ent );
}

//----------------------------------------------------------
void fx_explosion_trail_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	gentity_t *missile = G_Spawn();

	// We aren't a missile in the truest sense, rather we just move through the world and spawn effects
	if ( missile )
	{
		missile->classname = "fx_exp_trail";

		missile->nextthink = level.time + 50;
		missile->e_ThinkFunc = thinkF_fx_explosion_trail_think;

		missile->s.eType = ET_MOVER;

		missile->owner = self;

		missile->s.modelindex = self->s.modelindex2;

		missile->s.pos.trTime = level.time;
		G_SetOrigin( missile, self->currentOrigin );
		if ( self->spawnflags & 1 ) // gravity
		{
			missile->s.pos.trType = TR_GRAVITY;
		}
		else
		{
			missile->s.pos.trType = TR_LINEAR;
		}

		missile->spawnflags = self->spawnflags;

		G_SetAngles( missile, self->currentAngles );
		VectorScale( self->currentAngles, self->speed, missile->s.pos.trDelta );
		missile->s.pos.trTime = level.time;
		missile->radius = self->radius;
		missile->damage = self->damage;
		missile->splashDamage = self->splashDamage;
		missile->splashRadius = self->splashRadius;
		missile->fxID = self->fxID;
		missile->fullName = self->fullName;

		missile->clipmask = MASK_SHOT;

		gi.linkentity( missile );

		if ( VALIDSTRING( self->soundSet ) == true )
		{
			G_AddEvent( self, EV_BMODEL_SOUND, CAS_GetBModelSound( self->soundSet, BMS_START ));
			missile->s.loopSound = CAS_GetBModelSound( self->soundSet, BMS_MID );
			missile->soundSet = self->soundSet;

			if ( missile->s.loopSound < 0 )
			{
				missile->s.loopSound = 0;
			}
		}
	}
}

//----------------------------------------------------------
void fx_explosion_trail_link( gentity_t *ent )
{
	vec3_t		dir;
	gentity_t	*target = NULL;

	// we ony activate when used
	ent->e_UseFunc = useF_fx_explosion_trail_use;

	if ( ent->target )
	{
		// try to use the target to override the orientation
		target = G_Find( target, FOFS(targetname), ent->target );

		if ( !target )
		{
			gi.Printf( S_COLOR_RED"ERROR: fx_explosion_trail %s could not find target %s\n", ent->targetname, ent->target );
			G_FreeEntity( ent );
			return;
		}

		// Our target is valid so lets use that
		VectorSubtract( target->s.origin, ent->s.origin, dir );
		VectorNormalize( dir );
	}
	else
	{
		// we are assuming that we have angles, but there are no checks to verify this
		AngleVectors( ent->s.angles, dir, NULL, NULL );
	}

	// NOTE: this really isn't an angle, but rather an orientation vector
	G_SetAngles( ent, dir );
}

/*QUAKED fx_explosion_trail (0 0 1) (-8 -8 -8) (8 8 8) GRAVITY
Creates an explosion type trail using the specified effect file, damaging things as it moves through the environment
Can also be used for something like a meteor, just add an impact effect ( fxFile2 ) and a splashDamage and splashRadius

  GRAVITY - object uses gravity instead of linear motion

  "fxFile" - name of the effect to play for the trail ( default "env/exp_trail_comp" )
  "fxFile2" - effect file to play on impact

  "model" - model to attach to the trail

  "target" - direction to aim the trail in, required unless you specify angles
  "targetname" - (required) trail effect spawns only when used.
  "speed" - velocity through the world, ( default 350 )

  "radius" - damage radius around trail as it travels through the world ( default 128 )
  "damage" - radius damage ( default 128 )
  "splashDamage" - damage when thing impacts ( default 0 )
  "splashRadius" - damage radius on impact ( default 0 )
  "soundset" - soundset to use, start sound plays when explosion trail starts, loop sound plays on explosion trail, end sound plays when it impacts

*/
//----------------------------------------------------------
void SP_fx_explosion_trail( gentity_t *ent )
{
	// We have to be useable, otherwise we won't spawn in
	if ( !ent->targetname )
	{
		gi.Printf( S_COLOR_RED"ERROR: fx_explosion_trail at %s has no targetname specified\n", vtos( ent->s.origin ));
		G_FreeEntity( ent );
		return;
	}

	// Get our defaults
	G_SpawnString( "fxFile", "env/exp_trail_comp", &ent->fxFile );
	G_SpawnInt( "damage", "128", &ent->damage );
	G_SpawnFloat( "radius", "128", &ent->radius );
	G_SpawnFloat( "speed", "350", &ent->speed );

	// Try and associate an effect file, unfortunately we won't know if this worked or not until the CGAME trys to register it...
	ent->fxID = G_EffectIndex( ent->fxFile );

	if ( ent->fullName )
	{
		G_EffectIndex( ent->fullName );
	}

	if ( ent->model )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

	// Give us a bit of time to spawn in the other entities, since we may have to target one of 'em
	ent->e_ThinkFunc = thinkF_fx_explosion_trail_link;
	ent->nextthink = level.time + 500;

	// Save our position and link us up!
	G_SetOrigin( ent, ent->s.origin );

	VectorSet( ent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( ent->maxs, -1, ent->mins );

	gi.linkentity( ent );
}


//
//

//------------------------------------------
void fx_target_beam_set_debounce( gentity_t *self )
{
	if ( self->wait >= FRAMETIME )
	{
		self->attackDebounceTime = level.time + self->wait + Q_irand( -self->random, self->random );
	}
	else if ( self->wait < 0 )
	{
		self->e_UseFunc = useF_NULL;
	}
	else
	{
		self->attackDebounceTime = level.time + FRAMETIME + Q_irand( -self->random, self->random );
	}
}

//------------------------------------------
void fx_target_beam_fire( gentity_t *ent )
{
	trace_t		trace;
	vec3_t		dir, org, end;
	qboolean	open;

	if ( !ent->enemy || !ent->enemy->inuse )
	{//info_null most likely
		ent->enemy = NULL;
		VectorCopy( ent->s.origin2, org );
	}
	else
	{
		VectorCopy( ent->enemy->currentOrigin, org );
	}

	VectorCopy( org, ent->s.origin2 );
	VectorSubtract( org, ent->s.origin, dir );
	VectorNormalize( dir );

	gi.trace( &trace, ent->s.origin, NULL, NULL, org, ENTITYNUM_NONE, MASK_SHOT, G2_NOCOLLIDE, 0 );//ignore
	if ( ent->spawnflags & 2 )
	{
		open = qtrue;
		VectorCopy( org, end );
	}
	else
	{
		open = qfalse;
		VectorCopy( trace.endpos, end );
	}

	if ( trace.fraction < 1.0 )
	{
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{
			gentity_t *victim = &g_entities[trace.entityNum];
			if ( victim && victim->takedamage )
			{
				if ( ent->spawnflags & 4 ) // NO_KNOCKBACK
				{
					G_Damage( victim, ent, ent->activator, dir, trace.endpos, ent->damage, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN );
				}
				else
				{
					G_Damage( victim, ent, ent->activator, dir, trace.endpos, ent->damage, 0, MOD_UNKNOWN );
				}
			}
		}
	}

	G_AddEvent( ent, EV_TARGET_BEAM_DRAW, ent->fxID );
	VectorCopy( end, ent->s.origin2 );

	if ( open )
	{
		VectorScale( dir, -1, ent->pos1 );
	}
	else
	{
		VectorCopy( trace.plane.normal, ent->pos1 );
	}

	ent->e_ThinkFunc = thinkF_fx_target_beam_think;
	ent->nextthink = level.time + FRAMETIME;
}

//------------------------------------------
void fx_target_beam_fire_start( gentity_t *self )
{
	fx_target_beam_set_debounce( self );
	self->e_ThinkFunc		= thinkF_fx_target_beam_think;
	self->nextthink			= level.time + FRAMETIME;
	self->painDebounceTime	= level.time + self->speed + Q_irand( -500, 500 );
	fx_target_beam_fire( self );
}

//------------------------------------------
void fx_target_beam_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->spawnflags & 8 ) // one shot
	{
		fx_target_beam_fire( self );
		self->e_ThinkFunc	= thinkF_NULL;
	}
	else if ( self->e_ThinkFunc == thinkF_NULL )
	{
		self->e_ThinkFunc	= thinkF_fx_target_beam_think;
		self->nextthink		= level.time + 50;
	}
	else
	{
		self->e_ThinkFunc	= thinkF_NULL;
	}

	self->activator = activator;
}

//------------------------------------------
void fx_target_beam_think( gentity_t *ent )
{
	if ( ent->attackDebounceTime > level.time )
	{
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	fx_target_beam_fire_start( ent );
}

//------------------------------------------
void fx_target_beam_link( gentity_t *ent )
{
	gentity_t	*target = NULL;
	vec3_t		dir;

	target = G_Find( target, FOFS(targetname), ent->target );

	if ( !target )
	{
		Com_Printf( "bolt_link: unable to find target %s\n", ent->target );
		G_FreeEntity( ent );
		return;
	}

	ent->attackDebounceTime = level.time;

	if ( !target->classname || Q_stricmp( "info_null", target->classname ) )
	{//don't want to set enemy to something that's going to free itself... actually, this could be bad in other ways, too... ent pointer could be freed up and re-used by the time we check it next
		G_SetEnemy( ent, target );
	}
	VectorSubtract( target->s.origin, ent->s.origin, dir );//er, does it ever use dir?
	VectorNormalize( dir );//er, does it use len or dir?
	vectoangles( dir, ent->s.angles );//er, does it use s.angles?

	VectorCopy( target->s.origin, ent->s.origin2 );

	if ( ent->spawnflags & 1 )
	{
		// Do nothing
		ent->e_ThinkFunc	= thinkF_NULL;
	}
	else
	{
		if ( !(ent->spawnflags & 8 )) // one_shot, only calls when used
		{
			// switch think functions to avoid doing the bolt_link every time
			ent->e_ThinkFunc = thinkF_fx_target_beam_think;
			ent->nextthink	= level.time + FRAMETIME;
		}
	}

	ent->e_UseFunc = useF_fx_target_beam_use;
	gi.linkentity( ent );
}

/*QUAKED fx_target_beam (1 0.5 0.5) (-8 -8 -8) (8 8 8) STARTOFF OPEN NO_KNOCKBACK ONE_SHOT NO_IMPACT
 Emits specified effect file, doing damage if required

STARTOFF - must be used before it's on
OPEN - will draw all the way to the target, regardless of where the trace hits
NO_KNOCKBACK - beam damage does no knockback

 "fxFile" - Effect used to draw the beam, ( default "env/targ_beam" )
 "fxFile2" - Effect used for the beam impact effect, ( default "env/targ_beam_impact" )
 "targetname" - Fires only when used
 "duration" - How many seconds each burst lasts, -1 will make it stay on forever
 "wait" - If always on, how long to wait between blasts, in MILLISECONDS - default/min is 100 (1 frame at 10 fps), -1 means it will never fire again
 "random" - random amount of seconds added to/subtracted from "wait" each firing
 "damage" - How much damage to inflict PER FRAME (so probably want it kind of low), default is none
 "target" - ent to point at- you MUST have this.  This can be anything you want, including a moving ent - for static beams, just use info_null
*/
//------------------------------------------
void SP_fx_target_beam( gentity_t *ent )
{
	G_SetOrigin( ent, ent->s.origin );

	ent->speed	*= 1000;
	ent->wait	*= 1000;
	ent->random *= 1000;

	if ( ent->speed < FRAMETIME )
	{
		ent->speed = FRAMETIME;
	}

	G_SpawnInt( "damage", "0", &ent->damage );
	G_SpawnString( "fxFile", "env/targ_beam", &ent->fxFile );

	if ( ent->spawnflags & 16 ) // NO_IMPACT FX
	{
		ent->delay = 0;
	}
	else
	{
		G_SpawnString( "fxFile2", "env/targ_beam_impact", &ent->fullName );
		ent->delay = G_EffectIndex( ent->fullName );
	}

	ent->fxID = G_EffectIndex( ent->fxFile );

	ent->activator = ent;
	ent->owner	= NULL;

	ent->e_ThinkFunc = thinkF_fx_target_beam_link;
	ent->nextthink	= level.time + START_TIME_LINK_ENTS;

	VectorSet( ent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( ent->maxs, -1, ent->mins );

	gi.linkentity( ent );
}


/*QUAKED fx_cloudlayer (1 0.3 0.5) (-8 -8 -8) (8 8 8) TUBE ALT

  Creates a scalable scrolling cloud layer, mostly for bespin undercity but could be used other places

  TUBE - creates cloud layer with tube opening in the middle, must an INNER radius also
  ALT - uses slightly different shader, good if using two layers sort of close together

"radius" - outer radius of cloud layer, (default 2048)
"random" - inner radius of cloud layer, (default 128) only works for TUBE type
"wait" - adds curvature as it moves out to the edge of the layer.  ( default 0 ), 1 = small up, 3 = up more, -1 = small down, -3 = down more, etc.

*/

void SP_fx_cloudlayer( gentity_t *ent )
{
	// HACK: this effect is never played, rather it just caches the shaders I need cgame side
	G_EffectIndex( "world/haze_cache" );

	G_SpawnFloat( "radius", "2048", &ent->radius );
	G_SpawnFloat( "random", "128", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	ent->s.eType = ET_CLOUD; // dumb

	G_SetOrigin( ent, ent->s.origin );

	ent->contents = 0;
	VectorSet( ent->maxs, 200, 200, 200 );
	VectorScale( ent->maxs, -1, ent->mins );

	gi.linkentity( ent );
}