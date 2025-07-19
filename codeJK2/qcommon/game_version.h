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

#include "qcommon/q_version.h"

#if !defined(STRING) && !defined(XSTRING)
	// rather than including q_shared.h from win32 resource scripts (win32/*.rc)
	#define STRING( a ) #a
	#define XSTRING( a ) STRING( a )
#endif

// Current version of the single player game
#define VERSION_MAJOR_RELEASE		1
#define VERSION_MINOR_RELEASE		0
#define VERSION_EXTERNAL_BUILD		3
#define VERSION_INTERNAL_BUILD		0

#define VERSION_STRING XSTRING(VERSION_MAJOR_RELEASE) ", " XSTRING(VERSION_MINOR_RELEASE) ", " XSTRING(VERSION_EXTERNAL_BUILD) ", " XSTRING(VERSION_INTERNAL_BUILD) // "a, b, c, d"
#define VERSION_STRING_DOTTED XSTRING(VERSION_MAJOR_RELEASE) "." XSTRING(VERSION_MINOR_RELEASE) "." XSTRING(VERSION_EXTERNAL_BUILD) "." XSTRING(VERSION_INTERNAL_BUILD) // "a.b.c.d"

#if defined(_DEBUG)
	#define	JK_VERSION		"(debug)OpenJO: " GIT_TAG
	#define JK_VERSION_OLD	"(debug)JO: v" VERSION_STRING_DOTTED
#else
	#define	JK_VERSION		"OpenJO: " GIT_TAG
	#define JK_VERSION_OLD	"JO: v" VERSION_STRING_DOTTED
#endif
