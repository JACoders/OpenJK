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

set(MPCGameIncludeDirectories
	"${MPDir}"
	"${SharedDir}"
	"${GSLIncludeDirectory}"
	)
if(WIN32)
	set(MPCGameLibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPCGameDefines ${MPSharedDefines} "_CGAME" )
set(MPCGameGameFiles
	"${MPDir}/game/AnimalNPC.c"
	"${MPDir}/game/bg_g2_utils.c"
	"${MPDir}/game/bg_misc.c"
	"${MPDir}/game/bg_panimate.c"
	"${MPDir}/game/bg_pmove.c"
	"${MPDir}/game/bg_saber.c"
	"${MPDir}/game/bg_saberLoad.c"
	"${MPDir}/game/bg_saga.c"
	"${MPDir}/game/bg_slidemove.c"
	"${MPDir}/game/bg_vehicleLoad.c"
	"${MPDir}/game/bg_weapons.c"
	"${MPDir}/game/FighterNPC.c"
	"${MPDir}/game/SpeederNPC.c"
	"${MPDir}/game/WalkerNPC.c"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	)
source_group("game" FILES ${MPCGameGameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameGameFiles})

set(MPCGameCgameFiles
	"${MPDir}/cgame/cg_consolecmds.c"
	"${MPDir}/cgame/cg_cvar.c"
	"${MPDir}/cgame/cg_draw.c"
	"${MPDir}/cgame/cg_drawtools.c"
	"${MPDir}/cgame/cg_effects.c"
	"${MPDir}/cgame/cg_ents.c"
	"${MPDir}/cgame/cg_event.c"
	"${MPDir}/cgame/cg_info.c"
	"${MPDir}/cgame/cg_light.c"
	"${MPDir}/cgame/cg_localents.c"
	"${MPDir}/cgame/cg_main.c"
	"${MPDir}/cgame/cg_marks.c"
	"${MPDir}/cgame/cg_newDraw.c"
	"${MPDir}/cgame/cg_players.c"
	"${MPDir}/cgame/cg_playerstate.c"
	"${MPDir}/cgame/cg_predict.c"
	"${MPDir}/cgame/cg_saga.c"
	"${MPDir}/cgame/cg_scoreboard.c"
	"${MPDir}/cgame/cg_servercmds.c"
	"${MPDir}/cgame/cg_snapshot.c"
	"${MPDir}/cgame/cg_spawn.c"
	"${MPDir}/cgame/cg_syscalls.c"
	"${MPDir}/cgame/cg_turret.c"
	"${MPDir}/cgame/cg_view.c"
	"${MPDir}/cgame/cg_weaponinit.c"
	"${MPDir}/cgame/cg_weapons.c"
	"${MPDir}/cgame/fx_blaster.c"
	"${MPDir}/cgame/fx_bowcaster.c"
	"${MPDir}/cgame/fx_bryarpistol.c"
	"${MPDir}/cgame/fx_demp2.c"
	"${MPDir}/cgame/fx_disruptor.c"
	"${MPDir}/cgame/fx_flechette.c"
	"${MPDir}/cgame/fx_force.c"
	"${MPDir}/cgame/fx_heavyrepeater.c"
	"${MPDir}/cgame/fx_rocketlauncher.c"
	"${MPDir}/cgame/animtable.h"
	"${MPDir}/cgame/cg_local.h"
	"${MPDir}/cgame/cg_public.h"
	"${MPDir}/cgame/cg_xcvar.h"
	"${MPDir}/cgame/fx_local.h"
	)
source_group("cgame" FILES ${MPCGameCgameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCgameFiles})

set(MPCGameCommonFiles
	"${MPDir}/qcommon/q_shared.c"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"

	${SharedCommonFiles}
	)
source_group("common" FILES ${MPCGameCommonFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCommonFiles})

set(MPCGameCommonSafeFiles
	${SharedCommonSafeFiles}
	)
source_group("common/safe" FILES ${MPCGameCommonSafeFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCommonSafeFiles})

set(MPCGameUiFiles
	"${MPDir}/ui/ui_shared.c"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_shared.h"
	)
source_group("ui" FILES ${MPCGameUiFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameUiFiles})

set(MPCGameRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPCGameRendererFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameRendererFiles})

set(MPCGameGFiles
	"${MPDir}/ghoul2/G2.h"
	)
source_group("ghoul2" FILES ${MPCGameG2Files})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameG2Files})

add_library(${MPCGame} SHARED ${MPCGameFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPCGame} PROPERTIES PREFIX "")
endif()

if(MakeApplicationBundles AND BuildMPEngine)
	install(TARGETS ${MPCGame}
		LIBRARY
		DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/OpenJK"
		COMPONENT ${JKAMPCoreComponent})
	install(TARGETS ${MPCGame}
		LIBRARY
		DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/base"
		COMPONENT ${JKAMPCoreComponent})
elseif(WIN32)
	install(TARGETS ${MPCGame}
		RUNTIME
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKAMPCoreComponent})

	# Use OpenJK modules as default
	install(TARGETS ${MPCGame}
		RUNTIME
		DESTINATION "${JKAInstallDir}/base"
		COMPONENT ${JKAMPCoreComponent})
else()
	install(TARGETS ${MPCGame}
		LIBRARY
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKAMPCoreComponent})
	install(TARGETS ${MPCGame}
		LIBRARY
		DESTINATION "${JKAInstallDir}/base"
		COMPONENT ${JKAMPCoreComponent})
endif()

set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS "${MPCGameDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${MPCGame} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${MPCGame} PROPERTIES INCLUDE_DIRECTORIES "${MPCGameIncludeDirectories}")
set_target_properties(${MPCGame} PROPERTIES PROJECT_LABEL "MP Client Game Library")
# no libraries used
if(MPCGameLibraries)
	target_link_libraries(${MPCGame} ${MPCGameLibraries})
endif(MPCGameLibraries)
