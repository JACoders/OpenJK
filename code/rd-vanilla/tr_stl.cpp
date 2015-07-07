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

#include "../server/exe_headers.h"

// Filename:-	tr_stl.cpp
//
// I mainly made this file because I was getting sick of all the stupid error messages in MS's STL implementation,
//	and didn't want them showing up in the renderer files they were used in. This way keeps them more or less invisible
//	because of minimal dependancies
//
#include "tr_local.h"	// this isn't actually needed other than getting rid of warnings via pragmas
#include "tr_stl.h"

#include <map>
#include "../qcommon/sstring.h"	// #include <string>

typedef std::map<sstring_t, const char *>	ShaderEntryPtrs_t;
typedef ShaderEntryPtrs_t::size_type	ShaderEntryPtr_size;
										ShaderEntryPtrs_t ShaderEntryPtrs;

void ShaderEntryPtrs_Clear(void)
{
	ShaderEntryPtrs.clear();
}


int ShaderEntryPtrs_Size(void)
{
	return ShaderEntryPtrs.size();
}

void ShaderEntryPtrs_Insert(const char *token, const char *p)
{
	ShaderEntryPtrs_t::iterator it = ShaderEntryPtrs.find(token);

	if (it == ShaderEntryPtrs.end())
	{
		ShaderEntryPtrs[token] = p;
	}
	else
	{
		ri.Printf( PRINT_DEVELOPER, "Duplicate shader entry %s!\n",token );
	}
}

// returns NULL if not found...
//
const char *ShaderEntryPtrs_Lookup(const char *psShaderName)
{
	ShaderEntryPtrs_t::iterator it = ShaderEntryPtrs.find(psShaderName);
	if (it != ShaderEntryPtrs.end())
	{
		const char *p = (*it).second;
		return p;
	}

	return NULL;
}
