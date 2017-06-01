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

# Check for dependencies first

set(MPVanillaRendererIncludeDirectories ${MPDir})
set(MPVanillaRendererIncludeDirectories
	${MPVanillaRendererIncludeDirectories}
	${SharedDir}
	${MPDir}
	"${MPDir}/rd-vanilla"
	"${GSLIncludeDirectory}"
	)

set(MPVanillaRendererFiles
	"${MPDir}/rd-vanilla/glext.h"
	"${MPDir}/rd-vanilla/qgl.h"
	"${MPDir}/rd-vanilla/G2_API.cpp"
	"${MPDir}/rd-vanilla/G2_bolts.cpp"
	"${MPDir}/rd-vanilla/G2_bones.cpp"
	"${MPDir}/rd-vanilla/G2_misc.cpp"
	"${MPDir}/rd-vanilla/G2_surfaces.cpp"
	"${MPDir}/rd-vanilla/tr_arb.cpp"
	"${MPDir}/rd-vanilla/tr_backend.cpp"
	"${MPDir}/rd-vanilla/tr_bsp.cpp"
	"${MPDir}/rd-vanilla/tr_cmds.cpp"
	"${MPDir}/rd-vanilla/tr_curve.cpp"
	"${MPDir}/rd-vanilla/tr_decals.cpp"
	"${MPDir}/rd-vanilla/tr_ghoul2.cpp"
	"${MPDir}/rd-vanilla/tr_image.cpp"
	"${MPDir}/rd-vanilla/tr_init.cpp"
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
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h"
	"${MPDir}/rd-common/tr_types.h")
source_group("rd-common" FILES ${MPVanillaRendererRdCommonFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererRdCommonFiles})

set(MPVanillaRendererCommonFiles
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	
	${SharedCommonFiles})
source_group("common" FILES ${MPVanillaRendererCommonFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererCommonFiles})

set(MPVanillaRendererCommonSafeFiles
	${SharedCommonSafeFiles}
	)
source_group("common/safe" FILES ${MPVanillaRendererCommonSafeFiles})
set(MPVanillaRendererFiles ${MPVanillaRendererFiles} ${MPVanillaRendererCommonSafeFiles})

# Transparently use either bundled or system libjpeg.
list(APPEND MPVanillaRendererIncludeDirectories ${JPEG_INCLUDE_DIR})
list(APPEND MPVanillaRendererLibraries          ${JPEG_LIBRARIES})

# Transparently use either bundled or system libpng.  Order is important --
# libpng used zlib, so it must come before it on the linker command line.
list(APPEND MPVanillaRendererIncludeDirectories ${PNG_INCLUDE_DIRS})
list(APPEND MPVanillaRendererLibraries          ${PNG_LIBRARIES})

# Transparently use either bundled or system zlib.
list(APPEND MPVanillaRendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
list(APPEND MPVanillaRendererLibraries          ${ZLIB_LIBRARIES})

# Transparently use our bundled minizip.
list(APPEND MPVanillaRendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
list(APPEND MPVanillaRendererLibraries          ${MINIZIP_LIBRARIES})

find_package(OpenGL REQUIRED)
set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
set(MPVanillaRendererLibraries ${MPVanillaRendererLibraries} ${OPENGL_LIBRARIES})

set(MPVanillaRendererIncludeDirectories ${MPVanillaRendererIncludeDirectories} ${OpenJKLibDir})
add_library(${MPVanillaRenderer} SHARED ${MPVanillaRendererFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPVanillaRenderer} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${MPVanillaRenderer}
		RUNTIME
		DESTINATION ${JKAInstallDir}
		COMPONENT ${JKAMPClientComponent})
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPVanillaRenderer}
			LIBRARY
			DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/"
			COMPONENT ${JKAMPClientComponent})
	else()
		install(TARGETS ${MPVanillaRenderer}
			LIBRARY
			DESTINATION ${JKAInstallDir}
			COMPONENT ${JKAMPClientComponent})
	endif()
endif()

set_target_properties(${MPVanillaRenderer} PROPERTIES COMPILE_DEFINITIONS "${SharedDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${MPVanillaRenderer} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${MPVanillaRenderer} PROPERTIES INCLUDE_DIRECTORIES "${MPVanillaRendererIncludeDirectories}")
set_target_properties(${MPVanillaRenderer} PROPERTIES PROJECT_LABEL "MP Vanilla Renderer")
target_link_libraries(${MPVanillaRenderer} ${MPVanillaRendererLibraries})
