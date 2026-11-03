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

// cl_rpadmin.cpp -- whether this player is one of RPMod's admins
//
// The NPC, effect and shader managers, the storyteller and the map editor make what only
// admins' commands can do, so they're for admins. RPMod's rpstatus shows the player's admin
// rank: the first time one of them opens on a server, it's asked, its reply kept out of the
// console, and the tool opens once the rank is in.
//
//   toolaccess          whether the tools open for you here, and the rank found
//   toolaccess check    asks again, the reply shown
//
// A server run on this computer has no RPMod to ask: the tools open there as always.
// Logging in or out of an RPMod account ("You are now logged in as ...") can change the rank:
// the answer is forgotten, the next tool opened asks again.

#include "client.h"

#define RA_COMMAND		"rpstatus"
#define RA_TIMEOUT		4000	// ms the reply has to start in
#define RA_LINE_GAP		400		// ms after its last line it's over
#define RA_RETRY_MS		15000	// a no stands this long; a tool opened after it asks again
#define RA_MAX_LINES	48
#define RA_LINE_LEN		160
#define RA_RANK_LEN		64
#define RA_LABEL		"admin rank"

typedef enum {
	RA_UNKNOWN,
	RA_ASKING,
	RA_ADMIN,
	RA_NOT_ADMIN
} raState_t;

static struct {
	raState_t	state;
	int			askTime, lastLineTime, answerTime;
	int			accountTime;		// logged in or out then: an answer asked before it doesn't count
	qboolean	show;				// asked from the console: the reply is shown
	qboolean	sawLabel;			// a line had the admin rank in it, a rank or not
	char		rank[RA_RANK_LEN];
	char		tool[64];			// the tool waiting on the answer
	char		command[MAX_STRING_CHARS];	// and the command that opens it again
	char		lines[RA_MAX_LINES][RA_LINE_LEN];
	int			numLines;
} ra;

// what follows the label is a rank, not "none" or nothing at all
static qboolean RA_IsRank( const char *value ) {
	static const char *none[] = { "", "none", "no", "n/a", "na", "-", "0", "false", "null", "not admin", "not an admin", "player", "user" };

	for ( size_t i = 0; i < ARRAY_LEN( none ); i++ ) {
		if ( !Q_stricmp( value, none[i] ) )
			return qfalse;
	}
	return qtrue;
}

// one line of the reply: "Admin rank: Owner", "[Admin Rank] Owner", "Admin Rank - [Owner]"...
static void RA_ReadLine( const char *line ) {
	char plain[RA_LINE_LEN], value[RA_RANK_LEN];
	const char *p;
	int len;

	Q_strncpyz( plain, line, sizeof( plain ) );
	Q_StripColor( plain );
	if ( !plain[0] )
		return;
	if ( ra.numLines < RA_MAX_LINES )
		Q_strncpyz( ra.lines[ra.numLines++], plain, RA_LINE_LEN );

	p = Q_stristr( plain, RA_LABEL );
	if ( !p )
		return;
	ra.sawLabel = qtrue;
	p += strlen( RA_LABEL );
	while ( *p && strchr( " \t:]=-)>.", *p ) )
		p++;
	while ( *p == '[' || *p == '(' || *p == '<' )
		p++;
	Q_strncpyz( value, p, sizeof( value ) );
	for ( len = strlen( value ); len && strchr( " \t]>).", value[len - 1] ); len-- )
		value[len - 1] = '\0';
	if ( RA_IsRank( value ) )
		Q_strncpyz( ra.rank, value, sizeof( ra.rank ) );
}

static void RA_Ask( qboolean show ) {
	ra.state = RA_ASKING;
	ra.show = show;
	ra.askTime = cls.realtime;
	ra.lastLineTime = 0;
	ra.sawLabel = qfalse;
	ra.rank[0] = '\0';
	ra.numLines = 0;
	CL_AddReliableCommand( RA_COMMAND, qfalse );
}

// a tool about to open: qfalse keeps it shut, for now or for good
qboolean CL_RpAdmin_Allow( const char *tool, const char *command ) {
	if ( com_sv_running->integer || ra.state == RA_ADMIN )
		return qtrue;
	if ( ra.state == RA_NOT_ADMIN && cls.realtime - ra.answerTime < RA_RETRY_MS ) {
		Com_Printf( S_COLOR_YELLOW "%s is for RPMod admins, and " RA_COMMAND " shows no admin rank for you\n", tool );
		return qfalse;
	}

	// opened again with what opened it, once the answer is in
	Q_strncpyz( ra.tool, tool, sizeof( ra.tool ) );
	Q_strncpyz( ra.command, !Q_stricmp( Cmd_Argv( 0 ), command ) ? Cmd_Cmd() : command, sizeof( ra.command ) );
	if ( ra.state != RA_ASKING )
		RA_Ask( qfalse );
	Com_Printf( "%s: checking your admin rank...\n", tool );
	return qfalse;
}

// RPMod's "You are now logged in as ..." or a logout: the rank may be another now
static qboolean RA_AccountChanged( const char *text ) {
	char plain[RA_LINE_LEN];
	const char *p = plain;

	Q_strncpyz( plain, text, sizeof( plain ) );
	Q_StripColor( plain );
	while ( *p == ' ' || *p == '\n' )
		p++;
	if ( Q_stricmpn( p, "you", 3 ) )
		return qfalse;
	return (qboolean)( Q_stristr( p, "logged in as" ) || Q_stristr( p, "logged out" ) );
}

// a print while it waits for the answer: qtrue keeps it out of the console
qboolean CL_RpAdmin_ServerPrint( const char *text ) {
	char line[RA_LINE_LEN];

	if ( RA_AccountChanged( text ) ) {
		ra.accountTime = cls.realtime;
		if ( ra.state != RA_ASKING ) {
			ra.state = RA_UNKNOWN;
			ra.rank[0] = '\0';
		}
		return qfalse;
	}
	if ( ra.state != RA_ASKING || ( cls.realtime - ra.askTime > RA_TIMEOUT && !ra.lastLineTime ) )
		return qfalse;
	while ( *text ) {
		const char *end = strchr( text, '\n' );
		const int len = end ? (int)( end - text ) : (int)strlen( text );

		Q_strncpyz( line, text, Q_min( len + 1, (int)sizeof( line ) ) );
		RA_ReadLine( line );
		text += len;
		if ( *text == '\n' )
			text++;
	}
	ra.lastLineTime = cls.realtime;
	return (qboolean)!ra.show;
}

void CL_RpAdmin_Frame( void ) {
	if ( ra.state != RA_ASKING )
		return;
	if ( ra.lastLineTime ? cls.realtime - ra.lastLineTime < RA_LINE_GAP : cls.realtime - ra.askTime < RA_TIMEOUT )
		return;

	// logged in while it asked, and no rank: the reply may be from before, asked again
	if ( !ra.rank[0] && ra.accountTime && ra.accountTime >= ra.askTime ) {
		ra.accountTime = 0;
		RA_Ask( ra.show );
		return;
	}
	ra.answerTime = cls.realtime;
	ra.state = ra.rank[0] ? RA_ADMIN : RA_NOT_ADMIN;
	if ( ra.show ) {
		Com_Printf( ra.rank[0] ? "Tools: open, admin rank \"%s\"\n" : "Tools: shut, %s\n", ra.rank[0] ? ra.rank
			: !ra.lastLineTime ? "the server didn't answer " RA_COMMAND : ra.sawLabel ? "no admin rank" : "no admin rank in the reply" );
	} else if ( ra.tool[0] && ra.rank[0] ) {
		Cbuf_AddText( va( "%s\n", ra.command ) );
	} else if ( ra.tool[0] ) {
		Com_Printf( S_COLOR_YELLOW "%s is for RPMod admins, and %s\n", ra.tool,
			!ra.lastLineTime ? "the server didn't answer " RA_COMMAND : RA_COMMAND " shows no admin rank for you" );
	}
	ra.tool[0] = ra.command[0] = '\0';
}

// known to have an admin rank: asks nothing
qboolean CL_RpAdmin_IsAdmin( void ) {
	return (qboolean)( ra.state == RA_ADMIN );
}

// the rank asked for, the answer kept and not shown, unless it's known or being asked already
void CL_RpAdmin_AskQuietly( void ) {
	if ( ra.state == RA_UNKNOWN && cls.state == CA_ACTIVE ) {
		ra.tool[0] = ra.command[0] = '\0';
		RA_Ask( qfalse );
	}
}

static void RA_ToolAccess_f( void ) {
	if ( !Q_stricmp( Cmd_Argv( 1 ), "check" ) ) {
		if ( cls.state != CA_ACTIVE ) {
			Com_Printf( "Not connected to a server.\n" );
			return;
		}
		ra.tool[0] = ra.command[0] = '\0';
		RA_Ask( qtrue );
		return;
	}
	if ( com_sv_running->integer )
		Com_Printf( "Tools: open, the server is this computer's\n" );
	else if ( ra.state == RA_ADMIN )
		Com_Printf( "Tools: open, admin rank \"%s\"\n", ra.rank );
	else if ( ra.state == RA_NOT_ADMIN )
		Com_Printf( "Tools: shut, " RA_COMMAND " shows no admin rank\n" );
	else if ( ra.state == RA_ASKING )
		Com_Printf( "Tools: asking the server\n" );
	else
		Com_Printf( "Tools: not asked yet; the first one opened asks\n" );
	if ( ra.numLines && ra.state != RA_ASKING ) {
		Com_Printf( "The last " RA_COMMAND " reply:\n" );
		for ( int i = 0; i < ra.numLines; i++ )
			Com_Printf( "  %s\n", ra.lines[i] );
	}
	Com_Printf( "toolaccess check : ask again\n" );
}

void CL_RpAdmin_Init( void ) {
	memset( &ra, 0, sizeof( ra ) );
	Cmd_AddCommand( "toolaccess", RA_ToolAccess_f, "Whether the admin tools open for you on this server" );
}

// a new server asks again
void CL_RpAdmin_Disconnect( void ) {
	memset( &ra, 0, sizeof( ra ) );
}
