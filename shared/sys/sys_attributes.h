/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// Common attributes used by the engine and plugin modules.

#ifndef OPENJK_ATTRIBUTES_HH
#define OPENJK_ATTRIBUTES_HH

//Ignore __attribute__ on non-gcc platforms
#if !defined(__GNUC__) && !defined(__attribute__)
    #define __attribute__(x)
#endif

#if defined(__GNUC__)
    #define UNUSED_VAR __attribute__((unused))
#else
    #define UNUSED_VAR
#endif

#if (defined _MSC_VER)
    #define Q_EXPORT __declspec(dllexport)
#elif (defined __SUNPRO_C)
    #define Q_EXPORT __global
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
    #define Q_EXPORT __attribute__((visibility("default")))
#else
    #define Q_EXPORT
#endif

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#endif

// this is the define for determining if we have an asm version of a C function
#if (defined(_M_IX86) || defined(__i386__)) && !defined(__sun__)
    #define id386	1
#else
    #define id386	0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
    #define idppc	1
#else
    #define idppc	0
#endif

#endif
