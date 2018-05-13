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

# Build our bundled minizip.
#
# It is built as a static relocatable library, and linked into any target that
# requires it.  Other targets in OpenJK that consume need to add
# MINZIP_INCLUDE_DIRS and MINIZIP_LIBRARIES to their lists of include
# directories and target link libraries.
#
# The bundled copy is produced by taking the files ioapi.{c,h} and unzip.{c,h}
# from the contrib/minizip directory of the zlib release tarball.  The only
# changes applied are modifications of the macros ALLOC and TRYFREE in unzip.c,
# and a compatibility definition of the macro OF in include/ioapi.h
#
# Since minizip is expected to be reference by user code as
#
#     #include <minizip/unzip.h>
#
# the public header files are split into the include/minizip subdirectory.
#
# The current bundled copy comes from the zlib 1.2.8 release.

# These settings only apply to this directory.
include_directories(${ZLIB_INCLUDE_DIR})
include_directories(include/minizip)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

add_library(bundled_minizip STATIC ioapi.c unzip.c)

# Let consumers get at our bundled copy in the standard CMake way.  These
# variables are not set in the cache, but instead shadow the variables in the
# cache.
set(MINIZIP_LIBRARIES bundled_minizip PARENT_SCOPE)
set(MINIZIP_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/minizip/include/ PARENT_SCOPE)

mark_as_advanced(MINIZIP_LIBRARIES MINIZIP_INCLUDE_DIRS)

# We need to build it as position-independent code, because it might get linked
# into dynamic libraries.
set_property(TARGET bundled_minizip PROPERTY POSITION_INDEPENDENT_CODE True)
