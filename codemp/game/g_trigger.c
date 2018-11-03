// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"

int gTrigFallSound;

void InitTrigger( gentity_t *self ) {
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	if (self->model && self->model[0] == '*') { //Feature from Lugormod - Logical triggers without model key using mins/maxs instead
		trap->SetBrushModel( (sharedEntity_t *)self, self->model );
	}
	//Check for .md3 or .glm model ?
	else {
		G_SpawnVector("mins", "-8 -8 -8", self->r.mins);
		G_SpawnVector("maxs", "8 8 8", self->r.maxs);
	}

	self->r.contents = CONTENTS_TRIGGER;		// replaces the -1 from trap->SetBrushModel
	self->r.svFlags = SVF_NOCLIENT; //whats this? triggers are not networked to clients then?

	if(self->spawnflags & 128)
	{
		self->flags |= FL_INACTIVE;
	}
}

// the wait time has passed, so set back up for another activation
void multi_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}

void trigger_cleared_fire (gentity_t *self);

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger_run( gentity_t *ent ) 
{
	ent->think = 0;

	G_ActivateBehavior( ent, BSET_USE );

	if ( ent->soundSet && ent->soundSet[0] )
	{
		trap->SetConfigstring( CS_GLOBAL_AMBIENT_SET, ent->soundSet );
	}

	if (ent->genericValue4)
	{ //we want to activate target3 for team1 or target4 for team2
		if (ent->genericValue4 == SIEGETEAM_TEAM1 &&
			ent->target3 && ent->target3[0])
		{
			G_UseTargets2(ent, ent->activator, ent->target3);
		}
		else if (ent->genericValue4 == SIEGETEAM_TEAM2 &&
			ent->target4 && ent->target4[0])
		{
			G_UseTargets2(ent, ent->activator, ent->target4);
		}

		ent->genericValue4 = 0;
	}

	G_UseTargets (ent, ent->activator);
	if ( ent->noise_index )
	{
		if (ent->classname && (!Q_stricmp( "df_trigger_start", ent->classname) || !Q_stricmp( "df_trigger_finish", ent->classname)))//JAPRO FIXME, we dont want to do this if its a df_trigger_finish, since it spams ?
		{
		}
		else
			G_Sound( ent->activator, CHAN_AUTO, ent->noise_index );
	}

	if ( ent->target2 && ent->target2[0] && ent->wait >= 0 )
	{
		ent->think = trigger_cleared_fire;
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
		ent->r.contents &= ~CONTENTS_TRIGGER;//so the EntityContact trace doesn't have to be done against me
		ent->think = 0;
		ent->use = 0;
		//Don't remove, Icarus may barf?
		//ent->nextthink = level.time + FRAMETIME;
		//ent->think = G_FreeEntity;
	}
	if( ent->activator && ent->activator->client )
	{	// mark the trigger as being touched by the player
		ent->aimDebounceTime = level.time;
	}
}

//determine if the class given is listed in the string using the | formatting
qboolean G_NameInTriggerClassList(char *list, char *str)
{
	char cmp[MAX_STRING_CHARS];
	int i = 0;
	int j;

	while (list[i])
	{
        j = 0;
        while (list[i] && list[i] != '|')
		{
			cmp[j] = list[i];
			i++;
			j++;
		}
		cmp[j] = 0;

		if (!Q_stricmp(str, cmp))
		{ //found it
			return qtrue;
		}
		if (list[i] != '|')
		{ //reached the end and never found it
			return qfalse;
		}
		i++;
	}

	return qfalse;
}

extern qboolean gSiegeRoundBegun;
void SiegeItemRemoveOwner(gentity_t *ent, gentity_t *carrier);
void multi_trigger( gentity_t *ent, gentity_t *activator ) 
{
	qboolean haltTrigger = qfalse;

	if ( ent->think == multi_trigger_run )
	{//already triggered, just waiting to run
		return;
	}

	if (level.gametype == GT_SIEGE &&
		!gSiegeRoundBegun)
	{ //nothing can be used til the round starts.
		return;
	}

	if (level.gametype == GT_SIEGE &&
		activator && activator->client &&
		ent->alliedTeam &&
		activator->client->sess.sessionTeam != ent->alliedTeam)
	{ //this team can't activate this trigger.
		return;
	}

	if (level.gametype == GT_SIEGE &&
		ent->idealclass && ent->idealclass[0])
	{ //only certain classes can activate it
		if (!activator ||
			!activator->client ||
			activator->client->siegeClass < 0)
		{ //no class
			return;
		}

		if (!G_NameInTriggerClassList(bgSiegeClasses[activator->client->siegeClass].name, ent->idealclass))
		{ //wasn't in the list
			return;
		}
	}

	if (ent->spawnflags & 4096) {
		if (activator && activator->client && activator->client->sess.raceMode)
			return;
	}

	if (level.gametype == GT_SIEGE && ent->genericValue1)
	{
		haltTrigger = qtrue;

		if (activator && activator->client &&
			activator->client->holdingObjectiveItem &&
			ent->targetname && ent->targetname[0])
		{
			gentity_t *objItem = &g_entities[activator->client->holdingObjectiveItem];

			if (objItem && objItem->inuse)
			{
				if (objItem->goaltarget && objItem->goaltarget[0] &&
					!Q_stricmp(ent->targetname, objItem->goaltarget))
				{
					if (objItem->genericValue7 != activator->client->sess.sessionTeam)
					{ //The carrier of the item is not on the team which disallows objective scoring for it
						if (objItem->target3 && objItem->target3[0])
						{ //if it has a target3, fire it off instead of using the trigger
							G_UseTargets2(objItem, objItem, objItem->target3);

                            //3-24-03 - want to fire off the target too I guess, if we have one.
							if (ent->targetname && ent->targetname[0])
							{
								haltTrigger = qfalse;
							}
						}
						else
						{
							haltTrigger = qfalse;
						}

						//now that the item has been delivered, it can go away.
						SiegeItemRemoveOwner(objItem, activator);
						objItem->nextthink = 0;
						objItem->neverFree = qfalse;
						G_FreeEntity(objItem);
					}
				}
			}
		}
	}
	else if (ent->genericValue1)
	{ //Never activate in non-siege gametype I guess.
		return;
	}

	if (ent->genericValue2)
	{ //has "teambalance" property
		int i = 0;
		int team1ClNum = 0;
		int team2ClNum = 0;
		int owningTeam = ent->genericValue3;
		int newOwningTeam = 0;
		int numEnts = 0;
		int entityList[MAX_GENTITIES];
		gentity_t *cl;

		if (level.gametype != GT_SIEGE)
		{
			return;
		}

		if (!activator->client ||
			(activator->client->sess.sessionTeam != SIEGETEAM_TEAM1 && activator->client->sess.sessionTeam != SIEGETEAM_TEAM2))
		{ //activator must be a valid client to begin with
			return;
		}

		//Count up the number of clients standing within the bounds of the trigger and the number of them on each team
		numEnts = trap->EntitiesInBox( ent->r.absmin, ent->r.absmax, entityList, MAX_GENTITIES );
		while (i < numEnts)
		{
			if (entityList[i] < MAX_CLIENTS)
			{ //only care about clients
				cl = &g_entities[entityList[i]];

				//the client is valid
				if (cl->inuse && cl->client &&
					(cl->client->sess.sessionTeam == SIEGETEAM_TEAM1 || cl->client->sess.sessionTeam == SIEGETEAM_TEAM2) &&
					cl->health > 0 &&
					!(cl->client->ps.eFlags & EF_DEAD))
				{
					//See which team he's on
					if (cl->client->sess.sessionTeam == SIEGETEAM_TEAM1)
					{
						team1ClNum++;
					}
					else
					{
						team2ClNum++;
					}
				}
			}
			i++;
		}

		if (!team1ClNum && !team2ClNum)
		{ //no one in the box? How did we get activated? Oh well.
			return;
		}

		if (team1ClNum == team2ClNum)
		{ //if equal numbers the ownership will remain the same as it is now
			return;
		}

		//decide who owns it now
		if (team1ClNum > team2ClNum)
		{
			newOwningTeam = SIEGETEAM_TEAM1;
		}
		else
		{
			newOwningTeam = SIEGETEAM_TEAM2;
		}

		if (owningTeam == newOwningTeam)
		{ //it's the same one it already was, don't care then.
			return;
		}

		//Set the new owner and set the variable which will tell us to activate a team-specific target
		ent->genericValue3 = newOwningTeam;
		ent->genericValue4 = newOwningTeam;
	}

	if (haltTrigger)
	{ //This is an objective trigger and the activator is not carrying an objective item that matches the targetname.
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
	if( activator && activator->s.number < MAX_CLIENTS && ent->aimDebounceTime == level.time )
	{
		return;	
	}

	if ( ent->flags & FL_INACTIVE )
	{//Not active at this time
		return;
	}

	ent->activator = activator;

	if(ent->delay && ent->painDebounceTime < (level.time + ent->delay) )
	{//delay before firing trigger
		ent->think = multi_trigger_run;
		ent->nextthink = level.time + ent->delay;
		ent->painDebounceTime = level.time;
#if 1
		if (activator->client && activator->client->sess.raceMode)
			ent->nextthink = level.time; //No delay for triggers in racemode to prevent advantages from player interference... does this have bad side effects?
		//This can still be griefed by ppl just spamming the trigger to fuck its "wait" time up?
#endif

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

qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );

void Touch_Multi( gentity_t *self, gentity_t *other, trace_t *trace ) 
{
	if( !other->client ) 
	{
		return;
	}

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if( self->alliedTeam )
	{
		if ( other->client->sess.sessionTeam != self->alliedTeam )
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

//JAPRO - Serverside - Allow/disallow use button/trigger for duelers - Start
	if (other->client->ps.duelInProgress && (g_allowUseInDuel.integer >= 2))//Loda fixme, make this spawnflags 5 for button? or should it block all triggers?
		return;
	if ((self->spawnflags & 4) && other->client->ps.duelInProgress && !g_allowUseInDuel.integer)
		return;
//JAPRO - Serverside - Allow/disallow use button/trigger for duelers - End

	if ((self->spawnflags & 4) && (other->client->ps.powerups[PW_NEUTRALFLAG] && g_rabbit.integer))
		return;

	if ( self->spawnflags & 1 )
	{
		if ( other->s.eType == ET_NPC )
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

		AngleVectors( other->client->ps.viewangles, forward, NULL, NULL );

		if ( DotProduct( self->movedir, forward ) < 0.5 )
		{//Not Within 45 degrees
			return;
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if( !( other->client->pers.cmd.buttons & BUTTON_USE ) )
		{//not pressing use button
			return;
		}

		if ((other->client->ps.weaponTime > 0 && other->client->ps.torsoAnim != BOTH_BUTTON_HOLD && other->client->ps.torsoAnim != BOTH_CONSOLE1) || other->health < 1 ||
			(other->client->ps.pm_flags & PMF_FOLLOW) || other->client->sess.sessionTeam == TEAM_SPECTATOR ||
			other->client->ps.forceHandExtend != HANDEXTEND_NONE)
		{ //player has to be free of other things to use.
			return;
		}

		if (self->genericValue7)
		{ //we have to be holding the use key in this trigger for x milliseconds before firing
			if (level.gametype == GT_SIEGE &&
				self->idealclass && self->idealclass[0])
			{ //only certain classes can activate it
				if (!other ||
					!other->client ||
					other->client->siegeClass < 0)
				{ //no class
					return;
				}

				if (!G_NameInTriggerClassList(bgSiegeClasses[other->client->siegeClass].name, self->idealclass))
				{ //wasn't in the list
					return;
				}
			}

			if (!G_PointInBounds( other->client->ps.origin, self->r.absmin, self->r.absmax ))
			{
				return;
			}
			else if (other->client->isHacking != self->s.number && other->s.number < MAX_CLIENTS )
			{ //start the hack
				other->client->isHacking = self->s.number;
				VectorCopy(other->client->ps.viewangles, other->client->hackingAngles);
				other->client->ps.hackingTime = level.time + self->genericValue7;
				other->client->ps.hackingBaseTime = self->genericValue7;
				if (other->client->ps.hackingBaseTime > 60000)
				{ //don't allow a bit overflow
					other->client->ps.hackingTime = level.time + 60000;
					other->client->ps.hackingBaseTime = 60000;
				}
				return;
			}
			else if (other->client->ps.hackingTime < level.time)
			{ //finished with the hack, reset the hacking values and let it fall through
				other->client->isHacking = 0; //can't hack a client
				other->client->ps.hackingTime = 0;
			}
			else
			{ //hack in progress
				return;
			}
		}
	}

	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		if( !( other->client->pers.cmd.buttons & BUTTON_ATTACK ) &&
			!( other->client->pers.cmd.buttons & BUTTON_ALT_ATTACK ) )
		{//not pressing fire button or altfire button
			return;
		}
	}

	if ( self->radius )
	{
		vec3_t	eyeSpot;

		//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		VectorCopy(other->client->ps.origin, eyeSpot);
		eyeSpot[2] += other->client->ps.viewheight;

		if ( G_PointInBounds( eyeSpot, self->r.absmin, self->r.absmax ) )
		{
			if( !( other->client->pers.cmd.buttons & BUTTON_ATTACK ) &&
				!( other->client->pers.cmd.buttons & BUTTON_ALT_ATTACK ) )
			{//not attacking, so hiding bonus
				/*
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
				*/
				//Not using this, at least not yet.
			}
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if (other->client->sess.raceMode && other->client->sess.movementStyle == MV_JETPACK && VectorLengthSquared(other->client->ps.velocity))
			return;
		if (other->client->sess.raceMode && other->client->sess.movementStyle == MV_SWOOP && other->client->ps.m_iVehicleNum)
			return;
		if (other->client->ps.torsoAnim != BOTH_BUTTON_HOLD &&
			other->client->ps.torsoAnim != BOTH_CONSOLE1)
		{
			G_SetAnim( other, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
		}
		else
		{
			other->client->ps.torsoTimer = 500;
		}
		other->client->ps.weaponTime = other->client->ps.torsoTimer;
	}
	
	if ( self->think == trigger_cleared_fire )
	{//We're waiting to fire our target2 first
		self->nextthink = level.time + self->speed;
		return;
	}

	multi_trigger( self, other );
}

void trigger_cleared_fire (gentity_t *self)
{
	G_UseTargets2( self, self->activator, self->target2 );
	self->think = 0;
	// should start the wait timer now, because the trigger's just been cleared, so we must "wait" from this point
	if ( self->wait > 0 ) 
	{
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
}

/*QUAKED trigger_multiple (.1 .5 .1) ? CLIENTONLY FACING USE_BUTTON FIRE_BUTTON NPCONLY x x INACTIVE MULTIPLE
CLIENTONLY - only a player can trigger this by touch
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
"NPC_targetname"  Only the NPC with this NPC_targetname fires this trigger

Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"team" - If set, only this team can trip this trigger
	0 - any
	1 - red
	2 - blue

"soundSet"	Ambient sound set to play when this trigger is activated

usetime		-	If specified (in milliseconds) along with the USE_BUTTON flag, will
				require a client to hold the use key for x amount of ms before firing.

Applicable only during Siege gametype:
teamuser	-	if 1, team 2 can't use this. If 2, team 1 can't use this.
siegetrig	-	if non-0, can only be activated by players carrying a misc_siege_item
				which is associated with this trigger by the item's goaltarget value.
teambalance	-	if non-0, is "owned" by the last team that activated. Can only be activated
				by the other team if the number of players on the other team inside	the
				trigger outnumber the number of players on the owning team inside the
				trigger.
target3		-	fire when activated by team1
target4		-	fire when activated by team2

idealclass	-	Can only be used by this class/these classes. You can specify use by
				multiple classes with the use of |, e.g.:
				"Imperial Medic|Imperial Assassin|Imperial Demolitionist"
*/
void SP_trigger_multiple( gentity_t *ent ) 
{
	char	*s;
	if ( G_SpawnString( "noise", "", &s ) ) 
	{
		if (s && s[0])
		{
			ent->noise_index = G_SoundIndex(s);
		}
		else
		{
			ent->noise_index = 0;
		}
	}

	G_SpawnInt("usetime", "0", &ent->genericValue7);

	//For siege gametype
	G_SpawnInt("siegetrig", "0", &ent->genericValue1);
    G_SpawnInt("teambalance", "0", &ent->genericValue2);

	G_SpawnInt("delay", "0", &ent->delay);

	if ( (ent->wait > 0) && (ent->random >= ent->wait) ) {
		ent->random = ent->wait - FRAMETIME;
		Com_Printf(S_COLOR_YELLOW"trigger_multiple has random >= wait\n");
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

	ent->touch = Touch_Multi;
	ent->use   = Use_Multi;

	if ( ent->team && ent->team[0] )
	{
		ent->alliedTeam = atoi(ent->team);
		ent->team = NULL;
	}

	InitTrigger( ent );
	trap->LinkEntity ((sharedEntity_t *)ent);
}


/*QUAKED trigger_once (.5 1 .5) ? CLIENTONLY FACING USE_BUTTON FIRE_BUTTON x x x INACTIVE MULTIPLE
CLIENTONLY - only a player can trigger this by touch
FACING - Won't fire unless triggering ent's view angles are within 45 degrees of trigger's angles (in addition to any other conditions)
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
FIRE_BUTTON - Won't fire unless player/NPC is in it and pressing fire button (in addition to any other conditions)
INACTIVE - Start off, has to be activated to be touchable/usable
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"random"	wait variance, default is 0
"delay"		how many seconds to wait to fire targets after tripped
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
"noise"		Sound to play when the trigger fires (plays at activator's origin)
"NPC_targetname"  Only the NPC with this NPC_targetname fires this trigger

"team" - If set, only this team can trip this trigger
	0 - any
	1 - red
	2 - blue

"soundSet"	Ambient sound set to play when this trigger is activated

usetime		-	If specified (in milliseconds) along with the USE_BUTTON flag, will
				require a client to hold the use key for x amount of ms before firing.

Applicable only during Siege gametype:
teamuser - if 1, team 2 can't use this. If 2, team 1 can't use this.
siegetrig - if non-0, can only be activated by players carrying a misc_siege_item
			which is associated with this trigger by the item's goaltarget value.

idealclass	-	Can only be used by this class/these classes. You can specify use by
				multiple classes with the use of |, e.g.:
				"Imperial Medic|Imperial Assassin|Imperial Demolitionist"
*/
void SP_trigger_once( gentity_t *ent ) 
{
	char	*s;
	if ( G_SpawnString( "noise", "", &s ) ) 
	{
		if (s && s[0])
		{
			ent->noise_index = G_SoundIndex(s);
		}
		else
		{
			ent->noise_index = 0;
		}
	}

	G_SpawnInt("usetime", "0", &ent->genericValue7);

	//For siege gametype
	G_SpawnInt("siegetrig", "0", &ent->genericValue1);

	G_SpawnInt("delay", "0", &ent->delay);

	ent->wait = -1;

	ent->touch = Touch_Multi;
	ent->use   = Use_Multi;

	if ( ent->team && ent->team[0] )
	{
		ent->alliedTeam = atoi(ent->team);
		ent->team = NULL;
	}

	ent->delay *= 1000;//1 = 1 msec, 1000 = 1 sec

	InitTrigger( ent );
	trap->LinkEntity ((sharedEntity_t *)ent);
}

/*
======================================================================
trigger_lightningstrike -rww
======================================================================
*/
//lightning strike trigger lightning strike event
void Do_Strike(gentity_t *ent)
{
	trace_t localTrace;
	vec3_t strikeFrom;
	vec3_t strikePoint;
	vec3_t fxAng;
	
	//maybe allow custom fx direction at some point?
	VectorSet(fxAng, 90.0f, 0.0f, 0.0f);

	//choose a random point to strike within the bounds of the trigger
	strikePoint[0] = flrand(ent->r.absmin[0], ent->r.absmax[0]);
	strikePoint[1] = flrand(ent->r.absmin[1], ent->r.absmax[1]);

	//consider the bottom mins the ground level
	strikePoint[2] = ent->r.absmin[2];

	//set the from point
	strikeFrom[0] = strikePoint[0];
	strikeFrom[1] = strikePoint[1];
	strikeFrom[2] = ent->r.absmax[2]-4.0f;

	//now trace for damaging stuff, and do the effect
	trap->Trace(&localTrace, strikeFrom, NULL, NULL, strikePoint, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);
	VectorCopy(localTrace.endpos, strikePoint);

	if (localTrace.startsolid || localTrace.allsolid)
	{ //got a bad spot, think again next frame to try another strike
		ent->nextthink = level.time;
		return;
	}

	if (ent->radius)
	{ //do a radius damage at the end pos
		G_RadiusDamage(strikePoint, ent, ent->damage, ent->radius, ent, NULL, MOD_SUICIDE);
	}
	else
	{ //only damage individuals
		gentity_t *trHit = &g_entities[localTrace.entityNum];

		if (trHit->inuse && trHit->takedamage)
		{ //damage it then
			G_Damage(trHit, ent, ent, NULL, trHit->r.currentOrigin, ent->damage, 0, MOD_SUICIDE);
		}
	}

	G_PlayEffectID(ent->genericValue2, strikeFrom, fxAng);
}

//lightning strike trigger think loop
void Think_Strike(gentity_t *ent)
{
	if (ent->genericValue1)
	{ //turned off currently
		return;
	}

	ent->nextthink = level.time + ent->wait + Q_irand(0, ent->random);
	Do_Strike(ent);
}

//lightning strike trigger use event function
void Use_Strike( gentity_t *ent, gentity_t *other, gentity_t *activator ) 
{
	ent->genericValue1 = !ent->genericValue1;

	if (!ent->genericValue1)
	{ //turn it back on
		ent->nextthink = level.time;
	}
}

/*QUAKED trigger_lightningstrike (.1 .5 .1) ? START_OFF
START_OFF - start trigger disabled

"lightningfx"	effect to use for lightning, MUST be specified
"wait"			Seconds between strikes, 1000 default
"random"		wait variance, default is 2000
"dmg"			damage on strike (default 50)
"radius"		if non-0, does a radius damage at the lightning strike
				impact point (using this value as the radius). otherwise
				will only do line trace damage. default 0.

use to toggle on and off
*/
void SP_trigger_lightningstrike( gentity_t *ent ) 
{
	char *s;

	ent->use = Use_Strike;
	ent->think = Think_Strike;
	ent->nextthink = level.time + 500;

	G_SpawnString("lightningfx", "", &s);
	if (!s || !s[0])
	{
		Com_Error(ERR_DROP, "trigger_lightningstrike with no lightningfx");
	}

	//get a configstring index for it
	ent->genericValue2 = G_EffectIndex(s);

	if (ent->spawnflags & 1)
	{ //START_OFF
		ent->genericValue1 = 1;
	}

	if (!ent->wait)
	{ //default 1000
		ent->wait = 1000;
	}
	if (!ent->random)
	{ //default 2000
		ent->random = 2000;
	}
	if (!ent->damage)
	{ //default 50
		ent->damage = 50;
	}

	InitTrigger( ent );
	trap->LinkEntity ((sharedEntity_t *)ent);
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

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = trigger_always_think;
}


/*
==============================================================================

trigger_push

==============================================================================
*/
//trigger_push
#define PUSH_LINEAR		4
#define PUSH_RELATIVE	16
#define PUSH_MULTIPLE	2048
//target_push
#define PUSH_CONSTANT	2

void trigger_push_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( !(self->spawnflags&PUSH_LINEAR) )
	{//normal throw
		if ( !other->client ) {
			return;
		}
		BG_TouchJumpPad( &other->client->ps, &self->s );
		return;
	}

	//linear
	if( level.time < self->painDebounceTime + self->wait  ) // normal 'wait' check
	{
		if( self->spawnflags & PUSH_MULTIPLE ) // MULTIPLE - allow multiple entities to touch this trigger in one frame
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

	/*
	//???
	// if the player has already activated this trigger this frame
	if( other && !other->s.number && self->aimDebounceTime == level.time )
	{
		return;		
	}
	*/
	
	/*
	if( self->spawnflags & PUSH_CONVEYOR )
	{   // only push player if he's on the ground
		if( other->s.groundEntityNum == ENTITYNUM_NONE )
		{
			return;
		}
	}
	*/

	/*
	if ( self->spawnflags & 1 )
	{//PLAYERONLY
		if ( other->s.number >= MAX_CLIENTS )
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
	*/

	if ( !other->client ) {
		if ( other->s.pos.trType != TR_STATIONARY && other->s.pos.trType != TR_LINEAR_STOP && other->s.pos.trType != TR_NONLINEAR_STOP && VectorLengthSquared( other->s.pos.trDelta ) )
		{//already moving
			VectorCopy( other->r.currentOrigin, other->s.pos.trBase );
			VectorCopy( self->s.origin2, other->s.pos.trDelta );
			other->s.pos.trTime = level.time;
		}
		return;
	}

	if ( other->client->ps.pm_type != PM_NORMAL 
		&& other->client->ps.pm_type != PM_DEAD 
		&& other->client->ps.pm_type != PM_FREEZE ) 
	{
		return;
	}
	
	if ( (self->spawnflags&PUSH_RELATIVE) )
	{//relative, dir to it * speed
		vec3_t dir;
		VectorSubtract( self->s.origin2, other->r.currentOrigin, dir );
		if ( self->speed )
		{
			VectorNormalize( dir );
			VectorScale( dir, self->speed, dir );
		}
		VectorCopy( dir, other->client->ps.velocity );
	}
	else if ( (self->spawnflags&PUSH_LINEAR) )
	{//linear dir * speed
		VectorScale( self->s.origin2, self->speed, other->client->ps.velocity );
	}
	else
	{
		VectorCopy( self->s.origin2, other->client->ps.velocity );
	}
	//so we don't take damage unless we land lower than we start here...
	/*
	other->client->ps.forceJumpZStart = 0;
	other->client->ps.pm_flags |= PMF_TRIGGER_PUSHED;//pushed by a trigger
	other->client->ps.jumpZStart = other->client->ps.origin[2];
	*/

	if ( self->wait == -1 )
	{
		self->touch = NULL;
	}
	else if ( self->wait > 0 )
	{
		self->painDebounceTime = level.time;
		
	}
	/*
	if( other && !other->s.number )
	{	// mark that the player has activated this trigger this frame
		self->aimDebounceTime =level.time;
	}
	*/
}


/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget( gentity_t *self ) {
	gentity_t	*ent;
	vec3_t		origin;
	float		height, gravity, time, forward;
	float		dist;

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale ( origin, 0.5f, origin );

	ent = G_PickTarget( self->target );
	if ( !ent ) {
		G_FreeEntity( self );
		return;
	}

	if ( self->classname && !Q_stricmp( "trigger_push", self->classname ) )
	{
		if ( (self->spawnflags&PUSH_RELATIVE) )
		{//relative, not an arc or linear
			VectorCopy( ent->r.currentOrigin, self->s.origin2 );
			return;
		}
		else if ( (self->spawnflags&PUSH_LINEAR) )
		{//linear, not an arc
			VectorSubtract( ent->r.currentOrigin, origin, self->s.origin2 );
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
	gravity = g_gravity.value;
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


/*QUAKED trigger_push (.5 .5 .5) ? x x LINEAR x RELATIVE x x INACTIVE MULTIPLE
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push

LINEAR - Instead of tossing the client at the target_position, it will push them towards it.  Must set a "speed" (see below)
RELATIVE - instead of pushing you in a direction that is always from the center of the trigger to the target_position, it pushes *you* toward the target position, relative to your current location (can use with "speed"... if don't set a speed, it will use the distance from you to the target_position)
INACTIVE - not active until targeted by a target_activate
MULTIPLE - multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

wait - how long to wait between pushes: -1 = push only once
speed - when used with the LINEAR spawnflag, pushes the client toward the position at a constant speed (default is 1000)
*/
void SP_trigger_push( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/weapons/force/jump.wav");

	self->s.eType = ET_PUSH_TRIGGER;
	
	if ( !(self->spawnflags&2) )
	{//start on
		self->touch = trigger_push_touch;
	}

	if ( self->spawnflags & 4 )
	{//linear
		self->speed = 1000;
	}

	self->think = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap->LinkEntity ((sharedEntity_t *)self);
}

void Use_target_push( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL && activator->client->ps.pm_type != PM_FLOAT && activator->client->ps.pm_type != PM_FREEZE) { //freeze too?
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	VectorCopy (self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if ( activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		if (self->noise_index)
		{
			G_Sound( activator, CHAN_AUTO, self->noise_index );
		}
	}
}

qboolean ValidRaceSettings(int restrictions, gentity_t *player)
{ //How 2 check if cvars were valid the whole time of run.. and before? since you can get a headstart with higher g_speed before hitting start timer? :S
	//Make most of this hardcoded into racemode..? speed, knockback, debugmelee, stepslidefix, gravity
	int style;
	if (!player->client)
		return qfalse;
	if (!player->client->ps.stats[STAT_RACEMODE])
		return qfalse;

	style = player->client->sess.movementStyle;

	if (style == MV_JETPACK)
		return qfalse;//temp

	if (player->client->sess.accountFlags & JAPRO_ACCOUNTFLAG_NORACE)
		return qfalse;
	if (((style == MV_RJQ3) || (style == MV_RJCPM)) && g_knockback.value != 1000.0f)
		return qfalse;
	if (style != MV_CPM && style != MV_Q3 && style != MV_WSW && style != MV_RJQ3 && style != MV_RJCPM && style != MV_JETPACK && style != MV_SWOOP && style != MV_JETPACK && style != MV_SLICK && style != MV_BOTCPM) { //Ignore forcejump restrictions if in onlybhop movement modes
		if (restrictions & (1 << 0)) {//flags 1 = restrict to jump1
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 1 || player->client->ps.powerups[PW_YSALAMIRI] > 0) {
				trap->SendServerCommand( player-g_entities, "cp \"^3Warning: this course requires force jump level 1!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
		else if (restrictions & (1 << 1)) {//flags 2 = restrict to jump2
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 2 || player->client->ps.powerups[PW_YSALAMIRI] > 0) {
				trap->SendServerCommand( player-g_entities, "cp \"^3Warning: this course requires force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
		else if (restrictions & (1 << 2)) {//flags 4 = only jump3
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 3 || player->client->ps.powerups[PW_YSALAMIRI] > 0) { //Also dont allow ysal in FJ specified courses..?
				trap->SendServerCommand( player-g_entities, "cp \"^3Warning: this course requires force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
	}
	if (player->client->pers.haste && !(restrictions & (1 << 3))) 
		return qfalse; //IF client has haste, and the course does not allow haste, dont count it.
	if ((style != MV_JETPACK) && (player->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)) && !(restrictions & (1 << 4))) //kinda deprecated.. maybe just never allow jetpack?
		return qfalse; //IF client has jetpack, and the course does not allow jetpack, dont count it.
	if (style == MV_SWOOP && !player->client->ps.m_iVehicleNum)
		return qfalse;
	if (sv_cheats.integer)
		return qfalse;
	if (!g_stepSlideFix.integer)
		return qfalse;
	if (g_jediVmerc.integer) //umm..
		return qfalse;
	if (g_debugMelee.integer >= 2)
		return qfalse;
	if (!g_smoothClients.integer)
		return qfalse;
	if (sv_fps.integer != 20 && sv_fps.integer != 30 && sv_fps.integer != 40)//Dosnt really make a difference.. but eh.... loda fixme
		return qfalse;
	if (sv_pluginKey.integer > 0) {//RESTRICT PLUGIN
		if (!player->client->pers.validPlugin && player->client->pers.userName[0]) { //Meh.. only do this if they are logged in to keep the print colors working right i guess..
			trap->SendServerCommand( player-g_entities, "cp \"^3Warning: a newer client plugin version\nis required!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
			return qfalse;
		}
	}
	if (player->client->pers.noFollow)
		return qfalse;
	if (player->client->pers.practice)
		return qfalse;
	if ((restrictions & (1 << 5)) && (level.gametype == GT_CTF || level.gametype == GT_CTY))//spawnflags 32 is FFA_ONLY
		return qfalse;
	if ((player->client->ps.stats[STAT_RESTRICTIONS] & JAPRO_RESTRICT_ALLOWTELES) && !(restrictions & (1 << 6))) //spawnflags 64 is allow_teles 
		return qfalse;

	return qtrue;
}

qboolean InTrigger(vec3_t interpOrigin, gentity_t *trigger)
{
	vec3_t		mins, maxs;
	static const vec3_t	pmins = {-15, -15, DEFAULT_MINS_2};
	static const vec3_t	pmaxs = {15, 15, DEFAULT_MAXS_2};

	VectorAdd( interpOrigin, pmins, mins );
	VectorAdd( interpOrigin, pmaxs, maxs );

	if (trap->EntityContact(mins, maxs, (sharedEntity_t *)trigger, qfalse))
		return qtrue;//Player is touching the trigger
	return qfalse;//Player is not touching the trigger
}

int InterpolateTouchTime(gentity_t *activator, gentity_t *trigger)
{ //We know that last client frame, they were not touching the flag, but now they are.  Last client frame was pmoveMsec ms ago, so we only want to interp inbetween that range.
	vec3_t	interpOrigin, delta;
	int lessTime = -1;

	qboolean touched = qfalse;
	qboolean inTrigger;

	VectorCopy(activator->client->ps.origin, interpOrigin);
	VectorScale(activator->s.pos.trDelta, 0.001f, delta);//Delta is how much they travel in 1 ms.

	//VectorSubtract(interpOrigin, delta, interpOrigin);//Do it once before we loop

	//We know that..we should start in the trigger.  If not, we probably are on the other side of the trigger and 'warped' past it.  
	//So if we start outside of trigger, we should keep going until we hit trigger then keep going until we exit it again.
	while ((inTrigger = InTrigger(interpOrigin, trigger)) || !touched) {//This will be done a max of pml.msec times, in theory, before we are guarenteed to not be in the trigger anymore.
		if (inTrigger)
			touched = qtrue;

		lessTime++; //Add one more ms to be subtracted
		VectorSubtract(interpOrigin, delta, interpOrigin); //Keep Rewinding position by a tiny bit, that corresponds with 1ms precision (delta*0.001), since delta is per second.
		//if (lessTime >= activator->client->pmoveMsec || lessTime >= 8) { //activator->client->pmoveMsec
		if (lessTime >= 250) {
			break; //In theory, this should never happen, but just incase stop it here.
		}
	}

	//trap->SendServerCommand( -1, va("chat \"Subtracted %i milliseconds with interp\"", lessTime) );

	return lessTime;
}

static QINLINE int GetTimeMS() {
 //return level.time;
 return trap->Milliseconds();

 /*
 	regarding precision:
	currently, it assumes that client touch trigger checks are done every 8ms.  it might require the client to have atleast 125fps for this to happen though.  also, does maxpackets affect this?
	then, interpolation is done on the touch time to get it down to 1ms precision.

	if touch trigger checks were only done every server frame, this should use level.time, and interpolation would take it from 1000/sv_fps precision down to 1ms precision,
	but that doesnt seem to be the case.

	trace triggers shouldnt really affect this, atleast not in an abusable way.
	*/

}

//void G_SoundPrivate( gentity_t *ent, int channel, int soundIndex );
void G_UpdatePlaytime(int null, char *username, int seconds );
void TimerStart(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JAPRO Timers
	int lessTime;

	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (player->s.eType == ET_NPC)
		return;
	if (player->client->ps.duelInProgress)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE && player->client->ps.pm_type != PM_JETPACK) //Allow racemode emotes?
		return;
	if (player->client->sess.raceMode && player->client->sess.movementStyle == MV_SWOOP && !player->client->ps.m_iVehicleNum) //Dont start the timer for swoop racers if they dont have a swoop
		return;
	if (player->client->pers.stats.lastResetTime == level.time) //Dont allow a starttimer to start in the same frame as a resettimer (called from noclip or amtele)
		return;
	if (level.time - player->client->lastInStartTrigger <= 300) { //We were last in the trigger within 300ms ago.., //goal, make this negative edge ?
		player->client->lastInStartTrigger = level.time;
		return;
	}
	player->client->lastInStartTrigger = level.time;

	//if (GetTimeMS() - player->client->pers.stats.startTime < 500)//Some built in floodprotect per player?
		//return;
	//if (player->client->pers.stats.startTime) //Instead of floodprotect, dont let player start a timer if they already have one.  Mapmakers should then put reset timers over the start area.
		//return;


	//trap->Print("Actual trigger touch! time: %i\n", GetTimeMS());

	if (player->client->pers.recordingDemo && player->client->pers.keepDemo) {
		//We are still recording a demo that we want to keep? -shouldn't ever happen?
		//Stop and rename it
		//trap->SendServerCommand( player-g_entities, "chat \"RECORDING STOPPED (at startline), HIGHSCORE\"");
		trap->SendConsoleCommand( EXEC_APPEND, va("svstoprecord %i;wait 10;svrenamedemo temp/%s races/%s\n", player->client->ps.clientNum, player->client->pers.oldDemoName, player->client->pers.demoName));
		player->client->pers.recordingDemo = qfalse;
		player->client->pers.demoStoppedTime = level.time;
	}

	//in rename demo, also make sure demo is stopped before renaming? that way we dont have to have the ;wait 20; here

	if ((sv_autoRaceDemo.integer) && !(player->client->pers.noFollow) && !(player->client->pers.practice) && player->client->sess.raceMode && !sv_cheats.integer && player->client->pers.userName[0]) {
		if (!player->client->pers.recordingDemo) { //Start the new demo
			player->client->pers.recordingDemo = qtrue;
			//trap->SendServerCommand( player-g_entities, "chat \"RECORDING STARTED\"");
			trap->SendConsoleCommand( EXEC_APPEND, va("svrecord temp/%s %i\n", player->client->pers.userName, player->client->ps.clientNum));
		}
		else { //Check if we should "restart" the demo
			//if (!player->client->lastStartTime || (level.time - player->client->lastStartTime > 5000)) {
			if (!player->client->pers.stats.startTime || (GetTimeMS() - player->client->pers.stats.startTime > 5000)) { //we can just use starttime ?

				player->client->pers.recordingDemo = qtrue;
				player->client->pers.demoStoppedTime = level.time;
				//trap->SendServerCommand( player-g_entities, "chat \"RECORDING RESTARTED\"");
				trap->SendConsoleCommand( EXEC_APPEND, va("svstoprecord %i;wait 5;svrecord temp/%s %i\n", player->client->ps.clientNum, player->client->pers.userName, player->client->ps.clientNum));
				//trap->SendConsoleCommand( EXEC_APPEND, va("svrecord temp/%s %i\n", player->client->pers.userName, player->client->ps.clientNum));
			}
		}
	}

	//player->client->lastStartTime = level.time;
	player->client->pers.keepDemo = qfalse;

	if (player->client->ps.m_iVehicleNum) { //We are in a vehicle, so use the trigger on that instead maybe? or both..
		gentity_t *ourVeh = &g_entities[player->client->ps.m_iVehicleNum];	
		if (ourVeh->inuse && ourVeh->client) //just to be safe
			multi_trigger(trigger, ourVeh);
	}

	multi_trigger(trigger, player); //Let it have a target, so it can point to restricts.  Move this up here, so swoops can activate it for proper swoop teleporting?

	if (trigger->noise_index) //Only play on leaving trigger..?
		G_RaceSound( player, CHAN_AUTO, trigger->noise_index, RS_TIMER_START );//could just use player instead of trigger->activator ?   How do we make this so only the activator hears it?

	if (trigger->spawnflags & 1) {//set speed to speed value, keep our direction the same
		vec3_t hVel;
		hVel[0] = player->client->ps.velocity[0];
		hVel[1] = player->client->ps.velocity[1];
		hVel[2] = 0;
		VectorScale(player->client->ps.velocity, (trigger->speed * (player->client->ps.basespeed / 250.0f)) / VectorLength(hVel), player->client->ps.velocity);
	}

	player->client->pers.startLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime; //use level.previousTime?
	//trap->SendServerCommand( player-g_entities, va("chat \"startlag: %i\"", player->client->pers.startLag));

	/*
	//fixme? 
	so we need to get StartLag, defined by the difference between level.time and cmd.servertime.  i dont think it should be related to trap->milliseconds.
	level.frameStartTime is just trap_milliseconds at the start of the frame.
	level.time is just incrempted by a constant every frame (1000 / sv_fps->integer)
	how often is pers.cmd.serverTime updated? every client frame? maybe it should use trap->milliseconds then.. :/
	*/

	//Update playtime if needed
	if (player->client->sess.raceMode && !player->client->pers.practice && player->client->pers.userName[0] && player->client->pers.stats.startTime) {
		player->client->pers.stats.racetime += (GetTimeMS() - player->client->pers.stats.startTime)*0.001f - player->client->afkDuration*0.001f;
		player->client->afkDuration = 0;
		if (player->client->pers.stats.racetime > 120.0f) { //Avoid spamming the db
			G_UpdatePlaytime(0, player->client->pers.userName, (int)(player->client->pers.stats.racetime+0.5f));
			player->client->pers.stats.racetime = 0.0f;
		}
	}

	lessTime = InterpolateTouchTime(player, trigger);

	if (player->client->ps.stats[STAT_RACEMODE]) {
		player->client->ps.duelTime = level.time - lessTime;
		player->client->ps.stats[STAT_HEALTH] = player->health = player->client->ps.stats[STAT_MAX_HEALTH];
		player->client->ps.stats[STAT_ARMOR] = 25;

		if ((GetTimeMS() - player->client->pers.stats.startTime > 2000)) {
			//Floodprotect the prints
			if (!player->client->pers.userName[0]) //In racemode but not logged in
				trap->SendServerCommand(player-g_entities, "cp \"^3Warning: You are not logged in!\n\n\n\n\n\n\n\n\n\n\"");
			else if (player->client->pers.noFollow)
				trap->SendServerCommand( player-g_entities, "cp \"^3Warning: times are not valid while hidden!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
			else if (player->client->pers.practice)
				trap->SendServerCommand( player-g_entities, "cp \"^3Warning: times are not valid in practice mode!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
		}
	}

	player->client->pers.stats.startLevelTime = level.time; //Should this use trap milliseconds instead.. 
	player->client->pers.stats.startTime = GetTimeMS() - lessTime;
	player->client->pers.stats.topSpeed = 0;
	player->client->pers.stats.displacement = 0;
	player->client->pers.stats.displacementSamples = 0;

	if (player->client->ps.stats[STAT_RESTRICTIONS] & JAPRO_RESTRICT_ALLOWTELES) { //Reset their telemark on map start if this is the case
		player->client->pers.telemarkOrigin[0] = 0;
		player->client->pers.telemarkOrigin[1] = 0;
		player->client->pers.telemarkOrigin[2] = 0;
		player->client->pers.telemarkAngle = 0;
	}

	//if (player->r.svFlags & SVF_JUNIORADMIN)
		//trap->SendServerCommand(player-g_entities, va("cp \"Starting lag: %i\n 2: %i\n 3: %i\n\"", player->client->pers.startLag, level.time - player->client->pers.cmd.serverTime, GetTimeMS() - player->client->pers.cmd.serverTime));

}

void PrintRaceTime(char *username, char *playername, char *message, char *style, int topspeed, int average, char *timeStr, int clientNum, int season_newRank, qboolean spb, int global_newRank, qboolean loggedin, qboolean valid, int season_oldRank, int global_oldRank, float addedScore, int awesomenoise);
void IntegerToRaceName(int style, char *styleString, size_t styleStringSize);
void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMS);
void G_AddRaceTime(char *account, char *courseName, int duration_ms, int style, int topspeed, int average, int clientNum, int awesomenoise); //should this be extern?
void TimerStop(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JAPRO Timers
	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (player->s.eType == ET_NPC)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE && player->client->ps.pm_type != PM_JETPACK) 
		return;

	multi_trigger(trigger, player);

	if (player->client->pers.stats.startTime) {
		char styleStr[32] = {0}, timeStr[32] = {0}, playerName[MAX_NETNAME] = {0};
		char c[4] = S_COLOR_RED;
		float time = GetTimeMS() - player->client->pers.stats.startTime;
		int average, restrictions = 0, nameColor = 7;
		qboolean valid = qfalse;
		const int endLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime;
		const int diffLag = player->client->pers.startLag - endLag;
		const int lessTime = InterpolateTouchTime(player, trigger);

		if (diffLag > 0) {//Should this be more trusting..?.. -20? -30?
			time += diffLag;
		}
		//else 
			//time -= 10; //Clients time was massively fucked due to lag, improve it up the minimum ammount..

		//if (player->client->sess.fullAdmin) //think we can finally remove this debug print
			//trap->SendServerCommand( player-g_entities, va("chat \"Msec diff due to warp (added if > 0): %i\"", diffLag));
		
		//trap->SendServerCommand( player-g_entities, va("chat \"diffLag: %i\"", diffLag));

		if (lessTime < 16) //Don't really trust this yet, max possible is 250.
			time -= lessTime;

		time /= 1000.0f;
		if (time < 0.001f)
			time = 0.001f;
		//average = floorf(player->client->pers.stats.displacement / ((level.time - player->client->pers.stats.startLevelTime) * 0.001f)) + 0.5f;//Should use level time for this 
		if (player->client->pers.stats.displacementSamples)
			average = floorf(((player->client->pers.stats.displacement * sv_fps.value) / player->client->pers.stats.displacementSamples) + 0.5f);
		else 
			average = player->client->pers.stats.topSpeed;

		if (trigger->spawnflags)//Get the restrictions for the specific course (only allow jump1, or jump2, etc..)
			restrictions = trigger->spawnflags;

		if (ValidRaceSettings(restrictions, player)) {
			valid = qtrue;
			if (player->client->pers.userName && player->client->pers.userName[0])
				Q_strncpyz( c, S_COLOR_CYAN, sizeof(c) );
			else
				Q_strncpyz( c, S_COLOR_GREEN, sizeof(c) );
		}

		/*
		if (valid && (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_JKA) && trigger->awesomenoise_index && (time <= trigger->speed)) //Play the awesome noise if they were fast enough
			G_Sound(player, CHAN_AUTO, trigger->awesomenoise_index);//Just play it in jka physics for now...
		else*/
		if (trigger->noise_index) //Still play this always? Or handle this later..
			G_Sound(player, CHAN_AUTO, trigger->noise_index);

		//Pass awesomenoise_index through to play it if its a PB
	
		IntegerToRaceName(player->client->ps.stats[STAT_MOVEMENTSTYLE], styleStr, sizeof(styleStr));
		TimeToString((int)(time*1000), timeStr, sizeof(timeStr), qfalse);
		Q_strncpyz(playerName, player->client->pers.netname, sizeof(playerName));
		Q_StripColor(playerName);
	
		if (!valid) {
			PrintRaceTime(NULL, playerName, trigger->message, styleStr, (int)(player->client->pers.stats.topSpeed + 0.5f), average, timeStr, player->client->ps.clientNum, qfalse, qfalse, qfalse, qfalse, qfalse, 0, 0, 0, 0);
		}
		else {
			char strIP[NET_ADDRSTRMAXLEN] = {0};
			char *p = NULL;
			Q_strncpyz(strIP, player->client->sess.IP, sizeof(strIP));
			p = strchr(strIP, ':');
			if (p)
				*p = 0;
			if (player->client->pers.userName[0]) { //omg
				G_AddRaceTime(player->client->pers.userName, trigger->message, (int)(time*1000), player->client->ps.stats[STAT_MOVEMENTSTYLE], (int)(player->client->pers.stats.topSpeed + 0.5f), average, player->client->ps.clientNum, trigger->awesomenoise_index);
			}
			else {
				PrintRaceTime(NULL, playerName, trigger->message, styleStr, (int)(player->client->pers.stats.topSpeed + 0.5f), average, timeStr, player->client->ps.clientNum, qfalse, qfalse, qfalse, qfalse, qtrue, 0, 0, 0, 0);
			}
		}

		player->client->pers.stats.startLevelTime = 0;
		player->client->pers.stats.startTime = 0;
		player->client->pers.stats.topSpeed = 0;
		player->client->pers.stats.displacement = 0;
		if (player->client->ps.stats[STAT_RACEMODE])
			player->client->ps.duelTime = 0;
	}
}

void TimerCheckpoint(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JAPRO Timers
	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (player->s.eType == ET_NPC)
		return;
	if  (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE && player->client->ps.pm_type != PM_JETPACK)
		return;
	if (player->client->pers.stats.startTime && trigger && trigger->spawnflags & 2) { //Instead of a checkpoint, make it reset their time (they went out of bounds or something)
		player->client->pers.stats.startTime = 0;
		if (player->client->sess.raceMode)
			player->client->ps.duelTime = 0;
		trap->SendServerCommand( player-g_entities, "cp \"Timer reset\n\n\n\n\n\n\n\n\n\n\""); //Send message?
		return;
	}

	if (player->client->pers.stats.startTime && (level.time - player->client->pers.stats.lastCheckpointTime > 1000)) { //make this more accurate with interp? or dosnt really matter ...
		int i;
		int time = GetTimeMS() - player->client->pers.stats.startTime;
		const int endLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime;
		const int diffLag = player->client->pers.startLag - endLag;
		int average;
		//const int average = floorf(player->client->pers.stats.displacement / ((level.time - player->client->pers.stats.startLevelTime) * 0.001f)) + 0.5f; //Could this be more accurate?
		if (player->client->pers.stats.displacementSamples)
			average = floorf(((player->client->pers.stats.displacement * sv_fps.value) / player->client->pers.stats.displacementSamples) + 0.5f);
		else
			average = player->client->pers.stats.topSpeed;

		if (diffLag > 0) {//Should this be more trusting..?.. -20? -30?
			time += diffLag;
		}
		//else 
			//time -= 10; //Clients time was massively fucked due to lag, improve it up the minimum ammount..

		if (time < 1)
			time = 1;

		/*
		if (trigger && trigger->spawnflags & 1)//Minimalist print loda fixme get rid of target shit 
			trap->SendServerCommand( player-g_entities, va("cp \"^3%.3fs^5, avg ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average));
		else
			trap->SendServerCommand( player-g_entities, va("chat \"^5Checkpoint: ^3%.3f^5, max ^3%i^5, average ^3%i^5 ups\"", (float)time * 0.001f, player->client->pers.stats.topSpeed, average));
			*/

		if (player->client->pers.showCenterCP)
			trap->SendServerCommand( player-g_entities, va("cp \"^3%.3fs^5, avg ^3%i^5u, max ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average, (int)(player->client->pers.stats.topSpeed + 0.5f)));
		if (player->client->pers.showConsoleCP)
			trap->SendServerCommand(player - g_entities, va("print \"^5Checkpoint: ^3%.3f^5, avg ^3%i^5, max ^3%i^5 ups\n\"", (float)time * 0.001f, average, (int)(player->client->pers.stats.topSpeed + 0.5f)));
		else if (player->client->pers.showChatCP)
			trap->SendServerCommand( player-g_entities, va("chat \"^5Checkpoint: ^3%.3f^5, avg ^3%i^5, max ^3%i^5 ups\"", (float)time * 0.001f, average, (int)(player->client->pers.stats.topSpeed + 0.5f)));
		
		for (i=0; i<MAX_CLIENTS; i++) {//Also print to anyone spectating them..
			if (!g_entities[i].inuse)
				continue;
			if ((level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) && (level.clients[i].ps.pm_flags & PMF_FOLLOW) && (level.clients[i].sess.spectatorClient == player->client->ps.clientNum))
			{
				//if (trigger && trigger->spawnflags & 1)//Minimalist print loda fixme get rid of target shit 
				if (level.clients[i].pers.showCenterCP)
					trap->SendServerCommand( i, va("cp \"^3%.3fs^5, avg ^3%i^5u, max ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average, (int)(player->client->pers.stats.topSpeed + 0.5f)));
				if (level.clients[i].pers.showChatCP)
					trap->SendServerCommand( i, va("chat \"^5Checkpoint: ^3%.3f^5, avg ^3%i^5, max ^3%i^5 ups\"", (float)time * 0.001f, average, (int)(player->client->pers.stats.topSpeed + 0.5f)));
			}
		}

		player->client->pers.stats.lastCheckpointTime = level.time; //For built in floodprotect
	}
}

void Use_target_restrict_on(gentity_t *trigger, gentity_t *other, gentity_t *player) {//JAPRO OnlyBhop
	if (!player->client)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;

	if (trigger->spawnflags & RESTRICT_FLAG_HASTE) {
		if (!player->client->pers.haste)
			G_Sound( player, CHAN_AUTO, G_SoundIndex("sound/player/boon.mp3") );
			//G_AddEvent( player, EV_ITEM_PICKUP, 98 ); //100 shield sound i guess, Now why wont the boon sound work?
		player->client->pers.haste = qtrue;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_FLAGS) { //Reset flags
		if (player->client->ps.powerups[PW_NEUTRALFLAG]) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			player->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if (player->client->ps.powerups[PW_REDFLAG]) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			player->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if (player->client->ps.powerups[PW_BLUEFLAG]) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			player->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}
	if (trigger->spawnflags & RESTRICT_FLAG_JUMP) { //Change Jump Level with "count" val
		if (trigger->count) { //Set client jump without resetting timer because someone suggested a course that uses different movementstyles for each room.
			const int jumplevel = trigger->count;
			if (jumplevel >= 1 && jumplevel <= 3) {
				if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != jumplevel) {
					player->client->ps.fd.forcePowerLevel[FP_LEVITATION] = jumplevel;
					trap->SendServerCommand(player - g_entities, va("print \"Jumplevel updated (%i).\n\"", jumplevel));
				}
			}
		}
	}
	else if (trigger->spawnflags & RESTRICT_FLAG_MOVESTYLE) { //Change Movementstyle with "count" val
		if (trigger->count) { //Set client movementstyle without resetting timer because someone suggested a course that uses different movementstyles for each room.
			const int style = trigger->count;
			if (style > 0 && style < MV_NUMSTYLES && style != MV_SWOOP) {//idk how deal with swoop here rly, just spawn it ..?
				if (style == MV_JETPACK) {
					player->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_JETPACK);
				}
				else {
					player->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_JETPACK);
				}

				if (style == MV_SPEED) {
					player->client->ps.fd.forcePower = 50;
				}
				else {
					player->client->ps.fd.forcePower = 100; //ok
				}

				player->client->sess.movementStyle = style;
				trap->SendServerCommand(player - g_entities, "print \"Movement style updated.\n\"");//eh?
			}
		}
	}
	if (trigger->spawnflags & RESTRICT_FLAG_YSAL) {//Give Ysal
		if (player->client->ps.powerups[PW_YSALAMIRI] <= 0)
			G_Sound(player, CHAN_AUTO, G_SoundIndex("sound/player/boon.mp3"));
		player->client->ps.powerups[PW_YSALAMIRI] = 99999999;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_CROUCHJUMP) {//hl style crouch jump
		player->client->ps.stats[STAT_RESTRICTIONS] |= JAPRO_RESTRICT_CROUCHJUMP;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_ALLOWTELES) {
		player->client->ps.stats[STAT_RESTRICTIONS] |= JAPRO_RESTRICT_ALLOWTELES;
	}
	if (!trigger->spawnflags) {
		player->client->ps.stats[STAT_RESTRICTIONS] |= JAPRO_RESTRICT_BHOP;
	}
}

void Use_target_restrict_off( gentity_t *trigger, gentity_t *other, gentity_t *player ) {//JAPRO OnlyBhop
	if (!player->client)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;

	if (trigger->spawnflags & RESTRICT_FLAG_YSAL) {//Give Ysal
		player->client->ps.powerups[PW_YSALAMIRI] = 0;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_HASTE) {
		player->client->pers.haste = qfalse;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_CROUCHJUMP) {//hl style crouch jump
		player->client->ps.stats[STAT_RESTRICTIONS] &= ~JAPRO_RESTRICT_CROUCHJUMP;
	}
	if (trigger->spawnflags & RESTRICT_FLAG_ALLOWTELES) {
		player->client->ps.stats[STAT_RESTRICTIONS] &= ~JAPRO_RESTRICT_ALLOWTELES;
	}
	if (!trigger->spawnflags) {
		player->client->ps.stats[STAT_RESTRICTIONS] &= ~JAPRO_RESTRICT_BHOP;
	}
}

void NewPush(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JAPRO Timers
	float scale;

	if (!player->client)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;
	if (player->client->lastBounceTime > level.time - 500)
		return;

	if (trigger->spawnflags & 8) {//PLAYERONLY
		if (player->s.eType == ET_NPC)
			return;
	}
	else if (trigger->spawnflags & 16) {//NPCONLY
		if (player->NPC == NULL)
			return;
	}

	(trigger->speed) ? (scale = trigger->speed) : (scale = 2.0f); //Check for bounds? scale can be negative, that means "bounce".
	player->client->lastBounceTime = level.time;

	if (trigger->noise_index) 
		G_Sound(player, CHAN_AUTO, trigger->noise_index);

	if (trigger->spawnflags & 1) {
		if ((!g_fixSlidePhysics.integer && abs(player->client->lastVelocity[0]) > 350) || (g_fixSlidePhysics.integer && abs(player->client->lastVelocity[0]) > 90))
			player->client->ps.velocity[0] = player->client->lastVelocity[0] * scale;//XVel Relative Scale
	}
	if (trigger->spawnflags & 2) {
		if ((!g_fixSlidePhysics.integer && abs(player->client->lastVelocity[1]) > 350) || (g_fixSlidePhysics.integer && abs(player->client->lastVelocity[1]) > 90))
			player->client->ps.velocity[1] = player->client->lastVelocity[1] * scale;//YVel Relative Scale
	}
	if (trigger->spawnflags & 4) {
		player->client->ps.velocity[2] = player->client->lastVelocity[2] * scale;//ZVel Relative Scale
	}
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad CONSTANT
CONSTANT will push activator in direction of 'target' at constant 'speed'

Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of none
*/
void SP_target_push( gentity_t *self ) {
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		self->noise_index = G_SoundIndex("sound/weapons/force/jump.wav");
	} else {
		self->noise_index = 0;	//G_SoundIndex("sound/misc/windfly.wav");
	}
	if ( self->target ) {
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

void SP_trigger_timer_start( gentity_t *self )
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString( "noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}

	self->touch = TimerStart;
	trap->LinkEntity ((sharedEntity_t *)self);
}

void SP_trigger_timer_checkpoint( gentity_t *self )
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString( "noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}
	self->touch = TimerCheckpoint;
	trap->LinkEntity ((sharedEntity_t *)self);
}

void SP_trigger_timer_stop( gentity_t *self )
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString( "noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}
	if (G_SpawnString( "awesomenoise", "", &s)) {
		if (s && s[0])
			self->awesomenoise_index = G_SoundIndex(s);
		else
			self->awesomenoise_index = 0;
	}

	//For every stop trigger, increment numCourses and put its name in array
	if (self->message && self->message[0]) {
		Q_strncpyz(level.courseName[level.numCourses], self->message, sizeof(level.courseName[0]));
		Q_strlwr(level.courseName[level.numCourses]);
		Q_CleanStr(level.courseName[level.numCourses]);
		level.numCourses++;
	}
	else if (level.numCourses == 0) { //hmmmmmmmmm!
		Q_strncpyz(level.courseName[level.numCourses], "", sizeof(level.courseName[0]));
		level.numCourses = 1;
	}

	self->touch = TimerStop;
	trap->LinkEntity ((sharedEntity_t *)self);
}

void SP_target_restrict(gentity_t *self)//JAPRO Onlybhop
{
	if (self->spawnflags & RESTRICT_FLAG_DISABLE)
		self->use = Use_target_restrict_off;
	else
		self->use = Use_target_restrict_on;
}

void SP_trigger_newpush(gentity_t *self)//JAPRO Newpush
{
	char	*s;
	InitTrigger(self);

	if ( G_SpawnString( "noise", "", &s ) ) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}

	self->touch = NewPush;
	trap->LinkEntity ((sharedEntity_t *)self);
}

void Touch_KOTH( gentity_t *self, gentity_t *other, trace_t *trace ) 
{
	const int nowTime = GetTimeMS();
	int addTime;

	if( !other->client )
		return;
	if (other->s.eType == ET_NPC)
		return;
	if (self->flags & FL_INACTIVE) //set by target_deactivate
		return;
	if (!g_KOTH.integer)
		return;
	if (level.gametype != GT_TEAM)
		return;
	if (other->client->ps.duelInProgress)
		return;
	if (other->client->sess.raceMode)
		return;
	//if (level.startTime > (level.time - 1000*20)) //Dont enable for first 20 seconds of map
		//return;
	if (nowTime - other->client->kothDebounce < 100) {//Some built in floodprotect per player?
		return;
	}
	if (self->radius && ((self->r.currentOrigin[0]-other->client->ps.origin[0]) * (self->r.currentOrigin[0]-other->client->ps.origin[0]) +
		(self->r.currentOrigin[1]-other->client->ps.origin[1]) * (self->r.currentOrigin[1]-other->client->ps.origin[1])) > self->radius*self->radius) {
			return;
	}
	other->client->kothDebounce = nowTime;

	addTime = nowTime - other->client->pers.stats.kothTime;
	if (addTime > 200) {
		//Com_Printf("Add time is %i\n", addTime);
		addTime = 200;
	}

	if (other->client->sess.sessionTeam == TEAM_RED)
		level.kothTime += addTime;
	else if (other->client->sess.sessionTeam == TEAM_BLUE)
		level.kothTime -= addTime;

	other->client->pers.stats.kothTime = nowTime;

	if (level.kothTime > 3000) {
		AddTeamScore(other->s.pos.trBase, TEAM_RED, 1, qfalse);
		//trap->SendServerCommand( -1, "chat \"Red Scored\"");
		CalculateRanks();
		level.kothTime = 0;
	}
	else if (level.kothTime < -3000) {
		AddTeamScore(other->s.pos.trBase, TEAM_BLUE, 1, qfalse);
		//trap->SendServerCommand( -1, "chat \"Blue Scored\"");
		CalculateRanks();
		level.kothTime = 0;
	}

#if 0

// moved to just above multi_trigger because up here it just checks if the trigger is not being touched
// we want it to check any conditions set on the trigger, if one of those isn't met, the trigger is considered to be "cleared"
//	if ( self->e_ThinkFunc == thinkF_trigger_cleared_fire )
//	{//We're waiting to fire our target2 first
//		self->nextthink = level.time + self->speed;
//		return;
//	}

//JAPRO - Serverside - Allow/disallow use button/trigger for duelers - Start
	if (other->client->ps.duelInProgress && (g_allowUseInDuel.integer >= 2))//Loda fixme, make this spawnflags 5 for button? or should it block all triggers?
		return;
	if ((self->spawnflags & 4) && other->client->ps.duelInProgress && !g_allowUseInDuel.integer)
		return;
//JAPRO - Serverside - Allow/disallow use button/trigger for duelers - End

	if ((self->spawnflags & 4) && (other->client->ps.powerups[PW_NEUTRALFLAG] && g_rabbit.integer))
		return;

	if ( self->spawnflags & 1 )
	{
		if ( other->s.eType == ET_NPC )
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

		AngleVectors( other->client->ps.viewangles, forward, NULL, NULL );

		if ( DotProduct( self->movedir, forward ) < 0.5 )
		{//Not Within 45 degrees
			return;
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if( !( other->client->pers.cmd.buttons & BUTTON_USE ) )
		{//not pressing use button
			return;
		}

		if ((other->client->ps.weaponTime > 0 && other->client->ps.torsoAnim != BOTH_BUTTON_HOLD && other->client->ps.torsoAnim != BOTH_CONSOLE1) || other->health < 1 ||
			(other->client->ps.pm_flags & PMF_FOLLOW) || other->client->sess.sessionTeam == TEAM_SPECTATOR ||
			other->client->ps.forceHandExtend != HANDEXTEND_NONE)
		{ //player has to be free of other things to use.
			return;
		}

		if (self->genericValue7)
		{ //we have to be holding the use key in this trigger for x milliseconds before firing
			if (level.gametype == GT_SIEGE &&
				self->idealclass && self->idealclass[0])
			{ //only certain classes can activate it
				if (!other ||
					!other->client ||
					other->client->siegeClass < 0)
				{ //no class
					return;
				}

				if (!G_NameInTriggerClassList(bgSiegeClasses[other->client->siegeClass].name, self->idealclass))
				{ //wasn't in the list
					return;
				}
			}

			if (!G_PointInBounds( other->client->ps.origin, self->r.absmin, self->r.absmax ))
			{
				return;
			}
			else if (other->client->isHacking != self->s.number && other->s.number < MAX_CLIENTS )
			{ //start the hack
				other->client->isHacking = self->s.number;
				VectorCopy(other->client->ps.viewangles, other->client->hackingAngles);
				other->client->ps.hackingTime = level.time + self->genericValue7;
				other->client->ps.hackingBaseTime = self->genericValue7;
				if (other->client->ps.hackingBaseTime > 60000)
				{ //don't allow a bit overflow
					other->client->ps.hackingTime = level.time + 60000;
					other->client->ps.hackingBaseTime = 60000;
				}
				return;
			}
			else if (other->client->ps.hackingTime < level.time)
			{ //finished with the hack, reset the hacking values and let it fall through
				other->client->isHacking = 0; //can't hack a client
				other->client->ps.hackingTime = 0;
			}
			else
			{ //hack in progress
				return;
			}
		}
	}

	if ( self->spawnflags & 8 )
	{//FIRE_BUTTON
		if( !( other->client->pers.cmd.buttons & BUTTON_ATTACK ) &&
			!( other->client->pers.cmd.buttons & BUTTON_ALT_ATTACK ) )
		{//not pressing fire button or altfire button
			return;
		}
	}

	if ( self->radius )
	{
		vec3_t	eyeSpot;

		//Only works if your head is in it, but we allow leaning out
		//NOTE: We don't use CalcEntitySpot SPOT_HEAD because we don't want this
		//to be reliant on the physical model the player uses.
		VectorCopy(other->client->ps.origin, eyeSpot);
		eyeSpot[2] += other->client->ps.viewheight;

		if ( G_PointInBounds( eyeSpot, self->r.absmin, self->r.absmax ) )
		{
			if( !( other->client->pers.cmd.buttons & BUTTON_ATTACK ) &&
				!( other->client->pers.cmd.buttons & BUTTON_ALT_ATTACK ) )
			{//not attacking, so hiding bonus
				/*
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
				*/
				//Not using this, at least not yet.
			}
		}
	}

	if ( self->spawnflags & 4 )
	{//USE_BUTTON
		if (other->client->sess.raceMode && other->client->sess.movementStyle == MV_JETPACK && VectorLengthSquared(other->client->ps.velocity))
			return;
		if (other->client->sess.raceMode && other->client->sess.movementStyle == MV_SWOOP && other->client->ps.m_iVehicleNum)
			return;
		if (other->client->ps.torsoAnim != BOTH_BUTTON_HOLD &&
			other->client->ps.torsoAnim != BOTH_CONSOLE1)
		{
			G_SetAnim( other, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
		}
		else
		{
			other->client->ps.torsoTimer = 500;
		}
		other->client->ps.weaponTime = other->client->ps.torsoTimer;
	}
	
	if ( self->think == trigger_cleared_fire )
	{//We're waiting to fire our target2 first
		self->nextthink = level.time + self->speed;
		return;
	}

#endif

	multi_trigger( self, other );
}

void SP_trigger_KOTH(gentity_t *self)//JAPRO Newpush
{
	//char	*s;
	InitTrigger(self);

	/*
	if ( G_SpawnString( "noise", "", &s ) ) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}
	*/

	Com_Printf("Spawned koth\n");

	self->touch = Touch_KOTH;
	trap->LinkEntity ((sharedEntity_t *)self);
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t	*dest;
	qboolean keepVel = qfalse;

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( !other->client ) {
		return;
	}
	if ( other->client->ps.pm_type == PM_DEAD ) {
		return;
	}
	// Spectators only?
	if ( ( self->spawnflags & 1 ) && 
		other->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return;
	}

	// Racemode only?
	if ((self->spawnflags & 4) && !other->client->sess.raceMode) {
		return;
	}

	dest = 	G_PickTarget( self->target );
	if (!dest) {
		trap->Print ("Couldn't find teleporter destination\n");
		return;
	}

	if (self->spawnflags & 2)
		keepVel = qtrue;

	//Look at dest->speed, and.. dest->spawnflags?   if spawnflags & quake style, multiply their current speed
	//if not spawnflags & quake style, set their speed to specified speed
	//if neither, set their speed to 450 or whatever?


	TeleportPlayer( other, dest->s.origin, dest->s.angles, keepVel );
}


/*QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

If spectator is set, only spectators can use this teleport
Spectator teleporters are not normally placed in the editor, but are created
automatically near doors to allow spectators to move through them
*/
void SP_trigger_teleport( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->spawnflags & 1 ) {
		self->r.svFlags |= SVF_NOCLIENT;
	} else {
		self->r.svFlags &= ~SVF_NOCLIENT;
	}

	// make sure the client precaches this sound
	G_SoundIndex("sound/weapons/force/speed.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = trigger_teleporter_touch;

	trap->LinkEntity ((sharedEntity_t *)self);
}


/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF CAN_TARGET SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

CAN_TARGET		if you target it, it will toggle on and off
SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"team"			team (1 or 2) to allow hurting (if none then hurt anyone) only applicable for siege
"dmg"			default 5 (whole numbers only)
If dmg is set to -1 this brush will use the fade-kill method

*/
void hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if (activator && activator->inuse && activator->client)
	{
		self->activator = activator;
	}
	else
	{
		self->activator = NULL;
	}

	G_ActivateBehavior(self,BSET_USE);

	if ( self->r.linked ) {
		trap->UnlinkEntity( (sharedEntity_t *)self );
	} else {
		trap->LinkEntity( (sharedEntity_t *)self );
	}
}

void hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int		dflags;

	if (level.gametype == GT_SIEGE && self->team && self->team[0])
	{
		int team = atoi(self->team);

		if (other->inuse && other->s.number < MAX_CLIENTS && other->client &&
			other->client->sess.sessionTeam != team)
		{ //real client don't hurt
			return;
		}
		else if (other->inuse && other->client && other->s.eType == ET_NPC &&
			other->s.NPC_class == CLASS_VEHICLE && other->s.teamowner != team)
		{ //vehicle owned by team don't hurt
			return;
		}
	}

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( !other->takedamage ) {
		return;
	}

	if ( self->timestamp > level.time ) {
		return;
	}

	if (self->damage == -1 && other && other->client && other->health < 1)
	{
		other->client->ps.fallingToDeath = 0;
		ClientRespawn(other);
		return;
	}

	if (self->damage == -1 && other && other->client && other->client->ps.fallingToDeath)
	{
		return;
	}

	if ( self->spawnflags & 16 ) {
		self->timestamp = level.time + 1000;
	} else {
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	/*
	if ( !(self->spawnflags & 4) && self->damage != -1 ) {
		G_Sound( other, CHAN_AUTO, self->noise_index );
	}
	*/

	if (self->spawnflags & 8)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;

	if (self->damage == -1 && other && other->client && other->client->sess.raceMode) { //Racemode falling to death
		if (self->activator && self->activator->inuse && self->activator->client)
		{
			G_Damage (other, self->activator, self->activator, NULL, NULL, 9999, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
		else
		{
			G_Damage (other, self, self, NULL, NULL, 9999, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
	}
	else if (self->damage == -1 && other && other->client)
	{
		if (other->client->ps.otherKillerTime > level.time)
		{ //we're as good as dead, so if someone pushed us into this then remember them
//JAPRO - Serverside - Fixkillcredit - Start
			if (g_fixKillCredit.integer) {
				other->client->ps.otherKillerTime = level.time + 2000;
				other->client->ps.otherKillerDebounceTime = level.time + 100;
			}
			else {
				other->client->ps.otherKillerTime = level.time + 20000;
				other->client->ps.otherKillerDebounceTime = level.time + 10000;
			}
//JAPRO - Serverside - Fixkillcredit - End
		}
		other->client->ps.fallingToDeath = level.time;

		//rag on the way down, this flag will automatically be cleared for us on respawn
		other->client->ps.eFlags |= EF_RAG;

		//make sure his jetpack is off
		Jetpack_Off(other);

		if (other->NPC)
		{ //kill it now
			vec3_t vDir;

			VectorSet(vDir, 0, 1, 0);
			G_Damage(other, other, other, vDir, other->client->ps.origin, Q3_INFINITE, 0, MOD_FALLING);
		}
		else
		{
			G_EntitySound(other, CHAN_VOICE, G_SoundIndex("*falling1.wav"));
		}

		self->timestamp = 0; //do not ignore others
	}
	else	
	{
		int dmg = self->damage;

		if (dmg == -1)
		{ //so fall-to-blackness triggers destroy evertyhing
			dmg = 99999;
			self->timestamp = 0;
		}
		if (self->activator && self->activator->inuse && self->activator->client)
		{
			G_Damage (other, self->activator, self->activator, NULL, NULL, dmg, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
		else
		{
			G_Damage (other, self, self, NULL, NULL, dmg, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
	}
}

void SP_trigger_hurt( gentity_t *self ) {
	InitTrigger (self);

	gTrigFallSound = G_SoundIndex("*falling1.wav");

	self->noise_index = G_SoundIndex( "sound/weapons/force/speed.wav" );
	self->touch = hurt_touch;

	if ( !self->damage ) {
		self->damage = 5;
	}

	self->r.contents = CONTENTS_TRIGGER;

	if ( self->spawnflags & 2 ) {
		self->use = hurt_use;
	}

	// link in to the world if starting active
	if ( ! (self->spawnflags & 1) ) {
		trap->LinkEntity ((sharedEntity_t *)self);
	}
	else if (self->r.linked) {
		trap->UnlinkEntity((sharedEntity_t *)self);
	}
}

#define	INITIAL_SUFFOCATION_DELAY	500 //.5 seconds
void space_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if (!other || !other->inuse || !other->client )
		//NOTE: we need vehicles to know this, too...
		//|| other->s.number >= MAX_CLIENTS)
	{
		return;
	}

	if ( other->s.number < MAX_CLIENTS//player
		&& other->client->ps.m_iVehicleNum//in a vehicle
		&& other->client->ps.m_iVehicleNum >= MAX_CLIENTS )
	{//a player client inside a vehicle
		gentity_t *veh = &g_entities[other->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->hideRider)
		{ //if they are "inside" a vehicle, then let that protect them from THE HORRORS OF SPACE.
			other->client->inSpaceSuffocation = 0;
			other->client->inSpaceIndex = ENTITYNUM_NONE;
			return;
		}
	}

	if (!G_PointInBounds(other->client->ps.origin, self->r.absmin, self->r.absmax))
	{ //his origin must be inside the trigger
		return;
	}

	if (!other->client->inSpaceIndex ||
		other->client->inSpaceIndex == ENTITYNUM_NONE)
	{ //freshly entering space
		other->client->inSpaceSuffocation = level.time + INITIAL_SUFFOCATION_DELAY;
	}

	other->client->inSpaceIndex = self->s.number;
}

/*QUAKED trigger_space (.5 .5 .5) ? 
causes human clients to suffocate and have no gravity.

*/
void SP_trigger_space(gentity_t *self)
{
	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;
	
	self->touch = space_touch;

    trap->LinkEntity((sharedEntity_t *)self);
}

void shipboundary_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *ent;

	if (!other || !other->inuse || !other->client ||
		other->s.number < MAX_CLIENTS ||
		!other->m_pVehicle)
	{ //only let vehicles touch
		return;
	}

	if ( other->client->ps.hyperSpaceTime && level.time - other->client->ps.hyperSpaceTime < HYPERSPACE_TIME )
	{//don't interfere with hyperspacing ships
		return;
	}

	ent = G_Find (NULL, FOFS(targetname), self->target);
	if (!ent || !ent->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_shipboundary has invalid target '%s'\n", self->target );
		return;
	}

	if (!other->client->ps.m_iVehicleNum || other->m_pVehicle->m_iRemovedSurfaces)
	{ //if a vehicle touches a boundary without a pilot in it or with parts missing, just blow the thing up
		G_Damage(other, other, other, NULL, other->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		return;
	}

	//make sure this sucker is linked so the prediction knows where to go
	trap->LinkEntity((sharedEntity_t *)ent);

	other->client->ps.vehTurnaroundIndex = ent->s.number;
	other->client->ps.vehTurnaroundTime = level.time + (self->genericValue1*2);

	//keep up the detailed checks for another 2 seconds
	self->genericValue7 = level.time + 2000;
}

void shipboundary_think(gentity_t *ent)
{
	int			iEntityList[MAX_GENTITIES];
	int			numListedEntities;
	int			i = 0;
	gentity_t	*listedEnt;

	ent->nextthink = level.time + 100;

	if (ent->genericValue7 < level.time)
	{ //don't need to be doing this check, no one has touched recently
		return;
	}

	numListedEntities = trap->EntitiesInBox( ent->r.absmin, ent->r.absmax, iEntityList, MAX_GENTITIES );
	while (i < numListedEntities)
	{
		listedEnt = &g_entities[iEntityList[i]];
		if (listedEnt->inuse && listedEnt->client && listedEnt->client->ps.m_iVehicleNum)
		{
            if (listedEnt->s.eType == ET_NPC &&
				listedEnt->s.NPC_class == CLASS_VEHICLE)
			{
				Vehicle_t *pVeh = listedEnt->m_pVehicle;
				if (pVeh && pVeh->m_pVehicleInfo->type == VH_FIGHTER)
				{
                    shipboundary_touch(ent, listedEnt, NULL);
				}
			}
		}
		i++;
	}
}

/*QUAKED trigger_shipboundary (.5 .5 .5) ? 
causes vehicle to turn toward target and travel in that direction for a set time when hit.

"target"		name of entity to turn toward (can be info_notnull, or whatever).
"traveltime"	time to travel in this direction

*/
void SP_trigger_shipboundary(gentity_t *self)
{
	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;
	
	if (!self->target || !self->target[0])
	{
		trap->Error( ERR_DROP, "trigger_shipboundary without a target." );
	}
	G_SpawnInt("traveltime", "0", &self->genericValue1);

	if (!self->genericValue1)
	{
		trap->Error( ERR_DROP, "trigger_shipboundary without traveltime." );
	}

	self->think = shipboundary_think;
	self->nextthink = level.time + 500;
	self->touch = shipboundary_touch;

    trap->LinkEntity((sharedEntity_t *)self);
}

void hyperspace_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *ent;

	if (!other || !other->inuse || !other->client ||
		other->s.number < MAX_CLIENTS ||
		!other->m_pVehicle)
	{ //only let vehicles touch
		return;
	}

	if ( other->client->ps.hyperSpaceTime && level.time - other->client->ps.hyperSpaceTime < HYPERSPACE_TIME )
	{//already hyperspacing, just keep us moving
		if ( (other->client->ps.eFlags2&EF2_HYPERSPACE) )
		{//they've started the hyperspace but haven't been teleported yet
			float timeFrac = ((float)(level.time-other->client->ps.hyperSpaceTime))/HYPERSPACE_TIME;
			if ( timeFrac >= HYPERSPACE_TELEPORT_FRAC )
			{//half-way, now teleport them!
				vec3_t	diff, fwd, right, up, newOrg;
				float	fDiff, rDiff, uDiff;
				//take off the flag so we only do this once
				other->client->ps.eFlags2 &= ~EF2_HYPERSPACE;
				//Get the offset from the local position
				ent = G_Find (NULL, FOFS(targetname), self->target);
				if (!ent || !ent->inuse)
				{ //this is bad
					trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
					return;
				}
				VectorSubtract( other->client->ps.origin, ent->s.origin, diff );
				AngleVectors( ent->s.angles, fwd, right, up );
				fDiff = DotProduct( fwd, diff );
				rDiff = DotProduct( right, diff );
				uDiff = DotProduct( up, diff );
				//Now get the base position of the destination
				ent = G_Find (NULL, FOFS(targetname), self->target2);
				if (!ent || !ent->inuse)
				{ //this is bad
					trap->Error( ERR_DROP, "trigger_hyperspace has invalid target2 '%s'\n", self->target2 );
					return;
				}
				VectorCopy( ent->s.origin, newOrg );
				//finally, add the offset into the new origin
				AngleVectors( ent->s.angles, fwd, right, up );
				VectorMA( newOrg, fDiff, fwd, newOrg );
				VectorMA( newOrg, rDiff, right, newOrg );
				VectorMA( newOrg, uDiff, up, newOrg );
				//trap->Print("hyperspace from %s to %s\n", vtos(other->client->ps.origin), vtos(newOrg) );
				//now put them in the offset position, facing the angles that position wants them to be facing
				TeleportPlayer( other, newOrg, ent->s.angles, qfalse );
				if ( other->m_pVehicle && other->m_pVehicle->m_pPilot )
				{//teleport the pilot, too
					TeleportPlayer( (gentity_t*)other->m_pVehicle->m_pPilot, newOrg, ent->s.angles, qfalse );
					//FIXME: and the passengers?
				}
				//make them face the new angle
				//other->client->ps.hyperSpaceIndex = ent->s.number;
				VectorCopy( ent->s.angles, other->client->ps.hyperSpaceAngles );
				//sound
				G_Sound( other, CHAN_LOCAL, G_SoundIndex( "sound/vehicles/common/hyperend.wav" ) );
			}
		}
		return;
	}
	else
	{
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent || !ent->inuse)
		{ //this is bad
			trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
			return;
		}

		if (!other->client->ps.m_iVehicleNum || other->m_pVehicle->m_iRemovedSurfaces)
		{ //if a vehicle touches a boundary without a pilot in it or with parts missing, just blow the thing up
			G_Damage(other, other, other, NULL, other->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
			return;
		}
		//other->client->ps.hyperSpaceIndex = ent->s.number;
		VectorCopy( ent->s.angles, other->client->ps.hyperSpaceAngles );
		other->client->ps.hyperSpaceTime = level.time;
	}
}

/*
void trigger_hyperspace_find_targets( gentity_t *self )
{
	gentity_t *targEnt = NULL;
	targEnt = G_Find (NULL, FOFS(targetname), self->target);
	if (!targEnt || !targEnt->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_hyperspace has invalid target '%s'\n", self->target );
		return;
	}
	targEnt->r.svFlags |= SVF_BROADCAST;//crap, need to tell the cgame about the target_position
	targEnt = G_Find (NULL, FOFS(targetname), self->target2);
	if (!targEnt || !targEnt->inuse)
	{ //this is bad
		trap->Error( ERR_DROP, "trigger_hyperspace has invalid target2 '%s'\n", self->target2 );
		return;
	}
	targEnt->r.svFlags |= SVF_BROADCAST;//crap, need to tell the cgame about the target_position
}
*/
/*QUAKED trigger_hyperspace (.5 .5 .5) ? 
Ship will turn to face the angles of the first target_position then fly forward, playing the hyperspace effect, then pop out at a relative point around the target

"target"		whatever position the ship teleports from in relation to the target_position specified here, that's the relative position the ship will spawn at around the target2 target_position
"target2"		name of target_position to teleport the ship to (will be relative to it's origin)
*/
void SP_trigger_hyperspace(gentity_t *self)
{
	//register the hyperspace end sound (start sounds are customized)
	G_SoundIndex( "sound/vehicles/common/hyperend.wav" );

	InitTrigger(self);
	self->r.contents = CONTENTS_TRIGGER;
	
	if (!self->target || !self->target[0])
	{
		trap->Error( ERR_DROP, "trigger_hyperspace without a target." );
	}
	if (!self->target2 || !self->target2[0])
	{
		trap->Error( ERR_DROP, "trigger_hyperspace without a target2." );
	}
	
	self->delay = Distance( self->r.absmax, self->r.absmin );//my size

	self->touch = hyperspace_touch;

    trap->LinkEntity((sharedEntity_t *)self);

	//self->think = trigger_hyperspace_find_targets;
	//self->nextthink = level.time + FRAMETIME;
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

	self->use = func_timer_use;
	self->think = func_timer_think;

	if ( self->random >= self->wait ) {
		self->random = self->wait - 1;//NOTE: was - FRAMETIME, but FRAMETIME is in msec (100) and these numbers are in *seconds*!
		trap->Print( "func_timer at %s has random >= wait\n", vtos( self->s.origin ) );
	}

	if ( self->spawnflags & 1 ) {
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}

gentity_t *asteroid_pick_random_asteroid( gentity_t *self )
{
	int			t_count = 0, pick;
	gentity_t	*t = NULL;

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return NULL;
	}

	if(t_count == 1)
	{
		return t;
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
		
		if(t_count == pick)
		{
			return t;
		}
	}
	return NULL;
}

int asteroid_count_num_asteroids( gentity_t *self )
{
	int	i, count = 0;

	for ( i = MAX_CLIENTS; i < ENTITYNUM_WORLD; i++ )
	{
		if ( !g_entities[i].inuse )
		{
			continue;
		}
		if ( g_entities[i].r.ownerNum == self->s.number )
		{
			count++;
		}
	}
	return count;
}

extern void SP_func_rotating (gentity_t *ent);
extern void Q3_Lerp2Origin( int taskID, int entID, vec3_t origin, float duration );
void asteroid_field_think(gentity_t *self)
{
	int numAsteroids = asteroid_count_num_asteroids( self );

	self->nextthink = level.time + 500;

	if ( numAsteroids < self->count )
	{
		//need to spawn a new asteroid
		gentity_t *newAsteroid = G_Spawn(qtrue);
		if ( newAsteroid )
		{
			vec3_t startSpot, endSpot, startAngles;
			float dist, speed = flrand( self->speed * 0.25f, self->speed * 2.0f );
			int	capAxis, axis, time = 0;
			gentity_t *copyAsteroid = asteroid_pick_random_asteroid( self );
			if ( copyAsteroid )
			{
				newAsteroid->model = copyAsteroid->model;
				newAsteroid->model2 = copyAsteroid->model2;
				newAsteroid->health = copyAsteroid->health;
				newAsteroid->spawnflags = copyAsteroid->spawnflags;
				newAsteroid->mass = copyAsteroid->mass;
				newAsteroid->damage = copyAsteroid->damage;
				newAsteroid->speed = copyAsteroid->speed;

				G_SetOrigin( newAsteroid, copyAsteroid->s.origin );
				G_SetAngles( newAsteroid, copyAsteroid->s.angles );
				newAsteroid->classname = "func_rotating";

				SP_func_rotating( newAsteroid );

				newAsteroid->genericValue15 = copyAsteroid->genericValue15;
				newAsteroid->s.iModelScale = copyAsteroid->s.iModelScale;
				newAsteroid->maxHealth = newAsteroid->health;
				G_ScaleNetHealth(newAsteroid);
				newAsteroid->radius = copyAsteroid->radius;
				newAsteroid->material = copyAsteroid->material;
				//CacheChunkEffects( self->material );

				//keep track of it
				newAsteroid->r.ownerNum = self->s.number;

				//move it
				capAxis = Q_irand( 0, 2 );
				for ( axis = 0; axis < 3; axis++ )
				{
					if ( axis == capAxis )
					{
						if ( Q_irand( 0, 1 ) )
						{
							startSpot[axis] = self->r.mins[axis];
							endSpot[axis] = self->r.maxs[axis];
						}
						else
						{
							startSpot[axis] = self->r.maxs[axis];
							endSpot[axis] = self->r.mins[axis];
						}
					}
					else
					{
						startSpot[axis] = self->r.mins[axis]+(flrand(0,1.0f)*(self->r.maxs[axis]-self->r.mins[axis]));
						endSpot[axis] = self->r.mins[axis]+(flrand(0,1.0f)*(self->r.maxs[axis]-self->r.mins[axis]));
					}
				}
				//FIXME: maybe trace from start to end to make sure nothing is in the way?  How big of a trace?

				G_SetOrigin( newAsteroid, startSpot );
				dist = Distance( endSpot, startSpot );
				time = ceil(dist/speed)*1000;
				Q3_Lerp2Origin( -1, newAsteroid->s.number, endSpot, time );

				//spin it
				startAngles[0] = flrand( -360, 360 );
				startAngles[1] = flrand( -360, 360 );
				startAngles[2] = flrand( -360, 360 );
				G_SetAngles( newAsteroid, startAngles );
				newAsteroid->s.apos.trDelta[0] = flrand( -100, 100 );
				newAsteroid->s.apos.trDelta[1] = flrand( -100, 100 );
				newAsteroid->s.apos.trDelta[2] = flrand( -100, 100 );
				newAsteroid->s.apos.trTime = level.time;
				newAsteroid->s.apos.trType = TR_LINEAR;

				//remove itself when done
				newAsteroid->think = G_FreeEntity;
				newAsteroid->nextthink = level.time+time;

				//think again sooner if need even more
				if ( numAsteroids+1 < self->count )
				{//still need at least one more
					//spawn it in 100ms
					self->nextthink = level.time + 100;
				}
			}
		}
	}
}

/*QUAKED trigger_asteroid_field (.5 .5 .5) ? 
speed - how fast, on average, the asteroid moves
count - how many asteroids, max, to have at one time
target - target this at func_rotating asteroids
*/
void SP_trigger_asteroid_field(gentity_t *self)
{
	trap->SetBrushModel( (sharedEntity_t *)self, self->model );
	self->r.contents = 0;

	if ( !self->count )
	{
		self->health = 20;
	}

	if ( !self->speed )
	{
		self->speed = 10000;
	}

	self->think = asteroid_field_think;
	self->nextthink = level.time + 100;

    trap->LinkEntity((sharedEntity_t *)self);
}