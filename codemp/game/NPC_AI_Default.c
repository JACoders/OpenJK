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
#include "icarus/Q3_Interface.h"

extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);

/*
void NPC_LostEnemyDecideChase(void)

  We lost our enemy and want to drop him but see if we should chase him if we are in the proper bState
*/

void NPC_LostEnemyDecideChase(void)
{
	switch( NPCS.NPCInfo->behaviorState )
	{
	case BS_HUNT_AND_KILL:
		//We were chasing him and lost him, so try to find him
		if ( NPCS.NPC->enemy == NPCS.NPCInfo->goalEntity && NPCS.NPC->enemy->lastWaypoint != WAYPOINT_NONE )
		{//Remember his last valid Wp, then check it out
			//FIXME: Should we only do this if there's no other enemies or we've got LOCKED_ENEMY on?
			NPC_BSSearchStart( NPCS.NPC->enemy->lastWaypoint, BS_SEARCH );
		}
		//If he's not our goalEntity, we're running somewhere else, so lose him
		break;
	default:
		break;
	}
	G_ClearEnemy( NPCS.NPC );
}
/*
-------------------------
NPC_StandIdle
-------------------------
*/

void NPC_StandIdle( void )
{
/*
	//Must be done with any other animations
	if ( NPC->client->ps.legsAnimTimer != 0 )
		return;

	//Not ready to do another one
	if ( TIMER_Done( NPC, "idleAnim" ) == false )
		return;

	int anim = NPC->client->ps.legsAnim;

	if ( anim != BOTH_STAND1 && anim != BOTH_STAND2 )
		return;

	//FIXME: Account for STAND1 or STAND2 here and set the base anim accordingly
	int	baseSeq = ( anim == BOTH_STAND1 ) ? BOTH_STAND1_RANDOM1 : BOTH_STAND2_RANDOM1;

	//Must have at least one random idle animation
	//NOTENOTE: This relies on proper ordering of animations, which SHOULD be okay
	if ( PM_HasAnimation( NPC, baseSeq ) == false )
		return;

	int	newIdle = Q_irand( 0, MAX_IDLE_ANIMS-1 );

	//FIXME: Technically this could never complete.. but that's not really too likely
	while( 1 )
	{
		if ( PM_HasAnimation( NPC, baseSeq + newIdle ) )
			break;

		newIdle = Q_irand( 0, MAX_IDLE_ANIMS );
	}

	//Start that animation going
	NPC_SetAnim( NPC, SETANIM_BOTH, baseSeq + newIdle, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	int newTime = PM_AnimLength( NPC->client->clientInfo.animFileIndex, (animNumber_t) (baseSeq + newIdle) );

	//Don't do this again for a random amount of time
	TIMER_Set( NPC, "idleAnim", newTime + Q_irand( 2000, 10000 ) );
*/
}

qboolean NPC_StandTrackAndShoot (gentity_t *NPC, qboolean canDuck)
{
	qboolean	attack_ok = qfalse;
	qboolean	duck_ok = qfalse;
	qboolean	faced = qfalse;
	float		attack_scale = 1.0;

	//First see if we're hurt bad- if so, duck
	//FIXME: if even when ducked, we can shoot someone, we should.
	//Maybe is can be shot even when ducked, we should run away to the nearest cover?
	if ( canDuck )
	{
		if ( NPC->health < 20 )
		{
		//	if( NPC->svFlags&SVF_HEALING || Q_flrand(0.0f, 1.0f) )
			if( Q_flrand(0.0f, 1.0f) )
			{
				duck_ok = qtrue;
			}
		}
		else if ( NPC->health < 40 )
		{
//			if ( NPC->svFlags&SVF_HEALING )
//			{//Medic is on the way, get down!
//				duck_ok = qtrue;
//			}
		}
	}

	//NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( !duck_ok )
	{//made this whole part a function call
		attack_ok = NPC_CheckCanAttack( attack_scale, qtrue );
		faced = qtrue;
	}

	if ( canDuck && (duck_ok || (!attack_ok && NPCS.client->ps.weaponTime <= 0)) && NPCS.ucmd.upmove != -127 )
	{//if we didn't attack check to duck if we're not already
		if( !duck_ok )
		{
			if ( NPC->enemy->client )
			{
				if ( NPC->enemy->enemy == NPC )
				{
					if ( NPC->enemy->client->buttons & BUTTON_ATTACK )
					{//FIXME: determine if enemy fire angles would hit me or get close
						if ( NPC_CheckDefend( 1.0 ) )//FIXME: Check self-preservation?  Health?
						{
							duck_ok = qtrue;
						}
					}
				}
			}
		}

		if ( duck_ok )
		{//duck and don't shoot
			NPCS.ucmd.upmove = -127;
			NPCS.NPCInfo->duckDebounceTime = level.time + 1000;//duck for a full second
		}
	}

	return faced;
}


void NPC_BSIdle( void )
{
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() )
	{
		NPC_MoveToGoal( qtrue );
	}

	if ( ( NPCS.ucmd.forwardmove == 0 ) && ( NPCS.ucmd.rightmove == 0 ) && ( NPCS.ucmd.upmove == 0 ) )
	{
//		NPC_StandIdle();
	}

	NPC_UpdateAngles( qtrue, qtrue );
	NPCS.ucmd.buttons |= BUTTON_WALKING;
}

void NPC_BSRun (void)
{
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() )
	{
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSStandGuard (void)
{
	//FIXME: Use Snapshot info
	if ( NPCS.NPC->enemy == NULL )
	{//Possible to pick one up by being shot
		if( Q_flrand(0.0f, 1.0f) < 0.5 )
		{
			if(NPCS.NPC->client->enemyTeam)
			{
				gentity_t *newenemy = NPC_PickEnemy(NPCS.NPC, NPCS.NPC->client->enemyTeam, (NPCS.NPC->cantHitEnemyCounter < 10), (NPCS.NPC->client->enemyTeam == NPCTEAM_PLAYER), qtrue);
				//only checks for vis if couldn't hit last enemy
				if(newenemy)
				{
					G_SetEnemy( NPCS.NPC, newenemy );
				}
			}
		}
	}

	if ( NPCS.NPC->enemy != NULL )
	{
		if( NPCS.NPCInfo->tempBehavior == BS_STAND_GUARD )
		{
			NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
		}

		if( NPCS.NPCInfo->behaviorState == BS_STAND_GUARD )
		{
			NPCS.NPCInfo->behaviorState = BS_STAND_AND_SHOOT;
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSHuntAndKill
-------------------------
*/

void NPC_BSHuntAndKill( void )
{
	qboolean	turned = qfalse;
	vec3_t		vec;
	float		enemyDist;
	visibility_t	oEVis;
	int			curAnim;

	NPC_CheckEnemy( NPCS.NPCInfo->tempBehavior != BS_HUNT_AND_KILL, qfalse, qtrue );//don't find new enemy if this is tempbehav

	if ( NPCS.NPC->enemy )
	{
		oEVis = NPCS.enemyVisibility = NPC_CheckVisibility ( NPCS.NPC->enemy, CHECK_FOV|CHECK_SHOOT );//CHECK_360|//CHECK_PVS|
		if(NPCS.enemyVisibility > VIS_PVS)
		{
			if ( !NPC_EnemyTooFar( NPCS.NPC->enemy, 0, qtrue ) )
			{//Enemy is close enough to shoot - FIXME: this next func does this also, but need to know here for info on whether ot not to turn later
				NPC_CheckCanAttack( 1.0, qfalse );
				turned = qtrue;
			}
		}

		curAnim = NPCS.NPC->client->ps.legsAnim;
		if(curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 )
		{//Don't move toward enemy if we're in a full-body attack anim
			//FIXME, use IdealDistance to determin if we need to close distance
			VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, vec);
			enemyDist = VectorLength(vec);
			if( enemyDist > 48 && ((enemyDist*1.5)*(enemyDist*1.5) >= NPC_MaxDistSquaredForWeapon() ||
				oEVis != VIS_SHOOT ||
				//!(ucmd.buttons & BUTTON_ATTACK) ||
				enemyDist > IdealDistance(NPCS.NPC)*3 ) )
			{//We should close in?
				NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;

				NPC_MoveToGoal( qtrue );
			}
			else if(enemyDist < IdealDistance(NPCS.NPC))
			{//We should back off?
				//if(ucmd.buttons & BUTTON_ATTACK)
				{
					NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
					NPCS.NPCInfo->goalRadius = 12;
					NPC_MoveToGoal( qtrue );

					NPCS.ucmd.forwardmove *= -1;
					NPCS.ucmd.rightmove *= -1;
					VectorScale( NPCS.NPC->client->ps.moveDir, -1, NPCS.NPC->client->ps.moveDir );

					NPCS.ucmd.buttons |= BUTTON_WALKING;
				}
			}//otherwise, stay where we are
		}
	}
	else
	{//ok, stand guard until we find an enemy
		if( NPCS.NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{
			NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else
		{
			NPCS.NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
		}
		return;
	}

	if(!turned)
	{
		NPC_UpdateAngles(qtrue, qtrue);
	}
}

void NPC_BSStandAndShoot (void)
{
	//FIXME:
	//When our numbers outnumber enemies 3 to 1, or only one of them,
	//go into hunt and kill mode

	//FIXME:
	//When they're all dead, go to some script or wander off to sickbay?

	if(NPCS.NPC->client->playerTeam && NPCS.NPC->client->enemyTeam)
	{
		//FIXME: don't realize this right away- or else enemies show up and we're standing around
		/*
		if( teamNumbers[NPC->enemyTeam] == 0 )
		{//ok, stand guard until we find another enemy
			//reset our rush counter
			teamCounter[NPC->playerTeam] = 0;
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
			return;
		}*/
		/*
		//FIXME: whether to do this or not should be settable
		else if( NPC->playerTeam != TEAM_BORG )//Borg don't rush
		{
		//FIXME: In case reinforcements show up, we should wait a few seconds
		//and keep checking before rushing!
		//Also: what if not everyone on our team is going after playerTeam?
		//Also: our team count includes medics!
			if(NPC->health > 25)
			{//Can we rush the enemy?
				if(teamNumbers[NPC->enemyTeam] == 1 ||
					teamNumbers[NPC->playerTeam] >= teamNumbers[NPC->enemyTeam]*3)
				{//Only one of them or we outnumber 3 to 1
					if(teamStrength[NPC->playerTeam] >= 75 ||
						(teamStrength[NPC->playerTeam] >= 50 && teamStrength[NPC->playerTeam] > teamStrength[NPC->enemyTeam]))
					{//Our team is strong enough to rush
						teamCounter[NPC->playerTeam]++;
						if(teamNumbers[NPC->playerTeam] * 17 <= teamCounter[NPC->playerTeam])
						{//ok, we waited 1.7 think cycles on average and everyone is go, let's do it!
							//FIXME: Should we do this to everyone on our team?
							NPCInfo->behaviorState = BS_HUNT_AND_KILL;
							//FIXME: if the tide changes, we should retreat!
							//FIXME: when do we reset the counter?
							NPC_BSHuntAndKill ();
							return;
						}
					}
					else//Oops!  Something's wrong, reset the counter to rush
						teamCounter[NPC->playerTeam] = 0;
				}
				else//Oops!  Something's wrong, reset the counter to rush
					teamCounter[NPC->playerTeam] = 0;
			}
		}
		*/
	}

	NPC_CheckEnemy(qtrue, qfalse, qtrue);

	if(NPCS.NPCInfo->duckDebounceTime > level.time && NPCS.NPC->client->ps.weapon != WP_SABER )
	{
		NPCS.ucmd.upmove = -127;
		if(NPCS.NPC->enemy)
		{
			NPC_CheckCanAttack(1.0, qtrue);
		}
		return;
	}

	if(NPCS.NPC->enemy)
	{
		if(!NPC_StandTrackAndShoot( NPCS.NPC, qtrue ))
		{//That func didn't update our angles
			NPCS.NPCInfo->desiredYaw = NPCS.NPC->client->ps.viewangles[YAW];
			NPCS.NPCInfo->desiredPitch = NPCS.NPC->client->ps.viewangles[PITCH];
			NPC_UpdateAngles(qtrue, qtrue);
		}
	}
	else
	{
		NPCS.NPCInfo->desiredYaw = NPCS.NPC->client->ps.viewangles[YAW];
		NPCS.NPCInfo->desiredPitch = NPCS.NPC->client->ps.viewangles[PITCH];
		NPC_UpdateAngles(qtrue, qtrue);
//		NPC_BSIdle();//only moves if we have a goal
	}
}

void NPC_BSRunAndShoot (void)
{
	/*if(NPC->playerTeam && NPC->enemyTeam)
	{
		//FIXME: don't realize this right away- or else enemies show up and we're standing around
		if( teamNumbers[NPC->enemyTeam] == 0 )
		{//ok, stand guard until we find another enemy
			//reset our rush counter
			teamCounter[NPC->playerTeam] = 0;
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
			return;
		}
	}*/

	//NOTE: are we sure we want ALL run and shoot people to move this way?
	//Shouldn't it check to see if we have an enemy and our enemy is our goal?!
	//Moved that check into NPC_MoveToGoal
	//NPCInfo->combatMove = qtrue;

	NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( NPCS.NPCInfo->duckDebounceTime > level.time ) // && NPCInfo->hidingGoal )
	{
		NPCS.ucmd.upmove = -127;
		if ( NPCS.NPC->enemy )
		{
			NPC_CheckCanAttack( 1.0, qfalse );
		}
		return;
	}

	if ( NPCS.NPC->enemy )
	{
		int monitor = NPCS.NPC->cantHitEnemyCounter;
		NPC_StandTrackAndShoot( NPCS.NPC, qfalse );//(NPCInfo->hidingGoal != NULL) );

		if ( !(NPCS.ucmd.buttons & BUTTON_ATTACK) && NPCS.ucmd.upmove >= 0 && NPCS.NPC->cantHitEnemyCounter > monitor )
		{//not crouching and not firing
			vec3_t	vec;

			VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, vec );
			vec[2] = 0;
			if ( VectorLength( vec ) > 128 || NPCS.NPC->cantHitEnemyCounter >= 10 )
			{//run at enemy if too far away
				//The cantHitEnemyCounter getting high has other repercussions
				//100 (10 seconds) will make you try to pick a new enemy...
				//But we're chasing, so we clamp it at 50 here
				if ( NPCS.NPC->cantHitEnemyCounter > 60 )
				{
					NPCS.NPC->cantHitEnemyCounter = 60;
				}

				if ( NPCS.NPC->cantHitEnemyCounter >= (NPCS.NPCInfo->stats.aggression+1) * 10 )
				{
					NPC_LostEnemyDecideChase();
				}

				//chase and face
				NPCS.ucmd.angles[YAW] = 0;
				NPCS.ucmd.angles[PITCH] = 0;
				NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
				NPCS.NPCInfo->goalRadius = 12;
				//NAV_ClearLastRoute(NPC);
				NPC_MoveToGoal( qtrue );
				NPC_UpdateAngles(qtrue, qtrue);
			}
			else
			{
				//FIXME: this could happen if they're just on the other side
				//of a thin wall or something else blocking out shot.  That
				//would make us just stand there and not go around it...
				//but maybe it's okay- might look like we're waiting for
				//him to come out...?
				//Current solution: runs around if cantHitEnemyCounter gets
				//to 10 (1 second).
			}
		}
		else
		{//Clear the can't hit enemy counter here
			NPCS.NPC->cantHitEnemyCounter = 0;
		}
	}
	else
	{
		if ( NPCS.NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
			return;
		}

//		NPC_BSRun();//only moves if we have a goal
	}
}

//Simply turn until facing desired angles
void NPC_BSFace (void)
{
	//FIXME: once you stop sending turning info, they reset to whatever their delta_angles was last????
	//Once this is over, it snaps back to what it was facing before- WHY???
	if( NPC_UpdateAngles ( qtrue, qtrue ) )
	{
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPCS.NPC, TID_BSTATE );

		NPCS.NPCInfo->desiredYaw = NPCS.client->ps.viewangles[YAW];
		NPCS.NPCInfo->desiredPitch = NPCS.client->ps.viewangles[PITCH];

		NPCS.NPCInfo->aimTime = 0;//ok to turn normally now
	}
}

void NPC_BSPointShoot (qboolean shoot)
{//FIXME: doesn't check for clear shot...
	vec3_t	muzzle, dir, angles, org;

	if ( !NPCS.NPC->enemy || !NPCS.NPC->enemy->inuse || (NPCS.NPC->enemy->NPC && NPCS.NPC->enemy->health <= 0) )
	{//FIXME: should still keep shooting for a second or two after they actually die...
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPCS.NPC, TID_BSTATE );
		goto finished;
	}

	CalcEntitySpot(NPCS.NPC, SPOT_WEAPON, muzzle);
	CalcEntitySpot(NPCS.NPC->enemy, SPOT_HEAD, org);//Was spot_org
	//Head is a little high, so let's aim for the chest:
	if ( NPCS.NPC->enemy->client )
	{
		org[2] -= 12;//NOTE: is this enough?
	}

	VectorSubtract(org, muzzle, dir);
	vectoangles(dir, angles);

	switch( NPCS.NPC->client->ps.weapon )
	{
	case WP_NONE:
	case WP_STUN_BATON:
	case WP_SABER:
		//don't do any pitch change if not holding a firing weapon
		break;
	default:
		NPCS.NPCInfo->desiredPitch = NPCS.NPCInfo->lockedDesiredPitch = AngleNormalize360(angles[PITCH]);
		break;
	}

	NPCS.NPCInfo->desiredYaw = NPCS.NPCInfo->lockedDesiredYaw = AngleNormalize360(angles[YAW]);

	if ( NPC_UpdateAngles ( qtrue, qtrue ) )
	{//FIXME: if angles clamped, this may never work!
		//NPCInfo->shotTime = NPC->attackDebounceTime = 0;

		if ( shoot )
		{//FIXME: needs to hold this down if using a weapon that requires it, like phaser...
			NPCS.ucmd.buttons |= BUTTON_ATTACK;
		}

		//if ( !shoot || !(NPC->svFlags & SVF_LOCKEDENEMY) )
		if (1)
		{//If locked_enemy is on, dont complete until it is destroyed...
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPCS.NPC, TID_BSTATE );
			goto finished;
		}
	}
	//else if ( shoot && (NPC->svFlags & SVF_LOCKEDENEMY) )
	if (0)
	{//shooting them till their dead, not aiming right at them yet...
		/*
		qboolean movingTarget = qfalse;

		if ( NPC->enemy->client )
		{
			if ( VectorLengthSquared( NPC->enemy->client->ps.velocity ) )
			{
				movingTarget = qtrue;
			}
		}
		else if ( VectorLengthSquared( NPC->enemy->s.pos.trDelta ) )
		{
			movingTarget = qtrue;
		}

		if (movingTarget )
		*/
		{
			float	dist = VectorLength( dir );
			float	yawMiss, yawMissAllow = NPCS.NPC->enemy->r.maxs[0];
			float	pitchMiss, pitchMissAllow = (NPCS.NPC->enemy->r.maxs[2] - NPCS.NPC->enemy->r.mins[2])/2;

			if ( yawMissAllow < 8.0f )
			{
				yawMissAllow = 8.0f;
			}

			if ( pitchMissAllow < 8.0f )
			{
				pitchMissAllow = 8.0f;
			}

			yawMiss = tan(DEG2RAD(AngleDelta ( NPCS.NPC->client->ps.viewangles[YAW], NPCS.NPCInfo->desiredYaw ))) * dist;
			pitchMiss = tan(DEG2RAD(AngleDelta ( NPCS.NPC->client->ps.viewangles[PITCH], NPCS.NPCInfo->desiredPitch))) * dist;

			if ( yawMissAllow >= yawMiss && pitchMissAllow > pitchMiss )
			{
				NPCS.ucmd.buttons |= BUTTON_ATTACK;
			}
		}
	}

	return;

finished:
	NPCS.NPCInfo->desiredYaw = NPCS.client->ps.viewangles[YAW];
	NPCS.NPCInfo->desiredPitch = NPCS.client->ps.viewangles[PITCH];

	NPCS.NPCInfo->aimTime = 0;//ok to turn normally now
}

/*
void NPC_BSMove(void)
Move in a direction, face another
*/
void NPC_BSMove(void)
{
	gentity_t	*goal = NULL;

	NPC_CheckEnemy(qtrue, qfalse, qtrue);
	if(NPCS.NPC->enemy)
	{
		NPC_CheckCanAttack(1.0, qfalse);
	}
	else
	{
		NPC_UpdateAngles(qtrue, qtrue);
	}

	goal = UpdateGoal();
	if(goal)
	{
//		NPCInfo->moveToGoalMod = 1.0;

		NPC_SlideMoveToGoal();
	}
}

/*
void NPC_BSShoot(void)
Move in a direction, face another
*/

void NPC_BSShoot(void)
{
//	NPC_BSMove();

	NPCS.enemyVisibility = VIS_SHOOT;

	if ( NPCS.client->ps.weaponstate != WEAPON_READY && NPCS.client->ps.weaponstate != WEAPON_FIRING )
	{
		NPCS.client->ps.weaponstate = WEAPON_READY;
	}

	WeaponThink(qtrue);
}

/*
void NPC_BSPatrol( void )

  Same as idle, but you look for enemies every "vigilance"
  using your angles, HFOV, VFOV and visrange, and listen for sounds within earshot...
*/
void NPC_BSPatrol( void )
{
	//int	alertEventNum;

	if(level.time > NPCS.NPCInfo->enemyCheckDebounceTime)
	{
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + (NPCS.NPCInfo->stats.vigilance * 1000);
		NPC_CheckEnemy(qtrue, qfalse, qtrue);
		if(NPCS.NPC->enemy)
		{//FIXME: do anger script
			NPCS.NPCInfo->behaviorState = BS_HUNT_AND_KILL;
			//NPC_AngerSound();
			return;
		}
	}

	//FIXME: Implement generic sound alerts
	/*
	alertEventNum = NPC_CheckAlertEvents( qtrue, qtrue );
	if( alertEventNum != -1 )
	{//If we heard something, see if we should check it out
		if ( NPC_CheckInvestigate( alertEventNum ) )
		{
			return;
		}
	}
	*/

	NPCS.NPCInfo->investigateSoundDebounceTime = 0;
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() )
	{
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );

	NPCS.ucmd.buttons |= BUTTON_WALKING;
}

/*
void NPC_BSDefault(void)
	uses various scriptflags to determine how an npc should behave
*/
extern void NPC_CheckGetNewWeapon( void );
extern void NPC_BSST_Attack( void );

void NPC_BSDefault( void )
{
//	vec3_t		enemyDir;
//	float		enemyDist;
//	float		shootDist;
//	qboolean	enemyFOV = qfalse;
//	qboolean	enemyShotFOV = qfalse;
//	qboolean	enemyPVS = qfalse;
//	vec3_t		enemyHead;
//	vec3_t		muzzle;
//	qboolean	enemyLOS = qfalse;
//	qboolean	enemyCS = qfalse;
	qboolean	move = qtrue;
//	qboolean	shoot = qfalse;


	if( NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON )
	{
		WeaponThink( qtrue );
	}

	if ( NPCS.NPCInfo->scriptFlags & SCF_FORCED_MARCH )
	{//being forced to walk
		if( NPCS.NPC->client->ps.torsoAnim != TORSO_SURRENDER_START )
		{
			NPC_SetAnim( NPCS.NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD );
		}
	}
	//look for a new enemy if don't have one and are allowed to look, validate current enemy if have one
	NPC_CheckEnemy( (NPCS.NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES), qfalse, qtrue );
	if ( !NPCS.NPC->enemy )
	{//still don't have an enemy
		if ( !(NPCS.NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
		{//check for alert events
			//FIXME: Check Alert events, see if we should investigate or just look at it
			int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qtrue, AEL_DISCOVERED );

			//There is an event to look at
			if ( alertEvent >= 0 && level.alertEvents[alertEvent].ID != NPCS.NPCInfo->lastAlertID )
			{//heard/saw something
				if ( level.alertEvents[alertEvent].level >= AEL_DISCOVERED && (NPCS.NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
				{//was a big event
					if ( level.alertEvents[alertEvent].owner && level.alertEvents[alertEvent].owner->client && level.alertEvents[alertEvent].owner->health >= 0 && level.alertEvents[alertEvent].owner->client->playerTeam == NPCS.NPC->client->enemyTeam )
					{//an enemy
						G_SetEnemy( NPCS.NPC, level.alertEvents[alertEvent].owner );
					}
				}
				else
				{//FIXME: investigate lesser events
				}
			}
			//FIXME: also check our allies' condition?
		}
	}

	if ( NPCS.NPC->enemy && !(NPCS.NPCInfo->scriptFlags&SCF_FORCED_MARCH) )
	{
		// just use the stormtrooper attack AI...
		NPC_CheckGetNewWeapon();
		if ( NPCS.NPC->client->leader
			&& NPCS.NPCInfo->goalEntity == NPCS.NPC->client->leader
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPCS.NPC, TID_MOVE_NAV ) )
		{
			NPC_ClearGoal();
		}
			NPC_BSST_Attack();
		return;
/*
		//have an enemy
		//FIXME: if one of these fails, meaning we can't shoot, do we really need to do the rest?
		VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, enemyDir );
		enemyDist = VectorNormalize( enemyDir );
		enemyDist *= enemyDist;
		shootDist = NPC_MaxDistSquaredForWeapon();

		enemyFOV = InFOV( NPC->enemy, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov );
		enemyShotFOV = InFOV( NPC->enemy, NPC, 20, 20 );
		enemyPVS = trap->inPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin );

		if ( enemyPVS )
		{//in the pvs
			trace_t	tr;

			CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemyHead );
			enemyHead[2] -= Q_flrand( 0.0f, NPC->enemy->maxs[2]*0.5f );
			CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
			enemyLOS = NPC_ClearLOS( muzzle, enemyHead );

			trap->trace ( &tr, muzzle, vec3_origin, vec3_origin, enemyHead, NPC->s.number, MASK_SHOT );
			enemyCS = NPC_EvaluateShot( tr.entityNum, qtrue );
		}
		else
		{//skip thr 2 traces since they would have to fail
			enemyLOS = qfalse;
			enemyCS = qfalse;
		}

		if ( enemyCS && enemyShotFOV )
		{//can hit enemy if we want
			NPC->cantHitEnemyCounter = 0;
		}
		else
		{//can't hit
			NPC->cantHitEnemyCounter++;
		}

		if ( enemyCS && enemyShotFOV && enemyDist < shootDist )
		{//can shoot
			shoot = qtrue;
			if ( NPCInfo->goalEntity == NPC->enemy )
			{//my goal is my enemy and I have a clear shot, no need to chase right now
				move = qfalse;
			}
		}
		else
		{//don't shoot yet, keep chasing
			shoot = qfalse;
			move = qtrue;
		}

		//shoot decision
		if ( !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
		{//try to shoot
			if ( NPC->enemy )
			{
				if ( shoot )
				{
					if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
					{
						WeaponThink( qtrue );
					}
				}
			}
		}

		//chase decision
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{//go after him
			NPCInfo->goalEntity = NPC->enemy;
			//FIXME: don't need to chase when have a clear shot and in range?
			if ( !enemyCS && NPC->cantHitEnemyCounter > 60 )
			{//haven't been able to shoot enemy for about 6 seconds, need to do something
				//FIXME: combat points?  Just chase?
				if ( enemyPVS )
				{//in my PVS, just pick a combat point
					//FIXME: implement
				}
				else
				{//just chase him
				}
			}
			//FIXME: in normal behavior, should we use combat Points?  Do we care?  Is anyone actually going to ever use this AI?
		}
		else if ( NPC->cantHitEnemyCounter > 60 )
		{//pick a new one
			NPC_CheckEnemy( qtrue, qfalse, qtrue );
		}

		if ( enemyPVS && enemyLOS )//&& !enemyShotFOV )
		{//have a clear LOS to him//, but not looking at him
			//Find the desired angles
			vec3_t	angles;

			GetAnglesForDirection( muzzle, enemyHead, angles );

			NPCInfo->desiredYaw		= AngleNormalize180( angles[YAW] );
			NPCInfo->desiredPitch	= AngleNormalize180( angles[PITCH] );
		}
		*/
	}

	if ( UpdateGoal() )
	{//have a goal
		if ( !NPCS.NPC->enemy
			&& NPCS.NPC->client->leader
			&& NPCS.NPCInfo->goalEntity == NPCS.NPC->client->leader
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPCS.NPC, TID_MOVE_NAV ) )
		{
			NPC_BSFollowLeader();
		}
		else
		{
			//set angles
			if ( (NPCS.NPCInfo->scriptFlags & SCF_FACE_MOVE_DIR) || NPCS.NPCInfo->goalEntity != NPCS.NPC->enemy )
			{//face direction of movement, NOTE: default behavior when not chasing enemy
				NPCS.NPCInfo->combatMove = qfalse;
			}
			else
			{//face goal.. FIXME: what if have a navgoal but want to face enemy while moving?  Will this do that?
				vec3_t	dir, angles;

				NPCS.NPCInfo->combatMove = qfalse;

				VectorSubtract( NPCS.NPCInfo->goalEntity->r.currentOrigin, NPCS.NPC->r.currentOrigin, dir );
				vectoangles( dir, angles );
				NPCS.NPCInfo->desiredYaw = angles[YAW];
				if ( NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy )
				{
					NPCS.NPCInfo->desiredPitch = angles[PITCH];
				}
			}

			//set movement
			//override default walk/run behavior
			//NOTE: redundant, done in NPC_ApplyScriptFlags
			if ( NPCS.NPCInfo->scriptFlags & SCF_RUNNING )
			{
				NPCS.ucmd.buttons &= ~BUTTON_WALKING;
			}
			else if ( NPCS.NPCInfo->scriptFlags & SCF_WALKING )
			{
				NPCS.ucmd.buttons |= BUTTON_WALKING;
			}
			else if ( NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy )
			{
				NPCS.ucmd.buttons &= ~BUTTON_WALKING;
			}
			else
			{
				NPCS.ucmd.buttons |= BUTTON_WALKING;
			}

			if ( NPCS.NPCInfo->scriptFlags & SCF_FORCED_MARCH )
			{//being forced to walk
				//if ( g_crosshairEntNum != NPC->s.number )
				if (!NPC_SomeoneLookingAtMe(NPCS.NPC))
				{//don't walk if player isn't aiming at me
					move = qfalse;
				}
			}

			if ( move )
			{
				//move toward goal
				NPC_MoveToGoal( qtrue );
			}
		}
	}
	else if ( !NPCS.NPC->enemy && NPCS.NPC->client->leader )
	{
		NPC_BSFollowLeader();
	}

	//update angles
	NPC_UpdateAngles( qtrue, qtrue );
}
