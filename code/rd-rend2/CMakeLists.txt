# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(SPRend2IncludeDirectories
	"${SharedDir}"
	"${SharedDir}/rd-rend2"
	"${SPDir}"
	"${SPDir}/rd-rend2"
	"${GSLIncludeDirectory}"
	)

set(SPRend2Files
	"${SharedDir}/rd-rend2/iqm.h"
	"${SharedDir}/rd-rend2/glext.h"
	"${SharedDir}/rd-rend2/qgl.h"
	"${SharedDir}/rd-rend2/stb_image.h"
	"${SPDir}/rd-rend2/G2_API.cpp"
	"${SPDir}/rd-rend2/G2_bolts.cpp"
	"${SPDir}/rd-rend2/G2_bones.cpp"
	"${SharedDir}/rd-rend2/G2_gore_r2.cpp"
	"${SharedDir}/rd-rend2/G2_gore_r2.h"
	"${SPDir}/rd-rend2/G2_misc.cpp"
	"${SPDir}/rd-rend2/G2_surfaces.cpp"
	"${SharedDir}/rd-rend2/tr_allocator.cpp"
	"${SharedDir}/rd-rend2/tr_allocator.h"
	"${SharedDir}/rd-rend2/tr_animation.cpp"
	"${SharedDir}/rd-rend2/tr_backend.cpp"
	"${SharedDir}/rd-rend2/tr_bsp.cpp"
	"${SharedDir}/rd-rend2/tr_cache.cpp"
	"${SharedDir}/rd-rend2/tr_cache.h"
	"${SharedDir}/rd-rend2/tr_cmds.cpp"
	"${SharedDir}/rd-rend2/tr_curve.cpp"
	"${SharedDir}/rd-rend2/tr_decals.cpp"
	"${SPDir}/rd-rend2/tr_draw.cpp"
	"${SharedDir}/rd-rend2/tr_extensions.cpp"
	"${SharedDir}/rd-rend2/tr_extramath.cpp"
	"${SharedDir}/rd-rend2/tr_extramath.h"
	"${SharedDir}/rd-rend2/tr_extratypes.h"
	"${SharedDir}/rd-rend2/tr_fbo.cpp"
	"${SharedDir}/rd-rend2/tr_fbo.h"
	"${SharedDir}/rd-rend2/tr_flares.cpp"
	"${SharedDir}/rd-rend2/tr_ghoul2.cpp"
	"${SharedDir}/rd-rend2/tr_glsl.cpp"
	"${SharedDir}/rd-rend2/tr_glsl_parse.cpp"
	"${SharedDir}/rd-rend2/tr_image.cpp"
	"${SharedDir}/rd-rend2/tr_image_stb.cpp"
	"${SPDir}/rd-rend2/tr_init.cpp"
	"${SharedDir}/rd-rend2/tr_light.cpp"
	"${SharedDir}/rd-rend2/tr_local.h"
	"${SharedDir}/rd-rend2/tr_main.cpp"
	"${SharedDir}/rd-rend2/tr_marks.cpp"
	"${SharedDir}/rd-rend2/tr_mesh.cpp"
	"${SharedDir}/rd-rend2/tr_model.cpp"
	"${SharedDir}/rd-rend2/tr_model_iqm.cpp"
	"${SharedDir}/rd-rend2/tr_postprocess.cpp"
	"${SharedDir}/rd-rend2/tr_postprocess.h"
	"${SharedDir}/rd-rend2/tr_scene.cpp"
	"${SharedDir}/rd-rend2/tr_shade.cpp"
	"${SharedDir}/rd-rend2/tr_shade_calc.cpp"
	"${SharedDir}/rd-rend2/tr_shader.cpp"
	"${SharedDir}/rd-rend2/tr_shadows.cpp"
	"${SharedDir}/rd-rend2/tr_skin.cpp"
	"${SharedDir}/rd-rend2/tr_sky.cpp"
	"${SharedDir}/rd-rend2/tr_subs.cpp"
	"${SharedDir}/rd-rend2/tr_surface.cpp"
	"${SharedDir}/rd-rend2/tr_tangentspace.cpp"
	"${SharedDir}/rd-rend2/tr_vbo.cpp"
	"${SharedDir}/rd-rend2/tr_world.cpp"
	"${SharedDir}/rd-rend2/tr_weather.cpp"
	"${SharedDir}/rd-rend2/tr_weather.h")
source_group("renderer" FILES ${SPRend2Files})

file(GLOB SPRend2GLSLFiles "${SharedDir}/rd-rend2/glsl/*.glsl")
source_group("renderer\\glsl" FILES ${SPRend2GLSLFiles})
set(SPRend2Files ${SPRend2Files} ${SPRend2GLSLFiles})

set(SPRend2MikktFiles
	"${SharedDir}/rd-rend2/MikkTSpace/mikktspace.h"
	"${SharedDir}/rd-rend2/MikkTSpace/mikktspace.c")
source_group("MikktSpace" FILES ${SPRend2MikktFiles})
set(SPRend2Files ${SPRend2Files} ${SPRend2MikktFiles})

set(SPRend2Ghoul2Files
	"${SPDir}/ghoul2/G2.h"
	"${SPDir}/ghoul2/ghoul2_gore.h")
source_group("ghoul2" FILES ${SPRend2Ghoul2Files})
set(SPRend2Files ${SPRend2Files} ${SPRend2Ghoul2Files})

set(SPRend2RdCommonFiles
	"${SPDir}/rd-common/mdx_format.h"
	"${SPDir}/rd-common/tr_common.h"
	"${SPDir}/rd-common/tr_font.cpp"
	"${SPDir}/rd-common/tr_font.h"
	"${SPDir}/rd-common/tr_image_load.cpp"
	"${SPDir}/rd-common/tr_image_jpg.cpp"
	"${SPDir}/rd-common/tr_image_tga.cpp"
	"${SPDir}/rd-common/tr_image_png.cpp"
	"${SPDir}/rd-common/tr_noise.cpp"
	"${SPDir}/rd-common/tr_public.h"
	"${SPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${SPRend2RdCommonFiles})
set(SPRend2Files ${SPRend2Files} ${SPRend2RdCommonFiles})

set(SPRend2CommonFiles
	"${SPDir}/qcommon/matcomp.cpp"
	"${SPDir}/qcommon/q_shared.cpp"
	"${SharedCommonFiles}")
source_group("common" FILES ${SPRend2CommonFiles})
set(SPRend2Files ${SPRend2Files} ${SPRend2CommonFiles})

set(SPRend2CommonSafeFiles
	${SharedCommonSafeFiles}
	)
source_group("common/safe" FILES ${SPRend2CommonSafeFiles})
set(SPRend2Files ${SPRend2Files} ${SPRend2CommonSafeFiles})

# Transparently use either bundled or system libjpeg.
list(APPEND SPRend2IncludeDirectories ${JPEG_INCLUDE_DIR})
list(APPEND SPRend2Libraries          ${JPEG_LIBRARIES})

# Transparently use either bundled or system libpng.  Order is important --
# libpng used zlib, so it must come before it on the linker command line.
list(APPEND SPRend2IncludeDirectories ${PNG_INCLUDE_DIRS})
list(APPEND SPRend2Libraries          ${PNG_LIBRARIES})

# Transparently use either bundled or system zlib.
list(APPEND SPRend2IncludeDirectories ${ZLIB_INCLUDE_DIR})
list(APPEND SPRend2Libraries          ${ZLIB_LIBRARIES})

# Transparently use our bundled minizip.
list(APPEND SPRend2IncludeDirectories ${MINIZIP_INCLUDE_DIRS})
list(APPEND SPRend2Libraries          ${MINIZIP_LIBRARIES})

find_package(OpenGL REQUIRED)
set(SPRend2IncludeDirectories ${SPRend2IncludeDirectories} ${OPENGL_INCLUDE_DIR})
set(SPRend2Libraries ${SPRend2Libraries} ${OPENGL_LIBRARIES})

source_group("renderer"
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
	${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)
set(SPRend2Files
	${SPRend2Files}
	${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
	${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)

set(SPRend2IncludeDirectories ${SPRend2IncludeDirectories} ${OpenJKLibDir})

function(add_sp_renderer_project ProjectName Label EngineName InstallDir Component)
	add_library(${ProjectName} SHARED ${SPRend2Files})
	if(NOT MSVC)
		# remove "lib" prefix for .so/.dylib files
		set_target_properties(${ProjectName} PROPERTIES PREFIX "")
	endif()

	if(WIN32)
		install(TARGETS ${ProjectName}
			RUNTIME
			DESTINATION ${InstallDir}
			COMPONENT ${Component})
	else(WIN32)
		if(MakeApplicationBundles AND (BuildSPEngine OR BuildJK2SPEngine))
			install(TARGETS ${ProjectName}
				LIBRARY
				DESTINATION "${InstallDir}/${EngineName}.app/Contents/MacOS/"
				COMPONENT ${Component})
		else()
			install(TARGETS ${ProjectName}
				LIBRARY
				DESTINATION ${InstallDir}
				COMPONENT ${Component})
		endif()
	endif()

	set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS "${SPRend2Defines}")

	# Hide symbols not explicitly marked public.
	set_property(TARGET ${ProjectName} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

	set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPRend2IncludeDirectories}")
	set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})

	target_link_libraries(${ProjectName} ${SPRend2Libraries})
	target_include_directories(${ProjectName} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_sp_renderer_project)

# Defines
set(SPRend2Defines ${SharedDefines} "RENDERER" "REND2_SP" "REND2_SP_GORE")

if(BuildSPRend2)
	add_sp_renderer_project(${SPRend2} "SP Rend2 Renderer" ${SPEngine} ${JKAInstallDir} ${JKASPClientComponent})
endif(BuildSPRend2)
if(BuildJK2SPRdRend2)
	set(SPRend2Defines ${SPRend2Defines} "JK2_MODE")
	add_sp_renderer_project(${JK2SPRend2Renderer} "JK2 SP Rend2 Renderer" ${JK2SPEngine} ${JK2InstallDir} ${JK2SPClientComponent})
endif(BuildJK2SPRdRend2)

# GLSL shader file generator
add_executable(compact_glsl_sp
	${SharedDir}/rd-rend2/glsl/compact.cpp
	${SharedDir}/rd-rend2/tr_allocator.cpp
	${SharedDir}/rd-rend2/tr_allocator.h
	${SharedDir}/rd-rend2/tr_glsl_parse.cpp)
target_compile_definitions(compact_glsl_sp PRIVATE "GLSL_BUILDTOOL" "NOMINMAX" "REND2_SP")
if (NOT WIN32 AND NOT APPLE)
	target_compile_definitions(compact_glsl_sp PRIVATE "ARCH_STRING=\"${Architecture}\"")
endif()
target_include_directories(compact_glsl_sp PRIVATE "${SPRend2IncludeDirectories}")
if (WIN32 OR APPLE)
add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
	COMMAND
		compact_glsl_sp
			${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp
			${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
			${SPRend2GLSLFiles}
	DEPENDS
		compact_glsl_sp ${SPRend2GLSLFiles})
else()
add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
	COMMAND
		${CMAKE_BINARY_DIR}/compact_glsl_sp
			${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp
			${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
			${SPRend2GLSLFiles}
	DEPENDS
		compact_glsl_sp ${SPRend2GLSLFiles})
endif()
