#pragma once

#include <cstddef>
#include <cctype>
#include <algorithm>
#include <istream>
#include <streambuf>
#include <utility>
#include <cassert>
#include <type_traits>

#include "gsl.h"

namespace Q
{
	namespace detail
	{
		inline std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator )
		{
			// Scan successful, all format arguments satisfied
			return accumulator;
		}

		inline gsl::cstring_view::const_iterator skipWhitespace( gsl::cstring_view::const_iterator begin, gsl::cstring_view::const_iterator end )
		{
			return std::find_if_not< gsl::cstring_view::const_iterator, int( *)( int ) >(
				begin, end,
				std::isspace
				);
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
					inputIt = skipWhitespace( inputIt, input.end() );
					// might as well skip expected whitespace; we already consumed all input whitespace.
					++expectedIt;
					expectedIt = skipWhitespace( expectedIt, expected.end() );
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

		//    Whitespace-terminated string
		template< typename... Tail >
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator, gsl::cstring_view& string, Tail&&... tail )
		{
			// skip leading whitespace
			auto begin = skipWhitespace( input.begin(), input.end() );
			// string is whitespace-terminated
			auto end = std::find_if< gsl::cstring_view::const_iterator, int( *)( int ) >(
				begin, input.end(),
				std::isspace
				);
			if( begin == end )
			{
				// empty string is not accepted
				return accumulator;
			}
			string = { begin, end };
			return sscanf_impl( { end, input.end() }, accumulator + 1, std::forward< Tail >( tail )... );
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
				this->setg( data, data, data + view.size() );
			}

		protected:
			/// @note required by istream.tellg()
			virtual typename std::basic_streambuf< CharT >::pos_type seekoff(
				typename std::basic_streambuf< CharT >::off_type off,
				std::ios_base::seekdir dir,
				std::ios_base::openmode which
				) override
			{
				const typename std::basic_streambuf< CharT >::pos_type errVal{ -1 };
				if( which != std::ios_base::in )
				{
					// only input pointer can be moved
					return errVal;
				}
				char* newPos = ( dir == std::ios_base::beg ) ? this->eback()
					: ( dir == std::ios_base::cur ) ? this->gptr()
					: this->egptr();
				newPos += static_cast< int >( off );
				if( this->eback() <= newPos && newPos <= this->egptr() )
				{
					this->setg( this->eback(), newPos, this->egptr() );
					return newPos - this->eback();
				}
				else
				{
					return errVal;
				}
			}
		};

		/**
		Forward declaration.
		*/
		template< bool skipws = true, typename T, typename... Tail >
		std::size_t sscanf_impl_stream( const gsl::cstring_view& input, const std::size_t accumulator, T& value, Tail&&... tail );

		//    Float
		template< typename... Tail >
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator, float& f, Tail&&... tail )
		{
			return sscanf_impl_stream( input, accumulator, f, std::forward< Tail >( tail )... );
		}

		//    Int
		template< typename... Tail >
		std::size_t sscanf_impl( const gsl::cstring_view& input, const std::size_t accumulator, int& i, Tail&&... tail )
		{
			return sscanf_impl_stream( input, accumulator, i, std::forward< Tail >( tail )... );
		}

		/**
		Conversion using std::istream's operator>>
		*/
		template< bool skipws, typename T, typename... Tail >
		std::size_t sscanf_impl_stream( const gsl::cstring_view& input, const std::size_t accumulator, T& value, Tail&&... tail )
		{
			ArrayViewStreambuf< char > buf{ input };
			std::istream stream( &buf );
			if( !skipws )
			{
				stream >> std::noskipws;
			}
			stream >> value;
			if( !stream.fail() )
			{
				auto pos = stream.tellg();
				// tellg() fails on eof, returning -1
				if( pos == std::istream::pos_type{ -1 } )
				{
					assert( stream.eof() );
					pos = input.size();
				}
				gsl::cstring_view::const_iterator end = input.begin() + static_cast< int >( pos );
				return sscanf_impl( { end, input.end() }, accumulator + 1, std::forward< Tail >( tail )... );
			}
			else
			{
				return accumulator;
			}
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
