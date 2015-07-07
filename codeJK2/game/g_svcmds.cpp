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
#include "g_headers.h"

#include "g_local.h"
#include "g_icarus.h"
#include "wp_saber.h"

extern void Q3_SetViewEntity(int entID, const char *name);
extern qboolean G_ClearViewEntity( gentity_t *ent );

extern team_t TranslateTeamName( const char *name );
extern char	*TeamNames[TEAM_NUM_TEAMS];

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
			gi.Printf("ET_GENERAL ");
			break;
		case ET_PLAYER:
			gi.Printf("ET_PLAYER  ");
			break;
		case ET_ITEM:
			gi.Printf("ET_ITEM    ");
			break;
		case ET_MISSILE:
			gi.Printf("ET_MISSILE ");
			break;
		case ET_MOVER:
			gi.Printf("ET_MOVER   ");
			break;
		case ET_BEAM:
			gi.Printf("ET_BEAM    ");
			break;
		default:
			gi.Printf("#%i", check->s.eType);
			break;
		}

		if ( check->classname ) {
			gi.Printf("%s", check->classname);
		}
		gi.Printf("\n");
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			gi.Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	gi.Printf( "User %s is not on the server\n", s );

	return NULL;
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

static void Svcmd_SaberColor_f()
{
	const char *color = gi.argv(1);
	if ( !VALIDSTRING( color ))
	{
		gi.Printf( "Usage:  saberColor <color>\n" );
		gi.Printf( "valid colors:  red, orange, yellow, green, blue, and purple\n" );

		return;
	}
	
	gentity_t *self = G_GetSelfForPlayerCmd();

	if ( !Q_stricmp( color, "red" ))
	{
		self->client->ps.saberColor = SABER_RED;
	}
	else if ( !Q_stricmp( color, "green" ))
	{
		self->client->ps.saberColor = SABER_GREEN;
	}
	else if ( !Q_stricmp( color, "yellow" ))
	{
		self->client->ps.saberColor = SABER_YELLOW;
	}
	else if ( !Q_stricmp( color, "orange" ))
	{
		self->client->ps.saberColor = SABER_ORANGE;
	}
	else if ( !Q_stricmp( color, "purple" ))
	{
		self->client->ps.saberColor = SABER_PURPLE;
	}
	else if ( !Q_stricmp( color, "blue" ))
	{
		self->client->ps.saberColor = SABER_BLUE;
	}
	else
	{
		gi.Printf( "Usage:  saberColor <color>\n" );
		gi.Printf( "valid colors:  red, orange, yellow, green, blue, and purple\n" );
	}
}

struct SetForceCmd {
	const char *desc;
	const char *cmdname;
	const int maxlevel;
};

SetForceCmd SetForceTable[] = {
	{ "forceHeal",			"setForceHeal",			FORCE_LEVEL_3	},
	{ "forceJump",			"setForceJump",			FORCE_LEVEL_3	},
	{ "forceSpeed",			"setForceSpeed",		FORCE_LEVEL_3	},
	{ "forcePush",			"setForcePush",			FORCE_LEVEL_3	},
	{ "forcePull",			"setForcePull",			FORCE_LEVEL_3	},
	{ "forceMindTrick",		"setForceMindTrick",	FORCE_LEVEL_4	},
	{ "forceGrip",			"setForceGrip",			FORCE_LEVEL_3	},
	{ "forceLightning",		"setForceLightning",	FORCE_LEVEL_3	},
	{ "saberThrow",			"setSaberThrow",		FORCE_LEVEL_3	},
	{ "saberDefense",		"setSaberDefense",		FORCE_LEVEL_3	},
	{ "saberOffense",		"setSaberOffense",		FORCE_LEVEL_5	},
};

static void Svcmd_ForceSetLevel_f( int forcePower )
{
	if ( !&g_entities[0] || !g_entities[0].client )
	{
		return;
	}
	if ( !g_cheats->integer ) 
	{
		gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
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
void Svcmd_SaberAttackCycle_f( void )
{
	if ( !&g_entities[0] || !g_entities[0].client )
	{
		return;
	}

	gentity_t *self = G_GetSelfForPlayerCmd();
	if ( self->s.weapon != WP_SABER )
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
	if ( self->client->ps.forcePowerLevel[FP_SABER_OFFENSE] == FORCE_LEVEL_1 )
	{
		saberAnimLevel = FORCE_LEVEL_2;
	}
	else if ( self->client->ps.forcePowerLevel[FP_SABER_OFFENSE] == FORCE_LEVEL_2 )
	{
		if ( saberAnimLevel > FORCE_LEVEL_2 )
		{
			saberAnimLevel = FORCE_LEVEL_1;
		}
	}
	else
	{//level 3
		if ( saberAnimLevel > self->client->ps.forcePowerLevel[FP_SABER_OFFENSE] )
		{//wrap around
			saberAnimLevel = FORCE_LEVEL_1;
		}
	}

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
	case FORCE_LEVEL_1:
		gi.Printf( S_COLOR_BLUE"Lightsaber Combat Style: Fast\n" );
		//LIGHTSABERCOMBATSTYLE_FAST
		break;
	case FORCE_LEVEL_2:
		gi.Printf( S_COLOR_YELLOW"Lightsaber Combat Style: Medium\n" );
		//LIGHTSABERCOMBATSTYLE_MEDIUM
		break;
	case FORCE_LEVEL_3:
		gi.Printf( S_COLOR_RED"Lightsaber Combat Style: Strong\n" );
		//LIGHTSABERCOMBATSTYLE_STRONG
		break;
	case FORCE_LEVEL_4:
		gi.Printf( S_COLOR_CYAN"Lightsaber Combat Style: Desann\n" );
		//LIGHTSABERCOMBATSTYLE_DESANN
		break;
	case FORCE_LEVEL_5:
		gi.Printf( S_COLOR_MAGENTA"Lightsaber Combat Style: Tavion\n" );
		//LIGHTSABERCOMBATSTYLE_TAVION
		break;
	}
	//gi.Printf("\n");
#endif
}
/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	const char	*cmd;

	cmd = gi.argv(0);

	if ( Q_stricmp (cmd, "entitylist") == 0 ) 
	{
		Svcmd_EntityList_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

//	if (Q_stricmp (cmd, "addbot") == 0) {
//		Svcmd_AddBot_f();
//		return qtrue;
//	}

	if (Q_stricmp (cmd, "nav") == 0) 
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		Svcmd_Nav_f ();
		return qtrue;
	}

	if (Q_stricmp (cmd, "npc") == 0) 
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		Svcmd_NPC_f ();
		return qtrue;
	}

	if (Q_stricmp (cmd, "use") == 0) 
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		Svcmd_Use_f ();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ICARUS" ) == 0 )	
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		Svcmd_ICARUS_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "saberColor" ) == 0 )	
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		Svcmd_SaberColor_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "setForceJump" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_LEVITATION );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setSaberThrow" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_SABERTHROW );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForceHeal" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_HEAL );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForcePush" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_PUSH );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForcePull" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_PULL );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForceSpeed" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_SPEED );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForceGrip" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_GRIP );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForceLightning" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_LIGHTNING );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setMindTrick" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_TELEPATHY );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setSaberDefense" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_SABER_DEFENSE );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setSaberOffense" ) == 0 )
	{
		Svcmd_ForceSetLevel_f( FP_SABER_OFFENSE );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setForceAll" ) == 0 )
	{
		if ( !g_cheats->integer )
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"" );
			return qtrue;
		}

		for ( int i = FP_HEAL; i <= FP_SABER_OFFENSE; i++ )
		{
			Svcmd_ForceSetLevel_f( i );
		}
		return qtrue;
	}
	if ( Q_stricmp( cmd, "setSaberAll" ) == 0 )
	{
		if ( !g_cheats->integer )
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"" );
			return qtrue;
		}

		Svcmd_ForceSetLevel_f( FP_SABERTHROW );
		Svcmd_ForceSetLevel_f( FP_SABER_DEFENSE );
		Svcmd_ForceSetLevel_f( FP_SABER_OFFENSE );
		return qtrue;
	}
	if ( Q_stricmp( cmd, "saberAttackCycle" ) == 0 )	
	{
		Svcmd_SaberAttackCycle_f();
		return qtrue;
	}
	if ( Q_stricmp( cmd, "runscript" ) == 0 ) 
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		const char *cmd2 = gi.argv(1);

		if ( cmd2 && cmd2[0] )
		{
			const char *cmd3 = gi.argv(2);
			if ( cmd3 && cmd3[0] )
			{
				gentity_t *found = NULL;
				if ( (found = G_Find(NULL, FOFS(targetname), cmd2 ) ) != NULL )
				{
					ICARUS_RunScript( found, va( "%s/%s", Q3_SCRIPT_DIR, cmd3 ) );
				}
				else
				{
					//can't find cmd2
					gi.Printf( S_COLOR_RED"runscript: can't find targetname %s\n", cmd2 );
				}
			}
			else
			{
				ICARUS_RunScript( &g_entities[0], va( "%s/%s", Q3_SCRIPT_DIR, cmd2 ) );
			}
		}
		else
		{
			gi.Printf( S_COLOR_RED"usage: runscript <ent targetname> scriptname\n" );
		}
		//FIXME: else warning
		return qtrue;
	}

	if ( Q_stricmp( cmd, "playerteam" ) == 0 ) 
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		const char	*cmd2 = gi.argv(1);
		int		n;

		if ( !*cmd2 || !cmd2[0] )
		{
			gi.Printf( S_COLOR_RED"'playerteam' - change player team, requires a team name!\n" );
			gi.Printf( S_COLOR_RED"Valid team names are:\n");
			for ( n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
			{
				gi.Printf( S_COLOR_RED"%s\n", TeamNames[n] );
			}
		}
		else
		{
			team_t	team;

			team = TranslateTeamName( cmd2 );
			if ( team == TEAM_FREE )
			{
				gi.Printf( S_COLOR_RED"'playerteam' unrecognized team name %s!\n", cmd2 );
				gi.Printf( S_COLOR_RED"Valid team names are:\n");
				for ( n = (TEAM_FREE + 1); n < TEAM_NUM_TEAMS; n++ )
				{
					gi.Printf( S_COLOR_RED"%s\n", TeamNames[n] );
				}
			}
			else
			{
				g_entities[0].client->playerTeam = team;
				//FIXME: convert Imperial, Malon, Hirogen and Klingon to Scavenger?
			}
		}
		return qtrue;
	}

	if ( Q_stricmp( cmd, "control" ) == 0 )
	{
		if ( !g_cheats->integer ) 
		{
			gi.SendServerCommand( 0, "print \"Cheats are not enabled on this server.\n\"");
			return qtrue;
		}
		const char	*cmd2 = gi.argv(1);
		if ( !*cmd2 || !cmd2[0] )
		{
			if ( !G_ClearViewEntity( &g_entities[0] ) )
			{
				gi.Printf( S_COLOR_RED"control <NPC_targetname>\n", cmd2 );
			}
		}
		else
		{
			Q3_SetViewEntity( 0, cmd2 );
		}
		return qtrue;
	}

	if ( Q_stricmp( cmd, "exitview" ) == 0 )
	{
		Svcmd_ExitView_f();
		return qtrue;
	}
	
	return qfalse;
}

