#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# We only execute this script if the user wants to use our bundled zlib/minizip.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.
#
# The bundled copy can be produced from a zlib tarball by copying the c and h
# files from the root of the tarball to this folder, and then moving the public
# header files (zlib.h, zconf.h) into the include/ subfolder.
#
# The current bundled copy comes from the zlib 1.2.8 release.

include_directories(include/)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996")
endif(MSVC)

add_library(bundled_zlib STATIC adler32.c
                                compress.c
                                crc32.c
                                deflate.c
                                gzclose.c
                                gzlib.c
                                gzread.c
                                gzwrite.c
                                infback.c
                                inffast.c
                                inflate.c
                                inftrees.c
                                trees.c
                                uncompr.c
                                zutil.c)

# Let consumers get at our bundled copy in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.  These are the same variables exported by the built-in FindZLIB module.
set(ZLIB_LIBRARIES bundled_zlib PARENT_SCOPE)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/zlib/include/ PARENT_SCOPE)

mark_as_advanced(ZLIB_LIBRARIES ZLIB_INCLUDE_DIR)

# We need to build it as position-independent code, because it gets linked into
# dynamic libraries (the vanilla renderer, for example).
set_property(TARGET bundled_zlib PROPERTY POSITION_INDEPENDENT_CODE True)
