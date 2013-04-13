# - Find JPEG
# Find the native JPEG includes and library
# This module defines
#  JPEG_INCLUDE_DIR, where to find jpeglib.h, etc.
#  JPEG_LIBRARIES, the libraries needed to use JPEG.
#  JPEG_FOUND, If false, do not try to use JPEG.
# also defined, but not for general use are
#  JPEG_LIBRARY, where to find the JPEG library.

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

FIND_PATH(JPEG_INCLUDE_DIR NAMES jpeglib.h
	HINTS
		$ENV{JPEG_ROOT}
	PATH_SUFFIXES
		include
	PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
	)

SET(JPEG_NAMES ${JPEG_NAMES} jpeg)
FIND_LIBRARY(JPEG_LIBRARY
	NAMES
		${JPEG_NAMES}
	HINTS
		$ENV{JPEG_ROOT}
	PATH_SUFFIXES
		lib64
		lib
	PATHS
		/sw
		/opt/local
		/opt/csw
		/opt
	)

# handle the QUIETLY and REQUIRED arguments and set JPEG_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JPEG DEFAULT_MSG JPEG_LIBRARY JPEG_INCLUDE_DIR)

IF(JPEG_FOUND)
	SET(JPEG_LIBRARIES ${JPEG_LIBRARY})
ENDIF(JPEG_FOUND)

# Deprecated declarations.
SET (NATIVE_JPEG_INCLUDE_PATH ${JPEG_INCLUDE_DIR} )
IF(JPEG_LIBRARY)
	GET_FILENAME_COMPONENT (NATIVE_JPEG_LIB_PATH ${JPEG_LIBRARY} PATH)
ENDIF(JPEG_LIBRARY)

MARK_AS_ADVANCED(JPEG_LIBRARY JPEG_INCLUDE_DIR )
