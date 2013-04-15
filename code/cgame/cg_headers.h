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

// Precompiled header file for the client game

#include "cg_local.h"

// No PCH at all on Xbox build, we just include everything. Does this slow
// down builds? Somewhat. But then again, if I do change headers, I have to
// tolerate VS.net's piss poor dependency system that requires my to manually
// delete the .pch for the PC version to work at all. So, I'll live.
#ifdef _NO_PCH // (mrw) not sure how to make cmake use PCH, so they can be deactivated
#include "../game/g_local.h"
#include "../game/g_functions.h"
#include "../game/b_local.h"
#endif

//#include "CGEntity.h"
//#include "../game/SpawnSystem.h"
//#include "../game/EntitySystem.h"
//#include "../game/CScheduleSystem.h"

// end
