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

set(SPGameDefines ${SharedDefines})
set(SPGameIncludeDirectories
	${SharedDir}
	${SPDir}
	"${SPDir}/game"
	)

if(WIN32)
	set(SPGameLibraries "winmm")
endif(WIN32)

set(SPGameGameFiles
	"${SPDir}/game/AI_Animal.cpp"
	"${SPDir}/game/AI_AssassinDroid.cpp"
	"${SPDir}/game/AI_Atst.cpp"
	"${SPDir}/game/AI_BobaFett.cpp"
	"${SPDir}/game/AI_Civilian.cpp"
	"${SPDir}/game/AI_Default.cpp"
	"${SPDir}/game/AI_Droid.cpp"
	"${SPDir}/game/AI_GalakMech.cpp"
	"${SPDir}/game/AI_Grenadier.cpp"
	"${SPDir}/game/AI_HazardTrooper.cpp"
	"${SPDir}/game/AI_Howler.cpp"
	"${SPDir}/game/AI_ImperialProbe.cpp"
	"${SPDir}/game/AI_Interrogator.cpp"
	"${SPDir}/game/AI_Jedi.cpp"
	"${SPDir}/game/AI_Mark1.cpp"
	"${SPDir}/game/AI_Mark2.cpp"
	"${SPDir}/game/AI_MineMonster.cpp"
	"${SPDir}/game/AI_Rancor.cpp"
	"${SPDir}/game/AI_Remote.cpp"
	"${SPDir}/game/AI_RocketTrooper.cpp"
	"${SPDir}/game/AI_SaberDroid.cpp"
	"${SPDir}/game/AI_SandCreature.cpp"
	"${SPDir}/game/AI_Seeker.cpp"
	"${SPDir}/game/AI_Sentry.cpp"
	"${SPDir}/game/AI_Sniper.cpp"
	"${SPDir}/game/AI_Stormtrooper.cpp"
	"${SPDir}/game/AI_Tusken.cpp"
	"${SPDir}/game/AI_Utils.cpp"
	"${SPDir}/game/AI_Wampa.cpp"
	"${SPDir}/game/g_active.cpp"
	"${SPDir}/game/g_breakable.cpp"
	"${SPDir}/game/g_camera.cpp"
	"${SPDir}/game/g_client.cpp"
	"${SPDir}/game/g_cmds.cpp"
	"${SPDir}/game/g_combat.cpp"
	"${SPDir}/game/g_emplaced.cpp"
	"${SPDir}/game/g_functions.cpp"
	"${SPDir}/game/g_fx.cpp"
	"${SPDir}/game/g_inventory.cpp"
	"${SPDir}/game/g_itemLoad.cpp"
	"${SPDir}/game/g_items.cpp"
	"${SPDir}/game/g_main.cpp"
	"${SPDir}/game/g_mem.cpp"
	"${SPDir}/game/g_misc.cpp"
	"${SPDir}/game/g_misc_model.cpp"
	"${SPDir}/game/g_missile.cpp"
	"${SPDir}/game/g_mover.cpp"
	"${SPDir}/game/g_nav.cpp"
	"${SPDir}/game/g_navigator.cpp"
	"${SPDir}/game/g_navnew.cpp"
	"${SPDir}/game/g_object.cpp"
	"${SPDir}/game/g_objectives.cpp"
	"${SPDir}/game/g_rail.cpp"
	"${SPDir}/game/g_ref.cpp"
	"${SPDir}/game/g_roff.cpp"
	"${SPDir}/game/g_savegame.cpp"
	"${SPDir}/game/g_session.cpp"
	"${SPDir}/game/g_spawn.cpp"
	"${SPDir}/game/g_svcmds.cpp"
	"${SPDir}/game/g_target.cpp"
	"${SPDir}/game/G_Timer.cpp"
	"${SPDir}/game/g_trigger.cpp"
	"${SPDir}/game/g_turret.cpp"
	"${SPDir}/game/g_usable.cpp"
	"${SPDir}/game/g_utils.cpp"
	"${SPDir}/game/g_weapon.cpp"
	"${SPDir}/game/g_weaponLoad.cpp"
	"${SPDir}/game/genericparser2.cpp"
	"${SPDir}/game/NPC.cpp"
	"${SPDir}/game/NPC_behavior.cpp"
	"${SPDir}/game/NPC_combat.cpp"
	"${SPDir}/game/NPC_goal.cpp"
	"${SPDir}/game/NPC_misc.cpp"
	"${SPDir}/game/NPC_move.cpp"
	"${SPDir}/game/NPC_reactions.cpp"
	"${SPDir}/game/NPC_senses.cpp"
	"${SPDir}/game/NPC_sounds.cpp"
	"${SPDir}/game/NPC_spawn.cpp"
	"${SPDir}/game/NPC_stats.cpp"
	"${SPDir}/game/NPC_utils.cpp"
	"${SPDir}/game/Q3_Interface.h"
	"${SPDir}/game/wp_atst.cpp"
	"${SPDir}/game/wp_blaster_pistol.cpp"
	"${SPDir}/game/wp_blaster_rifle.cpp"
	"${SPDir}/game/wp_bot_laser.cpp"
	"${SPDir}/game/wp_bowcaster.cpp"
	"${SPDir}/game/wp_concussion.cpp"
	"${SPDir}/game/wp_demp2.cpp"
	"${SPDir}/game/wp_det_pack.cpp"
	"${SPDir}/game/wp_disruptor.cpp"
	"${SPDir}/game/wp_emplaced_gun.cpp"
	"${SPDir}/game/wp_flechette.cpp"
	"${SPDir}/game/wp_melee.cpp"
	"${SPDir}/game/wp_noghri_stick.cpp"
	"${SPDir}/game/wp_repeater.cpp"
	"${SPDir}/game/wp_rocket_launcher.cpp"
	"${SPDir}/game/wp_saber.cpp"
	"${SPDir}/game/wp_saberLoad.cpp"
	"${SPDir}/game/wp_stun_baton.cpp"
	"${SPDir}/game/wp_thermal.cpp"
	"${SPDir}/game/wp_trip_mine.cpp"
	"${SPDir}/game/wp_tusken.cpp"
	"${SPDir}/game/Q3_Interface.cpp"
	"${SPDir}/game/bg_misc.cpp"
	"${SPDir}/game/bg_pangles.cpp"
	"${SPDir}/game/bg_panimate.cpp"
	"${SPDir}/game/bg_pmove.cpp"
	"${SPDir}/game/bg_slidemove.cpp"
	"${SPDir}/game/AnimalNPC.cpp"
	"${SPDir}/game/bg_vehicleLoad.cpp"
	"${SPDir}/game/FighterNPC.cpp"
	"${SPDir}/game/g_vehicles.cpp"
	"${SPDir}/game/SpeederNPC.cpp"
	"${SPDir}/game/WalkerNPC.cpp"
	"${SPDir}/game/ai.h"
	"${SPDir}/game/anims.h"
	"${SPDir}/game/b_local.h"
	"${SPDir}/game/b_public.h"
	"${SPDir}/game/bg_local.h"
	"${SPDir}/game/bg_public.h"
	"${SPDir}/game/bset.h"
	"${SPDir}/game/bstate.h"
	"${SPDir}/game/channels.h"
	"${SPDir}/game/common_headers.h"
	"${SPDir}/game/dmstates.h"
	"${SPDir}/game/events.h"
	"${SPDir}/game/fields.h"
	"${SPDir}/game/g_functions.h"
	"${SPDir}/game/g_items.h"
	"${SPDir}/game/g_local.h"
	"${SPDir}/game/g_nav.h"
	"${SPDir}/game/g_navigator.h"
	"${SPDir}/game/g_public.h"
	"${SPDir}/game/g_roff.h"
	"${SPDir}/game/g_shared.h"
	"${SPDir}/game/genericparser2.h"
	"${SPDir}/game/ghoul2_shared.h"
	"${SPDir}/game/hitlocs.h"
	"${SPDir}/game/npc_headers.h"
	"${SPDir}/game/objectives.h"
	"${SPDir}/game/say.h"
	"${SPDir}/game/statindex.h"
	"${SPDir}/game/surfaceflags.h"
	"${SPDir}/game/teams.h"
	"${SPDir}/game/weapons.h"
	"${SPDir}/game/w_local.h"
	"${SPDir}/game/wp_saber.h"
	"${SPDir}/game/g_vehicles.h"
	)
source_group("game" FILES ${SPGameGameFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameGameFiles})

set(SPGameCGameFiles
	"${SPDir}/cgame/cg_camera.cpp"
	"${SPDir}/cgame/cg_consolecmds.cpp"
	"${SPDir}/cgame/cg_credits.cpp"
	"${SPDir}/cgame/cg_draw.cpp"
	"${SPDir}/cgame/cg_drawtools.cpp"
	"${SPDir}/cgame/cg_effects.cpp"
	"${SPDir}/cgame/cg_ents.cpp"
	"${SPDir}/cgame/cg_event.cpp"
	"${SPDir}/cgame/cg_headers.cpp"
	"${SPDir}/cgame/cg_info.cpp"
	"${SPDir}/cgame/cg_lights.cpp"
	"${SPDir}/cgame/cg_localents.cpp"
	"${SPDir}/cgame/cg_main.cpp"
	"${SPDir}/cgame/cg_marks.cpp"
	"${SPDir}/cgame/cg_players.cpp"
	"${SPDir}/cgame/cg_playerstate.cpp"
	"${SPDir}/cgame/cg_predict.cpp"
	"${SPDir}/cgame/cg_scoreboard.cpp"
	"${SPDir}/cgame/cg_servercmds.cpp"
	"${SPDir}/cgame/cg_snapshot.cpp"
	"${SPDir}/cgame/cg_syscalls.cpp"
	"${SPDir}/cgame/cg_text.cpp"
	"${SPDir}/cgame/cg_view.cpp"
	"${SPDir}/cgame/cg_weapons.cpp"
	"${SPDir}/cgame/FX_ATSTMain.cpp"
	"${SPDir}/cgame/FX_Blaster.cpp"
	"${SPDir}/cgame/FX_Bowcaster.cpp"
	"${SPDir}/cgame/FX_BryarPistol.cpp"
	"${SPDir}/cgame/FX_Concussion.cpp"
	"${SPDir}/cgame/FX_DEMP2.cpp"
	"${SPDir}/cgame/FX_Disruptor.cpp"
	"${SPDir}/cgame/FX_Emplaced.cpp"
	"${SPDir}/cgame/FX_Flechette.cpp"
	"${SPDir}/cgame/FX_HeavyRepeater.cpp"
	"${SPDir}/cgame/FX_NoghriShot.cpp"
	"${SPDir}/cgame/FX_RocketLauncher.cpp"
	"${SPDir}/cgame/FX_TuskenShot.cpp"
	"${SPDir}/cgame/FxPrimitives.cpp"
	"${SPDir}/cgame/FxScheduler.cpp"
	"${SPDir}/cgame/FxSystem.cpp"
	"${SPDir}/cgame/FxTemplate.cpp"
	"${SPDir}/cgame/FxUtil.cpp"
	"${SPDir}/cgame/cg_camera.h"
	"${SPDir}/cgame/animtable.h"
	"${SPDir}/cgame/cg_headers.h"
	"${SPDir}/cgame/cg_local.h"
	"${SPDir}/cgame/cg_media.h"
	"${SPDir}/cgame/cg_public.h"
	"${SPDir}/cgame/common_headers.h"
	"${SPDir}/cgame/FxPrimitives.h"
	"${SPDir}/cgame/FxScheduler.h"
	"${SPDir}/cgame/FxSystem.h"
	"${SPDir}/cgame/FxUtil.h"
	"${SPDir}/cgame/strip_objectives.h"
	)
source_group("cgame" FILES ${SPGameCGameFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameCGameFiles})

set(SPGameUIFiles
	"${SPDir}/ui/gameinfo.cpp"
	"${SPDir}/ui/gameinfo.h"
	)
source_group("ui" FILES ${SPGameUIFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameUIFiles})

set(SPGameCommonFiles
	"${SPDir}/qcommon/tri_coll_test.cpp"
	"${SPDir}/qcommon/sstring.h"
	"${SPDir}/qcommon/tags.h"
	"${SPDir}/qcommon/timing.h"
	"${SPDir}/qcommon/q_math.cpp"
	"${SPDir}/qcommon/q_shared.cpp"
	"${SPDir}/qcommon/q_shared.h"
	"${SPDir}/qcommon/q_platform.h"
	)
source_group("common" FILES ${SPGameCommonFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameCommonFiles})

set(SPGameIcarusFiles
	"${SPDir}/icarus/BlockStream.cpp"
	"${SPDir}/icarus/IcarusImplementation.cpp"
	"${SPDir}/icarus/Sequence.cpp"
	"${SPDir}/icarus/Sequencer.cpp"
	"${SPDir}/icarus/TaskManager.cpp"
	"${SPDir}/icarus/blockstream.h"
	"${SPDir}/icarus/IcarusImplementation.h"
	"${SPDir}/icarus/IcarusInterface.h"
	"${SPDir}/icarus/sequence.h"
	"${SPDir}/icarus/sequencer.h"
	"${SPDir}/icarus/StdAfx.h"
	"${SPDir}/icarus/taskmanager.h"
	)
source_group("icarus" FILES ${SPGameIcarusFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameIcarusFiles})

set(SPGameRatlFiles
	"${SPDir}/Ratl/ratl.cpp"
	"${SPDir}/Ratl/array_vs.h"
	"${SPDir}/Ratl/bits_vs.h"
	"${SPDir}/Ratl/grid_vs.h"
	"${SPDir}/Ratl/handle_pool_vs.h"
	"${SPDir}/Ratl/hash_pool_vs.h"
	"${SPDir}/Ratl/heap_vs.h"
	"${SPDir}/Ratl/list_vs.h"
	"${SPDir}/Ratl/map_vs.h"
	"${SPDir}/Ratl/pool_vs.h"
	"${SPDir}/Ratl/queue_vs.h"
	"${SPDir}/Ratl/ratl_common.h"
	"${SPDir}/Ratl/scheduler_vs.h"
	"${SPDir}/Ratl/stack_vs.h"
	"${SPDir}/Ratl/string_vs.h"
	"${SPDir}/Ratl/vector_vs.h"
	)
source_group("ratl" FILES ${SPGameRatlFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameRatlFiles})

set(SPGameRavlFiles
	"${SPDir}/Ravl/CBounds.cpp"
	"${SPDir}/Ravl/CVec.cpp"
	"${SPDir}/Ravl/CBounds.h"
	"${SPDir}/Ravl/CMatrix.h"
	"${SPDir}/Ravl/CVec.h"
	)
source_group("ravl" FILES ${SPGameRavlFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameRavlFiles})

set(SPGameRuflFiles
	"${SPDir}/Rufl/hfile.cpp"
	"${SPDir}/Rufl/hstring.cpp"
	"${SPDir}/Rufl/hfile.h"
	"${SPDir}/Rufl/hstring.h"
	)
source_group("rufl" FILES ${SPGameRuflFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameRuflFiles})

set(SPGameRaglFiles
	"${SPDir}/Ragl/graph_region.h"
	"${SPDir}/Ragl/graph_triangulate.h"
	"${SPDir}/Ragl/graph_vs.h"
	"${SPDir}/Ragl/kdtree_vs.h"
	"${SPDir}/Ragl/ragl_common.h"
	)
source_group("ragl" FILES ${SPGameRaglFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameRaglFiles})

set(SPGameG2Files
	"${SPDir}/ghoul2/G2.h"
	"${SPDir}/ghoul2/ghoul2_gore.h"
	)
source_group("ghoul2" FILES ${SPGameG2Files})
set(SPGameFiles ${SPGameFiles} ${SPGameG2Files})

set(SPGameRendererFiles
	"${SPDir}/rd-common/mdx_format.h"
	"${SPDir}/rd-common/tr_types.h"
	)
source_group("renderer" FILES ${SPGameRendererFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameRendererFiles})

set(SPGameClientFiles
	"${SPDir}/client/vmachine.h"
	)
source_group("client" FILES ${SPGameClientFiles})
set(SPGameFiles ${SPGameFiles} ${SPGameClientFiles})

if(WIN32)
	set(SPGameWin32Files
		"${SPDir}/win32/game.rc")
	source_group("win32" FILES ${SPGameWin32Files})
	set(SPGameFiles ${SPGameFiles} ${SPGameWin32Files})
endif(WIN32)

add_library(${SPGame} SHARED ${SPGameFiles})

# Hide symbols not explicitly marked public.
set_property(TARGET ${SPGame} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${SPGame} PROPERTIES PREFIX "")
endif()

if(WIN32)
	install(TARGETS ${SPGame}
		RUNTIME
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKASPClientComponent})
else(WIN32)
	if(MakeApplicationBundles AND BuildSPEngine)
		install(TARGETS ${SPGame}
			LIBRARY
			DESTINATION "${JKAInstallDir}/${SPEngine}.app/Contents/MacOS/OpenJK"
			COMPONENT ${JKASPClientComponent})
	else()
		install(TARGETS ${SPGame}
			LIBRARY
			DESTINATION "${JKAInstallDir}/OpenJK"
			COMPONENT ${JKASPClientComponent})
	endif() 
endif()

set_target_properties(${SPGame} PROPERTIES COMPILE_DEFINITIONS "${SPGameDefines}")
set_target_properties(${SPGame} PROPERTIES INCLUDE_DIRECTORIES "${SPGameIncludeDirectories}")
set_target_properties(${SPGame} PROPERTIES PROJECT_LABEL "SP Game Library")
# no libraries used
if(SPGameLibraries)
	target_link_libraries(${SPGame} ${SPGameLibraries})
endif(SPGameLibraries)
