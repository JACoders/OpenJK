// Filename:-	fffx_Feel.h	(Force-Feedback FX)
// ADDED BY IMMRESION

#ifndef FFFX_FEEL_H
#define FFFX_FEEL_H

#include "../../client/fffx.h"
#include "ffc.h"

/////////////////////////////////////////////////////////////////////////
/*  These functions were created to make the code a little easier to read.
 *  _FeelCreateEffect is quite long since it needs to create different
 *  kinds of effects.  When playing effects, the number of iterations
 *  may not act as expected.  I can't use CFeelCompound effects since I
 *  don't have a Project (which requires an ifr file at this point).  So,
 *  I simulate compound effects with arrays.  If an effect has multiple
 *  CFeelEffect in it, each CFeelEffect will be started individually with
 *  that number of iterations.  The only case where this will act strange
 *  is when the CFeelEffects have different durations.
*/
/////////////////////////////////////////////////////////////////////////
void _FeelInitEffects();
BOOL _FeelCreateEffect(int iSlotNum, ffFX_e fffx, CFeelDevice* pFeelDevice);
BOOL _FeelStartEffect(int iSlotNum, DWORD dwIterations, DWORD dwFlags);
BOOL _FeelEffectPlaying(int iSlotNum);
BOOL _FeelStopEffect(int iSlotNum);
BOOL _FeelClearEffect(int iSlotNum);

#endif	// #ifndef FFFX_FEEL_H
