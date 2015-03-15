#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

include(CMakeParseArguments)

unset(ZIP_EXECUTABLE CACHE)
if(WIN32)
  if(NOT ZIP_EXECUTABLE)
    find_program(ZIP_EXECUTABLE wzzip PATHS "$ENV{ProgramFiles}/WinZip")
    if(ZIP_EXECUTABLE)
      set(ZIP_COMMAND "${ZIP_EXECUTABLE}" -P "<ARCHIVE>" <FILES>)
    endif()
  endif()
  
  if(NOT ZIP_EXECUTABLE)
    find_program(ZIP_EXECUTABLE 7z PATHS "$ENV{ProgramFiles}/7-Zip")
    if(ZIP_EXECUTABLE)
      set(ZIP_COMMAND "${ZIP_EXECUTABLE}" a -tzip "<ARCHIVE>" <FILES>)
    endif()
  endif()
endif()

if(NOT ZIP_EXECUTABLE)
  if(WIN32)
    find_package(Cygwin)
    find_program(ZIP_EXECUTABLE zip PATHS "${CYGWIN_INSTALL_PATH}/bin")
  else()
    find_program(ZIP_EXECUTABLE zip)
  endif()
  
  if(ZIP_EXECUTABLE)
    set(ZIP_COMMAND "${ZIP_EXECUTABLE}" -r "<ARCHIVE>" . -i<FILES>)
  endif()
endif()

function(add_zip_command output)
  set(MultiValueArgs FILES DEPENDS)
  cmake_parse_arguments(ARGS "" "${OneValueArgs}" "${MultiValueArgs}" ${ARGN})
  
  set(ZipCommand ${ZIP_COMMAND})
  string(REPLACE <ARCHIVE> "${output}" ZipCommand "${ZipCommand}")
  string(REPLACE <FILES> "${ARGS_FILES}" ZipCommand "${ZipCommand}")
  add_custom_command(OUTPUT ${output}
    COMMAND ${ZipCommand}
    DEPENDS ${ARGS_DEPENDS})
endfunction(add_zip_command)