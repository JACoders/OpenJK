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

#pragma once

// for windows fastcall option
#define QDECL
#define QCALL

// Win64
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)

	#define idx64

	#undef QDECL
	#define QDECL __cdecl

	#undef QCALL
	#define QCALL __stdcall

	#if defined(_MSC_VER)
		#define OS_STRING "win_msvc"
	#elif defined(__MINGW64__)
		#define OS_STRING "win_mingw"
	#endif

	#define QINLINE __inline
	#define PATH_SEP '\\'

	#if defined(_M_ALPHA)
		#define ARCH_STRING "AXP"
	#else
		#define ARCH_STRING "x86_64"
	#endif

	#define Q3_LITTLE_ENDIAN

	#define DLL_EXT ".dll"

// Win32
#elif defined(_WIN32) || defined(__WIN32__)

	#undef QDECL
	#define	QDECL __cdecl

	#undef QCALL
	#define QCALL __stdcall

	#if defined(_MSC_VER)
		#define OS_STRING "win_msvc"
	#elif defined(__MINGW32__)
		#define OS_STRING "win_mingw"
	#endif

	#define QINLINE __inline
	#define PATH_SEP '\\'

	#if defined(_M_IX86) || defined(__i386__)
		#define ARCH_STRING "x86"
	#elif defined _M_ALPHA
		#define ARCH_STRING "AXP"
	#endif

	#define Q3_LITTLE_ENDIAN

	#define DLL_EXT ".dll"

// MAC OS X
#elif defined(MACOS_X) || defined(__APPLE_CC__)

	// make sure this is defined, just for sanity's sake...
	#ifndef MACOS_X
		#define MACOS_X
	#endif

	#define OS_STRING "macosx"
	#define QINLINE /*inline*/ 
	#define	PATH_SEP '/'

	#if defined(__ppc__)
		#define ARCH_STRING "ppc"
		#define Q3_BIG_ENDIAN
	#elif defined(__i386__)
		#define ARCH_STRING "x86"
		#define Q3_LITTLE_ENDIAN
	#elif defined(__x86_64__)
		#define idx64
		#define ARCH_STRING "x86_64"
		#define Q3_LITTLE_ENDIAN
	#endif

    #define DLL_EXT ".dylib"

// Linux
#elif defined(__linux__) || defined(__FreeBSD_kernel__)

	#include <endian.h>

	#if defined(__linux__)
		#define OS_STRING "linux"
	#else
		#define OS_STRING "kFreeBSD"
	#endif

	#ifdef __clang__
		#define QINLINE static inline
	#else
		#define QINLINE inline
	#endif

	#define PATH_SEP '/'

	#if !defined(ARCH_STRING)
		#error ARCH_STRING should be defined by the build system
	#endif

	#if defined(__x86_64__)
		#define idx64
	#endif

	#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
		#define Q3_BIG_ENDIAN
	#else
		#define Q3_LITTLE_ENDIAN
	#endif

	#define DLL_EXT ".so"

// BSD
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

	#include <sys/types.h>
	#include <machine/endian.h>
	
	#ifndef __BSD__
		#define __BSD__
	#endif
	
	#if defined(__FreeBSD__)
		#define OS_STRING "freebsd"
	#elif defined(__OpenBSD__)
		#define OS_STRING "openbsd"
	#elif defined(__NetBSD__)
		#define OS_STRING "netbsd"
	#endif
	
	#define QINLINE inline
	#define PATH_SEP '/'

	#if !defined(ARCH_STRING)
		#error ARCH_STRING should be defined by the build system
	#endif

	#if defined(__amd64__)
		#define idx64
	#endif

	#if BYTE_ORDER == BIG_ENDIAN
		#define Q3_BIG_ENDIAN
	#else
		#define Q3_LITTLE_ENDIAN
	#endif
	
	#define DLL_EXT ".so"
#endif

// catch missing defines in above blocks
#if !defined(OS_STRING)
	#error "Operating system not supported"
#endif
#if !defined(ARCH_STRING)
	#error "Architecture not supported"
#endif
#if !defined(DLL_EXT)
	#error "DLL_EXT not defined"
#endif
#if !defined(QINLINE)
	#error "QINLINE not defined"
#endif
#if !defined(PATH_SEP)
	#error "PATH_SEP not defined"
#endif

// endianness
void CopyShortSwap( void *dest, void *src );
void CopyLongSwap( void *dest, void *src );
short ShortSwap( short l );
int LongSwap( int l );
float FloatSwap( const float *f );

#if defined(Q3_BIG_ENDIAN) && defined(Q3_LITTLE_ENDIAN)
	#error "Endianness defined as both big and little"
#elif defined(Q3_BIG_ENDIAN)
	#define CopyLittleShort( dest, src )	CopyShortSwap( dest, src )
	#define CopyLittleLong( dest, src )		CopyLongSwap( dest, src )
	#define LittleShort( x )				ShortSwap( x )
	#define LittleLong( x )					LongSwap( x )
	#define LittleFloat( x )				FloatSwap( &x )
	#define BigShort
	#define BigLong
	#define BigFloat
#elif defined( Q3_LITTLE_ENDIAN )
	#define CopyLittleShort( dest, src )	Com_Memcpy(dest, src, 2)
	#define CopyLittleLong( dest, src )		Com_Memcpy(dest, src, 4)
	#define LittleShort
	#define LittleLong
	#define LittleFloat
	#define BigShort( x )					ShortSwap( x )
	#define BigLong( x )					LongSwap( x )
	#define BigFloat( x )					FloatSwap( &x )
#else
	#error "Endianness not defined"
#endif


// platform string
#if defined(NDEBUG)
	#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
#else
	#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
#endif
