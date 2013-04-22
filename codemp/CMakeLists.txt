cmake_minimum_required(VERSION 2.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPSharedDefines ${SharedDefines} "_JK2")

#    Add Game Project
if(BuildMPGame)
	add_subdirectory("${MPDir}/game")
endif(BuildMPGame)

#    Add CGame Project
if(BuildMPCGame)
	add_subdirectory("${MPDir}/cgame")
endif(BuildMPCGame)

#    Add UI Project
if(BuildMPUI)
	add_subdirectory("${MPDir}/ui")
endif(BuildMPUI)

#	 Add Vanilla JKA Renderer Project
if(BuildMPEngine)
	add_subdirectory("${MPDir}/rd-vanilla")
endif(BuildMPEngine)


#        Common things between Engine and Dedicated Server
if(BuildMPEngine OR BuildMPDed)

	#    Botlib
	# the files could arguably just be put into the engine and dedicated projects without having a library for it.
	set(MPBotlibDefines ${SharedDefines} "BOTLIB")
	set(MPBotlibIncludeDirectories "${MPDir}")
	set(MPBotlibHeaderFiles
		"${MPDir}/botlib/aasfile.h"
		"${MPDir}/botlib/be_aas.h"
		"${MPDir}/botlib/be_aas_bsp.h"
		"${MPDir}/botlib/be_aas_cluster.h"
		"${MPDir}/botlib/be_aas_debug.h"
		"${MPDir}/botlib/be_aas_def.h"
		"${MPDir}/botlib/be_aas_entity.h"
		"${MPDir}/botlib/be_aas_file.h"
		"${MPDir}/botlib/be_aas_funcs.h"
		"${MPDir}/botlib/be_aas_main.h"
		"${MPDir}/botlib/be_aas_move.h"
		"${MPDir}/botlib/be_aas_optimize.h"
		"${MPDir}/botlib/be_aas_reach.h"
		"${MPDir}/botlib/be_aas_route.h"
		"${MPDir}/botlib/be_aas_routealt.h"
		"${MPDir}/botlib/be_aas_sample.h"
		"${MPDir}/botlib/be_ai_char.h"
		"${MPDir}/botlib/be_ai_chat.h"
		"${MPDir}/botlib/be_ai_gen.h"
		"${MPDir}/botlib/be_ai_goal.h"
		"${MPDir}/botlib/be_ai_move.h"
		"${MPDir}/botlib/be_ai_weap.h"
		"${MPDir}/botlib/be_ai_weight.h"
		"${MPDir}/botlib/be_ea.h"
		"${MPDir}/botlib/be_interface.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/botlib/l_crc.h"
		"${MPDir}/botlib/l_libvar.h"
		"${MPDir}/botlib/l_log.h"
		"${MPDir}/botlib/l_memory.h"
		"${MPDir}/botlib/l_precomp.h"
		"${MPDir}/botlib/l_script.h"
		"${MPDir}/botlib/l_struct.h"
		"${MPDir}/botlib/l_utils.h"
		"${MPDir}/qcommon/q_shared.h"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/server/server.h"
		"${MPDir}/game/surfaceflags.h"
		"${MPDir}/qcommon/tags.h"
		)
	source_group("Header Files" FILES ${MPBotlibHeaderFiles})
	set(MPBotlibSourceFiles
		"${MPDir}/botlib/be_aas_bspq3.cpp"
		"${MPDir}/botlib/be_aas_cluster.cpp"
		"${MPDir}/botlib/be_aas_debug.cpp"
		"${MPDir}/botlib/be_aas_entity.cpp"
		"${MPDir}/botlib/be_aas_file.cpp"
		"${MPDir}/botlib/be_aas_main.cpp"
		"${MPDir}/botlib/be_aas_move.cpp"
		"${MPDir}/botlib/be_aas_optimize.cpp"
		"${MPDir}/botlib/be_aas_reach.cpp"
		"${MPDir}/botlib/be_aas_route.cpp"
		"${MPDir}/botlib/be_aas_routealt.cpp"
		"${MPDir}/botlib/be_aas_sample.cpp"
		"${MPDir}/botlib/be_ai_char.cpp"
		"${MPDir}/botlib/be_ai_chat.cpp"
		"${MPDir}/botlib/be_ai_gen.cpp"
		"${MPDir}/botlib/be_ai_goal.cpp"
		"${MPDir}/botlib/be_ai_move.cpp"
		"${MPDir}/botlib/be_ai_weap.cpp"
		"${MPDir}/botlib/be_ai_weight.cpp"
		"${MPDir}/botlib/be_ea.cpp"
		"${MPDir}/botlib/be_interface.cpp"
		"${MPDir}/botlib/l_crc.cpp"
		"${MPDir}/botlib/l_libvar.cpp"
		"${MPDir}/botlib/l_log.cpp"
		"${MPDir}/botlib/l_memory.cpp"
		"${MPDir}/botlib/l_precomp.cpp"
		"${MPDir}/botlib/l_script.cpp"
		"${MPDir}/botlib/l_struct.cpp"
		)
	source_group("Source Files" FILES ${MPBotlibSourceFiles})
	
	add_library(${MPBotLib} STATIC ${MPBotlibHeaderFiles} ${MPBotlibSourceFiles})
	set_target_properties(${MPBotLib} PROPERTIES COMPILE_DEFINITIONS "${MPBotlibDefines}")
	set_target_properties(${MPBotLib} PROPERTIES INCLUDE_DIRECTORIES "${MPBotlibIncludeDirectories}")
	
	#    Common files/libraries/defines of both Engine and Dedicated Server
	
	# libraries: Botlib
	set(MPEngineAndDedLibraries ${MPBotLib})
	# Platform-specific libraries
	if(WIN32)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} "winmm" "wsock32")
	else(WIN32)
		message("You might need to add some libraries to codemp/CMakeLists.txt or you could get linker errors on non-Windows-OSs.")
	endif(WIN32)
	# Include directories
	set(MPEngineAndDedIncludeDirectories ${MPDir}) # codemp folder, since includes are not always relative in the files
	
	if(UseInternalZlib)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} "${MPDir}/zlib")
		# OpenAL32.lib is included for windows
		find_library(ZlibLibrary NAMES zlib PATHS "${MPDir}/libs/win32")
		if(NOT ZlibLibrary)
			message(FATAL_ERROR "UseInternalOpenAL set, but libs/OpenAL32 not found!")
		endif(NOT ZlibLibrary)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${ZlibLibrary})
	else(UseInternalZlib)
		find_package(ZLIB REQUIRED)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${ZLIB_INCLUDE_DIR})
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${ZLIB_LIBRARY})
	endif(UseInternalZlib)

	if(NOT WIN32)
		find_package(SDL2 REQUIRED)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${SDL2_INCLUDE_DIR})
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${SDL2_LIBRARY})
	endif(NOT WIN32)
	
	file(GLOB_RECURSE J_SRC "${MPDir}/jpeg-8c/*.c" "${MPDir}/jpeg-8c/*.h")
	
	set(MPEngineAndDedCgameFiles
		"${MPDir}/cgame/cg_public.h"
		)
	source_group("cgame" FILES ${MPEngineAndDedCgameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCgameFiles})
	
	set(MPEngineAndDedClientFiles
		"${MPDir}/client/client.h"
		"${MPDir}/client/keys.h"
		"${MPDir}/client/snd_public.h"
		)
	source_group("client" FILES ${MPEngineAndDedClientFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedClientFiles})
	
	set(MPEngineAndDedGameFiles
		"${MPDir}/game/anims.h"
		"${MPDir}/game/bg_public.h"
		"${MPDir}/game/bg_weapons.h"
		"${MPDir}/botlib/botlib.h"
		"${MPDir}/game/g_public.h"
		"${MPDir}/game/surfaceflags.h"
		)
	source_group("game" FILES ${MPEngineAndDedGameFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedGameFiles})
	
	set(MPEngineAndDedCommonFiles
		"${MPDir}/qcommon/q_shared.h"
		"${MPDir}/qcommon/chash.h"
		"${MPDir}/qcommon/cm_draw.cpp"
		"${MPDir}/qcommon/cm_draw.h"
		"${MPDir}/qcommon/cm_landscape.h"
		"${MPDir}/qcommon/cm_load.cpp"
		"${MPDir}/qcommon/cm_local.h"
		"${MPDir}/qcommon/cm_patch.cpp"
		"${MPDir}/qcommon/cm_patch.h"
		"${MPDir}/qcommon/cm_polylib.cpp"
		"${MPDir}/qcommon/cm_polylib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/cm_randomterrain.cpp"
		"${MPDir}/qcommon/cm_randomterrain.h"
		"${MPDir}/qcommon/cm_shader.cpp"
		"${MPDir}/qcommon/cm_terrain.cpp"
		"${MPDir}/qcommon/cm_terrainmap.h"
		"${MPDir}/qcommon/cm_test.cpp"
		"${MPDir}/qcommon/cm_trace.cpp"
		"${MPDir}/qcommon/cmd_common.cpp"
		"${MPDir}/qcommon/cmd_pc.cpp"
		"${MPDir}/qcommon/common.cpp"
		"${MPDir}/qcommon/cvar.cpp"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/qcommon/files_common.cpp"
		"${MPDir}/qcommon/files_pc.cpp"
		"${MPDir}/qcommon/game_version.h"
		"${MPDir}/qcommon/GenericParser2.cpp"
		"${MPDir}/qcommon/GenericParser2.h"
		"${MPDir}/qcommon/huffman.cpp"
		"${MPDir}/qcommon/md4.cpp"
		"${MPDir}/qcommon/MiniHeap.h"
		"${MPDir}/qcommon/msg.cpp"
		"${MPDir}/qcommon/matcomp.h"
		"${MPDir}/qcommon/matcomp.cpp"
		"${MPDir}/qcommon/net_chan.cpp"
		"${MPDir}/qcommon/q_math.cpp"
		"${MPDir}/qcommon/q_shared.cpp"
		"${MPDir}/qcommon/qcommon.h"
		"${MPDir}/qcommon/qfiles.h"
		"${MPDir}/qcommon/RoffSystem.cpp"
		"${MPDir}/qcommon/RoffSystem.h"
		"${MPDir}/qcommon/sstring.h"
		"${MPDir}/qcommon/stringed_ingame.cpp"
		"${MPDir}/qcommon/stringed_ingame.h"
		"${MPDir}/qcommon/stringed_interface.cpp"
		"${MPDir}/qcommon/stringed_interface.h"
		"${MPDir}/qcommon/tags.h"
		"${MPDir}/qcommon/timing.h"
		"${MPDir}/qcommon/vm.cpp"
		"${MPDir}/qcommon/vm_interpreted.cpp"
		"${MPDir}/qcommon/vm_local.h"
		"${MPDir}/qcommon/z_memman_pc.cpp"
		)
	source_group("common" FILES ${MPEngineAndDedCommonFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCommonFiles})
	
	set(MPEngineAndDedG2Files
		"${MPDir}/ghoul2/G2.h"
		"${MPDir}/ghoul2/G2_gore.h"
		"${MPDir}/ghoul2/ghoul2_shared.h"
		)
	source_group("ghoul2" FILES ${MPEngineAndDedG2Files})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedG2Files})
	
	set(MPEngineAndDedIcarusFiles
		"${MPDir}/icarus/BlockStream.cpp"
		"${MPDir}/icarus/blockstream.h"
		"${MPDir}/icarus/GameInterface.cpp"
		"${MPDir}/icarus/GameInterface.h"
		"${MPDir}/icarus/icarus.h"
		"${MPDir}/icarus/Instance.cpp"
		"${MPDir}/icarus/instance.h"
		"${MPDir}/icarus/Interface.cpp"
		"${MPDir}/icarus/interface.h"
		"${MPDir}/icarus/interpreter.h"
		"${MPDir}/icarus/Memory.cpp"
		"${MPDir}/icarus/module.h"
		"${MPDir}/icarus/Q3_Interface.cpp"
		"${MPDir}/icarus/Q3_Interface.h"
		"${MPDir}/icarus/Q3_Registers.cpp"
		"${MPDir}/icarus/Q3_Registers.h"
		"${MPDir}/icarus/Sequence.cpp"
		"${MPDir}/icarus/sequence.h"
		"${MPDir}/icarus/Sequencer.cpp"
		"${MPDir}/icarus/sequencer.h"
		"${MPDir}/icarus/TaskManager.cpp"
		"${MPDir}/icarus/taskmanager.h"
		"${MPDir}/icarus/tokenizer.h"
		)
	source_group("icarus" FILES ${MPEngineAndDedIcarusFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedIcarusFiles})
	
	set(MPEngineAndDedRmgFiles
		"${MPDir}/RMG/RM_Area.cpp"
		"${MPDir}/RMG/RM_Area.h"
		"${MPDir}/RMG/RM_Headers.h"
		"${MPDir}/RMG/RM_Instance.cpp"
		"${MPDir}/RMG/RM_Instance.h"
		"${MPDir}/RMG/RM_Instance_BSP.cpp"
		"${MPDir}/RMG/RM_Instance_BSP.h"
		"${MPDir}/RMG/RM_Instance_Group.cpp"
		"${MPDir}/RMG/RM_Instance_Group.h"
		"${MPDir}/RMG/RM_Instance_Random.cpp"
		"${MPDir}/RMG/RM_Instance_Random.h"
		"${MPDir}/RMG/RM_Instance_Void.cpp"
		"${MPDir}/RMG/RM_Instance_Void.h"
		"${MPDir}/RMG/RM_InstanceFile.cpp"
		"${MPDir}/RMG/RM_InstanceFile.h"
		"${MPDir}/RMG/RM_Manager.cpp"
		"${MPDir}/RMG/RM_Manager.h"
		"${MPDir}/RMG/RM_Mission.cpp"
		"${MPDir}/RMG/RM_Mission.h"
		"${MPDir}/RMG/RM_Objective.cpp"
		"${MPDir}/RMG/RM_Objective.h"
		"${MPDir}/RMG/RM_Path.cpp"
		"${MPDir}/RMG/RM_Path.h"
		"${MPDir}/RMG/RM_Terrain.cpp"
		"${MPDir}/RMG/RM_Terrain.h"
		)
	source_group("rmg" FILES ${MPEngineAndDedRmgFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedRmgFiles})
	
	set(MPEngineAndDedServerFiles
		"${MPDir}/server/NPCNav/gameCallbacks.cpp"
		"${MPDir}/server/NPCNav/navigator.cpp"
		"${MPDir}/server/NPCNav/navigator.h"
		"${MPDir}/server/server.h"
		"${MPDir}/server/sv_bot.cpp"
		"${MPDir}/server/sv_ccmds.cpp"
		"${MPDir}/server/sv_client.cpp"
		"${MPDir}/server/sv_game.cpp"
		"${MPDir}/server/sv_init.cpp"
		"${MPDir}/server/sv_main.cpp"
		"${MPDir}/server/sv_net_chan.cpp"
		"${MPDir}/server/sv_snapshot.cpp"
		"${MPDir}/server/sv_world.cpp"
		)
	source_group("server" FILES ${MPEngineAndDedServerFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedServerFiles})
	
	set(MPEngineAndDedUiFiles
		"${MPDir}/ui/keycodes.h"
		"${MPDir}/ui/ui_public.h"
		)
	source_group("ui" FILES ${MPEngineAndDedUiFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedUiFiles})
	
	# TODO: replace the following with proper library? Update it?
	set(MPEngineAndDedMinizipFiles
		"${MPDir}/qcommon/unzip.cpp"
		"${MPDir}/qcommon/unzip.h"
		"${MPDir}/zlib/ioapi.c"
		"${MPDir}/zlib/ioapi.h"
		)
	source_group("minizip" FILES ${MPEngineAndDedMinizipFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedMinizipFiles})
	
	if(WIN32)
		set(MPEngineAndDedAllSortMe
			${MPEngineAndDedAllSortMe}
			"${MPDir}/win32/win_local.h"
			"${MPDir}/win32/win_net.cpp"
			"${MPDir}/win32/win_shared.cpp"
			"${MPDir}/qcommon/vm_x86.cpp"
			)
		source_group("win32" FILES ${MPEngineAndDedWin32Files})
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedWin32Files})
	else(WIN32)
		set(MPEngineAndDedAllSortMe
			${MPEngineAndDedAllSortMe}
			"${MPDir}/sys/vm_x86.cpp"
			"${MPDir}/sys/snapvector.nasm"
			)
	endif(WIN32)
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedAllSortMe})
	source_group("SortMe" FILES ${MPEngineAndDedAllSortMe})
	source_group("jpeg-8c" FILES ${J_SRC})
	
endif(BuildMPEngine OR BuildMPDed)

#        Engine/Executable (jamp.exe)

if(BuildMPEngine)
	set(MPEngineLibraries ${MPEngineAndDedLibraries})
	set(MPEngineIncludeDirectories ${MPEngineAndDedIncludeDirectories})
	set(MPEngineFiles ${MPEngineAndDedFiles})
	set(MPEngineDefines ${MPSharedDefines} "_JK2MP")
	
	#    Dependencies
	
	# OpenGL
	# (mrw) TODO Is this actually required on windows? It is loaded at runtime, should not be linked?
	find_package(OpenGL REQUIRED)
	set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(MPEngineLibraries ${MPEngineLibraries} ${OPENGL_LIBRARIES})

	# OpenAL (is optionally included for Windows)
	if(UseInternalOpenAL)
		set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} "${MPDir}/client/OpenAL")
		# There's a libs/OpenAL32.lib
		# (mrw) TODO Is this actually required on windows? It is loaded at runtime, should not be linked?
		find_library(OpenALLibrary NAMES OpenAL32 PATHS "${MPDir}/libs/win32")
		if(NOT OpenALLibrary)
			message(FATAL_ERROR "UseInternalOpenAL set, but codemp/libs/win32/OpenAL32 not found!")
		endif(NOT OpenALLibrary)
		set(MPEngineLibraries ${MPEngineLibraries} ${OpenALLibrary})
	else(UseInternalOpenAL)
		find_package(OpenAL REQUIRED)
		set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${OPENAL_INCLUDE_DIR})
		set(MPEngineLibraries ${MPEngineLibraries} ${OPENAL_LIBRARY})
	endif(UseInternalOpenAL)
	
	# EAX is Windows-Only (right?)
	if(WIN32)
		set(MPEngineFiles ${MPEngineFiles}
			"${MPDir}/client/eax/eax.h"
			"${MPDir}/client/eax/EaxMan.h"
			)
	endif(WIN32)
	
	#    Source Files
	
	# TODO: Allow for use of external library
	file(GLOB_RECURSE MPEngineJpegFiles "${MPDir}/jpeg-8c/*.c" "${MPDir}/jpeg-8c/*.h")
	source_group("jpeg-8c" FILES ${MPEngineJpegFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineJpegFiles})
	
	set(MPEngineClientFiles
		"${MPDir}/client/cl_avi.cpp"
		"${MPDir}/client/cl_cgame.cpp"
		"${MPDir}/client/cl_cin.cpp"
		"${MPDir}/client/cl_console.cpp"
		"${MPDir}/client/cl_input.cpp"
		"${MPDir}/client/cl_keys.cpp"
		"${MPDir}/client/cl_main.cpp"
		"${MPDir}/client/cl_net_chan.cpp"
		"${MPDir}/client/cl_parse.cpp"
		"${MPDir}/client/cl_scrn.cpp"
		"${MPDir}/client/cl_ui.cpp"
		"${MPDir}/client/FXExport.cpp"
		"${MPDir}/client/FXExport.h"
		"${MPDir}/client/FxPrimitives.cpp"
		"${MPDir}/client/FxPrimitives.h"
		"${MPDir}/client/FxScheduler.cpp"
		"${MPDir}/client/FxScheduler.h"
		"${MPDir}/client/FxSystem.cpp"
		"${MPDir}/client/FxSystem.h"
		"${MPDir}/client/FxTemplate.cpp"
		"${MPDir}/client/FxUtil.cpp"
		"${MPDir}/client/FxUtil.h"
		"${MPDir}/client/snd_ambient.cpp"
		"${MPDir}/client/snd_ambient.h"
		"${MPDir}/client/snd_dma.cpp"
		"${MPDir}/client/snd_local.h"
		"${MPDir}/client/snd_mem.cpp"
		"${MPDir}/client/snd_mix.cpp"
		"${MPDir}/client/snd_mp3.cpp"
		"${MPDir}/client/snd_mp3.h"
		"${MPDir}/client/snd_music.cpp"
		"${MPDir}/client/snd_music.h"
		)
	source_group("client" FILES ${MPEngineClientFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineClientFiles})
	
	set(MPEngineCommonFiles
		"${MPDir}/qcommon/cm_terrainmap.cpp"
		"${MPDir}/qcommon/CNetProfile.cpp"
		"${MPDir}/qcommon/exe_headers.cpp"
		"${MPDir}/qcommon/exe_headers.h"
		"${MPDir}/qcommon/files.h"
		"${MPDir}/qcommon/hstring.cpp"
		"${MPDir}/qcommon/hstring.h"
		"${MPDir}/qcommon/INetProfile.h"
		"${MPDir}/qcommon/platform.h"
		)
	source_group("common" FILES ${MPEngineCommonFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineCommonFiles})
	
	set(MPEngineRatlFiles
		"${MPDir}/Ratl/bits_vs.h"
		"${MPDir}/Ratl/ratl_common.h"
		"${MPDir}/Ratl/vector_vs.h"
		)
	source_group("ratl" FILES ${MPEngineRatlFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineRatlFiles})
	
	set(MPEngineRavlFiles
		"${MPDir}/Ravl/CVec.h"
		)
	source_group("ravl" FILES ${MPEngineRavlFiles})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineRavlFiles})
	
	set(MPEngineMp3Files
		"${MPDir}/mp3code/cdct.c"
		"${MPDir}/mp3code/config.h"
		"${MPDir}/mp3code/csbt.c"
		"${MPDir}/mp3code/csbtb.c"
		"${MPDir}/mp3code/csbtl3.c"
		"${MPDir}/mp3code/cup.c"
		"${MPDir}/mp3code/cupini.c"
		"${MPDir}/mp3code/cupl1.c"
		"${MPDir}/mp3code/cupl3.c"
		"${MPDir}/mp3code/cwin.c"
		"${MPDir}/mp3code/cwinb.c"
		"${MPDir}/mp3code/cwinm.c"
		"${MPDir}/mp3code/htable.h"
		"${MPDir}/mp3code/hwin.c"
		"${MPDir}/mp3code/jdw.h"
		"${MPDir}/mp3code/l3.h"
		"${MPDir}/mp3code/l3dq.c"
		"${MPDir}/mp3code/l3init.c"
		"${MPDir}/mp3code/mdct.c"
		"${MPDir}/mp3code/mhead.c"
		"${MPDir}/mp3code/mhead.h"
		"${MPDir}/mp3code/mp3struct.h"
		"${MPDir}/mp3code/msis.c"
		"${MPDir}/mp3code/port.h"
		"${MPDir}/mp3code/small_header.h"
		"${MPDir}/mp3code/tableawd.h"
		"${MPDir}/mp3code/towave.c"
		"${MPDir}/mp3code/uph.c"
		"${MPDir}/mp3code/upsf.c"
		"${MPDir}/mp3code/wavep.c"
		)
	source_group("mp3code" FILES ${MPEngineMp3Files})
	set(MPEngineFiles ${MPEngineFiles} ${MPEngineMp3Files})
	
	if(WIN32)
		set(MPEngineWin32Files
			"${MPDir}/win32/glw_win.h"
			"${MPDir}/win32/icon.ico"
			"${MPDir}/win32/resource.h"
			"${MPDir}/win32/win_input.cpp"
			"${MPDir}/win32/win_main.cpp"
			"${MPDir}/win32/win_snd.cpp"
			"${MPDir}/win32/win_syscon.cpp"
			"${MPDir}/win32/win_wndproc.cpp"
			"${MPDir}/win32/engine.rc"
			)
		source_group("win32" FILES ${MPEngineWin32Files})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineWin32Files})
	else(WIN32)
		set(MPEngineSortMe
			${MPEngineSortMe}
			"${MPDir}/sdl/sdl_input.cpp"
			"${MPDir}/sdl/sdl_qgl.h"
			"${MPDir}/sdl/sdl_snd.cpp"
			"${MPDir}/sys/sys_loadlib.h"
			"${MPDir}/sys/sys_local.h"
			"${MPDir}/sys/sys_main.cpp"
			"${MPDir}/sys/sys_net.cpp"
			"${MPDir}/sys/sys_unix.cpp"
			"${MPDir}/sys/unix_local.h"
			)
	endif(WIN32)

	set(MPEngineFiles ${MPEngineFiles} ${MPEngineSortMe})
	source_group("SortMe" FILES ${MPEngineSortMe})

	# todo: windows resources

	if(WIN32)
		add_executable(${MPEngine} WIN32 ${MPEngineFiles})
	else(WIN32)
		add_executable(${MPEngine} ${MPEngineFiles})
	endif(WIN32)
    install(TARGETS ${MPEngine} RUNTIME DESTINATION ".")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPEngineDefines};${DebugDefines}")
	set_target_properties(${MPEngine} PROPERTIES INCLUDE_DIRECTORIES "${MPEngineIncludeDirectories}")
	target_link_libraries(${MPEngine} ${MPEngineLibraries})
endif(BuildMPEngine)

#        Dedicated Server (Engine) (jampded.exe)

if(BuildMPDed)
	set(MPDedLibraries ${MPEngineAndDedLibraries})
	set(MPDedIncludeDirectories ${MPEngineAndDedIncludeDirectories})
	set(MPDedFiles ${MPEngineAndDedFiles})
	set(MPDedDefines ${MPSharedDefines} "_CONSOLE" "DEDICATED")

	# Dedicated renderer is compiled with the server.
	set(MPDedicatedRendererFiles
		"${MPDir}/renderer/mdx_format.h"
		"${MPDir}/renderer/tr_public.h"
		"${MPDir}/rd-dedicated/tr_local.h"
		"${MPDir}/rd-dedicated/G2_API.cpp"
		"${MPDir}/rd-dedicated/G2_bolts.cpp"
		"${MPDir}/rd-dedicated/G2_bones.cpp"
		"${MPDir}/rd-dedicated/G2_misc.cpp"
		"${MPDir}/rd-dedicated/G2_surfaces.cpp"
		"${MPDir}/rd-dedicated/tr_backend.cpp"
		"${MPDir}/rd-dedicated/tr_ghoul2.cpp"
		"${MPDir}/rd-dedicated/tr_image.cpp"
		"${MPDir}/rd-dedicated/tr_init.cpp"
		"${MPDir}/rd-dedicated/tr_main.cpp"
		"${MPDir}/rd-dedicated/tr_mesh.cpp"
		"${MPDir}/rd-dedicated/tr_model.cpp"
		"${MPDir}/rd-dedicated/tr_shader.cpp")
	source_group("renderer" FILES ${MPDedicatedRendererFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedicatedRendererFiles})

	set(MPDedicatedPngFiles
		"${MPDir}/png/rpng.cpp")
	source_group("png" FILES ${MPVanillaRendererPngFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedicatedPngFiles})

	set(MPDedLibPngFiles
		"${MPDir}/libpng/png.c"
		"${MPDir}/libpng/pngerror.c"
		"${MPDir}/libpng/pngget.c"
		"${MPDir}/libpng/pngmem.c"
		"${MPDir}/libpng/pngpread.c"
		"${MPDir}/libpng/pngread.c"
		"${MPDir}/libpng/pngrio.c"
		"${MPDir}/libpng/pngrtran.c"
		"${MPDir}/libpng/pngrutil.c"
		"${MPDir}/libpng/pngset.c"
		"${MPDir}/libpng/pngtrans.c"
		"${MPDir}/libpng/pngwio.c"
		"${MPDir}/libpng/pngwrite.c"
		"${MPDir}/libpng/pngwtran.c"
		"${MPDir}/libpng/pngwutil.c")
	source_group("libpng" FILES ${MPDedLibPngFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedLibPngFiles})
	
	# TODO sort this
	set(MPDedNullFiles
		"${MPDir}/null/null_client.cpp"
		"${MPDir}/null/null_input.cpp"
		"${MPDir}/null/null_renderer.cpp"
		"${MPDir}/null/null_snddma.cpp"
		)
	source_group("null" FILES ${MPDedNullFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedNullFiles})
	if(WIN32)
		set(MPDedWin32Files
			"${MPDir}/null/win_main.cpp"
			"${MPDir}/win32/dedicated.rc"
			)
		source_group("win32" FILES ${MPDedWin32Files})
		set(MPDedFiles ${MPDedFiles} ${MPDedWin32Files})
	else(WIN32)
		set(MPDedSortMe ${MPDedSortMe}
			"${MPDir}/sys/sys_main.cpp"
			"${MPDir}/sys/sys_net.cpp"
			"${MPDir}/sys/sys_unix.cpp"
			"${MPDir}/sys/unix_local.h"
			)
		set(MPDedLibraries ${MPDedLibraries} ${CMAKE_DL_LIBS})
	endif(WIN32)
	set(MPDedFiles ${MPDedFiles} ${MPDedSortMe})
	source_group("SortMe" FILES ${MPDedSortMe})
	
	add_executable(${MPDed} ${MPDedFiles})
	install(TARGETS ${MPDed} RUNTIME DESTINATION ".")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPDedDefines};${DebugDefines}")
	set_target_properties(${MPDed} PROPERTIES INCLUDE_DIRECTORIES "${MPDedIncludeDirectories}")
	target_link_libraries(${MPDed} ${MPDedLibraries})
endif(BuildMPDed)
