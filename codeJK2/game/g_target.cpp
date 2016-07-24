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
#include "g_headers.h"

#include "g_local.h"
#include "g_functions.h"
#include "g_icarus.h"
//#include "Q3_Interface.h"
extern void Q3_DebugPrint( int level, const char *format, ... );
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	G_ActivateBehavior(ent,BSET_USE);

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		gi.unlinkentity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->e_UseFunc = useF_Use_Target_Give;
}

//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent ) 
{
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) 
{
	G_ActivateBehavior(ent,BSET_USE);

	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->e_ThinkFunc = thinkF_Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) 
{
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) 
	{
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) 
	{
		ent->wait = 1;
	}

	ent->e_UseFunc = useF_Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) 
{
	G_ActivateBehavior(ent,BSET_USE);

	AddScore( activator, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->e_UseFunc = useF_Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8)
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator) 
{
	G_ActivateBehavior(ent,BSET_USE);

	if ( activator->client ) {
		gi.SendServerCommand( activator-g_entities, "cp \"%s\"", ent->message );
	}
}

void SP_target_print( gentity_t *ent ) {
	ent->e_UseFunc = useF_Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

"sounds" va() min max, so if your sound string is borgtalk%d.wav, and you set a "sounds" value of 4, it will randomly play borgtalk1.wav - borgtalk4.wav when triggered
to use this, you must store the wav name in "soundGroup", NOT "noise"

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if(ent->painDebounceTime > level.time)
	{
		return;
	}

	G_ActivateBehavior(ent,BSET_USE);

	if ( ent->sounds )
	{
		ent->noise_index = G_SoundIndex( va( ent->paintarget, Q_irand(1, ent->sounds ) ) );
	}

	if (ent->spawnflags & 3) {	// looping sound toggles
		gentity_t *looper = ent;
		if ( ent->spawnflags & 8 ) {
			looper = activator;
		}
		if (looper->s.loopSound)
			looper->s.loopSound = 0;	// turn it off
		else
			looper->s.loopSound = ent->noise_index;	// start it
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}

	if(ent->wait < 0)
	{//BYE!
		ent->e_UseFunc = useF_NULL;
	}
	else
	{
		ent->painDebounceTime = level.time + ent->wait;
	}
}

void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s;
	int		i;

	if ( VALIDSTRING( ent->soundSet ) )
	{
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		gi.linkentity (ent);
		return;
	}

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if(!ent->sounds)
	{
		if ( !G_SpawnString( "noise", "*NOSOUND*", &s ) ) {
			G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
		}

		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".wav");
		ent->noise_index = G_SoundIndex(buffer);
	}
	else
	{//Precache all possible sounds
		for( i = 0; i < ent->sounds; i++ )
		{
			ent->noise_index = G_SoundIndex( va( ent->paintarget, i+1 ) );
		}
	}

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;

	ent->wait *= 1000;

	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->e_UseFunc = useF_Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (ent);
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think (gentity_t *self) {
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->mins, point);
		VectorMA (point, 0.5, self->enemy->maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	gi.trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE, G2_NOCOLLIDE, 0);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir, 
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_ENERGY );
	}

	VectorCopy (tr.endpos, self->s.origin2);

	gi.linkentity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	gi.unlinkentity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			gi.Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		G_SetEnemy( self, ent );
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->e_UseFunc   = useF_target_laser_use;
	self->e_ThinkFunc = thinkF_target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	// let everything else get spawned before we start firing
	self->e_ThinkFunc = thinkF_target_laser_start;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;

	if (!activator->client)
		return;

	G_ActivateBehavior(self,BSET_USE);

	dest = 	G_PickTarget( self->target );
	if (!dest) {
		gi.Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAK-ED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if (!self->targetname)
		gi.Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->e_UseFunc = useF_target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM x x x x INACTIVE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them

INACTIVE  Can't be used until activated

  "delay" - Will actually fire this many seconds after being used
  "wait" - Cannot be fired again until this many seconds after the last time it was used
*/
void target_relay_use_go (gentity_t *self ) 
{
	G_ActivateBehavior( self, BSET_USE );
	
	if ( self->spawnflags & 4 ) 
	{
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && (ent->e_UseFunc != useF_NULL) ) 
		{	// e_UseFunc check can be omitted
			GEntity_UseFunc( ent, self, self->activator );			
		}
		return;
	}

	G_UseTargets( self, self->activator );
}

void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) 
{
	if ( ( self->spawnflags & 1 ) && activator->client ) 
	{//&& activator->client->ps.persistant[PERS_TEAM] != TEAM_RED ) {
		return;
	}

	if ( ( self->spawnflags & 2 ) && activator->client ) 
	{//&& activator->client->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( self->painDebounceTime > level.time )
	{
		return;
	}

	G_SetEnemy( self, other );
	self->activator = activator;

	if ( self->delay )
	{
		self->e_ThinkFunc = thinkF_target_relay_use_go;
		self->nextthink = level.time + self->delay;
		return;
	}

	target_relay_use_go( self );

	if ( self->wait < 0 )
	{
		self->e_UseFunc = useF_NULL;
	}
	else
	{
		self->painDebounceTime = level.time + self->wait;
	}
}

void SP_target_relay (gentity_t *self) 
{
	self->e_UseFunc = useF_target_relay_use;
	self->wait *= 1000;
	self->delay *= 1000;
	if ( self->spawnflags&128 )
	{
		self->svFlags |= SVF_INACTIVE;
	}
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8) FALLING ELECTRICAL
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {

	G_ActivateBehavior(self,BSET_USE);

	if ( self->spawnflags & 1 )
	{//falling death
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_FALLING );
		if ( !activator->s.number && activator->health <= 0 && 1 )
		{
			extern void CGCam_Fade( vec4_t source, vec4_t dest, float duration );
			float	src[4] = {0,0,0,0},dst[4]={0,0,0,1};
			CGCam_Fade( src, dst, 10000 );
		}
	}
	else if ( self->spawnflags & 2 ) // electrical
	{
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_ELECTROCUTE );
		
		if ( activator->client )
		{
			activator->s.powerups |= ( 1 << PW_SHOCKED );
			activator->client->ps.powerups[PW_SHOCKED] = level.time + 4000;
		}
	}
	else
	{
		G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
	}
}

void SP_target_kill( gentity_t *self ) 
{
	self->e_UseFunc = useF_target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
info_notnull does the same thing
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}

//static -slc
void target_location_linkup(gentity_t *ent)
{
	int i;

	if (level.locationLinked) 
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	for (i = 0, ent = g_entities; i < globals.num_entities; i++, ent++) {
		if (ent->classname && !Q_stricmp(ent->classname, "target_location")) {
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ){
	self->e_ThinkFunc = thinkF_target_location_linkup;
	self->nextthink = level.time + 1000;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}

//===NEW===================================================================

/*QUAKED target_counter (1.0 0 0) (-4 -4 -4) (4 4 4) x x x x x x x INACTIVE
Acts as an intermediary for an action that takes multiple inputs.

INACTIVE cannot be used until used by a target_activate

target2 - what the counter should fire each time it's incremented and does NOT reach it's count

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.

bounceCount - number of times the counter should reset to it's full count when it's done
*/

void target_counter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count == 0 )
	{
		return;
	}
	
	//gi.Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
	self->count--;

	if ( activator )
	{
		Q3_DebugPrint( WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname, (self->max_health-self->count), self->max_health );
	}

	if ( self->count )
	{
		if ( self->target2 )
		{
			//gi.Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
			G_UseTargets2( self, activator, self->target2 );
		}
		return;
	}
	
	G_ActivateBehavior( self,BSET_USE );

	if ( self->spawnflags & 128 )
	{
		self->svFlags |= SVF_INACTIVE;
	}

	self->activator = activator;
	G_UseTargets( self, activator );

	if ( self->count == 0 )
	{
		if ( self->bounceCount == 0 )
		{
			return;
		}
		self->count = self->max_health;
		if ( self->bounceCount > 0 )
		{//-1 means bounce back forever
			self->bounceCount--; 
		}
	}
}

void SP_target_counter (gentity_t *self)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}
	//if ( self->bounceCount > 0 )//let's always set this anyway
	{//we will reset when we use up our count, remember our initial count
		self->max_health = self->count;
	}

	self->e_UseFunc = useF_target_counter_use;
}

/*QUAKED target_random (.5 .5 .5) (-4 -4 -4) (4 4 4) USEONCE
Randomly fires off only one of it's targets each time used

USEONCE	set to never fire again
*/

void target_random_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			t_count = 0, pick;
	gentity_t	*t = NULL;

	//gi.Printf("target_random %s used by %s (entnum %d)\n", self->targetname, activator->targetname, activator->s.number );
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{
		self->e_UseFunc = useF_NULL;
	}

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return;
	}

	if(t_count == 1)
	{
		G_UseTargets (self, activator);
		return;
	}

	//FIXME: need a seed
	pick = Q_irand(1, t_count);
	t_count = 0;
	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
		else
		{
			continue;
		}
		
		if (t == self)
		{
//				gi.Printf ("WARNING: Entity used itself.\n");
		}
		else if(t_count == pick)
		{
			if (t->e_UseFunc != useF_NULL)	// check can be omitted
			{
				GEntity_UseFunc(t, self, activator);
				return;
			}
		}

		if (!self->inuse)
		{
			gi.Printf("entity was removed while using targets\n");
			return;
		}
	}
}

void SP_target_random (gentity_t *self)
{
	self->e_UseFunc = useF_target_random_use;
}

int	numNewICARUSEnts = 0;
void scriptrunner_run (gentity_t *self)
{
	/*
	if (self->behaviorSet[BSET_USE])
	{	
		char	newname[MAX_FILENAME_LENGTH];

		sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] );

		ICARUS_RunScript( self, newname );
	}
	*/

	if ( self->count != -1 )
	{
		if ( self->count <= 0 )
		{
			self->e_UseFunc = useF_NULL;
			self->behaviorSet[BSET_USE] = NULL;
			return;
		}
		else
		{
			--self->count;
		}
	}

	if (self->behaviorSet[BSET_USE])
	{
		if ( self->spawnflags & 1 )
		{
			if ( !self->activator )
			{
				Q3_DebugPrint( WL_ERROR, "target_scriptrunner tried to run on invalid entity!\n");
				return;
			}

			if ( !self->activator->sequencer || !self->activator->taskManager )
			{//Need to be initialized through ICARUS
				if ( !self->activator->script_targetname || !self->activator->script_targetname[0] )
				{
					//We don't have a script_targetname, so create a new one
					self->activator->script_targetname = va( "newICARUSEnt%d", numNewICARUSEnts++ );
				}

				if ( ICARUS_ValidEnt( self->activator ) )
				{
					ICARUS_InitEnt( self->activator );
				}
				else
				{
					Q3_DebugPrint( WL_ERROR, "target_scriptrunner tried to run on invalid ICARUS activator!\n");
					return;
				}
			}

			Q3_DebugPrint( WL_VERBOSE, "target_scriptrunner running %s on activator %s\n", self->behaviorSet[BSET_USE], self->activator->targetname );
			ICARUS_RunScript( self->activator, va( "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] ) );
		}
		else
		{
			if ( self->activator )
			{
				Q3_DebugPrint( WL_VERBOSE, "target_scriptrunner %s used by %s\n", self->targetname, self->activator->targetname );
			}
			G_ActivateBehavior( self, BSET_USE );
		}
	}

	if ( self->wait )
	{
		self->nextthink = level.time + self->wait;
	}
}

void target_scriptrunner_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->nextthink > level.time )
	{
		return;
	}

	self->activator = activator;
	G_SetEnemy( self, other );
	if ( self->delay )
	{//delay before firing scriptrunner
		self->e_ThinkFunc = thinkF_scriptrunner_run;
		self->nextthink = level.time + self->delay;
	}
	else
	{
		scriptrunner_run (self);
	}
}

/*QUAKED target_scriptrunner (1 0 0) (-4 -4 -4) (4 4 4) runonactivator x x x x x x INACTIVE
--- SPAWNFLAGS ---
runonactivator - Will run the script on the entity that used this or tripped the trigger that used this
INACTIVE - start off

----- KEYS ------
Usescript - Script to run when used
count - how many times to run, -1 = infinite.  Default is once
wait - can't be used again in this amount of seconds (Default is 1 second if it's multiple-use)
delay - how long to wait after use to run script

*/
void SP_target_scriptrunner( gentity_t *self )
{
	if ( self->spawnflags & 128 )
	{
		self->svFlags |= SVF_INACTIVE;
	}

	if ( !self->count )
	{
		self->count = 1;//default 1 use only
	}
	/*
	else if ( !self->wait )
	{
		self->wait = 1;//default wait of 1 sec
	}
	*/
	// FIXME: this is a hack... because delay is read in as an int, so I'm bypassing that because it's too late in the project to change it and I want to be able to set less than a second delays
	// no one should be setting a radius on a scriptrunner, if they are this would be bad, take this out for the next project
	self->radius = 0.0f;
	G_SpawnFloat( "delay", "0", &self->radius );
	self->delay = self->radius * 1000;//sec to ms
	self->wait *= 1000;//sec to ms

	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_scriptrunner_use;
}

void target_gravity_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	if ( self->spawnflags & 1 )
	{
		gi.cvar_set("g_gravity", va("%f", self->speed));
	}
	else if ( activator->client )
	{
		int	grav = floor(self->speed);
		/*
		if ( activator->client->ps.gravity != grav )
		{
			gi.Printf("%s gravity changed to %d\n", activator->targetname, grav );
		}
		*/
		activator->client->ps.gravity = grav;
		activator->svFlags |= SVF_CUSTOM_GRAVITY;
		//FIXME: need a way to set this back to normal?
	}
}

/*QUAKED target_gravity_change (1 0 0) (-4 -4 -4) (4 4 4) GLOBAL

"gravity" - Normal = 800, Valid range: any

GLOBAL - Apply to entire world, not just the activator
*/
void SP_target_gravity_change( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	G_SpawnFloat( "gravity", "0", &self->speed );
	self->e_UseFunc = useF_target_gravity_change_use;
}

void target_friction_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{//FIXME - make a global?
		//gi.Cvar_Set("g_friction", va("%d", self->health));
	}
	else if(activator->client)
	{
		activator->client->ps.friction = self->health;
	}
}

/*QUAKED target_friction_change (1 0 0) (-4 -4 -4) (4 4 4)

"friction" Normal = 6, Valid range 0 - 10

*/
void SP_target_friction_change( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_friction_change_use;
}


extern void G_ChangeMap (const char *mapname, const char *spawntarget, qboolean hub);	//g_utils
void target_level_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	if( self->message && !Q_stricmp( "disconnect", self->message ) )
	{
		gi.SendConsoleCommand( "disconnect\n");
	}
	else
	{
		G_ChangeMap( self->message, self->target, (self->spawnflags&1) );
	}
	if (self->spawnflags&2)	//HIDEINFO
	{
		gi.cvar_set("cg_missionstatusscreen", "0");
	}
	else
	{
		gi.cvar_set("cg_missionstatusscreen", "1");	//turn it on!
	}
}

/*QUAKED target_level_change (1 0 0) (-4 -4 -4) (4 4 4) HUB HIDEINFO
HUB - Will save the current map's status and load the next map with any saved status it may have

"mapname" - Name of map to change to
"target" - Name of spawnpoint to start at in the new map
*/
void SP_target_level_change( gentity_t *self )
{
	if ( !self->message )
	{
		G_Error( "target_level_change with no mapname!");
		return;
	}
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_level_change_use;
}

/*QUAKED target_change_parm (1 0 0) (-4 -4 -4) (4 4 4)
Copies any parms set on this ent to the entity that  fired the trigger/button/whatever that uses this
parm1
parm2
parm3
parm4
parm5
parm6
parm7
parm8
parm9
parm10
parm11
parm12
parm13
parm14
parm15
parm16
*/
void Q3_SetParm (int entID, int parmNum, const char *parmValue);
void target_change_parm_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( !activator || !self )
	{
		return;
	}

	//FIXME: call capyparms
	if ( self->parms )
	{
		for ( int parmNum = 0; parmNum < MAX_PARMS; parmNum++ )
		{
			if ( self->parms->parm[parmNum] && self->parms->parm[parmNum][0] )
			{
				Q3_SetParm( activator->s.number, parmNum, self->parms->parm[parmNum] );
			}
		}
	}
}

void SP_target_change_parm( gentity_t *self )
{
	if ( !self->parms )
	{//ERROR!
		return;
	}
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_change_parm_use;
}

void target_play_music_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	gi.SetConfigstring( CS_MUSIC, self->message );
}

/*QUAKED target_play_music (1 0 0) (-4 -4 -4) (4 4 4)
target_play_music
Plays the requested music files when this target is used.

"targetname"
"music"		music WAV or MP3 file ( music/introfile.mp3 [optional]  music/loopfile.mp3 )

If an intro file and loop file are specified, the intro plays first, then the looping
portion will start and loop indefinetly.  If no introfile is entered, only the loopfile
will play.
*/
void SP_target_play_music( gentity_t *self )
{
	char *s;
	G_SetOrigin( self, self->s.origin );
	if (!G_SpawnString( "music", "", &s )) {
		G_Error( "target_play_music without a music key at %s", vtos( self->s.origin ) );
	}
	self->message = G_NewString (s);
	self->e_UseFunc = useF_target_play_music_use;
extern	cvar_t	*com_buildScript;
	//Precache!
	if (com_buildScript->integer) {//copy this puppy over
		char buffer[MAX_QPATH];
		fileHandle_t	hFile;

		Q_strncpyz( buffer, s, sizeof(buffer) );
		COM_DefaultExtension( buffer, sizeof(buffer), ".mp3");
		
		gi.FS_FOpenFile(buffer, &hFile, FS_READ);
		if (hFile) {
			gi.FS_FCloseFile( hFile );
		}
	}
}

void target_autosave_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	//gi.SendServerCommand( 0, "cp @INGAME_CHECKPOINT" );
	CG_CenterPrint( "@INGAME_CHECKPOINT", SCREEN_HEIGHT * 0.25 );	//jump the network

//	if (self->spawnflags & 1)
		gi.SendConsoleCommand( "wait 2;save auto\n" );
}

/*QUAKED target_autosave (1 0 0) (-4 -4 -4) (4 4 4)
Auto save the game right now.  Might be a slight chug to capture the thumbnail.
Make sure it won't trigger during dialogue or cinematic or it will stutter!
*/
void SP_target_autosave( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_autosave_use;
}

void target_secret_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{	 
	//we'll assume that the activator is the player
	gclient_t* const client = &level.clients[0];
	client->sess.missionStats.secretsFound++;
	if ( activator )
	{
		G_Sound( activator, self->noise_index );
	}
	else
	{
		G_Sound( self, self->noise_index );
	}
	gi.SendServerCommand( 0, "cp @INGAME_SECRET_AREA" );
	assert(client->sess.missionStats.totalSecrets);
}

/*QUAKED target_secret (1 0 1) (-4 -4 -4) (4 4 4)
You found a Secret!
"count" - how many secrets on this level,
          if more than one on a level, be sure they all have the same count!
*/
void SP_target_secret( gentity_t *self ) 
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_secret_use;
	self->noise_index = G_SoundIndex("sound/interface/secret_area");
	if (self->count)
	{
		gi.cvar_set("newTotalSecrets", va("%i",self->count));
	}
}
