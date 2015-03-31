#============================================================================
# Copyright (C) 2013 - 2015, OpenJK contributors
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

cmake_minimum_required(VERSION 2.8.8)
# For checks in subdirectories
set(InOpenJK TRUE)

# Project name
set(ProjectName "OpenJK" CACHE STRING "Project Name")
project(${ProjectName})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Customizable options
option(BuildPortableVersion "Build portable version (does not read or write files from your user/home directory" OFF)

option(BuildMPEngine "Whether to create projects for the MP client (openjk.exe)" ON)
option(BuildMPRdVanilla "Whether to create projects for the MP default renderer (rd-vanilla_x86.dll)" ON)
option(BuildMPDed "Whether to create projects for the MP dedicated server (openjkded.exe)" ON)
option(BuildMPGame "Whether to create projects for the MP server-side gamecode (jampgamex86.dll)" ON)
option(BuildMPCGame "Whether to create projects for the MP clientside gamecode (cgamex86.dll)" ON)
option(BuildMPUI "Whether to create projects for the MP UI code (uix86.dll)" ON)
option(BuildMPRend2 "Whether to create projects for the EXPERIMENTAL MP rend2 renderer (rd-rend2_x86.dll)" ON)
option(BuildSPEngine "Whether to create projects for the SP engine (openjk_sp.exe)" ON)
option(BuildSPGame "Whether to create projects for the SP gamecode (jagamex86.dll)" ON)
option(BuildSPRdVanilla "Whether to create projects for the SP default renderer (rdsp-vanilla_x86.dll)" ON)

option(BuildJK2SPEngine "Whether to create projects for the jk2 SP engine (openjo_sp.exe)" OFF)
option(BuildJK2SPGame "Whether to create projects for the jk2 sp gamecode mod (jk2gamex86.dll)" OFF)
option(BuildJK2SPRdVanilla "Whether to create projects for the jk2 sp renderer (rdjosp-vanilla_x86.dll)" OFF)

# Customizable libraries
if(WIN32)
	option(UseInternalOpenAL "Whether to use the included OpenAL instead of a locally installed one" ON)
	option(UseInternalZlib "Whether to use the included zlib instead of a locally installed one" ON)
endif()

if(WIN32)
	option(UseInternalPNG "Whether to use the included libpng instead of a locally installed one" ON)
else()
	option(UseInternalPNG "Whether to use the included libpng instead of a locally installed one" OFF)
endif()

if(WIN32 OR APPLE)
	option(UseInternalJPEG "Whether to use the included libjpeg instead of a locally installed one" ON)
else()
	option(UseInternalJPEG "Whether to use the included libjpeg instead of a locally installed one" OFF)
endif()

if(APPLE)
	option(MakeApplicationBundles "Whether to build .app application bundles for engines built" ON)
else()
	set(MakeApplicationBundles OFF)
endif()

if(MSVC)
	option(UseInternalSDL2 "Whether to use the included SDL2 library instead of a locally installed one" ON)
else()
	set(UseInternalSDL2 OFF)
endif()

# Custom CMake Modules needed
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/CMakeModules")

# Arch Suffix
if (CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(ARCH_BITS 64)
	if(WIN32)
		set(Architecture "x86_64")
		set(WIN64 TRUE)
	else()
		set(Architecture "x86_64")
	endif()
else()
	set(ARCH_BITS 32)
	if(WIN32)
		set(Architecture "x86")
		set(WIN64 FALSE)
	elseif(APPLE)
		set(Architecture "x86")
	elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
		set(Architecture "arm")
	else()
		set(Architecture "i386")
	endif()
endif()

message("Architecture is ${Architecture}")

# Current Git SHA1 hash
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message("Git revision is ${GIT_SHA1}")

# Binary names
set(SPEngine "openjk_sp.${Architecture}")
set(SPGame "jagame${Architecture}")
set(SPRDVanillaRenderer "rdsp-vanilla_${Architecture}")
set(MPEngine "openjk.${Architecture}")
set(MPVanillaRenderer "rd-vanilla_${Architecture}")
set(MPDed "openjkded.${Architecture}")
set(MPGame "jampgame${Architecture}")
set(MPCGame "cgame${Architecture}")
set(MPUI "ui${Architecture}")
set(MPRend2 "rd-rend2_${Architecture}")
set(JK2SPEngine "openjo_sp.${Architecture}")
set(JK2SPGame "jospgame${Architecture}")
set(JK2SPVanillaRenderer "rdjosp-vanilla_${Architecture}")
# Library names
set(MPBotLib "botlib")
set(SharedLib "shared")

# Paths
set(SPDir "${CMAKE_SOURCE_DIR}/code")
set(MPDir "${CMAKE_SOURCE_DIR}/codemp")
set(JK2SPDir "${CMAKE_SOURCE_DIR}/codeJK2")
set(SharedDir ${CMAKE_SOURCE_DIR}/shared)

include(InstallConfig)

# Common settings
if(WIN32)
	set(SharedDefines "NOMINMAX")
	if(MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # enable parallel compilation
	endif(MSVC)
	if(WIN64)
		set(SharedDefines ${SharedDefines} "WIN64")
		if(MSVC)
			set(SharedDefines ${SharedDefines} "_CRT_SECURE_NO_WARNINGS")
		else()
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
		endif()
	else()
		if(MSVC)
			set(SharedDefines ${SharedDefines} "_CRT_SECURE_NO_WARNINGS")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
		endif()
	endif()
else()
	if (APPLE)
		set(SharedDefines "MACOS_X")
	#	set(SharedDefines "MACOS_X" "_M_IX86")
	#else()
	#	set(SharedDefines "__linux__" "_M_IX86")
	endif()

	# removes the -rdynamic flag at linking (which causes crashes for some reason)
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

	# additional flags for debug configuration
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")

	if (NOT Architecture STREQUAL "arm")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
	endif()

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
		if (Architecture STREQUAL "arm")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
		else()
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
		endif()
		#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
		if (Architecture STREQUAL "arm")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
		endif()
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-writable-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
	endif()
endif()

if (NOT CMAKE_BUILD_TYPE)
	message("No build type selected, default to RELEASE")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# Settings
if(BuildPortableVersion)
	set(SharedDefines ${SharedDefines} "_PORTABLE_VERSION")
endif()

if(UseInternalJPEG)
	set(SharedDefines ${SharedDefines} "USE_INTERNAL_JPEG")
endif()

if(UseInternalZlib)
	set(SharedDefines ${SharedDefines} "USE_INTERNAL_ZLIB")
endif()

set(OpenJKLibDir "${CMAKE_SOURCE_DIR}/lib")

if(NOT MSVC)
	# CMake already defines _DEBUG for MSVC.
	set(DebugDefines ${DebugDefines} "_DEBUG")
endif()

set(ReleaseDefines "FINAL_BUILD")

# Add projects
add_subdirectory(${SPDir})
if(BuildJK2SPGame)
	add_subdirectory("${JK2SPDir}/game")
endif()
add_subdirectory(${MPDir})
