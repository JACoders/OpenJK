cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

#    Add Game Project
if(BuildSPGame)
	add_subdirectory("${SPDir}/game")
endif(BuildSPGame)

#    Add Modular Renderer project
if(BuildSPEngine)
	add_subdirectory("${SPDir}/rd-vanilla")
endif(BuildSPEngine)

#    Add Engine Project
if(BuildSPEngine)
	# Libraries
	if(WIN32)
		set(SPEngineLibraries "winmm")
	endif(WIN32)
	# Defines
	set(SPEngineDefines ${SharedDefines} "_JK2EXE")

	set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${OpenJKLibDir})
	
	#    Dependencies
	
	# OpenGL
	# (mrw) TODO Is this actually required on windows? It is loaded at runtime, should not be linked?
	find_package(OpenGL REQUIRED)
	set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(SPEngineLibraries ${SPEngineLibraries} ${OPENGL_LIBRARIES})

	# OpenAL (is optionally included for Windows)
	if(UseInternalOpenAL)
		# There's a libs/OpenAL32.lib
		# (mrw) TODO Is this actually required on windows? It is loaded at runtime, should not be linked?
		find_library(OpenALLibrary NAMES OpenAL32 PATHS "${OpenJKLibDir}")
		if(NOT OpenALLibrary)
			message(FATAL_ERROR "UseInternalOpenAL enabled, but lib/OpenAL32.lib not found!")
		endif(NOT OpenALLibrary)
		set(SPEngineLibraries ${SPEngineLibraries} ${OpenALLibrary})
	else(UseInternalOpenAL)
		find_package(OpenAL REQUIRED)
		set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${OPENAL_INCLUDE_DIR})
		set(SPEngineLibraries ${SPEngineLibraries} ${OPENAL_LIBRARY})
	endif(UseInternalOpenAL)
	
	# EAX is Windows-Only (right?)
	if(WIN32)
		set(SPEngineEAXFiles
			"${SPDir}/client/eax/eax.h"
			"${SPDir}/client/eax/EaxMan.h"
			)
		source_group("eax" FILES ${SPEngineEAXFiles})
		set(SPEngineFiles ${SPEngineFiles} ${SPEngineEAXFiles})
	endif(WIN32)

	if(NOT WIN32)
		find_package(SDL2 REQUIRED)
		set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${SDL2_INCLUDE_DIR})
		set(SPEngineLibraries ${SPEngineLibraries} ${SDL2_LIBRARY})
	endif(NOT WIN32)

	
	#    Source Files

	# Client files
	set(SPEngineClientFiles
		"${SPDir}/client/OpenAL/al.h"
		"${SPDir}/client/OpenAL/alc.h"
		"${SPDir}/client/OpenAL/alctypes.h"
		"${SPDir}/client/OpenAL/altypes.h"
		"${SPDir}/client/OpenAL/alu.h"
		"${SPDir}/client/OpenAL/alut.h"

		"${SPDir}/client/keys.h"
		"${SPDir}/client/keycodes.h"

		"${SPDir}/client/cl_cgame.cpp"
		"${SPDir}/client/cl_cin.cpp"
		"${SPDir}/client/cl_console.cpp"
		"${SPDir}/client/cl_input.cpp"
		"${SPDir}/client/cl_keys.cpp"
		"${SPDir}/client/cl_main.cpp"
		"${SPDir}/client/cl_mp3.cpp"
		"${SPDir}/client/cl_parse.cpp"
		"${SPDir}/client/cl_scrn.cpp"
		"${SPDir}/client/cl_ui.cpp"
		"${SPDir}/client/cl_mp3.h"
		"${SPDir}/client/client.h"
		"${SPDir}/client/client_ui.h"

		"${SPDir}/client/snd_ambient.cpp"
		"${SPDir}/client/snd_dma.cpp"
		"${SPDir}/client/snd_mem.cpp"
		"${SPDir}/client/snd_mix.cpp"
		"${SPDir}/client/snd_music.cpp"
		"${SPDir}/client/snd_ambient.h"
		"${SPDir}/client/snd_local.h"
		"${SPDir}/client/snd_music.h"
		"${SPDir}/client/snd_public.h"

		"${SPDir}/client/vmachine.cpp"
		"${SPDir}/client/vmachine.h"
		)
	source_group("client" FILES ${SPEngineClientFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineClientFiles})


	# qcommon files
	set(SPEngineCommonFiles
		"${SPDir}/qcommon/cm_draw.cpp"
		"${SPDir}/qcommon/cm_load.cpp"
		"${SPDir}/qcommon/cm_patch.cpp"
		"${SPDir}/qcommon/cm_polylib.cpp"
		"${SPDir}/qcommon/cm_randomterrain.cpp"
		"${SPDir}/qcommon/cm_shader.cpp"
		"${SPDir}/qcommon/cm_terrain.cpp"
		"${SPDir}/qcommon/cm_terrainmap.cpp"
		"${SPDir}/qcommon/cm_test.cpp"
		"${SPDir}/qcommon/cm_trace.cpp"
		"${SPDir}/qcommon/cm_draw.h"
		"${SPDir}/qcommon/cm_landscape.h"
		"${SPDir}/qcommon/cm_local.h"
		"${SPDir}/qcommon/cm_patch.h"
		"${SPDir}/qcommon/cm_polylib.h"
		"${SPDir}/qcommon/cm_public.h"
		"${SPDir}/qcommon/cm_randomterrain.h"
		"${SPDir}/qcommon/cm_terrainmap.h"

		"${SPDir}/qcommon/cmd.cpp"
		"${SPDir}/qcommon/common.cpp"
		"${SPDir}/qcommon/cvar.cpp"

		"${SPDir}/qcommon/files_common.cpp"
		"${SPDir}/qcommon/files_pc.cpp"

		"${SPDir}/qcommon/md4.cpp"
		"${SPDir}/qcommon/msg.cpp"
		"${SPDir}/qcommon/net_chan.cpp"

		"${SPDir}/qcommon/q_math.cpp"
		"${SPDir}/qcommon/q_shared.cpp"
		"${SPDir}/qcommon/q_shared.cpp"

		"${SPDir}/qcommon/sstring.h"
		"${SPDir}/qcommon/stringed_ingame.cpp"
		"${SPDir}/qcommon/stringed_interface.cpp"
		"${SPDir}/qcommon/stringed_ingame.h"
		"${SPDir}/qcommon/stringed_interface.h"

		"${SPDir}/qcommon/strip.cpp"
		"${SPDir}/qcommon/unzip.cpp"
		"${SPDir}/qcommon/unzip.h"
		"${OpenJKLibDir}/zlib/ioapi.c"
		"${OpenJKLibDir}/zlib/ioapi.h"
		"${SPDir}/qcommon/tags.h"
		"${SPDir}/qcommon/z_memman_pc.cpp"
		"${SPDir}/qcommon/chash.h"
		"${SPDir}/qcommon/qfiles.h"
		"${SPDir}/qcommon/files.h"
		"${SPDir}/qcommon/qcommon.h"
		"${SPDir}/qcommon/stv_version.h"
		"${SPDir}/qcommon/timing.h"
		"${SPDir}/qcommon/MiniHeap.h"
		)
	source_group("common" FILES ${SPEngineCommonFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineCommonFiles})


	# Server files
	set(SPEngineServerFiles
		"${SPDir}/server/exe_headers.cpp"
		"${SPDir}/server/sv_ccmds.cpp"
		"${SPDir}/server/sv_client.cpp"
		"${SPDir}/server/sv_game.cpp"
		"${SPDir}/server/sv_init.cpp"
		"${SPDir}/server/sv_main.cpp"
		"${SPDir}/server/sv_savegame.cpp"
		"${SPDir}/server/sv_snapshot.cpp"
		"${SPDir}/server/sv_world.cpp"
		"${SPDir}/server/exe_headers.h"
		"${SPDir}/server/server.h"
		)
	source_group("server" FILES ${SPEngineServerFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineServerFiles})


	# Game files
	set(SPEngineGameFiles
		"${SPDir}/game/genericparser2.cpp"
		"${SPDir}/game/channels.h"
		"${SPDir}/game/anims.h"
		"${SPDir}/game/b_public.h"
		"${SPDir}/game/bg_public.h"
		"${SPDir}/game/bset.h"
		"${SPDir}/game/bstate.h"
		"${SPDir}/game/common_headers.h"
		"${SPDir}/game/g_functions.h"
		"${SPDir}/game/g_items.h"
		"${SPDir}/game/g_local.h"
		"${SPDir}/game/g_nav.h"
		"${SPDir}/game/g_public.h"
		"${SPDir}/game/g_shared.h"
		"${SPDir}/game/genericparser2.h"
		"${SPDir}/game/statindex.h"
		"${SPDir}/game/teams.h"
		"${SPDir}/game/weapons.h"
		"${SPDir}/game/surfaceflags.h"
		"${SPDir}/game/ghoul2_shared.h"
		)
	source_group("game" FILES ${SPEngineGameFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineGameFiles})


	# MP3 files
	set(SPEngineMP3Files
		"${SPDir}/mp3code/cdct.c"
		"${SPDir}/mp3code/csbt.c"
		"${SPDir}/mp3code/csbtb.c"
		"${SPDir}/mp3code/csbtl3.c"
		"${SPDir}/mp3code/cup.c"
		"${SPDir}/mp3code/cupini.c"
		"${SPDir}/mp3code/cupl1.c"
		"${SPDir}/mp3code/cupl3.c"
		"${SPDir}/mp3code/cwin.c"
		"${SPDir}/mp3code/cwinb.c"
		"${SPDir}/mp3code/cwinm.c"
		"${SPDir}/mp3code/hwin.c"
		"${SPDir}/mp3code/l3dq.c"
		"${SPDir}/mp3code/l3init.c"
		"${SPDir}/mp3code/mdct.c"
		"${SPDir}/mp3code/mhead.c"
		"${SPDir}/mp3code/msis.c"
		"${SPDir}/mp3code/towave.c"
		"${SPDir}/mp3code/uph.c"
		"${SPDir}/mp3code/upsf.c"
		"${SPDir}/mp3code/wavep.c"
		"${SPDir}/mp3code/config.h"
		"${SPDir}/mp3code/htable.h"
		"${SPDir}/mp3code/jdw.h"
		"${SPDir}/mp3code/l3.h"
		"${SPDir}/mp3code/mhead.h"
		"${SPDir}/mp3code/mp3struct.h"
		"${SPDir}/mp3code/port.h"
		"${SPDir}/mp3code/small_header.h"
		"${SPDir}/mp3code/tableawd.h"
		)
	source_group("mp3" FILES ${SPEngineMP3Files})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineMP3Files})

	set(SPEngineRendererFiles
		"${SPDir}/renderer/mdx_format.h"
		"${SPDir}/renderer/tr_public.h"
		"${SPDir}/renderer/tr_types.h"
		)


	# UI files
	set(SPEngineUIFiles
		"${SPDir}/ui/ui_atoms.cpp"
		"${SPDir}/ui/ui_connect.cpp"
		"${SPDir}/ui/ui_main.cpp"
		"${SPDir}/ui/ui_saber.cpp"
		"${SPDir}/ui/ui_shared.cpp"
		"${SPDir}/ui/ui_syscalls.cpp"
		"${SPDir}/ui/gameinfo.h"
		"${SPDir}/ui/menudef.h"
		"${SPDir}/ui/ui_local.h"
		"${SPDir}/ui/ui_public.h"
		"${SPDir}/ui/ui_shared.h"
		)
	source_group("ui" FILES ${SPEngineUIFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineUIFiles})


	# RMG files
	set(SPEngineRMGFiles
		"${SPDir}/RMG/RM_Area.cpp"
		"${SPDir}/RMG/RM_Instance.cpp"
		"${SPDir}/RMG/RM_Instance_BSP.cpp"
		"${SPDir}/RMG/RM_Instance_Group.cpp"
		"${SPDir}/RMG/RM_Instance_Random.cpp"
		"${SPDir}/RMG/RM_Instance_Void.cpp"
		"${SPDir}/RMG/RM_InstanceFile.cpp"
		"${SPDir}/RMG/RM_Manager.cpp"
		"${SPDir}/RMG/RM_Mission.cpp"
		"${SPDir}/RMG/RM_Objective.cpp"
		"${SPDir}/RMG/RM_Path.cpp"
		"${SPDir}/RMG/RM_Terrain.cpp"
		"${SPDir}/RMG/RM_Area.h"
		"${SPDir}/RMG/RM_Headers.h"
		"${SPDir}/RMG/RM_Instance.h"
		"${SPDir}/RMG/RM_Instance_BSP.h"
		"${SPDir}/RMG/RM_Instance_Group.h"
		"${SPDir}/RMG/RM_Instance_Random.h"
		"${SPDir}/RMG/RM_Instance_Void.h"
		"${SPDir}/RMG/RM_InstanceFile.h"
		"${SPDir}/RMG/RM_Manager.h"
		"${SPDir}/RMG/RM_Mission.h"
		"${SPDir}/RMG/RM_Objective.h"
		"${SPDir}/RMG/RM_Path.h"
		"${SPDir}/RMG/RM_Terrain.h"
		)
	source_group("rmg" FILES ${SPEngineRMGFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineRMGFiles})


	# Ghoul2 files
	set(SPEngineGhoul2Files
		"${SPDir}/ghoul2/G2.h"
		"${SPDir}/ghoul2/ghoul2_gore.h"	
		)
	source_group("ghoul2" FILES ${SPEngineGhoul2Files})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineGhoul2Files})


	# Server files
	set(SPEngineCGameFiles
		"${SPDir}/cgame/cg_public.h"
		"${SPDir}/cgame/animtable.h"
		)
	source_group("cgame" FILES ${SPEngineCGameFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineCGameFiles})


	# 
	if(WIN32)
		set(SPEngineWin32Files
			"${SPDir}/win32/afxres.h"
			"${SPDir}/win32/win_input.cpp"
			"${SPDir}/win32/win_main.cpp"
			"${SPDir}/win32/win_shared.cpp"
			"${SPDir}/win32/win_snd.cpp"
			"${SPDir}/win32/win_syscon.cpp"
			"${SPDir}/win32/win_video.cpp"
			"${SPDir}/win32/win_wndproc.cpp"
			"${SPDir}/win32/win_local.h"
			"${SPDir}/win32/AutoVersion.h"
			"${SPDir}/win32/resource.h"
			"${SPDir}/win32/glw_win.h"
			"${SPDir}/win32/winquake.rc"
			)
		source_group("win32" FILES ${SPEngineWin32Files})
		set(SPEngineFiles ${SPEngineFiles} ${SPEngineWin32Files})
	else(WIN32)
		set(SPEngineSysFiles
			"${SPDir}/sys/sys_main.cpp"
			"${SPDir}/sys/sys_unix.cpp"
			"${SPDir}/sys/sys_local.h"
			"${SPDir}/sys/sys_loadlib.h"
			"${SPDir}/sdl/sdl_qgl.h"
			"${SPDir}/sdl/sdl_snd.cpp"
			"${SPDir}/sdl/sdl_input.cpp"
			)
		source_group("sys" FILES ${SPEngineSysFiles})
		set(SPEngineFiles ${SPEngineFiles} ${SPEngineSysFiles})
	endif(WIN32)

	if(UseInternalZlib)
		# zlib.lib is included for windows
		find_library(ZlibLibrary NAMES zlib PATHS ${OpenJKLibDir})
		if(NOT ZlibLibrary)
			message(FATAL_ERROR "UseInternalZlib enabled, but lib/zlib.lib not found!")
		endif(NOT ZlibLibrary)
		set(SPEngineLibraries ${SPEngineLibraries} ${ZlibLibrary})
	else(UseInternalZlib)
		find_package(ZLIB REQUIRED)
		set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${ZLIB_INCLUDE_DIR})
		set(SPEngineLibraries ${SPEngineLibraries} ${ZLIB_LIBRARY})
	endif(UseInternalZlib)

	if(MakeApplicationBundles)
		set(MACOSX_BUNDLE_ICON_FILE OpenJK.icns)
		set_source_files_properties(${SPDir}/macosx/OpenJK.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		set(SPEngineFiles ${SPEngineFiles} ${SPDir}/macosx/OpenJK.icns)
	endif(MakeApplicationBundles)

	if(WIN32)
		add_executable(${SPEngine} WIN32 ${SPEngineFiles})
	else(WIN32)
		if(MakeApplicationBundles)
			add_executable(${SPEngine} MACOSX_BUNDLE ${SPEngineFiles})
		else(MakeApplicationBundles)
			add_executable(${SPEngine} ${SPEngineFiles})
		endif(MakeApplicationBundles)
	endif(WIN32)

	if(MakeApplicationBundles)
		install(TARGETS ${SPEngine} BUNDLE DESTINATION ".")
	else(MakeApplicationBundles)
		install(TARGETS ${SPEngine} RUNTIME DESTINATION ".")
	endif(MakeApplicationBundles)

	set_target_properties(${SPEngine} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SPEngineDefines};${ReleaseDefines}")
	set_target_properties(${SPEngine} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SPEngineDefines};${ReleaseDefines}")
	set_target_properties(${SPEngine} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SPEngineDefines};${ReleaseDefines}")
	set_target_properties(${SPEngine} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SPEngineDefines};${DebugDefines}")
	set_target_properties(${SPEngine} PROPERTIES INCLUDE_DIRECTORIES "${SPEngineIncludeDirectories}")
	if(SPEngineLibraries)
		target_link_libraries(${SPEngine} ${SPEngineLibraries})
	endif(SPEngineLibraries)

endif(BuildSPEngine)
