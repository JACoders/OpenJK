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

// cl_languages.cpp -- the languages a character knows, in RP chat
//
// Chat is Basic, which everyone knows, unless it's written as
//   ^3::Language:: << what is said >>
// Each of the player's characters knows some languages: it speaks them, or only understands
// them. What's said in a language the character playing doesn't know shows as gibberish
// (the same words always the same gibberish, each language its own), in the chat box, the
// console and the cinematic subtitles alike; the language's name still shows. A << >> with no
// name before it is in the language named last in the line, or Basic.
//
// With a language picked to speak, what the player says goes out in it: say <text> becomes
// say ^3::Language:: << text >>. Text already written that way, or OOC in (( )), goes as it
// is. Team chat and private messages are never changed.
//
//   languages                          the window: characters, their languages, which is spoken
//   language                           which character, what it knows, what it speaks
//   language select <language>         speak it; language basic speaks Basic again
//   language say <language> <text>    says one line in it
//   language add <language> [understand], language remove <language>
//   language character <name>          play as that character (made if new); language characters
//
// Nothing is gibberish until a character is made: players who don't use it see chat as before.
// Kept in languages.txt in the game folder.

#include "client.h"
#include "cl_widgets.h"

#define LG_FILE				"languages.txt"
#define LG_COMMAND			"languages"
#define LG_MAX_CHARS		32
#define LG_MAX_LANGS		48
#define LG_NAME_LEN			32
#define LG_BASIC			"Basic"
#define LG_LABEL_COLOR		S_COLOR_YELLOW
#define LG_LABEL_MAX		40		// a ::name:: longer than this isn't one
#define LG_TEXT_SCALE		0.8f
#define LG_ROW_H			13.0f
#define LG_CTRL_H			15.0f
#define LG_LIST_Y			66.0f
#define LG_LIST_H			208.0f
#define LG_CHAR_X			16.0f
#define LG_CHAR_W			150.0f
#define LG_LANG_X			176.0f
#define LG_LANG_W			300.0f
#define LG_CHECK_W			60.0f		// a checkbox column in the languages list
#define LG_EDIT_Y			282.0f
#define LG_MESSAGE_MS		5000
#define LG_CONFIRM_MS		3000
#define LG_SAMPLE			"Hello there. Have you seen the ship that came in from Corellia?"

typedef struct {
	char		name[LG_NAME_LEN];
	qboolean	speak;			// understood either way
} lgLang_t;

typedef struct {
	char		name[LG_NAME_LEN];
	lgLang_t	langs[LG_MAX_LANGS];
	int			numLangs;
	char		spoken[LG_NAME_LEN];	// "" is Basic
} lgChar_t;

static lgChar_t		lgChars[LG_MAX_CHARS];
static int			lgNumChars;
static int			lgActive = -1;		// the character played, -1 none
static qboolean		lgLoaded;

typedef enum {
	LG_F_NONE,
	LG_F_CHARNAME,
	LG_F_NEWLANG
} lgField_t;

// the window
static struct {
	qboolean	open;
	int			font;
	float		cursorX, cursorY;
	float		clickX, clickY;
	qboolean	click;
	int			wheel;
	int			charShownSel, langShownSel;	// the selections last kept in view: only a new one is again
	int			charScroll;
	int			langSel, langScroll;
	lgField_t	focus;
	char		nameBefore[LG_NAME_LEN];
	char		newLang[LG_NAME_LEN];
	int			confirmTime;
	int			confirmWhat;		// 1 deleting the character, 2 removing the language
	char		message[MAX_STRING_CHARS];
	int			messageTime;
} lg;

static const vec4_t lgWhite			= { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t lgDim			= { 0.7f, 0.7f, 0.7f, 1.0f };
static const vec4_t lgPanel			= { 0.0f, 0.0f, 0.0f, 0.65f };
static const vec4_t lgPanelLight	= { 0.15f, 0.15f, 0.15f, 0.8f };
static const vec4_t lgPanelFocus	= { 0.1f, 0.2f, 0.3f, 0.9f };
static const vec4_t lgHighlight		= { 0.2f, 0.45f, 0.8f, 0.8f };
static const vec4_t lgHover			= { 1.0f, 1.0f, 1.0f, 0.12f };
static const vec4_t lgHoverBox		= { 0.25f, 0.25f, 0.25f, 0.9f };
static const vec4_t lgDanger		= { 0.6f, 0.15f, 0.15f, 0.9f };
static const vec4_t lgBorder		= { 0.5f, 0.5f, 0.5f, 0.8f };
static const vec4_t lgCheck			= { 1.0f, 0.8f, 0.3f, 1.0f };

/*
===============================================================================

CHARACTERS AND THEIR FILE

===============================================================================
*/

static lgChar_t *LG_Active( void ) {
	return lgActive >= 0 && lgActive < lgNumChars ? &lgChars[lgActive] : NULL;
}

static int LG_FindChar( const char *name ) {
	for ( int i = 0; i < lgNumChars; i++ ) {
		if ( !Q_stricmp( lgChars[i].name, name ) )
			return i;
	}
	return -1;
}

static int LG_FindLang( const lgChar_t *c, const char *name ) {
	for ( int i = 0; c && i < c->numLangs; i++ ) {
		if ( !Q_stricmp( c->langs[i].name, name ) )
			return i;
	}
	return -1;
}

static lgChar_t *LG_AddChar( const char *name ) {
	lgChar_t *c;

	if ( lgNumChars >= LG_MAX_CHARS || !name[0] )
		return NULL;
	c = &lgChars[lgNumChars++];
	memset( c, 0, sizeof( *c ) );
	Q_strncpyz( c->name, name, sizeof( c->name ) );
	return c;
}

static lgLang_t *LG_AddLang( lgChar_t *c, const char *name, qboolean speak ) {
	lgLang_t *l;
	int i;

	if ( !c || !name[0] || !Q_stricmp( name, LG_BASIC ) )
		return NULL;
	i = LG_FindLang( c, name );
	if ( i >= 0 ) {
		c->langs[i].speak = speak;
		return &c->langs[i];
	}
	if ( c->numLangs >= LG_MAX_LANGS )
		return NULL;
	l = &c->langs[c->numLangs++];
	Q_strncpyz( l->name, name, sizeof( l->name ) );
	l->speak = speak;
	return l;
}

static void LG_Save( void ) {
	fileHandle_t f = FS_FOpenFileWrite( LG_FILE );

	if ( !f ) {
		Com_Printf( S_COLOR_RED "Languages: couldn't write " LG_FILE "\n" );
		return;
	}
	FS_Printf( f, "// Languages: character \"<name>\", then its languages, language \"<name>\" speak|understand,\n" );
	FS_Printf( f, "// and spoken \"<language>\" for the one it speaks; active \"<character>\" is the one played\n" );
	if ( LG_Active() )
		FS_Printf( f, "active \"%s\"\n", LG_Active()->name );
	for ( int i = 0; i < lgNumChars; i++ ) {
		const lgChar_t *c = &lgChars[i];

		FS_Printf( f, "character \"%s\"\n", c->name );
		for ( int j = 0; j < c->numLangs; j++ )
			FS_Printf( f, "language \"%s\" %s\n", c->langs[j].name, c->langs[j].speak ? "speak" : "understand" );
		if ( c->spoken[0] )
			FS_Printf( f, "spoken \"%s\"\n", c->spoken );
	}
	FS_FCloseFile( f );
}

// splits a line into words, "quoted" ones whole; the line is cut up in place
static int LG_SplitLine( char *line, char **argv, int max ) {
	int argc = 0;

	while ( *line && argc < max ) {
		while ( *line == ' ' || *line == '\t' )
			line++;
		if ( !*line )
			break;
		if ( *line == '"' ) {
			argv[argc++] = ++line;
			while ( *line && *line != '"' )
				line++;
		} else {
			argv[argc++] = line;
			while ( *line && *line != ' ' && *line != '\t' )
				line++;
		}
		if ( *line )
			*line++ = '\0';
	}
	return argc;
}

static void LG_Load( void ) {
	char *buf, *line, *next, *argv[3], active[LG_NAME_LEN] = "";
	lgChar_t *c = NULL;
	int len;

	if ( lgLoaded )
		return;
	lgLoaded = qtrue;
	lgNumChars = 0;
	lgActive = -1;
	len = FS_ReadFile( LG_FILE, (void **)&buf );
	if ( len < 0 || !buf )
		return;
	for ( line = buf; line && *line; line = next ) {
		next = strchr( line, '\n' );
		if ( next )
			*next++ = '\0';
		len = strlen( line );
		if ( len && line[len - 1] == '\r' )
			line[len - 1] = '\0';
		if ( line[0] == '/' && line[1] == '/' )
			continue;
		const int argc = LG_SplitLine( line, argv, ARRAY_LEN( argv ) );
		if ( argc >= 2 && !Q_stricmp( argv[0], "active" ) )
			Q_strncpyz( active, argv[1], sizeof( active ) );
		else if ( argc >= 2 && !Q_stricmp( argv[0], "character" ) )
			c = LG_FindChar( argv[1] ) >= 0 ? NULL : LG_AddChar( argv[1] );
		else if ( argc >= 2 && !Q_stricmp( argv[0], "language" ) )
			LG_AddLang( c, argv[1], (qboolean)( argc < 3 || Q_stricmp( argv[2], "understand" ) ) );
		else if ( argc >= 2 && !Q_stricmp( argv[0], "spoken" ) && c )
			Q_strncpyz( c->spoken, argv[1], sizeof( c->spoken ) );
	}
	FS_FreeFile( buf );
	lgActive = LG_FindChar( active );
	if ( lgActive < 0 && lgNumChars )
		lgActive = 0;
}

// the language the character playing speaks, "" for Basic
static const char *LG_Spoken( void ) {
	const lgChar_t *c = LG_Active();
	int i;

	if ( !c || !c->spoken[0] )
		return "";
	i = LG_FindLang( c, c->spoken );
	return i >= 0 && c->langs[i].speak ? c->langs[i].name : "";
}

// what the character playing understands; with none made, everything
static qboolean LG_Understands( const char *name ) {
	const lgChar_t *c;

	LG_Load();
	c = LG_Active();
	return (qboolean)( !c || !name[0] || !Q_stricmp( name, LG_BASIC ) || LG_FindLang( c, name ) >= 0 );
}

/*
===============================================================================

GIBBERISH

===============================================================================
*/

static unsigned LG_Hash( const char *s, int len ) {
	unsigned h = 2166136261u;

	for ( int i = 0; i < len && s[i]; i++ ) {
		h ^= (unsigned)tolower( (unsigned char)s[i] );
		h *= 16777619u;
	}
	return h;
}

static unsigned LG_Next( unsigned *state ) {
	unsigned x = *state ? *state : 0x9e3779b9u;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

// a language's sounds: some consonants and vowels of its own, and how its syllables go
typedef struct {
	char		consonants[12];
	char		vowels[6];
	int			numConsonants, numVowels;
	int			closed;			// in 100, how many syllables end in a consonant
	int			bare;			// in 100, how many start with the vowel
} lgSounds_t;

static void LG_Sounds( const char *lang, lgSounds_t *s ) {
	char consonants[] = "bcdfghjklmnprstvwxzq", vowels[] = "aeiouy";
	unsigned state = LG_Hash( lang, LG_NAME_LEN ) | 1u;
	const int nc = (int)strlen( consonants ), nv = (int)strlen( vowels );

	// a shuffle of each, the first few kept
	for ( int i = nc - 1; i > 0; i-- ) {
		const int j = (int)( LG_Next( &state ) % ( i + 1 ) );
		const char t = consonants[i];
		consonants[i] = consonants[j];
		consonants[j] = t;
	}
	for ( int i = nv - 1; i > 0; i-- ) {
		const int j = (int)( LG_Next( &state ) % ( i + 1 ) );
		const char t = vowels[i];
		vowels[i] = vowels[j];
		vowels[j] = t;
	}
	s->numConsonants = 6 + (int)( LG_Next( &state ) % 6 );
	s->numVowels = 2 + (int)( LG_Next( &state ) % 3 );
	memcpy( s->consonants, consonants, s->numConsonants );
	memcpy( s->vowels, vowels, s->numVowels );
	s->closed = 10 + (int)( LG_Next( &state ) % 50 );
	s->bare = (int)( LG_Next( &state ) % 25 );
}

// a word of the same length in the language, its capitals where the word's were
static void LG_Word( const lgSounds_t *s, const char *word, int len, unsigned langSeed, char *out ) {
	unsigned state = ( langSeed ^ LG_Hash( word, len ) ) | 1u;
	int n = 0;

	while ( n < len ) {
		const unsigned r = LG_Next( &state );

		if ( n && (int)( r % 100 ) < s->bare ) {
			out[n++] = s->vowels[( r >> 8 ) % s->numVowels];
			continue;
		}
		out[n++] = s->consonants[( r >> 8 ) % s->numConsonants];
		if ( n < len )
			out[n++] = s->vowels[( r >> 16 ) % s->numVowels];
		if ( n < len && (int)( ( r >> 24 ) % 100 ) < s->closed )
			out[n++] = s->consonants[( r >> 4 ) % s->numConsonants];
	}
	for ( int i = 0; i < len; i++ ) {
		if ( isupper( (unsigned char)word[i] ) )
			out[i] = (char)toupper( (unsigned char)out[i] );
	}
}

// what's said, as it sounds to someone who doesn't know the language: words turned, the rest kept
static void LG_Gibberish( const char *lang, const char *in, int len, char *out, int size ) {
	lgSounds_t sounds;
	const unsigned seed = LG_Hash( lang, LG_NAME_LEN );
	int o = 0;

	LG_Sounds( lang, &sounds );
	for ( int i = 0; i < len && o < size - 1; ) {
		if ( in[i] == Q_COLOR_ESCAPE && i + 1 < len && in[i + 1] && in[i + 1] != Q_COLOR_ESCAPE ) {
			out[o++] = in[i++];
			if ( o < size - 1 )
				out[o++] = in[i++];
			continue;
		}
		if ( isalpha( (unsigned char)in[i] ) ) {
			int n = 0;

			while ( i + n < len && isalpha( (unsigned char)in[i + n] ) )
				n++;
			if ( o + n >= size )
				n = size - 1 - o;
			LG_Word( &sounds, in + i, n, seed, out + o );
			o += n;
			i += n;
			continue;
		}
		out[o++] = in[i++];
	}
	out[o] = '\0';
}

// a ::name:: at text, its name in out with the colours left out; how long it is, 0 if it isn't one
static int LG_Label( const char *text, char *out, int size ) {
	int i = 2, len = 0;

	if ( text[0] != ':' || text[1] != ':' )
		return 0;
	while ( text[i] && i < LG_LABEL_MAX + 2 ) {
		if ( text[i] == ':' && text[i + 1] == ':' )
			break;
		if ( text[i] == '<' || text[i] == '>' || text[i] == '\n' )
			return 0;
		i++;
	}
	if ( text[i] != ':' || text[i + 1] != ':' || i == 2 )
		return 0;
	for ( int j = 2; j < i && len < size - 1; j++ ) {
		if ( text[j] == Q_COLOR_ESCAPE && j + 1 < i && text[j + 1] != Q_COLOR_ESCAPE ) {
			j++;
			continue;
		}
		out[len++] = text[j];
	}
	out[len] = '\0';
	// the spaces around it
	while ( len && out[len - 1] == ' ' )
		out[--len] = '\0';
	while ( out[0] == ' ' )
		memmove( out, out + 1, len-- );
	return out[0] ? i + 2 : 0;
}

// an emote at text, ^7:: what the character does ::, never in a language and never a ::name::;
// how long it is, to the end of the line if it isn't closed, 0 if it isn't one
static int LG_Emote( const char *text ) {
	const char *end;

	if ( strncmp( text, S_COLOR_WHITE "::", 4 ) )
		return 0;
	end = strstr( text + 4, "::" );
	return end ? (int)( end + 2 - text ) : (int)strlen( text );
}

// a line of chat with what the character playing doesn't know turned to gibberish; qfalse if
// nothing was
static qboolean LG_Filter( const char *in, char *out, int size ) {
	char lang[LG_NAME_LEN] = "", label[LG_LABEL_MAX + 1];
	qboolean changed = qfalse;
	int o = 0;

	for ( const char *p = in; *p && o < size - 1; ) {
		const int emote = LG_Emote( p );
		const int n = emote ? 0 : LG_Label( p, label, sizeof( label ) );

		if ( emote ) {
			for ( int i = 0; i < emote && o < size - 1; i++ )
				out[o++] = *p++;
			continue;
		}
		if ( n ) {
			Q_strncpyz( lang, label, sizeof( lang ) );
			for ( int i = 0; i < n && o < size - 1; i++ )
				out[o++] = *p++;
			continue;
		}
		if ( p[0] == '<' && p[1] == '<' ) {
			const char *end = strstr( p + 2, ">>" );
			const int len = end ? (int)( end - ( p + 2 ) ) : (int)strlen( p + 2 );

			out[o++] = *p++;
			if ( o < size - 1 )
				out[o++] = *p++;
			if ( lang[0] && !LG_Understands( lang ) ) {
				char said[MAX_STRING_CHARS];

				LG_Gibberish( lang, p, Q_min( len, (int)sizeof( said ) - 1 ), said, sizeof( said ) );
				for ( const char *s = said; *s && o < size - 1; )
					out[o++] = *s++;
				changed = qtrue;
			} else {
				for ( int i = 0; i < len && o < size - 1; i++ )
					out[o++] = p[i];
			}
			p += len;
			continue;
		}
		out[o++] = *p++;
	}
	out[o] = '\0';
	return changed;
}

// text as the character playing hears it, for what shows it outside the chat (subtitles)
const char *CL_Languages_Filter( const char *text ) {
	static char out[MAX_STRING_CHARS];

	if ( !LG_Filter( text, out, sizeof( out ) ) )
		return text;
	return out;
}

/*
===============================================================================

CHAT

===============================================================================
*/

// what's said, in the language, the emotes left out of it:
// ^3::Language:: << said >> ^7:: emote :: ^3<< said >>; NULL if there are only emotes.
// What's said keeps the colours typed in it, the << >> stay in the language's
static const char *LG_InLanguage( const char *lang, const char *text ) {
	static char out[MAX_STRING_CHARS];
	qboolean afterEmote = qfalse, spoke = qfalse;

	Com_sprintf( out, sizeof( out ), LG_LABEL_COLOR "::%s::", lang );
	for ( const char *p = text; *p; ) {
		const int emote = LG_Emote( p );
		const char *start, *end;

		if ( emote ) {
			Q_strcat( out, sizeof( out ), va( " %.*s", emote, p ) );
			p += emote;
			afterEmote = qtrue;
			continue;
		}
		// what's said up to the next emote, without the spaces around it nor colours at its end
		for ( end = p; *end && !LG_Emote( end ); end++ )
			;
		for ( start = p; start < end && *start == ' '; )
			start++;
		p = end;
		while ( end > start ) {
			if ( end[-1] == ' ' )
				end--;
			else if ( end - 2 >= start && end[-2] == Q_COLOR_ESCAPE && end[-1] != Q_COLOR_ESCAPE )
				end -= 2;
			else
				break;
		}
		// nothing said, or only colours
		{
			const char *t = start;

			while ( t < end && ( *t == ' ' || ( t[0] == Q_COLOR_ESCAPE && t + 1 < end && t[1] != Q_COLOR_ESCAPE ) ) )
				t += *t == ' ' ? 1 : 2;
			if ( t == end )
				continue;
		}
		// the << >> in the language's colour, after an emote or a colour typed; what's said without
		// a colour of its own in the language's too
		{
			qboolean colored = qfalse;

			for ( const char *t = start; t + 1 < end; t++ ) {
				if ( t[0] == Q_COLOR_ESCAPE && t[1] != Q_COLOR_ESCAPE )
					colored = qtrue;
			}
			Q_strcat( out, sizeof( out ), va( " %s<< %.*s%s >>", afterEmote ? LG_LABEL_COLOR : "",
				(int)( end - start ), start, colored ? LG_LABEL_COLOR : "" ) );
		}
		afterEmote = qfalse;
		spoke = qtrue;
	}
	return spoke ? out : NULL;
}

// a command on its way to the server: say, with a language spoken, is said in it
const char *CL_Languages_OutgoingChat( const char *cmd ) {
	static char out[MAX_STRING_CHARS];
	char text[MAX_STRING_CHARS];
	const char *lang, *said;
	int len;

	// a bind or a script can leave spaces before it
	while ( *cmd == ' ' )
		cmd++;
	if ( Q_stricmpn( cmd, "say ", 4 ) )
		return cmd;
	LG_Load();
	lang = LG_Spoken();
	if ( !lang[0] )
		return cmd;
	// the console's say may have it in quotes, the chat box's doesn't
	for ( said = cmd + 4; *said == ' '; )
		said++;
	Q_strncpyz( text, said, sizeof( text ) );
	len = strlen( text );
	if ( len >= 2 && text[0] == '"' && text[len - 1] == '"' ) {
		text[len - 1] = '\0';
		memmove( text, text + 1, len - 1 );
	}
	// nothing to say, already in a language, or out of character
	if ( !text[0] || strstr( text, "<<" ) || !strncmp( text, "((", 2 ) )
		return cmd;
	// only emotes
	if ( !( said = LG_InLanguage( lang, text ) ) )
		return cmd;
	Com_sprintf( out, sizeof( out ), "say %s", said );
	return out;
}

// the chat command just tokenized: what the character doesn't know turned to gibberish before the
// cgame (and the console, the chat box and the subtitles after it) sees it.
// "chat" "name\x19: message" ..., "lchat" "name" "location" "colour" "message" ...
void CL_Languages_ServerCommand( void ) {
	const char *cmd = Cmd_Argv( 0 );
	char rebuilt[BIG_INFO_STRING], filtered[MAX_STRING_CHARS];
	int argc = Cmd_Argc(), at;

	if ( !strcmp( cmd, "chat" ) || !strcmp( cmd, "tchat" ) )
		at = 1;
	else if ( !strcmp( cmd, "lchat" ) || !strcmp( cmd, "ltchat" ) )
		at = 4;
	else
		return;
	if ( argc <= at )
		return;
	LG_Load();
	if ( !LG_Active() )
		return;

	{
		const char *arg = Cmd_Argv( at ), *sep = at == 1 ? strstr( arg, "\x19:" ) : NULL;
		const int keep = sep ? (int)( sep - arg ) + 2 : 0;
		char said[MAX_STRING_CHARS];

		// the name stays as it is; it can have colons of its own
		if ( !LG_Filter( arg + keep, said, sizeof( said ) ) )
			return;
		Com_sprintf( filtered, sizeof( filtered ), "%.*s%s", keep, arg, said );
	}

	Q_strncpyz( rebuilt, cmd, sizeof( rebuilt ) );
	for ( int i = 1; i < argc; i++ ) {
		char part[MAX_STRING_CHARS];

		Q_strncpyz( part, i == at ? filtered : Cmd_Argv( i ), sizeof( part ) );
		for ( char *c = part; *c; c++ ) {
			if ( *c == '"' )
				*c = '\'';
		}
		Q_strcat( rebuilt, sizeof( rebuilt ), va( " \"%s\"", part ) );
	}
	Cmd_TokenizeString( rebuilt );
}

/*
===============================================================================

COMMANDS

===============================================================================
*/

static void LG_Status( void ) {
	const lgChar_t *c = LG_Active();
	const char *spoken = LG_Spoken();

	if ( !c ) {
		Com_Printf( "Languages: no character yet; language character <name> makes one, or /" LG_COMMAND " opens the window\n" );
		return;
	}
	Com_Printf( "Playing " S_COLOR_YELLOW "%s" S_COLOR_WHITE ", speaking " S_COLOR_YELLOW "%s\n", c->name, spoken[0] ? spoken : LG_BASIC );
	if ( !c->numLangs )
		Com_Printf( "  knows only Basic\n" );
	for ( int i = 0; i < c->numLangs; i++ )
		Com_Printf( "  %s  " S_COLOR_GREY "%s\n", c->langs[i].name, c->langs[i].speak ? "speaks" : "understands" );
}

// the character played; made if there's none yet
static lgChar_t *LG_EnsureChar( void ) {
	if ( !LG_Active() ) {
		lgChar_t *c = LG_AddChar( "Me" );

		if ( !c )
			return NULL;
		lgActive = lgNumChars - 1;
		Com_Printf( "Languages: made a character called Me; " S_COLOR_GREY "language character <name> to play as another\n" );
	}
	return LG_Active();
}

static qboolean LG_Select( const char *name ) {
	lgChar_t *c = LG_Active();
	int i;

	if ( !Q_stricmp( name, LG_BASIC ) || !Q_stricmp( name, "none" ) || !name[0] ) {
		if ( c ) {
			c->spoken[0] = '\0';
			LG_Save();
		}
		Com_Printf( "You speak " S_COLOR_YELLOW LG_BASIC S_COLOR_WHITE " again\n" );
		return qtrue;
	}
	i = LG_FindLang( c, name );
	if ( !c || i < 0 || !c->langs[i].speak ) {
		Com_Printf( S_COLOR_YELLOW "%s doesn't speak %s; " S_COLOR_GREY "language add %s adds it\n", c ? c->name : "Your character", name, name );
		return qfalse;
	}
	Q_strncpyz( c->spoken, c->langs[i].name, sizeof( c->spoken ) );
	LG_Save();
	Com_Printf( "You speak " S_COLOR_YELLOW "%s" S_COLOR_WHITE " now; " S_COLOR_GREY "language basic to go back\n", c->spoken );
	return qtrue;
}

static void LG_Open( void );

static void LG_Language_f( void ) {
	const char *arg = Cmd_Argv( 1 );
	lgChar_t *c;

	LG_Load();
	if ( Cmd_Argc() < 2 ) {
		LG_Status();
		return;
	}
	if ( !Q_stricmp( arg, "select" ) || !Q_stricmp( arg, "speak" ) ) {
		LG_Select( Cmd_ArgsFrom( 2 ) );
	} else if ( !Q_stricmp( arg, "basic" ) || !Q_stricmp( arg, "none" ) ) {
		LG_Select( LG_BASIC );
	} else if ( !Q_stricmp( arg, "say" ) && Cmd_Argc() >= 4 ) {
		const char *lang = Cmd_Argv( 2 ), *said;
		int i;

		c = LG_Active();
		i = LG_FindLang( c, lang );
		if ( !c || i < 0 || !c->langs[i].speak ) {
			Com_Printf( S_COLOR_YELLOW "%s doesn't speak %s\n", c ? c->name : "Your character", lang );
			return;
		}
		if ( cls.state != CA_ACTIVE ) {
			Com_Printf( "Not in game\n" );
			return;
		}
		said = LG_InLanguage( c->langs[i].name, Cmd_ArgsFrom( 3 ) );
		CL_AddReliableCommand( va( "say %s", said ? said : Cmd_ArgsFrom( 3 ) ), qfalse );
	} else if ( !Q_stricmp( arg, "add" ) && Cmd_Argc() >= 3 ) {
		const qboolean understand = (qboolean)( Cmd_Argc() >= 4 && !Q_stricmp( Cmd_Argv( Cmd_Argc() - 1 ), "understand" ) );
		char name[LG_NAME_LEN];

		Q_strncpyz( name, Cmd_Argv( 2 ), sizeof( name ) );
		c = LG_EnsureChar();
		if ( strchr( name, ':' ) || strchr( name, '<' ) || strchr( name, '>' ) || !Q_stricmp( name, LG_BASIC ) ) {
			Com_Printf( S_COLOR_YELLOW "Not a language name: %s\n", name );
			return;
		}
		if ( !LG_AddLang( c, name, (qboolean)!understand ) ) {
			Com_Printf( S_COLOR_YELLOW "%i languages at most\n", LG_MAX_LANGS );
			return;
		}
		LG_Save();
		Com_Printf( "%s %s %s\n", c->name, understand ? "understands" : "speaks", name );
	} else if ( !Q_stricmp( arg, "remove" ) && Cmd_Argc() >= 3 ) {
		int i;

		c = LG_Active();
		i = LG_FindLang( c, Cmd_Argv( 2 ) );
		if ( i < 0 ) {
			Com_Printf( S_COLOR_YELLOW "%s doesn't know %s\n", c ? c->name : "Your character", Cmd_Argv( 2 ) );
			return;
		}
		if ( !Q_stricmp( c->spoken, c->langs[i].name ) )
			c->spoken[0] = '\0';
		Com_Printf( "%s forgot %s\n", c->name, c->langs[i].name );
		memmove( &c->langs[i], &c->langs[i + 1], ( c->numLangs - i - 1 ) * sizeof( c->langs[0] ) );
		c->numLangs--;
		LG_Save();
	} else if ( !Q_stricmp( arg, "character" ) && Cmd_Argc() >= 3 ) {
		const char *name = Cmd_ArgsFrom( 2 );
		int i = LG_FindChar( name );

		if ( i < 0 ) {
			if ( !LG_AddChar( name ) ) {
				Com_Printf( S_COLOR_YELLOW "%i characters at most\n", LG_MAX_CHARS );
				return;
			}
			i = lgNumChars - 1;
			Com_Printf( "Made the character %s\n", name );
		}
		lgActive = i;
		LG_Save();
		LG_Status();
	} else if ( !Q_stricmp( arg, "characters" ) ) {
		for ( int i = 0; i < lgNumChars; i++ )
			Com_Printf( "%s%s\n", lgChars[i].name, i == lgActive ? S_COLOR_YELLOW "  (playing)" : "" );
		if ( !lgNumChars )
			Com_Printf( "No characters yet\n" );
	} else if ( !Q_stricmp( arg, "window" ) ) {
		LG_Open();
	} else {
		Com_Printf( "usage: language                          which character, what it knows and speaks\n" );
		Com_Printf( "       language select <language>        speak it; what you say goes out as ^3::Language:: << text >>\n" );
		Com_Printf( "       language basic                    speak Basic again\n" );
		Com_Printf( "       language say <language> <text>    say one line in it\n" );
		Com_Printf( "       language add <language> [understand]   it speaks it, or only understands it\n" );
		Com_Printf( "       language remove <language>\n" );
		Com_Printf( "       language character <name>         play as that character, made if new\n" );
		Com_Printf( "       language characters               list them\n" );
		Com_Printf( "       " LG_COMMAND "                         the window for all of it\n" );
	}
}

/*
===============================================================================

THE WINDOW

===============================================================================
*/

static void LG_Message( const char *text ) {
	Q_strncpyz( lg.message, text, sizeof( lg.message ) );
	lg.messageTime = cls.realtime;
}

static lgLang_t *LG_SelLang( void ) {
	lgChar_t *c = LG_Active();

	return c && lg.langSel >= 0 && lg.langSel < c->numLangs ? &c->langs[lg.langSel] : NULL;
}

static char *LG_FieldText( lgField_t f, int *size ) {
	switch ( f ) {
	case LG_F_CHARNAME:	*size = LG_NAME_LEN; return LG_Active() ? LG_Active()->name : NULL;
	case LG_F_NEWLANG:	*size = LG_NAME_LEN; return lg.newLang;
	default:			*size = 0; return NULL;
	}
}

// a character's name done with: none, or one taken, goes back
static void LG_CommitName( void ) {
	lgChar_t *c = LG_Active();

	if ( !c || !strcmp( c->name, lg.nameBefore ) )
		return;
	for ( int i = 0; i < lgNumChars; i++ ) {
		if ( &lgChars[i] != c && !Q_stricmp( lgChars[i].name, c->name ) ) {
			LG_Message( va( S_COLOR_YELLOW "There's a character called %s already", c->name ) );
			Q_strncpyz( c->name, lg.nameBefore, sizeof( c->name ) );
			return;
		}
	}
	if ( !c->name[0] ) {
		Q_strncpyz( c->name, lg.nameBefore, sizeof( c->name ) );
		return;
	}
	LG_Save();
}

static void LG_SetFocus( lgField_t f ) {
	if ( lg.focus == LG_F_CHARNAME && f != LG_F_CHARNAME )
		LG_CommitName();
	if ( f == LG_F_CHARNAME && lg.focus != LG_F_CHARNAME && LG_Active() )
		Q_strncpyz( lg.nameBefore, LG_Active()->name, sizeof( lg.nameBefore ) );
	lg.focus = f;
}

static void LG_PlayAs( int i ) {
	LG_SetFocus( LG_F_NONE );
	lgActive = i;
	lg.langSel = LG_Active() && LG_Active()->numLangs ? 0 : -1;
	lg.langScroll = 0;
	LG_Save();
}

static void LG_NewChar( void ) {
	char name[LG_NAME_LEN];
	int n = lgNumChars + 1;

	if ( lgNumChars >= LG_MAX_CHARS ) {
		LG_Message( va( S_COLOR_YELLOW "%i characters at most", LG_MAX_CHARS ) );
		return;
	}
	do {
		Com_sprintf( name, sizeof( name ), "Character %i", n++ );
	} while ( LG_FindChar( name ) >= 0 );
	LG_AddChar( name );
	LG_PlayAs( lgNumChars - 1 );
	LG_SetFocus( LG_F_CHARNAME );
}

static void LG_DeleteChar( void ) {
	const int i = lgActive;

	if ( !LG_Active() )
		return;
	LG_SetFocus( LG_F_NONE );
	LG_Message( va( "Deleted %s", lgChars[i].name ) );
	memmove( &lgChars[i], &lgChars[i + 1], ( lgNumChars - i - 1 ) * sizeof( lgChars[0] ) );
	lgNumChars--;
	LG_PlayAs( lgNumChars ? Q_min( i, lgNumChars - 1 ) : -1 );
}

static void LG_AddNewLang( void ) {
	lgChar_t *c;
	char name[LG_NAME_LEN];
	int len;

	Q_strncpyz( name, lg.newLang, sizeof( name ) );
	while ( ( len = strlen( name ) ) && name[len - 1] == ' ' )
		name[len - 1] = '\0';
	if ( !name[0] )
		return;
	if ( !Q_stricmp( name, LG_BASIC ) ) {
		LG_Message( "Everyone knows Basic" );
		return;
	}
	c = LG_EnsureChar();
	if ( LG_FindLang( c, name ) >= 0 ) {
		LG_Message( va( S_COLOR_YELLOW "%s knows %s already", c->name, name ) );
		return;
	}
	if ( !LG_AddLang( c, name, qtrue ) ) {
		LG_Message( va( S_COLOR_YELLOW "%i languages at most", LG_MAX_LANGS ) );
		return;
	}
	lg.langSel = c->numLangs - 1;
	lg.newLang[0] = '\0';
	LG_Save();
}

static void LG_RemoveLang( void ) {
	lgChar_t *c = LG_Active();
	const int i = lg.langSel;

	if ( !LG_SelLang() )
		return;
	if ( !Q_stricmp( c->spoken, c->langs[i].name ) )
		c->spoken[0] = '\0';
	LG_Message( va( "%s forgot %s", c->name, c->langs[i].name ) );
	memmove( &c->langs[i], &c->langs[i + 1], ( c->numLangs - i - 1 ) * sizeof( c->langs[0] ) );
	c->numLangs--;
	lg.langSel = Q_min( i, c->numLangs - 1 );
	LG_Save();
}

qboolean CL_Languages_Active( void ) {
	return lg.open;
}

void CL_Languages_Close( void ) {
	if ( !lg.open )
		return;
	LG_SetFocus( LG_F_NONE );
	lg.open = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_LANGUAGES );
}

static void LG_Open( void ) {
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Languages: join a server first; " S_COLOR_GREY "the language command works anywhere\n" );
		return;
	}
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_MapEditor_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_SoundManager_Close();
	CL_RadialMenu_CloseSettings();
	LG_Load();

	lg.font = re->RegisterFont( "arialnb" );
	if ( !lg.font )
		lg.font = cls.menuFont;
	lg.open = qtrue;
	lg.focus = LG_F_NONE;
	lg.cursorX = SCREEN_WIDTH * 0.5f;
	lg.cursorY = SCREEN_HEIGHT * 0.5f;
	lg.click = qfalse;
	lg.wheel = 0;
	if ( !LG_SelLang() )
		lg.langSel = LG_Active() && LG_Active()->numLangs ? 0 : -1;
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_LANGUAGES );
}

static void LG_Window_f( void ) {
	if ( lg.open )
		CL_Languages_Close();
	else
		LG_Open();
}

// a character the focused box takes
static qboolean LG_AllowChar( int ch ) {
	int size;

	if ( !LG_FieldText( lg.focus, &size ) || ch < ' ' || ch >= 127 || ch == '"' || ch == ';' )
		return qfalse;
	// a language's name sits between :: in chat, and isn't coloured
	if ( lg.focus == LG_F_NEWLANG && ( ch == ':' || ch == '<' || ch == '>' || ch == Q_COLOR_ESCAPE ) )
		return qfalse;
	return qtrue;
}

void CL_Languages_KeyEvent( int key, qboolean down ) {
	const char *binding;
	char *text;
	int size;

	if ( !lg.open || !down )
		return;

	// binds don't run while this holds the keys, so honour the one that toggles it here
	binding = Key_GetBinding( key );
	if ( !( lg.focus != LG_F_NONE && key >= A_SPACE && key <= A_TILDE ) && VALIDSTRING( binding ) && !Q_stricmp( binding, LG_COMMAND ) ) {
		CL_Languages_Close();
		return;
	}

	text = LG_FieldText( lg.focus, &size );
	if ( text && key != A_MOUSE1 && key != A_MWHEELUP && key != A_MWHEELDOWN && WG_EditKey( text, size, key, LG_AllowChar ) )
		return;

	switch ( key ) {
	case A_MOUSE1:
		lg.click = qtrue;
		lg.clickX = lg.cursorX;
		lg.clickY = lg.cursorY;
		return;
	case A_MWHEELUP:
		lg.wheel -= 3;
		return;
	case A_MWHEELDOWN:
		lg.wheel += 3;
		return;
	case A_ENTER:
	case A_KP_ENTER:
		if ( lg.focus == LG_F_NEWLANG )
			LG_AddNewLang();
		else
			LG_SetFocus( LG_F_NONE );
		return;
	case A_TAB:
		LG_SetFocus( lg.focus == LG_F_NEWLANG ? LG_F_NONE : LG_F_NEWLANG );
		return;
	case A_CURSOR_UP:
	case A_CURSOR_DOWN:
		if ( lg.focus == LG_F_NONE && LG_Active() && LG_Active()->numLangs )
			lg.langSel = Com_Clampi( 0, LG_Active()->numLangs - 1, ( lg.langSel < 0 ? 0 : lg.langSel ) + ( key == A_CURSOR_UP ? -1 : 1 ) );
		return;
	default:
		return;
	}
}

void CL_Languages_CharEvent( int ch ) {
	char *text;
	int size;

	text = LG_FieldText( lg.focus, &size );
	if ( !lg.open || !text )
		return;
	WG_EditChar( text, size, ch, LG_AllowChar );
}

void CL_Languages_MouseEvent( int dx, int dy ) {
	lg.cursorX = Com_Clamp( 0, SCREEN_WIDTH, lg.cursorX + dx );
	lg.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, lg.cursorY + dy );
}

void CL_Languages_Escape( void ) {
	if ( lg.focus != LG_F_NONE )
		LG_SetFocus( LG_F_NONE );
	else
		CL_Languages_Close();
}

static void LG_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void LG_Box( float x, float y, float w, float h, const float *fill, const float *border ) {
	LG_Fill( x, y, w, h, fill );
	LG_Fill( x, y, w, 1, border );
	LG_Fill( x, y + h - 1, w, 1, border );
	LG_Fill( x, y, 1, h, border );
	LG_Fill( x + w - 1, y, 1, h, border );
}

static qboolean LG_InRect( float x, float y, float w, float h ) {
	return (qboolean)( lg.cursorX >= x && lg.cursorX < x + w && lg.cursorY >= y && lg.cursorY < y + h );
}

static void LG_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, lg.font | STYLE_DROPSHADOW, -1, LG_TEXT_SCALE );
}

static float LG_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, lg.font, LG_TEXT_SCALE );
}

static void LG_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && LG_TextWidth( buf ) > w ) {
		len--;
		buf[len - 3] = '.';
		buf[len - 2] = '.';
		buf[len - 1] = '.';
		buf[len] = '\0';
	}
	LG_Text( x, y, buf, color );
}

// eats this frame's click if it landed in the rect
static qboolean LG_Clicked( float x, float y, float w, float h ) {
	if ( !lg.click || lg.clickX < x || lg.clickX >= x + w || lg.clickY < y || lg.clickY >= y + h )
		return qfalse;
	lg.click = qfalse;
	return qtrue;
}

static qboolean LG_Button( float x, float y, float w, const char *label, qboolean selected, qboolean enabled ) {
	const float *fill = selected ? lgHighlight : ( enabled && LG_InRect( x, y, w, LG_CTRL_H ) ) ? lgHoverBox : lgPanelLight;

	LG_Box( x, y, w, LG_CTRL_H, fill, lgBorder );
	LG_Text( x + ( w - LG_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? lgWhite : lgDim );
	return (qboolean)( LG_Clicked( x, y, w, LG_CTRL_H ) && enabled );
}

// a button that has to be clicked twice
static qboolean LG_DangerButton( float x, float y, float w, const char *label, int what, qboolean enabled ) {
	const qboolean armed = (qboolean)( lg.confirmWhat == what && lg.confirmTime && cls.realtime - lg.confirmTime < LG_CONFIRM_MS );
	const char *text = armed ? "Click again" : label;

	LG_Box( x, y, w, LG_CTRL_H, armed ? lgDanger : ( enabled && LG_InRect( x, y, w, LG_CTRL_H ) ) ? lgHoverBox : lgPanelLight, lgBorder );
	LG_Text( x + ( w - LG_TextWidth( text ) ) * 0.5f, y + 2, text, enabled ? lgWhite : lgDim );
	if ( !LG_Clicked( x, y, w, LG_CTRL_H ) || !enabled )
		return qfalse;
	if ( armed ) {
		lg.confirmTime = 0;
		return qtrue;
	}
	lg.confirmTime = cls.realtime;
	lg.confirmWhat = what;
	return qfalse;
}

static void LG_FieldStyle( wgStyle_t *style ) {
	style->font = lg.font;
	style->scale = LG_TEXT_SCALE;
	style->lineH = LG_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = lgPanelFocus;
	style->border = lgBorder;
	style->text = lgWhite;
	style->dim = lgDim;
}

static void LG_Field( lgField_t f, float x, float y, float w, const char *placeholder ) {
	const qboolean focused = (qboolean)( lg.focus == f );
	int size;
	char *text = LG_FieldText( f, &size );
	wgStyle_t style;

	LG_Box( x, y, w, LG_CTRL_H, focused ? lgPanelFocus : lgPanelLight, lgBorder );
	if ( !text ) {
		LG_TextClipped( x + 4, y + 2, w - 8, placeholder, lgDim );
		return;
	}
	LG_FieldStyle( &style );
	WG_Field( &style, x, y, w, LG_CTRL_H, text, focused, placeholder, lg.cursorX, lg.cursorY );
	if ( LG_Clicked( x, y, w, WG_FieldHeight( &style, w, LG_CTRL_H, text, focused ) ) ) {
		LG_SetFocus( f );
		WG_FieldClick( text, lg.clickX, lg.clickY );
	}
}

static void LG_Label( float x, float y, const char *label ) {
	LG_Text( x, y + 2, label, lgDim );
}

// the list's box, its scroll kept to a new selection, and a scrollbar when there are more than
// fit; how many rows show, and *rowW how wide they are
static int LG_ListRow( int count, int *scroll, int sel, int *shownSel, float x, float y, float w, float h, float *rowW ) {
	const int visible = (int)( h / LG_ROW_H );

	if ( lg.wheel && LG_InRect( x, y, w, h ) ) {
		*scroll += lg.wheel;
		lg.wheel = 0;
	}
	if ( sel != *shownSel ) {
		if ( sel >= 0 && sel < *scroll )
			*scroll = sel;
		if ( sel >= *scroll + visible )
			*scroll = sel - visible + 1;
		*shownSel = sel;
	}
	if ( *scroll > count - visible )
		*scroll = count - visible;
	if ( *scroll < 0 )
		*scroll = 0;
	LG_Box( x, y, w, h, lgPanelLight, lgBorder );
	*rowW = WG_Scrollbar( x + w - WG_SCROLL_W - 1, y + 1, h - 2, count, visible, scroll, lg.cursorX, lg.cursorY, &lg.click, lg.clickX, lg.clickY )
		? w - WG_SCROLL_W - 1 : w;
	return visible;
}

static void LG_Checkbox( float x, float y, qboolean on, qboolean enabled ) {
	LG_Box( x, y + 1, 10, 10, lgPanelFocus, enabled ? lgBorder : lgPanelLight );
	if ( on )
		LG_Fill( x + 2, y + 3, 6, 6, enabled ? lgCheck : lgDim );
}

static void LG_DrawChars( void ) {
	const float x = LG_CHAR_X, w = LG_CHAR_W;
	float rw;
	const int visible = LG_ListRow( lgNumChars, &lg.charScroll, lgActive, &lg.charShownSel, x, LG_LIST_Y, w, LG_LIST_H, &rw );
	float y = LG_EDIT_Y;

	LG_Label( x, 48, "Characters" );
	for ( int i = 0; i < visible; i++ ) {
		const int row = lg.charScroll + i;
		const float ry = LG_LIST_Y + i * LG_ROW_H;

		if ( row >= lgNumChars )
			break;
		if ( row == lgActive )
			LG_Fill( x + 1, ry, rw - 2, LG_ROW_H, lgHighlight );
		else if ( LG_InRect( x, ry, rw, LG_ROW_H ) )
			LG_Fill( x + 1, ry, rw - 2, LG_ROW_H, lgHover );
		LG_TextClipped( x + 4, ry + 1, rw - 8, lgChars[row].name, lgWhite );
		if ( LG_Clicked( x, ry, rw, LG_ROW_H ) && row != lgActive )
			LG_PlayAs( row );
	}
	if ( !lgNumChars )
		LG_TextClipped( x + 4, LG_LIST_Y + 2, w - 8, "None yet: press New", lgDim );
	LG_Clicked( x, LG_LIST_Y, w, LG_LIST_H );

	if ( LG_Button( x, y, 72, "New", qfalse, (qboolean)( lgNumChars < LG_MAX_CHARS ) ) )
		LG_NewChar();
	if ( LG_DangerButton( x + 78, y, 72, "Delete", 1, (qboolean)( LG_Active() != NULL ) ) )
		LG_DeleteChar();
	y += 20;
	LG_Label( x, y, "Name" );
	LG_Field( LG_F_CHARNAME, x + 34, y, w - 34, "make one first" );
	y += 20;
	LG_Text( x, y + 2, S_COLOR_GREY "Click one to play as it", lgWhite );
}

static void LG_DrawLangs( void ) {
	lgChar_t *c = LG_Active();
	const float x = LG_LANG_X, w = LG_LANG_W;
	const char *spoken = LG_Spoken();
	const lgLang_t *sel;
	int numLangs = c ? c->numLangs : 0;
	float rw;
	const int visible = LG_ListRow( numLangs, &lg.langScroll, lg.langSel, &lg.langShownSel, x, LG_LIST_Y, w, LG_LIST_H, &rw );
	const float speakX = x + rw - LG_CHECK_W * 2, knowX = x + rw - LG_CHECK_W;
	float y = LG_EDIT_Y;

	LG_Label( x, 48, c ? va( "Languages of %s", c->name ) : "Languages" );
	LG_Text( speakX + ( LG_CHECK_W - LG_TextWidth( "Speaks" ) ) * 0.5f, 50, "Speaks", lgDim );
	LG_Text( knowX + ( LG_CHECK_W - LG_TextWidth( "Understands" ) ) * 0.5f, 50, "Understands", lgDim );
	for ( int i = 0; c && i < visible; i++ ) {
		const int row = lg.langScroll + i;
		const float ry = LG_LIST_Y + i * LG_ROW_H;
		lgLang_t *l;

		if ( row >= c->numLangs )
			break;
		l = &c->langs[row];
		if ( row == lg.langSel )
			LG_Fill( x + 1, ry, rw - 2, LG_ROW_H, lgHighlight );
		else if ( LG_InRect( x, ry, rw, LG_ROW_H ) )
			LG_Fill( x + 1, ry, rw - 2, LG_ROW_H, lgHover );
		LG_TextClipped( x + 4, ry + 1, speakX - x - 8, !Q_stricmp( spoken, l->name ) ? va( "%s" S_COLOR_YELLOW "  speaking", l->name ) : l->name, lgWhite );
		LG_Checkbox( speakX + ( LG_CHECK_W - 10 ) * 0.5f, ry, l->speak, qtrue );
		// understood whenever it's spoken
		LG_Checkbox( knowX + ( LG_CHECK_W - 10 ) * 0.5f, ry, qtrue, (qboolean)!l->speak );
		if ( LG_Clicked( speakX, ry, LG_CHECK_W, LG_ROW_H ) ) {
			l->speak = (qboolean)!l->speak;
			lg.langSel = row;
			LG_Save();
		} else if ( LG_Clicked( knowX, ry, LG_CHECK_W, LG_ROW_H ) ) {
			// off is forgetting it; it has to stop being spoken first
			lg.langSel = row;
			if ( l->speak )
				LG_Message( va( "Untick Speaks first: %s can't speak %s without understanding it", c->name, l->name ) );
			else
				LG_Message( "Remove forgets it altogether" );
		} else if ( LG_Clicked( x, ry, rw, LG_ROW_H ) ) {
			lg.langSel = row;
		}
	}
	if ( c && !c->numLangs )
		LG_TextClipped( x + 4, LG_LIST_Y + 2, w - 8, va( "%s knows only Basic: add a language below", c->name ), lgDim );
	LG_Clicked( x, LG_LIST_Y, w, LG_LIST_H );

	LG_Field( LG_F_NEWLANG, x, y, 170, "a language, e.g. Huttese" );
	if ( LG_Button( x + 174, y, 56, "Add", qfalse, (qboolean)( lg.newLang[0] != '\0' ) ) )
		LG_AddNewLang();
	if ( LG_DangerButton( x + 234, y, w - 234, "Remove", 2, (qboolean)( LG_SelLang() != NULL ) ) )
		LG_RemoveLang();
	y += 20;
	sel = LG_SelLang();
	if ( LG_Button( x, y, 170, sel ? va( "Speak %s", sel->name ) : "Speak it", (qboolean)( sel && !Q_stricmp( spoken, sel->name ) ),
		(qboolean)( sel && sel->speak ) ) )
		LG_Select( sel->name );
	if ( LG_Button( x + 174, y, w - 174, "Speak Basic", (qboolean)( c && !spoken[0] ), (qboolean)( c != NULL ) ) )
		LG_Select( LG_BASIC );
	y += 20;
	LG_TextClipped( x, y + 2, w, c ? va( S_COLOR_GREY "Your chat is in " S_COLOR_YELLOW "%s", spoken[0] ? spoken : LG_BASIC ) : "", lgWhite );

	// how it sounds to the ones who don't know it
	y = 352;
	if ( sel ) {
		char said[MAX_STRING_CHARS];

		LG_Gibberish( sel->name, LG_SAMPLE, (int)strlen( LG_SAMPLE ), said, sizeof( said ) );
		LG_Label( LG_CHAR_X, y, va( "How %s sounds to those who don't know it:", sel->name ) );
		LG_TextClipped( LG_CHAR_X, y + 16, SCREEN_WIDTH - LG_CHAR_X * 2, va( LG_LABEL_COLOR "::%s:: << %s >>", sel->name, said ), lgWhite );
		LG_TextClipped( LG_CHAR_X, y + 30, SCREEN_WIDTH - LG_CHAR_X * 2, va( S_COLOR_GREY "%s", LG_SAMPLE ), lgWhite );
	}
}

void CL_Languages_Draw( void ) {
	if ( !lg.open )
		return;

	LG_Fill( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, lgPanel );
	LG_Text( LG_CHAR_X, 14, "Languages", lgWhite );
	if ( lg.message[0] && cls.realtime - lg.messageTime < LG_MESSAGE_MS )
		LG_TextClipped( LG_CHAR_X, 29, SCREEN_WIDTH - LG_CHAR_X * 2, lg.message, lgWhite );

	LG_DrawChars();
	LG_DrawLangs();

	LG_Text( LG_CHAR_X, 416, S_COLOR_GREY "Chat is Basic, which everyone knows. Speaking a language, what you say goes out as " LG_LABEL_COLOR "::Language:: << text >>", lgWhite );
	LG_Text( LG_CHAR_X, 428, S_COLOR_GREY "What's said in a language your character doesn't know shows as gibberish. Commands: language select <name>, language basic", lgWhite );
	LG_Text( LG_CHAR_X, 452, va( S_COLOR_GREY "Esc back / close   %s", Key_GetKey( LG_COMMAND ) >= 0 ? "Your " LG_COMMAND " bind closes"
		: "Tip: bind a key to " LG_COMMAND ), lgWhite );

	// a click on nothing leaves the text box
	if ( lg.click )
		LG_SetFocus( LG_F_NONE );
	lg.click = qfalse;
	lg.wheel = 0;

	WG_DrawOverlay();
	re->DrawStretchPic( lg.cursorX, lg.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

void CL_Languages_Frame( void ) {
	if ( lg.open && cls.state != CA_ACTIVE )
		CL_Languages_Close();
}

/*
===============================================================================

STARTING AND STOPPING

===============================================================================
*/

void CL_Languages_Init( void ) {
	memset( &lg, 0, sizeof( lg ) );
	lg.langSel = -1;
	lgLoaded = qfalse;
	Cmd_AddCommand( "language", LG_Language_f, "Your character's languages: select one to speak, add, remove, switch character" );
	Cmd_AddCommand( LG_COMMAND, LG_Window_f, "Edit your characters' languages" );
}

// cgame is going away: map change, disconnect or vid_restart
void CL_Languages_Shutdown( void ) {
	CL_Languages_Close();
}
