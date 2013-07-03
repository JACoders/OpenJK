/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#ifndef __ICR_STDAFX__
#define __ICR_STDAFX__

#ifdef _MSC_VER
#pragma warning( disable : 4786 )  // identifier was truncated 

#pragma warning (push, 3)
#endif
#include <string>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#ifdef _MSC_VER
#pragma warning (pop)
#endif

using namespace std;

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

#endif
