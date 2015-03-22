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

#include "bg_vehicles.h"
#include "b_local.h"
#include "ghoul2/G2.h"

extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
extern void WP_CalcVehMuzzle(gentity_t *ent, int muzzleNum);
extern gentity_t *WP_FireVehicleWeapon( gentity_t *ent, vec3_t start, vec3_t dir, vehWeaponInfo_t *vehWeapon, qboolean alt_fire, qboolean isTurretWeap );

extern void G_VehMuzzleFireFX( gentity_t *ent, gentity_t *broadcaster, int muzzlesFired );
//-----------------------------------------------------
void VEH_TurretCheckFire( Vehicle_t *pVeh,
						 gentity_t *parent,
						 //gentity_t *turretEnemy,
						 turretStats_t *turretStats,
						 vehWeaponInfo_t *vehWeapon,
						 int turretNum, int curMuzzle )
{
	// if it's time to fire and we have an enemy, then gun 'em down!  pushDebounce time controls next fire time
	if ( pVeh->m_iMuzzleTag[curMuzzle] == -1 )
	{//invalid muzzle?
		return;
	}

	if ( pVeh->m_iMuzzleWait[curMuzzle] >= level.time )
	{//can't fire yet
		return;
	}

	if ( pVeh->turretStatus[turretNum].ammo < vehWeapon->iAmmoPerShot )
	{//no ammo, can't fire
		return;
	}

	//if ( turretEnemy )
	{
		//FIXME: check to see if I'm aiming generally where I want to
		int nextMuzzle = 0, muzzlesFired = (1<<curMuzzle);
		gentity_t *missile;
		WP_CalcVehMuzzle( parent, curMuzzle );

		//FIXME: some variation in fire dir
		missile = WP_FireVehicleWeapon( parent, pVeh->m_vMuzzlePos[curMuzzle], pVeh->m_vMuzzleDir[curMuzzle], vehWeapon, (turretNum!=0), qtrue );

		//play the weapon's muzzle effect if we have one
		G_VehMuzzleFireFX(parent, missile, muzzlesFired );

		//take the ammo away
		pVeh->turretStatus[turretNum].ammo -= vehWeapon->iAmmoPerShot;
		//toggle to the next muzzle on this turret, if there is one
		nextMuzzle = ((curMuzzle+1)==pVeh->m_pVehicleInfo->turret[turretNum].iMuzzle[0])?pVeh->m_pVehicleInfo->turret[turretNum].iMuzzle[1]:pVeh->m_pVehicleInfo->turret[turretNum].iMuzzle[0];
		if ( nextMuzzle )
		{//a valid muzzle to toggle to
			pVeh->turretStatus[turretNum].nextMuzzle = nextMuzzle-1;//-1 because you type muzzles 1-10 in the .veh file
		}
		//add delay to the next muzzle so it doesn't fire right away on the next frame
		pVeh->m_iMuzzleWait[pVeh->turretStatus[turretNum].nextMuzzle] = level.time + turretStats->iDelay;
	}
}

void VEH_TurretAnglesToEnemy( Vehicle_t *pVeh, int curMuzzle, float fSpeed, gentity_t *turretEnemy, qboolean bAILead, vec3_t desiredAngles )
{
	vec3_t	enemyDir, org;
	VectorCopy( turretEnemy->r.currentOrigin, org );
	if ( bAILead )
	{//we want to lead them a bit
		vec3_t diff, velocity;
		float dist;
		VectorSubtract( org, pVeh->m_vMuzzlePos[curMuzzle], diff );
		dist = VectorNormalize( diff );
		if ( turretEnemy->client )
		{
			VectorCopy( turretEnemy->client->ps.velocity, velocity );
		}
		else
		{
			VectorCopy( turretEnemy->s.pos.trDelta, velocity );
		}
		VectorMA( org, (dist/fSpeed), velocity, org );
	}

	//FIXME: this isn't quite right, it's aiming from the muzzle, not the center of the turret...
	VectorSubtract( org, pVeh->m_vMuzzlePos[curMuzzle], enemyDir );
	//Get the desired absolute, world angles to our target
	vectoangles( enemyDir, desiredAngles );
}

//-----------------------------------------------------
qboolean VEH_TurretAim( Vehicle_t *pVeh,
						 gentity_t *parent,
						 gentity_t *turretEnemy,
						 turretStats_t *turretStats,
						 vehWeaponInfo_t *vehWeapon,
						 int turretNum, int curMuzzle, vec3_t desiredAngles )
//-----------------------------------------------------
{
	vec3_t	curAngles, addAngles, newAngles, yawAngles, pitchAngles;
	float	aimCorrect = qfalse;

	WP_CalcVehMuzzle( parent, curMuzzle );
	//get the current absolute angles of the turret right now
	vectoangles( pVeh->m_vMuzzleDir[curMuzzle], curAngles );
	//subtract out the vehicle's angles to get the relative alignment
	AnglesSubtract( curAngles, pVeh->m_vOrientation, curAngles );

	if ( turretEnemy )
	{
		aimCorrect = qtrue;
		// ...then we'll calculate what new aim adjustments we should attempt to make this frame
		// Aim at enemy
		VEH_TurretAnglesToEnemy( pVeh, curMuzzle, vehWeapon->fSpeed, turretEnemy, turretStats->bAILead, desiredAngles );
	}
	//subtract out the vehicle's angles to get the relative desired alignment
	AnglesSubtract( desiredAngles, pVeh->m_vOrientation, desiredAngles );
	//Now clamp the desired relative angles
	//clamp yaw
	desiredAngles[YAW] = AngleNormalize180( desiredAngles[YAW] );
	if ( pVeh->m_pVehicleInfo->turret[turretNum].yawClampLeft
		&& desiredAngles[YAW] > pVeh->m_pVehicleInfo->turret[turretNum].yawClampLeft )
	{
		aimCorrect = qfalse;
		desiredAngles[YAW] = pVeh->m_pVehicleInfo->turret[turretNum].yawClampLeft;
	}
	if ( pVeh->m_pVehicleInfo->turret[turretNum].yawClampRight
		&& desiredAngles[YAW] < pVeh->m_pVehicleInfo->turret[turretNum].yawClampRight )
	{
		aimCorrect = qfalse;
		desiredAngles[YAW] = pVeh->m_pVehicleInfo->turret[turretNum].yawClampRight;
	}
	//clamp pitch
	desiredAngles[PITCH] = AngleNormalize180( desiredAngles[PITCH] );
	if ( pVeh->m_pVehicleInfo->turret[turretNum].pitchClampDown
		&& desiredAngles[PITCH] > pVeh->m_pVehicleInfo->turret[turretNum].pitchClampDown )
	{
		aimCorrect = qfalse;
		desiredAngles[PITCH] = pVeh->m_pVehicleInfo->turret[turretNum].pitchClampDown;
	}
	if ( pVeh->m_pVehicleInfo->turret[turretNum].pitchClampUp
		&& desiredAngles[PITCH] < pVeh->m_pVehicleInfo->turret[turretNum].pitchClampUp )
	{
		aimCorrect = qfalse;
		desiredAngles[PITCH] = pVeh->m_pVehicleInfo->turret[turretNum].pitchClampUp;
	}
	//Now get the offset we want from our current relative angles
	AnglesSubtract( desiredAngles, curAngles, addAngles );
	//Now cap the addAngles for our fTurnSpeed
	if ( addAngles[PITCH] > turretStats->fTurnSpeed )
	{
		//aimCorrect = qfalse;//???
		addAngles[PITCH] = turretStats->fTurnSpeed;
	}
	else if ( addAngles[PITCH] < -turretStats->fTurnSpeed )
	{
		//aimCorrect = qfalse;//???
		addAngles[PITCH] = -turretStats->fTurnSpeed;
	}
	if ( addAngles[YAW] > turretStats->fTurnSpeed )
	{
		//aimCorrect = qfalse;//???
		addAngles[YAW] = turretStats->fTurnSpeed;
	}
	else if ( addAngles[YAW] < -turretStats->fTurnSpeed )
	{
		//aimCorrect = qfalse;//???
		addAngles[YAW] = -turretStats->fTurnSpeed;
	}
	//Now add the additional angles back in to our current relative angles
	//FIXME: add some AI aim error randomness...?
	newAngles[PITCH] = AngleNormalize180( curAngles[PITCH]+addAngles[PITCH] );
	newAngles[YAW] = AngleNormalize180( curAngles[YAW]+addAngles[YAW] );
	//Now set the bone angles to the new angles
	//set yaw
	if ( turretStats->yawBone )
	{
		VectorClear( yawAngles );
		yawAngles[turretStats->yawAxis] = newAngles[YAW];
		NPC_SetBoneAngles( parent, turretStats->yawBone, yawAngles );
	}
	//set pitch
	if ( turretStats->pitchBone )
	{
		VectorClear( pitchAngles );
		pitchAngles[turretStats->pitchAxis] = newAngles[PITCH];
		NPC_SetBoneAngles( parent, turretStats->pitchBone, pitchAngles );
	}
	//force muzzle to recalc next check
	pVeh->m_iMuzzleTime[curMuzzle] = 0;

	return aimCorrect;
}

//-----------------------------------------------------
static qboolean VEH_TurretFindEnemies( Vehicle_t *pVeh,
						 gentity_t *parent,
						 turretStats_t *turretStats,
						 int turretNum, int curMuzzle )
//-----------------------------------------------------
{
	qboolean	found = qfalse;
	int			i, count;
	float		bestDist = turretStats->fAIRange * turretStats->fAIRange;
	float		enemyDist;
	vec3_t		enemyDir, org, org2;
	qboolean	foundClient = qfalse;
	gentity_t	*entity_list[MAX_GENTITIES], *target, *bestTarget = NULL;

	WP_CalcVehMuzzle( parent, curMuzzle );
	VectorCopy( pVeh->m_vMuzzlePos[curMuzzle], org2 );

	count = G_RadiusList( org2, turretStats->fAIRange, parent, qtrue, entity_list );

	for ( i = 0; i < count; i++ )
	{
		trace_t	tr;
		target = entity_list[i];

		if ( target == parent
			|| !target->takedamage
			|| target->health <= 0
			|| ( target->flags & FL_NOTARGET ))
		{
			continue;
		}
		if ( !target->client )
		{// only attack clients
			if ( !(target->flags&FL_BBRUSH)//not a breakable brush
				|| !target->takedamage//is a bbrush, but invincible
				|| (target->NPC_targetname&&parent->targetname&&Q_stricmp(target->NPC_targetname,parent->targetname)!=0) )//not in invicible bbrush, but can only be broken by an NPC that is not me
			{
				if ( target->s.weapon == WP_TURRET
					&& target->classname
					&& Q_strncmp( "misc_turret", target->classname, 11 ) == 0 )
				{//these guys we want to shoot at
				}
				else
				{
					continue;
				}
			}
			//else: we will shoot at bbrushes!
		}
		else if ( target->client->sess.sessionTeam == TEAM_SPECTATOR )
		{
			continue;
		}
		else if ( target->client->tempSpectate >= level.time )
		{
			continue;
		}
		if ( target == ((gentity_t*)pVeh->m_pPilot)
			|| target->r.ownerNum == parent->s.number )
		{//don't get angry at my pilot or passengers?
			continue;
		}
		if ( parent->client
			&& parent->client->sess.sessionTeam )
		{
			if ( target->client )
			{
				if ( target->client->sess.sessionTeam == parent->client->sess.sessionTeam )
				{
					// A bot/client/NPC we don't want to shoot
					continue;
				}
			}
			else if ( target->teamnodmg == parent->client->sess.sessionTeam )
			{//some other entity that's allied with us
				continue;
			}
		}
		if ( !trap->InPVS( org2, target->r.currentOrigin ))
		{
			continue;
		}

		VectorCopy( target->r.currentOrigin, org );

		trap->Trace( &tr, org2, NULL, NULL, org, parent->s.number, MASK_SHOT, qfalse, 0, 0 );

		if ( tr.entityNum == target->s.number
			|| (!tr.allsolid && !tr.startsolid && tr.fraction == 1.0 ) )
		{
			// Only acquire if have a clear shot, Is it in range and closer than our best?
			VectorSubtract( target->r.currentOrigin, org2, enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < bestDist || (target->client && !foundClient))// all things equal, keep current
			{
				bestTarget = target;
				bestDist = enemyDist;
				found = qtrue;
				if ( target->client )
				{//prefer clients over non-clients
					foundClient = qtrue;
				}
			}
		}
	}

	if ( found )
	{
		pVeh->turretStatus[turretNum].enemyEntNum = bestTarget->s.number;
	}

	return found;
}

void VEH_TurretObeyPassengerControl( Vehicle_t *pVeh, gentity_t *parent, int turretNum )
{
	turretStats_t *turretStats = &pVeh->m_pVehicleInfo->turret[turretNum];
	gentity_t *passenger = (gentity_t *)pVeh->m_ppPassengers[turretStats->passengerNum-1];

	if ( passenger && passenger->client && passenger->health > 0 )
	{//a valid, living passenger client
		vehWeaponInfo_t	*vehWeapon = &g_vehWeaponInfo[turretStats->iWeapon];
		int	curMuzzle = pVeh->turretStatus[turretNum].nextMuzzle;
		vec3_t aimAngles;
		VectorCopy( passenger->client->ps.viewangles, aimAngles );

		VEH_TurretAim( pVeh, parent, NULL, turretStats, vehWeapon, turretNum, curMuzzle, aimAngles );
		if ( (passenger->client->pers.cmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
		{//he's pressing an attack button, so fire!
			VEH_TurretCheckFire( pVeh, parent, turretStats, vehWeapon, turretNum, curMuzzle );
		}
	}
}

void VEH_TurretThink( Vehicle_t *pVeh, gentity_t *parent, int turretNum )
//-----------------------------------------------------
{
	qboolean	doAim = qfalse;
	float		enemyDist, rangeSq;
	vec3_t		enemyDir;
	turretStats_t *turretStats = &pVeh->m_pVehicleInfo->turret[turretNum];
	vehWeaponInfo_t	*vehWeapon = NULL;
	gentity_t	*turretEnemy = NULL;
	int			curMuzzle = 0;//?


	if ( !turretStats || !turretStats->iAmmoMax )
	{//not a valid turret
		return;
	}

	if ( turretStats->passengerNum
		&& pVeh->m_iNumPassengers >= turretStats->passengerNum )
	{//the passenger that has control of this turret is on the ship
		VEH_TurretObeyPassengerControl( pVeh, parent, turretNum );
		return;
	}
	else if ( !turretStats->bAI )//try AI
	{//this turret does not think on its own.
		return;
	}

	vehWeapon = &g_vehWeaponInfo[turretStats->iWeapon];
	rangeSq = (turretStats->fAIRange*turretStats->fAIRange);
	curMuzzle = pVeh->turretStatus[turretNum].nextMuzzle;

	if ( pVeh->turretStatus[turretNum].enemyEntNum < ENTITYNUM_WORLD )
	{
		turretEnemy = &g_entities[pVeh->turretStatus[turretNum].enemyEntNum];
		if ( turretEnemy->health < 0
			|| !turretEnemy->inuse
			|| turretEnemy == ((gentity_t*)pVeh->m_pPilot)//enemy became my pilot///?
			|| turretEnemy == parent
			|| turretEnemy->r.ownerNum == parent->s.number // a passenger?
			|| ( turretEnemy->client && turretEnemy->client->sess.sessionTeam == TEAM_SPECTATOR )
			|| ( turretEnemy->client && turretEnemy->client->tempSpectate >= level.time ) )
		{//don't keep going after spectators, pilot, self, dead people, etc.
			turretEnemy = NULL;
			pVeh->turretStatus[turretNum].enemyEntNum = ENTITYNUM_NONE;
		}
	}

	if ( pVeh->turretStatus[turretNum].enemyHoldTime < level.time )
	{
		if ( VEH_TurretFindEnemies( pVeh, parent, turretStats, turretNum, curMuzzle ) )
		{
			turretEnemy = &g_entities[pVeh->turretStatus[turretNum].enemyEntNum];
			doAim = qtrue;
		}
		else if ( parent->enemy && parent->enemy->s.number < ENTITYNUM_WORLD )
		{
			turretEnemy = parent->enemy;
			doAim = qtrue;
		}
		if ( turretEnemy )
		{//found one
			if ( turretEnemy->client )
			{//hold on to clients for a min of 3 seconds
				pVeh->turretStatus[turretNum].enemyHoldTime = level.time + 3000;
			}
			else
			{//hold less
				pVeh->turretStatus[turretNum].enemyHoldTime = level.time + 500;
			}
		}
	}
	if ( turretEnemy != NULL )
	{
		if ( turretEnemy->health > 0 )
		{
			// enemy is alive
			WP_CalcVehMuzzle( parent, curMuzzle );
			VectorSubtract( turretEnemy->r.currentOrigin, pVeh->m_vMuzzlePos[curMuzzle], enemyDir );
			enemyDist = VectorLengthSquared( enemyDir );

			if ( enemyDist < rangeSq )
			{
				// was in valid radius
				if ( trap->InPVS( pVeh->m_vMuzzlePos[curMuzzle], turretEnemy->r.currentOrigin ) )
				{
					// Every now and again, check to see if we can even trace to the enemy
					trace_t tr;
					vec3_t start, end;
					VectorCopy( pVeh->m_vMuzzlePos[curMuzzle], start );

					VectorCopy( turretEnemy->r.currentOrigin, end );
					trap->Trace( &tr, start, NULL, NULL, end, parent->s.number, MASK_SHOT, qfalse, 0, 0 );

					if ( tr.entityNum == turretEnemy->s.number
						|| (!tr.allsolid && !tr.startsolid ) )
					{
						doAim = qtrue;	// Can see our enemy
					}
				}
			}
		}
	}

	if ( doAim )
	{
		vec3_t aimAngles;
		if ( VEH_TurretAim( pVeh, parent, turretEnemy, turretStats, vehWeapon, turretNum, curMuzzle, aimAngles ) )
		{
			VEH_TurretCheckFire( pVeh, parent, /*turretEnemy,*/ turretStats, vehWeapon, turretNum, curMuzzle );
		}
	}
}
