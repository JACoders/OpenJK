cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

set(MPSharedDefines ${SharedDefines})

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
if(BuildMPRdVanilla)
	add_subdirectory("${MPDir}/rd-vanilla")
endif(BuildMPRdVanilla)


#    Common things between Engine and Dedicated Server
if(BuildMPEngine OR BuildMPDed)

	#    Botlib
	# the files could arguably just be put into the engine and dedicated projects without having a library for it.
	set(MPBotlibDefines ${SharedDefines} "BOTLIB")
	set(MPBotlibIncludeDirectories ${MPDir})
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
		"${MPDir}/qcommon/q_platform.h"
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
	set_target_properties(${MPBotLib} PROPERTIES PROJECT_LABEL "Bot Library")

	#    Common files/libraries/defines of both Engine and Dedicated Server

	# libraries: Botlib
	set(MPEngineAndDedLibraries ${MPBotLib})
	# Platform-specific libraries
	if(WIN32)
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} "winmm" "wsock32")
	endif(WIN32)
	# Include directories
	set(MPEngineAndDedIncludeDirectories ${MPDir} ${OpenJKLibDir}) # codemp folder, since includes are not always relative in the files

	if(UseInternalZlib)
		set(MPEngineAndDedZLibFiles
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
		source_group("zlib" FILES ${MPEngineAndDedZLibFiles})
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedZLibFiles})
		
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} "${OpenJKLibDir}/zlib")
	else()
		find_package(ZLIB REQUIRED)
		set(MPEngineAndDedIncludeDirectories ${MPEngineAndDedIncludeDirectories} ${ZLIB_INCLUDE_DIR})
		set(MPEngineAndDedLibraries ${MPEngineAndDedLibraries} ${ZLIB_LIBRARY})
	endif()

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
		"${MPDir}/qcommon/q_platform.h"
		"${MPDir}/qcommon/cm_load.cpp"
		"${MPDir}/qcommon/cm_local.h"
		"${MPDir}/qcommon/cm_patch.cpp"
		"${MPDir}/qcommon/cm_patch.h"
		"${MPDir}/qcommon/cm_polylib.cpp"
		"${MPDir}/qcommon/cm_polylib.h"
		"${MPDir}/qcommon/cm_public.h"
		"${MPDir}/qcommon/cm_test.cpp"
		"${MPDir}/qcommon/cm_trace.cpp"
		"${MPDir}/qcommon/cmd.cpp"
		"${MPDir}/qcommon/common.cpp"
		"${MPDir}/qcommon/cvar.cpp"
		"${MPDir}/qcommon/disablewarnings.h"
		"${MPDir}/qcommon/files.cpp"
		"${MPDir}/qcommon/game_version.h"
		"${MPDir}/qcommon/GenericParser2.cpp"
		"${MPDir}/qcommon/GenericParser2.h"
		"${MPDir}/qcommon/huffman.cpp"
		"${MPDir}/qcommon/md4.cpp"
		"${MPDir}/qcommon/md5.cpp"
		"${MPDir}/qcommon/MiniHeap.h"
		"${MPDir}/qcommon/msg.cpp"
		"${MPDir}/qcommon/matcomp.h"
		"${MPDir}/qcommon/matcomp.cpp"
		"${MPDir}/qcommon/net_chan.cpp"
		"${MPDir}/qcommon/net_ip.cpp"
		"${MPDir}/qcommon/persistence.cpp"
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
		"${MPDir}/qcommon/z_memman_pc.cpp"
		)
	if(WIN32)
		set(MPEngineAndDedCommonFiles ${MPEngineAndDedCommonFiles})
	endif(WIN32)
	source_group("common" FILES ${MPEngineAndDedCommonFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedCommonFiles})

	set(MPEngineAndDedG2Files
		"${MPDir}/ghoul2/G2.h"
		"${MPDir}/ghoul2/G2_gore.h"
		"${MPDir}/ghoul2/ghoul2_shared.h"
		"${MPDir}/ghoul2/g2_local.h"
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

	set(MPEngineAndDedServerFiles
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
		"${MPDir}/server/sv_gameapi.cpp"
		"${MPDir}/server/sv_gameapi.h"
		)
	source_group("server" FILES ${MPEngineAndDedServerFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedServerFiles})

	set(MPEngineAndDedUiFiles
		"${MPDir}/ui/keycodes.h"
		"${MPDir}/ui/ui_public.h"
		)
	source_group("ui" FILES ${MPEngineAndDedUiFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedUiFiles})

	set(MPEngineAndDedMinizipFiles
		"${OpenJKLibDir}/minizip/ioapi.c"
		"${OpenJKLibDir}/minizip/ioapi.h"
		"${OpenJKLibDir}/minizip/unzip.cpp"
		"${OpenJKLibDir}/minizip/unzip.h"
		)
	source_group("minizip" FILES ${MPEngineAndDedMinizipFiles})
	set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedMinizipFiles})

	if(WIN32)
		set(MPEngineAndDedWin32Files
			"${MPDir}/win32/win_local.h"
			"${MPDir}/win32/win_shared.cpp"
			"${MPDir}/sys/snapvector.cpp"
			)
		source_group("win32" FILES ${MPEngineAndDedWin32Files})
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedWin32Files})
	else(WIN32)
		set(MPEngineAndDedSysFiles
			"${MPDir}/sys/snapvector.cpp"
			)
		set(MPEngineAndDedFiles ${MPEngineAndDedFiles} ${MPEngineAndDedSysFiles})
		source_group("sys" FILES ${MPEngineAndDedSysFiles})
	endif(WIN32)
endif(BuildMPEngine OR BuildMPDed)

#        Engine/Executable (jamp.exe)

if(BuildMPEngine)
	set(MPEngineLibraries ${MPEngineAndDedLibraries})
	set(MPEngineIncludeDirectories ${MPEngineAndDedIncludeDirectories})
	set(MPEngineFiles ${MPEngineAndDedFiles})
	set(MPEngineDefines ${MPSharedDefines})

	#    Dependencies

	if (NOT WIN32)
		# OpenGL
		find_package(OpenGL REQUIRED)
		set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${OPENGL_INCLUDE_DIR})
		set(MPEngineLibraries ${MPEngineLibraries} ${OPENGL_LIBRARIES})
	endif()

	# OpenAL (is optionally included for Windows)
	if (WIN32 AND NOT WIN64)
		if(UseInternalOpenAL)
			set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} "${MPDir}/client/OpenAL")
			# There's a libs/OpenAL32.lib
			# (mrw) TODO Is this actually required on windows? It is loaded at runtime, should not be linked?
			find_library(OpenALLibrary NAMES OpenAL32 PATHS ${OpenJKLibDir})
			if(NOT OpenALLibrary)
				message(FATAL_ERROR "UseInternalOpenAL enabled, but lib/OpenAL32.lib not found!")
			endif(NOT OpenALLibrary)
			set(MPEngineLibraries ${MPEngineLibraries} ${OpenALLibrary})
		else(UseInternalOpenAL)
			find_package(OpenAL REQUIRED)
			set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${OPENAL_INCLUDE_DIR})
			set(MPEngineLibraries ${MPEngineLibraries} ${OPENAL_LIBRARY})
		endif(UseInternalOpenAL)
	endif()

	if(NOT WIN32)
		find_package(SDL2 REQUIRED)
		set(MPEngineIncludeDirectories ${MPEngineIncludeDirectories} ${SDL2_INCLUDE_DIR})
		set(MPEngineLibraries ${MPEngineLibraries} ${SDL2_LIBRARY})
	endif(NOT WIN32)

	# EAX is Windows-Only (right?)
	if(WIN32)
		set(MPEngineEAXFiles
			"${MPDir}/client/eax/eax.h"
			"${MPDir}/client/eax/EaxMan.h"
			)
		source_group("eax" FILES ${MPEngineEAXFiles})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineEAXFiles})
	endif(WIN32)

	#    Source Files

	set(MPEngineClientFiles
		"${MPDir}/client/cl_avi.cpp"
		"${MPDir}/client/cl_cgame.cpp"
		"${MPDir}/client/cl_cgameapi.cpp"
		"${MPDir}/client/cl_cgameapi.h"
		"${MPDir}/client/cl_cin.cpp"
		"${MPDir}/client/cl_console.cpp"
		"${MPDir}/client/cl_input.cpp"
		"${MPDir}/client/cl_keys.cpp"
		"${MPDir}/client/cl_lan.cpp"
		"${MPDir}/client/cl_lan.h"
		"${MPDir}/client/cl_main.cpp"
		"${MPDir}/client/cl_net_chan.cpp"
		"${MPDir}/client/cl_parse.cpp"
		"${MPDir}/client/cl_scrn.cpp"
		"${MPDir}/client/cl_ui.cpp"
		"${MPDir}/client/cl_uiapi.cpp"
		"${MPDir}/client/cl_uiapi.h"
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
			"${MPDir}/win32/openjk.rc"
			)
		source_group("win32" FILES ${MPEngineWin32Files})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineWin32Files})
	else(WIN32)
		set(MPEngineSysFiles
			"${MPDir}/sdl/sdl_input.cpp"
			"${MPDir}/sdl/sdl_qgl.h"
			"${MPDir}/sdl/sdl_snd.cpp"
			"${MPDir}/sys/sys_loadlib.h"
			"${MPDir}/sys/sys_local.h"
			"${MPDir}/sys/sys_main.cpp"
			"${MPDir}/sys/sys_unix.cpp"
			"${MPDir}/sys/unix_local.h"
			)
		source_group("sys" FILES ${MPEngineSysFiles})
		set(MPEngineFiles ${MPEngineFiles} ${MPEngineSysFiles})
	endif(WIN32)

	# todo: windows resources

	if(MakeApplicationBundles)
		set(MACOSX_BUNDLE_BUNDLE_NAME ${MPEngine})
		set(MACOSX_BUNDLE_DISPLAY_NAME ${MPEngine})
		set(MACOSX_BUNDLE_ICON_FILE OpenJK.icns)
		set_source_files_properties(${MPDir}/macosx/OpenJK.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		set(MPEngineFiles ${MPEngineFiles} ${MPDir}/macosx/OpenJK.icns)
		set_source_files_properties(${MPDir}/macosx/English.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION Resources/English.lproj)
		set(MPEngineFiles ${MPEngineFiles} ${MPDir}/macosx/English.lproj/InfoPlist.strings)
	endif(MakeApplicationBundles)

	if(WIN32)
		add_executable(${MPEngine} WIN32 ${MPEngineFiles})
	else(WIN32)
		if(MakeApplicationBundles)
			add_executable(${MPEngine} MACOSX_BUNDLE ${MPEngineFiles})
			set_target_properties(${MPEngine} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${MPDir}/macosx/OpenJKInfo.plist")
		else(MakeApplicationBundles)
			add_executable(${MPEngine} ${MPEngineFiles})
		endif(MakeApplicationBundles)
	endif(WIN32)

	if(MakeApplicationBundles)
		install(TARGETS ${MPEngine} BUNDLE DESTINATION ".")
	else(MakeApplicationBundles)
		install(TARGETS ${MPEngine} RUNTIME DESTINATION ".")
	endif(MakeApplicationBundles)

	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPEngineDefines};${ReleaseDefines}")
	set_target_properties(${MPEngine} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPEngineDefines};${DebugDefines}")
	set_target_properties(${MPEngine} PROPERTIES INCLUDE_DIRECTORIES "${MPEngineIncludeDirectories}")
	set_target_properties(${MPEngine} PROPERTIES PROJECT_LABEL "MP Client")
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
		"${MPDir}/ghoul2/G2_gore.cpp"
		"${MPDir}/rd-common/mdx_format.h"
		"${MPDir}/rd-common/tr_public.h"
		"${MPDir}/rd-dedicated/tr_local.h"
		"${MPDir}/rd-dedicated/G2_API.cpp"
		"${MPDir}/rd-dedicated/G2_bolts.cpp"
		"${MPDir}/rd-dedicated/G2_bones.cpp"
		"${MPDir}/rd-dedicated/G2_misc.cpp"
		"${MPDir}/rd-dedicated/G2_surfaces.cpp"
		"${MPDir}/rd-dedicated/tr_backend.cpp"
		"${MPDir}/rd-dedicated/tr_ghoul2.cpp"
		"${MPDir}/rd-dedicated/tr_init.cpp"
		"${MPDir}/rd-dedicated/tr_main.cpp"
		"${MPDir}/rd-dedicated/tr_mesh.cpp"
		"${MPDir}/rd-dedicated/tr_model.cpp"
		"${MPDir}/rd-dedicated/tr_shader.cpp"
		"${MPDir}/rd-dedicated/tr_skin.cpp")
	source_group("renderer" FILES ${MPDedicatedRendererFiles})
	set(MPDedFiles ${MPDedFiles} ${MPDedicatedRendererFiles})

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
			"${MPDir}/win32/win_main_ded.cpp"
			"${MPDir}/win32/openjkded.rc"
			)
		source_group("win32" FILES ${MPDedWin32Files})
		set(MPDedFiles ${MPDedFiles} ${MPDedWin32Files})
	else(WIN32)
		set(MPDedSysFiles
			"${MPDir}/sys/sys_main.cpp"
			"${MPDir}/sys/sys_unix.cpp"
			"${MPDir}/sys/unix_local.h"
			)
		set(MPDedLibraries ${MPDedLibraries} ${CMAKE_DL_LIBS})
		source_group("sys" FILES ${MPDedSysFiles})
		set(MPDedFiles ${MPDedFiles} ${MPDedSysFiles})
	endif(WIN32)

	add_executable(${MPDed} ${MPDedFiles})
	install(TARGETS ${MPDed} RUNTIME DESTINATION ".")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPDedDefines};${ReleaseDefines}")
	set_target_properties(${MPDed} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPDedDefines};${DebugDefines}")
	set_target_properties(${MPDed} PROPERTIES INCLUDE_DIRECTORIES "${MPDedIncludeDirectories}")
	set_target_properties(${MPDed} PROPERTIES PROJECT_LABEL "MP Dedicated Server")
	target_link_libraries(${MPDed} ${MPDedLibraries})
endif(BuildMPDed)
