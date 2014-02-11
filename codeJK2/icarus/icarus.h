/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// ICARUS Public Header File

#ifdef _MSC_VER
	#pragma warning ( disable : 4786 )	//NOTENOTE: STL Debug name length warning
#endif

#ifndef	__ICARUS__
#define __ICARUS__

#ifdef _MSC_VER
	#pragma warning( disable : 4786 )  // identifier was truncated 
	#pragma warning( disable : 4514 )  // unreferenced inline was removed
	#pragma warning( disable : 4710 )  // not inlined

	#pragma warning( push, 3 )	//save current state and change to 3
#endif

//For system-wide prints
enum WL_e {
	WL_ERROR=1,
	WL_WARNING,
	WL_VERBOSE,
	WL_DEBUG
};

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

#include "tokenizer.h"
#include "blockstream.h"
#include "interpreter.h"
#include "sequencer.h"
#include "taskmanager.h"
#include "instance.h"

#ifdef _MSC_VER
	#pragma warning( pop )	//restore
#endif

extern void *ICARUS_Malloc(int iSize);
extern void  ICARUS_Free(void *pMem);

#endif	//__ICARUS__
