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

// cl_adminflags.cpp -- noclip, nodraw, nonsolid and notarget on you, kept track of
//
// The admin commands that turn them on or off say so in their reply, and only noclip shows in what
// the server sends of the player. So each is read from the replies: a line naming it, with on or off
// (enabled, disabled...) in it, soon after you sent it at yourself, or one saying it's yours. While
// any is on, a small line at the bottom of the screen says which: noclip: on | nodraw: off | ...

#include "client.h"

#define AF_REPLY_MS		4000	// a reply this soon after the command is its
#define AF_TEXT_SCALE	0.6f

static const char *afNames[] = { "noclip", "nodraw", "nonsolid", "notarget" };
// as the replies may spell them: "Non-solid mode enabled", "no clip"
static const char *afSpaced[] = { "no clip", "no draw", "non solid", "no target" };
#define AF_NUM			( (int)ARRAY_LEN( afNames ) )

static struct {
	qboolean	on[AF_NUM];
	int			sentTime[AF_NUM];		// sent at ourselves then
} af;

static cvar_t *cl_adminFlags;

// a word on its own in the line, letters around it not counting
static qboolean AF_HasWord( const char *line, const char *word ) {
	const size_t len = strlen( word );

	for ( const char *p = line; ( p = Q_stristr( p, word ) ) != NULL; p++ ) {
		const qboolean before = (qboolean)( p > line && isalpha( (unsigned char)p[-1] ) );
		const qboolean after = (qboolean)( isalpha( (unsigned char)p[len] ) );

		if ( !before && !after )
			return qtrue;
	}
	return qfalse;
}

// a command sent: one of these at ourselves, by no name, our number or our name
void CL_AdminFlags_Command( void ) {
	const char *cmd = Cmd_Argv( 0 );

	for ( int i = 0; i < AF_NUM; i++ ) {
		const char *target;
		qboolean self;

		if ( Q_stricmp( cmd, afNames[i] ) )
			continue;
		target = Cmd_Argv( 1 );
		self = (qboolean)( !target[0] || ( Q_isanumber( target ) && atoi( target ) == clc.clientNum ) );
		if ( !self && cls.state == CA_ACTIVE ) {
			const int ofs = cl.gameState.stringOffsets[CS_PLAYERS + clc.clientNum];
			char mine[MAX_QPATH], typed[MAX_QPATH];

			Q_strncpyz( mine, ofs ? Info_ValueForKey( cl.gameState.stringData + ofs, "n" ) : "", sizeof( mine ) );
			CL_StripAllColors( mine );
			Q_strncpyz( typed, Cmd_ArgsFrom( 1 ), sizeof( typed ) );
			CL_StripAllColors( typed );
			self = (qboolean)( mine[0] && typed[0] && ( !Q_stricmp( mine, typed ) || ( strlen( typed ) >= 3 && Q_stristr( mine, typed ) ) ) );
		}
		if ( self )
			af.sentTime[i] = cls.realtime;
	}
}

// a server print: one of them turned on or off for us
void CL_AdminFlags_ServerPrint( const char *text ) {
	char line[MAX_STRING_CHARS], joined[MAX_STRING_CHARS];
	qboolean off, on, aboutYou;
	int len = 0;

	Q_strncpyz( line, text, sizeof( line ) );
	CL_StripAllColors( line );
	// the hyphens out, "non-solid" read as "nonsolid"
	for ( const char *p = line; *p && len < (int)sizeof( joined ) - 1; p++ ) {
		if ( *p != '-' )
			joined[len++] = *p;
	}
	joined[len] = '\0';
	off = (qboolean)( AF_HasWord( line, "off" ) || AF_HasWord( line, "disabled" ) || AF_HasWord( line, "deactivated" ) || AF_HasWord( line, "false" ) );
	on = (qboolean)( AF_HasWord( line, "on" ) || AF_HasWord( line, "enabled" ) || AF_HasWord( line, "activated" ) || AF_HasWord( line, "true" ) );
	if ( on == off )
		return;
	aboutYou = (qboolean)( AF_HasWord( line, "you" ) || AF_HasWord( line, "your" ) );
	for ( int i = 0; i < AF_NUM; i++ ) {
		if ( !AF_HasWord( joined, afNames[i] ) && !AF_HasWord( line, afSpaced[i] ) )
			continue;
		if ( !aboutYou && ( !af.sentTime[i] || cls.realtime - af.sentTime[i] > AF_REPLY_MS ) )
			continue;
		af.on[i] = on;
		af.sentTime[i] = 0;
	}
}

void CL_AdminFlags_Draw( void ) {
	char text[MAX_STRING_CHARS];
	qboolean any = qfalse;
	qhandle_t font;
	int w;

	if ( !cl_adminFlags || !cl_adminFlags->integer || cls.state != CA_ACTIVE || ( Key_GetCatcher() & KEYCATCH_UI ) )
		return;
	// noclip the server shows us anyway
	if ( cl.snap.valid && cl.snap.ps.pm_type == PM_NOCLIP )
		af.on[0] = qtrue;
	text[0] = '\0';
	for ( int i = 0; i < AF_NUM; i++ ) {
		if ( af.on[i] )
			any = qtrue;
		Q_strcat( text, sizeof( text ), va( "%s%s: %s" S_COLOR_WHITE, i ? " | " : "", afNames[i], af.on[i] ? S_COLOR_GREEN "on" : S_COLOR_RED "off" ) );
	}
	if ( !any )
		return;
	font = re->RegisterFont( "arialnb" );
	if ( !font )
		font = cls.menuFont;
	w = re->Font_StrLenPixels( text, font, AF_TEXT_SCALE );
	re->Font_DrawString( ( SCREEN_WIDTH - w ) / 2, SCREEN_HEIGHT - 16, text, colorWhite, font | STYLE_DROPSHADOW, -1, AF_TEXT_SCALE );
}

// a new server or map: nothing is on
void CL_AdminFlags_Reset( void ) {
	memset( &af, 0, sizeof( af ) );
}

static void AF_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "reset" ) ) {
		CL_AdminFlags_Reset();
		return;
	}
	for ( int i = 0; i < AF_NUM; i++ )
		Com_Printf( "%s: %s\n", afNames[i], af.on[i] ? S_COLOR_GREEN "on" : S_COLOR_RED "off" );
	Com_Printf( S_COLOR_WHITE "adminflags reset : all taken as off\n" );
}

void CL_AdminFlags_Init( void ) {
	cl_adminFlags = Cvar_Get( "cl_adminFlags", "1", CVAR_ARCHIVE_ND, "Show noclip, nodraw, nonsolid and notarget at the bottom while one is on" );
	Cmd_AddCommand( "adminflags", AF_Cmd_f, "Noclip, nodraw, nonsolid and notarget on you, as known" );
	CL_AdminFlags_Reset();
}
