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

#include "g_headers.h"

#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "g_icarus.h"
#include "wp_saber.h"

extern cvar_t *g_sex;

extern qboolean G_CheckInSolid (gentity_t *self, qboolean fix);
extern void ClientUserinfoChanged( int clientNum );
extern qboolean SpotWouldTelefrag2( gentity_t *mover, vec3_t dest );
extern void Jedi_Cloak( gentity_t *self );

//extern void FX_BorgTeleport( vec3_t org );

extern void G_MatchPlayerWeapon( gentity_t *ent );
extern void Q3_SetParm (int entID, int parmNum, const char *parmValue);
extern team_t TranslateTeamName( const char *name );
extern char	*TeamNames[TEAM_NUM_TEAMS];

//extern void CG_ShimmeryThing_Spawner( vec3_t start, vec3_t end, float radius, qboolean taper, int duration );
extern void Q3_DebugPrint( int level, const char *format, ... );

//extern void NPC_StasisSpawnEffect( gentity_t *ent );

extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );

extern void WP_SaberInitBladeData( gentity_t *ent );
extern void ST_ClearTimers( gentity_t *ent );
extern void Jedi_ClearTimers( gentity_t *ent );
extern void NPC_ShadowTrooper_Precache( void );
extern void NPC_Gonk_Precache( void );
extern void NPC_Mouse_Precache( void );
extern void NPC_Seeker_Precache( void );
extern void NPC_Remote_Precache( void );
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t *self);
extern void NPC_MineMonster_Precache( void );
extern void NPC_Howler_Precache( void );
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_GalakMech_Precache( void );
extern void NPC_GalakMech_Init( gentity_t *ent );
extern void NPC_Protocol_Precache( void );
extern int WP_SetSaberModel( gclient_t *client, class_t npcClass );

#define	NSF_DROP_TO_FLOOR	16


//void HirogenAlpha_Precache( void );

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
		case CLASS_STORMTROOPER:
		case CLASS_SWAMPTROOPER:
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

extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
void NPC_SetMiscDefaultData( gentity_t *ent )
{
	if ( ent->spawnflags & SFB_CINEMATIC )
	{//if a cinematic guy, default us to wait bState
		ent->NPC->behaviorState = BS_CINEMATIC;
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
			ent->NPC->stats.moveType = MT_FLYSWIM;
			ent->count = 30; // SEEKER shot ammo count
			return;
		}
		else if ( ent->client->NPC_class == CLASS_JEDI || ent->client->NPC_class == CLASS_LUKE )
		{//good jedi
			ent->client->ps.saberActive = qfalse;
			ent->client->ps.saberLength = 0;
			WP_SaberInitBladeData( ent );
			G_CreateG2AttachedWeaponModel( ent, ent->client->ps.saberModel );
			ent->client->enemyTeam = TEAM_ENEMY;
			WP_InitForcePowers( ent );
			Jedi_ClearTimers( ent );
			if ( ent->spawnflags & JSF_AMBUSH )
			{//ambusher
				ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
				ent->client->noclip = qtrue;//hang
			}
		}
		else
		{
			if (ent->client->ps.weapon != WP_NONE)
			{
				G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl );
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
					ent->NPC->scriptFlags |= SCF_ALT_FIRE;
				}
				break;
			}
		}
		if ( ent->client->NPC_class == CLASS_KYLE || (ent->spawnflags & SFB_CINEMATIC) )
		{
			ent->NPC->defaultBehavior = BS_CINEMATIC;
		}
		else
		{
			ent->NPC->defaultBehavior = BS_FOLLOW_LEADER;
			ent->client->leader = &g_entities[0];
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
			if ( ent->client->NPC_class == CLASS_SHADOWTROOPER )
			{//FIXME: a spawnflag?
				Jedi_Cloak( ent );
			}
		 	if( ent->client->NPC_class == CLASS_TAVION ||
				ent->client->NPC_class == CLASS_REBORN ||
				ent->client->NPC_class == CLASS_DESANN ||
				ent->client->NPC_class == CLASS_SHADOWTROOPER )
			{
				ent->client->ps.saberActive = qfalse;
				ent->client->ps.saberLength = 0;
				WP_SaberInitBladeData( ent );
				G_CreateG2AttachedWeaponModel( ent, ent->client->ps.saberModel );
				WP_InitForcePowers( ent );
				ent->client->enemyTeam = TEAM_PLAYER;
				Jedi_ClearTimers( ent );
				if ( ent->spawnflags & JSF_AMBUSH )
				{//ambusher
					ent->NPC->scriptFlags |= SCF_IGNORE_ALERTS;
					ent->client->noclip = qtrue;//hang
				}
			}
			else if( ent->client->NPC_class == CLASS_PROBE || ent->client->NPC_class == CLASS_REMOTE ||
						ent->client->NPC_class == CLASS_INTERROGATOR || ent->client->NPC_class == CLASS_SENTRY)
			{		
				ent->NPC->defaultBehavior = BS_DEFAULT;
				ent->client->ps.gravity = 0;
				ent->svFlags |= SVF_CUSTOM_GRAVITY;
				ent->NPC->stats.moveType = MT_FLYSWIM;
			}
			else 
			{
				G_CreateG2AttachedWeaponModel( ent, weaponData[ent->client->ps.weapon].weaponMdl );
				switch ( ent->client->ps.weapon )
				{
				case WP_BRYAR_PISTOL:
					break;
				case WP_BLASTER_PISTOL:
					break;
				case WP_DISRUPTOR:
					//Sniper
					ent->NPC->scriptFlags |= SCF_ALT_FIRE;//FIXME: use primary fire sometimes?  Up close?  Different class of NPC?
					break;
				case WP_BOWCASTER:
					break;
				case WP_REPEATER:
					//machine-gunner
					break;
				case WP_DEMP2:
					break;
				case WP_FLECHETTE:
					//shotgunner
					if ( !Q_stricmp( "stofficeralt", ent->NPC_type ) )
					{
						ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					break;
				case WP_ROCKET_LAUNCHER:
					break;
				case WP_THERMAL:
					//Gran, use main, bouncy fire
//					ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					break;
				case WP_MELEE:
					break;
				default:
				case WP_BLASTER:
					//FIXME: health in NPCs.cfg, and not all blaster users are stormtroopers
					//FIXME: not necc. a ST
					ST_ClearTimers( ent );
					if ( ent->NPC->rank >= RANK_COMMANDER )
					{//commanders use alt-fire
						ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					if ( !Q_stricmp( "rodian2", ent->NPC_type ) )
					{
						ent->NPC->scriptFlags |= SCF_ALT_FIRE;
					}
					break;
				}
				if ( !Q_stricmp( "galak_mech", ent->NPC_type ) )
				{//starts with armor
					NPC_GalakMech_Init( ent );
				}
			}
		}
		break;

	default:
		break;
	}


	if ( ent->client->NPC_class == CLASS_ATST || ent->client->NPC_class == CLASS_MARK1 ) // chris/steve/kevin requested that the mark1 be shielded also
	{
		ent->flags |= (FL_SHIELDED|FL_NO_KNOCKBACK);
	}
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
			Q_strncmp( "reborn", NPC_type, 6 ) == 0 || 
			Q_stricmp( "desann", NPC_type ) == 0 || 
			Q_strncmp( "shadowtrooper", NPC_type, 13 ) == 0 )
			return ( 1 << WP_SABER);
//			return ( 1 << WP_IMPERIAL_BLADE);
		//NOTENOTE: Falls through if not a knife user

//	case TEAM_SCAVENGERS:
//	case TEAM_MALON:
		//FIXME: default weapon in npc config?
		if ( Q_strncmp( "stofficer", NPC_type, 9 ) == 0 )
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
		if ( Q_strncmp( "shadowtrooper", NPC_type, 13 ) == 0 )
		{
			return ( 1 << WP_SABER);//|( 1 << WP_RAPID_CONCUSSION)?
		}
		if ( Q_stricmp( "imperial", NPC_type ) == 0 )
		{
			return ( 1 << WP_BLASTER_PISTOL);
		}
		if ( Q_strncmp( "impworker", NPC_type, 9 ) == 0 )
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
		if ( Q_strncmp( "ugnaught", NPC_type, 8 ) == 0 )
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
		if ( Q_strncmp( "gran", NPC_type, 4 ) == 0 )
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

		if (( Q_stricmp( "interrogator",NPC_type) == 0) || ( Q_stricmp( "sentry",NPC_type) == 0) || (Q_strncmp( "protocol",NPC_type,8) == 0) )
		{
			return WP_NONE;
		}

		if ( Q_strncmp( "weequay", NPC_type, 7 ) == 0 )
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
		if ( Q_stricmp( "remote", NPC_type ) == 0 )
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

		if ( Q_strncmp( "jedi", NPC_type, 4 ) == 0 || Q_stricmp( "luke", NPC_type ) == 0 )
			return ( 1 << WP_SABER);

		if ( Q_strncmp( "prisoner", NPC_type, 8 ) == 0 )
		{
			return WP_NONE;
		}
		if ( Q_strncmp( "bespincop", NPC_type, 9 ) == 0 )
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
		if ( Q_strncmp( "ugnaught", NPC_type, 8 ) == 0 )
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

	// these teams are gone now anyway, plus all team stuff should be read in from the config file
/*
	switch ( ent->client->playerTeam )
	{
	case TEAM_KLINGON:
	case TEAM_MALON:
	case TEAM_HIROGEN:
	case TEAM_IMPERIAL:
		ent->client->playerTeam = TEAM_SCAVENGERS;
		break;
	}
*/

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
	ent->client->ps.weaponstate = WEAPON_IDLE;
	ChangeWeapon( ent, bestWeap );
}

/*
-------------------------
NPC_SpawnEffect

  NOTE:  Make sure any effects called here have their models, tga's and sounds precached in
			CG_RegisterNPCEffects in cg_player.cpp
-------------------------
*/

void NPC_SpawnEffect (gentity_t *ent)
{
	/*
	gentity_t	*tent;

	// NOTE:  Make sure any effects called here have their models, tga's and sounds precached in
	//		CG_RegisterNPCEffects in cg_player.cpp
	switch( ent->client->playerTeam )
	{
	case TEAM_BORG:
//		FX_BorgTeleport( ent->client->ps.origin );
		break;

	case TEAM_PARASITE:
	case TEAM_BOTS:
	default:
		break;
	}
	*/
}

//--------------------------------------------------------------
// NPC_SetFX_SpawnStates
//
// Set up any special parms for spawn effects
//--------------------------------------------------------------
void NPC_SetFX_SpawnStates( gentity_t *ent )
{

	// stasis, 8472, etc no longer exist. However if certain NPC's need customized spawn effects, use
	// NPC_class instead of TEAM_
/*
	// -Etherians -------
	if ( ent->client->playerTeam == TEAM_STASIS )
	{
		ent->svFlags |= SVF_CUSTOM_GRAVITY;
		ent->client->ps.gravity = 300;

		// The spawn effect can happen, so it's ok to do this extra setup stuff for the effect.
		ent->fx_time = level.time;
		ent->s.eFlags |= EF_SCALE_UP;
		ent->client->ps.eFlags |= EF_SCALE_UP;
		// Make it really small to start with
		VectorSet( ent->->s.modelScale, 0.001,0.001,0.001 );
	}
	else
*/
	{
		ent->client->ps.gravity = g_gravity->value;
	}
/*
	// -Hunterseeker -------
	if ( !stricmp( ent->NPC_type, "hunterseeker" ) )
	{
		// Set the custom banking flag
		ent->s.eFlags |= EF_BANK_STRAFE;
		ent->client->ps.eFlags |= EF_BANK_STRAFE;
	}

	// -8472 -------
	if ( ent->client->playerTeam == TEAM_8472 )
	{
		// The spawn effect can happen, so it's ok to do this extra setup stuff for the effect.
		ent->fx_time = level.time;
		ent->s.eFlags |= EF_SCALE_UP;
		ent->client->ps.eFlags |= EF_SCALE_UP;
		// Make it really small to start with
		VectorSet( ent->s.modelScale, 0.001,0.001,1 );
	}

	// -Forge -----
	if ( ent->client->playerTeam == TEAM_FORGE )
	{
		// The spawn effect can happen, so it's ok to do this extra setup stuff for the effect.
		ent->fx_time = level.time;
		ent->s.eFlags |= EF_SCALE_UP;
		ent->client->ps.eFlags |= EF_SCALE_UP;
		// Make it really small to start with
		VectorSet( ent->s.modelScale, 0.001,0.001,1 );
	}
*/
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
		if( SpotWouldTelefrag( ent, TEAM_FREE ) )//(team_t)(ent->client->playerTeam)
		{
			if ( ent->wait < 0 )
			{//remove yourself
				Q3_DebugPrint( WL_DEBUG, "NPC %s could not spawn, firing target3 (%s) and removing self\n", ent->targetname, ent->target3 );
				//Fire off our target3
				G_UseTargets2( ent, ent, ent->target3 );

				//Kill us
				ent->e_ThinkFunc = thinkF_G_FreeEntity;
				ent->nextthink = level.time + 100;
			}
			else
			{
				Q3_DebugPrint( WL_DEBUG, "NPC %s could not spawn, waiting %4.2 secs to try again\n", ent->targetname, ent->wait/1000.0f );
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
			ent->NPC->stats.health += ent->NPC->stats.health/4 * g_spskill->integer; // 100% on easy, 125% on medium, 150% on hard
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
	ent->inuse = qtrue;
	SetInUse(ent);
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
	if(!ent->NPC->stats.moveType)//Static?
	{
		ent->NPC->stats.moveType = MT_RUNJUMP;
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

	NPC_SetWeapons(ent);

	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	// clear entity state values
	ent->s.eType = ET_PLAYER;
	ent->s.eFlags |= EF_NPC;
//	ent->s.skinNum = ent - g_entities - 1;	// used as index to get custom models

	VectorCopy (spawn_origin, ent->s.origin);
//	ent->s.origin[2] += 1;	// make sure off ground

	SetClientViewAngle( ent, spawn_angles );
	client->renderInfo.lookTarget = ENTITYNUM_NONE;

	if(!(ent->spawnflags & 64))
	{
		G_KillBox( ent );
		gi.linkentity (ent);
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity->value * 1000;
	client->latched_buttons = 0;

	// set default animations
	NPC_SetAnim( ent, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL );

	if( spawnPoint )
	{
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	//ICARUS include
	ICARUS_InitEnt( ent );

//==NPC initialization
	SetNPCGlobals( ent );

	ent->enemy = NULL;
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

	ent->client->ps.eFlags |= EF_NPC;
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
		if( ent->taskManager && !stop_icarus )
		{
			ent->taskManager->Update();
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

	if ( ent->client->playerTeam == TEAM_ENEMY )
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

gNPC_t *New_NPC_t()
{
	gNPC_t *ptr = (gNPC_t *)G_Alloc (sizeof(gNPC_t));

	if (ptr)
	{
		// clear it...
		//
		memset(ptr, 0, sizeof( *ptr ) );
	}

	return ptr;
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
/*
-------------------------
NPC_Spawn_Go
-------------------------
*/

void NPC_Spawn_Go( gentity_t *ent )
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
	//Test for drop to floor
	if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
	{
		trace_t		tr;
		vec3_t		bottom;

		VectorCopy( ent->currentOrigin, saveOrg );
		VectorCopy( ent->currentOrigin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		gi.trace( &tr, ent->currentOrigin, ent->mins, ent->maxs, bottom, ent->s.number, MASK_NPCSOLID, G2_NOCOLLIDE, 0 );
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
			//FIXME: why not remove me...?  Because of all the string pointers?  Just do G_NewStrings?
		}
	}

	newent = G_Spawn();

	if ( newent == NULL ) 
	{
		gi.Printf ( S_COLOR_RED"ERROR: NPC G_Spawn failed\n" );
		
		goto finish;
		return;
	}
	
	newent->svFlags |= SVF_NPC;
	newent->fullName = ent->fullName;

	newent->NPC = New_NPC_t();	
	if ( newent->NPC == NULL ) 
	{		
		gi.Printf ( S_COLOR_RED"ERROR: NPC G_Alloc NPC failed\n" );		
		goto finish;
		return;
	}	

	newent->NPC->tempGoal = G_Spawn();
	
	if ( newent->NPC->tempGoal == NULL ) 
	{
		newent->NPC = NULL;
		goto finish;
		return;
	}

	newent->NPC->tempGoal->classname = "NPC_goal";
	newent->NPC->tempGoal->owner = newent;
	newent->NPC->tempGoal->svFlags |= SVF_NOCLIENT;

	newent->client = (gclient_s *)G_Alloc (sizeof(gclient_s));
	
	if ( newent->client == NULL ) 
	{
		gi.Printf ( S_COLOR_RED"ERROR: NPC G_Alloc client failed\n" );
		goto finish;
		return;
	}
	
	memset ( newent->client, 0, sizeof(*newent->client) );

//==NPC_Connect( newent, net_name );===================================

	if ( ent->NPC_type == NULL ) 
	{
		ent->NPC_type = "random";
	}
	else
	{
		ent->NPC_type = Q_strlwr( G_NewString( ent->NPC_type ) );
	}

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
		newent->message = ent->message;//transfer the key name
		newent->flags |= FL_NO_KNOCKBACK;//don't fall off ledges
	}

	if ( !NPC_ParseParms( ent->NPC_type, newent ) )
	{
		gi.Printf ( S_COLOR_RED "ERROR: Couldn't spawn NPC %s\n", ent->NPC_type );
		G_FreeEntity( newent );
		goto finish;
		return;
	}

	if ( ent->NPC_type )
	{
		if ( !Q_stricmp( ent->NPC_type, "kyle" ) )
		{
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
			newent->classname = "NPC";
	//		newent->svFlags |= SVF_NOPUSH;
		}
	}
//=====================================================================
	//set the info we want
	newent->health = ent->health;
	newent->script_targetname = ent->NPC_targetname;
	newent->targetname = ent->NPC_targetname;
	newent->target = ent->NPC_target;//death
	newent->target2 = ent->target2;//knocked out death
	newent->target3 = ent->target3;//???
	newent->target4 = ent->target4;//ffire death
	newent->wait = ent->wait;
	
	for( index = BSET_FIRST; index < NUM_BSETS; index++)
	{
		if ( ent->behaviorSet[index] )
		{
			newent->behaviorSet[index] = ent->behaviorSet[index];
		}
	}

	newent->classname = "NPC";
	newent->NPC_type = ent->NPC_type;
	gi.unlinkentity(newent);
	
	VectorCopy(ent->s.origin, newent->s.origin);
	VectorCopy(ent->s.origin, newent->client->ps.origin);
	VectorCopy(ent->s.origin, newent->currentOrigin);
	G_SetOrigin(newent, ent->s.origin);//just to be sure!

	VectorCopy(ent->s.angles, newent->s.angles);
	VectorCopy(ent->s.angles, newent->currentAngles);
	VectorCopy(ent->s.angles, newent->client->ps.viewangles);
	newent->NPC->desiredYaw =ent->s.angles[YAW];
	
	gi.linkentity(newent);
	newent->spawnflags = ent->spawnflags;

	if(ent->paintarget)
	{	//safe to point at owner's string since memory is never freed during game
		newent->paintarget = ent->paintarget;
	}
	if(ent->opentarget)
	{
		newent->opentarget = ent->opentarget;
	}

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

	newent->flags |= FL_NOTARGET;//So he's ignored until he's fully spawned
	newent->s.eFlags |= EF_NODRAW;//So he's ignored until he's fully spawned

	newent->e_ThinkFunc = thinkF_NPC_Begin;
	newent->nextthink = level.time + FRAMETIME;
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
			newent->target = ent->closetarget;
		}
		ent->targetname = NULL;
		//why not remove me...?  Because of all the string pointers?  Just do G_NewStrings?
		G_FreeEntity( ent );//bye!
	}

finish:
	if ( ent->spawnflags & NSF_DROP_TO_FLOOR )
	{
		G_SetOrigin( ent, saveOrg );
	}
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

/*QUAK-ED NPC_spawner (1 0 0) (-16 -16 -24) (16 16 32) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY

DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

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
//void NPC_PrecacheModels ( char *NPCName );
extern qboolean	spawning;				// the G_Spawn*() functions are valid  (only turned on during one function)
void SP_NPC_spawner( gentity_t *self)
{
	extern void NPC_PrecacheAnimationCFG( const char *NPC_type );

	if ( !self->fullName || !self->fullName[0] )
	{
		//FIXME: make an index into an external string table for localization
		self->fullName = "Humanoid Lifeform";
	}

	//register/precache the models needed for this NPC, not anymore
	//self->classname = "NPC_spawner";

	if(!self->count)
	{
		self->count = 1;
	}

	{//Stop loading of certain extra sounds
		static	int	garbage;

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
	}

	if ( !self->wait )
	{
		self->wait = 500;
	}
	else
	{
		self->wait *= 1000;//1 = 1 msec, 1000 = 1 sec
	}

	self->delay *= 1000;//1 = 1 msec, 1000 = 1 sec

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

	//FIXME: store cameraGroup somewhere else and apply to spawned NPCs' cameraGroup
	//Or just don't include NPC_spawners in cameraGroupings
}

//Characters

//STAR WARS NPCs============================================================================
/*QUAKED NPC_***** (1 0 0) (-16 -16 -24) (16 16 32) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY

CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

targetname - name this NPC goes by for targetting
target - NPC will fire this when it spawns it's last NPC (should this be when the last NPC it spawned dies?)

If targeted, will only spawn a NPC when triggered
count - how many NPCs to spawn (only if targetted) default = 1

NPC_targetname - NPC's targetname AND script_targetname
NPC_target - NPC's target to fire when killed
health - starting health (default = 100)
playerTeam - Who not to shoot! (default is TEAM_STARFLEET)
	TEAM_FREE (none) = 0
	TEAM_RED = 1
	TEAM_BLUE = 2
	TEAM_GOLD = 3
	TEAM_GREEN = 4
	TEAM_STARFLEET = 5
	TEAM_BORG = 6
	TEAM_SCAVENGERS = 7
	TEAM_STASIS = 8
	TEAM_NPCS = 9
	TEAM_HARVESTER, = 10
	TEAM_FORGE = 11
enemyTeam - Who to shoot (all but own if not set)

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

//=============================================================================================
//CHARACTERS
//=============================================================================================

/*QUAKED NPC_Kyle (1 0 0) (-16 -16 -24) (16 16 32) x RIFLEMAN PHASER TRICORDER DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
*/
void SP_NPC_Kyle( gentity_t *self)
{
	self->NPC_type = "Kyle";

	WP_SetSaberModel( NULL, CLASS_KYLE );

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

	WP_SetSaberModel( NULL, CLASS_LUKE );

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

	WP_SetSaberModel( NULL, CLASS_TAVION );

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
	if ( self->spawnflags & 1 )
	{
		self->NPC_type = "Galak_Mech";
		NPC_GalakMech_Precache();
	}
	else
	{
		self->NPC_type = "Galak";
	}

	SP_NPC_spawner( self );
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

	WP_SetSaberModel( NULL, CLASS_DESANN );

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

/*QUAKED NPC_Jedi(1 0 0) (-16 -16 -24) (16 16 40) TRAINER x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
TRAINER - Special Jedi- instructor
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Ally Jedi NPC Buddy - tags along with player
*/
void SP_NPC_Jedi( gentity_t *self)
{
	if(!self->NPC_type)
	{
		if ( self->spawnflags & 1 )
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

	WP_SetSaberModel( NULL, CLASS_JEDI );

	SP_NPC_spawner( self );
}

/*QUAKED NPC_Prisoner(1 0 0) (-16 -16 -24) (16 16 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
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
		if ( Q_irand( 0, 1 ) )
		{
			self->NPC_type = "Prisoner";
		}
		else
		{
			self->NPC_type = "Prisoner2";
		}
	}

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
		if ( Q_irand( 0, 1 ) )
		{
			self->NPC_type = "Rebel";
		}
		else
		{
			self->NPC_type = "Rebel2";
		}
	}

	SP_NPC_spawner( self );
}

//=============================================================================================
//ENEMIES
//=============================================================================================

/*QUAKED NPC_Stormtrooper(1 0 0) (-16 -16 -24) (16 16 40) OFFICER COMMANDER ALTOFFICER ROCKET DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
30 health, blaster

OFFICER - 60 health, flechette
COMMANDER - 60 health, heavy repeater
ALTOFFICER - 60 health, alt-fire flechette (grenades)
ROCKET - 60 health, rocket launcher

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
	if ( !self->NPC_type )
	{
		if ( !Q_irand( 0, 2 ) )
		{
			self->NPC_type = "ImpWorker";
		}
		else if ( Q_irand( 0, 1 ) )
		{
			self->NPC_type = "ImpWorker2";
		}
		else
		{
			self->NPC_type = "ImpWorker3";
		}
	}

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

Default Reborn is A poor lightsaber fighter, acrobatic and uses no force powers.  Yellow saber, 40 health.

FORCE - Uses force powers but is not the best lightsaber fighter and not acrobatic.  Purple saber, 75 health.
FENCER - A good lightsaber fighter, but not acrobatic and uses no force powers.  Yellow saber, 100 health.
ACROBAT - quite acrobatic, but not the best lightsaber fighter and uses no force powers.  Red saber, 100 health.
BOSS - quite acrobatic, good lightsaber fighter and uses force powers.  Orange saber, 150 health.

NOTE: Saber colors are temporary until they have different by skins to tell them apart

CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
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
	
	WP_SetSaberModel( NULL, CLASS_REBORN );
	SP_NPC_spawner( self );
}

/*QUAKED NPC_ShadowTrooper(1 0 0) (-16 -16 -24) (16 16 40) x x x x CEILING CINEMATIC NOTSOLID STARTINSOLID SHY
CEILING - Sticks to the ceiling until he sees an enemy or takes pain
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy
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
	
	NPC_ShadowTrooper_Precache();
	WP_SetSaberModel( NULL, CLASS_SHADOWTROOPER );

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

/*QUAKED NPC_Monster_Howler (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
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
	NPC_MineMonster_Precache();
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

	NPC_Interrogator_Precache(self);
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

	NPC_Probe_Precache();
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

	NPC_Mark1_Precache();
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

	NPC_Mark2_Precache();
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

	NPC_ATST_Precache();
}

/*QUAKED NPC_Droid_Remote (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
DROPTOFLOOR - NPC can be in air, but will spawn on the closest floor surface below it
CINEMATIC - Will spawn with no default AI (BS_CINEMATIC)
NOTSOLID - Starts not solid
STARTINSOLID - Don't try to fix if spawn in solid
SHY - Spawner is shy

Remote Droid - the floating round droid used by Obi Wan to train Luke about the force while on the Millenium Falcon.
*/
void SP_NPC_Droid_Remote( gentity_t *self)
{
	self->NPC_type = "remote";

	SP_NPC_spawner( self );

	NPC_Remote_Precache();
}

/*QUAKED NPC_Droid_Seeker (1 0 0) (-12 -12 -24) (12 12 40) x x x x DROPTOFLOOR CINEMATIC NOTSOLID STARTINSOLID SHY
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

	NPC_Seeker_Precache();
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

	NPC_Sentry_Precache();
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

	//precache the Gonk sounds
	NPC_Gonk_Precache();
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

	//precache the Mouse sounds
	NPC_Mouse_Precache();

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

	NPC_R2D2_Precache();
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

	NPC_R5D2_Precache();
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
	NPC_Protocol_Precache();
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
	
	if(!NPCspawner)
	{
		gi.Printf( S_COLOR_RED"NPC_Spawn Error: Out of entities!\n" );
		return;
	}

	NPCspawner->e_ThinkFunc = thinkF_G_FreeEntity;
	NPCspawner->nextthink = level.time + FRAMETIME;
	
	const char	*npc_type = gi.argv( 2 );
	if (!*npc_type )
	{
		gi.Printf( S_COLOR_RED"Error, expected:\n NPC spawn [NPC type (from NCPCs.cfg)]\n" );
		return;
	}

	//Spawn it at spot of first player
	//FIXME: will gib them!
	AngleVectors(g_entities[0].client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(g_entities[0].currentOrigin, 64, forward, end);
	gi.trace(&trace, g_entities[0].currentOrigin, NULL, NULL, end, 0, MASK_SOLID, G2_NOCOLLIDE, 0);
	VectorCopy(trace.endpos, end);
	end[2] -= 24;
	gi.trace(&trace, trace.endpos, NULL, NULL, end, 0, MASK_SOLID, G2_NOCOLLIDE, 0);
	VectorCopy(trace.endpos, end);
	end[2] += 24;
	G_SetOrigin(NPCspawner, end);
	VectorCopy(NPCspawner->currentOrigin, NPCspawner->s.origin);
	//set the yaw so that they face away from player
	NPCspawner->s.angles[1] = g_entities[0].client->ps.viewangles[1];

	gi.linkentity(NPCspawner);

	NPCspawner->NPC_type = G_NewString( npc_type );
	NPCspawner->NPC_targetname = G_NewString(gi.argv( 3 ));

	NPCspawner->count = 1;

	NPCspawner->delay = 0;

	//NPCspawner->spawnflags |= SFB_NOTSOLID;

	//NPCspawner->playerTeam = TEAM_FREE;
	//NPCspawner->behaviorSet[BSET_SPAWN] = "common/guard";
	//call precache funcs for James' builds
	if ( !Q_stricmp( "gonk", NPCspawner->NPC_type))
	{
		NPC_Gonk_Precache();
	}
	else if ( !Q_stricmp( "mouse", NPCspawner->NPC_type))
	{
		NPC_Mouse_Precache();
	}
	else if ( !Q_strncmp( "r2d2", NPCspawner->NPC_type, 4))
	{
		NPC_R2D2_Precache();
	}
	else if ( !Q_stricmp( "atst", NPCspawner->NPC_type))
	{
		NPC_ATST_Precache();
	}
	else if ( !Q_strncmp( "r5d2", NPCspawner->NPC_type, 4))
	{
		NPC_R5D2_Precache();
	}
	else if ( !Q_stricmp( "mark1", NPCspawner->NPC_type))
	{
		NPC_Mark1_Precache();
	}
	else if ( !Q_stricmp( "mark2", NPCspawner->NPC_type))
	{
		NPC_Mark2_Precache();
	}
	else if ( !Q_stricmp( "interrogator", NPCspawner->NPC_type))
	{
		NPC_Interrogator_Precache(NULL);
	}
	else if ( !Q_stricmp( "probe", NPCspawner->NPC_type))
	{
		NPC_Probe_Precache();
	}
	else if ( !Q_stricmp( "seeker", NPCspawner->NPC_type))
	{
		NPC_Seeker_Precache();
	}
	else if ( !Q_stricmp( "remote", NPCspawner->NPC_type))
	{
		NPC_Remote_Precache();
	}
	else if ( !Q_strncmp( "shadowtrooper", NPCspawner->NPC_type, 13 ) )
	{
		NPC_ShadowTrooper_Precache();
	}
	else if ( !Q_stricmp( "minemonster", NPCspawner->NPC_type ))
	{
		NPC_MineMonster_Precache();
	}
	else if ( !Q_stricmp( "howler", NPCspawner->NPC_type ))
	{
		NPC_Howler_Precache();
	}
	else if ( !Q_stricmp( "sentry", NPCspawner->NPC_type ))
	{
		NPC_Sentry_Precache();
	}
	else if ( !Q_stricmp( "protocol", NPCspawner->NPC_type ))
	{
		NPC_Protocol_Precache();
	}
	else if ( !Q_stricmp( "galak_mech", NPCspawner->NPC_type ))
	{
		NPC_GalakMech_Precache();
	}

	NPC_Spawn( NPCspawner, NPCspawner, NPCspawner );
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
				gi.Printf( S_COLOR_RED"%s\n", TeamNames[n] );
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
			killTeam = TranslateTeamName( name );

			if ( killTeam == TEAM_FREE )
			{
				gi.Printf( S_COLOR_RED"NPC_Kill Error: team '%s' not recognized\n", name );
				gi.Printf( S_COLOR_RED"Valid team names are:\n");
				for ( n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
				{
					gi.Printf( S_COLOR_RED"%s\n", TeamNames[n] );
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
						player->health = 0;
						GEntity_DieFunc(player, player, player, player->max_health, MOD_UNKNOWN);
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
					player->health = 0;
					GEntity_DieFunc(player, player, player, player->max_health, MOD_UNKNOWN);
				}
			}
			else if( (player->targetname && Q_stricmp( name, player->targetname ) == 0)
				|| Q_stricmp( name, "all" ) == 0 )
			{
				gi.Printf( S_COLOR_GREEN"Killing NPC %s named %s\n", player->NPC_type, player->targetname );
				player->health = 0;
				player->client->ps.stats[STAT_HEALTH] = 0;
				GEntity_DieFunc(player, player, player, 100, MOD_UNKNOWN);
			}
		}
		else if ( player && (player->svFlags&SVF_NPC_PRECACHE) )
		{//a spawner
			gi.Printf( S_COLOR_GREEN"Removing NPC spawner %s named %s\n", player->NPC_type, player->targetname );
			G_FreeEntity( player );
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
		gi.Printf( " spawn [NPC type (from NCPCs.cfg)]\n" );
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
