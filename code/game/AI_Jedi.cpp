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
#include "wp_saber.h"
#include "../qcommon/tri_coll_test.h"
#include "g_navigator.h"
#include "../cgame/cg_local.h"
#include "g_functions.h"

//Externs
extern qboolean G_ValidEnemy( gentity_t *self, gentity_t *enemy );
extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void G_StartMatrixEffect( gentity_t *ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f, int spinTime = 0 );
extern void ForceJump( gentity_t *self, usercmd_t *ucmd );
extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean NPC_CheckEnemyStealth( void );
extern gitem_t	*FindItemForAmmo( ammo_t ammo );
extern void ForceLightning( gentity_t *self );
extern void ForceHeal( gentity_t *self );
extern void ForceRage( gentity_t *self );
extern void ForceProtect( gentity_t *self );
extern void ForceAbsorb( gentity_t *self );
extern qboolean ForceDrain2( gentity_t *self );
extern int WP_MissileBlockForBlock( int saberBlock );
extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_KnockdownTurret( gentity_t *self, gentity_t *pas );
extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength = qfalse );
extern int PM_AnimLength( int index, animNumber_t anim );
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SaberInAttack( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SaberInDeflect( int move );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_InRoll( playerState_t *ps );
extern qboolean PM_InGetUp( playerState_t *ps );
extern qboolean PM_InSpecialJump( int anim );
extern qboolean PM_SuperBreakWinAnim( int anim );
extern qboolean PM_InOnGroundAnim ( playerState_t *ps );
extern qboolean PM_DodgeAnim( int anim );
extern qboolean PM_DodgeHoldAnim( int anim );
extern qboolean PM_InAirKickingAnim( int anim );
extern qboolean PM_KickingAnim( int anim );
extern qboolean PM_StabDownAnim( int anim );
extern qboolean PM_SuperBreakLoseAnim( int anim );
extern qboolean PM_SaberInKata( saberMoveName_t saberMove );
extern qboolean PM_InRollIgnoreTimer( playerState_t *ps );
extern qboolean PM_PainAnim( int anim );
extern qboolean G_CanKickEntity( gentity_t *self, gentity_t *target );
extern saberMoveName_t G_PickAutoKick( gentity_t *self, gentity_t *enemy, qboolean storeMove );
extern saberMoveName_t G_PickAutoMultiKick( gentity_t *self, qboolean allowSingles, qboolean storeMove );
extern qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist );
extern qboolean NAV_MoveDirSafe( gentity_t *self, usercmd_t *cmd, float distScale = 1.0f );
extern float NPC_EnemyRangeFromBolt( int boltIndex );
extern qboolean WP_SabersCheckLock2( gentity_t *attacker, gentity_t *defender, sabersLockMode_t lockMode );
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern qboolean G_EntIsBreakable( int entityNum, gentity_t *breaker );
extern qboolean PM_LockedAnim( int anim );
extern qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask);

extern cvar_t	*g_saberRealisticCombat;
extern cvar_t	*d_slowmodeath;
extern cvar_t	*g_saberNewControlScheme;
extern int parryDebounce[];

//Locals
static void Jedi_Aggression( gentity_t *self, int change );
qboolean Jedi_WaitingAmbush( gentity_t *self );
void Tavion_SithSwordRecharge( void );
qboolean Rosh_BeingHealed( gentity_t *self );

static qboolean enemy_in_striking_range = qfalse;
static int	jediSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several jedi from speaking all at once

void NPC_CultistDestroyer_Precache( void )
{
	G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );
	G_EffectIndex( "force/destruction_exp" );
}

void NPC_ShadowTrooper_Precache( void )
{
	RegisterItem( FindItemForAmmo( AMMO_FORCE ) );
	G_SoundIndex( "sound/chars/shadowtrooper/cloak.wav" );
	G_SoundIndex( "sound/chars/shadowtrooper/decloak.wav" );
}

void NPC_Rosh_Dark_Precache( void )
{
	G_EffectIndex( "force/kothos_recharge.efx" );
	G_EffectIndex( "force/kothos_beam.efx" );
}

void Jedi_ClearTimers( gentity_t *ent )
{
	TIMER_Set( ent, "roamTime", 0 );
	TIMER_Set( ent, "chatter", 0 );
	TIMER_Set( ent, "strafeLeft", 0 );
	TIMER_Set( ent, "strafeRight", 0 );
	TIMER_Set( ent, "noStrafe", 0 );
	TIMER_Set( ent, "walking", 0 );
	TIMER_Set( ent, "taunting", 0 );
	TIMER_Set( ent, "parryTime", 0 );
	TIMER_Set( ent, "parryReCalcTime", 0 );
	TIMER_Set( ent, "forceJumpChasing", 0 );
	TIMER_Set( ent, "jumpChaseDebounce", 0 );
	TIMER_Set( ent, "moveforward", 0 );
	TIMER_Set( ent, "moveback", 0 );
	TIMER_Set( ent, "movenone", 0 );
	TIMER_Set( ent, "moveright", 0 );
	TIMER_Set( ent, "moveleft", 0 );
	TIMER_Set( ent, "movecenter", 0 );
	TIMER_Set( ent, "saberLevelDebounce", 0 );
	TIMER_Set( ent, "noRetreat", 0 );
	TIMER_Set( ent, "holdLightning", 0 );
	TIMER_Set( ent, "gripping", 0 );
	TIMER_Set( ent, "draining", 0 );
	TIMER_Set( ent, "noturn", 0 );
	TIMER_Set( ent, "specialEvasion", 0 );
}

qboolean Jedi_CultistDestroyer( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return qfalse;
	}
	//FIXME: just make a flag, dude!
	if ( self->client->NPC_class == CLASS_REBORN
		&& self->s.weapon == WP_MELEE
		&& Q_stricmp( "cultist_destroyer", self->NPC_type ) == 0 )
	{
		return qtrue;
	}
	return qfalse;
}

void Jedi_PlayBlockedPushSound( gentity_t *self )
{
	if ( !self->s.number )
	{
		G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void Jedi_PlayDeflectSound( gentity_t *self )
{
	if ( !self->s.number )
	{
		G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void NPC_Jedi_PlayConfusionSound( gentity_t *self )
{
	if ( self->health > 0 )
	{
		if ( self->client
			&& ( self->client->NPC_class == CLASS_ALORA
				|| self->client->NPC_class == CLASS_TAVION
				|| self->client->NPC_class == CLASS_DESANN ) )
		{
			G_AddVoiceEvent( self, Q_irand( EV_CONFUSE1, EV_CONFUSE3 ), 2000 );
		}
		else if ( Q_irand( 0, 1 ) )
		{
			G_AddVoiceEvent( self, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 2000 );
		}
		else
		{
			G_AddVoiceEvent( self, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 2000 );
		}
	}
}

qboolean Jedi_StopKnockdown( gentity_t *self, gentity_t *pusher, const vec3_t pushDir )
{
	if ( self->s.number < MAX_CLIENTS || !self->NPC )
	{//only NPCs
		return qfalse;
	}

	if ( self->client->ps.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1 )
	{//only force-users
		return qfalse;
	}

	if ( self->client->moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	if ( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
	{//bosses always get out of a knockdown
	}
	else if ( Q_irand( 0, RANK_CAPTAIN+5 ) > self->NPC->rank )
	{//lower their rank, the more likely they are fall down
		return qfalse;
	}

	vec3_t	pDir, fwd, right, ang = {0, self->currentAngles[YAW], 0};
	float	fDot, rDot;
	int		strafeTime = Q_irand( 1000, 2000 );

	AngleVectors( ang, fwd, right, NULL );
	VectorNormalize2( pushDir, pDir );
	fDot = DotProduct( pDir, fwd );
	rDot = DotProduct( pDir, right );

	//flip or roll with it
	usercmd_t	tempCmd;
	if ( fDot >= 0.4f )
	{
		tempCmd.forwardmove = 127;
		TIMER_Set( self, "moveforward", strafeTime );
	}
	else if ( fDot <= -0.4f )
	{
		tempCmd.forwardmove = -127;
		TIMER_Set( self, "moveback", strafeTime );
	}
	else if ( rDot > 0 )
	{
		tempCmd.rightmove = 127;
		TIMER_Set( self, "strafeRight", strafeTime );
		TIMER_Set( self, "strafeLeft", -1 );
	}
	else
	{
		tempCmd.rightmove = -127;
		TIMER_Set( self, "strafeLeft", strafeTime );
		TIMER_Set( self, "strafeRight", -1 );
	}
	G_AddEvent( self, EV_JUMP, 0 );
	if ( !Q_irand( 0, 1 ) )
	{//flip
		self->client->ps.forceJumpCharge = 280;//FIXME: calc this intelligently?
		ForceJump( self, &tempCmd );
	}
	else
	{//roll
		TIMER_Set( self, "duck", strafeTime );
	}
	self->painDebounceTime = 0;//so we do something

	return qtrue;
}
extern void Boba_FireDecide( void );
extern void RT_FireDecide( void );
extern void Boba_FlyStart( gentity_t *self );





//===============================================================================================
//TAVION BOSS
//===============================================================================================
void NPC_TavionScepter_Precache( void )
{
	G_EffectIndex( "scepter/beam_warmup.efx" );
	G_EffectIndex( "scepter/beam.efx" );
	G_EffectIndex( "scepter/slam_warmup.efx" );
	G_EffectIndex( "scepter/slam.efx" );
	G_EffectIndex( "scepter/impact.efx" );
	G_SoundIndex( "sound/weapons/scepter/loop.wav" );
	G_SoundIndex( "sound/weapons/scepter/slam_warmup.wav" );
	G_SoundIndex( "sound/weapons/scepter/beam_warmup.wav" );
}

void NPC_TavionSithSword_Precache( void )
{
	G_EffectIndex( "scepter/recharge.efx" );
	G_EffectIndex( "scepter/invincibility.efx" );
	G_EffectIndex( "scepter/sword.efx" );
	G_SoundIndex( "sound/weapons/scepter/recharge.wav" );
}

void Tavion_ScepterDamage( void )
{
	if ( !NPC->ghoul2.size()
		|| NPC->weaponModel[1] <= 0 )
	{
		return;
	}

	if ( NPC->genericBolt1 != -1 )
	{
		int curTime = (cg.time?cg.time:level.time);
		qboolean hit = qfalse;
		int	lastHit = ENTITYNUM_NONE;
		for ( int time = curTime-25; time <= curTime+25&&!hit; time += 25 )
		{
			mdxaBone_t	boltMatrix;
			vec3_t		tip, dir, base, angles={0,NPC->currentAngles[YAW],0};
			trace_t		trace;

			gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->weaponModel[1],
						NPC->genericBolt1,
						&boltMatrix, angles, NPC->currentOrigin, time,
						NULL, NPC->s.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, base );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, dir );
			VectorMA( base, 512, dir, tip );
	#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				G_DebugLine(base, tip, 1000, 0x000000ff, qtrue);
			}
	#endif
			gi.trace( &trace, base, vec3_origin, vec3_origin, tip, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
			if ( trace.fraction < 1.0f )
			{//hit something
				gentity_t *traceEnt = &g_entities[trace.entityNum];

				//FIXME: too expensive!
				//if ( time == curTime )
				{//UGH
					G_PlayEffect( G_EffectIndex( "scepter/impact.efx" ), trace.endpos, trace.plane.normal );
				}

				if ( traceEnt->takedamage
					&& trace.entityNum != lastHit
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->NPC_class) )
				{//smack
					int dmg = Q_irand( 10, 20 )*(g_spskill->integer+1);//NOTE: was 6-12
					//FIXME: debounce?
					//FIXME: do dismemberment
					G_Damage( traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_SABER );//MOD_MELEE );
					if ( traceEnt->client )
					{
						if ( !Q_irand( 0, 2 ) )
						{
							G_AddVoiceEvent( NPC, Q_irand( EV_CONFUSE1, EV_CONFUSE2 ), 10000 );
						}
						else
						{
							G_AddVoiceEvent( NPC, EV_JDETECTED3, 10000 );
						}
						G_Throw( traceEnt, dir, Q_flrand( 50, 80 ) );
						if ( traceEnt->health > 0 && !Q_irand( 0, 2 ) )//FIXME: base on skill!
						{//do pain on enemy
							G_Knockdown( traceEnt, NPC, dir, 300, qtrue );
						}
					}
					hit = qtrue;
					lastHit = trace.entityNum;
				}
			}
		}
	}
}

void Tavion_ScepterSlam( void )
{
	if ( !NPC->ghoul2.size()
		|| NPC->weaponModel[1] <= 0 )
	{
		return;
	}

	int	boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[1]], "*weapon");
	if ( boltIndex != -1 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		handle, bottom, angles={0,NPC->currentAngles[YAW],0};
		trace_t		trace;
		gentity_t	*radiusEnts[ 128 ];
		int			numEnts;
		const float	radius = 300.0f;
		const float	halfRad = (radius/2);
		float		dist;
		int			i;
		vec3_t		mins, maxs, entDir;

		gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->weaponModel[1],
					boltIndex,
					&boltMatrix, angles, NPC->currentOrigin, (cg.time?cg.time:level.time),
					NULL, NPC->s.modelScale );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, handle );
		VectorCopy( handle, bottom );
		bottom[2] -= 128.0f;

		gi.trace( &trace, handle, vec3_origin, vec3_origin, bottom, NPC->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
		G_PlayEffect( G_EffectIndex( "scepter/slam.efx" ), trace.endpos, trace.plane.normal );

		//Setup the bbox to search in
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = trace.endpos[i] - radius;
			maxs[i] = trace.endpos[i] + radius;
		}

		//Get the number of entities in a given space
		numEnts = gi.EntitiesInBox( mins, maxs, radiusEnts, 128 );

		for ( i = 0; i < numEnts; i++ )
		{
			if ( !radiusEnts[i]->inuse )
			{
				continue;
			}

			if ( (radiusEnts[i]->flags&FL_NO_KNOCKBACK) )
			{//don't throw them back
				continue;
			}

			if ( radiusEnts[i] == NPC )
			{//Skip myself
				continue;
			}

			if ( radiusEnts[i]->client == NULL )
			{//must be a client
				if ( G_EntIsBreakable( radiusEnts[i]->s.number, NPC ) )
				{//damage breakables within range, but not as much
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, 100, 0, MOD_EXPLOSIVE_SPLASH );
				}
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR)
				|| (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			{//can't be one being held
				continue;
			}

			VectorSubtract( radiusEnts[i]->currentOrigin, trace.endpos, entDir );
			dist = VectorNormalize( entDir );
			if ( dist <= radius )
			{
				if ( dist < halfRad )
				{//close enough to do damage, too
					G_Damage( radiusEnts[i], NPC, NPC, vec3_origin, radiusEnts[i]->currentOrigin, Q_irand( 20, 30 ), DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE_SPLASH );
				}
				if ( radiusEnts[i]->client
					&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
					&& radiusEnts[i]->client->NPC_class != CLASS_ATST )
				{
					float throwStr = 0.0f;
					if ( g_spskill->integer > 1 )
					{
						throwStr = 10.0f+((radius-dist)/2.0f);
						if ( throwStr > 150.0f )
						{
							throwStr = 150.0f;
						}
					}
					else
					{
						throwStr = 10.0f+((radius-dist)/4.0f);
						if ( throwStr > 85.0f )
						{
							throwStr = 85.0f;
						}
					}
					entDir[2] += 0.1f;
					VectorNormalize( entDir );
					G_Throw( radiusEnts[i], entDir, throwStr );
					if ( radiusEnts[i]->health > 0 )
					{
						if ( dist < halfRad
							|| radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE )
						{//within range of my fist or within ground-shaking range and not in the air
							G_Knockdown( radiusEnts[i], NPC, vec3_origin, 500, qtrue );
						}
					}
				}
			}
		}
	}
}

void Tavion_StartScepterBeam( void )
{
	G_PlayEffect( G_EffectIndex( "scepter/beam_warmup.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, 0, qtrue );
	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/weapons/scepter/beam_warmup.wav" );
	NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.torsoAnimTimer += 200;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
}

void Tavion_StartScepterSlam( void )
{
	G_PlayEffect( G_EffectIndex( "scepter/slam_warmup.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, 0, qtrue );
	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/weapons/scepter/slam_warmup.wav" );
	NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TAVION_SCEPTERGROUND, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
	NPC->count = 0;
}

void Tavion_SithSwordRecharge( void )
{
	if ( NPC->client->ps.torsoAnim != BOTH_TAVION_SWORDPOWER
		&& NPC->count
		&& TIMER_Done( NPC, "rechargeDebounce" )
		&& NPC->weaponModel[0] != -1 )
	{
		NPC->s.loopSound = G_SoundIndex( "sound/weapons/scepter/recharge.wav" );
		int	boltIndex = gi.G2API_AddBolt(&NPC->ghoul2[NPC->weaponModel[0]], "*weapon");
		NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TAVION_SWORDPOWER, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		G_PlayEffect( G_EffectIndex( "scepter/recharge.efx" ), NPC->weaponModel[0], boltIndex, NPC->s.number, NPC->currentOrigin, NPC->client->ps.torsoAnimTimer, qtrue );
		NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
		NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
		NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		VectorClear( NPC->client->ps.velocity );
		VectorClear( NPC->client->ps.moveDir );
		NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->ps.torsoAnimTimer + 10000;
		G_PlayEffect( G_EffectIndex( "scepter/invincibility.efx" ), NPC->playerModel, 0, NPC->s.number, NPC->currentOrigin, NPC->client->ps.torsoAnimTimer + 10000, qfalse );
		TIMER_Set( NPC, "rechargeDebounce", NPC->client->ps.torsoAnimTimer + 10000 + Q_irand(10000,20000) );
		NPC->count--;
		//now you have a chance of killing her
		NPC->flags &= ~FL_UNDYING;
	}
}

//======================================================================================
//END TAVION BOSS
//======================================================================================

void Jedi_Cloak( gentity_t *self )
{
	if ( self && self->client )
	{
		if ( !self->client->ps.powerups[PW_CLOAKED] )
		{//cloak
			self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: debounce attacks?
			//FIXME: temp sound
			G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav" );
		}
	}
}

void Jedi_Decloak( gentity_t *self )
{
	if ( self && self->client )
	{
		if ( self->client->ps.powerups[PW_CLOAKED] )
		{//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: temp sound
			G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav" );
		}
	}
}

void Jedi_CheckCloak( void )
{
	if ( NPC
		&& NPC->client
		&& NPC->client->NPC_class == CLASS_SHADOWTROOPER
		&& Q_stricmpn("shadowtrooper", NPC->NPC_type, 13 ) == 0 )
	{
		if ( NPC->client->ps.SaberActive() ||
			NPC->health <= 0 ||
			NPC->client->ps.saberInFlight ||
			(NPC->client->ps.eFlags&EF_FORCE_GRIPPED) ||
			(NPC->client->ps.eFlags&EF_FORCE_DRAINED) ||
			NPC->painDebounceTime > level.time )
		{//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak( NPC );
		}
		else if ( NPC->health > 0
			&& !NPC->client->ps.saberInFlight
			&& !(NPC->client->ps.eFlags&EF_FORCE_GRIPPED)
			&& !(NPC->client->ps.eFlags&EF_FORCE_DRAINED)
			&& NPC->painDebounceTime < level.time )
		{//still alive, have saber in hand, not taking pain and not being gripped
			Jedi_Cloak( NPC );
		}
	}
}
/*
==========================================================================================
AGGRESSION
==========================================================================================
*/
static void Jedi_Aggression( gentity_t *self, int change )
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
		if ( self->client->NPC_class == CLASS_DESANN )
		{
			upper_threshold = 20;
			lower_threshold = 5;
		}
		else
		{
			upper_threshold = 10;
			lower_threshold = 3;
		}
	}

	if ( self->NPC->stats.aggression > upper_threshold )
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if ( self->NPC->stats.aggression < lower_threshold )
	{
		self->NPC->stats.aggression = lower_threshold;
	}
	//Com_Printf( "(%d) %s agg %d change: %d\n", level.time, self->NPC_type, self->NPC->stats.aggression, change );
}

static void Jedi_AggressionErosion( int amt )
{
	if ( TIMER_Done( NPC, "roamTime" ) )
	{//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set( NPC, "roamTime", Q_irand( 2000, 5000 ) );
		Jedi_Aggression( NPC, amt );
	}

	if ( NPCInfo->stats.aggression < 4 || (NPCInfo->stats.aggression < 6&&NPC->client->NPC_class == CLASS_DESANN))
	{//turn off the saber
		WP_DeactivateSaber( NPC );
	}
}

void NPC_Jedi_RateNewEnemy( gentity_t *self, gentity_t *enemy )
{
	float healthAggression;
	float weaponAggression;

	switch( enemy->s.weapon )
	{
	case WP_SABER:
		healthAggression = (float)self->health/200.0f*6.0f;
		weaponAggression = 7;//go after him
		break;
	case WP_BLASTER:
		if ( DistanceSquared( self->currentOrigin, enemy->currentOrigin ) < 65536 )//256 squared
		{
			healthAggression = (float)self->health/200.0f*8.0f;
			weaponAggression = 8;//go after him
		}
		else
		{
			healthAggression = 8.0f - ((float)self->health/200.0f*8.0f);
			weaponAggression = 2;//hang back for a second
		}
		break;
	default:
		healthAggression = (float)self->health/200.0f*8.0f;
		weaponAggression = 6;//approach
		break;
	}
	//Average these with current aggression
	int newAggression = ceil( (healthAggression + weaponAggression + (float)self->NPC->stats.aggression )/3.0f);
	//Com_Printf( "(%d) new agg %d - new enemy\n", level.time, newAggression );
	Jedi_Aggression( self, newAggression - self->NPC->stats.aggression );

	//don't taunt right away
	TIMER_Set( self, "chatter", Q_irand( 4000, 7000 ) );
}

static void Jedi_Rage( void )
{
	Jedi_Aggression( NPC, 10 - NPCInfo->stats.aggression + Q_irand( -2, 2 ) );
	TIMER_Set( NPC, "roamTime", 0 );
	TIMER_Set( NPC, "chatter", 0 );
	TIMER_Set( NPC, "walking", 0 );
	TIMER_Set( NPC, "taunting", 0 );
	TIMER_Set( NPC, "jumpChaseDebounce", 0 );
	TIMER_Set( NPC, "movenone", 0 );
	TIMER_Set( NPC, "movecenter", 0 );
	TIMER_Set( NPC, "noturn", 0 );
	ForceRage( NPC );
}

void Jedi_RageStop( gentity_t *self )
{
	if ( self->NPC )
	{//calm down and back off
		TIMER_Set( self, "roamTime", 0 );
		Jedi_Aggression( self, Q_irand( -5, 0 ) );
	}
}
/*
==========================================================================================
SPEAKING
==========================================================================================
*/

static qboolean Jedi_BattleTaunt( void )
{
	if ( TIMER_Done( NPC, "chatter" )
		&& !Q_irand( 0, 3 )
		&& NPCInfo->blockedSpeechDebounceTime < level.time
		&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time )
	{
		int event = -1;
		if ( NPC->enemy
			&& NPC->enemy->client
			&& (NPC->enemy->client->NPC_class == CLASS_RANCOR
				|| NPC->enemy->client->NPC_class == CLASS_WAMPA
				|| NPC->enemy->client->NPC_class == CLASS_SAND_CREATURE) )
		{//never taunt these mindless creatures
			//NOTE: howlers?  tusken?  etc?  Only reborn?
		}
		else
		{
			if ( NPC->client->playerTeam == TEAM_PLAYER
				&& NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI )
			{//a jedi fighting a jedi - training
				if ( NPC->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER )
				{//only trainer taunts
					event = EV_TAUNT1;
				}
			}
			else
			{//reborn or a jedi fighting an enemy
				event = Q_irand( EV_TAUNT1, EV_TAUNT3 );
			}
			if ( event != -1 )
			{
				G_AddVoiceEvent( NPC, event, 3000 );
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 6000;
				if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
				{
					TIMER_Set( NPC, "chatter", Q_irand( 8000, 20000 ) );
				}
				else
				{
					TIMER_Set( NPC, "chatter", Q_irand( 5000, 10000 ) );
				}

				if ( NPC->enemy && NPC->enemy->NPC && NPC->enemy->s.weapon == WP_SABER && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI )
				{//Have the enemy jedi say something in response when I'm done?
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
==========================================================================================
MOVEMENT
==========================================================================================
*/
static qboolean Jedi_ClearPathToSpot( vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	mins, start, end, dir;
	float	dist, drop;

	//Offset the step height
	VectorSet( mins, NPC->mins[0], NPC->mins[1], NPC->mins[2] + STEPSIZE );

	gi.trace( &trace, NPC->currentOrigin, mins, NPC->maxs, dest, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return qfalse;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if ( impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum )
		{//hit what we're going after
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	//otherwise, clear path in a straight line.
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract( dest, NPC->currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPC->currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( float i = NPC->maxs[0]*2; i < dist; i += NPC->maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPC->currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		gi.trace( &trace, start, mins, NPC->maxs, end, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );//NPC->mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset )
{
	vec3_t	forward, right, testPos, angles, mins;
	trace_t	trace;
	float	fwdDist, rtDist;
	float	bottom_max = -STEPSIZE*4 - 1;

	if ( !forwardmove && !rightmove )
	{//not even moving
		//gi.Printf( "%d skipping walk-cliff check (not moving)\n", level.time );
		return qtrue;
	}

	if ( ucmd.upmove > 0 || NPC->client->ps.forceJumpCharge )
	{//Going to jump
		//gi.Printf( "%d skipping walk-cliff check (going to jump)\n", level.time );
		return qtrue;
	}

	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in the air
		//gi.Printf( "%d skipping walk-cliff check (in air)\n", level.time );
		return qtrue;
	}
	/*
	if ( fabs( AngleDelta( NPC->currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
	{//Not turning much, don't do this
		//NOTE: Should this not happen only if you're not turning AT ALL?
		//	You could be turning slowly but moving fast, so that would
		//	still let you walk right off a cliff...
		//NOTE: Or maybe it is a good idea to ALWAYS do this, regardless
		//	of whether ot not we're turning?  But why would we be walking
		//  straight into a wall or off	a cliff unless we really wanted to?
		return;
	}
	*/

	//FIXME: to really do this right, we'd have to actually do a pmove to predict where we're
	//going to be... maybe this should be a flag and pmove handles it and sets a flag so AI knows
	//NEXT frame?  Or just incorporate current velocity, runspeed and possibly friction?
	VectorCopy( NPC->mins, mins );
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = NPC->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
	AngleVectors( angles, forward, right, NULL );
	fwdDist = ((float)forwardmove)/2.0f;
	rtDist = ((float)rightmove)/2.0f;
	VectorMA( NPC->currentOrigin, fwdDist, forward, testPos );
	VectorMA( testPos, rtDist, right, testPos );
	gi.trace( &trace, NPC->currentOrigin, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
	if ( trace.allsolid || trace.startsolid )
	{//hmm, trace started inside this brush... how do we decide if we should continue?
		//FIXME: what do we do if we start INSIDE a CONTENTS_BOTCLIP? Try the trace again without that in the clipmask?
		if ( reset )
		{
			trace.fraction = 1.0f;
		}
		VectorCopy( testPos, trace.endpos );
		//return qtrue;
	}
	if ( trace.fraction < 0.6 )
	{//Going to bump into something very close, don't move, just turn
		if ( (NPC->enemy && trace.entityNum == NPC->enemy->s.number) || (NPCInfo->goalEntity && trace.entityNum == NPCInfo->goalEntity->s.number) )
		{//okay to bump into enemy or goal
			//gi.Printf( "%d bump into enemy/goal okay\n", level.time );
			return qtrue;
		}
		else if ( reset )
		{//actually want to screw with the ucmd
			//gi.Printf( "%d avoiding walk into wall (entnum %d)\n", level.time, trace.entityNum );
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			VectorClear( NPC->client->ps.moveDir );
		}
		return qfalse;
	}

	if ( NPCInfo->goalEntity )
	{
		if ( NPCInfo->goalEntity->currentOrigin[2] < NPC->currentOrigin[2] )
		{//goal is below me, okay to step off at least that far plus stepheight
			bottom_max += NPCInfo->goalEntity->currentOrigin[2] - NPC->currentOrigin[2];
		}
	}
	VectorCopy( trace.endpos, testPos );
	testPos[2] += bottom_max;

	gi.trace( &trace, trace.endpos, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );

	//FIXME:Should we try to see if we can still get to our goal using the waypoint network from this trace.endpos?
	//OR: just put NPC clip brushes on these edges (still fall through when die)

	if ( trace.allsolid || trace.startsolid )
	{//Not going off a cliff
		//gi.Printf( "%d walk off cliff okay (droptrace in solid)\n", level.time );
		return qtrue;
	}

	if ( trace.fraction < 1.0 )
	{//Not going off a cliff
		//FIXME: what if plane.normal is sloped?  We'll slide off, not land... plus this doesn't account for slide-movement...
		//gi.Printf( "%d walk off cliff okay will hit entnum %d at dropdist of %4.2f\n", level.time, trace.entityNum, (trace.fraction*bottom_max) );
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if ( reset )
	{//actually want to screw with the ucmd
		//gi.Printf( "%d avoiding walk off cliff\n", level.time );
		ucmd.forwardmove *= -1.0;//= 0;
		ucmd.rightmove *= -1.0;//= 0;
		VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
	}
	return qfalse;
}
/*
-------------------------
Jedi_HoldPosition
-------------------------
*/

static void Jedi_HoldPosition( void )
{
	//NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
	NPCInfo->goalEntity = NULL;

	/*
	if ( TIMER_Done( NPC, "stand" ) )
	{
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

/*
-------------------------
Jedi_Move
-------------------------
*/

static qboolean Jedi_Move( gentity_t *goal, qboolean retreat )
{
	NPCInfo->combatMove = qtrue;
	NPCInfo->goalEntity = goal;

	qboolean	moved = NPC_MoveToGoal( qtrue );
	if (!moved)
	{
		Jedi_HoldPosition();
	}

	// NAV_TODO: Put Retreate Behavior Here
	//FIXME: temp retreat behavior- should really make this toward a safe spot or maybe to outflank enemy
	if ( retreat )
	{//FIXME: should we trace and make sure we can go this way?  Or somehow let NPC_MoveToGoal know we want to retreat and have it handle it?
		ucmd.forwardmove *= -1;
		ucmd.rightmove *= -1;
		//we clear moveDir here so the Jedi's ucmd-driven movement does do not enter checks
		VectorClear( NPC->client->ps.moveDir );
		//VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
	}
	return moved;
}

static qboolean Jedi_Hunt( void )
{
	//gi.Printf( "Hunting\n" );
	//if we're at all interested in fighting, go after him
	if ( NPCInfo->stats.aggression > 1 )
	{//approach enemy
		NPCInfo->combatMove = qtrue;
		if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
		{
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
		}
		else
		{
		/*	if ( NPCInfo->goalEntity == NULL )
			{//hunt
				NPCInfo->goalEntity = NPC->enemy;
			}
			if (NPC->client && NPC->client->NPC_class==CLASS_BOBAFETT)
			{
				NPCInfo->goalEntity = NPC->enemy;
			}*/
//			NPC_SetMoveGoal(NPC, NPC->enemy->currentOrigin, 40.0f, false, 0, NPC->enemy);
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = 40.0f;

			//Jedi_Move( NPC->enemy, qfalse );
			if ( NPC_MoveToGoal( qfalse ) )
			{
				NPC_UpdateAngles( qtrue, qtrue );
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
static qboolean Jedi_Track( void )
{
	//if we're at all interested in fighting, go after him
	if ( NPCInfo->stats.aggression > 1 )
	{//approach enemy
		NPCInfo->combatMove = qtrue;
		NPC_SetMoveGoal( NPC, NPCInfo->enemyLastSeenLocation, 16, qtrue );
		if ( NPC_MoveToGoal( qfalse ) )
		{
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
		}
	}
	return qfalse;
}
*/

static void Jedi_StartBackOff( void )
{
	TIMER_Set( NPC, "roamTime", -level.time );
	TIMER_Set( NPC, "strafeLeft", -level.time );
	TIMER_Set( NPC, "strafeRight", -level.time );
	TIMER_Set( NPC, "walking", -level.time );
	TIMER_Set( NPC, "moveforward", -level.time );
	TIMER_Set( NPC, "movenone", -level.time );
	TIMER_Set( NPC, "moveright", -level.time );
	TIMER_Set( NPC, "moveleft", -level.time );
	TIMER_Set( NPC, "movecenter", -level.time );
	TIMER_Set( NPC, "moveback", 1000 );
	ucmd.forwardmove = -127;
	ucmd.rightmove = 0;
	ucmd.upmove = 0;
	if ( d_JediAI->integer )
	{
		Com_Printf( "%s backing off from spin attack!\n", NPC->NPC_type );
	}
	TIMER_Set( NPC, "specialEvasion", 1000 );
	TIMER_Set( NPC, "noRetreat", -level.time );
	if ( PM_PainAnim(NPC->client->ps.legsAnim) )
	{
		NPC->client->ps.legsAnimTimer = 0;
	}
	VectorClear( NPC->client->ps.moveDir );
}

static qboolean Jedi_Retreat( void )
{
	if ( !TIMER_Done( NPC, "noRetreat" ) )
	{//don't actually move
		return qfalse;
	}
	//FIXME: when retreating, we should probably see if we can retreat
	//in the direction we want.  If not...?  Evade?
	//gi.Printf( "Retreating\n" );
	return Jedi_Move( NPC->enemy, qtrue );
}

static qboolean Jedi_Advance( void )
{
	if ( (NPCInfo->aiFlags&NPCAI_HEAL_ROSH) )
	{
		return qfalse;
	}
	if ( !NPC->client->ps.saberInFlight )
	{
		NPC->client->ps.SaberActivate();
	}
	//gi.Printf( "Advancing\n" );
	return Jedi_Move( NPC->enemy, qfalse );

	//TIMER_Set( NPC, "roamTime", Q_irand( 2000, 4000 ) );
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );
	//TIMER_Set( NPC, "duck", 0 );
}

static void Jedi_AdjustSaberAnimLevel( gentity_t *self, int newLevel )
{
	if ( !self || !self->client )
	{
		return;
	}
	//FIXME: each NPC shold have a unique pattern of behavior for the order in which they
	if ( self->client->playerTeam == TEAM_ENEMY )
	{
		//FIXME: CLASS_CULTIST + self->NPC->rank instead of these Q_stricmps?
		if ( !Q_stricmp( "cultist_saber_all", self->NPC_type )
			|| !Q_stricmp( "cultist_saber_all_throw", self->NPC_type ) )
		{//use any, regardless of rank, etc.
		}
		else if ( !Q_stricmp( "cultist_saber", self->NPC_type )
				|| !Q_stricmp( "cultist_saber_throw", self->NPC_type ) )
		{//fast only
			self->client->ps.saberAnimLevel = SS_FAST;
		}
		else if ( !Q_stricmp( "cultist_saber_med", self->NPC_type )
					|| !Q_stricmp( "cultist_saber_med_throw", self->NPC_type ) )
		{//med only
			self->client->ps.saberAnimLevel = SS_MEDIUM;
		}
		else if ( !Q_stricmp( "cultist_saber_strong", self->NPC_type )
				|| !Q_stricmp( "cultist_saber_strong_throw", self->NPC_type ) )
		{//strong only
			self->client->ps.saberAnimLevel = SS_STRONG;
		}
		else
		{//regular reborn
			if ( self->NPC->rank == RANK_CIVILIAN || self->NPC->rank == RANK_LT_JG )
			{//grunt and fencer always uses quick attacks
				self->client->ps.saberAnimLevel = SS_FAST;
				return;
			}
			if ( self->NPC->rank == RANK_CREWMAN
				|| self->NPC->rank == RANK_ENSIGN )
			{//acrobat & force-users always use medium attacks
				self->client->ps.saberAnimLevel = SS_MEDIUM;
				return;
			}
			/*
			if ( self->NPC->rank == RANK_LT )
			{//boss always uses strong attacks
				self->client->ps.saberAnimLevel = SS_STRONG;
				return;
			}
			*/
		}
	}
	if ( newLevel < SS_FAST )
	{
		newLevel = SS_FAST;
	}
	else if ( newLevel > SS_STAFF )
	{
		newLevel = SS_STAFF;
	}
	//use the different attacks, how often they switch and under what circumstances
	if ( !(self->client->ps.saberStylesKnown&(1<<newLevel)) )
	{//don't know that style, sorry
		return;
	}
	else
	{//go ahead and set it
		self->client->ps.saberAnimLevel = newLevel;
	}

	if ( d_JediAI->integer )
	{
		switch ( self->client->ps.saberAnimLevel )
		{
		case SS_FAST:
			gi.Printf( S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type );
			break;
		case SS_MEDIUM:
			gi.Printf( S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type );
			break;
		case SS_STRONG:
			gi.Printf( S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type );
			break;
		}
	}
}

static void Jedi_CheckDecreaseSaberAnimLevel( void )
{
	if ( !NPC->client->ps.weaponTime && !(ucmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS)) )
	{//not attacking
		if ( TIMER_Done( NPC, "saberLevelDebounce" ) && !Q_irand( 0, 10 ) )
		{
			//Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel-1) );//drop
			Jedi_AdjustSaberAnimLevel( NPC, Q_irand( SS_FAST, SS_STRONG ));//random
			TIMER_Set( NPC, "saberLevelDebounce", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{
		TIMER_Set( NPC, "saberLevelDebounce", Q_irand( 1000, 5000 ) );
	}
}

static qboolean Jedi_DecideKick( void )
{
	if ( PM_InKnockDown( &NPC->client->ps ) )
	{
		return qfalse;
	}
	if ( PM_InRoll( &NPC->client->ps ) )
	{
		return qfalse;
	}
	if ( PM_InGetUp( &NPC->client->ps ) )
	{
		return qfalse;
	}
	if ( !NPC->enemy || (NPC->enemy->s.number < MAX_CLIENTS&&NPC->enemy->health<=0) )
	{//have no enemy or enemy is a dead player
		return qfalse;
	}
	//FIXME: check FP_SABER_OFFENSE?
	//FIXME: check for saber staff style only?
	//FIXME: g_spskill?
	if ( Q_irand( 0, RANK_CAPTAIN+5 ) > NPCInfo->rank )
	{//low chance, based on rank
		return qfalse;
	}
	if ( Q_irand( 0, 10 ) > NPCInfo->stats.aggression )
	{//the madder the better
		return qfalse;
	}
	if ( !TIMER_Done( NPC, "kickDebounce" ) )
	{//just did one
		return qfalse;
	}
	if ( NPC->client->ps.weapon == WP_SABER )
	{
		if ( (NPC->client->ps.saber[0].saberFlags&SFL_NO_KICKS) )
		{
			return qfalse;
		}
		else if ( NPC->client->ps.dualSabers
			&& (NPC->client->ps.saber[1].saberFlags&SFL_NO_KICKS) )
		{
			return qfalse;
		}
	}
	//go for it!
	return qtrue;
}

void Kyle_GrabEnemy( void )
{
	WP_SabersCheckLock2( NPC, NPC->enemy, (sabersLockMode_t)Q_irand(LOCK_KYLE_GRAB1,LOCK_KYLE_GRAB2) );//LOCK_KYLE_GRAB3
	TIMER_Set( NPC, "grabEnemyDebounce", NPC->client->ps.torsoAnimTimer + Q_irand( 4000, 20000 ) );
}

void Kyle_TryGrab( void )
{
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.torsoAnimTimer += 200;
	NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
	NPC->client->ps.saberMove = NPC->client->ps.saberMoveNext = LS_READY;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
	ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
	//WTF?
	NPC->client->ps.SaberDeactivate();
}

qboolean Kyle_CanDoGrab( void )
{
	if ( NPC->client->NPC_class == CLASS_KYLE && (NPC->spawnflags&1) )
	{//Boss Kyle
		if ( NPC->enemy && NPC->enemy->client )
		{//have a valid enemy
			if ( TIMER_Done( NPC, "grabEnemyDebounce" ) )
			{//okay to grab again
				if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
					&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//me and enemy are on ground
					if ( !PM_InOnGroundAnim( &NPC->enemy->client->ps ) )
					{
						if ( (NPC->client->ps.weaponTime <= 200||NPC->client->ps.torsoAnim==BOTH_KYLE_GRAB)
							&& !NPC->client->ps.saberInFlight )
						{
							if ( fabs(NPC->enemy->currentOrigin[2]-NPC->currentOrigin[2])<=8.0f )
							{//close to same level of ground
								if ( DistanceSquared( NPC->enemy->currentOrigin, NPC->currentOrigin ) <= 10000.0f )
								{
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
	}
	return qfalse;
}

static void Jedi_CombatDistance( int enemy_dist )
{//FIXME: for many of these checks, what we really want is horizontal distance to enemy
	if ( Jedi_CultistDestroyer( NPC ) )
	{//destroyer
		Jedi_Advance();
		//always run, regardless of what navigation tells us to do!
		NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		ucmd.buttons &= ~BUTTON_WALKING;
		return;
	}
	if ( enemy_dist < 128
		&& NPC->enemy
		&& NPC->enemy->client
		&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 ) )
	{//whoa, back off!!!
		if ( Q_irand( -3, NPCInfo->rank ) > RANK_CREWMAN )
		{
			Jedi_StartBackOff();
			return;
		}
	}
	if ( NPC->client->ps.forcePowersActive&(1<<FP_GRIP) &&
		NPC->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//when gripping, don't move
		return;
	}
	else if ( !TIMER_Done( NPC, "gripping" ) )
	{//stopped gripping, clear timers just in case
		TIMER_Set( NPC, "gripping", -level.time );
		TIMER_Set( NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}

	if ( NPC->client->ps.forcePowersActive&(1<<FP_DRAIN) &&
		NPC->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
	{//when draining, don't move
		return;
	}
	else if ( !TIMER_Done( NPC, "draining" ) )
	{//stopped draining, clear timers just in case
		TIMER_Set( NPC, "draining", -level.time );
		TIMER_Set( NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( !TIMER_Done( NPC, "flameTime" ) )
		{
			if ( enemy_dist > 50 )
			{
				Jedi_Advance();
			}
			else if ( enemy_dist <= 0 )
			{
				Jedi_Retreat();
			}
		}
		else if ( enemy_dist < 200 )
		{
			Jedi_Retreat();
		}
		else if ( enemy_dist > 1024 )
		{
			Jedi_Advance();
		}
	}
	else if ( NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW )
	{//don't move at all
		//FIXME: sabers need trails
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB )
	{//see if we grabbed enemy
		if ( NPC->client->ps.torsoAnimTimer <= 200 )
		{
			if ( Kyle_CanDoGrab()
				&& NPC_EnemyRangeFromBolt( NPC->handRBolt ) <= 72.0f )
			{//grab him!
				Kyle_GrabEnemy();
				return;
			}
			else
			{
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
				return;
			}
		}
		//else just sit here?
		return;
	}
	else if ( NPC->client->ps.saberInFlight &&
		!PM_SaberInBrokenParry( NPC->client->ps.saberMove )
		&& NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{//maintain distance
		if ( enemy_dist < NPC->client->ps.saberEntityDist )
		{
			Jedi_Retreat();
		}
		else if ( enemy_dist > NPC->client->ps.saberEntityDist && enemy_dist > 100 )
		{
			Jedi_Advance();
		}
		if ( NPC->client->ps.weapon == WP_SABER //using saber
			&& NPC->client->ps.saberEntityState == SES_LEAVING  //not returning yet
			&& NPC->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(NPC->client->ps.forcePowersActive&(1 << FP_SPEED))
			&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			ucmd.buttons |= BUTTON_ALT_ATTACK;
			//FIXME: time limit?
		}
	}
	else if ( !TIMER_Done( NPC, "taunting" ) )
	{
		if ( enemy_dist <= 64 )
		{//he's getting too close
			ucmd.buttons |= BUTTON_ATTACK;
			if ( !NPC->client->ps.saberInFlight )
			{
				NPC->client->ps.SaberActivate();
			}
			TIMER_Set( NPC, "taunting", -level.time );
		}
		else if ( NPC->client->ps.torsoAnim == BOTH_GESTURE1 && NPC->client->ps.torsoAnimTimer < 2000 )
		{//we're almost done with our special taunt
			//FIXME: this doesn't always work, for some reason
			if ( !NPC->client->ps.saberInFlight )
			{
				NPC->client->ps.SaberActivate();
			}
		}
	}
	else if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage
		if ( enemy_dist > 0 )
		{//get closer so we can hit!
			Jedi_Advance();
		}
		if ( enemy_dist > 128 )
		{//lost 'em
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if ( NPC->enemy->painDebounceTime + 2000 < level.time )
		{//the window of opportunity is gone
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set( NPC, "strafeLeft", -1 );
		TIMER_Set( NPC, "strafeRight", -1 );
	}
	else if ( NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time
		&& NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		if ( enemy_dist < 64 )
		{//FIXME: maybe just pick another enemy?
			Jedi_Retreat();
		}
	}
	else if ( NPC->enemy->s.weapon == WP_TURRET
		&& !Q_stricmp( "PAS", NPC->enemy->classname )
		&& NPC->enemy->s.apos.trType == TR_STATIONARY )
	{
		if ( enemy_dist > forcePushPullRadius[FORCE_LEVEL_1] - 16 )
		{
			Jedi_Advance();
		}
		int	testlevel;
		if ( NPC->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1 )
		{//
			testlevel = FORCE_LEVEL_1;
		}
		else
		{
			testlevel = NPC->client->ps.forcePowerLevel[FP_PUSH];
		}
		if ( enemy_dist < forcePushPullRadius[testlevel] - 16 )
		{//close enough to push
			if ( InFront( NPC->enemy->currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, 0.6f ) )
			{//knock it down
				WP_KnockdownTurret( NPC, NPC->enemy );
				//do the forcethrow call just for effect
				ForceThrow( NPC, qfalse );
			}
		}
	}
	else if ( enemy_dist <= 64
		&& ((NPCInfo->scriptFlags&SCF_DONT_FIRE)||(!Q_stricmp("Yoda",NPC->NPC_type)&&!Q_irand(0,10))) )
	{//can't use saber and they're in striking range
		if ( !Q_irand( 0, 5 ) && InFront( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.2f ) )
		{
			if ( ((NPCInfo->scriptFlags&SCF_DONT_FIRE)||NPC->max_health - NPC->health > NPC->max_health*0.25f)//lost over 1/4 of our health or not firing
				&& WP_ForcePowerUsable( NPC, FP_DRAIN, 20 )//know how to drain and have enough power
				&& !Q_irand( 0, 2 ) )
			{//drain
				TIMER_Set( NPC, "draining", 3000 );
				TIMER_Set( NPC, "attackDelay", 3000 );
				Jedi_Advance();
				return;
			}
			else
			{
				if ( Jedi_DecideKick() )
				{//let's try a kick
					if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
						|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
					{//kicked!
						TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
						return;
					}
				}
				ForceThrow( NPC, qfalse );
			}
		}
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 64
		&& NPC->max_health - NPC->health > NPC->max_health*0.25f//lost over 1/4 of our health
		&& NPC->client->ps.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
		&& WP_ForcePowerAvailable( NPC, FP_DRAIN, 20 )//have enough power
		&& !Q_irand( 0, 10 )
		&& InFront( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 0.2f ) )
	{
		TIMER_Set( NPC, "draining", 3000 );
		TIMER_Set( NPC, "attackDelay", 3000 );
		Jedi_Advance();
		return;
	}
	else if ( enemy_dist <= -16 )
	{//we're too damn close!
		if ( !Q_irand( 0, 30 )
			&& Kyle_CanDoGrab() )
		{
			Kyle_TryGrab();
			return;
		}
		else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER)
			&& !Q_irand( 0, 20 ) )
		{
			Tavion_StartScepterSlam();
			return;
		}
		if ( Jedi_DecideKick() )
		{//let's try a kick
			if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
				|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
			{//kicked!
				TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
				return;
			}
		}
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 0 )
	{//we're within striking range
		//if we are attacking, see if we should stop
		if ( NPCInfo->stats.aggression < 4 )
		{//back off and defend
			if ( !Q_irand( 0, 30 )
				&& Kyle_CanDoGrab() )
			{
				Kyle_TryGrab();
				return;
			}
			else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER)
				&& !Q_irand( 0, 20 ) )
			{
				Tavion_StartScepterSlam();
				return;
			}
			if ( Jedi_DecideKick() )
			{//let's try a kick
				if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
					|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
				{//kicked!
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					return;
				}
			}
			Jedi_Retreat();
		}
	}
	else if ( enemy_dist > 256 )
	{//we're way out of range
		qboolean usedForce = qfalse;
		if ( NPCInfo->stats.aggression < Q_irand( 0, 20 )
			&& NPC->health < NPC->max_health*0.75f
			&& !Q_irand( 0, 2 ) )
		{
			if ( NPC->enemy
				&& NPC->enemy->s.number < MAX_CLIENTS
				&& NPC->client->NPC_class!=CLASS_KYLE
				&& ((NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
					|| NPC->client->NPC_class==CLASS_SHADOWTROOPER)
				&& Q_irand(0, 3-g_spskill->integer) )
			{//hmm, bosses should do this less against the player
			}
			else if ( NPC->client->ps.saber[0].type == SABER_SITH_SWORD
				&& NPC->weaponModel[0] != -1 )
			{
				Tavion_SithSwordRecharge();
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.forcePowersKnown&(1<<FP_HEAL)) != 0
				&& (NPC->client->ps.forcePowersActive&(1<<FP_HEAL)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceHeal( NPC );
				usedForce = qtrue;
				//FIXME: check level of heal and know not to move or attack when healing
			}
			else if ( (NPC->client->ps.forcePowersKnown&(1<<FP_PROTECT)) != 0
				&& (NPC->client->ps.forcePowersActive&(1<<FP_PROTECT)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceProtect( NPC );
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.forcePowersKnown&(1<<FP_ABSORB)) != 0
				&& (NPC->client->ps.forcePowersActive&(1<<FP_ABSORB)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceAbsorb( NPC );
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.forcePowersKnown&(1<<FP_RAGE)) != 0
				&& (NPC->client->ps.forcePowersActive&(1<<FP_RAGE)) == 0
				&& Q_irand( 0, 1 ) )
			{
				Jedi_Rage();
				usedForce = qtrue;
			}
			//FIXME: what about things like mind tricks and force sight?
		}
		if ( enemy_dist > 384 )
		{//FIXME: check for enemy facing away and/or moving away
			if ( !Q_irand( 0, 10 ) && NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time )
			{
				if ( NPC_ClearLOS( NPC->enemy ) )
				{
					G_AddVoiceEvent( NPC, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 3000 );
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
		//Unless we're totally hiding, go after him
		if ( NPCInfo->stats.aggression > 0 )
		{//approach enemy
			if ( !usedForce )
			{
				if ( NPC->enemy
					&& NPC->enemy->client
					&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
						|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 ) )
				{//stay put!
				}
				else
				{
					Jedi_Advance();
				}
			}
		}
	}
	/*
	else if ( enemy_dist < 96 && NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//too close and in air, so retreat
		Jedi_Retreat();
	}
	*/
	//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
	else if ( enemy_dist > 50 )//FIXME: not hardcoded- base on our reach (modelScale?) and saberLengthMax
	{//we're out of striking range and we are allowed to attack
		//first, check some tactical force power decisions
		if ( NPC->enemy && NPC->enemy->client && (NPC->enemy->client->ps.eFlags&EF_FORCE_GRIPPED) )
		{//They're being gripped, rush them!
			if ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//they're on the ground, so advance
				if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
					{//far away or allowed to use saber
						Jedi_Advance();
					}
				}
			}
			if ( (NPCInfo->rank >= RANK_LT_JG||WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 ))
				&& !Q_irand( 0, 5 )
				&& !(NPC->client->ps.forcePowersActive&(1 << FP_SPEED))
				&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
			{//throw saber
				ucmd.buttons |= BUTTON_ALT_ATTACK;
			}
		}
		else if ( NPC->enemy && NPC->enemy->client && //valid enemy
			NPC->enemy->client->ps.saberInFlight && NPC->enemy->client->ps.saber[0].Active() && //enemy throwing saber
			!NPC->client->ps.weaponTime && //I'm not busy
			WP_ForcePowerAvailable( NPC, FP_GRIP, 0 ) && //I can use the power
			!Q_irand( 0, 10 ) && //don't do it all the time, averages to 1 check a second
			Q_irand( 0, 6 ) < g_spskill->integer && //more likely on harder diff
			Q_irand( RANK_CIVILIAN, RANK_CAPTAIN ) < NPCInfo->rank //more likely against harder enemies
			&& InFOV( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 30 ) )
		{//They're throwing their saber, grip them!
			//taunt
			if ( TIMER_Done( NPC, "chatter" ) && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->blockedSpeechDebounceTime < level.time )
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
				{
					TIMER_Set( NPC, "chatter", 6000 );
				}
				else
				{
					TIMER_Set( NPC, "chatter", 3000 );
				}
			}

			//grip
			TIMER_Set( NPC, "gripping", 3000 );
			TIMER_Set( NPC, "attackDelay", 3000 );
		}
		else
		{
			if ( NPC->enemy && NPC->enemy->client && (NPC->enemy->client->ps.forcePowersActive&(1<<FP_GRIP)) )
			{//They're choking someone, probably an ally, run at them and do some sort of attack
				if ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//they're on the ground, so advance
					if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
					{//not parrying
						if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
						{//far away or allowed to use saber
							Jedi_Advance();
						}
					}
				}
			}
			if ( NPC->client->NPC_class == CLASS_KYLE
				&& (NPC->spawnflags&1)
				&& (NPC->enemy&&NPC->enemy->client&&!NPC->enemy->client->ps.saberInFlight)
				&& TIMER_Done( NPC, "kyleTakesSaber" )
				&& !Q_irand( 0, 20 ) )
			{
				ForceThrow( NPC, qtrue );
			}
			else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER)
				&& !Q_irand( 0, 20 ) )
			{
				Tavion_StartScepterBeam();
				return;
			}
			else
			{
				int chanceScale = 0;
				if ( NPC->client->NPC_class == CLASS_KYLE && (NPC->spawnflags&1) )
				{
					chanceScale = 4;
				}
				else if ( NPC->enemy
					&& NPC->enemy->s.number < MAX_CLIENTS
					&& ((NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
						|| NPC->client->NPC_class==CLASS_SHADOWTROOPER) )
				{//hmm, bosses do this less against player
					chanceScale = 8 - g_spskill->integer*2;
				}
				else if ( NPC->client->NPC_class == CLASS_DESANN
					|| !Q_stricmp("Yoda",NPC->NPC_type) )
					//|| (NPC->client->NPC_class == CLASS_CULTIST && NPC->client->ps.weapon == WP_NONE) )//force-only cultists use force a lot
				{
					chanceScale = 1;
				}
				else if ( NPCInfo->rank == RANK_ENSIGN )
				{
					chanceScale = 2;
				}
				else if ( NPCInfo->rank >= RANK_LT_JG )
				{
					chanceScale = 5;
				}
				if ( chanceScale
					&& (enemy_dist > Q_irand( 100, 200 ) || (NPCInfo->scriptFlags&SCF_DONT_FIRE) || (!Q_stricmp("Yoda",NPC->NPC_type)&&!Q_irand(0,3)) )
					&& enemy_dist < 500
					&& (Q_irand( 0, chanceScale*10 )<5 || (NPC->enemy->client && NPC->enemy->client->ps.weapon != WP_SABER && !Q_irand( 0, chanceScale ) ) ) )
				{//else, randomly try some kind of attack every now and then
					//FIXME: Cultist fencers don't have any of these fancy powers
					//			the only thing they might be able to do is throw their saber
					if ( (NPCInfo->rank == RANK_ENSIGN //old reborn crap
							|| NPCInfo->rank > RANK_LT_JG //old reborn crap
						/*
							|| WP_ForcePowerUsable( NPC, FP_PULL, 0 )
							|| WP_ForcePowerUsable( NPC, FP_LIGHTNING, 0 )
							|| WP_ForcePowerUsable( NPC, FP_DRAIN, 0 )
							|| WP_ForcePowerUsable( NPC, FP_GRIP, 0 )
							|| WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 )
						*/
						)
						&& (!Q_irand( 0, 1 ) || NPC->s.weapon != WP_SABER) )
					{
						if ( WP_ForcePowerUsable( NPC, FP_PULL, 0 ) && !Q_irand( 0, 2 ) )
						{
							//force pull the guy to me!
							//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
							ForceThrow( NPC, qtrue );
							//maybe strafe too?
							TIMER_Set( NPC, "duck", enemy_dist*3 );
							if ( Q_irand( 0, 1 ) )
							{
								ucmd.buttons |= BUTTON_ATTACK;
							}
						}
						else if ( WP_ForcePowerUsable( NPC, FP_LIGHTNING, 0 )
							&& (((NPCInfo->scriptFlags&SCF_DONT_FIRE)&&Q_stricmp("cultist_lightning",NPC->NPC_type)) || Q_irand( 0, 1 )) )
						{
							ForceLightning( NPC );
							if ( NPC->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
							{
								NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_spskill->integer*500) );
								TIMER_Set( NPC, "holdLightning", NPC->client->ps.weaponTime );
							}
							TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
						}
						else if ( NPC->health < NPC->max_health * 0.75f
							&& Q_irand( FORCE_LEVEL_0, NPC->client->ps.forcePowerLevel[FP_DRAIN] ) > FORCE_LEVEL_1
							&& WP_ForcePowerUsable( NPC, FP_DRAIN, 0 )
							&& (((NPCInfo->scriptFlags&SCF_DONT_FIRE)&&Q_stricmp("cultist_drain",NPC->NPC_type)) || Q_irand( 0, 1 )) )
						{
							ForceDrain2( NPC );
							NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_spskill->integer*500) );
							TIMER_Set( NPC, "draining", NPC->client->ps.weaponTime );
							TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
						}
						else if ( WP_ForcePowerUsable( NPC, FP_GRIP, 0 )
							&& NPC->enemy && InFOV( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 30 ) )
						{
							//taunt
							if ( TIMER_Done( NPC, "chatter" ) && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->blockedSpeechDebounceTime < level.time )
							{
								G_AddVoiceEvent( NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
								jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
								if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
								{
									TIMER_Set( NPC, "chatter", 6000 );
								}
								else
								{
									TIMER_Set( NPC, "chatter", 3000 );
								}
							}

							//grip
							TIMER_Set( NPC, "gripping", 3000 );
							TIMER_Set( NPC, "attackDelay", 3000 );
						}
						else
						{
							if ( WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 )
								&& !(NPC->client->ps.forcePowersActive&(1 << FP_SPEED))
								&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
							{//throw saber
								ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
						}
					}
					else
					{
						if ( (NPCInfo->rank >= RANK_LT_JG||WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 ))
							&& !(NPC->client->ps.forcePowersActive&(1 << FP_SPEED))
							&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
						{//throw saber
							ucmd.buttons |= BUTTON_ALT_ATTACK;
						}
					}
				}
				//see if we should advance now
				else if ( NPCInfo->stats.aggression > 5 )
				{//approach enemy
					if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
					{//not parrying
						if ( !NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
						{//they're on the ground, so advance
							if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
							{//far away or allowed to use saber
								Jedi_Advance();
							}
						}
					}
				}
				else
				{//maintain this distance?
					//walk?
				}
			}
		}
	}
	else
	{//we're not close enough to attack, but not far enough away to be safe
		if ( !Q_irand( 0, 30 )
			&& Kyle_CanDoGrab() )
		{
			Kyle_TryGrab();
			return;
		}
		if ( NPCInfo->stats.aggression < 4 )
		{//back off and defend
			if ( Jedi_DecideKick() )
			{//let's try a kick
				if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
					|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
				{//kicked!
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					return;
				}
			}
			Jedi_Retreat();
		}
		else if ( NPCInfo->stats.aggression > 5 )
		{//try to get closer
			if ( enemy_dist > 0 && !(NPCInfo->scriptFlags&SCF_DONT_FIRE))
			{//we're allowed to use our lightsaber, get closer
				if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( !NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//they're on the ground, so advance
						Jedi_Advance();
					}
				}
			}
		}
		else
		{//agression is 4 or 5... somewhere in the middle
			//what do we do here?  Nothing?
			//Move forward and back?
		}
	}
	//if really really mad, rage!
	if ( NPCInfo->stats.aggression > Q_irand( 5, 15 )
		&& NPC->health < NPC->max_health*0.75f
		&& !Q_irand( 0, 2 ) )
	{
		if ( (NPC->client->ps.forcePowersKnown&(1<<FP_RAGE)) != 0
			&& (NPC->client->ps.forcePowersActive&(1<<FP_RAGE)) == 0 )
		{
			Jedi_Rage();
		}
	}
}

static qboolean Jedi_Strafe( int strafeTimeMin, int strafeTimeMax, int nextStrafeTimeMin, int nextStrafeTimeMax, qboolean walking )
{
	if ( Jedi_CultistDestroyer( NPC ) )
	{//never strafe
		return qfalse;
	}
	if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON && NPC->enemy && NPC->enemy->painDebounceTime > level.time )
	{//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if ( TIMER_Done( NPC, "strafeLeft" ) && TIMER_Done( NPC, "strafeRight" ) )
	{
		qboolean strafed = qfalse;
		//TODO: make left/right choice a tactical decision rather than random:
		//		try to keep own back away from walls and ledges,
		//		try to keep enemy's back to a ledge or wall
		//		Maybe try to strafe toward designer-placed "safe spots" or "goals"?
		int	strafeTime = Q_irand( strafeTimeMin, strafeTimeMax );

		if ( Q_irand( 0, 1 ) )
		{
			if ( NPC_MoveDirClear( ucmd.forwardmove, -127, qfalse ) )
			{
				TIMER_Set( NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( ucmd.forwardmove, 127, qfalse ) )
			{
				TIMER_Set( NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
		}
		else
		{
			if ( NPC_MoveDirClear( ucmd.forwardmove, 127, qfalse  ) )
			{
				TIMER_Set( NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( ucmd.forwardmove, -127, qfalse  ) )
			{
				TIMER_Set( NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
		}

		if ( strafed )
		{
			TIMER_Set( NPC, "noStrafe", strafeTime + Q_irand( nextStrafeTimeMin, nextStrafeTimeMax ) );
			if ( walking )
			{//should be a slow strafe
				TIMER_Set( NPC, "walking", strafeTime );
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
static void Jedi_FaceEntity( gentity_t *self, gentity_t *other, qboolean doPitch )
{
	vec3_t		entPos;
	vec3_t		muzzle;

	//Get the positions
	CalcEntitySpot( other, SPOT_ORIGIN, entPos );

	//Get the positions
	CalcEntitySpot( self, SPOT_HEAD_LEAN, muzzle );//SPOT_HEAD

	//Find the desired angles
	vec3_t	angles;

	GetAnglesForDirection( muzzle, entPos, angles );

	self->NPC->desiredYaw		= AngleNormalize360( angles[YAW] );
	if ( doPitch )
	{
		self->NPC->desiredPitch	= AngleNormalize360( angles[PITCH] );
	}
}
*/

/*
qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )

Jedi will play a dodge anim, blur, and make the force speed noise.

Right now used to dodge instant-hit weapons.

FIXME: possibly call this for saber melee evasion and/or missile evasion?
FIXME: possibly let player do this too?
*/
qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )
{
	int	dodgeAnim = -1;

	if ( !self || !self->client || self->health <= 0 )
	{
		return qfalse;
	}

	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//can't dodge in mid-air
		return qfalse;
	}

	if ( self->client->ps.pm_time && (self->client->ps.pm_flags&PMF_TIME_KNOCKBACK) )
	{//in some effect that stops me from moving on my own
		return qfalse;
	}

	if ( self->enemy == shooter )
	{//FIXME: make it so that we are better able to dodge shots from my current enemy
	}
	if ( self->s.number )
	{//if an NPC, check game skill setting
		/*
		if ( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
		{//those NPCs are "bosses" and always succeed
			if ( Q_irand( 0, 2 ) > g_spskill->integer )
			{//more of a chance of failing the dodge on lower difficulty
				return qfalse;
			}
			//FIXME: check my overall skill (rank) to determine if I should be able to dodge it?
			//check force speed power level to determine if I should be able to dodge it
			if ( Q_irand( 0, 3 ) > self->client->ps.forcePowerLevel[FP_SPEED] )
			{//more likely to fail on lower force speed level, but NPCs are generally better at it than the player
				return qfalse;
			}
		}
		*/
	}
	else
	{//the player
		if ( !(self->client->ps.forcePowersActive&(1<<FP_SPEED)) )
		{//not already in speed
			if ( !WP_ForcePowerUsable( self, FP_SPEED, 0 ) )
			{//make sure we have it and have enough force power
				return qfalse;
			}
		}
		//check force speed power level to determine if I should be able to dodge it
		if ( Q_irand( 1, 10 ) > self->client->ps.forcePowerLevel[FP_SPEED] )
		{//more likely to fail on lower force speed level
			return qfalse;
		}
	}

	if ( hitLoc == HL_NONE )
	{
		if ( tr )
		{
			for ( int z = 0; z < MAX_G2_COLLISIONS; z++ )
			{
				if ( tr->G2CollisionMap[z].mEntityNum == -1 )
				{//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
					continue;//break;//
				}

				CCollisionRecord &coll	= tr->G2CollisionMap[z];
				G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex ), &hitLoc, coll.mCollisionPosition, NULL, NULL, MOD_UNKNOWN );
				//only want the first
				break;
			}
		}
	}

	switch( hitLoc )
	{
	case HL_NONE:
		return qfalse;
		break;

	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
	case HL_WAIST:
		if ( !self->s.number )
		{//don't force the player to jump
			return qfalse;
		}
		else
		{
			if ( !self->enemy && G_ValidEnemy(self,shooter))
			{
				G_SetEnemy( self, shooter );
			}
			if ( self->NPC
				&& ((self->NPC->scriptFlags&SCF_NO_ACROBATICS) || PM_InKnockDown( &self->client->ps ) ) )
			{
				return qfalse;
			}
			if ( self->client
				&& (self->client->ps.forceRageRecoveryTime > level.time || (self->client->ps.forcePowersActive&(1<<FP_RAGE))) )
			{//no fancy dodges when raging or recovering
				return qfalse;
			}
			if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand(0,1))
			{
				return qfalse; // half the time he dodges
			}


			if ( self->client->NPC_class == CLASS_BOBAFETT
				|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER)
				|| self->client->NPC_class == CLASS_ROCKETTROOPER )
			{
				self->client->ps.forceJumpCharge = 280;//FIXME: calc this intelligently?
			}
			else
			{
				self->client->ps.forceJumpCharge = 320;//FIXME: calc this intelligently?
				WP_ForcePowerStop( self, FP_GRIP );
			}
			return qtrue;
		}
		break;

	case HL_BACK_RT:
		dodgeAnim = BOTH_DODGE_FL;
		break;
	case HL_CHEST_RT:
		dodgeAnim = BOTH_DODGE_BL;
		break;
	case HL_BACK_LT:
		dodgeAnim = BOTH_DODGE_FR;
		break;
	case HL_CHEST_LT:
		dodgeAnim = BOTH_DODGE_BR;
		break;
	case HL_BACK:
	case HL_CHEST:
		dodgeAnim = Q_irand( BOTH_DODGE_FL, BOTH_DODGE_R );
		break;
	case HL_ARM_RT:
	case HL_HAND_RT:
		dodgeAnim = BOTH_DODGE_L;
		break;
	case HL_ARM_LT:
	case HL_HAND_LT:
		dodgeAnim = BOTH_DODGE_R;
		break;
	case HL_HEAD:
		dodgeAnim = Q_irand( BOTH_DODGE_FL, BOTH_DODGE_BR );
		break;
	}

	if ( dodgeAnim != -1 )
	{
		int extraHoldTime = 0;//Q_irand( 5, 40 ) * 50;
		/*
		int type = SETANIM_TORSO;
		if ( VectorCompare( self->client->ps.velocity, vec3_origin ) )
		{//not moving
			type = SETANIM_BOTH;
		}
		*/
		if ( self->s.number < MAX_CLIENTS )
		{//player
			if ( (self->client->ps.forcePowersActive&(1<<FP_SPEED)) )
			{//in speed
				if ( PM_DodgeAnim( self->client->ps.torsoAnim )
					&& !PM_DodgeHoldAnim( self->client->ps.torsoAnim ) )
				{//already in a dodge
					//use the hold pose, don't start it all over again
					dodgeAnim = BOTH_DODGE_HOLD_FL+(dodgeAnim-BOTH_DODGE_FL);
					extraHoldTime = 200;
				}
			}
		}

		//set the dodge anim we chose
		NPC_SetAnim( self, SETANIM_BOTH, dodgeAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );//type
		if ( extraHoldTime && self->client->ps.torsoAnimTimer < extraHoldTime )
		{
			self->client->ps.torsoAnimTimer += extraHoldTime;
		}
		//if ( type == SETANIM_BOTH )
		{
			self->client->ps.legsAnimTimer = self->client->ps.torsoAnimTimer;
		}

		if ( self->s.number )
		{//NPC
			//maybe force them to stop moving in this case?
			self->client->ps.pm_time = self->client->ps.torsoAnimTimer + Q_irand( 100, 1000 );
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//do force speed effect
			self->client->ps.forcePowersActive |= (1 << FP_SPEED);
			self->client->ps.forcePowerDuration[FP_SPEED] = level.time + self->client->ps.torsoAnimTimer;
			//sound
			G_Sound( self, G_SoundIndex( "sound/weapons/force/speed.wav" ) );
		}
		else
		{//player
			ForceSpeed( self, 500 );
		}

		WP_ForcePowerStop( self, FP_GRIP );
		if ( !self->enemy && G_ValidEnemy( self, shooter) )
		{
			G_SetEnemy( self, shooter );
			if ( self->s.number )
			{
				Jedi_Aggression( self, 10 );
			}
		}
		return qtrue;
	}
	return qfalse;
}

evasionType_t Jedi_CheckFlipEvasions( gentity_t *self, float rightdot, float zdiff )
{
	if ( self->NPC && (self->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return EVASION_NONE;
	}
	if ( self->client )
	{
		if ( self->client->NPC_class == CLASS_BOBAFETT )
		{//boba can't flip
			return EVASION_NONE;
		}
		if ( self->client->ps.forceRageRecoveryTime > level.time
			|| (self->client->ps.forcePowersActive&(1<<FP_RAGE)) )
		{//no fancy dodges when raging
			return EVASION_NONE;
		}
	}

	//Check for:
	//ARIALS/CARTWHEELS
	//WALL-RUNS
	//WALL-FLIPS
	//FIXME: if facing a wall, do forward wall-walk-backflip
	//FIXME: add new JKA ones

//FIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXME
//
//Make these check for do not enter and ledges!!!
//
//FIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXMEFIXME

	if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
	{//already running on a wall
		vec3_t right, fwdAngles = {0, self->client->ps.viewangles[YAW], 0};
		int		anim = -1;

		AngleVectors( fwdAngles, NULL, right, NULL );

		float animLength = PM_AnimLength( self->client->clientInfo.animFileIndex, (animNumber_t)self->client->ps.legsAnim );
		if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0 )
		{//I'm running on a wall to my left and the attack is on the left
			if ( animLength - self->client->ps.legsAnimTimer > 400
				&& self->client->ps.legsAnimTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if ( self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0 )
		{//I'm running on a wall to my right and the attack is on the right
			if ( animLength - self->client->ps.legsAnimTimer > 400
				&& self->client->ps.legsAnimTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if ( anim != -1 )
		{//flip off the wall!
			//FIXME: check the direction we will flip towards for do-not-enter/walls/drops?
			//NOTE: we presume there is still a wall there!
			if ( anim == BOTH_WALL_RUN_LEFT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
			}
			else if ( anim == BOTH_WALL_RUN_RIGHT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
			}
			int parts = SETANIM_LEGS;
			if ( !self->client->ps.weaponTime )
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
			G_AddEvent( self, EV_JUMP, 0 );
			return EVASION_OTHER;
		}
	}
	else if ( self->client->NPC_class != CLASS_DESANN //desann doesn't do these kind of frilly acrobatics
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT)
		&& Q_irand( 0, 1 )
		&& !PM_InRoll( &self->client->ps )
		&& !PM_InKnockDown( &self->client->ps )
		&& !PM_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		vec3_t fwd, right, traceto, mins = {self->mins[0],self->mins[1],self->mins[2]+STEPSIZE}, maxs = {self->maxs[0],self->maxs[1],24}, fwdAngles = {0, self->client->ps.viewangles[YAW], 0};
		trace_t	trace;

		AngleVectors( fwdAngles, fwd, right, NULL );

		int parts = SETANIM_BOTH, anim;
		float	speed, checkDist;
		qboolean allowCartWheels = qtrue;

		if ( self->client->ps.weapon == WP_SABER )
		{
			if ( (self->client->ps.saber[0].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			else if ( self->client->ps.dualSabers
				&& (self->client->ps.saber[1].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
		}

		if ( PM_SaberInAttack( self->client->ps.saberMove )
			|| PM_SaberInStart( self->client->ps.saberMove ) )
		{
			parts = SETANIM_LEGS;
		}
		if ( rightdot >= 0 )
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_LEFT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_LEFT;
			}
			checkDist = -128;
			speed = -200;
		}
		else
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_RIGHT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_RIGHT;
			}
			checkDist = 128;
			speed = 200;
		}
		//trace in the dir that we want to go
		VectorMA( self->currentOrigin, checkDist, right, traceto );
		gi.trace( &trace, self->currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
		if ( trace.fraction >= 1.0f && allowCartWheels )
		{//it's clear, let's do it
			//FIXME: check for drops?
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.weaponTime = self->client->ps.legsAnimTimer;//don't attack again until this anim is done
			vec3_t fwdAngles, jumpRt;
			VectorCopy( self->client->ps.viewangles, fwdAngles );
			fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
			//do the flip
			AngleVectors( fwdAngles, NULL, jumpRt, NULL );
			VectorScale( jumpRt, speed, self->client->ps.velocity );
			self->client->ps.forceJumpCharge = 0;//so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.forceJumpZStart = self->currentOrigin[2];//so we don't take damage if we land at same height
			self->client->ps.pm_flags |= PMF_JUMPING;
			if ( self->client->NPC_class == CLASS_BOBAFETT
				 || (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) )
			{
				G_AddEvent( self, EV_JUMP, 0 );
			}
			else
			{
				G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
			}
			//ucmd.upmove = 0;
			return EVASION_CARTWHEEL;
		}
		else if ( !(trace.contents&CONTENTS_BOTCLIP) )
		{//hit a wall, not a do-not-enter brush
			//FIXME: before we check any of these jump-type evasions, we should check for headroom, right?
			//Okay, see if we can flip *off* the wall and go the other way
			vec3_t	idealNormal;
			VectorSubtract( self->currentOrigin, traceto, idealNormal );
			VectorNormalize( idealNormal );
			gentity_t *traceEnt = &g_entities[trace.entityNum];
			if ( (trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid!=SOLID_BMODEL) || DotProduct( trace.plane.normal, idealNormal ) > 0.7f )
			{//it's a ent of some sort or it's a wall roughly facing us
				float bestCheckDist = 0;
				//hmm, see if we're moving forward
				if ( DotProduct( self->client->ps.velocity, fwd ) < 200 )
				{//not running forward very fast
					//check to see if it's okay to move the other way
					if ( (trace.fraction*checkDist) <= 32 )
					{//wall on that side is close enough to wall-flip off of or wall-run on
						bestCheckDist = checkDist;
						checkDist *= -1.0f;
						VectorMA( self->currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						gi.trace( &trace, self->currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
						if ( trace.fraction >= 1.0f )
						{//it's clear, let's do it
							qboolean allowWallFlips = qtrue;
							if ( self->client->ps.weapon == WP_SABER )
							{
								if ( (self->client->ps.saber[0].saberFlags&SFL_NO_WALL_FLIPS) )
								{
									allowWallFlips = qfalse;
								}
								else if ( self->client->ps.dualSabers
									&& (self->client->ps.saber[1].saberFlags&SFL_NO_WALL_FLIPS) )
								{
									allowWallFlips = qfalse;
								}
							}
							if ( allowWallFlips )
							{//okay to do wall-flips with this saber
								//FIXME: check for drops?
								//turn the cartwheel into a wallflip in the other dir
								if ( rightdot > 0 )
								{
									anim = BOTH_WALL_FLIP_LEFT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
								}
								else
								{
									anim = BOTH_WALL_FLIP_RIGHT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
								}
								self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
								//animate me
								int parts = SETANIM_LEGS;
								if ( !self->client->ps.weaponTime )
								{
									parts = SETANIM_BOTH;
								}
								NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.forceJumpZStart = self->currentOrigin[2];//so we don't take damage if we land at same height
								self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
								if ( self->client->NPC_class == CLASS_BOBAFETT
									|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
								{
									G_AddEvent( self, EV_JUMP, 0 );
								}
								else
								{
									G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}
								return EVASION_OTHER;
							}
						}
						else
						{//boxed in on both sides
							if ( DotProduct( self->client->ps.velocity, fwd ) < 0 )
							{//moving backwards
								return EVASION_NONE;
							}
							if ( (trace.fraction*checkDist) <= 32 && (trace.fraction*checkDist) < bestCheckDist )
							{
								bestCheckDist = checkDist;
							}
						}
					}
					else
					{//too far from that wall to flip or run off it, check other side
						checkDist *= -1.0f;
						VectorMA( self->currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						gi.trace( &trace, self->currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
						if ( (trace.fraction*checkDist) <= 32 )
						{//wall on this side is close enough
							bestCheckDist = checkDist;
						}
						else
						{//neither side has a wall within 32
							return EVASION_NONE;
						}
					}
				}
				//Try wall run?
				if ( bestCheckDist )
				{//one of the walls was close enough to wall-run on
					qboolean allowWallRuns = qtrue;
					if ( self->client->ps.weapon == WP_SABER )
					{
						if ( (self->client->ps.saber[0].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
						else if ( self->client->ps.dualSabers
							&& (self->client->ps.saber[1].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
					}
					if ( allowWallRuns )
					{//okay to do wallruns with this saber
						//FIXME: check for long enough wall and a drop at the end?
						if ( bestCheckDist > 0 )
						{//it was to the right
							anim = BOTH_WALL_RUN_RIGHT;
						}
						else
						{//it was to the left
							anim = BOTH_WALL_RUN_LEFT;
						}
						self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
						//animate me
						int parts = SETANIM_LEGS;
						if ( !self->client->ps.weaponTime )
						{
							parts = SETANIM_BOTH;
						}
						NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						self->client->ps.forceJumpZStart = self->currentOrigin[2];//so we don't take damage if we land at same height
						self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
						if ( self->client->NPC_class == CLASS_BOBAFETT
							|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
						{
							G_AddEvent( self, EV_JUMP, 0 );
						}
						else
						{
							G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
						}
						return EVASION_OTHER;
					}
				}
				//else check for wall in front, do backflip off wall
			}
		}
	}
	return EVASION_NONE;
}

int Jedi_ReCalcParryTime( gentity_t *self, evasionType_t evasionType )
{
	if ( !self->client )
	{
		return 0;
	}
	if ( !self->s.number )
	{//player
		return parryDebounce[self->client->ps.forcePowerLevel[FP_SABER_DEFENSE]];
	}
	else if ( self->NPC )
	{
		/*
		if ( !g_saberRealisticCombat->integer
			&& ( g_spskill->integer == 2 || (g_spskill->integer == 1 && (self->client->NPC_class == CLASS_TAVION||self->client->NPC_class == CLASS_ALORA) ) ) )
		{
			if ( (self->client->NPC_class == CLASS_TAVION||self->client->NPC_class == CLASS_ALORA) )
			{
				return 0;
			}
			else
			{
				return Q_irand( 0, 150 );
			}
		}
		else
		*/
		{
			int	baseTime;
			if ( evasionType == EVASION_DODGE )
			{
				baseTime = self->client->ps.torsoAnimTimer;
			}
			else if ( evasionType == EVASION_CARTWHEEL )
			{
				baseTime = self->client->ps.torsoAnimTimer;
			}
			else if ( self->client->ps.saberInFlight )
			{
				baseTime = Q_irand( 1, 3 ) * 50;
			}
			else
			{
				/*
				baseTime = 1000;

				switch ( g_spskill->integer )
				{
				case 0:
					baseTime = 1500;
					break;
				case 1:
					baseTime = 1000;
					break;
				case 2:
				default:
					baseTime = 500;
					break;
				}
				*/
				if ( 1 )//g_saberRealisticCombat->integer )
				{
					baseTime = 500;

					switch ( g_spskill->integer )
					{
					case 0:
						baseTime = 400;//was 500
						break;
					case 1:
						baseTime = 200;//was 300
						break;
					case 2:
					default:
						baseTime = 100;
						break;
					}
				}
				else
				{
					baseTime = 150;//500;

					switch ( g_spskill->integer )
					{
					case 0:
						baseTime = 200;//500;
						break;
					case 1:
						baseTime = 100;//300;
						break;
					case 2:
					default:
						baseTime = 50;//100;
						break;
					}
				}

				if ( self->client->NPC_class == CLASS_ALORA
					|| self->client->NPC_class == CLASS_SHADOWTROOPER
					|| self->client->NPC_class == CLASS_TAVION )
				{//Tavion & Alora are faster
					baseTime = ceil(baseTime/2.0f);
				}
				else if ( self->NPC->rank >= RANK_LT_JG )
				{//fencers, bosses, shadowtroopers, luke, desann, et al use the norm
					if ( !Q_irand( 0, 2 ) )
					{//with the occasional fast parry
						baseTime = ceil(baseTime/2.0f);
					}
				}
				else if ( self->NPC->rank == RANK_CIVILIAN )
				{//grunts are slowest
					baseTime = baseTime*Q_irand(1,3);
				}
				else if ( self->NPC->rank == RANK_CREWMAN )
				{//acrobats aren't so bad
					if ( evasionType == EVASION_PARRY
						|| evasionType == EVASION_DUCK_PARRY
						|| evasionType == EVASION_JUMP_PARRY )
					{//slower with parries
						baseTime = baseTime*Q_irand(1,2);
					}
					else
					{//faster with acrobatics
						//baseTime = baseTime;
					}
				}
				else
				{//force users are kinda slow
					baseTime = baseTime*Q_irand(1,2);
				}
				if ( evasionType == EVASION_DUCK || evasionType == EVASION_DUCK_PARRY )
				{
					baseTime += 250;//300;//100;
				}
				else if ( evasionType == EVASION_JUMP || evasionType == EVASION_JUMP_PARRY )
				{
					baseTime += 400;//500;//50;
				}
				else if ( evasionType == EVASION_OTHER )
				{
					baseTime += 50;//100;
				}
				else if ( evasionType == EVASION_FJUMP )
				{
					baseTime += 300;//400;//100;
				}
			}

			return baseTime;
		}
	}
	return 0;
}

qboolean Jedi_QuickReactions( gentity_t *self )
{
	if ( ( self->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER ) ||
		self->client->NPC_class == CLASS_SHADOWTROOPER || self->client->NPC_class == CLASS_ALORA || self->client->NPC_class == CLASS_TAVION ||
		(self->client->ps.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_1&&g_spskill->integer>1) ||
		(self->client->ps.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_2&&g_spskill->integer>0) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_SaberBusy( gentity_t *self )
{
	if ( self->client->ps.torsoAnimTimer > 300
	&& ( (PM_SaberInAttack( self->client->ps.saberMove )&&self->client->ps.saberAnimLevel==SS_STRONG)
		|| PM_SpinningSaberAnim( self->client->ps.torsoAnim )
		|| PM_SaberInSpecialAttack( self->client->ps.torsoAnim )
		//|| PM_SaberInBounce( self->client->ps.saberMove )
		|| PM_SaberInBrokenParry( self->client->ps.saberMove )
		//|| PM_SaberInDeflect( self->client->ps.saberMove )
		|| PM_FlippingAnim( self->client->ps.torsoAnim )
		|| PM_RollingAnim( self->client->ps.torsoAnim ) ) )
	{//my saber is not in a parrying position
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_InNoAIAnim( gentity_t *self )
{
	if ( !self || !self->client )
	{//wtf???
		return qtrue;
	}

	if ( NPCInfo->rank >= RANK_COMMANDER )
	{//boss-level guys can multitask, the rest need to chill out during special moves
		return qfalse;
	}

	if ( PM_KickingAnim( NPC->client->ps.legsAnim )
		||PM_StabDownAnim( NPC->client->ps.legsAnim )
		||PM_InAirKickingAnim( NPC->client->ps.legsAnim )
		||PM_InRollIgnoreTimer( &NPC->client->ps )
		||PM_SaberInKata((saberMoveName_t)NPC->client->ps.saberMove)
		||PM_SuperBreakWinAnim( NPC->client->ps.torsoAnim )
		||PM_SuperBreakLoseAnim( NPC->client->ps.torsoAnim ) )
	{
		return qtrue;
	}

	switch ( self->client->ps.legsAnim )
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FLIP_F:
	case BOTH_FLIP_B:
	case BOTH_FLIP_L:
	case BOTH_FLIP_R:
	case BOTH_DODGE_FL:
	case BOTH_DODGE_FR:
	case BOTH_DODGE_BL:
	case BOTH_DODGE_BR:
	case BOTH_DODGE_L:
	case BOTH_DODGE_R:
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_ROLL_STAB:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
		return qtrue;
		break;
	}
	return qfalse;
}

void Jedi_CheckJumpEvasionSafety( gentity_t *self, usercmd_t *cmd, evasionType_t evasionType )
{
	if ( evasionType != EVASION_OTHER//not a FlipEvasion, which does it's own safety checks
		&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{//on terra firma right now
		if ( NPC->client->ps.velocity[2] > 0
			|| NPC->client->ps.forceJumpCharge
			|| cmd->upmove > 0 )
		{//going to jump
			if ( !NAV_MoveDirSafe( NPC, cmd, NPC->client->ps.speed*10.0f ) )
			{//we can't jump in the dir we're pushing in
				//cancel the evasion
				NPC->client->ps.velocity[2] = NPC->client->ps.forceJumpCharge = 0;
				cmd->upmove = 0;
				if ( d_JediAI->integer )
				{
					Com_Printf( S_COLOR_RED"jump not safe, cancelling!" );
				}
			}
			else if ( NPC->client->ps.velocity[0] || NPC->client->ps.velocity[1] )
			{//sliding
				vec3_t jumpDir;
				float jumpDist = VectorNormalize2( NPC->client->ps.velocity, jumpDir );
				if ( !NAV_DirSafe( NPC, jumpDir, jumpDist ) )
				{//this jump combined with our momentum would send us into a do not enter brush, so cancel it
					//cancel the evasion
					NPC->client->ps.velocity[2] = NPC->client->ps.forceJumpCharge = 0;
					cmd->upmove = 0;
					if ( d_JediAI->integer )
					{
						Com_Printf( S_COLOR_RED"jump not safe, cancelling!\n" );
					}
				}
			}
			if ( d_JediAI->integer )
			{
				Com_Printf( S_COLOR_GREEN"jump checked, is safe\n" );
			}
		}
	}
}
/*
-------------------------
Jedi_SaberBlock

Pick proper block anim

FIXME: Based on difficulty level/enemy saber combat skill, make this decision-making more/less effective

NOTE: always blocking projectiles in this func!

-------------------------
*/
extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist = 0.0f )
{
	vec3_t hitloc, hitdir, diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;
	int	  duckChance = 0;
	int	  dodgeAnim = -1;
	qboolean	saberBusy = qfalse;
	evasionType_t	evasionType = EVASION_NONE;

	if ( !self || !self->client )
	{
		return EVASION_NONE;
	}

	if ( PM_LockedAnim( self->client->ps.torsoAnim )
		&& self->client->ps.torsoAnimTimer )
	{//Never interrupt these...
		return EVASION_NONE;
	}
	if ( PM_InSpecialJump( self->client->ps.legsAnim )
		&& PM_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		return EVASION_NONE;
	}

	if ( Jedi_InNoAIAnim( self ) )
	{
		return EVASION_NONE;
	}


	//FIXME: if we don't have our saber in hand, pick the force throw option or a jump or strafe!
	//FIXME: reborn don't block enough anymore
	if ( !incoming )
	{
		VectorCopy( pHitloc, hitloc );
		VectorCopy( phitDir, hitdir );
		//FIXME: maybe base this on rank some?  And/or g_spskill?
		if ( self->client->ps.saberInFlight )
		{//DOH!  do non-saber evasion!
			saberBusy = qtrue;
		}
		/*
		else if ( Jedi_QuickReactions( self ) )
		{//jedi trainer and tavion are must faster at parrying and can do it whenever they like
			//Also, on medium, all level 3 people can parry any time and on hard, all level 2 or 3 people can parry any time
		}
		*/
		else
		{
			saberBusy = Jedi_SaberBusy( self );
		}
	}
	else
	{
		if ( incoming->s.weapon == WP_SABER )
		{//flying lightsaber, face it!
			//FIXME: for this to actually work, we'd need to call update angles too?
			//Jedi_FaceEntity( self, incoming, qtrue );
		}
		VectorCopy( incoming->currentOrigin, hitloc );
		VectorNormalize2( incoming->s.pos.trDelta, hitdir );
	}

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	//VectorNormalize( diff );
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);// + Q_flrand(-0.10f,0.10f);
	//totalHeight = self->client->renderInfo.eyePoint[2] - self->absmin[2];
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];// + Q_irand(-6,6);

	qboolean doDodge = qfalse;
	qboolean alwaysDodgeOrRoll = qfalse;
	if ( self->client->NPC_class == CLASS_BOBAFETT )
	{
		saberBusy = qtrue;
		doDodge = qtrue;
		alwaysDodgeOrRoll = qtrue;
	}
	else
	{
		if ( self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER )
		{
			saberBusy = qtrue;
			alwaysDodgeOrRoll = qtrue;
		}
		//see if we can dodge if need-be
		if ( (dist>16&&(Q_irand( 0, 2 )||saberBusy))
			|| self->client->ps.saberInFlight
			|| !self->client->ps.SaberActive()
			|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) )
		{//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
			if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT_JG) )
			{//acrobat or fencer or above
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE &&//on the ground
					!(self->client->ps.pm_flags&PMF_DUCKED)&&cmd->upmove>=0&&TIMER_Done( self, "duck" )//not ducking
					&& !PM_InRoll( &self->client->ps )//not rolling
					&& !PM_InKnockDown( &self->client->ps )//not knocked down
					&& ( self->client->ps.saberInFlight ||
						(self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) ||
						(!PM_SaberInAttack( self->client->ps.saberMove )//not attacking
						&& !PM_SaberInStart( self->client->ps.saberMove )//not starting an attack
						&& !PM_SpinningSaberAnim( self->client->ps.torsoAnim )//not in a saber spin
						&& !PM_SaberInSpecialAttack( self->client->ps.torsoAnim ))//not in a special attack
						)
					)
				{//need to check all these because it overrides both torso and legs with the dodge
					doDodge = qtrue;
				}
			}
		}
	}

	qboolean doRoll = qfalse;
	if (	( self->client->NPC_class == CLASS_BOBAFETT //boba fett
				|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) //non-saber reborn (cultist)
			)
			&& !Q_irand( 0, 2 )
		)
	{
		doRoll = qtrue;
	}

	// Figure out what quadrant the block was in.
	if ( d_JediAI->integer )
	{
		gi.Printf( "(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, hitloc[2]-self->absmin[2],zdiff,rightdot);
	}

	//UL = > -1//-6
	//UR = > -6//-9
	//TOP = > +6//+4
	//FIXME: take FP_SABER_DEFENSE into account here somehow?
	if ( zdiff >= -5 )//was 0
	{
		if ( incoming || !saberBusy || alwaysDodgeOrRoll )
		{
			if ( rightdot > 12
				|| (rightdot > 3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, 0.3
			{//coming from right
				if ( doDodge )
				{
					if ( doRoll )
					{//roll!
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
						evasionType = EVASION_DUCK;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FL;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BL;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							if ( d_JediAI->integer )
							{
								gi.Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "UR block\n" );
				}
			}
			else if ( rightdot < -12
				|| (rightdot < -3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, -0.3
			{//coming from left
				if ( doDodge )
				{
					if ( doRoll )
					{
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeLeft", 0 );
						evasionType = EVASION_DUCK;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FR;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BR;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							if ( d_JediAI->integer )
							{
								gi.Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				evasionType = EVASION_PARRY;
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					duckChance = 4;
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "TOP block\n" );
				}
			}
		}
		else
		{
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				if ( d_JediAI->integer )
				{
					gi.Printf( "duck " );
				}
			}
		}
	}
	//LL = -22//= -18 to -39
	//LR = -23//= -20 to -41
	else if ( zdiff > -22 )//was-15 )
	{
		if ( 1 )//zdiff < -10 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				if ( d_JediAI->integer )
				{
					gi.Printf( "duck " );
				}
			}
			else
			{//in air!  Ducking does no good
			}
		}
		if ( incoming || !saberBusy || alwaysDodgeOrRoll )
		{
			if ( rightdot > 8 || (rightdot > 3 && zdiff < -11) )//was normalized, 0.2
			{
				if ( doDodge )
				{
					if ( doRoll )
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else
					{
						dodgeAnim = BOTH_DODGE_L;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "mid-UR block\n" );
				}
			}
			else if ( rightdot < -8 || (rightdot < -3 && zdiff < -11) )//was normalized, -0.2
			{
				if ( doDodge )
				{
					if ( doRoll )
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else
					{
						dodgeAnim = BOTH_DODGE_R;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "mid-UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				if ( evasionType == EVASION_DUCK )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_PARRY;
				}
				if ( d_JediAI->integer )
				{
					gi.Printf( "mid-TOP block\n" );
				}
			}
		}
	}
	else
	{
		if ( saberBusy || (zdiff < -36 && ( zdiff < -44 || !Q_irand( 0, 2 ) ) ) )//was -30 and -40//2nd one was -46
		{//jump!
			if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
			{//already in air, duck to pull up legs
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				if ( d_JediAI->integer )
				{
					gi.Printf( "legs up\n" );
				}
				if ( incoming || !saberBusy )
				{
					//since the jump may be cleared if not safe, set a lower block too
					if ( rightdot >= 0 )
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						evasionType = EVASION_DUCK_PARRY;
						if ( d_JediAI->integer )
						{
							gi.Printf( "LR block\n" );
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						evasionType = EVASION_DUCK_PARRY;
						if ( d_JediAI->integer )
						{
							gi.Printf( "LL block\n" );
						}
					}
				}
			}
			else
			{//gotta jump!
				if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
					(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
				{//superjump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& self->client->ps.forceRageRecoveryTime < level.time
						&& !(self->client->ps.forcePowersActive&(1<<FP_RAGE))
						&& !PM_InKnockDown( &self->client->ps ) )
					{
						self->client->ps.forceJumpCharge = 320;//FIXME: calc this intelligently
						evasionType = EVASION_FJUMP;
						if ( d_JediAI->integer )
						{
							gi.Printf( "force jump + " );
						}
					}
				}
				else
				{//normal jump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& self->client->ps.forceRageRecoveryTime < level.time
						&& !(self->client->ps.forcePowersActive&(1<<FP_RAGE)) )
					{
						if ( (self->client->NPC_class == CLASS_BOBAFETT
								|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER)
							)
							&& !Q_irand( 0, 1 ) )
						{//flip!
							if ( rightdot > 0 )
							{
								TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
								TIMER_Set( self, "strafeRight", 0 );
								TIMER_Set( self, "walking", 0 );
							}
							else
							{
								TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
								TIMER_Set( self, "strafeLeft", 0 );
								TIMER_Set( self, "walking", 0 );
							}
						}
						else
						{
							if ( self == NPC )
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
						}
						evasionType = EVASION_JUMP;
						if ( d_JediAI->integer )
						{
							gi.Printf( "jump + " );
						}
					}
					if ( self->client->NPC_class == CLASS_ALORA
						|| self->client->NPC_class == CLASS_SHADOWTROOPER
						|| self->client->NPC_class == CLASS_TAVION )
					{
						if ( !incoming
							&& self->client->ps.groundEntityNum < ENTITYNUM_NONE
							&& !Q_irand( 0, 2 ) )
						{
							if ( !PM_SaberInAttack( self->client->ps.saberMove )
								&& !PM_SaberInStart( self->client->ps.saberMove )
								&& !PM_InRoll( &self->client->ps )
								&& !PM_InKnockDown( &self->client->ps )
								&& !PM_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
							{//do the butterfly!
								int butterflyAnim;
								if ( self->client->NPC_class == CLASS_ALORA
									&& !Q_irand( 0, 2 ) )
								{
									butterflyAnim = BOTH_ALORA_SPIN;
								}
								else if ( Q_irand( 0, 1 ) )
								{
									butterflyAnim = BOTH_BUTTERFLY_LEFT;
								}
								else
								{
									butterflyAnim = BOTH_BUTTERFLY_RIGHT;
								}
								evasionType = EVASION_CARTWHEEL;
								NPC_SetAnim( self, SETANIM_BOTH, butterflyAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.velocity[2] = 225;
								self->client->ps.forceJumpZStart = self->currentOrigin[2];//so we don't take damage if we land at same height
								self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
								self->client->ps.SaberActivateTrail( 300 );//FIXME: reset this when done!
								/*
								if ( self->client->NPC_class == CLASS_BOBAFETT
									|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) )
								{
									G_AddEvent( self, EV_JUMP, 0 );
								}
								else
								*/
								{
									G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}
								cmd->upmove = 0;
								saberBusy = qtrue;
							}
						}
					}
				}
				if ( ((evasionType = Jedi_CheckFlipEvasions( self, rightdot, zdiff ))!=EVASION_NONE) )
				{
					if ( d_slowmodeath->integer > 5 && self->enemy && !self->enemy->s.number )
					{
						G_StartMatrixEffect( self );
					}
					saberBusy = qtrue;
				}
				else if ( incoming || !saberBusy )
				{
					//since the jump may be cleared if not safe, set a lower block too
					if ( rightdot >= 0 )
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						if ( evasionType == EVASION_JUMP )
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if ( evasionType == EVASION_NONE )
						{
							evasionType = EVASION_PARRY;
						}
						if ( d_JediAI->integer )
						{
							gi.Printf( "LR block\n" );
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						if ( evasionType == EVASION_JUMP )
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if ( evasionType == EVASION_NONE )
						{
							evasionType = EVASION_PARRY;
						}
						if ( d_JediAI->integer )
						{
							gi.Printf( "LL block\n" );
						}
					}
				}
			}
		}
		else
		{
			if ( incoming || !saberBusy )
			{
				if ( rightdot >= 0 )
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					evasionType = EVASION_PARRY;
					if ( d_JediAI->integer )
					{
						gi.Printf( "LR block\n" );
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					evasionType = EVASION_PARRY;
					if ( d_JediAI->integer )
					{
						gi.Printf( "LL block\n" );
					}
				}
				if ( incoming && incoming->s.weapon == WP_SABER )
				{//thrown saber!
					if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
						(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
					{//superjump
						//FIXME: check the jump, if can't, then block
						if ( self->NPC
							&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
							&& self->client->ps.forceRageRecoveryTime < level.time
							&& !(self->client->ps.forcePowersActive&(1<<FP_RAGE))
							&& !PM_InKnockDown( &self->client->ps ) )
						{
							self->client->ps.forceJumpCharge = 320;//FIXME: calc this intelligently
							evasionType = EVASION_FJUMP;
							if ( d_JediAI->integer )
							{
								gi.Printf( "force jump + " );
							}
						}
					}
					else
					{//normal jump
						//FIXME: check the jump, if can't, then block
						if ( self->NPC
							&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
							&& self->client->ps.forceRageRecoveryTime < level.time
							&& !(self->client->ps.forcePowersActive&(1<<FP_RAGE)))
						{
							if ( self == NPC )
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
							evasionType = EVASION_JUMP_PARRY;
							if ( d_JediAI->integer )
							{
								gi.Printf( "jump + " );
							}
						}
					}
				}
			}
		}
	}
	if ( evasionType == EVASION_NONE )
	{
		return EVASION_NONE;
	}
//=======================================================================================
	//see if it's okay to jump
	Jedi_CheckJumpEvasionSafety( self, cmd, evasionType );
//=======================================================================================
	//stop taunting
	TIMER_Set( self, "taunting", 0 );
	//stop gripping
	TIMER_Set( self, "gripping", -level.time );
	WP_ForcePowerStop( self, FP_GRIP );
	//stop draining
	TIMER_Set( self, "draining", -level.time );
	WP_ForcePowerStop( self, FP_DRAIN );

	if ( dodgeAnim != -1 )
	{//dodged
		evasionType = EVASION_DODGE;
		NPC_SetAnim( self, SETANIM_BOTH, dodgeAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoAnimTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
		if ( d_slowmodeath->integer > 5 && self->enemy && !self->enemy->s.number )
		{
			G_StartMatrixEffect( self );
		}
	}
	else
	{
		if ( duckChance )
		{
			if ( !Q_irand( 0, duckChance ) )
			{
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				if ( evasionType == EVASION_PARRY )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_DUCK;
				}
				/*
				if ( d_JediAI->integer )
				{
					gi.Printf( "duck " );
				}
				*/
			}
		}

		if ( incoming )
		{
			self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
		}

	}
	//if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		int parryReCalcTime = Jedi_ReCalcParryTime( self, evasionType );
		if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
	return evasionType;
}

static evasionType_t Jedi_CheckEvadeSpecialAttacks( void )
{
	if ( !NPC
		|| !NPC->client )
	{
		return EVASION_NONE;
	}

	if ( !NPC->enemy
		|| NPC->enemy->health <= 0
		|| !NPC->enemy->client )
	{//don't keep blocking him once he's dead (or if not a client)
		return EVASION_NONE;
	}

	if ( NPC->enemy->s.number >= MAX_CLIENTS )
	{//only do these against player
		return EVASION_NONE;
	}

	if ( !TIMER_Done( NPC, "specialEvasion" ) )
	{//still evading from last time
		return EVASION_NONE;
	}

	if ( NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
		|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 )
	{//back away from these
		if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
			|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
			|| NPC->client->NPC_class == CLASS_ALORA
			|| Q_irand( 0, NPCInfo->rank ) > RANK_LT_JG )
		{//see if we should back off
			if ( InFront( NPC->currentOrigin, NPC->enemy->currentOrigin, NPC->enemy->currentAngles ) )
			{//facing me
				float minSafeDistSq = (NPC->maxs[0]*1.5f+NPC->enemy->maxs[0]*1.5f+NPC->enemy->client->ps.SaberLength()+24.0f);
				minSafeDistSq *= minSafeDistSq;
				if ( DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin ) < minSafeDistSq )
				{//back off!
					Jedi_StartBackOff();
					return EVASION_OTHER;
				}
			}
		}
	}
	else
	{//check some other attacks?
		//check roll-stab
		if ( NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB
			|| (NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F && ((NPC->enemy->client->pers.lastCommand.buttons&BUTTON_ATTACK)||(NPC->enemy->client->ps.pm_flags&PMF_ATTACK_HELD)) ) )
		{//either already in a roll-stab or may go into one
			if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| Q_irand( -3, NPCInfo->rank ) > RANK_LT_JG )
			{//see if we should evade
				vec3_t yawOnlyAngles = {0, NPC->enemy->currentAngles[YAW], 0 };
				if ( InFront( NPC->currentOrigin, NPC->enemy->currentOrigin, yawOnlyAngles, 0.25f ) )
				{//facing me
					float minSafeDistSq = (NPC->maxs[0]*1.5f+NPC->enemy->maxs[0]*1.5f+NPC->enemy->client->ps.SaberLength()+24.0f);
					minSafeDistSq *= minSafeDistSq;
					float distSq = DistanceSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );
					if ( distSq < minSafeDistSq )
					{//evade!
						qboolean doJump = (qboolean)( NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB || distSq < 3000.0f );//not much time left, just jump!
						if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
							|| !doJump )
						{//roll?
							vec3_t enemyRight, dir2Me;

							AngleVectors( yawOnlyAngles, NULL, enemyRight, NULL );
							VectorSubtract( NPC->currentOrigin, NPC->enemy->currentOrigin, dir2Me );
							VectorNormalize( dir2Me );
							float dot = DotProduct( enemyRight, dir2Me );

							ucmd.forwardmove = 0;
							TIMER_Start( NPC, "duck", Q_irand( 500, 1500 ) );
							ucmd.upmove = -127;
							//NOTE: this *assumes* I'm facing him!
							if ( dot > 0 )
							{//I'm to his right
								if ( !NPC_MoveDirClear( 0, -127, qfalse  ) )
								{//fuck, jump instead
									doJump = qtrue;
								}
								else
								{
									TIMER_Start( NPC, "strafeLeft", Q_irand( 500, 1500 ) );
									TIMER_Set( NPC, "strafeRight", 0 );
									ucmd.rightmove = -127;
									if ( d_JediAI->integer )
									{
										Com_Printf( "%s rolling left from roll-stab!\n", NPC->NPC_type );
									}
									if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
									{//fuck it, just force it
										NPC_SetAnim(NPC,SETANIM_BOTH,BOTH_ROLL_L,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
										G_AddEvent( NPC, EV_ROLL, 0 );
										NPC->client->ps.saberMove = LS_NONE;
									}
								}
							}
							else
							{//I'm to his left
								if ( !NPC_MoveDirClear( 0, 127, qfalse  ) )
								{//fuck, jump instead
									doJump = qtrue;
								}
								else
								{
									TIMER_Start( NPC, "strafeRight", Q_irand( 500, 1500 ) );
									TIMER_Set( NPC, "strafeLeft", 0 );
									ucmd.rightmove = 127;
									if ( d_JediAI->integer )
									{
										Com_Printf( "%s rolling right from roll-stab!\n", NPC->NPC_type );
									}
									if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
									{//fuck it, just force it
										NPC_SetAnim(NPC,SETANIM_BOTH,BOTH_ROLL_R,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
										G_AddEvent( NPC, EV_ROLL, 0 );
										NPC->client->ps.saberMove = LS_NONE;
									}
								}
							}
							if ( !doJump )
							{
								TIMER_Set( NPC, "specialEvasion", 3000 );
								return EVASION_DUCK;
							}
						}
						//didn't roll, do jump
						if ( NPC->s.weapon != WP_SABER
							|| (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
							|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
							|| NPC->client->NPC_class == CLASS_ALORA
							|| Q_irand( -3, NPCInfo->rank ) > RANK_CREWMAN )
						{//superjump
							NPC->client->ps.forceJumpCharge = 320;//FIXME: calc this intelligently
							if ( Q_irand( 0, 2 ) )
							{//make it a backflip
								ucmd.forwardmove = -127;
								TIMER_Set( NPC, "roamTime", -level.time );
								TIMER_Set( NPC, "strafeLeft", -level.time );
								TIMER_Set( NPC, "strafeRight", -level.time );
								TIMER_Set( NPC, "walking", -level.time );
								TIMER_Set( NPC, "moveforward", -level.time );
								TIMER_Set( NPC, "movenone", -level.time );
								TIMER_Set( NPC, "moveright", -level.time );
								TIMER_Set( NPC, "moveleft", -level.time );
								TIMER_Set( NPC, "movecenter", -level.time );
								TIMER_Set( NPC, "moveback", Q_irand( 500, 1000 ) );
								if ( d_JediAI->integer )
								{
									Com_Printf( "%s backflipping from roll-stab!\n", NPC->NPC_type );
								}
							}
							else
							{
								if ( d_JediAI->integer )
								{
									Com_Printf( "%s force-jumping over roll-stab!\n", NPC->NPC_type );
								}
							}
							TIMER_Set( NPC, "specialEvasion", 3000 );
							return EVASION_FJUMP;
						}
						else
						{//normal jump
							ucmd.upmove = 127;
							if ( d_JediAI->integer )
							{
								Com_Printf( "%s jumping over roll-stab!\n", NPC->NPC_type );
							}
							TIMER_Set( NPC, "specialEvasion", 2000 );
							return EVASION_JUMP;
						}
					}
				}
			}
		}
	}
	return EVASION_NONE;
}

extern int WPDEBUG_SaberColor( saber_colors_t saberColor );
static qboolean Jedi_SaberBlock( void )
{
	vec3_t hitloc, saberTipOld, saberTip, top, bottom, axisPoint, saberPoint, dir;//saberBase,
	vec3_t pointDir, baseDir, tipDir, saberHitPoint, saberMins={-4,-4,-4}, saberMaxs={4,4,4};
	float	pointDist, baseDirPerc;
	float	dist, bestDist = Q3_INFINITE;
	int		saberNum = 0, bladeNum = 0;
	int		closestSaberNum = 0, closestBladeNum = 0;

	//FIXME: reborn don't block enough anymore
	/*
	//maybe do this on easy only... or only on grunt-level reborn
	if ( NPC->client->ps.weaponTime )
	{//i'm attacking right now
		return qfalse;
	}
	*/

	if ( !TIMER_Done( NPC, "parryReCalcTime" ) )
	{//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if ( NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't move the saber to another position yet
		return qfalse;
	}

	/*
	if ( NPCInfo->rank < RANK_LT_JG && Q_irand( 0, (2 - g_spskill->integer) ) )
	{//lower rank reborn have a random chance of not doing it at all
		NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 300;
		return qfalse;
	}
	*/

	if ( NPC->enemy->health <= 0 || !NPC->enemy->client )
	{//don't keep blocking him once he's dead (or if not a client)
		return qfalse;
	}
	/*
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePointNext, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirNext, saberTipNext );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePointOld, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirOld, saberTipOld );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );

	VectorSubtract( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->renderInfo.muzzlePointOld, dir );//get the dir
	VectorAdd( dir, NPC->enemy->client->renderInfo.muzzlePoint, saberBase );//extrapolate

	VectorSubtract( saberTip, saberTipOld, dir );//get the dir
	VectorAdd( dir, saberTip, saberTipOld );//extrapolate

	VectorCopy( NPC->currentOrigin, top );
	top[2] = NPC->absmax[2];
	VectorCopy( NPC->currentOrigin, bottom );
	bottom[2] = NPC->absmin[2];

	float dist = ShortestLineSegBewteen2LineSegs( saberBase, saberTipOld, bottom, top, saberPoint, axisPoint );
	if ( 0 )//dist > NPC->maxs[0]*4 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI->integer )
		{
			gi.Printf( "enemy saber dist: %4.2f\n", dist );
		}
		TIMER_Set( NPC, "parryTime", -1 );
		return qfalse;
	}

	//get the actual point of impact
	trace_t	tr;
	gi.trace( &tr, saberPoint, vec3_origin, vec3_origin, axisPoint, NPC->enemy->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid )
	{//estimate
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->maxs[0]*1.22, dir, hitloc );
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}
	*/

	//FIXME: need to check against both sabers/blades now!...?

	for ( saberNum = 0; saberNum < MAX_SABERS; saberNum++ )
	{
		for ( bladeNum = 0; bladeNum < NPC->enemy->client->ps.saber[saberNum].numBlades; bladeNum++ )
		{
			if ( NPC->enemy->client->ps.saber[saberNum].type != SABER_NONE
				&& NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].length > 0 )
			{//valid saber and this blade is on
				VectorMA( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld, NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].length, NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld, saberTipOld );
				VectorMA( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].length, NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, saberTip );

				VectorCopy( NPC->currentOrigin, top );
				top[2] = NPC->absmax[2];
				VectorCopy( NPC->currentOrigin, bottom );
				bottom[2] = NPC->absmin[2];

				dist = ShortestLineSegBewteen2LineSegs( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
				if ( dist < bestDist )
				{
					bestDist = dist;
					closestSaberNum = saberNum;
					closestBladeNum = bladeNum;
				}
			}
		}
	}

	if ( bestDist > NPC->maxs[0]*5 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI->integer )
		{
			Com_Printf( S_COLOR_RED"enemy saber dist: %4.2f\n", bestDist );
		}
		/*
		if ( bestDist < 300 //close
			&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
			&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
		{//he's swinging at me and close enough to be a threat, don't start an attack right now
			TIMER_Set( NPC, "parryTime", 100 );
		}
		else
		*/
		{
			TIMER_Set( NPC, "parryTime", -1 );
		}
		return qfalse;
	}

	dist = bestDist;

	if ( d_JediAI->integer )
	{
		Com_Printf( S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist );
	}

	//now use the closest blade for my evasion check
	VectorMA( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePointOld, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].length, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzleDirOld, saberTipOld );
	VectorMA( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].length, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzleDir, saberTip );

	VectorCopy( NPC->currentOrigin, top );
	top[2] = NPC->absmax[2];
	VectorCopy( NPC->currentOrigin, bottom );
	bottom[2] = NPC->absmin[2];

	dist = ShortestLineSegBewteen2LineSegs( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
	VectorSubtract( saberPoint, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, pointDir );
	pointDist = VectorLength( pointDir );

	if ( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].length <= 0 )
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist/NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].length;
	}
	VectorSubtract( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].muzzlePointOld, baseDir );
	VectorSubtract( saberTip, saberTipOld, tipDir );
	VectorScale( baseDir, baseDirPerc, baseDir );
	VectorMA( baseDir, 1.0f-baseDirPerc, tipDir, dir );
	VectorMA( saberPoint, 200, dir, hitloc );

	//get the actual point of impact
	trace_t	tr;
	gi.trace( &tr, saberPoint, saberMins, saberMaxs, hitloc, NPC->enemy->s.number, CONTENTS_BODY, (EG2_Collision)0, 0 );//, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid || tr.fraction >= 1.0f )
	{//estimate
		vec3_t	dir2Me;
		VectorSubtract( axisPoint, saberPoint, dir2Me );
		dist = VectorNormalize( dir2Me );
		if ( DotProduct( dir, dir2Me ) < 0.2f )
		{//saber is not swinging in my direction
			/*
			if ( dist < 300 //close
				&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
				&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || PM_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
			{//he's swinging at me and close enough to be a threat, don't start an attack right now
				TIMER_Set( NPC, "parryTime", 100 );
			}
			else
			*/
			{
				TIMER_Set( NPC, "parryTime", -1 );
			}
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs( saberPoint, hitloc, bottom, top, saberHitPoint, hitloc );
		/*
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->maxs[0]*1.22, dir, hitloc );
		*/
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}

	if ( d_JediAI->integer )
	{
		G_DebugLine( saberPoint, hitloc, FRAMETIME, WPDEBUG_SaberColor( NPC->enemy->client->ps.saber[closestSaberNum].blade[closestBladeNum].color ), qtrue );
	}

	//FIXME: if saber is off and/or we have force speed and want to be really cocky,
	//		and the swing misses by some amount, we can use the dodges here... :)
	evasionType_t	evasionType;
	if ( (evasionType=Jedi_SaberBlockGo( NPC, &ucmd, hitloc, dir, NULL, dist )) != EVASION_NONE )
	{//did some sort of evasion
		if ( evasionType != EVASION_DODGE )
		{//(not dodge)
			if ( !NPC->client->ps.saberInFlight )
			{//make sure saber is on
				NPC->client->ps.SaberActivate();
			}

			//debounce our parry recalc time
			int parryReCalcTime = Jedi_ReCalcParryTime( NPC, evasionType );
			TIMER_Set( NPC, "parryReCalcTime", Q_irand( 0, parryReCalcTime ) );
			if ( d_JediAI->integer )
			{
				gi.Printf( "Keep parry choice until: %d\n", level.time + parryReCalcTime );
			}

			//determine how long to hold this anim
			if ( TIMER_Done( NPC, "parryTime" ) )
			{
				if ( NPC->client->NPC_class == CLASS_TAVION
					|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
					|| NPC->client->NPC_class == CLASS_ALORA )
				{
					TIMER_Set( NPC, "parryTime", Q_irand( parryReCalcTime/2, parryReCalcTime*1.5 ) );
				}
				else if ( NPCInfo->rank >= RANK_LT_JG )
				{//fencers and higher hold a parry less
					TIMER_Set( NPC, "parryTime", parryReCalcTime );
				}
				else
				{//others hold it longer
					TIMER_Set( NPC, "parryTime", Q_irand( 1, 2 )*parryReCalcTime );
				}
			}
		}
		else
		{//dodged
			int dodgeTime = NPC->client->ps.torsoAnimTimer;
			if ( NPCInfo->rank > RANK_LT_COMM && NPC->client->NPC_class != CLASS_DESANN )
			{//higher-level guys can dodge faster
				dodgeTime -= 200;
			}
			TIMER_Set( NPC, "parryReCalcTime", dodgeTime );
			TIMER_Set( NPC, "parryTime", dodgeTime );
		}
	}
	if ( evasionType != EVASION_DUCK_PARRY
		&& evasionType != EVASION_JUMP_PARRY
		&& evasionType != EVASION_JUMP
		&& evasionType != EVASION_DUCK
		&& evasionType != EVASION_FJUMP )
	{
		if ( Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE )
		{//got a new evasion!
			//see if it's okay to jump
			Jedi_CheckJumpEvasionSafety( NPC, &ucmd, evasionType );
		}
	}
	return qtrue;
}
/*
-------------------------
Jedi_EvasionSaber

defend if other is using saber and attacking me!
-------------------------
*/
static void Jedi_EvasionSaber( vec3_t enemy_movedir, float enemy_dist, vec3_t enemy_dir )
{
	vec3_t	dirEnemy2Me;
	int		evasionChance = 30;//only step aside 30% if he's moving at me but not attacking
	qboolean	enemy_attacking = qfalse;
	qboolean	throwing_saber = qfalse;
	qboolean	shooting_lightning = qfalse;

	if ( !NPC->enemy->client )
	{
		return;
	}
	else if ( NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time )
	{//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	else if ( (NPC->client->ps.saberEventFlags&SEF_LOCK_WON)
		&& NPC->enemy->painDebounceTime > level.time )
	{//pressing the advantage of winning a saber lock
		return;
	}

	if ( NPC->enemy->client->ps.saberInFlight && !TIMER_Done( NPC, "taunting" ) )
	{//if he's throwing his saber, stop taunting
		TIMER_Set( NPC, "taunting", -level.time );
		if ( !NPC->client->ps.saberInFlight )
		{
			NPC->client->ps.SaberActivate();
		}
	}

	if ( TIMER_Done( NPC, "parryTime" ) )
	{
		if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if ( NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{
		if ( (!NPC->client->ps.saberInFlight || (NPC->client->ps.dualSabers&&NPC->client->ps.saber[1].Active()) )
			&& Jedi_SaberBlock() )
		{
			return;
		}
	}
	else if ( Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE )
	{
		return;
	}


	VectorSubtract( NPC->currentOrigin, NPC->enemy->currentOrigin, dirEnemy2Me );
	VectorNormalize( dirEnemy2Me );

	if ( NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{//enemy is attacking
		enemy_attacking = qtrue;
		evasionChance = 90;
	}

	if ( (NPC->enemy->client->ps.forcePowersActive&(1<<FP_LIGHTNING) ) )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasionChance = 50;
	}

	if ( NPC->enemy->client->ps.saberInFlight
		&& NPC->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE
		&& NPC->enemy->client->ps.saberEntityState != SES_RETURNING )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		throwing_saber = qtrue;
	}

	//FIXME: this needs to take skill and rank(reborn type) into account much more
	if ( Q_irand( 0, 100 ) < evasionChance )
	{//check to see if he's coming at me
		float facingAmt;
		if ( VectorCompare( enemy_movedir, vec3_origin ) || shooting_lightning || throwing_saber )
		{//he's not moving (or he's using a ranged attack), see if he's facing me
			vec3_t	enemy_fwd;
			AngleVectors( NPC->enemy->client->ps.viewangles, enemy_fwd, NULL, NULL );
			facingAmt = DotProduct( enemy_fwd, dirEnemy2Me );
		}
		else
		{//he's moving
			facingAmt = DotProduct( enemy_movedir, dirEnemy2Me );
		}

		if ( Q_flrand( 0.25, 1 ) < facingAmt )
		{//coming at/facing me!
			int whichDefense = 0;
			/*if ( NPC->client->NPC_class == CLASS_SABOTEUR )
			{
				int sabDef = Q_irand( 0, 3 );
				if ( sabDef )
				{//25% chance of trying normal jedi defense logic
					whichDefense = 100;
				}
				else
				{
					if ( sabDef == 1 )
					{//25% chance of strafing
						Jedi_Strafe( 300, 1000, 0, 1000, qfalse );
					}
					else
					{//50% chance of trying to dodge/roll/jump using jedi missile evasion logic
						Jedi_SaberBlock();
					}
					return;
				}
			}
			else */if ( NPC->client->ps.weaponTime
				|| NPC->client->ps.saberInFlight
				|| NPC->client->NPC_class == CLASS_BOBAFETT
				|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
			{//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if ( Q_irand( 0, 10 ) < NPCInfo->stats.aggression )
				{
					return;
				}
				whichDefense = 100;
			}
			else
			{
				if ( shooting_lightning )
				{//check for lightning attack
					//only valid defense is strafe and/or jump
					whichDefense = 100;
				}
				else if ( throwing_saber )
				{//he's thrown his saber!  See if it's coming at me
					float	saberDist;
					vec3_t	saberDir2Me;
					vec3_t	saberMoveDir;
					gentity_t *saber = &g_entities[NPC->enemy->client->ps.saberEntityNum];
					VectorSubtract( NPC->currentOrigin, saber->currentOrigin, saberDir2Me );
					saberDist = VectorNormalize( saberDir2Me );
					VectorCopy( saber->s.pos.trDelta, saberMoveDir );
					VectorNormalize( saberMoveDir );
					if ( !Q_irand( 0, 3 ) )
					{
						//Com_Printf( "(%d) raise agg - enemy threw saber\n", level.time );
						Jedi_Aggression( NPC, 1 );
					}
					if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
					{//it's heading towards me
						if ( saberDist < 100 )
						{//it's close
							whichDefense = Q_irand( 3, 6 );
						}
						else if ( saberDist < 200 )
						{//got some time, yet, try pushing
							whichDefense = Q_irand( 0, 8 );
						}
					}
				}
				if ( whichDefense )
				{//already chose one
				}
				else if ( enemy_dist > 80 || !enemy_attacking )
				{//he's pretty far, or not swinging, just strafe
					if ( VectorCompare( enemy_movedir, vec3_origin ) )
					{//if he's not moving, not swinging and far enough away, no evasion necc.
						return;
					}
					if ( Q_irand( 0, 10 ) < NPCInfo->stats.aggression )
					{
						return;
					}
					whichDefense = 100;
				}
				else
				{//he's getting close and swinging at me
					vec3_t	fwd;
					//see if I'm facing him
					AngleVectors( NPC->client->ps.viewangles, fwd, NULL, NULL );
					if ( DotProduct( enemy_dir, fwd ) < 0.5 )
					{//I'm not really facing him, best option is to strafe
						whichDefense = Q_irand( 5, 16 );
					}
					else if ( enemy_dist < 56 )
					{//he's very close, maybe we should be more inclined to block or throw
						whichDefense = Q_irand( NPCInfo->stats.aggression, 12 );
					}
					else
					{
						whichDefense = Q_irand( 2, 16 );
					}
				}
			}

			if ( whichDefense >= 4 && whichDefense <= 12 )
			{//would try to block
				if ( NPC->client->ps.saberInFlight )
				{//can't, saber in not in hand, so fall back to strafe/jump
					whichDefense = 100;
				}
			}

			switch( whichDefense )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				//use jedi force push? or kick?
				//FIXME: try to do this if health low or enemy back to a cliff?
				if ( Jedi_DecideKick()//let's try a kick
					&& ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
						|| (G_CanKickEntity(NPC, NPC->enemy )&&G_PickAutoKick( NPC, NPC->enemy, qtrue )!=LS_NONE)
						)
					)
				{//kicked
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
				}
				else if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && TIMER_Done( NPC, "parryTime" ) )
				{//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
					ForceThrow( NPC, qfalse );
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				//try to parry the blow
				//gi.Printf( "blocking\n" );
				Jedi_SaberBlock();
				break;
			default:
				//Evade!
				//start a strafe left/right if not already
				if ( !Q_irand( 0, 5 ) || !Jedi_Strafe( 300, 1000, 0, 1000, qfalse ) )
				{//certain chance they will pick an alternative evasion
					//if couldn't strafe, try a different kind of evasion...
					if ( Jedi_DecideKick() && G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE )
					{//kicked!
						TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					}
					else if ( shooting_lightning || throwing_saber || enemy_dist < 80 )
					{
						//FIXME: force-jump+forward - jump over the guy!
						if ( shooting_lightning || (!Q_irand( 0, 2 ) && NPCInfo->stats.aggression < 4 && TIMER_Done( NPC, "parryTime" ) ) )
						{
							if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && !shooting_lightning && Q_irand( 0, 2 ) )
							{//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
								ForceThrow( NPC, qfalse );
							}
							else if ( (NPCInfo->rank==RANK_CREWMAN||NPCInfo->rank>RANK_LT_JG)
								&& !(NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
								&& NPC->client->ps.forceRageRecoveryTime < level.time
								&& !(NPC->client->ps.forcePowersActive&(1<<FP_RAGE))
								&& !PM_InKnockDown( &NPC->client->ps ) )
							{//FIXME: make this a function call?
								//FIXME: check for clearance, safety of landing spot?
								NPC->client->ps.forceJumpCharge = 480;
								//Don't jump again for another 2 to 5 seconds
								TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
								if ( Q_irand( 0, 2 ) )
								{
									ucmd.forwardmove = 127;
									VectorClear( NPC->client->ps.moveDir );
								}
								else
								{
									ucmd.forwardmove = -127;
									VectorClear( NPC->client->ps.moveDir );
								}
								//FIXME: if this jump is cleared, we can't block... so pick a random lower block?
								if ( Q_irand( 0, 1 ) )//FIXME: make intelligent
								{
									NPC->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
								}
								else
								{
									NPC->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
								}
							}
						}
						else if ( enemy_attacking )
						{
							Jedi_SaberBlock();
						}
					}
				}
				else
				{//strafed
					if ( d_JediAI->integer )
					{
						gi.Printf( "def strafe\n" );
					}
					if ( !(NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
						&& NPC->client->ps.forceRageRecoveryTime < level.time
						&& !(NPC->client->ps.forcePowersActive&(1<<FP_RAGE))
						&& (NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG )
						&& !PM_InKnockDown( &NPC->client->ps )
						&& !Q_irand( 0, 5 ) )
					{//FIXME: make this a function call?
						//FIXME: check for clearance, safety of landing spot?
						if ( NPC->client->NPC_class == CLASS_BOBAFETT
							|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
							|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
						{
							NPC->client->ps.forceJumpCharge = 280;//FIXME: calc this intelligently?
						}
						else
						{
							NPC->client->ps.forceJumpCharge = 320;
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
					}
				}
				break;
			}

			//turn off slow walking no matter what
			TIMER_Set( NPC, "walking", -level.time );
			TIMER_Set( NPC, "taunting", -level.time );
		}
	}
}
/*
-------------------------
Jedi_Flee
-------------------------
*/
/*

static qboolean Jedi_Flee( void )
{
	return qfalse;
}
*/


/*
==========================================================================================
INTERNAL AI ROUTINES
==========================================================================================
*/
gentity_t *Jedi_FindEnemyInCone( gentity_t *self, gentity_t *fallback, float minDot )
{
	vec3_t forward, mins, maxs, dir;
	float	dist, bestDist = Q3_INFINITE;
	gentity_t	*enemy = fallback;
	gentity_t	*check = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			e, numListedEntities;
	trace_t		tr;

	if ( !self->client )
	{
		return enemy;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	for ( e = 0 ; e < 3 ; e++ )
	{
		mins[e] = self->currentOrigin[e] - 1024;
		maxs[e] = self->currentOrigin[e] + 1024;
	}
	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		check = entityList[e];
		if ( check == self )
		{//me
			continue;
		}
		if ( !(check->inuse) )
		{//freed
			continue;
		}
		if ( !check->client )
		{//not a client - FIXME: what about turrets?
			continue;
		}
		if ( check->client->playerTeam != self->client->enemyTeam )
		{//not an enemy - FIXME: what about turrets?
			continue;
		}
		if ( check->health <= 0 )
		{//dead
			continue;
		}

		if ( !gi.inPVS( check->currentOrigin, self->currentOrigin ) )
		{//can't potentially see them
			continue;
		}

		VectorSubtract( check->currentOrigin, self->currentOrigin, dir );
		dist = VectorNormalize( dir );

		if ( DotProduct( dir, forward ) < minDot )
		{//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		gi.trace( &tr, self->currentOrigin, vec3_origin, vec3_origin, check->currentOrigin, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
		if ( tr.fraction < 1.0f && tr.entityNum != check->s.number )
		{//must have clear shot
			continue;
		}

		if ( dist < bestDist )
		{//closer than our last best one
			dist = bestDist;
			enemy = check;
		}
	}
	return enemy;
}

static void Jedi_SetEnemyInfo( vec3_t enemy_dest, vec3_t enemy_dir, float *enemy_dist, vec3_t enemy_movedir, float *enemy_movespeed, int prediction )
{
	if ( !NPC || !NPC->enemy )
	{//no valid enemy
		return;
	}
	if ( !NPC->enemy->client )
	{
		VectorClear( enemy_movedir );
		*enemy_movespeed = 0;
		VectorCopy( NPC->enemy->currentOrigin, enemy_dest );
		enemy_dest[2] += NPC->enemy->mins[2] + 24;//get it's origin to a height I can work with
		VectorSubtract( enemy_dest, NPC->currentOrigin, enemy_dir );
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir );// - (NPC->client->ps.saberLengthMax + NPC->maxs[0]*1.5 + 16);
	}
	else
	{//see where enemy is headed
		VectorCopy( NPC->enemy->client->ps.velocity, enemy_movedir );
		*enemy_movespeed = VectorNormalize( enemy_movedir );
		//figure out where he'll be, say, 3 frames from now
		VectorMA( NPC->enemy->currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest );
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract( enemy_dest, NPC->currentOrigin, enemy_dir );//NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir ) - (NPC->client->ps.SaberLengthMax() + NPC->maxs[0]*1.5 + 16);
		//FIXME: keep a group of enemies around me and use that info to make decisions...
		//		For instance, if there are multiple enemies, evade more, push them away
		//		and use medium attacks.  If enemies are using blasters, switch to fast.
		//		If one jedi enemy, use strong attacks.  Use grip when fighting one or
		//		two enemies, use lightning spread when fighting multiple enemies, etc.
		//		Also, when kill one, check rest of group instead of walking up to victim.
	}
	//init this to false
	enemy_in_striking_range = qfalse;
	if ( *enemy_dist <= 0.0f )
	{
		enemy_in_striking_range = qtrue;
	}
	else
	{//if he's too far away, see if he's at least facing us or coming towards us
		if ( *enemy_dist <= 32.0f )
		{//has to be facing us
			vec3_t eAngles = {0,NPC->currentAngles[YAW],0};
			if ( InFOV( NPC->currentOrigin, NPC->enemy->currentOrigin, eAngles, 30, 90 ) )
			{//in striking range
				enemy_in_striking_range = qtrue;
			}
		}
		if ( *enemy_dist >= 64.0f )
		{//we have to be approaching each other
			float vDot = 1.0f;
			if ( !VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
			{//I am moving, see if I'm moving toward the enemy
				vec3_t	eDir;
				VectorSubtract( NPC->enemy->currentOrigin, NPC->currentOrigin, eDir );
				VectorNormalize( eDir );
				vDot = DotProduct( eDir, NPC->client->ps.velocity );
			}
			else if ( NPC->enemy->client && !VectorCompare( NPC->enemy->client->ps.velocity, vec3_origin ) )
			{//I'm not moving, but the enemy is, see if he's moving towards me
				vec3_t	meDir;
				VectorSubtract( NPC->currentOrigin, NPC->enemy->currentOrigin, meDir );
				VectorNormalize( meDir );
				vDot = DotProduct( meDir, NPC->enemy->client->ps.velocity );
			}
			else
			{//neither of us is moving, below check will fail, so just return
				return;
			}
			if ( vDot >= *enemy_dist )
			{//moving towards each other
				enemy_in_striking_range = qtrue;
			}
		}
	}
}

void NPC_EvasionSaber( void )
{
	if ( ucmd.upmove <= 0//not jumping
		&& (!ucmd.upmove || !ucmd.rightmove) )//either just ducking or just strafing (i.e.: not rolling
	{//see if we need to avoid their saber
		vec3_t	enemy_dir, enemy_movedir, enemy_dest;
		float	enemy_dist, enemy_movespeed;
		//set enemy
		Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );
		Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
	}
}

extern float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire );
static void Jedi_FaceEnemy( qboolean doPitch )
{
	vec3_t	enemy_eyes, eyes, angles;

	if ( NPC == NULL )
		return;

	if ( NPC->enemy == NULL )
		return;

	if ( NPC->client->ps.forcePowersActive & (1<<FP_GRIP) &&
		NPC->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//don't update?
		NPCInfo->desiredPitch = NPC->client->ps.viewangles[PITCH];
		NPCInfo->desiredYaw = NPC->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot( NPC, SPOT_HEAD, eyes );

	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_eyes );

	if ( NPC->client->NPC_class == CLASS_BOBAFETT
		&& TIMER_Done( NPC, "flameTime" )
		&& NPC->s.weapon != WP_NONE
		&& NPC->s.weapon != WP_DISRUPTOR
		&& (NPC->s.weapon != WP_ROCKET_LAUNCHER||!(NPCInfo->scriptFlags&SCF_ALT_FIRE))
		&& NPC->s.weapon != WP_THERMAL
		&& NPC->s.weapon != WP_TRIP_MINE
		&& NPC->s.weapon != WP_DET_PACK
		&& NPC->s.weapon != WP_STUN_BATON
		&& NPC->s.weapon != WP_MELEE )
	{//boba leads his enemy
		if ( NPC->health < NPC->max_health*0.5f )
		{//lead
			float missileSpeed = WP_SpeedOfMissileForWeapon( NPC->s.weapon, ((qboolean)(NPCInfo->scriptFlags&SCF_ALT_FIRE)) );
			if ( missileSpeed )
			{
				float eDist = Distance( eyes, enemy_eyes );
				eDist /= missileSpeed;//How many seconds it will take to get to the enemy
				VectorMA( enemy_eyes, eDist*Q_flrand(0.95f,1.25f), NPC->enemy->client->ps.velocity, enemy_eyes );
			}
		}
	}

	//Find the desired angles
	if ( !NPC->client->ps.saberInFlight
		&& (NPC->client->ps.legsAnim == BOTH_A2_STABBACK1
			|| NPC->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| NPC->client->ps.legsAnim == BOTH_ATTACK_BACK
			|| NPC->client->ps.legsAnim == BOTH_A7_KICK_B )
		)
	{//point *away*
		GetAnglesForDirection( enemy_eyes, eyes, angles );
	}
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_R )
	{//keep enemy to right
	}
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_L )
	{//keep enemy to left
	}
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_RL
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_BF
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_S )
	{//???
	}
	else
	{//point towards him
		GetAnglesForDirection( eyes, enemy_eyes, angles );
	}

	NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );
	/*
	if ( NPC->client->ps.saberBlocked == BLOCKED_UPPER_LEFT )
	{//temp hack- to make up for poor coverage on left side
		NPCInfo->desiredYaw += 30;
	}
	*/

	if ( doPitch )
	{
		NPCInfo->desiredPitch = AngleNormalize360( angles[PITCH] );
		if ( NPC->client->ps.saberInFlight )
		{//tilt down a little
			NPCInfo->desiredPitch += 10;
		}
	}
	//FIXME: else desiredPitch = 0?  Or keep previous?
}

static void Jedi_DebounceDirectionChanges( void )
{
	//FIXME: check these before making fwd/back & right/left decisions?
	//Time-debounce changes in forward/back dir
	if ( ucmd.forwardmove > 0 )
	{
		if ( !TIMER_Done( NPC, "moveback" ) || !TIMER_Done( NPC, "movenone" ) )
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.rightmove > 0 )
			{
				ucmd.rightmove = 127;
			}
			else if ( ucmd.rightmove < 0 )
			{
				ucmd.rightmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveback", -level.time );
			if ( TIMER_Done( NPC, "movenone" ) )
			{
				TIMER_Set( NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveforward" ) )
		{//FIXME: should be if it's zero?
			if ( TIMER_Done( NPC, "lastmoveforward" ) )
			{
				int holdDirTime = Q_irand( 500, 2000 );
				TIMER_Set( NPC, "moveforward", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveforward", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
			//if being forced to move forward, do a full-speed moveforward
			ucmd.forwardmove = 127;
			VectorClear( NPC->client->ps.moveDir );
		}
	}
	else if ( ucmd.forwardmove < 0 )
	{
		if ( !TIMER_Done( NPC, "moveforward" ) || !TIMER_Done( NPC, "movenone" ) )
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.rightmove > 0 )
			{
				ucmd.rightmove = 127;
			}
			else if ( ucmd.rightmove < 0 )
			{
				ucmd.rightmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveforward", -level.time );
			if ( TIMER_Done( NPC, "movenone" ) )
			{
				TIMER_Set( NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveback" ) )
		{//FIXME: should be if it's zero?
			if ( TIMER_Done( NPC, "lastmoveback" ) )
			{
				int holdDirTime = Q_irand( 500, 2000 );
				TIMER_Set( NPC, "moveback", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveback", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveback
			ucmd.forwardmove = -127;
			VectorClear( NPC->client->ps.moveDir );
		}
	}
	else if ( !TIMER_Done( NPC, "moveforward" ) )
	{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		ucmd.forwardmove = 127;
		VectorClear( NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPC, "moveback" ) )
	{//NOTE: edge checking should stop me if this is bad...
		ucmd.forwardmove = -127;
		VectorClear( NPC->client->ps.moveDir );
	}
	//Time-debounce changes in right/left dir
	if ( ucmd.rightmove > 0 )
	{
		if ( !TIMER_Done( NPC, "moveleft" ) || !TIMER_Done( NPC, "movecenter" ) )
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.forwardmove > 0 )
			{
				ucmd.forwardmove = 127;
			}
			else if ( ucmd.forwardmove < 0 )
			{
				ucmd.forwardmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveleft", -level.time );
			if ( TIMER_Done( NPC, "movecenter" ) )
			{
				TIMER_Set( NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveright" ) )
		{//FIXME: should be if it's zero?
			if ( TIMER_Done( NPC, "lastmoveright" ) )
			{
				int holdDirTime = Q_irand( 250, 1500 );
				TIMER_Set( NPC, "moveright", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveright", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveright
			ucmd.rightmove = 127;
			VectorClear( NPC->client->ps.moveDir );
		}
	}
	else if ( ucmd.rightmove < 0 )
	{
		if ( !TIMER_Done( NPC, "moveright" ) || !TIMER_Done( NPC, "movecenter" ) )
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.forwardmove > 0 )
			{
				ucmd.forwardmove = 127;
			}
			else if ( ucmd.forwardmove < 0 )
			{
				ucmd.forwardmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveright", -level.time );
			if ( TIMER_Done( NPC, "movecenter" ) )
			{
				TIMER_Set( NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveleft" ) )
		{//FIXME: should be if it's zero?
			if ( TIMER_Done( NPC, "lastmoveleft" ) )
			{
				int holdDirTime = Q_irand( 250, 1500 );
				TIMER_Set( NPC, "moveleft", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveleft", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveleft
			ucmd.rightmove = -127;
			VectorClear( NPC->client->ps.moveDir );
		}
	}
	else if ( !TIMER_Done( NPC, "moveright" ) )
	{//NOTE: edge checking should stop me if this is bad...
		ucmd.rightmove = 127;
		VectorClear( NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPC, "moveleft" ) )
	{//NOTE: edge checking should stop me if this is bad...
		ucmd.rightmove = -127;
		VectorClear( NPC->client->ps.moveDir );
	}
}

static void Jedi_TimersApply( void )
{
	//use careful anim/slower movement if not already moving
	if ( !ucmd.forwardmove && !TIMER_Done( NPC, "walking" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( NPC, "taunting" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !ucmd.rightmove )
	{//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if ( !TIMER_Done( NPC, "strafeLeft" ) )
		{
			if ( NPCInfo->desiredYaw > NPC->client->ps.viewangles[YAW] + 60 )
			{//we want to turn left, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				ucmd.rightmove = -127;
				VectorClear( NPC->client->ps.moveDir );
			}
		}
		else if ( !TIMER_Done( NPC, "strafeRight" ) )
		{
			if ( NPCInfo->desiredYaw < NPC->client->ps.viewangles[YAW] - 60 )
			{//we want to turn right, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				ucmd.rightmove = 127;
				VectorClear( NPC->client->ps.moveDir );
			}
		}
	}

	Jedi_DebounceDirectionChanges();

	if ( !TIMER_Done( NPC, "gripping" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCEGRIP;
	}

	if ( !TIMER_Done( NPC, "draining" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if ( !TIMER_Done( NPC, "holdLightning" ) )
	{//hold down the lightning key
		ucmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate( int enemy_dist )
{
	if ( Jedi_CultistDestroyer( NPC ) )
	{
		Jedi_Aggression( NPC, 5 );
		return;
	}
//===START MISSING CODE=================================================================
	if ( TIMER_Done( NPC, "roamTime" ) )
	{
		TIMER_Set( NPC, "roamTime", Q_irand( 2000, 5000 ) );
		//okay, now mess with agression
		if ( NPC->enemy && NPC->enemy->client )
		{
			switch( NPC->enemy->client->ps.weapon )
			{
			//FIXME: add new weapons
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if ( !NPC->enemy->client->ps.SaberActive() )
				{//fool!  Standing around unarmed, charge!
					//Com_Printf( "(%d) raise agg - enemy saber off\n", level.time );
					Jedi_Aggression( NPC, 2 );
				}
				else
				{
					//Com_Printf( "(%d) raise agg - enemy saber\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				break;
			case WP_BLASTER:
			case WP_BRYAR_PISTOL:
			case WP_BLASTER_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
			case WP_CONCUSSION:
				//if he has a blaster, move in when:
				//They're not shooting at me
				if ( NPC->enemy->attackDebounceTime < level.time )
				{//does this apply to players?
					//Com_Printf( "(%d) raise agg - enemy not shooting ranged weap\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				//He's closer than a dist that gives us time to deflect
				if ( enemy_dist < 256 )
				{
					//Com_Printf( "(%d) raise agg - enemy ranged weap- too close\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				break;
			default:
				break;
			}
		}
	}

	if ( TIMER_Done( NPC, "noStrafe" ) && TIMER_Done( NPC, "strafeLeft" ) && TIMER_Done( NPC, "strafeRight" ) )
	{
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if ( !Q_irand( 0, 4 ) )
		{//start a strafe
			if ( Jedi_Strafe( 1000, 3000, 0, 4000, qtrue ) )
			{
				if ( d_JediAI->integer )
				{
					gi.Printf( "off strafe\n" );
				}
			}
		}
		else
		{//postpone any strafing for a while
			TIMER_Set( NPC, "noStrafe", Q_irand( 1000, 3000 ) );
		}
	}
//===END MISSING CODE=================================================================
	if ( NPC->client->ps.saberEventFlags )
	{//some kind of saber combat event is still pending
		int newFlags = NPC->client->ps.saberEventFlags;
		if ( NPC->client->ps.saberEventFlags&SEF_PARRIED )
		{//parried
			TIMER_Set( NPC, "parryTime", -1 );
			/*
			if ( NPCInfo->rank >= RANK_LT_JG )
			{
				NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 100;
			}
			else
			{
				NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			*/
			if ( NPC->enemy && (!NPC->enemy->client||PM_SaberInKnockaway( NPC->enemy->client->ps.saberMove )) )
			{//advance!
				Jedi_Aggression( NPC, 1 );//get closer
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel-1) );//use a faster attack
			}
			else
			{
				if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we parried\n", level.time );
					Jedi_Aggression( NPC, -1 );
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel-1) );
				}
			}
			if ( d_JediAI->integer )
			{
				gi.Printf( "(%d) PARRY: agg %d, no parry until %d\n", level.time, NPCInfo->stats.aggression, level.time + 100 );
			}
			newFlags &= ~SEF_PARRIED;
		}
		if ( !NPC->client->ps.weaponTime && (NPC->client->ps.saberEventFlags&SEF_HITENEMY) )//hit enemy
		{//we hit our enemy last time we swung, drop our aggression
			if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
			{
				//Com_Printf( "(%d) drop agg - we hit enemy\n", level.time );
				Jedi_Aggression( NPC, -1 );
				if ( d_JediAI->integer )
				{
					gi.Printf( "(%d) HIT: agg %d\n", level.time, NPCInfo->stats.aggression );
				}
				if ( !Q_irand( 0, 3 )
					&& NPCInfo->blockedSpeechDebounceTime < level.time
					&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time
					&& NPC->painDebounceTime < level.time - 1000 )
				{
					G_AddVoiceEvent( NPC, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 3000 );
					jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				}
			}
			if ( !Q_irand( 0, 2 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel+1) );
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if ( (NPC->client->ps.saberEventFlags&SEF_BLOCKED) )
		{//was blocked whilst attacking
			if ( PM_SaberInBrokenParry( NPC->client->ps.saberMove )
				|| NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
			{
				//Com_Printf( "(%d) drop agg - we were knock-blocked\n", level.time );
				if ( NPC->client->ps.saberInFlight )
				{//lost our saber, too!!!
					Jedi_Aggression( NPC, -5 );//really really really should back off!!!
				}
				else
				{
					Jedi_Aggression( NPC, -2 );//really should back off!
				}
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel+1) );//use a stronger attack
				if ( d_JediAI->integer )
				{
					gi.Printf( "(%d) KNOCK-BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression );
				}
			}
			else
			{
				if ( !Q_irand( 0, 2 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we were blocked\n", level.time );
					Jedi_Aggression( NPC, -1 );
					if ( d_JediAI->integer )
					{
						gi.Printf( "(%d) BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression );
					}
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel+1) );
				}
			}
			newFlags &= ~SEF_BLOCKED;
			//FIXME: based on the type of parry the enemy is doing and my skill,
			//		choose an attack that is likely to get around the parry?
			//		right now that's generic in the saber animation code, auto-picks
			//		a next anim for me, but really should be AI-controlled.
		}
		if ( NPC->client->ps.saberEventFlags&SEF_DEFLECTED )
		{//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel-1) );
			}
		}
		if ( NPC->client->ps.saberEventFlags&SEF_HITWALL )
		{//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if ( NPC->client->ps.saberEventFlags&SEF_HITOBJECT )
		{//hit some other damagable object
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.saberAnimLevel-1) );
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		NPC->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle( int enemy_dist )
{
	if ( !TIMER_Done( NPC, "parryTime" ) )
	{
		return;
	}
	if ( NPC->client->ps.saberInFlight )
	{//don't do this idle stuff if throwing saber
		return;
	}
	if ( NPC->client->ps.forcePowersActive&(1<<FP_RAGE)
		|| NPC->client->ps.forceRageRecoveryTime > level.time )
	{//never taunt while raging or recovering from rage
		return;
	}
	if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) )
	{//never taunt when holding scepter
		return;
	}
	if ( NPC->client->ps.saber[0].type == SABER_SITH_SWORD )
	{//never taunt when holding sith sword
		return;
	}
	//FIXME: make these distance numbers defines?
	if ( enemy_dist >= 64 )
	{//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		int chance = 20;
		if ( NPC->client->NPC_class == CLASS_SHADOWTROOPER )
		{
			chance = 10;
		}
		//FIXME: possibly throw local objects at enemy?
		if ( Q_irand( 2, chance ) < NPCInfo->stats.aggression )
		{
			if ( TIMER_Done( NPC, "chatter" ) )
			{//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if ( enemy_dist > 200
					&& NPC->client->NPC_class != CLASS_BOBAFETT
					&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
					&& NPC->client->NPC_class != CLASS_ROCKETTROOPER
					&& NPC->client->ps.SaberActive()
					&& !Q_irand( 0, 5 ) )
				{//taunt even more, turn off the saber
					//FIXME: don't do this if health low?
					if ( NPC->client->ps.saberAnimLevel != SS_STAFF
						&& NPC->client->ps.saberAnimLevel != SS_DUAL )
					{//those taunts leave saber on
						WP_DeactivateSaber( NPC );
					}
					//Don't attack for a bit
					NPCInfo->stats.aggression = 3;
					//FIXME: maybe start strafing?
					//debounce this
					if ( NPC->client->playerTeam != TEAM_PLAYER && !Q_irand( 0, 1 ))
					{
						NPC->client->ps.taunting = level.time + 100;
						TIMER_Set( NPC, "chatter", Q_irand( 5000, 10000 ) );
						TIMER_Set( NPC, "taunting", 5500 );
					}
					else
					{
						Jedi_BattleTaunt();
						TIMER_Set( NPC, "taunting", Q_irand( 5000, 10000 ) );
					}
				}
				else if ( Jedi_BattleTaunt() )
				{//FIXME: pick some anims
				}
			}
		}
	}
}

extern qboolean PM_SaberInParry( int move );
static qboolean Jedi_AttackDecide( int enemy_dist )
{
	if ( !TIMER_Done( NPC, "allyJediDelay" ) )
	{
		return qfalse;
	}

	if ( Jedi_CultistDestroyer( NPC ) )
	{//destroyer
		if ( enemy_dist <= 32 )
		{//go boom!
			//float?
			//VectorClear( NPC->client->ps.velocity );
			//NPC->client->ps.gravity = 0;
			//NPC->svFlags |= SVF_CUSTOM_GRAVITY;
			//NPC->client->moveType = MT_FLYSWIM;
			//NPC->flags |= FL_NO_KNOCKBACK;
			NPC->flags |= FL_GODMODE;
			NPC->takedamage = qfalse;

			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			NPC->client->ps.forcePowersActive |= ( 1 << FP_RAGE );
			NPC->painDebounceTime = NPC->useDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			return qtrue;
		}
		return qfalse;
	}
	if ( NPC->enemy->client
		&& NPC->enemy->s.weapon == WP_SABER
		&& NPC->enemy->client->ps.saberLockTime > level.time
		&& NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		return qfalse;
	}

	if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage with an attack!
		int	chance = 0;
		if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
		{//desann and luke
			chance = 20;
		}
		else if ( NPC->client->NPC_class == CLASS_TAVION
			|| NPC->client->NPC_class == CLASS_ALORA )
		{//tavion
			chance = 10;
		}
		else if ( NPC->client->NPC_class == CLASS_SHADOWTROOPER )
		{//shadowtrooper
			chance = 5;
		}
		else if ( NPC->client->NPC_class == CLASS_REBORN && NPCInfo->rank == RANK_LT_JG )
		{//fencer
			chance = 5;
		}
		else
		{
			chance = NPCInfo->rank;
		}
		if ( Q_irand( 0, 30 ) < chance )
		{//based on skill with some randomness
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;//clear this now that we are using the opportunity
			TIMER_Set( NPC, "noRetreat", Q_irand( 500, 2000 ) );
			//FIXME: check enemy_dist?
			NPC->client->ps.weaponTime = NPCInfo->shotTime = NPC->attackDebounceTime = 0;
			//NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	if ( NPC->client->NPC_class == CLASS_TAVION ||
		NPC->client->NPC_class == CLASS_ALORA ||
		NPC->client->NPC_class == CLASS_SHADOWTROOPER ||
		( NPC->client->NPC_class == CLASS_REBORN && NPCInfo->rank == RANK_LT_JG ) ||
		( NPC->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER ) )
	{//tavion, fencers, jedi trainer are all good at following up a parry with an attack
		if ( ( PM_SaberInParry( NPC->client->ps.saberMove ) || PM_SaberInKnockaway( NPC->client->ps.saberMove ) )
			&& NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//try to attack straight from a parry
			NPC->client->ps.weaponTime = NPCInfo->shotTime = NPC->attackDebounceTime = 0;
			//NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			Jedi_AdjustSaberAnimLevel( NPC, SS_FAST );//try to follow-up with a quick attack
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	//try to hit them if we can
	if ( !enemy_in_striking_range )
	{
		return qfalse;
	}

	if ( !TIMER_Done( NPC, "parryTime" ) )
	{
		return qfalse;
	}

	if ( (NPCInfo->scriptFlags&SCF_DONT_FIRE) )
	{//not allowed to attack
		return qfalse;
	}

	if ( !(ucmd.buttons&BUTTON_ATTACK)
		&& !(ucmd.buttons&BUTTON_ALT_ATTACK)
		&& !(ucmd.buttons&BUTTON_FORCE_FOCUS) )
	{//not already attacking
		//Try to attack
		WeaponThink( qtrue );
	}

	//FIXME:  Maybe try to push enemy off a ledge?

	//close enough to step forward

	//FIXME: an attack debounce timer other than the phaser debounce time?
	//		or base it on aggression?

	if ( ucmd.buttons&BUTTON_ATTACK && !NPC_Jumping())
	{//attacking
		/*
		if ( enemy_dist > 32 && NPCInfo->stats.aggression >= 4 )
		{//move forward if we're too far away and we're chasing him
			ucmd.forwardmove = 127;
		}
		else if ( enemy_dist < 0 )
		{//move back if we're too close
			ucmd.forwardmove = -127;
		}
		*/
		//FIXME: based on the type of parry/attack the enemy is doing and my skill,
		//		choose an attack that is likely to get around the parry?
		//		right now that's generic in the saber animation code, auto-picks
		//		a next anim for me, but really should be AI-controlled.
		//FIXME: have this interact with/override above strafing code?
		if ( !ucmd.rightmove )
		{//not already strafing
			if ( !Q_irand( 0, 3 ) )
			{//25% chance of doing this
				vec3_t  right, dir2enemy;

				AngleVectors( NPC->currentAngles, NULL, right, NULL );
				VectorSubtract( NPC->enemy->currentOrigin, NPC->currentAngles, dir2enemy );
				if ( DotProduct( right, dir2enemy ) > 0 )
				{//he's to my right, strafe left
					ucmd.rightmove = -127;
					VectorClear( NPC->client->ps.moveDir );
				}
				else
				{//he's to my left, strafe right
					ucmd.rightmove = 127;
					VectorClear( NPC->client->ps.moveDir );
				}
			}
		}
		return qtrue;
	}

	return qfalse;
}


extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
extern qboolean PM_KickingAnim( int anim );
static void Jedi_CheckEnemyMovement( float enemy_dist )
{
	if ( !NPC->enemy || !NPC->enemy->client )
	{
		return;
	}

	if ( !(NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
	{
		if ( PM_KickingAnim( NPC->enemy->client->ps.legsAnim )
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			//FIXME: I'm relatively close to him
			&& (NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_RL
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_BF
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_S
				|| (NPC->enemy->enemy && NPC->enemy->enemy == NPC))
			)
		{//run into the kick!
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( NPC->enemy->client->ps.torsoAnim == BOTH_A7_HILT
				&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//run into the hilt bash
			//FIXME : only if in front!
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( (NPC->enemy->client->ps.torsoAnim == BOTH_A6_FB
					|| NPC->enemy->client->ps.torsoAnim == BOTH_A6_LR)
				&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//run into the attack
			//FIXME : only if on R/L or F/B?
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( NPC->enemy->enemy && NPC->enemy->enemy == NPC )
		{//enemy is mad at *me*
			if ( NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1
				|| NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN
				|| NPC->enemy->client->ps.legsAnim == BOTH_FLIP_ATTACK7 )
			{//enemy is flipping over me
				if ( Q_irand( 0, NPCInfo->rank ) < RANK_LT )
				{//be nice and stand still for him...
					ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
					VectorClear( NPC->client->ps.moveDir );
					NPC->client->ps.forceJumpCharge = 0;
					TIMER_Set( NPC, "strafeLeft", -1 );
					TIMER_Set( NPC, "strafeRight", -1 );
					TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
					TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
					TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
				}
			}
			else if ( NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP )
			{//he's flipping off a wall
				if ( NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
				{//still in air
					if ( enemy_dist < 256 )
					{//close
						if ( Q_irand( 0, NPCInfo->rank ) < RANK_LT )
						{//be nice and stand still for him...
							/*
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 200, 500 ) );
							*/
							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 250, 500 )*(3-g_spskill->integer) );

							vec3_t enemyFwd, dest, dir;

							VectorCopy( NPC->enemy->client->ps.velocity, enemyFwd );
							VectorNormalize( enemyFwd );
							VectorMA( NPC->enemy->currentOrigin, -64, enemyFwd, dest );
							VectorSubtract( dest, NPC->currentOrigin, dir );
							if ( VectorNormalize( dir ) > 32 )
							{
								G_UcmdMoveForDir( NPC, &ucmd, dir );
							}
							else
							{
								TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
			else if ( NPC->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 )
			{//he's stabbing backwards
				if ( enemy_dist < 256 && enemy_dist > 64 )
				{//close
					if ( !InFront( NPC->currentOrigin, NPC->enemy->currentOrigin, NPC->enemy->currentAngles, 0.0f ) )
					{//behind him
						if ( !Q_irand( 0, NPCInfo->rank ) )
						{//be nice and stand still for him...
							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );

							vec3_t enemyFwd, dest, dir;

							AngleVectors( NPC->enemy->currentAngles, enemyFwd, NULL, NULL );
							VectorMA( NPC->enemy->currentOrigin, -32, enemyFwd, dest );
							VectorSubtract( dest, NPC->currentOrigin, dir );
							if ( VectorNormalize( dir ) > 64 )
							{
								G_UcmdMoveForDir( NPC, &ucmd, dir );
							}
							else
							{
								TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
		}
	}
	//FIXME: also:
	//		If enemy doing wall flip, keep running forward
	//		If enemy doing back-attack and we're behind him keep running forward toward his back, don't strafe
}

static void Jedi_CheckJumps( void )
{
	if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		NPC->client->ps.forceJumpCharge = 0;
		ucmd.upmove = 0;
		return;
	}
	//FIXME: should probably check this before AI decides that best move is to jump?  Otherwise, they may end up just standing there and looking dumb
	//FIXME: all this work and he still jumps off ledges... *sigh*... need CONTENTS_BOTCLIP do-not-enter brushes...?
	vec3_t	jumpVel = {0,0,0};

	if ( NPC->client->ps.forceJumpCharge )
	{
		//gi.Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump( NPC, jumpVel, &ucmd );
	}
	else if ( ucmd.upmove > 0 )
	{
		//gi.Printf( "(%d) regular jump\n", level.time );
		VectorCopy( NPC->client->ps.velocity, jumpVel );
		jumpVel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( !jumpVel[0] && !jumpVel[1] )//FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{//we assume a jump straight up is safe
		//gi.Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}
	//Now predict where this is going
	//in steps, keep evaluating the trajectory until the new z pos is <= than current z pos, trace down from there
	trace_t	trace;
	trajectory_t	tr;
	vec3_t	lastPos, testPos, bottom;
	int		elapsedTime;

	VectorCopy( NPC->currentOrigin, tr.trBase );
	VectorCopy( jumpVel, tr.trDelta );
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy( NPC->currentOrigin, lastPos );

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for ( elapsedTime = 500; elapsedTime <= 4000; elapsedTime += 500 )
	{
		EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if ( testPos[2] < lastPos[2] )
		{//going down, don't check for BOTCLIP
			gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );//FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{//going up, check for BOTCLIP
			gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP, (EG2_Collision)0, 0 );
		}
		if ( trace.allsolid || trace.startsolid )
		{//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
			return;
		}
		if ( trace.fraction < 1.0f )
		{//hit something
			if ( trace.contents & CONTENTS_BOTCLIP )
			{//hit a do-not-enter brush
				goto jump_unsafe;
				return;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy( testPos, lastPos );
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy( trace.endpos, bottom );
	if ( bottom[2] > NPC->currentOrigin[2] )
	{//only care about dist down from current height or lower
		bottom[2] = NPC->currentOrigin[2];
	}
	else if ( NPC->currentOrigin[2] - bottom[2] > 400 )
	{//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
		return;
	}
	bottom[2] -= 128;
	gi.trace( &trace, trace.endpos, NPC->mins, NPC->maxs, bottom, NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0f )
	{//hit ground!
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//landed on an ent
			gentity_t *groundEnt = &g_entities[trace.entityNum];
			if ( groundEnt->svFlags&SVF_GLASS_BRUSH )
			{//don't land on breakable glass!
				goto jump_unsafe;
				return;
			}
		}
		//gi.Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	//gi.Printf( "(%d) unsafe jump cleared\n", level.time );
	NPC->client->ps.forceJumpCharge = 0;
	ucmd.upmove = 0;
}

extern void RT_JetPackEffect( int duration );
void RT_CheckJump( void )
{
	int jumpEntNum = ENTITYNUM_NONE;
	vec3_t	jumpPos = {0,0,0};

	if ( !NPCInfo->goalEntity )
	{
		if ( NPC->enemy )
		{
			//FIXME: debounce this?
			if ( TIMER_Done( NPC, "roamTime" )
				&& Q_irand( 0, 9 ) )
			{//okay to try to find another spot to be
				int cpFlags = (CP_CLEAR|CP_HAS_ROUTE);//must have a clear shot at enemy
				float enemyDistSq = DistanceHorizontalSquared( NPC->currentOrigin, NPC->enemy->currentOrigin );
				//FIXME: base these ranges on weapon
				if ( enemyDistSq > (2048*2048) )
				{//hmm, close in?
					cpFlags |= CP_APPROACH_ENEMY;
				}
				else if ( enemyDistSq < (256*256) )
				{//back off!
					cpFlags |= CP_RETREAT;
				}
				int sendFlags = cpFlags;
				int cp = NPC_FindCombatPointRetry( NPC->currentOrigin,
											NPC->currentOrigin,
											NPC->currentOrigin,
											&sendFlags,
											256,
											NPCInfo->lastFailedCombatPoint );
				if ( cp == -1 )
				{//try again, no route needed since we can rocket-jump to it!
					cpFlags &= ~CP_HAS_ROUTE;
					cp = NPC_FindCombatPointRetry( NPC->currentOrigin,
												NPC->currentOrigin,
												NPC->currentOrigin,
												&cpFlags,
												256,
												NPCInfo->lastFailedCombatPoint );
				}
				if ( cp != -1 )
				{
					NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp );
				}
				else
				{//FIXME: okay to do this if have good close-range weapon...
					//FIXME: should we really try to go right for him?!
					//NPCInfo->goalEntity = NPC->enemy;
					jumpEntNum = NPC->enemy->s.number;
					VectorCopy( NPC->enemy->currentOrigin, jumpPos );
					//return;
				}
				TIMER_Set( NPC, "roamTime", Q_irand( 3000, 12000 ) );
			}
			else
			{//FIXME: okay to do this if have good close-range weapon...
				//FIXME: should we really try to go right for him?!
				//NPCInfo->goalEntity = NPC->enemy;
				jumpEntNum = NPC->enemy->s.number;
				VectorCopy( NPC->enemy->currentOrigin, jumpPos );
				//return;
			}
		}
		else
		{
			return;
		}
	}
	else
	{
		jumpEntNum = NPCInfo->goalEntity->s.number;
		VectorCopy( NPCInfo->goalEntity->currentOrigin, jumpPos );
	}
	vec3_t vec2Goal;
	VectorSubtract( jumpPos, NPC->currentOrigin, vec2Goal );
	if ( fabs( vec2Goal[2] ) < 32 )
	{//not a big height diff, see how far it is
		vec2Goal[2] = 0;
		if ( VectorLengthSquared( vec2Goal ) < (256*256) )
		{//too close!  Don't rocket-jump to it...
			return;
		}
	}
	//If we can't get straight at him
	if ( !Jedi_ClearPathToSpot( jumpPos, jumpEntNum ) )
	{//hunt him down
		if ( (NPC_ClearLOS( NPC->enemy )||NPCInfo->enemyLastSeenTime>level.time-500)
			&& InFOV( jumpPos, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 60 ) )
		{
			if ( NPC_TryJump( jumpPos ) )	// Rocket Trooper
			{//just do the jetpack effect for a litte bit
				RT_JetPackEffect( Q_irand( 800, 1500) );
				return;
			}
		}

		if ( Jedi_Hunt() && !(NPCInfo->aiFlags&NPCAI_BLOCKED) )//FIXME: have to do this because they can ping-pong forever
		{//can macro-navigate to him
			return;
		}
		else
		{//FIXME: try to find a waypoint that can see enemy, jump from there
			if ( STEER::HasBeenBlockedFor(NPC, 2000) )
			{//try to jump to the blockedTargetPosition
				if ( NPC_TryJump(NPCInfo->blockedTargetPosition) )	// Rocket Trooper
				{//just do the jetpack effect for a litte bit
					RT_JetPackEffect( Q_irand( 800, 1500) );
				}
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
static void Jedi_Combat( void )
{
	vec3_t	enemy_dir, enemy_movedir, enemy_dest;
	float	enemy_dist, enemy_movespeed;
	trace_t	trace;

	//See where enemy will be 300 ms from now
	Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );

	if ( NPC_Jumping( ) )
	{//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide( enemy_dist );
		return;
	}

	if ( TIMER_Done( NPC, "allyJediDelay" ) )
	{
		if ( !(NPC->client->ps.forcePowersActive&(1<<FP_GRIP)) || NPC->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
		{//not gripping
			//If we can't get straight at him
			if ( !Jedi_ClearPathToSpot( enemy_dest, NPC->enemy->s.number ) )
			{//hunt him down
				//gi.Printf( "No Clear Path\n" );
				if ( (NPC_ClearLOS( NPC->enemy )||NPCInfo->enemyLastSeenTime>level.time-500) && NPC_FaceEnemy( qtrue ) )//( NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) &&
				{
					//try to jump to him?
					/*
					vec3_t end;
					VectorCopy( NPC->currentOrigin, end );
					end[2] += 36;
					gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
					if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0 )
					{
						vec3_t angles, forward;
						VectorCopy( NPC->client->ps.viewangles, angles );
						angles[0] = 0;
						AngleVectors( angles, forward, NULL, NULL );
						VectorMA( end, 64, forward, end );
						gi.trace( &trace, NPC->currentOrigin, NPC->mins, NPC->maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
						if ( !trace.allsolid && !trace.startsolid )
						{
							if ( trace.fraction >= 1.0 || trace.plane.normal[2] > 0 )
							{
								ucmd.upmove = 127;
								ucmd.forwardmove = 127;
								return;
							}
						}
					}
					*/
					//FIXME: about every 1 second calc a velocity,
					//run a loop of traces with evaluate trajectory
					//for gravity with my size, see if it makes it...
					//this will also catch misacalculations that send you off ledges!
					//gi.Printf( "Considering Jump\n" );
					if (NPC->client && NPC->client->NPC_class==CLASS_BOBAFETT)
					{
						Boba_FireDecide();
					}
				/*	else if ( NPC_TryJump( NPC->enemy ) )	// Jedi, can see enemy, but can't get to him
					{//FIXME: what about jumping to his enemyLastSeenLocation?
						return;
					}*/
				}

				//Check for evasion
				if ( TIMER_Done( NPC, "parryTime" ) )
				{//finished parrying
					if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
						NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
					{//wasn't blocked myself
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
					}
				}
				if ( Jedi_Hunt() && !(NPCInfo->aiFlags&NPCAI_BLOCKED) )//FIXME: have to do this because they can ping-pong forever
				{//can macro-navigate to him
					if ( enemy_dist < 384 && !Q_irand( 0, 10 ) && NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && !NPC_ClearLOS( NPC->enemy ) )
					{
						G_AddVoiceEvent( NPC, Q_irand( EV_JLOST1, EV_JLOST3 ), 3000 );
						jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
					}
					if (NPC->client && NPC->client->NPC_class==CLASS_BOBAFETT)
					{
						Boba_FireDecide();
					}

					return;
				}
				//well, try to head for his last seen location
		/*
				else if ( Jedi_Track() )
				{
					return;
				}
		*/		else
				{//FIXME: try to find a waypoint that can see enemy, jump from there
				/*	if ( STEER::HasBeenBlockedFor(NPC, 3000) )
					{//try to jump to the blockedDest
						if (NPCInfo->blockedTargetEntity)
						{
							NPC_TryJump(NPCInfo->blockedTargetEntity);	// commented Out
						}
						else
						{
							NPC_TryJump(NPCInfo->blockedTargetPosition);// commented Out
						}
					}*/

				}
			}
		}
		//else, we can see him or we can't track him at all

		//every few seconds, decide if we should we advance or retreat?
		Jedi_CombatTimersUpdate( enemy_dist );

		//We call this even if lost enemy to keep him moving and to update the taunting behavior
		//maintain a distance from enemy appropriate for our aggression level
		Jedi_CombatDistance( enemy_dist );
	}

	//if ( !enemy_lost )
 	if (NPC->client->NPC_class != CLASS_BOBAFETT)
	{
		//Update our seen enemy position
		if ( !NPC->enemy->client || ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && NPC->client->ps.groundEntityNum != ENTITYNUM_NONE ) )
		{
			VectorCopy( NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation );
		}
		NPCInfo->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	if ( TIMER_Done( NPC, "noturn" ) && !NPC_Jumping() )
	{
		Jedi_FaceEnemy( qtrue );
	}
	NPC_UpdateAngles( qtrue, qtrue );

	//Check for evasion
	if ( TIMER_Done( NPC, "parryTime" ) )
	{//finished parrying
		if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
	if ( NPC->enemy->s.weapon == WP_SABER )
	{
		Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
	}
	else
	{//do we need to do any evasion for other kinds of enemies?
	}

	//apply strafing/walking timers, etc.
	Jedi_TimersApply();

	if ( TIMER_Done( NPC, "allyJediDelay" ) )
	{
		if ( ( !NPC->client->ps.saberInFlight || (NPC->client->ps.saberAnimLevel == SS_DUAL && NPC->client->ps.saber[1].Active()) )
			&& (!(NPC->client->ps.forcePowersActive&(1<<FP_GRIP))||NPC->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2) )
		{//not throwing saber or using force grip
			//see if we can attack
			if ( !Jedi_AttackDecide( enemy_dist ) )
			{//we're not attacking, decide what else to do
				Jedi_CombatIdle( enemy_dist );
				//FIXME: lower aggression when actually strike offensively?  Or just when do damage?
			}
			else
			{//we are attacking
				//stop taunting
				TIMER_Set( NPC, "taunting", -level.time );
			}
		}
		else
		{
		}
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			Boba_FireDecide();
		}
		else if ( NPC->client->NPC_class == CLASS_ROCKETTROOPER )
		{
			RT_FireDecide();
		}
	}

	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement( enemy_dist );
	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps();
	//Just make sure we don't strafe into walls or off cliffs

	if ( VectorCompare( NPC->client->ps.moveDir, vec3_origin )//stomped the NAV system's moveDir
		&& !NPC_MoveDirClear( ucmd.forwardmove, ucmd.rightmove, qtrue ) )//check ucmd-driven movement
	{//uh-oh, we are going to fall or hit something
		/*
		navInfo_t	info;
		//Get the move info
		NAV_GetLastMove( info );
		if ( !(info.flags & NIF_MACRO_NAV) )
		{//micro-navigation told us to step off a ledge, try using macronav for now
			NPC_MoveToGoal( qfalse );
		}
		*/
		//reset the timers.
		TIMER_Set( NPC, "strafeLeft", 0 );
		TIMER_Set( NPC, "strafeRight", 0 );
	}
}


/*
==========================================================================================
EXTERNALLY CALLED BEHAVIOR STATES
==========================================================================================
*/

/*
-------------------------
NPC_Jedi_Pain
-------------------------
*/

void NPC_Jedi_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	//FIXME: base the actual aggression add/subtract on health?
	//FIXME: don't do this more than once per frame?
	//FIXME: when take pain, stop force gripping....?
	if ( other->s.weapon == WP_SABER )
	{//back off
		TIMER_Set( self, "parryTime", -1 );
		if ( self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) )
		{//less for Desann
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill->integer)*50;
		}
		else if ( self->NPC->rank >= RANK_LT_JG )
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill->integer)*100;//300
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill->integer)*200;//500
		}
		if ( !Q_irand( 0, 3 ) )
		{//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel( self, Q_irand( SS_FAST, SS_STRONG ) );
		}
		if ( !Q_irand( 0, 1 ) )//damage > 20 || self->health < 40 ||
		{
			//Com_Printf( "(%d) drop agg - hit by saber\n", level.time );
			Jedi_Aggression( self, -1 );
		}
		if ( d_JediAI->integer )
		{
			gi.Printf( "(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression, level.time+500 );
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if ( d_JediAI->integer )
		{
			vec3_t	diff, fwdangles, right;

			VectorSubtract( point, self->client->renderInfo.eyePoint, diff );
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors( fwdangles, NULL, right, NULL );
			float rightdot = DotProduct(right, diff);
			float zdiff = point[2] - self->client->renderInfo.eyePoint[2];

			gi.Printf( "(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, point[2]-self->absmin[2],zdiff,rightdot);
		}
	}
	else
	{//attack
		//Com_Printf( "(%d) raise agg - hit by ranged\n", level.time );
		Jedi_Aggression( self, 1 );
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop( self, FP_GRIP );

	NPC_Pain( self, inflictor, other, point, damage, mod );

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	}

	//drop me from the ceiling if I'm on it
	if ( Jedi_WaitingAmbush( self ) )
	{
		self->client->noclip = false;
	}
	if ( self->client->ps.legsAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	if ( self->client->ps.torsoAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}

	//check special defenses
	if ( other
		&& other->client
		&& !OnSameTeam( self, other ))
	{//hit by a client
		//FIXME: delay this until *after* the pain anim?
		if ( mod == MOD_FORCE_GRIP
			|| mod == MOD_FORCE_LIGHTNING
			|| mod == MOD_FORCE_DRAIN )
		{//see if we should turn on absorb
			if ( (self->client->ps.forcePowersKnown&(1<<FP_ABSORB)) != 0
				&& (self->client->ps.forcePowersActive&(1<<FP_ABSORB)) == 0 )
			{//know absorb and not already using it
				if ( other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand( 0, g_spskill->integer+1 ) )//enemy is player
				{
					if ( Q_irand( 0, self->NPC->rank ) > RANK_ENSIGN )
					{
						if ( !Q_irand( 0, 5 ) )
						{
							ForceAbsorb( self );
						}
					}
				}
			}
		}
		else if ( damage > Q_irand( 5, 20 ) )
		{//respectable amount of normal damage
			if ( (self->client->ps.forcePowersKnown&(1<<FP_PROTECT)) != 0
				&& (self->client->ps.forcePowersActive&(1<<FP_PROTECT)) == 0 )
			{//know protect and not already using it
				if ( other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand( 0, g_spskill->integer+1 ) )//enemy is player
				{
					if ( Q_irand( 0, self->NPC->rank ) > RANK_ENSIGN )
					{
						if ( !Q_irand( 0, 1 ) )
						{
							if ( other->s.number < MAX_CLIENTS
								&& ((self->NPC->aiFlags&NPCAI_BOSS_CHARACTER)
									|| self->client->NPC_class==CLASS_SHADOWTROOPER)
								&& Q_irand(0, 6-g_spskill->integer) )
							{
							}
							else
							{
								ForceProtect( self );
							}
						}
					}
				}
			}
		}
	}
}

qboolean Jedi_CheckDanger( void )
{
	int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue );
	if ( level.alertEvents[alertEvent].level >= AEL_DANGER )
	{//run away!
		if ( !level.alertEvents[alertEvent].owner
			|| !level.alertEvents[alertEvent].owner->client
			|| (level.alertEvents[alertEvent].owner!=NPC&&level.alertEvents[alertEvent].owner->client->playerTeam!=NPC->client->playerTeam) )
		{//no owner
			return qfalse;
		}
		G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}
	return qfalse;
}

extern int g_crosshairEntNum;
qboolean Jedi_CheckAmbushPlayer( void )
{
	if ( !player || !player->client )
	{
		return qfalse;
	}

	if ( !NPC_ValidEnemy( player ) )
	{
		return qfalse;
	}

	if ( NPC->client->ps.powerups[PW_CLOAKED] || g_crosshairEntNum != NPC->s.number )
	{//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
		if ( !gi.inPVS( player->currentOrigin, NPC->currentOrigin ) )
		{//must be in same room
			return qfalse;
		}
		else
		{
			if ( !NPC->client->ps.powerups[PW_CLOAKED] )
			{
				NPC_SetLookTarget( NPC, 0, 0 );
			}
		}
		float target_dist, zDiff = NPC->currentOrigin[2]-player->currentOrigin[2];
		if ( zDiff <= 0 || zDiff > 512 )
		{//never ambush if they're above me or way way below me
			return qfalse;
		}

		//If the target is this close, then wake up regardless
		if ( (target_dist = DistanceHorizontalSquared( player->currentOrigin, NPC->currentOrigin )) > 4096 )
		{//closer than 64 - always ambush
			if ( target_dist > 147456 )
			{//> 384, not close enough to ambush
				return qfalse;
			}
			//Check FOV first
			if ( NPC->client->ps.powerups[PW_CLOAKED] )
			{
				if ( InFOV( player, NPC, 30, 90 ) == qfalse )
				{
					return qfalse;
				}
			}
			else
			{
				if ( InFOV( player, NPC, 45, 90 ) == qfalse )
				{
					return qfalse;
				}
			}
		}

		if ( !NPC_ClearLOS( player ) )
		{
			return qfalse;
		}
	}

	G_SetEnemy( NPC, player );
	NPCInfo->enemyLastSeenTime = level.time;
	TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
	return qtrue;
}

void Jedi_Ambush( gentity_t *self )
{
	self->client->noclip = false;
	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
	if ( self->client->NPC_class != CLASS_BOBAFETT
		&& self->client->NPC_class != CLASS_ROCKETTROOPER )
	{
		self->client->ps.SaberActivate();
	}
	Jedi_Decloak( self );
	G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 1000 );
}

qboolean Jedi_WaitingAmbush( gentity_t *self )
{
	if ( (self->spawnflags&JSF_AMBUSH) && self->client->noclip )
	{
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Jedi_Patrol
-------------------------
*/

static void Jedi_Patrol( void )
{
	NPC->client->ps.saberBlocked = BLOCKED_NONE;

	if ( Jedi_WaitingAmbush( NPC ) )
	{//hiding on the ceiling
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{//look for enemies
			if ( Jedi_CheckAmbushPlayer() || Jedi_CheckDanger() )
			{//found him!
				Jedi_Ambush( NPC );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}
	else if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )//NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{//look for enemies
		gentity_t *best_enemy = NULL;
		float	best_enemy_dist = Q3_INFINITE;
		for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
		{
			gentity_t *enemy = &g_entities[i];
			float	enemy_dist;
			if ( enemy && enemy->client && NPC_ValidEnemy( enemy ))
			{
				if ( gi.inPVS( NPC->currentOrigin, enemy->currentOrigin ) )
				{//we could potentially see him
					enemy_dist = DistanceSquared( NPC->currentOrigin, enemy->currentOrigin );
					if ( enemy->s.number == 0 || enemy_dist < best_enemy_dist )
					{
						//if the enemy is close enough, or threw his saber, take him as the enemy
						//FIXME: what if he throws a thermal detonator?
						//FIXME: use jediSpeechDebounceTime[NPC->client->playerTeam] < level.time ) check for anger sound
						if ( enemy_dist < (220*220) || ( NPCInfo->investigateCount>= 3 && NPC->client->ps.SaberActive() ) )
						{
							G_SetEnemy( NPC, enemy );
							//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
							NPCInfo->stats.aggression = 3;
							break;
						}
						else if ( enemy->client->ps.saberInFlight && enemy->client->ps.SaberActive() )
						{//threw his saber, see if it's heading toward me and close enough to consider a threat
							float	saberDist;
							vec3_t	saberDir2Me;
							vec3_t	saberMoveDir;
							gentity_t *saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract( NPC->currentOrigin, saber->currentOrigin, saberDir2Me );
							saberDist = VectorNormalize( saberDir2Me );
							VectorCopy( saber->s.pos.trDelta, saberMoveDir );
							VectorNormalize( saberMoveDir );
							if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
							{//it's heading towards me
								if ( saberDist < 200 )
								{//incoming!
									G_SetEnemy( NPC, enemy );
									//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
									NPCInfo->stats.aggression = 3;
									break;
								}
							}
						}
						best_enemy_dist = enemy_dist;
						best_enemy = enemy;
					}
				}
			}
		}
		if ( !NPC->enemy )
		{//still not mad
			if ( !best_enemy )
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(-1);
				//FIXME: what about alerts?  But not if ignore alerts
			}
			else
			{//have one to consider
				if ( NPC_ClearLOS( best_enemy ) )
				{//we have a clear (of architecture) LOS to him
					if ( (NPCInfo->aiFlags&NPCAI_NO_JEDI_DELAY) )
					{//just get mad right away
						if ( DistanceHorizontalSquared( NPC->currentOrigin, best_enemy->currentOrigin ) < (1024*1024) )
						{
							G_SetEnemy( NPC, best_enemy );
							NPCInfo->stats.aggression = 20;
						}
					}
					else if ( best_enemy->s.number )
					{//just attack
						G_SetEnemy( NPC, best_enemy );
						NPCInfo->stats.aggression = 3;
					}
					else if ( NPC->client->NPC_class != CLASS_BOBAFETT )
					{//the player, toy with him
						//get progressively more interested over time
						if ( TIMER_Done( NPC, "watchTime" ) )
						{//we want to pick him up in stages
							if ( TIMER_Get( NPC, "watchTime" ) == -1 )
							{//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set( NPC, "watchTime", Q_irand( 3000, 5000 ) );
								goto finish;
							}
							else
							{//okay, we've ignored him, now start to notice him
								if ( !NPCInfo->investigateCount )
								{
									G_AddVoiceEvent( NPC, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 3000 );
								}
								NPCInfo->investigateCount++;
								TIMER_Set( NPC, "watchTime", Q_irand( 4000, 10000 ) );
							}
						}
						//while we're waiting, do what we need to do
						if ( best_enemy_dist < (440*440) || NPCInfo->investigateCount >= 2 )
						{//stage three: keep facing him
							NPC_FaceEntity( best_enemy, qtrue );
							if ( best_enemy_dist < (330*330) )
							{//stage four: turn on the saber
								if ( !NPC->client->ps.saberInFlight )
								{
									NPC->client->ps.SaberActivate();
								}
							}
						}
						else if ( best_enemy_dist < (550*550) || NPCInfo->investigateCount == 1 )
						{//stage two: stop and face him every now and then
							if ( TIMER_Done( NPC, "watchTime" ) )
							{
								NPC_FaceEntity( best_enemy, qtrue );
							}
						}
						else
						{//stage one: look at him.
							NPC_SetLookTarget( NPC, best_enemy->s.number, 0 );
						}
					}
				}
				else if ( TIMER_Done( NPC, "watchTime" ) )
				{//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget( NPC );
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPC->enemy )
	{//just picked one up
		NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
	}
}

qboolean Jedi_CanPullBackSaber( gentity_t *self )
{
	if ( self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done( self, "parryTime" ) )
	{
		return qfalse;
	}

	if ( self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_ALORA
		|| ( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) ) )
	{
		return qtrue;
	}

	if ( self->painDebounceTime > level.time )//|| self->client->ps.weaponTime > 0 )
	{
		return qfalse;
	}

	return qtrue;
}
/*
-------------------------
NPC_BSJedi_FollowLeader
-------------------------
*/
extern qboolean NAV_CheckAhead( gentity_t *self, vec3_t end, trace_t &trace, int clipmask );

void NPC_BSJedi_FollowLeader( void )
{
	NPC->client->ps.saberBlocked = BLOCKED_NONE;
	if ( !NPC->enemy )
	{
		//Com_Printf( "(%d) drop agg - no enemy (follow)\n", level.time );
		Jedi_AggressionErosion(-1);
	}

	//did we drop our saber?  If so, go after it!
	if ( NPC->client->ps.saberInFlight )
	{//saber is not in hand
		if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			{//fell to the ground, try to pick it up...
				if ( Jedi_CanPullBackSaber( NPC ) )
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->ps.saberEntityNum];
					ucmd.buttons |= BUTTON_ATTACK;
					if ( NPC->enemy && NPC->enemy->health > 0 )
					{//get our saber back NOW!
						if ( !NPC_MoveToGoal( qtrue ) )//Jedi_Move( NPCInfo->goalEntity, qfalse );
						{//can't nav to it, try jumping to it
							NPC_FaceEntity( NPCInfo->goalEntity, qtrue );
							NPC_TryJump( NPCInfo->goalEntity );
						}
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
			}
		}
	}

	//try normal movement
	NPC_BSFollowLeader();


	if (!NPC->enemy &&
		NPC->health < NPC->max_health &&
		(NPC->client->ps.forcePowersKnown&(1<<FP_HEAL)) != 0  &&
        (NPC->client->ps.forcePowersActive&(1<<FP_HEAL)) == 0 &&
		TIMER_Done(NPC, "FollowHealDebouncer"))
	{
		if (Q_irand(0,3)==0)
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(12000, 18000));
			ForceHeal( NPC );
		}
		else
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(1000, 2000));
		}
	}

}

qboolean Jedi_CheckKataAttack( void )
{
	if ( NPCInfo->rank >= RANK_LT_COMM )
	{//only top-level guys and bosses do this
		if ( (ucmd.buttons&BUTTON_ATTACK) )
		{//attacking
			if ( (g_saberNewControlScheme->integer
				&& !(ucmd.buttons&BUTTON_FORCE_FOCUS) )
				||(!g_saberNewControlScheme->integer
				&& !(ucmd.buttons&BUTTON_ALT_ATTACK) ) )
			{//not already going to do a kata move somehow
				if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//on the ground
					if ( ucmd.upmove <= 0 && NPC->client->ps.forceJumpCharge <= 0 )
					{//not going to try to jump
						/*
						if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
						{//uh-oh, no jumping moves!
							if ( NPC->client->ps.saberAnimLevel == SS_STAFF )
							{//this kata move has a jump in it...
								return qfalse;
							}
						}
						*/

						if ( Q_irand( 0, g_spskill->integer+1 ) //50% chance on easy, 66% on medium, 75% on hard
							&& !Q_irand( 0, 9 ) )//10% chance overall
						{//base on skill level
							ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							if ( g_saberNewControlScheme->integer )
							{
								ucmd.buttons |= BUTTON_FORCE_FOCUS;
							}
							else
							{
								ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}


/*
-------------------------
Jedi_Attack
-------------------------
*/

static void Jedi_Attack( void )
{
	//Don't do anything if we're in a pain anim
	if ( NPC->painDebounceTime > level.time )
	{
		if ( Q_irand( 0, 1 ) )
		{
			Jedi_FaceEnemy( qtrue );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		if ( NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB )
		{//see if we grabbed enemy
			if ( NPC->client->ps.torsoAnimTimer <= 200 )
			{
				if ( Kyle_CanDoGrab()
					&& NPC_EnemyRangeFromBolt( NPC->handRBolt ) < 88.0f )
				{//grab him!
					Kyle_GrabEnemy();
					return;
				}
				else
				{
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
					return;
				}
			}
			//else just sit here?
		}
		return;
	}

	if ( NPC->client->ps.saberLockTime > level.time )
	{
		//FIXME: maybe kick out of saberlock?
		//maybe if I'm losing I should try to force-push out of it?  Very rarely, though...
		if ( NPC->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
			&& NPC->client->ps.saberLockTime < level.time + 5000
			&& !Q_irand( 0, 10 ))
		{
			ForceThrow( NPC, qfalse );
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;

			if ( NPC->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",NPC->NPC_type) )
			{
				if ( g_spskill->integer )
				{
					chance = 4.0f;//he pushes *hard*
				}
				else
				{
					chance = 3.0f;//he pushes *hard*
				}
			}
			else if ( NPC->client->NPC_class == CLASS_TAVION
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| (NPC->client->NPC_class == CLASS_KYLE&&(NPC->spawnflags&1)) )
			{
				chance = 2.0f+g_spskill->value;//from 2 to 4
			}
			else
			{//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				float maxChance	= (float)(RANK_LT)/2.0f+3.0f;//5?
				if ( !g_spskill->value )
				{
					chance = (float)(NPCInfo->rank)/2.0f;
				}
				else
				{
					chance = (float)(NPCInfo->rank)/2.0f+1.0f;
				}
				if ( chance > maxChance )
				{
					chance = maxChance;
				}
			}
			if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
			{
				chance += Q_irand(0,2);
			}
			else if ( (NPCInfo->aiFlags&NPCAI_SUBBOSS_CHARACTER) )
			{
				chance += Q_irand(-1,1);
			}
			if ( Q_flrand( -4.0f, chance ) >= 0.0f && !(NPC->client->ps.pm_flags&PMF_ATTACK_HELD) )
			{
				ucmd.buttons |= BUTTON_ATTACK;
			}
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	//did we drop our saber?  If so, go after it!
	if ( NPC->client->ps.saberInFlight )
	{//saber is not in hand
		if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			{//fell to the ground, try to pick it up
				if ( Jedi_CanPullBackSaber( NPC ) )
				{
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->ps.saberEntityNum];
					ucmd.buttons |= BUTTON_ATTACK;
					if ( NPC->enemy && NPC->enemy->health > 0 )
					{//get our saber back NOW!
						Jedi_Move( NPCInfo->goalEntity, qfalse );
						NPC_UpdateAngles( qtrue, qtrue );
						if ( NPC->enemy->s.weapon == WP_SABER )
						{//be sure to continue evasion
							vec3_t	enemy_dir, enemy_movedir, enemy_dest;
							float	enemy_dist, enemy_movespeed;
							Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );
							Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
						}
						return;
					}
				}
			}
		}
	}
	//see if our enemy was killed by us, gloat and turn off saber after cool down.
	//FIXME: don't do this if we have other enemies to fight...?
	if ( NPC->enemy )
	{
		if ( NPC->enemy->health <= 0
			&& NPC->enemy->enemy == NPC
			&& (NPC->client->playerTeam != TEAM_PLAYER||(NPC->client->NPC_class==CLASS_KYLE&&(NPC->spawnflags&1)&&NPC->enemy==player)) )//good guys don't gloat (unless it's Kyle having just killed his student
		{//my enemy is dead and I killed him
			NPCInfo->enemyCheckDebounceTime = 0;//keep looking for others

			if ( NPC->client->NPC_class == CLASS_BOBAFETT
				|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
			{
				if ( NPCInfo->walkDebounceTime < level.time && NPCInfo->walkDebounceTime >= 0 )
				{
					TIMER_Set( NPC, "gloatTime", 10000 );
					NPCInfo->walkDebounceTime = -1;
				}
				if ( !TIMER_Done( NPC, "gloatTime" ) )
				{
					if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						NPCInfo->goalEntity = NPC->enemy;
						Jedi_Move( NPC->enemy, qfalse );
						ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{
						TIMER_Set( NPC, "gloatTime", 0 );
					}
				}
				else if ( NPCInfo->walkDebounceTime == -1 )
				{
					NPCInfo->walkDebounceTime = -2;
					G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
					jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
					NPCInfo->desiredPitch = 0;
					NPCInfo->goalEntity = NULL;
				}
				Jedi_FaceEnemy( qtrue );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
			else
			{
				if ( !TIMER_Done( NPC, "parryTime" ) )
				{
					TIMER_Set( NPC, "parryTime", -1 );
					NPC->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
				}
				NPC->client->ps.saberBlocked = BLOCKED_NONE;
				if ( NPC->client->ps.SaberActive() || NPC->client->ps.saberInFlight )
				{//saber is still on (or we're trying to pull it back), count down erosion and keep facing the enemy
					//FIXME: need to stop this from happening over and over again when they're blocking their victim's saber
					//FIXME: turn off saber sooner so we get cool walk anim?
					//Com_Printf( "(%d) drop agg - enemy dead\n", level.time );
					Jedi_AggressionErosion(-3);
					if ( !NPC->client->ps.SaberActive() && !NPC->client->ps.saberInFlight )
					{//turned off saber (in hand), gloat
						G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
						jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
						NPCInfo->desiredPitch = 0;
						NPCInfo->goalEntity = NULL;
					}
					TIMER_Set( NPC, "gloatTime", 10000 );
				}
				if ( NPC->client->ps.SaberActive() || NPC->client->ps.saberInFlight || !TIMER_Done( NPC, "gloatTime" ) )
				{//keep walking
					if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						NPCInfo->goalEntity = NPC->enemy;
						Jedi_Move( NPC->enemy, qfalse );
						ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{//got there
						if ( NPC->health < NPC->max_health )
						{
							if ( NPC->client->ps.saber[0].type == SABER_SITH_SWORD
								&& NPC->weaponModel[0] != -1 )
							{
								Tavion_SithSwordRecharge();
							}
							else if ( (NPC->client->ps.forcePowersKnown&(1<<FP_HEAL)) != 0
								&& (NPC->client->ps.forcePowersActive&(1<<FP_HEAL)) == 0 )
							{
								ForceHeal( NPC );
							}
						}
					}
					Jedi_FaceEnemy( qtrue );
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
		}
	}

	//If we don't have an enemy, just idle
	if ( NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPC->enemy->classname ) )
	{
		if ( NPC->enemy->count <= 0 )
		{//it's out of ammo
			if ( NPC->enemy->activator && NPC_ValidEnemy( NPC->enemy->activator ) )
			{
				gentity_t *turretOwner = NPC->enemy->activator;
				G_ClearEnemy( NPC );
				G_SetEnemy( NPC, turretOwner );
			}
			else
			{
				G_ClearEnemy( NPC );
			}
		}
	}
	else if ( NPC->enemy &&
		NPC->enemy->NPC
		&& NPC->enemy->NPC->charmedTime > level.time )
	{//my enemy was charmed
		if ( OnSameTeam( NPC, NPC->enemy ) )
		{//has been charmed to be on my team
			G_ClearEnemy( NPC );
		}
	}
	if ( NPC->client->playerTeam == TEAM_ENEMY
		&& NPC->client->enemyTeam == TEAM_PLAYER
		&& NPC->enemy
		&& NPC->enemy->client
		&& NPC->enemy->client->playerTeam != NPC->client->enemyTeam
		&& OnSameTeam( NPC, NPC->enemy )
		&& !(NPC->svFlags&SVF_LOCKEDENEMY) )
	{//an evil jedi somehow got another evil NPC as an enemy, they were probably charmed and it's run out now
		if ( !NPC_ValidEnemy( NPC->enemy ) )
		{
			G_ClearEnemy( NPC );
		}
	}
	NPC_CheckEnemy( qtrue, qtrue );

	if ( !NPC->enemy )
	{
		NPC->client->ps.saberBlocked = BLOCKED_NONE;
		if ( NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		Jedi_Patrol();//was calling Idle... why?
		return;
	}

	//always face enemy if have one
	NPCInfo->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat();

	if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES)
		|| ((NPC->client->ps.forcePowersActive&(1<<FP_HEAL))&&NPC->client->ps.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_2))
	{//this is really stupid, but okay...
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		if ( ucmd.upmove > 0 )
		{
			ucmd.upmove = 0;
		}
		NPC->client->ps.forceJumpCharge = 0;
		VectorClear( NPC->client->ps.moveDir );
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		VectorClear( NPC->client->ps.moveDir );
	}

	if ( !TIMER_Done( NPC, "duck" ) )
	{
		ucmd.upmove = -127;
	}

	if ( NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
	{
		if ( PM_SaberInBrokenParry( NPC->client->ps.saberMove ) || NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
		{//just make sure they don't pull their saber to them if they're being blocked
			ucmd.buttons &= ~BUTTON_ATTACK;
		}
	}

	if( (NPCInfo->scriptFlags&SCF_DONT_FIRE) //not allowed to attack
		|| ((NPC->client->ps.forcePowersActive&(1<<FP_HEAL))&&NPC->client->ps.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_3)
		|| ((NPC->client->ps.saberEventFlags&SEF_INWATER)&&!NPC->client->ps.saberInFlight) )//saber in water
	{
		ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS);
	}

	if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		ucmd.upmove = 0;
		NPC->client->ps.forceJumpCharge = 0;
	}

	if ( NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
	{
		Jedi_CheckDecreaseSaberAnimLevel();
	}

	if ( ucmd.buttons & BUTTON_ATTACK && NPC->client->playerTeam == TEAM_ENEMY )
	{
		if ( Q_irand( 0, NPC->client->ps.saberAnimLevel ) > 0
			&& Q_irand( 0, NPC->max_health+10 ) > NPC->health
			&& !Q_irand( 0, 3 ))
		{//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent( NPC, Q_irand( EV_COMBAT1, EV_COMBAT3 ), 1000 );
		}
	}

	//Check for trying a kata move
	//FIXME: what about force-pull attacks?
	if ( Jedi_CheckKataAttack() )
	{//doing a kata attack
	}
	else
	{//check other special combat behavior
		if ( NPC->client->NPC_class != CLASS_BOBAFETT
			&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
			&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
		{
			if ( NPC->client->NPC_class == CLASS_TAVION
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| (g_spskill->integer && ( NPC->client->NPC_class == CLASS_DESANN || NPCInfo->rank >= Q_irand( RANK_CREWMAN, RANK_CAPTAIN ))))
			{//Tavion will kick in force speed if the player does...
				if ( NPC->enemy
					&& !NPC->enemy->s.number
					&& NPC->enemy->client
					&& (NPC->enemy->client->ps.forcePowersActive & (1<<FP_SPEED))
					&& !(NPC->client->ps.forcePowersActive & (1<<FP_SPEED)) )
				{
					int chance = 0;
					switch ( g_spskill->integer )
					{
					case 0:
						chance = 9;
						break;
					case 1:
						chance = 3;
						break;
					case 2:
						chance = 1;
						break;
					}
					if ( !Q_irand( 0, chance ) )
					{
						ForceSpeed( NPC );
					}
				}
			}
		}
		//Sometimes Alora flips towards you instead of runs
		if ( NPC->client->NPC_class == CLASS_ALORA )
		{
			if ( (ucmd.buttons&BUTTON_ALT_ATTACK) )
			{//chance of doing a special dual saber throw
				if ( NPC->client->ps.saberAnimLevel == SS_DUAL
					&& !NPC->client->ps.saberInFlight )
				{
					if ( Distance( NPC->enemy->currentOrigin, NPC->currentOrigin ) >= 120 )
					{
						NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ALORA_SPIN_THROW, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
						NPC->client->ps.weaponTime = NPC->client->ps.torsoAnimTimer;
						//FIXME: don't move
						//FIXME: sabers need trails and sounds
					}
				}
			}
			else if ( NPC->enemy
				&& ucmd.forwardmove > 0
				&& fabs((float)ucmd.rightmove) < 32
				&& !(ucmd.buttons&BUTTON_WALKING)
				&& !(ucmd.buttons&BUTTON_ATTACK)
				&& NPC->client->ps.saberMove == LS_READY
				&& NPC->client->ps.legsAnim == BOTH_RUN_DUAL )
			{//running at us, not attacking
				if ( Distance( NPC->enemy->currentOrigin, NPC->currentOrigin ) > 80 )
				{
					if ( NPC->client->ps.legsAnim == BOTH_FLIP_F
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3 )
					{
						if ( NPC->client->ps.legsAnimTimer <= 200 && Q_irand( 0, 2 ) )
						{//go ahead and start anotther
							NPC_SetAnim( NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1,BOTH_ALORA_FLIP_3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
						}
					}
					else if ( !Q_irand( 0, 6 ) )
					{
						NPC_SetAnim( NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1,BOTH_ALORA_FLIP_3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
					}
				}
			}
		}
	}

	if ( VectorCompare( NPC->client->ps.moveDir, vec3_origin )
		&& (ucmd.forwardmove||ucmd.rightmove) )
	{//using ucmds to move this turn, not NAV
		if ( (ucmd.buttons&BUTTON_WALKING) )
		{//FIXME: NAV system screws with speed directly, so now I have to re-set it myself!
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
		}
		else
		{
			NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		}
	}
}

qboolean Rosh_BeingHealed( gentity_t *self )
{
	if ( self
		&& self->NPC
		&& self->client
		&& (self->NPC->aiFlags&NPCAI_ROSH)
		&& (self->flags&FL_UNDYING)
		&& ( self->health == 1 //need healing
			|| self->client->ps.powerups[PW_INVINCIBLE] > level.time ) )//being healed
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Rosh_TwinPresent( gentity_t *self )
{
	gentity_t *foundTwin = G_Find( NULL, FOFS(NPC_type), "DKothos" );
	if ( !foundTwin
		|| foundTwin->health < 0 )
	{
		foundTwin = G_Find( NULL, FOFS(NPC_type), "VKothos" );
	}
	if ( !foundTwin
		|| foundTwin->health < 0 )
	{//oh well, both twins are dead...
		return qfalse;
	}
	return qtrue;
}

qboolean Rosh_TwinNearBy( gentity_t *self )
{
	gentity_t *foundTwin = G_Find( NULL, FOFS(NPC_type), "DKothos" );
	if ( !foundTwin
		|| foundTwin->health < 0 )
	{
		foundTwin = G_Find( NULL, FOFS(NPC_type), "VKothos" );
	}
	if ( !foundTwin
		|| foundTwin->health < 0 )
	{//oh well, both twins are dead...
		return qfalse;
	}
	if ( self->client
		&& foundTwin->client )
	{
		if ( Distance( self->currentOrigin, foundTwin->currentOrigin ) <= 512.0f
			&& G_ClearLineOfSight( self->client->renderInfo.eyePoint, foundTwin->client->renderInfo.eyePoint, foundTwin->s.number, MASK_OPAQUE ) )
		{
			//make them look charge me for a bit while I do this
			TIMER_Set( self, "chargeMeUp", Q_irand( 2000, 4000 ) );
			return qtrue;
		}
	}
	return qfalse;
}

qboolean Kothos_HealRosh( void )
{
	if ( NPC->client
		&& NPC->client->leader
		&& NPC->client->leader->client )
	{
		if ( DistanceSquared( NPC->client->leader->currentOrigin, NPC->currentOrigin ) <= (256*256)
			&& G_ClearLineOfSight( NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint, NPC->s.number, MASK_OPAQUE ) )
		{
			//NPC_FaceEntity( NPC->client->leader, qtrue );
			NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoAnimTimer = 1000;

			//FIXME: unique effect and sound
			//NPC->client->ps.eFlags |= EF_POWERING_ROSH;
			if ( NPC->ghoul2.size() )
			{
				mdxaBone_t	boltMatrix;
				vec3_t		fxOrg, fxDir, angles={0,NPC->currentAngles[YAW],0};

				gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->playerModel,
							(Q_irand(0,1)?NPC->handLBolt:NPC->handRBolt),
							&boltMatrix, angles, NPC->currentOrigin, (cg.time?cg.time:level.time),
							NULL, NPC->s.modelScale );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
				VectorSubtract( NPC->client->leader->currentOrigin, fxOrg, fxDir );
				VectorNormalize( fxDir );
				G_PlayEffect( G_EffectIndex( "force/kothos_beam.efx" ), fxOrg, fxDir );
			}
			//BEG HACK LINE
			gentity_t *tent = G_TempEntity( NPC->currentOrigin, EV_KOTHOS_BEAM );
			tent->svFlags |= SVF_BROADCAST;
			tent->s.otherEntityNum = NPC->s.number;
			tent->s.otherEntityNum2 = NPC->client->leader->s.number;
			//END HACK LINE

			NPC->client->leader->health += Q_irand( 1+g_spskill->integer*2, 4+g_spskill->integer*3 );//from 1-5 to 4-10
			if ( NPC->client->leader->client )
			{
				if ( NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& NPC->client->leader->health >= NPC->client->leader->max_health )
				{//let him get up now
					NPC_SetAnim( NPC->client->leader, SETANIM_BOTH, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					//FIXME: temp effect
					G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), NPC->client->leader->playerModel, 0, NPC->client->leader->s.number, NPC->client->leader->currentOrigin, NPC->client->leader->client->ps.torsoAnimTimer, qfalse );
					//make him invincible while we recharge him
					NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->leader->client->ps.torsoAnimTimer;
					NPC->client->leader->NPC->ignorePain = qfalse;
					NPC->client->leader->health = NPC->client->leader->max_health;
				}
				else
				{
					G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), NPC->client->leader->playerModel, 0, NPC->client->leader->s.number, NPC->client->leader->currentOrigin, 500, qfalse );
					NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
				}
			}
			//decrement
			NPC->count--;
			if ( !NPC->count )
			{
				TIMER_Set( NPC, "healRoshDebounce", Q_irand( 5000, 10000 ) );
				NPC->count = 100;
			}
			//now protect me, too
			if ( g_spskill->integer )
			{//not on easy
				G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), NPC->playerModel, 0, NPC->s.number, NPC->currentOrigin, 500, qfalse );
				NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
			}
			return qtrue;
		}
	}
	return qfalse;
}

void Kothos_PowerRosh( void )
{
	if ( NPC->client
		&& NPC->client->leader )
	{
		if ( Distance( NPC->client->leader->currentOrigin, NPC->currentOrigin ) <= 512.0f
			&& G_ClearLineOfSight( NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint, NPC->s.number, MASK_OPAQUE ) )
		{
			NPC_FaceEntity( NPC->client->leader, qtrue );
			NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoAnimTimer = 500;
			//FIXME: unique effect and sound
			//NPC->client->ps.eFlags |= EF_POWERING_ROSH;
			G_PlayEffect( G_EffectIndex( "force/kothos_beam.efx" ), NPC->playerModel, NPC->handLBolt, NPC->s.number, NPC->currentOrigin, 500, qfalse );
			if ( NPC->client->leader->client )
			{//hmm, give him some force?
				NPC->client->leader->client->ps.forcePower++;
			}
		}
	}
}

qboolean Kothos_Retreat( void )
{
	STEER::Activate( NPC );
	STEER::Evade( NPC, NPC->enemy );
	STEER::AvoidCollisions( NPC, NPC->client->leader );
	STEER::DeActivate( NPC, &ucmd );
	if ( (NPCInfo->aiFlags&NPCAI_BLOCKED) )
	{
		if ( level.time - NPCInfo->blockedDebounceTime > 1000 )
		{
			return qfalse;
		}
	}
	return qtrue;
}

#define TWINS_DANGER_DIST_EASY (128.0f*128.0f)
#define TWINS_DANGER_DIST_MEDIUM (192.0f*192.0f)
#define TWINS_DANGER_DIST_HARD (256.0f*256.0f)
float Twins_DangerDist( void )
{
	switch ( g_spskill->integer )
	{
	case 0:
		return TWINS_DANGER_DIST_EASY;
		break;
	case 1:
		return TWINS_DANGER_DIST_MEDIUM;
		break;
	case 2:
	default:
		return TWINS_DANGER_DIST_HARD;
		break;
	}
}

qboolean Jedi_InSpecialMove( void )
{
	if ( NPC->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_2
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_3
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( Jedi_InNoAIAnim( NPC ) )
	{//in special anims, don't do force powers or attacks, just face the enemy
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}

	/*
	if ( NPC->client->ps.forceGripEntityNum < ENTITYNUM_WORLD
		&& (NPC->client->ps.forcePowersActive&(1<<FP_GRIP))
		&& NPC->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
	{//stop facing the enemy, just use your current angles
		NPC_UpdateAngles( qtrue, qtrue );
	}
	*/

	if ( NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD )
	{
		if ( !TIMER_Done( NPC, "draining" ) )
		{//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			ucmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( NPC->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER )
	{
		NPC->health += Q_irand( 1, 2 );
		if ( NPC->health > NPC->max_health )
		{
			NPC->health = NPC->max_health;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_START )
	{
		if ( NPC->client->ps.torsoAnimTimer <= 100 )
		{//go into the hold
			NPC->s.loopSound = G_SoundIndex( "sound/weapons/scepter/loop.wav" );
			G_PlayEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, 10000, qtrue );
			NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoAnimTimer += 200;
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear( NPC->client->ps.velocity );
			VectorClear( NPC->client->ps.moveDir );
		}
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD )
	{
		if ( NPC->client->ps.torsoAnimTimer <= 100 )
		{
			NPC->s.loopSound = 0;
			G_StopEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number );
			NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoAnimTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear( NPC->client->ps.velocity );
			VectorClear( NPC->client->ps.moveDir );
		}
		else
		{
			Tavion_ScepterDamage();
		}
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_STOP )
	{
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_TAVION_SCEPTERGROUND )
	{
		if ( NPC->client->ps.torsoAnimTimer <= 1200
			&& !NPC->count )
		{
			Tavion_ScepterSlam();
			NPC->count = 1;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( Jedi_CultistDestroyer( NPC ) )
	{
		if ( !NPC->takedamage )
		{//ready to explode
			if ( NPC->useDebounceTime <= level.time )
			{
				//this should damage everyone - FIXME: except other destroyers?
				NPC->client->playerTeam = TEAM_FREE;//FIXME: will this destroy wampas, tusken & rancors?
				WP_Explode( NPC );
				return qtrue;
			}
			if ( NPC->enemy )
			{
				NPC_FaceEnemy( qfalse );
			}
			return qtrue;
		}
	}

	if ( NPC->client->NPC_class == CLASS_REBORN )
	{
		if ( (NPCInfo->aiFlags&NPCAI_HEAL_ROSH) )
		{
			if ( !NPC->client->leader )
			{//find Rosh
				NPC->client->leader = G_Find( NULL, FOFS(NPC_type), "rosh_dark" );
			}
			//NPC->client->ps.eFlags &= ~EF_POWERING_ROSH;
			if ( NPC->client->leader )
			{
				qboolean helpingRosh = qfalse;
				NPC->flags |= FL_LOCK_PLAYER_WEAPONS;
				NPC->client->leader->flags |= FL_UNDYING;
				if ( NPC->client->leader->client )
				{
					NPC->client->leader->client->ps.forcePowersKnown |= FORCE_POWERS_ROSH_FROM_TWINS;
				}
				if ( NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& TIMER_Done( NPC, "healRoshDebounce" ) )
				{
					if ( Kothos_HealRosh() )
					{
						helpingRosh = qtrue;
					}
					else
					{//can't get to him!
						NPC_BSJedi_FollowLeader();
						NPC_UpdateAngles( qtrue, qtrue );
						return qtrue;
					}
				}

				/*
				if ( !helpingRosh
					&& !TIMER_Done( NPC->client->leader, "chargeMeUp" )
					&& NPC->client->leader->health > 0)
				{
					Kothos_PowerRosh();
					helpingRosh = qtrue;
				}
				*/

				if ( helpingRosh )
				{
					WP_ForcePowerStop( NPC, FP_LIGHTNING );
					WP_ForcePowerStop( NPC, FP_DRAIN );
					WP_ForcePowerStop( NPC, FP_GRIP );
					NPC_FaceEntity( NPC->client->leader, qtrue );
					return qtrue;
				}
				else if ( NPC->enemy && DistanceSquared( NPC->enemy->currentOrigin, NPC->currentOrigin ) < Twins_DangerDist() )
				{
					if ( NPC->enemy && Kothos_Retreat() )
					{
						NPC_FaceEnemy( qtrue );
						//NPC_UpdateAngles( qtrue, qtrue );
						if ( TIMER_Done( NPC, "attackDelay" ) )
						{
							if ( NPC->painDebounceTime > level.time
								|| (NPC->health < 100 && Q_irand(-20, (g_spskill->integer+1)*10) > 0 )
								|| !Q_irand( 0, 80-(g_spskill->integer*20) ) )
							{
								NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
								switch ( Q_irand( 0, 7+g_spskill->integer ) )//on easy: no lightning
								{
								case 0:
								case 1:
								case 2:
								case 3:
									ForceThrow( NPC, qfalse, qfalse );
									NPC->client->ps.weaponTime = Q_irand( 1000, 3000 )+(2-g_spskill->integer)*1000;
									if ( NPC->painDebounceTime <= level.time
										&& NPC->health >= 100 )
									{
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								case 4:
								case 5:
									ForceDrain2( NPC );
									NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill->integer)*2000;
									TIMER_Set( NPC, "draining", NPC->client->ps.weaponTime );
									if ( NPC->painDebounceTime <= level.time
										&& NPC->health >= 100 )
									{
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								case 6:
								case 7:
									if ( NPC->enemy && InFOV( NPC->enemy->currentOrigin, NPC->currentOrigin, NPC->client->ps.viewangles, 20, 30 ) )
									{
										NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill->integer)*2000;
										TIMER_Set( NPC, "gripping", 3000 );
										if ( NPC->painDebounceTime <= level.time
											&& NPC->health >= 100 )
										{
											TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
										}
									}
									break;
								case 8:
								case 9:
								default:
									ForceLightning( NPC );
									if ( NPC->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
									{
										NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill->integer)*2000;
										TIMER_Set( NPC, "holdLightning", NPC->client->ps.weaponTime );
									}
									if ( NPC->painDebounceTime <= level.time
										&& NPC->health >= 100 )
									{
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								}
							}
						}
						else
						{
							NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
						}
						Jedi_TimersApply();
						return qtrue;
					}
					else
					{
						NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
				}
				else if ( !G_ClearLOS( NPC, NPC->client->leader )
					|| DistanceSquared( NPC->currentOrigin, NPC->client->leader->currentOrigin ) > (512*512) )
				{//can't see Rosh or too far away, catch up with him
					if ( !TIMER_Done( NPC, "attackDelay" ) )
					{
						NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					NPC_BSJedi_FollowLeader();
					NPC_UpdateAngles( qtrue, qtrue );
					return qtrue;
				}
				else
				{
					if ( !TIMER_Done( NPC, "attackDelay" ) )
					{
						NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					STEER::Activate( NPC );
					STEER::Stop( NPC );
					STEER::DeActivate( NPC, &ucmd );
					NPC_FaceEnemy( qtrue );
					//NPC_UpdateAngles( qtrue, qtrue );
					return qtrue;
					//NPC_BSJedi_FollowLeader();
				}
			}
			NPC_UpdateAngles( qtrue, qtrue );
			//NPC->client->ps.eFlags &= ~EF_POWERING_ROSH;
			//G_StopEffect( G_EffectIndex( "force/kothos_beam.efx" ), NPC->playerModel, NPC->handLBolt, NPC->s.number );
		}
		else if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
		{
			if ( (NPC->flags&FL_UNDYING) )
			{//Vil and/or Dasariah still around to heal me
				if ( NPC->health == 1 //need healing
					|| NPC->client->ps.powerups[PW_INVINCIBLE] > level.time )//being healed
				{//FIXME: custom anims
					if ( Rosh_TwinPresent( NPC ) )
					{
						if ( !NPC->client->ps.weaponTime )
						{//not attacking
							if ( NPC->client->ps.legsAnim != BOTH_FORCEHEAL_START
								&& NPC->client->ps.legsAnim != BOTH_FORCEHEAL_STOP )
							{//get down and wait for Vil or Dasariah to help us
								//FIXME: sound?
								NPC->client->ps.legsAnimTimer = NPC->client->ps.torsoAnimTimer = 0;
								NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								NPC->client->ps.torsoAnimTimer = NPC->client->ps.legsAnimTimer = -1;
								NPC->client->ps.SaberDeactivate();
								NPCInfo->ignorePain = qtrue;
							}
						}
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
						NPC->client->ps.saberMove = NPC->client->ps.saberMoveNext = LS_NONE;
						NPC->painDebounceTime = level.time + 500;
						NPC->client->ps.pm_time = 500;
						NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						VectorClear( NPC->client->ps.velocity );
						VectorClear( NPC->client->ps.moveDir );
						return qtrue;
					}
				}
			}
		}
	}

	if ( PM_SuperBreakWinAnim( NPC->client->ps.torsoAnim ) )
	{
		NPC_FaceEnemy( qtrue );
		if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{
			VectorClear( NPC->client->ps.velocity );
		}
		VectorClear( NPC->client->ps.moveDir );
		ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
		return qtrue;
	}

	return qfalse;
}

extern void NPC_BSST_Patrol( void );
extern void NPC_BSSniper_Default( void );
extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
void NPC_BSJedi_Default( void )
{
	if ( Jedi_InSpecialMove() )
	{
		return;
	}

	Jedi_CheckCloak();

	if( !NPC->enemy )
	{//don't have an enemy, look for one
		if ( NPC->client->NPC_class == CLASS_BOBAFETT
			|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
			|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
		{
			NPC_BSST_Patrol();
		}
		else
		{
			Jedi_Patrol();
		}
	}
	else//if ( NPC->enemy )
	{//have an enemy
		if ( Jedi_WaitingAmbush( NPC ) )
		{//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush( NPC );
		}

		if ( Jedi_CultistDestroyer( NPC )
			&& !NPCInfo->charmedTime )
		{//destroyer
			//permanent effect
			NPCInfo->charmedTime = Q3_INFINITE;
			NPC->client->ps.forcePowersActive |= ( 1 << FP_RAGE );
			NPC->client->ps.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			//NPC->client->ps.eFlags |= EF_FORCE_DRAINED;
			//FIXME: precache me!
			NPC->s.loopSound = G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );//test/charm.wav" );
		}

		Jedi_Attack();
		//if we have multiple-jedi combat, probably need to keep checking (at certain debounce intervals) for a better (closer, more active) enemy and switch if needbe...
		if ( ((!ucmd.buttons&&!NPC->client->ps.forcePowersActive)||(NPC->enemy&&NPC->enemy->health<=0)) && NPCInfo->enemyCheckDebounceTime < level.time )
		{//not doing anything (or walking toward a vanquished enemy - fixme: always taunt the player?), not using force powers and it's time to look again
			//FIXME: build a list of all local enemies (since we have to find best anyway) for other AI factors- like when to use group attacks, determine when to change tactics, when surrounded, when blocked by another in the enemy group, etc.  Should we build this group list or let the enemies maintain their own list and we just access it?
			gentity_t *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
			NPC->enemy = NULL;
			gentity_t *newEnemy = NPC_CheckEnemy( (qboolean)(NPCInfo->confusionTime < level.time), qfalse, qfalse );
			NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPC->lastEnemy = NPC->enemy;
				G_SetEnemy( NPC, newEnemy );
			}
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 3000 );
		}
	}
	if ( NPC->client->ps.saber[0].type == SABER_SITH_SWORD
		&& NPC->weaponModel[0] != -1 )
	{
		if ( NPC->health < 100
			&& !Q_irand( 0, 20 ) )
		{
			Tavion_SithSwordRecharge();
		}
	}
}
