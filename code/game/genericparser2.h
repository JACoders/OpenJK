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

// Filename:-	genericparser2.h

#include <vector>
#include <forward_list>
#include <array>
#include <cassert>

#include "qcommon/safe/gsl.h"
#include "qcommon/safe/memory.h"
#include "qcommon/safe/files.h"
#include "qcommon/safe/string.h"

#ifndef GENERICPARSER2_H
#define GENERICPARSER2_H

namespace GP2
{
	template< typename T >
	using Vector = std::vector< T, Zone::Allocator< T, TAG_GP2 > >;
}

class CGPProperty
{
public:
	using Values = GP2::Vector< gsl::cstring_span >;
private:
	gsl::cstring_span mKey;
	Values mValues;


public:

	CGPProperty( gsl::cstring_span initKey, gsl::cstring_span initValue = {} );

	const gsl::cstring_span& GetName() const { return mKey; }
	bool IsList() const NOEXCEPT
	{
		return mValues.size() > 1;
	}
	const gsl::cstring_span& GetTopValue() const NOEXCEPT
	{
		static gsl::cstring_span empty{};
		return mValues.empty() ? empty : mValues.front();
	}
	const Values& GetValues() const NOEXCEPT
	{
		return mValues;
	}
	void AddValue( gsl::cstring_span newValue );
};



class CGPGroup
{
public:
	/// Key-Value-Pairs
	using Properties = GP2::Vector< CGPProperty >;
	using SubGroups = GP2::Vector< CGPGroup >;
private:
	Properties mProperties;
	gsl::cstring_span mName = CSTRING_VIEW( "Top Level" );
	SubGroups mSubGroups;
public:
	CGPGroup() = default;
	CGPGroup( const gsl::cstring_span& initName );
	// non-copyable; but just for performance reasons, since it would incur a deep copy.
	CGPGroup( const CGPGroup& ) = delete;
	CGPGroup& operator=( const CGPGroup& ) = delete;
	// movable
#if defined( _MSC_VER ) && _MSC_VER < 1900
	// alas no default move constructors on VS2013.
	// TODO DELETEME once we drop VS2013 (because fuck that).
	CGPGroup( CGPGroup&& rhs )
		: mProperties( std::move( rhs.mProperties ) )
		, mName( std::move( rhs.mName ) )
		, mSubGroups( std::move( rhs.mSubGroups ) )
	{
	}
	CGPGroup& operator=( CGPGroup&& rhs )
	{
		mProperties = std::move( rhs.mProperties );
		mName = std::move( rhs.mName );
		mSubGroups = std::move( rhs.mSubGroups );
		return *this;
	}
#else
	CGPGroup( CGPGroup&& ) = default;
	CGPGroup& operator=( CGPGroup&& ) = default;
#endif

	const Properties& GetProperties() const NOEXCEPT
	{
		return mProperties;
	}
	const SubGroups& GetSubGroups() const NOEXCEPT
	{
		return mSubGroups;
	}
	const CGPGroup* FindSubGroup( const gsl::cstring_span& name ) const NOEXCEPT
	{
		for( auto& sub : GetSubGroups() )
		{
			if( Q::stricmp( name, sub.GetName() ) == Q::Ordering::EQ )
			{
				return &sub;
			}
		}
		return nullptr;
	}
	const CGPProperty* FindProperty( const gsl::cstring_span& name ) const NOEXCEPT
	{
		for( auto& prop : GetProperties() )
		{
			if( Q::stricmp( name, prop.GetName() ) == Q::Ordering::EQ )
			{
				return &prop;
			}
		}
		return nullptr;
	}
	const gsl::cstring_span& GetName() const NOEXCEPT
	{
		return mName;
	}
	void Clear() NOEXCEPT
	{
		// name is retained
		mProperties.clear();
		mSubGroups.clear();
	}
	bool Parse( gsl::cstring_span& data, const bool topLevel = true );
};

/**
Generic Text Parser.

Used to parse effect files and the dynamic music system files. Parses blocks of the form `name { \n ... \n }`; blocks can contain other blocks or properties of the form `key value`. Value can also be a list; in that case the format is `key [\n value1 \n value2\n]`. Mind the separating newlines, values are actually newline-delimited.
*/
class CGenericParser2
{
private:
	CGPGroup mTopLevel;
	FS::FileBuffer mFileContent;

public:

	const CGPGroup& GetBaseParseGroup()
	{
		return mTopLevel;
	}

	bool Parse( gsl::czstring filename );
	void Clear() NOEXCEPT;
	bool ValidFile() const NOEXCEPT
	{
		return mFileContent.valid();
	}
};

#endif	// #ifndef GENERICPARSER2_H


//////////////////// eof /////////////////////

