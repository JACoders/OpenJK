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
cmake_minimum_required(VERSION 3.1)

add_executable(GetExeSymbolDir get_exe_dir.cpp)
target_link_libraries(GetExeSymbolDir Dbghelp)

add_executable(GetPdbSymbolDir get_pdb_dir.cpp)
target_link_libraries(GetPdbSymbolDir Dbghelp)
