cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPRend2IncludeDirectories ${MPDir})
set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} "${MPDir}/rd-rend2")

set(MPRend2Files
	"${MPDir}/rd-rend2/iqm.h"
	"${MPDir}/rd-rend2/qgl.h"
	"${MPDir}/rd-rend2/G2_API.cpp"
	"${MPDir}/rd-rend2/G2_bolts.cpp"
	"${MPDir}/rd-rend2/G2_bones.cpp"
	"${MPDir}/rd-rend2/G2_local.h"
	"${MPDir}/rd-rend2/G2_misc.cpp"
	"${MPDir}/rd-rend2/G2_surfaces.cpp"
	"${MPDir}/rd-rend2/glsl_shaders.cpp"
	"${MPDir}/rd-rend2/tr_animation.cpp"
	"${MPDir}/rd-rend2/tr_backend.cpp"
	"${MPDir}/rd-rend2/tr_bsp.cpp"
	"${MPDir}/rd-rend2/tr_cache.cpp"
	"${MPDir}/rd-rend2/tr_cmds.cpp"
	"${MPDir}/rd-rend2/tr_curve.cpp"
	"${MPDir}/rd-rend2/tr_extensions.cpp"
	"${MPDir}/rd-rend2/tr_extramath.cpp"
	"${MPDir}/rd-rend2/tr_extramath.h"
	"${MPDir}/rd-rend2/tr_extratypes.h"
	"${MPDir}/rd-rend2/tr_fbo.cpp"
	"${MPDir}/rd-rend2/tr_fbo.h"
	"${MPDir}/rd-rend2/tr_flares.cpp"
	"${MPDir}/rd-rend2/tr_ghoul2.cpp"
	"${MPDir}/rd-rend2/tr_glsl.cpp"
	"${MPDir}/rd-rend2/tr_image.cpp"
	"${MPDir}/rd-rend2/tr_init.cpp"
	"${MPDir}/rd-rend2/tr_light.cpp"
	"${MPDir}/rd-rend2/tr_local.h"
	"${MPDir}/rd-rend2/tr_main.cpp"
	"${MPDir}/rd-rend2/tr_marks.cpp"
	"${MPDir}/rd-rend2/tr_mesh.cpp"
	"${MPDir}/rd-rend2/tr_model.cpp"
	"${MPDir}/rd-rend2/tr_model_iqm.cpp"
	"${MPDir}/rd-rend2/tr_postprocess.cpp"
	"${MPDir}/rd-rend2/tr_postprocess.h"
	"${MPDir}/rd-rend2/tr_scene.cpp"
	"${MPDir}/rd-rend2/tr_shade.cpp"
	"${MPDir}/rd-rend2/tr_shade_calc.cpp"
	"${MPDir}/rd-rend2/tr_shader.cpp"
	"${MPDir}/rd-rend2/tr_shadows.cpp"
	"${MPDir}/rd-rend2/tr_skin.cpp"
	"${MPDir}/rd-rend2/tr_sky.cpp"
	"${MPDir}/rd-rend2/tr_subs.cpp"
	"${MPDir}/rd-rend2/tr_surface.cpp"
	"${MPDir}/rd-rend2/tr_vbo.cpp"
	"${MPDir}/rd-rend2/tr_world.cpp"
	)
source_group("renderer" FILES ${MPRend2Files})

set(MPRend2Ghoul2Files
	"${MPDir}/ghoul2/G2_gore.cpp"
	"${MPDir}/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPRend2Ghoul2Files})
set(MPRend2Files ${MPRend2Files} ${MPRend2Ghoul2Files})

set(MPRend2RdCommonFiles
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
	"${MPDir}/rd-common/tr_public.h")
source_group("rd-common" FILES ${MPRend2RdCommonFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2RdCommonFiles})

set(MPRend2QcommonFiles
	"${MPDir}/qcommon/GenericParser2.cpp"
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("qcommon" FILES ${MPRend2QcommonFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2QcommonFiles})

file(GLOB_RECURSE MPRend2JpegFiles "${OpenJKLibDir}/jpeg-8c/*.c" "${OpenJKLibDir}/jpeg-8c/*.h")
source_group("jpeg-8c" FILES ${MPRend2JpegFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2JpegFiles})

if(UseInternalPNG)
	set(MPRend2LibPngFiles
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
	source_group("libpng" FILES ${MPRend2LibPngFiles})
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} "${OpenJKLibDir}/libpng")
	set(MPRend2Files ${MPRend2Files} ${MPRend2LibPngFiles})
else(UseInternalPNG)
	find_package(PNG REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${PNG_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${PNG_LIBRARIES})
endif(UseInternalPNG)

if(UseInternalZlib)
	# zlib.lib is included for windows
	find_library(ZlibLibrary NAMES zlib PATHS ${OpenJKLibDir})
	if(NOT ZlibLibrary)
		message(FATAL_ERROR "UseInternalZlib enabled, but lib/zlib.lib not found!")
	endif(NOT ZlibLibrary)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} "${OpenJKLibDir}/zlib")
	set(MPRend2Libraries ${MPRend2Libraries} ${ZlibLibrary})
else(UseInternalZlib)
	find_package(ZLIB REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${ZLIB_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${ZLIB_LIBRARY})
endif(UseInternalZlib)

if(NOT WIN32)
	find_package(OpenGL REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${OPENGL_LIBRARIES})

	find_package(SDL2 REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${SDL2_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${SDL2_LIBRARY})
endif(NOT WIN32)


if(WIN32)
	set(MPRend2Win32Files
		"${MPDir}/win32/win_gamma.cpp"
		"${MPDir}/win32/win_glimp.cpp"
		"${MPDir}/win32/win_qgl.cpp")
	source_group("win32" FILES ${MPRend2Win32Files})
	set(MPRend2Files ${MPRend2Files} ${MPRend2Win32Files})
else(WIN32)
	set(MPRend2SysFiles
		"${MPDir}/sdl/sdl_glimp.cpp")
	source_group("sys" FILES ${MPRend2SysFiles})
	set(MPRend2Files ${MPRend2Files} ${MPRend2SysFiles})
endif(WIN32)

set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${OpenJKLibDir})
add_library(${MPRend2} SHARED ${MPRend2Files})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPRend2} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPRend2} RUNTIME DESTINATION ".")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPRend2} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPRend2} LIBRARY DESTINATION ".")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPRend2} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPRend2} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPRend2} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPRend2} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SharedDefines};${DebugDefines}")
set_target_properties(${MPRend2} PROPERTIES INCLUDE_DIRECTORIES "${MPRend2IncludeDirectories}")
set_target_properties(${MPRend2} PROPERTIES PROJECT_LABEL "MP Rend2 Renderer")
target_link_libraries(${MPRend2} ${MPRend2Libraries})
