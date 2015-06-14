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
				siegeClass[64] = {0}, IP[NET_ADDRSTRMAXLEN] = {0}, 
				filename[32] = {0};
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

	Q_strncpyz( filename, client->sess.filename, sizeof( filename ) );
	for ( i=0; filename[i]; i++ ) {
		if (filename[i] == ' ')
			filename[i] = 1;
	}

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
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.amrpgmode ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.selected_special_power ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.selected_left_special_power ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.selected_right_special_power ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.magic_fist_selection ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.ally1 ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.ally2 ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.ally3 ) );
	Q_strcat( s, sizeof( s ), va( "%i ", client->sess.vote_timer ) );
	Q_strcat( s, sizeof( s ), va( "%s ", filename ) );
	Q_strcat( s, sizeof( s ), va( "%s ", siegeClass ) );
	Q_strcat( s, sizeof( s ), va( "%s", IP ) );

	var = va( "session%i", client - level.clients );

	trap->Cvar_Set( var, s );
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
	int			i=0, tempSessionTeam=0, tempSpectatorState, tempTeamLeader;

	var = va( "session%i", client - level.clients );
	trap->Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %s %s %s",
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
		&client->sess.amrpgmode,
		&client->sess.selected_special_power,
		&client->sess.selected_left_special_power,
		&client->sess.selected_right_special_power,
		&client->sess.magic_fist_selection,
		&client->sess.ally1,
		&client->sess.ally2,
		&client->sess.ally3,
		&client->sess.vote_timer,
		client->sess.filename,
		client->sess.siegeClass,
		client->sess.IP
		);

	client->sess.sessionTeam	= (team_t)tempSessionTeam;
	client->sess.spectatorState	= (spectatorState_t)tempSpectatorState;
	client->sess.teamLeader		= (qboolean)tempTeamLeader;

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

	for ( i=0; client->sess.filename[i]; i++ )
	{
		if (client->sess.filename[i] == 1)
			client->sess.filename[i] = ' ';
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
				if ( g_maxGameClients.integer > 0 &&
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
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

	// zyk: setting initial value of RPG mode session attributes
	sess->amrpgmode = 0;
	strcpy(sess->filename,"");

	sess->magic_fist_selection = 0;
	sess->selected_special_power = 1;
	sess->selected_left_special_power = 1;
	sess->selected_right_special_power = 1;

	// zyk: initializing ally attributes
	sess->ally1 = -1;
	sess->ally2 = -1;
	sess->ally3 = -1;

	sess->vote_timer = 0;

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
