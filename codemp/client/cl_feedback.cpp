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

// cl_feedback.cpp -- /feedback: bug reports and ideas for the OpenJK-RPMod project, and the answers
//
// A window to write a report in, a bug or an idea, sent to the feedback bot (cl_feedbackUrl). The
// bot posts it in the project's Discord, where it's answered, marked planned or fixed, or closed.
// What's written there comes back here, under My reports, where the player can answer too. The
// bot is asked for news every few minutes while there are reports, and a line in the console
// says when one has some.
//
// A report goes with the player's RPMod account, read from rpinfo ("Account details for
// <character> (<account>):"), its reply kept out of the console: logged out, or not on an RPMod
// server, nothing is sent.
//
// Reading the answers needs no account: a random private key, made when the first report is sent
// and kept in feedback.key in the home folder, goes with each request, and only that key's
// reports come back. Losing the file loses them here; the project still has them.
//
// The bot: github.com/Ber-til/OpenJK-RPMod-feedback-bot, whose README says what's sent and how
// the answers are written.

#include "client.h"
#include "cl_widgets.h"
#include "cl_http.h"
#include "qcommon/game_version.h"
#include <time.h>
#include <map>
#include <string>
#include <vector>

#define FB_CMD				"feedback"
#define FB_DEFAULT_URL		"https://c1b3r.keztor.tech"
#define FB_KEY_FILE			"feedback.key"
#define FB_FIRST_POLL_MS	15000				// after the game starts, the first time it asks
#define FB_POLL_MS			( 5 * 60 * 1000 )	// then this often, while there are reports
#define FB_POLL_OPEN_MS		30000				// and this often while My reports is looked at

// as the bot takes them
#define FB_TITLE_LEN		90
#define FB_TEXT_LEN			3000
#define FB_COMMENT_LEN		1500

#define FB_X				60.0f
#define FB_Y				36.0f
#define FB_W				520.0f
#define FB_H				408.0f
#define FB_ROW_H			13.0f
#define FB_CTRL_H			15.0f
#define FB_GAP				5.0f
#define FB_SCALE			0.8f
#define FB_LIST_W			176.0f
#define FB_TEXT_ROWS		13

static vec4_t fbPanel		= { 0.06f, 0.07f, 0.09f, 0.96f };
static vec4_t fbLight		= { 0.16f, 0.17f, 0.21f, 1.0f };
static vec4_t fbHover		= { 0.24f, 0.26f, 0.32f, 1.0f };
static vec4_t fbLit			= { 0.18f, 0.42f, 0.62f, 1.0f };
static vec4_t fbBorder		= { 0.35f, 0.38f, 0.45f, 1.0f };
static vec4_t fbWhite		= { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t fbDim			= { 0.65f, 0.67f, 0.72f, 1.0f };
static vec4_t fbFaint		= { 0.45f, 0.47f, 0.52f, 1.0f };
static vec4_t fbRed			= { 1.0f, 0.45f, 0.40f, 1.0f };
static vec4_t fbGreen		= { 0.45f, 0.90f, 0.50f, 1.0f };
static vec4_t fbAmber		= { 1.0f, 0.80f, 0.35f, 1.0f };
static vec4_t fbBlue		= { 0.50f, 0.75f, 1.0f, 1.0f };
static vec4_t fbNews		= { 1.0f, 0.80f, 0.20f, 1.0f };

typedef enum {
	FB_NONE,
	FB_TITLE,
	FB_TEXT,
	FB_COMMENT
} fbFocus_t;

typedef enum {
	FB_REQ_NONE,
	FB_REQ_REPORT,
	FB_REQ_COMMENT,
	FB_REQ_MINE
} fbRequest_t;

typedef struct {
	std::string	who;			// staff, player or status
	int			time;
	std::string	author;
	std::string	text;			// for status: open, planned, fixed or closed
} fbMessage_t;

typedef struct {
	int			id;
	std::string	kind;			// bug or idea
	std::string	status;
	int			created, updated;
	std::string	title, text;
	std::vector<fbMessage_t> messages;
} fbReport_t;

static cvar_t *cl_feedbackUrl;

static httpRequest_t fbHttp;	// big: kept out of fb, which is cleared

static struct {
	qboolean	open;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	int			wheel;
	qhandle_t	font;
	int			page;			// 0 a new report, 1 my reports
	fbFocus_t	focus;

	// a new report
	int			kind;			// 0 a bug, 1 an idea
	char		title[FB_TITLE_LEN + 1];
	char		text[FB_TEXT_LEN + 1];

	// my reports
	std::vector<fbReport_t> reports;
	qboolean	listed;			// they've come at least once
	char		listError[128];
	int			listTime;		// when they last came
	int			selected;		// the report shown, by its id; 0 none
	int			listScroll;
	float		talkScroll;
	char		comment[FB_COMMENT_LEN + 1];

	// the key, and what's been seen of each report: the time of its last answer looked at
	char		key[65];
	std::map<int, int> seen;
	std::map<int, int> announced;	// said in the console this session, up to then

	fbRequest_t	request;		// what's being asked now
	qboolean	askAgain;		// the reports to be asked for once it's done
	int			nextPoll;
	int			sentId;			// the report just sent, shown once it's listed
	qboolean	untouched;		// opened on New report and nothing done yet: news shows My reports

	char		message[256];
	qboolean	messageError;
	int			messageTime;
} fb;

static void FB_Message( qboolean error, const char *fmt, ... ) {
	va_list ap;

	va_start( ap, fmt );
	Q_vsnprintf( fb.message, sizeof( fb.message ), fmt, ap );
	va_end( ap );
	fb.messageError = error;
	fb.messageTime = cls.realtime;
}

/*
===============================================================================

The key file: "key <hex>", then "seen <report> <time>" for each report

===============================================================================
*/

static void FB_LoadFile( void ) {
	fileHandle_t f;
	const int len = FS_SV_FOpenFileRead( FB_KEY_FILE, &f );
	char *buf, *p, *line;

	fb.key[0] = '\0';
	fb.seen.clear();
	if ( !f )
		return;
	if ( len <= 0 || len > 65536 ) {
		FS_FCloseFile( f );
		return;
	}
	buf = (char *)Z_Malloc( len + 1, TAG_TEMP_WORKSPACE, qfalse );
	FS_Read( buf, len, f );
	FS_FCloseFile( f );
	buf[len] = '\0';
	for ( p = buf; ( line = p ) != NULL && *line; ) {
		char word[16], value[80];
		int id, time;

		p = strchr( line, '\n' );
		if ( p )
			*p++ = '\0';
		if ( sscanf( line, "%15s %79s", word, value ) == 2 && !Q_stricmp( word, "key" ) ) {
			Q_strncpyz( fb.key, value, sizeof( fb.key ) );
			Q_strlwr( fb.key );
		} else if ( sscanf( line, "%15s %i %i", word, &id, &time ) == 3 && !Q_stricmp( word, "seen" ) ) {
			fb.seen[id] = time;
		}
		if ( !p )
			break;
	}
	Z_Free( buf );
	// not a key the bot takes: one is made again when a report is sent
	for ( const char *c = fb.key; *c; c++ ) {
		if ( !isxdigit( (unsigned char)*c ) ) {
			fb.key[0] = '\0';
			break;
		}
	}
	if ( strlen( fb.key ) < 32 )
		fb.key[0] = '\0';
}

static void FB_SaveFile( void ) {
	fileHandle_t f;
	std::string out;

	if ( !fb.key[0] )
		return;
	out = va( "key %s\n", fb.key );
	for ( std::map<int, int>::const_iterator it = fb.seen.begin(); it != fb.seen.end(); ++it )
		out += va( "seen %i %i\n", it->first, it->second );
	f = FS_SV_FOpenFileWrite( FB_KEY_FILE );
	if ( !f ) {
		Com_Printf( S_COLOR_YELLOW "Feedback: couldn't write %s\n", FB_KEY_FILE );
		return;
	}
	FS_Write( out.c_str(), (int)out.size(), f );
	FS_FCloseFile( f );
}

// the private key, made the first time it's needed
static void FB_MakeKey( void ) {
	byte bytes[16];

	if ( fb.key[0] )
		return;
	Com_RandomBytes( bytes, sizeof( bytes ) );
	for ( int i = 0; i < (int)sizeof( bytes ); i++ )
		Com_sprintf( fb.key + i * 2, 3, "%02x", bytes[i] );
	FB_SaveFile();
}

// the player's private key, made now if there's none: C1B3R's sessions are this player's by it too
const char *CL_Feedback_Key( void ) {
	FB_MakeKey();
	return fb.key;
}

/*
===============================================================================

Talking to the bot

===============================================================================
*/

// a string for JSON, quoted: the game's letters past ASCII taken as Latin-1
static void FB_Json( std::string &out, const char *s ) {
	out += '"';
	for ( ; *s; s++ ) {
		const unsigned char c = (unsigned char)*s;

		if ( c == '"' || c == '\\' ) {
			out += '\\';
			out += (char)c;
		} else if ( c == '\n' ) {
			out += "\\n";
		} else if ( c < 32 || c >= 127 ) {
			out += va( "\\u%04x", c );
		} else {
			out += (char)c;
		}
	}
	out += '"';
}

static void FB_JsonField( std::string &out, const char *name, const char *value ) {
	out += out.size() > 1 ? "," : "";
	FB_Json( out, name );
	out += ':';
	FB_Json( out, value );
}

static const char *FB_Url( const char *path ) {
	static char url[512];
	int len;

	Q_strncpyz( url, cl_feedbackUrl->string, sizeof( url ) );
	len = (int)strlen( url );
	while ( len > 0 && url[len - 1] == '/' )
		url[--len] = '\0';
	Q_strcat( url, sizeof( url ), path );
	return url;
}

static qboolean FB_Busy( void ) {
	return (qboolean)( fb.request != FB_REQ_NONE );
}

static qboolean FB_Ask( fbRequest_t what, const char *path, const std::string &body ) {
	char error[128];

	if ( !cl_feedbackUrl->string[0] ) {
		FB_Message( qtrue, "No feedback address: cl_feedbackUrl is empty" );
		return qfalse;
	}
	if ( FB_Busy() )
		return qfalse;
	if ( !HTTP_Start( &fbHttp, FB_Url( path ), body.c_str(), error, sizeof( error ) ) ) {
		FB_Message( qtrue, "Couldn't send: %s", error );
		return qfalse;
	}
	fb.request = what;
	return qtrue;
}

// the player's reports, asked again: now, or once what's being asked is done
static void FB_AskMine( void ) {
	std::string body = "{";

	if ( !fb.key[0] || !cl_feedbackUrl->string[0] )
		return;
	if ( FB_Busy() ) {
		fb.askAgain = qtrue;
		return;
	}
	FB_JsonField( body, "key", fb.key );
	body += "}";
	fb.askAgain = qfalse;
	FB_Ask( FB_REQ_MINE, "/v1/mine", body );
}

/*
===============================================================================

The RPMod account the report goes with, read from rpinfo

===============================================================================
*/

#define FB_ACC_COMMAND		"rpinfo"
#define FB_ACC_TIMEOUT		4000	// ms the reply has to start in
#define FB_ACC_GAP			600		// ms after its last line it's over
#define FB_ACC_RETRY		10000	// no answer: asked again after this, while it's wanted
#define FB_ACC_LINE			256

typedef enum {
	FB_ACC_UNKNOWN,
	FB_ACC_ASKING,
	FB_ACC_IN,
	FB_ACC_OUT,				// the reply had no account in it
	FB_ACC_NO_ANSWER
} fbAccState_t;

static struct {
	fbAccState_t	state;
	char			name[64];
	int				askTime, lastLine, answerTime;
} fbAcc;

// on a server with RPMod to ask: not one run on this computer
static qboolean FB_RpmodServer( void ) {
	return (qboolean)( cls.state == CA_ACTIVE && !com_sv_running->integer );
}

static void FB_AskAccount( void ) {
	if ( !FB_RpmodServer() || fbAcc.state == FB_ACC_ASKING )
		return;
	fbAcc.state = FB_ACC_ASKING;
	fbAcc.name[0] = '\0';
	fbAcc.askTime = cls.realtime;
	fbAcc.lastLine = 0;
	CL_AddReliableCommand( FB_ACC_COMMAND, qfalse );
}

// a line without its colours, RPMod's own too: a ^ and what follows it
static void FB_Plain( char *s ) {
	char *out = s;

	for ( ; *s; s++ ) {
		if ( *s == Q_COLOR_ESCAPE && s[1] ) {
			s++;
			continue;
		}
		*out++ = *s;
	}
	*out = '\0';
}

// "Account details for Ber-til Keztor (keztor):": the account, in the brackets
static void FB_AccountLine( const char *line ) {
	char plain[FB_ACC_LINE];
	const char *p, *open, *close;

	Q_strncpyz( plain, line, sizeof( plain ) );
	FB_Plain( plain );
	if ( !( p = Q_stristr( plain, "account details for" ) ) || !( close = strrchr( p, ')' ) ) )
		return;
	for ( open = close; open > p && *open != '('; open-- )
		;
	if ( *open != '(' || close - open < 2 )
		return;
	Q_strncpyz( fbAcc.name, open + 1, Q_min( (int)( close - open ), (int)sizeof( fbAcc.name ) ) );
}

// RPMod's "You are now logged in as ..." or a logout
static qboolean FB_AccountChanged( const char *text ) {
	char plain[FB_ACC_LINE];
	const char *p = plain;

	Q_strncpyz( plain, text, sizeof( plain ) );
	FB_Plain( plain );
	while ( *p == ' ' || *p == '\n' )
		p++;
	if ( Q_stricmpn( p, "you", 3 ) )
		return qfalse;
	return (qboolean)( Q_stristr( p, "logged in as" ) || Q_stristr( p, "logged out" ) );
}

// a server print: qtrue keeps it out of the console, being the reply asked for
qboolean CL_Feedback_ServerPrint( const char *text ) {
	char line[FB_ACC_LINE];

	if ( FB_AccountChanged( text ) ) {
		if ( fbAcc.state != FB_ACC_ASKING )
			fbAcc.state = FB_ACC_UNKNOWN;
		return qfalse;
	}
	if ( fbAcc.state != FB_ACC_ASKING || ( !fbAcc.lastLine && cls.realtime - fbAcc.askTime > FB_ACC_TIMEOUT ) )
		return qfalse;
	while ( *text ) {
		const char *end = strchr( text, '\n' );
		const int len = end ? (int)( end - text ) : (int)strlen( text );

		Q_strncpyz( line, text, Q_min( len + 1, (int)sizeof( line ) ) );
		FB_AccountLine( line );
		text += len;
		if ( *text == '\n' )
			text++;
	}
	fbAcc.lastLine = cls.realtime;
	return qtrue;
}

void CL_Feedback_Disconnect( void ) {
	memset( &fbAcc, 0, sizeof( fbAcc ) );
}

// for C1B3R's login (cl_presence.cpp): the account, its reply kept out of the console.
// 1 with the account in *name, 0 logged out, -2 being asked, -3 the server didn't answer (at
// *answerTime), -1 not asked yet
int CL_Feedback_Account( const char **name, int *answerTime ) {
	*name = fbAcc.name;
	*answerTime = fbAcc.answerTime;
	switch ( fbAcc.state ) {
	case FB_ACC_IN:			return 1;
	case FB_ACC_OUT:		return 0;
	case FB_ACC_ASKING:		return -2;
	case FB_ACC_NO_ANSWER:	return -3;
	default:				return -1;
	}
}

void CL_Feedback_AskAccount( void ) {
	FB_AskAccount();
}

// the reply over, and the account asked for while a report is being written
static void FB_AccountFrame( qboolean wanted ) {
	if ( fbAcc.state == FB_ACC_ASKING ) {
		if ( fbAcc.lastLine ? cls.realtime - fbAcc.lastLine < FB_ACC_GAP : cls.realtime - fbAcc.askTime < FB_ACC_TIMEOUT )
			return;
		fbAcc.state = fbAcc.name[0] ? FB_ACC_IN : fbAcc.lastLine ? FB_ACC_OUT : FB_ACC_NO_ANSWER;
		fbAcc.answerTime = cls.realtime;
		return;
	}
	if ( wanted && FB_RpmodServer() && ( fbAcc.state == FB_ACC_UNKNOWN
		|| ( fbAcc.state == FB_ACC_NO_ANSWER && cls.realtime - fbAcc.answerTime > FB_ACC_RETRY ) ) )
		FB_AskAccount();
}

// what goes with a report: this game, and where it is
static const char *FB_Details( void ) {
	static char text[512];
	extern cvar_t *cl_renderer;

	Com_sprintf( text, sizeof( text ), "%s, renderer %s", Cvar_VariableString( "version" ), cl_renderer->string );
	if ( cls.state == CA_ACTIVE ) {
		const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];

		Q_strcat( text, sizeof( text ), va( ", map %s", Info_ValueForKey( info, "mapname" ) ) );
		if ( !com_sv_running->integer )
			Q_strcat( text, sizeof( text ), va( ", server %s", cls.servername ) );
	}
	return text;
}

static void FB_SendReport( void ) {
	std::string body = "{";

	if ( FB_Busy() )
		return;
	if ( !fb.title[0] || !fb.text[0] ) {
		FB_Message( qtrue, "Write a title and what happened, or the idea, first" );
		return;
	}
	if ( fbAcc.state != FB_ACC_IN ) {
		FB_Message( qtrue, "Reports are sent from an RPMod server, logged in to your account" );
		return;
	}
	FB_MakeKey();
	if ( !fb.key[0] )
		return;
	FB_JsonField( body, "key", fb.key );
	FB_JsonField( body, "kind", fb.kind ? "idea" : "bug" );
	FB_JsonField( body, "title", fb.title );
	FB_JsonField( body, "text", fb.text );
	FB_JsonField( body, "info", FB_Details() );
	FB_JsonField( body, "account", fbAcc.name );
	body += "}";
	if ( FB_Ask( FB_REQ_REPORT, "/v1/report", body ) )
		FB_Message( qfalse, "Sending..." );
}

static fbReport_t *FB_Report( int id ) {
	for ( size_t i = 0; i < fb.reports.size(); i++ ) {
		if ( fb.reports[i].id == id )
			return &fb.reports[i];
	}
	return NULL;
}

static void FB_SendComment( void ) {
	const fbReport_t *r = FB_Report( fb.selected );
	std::string body = "{";

	if ( FB_Busy() || !r || !fb.comment[0] )
		return;
	FB_JsonField( body, "key", fb.key );
	body += va( ",\"id\":%i", r->id );
	FB_JsonField( body, "text", fb.comment );
	body += "}";
	if ( FB_Ask( FB_REQ_COMMENT, "/v1/comment", body ) )
		FB_Message( qfalse, "Sending..." );
}

// a field of an answer line: \t, \n and \\ undone, UTF-8 to the game's letters
static std::string FB_Unescape( const std::string &field ) {
	std::string plain, out;

	for ( size_t i = 0; i < field.size(); i++ ) {
		if ( field[i] == '\\' && i + 1 < field.size() ) {
			const char c = field[++i];

			plain += c == 'n' ? '\n' : c == 't' ? '\t' : c;
		} else {
			plain += field[i];
		}
	}
	for ( char *c = (char *)plain.c_str(); *c; ) {
		const uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
		const uint8_t got = ConvertUTF32ToExpectedCharset( utf32 );
		// a letter the game hasn't got (an emoji): a ? rather than whatever its low byte is
		const char ch = (char)( utf32 > 0xFF && got == ( utf32 & 0xFF ) ? '?' : got );

		// shown wrapped, on lines of their own: a line break is a space here
		out += ch == '\n' || ch == '\t' ? ' ' : ch;
	}
	return out;
}

static void FB_Split( const std::string &line, std::vector<std::string> &fields ) {
	size_t from = 0, tab;

	fields.clear();
	while ( ( tab = line.find( '\t', from ) ) != std::string::npos ) {
		fields.push_back( line.substr( from, tab - from ) );
		from = tab + 1;
	}
	fields.push_back( line.substr( from ) );
}

static void FB_ReadMine( const char *answer ) {
	std::vector<fbReport_t> reports;
	std::vector<std::string> f;
	const char *p = answer;

	while ( *p ) {
		const char *end = strchr( p, '\n' );
		const std::string line = end ? std::string( p, end - p ) : std::string( p );

		p = end ? end + 1 : p + strlen( p );
		FB_Split( line, f );
		if ( f[0] == "report" && f.size() >= 8 ) {
			fbReport_t r;

			r.id = atoi( f[1].c_str() );
			r.kind = f[2];
			r.status = f[3];
			r.created = atoi( f[4].c_str() );
			r.updated = atoi( f[5].c_str() );
			r.title = FB_Unescape( f[6] );
			r.text = FB_Unescape( f[7] );
			reports.push_back( r );
		} else if ( f[0] == "msg" && f.size() >= 6 ) {
			const int id = atoi( f[1].c_str() );

			for ( size_t i = 0; i < reports.size(); i++ ) {
				if ( reports[i].id == id ) {
					fbMessage_t m;

					m.who = f[2];
					m.time = atoi( f[3].c_str() );
					m.author = FB_Unescape( f[4] );
					m.text = FB_Unescape( f[5] );
					reports[i].messages.push_back( m );
					break;
				}
			}
		}
	}
	fb.reports.swap( reports );
}

// the time of a report's last answer: an answer, or its status changed
static int FB_LastAnswer( const fbReport_t &r ) {
	int last = 0;

	for ( size_t i = 0; i < r.messages.size(); i++ ) {
		if ( r.messages[i].who != "player" )
			last = Q_max( last, r.messages[i].time );
	}
	return last;
}

static qboolean FB_HasNews( const fbReport_t &r ) {
	const int last = FB_LastAnswer( r );
	std::map<int, int>::const_iterator it = fb.seen.find( r.id );

	return (qboolean)( last && ( it == fb.seen.end() || last > it->second ) );
}

static int FB_NewsCount( void ) {
	int n = 0;

	for ( size_t i = 0; i < fb.reports.size(); i++ )
		n += FB_HasNews( fb.reports[i] ) ? 1 : 0;
	return n;
}

static const char *FB_StatusWord( const std::string &status );

// a report's title for the console: its ^ as typed, not colours
static const char *FB_ConsoleTitle( const fbReport_t &r ) {
	static char title[FB_TITLE_LEN * 2 + 1];
	int n = 0;

	for ( const char *c = r.title.c_str(); *c && n < (int)sizeof( title ) - 2; c++ ) {
		title[n++] = *c;
		if ( *c == Q_COLOR_ESCAPE )
			title[n++] = ' ';
	}
	title[n] = '\0';
	return title;
}

// news not said yet, said in the console: an answer, the status changed, or both
static void FB_Announce( void ) {
	for ( size_t i = 0; i < fb.reports.size(); i++ ) {
		const fbReport_t &r = fb.reports[i];
		const int last = FB_LastAnswer( r );
		std::map<int, int>::const_iterator seen = fb.seen.find( r.id ), said = fb.announced.find( r.id );
		const int before = Q_max( seen != fb.seen.end() ? seen->second : 0, said != fb.announced.end() ? said->second : 0 );
		const fbMessage_t *status = NULL;
		qboolean answered = qfalse;

		if ( !last || last <= before )
			continue;
		fb.announced[r.id] = last;
		for ( size_t m = 0; m < r.messages.size(); m++ ) {
			const fbMessage_t &msg = r.messages[m];

			if ( msg.time <= before )
				continue;
			if ( msg.who == "staff" )
				answered = qtrue;
			else if ( msg.who == "status" )
				status = &msg;
		}
		if ( answered && status )
			Com_Printf( S_COLOR_YELLOW "Feedback: your report \"%s" S_COLOR_YELLOW "\" was answered, and %s. /feedback to read it\n",
				FB_ConsoleTitle( r ), status->text == "open" ? "is open again" : va( "is %s", FB_StatusWord( status->text ) ) );
		else if ( answered )
			Com_Printf( S_COLOR_YELLOW "Feedback: your report \"%s" S_COLOR_YELLOW "\" was answered. /feedback to read it\n", FB_ConsoleTitle( r ) );
		else if ( status )
			Com_Printf( S_COLOR_YELLOW "Feedback: your report \"%s" S_COLOR_YELLOW "\" is %s. /feedback to see it\n",
				FB_ConsoleTitle( r ), status->text == "open" ? "open again" : va( "now %s", FB_StatusWord( status->text ) ) );
	}
}

// the report looked at: its answers seen
static void FB_MarkSeen( const fbReport_t &r ) {
	const int last = FB_LastAnswer( r );
	std::map<int, int>::const_iterator it = fb.seen.find( r.id );

	if ( !last || ( it != fb.seen.end() && it->second >= last ) )
		return;
	fb.seen[r.id] = last;
	fb.announced[r.id] = Q_max( fb.announced[r.id], last );
	FB_SaveFile();
}

// what the bot said: the first line, for the player
static const char *FB_Answer( void ) {
	static char text[160];
	char *nl;

	if ( !fbHttp.status )
		return "the feedback server couldn't be reached";
	Q_strncpyz( text, fbHttp.answer, sizeof( text ) );
	if ( ( nl = strchr( text, '\n' ) ) != NULL )
		*nl = '\0';
	// a web page rather than the bot's words: Funnel's own, the bot being down
	if ( !text[0] || text[0] == '<' )
		return va( "the feedback server answered %i", fbHttp.status );
	return text;
}

static qboolean FB_Ok( void ) {
	return (qboolean)( fbHttp.status == 200 && !strncmp( fbHttp.answer, "ok", 2 ) );
}

static void FB_Done( fbRequest_t what ) {
	switch ( what ) {
	case FB_REQ_REPORT:
		if ( FB_Ok() ) {
			fb.sentId = atoi( fbHttp.answer + 3 );
			FB_Message( qfalse, "Sent, as report #%i: its answers come in My reports", fb.sentId );
			fb.title[0] = fb.text[0] = '\0';
			fb.focus = FB_NONE;
			WG_EditReset();
			FB_AskMine();
		} else {
			FB_Message( qtrue, "Not sent: %s", FB_Answer() );
		}
		break;
	case FB_REQ_COMMENT:
		if ( FB_Ok() ) {
			FB_Message( qfalse, "Sent" );
			fb.comment[0] = '\0';
			fb.focus = FB_NONE;
			WG_EditReset();
			fb.talkScroll = 1.0e9f;		// to the bottom, where it'll be
			FB_AskMine();
		} else {
			FB_Message( qtrue, "Not sent: %s", FB_Answer() );
		}
		break;
	case FB_REQ_MINE:
		if ( FB_Ok() ) {
			FB_ReadMine( fbHttp.answer );
			fb.listed = qtrue;
			fb.listError[0] = '\0';
			fb.listTime = cls.realtime;
			if ( fb.sentId && FB_Report( fb.sentId ) ) {
				fb.selected = fb.sentId;
				fb.sentId = 0;
			}
			if ( fb.open && fb.untouched && fb.page == 0 && FB_NewsCount() && !fb.title[0] && !fb.text[0] )
				fb.page = 1;
			fb.untouched = qfalse;
			FB_Announce();
		} else {
			Q_strncpyz( fb.listError, FB_Answer(), sizeof( fb.listError ) );
		}
		break;
	default:
		break;
	}
}

// each frame: an answer in, and the reports asked for now and then
void CL_Feedback_Frame( void ) {
	if ( !cl_feedbackUrl )
		return;
	FB_AccountFrame( (qboolean)( fb.open && fb.page == 0 ) );
	if ( FB_Busy() && HTTP_Poll( &fbHttp ) ) {
		const fbRequest_t what = fb.request;

		fb.request = FB_REQ_NONE;
		FB_Done( what );
	}
	if ( !FB_Busy() && fb.askAgain )
		FB_AskMine();
	// My reports looked at: asked more often
	if ( fb.open && fb.page == 1 && fb.nextPoll > cls.realtime + FB_POLL_OPEN_MS )
		fb.nextPoll = cls.realtime + FB_POLL_OPEN_MS;
	if ( fb.key[0] && !FB_Busy() && cls.realtime >= fb.nextPoll ) {
		fb.nextPoll = cls.realtime + ( fb.open && fb.page == 1 ? FB_POLL_OPEN_MS : FB_POLL_MS );
		FB_AskMine();
	}
}

/*
===============================================================================

Opening and closing

===============================================================================
*/

qboolean CL_Feedback_Active( void ) {
	return fb.open;
}

void CL_Feedback_Close( void ) {
	if ( !fb.open )
		return;
	fb.open = qfalse;
	fb.focus = FB_NONE;
	WG_EditReset();
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_FEEDBACK );
}

static void FB_Open( int page ) {
	if ( !fb.font )
		fb.font = re->RegisterFont( "arialnb" );
	if ( !fb.font )
		fb.font = cls.menuFont;
	if ( Key_GetCatcher() & KEYCATCH_CONSOLE )
		Con_Close();
	fb.page = page;
	fb.untouched = (qboolean)( page == 0 );
	if ( !fb.open ) {
		fb.cursorX = FB_X + FB_W * 0.5f;
		fb.cursorY = FB_Y + FB_H * 0.5f;
	}
	fb.open = qtrue;
	fb.click = qfalse;
	fb.wheel = 0;
	// there being reports, what's become of them
	if ( fb.key[0] )
		FB_AskMine();
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_FEEDBACK );
}

static void FB_Cmd_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "close" ) ) {
		CL_Feedback_Close();
	} else if ( !Q_stricmp( arg, "mine" ) || !Q_stricmp( arg, "reports" ) ) {
		FB_Open( 1 );
	} else if ( !arg[0] ) {
		if ( fb.open )
			CL_Feedback_Close();
		else
			FB_Open( fb.key[0] && FB_NewsCount() ? 1 : 0 );
	} else {
		Com_Printf( "usage: " FB_CMD "          a bug report or an idea for the OpenJK-RPMod project, and the answers\n" );
		Com_Printf( "       " FB_CMD " mine     your reports and their answers\n" );
		Com_Printf( "       " FB_CMD " close    closes the window\n" );
	}
}

void CL_Feedback_Init( void ) {
	// still asking from before (a restart of the client): the thread has the request
	const fbRequest_t asking = fb.request;

	fb.open = qfalse;
	fb.font = 0;
	fb.focus = FB_NONE;
	fb.request = asking;
	cl_feedbackUrl = Cvar_Get( "cl_feedbackUrl", FB_DEFAULT_URL, CVAR_ARCHIVE_ND | CVAR_PROTECTED,
		"Where /feedback sends bug reports and ideas, and asks for their answers" );
	Cmd_AddCommand( FB_CMD, FB_Cmd_f, "Send a bug report or an idea for the OpenJK-RPMod project, and read the answers" );
	FB_LoadFile();
	fb.nextPoll = cls.realtime + FB_FIRST_POLL_MS;
}

void CL_Feedback_Shutdown( void ) {
	CL_Feedback_Close();
	Cmd_RemoveCommand( FB_CMD );
	fb.font = 0;
}

void CL_Feedback_RendererRestarted( void ) {
	fb.font = 0;
	if ( fb.open ) {
		fb.font = re->RegisterFont( "arialnb" );
		if ( !fb.font )
			fb.font = cls.menuFont;
	}
}

/*
===============================================================================

The window

===============================================================================
*/

static void FB_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void FB_Box( float x, float y, float w, float h, const float *fill ) {
	FB_Fill( x, y, w, h, fill );
	FB_Fill( x, y, w, 1, fbBorder );
	FB_Fill( x, y + h - 1, w, 1, fbBorder );
	FB_Fill( x, y, 1, h, fbBorder );
	FB_Fill( x + w - 1, y, 1, h, fbBorder );
}

static float FB_TextWidth( const char *text ) {
	return (float)re->Font_StrLenPixels( text, fb.font, FB_SCALE );
}

static void FB_Text( float x, float y, const char *text, const float *color ) {
	re->Font_DrawString( (int)x, (int)y, text, color, fb.font | STYLE_DROPSHADOW, -1, FB_SCALE );
}

static void FB_TextClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	for ( len = strlen( buf ); len > 0 && FB_TextWidth( buf ) > w; len-- )
		buf[len - 1] = '\0';
	FB_Text( x, y, buf, color );
}

// text as typed, cut to w: a ^ drawn as itself, not taken for a colour
static void FB_PlainClipped( float x, float y, float w, const char *text, const float *color ) {
	char buf[MAX_STRING_CHARS], measured[MAX_STRING_CHARS];
	wgStyle_t style;
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	for ( len = strlen( buf ); len > 0; len-- ) {
		// measured with the ^ as a letter of its width, the font taking ^1 for a colour
		Q_strncpyz( measured, buf, sizeof( measured ) );
		for ( char *c = measured; *c; c++ ) {
			if ( *c == Q_COLOR_ESCAPE )
				*c = 'v';
		}
		if ( FB_TextWidth( measured ) <= w )
			break;
		buf[len - 1] = '\0';
	}
	style.font = fb.font;
	style.scale = FB_SCALE;
	style.lineH = FB_ROW_H;
	WG_DrawText( &style, x, y, 100000.0f, buf, 0, 1, color );
}

static qboolean FB_In( float x, float y, float w, float h ) {
	return (qboolean)( fb.cursorX >= x && fb.cursorX < x + w && fb.cursorY >= y && fb.cursorY < y + h );
}

static qboolean FB_Clicked( float x, float y, float w, float h ) {
	if ( !fb.click || fb.clickX < x || fb.clickX >= x + w || fb.clickY < y || fb.clickY >= y + h )
		return qfalse;
	fb.click = qfalse;
	return qtrue;
}

static void FB_Style( wgStyle_t *style ) {
	style->font = fb.font;
	style->scale = FB_SCALE;
	style->lineH = FB_ROW_H;
	style->padX = 4.0f;
	style->padY = 2.0f;
	style->fill = fbLight;
	style->border = fbBorder;
	style->text = fbWhite;
	style->dim = fbFaint;
}

static qboolean FB_Button( float x, float y, float w, const char *label, qboolean lit, qboolean enabled ) {
	FB_Box( x, y, w, FB_CTRL_H, lit ? fbLit : enabled && FB_In( x, y, w, FB_CTRL_H ) ? fbHover : fbLight );
	FB_Text( x + ( w - FB_TextWidth( label ) ) * 0.5f, y + 2, label, enabled ? fbWhite : fbFaint );
	return (qboolean)( FB_Clicked( x, y, w, FB_CTRL_H ) && enabled );
}

// a field typed in, one line, grown down while it's typed in and long
static void FB_Field( float x, float y, float w, char *buf, fbFocus_t which, const char *placeholder ) {
	const qboolean focused = (qboolean)( fb.focus == which );
	wgStyle_t style;

	FB_Style( &style );
	FB_Box( x, y, w, FB_CTRL_H, focused ? fbHover : fbLight );
	WG_Field( &style, x, y, w, FB_CTRL_H, buf, focused, placeholder, fb.cursorX, fb.cursorY );
	if ( FB_Clicked( x, y, w, WG_FieldHeight( &style, w, FB_CTRL_H, buf, focused ) ) ) {
		fb.focus = which;
		WG_FieldClick( buf, fb.clickX, fb.clickY );
	}
}

static const char *FB_StatusWord( const std::string &status ) {
	if ( status == "planned" )
		return "planned";
	if ( status == "fixed" )
		return "fixed";
	if ( status == "closed" )
		return "closed";
	return "open";
}

static const float *FB_StatusColor( const std::string &status ) {
	if ( status == "planned" )
		return fbBlue;
	if ( status == "fixed" )
		return fbGreen;
	if ( status == "closed" )
		return fbFaint;
	return fbAmber;
}

static const char *FB_Date( int time ) {
	static char text[32];
	const time_t t = (time_t)time;
	const struct tm *tm = localtime( &t );

	if ( !tm )
		return "";
	strftime( text, sizeof( text ), "%d %b %H:%M", tm );
	return text[0] == '0' ? text + 1 : text;
}

static void FB_DrawNew( float x, float y, float w ) {
	wgStyle_t style;
	const float boxH = FB_TEXT_ROWS * FB_ROW_H + 4;
	const qboolean busy = (qboolean)( fb.request == FB_REQ_REPORT );

	FB_TextClipped( x, y, w, "A bug you found, or an idea, for the OpenJK-RPMod project. The answers come back here,", fbDim );
	y += FB_ROW_H;
	FB_TextClipped( x, y, w, "under My reports.", fbDim );
	y += FB_ROW_H + FB_GAP + 4;

	FB_Text( x, y + 2, "It's", fbDim );
	if ( FB_Button( x + 50, y, 80, "A bug", (qboolean)( fb.kind == 0 ), qtrue ) )
		fb.kind = 0;
	if ( FB_Button( x + 134, y, 80, "An idea", (qboolean)( fb.kind == 1 ), qtrue ) )
		fb.kind = 1;
	y += FB_CTRL_H + FB_GAP + 2;

	FB_Text( x, y + 2, "Title", fbDim );
	FB_Text( x + w - FB_TextWidth( va( "%i/%i", (int)strlen( fb.title ), FB_TITLE_LEN ) ), y + 2,
		va( "%i/%i", (int)strlen( fb.title ), FB_TITLE_LEN ), fbFaint );
	y += FB_ROW_H + 2;
	FB_Field( x, y, w, fb.title, FB_TITLE, fb.kind ? "the idea, in a few words" : "what goes wrong, in a few words" );
	y += FB_CTRL_H + FB_GAP + 2;

	FB_Text( x, y + 2, fb.kind ? "The idea" : "What happened", fbDim );
	FB_Text( x + w - FB_TextWidth( va( "%i/%i", (int)strlen( fb.text ), FB_TEXT_LEN ) ), y + 2,
		va( "%i/%i", (int)strlen( fb.text ), FB_TEXT_LEN ), fbFaint );
	y += FB_ROW_H + 2;
	FB_Style( &style );
	FB_Box( x, y, w, boxH, fb.focus == FB_TEXT ? fbHover : fbLight );
	WG_TextBox( &style, x, y, w, boxH, fb.text, (qboolean)( fb.focus == FB_TEXT ),
		fb.kind ? "what it would be, and what for" : "what you did, what happened, and what you expected instead",
		fb.cursorX, fb.cursorY );
	if ( FB_Clicked( x, y, w, boxH ) ) {
		fb.focus = FB_TEXT;
		WG_FieldClick( fb.text, fb.clickX, fb.clickY );
	}
	y += boxH + FB_GAP + 2;

	// the account it goes with, and what goes with it
	if ( !FB_RpmodServer() )
		FB_TextClipped( x, y, w, "Reports are sent from an RPMod server, logged in to your RPMod account", fbAmber );
	else if ( fbAcc.state == FB_ACC_IN )
		FB_PlainClipped( x, y, w, va( "Sent with your RPMod account: %s", fbAcc.name ), fbWhite );
	else if ( fbAcc.state == FB_ACC_OUT )
		FB_TextClipped( x, y, w, "You're logged out: log in to your RPMod account to send a report", fbAmber );
	else if ( fbAcc.state == FB_ACC_NO_ANSWER )
		FB_TextClipped( x, y, w, "The server didn't say which RPMod account you're on: it's asked again in a moment", fbAmber );
	else
		FB_TextClipped( x, y, w, "Checking your RPMod account...", fbDim );
	y += FB_ROW_H + 4;
	FB_TextClipped( x, y, w, va( "Sent with it: %s", FB_Details() ), fbFaint );
	y += FB_ROW_H + FB_GAP + 4;

	if ( FB_Button( x + w - 110, y, 110, busy ? "Sending..." : "Send", qfalse,
		(qboolean)( !FB_Busy() && fb.title[0] && fb.text[0] && fbAcc.state == FB_ACC_IN ) ) )
		FB_SendReport();
	FB_TextClipped( x, y + 2, w - 120, S_COLOR_GREY "One bug or idea a report, please: each is answered on its own", fbWhite );
}

// a report in the list: its title, and its kind, status and number
static void FB_DrawRow( const fbReport_t &r, float x, float y, float w, qboolean selected ) {
	const qboolean news = FB_HasNews( r );

	if ( selected )
		FB_Fill( x, y, w, FB_ROW_H * 2 + 2, fbLit );
	else if ( FB_In( x, y, w, FB_ROW_H * 2 + 2 ) )
		FB_Fill( x, y, w, FB_ROW_H * 2 + 2, fbHover );
	if ( news )
		FB_Fill( x + 2, y + 4, 4, 4, fbNews );
	FB_PlainClipped( x + 9, y + 1, w - 12, r.title.c_str(), fbWhite );
	FB_Text( x + 9, y + FB_ROW_H + 1, r.kind == "idea" ? "Idea" : "Bug", fbDim );
	FB_Text( x + 9 + 30, y + FB_ROW_H + 1, FB_StatusWord( r.status ), FB_StatusColor( r.status ) );
	FB_TextClipped( x + 9 + 80, y + FB_ROW_H + 1, w - 92, news ? "news" : va( "#%i", r.id ), news ? fbNews : fbFaint );
}

// one part of a report's conversation, as drawn: who and when, then the text
typedef struct {
	std::string	head;
	const float	*color;
	std::string	text;
} fbEntry_t;

static void FB_Entries( const fbReport_t &r, std::vector<fbEntry_t> &out ) {
	fbEntry_t e;

	out.clear();
	e.head = va( "You, %s", FB_Date( r.created ) );
	e.color = fbDim;
	e.text = r.text;
	out.push_back( e );
	for ( size_t i = 0; i < r.messages.size(); i++ ) {
		const fbMessage_t &m = r.messages[i];

		if ( m.who == "status" ) {
			e.head = m.text == "open" ? va( "Opened again by %s, %s", m.author.c_str(), FB_Date( m.time ) )
				: va( "Marked %s by %s, %s", FB_StatusWord( m.text ), m.author.c_str(), FB_Date( m.time ) );
			e.color = FB_StatusColor( m.text );
			e.text = "";
		} else if ( m.who == "staff" ) {
			e.head = va( "%s, OpenJK-RPMod, %s", m.author.c_str(), FB_Date( m.time ) );
			e.color = fbBlue;
			e.text = m.text;
		} else {
			e.head = va( "You, %s", FB_Date( m.time ) );
			e.color = fbDim;
			e.text = m.text;
		}
		out.push_back( e );
	}
}

static void FB_DrawTalk( const fbReport_t &r, float x, float y, float w, float h ) {
	std::vector<fbEntry_t> entries;
	wgStyle_t style;
	const qboolean closed = (qboolean)( r.status == "closed" );
	const float viewY = y + FB_ROW_H * 2 + 8, viewH = h - ( viewY - y ) - FB_CTRL_H - FB_GAP - 2;
	float content = 0.0f, at, textW;

	FB_Style( &style );
	FB_PlainClipped( x, y, w, r.title.c_str(), fbWhite );
	FB_Text( x, y + FB_ROW_H + 1, r.kind == "idea" ? "Idea" : "Bug", fbDim );
	FB_Text( x + 30, y + FB_ROW_H + 1, FB_StatusWord( r.status ), FB_StatusColor( r.status ) );
	FB_Text( x + 80, y + FB_ROW_H + 1, va( "#%i", r.id ), fbFaint );
	FB_MarkSeen( r );

	// how tall it all is, then a view of it
	FB_Entries( r, entries );
	textW = w - 8 - WG_SCROLL_W - 2;
	for ( size_t i = 0; i < entries.size(); i++ )
		content += FB_ROW_H + WG_TextLines( &style, entries[i].text.c_str(), textW ) * FB_ROW_H + 6;
	if ( fb.wheel && FB_In( x, viewY, w, viewH ) ) {
		fb.talkScroll += fb.wheel * FB_ROW_H * 3;
		fb.wheel = 0;
	}
	fb.talkScroll = Com_Clamp( 0.0f, Q_max( 0.0f, content - viewH ), fb.talkScroll );
	FB_Box( x, viewY - 2, w, viewH + 4, fbLight );
	WG_ScrollbarPixels( x + w - WG_SCROLL_W - 1, viewY - 1, viewH + 2, content, &fb.talkScroll,
		fb.cursorX, fb.cursorY, &fb.click, fb.clickX, fb.clickY );

	// the lines wholly in the view
	at = viewY - fb.talkScroll;
	for ( size_t i = 0; i < entries.size(); i++ ) {
		const fbEntry_t &e = entries[i];
		const int lines = WG_TextLines( &style, e.text.c_str(), textW );

		if ( at >= viewY && at + FB_ROW_H <= viewY + viewH )
			FB_PlainClipped( x + 4, at, textW, e.head.c_str(), e.color );
		at += FB_ROW_H;
		if ( lines ) {
			const int first = at < viewY ? (int)ceilf( ( viewY - at ) / FB_ROW_H ) : 0;
			const int last = (int)floorf( ( viewY + viewH - at ) / FB_ROW_H );	// lines before it fit

			if ( last > first )
				WG_DrawText( &style, x + 4, at + first * FB_ROW_H, textW, e.text.c_str(), first, last - first, fbWhite );
		}
		at += lines * FB_ROW_H + 6;
	}

	// answering
	y = viewY + viewH + FB_GAP;
	if ( closed ) {
		FB_TextClipped( x, y + 2, w, "This report is closed: it can't be answered any more", fbFaint );
		return;
	}
	FB_Field( x, y, w - 64, fb.comment, FB_COMMENT, "your answer" );
	if ( FB_Button( x + w - 60, y, 60, fb.request == FB_REQ_COMMENT ? "..." : "Send", qfalse, (qboolean)( !FB_Busy() && fb.comment[0] ) ) )
		FB_SendComment();
}

static void FB_DrawMine( float x, float y, float w, float h ) {
	const int rows = (int)( ( h - FB_CTRL_H - FB_GAP - 4 ) / ( FB_ROW_H * 2 + 2 ) );
	const float listY = y + FB_CTRL_H + FB_GAP;
	float rowW = FB_LIST_W;
	const fbReport_t *shown;

	if ( FB_Button( x, y, 60, fb.request == FB_REQ_MINE ? "..." : "Refresh", qfalse, (qboolean)( !FB_Busy() && fb.key[0] ) ) )
		FB_AskMine();
	if ( fb.listError[0] )
		FB_TextClipped( x + 66, y + 2, FB_LIST_W - 66, va( "%s", fb.listError ), fbRed );
	else if ( fb.listed )
		FB_TextClipped( x + 66, y + 2, FB_LIST_W - 66, ( cls.realtime - fb.listTime ) < 60000 ? "just now"
			: va( "%i min ago", ( cls.realtime - fb.listTime ) / 60000 ), fbFaint );

	if ( !fb.key[0] || ( fb.listed && fb.reports.empty() ) ) {
		FB_TextClipped( x, listY + 4, w, "No reports yet: what you send under New report shows here, with its answers.", fbDim );
		return;
	}
	if ( !fb.listed ) {
		FB_TextClipped( x, listY + 4, w, fb.listError[0] ? "Your reports couldn't be fetched: Refresh to try again" : "Asking for your reports...", fbDim );
		return;
	}

	// the list
	if ( fb.wheel && FB_In( x, listY, FB_LIST_W, rows * ( FB_ROW_H * 2 + 2 ) ) ) {
		fb.listScroll += fb.wheel;
		fb.wheel = 0;
	}
	fb.listScroll = Com_Clampi( 0, Q_max( 0, (int)fb.reports.size() - rows ), fb.listScroll );
	FB_Box( x, listY, FB_LIST_W, rows * ( FB_ROW_H * 2 + 2 ) + 2, fbLight );
	if ( WG_Scrollbar( x + FB_LIST_W - WG_SCROLL_W - 1, listY + 1, rows * ( FB_ROW_H * 2 + 2 ), (int)fb.reports.size(), rows,
		&fb.listScroll, fb.cursorX, fb.cursorY, &fb.click, fb.clickX, fb.clickY ) )
		rowW -= WG_SCROLL_W + 1;
	for ( int i = 0; i < rows && fb.listScroll + i < (int)fb.reports.size(); i++ ) {
		const fbReport_t &r = fb.reports[fb.listScroll + i];
		const float ry = listY + 1 + i * ( FB_ROW_H * 2 + 2 );

		FB_DrawRow( r, x + 1, ry, rowW - 2, (qboolean)( r.id == fb.selected ) );
		if ( FB_Clicked( x, ry, rowW, FB_ROW_H * 2 + 2 ) && fb.selected != r.id ) {
			fb.selected = r.id;
			fb.talkScroll = 1.0e9f;		// the newest, at the bottom
			fb.comment[0] = '\0';
			if ( fb.focus == FB_COMMENT )
				fb.focus = FB_NONE;
		}
	}

	// none picked: the first with news
	if ( !FB_Report( fb.selected ) ) {
		for ( size_t i = 0; i < fb.reports.size(); i++ ) {
			if ( FB_HasNews( fb.reports[i] ) ) {
				fb.selected = fb.reports[i].id;
				fb.talkScroll = 1.0e9f;
				break;
			}
		}
	}
	shown = FB_Report( fb.selected );
	if ( !shown ) {
		FB_TextClipped( x + FB_LIST_W + 12, listY + 4, w - FB_LIST_W - 12, "Pick a report to read it and its answers", fbDim );
		return;
	}
	FB_DrawTalk( *shown, x + FB_LIST_W + 12, y, w - FB_LIST_W - 12, h );
}

void CL_Feedback_Draw( void ) {
	const float x = FB_X + 10, w = FB_W - 20;
	const int news = FB_NewsCount();
	float y = FB_Y + 6;

	if ( !fb.open )
		return;
	if ( !( Key_GetCatcher() & KEYCATCH_FEEDBACK ) ) {
		fb.open = qfalse;
		fb.focus = FB_NONE;
		return;
	}
	if ( !fb.font )
		CL_Feedback_RendererRestarted();

	FB_Box( FB_X, FB_Y, FB_W, FB_H, fbPanel );
	FB_Text( x, y, "Feedback", fbWhite );
	if ( FB_Button( x + 70, y - 2, 90, "New report", (qboolean)( fb.page == 0 ), qtrue ) ) {
		fb.page = 0;
		fb.focus = FB_NONE;
	}
	if ( FB_Button( x + 164, y - 2, 110, news ? va( "My reports (%i new)", news ) : "My reports", (qboolean)( fb.page == 1 ), qtrue ) ) {
		if ( fb.page != 1 ) {
			fb.page = 1;
			fb.focus = FB_NONE;
			FB_AskMine();
		}
	}
	if ( FB_Button( FB_X + FB_W - 10 - 80, y - 2, 80, "Close (Esc)", qfalse, qtrue ) ) {
		CL_Feedback_Close();
		return;
	}
	y += FB_CTRL_H + FB_GAP + 4;

	if ( fb.page == 0 )
		FB_DrawNew( x, y, w );
	else
		FB_DrawMine( x, y, w, FB_Y + FB_H - FB_ROW_H - 10 - y );

	if ( fb.message[0] && cls.realtime - fb.messageTime < 8000 )
		FB_TextClipped( x, FB_Y + FB_H - FB_ROW_H - 4, w, fb.message, fb.messageError ? fbRed : fbGreen );

	// a click on nothing: the field typed in let go
	if ( fb.click )
		fb.focus = FB_NONE;
	fb.click = qfalse;
	fb.wheel = 0;
	WG_DrawOverlay();
	re->SetColor( NULL );
	re->DrawStretchPic( fb.cursorX, fb.cursorY, 24, 24, 0, 0, 1, 1, cls.cursorShader );
}

/*
===============================================================================

Input

===============================================================================
*/

static char *FB_FocusBuf( int *size ) {
	switch ( fb.focus ) {
	case FB_TITLE:
		*size = sizeof( fb.title );
		return fb.title;
	case FB_TEXT:
		*size = sizeof( fb.text );
		return fb.text;
	case FB_COMMENT:
		*size = sizeof( fb.comment );
		return fb.comment;
	default:
		*size = 0;
		return NULL;
	}
}

void CL_Feedback_KeyEvent( int key, qboolean down ) {
	int size;
	char *buf;

	if ( !fb.open || !down )
		return;
	buf = FB_FocusBuf( &size );
	if ( buf && key != A_MOUSE1 && key != A_MOUSE2 && key != A_MWHEELUP && key != A_MWHEELDOWN && key != A_TAB
		&& key != A_ENTER && key != A_KP_ENTER && WG_EditKey( buf, size, key, NULL ) )
		return;
	fb.untouched = qfalse;
	switch ( key ) {
	case A_MOUSE1:
		fb.click = qtrue;
		fb.clickX = fb.cursorX;
		fb.clickY = fb.cursorY;
		return;
	case A_MWHEELUP:
		fb.wheel--;
		return;
	case A_MWHEELDOWN:
		fb.wheel++;
		return;
	case A_TAB:
		// the title, then what happened
		if ( fb.page == 0 ) {
			fb.focus = fb.focus == FB_TITLE ? FB_TEXT : FB_TITLE;
			WG_EditReset();
		}
		return;
	case A_ENTER:
	case A_KP_ENTER:
		if ( fb.focus == FB_TITLE ) {
			fb.focus = FB_TEXT;
			WG_EditReset();
		} else if ( fb.focus == FB_COMMENT ) {
			FB_SendComment();
		}
		return;
	default:
		return;
	}
}

void CL_Feedback_CharEvent( int ch ) {
	int size;
	char *buf;

	if ( !fb.open || !( buf = FB_FocusBuf( &size ) ) )
		return;
	WG_EditChar( buf, size, ch, NULL );
}

void CL_Feedback_MouseEvent( int dx, int dy ) {
	fb.cursorX = Com_Clamp( 0, SCREEN_WIDTH, fb.cursorX + dx );
	fb.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, fb.cursorY + dy );
}

void CL_Feedback_Escape( void ) {
	if ( fb.focus != FB_NONE ) {
		fb.focus = FB_NONE;
		WG_EditReset();
	} else {
		CL_Feedback_Close();
	}
}
