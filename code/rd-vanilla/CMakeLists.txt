cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

#      Modular Renderer crap (let's hope this works..)

# doesn't appear to use any libraries so far as I know, just message non-Windows users about this and go from there
if(NOT WIN32)
  message("You might need to add libraries to code/rd-vanilla/CMakeLists.txt for this to link properly (SDL etc)")
endif(NOT WIN32)

#      Defines
set(SPVanillaRendererDefines ${SharedDefines} "RENDERER" "_JK2EXE")

# not sure what to do about OpenGL/OpenAL since I don't actually use Linux (big surprise eh) but I know it isn't included on windows
if(NOT WIN32)
  message("You might need to link OpenGL/OpenAL in code/rd-vanilla/CMakeLists.txt")
endif(NOT WIN32)



# Files

# JPEG
file(GLOB_RECURSE J_SRC "${SPDir}/jpeg-6/*.cpp" "${SPDir}/jpeg-6/*.h")
	source_group("jpeg-6" FILES ${J_SRC})
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

# PNG
set(SPRDVanillaPNGFiles
		"${SPDir}/png/png.cpp"
		"${SPDir}/png/png.h"
		)
	source_group("png" FILES ${SPRDVanillaPNGFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaPNGFiles})

# Misc files
set(SPRDVanillaMiscFiles
		"${SPDir}/qcommon/cm_landscape.h"
		"${SPDir}/game/genericparser2.h"
		"${SPDir}/game/genericparser2.cpp"
		"${SPDir}/renderer/mdx_format.h"
		"${SPDir}/game/q_math.cpp"
		"${SPDir}/game/q_shared.cpp"
		"${SPDir}/renderer/tr_public.h"
		"${SPDir}/renderer/tr_types.h"
		)
	source_group("misc" FILES ${SPRDVanillaMiscFiles})
	set(SPRDVanillaFiles ${SPRDVanillaFiles} ${SPRDVanillaMiscFiles})

# zlib files
set(SPRDVanillaZlibFiles
	"${SPDir}/zlib32/deflate.cpp"
	"${SPDir}/zlib32/inflate.cpp"
	"${SPDir}/zlib32/zipcommon.cpp"
	"${SPDir}/zlib32/deflate.h"
	"${SPDir}/zlib32/inflate.h"
	"${SPDir}/zlib32/zip.h"
	)
source_group("rd-zlib" FILES ${SPRDVanillaZlibFiles})
set(SPEngineFiles ${SPRDVanillaFiles} ${SPRDVanillaZlibFiles})

# main files
set(SPRDVanillaMainFiles
	"${SPDir}/rd-vanilla/amd3d.h"
	"${SPDir}/rd-vanilla/glext.h"
	"${SPDir}/rd-vanilla/glext_console.h"
	"${SPDir}/rd-vanilla/matcomp.c"
	"${SPDir}/rd-vanilla/matcomp.h"
	"${SPDir}/rd-vanilla/qgl.h"
	"${SPDir}/rd-vanilla/qgl_console.h"
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
	"${SPDir}/rd-vanilla/tr_main.cpp"
	"${SPDir}/rd-vanilla/tr_marks.cpp"
	"${SPDir}/rd-vanilla/tr_mesh.cpp"
	"${SPDir}/rd-vanilla/tr_model.cpp"
	"${SPDir}/rd-vanilla/tr_noise.cpp"
	"${SPDir}/rd-vanilla/tr_quicksprite.cpp"
	"${SPDir}/rd-vanilla/tr_quicksprite.h"
	"${SPDir}/rd-vanilla/tr_scene.cpp"
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
	"${SPDir}/rd-vanilla/win_gamma.cpp"
	"${SPDir}/rd-vanilla/win_glimp.cpp"
	"${SPDir}/rd-vanilla/win_qgl.cpp"
	)
	source_group("SortMe" FILES ${SPEngineSortMe})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineSortMe})

if(WIN32)
	add_executable(${SPRDVanilla} WIN32 ${SPRDVanillaFiles})
else(WIN32)
	add_executable(${SPRDVanilla} ${SPRDVanillaFiles})
endif(WIN32)
install(TARGETS ${SPRDVanilla} RUNTIME DESTINATION ".")
set_target_properties(${SPRDVanilla} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanilla} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanilla} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SPRDVanillaDefines};${ReleaseDefines}")
set_target_properties(${SPRDVanilla} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SPRDVanillaDefines};${DebugDefines}")
