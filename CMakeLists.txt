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

# Binary names
set(SPEngine "openjksp")
set(SPGame "jagame")
set(SPCGame "jacgame")
set(MPEngine "openjkmp")
set(MPDed "openjkded")
set(MPGame "jampgame")
set(MPCGame "jampcgame")
# Library names
set(MPBotLib "botlib")

# Paths
set(SPDir "${CMAKE_SOURCE_DIR}/code")
set(MPDir "${CMAKE_SOURCE_DIR}/codemp")

#    Add projects
#add_subdirectory(${SPDir})
add_subdirectory(${MPDir})