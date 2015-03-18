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
#include "b_local.h"
#include "anims.h"

#define ENTDIST_PLAYER	1
#define ENTDIST_NPC		2

extern qboolean G_PointInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs );
extern qboolean G_ClearTrace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int ignore, int clipmask );
extern qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
extern qboolean PM_CrouchAnim( int anim );

void InitTrigger( gentity_t *self ) {
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	gi.SetBrushModel( self, self->model );
	self->contents = CONTENTS_TRIGGER;		// replaces the -1 from gi.SetBrushModel
	self->svFlags = SVF_NOCLIENT;

	if(self->spawnflags & 128)
	{
		self->svFlags |= SVF_INACTIVE;
	}
}


// the wait time has passed, so set back up for another activation
void multi_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger_run( gentity_t *ent ) 
{
	ent->e_ThinkFunc = thinkF_NULL;

	G_ActivateBehavior( ent, BSET_USE );

	if ( ent->soundSet && ent->soundSet[0] )
	{
		gi.SetConfigstring( CS_AMBIENT_SET, ent->soundSet );
	}

	G_UseTargets (ent, ent->activator);
	if ( ent->noise_index ) 
	{
		G_Sound( ent->activator, ent->noise_index );
	}

	if ( ent->target2 && ent->target2[0] && ent->wait >= 0 )
	{
		ent->e_ThinkFunc = thinkF_trigger_cleared_fire;
		ent->nextthink = level.time + ent->speed;
	}
	else if ( ent->wait > 0 ) 
	{
		if ( ent->painDebounceTime != level.time )
		{//first ent to touch it this frame
			//ent->e_ThinkFunc = thinkF_multi_wait;
			ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
			ent->painDebounceTime = level.time;
		}
	} 
	else if ( ent->wait < 0 )
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->contents &= ~CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		ent->e_TouchFunc = touchF_NULL;
		ent->e_UseFunc = useF_NULL;
		//Don't remove, Icarus may barf?
		//ent->nextthink = level.time + FRAMETIME;
		//ent->think = G_FreeEntity;
	}
	if( ent->activator && ent->activator->s.number == 0 )
	{	// mark the trigger as being touched by the player
		ent->aimDebounceTime = level.time;
	}
}


void multi_trigger( gentity_t *ent, gentity_t *activator ) 
{
	if ( ent->e_ThinkFunc == thinkF_multi_trigger_run )
	{//already triggered, just waiting to run
		return;
	}

	if ( ent->nextthink > level.time ) 
	{
		if( ent->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in a single frame
		{
			if ( ent->painDebounceTime && ent->painDebounceTime != level.time )
			{//this should still allow subsequent ents to fire this trigger in the current frame
				return;		// can't retrigger until the wait is over
			}
		}
		else
		{
			return;
		}

	}

	// if the player has already activated this trigger this frame
	if( activator && !activator->s.number && ent->aimDebounceTime == level.time )
	{
		return;	
	}

	if ( ent->svFlags & SVF_INACTIVE )
	{//Not active at this time
		return;
	}

	ent->activator = activator;

	if(ent->delay && ent->painDebounceTime < (level.time + ent->delay) )
	{//delay before firing trigger
		ent->e_ThinkFunc = thinkF_multi_trigger_run;
		ent->nextthink = level.time + ent->delay;
		ent->painDebounceTime = level.time;
		
	}
	else
	{
		multi_trigger_run (ent);
	}
}

void Use_Multi( gentity_t *ent, gentity_t *other, gentity_t *activator ) 
{
	multi_trigger( ent, activator );
}

void Touch_Multi( gentity_t *self, gentity_t *other, trace_t *trace ) 
{
	if( !other->client ) 
	{
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if( self->noDamageTeam )
	{
		if ( other->client->playerTeam != self->noDamageTeam )
		{
			return;
		}
	}

// moved to just above multi_trigger because up here it just checks if the trigger is not being touched
// we want it to check any conditions set on the trigger, if one of those isn't met, the trigger is considered to be "cleared"
//	if ( self->e_ThinkFunc == thinkF_trigger_cleared_fire )
//	{//We're waiting to fire our target2 first
//		self->nextthink = level.time + self->speed;
//		return;
//	}

	if ( self->spawnflags & 1 )
	{
		if ( other->s.number != 0 )
		{
			return;
		}
	}
	else
	{
		if ( self->spawnflags & 16 )
		{//NPCONLY
			if ( other->NPC == NULL )
			{
				return;
			}
		}

		if ( self->NPC_targetname && self->NPC_targetname[0] )
		{
			if ( other->script_targetname && other->script_targetname[0] )
			{
				if ( Q_stricmp( self->NPC_targetname, other->script_targetname ) != 0 )
				{//not the right guy to fire me off
					return;
				}
			}
			else
			{
				return;
			}
		}
	}

	if ( self->spawnflags & 2 )
	{//FACING
		vec3_t	forward;

		if ( other->client )
		{
			AngleVectors( other->client->ps.viewangles, forward, NULL, NULL );
		}
		else
		{
			AngleVectors( other->currentAngles, forward, NULL, NULL );
		}

		if ( DotProduct( self->movedir, forward ) < 0.5 )
		{//Not Within 45 degrees
			return;
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if ( !other->client )
		{
			return;
		}

		if( !( other->client->usercmd.buttons & BUTTON_USE ) )
		{//not pressing use button
			return;
		}
	}

	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		if ( !other->client )
		{
			return;
		}

		if( !( other->client->ps.eFlags & EF_FIRING /*usercmd.buttons & BUTTON_ATTACK*/ ) &&
			!( other->client->ps.eFlags & EF_ALT_FIRING/*usercmd.buttons & BUTTON_ALT_ATTACK*/ ) )
		{//not pressing fire button or altfire button
			return;
		}

		//FIXME: do we care about the sniper rifle or not?

		if( other->s.number == 0 && ( other->client->ps.weapon > MAX_PLAYER_WEAPONS || other->client->ps.weapon <= WP_NONE ) )
		{//don't care about non-player weapons if this is the player
			return;
		}
	}

	if ( other->client && self->radius )
	{
		vec3_t	eyeSpot;

		//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		VectorCopy(other->currentOrigin, eyeSpot);
		eyeSpot[2] += other->client->ps.viewheight;

		if ( G_PointInBounds( eyeSpot, self->absmin, self->absmax ) )
		{
			if( !( other->client->ps.eFlags & EF_FIRING ) &&
				!( other->client->ps.eFlags & EF_ALT_FIRING ) )
			{//not attacking, so hiding bonus
				//FIXME:  should really have sound events clear the hiddenDist
				other->client->hiddenDist = self->radius;
				//NOTE: movedir HAS to be normalized!
				if ( VectorLength( self->movedir ) )
				{//They can only be hidden from enemies looking in this direction
					VectorCopy( self->movedir, other->client->hiddenDir );
				}
				else
				{
					VectorClear( other->client->hiddenDir );
				}
			}
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		NPC_SetAnim( other, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		/*
		if ( !VectorLengthSquared( other->client->ps.velocity ) && !PM_CrouchAnim( other->client->ps.legsAnim ) )
		{
			NPC_SetAnim( other, SETANIM_LEGS, BOTH_BUTTON_HOLD, SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
		}
		*/
		//other->client->ps.weaponTime = other->client->ps.torsoAnimTimer;
	}
	
	if ( self->e_ThinkFunc == thinkF_trigger_cleared_fire )
	{//We're waiting to fire our target2 first
		self->nextthink = level.time + self->speed;
		return;
	}

	multi_trigger( self, other );
}

void trigger_cleared_fire (gentity_t *self)
{
	G_UseTargets2( self, self->activator, self->target2 );
	self->e_ThinkFunc = thinkF_NULL;
	// should start the wait timer now, because the trigger's just been cleared, so we must "wait" from this point
	if ( self->wait > 0 ) 
	{
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
}

qboolean G_TriggerActive( gentity_t *self )
{
	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return qfalse;
	}

	if ( self->spawnflags & 1 )
	{//player only
		return qfalse;
	}

	/*
	???
	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		return qfalse;
	}
	*/

	/*
	???
	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		return qfalse;
	}
	*/

	/*
	if ( self->radius )
	{//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		return qfalse;
	}
	*/
	return qtrue;
}

/*QUAKED trigger_multiple (.1 .5 .1) ? PLAYERONLY FACING USE_BUTTON FIRE_BUTTON NPCONLY x x INACTIVE MULTIPLE
PLAYERONLY - only a player can trigger this by touch
FACING - Won't fire unless triggering ent's view angles are within 45 degrees of trigger's angles (in addition to any other conditions)
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
FIRE_BUTTON - Won't fire unless player/NPC is in it and pressing fire button (in addition to any other conditions)
NPCONLY - only non-player NPCs can trigger this by touch
INACTIVE - Start off, has to be activated to be touchable/usable
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"wait"		Seconds between triggerings, 0 default, number < 0 means one time only.
"random"	wait variance, default is 0
"delay"		how many seconds to wait to fire targets after tripped
"hiderange" As long as NPC's head is in this trigger, NPCs out of this hiderange cannot see him.  If you set an angle on the trigger, they're only hidden from enemies looking in that direction.  the player's crouch viewheight is 36, his standing viewheight is 54.  So a trigger thast should hide you when crouched but not standing should be 48 tall.
"target2"	The trigger will fire this only when the trigger has been activated and subsequently 'cleared'( once any of the conditions on the trigger have not been satisfied).  This will not fire the "target" more than once until the "target2" is fired (trigger field is 'cleared')
"speed"		How many seconds to wait to fire the target2, default is 1
"noise"		Sound to play when the trigger fires (plays at activator's origin)

Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"team" - If set, only this team can trip this trigger
	player
	enemy
	neutral

"soundSet"	Ambient sound set to play when this trigger is activated
*/
team_t TranslateTeamName( const char *name );
void SP_trigger_multiple( gentity_t *ent ) 
{
	char	buffer[MAX_QPATH];
	char	*s;
	if ( G_SpawnString( "noise", "*NOSOUND*", &s ) ) 
	{
		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		ent->noise_index = G_SoundIndex(buffer);
	}
	
	G_SpawnFloat( "wait", "0", &ent->wait );//was 0.5 ... but that means wait can never be zero... we should probably put it back to 0.5, though...
	G_SpawnFloat( "random", "0", &ent->random );


	if ( (ent->wait > 0) && (ent->random >= ent->wait) ) {
		ent->random = ent->wait - FRAMETIME;
		gi.Printf(S_COLOR_YELLOW"trigger_multiple has random >= wait\n");
	}

	ent->delay *= 1000;//1 = 1 msec, 1000 = 1 sec
	if ( !ent->speed && ent->target2 && ent->target2[0] )
	{
		ent->speed = 1000;
	}
	else
	{
		ent->speed *= 1000;
	}

	ent->e_TouchFunc = touchF_Touch_Multi;
	ent->e_UseFunc   = useF_Use_Multi;

	if ( ent->team && ent->team[0] )
	{
		ent->noDamageTeam = TranslateTeamName( ent->team );
		ent->team = NULL;
	}

	InitTrigger( ent );
	gi.linkentity (ent);
}

/*QUAKED trigger_once (.5 1 .5) ? PLAYERONLY FACING USE_BUTTON FIRE_BUTTON NPCONLY x x INACTIVE MULTIPLE
PLAYERONLY - only a player can trigger this by touch
FACING - Won't fire unless triggering ent's view angles are within 45 degrees of trigger's angles (in addition to any other conditions)
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
FIRE_BUTTON - Won't fire unless player/NPC is in it and pressing fire button (in addition to any other conditions)
NPCONLY - only non-player NPCs can trigger this by touch
INACTIVE - Start off, has to be activated to be touchable/usable
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"random"	wait variance, default is 0
"delay"		how many seconds to wait to fire targets after tripped
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
"noise"		Sound to play when the trigger fires (plays at activator's origin)

"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"team" - If set, only this team can trip this trigger
	player
	enemy
	neutral

"soundSet"	Ambient sound set to play when this trigger is activated
*/
void SP_trigger_once( gentity_t *ent ) 
{
	char	buffer[MAX_QPATH];
	char	*s;
	if ( G_SpawnString( "noise", "*NOSOUND*", &s ) ) 
	{
		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		ent->noise_index = G_SoundIndex(buffer);
	}

	ent->wait = -1;

	ent->e_TouchFunc = touchF_Touch_Multi;
	ent->e_UseFunc   = useF_Use_Multi;

	if ( ent->team && ent->team[0] )
	{
		ent->noDamageTeam = TranslateTeamName( ent->team );
		ent->team = NULL;
	}

	ent->delay *= 1000;//1 = 1 msec, 1000 = 1 sec

	InitTrigger( ent );
	gi.linkentity (ent);
}


/*QUAKED trigger_bidirectional (.1 .5 .1) ? PLAYER_ONLY x x x x x x INACTIVE
NOT IMPLEMENTED
INACTIVE - Start off, has to be activated to be touchable/usable

set "angle" for forward direction
Fires "target" when someone moves through it in direction of angle
Fires "backwardstarget" when someone moves through it in the opposite direction of angle

"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger

"wait" - how long to wait between triggerings

  TODO:
	count
*/
void SP_trigger_bidirectional( gentity_t *ent ) 
{
	G_FreeEntity(ent);
	//FIXME: Implement
/*	if(!ent->wait)
	{
		ent->wait = -1;
	}

	ent->touch = Touch_Multi;
	ent->use = Use_Multi;
	
	InitTrigger( ent );
	gi.linkentity (ent);
*/
}

/*QUAKED trigger_location (.1 .5 .1) ? 
When an ent is asked for it's location, it will return this ent's "message" field if it is in it.
  "message" - location name

  NOTE: always rectangular
*/
char *G_GetLocationForEnt( gentity_t *ent )
{
	vec3_t		mins, maxs;
	gentity_t	*found = NULL;

	VectorAdd( ent->currentOrigin, ent->mins, mins );
	VectorAdd( ent->currentOrigin, ent->maxs, maxs );

	while( (found = G_Find(found, FOFS(classname), "trigger_location")) != NULL )
	{
		if ( gi.EntityContact( mins, maxs, found ) ) 
		{
			return found->message;
		}
	}

	return NULL;
}

void SP_trigger_location( gentity_t *ent ) 
{
	if ( !ent->message || !ent->message[0] )
	{
		gi.Printf("WARNING: trigger_location with no message!\n");
		G_FreeEntity(ent);
		return;
	}

	gi.SetBrushModel( ent, ent->model );
	ent->contents = 0;
	ent->svFlags = SVF_NOCLIENT;

	gi.linkentity (ent);
}
/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think( gentity_t *ent ) {
	G_UseTargets(ent, ent);
	G_FreeEntity( ent );
}

/*QUAKED trigger_always (.1 .5 .1) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->e_ThinkFunc = thinkF_trigger_always_think;
}


/*
==============================================================================

trigger_push

==============================================================================
*/
#define PUSH_CONVEYOR 32	
void trigger_push_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if( level.time < self->painDebounceTime + self->wait  ) // normal 'wait' check
	{
		if( self->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in one frame
		{
			if ( self->painDebounceTime && level.time > self->painDebounceTime ) // if we haven't reached the next frame continue to let ents touch the trigger
			{
				return;
			}
		}
		else // only allowing one ent per frame to touch trigger
		{
			return;
		}
	}

	// if the player has already activated this trigger this frame
	if( other && !other->s.number && self->aimDebounceTime == level.time )
	{
		return;		
	}
	
	
	if( self->spawnflags & PUSH_CONVEYOR )
	{   // only push player if he's on the ground
		if( other->s.groundEntityNum == ENTITYNUM_NONE )
		{
			return;
		}
	}

	if ( self->spawnflags & 1 )
	{//PLAYERONLY
		if ( other->s.number != 0 )
		{
			return;
		}
	}
	else
	{
		if ( self->spawnflags & 8 )
		{//NPCONLY
			if ( other->NPC == NULL )
			{
				return;
			}
		}
	}

	if ( !other->client ) {
		if ( other->s.pos.trType != TR_STATIONARY && other->s.pos.trType != TR_LINEAR_STOP && other->s.pos.trType != TR_NONLINEAR_STOP && VectorLengthSquared( other->s.pos.trDelta ) )
		{//already moving
			VectorCopy( other->currentOrigin, other->s.pos.trBase );
			VectorCopy( self->s.origin2, other->s.pos.trDelta );
			other->s.pos.trTime = level.time;
		}
		return;
	}

	if ( other->client->ps.pm_type != PM_NORMAL ) {
		return;
	}
	
	if ( (self->spawnflags&16) )
	{//relative, dir to it * speed
		vec3_t dir;
		VectorSubtract( self->s.origin2, other->currentOrigin, dir );
		if ( self->speed )
		{
			VectorNormalize( dir );
			VectorScale( dir, self->speed, dir );
		}
		VectorCopy( dir, other->client->ps.velocity );
	}
	else if ( (self->spawnflags&4) )
	{//linear dir * speed
		VectorScale( self->s.origin2, self->speed, other->client->ps.velocity );
	}
	else
	{
		VectorCopy( self->s.origin2, other->client->ps.velocity );
	}
	//so we don't take damage unless we land lower than we start here...
	other->client->ps.forceJumpZStart = 0;
	other->client->ps.pm_flags |= PMF_TRIGGER_PUSHED;//pushed by a trigger
	other->client->ps.jumpZStart = other->client->ps.origin[2];

	if ( self->wait == -1 )
	{
		self->e_TouchFunc = touchF_NULL;
	}
	else if ( self->wait > 0 )
	{
		self->painDebounceTime = level.time;
		
	}
	if( other && !other->s.number )
	{	// mark that the player has activated this trigger this frame
		self->aimDebounceTime =level.time;
	}
}

#define PUSH_CONSTANT 2

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget( gentity_t *self ) 
{
	gentity_t	*ent;
	vec3_t		origin;
	float		height, gravity, time, forward;
	float		dist;

	VectorAdd( self->absmin, self->absmax, origin );
	VectorScale ( origin, 0.5, origin );

	ent = G_PickTarget( self->target );
	if ( !ent ) 
	{
		G_FreeEntity( self );
		return;
	}

	if ( self->classname && !Q_stricmp( "trigger_push", self->classname ) )
	{
		if ( (self->spawnflags&2) )
		{//check once a second to see if we should activate or deactivate ourselves
			self->e_ThinkFunc = thinkF_trigger_push_checkclear;
			self->nextthink = level.time + FRAMETIME;
		}

		if ( (self->spawnflags&16) )
		{//relative, not an arc or linear
			VectorCopy( ent->currentOrigin, self->s.origin2 );
			return;
		}
		else if ( (self->spawnflags&4) )
		{//linear, not an arc
			VectorSubtract( ent->currentOrigin, origin, self->s.origin2 );
			VectorNormalize( self->s.origin2 );
			return;
		}
	}

	if ( self->classname && !Q_stricmp( "target_push", self->classname ) )
	{
		if( self->spawnflags & PUSH_CONSTANT )
		{
			VectorSubtract ( ent->s.origin, self->s.origin, self->s.origin2 );
			VectorNormalize( self->s.origin2);
			VectorScale (self->s.origin2, self->speed, self->s.origin2);
			return;
		}
	}
	height = ent->s.origin[2] - origin[2];
	if ( height < 0 )
	{//sqrt of negative is bad!
		height = 0;
	}
	gravity = g_gravity->value;
	if ( gravity < 0 )
	{
		gravity = 0;
	}
	time = sqrt( height / ( .5 * gravity ) );
	if ( !time ) {
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract ( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[2] = 0;
	dist = VectorNormalize( self->s.origin2);

	forward = dist / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[2] = time * gravity;
}

void trigger_push_checkclear( gentity_t *self )
{
	trace_t	trace;
	vec3_t	center;

	self->nextthink = level.time + 500;

	VectorAdd( self->absmin, self->absmax, center );
	VectorScale( center, 0.5, center );

	gentity_t *target = G_Find( NULL, FOFS(targetname), self->target );
	gi.trace( &trace, center, vec3_origin, vec3_origin, target->currentOrigin, ENTITYNUM_NONE, CONTENTS_SOLID, G2_NOCOLLIDE, 0 );

	if ( trace.fraction >= 1.0f )
	{//can trace, turn on
		self->contents |= CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		self->e_TouchFunc = touchF_trigger_push_touch;
		gi.linkentity( self );
	}
	else
	{//no trace, turn off
		self->contents &= ~CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		self->e_TouchFunc = touchF_NULL;
		gi.unlinkentity( self );
	}
}
/*QUAKED trigger_push (.1 .5 .1) ? PLAYERONLY CHECKCLEAR LINEAR NPCONLY RELATIVE CONVEYOR x INACTIVE MULTIPLE
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
PLAYERONLY - only the player is affected
LINEAR - Instead of tossing the client at the target_position, it will push them towards it.  Must set a "speed" (see below)
CHECKCLEAR - Every 1 second, it will check to see if it can trace to the target_position, if it can, the trigger is touchable, if it can't, the trigger is not touchable
NPCONLY - only NPCs are affected
RELATIVE - instead of pushing you in a direction that is always from the center of the trigger to the target_position, it pushes *you* toward the target position, relative to your current location (can use with "speed"... if don't set a speed, it will use the distance from you to the target_position)
CONVEYOR - acts like a conveyor belt, will only push if player is on the ground ( should probably use RELATIVE also, if you want a true conveyor belt )
INACTIVE - not active until targeted by a target_activate
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

wait - how long to wait between pushes: -1 = push only once
speed - when used with the LINEAR spawnflag, pushes the client toward the position at a constant speed (default is 1000)
*/
void SP_trigger_push( gentity_t *self ) {
	InitTrigger (self);

	if ( self->wait > 0 )
	{
		self->wait *= 1000;
	}

	// unlike other triggers, we need to send this one to the client
	self->svFlags &= ~SVF_NOCLIENT;

	self->s.eType = ET_PUSH_TRIGGER;
	if ( !(self->spawnflags&2) )
	{//start on
		self->e_TouchFunc = touchF_trigger_push_touch;
	}
	if ( self->spawnflags & 4 )
	{//linear
		self->speed = 1000;
	}
	self->e_ThinkFunc = thinkF_AimAtTarget;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
	gi.linkentity (self);
}

void Use_target_push( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL ) {
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	VectorCopy( self->s.origin2, activator->client->ps.velocity );

	if( self->spawnflags & 4 ) // lower
	{
		// reset this so I don't take falling damage when I land
		activator->client->ps.jumpZStart = activator->currentOrigin[2];
	}

	//so we don't take damage unless we land lower than we start here...
	activator->client->ps.forceJumpZStart = 0;
	activator->client->ps.pm_flags |= PMF_TRIGGER_PUSHED;//pushed by a trigger

	// play fly sound every 1.5 seconds
	if ( self->noise_index && activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound( activator, self->noise_index );
	}
}


/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) ENERGYNOISE CONSTANT NO_DAMAGE
When triggered, pushes the activator in the direction of angles
"speed"		defaults to 1000
ENERGYNOISE plays energy noise
CONSTANT will push activator in direction of 'target' at constant 'speed'
NO_DAMAGE the activator won't take falling damage after being pushed
*/
void SP_target_push( gentity_t *self ) {
	
	
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		//self->noise_index = G_SoundIndex("sound/ambience/forge/antigrav.wav");
	}
	if ( self->target ) {

		VectorCopy( self->s.origin, self->absmin );
		VectorCopy( self->s.origin, self->absmax );
		self->e_ThinkFunc = thinkF_AimAtTarget;
		self->nextthink = level.time + START_TIME_LINK_ENTS;
		
	}
	self->e_UseFunc = useF_Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/
#define SNAP_ANGLES 1
#define NO_MISSILES 2
#define NO_NPCS		4
#define TTSF_STASIS		8
#define TTSF_DEAD_OK	16
void TeleportMover( gentity_t *mover, vec3_t origin, vec3_t diffAngles, qboolean snapAngle );
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace ) 
{
	gentity_t	*dest;

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}
	
	dest = 	G_PickTarget( self->target );
	if (!dest) 
	{
		gi.Printf ("Couldn't find teleporter destination\n");
		return;
	}

	if ( other->client ) 
	{
		if ( other->client->ps.pm_type == PM_DEAD ) 
		{
			if ( !(self->spawnflags&TTSF_DEAD_OK) )
			{//dead men can't teleport
				return;
			}
		}
		if ( other->NPC )
		{
			if ( self->spawnflags & NO_NPCS )
			{
				return;
			}
		}

		if ( other->client->playerTeam != TEAM_FREE && SpotWouldTelefrag2( other, dest->currentOrigin ) )//SpotWouldTelefrag( dest, other->client->playerTeam ) )
		{//Don't go through if something blocking on the other side
			return;
		}
		
		TeleportPlayer( other, dest->s.origin, dest->s.angles );
	}
	//FIXME: check for SVF_NO_TELEPORT
	else if ( !(self->svFlags & SVF_NO_TELEPORT) && !(self->spawnflags & NO_MISSILES) && VectorLengthSquared( other->s.pos.trDelta ) )
	{//It's a mover of some sort and is currently moving
		vec3_t	diffAngles = {0, 0, 0};
		qboolean	snap = qfalse;

		if ( self->lastEnemy )
		{
			VectorSubtract( dest->s.angles, self->lastEnemy->s.angles, diffAngles );
		}
		else
		{//snaps to angle
			VectorSubtract( dest->s.angles, other->currentAngles, diffAngles );
			snap = qtrue;
		}

		TeleportMover( other, dest->s.origin, diffAngles, snap );
	}
}

void trigger_teleporter_find_closest_portal( gentity_t *self )
{
	gentity_t *found = NULL;
	vec3_t		org, vec;
	float		dist, bestDist = 64*64;

	VectorAdd( self->mins, self->maxs, org );
	VectorScale( org, 0.5, org );
	while ( (found = G_Find( found, FOFS(classname), "misc_portal_surface" )) != NULL )
	{
		VectorSubtract( found->currentOrigin, org, vec );
		dist = VectorLengthSquared( vec );
		if ( dist < bestDist )
		{
			self->lastEnemy = found;
			bestDist = dist;
		}
	}

	if ( self->lastEnemy )
	{
		gi.Printf("trigger_teleporter found misc_portal_surface\n");
	}
	self->e_ThinkFunc = thinkF_NULL;
}

/*QUAKED trigger_teleport (.1 .5 .1) ? SNAP_ANGLES NO_MISSILES NO_NPCS STASIS DEAD_OK x x INACTIVE
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

SNAP_ANGLES - Make the entity that passes through snap to the target_position's angles
NO_MISSILES - Missiles and thrown objects cannot pass through
NO_NPCS - NPCs cannot pass through
STASIS - will play stasis teleport sound and fx instead of starfleet
DEAD_OK - even if dead, you will teleport
*/
void SP_trigger_teleport( gentity_t *self ) 
{
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	self->svFlags &= ~SVF_NOCLIENT;

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->e_TouchFunc = touchF_trigger_teleporter_touch;

	self->e_ThinkFunc = thinkF_trigger_teleporter_find_closest_portal;
	self->nextthink = level.time + START_TIME_LINK_ENTS;

	gi.linkentity (self);
}



/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.1 .5 .1) ? START_OFF PLAYERONLY SILENT NO_PROTECTION LOCKCAM FALLING ELECTRICAL INACTIVE MULTIPLE
Any entity that touches this will be hurt.
It does dmg points of damage each server frame

PLAYERONLY		only the player is hurt by it
SILENT			supresses playing the sound
NO_PROTECTION	*nothing* stops the damage
LOCKCAM			Falling death results in camera locking in place
FALLING			Forces a falling scream and anim
ELECTRICAL		does electrical damage
INACTIVE		Cannot be triggered until used by a target_activate
MULTIPLE        multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"dmg"			default 5 (whole numbers only)
"delay"			How many seconds it takes to get from 0 to "dmg" (default is 0)
"wait"			Use in instead of "SLOW" - determines how often the player gets hurt, 0.1 is every frame, 1.0 is once per second.  -1 will stop after one use
"count"			If set, FALLING death causes a fade to black in this many milliseconds (default is 10000 = 10 seconds)
"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"noise"         sound to play when it hurts something ( default: "sound/world/electro" )
*/
void hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {

	G_ActivateBehavior(self,BSET_USE);

	//FIXME: Targeting the trigger will toggle its on / off state???
	if ( self->linked ) {
		gi.unlinkentity( self );
	} else {
		gi.linkentity( self );
	}
}

void trigger_hurt_reset (gentity_t *self)
{
	self->attackDebounceTime = 0;
	self->e_ThinkFunc = thinkF_NULL;
}

void hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace ) 
{
	int		dflags;
	int		actualDmg = self->damage;

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}
	
	if ( !other->takedamage ) 
	{
		return;
	}
	
	if( level.time < self->painDebounceTime + self->wait  ) // normal 'wait' check
	{
		if( self->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in one frame
		{
			if ( self->painDebounceTime && level.time > self->painDebounceTime ) // if we haven't reached the next frame continue to let ents touch the trigger
			{
				return;
			}
		}
		else // only allowing one ent per frame to touch trigger
		{
			return;
		}
	}

	// if the player has already activated this trigger this frame
	if( other && !other->s.number && self->aimDebounceTime == level.time )
	{
		return;		
	}


	if ( self->spawnflags & 2 )
	{//player only
		if ( other->s.number )
		{
			return;
		}
	}

	if ( self->NPC_targetname && self->NPC_targetname[0] )
	{//I am for you, Kirk
		if ( other->script_targetname && other->script_targetname[0] )
		{//must have a name
			if ( Q_stricmp( self->NPC_targetname, other->script_targetname ) != 0 )
			{//not the right guy to fire me off
				return;
			}
		}
		else
		{//no name?  No trigger.
			return;
		}
	}

	// play sound
	if ( !(self->spawnflags & 4) ) 
	{
		G_Sound( other, self->noise_index );
	}

	if ( self->spawnflags & 8 )
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}
	
	if ( self->delay )
	{//Increase dmg over time
		if ( self->attackDebounceTime < self->delay )
		{//FIXME: this is for the entire trigger, not per person, so if someone else jumped in after you were in it for 5 seconds, they'd get damaged faster
			actualDmg = floor( (double)(self->damage * self->attackDebounceTime / self->delay) );
		}
		self->attackDebounceTime += FRAMETIME;

		self->e_ThinkFunc = thinkF_trigger_hurt_reset;
		self->nextthink = level.time + FRAMETIME*2;
	}

	if ( actualDmg )
	{
		if (( self->spawnflags & 64 ) && other->client )//electrical damage
		{
			// zap effect
			other->s.powerups |= ( 1 << PW_SHOCKED );
			other->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
		}

		if ( self->spawnflags & 32 )
		{//falling death
			G_Damage (other, self, self, NULL, NULL, actualDmg, dflags|DAMAGE_NO_ARMOR, MOD_FALLING);
			// G_Damage will free this ent, which makes it s.number 0, so we must check inuse...
			if ( !other->s.number && other->health <= 0 )
			{
				if ( self->count )
				{
					extern void CGCam_Fade( vec4_t source, vec4_t dest, float duration );
					float	src[4] = {0,0,0,0},dst[4]={0,0,0,1};
					CGCam_Fade( src, dst, self->count );
				}
				if ( self->spawnflags & 16 )
				{//lock cam
					cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_CDP;
					cg.overrides.thirdPersonCameraDamp = 0;
				}
				if ( other->client )
				{
					other->client->ps.pm_flags |= PMF_SLOW_MO_FALL;
				}
				//G_SoundOnEnt( other, CHAN_VOICE, "*falling1.wav" );//CHAN_VOICE_ATTEN?
			}
		}
		else
		{
			G_Damage (other, self, self, NULL, NULL, actualDmg, dflags, MOD_TRIGGER_HURT);
		}
		if( other && !other->s.number )
		{
			self->aimDebounceTime = level.time;
		}
		if (( self->spawnflags & 64 ) && other->client && other->health <= 0 )//electrical damage
		{//just killed them, make the effect last longer since dead clients don't touch triggers
			other->client->ps.powerups[PW_SHOCKED] = level.time + 10000;
		}
		self->painDebounceTime = level.time;
	}

	if ( self->wait < 0 )
	{
		self->e_TouchFunc = touchF_NULL;
	}
}

void SP_trigger_hurt( gentity_t *self ) 
{
	char	buffer[MAX_QPATH];
	char	*s;

	InitTrigger (self);

	if ( !( self->spawnflags & 4 )) 
	{
		G_SpawnString( "noise", "sound/world/electro", &s );

		Q_strncpyz( buffer, s, sizeof(buffer) );
		self->noise_index = G_SoundIndex(buffer);
	}

	self->e_TouchFunc = touchF_hurt_touch;

	if ( !self->damage ) {
		self->damage = 5;
	}
	
	self->delay *= 1000;
	self->wait *= 1000;

	self->contents = CONTENTS_TRIGGER;

	if ( self->targetname ) {//NOTE: for some reason, this used to be: if(self->spawnflags&2)
		self->e_UseFunc = useF_hurt_use;
	}

	// link in to the world if starting active
	if ( !(self->spawnflags & 1) )
	{
		gi.linkentity (self);
	}
	else // triggers automatically get linked into the world by SetBrushModel, so we have to unlink it here
	{
		gi.unlinkentity(self);
	}
}


/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void func_timer_think( gentity_t *self ) {
	G_UseTargets (self, self->activator);
	// set time before next firing
	self->nextthink = level.time + 1000 * ( self->wait + crandom() * self->random );
}

void func_timer_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	self->activator = activator;

	G_ActivateBehavior(self,BSET_USE);


	// if on, turn it off
	if ( self->nextthink ) {
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think (self);
}

void SP_func_timer( gentity_t *self ) {
	G_SpawnFloat( "random", "1", &self->random);
	G_SpawnFloat( "wait", "1", &self->wait );

	self->e_UseFunc   = useF_func_timer_use;
	self->e_ThinkFunc = thinkF_func_timer_think;

	if ( self->random >= self->wait ) {
		self->random = self->wait - FRAMETIME;
		gi.Printf( "func_timer at %s has random >= wait\n", vtos( self->s.origin ) );
	}

	if ( self->spawnflags & 1 ) {
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->svFlags = SVF_NOCLIENT;
}

/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED trigger_entdist (.1 .5 .1) (-8 -8 -8) (8 8 8) PLAYER NPC
fires if the given entity is within the given distance.  Sets itself inactive after one use.
----- KEYS -----
distance - radius entity can be away to fire trigger
target - fired if entity is within distance
target2 - fired if entity not within distance

NPC_target - NPC_types to look for
ownername - If any, who to calc the distance from- default is the trigger_entdist himself
example: target "biessman telsia" will look for the biessman and telsia NPC
if it finds either of these within distance it will fire.

  todo - 
  add delay, count
  add monster classnames?????
  add LOS to it???
*/

void trigger_entdist_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	vec3_t		diff;
	gentity_t	*found = NULL;
	gentity_t	*owner = NULL;
	qboolean	useflag;
	const char	*token, *holdString;

	if ( self->svFlags & SVF_INACTIVE )	// Don't use INACTIVE
		return;

	G_ActivateBehavior(self,BSET_USE);

	if(self->ownername && self->ownername[0])
	{
		owner = G_Find(NULL, FOFS(targetname), self->ownername);
	}

	if(owner == NULL)
	{
		owner = self;
	}

	self->activator = activator;

	useflag = qfalse;

	self->svFlags |= SVF_INACTIVE;	// Make it inactive after one use

	if (self->spawnflags & ENTDIST_PLAYER)	// Look for player???
	{
		found = &g_entities[0];

		if (found)
		{	
			VectorSubtract(owner->currentOrigin, found->currentOrigin, diff);
			if(VectorLength(diff) < self->count)
			{
				useflag = qtrue;
			}
		}
	}

	if ((self->spawnflags & ENTDIST_NPC) && (!useflag))
	{
		holdString = self->NPC_target;

		while (holdString)
		{
			token = COM_Parse( &holdString);
			if ( !token ) // Nothing left to look at
			{
				break;
			}

			found = G_Find(found, FOFS(targetname), token);	// Look for the specified NPC
			if (found)	//Found???
			{	
				VectorSubtract(owner->currentOrigin, found->currentOrigin, diff);
				if(VectorLength(diff) < self->count)	// Within distance
				{
					useflag = qtrue;
					break;
				}
			}
		}
	}

	if (useflag)
	{
		G_UseTargets2 (self, self->activator, self->target);
	}
	else if (self->target2)
	{
		// This is the negative target
		G_UseTargets2 (self, self->activator, self->target2);
	}	


}

void SP_trigger_entdist( gentity_t *self ) 
{
	G_SpawnInt( "distance", "0", &self->count);

	self->e_UseFunc = useF_trigger_entdist_use;

}



void trigger_visible_check_player_visibility( gentity_t *self )
{
	//Check every FRAMETIME*2
	self->nextthink = level.time + FRAMETIME*2;

	if ( self->svFlags & SVF_INACTIVE )
	{
		return;
	}

	vec3_t	dir;
	float	dist;
	gentity_t	*player = &g_entities[0];

	if (!player || !player->client )
	{
		return;
	}

	//1: see if player is within 512*512 range
	VectorSubtract( self->currentOrigin, player->client->renderInfo.eyePoint, dir );
	dist = VectorNormalize( dir );
	if ( dist < self->radius )
	{//Within range
		vec3_t	forward;
		float	dot;
		//2: see if dot to us and player viewangles is > 0.7
		AngleVectors( player->client->renderInfo.eyeAngles, forward, NULL, NULL );
		dot = DotProduct( forward, dir );
		if ( dot > self->random )
		{//Within the desired FOV
			//3: see if player is in PVS
			if ( gi.inPVS( self->currentOrigin, player->client->renderInfo.eyePoint ) )
			{
				vec3_t	mins = {-1, -1, -1};
				vec3_t	maxs = {1, 1, 1};
				//4: If needbe, trace to see if there is clear LOS from player viewpos
				if ( (self->spawnflags&1) || G_ClearTrace( player->client->renderInfo.eyePoint, mins, maxs, self->currentOrigin, 0, MASK_OPAQUE ) )
				{
					//5: Fire!
					G_UseTargets( self, player );
					//6: Remove yourself
					G_FreeEntity( self );
				}
			}
		}
	}

}

/*QUAKED trigger_visible (.1 .5 .1) (-8 -8 -8) (8 8 8) NOTRACE x x x x x x INACTIVE

  Only fires when player is looking at it, fires only once then removes itself.

  NOTRACE - Doesn't check to make sure the line of sight is completely clear (penetrates walls, forcefields, etc)
  INACTIVE - won't check for player visibility until activated

  radius - how far this ent can be from player's eyes, max, and still be considered "seen"
  FOV - how far off to the side of the player's field of view this can be, max, and still be considered "seen".  Player FOV is 80, so the default for this value is 30.

  "target" - What to use when it fires.
*/
void SP_trigger_visible( gentity_t *self )
{
	if ( self->radius <= 0 )
	{
		self->radius = 512;
	}

	if ( self->random <= 0 )
	{//about 30 degrees
		self->random = 0.7f;
	}
	else
	{//convert from FOV degrees to number meaningful for dot products
		self->random = 1.0f - (self->random/90.0f);
	}

	if ( self->spawnflags & 128 )
	{// Make it inactive
		self->svFlags |= SVF_INACTIVE;	
	}

	G_SetOrigin( self, self->s.origin );
	gi.linkentity( self );

	self->e_ThinkFunc = thinkF_trigger_visible_check_player_visibility;
	self->nextthink = level.time + FRAMETIME*2;
}