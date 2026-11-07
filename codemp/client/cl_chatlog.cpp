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

// cl_chatlog.cpp -- the chat kept in files as you play: plain text, and a page like the console
//
//   chatlog               opens or closes its window (also in the in-game menu's TOOLS)
//   chatlog start|stop    records, or stops, for this server
//   chatlog open          today's page in the browser
//   chatlog folder        the folder of the logs
//
// A day on a server is a pair of files in chatlogs/: "<date> <server>.txt" and ".html", written
// a line at a time, so the page can be reloaded while playing. Chat is taken as the server sends
// it, once whispers, languages and chat range have had it: what you saw. Normal chat always goes
// in; tells, team chat, whispers and comms, OOC, emotes and admin chat each have a setting.
// Admin chat on RPMod is "<Admin>(name) : text"; a tell, base JKA's "[name]: text" in ^6.

#include "client.h"
#include "cl_widgets.h"

#define CL_CMD			"chatlog"
#define CL_DIR			"chatlogs"
#define CL_TELL_MS		3000	// a tell of yours coming back this soon after it went: to whom
#define CL_MESSAGE_MS	5000

typedef enum {
	CK_NORMAL,
	CK_TELL,
	CK_TEAM,
	CK_WHISPER,
	CK_OOC,
	CK_EMOTE,
	CK_ADMIN,
	CK_INFO,		// the log's own lines: joined, left
	CK_NUM
} ckKind_t;

static const char *ckClasses[CK_NUM] = { "normal", "tell", "team", "whisper", "ooc", "emote", "admin", "info" };
static const char *ckTitles[CK_NUM] = { "Chat", "Tells", "Team chat", "Whispers and comms", "OOC (( ))", "Emotes (/me)", "Admin chat", "Joined and left" };

static cvar_t *cl_chatLog;					// starts by itself on joining a server
static cvar_t *cl_chatLogKind[CK_NUM];		// each kind kept or not (NULL: always)

static struct {
	qboolean		recording;
	qboolean		stopped;		// stopped by hand: not started again by itself on this server
	fileHandle_t	txt, html;
	char			base[MAX_QPATH];	// chatlogs/<date> <server>, no extension
	int				day;			// the day the files are of
	char			tellTo[MAX_NETNAME];
	int				tellTime;
} lg;

/*
===============================================================================

THE TEXT

===============================================================================
*/

// any ^x, RPMod's ^a-^z and ^A-^Z too
static qboolean CL_IsCode( const char *p ) {
	return (qboolean)( p[0] == Q_COLOR_ESCAPE && p[1] && ( isalnum( (unsigned char)p[1] ) ) );
}

// the text without colour codes, the chat escape character or control characters
static void CL_Plain( const char *in, char *out, int size ) {
	int len = 0;

	for ( ; *in && len < size - 1; in++ ) {
		if ( CL_IsCode( in ) ) {
			in++;
			continue;
		}
		if ( (unsigned char)*in < ' ' )
			continue;
		out[len++] = *in;
	}
	out[len] = '\0';
}

// a player's name, by client number
static const char *CL_PlayerName( int num ) {
	if ( num < 0 || num >= MAX_CLIENTS || !cl.gameState.stringOffsets[CS_PLAYERS + num] )
		return "";
	return Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + num], "n" );
}

static const char *CL_ServerInfo( const char *key ) {
	return Info_ValueForKey( cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO], key );
}

// the line as the console shows it, and its kind; qfalse if the command isn't chat
static qboolean CL_ChatLine( char *line, int size, ckKind_t *kind ) {
	const char *cmd = Cmd_Argv( 0 );
	char raw[MAX_STRING_CHARS], plain[MAX_STRING_CHARS], msgPlain[MAX_STRING_CHARS], self[MAX_NETNAME];
	const char *msg;
	qboolean team = qfalse;
	int o = 0;

	if ( !Q_stricmp( cmd, "chat" ) || !Q_stricmp( cmd, "tchat" ) ) {
		Q_strncpyz( raw, Cmd_Argv( 1 ), sizeof( raw ) );
		team = (qboolean)( !Q_stricmp( cmd, "tchat" ) );
	} else if ( !Q_stricmp( cmd, "lchat" ) || !Q_stricmp( cmd, "ltchat" ) ) {
		// as the game prints it: name<location> ^color message
		Com_sprintf( raw, sizeof( raw ), "%s^7<%s> ^%s\x19:%s", Cmd_Argv( 1 ), Cmd_Argv( 2 ), Cmd_Argv( 3 ), Cmd_Argv( 4 ) );
		team = (qboolean)( !Q_stricmp( cmd, "ltchat" ) );
	} else {
		return qfalse;
	}
	if ( !raw[0] )
		return qfalse;

	// what's said, after the name's "\x19:"
	msg = strstr( raw, "\x19:" );
	CL_Plain( raw, plain, sizeof( plain ) );
	CL_Plain( msg ? msg + 2 : "", msgPlain, sizeof( msgPlain ) );
	{
		const char *p = msgPlain;

		while ( *p == ' ' )
			p++;
		memmove( msgPlain, p, strlen( p ) + 1 );
	}

	if ( !Q_stricmpn( plain, "<Admin>", 7 ) )
		*kind = CK_ADMIN;
	else if ( plain[0] == '[' && strchr( plain, ']' ) && msg )
		*kind = CK_TELL;
	else if ( team )
		*kind = CK_TEAM;
	else if ( msg && ( strstr( msg, "^k" ) || !Q_stricmpn( msgPlain, "comm-", 5 ) || !Q_stricmpn( plain, "comm-", 5 ) ) )
		*kind = CK_WHISPER;
	else if ( msg && !strncmp( msgPlain, "((", 2 ) )
		*kind = CK_OOC;
	else if ( !msg && strchr( raw, '\x19' ) )
		*kind = CK_EMOTE;
	else
		*kind = CK_NORMAL;

	// the console's line: the escape character left out
	for ( const char *p = raw; *p && o < size - 1; p++ ) {
		if ( *p != '\x19' )
			line[o++] = *p;
	}
	line[o] = '\0';

	// a tell of yours comes back with your name: to whom it went
	if ( *kind == CK_TELL && lg.tellTo[0] && cls.realtime - lg.tellTime < CL_TELL_MS ) {
		CL_Plain( CL_PlayerName( clc.clientNum ), self, sizeof( self ) );
		if ( self[0] && !Q_stricmpn( plain + 1, self, strlen( self ) ) && plain[1 + strlen( self )] == ']' ) {
			const char *to = va( "^7to %s^7 ", lg.tellTo );

			if ( strlen( to ) + strlen( line ) < (size_t)size ) {
				memmove( line + strlen( to ), line, strlen( line ) + 1 );
				memcpy( line, to, strlen( to ) );
			}
			lg.tellTo[0] = '\0';
		}
	}
	return qtrue;
}

/*
===============================================================================

THE FILES

===============================================================================
*/

static void CL_Write( fileHandle_t f, const char *text ) {
	if ( f )
		FS_Write( text, strlen( text ), f );
}

// "[13:37:00]"
static const char *CL_Time( void ) {
	qtime_t t;

	Com_RealTime( &t );
	return va( "[%02i:%02i:%02i]", t.tm_hour, t.tm_min, t.tm_sec );
}

// the line in the page: its colours as spans (or left out, plain), the text made safe for HTML
static void CL_WriteHtmlText( const char *text, qboolean colors ) {
	char out[MAX_STRING_CHARS * 4];
	int o = 0;
	qboolean open = qfalse;

	out[0] = '\0';
	for ( const char *p = text; *p && o < (int)sizeof( out ) - 64; p++ ) {
		const float *color = Con_TypedColor( p );

		if ( color && !colors ) {
			p++;
			continue;
		}
		if ( color ) {
			o += Com_sprintf( out + o, sizeof( out ) - o, "%s<span style=\"color:#%02x%02x%02x\">", open ? "</span>" : "",
				(int)( color[0] * 255 + 0.5f ), (int)( color[1] * 255 + 0.5f ), (int)( color[2] * 255 + 0.5f ) );
			open = qtrue;
			p++;
			continue;
		}
		switch ( *p ) {
		case '&':	o += Com_sprintf( out + o, sizeof( out ) - o, "&amp;" ); break;
		case '<':	o += Com_sprintf( out + o, sizeof( out ) - o, "&lt;" ); break;
		case '>':	o += Com_sprintf( out + o, sizeof( out ) - o, "&gt;" ); break;
		case '"':	o += Com_sprintf( out + o, sizeof( out ) - o, "&quot;" ); break;
		default:
			if ( (unsigned char)*p >= ' ' )
				out[o++] = *p;
			break;
		}
	}
	out[o] = '\0';
	CL_Write( lg.html, out );
	if ( open )
		CL_Write( lg.html, "</span>" );
}

static void CL_WriteLine( ckKind_t kind, const char *line ) {
	char plain[MAX_STRING_CHARS];
	const char *time = CL_Time();

	if ( !lg.recording )
		return;
	CL_Plain( line, plain, sizeof( plain ) );
	CL_Write( lg.txt, va( "%s %s\r\n", time, plain ) );
	CL_Write( lg.html, va( "<div class=\"l k-%s\"><span class=\"t\">%s</span> ", ckClasses[kind], time ) );
	CL_WriteHtmlText( line, (qboolean)( kind != CK_INFO ) );
	CL_Write( lg.html, "</div>\n" );
}

// the page's head: the console's look, and a box for each kind to show or hide it
static void CL_WriteHtmlHead( const char *title ) {
	CL_Write( lg.html,
		"<!DOCTYPE html>\n<html><head><meta charset=\"windows-1252\">\n" );
	CL_Write( lg.html, "<title>" );
	CL_WriteHtmlText( title, qfalse );
	CL_Write( lg.html, "</title>\n<style>\n"
		"body{margin:0;background:#06080d;background-image:linear-gradient(#0d1220,#05070b);background-attachment:fixed;color:#fff;"
		"font:15px/1.3 'Lucida Console',Consolas,'Courier New',monospace;text-shadow:1px 1px 0 #000}\n"
		"#bar{position:sticky;top:0;background:#0b0f1acc;border-bottom:2px solid #829bd8;padding:6px 12px;font-size:13px;color:#829bd8}\n"
		"#bar b{margin-right:14px}#bar label{margin-right:12px;cursor:pointer;color:#ccd}\n"
		"#log{padding:8px 12px 24px}.l{white-space:pre-wrap;word-wrap:break-word}\n"
		".t{color:#808080}.k-info{color:#829bd8;margin:6px 0}\n" );
	for ( int k = 0; k < CK_NUM; k++ )
		CL_Write( lg.html, va( ".hide-%s .k-%s{display:none}\n", ckClasses[k], ckClasses[k] ) );
	CL_Write( lg.html, "</style></head><body>\n<div id=\"bar\"><b>" );
	CL_WriteHtmlText( title, qtrue );
	CL_Write( lg.html, "</b>" );
	for ( int k = 0; k < CK_NUM; k++ )
		CL_Write( lg.html, va( "<label><input type=\"checkbox\" checked data-k=\"%s\"> %s</label>", ckClasses[k], ckTitles[k] ) );
	CL_Write( lg.html, "</div>\n<script>document.getElementById('bar').addEventListener('change',function(e){"
		"document.body.classList.toggle('hide-'+e.target.dataset.k,!e.target.checked);});</script>\n<div id=\"log\">\n" );
}

static void CL_Close( void ) {
	if ( lg.txt )
		FS_FCloseFile( lg.txt );
	if ( lg.html )
		FS_FCloseFile( lg.html );
	lg.txt = lg.html = 0;
	lg.recording = qfalse;
}

// today's pair for this server: chatlogs/<date> <server>
static void CL_TodaysBase( char *out, int size ) {
	char name[MAX_STRING_CHARS], safe[64];
	qtime_t t;
	int n = 0;

	Com_RealTime( &t );
	CL_Plain( CL_ServerInfo( "sv_hostname" ), name, sizeof( name ) );
	for ( const char *p = name; *p && n < (int)sizeof( safe ) - 1; p++ ) {
		if ( isalnum( (unsigned char)*p ) || *p == '-' || *p == '_' || *p == '(' || *p == ')' || *p == '.'
			|| ( *p == ' ' && n && safe[n - 1] != ' ' ) )
			safe[n++] = *p;
	}
	while ( n && ( safe[n - 1] == ' ' || safe[n - 1] == '.' ) )
		n--;
	safe[n] = '\0';
	Com_sprintf( out, size, CL_DIR "/%04i-%02i-%02i %s", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, safe[0] ? safe : "server" );
}

static qboolean CL_Start( void ) {
	char map[MAX_QPATH], title[MAX_STRING_CHARS];
	qboolean newHtml;
	qtime_t t;

	if ( lg.recording )
		return qtrue;
	if ( cls.state != CA_ACTIVE || clc.demoplaying )
		return qfalse;
	CL_TodaysBase( lg.base, sizeof( lg.base ) );
	newHtml = (qboolean)!FS_FileExists( va( "%s.html", lg.base ) );
	if ( !newHtml && !FS_FileExists( va( "%s.txt", lg.base ) ) )
		newHtml = qfalse;
	FS_FOpenFileByMode( va( "%s.txt", lg.base ), &lg.txt, FS_APPEND_SYNC );
	FS_FOpenFileByMode( va( "%s.html", lg.base ), &lg.html, FS_APPEND_SYNC );
	if ( !lg.txt || !lg.html ) {
		Com_Printf( S_COLOR_YELLOW "Chat log: couldn't write %s\n", lg.base );
		CL_Close();
		return qfalse;
	}
	Com_RealTime( &t );
	lg.day = t.tm_yday;
	lg.recording = qtrue;
	lg.stopped = qfalse;
	Com_sprintf( title, sizeof( title ), "%s^7, %04i-%02i-%02i", CL_ServerInfo( "sv_hostname" ), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday );
	if ( newHtml )
		CL_WriteHtmlHead( title );
	COM_StripExtension( COM_SkipPath( cl.mapname ), map, sizeof( map ) );
	CL_WriteLine( CK_INFO, va( "^7== Joined %s^7 (%s) on %s as %s^7 ==", CL_ServerInfo( "sv_hostname" ), NET_AdrToString( &clc.serverAddress ),
		map, CL_PlayerName( clc.clientNum ) ) );
	Com_Printf( "Chat log: recording to %s.txt and .html\n", lg.base );
	return qtrue;
}

static void CL_Stop( const char *why ) {
	if ( !lg.recording )
		return;
	CL_WriteLine( CK_INFO, va( "^7== %s ==", why ) );
	CL_Close();
}

/*
===============================================================================

FROM THE GAME

===============================================================================
*/

// a server command about to reach cgame: kept if it's chat of a kind that's wanted
void CL_ChatLog_ServerCommand( void ) {
	char line[MAX_STRING_CHARS];
	ckKind_t kind;

	if ( !lg.recording || !CL_ChatLine( line, sizeof( line ), &kind ) )
		return;
	if ( cl_chatLogKind[kind] && !cl_chatLogKind[kind]->integer )
		return;
	CL_WriteLine( kind, line );
}

// a command going to the server: a tell's target, to say whom it went to when it comes back
void CL_ChatLog_OutgoingCommand( const char *cmd ) {
	const char *name;

	if ( Q_stricmpn( cmd, "tell ", 5 ) )
		return;
	name = CL_PlayerName( atoi( cmd + 5 ) );
	if ( !name[0] )
		return;
	Q_strncpyz( lg.tellTo, name, sizeof( lg.tellTo ) );
	lg.tellTime = cls.realtime;
}

// in game: started by itself if it's set to, and a new pair at midnight
void CL_ChatLog_Frame( void ) {
	if ( lg.recording ) {
		qtime_t t;

		Com_RealTime( &t );
		if ( t.tm_yday != lg.day ) {
			CL_Stop( "The day ends" );
			CL_Start();
		}
		return;
	}
	if ( cl_chatLog->integer && !lg.stopped && cls.state == CA_ACTIVE && !clc.demoplaying && cl.snap.valid )
		CL_Start();
}

void CL_ChatLog_Disconnect( void ) {
	CL_Stop( "Left" );
	lg.stopped = qfalse;
	lg.tellTo[0] = '\0';
}

/*
===============================================================================

THE WINDOW

===============================================================================
*/

#define CW_W			340.0f
#define CW_H			300.0f
#define CW_X			( ( SCREEN_WIDTH - CW_W ) * 0.5f )
#define CW_Y			( ( SCREEN_HEIGHT - CW_H ) * 0.5f )
#define CW_ROW_H		20.0f
#define CW_CTRL_H		16.0f
#define CW_TEXT_SCALE	0.8f

static const float cwPanel[4]		= { 0.0f, 0.0f, 0.08f, 0.9f };
static const float cwBorder[4]		= { 0.5f, 0.5f, 0.7f, 1.0f };
static const float cwItem[4]		= { 0.15f, 0.15f, 0.25f, 0.8f };
static const float cwItemHover[4]	= { 0.25f, 0.25f, 0.4f, 0.9f };
static const float cwItemOn[4]		= { 0.35f, 0.24f, 0.05f, 0.9f };
static const float cwOrange[4]		= { 1.0f, 0.682f, 0.0f, 1.0f };
static const float cwWhite[4]		= { 1.0f, 1.0f, 1.0f, 1.0f };
static const float cwDim[4]			= { 0.6f, 0.6f, 0.7f, 1.0f };
static const float cwGreen[4]		= { 0.4f, 1.0f, 0.4f, 1.0f };

static struct {
	qboolean	open;
	int			font, titleFont;
	sfxHandle_t	clickSound;
	float		cursorX, cursorY;
	qboolean	click;
	float		clickX, clickY;
	char		message[MAX_STRING_CHARS];
	int			messageTime;
} cw;

static void CW_Message( const char *text ) {
	Q_strncpyz( cw.message, text, sizeof( cw.message ) );
	cw.messageTime = cls.realtime;
}

static void CW_Close( void ) {
	if ( !cw.open )
		return;
	cw.open = qfalse;
	Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_CHATLOG );
}

void CL_ChatLog_Close( void ) {
	CW_Close();
}

static void CW_Open( void ) {
	if ( cw.open )
		return;
	if ( cls.state != CA_ACTIVE || !cls.cgameStarted ) {
		Com_Printf( "Chat log: join a server first (chatlog start|stop|open|folder work from the console)\n" );
		return;
	}
	CL_ModelManager_Close();
	CL_NpcManager_Close();
	CL_ShaderManager_Close();
	CL_EffectManager_Close();
	CL_Storyteller_Close();
	CL_RpCinematic_EditorClose();
	CL_MapEditor_Close();
	CL_SoundManager_Close();
	CL_AnimManager_Close();
	CL_Presets_Close();
	CL_SpecialMoves_Close();
	CL_AssetManager_Close();
	CL_RadialMenu_CloseSettings();
	CL_Languages_Close();

	cw.font = re->RegisterFont( "arialnb" );
	if ( !cw.font )
		cw.font = cls.menuFont;
	cw.titleFont = re->RegisterFont( "ergoec" );
	if ( !cw.titleFont )
		cw.titleFont = cw.font;
	cw.clickSound = S_RegisterSound( "sound/interface/button1.wav" );
	cw.cursorX = SCREEN_WIDTH * 0.5f;
	cw.cursorY = SCREEN_HEIGHT * 0.5f;
	cw.click = qfalse;
	cw.open = qtrue;
	Key_SetCatcher( Key_GetCatcher() | KEYCATCH_CHATLOG );
}

// opens a file or folder of the game's in the desktop's own program
static void CL_OpenPath( const char *qpath, qboolean folder ) {
	const char *os;

	if ( !folder && !FS_FileExists( qpath ) ) {
		CW_Message( va( S_COLOR_YELLOW "There's no %s yet: nothing recorded today here", qpath ) );
		Com_Printf( "Chat log: there's no %s yet\n", qpath );
		return;
	}
	os = FS_BuildOSPath( Cvar_VariableString( "fs_homepath" ), FS_GetCurrentGameDir(), qpath );
	if ( !Sys_OpenPath( os ) ) {
		CW_Message( va( S_COLOR_YELLOW "Couldn't open %s", os ) );
		Com_Printf( "Chat log: couldn't open %s\n", os );
	}
}

static void CL_OpenToday( void ) {
	char base[MAX_QPATH];

	if ( lg.recording )
		Q_strncpyz( base, lg.base, sizeof( base ) );
	else if ( cls.state == CA_ACTIVE )
		CL_TodaysBase( base, sizeof( base ) );
	else {
		CL_OpenPath( CL_DIR, qtrue );
		return;
	}
	CL_OpenPath( va( "%s.html", base ), qfalse );
}

static void CL_OpenFolder( void ) {
	// made if it isn't there yet, so there's something to open
	if ( !FS_FileExists( CL_DIR "/readme.txt" ) ) {
		fileHandle_t f = FS_FOpenFileWrite( CL_DIR "/readme.txt" );

		if ( f ) {
			const char *text = "OpenJK-rpmod's chat logs: a .txt and an .html (open it in a browser) for each day on each server.\r\n";

			FS_Write( text, strlen( text ), f );
			FS_FCloseFile( f );
		}
	}
	CL_OpenPath( CL_DIR, qtrue );
}

static void CW_ClickSound( void ) {
	if ( cw.clickSound )
		S_StartLocalSound( cw.clickSound, CHAN_LOCAL_SOUND );
}

static void CW_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static void CW_Box( float x, float y, float w, float h, const float *fill ) {
	CW_Fill( x, y, w, h, fill );
	CW_Fill( x, y, w, 1, cwBorder );
	CW_Fill( x, y + h - 1, w, 1, cwBorder );
	CW_Fill( x, y, 1, h, cwBorder );
	CW_Fill( x + w - 1, y, 1, h, cwBorder );
}

static void CW_Text( float x, float y, float w, const char *text, const float *color, int font, float scale, qboolean centred ) {
	char buf[MAX_STRING_CHARS];
	int len;

	Q_strncpyz( buf, text, sizeof( buf ) );
	len = strlen( buf );
	while ( len > 3 && re->Font_StrLenPixels( buf, font, scale ) > w ) {
		len--;
		buf[len - 3] = buf[len - 2] = buf[len - 1] = '.';
		buf[len] = '\0';
	}
	if ( centred )
		x += ( w - re->Font_StrLenPixels( buf, font, scale ) ) * 0.5f;
	re->Font_DrawString( (int)x, (int)y, buf, color, font | STYLE_DROPSHADOW, -1, scale );
}

static qboolean CW_InRect( float x, float y, float w, float h ) {
	return (qboolean)( cw.cursorX >= x && cw.cursorX < x + w && cw.cursorY >= y && cw.cursorY < y + h );
}

static qboolean CW_Clicked( float x, float y, float w, float h ) {
	if ( !cw.click || cw.clickX < x || cw.clickX >= x + w || cw.clickY < y || cw.clickY >= y + h )
		return qfalse;
	cw.click = qfalse;
	CW_ClickSound();
	return qtrue;
}

static qboolean CW_Button( float x, float y, float w, const char *label, qboolean on ) {
	CW_Box( x, y, w, CW_CTRL_H, on ? cwItemOn : CW_InRect( x, y, w, CW_CTRL_H ) ? cwItemHover : cwItem );
	CW_Text( x, y + 2, w, label, cwWhite, cw.font, CW_TEXT_SCALE, qtrue );
	return CW_Clicked( x, y, w, CW_CTRL_H );
}

// a setting's row: its name, and On or Off to click
static void CW_Toggle( float x, float y, float w, const char *label, cvar_t *cvar ) {
	CW_Text( x, y + 2, w - 70, label, cwOrange, cw.font, CW_TEXT_SCALE, qfalse );
	if ( CW_Button( x + w - 60, y, 60, cvar->integer ? "On" : "Off", (qboolean)( cvar->integer != 0 ) ) )
		Cvar_Set( cvar->name, cvar->integer ? "0" : "1" );
}

void CL_ChatLog_Draw( void ) {
	const float x = CW_X + 14, w = CW_W - 28;
	float y = CW_Y + 8;

	if ( !cw.open )
		return;
	CW_Box( CW_X, CW_Y, CW_W, CW_H, cwPanel );
	CW_Text( x, y, w, "CHAT LOG", cwOrange, cw.titleFont, 0.8f, qfalse );
	if ( CW_Button( CW_X + CW_W - 14 - 50, y + 2, 50, "Close", qfalse ) ) {
		CW_Close();
		return;
	}
	y += 30;

	CW_Text( x, y, w, lg.recording ? va( "Recording: %s", COM_SkipPath( lg.base ) ) : "Not recording", lg.recording ? cwGreen : cwDim,
		cw.font, CW_TEXT_SCALE, qfalse );
	y += 18;
	if ( CW_Button( x, y, 130, lg.recording ? "Stop recording" : "Start recording", lg.recording ) ) {
		if ( lg.recording ) {
			CL_Stop( "Stopped" );
			lg.stopped = qtrue;
			CW_Message( "Stopped: it stays off on this server until you start it" );
		} else if ( CL_Start() ) {
			CW_Message( S_COLOR_GREEN "Recording" );
		}
	}
	if ( CW_Button( x + 136, y, 76, "Open page", qfalse ) )
		CL_OpenToday();
	if ( CW_Button( x + 218, y, w - 218, "Open folder", qfalse ) )
		CL_OpenFolder();
	y += CW_CTRL_H + 12;

	CW_Toggle( x, y, w, "Start by itself when you join a server", cl_chatLog );
	y += CW_ROW_H + 6;
	CW_Text( x, y, w, "Kept besides normal chat:", cwDim, cw.font, CW_TEXT_SCALE, qfalse );
	y += 16;
	for ( int k = CK_TELL; k <= CK_ADMIN; k++ ) {
		CW_Toggle( x + 10, y, w - 10, ckTitles[k], cl_chatLogKind[k] );
		y += CW_ROW_H;
	}

	if ( cw.message[0] && cls.realtime - cw.messageTime < CL_MESSAGE_MS )
		CW_Text( x, CW_Y + CW_H - 20, w, cw.message, cwWhite, cw.font, CW_TEXT_SCALE, qfalse );
	else
		CW_Text( x, CW_Y + CW_H - 20, w, "A .txt and an .html for each day on each server, in " CL_DIR "/", cwDim, cw.font, CW_TEXT_SCALE, qfalse );
	cw.click = qfalse;
	re->SetColor( NULL );
	re->DrawStretchPic( cw.cursorX, cw.cursorY, 32, 32, 0, 0, 1, 1, cls.cursorShader );
}

void CL_ChatLog_KeyEvent( int key, qboolean down ) {
	const char *binding;

	if ( !cw.open || !down )
		return;
	if ( key == A_MOUSE1 ) {
		cw.click = qtrue;
		cw.clickX = cw.cursorX;
		cw.clickY = cw.cursorY;
		return;
	}
	binding = Key_GetBinding( key );
	if ( VALIDSTRING( binding ) && !Q_stricmp( binding, CL_CMD ) )
		CW_Close();
}

void CL_ChatLog_MouseEvent( int dx, int dy ) {
	cw.cursorX = Com_Clamp( 0, SCREEN_WIDTH, cw.cursorX + dx );
	cw.cursorY = Com_Clamp( 0, SCREEN_HEIGHT, cw.cursorY + dy );
}

void CL_ChatLog_Escape( void ) {
	CW_Close();
}

/*
===============================================================================

COMMANDS

===============================================================================
*/

static void CL_ChatLog_f( void ) {
	const char *arg = Cmd_Argv( 1 );

	if ( !arg[0] ) {
		if ( cw.open )
			CW_Close();
		else
			CW_Open();
	} else if ( !Q_stricmp( arg, "start" ) ) {
		if ( lg.recording )
			Com_Printf( "Chat log: already recording to %s\n", lg.base );
		else if ( !CL_Start() )
			Com_Printf( "Chat log: join a server first\n" );
	} else if ( !Q_stricmp( arg, "stop" ) ) {
		if ( !lg.recording ) {
			Com_Printf( "Chat log: not recording\n" );
			return;
		}
		CL_Stop( "Stopped" );
		lg.stopped = qtrue;
		Com_Printf( "Chat log: stopped\n" );
	} else if ( !Q_stricmp( arg, "open" ) ) {
		CL_OpenToday();
	} else if ( !Q_stricmp( arg, "folder" ) ) {
		CL_OpenFolder();
	} else if ( !Q_stricmp( arg, "close" ) ) {
		CW_Close();
	} else {
		Com_Printf( "usage: " CL_CMD "          opens or closes its window\n" );
		Com_Printf( "       " CL_CMD " start    records the chat on this server\n" );
		Com_Printf( "       " CL_CMD " stop     stops (it stays off here until started)\n" );
		Com_Printf( "       " CL_CMD " open     today's page in the browser\n" );
		Com_Printf( "       " CL_CMD " folder   the folder of the logs\n" );
		Com_Printf( "%s\n", lg.recording ? va( "Recording to %s", lg.base ) : "Not recording" );
	}
}

void CL_ChatLog_Init( void ) {
	static const char *names[CK_NUM] = { NULL, "cl_chatLogTells", "cl_chatLogTeam", "cl_chatLogWhispers", "cl_chatLogOOC",
		"cl_chatLogEmotes", "cl_chatLogAdmin", NULL };

	memset( &lg, 0, sizeof( lg ) );
	memset( &cw, 0, sizeof( cw ) );
	cl_chatLog = Cvar_Get( "cl_chatLog", "0", CVAR_ARCHIVE_ND, "Records the chat in chatlogs/ by itself when you join a server" );
	for ( int k = 0; k < CK_NUM; k++ ) {
		cl_chatLogKind[k] = names[k] ? Cvar_Get( names[k], "1", CVAR_ARCHIVE_ND, va( "Chat log: keeps %s", ckTitles[k] ) ) : NULL;
	}
	Cmd_AddCommand( CL_CMD, CL_ChatLog_f, "The chat kept in files as you play: a .txt and an .html page like the console" );
}
