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

// cl_widgets.cpp -- what the tools' windows share: a scrollbar a list can be dragged by, and
// text fields with a cursor, a selection, the clipboard, and long text going on below.
// Everything is in the 640x480 screen the tools draw in.

#include "client.h"
#include "keys.h"
#include "cl_widgets.h"

/*
===============================================================================

SCROLLBAR

===============================================================================
*/

static const vec4_t wgTrack = { 0.0f, 0.0f, 0.0f, 0.35f };
static const vec4_t wgThumb = { 0.62f, 0.66f, 0.72f, 0.55f };
static const vec4_t wgThumbHot = { 0.80f, 0.84f, 0.90f, 0.85f };

static struct {
	const void	*id;			// the scroll being dragged, by its address
	float		grab;			// where on the handle it was taken
} wgDrag;

static void WG_Fill( float x, float y, float w, float h, const float *color ) {
	re->SetColor( color );
	re->DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re->SetColor( NULL );
}

static qboolean WG_MouseDown( void ) {
	return kg.keys[A_MOUSE1].down;
}

// the bar for a view of h out of content, at offset *scroll (0 to content - view); qtrue when shown
static qboolean WG_Bar( const void *id, float x, float y, float h, float content, float view, float *scroll,
	float cursorX, float cursorY, qboolean *click, float clickX, float clickY ) {
	const float range = content - view;
	float thumbH, thumbY;
	qboolean hot;

	if ( range <= 0.0f || h <= 4.0f ) {
		if ( wgDrag.id == id )
			wgDrag.id = NULL;
		return qfalse;
	}
	thumbH = Q_max( 12.0f, h * view / content );
	if ( thumbH > h )
		thumbH = h;
	*scroll = Com_Clamp( 0.0f, range, *scroll );
	thumbY = y + ( h - thumbH ) * ( *scroll / range );

	// taken: on the handle it's dragged from there, beside it the handle comes to it
	if ( click && *click && clickX >= x && clickX < x + WG_SCROLL_W && clickY >= y && clickY < y + h ) {
		*click = qfalse;
		wgDrag.id = id;
		wgDrag.grab = ( clickY >= thumbY && clickY < thumbY + thumbH ) ? clickY - thumbY : thumbH * 0.5f;
	}
	if ( wgDrag.id == id ) {
		if ( !WG_MouseDown() ) {
			wgDrag.id = NULL;
		} else {
			const float along = Com_Clamp( 0.0f, h - thumbH, cursorY - wgDrag.grab - y );

			*scroll = h - thumbH > 0.0f ? range * along / ( h - thumbH ) : 0.0f;
			thumbY = y + along;
		}
	}

	hot = (qboolean)( wgDrag.id == id || ( cursorX >= x && cursorX < x + WG_SCROLL_W && cursorY >= thumbY && cursorY < thumbY + thumbH ) );
	WG_Fill( x, y, WG_SCROLL_W, h, wgTrack );
	WG_Fill( x + 1, thumbY + 1, WG_SCROLL_W - 2, thumbH - 2, hot ? wgThumbHot : wgThumb );
	return qtrue;
}

qboolean WG_Scrollbar( float x, float y, float h, int count, int visible, int *scroll,
	float cursorX, float cursorY, qboolean *click, float clickX, float clickY ) {
	float s = (float)*scroll;
	qboolean shown;

	if ( visible < 1 )
		visible = 1;
	shown = WG_Bar( scroll, x, y, h, (float)count, (float)visible, &s, cursorX, cursorY, click, clickX, clickY );
	if ( shown && wgDrag.id == scroll )
		*scroll = Com_Clampi( 0, Q_max( 0, count - visible ), (int)( s + 0.5f ) );
	return shown;
}

qboolean WG_ScrollbarPixels( float x, float y, float h, float content, float *scroll,
	float cursorX, float cursorY, qboolean *click, float clickX, float clickY ) {
	return WG_Bar( scroll, x, y, h, content, h, scroll, cursorX, cursorY, click, clickX, clickY );
}

/*
===============================================================================

TEXT

What was typed is shown as it is: a ^ is drawn as itself, not taken for a colour.

===============================================================================
*/

#define WG_MAX_LINES		64		// a field's wrapped lines
#define WG_SHOWN_LINES		10		// grown no taller than this; it scrolls to the cursor
#define WG_DOUBLECLICK_MS	350

static const vec4_t wgSelect = { 0.30f, 0.50f, 0.95f, 0.45f };
static const vec4_t wgCursor = { 1.0f, 1.0f, 1.0f, 0.95f };

// each character's width in a font at scale 1
#define WG_FONT_CACHE	4
static struct {
	int		font;
	float	w[256];
	byte	known[256];
} wgWidths[WG_FONT_CACHE];
static int wgNextWidths;

static float WG_CharW( const wgStyle_t *s, int ch ) {
	int slot = -1;

	ch &= 255;
	for ( int i = 0; i < WG_FONT_CACHE; i++ ) {
		if ( wgWidths[i].font == s->font ) {
			slot = i;
			break;
		}
	}
	if ( slot < 0 ) {
		slot = wgNextWidths;
		wgNextWidths = ( wgNextWidths + 1 ) % WG_FONT_CACHE;
		memset( &wgWidths[slot], 0, sizeof( wgWidths[slot] ) );
		wgWidths[slot].font = s->font;
	}
	if ( !wgWidths[slot].known[ch] ) {
		const char str[2] = { (char)ch, '\0' };

		// a ^ alone isn't a colour; measured big for its fraction of a pixel
		wgWidths[slot].w[ch] = ch < 32 ? 0.0f : re->ext.Font_StrLenPixels( str, s->font, 8.0f ) / 8.0f;
		wgWidths[slot].known[ch] = 1;
	}
	return wgWidths[slot].w[ch] * s->scale;
}

static float WG_Width( const wgStyle_t *s, const char *text, int len ) {
	float w = 0.0f;

	for ( int i = 0; i < len && text[i]; i++ )
		w += WG_CharW( s, (byte)text[i] );
	return w;
}

// len characters at x, the ^ each drawn on its own
static void WG_DrawRun( const wgStyle_t *s, float x, float y, const char *text, int len, const float *color ) {
	char seg[MAX_STRING_CHARS];

	while ( len > 0 && *text ) {
		int n = 0;

		if ( *text == Q_COLOR_ESCAPE ) {
			re->Font_DrawString( (int)x, (int)y, "^", color, s->font, -1, s->scale );
			x += WG_CharW( s, Q_COLOR_ESCAPE );
			text++;
			len--;
			continue;
		}
		while ( n < len && text[n] && text[n] != Q_COLOR_ESCAPE && n < (int)sizeof( seg ) - 1 ) {
			seg[n] = text[n];
			n++;
		}
		seg[n] = '\0';
		re->Font_DrawString( (int)x, (int)y, seg, color, s->font, -1, s->scale );
		x += WG_Width( s, seg, n );
		text += n;
		len -= n;
	}
}

// the text in lines of width w: broken after a space where it can be, else where it must
typedef struct {
	int		start[WG_MAX_LINES + 1];	// start[num] is the length
	int		num;
} wgLines_t;

static void WG_Wrap( const wgStyle_t *s, const char *text, float w, wgLines_t *out ) {
	const int len = (int)strlen( text );
	int start = 0;

	out->num = 0;
	while ( out->num < WG_MAX_LINES - 1 ) {
		float x = 0.0f;
		int i = start, brk = -1;

		out->start[out->num++] = start;
		while ( i < len ) {
			const float cw = WG_CharW( s, (byte)text[i] );

			if ( x + cw > w && i > start )
				break;
			x += cw;
			if ( text[i] == ' ' )
				brk = i + 1;
			i++;
		}
		if ( i >= len ) {
			out->start[out->num] = len;
			return;
		}
		start = ( brk > start ) ? brk : i;
	}
	out->start[out->num++] = start;
	out->start[out->num] = len;
}

// the line the index is on; an index where one line ends and the next begins is the next's
static int WG_LineOf( const wgLines_t *l, int index ) {
	for ( int i = l->num - 1; i > 0; i-- ) {
		if ( index >= l->start[i] )
			return i;
	}
	return 0;
}

// the index nearest x along a line
static int WG_IndexAlong( const wgStyle_t *s, const char *text, const wgLines_t *l, int line, float x ) {
	const int a = l->start[line], b = l->start[line + 1];
	int end = b;
	float at = 0.0f;

	// the space a line broke after belongs to it, but the cursor goes before it
	if ( line < l->num - 1 && end > a && text[end - 1] == ' ' )
		end--;
	for ( int i = a; i < end; i++ ) {
		const float cw = WG_CharW( s, (byte)text[i] );

		if ( x < at + cw * 0.5f )
			return i;
		at += cw;
	}
	return end;
}

/*
===============================================================================

THE FIELD TYPED IN

===============================================================================
*/

static struct {
	char		*buf;			// the field focused, or NULL
	int			cursor, anchor;	// the selection is between them
	qboolean	dragging;
	int			blinkFrom;		// the cursor shows steady after it moved
	int			lastClick;
	int			lastClickAt;
	int			top;			// grown: the first line seen

	// where it was drawn
	wgStyle_t	style;
	float		x, y, w, h;
	qboolean	overlay;		// grown this frame: drawn last
	int			overlayFrame;

	// the last field drawn, focused or not, for a click on it
	char		*lastBuf;
	wgStyle_t	lastStyle;
	float		lastX, lastY, lastW, lastH;
} wg;

static qboolean WG_Ctrl( void ) {
	return (qboolean)( kg.keys[A_CTRL].down || kg.keys[A_CTRL2].down );
}

static qboolean WG_Shift( void ) {
	return (qboolean)( kg.keys[A_SHIFT].down || kg.keys[A_SHIFT2].down );
}

void WG_EditReset( void ) {
	wg.buf = NULL;
	wg.dragging = qfalse;
	wg.top = 0;
}

static void WG_Focus( char *buf ) {
	if ( wg.buf == buf )
		return;
	wg.buf = buf;
	wg.cursor = wg.anchor = (int)strlen( buf );
	wg.dragging = qfalse;
	wg.top = 0;
	wg.blinkFrom = cls.realtime;
}

static void WG_Clamp( const char *buf ) {
	const int len = (int)strlen( buf );

	wg.cursor = Com_Clampi( 0, len, wg.cursor );
	wg.anchor = Com_Clampi( 0, len, wg.anchor );
}

static float WG_TextW( const wgStyle_t *s, float w ) {
	return Q_max( 8.0f, w - s->padX * 2.0f );
}

// the lines the focused field is in, at its width
static void WG_FocusLines( const char *buf, wgLines_t *l ) {
	WG_Wrap( &wg.style, buf, WG_TextW( &wg.style, wg.w ), l );
}

static int WG_ShownLines( int lines ) {
	return Q_min( lines, WG_SHOWN_LINES );
}

float WG_FieldHeight( const wgStyle_t *style, float w, float h, const char *buf, qboolean focused ) {
	wgLines_t l;

	if ( !focused || !buf[0] )
		return h;
	WG_Wrap( style, buf, WG_TextW( style, w ), &l );
	return h + ( WG_ShownLines( l.num ) - 1 ) * style->lineH;
}

// the index under a point in the focused field
static int WG_IndexAt( const char *buf, float px, float py ) {
	wgLines_t l;
	int line;

	WG_FocusLines( buf, &l );
	line = (int)floorf( ( py - wg.y - wg.style.padY ) / wg.style.lineH );
	if ( l.num > 1 )
		line += wg.top;
	line = Com_Clampi( 0, l.num - 1, line );
	return WG_IndexAlong( &wg.style, buf, &l, line, px - wg.x - wg.style.padX );
}

static qboolean WG_WordChar( int ch ) {
	return (qboolean)( ch != ' ' && ch != '\0' );
}

void WG_FieldClick( char *buf, float clickX, float clickY ) {
	if ( wg.lastBuf == buf && wg.buf != buf ) {
		// not focused when it was drawn: it was drawn from its start, on one line
		WG_Focus( buf );
		wg.style = wg.lastStyle;
		wg.x = wg.lastX, wg.y = wg.lastY, wg.w = wg.lastW, wg.h = wg.lastH;
	} else if ( wg.buf != buf ) {
		WG_Focus( buf );
		return;
	}
	WG_Clamp( buf );

	const int at = WG_IndexAt( buf, clickX, clickY );

	// a second click on the same place selects the word
	if ( cls.realtime - wg.lastClick < WG_DOUBLECLICK_MS && at == wg.lastClickAt ) {
		int a = at, b = at;

		while ( a > 0 && WG_WordChar( buf[a - 1] ) )
			a--;
		while ( WG_WordChar( buf[b] ) )
			b++;
		wg.anchor = a;
		wg.cursor = b;
		wg.lastClick = 0;
		wg.dragging = qfalse;
		wg.blinkFrom = cls.realtime;
		return;
	}
	wg.lastClick = cls.realtime;
	wg.lastClickAt = at;
	wg.cursor = at;
	if ( !WG_Shift() )
		wg.anchor = at;
	wg.dragging = qtrue;
	wg.blinkFrom = cls.realtime;
}

// the lines, the selection and the cursor; first the top line shown
static void WG_DrawLines( const char *buf, const wgLines_t *l, int first, int count, float x, float y ) {
	const wgStyle_t *s = &wg.style;
	const int selA = Q_min( wg.cursor, wg.anchor ), selB = Q_max( wg.cursor, wg.anchor );
	const qboolean blink = (qboolean)( ( ( cls.realtime - wg.blinkFrom ) / 500 ) % 2 == 0 );
	const int cursorLine = WG_LineOf( l, wg.cursor );

	for ( int i = first; i < first + count && i < l->num; i++ ) {
		const int a = l->start[i], b = l->start[i + 1];
		const float ly = y + ( i - first ) * s->lineH;
		const float tx = x + s->padX, ty = ly + s->padY;

		if ( selB > a && selA < b ) {
			const int from = Q_max( selA, a ), to = Q_min( selB, b );
			const float sx = tx + WG_Width( s, buf + a, from - a );

			WG_Fill( sx, ty, Q_max( 2.0f, WG_Width( s, buf + from, to - from ) ), s->lineH, wgSelect );
		}
		WG_DrawRun( s, tx, ty, buf + a, b - a, s->text );
		if ( i == cursorLine && blink )
			WG_Fill( tx + WG_Width( s, buf + a, wg.cursor - a ), ty, 1.0f, s->lineH, wgCursor );
	}
}

void WG_Field( const wgStyle_t *style, float x, float y, float w, float h, char *buf, qboolean focused,
	const char *placeholder, float cursorX, float cursorY ) {
	wgLines_t l;

	wg.lastBuf = buf;
	wg.lastStyle = *style;
	wg.lastX = x, wg.lastY = y, wg.lastW = w, wg.lastH = h;

	if ( !focused ) {
		if ( wg.buf == buf )
			WG_EditReset();
		if ( !buf[0] ) {
			if ( placeholder && placeholder[0] )
				re->Font_DrawString( (int)( x + style->padX ), (int)( y + style->padY ), placeholder, style->dim, style->font, -1, style->scale );
			return;
		}
		// the start, cut with ...
		const float room = WG_TextW( style, w );
		const int len = (int)strlen( buf );

		if ( WG_Width( style, buf, len ) <= room ) {
			WG_DrawRun( style, x + style->padX, y + style->padY, buf, len, style->text );
		} else {
			const float dots = WG_Width( style, "...", 3 );
			int n = 0;
			float at = 0.0f;

			while ( n < len && at + WG_CharW( style, (byte)buf[n] ) <= room - dots ) {
				at += WG_CharW( style, (byte)buf[n] );
				n++;
			}
			WG_DrawRun( style, x + style->padX, y + style->padY, buf, n, style->text );
			re->Font_DrawString( (int)( x + style->padX + at ), (int)( y + style->padY ), "...", style->dim, style->font, -1, style->scale );
		}
		return;
	}

	WG_Focus( buf );
	WG_Clamp( buf );
	wg.style = *style;
	wg.x = x, wg.y = y, wg.w = w, wg.h = h;

	if ( wg.dragging ) {
		if ( WG_MouseDown() ) {
			const int at = WG_IndexAt( buf, cursorX, cursorY );

			if ( at != wg.cursor )
				wg.blinkFrom = cls.realtime;
			wg.cursor = at;
		} else {
			wg.dragging = qfalse;
		}
	}

	WG_FocusLines( buf, &l );
	if ( l.num > 1 ) {
		// grown: drawn over everything at the frame's end, scrolled to the cursor
		const int shown = WG_ShownLines( l.num ), line = WG_LineOf( &l, wg.cursor );

		if ( line < wg.top )
			wg.top = line;
		else if ( line >= wg.top + shown )
			wg.top = line - shown + 1;
		wg.top = Com_Clampi( 0, l.num - shown, wg.top );
		wg.overlay = qtrue;
		wg.overlayFrame = cls.framecount;
		return;
	}
	wg.top = 0;
	WG_DrawLines( buf, &l, 0, 1, x, y );
}

void WG_DrawOverlay( void ) {
	wgLines_t l;
	int shown;
	float h;

	if ( !wg.overlay )
		return;
	wg.overlay = qfalse;
	if ( !wg.buf || wg.overlayFrame != cls.framecount )
		return;
	WG_Clamp( wg.buf );
	WG_FocusLines( wg.buf, &l );
	shown = WG_ShownLines( l.num );
	h = wg.h + ( shown - 1 ) * wg.style.lineH;

	WG_Fill( wg.x, wg.y, wg.w, h, wg.style.fill );
	WG_Fill( wg.x, wg.y, wg.w, 1, wg.style.border );
	WG_Fill( wg.x, wg.y + h - 1, wg.w, 1, wg.style.border );
	WG_Fill( wg.x, wg.y, 1, h, wg.style.border );
	WG_Fill( wg.x + wg.w - 1, wg.y, 1, h, wg.style.border );
	WG_DrawLines( wg.buf, &l, wg.top, shown, wg.x, wg.y );
	if ( l.num > shown ) {
		float top = (float)wg.top;

		// how far down it is, not dragged: the cursor leads
		WG_Bar( &wg.top, wg.x + wg.w - WG_SCROLL_W - 1, wg.y + 1, h - 2, (float)l.num, (float)shown, &top,
			-1.0f, -1.0f, NULL, 0.0f, 0.0f );
	}
}

/*
===============================================================================

EDITING

===============================================================================
*/

static qboolean WG_HasSelection( void ) {
	return (qboolean)( wg.cursor != wg.anchor );
}

static void WG_DeleteSelection( char *buf ) {
	const int a = Q_min( wg.cursor, wg.anchor ), b = Q_max( wg.cursor, wg.anchor );

	memmove( buf + a, buf + b, strlen( buf + b ) + 1 );
	wg.cursor = wg.anchor = a;
}

static qboolean WG_Allowed( int ch, wgAllow_t allow ) {
	if ( ch < 32 || ch == 127 )
		return qfalse;
	return allow ? allow( ch ) : qtrue;
}

// text put in at the cursor over the selection, as much as fits; qtrue if anything changed
static qboolean WG_Insert( char *buf, int size, const char *text, int n, wgAllow_t allow ) {
	qboolean changed = qfalse;
	int len;

	if ( WG_HasSelection() ) {
		WG_DeleteSelection( buf );
		changed = qtrue;
	}
	len = (int)strlen( buf );
	for ( int i = 0; i < n && len < size - 1; i++ ) {
		int ch = (byte)text[i];

		if ( ch == '\n' || ch == '\r' || ch == '\t' )
			ch = ' ';
		if ( !WG_Allowed( ch, allow ) )
			continue;
		memmove( buf + wg.cursor + 1, buf + wg.cursor, len - wg.cursor + 1 );
		buf[wg.cursor++] = (char)ch;
		len++;
		changed = qtrue;
	}
	wg.anchor = wg.cursor;
	return changed;
}

static void WG_Copy( const char *buf ) {
	char text[MAX_STRING_CHARS * 2];
	const int a = Q_min( wg.cursor, wg.anchor ), b = Q_max( wg.cursor, wg.anchor );

	if ( a == b )
		return;
	Q_strncpyz( text, buf + a, Q_min( (int)sizeof( text ), b - a + 1 ) );
	Sys_SetClipboardData( text );
}

static qboolean WG_Paste( char *buf, int size, wgAllow_t allow ) {
	char text[MAX_STRING_CHARS * 2];
	char *clip = Sys_GetClipboardData(), *c;
	int n = 0;

	if ( !clip )
		return qfalse;
	// as the console takes it: UTF-8 to the game's own letters
	c = clip;
	while ( *c && n < (int)sizeof( text ) - 1 ) {
		const uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );

		text[n++] = (char)ConvertUTF32ToExpectedCharset( utf32 );
	}
	text[n] = '\0';
	Z_Free( clip );
	return WG_Insert( buf, size, text, n, allow );
}

static int WG_WordLeft( const char *buf, int at ) {
	while ( at > 0 && buf[at - 1] == ' ' )
		at--;
	while ( at > 0 && buf[at - 1] != ' ' )
		at--;
	return at;
}

static int WG_WordRight( const char *buf, int at ) {
	while ( buf[at] && buf[at] != ' ' )
		at++;
	while ( buf[at] == ' ' )
		at++;
	return at;
}

int WG_EditKey( char *buf, int size, int key, wgAllow_t allow ) {
	const qboolean ctrl = WG_Ctrl(), shift = WG_Shift();
	const int len = (int)strlen( buf ), letter = keynames[key].lower;
	int to = -1;

	if ( wg.buf != buf )
		WG_Focus( buf );
	WG_Clamp( buf );

	if ( ctrl && letter == 'a' ) {
		wg.anchor = 0;
		wg.cursor = len;
		wg.blinkFrom = cls.realtime;
		return WG_EDIT_MOVED;
	}
	if ( ( ctrl && letter == 'c' ) || ( ctrl && key == A_INSERT ) ) {
		WG_Copy( buf );
		return WG_EDIT_MOVED;
	}
	if ( ctrl && letter == 'x' ) {
		if ( !WG_HasSelection() )
			return WG_EDIT_MOVED;
		WG_Copy( buf );
		WG_DeleteSelection( buf );
		wg.blinkFrom = cls.realtime;
		return WG_EDIT_CHANGED;
	}
	if ( ( ctrl && letter == 'v' ) || ( shift && ( key == A_INSERT || key == A_KP_0 ) ) ) {
		wg.blinkFrom = cls.realtime;
		return WG_Paste( buf, size, allow ) ? WG_EDIT_CHANGED : WG_EDIT_MOVED;
	}

	switch ( key ) {
	case A_BACKSPACE:
	case A_DELETE:
		wg.blinkFrom = cls.realtime;
		if ( WG_HasSelection() ) {
			WG_DeleteSelection( buf );
			return WG_EDIT_CHANGED;
		}
		if ( key == A_BACKSPACE ) {
			if ( !wg.cursor )
				return WG_EDIT_MOVED;
			wg.anchor = ctrl ? WG_WordLeft( buf, wg.cursor ) : wg.cursor - 1;
		} else {
			if ( wg.cursor >= len )
				return WG_EDIT_MOVED;
			wg.anchor = ctrl ? WG_WordRight( buf, wg.cursor ) : wg.cursor + 1;
		}
		WG_DeleteSelection( buf );
		return WG_EDIT_CHANGED;

	case A_CURSOR_LEFT:
		if ( WG_HasSelection() && !shift )
			to = Q_min( wg.cursor, wg.anchor );
		else
			to = ctrl ? WG_WordLeft( buf, wg.cursor ) : Q_max( 0, wg.cursor - 1 );
		break;
	case A_CURSOR_RIGHT:
		if ( WG_HasSelection() && !shift )
			to = Q_max( wg.cursor, wg.anchor );
		else
			to = ctrl ? WG_WordRight( buf, wg.cursor ) : Q_min( len, wg.cursor + 1 );
		break;
	case A_HOME:
	case A_END:
	case A_CURSOR_UP:
	case A_CURSOR_DOWN: {
		wgLines_t l;
		const qboolean onLines = (qboolean)( wg.style.font && wg.w > 0.0f );
		int line = 0;

		if ( onLines ) {
			WG_FocusLines( buf, &l );
			line = WG_LineOf( &l, wg.cursor );
		} else {
			l.num = 1;
			l.start[0] = 0;
			l.start[1] = len;
		}
		if ( key == A_HOME ) {
			to = ctrl ? 0 : l.start[line];
		} else if ( key == A_END ) {
			to = ctrl ? len : ( line < l.num - 1 ? l.start[line + 1] - ( buf[l.start[line + 1] - 1] == ' ' ? 1 : 0 ) : len );
		} else {
			const int next = line + ( key == A_CURSOR_UP ? -1 : 1 );

			// one line: up and down are the tool's
			if ( l.num < 2 )
				return WG_EDIT_NONE;
			if ( next < 0 ) {
				to = 0;
			} else if ( next >= l.num ) {
				to = len;
			} else {
				const float x = WG_Width( &wg.style, buf + l.start[line], wg.cursor - l.start[line] );
				to = WG_IndexAlong( &wg.style, buf, &l, next, x );
			}
		}
		break;
	}
	default:
		return WG_EDIT_NONE;
	}

	wg.cursor = to;
	if ( !shift )
		wg.anchor = to;
	wg.blinkFrom = cls.realtime;
	return WG_EDIT_MOVED;
}

qboolean WG_EditChar( char *buf, int size, int ch, wgAllow_t allow ) {
	const char c = (char)ch;

	if ( wg.buf != buf )
		WG_Focus( buf );
	WG_Clamp( buf );
	if ( !WG_Allowed( (byte)c, allow ) )
		return qfalse;
	wg.blinkFrom = cls.realtime;
	return WG_Insert( buf, size, &c, 1, allow );
}
