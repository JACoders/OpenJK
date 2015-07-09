# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPRend2IncludeDirectories ${MPDir})
set(MPRend2IncludeDirectories
	${MPRend2IncludeDirectories}
	${SharedDir}
	"${MPDir}/rd-rend2"
	)

set(MPRend2Files
	"${MPDir}/rd-rend2/iqm.h"
	"${MPDir}/rd-rend2/glext.h"
	"${MPDir}/rd-rend2/qgl.h"
	"${MPDir}/rd-rend2/G2_API.cpp"
	"${MPDir}/rd-rend2/G2_bolts.cpp"
	"${MPDir}/rd-rend2/G2_bones.cpp"
	"${MPDir}/rd-rend2/G2_misc.cpp"
	"${MPDir}/rd-rend2/G2_surfaces.cpp"
	"${MPDir}/rd-rend2/tr_animation.cpp"
	"${MPDir}/rd-rend2/tr_backend.cpp"
	"${MPDir}/rd-rend2/tr_bsp.cpp"
	"${MPDir}/rd-rend2/tr_cache.cpp"
	"${MPDir}/rd-rend2/tr_cmds.cpp"
	"${MPDir}/rd-rend2/tr_curve.cpp"
	"${MPDir}/rd-rend2/tr_decals.cpp"
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

file(GLOB MPRend2GLSLFiles "${MPDir}/rd-rend2/glsl/*.glsl")
source_group("renderer\\glsl" FILES ${MPRend2GLSLFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2GLSLFiles})

set(MPRend2Ghoul2Files
	"${MPDir}/ghoul2/g2_local.h"
	"${MPDir}/ghoul2/ghoul2_shared.h"
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
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h"
	"${MPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${MPRend2RdCommonFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2RdCommonFiles})

set(MPRend2CommonFiles
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("common" FILES ${MPRend2CommonFiles})
set(MPRend2Files ${MPRend2Files} ${MPRend2CommonFiles})

if(UseInternalJPEG)
	file(GLOB_RECURSE MPRend2JpegFiles "${OpenJKLibDir}/jpeg-8c/*.c" "${OpenJKLibDir}/jpeg-8c/*.h")
	source_group("jpeg-8c" FILES ${MPRend2JpegFiles})
	set(MPRend2Files ${MPRend2Files} ${MPRend2JpegFiles})
else()
	find_package(JPEG REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${JPEG_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${JPEG_LIBRARIES})
endif()

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
	set(MPRend2ZLibFiles
		"${OpenJKLibDir}/zlib/adler32.c"
		"${OpenJKLibDir}/zlib/compress.c"
		"${OpenJKLibDir}/zlib/crc32.c"
		"${OpenJKLibDir}/zlib/deflate.c"
		"${OpenJKLibDir}/zlib/gzclose.c"
		"${OpenJKLibDir}/zlib/gzlib.c"
		"${OpenJKLibDir}/zlib/gzread.c"
		"${OpenJKLibDir}/zlib/gzwrite.c"
		"${OpenJKLibDir}/zlib/infback.c"
		"${OpenJKLibDir}/zlib/inffast.c"
		"${OpenJKLibDir}/zlib/inflate.c"
		"${OpenJKLibDir}/zlib/inftrees.c"
		"${OpenJKLibDir}/zlib/trees.c"
		"${OpenJKLibDir}/zlib/uncompr.c"
		"${OpenJKLibDir}/zlib/zutil.c"
		"${OpenJKLibDir}/zlib/crc32.h"
		"${OpenJKLibDir}/zlib/deflate.h"
		"${OpenJKLibDir}/zlib/gzguts.h"
		"${OpenJKLibDir}/zlib/inffast.h"
		"${OpenJKLibDir}/zlib/inffixed.h"
		"${OpenJKLibDir}/zlib/inflate.h"
		"${OpenJKLibDir}/zlib/inftrees.h"
		"${OpenJKLibDir}/zlib/trees.h"
		"${OpenJKLibDir}/zlib/zconf.h"
		"${OpenJKLibDir}/zlib/zlib.h"
		"${OpenJKLibDir}/zlib/zutil.h")
	source_group("zlib" FILES ${MPRend2ZLibFiles})
	set(MPRend2Files ${MPRend2Files} ${MPRend2ZLibFiles})
	
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} "${OpenJKLibDir}/zlib")
else(UseInternalZlib)
	find_package(ZLIB REQUIRED)
	set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${ZLIB_INCLUDE_DIR})
	set(MPRend2Libraries ${MPRend2Libraries} ${ZLIB_LIBRARY})
endif(UseInternalZlib)

find_package(OpenGL REQUIRED)
set(MPRend2IncludeDirectories ${MPRend2IncludeDirectories} ${OPENGL_INCLUDE_DIR})
set(MPRend2Libraries ${MPRend2Libraries} ${OPENGL_LIBRARIES})

source_group("renderer" FILES ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)
set(MPRend2Files ${MPRend2Files} ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)

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

# GLSL shader file generator
add_executable(compact_glsl ${MPDir}/rd-rend2/glsl/compact.cpp)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp
	COMMAND compact_glsl ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp ${MPRend2GLSLFiles}
	DEPENDS compact_glsl ${MPRend2GLSLFiles})
