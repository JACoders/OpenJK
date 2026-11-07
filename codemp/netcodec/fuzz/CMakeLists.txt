# libFuzzer target on the message decoder (preset mingw-clang-fuzz; clang only). The library and the target are
# instrumented for coverage (see the top-level file); libFuzzer provides main().
netcodec_add_program(netcodec_fuzz_decode fuzz_decode.cpp)
target_include_directories(netcodec_fuzz_decode PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/test")
if(MINGW)
	# the driver refuses -fsanitize=fuzzer for this target: libFuzzer's archive from the compiler's resource directory
	execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir OUTPUT_VARIABLE netcodec_clang_res OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(netcodec_fuzz_arch x86_64)
	else()
		set(netcodec_fuzz_arch i386)
	endif()
	target_link_libraries(netcodec_fuzz_decode PRIVATE "${netcodec_clang_res}/lib/windows/libclang_rt.fuzzer-${netcodec_fuzz_arch}.a")
else()
	target_compile_options(netcodec_fuzz_decode PRIVATE -fsanitize=fuzzer)
	target_link_options(netcodec_fuzz_decode PRIVATE -fsanitize=fuzzer)
endif()
