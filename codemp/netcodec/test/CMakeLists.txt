# Tests on traces generated locally (they come from recordings and are not committed): the rANS layer against the
# Python reference coder (export_rans_trace.py), and the round trip of the format on the corpus (export_demo_traces.py).
set(netcodec_TRACE_DIR "${PROJECT_SOURCE_DIR}/../../docs_rewrite/netcode/data/traces" CACHE PATH "Directory of the test traces")

netcodec_add_program(netcodec_test_rans test_rans.cpp rans_trace.h)

# Round-trip test of the format on the corpus traces (export_demo_traces.py): white-box (it reads the schema to
# make -0.0 values), so it sees the library's internal headers.
netcodec_add_program(netcodec_test_roundtrip test_roundtrip.cpp demo_trace.h message_compare.h fnv.h)
target_include_directories(netcodec_test_roundtrip PRIVATE "${PROJECT_SOURCE_DIR}/src")

# Determinism of the float steps (angle indices) against the Python reference (angle_hash.py); needs no trace.
netcodec_add_program(netcodec_test_determinism test_determinism.cpp fnv.h)
target_include_directories(netcodec_test_determinism PRIVATE "${PROJECT_SOURCE_DIR}/src")
add_test(NAME netcodec.determinism COMMAND netcodec_test_determinism)

# Synthetic round trip (every operation, vehicles, pilots, extreme values, base ages up to 255): 100 streams of 1,000
# messages; needs no trace. The hash of the coded bytes changes with the format, the tables or the generator.
netcodec_add_program(netcodec_test_synthetic test_synthetic.cpp synthetic.h demo_trace.h message_compare.h fnv.h)
target_include_directories(netcodec_test_synthetic PRIVATE "${PROJECT_SOURCE_DIR}/src")
set(netcodec_SYNTHETIC_HASH "0x0C3939AD115606ED")
add_test(NAME netcodec.synthetic COMMAND netcodec_test_synthetic "--hash=${netcodec_SYNTHETIC_HASH}" 100 1000)

# Malformed input: mutated copies of synthetic messages decoded with the client's state (20 streams of 500 messages,
# 20 mutations each); no fault, and decode(encode(decode(x))) == decode(x) for those that decode. Needs no trace.
netcodec_add_program(netcodec_test_decode_robustness test_decode_robustness.cpp synthetic.h demo_trace.h message_compare.h)
target_include_directories(netcodec_test_decode_robustness PRIVATE "${PROJECT_SOURCE_DIR}/src")
add_test(NAME netcodec.decode_robustness COMMAND netcodec_test_decode_robustness 20 500 20)

foreach(trace rans)
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
# The hash of the coded bytes of the corpus: the same on every compiler, architecture and build type. Changes with the
# format, the tables or the traces: update it from the test's output (clang Release), then check the other presets.
set(netcodec_ROUNDTRIP_HASH "0x7E66AC625CE9148E")
add_test(NAME netcodec.roundtrip COMMAND netcodec_test_roundtrip "--hash=${netcodec_ROUNDTRIP_HASH}"
	"${netcodec_TRACE_DIR}/captures.bin" "${netcodec_TRACE_DIR}/holdout.bin")
