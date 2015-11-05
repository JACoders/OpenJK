#include "string.h"

#include <cctype>
#include <algorithm>

namespace Q
{
	Ordering stricmp( const gsl::cstring_view& lhs, const gsl::cstring_view& rhs )
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
		auto end = view.end();
		// skip whitespace
		auto it = std::find_if_not( view.begin(), end, std::isspace );
		if( it == end )
		{
			return 0;
		}
		bool negate = false;
		if( *it == '+' )
		{
			++it;
		}
		else if( *it == '-' )
		{
			negate = true;
			++it;
		}
		int result = 0;
		while( it != end )
		{
			if( *it < '0' || *it > '9' )
			{
				break;
			}
			const int digit = *it - '0';
			result *= 10;
			// not negating after the fact so we don't lose MIN_INT
			if( negate )
			{
				result -= digit;
			}
			else
			{
				result += digit;
			}
			++it;
		}
		return result;
	}
}
