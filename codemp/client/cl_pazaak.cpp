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

// cl_pazaak.cpp -- Pazaak on the datapad: decks, lobbies on this server, matches and watching them
//
// C1B3R deals and checks every match (its /v2/pazaak, see OpenJK-rpmod-c1b3r's pazaak.py): this
// only shows it and sends the moves, on the C1B3R session (cl_presence.cpp). A match is shown as C1B3R
// says this player may see it: the other hand only as a count.
//
// The cards are drawn here, in the datapad's style. Someone who has art of their own can put it in a
// pk3: pazaak/cards/<main|plus|minus|either|special|back>.png (or .tga, .jpg) is drawn under the
// value instead, and pazaak/sounds/<deal|play|turn|stand|win|lose|click>.wav played instead of Jedi
// Academy's own sounds. Nothing of the kind is shipped.

#include <string>
#include "client.h"
#include "cl_datapad.h"
#include "cl_presence.h"
#include "cl_widgets.h"

#define PZ_DECK_SIZE		10
#define PZ_HAND				4
#define PZ_TABLE			9
#define PZ_MAX_DECKS		10
#define PZ_MAX_LOBBIES		32
#define PZ_NAME_LEN			64
#define PZ_LIST_MS			3000		// the lobbies asked again this often while they're shown
#define PZ_IDLE_WAIT_MS		2000		// not in a lobby: asked again after this
#define PZ_WAIT_MS			40000		// a wait's answer may take this long: C1B3R holds it 25 s
#define PZ_MESSAGE_MS		4000

typedef enum {
	PZ_MENU,
	PZ_LOBBIES,
	PZ_DECKS,
	PZ_RULES,
	PZ_GAME			// waiting for someone, playing or watching: as C1B3R says
} pzPage_t;

// the side cards, as C1B3R names them, in the order the deck builder lists them
static const char *pzCards[] = {
	"+1", "+2", "+3", "+4", "+5", "+6",
	"-1", "-2", "-3", "-4", "-5", "-6",
	"*1", "*2", "*3", "*4", "*5", "*6",
	"F24", "F36", "D", "T", "V"
};
#define PZ_NUM_CARDS	( (int)ARRAY_LEN( pzCards ) )
static const char *pzStarter[PZ_DECK_SIZE] = { "+1", "+2", "+3", "+4", "+5", "-1", "-2", "-3", "-4", "-5" };

typedef struct {
	char	code[8];
	int		value;
} pzCard_t;

typedef struct {
	char		name[64];
	int			sets, total, handCount;
	qboolean	stood;
	pzCard_t	table[PZ_TABLE + 4];
	int			tableCount;
} pzSeat_t;

typedef struct {
	char	id[16], status[16], rules[16], host[64], guest[64], name[64];
	int		spectators;
} pzLobby_t;

typedef struct {
	char	name[PZ_NAME_LEN];
	char	cards[PZ_DECK_SIZE][8];
} pzDeck_t;

typedef enum {
	PZ_SND_CLICK, PZ_SND_DEAL, PZ_SND_PLAY, PZ_SND_TURN, PZ_SND_STAND, PZ_SND_WIN, PZ_SND_LOSE, PZ_NUM_SOUNDS
} pzSound_t;

static const char *pzSoundNames[PZ_NUM_SOUNDS] = { "click", "deal", "play", "turn", "stand", "win", "lose" };
// Jedi Academy's own, when no pazaak/sounds/ of the player's
static const char *pzSoundDefaults[PZ_NUM_SOUNDS] = {
	"sound/interface/button1.wav", "sound/interface/sub_select.wav", "sound/interface/choose_color.wav",
	"sound/interface/update.wav", "sound/interface/zoomend.wav", "sound/interface/secret_area.wav",
	"sound/interface/ammocon_empty.wav"
};

static const char *pzArtNames[] = { "main", "plus", "minus", "either", "special", "back" };
#define PZ_NUM_ART	( (int)ARRAY_LEN( pzArtNames ) )

static struct {
	qboolean	shown;
	qboolean	registered;
	pzPage_t	page;
	sfxHandle_t	sounds[PZ_NUM_SOUNDS];
	qhandle_t	art[PZ_NUM_ART];

	// the match, or the lobby, we're in: as C1B3R last said
	int			version;
	qboolean	inLobby;
	char		lobbyId[16], status[16], rules[16], lobbyName[64];
	int			you;				// 0 or 1, -1 watching
	int			setNumber, turn, winner, seconds, secondsAt;
	qboolean	played;
	pzSeat_t	seats[2];
	char		hand[PZ_HAND][8];	// "" once played
	char		events[6][128];
	int			numEvents;
	int			selected;			// a hand card picked to play, -1 none

	// the server's lobbies
	pzLobby_t	lobbies[PZ_MAX_LOBBIES];
	int			numLobbies;
	int			listTime;
	char		newName[PZ_NAME_LEN];
	qboolean	newPm;				// a lobby for +, - and +/- cards only

	// decks, as C1B3R keeps them
	pzDeck_t	decks[PZ_MAX_DECKS];
	int			numDecks;
	qboolean	decksKnown;
	pzDeck_t	edit;				// the deck being built
	int			editCount;
	int			focus;				// 0 none, 1 the deck's name, 2 the new lobby's name

	int			waitNext;
	char		message[128];
	int			messageTime;
	qboolean	messageBad;
} pz;

static httpRequest_t pzWaitHttp, pzActHttp, pzListHttp, pzDecksHttp;
static char pzActWhat[32];			// what the action going out was, for its answer
static cvar_t *cl_pazaakDeck;

/*
===============================================================================

Sounds, art, messages

===============================================================================
*/

static qboolean PZ_FileExists( const char *path ) {
	return (qboolean)( FS_ReadFile( path, NULL ) > 0 );
}

static void PZ_Register( void ) {
	if ( pz.registered )
		return;
	pz.registered = qtrue;
	for ( int i = 0; i < PZ_NUM_SOUNDS; i++ ) {
		const char *own = va( "pazaak/sounds/%s.wav", pzSoundNames[i] );

		pz.sounds[i] = S_RegisterSound( PZ_FileExists( own ) ? own : pzSoundDefaults[i] );
	}
	for ( int i = 0; i < PZ_NUM_ART; i++ ) {
		static const char *exts[] = { "png", "tga", "jpg" };

		pz.art[i] = 0;
		for ( int e = 0; e < 3 && !pz.art[i]; e++ ) {
			if ( PZ_FileExists( va( "pazaak/cards/%s.%s", pzArtNames[i], exts[e] ) ) )
				pz.art[i] = re->RegisterShaderNoMip( va( "pazaak/cards/%s", pzArtNames[i] ) );
		}
	}
}

static void PZ_Sound( pzSound_t s ) {
	DP_PlaySound( pz.sounds[s] );
}

static void PZ_Message( const char *text, qboolean bad ) {
	Q_strncpyz( pz.message, text, sizeof( pz.message ) );
	pz.messageTime = cls.realtime;
	pz.messageBad = bad;
}

/*
===============================================================================

Talking to C1B3R

===============================================================================
*/

static qboolean PZ_Post( httpRequest_t *req, const char *path, const std::string &fields ) {
	const char *session = CL_Presence_Session();
	std::string body = "{\"session\":";

	if ( !session ) {
		PZ_Message( "Not connected to C1B3R: Pazaak needs it", qtrue );
		return qfalse;
	}
	if ( req->state == HTTP_BUSY )
		return qfalse;
	CL_Presence_Json( body, session );
	body += fields + "}";
	return CL_Presence_Post( req, path, body );
}

static std::string PZ_Field( const char *name, const char *value ) {
	std::string out = va( ",\"%s\":", name );

	CL_Presence_Json( out, value );
	return out;
}

// an action: one at a time; its answer said in a message
static void PZ_Act( const char *what, const char *path, const std::string &fields ) {
	if ( pzActHttp.state == HTTP_BUSY )
		return;
	Q_strncpyz( pzActWhat, what, sizeof( pzActWhat ) );
	if ( PZ_Post( &pzActHttp, path, fields ) )
		pz.waitNext = 0;
}

static void PZ_AskLobbies( void ) {
	if ( pzListHttp.state != HTTP_BUSY && PZ_Post( &pzListHttp, "/v2/pazaak/lobbies", "" ) )
		pz.listTime = cls.realtime;
}

static void PZ_AskDecks( void ) {
	if ( pzDecksHttp.state != HTTP_BUSY )
		PZ_Post( &pzDecksHttp, "/v2/pazaak/decks", "" );
}

// a line's tab-separated fields: how many
static int PZ_Split( char *line, char **field, int max ) {
	int n = 0;

	for ( char *f = line; f && n < max; n++ ) {
		field[n] = f;
		f = strchr( f, '\t' );
		if ( f )
			*f++ = '\0';
	}
	return n;
}

// each line of an answer, in turn
static const char *PZ_Line( const char *at, char *line, int size ) {
	const char *end = strchr( at, '\n' );
	const int len = end ? (int)( end - at ) : (int)strlen( at );

	Q_strncpyz( line, at, Q_min( len + 1, size ) );
	return end ? end + 1 : at + len;
}

static void PZ_ReadTable( pzSeat_t *seat, char *list ) {
	seat->tableCount = 0;
	for ( char *c = list; c && *c && seat->tableCount < (int)ARRAY_LEN( seat->table ); ) {
		char *next = strchr( c, ',' ), *colon;

		if ( next )
			*next++ = '\0';
		colon = strchr( c, ':' );
		if ( colon ) {
			*colon = '\0';
			Q_strncpyz( seat->table[seat->tableCount].code, c, sizeof( seat->table[0].code ) );
			seat->table[seat->tableCount].value = atoi( colon + 1 );
			seat->tableCount++;
		}
		c = next;
	}
}

// what C1B3R says of our lobby: see pazaak.py's wait
static void PZ_ReadState( const char *answer ) {
	char line[1024], *f[8];
	const char *at = answer;
	const int oldTurn = pz.turn, oldSet = pz.setNumber, oldWinner = pz.winner;
	const qboolean wasIn = pz.inLobby;
	int n;

	at = PZ_Line( at, line, sizeof( line ) );
	n = PZ_Split( line, f, 8 );
	if ( n < 2 || strcmp( f[0], "ok" ) )
		return;
	pz.version = atoi( f[1] );
	pz.inLobby = qfalse;
	pz.numEvents = 0;
	pz.setNumber = 0;
	pz.winner = -1;
	memset( pz.seats, 0, sizeof( pz.seats ) );
	while ( *at ) {
		at = PZ_Line( at, line, sizeof( line ) );
		n = PZ_Split( line, f, 8 );
		if ( n >= 6 && !strcmp( f[0], "lobby" ) ) {
			pz.inLobby = qtrue;
			Q_strncpyz( pz.lobbyId, f[1], sizeof( pz.lobbyId ) );
			Q_strncpyz( pz.status, f[2], sizeof( pz.status ) );
			Q_strncpyz( pz.rules, f[3], sizeof( pz.rules ) );
			pz.you = !strcmp( f[4], "s" ) ? -1 : atoi( f[4] );
			Q_strncpyz( pz.lobbyName, CL_Presence_Unfield( f[5] ).c_str(), sizeof( pz.lobbyName ) );
		} else if ( n >= 6 && !strcmp( f[0], "set" ) ) {
			pz.setNumber = atoi( f[1] );
			pz.turn = atoi( f[2] );
			pz.seconds = atoi( f[3] );
			pz.secondsAt = cls.realtime;
			pz.winner = atoi( f[4] );
			pz.played = (qboolean)( atoi( f[5] ) != 0 );
		} else if ( n >= 7 && !strcmp( f[0], "seat" ) ) {
			pzSeat_t *s = &pz.seats[atoi( f[1] ) & 1];

			s->sets = atoi( f[2] );
			s->total = atoi( f[3] );
			s->stood = (qboolean)( atoi( f[4] ) != 0 );
			s->handCount = atoi( f[5] );
			Q_strncpyz( s->name, CL_Presence_Unfield( f[6] ).c_str(), sizeof( s->name ) );
		} else if ( n >= 3 && !strcmp( f[0], "table" ) ) {
			PZ_ReadTable( &pz.seats[atoi( f[1] ) & 1], f[2] );
		} else if ( n >= 2 && !strcmp( f[0], "hand" ) ) {
			char *c = f[1];

			for ( int i = 0; i < PZ_HAND; i++ ) {
				char *next = c ? strchr( c, ',' ) : NULL;

				if ( next )
					*next++ = '\0';
				Q_strncpyz( pz.hand[i], c && strcmp( c, "x" ) ? c : "", sizeof( pz.hand[i] ) );
				c = next;
			}
		} else if ( n >= 2 && !strcmp( f[0], "event" ) && pz.numEvents < (int)ARRAY_LEN( pz.events ) ) {
			Q_strncpyz( pz.events[pz.numEvents++], CL_Presence_Unfield( f[1] ).c_str(), sizeof( pz.events[0] ) );
		}
	}

	// to the match as it starts; back to the lobbies as it's gone
	if ( pz.inLobby && !wasIn )
		pz.page = PZ_GAME;
	else if ( !pz.inLobby && pz.page == PZ_GAME )
		pz.page = PZ_LOBBIES;
	if ( pz.selected >= 0 && ( pz.you < 0 || !pz.hand[pz.selected][0] ) )
		pz.selected = -1;

	// what happened, heard
	if ( pz.inLobby && pz.setNumber ) {
		if ( pz.winner >= 0 && oldWinner < 0 && pz.you >= 0 )
			PZ_Sound( pz.winner == pz.you ? PZ_SND_WIN : PZ_SND_LOSE );
		else if ( pz.setNumber != oldSet || pz.turn != oldTurn )
			PZ_Sound( pz.you >= 0 && pz.turn == pz.you ? PZ_SND_TURN : PZ_SND_DEAL );
	}
}

static void PZ_ReadLobbies( const char *answer ) {
	char line[512], *f[8];
	const char *at = answer;

	pz.numLobbies = 0;
	at = PZ_Line( at, line, sizeof( line ) );
	if ( strncmp( line, "ok", 2 ) )
		return;
	while ( *at && pz.numLobbies < PZ_MAX_LOBBIES ) {
		pzLobby_t *l = &pz.lobbies[pz.numLobbies];

		at = PZ_Line( at, line, sizeof( line ) );
		if ( PZ_Split( line, f, 8 ) < 8 || strcmp( f[0], "lobby" ) )
			continue;
		Q_strncpyz( l->id, f[1], sizeof( l->id ) );
		Q_strncpyz( l->status, f[2], sizeof( l->status ) );
		Q_strncpyz( l->rules, f[3], sizeof( l->rules ) );
		l->spectators = atoi( f[4] );
		Q_strncpyz( l->host, CL_Presence_Unfield( f[5] ).c_str(), sizeof( l->host ) );
		Q_strncpyz( l->guest, CL_Presence_Unfield( f[6] ).c_str(), sizeof( l->guest ) );
		Q_strncpyz( l->name, CL_Presence_Unfield( f[7] ).c_str(), sizeof( l->name ) );
		pz.numLobbies++;
	}
}

static void PZ_ReadDecks( const char *answer ) {
	char line[512], *f[4];
	const char *at = answer;

	pz.numDecks = 0;
	at = PZ_Line( at, line, sizeof( line ) );
	if ( strncmp( line, "ok", 2 ) )
		return;
	pz.decksKnown = qtrue;
	while ( *at && pz.numDecks < PZ_MAX_DECKS ) {
		pzDeck_t *d = &pz.decks[pz.numDecks];
		char *c;
		int i = 0;

		at = PZ_Line( at, line, sizeof( line ) );
		if ( PZ_Split( line, f, 4 ) < 3 || strcmp( f[0], "deck" ) )
			continue;
		memset( d, 0, sizeof( *d ) );
		Q_strncpyz( d->name, CL_Presence_Unfield( f[1] ).c_str(), sizeof( d->name ) );
		for ( c = f[2]; c && *c && i < PZ_DECK_SIZE; i++ ) {
			char *next = strchr( c, ',' );

			if ( next )
				*next++ = '\0';
			Q_strncpyz( d->cards[i], c, sizeof( d->cards[i] ) );
			c = next;
		}
		pz.numDecks++;
	}
}

void CL_Pazaak_Frame( void ) {
	if ( HTTP_Poll( &pzWaitHttp ) ) {
		if ( pzWaitHttp.status == 200 ) {
			PZ_ReadState( pzWaitHttp.answer );
			pz.waitNext = pz.inLobby ? 0 : cls.realtime + PZ_IDLE_WAIT_MS;
		} else {
			pz.waitNext = cls.realtime + PZ_IDLE_WAIT_MS;
		}
	}
	if ( HTTP_Poll( &pzActHttp ) ) {
		if ( pzActHttp.status != 200 ) {
			PZ_Message( pzActHttp.answer[0] ? va( "%s", pzActHttp.answer ) : "C1B3R didn't answer", qtrue );
		} else if ( !strcmp( pzActWhat, "save" ) ) {
			PZ_Message( va( "Saved %s", pz.edit.name ), qfalse );
			PZ_AskDecks();
		} else if ( !strcmp( pzActWhat, "delete" ) ) {
			PZ_Message( "Deleted", qfalse );
			PZ_AskDecks();
		} else if ( !strcmp( pzActWhat, "leave" ) ) {
			PZ_AskLobbies();
		}
		// a newline at the end of C1B3R's message
		for ( char *nl = strchr( pz.message, '\n' ); nl; nl = NULL )
			*nl = '\0';
		pz.waitNext = 0;
	}
	if ( HTTP_Poll( &pzListHttp ) && pzListHttp.status == 200 )
		PZ_ReadLobbies( pzListHttp.answer );
	if ( HTTP_Poll( &pzDecksHttp ) && pzDecksHttp.status == 200 )
		PZ_ReadDecks( pzDecksHttp.answer );

	if ( !pz.shown || !CL_Presence_Session() )
		return;
	// our lobby or match, kept up with: C1B3R answers at once when it changes
	if ( pzWaitHttp.state != HTTP_BUSY && cls.realtime >= pz.waitNext ) {
		pzWaitHttp.waitMs = PZ_WAIT_MS;
		if ( !PZ_Post( &pzWaitHttp, "/v2/pazaak/wait", va( ",\"after\":%i", pz.inLobby ? pz.version : 0 ) ) )
			pz.waitNext = cls.realtime + PZ_IDLE_WAIT_MS;
	}
	if ( pz.page == PZ_LOBBIES && cls.realtime - pz.listTime >= PZ_LIST_MS )
		PZ_AskLobbies();
	if ( !pz.decksKnown && pzDecksHttp.state != HTTP_BUSY && ( pz.page == PZ_DECKS || pz.page == PZ_LOBBIES || pz.page == PZ_MENU ) )
		PZ_AskDecks();
}

void CL_Pazaak_Shown( qboolean shown ) {
	if ( shown && !pz.shown ) {
		PZ_Register();
		pz.waitNext = 0;
		pz.decksKnown = qfalse;
		if ( pz.page != PZ_GAME )
			pz.page = PZ_MENU;
	}
	pz.shown = shown;
	pz.focus = 0;
}

/*
===============================================================================

Cards

===============================================================================
*/

// what's printed on a card: its value on the table, or what it does in the hand
static const char *PZ_Label( const char *code, int value, qboolean onTable ) {
	if ( !strcmp( code, "M" ) )
		return va( "%i", value );
	if ( onTable && strcmp( code, "F24" ) && strcmp( code, "F36" ) )
		return va( "%+i", value );
	if ( code[0] == '*' )
		return va( "\xB1%s", code + 1 );
	if ( !strcmp( code, "F24" ) )
		return "2&4";
	if ( !strcmp( code, "F36" ) )
		return "3&6";
	if ( !strcmp( code, "D" ) )
		return "D";
	if ( !strcmp( code, "T" ) )
		return "\xB1" "1T";
	if ( !strcmp( code, "V" ) )
		return "1\xB1" "2";
	return code;
}

// card colours: the main deck's green, + blue, - red, +/- both, the special ones gold
static void PZ_CardColors( const char *code, int value, qboolean onTable, float *top, float *bottom, int *art ) {
	static const float green[4] = { 0.16f, 0.52f, 0.26f, 1.0f }, blue[4] = { 0.18f, 0.36f, 0.82f, 1.0f };
	static const float red[4] = { 0.74f, 0.20f, 0.18f, 1.0f }, gold[4] = { 0.78f, 0.60f, 0.16f, 1.0f };
	const float *t, *b;

	if ( !strcmp( code, "M" ) ) {
		t = b = green;
		*art = 0;
	} else if ( code[0] == '+' || ( onTable && code[0] == '*' && value >= 0 ) ) {
		t = b = blue;
		*art = 1;
	} else if ( code[0] == '-' || ( onTable && code[0] == '*' ) ) {
		t = b = red;
		*art = 2;
	} else if ( code[0] == '*' ) {
		t = blue;
		b = red;
		*art = 3;
	} else {
		t = b = gold;
		*art = 4;
	}
	memcpy( top, t, sizeof( float ) * 4 );
	memcpy( bottom, b, sizeof( float ) * 4 );
}

static void PZ_DrawBack( float x, float y, float w, float h ) {
	const float dark[4] = { 0.06f, 0.08f, 0.10f, 1.0f };

	if ( pz.art[5] ) {
		re->SetColor( NULL );
		re->DrawStretchPic( x, y, w, h, 0, 0, 1, 1, pz.art[5] );
		return;
	}
	DP_Fill( x, y, w, h, dark );
	for ( float ly = y + 3; ly < y + h - 2; ly += 4 )
		DP_Fill( x + 2, ly, w - 4, 1, dpDim );
	DP_Frame( x, y, w, h, dpGlow );
	DP_Frame( x + 2, y + 2, w - 4, h - 4, dpGlow );
}

// a card face up: its colour panel, a dark band with the value, a thin frame
static void PZ_DrawCard( float x, float y, float w, float h, const char *code, int value, qboolean onTable, qboolean lit ) {
	float top[4], bottom[4];
	int art;
	const float band[4] = { 0.04f, 0.05f, 0.06f, 0.85f };
	const char *label = PZ_Label( code, value, onTable );

	PZ_CardColors( code, value, onTable, top, bottom, &art );
	if ( pz.art[art] ) {
		re->SetColor( NULL );
		re->DrawStretchPic( x, y, w, h, 0, 0, 1, 1, pz.art[art] );
	} else {
		DP_Fill( x, y, w, h * 0.5f, top );
		DP_Fill( x, y + h * 0.5f, w, h * 0.5f, bottom );
		DP_Fill( x + 2, y + 2, w - 4, 2, dpWhite );
	}
	DP_Fill( x + 1, y + h * 0.32f, w - 2, h * 0.36f, band );
	DP_Text( x, y + h * 0.5f - 6, w, label, dpWhite, DP_Font(), w > DP_Wide( 26 ) ? 0.75f : 0.62f, qtrue );
	DP_Frame( x, y, w, h, lit ? dpWhite : dpGlow );
	if ( lit )
		DP_Frame( x - 1, y - 1, w + 2, h + 2, dpWhite );
}

static void PZ_DrawSlot( float x, float y, float w, float h ) {
	const float slot[4] = { 0.0f, 0.0f, 0.0f, 0.35f };

	DP_Fill( x, y, w, h, slot );
	DP_Frame( x, y, w, h, dpScan );
}

/*
===============================================================================

Pages

===============================================================================
*/

static const char *PZ_DeckName( void ) {
	return cl_pazaakDeck->string[0] ? cl_pazaakDeck->string : "Starter";
}

static void PZ_Title( const dpRect_t *s, float *y, const char *title, qboolean back ) {
	const float x = s->x + DP_Wide( 8 );

	if ( back && DP_Button( x, *y, DP_Wide( 50 ), "< Back", qfalse ) ) {
		PZ_Sound( PZ_SND_CLICK );
		pz.page = PZ_MENU;
		pz.focus = 0;
	}
	DP_Text( x + ( back ? DP_Wide( 58 ) : 0 ), *y, s->w, title, dpText, DP_TitleFont(), 0.62f, qfalse );
	*y += DP_CTRL_H + 8;
}

static void PZ_DrawMenu( const dpRect_t *s ) {
	const float x = s->x + DP_Wide( 30 ), w = s->w - DP_Wide( 60 );
	float y = s->y + DP_STATUS_H + 24;

	DP_Text( s->x, y, s->w, "PAZAAK", dpText, DP_TitleFont(), 1.1f, qtrue );
	y += 30;
	DP_Text( s->x, y, s->w, "Get closer to 20 than the other player. First to three sets.", dpDim, DP_Font(), 0.6f, qtrue );
	y += 30;
	if ( DP_Button( x, y, w, "Play: lobbies on this server", qfalse ) ) {
		pz.page = PZ_LOBBIES;
		pz.listTime = 0;
	}
	y += DP_CTRL_H + 8;
	if ( DP_Button( x, y, w, "Side decks", qfalse ) ) {
		pz.page = PZ_DECKS;
		pz.decksKnown = qfalse;
	}
	y += DP_CTRL_H + 8;
	if ( DP_Button( x, y, w, "Rules", qfalse ) )
		pz.page = PZ_RULES;
	y += DP_CTRL_H + 18;
	DP_Text( s->x, y, s->w, va( "Playing with: %s", PZ_DeckName() ), dpDim, DP_Font(), 0.6f, qtrue );
}

static const char *pzRules[] = {
	"^7The aim: a total closer to 20 than the other player's, never above it.",
	"Each turn the main deck deals you a card from 1 to 10.",
	"Then you may play one card from your hand, and End the turn or Stand.",
	"Standing keeps your total; the other player goes on alone.",
	"Above 20 at the end of your turn: you lose the set (a minus card can save you).",
	"Nine cards on your table without going above 20 wins the set at once.",
	"A full table takes no more cards: a ninth card above 20 loses it.",
	"Both stood: the higher total wins. A tie is played again, unless a",
	"Tiebreaker card was played by one of you: they win it.",
	"Exactly 20 stands by itself. A turn lasts 90 s, then you stand.",
	"The first to win three sets wins the match.",
	"",
	"^7Your hand: 4 cards drawn from your 10-card side deck as the match starts.",
	"They last the whole match: a card played is gone.",
	"+1..+6 and -1..-6 add or take away. +/- cards: you choose as you play them.",
	"2&4 and 3&6: those values on your table change sign.",
	"D: your last card counts twice. 1+/-2: one or two, up or down.",
	"Leaving a match gives it to the other player.",
};

static void PZ_DrawRules( const dpRect_t *s ) {
	float y = s->y + DP_STATUS_H + 4;

	PZ_Title( s, &y, "Rules", qtrue );
	for ( int i = 0; i < (int)ARRAY_LEN( pzRules ); i++ ) {
		DP_Text( s->x + DP_Wide( 8 ), y, s->w - DP_Wide( 16 ), pzRules[i], pzRules[i][0] == '^' ? dpText : dpDim, DP_Font(), 0.58f, qfalse );
		y += 13;
	}
}

// a text field on the datapad: the shared one, in its colours
static void PZ_Field( float x, float y, float w, char *buf, int which, const char *placeholder ) {
	wgStyle_t style;

	memset( &style, 0, sizeof( style ) );
	style.font = DP_Font();
	style.scale = 0.62f;
	style.lineH = 12;
	style.padX = 3;
	style.padY = 2;
	style.fill = dpScreen;
	style.border = dpGlow;
	style.text = dpText;
	style.dim = dpDim;
	DP_Fill( x, y, w, DP_CTRL_H, dpButton );
	DP_Frame( x, y, w, DP_CTRL_H, pz.focus == which ? dpWhite : dpGlow );
	WG_Field( &style, x, y, w, DP_CTRL_H, buf, (qboolean)( pz.focus == which ), placeholder, DP_CursorX(), DP_CursorY() );
	if ( DP_Clicked( x, y, w, DP_CTRL_H ) ) {
		if ( pz.focus != which )
			WG_EditReset();
		pz.focus = which;
		WG_FieldClick( buf, DP_CursorX(), DP_CursorY() );
	}
}

// the deck to play with: the starter, then the saved ones, a click for the next
static void PZ_DeckChooser( float x, float y, float w ) {
	if ( DP_Button( x, y, w, va( "Deck: %s  >", PZ_DeckName() ), qfalse ) ) {
		int i;

		for ( i = 0; i < pz.numDecks && Q_stricmp( pz.decks[i].name, cl_pazaakDeck->string ); i++ )
			;
		i = cl_pazaakDeck->string[0] ? i + 1 : 0;
		Cvar_Set( "cl_pazaakDeck", i < pz.numDecks ? pz.decks[i].name : "" );
	}
}

static void PZ_DrawLobbies( const dpRect_t *s ) {
	const float x = s->x + DP_Wide( 8 ), w = s->w - DP_Wide( 16 ), bw = DP_Wide( 46 );
	float y = s->y + DP_STATUS_H + 4;
	int shown = 0;

	PZ_Title( s, &y, "Lobbies on this server", qtrue );
	for ( int i = 0; i < pz.numLobbies && shown < 12; i++, shown++ ) {
		const pzLobby_t *l = &pz.lobbies[i];
		const qboolean open = (qboolean)!strcmp( l->status, "waiting" );

		DP_Fill( x, y, w, 26, dpTile );
		DP_Text( x + DP_Wide( 4 ), y + 2, w - bw - DP_Wide( 10 ), va( "%s%s", l->name, !strcmp( l->rules, "pm" ) ? "  (\xB1 only)" : "" ), dpText, DP_Font(), 0.62f, qfalse );
		DP_Text( x + DP_Wide( 4 ), y + 14, w - bw - DP_Wide( 10 ), open ? va( "%s waits for a player", l->host )
			: va( "%s vs %s%s%s", l->host, l->guest, !strcmp( l->status, "over" ) ? "  (over)" : "", l->spectators ? va( "  %i watching", l->spectators ) : "" ),
			dpDim, DP_Font(), 0.55f, qfalse );
		if ( DP_Button( x + w - bw - DP_Wide( 3 ), y + 5, bw, open ? "Join" : "Watch", qfalse ) ) {
			PZ_Sound( PZ_SND_CLICK );
			if ( open )
				PZ_Act( "join", "/v2/pazaak/join", PZ_Field( "lobby", l->id ) + PZ_Field( "deck", cl_pazaakDeck->string ) );
			else
				PZ_Act( "spectate", "/v2/pazaak/spectate", PZ_Field( "lobby", l->id ) );
		}
		y += 30;
	}
	if ( !pz.numLobbies )
		DP_Text( x, y + 4, w, "No one's playing here yet: start a table below.", dpDim, DP_Font(), 0.6f, qfalse );

	// a new table
	y = s->y + s->h - ( DP_CTRL_H + 6 ) * 3 - 22;
	DP_Fill( x, y - 6, w, 1, dpGlow );
	DP_Text( x, y, w, "Start a table", dpText, DP_Font(), 0.62f, qfalse );
	y += 14;
	PZ_Field( x, y, w, pz.newName, 2, "its name (optional)" );
	y += DP_CTRL_H + 6;
	if ( DP_Button( x, y, w * 0.48f, pz.newPm ? "Rules: \xB1 cards only" : "Rules: all cards", pz.newPm ) )
		pz.newPm = (qboolean)!pz.newPm;
	PZ_DeckChooser( x + w * 0.52f, y, w * 0.48f );
	y += DP_CTRL_H + 6;
	if ( DP_Button( x, y, w, "Start it and wait for a player", qfalse ) ) {
		PZ_Sound( PZ_SND_CLICK );
		PZ_Act( "create", "/v2/pazaak/create", PZ_Field( "name", pz.newName ) + PZ_Field( "rules", pz.newPm ? "pm" : "standard" )
			+ PZ_Field( "deck", cl_pazaakDeck->string ) );
		pz.focus = 0;
	}
}

// a seat's side: its 3x3 table, and its name, total, sets and state beside it
static void PZ_DrawSeat( const dpRect_t *s, float y, int which ) {
	const pzSeat_t *seat = &pz.seats[which];
	const float cw = DP_Wide( 26 ), ch = 34, gap = DP_Wide( 3 ), x = s->x + DP_Wide( 8 );
	const float ix = x + cw * 3 + gap * 3 + DP_Wide( 8 ), iw = s->x + s->w - ix - DP_Wide( 6 );
	const qboolean turn = (qboolean)( pz.winner < 0 && pz.turn == which && !seat->stood );

	for ( int i = 0; i < PZ_TABLE; i++ ) {
		const float cx = x + ( i % 3 ) * ( cw + gap ), cy = y + ( i / 3 ) * ( ch + 3 );

		if ( i < seat->tableCount )
			PZ_DrawCard( cx, cy, cw, ch, seat->table[i].code, seat->table[i].value, qtrue, qfalse );
		else
			PZ_DrawSlot( cx, cy, cw, ch );
	}
	DP_Text( ix, y, iw, va( "%s%s", seat->name, pz.you == which ? " (you)" : "" ), turn ? dpWhite : dpText, DP_Font(), 0.62f, qfalse );
	DP_Text( ix, y + 16, iw, va( "%i", seat->total ), seat->total > 20 ? dpWhite : dpText, DP_TitleFont(), 1.0f, qfalse );
	for ( int i = 0; i < 3; i++ ) {
		const float lx = ix + i * DP_Wide( 12 );

		DP_Fill( lx, y + 46, DP_Wide( 9 ), 9, i < seat->sets ? dpGlow : dpButton );
		DP_Frame( lx, y + 46, DP_Wide( 9 ), 9, dpGlow );
	}
	DP_Text( ix, y + 60, iw, seat->stood ? "Stands" : !turn ? "" : pz.you == which ? "Your turn" : "Their turn", turn ? dpText : dpDim, DP_Font(), 0.58f, qfalse );
	if ( pz.you != which && seat->name[0] ) {
		// the other hand, face down
		for ( int i = 0; i < seat->handCount; i++ )
			PZ_DrawBack( ix + i * DP_Wide( 11 ), y + 76, DP_Wide( 9 ), 14 );
	}
}

static void PZ_Play( int index, int sign, int value ) {
	PZ_Sound( PZ_SND_PLAY );
	PZ_Act( "play", "/v2/pazaak/move", va( ",\"action\":\"play\",\"card\":%i,\"sign\":%i,\"value\":%i", index, sign, value ) );
	pz.selected = -1;
}

static void PZ_DrawGame( const dpRect_t *s ) {
	const float x = s->x + DP_Wide( 8 ), w = s->w - DP_Wide( 16 );
	const int other = pz.you == 1 ? 0 : 1, me = pz.you == 1 ? 1 : 0;
	const qboolean myTurn = (qboolean)( pz.you >= 0 && pz.winner < 0 && pz.turn == pz.you && !strcmp( pz.status, "playing" ) );
	float y = s->y + DP_STATUS_H + 4;

	// waiting for someone to sit down
	if ( !strcmp( pz.status, "waiting" ) ) {
		PZ_Title( s, &y, pz.lobbyName, qfalse );
		DP_Text( s->x, y + 40, s->w, "Waiting for a player to join...", dpText, DP_Font(), 0.7f, qtrue );
		DP_Text( s->x, y + 58, s->w, va( "Rules: %s   Deck: %s", !strcmp( pz.rules, "pm" ) ? "\xB1 cards only" : "all cards", PZ_DeckName() ),
			dpDim, DP_Font(), 0.6f, qtrue );
		if ( DP_Button( x + w * 0.25f, y + 90, w * 0.5f, "Cancel", qfalse ) )
			PZ_Act( "leave", "/v2/pazaak/leave", "" );
		return;
	}

	// the set and the clock
	{
		const int left = Q_max( 0, pz.seconds - ( cls.realtime - pz.secondsAt ) / 1000 );
		const char *who = pz.winner >= 0 ? va( "%s wins the match", pz.seats[pz.winner].name )
			: myTurn ? "Your turn" : va( "%s's turn", pz.seats[pz.turn & 1].name );

		DP_Text( x, y, w, va( "%s   Set %i   %s%s", pz.lobbyName, pz.setNumber, who, pz.winner < 0 ? va( "   %is", left ) : "" ),
			pz.winner >= 0 || myTurn ? dpWhite : dpText, DP_Font(), 0.62f, qfalse );
	}
	y += 16;
	PZ_DrawSeat( s, y, other );
	y += 3 * 37 + 4;

	// what happened lately
	DP_Fill( x, y, w, 1, dpGlow );
	for ( int i = Q_max( 0, pz.numEvents - 2 ), line = 0; i < pz.numEvents; i++, line++ )
		DP_Text( x, y + 3 + line * 11, w, pz.events[i], dpDim, DP_Font(), 0.55f, qfalse );
	y += 28;
	PZ_DrawSeat( s, y, me );
	y += 3 * 37 + 6;

	if ( pz.you < 0 ) {
		DP_Text( x, y + 6, w, "Watching", dpDim, DP_Font(), 0.62f, qfalse );
		if ( DP_Button( x + w * 0.6f, y + 4, w * 0.4f, "Stop watching", qfalse ) )
			PZ_Act( "leave", "/v2/pazaak/leave", "" );
		return;
	}

	// our hand: a click picks a card, then how to play it
	{
		const float cw = DP_Wide( 30 ), ch = 40, gap = DP_Wide( 4 );

		for ( int i = 0; i < PZ_HAND; i++ ) {
			const float cx = x + i * ( cw + gap );

			if ( !pz.hand[i][0] ) {
				PZ_DrawSlot( cx, y, cw, ch );
				continue;
			}
			PZ_DrawCard( cx, y, cw, ch, pz.hand[i], 0, qfalse, (qboolean)( pz.selected == i || DP_InRect( cx, y, cw, ch ) ) );
			if ( DP_Clicked( cx, y, cw, ch ) && myTurn && !pz.played ) {
				PZ_Sound( PZ_SND_CLICK );
				pz.selected = pz.selected == i ? -1 : i;
			}
		}
		// how to play the one picked
		if ( pz.selected >= 0 && myTurn && !pz.played ) {
			const char *code = pz.hand[pz.selected];
			const float px = x + 4 * ( cw + gap ) + DP_Wide( 4 ), pw = x + w - px;

			if ( code[0] == '*' || !strcmp( code, "T" ) ) {
				if ( DP_Button( px, y, pw, "Play it +", qfalse ) )
					PZ_Play( pz.selected, 1, 0 );
				if ( DP_Button( px, y + DP_CTRL_H + 4, pw, "Play it -", qfalse ) )
					PZ_Play( pz.selected, -1, 0 );
			} else if ( !strcmp( code, "V" ) ) {
				const float hw = ( pw - DP_Wide( 3 ) ) * 0.5f;

				if ( DP_Button( px, y, hw, "+1", qfalse ) )
					PZ_Play( pz.selected, 1, 1 );
				if ( DP_Button( px + hw + DP_Wide( 3 ), y, hw, "+2", qfalse ) )
					PZ_Play( pz.selected, 1, 2 );
				if ( DP_Button( px, y + DP_CTRL_H + 4, hw, "-1", qfalse ) )
					PZ_Play( pz.selected, -1, 1 );
				if ( DP_Button( px + hw + DP_Wide( 3 ), y + DP_CTRL_H + 4, hw, "-2", qfalse ) )
					PZ_Play( pz.selected, -1, 2 );
			} else if ( DP_Button( px, y, pw, "Play it", qfalse ) ) {
				PZ_Play( pz.selected, 1, 0 );
			}
		}
		y += ch + 8;
	}

	// the turn's end
	if ( pz.winner >= 0 || !strcmp( pz.status, "over" ) ) {
		DP_Text( x, y + 2, w * 0.6f, pz.winner == pz.you ? "You win the match!" : "You lose the match", dpWhite, DP_Font(), 0.7f, qfalse );
		if ( DP_Button( x + w * 0.6f, y, w * 0.4f, "Leave the table", qfalse ) )
			PZ_Act( "leave", "/v2/pazaak/leave", "" );
		return;
	}
	{
		const float bw = ( w - DP_Wide( 8 ) ) / 3.0f;

		if ( DP_Button( x, y, bw, "End turn", (qboolean)myTurn ) && myTurn ) {
			PZ_Sound( PZ_SND_CLICK );
			PZ_Act( "move", "/v2/pazaak/move", ",\"action\":\"end\"" );
		}
		if ( DP_Button( x + bw + DP_Wide( 4 ), y, bw, "Stand", qfalse ) && myTurn ) {
			PZ_Sound( PZ_SND_STAND );
			PZ_Act( "move", "/v2/pazaak/move", ",\"action\":\"stand\"" );
		}
		if ( DP_Button( x + ( bw + DP_Wide( 4 ) ) * 2, y, bw, "Forfeit", qfalse ) )
			PZ_Act( "leave", "/v2/pazaak/leave", "" );
	}
}

// the deck being built: its name, its ten cards (a click takes one out), and every card to add
static void PZ_DrawDecks( const dpRect_t *s ) {
	const float x = s->x + DP_Wide( 8 ), w = s->w - DP_Wide( 16 );
	const float cw = DP_Wide( 28 ), ch = 36, gap = DP_Wide( 4 );
	float y = s->y + DP_STATUS_H + 4;

	PZ_Title( s, &y, "Side decks", qtrue );

	// the saved ones, a click to edit it
	DP_Text( x, y, w, pz.decksKnown ? ( pz.numDecks ? "Yours (kept on C1B3R): click one to edit it" : "None saved yet" ) : "Asking C1B3R...",
		dpDim, DP_Font(), 0.58f, qfalse );
	y += 13;
	for ( int i = 0; i < pz.numDecks; i++ ) {
		const float bw = ( w - DP_Wide( 4 ) * 4 ) / 5.0f, bx = x + ( i % 5 ) * ( bw + DP_Wide( 4 ) );

		if ( DP_Button( bx, y + ( i / 5 ) * ( DP_CTRL_H + 3 ), bw, pz.decks[i].name, (qboolean)!Q_stricmp( pz.decks[i].name, pz.edit.name ) ) ) {
			pz.edit = pz.decks[i];
			pz.editCount = 0;
			while ( pz.editCount < PZ_DECK_SIZE && pz.edit.cards[pz.editCount][0] )
				pz.editCount++;
		}
	}
	y += ( ( pz.numDecks + 4 ) / 5 ) * ( DP_CTRL_H + 3 ) + 6;

	PZ_Field( x, y, w * 0.6f, pz.edit.name, 1, "the deck's name" );
	if ( DP_Button( x + w * 0.62f, y, w * 0.38f, "Clear it", qfalse ) ) {
		memset( &pz.edit, 0, sizeof( pz.edit ) );
		pz.editCount = 0;
	}
	y += DP_CTRL_H + 8;

	// its ten
	DP_Text( x, y, w, va( "%i of %i cards: a click takes one out", pz.editCount, PZ_DECK_SIZE ), dpDim, DP_Font(), 0.58f, qfalse );
	y += 13;
	for ( int i = 0; i < PZ_DECK_SIZE; i++ ) {
		const float cx = x + ( i % 5 ) * ( cw + gap ), cy = y + ( i / 5 ) * ( ch + 4 );

		if ( i >= pz.editCount ) {
			PZ_DrawSlot( cx, cy, cw, ch );
			continue;
		}
		PZ_DrawCard( cx, cy, cw, ch, pz.edit.cards[i], 0, qfalse, DP_InRect( cx, cy, cw, ch ) );
		if ( DP_Clicked( cx, cy, cw, ch ) ) {
			memmove( &pz.edit.cards[i], &pz.edit.cards[i + 1], sizeof( pz.edit.cards[0] ) * ( PZ_DECK_SIZE - 1 - i ) );
			pz.edit.cards[PZ_DECK_SIZE - 1][0] = '\0';
			pz.editCount--;
			PZ_Sound( PZ_SND_CLICK );
			break;
		}
	}
	y += 2 * ( ch + 4 ) + 6;

	// every card: a click puts one in
	DP_Text( x, y, w, "Every card: a click adds one", dpDim, DP_Font(), 0.58f, qfalse );
	y += 13;
	{
		const float sw = DP_Wide( 24 ), sh = 30, sg = DP_Wide( 3 );

		for ( int i = 0; i < PZ_NUM_CARDS; i++ ) {
			const float cx = x + ( i % 6 ) * ( sw + sg ), cy = y + ( i / 6 ) * ( sh + 3 );

			PZ_DrawCard( cx, cy, sw, sh, pzCards[i], 0, qfalse, DP_InRect( cx, cy, sw, sh ) );
			if ( DP_Clicked( cx, cy, sw, sh ) && pz.editCount < PZ_DECK_SIZE ) {
				Q_strncpyz( pz.edit.cards[pz.editCount++], pzCards[i], sizeof( pz.edit.cards[0] ) );
				PZ_Sound( PZ_SND_CLICK );
			}
		}
		y += 4 * ( sh + 3 ) + 6;
	}

	// keep it
	{
		const float bw = ( w - DP_Wide( 8 ) ) / 3.0f;
		std::string cards;

		for ( int i = 0; i < pz.editCount; i++ )
			cards += std::string( i ? "," : "" ) + pz.edit.cards[i];
		if ( DP_Button( x, y, bw, "Save", qfalse ) ) {
			if ( pz.editCount != PZ_DECK_SIZE || !pz.edit.name[0] )
				PZ_Message( "A deck is 10 cards, with a name", qtrue );
			else
				PZ_Act( "save", "/v2/pazaak/deck/save", PZ_Field( "name", pz.edit.name ) + PZ_Field( "cards", cards.c_str() ) );
		}
		if ( DP_Button( x + bw + DP_Wide( 4 ), y, bw, "Delete", qfalse ) && pz.edit.name[0] )
			PZ_Act( "delete", "/v2/pazaak/deck/delete", PZ_Field( "name", pz.edit.name ) );
		if ( DP_Button( x + ( bw + DP_Wide( 4 ) ) * 2, y, bw, "Play with it", (qboolean)!Q_stricmp( cl_pazaakDeck->string, pz.edit.name ) ) && pz.edit.name[0] ) {
			Cvar_Set( "cl_pazaakDeck", pz.edit.name );
			PZ_Message( va( "Playing with %s (save it first)", pz.edit.name ), qfalse );
		}
	}
}

void CL_Pazaak_Draw( const dpRect_t *s ) {
	switch ( pz.page ) {
	case PZ_LOBBIES:	PZ_DrawLobbies( s );	break;
	case PZ_DECKS:		PZ_DrawDecks( s );		break;
	case PZ_RULES:		PZ_DrawRules( s );		break;
	case PZ_GAME:		PZ_DrawGame( s );		break;
	default:			PZ_DrawMenu( s );		break;
	}
	if ( pz.message[0] && cls.realtime - pz.messageTime < PZ_MESSAGE_MS )
		DP_Text( s->x + DP_Wide( 8 ), s->y + s->h - 14, s->w - DP_Wide( 16 ), pz.message, pz.messageBad ? dpWhite : dpText, DP_Font(), 0.6f, qfalse );
	WG_DrawOverlay();
}

/*
===============================================================================

Typing, and the command

===============================================================================
*/

static char *PZ_FocusedBuf( void ) {
	return pz.focus == 1 ? pz.edit.name : pz.focus == 2 ? pz.newName : NULL;
}

qboolean CL_Pazaak_Key( int key ) {
	char *buf = PZ_FocusedBuf();

	if ( !buf )
		return qfalse;
	if ( key == A_ENTER || key == A_KP_ENTER || key == A_TAB ) {
		pz.focus = 0;
		return qtrue;
	}
	WG_EditKey( buf, PZ_NAME_LEN, key, NULL );
	return qtrue;
}

void CL_Pazaak_Char( int ch ) {
	char *buf = PZ_FocusedBuf();

	if ( buf )
		WG_EditChar( buf, PZ_NAME_LEN, ch, NULL );
}

// the same as the buttons, from the console or a bind
static void PZ_Pazaak_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "create" ) ) {
		PZ_Act( "create", "/v2/pazaak/create", PZ_Field( "name", Cmd_Argv( 2 ) ) + PZ_Field( "rules", "standard" ) + PZ_Field( "deck", cl_pazaakDeck->string ) );
	} else if ( !Q_stricmp( arg, "join" ) || !Q_stricmp( arg, "watch" ) ) {
		const qboolean join = (qboolean)!Q_stricmp( arg, "join" );

		PZ_Act( join ? "join" : "spectate", join ? "/v2/pazaak/join" : "/v2/pazaak/spectate", PZ_Field( "lobby", Cmd_Argv( 2 ) ) + PZ_Field( "deck", cl_pazaakDeck->string ) );
	} else if ( !Q_stricmp( arg, "end" ) || !Q_stricmp( arg, "stand" ) ) {
		PZ_Act( "move", "/v2/pazaak/move", va( ",\"action\":\"%s\"", arg ) );
	} else if ( !Q_stricmp( arg, "play" ) ) {
		PZ_Play( atoi( Cmd_Argv( 2 ) ), Cmd_Argc() > 3 ? atoi( Cmd_Argv( 3 ) ) : 1, Cmd_Argc() > 4 ? atoi( Cmd_Argv( 4 ) ) : 0 );
	} else if ( !Q_stricmp( arg, "leave" ) ) {
		PZ_Act( "leave", "/v2/pazaak/leave", "" );
	} else if ( !Q_stricmp( arg, "page" ) ) {
		const char *page = Cmd_Argv( 2 );

		pz.page = !Q_stricmp( page, "lobbies" ) ? PZ_LOBBIES : !Q_stricmp( page, "decks" ) ? PZ_DECKS : !Q_stricmp( page, "rules" ) ? PZ_RULES
			: pz.inLobby ? PZ_GAME : PZ_MENU;
		if ( pz.page == PZ_DECKS )
			pz.decksKnown = qfalse;
	} else {
		Com_Printf( "usage: pazaak create [name] | join <lobby> | watch <lobby> | end | stand | play <card 0-3> [1|-1] [1|2] | leave\n" );
		Com_Printf( "       pazaak page menu|lobbies|decks|rules     (the datapad opens it: datapad pazaak)\n" );
		Com_Printf( "%s\n", pz.inLobby ? va( "In lobby %s (%s)", pz.lobbyId, pz.status ) : "Not at a table" );
	}
}

void CL_Pazaak_Init( void ) {
	memset( &pz, 0, sizeof( pz ) );
	pz.selected = -1;
	pz.winner = -1;
	for ( int i = 0; i < PZ_DECK_SIZE; i++ )
		Q_strncpyz( pz.edit.cards[i], pzStarter[i], sizeof( pz.edit.cards[0] ) );
	pz.editCount = PZ_DECK_SIZE;
	cl_pazaakDeck = Cvar_Get( "cl_pazaakDeck", "", CVAR_ARCHIVE_ND, "The Pazaak side deck you play with, by its name; empty: the starter deck" );
	Cmd_AddCommand( "pazaak", PZ_Pazaak_f, "Pazaak's moves and pages, as its buttons on the datapad (datapad pazaak opens it)" );
}
