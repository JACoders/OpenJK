#============================================================================
# Copyright (C) 2015, OpenJK contributors
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

# Subdirectory to package JKA into
set(JKAInstallDir "JediAcademy")

# Install components
set(JKAMPCoreComponent "JKAMPCore")
set(JKAMPServerComponent "JKAMPServer")
set(JKAMPClientComponent "JKAMPClient")

# Component display names
include(CPackComponent)

set(CPACK_COMPONENT_JKAMPCORE_DISPLAY_NAME "Core")
set(CPACK_COMPONENT_JKAMPCORE_REQUIRED TRUE)
set(CPACK_COMPONENT_JKAMPCORE_DESCRIPTION "Core files shared by the multiplayer client and server executables.")
set(CPACK_COMPONENT_JKAMPCLIENT_DISPLAY_NAME "Client")
set(CPACK_COMPONENT_JKAMPCLIENT_DESCRIPTION "Files required to play the multiplayer game.")
set(CPACK_COMPONENT_JKAMPSERVER_DISPLAY_NAME "Server")
set(CPACK_COMPONENT_JKAMPSERVER_DESCRIPTION "Files required to run a Jedi Academy server.")
set(CPACK_COMPONENTS_ALL
	${JKAMPCoreComponent}
	${JKAMPClientComponent}
	${JKAMPServerComponent})

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# Component groups
set(CPACK_COMPONENT_JKAMPCORE_GROUP "JKAMP")
set(CPACK_COMPONENT_JKAMPCLIENT_GROUP "JKAMP")
set(CPACK_COMPONENT_JKAMPSERVER_GROUP "JKAMP")

cpack_add_component_group(JKAMP
	DISPLAY_NAME "Jedi Academy Multiplayer"
	DESCRIPTION "Jedi Academy multiplayer game")

if(WIN32)
	include(CPackNSIS)
	set(CPACK_NSIS_DISPLAY_NAME "OpenJK")
	set(CPACK_NSIS_PACKAGE_NAME "OpenJK")
	set(CPACK_NSIS_MUI_ICON "${SharedDir}/icons/icon.ico")
	set(CPACK_NSIS_MUI_UNIICON "${SharedDir}/icons/icon.ico")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://openjk.org")

	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
	include(InstallRequiredSystemLibraries)

	if(BuildMPEngine)
		string(REPLACE "/" "\\\\" ICON "${MPDir}/win32/icon.ico")
		set(CPACK_NSIS_CREATE_ICONS_EXTRA
			"${CPACK_NSIS_CREATE_ICONS_EXTRA}
			CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Jedi Academy MP.lnk' \\\\
				'$INSTDIR\\\\${MPEngine}.exe' \\\\
				'' \\\\
				'${ICON}'")

		set(CPACK_NSIS_DELETE_ICONS_EXTRA
			"${CPACK_NSIS_DELETE_ICONS_EXTRA}
			Delete '$SMPROGRAMS\\\\$MUI_TEMP\\\\Jedi Academy MP.lnk'")

		install(FILES ${MPDir}/OpenAL32.dll ${MPDir}/EaxMan.dll
				DESTINATION ${JKAInstallDir}
				COMPONENT ${JKAMPClientComponent})

		install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
				DESTINATION ${JKAInstallDir}
				COMPONENT ${JKAMPClientComponent})
	endif()
endif()

# CPack for installer creation
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_FILE_NAME "OpenJK-${CMAKE_SYSTEM_NAME}-${Architecture}")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An improved Jedi Academy")
set(CPACK_PACKAGE_VENDOR "JACoders")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenJK")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_DIRECTORY ${PACKAGE_DIR})
set(CPACK_BINARY_ZIP ON) # always create at least a zip file
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0) # prevent additional directory in zip

include(CPack)