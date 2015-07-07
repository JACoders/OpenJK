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

//NPC_combat.cpp

#include "g_headers.h"



#include "b_local.h"
#include "g_nav.h"
#include "g_navigator.h"

extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void G_SetEnemy( gentity_t *self, gentity_t *enemy );
extern qboolean NPC_CheckLookTarget( gentity_t *self );
extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_Jedi_RateNewEnemy( gentity_t *self, gentity_t *enemy );
extern int NAV_FindClosestWaypointForPoint( vec3_t point );
extern int NAV_GetNearestNode( gentity_t *self, int lastNode );
extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel );
extern qboolean PM_DroidMelee( int npc_class );

extern	CNavigator	navigator;

void ChangeWeapon( gentity_t *ent, int newWeapon );

void G_ClearEnemy (gentity_t *self)
{
	NPC_CheckLookTarget( self );

	if ( self->enemy )
	{
		if(	self->client && self->client->renderInfo.lookTarget == self->enemy->s.number )
		{
			NPC_ClearLookTarget( self );
		}

		if ( self->NPC && self->enemy == self->NPC->goalEntity )
		{
			self->NPC->goalEntity = NULL;
		}
		//FIXME: set last enemy?
	}

	self->enemy = NULL;
}

/*
-------------------------
NPC_AngerAlert
-------------------------
*/

#define	ANGER_ALERT_RADIUS			512
#define	ANGER_ALERT_SOUND_RADIUS	256

void G_AngerAlert( gentity_t *self )
{
	if ( self && self->NPC && (self->NPC->scriptFlags&SCF_NO_GROUPS) )
	{//I'm not a team playa...
		return;
	}
	if ( !TIMER_Done( self, "interrogating" ) )
	{//I'm interrogating, don't wake everyone else up yet... FIXME: this may never wake everyone else up, though!
		return;
	}
	//FIXME: hmm.... with all the other new alerts now, is this still neccesary or even a good idea...?
	G_AlertTeam( self, self->enemy, ANGER_ALERT_RADIUS, ANGER_ALERT_SOUND_RADIUS );	
}

/*
-------------------------
G_TeamEnemy
-------------------------
*/

qboolean G_TeamEnemy( gentity_t *self )
{//FIXME: Probably a better way to do this, is a linked list of your teammates already available?
	int	i;
	gentity_t	*ent;

	if ( !self->client || self->client->playerTeam == TEAM_FREE )
	{
		return qfalse;
	}
	if ( self && self->NPC && (self->NPC->scriptFlags&SCF_NO_GROUPS) )
	{//I'm not a team playa...
		return qfalse;
	}

	for( i = 1; i < MAX_GENTITIES; i++ )
	{
		ent = &g_entities[i];

		if ( ent == self )
		{
			continue;
		}

		if ( ent->health <= 0 )
		{
			continue;
		}

		if ( !ent->client )
		{
			continue;
		}

		if ( ent->client->playerTeam != self->client->playerTeam )
		{//ent is not on my team
			continue;
		}

		if ( ent->enemy )
		{//they have an enemy
			if ( !ent->enemy->client || ent->enemy->client->playerTeam != self->client->playerTeam )
			{//the ent's enemy is either a normal ent or is a player/NPC that is not on my team
				return qtrue;
			}
		}
	}

	return qfalse;
}

void G_AttackDelay( gentity_t *self, gentity_t *enemy )
{
	if ( enemy && self->client && self->NPC )
	{//delay their attack based on how far away they're facing from enemy
		vec3_t		fwd, dir;
		int			attDelay;

		VectorSubtract( self->client->renderInfo.eyePoint, enemy->currentOrigin, dir );//purposely backwards
		VectorNormalize( dir );
		AngleVectors( self->client->renderInfo.eyeAngles, fwd, NULL, NULL );
		//dir[2] = fwd[2] = 0;//ignore z diff?
		
		attDelay = (4-g_spskill->integer)*500;//initial: from 1000ms delay on hard to 2000ms delay on easy
		if ( self->client->playerTeam == TEAM_PLAYER )
		{//invert
			attDelay = 2000-attDelay;
		}
		attDelay += floor( (DotProduct( fwd, dir )+1.0f) * 2000.0f );//add up to 4000ms delay if they're facing away

		//FIXME: should distance matter, too?

		//Now modify the delay based on NPC_class, weapon, and team
		//NOTE: attDelay should be somewhere between 1000 to 6000 milliseconds
		switch ( self->client->NPC_class )
		{
		case CLASS_IMPERIAL://they give orders and hang back
			attDelay += Q_irand( 500, 1500 );
			break;
		case CLASS_STORMTROOPER://stormtroopers shoot sooner
			if ( self->NPC->rank >= RANK_LT )
			{//officers shoot even sooner
				attDelay -= Q_irand( 500, 1500 );
			}
			else
			{//normal stormtroopers don't have as fast reflexes as officers
				attDelay -= Q_irand( 0, 1000 );
			}
			break;
		case CLASS_SWAMPTROOPER://shoot very quickly?  What about guys in water?
			attDelay -= Q_irand( 1000, 2000 );
			break;
		case CLASS_IMPWORKER://they panic, don't fire right away
			attDelay += Q_irand( 1000, 2500 );
			break;
		case CLASS_TRANDOSHAN:
			attDelay -= Q_irand( 500, 1500 );
			break;
		case CLASS_JAN:				
		case CLASS_LANDO:			
		case CLASS_PRISONER:
		case CLASS_REBEL:
			attDelay -= Q_irand( 500, 1500 );
			break;
		case CLASS_GALAKMECH:	
		case CLASS_ATST:		
			attDelay -= Q_irand( 1000, 2000 );
			break;
		case CLASS_REELO:
		case CLASS_UGNAUGHT:
			return;
			break;
		case CLASS_MINEMONSTER:
		case CLASS_MURJJ:
			return;
			break;
		case CLASS_INTERROGATOR:
		case CLASS_PROBE:		
		case CLASS_MARK1:		
		case CLASS_MARK2:		
		case CLASS_SENTRY:		
			return;
			break;
		case CLASS_REMOTE:
		case CLASS_SEEKER:
			return;
			break;
		/*
		case CLASS_GRAN:
		case CLASS_RODIAN:
		case CLASS_WEEQUAY:
			break;
		case CLASS_JEDI:				
		case CLASS_SHADOWTROOPER:
		case CLASS_TAVION:
		case CLASS_REBORN:
		case CLASS_LUKE:				
		case CLASS_DESANN:			
			break;
		*/
		default:
			break;
		}

		switch ( self->s.weapon )
		{
		case WP_NONE:
		case WP_SABER:
			return;
			break;
		case WP_BRYAR_PISTOL:
			break;
		case WP_BLASTER:
			if ( self->NPC->scriptFlags & SCF_ALT_FIRE )
			{//rapid-fire blasters
				attDelay += Q_irand( 0, 500 );
			}
			else
			{//regular blaster
				attDelay -= Q_irand( 0, 500 );
			}
			break;
		case WP_BOWCASTER:
			attDelay += Q_irand( 0, 500 );
			break;
		case WP_REPEATER:
			if ( !(self->NPC->scriptFlags&SCF_ALT_FIRE) )
			{//rapid-fire blasters
				attDelay += Q_irand( 0, 500 );
			}
			break;
		case WP_FLECHETTE:
			attDelay += Q_irand( 500, 1500 );
			break;
		case WP_ROCKET_LAUNCHER:
			attDelay += Q_irand( 500, 1500 );
			break;
		case WP_BLASTER_PISTOL:	// apparently some enemy only version of the blaster
			attDelay -= Q_irand( 500, 1500 );
			break;
		case WP_DISRUPTOR://sniper's don't delay?
			return;
			break;
		case WP_THERMAL://grenade-throwing has a built-in delay
			return;
			break;
		case WP_MELEE:			// Any ol' melee attack
			return;
			break;
		case WP_EMPLACED_GUN:
			return;
			break;
		case WP_TURRET:			// turret guns 
			return;
			break;
		case WP_BOT_LASER:		// Probe droid	- Laser blast
			return;
			break;
		/*
		case WP_DEMP2:
			break;
		case WP_TRIP_MINE:
			break;
		case WP_DET_PACK:
			break;
		case WP_STUN_BATON:
			break;
		case WP_ATST_MAIN:
			break;
		case WP_ATST_SIDE:
			break;
		case WP_TIE_FIGHTER:
			break;
		case WP_RAPID_FIRE_CONC:
			break;
		*/
		}

		if ( self->client->playerTeam == TEAM_PLAYER )
		{//clamp it
			if ( attDelay > 2000 )
			{
				attDelay = 2000;
			}
		}

		//don't shoot right away
		if ( attDelay > 4000+((2-g_spskill->integer)*3000) )
		{
			attDelay = 4000+((2-g_spskill->integer)*3000);
		}
		TIMER_Set( self, "attackDelay", attDelay );//Q_irand( 1500, 4500 ) );
		//don't move right away either
		if ( attDelay > 4000 )
		{
			attDelay = 4000 - Q_irand(500, 1500);
		}
		else
		{
			attDelay -= Q_irand(500, 1500);
		}

		TIMER_Set( self, "roamTime", attDelay );//was Q_irand( 1000, 3500 );
	}
}
/*
-------------------------
G_SetEnemy
-------------------------
*/
void G_AimSet( gentity_t *self, int aim );
void G_SetEnemy( gentity_t *self, gentity_t *enemy )
{
	int	event = 0;
	
	//Must be valid
	if ( enemy == NULL )
		return;

	//Must be valid
	if ( enemy->inuse == 0 )
	{
		return;
	}

	//Don't take the enemy if in notarget
	if ( enemy->flags & FL_NOTARGET )
		return;

	if ( !self->NPC )
	{
		self->enemy = enemy;
		return;
	}

	if ( self->NPC->confusionTime > level.time )
	{//can't pick up enemies if confused
		return;
	}

#ifdef _DEBUG
	if ( self->s.number )
	{
		assert( enemy != self );
	}
#endif// _DEBUG
	
//	if ( enemy->client && enemy->client->playerTeam == TEAM_DISGUISE )
//	{//unmask the player
//		enemy->client->playerTeam = TEAM_PLAYER;
//	}
	
	if ( self->client && self->NPC && enemy->client && enemy->client->playerTeam == self->client->playerTeam )
	{//Probably a damn script!
		if ( self->NPC->charmedTime > level.time )
		{//Probably a damn script!
			return;
		}
	}

	if ( self->NPC && self->client && self->client->ps.weapon == WP_SABER )
	{
		//when get new enemy, set a base aggression based on what that enemy is using, how far they are, etc.
		NPC_Jedi_RateNewEnemy( self, enemy );
	}

	//NOTE: this is not necessarily true!
	//self->NPC->enemyLastSeenTime = level.time;

	if ( self->enemy == NULL )
	{
		//TEMP HACK: turn on our saber
		if ( self->health > 0 )
		{
			self->client->ps.saberActive = qtrue;
		}

		//FIXME: Have to do this to prevent alert cascading
		G_ClearEnemy( self );
		self->enemy = enemy;

		//Special case- if player is being hunted by his own people, set their enemy team correctly
		if ( self->client->playerTeam == TEAM_PLAYER && enemy->s.number == 0 )
		{
			self->client->enemyTeam = TEAM_PLAYER;
		}

		//If have an anger script, run that instead of yelling
		if( G_ActivateBehavior( self, BSET_ANGER ) )
		{
		}
		else if ( self->client && enemy->client && self->client->playerTeam != enemy->client->playerTeam )
		{
			//FIXME: Use anger when entire team has no enemy.
			//		 Basically, you're first one to notice enemies
			if ( self->forcePushTime < level.time ) // not currently being pushed
			{
				if ( !G_TeamEnemy( self ) )
				{//team did not have an enemy previously
					event = Q_irand(EV_ANGER1, EV_ANGER3);
				}
			}
			
			if ( event )
			{//yell
				G_AddVoiceEvent( self, event, 2000 );
			}
		}
		
		if ( self->s.weapon == WP_BLASTER || self->s.weapon == WP_REPEATER ||
			self->s.weapon == WP_THERMAL || self->s.weapon == WP_BLASTER_PISTOL 
			|| self->s.weapon == WP_BOWCASTER )
		{//Hmm, how about sniper and bowcaster?
			//When first get mad, aim is bad
			//Hmm, base on game difficulty, too?  Rank?
			if ( self->client->playerTeam == TEAM_PLAYER )
			{
				G_AimSet( self, Q_irand( self->NPC->stats.aim - (5*(g_spskill->integer)), self->NPC->stats.aim - g_spskill->integer ) );
			}
			else
			{
				int minErr = 3;
				int maxErr = 12;
				if ( self->client->NPC_class == CLASS_IMPWORKER )
				{
					minErr = 15;
					maxErr = 30;
				}
				else if ( self->client->NPC_class == CLASS_STORMTROOPER && self->NPC && self->NPC->rank <= RANK_CREWMAN )
				{
					minErr = 5;
					maxErr = 15;
				}

				G_AimSet( self, Q_irand( self->NPC->stats.aim - (maxErr*(3-g_spskill->integer)), self->NPC->stats.aim - (minErr*(3-g_spskill->integer)) ) );
			}
		}
		
		//Alert anyone else in the area
		if ( Q_stricmp( "desperado", self->NPC_type ) != 0 && Q_stricmp( "paladin", self->NPC_type ) != 0 )
		{//special holodeck enemies exception
			if ( !(self->client->ps.eFlags&EF_FORCE_GRIPPED) )
			{//gripped people can't call for help
				G_AngerAlert( self );
			}
		}

		//Stormtroopers don't fire right away!
		G_AttackDelay( self, enemy );

		//FIXME: this is a disgusting hack that is supposed to make the Imperials start with their weapon holstered- need a better way
		if ( self->client->ps.weapon == WP_NONE && !Q_strncmp( self->NPC_type, "imp", 3 ) && !(self->NPC->scriptFlags & SCF_FORCED_MARCH)  )
		{
			if ( self->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_BLASTER ) )
			{
				ChangeWeapon( self, WP_BLASTER );
				self->client->ps.weapon = WP_BLASTER;
				self->client->ps.weaponstate = WEAPON_READY;
				G_CreateG2AttachedWeaponModel( self, weaponData[WP_BLASTER].weaponMdl );
			}
			else if ( self->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_BLASTER_PISTOL ) )
			{
				ChangeWeapon( self, WP_BLASTER_PISTOL );
				self->client->ps.weapon = WP_BLASTER_PISTOL;
				self->client->ps.weaponstate = WEAPON_READY;
				G_CreateG2AttachedWeaponModel( self, weaponData[WP_BLASTER_PISTOL].weaponMdl );
			}
		}
		return;
	}
	
	//Otherwise, just picking up another enemy

	if ( event )
	{
		G_AddVoiceEvent( self, event, 2000 );
	}

	//Take the enemy
	G_ClearEnemy(self);
	self->enemy = enemy;
}

/*
int ChooseBestWeapon( void ) 
{
	int		n;
	int		weapon;

	// check weapons in the NPC's weapon preference order
	for ( n = 0; n < MAX_WEAPONS; n++ ) 
	{
		weapon = NPCInfo->weaponOrder[n];

		if ( weapon == WP_NONE ) 
		{
			break;
		}

		if ( !HaveWeapon( weapon ) ) 
		{
			continue;
		}

		if ( client->ps.ammo[weaponData[weapon].ammoIndex] ) 
		{
			return weapon;
		}
	}

	// check weapons serially (mainly in case a weapon is not on the NPC's list)
	for ( weapon = 1; weapon < WP_NUM_WEAPONS; weapon++ ) 
	{
		if ( !HaveWeapon( weapon ) ) 
		{
			continue;
		}

		if ( client->ps.ammo[weaponData[weapon].ammoIndex] ) 
		{
			return weapon;
		}
	}

	return client->ps.weapon;
}
*/

void ChangeWeapon( gentity_t *ent, int newWeapon ) 
{
	if ( !ent || !ent->client || !ent->NPC )
	{
		return;
	}

	ent->client->ps.weapon = newWeapon;
	ent->NPC->shotTime = 0;
	ent->NPC->burstCount = 0;
	ent->NPC->attackHold = 0;
	ent->NPC->currentAmmo = ent->client->ps.ammo[weaponData[newWeapon].ammoIndex];

	switch ( newWeapon ) 
	{
	case WP_BRYAR_PISTOL://prifle
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000;//attackdebounce
		break;

	case WP_BLASTER_PISTOL:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 1000;//attackdebounce
		if ( g_spskill->integer == 0 )
			ent->NPC->burstSpacing = 1000;//attack debounce
		else if ( g_spskill->integer == 1 )
			ent->NPC->burstSpacing = 750;//attack debounce
		else 
			ent->NPC->burstSpacing = 500;//attack debounce
		break;

	case WP_BOT_LASER://probe attack
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 600;//attackdebounce
		if ( g_spskill->integer == 0 )
			ent->NPC->burstSpacing = 600;//attack debounce
		else if ( g_spskill->integer == 1 )
			ent->NPC->burstSpacing = 400;//attack debounce
		else 
			ent->NPC->burstSpacing = 200;//attack debounce
		break;

	case WP_SABER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 0;//attackdebounce
		break;

	case WP_DISRUPTOR:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if ( ent->NPC->scriptFlags & SCF_ALT_FIRE )
		{
			switch( g_spskill->integer )
			{
			case 0:
				ent->NPC->burstSpacing = 2500;//attackdebounce
				break;
			case 1:
				ent->NPC->burstSpacing = 2000;//attackdebounce
				break;
			case 2:
				ent->NPC->burstSpacing = 1500;//attackdebounce
				break;
			}
		}
		else
		{
			ent->NPC->burstSpacing = 1000;//attackdebounce
		}
		break;

	case WP_BOWCASTER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 1000;//attackdebounce
		if ( g_spskill->integer == 0 )
			ent->NPC->burstSpacing = 1000;//attack debounce
		else if ( g_spskill->integer == 1 )
			ent->NPC->burstSpacing = 750;//attack debounce
		else 
			ent->NPC->burstSpacing = 500;//attack debounce
		break;

	case WP_REPEATER:
		if ( ent->NPC->scriptFlags & SCF_ALT_FIRE )
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			ent->NPC->burstSpacing = 2000;//attackdebounce
		}
		else
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 3;
			ent->NPC->burstMax = 10;
			if ( g_spskill->integer == 0 )
				ent->NPC->burstSpacing = 1500;//attack debounce
			else if ( g_spskill->integer == 1 )
				ent->NPC->burstSpacing = 1000;//attack debounce
			else 
				ent->NPC->burstSpacing = 500;//attack debounce
		}
		break;

	case WP_DEMP2:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000;//attackdebounce
		break;

	case WP_FLECHETTE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		if ( ent->NPC->scriptFlags & SCF_ALT_FIRE )
		{
			ent->NPC->burstSpacing = 2000;//attackdebounce
		}
		else
		{
			ent->NPC->burstSpacing = 1000;//attackdebounce
		}
		break;

	case WP_ROCKET_LAUNCHER:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 2500;//attackdebounce
		if ( g_spskill->integer == 0 )
			ent->NPC->burstSpacing = 2500;//attack debounce
		else if ( g_spskill->integer == 1 )
			ent->NPC->burstSpacing = 2000;//attack debounce
		else 
			ent->NPC->burstSpacing = 1500;//attack debounce
		break;

	case WP_THERMAL:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 3000;//attackdebounce
		if ( g_spskill->integer == 0 )
			ent->NPC->burstSpacing = 3000;//attack debounce
		else if ( g_spskill->integer == 1 )
			ent->NPC->burstSpacing = 2500;//attack debounce
		else 
			ent->NPC->burstSpacing = 2000;//attack debounce
		break;

	/*
	case WP_SABER:
		ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
		ent->NPC->burstMin = 5;//0.5 sec
		ent->NPC->burstMax = 20;//3 seconds
		ent->NPC->burstSpacing = 2000;//2 seconds
		ent->NPC->attackHold = 1000;//Hold attack button for a 1-second burst
		break;
	
	case WP_TRICORDER:
		ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
		ent->NPC->burstMin = 5;
		ent->NPC->burstMax = 30;
		ent->NPC->burstSpacing = 1000;
		break;
	*/

	case WP_BLASTER:
		if ( ent->NPC->scriptFlags & SCF_ALT_FIRE )
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 3;
			ent->NPC->burstMax = 3;
			if ( g_spskill->integer == 0 )
				ent->NPC->burstSpacing = 1500;//attack debounce
			else if ( g_spskill->integer == 1 )
				ent->NPC->burstSpacing = 1000;//attack debounce
			else 
				ent->NPC->burstSpacing = 500;//attack debounce
		}
		else
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			if ( g_spskill->integer == 0 )
				ent->NPC->burstSpacing = 1000;//attack debounce
			else if ( g_spskill->integer == 1 )
				ent->NPC->burstSpacing = 750;//attack debounce
			else 
				ent->NPC->burstSpacing = 500;//attack debounce
		//	ent->NPC->burstSpacing = 1000;//attackdebounce
		}
		break;

	case WP_MELEE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		ent->NPC->burstSpacing = 1000;//attackdebounce
		break;

	case WP_ATST_MAIN:
	case WP_ATST_SIDE:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
	//	ent->NPC->burstSpacing = 1000;//attackdebounce
			if ( g_spskill->integer == 0 )
				ent->NPC->burstSpacing = 1000;//attack debounce
			else if ( g_spskill->integer == 1 )
				ent->NPC->burstSpacing = 750;//attack debounce
			else 
				ent->NPC->burstSpacing = 500;//attack debounce
		break;

	case WP_EMPLACED_GUN:
		//FIXME: give some designer-control over this?
		if ( ent->client && ent->client->NPC_class == CLASS_REELO )
		{
			ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
			ent->NPC->burstSpacing = 1000;//attack debounce
	//		if ( g_spskill->integer == 0 )
	//			ent->NPC->burstSpacing = 300;//attack debounce
	//		else if ( g_spskill->integer == 1 )
	//			ent->NPC->burstSpacing = 200;//attack debounce
	//		else 
	//			ent->NPC->burstSpacing = 100;//attack debounce
		}
		else
		{
			ent->NPC->aiFlags |= NPCAI_BURST_WEAPON;
			ent->NPC->burstMin = 2; // 3 shots, really
			ent->NPC->burstMax = 2;

			if ( ent->owner ) // if we have an owner, it should be the chair at this point...so query the chair for its shot debounce times, etc.
			{
				if ( g_spskill->integer == 0 )
				{
					ent->NPC->burstSpacing = ent->owner->wait + 400;//attack debounce
					ent->NPC->burstMin = ent->NPC->burstMax = 1; // two shots
				}
				else if ( g_spskill->integer == 1 )
				{
					ent->NPC->burstSpacing = ent->owner->wait + 200;//attack debounce
				}
				else 
				{
					ent->NPC->burstSpacing = ent->owner->wait;//attack debounce
				}
			}
			else
			{
				if ( g_spskill->integer == 0 )
				{
					ent->NPC->burstSpacing = 1200;//attack debounce
					ent->NPC->burstMin = ent->NPC->burstMax = 1; // two shots
				}
				else if ( g_spskill->integer == 1 )
				{
					ent->NPC->burstSpacing = 1000;//attack debounce
				}
				else 
				{
					ent->NPC->burstSpacing = 800;//attack debounce
				}
			}
		}
		break;

	default:
		ent->NPC->aiFlags &= ~NPCAI_BURST_WEAPON;
		break;
	}
}

void NPC_ChangeWeapon( int newWeapon ) 
{
	qboolean	changing = qfalse;
	if ( newWeapon != NPC->client->ps.weapon )
	{
		changing = qtrue;
	}
	if ( changing && NPC->weaponModel >= 9 )
	{
		gi.G2API_RemoveGhoul2Model( NPC->ghoul2, NPC->weaponModel );
	}
	ChangeWeapon( NPC, newWeapon );
	if ( changing && NPC->client->ps.weapon != WP_NONE )
	{
		if ( NPC->client->ps.weapon == WP_SABER )
		{
			G_CreateG2AttachedWeaponModel( NPC, NPC->client->ps.saberModel );
		}
		else
		{
			G_CreateG2AttachedWeaponModel( NPC, weaponData[NPC->client->ps.weapon].weaponMdl );
		}
	}
}
/*
void NPC_ApplyWeaponFireDelay(void)
How long, if at all, in msec the actual fire should delay from the time the attack was started
*/
void NPC_ApplyWeaponFireDelay(void)
{	
	if ( NPC->attackDebounceTime > level.time )
	{//Just fired, if attacking again, must be a burst fire, so don't add delay
		//NOTE: Borg AI uses attackDebounceTime "incorrectly", so this will always return for them!
		return;
	}
	
	switch(client->ps.weapon)
	{
	case WP_BOT_LASER:
		NPCInfo->burstCount = 0;
		client->fireDelay = 500;
		break;

	case WP_THERMAL:
		if ( client->ps.clientNum )
		{//NPCs delay... 
			//FIXME: player should, too, but would feel weird in 1st person, even though it
			//			would look right in 3rd person.  Really should have a wind-up anim
			//			for player as he holds down the fire button to throw, then play
			//			the actual throw when he lets go...
			client->fireDelay = 700;
		}
		break;

	case WP_MELEE:
		if ( !PM_DroidMelee( client->NPC_class ) )
		{//FIXME: should be unique per melee anim
			client->fireDelay = 300;
		}
		break;

	default:
		client->fireDelay = 0;
		break;
	}
};

/*
-------------------------
ShootThink
-------------------------
*/
void ShootThink( void ) 
{
	int			delay;
/*
	if ( enemyVisibility != VIS_SHOOT) 
		return;
*/

	ucmd.buttons |= BUTTON_ATTACK;

	NPCInfo->currentAmmo = client->ps.ammo[weaponData[client->ps.weapon].ammoIndex];	// checkme

	NPC_ApplyWeaponFireDelay();

	if ( NPCInfo->aiFlags & NPCAI_BURST_WEAPON ) 
	{
		if ( !NPCInfo->burstCount ) 
		{
			NPCInfo->burstCount = Q_irand( NPCInfo->burstMin, NPCInfo->burstMax );
			delay = 0;
		}
		else 
		{
			NPCInfo->burstCount--;
			if ( NPCInfo->burstCount == 0 ) 
			{
				delay = NPCInfo->burstSpacing;
			}
			else 
			{
				delay = 0;
			}
		}

		if ( !delay )
		{
			// HACK: dirty little emplaced bits, but is done because it would otherwise require some sort of new variable...
			if ( client->ps.weapon == WP_EMPLACED_GUN )
			{
				if ( NPC->owner ) // try and get the debounce values from the chair if we can
				{
					if ( g_spskill->integer == 0 )
					{
						delay = NPC->owner->random + 150;
					}
					else if ( g_spskill->integer == 1 )
					{
						delay = NPC->owner->random + 100;
					}
					else 
					{
						delay = NPC->owner->random;
					}
				}
				else
				{
					if ( g_spskill->integer == 0 )
					{
						delay = 350;
					}
					else if ( g_spskill->integer == 1 )
					{
						delay = 300;
					}
					else 
					{
						delay = 200;
					}
				}
			}
		}
	}
	else 
	{
		delay = NPCInfo->burstSpacing;
	}

	NPCInfo->shotTime = level.time + delay;
	NPC->attackDebounceTime = level.time + NPC_AttackDebounceForWeapon();
}

/*
static void WeaponThink( qboolean inCombat ) 
FIXME makes this so there's a delay from event that caused us to check to actually doing it

Added: hacks for Borg
*/
void WeaponThink( qboolean inCombat ) 
{
	ucmd.buttons &= ~BUTTON_ATTACK;

	if ( client->ps.weaponstate == WEAPON_RAISING || client->ps.weaponstate == WEAPON_DROPPING ) 
	{
		ucmd.weapon = client->ps.weapon;
		return;
	}

	if ( client->ps.weapon == WP_NONE )
	{
		return;
	}

	if ( client->ps.weaponstate != WEAPON_READY && client->ps.weaponstate != WEAPON_FIRING && client->ps.weaponstate != WEAPON_IDLE) 
	{
		return;
	}

	if ( level.time < NPCInfo->shotTime ) 
	{
		return;
	}

//MCG - Begin
	//For now, no-one runs out of ammo	
	if(NPC->client->ps.ammo[ weaponData[client->ps.weapon].ammoIndex ] < 10)	// checkme	
//	if(NPC->client->ps.ammo[ client->ps.weapon ] < 10)
	{
		Add_Ammo (NPC, client->ps.weapon, 100);
	}

	ucmd.weapon = client->ps.weapon;
	ShootThink();
}

/*
HaveWeapon
*/

qboolean HaveWeapon( int weapon ) 
{
	return ( client->ps.stats[STAT_WEAPONS] & ( 1 << weapon ) );
}

qboolean EntIsGlass (gentity_t *check)
{
	if(check->classname &&
		!Q_stricmp("func_breakable", check->classname) &&
		check->count == 1 && check->health <= 100)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean ShotThroughGlass (trace_t *tr, gentity_t *target, vec3_t spot, int mask)
{
	gentity_t	*hit = &g_entities[ tr->entityNum ];
	if(hit != target && EntIsGlass(hit))
	{//ok to shoot through breakable glass
		int			skip = hit->s.number;
		vec3_t		muzzle;

		VectorCopy(tr->endpos, muzzle);
		gi.trace (tr, muzzle, NULL, NULL, spot, skip, mask, G2_NOCOLLIDE, 0 );
		return qtrue;
	}

	return qfalse;
}

/*
CanShoot
determine if NPC can directly target enemy

this function does not check teams, invulnerability, notarget, etc....

Added: If can't shoot center, try head, if not, see if it's close enough to try anyway.
*/
qboolean CanShoot ( gentity_t *ent, gentity_t *shooter ) 
{
	trace_t		tr;
	vec3_t		muzzle;
	vec3_t		spot, diff;
	gentity_t	*traceEnt;

	CalcEntitySpot( shooter, SPOT_WEAPON, muzzle );
	CalcEntitySpot( ent, SPOT_ORIGIN, spot );		//FIXME preferred target locations for some weapons (feet for R/L)

	gi.trace ( &tr, muzzle, NULL, NULL, spot, shooter->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
	traceEnt = &g_entities[ tr.entityNum ];

	// point blank, baby!
	if (tr.startsolid && (shooter->NPC) && (shooter->NPC->touchedByPlayer) ) 
	{
		traceEnt = shooter->NPC->touchedByPlayer;
	}
	
	if ( ShotThroughGlass( &tr, ent, spot, MASK_SHOT ) )
	{
		traceEnt = &g_entities[ tr.entityNum ];
	}

	// shot is dead on
	if ( traceEnt == ent ) 
	{
		return qtrue;
	}
//MCG - Begin
	else
	{//ok, can't hit them in center, try their head
		CalcEntitySpot( ent, SPOT_HEAD, spot );
		gi.trace ( &tr, muzzle, NULL, NULL, spot, shooter->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
		traceEnt = &g_entities[ tr.entityNum ];
		if ( traceEnt == ent) 
		{
			return qtrue;
		}
	}

	//Actually, we should just check to fire in dir we're facing and if it's close enough,
	//and we didn't hit someone on our own team, shoot
	VectorSubtract(spot, tr.endpos, diff);
	if(VectorLength(diff) < random() * 32)
	{
		return qtrue;
	}
//MCG - End
	// shot would hit a non-client
	if ( !traceEnt->client ) 
	{
		return qfalse;
	}

	// shot is blocked by another player

	// he's already dead, so go ahead
	if ( traceEnt->health <= 0 ) 
	{
		return qtrue;
	}

	// don't deliberately shoot a teammate
	if ( traceEnt->client && ( traceEnt->client->playerTeam == shooter->client->playerTeam ) ) 
	{
		return qfalse;
	}

	// he's just in the wrong place, go ahead
	return qtrue;
}


/*
void NPC_CheckPossibleEnemy( gentity_t *other, visibility_t vis ) 

Added: hacks for scripted NPCs
*/
void NPC_CheckPossibleEnemy( gentity_t *other, visibility_t vis ) 
{
	// is he is already our enemy?
	if ( other == NPC->enemy ) 
		return;

	if ( other->flags & FL_NOTARGET ) 
		return;

	// we already have an enemy and this guy is in our FOV, see if this guy would be better
	if ( NPC->enemy && vis == VIS_FOV ) 
	{
		if ( NPCInfo->enemyLastSeenTime - level.time < 2000 ) 
		{
			return;
		}
		if ( enemyVisibility == VIS_UNKNOWN ) 
		{
			enemyVisibility = NPC_CheckVisibility ( NPC->enemy, CHECK_360|CHECK_FOV );
		}
		if ( enemyVisibility == VIS_FOV ) 
		{
			return;
		}
	}

	if ( !NPC->enemy )
	{//only take an enemy if you don't have one yet
		G_SetEnemy( NPC, other );
	}

	if ( vis == VIS_FOV ) 
	{
		NPCInfo->enemyLastSeenTime = level.time;
		VectorCopy( other->currentOrigin, NPCInfo->enemyLastSeenLocation );
		NPCInfo->enemyLastHeardTime = 0;
		VectorClear( NPCInfo->enemyLastHeardLocation );
	} 
	else 
	{
		NPCInfo->enemyLastSeenTime = 0;
		VectorClear( NPCInfo->enemyLastSeenLocation );
		NPCInfo->enemyLastHeardTime = level.time;
		VectorCopy( other->currentOrigin, NPCInfo->enemyLastHeardLocation );
	}
}


//==========================================
//MCG Added functions:
//==========================================

/*
int NPC_AttackDebounceForWeapon (void)

DOES NOT control how fast you can fire
Only makes you keep your weapon up after you fire

*/
int NPC_AttackDebounceForWeapon (void)
{
	switch ( NPC->client->ps.weapon ) 
	{
/*
	case WP_BLASTER://scav rifle
		return 1000;
		break;

	case WP_BRYAR_PISTOL://prifle
		return 3000;
		break;

	case WP_SABER:
		return 100;
		break;
	

	case WP_TRICORDER:
		return 0;//tricorder
		break;
*/
	case WP_SABER:
		return 0;
		break;

	case WP_BOT_LASER:
		
		if ( g_spskill->integer == 0 )
			return 2000;

		if ( g_spskill->integer == 1 )
			return 1500;

		return 1000;
		break;

	default:
		return NPCInfo->burstSpacing;//was 100 by default
		break;
	}
}

//FIXME: need a mindist for explosive weapons
float NPC_MaxDistSquaredForWeapon (void)
{
	if(NPCInfo->stats.shootDistance > 0)
	{//overrides default weapon dist
		return NPCInfo->stats.shootDistance * NPCInfo->stats.shootDistance;
	}

	switch ( NPC->s.weapon ) 
	{
	case WP_BLASTER://scav rifle
		return 1024 * 1024;//should be shorter?
		break;

	case WP_BRYAR_PISTOL://prifle
		return 1024 * 1024;
		break;

	case WP_BLASTER_PISTOL://prifle
		return 1024 * 1024;
		break;

	case WP_DISRUPTOR://disruptor
		if ( NPCInfo->scriptFlags & SCF_ALT_FIRE )
		{
			return ( 4096 * 4096 );
		}
		else
		{
			return 1024 * 1024;
		}
		break;
/*
	case WP_SABER:
		return 1024 * 1024;
		break;
	

	case WP_TRICORDER:
		return 0;//tricorder
		break;
*/
	case WP_SABER:
		if ( NPC->client && NPC->client->ps.saberLength )
		{//FIXME: account for whether enemy and I are heading towards each other!
			return (NPC->client->ps.saberLength + NPC->maxs[0]*1.5)*(NPC->client->ps.saberLength + NPC->maxs[0]*1.5);
		}
		else
		{
			return 48*48;
		}
		break;

	default:
		return 1024 * 1024;//was 0
		break;
	}
}

/*
-------------------------
ValidEnemy
-------------------------
*/

qboolean ValidEnemy(gentity_t *ent)
{
	if ( ent == NULL )
		return qfalse;

	if ( ent == NPC )
		return qfalse;

	//if team_free, maybe everyone is an enemy?
	if ( !NPC->client->enemyTeam )
		return qfalse;

	if ( !(ent->flags & FL_NOTARGET) )
	{
		if( ent->health > 0 )
		{
			if( !ent->client )
			{
				return qtrue;
			}
			else if( ent->client->playerTeam == NPC->client->enemyTeam )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean NPC_EnemyTooFar(gentity_t *enemy, float dist, qboolean toShoot)
{
	vec3_t	vec;

	
	if ( !toShoot )
	{//Not trying to actually press fire button with this check
		if ( NPC->client->ps.weapon == WP_SABER )
		{//Just have to get to him
			return qfalse;
		}
	}
	

	if(!dist)
	{
		VectorSubtract(NPC->currentOrigin, enemy->currentOrigin, vec);
		dist = VectorLengthSquared(vec);
	}

	if(dist > NPC_MaxDistSquaredForWeapon())
		return qtrue;

	return qfalse;
}

/*
NPC_PickEnemy

Randomly picks a living enemy from the specified team and returns it

FIXME: For now, you MUST specify an enemy team

If you specify choose closest, it will find only the closest enemy

If you specify checkVis, it will return and enemy that is visible

If you specify findPlayersFirst, it will try to find players first

You can mix and match any of those options (example: find closest visible players first)

FIXME: this should go through the snapshot and find the closest enemy
*/
gentity_t *NPC_PickEnemy( gentity_t *closestTo, int enemyTeam, qboolean checkVis, qboolean findPlayersFirst, qboolean findClosest )
{
	int			num_choices = 0;
	int			choice[128];//FIXME: need a different way to determine how many choices?
	gentity_t	*newenemy = NULL;
	gentity_t	*closestEnemy = NULL;
	int			entNum;
	vec3_t		diff;
	float		relDist;
	float		bestDist = Q3_INFINITE;
	qboolean	failed = qfalse;
	int			visChecks = (CHECK_360|CHECK_FOV|CHECK_VISRANGE);
	int			minVis = VIS_FOV;

	if (!enemyTeam)
	{
		return NULL;
	}

	if ( NPCInfo->behaviorState == BS_STAND_AND_SHOOT || 
		NPCInfo->behaviorState == BS_HUNT_AND_KILL ) 
	{//Formations guys don't require inFov to pick up a target
		//These other behavior states are active battle states and should not
		//use FOV.  FOV checks are for enemies who are patrolling, guarding, etc.
		visChecks &= ~CHECK_FOV;
		minVis = VIS_360;
	}

	if( findPlayersFirst )
	{//try to find a player first
		newenemy = &g_entities[0];
		if( newenemy->client && !(newenemy->flags & FL_NOTARGET) && !(newenemy->s.eFlags & EF_NODRAW))
		{
			if( newenemy->health > 0 )
			{
				if( enemyTeam == TEAM_PLAYER || newenemy->client->playerTeam == enemyTeam || 
					( enemyTeam == TEAM_PLAYER ) )
				{//FIXME:  check for range and FOV or vis?
					if( newenemy != NPC->lastEnemy )
					{//Make sure we're not just going back and forth here
						if ( gi.inPVS(newenemy->currentOrigin, NPC->currentOrigin) )
						{
							if(NPCInfo->behaviorState == BS_INVESTIGATE ||	NPCInfo->behaviorState == BS_PATROL)
							{
								if(!NPC->enemy)
								{
									if(!InVisrange(newenemy))
									{
										failed = qtrue;
									}
									else if(NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_FOV|CHECK_VISRANGE ) != VIS_FOV)
									{
										failed = qtrue;
									}
								}
							}

							if ( !failed )
							{
								VectorSubtract( closestTo->currentOrigin, newenemy->currentOrigin, diff );
								relDist = VectorLengthSquared(diff);
								if ( newenemy->client->hiddenDist > 0 )
								{
									if( relDist > newenemy->client->hiddenDist*newenemy->client->hiddenDist )
									{
										//out of hidden range
										if ( VectorLengthSquared( newenemy->client->hiddenDir ) )
										{//They're only hidden from a certain direction, check
											float	dot;
											VectorNormalize( diff );
											dot = DotProduct( newenemy->client->hiddenDir, diff ); 
											if ( dot > 0.5 )
											{//I'm not looking in the right dir toward them to see them 
												failed = qtrue;
											}
											else
											{
												Debug_Printf(debugNPCAI, DEBUG_LEVEL_INFO, "%s saw %s trying to hide - hiddenDir %s targetDir %s dot %f\n", NPC->targetname, newenemy->targetname, vtos(newenemy->client->hiddenDir), vtos(diff), dot );
											}
										}
										else
										{
											failed = qtrue;
										}
									}
									else
									{
										Debug_Printf(debugNPCAI, DEBUG_LEVEL_INFO, "%s saw %s trying to hide - hiddenDist %f\n", NPC->targetname, newenemy->targetname, newenemy->client->hiddenDist );
									}
								}

								if(!failed)
								{
									if(findClosest)
									{
										if(relDist < bestDist)
										{
											if(!NPC_EnemyTooFar(newenemy, relDist, qfalse))
											{
												if(checkVis)
												{
													if( NPC_CheckVisibility ( newenemy, visChecks ) == minVis )
													{
														bestDist = relDist;
														closestEnemy = newenemy;
													}
												}
												else
												{
													bestDist = relDist;
													closestEnemy = newenemy;
												}
											}
										}
									}
									else if(!NPC_EnemyTooFar(newenemy, 0, qfalse))
									{
										if(checkVis)
										{
											if( NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_FOV|CHECK_VISRANGE ) == VIS_FOV )
											{
												choice[num_choices++] = newenemy->s.number;
											}
										}
										else
										{
											choice[num_choices++] = newenemy->s.number;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (findClosest && closestEnemy)
	{
		return closestEnemy;
	}

	if (num_choices)
	{
		return &g_entities[ choice[rand() % num_choices] ];
	}

	/*
	//FIXME: used to have an option to look *only* for the player... now...?  Still need it?
	if ( enemyTeam == TEAM_PLAYER )
	{//couldn't find the player
		return NULL;
	}
	*/

	num_choices = 0;
	bestDist = Q3_INFINITE;
	closestEnemy = NULL;

	for ( entNum = 0; entNum < globals.num_entities; entNum++ )
	{
		newenemy = &g_entities[entNum];

		if ( newenemy != NPC && (newenemy->client || newenemy->svFlags & SVF_NONNPC_ENEMY) && !(newenemy->flags & FL_NOTARGET) && !(newenemy->s.eFlags & EF_NODRAW))
		{
			if ( newenemy->health > 0 )
			{
				if ( (newenemy->client && newenemy->client->playerTeam == enemyTeam) || (!newenemy->client && newenemy->noDamageTeam == enemyTeam) )
				{//FIXME:  check for range and FOV or vis?
					if ( NPC->client->playerTeam == TEAM_PLAYER && enemyTeam == TEAM_PLAYER )
					{//player allies turning on ourselves?  How?
						if ( newenemy->s.number )
						{//only turn on the player, not other player allies
							continue;
						}
					}

					if ( newenemy != NPC->lastEnemy )
					{//Make sure we're not just going back and forth here
						if(!gi.inPVS(newenemy->currentOrigin, NPC->currentOrigin))
						{
							continue;
						}

						if ( NPCInfo->behaviorState == BS_INVESTIGATE || NPCInfo->behaviorState == BS_PATROL )
						{
							if ( !NPC->enemy )
							{
								if ( !InVisrange( newenemy ) )
								{
									continue;
								}
								else if ( NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_FOV|CHECK_VISRANGE ) != VIS_FOV )
								{
									continue;
								}
							}
						}

						VectorSubtract( closestTo->currentOrigin, newenemy->currentOrigin, diff );
						relDist = VectorLengthSquared(diff);
						if ( newenemy->client && newenemy->client->hiddenDist > 0 )
						{
							if( relDist > newenemy->client->hiddenDist*newenemy->client->hiddenDist )
							{
								//out of hidden range
								if ( VectorLengthSquared( newenemy->client->hiddenDir ) )
								{//They're only hidden from a certain direction, check
									float	dot;

									VectorNormalize( diff );
									dot = DotProduct( newenemy->client->hiddenDir, diff ); 
									if ( dot > 0.5 )
									{//I'm not looking in the right dir toward them to see them 
										continue;
									}
									else
									{
										Debug_Printf(debugNPCAI, DEBUG_LEVEL_INFO, "%s saw %s trying to hide - hiddenDir %s targetDir %s dot %f\n", NPC->targetname, newenemy->targetname, vtos(newenemy->client->hiddenDir), vtos(diff), dot );
									}
								}
								else
								{
									continue;
								}
							}
							else
							{
								Debug_Printf(debugNPCAI, DEBUG_LEVEL_INFO, "%s saw %s trying to hide - hiddenDist %f\n", NPC->targetname, newenemy->targetname, newenemy->client->hiddenDist );
							}
						}

						if ( findClosest )
						{
							if ( relDist < bestDist )
							{
								if ( !NPC_EnemyTooFar( newenemy, relDist, qfalse ) )
								{
									if ( checkVis )
									{
										//FIXME: NPCs need to be able to pick up other NPCs behind them,
										//but for now, commented out because it was picking up enemies it shouldn't
										//if ( NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_VISRANGE ) >= VIS_360 )
										if ( NPC_CheckVisibility ( newenemy, visChecks ) == minVis )
										{
											bestDist = relDist;
											closestEnemy = newenemy;
										}
									}
									else
									{
										bestDist = relDist;
										closestEnemy = newenemy;
									}
								}
							}
						}
						else if ( !NPC_EnemyTooFar( newenemy, 0, qfalse ) )
						{
							if ( checkVis )
							{
								//if( NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_FOV|CHECK_VISRANGE ) == VIS_FOV )
								if ( NPC_CheckVisibility ( newenemy, CHECK_360|CHECK_VISRANGE ) >= VIS_360 )
								{
									choice[num_choices++] = newenemy->s.number;
								}
							}
							else
							{
								choice[num_choices++] = newenemy->s.number;
							}
						}
					}
				}
			}
		}
	}

	
	if (findClosest)
	{//FIXME: you can pick up an enemy around a corner this way.
		return closestEnemy;
	}

	if (!num_choices)
	{
		return NULL;
	}

	return &g_entities[ choice[rand() % num_choices] ];
}

/*
gentity_t *NPC_PickAlly ( void )

  Simply returns closest visible ally
*/

gentity_t *NPC_PickAlly ( qboolean facingEachOther, float range, qboolean ignoreGroup, qboolean movingOnly )
{
	gentity_t	*ally = NULL;
	gentity_t	*closestAlly = NULL;
	int			entNum;
	vec3_t		diff;
	float		relDist;
	float		bestDist = range;

	for ( entNum = 0; entNum < globals.num_entities; entNum++ )
	{
		ally = &g_entities[entNum];

		if ( ally->client )
		{
			if ( ally->health > 0 )
			{
				if ( ally->client && ( ally->client->playerTeam == NPC->client->playerTeam ||
					 NPC->client->playerTeam == TEAM_ENEMY ) )// && ally->client->playerTeam == TEAM_DISGUISE ) ) )
				{//if on same team or if player is disguised as your team
					if ( ignoreGroup )
					{
						if ( ally == NPC->client->leader )
						{
							//reject
							continue;
						}
						if ( ally->client && ally->client->leader && ally->client->leader == NPC )
						{
							//reject
							continue;
						}
					}

					if(!gi.inPVS(ally->currentOrigin, NPC->currentOrigin))
					{
						continue;
					}

					if ( movingOnly && ally->client && NPC->client )
					{//They have to be moving relative to each other
						if ( !DistanceSquared( ally->client->ps.velocity, NPC->client->ps.velocity ) )
						{
							continue;
						}
					}

					VectorSubtract( NPC->currentOrigin, ally->currentOrigin, diff );
					relDist = VectorNormalize( diff );
					if ( relDist < bestDist )
					{
						if ( facingEachOther )
						{
							vec3_t	vf;
							float	dot;

							AngleVectors( ally->client->ps.viewangles, vf, NULL, NULL );
							VectorNormalize(vf);
							dot = DotProduct(diff, vf);

							if ( dot < 0.5 )
							{//Not facing in dir to me
								continue;
							}
							//He's facing me, am I facing him?
							AngleVectors( NPC->client->ps.viewangles, vf, NULL, NULL );
							VectorNormalize(vf);
							dot = DotProduct(diff, vf);

							if ( dot > -0.5 )
							{//I'm not facing opposite of dir to me
								continue;
							}
							//I am facing him
						}

						if ( NPC_CheckVisibility ( ally, CHECK_360|CHECK_VISRANGE ) >= VIS_360 )
						{
							bestDist = relDist;
							closestAlly = ally;
						}
					}
				}
			}
		}
	}

	
	return closestAlly;
}

gentity_t *NPC_CheckEnemy( qboolean findNew, qboolean tooFarOk, qboolean setEnemy )
{
	qboolean	forcefindNew = qfalse;
	gentity_t	*closestTo;
	gentity_t	*newEnemy = NULL;
	//FIXME: have a "NPCInfo->persistance" we can set to determine how long to try to shoot
	//someone we can't hit?  Rather than hard-coded 10?

	//FIXME they shouldn't recognize enemy's death instantly

	//TEMP FIX:
	//if(NPC->enemy->client)
	//{
	//	NPC->enemy->health = NPC->enemy->client->ps.stats[STAT_HEALTH];
	//}

	if ( NPC->enemy )
	{
		if ( !NPC->enemy->inuse )//|| NPC->enemy == NPC )//wtf?  NPCs should never get mad at themselves!
		{
			if ( setEnemy )
			{
				G_ClearEnemy( NPC );
			}
		}
	}

	if ( NPC->svFlags & SVF_IGNORE_ENEMIES )
	{//We're ignoring all enemies for now
		if ( setEnemy )
		{
			G_ClearEnemy( NPC );
		}
		return NULL;
	}

	if ( NPC->svFlags & SVF_LOCKEDENEMY )
	{//keep this enemy until dead
		if ( NPC->enemy )
		{
			if ( (!NPC->NPC && !(NPC->svFlags & SVF_NONNPC_ENEMY) ) || NPC->enemy->health > 0 )
			{//Enemy never had health (a train or info_not_null, etc) or did and is now dead (NPCs, turrets, etc)
				return NULL;
			}
		}
		NPC->svFlags &= ~SVF_LOCKEDENEMY;
	}

	if ( NPC->enemy )
	{
		if ( NPC_EnemyTooFar(NPC->enemy, 0, qfalse) )
		{
			if(findNew)
			{//See if there is a close one and take it if so, else keep this one
				forcefindNew = qtrue;
			}
			else if(!tooFarOk)//FIXME: don't need this extra bool any more
			{
				if ( setEnemy )
				{
					G_ClearEnemy( NPC );
				}
			}
		}
		else if ( !gi.inPVS(NPC->currentOrigin, NPC->enemy->currentOrigin ) )
		{//FIXME: should this be a line-of site check?
			//FIXME: a lot of things check PVS AGAIN when deciding whether 
			//or not to shoot, redundant!
			//Should we lose the enemy?
			//FIXME: if lose enemy, run lostenemyscript
			if ( NPC->enemy->client && NPC->enemy->client->hiddenDist )
			{//He ducked into shadow while we weren't looking
				//Drop enemy and see if we should search for him
				NPC_LostEnemyDecideChase();
			}
			else
			{//If we're not chasing him, we need to lose him
				//NOTE: since we no longer have bStates, really, this logic doesn't work, so never give him up

				/*
				switch( NPCInfo->behaviorState )
				{
				case BS_HUNT_AND_KILL:
					//Okay to lose PVS, we're chasing them
					break;
				case BS_RUN_AND_SHOOT:
				//FIXME: only do this if !(NPCInfo->scriptFlags&SCF_CHASE_ENEMY)
					//If he's not our goalEntity, we're running somewhere else, so lose him
					if ( NPC->enemy != NPCInfo->goalEntity )
					{
						G_ClearEnemy( NPC );
					}
					break;
				default:
					//We're not chasing him, so lose him as an enemy
					G_ClearEnemy( NPC );
					break;
				}
				*/
			}
		}
	}

	if ( NPC->enemy )
	{
		if ( NPC->enemy->health <= 0 || NPC->enemy->flags & FL_NOTARGET )
		{
			if ( setEnemy )
			{
				G_ClearEnemy( NPC );
			}
		}
	}

	closestTo = NPC;
	//FIXME: check your defendEnt, if you have one, see if their enemy is different 
	//than yours, or, if they don't have one, pick the closest enemy to THEM?
	if ( NPCInfo->defendEnt )
	{//Trying to protect someone
		if ( NPCInfo->defendEnt->health > 0 )
		{//Still alive, We presume we're close to them, navigation should handle this?
			if ( NPCInfo->defendEnt->enemy )
			{//They were shot or acquired an enemy
				if ( NPC->enemy != NPCInfo->defendEnt->enemy )
				{//They have a different enemy, take it!
					newEnemy = NPCInfo->defendEnt->enemy;
					if ( setEnemy )
					{
						G_SetEnemy( NPC, NPCInfo->defendEnt->enemy );
					}
				}
			}
			else if ( NPC->enemy == NULL )
			{//We don't have an enemy, so find closest to defendEnt
				closestTo = NPCInfo->defendEnt;
			}
		}
	}

	if (!NPC->enemy || ( NPC->enemy && NPC->enemy->health <= 0 ) || forcefindNew )
	{//FIXME: NPCs that are moving after an enemy should ignore the can't hit enemy counter- that should only be for NPCs that are standing still
		//NOTE: cantHitEnemyCounter >= 100 means we couldn't hit enemy for a full
		//	10 seconds, so give up.  This means even if we're chasing him, we would
		//	try to find another enemy after 10 seconds (assuming the cantHitEnemyCounter
		//	is allowed to increment in a chasing bState)
		qboolean	foundenemy = qfalse;

		if(!findNew)
		{
			if ( setEnemy )
			{
				NPC->lastEnemy = NPC->enemy;
				G_ClearEnemy(NPC);
			}
			return NULL;
		}

		//If enemy dead or unshootable, look for others on out enemy's team
		if ( NPC->client->enemyTeam )
		{
			//NOTE:  this only checks vis if can't hit enemy for 10 tries, which I suppose
			//			means they need to find one that in more than just PVS
			//newenemy = NPC_PickEnemy( closestTo, NPC->client->enemyTeam, (NPC->cantHitEnemyCounter > 10), qfalse, qtrue );//3rd parm was (NPC->enemyTeam == TEAM_STARFLEET)
			//For now, made it so you ALWAYS have to check VIS
			newEnemy = NPC_PickEnemy( closestTo, NPC->client->enemyTeam, qtrue, qfalse, qtrue );//3rd parm was (NPC->enemyTeam == TEAM_STARFLEET)
			if ( newEnemy )
			{
				foundenemy = qtrue;
				if ( setEnemy )
				{
					G_SetEnemy( NPC, newEnemy );
				}
			}
		}
		
		if ( !forcefindNew )
		{
			if ( !foundenemy )
			{
				if ( setEnemy )
				{
					NPC->lastEnemy = NPC->enemy;
					G_ClearEnemy(NPC);
				}
			}
			
			NPC->cantHitEnemyCounter = 0;
		}
		//FIXME: if we can't find any at all, go into INdependant NPC AI, pursue and kill
	}

	if ( NPC->enemy && NPC->enemy->client ) 
	{
		if(NPC->enemy->client->playerTeam)
		{
//			assert( NPC->client->playerTeam != NPC->enemy->client->playerTeam);
			if( NPC->client->playerTeam != NPC->enemy->client->playerTeam )
			{
				NPC->client->enemyTeam = NPC->enemy->client->playerTeam;
			}
		}
	}
	return newEnemy;
}

/*
-------------------------
NPC_ClearShot
-------------------------
*/

qboolean NPC_ClearShot( gentity_t *ent )
{
	if ( ( NPC == NULL ) || ( ent == NULL ) )
		return qfalse;

	vec3_t	muzzle;
	trace_t	tr;

	CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );

	// add aim error
	// use weapon instead of specific npc types, although you could add certain npc classes if you wanted
//	if ( NPC->client->playerTeam == TEAM_SCAVENGERS )
	if( NPC->s.weapon == WP_BLASTER || NPC->s.weapon == WP_BLASTER_PISTOL ) // any other guns to check for?
	{
		vec3_t	mins = { -2, -2, -2 };
		vec3_t	maxs = {  2,  2,  2 };

		gi.trace ( &tr, muzzle, mins, maxs, ent->currentOrigin, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
	}
	else
	{
		gi.trace ( &tr, muzzle, NULL, NULL, ent->currentOrigin, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
	}
	
	if ( tr.startsolid || tr.allsolid )
	{
		return qfalse;
	}

	if ( tr.entityNum == ent->s.number ) 
		return qtrue;
	
	return qfalse;
}

/*
-------------------------
NPC_ShotEntity
-------------------------
*/

int NPC_ShotEntity( gentity_t *ent, vec3_t impactPos )
{
	if ( ( NPC == NULL ) || ( ent == NULL ) )
		return qfalse;

	vec3_t	muzzle;
	vec3_t targ;
	trace_t	tr;

	if ( NPC->s.weapon == WP_THERMAL )
	{//thermal aims from slightly above head
		//FIXME: what about low-angle shots, rolling the thermal under something?
		vec3_t	angles, forward, end;

		CalcEntitySpot( NPC, SPOT_HEAD, muzzle );
		VectorSet( angles, 0, NPC->client->ps.viewangles[1], 0 );
		AngleVectors( angles, forward, NULL, NULL );
		VectorMA( muzzle, 8, forward, end );
		end[2] += 24;
		gi.trace ( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
		VectorCopy( tr.endpos, muzzle );
	}
	else
	{
		CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
	}
	CalcEntitySpot( ent, SPOT_CHEST, targ );
	
	// add aim error
	// use weapon instead of specific npc types, although you could add certain npc classes if you wanted
//	if ( NPC->client->playerTeam == TEAM_SCAVENGERS )
	if( NPC->s.weapon == WP_BLASTER || NPC->s.weapon == WP_BLASTER_PISTOL ) // any other guns to check for?
	{
		vec3_t	mins = { -2, -2, -2 };
		vec3_t	maxs = {  2,  2,  2 };

		gi.trace ( &tr, muzzle, mins, maxs, targ, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
	}
	else
	{
		gi.trace ( &tr, muzzle, NULL, NULL, targ, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
	}
	//FIXME: if using a bouncing weapon like the bowcaster, should we check the reflection of the wall, too?
	if ( impactPos )
	{//they want to know *where* the hit would be, too
		VectorCopy( tr.endpos, impactPos );
	}
/* // NPCs should be able to shoot even if the muzzle would be inside their target
	if ( tr.startsolid || tr.allsolid )
	{
		return ENTITYNUM_NONE;
	}
*/
	return tr.entityNum;
}

qboolean NPC_EvaluateShot( int hit, qboolean glassOK )
{
	if ( !NPC->enemy )
	{
		return qfalse;
	}

	if ( hit == NPC->enemy->s.number || (&g_entities[hit] != NULL && (g_entities[hit].svFlags&SVF_GLASS_BRUSH)) )
	{//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

/*
NPC_CheckAttack

Simply checks aggression and returns true or false
*/

qboolean NPC_CheckAttack (float scale)
{
	if(!scale)
		scale = 1.0;

	if(((float)NPCInfo->stats.aggression) * scale < Q_flrand(0, 4))
	{
		return qfalse;
	}

	if(NPCInfo->shotTime > level.time)
		return qfalse;

	return qtrue;
}

/*
NPC_CheckDefend

Simply checks evasion and returns true or false
*/

qboolean NPC_CheckDefend (float scale)
{
	if(!scale)
		scale = 1.0;

	if((float)(NPCInfo->stats.evasion) > random() * 4 * scale)
		return qtrue;

	return qfalse;
}


//NOTE: BE SURE TO CHECK PVS BEFORE THIS!
qboolean NPC_CheckCanAttack (float attack_scale, qboolean stationary)
{
	vec3_t		delta, forward;
	vec3_t		angleToEnemy;
	vec3_t		hitspot, muzzle, diff, enemy_org;//, enemy_head;
	float		distanceToEnemy;
	qboolean	attack_ok = qfalse;
//	qboolean	duck_ok = qfalse;
	qboolean	dead_on = qfalse;
	float		aim_off;
	float		max_aim_off = 128 - (16 * (float)NPCInfo->stats.aim);
	trace_t		tr;
	gentity_t	*traceEnt = NULL;

	if(NPC->enemy->flags & FL_NOTARGET)
	{
		return qfalse;
	}

	//FIXME: only check to see if should duck if that provides cover from the
	//enemy!!!
	if(!attack_scale)
	{
		attack_scale = 1.0;
	}
	//Yaw to enemy
	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org );
	NPC_AimWiggle( enemy_org );

	CalcEntitySpot( NPC, SPOT_WEAPON, muzzle );
	
	VectorSubtract (enemy_org, muzzle, delta);
	vectoangles ( delta, angleToEnemy );
	distanceToEnemy = VectorNormalize(delta);

	NPC->NPC->desiredYaw = angleToEnemy[YAW];
	NPC_UpdateFiringAngles(qfalse, qtrue);

	if( NPC_EnemyTooFar(NPC->enemy, distanceToEnemy*distanceToEnemy, qtrue) )
	{//Too far away?  Do not attack
		return qfalse;
	}

	if(client->fireDelay > 0)
	{//already waiting for a shot to fire
		NPC->NPC->desiredPitch = angleToEnemy[PITCH];
		NPC_UpdateFiringAngles(qtrue, qfalse);
		return qfalse;
	}

	if(NPCInfo->scriptFlags & SCF_DONT_FIRE)
	{
		return qfalse;
	}

	NPCInfo->enemyLastVisibility = enemyVisibility;
	//See if they're in our FOV and we have a clear shot to them
	enemyVisibility = NPC_CheckVisibility ( NPC->enemy, CHECK_360|CHECK_FOV);////CHECK_PVS|

	if(enemyVisibility >= VIS_FOV)
	{//He's in our FOV
		
		attack_ok = qtrue;
		//CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_head);

		//Check to duck
		if ( NPC->enemy->client )
		{
			if ( NPC->enemy->enemy == NPC )
			{
				if ( NPC->enemy->client->buttons & BUTTON_ATTACK )
				{//FIXME: determine if enemy fire angles would hit me or get close
					if ( NPC_CheckDefend( 1.0 ) )//FIXME: Check self-preservation?  Health?
					{//duck and don't shoot
						attack_ok = qfalse;
						ucmd.upmove = -127;
					}
				}
			}
		}

		if(attack_ok)
		{
			//are we gonna hit him
			//NEW: use actual forward facing
			AngleVectors( client->ps.viewangles, forward, NULL, NULL );
			VectorMA( muzzle, distanceToEnemy, forward, hitspot );
			gi.trace( &tr, muzzle, NULL, NULL, hitspot, NPC->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );
			ShotThroughGlass( &tr, NPC->enemy, hitspot, MASK_SHOT );
			/*
			//OLD: trace regardless of facing
			gi.trace ( &tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_SHOT );
			ShotThroughGlass(&tr, NPC->enemy, enemy_org, MASK_SHOT);
			*/

			traceEnt = &g_entities[tr.entityNum];

			/*
			if( traceEnt != NPC->enemy &&//FIXME: if someone on our team is in the way, suggest that they duck if possible
				(!traceEnt || !traceEnt->client || !NPC->client->enemyTeam || NPC->client->enemyTeam != traceEnt->client->playerTeam) )
			{//no, so shoot for somewhere between the head and torso
				//NOTE: yes, I know this looks weird, but it works
				enemy_org[0] += 0.3*Q_flrand(NPC->enemy->mins[0], NPC->enemy->maxs[0]);
				enemy_org[1] += 0.3*Q_flrand(NPC->enemy->mins[1], NPC->enemy->maxs[1]);
				enemy_org[2] -= NPC->enemy->maxs[2]*Q_flrand(0.0f, 1.0f);

				attack_scale *= 0.75;
				gi.trace ( &tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_SHOT );
				ShotThroughGlass(&tr, NPC->enemy, enemy_org, MASK_SHOT);
				traceEnt = &g_entities[tr.entityNum];
			}
			*/

			VectorCopy( tr.endpos, hitspot );

			if( traceEnt == NPC->enemy || (traceEnt->client && NPC->client->enemyTeam && NPC->client->enemyTeam == traceEnt->client->playerTeam) )
			{
				dead_on = qtrue;
			}
			else
			{
				attack_scale *= 0.5;
				if(NPC->client->playerTeam)
				{
					if(traceEnt && traceEnt->client && traceEnt->client->playerTeam)
					{
						if(NPC->client->playerTeam == traceEnt->client->playerTeam)
						{//Don't shoot our own team
							attack_ok = qfalse;
						}
					}
				}
			}
		}

		if( attack_ok )
		{
			//ok, now adjust pitch aim
			VectorSubtract (hitspot, muzzle, delta);
			vectoangles ( delta, angleToEnemy );
			NPC->NPC->desiredPitch = angleToEnemy[PITCH];
			NPC_UpdateFiringAngles(qtrue, qfalse);

			if( !dead_on )
			{//We're not going to hit him directly, try a suppressing fire
				//see if where we're going to shoot is too far from his origin
				if(traceEnt && (traceEnt->health <= 30 || EntIsGlass(traceEnt)))
				{//easy to kill - go for it
					if(traceEnt->e_DieFunc == dieF_ExplodeDeath_Wait && traceEnt->splashDamage)
					{//going to explode, don't shoot if close to self
						VectorSubtract(NPC->currentOrigin, traceEnt->currentOrigin, diff);
						if(VectorLengthSquared(diff) < traceEnt->splashRadius*traceEnt->splashRadius)
						{//Too close to shoot!
							attack_ok = qfalse;
						}
						else 
						{//Hey, it might kill him, do it!
							attack_scale *= 2;//
						}
					}
				}
				else
				{
					AngleVectors (client->ps.viewangles, forward, NULL, NULL);
					VectorMA ( muzzle, distanceToEnemy, forward, hitspot);
					VectorSubtract(hitspot, enemy_org, diff);
					aim_off = VectorLength(diff);
					if(aim_off > random() * max_aim_off)//FIXME: use aim value to allow poor aim?
					{
						attack_scale *= 0.75;
						//see if where we're going to shoot is too far from his head
						VectorSubtract(hitspot, enemy_org, diff);
						aim_off = VectorLength(diff);
						if(aim_off > random() * max_aim_off)
						{
							attack_ok = qfalse;
						}
					}
					attack_scale *= (max_aim_off - aim_off + 1)/max_aim_off;
				}
			}
		}
	}
	else
	{//Update pitch anyway
		NPC->NPC->desiredPitch = angleToEnemy[PITCH];
		NPC_UpdateFiringAngles(qtrue, qfalse);
	}

	if( attack_ok )
	{
		if( NPC_CheckAttack( attack_scale ))
		{//check aggression to decide if we should shoot
			enemyVisibility = VIS_SHOOT;
			WeaponThink(qtrue);
		}
		else
			attack_ok = qfalse;
	}

	return attack_ok;
}
//========================================================================================
//OLD id-style hunt and kill
//========================================================================================
/*
IdealDistance

determines what the NPC's ideal distance from it's enemy should
be in the current situation
*/
float IdealDistance ( gentity_t *self ) 
{
	float	ideal;

	ideal = 225 - 20 * NPCInfo->stats.aggression;
	switch ( NPC->s.weapon ) 
	{
	case WP_ROCKET_LAUNCHER:
		ideal += 200;
		break;

	case WP_THERMAL:
		ideal += 50;
		break;

/*	case WP_TRICORDER:
		ideal = 0;
		break;
*/
	case WP_SABER:
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
	case WP_BLASTER:
	default:
		break;
	}

	return ideal;
}

/*QUAKED point_combat (0.7 0 0.7) (-16 -16 -24) (16 16 32) DUCK FLEE INVESTIGATE SQUAD LEAN SNIPE
NPCs in bState BS_COMBAT_POINT will find their closest empty combat_point

DUCK - NPC will duck and fire from this point, NOT IMPLEMENTED?
FLEE - Will choose this point when running
INVESTIGATE - Will look here if a sound is heard near it
SQUAD - NOT IMPLEMENTED
LEAN - Lean-type cover, NOT IMPLEMENTED
SNIPE - Snipers look for these first, NOT IMPLEMENTED
*/

void SP_point_combat( gentity_t *self )
{
	if(level.numCombatPoints >= MAX_COMBAT_POINTS)
	{
#ifndef FINAL_BUILD
		gi.Printf(S_COLOR_RED"ERROR:  Too many combat points, limit is %d\n", MAX_COMBAT_POINTS);
#endif
		G_FreeEntity(self);
		return;
	}

	self->s.origin[2] += 0.125;
	G_SetOrigin(self, self->s.origin);
	gi.linkentity(self);

	if ( G_CheckInSolid( self, qtrue ) )
	{
#ifndef FINAL_BUILD
		gi.Printf( S_COLOR_RED"ERROR: combat point at %s in solid!\n", vtos(self->currentOrigin) );
#endif
	}

	VectorCopy( self->currentOrigin, level.combatPoints[level.numCombatPoints].origin );
	
	level.combatPoints[level.numCombatPoints].flags = self->spawnflags;
	level.combatPoints[level.numCombatPoints].occupied = qfalse;

	level.numCombatPoints++;

	G_FreeEntity(self);
};

void CP_FindCombatPointWaypoints( void )
{
	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		level.combatPoints[i].waypoint = NAV_FindClosestWaypointForPoint( level.combatPoints[i].origin );
#ifndef FINAL_BUILD
		if ( level.combatPoints[i].waypoint == WAYPOINT_NONE )
		{
			gi.Printf( S_COLOR_RED"ERROR: Combat Point at %s has no waypoint!\n", vtos(level.combatPoints[i].origin) );
		}
#endif
	}
}


/*
-------------------------
NPC_CollectCombatPoints
-------------------------
*/

typedef	map< float, int >	combatPoint_m;

static int NPC_CollectCombatPoints( const vec3_t origin, const float radius, combatPoint_m &points, const int flags )
{
	float	radiusSqr = (radius*radius);
	float	distance;

	//Collect all nearest
	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		//Must be vacant
		if ( level.combatPoints[i].occupied == (int) qtrue )
			continue;

		//If we want a duck space, make sure this is one
		if ( ( flags & CP_DUCK ) && ( level.combatPoints[i].flags & CPF_DUCK ) )
			continue;

		//If we want a duck space, make sure this is one
		if ( ( flags & CP_FLEE ) && ( level.combatPoints[i].flags & CPF_FLEE ) )
			continue;

		///Make sure this is an investigate combat point
		if ( ( flags & CP_INVESTIGATE ) && ( level.combatPoints[i].flags & CPF_INVESTIGATE ) )
			continue;
		
		//Squad points are only valid if we're looking for them
		if ( ( level.combatPoints[i].flags & CPF_SQUAD ) && ( ( flags & CP_SQUAD ) == qfalse ) )
			continue;

		if ( flags&CP_NO_PVS )
		{//must not be within PVS of mu current origin
			if ( gi.inPVS( origin, level.combatPoints[i].origin ) )
			{
				continue;
			}
		}

		if ( flags&CP_HORZ_DIST_COLL )
		{
			distance = 	DistanceHorizontalSquared( origin, level.combatPoints[i].origin );
		}
		else
		{
			distance = 	DistanceSquared( origin, level.combatPoints[i].origin );
		}

		if ( distance < radiusSqr )
		{
			//Using a map will sort nearest automatically
			points[ distance ] = i;
		}
	}

	return points.size();
}

/*
-------------------------
NPC_FindCombatPoint
-------------------------
*/

#define	MIN_AVOID_DOT				0.75f
#define MIN_AVOID_DISTANCE			128
#define MIN_AVOID_DISTANCE_SQUARED	( MIN_AVOID_DISTANCE * MIN_AVOID_DISTANCE )
#define	CP_COLLECT_RADIUS			512.0f

int NPC_FindCombatPoint( const vec3_t position, const vec3_t avoidPosition, vec3_t enemyPosition, const int flags, float avoidDist, const int ignorePoint )
{
	combatPoint_m	points;
	int				best = -1, cost, bestCost = Q3_INFINITE, waypoint = WAYPOINT_NONE;
	float			dist;
	trace_t			tr;
	float			collRad = CP_COLLECT_RADIUS;

	if ( avoidDist <= 0 )
	{
		avoidDist = MIN_AVOID_DISTANCE_SQUARED;
	}
	else
	{
		avoidDist *= avoidDist;
	}

	if ( (flags & CP_HAS_ROUTE) || (flags & CP_NEAREST) )
	{//going to be doing macro nav tests
		if ( NPC->waypoint == WAYPOINT_NONE )
		{
			waypoint = NAV_GetNearestNode( NPC, NPC->lastWaypoint );
		}
		else
		{
			waypoint = NPC->waypoint;
		}
	}

	//Collect our nearest points
	if ( flags & CP_NO_PVS )
	{//much larger radius since most will be dropped?
		collRad = CP_COLLECT_RADIUS*4;
	}
	NPC_CollectCombatPoints( enemyPosition, collRad, points, flags );//position

	combatPoint_m::iterator	cpi;
	STL_ITERATE( cpi, points )
	{
		const int i = (*cpi).second;

		//Must not be one we want to ignore
		if ( i == ignorePoint )
			continue;

		//FIXME: able to mark certain ones as too dangerous to go to for now?  Like a tripmine/thermal/detpack is near or something?
		//If we need a cover point, check this point
		if ( ( flags & CP_COVER ) && ( NPC_ClearLOS( level.combatPoints[i].origin, enemyPosition ) == qtrue ) )//Used to use NPC->enemy
			continue;

		//Need a clear LOS to our target... and be within shot range to enemy position (FIXME: make this a separate CS_ flag? and pass in a range?)
		if ( flags & CP_CLEAR )
		{
			if ( NPC_ClearLOS( level.combatPoints[i].origin, NPC->enemy ) == qfalse )
			{
				continue;
			}
			if ( NPC->s.weapon == WP_THERMAL )
			{//horizontal
				dist = DistanceHorizontalSquared( level.combatPoints[i].origin, NPC->enemy->currentOrigin );
			}
			else
			{//actual
				dist = DistanceSquared( level.combatPoints[i].origin, NPC->enemy->currentOrigin );
			}
			if ( dist > (NPCInfo->stats.visrange*NPCInfo->stats.visrange) )
			{
				continue;
			}
		}

		//Avoid this position?
		if ( ( flags & CP_AVOID ) && ( DistanceSquared( level.combatPoints[i].origin, position ) < avoidDist ) )//was using MIN_AVOID_DISTANCE_SQUARED, not passed in avoidDist
			continue;

		//Try to find a point closer to the enemy than where we are
		if ( flags & CP_APPROACH_ENEMY )
		{
			if ( flags&CP_HORZ_DIST_COLL )
			{
				if ( (*cpi).first > DistanceHorizontalSquared( position, enemyPosition ) )
				{
					continue;
				}
			}
			else 
			{
				if ( (*cpi).first > DistanceSquared( position, enemyPosition ) )
				{
					continue;
				}
			}
		}
		//Try to find a point farther from the enemy than where we are
		if ( flags & CP_RETREAT )
		{
			if ( flags&CP_HORZ_DIST_COLL )
			{
				if ( (*cpi).first < DistanceHorizontalSquared( position, enemyPosition ) )
				{//it's closer, don't use it
					continue;
				}
			}
			else
			{
				if ( (*cpi).first < DistanceSquared( position, enemyPosition ) )
				{//it's closer, don't use it
					continue;
				}
			}
		}

		//We want a point on other side of the enemy from current pos
		if ( flags & CP_FLANK  )
		{
			vec3_t	eDir2Me, eDir2CP;
			
			VectorSubtract( position, enemyPosition, eDir2Me );
			VectorNormalize( eDir2Me );

			VectorSubtract( level.combatPoints[i].origin, enemyPosition, eDir2CP );
			VectorNormalize( eDir2CP );

			const float	dot = DotProduct( eDir2Me, eDir2CP );
			
			//Not far enough behind enemy from current pos
			if ( dot >= 0.4 )
				continue;
		}

		//See if we're trying to avoid our enemy
		//FIXME: this needs to check for the waypoint you'll be taking to get to that combat point
		if ( flags & CP_AVOID_ENEMY  )
		{
			vec3_t	eDir, gDir;
			vec3_t	wpOrg;
			
			VectorSubtract( position, enemyPosition, eDir );
			VectorNormalize( eDir );

			/*
			NAV_FindClosestWaypointForEnt( NPC, level.combatPoints[i].waypoint );
			if ( NPC->waypoint != WAYPOINT_NONE && NPC->waypoint != level.combatPoints[i].waypoint )
			{
				navigator.GetNodePosition( NPC->waypoint, wpOrg );
			}
			else
			*/
			{
				VectorCopy( level.combatPoints[i].origin, wpOrg );
			}
			VectorSubtract( position, wpOrg, gDir );
			VectorNormalize( gDir );

			float	dot = DotProduct( gDir, eDir );
			
			//Don't want to run at enemy
			if ( dot >= MIN_AVOID_DOT )
				continue;

			//Can't be too close to the enemy
			if ( DistanceSquared( wpOrg, enemyPosition ) < avoidDist )
				continue;
		}
		
		//Okay, now make sure it's not blocked
		gi.trace( &tr, level.combatPoints[i].origin, NPC->mins, NPC->maxs, level.combatPoints[i].origin, NPC->s.number, NPC->clipmask, G2_NOCOLLIDE, 0 );
		if ( tr.allsolid || tr.startsolid )
		{
			continue;
		}

		//we must have a route to the combat point
		if ( flags & CP_HAS_ROUTE )
		{
			/*
			if ( level.combatPoints[i].waypoint == WAYPOINT_NONE )
			{
				level.combatPoints[i].waypoint = NAV_FindClosestWaypointForPoint( level.combatPoints[i].origin );
			}
			*/

			if ( waypoint == WAYPOINT_NONE || level.combatPoints[i].waypoint == WAYPOINT_NONE || navigator.GetBestNodeAltRoute( waypoint, level.combatPoints[i].waypoint ) == WAYPOINT_NONE )
			{//can't possibly have a route to any OR can't possibly have a route to this one OR don't have a route to this one
				if ( !NAV_ClearPathToPoint( NPC, NPC->mins, NPC->maxs, level.combatPoints[i].origin, NPC->clipmask, ENTITYNUM_NONE ) )
				{//don't even have a clear straight path to this one
					continue;
				}
			}
		}

		//We want the one with the shortest path from current pos
		if ( flags & CP_NEAREST && waypoint != WAYPOINT_NONE && level.combatPoints[i].waypoint != WAYPOINT_NONE )
		{
			cost = navigator.GetPathCost( waypoint, level.combatPoints[i].waypoint );
			if ( cost < bestCost )
			{
				bestCost = cost;
				best = i;
			}
			continue;
		}

		//we want the combat point closest to the enemy
		//if ( flags & CP_CLOSEST )
		//they are sorted by this distance, so the first one to get this far is the closest
		return i;
	}

	return best;
}

/*
-------------------------
NPC_FindSquadPoint
-------------------------
*/

int NPC_FindSquadPoint( vec3_t position )
{
	float	dist, nearestDist = (float)WORLD_SIZE*(float)WORLD_SIZE;
	int		nearestPoint = -1;

	//float			playerDist = DistanceSquared( g_entities[0].currentOrigin, NPC->currentOrigin );

	for ( int i = 0; i < level.numCombatPoints; i++ )
	{
		//Squad points are only valid if we're looking for them
		if ( ( level.combatPoints[i].flags & CPF_SQUAD ) == qfalse )
			continue;

		//Must be vacant
		if ( level.combatPoints[i].occupied == qtrue )
			continue;
		
		dist = DistanceSquared( position, level.combatPoints[i].origin );

		//The point cannot take us past the player
		//if ( dist > ( playerDist * DotProduct( dirToPlayer, playerDir ) ) )	//FIXME: Retain this
		//	continue;

		//See if this is closer than the others
		if ( dist < nearestDist )
		{
			nearestPoint = i;
			nearestDist = dist;
		}
	}

	return nearestPoint;
}

/*
-------------------------
NPC_ReserveCombatPoint
-------------------------
*/

qboolean NPC_ReserveCombatPoint( int combatPointID )
{
	//Make sure it's valid
	if ( combatPointID > level.numCombatPoints )
		return qfalse;

	//Make sure it's not already occupied
	if ( level.combatPoints[combatPointID].occupied )
		return qfalse;

	//Reserve it
	level.combatPoints[combatPointID].occupied = qtrue;

	return qtrue;
}

/*
-------------------------
NPC_FreeCombatPoint
-------------------------
*/

qboolean NPC_FreeCombatPoint( int combatPointID, qboolean failed )
{
	if ( failed )
	{//remember that this one failed for us
		NPCInfo->lastFailedCombatPoint = combatPointID;
	}
	//Make sure it's valid
	if ( combatPointID > level.numCombatPoints )
		return qfalse;

	//Make sure it's currently occupied
	if ( level.combatPoints[combatPointID].occupied == qfalse )
		return qfalse;

	//Free it
	level.combatPoints[combatPointID].occupied = qfalse;
	
	return qtrue;
}

/*
-------------------------
NPC_SetCombatPoint
-------------------------
*/

qboolean NPC_SetCombatPoint( int combatPointID )
{
	//Free a combat point if we already have one
	if ( NPCInfo->combatPoint != -1 )
	{
		NPC_FreeCombatPoint( NPCInfo->combatPoint );
	}

	if ( NPC_ReserveCombatPoint( combatPointID ) == qfalse )
		return qfalse;

	NPCInfo->combatPoint = combatPointID;

	return qtrue;
}

extern qboolean CheckItemCanBePickedUpByNPC( gentity_t *item, gentity_t *pickerupper );
gentity_t *NPC_SearchForWeapons( void )
{
	gentity_t *found = g_entities, *bestFound = NULL;
	float		dist, bestDist = Q3_INFINITE;
	int i;
//	for ( found = g_entities; found < &g_entities[globals.num_entities] ; found++)
	for ( i = 0; i<globals.num_entities; i++)
	{
//		if ( !found->inuse )
//		{
//			continue;
//		}
		if(!PInUse(i))
			continue;
		
		found=&g_entities[i];
		
		//FIXME: Also look for ammo_racks that have weapons on them?
		if ( found->s.eType != ET_ITEM )
		{
			continue;
		}
		if ( found->item->giType != IT_WEAPON )
		{
			continue;
		}
		if ( found->s.eFlags & EF_NODRAW )
		{
			continue;
		}
		if ( CheckItemCanBePickedUpByNPC( found, NPC ) )
		{
			if ( gi.inPVS( found->currentOrigin, NPC->currentOrigin ) )
			{
				dist = DistanceSquared( found->currentOrigin, NPC->currentOrigin );
				if ( dist < bestDist )
				{
					if ( !navigator.GetBestPathBetweenEnts( NPC, found, NF_CLEAR_PATH ) 
						|| navigator.GetBestNodeAltRoute( NPC->waypoint, found->waypoint ) == WAYPOINT_NONE )
					{//can't possibly have a route to any OR can't possibly have a route to this one OR don't have a route to this one
						if ( NAV_ClearPathToPoint( NPC, NPC->mins, NPC->maxs, found->currentOrigin, NPC->clipmask, ENTITYNUM_NONE ) )
						{//have a clear straight path to this one
							bestDist = dist;
							bestFound = found;
						}
					}
					else
					{//can nav to it
						bestDist = dist;
						bestFound = found;
					}
				}
			}
		}
	}

	return bestFound;
}

void NPC_SetPickUpGoal( gentity_t *foundWeap )
{
	vec3_t org;

	//NPCInfo->goalEntity = foundWeap;
	VectorCopy( foundWeap->currentOrigin, org );
	org[2] += 24 - (foundWeap->mins[2]*-1);//adjust the origin so that I am on the ground
	NPC_SetMoveGoal( NPC, org, foundWeap->maxs[0]*0.75, qfalse, -1, foundWeap );
	NPCInfo->tempGoal->waypoint = foundWeap->waypoint;
	NPCInfo->tempBehavior = BS_DEFAULT;
	NPCInfo->squadState = SQUAD_TRANSITION;
}

void NPC_CheckGetNewWeapon( void )
{
	if ( NPC->s.weapon == WP_NONE && NPC->enemy )
	{//if running away because dropped weapon...
		if ( NPCInfo->goalEntity 
			&& NPCInfo->goalEntity == NPCInfo->tempGoal
			&& NPCInfo->goalEntity->enemy
			&& !NPCInfo->goalEntity->enemy->inuse )
		{//maybe was running at a weapon that was picked up
			NPCInfo->goalEntity = NULL;
		}
		if ( TIMER_Done( NPC, "panic" ) && NPCInfo->goalEntity == NULL )
		{//need a weapon, any lying around?
			gentity_t *foundWeap = NPC_SearchForWeapons();
			if ( foundWeap )
			{//try to nav to it
				/*
				if ( !navigator.GetBestPathBetweenEnts( NPC, foundWeap, NF_CLEAR_PATH ) 
					|| navigator.GetBestNodeAltRoute( NPC->waypoint, foundWeap->waypoint ) == WAYPOINT_NONE )
				{//can't possibly have a route to any OR can't possibly have a route to this one OR don't have a route to this one
					if ( !NAV_ClearPathToPoint( NPC, NPC->mins, NPC->maxs, foundWeap->currentOrigin, NPC->clipmask, ENTITYNUM_NONE ) )
					{//don't even have a clear straight path to this one
					}
					else
					{
						NPC_SetPickUpGoal( foundWeap );
					}
				}
				else
				*/
				{
					NPC_SetPickUpGoal( foundWeap );
				}
			}
		}
	}
}

void NPC_AimAdjust( int change )
{
	if ( !TIMER_Exists( NPC, "aimDebounce" ) )
	{
		int debounce = 500+(3-g_spskill->integer)*100;
		TIMER_Set( NPC, "aimDebounce", Q_irand( debounce,debounce+1000 ) );
		//int debounce = 1000+(3-g_spskill->integer)*500;
		//TIMER_Set( NPC, "aimDebounce", Q_irand( debounce, debounce+2000 ) );
		return;
	}
	if ( TIMER_Done( NPC, "aimDebounce" ) )
	{
		NPCInfo->currentAim += change;
		if ( NPCInfo->currentAim > NPCInfo->stats.aim )
		{//can never be better than max aim
			NPCInfo->currentAim = NPCInfo->stats.aim;
		}
		else if ( NPCInfo->currentAim < -30 )
		{//can never be worse than this
			NPCInfo->currentAim = -30;
		}

		//Com_Printf( "%s new aim = %d\n", NPC->NPC_type, NPCInfo->currentAim );

		int debounce = 500+(3-g_spskill->integer)*100;
		TIMER_Set( NPC, "aimDebounce", Q_irand( debounce,debounce+1000 ) );
		//int debounce = 1000+(3-g_spskill->integer)*500;
		//TIMER_Set( NPC, "aimDebounce", Q_irand( debounce, debounce+2000 ) );
	}
}

void G_AimSet( gentity_t *self, int aim )
{
	if ( self->NPC )
	{
		self->NPC->currentAim = aim;
		//Com_Printf( "%s new aim = %d\n", self->NPC_type, self->NPC->currentAim );

		int debounce = 500+(3-g_spskill->integer)*100;
		TIMER_Set( self, "aimDebounce", Q_irand( debounce,debounce+1000 ) );
	//	int debounce = 1000+(3-g_spskill->integer)*500;
	//	TIMER_Set( self, "aimDebounce", Q_irand( debounce,debounce+2000 ) );
	}
}
