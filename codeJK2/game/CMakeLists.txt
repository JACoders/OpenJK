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

set(JK2SPGameDefines ${SharedDefines} "JK2_MODE" "SP_GAME")

set(JK2SPGameIncludeDirectories
	"${JK2SPDir}"
	"${JK2SPDir}/game"
	"${SharedDir}"
	"${GSLIncludeDirectory}"
	"${CMAKE_BINARY_DIR}/shared"
	)
if(WIN32)
	set(JK2SPGameLibraries "winmm")
endif(WIN32)

set(JK2SPGameGameFiles
	"${JK2SPDir}/game/AI_Atst.cpp"
	"${JK2SPDir}/game/AI_Default.cpp"
	"${JK2SPDir}/game/AI_Droid.cpp"
	"${JK2SPDir}/game/AI_GalakMech.cpp"
	"${JK2SPDir}/game/AI_Grenadier.cpp"
	"${JK2SPDir}/game/AI_Howler.cpp"
	"${JK2SPDir}/game/AI_ImperialProbe.cpp"
	"${JK2SPDir}/game/AI_Interrogator.cpp"
	"${JK2SPDir}/game/AI_Jedi.cpp"
	"${JK2SPDir}/game/AI_Mark1.cpp"
	"${JK2SPDir}/game/AI_Mark2.cpp"
	"${JK2SPDir}/game/AI_MineMonster.cpp"
	"${JK2SPDir}/game/AI_Remote.cpp"
	"${JK2SPDir}/game/AI_Seeker.cpp"
	"${JK2SPDir}/game/AI_Sentry.cpp"
	"${JK2SPDir}/game/AI_Sniper.cpp"
	"${JK2SPDir}/game/AI_Stormtrooper.cpp"
	"${JK2SPDir}/game/AI_Utils.cpp"
	"${JK2SPDir}/game/G_Timer.cpp"
	"${JK2SPDir}/game/NPC.cpp"
	"${JK2SPDir}/game/NPC_behavior.cpp"
	"${JK2SPDir}/game/NPC_combat.cpp"
	"${JK2SPDir}/game/NPC_goal.cpp"
	"${JK2SPDir}/game/NPC_misc.cpp"
	"${JK2SPDir}/game/NPC_move.cpp"
	"${JK2SPDir}/game/NPC_reactions.cpp"
	"${JK2SPDir}/game/NPC_senses.cpp"
	"${JK2SPDir}/game/NPC_sounds.cpp"
	"${JK2SPDir}/game/NPC_spawn.cpp"
	"${JK2SPDir}/game/NPC_stats.cpp"
	"${JK2SPDir}/game/NPC_utils.cpp"
	"${JK2SPDir}/game/Q3_Interface.cpp"
	"${JK2SPDir}/game/Q3_Registers.cpp"
	"${JK2SPDir}/game/bg_misc.cpp"
	"${JK2SPDir}/game/bg_pangles.cpp"
	"${JK2SPDir}/game/bg_panimate.cpp"
	"${JK2SPDir}/game/bg_pmove.cpp"
	"${JK2SPDir}/game/bg_slidemove.cpp"
	"${JK2SPDir}/game/g_ICARUS.cpp"
	"${JK2SPDir}/game/g_active.cpp"
	"${JK2SPDir}/game/g_breakable.cpp"
	"${JK2SPDir}/game/g_camera.cpp"
	"${JK2SPDir}/game/g_client.cpp"
	"${JK2SPDir}/game/g_cmds.cpp"
	"${JK2SPDir}/game/g_combat.cpp"
	"${JK2SPDir}/game/g_functions.cpp"
	"${JK2SPDir}/game/g_fx.cpp"
	"${JK2SPDir}/game/g_inventory.cpp"
	"${JK2SPDir}/game/g_itemLoad.cpp"
	"${JK2SPDir}/game/g_items.cpp"
	"${JK2SPDir}/game/g_main.cpp"
	"${JK2SPDir}/game/g_mem.cpp"
	"${JK2SPDir}/game/g_misc.cpp"
	"${JK2SPDir}/game/g_misc_model.cpp"
	"${JK2SPDir}/game/g_missile.cpp"
	"${JK2SPDir}/game/g_mover.cpp"
	"${JK2SPDir}/game/g_nav.cpp"
	"${JK2SPDir}/game/g_navnew.cpp"
	"${JK2SPDir}/game/g_navigator.cpp"
	"${JK2SPDir}/game/g_object.cpp"
	"${JK2SPDir}/game/g_objectives.cpp"
	"${JK2SPDir}/game/g_ref.cpp"
	"${JK2SPDir}/game/g_roff.cpp"
	"${JK2SPDir}/game/g_savegame.cpp"
	"${JK2SPDir}/game/g_session.cpp"
	"${JK2SPDir}/game/g_spawn.cpp"
	"${JK2SPDir}/game/g_svcmds.cpp"
	"${JK2SPDir}/game/g_target.cpp"
	"${JK2SPDir}/game/g_trigger.cpp"
	"${JK2SPDir}/game/g_turret.cpp"
	"${JK2SPDir}/game/g_usable.cpp"
	"${JK2SPDir}/game/g_utils.cpp"
	"${JK2SPDir}/game/g_weapon.cpp"
	"${JK2SPDir}/game/g_weaponLoad.cpp"
	"${JK2SPDir}/game/genericparser2.cpp"
	"${JK2SPDir}/game/wp_atst.cpp"
	"${JK2SPDir}/game/wp_blaster_rifle.cpp"
	"${JK2SPDir}/game/wp_bot_laser.cpp"
	"${JK2SPDir}/game/wp_bowcaster.cpp"
	"${JK2SPDir}/game/wp_bryar_pistol.cpp"
	"${JK2SPDir}/game/wp_demp2.cpp"
	"${JK2SPDir}/game/wp_det_pack.cpp"
	"${JK2SPDir}/game/wp_disruptor.cpp"
	"${JK2SPDir}/game/wp_emplaced_gun.cpp"
	"${JK2SPDir}/game/wp_flechette.cpp"
	"${JK2SPDir}/game/wp_melee.cpp"
	"${JK2SPDir}/game/wp_repeater.cpp"
	"${JK2SPDir}/game/wp_rocket_launcher.cpp"
	"${JK2SPDir}/game/wp_saber.cpp"
	"${JK2SPDir}/game/wp_stun_baton.cpp"
	"${JK2SPDir}/game/wp_thermal.cpp"
	"${JK2SPDir}/game/wp_trip_mine.cpp"
	"${JK2SPDir}/game/Q3_Interface.h"
	"${JK2SPDir}/game/Q3_Registers.h"
	"${JK2SPDir}/game/ai.h"
	"${JK2SPDir}/game/anims.h"
	"${JK2SPDir}/game/b_local.h"
	"${JK2SPDir}/game/b_public.h"
	"${JK2SPDir}/game/bg_local.h"
	"${JK2SPDir}/game/bg_public.h"
	"${JK2SPDir}/game/bset.h"
	"${JK2SPDir}/game/bstate.h"
	"${JK2SPDir}/game/channels.h"
	"${JK2SPDir}/game/dmstates.h"
	"${JK2SPDir}/game/events.h"
	"${JK2SPDir}/game/fields.h"
	"${JK2SPDir}/game/g_functions.h"
	"${JK2SPDir}/game/g_headers.h"
	"${JK2SPDir}/game/g_icarus.h"
	"${JK2SPDir}/game/g_items.h"
	"${JK2SPDir}/game/g_local.h"
	"${JK2SPDir}/game/g_nav.h"
	"${JK2SPDir}/game/g_navigator.h"
	"${JK2SPDir}/game/g_public.h"
	"${JK2SPDir}/game/g_roff.h"
	"${JK2SPDir}/game/g_shared.h"
	"${JK2SPDir}/game/genericparser2.h"
	"${JK2SPDir}/game/ghoul2_shared.h"
	"${JK2SPDir}/game/npc_headers.h"
	"${JK2SPDir}/game/objectives.h"
	"${JK2SPDir}/game/say.h"
	"${JK2SPDir}/game/statindex.h"
	"${JK2SPDir}/game/surfaceflags.h"
	"${JK2SPDir}/game/teams.h"
	"${JK2SPDir}/game/weapons.h"
	"${JK2SPDir}/game/w_local.h"
	"${JK2SPDir}/game/wp_saber.h"
	)
source_group("game" FILES ${JK2SPGameGameFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameGameFiles})

set(JK2SPGameCGameFiles
	"${JK2SPDir}/cgame/FX_ATSTMain.cpp"
	"${JK2SPDir}/cgame/FX_Blaster.cpp"
	"${JK2SPDir}/cgame/FX_Bowcaster.cpp"
	"${JK2SPDir}/cgame/FX_BryarPistol.cpp"
	"${JK2SPDir}/cgame/FX_DEMP2.cpp"
	"${JK2SPDir}/cgame/FX_Disruptor.cpp"
	"${JK2SPDir}/cgame/FX_Emplaced.cpp"
	"${JK2SPDir}/cgame/FX_Flechette.cpp"
	"${JK2SPDir}/cgame/FX_HeavyRepeater.cpp"
	"${JK2SPDir}/cgame/FX_RocketLauncher.cpp"
	"${JK2SPDir}/cgame/FxPrimitives.cpp"
	"${JK2SPDir}/cgame/FxPrimitives.h"
	"${JK2SPDir}/cgame/FxScheduler.cpp"
	"${JK2SPDir}/cgame/FxScheduler.h"
	"${JK2SPDir}/cgame/FxSystem.cpp"
	"${JK2SPDir}/cgame/FxSystem.h"
	"${JK2SPDir}/cgame/FxTemplate.cpp"
	"${JK2SPDir}/cgame/FxUtil.cpp"
	"${JK2SPDir}/cgame/FxUtil.h"
	"${JK2SPDir}/cgame/animtable.h"
	"${JK2SPDir}/cgame/cg_camera.cpp"
	"${JK2SPDir}/cgame/cg_camera.h"
	"${JK2SPDir}/cgame/cg_consolecmds.cpp"
	"${JK2SPDir}/cgame/cg_credits.cpp"
	"${JK2SPDir}/cgame/cg_draw.cpp"
	"${JK2SPDir}/cgame/cg_drawtools.cpp"
	"${JK2SPDir}/cgame/cg_effects.cpp"
	"${JK2SPDir}/cgame/cg_ents.cpp"
	"${JK2SPDir}/cgame/cg_event.cpp"
	"${JK2SPDir}/cgame/cg_info.cpp"
	"${JK2SPDir}/cgame/cg_lights.cpp"
	"${JK2SPDir}/cgame/cg_local.h"
	"${JK2SPDir}/cgame/cg_localents.cpp"
	"${JK2SPDir}/cgame/cg_main.cpp"
	"${JK2SPDir}/cgame/cg_marks.cpp"
	"${JK2SPDir}/cgame/cg_media.h"
	"${JK2SPDir}/cgame/cg_players.cpp"
	"${JK2SPDir}/cgame/cg_playerstate.cpp"
	"${JK2SPDir}/cgame/cg_predict.cpp"
	"${JK2SPDir}/cgame/cg_public.h"
	"${JK2SPDir}/cgame/cg_scoreboard.cpp"
	"${JK2SPDir}/cgame/cg_servercmds.cpp"
	"${JK2SPDir}/cgame/cg_snapshot.cpp"
	"${JK2SPDir}/cgame/cg_syscalls.cpp"
	"${JK2SPDir}/cgame/cg_text.cpp"
	"${JK2SPDir}/cgame/cg_view.cpp"
	"${JK2SPDir}/cgame/cg_weapons.cpp"
	"${JK2SPDir}/cgame/strip_objectives.h"
	)
source_group("cgame" FILES ${JK2SPGameCGameFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameCGameFiles})

set(JK2SPGameUIFiles
	"${SPDir}/ui/gameinfo.cpp"
	"${SPDir}/ui/gameinfo.h"
	)
source_group("ui" FILES ${JK2SPGameUIFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameUIFiles})

set(JK2SPGameCommonFiles
	#We only have JKA qcommon files now.
	"${SPDir}/qcommon/tri_coll_test.cpp"
	"${SPDir}/qcommon/q_shared.cpp"
	"${SPDir}/qcommon/q_shared.h"
	"${SPDir}/qcommon/strippublic.h"
	"${SPDir}/qcommon/ojk_i_saved_game.h"
	"${SPDir}/qcommon/ojk_saved_game_class_archivers.h"
	"${SPDir}/qcommon/ojk_saved_game_helper.h"
	"${SPDir}/qcommon/ojk_saved_game_helper_fwd.h"
	"${SPDir}/qcommon/ojk_scope_guard.h"
	${SharedCommonFiles}
	)
source_group("common" FILES ${JK2SPGameCommonFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameCommonFiles})

set(JK2SPGameCommonSafeFiles
	${SharedCommonSafeFiles}
	"${SharedDir}/qcommon/safe/files.h"
	"${SharedDir}/qcommon/safe/files.cpp"
	"${JK2SPDir}/qcommon/safe/memory.h"
	)
source_group("common/safe" FILES ${JK2SPGameCommonSafeFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameCommonSafeFiles})

set(JK2SPGameIcarusFiles
	"${JK2SPDir}/icarus/BlockStream.cpp"
	"${JK2SPDir}/icarus/Sequence.cpp"
	"${JK2SPDir}/icarus/Sequencer.cpp"
	"${JK2SPDir}/icarus/TaskManager.cpp"
	"${JK2SPDir}/icarus/Instance.cpp"
	"${JK2SPDir}/icarus/blockstream.h"
	"${JK2SPDir}/icarus/icarus.h"
	"${JK2SPDir}/icarus/interface.h"
	"${JK2SPDir}/icarus/interpreter.h"
	"${JK2SPDir}/icarus/sequence.h"
	"${JK2SPDir}/icarus/sequencer.h"
	"${JK2SPDir}/icarus/taskmanager.h"
	"${JK2SPDir}/icarus/instance.h"
	"${JK2SPDir}/icarus/tokenizer.h"
	)
source_group("icarus" FILES ${JK2SPGameIcarusFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameIcarusFiles})

set(JK2SPGameRuflFiles
	"${SPDir}/Rufl/hstring.cpp"
	"${SPDir}/Rufl/hstring.h"
	)
source_group("rufl" FILES ${JK2SPGameRuflFiles})
set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameRuflFiles})

if(WIN32)
	set(JK2SPGameWin32Files
		"${JK2SPDir}/win32/game.rc")
	source_group("win32" FILES ${JK2SPGameWin32Files})
	set(JK2SPGameFiles ${JK2SPGameFiles} ${JK2SPGameWin32Files})
endif(WIN32)

add_library(${JK2SPGame} SHARED ${JK2SPGameFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${JK2SPGame} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${JK2SPGame} RUNTIME DESTINATION "${JK2InstallDir}/OpenJK")
else(WIN32)
	if(MakeApplicationBundles AND BuildJK2SPEngine)
		install(TARGETS ${JK2SPGame} LIBRARY DESTINATION "${JK2InstallDir}/${JK2SPEngine}.app/Contents/MacOS/OpenJK")
	else(MakeApplicationBundles AND BuildJK2SPEngine)
		install(TARGETS ${JK2SPGame} LIBRARY DESTINATION "${JK2InstallDir}/OpenJK")
	endif(MakeApplicationBundles AND BuildJK2SPEngine)
endif(WIN32)

set_target_properties(${JK2SPGame} PROPERTIES COMPILE_DEFINITIONS "${JK2SPGameDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${JK2SPGame} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${JK2SPGame} PROPERTIES INCLUDE_DIRECTORIES "${JK2SPGameIncludeDirectories}")
set_target_properties(${JK2SPGame} PROPERTIES PROJECT_LABEL "JK2 SP Game Library")
# no libraries used
if(JK2SPGameLibraries)
	target_link_libraries(${JK2SPGame} ${JK2SPGameLibraries})
endif(JK2SPGameLibraries)
