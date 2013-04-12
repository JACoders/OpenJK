# - Find zlib
# Find the native ZLIB includes and library.
# Once done this will define
#
#  ZLIB_INCLUDE_DIRS   - where to find zlib.h, etc.
#  ZLIB_LIBRARIES      - List of libraries when using zlib.
#  ZLIB_FOUND          - True if zlib found.
#
#  ZLIB_VERSION_STRING - The version of zlib found (x.y.z)
#  ZLIB_VERSION_MAJOR  - The major version of zlib
#  ZLIB_VERSION_MINOR  - The minor version of zlib
#  ZLIB_VERSION_PATCH  - The patch version of zlib
#  ZLIB_VERSION_TWEAK  - The tweak version of zlib
#
# The following variable are provided for backward compatibility
#
#  ZLIB_MAJOR_VERSION  - The major version of zlib
#  ZLIB_MINOR_VERSION  - The minor version of zlib
#  ZLIB_PATCH_VERSION  - The patch version of zlib

#=============================================================================
# License
#
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2009 Kitware, Inc., Insight Software Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium, nor the names of their contributors may be used to endorse or promote products derived from this software without specific prior written  permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
	HINTS
		$ENV{ZLIB_ROOT}
	PATH_SUFFIXES
		include
	PATHS
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]/include"
		~/Library/Frameworks
		/Library/Frameworks
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
)

SET(ZLIB_NAMES z zlib zdll zlib1 zlibd zlibd1)
FIND_LIBRARY(ZLIB_LIBRARY
	NAMES
		${ZLIB_NAMES}
	HINTS
		$ENV{ZLIB_ROOT}
	PATH_SUFFIXES
		lib64
		lib
	PATHS
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]/lib"
		/sw
		/opt/local
		/opt/csw
		/opt
)
MARK_AS_ADVANCED(ZLIB_LIBRARY ZLIB_INCLUDE_DIR)

IF(ZLIB_INCLUDE_DIR AND EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
	FILE(STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" ZLIB_H REGEX "^#define ZLIB_VERSION \"[^\"]*\"$")

	STRING(REGEX REPLACE "^.*ZLIB_VERSION \"([0-9]+).*$" "\\1" ZLIB_VERSION_MAJOR "${ZLIB_H}")
	STRING(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_MINOR  "${ZLIB_H}")
	STRING(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_PATCH "${ZLIB_H}")
	SET(ZLIB_VERSION_STRING "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")

	# only append a TWEAK version if it exists:
	SET(ZLIB_VERSION_TWEAK "")
	IF( "${ZLIB_H}" MATCHES "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*$")
		SET(ZLIB_VERSION_TWEAK "${CMAKE_MATCH_1}")
		SET(ZLIB_VERSION_STRING "${ZLIB_VERSION_STRING}.${ZLIB_VERSION_TWEAK}")
	ENDIF( "${ZLIB_H}" MATCHES "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*$")

	SET(ZLIB_MAJOR_VERSION "${ZLIB_VERSION_MAJOR}")
	SET(ZLIB_MINOR_VERSION "${ZLIB_VERSION_MINOR}")
	SET(ZLIB_PATCH_VERSION "${ZLIB_VERSION_PATCH}")
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB REQUIRED_VARS ZLIB_LIBRARY ZLIB_INCLUDE_DIR
                                       VERSION_VAR ZLIB_VERSION_STRING)

IF(ZLIB_FOUND)
	SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
	SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
ENDIF()

