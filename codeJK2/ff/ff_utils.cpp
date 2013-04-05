#include "common_headers.h"

#ifdef _IMMERSION

//#include "ff_utils.h"

extern cvar_t *ff_developer;

//
// Didn't know about strrchr. This is slightly different anyway.
//
int _rcpos( const char* string, char c, int pos )
{
	int length = strlen( string );
	length = ( pos >= 0 && pos < length ? pos : length );
	for ( int i = length - 1; i >= 0; i-- )
		if ( string[i] == c )
			return i;
	return -1;
}

void* LoadFile( const char *filename )
{
	void *buffer;

	int length = FS_ReadFile( filename, &buffer );

	return length != -1 ? buffer : NULL;
}

const char *UncommonDirectory( const char *target, const char *comp )
{
	const char *pos = target;

	for
	(	int i = 0
	;	target[ i ] && comp[ i ] && target[ i ] == comp[ i ]
	;	i++
	){
		if ( target[ i ] == '/' )
			pos = target + i + 1;
	}

	if ( !comp[ i ] && target[ i ] == '/' )
		pos = target + i + 1;
	else
	if ( !target[ i ] && comp[ i ] == '/' )
		pos = target + i;

	return pos;
}

////-------
/// RightOf
//-----------
//
//
//	Parameters:
//
//	Returns:
//
const char* RightOf( const char *str, const char *str2 )
{
	if ( !str || !str2 )
		return NULL;

	const char *s = str;
	int len1 = strlen( str );
	int len2 = strlen( str2 );

	if ( (len2)
	&&	 (len1 >= len2)
	){
		s = strstr( str, str2 );
		if ( s )
		{
			if ( ((s == str) && (*(s + len2) == '/'))
			||	 ((*(s - 1) == '/') && (*(s + len2) == '/'))
			){
				s += len2 + 1;
			}
		}
	}

	return s ? s : str;
}

#ifdef FF_PRINT

void ConsoleParseError( const char *message, const char *line, int pos /*=0*/)
{
	if ( ff_developer && ff_developer->integer )
	{
		Com_Printf( "Parse error: %s\n%s\n%*c\n", message, line, pos + 1, '^' );
	}
}

qboolean FS_VerifyName( const char *src, const char *name, char *out, int maxlen )
{
	return qtrue;
}

#endif // FF_PRINT

#endif // _IMMERSION