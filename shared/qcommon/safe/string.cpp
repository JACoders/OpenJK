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
		// lhs is a prefix or rhs
		return Ordering::LT;
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
