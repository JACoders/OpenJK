/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// Filename:-	genericparser2.cpp

#include "common_headers.h"
#include "genericparser2.h"

#ifdef _JK2EXE
#include "../qcommon/qcommon.h"
#endif

#include <algorithm>
#include <cctype>


static void skipWhitespace( gsl::cstring_span& text, const bool allowLineBreaks )
{
	gsl::cstring_span::iterator whitespaceEnd = text.begin();
	while( whitespaceEnd != text.end() // No EOF
		&& std::isspace( *whitespaceEnd ) // No End of Whitespace
		&& ( allowLineBreaks || *whitespaceEnd != '\n' ) ) // No unwanted newline
	{
		++whitespaceEnd;
	}
	text = { whitespaceEnd, text.end() };
}

static void skipWhitespaceAndComments( gsl::cstring_span& text, const bool allowLineBreaks )
{
	skipWhitespace( text, allowLineBreaks );
	// skip single line comment
	if( text.size() >= 2 && text[ 0 ] == '/' && text[ 1 ] == '/' )
	{
		auto commentEnd = std::find( text.begin() + 2, text.end(), '\n' );
		if( commentEnd == text.end() )
		{
			text = { text.end(), text.end() };
			return;
		}
		else
		{
			text = { commentEnd, text.end() };
			skipWhitespaceAndComments( text, allowLineBreaks );
			return;
		}
	}

	// skip multi line comments
	if( text.size() >= 2 && text[ 0 ] == '/' && text[ 1 ] == '*' )
	{
		static const std::array< char, 2 > endStr{ '*', '/' };
		auto commentEnd = std::search( text.begin(), text.end(), endStr.begin(), endStr.end() );
		if( commentEnd == text.end() )
		{
			text = { text.end(), text.end() };
			return;
		}
		else
		{
			text = { commentEnd + endStr.size(), text.end() };
			skipWhitespace( text, allowLineBreaks );
			return;
		}
	}

	// found start of token
	return;
}

static gsl::cstring_span removeTrailingWhitespace( const gsl::cstring_span& text )
{
	return{
		text.begin(),
		std::find_if_not(
			std::reverse_iterator< const char *>( text.end() ), std::reverse_iterator< const char* >( text.begin() ),
			static_cast< int( *)( int ) >( std::isspace )
			).base()
	};
}

/**
Skips whitespace (including lineBreaks, if desired) & comments, then reads one token.
A token can be:
- a string ("" delimited; ignores readToEOL)
- whitespace-delimited (if readToEOL == false)
- EOL- or comment-delimited (if readToEOL == true); i.e. reads to end of line or the first // or /*
@param text adjusted to start beyond the read token
*/
static gsl::cstring_span GetToken( gsl::cstring_span& text, bool allowLineBreaks, bool readToEOL = false )
{
	skipWhitespaceAndComments( text, allowLineBreaks );
	// EOF
	if( text.empty() )
	{
		return{};
	}
	// string. ignores readToEOL.
	if( text[ 0 ] == '"' )
	{
		// there are no escapes, string just ends at the next "
		auto tokenEnd = std::find( text.begin() + 1, text.end(), '"' );
		if( tokenEnd == text.end() )
		{
			gsl::cstring_span token = { text.begin() + 1, text.end() };
			text = { text.end(), text.end() };
			return token;
		}
		else
		{
			gsl::cstring_span token = { text.begin() + 1, tokenEnd };
			text = { tokenEnd + 1, text.end() };
			return token;
		}
	}
	else if( readToEOL )
	{
		// find the first of '\n', "//" or "/*"; that's end of token
		auto tokenEnd = std::find( text.begin(), text.end(), '\n' );
		static const std::array< char, 2 > commentPatterns[]{
			{ { '/', '*' } },
			{ { '/', '/' } }
		};
		for( auto& pattern : commentPatterns )
		{
			tokenEnd = std::min(
				tokenEnd,
				std::search(
					text.begin(), tokenEnd,
					pattern.begin(), pattern.end()
					)
				);
		}
		gsl::cstring_span token{ text.begin(), tokenEnd };
		text = { tokenEnd, text.end() };
		return removeTrailingWhitespace( token );
	}
	else
	{
		// consume until first whitespace (if allowLineBreaks == false, that may be text.begin(); in that case token is empty.)
		auto tokenEnd = std::find_if( text.begin(), text.end(), static_cast< int( *)( int ) >( std::isspace ) );
		gsl::cstring_span token{ text.begin(), tokenEnd };
		text = { tokenEnd, text.end() };
		return token;
	}
}





CGPProperty::CGPProperty( gsl::cstring_span initKey, gsl::cstring_span initValue )
	: mKey( initKey )
{
	if( !initValue.empty() )
	{
		mValues.push_back( initValue );
	}
}

void CGPProperty::AddValue( gsl::cstring_span newValue )
{
	mValues.push_back( newValue );
}
















CGPGroup::CGPGroup( const gsl::cstring_span& initName )
	: mName( initName )
{
}

bool CGPGroup::Parse( gsl::cstring_span& data, const bool topLevel )
{
	while( true )
	{
		gsl::cstring_span token = GetToken( data, true );

		if( token.empty() )
		{
			if ( topLevel )
			{
				// top level parse; there was no opening "{", so there should be no closing one either.
				return true;
			}
			else
			{
				// end of data - error!
				return false;
			}
		}
		else if( token == CSTRING_VIEW( "}" ) )
		{
			if( topLevel )
			{
				// top-level group; there was no opening "{" so there should be no closing one, either.
				return false;
			}
			else
			{
				// ending brace for this group
				return true;
			}
		}
		gsl::cstring_span lastToken = token;

		// read ahead to see what we are doing
		token = GetToken( data, true, true );
		if( token == CSTRING_VIEW( "{" ) )
		{
			// new sub group
			mSubGroups.emplace_back( lastToken );
			if( !mSubGroups.back().Parse( data, false ) )
			{
				return false;
			}
		}
		else if( token == CSTRING_VIEW( "[" ) )
		{
			// new list
			mProperties.emplace_back( lastToken );
			CGPProperty& list = mProperties.back();
			while( true )
			{
				token = GetToken( data, true, true );
				if( token.empty() )
				{
					return false;
				}
				if( token == CSTRING_VIEW( "]" ) )
				{
					break;
				}
				list.AddValue( token );
			}
		}
		else
		{
			// new value
			mProperties.emplace_back( lastToken, token );
		}
	}
}














bool CGenericParser2::Parse( gsl::czstring filename )
{
	Clear();
	mFileContent = FS::ReadFile( filename );
	if( !mFileContent.valid() )
	{
		return false;
	}
	auto view = mFileContent.view();
	return mTopLevel.Parse( view );
}

void CGenericParser2::Clear() NOEXCEPT
{
	mTopLevel.Clear();
}



//////////////////// eof /////////////////////

