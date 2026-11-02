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

// cl_whisper.cpp -- whispers, heard only up close
//
// On RPMod a character whispers in ^k. With cl_whisper 1, chat in ^k, every run of it until
// another colour, is only for those near enough:
//
//   cl_whisper          0 off (everyone sees it), 1 on
//   cl_whisperHear      up to this many units away the whisper shows as it is
//   cl_whisperNotice    up to here it's left out of the chat, and RPMod's notice says
//                       ":: You hear <name> whispering... ::"; farther, only left out
//
// The rest of the line, in other colours, shows as ever; a line that was all whisper doesn't
// show at all. It works on the chat before the cgame gets it, so the chat box, the console and
// the subtitles alike. The distance is the one RPMod's chat carries, a /me's worked out as the
// chat range's is (CL_ChatSenderDistance). Your own whispers always show.
//
// The notice is RPMod's own, an "info" the cgame is handed after the chat (CL_QueueCgameCommand):
//   info <ms> <icon> <sound> 8 "<text>", icon 2 the "i", as rpnotify sends it. The sound is a
//   slot in the server's sound list (CS_SOUNDS), which isn't the same on every map or server:
//   info.wav's slot is looked up; when the server hasn't got it, slot 0 (silent) and the client
//   plays info.wav itself.

#include "client.h"

#define WH_COLOR			'k'
#define WH_NOTICE			"info 5000 2 %i 8 \":: You hear %s" S_COLOR_WHITE " whispering... ::\""
#define WH_SOUND			"sound/effects/info"	// RPMod's info sound, in RPMod-Client-Assets.pk3
#define WH_REPEAT_MS		3000		// the same one whispering again this soon: no second notice
#define WH_LABEL			S_COLOR_YELLOW "::"	// a language's ::Name:: before what's said

static cvar_t *cl_whisper;
static cvar_t *cl_whisperHear;
static cvar_t *cl_whisperNotice;

static struct {
	char	name[MAX_STRING_CHARS];		// the last noticed whispering
	int		time;
} wh;

// the slot of RPMod's info sound in the server's sound list, 0 if it isn't there
static int WH_InfoSoundSlot( void ) {
	for ( int i = 1; i < MAX_SOUNDS; i++ ) {
		const int ofs = cl.gameState.stringOffsets[CS_SOUNDS + i];
		char name[MAX_QPATH];

		if ( !ofs )
			continue;
		COM_StripExtension( cl.gameState.stringData + ofs, name, sizeof( name ) );
		if ( !Q_stricmp( name, WH_SOUND ) )
			return i;
	}
	return 0;
}

// the notice for the cgame, with the info sound
static const char *WH_Notice( const char *name ) {
	const int slot = WH_InfoSoundSlot();

	if ( !slot ) {
		const sfxHandle_t sfx = S_RegisterSound( WH_SOUND ".wav" );

		if ( sfx )
			S_StartLocalSound( sfx, CHAN_LOCAL_SOUND );
	}
	return va( WH_NOTICE, slot, name );
}

void CL_Whisper_Init( void ) {
	cl_whisper = Cvar_Get( "cl_whisper", "0", CVAR_ARCHIVE_ND,
		"Whispers (chat in ^k): 1 shows them only to those close by, a notice to those a little farther" );
	cl_whisperHear = Cvar_Get( "cl_whisperHear", "128", CVAR_ARCHIVE_ND,
		"With cl_whisper 1, whispers from this many units away or closer show" );
	cl_whisperNotice = Cvar_Get( "cl_whisperNotice", "384", CVAR_ARCHIVE_ND,
		"With cl_whisper 1, whispers from this many units away or closer are noticed, without what's said" );
	memset( &wh, 0, sizeof( wh ) );
}

static qboolean WH_IsCode( const char *p ) {
	return (qboolean)( p[0] == Q_COLOR_ESCAPE && p[1] && p[1] != Q_COLOR_ESCAPE );
}

// whether there's a whisper in it
static qboolean WH_Has( const char *text ) {
	for ( const char *p = text; *p; p++ ) {
		if ( WH_IsCode( p ) && p[1] == WH_COLOR )
			return qtrue;
	}
	return qfalse;
}

// anything to read in it, past the colours and spaces
static qboolean WH_Visible( const char *p, const char *end ) {
	for ( ; p < end && *p; p++ ) {
		if ( WH_IsCode( p ) ) {
			p++;
			continue;
		}
		if ( *p != ' ' )
			return qtrue;
	}
	return qfalse;
}

// the text with its whispers left out, and what they leave empty: double spaces, a << >> with
// nothing in it
static void WH_Strip( const char *in, char *out, int size ) {
	char tmp[MAX_STRING_CHARS];
	qboolean whisper = qfalse;
	int o = 0;

	for ( const char *p = in; *p && o < (int)sizeof( tmp ) - 1; ) {
		if ( WH_IsCode( p ) ) {
			whisper = (qboolean)( p[1] == WH_COLOR );
			if ( !whisper && o < (int)sizeof( tmp ) - 2 ) {
				tmp[o++] = p[0];
				tmp[o++] = p[1];
			}
			p += 2;
			continue;
		}
		if ( !whisper && !( *p == ' ' && o && tmp[o - 1] == ' ' ) )
			tmp[o++] = *p;
		p++;
	}
	tmp[o] = '\0';

	o = 0;
	for ( const char *p = tmp; *p && o < size - 1; ) {
		if ( p[0] == '<' && p[1] == '<' ) {
			const char *close = strstr( p + 2, ">>" );

			if ( close && !WH_Visible( p + 2, close ) ) {
				p = close + 2;
				while ( *p == ' ' )
					p++;
				continue;
			}
		}
		out[o++] = *p++;
	}
	while ( o && out[o - 1] == ' ' )
		o--;
	out[o] = '\0';
}

// an RPMod comm: "Comm-<name>: ...", colours and all
static qboolean WH_IsComm( const char *p ) {
	static const char comm[] = "comm-";
	int i = 0;

	while ( *p == ' ' )
		p++;
	for ( ; *p && comm[i]; p++ ) {
		if ( WH_IsCode( p ) ) {
			p++;
			continue;
		}
		if ( tolower( (unsigned char)*p ) != comm[i++] )
			return qfalse;
	}
	return (qboolean)( !comm[i] && strchr( p, ':' ) != NULL );
}

// nothing left to read: no text, or only a language's ::Name:: with nothing said in it
static qboolean WH_Empty( const char *text ) {
	const char *p = text, *end;

	while ( *p == ' ' )
		p++;
	if ( !strncmp( p, WH_LABEL, strlen( WH_LABEL ) ) && ( end = strstr( p + strlen( WH_LABEL ), "::" ) ) != NULL )
		p = end + 2;
	return (qboolean)!WH_Visible( p, p + strlen( p ) );
}

// the chat command just tokenized: whispers left out by how far they come from, a notice after
// it for those a little farther. "chat" "name\x19: message" <distance>, a /me "name\x19 message";
// "lchat" "name" "location" "colour" "message" <distance>
void CL_Whisper_ServerCommand( void ) {
	const char *cmd = Cmd_Argv( 0 ), *arg, *num, *message;
	char name[MAX_STRING_CHARS], stripped[MAX_STRING_CHARS], changed[MAX_STRING_CHARS], rebuilt[BIG_INFO_STRING];
	qboolean notice;
	float distance;
	int at, keep = 0, argc = Cmd_Argc();

	if ( !cl_whisper->integer || cls.state != CA_ACTIVE )
		return;
	if ( !strcmp( cmd, "chat" ) || !strcmp( cmd, "tchat" ) ) {
		at = 1;
		num = argc > 2 ? Cmd_Argv( 2 ) : "";
	} else if ( !strcmp( cmd, "lchat" ) || !strcmp( cmd, "ltchat" ) ) {
		at = 4;
		num = argc > 5 ? Cmd_Argv( 5 ) : "";
	} else {
		return;
	}
	if ( argc <= at )
		return;
	arg = Cmd_Argv( at );

	// the name, and where what's said starts
	if ( at == 1 ) {
		const char *sep = strchr( arg, '\x19' );

		if ( !sep )
			return;
		keep = (int)( sep - arg ) + 1;
		if ( arg[keep] == ':' )
			keep++;
		Q_strncpyz( name, arg, Q_min( (int)sizeof( name ), (int)( sep - arg ) + 1 ) );
	} else {
		Q_strncpyz( name, Cmd_Argv( 1 ), sizeof( name ) );
	}
	message = arg + keep;
	if ( !WH_Has( message ) )
		return;
	// a comm goes straight to the one it's for: whispered, it's still heard in full
	if ( WH_IsComm( message ) )
		return;

	// RPMod's chat says how far; a /me doesn't, nor does chat elsewhere
	if ( !Q_stricmp( FS_GetCurrentGameDir(), "rpmod" ) && isdigit( (unsigned char)num[0] ) )
		distance = (float)atoi( num );
	else if ( !CL_ChatSenderDistance( at == 1 ? arg : va( "%s\x19", name ), &distance ) )
		return;
	if ( distance <= cl_whisperHear->value )
		return;
	notice = (qboolean)( distance <= cl_whisperNotice->value );

	// once for each one whispering, a long whisper's parts too
	if ( notice ) {
		if ( !Q_stricmp( wh.name, name ) && cls.realtime - wh.time < WH_REPEAT_MS && cls.realtime >= wh.time )
			notice = qfalse;
		else
			Q_strncpyz( wh.name, name, sizeof( wh.name ) );
		wh.time = cls.realtime;
	}
	for ( char *c = name; *c; c++ ) {
		if ( *c == '"' )
			*c = '\'';
	}

	WH_Strip( message, stripped, sizeof( stripped ) );
	if ( WH_Empty( stripped ) ) {
		// all of it a whisper: the notice in its place, or nothing
		Cmd_TokenizeString( notice ? WH_Notice( name ) : "print \"\"" );
		return;
	}

	Com_sprintf( changed, sizeof( changed ), "%.*s%s%s", keep, arg, keep && arg[keep - 1] == ':' ? " " : "",
		stripped[0] == ' ' ? stripped + 1 : stripped );
	Q_strncpyz( rebuilt, cmd, sizeof( rebuilt ) );
	for ( int i = 1; i < argc; i++ ) {
		char part[MAX_STRING_CHARS];

		Q_strncpyz( part, i == at ? changed : Cmd_Argv( i ), sizeof( part ) );
		for ( char *c = part; *c; c++ ) {
			if ( *c == '"' )
				*c = '\'';
		}
		Q_strcat( rebuilt, sizeof( rebuilt ), va( " \"%s\"", part ) );
	}
	if ( notice )
		CL_QueueCgameCommand( WH_Notice( name ) );
	Cmd_TokenizeString( rebuilt );
}
