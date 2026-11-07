# Determinism test of the game's player movement (pmove_det.c): bg_pmove.c and its files built as the game module
# (_GAME) and as cgame (_CGAME), with OpenJK's own compiler settings for the modules (not netcodec's: the point is
# how the shipped game code behaves), and the engine's Sys_SnapVector. Its output is compared between builds and
# compilers (validate.sh, validate.bat).
set(netcodec_OPENJK_DIR "${PROJECT_SOURCE_DIR}/../.." CACHE PATH "OpenJK source tree, for the legacy benchmark")
set(netcodec_ojk "${netcodec_OPENJK_DIR}")
set(netcodec_pmove_sources
	"${netcodec_ojk}/codemp/game/bg_g2_utils.c"
	"${netcodec_ojk}/codemp/game/bg_misc.c"
	"${netcodec_ojk}/codemp/game/bg_panimate.c"
	"${netcodec_ojk}/codemp/game/bg_pmove.c"
	"${netcodec_ojk}/codemp/game/bg_saber.c"
	"${netcodec_ojk}/codemp/game/bg_saberLoad.c"
	"${netcodec_ojk}/codemp/game/bg_saga.c"
	"${netcodec_ojk}/codemp/game/bg_slidemove.c"
	"${netcodec_ojk}/codemp/game/bg_vehicleLoad.c"
	"${netcodec_ojk}/codemp/game/bg_weapons.c"
	"${netcodec_ojk}/codemp/qcommon/q_shared.c"
	"${netcodec_ojk}/shared/qcommon/q_math.c"
	"${netcodec_ojk}/shared/qcommon/q_string.c"
	"${netcodec_ojk}/shared/qcommon/q_color.c"
	"${netcodec_ojk}/shared/sys/snapvector.cpp"
	pmove_det.c)

foreach(side game cgame)
	set(t netcodec_pmove_${side})
	add_executable(${t} ${netcodec_pmove_sources})
	string(TOUPPER "_${side}" def)
	target_compile_definitions(${t} PRIVATE ${def})
	# OpenJK's SharedDefines (top-level CMakeLists.txt)
	target_compile_definitions(${t} PRIVATE $<IF:$<CONFIG:Debug>,_DEBUG,FINAL_BUILD>)
	if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
		target_compile_definitions(${t} PRIVATE WIN64)
	elseif(NOT WIN32)
		target_compile_definitions(${t} PRIVATE "ARCH_STRING=\"$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x86_64,i386>\"")
	endif()
	if(MSVC)
		target_compile_definitions(${t} PRIVATE NOMINMAX _CRT_NONSTDC_NO_DEPRECATE)
	endif()
	target_include_directories(${t} PRIVATE "${netcodec_ojk}/codemp" "${netcodec_ojk}/shared" include)
	# OpenJK's settings for C (top-level CMakeLists.txt): SSE2 math on 32-bit x86, signed char, -O3 in Release
	if(MSVC)
		target_compile_options(${t} PRIVATE /W0)   # /W0, not /w: CMake maps /W0 to the project's warning level (/w comes after MSBuild's default /W1: D9025)
		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			target_compile_options(${t} PRIVATE /arch:SSE2)
		endif()
		target_compile_definitions(${t} PRIVATE _CRT_SECURE_NO_WARNINGS)
	else()
		target_compile_options(${t} PRIVATE -w -fsigned-char $<$<CONFIG:Release>:-O3>)
		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			target_compile_options(${t} PRIVATE -msse2 -mfpmath=sse -mstackrealign)
		endif()
		target_link_libraries(${t} PRIVATE m)
	endif()
	if(MINGW)
		target_link_options(${t} PRIVATE -static)
	endif()
	add_test(NAME netcodec.pmove_${side} COMMAND ${t} 20000)
endforeach()

# The original 1.01 game code (E:\Git\jka\jediacademy, next to this repository; on the VMs' shared folders too), built
# as 1.01 was: 32-bit with x87 math (Visual C++ 7.1 for Windows, GCC for the Linux server), QAGAME, its own rounding.
# Only on 32-bit presets: the original headers hold x86 inline assembly.
set(netcodec_JKA101_DIR "${netcodec_OPENJK_DIR}/../jediacademy" CACHE PATH "Original 1.01 source tree, for the pmove test")
if(EXISTS "${netcodec_JKA101_DIR}/codemp/game/bg_pmove.c" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(g "${netcodec_JKA101_DIR}/codemp/game")
	add_executable(netcodec_pmove_101 "${g}/bg_g2_utils.c" "${g}/bg_misc.c" "${g}/bg_panimate.c" "${g}/bg_pmove.c" "${g}/bg_saber.c"
		"${g}/bg_saberLoad.c" "${g}/bg_saga.c" "${g}/bg_slidemove.c" "${g}/bg_vehicleLoad.c" "${g}/bg_weapons.c" "${g}/q_math.c"
		"${g}/q_shared.c" pmove_det.c stubs101.c)
	# C_ONLY: the C versions of q_math.c's assembly functions (collision helpers that pmove does not call)
	target_compile_definitions(netcodec_pmove_101 PRIVATE JKA101 QAGAME MISSIONPACK _JK2 FINAL_BUILD C_ONLY $<$<BOOL:${WIN32}>:WIN32>)
	target_include_directories(netcodec_pmove_101 PRIVATE "${g}")
	if(MSVC)
		target_compile_options(netcodec_pmove_101 PRIVATE /W0 /arch:IA32 "/FI${CMAKE_CURRENT_SOURCE_DIR}/prelude101.h")
		target_compile_definitions(netcodec_pmove_101 PRIVATE _CRT_SECURE_NO_WARNINGS)
	else()
		target_compile_options(netcodec_pmove_101 PRIVATE -w -fsigned-char -include "${CMAKE_CURRENT_SOURCE_DIR}/prelude101.h"
			$<$<CONFIG:Release>:-O2>)
		if(CMAKE_C_COMPILER_ID MATCHES "Clang")
			target_compile_options(netcodec_pmove_101 PRIVATE -fms-extensions -fasm-blocks -mno-sse)   # x87 math
		else()
			target_compile_options(netcodec_pmove_101 PRIVATE -mfpmath=387)   # x87 math
		endif()
		target_link_libraries(netcodec_pmove_101 PRIVATE m)
	endif()
	if(MINGW)
		target_link_options(netcodec_pmove_101 PRIVATE -static)
	endif()
	add_test(NAME netcodec.pmove_101 COMMAND netcodec_pmove_101 20000)
endif()
