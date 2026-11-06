# Golden tests against the Python reference codec. The traces are generated locally (they come from recordings and
# are not committed): docs_rewrite/netcode/tools/python/export_rans_trace.py and export_codec_trace.py.
set(netcodec_TRACE_DIR "${PROJECT_SOURCE_DIR}/../../docs_rewrite/netcode/data/traces" CACHE PATH "Directory of the golden traces")

netcodec_add_program(netcodec_test_rans test_rans.cpp rans_trace.h)
netcodec_add_program(netcodec_test_codec test_codec.cpp codec_trace.h message_compare.h)

# Round-trip test of the format on the corpus traces (export_demo_traces.py): white-box (it reads the schema to
# make -0.0 values), so it sees the library's internal headers.
netcodec_add_program(netcodec_test_roundtrip test_roundtrip.cpp demo_trace.h message_compare.h)
target_include_directories(netcodec_test_roundtrip PRIVATE "${PROJECT_SOURCE_DIR}/src")

foreach(trace rans codec)
	if(NOT EXISTS "${netcodec_TRACE_DIR}/${trace}_trace.bin")
		message(STATUS "netcodec: ${netcodec_TRACE_DIR}/${trace}_trace.bin not found, test netcodec.${trace}_golden will fail")
	endif()
	add_test(NAME netcodec.${trace}_golden COMMAND netcodec_test_${trace} "${netcodec_TRACE_DIR}/${trace}_trace.bin")
endforeach()
foreach(trace captures holdout)
	if(NOT EXISTS "${netcodec_TRACE_DIR}/${trace}.bin")
		message(STATUS "netcodec: ${netcodec_TRACE_DIR}/${trace}.bin not found, test netcodec.roundtrip will fail")
	endif()
endforeach()
add_test(NAME netcodec.roundtrip COMMAND netcodec_test_roundtrip "${netcodec_TRACE_DIR}/captures.bin" "${netcodec_TRACE_DIR}/holdout.bin")
