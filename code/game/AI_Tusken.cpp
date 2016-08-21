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
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern void NPC_AimAdjust( int change );
extern qboolean FlyingCreature( gentity_t *ent );
extern int PM_AnimLength( int index, animNumber_t anim );


#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1

#define	DISTANCE_SCALE		0.25f
#define	DISTANCE_THRESHOLD	0.075f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75 )

qboolean NPC_CheckPlayerTeamStealth( void );

static float	enemyDist;

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

/*
-------------------------
NPC_Tusken_Precache
-------------------------
*/
void NPC_Tusken_Precache( void )
{
	int i;
	for ( i = 1; i < 5; i ++ )
	{
		G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", i ) );
	}
}

void Tusken_ClearTimers( gentity_t *ent )
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
	TIMER_Set( ent, "taunting", 0 );
}

void NPC_Tusken_PlayConfusionSound( gentity_t *self )
{//FIXME: make this a custom sound in sound set
	if ( self->health > 0 )
	{
		G_AddVoiceEvent( self, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 2000 );
	}
	//reset him to be totally unaware again
	TIMER_Set( self, "enemyLastVisible", 0 );
	TIMER_Set( self, "flee", 0 );
	self->NPC->squadState = SQUAD_IDLE;
	self->NPC->tempBehavior = BS_DEFAULT;

	//self->NPC->behaviorState = BS_PATROL;
	G_ClearEnemy( self );//FIXME: or just self->enemy = NULL;?

	self->NPC->investigateCount = 0;
}


/*
-------------------------
NPC_ST_Pain
-------------------------
*/

void NPC_Tusken_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, vec3_t point, int damage, int mod )
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stand", 2000 );

	NPC_Pain( self, inflictor, other, point, damage, mod );

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

static void Tusken_HoldPosition( void )
{
	NPC_FreeCombatPoint( NPCInfo->combatPoint, qtrue );
	NPCInfo->goalEntity = NULL;
}

/*
-------------------------
ST_Move
-------------------------
*/

static qboolean Tusken_Move( void )
{
	NPCInfo->combatMove = qtrue;//always move straight toward our goal

	qboolean	moved = NPC_MoveToGoal( qtrue );

	//If our move failed, then reset
	if ( moved == qfalse )
	{//couldn't get to enemy
		//just hang here
		Tusken_HoldPosition();
	}

	return moved;
}

/*
-------------------------
NPC_BSTusken_Patrol
-------------------------
*/

void NPC_BSTusken_Patrol( void )
{//FIXME: pick up on bodies of dead buddies?
	if ( NPCInfo->confusionTime < level.time )
	{
		//Look for any enemies
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth() )
			{
				//NPC_AngerSound();
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}

		if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
		{
			//Is there danger nearby
			int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_SUSPICIOUS );
			if ( NPC_CheckForDanger( alertEvent ) )
			{
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
			else
			{//check for other alert events
				//There is an event to look at
				if ( alertEvent >= 0 )//&& level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID )
				{
					//NPCInfo->lastAlertID = level.alertEvents[alertEvent].ID;
					if ( level.alertEvents[alertEvent].level == AEL_DISCOVERED )
					{
						if ( level.alertEvents[alertEvent].owner &&
							level.alertEvents[alertEvent].owner->client &&
							level.alertEvents[alertEvent].owner->health >= 0 &&
							level.alertEvents[alertEvent].owner->client->playerTeam == NPC->client->enemyTeam )
						{//an enemy
							G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
							//NPCInfo->enemyLastSeenTime = level.time;
							TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
						}
					}
					else
					{//FIXME: get more suspicious over time?
						//Save the position for movement (if necessary)
						VectorCopy( level.alertEvents[alertEvent].position, NPCInfo->investigateGoal );
						NPCInfo->investigateDebounceTime = level.time + Q_irand( 500, 1000 );
						if ( level.alertEvents[alertEvent].level == AEL_SUSPICIOUS )
						{//suspicious looks longer
							NPCInfo->investigateDebounceTime += Q_irand( 500, 2500 );
						}
					}
				}
			}

			if ( NPCInfo->investigateDebounceTime > level.time )
			{//FIXME: walk over to it, maybe?  Not if not chase enemies
				//NOTE: stops walking or doing anything else below
				vec3_t	dir, angles;
				float	o_yaw, o_pitch;

				VectorSubtract( NPCInfo->investigateGoal, NPC->client->renderInfo.eyePoint, dir );
				vectoangles( dir, angles );

				o_yaw = NPCInfo->desiredYaw;
				o_pitch = NPCInfo->desiredPitch;
				NPCInfo->desiredYaw = angles[YAW];
				NPCInfo->desiredPitch = angles[PITCH];

				NPC_UpdateAngles( qtrue, qtrue );

				NPCInfo->desiredYaw = o_yaw;
				NPCInfo->desiredPitch = o_pitch;
				return;
			}
		}
	}

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}


void NPC_Tusken_Taunt( void )
{
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	TIMER_Set( NPC, "taunting", NPC->client->ps.torsoAnimTimer );
	TIMER_Set( NPC, "duck", -1 );
}

/*
-------------------------
NPC_BSTusken_Attack
-------------------------
*/

void NPC_BSTusken_Attack( void )
{
// IN PAIN
//---------
	if ( NPC->painDebounceTime > level.time )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

// IN FLEE
//---------
	if ( TIMER_Done( NPC, "flee" ) && NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}



// UPDATE OUR ENEMY
//------------------
	if (NPC_CheckEnemyExt()==qfalse || !NPC->enemy)
	{
		NPC_BSTusken_Patrol();
		return;
	}
	enemyDist	= Distance(NPC->enemy->currentOrigin, NPC->currentOrigin);

	// Is The Current Enemy A Jawa?
	//------------------------------
	if (NPC->enemy->client && NPC->enemy->client->NPC_class==CLASS_JAWA)
	{
		// Make Sure His Enemy Is Me
		//---------------------------
		if (NPC->enemy->enemy!=NPC)
		{
			G_SetEnemy(NPC->enemy, NPC);
		}

		// Should We Forget About Our Current Enemy And Go After The Player?
		//-------------------------------------------------------------------
		if ((player) &&															// If There Is A Player Pointer
			(player!=NPC->enemy) &&												// The Player Is Not Currently My Enemy
			(Distance(player->currentOrigin, NPC->currentOrigin)<130.0f) &&		// The Player Is Close Enough
			(NAV::InSameRegion(NPC, player))									// And In The Same Region
			)
		{
			G_SetEnemy(NPC, player);
		}
	}

	// Update Our Last Seen Time
	//---------------------------
	if (NPC_ClearLOS(NPC->enemy))
	{
		NPCInfo->enemyLastSeenTime = level.time;
	}



	// Check To See If We Are In Attack Range
	//----------------------------------------
 	float	boundsMin	= (NPC->maxs[0]+NPC->enemy->maxs[0]);
 	float	lungeRange	= (boundsMin + 65.0f);
	float	strikeRange = (boundsMin + 40.0f);
	bool	meleeRange	= (enemyDist<lungeRange);
	bool	meleeWeapon	= (NPC->client->ps.weapon!=WP_TUSKEN_RIFLE);
	bool	canSeeEnemy = ((level.time - NPCInfo->enemyLastSeenTime)<3000);

	// Check To Start Taunting
	//-------------------------
 	if (canSeeEnemy && !meleeRange && TIMER_Done(NPC, "tuskenTauntCheck"))
	{
		TIMER_Set(NPC, "tuskenTauntCheck", Q_irand(2000, 6000));
		if (!Q_irand(0,3))
		{
			NPC_Tusken_Taunt();
		}
	}


	if (TIMER_Done(NPC, "taunting"))
	{
		// Should I Attack?
		//------------------
		if (meleeRange || (!meleeWeapon && canSeeEnemy))
		{
			if (!(NPCInfo->scriptFlags&SCF_FIRE_WEAPON) && 	// If This Flag Is On, It Calls Attack From Elsewhere
				!(NPCInfo->scriptFlags&SCF_DONT_FIRE) &&	// If This Flag Is On, Don't Fire At All
				 (TIMER_Done(NPC, "attackDelay"))
				)
			{
				ucmd.buttons &= ~BUTTON_ALT_ATTACK;

				// If Not In Strike Range, Do Lunge, Or If We Don't Have The Staff, Just Shoot Normally
				//--------------------------------------------------------------------------------------
				if (enemyDist > strikeRange)
				{
					ucmd.buttons |= BUTTON_ALT_ATTACK;
				}

				WeaponThink( qtrue );
				TIMER_Set(NPC, "attackDelay", NPCInfo->shotTime-level.time);
			}

			if ( !TIMER_Done( NPC, "duck" ) )
			{
				ucmd.upmove = -127;
			}
		}

		// Or Should I Move?
		//-------------------
		else if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)
		{
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = lungeRange;
			Tusken_Move();
		}
	}


// UPDATE ANGLES
//---------------
	if (canSeeEnemy)
	{
		NPC_FaceEnemy(qtrue);
	}
	NPC_UpdateAngles(qtrue, qtrue);
}

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
void Tusken_StaffTrace( void )
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
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
			VectorMA( base, -20, dir, base );
			VectorMA( base, 78, dir, tip );
	#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				G_DebugLine(base, tip, 1000, 0x000000ff, qtrue);
			}
	#endif
			gi.trace( &trace, base, mins, maxs, tip, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
			if ( trace.fraction < 1.0f && trace.entityNum != lastHit )
			{//hit something
				gentity_t *traceEnt = &g_entities[trace.entityNum];
				if ( traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->NPC_class) )
				{//smack
					int dmg = Q_irand( 5, 10 ) * (g_spskill->integer+1);

					//FIXME: debounce?
					G_Sound( traceEnt, G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", Q_irand( 1, 4 ) ) ) );
					G_Damage( traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
					if ( traceEnt->health > 0
						&& ( (traceEnt->client&&traceEnt->client->NPC_class==CLASS_JAWA&&!Q_irand(0,1))
							|| dmg > 19 ) )//FIXME: base on skill!
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

qboolean G_TuskenAttackAnimDamage( gentity_t *self )
{
 	if (self->client->ps.torsoAnim==BOTH_TUSKENATTACK1 ||
		self->client->ps.torsoAnim==BOTH_TUSKENATTACK2 ||
		self->client->ps.torsoAnim==BOTH_TUSKENATTACK3 ||
		self->client->ps.torsoAnim==BOTH_TUSKENLUNGE1)
	{
		float		current	  = 0.0f;
		int			end		  = 0;
		int			start	  = 0;
		if (!!gi.G2API_GetBoneAnimIndex(&
					self->ghoul2[self->playerModel],
					self->lowerLumbarBone,
					level.time,
					&current,
					&start,
					&end,
					NULL,
					NULL,
					NULL))
		{
 			float percentComplete = (current-start)/(end-start);
			//gi.Printf("%f\n", percentComplete);
			switch (self->client->ps.torsoAnim)
			{
			case BOTH_TUSKENATTACK1: return (qboolean)(percentComplete>0.3 && percentComplete<0.7);
			case BOTH_TUSKENATTACK2: return (qboolean)(percentComplete>0.3 && percentComplete<0.7);
			case BOTH_TUSKENATTACK3: return (qboolean)(percentComplete>0.1 && percentComplete<0.5);
			case BOTH_TUSKENLUNGE1:  return (qboolean)(percentComplete>0.3 && percentComplete<0.5);
			}
		}
	}
	return qfalse;
}

void NPC_BSTusken_Default( void )
{
	if( NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}

	if ( G_TuskenAttackAnimDamage( NPC ) )
	{
		Tusken_StaffTrace();
	}

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		NPC_BSTusken_Patrol();
	}
	else//if ( NPC->enemy )
	{//have an enemy
		NPC_BSTusken_Attack();
	}
}
