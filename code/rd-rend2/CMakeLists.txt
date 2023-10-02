# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

if(BuildSPRend2)

	set(SPRend2RendererIncludeDirectories ${SPDir})
	set(SPRend2RendererIncludeDirectories
		${SharedDir}
		${SPRend2RendererIncludeDirectories}
		"${SPDir}/rd-rend2"
		"${GSLIncludeDirectory}"
		)

	# Modular Renderer crap (let's hope this works..)

	# Defines
	set(SPRend2Defines ${SharedDefines} "RENDERER" "_JK2EXE")

	# Files
	
	set(SPRend2MikktFiles
		"${SPDir}/rd-rend2/MikkTSpace/mikktspace.h"
		"${SPDir}/rd-rend2/MikkTSpace/mikktspace.c")
	source_group("MikktSpace" FILES ${SPRend2MikktFiles})
	set(SPRend2Files ${SPRend2Files} ${SPRend2MikktFiles})

	# GHOUL 2
	set(SPRend2G2Files
		"${SPDir}/ghoul2/G2.h"
		"${SPDir}/ghoul2/ghoul2_gore.h"
		"${SPDir}/rd-rend2/G2_API.cpp"
		"${SPDir}/rd-rend2/G2_bolts.cpp"
		"${SPDir}/rd-rend2/G2_bones.cpp"
		"${SPDir}/rd-rend2/G2_gore_r2.cpp"
		"${SPDir}/rd-rend2/G2_gore_r2.h"
		"${SPDir}/rd-rend2/G2_misc.cpp"
		"${SPDir}/rd-rend2/G2_surfaces.cpp"	
		)
	source_group("ghoul2" FILES ${SPRend2G2Files})
	set(SPRend2Files ${SPRend2Files} ${SPRend2G2Files})

    # Transparently use either bundled or system jpeg.
    list(APPEND SPRend2RendererIncludeDirectories ${JPEG_INCLUDE_DIR})
    list(APPEND SPRend2RendererLibraries          ${JPEG_LIBRARIES})

    # Transparently use either bundled or system libpng.  Order is important --
    # libpng used zlib, so it must come before it on the linker command line.
    list(APPEND SPRend2RendererIncludeDirectories ${PNG_INCLUDE_DIRS})
    list(APPEND SPRend2RendererLibraries          ${PNG_LIBRARIES})

    # Transparently use either bundled or system zlib.
    list(APPEND SPRend2RendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
    list(APPEND SPRend2RendererLibraries          ${ZLIB_LIBRARIES})

    # Transparently use our bundled minizip.
    list(APPEND SPRend2RendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
    list(APPEND SPRend2RendererLibraries          ${MINIZIP_LIBRARIES})

    # Transparantly make use of all renderer directories
    list(APPEND SPRend2RendererIncludeDirectories "${SPDir}/rd-common")
    list(APPEND SPRend2RendererIncludeDirectories "${SPDir}/rd-rend2")

	# Misc files
	set(SPRend2CommonFiles
		"${SPDir}/qcommon/matcomp.cpp"
		"${SPDir}/qcommon/q_shared.cpp"
		"${SPDir}/qcommon/q_shared.h"
		"${SPDir}/qcommon/ojk_i_saved_game.h"
		"${SPDir}/qcommon/ojk_saved_game_class_archivers.h"
		"${SPDir}/qcommon/ojk_saved_game_helper.h"
		"${SPDir}/qcommon/ojk_saved_game_helper_fwd.h"
		"${SPDir}/qcommon/ojk_scope_guard.h"
		${SharedCommonFiles}
		)
	source_group("common" FILES ${SPRend2CommonFiles})
	set(SPRend2Files ${SPRend2Files} ${SPRend2CommonFiles})

	# Misc files
	set(SPRend2CommonSafeFiles
		${SharedCommonSafeFiles}
		)
	source_group("common/safe" FILES ${SPRend2CommonSafeFiles})
	set(SPRend2Files ${SPRend2Files} ${SPRend2CommonSafeFiles})

	# main files
	set(SPRend2MainFiles
		"${SPDir}/rd-rend2/iqm.h"
		"${SPDir}/rd-rend2/glext.h"
		"${SPDir}/rd-rend2/qgl.h"
		"${SPDir}/rd-rend2/stb_image.h"
		"${SPDir}/rd-rend2/tr_allocator.cpp"
		"${SPDir}/rd-rend2/tr_allocator.h"
		"${SPDir}/rd-rend2/tr_animation.cpp"
		"${SPDir}/rd-rend2/tr_backend.cpp"
		"${SPDir}/rd-rend2/tr_bsp.cpp"
		"${SPDir}/rd-rend2/tr_cache.cpp"
		"${SPDir}/rd-rend2/tr_cache.h"
		"${SPDir}/rd-rend2/tr_cmds.cpp"
		"${SPDir}/rd-rend2/tr_curve.cpp"
		"${SPDir}/rd-rend2/tr_decals.cpp"
		"${SPDir}/rd-rend2/tr_extensions.cpp"
		"${SPDir}/rd-rend2/tr_extramath.cpp"
		"${SPDir}/rd-rend2/tr_extramath.h"
		"${SPDir}/rd-rend2/tr_extratypes.h"
		"${SPDir}/rd-rend2/tr_fbo.cpp"
		"${SPDir}/rd-rend2/tr_fbo.h"
		"${SPDir}/rd-rend2/tr_flares.cpp"
		"${SPDir}/rd-rend2/tr_ghoul2.cpp"
		"${SPDir}/rd-rend2/tr_glsl.cpp"
		"${SPDir}/rd-rend2/tr_glsl_parse.cpp"
		"${SPDir}/rd-rend2/tr_image.cpp"
		"${SPDir}/rd-rend2/tr_image_stb.cpp"
		"${SPDir}/rd-rend2/tr_init.cpp"
		"${SPDir}/rd-rend2/tr_light.cpp"
		"${SPDir}/rd-rend2/tr_local.h"
		"${SPDir}/rd-rend2/tr_main.cpp"
		"${SPDir}/rd-rend2/tr_marks.cpp"
		"${SPDir}/rd-rend2/tr_mesh.cpp"
		"${SPDir}/rd-rend2/tr_model.cpp"
		"${SPDir}/rd-rend2/tr_model_iqm.cpp"
		"${SPDir}/rd-rend2/tr_postprocess.cpp"
		"${SPDir}/rd-rend2/tr_postprocess.h"
		"${SPDir}/rd-rend2/tr_scene.cpp"
		"${SPDir}/rd-rend2/tr_shade.cpp"
		"${SPDir}/rd-rend2/tr_shade_calc.cpp"
		"${SPDir}/rd-rend2/tr_shader.cpp"
		"${SPDir}/rd-rend2/tr_shadows.cpp"
		"${SPDir}/rd-rend2/tr_skin.cpp"
		"${SPDir}/rd-rend2/tr_sky.cpp"
		"${SPDir}/rd-rend2/tr_subs.cpp"
		"${SPDir}/rd-rend2/tr_surface.cpp"
		"${SPDir}/rd-rend2/tr_tangentspace.cpp"
		"${SPDir}/rd-rend2/tr_vbo.cpp"
		"${SPDir}/rd-rend2/tr_world.cpp"
		"${SPDir}/rd-rend2/tr_weather.cpp"
		"${SPDir}/rd-rend2/tr_weather.h"
		)
	source_group("renderer" FILES ${SPRend2MainFiles})
	set(SPRend2Files ${SPRend2Files} ${SPRend2MainFiles})
	
	file(GLOB SPRend2GLSLFiles "${SPDir}/rd-rend2/glsl/*.glsl")
	source_group("renderer\\glsl" FILES ${SPRend2GLSLFiles})
	set(SPRend2Files ${SPRend2Files} ${SPRend2GLSLFiles})

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

	find_package(OpenGL REQUIRED)
	set(SPRend2RendererIncludeDirectories ${SPRend2RendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(SPRend2RendererLibraries ${SPRend2RendererLibraries} ${OPENGL_LIBRARIES})
	
	source_group("renderer"
		FILES
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)
	set(SPRend2Files
		${SPRend2Files}
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.h
		${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)

	set(SPRend2RendererIncludeDirectories ${SPRend2RendererIncludeDirectories} ${OpenJKLibDir})

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

		set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPRend2RendererIncludeDirectories}")
		set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})

		target_link_libraries(${ProjectName} ${SPRend2RendererLibraries})
		target_include_directories(${ProjectName} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	endfunction(add_sp_renderer_project)

	if(BuildSPRend2)
		add_sp_renderer_project(${SPRend2} "SP Rend2 Renderer" ${SPEngine} ${JKAInstallDir} ${JKASPClientComponent})
	endif(BuildSPRend2)
	
	# GLSL shader file generator
	add_executable(compact_glsl_sp
		${SPDir}/rd-rend2/glsl/compact.cpp
		${SPDir}/rd-rend2/tr_allocator.cpp
		${SPDir}/rd-rend2/tr_allocator.h
		${SPDir}/rd-rend2/tr_glsl_parse.cpp)
	target_compile_definitions(compact_glsl_sp PRIVATE "GLSL_BUILDTOOL" "NOMINMAX")
	if (NOT WIN32 AND NOT APPLE)
		target_compile_definitions(compact_glsl_sp PRIVATE "ARCH_STRING=\"${Architecture}\"")
	endif()
	target_include_directories(compact_glsl_sp PRIVATE "${SPRend2RendererIncludeDirectories}")
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

endif(BuildSPRend2)
