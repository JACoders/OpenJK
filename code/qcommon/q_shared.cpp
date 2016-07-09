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

// q_shared.c -- stateless support routines that are included in each code dll

#include "../game/common_headers.h"
#include "qcommon/ojk_sg_wrappers.h"
#include "qcommon/ojk_saved_game.h"

int Com_Clampi( int min, int max, int value )
{
	if ( value < min )
	{
		return min;
	}
	if ( value > max )
	{
		return max;
	}
	return value;
}

float Com_Clamp( float min, float max, float value )
{
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

int Com_AbsClampi( int min, int max, int value )
{
	if( value < 0 )
	{
		return Com_Clampi( -max, -min, value );
	}
	else
	{
		return Com_Clampi( min, max, value );
	}
}

float Com_AbsClamp( float min, float max, float value )
{
	if( value < 0.0f )
	{
		return Com_Clamp( -max, -min, value );
	}
	else
	{
		return Com_Clamp( min, max, value );
	}
}


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_GetExtension
============
*/
const char *COM_GetExtension( const char *name )
{
	const char *dot = strrchr(name, '.'), *slash;
	if (dot && (!(slash = strrchr(name, '/')) || slash < dot))
		return dot + 1;
	else
		return "";
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	int inlen, extlen;

	inlen = strlen(in);
	extlen = strlen(ext);

	if(extlen <= inlen)
	{
		in += inlen - extlen;

		if(!Q_stricmp(in, ext))
			return qtrue;
	}

	return qfalse;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
/*
// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static float	(*_BigFloat) (const float *l);
static float	(*_LittleFloat) (const float *l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
float	BigFloat (const float *l) {return _BigFloat(l);}
float	LittleFloat (const float *l) {return _LittleFloat(l);}
*/

void CopyShortSwap( void *dest, void *src )
{
	byte *to = (byte *)dest, *from = (byte *)src;

	to[0] = from[1];
	to[1] = from[0];
}

void CopyLongSwap( void *dest, void *src )
{
	byte *to = (byte *)dest, *from = (byte *)src;

	to[0] = from[3];
	to[1] = from[2];
	to[2] = from[1];
	to[3] = from[0];
}

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

float FloatSwap (const float *f) {
	byteAlias_t out;

	out.f = *f;
	out.ui = LongSwap(out.ui);

	return out.f;
}

float FloatNoSwap (const float *f)
{
	return *f;
}

/*
================
Swap_Init
================
*/
/*
void Swap_Init (void)
{
	byte	swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1)
	{
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}
*/


/*
============================================================================

PARSING

============================================================================
*/

static	char	com_token[MAX_TOKEN_CHARS];
//JLFCALLOUT MPNOTUSED
int parseDataCount = -1;
const int MAX_PARSE_DATA = 5;
parseData_t parseData[MAX_PARSE_DATA];

void COM_ParseInit( void )
{
	memset(parseData, 0, sizeof(parseData));
	parseDataCount = -1;
}

void COM_BeginParseSession( void )
{
	parseDataCount++;
#ifdef _DEBUG
	if ( parseDataCount >= MAX_PARSE_DATA )
	{
		Com_Error (ERR_FATAL, "COM_BeginParseSession: cannot nest more than %d parsing sessions.\n", MAX_PARSE_DATA);
	}
#endif

	parseData[parseDataCount].com_lines = 1;
	parseData[parseDataCount].com_tokenline = 0;
}

void COM_EndParseSession( void )
{
	parseDataCount--;
#ifdef _DEBUG
	assert (parseDataCount >= -1 && "COM_EndParseSession: called without a starting COM_BeginParseSession.\n");
#endif
}

int COM_GetCurrentParseLine( int index )
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "COM_GetCurrentParseLine: parseDataCount < 0 (be sure to call COM_BeginParseSession!)");

	if ( parseData[parseDataCount].com_tokenline )
		return parseData[parseDataCount].com_tokenline;

	return parseData[parseDataCount].com_lines;
}

char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
const char *SkipWhitespace( const char *data, qboolean *hasNewLines )
{
	int c;

	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "SkipWhitespace: parseDataCount < 0");

	while( (c = *(const unsigned char* /*eurofix*/)data) <= ' ')
	{
		if( !c )
		{
			return NULL;
		}
		if( c == '\n' )
		{
			parseData[parseDataCount].com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
			// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) )
					in++;
				if ( *in )
					in += 2;
				// record when we hit a newline
			} else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			} else if ( c == ' ' || c == '\t') {
				whitespace = qtrue;
				in++;
				// an actual token
			} else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace) {
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = c;
						in++;
					}
				} else {
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	if(parseDataCount >= 0)
		parseData[parseDataCount].com_tokenline = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "COM_ParseExt: parseDataCount < 0 (be sure to call COM_BeginParseSession!)");

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' )
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				if ( *data == '\n' )
				{
					parseData[parseDataCount].com_lines++;
				}
				data++;
			}
			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// token starts on this line
	parseData[parseDataCount].com_tokenline = parseData[parseDataCount].com_lines;

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( c == '\n' )
			{
				parseData[parseDataCount].com_lines++;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
===============
COM_ParseString
===============
*/
qboolean COM_ParseString( const char **data, const char **s )
{
	*s = COM_ParseExt( data, qfalse );
	if ( s[0] == 0 )
	{
		Com_Printf("unexpected EOF in COM_ParseString\n");
		return qtrue;
	}
	return qfalse;
}

/*
===============
COM_ParseInt
===============
*/
qboolean COM_ParseInt( const char **data, int *i )
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 )
	{
		Com_Printf( "unexpected EOF in COM_ParseInt\n" );
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

/*
===============
COM_ParseFloat
===============
*/
qboolean COM_ParseFloat( const char **data, float *f )
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 )
	{
		Com_Printf( "unexpected EOF in COM_ParseFloat\n" );
		return qtrue;
	}

	*f = atof( token );
	return qfalse;
}

/*
===============
COM_ParseVec4
===============
*/
qboolean COM_ParseVec4( const char **buffer, vec4_t *c)
{
	int i;
	float f;

	for (i = 0; i < 4; i++)
	{
		if (COM_ParseFloat(buffer, &f))
		{
			return qtrue;
		}
		(*c)[i] = f;
	}
	return qfalse;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( const char **buf_p, const char *match ) {
	const char	*token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) )
	{
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}


/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection ( const char **program) {
	const char			*token;
	int				depth=0;

	if (com_token[0]=='{') {	//for tr_shader which just ate the brace
		depth = 1;
	}
	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;

			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}

	} while (depth && *program);
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine ( const char **data ) {
	const char	*p;
	int		c;

	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "SkipRestOfLine: parseDataCount < 0");

	p = *data;

	if ( !*p )
		return;

	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			parseData[parseDataCount].com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix ( const char **buf_p, int x, float *m) {
	const char	*token;
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i] = atof(token);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix ( const char **buf_p, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix ( const char **buf_p, int z, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	COM_MatchToken( buf_p, ")" );
}

/*
 ===================
 Com_HexStrToInt
 ===================
 */
int Com_HexStrToInt( const char *str )
{
	if ( !str || !str[ 0 ] )
		return -1;

	// check for hex code
	if( str[ 0 ] == '0' && str[ 1 ] == 'x' )
	{
		size_t i, n = 0;

		for( i = 2; i < strlen( str ); i++ )
		{
			char digit;

			n *= 16;

			digit = tolower( str[ i ] );

			if( digit >= '0' && digit <= '9' )
				digit -= '0';
			else if( digit >= 'a' && digit <= 'f' )
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}

		return n;
	}

	return -1;
}


/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

qboolean Q_isanumber( const char *s )
{
	char *p;
	double ret;

	if( *s == '\0' )
		return qfalse;

	ret = strtod( s, &p );

	if ( ret == HUGE_VAL || errno == ERANGE )
		return qfalse;

	return (qboolean)(*p == '\0');
}

qboolean Q_isintegral( float f )
{
	return (qboolean)( (int)f == f );
}

/*
char* Q_strrchr( const char* string, int c )
{
	char cc = c;
	char *s;
	char *sp=(char *)0;

	s = (char*)string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}
*/
/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize, qboolean bBarfIfTooLong/* = qfalse */ )
{
	if ( !dest ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
	}
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" );
	}

	if (bBarfIfTooLong)
	{
		if ( strlen(src)+1 > (size_t)destsize)
		{
			Com_Error(ERR_FATAL,"String dest buffer too small to hold string \"%s\" %d > %d\n(source addr = %x, dest addr = %x",src, strlen(src)+1, destsize, src, dest);
		}
	}
	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}
#if 1
int Q_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

	if ( s1 == NULL ) {
		if ( s2 == NULL )
			return 0;
		else
			return -1;
	}
	else if ( s2==NULL )
		return 1;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

int Q_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = toupper(*s);
		s++;
	}
    return s1;
}
#endif

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	if ( strlen(src)+1 > (size_t)(size - l1))
	{	//do the error here instead of in Q_strncpyz to get a meaningful msg
		Com_Error(ERR_FATAL,"Q_strcat: cannot append \"%s\" to \"%s\"", src, dest);
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find )
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0)
  {
    if (c >= 'a' && c <= 'z')
    {
      c -= ('a' - 'A');
    }
    len = strlen(find);
    do
    {
      do
      {
        if ((sc = *s++) == 0)
          return NULL;
        if (sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
      } while (sc != c);
    } while (Q_stricmpn(s, find, len) != 0);
    s--;
  }
  return s;
}

int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
==================
Q_StripColor

Strips coloured strings in-place using multiple passes: "fgs^^56fds" -> "fgs^6fds" -> "fgsfds"

This function modifies INPUT (is mutable)

(Also strips ^8 and ^9)
==================
*/
void Q_StripColor(char *text)
{
	qboolean doPass = qtrue;
	char *read;
	char *write;

	while ( doPass )
	{
		doPass = qfalse;
		read = write = text;
		while ( *read )
		{
			if ( Q_IsColorStringExt(read) )
			{
				doPass = qtrue;
				read += 2;
			}
			else
			{
				// Avoid writing the same data over itself
				if (write != read)
				{
					*write = *read;
				}
				write++;
				read++;
			}
		}
		if ( write < read )
		{
			// Add trailing NUL byte if string has shortened
			*write = '\0';
		}
	}
}

/*
Q_strstrip

	Description:	Replace strip[x] in string with repl[x] or remove characters entirely
	Mutates:		string
	Return:			--

	Examples:		Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
*/

void Q_strstrip( char *string, const char *strip, const char *repl )
{
	char		*out=string, *p=string, c;
	const char	*s=strip;
	int			replaceLen = repl?strlen( repl ):0, offset=0;
	qboolean	recordChar = qtrue;

	while ( (c = *p++) != '\0' )
	{
		recordChar = qtrue;
		for ( s=strip; *s; s++ )
		{
			offset = s-strip;
			if ( c == *s )
			{
				if ( !repl || offset >= replaceLen )
					recordChar = qfalse;
				else
					c = repl[offset];
				break;
			}
		}
		if ( recordChar )
			*out++ = c;
	}
	*out = '\0';
}

/*
Q_strchrs

	Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
	Mutates:		--
	Return:			first instance of any characters found
					 otherwise NULL
*/

const char *Q_strchrs( const char *string, const char *search )
{
	const char *p = string, *s = search;

	while ( *p != '\0' )
	{
		for ( s=search; *s; s++ )
		{
			if ( *p == *s )
				return p;
		}
		p++;
	}

	return NULL;
}

#ifdef _MSC_VER
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);

	if(retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}
#endif

int QDECL Com_sprintf( char *dest, int size, const char *fmt, ...) {
	int		len;
	va_list		argptr;

	va_start (argptr,fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end (argptr);

	if(len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1);

	return len;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define	MAX_VA_STRING	32000
#define MAX_VA_BUFFERS 4

char * QDECL va( const char *format, ... )
{
	va_list		argptr;
	static char	string[MAX_VA_BUFFERS][MAX_VA_STRING];	// in case va is called by nested functions
	static int	index = 0;
	char		*buf;

	va_start( argptr, format );
	buf = (char *)&string[index++ & 3];
	Q_vsnprintf( buf, sizeof(*string), format, argptr );
	va_end( argptr );
	return buf;
}

/*
============
Com_TruncateLongString

Assumes buffer is atleast TRUNCATE_LENGTH big
============
*/
void Com_TruncateLongString( char *buffer, const char *s ) {
	int length = strlen( s );

	if ( length <= TRUNCATE_LENGTH )
		Q_strncpyz( buffer, s, TRUNCATE_LENGTH );
	else {
		Q_strncpyz( buffer, s, (TRUNCATE_LENGTH/2) - 3 );
		Q_strcat( buffer, TRUNCATE_LENGTH, " ... " );
		Q_strcat( buffer, TRUNCATE_LENGTH, s + length - (TRUNCATE_LENGTH/2) + 3 );
	}
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
const char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[MAX_INFO_KEY];
	static	char value[2][MAX_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;

	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE] ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		//OJKNOTE: static analysis pointed out pkey may not be null-terminated
		if (!strcmp (key, pkey) )
		{
			memmove(start, s, strlen(s) + 1);	// remove this part
			return;
		}

		if (!*s)
			return;
	}
}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	strcat (newi, s);
	strcpy (s, newi);
}

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset( char c, char *set ) {
	size_t i;

	for ( i=0; i<strlen( set ); i++ ) {
		if ( set[i] == c )
			return qtrue;
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char *Com_SkipCharset( char *s, char *sep ) {
	char *p = s;

	while ( p ) {
		if ( Com_CharIsOneOfCharset( *p, sep ) )
			p++;
		else
			break;
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char *Com_SkipTokens( char *s, int numTokens, char *sep ) {
	int sepCount = 0;
	char *p = s;

	while ( sepCount < numTokens ) {
		if ( Com_CharIsOneOfCharset( *p++, sep ) ) {
			sepCount++;
			while ( Com_CharIsOneOfCharset( *p, sep ) )
				p++;
		}
		else if ( *p == '\0' )
			break;
	}

	if ( sepCount == numTokens )
		return p;
	else
		return s;
}


/*
========================================================================

String ID Tables

========================================================================
*/


/*
-------------------------
GetIDForString
-------------------------
*/

int GetIDForString ( const stringID_table_t *table, const char *string )
{
	int	index = 0;

	while ( VALIDSTRING( table[index].name ) )
	{
		if ( !Q_stricmp( table[index].name, string ) )
			return table[index].id;

		index++;
	}

	return -1;
}

/*
-------------------------
GetStringForID
-------------------------
*/

const char *GetStringForID( const stringID_table_t *table, int id )
{
	int	index = 0;

	while ( VALIDSTRING( table[index].name ) )
	{
		if ( table[index].id == id )
			return table[index].name;

		index++;
	}

	return NULL;
}


void cplane_t::sg_export(
    SgType& dst) const
{
    ::sg_export(normal, dst.normal);
    ::sg_export(dist, dst.dist);
    ::sg_export(type, dst.type);
    ::sg_export(signbits, dst.signbits);
    ::sg_export(pad, dst.pad);
}

void cplane_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.normal, normal);
    ::sg_import(src.dist, dist);
    ::sg_import(src.type, type);
    ::sg_import(src.signbits, signbits);
    ::sg_import(src.pad, pad);
}


void trace_t::sg_export(
    SgType& dst) const
{
    ::sg_export(allsolid, dst.allsolid);
    ::sg_export(startsolid, dst.startsolid);
    ::sg_export(fraction, dst.fraction);
    ::sg_export(endpos, dst.endpos);
    ::sg_export(plane, dst.plane);
    ::sg_export(surfaceFlags, dst.surfaceFlags);
    ::sg_export(contents, dst.contents);
    ::sg_export(entityNum, dst.entityNum);
    ::sg_export(G2CollisionMap, dst.G2CollisionMap);
}

void trace_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.allsolid, allsolid);
    ::sg_import(src.startsolid, startsolid);
    ::sg_import(src.fraction, fraction);
    ::sg_import(src.endpos, endpos);
    ::sg_import(src.plane, plane);
    ::sg_import(src.surfaceFlags, surfaceFlags);
    ::sg_import(src.contents, contents);
    ::sg_import(src.entityNum, entityNum);
    ::sg_import(src.G2CollisionMap, G2CollisionMap);
}


void saberTrail_t::sg_export(
    SgType& dst) const
{
    ::sg_export(inAction, dst.inAction);
    ::sg_export(duration, dst.duration);
    ::sg_export(lastTime, dst.lastTime);
    ::sg_export(base, dst.base);
    ::sg_export(tip, dst.tip);
    ::sg_export(haveOldPos, dst.haveOldPos);
    ::sg_export(oldPos, dst.oldPos);
    ::sg_export(oldNormal, dst.oldNormal);
}

void saberTrail_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.inAction, inAction);
    ::sg_import(src.duration, duration);
    ::sg_import(src.lastTime, lastTime);
    ::sg_import(src.base, base);
    ::sg_import(src.tip, tip);
    ::sg_import(src.haveOldPos, haveOldPos);
    ::sg_import(src.oldPos, oldPos);
    ::sg_import(src.oldNormal, oldNormal);
}


void bladeInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(active, dst.active);
    ::sg_export(color, dst.color);
    ::sg_export(radius, dst.radius);
    ::sg_export(length, dst.length);
    ::sg_export(lengthMax, dst.lengthMax);
    ::sg_export(lengthOld, dst.lengthOld);
    ::sg_export(muzzlePoint, dst.muzzlePoint);
    ::sg_export(muzzlePointOld, dst.muzzlePointOld);
    ::sg_export(muzzleDir, dst.muzzleDir);
    ::sg_export(muzzleDirOld, dst.muzzleDirOld);
    ::sg_export(trail, dst.trail);
}

void bladeInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.active, active);
    ::sg_import(src.color, color);
    ::sg_import(src.radius, radius);
    ::sg_import(src.length, length);
    ::sg_import(src.lengthMax, lengthMax);
    ::sg_import(src.lengthOld, lengthOld);
    ::sg_import(src.muzzlePoint, muzzlePoint);
    ::sg_import(src.muzzlePointOld, muzzlePointOld);
    ::sg_import(src.muzzleDir, muzzleDir);
    ::sg_import(src.muzzleDirOld, muzzleDirOld);
    ::sg_import(src.trail, trail);
}


void saberInfo_t::sg_export(
    SgType& dst) const
{
    ::sg_export(name, dst.name);
    ::sg_export(fullName, dst.fullName);
    ::sg_export(type, dst.type);
    ::sg_export(model, dst.model);
    ::sg_export(skin, dst.skin);
    ::sg_export(soundOn, dst.soundOn);
    ::sg_export(soundLoop, dst.soundLoop);
    ::sg_export(soundOff, dst.soundOff);
    ::sg_export(numBlades, dst.numBlades);
    ::sg_export(blade, dst.blade);
    ::sg_export(stylesLearned, dst.stylesLearned);
    ::sg_export(stylesForbidden, dst.stylesForbidden);
    ::sg_export(maxChain, dst.maxChain);
    ::sg_export(forceRestrictions, dst.forceRestrictions);
    ::sg_export(lockBonus, dst.lockBonus);
    ::sg_export(parryBonus, dst.parryBonus);
    ::sg_export(breakParryBonus, dst.breakParryBonus);
    ::sg_export(breakParryBonus2, dst.breakParryBonus2);
    ::sg_export(disarmBonus, dst.disarmBonus);
    ::sg_export(disarmBonus2, dst.disarmBonus2);
    ::sg_export(singleBladeStyle, dst.singleBladeStyle);
    ::sg_export(brokenSaber1, dst.brokenSaber1);
    ::sg_export(brokenSaber2, dst.brokenSaber2);
    ::sg_export(saberFlags, dst.saberFlags);
    ::sg_export(saberFlags2, dst.saberFlags2);
    ::sg_export(spinSound, dst.spinSound);
    ::sg_export(swingSound, dst.swingSound);
    ::sg_export(fallSound, dst.fallSound);
    ::sg_export(moveSpeedScale, dst.moveSpeedScale);
    ::sg_export(animSpeedScale, dst.animSpeedScale);
    ::sg_export(kataMove, dst.kataMove);
    ::sg_export(lungeAtkMove, dst.lungeAtkMove);
    ::sg_export(jumpAtkUpMove, dst.jumpAtkUpMove);
    ::sg_export(jumpAtkFwdMove, dst.jumpAtkFwdMove);
    ::sg_export(jumpAtkBackMove, dst.jumpAtkBackMove);
    ::sg_export(jumpAtkRightMove, dst.jumpAtkRightMove);
    ::sg_export(jumpAtkLeftMove, dst.jumpAtkLeftMove);
    ::sg_export(readyAnim, dst.readyAnim);
    ::sg_export(drawAnim, dst.drawAnim);
    ::sg_export(putawayAnim, dst.putawayAnim);
    ::sg_export(tauntAnim, dst.tauntAnim);
    ::sg_export(bowAnim, dst.bowAnim);
    ::sg_export(meditateAnim, dst.meditateAnim);
    ::sg_export(flourishAnim, dst.flourishAnim);
    ::sg_export(gloatAnim, dst.gloatAnim);
    ::sg_export(bladeStyle2Start, dst.bladeStyle2Start);
    ::sg_export(trailStyle, dst.trailStyle);
    ::sg_export(g2MarksShader, dst.g2MarksShader);
    ::sg_export(g2WeaponMarkShader, dst.g2WeaponMarkShader);
    ::sg_export(hitSound, dst.hitSound);
    ::sg_export(blockSound, dst.blockSound);
    ::sg_export(bounceSound, dst.bounceSound);
    ::sg_export(blockEffect, dst.blockEffect);
    ::sg_export(hitPersonEffect, dst.hitPersonEffect);
    ::sg_export(hitOtherEffect, dst.hitOtherEffect);
    ::sg_export(bladeEffect, dst.bladeEffect);
    ::sg_export(knockbackScale, dst.knockbackScale);
    ::sg_export(damageScale, dst.damageScale);
    ::sg_export(splashRadius, dst.splashRadius);
    ::sg_export(splashDamage, dst.splashDamage);
    ::sg_export(splashKnockback, dst.splashKnockback);
    ::sg_export(trailStyle2, dst.trailStyle2);
    ::sg_export(g2MarksShader2, dst.g2MarksShader2);
    ::sg_export(g2WeaponMarkShader2, dst.g2WeaponMarkShader2);
    ::sg_export(hit2Sound, dst.hit2Sound);
    ::sg_export(block2Sound, dst.block2Sound);
    ::sg_export(bounce2Sound, dst.bounce2Sound);
    ::sg_export(blockEffect2, dst.blockEffect2);
    ::sg_export(hitPersonEffect2, dst.hitPersonEffect2);
    ::sg_export(hitOtherEffect2, dst.hitOtherEffect2);
    ::sg_export(bladeEffect2, dst.bladeEffect2);
    ::sg_export(knockbackScale2, dst.knockbackScale2);
    ::sg_export(damageScale2, dst.damageScale2);
    ::sg_export(splashRadius2, dst.splashRadius2);
    ::sg_export(splashDamage2, dst.splashDamage2);
    ::sg_export(splashKnockback2, dst.splashKnockback2);
}

void saberInfo_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.name, name);
    ::sg_import(src.fullName, fullName);
    ::sg_import(src.type, type);
    ::sg_import(src.model, model);
    ::sg_import(src.skin, skin);
    ::sg_import(src.soundOn, soundOn);
    ::sg_import(src.soundLoop, soundLoop);
    ::sg_import(src.soundOff, soundOff);
    ::sg_import(src.numBlades, numBlades);
    ::sg_import(src.blade, blade);
    ::sg_import(src.stylesLearned, stylesLearned);
    ::sg_import(src.stylesForbidden, stylesForbidden);
    ::sg_import(src.maxChain, maxChain);
    ::sg_import(src.forceRestrictions, forceRestrictions);
    ::sg_import(src.lockBonus, lockBonus);
    ::sg_import(src.parryBonus, parryBonus);
    ::sg_import(src.breakParryBonus, breakParryBonus);
    ::sg_import(src.breakParryBonus2, breakParryBonus2);
    ::sg_import(src.disarmBonus, disarmBonus);
    ::sg_import(src.disarmBonus2, disarmBonus2);
    ::sg_import(src.singleBladeStyle, singleBladeStyle);
    ::sg_import(src.brokenSaber1, brokenSaber1);
    ::sg_import(src.brokenSaber2, brokenSaber2);
    ::sg_import(src.saberFlags, saberFlags);
    ::sg_import(src.saberFlags2, saberFlags2);
    ::sg_import(src.spinSound, spinSound);
    ::sg_import(src.swingSound, swingSound);
    ::sg_import(src.fallSound, fallSound);
    ::sg_import(src.moveSpeedScale, moveSpeedScale);
    ::sg_import(src.animSpeedScale, animSpeedScale);
    ::sg_import(src.kataMove, kataMove);
    ::sg_import(src.lungeAtkMove, lungeAtkMove);
    ::sg_import(src.jumpAtkUpMove, jumpAtkUpMove);
    ::sg_import(src.jumpAtkFwdMove, jumpAtkFwdMove);
    ::sg_import(src.jumpAtkBackMove, jumpAtkBackMove);
    ::sg_import(src.jumpAtkRightMove, jumpAtkRightMove);
    ::sg_import(src.jumpAtkLeftMove, jumpAtkLeftMove);
    ::sg_import(src.readyAnim, readyAnim);
    ::sg_import(src.drawAnim, drawAnim);
    ::sg_import(src.putawayAnim, putawayAnim);
    ::sg_import(src.tauntAnim, tauntAnim);
    ::sg_import(src.bowAnim, bowAnim);
    ::sg_import(src.meditateAnim, meditateAnim);
    ::sg_import(src.flourishAnim, flourishAnim);
    ::sg_import(src.gloatAnim, gloatAnim);
    ::sg_import(src.bladeStyle2Start, bladeStyle2Start);
    ::sg_import(src.trailStyle, trailStyle);
    ::sg_import(src.g2MarksShader, g2MarksShader);
    ::sg_import(src.g2WeaponMarkShader, g2WeaponMarkShader);
    ::sg_import(src.hitSound, hitSound);
    ::sg_import(src.blockSound, blockSound);
    ::sg_import(src.bounceSound, bounceSound);
    ::sg_import(src.blockEffect, blockEffect);
    ::sg_import(src.hitPersonEffect, hitPersonEffect);
    ::sg_import(src.hitOtherEffect, hitOtherEffect);
    ::sg_import(src.bladeEffect, bladeEffect);
    ::sg_import(src.knockbackScale, knockbackScale);
    ::sg_import(src.damageScale, damageScale);
    ::sg_import(src.splashRadius, splashRadius);
    ::sg_import(src.splashDamage, splashDamage);
    ::sg_import(src.splashKnockback, splashKnockback);
    ::sg_import(src.trailStyle2, trailStyle2);
    ::sg_import(src.g2MarksShader2, g2MarksShader2);
    ::sg_import(src.g2WeaponMarkShader2, g2WeaponMarkShader2);
    ::sg_import(src.hit2Sound, hit2Sound);
    ::sg_import(src.block2Sound, block2Sound);
    ::sg_import(src.bounce2Sound, bounce2Sound);
    ::sg_import(src.blockEffect2, blockEffect2);
    ::sg_import(src.hitPersonEffect2, hitPersonEffect2);
    ::sg_import(src.hitOtherEffect2, hitOtherEffect2);
    ::sg_import(src.bladeEffect2, bladeEffect2);
    ::sg_import(src.knockbackScale2, knockbackScale2);
    ::sg_import(src.damageScale2, damageScale2);
    ::sg_import(src.splashRadius2, splashRadius2);
    ::sg_import(src.splashDamage2, splashDamage2);
    ::sg_import(src.splashKnockback2, splashKnockback2);
}


void saberInfoRetail_t::sg_export(
    SgType& dst) const
{
    ::sg_export(name, dst.name);
    ::sg_export(fullName, dst.fullName);
    ::sg_export(type, dst.type);
    ::sg_export(model, dst.model);
    ::sg_export(skin, dst.skin);
    ::sg_export(soundOn, dst.soundOn);
    ::sg_export(soundLoop, dst.soundLoop);
    ::sg_export(soundOff, dst.soundOff);
    ::sg_export(numBlades, dst.numBlades);
    ::sg_export(blade, dst.blade);
    ::sg_export(style, dst.style);
    ::sg_export(maxChain, dst.maxChain);
    ::sg_export(lockable, dst.lockable);
    ::sg_export(throwable, dst.throwable);
    ::sg_export(disarmable, dst.disarmable);
    ::sg_export(activeBlocking, dst.activeBlocking);
    ::sg_export(twoHanded, dst.twoHanded);
    ::sg_export(forceRestrictions, dst.forceRestrictions);
    ::sg_export(lockBonus, dst.lockBonus);
    ::sg_export(parryBonus, dst.parryBonus);
    ::sg_export(breakParryBonus, dst.breakParryBonus);
    ::sg_export(disarmBonus, dst.disarmBonus);
    ::sg_export(singleBladeStyle, dst.singleBladeStyle);
    ::sg_export(singleBladeThrowable, dst.singleBladeThrowable);
    ::sg_export(brokenSaber1, dst.brokenSaber1);
    ::sg_export(brokenSaber2, dst.brokenSaber2);
    ::sg_export(returnDamage, dst.returnDamage);
}

void saberInfoRetail_t::sg_import(
    SgType& src)
{
    ::sg_import(src.name, name);
    ::sg_import(src.fullName, fullName);
    ::sg_import(src.type, type);
    ::sg_import(src.model, model);
    ::sg_import(src.skin, skin);
    ::sg_import(src.soundOn, soundOn);
    ::sg_import(src.soundLoop, soundLoop);
    ::sg_import(src.soundOff, soundOff);
    ::sg_import(src.numBlades, numBlades);
    ::sg_import(src.blade, blade);
    ::sg_import(src.style, style);
    ::sg_import(src.maxChain, maxChain);
    ::sg_import(src.lockable, lockable);
    ::sg_import(src.throwable, throwable);
    ::sg_import(src.disarmable, disarmable);
    ::sg_import(src.activeBlocking, activeBlocking);
    ::sg_import(src.twoHanded, twoHanded);
    ::sg_import(src.forceRestrictions, forceRestrictions);
    ::sg_import(src.lockBonus, lockBonus);
    ::sg_import(src.parryBonus, parryBonus);
    ::sg_import(src.breakParryBonus, breakParryBonus);
    ::sg_import(src.disarmBonus, disarmBonus);
    ::sg_import(src.singleBladeStyle, singleBladeStyle);
    ::sg_import(src.singleBladeThrowable, singleBladeThrowable);
    ::sg_import(src.brokenSaber1, brokenSaber1);
    ::sg_import(src.brokenSaber2, brokenSaber2);
    ::sg_import(src.returnDamage, returnDamage);
}


void playerState_t::sg_export(
    SgType& dst) const
{
    ::sg_export(commandTime, dst.commandTime);
    ::sg_export(pm_type, dst.pm_type);
    ::sg_export(bobCycle, dst.bobCycle);
    ::sg_export(pm_flags, dst.pm_flags);
    ::sg_export(pm_time, dst.pm_time);
    ::sg_export(origin, dst.origin);
    ::sg_export(velocity, dst.velocity);
    ::sg_export(weaponTime, dst.weaponTime);
    ::sg_export(weaponChargeTime, dst.weaponChargeTime);
    ::sg_export(rechargeTime, dst.rechargeTime);
    ::sg_export(gravity, dst.gravity);
    ::sg_export(leanofs, dst.leanofs);
    ::sg_export(friction, dst.friction);
    ::sg_export(speed, dst.speed);
    ::sg_export(delta_angles, dst.delta_angles);
    ::sg_export(groundEntityNum, dst.groundEntityNum);
    ::sg_export(legsAnim, dst.legsAnim);
    ::sg_export(legsAnimTimer, dst.legsAnimTimer);
    ::sg_export(torsoAnim, dst.torsoAnim);
    ::sg_export(torsoAnimTimer, dst.torsoAnimTimer);
    ::sg_export(movementDir, dst.movementDir);
    ::sg_export(eFlags, dst.eFlags);
    ::sg_export(eventSequence, dst.eventSequence);
    ::sg_export(events, dst.events);
    ::sg_export(eventParms, dst.eventParms);
    ::sg_export(externalEvent, dst.externalEvent);
    ::sg_export(externalEventParm, dst.externalEventParm);
    ::sg_export(externalEventTime, dst.externalEventTime);
    ::sg_export(clientNum, dst.clientNum);
    ::sg_export(weapon, dst.weapon);
    ::sg_export(weaponstate, dst.weaponstate);
    ::sg_export(batteryCharge, dst.batteryCharge);
    ::sg_export(viewangles, dst.viewangles);
    ::sg_export(legsYaw, dst.legsYaw);
    ::sg_export(viewheight, dst.viewheight);
    ::sg_export(damageEvent, dst.damageEvent);
    ::sg_export(damageYaw, dst.damageYaw);
    ::sg_export(damagePitch, dst.damagePitch);
    ::sg_export(damageCount, dst.damageCount);
    ::sg_export(stats, dst.stats);
    ::sg_export(persistant, dst.persistant);
    ::sg_export(powerups, dst.powerups);
    ::sg_export(ammo, dst.ammo);
    ::sg_export(inventory, dst.inventory);
    ::sg_export(security_key_message, dst.security_key_message);
    ::sg_export(serverViewOrg, dst.serverViewOrg);
    ::sg_export(saberInFlight, dst.saberInFlight);

#ifdef JK2_MODE
    ::sg_export(saberActive, dst.saberActive);
    ::sg_export(vehicleModel, dst.vehicleModel);
    ::sg_export(viewEntity, dst.viewEntity);
    ::sg_export(saberColor, dst.saberColor);
    ::sg_export(saberLength, dst.saberLength);
    ::sg_export(saberLengthMax, dst.saberLengthMax);
    ::sg_export(forcePowersActive, dst.forcePowersActive);
#else
    ::sg_export(viewEntity, dst.viewEntity);
    ::sg_export(forcePowersActive, dst.forcePowersActive);
#endif

    ::sg_export(useTime, dst.useTime);
    ::sg_export(lastShotTime, dst.lastShotTime);
    ::sg_export(ping, dst.ping);
    ::sg_export(lastOnGround, dst.lastOnGround);
    ::sg_export(lastStationary, dst.lastStationary);
    ::sg_export(weaponShotCount, dst.weaponShotCount);
    ::sg_export(saber, dst.saber);
    ::sg_export(dualSabers, dst.dualSabers);
    ::sg_export(saberMove, dst.saberMove);
    ::sg_export(saberMoveNext, dst.saberMoveNext);
    ::sg_export(saberBounceMove, dst.saberBounceMove);
    ::sg_export(saberBlocking, dst.saberBlocking);
    ::sg_export(saberBlocked, dst.saberBlocked);
    ::sg_export(leanStopDebounceTime, dst.leanStopDebounceTime);

#ifdef JK2_MODE
    ::sg_export(saberLengthOld, dst.saberLengthOld);
#endif

    ::sg_export(saberEntityNum, dst.saberEntityNum);
    ::sg_export(saberEntityDist, dst.saberEntityDist);
    ::sg_export(saberThrowTime, dst.saberThrowTime);
    ::sg_export(saberEntityState, dst.saberEntityState);
    ::sg_export(saberDamageDebounceTime, dst.saberDamageDebounceTime);
    ::sg_export(saberHitWallSoundDebounceTime, dst.saberHitWallSoundDebounceTime);
    ::sg_export(saberEventFlags, dst.saberEventFlags);
    ::sg_export(saberBlockingTime, dst.saberBlockingTime);
    ::sg_export(saberAnimLevel, dst.saberAnimLevel);
    ::sg_export(saberAttackChainCount, dst.saberAttackChainCount);
    ::sg_export(saberLockTime, dst.saberLockTime);
    ::sg_export(saberLockEnemy, dst.saberLockEnemy);
    ::sg_export(saberStylesKnown, dst.saberStylesKnown);

#ifdef JK2_MODE
    ::sg_export(saberModel, dst.saberModel);
#endif

    ::sg_export(forcePowersKnown, dst.forcePowersKnown);
    ::sg_export(forcePowerDuration, dst.forcePowerDuration);
    ::sg_export(forcePowerDebounce, dst.forcePowerDebounce);
    ::sg_export(forcePower, dst.forcePower);
    ::sg_export(forcePowerMax, dst.forcePowerMax);
    ::sg_export(forcePowerRegenDebounceTime, dst.forcePowerRegenDebounceTime);
    ::sg_export(forcePowerRegenRate, dst.forcePowerRegenRate);
    ::sg_export(forcePowerRegenAmount, dst.forcePowerRegenAmount);
    ::sg_export(forcePowerLevel, dst.forcePowerLevel);
    ::sg_export(forceJumpZStart, dst.forceJumpZStart);
    ::sg_export(forceJumpCharge, dst.forceJumpCharge);
    ::sg_export(forceGripEntityNum, dst.forceGripEntityNum);
    ::sg_export(forceGripOrg, dst.forceGripOrg);
    ::sg_export(forceDrainEntityNum, dst.forceDrainEntityNum);
    ::sg_export(forceDrainOrg, dst.forceDrainOrg);
    ::sg_export(forceHealCount, dst.forceHealCount);
    ::sg_export(forceAllowDeactivateTime, dst.forceAllowDeactivateTime);
    ::sg_export(forceRageDrainTime, dst.forceRageDrainTime);
    ::sg_export(forceRageRecoveryTime, dst.forceRageRecoveryTime);
    ::sg_export(forceDrainEntNum, dst.forceDrainEntNum);
    ::sg_export(forceDrainTime, dst.forceDrainTime);
    ::sg_export(forcePowersForced, dst.forcePowersForced);
    ::sg_export(pullAttackEntNum, dst.pullAttackEntNum);
    ::sg_export(pullAttackTime, dst.pullAttackTime);
    ::sg_export(lastKickedEntNum, dst.lastKickedEntNum);
    ::sg_export(taunting, dst.taunting);
    ::sg_export(jumpZStart, dst.jumpZStart);
    ::sg_export(moveDir, dst.moveDir);
    ::sg_export(waterheight, dst.waterheight);
    ::sg_export(waterHeightLevel, dst.waterHeightLevel);
    ::sg_export(ikStatus, dst.ikStatus);
    ::sg_export(heldClient, dst.heldClient);
    ::sg_export(heldByClient, dst.heldByClient);
    ::sg_export(heldByBolt, dst.heldByBolt);
    ::sg_export(heldByBone, dst.heldByBone);
    ::sg_export(vehTurnaroundIndex, dst.vehTurnaroundIndex);
    ::sg_export(vehTurnaroundTime, dst.vehTurnaroundTime);
    ::sg_export(brokenLimbs, dst.brokenLimbs);
    ::sg_export(electrifyTime, dst.electrifyTime);
}

void playerState_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.commandTime, commandTime);
    ::sg_import(src.pm_type, pm_type);
    ::sg_import(src.bobCycle, bobCycle);
    ::sg_import(src.pm_flags, pm_flags);
    ::sg_import(src.pm_time, pm_time);
    ::sg_import(src.origin, origin);
    ::sg_import(src.velocity, velocity);
    ::sg_import(src.weaponTime, weaponTime);
    ::sg_import(src.weaponChargeTime, weaponChargeTime);
    ::sg_import(src.rechargeTime, rechargeTime);
    ::sg_import(src.gravity, gravity);
    ::sg_import(src.leanofs, leanofs);
    ::sg_import(src.friction, friction);
    ::sg_import(src.speed, speed);
    ::sg_import(src.delta_angles, delta_angles);
    ::sg_import(src.groundEntityNum, groundEntityNum);
    ::sg_import(src.legsAnim, legsAnim);
    ::sg_import(src.legsAnimTimer, legsAnimTimer);
    ::sg_import(src.torsoAnim, torsoAnim);
    ::sg_import(src.torsoAnimTimer, torsoAnimTimer);
    ::sg_import(src.movementDir, movementDir);
    ::sg_import(src.eFlags, eFlags);
    ::sg_import(src.eventSequence, eventSequence);
    ::sg_import(src.events, events);
    ::sg_import(src.eventParms, eventParms);
    ::sg_import(src.externalEvent, externalEvent);
    ::sg_import(src.externalEventParm, externalEventParm);
    ::sg_import(src.externalEventTime, externalEventTime);
    ::sg_import(src.clientNum, clientNum);
    ::sg_import(src.weapon, weapon);
    ::sg_import(src.weaponstate, weaponstate);
    ::sg_import(src.batteryCharge, batteryCharge);
    ::sg_import(src.viewangles, viewangles);
    ::sg_import(src.legsYaw, legsYaw);
    ::sg_import(src.viewheight, viewheight);
    ::sg_import(src.damageEvent, damageEvent);
    ::sg_import(src.damageYaw, damageYaw);
    ::sg_import(src.damagePitch, damagePitch);
    ::sg_import(src.damageCount, damageCount);
    ::sg_import(src.stats, stats);
    ::sg_import(src.persistant, persistant);
    ::sg_import(src.powerups, powerups);
    ::sg_import(src.ammo, ammo);
    ::sg_import(src.inventory, inventory);
    ::sg_import(src.security_key_message, security_key_message);
    ::sg_import(src.serverViewOrg, serverViewOrg);
    ::sg_import(src.saberInFlight, saberInFlight);

#ifdef JK2_MODE
    ::sg_import(src.saberActive, saberActive);
    ::sg_import(src.vehicleModel, vehicleModel);
    ::sg_import(src.viewEntity, viewEntity);
    ::sg_import(src.saberColor, saberColor);
    ::sg_import(src.saberLength, saberLength);
    ::sg_import(src.saberLengthMax, saberLengthMax);
    ::sg_import(src.forcePowersActive, forcePowersActive);
#else
    ::sg_import(src.viewEntity, viewEntity);
    ::sg_import(src.forcePowersActive, forcePowersActive);
#endif

    ::sg_import(src.useTime, useTime);
    ::sg_import(src.lastShotTime, lastShotTime);
    ::sg_import(src.ping, ping);
    ::sg_import(src.lastOnGround, lastOnGround);
    ::sg_import(src.lastStationary, lastStationary);
    ::sg_import(src.weaponShotCount, weaponShotCount);
    ::sg_import(src.saber, saber);
    ::sg_import(src.dualSabers, dualSabers);
    ::sg_import(src.saberMove, saberMove);
    ::sg_import(src.saberMoveNext, saberMoveNext);
    ::sg_import(src.saberBounceMove, saberBounceMove);
    ::sg_import(src.saberBlocking, saberBlocking);
    ::sg_import(src.saberBlocked, saberBlocked);
    ::sg_import(src.leanStopDebounceTime, leanStopDebounceTime);

#ifdef JK2_MODE
    ::sg_import(src.saberLengthOld, saberLengthOld);
#endif

    ::sg_import(src.saberEntityNum, saberEntityNum);
    ::sg_import(src.saberEntityDist, saberEntityDist);
    ::sg_import(src.saberThrowTime, saberThrowTime);
    ::sg_import(src.saberEntityState, saberEntityState);
    ::sg_import(src.saberDamageDebounceTime, saberDamageDebounceTime);
    ::sg_import(src.saberHitWallSoundDebounceTime, saberHitWallSoundDebounceTime);
    ::sg_import(src.saberEventFlags, saberEventFlags);
    ::sg_import(src.saberBlockingTime, saberBlockingTime);
    ::sg_import(src.saberAnimLevel, saberAnimLevel);
    ::sg_import(src.saberAttackChainCount, saberAttackChainCount);
    ::sg_import(src.saberLockTime, saberLockTime);
    ::sg_import(src.saberLockEnemy, saberLockEnemy);
    ::sg_import(src.saberStylesKnown, saberStylesKnown);

#ifdef JK2_MODE
    ::sg_import(src.saberModel, saberModel);
#endif

    ::sg_import(src.forcePowersKnown, forcePowersKnown);
    ::sg_import(src.forcePowerDuration, forcePowerDuration);
    ::sg_import(src.forcePowerDebounce, forcePowerDebounce);
    ::sg_import(src.forcePower, forcePower);
    ::sg_import(src.forcePowerMax, forcePowerMax);
    ::sg_import(src.forcePowerRegenDebounceTime, forcePowerRegenDebounceTime);
    ::sg_import(src.forcePowerRegenRate, forcePowerRegenRate);
    ::sg_import(src.forcePowerRegenAmount, forcePowerRegenAmount);
    ::sg_import(src.forcePowerLevel, forcePowerLevel);
    ::sg_import(src.forceJumpZStart, forceJumpZStart);
    ::sg_import(src.forceJumpCharge, forceJumpCharge);
    ::sg_import(src.forceGripEntityNum, forceGripEntityNum);
    ::sg_import(src.forceGripOrg, forceGripOrg);
    ::sg_import(src.forceDrainEntityNum, forceDrainEntityNum);
    ::sg_import(src.forceDrainOrg, forceDrainOrg);
    ::sg_import(src.forceHealCount, forceHealCount);
    ::sg_import(src.forceAllowDeactivateTime, forceAllowDeactivateTime);
    ::sg_import(src.forceRageDrainTime, forceRageDrainTime);
    ::sg_import(src.forceRageRecoveryTime, forceRageRecoveryTime);
    ::sg_import(src.forceDrainEntNum, forceDrainEntNum);
    ::sg_import(src.forceDrainTime, forceDrainTime);
    ::sg_import(src.forcePowersForced, forcePowersForced);
    ::sg_import(src.pullAttackEntNum, pullAttackEntNum);
    ::sg_import(src.pullAttackTime, pullAttackTime);
    ::sg_import(src.lastKickedEntNum, lastKickedEntNum);
    ::sg_import(src.taunting, taunting);
    ::sg_import(src.jumpZStart, jumpZStart);
    ::sg_import(src.moveDir, moveDir);
    ::sg_import(src.waterheight, waterheight);
    ::sg_import(src.waterHeightLevel, waterHeightLevel);
    ::sg_import(src.ikStatus, ikStatus);
    ::sg_import(src.heldClient, heldClient);
    ::sg_import(src.heldByClient, heldByClient);
    ::sg_import(src.heldByBolt, heldByBolt);
    ::sg_import(src.heldByBone, heldByBone);
    ::sg_import(src.vehTurnaroundIndex, vehTurnaroundIndex);
    ::sg_import(src.vehTurnaroundTime, vehTurnaroundTime);
    ::sg_import(src.brokenLimbs, brokenLimbs);
    ::sg_import(src.electrifyTime, electrifyTime);
}


void usercmd_t::sg_export(
    SgType& dst) const
{
    ::sg_export(serverTime, dst.serverTime);
    ::sg_export(buttons, dst.buttons);
    ::sg_export(weapon, dst.weapon);
    ::sg_export(angles, dst.angles);
    ::sg_export(generic_cmd, dst.generic_cmd);
    ::sg_export(forwardmove, dst.forwardmove);
    ::sg_export(rightmove, dst.rightmove);
    ::sg_export(upmove, dst.upmove);
}

void usercmd_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.serverTime, serverTime);
    ::sg_import(src.buttons, buttons);
    ::sg_import(src.weapon, weapon);
    ::sg_import(src.angles, angles);
    ::sg_import(src.generic_cmd, generic_cmd);
    ::sg_import(src.forwardmove, forwardmove);
    ::sg_import(src.rightmove, rightmove);
    ::sg_import(src.upmove, upmove);
}


void trajectory_t::sg_export(
    SgType& dst) const
{
    ::sg_export(trType, dst.trType);
    ::sg_export(trTime, dst.trTime);
    ::sg_export(trDuration, dst.trDuration);
    ::sg_export(trBase, dst.trBase);
    ::sg_export(trDelta, dst.trDelta);
}

void trajectory_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.trType, trType);
    ::sg_import(src.trTime, trTime);
    ::sg_import(src.trDuration, trDuration);
    ::sg_import(src.trBase, trBase);
    ::sg_import(src.trDelta, trDelta);
}


void entityState_t::sg_export(
    SgType& dst) const
{
    ::sg_export(number, dst.number);
    ::sg_export(eType, dst.eType);
    ::sg_export(eFlags, dst.eFlags);
    ::sg_export(pos, dst.pos);
    ::sg_export(apos, dst.apos);
    ::sg_export(time, dst.time);
    ::sg_export(time2, dst.time2);
    ::sg_export(origin, dst.origin);
    ::sg_export(origin2, dst.origin2);
    ::sg_export(angles, dst.angles);
    ::sg_export(angles2, dst.angles2);
    ::sg_export(otherEntityNum, dst.otherEntityNum);
    ::sg_export(otherEntityNum2, dst.otherEntityNum2);
    ::sg_export(groundEntityNum, dst.groundEntityNum);
    ::sg_export(constantLight, dst.constantLight);
    ::sg_export(loopSound, dst.loopSound);
    ::sg_export(modelindex, dst.modelindex);
    ::sg_export(modelindex2, dst.modelindex2);
    ::sg_export(modelindex3, dst.modelindex3);
    ::sg_export(clientNum, dst.clientNum);
    ::sg_export(frame, dst.frame);
    ::sg_export(solid, dst.solid);
    ::sg_export(event, dst.event);
    ::sg_export(eventParm, dst.eventParm);
    ::sg_export(powerups, dst.powerups);
    ::sg_export(weapon, dst.weapon);
    ::sg_export(legsAnim, dst.legsAnim);
    ::sg_export(legsAnimTimer, dst.legsAnimTimer);
    ::sg_export(torsoAnim, dst.torsoAnim);
    ::sg_export(torsoAnimTimer, dst.torsoAnimTimer);
    ::sg_export(scale, dst.scale);
    ::sg_export(saberInFlight, dst.saberInFlight);
    ::sg_export(saberActive, dst.saberActive);

#ifdef JK2_MODE
    ::sg_export(vehicleModel, dst.vehicleModel);
#endif

    ::sg_export(vehicleAngles, dst.vehicleAngles);
    ::sg_export(vehicleArmor, dst.vehicleArmor);
    ::sg_export(m_iVehicleNum, dst.m_iVehicleNum);
    ::sg_export(modelScale, dst.modelScale);
    ::sg_export(radius, dst.radius);
    ::sg_export(boltInfo, dst.boltInfo);
    ::sg_export(isPortalEnt, dst.isPortalEnt);
}

void entityState_t::sg_import(
    const SgType& src)
{
    ::sg_import(src.number, number);
    ::sg_import(src.eType, eType);
    ::sg_import(src.eFlags, eFlags);
    ::sg_import(src.pos, pos);
    ::sg_import(src.apos, apos);
    ::sg_import(src.time, time);
    ::sg_import(src.time2, time2);
    ::sg_import(src.origin, origin);
    ::sg_import(src.origin2, origin2);
    ::sg_import(src.angles, angles);
    ::sg_import(src.angles2, angles2);
    ::sg_import(src.otherEntityNum, otherEntityNum);
    ::sg_import(src.otherEntityNum2, otherEntityNum2);
    ::sg_import(src.groundEntityNum, groundEntityNum);
    ::sg_import(src.constantLight, constantLight);
    ::sg_import(src.loopSound, loopSound);
    ::sg_import(src.modelindex, modelindex);
    ::sg_import(src.modelindex2, modelindex2);
    ::sg_import(src.modelindex3, modelindex3);
    ::sg_import(src.clientNum, clientNum);
    ::sg_import(src.frame, frame);
    ::sg_import(src.solid, solid);
    ::sg_import(src.event, event);
    ::sg_import(src.eventParm, eventParm);
    ::sg_import(src.powerups, powerups);
    ::sg_import(src.weapon, weapon);
    ::sg_import(src.legsAnim, legsAnim);
    ::sg_import(src.legsAnimTimer, legsAnimTimer);
    ::sg_import(src.torsoAnim, torsoAnim);
    ::sg_import(src.torsoAnimTimer, torsoAnimTimer);
    ::sg_import(src.scale, scale);
    ::sg_import(src.saberInFlight, saberInFlight);
    ::sg_import(src.saberActive, saberActive);

#ifdef JK2_MODE
    ::sg_import(src.vehicleModel, vehicleModel);
#endif

    ::sg_import(src.vehicleAngles, vehicleAngles);
    ::sg_import(src.vehicleArmor, vehicleArmor);
    ::sg_import(src.m_iVehicleNum, m_iVehicleNum);
    ::sg_import(src.modelScale, modelScale);
    ::sg_import(src.radius, radius);
    ::sg_import(src.boltInfo, boltInfo);
    ::sg_import(src.isPortalEnt, isPortalEnt);
}


// end

