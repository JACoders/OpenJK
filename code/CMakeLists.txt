#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
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

#    Add Game Project
if(BuildSPGame)
	add_subdirectory("${SPDir}/game")
endif(BuildSPGame)

#    Add Modular Renderer project
add_subdirectory("${SPDir}/rd-vanilla")

#    Add Engine Project
if(BuildSPEngine OR BuildJK2SPEngine)
	# Libraries
	if(WIN32)
		set(SPEngineLibraries "winmm")
	endif(WIN32)
	# Defines
	set(SPEngineDefines ${SharedDefines} "_JK2EXE") # it's called JK2EXE but it really just means Singleplayer Exe

	set(SPEngineIncludeDirectories
		${SPEngineIncludeDirectories}
		${SharedDir}
		${CMAKE_BINARY_DIR}/shared
		${SPDir}
		"${GSLIncludeDirectory}")

	#    Dependencies

	if (NOT WIN32)
		# OpenGL
		find_package(OpenGL REQUIRED)
		set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${OPENGL_INCLUDE_DIR})
		set(SPEngineLibraries ${SPEngineLibraries} ${OPENGL_LIBRARIES})
	endif()

	# OpenAL (is optionally included for Windows)
	if(MSVC AND NOT WIN64)
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

		set(SPEngineEAXFiles
			"${SPDir}/client/eax/eax.h"
			"${SPDir}/client/eax/EaxMan.h"
			)
		source_group("eax" FILES ${SPEngineEAXFiles})
		set(SPEngineFiles ${SPEngineFiles} ${SPEngineEAXFiles})
	endif()

	if(UseInternalSDL2)
		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(SPEngineLibraries
				${SPEngineLibraries}
				${OpenJKLibDir}/SDL2/lib/x86/SDL2main.lib
				${OpenJKLibDir}/SDL2/lib/x86/SDL2.lib
				)
		else()
			set(SPEngineLibraries
				${SPEngineLibraries}
				${OpenJKLibDir}/SDL2/lib/x64/SDL2main.lib
				${OpenJKLibDir}/SDL2/lib/x64/SDL2.lib
				)
		endif()

		set(SPEngineIncludeDirectories
			${SPEngineIncludeDirectories}
			${OpenJKLibDir}/SDL2/include
			)
	else()
		find_package(SDL2 REQUIRED CONFIG)
		if (TARGET SDL2::SDL2)
			set(SDL2_INCLUDE_DIRS "")
			set(SDL2_LIBRARIES SDL2::SDL2)
		endif()
		string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
		set(SPEngineIncludeDirectories ${SPEngineIncludeDirectories} ${SDL2_INCLUDE_DIRS})
		set(SPEngineLibraries ${SPEngineLibraries} ${SDL2_LIBRARIES})
	endif()

	#    Source Files

	# Client files
	set(SPEngineClientFiles
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
		"${SPDir}/qcommon/cm_load.cpp"
		"${SPDir}/qcommon/cm_patch.cpp"
		"${SPDir}/qcommon/cm_polylib.cpp"
		"${SPDir}/qcommon/cm_test.cpp"
		"${SPDir}/qcommon/cm_trace.cpp"
		"${SPDir}/qcommon/cm_local.h"
		"${SPDir}/qcommon/cm_patch.h"
		"${SPDir}/qcommon/cm_polylib.h"
		"${SPDir}/qcommon/cm_public.h"

		"${SPDir}/qcommon/cmd.cpp"
		"${SPDir}/qcommon/common.cpp"
		"${SPDir}/qcommon/cvar.cpp"

		"${SPDir}/qcommon/files.cpp"

		"${SPDir}/qcommon/md4.cpp"
		"${SPDir}/qcommon/msg.cpp"
		"${SPDir}/qcommon/net_chan.cpp"

		"${SPDir}/qcommon/q_shared.cpp"
		"${SPDir}/qcommon/q_shared.h"

		"${SPDir}/qcommon/sstring.h"
		"${SPDir}/qcommon/stringed_ingame.cpp"
		"${SPDir}/qcommon/stringed_interface.cpp"
		"${SPDir}/qcommon/stringed_ingame.h"
		"${SPDir}/qcommon/stringed_interface.h"

		"${SPDir}/qcommon/strip.cpp"
		"${SPDir}/qcommon/persistence.cpp"
		"${SPDir}/qcommon/tags.h"
		"${SPDir}/qcommon/z_memman_pc.cpp"
		"${SPDir}/qcommon/qfiles.h"
		"${SPDir}/qcommon/qcommon.h"
		"${SPDir}/qcommon/stv_version.h"
		"${SPDir}/qcommon/timing.h"
		"${SPDir}/qcommon/MiniHeap.h"

		"${SPDir}/qcommon/ojk_i_saved_game.h"
		"${SPDir}/qcommon/ojk_saved_game.h"
		"${SPDir}/qcommon/ojk_saved_game.cpp"
		"${SPDir}/qcommon/ojk_saved_game_helper.h"
		"${SPDir}/qcommon/ojk_saved_game_helper_fwd.h"
		"${SPDir}/qcommon/ojk_scope_guard.h"

		${SharedCommonFiles}
		)
	source_group("common" FILES ${SPEngineCommonFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineCommonFiles})

	set(SPEngineCommonSafeFiles
		"${SharedDir}/qcommon/safe/files.cpp"
		"${SharedDir}/qcommon/safe/files.h"
		${SharedCommonSafeFiles}
		"${SPDir}/qcommon/safe/memory.h"
		)
	source_group("common/safe" FILES ${SPEngineCommonSafeFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineCommonSafeFiles})


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
		"${SPDir}/rd-common/mdx_format.h"
		"${SPDir}/rd-common/tr_public.h"
		"${SPDir}/rd-common/tr_types.h"
		)
	source_group("renderer" FILES ${SPEngineRendererFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineRendererFiles})

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

	if(WIN32)
		set(SPEngineWin32Files
			"${SPDir}/win32/resource.h"
			"${SPDir}/win32/winquake.rc"
			"${SharedDir}/sys/win_manifest.manifest"
			)
		source_group("win32" FILES ${SPEngineWin32Files})
		set(SPEngineFiles ${SPEngineFiles} ${SPEngineWin32Files})
	endif()

	set(SPEngineSysFiles
		"${SharedDir}/sys/sys_main.cpp"
		"${SharedDir}/sys/sys_event.cpp"
		"${SharedDir}/sys/sys_local.h"
		"${SharedDir}/sys/sys_public.h"
		"${SharedDir}/sys/sys_loadlib.h"
		"${SharedDir}/sys/con_local.h"
		"${SharedDir}/sys/con_log.cpp"
		"${SharedDir}/sdl/sdl_qgl.h"
		"${SharedDir}/sdl/sdl_sound.cpp"
		"${SharedDir}/sdl/sdl_input.cpp"
		"${SharedDir}/sdl/sdl_window.cpp"
		"${SharedDir}/sdl/sdl_icon.h"
		)

	if(WIN32)
		set(SPEngineSysFiles
			${SPEngineSysFiles}
			"${SharedDir}/sys/sys_win32.cpp"
			"${SharedDir}/sys/con_passive.cpp"
			)
	else()
		set(SPEngineSysFiles
			${SPEngineSysFiles}
			"${SharedDir}/sys/sys_unix.cpp"
			"${SharedDir}/sys/con_tty.cpp"
			)
	endif()

	source_group("sys" FILES ${SPEngineSysFiles})
	set(SPEngineFiles ${SPEngineFiles} ${SPEngineSysFiles})

    # Transparently use our bundled minizip.
    list(APPEND SPEngineIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
    list(APPEND SPEngineLibraries          ${MINIZIP_LIBRARIES})

    # Transparently use either bundled or system zlib.
    list(APPEND SPEngineIncludeDirectories ${ZLIB_INCLUDE_DIR})
    list(APPEND SPEngineLibraries          ${ZLIB_LIBRARIES})

	# project macro so we can invoke it twice: for jk2 and for ja
	function(add_sp_project ProjectName Label SPDirName InstallDir Component)
		if(MakeApplicationBundles)
			set_source_files_properties(${SPDirName}/macosx/OpenJK.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
			set(SPEngineFiles ${SPEngineFiles} ${SPDirName}/macosx/OpenJK.icns)
			set_source_files_properties(${SPDirName}/macosx/English.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION Resources/English.lproj)
			set(SPEngineFiles ${SPEngineFiles} ${SPDirName}/macosx/English.lproj/InfoPlist.strings)
		endif(MakeApplicationBundles)

		if(WIN32)
			add_executable(${ProjectName} WIN32 ${SPEngineFiles})
			if(UseInternalSDL2)
				if(CMAKE_SIZEOF_VOID_P EQUAL 4)
					set(SPEngineExtraInstallFiles
						${OpenJKLibDir}/SDL2/bin/x86/SDL2.dll
						)
				else()
					set(SPEngineExtraInstallFiles
						${OpenJKLibDir}/SDL2/bin/x64/SDL2.dll
						)
				endif()
			endif()
		else(WIN32)
			if(MakeApplicationBundles)
				add_executable(${ProjectName} MACOSX_BUNDLE ${SPEngineFiles})
				set_target_properties(${ProjectName} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${SPDirName}/macosx/OpenJKInfo.plist")
			else(MakeApplicationBundles)
				add_executable(${ProjectName} ${SPEngineFiles})
			endif(MakeApplicationBundles)
		endif(WIN32)

		if(MakeApplicationBundles)
			install(CODE "
				include(BundleUtilities)
				set(BU_CHMOD_BUNDLE_ITEMS ON)
				fixup_bundle(\"${CMAKE_BINARY_DIR}/${ProjectName}.app\" \"\" \"\")
				"
				COMPONENT Runtime)
			install(TARGETS ${ProjectName}
				BUNDLE
				DESTINATION ${InstallDir}
				COMPONENT ${Component})
		else(MakeApplicationBundles)
			install(TARGETS ${ProjectName}
				RUNTIME
				DESTINATION ${InstallDir}
				COMPONENT ${Component})
			if(SPEngineExtraInstallFiles)
				install(FILES ${SPEngineExtraInstallFiles}
					DESTINATION ${InstallDir}
					COMPONENT ${Component})
			endif()
		endif(MakeApplicationBundles)

		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS "${SPEngineDefines}")

		# Hide symbols not explicitly marked public.
		set_property(TARGET ${ProjectName} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

		set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPEngineIncludeDirectories}")
		set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})
		if(SPEngineLibraries)
			target_link_libraries(${ProjectName} ${SPEngineLibraries})
		endif(SPEngineLibraries)
	endfunction(add_sp_project)
	if(BuildSPEngine)
		add_sp_project(${SPEngine} "SP Client" ${SPDir} ${JKAInstallDir} ${JKASPClientComponent})
	endif(BuildSPEngine)
	if(BuildJK2SPEngine)
		set(SPEngineDefines ${SPEngineDefines} "JK2_MODE")
		add_sp_project(${JK2SPEngine} "JK2 SP Client" ${JK2SPDir} ${JK2InstallDir} ${JK2SPClientComponent})
	endif(BuildJK2SPEngine)

endif(BuildSPEngine OR BuildJK2SPEngine)
