cmake_minimum_required(VERSION 3.10)
project(jkabench C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# own coders (A engine Huffman, B static Huffman, C rANS, D adaptive binary)
add_executable(jkabench bench.c)

# third-party libraries (sources cloned under third_party/, see README of each for its licence)
set(TP ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(FSE ${TP}/FiniteStateEntropy/lib)

add_library(fse STATIC
  ${FSE}/fse_compress.c
  ${FSE}/fse_decompress.c
  ${FSE}/entropy_common.c
  ${FSE}/hist.c
  ${FSE}/debug.c)
target_include_directories(fse PUBLIC ${FSE})

add_library(nibrans_c STATIC nibrans_impl.c)
target_include_directories(nibrans_c PUBLIC ${TP}/nibrans)
set_target_properties(nibrans_c PROPERTIES C_STANDARD 11)

add_executable(jkabench_libs bench_libs.cpp)
target_include_directories(jkabench_libs PRIVATE ${TP}/ryg_rans ${TP}/qbc)
target_link_libraries(jkabench_libs PRIVATE fse nibrans_c)

if(MSVC)
  target_compile_options(nibrans_c PRIVATE /O2 /std:c11)
  target_compile_options(jkabench PRIVATE /O2 /W3)
  target_compile_options(fse PRIVATE /O2)
  target_compile_options(jkabench_libs PRIVATE /O2 /W3)
else()
  target_compile_options(nibrans_c PRIVATE -O2 -msse2)
  target_compile_options(jkabench PRIVATE -O2 -Wall)
  target_compile_options(fse PRIVATE -O2)
  target_compile_options(jkabench_libs PRIVATE -O2 -Wall -msse2)
endif()
