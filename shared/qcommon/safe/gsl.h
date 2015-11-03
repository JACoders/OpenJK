#pragma once

// central point of include to simplify possible future swap for Microsoft's implementation
#include <gsl/gsl-lite.h>

/** gsl::cstring_view from string literal (without null-termination) */
inline gsl::cstring_view operator"" _v( const char* str, std::size_t length )
{
	return{ str, str + length };
}
