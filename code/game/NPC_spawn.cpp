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

//b_spawn.cpp
//added by MCG

#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"

extern qboolean G_CheckInSolid (gentity_t *self, qboolean fix);
extern void ClientUserinfoChanged( int clientNum );
extern qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
extern void Jedi_Cloak( gentity_t *self );
extern void Saboteur_Cloak( gentity_t *self );

extern void G_MatchPlayerWeapon( gentity_t *ent );
extern void Q3_SetParm (int entID, int parmNum, const char *parmValue);

extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );

extern int WP_SaberInitBladeData( gentity_t *ent );
extern void ST_ClearTimers( gentity_t *ent );
extern void Jedi_ClearTimers( gentity_t *ent );
extern void Howler_ClearTimers( gentity_t *self );
#define	NSF_DROP_TO_FLOOR	16


/*
-------------------------
NPC_PainFunc
-------------------------
*/

painFunc_t NPC_PainFunc( gentity_t *ent )
{
	painFunc_t	func;

	if ( ent->client->ps.weapon == WP_SABER )
	{
		func = painF_NPC_Jedi_Pain;
	}
	else
	{
		// team no longer indicates race/species, use NPC_class to determine different npc types
		/*
		switch ( ent->client->playerTeam )
		{
		default:
			func = painF_NPC_Pain;
			break;
		}
		*/
		switch( ent->client->NPC_class )
		{
		// troopers get special pain
		case CLASS_SABOTEUR:
		case CLASS_STORMTROOPER:
		case CLASS_SWAMPTROOPER:
		case CLASS_NOGHRI:
			func = painF_NPC_ST_Pain;
			break;

		case CLASS_SEEKER:
			func = painF_NPC_Seeker_Pain;
			break;

		case CLASS_REMOTE:
			func = painF_NPC_Remote_Pain;
			break;

		case CLASS_MINEMONSTER:
			func = painF_NPC_MineMonster_Pain;
			break;

		case CLASS_HOWLER:
			func = painF_NPC_Howler_Pain;
			break;

		case CLASS_RANCOR:
			func = painF_NPC_Rancor_Pain;
			break;

		case CLASS_WAMPA:
			func = painF_NPC_Wampa_Pain;
			break;

		case CLASS_SAND_CREATURE:
			func = painF_NPC_SandCreature_Pain;
			break;

		// all other droids, did I miss any?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MOUSE:
		case CLASS_PROTOCOL:
		case CLASS_INTERROGATOR:
			func = painF_NPC_Droid_Pain;
			break;
		case CLASS_PROBE:
			func = painF_NPC_Probe_Pain;
			break;

		case CLASS_SENTRY:
			func = painF_NPC_Sentry_Pain;
			break;
		case CLASS_MARK1:
			func = painF_NPC_Mark1_Pain;
			break;
		case CLASS_MARK2:
			func = painF_NPC_Mark2_Pain;
			break;
		case CLASS_ATST:  
			func = painF_NPC_ATST_Pain;
			break;
		case CLASS_GALAKMECH:
			func = painF_NPC_GM_Pain;
			break;
		// everyone else gets the normal pain func
		default:
			func = painF_NPC_Pain;
			break;
		}

	}

	return func;
}


/*
-------------------------
NPC_TouchFunc
-------------------------
*/

touchFunc_t NPC_TouchFunc( gentity_t *ent )
{
	return touchF_NPC_Touch;
}

/*
-------------------------
NPC_SetMiscDefaultData
-------------------------
*/
void G_ClassSetDontFlee( gentity_t *self )
{
	if ( !self || !self->client || !self->NPC )
	{
		return;
	}
	switch ( self->client->NPC_class )
	{
	case CLASS_ATST:
	case CLASS_CLAW:
	case CLASS_DESANN:
	case CLASS_FISH:
	case CLASS_FLIER2:
	case CLASS_GALAK:
	case CLASS_GLIDER:
	case CLASS_RANCOR:
	case CLASS_SAND_CREATURE:
	case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	case CLASS_JEDI:				
	case CLASS_KYLE:
	case CLASS_LANDO:			
	case CLASS_LIZARD:
	case CLASS_LUKE:				
	case CLASS_MARK1:			// droid
	case CLASS_MARK2:			// droid
	case CLASS_GALAKMECH:		// droid
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	case CLASS_MURJJ:
	case CLASS_PROBE:			// droid
	case CLASS_REBORN:
	case CLASS_REELO:
	case CLASS_REMOTE:
	case CLASS_SEEKER:			// droid
	case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_SWAMP:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_BOBAFETT:
	case CLASS_SABER_DROID:
	case CLASS_ASSASSIN_DROID:
	case CLASS_PLAYER:
	case CLASS_VEHICLE:
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
		break;
	default:
		break;
	}
	if ( (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if ( (self->NPC->aiFlags&NPCAI_SUBBOSS_CHARACTER) )
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if ( (self->NPC->aiFlags&NPCAI_ROSH) )
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if ( (self->NPC->aiFlags&NPCAI_HEAL_ROSH) )
	{
		self->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
}

extern void	Vehicle_Register(gentity_t *ent);
extern void RT_FlyStart( gentity_t *self );
extern void SandCreature_ClearTimers( gentity_t *ent );
void NPC_SetMiscDefaultData( gentity_t *ent )
{
	if ( ent->spawnflags & SFB_CINEMATIC )
	{//if a cinematic guy, default us to wait bState
		ent->NPC->behaviorState = BS_CINEMATIC;
	}
	if ( ent->client->NPC_class == CLASS_RANCOR )
	{
		if ( Q_stricmp( "mutant_rancor", ent->NPC_type ) == 0 )
		{
			ent->spawnflags |= 1;//just so I know it's a mutant rancor as opposed to a normal one
			ent->NPC->aiFlags |= NPCAI_NAV_THROUGH_BREAKABLES;
			ent->mass = 2000;
		}
		else
		{
			ent->NPC->aiFlags |= NPCAI_NAV_THROUGH_BREAKABLES;
			ent->mass = 1000;
		}
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else if ( ent->client->NPC_class == CLASS_SAND_CREATURE )
	{//???
		ent->clipmask = CONTENTS_SOLID|CONTENTS_MONSTERCLIP;//it can go through others
		ent->contents = 0;//can't be hit?
		ent->takedamage = qfalse;//can't be killed
		ent->flags |= FL_NO_KNOCKBACK;
		SandCreature_ClearTimers( ent );
	}
	else if ( ent->client->NPC_class == CLASS_BOBAFETT )
	{//set some stuff, precache
		ent->client->ps.forcePowersKnown |= ( 1 << FP_LEVITATION );
		ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
		ent->client->ps.forcePower	= 100;
		ent->NPC->scriptFlags		|= (SCF_NAV_CAN_FLY|SCF_FLY_WITH_JET|SCF_NAV_CAN_JUMP);
		NPC->flags					|= FL_UNDYING;		// Can't Kill Boba
	}
	else if ( ent->client->NPC_class == CLASS_ROCKETTROOPER )
	{//set some stuff, precache
		ent->client->ps.forcePowersKnown |= ( 1 << FP_LEVITATION );
		ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_3;
		ent->client->ps.forcePower = 100;
		ent->NPC->scriptFlags |= (SCF_NAV_CAN_FLY|SCF_FLY_WITH_JET|SCF_NAV_CAN_JUMP);//no groups, no combat points!
		if ( Q_stricmp( "rockettrooper2Officer", ent->NPC_type ) == 0 )
		{//start in the air, use spotlight
			//ent->NPC->scriptFlags |= SCF_NO_GROUPS;
			ent->NPC->scriptFlags &= ~SCF_FLY_WITH_JET;
			RT_FlyStart( ent );
			NPC_SetMoveGoal( ent, ent->currentOrigin, 16, qfalse, -1, NULL );
			VectorCopy( ent->currentOrigin, ent->pos1 );
		}
		if ( (ent->spawnflags&2) )
		{//spotlight
			ent->client->ps.eFlags |= EF_SPOTLIGHT;
		}
	}
	else if (ent->client->NPC_class == CLASS_SABER_DROID)
	{
		ent->flags |= FL_NO_KNOCKBACK;
	}
	else if ( ent->client->NPC_class == CLASS_SABOTEUR )
	{//can cloak
		ent->NPC->aiFlags |= NPCAI_SHIELDS;//give them the ability to cloak
		if ( (ent->spawnflags&16) )
		{//start cloaked
			Saboteur_Cloak( ent );
		}
	}
	else if ( ent->client->NPC_class == CLASS_ASSASSIN_DROID )
	{
		ent->client->ps.stats[STAT_ARMOR] = 250;	// start with full armor
		if (ent->s.weapon==WP_BLASTER)
		{
			ent->NPC->scriptFlags |= SCF_ALT_FIRE;
		}
		ent->flags |= (FL_NO_KNOCKBACK);
	}

	if (ent->spawnflags&4096)
	{
		ent->NPC->scriptFlags |= SCF_NO_GROUPS;//don't use combat points or group AI
	}

	if ( Q_stricmp( "DKothos", ent->NPC_type ) == 0
		|| Q_stricmp( "VKothos", ent->NPC_type ) == 0 )
	{
		ent->NPC->scriptFlags |= SCF_DONT_FIRE;
		ent->NPC->aiFlags |= NPCAI_HEAL_ROSH;
		ent->count = 100;
	}
	else if ( Q_stricmp( "rosh_dark", ent->NPC_type ) == 0 )
	{
		ent->NPC->aiFlags |= NPCAI_ROSH;
	}

	if ( Q_stricmpn( ent->NPC_type, "hazardtrooper", 13 ) == 0 )
	{//hazard trooper
		ent->NPC->scriptFlags |= SCF_NO_GROUPS;//don't use combat points or group AI
		ent->flags |= (FL_SHIELDED|FL_NO_KNOCKBACK);//low-level shots bounce off, no knockback
	}
	if ( !Q_stricmp( "Yoda", ent->NPC_type ) )
	{//FIXME: extern this into NPC.cfg?
		ent->NPC->scriptFlags |= SCF_NO_FORCE;//force powers don't work on him
		ent->NPC->aiFlags |= NPCAI_BOSS_CHARACTER;
	}
	if ( !Q_stricmp( "emperor", ent->NPC_type ) 
		|| !Q_stricmp( "cultist_grip", ent->NPC_type )
		|| !Q_stricmp( "cultist_drain", ent->NPC_type )
		|| !Q_stricmp( "cultist_lightning", ent->NPC_type ))
	{//FIXME: extern this into NPC.cfg?
		ent->NPC->scriptFlags |= SCF_DONT_FIRE;//so he uses only force powers
	}
	if (!Q_stricmp( "Rax", ent->NPC_type ) )
	{
		ent->NPC->scriptFlags |= SCF_DONT_FLEE;
	}
	if ( !Q_stricmp( "cultist_destroyer", ent->NPC_type ) )
	{
		ent->splashDamage = 1000;
		ent->splashRadius = 384;
		//FIXME: precache these!
		ent->fxID = G_EffectIndex( "force/destruction_exp" );
		ent->NPC->scriptFlags |= (SCF_DONT_FLEE|SCF_IGNORE_ALERTS);
		ent->NPC->ignorePain = qtrue;
	}
	if ( Q_stricmp( "chewie", ent->NPC_type ) )
	{
		//in case chewie ever loses his gun...
		ent->NPC->aiFlags |= NPCAI_HEAVY_MELEE;
	}
	//==================
	if ( ent->client->ps.saber[0].type != SABER_NONE 
		&& (!(ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)||!ent->weaponModel[0]) )
	{//if I'm equipped with a saber, initialize it (them)
		ent->client->ps.SaberDeactivate();
		ent->client->ps.SetSaberLength( 0 );
		WP_SaberInitBladeData( ent );
		if ( ent->client->ps.weapon == WP_SABER )
		{//this is our current weapon, add the models now
			WP_SaberAddG2SaberModels( ent );
		}
		Jedi_ClearTimers( ent );
	}
	if ( ent->client->ps.forcePowersKnown != 0 )
	{
		WP_InitForcePowers( ent );
		if (ent->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0)
		{
			ent->NPC->scriptFlags |= SCF_NAV_CAN_JUMP;	// anyone who has any force jump can jump
		}
	}
	if ( ent->client->NPC_class == CLASS_HOWLER )
	{
		Howler_ClearTimers( ent );
		ent->NPC->scriptFlags |= SCF_NO_FALLTODEATH;
		ent->flags |= FL_NO_IMPACT_DMG;
		ent->NPC->scriptFlags |= SCF_NAV_CAN_JUMP;	// These jokers can jump
	}
	if ( ent->client->NPC_class == CLASS_DESANN
		|| ent->client->NPC_class == CLASS_TAVION
		|| ent->client->NPC_class == CLASS_LUKE
		|| ent->client->NPC_class == CLASS_KYLE
		|| Q_stricmp("tavion_scepter", ent->NPC_type ) == 0 
		|| Q_stricmp("alora_dual", ent->NPC_type ) == 0 )
	{
		ent->NPC->aiFlags |= NPCAI_BOSS_CHARACTER;
	}
	else if ( Q_stricmp( "alora", ent->NPC_type ) == 0
		|| Q_stricmp( "rosh_dark", ent->NPC_type ) == 0 )
	{
		ent->NPC->aiFlags |= NPCAI_SUBBOSS_CHARACTER;
	}
	if ( ent->client->NPC_class == CLASS_TUSKEN )
	{
		if ( g_spskill->integer > 1 )
		{//on hard, tusken raiders are faster than you
			ent->NPC->stats.runSpeed = 280;
			ent->NPC->stats.walkSpeed = 65;
		}
	}
	//***I'm not sure whether I should leave this as a TEAM_ switch, I think NPC_class may be more appropriate - dmv
	switch(ent->client->playerTeam)
	{
	case TEAM_PLAYER:
		//ent->flags |= FL_NO_KNOCKBACK;
		if ( ent->client->NPC_class == CLASS_SEEKER )
		{		
			ent->NPC->defaultBehavior = BS_DEFAULT;
			ent->client->ps.gravity = 0;
			ent->svFlags |= SVF_CUSTOM_GRAVITY;
			ent->client->moveType = MT_FLYSWIM;
			ent->count = 30; // SEEKER shot ammo count
			return;
		}
		else if ( ent->client->NPC_class == CLASS_JEDI 
			|| ent->client->NPC_class == CLASS_KYLE
			|| ent->client->NPC_class == CLASS_LUKE )
		{//good jedi
			ent->client->enemyTeam = TEAM_ENEMY;
			if ( ent->spawnflags & JSF_AMBUSH )
			{//ambusher
				ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
				ent->client->noclip = true;//hang
			}
		}
		else
		{
			if (ent->client->ps.weapon != WP_NONE
				&& ent->client->ps.weapon != WP_SABER //sabers done above
				&& (!(ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)||!ent->weaponModel[0]) )//they do this themselves
			{
				G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
			}
			switch ( ent->client->ps.weapon )
			{
			case WP_BRYAR_PISTOL://FIXME: new weapon: imp blaster pistol
			case WP_BLASTER_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
			case WP_CONCUSSION:
			default:
				break;
			case WP_THERMAL:
			case WP_BLASTER:
				//FIXME: health in NPCs.cfg, and not all blaster users are stormtroopers
				//ent->health = 25;
				//FIXME: not necc. a ST
				ST_ClearTimers( ent );
				if ( ent->NPC->rank >= RANK_LT || ent->client->ps.weapon == WP_THERMAL )
				{//officers, grenade-throwers use alt-fire
					//ent->health = 50;
					//ent->NPC->scriptFlags |= SCF_ALT_FIRE;
				}
				break;
			}
		}
		if ( ent->client->NPC_class == CLASS_PLAYER 
			|| ent->client->NPC_class == CLASS_VEHICLE 
			|| (ent->spawnflags & SFB_CINEMATIC) )
		{
			ent->NPC->defaultBehavior = BS_CINEMATIC;
		}
		else
		{
			ent->NPC->defaultBehavior = BS_FOLLOW_LEADER;
			ent->client->leader = &g_entities[0];//player
		}
		break;

	case TEAM_NEUTRAL: 

		if ( Q_stricmp( ent->NPC_type, "gonk" ) == 0 ) 
		{
			// I guess we generically make them player usable
			ent->svFlags |= SVF_PLAYER_USABLE;

			// Not even sure if we want to give different levels of batteries?  ...Or even that these are the values we'd want to use.
			switch ( g_spskill->integer )
			{
			case 0:	//	EASY
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.8f; 
				break;
			case 1:	//	MEDIUM
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.75f; 
				break;
			default :
			case 2:	//	HARD
				ent->client->ps.batteryCharge = MAX_BATTERIES * 0.5f; 
				break;
			}
		}
		break;

	case TEAM_ENEMY:
		{
			ent->NPC->defaultBehavior = BS_DEFAULT;
			if ( ent->client->NPC_class == CLASS_SHADOWTROOPER
				&& Q_stricmpn("shadowtrooper", ent->NPC_type, 13 ) == 0 )
			{//FIXME: a spawnflag?
				Jedi_Cloak( ent );
			}
		 	if( ent->client->NPC_class == CLASS_TAVION ||
				ent->client->NPC_class == CLASS_ALORA ||
				(ent->client->NPC_class == CLASS_REBORN && ent->client->ps.weapon == WP_SABER) ||
				ent->client->NPC_class == CLASS_DESANN ||
				ent->client->NPC_class == CLASS_SHADOWTROOPER )
			{
				ent->client->enemyTeam = TEAM_PLAYER;
				if ( ent->spawnflags & JSF_AMBUSH )
				{//ambusher
					ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
					ent->client->noclip = true;//hang
				}
			}
			else if( ent->client->NPC_class == CLASS_PROBE || ent->client->NPC_class == CLASS_REMOTE ||
						ent->client->NPC_class == CLASS_INTERROGATOR || ent->client->NPC_class == CLASS_SENTRY)
			{		
				ent->NPC->defaultBehavior = BS_DEFAULT;
				ent->client->ps.gravity = 0;
				ent->svFlags |= SVF_CUSTOM_GRAVITY;
				ent->client->moveType = MT_FLYSWIM;
			}
			else 
			{
				if ( ent->client->ps.weapon != WP_NONE
					&& ent->client->ps.weapon != WP_SABER//sabers done above
					&& (!(ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)||!ent->weaponModel[0]) )//they do this themselves
				{
					G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
				}
				switch ( ent->client->ps.weapon )
				{
				case WP_BRYAR_PISTOL:
					break;
				case WP_BLASTER_PISTOL:
					NPCInfo->scriptFlags |= SCF_PILOT;
					if ( ent->client->NPC_class == CLASS_REBORN
						&& ent->NPC->rank >= RANK_LT_COMM
						&& (!(ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)||!ent->weaponModel[0]) )//they do this themselves
					{//dual blaster pistols, so add the left-hand one, too
						G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handLBolt, 1 );
					}
					break;
				case WP_DISRUPTOR:
					//Sniper
					//ent->NPC->scriptFlags |= SCF_ALT_FIRE;//FIXME: use primary fire sometimes?  Up close?  Different class of NPC?
					break;
				case WP_BOWCASTER:
					NPCInfo->scriptFlags |= SCF_PILOT;
					break;
				case WP_REPEATER:
					NPCInfo->scriptFlags |= SCF_PILOT;
					//machine-gunner
					break;
				case WP_DEMP2:
					break;
				case WP_FLECHETTE:
					NPCInfo->scriptFlags |= SCF_PILOT;
					//shotgunner
					if ( !Q_stricmp( "stofficeralt", ent->NPC_type ) )
					{
						//ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					break;
				case WP_ROCKET_LAUNCHER:
					break;
				case WP_CONCUSSION:
					break;
				case WP_THERMAL:
					//Gran, use main, bouncy fire
//					ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					break;
				case WP_MELEE:
					break;
				case WP_NOGHRI_STICK:
					break;
				default:
				case WP_BLASTER:
					//FIXME: health in NPCs.cfg, and not all blaster users are stormtroopers
					//FIXME: not necc. a ST
					NPCInfo->scriptFlags |= SCF_PILOT;

					ST_ClearTimers( ent );
					if ( ent->NPC->rank >= RANK_COMMANDER )
					{//commanders use alt-fire
						//ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					if ( !Q_stricmp( "rodian2", ent->NPC_type ) )
					{
						//ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					break;
				}
			}
		}
		break;

	default:
		ent->NPC->defaultBehavior = BS_DEFAULT;
		if ( ent->client->ps.weapon != WP_NONE 
			&& ent->client->ps.weapon != WP_MELEE 
			&& ent->client->ps.weapon != WP_SABER//sabers done above
			&& (!(ent->NPC->aiFlags&NPCAI_MATCHPLAYERWEAPON)||!ent->weaponModel[0]) )//they do this themselves
		{
			G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl, ent->handRBolt, 0 );
		}
		break;
	}


	if ( ent->client->NPC_class == CLASS_ATST || ent->client->NPC_class == CLASS_MARK1 ) // chris/steve/kevin requested that the mark1 be shielded also
	{
		ent->flags |= (FL_SHIELDED|FL_NO_KNOCKBACK);
	}

	// Set CAN FLY Flag for Navigation On The Following Classes
	//----------------------------------------------------------
	if (ent->client->NPC_class==CLASS_PROBE ||
		ent->client->NPC_class==CLASS_REMOTE ||
		ent->client->NPC_class==CLASS_SEEKER ||
		ent->client->NPC_class==CLASS_SENTRY ||
		ent->client->NPC_class==CLASS_GLIDER ||
		ent->client->NPC_class==CLASS_IMPWORKER ||
		ent->client->NPC_class==CLASS_BOBAFETT ||
		ent->client->NPC_class==CLASS_ROCKETTROOPER 
		)
	{
		ent->NPC->scriptFlags |= SCF_NAV_CAN_FLY;
	}

	if (ent->client->NPC_class==CLASS_VEHICLE)
	{
		Vehicle_Register(ent);
	}

	if ( ent->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) )
	{
		if ( !ent->weaponModel[1] )
		{//we have the scepter, so put it in our left hand if we don't already have a second weapon
			G_CreateG2AttachedWeaponModel( ent, weaponData[WP_SCEPTER].weaponMdl, ent->handLBolt, 1 );
		}
		ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[1]], "*flash");
	}

	if ( ent->client->ps.saber[0].type == SABER_SITH_SWORD )
	{
		ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[0]], "*flash");
		G_PlayEffect( G_EffectIndex( "scepter/sword.efx" ), ent->weaponModel[0], ent->genericBolt1, ent->s.number, ent->currentOrigin, qtrue, qtrue );
		//how many times can she recharge?
		ent->count = g_spskill->integer*2;
		//To make sure she can do it at least once
		ent->flags |= FL_UNDYING;
	}

	if ( ent->client->ps.weapon == WP_NOGHRI_STICK 
		&& ent->weaponModel[0] )
	{
		ent->genericBolt1 = gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[0]], "*flash");
	}

	G_ClassSetDontFlee( ent );
}

/*
-------------------------
NPC_WeaponsForTeam
-------------------------
*/

int NPC_WeaponsForTeam( team_t team, int spawnflags, const char *NPC_type )
{
	//*** not sure how to handle this, should I pass in class instead of team and go from there? - dmv
	switch(team)
	{
	// no longer exists
//	case TEAM_BORG:
//		break;

//	case TEAM_HIROGEN:
//		if( Q_stricmp( "hirogenalpha", NPC_type ) == 0 )
//			return ( 1 << WP_BLASTER);
		//Falls through

//	case TEAM_KLINGON:

		//NOTENOTE: Falls through

//	case TEAM_IMPERIAL:
	case TEAM_ENEMY:
		if ( Q_stricmp( "tavion", NPC_type ) == 0 || 
			Q_stricmpn( "reborn", NPC_type, 6 ) == 0 || 
			Q_stricmp( "desann", NPC_type ) == 0 || 
			Q_stricmpn( "shadowtrooper", NPC_type, 13 ) == 0 )
			return ( 1 << WP_SABER);
//			return ( 1 << WP_IMPERIAL_BLADE);
		//NOTENOTE: Falls through if not a knife user

//	case TEAM_SCAVENGERS:
//	case TEAM_MALON:
		//FIXME: default weapon in npc config?
		if ( Q_stricmpn( "stofficer", NPC_type, 9 ) == 0 )
		{
			return ( 1 << WP_FLECHETTE);
		}
		if ( Q_stricmp( "stcommander", NPC_type ) == 0 )
		{
			return ( 1 << WP_REPEATER);
		}
		if ( Q_stricmp( "swamptrooper", NPC_type ) == 0 )
		{
			return ( 1 << WP_FLECHETTE);
		}
		if ( Q_stricmp( "swamptrooper2", NPC_type ) == 0 )
		{
			return ( 1 << WP_REPEATER);
		}
		if ( Q_stricmp( "rockettrooper", NPC_type ) == 0 )
		{
			return ( 1 << WP_ROCKET_LAUNCHER);
		}
		if ( Q_stricmpn( "shadowtrooper", NPC_type, 13 ) == 0 )
		{
			return ( 1 << WP_SABER);//|( 1 << WP_RAPID_CONCUSSION)?
		}
		if ( Q_stricmp( "imperial", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER_PISTOL);
		}
		if ( Q_stricmpn( "impworker", NPC_type, 9 ) == 0 )
		{
			return ( 1 << WP_BLASTER_PISTOL);
		}
		if ( Q_stricmp( "stormpilot", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER_PISTOL);
		}
		if ( Q_stricmp( "galak", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER);
		}
		if ( Q_stricmp( "galak_mech", NPC_type ) == 0 )
		{
			return ( 1 << WP_REPEATER);
		}
		if ( Q_stricmpn( "ugnaught", NPC_type, 8 ) == 0 )
		{
			return WP_NONE;
		}
		if ( Q_stricmp( "granshooter", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER);
		}
		if ( Q_stricmp( "granboxer", NPC_type ) == 0 )
		{
			return ( 1 << WP_MELEE);
		}
		if ( Q_stricmpn( "gran", NPC_type, 4 ) == 0 )
		{
			return (( 1 << WP_THERMAL)|( 1 << WP_MELEE));
		}
		if ( Q_stricmp( "rodian", NPC_type ) == 0 )
		{
			return ( 1 << WP_DISRUPTOR);
		}
		if ( Q_stricmp( "rodian2", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER);
		}

		if (( Q_stricmp( "interrogator",NPC_type) == 0) || ( Q_stricmp( "sentry",NPC_type) == 0) || (Q_stricmpn( "protocol",NPC_type,8) == 0) )
		{
			return WP_NONE;
		}

		if ( Q_stricmpn( "weequay", NPC_type, 7 ) == 0 )
		{
			return ( 1 << WP_BOWCASTER);//|( 1 << WP_STAFF )(FIXME: new weap?)
		}
		if ( Q_stricmp( "impofficer", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER);
		}
		if ( Q_stricmp( "impcommander", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER);
		}
		if (( Q_stricmp( "probe", NPC_type ) == 0 ) || ( Q_stricmp( "seeker", NPC_type ) == 0 ))
		{
			return ( 1 << WP_BOT_LASER);
		}	
		if ( Q_stricmpn( "remote", NPC_type, 6 ) == 0 )
		{
			return ( 1 << WP_BOT_LASER );
		}	
		if ( Q_stricmp( "trandoshan", NPC_type ) == 0 )
		{
			return (1<<WP_REPEATER);
		}
		if ( Q_stricmp( "atst", NPC_type ) == 0 )
		{
			return (( 1 << WP_ATST_MAIN)|( 1 << WP_ATST_SIDE));
		}
		if ( Q_stricmp( "mark1", NPC_type ) == 0 )
		{
			return ( 1 << WP_BOT_LASER);
		}
		if ( Q_stricmp( "mark2", NPC_type ) == 0 )
		{
			return ( 1 << WP_BOT_LASER);
		}
		if ( Q_stricmp( "minemonster", NPC_type ) == 0 )
		{
			return (( 1 << WP_MELEE));
		}
		if ( Q_stricmp( "howler", NPC_type ) == 0 )
		{
			return (( 1 << WP_MELEE));
		}
		//Stormtroopers, etc.
		return ( 1 << WP_BLASTER);
		break;

	case TEAM_PLAYER:
		
//		if(spawnflags & SFB_TRICORDER)
//			return ( 1 << WP_TRICORDER);
		
		if(spawnflags & SFB_RIFLEMAN)
			return ( 1 << WP_REPEATER);
		
		if(spawnflags & SFB_PHASER)
			return ( 1 << WP_BLASTER_PISTOL);

		if ( Q_stricmpn( "jedi", NPC_type, 4 ) == 0 || Q_stricmp( "luke", NPC_type ) == 0 )
			return ( 1 << WP_SABER);

		if ( Q_stricmpn( "prisoner", NPC_type, 8 ) == 0 
			|| Q_stricmpn( "elder", NPC_type, 5 ) == 0 )
		{
			return WP_NONE;
		}

		if ( Q_stricmpn( "bespincop", NPC_type, 9 ) == 0 )
		{
			return ( 1 << WP_BLASTER_PISTOL);
		}

		if ( Q_stricmp( "MonMothma", NPC_type ) == 0 )
		{
			return WP_NONE;
		}

		//rebel
		return ( 1 << WP_BLASTER);
		break;

	case TEAM_NEUTRAL:

		if ( Q_stricmp( "mark1", NPC_type ) == 0 )
		{
			return WP_NONE;
		}	
		if ( Q_stricmp( "mark2", NPC_type ) == 0 )
		{
			return WP_NONE;
		}	
		if ( Q_stricmpn( "ugnaught", NPC_type, 8 ) == 0 )
		{
			return WP_NONE;
		}	
		if ( Q_stricmp( "bartender", NPC_type ) == 0 )
		{
			return WP_NONE;
		}
		if ( Q_stricmp( "morgankatarn", NPC_type ) == 0 )
		{
			return WP_NONE;
		}
	
		break;

	// these no longer exist
//	case TEAM_FORGE:
//		return ( 1 << WP_MELEE);
//		break;

//	case TEAM_STASIS:
//		return ( 1 << WP_MELEE);
//		break;

//	case TEAM_PARASITE:
//		break;

//	case TEAM_8472:
//		break;

	default:
		break;
	}

	return WP_NONE;
}

extern void ChangeWeapon( gentity_t *ent, int newWeapon );

/*
-------------------------
NPC_SetWeapons
-------------------------
*/

void NPC_SetWeapons( gentity_t *ent )
{
	int			bestWeap = WP_NONE;
	int			weapons = NPC_WeaponsForTeam( ent->client->playerTeam, ent->spawnflags, ent->NPC_type );

	ent->client->ps.stats[STAT_WEAPONS] = 0;
	for ( int curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++ )
	{
		if ( (weapons & ( 1 << curWeap )) )
		{
			ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << curWeap );
			RegisterItem( FindItemForWeapon( (weapon_t)(curWeap) ) );	//precache the weapon
			ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[curWeap].ammoIndex] = 100;//FIXME: max ammo

			if ( bestWeap == WP_SABER )
			{
				// still want to register other weapons -- force saber to be best weap
				continue;
			}

			if ( curWeap == WP_MELEE )
			{
				if ( bestWeap == WP_NONE )
				{// We'll only consider giving Melee since we haven't found anything better yet.
					bestWeap = curWeap;
				}
			}
			else if ( curWeap > bestWeap || bestWeap == WP_MELEE )
			{
				// This will never override saber as best weap.  Also will override WP_MELEE if something better comes later in the list
				bestWeap = curWeap;
			}
		}
	}

	ent->client->ps.weapon = bestWeap;
}

/*
-------------------------
NPC_SpawnEffect

  NOTE:  Make sure any effects called here have their models, tga's and sounds precached in
			CG_RegisterNPCEffects in cg_player.cpp
-------------------------
*/

static void NPC_SpawnEffect (gentity_t *ent)
{
}

//--------------------------------------------------------------
// NPC_SetFX_SpawnStates
//
// Set up any special parms for spawn effects
//--------------------------------------------------------------
void NPC_SetFX_SpawnStates( gentity_t *ent )
{
	ent->client->ps.gravity = g_gravity->value;
}

//--------------------------------------------------------------
extern qboolean	stop_icarus;
void NPC_Begin (gentity_t *ent)
{
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	usercmd_t	ucmd;
	gentity_t	*spawnPoint = NULL;

	memset( &ucmd, 0, sizeof( ucmd ) );

	if ( !(ent->spawnflags & SFB_NOTSOLID) )
	{//No NPCs should telefrag
		if ( Q_stricmp( ent->NPC_type, "nullDriver") == 0 )
		{//FIXME: should be a better way to check this
		}
		else if( SpotWouldTelefrag( ent, TEAM_FREE ) )//(team_t)(ent->client->playerTeam)
		{
			if ( ent->wait < 0 )
			{//remove yourself
				Quake3Game()->DebugPrint( IGameInterface::WL_ERROR, "NPC %s could not spawn, firing target3 (%s) and removing self\n", ent->targetname, ent->target3 );
				//Fire off our target3
				G_UseTargets2( ent, ent, ent->target3 );

				//Kill us
				ent->e_ThinkFunc = thinkF_G_FreeEntity;
				ent->nextthink = level.time + 100;
			}
			else
			{
				Quake3Game()->DebugPrint( IGameInterface::WL_ERROR, "NPC %s at (%5.0f %5.0f %5.0f) couldn't spawn, waiting %4.2f secs to try again\n",
					ent->targetname, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], ent->wait/1000.0f );
				ent->e_ThinkFunc = thinkF_NPC_Begin;
				ent->nextthink = level.time + ent->wait;//try again in half a second
			}
			return;
		}
	}
	//Spawn effect
	NPC_SpawnEffect( ent );

	VectorCopy( ent->client->ps.origin, spawn_origin);
	VectorCopy( ent->s.angles, spawn_angles);
	spawn_angles[YAW] = ent->NPC->desiredYaw;

	client = ent->client;

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;

	client->airOutTime = level.time + 12000;

	client->ps.clientNum = ent->s.number;
	// clear entity values

	if ( ent->health )	// Was health supplied in map
	{
		ent->max_health = client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = ent->health;
	}
	else if ( ent->NPC->stats.health )	// Was health supplied in NPC.cfg?
	{
		
		if ( ent->client->NPC_class != CLASS_REBORN
			&& ent->client->NPC_class != CLASS_SHADOWTROOPER 
			//&& ent->client->NPC_class != CLASS_TAVION
			//&& ent->client->NPC_class != CLASS_DESANN 
			&& ent->client->NPC_class != CLASS_JEDI )
		{// up everyone except jedi
			if ( !Q_stricmp("tavion_sith_sword", ent->NPC_type )
				|| !Q_stricmp("tavion_scepter", ent->NPC_type )
				|| !Q_stricmp("kyle_boss", ent->NPC_type )
				|| !Q_stricmp("alora_dual", ent->NPC_type )
				|| !Q_stricmp("alora_boss", ent->NPC_type ) )
			{//bosses are a bit different
				ent->NPC->stats.health = ceil((float)ent->NPC->stats.health*0.75f + ((float)ent->NPC->stats.health/4.0f*g_spskill->value)); // 75% on easy, 100% on medium, 125% on hard
			}
			else
			{
				ent->NPC->stats.health += ent->NPC->stats.health/4 * g_spskill->integer; // 100% on easy, 125% on medium, 150% on hard
			}
		}
		
		ent->max_health = client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = ent->NPC->stats.health;
	}
	else
	{
		ent->max_health = client->pers.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = 100;
	}

	if ( !Q_stricmp( "rodian", ent->NPC_type ) )
	{//sniper
		//NOTE: this will get overridden by any aim settings in their spawnscripts
		switch ( g_spskill->integer )
		{
		case 0:
			ent->NPC->stats.aim = 1;
			break;
		case 1:
			ent->NPC->stats.aim = Q_irand( 2, 3 );
			break;
		case 2:
			ent->NPC->stats.aim = Q_irand( 3, 4 );
			break;
		}
	}
	else if ( ent->client->NPC_class == CLASS_STORMTROOPER
		|| ent->client->NPC_class == CLASS_SWAMPTROOPER
		|| ent->client->NPC_class == CLASS_IMPWORKER
		|| !Q_stricmp( "rodian2", ent->NPC_type ) )
	{//tweak yawspeed for these NPCs based on difficulty
		switch ( g_spskill->integer )
		{
		case 0:
			ent->NPC->stats.yawSpeed *= 0.75f;
			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{
				ent->NPC->stats.aim -= Q_irand( 3, 6 );
			}
			break;
		case 1:
			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{
				ent->NPC->stats.aim -= Q_irand( 2, 4 );
			}
			break;
		case 2:
			ent->NPC->stats.yawSpeed *= 1.5f;
			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{
				ent->NPC->stats.aim -= Q_irand( 0, 2 );
			}
			break;
		}
	}
	else if ( ent->client->NPC_class == CLASS_REBORN
		|| ent->client->NPC_class == CLASS_SHADOWTROOPER )
	{
		switch ( g_spskill->integer )
		{
		case 1:
			ent->NPC->stats.yawSpeed *= 1.25f;
			break;
		case 2:
			ent->NPC->stats.yawSpeed *= 1.5f;
			break;
		}
	}


	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->mass = 10;
	ent->takedamage = qtrue;

	/*ent->inuse = qtrue;
	SetInUse(ent);
	ent->m_iIcarusID = -1; // ICARUS_INVALID
	*/
	assert(ent->inuse);
	assert(ent->m_iIcarusID==IIcarusInterface::ICARUS_INVALID); 

	// CRITICAL NOTE! This was already done somewhere else and it was overwriting the previous value!!!
	if ( !ent->classname || Q_stricmp( ent->classname, "noclass" ) == 0 )
		ent->classname = "NPC";

//	if ( ent->client->race == RACE_HOLOGRAM )
//	{//can shoot through holograms, but not walk through them
//		ent->contents = CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_ITEM;//contents_corspe to make them show up in ID and use traces
//		ent->clipmask = MASK_NPCSOLID;
//	} else
	if(!(ent->spawnflags & SFB_NOTSOLID))
	{
		ent->contents = CONTENTS_BODY;
		ent->clipmask = MASK_NPCSOLID;
	}
	else
	{
		ent->contents = 0;
		ent->clipmask = MASK_NPCSOLID&~CONTENTS_BODY;
	}
	if(!ent->client->moveType)//Static?
	{
		ent->client->moveType = MT_RUNJUMP;
	}
	ent->e_DieFunc = dieF_player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	
	//visible to player and NPCs
	if ( ent->client->NPC_class != CLASS_R2D2 &&
		ent->client->NPC_class != CLASS_R5D2 &&
		ent->client->NPC_class != CLASS_MOUSE &&
		ent->client->NPC_class != CLASS_GONK &&
		ent->client->NPC_class != CLASS_PROTOCOL )
	{
		ent->flags &= ~FL_NOTARGET;
	}
	ent->s.eFlags &= ~EF_NODRAW;

	NPC_SetFX_SpawnStates( ent );
	
	client->ps.friction = 6;

	if ( ent->client->ps.weapon == WP_NONE )
	{//not set by the NPCs.cfg
		NPC_SetWeapons(ent);
	}
	//select the weapon
	ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex];
	ent->client->ps.weaponstate = WEAPON_IDLE;
	ChangeWeapon( ent, ent->client->ps.weapon );

	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	// clear entity state values
	ent->s.eType = ET_PLAYER;
//	ent->s.skinNum = ent - g_entities - 1;	// used as index to get custom models

	VectorCopy (spawn_origin, ent->s.origin);
//	ent->s.origin[2] += 1;	// make sure off ground

	SetClientViewAngle( ent, spawn_angles );
	client->renderInfo.lookTarget = ENTITYNUM_NONE;
	//clear IK grabbing stuff
	client->ps.heldClient = client->ps.heldByClient = ENTITYNUM_NONE;

	if(!(ent->spawnflags & 64))
	{
		if ( Q_stricmp( ent->NPC_type, "nullDriver") == 0 )
		{//FIXME: should be a better way to check this
		}
		else 
		{	
			G_KillBox( ent );
		}
		gi.linkentity (ent);
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->latched_buttons = 0;

	if ( ent->client->NPC_class != CLASS_VEHICLE )
	{
		// set default animations
		NPC_SetAnim( ent, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL );
	}

	if( spawnPoint )
	{
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	//ICARUS include
	// NOTE! Does this need to be called here? It seems to be getting called twice...
	// Once before the level starts, then once again when the entity is spawned!
	Quake3Game()->InitEntity( ent );

//==NPC initialization
	SetNPCGlobals( ent );

	ent->enemy = NPCInfo->eventualGoal;
	NPCInfo->timeOfDeath = 0;
	NPCInfo->shotTime = 0;
	NPC_ClearGoal();
	NPC_ChangeWeapon( ent->client->ps.weapon );

//==Final NPC initialization
	ent->e_PainFunc  = NPC_PainFunc( ent ); //painF_NPC_Pain;
	ent->e_TouchFunc = NPC_TouchFunc( ent ); //touchF_NPC_Touch;
//	ent->NPC->side = 1;

	ent->client->ps.ping = ent->NPC->stats.reactions * 50;

	//MCG - Begin: NPC hacks
	//FIXME: Set the team correctly
	ent->client->ps.persistant[PERS_TEAM] = ent->client->playerTeam;

	ent->e_UseFunc   = useF_NPC_Use;
	ent->e_ThinkFunc = thinkF_NPC_Think;
	ent->nextthink = level.time + FRAMETIME + Q_irand(0, 100);

	NPC_SetMiscDefaultData( ent );
	if ( ent->health <= 0 )
	{
		//ORIGINAL ID: health will count down towards max_health
		ent->health = client->ps.stats[STAT_HEALTH] = ent->max_health;
	}
	else
	{
		client->ps.stats[STAT_HEALTH] = ent->max_health = ent->health;
	}
	ChangeWeapon( ent, ent->client->ps.weapon );//yes, again... sigh

	if ( !(ent->spawnflags & SFB_STARTINSOLID) )
	{//Not okay to start in solid
		G_CheckInSolid( ent, qtrue );
	}
	VectorClear( ent->NPC->lastClearOrigin );

	//Run a script if you have one assigned to you
	if ( G_ActivateBehavior( ent, BSET_SPAWN ) )
	{
		if( ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID/*ent->taskManager*/ && !stop_icarus )
		{
			IIcarusInterface::GetIcarus()->Update( ent->m_iIcarusID );
		}
	}

	VectorCopy( ent->currentOrigin, ent->client->renderInfo.eyePoint );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	memset( &ucmd, 0, sizeof( ucmd ) );
	_VectorCopy( client->pers.cmd_angles, ucmd.angles );
	
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;

	if ( ent->NPC->aiFlags & NPCAI_MATCHPLAYERWEAPON )
	{
		G_MatchPlayerWeapon( ent );
	}

	ClientThink( ent->s.number, &ucmd );

	gi.linkentity( ent );

	if ( ent->client->playerTeam == TEAM_ENEMY || ent->client->playerTeam == TEAM_FREE )
	{//valid enemy spawned
		if ( !(ent->spawnflags&SFB_CINEMATIC) && ent->NPC->behaviorState != BS_CINEMATIC )
		{//not a cinematic enemy
			if ( g_entities[0].client )
			{
				g_entities[0].client->sess.missionStats.enemiesSpawned++;
			}
		}
	}
}

/*
-------------------------
NPC_StasisSpawn_Go
-------------------------
*/
/*	
qboolean NPC_StasisSpawn_Go( gentity_t *ent )
{
	//Setup an owner pointer if we need it
	if VALIDSTRING( ent->ownername )
	{
		ent->owner = G_Find( NULL, FOFS( targetname ), ent->ownername );
		
		if ( ( ent->owner ) && ( ent->owner->health <= 0 ) )
		{//our spawner thing is broken
			if ( ent->target2 && ent->target2[0] )
			{
				//Fire off our target2
				G_UseTargets2( ent, ent, ent->target2 );

				//Kill us
				ent->e_ThinkFunc = thinkF_G_FreeEntity;
				ent->nextthink = level.time + 100;
			}
			else
			{
				//Try to spawn again in one second
				ent->e_ThinkFunc = thinkF_NPC_Spawn_Go;
				ent->nextthink = level.time + 1000;
			}
			return qfalse;
		}
	}

	//Test for an entity blocking the spawn
	trace_t	tr;
	gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, ent->currentOrigin, ent->s.number, MASK_NPCSOLID );

	//Can't have anything in the way
	if ( tr.allsolid || tr.startsolid )
	{
		ent->nextthink = level.time + 1000;
		return qfalse;
	}

	return qtrue;
}
*/
void NPC_DefaultScriptFlags( gentity_t *ent )
{
	if ( !ent || !ent->NPC )
	{
		return;
	}
	//Set up default script flags
	ent->NPC->scriptFlags = (SCF_CHASE_ENEMIES|SCF_LOOK_FOR_ENEMIES);
}

#define MAX_SAFESPAWN_ENTS 4

bool NPC_SafeSpawn( gentity_t *ent, float safeRadius )
{
	gentity_t	*radiusEnts[ MAX_SAFESPAWN_ENTS ];
	vec3_t		safeMins, safeMaxs;
	float		distance = 999999;
	int			numEnts = 0;
	float		safeRadiusSquared = safeRadius*safeRadius;
	int i;

	if (!ent)
	{
		return false;
	}

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		safeMins[i] = ent->currentOrigin[i] - safeRadius;
		safeMaxs[i] = ent->currentOrigin[i] + safeRadius;
	}

	//Get a number of entities in a given space
	numEnts = gi.EntitiesInBox( safeMins, safeMaxs, radiusEnts, MAX_SAFESPAWN_ENTS );

	for ( i = 0; i < numEnts; i++ )
	{
		//Don't consider self
		if ( radiusEnts[i] == ent )
			continue;

		if (radiusEnts[i]->NPC && (radiusEnts[i]->health == 0))
		{
			// ignore dead guys
			continue;
		}

		distance = DistanceSquared( ent->currentOrigin, radiusEnts[i]->currentOrigin );

		//Found one too close to us
		if ( distance < safeRadiusSquared )
		{
			return false;
		}
	}
	return true;
}


/*
-------------------------
NPC_Spawn_Go
-------------------------
*/

gentity_t *NPC_Spawn_Do( gentity_t *ent, qboolean fullSpawnNow )
{
	gentity_t	*newent;
	int			index;
	vec3_t		saveOrg;

/*	//Do extra code for stasis spawners
	if ( Q_stricmp( ent->classname, "NPC_Stasis" ) == 0 )
	{
		if ( NPC_StasisSpawn_Go( ent ) == qfalse )
			return;
	}
*/

	// 4/18/03 kef -- don't let guys spawn into other guys
	if (ent->spawnflags & 4096)
	{
		if (!NPC_SafeSpawn(ent, 64))
		{
			return NULL;
		}
	}

	//Test for drop to floor
	if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
	{
		trace_t		tr;
		vec3_t		bottom;

		VectorCopy( ent->currentOrigin, saveOrg );
		VectorCopy( ent->currentOrigin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, bottom, ent->s.number, MASK_NPCSOLID, (EG2_Collision)0, 0 );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			G_SetOrigin( ent, tr.endpos );
		}
	}

	//Check the spawner's count
	if( ent->count != -1 )
	{
		ent->count--;
		
		if( ent->count <= 0 )
		{
			ent->e_UseFunc = useF_NULL;//never again
			//will be removed below
		}
	}

	newent = G_Spawn();

	if ( newent == NULL ) 
	{
		gi.Printf ( S_COLOR_RED"ERROR: NPC G_Spawn failed\n" );
		
		if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
		{
			G_SetOrigin( ent, saveOrg );
		}
		return NULL;
	}

	newent->client = (gclient_s *)gi.Malloc(sizeof(gclient_s), TAG_G_ALLOC, qtrue);
	
	newent->svFlags |= SVF_NPC;

	if ( ent->NPC_type == NULL ) 
	{
		ent->NPC_type = "random";
		newent->NPC_type = "random";
	}
	else
	{
		newent->NPC_type = Q_strlwr( G_NewString( ent->NPC_type ) );	//get my own copy so i can free it when i die
	}

	newent->NPC = (gNPC_t*) gi.Malloc(sizeof(gNPC_t), TAG_G_ALLOC, qtrue);

	newent->NPC->tempGoal = G_Spawn();
	
	newent->NPC->tempGoal->classname = "NPC_goal";
	newent->NPC->tempGoal->owner = newent;
	newent->NPC->tempGoal->svFlags |= SVF_NOCLIENT;

//==NPC_Connect( newent, net_name );===================================

	if ( ent->svFlags & SVF_NO_BASIC_SOUNDS )
	{
		newent->svFlags |= SVF_NO_BASIC_SOUNDS;
	}
	if ( ent->svFlags & SVF_NO_COMBAT_SOUNDS )
	{
		newent->svFlags |= SVF_NO_COMBAT_SOUNDS;
	}
	if ( ent->svFlags & SVF_NO_EXTRA_SOUNDS )
	{
		newent->svFlags |= SVF_NO_EXTRA_SOUNDS;
	}
	
	if ( ent->message )
	{//has a key
		newent->message = G_NewString(ent->message);//copy the key name
		newent->flags |= FL_NO_KNOCKBACK;//don't fall off ledges
	}

	// If this is a vehicle we need to see what kind it is so we properlly allocate it.
	if ( Q_stricmp( ent->classname, "NPC_Vehicle" ) == 0 )
	{
		// Get the vehicle entry index.
		int iVehIndex = BG_VehicleGetIndex( newent->NPC_type );

		if ( iVehIndex == -1 )
		{
			Com_Printf( S_COLOR_RED"ERROR: Attempting to Spawn an unrecognized Vehicle! - %s\n", newent->NPC_type );
			G_FreeEntity( newent );
			if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
			{
				G_SetOrigin( ent, saveOrg );
			}
            return NULL;
		}
		newent->soundSet = G_NewString(ent->soundSet);//get my own copy so i can free it when i die

		// NOTE: If you change/add any of these, update NPC_Spawn_f for the new vehicle you
		// want to be able to spawn in manually.

		// See what kind of vehicle this is and allocate it properly.
		switch( g_vehicleInfo[iVehIndex].type )
		{
			case VH_ANIMAL:
				// Create the animal (making sure all it's data is initialized).
				G_CreateAnimalNPC( &newent->m_pVehicle, newent->NPC_type );
				break;

			case VH_SPEEDER:
				// Create the speeder (making sure all it's data is initialized).
				G_CreateSpeederNPC( &newent->m_pVehicle, newent->NPC_type );
				break;

			case VH_FIGHTER:
				// Create the fighter (making sure all it's data is initialized).
				G_CreateFighterNPC( &newent->m_pVehicle, newent->NPC_type );
				break;
			case VH_WALKER:
				// Create the animal (making sure all it's data is initialized).
				G_CreateWalkerNPC( &newent->m_pVehicle, newent->NPC_type );
				break;

			default:
				Com_Printf( S_COLOR_RED"ERROR: Attempting to Spawn an unrecognized Vehicle Type! - %s\n", newent->NPC_type );
				G_FreeEntity( newent );
				if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
				{
					G_SetOrigin( ent, saveOrg );
				}
				return NULL;
		}

		//grab this from the spawner
		if ( (ent->spawnflags&1) )
		{//wants to explode when not in vis of player
			newent->endFrame = ent->endFrame;
		}

		// Setup the vehicle.
		newent->m_pVehicle->m_pParentEntity = newent;
		newent->m_pVehicle->m_pVehicleInfo->Initialize( newent->m_pVehicle );
		newent->client->NPC_class = CLASS_VEHICLE;
		if ( g_vehicleInfo[iVehIndex].type == VH_FIGHTER )
		{//FIXME: EXTERN!!!
			newent->flags |= (FL_NO_KNOCKBACK|FL_SHIELDED);//don't get pushed around, blasters bounce off
		}
		//WTF?!!! Ships spawning in pointing straight down!
		//set them up to start landed
		newent->m_pVehicle->m_vOrientation[YAW] = ent->s.angles[YAW];
		newent->m_pVehicle->m_vOrientation[PITCH] = newent->m_pVehicle->m_vOrientation[ROLL] = 0.0f;
		G_SetAngles( newent, newent->m_pVehicle->m_vOrientation );
		SetClientViewAngle( newent, newent->m_pVehicle->m_vOrientation );
		
		//newent->m_pVehicle->m_ulFlags |= VEH_GEARSOPEN;
		//why? this would just make it so the initial anim never got played... -rww
		//There was no initial anim, it would just open the gear even though it's already on the ground (fixed now, made an initial anim)
		
		//For SUSPEND spawnflag, the amount of time to drop like a rock after SUSPEND turns off
		newent->fly_sound_debounce_time = ent->fly_sound_debounce_time;
		
		//for no-pilot-death delay
		newent->damage = ent->damage;
		
		//no-pilot-death distance
		newent->speed = ent->speed;
		
		newent->model2 = ent->model2;//for droidNPC
	}
	else
	{
		newent->client->ps.weapon = WP_NONE;//init for later check in NPC_Begin
	}

	newent->classname = "NPC";
	VectorCopy(ent->s.origin, newent->s.origin);
	VectorCopy(ent->s.origin, newent->client->ps.origin);
	VectorCopy(ent->s.origin, newent->currentOrigin);
	G_SetOrigin(newent, ent->s.origin);//just to be sure!
	//NOTE: on vehicles, anything in the .npc file will STOMP data on the NPC that's set by the vehicle
	if ( !NPC_ParseParms( ent->NPC_type, newent ) )
	{
		gi.Printf ( S_COLOR_RED "ERROR: Couldn't spawn NPC %s\n", ent->NPC_type );
		G_FreeEntity( newent );
		if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
		{
			G_SetOrigin( ent, saveOrg );
		}
		return NULL;
	}

	if ( ent->NPC_type )
	{
		if ( !Q_stricmp( ent->NPC_type, "player" ) )
		{// Or check NPC_type against player's NPC_type?
			newent->NPC->aiFlags |= NPCAI_MATCHPLAYERWEAPON;
		}
		else if ( !Q_stricmp( ent->NPC_type, "test" ) )
		{
			int	n;
			for ( n = 0; n < 1 ; n++) 
			{
				if ( !(g_entities[n].svFlags & SVF_NPC) && g_entities[n].client) 
				{
					VectorCopy(g_entities[n].s.origin, newent->s.origin);
					newent->client->playerTeam = g_entities[n].client->playerTeam;
					break;
				}
			}
			newent->NPC->defaultBehavior = newent->NPC->behaviorState = BS_WAIT;
	//		newent->svFlags |= SVF_NOPUSH;
		}
	}
//=====================================================================
	//set the info we want
	newent->health = ent->health;
	newent->wait = ent->wait;

	//copy strings so we can safely free them
	newent->script_targetname = G_NewString(ent->NPC_targetname);
	newent->targetname = G_NewString(ent->NPC_targetname);
	newent->target = G_NewString(ent->NPC_target);//death
	newent->target2 = G_NewString(ent->target2);//knocked out death
	newent->target3 = G_NewString(ent->target3);//???
	newent->target4 = G_NewString(ent->target4);//ffire death
	newent->paintarget = G_NewString(ent->paintarget);
	newent->opentarget = G_NewString(ent->opentarget);
	newent->radius = ent->radius;

	newent->NPC->eventualGoal = ent->enemy;
	
	for( index = BSET_FIRST; index < NUM_BSETS; index++)
	{
		if ( ent->behaviorSet[index] )
		{
			newent->behaviorSet[index] = ent->behaviorSet[index];
		}
	}
	
	VectorCopy(ent->s.angles, newent->s.angles);
	VectorCopy(ent->s.angles, newent->currentAngles);
	VectorCopy(ent->s.angles, newent->client->ps.viewangles);
	newent->NPC->desiredYaw =ent->s.angles[YAW];
	
	//gi.linkentity(newent);  //check: don't need this here?
	newent->spawnflags = ent->spawnflags;

//==New stuff=====================================================================
	newent->s.eType	= ET_PLAYER;
	
	//FIXME: Call CopyParms
	if ( ent->parms )
	{
		for ( int parmNum = 0; parmNum < MAX_PARMS; parmNum++ )
		{
			if ( ent->parms->parm[parmNum] && ent->parms->parm[parmNum][0] )
			{
				Q3_SetParm( newent->s.number, parmNum, ent->parms->parm[parmNum] );
			}
		}
	}
	//FIXME: copy cameraGroup, store mine in message or other string field

	//set origin
	newent->s.pos.trType = TR_INTERPOLATE;
	newent->s.pos.trTime = level.time;
	VectorCopy( newent->currentOrigin, newent->s.pos.trBase );
	VectorClear( newent->s.pos.trDelta );
	newent->s.pos.trDuration = 0;
	//set angles
	newent->s.apos.trType = TR_INTERPOLATE;
	newent->s.apos.trTime = level.time;
	VectorCopy( newent->currentOrigin, newent->s.apos.trBase );
	VectorClear( newent->s.apos.trDelta );
	newent->s.apos.trDuration = 0;
	
	newent->NPC->combatPoint = -1;
	newent->NPC->aiFlags |= ent->bounceCount;//ai flags


	newent->flags |= FL_NOTARGET;//So he's ignored until he's fully spawned
	newent->s.eFlags |= EF_NODRAW;//So he's ignored until he's fully spawned

	if ( fullSpawnNow )
	{
		newent->owner = ent->owner;
	}
	else
	{
		newent->e_ThinkFunc = thinkF_NPC_Begin;
		newent->nextthink = level.time + FRAMETIME;
	}
	NPC_DefaultScriptFlags( newent );

	gi.linkentity (newent);

	if(ent->e_UseFunc == useF_NULL)
	{
		if( ent->target )
		{//use any target we're pointed at
			G_UseTargets ( ent, ent );
		}
		if(ent->closetarget)
		{//last guy should fire this target when he dies
			if (newent->target)
			{//zap
				gi.Free(newent->target);
			}
			newent->target = G_NewString(ent->closetarget);
		}
		G_FreeEntity( ent );//bye!
	}
	else if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
	{
		G_SetOrigin( ent, saveOrg );
	}
	if ( fullSpawnNow )
	{
		NPC_Begin( newent );
	}
	return newent;
}

void NPC_Spawn_Go( gentity_t *ent )
{
	NPC_Spawn_Do( ent, qfalse );
}

/*
-------------------------
NPC_StasisSpawnEffect
-------------------------
*/
/*
void NPC_StasisSpawnEffect( gentity_t *ent )
{
	vec3_t		start, end, forward;
	qboolean	taper;

	//Floor or wall?
	if ( ent->spawnflags & 1 )
	{
		AngleVectors( ent->s.angles, forward, NULL, NULL );
		VectorMA( ent->currentOrigin,  24, forward, end );
		VectorMA( ent->currentOrigin, -20, forward, start );
		
		start[2] += 64;
		
		taper = qtrue;
	}
	else
	{
		VectorCopy( ent->currentOrigin, start );
		VectorCopy( start, end );
		end[2] += 48;
		taper = qfalse;
	}

	//Add the effect
//	CG_ShimmeryThing_Spawner( start, end, 32, qtrue, 1000 );
}
*/
/*
-------------------------
NPC_ShySpawn
-------------------------
*/

#define SHY_THINK_TIME			1000
#define SHY_SPAWN_DISTANCE		128
#define SHY_SPAWN_DISTANCE_SQR	( SHY_SPAWN_DISTANCE * SHY_SPAWN_DISTANCE )

void NPC_ShySpawn( gentity_t *ent )
{
	ent->nextthink = level.time + SHY_THINK_TIME;
	ent->e_ThinkFunc = thinkF_NPC_ShySpawn;

	if ( DistanceSquared( g_entities[0].currentOrigin, ent->currentOrigin ) <= SHY_SPAWN_DISTANCE_SQR )
		return;

	if ( (InFOV( ent, &g_entities[0], 80, 64 )) ) // FIXME: hardcoded fov
		if ( (NPC_ClearLOS( &g_entities[0], ent->currentOrigin )) )
			return;

	// 4/18/03 kef -- don't let guys spawn into other guys
	if (ent->spawnflags & 4096)
	{
		if (!NPC_SafeSpawn(ent, 64))
		{
			return;
		}
	}

	ent->e_ThinkFunc = thinkF_NULL;
	ent->nextthink = 0;

	NPC_Spawn_Go( ent );
}

/*
-------------------------
NPC_Spawn
-------------------------
*/

void NPC_Spawn ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	//delay before spawning NPC
	if (other->spawnflags&32)
	{
		ent->enemy = activator;
	}
	if( ent->delay )
	{
/*		//Stasis does an extra step
		if ( Q_stricmp( ent->classname, "NPC_Stasis" ) == 0 )
		{
			if ( NPC_StasisSpawn_Go( ent ) == qfalse )
				return;
		}
*/
		if ( ent->spawnflags & 2048 )  // SHY
			ent->e_ThinkFunc = thinkF_NPC_ShySpawn;
		else
			ent->e_ThinkFunc = thinkF_NPC_Spawn_Go;

		ent->nextthink = level.time + ent->delay;
	}
	else
	{
		if ( ent->spawnflags & 2048 )  // SHY
			NPC_ShySpawn( ent );
		else
			NPC_Spawn_Go( ent );
	}
}

/*QUAKED NPC_spawner (1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY SAFE

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
SAFE - Won't spawn if an entity is within 64 units

NPC_type - name of NPC (in npcs.cfg) to spawn in

targetname - name this NPC goes by for targetting
target - NPC will fire this when it spawns it's last NPC (should this be when the last NPC it spawned dies?)
target2 - Fired by stasis spawners when they try to spawn while their spawner model is broken
target3 - Fired by spawner if they try to spawn and are blocked and have a wait < 0 (removes them)

If targeted, will only spawn a NPC when triggered
count - how many NPCs to spawn (only if targetted) default = 1
delay - how long to wait to spawn after used
wait - if trying to spawn and blocked, how many seconds to wait before trying again (default = 0.5, < 0 = never try again and fire target2)

NPC_targetname - NPC's targetname AND script_targetname
NPC_target - NPC's target to fire when killed
NPC_target2 - NPC's target to fire when knocked out
NPC_target4 - NPC's target to fire when killed by friendly fire
NPC_type - type of NPC ("Borg" (default), "Xian", etc)
health - starting health (default = 100)

"noBasicSounds" - set to 1 to prevent loading and usage of basic sounds (pain, death, etc)
"noCombatSounds" - set to 1 to prevent loading and usage of combat sounds (anger, victory, etc.)
"noExtraSounds" - set to 1 to prevent loading and usage of "extra" sounds (chasing the enemy - detecting them, flanking them... also jedi combat sounds)

spawnscript - default script to run once spawned (none by default)
usescript - default script to run when used (none by default)
awakescript - default script to run once awoken (none by default)
angerscript - default script to run once angered (none by default)
painscript - default script to run when hit (none by default)
fleescript - default script to run when hit and below 50% health (none by default)
deathscript - default script to run when killed (none by default)
These strings can be used to activate behaviors instead of scripts - these are checked
first and so no scripts should be names with these names:
    default - 0: whatever
	idle - 1: Stand around, do abolutely nothing
	roam - 2: Roam around, collect stuff
	walk - 3: Crouch-Walk toward their goals
	run - 4: Run toward their goals
	standshoot - 5: Stay in one spot and shoot- duck when neccesary
	standguard - 6: Wait around for an enemy
	patrol - 7: Follow a path, looking for enemies
	huntkill - 8: Track down enemies and kill them
	evade - 9: Run from enemies
	evadeshoot - 10: Run from enemies, shoot them if they hit you
	runshoot - 11: Run to your goal and shoot enemy when possible
	defend - 12: Defend an entity or spot?
	snipe - 13: Stay hidden, shoot enemy only when have perfect shot and back turned
	combat - 14: Attack, evade, use cover, move about, etc.  Full combat AI - id NPC code
	medic - 15: Go for lowest health buddy, hide and heal him.
	takecover - 16: Find nearest cover from enemies
	getammo - 17: Go get some ammo
	advancefight - 18: Go somewhere and fight along the way
	face - 19: turn until facing desired angles
	wait - 20: do nothing
	formation - 21: Maintain a formation
	crouch - 22: Crouch-walk toward their goals

delay - after spawned or triggered, how many seconds to wait to spawn the NPC
*/
extern qboolean	spawning;				// the G_Spawn*() functions are valid  (only turned on during one function)
extern void	NPC_PrecacheByClassName(const char*);

void SP_NPC_spawner( gentity_t *self)
{
	extern void NPC_PrecacheAnimationCFG( const char *NPC_type );
	float	fDelay;

	//register/precache the models needed for this NPC, not anymore
	//self->classname = "NPC_spawner";

	if(!self->count)
	{
		self->count = 1;
	}

	//NOTE: bounceCount is transferred to the spawned NPC's NPC->aiFlags
	self->bounceCount = 0;

	{
		static	int	garbage;
		//Stop loading of certain extra sounds
		if ( G_SpawnInt( "noBasicSounds", "0", &garbage ) )
		{
			self->svFlags |= SVF_NO_BASIC_SOUNDS;
		}
		if ( G_SpawnInt( "noCombatSounds", "0", &garbage ) )
		{
			self->svFlags |= SVF_NO_COMBAT_SOUNDS;
		}
		if ( G_SpawnInt( "noExtraSounds", "0", &garbage ) )
		{
			self->svFlags |= SVF_NO_EXTRA_SOUNDS;
		}
		if ( G_SpawnInt( "nodelay", "0", &garbage ) )
		{
			self->bounceCount |= NPCAI_NO_JEDI_DELAY;
		}
	}


	if ( !self->wait )
	{
		self->wait = 500;
	}
	else
	{
		self->wait *= 1000;//1 = 1 msec, 1000 = 1 sec
	}

	G_SpawnFloat( "delay", "0", &fDelay );
	if ( fDelay )
	{
        self->delay = ceil(1000.0f*fDelay);//1 = 1 msec, 1000 = 1 sec
	}

	if ( self->delay > 0 )
	{
		self->svFlags |= SVF_NPC_PRECACHE;
	}

	//We have to load the animation.cfg now because spawnscripts are going to want to set anims and we need to know their length and if they're valid
	NPC_PrecacheAnimationCFG( self->NPC_type );

	if ( self->targetname )
	{//Wait for triggering
		self->e_UseFunc = useF_NPC_Spawn;
		self->svFlags |= SVF_NPC_PRECACHE;//FIXME: precache my weapons somehow?
		//NPC_PrecacheModels( self->NPC_type );
	}
	else
	{
		//NOTE: auto-spawners never check for shy spawning
		if ( spawning )
		{//in entity spawn stage - map starting up
			self->e_ThinkFunc = thinkF_NPC_Spawn_Go;
			self->nextthink = level.time + START_TIME_REMOVE_ENTS + 50;
		}
		else
		{//else spawn right now
			NPC_Spawn( self, self, self );
		}
	}

	if (!(self->svFlags&SVF_NPC_PRECACHE))
	{
		NPC_PrecacheByClassName(self->NPC_type);
	}

	//FIXME: store cameraGroup somewhere else and apply to spawned NPCs' cameraGroup
	//Or just don't include NPC_spawners in cameraGroupings

	if ( self->message )
	{//may drop a key, precache the key model and pickup sound
		G_SoundIndex( "sound/weapons/key_pkup.wav" );
		if ( !Q_stricmp( "goodie", self->message ) )
		{
			RegisterItem( FindItemForInventory( INV_GOODIE_KEY ) );
		}
		else
		{
			RegisterItem( FindItemForInventory( INV_SECURITY_KEY ) );
		}
	}
}


//=============================================================================================
//VEHICLES
//=============================================================================================
/*QUAKED NPC_Vehicle (1 0 0) (-16 -16 -24) (16 16 32) NO_VIS_DIE x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY SAFE
set NPC_type to vehicle name in vehicles.dat

NO_VIS_DIE - die after certain amount of time of not having a LOS to the Player (default time is 10 seconds, change by setting "noVisTime")
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
SAFE - Won't spawn if an entity is within 64 units

"noVisTime" - how long to wait after spawning/last being seen by the player beforw blowing ourselves up (default is 10 seconds)
"skin" - which skin to set "red" for example - If no skin it is random
*/
void NPC_VehicleSpawnUse( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_VehicleSpawn( self );
}

void SP_NPC_Vehicle( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		self->NPC_type = "swoop";
	}

	if ( !self->classname )
	{
		self->classname = "NPC_Vehicle";
	}

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	G_SpawnString("skin", "", &self->soundSet);

	//grab this from the spawner
	if ( (self->spawnflags&1) )
	{//wants to explode when not in vis of player
		if ( !self->endFrame )
		{
			self->endFrame = NO_PILOT_DIE_TIME;
		}
	}

	if ( self->targetname )
	{
		self->svFlags |= SVF_NPC_PRECACHE;				// Precache The Bike when all other npcs are precached
		self->e_UseFunc = useF_NPC_VehicleSpawnUse;
		//we're not spawning until later, so precache us now
		BG_VehicleGetIndex( self->NPC_type );
	}
	else
	{
		G_VehicleSpawn( self );
	}
}

//=============================================================================================
//CHARACTERS
//=============================================================================================

/*QUAKED NPC_Player (1 0 0) (-16 -16 -24) (16 16 32) x RIFLEMAN PHASER TRICORDER DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Player( gentity_t *self)
{
	self->NPC_type = "Player";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Kyle (1 0 0) (-16 -16 -24) (16 16 32) BOSS x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
BOSS - Uses Boss AI
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Kyle( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "Kyle_boss";
	}
	else
	{
		self->NPC_type = "Kyle";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Lando(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Lando( gentity_t *self)
{
	self->NPC_type = "Lando";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Jan(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Jan( gentity_t *self)
{
	self->NPC_type = "Jan";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Luke(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Luke( gentity_t *self)
{
	self->NPC_type = "Luke";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_MonMothma(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MonMothma( gentity_t *self)
{
	self->NPC_type = "MonMothma";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Rosh_Penin (1 0 0) (-16 -16 -24) (16 16 32) DARKSIDE NOFORCE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
Good Rosh
DARKSIDE - Evil Rosh
NOFORCE - Can't jump, starts with no saber
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rosh_Penin( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "rosh_dark";
	}
	else if ( (self->spawnflags&2) )
	{
		self->NPC_type = "rosh_penin_noforce";
	}
	else
	{
		self->NPC_type = "rosh_penin";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Tavion (1 0 0) (-16 -16 -24) (16 16 32) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tavion( gentity_t *self)
{
	self->NPC_type = "Tavion";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Tavion_New (1 0 0) (-16 -16 -24) (16 16 32) SCEPTER SITH_SWORD x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Has a red lightsaber and force powers, uses her saber style from JK2

SCEPTER - Has a red lightsaber and force powers, Ragnos' Scepter in left hand, uses dual saber style and occasionally attacks with Scepter
SITH_SWORD - Has Ragnos' Sith Sword in right hand and force powers, uses strong style
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tavion_New( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "tavion_scepter";
	}
	else if ( (self->spawnflags&2) )
	{
		self->NPC_type = "tavion_sith_sword";
	}
	else
	{
		self->NPC_type = "tavion_new";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Alora (1 0 0) (-16 -16 -24) (16 16 32) DUAL x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Lightsaber and level 2 force powers, 300 health

DUAL - Dual sabers and level 3 force powers, 500 health
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Alora( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "alora_dual";
	}
	else
	{
		self->NPC_type = "alora";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Reelo(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Reelo( gentity_t *self)
{
	self->NPC_type = "Reelo";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Galak(1 0 0) (-16 -16 -24) (16 16 40) MECH x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
MECH - will be the armored Galak

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Galak( gentity_t *self)
{
}

/*QUAKED NPC_Desann(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Desann( gentity_t *self)
{
	self->NPC_type = "Desann";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Rax(1 0 0) (-16 -16 -24) (16 16 40) FUN x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
FUN - Makes him magically the funnest thing ever in any game ever made. (actually does nothing, it'll just be fun by the power of suggestion).
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rax( gentity_t *self )
{
	self->NPC_type = "Rax";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_BobaFett(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_BobaFett( gentity_t *self )
{
	self->NPC_type = "Boba_Fett";
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Ragnos(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Ragnos( gentity_t *self )
{
	self->NPC_type = "Ragnos";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Lannik_Racto(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Lannik_Racto( gentity_t *self )
{
	self->NPC_type = "lannik_racto";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Kothos(1 0 0) (-16 -16 -24) (16 16 40) VIL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
VIL - spawns Vil Kothos instead of Dasariah (can anyone tell them apart anyway...?)

Force only... will (eventually) be set up to re-inforce their leader (use SET_LEADER) by healing them, recharging them, keeping the player away, etc.

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Kothos( gentity_t *self )
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "VKothos";
	}
	else
	{
		self->NPC_type = "DKothos";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Chewbacca(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Chewbacca( gentity_t *self )
{
	self->NPC_type = "Chewie";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Bartender(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Bartender( gentity_t *self)
{
	self->NPC_type = "Bartender";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_MorganKatarn(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MorganKatarn( gentity_t *self)
{
	self->NPC_type = "MorganKatarn";

	SP_NPC_spawner( self );
}

//=============================================================================================
//ALLIES
//=============================================================================================

/*QUAKED NPC_Jedi(1 0 0) (-16 -16 -24) (16 16 40) TRAINER MASTER RANDOM x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
TRAINER - Special Jedi- instructor
MASTER - Special Jedi- master
RANDOM - creates a random Jedi student using the available player models/skins (excludes the current model of the player)
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Ally Jedi NPC Buddy - tags along with player
*/
extern cvar_t	*g_char_model;
void SP_NPC_Jedi( gentity_t *self)
{
	if(!self->NPC_type)
	{
		if ( self->spawnflags & 4 )
		{//random!
			int sanityCheck = 20;	//just in case
			while ( sanityCheck-- )
			{
				switch( Q_irand( 0, 11 ) )
				{
				case 0:
					self->NPC_type = "jedi_hf1";
					break;
				case 1:
					self->NPC_type = "jedi_hf2";
					break;
				case 2:
					self->NPC_type = "jedi_hm1";
					break;
				case 3:
					self->NPC_type = "jedi_hm2";
					break;
				case 4:
					self->NPC_type = "jedi_kdm1";
					break;
				case 5:
					self->NPC_type = "jedi_kdm2";
					break;
				case 6:
					self->NPC_type = "jedi_rm1";
					break;
				case 7:
					self->NPC_type = "jedi_rm2";
					break;
				case 8:
					self->NPC_type = "jedi_tf1";
					break;
				case 9:
					self->NPC_type = "jedi_tf2";
					break;
				case 10:
					self->NPC_type = "jedi_zf1";
					break;
				case 11:
				default://just in case
					self->NPC_type = "jedi_zf2";
					break;
				}
				if ( strstr( self->NPC_type, g_char_model->string ) != NULL )
				{//bah, we're using this one, try again
					continue;
				}
				break;	//get out of the while
			}
		}
		else if ( self->spawnflags & 2 )
		{
			self->NPC_type = "jedimaster";
		}
		else if ( self->spawnflags & 1 )
		{
			self->NPC_type = "jeditrainer";
		}
		else 
		{
			/*
			if ( !Q_irand( 0, 2 ) )
			{
				self->NPC_type = "JediF";
			}
			else 
			*/if ( Q_irand( 0, 1 ) )
			{
				self->NPC_type = "Jedi";
			}
			else
			{
				self->NPC_type = "Jedi2";
			}
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Prisoner(1 0 0) (-16 -16 -24) (16 16 40) ELDER x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Prisoner( gentity_t *self)
{
	if(!self->NPC_type)
	{
		if ( (self->spawnflags&1) )
		{
			if ( Q_irand( 0, 1 ) )
			{
				self->NPC_type = "elder";
			}
			else
			{
				self->NPC_type = "elder2";
			}
		}
		else
		{
			if ( Q_irand( 0, 1 ) )
			{
				self->NPC_type = "Prisoner";
			}
			else
			{
				self->NPC_type = "Prisoner2";
			}
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Merchant(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Merchant( gentity_t *self)
{
	self->NPC_type = "merchant";

	SP_NPC_spawner( self );
}
/*QUAKED NPC_Rebel(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rebel( gentity_t *self)
{
	if(!self->NPC_type)
	{
		self->NPC_type = "Rebel";
	}

	SP_NPC_spawner( self );
}

//=============================================================================================
//ENEMIES
//=============================================================================================

/*QUAKED NPC_Human_Merc(1 0 0) (-16 -16 -24) (16 16 40) BOWCASTER REPEATER FLECHETTE CONCUSSION DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
100 health, blaster rifle

BOWCASTER - Starts with a Bowcaster
REPEATER - Starts with a Repeater
FLECHETTE - Starts with a Flechette gun
CONCUSSION - Starts with a Concussion Rifle

If you want them to start with any other kind of weapon, make a spawnscript for them that sets their weapon.

"message" - turns on his key surface.  This is the name of the key you get when you walk over his body.  This must match the "message" field of the func_security_panel you want this key to open.  Set to "goodie" to have him carrying a goodie key that player can use to operate doors with "GOODIE" spawnflag.  NOTE: this overrides all the weapon spawnflags

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Human_Merc( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->message )
		{
			self->NPC_type = "human_merc_key";
		}
		else if ( (self->spawnflags&1) )
		{
			self->NPC_type = "human_merc_bow";
		}
		else if ( (self->spawnflags&2) )
		{
			self->NPC_type = "human_merc_rep";
		}
		else if ( (self->spawnflags&4) )
		{
			self->NPC_type = "human_merc_flc";
		}
		else if ( (self->spawnflags&8) )
		{
			self->NPC_type = "human_merc_cnc";
		}
		else
		{
			self->NPC_type = "human_merc";
		}
	}
	SP_NPC_spawner( self );
}

//TROOPERS=============================================================================

/*QUAKED NPC_Stormtrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER COMMANDER ALTOFFICER ROCKET DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY COMMANDO
30 health, blaster

OFFICER - 60 health, flechette
COMMANDER - 60 health, heavy repeater
ALTOFFICER - 60 health, alt-fire flechette (grenades)
ROCKET - 60 health, rocket launcher

COMMANDO - Causes character to use Hazard Trooper (move in formation AI)

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Stormtrooper( gentity_t *self)
{
	if ( self->spawnflags & 8 )
	{//rocketer
		self->NPC_type = "rockettrooper";
	}
	else if ( self->spawnflags & 4 )
	{//alt-officer
		self->NPC_type = "stofficeralt";
	}
	else if ( self->spawnflags & 2 )
	{//commander
		self->NPC_type = "stcommander";
	}
	else if ( self->spawnflags & 1 )
	{//officer
		self->NPC_type = "stofficer";
	}
	else
	{//regular trooper
		if ( Q_irand( 0, 1 ) )
		{
			self->NPC_type = "StormTrooper";
		}
		else
		{
			self->NPC_type = "StormTrooper2";
		}
	}

	SP_NPC_spawner( self );
}
void SP_NPC_StormtrooperOfficer( gentity_t *self)
{
	self->spawnflags |= 1;
	SP_NPC_Stormtrooper( self );
}
/*QUAKED NPC_Snowtrooper(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Snowtrooper( gentity_t *self)
{
	self->NPC_type = "snowtrooper";

	SP_NPC_spawner( self );
}
/*QUAKED NPC_Tie_Pilot(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tie_Pilot( gentity_t *self)
{
	self->NPC_type = "stormpilot";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_RocketTrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER SPOTLIGHT x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
200 health, flies, rockets

OFFICER - starts flying, uses concussion rifle instead of rockets
SPOTLIGHT - uses a shoulder-mounted spotlight

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_RocketTrooper( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "rockettrooper2Officer";
		}
		else
		{
			self->NPC_type = "rockettrooper2";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_HazardTrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER CONCUSSION x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
250 health, repeater

OFFICER - 400 health, flechette
CONCUSSION - 400 health, concussion rifle
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_HazardTrooper( gentity_t *self)
{
	if ( !self->NPC_type )
	{


		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "hazardtrooperofficer";
		}
		else if ( (self->spawnflags&2) )
		{
			self->NPC_type = "hazardtrooperconcussion";
		}
		else
		{
			self->NPC_type = "hazardtrooper";
		}
	}

	SP_NPC_spawner( self );
}

//OTHERS=============================================================================

/*QUAKED NPC_Ugnaught(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Ugnaught( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( Q_irand( 0, 1 ) )
		{
			self->NPC_type = "Ugnaught";
		}
		else
		{
			self->NPC_type = "Ugnaught2";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Jawa(1 0 0) (-16 -16 -24) (16 16 40) ARMED x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
ARMED - starts with the Jawa gun in-hand

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Jawa( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "jawa_armed";
		}
		else
		{
			self->NPC_type = "jawa";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Gran(1 0 0) (-16 -16 -24) (16 16 40) SHOOTER BOXER x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
Uses grenade

SHOOTER - uses blaster instead of 
BOXER - uses fists only
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Gran( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->spawnflags & 1 )
		{
			self->NPC_type = "granshooter";
		}
		else if ( self->spawnflags & 2 )
		{
			self->NPC_type = "granboxer";
		}
		else
		{
			if ( Q_irand( 0, 1 ) )
			{
				self->NPC_type = "gran";
			}
			else
			{
				self->NPC_type = "gran2";
			}
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Rodian(1 0 0) (-16 -16 -24) (16 16 40) BLASTER NO_HIDE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
BLASTER uses a blaster instead of sniper rifle, different skin
NO_HIDE (only applicable with snipers) does not duck and hide between shots
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Rodian( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->spawnflags&1 )
		{
			self->NPC_type = "rodian2";
		}
		else
		{
			self->NPC_type = "rodian";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Weequay(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Weequay( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		switch ( Q_irand( 0, 3 ) )
		{
		case 0:
			self->NPC_type = "Weequay";
			break;
		case 1:
			self->NPC_type = "Weequay2";
			break;
		case 2:
			self->NPC_type = "Weequay3";
			break;
		case 3:
			self->NPC_type = "Weequay4";
			break;
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Trandoshan(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Trandoshan( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		self->NPC_type = "Trandoshan";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Tusken(1 0 0) (-16 -16 -24) (16 16 40) SNIPER x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Tusken( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "tuskensniper";
		}
		else
		{
			self->NPC_type = "tusken";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Noghri(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Noghri( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		self->NPC_type = "noghri";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_SwampTrooper(1 0 0) (-16 -16 -24) (16 16 40) REPEATER x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
REPEATER - Swaptrooper who uses the repeater
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_SwampTrooper( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->spawnflags & 1 )
		{
			self->NPC_type = "SwampTrooper2";
		}
		else
		{
			self->NPC_type = "SwampTrooper";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Imperial(1 0 0) (-16 -16 -24) (16 16 40) OFFICER COMMANDER x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY

Greyshirt grunt, uses blaster pistol, 20 health.

OFFICER - Brownshirt Officer, uses blaster rifle, 40 health
COMMANDER - Blackshirt Commander, uses rapid-fire blaster rifle, 80 healt

"message" - if a COMMANDER, turns on his key surface.  This is the name of the key you get when you walk over his body.  This must match the "message" field of the func_security_panel you want this key to open.  Set to "goodie" to have him carrying a goodie key that player can use to operate doors with "GOODIE" spawnflag.

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Imperial( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->spawnflags & 1 )
		{
			self->NPC_type = "ImpOfficer";
		}
		else if ( self->spawnflags & 2 )
		{
			self->NPC_type = "ImpCommander";
		}
		else
		{
			self->NPC_type = "Imperial";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_ImpWorker(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_ImpWorker( gentity_t *self)
{
	self->NPC_type = "ImpWorker";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_BespinCop(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_BespinCop( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( !Q_irand( 0, 1 ) )
		{
			self->NPC_type = "BespinCop";
		}
		else
		{
			self->NPC_type = "BespinCop2";
		}
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Reborn(1 0 0) (-16 -16 -24) (16 16 40) FORCE FENCER ACROBAT BOSS CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Default Reborn is A poor lightsaber fighter, acrobatic and uses no force powers.  40 health.

FORCE - Uses force powers but is not the best lightsaber fighter and not acrobatic.  75 health.
FENCER - A good lightsaber fighter, but not acrobatic and uses no force powers.  100 health.
ACROBAT - quite acrobatic, but not the best lightsaber fighter and uses no force powers.  100 health.
BOSS - quite acrobatic, good lightsaber fighter and uses force powers.  150 health.

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Reborn not stand around and taunt the player before attacking
*/
void SP_NPC_Reborn( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( self->spawnflags & 1 )
		{
			self->NPC_type = "rebornforceuser";
		}
		else if ( self->spawnflags & 2 )
		{
			self->NPC_type = "rebornfencer";
		}
		else if ( self->spawnflags & 4 )
		{
			self->NPC_type = "rebornacrobat";
		}
		else if ( self->spawnflags & 8 )
		{
			self->NPC_type = "rebornboss";
		}
		else
		{
			self->NPC_type = "reborn";
		}
	}
	
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Reborn_New(1 0 0) (-16 -16 -24) (16 16 40) DUAL STAFF WEAK MASTER CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Reborn is an excellent lightsaber fighter, acrobatic and uses force powers.  Full-length red saber, 200 health.

DUAL - Use 2 shorter sabers
STAFF - Uses a saber staff
WEAK - Is a bit less tough
MASTER - Is SUPER tough
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Reborn not stand around and taunt the player before attacking
*/
void SP_NPC_Reborn_New( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&8) )
		{//tougher guys
			if ( (self->spawnflags&1) )
			{
				self->NPC_type = "RebornMasterDual";
			}
			else if ( (self->spawnflags&2) )
			{
				self->NPC_type = "RebornMasterStaff";
			}
			else
			{
				self->NPC_type = "RebornMaster";
			}
		}
		else if ( (self->spawnflags&4) )
		{//weaker guys
			if ( (self->spawnflags&1) )
			{
				self->NPC_type = "reborn_dual2";
			}
			else if ( (self->spawnflags&2) )
			{
				self->NPC_type = "reborn_staff2";
			}
			else
			{
				self->NPC_type = "reborn_new2";
			}
		}
		else
		{
			if ( (self->spawnflags&1) )
			{
				self->NPC_type = "reborn_dual";
			}
			else if ( (self->spawnflags&2) )
			{
				self->NPC_type = "reborn_staff";
			}
			else
			{
				self->NPC_type = "reborn_new";
			}
		}
	}
	
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Cultist_Saber(1 0 0) (-16 -16 -24) (16 16 40) MED STRONG ALL THROW CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Uses a saber and no force powers.  100 health.

default fencer uses fast style - weak, but can attack rapidly.  Good defense.

MED - Uses medium style - average speed and attack strength, average defense.
STRONG - Uses strong style, slower than others, but can do a lot of damage with one blow.  Weak defense.
ALL - Knows all 3 styles, switches between them, good defense.
THROW - can throw their saber (level 2) - reduces their defense some (can use this spawnflag alone or in combination with any *one* of the previous 3 spawnflags)

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Cultist not stand around and taunt the player before attacking
*/
void SP_NPC_Cultist_Saber( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_med_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_med";
			}
		}
		else if ( (self->spawnflags&2) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_strong_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_strong";
			}
		}
		else if ( (self->spawnflags&2) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_all_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber_all";
			}
		}
		else
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber";
			}
		}
	}
	
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Cultist_Saber_Powers(1 0 0) (-16 -16 -24) (16 16 40) MED STRONG ALL THROW CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
Uses a saber and has a couple low-level powers.  150 health.

default fencer uses fast style - weak, but can attack rapidly.  Good defense.

MED - Uses medium style - average speed and attack strength, average defense.
STRONG - Uses strong style, slower than others, but can do a lot of damage with one blow.  Weak defense.
ALL - Knows all 3 styles, switches between them, good defense.
THROW - can throw their saber (level 2) - reduces their defense some (can use this spawnflag alone or in combination with any *one* of the previous 3 spawnflags)

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Cultist not stand around and taunt the player before attacking
*/
void SP_NPC_Cultist_Saber_Powers( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_med_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_med2";
			}
		}
		else if ( (self->spawnflags&2) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_strong_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_strong2";
			}
		}
		else if ( (self->spawnflags&2) )
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_all_throw2";
			}
			else
			{
				self->NPC_type = "cultist_saber_all2";
			}
		}
		else
		{
			if ( (self->spawnflags&8) )
			{
				self->NPC_type = "cultist_saber_throw";
			}
			else
			{
				self->NPC_type = "cultist_saber2";
			}
		}
	}
	
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Cultist(1 0 0) (-16 -16 -24) (16 16 40) SABER GRIP LIGHTNING DRAIN CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist uses a blaster and force powers.  40 health.

SABER - Uses a saber and no force powers
GRIP - Uses no weapon and grip, push and pull
LIGHTNING - Uses no weapon and lightning and push
DRAIN - Uses no weapons and drain and push

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Cultist not stand around and taunt the player before attacking
*/
void SP_NPC_Cultist( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = NULL;
			self->spawnflags = 0;//fast, no throw
			switch ( Q_irand( 0, 2 ) )
			{
			case 0://medium
				self->spawnflags |= 1;
				break;
			case 1://strong
				self->spawnflags |= 2;
				break;
			case 2://all
				self->spawnflags |= 4;
				break;
			}
			if ( Q_irand( 0, 1 ) )
			{//throw
				self->spawnflags |= 8;
			}
			SP_NPC_Cultist_Saber( self );
			return;
		}
		else if ( (self->spawnflags&2) )
		{
			self->NPC_type = "cultist_grip";
		}
		else if ( (self->spawnflags&4) )
		{
			self->NPC_type = "cultist_lightning";
		}
		else if ( (self->spawnflags&8) )
		{
			self->NPC_type = "cultist_drain";
		}
		else
		{
			self->NPC_type = "cultist";
		}
	}
	
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Cultist_Commando(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist uses dual blaster pistols and force powers.  40 health.

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Cultist not stand around and taunt the player before attacking
*/
void SP_NPC_Cultist_Commando( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		self->NPC_type = "cultistcommando";
	}
	SP_NPC_spawner( self );
}
/*QUAKED NPC_Cultist_Destroyer(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY

Cultist has no weapons, runs up to you chanting & building up a Force Destruction blast - when gets to you, screams & explodes

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Cultist_Destroyer( gentity_t *self)
{
	self->NPC_type = "cultist_destroyer";
	SP_NPC_spawner( self );
}

/*QUAKED NPC_ShadowTrooper(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

nodelay - set to "1" to make the Cultist not stand around and taunt the player before attacking
*/
void SP_NPC_ShadowTrooper( gentity_t *self)
{
	if(!self->NPC_type)
	{
		if ( !Q_irand( 0, 1 ) )
		{
			self->NPC_type = "ShadowTrooper";
		}
		else
		{
			self->NPC_type = "ShadowTrooper2";
		}
	}
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Saboteur(1 0 0) (-16 -16 -24) (16 16 40) SNIPER PISTOL x x CLOAKED CINEMATIC NOTSOLID STARTINSOLID SHY
Has a blaster rifle, can cloak and roll

SNIPER - Has a sniper rifle, no acrobatics, but can dodge
PISTOL - Just has a pistol, can roll
COMMANDO - Has 2 pistols and can roll & dodge

CLOAKED - Starts cloaked
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Saboteur( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "saboteursniper";
		}
		else if ( (self->spawnflags&2) )
		{
			self->NPC_type = "saboteurpistol";
		}
		else if ( (self->spawnflags&4) )
		{
			self->NPC_type = "saboteurcommando";
		}
		else
		{
			self->NPC_type = "saboteur";
		}
	}
	SP_NPC_spawner( self );
}

//=============================================================================================
//MONSTERS
//=============================================================================================

/*QUAKED NPC_Monster_Murjj (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Murjj( gentity_t *self)
{
	self->NPC_type = "Murjj";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Swamp (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Swamp( gentity_t *self)
{
	self->NPC_type = "Swamp";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Howler (1 0 0) (-16 -16 -24) (16 16 8) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Howler( gentity_t *self)
{
	self->NPC_type = "howler";
	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Rancor (1 0 0) (-30 -30 -24) (30 30 136) MUTANT FASTKILL x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
2000 health, picks people up and eats them

MUTANT - Bigger, meaner, nastier, Frencher.  Breath attack, pound attack.
FASTKILL - Kills NPCs faster
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Rancor( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "mutant_rancor";
	}
	else
	{
		self->NPC_type = "rancor";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Mutant_Rancor (1 0 0) (-60 -60 -24) (60 60 360) x FASTKILL x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
Bigger, meaner, nastier, Frencher.  Breath attack, pound attack.

FASTKILL - Kills NPCs faster
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Mutant_Rancor( gentity_t *self)
{
	self->NPC_type = "mutant_rancor";

	SP_NPC_spawner( self );
}
/*QUAKED NPC_Monster_Wampa (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Wampa( gentity_t *self)
{
	self->NPC_type = "wampa";

	SP_NPC_spawner( self );
}
/*QUAKED NPC_MineMonster (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_MineMonster( gentity_t *self)
{
	self->NPC_type = "minemonster";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Claw (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Claw( gentity_t *self)
{
	self->NPC_type = "Claw";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Glider (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Glider( gentity_t *self)
{
	self->NPC_type = "Glider";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Flier2 (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Flier2( gentity_t *self)
{
	self->NPC_type = "Flier2";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Lizard (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Lizard( gentity_t *self)
{
	self->NPC_type = "Lizard";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Fish (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Monster_Fish( gentity_t *self)
{
	self->NPC_type = "Fish";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Monster_Sand_Creature (1 0 0) (-24 -24 -24) (24 24 0) FAST x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

turfrange - if set, they will not go beyond this dist from their spawn position
*/
void SP_NPC_Monster_Sand_Creature( gentity_t *self)
{
	if ( (self->spawnflags&1) )
	{
		self->NPC_type = "sand_creature_fast";
	}
	else
	{
		self->NPC_type = "sand_creature";
	}

	SP_NPC_spawner( self );
}

//=============================================================================================
//DROIDS
//=============================================================================================

/*QUAKED NPC_Droid_Interrogator (1 0 0) (-12 -12 -24) (12 12 0) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Interrogator( gentity_t *self)
{
	self->NPC_type = "interrogator";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Probe (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Imperial Probe Droid - the multilegged floating droid that Han and Chewie shot on the ice planet Hoth
*/
void SP_NPC_Droid_Probe( gentity_t *self)
{
	self->NPC_type = "probe";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Mark1 (1 0 0) (-36 -36 -24) (36 36 80) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Big walking droid

*/
void SP_NPC_Droid_Mark1( gentity_t *self)
{
	self->NPC_type = "mark1";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Mark2 (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Small rolling droid with one gun.

*/
void SP_NPC_Droid_Mark2( gentity_t *self)
{
	self->NPC_type = "mark2";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_ATST (1 0 0) (-40 -40 -24) (40 40 248) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_ATST( gentity_t *self)
{
	self->NPC_type = "atst";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Remote (1 0 0) (-4 -4 -24) (4 4 8) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Remote Droid - the floating round droid used by Obi Wan to train Luke about the force while on the Millenium Falcon.
*/
void SP_NPC_Droid_Remote( gentity_t *self)
{
	self->NPC_type = "remote_sp";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Seeker (1 0 0) (-4 -4 -24) (4 4 8) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Seeker Droid - floating round droids that shadow troopers spawn
*/
void SP_NPC_Droid_Seeker( gentity_t *self)
{
	self->NPC_type = "seeker";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Sentry (1 0 0) (-24 -24 -24) (24 24 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Sentry Droid - Large, armored floating Imperial droids with 3 forward-facing gun turrets
*/
void SP_NPC_Droid_Sentry( gentity_t *self)
{
	self->NPC_type = "sentry";

	SP_NPC_spawner( self );

}

/*QUAKED NPC_Droid_Gonk (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Gonk Droid - the droid that looks like a walking ice machine. Was in the Jawa land crawler, walking around talking to itself.

NOTARGET by default
*/
void SP_NPC_Droid_Gonk( gentity_t *self)
{
	self->NPC_type = "gonk";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Mouse (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Mouse Droid - small, box shaped droid, first seen on the Death Star. Chewie yelled at it and it backed up and ran away.

NOTARGET by default
*/
void SP_NPC_Droid_Mouse( gentity_t *self)
{
	self->NPC_type = "mouse";

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_R2D2 (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

R2D2 Droid - you probably know this one already. 

NOTARGET by default
*/
void SP_NPC_Droid_R2D2( gentity_t *self)
{
	if ( self->spawnflags&1 )
	{//imperial skin
		self->NPC_type = "r2d2_imp";
	}
	else
	{
		self->NPC_type = "r2d2";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_R5D2 (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL ALWAYSDIE x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
ALWAYSDIE - won't go into spinning zombie AI when at low health.
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

R5D2 Droid - the droid originally chosen by Uncle Owen until it blew a bad motivator, and they took R2D2 instead.

NOTARGET by default
*/
void SP_NPC_Droid_R5D2( gentity_t *self)
{
	if ( self->spawnflags&1 )
	{//imperial skin
		self->NPC_type = "r5d2_imp";
	}
	else
	{
		self->NPC_type = "r5d2";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Protocol (1 0 0) (-12 -12 -24) (12 12 40) IMPERIAL x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

NOTARGET by default
*/
void SP_NPC_Droid_Protocol( gentity_t *self)
{
	if ( self->spawnflags&1 )
	{//imperial skin
		self->NPC_type = "protocol_imp";
	}
	else
	{
		self->NPC_type = "protocol";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Assassin (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Assassin( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		self->NPC_type = "assassin_droid";
	}

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Droid_Saber (1 0 0) (-12 -12 -24) (12 12 40) TRAINING x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Droid_Saber( gentity_t *self)
{
	if ( !self->NPC_type )
	{
		if ( (self->spawnflags&1) )
		{
			self->NPC_type = "saber_droid_training";
		}
		else
		{
			self->NPC_type = "saber_droid";
		}
	}

	SP_NPC_spawner( self );
}

//NPC console commands
/*
NPC_Spawn_f
*/

static void NPC_Spawn_f(void) 
{
	gentity_t		*NPCspawner = G_Spawn();
	vec3_t			forward, end;
	trace_t			trace;
	qboolean		isVehicle = qfalse;
	
	if(!NPCspawner)
	{
		gi.Printf( S_COLOR_RED"NPC_Spawn Error: Out of entities!\n" );
		return;
	}

	NPCspawner->e_ThinkFunc = thinkF_G_FreeEntity;
	NPCspawner->nextthink = level.time + FRAMETIME;
	
	char	*npc_type = gi.argv( 2 );
	if (!npc_type || !npc_type[0] )
	{
		gi.Printf( S_COLOR_RED"Error, expected:\n NPC spawn [NPC type (from NCPCs.cfg)]\n" );
		return;
	}

	if ( !Q_stricmp( "vehicle", npc_type ) )
	{//spawning a vehicle
		isVehicle = qtrue;
		npc_type = gi.argv( 3 );
		if (!npc_type || !npc_type[0] )
		{
			gi.Printf( S_COLOR_RED"Error, expected:\n NPC spawn vehicle [NPC type (from NCPCs.cfg)]\n" );
			return;
		}
	}

	//Spawn it at spot of first player
	//FIXME: will gib them!
	AngleVectors(g_entities[0].client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(g_entities[0].currentOrigin, 64, forward, end);
	gi.trace(&trace, g_entities[0].currentOrigin, NULL, NULL, end, 0, MASK_SOLID, (EG2_Collision)0, 0);
	VectorCopy(trace.endpos, end);
	end[2] -= 24;
	gi.trace(&trace, trace.endpos, NULL, NULL, end, 0, MASK_SOLID, (EG2_Collision)0, 0);
	VectorCopy(trace.endpos, end);
	end[2] += 24;
	G_SetOrigin(NPCspawner, end);
	VectorCopy(NPCspawner->currentOrigin, NPCspawner->s.origin);
	//set the yaw so that they face away from player
	NPCspawner->s.angles[1] = g_entities[0].client->ps.viewangles[1];

	gi.linkentity(NPCspawner);

	NPCspawner->NPC_type = Q_strlwr( G_NewString( npc_type ) );
	NPCspawner->NPC_targetname = G_NewString(gi.argv( 3 ));

	NPCspawner->count = 1;

	NPCspawner->delay = 0;

	NPCspawner->wait = 500;

	//NPCspawner->spawnflags |= SFB_NOTSOLID;

	//NPCspawner->playerTeam = TEAM_FREE;
	//NPCspawner->behaviorSet[BSET_SPAWN] = "common/guard";

	if ( isVehicle )
	{//must let NPC spawn func know this is a vehicle we're trying to spawn
		NPCspawner->classname = "NPC_Vehicle";
	}

	NPC_PrecacheByClassName(NPCspawner->NPC_type);
	if ( !Q_stricmp( "kyle_boss", NPCspawner->NPC_type ))
	{//bah
		NPCspawner->spawnflags |= 1;
	}

	if ( !Q_stricmp( "key", NPCspawner->NPC_type ))
	{//bah
		NPCspawner->message = "key";
		NPCspawner->NPC_type = "imperial";
	}
	if ( !Q_stricmp( "jedi_random", NPCspawner->NPC_type ) )
	{//special case, for testing
		NPCspawner->NPC_type = NULL;
		NPCspawner->spawnflags |= 4;
		SP_NPC_Jedi( NPCspawner );
	}
	else if ( isVehicle )
	{
		SP_NPC_Vehicle( NPCspawner );
	}
	else
	{
		NPC_Spawn( NPCspawner, NPCspawner, NPCspawner );
	}
}

/*
NPC_Kill_f
*/

void NPC_Kill_f( void ) 
{
	int			n;
	gentity_t	*player;
	char		*name;
	team_t		killTeam = TEAM_FREE;
	qboolean	killNonSF = qfalse;

	name = gi.argv( 2 );

	if ( !*name || !name[0] )
	{
		gi.Printf( S_COLOR_RED"Error, Expected:\n");
		gi.Printf( S_COLOR_RED"NPC kill '[NPC targetname]' - kills NPCs with certain targetname\n" );
		gi.Printf( S_COLOR_RED"or\n" );
		gi.Printf( S_COLOR_RED"NPC kill 'all' - kills all NPCs\n" );
		gi.Printf( S_COLOR_RED"or\n" );
		gi.Printf( S_COLOR_RED"NPC team '[teamname]' - kills all NPCs of a certain team ('nonally' is all but your allies)\n" );
		return;
	}

	if ( Q_stricmp( "team", name ) == 0 )
	{
		name = gi.argv( 3 );

		if ( !*name || !name[0] )
		{
			gi.Printf( S_COLOR_RED"NPC_Kill Error: 'npc kill team' requires a team name!\n" );
			gi.Printf( S_COLOR_RED"Valid team names are:\n");
			for ( n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
			{
				gi.Printf( S_COLOR_RED"%s\n", GetStringForID( TeamTable, n ) );
			}
			gi.Printf( S_COLOR_RED"nonally - kills all but your teammates\n" );
			return;
		}

		if ( Q_stricmp( "nonally", name ) == 0 )
		{
			killNonSF = qtrue;
		}
		else
		{
			killTeam = (team_t)GetIDForString( TeamTable, name );

			if ( killTeam == (team_t)-1 )
			{
				gi.Printf( S_COLOR_RED"NPC_Kill Error: team '%s' not recognized\n", name );
				gi.Printf( S_COLOR_RED"Valid team names are:\n");
				for ( n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
				{
					gi.Printf( S_COLOR_RED"%s\n", GetStringForID( TeamTable, n ) );
				}
				gi.Printf( S_COLOR_RED"nonally - kills all but your teammates\n" );
				return;
			}
		}
	}

	for ( n = 1; n < ENTITYNUM_MAX_NORMAL; n++) 
	{
		player = &g_entities[n];
		if (!player->inuse) {
			continue;
		}
		if ( killNonSF )
		{
			if ( player )
			{
				if ( player->client )
				{
					if ( player->client->playerTeam != TEAM_PLAYER )
					{
						gi.Printf( S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname );
						/*
						if ( (player->flags&FL_UNDYING) )
						{
							G_Damage( player, NULL, NULL, NULL, NULL, player->health+10000, 0, MOD_UNKNOWN );
						}
						else
						*/
						{
							player->health = 0;
							GEntity_DieFunc(player, player, player, player->max_health, MOD_UNKNOWN);
						}
					}
				}
				else if ( player->NPC_type && player->classname && player->classname[0] && Q_stricmp( "NPC_starfleet", player->classname ) != 0 )
				{//A spawner, remove it
					gi.Printf( S_COLOR_GREEN"Removing NPC spawner %s with NPC named %s\n", player->NPC_type, player->NPC_targetname );
					G_FreeEntity( player );
					//FIXME: G_UseTargets2(player, player, player->NPC_target & player->target);?
				}
			}
		}
		else if ( player && player->NPC && player->client )
		{
			if ( killTeam != TEAM_FREE )
			{
				if ( player->client->playerTeam == killTeam )
				{
					gi.Printf( S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname );
					/*
					if ( (player->flags&FL_UNDYING) )
					{
						G_Damage( player, NULL, NULL, NULL, NULL, player->health+10000, 0, MOD_UNKNOWN );
					}
					else
					*/
					{
						player->health = 0;
						GEntity_DieFunc(player, player, player, player->max_health, MOD_UNKNOWN);
					}
				}
			}
			else if( (player->targetname && Q_stricmp( name, player->targetname ) == 0)
				|| Q_stricmp( name, "all" ) == 0 )
			{
				gi.Printf( S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname );
				player->client->ps.stats[STAT_HEALTH] = 0;
				/*
				if ( (player->flags&FL_UNDYING) )
				{
					G_Damage( player, NULL, NULL, NULL, NULL, player->health+10000, 0, MOD_UNKNOWN );
				}
				else
				*/
				{
					player->health = 0;
					GEntity_DieFunc(player, player, player, 100, MOD_UNKNOWN);
				}
			}
		}
		else if ( player && (player->svFlags&SVF_NPC_PRECACHE) )
		{//a spawner
			if( (player->targetname && Q_stricmp( name, player->targetname ) == 0)
				|| Q_stricmp( name, "all" ) == 0 )
			{
				gi.Printf( S_COLOR_GREEN"Removing NPC spawner %s named %s\n", player->NPC_type, player->targetname );
				G_FreeEntity( player );
			}
		}
	}
}

void NPC_PrintScore( gentity_t *ent )
{
	gi.Printf( "%s: %d\n", ent->targetname, ent->client->ps.persistant[PERS_SCORE] );
}

/*
Svcmd_NPC_f

parse and dispatch bot commands
*/
qboolean	showBBoxes = qfalse;
void Svcmd_NPC_f( void ) 
{
	char	*cmd;

	cmd = gi.argv( 1 );

	if ( !*cmd ) 
	{
		gi.Printf( "Valid NPC commands are:\n" );
		gi.Printf( " spawn [NPC type (from *.npc files)]\n" );
		gi.Printf( " spawn vehicle [NPC type (from *.npc files, only for NPCs that are CLASS_VEHICLE and have a .veh file)]\n" );
		gi.Printf( " kill [NPC targetname] or [all(kills all NPCs)] or 'team [teamname]'\n" );
		gi.Printf( " showbounds (draws exact bounding boxes of NPCs)\n" );
		gi.Printf( " score [NPC targetname] (prints number of kills per NPC)\n" );
	}
	else if ( Q_stricmp( cmd, "spawn" ) == 0 ) 
	{
		NPC_Spawn_f();
	}
	else if ( Q_stricmp( cmd, "kill" ) == 0 ) 
	{
		NPC_Kill_f();
	}
	else if ( Q_stricmp( cmd, "showbounds" ) == 0 )
	{//Toggle on and off
		showBBoxes = showBBoxes ? qfalse : qtrue;
	}
	else if ( Q_stricmp ( cmd, "score" ) == 0 )
	{
		char		*cmd2 = gi.argv(2);
		gentity_t *ent = NULL;

		if ( !cmd2 || !cmd2[0] )
		{//Show the score for all NPCs
			gi.Printf( "SCORE LIST:\n" );
			for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
			{
				ent = &g_entities[i];
				if ( !ent || !ent->client )
				{
					continue;
				}
				NPC_PrintScore( ent );
			}
		}
		else
		{
			if ( (ent = G_Find( NULL, FOFS(targetname), cmd2 )) != NULL && ent->client )
			{
				NPC_PrintScore( ent );
			}
			else
			{
				gi.Printf( "ERROR: NPC score - no such NPC %s\n", cmd2 );
			}
		}
	}
}
