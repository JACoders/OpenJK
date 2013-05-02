#pragma once

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Headers.h")
#endif

#pragma warning (push, 3)
#include <vector>
#include <list>
#pragma warning (pop)

using namespace std;

#include "qcommon/GenericParser2.h"
#include "qcommon/cm_local.h"
#include "client/client.h"

#define MAX_INSTANCE_TRIES			5

// on a symmetric map which corner is the first node
typedef enum
{
	SYMMETRY_NONE,
	SYMMETRY_TOPLEFT,
	SYMMETRY_BOTTOMRIGHT

} symmetry_t;

#include "qcommon/cm_terrainmap.h"
#include "RM_Area.h"
#include "RM_Path.h"
#include "RM_Objective.h"
#include "RM_InstanceFile.h"
#include "RM_Instance.h"
#include "RM_Mission.h"
#include "RM_Manager.h"
#include "RM_Terrain.h"
