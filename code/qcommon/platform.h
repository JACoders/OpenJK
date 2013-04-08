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

// Simple header file to dispatch to the relevant platform API headers
#ifndef _PLATFORM_H
#define _PLATFORM_H

#if defined(_XBOX)
#include <xtl.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#endif

#if defined(_WINDOWS)
#include <windows.h>
#endif

#endif
