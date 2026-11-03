/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// cl_quickconnect.cpp -- JEDI's servers, a click away: QUICK CONNECT in the in-game menu
//
// The menu's panel (see cl_rendermenu.cpp) shows each server's address, map and players from the
// ui_qc<n>_* cvars set here, asked of the servers themselves (getinfo) when it opens or Refresh is
// clicked; a server clicked is connected to.

#include "client.h"
#include "cl_uiapi.h"

#define QC_TIMEOUT_MS		3000

typedef struct {
	const char	*name;
	const char	*address;
} qcServer_t;

static const qcServer_t qcServers[] = {
	{ "JEDI Temple",	"temple.jediholo.net:29070" },
	{ "JEDI Grounds",	"grounds.jediholo.net:29071" },
	{ "JEDI Region",	"region.jediholo.net:29072" },
	{ "JEDI Galaxy",	"galaxy.jediholo.net:29073" },
};
#define QC_NUM			( (int)ARRAY_LEN( qcServers ) )

static struct {
	netadr_t	adr[QC_NUM];
	qboolean	resolved[QC_NUM];
	int			askTime;
	qboolean	answered[QC_NUM];
} qc;

static void QC_Set( int i, const char *what, const char *value ) {
	Cvar_Set( va( "ui_qc%i_%s", i + 1, what ), value );
}

// each asked again what map it's on and who's there
static void QC_Refresh( void ) {
	qc.askTime = cls.realtime;
	for ( int i = 0; i < QC_NUM; i++ ) {
		qc.answered[i] = qfalse;
		QC_Set( i, "map", "Map: ..." );
		QC_Set( i, "players", "Players: ..." );
		if ( !qc.resolved[i] ) {
			qc.resolved[i] = NET_StringToAdr( qcServers[i].address, &qc.adr[i] );
			if ( !qc.resolved[i] ) {
				QC_Set( i, "map", "Map: address not found" );
				QC_Set( i, "players", "Players: -" );
				continue;
			}
		}
		NET_OutOfBandPrint( NS_CLIENT, &qc.adr[i], "getinfo quickconnect" );
	}
}

// an infoResponse: one of ours answering; qtrue if it was
qboolean CL_QuickConnect_InfoResponse( const netadr_t *from, const char *info ) {
	for ( int i = 0; i < QC_NUM; i++ ) {
		if ( !qc.resolved[i] || !NET_CompareAdr( from, &qc.adr[i] ) )
			continue;
		qc.answered[i] = qtrue;
		QC_Set( i, "map", va( "Map: %s", Info_ValueForKey( info, "mapname" ) ) );
		QC_Set( i, "players", va( "Players: %s/%s", Info_ValueForKey( info, "clients" ), Info_ValueForKey( info, "sv_maxclients" ) ) );
		return qtrue;
	}
	return qfalse;
}

// Esc with the panel out closes the panel, not the menu: the in-game menu opened again, which
// puts the panel away (its onOpen, see cl_rendermenu.cpp). qtrue if it did
qboolean CL_QuickConnect_Escape( void ) {
	if ( !( Key_GetCatcher() & KEYCATCH_UI ) || cls.state != CA_ACTIVE || !Cvar_VariableIntegerValue( "ui_qcOpen" ) )
		return qfalse;
	Cvar_Set( "ui_qcOpen", "0" );
	UIVM_SetActiveMenu( UIMENU_INGAME );
	return qtrue;
}

void CL_QuickConnect_Frame( void ) {
	// the menu gone, by whatever way: the panel with it
	if ( !( Key_GetCatcher() & KEYCATCH_UI ) && Cvar_VariableIntegerValue( "ui_qcOpen" ) )
		Cvar_Set( "ui_qcOpen", "0" );
	if ( !qc.askTime || cls.realtime - qc.askTime < QC_TIMEOUT_MS )
		return;
	for ( int i = 0; i < QC_NUM; i++ ) {
		if ( !qc.answered[i] && qc.resolved[i] ) {
			QC_Set( i, "map", "Map: no answer" );
			QC_Set( i, "players", "Players: -" );
		}
	}
	qc.askTime = 0;
}

// quickconnect refresh | connect <1-4> | list
static void QC_Cmd_f( void ) {
	const char *verb = Cmd_Argv( 1 );

	if ( !Q_stricmp( verb, "refresh" ) ) {
		QC_Refresh();
	} else if ( !Q_stricmp( verb, "connect" ) ) {
		const int i = atoi( Cmd_Argv( 2 ) ) - 1;

		if ( i < 0 || i >= QC_NUM ) {
			Com_Printf( "quickconnect: connect 1 to %i\n", QC_NUM );
			return;
		}
		Cbuf_AddText( va( "connect %s\n", qcServers[i].address ) );
	} else {
		for ( int i = 0; i < QC_NUM; i++ )
			Com_Printf( "  %i  %s  %s\n", i + 1, qcServers[i].name, qcServers[i].address );
		Com_Printf( "quickconnect connect <number> | refresh\n" );
	}
}

const char *CL_QuickConnect_Name( int i ) {
	return i >= 0 && i < QC_NUM ? qcServers[i].name : NULL;
}

void CL_QuickConnect_Init( void ) {
	for ( int i = 0; i < QC_NUM; i++ ) {
		Cvar_Get( va( "ui_qc%i_server", i + 1 ), "", 0, "Quick connect: the server's address" );
		Cvar_Get( va( "ui_qc%i_map", i + 1 ), "", 0, "Quick connect: its map" );
		Cvar_Get( va( "ui_qc%i_players", i + 1 ), "", 0, "Quick connect: its players" );
		QC_Set( i, "server", va( "Server: %s", qcServers[i].address ) );
		QC_Set( i, "map", "Map: ..." );
		QC_Set( i, "players", "Players: ..." );
	}
	Cmd_AddCommand( "quickconnect", QC_Cmd_f, "JEDI's servers: quickconnect connect <1-4>, refresh" );
}
