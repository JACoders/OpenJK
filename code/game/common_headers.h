#pragma once
#if !defined(COMMON_HEADERS_H_INC)
#define COMMON_HEADERS_H_INC

#if !defined(__Q_SHARED_H)
	#include "../game/q_shared.h"
#endif

//#if !defined(Q_SHAREDBASIC_H_INC)
//	#include "../game/q_sharedbasic.h"			// fileHandle_t
//#endif

//#if !defined(Q_MATH_H_INC)
//	#include "../game/q_math.h"
//#endif

#ifdef _XBOX
#define GAME_INCLUDE
	#include "../game/b_local.h"
	#include "../cgame/cg_local.h"
	#include "../game/g_navigator.h"
	#include "../game/g_shared.h"
	#include "../game/g_functions.h"
#endif

#endif // COMMON_HEADERS_H_INC
