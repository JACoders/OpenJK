cmake_minimum_required(VERSION 2.8)
# For checks in subdirectories
set(InOpenJK TRUE)

#    Project name
project("OpenJK")

#    Customizable options
#option(InstallDir "Where to install the binaries to" "${CMAKE_SOURCE_DIR}/install") # just use CMAKE_INSTALL_PREFIX
option(BuildMPEngine "Whether to create projects for the mp engine (jamp.exe)" ON)
option(BuildMPDed "Whether to create projects for the dedicated server (jampded.exe)" ON)
option(BuildMPGame "Whether to create projects for the mp serverside gamecode (jampgamex86.dll)" ON)
option(BuildMPCGame "Whether to create projects for the mp clientside gamecode (jampcgamex86.dll)" ON)
option(BuildMPUI "Whether to create projects for the mp ui code (jampuix86.dll)" ON)
if(WIN32)
	option(UseInternalOpenAL "Whether to use the included OpenAL instead of a locally installed one" ON)
	option(UseInternalZlib "Whether to use the included zlib instead of a locally installed one" ON)
endif(WIN32)
# External version not yet supported, WIP
# option(UseInternalJpeg "Whether to use the included libjpeg instead of a locally installed one" ON)
# option(UseInternalPng "Whether to use the included libpng instead of a locally installed one" ON)

# Custom CMake Modules needed
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

# Arch Suffix
# TODO don't hardcode
set(Architecture "x86")

# Binary names
set(SPEngine "openjk_sp.${Architecture}")
set(SPGame "jagame${Architecture}")
set(MPEngine "openjk.${Architecture}")
set(MPDed "openjkded.${Architecture}")
set(MPGame "jampgame${Architecture}")
set(MPCGame "cgame${Architecture}")
set(MPUI "ui${Architecture}")
# Library names
set(MPBotLib "botlib")

# Paths
set(SPDir "${CMAKE_SOURCE_DIR}/code")
set(MPDir "${CMAKE_SOURCE_DIR}/codemp")

# Common settings
if(WIN32)
	set(SharedDefines "WIN32" "_WINDOWS")
else(WIN32)
	set(SharedDefines "__linux__" "_M_IX86")

	# removes the -rdynamic flag at linking (which causes crashes for some reason)
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")


	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -fpermissive")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -fpermissive")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")

	# there are some assembly files to build
	enable_language(ASM_NASM)
	
	if (NOT CMAKE_BUILD_TYPE)
 		message("No build type selected, default to RELEASE")
 		set(CMAKE_BUILD_TYPE "RELEASE")
 	endif()
endif(WIN32)

set(DebugDefines "_DEBUG")
set(ReleaseDefines "NDEBUG" "FINAL_BUILD")

#    Add projects
#add_subdirectory(${SPDir})
add_subdirectory(${MPDir})
