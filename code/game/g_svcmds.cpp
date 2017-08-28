/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "../cgame/cg_local.h"
#include "Q3_Interface.h"

#include "g_local.h"
#include "wp_saber.h"
#include "g_functions.h"

extern void G_NextTestAxes( void );
extern void G_ChangePlayerModel( gentity_t *ent, const char *newModel );
extern void G_InitPlayerFromCvars( gentity_t *ent );
extern void Q3_SetViewEntity(int entID, const char *name);
extern qboolean G_ClearViewEntity( gentity_t *ent );
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );

extern void WP_SetSaber( gentity_t *ent, int saberNum, const char *saberName );
extern void WP_RemoveSaber( gentity_t *ent, int saberNum );
extern saber_colors_t TranslateSaberColor( const char *name );
extern qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );
extern qboolean WP_UseFirstValidSaberStyle( gentity_t *ent, int *saberAnimLevel );

extern void G_SetWeapon( gentity_t *self, int wp );
extern stringID_table_t WPTable[];

extern cvar_t	*g_char_model;
extern cvar_t	*g_char_skin_head;
extern cvar_t	*g_char_skin_torso;
extern cvar_t	*g_char_skin_legs;
extern cvar_t	*g_char_color_red;
extern cvar_t	*g_char_color_green;
extern cvar_t	*g_char_color_blue;
extern cvar_t	*g_saber;
extern cvar_t	*g_saber2;
extern cvar_t	*g_saber_color;
extern cvar_t	*g_saber2_color;

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities;
	for (e = 0; e < globals.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		gi.Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			gi.Printf( "ET_GENERAL          " );
			break;
		case ET_PLAYER:
			gi.Printf( "ET_PLAYER           " );
			break;
		case ET_ITEM:
			gi.Printf( "ET_ITEM             " );
			break;
		case ET_MISSILE:
			gi.Printf( "ET_MISSILE          " );
			break;
		case ET_MOVER:
			gi.Printf( "ET_MOVER            " );
			break;
		case ET_BEAM:
			gi.Printf( "ET_BEAM             " );
			break;
		case ET_PORTAL:
			gi.Printf( "ET_PORTAL           " );
			break;
		case ET_SPEAKER:
			gi.Printf( "ET_SPEAKER          " );
			break;
		case ET_PUSH_TRIGGER:
			gi.Printf( "ET_PUSH_TRIGGER     " );
			break;
		case ET_TELEPORT_TRIGGER:
			gi.Printf( "ET_TELEPORT_TRIGGER " );
			break;
		case ET_INVISIBLE:
			gi.Printf( "ET_INVISIBLE        " );
			break;
		case ET_THINKER:
			gi.Printf( "ET_THINKER          " );
			break;
		case ET_CLOUD:
			gi.Printf( "ET_CLOUD            " );
			break;
		case ET_TERRAIN:
			gi.Printf( "ET_TERRAIN          " );
			break;
		default:
			gi.Printf( "%-3i                ", check->s.eType );
			break;
		}

		if ( check->classname ) {
			gi.Printf("%s", check->classname);
		}
		gi.Printf("\n");
	}
}

//---------------------------
extern void G_StopCinematicSkip( void );
extern void G_StartCinematicSkip( void );
extern void ExitEmplacedWeapon( gentity_t *ent );
static void Svcmd_ExitView_f( void )
{
extern cvar_t	*g_skippingcin;
	static int exitViewDebounce = 0;
	if ( exitViewDebounce > level.time )
	{
		return;
	}
	exitViewDebounce = level.time + 500;
	if ( in_camera )
	{//see if we need to exit an in-game cinematic
		if ( g_skippingcin->integer )	// already doing cinematic skip?
		{// yes...   so stop skipping...
			G_StopCinematicSkip();
		}
		else
		{// no... so start skipping...
			G_StartCinematicSkip();
		}
	}
	else if ( !G_ClearViewEntity( player ) )
	{//didn't exit control of a droid or turret
		//okay, now try exiting emplaced guns or AT-ST's
		if ( player->s.eFlags & EF_LOCKED_TO_WEAPON )
		{//get out of emplaced gun
			ExitEmplacedWeapon( player );
		}
		else if ( player->client && player->client->NPC_class == CLASS_ATST )
		{//a player trying to get out of his ATST
			GEntity_UseFunc( player->activator, player, player );
		}
	}
}

gentity_t *G_GetSelfForPlayerCmd( void )
{
	if ( g_entities[0].client->ps.viewEntity > 0
		&& g_entities[0].client->ps.viewEntity < ENTITYNUM_WORLD
		&& g_entities[g_entities[0].client->ps.viewEntity].client
		&& g_entities[g_entities[0].client->ps.viewEntity].s.weapon == WP_SABER )
	{//you're controlling another NPC
		return (&g_entities[g_entities[0].client->ps.viewEntity]);
	}
	else
	{
		return (&g_entities[0]);
	}
}

static void Svcmd_Saber_f()
{
	const char *saber = gi.argv(1);
	const char *saber2 = gi.argv(2);
	char name[MAX_CVAR_VALUE_STRING] = {0};

	if ( gi.argc() < 2 )
	{
		gi.Printf( "Usage: saber <saber1> <saber2>\n" );
		gi.Cvar_VariableStringBuffer( "g_saber", name, sizeof(name) );
		gi.Printf("g_saber is set to %s\n", name);
		gi.Cvar_VariableStringBuffer( "g_saber2", name, sizeof(name) );
		if ( name[0] )
			gi.Printf("g_saber2 is set to %s\n", name);
		return;
	}

	if ( !g_entities[0].client || !saber || !saber[0] )
	{
		return;
	}

	gi.cvar_set( "g_saber", saber );
	WP_SetSaber( &g_entities[0], 0, saber );
	if ( saber2 && saber2[0] && !(g_entities[0].client->ps.saber[0].saberFlags&SFL_TWO_HANDED) )
	{//want to use a second saber and first one is not twoHanded
		gi.cvar_set( "g_saber2", saber2 );
		WP_SetSaber( &g_entities[0], 1, saber2 );
	}
	else
	{
		gi.cvar_set( "g_saber2", "" );
		WP_RemoveSaber( &g_entities[0], 1 );
	}
}

static void Svcmd_SaberBlade_f()
{
	if ( gi.argc() < 2 )
	{
		gi.Printf( "USAGE: saberblade <sabernum> <bladenum> [0 = off, 1 = on, no arg = toggle]\n" );
		return;
	}
	if ( &g_entities[0] == NULL || g_entities[0].client == NULL )
	{
		return;
	}
	int sabernum = atoi(gi.argv(1)) - 1;
	if ( sabernum < 0 || sabernum > 1 )
	{
		return;
	}
	if ( sabernum > 0 && !g_entities[0].client->ps.dualSabers )
	{
		return;
	}
	//FIXME: what if don't even have a single saber at all?
	int bladenum = atoi(gi.argv(2)) - 1;
	if ( bladenum < 0 || bladenum >= g_entities[0].client->ps.saber[sabernum].numBlades )
	{
		return;
	}
	qboolean turnOn;
	if ( gi.argc() > 2 )
	{//explicit
		turnOn = (qboolean)(atoi(gi.argv(3))!=0);
	}
	else
	{//toggle
		turnOn = (qboolean)!g_entities[0].client->ps.saber[sabernum].blade[bladenum].active;
	}

	g_entities[0].client->ps.SaberBladeActivate( sabernum, bladenum, turnOn );
}

static void Svcmd_SaberColor_f()
{//FIXME: just list the colors, each additional listing sets that blade
	int saberNum = atoi(gi.argv(1));
	const char *color[MAX_BLADES];
	int bladeNum;

	for ( bladeNum = 0; bladeNum < MAX_BLADES; bladeNum++ )
	{
		color[bladeNum] = gi.argv(2+bladeNum);
	}

	if ( saberNum < 1 || saberNum > 2 || gi.argc() < 3 )
	{
		gi.Printf( "Usage:  saberColor <saberNum> <blade1 color> <blade2 color> ... <blade8 color>\n" );
		gi.Printf( "valid saberNums:  1 or 2\n" );
		gi.Printf( "valid colors:  red, orange, yellow, green, blue, and purple\n" );

		return;
	}
	saberNum--;

	gentity_t *self = G_GetSelfForPlayerCmd();

	for ( bladeNum = 0; bladeNum < MAX_BLADES; bladeNum++ )
	{
		if ( !color[bladeNum] || !color[bladeNum][0] )
		{
			break;
		}
		else
		{
			self->client->ps.saber[saberNum].blade[bladeNum].color = TranslateSaberColor( color[bladeNum] );
		}
	}

	if ( saberNum == 0 )
	{
		gi.cvar_set( "g_saber_color", color[0] );
	}
	else if ( saberNum == 1 )
	{
		gi.cvar_set( "g_saber2_color", color[0] );
	}
}

struct SetForceCmd {
	const char *desc;
	const char *cmdname;
	const int maxlevel;
};

SetForceCmd SetForceTable[NUM_FORCE_POWERS] = {
	{ "forceHeal",			"setForceHeal",			FORCE_LEVEL_3			},
	{ "forceJump",			"setForceJump",			FORCE_LEVEL_3			},
	{ "forceSpeed",			"setForceSpeed",		FORCE_LEVEL_3			},
	{ "forcePush",			"setForcePush",			FORCE_LEVEL_3			},
	{ "forcePull",			"setForcePull",			FORCE_LEVEL_3			},
	{ "forceMindTrick",		"setForceMindTrick",	FORCE_LEVEL_4			},
	{ "forceGrip",			"setForceGrip",			FORCE_LEVEL_3			},
	{ "forceLightning",		"setForceLightning",	FORCE_LEVEL_3			},
	{ "saberThrow",			"setSaberThrow",		FORCE_LEVEL_3			},
	{ "saberDefense",		"setSaberDefense",		FORCE_LEVEL_3			},
	{ "saberOffense",		"setSaberOffense",		SS_NUM_SABER_STYLES-1	},
	{ "forceRage",			"setForceRage",			FORCE_LEVEL_3			},
	{ "forceProtect",		"setForceProtect",		FORCE_LEVEL_3			},
	{ "forceAbsorb",		"setForceAbsorb",		FORCE_LEVEL_3			},
	{ "forceDrain",			"setForceDrain",		FORCE_LEVEL_3			},
	{ "forceSight",			"setForceSight",		FORCE_LEVEL_3			},
};

static void Svcmd_ForceSetLevel_f( int forcePower )
{
	if ( !&g_entities[0] || !g_entities[0].client )
	{
		return;
	}
	const char *newVal = gi.argv(1);
	if ( !VALIDSTRING( newVal ) )
	{
		gi.Printf( "Current %s level is %d\n", SetForceTable[forcePower].desc, g_entities[0].client->ps.forcePowerLevel[forcePower] );
		gi.Printf( "Usage:  %s <level> (0 - %i)\n", SetForceTable[forcePower].cmdname, SetForceTable[forcePower].maxlevel );
		return;
	}
	int val = atoi(newVal);
	if ( val > FORCE_LEVEL_0 )
	{
		g_entities[0].client->ps.forcePowersKnown |= ( 1 << forcePower );
	}
	else
	{
		g_entities[0].client->ps.forcePowersKnown &= ~( 1 << forcePower );
	}
	g_entities[0].client->ps.forcePowerLevel[forcePower] = val;
	if ( g_entities[0].client->ps.forcePowerLevel[forcePower] < FORCE_LEVEL_0 )
	{
		g_entities[0].client->ps.forcePowerLevel[forcePower] = FORCE_LEVEL_0;
	}
	else if ( g_entities[0].client->ps.forcePowerLevel[forcePower] > SetForceTable[forcePower].maxlevel )
	{
		g_entities[0].client->ps.forcePowerLevel[forcePower] = SetForceTable[forcePower].maxlevel;
	}
}

extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransition( int move );
extern qboolean PM_SaberInAttack( int move );
extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
void Svcmd_SaberAttackCycle_f( void )
{
	if ( !&g_entities[0] || !g_entities[0].client )
	{
		return;
	}

	gentity_t *self = G_GetSelfForPlayerCmd();
	if ( self->s.weapon != WP_SABER )
	{// saberAttackCycle button also switches to saber
		gi.SendConsoleCommand("weapon 1" );
		return;
	}

	if ( self->client->ps.dualSabers )
	{//can't cycle styles with dualSabers, so just toggle second saber on/off
		if ( WP_SaberCanTurnOffSomeBlades( &self->client->ps.saber[1] ) )
		{//can turn second saber off
			if ( self->client->ps.saber[1].ActiveManualOnly() )
			{//turn it off
				qboolean skipThisBlade;
				for ( int bladeNum = 0; bladeNum < self->client->ps.saber[1].numBlades; bladeNum++ )
				{
					skipThisBlade = qfalse;
					if ( WP_SaberBladeUseSecondBladeStyle( &self->client->ps.saber[1], bladeNum ) )
					{//check to see if we should check the secondary style's flags
						if ( (self->client->ps.saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
						{
							skipThisBlade = qtrue;
						}
					}
					else
					{//use the primary style's flags
						if ( (self->client->ps.saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
						{
							skipThisBlade = qtrue;
						}
					}
					if ( !skipThisBlade )
					{
						self->client->ps.saber[1].BladeActivate( bladeNum, qfalse );
						G_SoundIndexOnEnt( self, CHAN_WEAPON, self->client->ps.saber[1].soundOff );
					}
				}
			}
			else if ( !self->client->ps.saber[0].ActiveManualOnly() )
			{//first one is off, too, so just turn that one on
				if ( !self->client->ps.saberInFlight )
				{//but only if it's in your hand!
					self->client->ps.saber[0].Activate();
				}
			}
			else
			{//turn on the second one
				self->client->ps.saber[1].Activate();
			}
			return;
		}
	}
	else if ( self->client->ps.saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &self->client->ps.saber[0] ) )//self->client->ps.saber[0].type == SABER_STAFF )
	{//can't cycle styles with saberstaff, so just toggles saber blades on/off
		if ( self->client->ps.saberInFlight )
		{//can't turn second blade back on if it's in the air, you naughty boy!
			return;
		}
		/*
		if ( self->client->ps.saber[0].singleBladeStyle == SS_NONE )
		{//can't use just one blade?
			return;
		}
		*/
		qboolean playedSound = qfalse;
		if ( !self->client->ps.saber[0].blade[0].active )
		{//first one is not even on
			//turn only it on
			self->client->ps.SaberBladeActivate( 0, 0, qtrue );
			return;
		}

		qboolean skipThisBlade;
		for ( int bladeNum = 1; bladeNum < self->client->ps.saber[0].numBlades; bladeNum++ )
		{
			if ( !self->client->ps.saber[0].blade[bladeNum].active )
			{//extra is off, turn it on
				self->client->ps.saber[0].BladeActivate( bladeNum, qtrue );
			}
			else
			{//turn extra off
				skipThisBlade = qfalse;
				if ( WP_SaberBladeUseSecondBladeStyle( &self->client->ps.saber[1], bladeNum ) )
				{//check to see if we should check the secondary style's flags
					if ( (self->client->ps.saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
					{
						skipThisBlade = qtrue;
					}
				}
				else
				{//use the primary style's flags
					if ( (self->client->ps.saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
					{
						skipThisBlade = qtrue;
					}
				}
				if ( !skipThisBlade )
				{
					self->client->ps.saber[0].BladeActivate( bladeNum, qfalse );
					if ( !playedSound )
					{
						G_SoundIndexOnEnt( self, CHAN_WEAPON, self->client->ps.saber[0].soundOff );
						playedSound = qtrue;
					}
				}
			}
		}
		return;
	}

	int allowedStyles = self->client->ps.saberStylesKnown;
	if ( self->client->ps.dualSabers
		&& self->client->ps.saber[0].Active()
		&& self->client->ps.saber[1].Active() )
	{
		allowedStyles |= (1<<SS_DUAL);
		for ( int styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
		{
			if ( styleNum == SS_TAVION
				&& ((self->client->ps.saber[0].stylesLearned&(1<<SS_TAVION))||(self->client->ps.saber[1].stylesLearned&(1<<SS_TAVION)))//was given this style by one of my sabers
				&& !(self->client->ps.saber[0].stylesForbidden&(1<<SS_TAVION))
				&& !(self->client->ps.saber[1].stylesForbidden&(1<<SS_TAVION)) )
			{//if have both sabers on, allow tavion only if one of our sabers specifically wanted to use it... (unless specifically forbidden)
			}
			else if ( styleNum == SS_DUAL
				&& !(self->client->ps.saber[0].stylesForbidden&(1<<SS_DUAL))
				&& !(self->client->ps.saber[1].stylesForbidden&(1<<SS_DUAL)) )
			{//if have both sabers on, only dual style is allowed (unless specifically forbidden)
			}
			else
			{
				allowedStyles &= ~(1<<styleNum);
			}
		}
	}

	if ( !allowedStyles )
	{
		return;
	}

	int	saberAnimLevel;
	if ( !self->s.number )
	{
		saberAnimLevel = cg.saberAnimLevelPending;
	}
	else
	{
		saberAnimLevel = self->client->ps.saberAnimLevel;
	}
	saberAnimLevel++;
	int sanityCheck = 0;
	while ( self->client->ps.saberAnimLevel != saberAnimLevel
		&& !(allowedStyles&(1<<saberAnimLevel))
		&& sanityCheck < SS_NUM_SABER_STYLES+1 )
	{
		saberAnimLevel++;
		if ( saberAnimLevel > SS_STAFF )
		{
			saberAnimLevel = SS_FAST;
		}
		sanityCheck++;
	}

	if ( !(allowedStyles&(1<<saberAnimLevel)) )
	{
		return;
	}

	WP_UseFirstValidSaberStyle( self, &saberAnimLevel );
	if ( !self->s.number )
	{
		cg.saberAnimLevelPending = saberAnimLevel;
	}
	else
	{
		self->client->ps.saberAnimLevel = saberAnimLevel;
	}

#ifndef FINAL_BUILD
	switch ( saberAnimLevel )
	{
	case SS_FAST:
		gi.Printf( S_COLOR_BLUE "Lightsaber Combat Style: Fast\n" );
		//LIGHTSABERCOMBATSTYLE_FAST
		break;
	case SS_MEDIUM:
		gi.Printf( S_COLOR_YELLOW "Lightsaber Combat Style: Medium\n" );
		//LIGHTSABERCOMBATSTYLE_MEDIUM
		break;
	case SS_STRONG:
		gi.Printf( S_COLOR_RED "Lightsaber Combat Style: Strong\n" );
		//LIGHTSABERCOMBATSTYLE_STRONG
		break;
	case SS_DESANN:
		gi.Printf( S_COLOR_CYAN "Lightsaber Combat Style: Desann\n" );
		//LIGHTSABERCOMBATSTYLE_DESANN
		break;
	case SS_TAVION:
		gi.Printf( S_COLOR_MAGENTA "Lightsaber Combat Style: Tavion\n" );
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	case SS_DUAL:
		gi.Printf( S_COLOR_MAGENTA "Lightsaber Combat Style: Dual\n" );
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	case SS_STAFF:
		gi.Printf( S_COLOR_MAGENTA "Lightsaber Combat Style: Staff\n" );
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	}
	//gi.Printf("\n");
#endif
}

qboolean G_ReleaseEntity( gentity_t *grabber )
{
	if ( grabber && grabber->client && grabber->client->ps.heldClient < ENTITYNUM_WORLD )
	{
		gentity_t *heldClient = &g_entities[grabber->client->ps.heldClient];
		grabber->client->ps.heldClient = ENTITYNUM_NONE;
		if ( heldClient && heldClient->client )
		{
			heldClient->client->ps.heldByClient = ENTITYNUM_NONE;

			heldClient->owner = NULL;
		}
		return qtrue;
	}
	return qfalse;
}

void G_GrabEntity( gentity_t *grabber, const char *target )
{
	if ( !grabber || !grabber->client )
	{
		return;
	}
	gentity_t	*heldClient = G_Find( NULL, FOFS(targetname), (char *)target );
	if ( heldClient && heldClient->client && heldClient != grabber )//don't grab yourself, it's not polite
	{//found him
		grabber->client->ps.heldClient = heldClient->s.number;
		heldClient->client->ps.heldByClient = grabber->s.number;

		heldClient->owner = grabber;
	}
}

static void Svcmd_ICARUS_f( void )
{
	Quake3Game()->Svcmd();
}

template <int32_t power>
static void Svcmd_ForceSetLevel_f(void)
{
	Svcmd_ForceSetLevel_f(power);
}

static void Svcmd_SetForceAll_f(void)
{
	for ( int i = FP_HEAL; i < NUM_FORCE_POWERS; i++ )
	{
		Svcmd_ForceSetLevel_f( i );
	}

	if( gi.argc() > 1 )
	{
		for ( int i = SS_NONE+1; i < SS_NUM_SABER_STYLES; i++ )
		{
			g_entities[0].client->ps.saberStylesKnown |= (1<<i);
		}
	}
}

static void Svcmd_SetSaberAll_f(void)
{
	Svcmd_ForceSetLevel_f( FP_SABERTHROW );
	Svcmd_ForceSetLevel_f( FP_SABER_DEFENSE );
	Svcmd_ForceSetLevel_f( FP_SABER_OFFENSE );
	for ( int i = SS_NONE+1; i < SS_NUM_SABER_STYLES; i++ )
	{
		g_entities[0].client->ps.saberStylesKnown |= (1<<i);
	}
}

static void Svcmd_RunScript_f(void)
{
	const char *cmd2 = gi.argv(1);

	if ( cmd2 && cmd2[0] )
	{
		const char *cmd3 = gi.argv(2);
		if ( cmd3 && cmd3[0] )
		{
			gentity_t *found = NULL;
			if ( (found = G_Find(NULL, FOFS(targetname), cmd2 ) ) != NULL )
			{
				Quake3Game()->RunScript( found, cmd3 );
			}
			else
			{
				//can't find cmd2
				gi.Printf( S_COLOR_RED "runscript: can't find targetname %s\n", cmd2 );
			}
		}
		else
		{
			Quake3Game()->RunScript( &g_entities[0], cmd2 );
		}
	}
	else
	{
		gi.Printf( S_COLOR_RED "usage: runscript <ent targetname> scriptname\n" );
	}
}

static void Svcmd_PlayerTeam_f(void)
{
	const char *cmd2 = gi.argv(1);

	if ( !*cmd2 || !cmd2[0] )
	{
		gi.Printf( S_COLOR_RED "'playerteam' - change player team, requires a team name!\n" );
		gi.Printf( S_COLOR_RED "Current team is: %s\n", GetStringForID( TeamTable, g_entities[0].client->playerTeam ) );
		gi.Printf( S_COLOR_RED "Valid team names are:\n");
		for ( int n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
		{
			gi.Printf( S_COLOR_RED "%s\n", GetStringForID( TeamTable, n ) );
		}
	}
	else
	{
		team_t	team;

		team = (team_t)GetIDForString( TeamTable, cmd2 );
		if ( team == (team_t)-1 )
		{
			gi.Printf( S_COLOR_RED "'playerteam' unrecognized team name %s!\n", cmd2 );
			gi.Printf( S_COLOR_RED "Current team is: %s\n", GetStringForID( TeamTable, g_entities[0].client->playerTeam ) );
			gi.Printf( S_COLOR_RED "Valid team names are:\n");
			for ( int n = TEAM_FREE; n < TEAM_NUM_TEAMS; n++ )
			{
				gi.Printf( S_COLOR_RED "%s\n", GetStringForID( TeamTable, n ) );
			}
		}
		else
		{
			g_entities[0].client->playerTeam = team;
			//FIXME: convert Imperial, Malon, Hirogen and Klingon to Scavenger?
		}
	}
}

static void Svcmd_Control_f(void)
{
	const char	*cmd2 = gi.argv(1);
	if ( !*cmd2 || !cmd2[0] )
	{
		if ( !G_ClearViewEntity( &g_entities[0] ) )
		{
			gi.Printf( S_COLOR_RED "control <NPC_targetname>\n", cmd2 );
		}
	}
	else
	{
		Q3_SetViewEntity( 0, cmd2 );
	}
}

static void Svcmd_Grab_f(void)
{
	const char	*cmd2 = gi.argv(1);
	if ( !*cmd2 || !cmd2[0] )
	{
		if ( !G_ReleaseEntity( &g_entities[0] ) )
		{
			gi.Printf( S_COLOR_RED "grab <NPC_targetname>\n", cmd2 );
		}
	}
	else
	{
		G_GrabEntity( &g_entities[0], cmd2 );
	}
}

static void Svcmd_Knockdown_f(void)
{
	G_Knockdown( &g_entities[0], &g_entities[0], vec3_origin, 300, qtrue );
}

static void Svcmd_PlayerModel_f(void)
{
	if ( gi.argc() == 1 )
	{
		gi.Printf( S_COLOR_RED "USAGE: playerModel <NPC Name>\n       playerModel <g2model> <skinhead> <skintorso> <skinlower>\n       playerModel player (builds player from customized menu settings)" S_COLOR_WHITE "\n" );
		gi.Printf( "playerModel = %s ", va("%s %s %s %s\n", g_char_model->string, g_char_skin_head->string, g_char_skin_torso->string, g_char_skin_legs->string ) );
	}
	else if ( gi.argc() == 2 )
	{
		G_ChangePlayerModel( &g_entities[0], gi.argv(1) );
	}
	else if (  gi.argc() == 5 )
	{
		//instead of setting it directly via a command, we now store it in cvars
		//G_ChangePlayerModel( &g_entities[0], va("%s|%s|%s|%s", gi.argv(1), gi.argv(2), gi.argv(3), gi.argv(4)) );
		gi.cvar_set("g_char_model", gi.argv(1) );
		gi.cvar_set("g_char_skin_head", gi.argv(2) );
		gi.cvar_set("g_char_skin_torso", gi.argv(3) );
		gi.cvar_set("g_char_skin_legs", gi.argv(4) );
		G_InitPlayerFromCvars( &g_entities[0] );
	}
}

static void Svcmd_PlayerTint_f(void)
{
	if ( gi.argc() == 4 )
	{
		g_entities[0].client->renderInfo.customRGBA[0] = atoi(gi.argv(1));
		g_entities[0].client->renderInfo.customRGBA[1] = atoi(gi.argv(2));
		g_entities[0].client->renderInfo.customRGBA[2] = atoi(gi.argv(3));
		gi.cvar_set("g_char_color_red", gi.argv(1) );
		gi.cvar_set("g_char_color_green", gi.argv(2) );
		gi.cvar_set("g_char_color_blue", gi.argv(3) );
	}
	else
	{
		gi.Printf( S_COLOR_RED "USAGE: playerTint <red 0 - 255> <green 0 - 255> <blue 0 - 255>\n" );
		gi.Printf( "playerTint = %s\n", va("%d %d %d", g_char_color_red->integer, g_char_color_green->integer, g_char_color_blue->integer ) );
	}
}

static void Svcmd_IKnowKungfu_f(void)
{
	gi.cvar_set( "g_debugMelee", "1" );
	G_SetWeapon( &g_entities[0], WP_MELEE );
	for ( int i = FP_FIRST; i < NUM_FORCE_POWERS; i++ )
	{
		g_entities[0].client->ps.forcePowersKnown |= ( 1 << i );
		if ( i == FP_TELEPATHY )
		{
			g_entities[0].client->ps.forcePowerLevel[i] = FORCE_LEVEL_4;
		}
		else
		{
			g_entities[0].client->ps.forcePowerLevel[i] = FORCE_LEVEL_3;
		}
	}
}

static void Svcmd_Secrets_f(void)
{
	const gentity_t *pl = &g_entities[0];
	if(pl->client->sess.missionStats.totalSecrets < 1)
	{
		gi.Printf( "There are" S_COLOR_RED " NO " S_COLOR_WHITE "secrets on this map!\n" );
	}
	else if(pl->client->sess.missionStats.secretsFound == pl->client->sess.missionStats.totalSecrets)
	{
		gi.Printf( "You've found all " S_COLOR_GREEN "%i" S_COLOR_WHITE " secrets on this map!\n", pl->client->sess.missionStats.secretsFound );
	}
	else
	{
		gi.Printf( "You've found " S_COLOR_GREEN "%i" S_COLOR_WHITE " out of " S_COLOR_GREEN "%i" S_COLOR_WHITE " secrets!\n", pl->client->sess.missionStats.secretsFound, pl->client->sess.missionStats.totalSecrets );
	}
}

// PADAWAN - g_spskill 0 + cg_crosshairForceHint 1 + handicap 100
// JEDI - g_spskill 1 + cg_crosshairForceHint 1 + handicap 100
// JEDI KNIGHT - g_spskill 2 + cg_crosshairForceHint 0 + handicap 100
// JEDI MASTER - g_spskill 2 + cg_crosshairForceHint 0 + handicap 50

extern cvar_t *g_spskill;
static void Svcmd_Difficulty_f(void)
{
	if(gi.argc() == 1)
	{
		if(g_spskill->integer == 0)
		{
			gi.Printf( S_COLOR_GREEN "Current Difficulty: Padawan" S_COLOR_WHITE "\n" );
		}
		else if(g_spskill->integer == 1)
		{
			gi.Printf( S_COLOR_GREEN "Current Difficulty: Jedi" S_COLOR_WHITE "\n" );
		}
		else if(g_spskill->integer == 2)
		{
			int crosshairHint = gi.Cvar_VariableIntegerValue("cg_crosshairForceHint");
			int handicap = gi.Cvar_VariableIntegerValue("handicap");
			if(handicap == 100 && crosshairHint == 0)
			{
				gi.Printf( S_COLOR_GREEN "Current Difficulty: Jedi Knight" S_COLOR_WHITE "\n" );
			}
			else if(handicap == 50 && crosshairHint == 0)
			{
				gi.Printf( S_COLOR_GREEN "Current Difficulty: Jedi Master" S_COLOR_WHITE "\n" );
			}
			else
			{
				gi.Printf( S_COLOR_GREEN "Current Difficulty: Jedi Knight (Custom)" S_COLOR_WHITE "\n" );
				gi.Printf( S_COLOR_GREEN "Crosshair Force Hint: %i" S_COLOR_WHITE "\n", crosshairHint != 0 ? 1 : 0 );
				gi.Printf( S_COLOR_GREEN "Handicap: %i" S_COLOR_WHITE "\n", handicap );
			}
		}
		else
		{
			gi.Printf( S_COLOR_RED "Invalid difficulty cvar set! g_spskill (%i) [0-2] is valid range only" S_COLOR_WHITE "\n", g_spskill->integer );
		}
	}
}

#define CMD_NONE				(0x00000000u)
#define CMD_CHEAT				(0x00000001u)
#define CMD_ALIVE				(0x00000002u)

typedef struct svcmd_s {
	const char	*name;
	void		(*func)(void);
	uint32_t	flags;
} svcmd_t;

static int svcmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((svcmd_t*)b)->name );
}

// FIXME some of these should be made CMD_ALIVE too!
static svcmd_t svcmds[] = {
	{ "entitylist",					Svcmd_EntityList_f,							CMD_NONE },
	{ "game_memory",				Svcmd_GameMem_f,							CMD_NONE },

	{ "nav",						Svcmd_Nav_f,								CMD_CHEAT },
	{ "npc",						Svcmd_NPC_f,								CMD_CHEAT },
	{ "use",						Svcmd_Use_f,								CMD_CHEAT },
	{ "ICARUS",						Svcmd_ICARUS_f,								CMD_CHEAT },

	{ "saberColor",					Svcmd_SaberColor_f,							CMD_CHEAT },
	{ "saber",						Svcmd_Saber_f,								CMD_CHEAT },
	{ "saberBlade",					Svcmd_SaberBlade_f,							CMD_CHEAT },
	
	{ "setForceJump",				Svcmd_ForceSetLevel_f<FP_LEVITATION>,		CMD_CHEAT },
	{ "setSaberThrow",				Svcmd_ForceSetLevel_f<FP_SABERTHROW>,		CMD_CHEAT },
	{ "setForceHeal",				Svcmd_ForceSetLevel_f<FP_HEAL>,				CMD_CHEAT },
	{ "setForcePush",				Svcmd_ForceSetLevel_f<FP_PUSH>,				CMD_CHEAT },
	{ "setForcePull",				Svcmd_ForceSetLevel_f<FP_PULL>,				CMD_CHEAT },
	{ "setForceSpeed",				Svcmd_ForceSetLevel_f<FP_SPEED>,			CMD_CHEAT },
	{ "setForceGrip",				Svcmd_ForceSetLevel_f<FP_GRIP>,				CMD_CHEAT },
	{ "setForceLightning",			Svcmd_ForceSetLevel_f<FP_LIGHTNING>,		CMD_CHEAT },
	{ "setMindTrick",				Svcmd_ForceSetLevel_f<FP_TELEPATHY>,		CMD_CHEAT },
	{ "setSaberDefense",			Svcmd_ForceSetLevel_f<FP_SABER_DEFENSE>,	CMD_CHEAT },
	{ "setSaberOffense",			Svcmd_ForceSetLevel_f<FP_SABER_OFFENSE>,	CMD_CHEAT },
	{ "setForceRage",				Svcmd_ForceSetLevel_f<FP_RAGE>,				CMD_CHEAT },
	{ "setForceDrain",				Svcmd_ForceSetLevel_f<FP_DRAIN>,			CMD_CHEAT },
	{ "setForceProtect",			Svcmd_ForceSetLevel_f<FP_PROTECT>,			CMD_CHEAT },
	{ "setForceAbsorb",				Svcmd_ForceSetLevel_f<FP_ABSORB>,			CMD_CHEAT },
	{ "setForceSight",				Svcmd_ForceSetLevel_f<FP_SEE>,				CMD_CHEAT },
	{ "setForceAll",				Svcmd_SetForceAll_f,						CMD_CHEAT },
	{ "setSaberAll",				Svcmd_SetSaberAll_f,						CMD_CHEAT },
	
	{ "saberAttackCycle",			Svcmd_SaberAttackCycle_f,					CMD_NONE },
	
	{ "runscript",					Svcmd_RunScript_f,							CMD_CHEAT },
	
	{ "playerTeam",					Svcmd_PlayerTeam_f,							CMD_CHEAT },
	
	{ "control",					Svcmd_Control_f,							CMD_CHEAT },
	{ "grab",						Svcmd_Grab_f,								CMD_CHEAT },
	{ "knockdown",					Svcmd_Knockdown_f,							CMD_CHEAT },

	{ "playerModel",				Svcmd_PlayerModel_f,						CMD_NONE },
	{ "playerTint",					Svcmd_PlayerTint_f,							CMD_NONE },
	
	{ "nexttestaxes",				G_NextTestAxes,								CMD_NONE },
	
	{ "exitview",					Svcmd_ExitView_f,							CMD_NONE },
	
	{ "iknowkungfu",				Svcmd_IKnowKungfu_f,						CMD_CHEAT },
	
	{ "secrets",					Svcmd_Secrets_f,							CMD_NONE },
	{ "difficulty",					Svcmd_Difficulty_f,							CMD_NONE },
	
	//{ "say",						Svcmd_Say_f,						qtrue },
	//{ "toggleallowvote",			Svcmd_ToggleAllowVote_f,			qfalse },
	//{ "toggleuserinfovalidation",	Svcmd_ToggleUserinfoValidation_f,	qfalse },
};
static const size_t numsvcmds = ARRAY_LEN( svcmds );

/*
=================
ConsoleCommand
=================
*/
qboolean	ConsoleCommand( void ) {
	const char *cmd = gi.argv(0);
	const svcmd_t *command = (const svcmd_t *)Q_LinearSearch( cmd, svcmds, numsvcmds, sizeof( svcmds[0] ), svcmdcmp );

	if ( !command )
		return qfalse;
	
	if ( (command->flags & CMD_CHEAT)
		&& !g_cheats->integer )
	{
		gi.Printf( "Cheats are not enabled on this server.\n" );
		return qtrue;
	}
	else if ( (command->flags & CMD_ALIVE)
		&& (g_entities[0].health <= 0) )
	{
		gi.Printf( "You must be alive to use this command.\n" );
		return qtrue;
	}
	else
		command->func();
	return qtrue;
}

