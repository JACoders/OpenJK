#pragma once

#include "sscanf.h"
#include "gsl.h"
#include "qcommon/q_platform.h"

#include <cassert>
#include <iostream>
#include <string>

namespace Q
{
	enum class Ordering : int
	{
		LT = -1,
		EQ = 0,
		GT = 1
	};
	Ordering stricmp( const gsl::cstring_view& lhs, const gsl::cstring_view& rhs ) NOEXCEPT;
	/// Case-insensitive less comparator for cstring_view; e.g. for case insensitive std::map
	struct CStringViewILess
	{
		bool operator()( const gsl::cstring_view& lhs, const gsl::cstring_view& rhs ) const NOEXCEPT
		{
			return stricmp( lhs, rhs ) == Ordering::LT;
		}
	};

	gsl::cstring_view substr( const gsl::cstring_view& lhs, const std::string::size_type pos = 0, const std::string::size_type count = std::string::npos );

	int svtoi( const gsl::cstring_view& view );
	float svtof( const gsl::cstring_view& view );
}

// operator<< overloads
namespace std
{
	inline std::ostream& operator<<( std::ostream& stream, Q::Ordering ordering )
	{
		switch( ordering )
		{
		case Q::Ordering::EQ:
			return stream << "EQ";
		case Q::Ordering::LT:
			return stream << "LT";
		case Q::Ordering::GT:
			return stream << "GT";
		default:
			assert( false );
			return stream;
		}
	}
}
