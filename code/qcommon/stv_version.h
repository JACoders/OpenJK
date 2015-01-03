/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Current version of the single player game
#include "../win32/AutoVersion.h"

#ifdef _DEBUG
	#define	Q3_VERSION		"(debug)OpenJK: v"VERSION_STRING_DOTTED
#elif defined FINAL_BUILD
	#define	Q3_VERSION		"OpenJK: v"VERSION_STRING_DOTTED
#else
	#define	Q3_VERSION		"(internal)OpenJK: v"VERSION_STRING_DOTTED
#endif
// end


