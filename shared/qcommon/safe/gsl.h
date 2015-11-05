#pragma once

// central point of include to simplify possible future swap for Microsoft's implementation
#include <gsl/gsl-lite.h>

// the default cstring_view constructor from string literals includes the terminating null; this one does not.
#if defined( _MSC_VER ) && _MSC_VER < 1900
// VS2013 needs a workaround for its lack of user-defined literals. Fuck VS2013.
// TODO: eradicate VS2013
// The workaround is using CSTRING_VIEW("literal") instead of "literal"_v (for the time being).
# define CSTRING_VIEW(x) vs2013hack_cstring_view_literal(x)
template< int length >
inline gsl::cstring_view vs2013hack_cstring_view_literal( const char (&str)[length] )
{
	static_assert( length > 0, "CSTRING_VIEW expects a string literal argument." );
	return{ str, str + length - 1 };
}
#else
# define CSTRING_VIEW(x) x ## _v
/** gsl::cstring_view from string literal (without null-termination) */
inline gsl::cstring_view operator"" _v( const char* str, std::size_t length )
{
	return{ str, str + length };
}
#endif
