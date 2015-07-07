/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#pragma once

// hide these nasty warnings

#ifdef _MSC_VER
	#pragma warning( disable : 4018 )	// signed/unsigned mismatch
	#pragma warning( disable : 4032 )	//
	#pragma warning( disable : 4051 )	//
	#pragma warning( disable : 4057 )	// slightly different base types
	#pragma warning( disable : 4100 )	// unreferenced formal parameter
	#pragma warning( disable : 4115 )	//
	#pragma warning( disable : 4125 )	// decimal digit terminates octal escape sequence
	#pragma warning( disable : 4127 )	// conditional expression is constant
	#pragma warning( disable : 4136 )	//
	#pragma warning( disable : 4152 )	// nonstandard extension, function/data pointer conversion in expression
	#pragma warning( disable : 4201 )	//
	#pragma warning( disable : 4213 )	// nonstandard extension used : cast on l-value
	#pragma warning( disable : 4214 )	//
	#pragma warning( disable : 4220 )	// varargs matches remaining parameters
	#pragma warning( disable : 4244 )	// conversion from double to float
	#pragma warning( disable : 4245 )	// signed/unsigned mismatch
	#pragma warning( disable : 4284 )	// return type not UDT
	#pragma warning( disable : 4305 )	// truncation from const double to float
	#pragma warning( disable : 4310 )	// cast truncates constant value
	#pragma warning( disable : 4389 )	// signed/unsigned mismatch
	#pragma warning( disable : 4503 )	// decorated name length truncated
//	#pragma warning( disable:  4505 )	// unreferenced local function has been removed
	#pragma warning( disable : 4511 )	// copy ctor could not be genned
	#pragma warning( disable : 4512 )	// assignment op could not be genned
	#pragma warning( disable : 4514 )	// unreffed inline removed
	#pragma warning( disable : 4663 )	// c++ lang change
	#pragma warning( disable : 4702 )	// unreachable code
	#pragma warning( disable : 4710 )	// not inlined
	#pragma warning( disable : 4711 )	// selected for automatic inline expansion
	#pragma warning( disable : 4786 )	// identifier was truncated
	#pragma warning( disable : 4996 )	// This function or variable may be unsafe.
#endif
