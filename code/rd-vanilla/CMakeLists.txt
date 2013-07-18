cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(SPRDVanillaRendererIncludeDirectories ${SPDir})
set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} "${SPDir}/rd-vanilla")

# Modular Renderer crap (let's hope this works..)

# Defines
set(SPRDVanillaDefines ${SharedDefines} "RENDERER" "_JK2EXE")

# Files

# JPEG
file(GLOB_RECURSE J_SRC "${OpenJKLibDir}/jpeg-8c/*.c" "${OpenJKLibDir}/jpeg-8c/*.h")
source_group("jpeg-8c" FILES ${J_SRC})
set(SPRDVanillaFiles ${SPRDVanillaFiles} ${J_SRC})

# GHOUL 2
set(SPRDVanillaG2Files
	"${SPDir}/ghoul2/G2.h"
	"${SPDir}/ghoul2/ghoul2_gore.h"
	"${SPDir}/rd-vanilla/ghoul2/G2_API.cpp"
	"${SPDir}/rd-vanilla/ghoul2/G2_bolts.cpp"
	"${SPDir}/rd-vanilla/ghoul2/G2_bones.cpp"
	"${SPDir}/rd-vanilla/ghoul2/G2_misc.cpp"
	"${SPDir}/rd-vanilla/ghoul2/G2_surfaces.cpp"
	)
source_group("ghoul2" FILES ${SPRDVanillaG2Files})
set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaG2Files})

if(UseInternalPNG)
	set(SPRDVanillaLibPngFiles
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
	source_group("libpng" FILES ${SPRDVanillaLibPngFiles})
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} "${OpenJKLibDir}/libpng")
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaLibPngFiles})
else()
	find_package(PNG REQUIRED)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${PNG_INCLUDE_DIR})
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${PNG_LIBRARIES})
endif(UseInternalPNG)

if(UseInternalZlib)
	# zlib.lib is included for windows
	find_library(ZlibLibrary NAMES zlib PATHS ${OpenJKLibDir})
	if(NOT ZlibLibrary)
		message(FATAL_ERROR "UseInternalZlib enabled, but lib/zlib.lib not found!")
	endif(NOT ZlibLibrary)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} "${OpenJKLibDir}/zlib")
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${ZlibLibrary})
else(UseInternalZlib)
	find_package(ZLIB REQUIRED)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${ZLIB_INCLUDE_DIR})
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${ZLIB_LIBRARY})
endif(UseInternalZlib)

# Misc files
set(SPRDVanillaGameFiles
	"${SPDir}/game/genericparser2.h"
	"${SPDir}/game/genericparser2.cpp"
	)
source_group("game" FILES ${SPRDVanillaGameFiles})
set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaGameFiles})

# Misc files
set(SPRDVanillaCommonFiles
	"${SPDir}/qcommon/q_math.cpp"
	"${SPDir}/qcommon/q_shared.cpp"
	"${SPDir}/qcommon/q_shared.h"
	"${SPDir}/qcommon/cm_landscape.h"
	)
source_group("common" FILES ${SPRDVanillaCommonFiles})
set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaCommonFiles})

# main files
set(SPRDVanillaMainFiles
	"${SPDir}/renderer/mdx_format.h"
	"${SPDir}/renderer/tr_public.h"
	"${SPDir}/renderer/tr_types.h"

	"${SPDir}/rd-vanilla/amd3d.h"
	"${SPDir}/rd-vanilla/glext.h"
	"${SPDir}/rd-vanilla/glext_console.h"
	"${SPDir}/rd-vanilla/matcomp.c"
	"${SPDir}/rd-vanilla/matcomp.h"
	"${SPDir}/rd-vanilla/qgl.h"
	"${SPDir}/rd-vanilla/qgl_linked.h"
	"${SPDir}/rd-vanilla/ref_trin.def"
	"${SPDir}/rd-vanilla/tr_animation.cpp"
	"${SPDir}/rd-vanilla/tr_arioche.cpp"
	"${SPDir}/rd-vanilla/tr_backend.cpp"
	"${SPDir}/rd-vanilla/tr_bsp.cpp"
	"${SPDir}/rd-vanilla/tr_cmds.cpp"
	"${SPDir}/rd-vanilla/tr_curve.cpp"
	"${SPDir}/rd-vanilla/tr_draw.cpp"
	"${SPDir}/rd-vanilla/tr_flares.cpp"
	"${SPDir}/rd-vanilla/tr_font.cpp"
	"${SPDir}/rd-vanilla/tr_font.h"
	"${SPDir}/rd-vanilla/tr_ghoul2.cpp"
	"${SPDir}/rd-vanilla/tr_image.cpp"
	"${SPDir}/rd-vanilla/tr_init.cpp"
	"${SPDir}/rd-vanilla/tr_jpeg_interface.cpp"
	"${SPDir}/rd-vanilla/tr_jpeg_interface.h"
	"${SPDir}/rd-vanilla/tr_landscape.h"
	"${SPDir}/rd-vanilla/tr_light.cpp"
	"${SPDir}/rd-vanilla/tr_local.h"
	"${SPDir}/rd-vanilla/tr_main.cpp"
	"${SPDir}/rd-vanilla/tr_marks.cpp"
	"${SPDir}/rd-vanilla/tr_mesh.cpp"
	"${SPDir}/rd-vanilla/tr_model.cpp"
	"${SPDir}/rd-vanilla/tr_noise.cpp"
	"${SPDir}/rd-vanilla/tr_quicksprite.cpp"
	"${SPDir}/rd-vanilla/tr_quicksprite.h"
	"${SPDir}/rd-vanilla/tr_scene.cpp"
	"${SPDir}/rd-vanilla/tr_shade.cpp"
	"${SPDir}/rd-vanilla/tr_shade_calc.cpp"
	"${SPDir}/rd-vanilla/tr_shader.cpp"
	"${SPDir}/rd-vanilla/tr_shadows.cpp"
	"${SPDir}/rd-vanilla/tr_sky.cpp"
	"${SPDir}/rd-vanilla/tr_stl.cpp"
	"${SPDir}/rd-vanilla/tr_stl.h"
	"${SPDir}/rd-vanilla/tr_subs.cpp"
	"${SPDir}/rd-vanilla/tr_surface.cpp"
	"${SPDir}/rd-vanilla/tr_surfacesprites.cpp"
	"${SPDir}/rd-vanilla/tr_terrain.cpp"
	"${SPDir}/rd-vanilla/tr_world.cpp"
	"${SPDir}/rd-vanilla/tr_WorldEffects.cpp"
	"${SPDir}/rd-vanilla/tr_WorldEffects.h"
	)
source_group("renderer" FILES ${SPRDVanillaMainFiles})
set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaMainFiles})

if(WIN32)
    set(SPRDVanillaWin32Files
        "${SPDir}/rd-vanilla/win_gamma.cpp"
        "${SPDir}/rd-vanilla/win_glimp.cpp"
        "${SPDir}/rd-vanilla/win_qgl.cpp"
        )
    source_group("win32" FILES ${SPRDVanillaWin32Files})
    set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaWin32Files})
else(WIN32)
    set(SPRDVanillaSdlFiles
        "${SPDir}/sdl/sdl_glimp.cpp")
    source_group("sdl" FILES ${SPRDVanillaSdlFiles})
    set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaSdlFiles})
endif(WIN32)

if(NOT WIN32)
	find_package(OpenGL REQUIRED)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${OPENGL_LIBRARIES})

	find_package(SDL2 REQUIRED)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${SDL2_INCLUDE_DIR})
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${SDL2_LIBRARY})
endif(NOT WIN32)

set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${OpenJKLibDir})
add_library(${SPRDVanillaRenderer} SHARED ${SPRDVanillaFiles})
if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${SPRDVanillaRenderer} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${SPRDVanillaRenderer} RUNTIME DESTINATION ".")
else(WIN32)
	if(MakeApplicationBundles AND BuildSPEngine)
		install(TARGETS ${SPRDVanillaRenderer} LIBRARY DESTINATION "${SPEngine}.app/Contents/MacOS/")
	else(MakeApplicationBundles AND BuildSPEngine)
		install(TARGETS ${SPRDVanillaRenderer} LIBRARY DESTINATION ".")
	endif(MakeApplicationBundles AND BuildSPEngine)
endif(WIN32)

set_target_properties(${SPRDVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SPRDVanillaDefines};${DebugDefines}")
set_target_properties(${SPRDVanillaRenderer} PROPERTIES INCLUDE_DIRECTORIES "${SPRDVanillaRendererIncludeDirectories}")

target_link_libraries(${SPRDVanillaRenderer} ${SPRDVanillaRendererLibraries})
