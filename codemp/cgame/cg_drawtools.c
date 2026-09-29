/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"
#include "qcommon/q_shared.h"

static qboolean cg_2DViewportTransformActive = qfalse;
static float cg_2DViewportX = 0.0f;
static float cg_2DViewportY = 0.0f;
static float cg_2DViewportW = SCREEN_WIDTH;
static float cg_2DViewportH = SCREEN_HEIGHT;

void CG_Set2DViewportTransform( qboolean active, float x, float y, float w, float h )
{
	cg_2DViewportTransformActive = active;
	cg_2DViewportX = x;
	cg_2DViewportY = y;
	cg_2DViewportW = w;
	cg_2DViewportH = h;
}

void CG_Transform2DRect( float *x, float *y, float *w, float *h )
{
	if ( !cg_2DViewportTransformActive ) {
		return;
	}

	*x = cg_2DViewportX + ( *x * cg_2DViewportW / SCREEN_WIDTH );
	*y = cg_2DViewportY + ( *y * cg_2DViewportH / SCREEN_HEIGHT );
	*w = *w * cg_2DViewportW / SCREEN_WIDTH;
	*h = *h * cg_2DViewportH / SCREEN_HEIGHT;
}

float CG_Transform2DScale( float scale )
{
	float xScale;
	float yScale;

	if ( !cg_2DViewportTransformActive ) {
		return scale;
	}

	xScale = cg_2DViewportW / SCREEN_WIDTH;
	yScale = cg_2DViewportH / SCREEN_HEIGHT;
	return scale * ( xScale < yScale ? xScale : yScale );
}

/* Return viewport-local text metrics.  Callers perform alignment before the
 * origin is transformed, so compensate for the axis transform here. */
float CG_Transform2DWidth( float width )
{
	float xScale;

	if ( !cg_2DViewportTransformActive ) {
		return width;
	}
	xScale = cg_2DViewportW / SCREEN_WIDTH;
	return width * CG_Transform2DScale( 1.0f ) / xScale;
}

float CG_Transform2DHeight( float height )
{
	float yScale;

	if ( !cg_2DViewportTransformActive ) {
		return height;
	}
	yScale = cg_2DViewportH / SCREEN_HEIGHT;
	return height * CG_Transform2DScale( 1.0f ) / yScale;
}

qboolean CG_2DViewportTransformActive( void )
{
	return cg_2DViewportTransformActive;
}

/* Axis-aligned HUD primitive with software clipping at the local viewport.
 * The input is in 640x480 viewport-local coordinates. */
void CG_DrawPicUV( float x, float y, float width, float height,
	float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	float right, bottom, ds, dt;

	CG_Transform2DRect( &x, &y, &width, &height );
	if ( width <= 0.0f || height <= 0.0f ) {
		return;
	}

	right = x + width;
	bottom = y + height;
	ds = s2 - s1;
	dt = t2 - t1;
	if ( cg_2DViewportTransformActive ) {
		const float clipRight = cg_2DViewportX + cg_2DViewportW;
		const float clipBottom = cg_2DViewportY + cg_2DViewportH;
		if ( right <= cg_2DViewportX || bottom <= cg_2DViewportY ||
			x >= clipRight || y >= clipBottom ) {
			return;
		}
		if ( x < cg_2DViewportX ) {
			s1 += ds * ( cg_2DViewportX - x ) / width;
			width -= cg_2DViewportX - x;
			x = cg_2DViewportX;
		}
		if ( y < cg_2DViewportY ) {
			t1 += dt * ( cg_2DViewportY - y ) / height;
			height -= cg_2DViewportY - y;
			y = cg_2DViewportY;
		}
		ds = s2 - s1;
		dt = t2 - t1;
		if ( right > clipRight ) {
			s2 -= ds * ( right - clipRight ) / width;
			width = clipRight - x;
		}
		if ( bottom > clipBottom ) {
			t2 -= dt * ( bottom - clipBottom ) / height;
			height = clipBottom - y;
		}
	}
	trap->R_DrawStretchPic( x, y, width, height, s1, t1, s2, t2, hShader );
}


/*
================
CG_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap->R_SetColor( color );

	CG_DrawTopBottom(x, y, width, height, size);
	CG_DrawSides(x, y, width, height, size);

	trap->R_SetColor( NULL );
}



/*
=================
CG_GetColorForHealth
=================
*/
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor ) {
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear( hcolor );	// black
		hcolor[3] = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count ) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) / 33.0;
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) / 30.0;
	}
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size) {
	CG_DrawPicUV( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	CG_DrawPicUV( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	CG_DrawPicUV( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	CG_DrawPicUV( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

/*
-------------------------
CGC_FillRect2
real coords
-------------------------
*/
void CG_FillRect2( float x, float y, float width, float height, const float *color ) {
	trap->R_SetColor( color );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader);
	trap->R_SetColor( NULL );
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	trap->R_SetColor( color );
	CG_DrawPicUV( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader);
	trap->R_SetColor( NULL );
}


/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_DrawPicUV( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
CG_DrawRotatePic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
rotates around the upper right corner of the passed in point
=================
*/
void CG_DrawRotatePic( float x, float y, float width, float height,float angle, qhandle_t hShader ) {
	if ( cg_2DViewportTransformActive ) {
		CG_DrawPic( x, y, width, height, hShader );
		return;
	}
	CG_Transform2DRect( &x, &y, &width, &height );
	trap->R_DrawRotatePic( x, y, width, height, 0, 0, 1, 1, angle, hShader );
}

/*
================
CG_DrawRotatePic2

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
Actually rotates around the center point of the passed in coordinates
=================
*/
void CG_DrawRotatePic2( float x, float y, float width, float height,float angle, qhandle_t hShader ) {
	if ( cg_2DViewportTransformActive ) {
		CG_DrawPic( x - width * 0.5f, y - height * 0.5f, width, height, hShader );
		return;
	}
	CG_Transform2DRect( &x, &y, &width, &height );
	trap->R_DrawRotatePic2( x, y, width, height, 0, 0, 1, 1, angle, hShader );
}

/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float size2;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.03125;
	size2 = 0.0625;

	CG_DrawPicUV( x, y, width, height, fcol, frow,
		fcol + size, frow + size2, cgs.media.charsetShader );

}

/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
#include "ui/menudef.h"	// for "ITEM_TEXTSTYLE_SHADOWED"
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars )
{
	if (trap->R_Language_IsAsian())
	{
		// hack-a-doodle-do (post-release quick fix code)...
		//
		vec4_t color;
		memcpy(color,setColor, sizeof(color));	// de-const it
		CG_Text_Paint(x, y, 1.0f,	// float scale,
						color,		// vec4_t color,
						string,		// const char *text,
						0.0f,		// float adjust,
						0,			// int limit,
						shadow ? ITEM_TEXTSTYLE_SHADOWED : 0,	// int style,
						FONT_MEDIUM		// iMenuFont
						) ;
	}
	else
	{
		vec4_t		color;
		const char	*s;
		int			xx;

		// draw the drop shadow
		if (shadow) {
			color[0] = color[1] = color[2] = 0;
			color[3] = setColor[3];
			trap->R_SetColor( color );
			s = string;
			xx = x;
			while ( *s ) {
				if ( Q_IsColorString( s ) ) {
					s += 2;
					continue;
				}
				CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
				xx += charWidth;
				s++;
			}
		}

		// draw the colored text
		s = string;
		xx = x;
		trap->R_SetColor( setColor );
		while ( *s ) {
			if ( Q_IsColorString( s ) ) {
				if ( !forceColor ) {
					memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
					color[3] = setColor[3];
					trap->R_SetColor( color );
				}
				s += 2;
				continue;
			}
			CG_DrawChar( xx, y, charWidth, charHeight, *s );
			xx += charWidth;
			s++;
		}
		trap->R_SetColor( NULL );
	}
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap->R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 &&
		cg.refdef.width == w && cg.refdef.height == h ) {
		return;		// full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// this color shouldn't be visible yet
	if (t < 0){
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}


/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForGivenHealth( vec4_t hcolor, int health )
{
	// set the color based on health
	hcolor[0] = 1.0;
	if ( health >= 100 )
	{
		hcolor[2] = 1.0;
	}
	else if ( health < 66 )
	{
		hcolor[2] = 0;
	}
	else
	{
		hcolor[2] = ( health - 66 ) / 33.0;
	}

	if ( health > 60 )
	{
		hcolor[1] = 1.0;
	}
	else if ( health < 30 )
	{
		hcolor[1] = 0;
	}
	else
	{
		hcolor[1] = ( health - 30 ) / 30.0;
	}
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForHealth( vec4_t hcolor )
{
	int		health;
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	health = cg.snap->ps.stats[STAT_HEALTH];

	if ( health <= 0 )
	{
		VectorClear( hcolor );	// black
		hcolor[3] = 1;
		return;
	}

	count = cg.snap->ps.stats[STAT_ARMOR];
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count )
	{
		count = max;
	}
	health += count;

	hcolor[3] = 1.0;
	CG_ColorForGivenHealth( hcolor, health );
}

/*
==============
CG_DrawNumField

Take x,y positions as if 640 x 480 and scales them to the proper resolution

==============
*/
void CG_DrawNumField (int x, int y, int width, int value,int charWidth,int charHeight,int style,qboolean zeroFill)
{
	char	num[16], *ptr;
	int		l;
	int		frame;
	int		xWidth;
	int		i = 0;

	if (width < 1) {
		return;
	}

	// draw number string
	if (width > 5) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;

	// FIXME: Might need to do something different for the chunky font??
	switch(style)
	{
	case NUM_FONT_SMALL:
		xWidth = charWidth;
		break;
	case NUM_FONT_CHUNKY:
		xWidth = (charWidth/1.2f) + 2;
		break;
	default:
	case NUM_FONT_BIG:
		xWidth = (charWidth/2) + 7;//(charWidth/6);
		break;
	}

	if ( zeroFill )
	{
		for (i = 0; i < (width - l); i++ )
		{
			switch(style)
			{
			case NUM_FONT_SMALL:
				CG_DrawPic( x,y, charWidth, charHeight, cgs.media.smallnumberShaders[0] );
				break;
			case NUM_FONT_CHUNKY:
				CG_DrawPic( x,y, charWidth, charHeight, cgs.media.chunkyNumberShaders[0] );
				break;
			default:
			case NUM_FONT_BIG:
				CG_DrawPic( x,y, charWidth, charHeight, cgs.media.numberShaders[0] );
				break;
			}
			x += 2 + (xWidth);
		}
	}
	else
	{
		x += 2 + (xWidth)*(width - l);
	}

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		switch(style)
		{
		case NUM_FONT_SMALL:
			CG_DrawPic( x,y, charWidth, charHeight, cgs.media.smallnumberShaders[frame] );
			x++;	// For a one line gap
			break;
		case NUM_FONT_CHUNKY:
			CG_DrawPic( x,y, charWidth, charHeight, cgs.media.chunkyNumberShaders[frame] );
			break;
		default:
		case NUM_FONT_BIG:
			CG_DrawPic( x,y, charWidth, charHeight, cgs.media.numberShaders[frame] );
			break;
		}

		x += (xWidth);
		ptr++;
		l--;
	}

}

#include "ui/ui_shared.h"	// for some text style junk
void CG_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color )
{
	// having all these different style defines (1 for UI, one for CG, and now one for the re->font stuff)
	//	is dumb, but for now...
	//
	int iStyle = 0;
	int iMenuFont = (style & UI_SMALLFONT) ? FONT_SMALL : FONT_MEDIUM;

	switch (style & (UI_LEFT|UI_CENTER|UI_RIGHT))
	{
		default:
		case UI_LEFT:
		{
			// nada...
		}
		break;

		case UI_CENTER:
		{
			x -= CG_Text_Width(str, 1.0, iMenuFont) / 2;
		}
		break;

		case UI_RIGHT:
		{
			x -= CG_Text_Width(str, 1.0, iMenuFont) / 2;
		}
		break;
	}

	if (style & UI_DROPSHADOW)
	{
		iStyle = ITEM_TEXTSTYLE_SHADOWED;
	}
	else
	if ( style & (UI_BLINK|UI_PULSE) )
	{
		iStyle = ITEM_TEXTSTYLE_BLINK;
	}

	CG_Text_Paint(x, y, 1.0, color, str, 0, 0, iStyle, iMenuFont);
}

void CG_DrawScaledProportionalString( int x, int y, const char* str, int style, vec4_t color, float scale)
{
	// having all these different style defines (1 for UI, one for CG, and now one for the re->font stuff)
	//	is dumb, but for now...
	//
	int iStyle = 0;

	switch (style & (UI_LEFT|UI_CENTER|UI_RIGHT))
	{
		default:
		case UI_LEFT:
		{
			// nada...
		}
		break;

		case UI_CENTER:
		{
			x -= CG_Text_Width(str, scale, FONT_MEDIUM) / 2;
		}
		break;

		case UI_RIGHT:
		{
			x -= CG_Text_Width(str, scale, FONT_MEDIUM) / 2;
		}
		break;
	}

	if (style & UI_DROPSHADOW)
	{
		iStyle = ITEM_TEXTSTYLE_SHADOWED;
	}
	else
	if ( style & (UI_BLINK|UI_PULSE) )
	{
		iStyle = ITEM_TEXTSTYLE_BLINK;
	}

	CG_Text_Paint(x, y, scale, color, str, 0, 0, iStyle, FONT_MEDIUM);
}
