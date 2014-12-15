// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

//TODO: Replace with reading/writing to file(s)

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client )
{
	char		s[MAX_CVAR_VALUE_STRING] = {0},
				siegeClass[64] = {0}, IP[NET_ADDRSTRMAXLEN] = {0};
	const char	*var;
	int			i = 0;

	// for the strings, replace ' ' with 1

	Q_strncpyz( siegeClass, client->sess.siegeClass, sizeof( siegeClass ) );
	for ( i=0; siegeClass[i]; i++ ) {
		if (siegeClass[i] == ' ')
			siegeClass[i] = 1;
	}
	if ( !siegeClass[0] )
		Q_strncpyz( siegeClass, "none", sizeof( siegeClass ) );

	Q_strncpyz( IP, client->sess.IP, sizeof( IP ) );
	for ( i=0; IP[i]; i++ ) {
		if (IP[i] == ' ')
			IP[i] = 1;
	}

	if (!IP[0])
		Q_strncpyz( IP, "IP", sizeof( IP ) ); //sad hack. i think this is what was messing up session data.. since scanf was skipping right past it

	// Make sure there is no space on the last entry
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.sessionTeam ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.spectatorNum ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.spectatorState ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.spectatorClient ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.wins ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.losses ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.teamLeader ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.setForce ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.saberLevel ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.selectedFP ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.duelTeam ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.siegeDesiredTeam ) );
	Q_strcat( s, sizeof( s ), va( "%s ", siegeClass ) );
	Q_strcat( s, sizeof( s ), va( "%s ", IP ) );

	Q_strcat( s, sizeof( s ), va( "%u ", client->sess.ignore ) );
	Q_strcat( s, sizeof( s ), va( "%i ", (int)client->sess.sawMOTD ) );
	Q_strcat( s, sizeof( s ), va( "%i ", (int)client->sess.raceMode ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.movementStyle ) );

	Q_strcat( s, sizeof( s ), va( "%i ", (int)client->sess.juniorAdmin ) );
	Q_strcat( s, sizeof( s ), va( "%i ", (int)client->sess.fullAdmin ) );

	Q_strcat( s, sizeof( s ), va( "%i ", (int)client->sess.sayteammod ) );
	Q_strcat( s, sizeof( s ), va( "%s", (int)client->sess.clanpass ) );

	var = va( "session%i", client - level.clients );

	trap->Cvar_Set( var, s );

	//trap->Print("WRITING: %s, Session: %s\n", var, s);
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client )
{
	char			s[MAX_CVAR_VALUE_STRING] = {0};
	const char		*var;
	int			i=0, tempSessionTeam=0, tempSpectatorState, tempTeamLeader, tempSawMOTD, tempRaceMode, tempJRAdmin, tempFullAdmin;

	var = va( "session%i", client - level.clients );
	trap->Cvar_VariableStringBuffer( var, s, sizeof(s) );

	//trap->Print("READING: %s, Session: %s\n", var, s);

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %s %s %u %i %i %i %i %i %i %s",//[JAPRO - Serverside - All - Ignore]
		&tempSessionTeam, //&client->sess.sessionTeam,
		&client->sess.spectatorNum,
		&tempSpectatorState, //&client->sess.spectatorState,
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&tempTeamLeader, //&client->sess.teamLeader,
		&client->sess.setForce,
		&client->sess.saberLevel,
		&client->sess.selectedFP,
		&client->sess.duelTeam,
		&client->sess.siegeDesiredTeam,
		client->sess.siegeClass,
		client->sess.IP,
		&client->sess.ignore, //[JAPRO - Serverside - All - Ignore]
		&tempSawMOTD,
		&tempRaceMode, 
		&client->sess.movementStyle,
		&tempJRAdmin,
		&tempFullAdmin,
		&client->sess.sayteammod,
		client->sess.clanpass
		);

	client->sess.sessionTeam	= (team_t)tempSessionTeam;
	client->sess.spectatorState	= (spectatorState_t)tempSpectatorState;
	client->sess.teamLeader		= (qboolean)tempTeamLeader;
	client->sess.sawMOTD		= (qboolean)tempSawMOTD;
	client->sess.raceMode		= (qboolean)tempRaceMode;
	client->sess.juniorAdmin	= (qboolean)tempJRAdmin; //lets see if this works now
	client->sess.fullAdmin		= (qboolean)tempFullAdmin;

	if (client->sess.movementStyle == 0)
		client->sess.movementStyle = 1;//Sad fucking hack to stop it defaulting to siege if player never joined game previous round.. Dunno man

	// convert back to spaces from unused chars, as session data is written that way.
	for ( i=0; client->sess.siegeClass[i]; i++ )
	{
		if (client->sess.siegeClass[i] == 1)
			client->sess.siegeClass[i] = ' ';
	}

	for ( i=0; client->sess.IP[i]; i++ )
	{
		if (client->sess.IP[i] == 1)
			client->sess.IP[i] = ' ';
	}

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.saberDrawAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	client->sess.siegeDesiredTeam = TEAM_FREE;

	// initial team determination
	if ( level.gametype >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer && !(g_entities[client-level.clients].r.svFlags & SVF_BOT) ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			if (!isBot)
			{
				sess->sessionTeam = TEAM_SPECTATOR;	
			}
			else
			{ //Bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if (value[0] == 'r' || value[0] == 'R')
				{
					sess->sessionTeam = TEAM_RED;
				}
				else if (value[0] == 'b' || value[0] == 'B')
				{
					sess->sessionTeam = TEAM_BLUE;
				}
				else
				{
					sess->sessionTeam = PickTeam( -1 );
				}
				BroadcastTeamChange( client, -1 );
			}
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( level.gametype ) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
			case GT_SINGLE_PLAYER:
				if (!isBot && (!g_maxGameClients.integer || (g_maxGameClients.integer > 0 && //loda fixme - this should fix clients showing ingame when they really arnt , when first connect?
					level.numNonSpectatorClients >= g_maxGameClients.integer))) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_DUEL:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_POWERDUEL:
				//sess->duelTeam = DUELTEAM_LONE; //default
				{
					int loners = 0;
					int doubles = 0;

					G_PowerDuelCount(&loners, &doubles, qtrue);

					if (!doubles || loners > (doubles/2))
					{
						sess->duelTeam = DUELTEAM_DOUBLE;
					}
					else
					{
						sess->duelTeam = DUELTEAM_LONE;
					}
				}
				sess->sessionTeam = TEAM_SPECTATOR;
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	AddTournamentQueue(client);

	sess->siegeClass[0] = 0;

	sess->ignore = 0;//[JAPRO - Serverside - All - Ignore]

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap->Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( level.gametype != gt ) {
		level.newSession = qtrue;
		trap->Print( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap->Cvar_Set( "session", va("%i", level.gametype) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
