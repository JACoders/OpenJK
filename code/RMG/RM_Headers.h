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
#if !defined(RM_HEADERS_H_INC)
#define RM_HEADERS_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Headers.h")
#endif

#pragma warning (push, 3)
#include <vector>
#include <list>
#pragma warning (pop)

using namespace std;

#if !defined(GENERICPARSER2_H_INC)
#include "../game/genericparser2.h"
#endif

#if !defined(CM_LOCAL_H_INC)
#include "../qcommon/cm_local.h"
#endif

#define MAX_INSTANCE_TRIES			5

// on a symmetric map which corner is the first node
typedef enum
{
	SYMMETRY_NONE,
	SYMMETRY_TOPLEFT,
	SYMMETRY_BOTTOMRIGHT

} symmetry_t;

#if !defined(CM_TERRAINMAP_H_INC)
	#include "../qcommon/cm_terrainmap.h"
#endif

#if !defined(RM_AREA_H_INC)
	#include "RM_Area.h"
#endif

#if !defined(RM_PATH_H_INC)
	#include "RM_Path.h"
#endif

#if !defined(RM_OBJECTIVE_H_INC)
	#include "RM_Objective.h"
#endif

#if !defined(RM_INSTANCEFILE_H_INC)
	#include "RM_InstanceFile.h"
#endif

#if !defined(RM_INSTANCE_H_INC)
	#include "RM_Instance.h"
#endif

#if !defined(RM_MISSION_H_INC)
	#include "RM_Mission.h"
#endif

#if !defined(RM_MANAGER_H_INC)
	#include "RM_Manager.h"
#endif

#if !defined(RM_TERRAIN_H_INC)
	#include "RM_Terrain.h"
#endif

#endif
