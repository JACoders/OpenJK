# netcodec: snapshot and message codec of the new netcode (docs_rewrite/netcode/PLAN.md, step 6).
#
# Layout after cpp-best-practices/cmake_template: include/ (public headers), src/, test/, bench/, cmake/.
# Two uses:
#  - top-level project (development, tests, benchmarks): see CMakePresets.json, for example
#      cmake --preset gcc-release && cmake --build --preset gcc-release && ctest --preset gcc-release
#  - inside OpenJK (add_subdirectory): only the library target netcodec::netcodec; no tests, no warnings as errors,
#    and none of the development settings below reach the engine's targets.
cmake_minimum_required(VERSION 3.21)

project(netcodec VERSION 0.1.0 DESCRIPTION "JKA netcode snapshot and message codec" LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
	file(REAL_PATH "${CMAKE_SOURCE_DIR}" netcodec_src)
	file(REAL_PATH "${CMAKE_BINARY_DIR}" netcodec_bin)
	if(netcodec_src STREQUAL netcodec_bin)
		message(FATAL_ERROR "netcodec: in-source builds are disabled; use a build directory or a preset")
	endif()
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	endif()
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clang-tidy and editors
endif()

option(netcodec_BUILD_TESTS "Build the netcodec tests" ${PROJECT_IS_TOP_LEVEL})
option(netcodec_BUILD_BENCHMARKS "Build the netcodec benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(netcodec_WARNINGS_AS_ERRORS "Treat warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(netcodec_ENABLE_SANITIZER_ADDRESS "AddressSanitizer" OFF)
option(netcodec_ENABLE_SANITIZER_UNDEFINED "UndefinedBehaviorSanitizer" OFF)
option(netcodec_ENABLE_CLANG_TIDY "Run clang-tidy on the library (checks in .clang-tidy)" OFF)
option(netcodec_ENABLE_CPPCHECK "Run cppcheck on the library" OFF)

include(cmake/Warnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/StaticAnalyzers.cmake)

# Settings shared by every netcodec target, linked privately: C++20; floating-point determinism (the codec's float
# tests must give the same bits on every compiler: no FMA contraction, SSE2 instead of x87 on 32-bit x86); index
# checks of std::span and the containers in debug builds (libstdc++, libc++; MSVC checks by default in Debug).
add_library(netcodec_options INTERFACE)
target_compile_features(netcodec_options INTERFACE cxx_std_20)
target_compile_definitions(netcodec_options INTERFACE
	$<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>)
if(MSVC)
	target_compile_options(netcodec_options INTERFACE /fp:precise)
else()
	target_compile_options(netcodec_options INTERFACE -ffp-contract=off)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		target_compile_options(netcodec_options INTERFACE -msse2 -mfpmath=sse)
	endif()
endif()
netcodec_enable_sanitizers(netcodec_options ${netcodec_ENABLE_SANITIZER_ADDRESS} ${netcodec_ENABLE_SANITIZER_UNDEFINED})

add_library(netcodec_warnings INTERFACE)
netcodec_set_warnings(netcodec_warnings ${netcodec_WARNINGS_AS_ERRORS})

add_library(netcodec STATIC
	src/codec.cpp
	src/generated/jka_codec.inc
	include/netcodec/codec.h
	include/netcodec/rans.h)
add_library(netcodec::netcodec ALIAS netcodec)
target_include_directories(netcodec PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(netcodec PUBLIC cxx_std_20)
set_target_properties(netcodec PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(netcodec PRIVATE netcodec_options netcodec_warnings)
if(netcodec_ENABLE_CLANG_TIDY)
	netcodec_enable_clang_tidy(netcodec ${netcodec_WARNINGS_AS_ERRORS})
endif()
if(netcodec_ENABLE_CPPCHECK)
	netcodec_enable_cppcheck(netcodec ${netcodec_WARNINGS_AS_ERRORS})
endif()

# Programs of the tests and benchmarks: same settings as the library; static runtime with MinGW (no DLL to find).
function(netcodec_add_program name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE netcodec netcodec_options netcodec_warnings)
	set_target_properties(${name} PROPERTIES CXX_EXTENSIONS OFF)
	if(MSVC)
		target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)   # fopen
	endif()
	if(MINGW)
		target_link_options(${name} PRIVATE -static)
	endif()
endfunction()

if(netcodec_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
if(netcodec_BUILD_BENCHMARKS)
	add_subdirectory(bench)
endif()
