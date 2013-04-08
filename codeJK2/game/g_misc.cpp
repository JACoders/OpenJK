/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// g_misc.c

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"


#include "g_local.h"
#include "g_functions.h"
#include "g_nav.h"
#include "g_items.h"

extern gentity_t *G_FindDoorTrigger( gentity_t *door );
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
extern void SetMiscModelDefaults( gentity_t *ent, useFunc_t use_func, char *material, int solid_mask,int animFlag, 
									qboolean take_damage, qboolean damage_model);

#define MAX_AMMO_GIVE 4



/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.

q3map_onlyvertexlighting 1	=	brush only gets vertex lighting (reduces bsp size!)
*/


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self ) {
	//FIXME: store targetname and vector (origin) in a list for further reference... remove after 1st second of game?
	G_SetOrigin( self, self->s.origin );
	self->e_ThinkFunc = thinkF_G_FreeEntity;
	//Give other ents time to link
	self->nextthink = level.time + START_TIME_REMOVE_ENTS;
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ){
	//FIXME: store in ref_tag system?
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear noIncidence START_OFF
Non-displayed light.
"light" overrides the default 300 intensity. - affects size
a negative "light" will subtract the light's color
'Linear' checkbox gives linear falloff instead of inverse square
'noIncidence' checkbox makes lighting smoother
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"scale" multiplier for the light intensity - does not affect size (default 1)
		greater than 1 is brighter, between 0 and 1 is dimmer.
"color" sets the light's color
"targetname" to indicate a switchable light - NOTE that all lights with the same targetname will be grouped together and act as one light (ie: don't mix colors, styles or start_off flag)
"style" to specify a specify light style, even for switchable lights!
"style_off" light style to use when switched off (Only for switchable lights)

   1 FLICKER (first variety)
   2 SLOW STRONG PULSE
   3 CANDLE (first variety)
   4 FAST STROBE
   5 GENTLE PULSE 1
   6 FLICKER (second variety)
   7 CANDLE (second variety)
   8 CANDLE (third variety)
   9 SLOW STROBE (fourth variety)
   10 FLUORESCENT FLICKER
   11 SLOW PULSE NOT FADE TO BLACK
   12 FAST PULSE FOR JEREMY
   13 Test Blending
*/
static void misc_lightstyle_set ( gentity_t *ent)
{
	const int mLightStyle = ent->count;
	const int mLightSwitchStyle = ent->bounceCount;
	const int mLightOffStyle = ent->fly_sound_debounce_time;
	if (!ent->misc_dlight_active)
	{	//turn off
		if (mLightOffStyle)	//i have a light style i'd like to use when off
		{
			char lightstyle[32];
			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+0, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, lightstyle);

			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+1, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, lightstyle);

			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+2, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, lightstyle);
		}else
		{
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, "a");
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, "a");
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, "a");
		}
	} 
	else
	{	//Turn myself on now
		if (mLightSwitchStyle)	//i have a light style i'd like to use when on
		{
			char lightstyle[32];
			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+0, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, lightstyle);

			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+1, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, lightstyle);

			gi.GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+2, lightstyle, 32);
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, lightstyle);
		}
		else
		{
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, "z");
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, "z");
			gi.SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, "z");
		}
	}
}
void SP_light( gentity_t *self ) {
	if (!self->targetname )
	{//if i don't have a light style switch, the i go away
		G_FreeEntity( self );
		return;
	}

	G_SpawnInt( "style", "0", &self->count );
	G_SpawnInt( "switch_style", "0", &self->bounceCount );
	G_SpawnInt( "style_off", "0", &self->fly_sound_debounce_time );
	G_SetOrigin( self, self->s.origin );
	gi.linkentity( self );

	self->e_UseFunc = useF_misc_dlight_use;
	self->e_clThinkFunc = clThinkF_NULL;

	self->s.eType = ET_GENERAL;
	self->misc_dlight_active = qfalse;
	self->svFlags |= SVF_NOCLIENT;

	if ( !(self->spawnflags & 4) )
	{	//turn myself on now
		self->misc_dlight_active = qtrue;
	}
	misc_lightstyle_set (self);
}

void misc_dlight_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior(ent,BSET_USE);

	ent->misc_dlight_active = !ent->misc_dlight_active;	//toggle
	misc_lightstyle_set (ent);
}


/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) 
{
	if ( player->NPC && ( player->NPC->aiFlags&NPCAI_FORM_TELE_NAV ) )
	{
		//My leader teleported, I was trying to catch up, take this off
		player->NPC->aiFlags &= ~NPCAI_FORM_TELE_NAV;
		
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	gi.unlinkentity (player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;
	VectorCopy ( player->client->ps.origin, player->currentOrigin );

	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 0, player->client->ps.velocity );
	//player->client->ps.pm_time = 160;		// hold time
	//player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	G_KillBox (player);

	// save results of pmove
	PlayerStateToEntityState( &player->client->ps, &player->s );

	gi.linkentity (player);
}

void TeleportMover( gentity_t *mover, vec3_t origin, vec3_t diffAngles, qboolean snapAngle ) 
{//FIXME: need an effect
	vec3_t		oldAngle, newAngle;
	float		speed;

	// unlink to make sure it can't possibly interfere with G_KillBox
	gi.unlinkentity (mover);

	//reposition it
	VectorCopy( origin, mover->s.pos.trBase );
	VectorCopy( origin, mover->currentOrigin );

	//Maintain their previous speed, but adjusted for new direction
	if ( snapAngle )
	{//not a diffAngle, actually an absolute angle
		vec3_t	dir;

		VectorCopy( diffAngles, newAngle );
		AngleVectors( newAngle, dir, NULL, NULL );
		VectorNormalize( dir );//necessary?
		speed = VectorLength( mover->s.pos.trDelta );
		VectorScale( dir, speed, mover->s.pos.trDelta );
		mover->s.pos.trTime = level.time;

		VectorSubtract( newAngle, mover->s.apos.trBase, diffAngles );
		VectorCopy( newAngle, mover->s.apos.trBase );
	}
	else
	{
		speed = VectorNormalize( mover->s.pos.trDelta );

		vectoangles( mover->s.pos.trDelta, oldAngle );
		VectorAdd( oldAngle, diffAngles, newAngle );

		AngleVectors( newAngle, mover->s.pos.trDelta, NULL, NULL );
		VectorNormalize( mover->s.pos.trDelta );

		VectorScale( mover->s.pos.trDelta, speed, mover->s.pos.trDelta );
		mover->s.pos.trTime = level.time;

		//Maintain their previous angles, but adjusted to new orientation
		VectorAdd( mover->s.apos.trBase, diffAngles, mover->s.apos.trBase );
	}

	//Maintain their previous anglespeed, but adjusted to new orientation
	speed = VectorNormalize( mover->s.apos.trDelta );
	VectorAdd( mover->s.apos.trDelta, diffAngles, mover->s.apos.trDelta );
	VectorNormalize( mover->s.apos.trDelta );
	VectorScale( mover->s.apos.trDelta, speed, mover->s.apos.trDelta );

	mover->s.apos.trTime = level.time;
	
	//Tell them it was teleported this move
	mover->s.eFlags |= EF_TELEPORT_BIT;

	// kill anything at the destination
	//G_KillBox (mover);
	//FIXME: call touch func instead of killbox?

	gi.linkentity (mover);
}

void teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace)
{
	gentity_t		*dest;

	if (!other->client)
		return;
	dest = 	G_PickTarget( self->target );
	if (!dest) {
		gi.Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( other, dest->s.origin, dest->s.angles );
}

/*QUAK-D misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter (gentity_t *ent)
{
	gentity_t		*trig;

	if (!ent->target)
	{
		gi.Printf ("teleporter without a target.\n");
		G_FreeEntity( ent );
		return;
	}

	ent->s.modelindex = G_ModelIndex( "models/objects/dmspot.md3" );
	ent->s.clientNum = 1;
//	ent->s.loopSound = G_SoundIndex("sound/world/amb10.wav");
	ent->contents = CONTENTS_SOLID;

	G_SetOrigin( ent, ent->s.origin );

	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, -16);
	gi.linkentity (ent);

	trig = G_Spawn ();
	trig->e_TouchFunc = touchF_teleporter_touch;
	trig->contents = CONTENTS_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	G_SetOrigin( trig, ent->s.origin );
	VectorSet (trig->mins, -8, -8, 8);
	VectorSet (trig->maxs, 8, 8, 24);
	gi.linkentity (trig);
	
}

/*QUAK-D misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16) - - NODRAW
Point teleporters at these.
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
	if ( ent->spawnflags & 4 ){
		return;
	}

	G_SetOrigin( ent, ent->s.origin );

	gi.linkentity (ent);
}


//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 or .ase file to display
turns into map triangles - not solid
*/
void SP_misc_model( gentity_t *ent ) {
	G_FreeEntity( ent );
}

//===========================================================

void setCamera ( gentity_t *ent )
{
	vec3_t		dir;
	gentity_t	*target = 0;

	// frame holds the rotate speed
	if ( ent->owner->spawnflags & 1 ) 
	{
		ent->s.frame = 25;
	} 
	else if ( ent->owner->spawnflags & 2 ) 
	{
		ent->s.frame = 75;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = ent->owner->s.clientNum;

	VectorCopy( ent->owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	if (ent->owner->target) {
		target = G_PickTarget( ent->owner->target );
	}
	if ( target ) 
	{
		VectorSubtract( target->s.origin, ent->owner->s.origin, dir );
		VectorNormalize( dir );
	} 
	else 
	{
		G_SetMovedir( ent->owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

void cycleCamera( gentity_t *self )
{
	self->owner = G_Find( self->owner, FOFS(targetname), self->target );
	if  ( self->owner == NULL )
	{
		//Uh oh! Not targeted at any ents!  Or reached end of list?  Which is it?
		//for now assume reached end of list and are cycling
		self->owner = G_Find( self->owner, FOFS(targetname), self->target );
		if  ( self->owner == NULL )
		{//still didn't find one
			gi.Printf( "Couldn't find target for misc_portal_surface\n" );
			G_FreeEntity( self );
			return;
		}
	}
	setCamera( self );

	if ( self->e_ThinkFunc == thinkF_cycleCamera )
	{
		if ( self->owner->wait > 0 )
		{
			self->nextthink = level.time + self->owner->wait;
		}
		else
		{
			self->nextthink = level.time + self->wait;
		}
	}
}

void misc_portal_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	cycleCamera( self );
}

void locateCamera( gentity_t *ent ) 
{//FIXME: make this fadeout with distance from misc_camera_portal

	ent->owner = G_Find(NULL, FOFS(targetname), ent->target);
	if ( !ent->owner ) 
	{
		gi.Printf( "Couldn't find target for misc_portal_surface\n" );
		G_FreeEntity( ent );
		return;
	}

	setCamera( ent );

	if ( !ent->targetname )
	{//not targetted, so auto-cycle
		if ( G_Find(ent->owner, FOFS(targetname), ent->target) != NULL  )
		{//targeted at more than one thing
			ent->e_ThinkFunc = thinkF_cycleCamera;
			if ( ent->owner->wait > 0 )
			{
				ent->nextthink = level.time + ent->owner->wait;
			}
			else
			{
				ent->nextthink = level.time + ent->wait;
			}
		}
	}
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!

targetname - When used, cycles to the next misc_portal_camera it's targeted
wait - makes it auto-cycle between all cameras it's pointed at at intevervals of specified number of seconds.

  cameras will be cycled through in the order they were created on the map.
*/
void SP_misc_portal_surface(gentity_t *ent) 
{
	VectorClear( ent->mins );
	VectorClear( ent->maxs );
	gi.linkentity (ent);

	ent->svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;
	ent->wait *= 1000;

	if ( !ent->target ) 
	{//mirror?
		VectorCopy( ent->s.origin, ent->s.origin2 );
	} 
	else 
	{
		ent->e_ThinkFunc = thinkF_locateCamera;
		ent->nextthink = level.time + 100;

		if ( ent->targetname )
		{
			ent->e_UseFunc = useF_misc_portal_use;
		}
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate
The target for a misc_portal_surface.  You can set either angles or target another entity (NOT an info_null) to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera(gentity_t *ent) {
	float	roll;

	VectorClear( ent->mins );
	VectorClear( ent->maxs );
	gi.linkentity (ent);

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll/360.0 * 256;
	ent->wait *= 1000;
}

extern qboolean G_ClearViewEntity( gentity_t *ent );
extern void G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
extern void SP_fx_runner( gentity_t *ent );
void camera_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	if ( player && player->client && player->client->ps.viewEntity == self->s.number )
	{
		G_UseTargets2( self, player, self->target4 );
		G_ClearViewEntity( player );
		G_Sound( player, self->soundPos2 );
	}
	G_UseTargets2( self, player, self->closetarget );
	//FIXME: explosion fx/sound
	//leave sparks at origin- where base's pole is still at?
	gentity_t *sparks = G_Spawn();
	if ( sparks )
	{
		sparks->fxFile = "spark";
		sparks->delay = 100;
		sparks->random = 500;
		sparks->s.angles[0] = 180;//point down
		VectorCopy( self->s.origin, sparks->s.origin );
		SP_fx_runner( sparks );
	}

	//bye!
	self->takedamage = qfalse;
	self->contents = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelindex = 0;
}

void camera_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator || !activator->client || activator->s.number )
	{//really only usable by the player
		return;
	}
	self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms

	// FIXME: I guess we are allowing them to switch to a dead camera.  Maybe we should conditionally do this though?
	if ( /*self->health <= 0 ||*/ (player && player->client && player->client->ps.viewEntity == self->s.number) )
	{//I'm already viewEntity, or I'm destroyed, find next
		gentity_t *next = NULL;
		if ( self->target2 != NULL )
		{
			next = G_Find( NULL, FOFS(targetname), self->target2 );
		}
		if ( next )
		{//found another one
			if ( !Q_stricmp( "misc_camera", next->classname ) )
			{//make sure it's another camera
				camera_use( next, other, activator );
			}
		}
		else //if ( self->health > 0 )
		{//I was the last (only?) one, clear out the viewentity
			G_UseTargets2( self, activator, self->target4 );
			G_ClearViewEntity( activator );
			G_Sound( activator, self->soundPos2 );
		}
	}
	else
	{//set me as view entity
		G_UseTargets2( self, activator, self->target3 );
		self->s.eFlags |= EF_NODRAW;
		self->s.modelindex = 0;
		G_SetViewEntity( activator, self );
		G_Sound( activator, self->soundPos1 );
	}
}

void camera_aim( gentity_t *self )
{
	self->nextthink = level.time + FRAMETIME;
	if ( player && player->client && player->client->ps.viewEntity == self->s.number )
	{//I am the viewEntity
		if ( (player->client->usercmd.buttons&BUTTON_BLOCKING) || player->client->usercmd.forwardmove || player->client->usercmd.rightmove || player->client->usercmd.upmove )
		{//player wants to back out of camera
			G_UseTargets2( self, player, self->target4 );
			G_ClearViewEntity( player );
			G_Sound( player, self->soundPos2 );
			self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms
			if ( player->client->usercmd.upmove > 0 )
			{//stop player from doing anything for a half second after
				player->aimDebounceTime = level.time + 500;
			}
		}
		else if ( self->painDebounceTime < level.time )
		{//check for use button
			if ( (player->client->usercmd.buttons&BUTTON_USE) )
			{//player pressed use button, wants to cycle to next
				camera_use( self, player, player );
			}
		}
		else
		{//don't draw me when being looked through
			self->s.eFlags |= EF_NODRAW;
			self->s.modelindex = 0;
		}
	}
	else if ( self->health > 0 )
	{//still alive, can draw me again
		self->s.eFlags &= ~EF_NODRAW;
		self->s.modelindex = self->s.modelindex3;
	}
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			vec3_t angles, dir;
			VectorSubtract( targ->currentOrigin, self->currentOrigin, dir );
			vectoangles( dir, angles );
			//FIXME: if a G2 model, do a bone override..???
			VectorCopy( self->currentAngles, self->s.apos.trBase );

			for( int i = 0; i < 3; i++ )
			{
				angles[i] = AngleNormalize180( angles[i] );
				self->s.apos.trDelta[i] = AngleNormalize180( (angles[i]-self->currentAngles[i])*10 );
			}
			//VectorSubtract( angles, self->currentAngles, self->s.apos.trDelta );
			//VectorScale( self->s.apos.trDelta, 10, self->s.apos.trDelta );
			self->s.apos.trTime = level.time;
			self->s.apos.trDuration = FRAMETIME;
			VectorCopy( angles, self->currentAngles );

			if ( DistanceSquared( self->currentAngles, self->lastAngles ) > 0.01f ) // if it moved at all, start a loop sound? not exactly the "bestest" solution
			{
				self->s.loopSound = G_SoundIndex( "sound/movers/objects/cameramove_lp2" );
			}
			else
			{
				self->s.loopSound = 0; // not moving so don't bother
			}

			VectorCopy( self->currentAngles, self->lastAngles );
			//G_SetAngles( self, angles );
		}
	}
}
/*QUAKED misc_camera (0 0 1) (-8 -8 -12) (8 8 16) VULNERABLE
A model in the world that can be used by the player to look through it's viewpoint

There will be a video overlay instead of the regular HUD and the FOV will be wider

VULNERABLE - allow camera to be destroyed

"target" - camera will remain pointed at this entity (if it's a train or some other moving object, it will keep following it)
"target2" - when player is in this camera and hits the use button, it will cycle to this next camera (if no target2, returns to normal view )
"target3" - thing to use when player enters this camera view
"target4" - thing to use when player leaves this camera view
"closetarget" - (sigh...) yet another target, fired this when it's destroyed
"wait"    - how long to wait between being used (default 0.5)
*/
void SP_misc_camera( gentity_t *self ) 
{
	G_SpawnFloat( "wait", "0.5", &self->wait );

	//FIXME: spawn base, too
	gentity_t *base = G_Spawn();
	if ( base )
	{
		base->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam_base.md3" );
		VectorCopy( self->s.origin, base->s.origin );
		base->s.origin[2] += 16;
		G_SetOrigin( base, base->s.origin );
		G_SetAngles( base, self->s.angles );
		gi.linkentity( base );
	}
	self->s.modelindex3 = self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam.md3" );
	self->soundPos1 = G_SoundIndex( "sound/movers/camera_on.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/camera_off.mp3" );
	G_SoundIndex( "sound/movers/objects/cameramove_lp2" );

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	self->s.apos.trType = TR_LINEAR_STOP;//TR_INTERPOLATE;//
	self->alt_fire = qtrue;
	VectorSet( self->mins, -8, -8, -12 );
	VectorSet( self->maxs, 8, 8, 0 );
	self->contents = CONTENTS_SOLID;
	gi.linkentity( self );

	self->fxID = G_EffectIndex( "spark" );

	if ( self->spawnflags & 1 ) // VULNERABLE
	{
		self->takedamage = qtrue;
	}

	self->health = 10;
	self->e_DieFunc = dieF_camera_die;
	
	self->e_UseFunc = useF_camera_use;

	self->e_ThinkFunc = thinkF_camera_aim;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}
/*
======================================================================

  SHOOTERS

======================================================================
*/

void Use_Shooter( gentity_t *ent, gentity_t *other, gentity_t *activator ) 
{
/*	vec3_t		dir;
	float		deg;
	vec3_t		up, right;
*/
	G_ActivateBehavior(ent,BSET_USE);
/*
	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->currentOrigin, ent->s.origin, dir );
		VectorNormalize( dir );
	} else {
		VectorCopy( ent->movedir, dir );
	}

	// randomize a bit
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, up, dir );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, right, dir );

	VectorNormalize( dir );

	switch ( ent->s.weapon ) 
	{
	case WP_GRENADE_LAUNCHER:
		fire_grenade( ent, ent->s.origin, dir );
		break;
	case WP_ROCKET_LAUNCHER:
		fire_rocket( ent, ent->s.origin, dir );
		break;
	case WP_PLASMAGUN:
		fire_plasma( ent, ent->s.origin, dir );
		break;
	}

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
*/
}

void InitShooter( gentity_t *ent, int weapon ) {
	ent->e_UseFunc = useF_Use_Shooter;
	ent->s.weapon = weapon;

	RegisterItem( FindItemForWeapon( (weapon_t) weapon ) );

	G_SetMovedir( ent->s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}
	ent->random = sin( M_PI * ent->random / 180 );
	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		G_SetEnemy(ent, G_PickTarget( ent->target ));
	}
	gi.linkentity( ent );
}

/*QUAK-ED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket( gentity_t *ent ) 
{
//	InitShooter( ent, WP_TETRION_DISRUPTOR );
}

/*QUAK-ED shooter_plasma (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_plasma( gentity_t *ent ) 
{
	InitShooter( ent, WP_BRYAR_PISTOL);
}

/*QUAK-ED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade( gentity_t *ent ) 
{
//	InitShooter( ent, WP_GRENADE_LAUNCHER);
}


/*QUAKED object_cargo_barrel1 (1 0 0) (-16 -16 -16) (16 16 29) SMALLER KLINGON NO_SMOKE POWDERKEG
Cargo Barrel
if given a targetname, using it makes it explode

SMALLER - (-8, -8, -16) (8, 8, 8)
KLINGON - klingon style barrel
NO_SMOKE - will not leave lingering smoke cloud when killed
POWDERKEG - wooden explosive barrel

health		 default = 20 
splashDamage default = 100
splashRadius default = 200
*/
void SP_object_cargo_barrel1(gentity_t *ent)
{
	if(ent->spawnflags & 8)
	{
		//FIXME: make an index into an external string table for localization
		ent->fullName = "Powderkeg Barrel";
		ent->s.modelindex = G_ModelIndex( "/models/mapobjects/cargo/barrel_wood2.md3" );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode3.wav");
	}
	else if(ent->spawnflags & 2)
	{
		//FIXME: make an index into an external string table for localization
		ent->fullName = "Klingon Cargo Barrel";
		ent->s.modelindex = G_ModelIndex( "/models/mapobjects/scavenger/k_barrel.md3" );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode4.wav");
	}
	else
	{
		//FIXME: make an index into an external string table for localization
		ent->fullName = "Federation Cargo Barrel";
		ent->s.modelindex = G_ModelIndex( va("/models/mapobjects/cargo/barrel%i.md3", Q_irand( 0, 2 )) );
//		ent->sounds = G_SoundIndex("sound/weapons/explosions/explode1.wav");
	}

	ent->contents = CONTENTS_SOLID|CONTENTS_OPAQUE;
	
	if ( ent->spawnflags & 1 )
	{
		VectorSet (ent->mins, -8, -8, -16);
		VectorSet (ent->maxs, 8, 8, 8);
	}
	else
	{
		VectorSet (ent->mins, -16, -16, -16);
		VectorSet (ent->maxs, 16, 16, 29);
	}

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	if(!ent->health)
		ent->health = 20;
	
	if(!ent->splashDamage)
		ent->splashDamage = 100;

	if(!ent->splashRadius)
		ent->splashRadius = 200;

	ent->takedamage = qtrue;

	ent->e_DieFunc = dieF_ExplodeDeath_Wait;
	
	if(ent->targetname)
		ent->e_UseFunc = useF_GoExplodeDeath;

	gi.linkentity (ent);
}


/*QUAKED misc_dlight (0.2 0.8 0.2) (-4 -4 -4) (4 4 4) STARTOFF FADEON FADEOFF PULSE
Dynamic light, toggles on and off when used
  
STARTOFF - Starts off
FADEON - Fades from 0 Radius to start Radius
FADEOFF - Fades from current Radius to 0 Radius before turning off
PULSE - This flag must be checked if you want it to fade/switch between start and final RGBA, otherwise it will just sit at startRGBA

ownername - Will display the light at the origin of the entity with this targetname

startRGBA - Red Green Blue Radius to start with - This MUST be set or your light won't do anything

These next values are used only if you want to fade/switch between 2 values (PULSE flag on)
finalRGBA - Red Green Blue Radius to end with
speed - how long to take to fade from start to final and final to start.  Also how long to fade on and off if appropriate flags are checked (seconds)
finaltime - how long to hold at final (seconds)
starttime - how long to hold at start (seconds)

TODO: Add random to speed/radius?
*/
void SP_misc_dlight(gentity_t *ent)
{
	G_SetOrigin( ent, ent->s.origin );
	gi.linkentity( ent );

	ent->speed *= 1000;
	ent->wait *= 1000;
	ent->radius *= 1000;

	//FIXME: attach self to a train or something?
	ent->e_UseFunc = useF_misc_dlight_use;
	
	ent->misc_dlight_active = qfalse;
	ent->e_clThinkFunc = clThinkF_NULL;

	ent->s.eType = ET_GENERAL;
	//Delay first think so we can find owner
	if ( ent->ownername )
	{
		ent->e_ThinkFunc = thinkF_misc_dlight_think;
		ent->nextthink = level.time + START_TIME_LINK_ENTS;
	}

	if ( !(ent->spawnflags & 1) )
	{//Turn myself on now
		GEntity_UseFunc( ent, ent, ent );
	}
}

void misc_dlight_use_old ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior(ent,BSET_USE);

	if ( ent->misc_dlight_active )
	{//We're on, turn off
		if ( ent->spawnflags & 4 )
		{//fade off
			ent->pushDebounceTime = 3;
		}
		else
		{
			ent->misc_dlight_active = qfalse;
			ent->e_clThinkFunc = clThinkF_NULL;

			ent->s.eType = ET_GENERAL;
			ent->svFlags &= ~SVF_BROADCAST;
		}
	}
	else
	{
		//Start at start regardless of when we were turned off
		if ( ent->spawnflags & 4 )
		{//fade on
			ent->pushDebounceTime = 2;
		}
		else
		{//Just start on
			ent->pushDebounceTime = 0;
		}
		ent->painDebounceTime = level.time;

		ent->misc_dlight_active = qtrue;

		ent->e_ThinkFunc = thinkF_misc_dlight_think;
		ent->nextthink = level.time + FRAMETIME;

		ent->e_clThinkFunc = clThinkF_CG_DLightThink;

		ent->s.eType = ET_THINKER;
		ent->svFlags |= SVF_BROADCAST;// Broadcast to all clients
	}
}

void misc_dlight_think ( gentity_t *ent )
{
	//Stay Attached to owner
	if ( ent->owner )
	{
		G_SetOrigin( ent, ent->owner->currentOrigin );
		gi.linkentity( ent );
	}
	else if ( ent->ownername )
	{
		ent->owner = G_Find( NULL, FOFS(targetname), ent->ownername );
		ent->ownername = NULL;
	}
	ent->nextthink = level.time + FRAMETIME;
}


void station_pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, vec3_t point, int damage, int mod,int hitLoc ) 
{
//	self->s.modelindex = G_ModelIndex("/models/mapobjects/stasis/plugin2_in.md3");
//	self->s.eFlags &= ~ EF_ANIM_ALLFAST;
//	self->s.eFlags |= EF_ANIM_ONCE;
//	gi.linkentity (self);
	self->s.modelindex = self->s.modelindex2;
	gi.linkentity (self);
}

// --------------------------------------------------------------------
//
//   HEALTH/ARMOR plugin functions
//
// --------------------------------------------------------------------

void health_use( gentity_t *self, gentity_t *other, gentity_t *activator);
int ITM_AddArmor (gentity_t *ent, int count);
int ITM_AddHealth (gentity_t *ent, int count);

void health_shutdown( gentity_t *self )
{
	if (!(self->s.eFlags & EF_ANIM_ONCE))
	{
		self->s.eFlags &= ~ EF_ANIM_ALLFAST;
		self->s.eFlags |= EF_ANIM_ONCE;

		// Switch to and animate its used up model.
		if (!Q_stricmp(self->model,"models/mapobjects/stasis/plugin2.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
		}
		else if (!Q_stricmp(self->model,"models/mapobjects/borg/plugin2.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
		}                                
		else if (!Q_stricmp(self->model,"models/mapobjects/stasis/plugin2_floor.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
//			G_Sound(self, G_SoundIndex("sound/ambience/stasis/shrinkage1.wav") );
		}
		else if (!Q_stricmp(self->model,"models/mapobjects/forge/panels.md3"))
		{
			self->s.modelindex = self->s.modelindex2;
		}

		gi.linkentity (self);
	}
}

void health_think( gentity_t *ent )
{
	int dif;

	// He's dead, Jim. Don't give him health
	if (ent->enemy->health<1)
	{
		ent->count = 0;
		ent->e_ThinkFunc = thinkF_NULL;
	}

	// Still has power to give
	if (ent->count > 0)
	{
		// For every 3 points of health, you get 1 point of armor
		// BUT!!! after health is filled up, you get the full energy going to armor

		dif = ent->enemy->client->ps.stats[STAT_MAX_HEALTH] - ent->enemy->health;

		if (dif > 3 )
		{
			dif= 3;
		}
		else if (dif < 0) 
		{
			dif= 0;	
		}

		if (dif > ent->count)	// Can't give more than count
		{
			dif = ent->count;
		}

		if ((ITM_AddHealth (ent->enemy,dif)) && (dif>0))		
		{
			ITM_AddArmor (ent->enemy,1);	// 1 armor for every 3 health

			ent->count-=dif;
			ent->nextthink = level.time + 10;
		}
		else	// User has taken all health he can hold, see about giving it all to armor
		{
			dif = ent->enemy->client->ps.stats[STAT_MAX_HEALTH] - 
				ent->enemy->client->ps.stats[STAT_ARMOR];

			if (dif > 3)
			{
				dif = 3;
			}
			else if (dif < 0) 
			{
				dif= 0;	
			}

			if (ent->count < dif)	// Can't give more than count
			{
				dif = ent->count;
			}

			if ((!ITM_AddArmor(ent->enemy,dif)) || (dif<=0))
			{
				ent->e_UseFunc = useF_health_use;	
				ent->e_ThinkFunc = thinkF_NULL;
			}
			else
			{
				ent->count-=dif;
				ent->nextthink = level.time + 10;
			}
		}
	}

	if (ent->count < 1)
	{
		health_shutdown(ent);
	}
}

void misc_model_useup( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	self->s.eFlags &= ~ EF_ANIM_ALLFAST;
	self->s.eFlags |= EF_ANIM_ONCE;

	// Switch to and animate its used up model.
	self->s.modelindex = self->s.modelindex2;

	gi.linkentity (self);

	// Use target when used
	if (self->spawnflags & 8)
	{
		G_UseTargets( self, activator );	
	}

	self->e_UseFunc = useF_NULL;	
	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;
}

void health_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{//FIXME: Heal entire team?  Or only those that are undying...?
	int dif;
	int dif2;
	int hold;

	G_ActivateBehavior(self,BSET_USE);

	if (self->e_ThinkFunc != thinkF_NULL)
	{
		self->e_ThinkFunc = thinkF_NULL;
	}
	else
	{

		if (other->client)
		{
			// He's dead, Jim. Don't give him health
			if (other->client->ps.stats[STAT_HEALTH]<1)
			{
				dif = 1;
				self->count = 0;
			}
			else 
			{	// Health
				dif = other->client->ps.stats[STAT_MAX_HEALTH] - other->client->ps.stats[STAT_HEALTH];
				// Armor
				dif2 = other->client->ps.stats[STAT_MAX_HEALTH] - other->client->ps.stats[STAT_ARMOR];
				hold = (dif2 - dif);
				// For every 3 points of health, you get 1 point of armor
				// BUT!!! after health is filled up, you get the full energy going to armor
				if (hold>0)	// Need more armor than health
				{
					// Calculate total amount of station energy needed.
					
					hold = dif / 3;	//	For every 3 points of health, you get 1 point of armor
					dif2 -= hold;
					dif2 += dif;	
					
					dif = dif2;
				}
			}
		}
		else
		{	// Being triggered to be used up
			dif = 1;
			self->count = 0;
		}
		
		// Does player already have full health and full armor?
		if (dif > 0)
		{
//			G_Sound(self, G_SoundIndex("sound/player/suithealth.wav") );

			if ((dif >= self->count) || (self->count<1)) // use it all up?
			{
				health_shutdown(self);
			}
			// Use target when used
			if (self->spawnflags & 8)
			{
				G_UseTargets( self, activator );	
			}

			self->e_UseFunc = useF_NULL;	
			self->enemy = other;
			self->e_ThinkFunc = thinkF_health_think;
			self->nextthink = level.time + 50;
		}
		else
		{
//			G_Sound(self, G_SoundIndex("sound/weapons/noammo.wav") );
		}
	}	
}

// --------------------------------------------------------------------
//
//   AMMO plugin functions
//
// --------------------------------------------------------------------
void ammo_use( gentity_t *self, gentity_t *other, gentity_t *activator);
int Add_Ammo2 (gentity_t *ent, int ammoType, int count);

void ammo_shutdown( gentity_t *self )
{
	if (!(self->s.eFlags & EF_ANIM_ONCE))
	{
		self->s.eFlags &= ~ EF_ANIM_ALLFAST;
		self->s.eFlags |= EF_ANIM_ONCE;

		gi.linkentity (self);
	}
}
void ammo_think( gentity_t *ent )
{
	int dif;

	// Still has ammo to give
	if (ent->count > 0 && ent->enemy )
	{
		dif = ammoData[AMMO_BLASTER].max  - ent->enemy->client->ps.ammo[AMMO_BLASTER];

		if (dif > 2 )
		{
			dif= 2;
		}
		else if (dif < 0) 
		{
			dif= 0;	
		}

		if (ent->count < dif)	// Can't give more than count
		{
			dif = ent->count;
		}

		// Give player ammo 
		if (Add_Ammo2(ent->enemy,AMMO_BLASTER,dif) && (dif!=0))	
		{
			ent->count-=dif;
			ent->nextthink = level.time + 10;
		}
		else	// User has taken all ammo he can hold
		{
			ent->e_UseFunc = useF_ammo_use;	
			ent->e_ThinkFunc = thinkF_NULL;
		}
	}

	if (ent->count < 1)
	{
		ammo_shutdown(ent);
	}
}

//------------------------------------------------------------
void ammo_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif;

	G_ActivateBehavior(self,BSET_USE);

	if (self->e_ThinkFunc != thinkF_NULL)
	{
		if (self->e_UseFunc != useF_NULL)
		{
			self->e_ThinkFunc = thinkF_NULL;
		}
	}
	else
	{
		if (other->client)
		{
			dif = ammoData[AMMO_BLASTER].max - other->client->ps.ammo[AMMO_BLASTER];
		}
		else
		{	// Being triggered to be used up
			dif = 1;
			self->count = 0;
		}

		// Does player already have full ammo?
		if (dif > 0)
		{
//			G_Sound(self, G_SoundIndex("sound/player/suitenergy.wav") );

			if ((dif >= self->count) || (self->count<1)) // use it all up?
			{
				ammo_shutdown(self);
			}
		}	
		else
		{
//			G_Sound(self, G_SoundIndex("sound/weapons/noammo.wav") );
		}
		// Use target when used
		if (self->spawnflags & 8)
		{
			G_UseTargets( self, activator );	
		}

		self->e_UseFunc = useF_NULL;	
		G_SetEnemy( self, other );
		self->e_ThinkFunc = thinkF_ammo_think;
		self->nextthink = level.time + 50;
	}	
}

//------------------------------------------------------------
void mega_ammo_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior( self, BSET_USE );

	// Use target when used
	G_UseTargets( self, activator );

	// first use, adjust the max ammo a person can hold for each type of ammo
	ammoData[AMMO_BLASTER].max	= 999;
	ammoData[AMMO_POWERCELL].max		= 999;

	// Set up our count with whatever the max difference will be
	if ( other->client->ps.ammo[AMMO_POWERCELL] > other->client->ps.ammo[AMMO_BLASTER] )
		self->count = ammoData[AMMO_BLASTER].max - other->client->ps.ammo[AMMO_BLASTER];
	else
		self->count = ammoData[AMMO_POWERCELL].max - other->client->ps.ammo[AMMO_POWERCELL];

//	G_Sound( self, G_SoundIndex("sound/player/superenergy.wav") );

	// Clear our usefunc, then think until our ammo is full
	self->e_UseFunc = useF_NULL;	
	G_SetEnemy( self, other );
	self->e_ThinkFunc = thinkF_mega_ammo_think;
	self->nextthink = level.time + 50;

	self->s.frame = 0;
	self->s.eFlags |= EF_ANIM_ONCE;
}

//------------------------------------------------------------
void mega_ammo_think( gentity_t *self )
{
	int	ammo_add = 5;

	// If the middle model is done animating, and we haven't switched to the last model yet...
	//		chuck up the last model.

	if (!Q_stricmp(self->model,"models/mapobjects/forge/power_up_boss.md3"))	// Because the normal forge_ammo model can use this too
	{
		if ( self->s.frame > 16 && self->s.modelindex != self->s.modelindex2 )
			self->s.modelindex = self->s.modelindex2;
	}
	
	if ( self->enemy && self->count > 0 )
	{
		// Add an equal ammount of ammo to each type
		self->enemy->client->ps.ammo[AMMO_BLASTER]		+= ammo_add;
		self->enemy->client->ps.ammo[AMMO_POWERCELL]	+= ammo_add;

		// Now cap to prevent overflows
		if ( self->enemy->client->ps.ammo[AMMO_BLASTER] > ammoData[AMMO_BLASTER].max )
			self->enemy->client->ps.ammo[AMMO_BLASTER] = ammoData[AMMO_BLASTER].max;

		if ( self->enemy->client->ps.ammo[AMMO_POWERCELL] > ammoData[AMMO_POWERCELL].max )
			self->enemy->client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;

		// Decrement the count given counter
		self->count -= ammo_add;

		// If we've given all we should, prevent giving any more, even if they player is no longer full
		if ( self->count <= 0 )
		{
			self->count = 0;
			self->e_ThinkFunc = thinkF_NULL;
			self->nextthink = -1;
		}
		else
			self->nextthink = 20;
	}
}


//------------------------------------------------------------
void switch_models( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// FIXME: need a sound here!!
	if ( self->s.modelindex2 )
		self->s.modelindex = self->s.modelindex2;
}

//------------------------------------------------------------
void touch_ammo_crystal_tigger( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if ( !other->client )
		return;

	// dead people can't pick things up
	if ( other->health < 1 )
		return;

	// Only player can pick it up
	if ( !other->s.number == 0 )
	{
		return;
	}

	if ( other->client->ps.ammo[ AMMO_POWERCELL ] >= ammoData[AMMO_POWERCELL].max )
	{
		return;		// can't hold any more
	}

	// Add the ammo
	other->client->ps.ammo[AMMO_POWERCELL] += self->owner->count;

	if ( other->client->ps.ammo[AMMO_POWERCELL] > ammoData[AMMO_POWERCELL].max ) 
	{
		other->client->ps.ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
	}

	// Trigger once only
	self->e_TouchFunc = touchF_NULL;

	// swap the model to the version without the crystal and ditch the infostring
	self->owner->s.modelindex = self->owner->s.modelindex2;

	// play the normal pickup sound
//	G_AddEvent( other, EV_ITEM_PICKUP, ITM_AMMO_CRYSTAL_BORG );

	// fire item targets
	G_UseTargets( self->owner, other );
}

//------------------------------------------------------------
void spawn_ammo_crystal_trigger( gentity_t *ent )
{
	gentity_t	*other;
	vec3_t		mins, maxs;

	// Set the base bounds
	VectorCopy( ent->s.origin, mins );
	VectorCopy( ent->s.origin, maxs );

	// Now add an area of influence around the thing
	for ( int i = 0; i < 3; i++ )
	{
		maxs[i] += 48;
		mins[i] -= 48;
	}

	// create a trigger with this size
	other = G_Spawn( );

	VectorCopy( mins, other->mins );
	VectorCopy( maxs, other->maxs );

	// set up the other bits that the engine needs to know
	other->owner = ent;
	other->contents = CONTENTS_TRIGGER;
	other->e_TouchFunc = touchF_touch_ammo_crystal_tigger;

	gi.linkentity( other );
}

void misc_replicator_item_remove ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->s.eFlags |= EF_NODRAW;
	//self->contents = 0;
	self->s.modelindex = 0;
	self->e_UseFunc = useF_misc_replicator_item_spawn;
	//FIXME: pickup sound?
	if ( activator->client )
	{
		activator->health += 5;
		if ( activator->health > activator->client->ps.stats[STAT_MAX_HEALTH] )	// Past max health
		{
			activator->health = activator->client->ps.stats[STAT_MAX_HEALTH];
		}
	}
}

void misc_replicator_item_finish_spawn( gentity_t *self )
{
	//self->contents = CONTENTS_ITEM;
	//FIXME: blinks out for a couple frames when transporter effect is done?
	self->e_UseFunc = useF_misc_replicator_item_remove;
}

void misc_replicator_item_spawn ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	switch ( Q_irand( 1, self->count ) )
	{
	case 1:
		self->s.modelindex = self->bounceCount;
		break;
	case 2:
		self->s.modelindex = self->fly_sound_debounce_time;
		break;
	case 3:
		self->s.modelindex = self->painDebounceTime;
		break;
	case 4:
		self->s.modelindex = self->disconnectDebounceTime;
		break;
	case 5:
		self->s.modelindex = self->attackDebounceTime;
		break;
	case 6://max
		self->s.modelindex = self->pushDebounceTime;
		break;
	}
	self->s.eFlags &= ~EF_NODRAW;
	self->e_ThinkFunc = thinkF_misc_replicator_item_finish_spawn;
	self->nextthink = level.time + 4000;//shorter?
	self->e_UseFunc = useF_NULL;

	gentity_t *tent = G_TempEntity( self->currentOrigin, EV_REPLICATOR );
	tent->owner = self;
}

/*QUAK-ED misc_replicator_item (0.2 0.8 0.2) (-4 -4 0) (4 4 8) 
When used. this will "spawn" an entity with a random model from the ones provided below...

Using it again removes the item as if it were picked up.

model  - first random model key
model2  - second random model key
model3  - third random model key
model4  - fourth random model key
model5  - fifth random model key
model6  - sixth random model key

NOTE: do not skip one of these model names, start with the lowest and fill in each next highest one with a value.  A gap will cause the item to not work correctly.

NOTE: if you use an invalid model, it will still try to use it and show the NULL axis model (or nothing at all)

targetname - how you refer to it for using it
*/
void SP_misc_replicator_item ( gentity_t *self )
{
	if ( self->model )
	{
		self->bounceCount = G_ModelIndex( self->model );
		self->count++;
		if ( self->model2 )
		{
			self->fly_sound_debounce_time = G_ModelIndex( self->model2 );
			self->count++;
			if ( self->target )
			{
				self->painDebounceTime = G_ModelIndex( self->target );
				self->count++;
				if ( self->target2 )
				{
					self->disconnectDebounceTime = G_ModelIndex( self->target2 );
					self->count++;
					if ( self->target3 )
					{
						self->attackDebounceTime = G_ModelIndex( self->target3 );
						self->count++;
						if ( self->target4 )
						{
							self->pushDebounceTime = G_ModelIndex( self->target4 );
							self->count++;
						}
					}
				}
			}
		}
	}
//	G_SoundIndex( "sound/movers/switches/replicator.wav" );
	self->e_UseFunc = useF_misc_replicator_item_spawn;

	self->s.eFlags |= EF_NODRAW;
	//self->contents = 0;

	VectorSet( self->mins, -4, -4, 0 );
	VectorSet( self->maxs, 4, 4, 8 );
	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );

	gi.linkentity( self );
}

/*QUAKED misc_trip_mine (0.2 0.8 0.2) (-4 -4 -4) (4 4 4) START_ON BROADCAST
Place in a map and point the angles at whatever surface you want it to attach to.

START_ON - If you give it a targetname to make it toggle-able, but want it to start on, set this flag
BROADCAST - ONLY USE THIS IF YOU HAVE TO!  causes the trip wire and loop sound to always happen, use this if the beam drops out in certain situations

The trip mine will attach to that surface and fire it's beam away from the surface at an angle perpendicular to it.

targetname - starts off, when used, turns on (toggles)

FIXME: sometimes we want these to not be shootable... maybe just put them behind a force field?
*/
extern void touchLaserTrap( gentity_t *ent, gentity_t *other, trace_t *trace );
extern void CreateLaserTrap( gentity_t *laserTrap, vec3_t start, gentity_t *owner );

void misc_trip_mine_activate( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->e_ThinkFunc == thinkF_laserTrapThink )
	{
		self->s.eFlags &= ~EF_FIRING;
		self->s.loopSound = 0;
		self->e_ThinkFunc = thinkF_NULL;
		self->nextthink = -1;
	}
	else
	{
		self->e_ThinkFunc = thinkF_laserTrapThink;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_trip_mine( gentity_t *self )
{
	vec3_t	forward, end;
	trace_t	trace;

	AngleVectors( self->s.angles, forward, NULL, NULL );
	VectorMA( self->s.origin, 128, forward, end );

	gi.trace( &trace, self->s.origin, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );

	if ( trace.allsolid || trace.startsolid )
	{
		Com_Error( ERR_DROP,"misc_trip_mine at %s in solid\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		return;
	}
	if ( trace.fraction == 1.0 )
	{
		Com_Error( ERR_DROP,"misc_trip_mine at %s pointed at no surface\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		return;
	}

	RegisterItem( FindItemForWeapon( WP_TRIP_MINE ));	//precache the weapon

	self->count = 2/*TRIPWIRE_STYLE*/;

	vectoangles( trace.plane.normal, end );
	G_SetOrigin( self, trace.endpos );
	G_SetAngles( self, end );

	CreateLaserTrap( self, trace.endpos, self );
	touchLaserTrap( self, self, &trace );
	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;

	if ( !self->targetname || (self->spawnflags&1) )
	{//starts on
		misc_trip_mine_activate( self, self, self );
	}
	if ( self->targetname )
	{
		self->e_UseFunc = useF_misc_trip_mine_activate;
	}

	if (( self->spawnflags & 2 )) // broadcast...should only be used in very rare cases.  could fix networking, perhaps, but james suggested this because it's easier
	{
		self->svFlags |= SVF_BROADCAST;
	}

	gi.linkentity( self );
}

/*QUAKED misc_maglock (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x x
Place facing a door (using the angle, not a targetname) and it will lock that door.  Can only be destroyed by lightsaber and will automatically unlock the door it's attached to

NOTE: place these half-way in the door to make it flush with the door's surface.

"target"	thing to use when destoryed (not doors - it automatically unlocks the door it was angled at)
"health"	default is 10
*/
void maglock_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
{
	//unlock our door if we're the last lock pointed at the door
	if ( self->activator )
	{
		self->activator->lockCount--;
		if ( !self->activator->lockCount )
		{
			self->activator->svFlags &= ~SVF_INACTIVE;
		}
	}
	
	//use targets
	G_UseTargets( self, attacker );
	//die
	WP_Explode( self );
}
void SP_misc_maglock ( gentity_t *self )
{
	//NOTE: May have to make these only work on doors that are either untargeted 
	//		or are targeted by a trigger, not doors fired off by scripts, counters 
	//		or other such things?
	self->s.modelindex = G_ModelIndex( "models/map_objects/imp_detention/door_lock.md3" );
	self->fxID = G_EffectIndex( "maglock/explosion" );

	G_SetOrigin( self, self->s.origin );

	self->e_ThinkFunc = thinkF_maglock_link;
	//FIXME: for some reason, when you re-load a level, these fail to find their doors...?  Random?  Testing an additional 200ms after the START_TIME_FIND_LINKS
	self->nextthink = level.time + START_TIME_FIND_LINKS+200;//START_TIME_FIND_LINKS;//because we need to let the doors link up and spawn their triggers first!
}
void maglock_link( gentity_t *self )
{
	//find what we're supposed to be attached to
	vec3_t	forward, start, end;
	trace_t	trace;

	AngleVectors( self->s.angles, forward, NULL, NULL );
	VectorMA( self->s.origin, 128, forward, end );
	VectorMA( self->s.origin, -4, forward, start );

	gi.trace( &trace, start, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );

	if ( trace.allsolid || trace.startsolid )
	{
		Com_Error( ERR_DROP,"misc_maglock at %s in solid\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		return;
	}
	if ( trace.fraction == 1.0 )
	{
		self->e_ThinkFunc = thinkF_maglock_link;
		self->nextthink = level.time + 100;
		/*
		Com_Error( ERR_DROP,"misc_maglock at %s pointed at no surface\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		*/
		return;
	}
	gentity_t *traceEnt = &g_entities[trace.entityNum];
	if ( trace.entityNum >= ENTITYNUM_WORLD || !traceEnt || Q_stricmp( "func_door", traceEnt->classname ) )
	{
		self->e_ThinkFunc = thinkF_maglock_link;
		self->nextthink = level.time + 100;
		//Com_Error( ERR_DROP,"misc_maglock at %s not pointed at a door\n", vtos(self->s.origin) );
		//G_FreeEntity( self );
		return;
	}

	//check the traceEnt, make sure it's a door and give it a lockCount and deactivate it
	//find the trigger for the door
	self->activator = G_FindDoorTrigger( traceEnt );
	if ( !self->activator )
	{
		self->activator = traceEnt;
	}
	self->activator->lockCount++;
	self->activator->svFlags |= SVF_INACTIVE;

	//now position and orient it
	vectoangles( trace.plane.normal, end );
	G_SetOrigin( self, trace.endpos );
	G_SetAngles( self, end );

	//make it hittable
	//FIXME: if rotated/inclined this bbox may be off... but okay if we're a ghoul model?
	//self->s.modelindex = G_ModelIndex( "models/map_objects/imp_detention/door_lock.md3" );
	VectorSet( self->mins, -8, -8, -8 );
	VectorSet( self->maxs, 8, 8, 8 );
	self->contents = CONTENTS_CORPSE;

	//make it destroyable
	self->flags |= FL_SHIELDED;//only damagable by lightsabers
	self->takedamage = qtrue;
	self->health = 10;
	self->e_DieFunc = dieF_maglock_die;
	//self->fxID = G_EffectIndex( "maglock/explosion" );

	gi.linkentity( self );
}

/*
================
EnergyShieldStationSettings
================
*/
void EnergyShieldStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 100; 
			break;
		case 1:	//	MEDIUM
			ent->count = 75; 
			break;
		default :
		case 2:	//	HARD
			ent->count = 50; 
			break;
		}
	}
}

/*
================
shield_power_converter_use
================
*/
void shield_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif,add;

	if ( !activator || activator->s.number != 0 )
	{
		//only the player gets to use these
		return;
	}

	G_ActivateBehavior( self,BSET_USE );

	if ( self->setTime < level.time )
	{
		self->setTime = level.time + 100;

		dif = 100 - activator->client->ps.stats[STAT_ARMOR]; // FIXME: define for max armor?

		if ( dif > 0 && self->count )	// Already at full armor?..and do I even have anything to give
		{
			if ( dif > MAX_AMMO_GIVE )
			{
				add = MAX_AMMO_GIVE;
			}
			else
			{
				add = dif;
			}

			if ( self->count < add )
			{
				add = self->count;
			}

			self->count -= add;

			activator->client->ps.stats[STAT_ARMOR] += add;

			self->s.loopSound = G_SoundIndex( "sound/interface/shieldcon_run.wav" );
		}
	
		if ( self->count <= 0 )
		{
			// play empty sound
			self->setTime = level.time + 1000; // extra debounce so that the sounds don't overlap too much
			G_Sound( self, G_SoundIndex( "sound/interface/shieldcon_empty.mp3" ));
			self->s.loopSound = 0;

			if ( self->s.eFlags & EF_SHADER_ANIM )
			{
	 			self->s.frame = 1;
			}
		}
		else if ( activator->client->ps.stats[STAT_ARMOR] >= 100 ) // FIXME: define for max
		{
			// play full sound
			G_Sound( self, G_SoundIndex( "sound/interface/shieldcon_done.mp3" ));
			self->setTime = level.time + 1000; // extra debounce so that the sounds don't overlap too much
			self->s.loopSound = 0;
		}
	}

	if ( self->s.loopSound )
	{
		// we will have to shut of the loop sound, so I guess try and do it intelligently...NOTE: this could get completely stomped every time through the loop
		//	this is fine, since it just controls shutting off the sound when there are situations that could start the sound but not shut it off
		self->e_ThinkFunc = thinkF_poll_converter;
		self->nextthink = level.time + 500;
	}
	else
	{
		// sound is already off, so we don't need to "think" about it.
		self->e_ThinkFunc = thinkF_NULL;
		self->nextthink = 0;
	}
	
	if ( activator->client->ps.stats[STAT_ARMOR] > 0 )
	{
		activator->client->ps.powerups[PW_BATTLESUIT] = Q3_INFINITE;
	}
}


/*QUAKED misc_model_shield_power_converter (1 0 0) (-16 -16 -16) (16 16 16) x x x USETARGET
#MODELNAME="models/items/psd_big.md3"
Gives shield energy when used.

USETARGET - when used it fires off target

"health" - how much health the model has - default 60 (zero makes non-breakable)
"targetname" - dies and displays damagemodel when used, if any (if not, removes itself)
"target" - what to use when it dies
"paintarget" - target to fire when hit (but not destroyed)
"count" - the amount of ammo given when used (default 100)
*/
//------------------------------------------------------------
void SP_misc_model_shield_power_converter( gentity_t *ent )
{
	SetMiscModelDefaults( ent, useF_shield_power_converter_use, "4", CONTENTS_SOLID, NULL, qfalse, NULL );

	ent->takedamage = qfalse;

	EnergyShieldStationSettings(ent);

	G_SoundIndex("sound/interface/shieldcon_run.wav");
	G_SoundIndex("sound/interface/shieldcon_done.mp3");
	G_SoundIndex("sound/interface/shieldcon_empty.mp3");

	ent->s.modelindex2 = G_ModelIndex("/models/items/psd_big.md3");	// Precache model
}

/*QUAKED misc_shield_floor_unit (1 0 0) (-16 -16 0) (16 16 32) x x x USETARGET
#MODELNAME="/models/items/a_shield_converter.md3"
Gives shield energy when used.

USETARGET - when used it fires off target

"health" - how much health the model has - default 60 (zero makes non-breakable)
"targetname" - dies and displays damagemodel when used, if any (if not, removes itself)
"target" - what to use when it dies
"paintarget" - target to fire when hit (but not destroyed)
"count" - the amount of ammo given when used (default 100)
*/
//------------------------------------------------------------
void SP_misc_shield_floor_unit( gentity_t *ent )
{
	VectorSet( ent->mins, -16, -16, 0 );
	VectorSet( ent->maxs, 16, 16, 32 );

	SetMiscModelDefaults( ent, useF_shield_power_converter_use, "4", CONTENTS_SOLID, NULL, qfalse, NULL );

	ent->takedamage = qfalse;

	EnergyShieldStationSettings(ent);

	G_SoundIndex("sound/interface/shieldcon_run.wav");
	G_SoundIndex("sound/interface/shieldcon_done.mp3");
	G_SoundIndex("sound/interface/shieldcon_empty.mp3");

	ent->s.modelindex = G_ModelIndex( "models/items/a_shield_converter.md3" );	// Precache model
	ent->s.eFlags |= EF_SHADER_ANIM;
}


/*
================
EnergyAmmoShieldStationSettings
================
*/
void EnergyAmmoStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "0", &ent->count );

	if (!ent->count)
	{
		switch (g_spskill->integer)
		{
		case 0:	//	EASY
			ent->count = 100; 
			break;
		case 1:	//	MEDIUM
			ent->count = 75; 
			break;
		default :
		case 2:	//	HARD
			ent->count = 50; 
			break;
		}
	}
}

// There has to be an easier way to turn off the looping sound...but
//	it's the night before beta and my brain is fried
//------------------------------------------------------------------
void poll_converter( gentity_t *self )
{
	self->s.loopSound = 0;
	self->nextthink = 0;
	self->e_ThinkFunc = thinkF_NULL;
}

/*
================
ammo_power_converter_use
================
*/
void ammo_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			add;
	int			difBlaster,difPowerCell,difMetalBolts;
	playerState_t *ps;

	if ( !activator || activator->s.number != 0 )
	{//only the player gets to use these
		return;
	}

	G_ActivateBehavior( self, BSET_USE );

	ps = &activator->client->ps;

	if ( self->setTime < level.time )
	{
		difBlaster		= ammoData[AMMO_BLASTER].max - ps->ammo[AMMO_BLASTER];
		difPowerCell	= ammoData[AMMO_POWERCELL].max - ps->ammo[AMMO_POWERCELL];
		difMetalBolts	= ammoData[AMMO_METAL_BOLTS].max - ps->ammo[AMMO_METAL_BOLTS];

		// Has it got any power left...and can we even use any of it?
		if ( self->count && ( difBlaster > 0 || difPowerCell > 0 || difMetalBolts > 0 ))	
		{
			// at least one of the ammo types could stand to take on a bit more ammo
			self->setTime = level.time + 100;
			self->s.loopSound = G_SoundIndex( "sound/interface/ammocon_run.wav" );

			// dole out ammo in little packets
			if ( self->count > MAX_AMMO_GIVE )
			{
				add = MAX_AMMO_GIVE;
			}
			else if ( self->count < 0 )
			{
				add = 0;
			}
			else
			{
				add = self->count;
			}

			// all weapons fill at same rate...
			ps->ammo[AMMO_BLASTER]		+= add;
			ps->ammo[AMMO_POWERCELL]	+= add;
			ps->ammo[AMMO_METAL_BOLTS]	+= add;

			// ...then get clamped to max
			if ( ps->ammo[AMMO_BLASTER] > ammoData[AMMO_BLASTER].max )
			{
				ps->ammo[AMMO_BLASTER] = ammoData[AMMO_BLASTER].max;
			}

			if ( ps->ammo[AMMO_POWERCELL] > ammoData[AMMO_POWERCELL].max )
			{
				ps->ammo[AMMO_POWERCELL] = ammoData[AMMO_POWERCELL].max;
			}

			if ( ps->ammo[AMMO_METAL_BOLTS] > ammoData[AMMO_METAL_BOLTS].max )
			{
				ps->ammo[AMMO_METAL_BOLTS] = ammoData[AMMO_METAL_BOLTS].max;
			}

			self->count -= add;
		}

		if ( self->count <= 0 )
		{
			// play empty sound
			self->setTime = level.time + 1000; // extra debounce so that the sounds don't overlap too much
			G_Sound( self, G_SoundIndex( "sound/interface/ammocon_empty.mp3" ));
			self->s.loopSound = 0;

			if ( self->s.eFlags & EF_SHADER_ANIM )
			{
				self->s.frame = 1;
			}
		}
		else if  ( ps->ammo[AMMO_BLASTER] >= ammoData[AMMO_BLASTER].max 
						&& ps->ammo[AMMO_POWERCELL] >= ammoData[AMMO_POWERCELL].max 
						&& ps->ammo[AMMO_METAL_BOLTS] >= ammoData[AMMO_METAL_BOLTS].max )
		{
			// play full sound
			G_Sound( self, G_SoundIndex( "sound/interface/ammocon_done.wav" ));
			self->setTime = level.time + 1000; // extra debounce so that the sounds don't overlap too much
			self->s.loopSound = 0;
		}
	}

	
	if ( self->s.loopSound )
	{
		// we will have to shut of the loop sound, so I guess try and do it intelligently...NOTE: this could get completely stomped every time through the loop
		//	this is fine, since it just controls shutting off the sound when there are situations that could start the sound but not shut it off
		self->e_ThinkFunc = thinkF_poll_converter;
		self->nextthink = level.time + 500;
	}
	else
	{
		// sound is already off, so we don't need to "think" about it.
		self->e_ThinkFunc = thinkF_NULL;
		self->nextthink = 0;
	}
}

/*QUAKED misc_model_ammo_power_converter (1 0 0) (-16 -16 -16) (16 16 16) x x x USETARGET
#MODELNAME="models/items/power_converter.md3"
Gives ammo energy when used.

USETARGET - when used it fires off target

"health" - how much health the model has - default 60 (zero makes non-breakable)
"targetname" - dies and displays damagemodel when used, if any (if not, removes itself)
"target" - what to use when it dies
"paintarget" - target to fire when hit (but not destroyed)
"count" - the amount of ammo given when used (default 100)
*/
//------------------------------------------------------------
void SP_misc_model_ammo_power_converter( gentity_t *ent )
{
	SetMiscModelDefaults( ent, useF_ammo_power_converter_use, "4", CONTENTS_SOLID, NULL, qfalse, NULL );

	ent->takedamage = qfalse;

	EnergyAmmoStationSettings(ent);

	G_SoundIndex("sound/interface/ammocon_run.wav");
	G_SoundIndex("sound/interface/ammocon_done.mp3");
	G_SoundIndex("sound/interface/ammocon_empty.mp3");

	ent->s.modelindex2 = G_ModelIndex("/models/items/power_converter.md3");	// Precache model
}

/*QUAKED misc_ammo_floor_unit (1 0 0) (-16 -16 0) (16 16 32) x x x USETARGET
#MODELNAME="models/items/a_pwr_converter.md3"
Gives ammo energy when used.

USETARGET - when used it fires off target

"health" - how much health the model has - default 60 (zero makes non-breakable)
"targetname" - dies and displays damagemodel when used, if any (if not, removes itself)
"target" - what to use when it dies
"paintarget" - target to fire when hit (but not destroyed)
"count" - the amount of ammo given when used (default 100)
*/
//------------------------------------------------------------
void SP_misc_ammo_floor_unit( gentity_t *ent )
{
	VectorSet( ent->mins, -16, -16, 0 );
	VectorSet( ent->maxs, 16, 16, 32 );

	SetMiscModelDefaults( ent, useF_ammo_power_converter_use, "4", CONTENTS_SOLID, NULL, qfalse, NULL );

	ent->takedamage = qfalse;

	EnergyAmmoStationSettings(ent);

	G_SoundIndex("sound/interface/ammocon_run.wav");
	G_SoundIndex("sound/interface/ammocon_done.mp3");
	G_SoundIndex("sound/interface/ammocon_empty.mp3");

	ent->s.modelindex = G_ModelIndex("models/items/a_pwr_converter.md3");	// Precache model
	ent->s.eFlags |= EF_SHADER_ANIM;
}


/*
================
welder_think
================
*/
void welder_think( gentity_t *self )
{
	self->nextthink = level.time + 200;
	vec3_t		org,
				dir;
	mdxaBone_t	boltMatrix;

	if( self->svFlags & SVF_INACTIVE )
	{	
		return;
	}

	int newBolt;
	
	// could alternate between the two... or make it random... ?
	newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*flash" );
//	newBolt = gi.G2API_AddBolt( &self->ghoul2[self->playerModel], "*flash01" );

	if ( newBolt != -1 )
	{
		G_Sound( self, self->noise_index );
	//	G_PlayEffect( "blueWeldSparks", self->playerModel, newBolt, self->s.number);
		// The welder gets rotated around a lot, and since the origin is offset by 352 I have to make this super expensive call to position the hurt...
		gi.G2API_GetBoltMatrix( self->ghoul2, self->playerModel, newBolt,
					&boltMatrix, self->currentAngles, self->currentOrigin, (cg.time?cg.time:level.time),
					NULL, self->s.modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		VectorSubtract( self->currentOrigin, org, dir );
		VectorNormalize( dir );
		// we want the  welder effect to face inwards....
		G_PlayEffect( "blueWeldSparks", org, dir );
		G_RadiusDamage( org, self, 10, 45, self, MOD_UNKNOWN );
	}

} 

/*
================
welder_use
================
*/
void welder_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// Toggle on and off
	if( self->spawnflags & 1 )
	{
		self->nextthink = level.time + FRAMETIME;		
	}
	else
	{
		self->nextthink = -1;	
	}
	self->spawnflags = (self->spawnflags ^ 1);
	
}

/*QUAKED misc_model_welder (1 0 0) (-16 -16 -16) (16 16 16) START_OFF 
#MODELNAME="models/map_objects/cairn/welder.md3"
When 'on' emits sparks from it's welding points

START_OFF  - welder starts off, using it toggles it on/off

*/
//------------------------------------------------------------
void SP_misc_model_welder( gentity_t *ent )
{
	VectorSet( ent->mins, 336, -16, 0 );
	VectorSet( ent->maxs, 368, 16, 32 );

	SetMiscModelDefaults( ent, useF_welder_use, "4", CONTENTS_SOLID, NULL, qfalse, NULL );

	ent->takedamage = qfalse;
	ent->contents = 0;
	G_EffectIndex( "blueWeldSparks" );
	ent->noise_index = G_SoundIndex( "sound/movers/objects/welding.wav" );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/cairn/welder.glm" );
//	ent->s.modelindex2 = G_ModelIndex( "models/map_objects/cairn/welder.md3" );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, "models/map_objects/cairn/welder.glm", ent->s.modelindex, NULL, NULL, 0, 0 );
	ent->s.radius = 400.0f;// the origin of the welder is offset by approximately 352, so we need the big radius

	ent->e_ThinkFunc = thinkF_welder_think;

	ent->nextthink = level.time + 1000;
	if( ent->spawnflags & 1 )
	{
		ent->nextthink = -1;
	}
}


/*
================
jabba_cam_use
================
*/
void jabba_cam_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if( self->spawnflags & 1 )
	{
		self->spawnflags &= ~1;
		gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone, 15, 0, BONE_ANIM_OVERRIDE_FREEZE, -1.5f, (cg.time?cg.time:level.time), -1, 0 );

	}
	else
	{
		self->spawnflags |= 1;
		gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], self->rootBone, 0, 15, BONE_ANIM_OVERRIDE_FREEZE, 1.5f, (cg.time?cg.time:level.time), -1, 0 );
	}
}

/*QUAKED misc_model_jabba_cam (1 0 0) ( 0 -8 0) (60 8 16) EXTENDED
#MODELNAME="models/map_objects/nar_shaddar/jabacam.md3"

The eye camera that popped out of Jabba's front door

  EXTENDED - Starts in the extended position

  targetname - Toggles it on/off
  
*/
//-----------------------------------------------------
void SP_misc_model_jabba_cam( gentity_t *ent )
//-----------------------------------------------------
{

	VectorSet( ent->mins, -60.0f, -8.0f, 0.0f );
	VectorSet( ent->maxs, 60.0f, 8.0f, 16.0f );
		
	SetMiscModelDefaults( ent, useF_jabba_cam_use, "4", 0, NULL, qfalse, NULL );
	G_SetAngles( ent, ent->s.angles );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/nar_shaddar/jabacam/jabacam.glm" );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, "models/map_objects/nar_shaddar/jabacam/jabacam.glm", ent->s.modelindex, NULL, NULL, 0, 0 );
	ent->s.radius = 150.0f;  //......
	VectorSet( ent->s.modelScale, 1.0f, 1.0f, 1.0f );

	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
	
	ent->e_UseFunc = useF_jabba_cam_use;

	ent->takedamage = qfalse;

	// start extended..
	if ( ent->spawnflags & 1 )
	{
		gi.G2API_SetBoneAnimIndex( &ent->ghoul2[ent->playerModel], ent->rootBone, 0, 15, BONE_ANIM_OVERRIDE_FREEZE, 0.6f, cg.time, -1, -1 );
	}

	gi.linkentity( ent );
}

//------------------------------------------------------------------------
void misc_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	misc_model_breakable_die( self, other, activator, 100, MOD_UNKNOWN );
}

/*QUAKED misc_exploding_crate (1 0 0.25) (-24 -24 0) (24 24 64)
#MODELNAME="models/map_objects/nar_shaddar/crate_xplode.md3"
Basic exploding crate

"health" - how much health the model has - default 40 (zero makes non-breakable)

"splashRadius" - radius to do damage in - default 128
"splashDamage" - amount of damage to do when it explodes - default 50

"targetname" - auto-explodes
"target" - what to use when it dies

*/
//------------------------------------------------------------
void SP_misc_exploding_crate( gentity_t *ent )
{
	G_SpawnInt( "health", "40", &ent->health );
	G_SpawnInt( "splashRadius", "128", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "50", &ent->splashDamage );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/nar_shaddar/crate_xplode.md3" );
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_EffectIndex( "chunks/metalexplode" );
	
	VectorSet( ent->mins, -24, -24, 0 );
	VectorSet( ent->maxs, 24, 24, 64 );

	ent->contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;
	ent->takedamage = qtrue;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	gi.linkentity (ent);

	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_misc_use;
	}

	ent->material = MAT_CRATE1;
	ent->e_DieFunc = dieF_misc_model_breakable_die;//ExplodeDeath;
}

/*QUAKED misc_gas_tank (1 0 0.25) (-4 -4 0) (4 4 40)
#MODELNAME="models/map_objects/imp_mine/tank.md3"
Basic exploding oxygen tank

"health" - how much health the model has - default 20 (zero makes non-breakable)

"splashRadius" - radius to do damage in - default 48
"splashDamage" - amount of damage to do when it explodes - default 32

"targetname" - auto-explodes
"target" - what to use when it dies

*/

void gas_random_jet( gentity_t *self )
{
	vec3_t pt;

	VectorCopy( self->currentOrigin, pt );
	pt[2] += 50;

	G_PlayEffect( "env/mini_gasjet", pt );

	self->nextthink = level.time + random() * 16000 + 12000; // do this rarely
}

//------------------------------------------------------------
void GasBurst( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod, int hitLoc )
{
	vec3_t pt;

	VectorCopy( self->currentOrigin, pt );
	pt[2] += 46;

	G_PlayEffect( "env/mini_flamejet", pt );

	// do some damage to anything that may be standing on top of it when it bursts into flame
	pt[2] += 32;
	G_RadiusDamage( pt, self, 32, 32, self, MOD_UNKNOWN );

	//  only get one burst
	self->e_PainFunc = painF_NULL;
}

//------------------------------------------------------------
void SP_misc_gas_tank( gentity_t *ent )
{
	G_SpawnInt( "health", "20", &ent->health );
	G_SpawnInt( "splashRadius", "48", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "32", &ent->splashDamage );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/tank.md3" );
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_EffectIndex( "chunks/metalexplode" );
	G_EffectIndex( "env/mini_flamejet" );
	G_EffectIndex( "env/mini_gasjet" );

	VectorSet( ent->mins, -4, -4, 0 );
	VectorSet( ent->maxs, 4, 4, 40 );

	ent->contents = CONTENTS_SOLID;
	ent->takedamage = qtrue;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	gi.linkentity (ent);

	ent->e_PainFunc = painF_GasBurst;

	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_misc_use;
	}

	ent->material = MAT_METAL3;

	ent->e_DieFunc = dieF_misc_model_breakable_die;

	ent->e_ThinkFunc = thinkF_gas_random_jet;
	ent->nextthink = level.time + random() * 12000 + 6000; // do this rarely
}

/*QUAKED misc_crystal_crate (1 0 0.25) (-34 -34 0) (34 34 44) NON_SOLID
#MODELNAME="models/map_objects/imp_mine/crate_open.md3"
Open crate of crystals that explode when shot

NON_SOLID - can only be shot 

"health" - how much health the crate has, default 80 

"splashRadius" - radius to do damage in, default 80
"splashDamage" - amount of damage to do when it explodes, default 40

"targetname" - auto-explodes
"target" - what to use when it dies

*/

//------------------------------------------------------------
void CrystalCratePain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod, int hitLoc )
{
	vec3_t pt;

	VectorCopy( self->currentOrigin, pt );
	pt[2] += 36;

	G_PlayEffect( "env/crystal_crate", pt );

	// do some damage, heh
	pt[2] += 32;
	G_RadiusDamage( pt, self, 16, 32, self, MOD_UNKNOWN );

}

//------------------------------------------------------------
void SP_misc_crystal_crate( gentity_t *ent )
{
	G_SpawnInt( "health", "80", &ent->health );
	G_SpawnInt( "splashRadius", "80", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "40", &ent->splashDamage );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/crate_open.md3" );
	ent->fxID = G_EffectIndex( "thermal/explosion" ); // FIXME: temp
	G_EffectIndex( "env/crystal_crate" );
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");

	VectorSet( ent->mins, -34, -34, 0 );
	VectorSet( ent->maxs, 34, 34, 44 );
	
	//Blocks movement
	ent->contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//Was CONTENTS_SOLID, but only architecture should be this

	if ( ent->spawnflags & 1 )  // non-solid
	{	// Override earlier contents flag...
		//Can only be shot
		ent->contents = CONTENTS_SHOTCLIP;
	}
	

	ent->takedamage = qtrue;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	gi.linkentity (ent);

	ent->e_PainFunc = painF_CrystalCratePain;

	if ( ent->targetname )
	{
		ent->e_UseFunc = useF_misc_use;
	}

	ent->material = MAT_CRATE2;

	ent->e_DieFunc = dieF_misc_model_breakable_die;
}

/*QUAKED misc_atst_drivable (1 0 0.25) (-40 -40 -24) (40 40 248)
#MODELNAME="models/players/atst/model.glm"

Drivable ATST, when used by player, they become the ATST.  When the player hits use again, they get out.

"health" - how much health the atst has - default 800

"target" - what to use when it dies
*/
void misc_atst_setanim( gentity_t *self, int bone, int anim )
{
	if ( bone < 0 || anim < 0 )
	{
		return;
	}
	int	firstFrame = -1;
	int	lastFrame = -1;
	float animSpeed = 0;
	//try to get anim ranges from the animation.cfg for an AT-ST
	for ( int i = 0; i < level.numKnownAnimFileSets; i++ )
	{
		if ( !Q_stricmp( "atst", level.knownAnimFileSets[i].filename ) )
		{ 
			firstFrame = level.knownAnimFileSets[i].animations[anim].firstFrame;
			lastFrame = firstFrame+level.knownAnimFileSets[i].animations[anim].numFrames;
			animSpeed = 50.0f / level.knownAnimFileSets[i].animations[anim].frameLerp;
			break;
		}
	}
	if ( firstFrame != -1 && lastFrame != -1 && animSpeed != 0 )
	{
		if (!gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], bone, firstFrame,
								lastFrame, BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND, animSpeed, 
								(cg.time?cg.time:level.time), -1, 150 ))
		{
			gi.G2API_SetBoneAnimIndex( &self->ghoul2[self->playerModel], bone, firstFrame,
								lastFrame, BONE_ANIM_OVERRIDE_FREEZE, animSpeed, 
								(cg.time?cg.time:level.time), -1, 150 );
		}
	}
}
void misc_atst_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{//ATST was destroyed while you weren't in it
	//can't be used anymore
	self->e_UseFunc = useF_NULL;
	//sigh... remove contents so we don't block the player's path...
	self->contents = CONTENTS_CORPSE;
	self->takedamage = qfalse;
	self->maxs[2] = 48;

	//FIXME: match to slope
	vec3_t effectPos;
	VectorCopy( self->currentOrigin, effectPos );
	effectPos[2] -= 15;
	G_PlayEffect( "droidexplosion1", effectPos );
//	G_PlayEffect( "small_chunks", effectPos );
	//set these to defaults that work in a worst-case scenario (according to current animation.cfg)
	gi.G2API_StopBoneAnimIndex( &self->ghoul2[self->playerModel], self->craniumBone );
	misc_atst_setanim( self, self->rootBone, BOTH_DEATH1 );
}

extern void G_DriveATST( gentity_t *ent, gentity_t *atst );
extern void SetClientViewAngle( gentity_t *ent, vec3_t angle );
extern qboolean PM_InSlopeAnim( int anim );
void misc_atst_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator || activator->s.number )
	{//only player can do this
		return;
	}

	int	tempLocDmg[HL_MAX];
	int	hl, tempHealth;

	if ( activator->client->NPC_class != CLASS_ATST )
	{//get in the ATST
		if ( activator->client->ps.groundEntityNum != self->s.number )
		{//can only get in if on top of me...
			//we *could* even check for the hatch surf...?
			return;
		}
		//copy origin
		G_SetOrigin( activator, self->currentOrigin );

		//copy angles
		VectorCopy( self->s.angles2, self->currentAngles );
		G_SetAngles( activator, self->currentAngles );
		SetClientViewAngle( activator, self->currentAngles );

		//set player to my g2 instance
		gi.G2API_StopBoneAnimIndex( &self->ghoul2[self->playerModel], self->craniumBone );
		G_DriveATST( activator, self );
		activator->activator = self;
		self->s.eFlags |= EF_NODRAW;
		self->svFlags |= SVF_NOCLIENT;
		self->contents = 0;
		self->takedamage = qfalse;
		//transfer armor
		tempHealth = self->health;
		self->health = activator->client->ps.stats[STAT_ARMOR];
		activator->client->ps.stats[STAT_ARMOR] = tempHealth;
		//transfer locationDamage
		for ( hl = HL_NONE; hl < HL_MAX; hl++ )
		{
			tempLocDmg[hl] = activator->locationDamage[hl];
			activator->locationDamage[hl] = self->locationDamage[hl];
			self->locationDamage[hl] = tempLocDmg[hl];
		}
		if ( !self->s.number )
		{
			CG_CenterPrint( "@INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
		}
	}
	else
	{//get out of ATST
		int legsAnim = activator->client->ps.legsAnim;
		if ( legsAnim != BOTH_STAND1 
			&& !PM_InSlopeAnim( legsAnim ) 
			&& legsAnim != BOTH_TURN_RIGHT1 && legsAnim != BOTH_TURN_LEFT1 )
		{//can't get out of it while it's still moving
			return;
		}
		//FIXME: after a load/save, this crashes, BAD... somewhere in G2
		G_SetOrigin( self, activator->currentOrigin );
		VectorSet( self->currentAngles, 0, activator->client->ps.legsYaw, 0 );
		//self->currentAngles[PITCH] = activator->currentAngles[ROLL] = 0;
		G_SetAngles( self, self->currentAngles );
		VectorCopy( activator->currentAngles, self->s.angles2 );
		//remove my G2
		if ( self->playerModel >= 0 )
		{
			gi.G2API_RemoveGhoul2Model( self->ghoul2, self->playerModel );
			self->playerModel = -1;
		}
		//copy player's
		gi.G2API_CopyGhoul2Instance( activator->ghoul2, self->ghoul2, -1 );
		self->playerModel = 0;//assumption
		//reset player to kyle
		G_DriveATST( activator, NULL );
		activator->activator = NULL;
		self->s.eFlags &= ~EF_NODRAW;
		self->svFlags &= ~SVF_NOCLIENT;
		self->contents = CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;
		self->takedamage = qtrue;
		//transfer armor
		tempHealth = self->health;
		self->health = activator->client->ps.stats[STAT_ARMOR];
		activator->client->ps.stats[STAT_ARMOR] = tempHealth;
		//transfer locationDamage
		for ( hl = HL_NONE; hl < HL_MAX; hl++ )
		{
			tempLocDmg[hl] = self->locationDamage[hl];
			self->locationDamage[hl] = activator->locationDamage[hl];
			activator->locationDamage[hl] = tempLocDmg[hl];
		}
		//link me back in
		gi.linkentity ( self );
		//put activator on top of me?
		vec3_t	newOrg = {activator->currentOrigin[0], activator->currentOrigin[1], activator->currentOrigin[2] + (self->maxs[2]-self->mins[2]) + 1 };
		G_SetOrigin( activator, newOrg );
		//open the hatch
		misc_atst_setanim( self, self->craniumBone, BOTH_STAND2 );
		gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "head_hatchcover_off", 0 );
		G_Sound( self, G_SoundIndex( "sound/chars/atst/atst_hatch_open" ));
	}
}

void SP_misc_atst_drivable( gentity_t *ent )
{
	extern void NPC_ATST_Precache(void);
	extern void NPC_PrecacheAnimationCFG( const char *NPC_type );

	ent->s.modelindex = G_ModelIndex( "models/players/atst/model.glm" );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, "models/players/atst/model.glm", ent->s.modelindex, NULL, NULL, 0, 0 );
	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "model_root", qtrue );
	ent->craniumBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "cranium", qtrue );	//FIXME: need to somehow set the anim/frame to the equivalent of BOTH_STAND1...  use to be that BOTH_STAND1 was the first frame of the glm, but not anymore
	ent->s.radius = 320;
	VectorSet( ent->s.modelScale, 1.0f, 1.0f, 1.0f );

	//register my weapons, sounds and model
	RegisterItem( FindItemForWeapon( WP_ATST_MAIN ));	//precache the weapon
	RegisterItem( FindItemForWeapon( WP_ATST_SIDE ));	//precache the weapon
	//HACKHACKHACKTEMP - until ATST gets real weapons of it's own?
	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));	//precache the weapon
//	RegisterItem( FindItemForWeapon( WP_ROCKET_LAUNCHER ));	//precache the weapon
//	RegisterItem( FindItemForWeapon( WP_BOWCASTER ));	//precache the weapon
	//HACKHACKHACKTEMP - until ATST gets real weapons of it's own?

	G_SoundIndex( "sound/chars/atst/atst_hatch_open" );
	G_SoundIndex( "sound/chars/atst/atst_hatch_close" );

	NPC_ATST_Precache();
	ent->NPC_type = "atst";
	NPC_PrecacheAnimationCFG( ent->NPC_type );
	//open the hatch
	misc_atst_setanim( ent, ent->rootBone, BOTH_STAND2 );
	gi.G2API_SetSurfaceOnOff( &ent->ghoul2[ent->playerModel], "head_hatchcover_off", 0 );

	VectorSet( ent->mins, ATST_MINS0, ATST_MINS1, ATST_MINS2 );
	VectorSet( ent->maxs, ATST_MAXS0, ATST_MAXS1, ATST_MAXS2 );

	ent->contents = CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;
	ent->flags |= FL_SHIELDED;
	ent->takedamage = qtrue;
	if ( !ent->health )
	{
		ent->health = 800;
	}

	ent->max_health = ent->health; // cg_draw needs this

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	VectorCopy( ent->currentAngles, ent->s.angles2 );

	gi.linkentity ( ent );

	//FIXME: test the origin to make sure I'm clear?

	ent->e_UseFunc = useF_misc_atst_use;
	ent->svFlags |= SVF_PLAYER_USABLE;

	//make it able to take damage and die when you're not in it...
	//do an explosion and play the death anim, remove use func.
	ent->e_DieFunc = dieF_misc_atst_die;
}