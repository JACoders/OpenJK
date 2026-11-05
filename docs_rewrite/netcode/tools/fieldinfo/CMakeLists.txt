cmake_minimum_required(VERSION 3.10)
project(fieldinfo CXX)

set(CMAKE_CXX_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# repository root: docs_rewrite/netcode/tools/fieldinfo -> ../../../..
get_filename_component(REPO ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)

add_executable(fieldinfo fieldinfo.cpp)
# same include directories as the engine for q_shared.h
target_include_directories(fieldinfo PRIVATE ${REPO}/codemp ${REPO}/shared)
if(MSVC)
  target_compile_definitions(fieldinfo PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()
