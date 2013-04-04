#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean FlyingCreature( gentity_t *ent );

#define	SPF_NO_HIDE			2

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

static qboolean enemyLOS2;
static qboolean enemyCS2;
static qboolean faceEnemy2;
static qboolean move2;
static qboolean shoot2;
static float	enemyDist2;

//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void Sniper_ClearTimers( gentity_t *ent )
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
}

void NPC_Sniper_PlayConfusionSound( gentity_t *self )
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

void NPC_Sniper_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	self->NPC->localState = LSTATE_UNDERFIRE;

	TIMER_Set( self, "duck", -1 );
	TIMER_Set( self, "stand", 2000 );

	NPC_Pain( self, attacker, damage );

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

static void Sniper_HoldPosition( void )
{
	NPC_FreeCombatPoint( NPCInfo->combatPoint, qtrue );
	NPCInfo->goalEntity = NULL;
	
	/*if ( TIMER_Done( NPC, "stand" ) )
	{//FIXME: what if can't shoot from this pos?
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

/*
-------------------------
ST_Move
-------------------------
*/

static qboolean Sniper_Move( void )
{
	qboolean	moved;
	navInfo_t	info;

	NPCInfo->combatMove = qtrue;//always move straight toward our goal

	moved = NPC_MoveToGoal( qtrue );
	
	//Get the move info
	NAV_GetLastMove( &info );

	//FIXME: if we bump into another one of our guys and can't get around him, just stop!
	//If we hit our target, then stop and fire!
	if ( info.flags & NIF_COLLISION ) 
	{
		if ( info.blocker == NPC->enemy )
		{
			Sniper_HoldPosition();
		}
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{//couldn't get to enemy
		if ( (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) && NPCInfo->goalEntity && NPCInfo->goalEntity == NPC->enemy )
		{//we were running after enemy
			//Try to find a combat point that can hit the enemy
			int cpFlags = (CP_CLEAR|CP_HAS_ROUTE);
			int cp;
			if ( NPCInfo->scriptFlags&SCF_USE_CP_NEAREST )
			{
				cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
				cpFlags |= CP_NEAREST;
			}
			cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, NPC->r.currentOrigin, cpFlags, 32, -1 );
			if ( cp == -1 && !(NPCInfo->scriptFlags&SCF_USE_CP_NEAREST) )
			{//okay, try one by the enemy
				cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, CP_CLEAR|CP_HAS_ROUTE|CP_HORZ_DIST_COLL, 32, -1 );
			}
			//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
			if ( cp != -1 )
			{//found a combat point that has a clear shot to enemy
				NPC_SetCombatPoint( cp );
				NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
				return moved;
			}
		}
		//just hang here
		Sniper_HoldPosition();
	}

	return moved;
}

/*
-------------------------
NPC_BSSniper_Patrol
-------------------------
*/

void NPC_BSSniper_Patrol( void )
{//FIXME: pick up on bodies of dead buddies?
	NPC->count = 0;

	if ( NPCInfo->confusionTime < level.time )
	{
		//Look for any enemies
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{
			if ( NPC_CheckPlayerTeamStealth() )
			{
				//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//Should be auto now
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
				if ( alertEvent >= 0 && level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID )
				{
					NPCInfo->lastAlertID = level.alertEvents[alertEvent].ID;
					if ( level.alertEvents[alertEvent].level == AEL_DISCOVERED )
					{
						if ( level.alertEvents[alertEvent].owner && 
							level.alertEvents[alertEvent].owner->client && 
							level.alertEvents[alertEvent].owner->health >= 0 &&
							level.alertEvents[alertEvent].owner->client->playerTeam == NPC->client->enemyTeam )
						{//an enemy
							G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
							//NPCInfo->enemyLastSeenTime = level.time;
							TIMER_Set( NPC, "attackDelay", Q_irand( (6-NPCInfo->stats.aim)*100, (6-NPCInfo->stats.aim)*500 ) );
						}
					}
					else
					{//FIXME: get more suspicious over time?
						//Save the position for movement (if necessary)
						//FIXME: sound?
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
			{//FIXME: walk over to it, maybe?  Not if not chase enemies flag
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

/*
-------------------------
NPC_BSSniper_Idle
-------------------------
*/
/*
void NPC_BSSniper_Idle( void )
{
	//reset our shotcount
	NPC->count = 0;

	//FIXME: check for other alert events?

	//Is there danger nearby?
	if ( NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue ) ) )
	{
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

static void Sniper_CheckMoveState( void )
{
	//See if we're a scout
	if ( !(NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) )//NPCInfo->behaviorState == BS_STAND_AND_SHOOT )
	{
		if ( NPCInfo->goalEntity == NPC->enemy )
		{
			move2 = qfalse;
			return;
		}
	}
	//See if we're running away
	else if ( NPCInfo->squadState == SQUAD_RETREAT )
	{
		if ( TIMER_Done( NPC, "flee" ) )
		{
			NPCInfo->squadState = SQUAD_IDLE;
		}
		else
		{
			faceEnemy2 = qfalse;
		}
	}
	else if ( NPCInfo->squadState == SQUAD_IDLE )
	{
		if ( !NPCInfo->goalEntity )
		{
			move2 = qfalse;
			return;
		}
	}

	//See if we're moving towards a goal, not the enemy
	if ( ( NPCInfo->goalEntity != NPC->enemy ) && ( NPCInfo->goalEntity != NULL ) )
	{
		//Did we make it?
		if ( NAV_HitNavGoal( NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, NPCInfo->goalEntity->r.currentOrigin, 16, FlyingCreature( NPC ) ) || 
			( NPCInfo->squadState == SQUAD_SCOUT && enemyLOS2 && enemyDist2 <= 10000 ) )
		{
			int	newSquadState = SQUAD_STAND_AND_SHOOT;
			//we got where we wanted to go, set timers based on why we were running
			switch ( NPCInfo->squadState )
			{
			case SQUAD_RETREAT://was running away
				TIMER_Set( NPC, "duck", (NPC->client->pers.maxHealth - NPC->health) * 100 );
				TIMER_Set( NPC, "hideTime", Q_irand( 3000, 7000 ) );
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
			NPC_ReachedGoal();
			//don't attack right away
			TIMER_Set( NPC, "attackDelay", Q_irand( (6-NPCInfo->stats.aim)*50, (6-NPCInfo->stats.aim)*100 ) );	//FIXME: Slant for difficulty levels, too?
			//don't do something else just yet
			TIMER_Set( NPC, "roamTime", Q_irand( 1000, 4000 ) );
			//stop fleeing
			if ( NPCInfo->squadState == SQUAD_RETREAT )
			{
				TIMER_Set( NPC, "flee", -level.time );
				NPCInfo->squadState = SQUAD_IDLE;
			}
			return;
		}

		//keep going, hold of roamTimer until we get there
		TIMER_Set( NPC, "roamTime", Q_irand( 4000, 8000 ) );
	}
}

static void Sniper_ResolveBlockedShot( void )
{
	if ( TIMER_Done( NPC, "duck" ) )
	{//we're not ducking
		if ( TIMER_Done( NPC, "roamTime" ) )
		{//not roaming
			//FIXME: try to find another spot from which to hit the enemy
			if ( (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) && (!NPCInfo->goalEntity || NPCInfo->goalEntity == NPC->enemy) )
			{//we were running after enemy
				//Try to find a combat point that can hit the enemy
				int cpFlags = (CP_CLEAR|CP_HAS_ROUTE);
				int cp;

				if ( NPCInfo->scriptFlags&SCF_USE_CP_NEAREST )
				{
					cpFlags &= ~(CP_FLANK|CP_APPROACH_ENEMY|CP_CLOSEST);
					cpFlags |= CP_NEAREST;
				}
				cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, NPC->r.currentOrigin, cpFlags, 32, -1 );
				if ( cp == -1 && !(NPCInfo->scriptFlags&SCF_USE_CP_NEAREST) )
				{//okay, try one by the enemy
					cp = NPC_FindCombatPoint( NPC->r.currentOrigin, NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, CP_CLEAR|CP_HAS_ROUTE|CP_HORZ_DIST_COLL, 32, -1 );
				}
				//NOTE: there may be a perfectly valid one, just not one within CP_COLLECT_RADIUS of either me or him...
				if ( cp != -1 )
				{//found a combat point that has a clear shot to enemy
					NPC_SetCombatPoint( cp );
					NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp, NULL );
					TIMER_Set( NPC, "duck", -1 );
					TIMER_Set( NPC, "attackDelay", Q_irand( 1000, 3000 ) );
					return;
				}
			}
		}
	}
	/*
	else
	{//maybe we should stand
		if ( TIMER_Done( NPC, "stand" ) )
		{//stand for as long as we'll be here
			TIMER_Set( NPC, "stand", Q_irand( 500, 2000 ) );
			return;
		}
	}
	//Hmm, can't resolve this by telling them to duck or telling me to stand
	//We need to move!
	TIMER_Set( NPC, "roamTime", -1 );
	TIMER_Set( NPC, "stick", -1 );
	TIMER_Set( NPC, "duck", -1 );
	TIMER_Set( NPC, "attackDelay", Q_irand( 1000, 3000 ) );
	*/
}

/*
-------------------------
ST_CheckFireState
-------------------------
*/

static void Sniper_CheckFireState( void )
{
	if ( enemyCS2 )
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

	//continue to fire on their last position
	if ( !Q_irand( 0, 1 ) && NPCInfo->enemyLastSeenTime && level.time - NPCInfo->enemyLastSeenTime < ((5-NPCInfo->stats.aim)*1000) )//FIXME: incorporate skill too?
	{
		if ( !VectorCompare( vec3_origin, NPCInfo->enemyLastSeenLocation ) )
		{
			//Fire on the last known position
			vec3_t	muzzle, dir, angles;

			CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
			VectorSubtract( NPCInfo->enemyLastSeenLocation, muzzle, dir );

			VectorNormalize( dir );

			vectoangles( dir, angles );

			NPCInfo->desiredYaw		= angles[YAW];
			NPCInfo->desiredPitch	= angles[PITCH];

			shoot2 = qtrue;
			//faceEnemy2 = qfalse;
		}
		return;
	}
	else if ( level.time - NPCInfo->enemyLastSeenTime > 10000 )
	{//next time we see him, we'll miss few times first
		NPC->count = 0;
	}
}

qboolean Sniper_EvaluateShot( int hit )
{
	gentity_t *hitEnt;

	if ( !NPC->enemy )
	{
		return qfalse;
	}

	hitEnt = &g_entities[hit];
	if ( hit == NPC->enemy->s.number 
		|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
		|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPC->s.weapon == WP_EMPLACED_GUN) )
		|| ( hitEnt && (hitEnt->r.svFlags&SVF_GLASS_BRUSH)) )
	{//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

void Sniper_FaceEnemy( void )
{
	//FIXME: the ones behind kill holes are facing some arbitrary direction and not firing
	//FIXME: If actually trying to hit enemy, don't fire unless enemy is at least in front of me?
	//FIXME: need to give designers option to make them not miss first few shots
	if ( NPC->enemy )
	{
		vec3_t	muzzle, target, angles, forward, right, up;
		//Get the positions
		AngleVectors( NPC->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( NPC, forward, right, up, muzzle );
		//CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
		CalcEntitySpot( NPC->enemy, SPOT_ORIGIN, target );

		if ( enemyDist2 > 65536 && NPCInfo->stats.aim < 5 )//is 256 squared, was 16384 (128*128)
		{
			if ( NPC->count < (5-NPCInfo->stats.aim) )
			{//miss a few times first
				if ( shoot2 && TIMER_Done( NPC, "attackDelay" ) && level.time >= NPCInfo->shotTime )
				{//ready to fire again
					qboolean	aimError = qfalse;
					qboolean	hit = qtrue;
					int			tryMissCount = 0;
					trace_t		trace;

					GetAnglesForDirection( muzzle, target, angles );
					AngleVectors( angles, forward, right, up );

					while ( hit && tryMissCount < 10 )
					{
						tryMissCount++;
						if ( !Q_irand( 0, 1 ) )
						{
							aimError = qtrue;
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, NPC->enemy->r.maxs[2]*flrand(1.5, 4), right, target );
							}
							else
							{
								VectorMA( target, NPC->enemy->r.mins[2]*flrand(1.5, 4), right, target );
							}
						}
						if ( !aimError || !Q_irand( 0, 1 ) )
						{
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, NPC->enemy->r.maxs[2]*flrand(1.5, 4), up, target );
							}
							else
							{
								VectorMA( target, NPC->enemy->r.mins[2]*flrand(1.5, 4), up, target );
							}
						}
						trap_Trace( &trace, muzzle, vec3_origin, vec3_origin, target, NPC->s.number, MASK_SHOT );
						hit = Sniper_EvaluateShot( trace.entityNum );
					}
					NPC->count++;
				}
				else
				{
					if ( !enemyLOS2 )
					{
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
			}
			else
			{//based on distance, aim value, difficulty and enemy movement, miss
				//FIXME: incorporate distance as a factor?
				int missFactor = 8-(NPCInfo->stats.aim+g_spskill.integer) * 3;
				if ( missFactor > ENEMY_POS_LAG_STEPS )
				{
					missFactor = ENEMY_POS_LAG_STEPS;
				}
				else if ( missFactor < 0 )
				{//???
					missFactor = 0 ;
				}
				VectorCopy( NPCInfo->enemyLaggedPos[missFactor], target );
			}
			GetAnglesForDirection( muzzle, target, angles );
		}
		else
		{
			target[2] += flrand( 0, NPC->enemy->r.maxs[2] );
			//CalcEntitySpot( NPC->enemy, SPOT_HEAD_LEAN, target );
			GetAnglesForDirection( muzzle, target, angles );
		}

		NPCInfo->desiredYaw		= AngleNormalize360( angles[YAW] );
		NPCInfo->desiredPitch	= AngleNormalize360( angles[PITCH] );
	}
	NPC_UpdateAngles( qtrue, qtrue );
}

void Sniper_UpdateEnemyPos( void )
{
	int index;
	int i;

	for ( i = MAX_ENEMY_POS_LAG-ENEMY_POS_LAG_INTERVAL; i >= 0; i -= ENEMY_POS_LAG_INTERVAL )
	{
		index = i/ENEMY_POS_LAG_INTERVAL;
		if ( !index )
		{
			CalcEntitySpot( NPC->enemy, SPOT_HEAD_LEAN, NPCInfo->enemyLaggedPos[index] );
			NPCInfo->enemyLaggedPos[index][2] -= flrand( 2, 16 );
		}
		else
		{
			VectorCopy( NPCInfo->enemyLaggedPos[index-1], NPCInfo->enemyLaggedPos[index] );
		}
	}
}

/*
-------------------------
NPC_BSSniper_Attack
-------------------------
*/

void Sniper_StartHide( void )
{
	int duckTime = Q_irand( 2000, 5000 );
	
	TIMER_Set( NPC, "duck", duckTime );
	TIMER_Set( NPC, "watch", 500 );
	TIMER_Set( NPC, "attackDelay", duckTime + Q_irand( 500, 2000 ) );
}

void NPC_BSSniper_Attack( void )
{
	//Don't do anything if we're hurt
	if ( NPC->painDebounceTime > level.time )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	//NPC_CheckEnemy( qtrue, qfalse );
	//If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt(qfalse) == qfalse )//!NPC->enemy )//
	{
		NPC->enemy = NULL;
		NPC_BSSniper_Patrol();//FIXME: or patrol?
		return;
	}

	if ( TIMER_Done( NPC, "flee" ) && NPC_CheckForDanger( NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_DANGER ) ) )
	{//going to run
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( !NPC->enemy )
	{//WTF?  somehow we lost our enemy?
		NPC_BSSniper_Patrol();//FIXME: or patrol?
		return;
	}

	enemyLOS2 = enemyCS2 = qfalse;
	move2 = qtrue;
	faceEnemy2 = qfalse;
	shoot2 = qfalse;
	enemyDist2 = DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );
	if ( enemyDist2 < 16384 )//128 squared
	{//too close, so switch to primary fire
		if ( NPC->client->ps.weapon == WP_DISRUPTOR )
		{//sniping... should be assumed
			if ( NPCInfo->scriptFlags & SCF_ALT_FIRE )
			{//use primary fire
				trace_t	trace;
				trap_Trace ( &trace, NPC->enemy->r.currentOrigin, NPC->enemy->r.mins, NPC->enemy->r.maxs, NPC->r.currentOrigin, NPC->enemy->s.number, NPC->enemy->clipmask );
				if ( !trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == NPC->s.number ) )
				{//he can get right to me
					NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
					//reset fire-timing variables
					NPC_ChangeWeapon( WP_DISRUPTOR );
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
			//FIXME: switch back if he gets far away again?
		}
	}
	else if ( enemyDist2 > 65536 )//256 squared
	{
		if ( NPC->client->ps.weapon == WP_DISRUPTOR )
		{//sniping... should be assumed
			if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon( WP_DISRUPTOR );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	Sniper_UpdateEnemyPos();
	//can we see our target?
	if ( NPC_ClearLOS4( NPC->enemy ) )//|| (NPCInfo->stats.aim >= 5 && gi.inPVS( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin )) )
	{
		float maxShootDist;

		NPCInfo->enemyLastSeenTime = level.time;
		VectorCopy( NPC->enemy->r.currentOrigin, NPCInfo->enemyLastSeenLocation );
		enemyLOS2 = qtrue;
		maxShootDist = NPC_MaxDistSquaredForWeapon();
		if ( enemyDist2 < maxShootDist )
		{
			vec3_t fwd, right, up, muzzle, end;
			trace_t	tr;
			int hit;

			AngleVectors( NPC->client->ps.viewangles, fwd, right, up );
			CalcMuzzlePoint( NPC, fwd, right, up, muzzle );
			VectorMA( muzzle, 8192, fwd, end );
			trap_Trace ( &tr, muzzle, NULL, NULL, end, NPC->s.number, MASK_SHOT );

			hit = tr.entityNum;
			//can we shoot our target?
			if ( Sniper_EvaluateShot( hit ) )
			{
				enemyCS2 = qtrue;
			}
		}
	}
	/*
	else if ( gi.inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin ) )
	{
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy2 = qtrue;
	}
	*/

	if ( enemyLOS2 )
	{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
		faceEnemy2 = qtrue;
	}
	if ( enemyCS2 )
	{
		shoot2 = qtrue;
	}
	else if ( level.time - NPCInfo->enemyLastSeenTime > 3000 )
	{//Hmm, have to get around this bastard... FIXME: this NPCInfo->enemyLastSeenTime builds up when ducked seems to make them want to run when they uncrouch
		Sniper_ResolveBlockedShot();
	}

	//Check for movement to take care of
	Sniper_CheckMoveState();

	//See if we should override shooting decision with any special considerations
	Sniper_CheckFireState();

	if ( move2 )
	{//move toward goal
		if ( NPCInfo->goalEntity )//&& ( NPCInfo->goalEntity != NPC->enemy || enemyDist2 > 10000 ) )//100 squared
		{
			move2 = Sniper_Move();
		}
		else
		{
			move2 = qfalse;
		}
	}

	if ( !move2 )
	{
		if ( !TIMER_Done( NPC, "duck" ) )
		{
			if ( TIMER_Done( NPC, "watch" ) )
			{//not while watching
				ucmd.upmove = -127;
			}
		}
		//FIXME: what about leaning?
		//FIXME: also, when stop ducking, start looking, if enemy can see me, chance of ducking back down again
	}
	else
	{//stop ducking!
		TIMER_Set( NPC, "duck", -1 );
	}

	if ( TIMER_Done( NPC, "duck" ) 
		&& TIMER_Done( NPC, "watch" ) 
		&& (TIMER_Get( NPC, "attackDelay" )-level.time) > 1000 
		&& NPC->attackDebounceTime < level.time )
	{
		if ( enemyLOS2 && (NPCInfo->scriptFlags&SCF_ALT_FIRE) )
		{
			if ( NPC->fly_sound_debounce_time < level.time )
			{
				NPC->fly_sound_debounce_time = level.time + 2000;
			}
		}
	}

	if ( !faceEnemy2 )
	{//we want to face in the dir we're running
		if ( move2 )
		{//don't run away and shoot
			NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
			NPCInfo->desiredPitch = 0;
			shoot2 = qfalse;
		}
		NPC_UpdateAngles( qtrue, qtrue );
	}
	else// if ( faceEnemy2 )
	{//face the enemy
		Sniper_FaceEnemy();
	}

	if ( NPCInfo->scriptFlags&SCF_DONT_FIRE )
	{
		shoot2 = qfalse;
	}

	//FIXME: don't shoot right away!
	if ( shoot2 )
	{//try to shoot if it's time
		if ( TIMER_Done( NPC, "attackDelay" ) )
		{
			WeaponThink( qtrue );
			if ( ucmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK) )
			{
				G_SoundOnEnt( NPC, CHAN_WEAPON, "sound/null.wav" );
			}

			//took a shot, now hide
			if ( !(NPC->spawnflags&SPF_NO_HIDE) && !Q_irand( 0, 1 ) )
			{
				//FIXME: do this if in combat point and combat point has duck-type cover... also handle lean-type cover
				Sniper_StartHide();
			}
			else
			{
				TIMER_Set( NPC, "attackDelay", NPCInfo->shotTime-level.time );
			}
		}
	}
}

void NPC_BSSniper_Default( void )
{
	if( !NPC->enemy )
	{//don't have an enemy, look for one
		NPC_BSSniper_Patrol();
	}
	else//if ( NPC->enemy )
	{//have an enemy
		NPC_BSSniper_Attack();
	}
}
