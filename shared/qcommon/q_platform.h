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
	#define QINLINE inline
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

	#define QINLINE inline

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

#if (defined( _MSC_VER ) && (_MSC_VER < 1900)) || (defined(__GNUC__))
// VS2013, which for some reason we still support, does not support noexcept
// GCC GNU has the same problem: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52869
#define NOEXCEPT
#define NOEXCEPT_IF(x)
#define IS_NOEXCEPT(x) false
#else
#define NOEXCEPT noexcept
#define NOEXCEPT_IF(x) noexcept(x)
#define IS_NOEXCEPT(x) noexcept(x)
#endif

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#define NORETURN_PTR __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
// __declspec doesn't work on function pointers
#define NORETURN_PTR /* nothing */
#else
#define NORETURN /* nothing */
#define NORETURN_PTR /* nothing */
#endif

#define OVERRIDE override

#if defined(__cplusplus)
	#include <cstddef>

	// gcc versions < 4.9 did not add max_align_t to the std:: namespace, but instead
	// put it in the global namespace. Need this to provide uniform access to max_align_t
	#if defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 9))
		typedef max_align_t qmax_align_t;
	#else
		typedef std::max_align_t qmax_align_t;
	#endif
#endif

#if defined (_MSC_VER)
	#if _MSC_VER >= 1600
		#include <stdint.h>
	#else
		typedef signed __int64 int64_t;
		typedef signed __int32 int32_t;
		typedef signed __int16 int16_t;
		typedef signed __int8  int8_t;
		typedef unsigned __int64 uint64_t;
		typedef unsigned __int32 uint32_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int8  uint8_t;
	#endif
#else // not using MSVC
	#if !defined(__STDC_LIMIT_MACROS)
		#define __STDC_LIMIT_MACROS
	#endif
	#include <stdint.h>
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
// Use compiler builtins where possible for maximum performance
#include <stdint.h>
#if !defined(__clang__) && (defined(__GNUC__) || defined(__GNUG__)) \
            && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 403)
// gcc >= 4.3

static inline uint16_t ShortSwap(uint16_t v)
{
#if __GNUC_MINOR__ >= 8
    return __builtin_bswap16(v);
#else
    return (v << 8) | (v >> 8);
#endif // gcc >= 4.8
}

static inline uint32_t LongSwap(uint32_t v)
{
    return __builtin_bswap32(v);
}
#elif defined(_MSC_VER)
// MSVC

// required for _byteswap_ushort/ulong
#include <stdlib.h>

static uint16_t ShortSwap(uint16_t v)
{
    return _byteswap_ushort(v);
}

static uint32_t LongSwap(uint32_t v)
{
    return _byteswap_ulong(v);
}

#else
// clang, gcc < 4.3 and others

static inline uint16_t ShortSwap(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

static inline uint32_t LongSwap(uint32_t v)
{
    return ((v & 0x000000FF) << 24) |
           ((v & 0x0000FF00) << 8)  |
           ((v & 0x00FF0000) >> 8)  |
           ((v & 0xFF000000) >> 24);
}
#endif

static QINLINE void CopyShortSwap( void *dest, const void *src )
{
    *(uint16_t*)dest = ShortSwap(*(uint16_t*)src);
}

static QINLINE void CopyLongSwap( void *dest, const void *src )
{
    *(uint32_t*)dest = LongSwap(*(uint32_t*)src);
}

static QINLINE float FloatSwap(float f)
{
    float out;
    CopyLongSwap(&out, &f);
    return out;
}

#if defined(Q3_BIG_ENDIAN) && defined(Q3_LITTLE_ENDIAN)
	#error "Endianness defined as both big and little"
#elif defined(Q3_BIG_ENDIAN)
	#define CopyLittleShort( dest, src )	CopyShortSwap( dest, src )
	#define CopyLittleLong( dest, src )		CopyLongSwap( dest, src )
	#define LittleShort( x )				ShortSwap( x )
	#define LittleLong( x )					LongSwap( x )
	#define LittleFloat( x )				FloatSwap( x )
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
	#define BigFloat( x )					FloatSwap( x )
#else
	#error "Endianness not defined"
#endif

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long ulong;

typedef enum { qfalse, qtrue } qboolean;

// 32 bit field aliasing
typedef union byteAlias_u {
	float f;
	int32_t i;
	uint32_t ui;
	qboolean qb;
	byte b[4];
	char c[4];
} byteAlias_t;

// platform string
#if defined(NDEBUG)
	#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
#else
	#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
#endif
