cmake_minimum_required(VERSION 2.8.8)

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
	"${MPDir}/rd-vanilla/G2_misc.cpp"
	"${MPDir}/rd-vanilla/G2_surfaces.cpp"
	"${MPDir}/rd-vanilla/tr_arioche.cpp"
	"${MPDir}/rd-vanilla/tr_backend.cpp"
	"${MPDir}/rd-vanilla/tr_bsp.cpp"
	"${MPDir}/rd-vanilla/tr_cmds.cpp"
	"${MPDir}/rd-vanilla/tr_curve.cpp"
	"${MPDir}/rd-vanilla/tr_decals.cpp"
	"${MPDir}/rd-vanilla/tr_ghoul2.cpp"
	"${MPDir}/rd-vanilla/tr_image.cpp"
	"${MPDir}/rd-vanilla/tr_init.cpp"
	"${MPDir}/rd-vanilla/tr_landscape.h"
	"${MPDir}/rd-vanilla/tr_light.cpp"
	"${MPDir}/rd-vanilla/tr_local.h"
	"${MPDir}/rd-vanilla/tr_main.cpp"
	"${MPDir}/rd-vanilla/tr_marks.cpp"
	"${MPDir}/rd-vanilla/tr_mesh.cpp"
	"${MPDir}/rd-vanilla/tr_model.cpp"
	"${MPDir}/rd-vanilla/tr_quicksprite.cpp"
	"${MPDir}/rd-vanilla/tr_quicksprite.h"
	"${MPDir}/rd-vanilla/tr_scene.cpp"
	"${MPDir}/rd-vanilla/tr_shade.cpp"
	"${MPDir}/rd-vanilla/tr_shade_calc.cpp"
	"${MPDir}/rd-vanilla/tr_shader.cpp"
	"${MPDir}/rd-vanilla/tr_shadows.cpp"
	"${MPDir}/rd-vanilla/tr_skin.cpp"
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

set(MPVanillaRendererGhoul2Files
	"${MPDir}/ghoul2/g2_local.h"
	"${MPDir}/ghoul2/ghoul2_shared.h"
	"${MPDir}/ghoul2/G2_gore.cpp"
	"${MPDir}/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPVanillaRendererGhoul2Files})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererGhoul2Files})

set(MPVanillaRendererRdCommonFiles
	"${MPDir}/rd-common/mdx_format.h"
	"${MPDir}/rd-common/tr_common.h"
	"${MPDir}/rd-common/tr_font.cpp"
	"${MPDir}/rd-common/tr_font.h"
	"${MPDir}/rd-common/tr_image_load.cpp"
	"${MPDir}/rd-common/tr_image_manipulation.cpp"
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h"
	"${MPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${MPVanillaRendererRdCommonFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererRdCommonFiles})

set(MPVanillaRendererQcommonFiles
	"${MPDir}/qcommon/GenericParser2.cpp"
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("qcommon" FILES ${MPVanillaRendererQcommonFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererQcommonFiles})

file(GLOB_RECURSE MPVanillaRendererJpegFiles "${OpenJKLibDir}/jpeg-8c/*.c" "${OpenJKLibDir}/jpeg-8c/*.h")
source_group("jpeg-8c" FILES ${MPVanillaRendererJpegFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererJpegFiles})

if(UseInternalPNG)
	set(MPVanillaRendererLibPngFiles
		"${OpenJKLibDir}/libpng/png.c"
		"${OpenJKLibDir}/libpng/pngerror.c"
		"${OpenJKLibDir}/libpng/pngget.c"
		"${OpenJKLibDir}/libpng/pngmem.c"
		"${OpenJKLibDir}/libpng/pngpread.c"
		"${OpenJKLibDir}/libpng/pngread.c"
		"${OpenJKLibDir}/libpng/pngrio.c"
		"${OpenJKLibDir}/libpng/pngrtran.c"
		"${OpenJKLibDir}/libpng/pngrutil.c"
		"${OpenJKLibDir}/libpng/pngset.c"
		"${OpenJKLibDir}/libpng/pngtrans.c"
		"${OpenJKLibDir}/libpng/pngwio.c"
		"${OpenJKLibDir}/libpng/pngwrite.c"
		"${OpenJKLibDir}/libpng/pngwtran.c"
		"${OpenJKLibDir}/libpng/pngwutil.c")
	source_group("libpng" FILES ${MPVanillaRendererLibPngFiles})
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} "${OpenJKLibDir}/libpng")
	set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererLibPngFiles})
else(UseInternalPNG)
	find_package(PNG REQUIRED)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${PNG_INCLUDE_DIR})
	set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${PNG_LIBRARIES})
endif(UseInternalPNG)

if(UseInternalZlib)
	# zlib.lib is included for windows
	find_library(ZlibLibrary NAMES zlib PATHS ${OpenJKLibDir})
	if(NOT ZlibLibrary)
		message(FATAL_ERROR "UseInternalZlib enabled, but lib/zlib.lib not found!")
	endif(NOT ZlibLibrary)
	set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} "${OpenJKLibDir}/zlib")
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

	set(SDL2_BUILDING_LIBRARY ON) # don't want main() in Renderer
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

set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${OpenJKLibDir})
add_library(${MPVanillaRenderer} SHARED ${MPVanillaRendererFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPVanillaRenderer} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPVanillaRenderer} RUNTIME DESTINATION ".")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPVanillaRenderer} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPVanillaRenderer} LIBRARY DESTINATION ".")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SharedDefines};${DebugDefines}")
set_target_properties(${MPVanillaRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPVanillaRendererIncludeDirectories}")
set_target_properties(${MPVanillaRenderer} PROPERTIES PROJECT_LABEL "MP Vanilla Renderer")
target_link_libraries(${MPVanillaRenderer} ${MPVanillaRendererLibraries})
