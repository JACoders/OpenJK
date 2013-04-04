#ifndef FF_UTILS_H
#define FF_UTILS_H

//#include "ff_public.h"

template<typename Type>
inline Type Clamp( Type arg, Type min, Type max )
{;
	if ( arg <= min )
		return min;
	else 
	if ( arg > max )
		return max;
	return arg;
}

template<typename Type>
inline Type Max( Type arg, Type arg2 )
{
	if ( arg < arg2 )
		return arg2;
	return arg;
}

template<typename Type>
inline Type Min( Type arg, Type arg2 )
{
	if ( arg > arg2 )
		return arg2;
	return arg;
}

template<typename Type>
inline Type InRange( Type arg, Type min, Type max, Type invalid )
{
	if ( arg < min || arg > max )
		return invalid;
	return arg;
}

typedef vector<string> TNameTable;

int _rcpos( const char* string, char c, int pos = -1 );
void* LoadFile( const char *filename );
const char *UncommonDirectory( const char *target, const char *comp );
const char* RightOf( const char *str, const char *str2 );

template< class Type >
void DeletePointer( Type &Pointer, const char *String = 0 )
{
	if ( Pointer )
	{
#ifdef FF_PRINT
		if ( String )
			Com_Printf( "%s\n", String );
#endif
		delete Pointer;
		Pointer = NULL;
	}
}

#ifdef FF_PRINT
void ConsoleParseError( const char *message, const char *line, int pos = 0 );
#endif

qboolean FS_VerifyName( const char *src, const char *name, char *out, int maxlen = FF_MAX_PATH );

//===[multimapIterator]================================================/////////////
//
//	Convenience class for iterating through a multimap. It's not actually
//	all that convenient :(  It's slightly more intuitive than the
//	actual multimap iteration logic.
//
//====================================================================/////////////

template< class T >
class multimapIterator
{
protected:
	typename T::iterator mIt;
	T &mMap;
	typename T::key_type mKey;
public:
	multimapIterator( T &map, typename T::key_type key )
	:	mMap( map )
	,	mKey( key )
	{
		mIt = mMap.find( mKey );
	}
	multimapIterator& operator ++ ()
	{
		if ( mIt != mMap.end() )
		{
			mIt++;
			if ( (*mIt).first != mKey )
				mIt = mMap.end();
		}
		return *this;
	}
	qboolean operator != ( typename T::iterator it )
	{
		return qboolean( mIt != it );
	}
	qboolean operator == ( typename T::iterator it )
	{
		return qboolean( mIt == it );
	}
	typename T::iterator operator * ()
	{
		return mIt;				// must dereference twice to access first and second
	}
	typename T::iterator operator = ( typename T::iterator it )
	{
		mIt = it;
		return mIt;
	}
	operator qboolean ()
	{
		return qboolean( mIt != mMap.end() );
	}
};

/*
template< class T >
class multimapIteratorIterator
{
protected:
	multimapIterator mIt;

public:
	multimapIteratorIterator( T &map )
	:	mIt( map, map.begin() )
	{
	}
	multimapIteratorIterator& operator ++ ()
	{
		for
		(	T::iterator last = *mIt
		;	mIt
		;	last = mIt, ++mIt
		);

		mIt = ++last;

		return *this;
	}
	multimapIterator& operator * ()
	{
		return mIt;
	}
};
*/

#endif // FF_UTILS_H