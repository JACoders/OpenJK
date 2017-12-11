cmake_minimum_required(VERSION 2.8.8)

add_executable(GetExeSymbolDir
    get_exe_dir.cpp
    )
add_executable(GetPdbSymbolDir
    get_pdb_dir.cpp
    )
target_link_libraries(GetExeSymbolDir Dbghelp)
target_link_libraries(GetPdbSymbolDir Dbghelp)