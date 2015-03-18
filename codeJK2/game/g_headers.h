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
#if !defined(G_HEADERS_H_INC)
#define G_HEADERS_H_INC

#if !defined(G_LOCAL_H_INC)
	#include "../game/g_local.h"
#endif

//#if !defined(G_WRAITH_H_INC)
//	#include "../game/g_Wraith.h"
//#endif

#if !defined(TEAMS_H_INC)
	#include "../game/teams.h"
#endif

//#if !defined(IGINTERFACE_H_INC)
//	#include "../game/IGInterface.h"
//#endif

#include "../game/b_local.h"
#include "../game/g_functions.h"
#include "../game/g_nav.h"
#include "../game/g_navigator.h"
#include "../cgame/cg_camera.h"	// Just for AI_Rancor
#include "../cgame/cg_local.h"	// Evil? Maybe. Necessary? Absolutely.

#endif // G_HEADERS_H_INC
