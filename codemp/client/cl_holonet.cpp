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

// cl_holonet.cpp -- the HoloNet on the datapad: JEDI's news, from jediholo.net's feed
//
// The game reads the feed itself (cl_holonet_url), when the app opens and again after a while: the
// latest articles, listed; one opened is shown whole, its pictures too (cl_html.cpp), scrolled
// with the wheel. Open on the web opens it in the browser.

#include "client.h"
#include "cl_datapad.h"
#include "cl_html.h"
#include "cl_http.h"

#define HN_FEED_URL		"https://www.jediholo.net/feed/"
#define HN_REFRESH_MS	( 10 * 60 * 1000 )
#define HN_FEED_BYTES	( 2 * 1024 * 1024 )
#define HN_ROW_H		30.0f
#define HN_WHEEL		36.0f		// pixels a notch of the wheel scrolls

static struct {
	qboolean					shown, ready;
	std::vector<rssItem_t>		items;
	int							readTime;			// the feed's last answer
	qboolean					failed;
	int							open;				// the article on screen, -1 the list
	std::vector<htmlBlock_t>	blocks;				// its blocks
	float						scroll, content, view;
	int							listScroll;
} hn;

static httpRequest_t hnHttp;

static void HN_Ask( void ) {
	char error[128];

	if ( hnHttp.state == HTTP_BUSY )
		return;
	hnHttp.maxAnswer = HN_FEED_BYTES;
	hnHttp.headers[0] = '\0';
	hnHttp.form = qfalse;
	hnHttp.noRedirect = qfalse;
	if ( !HTTP_Start( &hnHttp, HN_FEED_URL, NULL, error, sizeof( error ) ) ) {
		hn.failed = qtrue;
		Com_DPrintf( "holonet: %s\n", error );
	}
}

void CL_HoloNet_Frame( void ) {
	if ( HTTP_Poll( &hnHttp ) ) {
		hn.readTime = cls.realtime;
		if ( hnHttp.status == 200 ) {
			std::vector<rssItem_t> items;

			RSS_Items( HTTP_Answer( &hnHttp ), items );
			if ( !items.empty() ) {
				hn.items = items;
				hn.failed = qfalse;
			}
		} else {
			hn.failed = qtrue;
		}
		HTTP_Release( &hnHttp );
	}
	if ( hn.shown && hnHttp.state != HTTP_BUSY && ( !hn.readTime || cls.realtime - hn.readTime > HN_REFRESH_MS ) )
		HN_Ask();
}

void CL_HoloNet_Shown( qboolean shown ) {
	// the first time: on the list
	if ( !hn.ready ) {
		hn.ready = qtrue;
		hn.open = -1;
	}
	hn.shown = shown;
}

static void HN_Open( int i ) {
	hn.open = i;
	hn.scroll = 0;
	hn.blocks.clear();
	if ( i >= 0 && i < (int)hn.items.size() )
		HTML_Blocks( hn.items[i].content, hn.items[i].link.c_str(), hn.blocks );
}

static void HN_DrawList( const dpRect_t *s ) {
	const float x = s->x + DP_Wide( 8 ), w = s->w - DP_Wide( 16 );
	float y = s->y + DP_STATUS_H + 4;
	int rows;

	DP_Text( x, y, w * 0.6f, "HOLONET", dpText, DP_TitleFont(), 0.7f, qfalse );
	if ( DP_Button( x + w - DP_Wide( 64 ), y, DP_Wide( 64 ), hnHttp.state == HTTP_BUSY ? "Reading..." : "Refresh", qfalse ) )
		HN_Ask();
	y += DP_CTRL_H + 4;
	DP_Text( x, y, w, "News from across the galaxy, from jediholo.net", dpDim, DP_Font(), 0.58f, qfalse );
	y += 16;
	if ( hn.items.empty() ) {
		DP_Text( x, y + 10, w, hn.failed ? "The HoloNet couldn't be reached: try Refresh in a moment" : "Tuning in to the HoloNet...", dpDim, DP_Font(), 0.62f, qfalse );
		return;
	}
	rows = (int)( ( s->y + s->h - y - 6 ) / HN_ROW_H );
	hn.listScroll = Com_Clampi( 0, Q_max( 0, (int)hn.items.size() - rows ), hn.listScroll );
	for ( int r = 0; r < rows && hn.listScroll + r < (int)hn.items.size(); r++ ) {
		const rssItem_t &it = hn.items[hn.listScroll + r];
		const qboolean hover = DP_InRect( x, y, w, HN_ROW_H - 3 );

		DP_Fill( x, y, w, HN_ROW_H - 3, hover ? dpButtonHover : dpTile );
		DP_Text( x + DP_Wide( 4 ), y + 2, w - DP_Wide( 8 ), it.title.c_str(), hover ? dpWhite : dpText, DP_Font(), 0.62f, qfalse );
		DP_Text( x + DP_Wide( 4 ), y + 15, w - DP_Wide( 8 ), va( "%s   %s%s%s", it.date.c_str(), it.author.c_str(),
			it.category.empty() ? "" : "   ", it.category.c_str() ), dpDim, DP_Font(), 0.55f, qfalse );
		if ( DP_Clicked( x, y, w, HN_ROW_H - 3 ) )
			HN_Open( hn.listScroll + r );
		y += HN_ROW_H;
	}
	DP_ScrollBar( s->x + s->w - DP_Wide( 5 ), s->y + DP_STATUS_H + 40, s->h - DP_STATUS_H - 46, (float)hn.listScroll,
		(float)hn.items.size() + rows - Q_min( rows, (int)hn.items.size() ) );
}

static void HN_DrawArticle( const dpRect_t *s ) {
	const rssItem_t &it = hn.items[hn.open];
	const float x = s->x + DP_Wide( 8 ), w = s->w - DP_Wide( 18 );
	const float top = s->y + DP_STATUS_H + DP_CTRL_H + 10, bottom = s->y + s->h - 4;
	float y = s->y + DP_STATUS_H + 4;
	std::vector<htmlBlock_t> head;
	htmlBlock_t b;

	if ( DP_Button( x, y, DP_Wide( 50 ), "< Back", qfalse ) ) {
		HN_Open( -1 );
		return;
	}
	if ( DP_Button( x + w - DP_Wide( 96 ), y, DP_Wide( 96 ), "Open on the web", qfalse ) && !it.link.empty() )
		Sys_OpenPath( it.link.c_str() );

	// its title and byline, then the article, scrolled
	b.depth = 0;
	b.kind = HB_HEADING;
	b.text = it.title;
	head.push_back( b );
	b.kind = HB_QUOTE_HEAD;
	b.text = it.date + "   " + it.author + ( it.category.empty() ? "" : "   " + it.category );
	head.push_back( b );
	b.kind = HB_RULE;
	b.text.clear();
	head.push_back( b );
	hn.view = bottom - top;
	y = top - hn.scroll;
	y += DP_DrawBlocks( head, x, y, w, top, bottom );
	y += DP_DrawBlocks( hn.blocks, x, y, w, top, bottom );
	hn.content = y + hn.scroll - top;
	hn.scroll = Com_Clamp( 0.0f, Q_max( 0.0f, hn.content - hn.view ), hn.scroll );
	DP_ScrollBar( s->x + s->w - DP_Wide( 6 ), top, hn.view, hn.scroll, hn.content );
}

// datapad holonet <n>: the n-th article, once the feed's in
void CL_HoloNet_Open( int n ) {
	if ( n >= 1 && n <= (int)hn.items.size() )
		HN_Open( n - 1 );
}

void CL_HoloNet_Draw( const dpRect_t *s ) {
	if ( hn.open >= 0 && hn.open < (int)hn.items.size() )
		HN_DrawArticle( s );
	else
		HN_DrawList( s );
}

// the wheel and the page keys scroll the article, or the list
qboolean CL_HoloNet_Key( int key ) {
	const float page = hn.view > 0 ? hn.view - 30 : 200;
	float step = 0;

	if ( key == A_MWHEELUP || key == A_CURSOR_UP )
		step = -HN_WHEEL;
	else if ( key == A_MWHEELDOWN || key == A_CURSOR_DOWN )
		step = HN_WHEEL;
	else if ( key == A_PAGE_UP )
		step = -page;
	else if ( key == A_PAGE_DOWN || key == A_SPACE )
		step = page;
	else if ( key == A_HOME )
		step = -1e6f;
	else if ( key == A_END )
		step = 1e6f;
	else if ( key == A_BACKSPACE && hn.open >= 0 ) {
		HN_Open( -1 );
		return qtrue;
	} else
		return qfalse;
	if ( hn.open >= 0 )
		hn.scroll = Com_Clamp( 0.0f, Q_max( 0.0f, hn.content - hn.view ), hn.scroll + step );
	else
		hn.listScroll += step > 0 ? 1 : -1;
	return qtrue;
}

// each frame: the web apps' answers, and their pictures coming
void CL_DatapadWeb_Frame( void ) {
	HTML_PictureFrame();
	CL_HoloNet_Frame();
	CL_Comport_Frame();
}
