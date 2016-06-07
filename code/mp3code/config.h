/*____________________________________________________________________________

	FreeAmp - The Free MP3 Player

	Portions Copyright (C) 1998-1999 EMusic.com

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: config.win32,v 1.16 1999/12/09 08:44:07 elrod Exp $
____________________________________________________________________________*/

#ifndef CONFIG_H
#define CONFIG_H

#if !defined(RC_INVOKED)

#include <limits.h>

#define HAVE_IO_H 1
#define HAVE_ERRNO_H 1

#if HAVE_UNISTD_H
#define RD_BNRY_FLAGS O_RDONLY
#elif HAVE_IO_H
#define RD_BNRY_FLAGS O_RDONLY | O_BINARY
#endif

/* Endian Issues */
#ifdef LINUX
#include <endian.h>
#endif

#ifdef WIN32
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __PDP_ENDIAN    3412
#define __BYTE_ORDER    __LITTLE_ENDIAN
#define usleep(x) ::Sleep(x/1000)
typedef int socklen_t;
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

/* define our datatypes */
// real number
typedef double real;

#if UCHAR_MAX == 0xff

typedef unsigned char	uint8;
typedef signed char		int8;

#else
#error This machine has no 8-bit type
#endif

#if UINT_MAX == 0xffff

typedef unsigned int	uint16;
typedef int				int16;

#elif USHRT_MAX == 0xffff

typedef unsigned short	uint16;
typedef short			int16;

#else
#error This machine has no 16-bit type
#endif


#if UINT_MAX == 0xfffffffful

typedef unsigned int	uint32;
typedef int				int32;

#elif ULONG_MAX == 0xfffffffful

typedef unsigned long	uint32;
typedef long			int32;

#elif USHRT_MAX == 0xfffffffful

typedef unsigned short	uint32;
typedef short			int32;

#else
#error This machine has no 32-bit type
#endif


// What character marks the end of a directory entry? For DOS and
// Windows, it is "\"; in UNIX it is "/".
#if defined(WIN32) || defined(OS2) || defined(__DOS__)
#define DIR_MARKER '\\'
#define DIR_MARKER_STR "\\"
#else
#define DIR_MARKER '/'
#define DIR_MARKER_STR "/"
#endif /* WIN32 */

// What character(s) marks the end of a line in a text file?
// For DOS and Windows, it is "\r\n"; in UNIX it is "\r".
#if defined(WIN32) || defined(OS2) || defined(__DOS__)
#define LINE_END_MARKER_STR "\r\n"
#else
#define LINE_END_MARKER_STR "\n"
#endif /* WIN32 */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif /* NULL */

#endif /* RC_INVOKED */

#endif /* CONFIG_H */
