// Precompiled header file for the client game

#include "cg_local.h"

// No PCH at all on Xbox build, we just include everything. Does this slow
// down builds? Somewhat. But then again, if I do change headers, I have to
// tolerate VS.net's piss poor dependency system that requires my to manually
// delete the .pch for the PC version to work at all. So, I'll live.
#ifdef _XBOX
	#include "../game/g_local.h"
	#include "../game/g_functions.h"
	#include "../game/b_local.h"
#endif

//#include "CGEntity.h"
//#include "../game/SpawnSystem.h"
//#include "../game/EntitySystem.h"
//#include "../game/CScheduleSystem.h"

// end
