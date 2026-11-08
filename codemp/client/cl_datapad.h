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

// cl_datapad.h -- what the datapad's apps draw with: its colours, text, buttons and clicks
//
// An app in its own file draws on the screen's rect it's given, in the 640x480 layout; widths are
// as on a 4:3 screen, through DP_Wide, so it keeps its shape on a wide one with cl_ratioFix.

#pragma once

#include "client.h"

typedef struct {
	float	x, y, w, h;
} dpRect_t;

#define DP_STATUS_H		14.0f		// the bar at the top of the screen, above what an app draws
#define DP_CTRL_H		15.0f

// the lit colours of the moment, from the one picked in Settings
extern float dpGlow[4], dpScan[4], dpText[4], dpDim[4], dpScreen[4], dpButton[4], dpButtonHover[4], dpTile[4];
extern const float dpWhite[4];

float		DP_Wide( float w );
int			DP_Font( void );
int			DP_TitleFont( void );
float		DP_CursorX( void );
float		DP_CursorY( void );
void		DP_PlaySound( sfxHandle_t sound );

void		DP_Fill( float x, float y, float w, float h, const float *color );
void		DP_Frame( float x, float y, float w, float h, const float *color );
void		DP_Text( float x, float y, float w, const char *text, const float *color, int font, float scale, qboolean centred );
qboolean	DP_InRect( float x, float y, float w, float h );
qboolean	DP_Clicked( float x, float y, float w, float h );		// the click in it, taken
qboolean	DP_Button( float x, float y, float w, const char *label, qboolean on );

// web pages' blocks (cl_html.h) laid out to w from y; only what's between top and bottom drawn, a
// picture cut to them. Their height, drawn or not
#include <vector>
#include "cl_html.h"
float		DP_DrawBlocks( const std::vector<htmlBlock_t> &blocks, float x, float y, float w, float top, float bottom );
// a scroll bar's look beside a scrolled view: how far down, of how much
void		DP_ScrollBar( float x, float y, float h, float scroll, float content );

// the datapad's apps in files of their own: what each is given
void		CL_HoloNet_Draw( const dpRect_t *screen );
void		CL_HoloNet_Shown( qboolean shown );
qboolean	CL_HoloNet_Key( int key );
void		CL_HoloNet_Frame( void );
void		CL_Comport_Draw( const dpRect_t *screen );
void		CL_Comport_Shown( qboolean shown );
qboolean	CL_Comport_Key( int key );
void		CL_Comport_Char( int ch );
void		CL_Comport_Frame( void );
void		CL_Comport_Init( void );
void		CL_Comport_Open( const char *what );
void		CL_HoloNet_Open( int n );

// cl_pazaak.cpp: the Pazaak app
void		CL_Pazaak_Draw( const dpRect_t *screen );
void		CL_Pazaak_Shown( qboolean shown );		// on the datapad's screen, or not any more
qboolean	CL_Pazaak_Key( int key );				// a key for its text field: qtrue if taken
void		CL_Pazaak_Char( int ch );
void		CL_Pazaak_Frame( void );
void		CL_Pazaak_Init( void );
