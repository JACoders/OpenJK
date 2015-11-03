#pragma once

#include <cstddef>
#include <cctype>
#include <algorithm>
#include <istream>
#include <streambuf>
#include <utility>

#include "gsl.h"

namespace Q
{
	namespace detail
	{
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator )
		{
			// Scan successful, all format arguments satisfied
			return accumulator;
		}

		//    Verbatim string
		// Try to consume the given string; whitespace means consume all available consecutive whitespace. (So format `"    "_v` also accepts input `""_v` and vice-versa.)
		template< typename... Tail >
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator, const gsl::cstring_view& expected, Tail&&... tail )
		{
			auto inputIt = input.begin();
			auto expectedIt = expected.begin();
			while( true )
			{
				if( expectedIt == expected.end() )
				{
					// success
					return sscanf_impl( { inputIt, input.end() }, accumulator, std::forward< Tail >( tail )... );
				}
				else if( std::isspace( *expectedIt ) )
				{
					// whitespace -> skip all input whitespace
					// this check is done before the end-of-input check because that's valid here.
					inputIt = std::find_if_not< gsl::cstring_view::const_iterator, int( *)(int) >(
						inputIt, input.end(),
						std::isspace
						);
					// might as well skip expected whitespace; we already consumed all input whitespace.
					++expectedIt;
					expectedIt = std::find_if_not< gsl::cstring_view::const_iterator, int( *)( int ) >(
						expectedIt, expected.end(),
						std::isspace
						);
				}
				else if( inputIt == input.end() )
				{
					// input ended too early
					return accumulator;
				}
				else if( *inputIt != *expectedIt )
				{
					// no match
					return accumulator;
				}
				else
				{
					++inputIt;
					++expectedIt;
				}
			}
		}

		/**
		A non-owning stream buffer; for adapting a gsl::cstring_view to std::istream
		*/
		template< typename CharT >
		class ArrayViewStreambuf : public std::basic_streambuf< CharT >
		{
		public:
			ArrayViewStreambuf( const gsl::array_view< const CharT >& view )
			{
				// it is not written to, but the basic_streambuf interface still wants a non-const CharT.
				char* data = const_cast< CharT* >( view.data() );
				setg( data, data, data + view.size() );
			}
		};

		/// For deducing ArrayViewStreambuf's template type
		template< typename CharT >
		inline ArrayViewStreambuf< CharT > MakeStreambuf( const gsl::array_view< const CharT >& view )
		{
			return{ view };
		}

		/**
		Conversion using std::istream's operator>>
		*/
		template< bool skipws = true, typename T, typename... Tail >
		std::size_t sscanf_impl_stream( const gsl::cstring_view& input, const std::size_t accumulator, T& value, Tail&&... tail )
		{
			auto buf = MakeStreambuf( input );
			std::istream stream( &buf );
			if( !skipws )
			{
				stream >> std::noskipws;
			}
			stream >> value;
			if( stream )
			{
				return sscanf_impl( { input.begin() + stream.gcount(), input.end() }, accumulator + 1, std::forward< Tail >( tail )... );
			}
			else
			{
				return accumulator;
			}
		}

		//    Float
		template< typename... Tail >
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator, float& f, Tail&&... tail )
		{
			return sscanf_impl_stream( input, accumulator, f, std::forward< Tail >( tail )... );
		}
	}

	/**
	Parses a given input string according to the parameters.

	Works very much like std::sscanf; but the input is a string_view instead of a null-terminated string, and the format is implicit in the arguments.

	To describe an expected string literal, pass it as a const reference to gsl::cstring_view, e.g. `"foo bar"_v`; just as with sscanf a single whitespace is taken to mean "all available whitespace".

	Returns the number of successful assignments made.
	*/
	template< typename... Format >
	std::size_t sscanf( const gsl::cstring_view& input, Format&&... format )
	{
		return detail::sscanf_impl( input, 0, std::forward< Format >( format )... );
	}
}
