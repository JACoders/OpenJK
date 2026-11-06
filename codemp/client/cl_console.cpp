/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

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

// console.c

#include "client.h"
#include "cl_cgameapi.h"
#include "qcommon/stringed_ingame.h"
#include "qcommon/game_version.h"
#include "qcommon/q_version.h"


int g_console_field_width = 78;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;
cvar_t		*con_opacity; // background alpha multiplier
cvar_t		*con_autoclear;
cvar_t		*con_height;
cvar_t		*con_scale;
cvar_t		*con_timestamps;

// easier to read (see /consolesettings)
cvar_t		*con_lineSpacing;	// room between the rows, a part of a letter's height
cvar_t		*con_textShadow;	// 0 none, 1 a shadow, 2 an outline
cvar_t		*con_bgDarken;		// the picture behind darkened, 0-1
cvar_t		*con_margin;		// letters kept clear at either side
cvar_t		*con_textColor;		// "r g b" 0-255 for plain text, "" white
cvar_t		*con_contrast;		// 0 as sent, 1 dark colours brightened, 2 every line in the text colour
cvar_t		*con_colorblind;	// 0 none, 1 red-green, 2 blue-yellow
cvar_t		*con_noFade;		// 1 nothing drawn faded, far chat neither
static cvar_t *con_modernInput;	// 1 the typing goes on to more lines, with a selection and word jumps
cvar_t		*con_font;			// 0 the classic letters, else one of conFonts
cvar_t		*con_notifyScale;	// the lines over the game: their letters' size, 0 as the console's

#define	DEFAULT_CONSOLE_WIDTH	78

#define CON_BLANK_CHAR			' '
#define CON_SCROLL_L_CHAR		'$'
#define CON_SCROLL_R_CHAR		'$'
#define CON_TIMESTAMP_LEN		11 // "[13:37:00] "
#define CON_MIN_WIDTH			20


// RPMod's chat colours: ^0-^9, ^A-^Z and ^a-^z, indexed by code - '0' like its cgame does
// (the codes between them are white). Its ^9 is purple, so the console's own grey has its own index
#define CON_NUM_COLORS			( 'z' - '0' + 1 )
#define CON_COLOR_GREY			CON_NUM_COLORS

static const vec4_t rpmodColors[CON_NUM_COLORS] = {
	{ 0.00f, 0.00f, 0.00f, 1 }, { 1.00f, 0.00f, 0.00f, 1 }, { 0.00f, 1.00f, 0.00f, 1 }, { 1.00f, 1.00f, 0.00f, 1 }, // 0-3
	{ 0.00f, 0.00f, 1.00f, 1 }, { 0.00f, 1.00f, 1.00f, 1 }, { 1.00f, 0.00f, 1.00f, 1 }, { 1.00f, 1.00f, 1.00f, 1 }, // 4-7
	{ 1.00f, 0.50f, 0.00f, 1 }, { 0.50f, 0.00f, 0.80f, 1 },                                                         // 8-9
	{ 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 },   // :-@
	{ 0.59f, 0.29f, 0.00f, 1 }, { 1.00f, 0.80f, 0.64f, 1 }, { 0.00f, 0.80f, 0.80f, 1 }, { 0.00f, 0.00f, 0.50f, 1 }, // A-D
	{ 0.31f, 0.15f, 0.51f, 1 }, { 1.00f, 0.75f, 0.80f, 1 }, { 0.00f, 0.48f, 0.65f, 1 }, { 0.00f, 0.29f, 0.33f, 1 }, // E-H
	{ 0.97f, 0.51f, 0.47f, 1 }, { 0.00f, 0.50f, 0.50f, 1 }, { 0.50f, 0.00f, 0.00f, 1 }, { 0.64f, 0.35f, 0.32f, 1 }, // I-L
	{ 0.25f, 0.51f, 0.43f, 1 }, { 0.50f, 0.00f, 1.00f, 1 }, { 0.75f, 0.75f, 0.75f, 1 }, { 0.21f, 0.27f, 0.31f, 1 }, // M-P
	{ 0.40f, 0.60f, 0.80f, 1 }, { 0.89f, 0.26f, 0.20f, 1 }, { 0.77f, 0.38f, 0.06f, 1 }, { 0.85f, 0.56f, 0.35f, 1 }, // Q-T
	{ 0.91f, 0.38f, 0.00f, 1 }, { 0.80f, 0.80f, 1.00f, 1 }, { 0.69f, 0.88f, 0.90f, 1 }, { 0.68f, 0.85f, 0.90f, 1 }, // U-X
	{ 0.54f, 0.81f, 0.94f, 1 }, { 0.00f, 0.00f, 1.00f, 1 },                                                         // Y-Z
	{ 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 },                   // [-`
	{ 0.25f, 0.00f, 1.00f, 1 }, { 0.00f, 0.00f, 0.55f, 1 }, { 0.11f, 0.16f, 0.32f, 1 }, { 1.00f, 1.00f, 0.80f, 1 }, // a-d
	{ 0.94f, 0.80f, 0.00f, 1 }, { 1.00f, 0.83f, 0.00f, 1 }, { 1.00f, 0.94f, 0.00f, 1 }, { 1.00f, 1.00f, 0.47f, 1 }, // e-h
	{ 0.98f, 0.85f, 0.37f, 1 }, { 1.00f, 0.84f, 0.00f, 1 }, { 0.76f, 0.69f, 0.57f, 1 }, { 0.50f, 0.50f, 0.00f, 1 }, // i-l
	{ 0.88f, 0.07f, 0.37f, 1 }, { 1.00f, 0.08f, 0.58f, 1 }, { 1.00f, 0.44f, 1.00f, 1 }, { 0.40f, 0.01f, 0.24f, 1 }, // m-p
	{ 0.47f, 0.32f, 0.66f, 1 }, { 0.44f, 0.16f, 0.39f, 1 }, { 0.47f, 0.09f, 0.29f, 1 }, { 0.56f, 0.59f, 0.47f, 1 }, // q-t
	{ 0.53f, 0.66f, 0.42f, 1 }, { 0.34f, 0.51f, 0.01f, 1 }, { 0.68f, 1.00f, 0.18f, 1 }, { 0.27f, 0.30f, 0.22f, 1 }, // u-x
	{ 0.00f, 0.34f, 0.25f, 1 }, { 0.54f, 0.60f, 0.36f, 1 },                                                         // y-z
};

// only on RPMod: anywhere else ^a stays text and ^9 stays grey, as in the base game
static qboolean Con_RPModColors( void ) {
	return (qboolean)( FS_Initialized() && !Q_stricmp( FS_GetCurrentGameDir(), "rpmod" ) );
}

static qboolean Con_IsColorString( const char *p, qboolean rpmod ) {
	if ( !rpmod )
		return (qboolean)Q_IsColorString( p );
	return (qboolean)( p[0] == Q_COLOR_ESCAPE && ( ( p[1] >= '0' && p[1] <= '9' )
		|| ( p[1] >= 'A' && p[1] <= 'Z' ) || ( p[1] >= 'a' && p[1] <= 'z' ) ) );
}

static const float *Con_Color( int index, qboolean rpmod ) {
	if ( index >= CON_NUM_COLORS )
		return g_color_table[ColorIndex(COLOR_GREY)];
	if ( index > 9 || rpmod )
		return rpmodColors[index];
	return g_color_table[index];
}

// the colour a code at p in typed text gives what follows, NULL when p isn't one: on RPMod its
// colours too (^a-^z, ^A-^Z and its ^8 ^9), as the console shows them
const float *Con_TypedColor( const char *p ) {
	const qboolean rpmod = Con_RPModColors();

	if ( !Con_IsColorString( p, rpmod ) )
		return NULL;
	return Con_Color( p[1] - '0', rpmod );
}

// the asian path hands its line to the renderer's font code, which only knows ^0-^9
static char Con_ColorCode( int index ) {
	if ( index >= CON_NUM_COLORS )
		return COLOR_GREY;
	return index > 9 ? COLOR_WHITE : (char)( '0' + index );
}

// plain text's colour: con_textColor's, white if it has none
static void Con_TextColor( vec4_t out ) {
	static int seen = -1;
	static vec4_t color = { 1, 1, 1, 1 };

	if ( con_textColor && con_textColor->modificationCount != seen ) {
		int r = 255, g = 255, b = 255;

		seen = con_textColor->modificationCount;
		if ( sscanf( con_textColor->string, "%i %i %i", &r, &g, &b ) != 3 )
			r = g = b = 255;
		VectorSet4( color, Com_Clampi( 0, 255, r ) / 255.0f, Com_Clampi( 0, 255, g ) / 255.0f, Com_Clampi( 0, 255, b ) / 255.0f, 1.0f );
	}
	VectorCopy4( color, out );
}

// a hue told apart by those who can't tell red from green, or blue from yellow
static void Con_Colorblind( float *c, int mode ) {
	const float mx = Q_max( c[0], Q_max( c[1], c[2] ) ), mn = Q_min( c[0], Q_min( c[1], c[2] ) ), d = mx - mn;
	float h, f, p, q, t;
	int i;

	if ( mx <= 0.0f || d / mx < 0.25f )
		return;		// greys stay
	if ( mx == c[0] )
		h = 60.0f * fmodf( ( c[1] - c[2] ) / d + 6.0f, 6.0f );
	else if ( mx == c[1] )
		h = 60.0f * ( ( c[2] - c[0] ) / d + 2.0f );
	else
		h = 60.0f * ( ( c[0] - c[1] ) / d + 4.0f );
	if ( mode == 1 ) {
		// reds go orange, greens go sky blue
		if ( h < 20.0f || h >= 330.0f )
			h = 28.0f;
		else if ( h >= 75.0f && h < 170.0f )
			h = 200.0f;
	} else {
		// blues go purple, yellows go orange
		if ( h >= 185.0f && h < 255.0f )
			h = 285.0f;
		else if ( h >= 45.0f && h < 75.0f )
			h = 25.0f;
	}
	// back from hue, keeping how bright and how strong it was
	h /= 60.0f;
	i = (int)h % 6;
	f = h - floorf( h );
	p = mx * ( 1.0f - d / mx );
	q = mx * ( 1.0f - f * d / mx );
	t = mx * ( 1.0f - ( 1.0f - f ) * d / mx );
	switch ( i ) {
	case 0:		VectorSet( c, mx, t, p );	break;
	case 1:		VectorSet( c, q, mx, p );	break;
	case 2:		VectorSet( c, p, mx, t );	break;
	case 3:		VectorSet( c, p, q, mx );	break;
	case 4:		VectorSet( c, t, p, mx );	break;
	default:	VectorSet( c, mx, p, q );	break;
	}
}

// a colour as it's to be read: plain text in its colour, the colour-blind's hues, dark ones lit
static void Con_Readable( int index, float *c ) {
	const int contrast = con_contrast ? con_contrast->integer : 0;
	const float alpha = c[3];

	if ( contrast == 2 || index == 7 ) {
		Con_TextColor( c );
		c[3] = alpha;
		if ( contrast == 2 )
			return;
	}
	if ( con_colorblind && con_colorblind->integer )
		Con_Colorblind( c, con_colorblind->integer );
	if ( contrast == 1 ) {
		const float lum = 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2], want = 0.6f;

		if ( lum < want ) {
			const float f = ( want - lum ) / ( 1.0f - lum );

			for ( int i = 0; i < 3; i++ )
				c[i] += ( 1.0f - c[i] ) * f;
		}
	}
}

// a colour as drawn: readable, and see-through with a fade (chat from far away)
static const float *Con_FadedColor( int index, int fade, qboolean rpmod ) {
	static vec4_t shown[8];
	static int next;
	float *c = shown[next++ & 7];

	VectorCopy4( Con_Color( index, rpmod ), c );
	Con_Readable( index, c );
	if ( fade && !( con_noFade && con_noFade->integer ) )
		c[3] *= ( 255 - fade ) / 255.0f;
	return c;
}

/*
==============================================================================

LETTERS

==============================================================================
*/

// the fonts the console can be drawn in, besides its own letters
static const char *conFonts[] = { "", "arialnb", "ergoec", "ocr_a", "anewhope" };
static qhandle_t conFontHandle[ARRAY_LEN( conFonts )];
static qboolean conFontTried[ARRAY_LEN( conFonts )];

// the renderer started again: the fonts are looked for anew
void Con_FontsLost( void ) {
	memset( conFontHandle, 0, sizeof( conFontHandle ) );
	memset( conFontTried, 0, sizeof( conFontTried ) );
}

static int Con_Font( void ) {
	const int i = con_font ? Com_Clampi( 0, ARRAY_LEN( conFonts ) - 1, con_font->integer ) : 0;

	if ( !i )
		return 0;
	if ( !conFontTried[i] ) {
		conFontTried[i] = qtrue;
		conFontHandle[i] = re->RegisterFont( conFonts[i] );
	}
	return conFontHandle[i];
}

const char *Con_FontName( int index ) {
	static const char *names[] = { "The console's own", "Arial", "Ergo", "OCR, typewriter-like", "A New Hope" };

	return names[Com_Clampi( 0, ARRAY_LEN( names ) - 1, index )];
}

int Con_NumFonts( void ) {
	return ARRAY_LEN( conFonts );
}

// rows apart, in screen pixels
static int Con_RowH( void ) {
	const float extra = con_lineSpacing ? Com_Clamp( 0.0f, 1.5f, con_lineSpacing->value ) : 0.0f;

	return con.charHeight + (int)( con.charHeight * extra + 0.5f );
}

// the letters kept clear at the left, in screen pixels
static int Con_Left( void ) {
	return ( con_margin ? Com_Clampi( 0, 30, con_margin->integer ) : 0 ) * con.charWidth;
}

// the size a font is drawn at in the console, a letter as tall as the console's own
static float Con_FontScale( int font ) {
	const float height = (float)re->Font_HeightPixels( font, 1.0f );

	return height > 0.0f ? con.charHeight * con.yadjust / height : 1.0f;
}

// how far into a row its first col letters reach, in the console's pixels. In a font each letter
// is as wide as it is, so they're measured all together, the rounding not adding up
static float Con_ColX( const conChar_t *text, int col ) {
	const int font = Con_Font();
	char buf[MAX_STRING_CHARS];
	int n;

	if ( col <= 0 )
		return 0.0f;
	if ( !font || con.xadjust <= 0.0f )
		return (float)( col * con.charWidth );
	n = Q_min( col, (int)sizeof( buf ) - 1 );
	for ( int i = 0; i < n; i++ )
		buf[i] = text[i].f.character ? text[i].f.character : ' ';
	buf[n] = '\0';
	return re->Font_StrLenPixels( buf, font, Con_FontScale( font ) ) / con.xadjust;
}

// a letter in its cell, the console's own or in the font picked
static void Con_GlyphRaw( int x, int y, int ch, const float *color ) {
	const int font = Con_Font();

	if ( !font ) {
		re->SetColor( color );
		SCR_DrawSmallChar( x, y, ch );
		return;
	}
	if ( ( ch & 255 ) == ' ' )
		return;
	{
		char text[2] = { (char)ch, '\0' };
		const float height = (float)re->Font_HeightPixels( font, 1.0f );
		const float scale = height > 0.0f ? con.charHeight * con.yadjust / height : 1.0f;
		const float w = (float)re->Font_StrLenPixels( text, font, scale );

		re->Font_DrawString( (int)( ( x + con.charWidth * 0.5f ) * con.xadjust - w * 0.5f ), (int)( y * con.yadjust ), text, color, font, -1, scale );
	}
}

// a letter, with a shadow or an outline behind it to stand out from what's behind
static void Con_Glyph( int x, int y, int ch, const float *color ) {
	const int mode = con_textShadow ? con_textShadow->integer : 0;

	if ( mode && ( ch & 255 ) != ' ' ) {
		const vec4_t dark = { 0.0f, 0.0f, 0.0f, color[3] * 0.9f };
		const int o = Q_max( 1, con.charWidth / 8 );

		if ( mode >= 2 )
			Con_GlyphRaw( x - o, y - o, ch, dark );
		Con_GlyphRaw( x + o, y + o, ch, dark );
	}
	Con_GlyphRaw( x, y, ch, color );
}

// a row of the console from x0: the shadows first, then the letters, the colour set only where
// it changes; in a font, a run of one colour as one string. As few draws as can be, a full
// console being thousands of letters
static void Con_DrawRow( int x0, int y, const conChar_t *text, int count, qboolean rpmod ) {
	const int mode = con_textShadow ? con_textShadow->integer : 0;
	const int o = Q_max( 1, con.charWidth / 8 );
	const int font = Con_Font();
	const vec4_t dark = { 0.0f, 0.0f, 0.0f, 0.9f };

	if ( font ) {
		const float scale = Con_FontScale( font );

		// a run goes where the letters before it end, not in the console's own letters' columns
		for ( int x = 0; x < count; ) {
			char run[MAX_STRING_CHARS];
			const int start = x;
			int n = 0;
			const float *color;
			float px;

			if ( text[x].f.character == ' ' ) {
				x++;
				continue;
			}
			while ( x < count && n < (int)sizeof( run ) - 1 && text[x].f.color == text[start].f.color && text[x].f.fade == text[start].f.fade )
				run[n++] = text[x++].f.character;
			while ( n && run[n - 1] == ' ' )
				n--;
			run[n] = '\0';
			color = Con_FadedColor( text[start].f.color, text[start].f.fade, rpmod );
			px = x0 + Con_ColX( text, start );
			if ( mode ) {
				vec4_t shade;

				VectorCopy4( dark, shade );
				shade[3] *= color[3];
				if ( mode >= 2 )
					re->Font_DrawString( (int)( ( px - o ) * con.xadjust ), (int)( ( y - o ) * con.yadjust ), run, shade, font, -1, scale );
				re->Font_DrawString( (int)( ( px + o ) * con.xadjust ), (int)( ( y + o ) * con.yadjust ), run, shade, font, -1, scale );
			}
			re->Font_DrawString( (int)( px * con.xadjust ), (int)( y * con.yadjust ), run, color, font, -1, scale );
		}
		return;
	}
	if ( mode ) {
		re->SetColor( dark );
		for ( int x = 0; x < count; x++ ) {
			if ( text[x].f.character == ' ' )
				continue;
			if ( mode >= 2 )
				SCR_DrawSmallChar( x0 + x * con.charWidth - o, y - o, text[x].f.character );
			SCR_DrawSmallChar( x0 + x * con.charWidth + o, y + o, text[x].f.character );
		}
	}
	for ( int x = 0, color = -1, fade = -1; x < count; x++ ) {
		if ( text[x].f.character == ' ' )
			continue;
		if ( text[x].f.color != color || text[x].f.fade != fade ) {
			color = text[x].f.color;
			fade = text[x].f.fade;
			re->SetColor( Con_FadedColor( color, fade, rpmod ) );
		}
		SCR_DrawSmallChar( x0 + x * con.charWidth, y, text[x].f.character );
	}
}

// the fade of a line, for the asian path that draws the line as one string
static int Con_LineFade( const conChar_t *text, int len ) {
	for ( int x = 0; x < len; x++ ) {
		if ( text[x].f.character != CON_BLANK_CHAR )
			return text[x].f.fade;
	}
	return 0;
}

// the text CL_ConsolePrint prints until this is set back to 0 is faded by this, 1-255
void Con_SetPrintFade( int fade ) {
	con.fade = (unsigned char)Com_Clampi( 0, 255, fade );
}

// set while /colorcodes prints: colour codes still colour the text but stay visible
static qboolean con_showColorCodes = qfalse;

static const conChar_t CON_WRAP = { { CON_COLOR_GREY, '\\' } };
static const conChar_t CON_BLANK = { { ColorIndex(COLOR_WHITE), CON_BLANK_CHAR } };

vec4_t	console_color = {0.509f, 0.609f, 0.847f, 1.0f};

static void Con_ForgetText( void );
static int conPrintSerial;		// up by one each print, so the matches know to look again

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	if( con_autoclear->integer )
		Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_CONSOLE );
	Con_MouseReset();
}

/*
===================
Con_ToggleMenu_f
===================
*/
void Con_ToggleMenu_f( void ) {
	CL_KeyEvent( A_ESCAPE, qtrue, Sys_Milliseconds() );
	CL_KeyEvent( A_ESCAPE, qfalse, Sys_Milliseconds() );
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {	//yell
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {	//team chat
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void)
{		//target chat
	if (!cls.cgameStarted)
	{
		assert(!"null cgvm");
		return;
	}

	chat_playerNum = CGVM_CrosshairPlayer();
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS*2 ) {	// RPMod: allow up to 64 clients
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void)
{	//attacker
	if (!cls.cgameStarted)
	{
		assert(!"null cgvm");
		return;
	}

	chat_playerNum = CGVM_LastAttacker();
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS*2 ) {	// RPMod: allow up to 64 clients
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_MESSAGE );
}

/*
================
Con_Clear_f
================
*/
/*
================
Con_ColorCodes_f

Prints your name (or the given text) with its colour codes shown, each in its colour
================
*/
static void Con_ColorCodes_f( void ) {
	const char *text = Cmd_Argc() > 1 ? Cmd_ArgsFrom( 1 ) : Cvar_VariableString( "name" );

	con_showColorCodes = qtrue;
	Com_Printf( "%s\n", text );
	con_showColorCodes = qfalse;
}

void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = CON_BLANK;
	}
	Con_ForgetText();

	Con_Bottom();		// go to end
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	char			filename[MAX_QPATH];
	qboolean		empty;
	int				l, i, j;
	int				line;
	int				lineLen;
	fileHandle_t	f;
#ifdef WIN32
	char			buffer[CON_TIMESTAMP_LEN + MAXPRINTMSG + 2];
#else
	char			buffer[CON_TIMESTAMP_LEN + MAXPRINTMSG + 1];
#endif

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("%s\n", SE_GetString("CON_TEXT_DUMP_USAGE"));
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	if(!COM_CompareExtension(filename, ".txt"))
	{
		Com_Printf( "Con_Dump_f: Only the \".txt\" extension is supported by this command!\n" );
		return;
	}

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", filename );

	// skip empty lines
	for (l = 1, empty = qtrue ; l < con.totallines && empty ; l++)
	{
		line = ((con.current + l) % con.totallines) * con.rowwidth;

		for (j = CON_TIMESTAMP_LEN ; j < con.rowwidth - 1 ; j++)
			if (con.text[line + j].f.character != CON_BLANK_CHAR)
				empty = qfalse;
	}

	for ( ; l < con.totallines ; l++)
	{
		lineLen = 0;
		i = 0;

		// Print timestamp
		if (con_timestamps->integer) {
			line = ((con.current + l) % con.totallines) * con.rowwidth;

			for (i = 0; i < CON_TIMESTAMP_LEN; i++)
				buffer[i] = con.text[line + i].f.character;

			lineLen = CON_TIMESTAMP_LEN;
		}

		// Concatenate wrapped lines
		for ( ; l < con.totallines ; l++)
		{
			line = ((con.current + l) % con.totallines) * con.rowwidth;

			for (j = CON_TIMESTAMP_LEN; j < con.rowwidth - 1 && i < (int)sizeof(buffer) - 1; j++, i++) {
				buffer[i] = con.text[line + j].f.character;

				if (con.text[line + j].f.character != CON_BLANK_CHAR)
					lineLen = i + 1;
			}

			if (i == sizeof(buffer) - 1)
				break;

			if (con.text[line + j].compare != CON_WRAP.compare)
				break;
		}

#ifdef WIN32 // I really don't like this inconsistency, but OpenJK has been doing this since April 2013
		buffer[lineLen] = '\r';
		buffer[lineLen+1] = '\n';
		FS_Write(buffer, lineLen + 2, f);
#else
		buffer[lineLen] = '\n';
		FS_Write(buffer, lineLen + 1, f);
#endif
	}

	FS_FCloseFile( f );
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;

	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

/*
================
Con_Initialize

Initialize console for the first time.
================
*/
void Con_Initialize(void)
{
	int	i;

	VectorCopy4(colorWhite, con.color);
	con.charWidth = SMALLCHAR_WIDTH;
	con.charHeight = SMALLCHAR_HEIGHT;
	con.linewidth = DEFAULT_CONSOLE_WIDTH;
	con.rowwidth = CON_TIMESTAMP_LEN + con.linewidth + 1;
	con.totallines = CON_TEXTSIZE / con.rowwidth;
	con.current = con.totallines - 1;
	con.display = con.current;
	con.xadjust = 1.0f;
	con.yadjust = 1.0f;
	for(i=0; i<CON_TEXTSIZE; i++)
	{
		con.text[i] = CON_BLANK;
	}

	con.initialized = qtrue;
}



/*
================
Con_Resize

Reformat the buffer for new row width
================
*/
static void Con_Resize(int rowwidth)
{
	static conChar_t tbuf[CON_TEXTSIZE];
	int		i, j;
	int		oldrowwidth;
	int		oldtotallines;

	oldrowwidth = con.rowwidth;
	oldtotallines = con.totallines;

	con.rowwidth = rowwidth;
	con.totallines = CON_TEXTSIZE / rowwidth;

	Com_Memcpy (tbuf, con.text, sizeof(tbuf));
	for(i=0; i<CON_TEXTSIZE; i++)
		con.text[i] = CON_BLANK;

	int oi = 0;
	int ni = 0;

	while (oi < oldtotallines)
		{
			conChar_t	line[MAXPRINTMSG];
			conChar_t	timestamp[CON_TIMESTAMP_LEN];
			int		lineLen = 0;
			int		oldline = ((con.current + oi) % oldtotallines) * oldrowwidth;
			int		newline = (ni % con.totallines) * con.rowwidth;

			// Store timestamp
			for (i = 0; i < CON_TIMESTAMP_LEN; i++)
				timestamp[i] = tbuf[oldline + i];

			// Store whole line concatenating on CON_WRAP
			for (i = 0; oi < oldtotallines; oi++)
				{
					oldline = ((con.current + oi) % oldtotallines) * oldrowwidth;

					for (j = CON_TIMESTAMP_LEN; j < oldrowwidth - 1 && i < (int)ARRAY_LEN(line); j++, i++) {
						line[i] = tbuf[oldline + j];

						if (line[i].f.character != CON_BLANK_CHAR)
							lineLen = i + 1;
					}

					if (i == ARRAY_LEN(line))
						break;

					if (tbuf[oldline + j].compare != CON_WRAP.compare)
						break;
				}

			oi++;

			// Print stored line to a new text buffer
			for (i = 0; ; ni++) {
				newline = (ni % con.totallines) * con.rowwidth;

				// Print timestamp at the begining of each line
				for (j = 0; j < CON_TIMESTAMP_LEN; j++)
					con.text[newline + j] = timestamp[j];

				for (j = CON_TIMESTAMP_LEN; j < con.rowwidth - 1 && i < lineLen; j++, i++)
					con.text[newline + j] = line[i];

				if (i == lineLen) {
					// Erase remaining chars in case newline wrapped
					for (; j < con.rowwidth - 1; j++)
						con.text[newline + j] = CON_BLANK;

					ni++;
					break;
				}

				con.text[newline + j] = CON_WRAP;
			}
		}

	con.current = ni;

	// Erase con.current line for next CL_ConsolePrint
	int newline = (con.current % con.totallines) * con.rowwidth;
	for (j = 0; j < con.rowwidth; j++)
		con.text[newline + j] = CON_BLANK;

	Con_ClearNotify ();
	Con_ForgetText();

	con.display = con.current;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void WIN_GetWindowSize( int *width, int *height );

// OpenJK-rpmod: the console's letters are in the renderer's pixels, which with supersampling are
// half the window's (twice as many) and with a render resolution of its own whatever that makes
// them; scaled by this they're the same size on the screen either way
static float Con_PixelScale( void )
{
	int w, h;
	float s;

	WIN_GetWindowSize( &w, &h );
	if ( w <= 0 || cls.glconfig.vidWidth <= 0 )
		return 1.0f;
	s = (float)cls.glconfig.vidWidth / w;
	return fabsf( s - 1.0f ) < 0.02f ? 1.0f : Com_Clamp( 0.25f, 4.0f, s );
}

void Con_CheckResize (void)
{
	int		charWidth, rowwidth, width;
	float	scale;

	assert(SMALLCHAR_HEIGHT >= SMALLCHAR_WIDTH);

	scale = ((con_scale->value > 0.0f) ? con_scale->value : 1.0f) * Con_PixelScale();
	charWidth = scale * SMALLCHAR_WIDTH;

	if (charWidth < 1) {
		charWidth = 1;
		scale = (float)charWidth / SMALLCHAR_WIDTH;
	}

	width = (cls.glconfig.vidWidth / charWidth) - 2;
	// kept clear at either side
	if ( con_margin )
		width -= 2 * Com_Clampi( 0, 30, con_margin->integer );

	if (width < 20) {
		width = 20;
		charWidth = cls.glconfig.vidWidth / 22;
		scale = (float)charWidth / SMALLCHAR_WIDTH;
	}

	if (charWidth < 1) {
		Com_Error(ERR_FATAL, "Con_CheckResize: Window too small to draw a console");
	}

	rowwidth = width + 1 + (con_timestamps->integer ? 0 : CON_TIMESTAMP_LEN);

	con.charWidth = charWidth;
	con.charHeight = scale * SMALLCHAR_HEIGHT;
	con.linewidth = width;
	con.xadjust = ((float)SCREEN_WIDTH) / cls.glconfig.vidWidth;
	con.yadjust = ((float)SCREEN_HEIGHT) / cls.glconfig.vidHeight;
	g_consoleField.widthInChars = width - 1; // Command prompt

	if (con.rowwidth != rowwidth)
	{
		Con_Resize(rowwidth);
	}
}


/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName( char *args, int argNum ) {
	if ( argNum == 2 )
		Field_CompleteFilename( "", "txt", qfalse, qtrue );
}

/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0, "How many seconds notify messages should be shown before they fade away");
	con_conspeed = Cvar_Get ("scr_conspeed", "3", 0, "Console open/close speed");
	Cvar_CheckRange (con_conspeed, 1.0f, 100.0f, qfalse);

	con_opacity = Cvar_Get ("con_opacity", "1.0", CVAR_ARCHIVE_ND, "Opacity of console background");
	con_autoclear = Cvar_Get ("con_autoclear", "1", CVAR_ARCHIVE_ND, "Automatically clear console input on close");
	con_height = Cvar_Get ("con_height", "0.5", CVAR_ARCHIVE_ND);

	con_scale = Cvar_Get ("con_scale", "1", CVAR_ARCHIVE_ND, "Scale console font");
	con_timestamps = Cvar_Get ("con_timestamps", "0", CVAR_ARCHIVE_ND, "Display timestamps infront of console lines");
	con_lineSpacing = Cvar_Get( "con_lineSpacing", "0", CVAR_ARCHIVE_ND, "Room between the console's rows, a part of a letter's height" );
	con_textShadow = Cvar_Get( "con_textShadow", "0", CVAR_ARCHIVE_ND, "Behind the console's letters: 0 nothing, 1 a shadow, 2 an outline" );
	con_bgDarken = Cvar_Get( "con_bgDarken", "0", CVAR_ARCHIVE_ND, "How much darker the picture behind the console is, 0 to 1" );
	con_margin = Cvar_Get( "con_margin", "0", CVAR_ARCHIVE_ND, "Letters kept clear at either side of the console" );
	con_textColor = Cvar_Get( "con_textColor", "", CVAR_ARCHIVE_ND, "Colour of the console's plain text, \"r g b\" 0-255, empty for white" );
	con_contrast = Cvar_Get( "con_contrast", "0", CVAR_ARCHIVE_ND, "0 colours as sent, 1 dark colours brightened, 2 every line in the text colour" );
	con_colorblind = Cvar_Get( "con_colorblind", "0", CVAR_ARCHIVE_ND, "Colours for the colour-blind: 0 none, 1 red-green, 2 blue-yellow" );
	con_modernInput = Cvar_Get( "con_modernInput", "0", CVAR_ARCHIVE_ND, "1: the console's typing goes on to more lines, Ctrl+arrows jump words, Shift or the mouse select, Ctrl+A/C/X/V; the chat box too" );
	con_noFade = Cvar_Get( "con_noFade", "0", CVAR_ARCHIVE_ND, "1 draws nothing in the console faded, chat from far away neither" );
	con_font = Cvar_Get( "con_font", "0", CVAR_ARCHIVE_ND, "The console's letters: 0 its own, 1 Arial, 2 Ergo, 3 OCR, 4 A New Hope" );
	con_notifyScale = Cvar_Get( "con_notifyScale", "0", CVAR_ARCHIVE_ND, "Size of the lines over the game, 0 as the console's letters" );

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f, "Show/hide console" );
	Cmd_AddCommand( "togglemenu", Con_ToggleMenu_f, "Show/hide the menu" );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f, "Global Chat" );
	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f, "Team Chat" );
	Cmd_AddCommand( "messagemode3", Con_MessageMode3_f, "Private Chat with Target Player" );
	Cmd_AddCommand( "messagemode4", Con_MessageMode4_f, "Private Chat with Last Attacker" );
	Cmd_AddCommand( "clear", Con_Clear_f, "Clear console text" );
	Cmd_AddCommand( "condump", Con_Dump_f, "Dump console text to file" );
	Cmd_AddCommand( "colorcodes", Con_ColorCodes_f, "Print your name, or the given text, with its colour codes shown" );
	Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );

	//Initialize values on first print
	con.initialized = qfalse;
}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown(void)
{
	Cmd_RemoveCommand("toggleconsole");
	Cmd_RemoveCommand("togglemenu");
	Cmd_RemoveCommand("messagemode");
	Cmd_RemoveCommand("messagemode2");
	Cmd_RemoveCommand("messagemode3");
	Cmd_RemoveCommand("messagemode4");
	Cmd_RemoveCommand("clear");
	Cmd_RemoveCommand("condump");
	Cmd_RemoveCommand("colorcodes");
}

/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed (qboolean skipnotify)
{
	int		i;
	int		line = (con.current % con.totallines) * con.rowwidth;

	// print timestamp on the PREVIOUS line
	{
		time_t t = time( NULL );
		struct tm *tms = localtime( &t );
		char	timestamp[CON_TIMESTAMP_LEN + 1];
		const unsigned char color = CON_COLOR_GREY;

		Com_sprintf(timestamp, sizeof(timestamp), "[%02d:%02d:%02d] ",
			tms->tm_hour, tms->tm_min, tms->tm_sec);

		for ( i = 0; i < CON_TIMESTAMP_LEN; i++ ) {
			con.text[line + i].f = { color, timestamp[i], con.fade };
		}
	}

	// mark time for transparent overlay
	if (con.current >= 0)
		con.times[con.current % NUM_CON_TIMES] = skipnotify ? 0 : cls.realtime;

	con.x = 0;

	if (con.display == con.current)
		con.display++;
	con.current++;

	line = (con.current % con.totallines) * con.rowwidth;

	for ( i = 0; i < con.rowwidth; i++ )
		con.text[line + i] = CON_BLANK;
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( const char *txt) {
	int		y;
	char			c;
	unsigned char	color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	if ( txt[0] == '*' ) {
		skipnotify = qtrue;
		txt += 1;
	}

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	if (!con.initialized) {
		Con_Initialize();
	}
	conPrintSerial++;

	color = ColorIndex(COLOR_WHITE);
	const qboolean rpmod = Con_RPModColors();

	while ( (c = (unsigned char) *txt) != 0 ) {
		if ( Con_IsColorString( txt, rpmod ) ) {
			color = *(txt+1) - '0';
			if ( !con_showColorCodes ) {
				txt += 2;
				continue;
			}
		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;

			if (con.x == con.rowwidth - CON_TIMESTAMP_LEN - 1) {
				con.text[y * con.rowwidth + CON_TIMESTAMP_LEN + con.x] = CON_WRAP;
				Con_Linefeed( skipnotify );
				y = con.current % con.totallines;
			}

			con.text[y * con.rowwidth + CON_TIMESTAMP_LEN + con.x].f = { color, c, con.fade };
			con.x++;
			break;
		}
	}


	// mark time for transparent overlay

	if (con.current >= 0 )
	{
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}



/*
==============================================================================

MOUSE: SELECTING, COPYING AND FINDING

The mouse drags over the console's text to select it, Ctrl+C or a right-click copies it (a
right-click with nothing selected pastes), a double-click takes a word. Ctrl+F, with the mouse
over the console, opens a find box at its bottom right: what's typed in it is lit up wherever
it is, and Enter, Up and Down (or the box's arrows) scroll from one to the next.

In a window the mouse is the system's while the console is down, so it's read where it is; full
screen it's the game's, and a cursor is drawn for it.

==============================================================================
*/

#define CON_MAX_MATCHES		4096
#define CON_FIND_LEN		64
#define CON_FIND_SHOWN		22		// characters of the find text shown
#define CON_DOUBLECLICK_MS	400
#define CON_DRAG_SCROLL_MS	40		// dragging past the top or bottom scrolls a line this often
#define CON_COPIED_MS		1500

// a place between two characters: before column caret of a row, the timestamp's columns counted
typedef struct {
	int		row, caret;
} conPos_t;

static struct {
	int			x, y;			// the mouse, in screen pixels
	qboolean	soft;			// the game has the mouse (full screen): a cursor is drawn for it
	qboolean	inside;			// over the window

	// the text as drawn last frame
	qboolean	drawn;
	int			bottomRow, rows, bottomY, firstCol;
	int			inputY;			// the input line, and below it

	// the selection, from anchor to end, either way round
	qboolean	selecting;		// the button is held
	qboolean	haveSel;
	conPos_t	anchor, end;
	int			clickTime, clickRow, clickCaret;
	int			scrollTime;
	int			copiedTime, copiedLen;

	// the find box
	qboolean	findOpen, findFocus;
	char		find[CON_FIND_LEN];
	qboolean	findDirty;		// the text changed: matches again, and the nearest shown
	int			findSerial;		// conPrintSerial the matches are of
	struct {
		int		row, col;
	}			matches[CON_MAX_MATCHES];
	int			numMatches, cur;	// cur -1 none
	float		boxX, boxY, boxW, boxH, btnW;
} cm;


/*
The input line goes on to more rows as it gets long, broken after a space where it can be, the
text above moving up for them; past CON_INPUT_ROWS it scrolls to the cursor. The mouse puts the
cursor in it and drags a selection, a double-click takes a word.
*/
#define CON_INPUT_ROWS		8

typedef struct {
	int		start[MAX_EDIT_LINE + 1];	// start[num] is the length
	int		num;
} conInputRows_t;

static struct {
	int			first;			// the first row shown
	int			shown;			// rows drawn last frame, 0 none
	int			topY;			// the first one's y
	qboolean	dragging;
	int			clickTime, clickAt;
} ci;

static const vec4_t conSelColor		= { 0.25f, 0.45f, 0.9f, 0.55f };
static const vec4_t conMatchColor	= { 0.9f, 0.75f, 0.1f, 0.35f };
static const vec4_t conCurColor		= { 1.0f, 0.5f, 0.0f, 0.8f };
static const vec4_t conBoxColor		= { 0.08f, 0.08f, 0.1f, 0.95f };
static const vec4_t conBoxBorder	= { 0.509f, 0.609f, 0.847f, 1.0f };
static const vec4_t conBtnHover		= { 0.3f, 0.3f, 0.35f, 1.0f };

int Con_ModernInput( void ) {
	return con_modernInput ? con_modernInput->integer : 0;
}

static int Con_InputCols( void ) {
	return Q_max( 8, g_consoleField.widthInChars - 1 );
}

static void Con_InputWrap( conInputRows_t *r ) {
	const char *b = g_consoleField.buffer;
	const int len = (int)strlen( b ), cols = Con_InputCols();
	int start = 0;

	r->num = 0;
	for ( ;; ) {
		int brk = -1;

		r->start[r->num++] = start;
		if ( len - start <= cols || r->num >= MAX_EDIT_LINE )
			break;
		for ( int i = start; i < start + cols; i++ ) {
			if ( b[i] == ' ' )
				brk = i + 1;
		}
		start = brk > start ? brk : start + cols;
	}
	r->start[r->num] = len;
}

// the row an index is on: where one row ends and the next begins is the next's
static int Con_InputRowOf( const conInputRows_t *r, int at ) {
	for ( int i = r->num - 1; i > 0; i-- ) {
		if ( at >= r->start[i] )
			return i;
	}
	return 0;
}

// the index under a point on the input rows
static int Con_InputIndexAt( int px, int py ) {
	conInputRows_t r;
	int row, col, a, e;

	Con_InputWrap( &r );
	row = ci.first + Com_Clampi( 0, Q_max( 0, ci.shown - 1 ), (int)floorf( (float)( py - ci.topY ) / con.charHeight ) );
	row = Com_Clampi( 0, r.num - 1, row );
	a = r.start[row];
	e = r.start[row + 1];
	// the space a row broke after: the cursor goes before it
	if ( row < r.num - 1 && e > a && g_consoleField.buffer[e - 1] == ' ' )
		e--;
	col = (int)floorf( (float)( px - Con_Left() ) / con.charWidth + 0.5f ) - 2;
	return a + Com_Clampi( 0, e - a, col );
}

static qboolean Con_MouseOwned( void ) {
	return (qboolean)( ( Key_GetCatcher() & KEYCATCH_CONSOLE ) && con.displayFrac > 0.0f && con.initialized );
}

static qboolean Con_MouseOnConsole( void ) {
	return (qboolean)( Con_MouseOwned() && cm.inside && cm.y < con.vislines );
}

// a rectangle in screen pixels
static void Con_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x * con.xadjust, y * con.yadjust, w * con.xadjust, h * con.yadjust, 0, 0, 0, 0, cls.whiteShader );
}

static void Con_String( float x, float y, const char *s, const float *color ) {
	re->SetColor( color );
	for ( ; *s; s++, x += con.charWidth )
		SCR_DrawSmallChar( (int)x, (int)y, *s );
}

static qboolean Con_MouseIn( float x, float y, float w, float h ) {
	return (qboolean)( cm.inside && cm.x >= x && cm.x < x + w && cm.y >= y && cm.y < y + h );
}

static qboolean Con_RowValid( int row ) {
	return (qboolean)( row >= 0 && row <= con.current && con.current - row < con.totallines );
}

static const conChar_t *Con_Row( int row ) {
	return con.text + ( row % con.totallines ) * con.rowwidth;
}

static qboolean Con_Blank( const conChar_t *c ) {
	return (qboolean)( c->f.character == ' ' || !c->f.character );
}

static qboolean Con_PosLess( const conPos_t *a, const conPos_t *b ) {
	return (qboolean)( a->row < b->row || ( a->row == b->row && a->caret < b->caret ) );
}

// the selection from its first place to its last
static void Con_SelOrder( conPos_t *a, conPos_t *b ) {
	if ( Con_PosLess( &cm.end, &cm.anchor ) ) {
		*a = cm.end;
		*b = cm.anchor;
	} else {
		*a = cm.anchor;
		*b = cm.end;
	}
}

// the place in the text under a point on screen, the nearest row when it's above or below them
static qboolean Con_HitPos( int px, int py, conPos_t *pos ) {
	const int h = Con_RowH();
	int k, caret;

	if ( !cm.drawn || cm.rows <= 0 || h <= 0 || con.charWidth <= 0 )
		return qfalse;
	k = (int)floorf( (float)( cm.bottomY + h - 1 - py ) / h );
	k = Com_Clampi( 0, cm.rows - 1, k );
	pos->row = cm.bottomRow - k;
	if ( !Con_RowValid( pos->row ) )
		return qfalse;
	// the characters are drawn from one character in, after the margin
	{
		const float at = (float)( px - Con_Left() - con.charWidth );
		const int last = cm.firstCol ? con.rowwidth - 1 - cm.firstCol : con.rowwidth - 1;

		if ( Con_Font() ) {
			// in a font, the letter boundary nearest
			const conChar_t *text = Con_Row( pos->row ) + cm.firstCol;
			float left = 0.0f;

			for ( caret = 0; caret < last; caret++ ) {
				const float right = Con_ColX( text, caret + 1 );

				if ( at < ( left + right ) * 0.5f )
					break;
				left = right;
			}
		} else {
			caret = (int)floorf( at / con.charWidth + 0.5f );
		}
		pos->caret = cm.firstCol + Com_Clampi( 0, last, caret );
	}
	return qtrue;
}

static void Con_ClearSelection( void ) {
	cm.haveSel = cm.selecting = qfalse;
}

// the text selected on the clipboard, colour codes and all left out; a line goes on to the next
// row when it was too long for one
static void Con_CopySelection( void ) {
	static char buf[CON_TEXTSIZE * 2];
	conPos_t a, b;
	int len = 0;

	if ( !cm.haveSel )
		return;
	Con_SelOrder( &a, &b );
	if ( !Con_RowValid( a.row ) )
		a.row = Q_max( 0, con.current - con.totallines + 1 ), a.caret = 0;
	for ( int row = a.row; row <= b.row && Con_RowValid( row ); row++ ) {
		const conChar_t *text = Con_Row( row );
		const int from = row == a.row ? a.caret : cm.firstCol;
		const int to = row == b.row ? b.caret : con.rowwidth - 1;	// the wrap mark left out
		const qboolean wrapped = (qboolean)( text[con.rowwidth - 1].compare == CON_WRAP.compare );
		const int lineStart = len;

		for ( int c = from; c < to && len < (int)sizeof( buf ) - 2; c++ )
			buf[len++] = Con_Blank( &text[c] ) ? ' ' : text[c].f.character;
		if ( !wrapped || row == b.row ) {
			while ( len > lineStart && buf[len - 1] == ' ' )
				len--;
		}
		if ( row != b.row && !wrapped && len < (int)sizeof( buf ) - 2 )
			buf[len++] = '\n';
	}
	buf[len] = '\0';
	if ( !len )
		return;
	Sys_SetClipboardData( buf );
	cm.copiedTime = cls.realtime;
	cm.copiedLen = len;
}

// a double-click: the word under it
static void Con_SelectWord( const conPos_t *at ) {
	const conChar_t *text = Con_Row( at->row );
	int c = Com_Clampi( cm.firstCol, con.rowwidth - 2, at->caret ), a, b;

	if ( Con_Blank( &text[c] ) && c > cm.firstCol && !Con_Blank( &text[c - 1] ) )
		c--;
	if ( Con_Blank( &text[c] ) )
		return;
	for ( a = c; a > CON_TIMESTAMP_LEN && !Con_Blank( &text[a - 1] ); a-- )
		;
	for ( b = c + 1; b < con.rowwidth - 1 && !Con_Blank( &text[b] ); b++ )
		;
	cm.anchor.row = cm.end.row = at->row;
	cm.anchor.caret = a;
	cm.end.caret = b;
	cm.haveSel = qtrue;
	cm.selecting = qfalse;
}

// the console scrolled so the row is in view, near the middle when it wasn't
static void Con_ShowRow( int row ) {
	const int rows = cm.drawn && cm.rows > 0 ? cm.rows : 10;
	const int bottom = cm.drawn ? cm.bottomRow : con.display;
	int display;

	// the bottom row is under the find box
	if ( row > bottom - rows && row < bottom )
		return;
	display = row + rows / 2 + ( con.x == 0 ? 1 : 0 );
	display = Q_min( display, con.current );
	display = Q_max( display, con.current - con.totallines + 1 );
	con.display = display;
}

// every place the find text is, oldest first; the one shown stays shown if it's still there
static void Con_FindMatches( void ) {
	char needle[CON_FIND_LEN];
	const int n = strlen( cm.find );
	int keepRow = -1, keepCol = -1;

	if ( cm.cur >= 0 && cm.cur < cm.numMatches ) {
		keepRow = cm.matches[cm.cur].row;
		keepCol = cm.matches[cm.cur].col;
	}
	cm.numMatches = 0;
	cm.cur = -1;
	cm.findSerial = conPrintSerial;
	if ( !n || !con.initialized )
		return;
	for ( int i = 0; i <= n; i++ )
		needle[i] = (char)tolower( (unsigned char)cm.find[i] );
	for ( int row = Q_max( 0, con.current - con.totallines + 1 ); row <= con.current && cm.numMatches < CON_MAX_MATCHES; row++ ) {
		const conChar_t *text = Con_Row( row );

		for ( int c = CON_TIMESTAMP_LEN; c + n <= con.rowwidth - 1 && cm.numMatches < CON_MAX_MATCHES; c++ ) {
			int i;

			for ( i = 0; i < n && tolower( (unsigned char)text[c + i].f.character ) == needle[i]; i++ )
				;
			if ( i < n )
				continue;
			if ( row == keepRow && c == keepCol )
				cm.cur = cm.numMatches;
			cm.matches[cm.numMatches].row = row;
			cm.matches[cm.numMatches].col = c;
			cm.numMatches++;
			c += n - 1;
		}
	}
}

// the match that comes next: newer down the console, older up it, round at the end
static void Con_FindStep( int dir ) {
	if ( cm.findSerial != conPrintSerial || cm.findDirty ) {
		cm.findDirty = qfalse;
		Con_FindMatches();
	}
	if ( !cm.numMatches )
		return;
	if ( cm.cur < 0 )
		cm.cur = dir > 0 ? 0 : cm.numMatches - 1;
	else
		cm.cur = ( cm.cur + dir + cm.numMatches ) % cm.numMatches;
	Con_ShowRow( cm.matches[cm.cur].row );
}

static void Con_FindChanged( void ) {
	cm.findDirty = qtrue;
}

static void Con_CloseFind( void ) {
	cm.findOpen = cm.findFocus = qfalse;
	cm.numMatches = 0;
	cm.cur = -1;
}

// the mouse moving, full screen; qtrue when the console takes it
qboolean Con_MouseMove( int dx, int dy ) {
	if ( !Con_MouseOwned() || !cm.soft )
		return qfalse;
	cm.x = Com_Clampi( 0, cls.glconfig.vidWidth - 1, cm.x + dx );
	cm.y = Com_Clampi( 0, cls.glconfig.vidHeight - 1, cm.y + dy );
	cm.inside = qtrue;
	return qtrue;
}

// a mouse button; qtrue when the console takes it, every one it uses while it's down
qboolean Con_MouseKey( int key, qboolean down, qboolean shift ) {
	conPos_t pos;

	if ( key != A_MOUSE1 && key != A_MOUSE2 )
		return qfalse;
	if ( !down ) {
		if ( key == A_MOUSE1 )
			ci.dragging = qfalse;
		if ( key == A_MOUSE1 && cm.selecting ) {
			cm.selecting = qfalse;
			cm.haveSel = (qboolean)( cm.anchor.row != cm.end.row || cm.anchor.caret != cm.end.caret );
		}
		return qfalse;
	}
	if ( !Con_MouseOwned() )
		return qfalse;
	if ( !Con_MouseOnConsole() )
		return qtrue;

	if ( key == A_MOUSE2 ) {
		int a, b;

		// copies what's selected, else pastes
		if ( Field_Selection( &g_consoleField, &a, &b ) ) {
			Field_CharEvent( &g_consoleField, 'c' - 'a' + 1 );
		} else if ( cm.haveSel ) {
			Con_CopySelection();
			Con_ClearSelection();
		} else if ( cm.findFocus ) {
			char *clip = Sys_GetClipboardData();

			if ( clip ) {
				for ( char *c = clip; *c && *c != '\n' && *c != '\r'; c++ ) {
					const int len = strlen( cm.find );

					if ( len < CON_FIND_LEN - 1 && (unsigned char)*c >= 32 ) {
						cm.find[len] = *c;
						cm.find[len + 1] = '\0';
					}
				}
				Z_Free( clip );
				Con_FindChanged();
			}
		} else {
			Field_Paste( &g_consoleField );
		}
		return qtrue;
	}

	// the find box's buttons, or the box taking the typing again
	if ( cm.findOpen && Con_MouseIn( cm.boxX, cm.boxY, cm.boxW, cm.boxH ) ) {
		const float bx = cm.boxX + cm.boxW - cm.btnW * 3 - 2;

		if ( Con_MouseIn( bx, cm.boxY, cm.btnW, cm.boxH ) )
			Con_FindStep( -1 );
		else if ( Con_MouseIn( bx + cm.btnW, cm.boxY, cm.btnW, cm.boxH ) )
			Con_FindStep( 1 );
		else if ( Con_MouseIn( bx + cm.btnW * 2, cm.boxY, cm.btnW, cm.boxH ) )
			Con_CloseFind();
		else
			cm.findFocus = qtrue;
		return qtrue;
	}
	cm.findFocus = qfalse;

	// the input line as it was: the cursor goes where it's clicked
	if ( cm.y >= cm.inputY && !Con_ModernInput() ) {
		const int len = strlen( g_consoleField.buffer );
		const int caret = (int)floorf( (float)( cm.x - Con_Left() ) / con.charWidth + 0.5f ) - 2;

		Con_ClearSelection();
		g_consoleField.cursor = Com_Clampi( 0, len, g_consoleField.scroll + caret );
		return qtrue;
	}

	// the input line: the cursor goes where it's clicked, a drag selects, a double-click a word
	if ( cm.y >= cm.inputY ) {
		const int at = Con_InputIndexAt( cm.x, cm.y );
		int a, b;

		Con_ClearSelection();
		if ( cls.realtime - ci.clickTime < CON_DOUBLECLICK_MS && at == ci.clickAt ) {
			const char *buf = g_consoleField.buffer;

			for ( a = at; a > 0 && buf[a - 1] != ' '; a-- )
				;
			for ( b = at; buf[b] && buf[b] != ' '; b++ )
				;
			g_consoleField.cursor = b;
			Field_Select( &g_consoleField, a );
			ci.clickTime = 0;
			ci.dragging = qfalse;
			return qtrue;
		}
		ci.clickTime = cls.realtime;
		ci.clickAt = at;
		// Shift carries the selection on from where it began
		if ( shift ) {
			if ( !Field_Selection( &g_consoleField, &a, &b ) )
				Field_Select( &g_consoleField, g_consoleField.cursor );
		} else {
			Field_Select( &g_consoleField, at );
		}
		g_consoleField.cursor = at;
		ci.dragging = qtrue;
		return qtrue;
	}
	Field_ClearSelection();

	if ( !Con_HitPos( cm.x, cm.y, &pos ) )
		return qtrue;
	// a double-click takes the word
	if ( cls.realtime - cm.clickTime < CON_DOUBLECLICK_MS && pos.row == cm.clickRow && abs( pos.caret - cm.clickCaret ) <= 1 ) {
		cm.clickTime = 0;
		Con_SelectWord( &pos );
		return qtrue;
	}
	cm.clickTime = cls.realtime;
	cm.clickRow = pos.row;
	cm.clickCaret = pos.caret;
	// Shift carries on the selection there is
	if ( !( shift && cm.haveSel ) )
		cm.anchor = pos;
	cm.end = pos;
	cm.selecting = qtrue;
	cm.haveSel = qfalse;
	return qtrue;
}

// a key down in the console, before the input line has it; qtrue when it's taken
qboolean Con_KeyEvent( int key, int lower, qboolean ctrl, qboolean shift ) {
	if ( ctrl && lower == 'f' ) {
		if ( !Con_MouseOnConsole() )
			return qfalse;
		if ( !cm.findOpen )
			cm.cur = -1;
		cm.findOpen = cm.findFocus = qtrue;
		Con_FindChanged();
		return qtrue;
	}
	if ( !cm.findOpen || !cm.findFocus )
		return qfalse;
	switch ( key ) {
	case A_ENTER:
	case A_KP_ENTER:
		Con_FindStep( shift ? -1 : 1 );
		return qtrue;
	case A_CURSOR_UP:
	case A_KP_8:
		Con_FindStep( -1 );
		return qtrue;
	case A_CURSOR_DOWN:
	case A_KP_2:
		Con_FindStep( 1 );
		return qtrue;
	case A_TAB:
		// back to the input line, the matches still lit
		cm.findFocus = qfalse;
		return qtrue;
	case A_DELETE:
		cm.find[0] = '\0';
		Con_FindChanged();
		return qtrue;
	case A_BACKSPACE:
	case A_CURSOR_LEFT:
	case A_CURSOR_RIGHT:
	case A_HOME:
	case A_END:
	case A_INSERT:
		return qtrue;
	default:
		return qfalse;
	}
}

// a character typed in the console; qtrue when it's taken
qboolean Con_CharEvent( int ch ) {
	int len;

	// Ctrl+C copies what's selected, and clears the input line only without
	int a, b;

	if ( ch == 'c' - 'a' + 1 && cm.haveSel && !Field_Selection( &g_consoleField, &a, &b ) ) {
		Con_CopySelection();
		return qtrue;
	}
	if ( !cm.findOpen || !cm.findFocus )
		return qfalse;
	len = strlen( cm.find );
	if ( ch == 'h' - 'a' + 1 ) {
		if ( len ) {
			cm.find[len - 1] = '\0';
			Con_FindChanged();
		}
	} else if ( ch == 'v' - 'a' + 1 ) {
		char *clip = Sys_GetClipboardData();

		if ( clip ) {
			for ( char *c = clip; *c && *c != '\n' && *c != '\r' && len < CON_FIND_LEN - 1; c++ ) {
				if ( (unsigned char)*c >= 32 )
					cm.find[len++] = *c;
			}
			cm.find[len] = '\0';
			Z_Free( clip );
			Con_FindChanged();
		}
	} else if ( ch >= 32 && ch != 127 && len < CON_FIND_LEN - 1 ) {
		cm.find[len] = (char)ch;
		cm.find[len + 1] = '\0';
		Con_FindChanged();
	}
	return qtrue;
}

// Escape closes the find box before the console
qboolean Con_FindEscape( void ) {
	if ( !cm.findOpen )
		return qfalse;
	Con_CloseFind();
	return qtrue;
}

// the mouse read each frame, and a selection being dragged follows it, scrolling past the edges
static void Con_UpdateMouse( void ) {
	int x, y;
	const int got = IN_MousePosition( &x, &y );

	cm.soft = (qboolean)( got == 0 );
	if ( got != 0 ) {
		cm.inside = (qboolean)( got > 0 );
		// held outside the window, the selection still follows
		if ( got > 0 || cm.selecting ) {
			cm.x = x;
			cm.y = y;
		}
	}
	if ( ci.dragging ) {
		if ( kg.keys[A_MOUSE1].down && Con_MouseOwned() )
			g_consoleField.cursor = Con_InputIndexAt( cm.x, cm.y );
		else
			ci.dragging = qfalse;
	}
	if ( !cm.selecting || !Con_MouseOwned() )
		return;
	if ( cls.realtime - cm.scrollTime >= CON_DRAG_SCROLL_MS && cm.drawn ) {
		const int top = cm.bottomY - ( cm.rows - 1 ) * Con_RowH();

		if ( cm.y < top && con.current - con.display < con.totallines - 1 ) {
			con.display--;
			cm.scrollTime = cls.realtime;
		} else if ( cm.y >= cm.bottomY + Con_RowH() && con.display < con.current ) {
			con.display++;
			cm.scrollTime = cls.realtime;
		}
	}
	Con_HitPos( cm.x, cm.y, &cm.end );
}

// the matches looked for again when the text or the console changed; as it's typed, the newest
// from the bottom of the view up is shown, else the newest
static void Con_FindRefresh( void ) {
	qboolean jump;

	if ( cm.findSerial == conPrintSerial && !cm.findDirty )
		return;
	jump = cm.findDirty;
	cm.findDirty = qfalse;
	Con_FindMatches();
	if ( !jump || !cm.numMatches )
		return;
	cm.cur = cm.numMatches - 1;
	for ( int i = cm.numMatches - 1; i >= 0; i-- ) {
		if ( cm.matches[i].row <= cm.bottomRow ) {
			cm.cur = i;
			break;
		}
	}
	Con_ShowRow( cm.matches[cm.cur].row );
}

// the selection and the matches lit up on a row of text, before its characters
static void Con_DrawRowMarks( int row, int y, int firstCol ) {
	const float left = (float)( Con_Left() + con.charWidth );
	const int n = strlen( cm.find );
	const conChar_t *text = Con_Row( row ) + firstCol;

	if ( cm.findOpen && n ) {
		for ( int i = 0; i < cm.numMatches; i++ ) {
			const int c = cm.matches[i].col - firstCol;
			float x1;

			if ( cm.matches[i].row != row || c < 0 )
				continue;
			x1 = Con_ColX( text, c );
			Con_Fill( left + x1, (float)y, Con_ColX( text, c + n ) - x1, (float)con.charHeight, i == cm.cur ? conCurColor : conMatchColor );
		}
	}
	if ( cm.haveSel || cm.selecting ) {
		conPos_t a, b;
		int from, to;

		Con_SelOrder( &a, &b );
		if ( row < a.row || row > b.row )
			return;
		from = row == a.row ? a.caret : firstCol;
		to = row == b.row ? b.caret : con.rowwidth - 1;
		from = Q_max( from, firstCol );
		if ( to > from ) {
			const float x1 = Con_ColX( text, from - firstCol );

			Con_Fill( left + x1, (float)y, Con_ColX( text, to - firstCol ) - x1, (float)con.charHeight, conSelColor );
		}
	}
}

// a triangle in a button, up or down
static void Con_Arrow( float x, float y, float w, float h, qboolean up ) {
	const int size = Q_max( 2, (int)( h / 3 ) );
	const float cx = floorf( x + w * 0.5f ), top = floorf( y + ( h - size ) * 0.5f );

	for ( int i = 0; i < size; i++ ) {
		const int half = up ? i : size - 1 - i;

		Con_Fill( cx - half, top + i, (float)( half * 2 + 1 ), 1, colorWhite );
	}
}

// the find box, bottom right over the last row, and "Copied" over it
static void Con_DrawFindBox( void ) {
	const float cw = (float)con.charWidth, ch = (float)con.charHeight;
	char count[16];
	const char *shown = cm.find;

	cm.btnW = cw * 2;
	cm.boxW = Q_min( cw * 46, (float)cls.glconfig.vidWidth - cw * 4 );
	cm.boxH = ch + 6;
	cm.boxX = cls.glconfig.vidWidth - cm.boxW - cw;
	cm.boxY = cm.inputY - cm.boxH - 2;

	Con_Fill( cm.boxX - 1, cm.boxY - 1, cm.boxW + 2, cm.boxH + 2, conBoxBorder );
	Con_Fill( cm.boxX, cm.boxY, cm.boxW, cm.boxH, conBoxColor );
	Con_String( cm.boxX + cw * 0.5f, cm.boxY + 3, "Find", g_color_table[ColorIndex( COLOR_GREY )] );
	while ( strlen( shown ) > CON_FIND_SHOWN - 1 )
		shown++;
	Con_Fill( cm.boxX + cw * 5, cm.boxY + 2, cw * ( CON_FIND_SHOWN + 1 ), cm.boxH - 4, cm.findFocus ? conBtnHover : conBoxColor );
	Con_String( cm.boxX + cw * 5.5f, cm.boxY + 3, shown, colorWhite );
	if ( cm.findFocus && ( ( cls.realtime >> 8 ) & 1 ) )
		Con_String( cm.boxX + cw * ( 5.5f + strlen( shown ) ), cm.boxY + 3, "_", colorWhite );
	if ( !cm.find[0] )
		Q_strncpyz( count, "", sizeof( count ) );
	else if ( !cm.numMatches )
		Q_strncpyz( count, "none", sizeof( count ) );
	else
		Com_sprintf( count, sizeof( count ), "%i/%i%s", cm.cur + 1, cm.numMatches, cm.numMatches >= CON_MAX_MATCHES ? "+" : "" );
	Con_String( cm.boxX + cm.boxW - cm.btnW * 3 - 4 - cw * strlen( count ), cm.boxY + 3, count,
		cm.find[0] && !cm.numMatches ? g_color_table[ColorIndex( COLOR_RED )] : colorWhite );

	// up, down, close
	for ( int i = 0; i < 3; i++ ) {
		const float bx = cm.boxX + cm.boxW - cm.btnW * 3 - 2 + cm.btnW * i;

		if ( Con_MouseIn( bx, cm.boxY, cm.btnW, cm.boxH ) )
			Con_Fill( bx, cm.boxY + 1, cm.btnW, cm.boxH - 2, conBtnHover );
		if ( i < 2 )
			Con_Arrow( bx, cm.boxY, cm.btnW, cm.boxH, (qboolean)( i == 0 ) );
		else
			Con_String( bx + ( cm.btnW - cw ) * 0.5f, cm.boxY + 3, "x", colorWhite );
	}
}

static void Con_DrawCopied( void ) {
	const char *text;
	float x, y;

	if ( !cm.copiedTime || cls.realtime - cm.copiedTime >= CON_COPIED_MS )
		return;
	text = va( "Copied %i characters", cm.copiedLen );
	x = cls.glconfig.vidWidth - ( strlen( text ) + 2 ) * con.charWidth;
	y = ( cm.findOpen ? cm.boxY : cm.inputY ) - con.charHeight - 6;
	Con_Fill( x - con.charWidth * 0.5f, y - 2, ( strlen( text ) + 1 ) * con.charWidth, con.charHeight + 4, conBoxColor );
	Con_String( x, y, text, g_color_table[ColorIndex( COLOR_GREEN )] );
}

// the cursor, full screen where the system's isn't drawn
static void Con_DrawCursor( void ) {
	if ( !cm.soft || !Con_MouseOwned() )
		return;
	re->SetColor( NULL );
	re->DrawStretchPic( cm.x * con.xadjust, cm.y * con.yadjust, 24, 24, 0, 0, 1, 1, cls.cursorShader );
}

/*
==============================================================================

DRAWING

==============================================================================
*/


// the input line as it was, on one line scrolled sideways
static void Con_DrawInputClassic( void ) {
	int		y;

	y = con.vislines - ( con.charHeight * (re->Language_IsAsian() ? 1.5 : 2) );

	re->SetColor( con.color );

	Field_Draw( &g_consoleField, Con_Left() + 2 * con.charWidth, y, qtrue, qtrue );

	SCR_DrawSmallChar( Con_Left() + con.charWidth, y, CONSOLE_PROMPT_CHAR );

	re->SetColor( g_color_table[ColorIndex(COLOR_GREY)] );

	if ( g_consoleField.scroll > 0 )
		SCR_DrawSmallChar( 0, y, CON_SCROLL_L_CHAR );

	int len = Q_PrintStrlen( g_consoleField.buffer );
	int pos = Q_PrintStrLenTo( g_consoleField.buffer, g_consoleField.scroll, NULL );
	if ( pos + g_consoleField.widthInChars < len )
		SCR_DrawSmallChar( cls.glconfig.vidWidth - con.charWidth, y, CON_SCROLL_R_CHAR );
}

/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	const vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
	const char *buf = g_consoleField.buffer;
	const int cw = con.charWidth, x0 = Con_Left() + 2 * cw;
	conInputRows_t r;
	int		y, row, shown, a, b;
	qboolean sel;

	ci.shown = 0;
	if ( cls.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}
	if ( !Con_ModernInput() ) {
		Con_DrawInputClassic();
		return;
	}

	g_consoleField.cursor = Com_Clampi( 0, (int)strlen( buf ), g_consoleField.cursor );
	Con_InputWrap( &r );
	row = Con_InputRowOf( &r, g_consoleField.cursor );
	shown = Q_min( r.num, CON_INPUT_ROWS );
	if ( row < ci.first )
		ci.first = row;
	else if ( row >= ci.first + shown )
		ci.first = row - shown + 1;
	ci.first = Com_Clampi( 0, r.num - shown, ci.first );

	y = con.vislines - ( con.charHeight * (re->Language_IsAsian() ? 1.5 : 2) );
	ci.topY = y - ( shown - 1 ) * con.charHeight;
	ci.shown = shown;
	sel = Field_Selection( &g_consoleField, &a, &b );

	// the colour the rows above the first shown leave off in
	const float *color = white;

	for ( int i = 0; i < r.start[ci.first]; i++ ) {
		const float *c = Con_TypedColor( buf + i );

		if ( c )
			color = c;
	}
	for ( int k = 0; k < shown; k++ ) {
		const int n = ci.first + k, from = r.start[n], to = r.start[n + 1];
		const int ry = ci.topY + k * con.charHeight;

		if ( sel && b > from && a < to ) {
			const int sa = Q_max( a, from ), sb = Q_min( b, to );
			Con_Fill( (float)( x0 + ( sa - from ) * cw ), (float)ry, (float)( ( sb - sa ) * cw ), (float)con.charHeight, conSelColor );
		}
		// the codes shown, in the colour they give
		for ( int i = from; i < to; i++ ) {
			const float *c = Con_TypedColor( buf + i );

			if ( c )
				color = c;
			re->SetColor( color );
			SCR_DrawSmallChar( x0 + ( i - from ) * cw, ry, buf[i] );
		}
	}

	re->SetColor( con.color );
	SCR_DrawSmallChar( Con_Left() + cw, ci.topY, CONSOLE_PROMPT_CHAR );

	// rows out of view above or below
	re->SetColor( g_color_table[ColorIndex(COLOR_GREY)] );
	if ( ci.first > 0 )
		SCR_DrawSmallChar( 0, ci.topY, CON_SCROLL_L_CHAR );
	if ( ci.first + shown < r.num )
		SCR_DrawSmallChar( cls.glconfig.vidWidth - cw, y, CON_SCROLL_R_CHAR );

	// the cursor, blinking
	if ( !( ( cls.realtime >> 8 ) & 1 ) && row >= ci.first && row < ci.first + shown ) {
		re->SetColor( white );
		SCR_DrawSmallChar( x0 + ( g_consoleField.cursor - r.start[row] ) * cw, ci.topY + ( row - ci.first ) * con.charHeight,
			kg.key_overstrikeMode ? 11 : 10 );
	}
}




/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	int		lineLimit = con.linewidth;
	conChar_t		*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;
	int		currentFade = 0;
	const char* chattext;

	const qboolean rpmod = Con_RPModColors();
	currentColor = 7;
	re->SetColor( Con_Color( currentColor, rpmod ) );

	int iFontIndex = cls.consoleFont;
	float fFontScale = 1.0f;
	int iPixelHeightToAdvance = 0;
	if (re->Language_IsAsian())
	{
		fFontScale = con.charWidth * 10.0f /
			re->Font_StrLenPixels("aaaaaaaaaa", iFontIndex, 1.0f);
		fFontScale *= con.yadjust;
		iPixelHeightToAdvance = 2+(1.3/con.yadjust) * re->Font_HeightPixels(iFontIndex, fFontScale);
	}

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.rowwidth;
		if (con_timestamps->integer == 0 || con_timestamps->integer == 2) {
			// don't show timestamps in the notify lines
			text += CON_TIMESTAMP_LEN;
			lineLimit -= CON_TIMESTAMP_LEN;
		}

		if (cl.snap.ps.pm_type != PM_INTERMISSION && Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
			continue;
		}
		if (!cl_conXOffset)
		{
			cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
		}


		if (!cl_conXOffset)
		{
			cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
		}

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re->Language_IsAsian())
		{
			// concat the text to be printed...
			//
			char sTemp[4096];	// ott
			sTemp[0] = '\0';
			for (x = 0 ; x < lineLimit ; x++)
			{
				if ( text[x].f.color != currentColor ) {
					currentColor = text[x].f.color;
					strcat(sTemp,va("^%c", Con_ColorCode( currentColor ) ));
				}
				strcat(sTemp,va("%c",text[x].f.character));
			}
			//
			// and print...
			//
			re->Font_DrawString(cl_conXOffset->integer + con.xadjust * (con.xadjust + con.charWidth), con.yadjust * v, sTemp,
				Con_FadedColor( currentColor, Con_LineFade( text, lineLimit ), rpmod ), iFontIndex, -1, fFontScale);

			v +=  iPixelHeightToAdvance;
		}
		else
		{
			// in letters of their own size, on to the next row where they'd run off the screen
			const int savedW = con.charWidth, savedH = con.charHeight;
			const float ns = con_notifyScale ? con_notifyScale->value : 0.0f;
			int col = 0, last = lineLimit;

			if ( ns > 0.0f ) {
				con.charWidth = Q_max( 1, (int)( SMALLCHAR_WIDTH * Com_Clamp( 0.5f, 4.0f, ns ) * Con_PixelScale() + 0.5f ) );
				con.charHeight = con.charWidth * SMALLCHAR_HEIGHT / SMALLCHAR_WIDTH;
			}
			while ( last > 0 && text[last - 1].f.character == ' ' )
				last--;
			for (x = 0 ; x < last ; x++, col++) {
				const int px = cl_conXOffset->integer + Con_Left() + ( col + 1 ) * con.charWidth;

				if ( px + con.charWidth * 2 > cls.glconfig.vidWidth && col ) {
					v += Con_RowH();
					col = 0;
				}
				if ( text[x].f.character == ' ' ) {
					continue;
				}
				currentColor = text[x].f.color;
				currentFade = text[x].f.fade;
				Con_Glyph( cl_conXOffset->integer + Con_Left() + ( col + 1 ) * con.charWidth, v, text[x].f.character, Con_FadedColor( currentColor, currentFade, rpmod ) );
			}

			v += Con_RowH();
			con.charWidth = savedW;
			con.charHeight = savedH;
		}
	}

	re->SetColor( NULL );

	if (Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( Key_GetCatcher( ) & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			chattext = SE_GetString("MP_SVGAME", "SAY_TEAM");
			SCR_DrawBigString (8, v, chattext, 1.0f, qfalse );
			skip = strlen(chattext)+1;
		}
		else
		{
			chattext = SE_GetString("MP_SVGAME", "SAY");
			SCR_DrawBigString (8, v, chattext, 1.0f, qfalse );
			skip = strlen(chattext)+1;
		}

		Field_BigDraw( &chatField, (int)( skip * BIGCHAR_WIDTH * cls.widthRatioCoef ), v, qtrue, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	conChar_t		*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	int				currentFade = 0;

	lines = (int) (cls.glconfig.vidHeight * frac);
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// draw the background
	y = (int) (frac * SCREEN_HEIGHT - 2);
	if ( y < 1 ) {
		y = 0;
	}
	else {
		// draw the background at full opacity only if fullscreen
		if (frac < 1.0f)
		{
			vec4_t con_color;
			MAKERGBA(con_color, 1.0f, 1.0f, 1.0f, Com_Clamp(0.0f, 1.0f, con_opacity->value));
			re->SetColor(con_color);
		}
		else
		{
			re->SetColor(NULL);
		}
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, (float) y, CL_ConSettings_Background() );
		// darker, for the letters to stand out
		if ( con_bgDarken && con_bgDarken->value > 0.0f ) {
			const vec4_t dark = { 0.0f, 0.0f, 0.0f, Com_Clamp( 0.0f, 1.0f, con_bgDarken->value ) };

			re->SetColor( dark );
			re->DrawStretchPic( 0, 0, SCREEN_WIDTH, (float)y, 0, 0, 0, 0, cls.whiteShader );
		}
	}

	// draw the bottom bar and version number

	re->SetColor( console_color );
	re->DrawStretchPic( 0, y, SCREEN_WIDTH, 2, 0, 0, 0, 0, cls.whiteShader );

	i = strlen( JK_VERSION );

	for (x=0 ; x<i ; x++) {
		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x + 1 ) * con.charWidth,
			(lines-(con.charHeight+con.charHeight/2)), JK_VERSION[x] );
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	// draw the text
	con.vislines = lines;
	const int inputExtra = ci.shown > 1 ? ( ci.shown - 1 ) * con.charHeight : 0;
	cm.inputY = lines - (int)( con.charHeight * ( re->Language_IsAsian() ? 1.5f : 2.0f ) ) - 1 - inputExtra;
	Con_UpdateMouse();
	rows = ( lines - con.charHeight * 3 - inputExtra ) / Con_RowH() + 1;		// rows of text to draw

	y = lines - (con.charHeight*3) - inputExtra;

	// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		re->SetColor( console_color );
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( Con_Left() + (x+1)*con.charWidth, y, '^' );
		y -= Con_RowH();
		rows--;
	}

	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	// where each row is, for the mouse
	cm.drawn = qtrue;
	cm.bottomRow = row;
	cm.bottomY = y;
	cm.rows = rows;
	cm.firstCol = con_timestamps->integer ? 0 : CON_TIMESTAMP_LEN;
	if ( cm.findOpen )
		Con_FindRefresh();

	const qboolean rpmod = Con_RPModColors();
	currentColor = 7;
	re->SetColor( Con_Color( currentColor, rpmod ) );

	int iFontIndex = cls.consoleFont;
	float fFontScale = 1.0f;
	int iPixelHeightToAdvance = Con_RowH();
	if (re->Language_IsAsian())
	{
		fFontScale = con.charWidth * 10.0f /
			re->Font_StrLenPixels("aaaaaaaaaa", iFontIndex, 1.0f);
		fFontScale *= con.yadjust;
		iPixelHeightToAdvance = 2+(1.3/con.yadjust) * re->Font_HeightPixels(iFontIndex, fFontScale);
	}

	for (i=0 ; i<rows ; i++, y -= iPixelHeightToAdvance, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;
		}

		text = con.text + (row % con.totallines)*con.rowwidth;
		if (!con_timestamps->integer)
			text += CON_TIMESTAMP_LEN;

		// the selection and what's found, under the characters
		if ( cm.haveSel || cm.selecting || cm.findOpen ) {
			Con_DrawRowMarks( row, y, cm.firstCol );
			re->SetColor( Con_FadedColor( currentColor, currentFade, rpmod ) );
		}

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re->Language_IsAsian())
		{
			// concat the text to be printed...
			//
			char sTemp[4096];	// ott
			sTemp[0] = '\0';
			for (x = 0 ; x < con.linewidth + 1 ; x++)
			{
				if ( text[x].f.color != currentColor ) {
					currentColor = text[x].f.color;
					strcat(sTemp,va("^%c", Con_ColorCode( currentColor ) ));
				}
				strcat(sTemp,va("%c",text[x].f.character));
			}
			//
			// and print...
			//
			re->Font_DrawString(con.xadjust*(con.xadjust + con.charWidth), con.yadjust * y, sTemp, Con_FadedColor( currentColor, Con_LineFade( text, con.linewidth + 1 ), rpmod ),
				iFontIndex, -1, fFontScale);
		}
		else
		{
			Con_DrawRow( Con_Left() + con.charWidth, y, text, con.linewidth + 1, rpmod );
			currentColor = 7;
		}
	}

	if ( cm.findOpen && Con_MouseOwned() )
		Con_DrawFindBox();
	Con_DrawCopied();
	Con_DrawCursor();
	re->SetColor( NULL );
}

// the text gone or moved: what was selected and found is let go
static void Con_ForgetText( void ) {
	Con_ClearSelection();
	cm.numMatches = 0;
	cm.cur = -1;
	conPrintSerial++;
}

// the console opening or closing: a drag ends, the find box closes with it, and full screen the
// cursor starts in the middle of the console
void Con_MouseReset( void ) {
	cm.selecting = qfalse;
	if ( !( Key_GetCatcher() & KEYCATCH_CONSOLE ) ) {
		Con_CloseFind();
		return;
	}
	cm.x = cls.glconfig.vidWidth / 2;
	cm.y = (int)( cls.glconfig.vidHeight * Com_Clamp( 0.1f, 1.0f, con_height->value ) * 0.5f );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			CL_ConSettings_Draw();
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
	// the console settings window, over it
	CL_ConSettings_Draw();
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	// decide on the destination height of the console
	if ( ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) || CL_ConSettings_Active() )
		con.finalFrac = con_height->value;
	else
		con.finalFrac = 0;				// none visible

	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
	Con_MouseReset();
}
