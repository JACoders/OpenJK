/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

#pragma once
#if !defined(COMMON_HEADERS_H_INC)
#define COMMON_HEADERS_H_INC

#if !defined(__Q_SHARED_H)
	#include "../qcommon/q_shared.h"
#endif

//#if !defined(Q_SHAREDBASIC_H_INC)
//	#include "../game/q_sharedbasic.h"			// fileHandle_t
//#endif

//#if !defined(Q_MATH_H_INC)
//	#include "../game/q_math.h"
//#endif

#define GAME_INCLUDE
#include "../game/b_local.h"
#include "../cgame/cg_local.h"
#include "../game/g_navigator.h"
#include "../game/g_shared.h"
#include "../game/g_functions.h"

#endif // COMMON_HEADERS_H_INC
