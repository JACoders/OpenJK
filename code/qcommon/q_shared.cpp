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
#include "qcommon/ojk_i_saved_game.h"

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
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<float>(normal);
    saved_game->write<float>(dist);
    saved_game->write<uint8_t>(type);
    saved_game->write<uint8_t>(signbits);
    saved_game->write<uint8_t>(pad);
}

void cplane_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<float>(normal);
    saved_game->read<float>(dist);
    saved_game->read<uint8_t>(type);
    saved_game->read<uint8_t>(signbits);
    saved_game->read<uint8_t>(pad);
}


void trace_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int8_t>(allsolid);
    saved_game->write<int8_t>(startsolid);
    saved_game->write<float>(fraction);
    saved_game->write<float>(endpos);
    saved_game->write<>(plane);
    saved_game->write<int8_t>(surfaceFlags);
    saved_game->write<int8_t>(contents);
    saved_game->write<int8_t>(entityNum);
    saved_game->write<>(G2CollisionMap);
}

void trace_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int8_t>(allsolid);
    saved_game->read<int8_t>(startsolid);
    saved_game->read<float>(fraction);
    saved_game->read<float>(endpos);
    saved_game->read<>(plane);
    saved_game->read<int8_t>(surfaceFlags);
    saved_game->read<int8_t>(contents);
    saved_game->read<int8_t>(entityNum);
    saved_game->read<>(G2CollisionMap);
}


void saberTrail_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(inAction);
    saved_game->write<int32_t>(duration);
    saved_game->write<int32_t>(lastTime);
    saved_game->write<float>(base);
    saved_game->write<float>(tip);
    saved_game->write<int32_t>(haveOldPos);
    saved_game->write<float>(oldPos);
    saved_game->write<float>(oldNormal);
}

void saberTrail_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(inAction);
    saved_game->read<int32_t>(duration);
    saved_game->read<int32_t>(lastTime);
    saved_game->read<float>(base);
    saved_game->read<float>(tip);
    saved_game->read<int32_t>(haveOldPos);
    saved_game->read<float>(oldPos);
    saved_game->read<float>(oldNormal);
}


void bladeInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(active);
    saved_game->write<int32_t>(color);
    saved_game->write<float>(radius);
    saved_game->write<float>(length);
    saved_game->write<float>(lengthMax);
    saved_game->write<float>(lengthOld);
    saved_game->write<float>(muzzlePoint);
    saved_game->write<float>(muzzlePointOld);
    saved_game->write<float>(muzzleDir);
    saved_game->write<float>(muzzleDirOld);
    saved_game->write<>(trail);
}

void bladeInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(active);
    saved_game->read<int32_t>(color);
    saved_game->read<float>(radius);
    saved_game->read<float>(length);
    saved_game->read<float>(lengthMax);
    saved_game->read<float>(lengthOld);
    saved_game->read<float>(muzzlePoint);
    saved_game->read<float>(muzzlePointOld);
    saved_game->read<float>(muzzleDir);
    saved_game->read<float>(muzzleDirOld);
    saved_game->read<>(trail);
}


void saberInfo_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(name);
    saved_game->write<int32_t>(fullName);
    saved_game->write<int32_t>(type);
    saved_game->write<int32_t>(model);
    saved_game->write<int32_t>(skin);
    saved_game->write<int32_t>(soundOn);
    saved_game->write<int32_t>(soundLoop);
    saved_game->write<int32_t>(soundOff);
    saved_game->write<int32_t>(numBlades);
    saved_game->write<>(blade);
    saved_game->write<int32_t>(stylesLearned);
    saved_game->write<int32_t>(stylesForbidden);
    saved_game->write<int32_t>(maxChain);
    saved_game->write<int32_t>(forceRestrictions);
    saved_game->write<int32_t>(lockBonus);
    saved_game->write<int32_t>(parryBonus);
    saved_game->write<int32_t>(breakParryBonus);
    saved_game->write<int32_t>(breakParryBonus2);
    saved_game->write<int32_t>(disarmBonus);
    saved_game->write<int32_t>(disarmBonus2);
    saved_game->write<int32_t>(singleBladeStyle);
    saved_game->write<int32_t>(brokenSaber1);
    saved_game->write<int32_t>(brokenSaber2);
    saved_game->write<int32_t>(saberFlags);
    saved_game->write<int32_t>(saberFlags2);
    saved_game->write<int32_t>(spinSound);
    saved_game->write<int32_t>(swingSound);
    saved_game->write<int32_t>(fallSound);
    saved_game->write<float>(moveSpeedScale);
    saved_game->write<float>(animSpeedScale);
    saved_game->write<int32_t>(kataMove);
    saved_game->write<int32_t>(lungeAtkMove);
    saved_game->write<int32_t>(jumpAtkUpMove);
    saved_game->write<int32_t>(jumpAtkFwdMove);
    saved_game->write<int32_t>(jumpAtkBackMove);
    saved_game->write<int32_t>(jumpAtkRightMove);
    saved_game->write<int32_t>(jumpAtkLeftMove);
    saved_game->write<int32_t>(readyAnim);
    saved_game->write<int32_t>(drawAnim);
    saved_game->write<int32_t>(putawayAnim);
    saved_game->write<int32_t>(tauntAnim);
    saved_game->write<int32_t>(bowAnim);
    saved_game->write<int32_t>(meditateAnim);
    saved_game->write<int32_t>(flourishAnim);
    saved_game->write<int32_t>(gloatAnim);
    saved_game->write<int32_t>(bladeStyle2Start);
    saved_game->write<int32_t>(trailStyle);
    saved_game->write<int8_t>(g2MarksShader);
    saved_game->write<int8_t>(g2WeaponMarkShader);
    saved_game->write<int32_t>(hitSound);
    saved_game->write<int32_t>(blockSound);
    saved_game->write<int32_t>(bounceSound);
    saved_game->write<int32_t>(blockEffect);
    saved_game->write<int32_t>(hitPersonEffect);
    saved_game->write<int32_t>(hitOtherEffect);
    saved_game->write<int32_t>(bladeEffect);
    saved_game->write<float>(knockbackScale);
    saved_game->write<float>(damageScale);
    saved_game->write<float>(splashRadius);
    saved_game->write<int32_t>(splashDamage);
    saved_game->write<float>(splashKnockback);
    saved_game->write<int32_t>(trailStyle2);
    saved_game->write<int8_t>(g2MarksShader2);
    saved_game->write<int8_t>(g2WeaponMarkShader2);
    saved_game->write<int32_t>(hit2Sound);
    saved_game->write<int32_t>(block2Sound);
    saved_game->write<int32_t>(bounce2Sound);
    saved_game->write<int32_t>(blockEffect2);
    saved_game->write<int32_t>(hitPersonEffect2);
    saved_game->write<int32_t>(hitOtherEffect2);
    saved_game->write<int32_t>(bladeEffect2);
    saved_game->write<float>(knockbackScale2);
    saved_game->write<float>(damageScale2);
    saved_game->write<float>(splashRadius2);
    saved_game->write<int32_t>(splashDamage2);
    saved_game->write<float>(splashKnockback2);
}

void saberInfo_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(name);
    saved_game->read<int32_t>(fullName);
    saved_game->read<int32_t>(type);
    saved_game->read<int32_t>(model);
    saved_game->read<int32_t>(skin);
    saved_game->read<int32_t>(soundOn);
    saved_game->read<int32_t>(soundLoop);
    saved_game->read<int32_t>(soundOff);
    saved_game->read<int32_t>(numBlades);
    saved_game->read<>(blade);
    saved_game->read<int32_t>(stylesLearned);
    saved_game->read<int32_t>(stylesForbidden);
    saved_game->read<int32_t>(maxChain);
    saved_game->read<int32_t>(forceRestrictions);
    saved_game->read<int32_t>(lockBonus);
    saved_game->read<int32_t>(parryBonus);
    saved_game->read<int32_t>(breakParryBonus);
    saved_game->read<int32_t>(breakParryBonus2);
    saved_game->read<int32_t>(disarmBonus);
    saved_game->read<int32_t>(disarmBonus2);
    saved_game->read<int32_t>(singleBladeStyle);
    saved_game->read<int32_t>(brokenSaber1);
    saved_game->read<int32_t>(brokenSaber2);
    saved_game->read<int32_t>(saberFlags);
    saved_game->read<int32_t>(saberFlags2);
    saved_game->read<int32_t>(spinSound);
    saved_game->read<int32_t>(swingSound);
    saved_game->read<int32_t>(fallSound);
    saved_game->read<float>(moveSpeedScale);
    saved_game->read<float>(animSpeedScale);
    saved_game->read<int32_t>(kataMove);
    saved_game->read<int32_t>(lungeAtkMove);
    saved_game->read<int32_t>(jumpAtkUpMove);
    saved_game->read<int32_t>(jumpAtkFwdMove);
    saved_game->read<int32_t>(jumpAtkBackMove);
    saved_game->read<int32_t>(jumpAtkRightMove);
    saved_game->read<int32_t>(jumpAtkLeftMove);
    saved_game->read<int32_t>(readyAnim);
    saved_game->read<int32_t>(drawAnim);
    saved_game->read<int32_t>(putawayAnim);
    saved_game->read<int32_t>(tauntAnim);
    saved_game->read<int32_t>(bowAnim);
    saved_game->read<int32_t>(meditateAnim);
    saved_game->read<int32_t>(flourishAnim);
    saved_game->read<int32_t>(gloatAnim);
    saved_game->read<int32_t>(bladeStyle2Start);
    saved_game->read<int32_t>(trailStyle);
    saved_game->read<int8_t>(g2MarksShader);
    saved_game->read<int8_t>(g2WeaponMarkShader);
    saved_game->read<int32_t>(hitSound);
    saved_game->read<int32_t>(blockSound);
    saved_game->read<int32_t>(bounceSound);
    saved_game->read<int32_t>(blockEffect);
    saved_game->read<int32_t>(hitPersonEffect);
    saved_game->read<int32_t>(hitOtherEffect);
    saved_game->read<int32_t>(bladeEffect);
    saved_game->read<float>(knockbackScale);
    saved_game->read<float>(damageScale);
    saved_game->read<float>(splashRadius);
    saved_game->read<int32_t>(splashDamage);
    saved_game->read<float>(splashKnockback);
    saved_game->read<int32_t>(trailStyle2);
    saved_game->read<int8_t>(g2MarksShader2);
    saved_game->read<int8_t>(g2WeaponMarkShader2);
    saved_game->read<int32_t>(hit2Sound);
    saved_game->read<int32_t>(block2Sound);
    saved_game->read<int32_t>(bounce2Sound);
    saved_game->read<int32_t>(blockEffect2);
    saved_game->read<int32_t>(hitPersonEffect2);
    saved_game->read<int32_t>(hitOtherEffect2);
    saved_game->read<int32_t>(bladeEffect2);
    saved_game->read<float>(knockbackScale2);
    saved_game->read<float>(damageScale2);
    saved_game->read<float>(splashRadius2);
    saved_game->read<int32_t>(splashDamage2);
    saved_game->read<float>(splashKnockback2);
}


void saberInfoRetail_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(name);
    saved_game->write<int32_t>(fullName);
    saved_game->write<int32_t>(type);
    saved_game->write<int32_t>(model);
    saved_game->write<int32_t>(skin);
    saved_game->write<int32_t>(soundOn);
    saved_game->write<int32_t>(soundLoop);
    saved_game->write<int32_t>(soundOff);
    saved_game->write<int32_t>(numBlades);
    saved_game->write<>(blade);
    saved_game->write<int32_t>(style);
    saved_game->write<int32_t>(maxChain);
    saved_game->write<int32_t>(lockable);
    saved_game->write<int32_t>(throwable);
    saved_game->write<int32_t>(disarmable);
    saved_game->write<int32_t>(activeBlocking);
    saved_game->write<int32_t>(twoHanded);
    saved_game->write<int32_t>(forceRestrictions);
    saved_game->write<int32_t>(lockBonus);
    saved_game->write<int32_t>(parryBonus);
    saved_game->write<int32_t>(breakParryBonus);
    saved_game->write<int32_t>(disarmBonus);
    saved_game->write<int32_t>(singleBladeStyle);
    saved_game->write<int32_t>(singleBladeThrowable);
    saved_game->write<int32_t>(brokenSaber1);
    saved_game->write<int32_t>(brokenSaber2);
    saved_game->write<int32_t>(returnDamage);
}

void saberInfoRetail_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(name);
    saved_game->read<int32_t>(fullName);
    saved_game->read<int32_t>(type);
    saved_game->read<int32_t>(model);
    saved_game->read<int32_t>(skin);
    saved_game->read<int32_t>(soundOn);
    saved_game->read<int32_t>(soundLoop);
    saved_game->read<int32_t>(soundOff);
    saved_game->read<int32_t>(numBlades);
    saved_game->read<>(blade);
    saved_game->read<int32_t>(style);
    saved_game->read<int32_t>(maxChain);
    saved_game->read<int32_t>(lockable);
    saved_game->read<int32_t>(throwable);
    saved_game->read<int32_t>(disarmable);
    saved_game->read<int32_t>(activeBlocking);
    saved_game->read<int32_t>(twoHanded);
    saved_game->read<int32_t>(forceRestrictions);
    saved_game->read<int32_t>(lockBonus);
    saved_game->read<int32_t>(parryBonus);
    saved_game->read<int32_t>(breakParryBonus);
    saved_game->read<int32_t>(disarmBonus);
    saved_game->read<int32_t>(singleBladeStyle);
    saved_game->read<int32_t>(singleBladeThrowable);
    saved_game->read<int32_t>(brokenSaber1);
    saved_game->read<int32_t>(brokenSaber2);
    saved_game->read<int32_t>(returnDamage);
}


void playerState_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(commandTime);
    saved_game->write<int32_t>(pm_type);
    saved_game->write<int32_t>(bobCycle);
    saved_game->write<int32_t>(pm_flags);
    saved_game->write<int32_t>(pm_time);
    saved_game->write<float>(origin);
    saved_game->write<float>(velocity);
    saved_game->write<int32_t>(weaponTime);
    saved_game->write<int32_t>(weaponChargeTime);
    saved_game->write<int32_t>(rechargeTime);
    saved_game->write<int32_t>(gravity);
    saved_game->write<int32_t>(leanofs);
    saved_game->write<int32_t>(friction);
    saved_game->write<int32_t>(speed);
    saved_game->write<int32_t>(delta_angles);
    saved_game->write<int32_t>(groundEntityNum);
    saved_game->write<int32_t>(legsAnim);
    saved_game->write<int32_t>(legsAnimTimer);
    saved_game->write<int32_t>(torsoAnim);
    saved_game->write<int32_t>(torsoAnimTimer);
    saved_game->write<int32_t>(movementDir);
    saved_game->write<int32_t>(eFlags);
    saved_game->write<int32_t>(eventSequence);
    saved_game->write<int32_t>(events);
    saved_game->write<int32_t>(eventParms);
    saved_game->write<int32_t>(externalEvent);
    saved_game->write<int32_t>(externalEventParm);
    saved_game->write<int32_t>(externalEventTime);
    saved_game->write<int32_t>(clientNum);
    saved_game->write<int32_t>(weapon);
    saved_game->write<int32_t>(weaponstate);
    saved_game->write<int32_t>(batteryCharge);
    saved_game->write<float>(viewangles);
    saved_game->write<float>(legsYaw);
    saved_game->write<int32_t>(viewheight);
    saved_game->write<int32_t>(damageEvent);
    saved_game->write<int32_t>(damageYaw);
    saved_game->write<int32_t>(damagePitch);
    saved_game->write<int32_t>(damageCount);
    saved_game->write<int32_t>(stats);
    saved_game->write<int32_t>(persistant);
    saved_game->write<int32_t>(powerups);
    saved_game->write<int32_t>(ammo);
    saved_game->write<int32_t>(inventory);
    saved_game->write<int8_t>(security_key_message);
    saved_game->write<float>(serverViewOrg);
    saved_game->write<int32_t>(saberInFlight);

#ifdef JK2_MODE
    saved_game->write<int32_t>(saberActive);
    saved_game->write<int32_t>(vehicleModel);
    saved_game->write<int32_t>(viewEntity);
    saved_game->write<int32_t>(saberColor);
    saved_game->write<float>(saberLength);
    saved_game->write<float>(saberLengthMax);
    saved_game->write<int32_t>(forcePowersActive);
#else
    saved_game->write<int32_t>(viewEntity);
    saved_game->write<int32_t>(forcePowersActive);
#endif

    saved_game->write<int32_t>(useTime);
    saved_game->write<int32_t>(lastShotTime);
    saved_game->write<int32_t>(ping);
    saved_game->write<int32_t>(lastOnGround);
    saved_game->write<int32_t>(lastStationary);
    saved_game->write<int32_t>(weaponShotCount);
    saved_game->write<>(saber);
    saved_game->write<int32_t>(dualSabers);
    saved_game->write<int16_t>(saberMove);
    saved_game->write<int16_t>(saberMoveNext);
    saved_game->write<int16_t>(saberBounceMove);
    saved_game->write<int16_t>(saberBlocking);
    saved_game->write<int16_t>(saberBlocked);
    saved_game->write<int16_t>(leanStopDebounceTime);

#ifdef JK2_MODE
    saved_game->write<float>(saberLengthOld);
#endif

    saved_game->write<int32_t>(saberEntityNum);
    saved_game->write<float>(saberEntityDist);
    saved_game->write<int32_t>(saberThrowTime);
    saved_game->write<int32_t>(saberEntityState);
    saved_game->write<int32_t>(saberDamageDebounceTime);
    saved_game->write<int32_t>(saberHitWallSoundDebounceTime);
    saved_game->write<int32_t>(saberEventFlags);
    saved_game->write<int32_t>(saberBlockingTime);
    saved_game->write<int32_t>(saberAnimLevel);
    saved_game->write<int32_t>(saberAttackChainCount);
    saved_game->write<int32_t>(saberLockTime);
    saved_game->write<int32_t>(saberLockEnemy);
    saved_game->write<int32_t>(saberStylesKnown);

#ifdef JK2_MODE
    saved_game->write<int32_t>(saberModel);
#endif

    saved_game->write<int32_t>(forcePowersKnown);
    saved_game->write<int32_t>(forcePowerDuration);
    saved_game->write<int32_t>(forcePowerDebounce);
    saved_game->write<int32_t>(forcePower);
    saved_game->write<int32_t>(forcePowerMax);
    saved_game->write<int32_t>(forcePowerRegenDebounceTime);
    saved_game->write<int32_t>(forcePowerRegenRate);
    saved_game->write<int32_t>(forcePowerRegenAmount);
    saved_game->write<int32_t>(forcePowerLevel);
    saved_game->write<float>(forceJumpZStart);
    saved_game->write<float>(forceJumpCharge);
    saved_game->write<int32_t>(forceGripEntityNum);
    saved_game->write<float>(forceGripOrg);
    saved_game->write<int32_t>(forceDrainEntityNum);
    saved_game->write<float>(forceDrainOrg);
    saved_game->write<int32_t>(forceHealCount);
    saved_game->write<int32_t>(forceAllowDeactivateTime);
    saved_game->write<int32_t>(forceRageDrainTime);
    saved_game->write<int32_t>(forceRageRecoveryTime);
    saved_game->write<int32_t>(forceDrainEntNum);
    saved_game->write<float>(forceDrainTime);
    saved_game->write<int32_t>(forcePowersForced);
    saved_game->write<int32_t>(pullAttackEntNum);
    saved_game->write<int32_t>(pullAttackTime);
    saved_game->write<int32_t>(lastKickedEntNum);
    saved_game->write<int32_t>(taunting);
    saved_game->write<float>(jumpZStart);
    saved_game->write<float>(moveDir);
    saved_game->write<float>(waterheight);
    saved_game->write<int32_t>(waterHeightLevel);
    saved_game->write<int32_t>(ikStatus);
    saved_game->write<int32_t>(heldClient);
    saved_game->write<int32_t>(heldByClient);
    saved_game->write<int32_t>(heldByBolt);
    saved_game->write<int32_t>(heldByBone);
    saved_game->write<int32_t>(vehTurnaroundIndex);
    saved_game->write<int32_t>(vehTurnaroundTime);
    saved_game->write<int32_t>(brokenLimbs);
    saved_game->write<int32_t>(electrifyTime);
}

void playerState_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(commandTime);
    saved_game->read<int32_t>(pm_type);
    saved_game->read<int32_t>(bobCycle);
    saved_game->read<int32_t>(pm_flags);
    saved_game->read<int32_t>(pm_time);
    saved_game->read<float>(origin);
    saved_game->read<float>(velocity);
    saved_game->read<int32_t>(weaponTime);
    saved_game->read<int32_t>(weaponChargeTime);
    saved_game->read<int32_t>(rechargeTime);
    saved_game->read<int32_t>(gravity);
    saved_game->read<int32_t>(leanofs);
    saved_game->read<int32_t>(friction);
    saved_game->read<int32_t>(speed);
    saved_game->read<int32_t>(delta_angles);
    saved_game->read<int32_t>(groundEntityNum);
    saved_game->read<int32_t>(legsAnim);
    saved_game->read<int32_t>(legsAnimTimer);
    saved_game->read<int32_t>(torsoAnim);
    saved_game->read<int32_t>(torsoAnimTimer);
    saved_game->read<int32_t>(movementDir);
    saved_game->read<int32_t>(eFlags);
    saved_game->read<int32_t>(eventSequence);
    saved_game->read<int32_t>(events);
    saved_game->read<int32_t>(eventParms);
    saved_game->read<int32_t>(externalEvent);
    saved_game->read<int32_t>(externalEventParm);
    saved_game->read<int32_t>(externalEventTime);
    saved_game->read<int32_t>(clientNum);
    saved_game->read<int32_t>(weapon);
    saved_game->read<int32_t>(weaponstate);
    saved_game->read<int32_t>(batteryCharge);
    saved_game->read<float>(viewangles);
    saved_game->read<float>(legsYaw);
    saved_game->read<int32_t>(viewheight);
    saved_game->read<int32_t>(damageEvent);
    saved_game->read<int32_t>(damageYaw);
    saved_game->read<int32_t>(damagePitch);
    saved_game->read<int32_t>(damageCount);
    saved_game->read<int32_t>(stats);
    saved_game->read<int32_t>(persistant);
    saved_game->read<int32_t>(powerups);
    saved_game->read<int32_t>(ammo);
    saved_game->read<int32_t>(inventory);
    saved_game->read<int8_t>(security_key_message);
    saved_game->read<float>(serverViewOrg);
    saved_game->read<int32_t>(saberInFlight);

#ifdef JK2_MODE
    saved_game->read<int32_t>(saberActive);
    saved_game->read<int32_t>(vehicleModel);
    saved_game->read<int32_t>(viewEntity);
    saved_game->read<int32_t>(saberColor);
    saved_game->read<float>(saberLength);
    saved_game->read<float>(saberLengthMax);
    saved_game->read<int32_t>(forcePowersActive);
#else
    saved_game->read<int32_t>(viewEntity);
    saved_game->read<int32_t>(forcePowersActive);
#endif

    saved_game->read<int32_t>(useTime);
    saved_game->read<int32_t>(lastShotTime);
    saved_game->read<int32_t>(ping);
    saved_game->read<int32_t>(lastOnGround);
    saved_game->read<int32_t>(lastStationary);
    saved_game->read<int32_t>(weaponShotCount);
    saved_game->read<>(saber);
    saved_game->read<int32_t>(dualSabers);
    saved_game->read<int16_t>(saberMove);
    saved_game->read<int16_t>(saberMoveNext);
    saved_game->read<int16_t>(saberBounceMove);
    saved_game->read<int16_t>(saberBlocking);
    saved_game->read<int16_t>(saberBlocked);
    saved_game->read<int16_t>(leanStopDebounceTime);

#ifdef JK2_MODE
    saved_game->read<float>(saberLengthOld);
#endif

    saved_game->read<int32_t>(saberEntityNum);
    saved_game->read<float>(saberEntityDist);
    saved_game->read<int32_t>(saberThrowTime);
    saved_game->read<int32_t>(saberEntityState);
    saved_game->read<int32_t>(saberDamageDebounceTime);
    saved_game->read<int32_t>(saberHitWallSoundDebounceTime);
    saved_game->read<int32_t>(saberEventFlags);
    saved_game->read<int32_t>(saberBlockingTime);
    saved_game->read<int32_t>(saberAnimLevel);
    saved_game->read<int32_t>(saberAttackChainCount);
    saved_game->read<int32_t>(saberLockTime);
    saved_game->read<int32_t>(saberLockEnemy);
    saved_game->read<int32_t>(saberStylesKnown);

#ifdef JK2_MODE
    saved_game->read<int32_t>(saberModel);
#endif

    saved_game->read<int32_t>(forcePowersKnown);
    saved_game->read<int32_t>(forcePowerDuration);
    saved_game->read<int32_t>(forcePowerDebounce);
    saved_game->read<int32_t>(forcePower);
    saved_game->read<int32_t>(forcePowerMax);
    saved_game->read<int32_t>(forcePowerRegenDebounceTime);
    saved_game->read<int32_t>(forcePowerRegenRate);
    saved_game->read<int32_t>(forcePowerRegenAmount);
    saved_game->read<int32_t>(forcePowerLevel);
    saved_game->read<float>(forceJumpZStart);
    saved_game->read<float>(forceJumpCharge);
    saved_game->read<int32_t>(forceGripEntityNum);
    saved_game->read<float>(forceGripOrg);
    saved_game->read<int32_t>(forceDrainEntityNum);
    saved_game->read<float>(forceDrainOrg);
    saved_game->read<int32_t>(forceHealCount);
    saved_game->read<int32_t>(forceAllowDeactivateTime);
    saved_game->read<int32_t>(forceRageDrainTime);
    saved_game->read<int32_t>(forceRageRecoveryTime);
    saved_game->read<int32_t>(forceDrainEntNum);
    saved_game->read<float>(forceDrainTime);
    saved_game->read<int32_t>(forcePowersForced);
    saved_game->read<int32_t>(pullAttackEntNum);
    saved_game->read<int32_t>(pullAttackTime);
    saved_game->read<int32_t>(lastKickedEntNum);
    saved_game->read<int32_t>(taunting);
    saved_game->read<float>(jumpZStart);
    saved_game->read<float>(moveDir);
    saved_game->read<float>(waterheight);
    saved_game->read<int32_t>(waterHeightLevel);
    saved_game->read<int32_t>(ikStatus);
    saved_game->read<int32_t>(heldClient);
    saved_game->read<int32_t>(heldByClient);
    saved_game->read<int32_t>(heldByBolt);
    saved_game->read<int32_t>(heldByBone);
    saved_game->read<int32_t>(vehTurnaroundIndex);
    saved_game->read<int32_t>(vehTurnaroundTime);
    saved_game->read<int32_t>(brokenLimbs);
    saved_game->read<int32_t>(electrifyTime);
}


void usercmd_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(serverTime);
    saved_game->write<int32_t>(buttons);
    saved_game->write<uint8_t>(weapon);
    saved_game->write<int32_t>(angles);
    saved_game->write<uint8_t>(generic_cmd);
    saved_game->write<int8_t>(forwardmove);
    saved_game->write<int8_t>(rightmove);
    saved_game->write<int8_t>(upmove);
}

void usercmd_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(serverTime);
    saved_game->read<int32_t>(buttons);
    saved_game->read<uint8_t>(weapon);
    saved_game->read<int32_t>(angles);
    saved_game->read<uint8_t>(generic_cmd);
    saved_game->read<int8_t>(forwardmove);
    saved_game->read<int8_t>(rightmove);
    saved_game->read<int8_t>(upmove);
}


void trajectory_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(trType);
    saved_game->write<int32_t>(trTime);
    saved_game->write<int32_t>(trDuration);
    saved_game->write<float>(trBase);
    saved_game->write<float>(trDelta);
}

void trajectory_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(trType);
    saved_game->read<int32_t>(trTime);
    saved_game->read<int32_t>(trDuration);
    saved_game->read<float>(trBase);
    saved_game->read<float>(trDelta);
}


void entityState_t::sg_export(
    ojk::ISavedGame* saved_game) const
{
    saved_game->write<int32_t>(number);
    saved_game->write<int32_t>(eType);
    saved_game->write<int32_t>(eFlags);
    saved_game->write<>(pos);
    saved_game->write<>(apos);
    saved_game->write<int32_t>(time);
    saved_game->write<int32_t>(time2);
    saved_game->write<float>(origin);
    saved_game->write<float>(origin2);
    saved_game->write<float>(angles);
    saved_game->write<float>(angles2);
    saved_game->write<int32_t>(otherEntityNum);
    saved_game->write<int32_t>(otherEntityNum2);
    saved_game->write<int32_t>(groundEntityNum);
    saved_game->write<int32_t>(constantLight);
    saved_game->write<int32_t>(loopSound);
    saved_game->write<int32_t>(modelindex);
    saved_game->write<int32_t>(modelindex2);
    saved_game->write<int32_t>(modelindex3);
    saved_game->write<int32_t>(clientNum);
    saved_game->write<int32_t>(frame);
    saved_game->write<int32_t>(solid);
    saved_game->write<int32_t>(event);
    saved_game->write<int32_t>(eventParm);
    saved_game->write<int32_t>(powerups);
    saved_game->write<int32_t>(weapon);
    saved_game->write<int32_t>(legsAnim);
    saved_game->write<int32_t>(legsAnimTimer);
    saved_game->write<int32_t>(torsoAnim);
    saved_game->write<int32_t>(torsoAnimTimer);
    saved_game->write<int32_t>(scale);
    saved_game->write<int32_t>(saberInFlight);
    saved_game->write<int32_t>(saberActive);

#ifdef JK2_MODE
    saved_game->write<int32_t>(vehicleModel);
#endif

    saved_game->write<float>(vehicleAngles);
    saved_game->write<int32_t>(vehicleArmor);
    saved_game->write<int32_t>(m_iVehicleNum);
    saved_game->write<float>(modelScale);
    saved_game->write<int32_t>(radius);
    saved_game->write<int32_t>(boltInfo);
    saved_game->write<int32_t>(isPortalEnt);
}

void entityState_t::sg_import(
    ojk::ISavedGame* saved_game)
{
    saved_game->read<int32_t>(number);
    saved_game->read<int32_t>(eType);
    saved_game->read<int32_t>(eFlags);
    saved_game->read<>(pos);
    saved_game->read<>(apos);
    saved_game->read<int32_t>(time);
    saved_game->read<int32_t>(time2);
    saved_game->read<float>(origin);
    saved_game->read<float>(origin2);
    saved_game->read<float>(angles);
    saved_game->read<float>(angles2);
    saved_game->read<int32_t>(otherEntityNum);
    saved_game->read<int32_t>(otherEntityNum2);
    saved_game->read<int32_t>(groundEntityNum);
    saved_game->read<int32_t>(constantLight);
    saved_game->read<int32_t>(loopSound);
    saved_game->read<int32_t>(modelindex);
    saved_game->read<int32_t>(modelindex2);
    saved_game->read<int32_t>(modelindex3);
    saved_game->read<int32_t>(clientNum);
    saved_game->read<int32_t>(frame);
    saved_game->read<int32_t>(solid);
    saved_game->read<int32_t>(event);
    saved_game->read<int32_t>(eventParm);
    saved_game->read<int32_t>(powerups);
    saved_game->read<int32_t>(weapon);
    saved_game->read<int32_t>(legsAnim);
    saved_game->read<int32_t>(legsAnimTimer);
    saved_game->read<int32_t>(torsoAnim);
    saved_game->read<int32_t>(torsoAnimTimer);
    saved_game->read<int32_t>(scale);
    saved_game->read<int32_t>(saberInFlight);
    saved_game->read<int32_t>(saberActive);

#ifdef JK2_MODE
    saved_game->read<int32_t>(vehicleModel);
#endif

    saved_game->read<float>(vehicleAngles);
    saved_game->read<int32_t>(vehicleArmor);
    saved_game->read<int32_t>(m_iVehicleNum);
    saved_game->read<float>(modelScale);
    saved_game->read<int32_t>(radius);
    saved_game->read<int32_t>(boltInfo);
    saved_game->read<int32_t>(isPortalEnt);
}


// end

