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

// cl_html.h -- web pages read for the datapad: HTML into blocks of text and pictures, a page's
// parts found by their class, an RSS feed's items, and pictures downloaded for the renderer

#pragma once

#include <map>
#include <string>
#include <vector>
#include "client.h"

typedef enum {
	HB_TEXT,			// a paragraph
	HB_HEADING,
	HB_QUOTE_HEAD,		// "Someone wrote:" over a quote
	HB_LIST,			// a list's item
	HB_CODE,			// kept as it's written, line by line
	HB_IMAGE,			// src: the picture's address
	HB_RULE
} htmlBlockKind_t;

typedef struct {
	htmlBlockKind_t	kind;
	int				depth;			// inside this many quotes
	std::string		text;			// the game's Latin-1, entities decoded
	std::string		src;
} htmlBlock_t;

// an element found: from its < to past its end tag; inner between them
typedef struct {
	size_t		start, innerStart, innerEnd, end;
	std::string	tag, attrs;
} htmlElement_t;

// text: entities decoded, UTF-8 made the game's Latin-1 (curly quotes, dashes and ... as ASCII)
std::string	HTML_Decode( const std::string &in );
// a fragment's text alone: tags out, spaces as one
std::string	HTML_Text( const std::string &html );
// a fragment as the datapad shows it; links and pictures made absolute from base
void		HTML_Blocks( const std::string &html, const char *base, std::vector<htmlBlock_t> &out );

// the next element from pos with this tag ("" any) and this class among its classes ("" any)
bool		HTML_Find( const std::string &html, size_t pos, const char *tag, const char *cls, htmlElement_t &el );
std::string	HTML_Attr( const std::string &attrs, const char *name );
std::string	HTML_Inner( const std::string &html, const htmlElement_t &el );
// an address made absolute from the page's
std::string	HTML_Url( const char *base, const std::string &href );
// a form's hidden inputs, and those named: name -> value
void		HTML_Inputs( const std::string &html, std::map<std::string, std::string> &out );
// a query parameter's value in an address ("" none)
std::string	HTML_Param( const std::string &url, const char *name );
// a form's value encoded for a POST
std::string	HTML_FormEncode( const std::string &in );

typedef struct {
	std::string	title, link, date, author, category, content;
} rssItem_t;

void		RSS_Items( const std::string &xml, std::vector<rssItem_t> &out );

// a picture from the web for the renderer: 0 while it comes (or if it can't), its size once known
qhandle_t	HTML_Picture( const char *url, int *width, int *height );
void		HTML_PictureFrame( void );
