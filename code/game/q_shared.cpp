/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software
// q_shared.c -- stateless support routines that are included in each code dll

// leave this at the top for PCH reasons...
#include "common_headers.h"

//#include "q_shared.h"

float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
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
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out ) {
	while ( *in && *in != '.' ) {
		*out++ = *in++;
	}
	*out = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, int maxSize, const char *extension ) {
	char    *src;

	if (path[0])	// or the strlen()-1 stuff gets a bad ptr for blank string
	{
	//
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	//
		src = path + strlen(path) - 1;

		while (*src != '/' && src != path) {
			if ( *src == '.' ) {
				return;                 // it has an extension
			}
			src--;
		}
	}

	if (strlen(path)+strlen(extension) >= (unsigned int)maxSize)
	{
		Com_Printf ("COM_DefaultExtension: overflow adding %s to %s\n", extension, path);
	}
	else
	{
		strcat(path, extension);
	}
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static float	(*_BigFloat) (float l);
static float	(*_LittleFloat) (float l);

#ifdef id386 // _M_IX86
//
// optimised stuff for Intel, since most of our data is in that format anyway...
//
short	BigShort(short l){return _BigShort(l);}
int		BigLong (int l) {return _BigLong(l);}
float	BigFloat (float l) {return _BigFloat(l);}
//short	LittleShort(short l) {return _LittleShort(l);}	// these are now macros in q_shared.h
//int		LittleLong (int l) {return _LittleLong(l);}	//
//float	LittleFloat (float l) {return _LittleFloat(l);}	//
//
#else
//
// standard smart-swap code...
//
short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
float	BigFloat (float l) {return _BigFloat(l);}
float	LittleFloat (float l) {return _LittleFloat(l);}
//
#endif

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

float FloatSwap (float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
================
Swap_Init
================
*/
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


/*
============================================================================

PARSING

============================================================================
*/

static	char	com_token[MAX_TOKEN_CHARS];
//JLFCALLOUT MPNOTUSED
//#include functionality for files
int parseDataCount = -1;
parseData_t parseData[2];

void COM_ParseInit( void )
{
	memset(&(parseData[0]),0,sizeof(parseData_t));
	memset(&(parseData[1]),0,sizeof(parseData_t));
	COM_BeginParseSession();
}

#ifdef _XBOX
void COM_BeginParseSession( bool nested )
{
	if (nested)
		parseDataCount =1;
	else
		parseDataCount = 0;
	parseData[parseDataCount].com_lines = 1;
	
}
#else
void COM_BeginParseSession( void )
{
	parseDataCount =0;
	parseData[parseDataCount].com_lines = 1;
}

#endif

void COM_EndParseSession( void )
{
	// FIXME: I have no idea if I'm doing this right ( :D... )
	parseDataCount = -1;
}

int COM_GetCurrentParseLine( int index )
{
	if(parseDataCount < 0)
		Com_Error(ERR_FATAL, "COM_GetCurrentParseLine: parseDataCount < 0 (be sure to call COM_BeginParseSession!)");

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

	while( (c = *data) <= ' ') 
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

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

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
			while (*data && *data != '\n')	// Advance to the end of the line
			{
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) )	// Advance to the */ characters
			{
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
			if (len < MAX_TOKEN_CHARS-1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS-1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if ( c == '\n' )
		{
			parseData[parseDataCount].com_lines++;
		}
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
==============
COM_Compress
remove blank space and comments from source
==============
*/

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
	}
	*out = 0;
	return out - data_p;
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

//Raz: Patched version of Com_sprintf
//Ensiform: But this is better
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
	Q_vsnprintf( buf, MAX_VA_STRING-1, format, argptr );
	va_end( argptr );
	return buf;
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

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
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

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Printf ("Can't use keys or values with a \\(%s, %s)\n",key,value);
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Printf ("Can't use keys or values with a semicolon(%s, %s)\n",key,value);
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Printf ("Can't use keys or values with a \"(%s, %s)\n",key,value);
		return;
	}

	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	strcat (s, newi);
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

#define VALIDSTRING( a )	( ( a != NULL ) && ( a[0] != '\0' ) )

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

/*
===============
COM_ParseString
===============
*/
qboolean COM_ParseString( const char **data, const char **s ) 
{
//	*s = COM_ParseExt( data, qtrue );
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



// end

