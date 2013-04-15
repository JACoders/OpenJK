/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

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
	#include "../game/Teams.h"
#endif

//#if !defined(IGINTERFACE_H_INC)
//	#include "../game/IGInterface.h"
//#endif

// More stuff that we "need" on Xbox, as we don't use PCH
#ifdef _NO_PCH // (mrw) not sure how to make cmake use PCH, so they can be deactivated
#include "../game/b_local.h"
#include "../game/g_functions.h"
#include "../game/g_nav.h"
#include "../game/g_navigator.h"
#include "../game/g_vehicles.h"
#include "../cgame/cg_camera.h"	// Just for AI_Rancor
#include "../cgame/cg_local.h"	// Evil? Maybe. Necessary? Absolutely.
#endif

#endif // G_HEADERS_H_INC