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

// cl_widgets.h -- the scrollbar and text field the tools share; see cl_widgets.cpp

#pragma once

#define WG_SCROLL_W		7		// a scrollbar's width, taken from the list's right side

/*
A list's scrollbar, at x along its right side, shown only when there's more than fits. A click
on the handle drags it, one beside it jumps there and drags on; it takes the click (*click
false) so the list doesn't see it. qtrue when it's shown: the rows are WG_SCROLL_W narrower.
*/
qboolean WG_Scrollbar( float x, float y, float h, int count, int visible, int *scroll,
	float cursorX, float cursorY, qboolean *click, float clickX, float clickY );

// the same for a list scrolled by pixels rather than rows: content tall, view h of it seen
qboolean WG_ScrollbarPixels( float x, float y, float h, float content, float *scroll,
	float cursorX, float cursorY, qboolean *click, float clickX, float clickY );

// how a tool's text field looks
typedef struct {
	int				font;
	float			scale;
	float			lineH;			// one line's height
	float			padX, padY;		// the text from the box's left and top
	const float		*fill;			// a field grown over what's below: its box
	const float		*border;
	const float		*text;
	const float		*dim;			// the placeholder
} wgStyle_t;

typedef qboolean ( *wgAllow_t )( int ch );	// a character a field takes; NULL any printable one

/*
A text field: one typed in at a time, known by its buffer. Its box is the tool's; this draws the
text in it and, focused, the cursor and the selection. Focused, a text too long for one line goes
on to the lines below, the box grown down over what's under it: drawn by WG_DrawOverlay at the
end of the tool's frame. Unfocused, the start is shown, cut with ... .
*/
void		WG_Field( const wgStyle_t *style, float x, float y, float w, float h, char *buf, qboolean focused,
				const char *placeholder, float cursorX, float cursorY );
// the field's height now, grown when it's focused and long: what a click in it is looked for in
float		WG_FieldHeight( const wgStyle_t *style, float w, float h, const char *buf, qboolean focused );
// a click in the focused field: the cursor goes there, and a drag selects
void		WG_FieldClick( char *buf, float clickX, float clickY );
// the grown field, over everything else; each tool calls it last in its frame
void		WG_DrawOverlay( void );

// the field typed in: its cursor and selection forgotten, as when another one is clicked
void		WG_EditReset( void );

enum {
	WG_EDIT_NONE		= 0,		// not a key for the field
	WG_EDIT_MOVED		= 1,		// the cursor or selection moved
	WG_EDIT_CHANGED		= 2			// the text changed
};
// keys in the focused field: the arrows (ctrl a word), home, end, delete, backspace (ctrl a
// word), shift to select, ctrl+a, ctrl+c, ctrl+x, ctrl+v and shift+insert. Up and down only when
// it's on more than one line
int			WG_EditKey( char *buf, int size, int key, wgAllow_t allow );
// a typed character, put in at the cursor over the selection; qtrue if it went in
qboolean	WG_EditChar( char *buf, int size, int ch, wgAllow_t allow );
