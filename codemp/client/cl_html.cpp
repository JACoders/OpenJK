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

// cl_html.cpp -- web pages read for the datapad (cl_holonet.cpp, cl_comport.cpp)
//
// Not a browser: a tolerant reading of HTML that's good enough for a WordPress article and a phpBB
// page. Text comes out in blocks (paragraphs, headings, quotes, list items, code, pictures, rules)
// in the game's Latin-1; a page's parts are found by their tag and class.
//
// Pictures are downloaded and kept in datapad/cache/ in the home folder, so the renderer can load
// them as it loads any image: JPEG and PNG only, checked by their first bytes. Each is decoded here,
// scaled to sizes in powers of two (the classic renderer takes nothing else) and kept as a TGA; it's
// drawn at its own shape all the same. The cache is emptied once it's grown past HTML_CACHE_MAX.

#include <algorithm>
#include "cl_html.h"
#include "cl_http.h"

// Pictures decoded here: the classic renderer takes only sizes in powers of two, the web's aren't.
// stb_image (public domain), as rend2 has it, private to this file
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../rd-rend2/stb_image.h"

#define HTML_CACHE_DIR		"datapad/cache"
#define HTML_CACHE_MAX		( 64 * 1024 * 1024 )
#define HTML_DOWNLOADS		3			// at once
#define HTML_MAX_PICTURES	256			// known in a game's run
#define HTML_PICTURE_BYTES	( 8 * 1024 * 1024 )

/*
===============================================================================

Text

===============================================================================
*/

// a character past ASCII as the game shows it: Latin-1 itself, and the usual typography as ASCII
static void HTML_PutCode( std::string &out, unsigned int c ) {
	switch ( c ) {
	case 0x2018: case 0x2019: case 0x201A: case 0x2032:	out += '\''; return;
	case 0x201C: case 0x201D: case 0x201E: case 0x2033:	out += '"'; return;
	case 0x2013: case 0x2014: case 0x2212:				out += '-'; return;
	case 0x2026:										out += "..."; return;
	case 0x2022: case 0x00B7:							out += '*'; return;
	case 0x00A0:										out += ' '; return;
	case 0x2122:										out += "(TM)"; return;
	case 0x200B: case 0xFEFF: case 0x200C: case 0x200D:	return;
	}
	if ( c == '\t' || c == '\n' || c == '\r' )
		out += ' ';
	else if ( c >= 32 && c < 256 && !( c >= 127 && c < 160 ) )
		out += (char)c;
	else if ( c >= 256 )
		out += '?';
}

static const struct {
	const char		*name;
	unsigned int	code;
} htmlEntities[] = {
	{ "amp", '&' }, { "lt", '<' }, { "gt", '>' }, { "quot", '"' }, { "apos", '\'' }, { "nbsp", 0xA0 },
	{ "laquo", 0xAB }, { "raquo", 0xBB }, { "copy", 0xA9 }, { "reg", 0xAE }, { "deg", 0xB0 }, { "plusmn", 0xB1 },
	{ "middot", 0xB7 }, { "eacute", 0xE9 }, { "egrave", 0xE8 }, { "agrave", 0xE0 }, { "ccedil", 0xE7 }, { "uuml", 0xFC },
	{ "ouml", 0xF6 }, { "auml", 0xE4 }, { "szlig", 0xDF }, { "hellip", 0x2026 }, { "mdash", 0x2014 }, { "ndash", 0x2013 },
	{ "lsquo", 0x2018 }, { "rsquo", 0x2019 }, { "ldquo", 0x201C }, { "rdquo", 0x201D }, { "bull", 0x2022 }, { "trade", 0x2122 },
	{ "times", 0xD7 }, { "frac12", 0xBD },
};

std::string HTML_Decode( const std::string &in ) {
	std::string out;
	const unsigned char *p = (const unsigned char *)in.c_str();

	out.reserve( in.size() );
	while ( *p ) {
		if ( *p == '&' ) {
			const unsigned char *semi = p + 1;

			while ( *semi && *semi != ';' && semi - p < 12 )
				semi++;
			if ( *semi == ';' ) {
				const std::string name( (const char *)p + 1, semi - p - 1 );
				unsigned int code = 0;
				bool known = false;

				if ( name.size() > 1 && name[0] == '#' ) {
					code = ( name[1] == 'x' || name[1] == 'X' ) ? (unsigned int)strtoul( name.c_str() + 2, NULL, 16 ) : (unsigned int)strtoul( name.c_str() + 1, NULL, 10 );
					known = code > 0;
				} else {
					for ( size_t i = 0; i < ARRAY_LEN( htmlEntities ) && !known; i++ ) {
						if ( name == htmlEntities[i].name ) {
							code = htmlEntities[i].code;
							known = true;
						}
					}
				}
				if ( known ) {
					HTML_PutCode( out, code );
					p = semi + 1;
					continue;
				}
			}
			out += '&';
			p++;
			continue;
		}
		// UTF-8
		if ( *p >= 0xC0 ) {
			unsigned int c;
			int more = *p >= 0xF0 ? 3 : *p >= 0xE0 ? 2 : 1;

			c = *p & ( 0x3F >> more );
			p++;
			while ( more-- > 0 && ( *p & 0xC0 ) == 0x80 ) {
				c = ( c << 6 ) | ( *p & 0x3F );
				p++;
			}
			HTML_PutCode( out, c );
			continue;
		}
		if ( *p >= 0x80 ) {
			p++;		// a stray byte
			continue;
		}
		HTML_PutCode( out, *p );
		p++;
	}
	return out;
}

// spaces as one, none at the ends
static std::string HTML_Squeeze( const std::string &in ) {
	std::string out;
	bool space = false;

	for ( char c : in ) {
		if ( c == ' ' || c == '\t' || c == '\n' || c == '\r' ) {
			space = !out.empty();
			continue;
		}
		if ( space )
			out += ' ';
		space = false;
		out += c;
	}
	return out;
}

static std::string HTML_Lower( std::string s ) {
	std::transform( s.begin(), s.end(), s.begin(), []( unsigned char c ) { return (char)tolower( c ); } );
	return s;
}

/*
===============================================================================

Tags

===============================================================================
*/

typedef struct {
	size_t		start, end;			// < and past >
	std::string	name;				// lower case, no /
	std::string	attrs;
	bool		closing, selfClosing;
} htmlTag_t;

// the next tag from pos, comments and doctype skipped; false at the end
static bool HTML_NextTag( const std::string &html, size_t pos, htmlTag_t &t ) {
	while ( ( pos = html.find( '<', pos ) ) != std::string::npos ) {
		size_t p = pos + 1, end;

		if ( !html.compare( pos, 4, "<!--" ) ) {
			end = html.find( "-->", pos + 4 );
			pos = end == std::string::npos ? html.size() : end + 3;
			continue;
		}
		if ( !html.compare( pos, 9, "<![CDATA[" ) ) {
			end = html.find( "]]>", pos );
			pos = end == std::string::npos ? html.size() : end + 3;
			continue;
		}
		t.closing = p < html.size() && html[p] == '/';
		if ( t.closing )
			p++;
		if ( p >= html.size() || !( isalpha( (unsigned char)html[p] ) || html[p] == '!' || html[p] == '?' ) ) {
			pos++;
			continue;
		}
		// the end, quotes kept whole
		end = p;
		for ( char quote = 0; end < html.size(); end++ ) {
			if ( quote ) {
				if ( html[end] == quote )
					quote = 0;
			} else if ( html[end] == '"' || html[end] == '\'' ) {
				quote = html[end];
			} else if ( html[end] == '>' ) {
				break;
			}
		}
		if ( end >= html.size() )
			return false;
		{
			size_t n = p;

			while ( n < end && ( isalnum( (unsigned char)html[n] ) || html[n] == ':' || html[n] == '-' || html[n] == '!' || html[n] == '?' ) )
				n++;
			t.name = HTML_Lower( html.substr( p, n - p ) );
			t.attrs = html.substr( n, end - n );
			t.selfClosing = end > 0 && html[end - 1] == '/';
		}
		t.start = pos;
		t.end = end + 1;
		return true;
	}
	return false;
}

static bool HTML_Void( const std::string &name ) {
	static const char *voids[] = { "br", "img", "input", "hr", "meta", "link", "source", "area", "col", "embed", "wbr", "!doctype", "base" };

	for ( const char *v : voids ) {
		if ( name == v )
			return true;
	}
	return false;
}

std::string HTML_Attr( const std::string &attrs, const char *name ) {
	const std::string lower = HTML_Lower( attrs ), want = name;
	size_t pos = 0;

	while ( ( pos = lower.find( want, pos ) ) != std::string::npos ) {
		size_t p = pos + want.size();
		const bool starts = pos == 0 || isspace( (unsigned char)lower[pos - 1] );

		while ( p < lower.size() && isspace( (unsigned char)lower[p] ) )
			p++;
		if ( !starts || p >= lower.size() || lower[p] != '=' ) {
			pos++;
			continue;
		}
		p++;
		while ( p < lower.size() && isspace( (unsigned char)lower[p] ) )
			p++;
		if ( p < attrs.size() && ( attrs[p] == '"' || attrs[p] == '\'' ) ) {
			const size_t close = attrs.find( attrs[p], p + 1 );

			return HTML_Decode( attrs.substr( p + 1, close == std::string::npos ? std::string::npos : close - p - 1 ) );
		}
		{
			size_t e = p;

			while ( e < attrs.size() && !isspace( (unsigned char)attrs[e] ) && attrs[e] != '>' )
				e++;
			return HTML_Decode( attrs.substr( p, e - p ) );
		}
	}
	return "";
}

static bool HTML_HasClass( const std::string &attrs, const char *cls ) {
	const std::string classes = " " + HTML_Attr( attrs, "class" ) + " ";

	return classes.find( std::string( " " ) + cls + " " ) != std::string::npos;
}

bool HTML_Find( const std::string &html, size_t pos, const char *tag, const char *cls, htmlElement_t &el ) {
	htmlTag_t t;

	while ( HTML_NextTag( html, pos, t ) ) {
		pos = t.end;
		if ( t.closing || ( tag[0] && t.name != tag ) || ( cls[0] && !HTML_HasClass( t.attrs, cls ) ) )
			continue;
		el.tag = t.name;
		el.attrs = t.attrs;
		el.start = t.start;
		el.innerStart = t.end;
		if ( HTML_Void( t.name ) || t.selfClosing ) {
			el.innerEnd = el.end = t.end;
			return true;
		}
		// its end: the same tag, nested ones counted
		{
			htmlTag_t u;
			size_t p = t.end;
			int depth = 1;

			while ( HTML_NextTag( html, p, u ) ) {
				p = u.end;
				if ( u.name != t.name || HTML_Void( u.name ) )
					continue;
				depth += u.closing ? -1 : u.selfClosing ? 0 : 1;
				if ( !depth ) {
					el.innerEnd = u.start;
					el.end = u.end;
					return true;
				}
			}
			el.innerEnd = el.end = html.size();
		}
		return true;
	}
	return false;
}

std::string HTML_Inner( const std::string &html, const htmlElement_t &el ) {
	return html.substr( el.innerStart, el.innerEnd - el.innerStart );
}

std::string HTML_Text( const std::string &html ) {
	std::string raw;
	htmlTag_t t;
	size_t pos = 0;

	while ( HTML_NextTag( html, pos, t ) ) {
		raw.append( html, pos, t.start - pos );
		raw += ' ';
		pos = t.end;
		// what scripts and styles hold is no text
		if ( !t.closing && ( t.name == "script" || t.name == "style" ) ) {
			const size_t close = html.find( "</" + t.name, pos );

			pos = close == std::string::npos ? html.size() : close;
		}
	}
	raw.append( html, pos, std::string::npos );
	return HTML_Squeeze( HTML_Decode( raw ) );
}

std::string HTML_Url( const char *base, const std::string &href ) {
	std::string b = base ? base : "";
	size_t scheme, root;

	if ( href.empty() )
		return b;
	if ( !href.compare( 0, 7, "http://" ) || !href.compare( 0, 8, "https://" ) )
		return href;
	scheme = b.find( "://" );
	if ( !href.compare( 0, 2, "//" ) )
		return ( scheme == std::string::npos ? "https:" : b.substr( 0, scheme + 1 ) ) + href;
	root = scheme == std::string::npos ? std::string::npos : b.find( '/', scheme + 3 );
	if ( href[0] == '/' )
		return ( root == std::string::npos ? b : b.substr( 0, root ) ) + href;
	// beside the page: its folder, ./ dropped
	{
		const size_t q = b.find_first_of( "?#" );
		std::string folder = b.substr( 0, q );
		const size_t slash = folder.rfind( '/' );
		std::string rel = href;

		if ( slash != std::string::npos && slash > ( scheme == std::string::npos ? 0 : scheme + 2 ) )
			folder = folder.substr( 0, slash + 1 );
		else
			folder += '/';
		while ( !rel.compare( 0, 2, "./" ) )
			rel = rel.substr( 2 );
		return folder + rel;
	}
}

void HTML_Inputs( const std::string &html, std::map<std::string, std::string> &out ) {
	htmlElement_t el;
	size_t pos = 0;

	while ( HTML_Find( html, pos, "input", "", el ) ) {
		const std::string name = HTML_Attr( el.attrs, "name" );

		pos = el.end;
		if ( !name.empty() && !out.count( name ) )
			out[name] = HTML_Attr( el.attrs, "value" );
	}
}

std::string HTML_Param( const std::string &url, const char *name ) {
	const std::string want = std::string( name ) + "=";
	size_t pos = url.find( '?' );

	while ( pos != std::string::npos ) {
		pos++;
		if ( !url.compare( pos, want.size(), want ) ) {
			const size_t end = url.find_first_of( "&#", pos );

			return url.substr( pos + want.size(), end == std::string::npos ? std::string::npos : end - pos - want.size() );
		}
		pos = url.find( '&', pos );
	}
	return "";
}

// the game's Latin-1 as UTF-8, percent-encoded
std::string HTML_FormEncode( const std::string &in ) {
	std::string out;

	for ( unsigned char c : in ) {
		if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' ) {
			out += (char)c;
		} else if ( c == ' ' ) {
			out += '+';
		} else if ( c < 0x80 ) {
			out += va( "%%%02X", c );
		} else {
			out += va( "%%%02X%%%02X", 0xC0 | ( c >> 6 ), 0x80 | ( c & 0x3F ) );
		}
	}
	return out;
}

/*
===============================================================================

Blocks

===============================================================================
*/

typedef struct {
	std::vector<htmlBlock_t>	*out;
	const char					*base;
	std::string					text;		// the paragraph going
	htmlBlockKind_t				kind;
	int							depth;
	bool						pre;		// in code: the lines kept
} htmlFlow_t;

static void HTML_Flush( htmlFlow_t &f ) {
	htmlBlock_t b;

	b.text = f.pre ? HTML_Decode( f.text ) : HTML_Squeeze( HTML_Decode( f.text ) );
	f.text.clear();
	if ( b.text.empty() || ( !f.pre && b.text.find_first_not_of( ' ' ) == std::string::npos ) ) {
		f.kind = HB_TEXT;
		return;
	}
	b.kind = f.pre ? HB_CODE : f.kind;
	b.depth = f.depth;
	f.out->push_back( b );
	f.kind = HB_TEXT;
}

static void HTML_Add( htmlFlow_t &f, htmlBlockKind_t kind, const std::string &text, const std::string &src ) {
	htmlBlock_t b;

	b.kind = kind;
	b.depth = f.depth;
	b.text = text;
	b.src = src;
	f.out->push_back( b );
}

void HTML_Blocks( const std::string &html, const char *base, std::vector<htmlBlock_t> &out ) {
	htmlFlow_t f;
	htmlTag_t t;
	size_t pos = 0;
	static const char *blocks[] = { "p", "div", "section", "article", "header", "footer", "ul", "ol", "table", "tr", "dl", "dd", "dt",
		"figure", "figcaption", "center", "form", "nav", "aside", "main" };

	f.out = &out;
	f.base = base;
	f.kind = HB_TEXT;
	f.depth = 0;
	f.pre = false;
	while ( HTML_NextTag( html, pos, t ) ) {
		std::string between = html.substr( pos, t.start - pos );
		const std::string &n = t.name;

		if ( f.pre ) {
			// code keeps its own line breaks
			f.text += between;
		} else {
			f.text += between;
		}
		pos = t.end;
		if ( n == "script" || n == "style" || n == "noscript" || n == "head" ) {
			if ( !t.closing && !t.selfClosing ) {
				const size_t close = html.find( "</" + n, pos );

				pos = close == std::string::npos ? html.size() : close;
			}
			continue;
		}
		if ( n == "br" ) {
			if ( f.pre )
				f.text += '\n';
			else
				HTML_Flush( f );
			continue;
		}
		if ( n == "img" ) {
			const std::string src = HTML_Attr( t.attrs, "src" ), alt = HTML_Attr( t.attrs, "alt" );

			// smilies and emoji: what they stand for, in the text
			if ( HTML_HasClass( t.attrs, "smilies" ) || HTML_HasClass( t.attrs, "emoji" ) || HTML_HasClass( t.attrs, "wp-smiley" ) ) {
				f.text += alt.empty() ? " " : alt;
				continue;
			}
			HTML_Flush( f );
			if ( !src.empty() && src.compare( 0, 5, "data:" ) )
				HTML_Add( f, HB_IMAGE, alt, HTML_Url( base, src ) );
			continue;
		}
		if ( n == "hr" ) {
			HTML_Flush( f );
			HTML_Add( f, HB_RULE, "", "" );
			continue;
		}
		if ( n == "pre" || n == "code" ) {
			HTML_Flush( f );
			f.pre = !t.closing;
			continue;
		}
		if ( n == "blockquote" ) {
			HTML_Flush( f );
			f.depth = Q_max( 0, f.depth + ( t.closing ? -1 : 1 ) );
			continue;
		}
		if ( n == "cite" ) {
			// phpBB's "Someone wrote:" at a quote's top
			HTML_Flush( f );
			if ( t.closing && !out.empty() && out.back().kind == HB_TEXT && out.back().depth == f.depth )
				out.back().kind = HB_QUOTE_HEAD;
			continue;
		}
		if ( n.size() == 2 && n[0] == 'h' && n[1] >= '1' && n[1] <= '6' ) {
			HTML_Flush( f );
			if ( !t.closing )
				f.kind = HB_HEADING;
			continue;
		}
		if ( n == "li" ) {
			HTML_Flush( f );
			if ( !t.closing )
				f.kind = HB_LIST;
			continue;
		}
		for ( const char *b : blocks ) {
			if ( n == b ) {
				HTML_Flush( f );
				break;
			}
		}
	}
	f.text += html.substr( pos );
	HTML_Flush( f );
}

/*
===============================================================================

RSS

===============================================================================
*/

// a tag's text inside an item: CDATA kept whole, entities decoded
static std::string RSS_Field( const std::string &item, const char *tag, bool html ) {
	const std::string open = std::string( "<" ) + tag, close = std::string( "</" ) + tag + ">";
	size_t a = item.find( open + ">" ), b;
	std::string v;

	if ( a == std::string::npos ) {
		a = item.find( open + " " );
		if ( a == std::string::npos )
			return "";
	}
	a = item.find( '>', a ) + 1;
	b = item.find( close, a );
	if ( b == std::string::npos )
		return "";
	v = item.substr( a, b - a );
	if ( !v.compare( 0, 9, "<![CDATA[" ) ) {
		const size_t e = v.rfind( "]]>" );

		v = v.substr( 9, e == std::string::npos ? std::string::npos : e - 9 );
	} else if ( html ) {
		// escaped HTML: one decoding gives the markup back
		std::string un;

		for ( size_t i = 0; i < v.size(); i++ ) {
			if ( !v.compare( i, 4, "&lt;" ) ) { un += '<'; i += 3; }
			else if ( !v.compare( i, 4, "&gt;" ) ) { un += '>'; i += 3; }
			else if ( !v.compare( i, 6, "&quot;" ) ) { un += '"'; i += 5; }
			else if ( !v.compare( i, 5, "&amp;" ) ) { un += '&'; i += 4; }
			else un += v[i];
		}
		v = un;
	}
	return html ? v : HTML_Squeeze( HTML_Decode( v ) );
}

void RSS_Items( const std::string &xml, std::vector<rssItem_t> &out ) {
	size_t pos = 0;

	out.clear();
	while ( ( pos = xml.find( "<item", pos ) ) != std::string::npos ) {
		const size_t end = xml.find( "</item>", pos );
		const std::string item = xml.substr( pos, end == std::string::npos ? std::string::npos : end - pos );
		rssItem_t r;

		pos = end == std::string::npos ? xml.size() : end + 7;
		r.title = RSS_Field( item, "title", false );
		r.link = RSS_Field( item, "link", false );
		r.author = RSS_Field( item, "dc:creator", false );
		r.category = RSS_Field( item, "category", false );
		r.content = RSS_Field( item, "content:encoded", true );
		if ( r.content.empty() )
			r.content = RSS_Field( item, "description", true );
		// "Thu, 05 Nov 2026 20:46:25 +0000": the day, month and year
		{
			const std::string d = RSS_Field( item, "pubDate", false );
			const size_t comma = d.find( ", " );

			r.date = comma != std::string::npos && d.size() >= comma + 13 ? d.substr( comma + 2, 11 ) : d;
		}
		if ( !r.title.empty() )
			out.push_back( r );
	}
}

/*
===============================================================================

Pictures

===============================================================================
*/

typedef struct {
	std::string		url, path;		// path: in the cache, without its extension
	qhandle_t		shader;
	int				width, height;
	bool			failed, asked;
	int				lastUse;
} htmlPicture_t;

static std::vector<htmlPicture_t>	htmlPictures;
static httpRequest_t				htmlDownloads[HTML_DOWNLOADS];
static int							htmlDownloadOf[HTML_DOWNLOADS] = { -1, -1, -1 };
static bool							htmlCacheChecked;

static unsigned int HTML_Hash( const std::string &s ) {
	unsigned int h = 2166136261u;

	for ( unsigned char c : s )
		h = ( h ^ c ) * 16777619u;
	return h;
}

// a JPEG's or PNG's size from its bytes; false for anything else
static bool HTML_ImageSize( const unsigned char *b, int len, int *w, int *h, const char **ext ) {
	if ( len > 24 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' ) {
		*w = ( b[16] << 24 ) | ( b[17] << 16 ) | ( b[18] << 8 ) | b[19];
		*h = ( b[20] << 24 ) | ( b[21] << 16 ) | ( b[22] << 8 ) | b[23];
		*ext = "png";
		return *w > 0 && *h > 0;
	}
	if ( len > 4 && b[0] == 0xFF && b[1] == 0xD8 ) {
		// the frame's header, past the other segments
		for ( int p = 2; p + 9 < len; ) {
			int marker, size;

			if ( b[p] != 0xFF ) {
				p++;
				continue;
			}
			marker = b[p + 1];
			size = ( b[p + 2] << 8 ) | b[p + 3];
			if ( marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC ) {
				*h = ( b[p + 5] << 8 ) | b[p + 6];
				*w = ( b[p + 7] << 8 ) | b[p + 8];
				*ext = "jpg";
				return *w > 0 && *h > 0;
			}
			p += 2 + size;
		}
	}
	return false;
}

// the cache emptied once it's too big, the first time a picture's wanted
static void HTML_CheckCache( void ) {
	char **files;
	int count, total = 0;

	if ( htmlCacheChecked )
		return;
	htmlCacheChecked = true;
	files = FS_ListFiles( HTML_CACHE_DIR, "", &count );
	for ( int i = 0; i < count; i++ )
		total += Q_max( 0, (int)FS_ReadFile( va( "%s/%s", HTML_CACHE_DIR, files[i] ), NULL ) );
	if ( total > HTML_CACHE_MAX ) {
		for ( int i = 0; i < count; i++ )
			FS_HomeRemove( va( "%s/%s", HTML_CACHE_DIR, files[i] ) );
		Com_DPrintf( "datapad: picture cache emptied (%i KB)\n", total / 1024 );
	}
	FS_FreeFileList( files );
}

// one kept in the cache from before: its own size from the TGA's id field, then the renderer's
static bool HTML_FromCache( htmlPicture_t &p ) {
	void *buf;
	const int len = FS_ReadFile( va( "%s.tga", p.path.c_str() ), &buf );

	if ( len > 26 && buf ) {
		const unsigned char *b = (const unsigned char *)buf;

		if ( b[0] == 8 ) {
			p.width = b[18] | ( b[19] << 8 ) | ( b[20] << 16 ) | ( b[21] << 24 );
			p.height = b[22] | ( b[23] << 8 ) | ( b[24] << 16 ) | ( b[25] << 24 );
			p.shader = re->RegisterShaderNoMip( p.path.c_str() );
		}
	}
	if ( buf )
		FS_FreeFile( buf );
	return p.shader != 0;
}

static int HTML_Pow2( int n ) {
	int p = 1;

	while ( p < n && p < 1024 )
		p <<= 1;
	return p;
}

// a JPEG or PNG as a TGA the renderers all take: scaled to sizes in powers of two (smoothly), 32
// bits, the top row first; its own size kept in the TGA's id field (8 bytes) for its shape
static bool HTML_ToTga( const unsigned char *data, int len, const std::string &path, int *width, int *height ) {
	int w, h, comp;
	unsigned char *rgba = stbi_load_from_memory( data, len, &w, &h, &comp, 4 );
	std::vector<unsigned char> tga;
	int tw, th;

	if ( !rgba )
		return false;
	if ( w <= 0 || h <= 0 || w > 8192 || h > 8192 ) {
		stbi_image_free( rgba );
		return false;
	}
	tw = HTML_Pow2( w );
	th = HTML_Pow2( h );
	tga.assign( 18 + 8, 0 );
	tga[0] = 8;							// the id field: the picture's own size
	tga[2] = 2;							// true colour, not compressed
	tga[12] = tw & 255;
	tga[13] = ( tw >> 8 ) & 255;
	tga[14] = th & 255;
	tga[15] = ( th >> 8 ) & 255;
	tga[16] = 32;
	tga[17] = 0x28;						// 8 bits of alpha, the top row first
	for ( int i = 0; i < 4; i++ ) {
		tga[18 + i] = ( w >> ( i * 8 ) ) & 255;
		tga[22 + i] = ( h >> ( i * 8 ) ) & 255;
	}
	tga.reserve( 26 + tw * th * 4 );
	for ( int y = 0; y < th; y++ ) {
		const float fy = Q_max( 0.0f, ( y + 0.5f ) * h / th - 0.5f );
		const int y0 = Q_min( (int)fy, h - 1 ), y1 = Q_min( y0 + 1, h - 1 );
		const float ay = fy - y0;

		for ( int x = 0; x < tw; x++ ) {
			const float fx = Q_max( 0.0f, ( x + 0.5f ) * w / tw - 0.5f );
			const int x0 = Q_min( (int)fx, w - 1 ), x1 = Q_min( x0 + 1, w - 1 );
			const float ax = fx - x0;
			unsigned char px[4];

			for ( int c = 0; c < 4; c++ ) {
				const float top = rgba[( y0 * w + x0 ) * 4 + c] * ( 1 - ax ) + rgba[( y0 * w + x1 ) * 4 + c] * ax;
				const float bot = rgba[( y1 * w + x0 ) * 4 + c] * ( 1 - ax ) + rgba[( y1 * w + x1 ) * 4 + c] * ax;

				px[c] = (unsigned char)( top * ( 1 - ay ) + bot * ay + 0.5f );
			}
			// TGA keeps blue, green, red, alpha
			tga.push_back( px[2] );
			tga.push_back( px[1] );
			tga.push_back( px[0] );
			tga.push_back( px[3] );
		}
	}
	stbi_image_free( rgba );
	FS_WriteFile( va( "%s.tga", path.c_str() ), tga.data(), (int)tga.size() );
	*width = w;
	*height = h;
	return true;
}

qhandle_t HTML_Picture( const char *url, int *width, int *height ) {
	size_t i;

	for ( i = 0; i < htmlPictures.size() && htmlPictures[i].url != url; i++ )
		;
	if ( i == htmlPictures.size() ) {
		htmlPicture_t p;

		if ( htmlPictures.size() >= HTML_MAX_PICTURES || !url[0] )
			return 0;
		HTML_CheckCache();
		p.url = url;
		p.path = va( HTML_CACHE_DIR "/%08x", HTML_Hash( url ) );
		p.shader = 0;
		p.width = p.height = 0;
		p.failed = p.asked = false;
		if ( !HTML_FromCache( p ) )
			p.shader = 0;
		htmlPictures.push_back( p );
	}
	htmlPictures[i].lastUse = cls.realtime;
	*width = htmlPictures[i].width;
	*height = htmlPictures[i].height;
	return htmlPictures[i].shader;
}

void HTML_PictureFrame( void ) {
	for ( int d = 0; d < HTML_DOWNLOADS; d++ ) {
		httpRequest_t *req = &htmlDownloads[d];

		// one come: kept in the cache, then the renderer's
		if ( HTTP_Poll( req ) && htmlDownloadOf[d] >= 0 && htmlDownloadOf[d] < (int)htmlPictures.size() ) {
			htmlPicture_t &p = htmlPictures[htmlDownloadOf[d]];
			const char *ext;

			if ( req->status == 200 && HTML_ImageSize( (const unsigned char *)HTTP_Answer( req ), req->answerLen, &p.width, &p.height, &ext )
				&& HTML_ToTga( (const unsigned char *)HTTP_Answer( req ), req->answerLen, p.path, &p.width, &p.height ) )
				p.shader = re->RegisterShaderNoMip( p.path.c_str() );
			if ( !p.shader )
				p.failed = true;
			HTTP_Release( req );
			htmlDownloadOf[d] = -1;
		}
		if ( req->state == HTTP_BUSY )
			continue;
		// the next wanted, lately looked at
		for ( size_t i = 0; i < htmlPictures.size(); i++ ) {
			htmlPicture_t &p = htmlPictures[i];
			char error[128];

			if ( p.shader || p.failed || p.asked || cls.realtime - p.lastUse > 2000 )
				continue;
			p.asked = true;
			req->maxAnswer = HTML_PICTURE_BYTES;
			req->headers[0] = '\0';
			req->form = qfalse;
			req->noRedirect = qfalse;
			if ( HTTP_Start( req, p.url.c_str(), NULL, error, sizeof( error ) ) )
				htmlDownloadOf[d] = (int)i;
			else
				p.failed = true;
			break;
		}
	}
}
