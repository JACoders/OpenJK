cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

if(BuildSPRdVanilla OR BuildJK2SPRdVanilla)

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
		"${SPDir}/rd-vanilla/G2_API.cpp"
		"${SPDir}/rd-vanilla/G2_bolts.cpp"
		"${SPDir}/rd-vanilla/G2_bones.cpp"
		"${SPDir}/rd-vanilla/G2_misc.cpp"
		"${SPDir}/rd-vanilla/G2_surfaces.cpp"
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
	else(UseInternalPNG)
		find_package(PNG REQUIRED)
		set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${PNG_INCLUDE_DIR})
		set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${PNG_LIBRARIES})
	endif(UseInternalPNG)

	if(UseInternalZlib)
		set(SPRDVanillaRendererZlibFiles
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
		source_group("zlib" FILES ${SPRDVanillaRendererZlibFiles})
		set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaRendererZlibFiles})
		
		set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} "${OpenJKLibDir}/zlib")
	else(UseInternalZlib)
		find_package(ZLIB REQUIRED)
		set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${ZLIB_INCLUDE_DIR})
		set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${ZLIB_LIBRARY})
	endif(UseInternalZlib)

	# Misc files
	set(SPRDVanillaCommonFiles
		"${SPDir}/qcommon/matcomp.cpp"
		"${SPDir}/qcommon/q_math.cpp"
		"${SPDir}/qcommon/q_shared.cpp"
		"${SPDir}/qcommon/q_shared.h"
		"${SPDir}/qcommon/q_platform.h"
		)
	source_group("common" FILES ${SPRDVanillaCommonFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaCommonFiles})

	# main files
	set(SPRDVanillaMainFiles
		"${SPDir}/rd-vanilla/glext.h"
		"${SPDir}/rd-vanilla/qgl.h"
		"${SPDir}/rd-vanilla/tr_backend.cpp"
		"${SPDir}/rd-vanilla/tr_bsp.cpp"
		"${SPDir}/rd-vanilla/tr_cmds.cpp"
		"${SPDir}/rd-vanilla/tr_curve.cpp"
		"${SPDir}/rd-vanilla/tr_draw.cpp"
		"${SPDir}/rd-vanilla/tr_flares.cpp"
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

	if(WIN32)
		set(SPRDVanillaWin32Files
			"${SPDir}/win32/win_gamma.cpp"
			"${SPDir}/win32/win_glimp.cpp"
			"${SPDir}/win32/win_qgl.cpp"
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

		set(SDL2_BUILDING_LIBRARY ON) # don't want main() in Renderer
		find_package(SDL2 REQUIRED)
		set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${SDL2_INCLUDE_DIR})
		set(SPRDVanillaRendererLibraries ${SPRDVanillaRendererLibraries} ${SDL2_LIBRARY})
	endif(NOT WIN32)

	set(SPRDVanillaRendererIncludeDirectories ${SPRDVanillaRendererIncludeDirectories} ${OpenJKLibDir})
	
	function(add_sp_renderer_project ProjectName Label EngineName)
		add_library(${ProjectName} SHARED ${SPRDVanillaFiles})
		if(NOT WIN32)
			# remove "lib" prefix for .so/.dylib files
			set_target_properties(${ProjectName} PROPERTIES PREFIX "")
		endif(NOT WIN32)

		if(WIN32)
			install(TARGETS ${ProjectName} RUNTIME DESTINATION ".")
		else(WIN32)
			if(MakeApplicationBundles AND (BuildSPEngine OR BuildJK2SPEngine))
				install(TARGETS ${ProjectName} LIBRARY DESTINATION "${EngineName}.app/Contents/MacOS/")
			else(MakeApplicationBundles AND (BuildSPEngine OR BuildJK2SPEngine))
				install(TARGETS ${ProjectName} LIBRARY DESTINATION ".")
			endif(MakeApplicationBundles AND (BuildSPEngine OR BuildJK2SPEngine))
		endif(WIN32)

		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SPRDVanillaDefines};${ReleaseDefines}")
		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SPRDVanillaDefines};${ReleaseDefines}")
		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SPRDVanillaDefines};${ReleaseDefines}")
		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SPRDVanillaDefines};${DebugDefines}")
		set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPRDVanillaRendererIncludeDirectories}")
		set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})

		target_link_libraries(${ProjectName} ${SPRDVanillaRendererLibraries})
	endfunction(add_sp_renderer_project)

	if(BuildSPRdVanilla)
		add_sp_renderer_project(${SPRDVanillaRenderer} "SP Vanilla Renderer" ${SPEngine})
	endif(BuildSPRdVanilla)
	if(BuildJK2SPRdVanilla)
		set(SPRDVanillaDefines ${SPRDVanillaDefines} "JK2_MODE")
		add_sp_renderer_project(${JK2SPVanillaRenderer} "JK2 SP Vanilla Renderer" ${JK2SPEngine})
	endif(BuildJK2SPRdVanilla)
	
endif(BuildSPRdVanilla OR BuildJK2SPRdVanilla)
