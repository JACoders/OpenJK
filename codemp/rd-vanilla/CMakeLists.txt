cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPVanillaRendererIncludeDirectories ${MPDir})
set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} "${MPDir}/rd-vanilla")

set(MPVanillaRendererFiles
	"${MPDir}/rd-vanilla/glext.h"
	"${MPDir}/rd-vanilla/qgl.h"
	"${MPDir}/rd-vanilla/G2_API.cpp"
	"${MPDir}/rd-vanilla/G2_bolts.cpp"
	"${MPDir}/rd-vanilla/G2_bones.cpp"
	"${MPDir}/rd-vanilla/G2_local.h"
	"${MPDir}/rd-vanilla/G2_misc.cpp"
	"${MPDir}/rd-vanilla/G2_surfaces.cpp"
	"${MPDir}/rd-vanilla/tr_animation.cpp"
	"${MPDir}/rd-vanilla/tr_arioche.cpp"
	"${MPDir}/rd-vanilla/tr_backend.cpp"
	"${MPDir}/rd-vanilla/tr_bsp.cpp"
	"${MPDir}/rd-vanilla/tr_cmds.cpp"
	"${MPDir}/rd-vanilla/tr_curve.cpp"
	"${MPDir}/rd-vanilla/tr_font.cpp"
	"${MPDir}/rd-vanilla/tr_ghoul2.cpp"
	"${MPDir}/rd-vanilla/tr_image.cpp"
	"${MPDir}/rd-vanilla/tr_init.cpp"
	"${MPDir}/rd-vanilla/tr_landscape.h"
	"${MPDir}/rd-vanilla/tr_light.cpp"
	"${MPDir}/rd-vanilla/tr_main.cpp"
	"${MPDir}/rd-vanilla/tr_marks.cpp"
	"${MPDir}/rd-vanilla/tr_mesh.cpp"
	"${MPDir}/rd-vanilla/tr_model.cpp"
	"${MPDir}/rd-vanilla/tr_noise.cpp"
	"${MPDir}/rd-vanilla/tr_quicksprite.cpp"
	"${MPDir}/rd-vanilla/tr_quicksprite.h"
	"${MPDir}/rd-vanilla/tr_scene.cpp"
	"${MPDir}/rd-vanilla/tr_shade.cpp"
	"${MPDir}/rd-vanilla/tr_shade_calc.cpp"
	"${MPDir}/rd-vanilla/tr_shader.cpp"
	"${MPDir}/rd-vanilla/tr_shadows.cpp"
	"${MPDir}/rd-vanilla/tr_sky.cpp"
	"${MPDir}/rd-vanilla/tr_subs.cpp"
	"${MPDir}/rd-vanilla/tr_surface.cpp"
	"${MPDir}/rd-vanilla/tr_surfacesprites.cpp"
	"${MPDir}/rd-vanilla/tr_terrain.cpp"
	"${MPDir}/rd-vanilla/tr_world.cpp"
	"${MPDir}/rd-vanilla/tr_WorldEffects.cpp"
	"${MPDir}/rd-vanilla/tr_WorldEffects.h"
	)
source_group("renderer" FILES ${MPVanillaRendererFiles})

set(MPVanillaRendererQcommonFiles
	"${MPDir}/qcommon/GenericParser2.cpp"
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("qcommon" FILES ${MPVanillaRendererQcommonFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererQcommonFiles})

file(GLOB_RECURSE MPVanillaRendererJpegFiles "${MPDir}/jpeg-8c/*.c" "${MPDir}/jpeg-8c/*.h")
source_group("jpeg-8c" FILES ${MPVanillaRendererJpegFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererJpegFiles})

set(MPVanillaRendererPngFiles
	"${MPDir}/png/rpng.cpp")
source_group("png" FILES ${MPVanillaRendererPngFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererPngFiles})

set(MPVanillaRendererLibPngFiles
	"${MPDir}/libpng/png.c"
	"${MPDir}/libpng/pngerror.c"
	"${MPDir}/libpng/pngget.c"
	"${MPDir}/libpng/pngmem.c"
	"${MPDir}/libpng/pngpread.c"
	"${MPDir}/libpng/pngread.c"
	"${MPDir}/libpng/pngrio.c"
	"${MPDir}/libpng/pngrtran.c"
	"${MPDir}/libpng/pngrutil.c"
	"${MPDir}/libpng/pngset.c"
	"${MPDir}/libpng/pngtrans.c"
	"${MPDir}/libpng/pngwio.c"
	"${MPDir}/libpng/pngwrite.c"
	"${MPDir}/libpng/pngwtran.c"
	"${MPDir}/libpng/pngwutil.c")
source_group("libpng" FILES ${MPVanillaRendererLibPngFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererLibPngFiles})

if(UseInternalZlib)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} "${MPDir}/zlib")
	# OpenAL32.lib is included for windows
	find_library(ZlibLibrary NAMES zlib PATHS "${MPDir}/libs/win32")
	if(NOT ZlibLibrary)
		message(FATAL_ERROR "UseInternalOpenAL set, but libs/OpenAL32 not found!")
	endif(NOT ZlibLibrary)
	set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${ZlibLibrary})
else(UseInternalZlib)
	find_package(ZLIB REQUIRED)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${ZLIB_INCLUDE_DIR})
	set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${ZLIB_LIBRARY})
endif(UseInternalZlib)

if(NOT WIN32)
	find_package(OpenGL REQUIRED)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${OPENGL_LIBRARIES})

	find_package(SDL2 REQUIRED)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${SDL2_INCLUDE_DIR})
	set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${SDL2_LIBRARY})
endif(NOT WIN32)


if(WIN32)
	set(MPVanillaRendererWin32Files
		"${MPDir}/win32/win_gamma.cpp"
		"${MPDir}/win32/win_glimp.cpp"
		"${MPDir}/win32/win_qgl.cpp")
	source_group("win32" FILES ${MPVanillaRendererWin32Files})
	set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererWin32Files})
else(WIN32)
	set(MPVanillaRendererSysFiles
		"${MPDir}/sdl/sdl_glimp.cpp")
	source_group("sys" FILES ${MPVanillaRendererSysFiles})
	set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererSysFiles})
endif(WIN32)

add_library(${MPVanillaRenderer} SHARED ${MPVanillaRendererFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPVanillaRenderer} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPVanillaRenderer} RUNTIME DESTINATION ".")
else(WIN32)
	install(TARGETS ${MPVanillaRenderer} LIBRARY DESTINATION ".")
endif(WIN32)
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SharedDefines};${DebugDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPVanillaRendererIncludeDirectories}")
target_link_libraries(${MPVanillaRenderer} ${MPVanillaRendererLibraries})
