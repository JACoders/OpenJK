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
#include "../cgame/cg_media.h"

extern team_t TranslateTeamName( const char *name );

//client side shortcut hacks from cg_local.h
//extern void CG_SurfaceExplosion( vec3_t origin, vec3_t normal, float radius, float shake_speed, qboolean smoke );
extern void CG_MiscModelExplosion( vec3_t mins, vec3_t maxs, int size, material_t chunkType );
extern void CG_Chunks( int owner, vec3_t origin, const vec3_t normal, const vec3_t mins, const vec3_t maxs,
							float speed, int numChunks, material_t chunkType, int customChunk, float baseScale );
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

extern	qboolean	player_locked;

//---------------------------------------------------
static void CacheChunkEffects( material_t material )
{
	switch( material )
	{
	default:
		break;
	case MAT_GLASS:
		G_EffectIndex( "chunks/glassbreak" );
		break;
	case MAT_GLASS_METAL:
		G_EffectIndex( "chunks/glassbreak" );
		G_EffectIndex( "chunks/metalexplode" );
		break;
	case MAT_ELECTRICAL:
	case MAT_ELEC_METAL:
		G_EffectIndex( "chunks/sparkexplode" );
		break;
	case MAT_METAL:
	case MAT_METAL2:
	case MAT_METAL3:
	case MAT_CRATE1:
	case MAT_CRATE2:
		G_EffectIndex( "chunks/metalexplode" );
		break;
	case MAT_GRATE1:
		G_EffectIndex( "chunks/grateexplode" );
		break;
	case MAT_DRK_STONE:
	case MAT_LT_STONE:
	case MAT_GREY_STONE:
	case MAT_WHITE_METAL: // what is this crap really supposed to be??
		G_EffectIndex( "chunks/rockbreaklg" );
		G_EffectIndex( "chunks/rockbreakmed" );
		break;
	case MAT_ROPE:
		G_EffectIndex( "chunks/ropebreak" );
//		G_SoundIndex(); // FIXME: give it a sound
		break;
	}
}

//--------------------------------------
void funcBBrushDieGo (gentity_t *self)
{
	vec3_t		org, dir, up;
	gentity_t	*attacker = self->enemy;
	float		scale;
	int			numChunks, size = 0;
	material_t	chunkType = self->material;
	
	// if a missile is stuck to us, blow it up so we don't look dumb
	for ( int i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( g_entities[i].s.groundEntityNum == self->s.number && ( g_entities[i].s.eFlags & EF_MISSILE_STICK ))
		{
			G_Damage( &g_entities[i], self, self, NULL, NULL, 99999, 0, MOD_CRUSH ); //?? MOD?
		}
	}

	//So chunks don't get stuck inside me
	self->s.solid = 0;
	self->contents = 0;
	self->clipmask = 0;
	gi.linkentity(self); 

	VectorSet(up, 0, 0, 1);

	if ( self->target && attacker != NULL )
	{
		G_UseTargets(self, attacker);
	}

	VectorSubtract( self->absmax, self->absmin, org );// size

	numChunks = random() * 6 + 18;

	// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
	// Volume is length * width * height...then break that volume down based on how many chunks we have
	scale = sqrt( sqrt( org[0] * org[1] * org[2] )) * 1.75f;

	if ( scale > 48 )
	{
		size = 2;
	}
	else if ( scale > 24 )
	{
		size = 1;
	}

	scale = scale / numChunks;

	if ( self->radius > 0.0f )
	{
		// designer wants to scale number of chunks, helpful because the above scale code is far from perfect
		//	I do this after the scale calculation because it seems that the chunk size generally seems to be very close, it's just the number of chunks is a bit weak
		numChunks *= self->radius;
	}

	VectorMA( self->absmin, 0.5, org, org );
	VectorAdd( self->absmin,self->absmax, org );
	VectorScale( org, 0.5f, org );

	if ( attacker != NULL && attacker->client )
	{
		VectorSubtract( org, attacker->currentOrigin, dir );
		VectorNormalize( dir );
	}
	else
	{
		VectorCopy(up, dir);
	} 

	if ( !(self->spawnflags & 2048) ) // NO_EXPLOSION
	{
		// we are allowed to explode
		CG_MiscModelExplosion( self->mins, self->maxs, size, chunkType );
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		//explode
		AddSightEvent( attacker, org, 256, AEL_DISCOVERED, 100 );
		AddSoundEvent( attacker, org, 128, AEL_DISCOVERED );
		G_RadiusDamage( org, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

		gentity_t *te = G_TempEntity( org, EV_GENERAL_SOUND );
		te->s.eventParm = G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	}
	else
	{//just break
		AddSightEvent( attacker, org, 128, AEL_DISCOVERED );
		AddSoundEvent( attacker, org, 64, AEL_SUSPICIOUS );
	}

	//FIXME: base numChunks off size?
	CG_Chunks( self->s.number, org, dir, self->mins, self->maxs, 300, numChunks, chunkType, 0, scale );

	gi.AdjustAreaPortalState( self, qtrue );
	self->e_ThinkFunc = thinkF_G_FreeEntity;
	self->nextthink = level.time + 50;
	//G_FreeEntity( self );
}

void funcBBrushDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc)
{
	self->takedamage = qfalse;//stop chain reaction runaway loops

	G_SetEnemy(self, attacker);

	if(self->delay)
	{
		self->e_ThinkFunc = thinkF_funcBBrushDieGo;
		self->nextthink = level.time + floor(self->delay * 1000.0f);
		return;
	}

	funcBBrushDieGo(self);
}

void funcBBrushUse (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior( self, BSET_USE );
	if(self->spawnflags & 64)
	{//Using it doesn't break it, makes it use it's targets
		if(self->target && self->target[0])
		{
			G_UseTargets(self, activator);
		}
	}
	else
	{
		funcBBrushDie(self, other, activator, self->health, MOD_UNKNOWN);
	}
}

void funcBBrushPain(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc)
{
	if ( self->painDebounceTime > level.time )
	{
		return;
	}

	if ( self->paintarget )
	{
		G_UseTargets2 (self, self->activator, self->paintarget);
	}

	G_ActivateBehavior( self, BSET_PAIN );

	if ( self->material == MAT_DRK_STONE
		|| self->material == MAT_LT_STONE
		|| self->material == MAT_GREY_STONE )
	{
		vec3_t	org, dir;
		float	scale;
		VectorSubtract( self->absmax, self->absmin, org );// size
		// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
		// Volume is length * width * height...then break that volume down based on how many chunks we have
		scale = VectorLength( org ) / 100.0f;
		VectorMA( self->absmin, 0.5, org, org );
		VectorAdd( self->absmin,self->absmax, org );
		VectorScale( org, 0.5f, org );
		if ( attacker != NULL && attacker->client )
		{
			VectorSubtract( attacker->currentOrigin, org, dir );
			VectorNormalize( dir );
		}
		else
		{
			VectorSet( dir, 0, 0, 1 );
		} 
		CG_Chunks( self->s.number, org, dir, self->mins, self->maxs, 300, Q_irand( 1, 3 ), self->material, 0, scale );
	}

	if ( self->wait == -1 )
	{
		self->e_PainFunc = painF_NULL;
		return;
	}

	self->painDebounceTime = level.time + self->wait;
}

static void InitBBrush ( gentity_t *ent ) 
{
	float		light;
	vec3_t		color;
	qboolean	lightSet, colorSet;

	VectorCopy( ent->s.origin, ent->pos1 );
	
	gi.SetBrushModel( ent, ent->model );

	ent->e_DieFunc = dieF_funcBBrushDie;
	
	ent->svFlags |= SVF_BBRUSH;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) 
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) 
	{
		int		r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	if(ent->spawnflags & 128)
	{//Can be used by the player's BUTTON_USE
		ent->svFlags |= SVF_PLAYER_USABLE;
	}

	ent->s.eType = ET_MOVER;
	gi.linkentity (ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );
}

void funcBBrushTouch( gentity_t *ent, gentity_t *other, trace_t *trace )
{
}

/*QUAKED func_breakable (0 .8 .5) ? INVINCIBLE IMPACT CRUSHER THIN SABERONLY HEAVY_WEAP USE_NOT_BREAK PLAYER_USE NO_EXPLOSION
INVINCIBLE - can only be broken by being used
IMPACT - does damage on impact
CRUSHER - won't reverse movement when hit an obstacle
THIN - can be broken by impact damage, like glass
SABERONLY - only takes damage from sabers
HEAVY_WEAP - only takes damage by a heavy weapon, like an emplaced gun or AT-ST gun.
USE_NOT_BREAK - Using it doesn't make it break, still can be destroyed by damage
PLAYER_USE - Player can use it with the use button
NO_EXPLOSION - Does not play an explosion effect, though will still create chunks if specified

When destroyed, fires it's trigger and chunks and plays sound "noise" or sound for type if no noise specified

"targetname" entities with matching target will fire it
"paintarget" target to fire when hit (but not destroyed)
"wait"		how long minimum to wait between firing paintarget each time hit
"delay"		When killed or used, how long (in seconds) to wait before blowing up (none by default)
"model2"	.md3 model to also draw
"target"	all entities with a matching targetname will be used when this is destoryed
"health"	default is 10
"radius"  Chunk code tries to pick a good volume of chunks, but you can alter this to scale the number of spawned chunks. (default 1)  (.5) is half as many chunks, (2) is twice as many chunks

Damage: default is none
"splashDamage" - damage to do
"splashRadius" - radius for above damage

"team" - This cannot take damage from members of this team:
	"player"
	"neutral"
	"enemy"

Don't know if these work:  
"color"		constantLight color
"light"		constantLight radius

"material" - default is "0 - MAT_METAL" - choose from this list:
0 = MAT_METAL		(basic blue-grey scorched-DEFAULT)
1 = MAT_GLASS		
2 = MAT_ELECTRICAL	(sparks only)
3 = MAT_ELEC_METAL	(METAL2 chunks and sparks)
4 =	MAT_DRK_STONE	(brown stone chunks)
5 =	MAT_LT_STONE	(tan stone chunks)
6 =	MAT_GLASS_METAL (glass and METAL2 chunks)
7 = MAT_METAL2		(electronic type of metal)
8 = MAT_NONE		(no chunks)
9 = MAT_GREY_STONE	(grey colored stone)
10 = MAT_METAL3		(METAL and METAL2 chunk combo)
11 = MAT_CRATE1		(yellow multi-colored crate chunks)
12 = MAT_GRATE1		(grate chunks--looks horrible right now)
13 = MAT_ROPE		(for yavin_trial, no chunks, just wispy bits )
14 = MAT_CRATE2		(red multi-colored crate chunks)
15 = MAT_WHITE_METAL (white angular chunks for Stu, NS_hideout )

*/
void SP_func_breakable( gentity_t *self ) 
{
	if(!(self->spawnflags & 1))
	{
		if(!self->health)
		{
			self->health = 10;
		}
	}

	if ( self->spawnflags & 16 ) // saber only
	{
		self->flags |= FL_DMG_BY_SABER_ONLY;
	}
	else if ( self->spawnflags & 32 ) // heavy weap
	{
		self->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;
	}

	if (self->health) 
	{
		self->takedamage = qtrue;
	}

	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");//precaching
	G_SpawnFloat( "radius", "1", &self->radius ); // used to scale chunk code if desired by a designer
	G_SpawnInt( "material", "0", (int*)&self->material );
	CacheChunkEffects( self->material );

	self->e_UseFunc = useF_funcBBrushUse;

	//if ( self->paintarget )
	{
		self->e_PainFunc = painF_funcBBrushPain;
	}

	self->e_TouchFunc = touchF_funcBBrushTouch;

	if ( self->team && self->team[0] )
	{
		self->noDamageTeam = TranslateTeamName( self->team );
		if(self->noDamageTeam == TEAM_FREE)
		{
			G_Error("team name %s not recognized", self->team);
		}
	}
	self->team = NULL;
	if (!self->model) {
		G_Error("func_breakable with NULL model");
	}
	InitBBrush( self );
}


void misc_model_breakable_pain ( gentity_t *self, gentity_t *inflictor, gentity_t *other, vec3_t point, int damage, int mod,int hitLoc )
{
	if ( self->health > 0 )
	{
		// still alive, react to the pain
		if ( self->paintarget )
		{
			G_UseTargets2 (self, self->activator, self->paintarget);
		}

		// Don't do script if dead
		G_ActivateBehavior( self, BSET_PAIN );
	}
}

void misc_model_breakable_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc ) 
{
	int		numChunks;
	float	size = 0, scale;
	vec3_t	dir, up, dis;

	//NOTE: Stop any scripts that are currently running (FLUSH)... ?
	//Turn off animation
	self->s.frame = self->startFrame = self->endFrame = 0;
	self->svFlags &= ~SVF_ANIMATING;
			
	self->health = 0;

	//Throw some chunks
	AngleVectors( self->s.apos.trBase, dir, NULL, NULL );
	VectorNormalize( dir );

	numChunks = random() * 6 + 20;

	VectorSubtract( self->absmax, self->absmin, dis );

	// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
	// Volume is length * width * height...then break that volume down based on how many chunks we have
	scale = sqrt( sqrt( dis[0] * dis[1] * dis[2] )) * 1.75f;

	if ( scale > 48 )
	{
		size = 2;
	}
	else if ( scale > 24 )
	{
		size = 1;
	}

	scale = scale / numChunks;

	if ( self->radius > 0.0f )
	{
		// designer wants to scale number of chunks, helpful because the above scale code is far from perfect
		//	I do this after the scale calculation because it seems that the chunk size generally seems to be very close, it's just the number of chunks is a bit weak
		numChunks *= self->radius;
	}

	VectorAdd( self->absmax, self->absmin, dis );
	VectorScale( dis, 0.5f, dis );

	CG_Chunks( self->s.number, dis, dir, self->absmin, self->absmax, 300, numChunks, self->material, self->s.modelindex3, scale );

	self->e_PainFunc = painF_NULL;
	self->e_DieFunc  = dieF_NULL;
//	self->e_UseFunc  = useF_NULL;

	self->takedamage = qfalse;

	if ( !(self->spawnflags & 4) )
	{//We don't want to stay solid
		self->s.solid = 0;
		self->contents = 0;
		self->clipmask = 0;
		gi.linkentity(self);
	}

	VectorSet(up, 0, 0, 1);

	if(self->target)
	{
		G_UseTargets(self, attacker);
	}

	if(inflictor->client)
	{
		VectorSubtract( self->currentOrigin, inflictor->currentOrigin, dir );
		VectorNormalize( dir );
	}
	else
	{
		VectorCopy(up, dir);
	}

	if ( !(self->spawnflags & 2048) ) // NO_EXPLOSION
	{
		// Ok, we are allowed to explode, so do it now!
		if(self->splashDamage > 0 && self->splashRadius > 0)
		{//explode
			vec3_t org;
			AddSightEvent( attacker, self->currentOrigin, 256, AEL_DISCOVERED, 100 );
			AddSoundEvent( attacker, self->currentOrigin, 128, AEL_DISCOVERED );
			//FIXME: specify type of explosion?  (barrel, electrical, etc.)  Also, maybe just use the explosion effect below since it's
			//				a bit better?
			// up the origin a little for the damage check, because several models have their origin on the ground, so they don't alwasy do damage, not the optimal solution...
			VectorCopy( self->currentOrigin, org );
			if ( self->mins[2] > -4 )
			{//origin is going to be below it or very very low in the model
				//center the origin
				org[2] = self->currentOrigin[2] + self->mins[2] + (self->maxs[2] - self->mins[2])/2.0f;
			}
			G_RadiusDamage( org, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

			if ( self->model && Q_stricmp( "models/map_objects/ships/tie_fighter.md3", self->model ) == 0 )
			{//TEMP HACK for Tie Fighters- they're HUGE
				G_PlayEffect( "fighter_explosion2", self->currentOrigin );
				G_Sound( self, G_SoundIndex( "sound/weapons/tie_fighter/TIEexplode.wav" ) );
			}
			else
			{
				CG_MiscModelExplosion( self->absmin, self->absmax, size, self->material );
				G_Sound( self, G_SoundIndex("sound/weapons/explosions/cargoexplode.wav") );
			}
		}
		else
		{//just break
			AddSightEvent( attacker, self->currentOrigin, 128, AEL_DISCOVERED );
			AddSoundEvent( attacker, self->currentOrigin, 64, AEL_SUSPICIOUS );
			// This is the default explosion
			CG_MiscModelExplosion( self->absmin, self->absmax, size, self->material );
			G_Sound(self, G_SoundIndex("sound/weapons/explosions/cargoexplode.wav"));
		}
	}

	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;

	if(self->s.modelindex2 != -1 && !(self->spawnflags & 8))
	{//FIXME: modelindex doesn't get set to -1 if the damage model doesn't exist
		self->svFlags |= SVF_BROKEN;
		self->s.modelindex = self->s.modelindex2;
		G_ActivateBehavior( self, BSET_DEATH );
	}
	else
	{
		G_FreeEntity( self );
	}
}

void misc_model_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->health <= 0 && self->max_health > 0)
	{//used while broken fired target3
		G_UseTargets2( self, activator, self->target3 );
		return;
	}

	G_ActivateBehavior( self, BSET_USE );
	//Don't explode if they've requested it to not
	if ( self->spawnflags & 64 )
	{//Usemodels toggling
		if ( self->spawnflags & 32 )
		{
			if( self->s.modelindex == self->sound1to2 )
			{
				self->s.modelindex = self->sound2to1;
			}
			else
			{
				self->s.modelindex = self->sound1to2;
			}
		}

		return;
	}

	misc_model_breakable_die( self, other, activator, self->health, MOD_UNKNOWN );
}

#define MDL_OTHER			0
#define MDL_ARMOR_HEALTH	1
#define MDL_AMMO			2

void misc_model_breakable_init( gentity_t *ent )
{
	int		type;

	type = MDL_OTHER;

	if (!ent->model) {
		G_Error("no model set on %s at (%.1f %.1f %.1f)", ent->classname, ent->s.origin[0],ent->s.origin[1],ent->s.origin[2]);
	}
	//Main model
	ent->s.modelindex = ent->sound2to1 = G_ModelIndex( ent->model );

	if ( ent->spawnflags & 1 )
	{//Blocks movement
		ent->contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//Was CONTENTS_SOLID, but only architecture should be this
	}
	else if ( ent->health )
	{//Can only be shot
		ent->contents = CONTENTS_SHOTCLIP;
	}

	if (type == MDL_OTHER)
	{
		ent->e_UseFunc = useF_misc_model_use;	
	}
	else if ( type == MDL_ARMOR_HEALTH )
	{
//		G_SoundIndex("sound/player/suithealth.wav");
		ent->e_UseFunc = useF_health_use;
		if (!ent->count)
		{
			ent->count = 100;
		}
		ent->health = 60;
	}
	else if ( type == MDL_AMMO )
	{
//		G_SoundIndex("sound/player/suitenergy.wav");
		ent->e_UseFunc = useF_ammo_use;
		if (!ent->count)
		{
			ent->count = 100;
		}
		ent->health = 60;
	}

	if ( ent->health ) 
	{
		G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
		ent->max_health = ent->health;
		ent->takedamage = qtrue;
		ent->e_PainFunc = painF_misc_model_breakable_pain;
		ent->e_DieFunc  = dieF_misc_model_breakable_die;
	}
}

void TieFighterThink ( gentity_t *self )
{
	gentity_t *player = &g_entities[0];

	if ( self->health <= 0 )
	{
		return;
	}

	self->nextthink = level.time + FRAMETIME;
	if ( player )
	{
		vec3_t	playerDir, fighterDir, fwd, rt;
		float	playerDist, fighterSpeed;

		//use player eyepoint
		VectorSubtract( player->currentOrigin, self->currentOrigin, playerDir );
		playerDist = VectorNormalize( playerDir );
		VectorSubtract( self->currentOrigin, self->lastOrigin, fighterDir );
		VectorCopy( self->currentOrigin, self->lastOrigin );
		fighterSpeed = VectorNormalize( fighterDir )*1000;
		AngleVectors( self->currentAngles, fwd, rt, NULL );

		if ( fighterSpeed ) 
		{
			float	side;

			// Magic number fun!  Speed is used for banking, so modulate the speed by a sine wave
			fighterSpeed *= sin( ( 100 ) * 0.003 );

			// Clamp to prevent harsh rolling
			if ( fighterSpeed > 10 )
				fighterSpeed = 10;

			side = fighterSpeed * DotProduct( fighterDir, rt );
			self->s.apos.trBase[2] -= side;
		}

		//FIXME: bob up/down, strafe left/right some
		float dot = DotProduct( playerDir, fighterDir );
		if ( dot > 0 )
		{//heading toward the player
			if ( playerDist < 1024 )
			{
				if ( DotProduct( playerDir, fwd ) > 0.7 )
				{//facing the player
					if ( self->attackDebounceTime < level.time )
					{
						gentity_t	*bolt;

						bolt = G_Spawn();
						
						bolt->classname = "tie_proj";
						bolt->nextthink = level.time + 10000;
						bolt->e_ThinkFunc = thinkF_G_FreeEntity;
						bolt->s.eType = ET_MISSILE;
						bolt->s.weapon = WP_BLASTER;
						bolt->owner = self;
						bolt->damage = 30;
						bolt->dflags = DAMAGE_NO_KNOCKBACK;		// Don't push them around, or else we are constantly re-aiming
						bolt->splashDamage = 0;
						bolt->splashRadius = 0;
						bolt->methodOfDeath = MOD_ENERGY;	// ?
						bolt->clipmask = MASK_SHOT;

						bolt->s.pos.trType = TR_LINEAR;
						bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
						VectorCopy( self->currentOrigin, bolt->s.pos.trBase );
						VectorScale( fwd, 8000, bolt->s.pos.trDelta );
						SnapVector( bolt->s.pos.trDelta );		// save net bandwidth
						VectorCopy( self->currentOrigin, bolt->currentOrigin);

						if ( !Q_irand( 0, 2 ) )
						{
							G_SoundOnEnt( bolt, CHAN_VOICE, "sound/weapons/tie_fighter/tie_fire.wav" );
						}
						else
						{
							G_SoundOnEnt( bolt, CHAN_VOICE, va( "sound/weapons/tie_fighter/tie_fire%d.wav", Q_irand( 2, 3 ) ) );
						}
						self->attackDebounceTime = level.time + Q_irand( 300, 2000 );
					}
				}
			}
		}

		if ( playerDist < 1024 )//512 )
		{//within range to start our sound
			if ( dot > 0 )
			{
				if ( !self->fly_sound_debounce_time )
				{//start sound
					G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/tie_fighter/tiepass%d.wav", Q_irand( 1, 5 ) ) );
					self->fly_sound_debounce_time = 2000;
				}
				else
				{//sound already started
					self->fly_sound_debounce_time = -1;
				}
			}
		}
		else if ( self->fly_sound_debounce_time < level.time )
		{
			self->fly_sound_debounce_time = 0;
		}
	}
}

void misc_model_breakable_gravity_init( gentity_t *ent, qboolean dropToFloor )
{
	ent->s.eType = ET_GENERAL;
	ent->s.eFlags |= EF_BOUNCE_HALF;
	ent->clipmask = MASK_SOLID|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//?
	ent->physicsBounce = ent->mass = VectorLength( ent->maxs ) + VectorLength( ent->mins );
	//drop to floor
	trace_t		tr;
	vec3_t		top, bottom;

	if ( dropToFloor )
	{
		VectorCopy( ent->currentOrigin, top );
		top[2] += 1;
		VectorCopy( ent->currentOrigin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		gi.trace( &tr, top, ent->mins, ent->maxs, bottom, ent->s.number, MASK_NPCSOLID, G2_NOCOLLIDE, 0 );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			G_SetOrigin( ent, tr.endpos );
			gi.linkentity( ent );
		}
	}
	else
	{
		G_SetOrigin( ent, ent->currentOrigin );
		gi.linkentity( ent );
	}
	//set up for object thinking
	if ( VectorCompare( ent->s.pos.trDelta, vec3_origin ) )
	{//not moving
		ent->s.pos.trType = TR_STATIONARY;
	}
	else
	{
		ent->s.pos.trType = TR_GRAVITY;
	}
	VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
	VectorClear( ent->s.pos.trDelta );
	ent->s.pos.trTime = level.time;
	if ( VectorCompare( ent->s.apos.trDelta, vec3_origin ) )
	{//not moving
		ent->s.apos.trType = TR_STATIONARY;
	}
	else
	{
		ent->s.apos.trType = TR_LINEAR;
	}
	VectorCopy( ent->currentAngles, ent->s.apos.trBase );
	VectorClear( ent->s.apos.trDelta );
	ent->s.apos.trTime = level.time;
	ent->nextthink = level.time + FRAMETIME;
	ent->e_ThinkFunc = thinkF_G_RunObject;
}
/*QUAKED misc_model_breakable (1 0 0) (-16 -16 -16) (16 16 16) SOLID AUTOANIMATE DEADSOLID NO_DMODEL NO_SMOKE USE_MODEL USE_NOT_BREAK PLAYER_USE NO_EXPLOSION
SOLID - Movement is blocked by it, if not set, can still be broken by explosions and shots if it has health
AUTOANIMATE - Will cycle it's anim
DEADSOLID - Stay solid even when destroyed (in case damage model is rather large).
NO_DMODEL - Makes it NOT display a damage model when destroyed, even if one exists
USE_MODEL - When used, will toggle to it's usemodel (model name + "_u1.md3")... this obviously does nothing if USE_NOT_BREAK is not checked
USE_NOT_BREAK - Using it, doesn't make it break, still can be destroyed by damage
PLAYER_USE - Player can use it with the use button
NO_EXPLOSION - By default, will explode when it dies...this is your override.

"model"		arbitrary .md3 file to display
"health"	how much health to have - default is zero (not breakable)  If you don't set the SOLID flag, but give it health, it can be shot but will not block NPCs or players from moving
"targetname" when used, dies and displays damagemodel, if any (if not, removes itself)
"target" What to use when it dies
"target2" What to use when it's repaired
"target3" What to use when it's used while it's broken
"paintarget" target to fire when hit (but not destroyed)
"count"  the amount of armor/health/ammo given (default 50)
"gravity"	if set to 1, this will be affected by gravity
"radius"  Chunk code tries to pick a good volume of chunks, but you can alter this to scale the number of spawned chunks. (default 1)  (.5) is half as many chunks, (2) is twice as many chunks

Damage: default is none
"splashDamage" - damage to do (will make it explode on death)
"splashRadius" - radius for above damage

"team" - This cannot take damage from members of this team:
	"player"
	"neutral"
	"enemy"

"material" - default is "8 - MAT_NONE" - choose from this list:
0 = MAT_METAL		(grey metal)
1 = MAT_GLASS		
2 = MAT_ELECTRICAL	(sparks only)
3 = MAT_ELEC_METAL	(METAL chunks and sparks)
4 =	MAT_DRK_STONE	(brown stone chunks)
5 =	MAT_LT_STONE	(tan stone chunks)
6 =	MAT_GLASS_METAL (glass and METAL chunks)
7 = MAT_METAL2		(blue/grey metal)
8 = MAT_NONE		(no chunks-DEFAULT)
9 = MAT_GREY_STONE	(grey colored stone)
10 = MAT_METAL3		(METAL and METAL2 chunk combo)
11 = MAT_CRATE1		(yellow multi-colored crate chunks)
12 = MAT_GRATE1		(grate chunks--looks horrible right now)
13 = MAT_ROPE		(for yavin_trial, no chunks, just wispy bits )
14 = MAT_CRATE2		(red multi-colored crate chunks)
15 = MAT_WHITE_METAL (white angular chunks for Stu, NS_hideout )
FIXME/TODO: 
set size better?
multiple damage models?
custom explosion effect/sound?
*/
void SP_misc_model_breakable( gentity_t *ent ) 
{
	char	damageModel[MAX_QPATH];
	char	chunkModel[MAX_QPATH];
	char	useModel[MAX_QPATH];
	int		len;
	
	// Chris F. requested default for misc_model_breakable to be NONE...so don't arbitrarily change this.
	G_SpawnInt( "material", "8", (int*)&ent->material );
	G_SpawnFloat( "radius", "1", &ent->radius ); // used to scale chunk code if desired by a designer
	CacheChunkEffects( ent->material );

	misc_model_breakable_init( ent );

	len = strlen( ent->model ) - 4;
	strncpy( damageModel, ent->model, len );
	damageModel[len] = 0;	//chop extension
	strncpy( chunkModel, damageModel, sizeof(chunkModel));
	strncpy( useModel, damageModel, sizeof(useModel));
	
	if (ent->takedamage) {
		//Dead/damaged model
		if( !(ent->spawnflags & 8) ) {	//no dmodel
			strcat( damageModel, "_d1.md3" );
			ent->s.modelindex2 = G_ModelIndex( damageModel );
		}
		
		//Chunk model
		strcat( chunkModel, "_c1.md3" );
		ent->s.modelindex3 = G_ModelIndex( chunkModel );
	}

	//Use model
	if( ent->spawnflags & 32 ) {	//has umodel
		strcat( useModel, "_u1.md3" );
		ent->sound1to2 = G_ModelIndex( useModel );
	}
	if ( !ent->mins[0] && !ent->mins[1] && !ent->mins[2] )
	{
		VectorSet (ent->mins, -16, -16, -16);
	}
	if ( !ent->maxs[0] && !ent->maxs[1] && !ent->maxs[2] )
	{
		VectorSet (ent->maxs, 16, 16, 16);
	}

	if ( ent->spawnflags & 2 )
	{
		ent->s.eFlags |= EF_ANIM_ALLFAST;
	}

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	gi.linkentity (ent);

	if ( ent->spawnflags & 128 )
	{//Can be used by the player's BUTTON_USE
		ent->svFlags |= SVF_PLAYER_USABLE;
	}

	if ( ent->team && ent->team[0] )
	{
		ent->noDamageTeam = TranslateTeamName( ent->team );
		if ( ent->noDamageTeam == TEAM_FREE )
		{
			G_Error("team name %s not recognized", ent->team);
		}
	}
	
	ent->team = NULL;

	//HACK
	if ( ent->model && Q_stricmp( "models/map_objects/ships/tie_fighter.md3", ent->model ) == 0 )
	{//run a think
		G_EffectIndex( "fighter_explosion2" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass1.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass2.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass3.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass4.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass5.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire2.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire3.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/TIEexplode.wav" );
		ent->e_ThinkFunc = thinkF_TieFighterThink;
		ent->nextthink = level.time + FRAMETIME;
	}
	float grav = 0;
	G_SpawnFloat( "gravity", "0", &grav );
	if ( grav )
	{//affected by gravity
		G_SetAngles( ent, ent->s.angles );
		G_SetOrigin( ent, ent->currentOrigin );
		misc_model_breakable_gravity_init( ent, qtrue );
	}
}


//----------------------------------
//
// Breaking Glass Technology
//
//----------------------------------

// Really naughty cheating.  Put in an EVENT at some point...
extern void cgi_R_GetBModelVerts(int bmodelIndex, vec3_t *verts, vec3_t normal );	
extern void CG_DoGlass( vec3_t verts[4], vec3_t normal, vec3_t dmgPt, vec3_t dmgDir, float dmgRadius );
extern	cgs_t			cgs;

//-----------------------------------------------------
void funcGlassDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	vec3_t		verts[4], normal;

	// if a missile is stuck to us, blow it up so we don't look dumb....we could, alternately, just let the missile drop off??
	for ( int i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( g_entities[i].s.groundEntityNum == self->s.number && ( g_entities[i].s.eFlags & EF_MISSILE_STICK ))
		{
			G_Damage( &g_entities[i], self, self, NULL, NULL, 99999, 0, MOD_CRUSH ); //?? MOD?
		}
	}

	// Really naughty cheating.  Put in an EVENT at some point...
	cgi_R_GetBModelVerts( cgs.inlineDrawModel[self->s.modelindex], verts, normal );
	CG_DoGlass( verts, normal, self->pos1, self->pos2, self->splashRadius );

	self->takedamage = qfalse;//stop chain reaction runaway loops

	G_SetEnemy( self, self->enemy );

	//So chunks don't get stuck inside me
	self->s.solid = 0;
	self->contents = 0;
	self->clipmask = 0;
	gi.linkentity(self); 

	if ( self->target && attacker != NULL )
	{
		G_UseTargets( self, attacker );
	}

	gi.AdjustAreaPortalState( self, qtrue );
	G_FreeEntity( self );
}

//-----------------------------------------------------
void funcGlassUse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	vec3_t temp1, temp2;

	// For now, we just break on use
	G_ActivateBehavior( self, BSET_USE );

	VectorAdd( self->mins, self->maxs, temp1 );
	VectorScale( temp1, 0.5f, temp1 );

	VectorAdd( other->mins, other->maxs, temp2 );
	VectorScale( temp2, 0.5f, temp2 );

	VectorSubtract( temp1, temp2, self->pos2 );
	VectorCopy( temp1, self->pos1 );

	VectorNormalize( self->pos2 );
	VectorScale( self->pos2, 390, self->pos2 );

	self->splashRadius = 40; // ?? some random number, maybe it's ok?

	funcGlassDie( self, other, activator, self->health, MOD_UNKNOWN );
}

//-----------------------------------------------------
/*QUAKED func_glass (0 .8 .5) ? INVINCIBLE
When destroyed, fires it's target, breaks into glass chunks and plays glass noise
For now, instantly breaks on impact

INVINCIBLE - can only be broken by being used

"targetname" entities with matching target will fire it
"target"	all entities with a matching targetname will be used when this is destroyed
"health"	default is 1
*/
//-----------------------------------------------------
void SP_func_glass( gentity_t *self ) 
{
	if ( !(self->spawnflags & 1 ))
	{
		if ( !self->health )
		{
			self->health = 1;
		}
	}

	if ( self->health ) 
	{
		self->takedamage = qtrue;
	}

	self->e_UseFunc = useF_funcGlassUse;
	self->e_DieFunc = dieF_funcGlassDie;

	VectorCopy( self->s.origin, self->pos1 );
	
	gi.SetBrushModel( self, self->model );
	self->svFlags |= (SVF_GLASS_BRUSH|SVF_BBRUSH);
	self->material = MAT_GLASS;

	self->s.eType = ET_MOVER;

	self->s.pos.trType = TR_STATIONARY;
	VectorCopy( self->pos1, self->s.pos.trBase );

	G_SoundIndex( "sound/effects/glassbreak1.wav" );
	G_EffectIndex( "glass_impact" );

	gi.linkentity( self );
}

qboolean G_EntIsBreakable( int entityNum )
{
	if ( entityNum < 0 || entityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}

	gentity_t *ent = &g_entities[entityNum];
	if ( (ent->svFlags&SVF_GLASS_BRUSH) )
	{
		return qtrue;
	}
	if ( (ent->svFlags&SVF_BBRUSH) )
	{
		return qtrue;
	}
	if ( !Q_stricmp( "misc_model_breakable", ent->classname ) )
	{
		return qtrue;
	}
	if ( !Q_stricmp( "misc_maglock", ent->classname ) )
	{
		return qtrue;
	}

	return qfalse;
}
