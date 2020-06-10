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
#include "w_saber.h"

extern qboolean BG_SabersOff( playerState_t *ps );

extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void ForceJump( gentity_t *self, usercmd_t *ucmd );

#define	MAX_VIEW_DIST		2048
#define MAX_VIEW_SPEED		100
#define	JEDI_MAX_LIGHT_INTENSITY 64
#define	JEDI_MIN_LIGHT_THRESHOLD 10
#define	JEDI_MAX_LIGHT_THRESHOLD 50

#define	DISTANCE_SCALE		0.25f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.3 )

#define	MAX_CHECK_THRESHOLD	1

extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean NPC_CheckEnemyStealth( void );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );

extern void ForceThrow( gentity_t *self, qboolean pull );
extern void ForceLightning( gentity_t *self );
extern void ForceHeal( gentity_t *self );
extern void ForceRage( gentity_t *self );
extern void ForceProtect( gentity_t *self );
extern void ForceAbsorb( gentity_t *self );
extern int WP_MissileBlockForBlock( int saberBlock );
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower );
extern qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength ); //clearLength = qfalse
extern void WP_ActivateSaber( gentity_t *self );

extern qboolean PM_SaberInStart( int move );
extern qboolean BG_SaberInSpecialAttack( int anim );
extern qboolean BG_SaberInAttack( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SaberInDeflect( int move );
extern qboolean BG_SpinningSaberAnim( int anim );
extern qboolean BG_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean BG_InRoll( playerState_t *ps, int anim );
extern qboolean BG_CrouchAnim( int anim );

extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);

extern int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd );

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

static void Jedi_Aggression( gentity_t *self, int change );
qboolean Jedi_WaitingAmbush( gentity_t *self );

extern int bg_parryDebounce[];

static int	jediSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several jedi from speaking all at once
//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

void NPC_ShadowTrooper_Precache( void )
{
	RegisterItem( BG_FindItemForAmmo( AMMO_FORCE ) );
	G_SoundIndex( "sound/chars/shadowtrooper/cloak.wav" );
	G_SoundIndex( "sound/chars/shadowtrooper/decloak.wav" );
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
}

void Jedi_PlayBlockedPushSound( gentity_t *self )
{
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
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
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
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
		if ( self->client && ( self->client->NPC_class == CLASS_TAVION || self->client->NPC_class == CLASS_DESANN ) )
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

qboolean Jedi_CultistDestroyer( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return qfalse;
	}
	if( self->client->NPC_class == CLASS_REBORN &&
		self->s.weapon == WP_MELEE &&
		!Q_stricmp( "cultist_destroyer", self->NPC_type ) )
	{
		return qtrue;
	}
	return qfalse;
}

void Boba_Precache( void )
{
	G_SoundIndex( "sound/boba/jeton.wav" );
	G_SoundIndex( "sound/boba/jethover.wav" );
	G_SoundIndex( "sound/effects/combustfire.mp3" );
	G_EffectIndex( "boba/jet" );
	G_EffectIndex( "boba/fthrw" );
}

extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void Boba_ChangeWeapon( int wp )
{
	if ( NPCS.NPC->s.weapon == wp )
	{
		return;
	}
	NPC_ChangeWeapon( wp );
	G_AddEvent( NPCS.NPC, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));
}

void WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty )
{
	int parts;
	qboolean runningResist = qfalse;

	if ( !self || self->health <= 0 || !self->client || !pusher || !pusher->client )
	{
		return;
	}
	if ( ((self->s.number >= 0 && self->s.number < MAX_CLIENTS) || self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) || self->client->NPC_class == CLASS_LUKE)
		&& (VectorLengthSquared( self->client->ps.velocity ) > 10000 || self->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 ) )
	{
		runningResist = qtrue;
	}
	if ( !runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !BG_SpinningSaberAnim( self->client->ps.legsAnim )
		&& !BG_FlippingAnim( self->client->ps.legsAnim )
		&& !PM_RollingAnim( self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !BG_CrouchAnim( self->client->ps.legsAnim ))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}
	NPC_SetAnim( self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( !noPenalty )
	{
		char buf[128];
		float tFVal = 0;

		trap->Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		if ( !runningResist )
		{
			VectorClear( self->client->ps.velocity );
			//still stop them from attacking or moving for a bit, though
			//FIXME: maybe push just a little (like, slide)?
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			//self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			//rwwFIXMEFIXME: Do this?
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_DISINT_4] = level.time + self->client->ps.torsoTimer + 500;
	self->client->ps.powerups[PW_PULL] = 0;
	Jedi_PlayBlockedPushSound( self );
}

qboolean Boba_StopKnockdown( gentity_t *self, gentity_t *pusher, vec3_t pushDir, qboolean forceKnockdown ) //forceKnockdown = qfalse
{
	vec3_t	pDir, fwd, right, ang;
	float	fDot, rDot;
	int		strafeTime;

	if ( self->client->NPC_class != CLASS_BOBAFETT )
	{
		return qfalse;
	}

	if ( (self->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);
	strafeTime = Q_irand( 1000, 2000 );

	AngleVectors( ang, fwd, right, NULL );
	VectorNormalize2( pushDir, pDir );
	fDot = DotProduct( pDir, fwd );
	rDot = DotProduct( pDir, right );

	if ( Q_irand( 0, 2 ) )
	{//flip or roll with it
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
			self->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
			ForceJump( self, &tempCmd );
		}
		else
		{//roll
			TIMER_Set( self, "duck", strafeTime );
		}
		self->painDebounceTime = 0;//so we do something
	}
	else if ( !Q_irand( 0, 1 ) && forceKnockdown )
	{//resist
		WP_ResistForcePush( self, pusher, qtrue );
	}
	else
	{//fall down
		return qfalse;
	}

	return qtrue;
}

void Boba_FlyStart( gentity_t *self )
{//switch to seeker AI for a while
	if ( TIMER_Done( self, "jetRecharge" ) )
	{
		self->client->ps.gravity = 0;
		if ( self->NPC )
		{
			self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		}
		self->client->ps.eFlags2 |= EF2_FLYING;//moveType = MT_FLYSWIM;
		self->client->jetPackTime = level.time + Q_irand( 3000, 10000 );
		//take-off sound
		G_SoundOnEnt( self, CHAN_ITEM, "sound/boba/jeton.wav" );
		//jet loop sound
		self->s.loopSound = G_SoundIndex( "sound/boba/jethover.wav" );
		if ( self->NPC )
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
}

void Boba_FlyStop( gentity_t *self )
{
	self->client->ps.gravity = g_gravity.value;
	if ( self->NPC )
	{
		self->NPC->aiFlags &= ~NPCAI_CUSTOM_GRAVITY;
	}
	self->client->ps.eFlags2 &= ~EF2_FLYING;
	self->client->jetPackTime = 0;
	//stop jet loop sound
	self->s.loopSound = 0;
	if ( self->NPC )
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set( self, "jetRecharge", Q_irand( 1000, 5000 ) );
		TIMER_Set( self, "jumpChaseDebounce", Q_irand( 500, 2000 ) );
	}
}

qboolean Boba_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->ps.eFlags2&EF2_FLYING));//moveType==MT_FLYSWIM));
}

void Boba_FireFlameThrower( gentity_t *self )
{
	int		damage	= Q_irand( 20, 30 );
	trace_t		tr;
	gentity_t	*traceEnt = NULL;
	mdxaBone_t	boltMatrix;
	vec3_t		start, end, dir, traceMins = {-4, -4, -4}, traceMaxs = {4, 4, 4};

	trap->G2API_GetBoltMatrix( self->ghoul2, 0, self->client->renderInfo.handLBolt,
			&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
			NULL, self->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );
	//G_PlayEffect( "boba/fthrw", start, dir );
	VectorMA( start, 128, dir, end );

	trap->Trace( &tr, start, traceMins, traceMaxs, end, self->s.number, MASK_SHOT, qfalse, 0, 0 );

	traceEnt = &g_entities[tr.entityNum];
	if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
	{
		G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_LAVA );
		//rwwFIXMEFIXME: add DAMAGE_NO_HIT_LOC?
	}
}

void Boba_StartFlameThrower( gentity_t *self )
{
	int	flameTime = 4000;//Q_irand( 1000, 3000 );
	mdxaBone_t	boltMatrix;
	vec3_t		org, dir;

	self->client->ps.torsoTimer = flameTime;//+1000;
	if ( self->NPC )
	{
		TIMER_Set( self, "nextAttackDelay", flameTime );
		TIMER_Set( self, "walking", 0 );
	}
	TIMER_Set( self, "flameTime", flameTime );
	/*
	gentity_t *fire = G_Spawn();
	if ( fire != NULL )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir, ang;
		trap->G2API_GetBoltMatrix( NPC->ghoul2, NPC->playerModel, NPC->handRBolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, (cg.time?cg.time:level.time),
				NULL, NPC->s.modelScale );

		trap->G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		trap->G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
		vectoangles( dir, ang );

		VectorCopy( org, fire->s.origin );
		VectorCopy( ang, fire->s.angles );

		fire->targetname = "bobafire";
		SP_fx_explosion_trail( fire );
		fire->damage = 1;
		fire->radius = 10;
		fire->speed = 200;
		fire->fxID = G_EffectIndex( "boba/fthrw" );//"env/exp_fire_trail" );//"env/small_fire"
		fx_explosion_trail_link( fire );
		fx_explosion_trail_use( fire, NPC, NPC );

	}
	*/
	G_SoundOnEnt( self, CHAN_WEAPON, "sound/effects/combustfire.mp3" );

	trap->G2API_GetBoltMatrix(NPCS.NPC->ghoul2, 0, NPCS.NPC->client->renderInfo.handRBolt, &boltMatrix, NPCS.NPC->r.currentAngles,
		NPCS.NPC->r.currentOrigin, level.time, NULL, NPCS.NPC->modelScale);

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

	G_PlayEffectID( G_EffectIndex("boba/fthrw"), org, dir);
}

void Boba_DoFlameThrower( gentity_t *self )
{
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( TIMER_Done( self, "nextAttackDelay" ) && TIMER_Done( self, "flameTime" ) )
	{
		Boba_StartFlameThrower( self );
	}
	Boba_FireFlameThrower( self );
}

void Boba_FireDecide( void )
{
	qboolean enemyLOS = qfalse, enemyCS = qfalse, enemyInFOV = qfalse;
	//qboolean move = qtrue;
	qboolean shoot = qfalse, hitAlly = qfalse;
	vec3_t	impactPos, enemyDir, shootDir;
	float	enemyDist, dot;

	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& NPCS.NPC->client->ps.fd.forceJumpZStart
		&& !BG_FlippingAnim( NPCS.NPC->client->ps.legsAnim )
		&& !Q_irand( 0, 10 ) )
	{//take off
		Boba_FlyStart( NPCS.NPC );
	}

	if ( !NPCS.NPC->enemy )
	{
		return;
	}

	/*
	if ( NPC->enemy->enemy != NPC && NPC->health == NPC->client->pers.maxHealth )
	{
		NPCInfo->scriptFlags |= SCF_ALT_FIRE;
		Boba_ChangeWeapon( WP_DISRUPTOR );
	}
	else */if ( NPCS.NPC->enemy->s.weapon == WP_SABER )
	{
		NPCS.NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
		Boba_ChangeWeapon( WP_ROCKET_LAUNCHER );
	}
	else
	{
		if ( NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth*0.5f )
		{
			NPCS.NPCInfo->scriptFlags |= SCF_ALT_FIRE;
			Boba_ChangeWeapon( WP_BLASTER );
			NPCS.NPCInfo->burstMin = 3;
			NPCS.NPCInfo->burstMean = 12;
			NPCS.NPCInfo->burstMax = 20;
			NPCS.NPCInfo->burstSpacing = Q_irand( 300, 750 );//attack debounce
		}
		else
		{
			NPCS.NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			Boba_ChangeWeapon( WP_BLASTER );
		}
	}

	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );

	VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPCS.NPC->client->ps.viewangles, shootDir, NULL, NULL );
	dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( (enemyDist < (128*128)&&enemyInFOV) || !TIMER_Done( NPCS.NPC, "flameTime" ) )
	{//flamethrower
		Boba_DoFlameThrower( NPCS.NPC );
		enemyCS = qfalse;
		shoot = qfalse;
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		NPCS.ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}
	else if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (NPCS.NPC->client->ps.weapon == WP_FLECHETTE || NPCS.NPC->client->ps.weapon == WP_REPEATER) &&
			(NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			NPCS.NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}
	else if ( enemyDist > 65536 )//256 squared
	{
		if ( NPCS.NPC->client->ps.weapon == WP_DISRUPTOR )
		{//sniping... should be assumed
			if ( !(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				NPCS.NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon( WP_DISRUPTOR );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	//can we see our target?
	if ( TIMER_Done( NPCS.NPC, "nextAttackDelay" ) && TIMER_Done( NPCS.NPC, "flameTime" ) )
	{
		if ( NPC_ClearLOS4( NPCS.NPC->enemy ) )
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if ( NPCS.NPC->client->ps.weapon == WP_NONE )
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
			else
			{//can we shoot our target?
				if ( (NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER || (NPCS.NPC->client->ps.weapon == WP_FLECHETTE && (NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist < MIN_ROCKET_DIST_SQUARED )//128*128
				{
					enemyCS = qfalse;//not true, but should stop us from firing
					hitAlly = qtrue;//us!
					//FIXME: if too close, run away!
				}
				else if ( enemyInFOV )
				{//if enemy is FOV, go ahead and check for shooting
					int hit = NPC_ShotEntity( NPCS.NPC->enemy, impactPos );
					gentity_t *hitEnt = &g_entities[hit];

					if ( hit == NPCS.NPC->enemy->s.number
						|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPCS.NPC->client->enemyTeam )
						|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPCS.NPC->s.weapon == WP_EMPLACED_GUN) ) )
					{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
						enemyCS = qtrue;
						//NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation );
					}
					else
					{//Hmm, have to get around this bastard
						//NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
						if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPCS.NPC->client->playerTeam )
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
		else if ( trap->InPVS( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) )
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			//NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}

		if ( NPCS.NPC->client->ps.weapon == WP_NONE )
		{
			shoot = qfalse;
		}
		else
		{
			if ( enemyCS )
			{
				shoot = qtrue;
			}
		}

		if ( !enemyCS )
		{//if have a clear shot, always try
			//See if we should continue to fire on their last position
			//!TIMER_Done( NPC, "stick" ) ||
			if ( !hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& NPCS.NPCInfo->enemyLastSeenTime > 0 )//we've seen the enemy
			{
				if ( level.time - NPCS.NPCInfo->enemyLastSeenTime < 10000 )//we have seem the enemy in the last 10 seconds
				{
					if ( !Q_irand( 0, 10 ) )
					{
						//Fire on the last known position
						vec3_t	muzzle, dir, angles;
						qboolean tooClose = qfalse;
						qboolean tooFar = qfalse;
						float distThreshold;
						float dist;

						CalcEntitySpot( NPCS.NPC, SPOT_HEAD, muzzle );
						if ( VectorCompare( impactPos, vec3_origin ) )
						{//never checked ShotEntity this frame, so must do a trace...
							trace_t tr;
							//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
							vec3_t	forward, end;
							AngleVectors( NPCS.NPC->client->ps.viewangles, forward, NULL, NULL );
							VectorMA( muzzle, 8192, forward, end );
							trap->Trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0 );
							VectorCopy( tr.endpos, impactPos );
						}

						//see if impact would be too close to me
						distThreshold = 16384/*128*128*/;//default
						switch ( NPCS.NPC->s.weapon )
						{
						case WP_ROCKET_LAUNCHER:
						case WP_FLECHETTE:
						case WP_THERMAL:
						case WP_TRIP_MINE:
						case WP_DET_PACK:
							distThreshold = 65536/*256*256*/;
							break;
						case WP_REPEATER:
							if ( NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						default:
							break;
						}

						dist = DistanceSquared( impactPos, muzzle );

						if ( dist < distThreshold )
						{//impact would be too close to me
							tooClose = qtrue;
						}
						else if ( level.time - NPCS.NPCInfo->enemyLastSeenTime > 5000 ||
							(NPCS.NPCInfo->group && level.time - NPCS.NPCInfo->group->lastSeenEnemyTime > 5000 ))
						{//we've haven't seen them in the last 5 seconds
							//see if it's too far from where he is
							distThreshold = 65536/*256*256*/;//default
							switch ( NPCS.NPC->s.weapon )
							{
							case WP_ROCKET_LAUNCHER:
							case WP_FLECHETTE:
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
								break;
							case WP_REPEATER:
								if ( NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							default:
								break;
							}
							dist = DistanceSquared( impactPos, NPCS.NPCInfo->enemyLastSeenLocation );
							if ( dist > distThreshold )
							{//impact would be too far from enemy
								tooFar = qtrue;
							}
						}

						if ( !tooClose && !tooFar )
						{//okay too shoot at last pos
							VectorSubtract( NPCS.NPCInfo->enemyLastSeenLocation, muzzle, dir );
							VectorNormalize( dir );
							vectoangles( dir, angles );

							NPCS.NPCInfo->desiredYaw		= angles[YAW];
							NPCS.NPCInfo->desiredPitch	= angles[PITCH];

							shoot = qtrue;
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		if ( NPCS.NPC->client->ps.weaponTime > 0 )
		{
			if ( NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER )
			{
				if ( !enemyLOS || !enemyCS )
				{//cancel it
					NPCS.NPC->client->ps.weaponTime = 0;
				}
				else
				{//delay our next attempt
					TIMER_Set( NPCS.NPC, "nextAttackDelay", Q_irand( 500, 1000 ) );
				}
			}
		}
		else if ( shoot )
		{//try to shoot if it's time
			if ( TIMER_Done( NPCS.NPC, "nextAttackDelay" ) )
			{
				if( !(NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
				{
					WeaponThink( qtrue );
				}
				//NASTY
				if ( NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER
					&& (NPCS.ucmd.buttons&BUTTON_ATTACK)
					&& !Q_irand( 0, 3 ) )
				{//every now and then, shoot a homing rocket
					NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
					NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPCS.NPC->client->ps.weaponTime = Q_irand( 500, 1500 );
				}
			}
		}
	}
}

void Jedi_Cloak( gentity_t *self )
{
	if ( self )
	{
		self->flags |= FL_NOTARGET;
		if ( self->client )
		{
			if ( !self->client->ps.powerups[PW_CLOAKED] )
			{//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;

				//FIXME: debounce attacks?
				//FIXME: temp sound
				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/cloak.wav") );
			}
		}
	}
}

void Jedi_Decloak( gentity_t *self )
{
	if ( self )
	{
		self->flags &= ~FL_NOTARGET;
		if ( self->client )
		{
			if ( self->client->ps.powerups[PW_CLOAKED] )
			{//Uncloak
				self->client->ps.powerups[PW_CLOAKED] = 0;

				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/decloak.wav") );
			}
		}
	}
}

void Jedi_CheckCloak( void )
{
	if ( NPCS.NPC && NPCS.NPC->client && NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER )
	{
		if ( !NPCS.NPC->client->ps.saberHolstered ||
			NPCS.NPC->health <= 0 ||
			NPCS.NPC->client->ps.saberInFlight ||
		//	(NPC->client->ps.eFlags&EF_FORCE_GRIPPED) ||
		//	(NPC->client->ps.eFlags&EF_FORCE_DRAINED) ||
			NPCS.NPC->painDebounceTime > level.time )
		{//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak( NPCS.NPC );
		}
		else if ( NPCS.NPC->health > 0
			&& !NPCS.NPC->client->ps.saberInFlight
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_GRIPPED)
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_DRAINED)
			&& NPCS.NPC->painDebounceTime < level.time )
		{//still alive, have saber in hand, not taking pain and not being gripped
			Jedi_Cloak( NPCS.NPC );
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
	if ( self->client->playerTeam == NPCTEAM_PLAYER )
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
	if ( TIMER_Done( NPCS.NPC, "roamTime" ) )
	{//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set( NPCS.NPC, "roamTime", Q_irand( 2000, 5000 ) );
		Jedi_Aggression( NPCS.NPC, amt );
	}

	if ( NPCS.NPCInfo->stats.aggression < 4 || (NPCS.NPCInfo->stats.aggression < 6&&NPCS.NPC->client->NPC_class == CLASS_DESANN))
	{//turn off the saber
		WP_DeactivateSaber( NPCS.NPC, qfalse );
	}
}

void NPC_Jedi_RateNewEnemy( gentity_t *self, gentity_t *enemy )
{
	float healthAggression;
	float weaponAggression;
	int newAggression;

	switch( enemy->s.weapon )
	{
	case WP_SABER:
		healthAggression = (float)self->health/200.0f*6.0f;
		weaponAggression = 7;//go after him
		break;
	case WP_BLASTER:
		if ( DistanceSquared( self->r.currentOrigin, enemy->r.currentOrigin ) < 65536 )//256 squared
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
	newAggression = ceil( (healthAggression + weaponAggression + (float)self->NPC->stats.aggression )/3.0f);
	//Com_Printf( "(%d) new agg %d - new enemy\n", level.time, newAggression );
	Jedi_Aggression( self, newAggression - self->NPC->stats.aggression );

	//don't taunt right away
	TIMER_Set( self, "chatter", Q_irand( 4000, 7000 ) );
}

static void Jedi_Rage( void )
{
	Jedi_Aggression( NPCS.NPC, 10 - NPCS.NPCInfo->stats.aggression + Q_irand( -2, 2 ) );
	TIMER_Set( NPCS.NPC, "roamTime", 0 );
	TIMER_Set( NPCS.NPC, "chatter", 0 );
	TIMER_Set( NPCS.NPC, "walking", 0 );
	TIMER_Set( NPCS.NPC, "taunting", 0 );
	TIMER_Set( NPCS.NPC, "jumpChaseDebounce", 0 );
	TIMER_Set( NPCS.NPC, "movenone", 0 );
	TIMER_Set( NPCS.NPC, "movecenter", 0 );
	TIMER_Set( NPCS.NPC, "noturn", 0 );
	ForceRage( NPCS.NPC );
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
	if ( TIMER_Done( NPCS.NPC, "chatter" )
		&& !Q_irand( 0, 3 )
		&& NPCS.NPCInfo->blockedSpeechDebounceTime < level.time
		&& jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time )
	{
		int event = -1;
		if ( NPCS.NPC->client->playerTeam == NPCTEAM_PLAYER
			&& NPCS.NPC->enemy && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_JEDI )
		{//a jedi fighting a jedi - training
			if ( NPCS.NPC->client->NPC_class == CLASS_JEDI && NPCS.NPCInfo->rank == RANK_COMMANDER )
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
			G_AddVoiceEvent( NPCS.NPC, event, 3000 );
			jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 6000;
			TIMER_Set( NPCS.NPC, "chatter", Q_irand( 5000, 10000 ) );

			if ( NPCS.NPC->enemy && NPCS.NPC->enemy->NPC && NPCS.NPC->enemy->s.weapon == WP_SABER && NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->NPC_class == CLASS_JEDI )
			{//Have the enemy jedi say something in response when I'm done?
			}
			return qtrue;
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
	float	dist, drop, i;

	//Offset the step height
	VectorSet( mins, NPCS.NPC->r.mins[0], NPCS.NPC->r.mins[1], NPCS.NPC->r.mins[2] + STEPSIZE );

	trap->Trace( &trace, NPCS.NPC->r.currentOrigin, mins, NPCS.NPC->r.maxs, dest, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );

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
	VectorSubtract( dest, NPCS.NPC->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPCS.NPC->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = NPCS.NPC->r.maxs[0]*2; i < dist; i += NPCS.NPC->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPCS.NPC->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap->Trace( &trace, start, mins, NPCS.NPC->r.maxs, end, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );//NPC->r.mins?
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
		//Com_Printf( "%d skipping walk-cliff check (not moving)\n", level.time );
		return qtrue;
	}

	if ( NPCS.ucmd.upmove > 0 || NPCS.NPC->client->ps.fd.forceJumpCharge )
	{//Going to jump
		//Com_Printf( "%d skipping walk-cliff check (going to jump)\n", level.time );
		return qtrue;
	}

	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in the air
		//Com_Printf( "%d skipping walk-cliff check (in air)\n", level.time );
		return qtrue;
	}
	/*
	if ( fabs( AngleDelta( NPC->r.currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
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
	VectorCopy( NPCS.NPC->r.mins, mins );
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = NPCS.NPC->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
	AngleVectors( angles, forward, right, NULL );
	fwdDist = ((float)forwardmove)/2.0f;
	rtDist = ((float)rightmove)/2.0f;
	VectorMA( NPCS.NPC->r.currentOrigin, fwdDist, forward, testPos );
	VectorMA( testPos, rtDist, right, testPos );
	trap->Trace( &trace, NPCS.NPC->r.currentOrigin, mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
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
		if ( (NPCS.NPC->enemy && trace.entityNum == NPCS.NPC->enemy->s.number) || (NPCS.NPCInfo->goalEntity && trace.entityNum == NPCS.NPCInfo->goalEntity->s.number) )
		{//okay to bump into enemy or goal
			//Com_Printf( "%d bump into enemy/goal okay\n", level.time );
			return qtrue;
		}
		else if ( reset )
		{//actually want to screw with the ucmd
			//Com_Printf( "%d avoiding walk into wall (entnum %d)\n", level.time, trace.entityNum );
			NPCS.ucmd.forwardmove = 0;
			NPCS.ucmd.rightmove = 0;
			VectorClear( NPCS.NPC->client->ps.moveDir );
		}
		return qfalse;
	}

	if ( NPCS.NPCInfo->goalEntity )
	{
		if ( NPCS.NPCInfo->goalEntity->r.currentOrigin[2] < NPCS.NPC->r.currentOrigin[2] )
		{//goal is below me, okay to step off at least that far plus stepheight
			bottom_max += NPCS.NPCInfo->goalEntity->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];
		}
	}
	VectorCopy( trace.endpos, testPos );
	testPos[2] += bottom_max;

	trap->Trace( &trace, trace.endpos, mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );

	//FIXME:Should we try to see if we can still get to our goal using the waypoint network from this trace.endpos?
	//OR: just put NPC clip brushes on these edges (still fall through when die)

	if ( trace.allsolid || trace.startsolid )
	{//Not going off a cliff
		//Com_Printf( "%d walk off cliff okay (droptrace in solid)\n", level.time );
		return qtrue;
	}

	if ( trace.fraction < 1.0 )
	{//Not going off a cliff
		//FIXME: what if plane.normal is sloped?  We'll slide off, not land... plus this doesn't account for slide-movement...
		//Com_Printf( "%d walk off cliff okay will hit entnum %d at dropdist of %4.2f\n", level.time, trace.entityNum, (trace.fraction*bottom_max) );
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if ( reset )
	{//actually want to screw with the ucmd
		//Com_Printf( "%d avoiding walk off cliff\n", level.time );
		NPCS.ucmd.forwardmove *= -1.0;//= 0;
		NPCS.ucmd.rightmove *= -1.0;//= 0;
		VectorScale( NPCS.NPC->client->ps.moveDir, -1, NPCS.NPC->client->ps.moveDir );
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
	NPCS.NPCInfo->goalEntity = NULL;

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

static void Jedi_Move( gentity_t *goal, qboolean retreat )
{
	qboolean	moved;
	navInfo_t	info;

	NPCS.NPCInfo->combatMove = qtrue;
	NPCS.NPCInfo->goalEntity = goal;

	moved = NPC_MoveToGoal( qtrue );

	//FIXME: temp retreat behavior- should really make this toward a safe spot or maybe to outflank enemy
	if ( retreat )
	{//FIXME: should we trace and make sure we can go this way?  Or somehow let NPC_MoveToGoal know we want to retreat and have it handle it?
		NPCS.ucmd.forwardmove *= -1;
		NPCS.ucmd.rightmove *= -1;
		VectorScale( NPCS.NPC->client->ps.moveDir, -1, NPCS.NPC->client->ps.moveDir );
	}

	//Get the move info
	NAV_GetLastMove( &info );

	//If we hit our target, then stop and fire!
	if ( ( info.flags & NIF_COLLISION ) && ( info.blocker == NPCS.NPC->enemy ) )
	{
		Jedi_HoldPosition();
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{
		Jedi_HoldPosition();
	}
}

static qboolean Jedi_Hunt( void )
{
	//Com_Printf( "Hunting\n" );
	//if we're at all interested in fighting, go after him
	if ( NPCS.NPCInfo->stats.aggression > 1 )
	{//approach enemy
		NPCS.NPCInfo->combatMove = qtrue;
		if ( !(NPCS.NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
		{
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
		}
		else
		{
			if ( NPCS.NPCInfo->goalEntity == NULL )
			{//hunt
				NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
			}
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

static void Jedi_Retreat( void )
{
	if ( !TIMER_Done( NPCS.NPC, "noRetreat" ) )
	{//don't actually move
		return;
	}
	//FIXME: when retreating, we should probably see if we can retreat
	//in the direction we want.  If not...?  Evade?
	//Com_Printf( "Retreating\n" );
	Jedi_Move( NPCS.NPC->enemy, qtrue );
}

static void Jedi_Advance( void )
{
	if ( !NPCS.NPC->client->ps.saberInFlight )
	{
		//NPC->client->ps.SaberActivate();
		WP_ActivateSaber(NPCS.NPC);
	}
	//Com_Printf( "Advancing\n" );
	Jedi_Move( NPCS.NPC->enemy, qfalse );

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
	if ( self->client->NPC_class == CLASS_TAVION )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_5;
		return;
	}
	else if ( self->client->NPC_class == CLASS_DESANN )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_4;
		return;
	}
	if ( self->client->playerTeam == NPCTEAM_ENEMY )
	{
		if ( self->NPC->rank == RANK_CIVILIAN || self->NPC->rank == RANK_LT_JG )
		{//grunt and fencer always uses quick attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
			return;
		}
		if ( self->NPC->rank == RANK_CREWMAN
			|| self->NPC->rank == RANK_ENSIGN )
		{//acrobat & force-users always use medium attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_2;
			return;
		}
		/*
		if ( self->NPC->rank == RANK_LT )
		{//boss always uses strong attacks
			self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
			return;
		}
		*/
	}
	//use the different attacks, how often they switch and under what circumstances
	if ( newLevel > self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
	{//cap it
		self->client->ps.fd.saberAnimLevel = self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	else if ( newLevel < FORCE_LEVEL_1 )
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	else
	{//go ahead and set it
		self->client->ps.fd.saberAnimLevel = newLevel;
	}

	if ( d_JediAI.integer )
	{
		switch ( self->client->ps.fd.saberAnimLevel )
		{
		case FORCE_LEVEL_1:
			Com_Printf( S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type );
			break;
		case FORCE_LEVEL_2:
			Com_Printf( S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type );
			break;
		case FORCE_LEVEL_3:
			Com_Printf( S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type );
			break;
		}
	}
}

static void Jedi_CheckDecreaseSaberAnimLevel( void )
{
	if ( !NPCS.NPC->client->ps.weaponTime && !(NPCS.ucmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
	{//not attacking
		if ( TIMER_Done( NPCS.NPC, "saberLevelDebounce" ) && !Q_irand( 0, 10 ) )
		{
			//Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );//drop
			Jedi_AdjustSaberAnimLevel( NPCS.NPC, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ));//random
			TIMER_Set( NPCS.NPC, "saberLevelDebounce", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{
		TIMER_Set( NPCS.NPC, "saberLevelDebounce", Q_irand( 1000, 5000 ) );
	}
}

extern void ForceDrain( gentity_t *self );
static void Jedi_CombatDistance( int enemy_dist )
{//FIXME: for many of these checks, what we really want is horizontal distance to enemy
	if ( NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP) &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//when gripping, don't move
		return;
	}
	else if ( !TIMER_Done( NPCS.NPC, "gripping" ) )
	{//stopped gripping, clear timers just in case
		TIMER_Set( NPCS.NPC, "gripping", -level.time );
		TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}

	if ( Jedi_CultistDestroyer( NPCS.NPC ) )
	{
		Jedi_Advance();
		NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
	}

	if ( NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_DRAIN) &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
	{//when draining, don't move
		return;
	}
	else if ( !TIMER_Done( NPCS.NPC, "draining" ) )
	{//stopped draining, clear timers just in case
		TIMER_Set( NPCS.NPC, "draining", -level.time );
		TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}

	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( !TIMER_Done( NPCS.NPC, "flameTime" ) )
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
	else if ( NPCS.NPC->client->ps.saberInFlight &&
		!PM_SaberInBrokenParry( NPCS.NPC->client->ps.saberMove )
		&& NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{//maintain distance
		if ( enemy_dist < NPCS.NPC->client->ps.saberEntityDist )
		{
			Jedi_Retreat();
		}
		else if ( enemy_dist > NPCS.NPC->client->ps.saberEntityDist && enemy_dist > 100 )
		{
			Jedi_Advance();
		}
		if ( NPCS.NPC->client->ps.weapon == WP_SABER //using saber
			&& NPCS.NPC->client->ps.saberEntityState == SES_LEAVING  //not returning yet
			&& NPCS.NPC->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
			&& !(NPCS.NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
			//FIXME: time limit?
		}
	}
	else if ( !TIMER_Done( NPCS.NPC, "taunting" ) )
	{
		if ( enemy_dist <= 64 )
		{//he's getting too close
			NPCS.ucmd.buttons |= BUTTON_ATTACK;
			if ( !NPCS.NPC->client->ps.saberInFlight )
			{
				WP_ActivateSaber(NPCS.NPC);
			}
			TIMER_Set( NPCS.NPC, "taunting", -level.time );
		}
		//else if ( NPC->client->ps.torsoAnim == BOTH_GESTURE1 && NPC->client->ps.torsoTimer < 2000 )
		else if (NPCS.NPC->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT && (NPCS.NPC->client->ps.forceHandExtendTime - level.time) < 200)
		{//we're almost done with our special taunt
			//FIXME: this doesn't always work, for some reason
			if ( !NPCS.NPC->client->ps.saberInFlight )
			{
				WP_ActivateSaber(NPCS.NPC);
			}
		}
	}
	else if ( NPCS.NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage
		if ( enemy_dist > 0 )
		{//get closer so we can hit!
			Jedi_Advance();
		}
		if ( enemy_dist > 128 )
		{//lost 'em
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if ( NPCS.NPC->enemy->painDebounceTime + 2000 < level.time )
		{//the window of opportunity is gone
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set( NPCS.NPC, "strafeLeft", -1 );
		TIMER_Set( NPCS.NPC, "strafeRight", -1 );
	}
	else if ( NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time
		&& NPCS.NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		if ( enemy_dist < 64 )
		{//FIXME: maybe just pick another enemy?
			Jedi_Retreat();
		}
	}
	//rwwFIXMEFIXME: Give them the ability to do this again (turret needs to be fixed up to allow it)
	else if ( enemy_dist <= 64
		&& ((NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE)||(!Q_stricmp("Yoda",NPCS.NPC->NPC_type)&&!Q_irand(0,10))) )
	{//can't use saber and they're in striking range
		if ( !Q_irand( 0, 5 ) && InFront( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles, 0.2f ) )
		{
			if ( ((NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE)||NPCS.NPC->client->pers.maxHealth - NPCS.NPC->health > NPCS.NPC->client->pers.maxHealth*0.25f)//lost over 1/4 of our health or not firing
				&& NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
				&& WP_ForcePowerAvailable( NPCS.NPC, FP_DRAIN, 20 )//have enough power
				&& !Q_irand( 0, 2 ) )
			{//drain
				TIMER_Set( NPCS.NPC, "draining", 3000 );
				TIMER_Set( NPCS.NPC, "attackDelay", 3000 );
				Jedi_Advance();
				return;
			}
			else
			{
				ForceThrow( NPCS.NPC, qfalse );
			}
		}
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 64
		&& NPCS.NPC->client->pers.maxHealth - NPCS.NPC->health > NPCS.NPC->client->pers.maxHealth*0.25f//lost over 1/4 of our health
		&& NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
		&& WP_ForcePowerAvailable( NPCS.NPC, FP_DRAIN, 20 )//have enough power
		&& !Q_irand( 0, 10 )
		&& InFront( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.viewangles, 0.2f ) )
	{
		TIMER_Set( NPCS.NPC, "draining", 3000 );
		TIMER_Set( NPCS.NPC, "attackDelay", 3000 );
		Jedi_Advance();
		return;
	}
	else if ( enemy_dist <= -16 )
	{//we're too damn close!
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 0 )
	{//we're within striking range
		//if we are attacking, see if we should stop
		if ( NPCS.NPCInfo->stats.aggression < 4 )
		{//back off and defend
			Jedi_Retreat();
		}
	}
	else if ( enemy_dist > 256 )
	{//we're way out of range
		qboolean usedForce = qfalse;
		if ( NPCS.NPCInfo->stats.aggression < Q_irand( 0, 20 )
			&& NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth*0.75f
			&& !Q_irand( 0, 2 ) )
		{
			if ( (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceHeal( NPCS.NPC );
				usedForce = qtrue;
				//FIXME: check level of heal and know not to move or attack when healing
			}
			else if ( (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceProtect( NPCS.NPC );
				usedForce = qtrue;
			}
			else if ( (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceAbsorb( NPCS.NPC );
				usedForce = qtrue;
			}
			else if ( (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0
				&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0
				&& Q_irand( 0, 1 ) )
			{
				Jedi_Rage();
				usedForce = qtrue;
			}
			//FIXME: what about things like mind tricks and force sight?
		}
		if ( enemy_dist > 384 )
		{//FIXME: check for enemy facing away and/or moving away
			if ( !Q_irand( 0, 10 ) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time )
			{
				if ( NPC_ClearLOS4( NPCS.NPC->enemy ) )
				{
					G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 3000 );
				}
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
		//Unless we're totally hiding, go after him
		if ( NPCS.NPCInfo->stats.aggression > 0 )
		{//approach enemy
			if ( !usedForce )
			{
				Jedi_Advance();
			}
		}
	}
	//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
	else if ( enemy_dist > 50 )//FIXME: not hardcoded- base on our reach (modelScale?) and saberLengthMax
	{//we're out of striking range and we are allowed to attack
		//first, check some tactical force power decisions
		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && (NPCS.NPC->enemy->client->ps.fd.forceGripBeingGripped > level.time) )
		{//They're being gripped, rush them!
			if ( NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//they're on the ground, so advance
				if ( TIMER_Done( NPCS.NPC, "parryTime" ) || NPCS.NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) )
					{//far away or allowed to use saber
						Jedi_Advance();
					}
				}
			}
			if ( NPCS.NPCInfo->rank >= RANK_LT_JG
				&& !Q_irand( 0, 5 )
				&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
				&& !(NPCS.NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
			{//throw saber
				NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
			}
		}
		else if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && //valid enemy
			NPCS.NPC->enemy->client->ps.saberInFlight && /*NPC->enemy->client->ps.saber[0].Active()*/ NPCS.NPC->enemy->client->ps.saberEntityNum && //enemy throwing saber
			NPCS.NPC->client->ps.weaponTime <= 0 && //I'm not busy
			WP_ForcePowerAvailable( NPCS.NPC, FP_GRIP, 0 ) && //I can use the power
			!Q_irand( 0, 10 ) && //don't do it all the time, averages to 1 check a second
			Q_irand( 0, 6 ) < g_npcspskill.integer && //more likely on harder diff
			Q_irand( RANK_CIVILIAN, RANK_CAPTAIN ) < NPCS.NPCInfo->rank )//more likely against harder enemies
		{//They're throwing their saber, grip them!
			//taunt
			if ( TIMER_Done( NPCS.NPC, "chatter" ) && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time )
			{
				G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				TIMER_Set( NPCS.NPC, "chatter", 3000 );
			}

			//grip
			TIMER_Set( NPCS.NPC, "gripping", 3000 );
			TIMER_Set( NPCS.NPC, "attackDelay", 3000 );
		}
		else
		{
			int chanceScale;

			if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && (NPCS.NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_GRIP)) )
			{//They're choking someone, probably an ally, run at them and do some sort of attack
				if ( NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//they're on the ground, so advance
					if ( TIMER_Done( NPCS.NPC, "parryTime" ) || NPCS.NPCInfo->rank > RANK_LT )
					{//not parrying
						if ( enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) )
						{//far away or allowed to use saber
							Jedi_Advance();
						}
					}
				}
			}
			chanceScale = 0;
			if ( NPCS.NPC->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",NPCS.NPC->NPC_type) )
			{
				chanceScale = 1;
			}
			else if ( NPCS.NPCInfo->rank == RANK_ENSIGN )
			{
				chanceScale = 2;
			}
			else if ( NPCS.NPCInfo->rank >= RANK_LT_JG )
			{
				chanceScale = 5;
			}
			if ( chanceScale
				&& (enemy_dist > Q_irand( 100, 200 ) || (NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) || (!Q_stricmp("Yoda",NPCS.NPC->NPC_type)&&!Q_irand(0,3)) )
				&& enemy_dist < 500
				&& (Q_irand( 0, chanceScale*10 )<5 || (NPCS.NPC->enemy->client && NPCS.NPC->enemy->client->ps.weapon != WP_SABER && !Q_irand( 0, chanceScale ) ) ) )
			{//else, randomly try some kind of attack every now and then
				if ( ((NPCS.NPCInfo->rank == RANK_ENSIGN || NPCS.NPCInfo->rank > RANK_LT_JG) && !Q_irand( 0, 1 )) || NPCS.NPC->s.weapon != WP_SABER )
				{
					if ( WP_ForcePowerUsable( NPCS.NPC, FP_PULL ) && !Q_irand( 0, 2 ) )
					{
						//force pull the guy to me!
						//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
						ForceThrow( NPCS.NPC, qtrue );
						//maybe strafe too?
						TIMER_Set( NPCS.NPC, "duck", enemy_dist*3 );
						if ( Q_irand( 0, 1 ) )
						{
							NPCS.ucmd.buttons |= BUTTON_ATTACK;
						}
					}
					else if ( (WP_ForcePowerUsable( NPCS.NPC, FP_LIGHTNING )
						&& ((NPCS.NPCInfo->scriptFlags & SCF_DONT_FIRE) && Q_stricmp("cultist_lightning",NPCS.NPC->NPC_type))) || Q_irand( 0, 1 ))
					{
						ForceLightning( NPCS.NPC );
						if ( NPCS.NPC->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
						{
							NPCS.NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_npcspskill.integer*500) );
							TIMER_Set( NPCS.NPC, "holdLightning", NPCS.NPC->client->ps.weaponTime );
						}
						TIMER_Set( NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime );
					}
					//rwwFIXMEFIXME: After new drain stuff from SP is in re-enable this.
					else if ( (NPCS.NPC->health < NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH] * 0.75f
							&& Q_irand( FORCE_LEVEL_0, NPCS.NPC->client->ps.fd.forcePowerLevel[FP_DRAIN] ) > FORCE_LEVEL_1
							&& WP_ForcePowerUsable( NPCS.NPC, FP_DRAIN )
							&& ((NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE)&&Q_stricmp("cultist_drain",NPCS.NPC->NPC_type))) || Q_irand( 0, 1 ) )
						{
							ForceDrain( NPCS.NPC );
							NPCS.NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_npcspskill.integer*500) );
							TIMER_Set( NPCS.NPC, "draining", NPCS.NPC->client->ps.weaponTime );
							TIMER_Set( NPCS.NPC, "attackDelay", NPCS.NPC->client->ps.weaponTime );
						}
						else if ( WP_ForcePowerUsable( NPCS.NPC, FP_GRIP )
							&& NPCS.NPC->enemy && InFOV( NPCS.NPC->enemy, NPCS.NPC, 20, 30 ) )
						{
							//taunt
							if ( TIMER_Done( NPCS.NPC, "chatter" ) && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time )
							{
								G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
								jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
								TIMER_Set( NPCS.NPC, "chatter", 3000 );
							}

							//grip
							TIMER_Set( NPCS.NPC, "gripping", 3000 );
							TIMER_Set( NPCS.NPC, "attackDelay", 3000 );
						}
					else
					{
						if ( WP_ForcePowerUsable( NPCS.NPC, FP_SABERTHROW )
							&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
							&& !(NPCS.NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
						{//throw saber
							NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
						}
					}
				}
				else
				{
					if ( NPCS.NPCInfo->rank >= RANK_LT_JG
						&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
						&& !(NPCS.NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
					{//throw saber
						NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
					}
				}
			}
			//see if we should advance now
			else if ( NPCS.NPCInfo->stats.aggression > 5 )
			{//approach enemy
				if ( TIMER_Done( NPCS.NPC, "parryTime" ) || NPCS.NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( !NPCS.NPC->enemy->client || NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//they're on the ground, so advance
						if ( enemy_dist > 200 || !(NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) )
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
	else
	{//we're not close enough to attack, but not far enough away to be safe
		if ( NPCS.NPCInfo->stats.aggression < 4 )
		{//back off and defend
			Jedi_Retreat();
		}
		else if ( NPCS.NPCInfo->stats.aggression > 5 )
		{//try to get closer
			if ( enemy_dist > 0 && !(NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE))
			{//we're allowed to use our lightsaber, get closer
				if ( TIMER_Done( NPCS.NPC, "parryTime" ) || NPCS.NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( !NPCS.NPC->enemy->client || NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
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
	if ( NPCS.NPCInfo->stats.aggression > Q_irand( 5, 15 )
		&& NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth*0.75f
		&& !Q_irand( 0, 2 ) )
	{
		if ( (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0
			&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0 )
		{
			Jedi_Rage();
		}
	}
}

static qboolean Jedi_Strafe( int strafeTimeMin, int strafeTimeMax, int nextStrafeTimeMin, int nextStrafeTimeMax, qboolean walking )
{
	if( Jedi_CultistDestroyer( NPCS.NPC ) )
	{
		return qfalse;
	}
	if ( NPCS.NPC->client->ps.saberEventFlags&SEF_LOCK_WON && NPCS.NPC->enemy && NPCS.NPC->enemy->painDebounceTime > level.time )
	{//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if ( TIMER_Done( NPCS.NPC, "strafeLeft" ) && TIMER_Done( NPCS.NPC, "strafeRight" ) )
	{
		qboolean strafed = qfalse;
		//TODO: make left/right choice a tactical decision rather than random:
		//		try to keep own back away from walls and ledges,
		//		try to keep enemy's back to a ledge or wall
		//		Maybe try to strafe toward designer-placed "safe spots" or "goals"?
		int	strafeTime = Q_irand( strafeTimeMin, strafeTimeMax );

		if ( Q_irand( 0, 1 ) )
		{
			if ( NPC_MoveDirClear( NPCS.ucmd.forwardmove, -127, qfalse ) )
			{
				TIMER_Set( NPCS.NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( NPCS.ucmd.forwardmove, 127, qfalse ) )
			{
				TIMER_Set( NPCS.NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
		}
		else
		{
			if ( NPC_MoveDirClear( NPCS.ucmd.forwardmove, 127, qfalse  ) )
			{
				TIMER_Set( NPCS.NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( NPCS.ucmd.forwardmove, -127, qfalse  ) )
			{
				TIMER_Set( NPCS.NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
		}

		if ( strafed )
		{
			TIMER_Set( NPCS.NPC, "noStrafe", strafeTime + Q_irand( nextStrafeTimeMin, nextStrafeTimeMax ) );
			if ( walking )
			{//should be a slow strafe
				TIMER_Set( NPCS.NPC, "walking", strafeTime );
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
//rwwFIXMEFIXME: Going to use qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc ) from
//w_saber.c.. maybe use seperate one for NPCs or add cases to that one?

evasionType_t Jedi_CheckFlipEvasions( gentity_t *self, float rightdot, float zdiff )
{
	if ( self->NPC && (self->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return EVASION_NONE;
	}
	if ( self->client
		&& (self->client->ps.fd.forceRageRecoveryTime > level.time	|| (self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))) )
	{//no fancy dodges when raging
		return EVASION_NONE;
	}
	//Check for:
	//ARIALS/CARTWHEELS
	//WALL-RUNS
	//WALL-FLIPS
	//FIXME: if facing a wall, do forward wall-walk-backflip
	if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
	{//already running on a wall
		vec3_t right, fwdAngles;
		int		anim = -1;
		float animLength;

		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, NULL, right, NULL );

		animLength = BG_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim );
		if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0 )
		{//I'm running on a wall to my left and the attack is on the left
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if ( self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0 )
		{//I'm running on a wall to my right and the attack is on the right
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if ( anim != -1 )
		{//flip off the wall!
			int parts;
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
			parts = SETANIM_LEGS;
			if ( !self->client->ps.weaponTime )
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
			//rwwFIXMEFIXME: Add these pm flags?
			G_AddEvent( self, EV_JUMP, 0 );
			return EVASION_OTHER;
		}
	}
	else if ( self->client->NPC_class != CLASS_DESANN //desann doesn't do these kind of frilly acrobatics
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT)
		&& Q_irand( 0, 1 )
		&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
		trace_t	trace;
		int parts, anim;
		float	speed, checkDist;
		qboolean allowCartWheels = qtrue;
		qboolean allowWallFlips = qtrue;

		if ( self->client->ps.weapon == WP_SABER )
		{
			if ( self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			else if ( self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			if ( self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
			else if ( self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
		}

		VectorSet(mins, self->r.mins[0],self->r.mins[1],0);
		VectorSet(maxs, self->r.maxs[0],self->r.maxs[1],24);
		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, fwd, right, NULL );

		parts = SETANIM_BOTH;

		if ( BG_SaberInAttack( self->client->ps.saberMove )
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
		VectorMA( self->r.currentOrigin, checkDist, right, traceto );
		trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
		if ( trace.fraction >= 1.0f && allowCartWheels )
		{//it's clear, let's do it
			//FIXME: check for drops?
			vec3_t jumpRt;

			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.weaponTime = self->client->ps.legsTimer;//don't attack again until this anim is done
			VectorCopy( self->client->ps.viewangles, fwdAngles );
			fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
			//do the flip
			AngleVectors( fwdAngles, NULL, jumpRt, NULL );
			VectorScale( jumpRt, speed, self->client->ps.velocity );
			self->client->ps.fd.forceJumpCharge = 0;//so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
			//self->client->ps.pm_flags |= PMF_JUMPING;
			if ( self->client->NPC_class == CLASS_BOBAFETT )
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
			gentity_t *traceEnt;

			VectorSubtract( self->r.currentOrigin, traceto, idealNormal );
			VectorNormalize( idealNormal );
			traceEnt = &g_entities[trace.entityNum];
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
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
						if ( trace.fraction >= 1.0f )
						{//it's clear, let's do it
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
								parts = SETANIM_LEGS;
								if ( !self->client->ps.weaponTime )
								{
									parts = SETANIM_BOTH;
								}
								NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
								//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
								if ( self->client->NPC_class == CLASS_BOBAFETT )
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
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap->Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP, qfalse, 0, 0 );
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
						if ( self->client->saber[0].model[0]
							&& (self->client->saber[0].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
						else if ( self->client->saber[1].model[0]
							&& (self->client->saber[1].saberFlags&SFL_NO_WALL_RUNS) )
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
						parts = SETANIM_LEGS;
						if ( !self->client->ps.weaponTime )
						{
							parts = SETANIM_BOTH;
						}
						NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
						//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
						if ( self->client->NPC_class == CLASS_BOBAFETT )
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
	if ( self->s.number >= 0 && self->s.number < MAX_CLIENTS )
	{//player
		return bg_parryDebounce[self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]];
	}
	else if ( self->NPC )
	{
		if ( !g_saberRealisticCombat.integer
			&& ( g_npcspskill.integer == 2 || (g_npcspskill.integer == 1 && self->client->NPC_class == CLASS_TAVION) ) )
		{
			if ( self->client->NPC_class == CLASS_TAVION )
			{
				return 0;
			}
			else
			{
				return Q_irand( 0, 150 );
			}
		}
		else
		{
			int	baseTime;
			if ( evasionType == EVASION_DODGE )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( evasionType == EVASION_CARTWHEEL )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( self->client->ps.saberInFlight )
			{
				baseTime = Q_irand( 1, 3 ) * 50;
			}
			else
			{
				if ( g_saberRealisticCombat.integer )
				{
					baseTime = 500;

					switch ( g_npcspskill.integer )
					{
					case 0:
						baseTime = 500;
						break;
					case 1:
						baseTime = 300;
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

					switch ( g_npcspskill.integer )
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

				if ( self->client->NPC_class == CLASS_TAVION )
				{//Tavion is faster
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
					baseTime += 100;
				}
				else if ( evasionType == EVASION_JUMP || evasionType == EVASION_JUMP_PARRY )
				{
					baseTime += 50;
				}
				else if ( evasionType == EVASION_OTHER )
				{
					baseTime += 100;
				}
				else if ( evasionType == EVASION_FJUMP )
				{
					baseTime += 100;
				}
			}

			return baseTime;
		}
	}
	return 0;
}

qboolean Jedi_QuickReactions( gentity_t *self )
{
	if ( ( self->client->NPC_class == CLASS_JEDI && NPCS.NPCInfo->rank == RANK_COMMANDER ) ||
		self->client->NPC_class == CLASS_TAVION ||
		(self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_1&&g_npcspskill.integer>1) ||
		(self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_2&&g_npcspskill.integer>0) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_SaberBusy( gentity_t *self )
{
	if ( self->client->ps.torsoTimer > 300
	&& ( (BG_SaberInAttack( self->client->ps.saberMove )&&self->client->ps.fd.saberAnimLevel==FORCE_LEVEL_3)
		|| BG_SpinningSaberAnim( self->client->ps.torsoAnim )
		|| BG_SaberInSpecialAttack( self->client->ps.torsoAnim )
		//|| PM_SaberInBounce( self->client->ps.saberMove )
		|| PM_SaberInBrokenParry( self->client->ps.saberMove )
		//|| PM_SaberInDeflect( self->client->ps.saberMove )
		|| BG_FlippingAnim( self->client->ps.torsoAnim )
		|| PM_RollingAnim( self->client->ps.torsoAnim ) ) )
	{//my saber is not in a parrying position
		return qtrue;
	}
	return qfalse;
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
evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist ) //dist = 0.0f
{
	vec3_t hitloc, hitdir, diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;
	int	  duckChance = 0;
	int	  dodgeAnim = -1;
	qboolean	saberBusy = qfalse, doDodge = qfalse;
	evasionType_t	evasionType = EVASION_NONE;

	//FIXME: if we don't have our saber in hand, pick the force throw option or a jump or strafe!
	//FIXME: reborn don't block enough anymore
	if ( !incoming )
	{
		VectorCopy( pHitloc, hitloc );
		VectorCopy( phitDir, hitdir );
		//FIXME: maybe base this on rank some?  And/or g_npcspskill?
		if ( self->client->ps.saberInFlight )
		{//DOH!  do non-saber evasion!
			saberBusy = qtrue;
		}
		else if ( Jedi_QuickReactions( self ) )
		{//jedi trainer and tavion are must faster at parrying and can do it whenever they like
			//Also, on medium, all level 3 people can parry any time and on hard, all level 2 or 3 people can parry any time
		}
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
		VectorCopy( incoming->r.currentOrigin, hitloc );
		VectorNormalize2( incoming->s.pos.trDelta, hitdir );
	}
	if ( self->client && self->client->NPC_class == CLASS_BOBAFETT )
	{
		saberBusy = qtrue;
	}

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	//VectorNormalize( diff );
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);// + flrand(-0.10f,0.10f);
	//totalHeight = self->client->renderInfo.eyePoint[2] - self->r.absmin[2];
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];// + Q_irand(-6,6);

	//see if we can dodge if need-be
	if ( (dist>16&&(Q_irand( 0, 2 )||saberBusy))
		|| self->client->ps.saberInFlight
		|| BG_SabersOff( &self->client->ps )
		|| self->client->NPC_class == CLASS_BOBAFETT )
	{//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
		if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT_JG) )
		{//acrobat or fencer or above
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE &&//on the ground
				!(self->client->ps.pm_flags&PMF_DUCKED)&&cmd->upmove>=0&&TIMER_Done( self, "duck" )//not ducking
				&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )//not rolling
				&& !PM_InKnockDown( &self->client->ps )//not knocked down
				&& ( self->client->ps.saberInFlight ||
					self->client->NPC_class == CLASS_BOBAFETT ||
					(!BG_SaberInAttack( self->client->ps.saberMove )//not attacking
					&& !PM_SaberInStart( self->client->ps.saberMove )//not starting an attack
					&& !BG_SpinningSaberAnim( self->client->ps.torsoAnim )//not in a saber spin
					&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ))//not in a special attack
					)
				)
			{//need to check all these because it overrides both torso and legs with the dodge
				doDodge = qtrue;
			}
		}
	}
	// Figure out what quadrant the block was in.
	if ( d_JediAI.integer )
	{
		Com_Printf( "(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, hitloc[2]-self->r.absmin[2],zdiff,rightdot);
	}

	//UL = > -1//-6
	//UR = > -6//-9
	//TOP = > +6//+4
	//FIXME: take FP_SABER_DEFENSE into account here somehow?
	if ( zdiff >= -5 )//was 0
	{
		if ( incoming || !saberBusy )
		{
			if ( rightdot > 12
				|| (rightdot > 3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, 0.3
			{//coming from right
				if ( doDodge )
				{
					if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
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
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UR block\n" );
				}
			}
			else if ( rightdot < -12
				|| (rightdot < -3 && zdiff < 5)
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, -0.3
			{//coming from left
				if ( doDodge )
				{
					if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
					{//roll!
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
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UL block\n" );
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "TOP block\n" );
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
			}
			else
			{//in air!  Ducking does no good
			}
		}
		if ( incoming || !saberBusy )
		{
			if ( rightdot > 8 || (rightdot > 3 && zdiff < -11) )//was normalized, 0.2
			{
				if ( doDodge )
				{
					if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UR block\n" );
				}
			}
			else if ( rightdot < -8 || (rightdot < -3 && zdiff < -11) )//was normalized, -0.2
			{
				if ( doDodge )
				{
					if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UL block\n" );
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-TOP block\n" );
				}
			}
		}
	}
	else if ( saberBusy || (zdiff < -36 && ( zdiff < -44 || !Q_irand( 0, 2 ) ) ) )//was -30 and -40//2nd one was -46
	{//jump!
		if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//already in air, duck to pull up legs
			TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
			evasionType = EVASION_DUCK;
			if ( d_JediAI.integer )
			{
				Com_Printf( "legs up\n" );
			}
			if ( incoming || !saberBusy )
			{
				//since the jump may be cleared if not safe, set a lower block too
				if ( rightdot >= 0 )
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					evasionType = EVASION_DUCK_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LR block\n" );
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					evasionType = EVASION_DUCK_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LL block\n" );
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
					&& self->client->ps.fd.forceRageRecoveryTime < level.time
					&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
					&& !PM_InKnockDown( &self->client->ps ) )
				{
					self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
					evasionType = EVASION_FJUMP;
					if ( d_JediAI.integer )
					{
						Com_Printf( "force jump + " );
					}
				}
			}
			else
			{//normal jump
				//FIXME: check the jump, if can't, then block
				if ( self->NPC
					&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
					&& self->client->ps.fd.forceRageRecoveryTime < level.time
					&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) )
				{
					if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 1 ) )
					{//roll!
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
						if ( self == NPCS.NPC )
						{
							cmd->upmove = 127;
						}
						else
						{
							self->client->ps.velocity[2] = JUMP_VELOCITY;
						}
					}
					evasionType = EVASION_JUMP;
					if ( d_JediAI.integer )
					{
						Com_Printf( "jump + " );
					}
				}
				if ( self->client->NPC_class == CLASS_TAVION )
				{
					if ( !incoming
						&& self->client->ps.groundEntityNum < ENTITYNUM_NONE
						&& !Q_irand( 0, 2 ) )
					{
						if ( !BG_SaberInAttack( self->client->ps.saberMove )
							&& !PM_SaberInStart( self->client->ps.saberMove )
							&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
							&& !PM_InKnockDown( &self->client->ps )
							&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
						{//do the butterfly!
							int butterflyAnim;
							if ( Q_irand( 0, 1 ) )
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
							self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
						//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
						//	self->client->ps.SaberActivateTrail( 300 );//FIXME: reset this when done!
							//Ah well. No hacking from the server for now.
							if ( self->client->NPC_class == CLASS_BOBAFETT )
							{
								G_AddEvent( self, EV_JUMP, 0 );
							}
							else
							{
								G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jump.wav") );
							}
							cmd->upmove = 0;
							saberBusy = qtrue;
						}
					}
				}
			}
			if ( ((evasionType = Jedi_CheckFlipEvasions( self, rightdot, zdiff ))!=EVASION_NONE) )
			{
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
					if ( d_JediAI.integer )
					{
						Com_Printf( "LR block\n" );
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
					if ( d_JediAI.integer )
					{
						Com_Printf( "LL block\n" );
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "LR block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
				evasionType = EVASION_PARRY;
				if ( d_JediAI.integer )
				{
					Com_Printf( "LL block\n" );
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
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
						&& !PM_InKnockDown( &self->client->ps ) )
					{
						self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
						evasionType = EVASION_FJUMP;
						if ( d_JediAI.integer )
						{
							Com_Printf( "force jump + " );
						}
					}
				}
				else
				{//normal jump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS)
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)))
					{
						if ( self == NPCS.NPC )
						{
							cmd->upmove = 127;
						}
						else
						{
							self->client->ps.velocity[2] = JUMP_VELOCITY;
						}
						evasionType = EVASION_JUMP_PARRY;
						if ( d_JediAI.integer )
						{
							Com_Printf( "jump + " );
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
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
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
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
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
		if ( self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
	return evasionType;
}

extern float ShortestLineSegBewteen2LineSegs( vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2 );
extern int WPDEBUG_SaberColor( saber_colors_t saberColor );
static qboolean Jedi_SaberBlock( int saberNum, int bladeNum ) //saberNum = 0, bladeNum = 0
{
	vec3_t hitloc, saberTipOld, saberTip, top, bottom, axisPoint, saberPoint, dir;//saberBase,
	vec3_t pointDir, baseDir, tipDir, saberHitPoint, saberMins, saberMaxs;
	float	pointDist, baseDirPerc, dist;
	float	bladeLen = 0;
	trace_t	tr;
	evasionType_t	evasionType;

	//FIXME: reborn don't block enough anymore
	/*
	//maybe do this on easy only... or only on grunt-level reborn
	if ( NPC->client->ps.weaponTime )
	{//i'm attacking right now
		return qfalse;
	}
	*/

	if ( !TIMER_Done( NPCS.NPC, "parryReCalcTime" ) )
	{//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if ( NPCS.NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't move the saber to another position yet
		return qfalse;
	}

	/*
	if ( NPCInfo->rank < RANK_LT_JG && Q_irand( 0, (2 - g_npcspskill.integer) ) )
	{//lower rank reborn have a random chance of not doing it at all
		NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 300;
		return qfalse;
	}
	*/

	if ( NPCS.NPC->enemy->health <= 0 || !NPCS.NPC->enemy->client )
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

	VectorCopy( NPC->r.currentOrigin, top );
	top[2] = NPC->r.absmax[2];
	VectorCopy( NPC->r.currentOrigin, bottom );
	bottom[2] = NPC->r.absmin[2];

	float dist = ShortestLineSegBewteen2LineSegs( saberBase, saberTipOld, bottom, top, saberPoint, axisPoint );
	if ( 0 )//dist > NPC->r.maxs[0]*4 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( "enemy saber dist: %4.2f\n", dist );
		}
		TIMER_Set( NPC, "parryTime", -1 );
		return qfalse;
	}

	//get the actual point of impact
	trace_t	tr;
	trap->Trace( &tr, saberPoint, vec3_origin, vec3_origin, axisPoint, NPC->enemy->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid )
	{//estimate
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}
	*/
	VectorSet(saberMins,-4,-4,-4);
	VectorSet(saberMaxs,4,4,4);

	VectorMA( NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePointOld, NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDirOld, saberTipOld );
	VectorMA( NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePoint, NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDir, saberTip );
//	VectorCopy(NPC->enemy->client->lastSaberBase_Always, muzzlePoint);
//	VectorMA(muzzlePoint, GAME_SABER_LENGTH, NPC->enemy->client->lastSaberDir_Always, saberTip);
//	VectorCopy(saberTip, saberTipOld);

	VectorCopy( NPCS.NPC->r.currentOrigin, top );
	top[2] = NPCS.NPC->r.absmax[2];
	VectorCopy( NPCS.NPC->r.currentOrigin, bottom );
	bottom[2] = NPCS.NPC->r.absmin[2];

	dist = ShortestLineSegBewteen2LineSegs( NPCS.NPC->enemy->client->renderInfo.muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
	if ( dist > NPCS.NPC->r.maxs[0]*5 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( S_COLOR_RED"enemy saber dist: %4.2f\n", dist );
		}
		/*
		if ( dist < 300 //close
			&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
			&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
		{//he's swinging at me and close enough to be a threat, don't start an attack right now
			TIMER_Set( NPC, "parryTime", 100 );
		}
		else
		*/
		{
			TIMER_Set( NPCS.NPC, "parryTime", -1 );
		}
		return qfalse;
	}
	if ( d_JediAI.integer )
	{
		Com_Printf( S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist );
	}

	VectorSubtract( saberPoint, NPCS.NPC->enemy->client->renderInfo.muzzlePoint, pointDir );
	pointDist = VectorLength( pointDir );

	bladeLen = NPCS.NPC->enemy->client->saber[saberNum].blade[bladeNum].length;

	if ( bladeLen <= 0 )
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist/bladeLen;
	}
	VectorSubtract( NPCS.NPC->enemy->client->renderInfo.muzzlePoint, NPCS.NPC->enemy->client->renderInfo.muzzlePointOld, baseDir );
	VectorSubtract( saberTip, saberTipOld, tipDir );
	VectorScale( baseDir, baseDirPerc, baseDir );
	VectorMA( baseDir, 1.0f-baseDirPerc, tipDir, dir );
	VectorMA( saberPoint, 200, dir, hitloc );

	//get the actual point of impact
	trap->Trace( &tr, saberPoint, saberMins, saberMaxs, hitloc, NPCS.NPC->enemy->s.number, CONTENTS_BODY, qfalse, 0, 0 );//, G2_RETURNONHIT, 10 );
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
				&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
			{//he's swinging at me and close enough to be a threat, don't start an attack right now
				TIMER_Set( NPC, "parryTime", 100 );
			}
			else
			*/
			{
				TIMER_Set( NPCS.NPC, "parryTime", -1 );
			}
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs( saberPoint, hitloc, bottom, top, saberHitPoint, hitloc );
		/*
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
		*/
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}

	if ( d_JediAI.integer )
	{
		//G_DebugLine( saberPoint, hitloc, FRAMETIME, WPDEBUG_SaberColor( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].color ), qtrue );
		G_TestLine(saberPoint, hitloc, 0x0000ff, FRAMETIME);
	}

	//FIXME: if saber is off and/or we have force speed and want to be really cocky,
	//		and the swing misses by some amount, we can use the dodges here... :)
	if ( (evasionType=Jedi_SaberBlockGo( NPCS.NPC, &NPCS.ucmd, hitloc, dir, NULL, dist )) != EVASION_DODGE )
	{//we did block (not dodge)
		int parryReCalcTime;

		if ( !NPCS.NPC->client->ps.saberInFlight )
		{//make sure saber is on
			WP_ActivateSaber(NPCS.NPC);
		}

		//debounce our parry recalc time
		parryReCalcTime = Jedi_ReCalcParryTime( NPCS.NPC, evasionType );
		TIMER_Set( NPCS.NPC, "parryReCalcTime", Q_irand( 0, parryReCalcTime ) );
		if ( d_JediAI.integer )
		{
			Com_Printf( "Keep parry choice until: %d\n", level.time + parryReCalcTime );
		}

		//determine how long to hold this anim
		if ( TIMER_Done( NPCS.NPC, "parryTime" ) )
		{
			if ( NPCS.NPC->client->NPC_class == CLASS_TAVION )
			{
				TIMER_Set( NPCS.NPC, "parryTime", Q_irand( parryReCalcTime/2, parryReCalcTime*1.5 ) );
			}
			else if ( NPCS.NPCInfo->rank >= RANK_LT_JG )
			{//fencers and higher hold a parry less
				TIMER_Set( NPCS.NPC, "parryTime", parryReCalcTime );
			}
			else
			{//others hold it longer
				TIMER_Set( NPCS.NPC, "parryTime", Q_irand( 1, 2 )*parryReCalcTime );
			}
		}
	}
	else
	{
		int dodgeTime = NPCS.NPC->client->ps.torsoTimer;
		if ( NPCS.NPCInfo->rank > RANK_LT_COMM && NPCS.NPC->client->NPC_class != CLASS_DESANN )
		{//higher-level guys can dodge faster
			dodgeTime -= 200;
		}
		TIMER_Set( NPCS.NPC, "parryReCalcTime", dodgeTime );
		TIMER_Set( NPCS.NPC, "parryTime", dodgeTime );
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

	if ( !NPCS.NPC->enemy->client )
	{
		return;
	}
	else if ( NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time )
	{//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	else if ( NPCS.NPC->client->ps.saberEventFlags&SEF_LOCK_WON && NPCS.NPC->enemy->painDebounceTime > level.time )
	{//pressing the advantage of winning a saber lock
		return;
	}

	if ( NPCS.NPC->enemy->client->ps.saberInFlight && !TIMER_Done( NPCS.NPC, "taunting" ) )
	{//if he's throwing his saber, stop taunting
		TIMER_Set( NPCS.NPC, "taunting", -level.time );
		if ( !NPCS.NPC->client->ps.saberInFlight )
		{
			WP_ActivateSaber(NPCS.NPC);
		}
	}

	if ( TIMER_Done( NPCS.NPC, "parryTime" ) )
	{
		if ( NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if ( NPCS.NPC->enemy->client->ps.weaponTime && NPCS.NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{
		if ( !NPCS.NPC->client->ps.saberInFlight && Jedi_SaberBlock(0, 0) )
		{
			return;
		}
	}

	VectorSubtract( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, dirEnemy2Me );
	VectorNormalize( dirEnemy2Me );

	if ( NPCS.NPC->enemy->client->ps.weaponTime && NPCS.NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{//enemy is attacking
		enemy_attacking = qtrue;
		evasionChance = 90;
	}

	if ( (NPCS.NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_LIGHTNING) ) )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasionChance = 50;
	}

	if ( NPCS.NPC->enemy->client->ps.saberInFlight && NPCS.NPC->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE && NPCS.NPC->enemy->client->ps.saberEntityState != SES_RETURNING )
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
			AngleVectors( NPCS.NPC->enemy->client->ps.viewangles, enemy_fwd, NULL, NULL );
			facingAmt = DotProduct( enemy_fwd, dirEnemy2Me );
		}
		else
		{//he's moving
			facingAmt = DotProduct( enemy_movedir, dirEnemy2Me );
		}

		if ( flrand( 0.25, 1 ) < facingAmt )
		{//coming at/facing me!
			int whichDefense = 0;
			if ( NPCS.NPC->client->ps.weaponTime || NPCS.NPC->client->ps.saberInFlight || NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
			{//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if ( Q_irand( 0, 10 ) < NPCS.NPCInfo->stats.aggression )
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
					gentity_t *saber = &g_entities[NPCS.NPC->enemy->client->ps.saberEntityNum];
					VectorSubtract( NPCS.NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
					saberDist = VectorNormalize( saberDir2Me );
					VectorCopy( saber->s.pos.trDelta, saberMoveDir );
					VectorNormalize( saberMoveDir );
					if ( !Q_irand( 0, 3 ) )
					{
						//Com_Printf( "(%d) raise agg - enemy threw saber\n", level.time );
						Jedi_Aggression( NPCS.NPC, 1 );
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
					if ( Q_irand( 0, 10 ) < NPCS.NPCInfo->stats.aggression )
					{
						return;
					}
					whichDefense = 100;
				}
				else
				{//he's getting close and swinging at me
					vec3_t	fwd;
					//see if I'm facing him
					AngleVectors( NPCS.NPC->client->ps.viewangles, fwd, NULL, NULL );
					if ( DotProduct( enemy_dir, fwd ) < 0.5 )
					{//I'm not really facing him, best option is to strafe
						whichDefense = Q_irand( 5, 16 );
					}
					else if ( enemy_dist < 56 )
					{//he's very close, maybe we should be more inclined to block or throw
						whichDefense = Q_irand( NPCS.NPCInfo->stats.aggression, 12 );
					}
					else
					{
						whichDefense = Q_irand( 2, 16 );
					}
				}
			}

			if ( whichDefense >= 4 && whichDefense <= 12 )
			{//would try to block
				if ( NPCS.NPC->client->ps.saberInFlight )
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
				//use jedi force push?
				//FIXME: try to do this if health low or enemy back to a cliff?
				if ( (NPCS.NPCInfo->rank == RANK_ENSIGN || NPCS.NPCInfo->rank > RANK_LT_JG) && TIMER_Done( NPCS.NPC, "parryTime" ) )
				{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
					ForceThrow( NPCS.NPC, qfalse );
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
				//Com_Printf( "blocking\n" );
				Jedi_SaberBlock(0, 0);
				break;
			default:
				//Evade!
				//start a strafe left/right if not already
				if ( !Q_irand( 0, 5 ) || !Jedi_Strafe( 300, 1000, 0, 1000, qfalse ) )
				{//certain chance they will pick an alternative evasion
					//if couldn't strafe, try a different kind of evasion...
					if ( shooting_lightning || throwing_saber || enemy_dist < 80 )
					{
						//FIXME: force-jump+forward - jump over the guy!
						if ( shooting_lightning || (!Q_irand( 0, 2 ) && NPCS.NPCInfo->stats.aggression < 4 && TIMER_Done( NPCS.NPC, "parryTime" ) ) )
						{
							if ( (NPCS.NPCInfo->rank == RANK_ENSIGN || NPCS.NPCInfo->rank > RANK_LT_JG) && !shooting_lightning && Q_irand( 0, 2 ) )
							{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
								ForceThrow( NPCS.NPC, qfalse );
							}
							else if ( (NPCS.NPCInfo->rank==RANK_CREWMAN||NPCS.NPCInfo->rank>RANK_LT_JG)
								&& !(NPCS.NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
								&& NPCS.NPC->client->ps.fd.forceRageRecoveryTime < level.time
								&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
								&& !PM_InKnockDown( &NPCS.NPC->client->ps ) )
							{//FIXME: make this a function call?
								//FIXME: check for clearance, safety of landing spot?
								NPCS.NPC->client->ps.fd.forceJumpCharge = 480;
								//Don't jump again for another 2 to 5 seconds
								TIMER_Set( NPCS.NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
								if ( Q_irand( 0, 2 ) )
								{
									NPCS.ucmd.forwardmove = 127;
									VectorClear( NPCS.NPC->client->ps.moveDir );
								}
								else
								{
									NPCS.ucmd.forwardmove = -127;
									VectorClear( NPCS.NPC->client->ps.moveDir );
								}
								//FIXME: if this jump is cleared, we can't block... so pick a random lower block?
								if ( Q_irand( 0, 1 ) )//FIXME: make intelligent
								{
									NPCS.NPC->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
								}
								else
								{
									NPCS.NPC->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
								}
							}
						}
						else if ( enemy_attacking )
						{
							Jedi_SaberBlock(0, 0);
						}
					}
				}
				else
				{//strafed
					if ( d_JediAI.integer )
					{
						Com_Printf( "def strafe\n" );
					}
					if ( !(NPCS.NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
						&& NPCS.NPC->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE))
						&& (NPCS.NPCInfo->rank == RANK_CREWMAN || NPCS.NPCInfo->rank > RANK_LT_JG )
						&& !PM_InKnockDown( &NPCS.NPC->client->ps )
						&& !Q_irand( 0, 5 ) )
					{//FIXME: make this a function call?
						//FIXME: check for clearance, safety of landing spot?
						if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
						{
							NPCS.NPC->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
						}
						else
						{
							NPCS.NPC->client->ps.fd.forceJumpCharge = 320;
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPCS.NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
					}
				}
				break;
			}

			//turn off slow walking no matter what
			TIMER_Set( NPCS.NPC, "walking", -level.time );
			TIMER_Set( NPCS.NPC, "taunting", -level.time );
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
	int			entityList[MAX_GENTITIES];
	int			e, numListedEntities;
	trace_t		tr;

	if ( !self->client )
	{
		return enemy;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	for ( e = 0 ; e < 3 ; e++ )
	{
		mins[e] = self->r.currentOrigin[e] - 1024;
		maxs[e] = self->r.currentOrigin[e] + 1024;
	}
	numListedEntities = trap->EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		check = &g_entities[entityList[e]];
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

		if ( !trap->InPVS( check->r.currentOrigin, self->r.currentOrigin ) )
		{//can't potentially see them
			continue;
		}

		VectorSubtract( check->r.currentOrigin, self->r.currentOrigin, dir );
		dist = VectorNormalize( dir );

		if ( DotProduct( dir, forward ) < minDot )
		{//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		trap->Trace( &tr, self->r.currentOrigin, vec3_origin, vec3_origin, check->r.currentOrigin, self->s.number, MASK_SHOT, qfalse, 0, 0 );
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
	if ( !NPCS.NPC || !NPCS.NPC->enemy )
	{//no valid enemy
		return;
	}
	if ( !NPCS.NPC->enemy->client )
	{
		VectorClear( enemy_movedir );
		*enemy_movespeed = 0;
		VectorCopy( NPCS.NPC->enemy->r.currentOrigin, enemy_dest );
		enemy_dest[2] += NPCS.NPC->enemy->r.mins[2] + 24;//get it's origin to a height I can work with
		VectorSubtract( enemy_dest, NPCS.NPC->r.currentOrigin, enemy_dir );
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir );// - (NPC->client->ps.saberLengthMax + NPC->r.maxs[0]*1.5 + 16);
	}
	else
	{//see where enemy is headed
		VectorCopy( NPCS.NPC->enemy->client->ps.velocity, enemy_movedir );
		*enemy_movespeed = VectorNormalize( enemy_movedir );
		//figure out where he'll be, say, 3 frames from now
		VectorMA( NPCS.NPC->enemy->r.currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest );
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract( enemy_dest, NPCS.NPC->r.currentOrigin, enemy_dir );//NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir ) - (NPCS.NPC->client->saber[0].blade[0].lengthMax + NPCS.NPC->r.maxs[0]*1.5 + 16); //just use the blade 0 len I guess
		//FIXME: keep a group of enemies around me and use that info to make decisions...
		//		For instance, if there are multiple enemies, evade more, push them away
		//		and use medium attacks.  If enemies are using blasters, switch to fast.
		//		If one jedi enemy, use strong attacks.  Use grip when fighting one or
		//		two enemies, use lightning spread when fighting multiple enemies, etc.
		//		Also, when kill one, check rest of group instead of walking up to victim.
	}
}

extern float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire );
static void Jedi_FaceEnemy( qboolean doPitch )
{
	vec3_t	enemy_eyes, eyes, angles;

	if ( NPCS.NPC == NULL )
		return;

	if ( NPCS.NPC->enemy == NULL )
		return;

	if ( NPCS.NPC->client->ps.fd.forcePowersActive & (1<<FP_GRIP) &&
		NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//don't update?
		NPCS.NPCInfo->desiredPitch = NPCS.NPC->client->ps.viewangles[PITCH];
		NPCS.NPCInfo->desiredYaw = NPCS.NPC->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot( NPCS.NPC, SPOT_HEAD, eyes );

	CalcEntitySpot( NPCS.NPC->enemy, SPOT_HEAD, enemy_eyes );

	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
		&& TIMER_Done( NPCS.NPC, "flameTime" )
		&& NPCS.NPC->s.weapon != WP_NONE
		&& NPCS.NPC->s.weapon != WP_DISRUPTOR
		&& (NPCS.NPC->s.weapon != WP_ROCKET_LAUNCHER||!(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE))
		&& NPCS.NPC->s.weapon != WP_THERMAL
		&& NPCS.NPC->s.weapon != WP_TRIP_MINE
		&& NPCS.NPC->s.weapon != WP_DET_PACK
		&& NPCS.NPC->s.weapon != WP_STUN_BATON
		/*&& NPC->s.weapon != WP_MELEE*/ )
	{//boba leads his enemy
		if ( NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth*0.5f )
		{//lead
			float missileSpeed = WP_SpeedOfMissileForWeapon( NPCS.NPC->s.weapon, ((qboolean)(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE)) );
			if ( missileSpeed )
			{
				float eDist = Distance( eyes, enemy_eyes );
				eDist /= missileSpeed;//How many seconds it will take to get to the enemy
				VectorMA( enemy_eyes, eDist*flrand(0.95f,1.25f), NPCS.NPC->enemy->client->ps.velocity, enemy_eyes );
			}
		}
	}

	//Find the desired angles
	if ( !NPCS.NPC->client->ps.saberInFlight
		&& (NPCS.NPC->client->ps.legsAnim == BOTH_A2_STABBACK1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK_BACK)
		)
	{//point *away*
		GetAnglesForDirection( enemy_eyes, eyes, angles );
	}
	else
	{//point towards him
		GetAnglesForDirection( eyes, enemy_eyes, angles );
	}

	NPCS.NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );
	/*
	if ( NPC->client->ps.saberBlocked == BLOCKED_UPPER_LEFT )
	{//temp hack- to make up for poor coverage on left side
		NPCInfo->desiredYaw += 30;
	}
	*/

	if ( doPitch )
	{
		NPCS.NPCInfo->desiredPitch = AngleNormalize360( angles[PITCH] );
		if ( NPCS.NPC->client->ps.saberInFlight )
		{//tilt down a little
			NPCS.NPCInfo->desiredPitch += 10;
		}
	}
	//FIXME: else desiredPitch = 0?  Or keep previous?
}

static void Jedi_DebounceDirectionChanges( void )
{
	//FIXME: check these before making fwd/back & right/left decisions?
	//Time-debounce changes in forward/back dir
	if ( NPCS.ucmd.forwardmove > 0 )
	{
		if ( !TIMER_Done( NPCS.NPC, "moveback" ) || !TIMER_Done( NPCS.NPC, "movenone" ) )
		{
			NPCS.ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( NPCS.ucmd.rightmove > 0 )
			{
				NPCS.ucmd.rightmove = 127;
			}
			else if ( NPCS.ucmd.rightmove < 0 )
			{
				NPCS.ucmd.rightmove = -127;
			}
			VectorClear( NPCS.NPC->client->ps.moveDir );
			TIMER_Set( NPCS.NPC, "moveback", -level.time );
			if ( TIMER_Done( NPCS.NPC, "movenone" ) )
			{
				TIMER_Set( NPCS.NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPCS.NPC, "moveforward" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( NPCS.NPC, "moveforward", Q_irand( 500, 2000 ) );
		}
	}
	else if ( NPCS.ucmd.forwardmove < 0 )
	{
		if ( !TIMER_Done( NPCS.NPC, "moveforward" ) || !TIMER_Done( NPCS.NPC, "movenone" ) )
		{
			NPCS.ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( NPCS.ucmd.rightmove > 0 )
			{
				NPCS.ucmd.rightmove = 127;
			}
			else if ( NPCS.ucmd.rightmove < 0 )
			{
				NPCS.ucmd.rightmove = -127;
			}
			VectorClear( NPCS.NPC->client->ps.moveDir );
			TIMER_Set( NPCS.NPC, "moveforward", -level.time );
			if ( TIMER_Done( NPCS.NPC, "movenone" ) )
			{
				TIMER_Set( NPCS.NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPCS.NPC, "moveback" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( NPCS.NPC, "moveback", Q_irand( 250, 1000 ) );
		}
	}
	else if ( !TIMER_Done( NPCS.NPC, "moveforward" ) )
	{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		NPCS.ucmd.forwardmove = 127;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPCS.NPC, "moveback" ) )
	{//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.forwardmove = -127;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}
	//Time-debounce changes in right/left dir
	if ( NPCS.ucmd.rightmove > 0 )
	{
		if ( !TIMER_Done( NPCS.NPC, "moveleft" ) || !TIMER_Done( NPCS.NPC, "movecenter" ) )
		{
			NPCS.ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( NPCS.ucmd.forwardmove > 0 )
			{
				NPCS.ucmd.forwardmove = 127;
			}
			else if ( NPCS.ucmd.forwardmove < 0 )
			{
				NPCS.ucmd.forwardmove = -127;
			}
			VectorClear( NPCS.NPC->client->ps.moveDir );
			TIMER_Set( NPCS.NPC, "moveleft", -level.time );
			if ( TIMER_Done( NPCS.NPC, "movecenter" ) )
			{
				TIMER_Set( NPCS.NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPCS.NPC, "moveright" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( NPCS.NPC, "moveright", Q_irand( 250, 1500 ) );
		}
	}
	else if ( NPCS.ucmd.rightmove < 0 )
	{
		if ( !TIMER_Done( NPCS.NPC, "moveright" ) || !TIMER_Done( NPCS.NPC, "movecenter" ) )
		{
			NPCS.ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( NPCS.ucmd.forwardmove > 0 )
			{
				NPCS.ucmd.forwardmove = 127;
			}
			else if ( NPCS.ucmd.forwardmove < 0 )
			{
				NPCS.ucmd.forwardmove = -127;
			}
			VectorClear( NPCS.NPC->client->ps.moveDir );
			TIMER_Set( NPCS.NPC, "moveright", -level.time );
			if ( TIMER_Done( NPCS.NPC, "movecenter" ) )
			{
				TIMER_Set( NPCS.NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPCS.NPC, "moveleft" ) )
		{//FIXME: should be if it's zero?
			TIMER_Set( NPCS.NPC, "moveleft", Q_irand( 250, 1500 ) );
		}
	}
	else if ( !TIMER_Done( NPCS.NPC, "moveright" ) )
	{//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.rightmove = 127;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPCS.NPC, "moveleft" ) )
	{//NOTE: edge checking should stop me if this is bad...
		NPCS.ucmd.rightmove = -127;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}
}

static void Jedi_TimersApply( void )
{
	if ( !NPCS.ucmd.rightmove )
	{//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if ( !TIMER_Done( NPCS.NPC, "strafeLeft" ) )
		{
			if ( NPCS.NPCInfo->desiredYaw > NPCS.NPC->client->ps.viewangles[YAW] + 60 )
			{//we want to turn left, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				NPCS.ucmd.rightmove = -127;
				VectorClear( NPCS.NPC->client->ps.moveDir );
			}
		}
		else if ( !TIMER_Done( NPCS.NPC, "strafeRight" ) )
		{
			if ( NPCS.NPCInfo->desiredYaw < NPCS.NPC->client->ps.viewangles[YAW] - 60 )
			{//we want to turn right, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				NPCS.ucmd.rightmove = 127;
				VectorClear( NPCS.NPC->client->ps.moveDir );
			}
		}
	}

	Jedi_DebounceDirectionChanges();

	//use careful anim/slower movement if not already moving
	if ( !NPCS.ucmd.forwardmove && !TIMER_Done( NPCS.NPC, "walking" ) )
	{
		NPCS.ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( NPCS.NPC, "taunting" ) )
	{
		NPCS.ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( NPCS.NPC, "gripping" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		NPCS.ucmd.buttons |= BUTTON_FORCEGRIP;
	}

	if ( !TIMER_Done( NPCS.NPC, "draining" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		NPCS.ucmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if ( !TIMER_Done( NPCS.NPC, "holdLightning" ) )
	{//hold down the lightning key
		NPCS.ucmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate( int enemy_dist )
{
	if( Jedi_CultistDestroyer( NPCS.NPC ) )
	{
		Jedi_Aggression( NPCS.NPC, 5 );
		return;
	}
	if ( TIMER_Done( NPCS.NPC, "roamTime" ) )
	{
		TIMER_Set( NPCS.NPC, "roamTime", Q_irand( 2000, 5000 ) );
		//okay, now mess with agression
		if ( NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE) )
		{//raging
			Jedi_Aggression( NPCS.NPC, Q_irand( 0, 3 ) );
		}
		else if ( NPCS.NPC->client->ps.fd.forceRageRecoveryTime > level.time )
		{//recovering
			Jedi_Aggression( NPCS.NPC, Q_irand( -2, 0 ) );
		}
		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client )
		{
			switch( NPCS.NPC->enemy->client->ps.weapon )
			{
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if ( BG_SabersOff( &NPCS.NPC->enemy->client->ps ) )
				{//fool!  Standing around unarmed, charge!
					//Com_Printf( "(%d) raise agg - enemy saber off\n", level.time );
					Jedi_Aggression( NPCS.NPC, 2 );
				}
				else
				{
					//Com_Printf( "(%d) raise agg - enemy saber\n", level.time );
					Jedi_Aggression( NPCS.NPC, 1 );
				}
				break;
			case WP_BLASTER:
			case WP_BRYAR_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
				//if he has a blaster, move in when:
				//They're not shooting at me
				if ( NPCS.NPC->enemy->attackDebounceTime < level.time )
				{//does this apply to players?
					//Com_Printf( "(%d) raise agg - enemy not shooting ranged weap\n", level.time );
					Jedi_Aggression( NPCS.NPC, 1 );
				}
				//He's closer than a dist that gives us time to deflect
				if ( enemy_dist < 256 )
				{
					//Com_Printf( "(%d) raise agg - enemy ranged weap- too close\n", level.time );
					Jedi_Aggression( NPCS.NPC, 1 );
				}
				break;
			default:
				break;
			}
		}
	}

	if ( TIMER_Done( NPCS.NPC, "noStrafe" ) && TIMER_Done( NPCS.NPC, "strafeLeft" ) && TIMER_Done( NPCS.NPC, "strafeRight" ) )
	{
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if ( !Q_irand( 0, 4 ) )
		{//start a strafe
			if ( Jedi_Strafe( 1000, 3000, 0, 4000, qtrue ) )
			{
				if ( d_JediAI.integer )
				{
					Com_Printf( "off strafe\n" );
				}
			}
		}
		else
		{//postpone any strafing for a while
			TIMER_Set( NPCS.NPC, "noStrafe", Q_irand( 1000, 3000 ) );
		}
	}

	if ( NPCS.NPC->client->ps.saberEventFlags )
	{//some kind of saber combat event is still pending
		int newFlags = NPCS.NPC->client->ps.saberEventFlags;
		if ( NPCS.NPC->client->ps.saberEventFlags&SEF_PARRIED )
		{//parried
			TIMER_Set( NPCS.NPC, "parryTime", -1 );
			/*
			if ( NPCInfo->rank >= RANK_LT_JG )
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 100;
			}
			else
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			*/
			if ( NPCS.NPC->enemy && PM_SaberInKnockaway( NPCS.NPC->enemy->client->ps.saberMove ) )
			{//advance!
				Jedi_Aggression( NPCS.NPC, 1 );//get closer
				Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel-1) );//use a faster attack
			}
			else
			{
				if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we parried\n", level.time );
					Jedi_Aggression( NPCS.NPC, -1 );
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel-1) );
				}
			}
			if ( d_JediAI.integer )
			{
				Com_Printf( "(%d) PARRY: agg %d, no parry until %d\n", level.time, NPCS.NPCInfo->stats.aggression, level.time + 100 );
			}
			newFlags &= ~SEF_PARRIED;
		}
		if ( !NPCS.NPC->client->ps.weaponTime && (NPCS.NPC->client->ps.saberEventFlags&SEF_HITENEMY) )//hit enemy
		{//we hit our enemy last time we swung, drop our aggression
			if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
			{
				//Com_Printf( "(%d) drop agg - we hit enemy\n", level.time );
				Jedi_Aggression( NPCS.NPC, -1 );
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) HIT: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression );
				}
				if ( !Q_irand( 0, 3 )
					&& NPCS.NPCInfo->blockedSpeechDebounceTime < level.time
					&& jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time
					&& NPCS.NPC->painDebounceTime < level.time - 1000 )
				{
					G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 3000 );
					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				}
			}
			if ( !Q_irand( 0, 2 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel+1) );
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if ( (NPCS.NPC->client->ps.saberEventFlags&SEF_BLOCKED) )
		{//was blocked whilst attacking
			if ( PM_SaberInBrokenParry( NPCS.NPC->client->ps.saberMove )
				|| NPCS.NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
			{
				//Com_Printf( "(%d) drop agg - we were knock-blocked\n", level.time );
				if ( NPCS.NPC->client->ps.saberInFlight )
				{//lost our saber, too!!!
					Jedi_Aggression( NPCS.NPC, -5 );//really really really should back off!!!
				}
				else
				{
					Jedi_Aggression( NPCS.NPC, -2 );//really should back off!
				}
				Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel+1) );//use a stronger attack
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) KNOCK-BLOCKED: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression );
				}
			}
			else
			{
				if ( !Q_irand( 0, 2 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we were blocked\n", level.time );
					Jedi_Aggression( NPCS.NPC, -1 );
					if ( d_JediAI.integer )
					{
						Com_Printf( "(%d) BLOCKED: agg %d\n", level.time, NPCS.NPCInfo->stats.aggression );
					}
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel+1) );
				}
			}
			newFlags &= ~SEF_BLOCKED;
			//FIXME: based on the type of parry the enemy is doing and my skill,
			//		choose an attack that is likely to get around the parry?
			//		right now that's generic in the saber animation code, auto-picks
			//		a next anim for me, but really should be AI-controlled.
		}
		if ( NPCS.NPC->client->ps.saberEventFlags&SEF_DEFLECTED )
		{//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel-1) );
			}
		}
		if ( NPCS.NPC->client->ps.saberEventFlags&SEF_HITWALL )
		{//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if ( NPCS.NPC->client->ps.saberEventFlags&SEF_HITOBJECT )
		{//hit some other damagable object
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPCS.NPC, (NPCS.NPC->client->ps.fd.saberAnimLevel-1) );
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		NPCS.NPC->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle( int enemy_dist )
{
	if ( !TIMER_Done( NPCS.NPC, "parryTime" ) )
	{
		return;
	}
	if ( NPCS.NPC->client->ps.saberInFlight )
	{//don't do this idle stuff if throwing saber
		return;
	}
	if ( NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)
		|| NPCS.NPC->client->ps.fd.forceRageRecoveryTime > level.time )
	{//never taunt while raging or recovering from rage
		return;
	}
	//FIXME: make these distance numbers defines?
	if ( enemy_dist >= 64 )
	{//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		int chance = 20;
		if ( NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER )
		{
			chance = 10;
		}
		//FIXME: possibly throw local objects at enemy?
		if ( Q_irand( 2, chance ) < NPCS.NPCInfo->stats.aggression )
		{
			if ( TIMER_Done( NPCS.NPC, "chatter" ) && NPCS.NPC->client->ps.forceHandExtend == HANDEXTEND_NONE )
			{//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if ( enemy_dist > 200
					&& NPCS.NPC->client->NPC_class != CLASS_BOBAFETT
					&& !NPCS.NPC->client->ps.saberHolstered
					&& !Q_irand( 0, 5 ) )
				{//taunt even more, turn off the saber
					//FIXME: don't do this if health low?
					WP_DeactivateSaber( NPCS.NPC, qfalse );
					//Don't attack for a bit
					NPCS.NPCInfo->stats.aggression = 3;
					//FIXME: maybe start strafing?
					//debounce this
					if ( NPCS.NPC->client->playerTeam != NPCTEAM_PLAYER && !Q_irand( 0, 1 ))
					{
						//NPC->client->ps.taunting = level.time + 100;
						NPCS.NPC->client->ps.forceHandExtend = HANDEXTEND_JEDITAUNT;
						NPCS.NPC->client->ps.forceHandExtendTime = level.time + 5000;

						TIMER_Set( NPCS.NPC, "chatter", Q_irand( 5000, 10000 ) );
						TIMER_Set( NPCS.NPC, "taunting", 5500 );
					}
					else
					{
						Jedi_BattleTaunt();
						TIMER_Set( NPCS.NPC, "taunting", Q_irand( 5000, 10000 ) );
					}
				}
				else if ( Jedi_BattleTaunt() )
				{//FIXME: pick some anims
				}
			}
		}
	}
}

static qboolean Jedi_AttackDecide( int enemy_dist )
{
	// Begin fixed cultist_destroyer AI
	if ( Jedi_CultistDestroyer( NPCS.NPC ) )
	{ // destroyer
		if ( enemy_dist <= 32 )
		{//go boom!
			//float?
			//VectorClear( NPC->client->ps.velocity );
			//NPC->client->ps.gravity = 0;
			//NPC->svFlags |= SVF_CUSTOM_GRAVITY;
			//NPC->client->moveType = MT_FLYSWIM;
			//NPC->flags |= FL_NO_KNOCKBACK;
			NPCS.NPC->flags |= FL_GODMODE;
			NPCS.NPC->takedamage = qfalse;

			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			NPCS.NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
			NPCS.NPC->painDebounceTime = NPCS.NPC->useDebounceTime = level.time + NPCS.NPC->client->ps.torsoTimer;
			return qtrue;
		}
		return qfalse;
	}

	if ( NPCS.NPC->enemy->client
		&& NPCS.NPC->enemy->s.weapon == WP_SABER
		&& NPCS.NPC->enemy->client->ps.saberLockTime > level.time
		&& NPCS.NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		return qfalse;
	}

	if ( NPCS.NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage with an attack!
		int	chance = 0;
		if ( NPCS.NPC->client->NPC_class == CLASS_DESANN || NPCS.NPC->client->NPC_class == CLASS_LUKE || !Q_stricmp("Yoda",NPCS.NPC->NPC_type) )
		{//desann and luke
			chance = 20;
		}
		else if ( NPCS.NPC->client->NPC_class == CLASS_TAVION )
		{//tavion
			chance = 10;
		}
		else if ( NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPCInfo->rank == RANK_LT_JG )
		{//fencer
			chance = 5;
		}
		else
		{
			chance = NPCS.NPCInfo->rank;
		}
		if ( Q_irand( 0, 30 ) < chance )
		{//based on skill with some randomness
			NPCS.NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;//clear this now that we are using the opportunity
			TIMER_Set( NPCS.NPC, "noRetreat", Q_irand( 500, 2000 ) );
			//FIXME: check enemy_dist?
			NPCS.NPC->client->ps.weaponTime = NPCS.NPCInfo->shotTime = NPCS.NPC->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	if ( NPCS.NPC->client->NPC_class == CLASS_TAVION ||
		( NPCS.NPC->client->NPC_class == CLASS_REBORN && NPCS.NPCInfo->rank == RANK_LT_JG ) ||
		( NPCS.NPC->client->NPC_class == CLASS_JEDI && NPCS.NPCInfo->rank == RANK_COMMANDER ) )
	{//tavion, fencers, jedi trainer are all good at following up a parry with an attack
		if ( ( PM_SaberInParry( NPCS.NPC->client->ps.saberMove ) || PM_SaberInKnockaway( NPCS.NPC->client->ps.saberMove ) )
			&& NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//try to attack straight from a parry
			NPCS.NPC->client->ps.weaponTime = NPCS.NPCInfo->shotTime = NPCS.NPC->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
			Jedi_AdjustSaberAnimLevel( NPCS.NPC, FORCE_LEVEL_1 );//try to follow-up with a quick attack
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	//try to hit them if we can
	if ( enemy_dist >= 64 )
	{
		return qfalse;
	}

	if ( !TIMER_Done( NPCS.NPC, "parryTime" ) )
	{
		return qfalse;
	}

	if ( (NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) )
	{//not allowed to attack
		return qfalse;
	}

	if ( !(NPCS.ucmd.buttons&BUTTON_ATTACK) && !(NPCS.ucmd.buttons&BUTTON_ALT_ATTACK) )
	{//not already attacking
		//Try to attack
		WeaponThink( qtrue );
	}

	//FIXME:  Maybe try to push enemy off a ledge?

	//close enough to step forward

	//FIXME: an attack debounce timer other than the phaser debounce time?
	//		or base it on aggression?

	if ( NPCS.ucmd.buttons&BUTTON_ATTACK )
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
		if ( !NPCS.ucmd.rightmove )
		{//not already strafing
			if ( !Q_irand( 0, 3 ) )
			{//25% chance of doing this
				vec3_t  right, dir2enemy;

				AngleVectors( NPCS.NPC->r.currentAngles, NULL, right, NULL );
				VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentAngles, dir2enemy );
				if ( DotProduct( right, dir2enemy ) > 0 )
				{//he's to my right, strafe left
					NPCS.ucmd.rightmove = -127;
					VectorClear( NPCS.NPC->client->ps.moveDir );
				}
				else
				{//he's to my left, strafe right
					NPCS.ucmd.rightmove = 127;
					VectorClear( NPCS.NPC->client->ps.moveDir );
				}
			}
		}
		return qtrue;
	}

	return qfalse;
}

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f

static qboolean Jedi_Jump( vec3_t dest, int goalEntNum )
{//FIXME: if land on enemy, knock him down & jump off again
	/*
	if ( dest[2] - NPC->r.currentOrigin[2] < 64 && DistanceHorizontal( NPC->r.currentOrigin, dest ) > 256 )
	{//a pretty horizontal jump, easy to fake:
		vec3_t enemy_diff;

		VectorSubtract( dest, NPC->r.currentOrigin, enemy_diff );
		float enemy_z_diff = enemy_diff[2];
		enemy_diff[2] = 0;
		float enemy_xy_diff = VectorNormalize( enemy_diff );

		VectorScale( enemy_diff, enemy_xy_diff*0.8, NPC->client->ps.velocity );
		if ( enemy_z_diff < 64 )
		{
			NPC->client->ps.velocity[2] = enemy_xy_diff;
		}
		else
		{
			NPC->client->ps.velocity[2] = enemy_z_diff*2+enemy_xy_diff/2;
		}
	}
	else
	*/
	if ( 1 )
	{
		float	targetDist, shotSpeed = 300, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed,
		vec3_t	targetDir, shotVel, failCase;
		trace_t	trace;
		trajectory_t	tr;
		qboolean	blocked;
		int		elapsedTime, timeStep = 500, hitCount = 0, maxHits = 7;
		vec3_t	lastPos, testPos, bottom;

		while ( hitCount < maxHits )
		{
			VectorSubtract( dest, NPCS.NPC->r.currentOrigin, targetDir );
			targetDist = VectorNormalize( targetDir );

			VectorScale( targetDir, shotSpeed, shotVel );
			travelTime = targetDist/shotSpeed;
			shotVel[2] += travelTime * 0.5 * NPCS.NPC->client->ps.gravity;

			if ( !hitCount )
			{//save the first one as the worst case scenario
				VectorCopy( shotVel, failCase );
			}

			if ( 1 )//tracePath )
			{//do a rough trace of the path
				blocked = qfalse;

				VectorCopy( NPCS.NPC->r.currentOrigin, tr.trBase );
				VectorCopy( shotVel, tr.trDelta );
				tr.trType = TR_GRAVITY;
				tr.trTime = level.time;
				travelTime *= 1000.0f;
				VectorCopy( NPCS.NPC->r.currentOrigin, lastPos );

				//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
				for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
				{
					if ( (float)elapsedTime > travelTime )
					{//cap it
						elapsedTime = floor( travelTime );
					}
					BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
					if ( testPos[2] < lastPos[2] )
					{//going down, ignore botclip
						trap->Trace( &trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
					}
					else
					{//going up, check for botclip
						trap->Trace( &trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
					}

					if ( trace.allsolid || trace.startsolid )
					{
						blocked = qtrue;
						break;
					}
					if ( trace.fraction < 1.0f )
					{//hit something
						if ( trace.entityNum == goalEntNum )
						{//hit the enemy, that's perfect!
							//Hmm, don't want to land on him, though...
							break;
						}
						else
						{
							if ( trace.contents & CONTENTS_BOTCLIP )
							{//hit a do-not-enter brush
								blocked = qtrue;
								break;
							}
							if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, dest ) < 4096 )//hit within 64 of desired location, should be okay
							{//close enough!
								break;
							}
							else
							{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
								impactDist = DistanceSquared( trace.endpos, dest );
								if ( impactDist < bestImpactDist )
								{
									bestImpactDist = impactDist;
									VectorCopy( shotVel, failCase );
								}
								blocked = qtrue;
								break;
							}
						}
					}
					if ( elapsedTime == floor( travelTime ) )
					{//reached end, all clear
						if ( trace.fraction >= 1.0f )
						{//hmm, make sure we'll land on the ground...
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy( trace.endpos, bottom );
							bottom[2] -= 128;
							trap->Trace( &trace, trace.endpos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
							if ( trace.fraction >= 1.0f )
							{//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					else
					{
						//all clear, try next slice
						VectorCopy( testPos, lastPos );
					}
				}
				if ( blocked )
				{//hit something, adjust speed (which will change arc)
					hitCount++;
					shotSpeed = 300 + ((hitCount-2) * 100);//from 100 to 900 (skipping 300)
					if ( hitCount >= 2 )
					{//skip 300 since that was the first value we tested
						shotSpeed += 100;
					}
				}
				else
				{//made it!
					break;
				}
			}
			else
			{//no need to check the path, go with first calc
				break;
			}
		}

		if ( hitCount >= maxHits )
		{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
			//NOTE: or try failcase?
			VectorCopy( failCase, NPCS.NPC->client->ps.velocity );
		}
		VectorCopy( shotVel, NPCS.NPC->client->ps.velocity );
	}
	else
	{//a more complicated jump
		vec3_t		dir, p1, p2, apex;
		float		time, height, forward, z, xy, dist, apexHeight;

		if ( NPCS.NPC->r.currentOrigin[2] > dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( NPCS.NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}
		else if ( NPCS.NPC->r.currentOrigin[2] < dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( dest, p1 );//NPCInfo->goalEntity->r.currentOrigin
			VectorCopy( NPCS.NPC->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( NPCS.NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );
		z = p1[2] - p2[2];

		apexHeight = APEX_HEIGHT/2;

		//Determine most desirable apex height
		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH
		/*
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5 )
		{
			apexHeight = APEX_HEIGHT*0.5;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
			apexHeight = APEX_HEIGHT*2;
		}
		*/

		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		xy -= z;
		xy *= 0.5;

		assert(xy > 0);

		VectorMA( p1, xy, dir, apex );
		apex[2] += apexHeight;

		VectorCopy(apex, NPCS.NPC->pos1);

		//Now we have the apex, aim for it
		height = apex[2] - NPCS.NPC->r.currentOrigin[2];
		time = sqrt( height / ( .5 * NPCS.NPC->client->ps.gravity ) );//was 0.5, but didn't work well for very long jumps
		if ( !time )
		{
			//Com_Printf( S_COLOR_RED"ERROR: no time in jump\n" );
			return qfalse;
		}

		VectorSubtract ( apex, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.velocity );
		NPCS.NPC->client->ps.velocity[2] = 0;
		dist = VectorNormalize( NPCS.NPC->client->ps.velocity );

		forward = dist / time * 1.25;//er... probably bad, but...
		VectorScale( NPCS.NPC->client->ps.velocity, forward, NPCS.NPC->client->ps.velocity );

		//FIXME:  Uh.... should we trace/EvaluateTrajectory this to make sure we have clearance and we land where we want?
		NPCS.NPC->client->ps.velocity[2] = time * NPCS.NPC->client->ps.gravity;

		//Com_Printf("Jump Velocity: %4.2f, %4.2f, %4.2f\n", NPC->client->ps.velocity[0], NPC->client->ps.velocity[1], NPC->client->ps.velocity[2] );
	}
	return qtrue;
}

static qboolean Jedi_TryJump( gentity_t *goal )
{//FIXME: never does a simple short, regular jump...
	//FIXME: I need to be on ground too!
	if ( (NPCS.NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return qfalse;
	}
	if ( TIMER_Done( NPCS.NPC, "jumpChaseDebounce" ) )
	{
		if ( (!goal->client || goal->client->ps.groundEntityNum != ENTITYNUM_NONE) )
		{
			if ( !PM_InKnockDown( &NPCS.NPC->client->ps ) && !BG_InRoll( &NPCS.NPC->client->ps, NPCS.NPC->client->ps.legsAnim ) )
			{//enemy is on terra firma
				vec3_t goal_diff;
				float goal_z_diff;
				float goal_xy_dist;
				VectorSubtract( goal->r.currentOrigin, NPCS.NPC->r.currentOrigin, goal_diff );
				goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				goal_xy_dist = VectorNormalize( goal_diff );
				if ( goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/ )//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if ( NPCS.NPC->health < 150 && ((NPCS.NPC->health < 30 && goal_z_diff < 0) || goal_z_diff < -128 ) )
					{//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if ( goal_z_diff < 32 && goal_xy_dist < 200 )
					{//what is their ideal jump height?
						NPCS.ucmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						/*
						//NO!  All Jedi can jump-navigate now...
						if ( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG )
						{//can't do acrobatics
							return qfalse;
						}
						*/
						if ( goal_z_diff > 0 || goal_xy_dist > 128 )
						{//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy( goal->r.currentOrigin, dest );
							if ( goal == NPCS.NPC->enemy )
							{
								int	sideTry = 0;
								while( sideTry < 10 )
								{//FIXME: make it so it doesn't try the same spot again?
									trace_t	trace;
									vec3_t	bottom;

									if ( Q_irand( 0, 1 ) )
									{
										dest[0] += NPCS.NPC->enemy->r.maxs[0]*1.25;
									}
									else
									{
										dest[0] += NPCS.NPC->enemy->r.mins[0]*1.25;
									}
									if ( Q_irand( 0, 1 ) )
									{
										dest[1] += NPCS.NPC->enemy->r.maxs[1]*1.25;
									}
									else
									{
										dest[1] += NPCS.NPC->enemy->r.mins[1]*1.25;
									}
									VectorCopy( dest, bottom );
									bottom[2] -= 128;
									trap->Trace( &trace, dest, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom, goal->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
									if ( trace.fraction < 1.0f )
									{//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if ( sideTry >= 10 )
								{//screw it, just jump right at him?
									VectorCopy( goal->r.currentOrigin, dest );
								}
							}
							if ( Jedi_Jump( dest, goal->s.number ) )
							{
								//Com_Printf( "(%d) pre-checked force jump\n", level.time );

								//FIXME: store the dir we;re going in in case something gets in the way of the jump?
								//? = vectoyaw( NPC->client->ps.velocity );
								/*
								if ( NPC->client->ps.velocity[2] < 320 )
								{
									NPC->client->ps.velocity[2] = 320;
								}
								else
								*/
								{//FIXME: make this a function call
									int jumpAnim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT
										||( NPCS.NPCInfo->rank != RANK_CREWMAN && NPCS.NPCInfo->rank <= RANK_LT_JG ) )
									{//can't do acrobatics
										jumpAnim = BOTH_FORCEJUMP1;
									}
									else
									{
										jumpAnim = BOTH_FLIP_F;
									}
									NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								}

								NPCS.NPC->client->ps.fd.forceJumpZStart = NPCS.NPC->r.currentOrigin[2];
								//NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPCS.NPC->client->ps.weaponTime = NPCS.NPC->client->ps.torsoTimer;
								NPCS.NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );
								if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
								{
									G_SoundOnEnt( NPCS.NPC, CHAN_ITEM, "sound/boba/jeton.wav" );
									NPCS.NPC->client->jetPackTime = level.time + Q_irand( 1000, 3000 );
								}
								else
								{
									G_SoundOnEnt( NPCS.NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}

								TIMER_Set( NPCS.NPC, "forceJumpChasing", Q_irand( 2000, 3000 ) );
								debounce = qtrue;
							}
						}
					}
					if ( debounce )
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPCS.NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
						NPCS.ucmd.forwardmove = 127;
						VectorClear( NPCS.NPC->client->ps.moveDir );
						TIMER_Set( NPCS.NPC, "duck", -level.time );
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

static qboolean Jedi_Jumping( gentity_t *goal )
{
	if ( !TIMER_Done( NPCS.NPC, "forceJumpChasing" ) && goal )
	{//force-jumping at the enemy
//		if ( !(NPC->client->ps.pm_flags & PMF_JUMPING )//forceJumpZStart )
//			&& !(NPC->client->ps.pm_flags&PMF_TRIGGER_PUSHED))
		if (NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE) //rwwFIXMEFIXME: Not sure if this is gonna work, use the PM flags ideally.
		{//landed
			TIMER_Set( NPCS.NPC, "forceJumpChasing", 0 );
		}
		else
		{
			NPC_FaceEntity( goal, qtrue );
			//FIXME: push me torward where I was heading
			//FIXME: if hit a ledge as soon as we jumped, we need to push toward our goal... must remember original jump dir and/or original jump dest
			/*
			vec3_t	viewangles_xy={0,0,0}, goal_dir, goal_xy_dir, forward, right;
			float	goal_dist;

			//gert horz dir to goal
			VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, goal_dir );
			VectorCopy( goal_dir, goal_xy_dir );
			goal_dist = VectorNormalize( goal_dir );
			goal_xy_dir[2] = 0;
			VectorNormalize( goal_xy_dir );

			//get horz facing
			viewangles_xy[1] = NPC->client->ps.viewangles[1];
			AngleVectors( viewangles_xy, forward, right, NULL );

			//get movement commands to push me toward enemy
			float fDot = DotProduct( forward, goal_dir ) * 127;
			float rDot = DotProduct( right, goal_dir ) * 127;

			ucmd.forwardmove = floor(fDot);
			ucmd.rightmove = floor(rDot);
			ucmd.upmove = 0;//don't duck
			//Cheat:
			if ( goal_dist < 128 && goal->r.currentOrigin[2] > NPC->r.currentOrigin[2] && NPC->client->ps.velocity[2] <= 0 )
			{
				NPC->client->ps.velocity[2] += 320;
			}
			*/
			return qtrue;
		}
	}
	return qfalse;
}

extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
static void Jedi_CheckEnemyMovement( float enemy_dist )
{
	if ( !NPCS.NPC->enemy || !NPCS.NPC->enemy->client )
	{
		return;
	}

	if ( NPCS.NPC->client->NPC_class != CLASS_TAVION
		&& NPCS.NPC->client->NPC_class != CLASS_DESANN
		&& NPCS.NPC->client->NPC_class != CLASS_LUKE
		&& Q_stricmp("Yoda",NPCS.NPC->NPC_type) )
	{
		if ( NPCS.NPC->enemy->enemy && NPCS.NPC->enemy->enemy == NPCS.NPC )
		{//enemy is mad at *me*
			if ( NPCS.NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1 ||
				NPCS.NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN )
			{//enemy is flipping over me
				if ( Q_irand( 0, NPCS.NPCInfo->rank ) < RANK_LT )
				{//be nice and stand still for him...
					NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
					VectorClear( NPCS.NPC->client->ps.moveDir );
					NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
					TIMER_Set( NPCS.NPC, "strafeLeft", -1 );
					TIMER_Set( NPCS.NPC, "strafeRight", -1 );
					TIMER_Set( NPCS.NPC, "noStrafe", Q_irand( 500, 1000 ) );
					TIMER_Set( NPCS.NPC, "movenone", Q_irand( 500, 1000 ) );
					TIMER_Set( NPCS.NPC, "movecenter", Q_irand( 500, 1000 ) );
				}
			}
			else if ( NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP
				|| NPCS.NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP )
			{//he's flipping off a wall
				if ( NPCS.NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
				{//still in air
					if ( enemy_dist < 256 )
					{//close
						if ( Q_irand( 0, NPCS.NPCInfo->rank ) < RANK_LT )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							/*
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 200, 500 ) );
							*/
							//stop current movement
							NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
							VectorClear( NPCS.NPC->client->ps.moveDir );
							NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPCS.NPC, "strafeLeft", -1 );
							TIMER_Set( NPCS.NPC, "strafeRight", -1 );
							TIMER_Set( NPCS.NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPCS.NPC, "noturn", Q_irand( 250, 500 )*(3-g_npcspskill.integer) );

							VectorCopy( NPCS.NPC->enemy->client->ps.velocity, enemyFwd );
							VectorNormalize( enemyFwd );
							VectorMA( NPCS.NPC->enemy->r.currentOrigin, -64, enemyFwd, dest );
							VectorSubtract( dest, NPCS.NPC->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 32 )
							{
								G_UcmdMoveForDir( NPCS.NPC, &NPCS.ucmd, dir );
							}
							else
							{
								TIMER_Set( NPCS.NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPCS.NPC, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
			else if ( NPCS.NPC->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 )
			{//he's stabbing backwards
				if ( enemy_dist < 256 && enemy_dist > 64 )
				{//close
					if ( !InFront( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->enemy->r.currentAngles, 0.0f ) )
					{//behind him
						if ( !Q_irand( 0, NPCS.NPCInfo->rank ) )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							//stop current movement
							NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = NPCS.ucmd.upmove = 0;
							VectorClear( NPCS.NPC->client->ps.moveDir );
							NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPCS.NPC, "strafeLeft", -1 );
							TIMER_Set( NPCS.NPC, "strafeRight", -1 );
							TIMER_Set( NPCS.NPC, "noStrafe", Q_irand( 500, 1000 ) );

							AngleVectors( NPCS.NPC->enemy->r.currentAngles, enemyFwd, NULL, NULL );
							VectorMA( NPCS.NPC->enemy->r.currentOrigin, -32, enemyFwd, dest );
							VectorSubtract( dest, NPCS.NPC->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 64 )
							{
								G_UcmdMoveForDir( NPCS.NPC, &NPCS.ucmd, dir );
							}
							else
							{
								TIMER_Set( NPCS.NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPCS.NPC, "movecenter", Q_irand( 500, 1000 ) );
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
	vec3_t	jumpVel;
	trace_t	trace;
	trajectory_t	tr;
	vec3_t	lastPos, testPos, bottom;
	int		elapsedTime;

	if ( (NPCS.NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
		NPCS.ucmd.upmove = 0;
		return;
	}
	//FIXME: should probably check this before AI decides that best move is to jump?  Otherwise, they may end up just standing there and looking dumb
	//FIXME: all this work and he still jumps off ledges... *sigh*... need CONTENTS_BOTCLIP do-not-enter brushes...?
	VectorClear(jumpVel);

	if ( NPCS.NPC->client->ps.fd.forceJumpCharge )
	{
		//Com_Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump( NPCS.NPC, jumpVel, &NPCS.ucmd );
	}
	else if ( NPCS.ucmd.upmove > 0 )
	{
		//Com_Printf( "(%d) regular jump\n", level.time );
		VectorCopy( NPCS.NPC->client->ps.velocity, jumpVel );
		jumpVel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( !jumpVel[0] && !jumpVel[1] )//FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{//we assume a jump straight up is safe
		//Com_Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}
	//Now predict where this is going
	//in steps, keep evaluating the trajectory until the new z pos is <= than current z pos, trace down from there

	VectorCopy( NPCS.NPC->r.currentOrigin, tr.trBase );
	VectorCopy( jumpVel, tr.trDelta );
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy( NPCS.NPC->r.currentOrigin, lastPos );

	VectorClear(trace.endpos); //shut the compiler up

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for ( elapsedTime = 500; elapsedTime <= 4000; elapsedTime += 500 )
	{
		BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if ( testPos[2] < lastPos[2] )
		{//going down, don't check for BOTCLIP
			trap->Trace( &trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );//FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{//going up, check for BOTCLIP
			trap->Trace( &trace, lastPos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, testPos, NPCS.NPC->s.number, NPCS.NPC->clipmask|CONTENTS_BOTCLIP, qfalse, 0, 0 );
		}
		if ( trace.allsolid || trace.startsolid )
		{//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
		}
		if ( trace.fraction < 1.0f )
		{//hit something
			if ( trace.contents & CONTENTS_BOTCLIP )
			{//hit a do-not-enter brush
				goto jump_unsafe;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy( testPos, lastPos );
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy( trace.endpos, bottom );
	if ( bottom[2] > NPCS.NPC->r.currentOrigin[2] )
	{//only care about dist down from current height or lower
		bottom[2] = NPCS.NPC->r.currentOrigin[2];
	}
	else if ( NPCS.NPC->r.currentOrigin[2] - bottom[2] > 400 )
	{//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
	}
	bottom[2] -= 128;
	trap->Trace( &trace, trace.endpos, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, bottom, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0f )
	{//hit ground!
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//landed on an ent
			gentity_t *groundEnt = &g_entities[trace.entityNum];
			if ( groundEnt->r.svFlags&SVF_GLASS_BRUSH )
			{//don't land on breakable glass!
				goto jump_unsafe;
			}
		}
		//Com_Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	//Com_Printf( "(%d) unsafe jump cleared\n", level.time );
	NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
	NPCS.ucmd.upmove = 0;
}

static void Jedi_Combat( void )
{
	vec3_t	enemy_dir, enemy_movedir, enemy_dest;
	float	enemy_dist, enemy_movespeed;
	qboolean	enemy_lost = qfalse;

	//See where enemy will be 300 ms from now
	Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );

	if ( Jedi_Jumping( NPCS.NPC->enemy ) )
	{//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide( enemy_dist );
		return;
	}

	if ( !(NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP)) || NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
	{//not gripping
		//If we can't get straight at him
		if ( !Jedi_ClearPathToSpot( enemy_dest, NPCS.NPC->enemy->s.number ) )
		{//hunt him down
			//Com_Printf( "No Clear Path\n" );
			if ( (NPC_ClearLOS4( NPCS.NPC->enemy )||NPCS.NPCInfo->enemyLastSeenTime>level.time-500) && NPC_FaceEnemy( qtrue ) )//( NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) &&
			{
				//try to jump to him?
				/*
				vec3_t end;
				VectorCopy( NPC->r.currentOrigin, end );
				end[2] += 36;
				trap->Trace( &trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
				if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0 )
				{
					vec3_t angles, forward;
					VectorCopy( NPC->client->ps.viewangles, angles );
					angles[0] = 0;
					AngleVectors( angles, forward, NULL, NULL );
					VectorMA( end, 64, forward, end );
					trap->Trace( &trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
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
				//Com_Printf( "Considering Jump\n" );
				if ( Jedi_TryJump( NPCS.NPC->enemy ) )
				{//FIXME: what about jumping to his enemyLastSeenLocation?
					return;
				}
			}

			//Check for evasion
			if ( TIMER_Done( NPCS.NPC, "parryTime" ) )
			{//finished parrying
				if ( NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
					NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
				{//wasn't blocked myself
					NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
				}
			}
			if ( Jedi_Hunt() && !(NPCS.NPCInfo->aiFlags&NPCAI_BLOCKED) )//FIXME: have to do this because they can ping-pong forever
			{//can macro-navigate to him
				if ( enemy_dist < 384 && !Q_irand( 0, 10 ) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time && !NPC_ClearLOS4( NPCS.NPC->enemy ) )
				{
					G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_JLOST1, EV_JLOST3 ), 3000 );
					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
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
				if ( NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED )
				{//try to jump to the blockedDest
					gentity_t *tempGoal = G_Spawn();//ugh, this is NOT good...?
					G_SetOrigin( tempGoal, NPCS.NPCInfo->blockedDest );
					trap->LinkEntity( (sharedEntity_t *)tempGoal );
					if ( Jedi_TryJump( tempGoal ) )
					{//going to jump to the dest
						G_FreeEntity( tempGoal );
						return;
					}
					G_FreeEntity( tempGoal );
				}

				enemy_lost = qtrue;
			}
		}
	}
	//else, we can see him or we can't track him at all

	//every few seconds, decide if we should we advance or retreat?
	Jedi_CombatTimersUpdate( enemy_dist );

	//We call this even if lost enemy to keep him moving and to update the taunting behavior
	//maintain a distance from enemy appropriate for our aggression level
	Jedi_CombatDistance( enemy_dist );

	if ( !enemy_lost )
	{
		//Update our seen enemy position
		if ( !NPCS.NPC->enemy->client || ( NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE ) )
		{
			VectorCopy( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation );
		}
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	if ( TIMER_Done( NPCS.NPC, "noturn" ) )
	{
		Jedi_FaceEnemy( qtrue );
	}
	NPC_UpdateAngles( qtrue, qtrue );

	//Check for evasion
	if ( TIMER_Done( NPCS.NPC, "parryTime" ) )
	{//finished parrying
		if ( NPCS.NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPCS.NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
	if ( NPCS.NPC->enemy->s.weapon == WP_SABER )
	{
		Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
	}
	else
	{//do we need to do any evasion for other kinds of enemies?
	}

	//apply strafing/walking timers, etc.
	Jedi_TimersApply();

	if ( !NPCS.NPC->client->ps.saberInFlight && (!(NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP))||NPCS.NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2) )
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
			TIMER_Set( NPCS.NPC, "taunting", -level.time );
		}
	}
	else
	{
	}
	if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		Boba_FireDecide();
	}

	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement( enemy_dist );
	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps();
	//Just make sure we don't strafe into walls or off cliffs
	if ( !NPC_MoveDirClear( NPCS.ucmd.forwardmove, NPCS.ucmd.rightmove, qtrue ) )
	{//uh-oh, we are going to fall or hit something
		navInfo_t	info;
		//Get the move info
		NAV_GetLastMove( &info );
		if ( !(info.flags & NIF_MACRO_NAV) )
		{//micro-navigation told us to step off a ledge, try using macronav for now
			NPC_MoveToGoal( qfalse );
		}
		//reset the timers.
		TIMER_Set( NPCS.NPC, "strafeLeft", 0 );
		TIMER_Set( NPCS.NPC, "strafeRight", 0 );
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

void NPC_Jedi_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	gentity_t *other = attacker;
	vec3_t point;

	VectorCopy(gPainPoint, point);

	//FIXME: base the actual aggression add/subtract on health?
	//FIXME: don't do this more than once per frame?
	//FIXME: when take pain, stop force gripping....?
	if ( other->s.weapon == WP_SABER )
	{//back off
		TIMER_Set( self, "parryTime", -1 );
		if ( self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) )
		{//less for Desann
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*50;
		}
		else if ( self->NPC->rank >= RANK_LT_JG )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*100;//300
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_npcspskill.integer)*200;//500
		}
		if ( !Q_irand( 0, 3 ) )
		{//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel( self, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ) );
		}
		if ( !Q_irand( 0, 1 ) )//damage > 20 || self->health < 40 ||
		{
			//Com_Printf( "(%d) drop agg - hit by saber\n", level.time );
			Jedi_Aggression( self, -1 );
		}
		if ( d_JediAI.integer )
		{
			Com_Printf( "(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression, level.time+500 );
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if ( d_JediAI.integer )
		{
			vec3_t	diff, fwdangles, right;
			float rightdot, zdiff;

			VectorSubtract( point, self->client->renderInfo.eyePoint, diff );
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors( fwdangles, NULL, right, NULL );
			rightdot = DotProduct(right, diff);
			zdiff = point[2] - self->client->renderInfo.eyePoint[2];

			Com_Printf( "(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, point[2]-self->r.absmin[2],zdiff,rightdot);
		}
	}
	else
	{//attack
		//Com_Printf( "(%d) raise agg - hit by ranged\n", level.time );
		Jedi_Aggression( self, 1 );
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop( self, FP_GRIP );

	//NPC_Pain( self, inflictor, other, point, damage, mod );
	NPC_Pain(self, attacker, damage);

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	}

	//drop me from the ceiling if I'm on it
	if ( Jedi_WaitingAmbush( self ) )
	{
		self->client->noclip = qfalse;
	}
	if ( self->client->ps.legsAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	if ( self->client->ps.torsoAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
}

qboolean Jedi_CheckDanger( void )
{
	int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR );

	if ( level.alertEvents[alertEvent].level >= AEL_DANGER )
	{//run away!
		if ( !level.alertEvents[alertEvent].owner
			|| !level.alertEvents[alertEvent].owner->client
			|| (level.alertEvents[alertEvent].owner!=NPCS.NPC&&level.alertEvents[alertEvent].owner->client->playerTeam!=NPCS.NPC->client->playerTeam) )
		{//no owner
			return qfalse;
		}
		G_SetEnemy( NPCS.NPC, level.alertEvents[alertEvent].owner );
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_CheckAmbushPlayer( void )
{
	int i = 0;
	gentity_t *player;
	float target_dist;
	float zDiff;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		player = &g_entities[i];

		if ( !player || !player->client )
		{
			continue;
		}

		if ( !NPC_ValidEnemy( player ) )
		{
			continue;
		}

//		if ( NPC->client->ps.powerups[PW_CLOAKED] || g_crosshairEntNum != NPC->s.number )
		if (NPCS.NPC->client->ps.powerups[PW_CLOAKED] || !NPC_SomeoneLookingAtMe(NPCS.NPC)) //rwwFIXMEFIXME: Need to pay attention to who is under crosshair for each player or something.
		{//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
			if ( !trap->InPVS( player->r.currentOrigin, NPCS.NPC->r.currentOrigin ) )
			{//must be in same room
				continue;
			}
			else
			{
				if ( !NPCS.NPC->client->ps.powerups[PW_CLOAKED] )
				{
					NPC_SetLookTarget( NPCS.NPC, 0, 0 );
				}
			}
			zDiff = NPCS.NPC->r.currentOrigin[2]-player->r.currentOrigin[2];
			if ( zDiff <= 0 || zDiff > 512 )
			{//never ambush if they're above me or way way below me
				continue;
			}

			//If the target is this close, then wake up regardless
			if ( (target_dist = DistanceHorizontalSquared( player->r.currentOrigin, NPCS.NPC->r.currentOrigin )) > 4096 )
			{//closer than 64 - always ambush
				if ( target_dist > 147456 )
				{//> 384, not close enough to ambush
					continue;
				}
				//Check FOV first
				if ( NPCS.NPC->client->ps.powerups[PW_CLOAKED] )
				{
					if ( InFOV( player, NPCS.NPC, 30, 90 ) == qfalse )
					{
						continue;
					}
				}
				else
				{
					if ( InFOV( player, NPCS.NPC, 45, 90 ) == qfalse )
					{
						continue;
					}
				}
			}

			if ( !NPC_ClearLOS4( player ) )
			{
				continue;
			}
		}

		//Got him, return true;
		G_SetEnemy( NPCS.NPC, player );
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPCS.NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}

	//Didn't get anyone.
	return qfalse;
}

void Jedi_Ambush( gentity_t *self )
{
	self->client->noclip = qfalse;
//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.weaponTime = self->client->ps.torsoTimer; //NPC->client->ps.torsoTimer; //what the?
	if ( self->client->NPC_class != CLASS_BOBAFETT )
	{
		WP_ActivateSaber(self);
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
	NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;

	if ( Jedi_WaitingAmbush( NPCS.NPC ) )
	{//hiding on the ceiling
		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( NPCS.NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{//look for enemies
			if ( Jedi_CheckAmbushPlayer() || Jedi_CheckDanger() )
			{//found him!
				Jedi_Ambush( NPCS.NPC );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}
	else if ( NPCS.NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )//NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{//look for enemies
		gentity_t *best_enemy = NULL;
		float	best_enemy_dist = Q3_INFINITE;
		int i;
		for ( i = 0; i < ENTITYNUM_WORLD; i++ )
		{
			gentity_t *enemy = &g_entities[i];
			float	enemy_dist;
			if ( enemy && enemy->client && NPC_ValidEnemy( enemy ) && enemy->client->playerTeam == NPCS.NPC->client->enemyTeam )
			{
				if ( trap->InPVS( NPCS.NPC->r.currentOrigin, enemy->r.currentOrigin ) )
				{//we could potentially see him
					enemy_dist = DistanceSquared( NPCS.NPC->r.currentOrigin, enemy->r.currentOrigin );
					if ( enemy->s.eType == ET_PLAYER || enemy_dist < best_enemy_dist )
					{
						//if the enemy is close enough, or threw his saber, take him as the enemy
						//FIXME: what if he throws a thermal detonator?
						if ( enemy_dist < (220*220) || ( NPCS.NPCInfo->investigateCount>= 3 && !NPCS.NPC->client->ps.saberHolstered ) )
						{
							G_SetEnemy( NPCS.NPC, enemy );
							//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
							NPCS.NPCInfo->stats.aggression = 3;
							break;
						}
						else if ( enemy->client->ps.saberInFlight && !enemy->client->ps.saberHolstered )
						{//threw his saber, see if it's heading toward me and close enough to consider a threat
							float	saberDist;
							vec3_t	saberDir2Me;
							vec3_t	saberMoveDir;
							gentity_t *saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract( NPCS.NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
							saberDist = VectorNormalize( saberDir2Me );
							VectorCopy( saber->s.pos.trDelta, saberMoveDir );
							VectorNormalize( saberMoveDir );
							if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
							{//it's heading towards me
								if ( saberDist < 200 )
								{//incoming!
									G_SetEnemy( NPCS.NPC, enemy );
									//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
									NPCS.NPCInfo->stats.aggression = 3;
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
		if ( !NPCS.NPC->enemy )
		{//still not mad
			if ( !best_enemy )
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(-1);
				//FIXME: what about alerts?  But not if ignore alerts
			}
			else
			{//have one to consider
				if ( NPC_ClearLOS4( best_enemy ) )
				{//we have a clear (of architecture) LOS to him
					if ( best_enemy->s.number )
					{//just attack
						G_SetEnemy( NPCS.NPC, best_enemy );
						NPCS.NPCInfo->stats.aggression = 3;
					}
					else if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
					{//the player, toy with him
						//get progressively more interested over time
						if ( TIMER_Done( NPCS.NPC, "watchTime" ) )
						{//we want to pick him up in stages
							if ( TIMER_Get( NPCS.NPC, "watchTime" ) == -1 )
							{//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set( NPCS.NPC, "watchTime", Q_irand( 3000, 5000 ) );
								goto finish;
							}
							else
							{//okay, we've ignored him, now start to notice him
								if ( !NPCS.NPCInfo->investigateCount )
								{
									G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 3000 );
								}
								NPCS.NPCInfo->investigateCount++;
								TIMER_Set( NPCS.NPC, "watchTime", Q_irand( 4000, 10000 ) );
							}
						}
						//while we're waiting, do what we need to do
						if ( best_enemy_dist < (440*440) || NPCS.NPCInfo->investigateCount >= 2 )
						{//stage three: keep facing him
							NPC_FaceEntity( best_enemy, qtrue );
							if ( best_enemy_dist < (330*330) )
							{//stage four: turn on the saber
								if ( !NPCS.NPC->client->ps.saberInFlight )
								{
									WP_ActivateSaber(NPCS.NPC);
								}
							}
						}
						else if ( best_enemy_dist < (550*550) || NPCS.NPCInfo->investigateCount == 1 )
						{//stage two: stop and face him every now and then
							if ( TIMER_Done( NPCS.NPC, "watchTime" ) )
							{
								NPC_FaceEntity( best_enemy, qtrue );
							}
						}
						else
						{//stage one: look at him.
							NPC_SetLookTarget( NPCS.NPC, best_enemy->s.number, 0 );
						}
					}
				}
				else if ( TIMER_Done( NPCS.NPC, "watchTime" ) )
				{//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget( NPCS.NPC );
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPCS.NPC->enemy )
	{//just picked one up
		NPCS.NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
	}
}

qboolean Jedi_CanPullBackSaber( gentity_t *self )
{
	if ( self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done( self, "parryTime" ) )
	{
		return qfalse;
	}

	if ( self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_TAVION
		|| self->client->NPC_class == CLASS_LUKE
		|| self->client->NPC_class == CLASS_DESANN
		|| !Q_stricmp( "Yoda", self->NPC_type ) )
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
void NPC_BSJedi_FollowLeader( void )
{
	NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
	if ( !NPCS.NPC->enemy )
	{
		//Com_Printf( "(%d) drop agg - no enemy (follow)\n", level.time );
		Jedi_AggressionErosion(-1);
	}

	//did we drop our saber?  If so, go after it!
	if ( NPCS.NPC->client->ps.saberInFlight )
	{//saber is not in hand
		if ( NPCS.NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPCS.NPC->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( g_entities[NPCS.NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			{//fell to the ground, try to pick it up...
				if ( Jedi_CanPullBackSaber( NPCS.NPC ) )
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCS.NPCInfo->goalEntity = &g_entities[NPCS.NPC->client->ps.saberEntityNum];
					NPCS.ucmd.buttons |= BUTTON_ATTACK;
					if ( NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0 )
					{//get our saber back NOW!
						if ( !NPC_MoveToGoal( qtrue ) )//Jedi_Move( NPCInfo->goalEntity, qfalse );
						{//can't nav to it, try jumping to it
							NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qtrue );
							Jedi_TryJump( NPCS.NPCInfo->goalEntity );
						}
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
			}
		}
	}

	if ( NPCS.NPCInfo->goalEntity )
	{
		trace_t	trace;

		if ( Jedi_Jumping( NPCS.NPCInfo->goalEntity ) )
		{//in mid-jump
			return;
		}

		if ( !NAV_CheckAhead( NPCS.NPC, NPCS.NPCInfo->goalEntity->r.currentOrigin, &trace, ( NPCS.NPC->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
		{//can't get straight to him
			if ( NPC_ClearLOS4( NPCS.NPCInfo->goalEntity ) && NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qtrue ) )
			{//no line of sight
				if ( Jedi_TryJump( NPCS.NPCInfo->goalEntity ) )
				{//started a jump
					return;
				}
			}
		}
		if ( NPCS.NPCInfo->aiFlags & NPCAI_BLOCKED )
		{//try to jump to the blockedDest
			if ( fabs(NPCS.NPCInfo->blockedDest[2]-NPCS.NPC->r.currentOrigin[2]) > 64 )
			{
				gentity_t *tempGoal = G_Spawn();//ugh, this is NOT good...?
				G_SetOrigin( tempGoal, NPCS.NPCInfo->blockedDest );
				trap->LinkEntity( (sharedEntity_t *)tempGoal );
				TIMER_Set( NPCS.NPC, "jumpChaseDebounce", -1 );
				if ( Jedi_TryJump( tempGoal ) )
				{//going to jump to the dest
					G_FreeEntity( tempGoal );
					return;
				}
				G_FreeEntity( tempGoal );
			}
		}
	}
	//try normal movement
	NPC_BSFollowLeader();
}


/*
-------------------------
Jedi_Attack
-------------------------
*/

static void Jedi_Attack( void )
{
	//Don't do anything if we're in a pain anim
	if ( NPCS.NPC->painDebounceTime > level.time )
	{
		if ( Q_irand( 0, 1 ) )
		{
			Jedi_FaceEnemy( qtrue );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	if ( NPCS.NPC->client->ps.saberLockTime > level.time )
	{
		//FIXME: maybe if I'm losing I should try to force-push out of it?  Very rarely, though...
		if ( NPCS.NPC->client->ps.fd.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
			&& NPCS.NPC->client->ps.saberLockTime < level.time + 5000
			&& !Q_irand( 0, 10 ))
		{
			ForceThrow( NPCS.NPC, qfalse );
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;

			if ( NPCS.NPC->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",NPCS.NPC->NPC_type) )
			{
				if ( g_npcspskill.integer )
				{
					chance = 4.0f;//he pushes *hard*
				}
				else
				{
					chance = 3.0f;//he pushes *hard*
				}
			}
			else if ( NPCS.NPC->client->NPC_class == CLASS_TAVION )
			{
				chance = 2.0f+g_npcspskill.value;//from 2 to 4
			}
			else
			{//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				float maxChance	= (float)(RANK_LT)/2.0f+3.0f;//5?
				if ( !g_npcspskill.value )
				{
					chance = (float)(NPCS.NPCInfo->rank)/2.0f;
				}
				else
				{
					chance = (float)(NPCS.NPCInfo->rank)/2.0f+1.0f;
				}
				if ( chance > maxChance )
				{
					chance = maxChance;
				}
			}
		//	if ( flrand( -4.0f, chance ) >= 0.0f && !(NPC->client->ps.pm_flags&PMF_ATTACK_HELD) )
		//	{
		//		ucmd.buttons |= BUTTON_ATTACK;
		//	}
			if ( flrand( -4.0f, chance ) >= 0.0f )
			{
				NPCS.ucmd.buttons |= BUTTON_ATTACK;
			}
			//rwwFIXMEFIXME: support for PMF_ATTACK_HELD
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	//did we drop our saber?  If so, go after it!
	if ( NPCS.NPC->client->ps.saberInFlight )
	{//saber is not in hand
	//	if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		if (!NPCS.NPC->client->ps.saberEntityNum && NPCS.NPC->client->saberStoredIndex) //this is valid, it's 0 when our saber is gone -rww (mp-specific)
		{//
			//if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			if (1) //no matter
			{//fell to the ground, try to pick it up
			//	if ( Jedi_CanPullBackSaber( NPC ) )
				if (1) //no matter
				{
					NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCS.NPCInfo->goalEntity = &g_entities[NPCS.NPC->client->saberStoredIndex];
					NPCS.ucmd.buttons |= BUTTON_ATTACK;
					if ( NPCS.NPC->enemy && NPCS.NPC->enemy->health > 0 )
					{//get our saber back NOW!
						Jedi_Move( NPCS.NPCInfo->goalEntity, qfalse );
						NPC_UpdateAngles( qtrue, qtrue );
						if ( NPCS.NPC->enemy->s.weapon == WP_SABER )
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
	if ( NPCS.NPC->enemy )
	{
		if ( NPCS.NPC->enemy->health <= 0 && NPCS.NPC->enemy->enemy == NPCS.NPC && NPCS.NPC->client->playerTeam != NPCTEAM_PLAYER )//good guys don't gloat
		{//my enemy is dead and I killed him
			NPCS.NPCInfo->enemyCheckDebounceTime = 0;//keep looking for others

			if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				if ( NPCS.NPCInfo->walkDebounceTime < level.time && NPCS.NPCInfo->walkDebounceTime >= 0 )
				{
					TIMER_Set( NPCS.NPC, "gloatTime", 10000 );
					NPCS.NPCInfo->walkDebounceTime = -1;
				}
				if ( !TIMER_Done( NPCS.NPC, "gloatTime" ) )
				{
					if ( DistanceHorizontalSquared( NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->enemy->r.currentOrigin ) > 4096 && (NPCS.NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
						Jedi_Move( NPCS.NPC->enemy, qfalse );
						NPCS.ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{
						TIMER_Set( NPCS.NPC, "gloatTime", 0 );
					}
				}
				else if ( NPCS.NPCInfo->walkDebounceTime == -1 )
				{
					NPCS.NPCInfo->walkDebounceTime = -2;
					G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
					jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = level.time + 3000;
					NPCS.NPCInfo->desiredPitch = 0;
					NPCS.NPCInfo->goalEntity = NULL;
				}
				Jedi_FaceEnemy( qtrue );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
			else
			{
				if ( !TIMER_Done( NPCS.NPC, "parryTime" ) )
				{
					TIMER_Set( NPCS.NPC, "parryTime", -1 );
					NPCS.NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
				}
				NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
				if ( !NPCS.NPC->client->ps.saberHolstered && NPCS.NPC->client->ps.saberInFlight )
				{//saber is still on (or we're trying to pull it back), count down erosion and keep facing the enemy
					//FIXME: need to stop this from happening over and over again when they're blocking their victim's saber
					//FIXME: turn off saber sooner so we get cool walk anim?
					//Com_Printf( "(%d) drop agg - enemy dead\n", level.time );
					Jedi_AggressionErosion(-3);
					if ( BG_SabersOff( &NPCS.NPC->client->ps ) && !NPCS.NPC->client->ps.saberInFlight )
					{//turned off saber (in hand), gloat
						G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
						jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = level.time + 3000;
						NPCS.NPCInfo->desiredPitch = 0;
						NPCS.NPCInfo->goalEntity = NULL;
					}
					TIMER_Set( NPCS.NPC, "gloatTime", 10000 );
				}
				if ( !NPCS.NPC->client->ps.saberHolstered || NPCS.NPC->client->ps.saberInFlight || !TIMER_Done( NPCS.NPC, "gloatTime" ) )
				{//keep walking
					if ( DistanceHorizontalSquared( NPCS.NPC->client->renderInfo.eyePoint, NPCS.NPC->enemy->r.currentOrigin ) > 4096 && (NPCS.NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
						Jedi_Move( NPCS.NPC->enemy, qfalse );
						NPCS.ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{//got there
						if ( NPCS.NPC->health < NPCS.NPC->client->pers.maxHealth
							&& (NPCS.NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0
							&& (NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 )
						{
							ForceHeal( NPCS.NPC );
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
	if ( NPCS.NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPCS.NPC->enemy->classname ) )
	{
		if ( NPCS.NPC->enemy->count <= 0 )
		{//it's out of ammo
			if ( NPCS.NPC->enemy->activator && NPC_ValidEnemy( NPCS.NPC->enemy->activator ) )
			{
				gentity_t *turretOwner = NPCS.NPC->enemy->activator;
				G_ClearEnemy( NPCS.NPC );
				G_SetEnemy( NPCS.NPC, turretOwner );
			}
			else
			{
				G_ClearEnemy( NPCS.NPC );
			}
		}
	}
	NPC_CheckEnemy( qtrue, qtrue, qtrue );

	if ( !NPCS.NPC->enemy )
	{
		NPCS.NPC->client->ps.saberBlocked = BLOCKED_NONE;
		if ( NPCS.NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			NPCS.NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		Jedi_Patrol();//was calling Idle... why?
		return;
	}

	//always face enemy if have one
	NPCS.NPCInfo->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat();

	if ( !(NPCS.NPCInfo->scriptFlags&SCF_CHASE_ENEMIES)
		|| ((NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&NPCS.NPC->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_2))
	{//this is really stupid, but okay...
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.rightmove = 0;
		if ( NPCS.ucmd.upmove > 0 )
		{
			NPCS.ucmd.upmove = 0;
		}
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.rightmove = 0;
		VectorClear( NPCS.NPC->client->ps.moveDir );
	}

	if ( !TIMER_Done( NPCS.NPC, "duck" ) )
	{
		NPCS.ucmd.upmove = -127;
	}

	if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( PM_SaberInBrokenParry( NPCS.NPC->client->ps.saberMove ) || NPCS.NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
		{//just make sure they don't pull their saber to them if they're being blocked
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
		}
	}

	if( (NPCS.NPCInfo->scriptFlags&SCF_DONT_FIRE) //not allowed to attack
		|| ((NPCS.NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&NPCS.NPC->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_3)
		|| ((NPCS.NPC->client->ps.saberEventFlags&SEF_INWATER)&&!NPCS.NPC->client->ps.saberInFlight) )//saber in water
	{
		NPCS.ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}

	if ( NPCS.NPCInfo->scriptFlags&SCF_NO_ACROBATICS )
	{
		NPCS.ucmd.upmove = 0;
		NPCS.NPC->client->ps.fd.forceJumpCharge = 0;
	}

	if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
	{
		Jedi_CheckDecreaseSaberAnimLevel();
	}

	if ( NPCS.ucmd.buttons & BUTTON_ATTACK && NPCS.NPC->client->playerTeam == NPCTEAM_ENEMY )
	{
		if ( Q_irand( 0, NPCS.NPC->client->ps.fd.saberAnimLevel ) > 0
			&& Q_irand( 0, NPCS.NPC->client->pers.maxHealth+10 ) > NPCS.NPC->health
			&& !Q_irand( 0, 3 ))
		{//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_COMBAT1, EV_COMBAT3 ), 1000 );
		}
	}

	if ( NPCS.NPC->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( NPCS.NPC->client->NPC_class == CLASS_TAVION
			|| (g_npcspskill.integer && ( NPCS.NPC->client->NPC_class == CLASS_DESANN || NPCS.NPCInfo->rank >= Q_irand( RANK_CREWMAN, RANK_CAPTAIN ))))
		{//Tavion will kick in force speed if the player does...
			if ( NPCS.NPC->enemy
				&& NPCS.NPC->enemy->s.number >= 0 && NPCS.NPC->enemy->s.number < MAX_CLIENTS
				&& NPCS.NPC->enemy->client
				&& (NPCS.NPC->enemy->client->ps.fd.forcePowersActive & (1<<FP_SPEED))
				&& !(NPCS.NPC->client->ps.fd.forcePowersActive & (1<<FP_SPEED)) )
			{
				int chance = 0;
				switch ( g_npcspskill.integer )
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
					ForceSpeed( NPCS.NPC, 0 );
				}
			}
		}
	}
}

extern void WP_Explode( gentity_t *self );
qboolean Jedi_InSpecialMove( void )
{
	if ( NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_2
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_KYLE_PA_3
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	/*if ( Jedi_InNoAIAnim( NPC ) )
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
	}*/

	if ( NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| NPCS.NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD )
	{
		if ( !TIMER_Done( NPCS.NPC, "draining" ) )
		{//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			NPCS.ucmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( NPCS.NPC->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER )
	{
		NPCS.NPC->health += Q_irand( 1, 2 );
		if ( NPCS.NPC->health > NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH] )
		{
			NPCS.NPC->health = NPCS.NPC->client->ps.stats[STAT_MAX_HEALTH];
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}
	/*else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD )
	{
		if ( NPC->client->ps.torsoTimer <= 100 )
		{
			NPC->s.loopSound = 0;
			G_StopEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number );
			NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
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
		if ( NPC->client->ps.torsoTimer <= 1200
			&& !NPC->count )
		{
			Tavion_ScepterSlam();
			NPC->count = 1;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}*/

	if ( Jedi_CultistDestroyer( NPCS.NPC ) )
	{
		if ( !NPCS.NPC->takedamage )
		{//ready to explode
			if ( NPCS.NPC->useDebounceTime <= level.time )
			{
				//this should damage everyone - FIXME: except other destroyers?
				NPCS.NPC->client->playerTeam = NPCTEAM_FREE;//FIXME: will this destroy wampas, tusken & rancors?
				NPCS.NPC->splashDamage = 200;	// rough match to SP
				NPCS.NPC->splashRadius = 512;	// see above
				WP_Explode( NPCS.NPC );
				return qtrue;
			}
			if ( NPCS.NPC->enemy )
			{
				NPC_FaceEnemy( qfalse );
			}
			return qtrue;
		}
	}
	return qfalse;
}

extern void NPC_BSST_Patrol( void );
extern void NPC_BSSniper_Default( void );
void NPC_BSJedi_Default( void )
{
	if ( Jedi_InSpecialMove() )
	{
		return;
	}

	Jedi_CheckCloak();

	if( !NPCS.NPC->enemy )
	{//don't have an enemy, look for one
		if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
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
		if ( Jedi_WaitingAmbush( NPCS.NPC ) )
		{//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush( NPCS.NPC );
		}

		if ( Jedi_CultistDestroyer( NPCS.NPC )
			&& !NPCS.NPCInfo->charmedTime )
		{//destroyer
			//permanent effect
			NPCS.NPCInfo->charmedTime = Q3_INFINITE;
			NPCS.NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
			NPCS.NPC->client->ps.fd.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			//NPC->client->ps.eFlags |= EF_FORCE_DRAINED;
			//FIXME: precache me!
			NPCS.NPC->s.loopSound = G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );//test/charm.wav" );
		}

		if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			if ( NPCS.NPC->enemy->enemy != NPCS.NPC && NPCS.NPC->health == NPCS.NPC->client->pers.maxHealth && DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(800*800) )
			{
				NPCS.NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				Boba_ChangeWeapon( WP_DISRUPTOR );
				NPC_BSSniper_Default();
				return;
			}
		}
		Jedi_Attack();
		//if we have multiple-jedi combat, probably need to keep checking (at certain debounce intervals) for a better (closer, more active) enemy and switch if needbe...
		if ( ((!NPCS.ucmd.buttons&&!NPCS.NPC->client->ps.fd.forcePowersActive)||(NPCS.NPC->enemy&&NPCS.NPC->enemy->health<=0)) && NPCS.NPCInfo->enemyCheckDebounceTime < level.time )
		{//not doing anything (or walking toward a vanquished enemy - fixme: always taunt the player?), not using force powers and it's time to look again
			//FIXME: build a list of all local enemies (since we have to find best anyway) for other AI factors- like when to use group attacks, determine when to change tactics, when surrounded, when blocked by another in the enemy group, etc.  Should we build this group list or let the enemies maintain their own list and we just access it?
			gentity_t *sav_enemy = NPCS.NPC->enemy;//FIXME: what about NPC->lastEnemy?
			gentity_t *newEnemy;

			NPCS.NPC->enemy = NULL;
			newEnemy = NPC_CheckEnemy( (qboolean)(NPCS.NPCInfo->confusionTime < level.time), qfalse, qfalse );
			NPCS.NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
				G_SetEnemy( NPCS.NPC, newEnemy );
			}
			NPCS.NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 3000 );
		}
	}
}
