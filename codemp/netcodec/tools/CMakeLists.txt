# Tools of the format: netcodec_train trains the tables on the corpus traces (docs_rewrite/netcode/tools/python/
# export_demo_traces.py) and writes src/generated/jka_tables.inc.
netcodec_add_program(netcodec_train train.cpp)
target_include_directories(netcodec_train PRIVATE "${PROJECT_SOURCE_DIR}/test" "${PROJECT_SOURCE_DIR}/src")
