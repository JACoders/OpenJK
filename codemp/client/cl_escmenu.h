/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cl_escmenu.h -- the tools this client adds, listed once: the Esc menus (cl_escmenu.cpp) and the
// classic in-game menu's TOOLS panel (cl_rendermenu.cpp) both show them

#pragma once

typedef struct {
	const char	*name;
	const char	*command;
	const char	*desc;
} escTool_t;

extern const escTool_t	escTools[];
extern const int		escNumTools;
