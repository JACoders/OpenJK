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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// Boba Fett
// ---------
// Ah yes, this file is pretty messy.  I've tried to move everything in here, but in fact
// a lot of his AI occurs in the seeker and jedi AI files.  Some of these functions
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#include "../Ravl/CVec.h"


////////////////////////////////////////////////////////////////////////////////////////
// Forward References Of Functions
////////////////////////////////////////////////////////////////////////////////////////
void		Boba_Precache( void );
void		Boba_DustFallNear(const vec3_t origin, int dustcount);
void		Boba_ChangeWeapon(int wp);
qboolean	Boba_StopKnockdown(gentity_t *self, gentity_t *pusher, const vec3_t pushDir, qboolean forceKnockdown = qfalse);

// Flight Related Functions (also used by Rocket Trooper)
//--------------------------------------------------------
qboolean	Boba_Flying( gentity_t *self );
void		Boba_FlyStart( gentity_t *self );
void		Boba_FlyStop( gentity_t *self );

// Called From NPC_Pain()
//-----------------------------
void		Boba_Pain( gentity_t *self, gentity_t *inflictor, int damage, int mod);


// Local: Flame Thrower Weapon
//-----------------------------
void		Boba_FireFlameThrower( gentity_t *self );
void		Boba_StopFlameThrower( gentity_t *self );
void		Boba_StartFlameThrower( gentity_t *self );
void		Boba_DoFlameThrower( gentity_t *self );

// Local: Other Tactics
//----------------------
void		Boba_DoAmbushWait( gentity_t *self);
void		Boba_DoSniper( gentity_t *self);

// Local: Respawning 
//-------------------
bool		Boba_Respawn();

// Called From Within AI_Jedi && AI_Seeker
//-----------------------------------------
void		Boba_Fire();
void		Boba_FireDecide();

// Local: Called From Tactics()
//----------------------------
void		Boba_TacticsSelect();
bool		Boba_CanSeeEnemy( gentity_t *self );


// Called From NPC_RunBehavior()
//-------------------------------
void		Boba_Update();		// Always Called First, Before Any Other Thinking
bool		Boba_Tactics();		// If returns true, Jedi and Seeker AI not used
bool		Boba_Flee();		// If returns true, Jedi and Seeker AI not used



////////////////////////////////////////////////////////////////////////////////////////
// External Functions 
////////////////////////////////////////////////////////////////////////////////////////
extern void		G_SoundAtSpot( vec3_t org, int soundIndex, qboolean broadcast );
extern void		G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern void		ChangeWeapon( gentity_t *ent, int newWeapon );
extern void		WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty );
extern void		ForceJump( gentity_t *self, usercmd_t *ucmd );
extern void		G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );

extern void CG_DrawEdge( vec3_t start, vec3_t end, int type );

////////////////////////////////////////////////////////////////////////////////////////
// External Data
////////////////////////////////////////////////////////////////////////////////////////
extern cvar_t*		g_bobaDebug;



////////////////////////////////////////////////////////////////////////////////////////
// Boba Debug Output
////////////////////////////////////////////////////////////////////////////////////////
#ifndef FINAL_BUILD
#if !defined(CTYPE_H_INC)
	#include <ctype.h>
	#define CTYPE_H_INC
#endif

#if !defined(STDARG_H_INC)
	#include <stdarg.h>
	#define STDARG_H_INC
#endif

#if !defined(STDIO_H_INC)
	#include <stdio.h>
	#define STDIO_H_INC
#endif
void	Boba_Printf(const char * format, ...)
{
	if (g_bobaDebug->integer==0)
	{
		return;
	}

	static char		string[2][1024];	// in case this is called by nested functions
	static int		index = 0;
	static char		nFormat[300];
	char*			buf;

	// Tack On The Standard Format Around The Given Format
	//-----------------------------------------------------
	Com_sprintf(nFormat, sizeof(nFormat), "[BOBA %8d] %s\n", level.time, format);


	// Resolve Remaining Elipsis Parameters Into Newly Formated String
	//-----------------------------------------------------------------
	buf = string[index & 1];
	index++;

	va_list		argptr;
	va_start (argptr, format);
	Q_vsnprintf (buf, sizeof(*string), nFormat, argptr);
	va_end (argptr);

	// Print It To Debug Output Console
	//----------------------------------
	gi.Printf(buf);
}
#else
void	Boba_Printf(const char * format, ...)
{
}
#endif


////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		BOBA_FLAMEDURATION			3000
#define		BOBA_FLAMETHROWRANGE		128
#define		BOBA_FLAMETHROWSIZE			40
#define		BOBA_FLAMETHROWDAMAGEMIN	1//10
#define		BOBA_FLAMETHROWDAMAGEMAX	5//40
#define		BOBA_ROCKETRANGEMIN			300
#define		BOBA_ROCKETRANGEMAX			2000


////////////////////////////////////////////////////////////////////////////////////////
// Global Data 
////////////////////////////////////////////////////////////////////////////////////////
bool	BobaHadDeathScript = false;
bool	BobaActive = false;
vec3_t	BobaFootStepLoc;
int		BobaFootStepCount = 0;

vec3_t	AverageEnemyDirection;
int		AverageEnemyDirectionSamples;


////////////////////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////////////////////
enum	EBobaTacticsState
{
	BTS_NONE,

	// Attack 
	//--------
	BTS_RIFLE,			// Uses Jedi / Seeker Movement
	BTS_MISSILE,		// Uses Jedi / Seeker Movement
	BTS_SNIPER,			// Uses Special Movement Internal To This File
	BTS_FLAMETHROW,		// Locked In Place

	// Waiting
	//---------
	BTS_AMBUSHWAIT,		// Goto CP & Wait

	BTS_MAX
};









////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_Precache( void )
{
	G_SoundIndex( "sound/chars/boba/bf_blast-off.wav" );
	G_SoundIndex( "sound/chars/boba/bf_jetpack_lp.wav" );
	G_SoundIndex( "sound/chars/boba/bf_land.wav" );
	G_SoundIndex( "sound/weapons/boba/bf_flame.mp3" );
	G_SoundIndex( "sound/player/footsteps/boot1" );
	G_SoundIndex( "sound/player/footsteps/boot2" );
	G_SoundIndex( "sound/player/footsteps/boot3" );
	G_SoundIndex( "sound/player/footsteps/boot4" );
	G_EffectIndex( "boba/jetSP" );
	G_EffectIndex( "boba/fthrw" );
	G_EffectIndex( "volumetric/black_smoke" );
	G_EffectIndex( "chunks/dustFall" );

	AverageEnemyDirectionSamples = 0;
	VectorClear(AverageEnemyDirection);
	BobaHadDeathScript			= false;
	BobaActive					= true;
	BobaFootStepCount			= 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void	Boba_DustFallNear(const vec3_t origin, int dustcount)
{
	if (!BobaActive)
	{
		return;
	}

	trace_t		testTrace;
	vec3_t		testDirection;
	vec3_t		testStartPos;
	vec3_t		testEndPos;

	VectorCopy(origin, testStartPos);
	for (int i=0; i<dustcount; i++)
	{
		testDirection[0] = (random() * 2.0f) - 1.0f;
		testDirection[1] = (random() * 2.0f) - 1.0f;
		testDirection[2] = 1.0f;

		VectorMA(origin, 1000.0f, testDirection, testEndPos);
		gi.trace (&testTrace, origin, NULL, NULL, testEndPos, (player && player->inuse)?(0):(ENTITYNUM_NONE), MASK_SHOT, (EG2_Collision)0, 0 );

		if (!testTrace.startsolid && 
			!testTrace.allsolid && 
			testTrace.fraction>0.1f &&
			testTrace.fraction<0.9f)
		{
			G_PlayEffect( "chunks/dustFall", testTrace.endpos, testTrace.plane.normal );
		}
	}
} 

////////////////////////////////////////////////////////////////////////////////////////
// This is just a super silly wrapper around NPC_Change Weapon
////////////////////////////////////////////////////////////////////////////////////////
void Boba_ChangeWeapon( int wp )
{
	if ( NPC->s.weapon == wp )
	{
		return;
	}
	NPC_ChangeWeapon( wp );
	G_AddEvent( NPC, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));
}

////////////////////////////////////////////////////////////////////////////////////////
// Choose an "anti-knockdown" response
////////////////////////////////////////////////////////////////////////////////////////
qboolean Boba_StopKnockdown( gentity_t *self, gentity_t *pusher, const vec3_t pushDir, qboolean forceKnockdown )
{
	if ( self->client->NPC_class != CLASS_BOBAFETT )
	{
		return qfalse;
	}

	if ( self->client->moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	vec3_t	pDir, fwd, right, ang = {0, self->currentAngles[YAW], 0};
	float	fDot, rDot;
	int		strafeTime = Q_irand( 1000, 2000 );

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
			self->client->ps.forceJumpCharge = 280;//FIXME: calc this intelligently?
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

////////////////////////////////////////////////////////////////////////////////////////
// Is this entity flying
////////////////////////////////////////////////////////////////////////////////////////
qboolean Boba_Flying( gentity_t *self )
{
	assert(self && self->client && self->client->NPC_class==CLASS_BOBAFETT);//self->NPC && 
	return ((qboolean)(self->client->moveType==MT_FLYSWIM));
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
bool	Boba_CanSeeEnemy( gentity_t *self )
{
	assert(self && self->NPC && self->client && self->client->NPC_class==CLASS_BOBAFETT);
 	return ((level.time - self->NPC->enemyLastSeenTime)<1000);
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void	Boba_Pain( gentity_t *self, gentity_t *inflictor, int damage, int mod)
{
	if (mod==MOD_SABER && !(NPCInfo->aiFlags&NPCAI_FLAMETHROW))
	{
		TIMER_Set( self, "Boba_TacticsSelect", 0);	// Hurt By The Saber, Time To Try Something New
	}
	if (self->NPC->aiFlags&NPCAI_FLAMETHROW)
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	 	self->client->ps.torsoAnimTimer  =	level.time - TIMER_Get(self, "falmeTime");
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_FlyStart( gentity_t *self )
{//switch to seeker AI for a while
	if ( TIMER_Done( self, "jetRecharge" ) 
		&& !Boba_Flying( self ) )
	{
		self->client->ps.gravity = 0;
		self->svFlags |= SVF_CUSTOM_GRAVITY;
		self->client->moveType = MT_FLYSWIM;
		//start jet effect
		self->client->jetPackTime = level.time + Q_irand( 3000, 10000 );
		if ( self->genericBolt1 != -1 )
		{
			G_PlayEffect( G_EffectIndex( "boba/jetSP" ), self->playerModel, self->genericBolt1, self->s.number, self->currentOrigin, qtrue, qtrue );
		}
		if ( self->genericBolt2 != -1 )
		{
			G_PlayEffect( G_EffectIndex( "boba/jetSP" ), self->playerModel, self->genericBolt2, self->s.number, self->currentOrigin, qtrue, qtrue );
		}

		//take-off sound
		G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/boba/bf_blast-off.wav" );
		//jet loop sound
		self->s.loopSound = G_SoundIndex( "sound/chars/boba/bf_jetpack_lp.wav" );
		if ( self->NPC )
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_FlyStop( gentity_t *self )
{
	self->client->ps.gravity = g_gravity->value;
	self->svFlags &= ~SVF_CUSTOM_GRAVITY;
	self->client->moveType = MT_RUNJUMP;
	//Stop effect
	self->client->jetPackTime = 0;
	if ( self->genericBolt1 != -1 )
	{
		G_StopEffect( "boba/jetSP", self->playerModel, self->genericBolt1, self->s.number );
	}
	if ( self->genericBolt2 != -1 )
	{
		G_StopEffect( "boba/jetSP", self->playerModel, self->genericBolt2, self->s.number );
	}

	//stop jet loop sound
	G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/boba/bf_land.wav" );

	self->s.loopSound = 0;
	if ( self->NPC )
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set( self, "jetRecharge", Q_irand( 1000, 5000 ) );
		TIMER_Set( self, "jumpChaseDebounce", Q_irand( 500, 2000 ) );
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// This func actually does the damage inflicting traces
////////////////////////////////////////////////////////////////////////////////////////
void Boba_FireFlameThrower( gentity_t *self )
{
	trace_t		tr;
	vec3_t		start, end, dir;
	CVec3		traceMins(self->mins);
	CVec3		traceMaxs(self->maxs);
	gentity_t*	traceEnt	= NULL;  
	int			damage		= Q_irand( BOBA_FLAMETHROWDAMAGEMIN, BOBA_FLAMETHROWDAMAGEMAX );

  	AngleVectors(self->currentAngles, dir, 0, 0); 
	dir[2] = 0.0f; 
	VectorCopy(self->currentOrigin, start); 
	traceMins *= 0.5f;
	traceMaxs *= 0.5f; 
	start[2] += 40.0f;

	VectorMA( start, 150.0f, dir, end ); 

	if (g_bobaDebug->integer)
	{
		CG_DrawEdge(start, end, EDGE_IMPACT_POSSIBLE);
	}
	gi.trace( &tr, start, self->mins, self->maxs, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0);

	traceEnt = &g_entities[tr.entityNum];
	if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
	{
		G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC|DAMAGE_IGNORE_TEAM, MOD_LAVA, HL_NONE );
		if (traceEnt->health>0)
		{
//			G_Knockdown( traceEnt, self, dir, Q_irand(200, 330), qfalse);
			G_Throw(traceEnt, dir, 30);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_StopFlameThrower( gentity_t *self )
{
	if ( self->s.number < MAX_CLIENTS )
	{
		self->client->ps.torsoAnimTimer  =	0;
		G_StopEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number);
		return;
	}
	if ((NPCInfo->aiFlags&NPCAI_FLAMETHROW))
	{
		self->NPC->aiFlags				&= ~NPCAI_FLAMETHROW;
		self->client->ps.torsoAnimTimer  =	0;

		TIMER_Set( self, "flameTime",			0);
		TIMER_Set( self, "nextAttackDelay",		0);
		TIMER_Set( self, "Boba_TacticsSelect",	0);

	//	G_SoundOnEnt( self, CHAN_WEAPON, "sound/effects/flameoff.mp3" );
		G_StopEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number);

		Boba_Printf("FlameThrower OFF");
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_StartFlameThrower( gentity_t *self )
{
	if (!(NPCInfo->aiFlags&NPCAI_FLAMETHROW))
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

		self->NPC->aiFlags				|=  NPCAI_FLAMETHROW;
	 	self->client->ps.torsoAnimTimer  =	BOBA_FLAMEDURATION;

		TIMER_Set( self, "flameTime",			BOBA_FLAMEDURATION);
		TIMER_Set( self, "nextAttackDelay",		BOBA_FLAMEDURATION);
		TIMER_Set( self, "nextFlameDelay",		BOBA_FLAMEDURATION*2);
		TIMER_Set( self, "Boba_TacticsSelect",	BOBA_FLAMEDURATION);

		G_SoundOnEnt( self, CHAN_WEAPON, "sound/weapons/boba/bf_flame.mp3" );
		G_PlayEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number, self->s.origin, 1 );

		Boba_Printf("FlameThrower ON");
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void Boba_DoFlameThrower( gentity_t *self )
{
	if ( self->s.number < MAX_CLIENTS )
	{
		if ( self->client )
		{
			if ( !self->client->ps.forcePowerDuration[FP_LIGHTNING] )
			{
				NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	 			self->client->ps.torsoAnimTimer  =	BOBA_FLAMEDURATION;
				G_SoundOnEnt( self, CHAN_WEAPON, "sound/weapons/boba/bf_flame.mp3" );
				G_PlayEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number, self->s.origin, 1 );
				self->client->ps.forcePowerDuration[FP_LIGHTNING] = 1;
			}
			Boba_FireFlameThrower( self );
		}
		return;
	}
	if (!(NPCInfo->aiFlags&NPCAI_FLAMETHROW) && TIMER_Done(self, "nextAttackDelay"))
	{
		Boba_StartFlameThrower( self );
	}

	if ( (NPCInfo->aiFlags&NPCAI_FLAMETHROW))
	{
		Boba_FireFlameThrower( self );
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void		Boba_DoAmbushWait( gentity_t *self)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void		Boba_DoSniper( gentity_t *self)
{
	if (TIMER_Done(NPC, "PickNewSniperPoint"))
	{
		TIMER_Set(NPC, "PickNewSniperPoint", Q_irand(15000, 25000));
 		int		SniperPoint = NPC_FindCombatPoint(NPC->currentOrigin, 0, NPC->currentOrigin, CP_SNIPE|CP_CLEAR|CP_HAS_ROUTE|CP_TRYFAR|CP_HORZ_DIST_COLL, 0, -1);
		if (SniperPoint!=-1)
		{
			NPC_SetCombatPoint(SniperPoint);
			NPC_SetMoveGoal( NPC, level.combatPoints[SniperPoint].origin, 20, qtrue, SniperPoint );
		}
	}

    if (Distance(NPC->currentOrigin, level.combatPoints[NPCInfo->combatPoint].origin)<50.0f)
	{
		Boba_FireDecide();
	}


	bool	IsOnAPath = !!NPC_MoveToGoal(qtrue);

	// Resolve Blocked Problems
	//--------------------------
	if (NPCInfo->aiFlags&NPCAI_BLOCKED && 
		NPC->client->moveType!=MT_FLYSWIM && 
		((level.time - NPCInfo->blockedDebounceTime)>3000)
		)
	{
		Boba_Printf("BLOCKED: Attempting Jump");
		if (IsOnAPath)
		{
			if (!NPC_TryJump(NPCInfo->blockedTargetPosition))
			{
				Boba_Printf("  Failed");
			}
		}
	}

	NPC_FaceEnemy(qtrue);
	NPC_UpdateAngles( qtrue, qtrue );
}


////////////////////////////////////////////////////////////////////////////////////////
// Call This function to make Boba actually shoot his current weapon
////////////////////////////////////////////////////////////////////////////////////////
void	Boba_Fire()
{
	WeaponThink(qtrue);

	// If Actually Fired, Decide To Apply Alt Fire And Calc Next Attack Delay
	//------------------------------------------------------------------------
	if (ucmd.buttons&BUTTON_ATTACK)
	{
		switch (NPC->s.weapon)
		{
		case WP_ROCKET_LAUNCHER:
			TIMER_Set( NPC, "nextAttackDelay", Q_irand(1000, 2000));

			// Occasionally Shoot A Homing Missile
			//-------------------------------------
			if (!Q_irand(0,3))
			{
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |=  BUTTON_ALT_ATTACK;
				NPC->client->fireDelay = Q_irand( 1000, 3000 );
			}
			break;

		case WP_DISRUPTOR:
			TIMER_Set(NPC, "nextAttackDelay", Q_irand(1000, 4000));

			// Occasionally Alt-Fire
			//-----------------------
			if (!Q_irand(0,3))
			{
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |=  BUTTON_ALT_ATTACK;
				NPC->client->fireDelay = Q_irand( 1000, 3000 );
			}
			break;

		case WP_BLASTER:

			if (TIMER_Done(NPC, "nextBlasterAltFireDecide"))
			{
 			 	if (Q_irand(0, (NPC->count*2)+3)>2)
				{
			 		TIMER_Set(NPC, "nextBlasterAltFireDecide", Q_irand(3000, 8000));
					if (!(NPCInfo->scriptFlags&SCF_ALT_FIRE))
					{
						Boba_Printf("ALT FIRE On");
						NPCInfo->scriptFlags |= SCF_ALT_FIRE;
						NPC_ChangeWeapon(WP_BLASTER);			// Update Delay Timers
					}
				}
				else
				{
					TIMER_Set(NPC, "nextBlasterAltFireDecide", Q_irand(2000, 5000));
					if ( (NPCInfo->scriptFlags&SCF_ALT_FIRE))
					{
						Boba_Printf("ALT FIRE Off");
						NPCInfo->scriptFlags &=~SCF_ALT_FIRE;
						NPC_ChangeWeapon(WP_BLASTER);			// Update Delay Timers
					}
				}
			}

			// Occasionally Alt Fire
			//-----------------------
			if (NPCInfo->scriptFlags&SCF_ALT_FIRE)
			{
				ucmd.buttons &= ~BUTTON_ATTACK;
				ucmd.buttons |=  BUTTON_ALT_ATTACK;
			}
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Call this function to see if Fett should fire his current weapon
////////////////////////////////////////////////////////////////////////////////////////
void Boba_FireDecide( void )
{
	// Any Reason Not To Shoot?
	//--------------------------
	if (!NPC ||											// Only NPCs
		!NPC->client ||									// Only Clients
		 NPC->client->NPC_class!=CLASS_BOBAFETT ||		// Only Boba
		!NPC->enemy ||									// Only If There Is An Enemy
		 NPC->s.weapon==WP_NONE ||						// Only If Using A Valid Weapon
		!TIMER_Done(NPC, "nextAttackDelay") ||			// Only If Ready To Shoot Again
		!Boba_CanSeeEnemy(NPC)							// Only If Enemy Recently Seen
		)
	{
		return;
	}

	// Now Check Weapon Specific Parameters To See If We Should Shoot Or Not
	//-----------------------------------------------------------------------
	switch (NPC->s.weapon)
	{
	case WP_ROCKET_LAUNCHER:
		if (Distance(NPC->currentOrigin, NPC->enemy->currentOrigin)>400.0f)
		{
			Boba_Fire();
		}
		break;

	case WP_DISRUPTOR:
		// TODO: Add Conditions Here
		Boba_Fire();
		break;

	case WP_BLASTER:
		// TODO: Add Conditions Here
		Boba_Fire();
		break;
	}
}



////////////////////////////////////////////////////////////////////////////////////////
// Tactics avaliable to Boba Fett:
// --------------------------------
//	BTS_RIFLE,			// Uses Jedi / Seeker Movement
//	BTS_MISSILE,		// Uses Jedi / Seeker Movement
//	BTS_SNIPER,			// Uses Special Movement Internal To This File
//	BTS_FLAMETHROW,		// Locked In Place
//	BTS_AMBUSHWAIT,		// Goto CP & Wait
//
//
// Weapons available to Boba Fett:
// --------------------------------
//	WP_NONE   (Flame Thrower)
//	WP_ROCKET_LAUNCHER
//	WP_BLASTER
//	WP_DISRUPTOR
//  
////////////////////////////////////////////////////////////////////////////////////////
void	Boba_TacticsSelect()
{
	// Don't Change Tactics For A Little While
	//------------------------------------------
	TIMER_Set(NPC, "Boba_TacticsSelect", Q_irand(8000, 15000));
	int		nextState = NPCInfo->localState;


	// Get Some Data That Will Help With The Selection Of The Next Tactic
	//--------------------------------------------------------------------
	bool	enemyAlive			= (NPC->enemy->health>0);
	float	enemyDistance		= Distance(NPC->currentOrigin, NPC->enemy->currentOrigin);
	bool	enemyInFlameRange	= (enemyDistance<BOBA_FLAMETHROWRANGE);
	bool	enemyInRocketRange	= (enemyDistance>BOBA_ROCKETRANGEMIN && enemyDistance<BOBA_ROCKETRANGEMAX);
	bool	enemyRecentlySeen	= Boba_CanSeeEnemy(NPC);


	// Enemy Is Really Close
	//-----------------------
	if (!enemyAlive)
	{
		nextState = BTS_RIFLE;
	}
	else if (enemyInFlameRange)
	{
		// If It's Been Long Enough Since Our Last Flame Blast, Try To Torch The Enemy
		//-----------------------------------------------------------------------------
		if (TIMER_Done(NPC, "nextFlameDelay"))
		{
			nextState = BTS_FLAMETHROW;
		}

		// Otherwise, He's Probably Too Close, So Try To Get Clear Of Him
		//----------------------------------------------------------------
		else
		{
			nextState = BTS_RIFLE;
		}
	}

	// Recently Saw The Enemy, Time For Some Good Ole Fighten!
	//---------------------------------------------------------
	else if (enemyRecentlySeen)
	{
		// At First, Boba will prefer to use his blaster against the player, but
		//  the more times he is driven away (NPC->count), he will be less likely to
		//  choose the blaster, and more likely to go for the missile launcher
		nextState = (!enemyInRocketRange || Q_irand(0, NPC->count)<1)?(BTS_RIFLE):(BTS_MISSILE);
	}

	// Hmmm...  Havn't Seen The Player In A While, We Might Want To Try Something Sneaky
	//-----------------------------------------------------------------------------------
	else
	{
		bool	SnipePointsNear = false;		 // TODO
		bool	AmbushPointNear = false;		 // TODO

		if (Q_irand(0, NPC->count)>0)
		{
			int		SniperPoint = NPC_FindCombatPoint(NPC->currentOrigin, 0, NPC->currentOrigin, CP_SNIPE|CP_CLEAR|CP_HAS_ROUTE|CP_TRYFAR|CP_HORZ_DIST_COLL, 0, -1);
			if (SniperPoint!=-1)
			{
				NPC_SetCombatPoint(SniperPoint);
				NPC_SetMoveGoal( NPC, level.combatPoints[SniperPoint].origin, 20, qtrue, SniperPoint );
				TIMER_Set(NPC, "PickNewSniperPoint", Q_irand(15000, 25000));
				SnipePointsNear = true;
			}
		}


 		if (SnipePointsNear && TIMER_Done(NPC, "Boba_NoSniperTime"))
		{
			TIMER_Set(NPC, "Boba_NoSniperTime", 120000);				// Don't snipe again for a while
			TIMER_Set(NPC, "Boba_TacticsSelect", Q_irand(35000, 45000));// More patience here
			nextState = BTS_SNIPER;
		}
		else if (AmbushPointNear)
		{
			TIMER_Set(NPC, "Boba_TacticsSelect", Q_irand(15000, 25000));// More patience here
			nextState = BTS_AMBUSHWAIT;
		}
		else
		{
			nextState = (!enemyInRocketRange || Q_irand(0, NPC->count)<1)?(BTS_RIFLE):(BTS_MISSILE);
		}
	}



	// The Next State Has Been Selected, Now Change Weapon If Necessary 
	//------------------------------------------------------------------
	if (nextState!=NPCInfo->localState)
	{
		NPCInfo->localState = nextState;
		switch (NPCInfo->localState)
		{
		case BTS_FLAMETHROW:
			Boba_Printf("NEW TACTIC: Flame Thrower");
			Boba_ChangeWeapon(WP_NONE);
			Boba_DoFlameThrower(NPC);
			break;

		case BTS_RIFLE:
			Boba_Printf("NEW TACTIC: Rifle");
			Boba_ChangeWeapon(WP_BLASTER);
			break;

		case BTS_MISSILE:
			Boba_Printf("NEW TACTIC: Rocket Launcher");
			Boba_ChangeWeapon(WP_ROCKET_LAUNCHER);
			break;

		case BTS_SNIPER:
			Boba_Printf("NEW TACTIC: Sniper");
			Boba_ChangeWeapon(WP_DISRUPTOR);
			break;

		case BTS_AMBUSHWAIT:
			Boba_Printf("NEW TACTIC: Ambush");
			Boba_ChangeWeapon(WP_NONE);
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Tactics
//
// This function is called right after Update()
// If returns true, Jedi and Seeker AI not used for movement
//
////////////////////////////////////////////////////////////////////////////////////////
bool	Boba_Tactics()
{
	if (!NPC->enemy)
	{
		return false;
	}

	// Think About Changing Tactics
	//------------------------------
	if (TIMER_Done(NPC, "Boba_TacticsSelect"))
	{
		Boba_TacticsSelect();
	}

	// These Tactics Require Seeker & Jedi Movement
	//----------------------------------------------
	if (!NPCInfo->localState ||
		 NPCInfo->localState==BTS_RIFLE || 
		 NPCInfo->localState==BTS_MISSILE)
	{
		return false;
	}

	// Flame Thrower - Locked In Place
	//---------------------------------
	if (NPCInfo->localState==BTS_FLAMETHROW)
	{
		Boba_DoFlameThrower( NPC );
	}

	// Sniper - Move Around, And Take Shots
	//--------------------------------------
	else if (NPCInfo->localState==BTS_SNIPER)
	{
		Boba_DoSniper( NPC );
	}

	// Ambush Wait
	//------------
	else if (NPCInfo->localState==BTS_AMBUSHWAIT)
	{
		Boba_DoAmbushWait( NPC );
	}


	NPC_FacePosition( NPC->enemy->currentOrigin, qtrue);
	NPC_UpdateAngles(qtrue, qtrue);

	return true;			// Do Not Use Normal Jedi Or Seeker Movement
}


////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
bool	Boba_Respawn()
{
	int cp = -1;

	// Try To Predict Where The Enemy Is Going
	//-----------------------------------------
	if (AverageEnemyDirectionSamples && NPC->behaviorSet[BSET_DEATH]==0)
	{
		vec3_t	endPos;
	 	VectorMA(NPC->enemy->currentOrigin, 1000.0f / (float)AverageEnemyDirectionSamples, AverageEnemyDirection, endPos);
		cp = NPC_FindCombatPoint(endPos, 0, endPos, CP_FLEE|CP_TRYFAR|CP_HORZ_DIST_COLL, 0, -1);
		Boba_Printf("Attempting Predictive Spawn Point");
	}

	// If That Failed, Try To Go Directly To The Enemy
	//-------------------------------------------------
	if (cp==-1)
	{
		cp = NPC_FindCombatPoint(NPC->enemy->currentOrigin, 0, NPC->enemy->currentOrigin, CP_FLEE|CP_TRYFAR|CP_HORZ_DIST_COLL, 0, -1);
		Boba_Printf("Attempting Closest Current Spawn Point");
	}

	// If We've Found One, Go There
	//------------------------------
	if (cp!=-1)
	{
		NPC_SetCombatPoint( cp );
		NPCInfo->surrenderTime = 0;
		NPC->health = NPC->max_health;
		NPC->svFlags &=~SVF_NOCLIENT;
		NPC->count ++;										// This is the number of times spawned
		G_SetOrigin(NPC, level.combatPoints[cp].origin);

		AverageEnemyDirectionSamples = 0;
		VectorClear(AverageEnemyDirection);

		Boba_Printf("Found Spawn Point (%d)", cp);
		return true;
	}

	assert(0);	// Yea, that's bad...
	Boba_Printf("FAILED TO FIND SPAWN POINT");
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
void	Boba_Update()
{
	// Never Forget The Player... Never.
	//-----------------------------------
	if (player && player->inuse && !NPC->enemy)
	{
		G_SetEnemy(NPC, player);
		NPC->svFlags				|= SVF_LOCKEDENEMY;	// Don't forget about the enemy once you've found him
	}

	// Hey, This Is Boba, He Tests The Trace All The Time
	//----------------------------------------------------
	if (NPC->enemy)
	{
		if (!(NPC->svFlags&SVF_NOCLIENT))
		{
			trace_t		testTrace;
			vec3_t		eyes;
			CalcEntitySpot( NPC, SPOT_HEAD_LEAN, eyes );
			gi.trace (&testTrace, eyes, NULL, NULL, NPC->enemy->currentOrigin, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0);

			bool	wasSeen = Boba_CanSeeEnemy(NPC);

			if (!testTrace.startsolid && 
				!testTrace.allsolid && 
				testTrace.entityNum == NPC->enemy->s.number)
			{
				NPCInfo->enemyLastSeenTime	= level.time;
				NPCInfo->enemyLastHeardTime	= level.time;
				VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastSeenLocation);
				VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastHeardLocation);
			}
			else if (gi.inPVS( NPC->enemy->currentOrigin, NPC->currentOrigin))
			{
				NPCInfo->enemyLastHeardTime	= level.time;
				VectorCopy(NPC->enemy->currentOrigin, NPCInfo->enemyLastHeardLocation);
			}

			if (g_bobaDebug->integer)
			{
				bool	nowSeen = Boba_CanSeeEnemy(NPC);
				if (!wasSeen && nowSeen)
				{
					Boba_Printf("Enemy Seen");
				}
				if (wasSeen && !nowSeen)
				{
					Boba_Printf("Enemy Lost");
				}
				CG_DrawEdge(NPC->currentOrigin, NPC->enemy->currentOrigin, (nowSeen)?(EDGE_IMPACT_SAFE):(EDGE_IMPACT_POSSIBLE));
			}
		}

		if (!NPCInfo->surrenderTime)
		{
			if ((level.time - NPCInfo->enemyLastSeenTime)>20000 && TIMER_Done(NPC, "TooLongGoneRespawn"))
			{
				TIMER_Set(NPC, "TooLongGoneRespawn", 30000);	// Give him some time to get to you before trying again
				Boba_Printf("Gone Too Long, Attempting Respawn Even Though Not Hiding");
				Boba_Respawn();
			}
		}
	}


	// Make Sure He Always Appears In The Last Area With Full Health When His Death Script Is Turned On
	//--------------------------------------------------------------------------------------------------
	if (!BobaHadDeathScript && NPC->behaviorSet[BSET_DEATH]!=0)
	{
		if (!gi.inPVS(NPC->enemy->currentOrigin, NPC->currentOrigin))
		{
			Boba_Printf("Attempting Final Battle Spawn...");
			if (Boba_Respawn())
			{
				BobaHadDeathScript = true;
			}
			else
			{
				Boba_Printf("Failed");
			}
		}
	}



	// Don't Forget To Turn Off That Flame Thrower, Mr. Fett - You're Waisting Precious Natural Gases
	//------------------------------------------------------------------------------------------------
	if ((NPCInfo->aiFlags&NPCAI_FLAMETHROW) && (TIMER_Done(NPC, "flameTime")))
	{
		Boba_StopFlameThrower(NPC);
	}


	// Occasionally A Jump Turns Into A Rocket Fly
	//---------------------------------------------
	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE 
		&& NPC->client->ps.forceJumpZStart
		&& !Q_irand( 0, 10 ) )
	{//take off
		Boba_FlyStart( NPC );
	}


	// If Hurting, Try To Run Away
	//-----------------------------
	if (!NPCInfo->surrenderTime && (NPC->health<NPC->max_health/10))
	{
		Boba_Printf("Time To Surrender, Searching For Flee Point");


		// Find The Closest Flee Point That I Can Get To
		//-----------------------------------------------
		int cp = NPC_FindCombatPoint(NPC->currentOrigin, 0, NPC->currentOrigin, CP_FLEE|CP_HAS_ROUTE|CP_TRYFAR|CP_HORZ_DIST_COLL, 0, -1);
		if (cp!=-1)
		{
			NPC_SetCombatPoint( cp );
			NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp );
			if (NPC->count<6)
			{
 	 		 	NPCInfo->surrenderTime = level.time + Q_irand(5000, 10000) + 1000*(6-NPC->count);
			}
			else
			{
 	 			NPCInfo->surrenderTime = level.time + Q_irand(5000, 10000);
			}
		}
		else
		{
			Boba_Printf("  Failure");
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////////////
bool	Boba_Flee()
{
	bool	EnemyRecentlySeen	= ((level.time - NPCInfo->enemyLastSeenTime)<10000);
	bool	ReachedEscapePoint	= (Distance(level.combatPoints[NPCInfo->combatPoint].origin, NPC->currentOrigin)<50.0f);
	bool	HasBeenGoneEnough	= (level.time>NPCInfo->surrenderTime || (level.time - NPCInfo->enemyLastSeenTime)>400000);


	// Is It Time To Come Back For Some More?
	//----------------------------------------
 	if (!EnemyRecentlySeen || ReachedEscapePoint)
	{
		NPC->svFlags |= SVF_NOCLIENT;
		if (HasBeenGoneEnough)
		{
			if ((level.time - NPCInfo->enemyLastSeenTime)>400000)
			{
				Boba_Printf("  Gone Too Long, Attempting Respawn");
			}

			if (Boba_Respawn())
			{
				return true;
			}
		}
	  	else if (ReachedEscapePoint && (NPCInfo->surrenderTime - level.time)>3000)
		{
 			if (TIMER_Done(NPC, "SpookPlayerTimer"))
			{
				vec3_t		testDirection;
				TIMER_Set(NPC, "SpookPlayerTimer", Q_irand(2000, 10000));
				switch(Q_irand(0, 1))
				{
				case 0:
					Boba_Printf("SPOOK: Dust");
					Boba_DustFallNear(NPC->enemy->currentOrigin, Q_irand(1,2));
					break;

				case 1:
					Boba_Printf("SPOOK: Footsteps");
  					testDirection[0] =  (random() * 0.5f) - 1.0f;
	 		 		testDirection[0] += (testDirection[0]>0.0f)?(0.5f):(-0.5f);
					testDirection[1] = (random() * 0.5f) - 1.0f;
					testDirection[1] += (testDirection[1]>0.0f)?(0.5f):(-0.5f);
					testDirection[2] = 1.0f;
		 	 		VectorMA(NPC->enemy->currentOrigin, 400.0f, testDirection, BobaFootStepLoc);

					BobaFootStepCount = Q_irand(3,8);
					break;
				}
			}

			if (BobaFootStepCount && TIMER_Done(NPC, "BobaFootStepFakeTimer"))
			{
				TIMER_Set(NPC, "BobaFootStepFakeTimer", Q_irand(300, 800));
				BobaFootStepCount --;
				G_SoundAtSpot(BobaFootStepLoc, G_SoundIndex(va("sound/player/footsteps/boot%d", Q_irand(1,4))), qtrue);
			}

		 	if (TIMER_Done(NPC, "ResampleEnemyDirection") && NPC->enemy->resultspeed>10.0f)
			{
				TIMER_Set(NPC, "ResampleEnemyDirection", Q_irand(500, 1000));
				AverageEnemyDirectionSamples ++;

				vec3_t	moveDir;
				VectorCopy(NPC->enemy->client->ps.velocity, moveDir);
				VectorNormalize(moveDir);

				VectorAdd(AverageEnemyDirection, moveDir, AverageEnemyDirection);
			}

	 		if (g_bobaDebug->integer && AverageEnemyDirectionSamples)
			{
				vec3_t	endPos;
				VectorMA(NPC->enemy->currentOrigin, 500.0f / (float)AverageEnemyDirectionSamples, AverageEnemyDirection, endPos);
				CG_DrawEdge(NPC->enemy->currentOrigin, endPos, EDGE_IMPACT_POSSIBLE);
			}
		}
	}
	else
	{
		NPCInfo->surrenderTime += 100;
	}

	// Finish The Flame Thrower First...
	//-----------------------------------
	if (NPCInfo->aiFlags&NPCAI_FLAMETHROW)
	{
		Boba_DoFlameThrower( NPC );
		NPC_FacePosition( NPC->enemy->currentOrigin, qtrue);
		NPC_UpdateAngles(qtrue, qtrue);
		return true;
	}

	bool	IsOnAPath = !!NPC_MoveToGoal(qtrue);
	if (!ReachedEscapePoint &&
		NPCInfo->aiFlags&NPCAI_BLOCKED && 
		NPC->client->moveType!=MT_FLYSWIM && 
		((level.time - NPCInfo->blockedDebounceTime)>1000)
		)
	{
		if (!Boba_CanSeeEnemy(NPC) && Distance(NPC->currentOrigin, level.combatPoints[NPCInfo->combatPoint].origin)<200)
		{
			Boba_Printf("BLOCKED: Just Teleporting There");
			G_SetOrigin(NPC, level.combatPoints[NPCInfo->combatPoint].origin);
		}
		else
		{
			Boba_Printf("BLOCKED: Attempting Jump");

			if (IsOnAPath)
			{
				if (NPC_TryJump(NPCInfo->blockedTargetPosition))
				{
				}
				else
				{
					Boba_Printf("  Failed");
				}
			}
			else if (EnemyRecentlySeen)
			{
				if (NPC_TryJump(NPCInfo->enemyLastSeenLocation))
				{
	 			}
				else
				{
					Boba_Printf("  Failed");
				}
			}
		}
	}


	NPC_UpdateAngles( qtrue, qtrue );
	return true;
}