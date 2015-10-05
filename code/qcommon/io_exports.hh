/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

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

// IO-related functions that the engine provides to plugin modules.

#ifndef OPENJK_IO_EXPORTS_HH
#define OPENJK_IO_EXPORTS_HH

// Cross-platform abstraction of function attributes.
#include "../../shared/sys/sys_attributes.h"

void Q_EXPORT Com_Printf(const char *fmt, ...);
void Q_EXPORT NORETURN Com_Error(int code, const char *fmt, ...);
void Q_EXPORT CL_RefPrintf(int print_level, const char *fmt, ...);

#endif
