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
#ifdef _XBOX
	#include "../game/b_local.h"
	#include "../game/g_functions.h"
	#include "../game/g_nav.h"
	#include "../game/g_navigator.h"
	#include "../game/g_vehicles.h"
	#include "../cgame/cg_camera.h"	// Just for AI_Rancor
	#include "../cgame/cg_local.h"	// Evil? Maybe. Necessary? Absolutely.
#endif

#endif // G_HEADERS_H_INC