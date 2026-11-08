# Benchmarks. ryg/rans_byte.h is the original ryg_rans header (public domain), kept unchanged for comparison:
# included as a system header so that the project's warnings do not apply to it.
netcodec_add_program(netcodec_bench_rans bench_rans.cpp)
target_include_directories(netcodec_bench_rans PRIVATE "${PROJECT_SOURCE_DIR}/test")
target_include_directories(netcodec_bench_rans SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ryg")

# Coded size of the corpus traces against the bytes the server sent (docs_rewrite/netcode/tools/python/export_demo_traces.py).
netcodec_add_program(netcodec_bench_size bench_size.cpp)
target_include_directories(netcodec_bench_size PRIVATE "${PROJECT_SOURCE_DIR}/test")

# Codec benchmark against the legacy coding: OpenJK's own codemp/qcommon/msg.cpp and huffman.cpp, built as the
# engine builds them (C++11, FINAL_BUILD, its headers), without the project's warnings (engine code, unchanged).
set(netcodec_OPENJK_DIR "${PROJECT_SOURCE_DIR}/../.." CACHE PATH "OpenJK source tree, for the legacy benchmark")
if(EXISTS "${netcodec_OPENJK_DIR}/codemp/qcommon/msg.cpp")
	add_library(netcodec_legacy OBJECT
		"${netcodec_OPENJK_DIR}/codemp/qcommon/msg.cpp"
		"${netcodec_OPENJK_DIR}/codemp/qcommon/huffman.cpp"
		legacy/legacy_codec.cpp
		legacy/legacy_stubs.cpp)
	set_target_properties(netcodec_legacy PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)
	target_include_directories(netcodec_legacy PRIVATE
		"${netcodec_OPENJK_DIR}/codemp" "${netcodec_OPENJK_DIR}/shared" "${netcodec_OPENJK_DIR}/lib/gsl-lite/include")
	target_compile_definitions(netcodec_legacy PRIVATE FINAL_BUILD $<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>)
	if(NOT WIN32)   # OpenJK's CMakeLists.txt defines it outside Windows (q_platform.h does on Windows)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_compile_definitions(netcodec_legacy PRIVATE ARCH_STRING="x86_64")
		else()
			target_compile_definitions(netcodec_legacy PRIVATE ARCH_STRING="i386")
		endif()
	endif()
	target_compile_options(netcodec_legacy PRIVATE $<IF:$<BOOL:${MSVC}>,/W0,-w>)
	# aligned as the library (CMakeLists.txt): the legacy timings are the A/B scripts' check of a busy machine, so they
	# must not move with where the code falls
	if(NOT MSVC)
		target_compile_options(netcodec_legacy PRIVATE -falign-functions=64 -falign-loops=64)
	endif()
	# not netcodec_options: its C++20 requirement would turn on MSVC's conformance mode, which the engine code does
	# not build with (the engine builds msg.cpp as C++11); the legacy code is not instrumented by the sanitizers
	if(MSVC AND netcodec_ENABLE_SANITIZER_ADDRESS)
		# except AddressSanitizer with MSVC: its standard library refuses to link objects built with and without it, or
		# with different container annotations (same settings as cmake/Sanitizers.cmake)
		target_compile_options(netcodec_legacy PRIVATE /fsanitize=address)
		target_compile_definitions(netcodec_legacy PRIVATE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
	endif()

	netcodec_add_program(netcodec_bench_codec bench_codec.cpp)
	target_include_directories(netcodec_bench_codec PRIVATE "${PROJECT_SOURCE_DIR}/test" "${CMAKE_CURRENT_SOURCE_DIR}")
	target_link_libraries(netcodec_bench_codec PRIVATE netcodec_legacy)
else()
	message(STATUS "netcodec: OpenJK sources not found in ${netcodec_OPENJK_DIR}, no legacy benchmark")
endif()
