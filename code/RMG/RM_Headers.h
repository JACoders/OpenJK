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
