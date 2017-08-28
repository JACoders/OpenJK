#============================================================================
# Copyright (C) 2013 - 2015, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

if(BuildSPRdVanilla OR BuildJK2SPRdVanilla)

	set(SPRDVanillaRendererIncludeDirectories ${SPDir})
	set(SPRDVanillaRendererIncludeDirectories
		${SharedDir}
		${SPRDVanillaRendererIncludeDirectories}
		"${SPDir}/rd-vanilla"
		"${GSLIncludeDirectory}"
		)

	# Modular Renderer crap (let's hope this works..)

	# Defines
	set(SPRDVanillaDefines ${SharedDefines} "RENDERER" "_JK2EXE")

	# Files

	# GHOUL 2
	set(SPRDVanillaG2Files
		"${SPDir}/ghoul2/G2.h"
		"${SPDir}/ghoul2/ghoul2_gore.h"
		"${SPDir}/rd-vanilla/G2_API.cpp"
		"${SPDir}/rd-vanilla/G2_bolts.cpp"
		"${SPDir}/rd-vanilla/G2_bones.cpp"
		"${SPDir}/rd-vanilla/G2_misc.cpp"
		"${SPDir}/rd-vanilla/G2_surfaces.cpp"
		)
	source_group("ghoul2" FILES ${SPRDVanillaG2Files})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaG2Files})

    # Transparently use either bundled or system jpeg.
    list(APPEND SPRDVanillaRendererIncludeDirectories ${JPEG_INCLUDE_DIR})
    list(APPEND SPRDVanillaRendererLibraries          ${JPEG_LIBRARIES})

    # Transparently use either bundled or system libpng.  Order is important --
    # libpng used zlib, so it must come before it on the linker command line.
    list(APPEND SPRDVanillaRendererIncludeDirectories ${PNG_INCLUDE_DIRS})
    list(APPEND SPRDVanillaRendererLibraries          ${PNG_LIBRARIES})

    # Transparently use either bundled or system zlib.
    list(APPEND SPRDVanillaRendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
    list(APPEND SPRDVanillaRendererLibraries          ${ZLIB_LIBRARIES})

    # Transparently use our bundled minizip.
    list(APPEND SPRDVanillaRendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
    list(APPEND SPRDVanillaRendererLibraries          ${MINIZIP_LIBRARIES})

    # Transparantly make use of all renderer directories
    list(APPEND SPRDVanillaRendererIncludeDirectories "${SPDir}/rd-common")
    list(APPEND SPRDVanillaRendererIncludeDirectories "${SPDir}/rd-vanilla")

	# Misc files
	set(SPRDVanillaCommonFiles
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
	source_group("common" FILES ${SPRDVanillaCommonFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaCommonFiles})

	# Misc files
	set(SPRDVanillaCommonSafeFiles
		${SharedCommonSafeFiles}
		)
	source_group("common/safe" FILES ${SPRDVanillaCommonSafeFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaCommonSafeFiles})

	# main files
	set(SPRDVanillaMainFiles
		"${SPDir}/rd-vanilla/glext.h"
		"${SPDir}/rd-vanilla/qgl.h"
		"${SPDir}/rd-vanilla/tr_arb.cpp"
		"${SPDir}/rd-vanilla/tr_backend.cpp"
		"${SPDir}/rd-vanilla/tr_bsp.cpp"
		"${SPDir}/rd-vanilla/tr_cmds.cpp"
		"${SPDir}/rd-vanilla/tr_curve.cpp"
		"${SPDir}/rd-vanilla/tr_draw.cpp"
		"${SPDir}/rd-vanilla/tr_ghoul2.cpp"
		"${SPDir}/rd-vanilla/tr_image.cpp"
		"${SPDir}/rd-vanilla/tr_init.cpp"
		"${SPDir}/rd-vanilla/tr_light.cpp"
		"${SPDir}/rd-vanilla/tr_local.h"
		"${SPDir}/rd-vanilla/tr_main.cpp"
		"${SPDir}/rd-vanilla/tr_marks.cpp"
		"${SPDir}/rd-vanilla/tr_mesh.cpp"
		"${SPDir}/rd-vanilla/tr_model.cpp"
		"${SPDir}/rd-vanilla/tr_quicksprite.cpp"
		"${SPDir}/rd-vanilla/tr_quicksprite.h"
		"${SPDir}/rd-vanilla/tr_scene.cpp"
		"${SPDir}/rd-vanilla/tr_shade.cpp"
		"${SPDir}/rd-vanilla/tr_shade_calc.cpp"
		"${SPDir}/rd-vanilla/tr_shader.cpp"
		"${SPDir}/rd-vanilla/tr_shadows.cpp"
		"${SPDir}/rd-vanilla/tr_skin.cpp"
		"${SPDir}/rd-vanilla/tr_sky.cpp"
		"${SPDir}/rd-vanilla/tr_stl.cpp"
		"${SPDir}/rd-vanilla/tr_stl.h"
		"${SPDir}/rd-vanilla/tr_subs.cpp"
		"${SPDir}/rd-vanilla/tr_surface.cpp"
		"${SPDir}/rd-vanilla/tr_surfacesprites.cpp"
		"${SPDir}/rd-vanilla/tr_world.cpp"
		"${SPDir}/rd-vanilla/tr_WorldEffects.cpp"
		"${SPDir}/rd-vanilla/tr_WorldEffects.h"
		)
	source_group("renderer" FILES ${SPRDVanillaMainFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaMainFiles})

	set(SPRDVanillaRdCommonFiles
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
	source_group("rd-common" FILES ${SPRDVanillaRdCommonFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaRdCommonFiles})

	find_package(OpenGL REQUIRED)
	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${OPENGL_LIBRARIES})

	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${OpenJKLibDir})

	function(add_sp_renderer_project ProjectName Label EngineName InstallDir Component)
		add_library(${ProjectName} SHARED ${SPRDVanillaFiles})
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

		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS "${SPRDVanillaDefines}")

		# Hide symbols not explicitly marked public.
		set_property(TARGET ${ProjectName} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

		set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPRDVanillaRendererIncludeDirectories}")
		set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})

		target_link_libraries(${ProjectName} ${SPRDVanillaRendererLibraries})
	endfunction(add_sp_renderer_project)

	if(BuildSPRdVanilla)
		add_sp_renderer_project(${SPRDVanillaRenderer} "SP Vanilla Renderer" ${SPEngine} ${JKAInstallDir} ${JKASPClientComponent})
	endif(BuildSPRdVanilla)
	if(BuildJK2SPRdVanilla)
		set(SPRDVanillaDefines ${SPRDVanillaDefines} "JK2_MODE")
		add_sp_renderer_project(${JK2SPVanillaRenderer} "JK2 SP Vanilla Renderer" ${JK2SPEngine} ${JK2InstallDir} ${JK2SPClientComponent})
	endif(BuildJK2SPRdVanilla)

endif(BuildSPRdVanilla OR BuildJK2SPRdVanilla)
