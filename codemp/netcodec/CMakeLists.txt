# netcodec: snapshot and message codec of the new netcode (docs_rewrite/netcode/PLAN.md, step 6).
# Builds inside OpenJK (add_subdirectory) or on its own:
#   cmake -S codemp/netcodec -B build-netcodec -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-netcodec
# Options: NETCODEC_TESTS (tests and benchmarks, on when built on its own), NETCODEC_SANITIZE (ASan + UBSan, GCC / Clang).
cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(netcodec CXX)
	set(NetcodecStandalone ON)
else()
	set(NetcodecStandalone OFF)
endif()

option(NETCODEC_TESTS "Build the netcodec tests and benchmarks" ${NetcodecStandalone})
option(NETCODEC_SANITIZE "Build netcodec with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(netcodec INTERFACE)
target_include_directories(netcodec INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(netcodec INTERFACE cxx_std_20)

if(NETCODEC_TESTS)
	function(netcodec_program name)
		add_executable(${name} ${ARGN})
		target_link_libraries(${name} PRIVATE netcodec)
		set_target_properties(${name} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
		# index checks of std::span and containers in debug builds (libstdc++, libc++; MSVC checks by default in Debug)
		target_compile_definitions(${name} PRIVATE $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG>)
		if(MSVC)
			target_compile_options(${name} PRIVATE /W4 /permissive-)
			target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)   # fopen in the test programs
		else()
			target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
			if(NETCODEC_SANITIZE)
				target_compile_options(${name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
				target_link_options(${name} PRIVATE -fsanitize=address,undefined)
			endif()
		endif()
	endfunction()

	netcodec_program(netcodec_test_rans tests/test_rans.cpp)
	netcodec_program(netcodec_bench_rans bench/bench_rans.cpp)
endif()
