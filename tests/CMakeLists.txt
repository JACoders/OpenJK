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

set(TestFiles
	"main.cpp"
	"safe/string.cpp"
	"safe/limited_vector.cpp"
	"${SharedDir}/qcommon/safe/string.cpp"
	)
if(MSVC)
	set(TestFiles
		${TestFiles}
		"${SharedDir}/openjk.natvis"
		)
endif()
source_group( "tests" REGULAR_EXPRESSION ".*")
source_group( "tests\\safe" REGULAR_EXPRESSION "safe/.*" )
source_group( "qcommon\\safe" REGULAR_EXPRESSION "${SharedDir}/qcommon/safe/.*" )

if(MSVC)
	set( Boost_USE_STATIC_LIBS ON )
endif()
find_package( Boost COMPONENTS unit_test_framework REQUIRED )

set(TestTarget "UnitTests")
set(TestLibraries "${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")
set(TestIncludeDirectories
	"${Boost_INCLUDE_DIRS}"
	"${SharedDir}"
	"${GSLIncludeDirectory}"
	)
set(TestDefines "${SharedDefines}")

add_executable(${TestTarget} ${TestFiles})
set_target_properties(${TestTarget} PROPERTIES COMPILE_DEFINITIONS "${TestDefines}")
set_target_properties(${TestTarget} PROPERTIES INCLUDE_DIRECTORIES "${TestIncludeDirectories}")
set_target_properties(${TestTarget} PROPERTIES PROJECT_LABEL "Unit Tests")
target_link_libraries(${TestTarget} ${TestLibraries})
install(TARGETS ${TestTarget} DESTINATION ".")

if(NOT MSVC)
	#Generate a main function
	target_compile_definitions(${TestTarget} PRIVATE BOOST_TEST_DYN_LINK)
endif()

add_test(NAME unittests COMMAND ${TestTarget})
