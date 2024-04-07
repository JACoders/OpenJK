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

set(MPUIIncludeDirectories
	"${MPDir}"
	"${SharedDir}"
	"${GSLIncludeDirectory}"
	)
if(WIN32)
	set(MPUILibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPUIDefines ${MPSharedDefines} "UI_BUILD")

set(MPUIGameFiles
	"${MPDir}/game/bg_misc.c"
	"${MPDir}/game/bg_saberLoad.c"
	"${MPDir}/game/bg_saga.c"
	"${MPDir}/game/bg_vehicleLoad.c"
	"${MPDir}/game/bg_weapons.c"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	)
source_group("game" FILES ${MPUIGameFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIGameFiles})

set(MPUICommonFiles
	"${MPDir}/qcommon/q_shared.c"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/game_version.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"

	${SharedCommonFiles}
	)
source_group("common" FILES ${MPUICommonFiles})
set(MPUIFiles ${MPUIFiles} ${MPUICommonFiles})

set(MPUICommonSafeFiles
	${SharedCommonSafeFiles}
	)
source_group("common/safe" FILES ${MPUICommonSafeFiles})
set(MPUIFiles ${MPUIFiles} ${MPUICommonSafeFiles})

set(MPUIUIFiles
	"${MPDir}/ui/ui_atoms.c"
	"${MPDir}/ui/ui_cvar.c"
	"${MPDir}/ui/ui_force.c"
	"${MPDir}/ui/ui_gameinfo.c"
	"${MPDir}/ui/ui_main.c"
	"${MPDir}/ui/ui_saber.c"
	"${MPDir}/ui/ui_shared.c"
	"${MPDir}/ui/ui_syscalls.c"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_force.h"
	"${MPDir}/ui/ui_local.h"
	"${MPDir}/ui/ui_public.h"
	"${MPDir}/ui/ui_shared.h"
	"${MPDir}/ui/ui_xcvar.h"
	)
source_group("ui" FILES ${MPUIUIFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIUIFiles})

set(MPUIRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPUIRendererFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIRendererFiles})

add_library(${MPUI} SHARED ${MPUIFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPUI} PROPERTIES PREFIX "")
endif()

if(MakeApplicationBundles AND BuildMPEngine)
	install(TARGETS ${MPUI}
		LIBRARY
		DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/OpenJK"
		COMPONENT ${JKAMPCoreComponent})
	install(TARGETS ${MPUI}
		LIBRARY
		DESTINATION "${JKAInstallDir}/${MPEngine}.app/Contents/MacOS/base"
		COMPONENT ${JKAMPCoreComponent})
elseif(WIN32)
	install(TARGETS ${MPUI}
		RUNTIME
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKAMPCoreComponent})

	# Use OpenJK modules as default
	install(TARGETS ${MPUI}
		RUNTIME
		DESTINATION "${JKAInstallDir}/base"
		COMPONENT ${JKAMPCoreComponent})
else()
	install(TARGETS ${MPUI}
		LIBRARY
		DESTINATION "${JKAInstallDir}/OpenJK"
		COMPONENT ${JKAMPCoreComponent})
	install(TARGETS ${MPUI}
		LIBRARY
		DESTINATION "${JKAInstallDir}/base"
		COMPONENT ${JKAMPCoreComponent})
endif()

set_target_properties(${MPUI} PROPERTIES COMPILE_DEFINITIONS "${MPUIDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${MPUI} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

set_target_properties(${MPUI} PROPERTIES INCLUDE_DIRECTORIES "${MPUIIncludeDirectories}")
set_target_properties(${MPUI} PROPERTIES PROJECT_LABEL "MP UI Library")
# no libraries used
if(MPUILibraries)
	target_link_libraries(${MPUI} ${MPUILibraries})
endif(MPUILibraries)
