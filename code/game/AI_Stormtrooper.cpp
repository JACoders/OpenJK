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

#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "g_navigator.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState );
extern qboolean AI_GroupContainsEntNum( AIGroupInfo_t *group, int entNum );
extern void AI_GroupUpdateEnemyLastSeen( AIGroupInfo_t *group, vec3_t spot );
extern void AI_GroupUpdateClearShotTime( AIGroupInfo_t *group );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
extern void NPC_CheckGetNewWeapon( void );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern int GetTime ( int lastTime );
extern void NPC_AimAdjust( int change );
extern qboolean FlyingCreature( gentity_t *ent );
extern void NPC_EvasionSaber( void );
extern qboolean RT_Flying( gentity_t *self );

extern	cvar_t		*d_asynchronousGroupAI;

#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1
#define	ST_MIN_LIGHT_THRESHOLD 30
#define	ST_MAX_LIGHT_THRESHOLD 180
#define	DISTANCE_THRESHOLD	0.075f
#define	MIN_TURN_AROUND_DIST_SQ	(10000)	//(100 squared) don't stop running backwards if your goal is less than 100 away
#define SABER_AVOID_DIST	128.0f//256.0f
#define SABER_AVOID_DIST_SQ (SABER_AVOID_DIST*SABER_AVOID_DIST)

#define	DISTANCE_SCALE		0.35f	//These first three get your base detection rating, ideally add up to 1
#define	FOV_SCALE			0.40f	//
#define	LIGHT_SCALE			0.25f	//

#define	SPEED_SCALE			0.25f	//These next two are bonuses
#define	TURNING_SCALE		0.25f	//

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth( void );

static qboolean enemyLOS;
static qboolean enemyCS;
static qboolean enemyInFOV;
static qboolean hitAlly;
static qboolean faceEnemy;
static qboolean doMove;
static qboolean shoot;
static float	enemyDist;
static vec3_t	impactPos;

int groupSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several group AI from speaking all at once

void NPC_Saboteur_Precache( void )
{
	G_SoundIndex( "sound/chars/shadowtrooper/cloak.wav" );
	G_SoundIndex( "sound/chars/shadowtrooper/decloak.wav" );
}

void Saboteur_Decloak( gentity_t *self, int uncloakTime )
{
	if ( self && self->client )
	{
		if ( self->client->ps.powerups[PW_CLOAKED] && TIMER_Done(self, "decloakwait"))
		{//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: temp sound
			G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav" );
			TIMER_Set( self, "nocloak", uncloakTime );

			// Can't Recloak
			//self->NPC->aiFlags	&= ~NPCAI_SHIELDS;
		}
	}
}

void Saboteur_Cloak( gentity_t *self )
{
	if ( self && self->client && self->NPC )
	{//FIXME: need to have this timer set once first?
		if ( TIMER_Done( self, "nocloak" ) )
		{//not sitting around waiting to cloak again
			if ( !(self->NPC->aiFlags&NPCAI_SHIELDS) )
			{//not allowed to cloak, actually
				Saboteur_Decloak( self );
			}
			else if ( !self->client->ps.powerups[PW_CLOAKED] )
			{//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
				self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
				//FIXME: debounce attacks?
				//FIXME: temp sound
				G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav" );
			}
		}
	}
}



//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void ST_AggressionAdjust( gentity_t *self, int change )
{
	int	upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;

	//FIXME: base this on initial NPC stats
	if ( self->client->playerTeam == TEAM_PLAYER )
	{//good guys are less aggressive
		upper_threshold = 7;
		lower_threshold = 1;
	}
	else
	{//bad guys are more aggressive
		upper_threshold = 10;
		lower_threshold = 3;
	}

	if ( self->NPC->stats.aggression > upper_threshold )
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if ( self->NPC->stats.aggression < lower_threshold )
	{
		self->NPC->stats.aggression = lower_threshold;
	}
}

void ST_ClearTimers( gentity_t *ent )
{
	TIMER_Set( ent, "chatter", 0 );
	TIMER_Set( ent, "duck", 0 );
	TIMER_Set( ent, "stand", 0 );
	TIMER_Set( ent, "shuffleTime", 0 );
	TIMER_Set( ent, "sleepTime", 0 );
	TIMER_Set( ent, "enemyLastVisible", 0 );
	TIMER_Set( ent, "roamTime", 0 );
	TIMER_Set( ent, "hideTime", 0 );
	TIMER_Set( ent, "attackDelay", 0 );	//FIXME: Slant for difficulty levels
	TIMER_Set( ent, "stick", 0 );
	TIMER_Set( ent, "scoutTime", 0 );
	TIMER_Set( ent, "flee", 0 );
	TIMER_Set( ent, "interrogating", 0 );
	TIMER_Set( ent, "verifyCP", 0 );
	TIMER_Set( ent, "strafeRight", 0 );
	TIMER_Set( ent, "strafeLeft", 0 );
}

enum
{
	SPEECH_CHASE,
	SPEECH_CONFUSED,
	SPEECH_COVER,
	SPEECH_DETECTED,
	SPEECH_GIVEUP,
	SPEECH_LOOK,
	SPEECH_LOST,
	SPEECH_OUTFLANK,
	SPEECH_ESCAPING,
	SPEECH_SIGHT,
	SPEECH_SOUND,
	SPEECH_SUSPICIOUS,
	SPEECH_YELL,
	SPEECH_PUSHED
};

static void ST_Speech( gentity_t *self, int speechType, float failChance )
{
	if ( Q_flrand(0.0f, 1.0f) < failChance )
	{
		return;
	}

	if ( failChance >= 0 )
	{//a negative failChance makes it always talk
		if ( self->NPC->group )
		{//group AI speech debounce timer
			if ( self->NPC->group->speechDebounceTime > level.time )
			{
				return;
			}
			/*
			else if ( !self->NPC->group->enemy )
			{
				if ( groupSpeechDebounceTime[self->client->playerTeam] > level.time )
				{
					return;
				}
			}
			*/
		}
		else if ( !TIMER_Done( self, "chatter" ) )
		{//personal timer
			return;
		}
		else if ( groupSpeechDebounceTime[self->client->playerTeam] > level.time )
		{//for those not in group AI
			//FIXME: let certain speech types interrupt others?  Let closer NPCs interrupt farther away ones?
			return;
		}
	}

	if ( self->NPC->group )
	{//So they don't all speak at once...
		//FIXME: if they're not yet mad, they have no group, so distracting a group of them makes them all speak!
		self->NPC->group->speechDebounceTime = level.time + Q_irand( 2000, 4000 );
	}
	else
	{
		TIMER_Set( self, "chatter", Q_irand( 2000, 4000 ) );
	}
	groupSpeechDebounceTime[self->client->playerTeam] = level.time + Q_irand( 2000, 4000 );

	if ( self->NPC->blockedSpeechDebounceTime > level.time )
	{
		return;
	}

	switch( speechType )
	{
	case SPEECH_CHASE:
		G_AddVoiceEvent( self, Q_irand(EV_CHASE1, EV_CHASE3), 2000 );
		break;
	case SPEECH_CONFUSED:
		G_AddVoiceEvent( self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
		break;
	case SPEECH_COVER:
		G_AddVoiceEvent( self, Q_irand(EV_COVER1, EV_COVER5), 2000 );
		break;
	case SPEECH_DETECTED:
		G_AddVoiceEvent( self, Q_irand(EV_DETECTED1, EV_DETECTED5), 2000 );
		break;
	case SPEECH_GIVEUP:
		G_AddVoiceEvent( self, Q_irand(EV_GIVEUP1, EV_GIVEUP4), 2000 );
		break;
	case SPEECH_LOOK:
		G_AddVoiceEvent( self, Q_irand(EV_LOOK1, EV_LOOK2), 2000 );
		break;
	case SPEECH_LOST:
		G_AddVoiceEvent( self, EV_LOST1, 2000 );
		break;
	case SPEECH_OUTFLANK:
		G_AddVoiceEvent( self, Q_irand(EV_OUTFLANK1, EV_OUTFLANK2), 2000 );
		break;
	case SPEECH_ESCAPING:
		G_AddVoiceEvent( self, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 2000 );
		break;
	case SPEECH_SIGHT:
		G_AddVoiceEvent( self, Q_irand(EV_SIGHT1, EV_SIGHT3), 2000 );
		break;
	case SPEECH_SOUND:
		G_AddVoiceEvent( self, Q_irand(EV_SOUND1, EV_SOUND3), 2000 );
		break;
	case SPEECH_SUSPICIOUS:
		G_AddVoiceEvent( self, Q_irand(EV_SUSPICIOUS1, EV_SUSPICIOUS5), 2000 );
		break;
	case SPEECH_YELL:
		G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 2000 );
		break;
	case SPEECH_PUSHED:
		G_AddVoiceEvent( self, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 2000 );
		break;
	default:
		break;
	}

	self->NPC->blockedSpeechDebounceTime = level.time + 2000;
}

void ST_MarkToCover( gentity_t *self )
{
	if ( !self || !self->NPC )
	{
		return;
	}
	self->NPC->localState = LSTATE_UNDERFIRE;
	TIMER_Set( self, "attackDelay", Q_irand( 500, 2500 ) );
	ST_AggressionAdjust( self, -3 );
	if ( self->NPC->group && self->NPC->group->numGroup > 1 )
	{
		ST_Speech( self, SPEECH_COVER, 0 );//FIXME: flee sound?
	}
}

void ST_StartFlee( gentity_t *self, gentity_t *enemy, vec3_t dangerPoint, int dangerLevel, int minTime, int maxTime )
{
	if ( !self || !self->NPC )
	{
		return;
	}
	G_StartFlee( self, enemy, dangerPoint, dangerLevel, minTime, maxTime );
	if ( self->NPC->group && self->NPC->group->numGroup > 1 )
	{
		ST_Speech( self, SPEECH_COVER, 0 );//FIXME: flee sound?
	}
}
/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_ST_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "hideTime", -1 );
	TIMER_Set( self, "stand", 2000 );

	NPC_Pain( self, inflictor, other, point, damage, mod, hitLoc );

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	}
}

/*
-------------------------
ST_HoldPosition
-------------------------
*/

static void ST_HoldPosition( void )
{
	if ( NPCInfo->squadState == SQUAD_RETREAT )
	{
		TIMER_Set( NPC, "flee", -level.time );
	}
	TIMER_Set( NPC, "verifyCP", Q_irand( 1000, 3000 ) );//don't look for another one for a few seconds
	NPC_FreeCombatPoint( NPCInfo->combatPoint, qtrue );
	//NPCInfo->combatPoint = -1;//???
	if ( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//don't have a script waiting for me to get to my point, okay to stop trying and stand
		AI_GroupUpdateSquadstates( NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT );
		NPCInfo->goalEntity = NULL;
	}

}

void NPC_ST_SayMovementSpeech( void )
{
	if ( !NPCInfo->movementSpeech )
	{
		return;
	}
	if ( NPCInfo->group &&
		NPCInfo->group->commander &&
		NPCInfo->group->commander->client &&
		NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
		!Q_irand( 0, 3 ) )
	{//imperial (commander) gives the order
		ST_Speech( NPCInfo->group->commander, NPCInfo->movementSpeech, NPCInfo->movementSpeechChance );
	}
	else
	{//really don't want to say this unless we can actually get there...
		ST_Speech( NPC, NPCInfo->movementSpeech, NPCInfo->movementSpeechChance );
	}

	NPCInfo->movementSpeech = 0;
	NPCInfo->movementSpeechChance = 0.0f;
}

void NPC_ST_StoreMovementSpeech( int speech, float chance )
{
	NPCInfo->movementSpeech = speech;
	NPCInfo->movementSpeechChance = chance;
}
/*
-------------------------
ST_Move
-------------------------
*/
void ST_TransferMoveGoal( gentity_t *self, gentity_t *other );
static qboolean ST_Move( void )
{
	NPCInfo->combatMove = qtrue;//always doMove straight toward our goal

	qboolean	moved = NPC_MoveToGoal( qtrue );
	if (moved==qfalse)
	{
		ST_HoldPosition();
	}

	NPC_ST_SayMovementSpeech();

	return moved;
}


/*
-------------------------
NPC_ST_SleepShuffle
-------------------------
*/

static void NPC_ST_SleepShuffle( void )
{
	//Play an awake script if we have one
	if ( G_ActivateBehavior( NPC, BSET_AWAKE) )
	{
		return;
	}

	//Automate some movement and noise
	if ( TIMER_Done( NPC, "shuffleTime" ) )
	{

		//TODO: Play sleeping shuffle animation

		//int	soundIndex = Q_irand( 0, 1 );

		/*
		switch ( soundIndex )
		{
		case 0:
			G_Sound( NPC, G_SoundIndex("sound/chars/imperialsleeper1/scav4/hunh.mp3") );
			break;

		case 1:
			G_Sound( NPC, G_SoundIndex("sound/chars/imperialsleeper3/scav4/tryingtosleep.wav") );
			break;
		}
		*/

		TIMER_Set( NPC, "shuffleTime", 4000 );
		TIMER_Set( NPC, "sleepTime", 2000 );
		return;
	}

	//They made another noise while we were stirring, see if we can see them
	if ( TIMER_Done( NPC, "sleepTime" ) )
	{
		NPC_CheckPlayerTeamStealth();
		TIMER_Set( NPC, "sleepTime", 2000 );
	}
}

/*
-------------------------
NPC_ST_Sleep
-------------------------
*/

void NPC_BSST_Sleep( void )
{
	int alertEvent = NPC_CheckAlertEvents( qfalse, qtrue );//only check sounds since we're alseep!

	//There is an event we heard
	if ( alertEvent >= 0 )
	{
		//See if it was enough to wake us up
		if ( level.alertEvents[alertEvent].level == AEL_DISCOVERED && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			if ( &g_entities[0] && g_entities[0].health > 0 )
			{
				G_SetEnemy( NPC, &g_entities[0] );
				return;
			}
		}

		//Otherwise just stir a bit
		NPC_ST_SleepShuffle();
		return;
	}
}

/*
-------------------------
NPC_CheckEnemyStealth
-------------------------
*/

qboolean NPC_CheckEnemyStealth( gentity_t *target )
{
	float		target_dist, minDist = 40;//any closer than 40 and we definitely notice

	//In case we aquired one some other way
	if ( NPC->enemy != NULL )
		return qtrue;

	//Ignore notarget
	if ( target->flags & FL_NOTARGET )
		return qfalse;

	if ( target->health <= 0 )
	{
		return qfalse;
	}

	if ( target->client->ps.weapon == WP_SABER && target->client->ps.SaberActive() && !target->client->ps.saberInFlight )
	{//if target has saber in hand and activated, we wake up even sooner even if not facing him
		minDist = 100;
	}

	target_dist = DistanceSquared( target->currentOrigin, NPC->currentOrigin );
	//If the target is this close, then wake up regardless
	if ( !(target->client->ps.pm_flags&PMF_DUCKED)//not ducking
		&& (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES)//looking for enemies
		&& target_dist < (minDist*minDist) )//closer than minDist
	{
		G_SetEnemy( NPC, target );
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}

	float	maxViewDist			= MAX_VIEW_DIST;

//	if ( NPCInfo->stats.visrange > maxViewDist )
	{//FIXME: should we always just set maxViewDist to this?
		maxViewDist = NPCInfo->stats.visrange;
	}

	if ( target_dist > (maxViewDist*maxViewDist) )
	{//out of possible visRange
		return qfalse;
	}

	//Check FOV first
	if ( InFOV( target, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov ) == qfalse )
		return qfalse;

	qboolean clearLOS = ( target->client->ps.leanofs ) ? NPC_ClearLOS( target->client->renderInfo.eyePoint ) : NPC_ClearLOS( target );

	//Now check for clear line of vision
	if ( clearLOS )
	{
		if ( target->client->NPC_class == CLASS_ATST )
		{//can't miss 'em!
			G_SetEnemy( NPC, target );
			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}
		vec3_t	targ_org			= {target->currentOrigin[0],target->currentOrigin[1],target->currentOrigin[2]+target->maxs[2]-4};
		float	hAngle_perc			= NPC_GetHFOVPercentage( targ_org, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, NPCInfo->stats.hfov );
		float	vAngle_perc			= NPC_GetVFOVPercentage( targ_org, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, NPCInfo->stats.vfov );

		//Scale them vertically some, and horizontally pretty harshly
		vAngle_perc *= vAngle_perc;//( vAngle_perc * vAngle_perc );
		hAngle_perc *= ( hAngle_perc * hAngle_perc );

		//Cap our vertical vision severely
		//if ( vAngle_perc <= 0.3f ) // was 0.5f
		//	return qfalse;

		//Assess the player's current status
		target_dist			= Distance( target->currentOrigin, NPC->currentOrigin );

		float	target_speed		= VectorLength( target->client->ps.velocity );
		int		target_crouching	= ( target->client->usercmd.upmove < 0 );
		float	dist_rating			= ( target_dist / maxViewDist );
		float	speed_rating		= ( target_speed / MAX_VIEW_SPEED );
		float	turning_rating		= AngleDelta( target->client->ps.viewangles[PITCH], target->lastAngles[PITCH] )/180.0f + AngleDelta( target->client->ps.viewangles[YAW], target->lastAngles[YAW] )/180.0f;
		float	light_level			= ( target->lightLevel / MAX_LIGHT_INTENSITY );
		float	FOV_perc			= 1.0f - ( hAngle_perc + vAngle_perc ) * 0.5f;	//FIXME: Dunno about the average...
		float	vis_rating			= 0.0f;

		//Too dark
		if ( light_level < MIN_LIGHT_THRESHOLD )
			return qfalse;

		//Too close?
		if ( dist_rating < DISTANCE_THRESHOLD )
		{
			G_SetEnemy( NPC, target );
			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}

		//Out of range
		if ( dist_rating > 1.0f )
			return qfalse;

		//Cap our speed checks
		if ( speed_rating > 1.0f )
			speed_rating = 1.0f;


		//Calculate the distance, fov and light influences
		//...Visibilty linearly wanes over distance
		float dist_influence	= DISTANCE_SCALE * ( ( 1.0f - dist_rating ) );
		//...As the percentage out of the FOV increases, straight perception suffers on an exponential scale
		float fov_influence		= FOV_SCALE * ( 1.0f - FOV_perc );
		//...Lack of light hides, abundance of light exposes
		float light_influence	= ( light_level - 0.5f ) * LIGHT_SCALE;

		//Calculate our base rating
		float target_rating		= dist_influence + fov_influence + light_influence;

		//Now award any final bonuses to this number
		int contents = gi.pointcontents( targ_org, target->s.number );
		if ( contents&CONTENTS_WATER )
		{
			int myContents = gi.pointcontents( NPC->client->renderInfo.eyePoint, NPC->s.number );
			if ( !(myContents&CONTENTS_WATER) )
			{//I'm not in water
				if ( NPC->client->NPC_class == CLASS_SWAMPTROOPER )
				{//these guys can see in in/through water pretty well
					vis_rating = 0.10f;//10% bonus
				}
				else
				{
					vis_rating = 0.35f;//35% bonus
				}
			}
			else
			{//else, if we're both in water
				if ( NPC->client->NPC_class == CLASS_SWAMPTROOPER )
				{//I can see him just fine
				}
				else
				{
					vis_rating = 0.15f;//15% bonus
				}
			}
		}
		else
		{//not in water
			if ( contents&CONTENTS_FOG )
			{
				vis_rating = 0.15f;//15% bonus
			}
		}

		target_rating *= (1.0f - vis_rating);

		//...Motion draws the eye quickly
		target_rating += speed_rating * SPEED_SCALE;
		target_rating += turning_rating * TURNING_SCALE;
		//FIXME: check to see if they're animating, too?  But can we do something as simple as frame != oldframe?

		//...Smaller targets are harder to indentify
		if ( target_crouching )
		{
			target_rating *= 0.9f;	//10% bonus
		}

		//If he's violated the threshold, then realize him
		//float difficulty_scale = 1.0f + (2.0f-g_spskill->value);//if playing on easy, 20% harder to be seen...?
		float realize, cautious;
		if ( NPC->client->NPC_class == CLASS_SWAMPTROOPER )
		{//swamptroopers can see much better
			realize = (float)CAUTIOUS_THRESHOLD/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}
		else
		{
			realize = (float)REALIZE_THRESHOLD/**difficulty_scale*/;
			cautious = (float)CAUTIOUS_THRESHOLD * 0.75f/**difficulty_scale*/;
		}

		if ( target_rating > realize && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			G_SetEnemy( NPC, target );
			NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
			return qtrue;
		}

		//If he's above the caution threshold, then realize him in a few seconds unless he moves to cover
		if ( target_rating > cautious && !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
		{//FIXME: ambushing guys should never talk
			if ( TIMER_Done( NPC, "enemyLastVisible" ) )
			{//If we haven't already, start the counter
				int	lookTime = Q_irand( 4500, 8500 );
				//NPCInfo->timeEnemyLastVisible = level.time + 2000;
				TIMER_Set( NPC, "enemyLastVisible", lookTime );
				//TODO: Play a sound along the lines of, "Huh?  What was that?"
				ST_Speech( NPC, SPEECH_SIGHT, 0 );
				NPC_TempLookTarget( NPC, target->s.number, lookTime, lookTime );
				//FIXME: set desired yaw and pitch towards this guy?
			}
			else if ( TIMER_Get( NPC, "enemyLastVisible" ) <= level.time + 500 && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )	//FIXME: Is this reliable?
			{
				if ( NPCInfo->rank < RANK_LT && !Q_irand( 0, 2 ) )
				{
					int	interrogateTime = Q_irand( 2000, 4000 );
					ST_Speech( NPC, SPEECH_SUSPICIOUS, 0 );
					TIMER_Set( NPC, "interrogating", interrogateTime );
					G_SetEnemy( NPC, target );
					NPCInfo->enemyLastSeenTime = level.time;
					TIMER_Set( NPC, "attackDelay", interrogateTime );
					TIMER_Set( NPC, "stand", interrogateTime );
				}
				else
				{
					G_SetEnemy( NPC, target );
					NPCInfo->enemyLastSeenTime = level.time;
					//FIXME: ambush guys (like those popping out of water) shouldn't delay...
					TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
					TIMER_Set( NPC, "stand", Q_irand( 500, 2500 ) );
				}
				return qtrue;
			}

			return qfalse;
		}
	}

	return qfalse;
}

qboolean NPC_CheckPlayerTeamStealth( void )
{
	/*
	//NOTENOTE: For now, all stealh checks go against the player, since
	//			he is the main focus.  Squad members and rivals do not
	//			fall into this category and will be ignored.

	NPC_CheckEnemyStealth( &g_entities[0] );	//Change this pointer to assess other entities
	*/
	gentity_t *enemy;
	for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
	{
		if(!PInUse(i))
			continue;
		enemy = &g_entities[i];
		if ( enemy
			&& enemy->client
			&& NPC_ValidEnemy( enemy ) )
		{
			if ( NPC_CheckEnemyStealth( enemy ) )	//Change this pointer to assess other entities
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean NPC_CheckEnemiesInSpotlight( void )
{
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*enemy, *suspect = NULL;
	int			i, numListedEntities;
	vec3_t		mins, maxs;

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = NPC->client->renderInfo.eyePoint[i] - NPC->speed;
		maxs[i] = NPC->client->renderInfo.eyePoint[i] + NPC->speed;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < numListedEntities; i++ )
	{
		if(!PInUse(i))
			continue;

		enemy = entityList[i];

		if ( enemy && enemy->client && NPC_ValidEnemy( enemy ) && enemy->client->playerTeam == NPC->client->enemyTeam )
		{//valid ent & client, valid enemy, on the target team
			//check to see if they're in my FOV
			if ( InFOV( enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, NPCInfo->stats.hfov, NPCInfo->stats.vfov ) )
			{//in my cone
				//check to see that they're close enough
				if ( DistanceSquared( NPC->client->renderInfo.eyePoint, enemy->currentOrigin )-256/*fudge factor: 16 squared*/ <= NPC->speed*NPC->speed )
				{//within range
					//check to see if we have a clear trace to them
					if ( G_ClearLOS( NPC, enemy ) )
					{//clear LOS
						//make sure their light level is at least my beam's brightness
						//FIXME: HOW?
						//enemy->lightLevel / MAX_LIGHT_INTENSITY

						//good enough, take him!
						//FIXME: pick closest one?
						//FIXME: have the graduated noticing like other NPCs? (based on distance, FOV dot, etc...)
						G_SetEnemy( NPC, enemy );
						TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
						return qtrue;
					}
				}
			}
			if ( InFOV( enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, 90, NPCInfo->stats.vfov*3 ) )
			{//one to look at if we don't get an enemy
				if ( G_ClearLOS( NPC, enemy ) )
				{//clear LOS
					if ( suspect == NULL || DistanceSquared( NPC->client->renderInfo.eyePoint, enemy->currentOrigin ) < DistanceSquared( NPC->client->renderInfo.eyePoint, suspect->currentOrigin ) )
					{//remember him
						suspect = enemy;
					}
				}
			}
		}
	}
	if ( suspect && Q_flrand( 0, NPCInfo->stats.visrange*NPCInfo->stats.visrange ) > DistanceSquared( NPC->client->renderInfo.eyePoint, suspect->currentOrigin ) )
	{//hey!  who's that?
		if ( TIMER_Done( NPC, "enemyLastVisible" ) )
		{//If we haven't already, start the counter
			int	lookTime = Q_irand( 4500, 8500 );
			//NPCInfo->timeEnemyLastVisible = level.time + 2000;
			TIMER_Set( NPC, "enemyLastVisible", lookTime );
			//TODO: Play a sound along the lines of, "Huh?  What was that?"
			ST_Speech( NPC, SPEECH_SIGHT, 0 );
			//set desired yaw and pitch towards this guy?
			//FIXME: this is permanent, they will never look away... *sigh*
			NPC_FacePosition( suspect->currentOrigin, qtrue );
			//FIXME: they still need some sort of eye/head tag/bone that can turn?
			//NPC_TempLookTarget( NPC, suspect->s.number, lookTime, lookTime );
		}
		else if ( TIMER_Get( NPC, "enemyLastVisible" ) <= level.time + 500
			&& (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )	//FIXME: Is this reliable?
		{
			if ( !Q_irand( 0, 2 ) )
			{
				int	interrogateTime = Q_irand( 2000, 4000 );
				ST_Speech( NPC, SPEECH_SUSPICIOUS, 0 );
				TIMER_Set( NPC, "interrogating", interrogateTime );
				//G_SetEnemy( NPC, target );
				//NPCInfo->enemyLastSeenTime = level.time;
				//TIMER_Set( NPC, "attackDelay", interrogateTime );
				//TIMER_Set( NPC, "stand", interrogateTime );
				//set desired yaw and pitch towards this guy?
				//FIXME: this is permanent, they will never look away... *sigh*
				NPC_FacePosition( suspect->currentOrigin, qtrue );
				//FIXME: they still need some sort of eye/head tag/bone that can turn?
				//NPC_TempLookTarget( NPC, suspect->s.number, interrogateTime, interrogateTime );
			}
		}
	}
	return qfalse;
}
/*
-------------------------
NPC_ST_InvestigateEvent
-------------------------
*/

#define	MAX_CHECK_THRESHOLD	1

static qboolean NPC_ST_InvestigateEvent( int eventID, bool extraSuspicious )
{
	//If they've given themselves away, just take them as an enemy
	if ( NPCInfo->confusionTime < level.time )
	{
		if ( level.alertEvents[eventID].level == AEL_DISCOVERED && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			//NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
			if ( !level.alertEvents[eventID].owner ||
				!level.alertEvents[eventID].owner->client ||
				level.alertEvents[eventID].owner->health <= 0 ||
				level.alertEvents[eventID].owner->client->playerTeam != NPC->client->enemyTeam )
			{//not an enemy
				return qfalse;
			}
			//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
			//ST_Speech( NPC, SPEECH_CHARGE, 0 );
			G_SetEnemy( NPC, level.alertEvents[eventID].owner );
			NPCInfo->enemyLastSeenTime = level.time;
			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
			if ( level.alertEvents[eventID].type == AET_SOUND )
			{//heard him, didn't see him, stick for a bit
				TIMER_Set( NPC, "roamTime", Q_irand( 500, 2500 ) );
			}
			return qtrue;
		}
	}

	//don't look at the same alert twice
	/*
	if ( level.alertEvents[eventID].ID == NPCInfo->lastAlertID )
	{
		return qfalse;
	}
	NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
	*/

	//Must be ready to take another sound event
	/*
	if ( NPCInfo->investigateSoundDebounceTime > level.time )
	{
		return qfalse;
	}
	*/

	if ( level.alertEvents[eventID].type == AET_SIGHT )
	{//sight alert, check the light level
		if ( level.alertEvents[eventID].light < Q_irand( ST_MIN_LIGHT_THRESHOLD, ST_MAX_LIGHT_THRESHOLD ) )
		{//below my threshhold of potentially seeing
			return qfalse;
		}
	}

	//Save the position for movement (if necessary)
	VectorCopy( level.alertEvents[eventID].position, NPCInfo->investigateGoal );

	//First awareness of it
	NPCInfo->investigateCount += ( extraSuspicious ) ? 2 : 1;

	//Clamp the value
	if ( NPCInfo->investigateCount > 4 )
		NPCInfo->investigateCount = 4;

	//See if we should walk over and investigate
	if ( level.alertEvents[eventID].level > AEL_MINOR && NPCInfo->investigateCount > 1 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
	{
		//make it so they can walk right to this point and look at it rather than having to use combatPoints
		if ( G_ExpandPointToBBox( NPCInfo->investigateGoal, NPC->mins, NPC->maxs, NPC->s.number, ((NPC->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP) ) )
		{//we were able to doMove the investigateGoal to a point in which our bbox would fit
			//drop the goal to the ground so we can get at it
			vec3_t	end;
			trace_t	trace;
			VectorCopy( NPCInfo->investigateGoal, end );
			end[2] -= 512;//FIXME: not always right?  What if it's even higher, somehow?
			gi.trace( &trace, NPCInfo->investigateGoal, NPC->mins, NPC->maxs, end, ENTITYNUM_NONE, ((NPC->clipmask&~CONTENTS_BODY)|CONTENTS_BOTCLIP), (EG2_Collision)0, 0 );
			if ( trace.fraction >= 1.0f )
			{//too high to even bother
				//FIXME: look at them???
			}
			else
			{
				VectorCopy( trace.endpos, NPCInfo->investigateGoal );
				NPC_SetMoveGoal( NPC, NPCInfo->investigateGoal, 16, qtrue );
				NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		else
		{
			int id = NPC_FindCombatPoint( NPCInfo->investigateGoal, NPCInfo->investigateGoal, NPCInfo->investigateGoal, CP_INVESTIGATE|CP_HAS_ROUTE, 0 );

			if ( id != -1 )
			{
				NPC_SetMoveGoal( NPC, level.combatPoints[id].origin, 16, qtrue, id );
				NPCInfo->localState = LSTATE_INVESTIGATE;
			}
		}
		//Say something
		//FIXME: only if have others in group... these should be responses?
		if ( NPCInfo->investigateDebounceTime+NPCInfo->pauseTime > level.time )
		{//was already investigating
			if ( NPCInfo->group &&
				NPCInfo->group->commander &&
				NPCInfo->group->commander->client &&
				NPCInfo->group->commander->client->NPC_class == CLASS_IMPERIAL &&
				!Q_irand( 0, 3 ) )
			{
				ST_Speech( NPCInfo->group->commander, SPEECH_LOOK, 0 );//FIXME: "I'll go check it out" type sounds
			}
			else
			{
				ST_Speech( NPC, SPEECH_LOOK, 0 );//FIXME: "I'll go check it out" type sounds
			}
		}
		else
		{
			if ( level.alertEvents[eventID].type == AET_SIGHT )
			{
				ST_Speech( NPC, SPEECH_SIGHT, 0 );
			}
			else if ( level.alertEvents[eventID].type == AET_SOUND )
			{
				ST_Speech( NPC, SPEECH_SOUND, 0 );
			}
		}
		//Setup the debounce info
		NPCInfo->investigateDebounceTime		= NPCInfo->investigateCount * 5000;
		NPCInfo->investigateSoundDebounceTime	= level.time + 2000;
		NPCInfo->pauseTime						= level.time;
	}
	else
	{//just look?
		//Say something
		if ( level.alertEvents[eventID].type == AET_SIGHT )
		{
			ST_Speech( NPC, SPEECH_SIGHT, 0 );
		}
		else if ( level.alertEvents[eventID].type == AET_SOUND )
		{
			ST_Speech( NPC, SPEECH_SOUND, 0 );
		}
		//Setup the debounce info
		NPCInfo->investigateDebounceTime		= NPCInfo->investigateCount * 1000;
		NPCInfo->investigateSoundDebounceTime	= level.time + 1000;
		NPCInfo->pauseTime						= level.time;
		VectorCopy( level.alertEvents[eventID].position, NPCInfo->investigateGoal );
		if ( NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& !RT_Flying( NPC ) )
		{
			//if ( !Q_irand( 0, 2 ) )
			{//look around
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
		}
	}

	if ( level.alertEvents[eventID].level >= AEL_DANGER )
	{
		NPCInfo->investigateDebounceTime = Q_irand( 500, 2500 );
	}

	//Start investigating
	NPCInfo->tempBehavior = BS_INVESTIGATE;
	return qtrue;
}

/*
-------------------------
ST_OffsetLook
-------------------------
*/

static void ST_OffsetLook( float offset, vec3_t out )
{
	vec3_t	angles, forward, temp;

	GetAnglesForDirection( NPC->currentOrigin, NPCInfo->investigateGoal, angles );
	angles[YAW] += offset;
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( NPC->currentOrigin, 64, forward, out );

	CalcEntitySpot( NPC, SPOT_HEAD, temp );
	out[2] = temp[2];
}

/*
-------------------------
ST_LookAround
-------------------------
*/

static void ST_LookAround( void )
{
	vec3_t	lookPos;
	float	perc = (float) ( level.time - NPCInfo->pauseTime ) / (float) NPCInfo->investigateDebounceTime;

	//Keep looking at the spot
	if ( perc < 0.25 )
	{
		VectorCopy( NPCInfo->investigateGoal, lookPos );
	}
	else if ( perc < 0.5f )		//Look up but straight ahead
	{
		ST_OffsetLook( 0.0f, lookPos );
	}
	else if ( perc < 0.75f )	//Look right
	{
		ST_OffsetLook( 45.0f, lookPos );
	}
	else	//Look left
	{
		ST_OffsetLook( -45.0f, lookPos );
	}

	NPC_FacePosition( lookPos );
}

/*
-------------------------
NPC_BSST_Investigate
-------------------------
*/

void NPC_BSST_Investigate( void )
{
	//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
	AI_GetGroup( NPC );

	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}

	if ( NPCInfo->confusionTime < level.time )
	{
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			//Look for an enemy
			if ( NPC_CheckPlayerTeamStealth() )
			{
				//NPCInfo->behaviorState	= BS_HUNT_AND_KILL;//should be auto now
				ST_Speech( NPC, SPEECH_DETECTED, 0 );
				NPCInfo->tempBehavior	= BS_DEFAULT;
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, NPCInfo->lastAlertID );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			if ( NPCInfo->confusionTime < level.time )
			{
				if ( NPC_CheckForDanger( alertEvent ) )
				{//running like hell
					ST_Speech( NPC, SPEECH_COVER, 0 );//FIXME: flee sound?
					return;
				}
			}

			//if ( level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID )
			{
				NPC_ST_InvestigateEvent( alertEvent, qtrue );
			}
		}
	}

	//If we're done looking, then just return to what we were doing
	if ( ( NPCInfo->investigateDebounceTime + NPCInfo->pauseTime ) < level.time )
	{
		NPCInfo->tempBehavior = BS_DEFAULT;
		NPCInfo->goalEntity = UpdateGoal();

		NPC_UpdateAngles( qtrue, qtrue );
		//Say something
		ST_Speech( NPC, SPEECH_GIVEUP, 0 );
		return;
	}

	//FIXME: else, look for new alerts

	//See if we're searching for the noise's origin
	if ( NPCInfo->localState == LSTATE_INVESTIGATE && (NPCInfo->goalEntity!=NULL) )
	{
		//See if we're there
		if ( !STEER::Reached(NPC, NPCInfo->goalEntity, 32, FlyingCreature(NPC) != qfalse) )
		{
			ucmd.buttons |= BUTTON_WALKING;

			//Try and doMove there
			if ( NPC_MoveToGoal( qtrue )  )
			{
				//Bump our times
				NPCInfo->investigateDebounceTime	= NPCInfo->investigateCount * 5000;
				NPCInfo->pauseTime					= level.time;

				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}

		//Otherwise we're done or have given up
		//Say something
		//ST_Speech( NPC, SPEECH_LOOK, 0.33f );
		NPCInfo->localState = LSTATE_NONE;
	}

	//Look around
	ST_LookAround();
}

/*
-------------------------
NPC_BSST_Patrol
-------------------------
*/

void NPC_BSST_Patrol( void )
{//FIXME: pick up on bodies of dead buddies?

	//Not a scriptflag, but...
	if ( NPC->client->NPC_class == CLASS_ROCKETTROOPER && (NPC->client->ps.eFlags&EF_SPOTLIGHT) )
	{//using spotlight search mode
		vec3_t	eyeFwd, end, mins={-2,-2,-2}, maxs={2,2,2};
		trace_t	trace;
		AngleVectors( NPC->client->renderInfo.eyeAngles, eyeFwd, NULL, NULL );
		VectorMA( NPC->client->renderInfo.eyePoint, NPCInfo->stats.visrange, eyeFwd, end );
		//get server-side trace impact point
		gi.trace( &trace, NPC->client->renderInfo.eyePoint, mins, maxs, end, NPC->s.number, MASK_OPAQUE|CONTENTS_BODY|CONTENTS_CORPSE, (EG2_Collision)0, 0 );
		NPC->speed = (trace.fraction*NPCInfo->stats.visrange);
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			//FIXME: do a FOV cone check, then a trace
			if ( trace.entityNum < ENTITYNUM_WORLD )
			{//hit something
				//try cheap check first
				gentity_t *enemy = &g_entities[trace.entityNum];
				if ( enemy && enemy->client && NPC_ValidEnemy( enemy ) && enemy->client->playerTeam == NPC->client->enemyTeam )
				{
					G_SetEnemy( NPC, enemy );
					TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
					//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
					//NPC_AngerSound();
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
			//FIXME: maybe do a quick check of ents within the spotlight's radius?
			//hmmm, look around
			if ( NPC_CheckEnemiesInSpotlight() )
			{
				//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
				//NPC_AngerSound();
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}
	else
	{
		//get group- mainly for group speech debouncing, but may use for group scouting/investigating AI, too
		AI_GetGroup( NPC );

		if ( NPCInfo->confusionTime < level.time )
		{
			//Look for any enemies
			if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
			{
				if ( NPC_CheckPlayerTeamStealth() )
				{
					//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
					//NPC_AngerSound();
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
		}
	}

	if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			if ( NPC_CheckForDanger( alertEvent ) )
			{//going to run?
				ST_Speech( NPC, SPEECH_COVER, 0 );
				return;
			}
			else if (NPC->client->NPC_class==CLASS_BOBAFETT)
			{
				//NPCInfo->lastAlertID = level.alertEvents[eventID].ID;
				if ( !level.alertEvents[alertEvent].owner ||
					!level.alertEvents[alertEvent].owner->client ||
					level.alertEvents[alertEvent].owner->health <= 0 ||
					level.alertEvents[alertEvent].owner->client->playerTeam != NPC->client->enemyTeam )
				{//not an enemy
					return;
				}
				//FIXME: what if can't actually see enemy, don't know where he is... should we make them just become very alert and start looking for him?  Or just let combat AI handle this... (act as if you lost him)
				//ST_Speech( NPC, SPEECH_CHARGE, 0 );
				G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
				NPCInfo->enemyLastSeenTime = level.time;
				TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
				return;
			}
			else if ( NPC_ST_InvestigateEvent( alertEvent, qfalse ) )
			{//actually going to investigate it
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		//ST_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal( qtrue );
	}
	else// if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		if ( NPC->client->NPC_class != CLASS_IMPERIAL && NPC->client->NPC_class != CLASS_IMPWORKER )
		{//imperials do not look around
			if ( TIMER_Done( NPC, "enemyLastVisible" ) )
			{//nothing suspicious, look around
				if ( !Q_irand( 0, 30 ) )
				{
					NPCInfo->desiredYaw = NPC->s.angles[1] + Q_irand( -90, 90 );
				}
				if ( !Q_irand( 0, 30 ) )
				{
					NPCInfo->desiredPitch = Q_irand( -20, 20 );
				}
			}
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
	//TEMP hack for Imperial stand anim
	if ( NPC->client->NPC_class == CLASS_IMPERIAL
		|| NPC->client->NPC_class == CLASS_IMPWORKER )
	{//hack
		if ( NPC->client->ps.weapon != WP_CONCUSSION )
		{//not Rax
			if ( ucmd.forwardmove || ucmd.rightmove || ucmd.upmove )
			{//moving

				if( (!NPC->client->ps.torsoAnimTimer) || (NPC->client->ps.torsoAnim == BOTH_STAND4) )
				{
					if ( (ucmd.buttons&BUTTON_WALKING) && !(NPCInfo->scriptFlags&SCF_RUNNING) )
					{//not running, only set upper anim
						//  No longer overrides scripted anims
						NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						NPC->client->ps.torsoAnimTimer = 200;
					}
				}
			}
			else
			{//standing still, set both torso and legs anim
				//  No longer overrides scripted anims
				if( ( !NPC->client->ps.torsoAnimTimer || (NPC->client->ps.torsoAnim == BOTH_STAND4) ) &&
					( !NPC->client->ps.legsAnimTimer  || (NPC->client->ps.legsAnim == BOTH_STAND4) ) )
				{
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					NPC->client->ps.torsoAnimTimer = NPC->client->ps.legsAnimTimer = 200;
				}
			}
			//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
			if ( NPC->client->ps.weapon != WP_NONE )
			{
				ChangeWeapon( NPC, WP_NONE );
				NPC->client->ps.weapon = WP_NONE;
				NPC->client->ps.weaponstate = WEAPON_READY;
				G_RemoveWeaponModels( NPC );
			}
		}
	}
}

/*
-------------------------
NPC_BSST_Idle
-------------------------
*/
/*
void NPC_BSST_Idle( void )
{
	int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue );

	//There is an event to look at
	if ( alertEvent >= 0 )
	{
		NPC_ST_InvestigateEvent( alertEvent, qfalse );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	TIMER_Set( NPC, "roamTime", 2000 + Q_irand( 1000, 2000 ) );

	NPC_UpdateAngles( qtrue, qtrue );
}
*/
/*
-------------------------
ST_CheckMoveState
-------------------------
*/

static void ST_CheckMoveState( void )
{
	if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
	{//moving toward a goal that a script is waiting on, so don't stop for anything!
		doMove = qtrue;
	}
	else if ( NPC->client->NPC_class == CLASS_ROCKETTROOPER
		&& NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//no squad stuff
		return;
	}
//	else if ( NPC->NPC->scriptFlags&SCF_NO_GROUPS )
	{
		doMove = qtrue;
	}
	//See if we're a scout

	//See if we're moving towards a goal, not the enemy
	if ( ( NPCInfo->goalEntity != NPC->enemy ) && ( NPCInfo->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( STEER::Reached(NPC, NPCInfo->goalEntity, 16, !!FlyingCreature(NPC)) ||
			(enemyLOS && (NPCInfo->aiFlags&NPCAI_STOP_AT_LOS) && !Q3_TaskIDPending(NPC, TID_MOVE_NAV))
			)
		{//either hit our navgoal or our navgoal was not a crucial (scripted) one (maybe a combat point) and we're scouting and found our enemy
			int	newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch ( NPCInfo->squadState )
			{
			case SQUAD_RETREAT://was running away
				//done fleeing, obviously
				TIMER_Set( NPC, "duck", (NPC->max_health - NPC->health) * 100 );
				TIMER_Set( NPC, "hideTime", Q_irand( 3000, 7000 ) );
				TIMER_Set( NPC, "flee", -level.time );
				newSquadState = SQUAD_COVER;
				break;
			case SQUAD_TRANSITION://was heading for a combat point
				TIMER_Set( NPC, "hideTime", Q_irand( 2000, 4000 ) );
				break;
			case SQUAD_SCOUT://was running after player
				break;
			default:
				break;
			}
			AI_GroupUpdateSquadstates( NPCInfo->group, NPC, newSquadState );
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );	//FIXME: Slant for difficulty levels
			//don't do something else just yet

			// THIS IS THE ONE TRUE PLACE WHERE ROAM TIME IS SET
			TIMER_Set( NPC, "roamTime", Q_irand( 8000, 15000 ) );//Q_irand( 1000, 4000 ) );
			if (Q_irand(0, 3)==0)
			{
				TIMER_Set( NPC, "duck", Q_irand(5000, 10000) );		// just reached our goal, chance of ducking now
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set( NPC, "roamTime", Q_irand( 8000, 9000 ) );
	}
}

void ST_ResolveBlockedShot( int hit )
{
	int	stuckTime;
	//figure out how long we intend to stand here, max
	if ( TIMER_Get( NPC, "roamTime" ) > TIMER_Get( NPC, "stick" ) )
	{
		stuckTime = TIMER_Get( NPC, "roamTime" )-level.time;
	}
	else
	{
		stuckTime = TIMER_Get( NPC, "stick" )-level.time;
	}

	if ( TIMER_Done( NPC, "duck" ) )
	{//we're not ducking
		if ( AI_GroupContainsEntNum( NPCInfo->group, hit ) )
		{
			gentity_t *member = &g_entities[hit];
			if ( TIMER_Done( member, "duck" ) )
			{//they aren't ducking
				if ( TIMER_Done( member, "stand" ) )
				{//they're not being forced to stand
					//tell them to duck at least as long as I'm not moving
					TIMER_Set( member, "duck", stuckTime );	// tell my friend to duck so I can shoot over his head
					return;
				}
			}
		}
	}
	else
	{//maybe we should stand
		if ( TIMER_Done( NPC, "stand" ) )
		{//stand for as long as we'll be here
			TIMER_Set( NPC, "stand", stuckTime );
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to doMove!
	TIMER_Set( NPC, "roamTime", -1 );
	TIMER_Set( NPC, "stick", -1 );
	TIMER_Set( NPC, "duck", -1 );
	TIMER_Set( NPC, "attakDelay", Q_irand( 1000, 3000 ) );
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void ST_CheckFireState( void )
{
	if ( enemyCS )
	{//if have a clear shot, always try
		return;
	}

	if ( NPCInfo->squadState == SQUAD_RETREAT || NPCInfo->squadState == SQUAD_TRANSITION || NPCInfo->squadState == SQUAD_SCOUT )
	{//runners never try to fire at the last pos
		return;
	}

	if ( !VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
	{//if moving at all, don't do this
		return;
	}

	//See if we should continue to fire on their last position
	//!TIMER_Done( NPC, "stick" ) ||
	if ( !hitAlly //we're not going to hit an ally
		&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
		&& NPCInfo->enemyLastSeenTime > 0 //we've seen the enemy
		&& NPCInfo->group //have a group
		&& (NPCInfo->group->numState[SQUAD_RETREAT]>0||NPCInfo->group->numState[SQUAD_TRANSITION]>0||NPCInfo->group->numState[SQUAD_SCOUT]>0) )//laying down covering fire
	{
		if ( level.time - NPCInfo->enemyLastSeenTime < 10000 &&//we have seem the enemy in the last 10 seconds
			(!NPCInfo->group || level.time - NPCInfo->group->lastSeenEnemyTime < 10000 ))//we are not in a group or the group has seen the enemy in the last 10 seconds
		{
			if ( !Q_irand( 0, 10 ) )
			{
				//Fire on the last known position
				vec3_t	muzzle, dir, angles;
				qboolean tooClose = qfalse;
				qboolean tooFar = qfalse;

				CalcEntitySpot( NPC, SPOT_HEAD, muzzle );
				if ( VectorCompare( impactPos, vec3_origin ) )
				{//never checked ShotEntity this frame, so must do a trace...
					trace_t tr;
					//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
					vec3_t	forward, end;
					AngleVectors( NPC->client->ps.viewangles, forward, NULL, NULL );
					VectorMA( muzzle, 8192, forward, end );
					gi.trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
					VectorCopy( tr.endpos, impactPos );
				}

				//see if impact would be too close to me
				float distThreshold = 16384/*128*128*/;//default
				switch ( NPC->s.weapon )
				{
				case WP_ROCKET_LAUNCHER:
				case WP_FLECHETTE:
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					distThreshold = 65536/*256*256*/;
					break;
				case WP_REPEATER:
					if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				case WP_CONCUSSION:
					if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				float dist = DistanceSquared( impactPos, muzzle );

				if ( dist < distThreshold )
				{//impact would be too close to me
					tooClose = qtrue;
				}
				else if ( level.time - NPCInfo->enemyLastSeenTime > 5000 ||
					(NPCInfo->group && level.time - NPCInfo->group->lastSeenEnemyTime > 5000 ))
				{//we've haven't seen them in the last 5 seconds
					//see if it's too far from where he is
					distThreshold = 65536/*256*256*/;//default
					switch ( NPC->s.weapon )
					{
					case WP_ROCKET_LAUNCHER:
					case WP_FLECHETTE:
					case WP_THERMAL:
					case WP_TRIP_MINE:
					case WP_DET_PACK:
						distThreshold = 262144/*512*512*/;
						break;
					case WP_REPEATER:
						if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
						{
							distThreshold = 262144/*512*512*/;
						}
						break;
					case WP_CONCUSSION:
						if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
						{
							distThreshold = 262144/*512*512*/;
						}
						break;
					default:
						break;
					}
					dist = DistanceSquared( impactPos, NPCInfo->enemyLastSeenLocation );
					if ( dist > distThreshold )
					{//impact would be too far from enemy
						tooFar = qtrue;
					}
				}

				if ( !tooClose && !tooFar )
				{//okay too shoot at last pos
					VectorSubtract( NPCInfo->enemyLastSeenLocation, muzzle, dir );
					VectorNormalize( dir );
					vectoangles( dir, angles );

					NPCInfo->desiredYaw		= angles[YAW];
					NPCInfo->desiredPitch	= angles[PITCH];

					shoot = qtrue;
					faceEnemy = qfalse;
					//AI_GroupUpdateSquadstates( NPCInfo->group, NPC, SQUAD_STAND_AND_SHOOT );
					return;
				}
			}
		}
	}
}

void ST_TrackEnemy( gentity_t *self, vec3_t enemyPos )
{
	//clear timers
	TIMER_Set( self, "attackDelay", Q_irand( 1000, 2000 ) );
	//TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", Q_irand( 500, 1500 ) );
	TIMER_Set( self, "stand", -1 );
	TIMER_Set( self, "scoutTime", TIMER_Get( self, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint( self->NPC->combatPoint );
	//go after his last seen pos
	NPC_SetMoveGoal( self, enemyPos, 100.0f, qfalse );
	if (Q_irand(0,3)==0)
	{
		NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
	}
}

int ST_ApproachEnemy( gentity_t *self )
{
	TIMER_Set( self, "attackDelay", Q_irand( 250, 500 ) );
	//TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", Q_irand( 1000, 2000 ) );
	TIMER_Set( self, "stand", -1 );
	TIMER_Set( self, "scoutTime", TIMER_Get( self, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint( self->NPC->combatPoint );
	//return the relevant combat point flags
	return (CP_CLEAR|CP_CLOSEST);
}

void ST_HuntEnemy( gentity_t *self )
{
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );//Disabled this for now, guys who couldn't hunt would never attack
	//TIMER_Set( NPC, "duck", -1 );
	TIMER_Set( NPC, "stick", Q_irand( 250, 1000 ) );
	TIMER_Set( NPC, "stand", -1 );
	TIMER_Set( NPC, "scoutTime", TIMER_Get( NPC, "stick" )-level.time+Q_irand(5000, 10000) );
	//leave my combat point
	NPC_FreeCombatPoint( NPCInfo->combatPoint );
	//go directly after the enemy
	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		self->NPC->goalEntity = NPC->enemy;
	}
}

void ST_TransferTimers( gentity_t *self, gentity_t *other )
{
	TIMER_Set( other, "attackDelay", TIMER_Get( self, "attackDelay" )-level.time );
	TIMER_Set( other, "duck", TIMER_Get( self, "duck" )-level.time );
	TIMER_Set( other, "stick", TIMER_Get( self, "stick" )-level.time );
	TIMER_Set( other, "scoutTime", TIMER_Get( self, "scoutTime" )-level.time );
	TIMER_Set( other, "roamTime", TIMER_Get( self, "roamTime" )-level.time );
	TIMER_Set( other, "stand", TIMER_Get( self, "stand" )-level.time );
	TIMER_Set( self, "attackDelay", -1 );
	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stick", -1 );
	TIMER_Set( self, "scoutTime", -1 );
	TIMER_Set( self, "roamTime", -1 );
	TIMER_Set( self, "stand", -1 );
}

void ST_TransferMoveGoal( gentity_t *self, gentity_t *other )
{
	if ( Q3_TaskIDPending( self, TID_MOVE_NAV ) )
	{//can't transfer movegoal when a script we're running is waiting to complete
		return;
	}
	if ( self->NPC->combatPoint != -1 )
	{//I've got a combatPoint I'm going to, give it to him
		self->NPC->lastFailedCombatPoint = other->NPC->combatPoint = self->NPC->combatPoint;
		self->NPC->combatPoint = -1;
	}
	else
	{//I must be going for a goal, give that to him instead
		if ( self->NPC->goalEntity == self->NPC->tempGoal )
		{
			NPC_SetMoveGoal( other, self->NPC->tempGoal->currentOrigin, self->NPC->goalRadius, (qboolean)((self->NPC->tempGoal->svFlags & SVF_NAVGOAL) != 0) );
		}
		else
		{
			other->NPC->goalEntity = self->NPC->goalEntity;
		}
	}
	//give him my squadstate
	AI_GroupUpdateSquadstates( self->NPC->group, other, NPCInfo->squadState );

	//give him my timers and clear mine
	ST_TransferTimers( self, other );

	//now make me stand around for a second or two at least
	AI_GroupUpdateSquadstates( self->NPC->group, self, SQUAD_STAND_AND_SHOOT );
	TIMER_Set( self, "stand", Q_irand( 1000, 3000 ) );
}

int ST_GetCPFlags( void )
{
	int cpFlags = 0;
	if ( NPC && NPCInfo->group )
	{
		if ( NPC == NPCInfo->group->commander && NPC->client->NPC_class == CLASS_IMPERIAL )
		{//imperials hang back and give orders
			if ( NPCInfo->group->numGroup > 1 && Q_irand( -3, NPCInfo->group->numGroup ) > 1 )
			{//FIXME: make sure he;s giving orders with these lines
				if ( Q_irand( 0, 1 ) )
				{
					ST_Speech( NPC, SPEECH_CHASE, 0.5 );
				}
				else
				{
					ST_Speech( NPC, SPEECH_YELL, 0.5 );
				}
			}
			cpFlags = (CP_CLEAR|CP_COVER|CP_AVOID|CP_SAFE|CP_RETREAT);
		}
		else if ( NPCInfo->group->morale < 0 )
		{//hide
			cpFlags = (CP_COVER|CP_AVOID|CP_SAFE|CP_RETREAT);
			/*
			if ( NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand( 0, 3 ) )
			{
				Saboteur_Cloak( NPC );
			}
			*/
		}
/*		else if ( NPCInfo->group->morale < NPCInfo->group->numGroup )
		{//morale is low for our size
			int moraleDrop = NPCInfo->group->numGroup - NPCInfo->group->morale;
			if ( moraleDrop < -6 )
			{//flee (no clear shot needed)
				cpFlags = (CP_FLEE|CP_RETREAT|CP_COVER|CP_AVOID|CP_SAFE);
			}
			else if ( moraleDrop < -3 )
			{//retreat (no clear shot needed)
				cpFlags = (CP_RETREAT|CP_COVER|CP_AVOID|CP_SAFE);
			}
			else if ( moraleDrop < 0 )
			{//cover (no clear shot needed)
				cpFlags = (CP_COVER|CP_AVOID|CP_SAFE);
			}
		}*/
		else
		{
			int moraleBoost = NPCInfo->group->morale - NPCInfo->group->numGroup;
			if ( moraleBoost > 20 )
			{//charge to any one and outflank (no cover needed)
				cpFlags = (CP_CLEAR|CP_FLANK|CP_APPROACH_ENEMY);
				//Saboteur_Decloak( NPC );
			}
			else if ( moraleBoost > 15 )
			{//charge to closest one (no cover needed)
				cpFlags = (CP_CLEAR|CP_CLOSEST|CP_APPROACH_ENEMY);
				/*
				if ( NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand( 0, 3 ) )
				{
					Saboteur_Decloak( NPC );
				}
				*/
			}
			else if ( moraleBoost > 10 )
			{//charge closer (no cover needed)
				cpFlags = (CP_CLEAR|CP_APPROACH_ENEMY);
				/*
				if ( NPC->client->NPC_class == CLASS_SABOTEUR && !Q_irand( 0, 6 ) )
				{
					Saboteur_Decloak( NPC );
				}
				*/
			}
		}
	}
	if ( !cpFlags )
	{
		//at some medium level of morale
		switch( Q_irand( 0, 3 ) )
		{
		case 0://just take the nearest one
			cpFlags = (CP_CLEAR|CP_COVER|CP_NEAREST);
			break;
		case 1://take one closer to the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_APPROACH_ENEMY);
			break;
		case 2://take the one closest to the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_CLOSEST|CP_APPROACH_ENEMY);
			break;
		case 3://take the one on the other side of the enemy
			cpFlags = (CP_CLEAR|CP_COVER|CP_FLANK|CP_APPROACH_ENEMY);
			break;
		}
	}
	if ( NPC && (NPCInfo->scriptFlags&SCF_USE_CP_NEAREST) )
	{
		cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
		cpFlags |= CP_NEAREST;
	}
	return cpFlags;
}

/*
-------------------------
ST_Commander

  Make decisions about who should go where, etc.

FIXME: leader (group-decision-making) AI?
FIXME: need alternate routes!
FIXME: more group voice interaction
FIXME: work in pairs?

-------------------------
*/
void ST_Commander( void )
{
	int		i;//, j;
	int		cp, cpFlags;
	AIGroupInfo_t	*group = NPCInfo->group;
	gentity_t	*member;//, *buddy;
	qboolean	enemyLost = qfalse;
	float		avoidDist;

	group->processed = qtrue;

	if ( group->enemy == NULL || group->enemy->client == NULL )
	{//hmm, no enemy...?!
		return;
	}

	//FIXME: have this group commander check the enemy group (if any) and see if they have
	//		superior numbers.  If they do, fall back rather than advance.  If you have
	//		superior numbers, advance on them.
	//FIXME: find the group commander and have him occasionally give orders when there is speech
	//FIXME: start fleeing when only a couple of you vs. a lightsaber, possibly give up if the only one left

	SaveNPCGlobals();

	if ( group->lastSeenEnemyTime < level.time - 180000 )
	{//dissolve the group
		ST_Speech( NPC, SPEECH_LOST, 0.0f );
		group->enemy->waypoint = NAV::GetNearestNode(group->enemy);
		for ( i = 0; i < group->numGroup; i++ )
		{
			member = &g_entities[group->member[i].number];
			SetNPCGlobals( member );
			if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
			{//running somewhere that a script requires us to go, don't break from that
				continue;
			}
			if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
			{//not allowed to doMove on my own
				continue;
			}
			//Lost enemy for three minutes?  go into search mode?
			G_ClearEnemy( NPC );
			NPC->waypoint = NAV::GetNearestNode(group->enemy);
			if ( NPC->waypoint == WAYPOINT_NONE )
			{
				NPCInfo->behaviorState = BS_DEFAULT;//BS_PATROL;
			}
			else if ( group->enemy->waypoint == WAYPOINT_NONE || (NAV::EstimateCostToGoal( NPC->waypoint, group->enemy->waypoint ) >= Q3_INFINITE) )
			{
				NPC_BSSearchStart( NPC->waypoint, BS_SEARCH );
			}
			else
			{
				NPC_BSSearchStart( group->enemy->waypoint, BS_SEARCH );
			}
		}
		group->enemy = NULL;
		RestoreNPCGlobals();
		return;
	}

	if ( /*!runner &&*/ group->lastSeenEnemyTime > level.time - 32000 && group->lastSeenEnemyTime < level.time - 30000 )
	{//no-one has seen the enemy for 30 seconds// and no-one is running after him
		if ( group->commander && !Q_irand( 0, 1 ) )
		{
			ST_Speech( group->commander, SPEECH_ESCAPING, 0.0f );
		}
		else
		{
			ST_Speech( NPC, SPEECH_ESCAPING, 0.0f );
		}
		//don't say this again
		NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
	}

	if ( group->lastSeenEnemyTime < level.time - 7000 )
	{//no-one has seen the enemy for at least 10 seconds!  Should send a scout
		enemyLost = qtrue;
	}

	//Go through the list:

	//Everyone should try to get to a combat point if possible
	int	curMemberNum, lastMemberNum;
	if ( d_asynchronousGroupAI->integer )
	{//do one member a turn
		group->activeMemberNum++;
		if ( group->activeMemberNum >= group->numGroup )
		{
			group->activeMemberNum = 0;
		}
		curMemberNum = group->activeMemberNum;
		lastMemberNum = curMemberNum + 1;
	}
	else
	{
		curMemberNum = 0;
		lastMemberNum = group->numGroup;
	}
	for ( i = curMemberNum; i < lastMemberNum; i++ )
	{
		//reset combat point flags
		cp = -1;
		cpFlags = 0;
		avoidDist = 0;

		//get the next guy
		member = &g_entities[group->member[i].number];
		if ( !member->enemy )
		{//don't include guys that aren't angry
			continue;
		}
		SetNPCGlobals( member );

		if ( !TIMER_Done( NPC, "flee" ) )
		{//running away
			continue;
		}

		if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
		{//running somewhere that a script requires us to go
			continue;
		}

		if ( NPC->s.weapon == WP_NONE
			&& NPCInfo->goalEntity
			&& NPCInfo->goalEntity == NPCInfo->tempGoal
			&& NPCInfo->goalEntity->s.eType == ET_ITEM )
		{//running to pick up a gun, don't do other logic
			continue;
		}


		if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
		{//not allowed to do combat-movement
			continue;
		}


		if ( NPC->client->ps.weapon == WP_NONE )
		{//weaponless, should be hiding
			if ( NPCInfo->goalEntity == NULL || NPCInfo->goalEntity->enemy == NULL || NPCInfo->goalEntity->enemy->s.eType != ET_ITEM )
			{//not running after a pickup
				if ( TIMER_Done( NPC, "hideTime" ) || (DistanceSquared( group->enemy->currentOrigin, NPC->currentOrigin ) < 65536 && NPC_ClearLOS( NPC->enemy )) )
				{//done hiding or enemy near and can see us
					//er, start another flee I guess?
					NPC_StartFlee( NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
				}//else, just hang here
			}
			continue;
		}

		if (enemyLost && NAV::InSameRegion(NPC, NPC->enemy->currentOrigin))
		{
			ST_TrackEnemy( NPC, NPC->enemy->currentOrigin );
			continue;
		}

		if (!NPC->enemy)
		{
			continue;
		}


		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done( NPC, "checkGrenadeTooCloseDebouncer" ))
		{
			TIMER_Set (NPC, "checkGrenadeTooCloseDebouncer", Q_irand(300, 600));

			vec3_t		mins;
			vec3_t		maxs;
			bool		fled = false;
			gentity_t*	ent;

			gentity_t	*entityList[MAX_GENTITIES];

			for (int i = 0 ; i < 3 ; i++ )
			{
				mins[i] = NPC->currentOrigin[i] - 200;
				maxs[i] = NPC->currentOrigin[i] + 200;
			}

			int	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

			for (int e = 0 ; e < numListedEntities ; e++ )
			{
				ent = entityList[ e ];

				if (ent == NPC)
					continue;
				if (ent->owner == NPC)
					continue;
				if ( !(ent->inuse) )
					continue;
				if ( ent->s.eType == ET_MISSILE )
				{
					if ( ent->s.weapon == WP_THERMAL )
					{//a thermal
						if ( ent->has_bounced && (!ent->owner || !OnSameTeam(ent->owner, NPC)))
						{//bounced and an enemy thermal
							ST_Speech( NPC, SPEECH_COVER, 0 );//FIXME: flee sound?
							NPC_StartFlee(NPC->enemy, ent->currentOrigin, AEL_DANGER_GREAT, 1000, 2000);
							fled = true;
//							cpFlags |= (CP_CLEAR|CP_COVER);	// NOPE, Can't See The Enemy, So Find A New Combat Point
							TIMER_Set (NPC, "checkGrenadeTooCloseDebouncer", Q_irand(2000, 4000));
							break;
						}
					}
				}
			}
			if (fled)
			{
				continue;
			}
		}


		// Check To See We Have A Clear Shot To The Enemy Every Couple Seconds
		//---------------------------------------------------------------------
		if (TIMER_Done( NPC, "checkEnemyVisDebouncer" ))
		{
			TIMER_Set (NPC, "checkEnemyVisDebouncer", Q_irand(3000, 7000));
			if (!NPC_ClearLOS(NPC->enemy))
			{
				cpFlags |= (CP_CLEAR|CP_COVER);	// NOPE, Can't See The Enemy, So Find A New Combat Point
			}
		}

		// Check To See If The Enemy Is Too Close For Comfort
		//----------------------------------------------------
		if (NPC->client->NPC_class!=CLASS_ASSASSIN_DROID)
		{
			if (TIMER_Done(NPC, "checkEnemyTooCloseDebouncer"))
			{
				TIMER_Set (NPC, "checkEnemyTooCloseDebouncer", Q_irand(1000, 6000));

				float distThreshold = 16384/*128*128*/;//default
				switch ( NPC->s.weapon )
				{
				case WP_ROCKET_LAUNCHER:
				case WP_FLECHETTE:
				case WP_THERMAL:
				case WP_TRIP_MINE:
				case WP_DET_PACK:
					distThreshold = 65536/*256*256*/;
					break;
				case WP_REPEATER:
					if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				case WP_CONCUSSION:
					if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
					{
						distThreshold = 65536/*256*256*/;
					}
					break;
				default:
					break;
				}

				if ( DistanceSquared( group->enemy->currentOrigin, NPC->currentOrigin ) < distThreshold )
				{
					cpFlags |= (CP_CLEAR|CP_COVER);
				}
			}
		}


		//clear the local state
		NPCInfo->localState = LSTATE_NONE;

		cpFlags &= ~CP_NEAREST;
		//Assign combat points
		if ( cpFlags )
		{//we want to run to a combat point
			//always avoid enemy when picking combat points, and we always want to be able to get there
			cpFlags		|= CP_AVOID_ENEMY|CP_HAS_ROUTE|CP_TRYFAR;
			avoidDist	 = 200;

			//now get a combat point
			if ( cp == -1 )
			{//may have had sone set above
				cp = NPC_FindCombatPointRetry( NPC->currentOrigin, NPC->currentOrigin, NPC->currentOrigin, &cpFlags, avoidDist, NPCInfo->lastFailedCombatPoint );
			}

			//see if we got a valid one
			if ( cp != -1 )
			{//found a combat point
				//let others know that someone is now running
				//don't change course again until we get to where we're going
				TIMER_Set( NPC, "roamTime", Q3_INFINITE );


				NPC_SetCombatPoint( cp );
				NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp );

				// If Successfully
				if ((cpFlags&CP_FLANK) || ((cpFlags&CP_COVER) && (cpFlags&CP_CLEAR)))
				{
				}
				else if (Q_irand(0,3)==0)
				{
					NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
				}

				//okay, try a doMove right now to see if we can even get there
				if ( (cpFlags&CP_FLANK) )
				{
					if ( group->numGroup > 1 )
					{
						NPC_ST_StoreMovementSpeech( SPEECH_OUTFLANK, -1 );
					}
				}
				else if ( (cpFlags&CP_COVER) && !(cpFlags&CP_CLEAR) )
				{//going into hiding
					NPC_ST_StoreMovementSpeech( SPEECH_COVER, -1 );
				}
				else
				{
					if ( !Q_irand( 0, 20 ) )
					{//hell, we're loading the sounds, use them every now and then!
						if ( Q_irand( 0, 1 ) )
						{
							NPC_ST_StoreMovementSpeech( SPEECH_OUTFLANK, -1 );
						}
						else
						{
							NPC_ST_StoreMovementSpeech( SPEECH_ESCAPING, -1 );
						}
					}
				}
			}
		}
	}

	RestoreNPCGlobals();
	return;
}

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
void Noghri_StickTrace( void )
{
	if ( !NPC->ghoul2.size()
		|| NPC->weaponModel[0] <= 0 )
	{
		return;
	}

	int			boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[0]], "*weapon");
	if ( boltIndex != -1 )
	{
		int curTime = (cg.time?cg.time:level.time);
		qboolean hit = qfalse;
		int	lastHit = ENTITYNUM_NONE;
		for ( int time = curTime-25; time <= curTime+25&&!hit; time += 25 )
		{
			mdxaBone_t	boltMatrix;
			vec3_t		tip, dir, base, angles={0,NPC->currentAngles[YAW],0};
			vec3_t		mins={-2,-2,-2},maxs={2,2,2};
			trace_t		trace;

			gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->weaponModel[0],
						boltIndex,
						&boltMatrix, angles, NPC->currentOrigin, time,
						NULL, NPC->s.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, base );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, dir );
			VectorMA( base, 48, dir, tip );
	#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				G_DebugLine(base, tip, FRAMETIME, 0x000000ff, qtrue);
			}
	#endif
			gi.trace( &trace, base, mins, maxs, tip, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
			if ( trace.fraction < 1.0f && trace.entityNum != lastHit )
			{//hit something
				gentity_t *traceEnt = &g_entities[trace.entityNum];
				if ( traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->NPC_class) )
				{//smack
					int dmg = Q_irand( 12, 20 );//FIXME: base on skill!
					//FIXME: debounce?
					G_Sound( traceEnt, G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", Q_irand( 1, 4 ) ) ) );
					G_Damage( traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
					if ( traceEnt->health > 0 && dmg > 17 )
					{//do pain on enemy
						G_Knockdown( traceEnt, NPC, dir, 300, qtrue );
					}
					lastHit = trace.entityNum;
					hit = qtrue;
				}
			}
		}
	}
}
/*
-------------------------
NPC_BSST_Attack
-------------------------
*/

void NPC_BSST_Attack( void )
{
	//Don't do anything if we're hurt
	if ( NPC->painDebounceTime > level.time )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt() == qfalse )//!NPC->enemy )//
	{
		if( NPC->client->playerTeam == TEAM_PLAYER )
		{
			NPC_BSPatrol();
		}
		else
		{
			NPC_BSST_Patrol();//FIXME: or patrol?
		}
		return;
	}

	//FIXME: put some sort of delay into the guys depending on how they saw you...?

	//Get our group info
	if ( TIMER_Done( NPC, "interrogating" ) )
	{
		AI_GetGroup( NPC );//, 45, 512, NPC->enemy );
	}
	else
	{
		//FIXME: when done interrogating, I should send out a team alert!
	}

	if ( NPCInfo->group )
	{//I belong to a squad of guys - we should *always* have a group
		if ( !NPCInfo->group->processed )
		{//I'm the first ent in my group, I'll make the command decisions
#if	AI_TIMERS
			int	startTime = GetTime(0);
#endif//	AI_TIMERS
			ST_Commander();
#if	AI_TIMERS
			int commTime = GetTime ( startTime );
			if ( commTime > 20 )
			{
				gi.Printf( S_COLOR_RED"ERROR: Commander time: %d\n", commTime );
			}
			else if ( commTime > 10 )
			{
				gi.Printf( S_COLOR_YELLOW"WARNING: Commander time: %d\n", commTime );
			}
			else if ( commTime > 2 )
			{
				gi.Printf( S_COLOR_GREEN"Commander time: %d\n", commTime );
			}
#endif//	AI_TIMERS
		}
	}
	else if ( TIMER_Done( NPC, "flee" ) && NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{//not already fleeing, and going to run
		ST_Speech( NPC, SPEECH_COVER, 0 );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( !NPC->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSST_Patrol();//FIXME: or patrol?
		return;
	}

	if (NPCInfo->goalEntity && NPCInfo->goalEntity!=NPC->enemy)
	{
        NPCInfo->goalEntity = UpdateGoal();
	}


	enemyLOS = enemyCS = enemyInFOV = qfalse;
	doMove = qtrue;
	faceEnemy = qfalse;
	shoot = qfalse;
	hitAlly = qfalse;
	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );

	vec3_t	enemyDir, shootDir;
	VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPC->client->ps.viewangles, shootDir, NULL, NULL );
	float dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (NPC->client->ps.weapon == WP_FLECHETTE || NPC->client->ps.weapon == WP_REPEATER) &&
			(NPCInfo->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}
	else if ( enemyDist > 65536 )//256 squared
	{
		if ( NPC->client->ps.weapon == WP_DISRUPTOR )
		{//sniping...
			if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon( NPC->client->ps.weapon );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	//can we see our target?
	if ( NPC_ClearLOS( NPC->enemy ) )
	{
		AI_GroupUpdateEnemyLastSeen( NPCInfo->group, NPC->enemy->currentOrigin );
		NPCInfo->enemyLastSeenTime = level.time;
		enemyLOS = qtrue;

		if ( NPC->client->ps.weapon == WP_NONE )
		{
			enemyCS = qfalse;//not true, but should stop us from firing
			NPC_AimAdjust( -1 );//adjust aim worse longer we have no weapon
		}
		else
		{//can we shoot our target?
			if ((enemyDist < MIN_ROCKET_DIST_SQUARED) &&
				((level.time - NPC->lastMoveTime)<5000) &&
				(
				(NPC->client->ps.weapon == WP_ROCKET_LAUNCHER
				|| (NPC->client->ps.weapon == WP_CONCUSSION && !(NPCInfo->scriptFlags&SCF_ALT_FIRE))
				|| (NPC->client->ps.weapon == WP_FLECHETTE && (NPCInfo->scriptFlags&SCF_ALT_FIRE)))))
			{
				enemyCS = qfalse;//not true, but should stop us from firing
				hitAlly = qtrue;//us!
				//FIXME: if too close, run away!
			}
			else if ( enemyInFOV )
			{//if enemy is FOV, go ahead and check for shooting
				int hit = NPC_ShotEntity( NPC->enemy, impactPos );
				gentity_t *hitEnt = &g_entities[hit];

				if ( hit == NPC->enemy->s.number
					|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
					|| ( hitEnt && hitEnt->takedamage && ((hitEnt->svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPC->s.weapon == WP_EMPLACED_GUN) ) )
				{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
					AI_GroupUpdateClearShotTime( NPCInfo->group );
					enemyCS = qtrue;
					NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
					VectorCopy( NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation );
				}
				else
				{//Hmm, have to get around this bastard
					NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
					ST_ResolveBlockedShot( hit );
					if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam )
					{//would hit an ally, don't fire!!!
						hitAlly = qtrue;
					}
					else
					{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
					}
				}
			}
			else
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
		}
	}
	else if ( gi.inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
	}

	if ( NPC->client->ps.weapon == WP_NONE )
	{
		faceEnemy = qfalse;
		shoot = qfalse;
	}
	else
	{
		if ( enemyLOS )
		{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
			faceEnemy = qtrue;
		}
		if ( enemyCS )
		{
			shoot = qtrue;
		}
	}

	//Check for movement to take care of
	ST_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	ST_CheckFireState();

	if ( faceEnemy )
	{//face the enemy
		NPC_FaceEnemy( qtrue );
	}

	if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
	{//not supposed to chase my enemies
		if ( NPCInfo->goalEntity == NPC->enemy )
		{//goal is my entity, so don't doMove
			doMove = qfalse;
		}
	}
	else if (NPC->NPC->scriptFlags&SCF_NO_GROUPS)
	{
			//	NPCInfo->goalEntity = UpdateGoal();

 		NPCInfo->goalEntity = (enemyLOS)?(0):(NPC->enemy);
	}

	if ( NPC->client->fireDelay && NPC->s.weapon == WP_ROCKET_LAUNCHER )
	{
		doMove = qfalse;
	}

	if ( !ucmd.rightmove )
	{//only if not already strafing for some strange reason...?
		//NOTE: these are never set here, but can be set in AI_Jedi.cpp for those NPCs who are sort of Stormtrooper/Jedi hybrids
		//NOTE: this stomps navigation movement entirely!
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if ( !TIMER_Done( NPC, "strafeLeft" ) )
		{
			/*
			if ( NPCInfo->desiredYaw > NPC->client->ps.viewangles[YAW] + 60 )
			{//we want to turn left, don't apply the strafing
			}
			else
			*/
			{//go ahead and strafe left
				ucmd.rightmove = -127;
				//re-check the duck as we might want to be rolling
				VectorClear( NPC->client->ps.moveDir );
				doMove = qfalse;
			}
		}
		else if ( !TIMER_Done( NPC, "strafeRight" ) )
		{
			/*if ( NPCInfo->desiredYaw < NPC->client->ps.viewangles[YAW] - 60 )
			{//we want to turn right, don't apply the strafing
			}
			else
			*/
			{//go ahead and strafe left
				ucmd.rightmove = 127;
				VectorClear( NPC->client->ps.moveDir );
				doMove = qfalse;
			}
		}
	}

	if ( NPC->client->ps.legsAnim == BOTH_GUARD_LOOKAROUND1 )
	{//don't doMove when doing silly look around thing
		doMove = qfalse;
	}
	if ( doMove )
	{//doMove toward goal
		if ( NPCInfo->goalEntity )//&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist > 10000 ) )//100 squared
		{
			doMove = ST_Move();
			if ( (NPC->client->NPC_class != CLASS_ROCKETTROOPER||NPC->s.weapon!=WP_ROCKET_LAUNCHER||enemyDist<MIN_ROCKET_DIST_SQUARED)//rockettroopers who use rocket launchers turn around and run if you get too close (closer than 128)
				&& ucmd.forwardmove <= -32 )
			{//moving backwards at least 45 degrees
				if ( NPCInfo->goalEntity
					&& DistanceSquared( NPCInfo->goalEntity->currentOrigin, NPC->currentOrigin ) > MIN_TURN_AROUND_DIST_SQ )
				{//don't stop running backwards if your goal is less than 100 away
					if ( TIMER_Done( NPC, "runBackwardsDebounce" ) )
					{//not already waiting for next run backwards
						if ( !TIMER_Exists( NPC, "runningBackwards" ) )
						{//start running backwards
							TIMER_Set( NPC, "runningBackwards", Q_irand( 500, 1000 ) );//Q_irand( 2000, 3500 ) );
						}
						else if ( TIMER_Done2( NPC, "runningBackwards", qtrue ) )
						{//done running backwards
							TIMER_Set( NPC, "runBackwardsDebounce", Q_irand( 3000, 5000 ) );
						}
					}
				}
			}
			else
			{//not running backwards
				//TIMER_Remove( NPC, "runningBackwards" );
			}
		}
		else
		{
			doMove = qfalse;
		}
	}

	if ( !doMove )
	{
		if (NPC->client->NPC_class != CLASS_ASSASSIN_DROID)
		{
			if ( !TIMER_Done( NPC, "duck" ) )
			{
				ucmd.upmove = -127;
			}
		}
		//FIXME: what about leaning?
	}
	else
	{//stop ducking!
		TIMER_Set( NPC, "duck", -1 );
	}

	if ( NPC->client->NPC_class == CLASS_REBORN//cultist using a gun
		&& NPCInfo->rank >= RANK_LT_COMM //commando or better
		&& NPC->enemy->s.weapon == WP_SABER )//fighting a saber-user
	{//commando saboteur vs. jedi/reborn
		//see if we need to avoid their saber
		NPC_EvasionSaber();
	}

	if ( //!TIMER_Done( NPC, "flee" ) ||
		(doMove&&!TIMER_Done( NPC, "runBackwardsDebounce" )) )
	{//running away
		faceEnemy = qfalse;
	}

	//FIXME: check scf_face_move_dir here?

	if ( !faceEnemy )
	{//we want to face in the dir we're running
		if ( !doMove )
		{//if we haven't moved, we should look in the direction we last looked?
			VectorCopy( NPC->client->ps.viewangles, NPCInfo->lastPathAngles );
		}
		NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
		NPCInfo->desiredPitch = 0;
		NPC_UpdateAngles( qtrue, qtrue );
		if ( doMove )
		{//don't run away and shoot
			shoot = qfalse;
		}
	}

	if ( NPCInfo->scriptFlags & SCF_DONT_FIRE )
	{
		shoot = qfalse;
	}

	if ( NPC->enemy && NPC->enemy->enemy )
	{
		if ( NPC->enemy->s.weapon == WP_SABER && NPC->enemy->enemy->s.weapon == WP_SABER )
		{//don't shoot at an enemy jedi who is fighting another jedi, for fear of injuring one or causing rogue blaster deflections (a la Obi Wan/Vader duel at end of ANH)
			shoot = qfalse;
		}
	}
	//FIXME: don't shoot right away!
	if ( NPC->client->fireDelay )
	{
		if ( NPC->client->NPC_class == CLASS_SABOTEUR )
		{
			Saboteur_Decloak( NPC );
		}
		if ( NPC->s.weapon == WP_ROCKET_LAUNCHER
			|| (NPC->s.weapon==WP_CONCUSSION&&!(NPCInfo->scriptFlags&SCF_ALT_FIRE)) )
		{
			if ( !enemyLOS || !enemyCS )
			{//cancel it
				NPC->client->fireDelay = 0;
			}
			else
			{//delay our next attempt
				TIMER_Set( NPC, "attackDelay", Q_irand( 3000, 5000 ) );
			}
		}
	}
	else if ( shoot )
	{//try to shoot if it's time
		if ( NPC->client->NPC_class == CLASS_SABOTEUR )
		{
			Saboteur_Decloak( NPC );
		}
		if ( TIMER_Done( NPC, "attackDelay" ) )
		{
			if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
			{
				WeaponThink( qtrue );
			}
			//NASTY
			if ( NPC->s.weapon == WP_ROCKET_LAUNCHER )
			{
				if ( (ucmd.buttons&BUTTON_ATTACK)
					&& !doMove
					&& g_spskill->integer > 1
					&& !Q_irand( 0, 3 ) )
				{//every now and then, shoot a homing rocket
					ucmd.buttons &= ~BUTTON_ATTACK;
					ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPC->client->fireDelay = Q_irand( 1000, 2500 );
				}
			}
			else if ( NPC->s.weapon == WP_NOGHRI_STICK
				&& enemyDist < (48*48) )//?
			{
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |= BUTTON_ALT_ATTACK;
				NPC->client->fireDelay = Q_irand( 1500, 2000 );
			}
		}
	}
	else
	{
		if ( NPC->attackDebounceTime < level.time )
		{
			if ( NPC->client->NPC_class == CLASS_SABOTEUR )
			{
				Saboteur_Cloak( NPC );
			}
		}
	}
}

extern qboolean G_TuskenAttackAnimDamage( gentity_t *self );
void NPC_BSST_Default( void )
{
	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}

	if ( NPC->s.weapon == WP_NOGHRI_STICK )
	{
		if ( G_TuskenAttackAnimDamage( NPC ) )
		{
			Noghri_StickTrace();
		}
	}

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		NPC_BSST_Patrol();
	}
	else //if ( NPC->enemy )
	{//have an enemy
		if ( NPC->enemy->client //enemy is a client
			&& (NPC->enemy->client->NPC_class == CLASS_UGNAUGHT || NPC->enemy->client->NPC_class == CLASS_JAWA )//enemy is a lowly jawa or ugnaught
			&& NPC->enemy->enemy != NPC//enemy's enemy is not me
			&& (!NPC->enemy->enemy || !NPC->enemy->enemy->client || (NPC->enemy->enemy->client->NPC_class!=CLASS_RANCOR&&NPC->enemy->enemy->client->NPC_class!=CLASS_WAMPA)) )//enemy's enemy is not a client or is not a wampa or rancor (which is scarier than me)
		{//they should be scared of ME and no-one else
			G_SetEnemy( NPC->enemy, NPC );
		}
		NPC_CheckGetNewWeapon();
		NPC_BSST_Attack();
	}
}
