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

// cl_rphooks.cpp -- client hooks carried in RPMod's prints
//
// Until the server can send them itself, an admin's
//   rpnotify -1 1 cmd:<hook>/<args...>: <message>
// reaches clients as a "print" and an "info" (RPMod's banner) whose text starts with
// "cmd:" (rpcentersay's "cp" works too, but the server crashes on its long words). These
// are never shown, in the console, the banner or the middle of the screen: the first one
// goes to the hook named instead.
//
//   cmd:npc/talk/<npc>/<display name>/<name colour>/<emote|none>: <message>
//     (cmd:npc/<npc>/talk/... too, say for talk, and spaces around the parts)
//     shows the message as chat from that name, in the console and the chat
//     box, as a player's chat would be. The emote is for whoever sends it to
//     play with "npc emote"; the storyteller does.
//
//   cmd:npc/walk/<entity number>/<speed>/<x y z>/<x y z>...
//     the NPC walks the spots, on the screens of everyone with this client; see
//     cl_npcwalk.cpp. cmd:npc/walk/<entity number>[,<entity number>...]/stop ends it.
//     cmd:npc/group/<speed>/<entity number x y z>.../<x y z>... walks several together.
//
//   cmd:objective/<add|done|undone|remove|clear|title>: <text or number>
//     the mission's objectives, listed on the left while the menu (Esc) is up.
//     done strikes one through, undone takes the line off again, remove takes it
//     away; they name it by its text or its number in the list. clear empties the
//     list, or with "done" only takes the struck ones. cmd:objective: <text> adds.
//
//   cmd:cine/<id>/<part>/<parts>: <text>
//     a cinematic from the cinematic manager, played once all its parts are in; cmd:cine/stop
//     ends it. See cl_cinematic.cpp.
//   cmd:cin/<verb>/...
//     a cinematic camera for everyone with this client; see cl_rpcinematic.cpp.
//
//   cmd:map/<verb>/...
//     an admin's map edits, shown to everyone on the map; see cl_mapeditor.cpp.
//
//   cmd:mine/<client number>/<part>/<parts>: <shader>><shader>|...
//     a player's shader manager player shaders: their own force powers, sabers and
//     shells drawn with them on the screens of everyone with this client; sent as
//     they change, as the map starts and as someone joins. cmd:mine/<client number>/clear
//     drops them. See cl_shadermanager.cpp.
//
//   cmd:fxscale/<x y z>/<size>[/once]: <effect>
//     the effect playing there drawn that much bigger or smaller, 1 as made; see
//     cl_effectmanager.cpp.
//
//   cmd:sky/<r g b>/<degrees a second>/<pitch yaw roll>/<x y z>[/<portal x y z>|-[/<zoom>]]
//     the sky tinted, spun round, turned and zoomed, a 3D skybox's view also moved, the map
//     seen from a spot in place of the sky with a portal, until the map changes; see
//     cl_shadermanager.cpp. cmd:sky/off puts it back.
//     cmd:startcinematic and cmd:endcinematic start and end one too.
//
// Hooks only ever show things: nothing a server sends here runs a command.

#include "client.h"

#define RH_PREFIX		"cmd:"
#define RH_MAX_ARGS		64		// a camera move's spots, an NPC walk's, a group's NPCs and spots
// RPMod word-wraps centre prints at about 50 characters a line, and a word longer than
// a line crashes the server: 36 in a row went through, 65 didn't
#define RH_MAX_WORD		40
#define RH_REPEAT_MS	2000	// the same hook again this soon is a copy of it, a print and a cp

#define RH_MAX_OBJECTIVES	12
#define RH_OBJECTIVE_LEN	256
#define RH_OBJ_TITLE		"Objective:"
#define RH_OBJ_X			16.0f
#define RH_OBJ_Y			84.0f		// under RPMod's menu bar and its screenshot buttons
#define RH_OBJ_W			200.0f		// inside the panel
#define RH_OBJ_PAD			6.0f
#define RH_OBJ_ROW_H		14.0f
#define RH_OBJ_SCALE		0.8f
#define RH_OBJ_BULLET		"- "

// NPC talk and players' chat during a cinematic with the HUD hidden, where the chat box can't be seen
#define RH_MAX_SUBS			3
#define RH_SUB_LEN			512
#define RH_SUB_W			480.0f
#define RH_SUB_SCALE		1.0f
#define RH_SUB_BOTTOM		10.0f		// above the bottom of the screen
#define RH_SUB_FADE_MS		500

typedef enum {
	RH_PASS,		// not a hook, the cgame gets it
	RH_DROP,		// a hook, the cgame never hears of it
	RH_REPLACED		// a hook that hands the cgame another command, already tokenized
} rhResult_t;

// "^1", "1" or "" as a colour code
static const char *RH_Color( const char *in ) {
	static char out[3];

	if ( in[0] == Q_COLOR_ESCAPE && in[1] && in[1] != Q_COLOR_ESCAPE ) {
		out[0] = Q_COLOR_ESCAPE;
		out[1] = in[1];
	} else if ( in[0] && !in[1] && isalnum( (unsigned char)in[0] ) ) {
		out[0] = Q_COLOR_ESCAPE;
		out[1] = in[0];
	} else {
		return S_COLOR_WHITE;
	}
	out[2] = '\0';
	return out;
}

static qboolean RH_IsTalk( const char *verb ) {
	return (qboolean)( !Q_stricmp( verb, "talk" ) || !Q_stricmp( verb, "say" ) );
}

static struct {
	char		text[RH_SUB_LEN];
	int			until;			// cls.realtime it's gone by
	float		alpha;			// chat from far away fainter, as in the chat box
}			rhSubs[RH_MAX_SUBS];	// oldest first

// as long as it takes to read, the oldest making way when there are too many
static void RH_AddSubtitle( const char *text, float alpha ) {
	int ms = 3000 + 60 * (int)strlen( text );

	if ( rhSubs[RH_MAX_SUBS - 1].text[0] ) {
		memmove( &rhSubs[0], &rhSubs[1], sizeof( rhSubs[0] ) * ( RH_MAX_SUBS - 1 ) );
		rhSubs[RH_MAX_SUBS - 1].text[0] = '\0';
	}
	for ( int i = 0; i < RH_MAX_SUBS; i++ ) {
		if ( !rhSubs[i].text[0] ) {
			Q_strncpyz( rhSubs[i].text, text, sizeof( rhSubs[i].text ) );
			rhSubs[i].until = cls.realtime + Q_min( ms, 10000 );
			rhSubs[i].alpha = alpha;
			break;
		}
	}
}

// cmd:npc/talk/<npc>/<name>/<colour>/<emote>: <message>
static rhResult_t RH_Npc( char **argv, int argc, const char *message ) {
	char cmd[MAX_STRING_CHARS];
	const char *npc, *name, *color;
	int rest;

	// the verb before the NPC, or after it
	if ( argc >= 2 && RH_IsTalk( argv[0] ) ) {
		npc = argv[1];
	} else if ( argc >= 2 && RH_IsTalk( argv[1] ) ) {
		npc = argv[0];
	} else {
		Com_DPrintf( "RP hook: npc wants talk or say, and the NPC\n" );
		return RH_DROP;
	}
	rest = 2;
	// the name and its colour may have a / or : in them, escaped
	for ( int i = rest; i < argc; i++ )
		CL_RpHooks_Unescape( argv[i] );
	name = argc > rest && argv[rest][0] ? argv[rest] : npc;
	color = RH_Color( argc > rest + 1 ? argv[rest + 1] : "" );
	if ( !message[0] )
		return RH_DROP;

	RH_AddSubtitle( va( "%s%s" S_COLOR_WHITE ": " S_COLOR_GREEN "%s", color, name, CL_Languages_Filter( message ) ), 1.0f );
	// chat as the game sends it, from right next to us so RPMod's chat range lets it through
	Com_sprintf( cmd, sizeof( cmd ), "chat \"%s%s" S_COLOR_WHITE "\x19: " S_COLOR_GREEN "%s\" 0", color, name, message );
	Cmd_TokenizeString( cmd );
	return RH_REPLACED;
}

static qboolean rhExperimental;
static cvar_t *cl_rpExperimental;		// on as the game starts: rpexperimental

qboolean CL_RpHooks_Experimental( void ) {
	return rhExperimental;
}

// rpexperimental [on|off]: experimental features on or off now, and so every time the game starts.
// A tool's TOGGLE EXPERIMENTAL is for the session only
static void CL_RpExperimental_f( void ) {
	const char *arg = Cmd_Argv( 1 );
	qboolean on;

	if ( !arg[0] )
		on = (qboolean)!rhExperimental;
	else if ( !Q_stricmp( arg, "on" ) || !Q_stricmp( arg, "1" ) )
		on = qtrue;
	else if ( !Q_stricmp( arg, "off" ) || !Q_stricmp( arg, "0" ) )
		on = qfalse;
	else {
		Com_Printf( "usage: rpexperimental [on|off]   experimental features, kept for every time the game starts\n" );
		return;
	}
	rhExperimental = on;
	Cvar_Set( "cl_rpExperimental", on ? "1" : "0" );
	Com_Printf( "Experimental features %s, now and every time the game starts\n", on ? S_COLOR_YELLOW "on" S_COLOR_WHITE : "off" );
	if ( on )
		Com_Printf( S_COLOR_YELLOW "%s\n", RP_EXPERIMENTAL_WARNING );
}

void CL_RpHooks_SetExperimental( qboolean on ) {
	rhExperimental = on;
	Com_Printf( "Experimental features %s\n", on ? S_COLOR_YELLOW "on" S_COLOR_WHITE " for this session" : "off" );
}

// a part of the hook: what would end it early, a / or the : before the message, goes as ~s and
// ~c (a ~ as ~~), so a name like ^7::JEDI:: comes through whole
static const char *RH_HookPart( const char *in ) {
	static char out[4][MAX_QPATH * 2];
	static int index;
	char *o = out[index++ & 3];
	int len = 0;

	for ( ; *in && len < MAX_QPATH * 2 - 3; in++ ) {
		if ( *in == '/' || *in == ':' || *in == '~' ) {
			o[len++] = '~';
			o[len++] = *in == '/' ? 's' : *in == ':' ? 'c' : '~';
		} else {
			o[len++] = *in;
		}
	}
	o[len] = '\0';
	return o;
}

// a part of the hook as it was before RH_HookPart, in place
void CL_RpHooks_Unescape( char *s ) {
	char *o = s;

	for ( ; *s; s++ ) {
		if ( s[0] == '~' && ( s[1] == 's' || s[1] == 'c' || s[1] == '~' ) ) {
			s++;
			*o++ = *s == 's' ? '/' : *s == 'c' ? ':' : '~';
		} else {
			*o++ = *s;
		}
	}
	*o = '\0';
}

// the command that has everyone with this client see the NPC say it: the storyteller's NPC talk
// and the NPC manager's. An empty part would make a // the server takes for a comment
const char *CL_RpHooks_NpcTalkCommand( const char *npc, const char *name, const char *color, const char *emote, const char *text ) {
	static char cmd[MAX_STRING_CHARS];

	Com_sprintf( cmd, sizeof( cmd ), "rpnotify -1 1 cmd:npc/talk/%s/%s/%s/%s: %s", RH_HookPart( npc ), RH_HookPart( name[0] ? name : npc ),
		RH_HookPart( color[0] ? color : "^7" ), emote[0] ? RH_HookPart( emote ) : "none", text );
	return cmd;
}

static struct {
	char		title[RH_OBJECTIVE_LEN];	// "" for RH_OBJ_TITLE
	struct {
		char		text[RH_OBJECTIVE_LEN];
		qboolean	done;
	}			items[RH_MAX_OBJECTIVES];
	int			count;
} rhObj;

// the objective a number in the list or its text names, -1 if none
static int RH_FindObjective( const char *name ) {
	char a[RH_OBJECTIVE_LEN], b[RH_OBJECTIVE_LEN];

	if ( Q_isanumber( name ) && strchr( name, '.' ) == NULL ) {
		const int n = atoi( name );
		return n >= 1 && n <= rhObj.count ? n - 1 : -1;
	}
	Q_strncpyz( a, name, sizeof( a ) );
	Q_StripColor( a );
	for ( int i = 0; i < rhObj.count; i++ ) {
		Q_strncpyz( b, rhObj.items[i].text, sizeof( b ) );
		Q_StripColor( b );
		if ( !Q_stricmp( a, b ) )
			return i;
	}
	return -1;
}

static void RH_RemoveObjective( int i ) {
	memmove( &rhObj.items[i], &rhObj.items[i + 1], ( rhObj.count - i - 1 ) * sizeof( rhObj.items[0] ) );
	rhObj.count--;
}

// cmd:objective/<verb>: <text or number>
static rhResult_t RH_Objective( char **argv, int argc, const char *message ) {
	const char *verb = argc >= 1 && argv[0][0] ? argv[0] : "add";
	int i;

	if ( !Q_stricmp( verb, "clear" ) ) {
		if ( Q_stricmp( message, "done" ) ) {
			rhObj.count = 0;
			return RH_DROP;
		}
		for ( i = rhObj.count - 1; i >= 0; i-- ) {
			if ( rhObj.items[i].done )
				RH_RemoveObjective( i );
		}
		return RH_DROP;
	}
	if ( !Q_stricmp( verb, "title" ) ) {
		Q_strncpyz( rhObj.title, message, sizeof( rhObj.title ) );
		return RH_DROP;
	}
	if ( !message[0] )
		return RH_DROP;

	i = RH_FindObjective( message );
	if ( !Q_stricmp( verb, "add" ) ) {
		// already there, it's back to not done
		if ( i >= 0 && !Q_isanumber( message ) ) {
			rhObj.items[i].done = qfalse;
			return RH_DROP;
		}
		if ( rhObj.count == RH_MAX_OBJECTIVES ) {
			Com_DPrintf( "RP hook: objectives full, \"%s\" left out\n", message );
			return RH_DROP;
		}
		Q_strncpyz( rhObj.items[rhObj.count].text, message, sizeof( rhObj.items[0].text ) );
		rhObj.items[rhObj.count].done = qfalse;
		rhObj.count++;
		return RH_DROP;
	}
	if ( i < 0 ) {
		Com_DPrintf( "RP hook: no objective \"%s\"\n", message );
		return RH_DROP;
	}
	if ( !Q_stricmp( verb, "done" ) )
		rhObj.items[i].done = qtrue;
	else if ( !Q_stricmp( verb, "undone" ) || !Q_stricmp( verb, "undo" ) )
		rhObj.items[i].done = qfalse;
	else if ( !Q_stricmp( verb, "remove" ) )
		RH_RemoveObjective( i );
	else
		Com_DPrintf( "RP hook: objective wants add, done, undone, remove, clear or title, not \"%s\"\n", verb );
	return RH_DROP;
}

// past "cmd:" if the text is a hook's, NULL if not; colours and spaces before it are let through
static const char *RH_HookText( const char *text ) {
	while ( *text == ' ' || Q_IsColorString( text ) )
		text += *text == ' ' ? 1 : 2;
	if ( Q_stricmpn( text, RH_PREFIX, strlen( RH_PREFIX ) ) )
		return NULL;
	return text + strlen( RH_PREFIX );
}

// the server word-wraps centre prints, a line break where a space was: back to one line
static void RH_Unwrap( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( *in == '\n' || *in == '\r' ) {
			// next to a space already, it was put in beside it rather than for it
			if ( len && out[len - 1] != ' ' && in[1] != ' ' && in[1] != '\n' && in[1] != '\r' && in[1] )
				out[len++] = ' ';
			continue;
		}
		out[len++] = *in;
	}
	out[len] = '\0';
}

static struct {
	char	text[MAX_STRING_CHARS];
	int		time;
} rhLast;

// a print or centre print about to reach the cgame: a hook's, or one to show
static rhResult_t RH_Run( const char *text ) {
	char flat[MAX_STRING_CHARS], header[MAX_STRING_CHARS], *argv[RH_MAX_ARGS], *p;
	const char *colon, *message;
	int argc = 0;

	text = RH_HookText( text );
	if ( !text )
		return RH_PASS;
	RH_Unwrap( text, flat, sizeof( flat ) );
	text = flat;

	// sent both ways, it runs once
	if ( rhLast.time && cls.realtime - rhLast.time < RH_REPEAT_MS && !strcmp( rhLast.text, flat ) )
		return RH_DROP;
	Q_strncpyz( rhLast.text, flat, sizeof( rhLast.text ) );
	rhLast.time = cls.realtime;

	// the hook and its arguments, up to the first colon; the message after it
	colon = strchr( text, ':' );
	message = colon ? colon + 1 : "";
	while ( *message == ' ' )
		message++;
	Q_strncpyz( header, text, colon ? Q_min( (int)sizeof( header ), (int)( colon - text ) + 1 ) : (int)sizeof( header ) );

	// the parts may have spaces around them, to keep the words short for the server
	for ( p = header; argc < RH_MAX_ARGS; ) {
		char *end;

		while ( *p == ' ' )
			p++;
		argv[argc++] = p;
		end = strchr( p, '/' );
		if ( end )
			*end = '\0';
		for ( char *t = p + strlen( p ); t > p && t[-1] == ' '; )
			*--t = '\0';
		if ( !end )
			break;
		p = end + 1;
	}

	if ( !Q_stricmp( argv[0], "npc" ) && argc >= 2 && !Q_stricmp( argv[1], "walk" ) ) {
		CL_NpcWalk_Hook( argv + 2, argc - 2 );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "npc" ) && argc >= 2 && !Q_stricmp( argv[1], "group" ) ) {
		CL_NpcWalk_GroupHook( argv + 2, argc - 2 );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "npc" ) )
		return RH_Npc( argv + 1, argc - 1, message );
	if ( !Q_stricmp( argv[0], "objective" ) || !Q_stricmp( argv[0], "objectives" ) )
		return RH_Objective( argv + 1, argc - 1, message );
	if ( !Q_stricmp( argv[0], "cine" ) ) {
		CL_Cinematic_Hook( argv + 1, argc - 1, message );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "cin" ) || !Q_stricmp( argv[0], "cinematic" ) ) {
		CL_RpCinematic_Hook( argv + 1, argc - 1 );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "map" ) ) {
		CL_MapEditor_Hook( argv + 1, argc - 1, message );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "mine" ) ) {
		CL_ShaderManager_MineHook( argv + 1, argc - 1, message );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "story" ) ) {
		CL_Storyteller_Hook( argv + 1, argc - 1, message );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "sky" ) ) {
		CL_ShaderManager_SkyHook( argv + 1, argc - 1 );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "fxscale" ) ) {
		CL_EffectManager_ScaleHook( argv + 1, argc - 1, message );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "anim" ) ) {
		CL_RpCinematic_AnimHook( argv + 1, argc - 1 );
		return RH_DROP;
	}
	if ( !Q_stricmp( argv[0], "startcinematic" ) || !Q_stricmp( argv[0], "endcinematic" ) ) {
		char *verb = (char *)( !Q_stricmp( argv[0], "startcinematic" ) ? "start" : "end" );

		argv[0] = verb;
		CL_RpCinematic_Hook( argv, argc );
		return RH_DROP;
	}

	Com_DPrintf( "RP hook: unknown hook \"%s\"\n", argv[0] );
	return RH_DROP;
}

// the most characters in a row without a space
int CL_RpHooks_LongestWord( const char *text ) {
	int longest = 0, run = 0;

	for ( ; *text; text++ ) {
		if ( *text == ' ' || *text == '\n' ) {
			run = 0;
			continue;
		}
		if ( ++run > longest )
			longest = run;
	}
	return longest;
}

// a command on its way to the server: qfalse keeps it here, as it would crash RPMod
qboolean CL_RpHooks_AllowCommand( const char *cmd ) {
	int longest;

	if ( Q_stricmpn( cmd, "rpcentersay ", 12 ) )
		return qtrue;
	longest = CL_RpHooks_LongestWord( cmd + 12 );
	if ( longest <= RH_MAX_WORD )
		return qtrue;
	Com_Printf( S_COLOR_RED "Not sent: rpcentersay text with a word of %i characters crashes the server (%i at most); add spaces\n",
		longest, RH_MAX_WORD );
	return qfalse;
}

/*
===============================================================================

Objectives on screen

===============================================================================
*/

static const vec4_t rhObjPanel	= { 0.0f, 0.0f, 0.0f, 0.6f };
static const vec4_t rhObjBorder	= { 1.0f, 1.0f, 1.0f, 0.25f };
static const vec4_t rhObjTitle	= { 1.0f, 0.8f, 0.3f, 1.0f };
static const vec4_t rhObjText	= { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t rhObjDone	= { 0.6f, 0.6f, 0.6f, 0.8f };

static void RH_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

// RPMod's colours: ^ and any digit or letter, where the renderer only knows the digits (and
// some of them otherwise). Taken from RPMod's own colour chart
static const byte rhPalette[62][3] = {
	{   0,   0,   0 }, { 255,   0,   0 }, {   0, 255,   0 }, { 255, 255,   0 }, {   0,   0, 255 }, {   0, 255, 255 },	// 0 1 2 3 4 5
	{ 255,   0, 255 }, { 255, 255, 255 }, { 255, 127,   0 }, { 127,   0, 204 }, {  63,   0, 255 }, {   0,   0, 140 },	// 6 7 8 9 a b
	{  64,  64,  64 }, { 255, 255, 204 }, { 239, 204,   0 }, { 255, 211,   0 }, { 255, 239,   0 }, { 255, 255, 119 },	// c d e f g h
	{ 249, 216,  94 }, { 255, 214,   0 }, { 193, 175, 145 }, { 127, 127,   0 }, { 224,  17,  94 }, { 255,  20, 147 },	// i j k l m n
	{ 255, 112, 255 }, { 102,   2,  61 }, { 119,  81, 168 }, { 112,  40,  99 }, { 119,  22,  73 }, { 142, 150, 119 },	// o p q r s t
	{ 135, 168, 107 }, {  86, 130,   2 }, { 172, 253,  45 }, {  68,  76,  56 }, {   0,  86,  63 }, { 137, 153,  91 },	// u v w x y z
	{ 150,  73,   0 }, { 255, 204, 163 }, {   0, 204, 204 }, {   0,   0, 127 }, {  79,  38, 130 }, { 255, 191, 204 },	// A B C D E F
	{   0, 122, 165 }, {   0,  73,  84 }, { 247, 130, 119 }, {   0, 127, 127 }, { 127,   0,   0 }, { 163,  89,  81 },	// G H I J K L
	{  63, 130, 109 }, { 127,   0, 255 }, { 191, 191, 191 }, {  53,  68,  79 }, { 102, 153, 204 }, { 226,  66,  51 },	// M N O P Q R
	{ 196,  96,  15 }, { 216, 142,  89 }, { 232,  96,   0 }, { 204, 204, 255 }, { 175, 224, 229 }, { 173, 216, 229 },	// S T U V W X
	{ 137, 206, 239 }, {   0,   0, 255 },	// Y Z
};

static qboolean RH_IsCode( const char *p ) {
	return (qboolean)( p[0] == Q_COLOR_ESCAPE && p[1] && isalnum( (unsigned char)p[1] ) );
}

static const byte *RH_CodeColor( char c ) {
	if ( c >= '0' && c <= '9' )
		return rhPalette[c - '0'];
	if ( c >= 'a' && c <= 'z' )
		return rhPalette[10 + c - 'a'];
	return rhPalette[36 + c - 'A'];
}

// the text as it's seen, the colour codes left out
static void RH_Plain( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( RH_IsCode( in ) ) {
			in++;
			continue;
		}
		out[len++] = *in;
	}
	out[len] = '\0';
}

static int RH_TextWidth( const char *text, int font, float scale ) {
	char plain[MAX_STRING_CHARS];

	RH_Plain( text, plain, sizeof( plain ) );
	return re->Font_StrLenPixels( plain, font, scale );
}

// drawn a stretch of one colour at a time; color is the colour before the first code, and its
// alpha goes for all of it
static void RH_DrawText( float x, float y, const char *text, const float *color, int font, float scale ) {
	char run[MAX_STRING_CHARS];
	vec4_t rgba;
	int len = 0;

	VectorCopy4( color, rgba );
	for ( ;; text++ ) {
		if ( !*text || RH_IsCode( text ) ) {
			if ( len ) {
				run[len] = '\0';
				re->Font_DrawString( (int)x, (int)y, run, rgba, font, -1, scale );
				x += re->Font_StrLenPixels( run, font, scale );
				len = 0;
			}
			if ( !*text )
				break;
			const byte *c = RH_CodeColor( *++text );
			VectorSet( rgba, c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f );
			continue;
		}
		if ( len < (int)sizeof( run ) - 1 )
			run[len++] = *text;
	}
}

// text broken at spaces to fit the width, each line's colour carried on to the next; how many lines.
// Drawn only with draw, otherwise just counted
static int RH_WrapEx( int font, float x, float y, float w, float scale, float rowH, const char *text, const float *color,
	qboolean strike, qboolean center, qboolean draw ) {
	char line[RH_OBJECTIVE_LEN + 2];
	char colorCode[3] = "";
	int lines = 0;

	while ( *text ) {
		int len = 0, fit = 0, start = strlen( colorCode );

		Q_strncpyz( line, colorCode, sizeof( line ) );
		while ( text[len] && start + len < (int)sizeof( line ) - 1 ) {
			line[start + len] = text[len];
			line[start + len + 1] = '\0';
			if ( RH_TextWidth( line, font, scale ) > w )
				break;
			if ( text[len] == ' ' || !text[len + 1] )
				fit = len + 1;
			len++;
		}
		if ( !text[len] )
			fit = len;
		if ( !fit )
			fit = Q_max( len, 1 );
		line[start + fit] = '\0';

		if ( draw ) {
			const float ly = y + lines * rowH;
			const float lw = (float)RH_TextWidth( line, font, scale );
			const float lx = center ? x + ( w - lw ) * 0.5f : x;

			RH_DrawText( lx, ly, line, color, font | STYLE_DROPSHADOW, scale );
			if ( strike )
				RH_Fill( lx, ly + re->Font_HeightPixels( font, scale ) * 0.55f, lw, 1.0f, color );
		}
		for ( int i = 0; i < fit; i++ ) {
			if ( RH_IsCode( text + i ) ) {
				colorCode[0] = text[i];
				colorCode[1] = text[i + 1];
				colorCode[2] = '\0';
			}
		}
		lines++;
		text += fit;
		while ( *text == ' ' )
			text++;
	}
	return lines;
}

static int RH_Wrap( int font, float x, float y, const char *text, const float *color, qboolean strike, qboolean draw ) {
	return RH_WrapEx( font, x, y, RH_OBJ_W - RH_OBJ_PAD * 2, RH_OBJ_SCALE, RH_OBJ_ROW_H, text, color, strike, qfalse, draw );
}

static int RH_Font( void ) {
	const int font = re->RegisterFont( "arialnb" );
	return font ? font : cls.menuFont;
}

// how RPMod's chat box shows chat from this far away (its rpc_chatRange_* cvars): qfalse
// if it leaves it out, else how solid
static qboolean RH_ChatRange( float distance, float *alpha ) {
	const int mode = Cvar_VariableIntegerValue( "rpc_chatRange_mode" );
	const float min = Cvar_VariableValue( "rpc_chatRange_min" );
	const float max = Cvar_VariableValue( "rpc_chatRange_max" );
	const float minAlpha = Com_Clamp( 0.0f, 100.0f, Cvar_VariableValue( "rpc_chatRange_alpha" ) ) / 100.0f;

	*alpha = 1.0f;
	if ( mode == 3 )
		return (qboolean)( distance <= min );
	if ( mode == 2 && distance > max )
		return qfalse;
	if ( ( mode != 1 && mode != 2 ) || distance <= min )
		return qtrue;
	*alpha = 1.0f - ( max > min ? Q_min( 1.0f, ( distance - min ) / ( max - min ) ) : 1.0f ) * ( 1.0f - minAlpha );
	return qtrue;
}

// a player's chat about to reach the cgame: during a cinematic with the HUD hidden it's a
// subtitle too, as NPC talk is. "chat" "name\x19: message" <distance>, "lchat" "name"
// "location" "colour" "message" <distance>; the distance is RPMod's, elsewhere a client number
void CL_RpHooks_ChatSubtitle( void ) {
	const char *cmd = Cmd_Argv( 0 ), *num;
	char name[MAX_STRING_CHARS], message[MAX_STRING_CHARS];
	float alpha = 1.0f;

	if ( !CL_RpCinematic_Subtitles() )
		return;
	if ( !strcmp( cmd, "chat" ) || !strcmp( cmd, "tchat" ) ) {
		const char *text = Cmd_Argv( 1 ), *sep = strstr( text, "\x19:" );

		if ( Cmd_Argc() < 2 || !text[0] )
			return;
		num = Cmd_Argv( 2 );
		if ( sep ) {
			Q_strncpyz( name, text, Q_min( (int)sizeof( name ), (int)( sep - text ) + 1 ) );
			text = sep + 2;
		} else {
			name[0] = '\0';
		}
		while ( *text == ' ' )
			text++;
		Q_strncpyz( message, text, sizeof( message ) );
	} else if ( !strcmp( cmd, "lchat" ) || !strcmp( cmd, "ltchat" ) ) {
		if ( Cmd_Argc() < 5 || !Cmd_Argv( 4 )[0] )
			return;
		Q_strncpyz( name, Cmd_Argv( 1 ), sizeof( name ) );
		Q_strncpyz( message, Cmd_Argv( 4 ), sizeof( message ) );
		num = Cmd_Argv( 5 );
	} else {
		return;
	}

	if ( !Q_stricmp( FS_GetCurrentGameDir(), "rpmod" ) && isdigit( (unsigned char)num[0] ) && !RH_ChatRange( (float)atoi( num ), &alpha ) )
		return;
	// green as the chat is, unless it has colour codes of its own, which come after and win
	if ( name[0] )
		RH_AddSubtitle( va( "%s" S_COLOR_WHITE ": " S_COLOR_GREEN "%s", name, message ), alpha );
	else
		RH_AddSubtitle( va( S_COLOR_GREEN "%s", message ), alpha );
}

// NPCs talking and players' chat, at the bottom of the screen: the cinematic hides the chat
// box with the HUD
void CL_RpHooks_DrawSubtitles( void ) {
	const int font = RH_Font();
	const float rowH = re->Font_HeightPixels( font, RH_SUB_SCALE ) + 2.0f;
	const float x = ( SCREEN_WIDTH - RH_SUB_W ) * 0.5f;
	float y = SCREEN_HEIGHT - RH_SUB_BOTTOM;

	// the newest at the bottom, the ones before it above
	for ( int i = RH_MAX_SUBS - 1; i >= 0; i-- ) {
		vec4_t color = { 1.0f, 1.0f, 1.0f, 1.0f };
		const int left = rhSubs[i].until - cls.realtime;

		if ( !rhSubs[i].text[0] || left <= 0 )
			continue;
		color[3] = rhSubs[i].alpha;
		if ( left < RH_SUB_FADE_MS )
			color[3] *= (float)left / RH_SUB_FADE_MS;
		y -= RH_WrapEx( font, 0, 0, RH_SUB_W, RH_SUB_SCALE, rowH, rhSubs[i].text, color, qfalse, qtrue, qfalse ) * rowH;
		RH_WrapEx( font, x, y, RH_SUB_W, RH_SUB_SCALE, rowH, rhSubs[i].text, color, qfalse, qtrue, qtrue );
		y -= 2.0f;
	}
}

// the objectives, while the menu is up in game
void CL_RpHooks_Draw( void ) {
	char plain[RH_OBJECTIVE_LEN];
	float x = RH_OBJ_X + RH_OBJ_PAD, y, h;
	int font, lines = 1;

	if ( !rhObj.count || cls.state != CA_ACTIVE || !( Key_GetCatcher() & KEYCATCH_UI ) )
		return;
	font = RH_Font();

	for ( int i = 0; i < rhObj.count; i++ )
		lines += RH_Wrap( font, 0, 0, va( RH_OBJ_BULLET "%s", rhObj.items[i].text ), rhObjText, qfalse, qfalse );
	h = lines * RH_OBJ_ROW_H + RH_OBJ_PAD * 2 + 4;
	RH_Fill( RH_OBJ_X, RH_OBJ_Y, RH_OBJ_W, h, rhObjPanel );
	RH_Fill( RH_OBJ_X, RH_OBJ_Y, RH_OBJ_W, 1, rhObjBorder );
	RH_Fill( RH_OBJ_X, RH_OBJ_Y + h - 1, RH_OBJ_W, 1, rhObjBorder );
	RH_Fill( RH_OBJ_X, RH_OBJ_Y, 1, h, rhObjBorder );
	RH_Fill( RH_OBJ_X + RH_OBJ_W - 1, RH_OBJ_Y, 1, h, rhObjBorder );

	y = RH_OBJ_Y + RH_OBJ_PAD;
	re->Font_DrawString( (int)x, (int)y, rhObj.title[0] ? rhObj.title : RH_OBJ_TITLE, rhObjTitle, font | STYLE_DROPSHADOW, -1, RH_OBJ_SCALE );
	y += RH_OBJ_ROW_H + 4;

	for ( int i = 0; i < rhObj.count; i++ ) {
		if ( rhObj.items[i].done ) {
			// struck through in grey, its own colours left out
			Q_strncpyz( plain, rhObj.items[i].text, sizeof( plain ) );
			Q_StripColor( plain );
			y += RH_Wrap( font, x, y, va( RH_OBJ_BULLET "%s", plain ), rhObjDone, qtrue, qtrue ) * RH_OBJ_ROW_H;
		} else {
			y += RH_Wrap( font, x, y, va( RH_OBJ_BULLET "%s", rhObj.items[i].text ), rhObjText, qfalse, qtrue ) * RH_OBJ_ROW_H;
		}
	}
}

// objective: lists them; objective <verb> [text]: sends one to everyone, hidden in an rpnotify
static void CL_Objective_f( void ) {
	static const char *verbs[] = { "add", "done", "undone", "remove", "clear", "title" };
	const char *verb = Cmd_Argv( 1 ), *text = Cmd_ArgsFrom( 2 );
	qboolean known = qfalse;

	if ( Cmd_Argc() < 2 || !Q_stricmp( verb, "list" ) ) {
		Com_Printf( "%s\n", rhObj.title[0] ? rhObj.title : RH_OBJ_TITLE );
		for ( int i = 0; i < rhObj.count; i++ ) {
			char plain[RH_OBJECTIVE_LEN];

			if ( !rhObj.items[i].done ) {
				Com_Printf( "%2i. %s\n", i + 1, rhObj.items[i].text );
				continue;
			}
			Q_strncpyz( plain, rhObj.items[i].text, sizeof( plain ) );
			Q_StripColor( plain );
			Com_Printf( "%2i. " S_COLOR_GREY "%s (done)\n", i + 1, plain );
		}
		if ( !rhObj.count )
			Com_Printf( "  none\n" );
		return;
	}

	for ( size_t i = 0; i < ARRAY_LEN( verbs ); i++ ) {
		if ( !Q_stricmp( verb, verbs[i] ) )
			known = qtrue;
	}
	if ( !known || ( !text[0] && Q_stricmp( verb, "clear" ) && Q_stricmp( verb, "title" ) ) ) {
		Com_Printf( "objective [list] : the objectives\n"
			"objective add <text> : a new one, for everyone with this client\n"
			"objective done|undone|remove <number or text> : strike one through, take the line off, or take it away\n"
			"objective clear [done] : all of them, or the struck ones\n"
			"objective title [text] : the heading, \"" RH_OBJ_TITLE "\" if none\n" );
		return;
	}
	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n" );
		return;
	}
	// the server takes a // for a comment
	if ( strstr( text, "//" ) ) {
		Com_Printf( S_COLOR_YELLOW "Not sent: the server would cut the text at the //\n" );
		return;
	}
	CL_AddReliableCommand( text[0] ? va( "rpnotify -1 1 " RH_PREFIX "objective/%s: %s", verb, text )
		: va( "rpnotify -1 1 " RH_PREFIX "objective/%s", verb ), qfalse );
}

void CL_RpHooks_Init( void ) {
	memset( &rhObj, 0, sizeof( rhObj ) );
	cl_rpExperimental = Cvar_Get( "cl_rpExperimental", "0", CVAR_ARCHIVE, "Experimental features on as the game starts (rpexperimental)" );
	rhExperimental = (qboolean)( cl_rpExperimental->integer != 0 );
	Cmd_AddCommand( "objective", CL_Objective_f, "Lists the mission objectives, or sends one to everyone with this client" );
	Cmd_AddCommand( "rpexperimental", CL_RpExperimental_f, "Turns experimental features on or off, and keeps them so every time the game starts" );
}

void CL_RpHooks_Shutdown( void ) {
	Cmd_RemoveCommand( "objective" );
	Cmd_RemoveCommand( "rpexperimental" );
}

// a new server is a new story
void CL_RpHooks_Disconnect( void ) {
	memset( &rhObj, 0, sizeof( rhObj ) );
	memset( &rhLast, 0, sizeof( rhLast ) );
	memset( &rhSubs, 0, sizeof( rhSubs ) );
}

// the server command just tokenized: qfalse keeps it from the cgame
qboolean CL_RpHooks_ServerCommand( qboolean *replaced ) {
	const char *cmd = Cmd_Argv( 0 );
	rhResult_t result;

	*replaced = qfalse;
	// rpnotify sends its text to the console (print) and to RPMod's banner (info), rpcentersay
	// to the middle of the screen (cp); the first to arrive runs the hook, the copies are hidden
	if ( strcmp( cmd, "print" ) && strcmp( cmd, "cp" ) && strcmp( cmd, "cps" ) && strcmp( cmd, "info" ) )
		return qtrue;
	for ( int i = 1; i < Cmd_Argc(); i++ ) {
		if ( !RH_HookText( Cmd_Argv( i ) ) )
			continue;
		result = RH_Run( Cmd_Argv( i ) );
		*replaced = (qboolean)( result == RH_REPLACED );
		return (qboolean)( result != RH_DROP );
	}
	return qtrue;
}
