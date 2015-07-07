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

//NPC_reactions.cpp

#include "g_headers.h"


#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "characters.h"
#include "wp_saber.h"

extern qboolean G_CheckForStrongAttackMomentum( gentity_t *self );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern int PM_AnimLength( int index, animNumber_t anim );
extern void cgi_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern int PM_PickAnim( gentity_t *self, int minAnim, int maxAnim );
extern qboolean NPC_CheckLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void Jedi_Ambush( gentity_t *self );

extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_SpinningAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InCartwheel( int anim );

extern	cvar_t	*g_spskill;
extern int	teamLastEnemyTime[];
extern qboolean	stop_icarus;
extern int killPlayerTimer;

float g_crosshairEntDist = Q3_INFINITE;
int g_crosshairSameEntTime = 0;
int g_crosshairEntNum = ENTITYNUM_NONE;
int g_crosshairEntTime = 0;
/*
-------------------------
NPC_CheckAttacker
-------------------------
*/

static void NPC_CheckAttacker( gentity_t *other, int mod )
{
	//FIXME: I don't see anything in here that would stop teammates from taking a teammate
	//			as an enemy.  Ideally, there would be code before this to prevent that from
	//			happening, but that is presumptuous.
	
	//valid ent - FIXME: a VALIDENT macro would be nice here
	if ( !other )
		return;

	if ( other == NPC )
		return;

	if ( !other->inuse )
		return;

	//Don't take a target that doesn't want to be
	if ( other->flags & FL_NOTARGET ) 
		return;

	if ( NPC->svFlags & SVF_LOCKEDENEMY )
	{//IF LOCKED, CANNOT CHANGE ENEMY!!!!!
		return;
	}

	//If we haven't taken a target, just get mad
	if ( NPC->enemy == NULL )//was using "other", fixed to NPC
	{
		G_SetEnemy( NPC, other );
		return;
	}

	//we have an enemy, see if he's dead
	if ( NPC->enemy->health <= 0 )
	{
		G_ClearEnemy( NPC );
		G_SetEnemy( NPC, other );
		return;
	}

	//Don't take the same enemy again
	if ( other == NPC->enemy )
		return;

	if ( NPC->client->ps.weapon == WP_SABER )
	{//I'm a jedi
		if ( mod == MOD_SABER )
		{//I was hit by a saber  FIXME: what if this was a thrown saber?
			//always switch to this enemy if I'm a jedi and hit by another saber
			G_ClearEnemy( NPC );
			G_SetEnemy( NPC, other );
			return;
		}
	}
	//Special case player interactions
	if ( other == &g_entities[0] )
	{
		//Account for the skill level to skew the results
		float	luckThreshold;

		switch ( g_spskill->integer )
		{
		//Easiest difficulty, mild chance of picking up the player
		case 0:
			luckThreshold = 0.9f;
			break;

		//Medium difficulty, half-half chance of picking up the player
		case 1:
			luckThreshold = 0.5f;
			break;

		//Hardest difficulty, always turn on attacking player
		case 2:
		default:
			luckThreshold = 0.0f;
			break;
		}

		//Randomly pick up the target
		if ( random() > luckThreshold )
		{
			G_ClearEnemy( other );
			other->enemy = NPC;
		}

		return;
	}
}

void NPC_SetPainEvent( gentity_t *self )
{
	if ( !self->NPC || !(self->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
	{
	// no more borg
	//	if( self->client->playerTeam != TEAM_BORG )
	//	{
			if ( !Q3_TaskIDPending( self, TID_CHAN_VOICE ) )
			{
				G_AddEvent( self, EV_PAIN, floor((float)self->health/self->max_health*100.0f) );
			}
	//	}
	}
}

/*
-------------------------
NPC_GetPainChance
-------------------------
*/

float NPC_GetPainChance( gentity_t *self, int damage )
{
	if ( !self->enemy )
	{//surprised, always take pain
		return 1.0f;
	}

	if ( damage > self->max_health/2.0f )
	{
		return 1.0f;
	}

	float pain_chance = (float)(self->max_health-self->health)/(self->max_health*2.0f) + (float)damage/(self->max_health/2.0f);
	switch ( g_spskill->integer )
	{
	case 0:	//easy
		//return 0.75f;
		break;

	case 1://med
		pain_chance *= 0.5f;
		//return 0.35f;
		break;

	case 2://hard
	default:
		pain_chance *= 0.1f;
		//return 0.05f;
		break;
	}
	//Com_Printf( "%s: %4.2f\n", self->NPC_type, pain_chance );
	return pain_chance;
}

/*
-------------------------
NPC_ChoosePainAnimation
-------------------------
*/

#define	MIN_PAIN_TIME	200

extern int G_PickPainAnim( gentity_t *self, vec3_t point, int damage, int hitLoc );
void NPC_ChoosePainAnimation( gentity_t *self, gentity_t *other, vec3_t point, int damage, int mod, int hitLoc, int voiceEvent = -1 )
{
	//If we've already taken pain, then don't take it again
	if ( level.time < self->painDebounceTime && mod != MOD_ELECTROCUTE && mod != MOD_MELEE )
	{//FIXME: if hit while recoving from losing a saber lock, we should still play a pain anim?
		return;
	}

	int		pain_anim = -1;
	float	pain_chance;
	
	if ( self->s.weapon == WP_THERMAL && self->client->fireDelay > 0 )
	{//don't interrupt thermal throwing anim
		return;
	}
	else if ( self->client->NPC_class == CLASS_GALAKMECH )
	{
		if ( hitLoc == HL_GENERIC1 )
		{//hit the antenna!
			pain_chance = 1.0f;
			self->s.powerups |= ( 1 << PW_SHOCKED );
			self->client->ps.powerups[PW_SHOCKED] = level.time + Q_irand( 500, 2500 );
		}
		else if ( self->client->ps.powerups[PW_GALAK_SHIELD] )
		{//shield up
			return;
		}
		else if ( self->health > 200 && damage < 100 )
		{//have a *lot* of health
			pain_chance = 0.05f;
		}
		else
		{//the lower my health and greater the damage, the more likely I am to play a pain anim
			pain_chance = (200.0f-self->health)/100.0f + damage/50.0f;
		}
	}
	else if ( self->client && self->client->playerTeam == TEAM_PLAYER && other && !other->s.number )
	{//ally shot by player always complains
		pain_chance = 1.1f;
	}
	else 
	{
		if ( other && (other->s.weapon == WP_SABER || mod == MOD_ELECTROCUTE || mod == MOD_CRUSH/*FIXME:MOD_FORCE_GRIP*/) )
		{
			pain_chance = 1.0f;//always take pain from saber
		}
		else if ( mod == MOD_MELEE )
		{//higher in rank (skill) we are, less likely we are to be fazed by a punch
			pain_chance = 1.0f - ((RANK_CAPTAIN-self->NPC->rank)/(float)RANK_CAPTAIN);
		}
		else if ( self->client->NPC_class == CLASS_PROTOCOL )
		{
			pain_chance = 1.0f;
		}
		else
		{
			pain_chance = NPC_GetPainChance( self, damage );
		}
		if ( self->client->NPC_class == CLASS_DESANN )
		{
			pain_chance *= 0.5f;
		}
	}

	//See if we're going to flinch
	if ( random() < pain_chance )
	{
		//Pick and play our animation
		if ( !(self->client->ps.eFlags&EF_FORCE_GRIPPED) )
		{//not being force-gripped
			if ( G_CheckForStrongAttackMomentum( self )
				|| PM_SpinningAnim( self->client->ps.legsAnim )
				|| PM_SaberInSpecialAttack( self->client->ps.torsoAnim )
				|| PM_InKnockDown( &self->client->ps )
				|| PM_RollingAnim( self->client->ps.legsAnim )
				|| (PM_FlippingAnim( self->client->ps.legsAnim )&&!PM_InCartwheel( self->client->ps.legsAnim )) )
			{//strong attacks, rolls, knockdowns, flips and spins cannot be interrupted by pain
				return;
			}
			else
			{//play an anim
				if ( self->client->NPC_class == CLASS_GALAKMECH )
				{//only has 1 for now
					//FIXME: never plays this, it seems...
					pain_anim = BOTH_PAIN1;
				}
				else if ( mod == MOD_MELEE )
				{
					pain_anim = PM_PickAnim( self, BOTH_PAIN2, BOTH_PAIN3 );
				}
				else if ( self->s.weapon == WP_SABER )
				{//temp HACK: these are the only 2 pain anims that look good when holding a saber
					pain_anim = PM_PickAnim( self, BOTH_PAIN2, BOTH_PAIN3 );
				}
				else if ( mod != MOD_ELECTROCUTE )
				{
					pain_anim = G_PickPainAnim( self, point, damage, hitLoc );
				}

				if ( pain_anim == -1 )
				{
					pain_anim = PM_PickAnim( self, BOTH_PAIN1, BOTH_PAIN19 );
				}
				self->client->ps.saberAnimLevel = FORCE_LEVEL_1;//next attack must be a quick attack
				self->client->ps.saberMove = LS_READY;//don't finish whatever saber move you may have been in
				int parts = SETANIM_BOTH;
				if ( PM_CrouchAnim( self->client->ps.legsAnim ) || PM_InCartwheel( self->client->ps.legsAnim ) )
				{
					parts = SETANIM_LEGS;
				}
				NPC_SetAnim( self, parts, pain_anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			if ( voiceEvent != -1 )
			{
				G_AddVoiceEvent( self, voiceEvent, Q_irand( 2000, 4000 ) );
			}
			else
			{
				NPC_SetPainEvent( self );
			}
		}
		else
		{
			G_AddVoiceEvent( self, Q_irand(EV_CHOKE1, EV_CHOKE3), 0 );
		}
		
		//Setup the timing for it
		if ( mod == MOD_ELECTROCUTE )
		{
			self->painDebounceTime = level.time + 4000;
		}
		self->painDebounceTime = level.time + PM_AnimLength( self->client->clientInfo.animFileIndex, (animNumber_t) pain_anim );
		self->client->fireDelay = 0;
	}
}

/*
===============
NPC_Pain
===============
*/
void NPC_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, vec3_t point, int damage, int mod, int hitLoc ) 
{
	team_t otherTeam = TEAM_FREE;
	int		voiceEvent = -1;

	if ( self->NPC == NULL ) 
		return;

	if ( other == NULL ) 
		return;

	//or just remove ->pain in player_die?
	if ( self->client->ps.pm_type == PM_DEAD )
		return;

	if ( other == self ) 
		return;

	//MCG: Ignore damage from your own team for now
	if ( other->client )
	{
		otherTeam = other->client->playerTeam;
	//	if ( otherTeam == TEAM_DISGUISE )
	//	{
	//		otherTeam = TEAM_PLAYER;
	//	}
	}

	if ( self->client->playerTeam 
		&& other->client 
		&& otherTeam == self->client->playerTeam 
		&& (!player->client->ps.viewEntity || other->s.number != player->client->ps.viewEntity)) 
	{//hit by a teammate
		if ( other != self->enemy && self != other->enemy )
		{//we weren't already enemies
			if ( self->enemy || other->enemy 
				|| (other->s.number&&other->s.number!=player->client->ps.viewEntity) 
				/*|| (!other->s.number&&Q_irand( 0, 3 ))*/ )
			{//if one of us actually has an enemy already, it's okay, just an accident OR wasn't hit by player or someone controlled by player OR player hit ally and didn't get 25% chance of getting mad (FIXME:accumulate anger+base on diff?)
				//FIXME: player should have to do a certain amount of damage to ally or hit them several times to make them mad
				//Still run pain and flee scripts
				if ( self->client && self->NPC )
				{//Run any pain instructions
					if ( self->health <= (self->max_health/3) && G_ActivateBehavior(self, BSET_FLEE) )
					{
						
					}
					else// if( VALIDSTRING( self->behaviorSet[BSET_PAIN] ) )
					{
						G_ActivateBehavior(self, BSET_PAIN);
					}
				}
				if ( damage != -1 )
				{//-1 == don't play pain anim
					//Set our proper pain animation
					if ( Q_irand( 0, 1 ) )
					{
						NPC_ChoosePainAnimation( self, other, point, damage, mod, hitLoc, EV_FFWARN );
					}
					else
					{
						NPC_ChoosePainAnimation( self, other, point, damage, mod, hitLoc );
					}
				}
				return;
			}
			else if ( self->NPC && !other->s.number )//should be assumed, but...
			{//dammit, stop that!
				if ( self->NPC->charmedTime )
				{//mindtricked
					return;
				}
				else if ( self->NPC->ffireCount < 3+((2-g_spskill->integer)*2) )
				{//not mad enough yet
					//Com_Printf( "chck: %d < %d\n", self->NPC->ffireCount, 3+((2-g_spskill->integer)*2) );
					if ( damage != -1 )
					{//-1 == don't play pain anim
						//Set our proper pain animation
						if ( Q_irand( 0, 1 ) )
						{
							NPC_ChoosePainAnimation( self, other, point, damage, mod, hitLoc, EV_FFWARN );
						}
						else
						{
							NPC_ChoosePainAnimation( self, other, point, damage, mod, hitLoc );
						}
					}
					return;
				}
				else if ( G_ActivateBehavior( self, BSET_FFIRE ) )
				{//we have a specific script to run, so do that instead
					return;
				}
				else
				{//okay, we're going to turn on our ally, we need to set and lock our enemy and put ourselves in a bstate that lets us attack him (and clear any flags that would stop us)
					self->NPC->blockedSpeechDebounceTime = 0;
					voiceEvent = EV_FFTURN;
					self->NPC->behaviorState = self->NPC->tempBehavior = self->NPC->defaultBehavior = BS_DEFAULT;
					other->flags &= ~FL_NOTARGET;
					self->svFlags &= ~(SVF_IGNORE_ENEMIES|SVF_ICARUS_FREEZE|SVF_NO_COMBAT_SOUNDS);
					G_SetEnemy( self, other );
					self->svFlags |= SVF_LOCKEDENEMY;
					self->NPC->scriptFlags &= ~(SCF_DONT_FIRE|SCF_CROUCHED|SCF_WALKING|SCF_NO_COMBAT_TALK|SCF_FORCED_MARCH);
					self->NPC->scriptFlags |= (SCF_CHASE_ENEMIES|SCF_NO_MIND_TRICK);
					//NOTE: we also stop ICARUS altogether
					stop_icarus = qtrue;
					if ( !killPlayerTimer )
					{
						killPlayerTimer = level.time + 10000;
					}
				}
			}
		}
	}

	SaveNPCGlobals();
	SetNPCGlobals( self );

	//Do extra bits
	if ( NPCInfo->ignorePain == qfalse )
	{
		NPCInfo->confusionTime = 0;//clear any charm or confusion, regardless
		if ( damage != -1 )
		{//-1 == don't play pain anim
			//Set our proper pain animation
			NPC_ChoosePainAnimation( self, other, point, damage, mod, hitLoc, voiceEvent );
		}
		//Check to take a new enemy
		if ( NPC->enemy != other && NPC != other )
		{//not already mad at them
			NPC_CheckAttacker( other, mod );
		}
	}

	//Attempt to run any pain instructions
	if ( self->client && self->NPC )
	{
		//FIXME: This needs better heuristics perhaps
		if(self->health <= (self->max_health/3) && G_ActivateBehavior(self, BSET_FLEE) )
		{
		}
		else //if( VALIDSTRING( self->behaviorSet[BSET_PAIN] ) )
		{
			G_ActivateBehavior(self, BSET_PAIN);
		}
	}

	//Attempt to fire any paintargets we might have
	if( self->paintarget && self->paintarget[0] )
	{
		G_UseTargets2(self, other, self->paintarget);
	}

	RestoreNPCGlobals();
}

/*
-------------------------
NPC_Touch
-------------------------
*/
extern qboolean INV_SecurityKeyGive( gentity_t *target, const char *keyname );
void NPC_Touch(gentity_t *self, gentity_t *other, trace_t *trace) 
{
	if(!self->NPC)
		return;

	SaveNPCGlobals();
	SetNPCGlobals( self );

	if ( self->message && self->health <= 0 )
	{//I am dead and carrying a key
		if ( other && player && player->health > 0 && other == player )
		{//player touched me
			char *text;
			qboolean	keyTaken;
			//give him my key
			if ( Q_stricmp( "goodie", self->message ) == 0 )
			{//a goodie key
				if ( (keyTaken = INV_GoodieKeyGive( other )) == qtrue )
				{
					text = "cp @INGAME_TOOK_IMPERIAL_GOODIE_KEY";
					G_AddEvent( other, EV_ITEM_PICKUP, (FindItemForInventory( INV_GOODIE_KEY )-bg_itemlist) );
				}
				else
				{
					text = "cp @INGAME_CANT_CARRY_GOODIE_KEY";
				}
			}
			else
			{//a named security key
				if ( (keyTaken = INV_SecurityKeyGive( player, self->message )) == qtrue )
				{
					text = "cp @INGAME_TOOK_IMPERIAL_SECURITY_KEY";
					G_AddEvent( other, EV_ITEM_PICKUP, (FindItemForInventory( INV_SECURITY_KEY )-bg_itemlist) );
				}
				else
				{
					text = "cp @INGAME_CANT_CARRY_SECURITY_KEY";
				}
			}
			if ( keyTaken )
			{//remove my key
				gi.G2API_SetSurfaceOnOff( &self->ghoul2[self->playerModel], "l_arm_key", 0x00000002 );
				self->message = NULL;
				//FIXME: temp pickup sound
				G_Sound( player, G_SoundIndex( "sound/weapons/key_pkup.wav" ) );
				//FIXME: need some event to pass to cgame for sound/graphic/message?
			}
			gi.SendServerCommand( 0, text );
		}
	}

	if ( other->client ) 
	{//FIXME:  if pushing against another bot, both ucmd.rightmove = 127???
		//Except if not facing one another...
		if ( other->health > 0 ) 
		{
			NPCInfo->touchedByPlayer = other;
		}

		if ( other == NPCInfo->goalEntity ) 
		{
			NPCInfo->aiFlags |= NPCAI_TOUCHED_GOAL;
		}

		if( !(self->svFlags&SVF_LOCKEDENEMY) && !(self->svFlags&SVF_IGNORE_ENEMIES) && !(other->flags & FL_NOTARGET) )
		{
			if ( self->client->enemyTeam )
			{//See if we bumped into an enemy
				if ( other->client->playerTeam == self->client->enemyTeam )
				{//bumped into an enemy
					if( NPCInfo->behaviorState != BS_HUNT_AND_KILL && !NPCInfo->tempBehavior )
					{//MCG - Begin: checking specific BS mode here, this is bad, a HACK
						//FIXME: not medics?
						if ( NPC->enemy != other )
						{//not already mad at them
							G_SetEnemy( NPC, other );
						}
		//				NPCInfo->tempBehavior = BS_HUNT_AND_KILL;
					}
				}
			}
		}

		//FIXME: do this if player is moving toward me and with a certain dist?
		/*
		if ( other->s.number == 0 && self->client->playerTeam == other->client->playerTeam )
		{
			VectorAdd( self->client->pushVec, other->client->ps.velocity, self->client->pushVec );
		}
		*/
	}
	else 
	{//FIXME: check for SVF_NONNPC_ENEMY flag here?
		if ( other->health > 0 ) 
		{
			if ( NPC->enemy == other && (other->svFlags&SVF_NONNPC_ENEMY) )
			{
				NPCInfo->touchedByPlayer = other;
			}
		}

		if ( other == NPCInfo->goalEntity ) 
		{
			NPCInfo->aiFlags |= NPCAI_TOUCHED_GOAL;
		}
	}

	RestoreNPCGlobals();
}

/*
-------------------------
NPC_TempLookTarget
-------------------------
*/

void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime )
{
	if ( !self->client )
	{
		return;
	}

	if ( !minLookTime )
	{
		minLookTime = 1000;
	}

	if ( !maxLookTime )
	{
		maxLookTime = 1000;
	}

	if ( !NPC_CheckLookTarget( self ) )
	{//Not already looking at something else
		//Look at him for 1 to 3 seconds
		NPC_SetLookTarget( self, lookEntNum, level.time + Q_irand( minLookTime, maxLookTime ) );
	}
}

void NPC_Respond( gentity_t *self, int userNum )
{
	int event = -1;
	/*

	if ( Q_irand( 0, 1 ) )
	{
		event = Q_irand(EV_RESPOND1, EV_RESPOND3);
	}
	else
	{
		event = Q_irand(EV_BUSY1, EV_BUSY3);
	}
	*/

	if ( !Q_irand( 0, 1 ) )
	{//set looktarget to them for a second or two
		NPC_TempLookTarget( self, userNum, 1000, 3000 );
	}

	//some last-minute hacked in responses
	switch ( self->client->NPC_class )
	{
	case CLASS_JAN:
		if ( self->enemy )
		{
			if ( !Q_irand( 0, 2 ) )
			{
				event = Q_irand( EV_CHASE1, EV_CHASE3 );
			}
			else if ( Q_irand( 0, 1 ) )
			{
				event = Q_irand( EV_OUTFLANK1, EV_OUTFLANK2 );
			}
			else
			{
				event = Q_irand( EV_COVER1, EV_COVER5 );
			}
		}
		else if ( !Q_irand( 0, 2 ) )
		{
			event = EV_SUSPICIOUS4;
		}
		else if ( !Q_irand( 0, 1 ) )
		{
			event = EV_SOUND1;
		}
		else
		{
			event = EV_CONFUSE1;
		}
		break;
	case CLASS_LANDO:
		if ( self->enemy )
		{
			if ( !Q_irand( 0, 2 ) )
			{
				event = Q_irand( EV_CHASE1, EV_CHASE3 );
			}
			else if ( Q_irand( 0, 1 ) )
			{
				event = Q_irand( EV_OUTFLANK1, EV_OUTFLANK2 );
			}
			else
			{
				event = Q_irand( EV_COVER1, EV_COVER5 );
			}
		}
		else if ( !Q_irand( 0, 6 ) )
		{
			event = EV_SIGHT2;
		}
		else if ( !Q_irand( 0, 5 ) )
		{
			event = EV_GIVEUP4;
		}
		else if ( Q_irand( 0, 4 ) > 1 )
		{
			event = Q_irand( EV_SOUND1, EV_SOUND3 );
		}
		else
		{
			event = Q_irand( EV_JDETECTED1, EV_JDETECTED2 );
		}
		break;
	case CLASS_LUKE:
		if ( self->enemy )
		{
			event = EV_COVER1;
		}
		else
		{
			event = Q_irand( EV_SOUND1, EV_SOUND3 );
		}
		break;
	case CLASS_JEDI:
		if ( !self->enemy )
		{
			if ( !(self->svFlags&SVF_IGNORE_ENEMIES) 
				&& (self->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES)
				&& self->client->enemyTeam == TEAM_ENEMY )
			{
				event = Q_irand( EV_ANGER1, EV_ANGER3 );
			}
			else
			{
				event = Q_irand( EV_TAUNT1, EV_TAUNT2 );
			}
		}
		break;
	case CLASS_PRISONER:
		if ( self->enemy )
		{
			if ( Q_irand( 0, 1 ) )
			{
				event = Q_irand( EV_CHASE1, EV_CHASE3 );
			}
			else
			{
				event = Q_irand( EV_OUTFLANK1, EV_OUTFLANK2 );
			}
		}
		else
		{
			event = Q_irand( EV_SOUND1, EV_SOUND3 );
		}
		break;
	case CLASS_REBEL:
		if ( self->enemy )
		{
			if ( !Q_irand( 0, 2 ) )
			{
				event = Q_irand( EV_CHASE1, EV_CHASE3 );
			}
			else
			{
				event = Q_irand( EV_DETECTED1, EV_DETECTED5 );
			}
		}
		else
		{
			event = Q_irand( EV_SOUND1, EV_SOUND3 );
		}
		break;
	case CLASS_BESPIN_COP:
		if ( !Q_stricmp( "bespincop", self->NPC_type ) )
		{//variant 1
			if ( self->enemy )
			{
				if ( Q_irand( 0, 9 ) > 6 )
				{
					event = Q_irand( EV_CHASE1, EV_CHASE3 );
				}
				else if ( Q_irand( 0, 6 ) > 4 )
				{
					event = Q_irand( EV_OUTFLANK1, EV_OUTFLANK2 );
				}
				else
				{
					event = Q_irand( EV_COVER1, EV_COVER5 );
				}
			}
			else if ( !Q_irand( 0, 3 ) )
			{
				event = Q_irand( EV_SIGHT2, EV_SIGHT3 );
			}
			else if ( !Q_irand( 0, 1 ) )
			{
				event = Q_irand( EV_SOUND1, EV_SOUND3 );
			}
			else if ( !Q_irand( 0, 2 ) )
			{
				event = EV_LOST1;
			}
			else if ( !Q_irand( 0, 1 ) )
			{
				event = EV_ESCAPING2;
			}
			else
			{
				event = EV_GIVEUP4;
			}
		}
		else
		{//variant2
			if ( self->enemy )
			{
				if ( Q_irand( 0, 9 ) > 6 )
				{
					event = Q_irand( EV_CHASE1, EV_CHASE3 );
				}
				else if ( Q_irand( 0, 6 ) > 4 )
				{
					event = Q_irand( EV_OUTFLANK1, EV_OUTFLANK2 );
				}
				else
				{
					event = Q_irand( EV_COVER1, EV_COVER5 );
				}
			}
			else if ( !Q_irand( 0, 3 ) )
			{
				event = Q_irand( EV_SIGHT1, EV_SIGHT2 );
			}
			else if ( !Q_irand( 0, 1 ) )
			{
				event = Q_irand( EV_SOUND1, EV_SOUND3 );
			}
			else if ( !Q_irand( 0, 2 ) )
			{
				event = EV_LOST1;
			}
			else if ( !Q_irand( 0, 1 ) )
			{
				event = EV_GIVEUP3;
			}
			else
			{
				event = EV_CONFUSE1;
			}
		}
		break;
	case CLASS_R2D2:				// droid
		G_Sound(self, G_SoundIndex(va("sound/chars/r2d2/misc/r2d2talk0%d.wav",Q_irand(1, 3))));
		break;
	case CLASS_R5D2:				// droid
		G_Sound(self, G_SoundIndex(va("sound/chars/r5d2/misc/r5talk%d.wav",Q_irand(1, 4))));
		break;
	case CLASS_MOUSE:				// droid
		G_Sound(self, G_SoundIndex(va("sound/chars/mouse/misc/mousego%d.wav",Q_irand(1, 3))));
		break;
	case CLASS_GONK:				// droid
		G_Sound(self, G_SoundIndex(va("sound/chars/gonk/misc/gonktalk%d.wav",Q_irand(1, 2))));
		break;
	default:
		break;
	}
	
	if ( event != -1 )
	{
		//hack here because we reuse some "combat" and "extra" sounds
		qboolean addFlag = (self->NPC->scriptFlags&SCF_NO_COMBAT_TALK);
		self->NPC->scriptFlags &= ~SCF_NO_COMBAT_TALK;

		G_AddVoiceEvent( self, event, 3000 );

		if ( addFlag )
		{
			self->NPC->scriptFlags |= SCF_NO_COMBAT_TALK;
		}
	}
}

/*
-------------------------
NPC_UseResponse
-------------------------
*/

void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone )
{
	if ( !self->NPC || !self->client )
	{
		return;
	}

	if ( user->s.number != 0 )
	{//not used by the player
		if ( useWhenDone )
		{
			G_ActivateBehavior( self, BSET_USE );
		}
		return;
	}

	if ( user->client && self->client->playerTeam != user->client->playerTeam && self->client->playerTeam != TEAM_NEUTRAL )
	{//only those on the same team react
		if ( useWhenDone )
		{
			G_ActivateBehavior( self, BSET_USE );
		}
		return;
	}

	if ( self->NPC->blockedSpeechDebounceTime > level.time )
	{//I'm not responding right now
		return;
	}

	if ( gi.VoiceVolume[self->s.number] )
	{//I'm talking already
		if ( !useWhenDone )
		{//you're not trying to use me
			return;
		}
	}

	if ( useWhenDone )
	{
		G_ActivateBehavior( self, BSET_USE );
	}
	else
	{
		NPC_Respond( self, user->s.number );
	}
}

/*
-------------------------
NPC_Use
-------------------------
*/
extern void Add_Batteries( gentity_t *ent, int *count );

void NPC_Use( gentity_t *self, gentity_t *other, gentity_t *activator ) 
{
	if (self->client->ps.pm_type == PM_DEAD)
	{//or just remove ->pain in player_die?
		return;
	}

	SaveNPCGlobals();
	SetNPCGlobals( self );

	if(self->client && self->NPC)
	{
		if ( Jedi_WaitingAmbush( NPC ) )
		{
			Jedi_Ambush( NPC );
		}
		//Run any use instructions
		if ( activator && activator->s.number == 0 && self->client->NPC_class == CLASS_GONK )
		{
			// must be using the gonk, so attempt to give battery power.
			// NOTE: this will steal up to MAX_BATTERIES for the activator, leaving the residual on the gonk for potential later use.
			Add_Batteries( activator, &self->client->ps.batteryCharge );
		}
		// Not using MEDICs anymore
/*
		if ( self->NPC->behaviorState == BS_MEDIC_HIDE && activator->client )
		{//Heal me NOW, dammit!
			if ( activator->health < activator->max_health )
			{//person needs help
				if ( self->NPC->eventualGoal != activator )
				{//not my current patient already
					NPC_TakePatient( activator );
					G_ActivateBehavior( self, BSET_USE );
				}
			}
			else if ( !self->enemy && activator->s.number == 0 && !gi.VoiceVolume[self->s.number] && !(self->NPC->scriptFlags&SCF_NO_RESPONSE) )
			{//I don't have an enemy and I'm not talking and I was used by the player
				NPC_UseResponse( self, other, qfalse );
			}
		}
*/
//		else if ( self->behaviorSet[BSET_USE] )
		if ( self->behaviorSet[BSET_USE] )
		{
			NPC_UseResponse( self, other, qtrue );
		}
//		else if ( isMedic( self ) )
//		{//Heal me NOW, dammit!
//			NPC_TakePatient( activator );
//		}
		else if ( !self->enemy && activator->s.number == 0 && !gi.VoiceVolume[self->s.number] && !(self->NPC->scriptFlags&SCF_NO_RESPONSE) )
		{//I don't have an enemy and I'm not talking and I was used by the player
			NPC_UseResponse( self, other, qfalse );
		}
	}

	RestoreNPCGlobals();
}

void NPC_CheckPlayerAim( void )
{
	//FIXME: need appropriate dialogue
	/*
	gentity_t *player = &g_entities[0];

	if ( player && player->client && player->client->ps.weapon > (int)(WP_NONE) && player->client->ps.weapon < (int)(WP_TRICORDER) )
	{//player has a weapon ready
		if ( g_crosshairEntNum == NPC->s.number && level.time - g_crosshairEntTime < 200 
			&& g_crosshairSameEntTime >= 3000 && g_crosshairEntDist < 256 )
		{//if the player holds the crosshair on you for a few seconds
			//ask them what the fuck they're doing
			G_AddVoiceEvent( NPC, Q_irand( EV_FF_1A, EV_FF_1C ), 0 );
		}
	}
	*/
}

void NPC_CheckAllClear( void )
{
	//FIXME: need to make this happen only once after losing enemies, not over and over again
	/*
	if ( NPC->client && !NPC->enemy && level.time - teamLastEnemyTime[NPC->client->playerTeam] > 10000 )
	{//Team hasn't seen an enemy in 10 seconds
		if ( !Q_irand( 0, 2 ) )
		{
			G_AddVoiceEvent( NPC, Q_irand(EV_SETTLE1, EV_SETTLE3), 3000 );
		}
	}
	*/
}
