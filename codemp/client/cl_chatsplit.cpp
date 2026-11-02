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

// cl_chatsplit.cpp -- a chat too long for the server sent as several, each but the last ending
// with -- and each but the first starting with it. The server cuts a message at its length
// (149 letters in the base game) and ignores a command come within a second of the one before,
// so the parts go one by one, cl_chatSplitDelay apart.
//
//	cl_chatSplit			1 on, 0 off (the default)
//	cl_chatSplitLength		the letters the server keeps of one message
//	cl_chatSplitMark		what ends a part cut short and starts the next
//	cl_chatSplitDelay		ms between the parts
//	cl_chatSplitCommands	the chat commands; name:n keeps n words before the text, as tell's target
//
// And a chat starting with a small letter given a capital, the console's and the chat box's:
//
//	cl_chatCapital			1 the first letter said (after any emotes it starts with) and each sentence's, 0 as typed
//	cl_chatCapitalEmotes	1 each :: emote ::'s first letter, 0 as typed
//	cl_chatCapitalCommands	the chat commands given them, name:n as above

#include "client.h"

#define CS_MAX_QUEUED	32
#define CS_MIN_PART		16		// a part's room no smaller than this, whatever the mark

static cvar_t *cl_chatSplit;
static cvar_t *cl_chatSplitLength;
static cvar_t *cl_chatSplitMark;
static cvar_t *cl_chatSplitDelay;
static cvar_t *cl_chatSplitCommands;
static cvar_t *cl_chatCapital;
static cvar_t *cl_chatCapitalEmotes;
static cvar_t *cl_chatCapitalCommands;

static struct {
	char	cmd[CS_MAX_QUEUED][MAX_STRING_CHARS];
	int		head, count;
	int		nextTime;			// the next part not before
} cs;

void CL_ChatSplit_Init( void ) {
	cl_chatSplit = Cvar_Get( "cl_chatSplit", "0", CVAR_ARCHIVE_ND, "A chat too long for the server sent as several, -- between them" );
	cl_chatSplitLength = Cvar_Get( "cl_chatSplitLength", "149", CVAR_ARCHIVE_ND, "The letters the server keeps of one chat message" );
	cl_chatSplitMark = Cvar_Get( "cl_chatSplitMark", "--", CVAR_ARCHIVE_ND, "Ends a chat message cut in parts, and starts the next part" );
	cl_chatSplitDelay = Cvar_Get( "cl_chatSplitDelay", "1100", CVAR_ARCHIVE_ND, "Milliseconds between the parts of a long chat (the server ignores faster)" );
	cl_chatSplitCommands = Cvar_Get( "cl_chatSplitCommands", "say say_team tell:1 me", CVAR_ARCHIVE_ND,
		"The chat commands split when too long; name:n keeps n words before the text" );
	cl_chatCapital = Cvar_Get( "cl_chatCapital", "0", CVAR_ARCHIVE_ND, "Chat and each sentence in it starting with a small letter start with a capital" );
	cl_chatCapitalEmotes = Cvar_Get( "cl_chatCapitalEmotes", "0", CVAR_ARCHIVE_ND, "A :: emote :: in chat starts with a capital" );
	cl_chatCapitalCommands = Cvar_Get( "cl_chatCapitalCommands", "say say_team me", CVAR_ARCHIVE_ND,
		"The chat commands given capitals; name:n keeps n words before the text" );
}

static void CS_Queue( const char *cmd ) {
	if ( cs.count >= CS_MAX_QUEUED ) {
		Com_Printf( S_COLOR_YELLOW "Chat: too much waiting to be sent; the rest is dropped\n" );
		return;
	}
	Q_strncpyz( cs.cmd[( cs.head + cs.count ) % CS_MAX_QUEUED], cmd, MAX_STRING_CHARS );
	cs.count++;
}

// sends what waits, one part a delay
void CL_ChatSplit_Frame( void ) {
	if ( !cs.count )
		return;
	if ( cls.state != CA_ACTIVE || clc.demoplaying ) {
		cs.count = 0;
		return;
	}
	if ( cls.realtime < cs.nextTime )
		return;
	CL_AddReliableCommand( cs.cmd[cs.head], qfalse );
	cs.head = ( cs.head + 1 ) % CS_MAX_QUEUED;
	cs.count--;
	cs.nextTime = cls.realtime + Com_Clampi( 0, 10000, cl_chatSplitDelay->integer );
}

// the command's words kept before the text, -1 if it isn't one of the list's
static int CS_KeptWordsIn( const char *commands, const char *cmd ) {
	char list[MAX_CVAR_VALUE_STRING];
	char *p, *name;

	Q_strncpyz( list, commands, sizeof( list ) );
	for ( p = list; *p; ) {
		char *colon;

		while ( *p == ' ' || *p == ',' )
			p++;
		if ( !*p )
			break;
		name = p;
		while ( *p && *p != ' ' && *p != ',' )
			p++;
		if ( *p )
			*p++ = '\0';
		colon = strchr( name, ':' );
		if ( colon )
			*colon = '\0';
		if ( !Q_stricmp( name, cmd ) )
			return colon ? Q_max( 0, atoi( colon + 1 ) ) : 0;
	}
	return -1;
}

static int CS_KeptWords( const char *cmd ) {
	return CS_KeptWordsIn( cl_chatSplitCommands->string, cmd );
}

// past the spaces and colours at p
static char *CC_Visible( char *p ) {
	for ( ;; ) {
		if ( *p == ' ' )
			p++;
		else if ( p[0] == Q_COLOR_ESCAPE && p[1] && p[1] != Q_COLOR_ESCAPE )
			p += 2;
		else
			return p;
	}
}

static void CC_Upper( char *p ) {
	if ( *p >= 'a' && *p <= 'z' )
		*p = (char)( *p - 'a' + 'A' );
}

// each sentence's first letter from p on: after a . ! or ? and a space. Not after "..." (a pause,
// the sentence going on), nor in 3.5 or www.site.com, with no space after the stop
static void CC_Sentences( char *p ) {
	while ( *p ) {
		int dots = 0;
		qboolean stop = qfalse;

		if ( *p != '.' && *p != '!' && *p != '?' ) {
			p++;
			continue;
		}
		for ( ; *p == '.' || *p == '!' || *p == '?'; p++ ) {
			if ( *p == '.' )
				dots++;
			else
				stop = qtrue;
		}
		if ( dots == 1 )
			stop = qtrue;
		if ( stop && *p == ' ' )
			CC_Upper( CC_Visible( p ) );
	}
}

// a chat's text given its capitals, as cmd's: the first letter said, after any emotes it starts
// with, each sentence's, and each emote's first letter
void CL_ChatCaps_Text( const char *cmd, char *text ) {
	qboolean first, emotes;
	char *p;

	if ( !cl_chatCapital )
		return;
	first = (qboolean)( cl_chatCapital->integer != 0 );
	emotes = (qboolean)( cl_chatCapitalEmotes->integer != 0 );
	if ( ( !first && !emotes ) || CS_KeptWordsIn( cl_chatCapitalCommands->string, cmd ) < 0 )
		return;

	// the emotes it starts with, then the first letter said
	for ( p = CC_Visible( text ); !strncmp( p, "::", 2 ); ) {
		char *end = strstr( p + 2, "::" );

		if ( emotes )
			CC_Upper( CC_Visible( p + 2 ) );
		if ( !end )
			return;
		p = CC_Visible( end + 2 );
	}
	if ( first ) {
		CC_Upper( p );
		CC_Sentences( p );
	}

	// the emotes after it
	if ( !emotes )
		return;
	for ( p = strstr( p, "::" ); p; ) {
		char *end = strstr( p + 2, "::" );

		CC_Upper( CC_Visible( p + 2 ) );
		if ( !end )
			break;
		p = strstr( end + 2, "::" );
	}
}

// the chat command just typed, given its capitals: rebuilt into out, qtrue, when they changed it
qboolean CL_ChatCaps_Forward( char *out, int size ) {
	char text[MAX_STRING_CHARS], typed[MAX_STRING_CHARS];
	const char *cmd = Cmd_Argv( 0 );
	int keep;

	if ( !cl_chatCapital || ( !cl_chatCapital->integer && !cl_chatCapitalEmotes->integer ) )
		return qfalse;
	keep = CS_KeptWordsIn( cl_chatCapitalCommands->string, cmd );
	if ( keep < 0 || Cmd_Argc() <= 1 + keep )
		return qfalse;
	Q_strncpyz( text, Cmd_ArgsFrom( 1 + keep ), sizeof( text ) );
	Q_strncpyz( typed, text, sizeof( typed ) );
	CL_ChatCaps_Text( cmd, text );
	if ( !strcmp( text, typed ) )
		return qfalse;
	Q_strncpyz( out, cmd, size );
	for ( int i = 1; i <= keep; i++ )
		Q_strcat( out, size, va( " %s", Cmd_Argv( i ) ) );
	Q_strcat( out, size, va( " \"%s\"", text ) );
	return qtrue;
}

// the length the server will see of a part: a spoken language's label counted in
static int CS_SaidLength( const char *cmd, const char *text, int n ) {
	char part[MAX_STRING_CHARS];
	const char *out;

	Q_strncpyz( part, text, Q_min( (int)sizeof( part ), n + 1 ) );
	if ( Q_stricmp( cmd, "say" ) )
		return (int)strlen( part );
	out = CL_Languages_OutgoingChat( va( "say \"%s\"", part ) );
	if ( !Q_stricmpn( out, "say ", 4 ) ) {
		int len = (int)strlen( out + 4 );

		// still quoted: not changed
		if ( out[4] == '"' )
			len -= 2;
		return len;
	}
	return (int)strlen( part );
}

// the last colour set in the text, 0 if none
static char CS_LastColour( const char *text, int n ) {
	char colour = 0;

	for ( int i = 0; i + 1 < n; i++ ) {
		if ( text[i] == Q_COLOR_ESCAPE && text[i + 1] != Q_COLOR_ESCAPE && text[i + 1] ) {
			colour = text[i + 1];
			i++;
		}
	}
	return colour;
}

// the command forwarded to the server, when it's a chat too long: queued in parts. qtrue if it was
qboolean CL_ChatSplit_Forward( void ) {
	char text[MAX_STRING_CHARS], prefix[MAX_STRING_CHARS], before[64], after[64];
	const char *cmd = Cmd_Argv( 0 );
	const char *mark;
	int keep, limit, len, at, parts = 0;
	char colour = 0;

	if ( !cl_chatSplit || !cl_chatSplit->integer )
		return qfalse;
	keep = CS_KeptWords( cmd );
	if ( keep < 0 || Cmd_Argc() <= 1 + keep )
		return qfalse;

	Q_strncpyz( text, Cmd_ArgsFrom( 1 + keep ), sizeof( text ) );
	len = (int)strlen( text );
	limit = Com_Clampi( CS_MIN_PART * 2, MAX_STRING_CHARS - 64, cl_chatSplitLength->integer );

	// the same chat behind parts still waiting goes after them, not before
	if ( CS_SaidLength( cmd, text, len ) <= limit && !cs.count )
		return qfalse;

	// what goes before the text: the command and its kept words
	Q_strncpyz( prefix, cmd, sizeof( prefix ) );
	for ( int i = 1; i <= keep; i++ )
		Q_strcat( prefix, sizeof( prefix ), va( " %s", Cmd_Argv( i ) ) );

	mark = cl_chatSplitMark->string;
	Com_sprintf( before, sizeof( before ), "%s ", mark );
	Com_sprintf( after, sizeof( after ), " %s", mark );
	if ( !mark[0] )
		before[0] = after[0] = '\0';

	for ( at = 0; at < len; ) {
		char part[MAX_STRING_CHARS];
		const char *rest = text + at;
		const int restLen = len - at;
		int n;

		while ( *rest == ' ' && at < len ) {
			rest++;
			at++;
		}
		if ( at >= len )
			break;

		// all that's left, if it fits
		// the colour the part before ended in, the mark in it too
		Com_sprintf( part, sizeof( part ), "%s%s%s", colour ? va( "^%c", colour ) : "", parts ? before : "", rest );
		if ( CS_SaidLength( cmd, part, (int)strlen( part ) ) <= limit ) {
			CS_Queue( va( "%s \"%s\"", prefix, part ) );
			break;
		}

		// the most that fits with the mark after it, cut at a space where it can be
		{
			char head[64];
			int lo = 1, hi = restLen, best = 1, cut;

			Com_sprintf( head, sizeof( head ), "%s%s", colour ? va( "^%c", colour ) : "", parts ? before : "" );
			while ( lo <= hi ) {
				const int mid = ( lo + hi ) / 2;

				Com_sprintf( part, sizeof( part ), "%s%.*s%s", head, mid, rest, after );
				if ( CS_SaidLength( cmd, part, (int)strlen( part ) ) <= limit ) {
					best = mid;
					lo = mid + 1;
				} else {
					hi = mid - 1;
				}
			}
			n = best;
			for ( cut = n; cut > n / 2; cut-- ) {
				if ( rest[cut] == ' ' ) {
					n = cut;
					break;
				}
			}
			// not between a ^ and its colour
			if ( n > 1 && rest[n - 1] == Q_COLOR_ESCAPE )
				n--;
			while ( n > 1 && rest[n - 1] == ' ' )
				n--;
			Com_sprintf( part, sizeof( part ), "%s%.*s%s", head, n, rest, after );
			CS_Queue( va( "%s \"%s\"", prefix, part ) );
			{
				const char c = CS_LastColour( rest, n );

				if ( c )
					colour = c;
			}
		}
		at += n;
		parts++;
	}
	CL_ChatSplit_Frame();
	return qtrue;
}
