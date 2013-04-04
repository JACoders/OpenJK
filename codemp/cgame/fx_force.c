// Any dedicated force oriented effects

#include "cg_local.h"

/*
-------------------------
FX_ForceDrained
-------------------------
*/
// This effect is not generic because of possible enhancements
void FX_ForceDrained(vec3_t origin, vec3_t dir)
{
	VectorScale(dir, -1.0, dir);
	trap_FX_PlayEffectID(cgs.effects.forceDrained, origin, dir, -1, -1);
}
	
