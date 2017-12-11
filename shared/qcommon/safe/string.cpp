#include "string.h"
#include "sscanf.h"

#include <cctype>
#include <algorithm>

namespace Q
{
	Ordering stricmp( const gsl::cstring_view& lhs, const gsl::cstring_view& rhs ) NOEXCEPT
	{
		auto lIt = lhs.begin();
		auto rIt = rhs.begin();
		auto lEnd = lhs.end();
		auto rEnd = rhs.end();
		while( lIt != lEnd )
		{
			if( rIt == rEnd )
			{
				// rhs is prefix of lhs
				return Ordering::GT;
			}
			if( std::tolower( *lIt ) < std::tolower( *rIt ) )
			{
				return Ordering::LT;
			}
			if( std::tolower( *lIt ) > std::tolower( *rIt ) )
			{
				return Ordering::GT;
			}
			++lIt;
			++rIt;
		}
		if( rIt == rEnd )
		{
			// lhs == rhs
			return Ordering::EQ;
		}
		// lhs is a prefix of rhs
		return Ordering::LT;
	}

	gsl::cstring_view substr( const gsl::cstring_view& lhs, const std::string::size_type pos, const std::string::size_type count )
	{
		if( pos > lhs.size() )
		{
			throw std::out_of_range( "Q::substr called with out-of-bounds pos parameter!" );
		}
		auto start = lhs.begin() + pos;
		auto end = count == std::string::npos ? lhs.end() : std::min( start + count, lhs.end() );
		gsl::cstring_view result{ start, end };
		return result;
	}

	int svtoi( const gsl::cstring_view& view )
	{
		int result = 0;
		Q::sscanf( view, result );
		return result;
	}

	float svtof( const gsl::cstring_view& view )
	{
		float result = 0.f;
		Q::sscanf( view, result );
		return result;
	}
}
