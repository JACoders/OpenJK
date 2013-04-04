/*
** WIN_GAMMA.C
*/
// leave this as first line for PCH reasons...
//
//#include "../server/exe_headers.h"



#include <assert.h>
#include "../game/q_shared.h"
#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"

#if defined(_XBOX)
#include "glw_win_dx8.h"
#endif


/*
** WG_CheckHardwareGamma
**
** Determines if the underlying hardware supports the Win32 gamma correction API.
*/
void WG_CheckHardwareGamma( void )
{
	glConfig.deviceSupportsGamma = qtrue;
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( float g ) {
#if defined(_GAMECUBE)
	GXGamma gamma = GX_GM_1_0;
	if (g >= 2.2f)
	{
		gamma = GX_GM_2_2;
	}
	else if (g >= 1.7f)
	{
		gamma = GX_GM_1_7;
	}
	GXSetDispCopyGamma(gamma);
#elif defined(_XBOX)
	const int maxval = 255;

	D3DGAMMARAMP ramp;
	for ( int i = 0; i < 256; i++ )
	{
		int inf;
		if ( g == 1 ) {
			inf = maxval * i / 255.0f;
		} else {
			inf = maxval * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		if (inf < 0) {
			inf = 0;
		}
		if (inf > maxval) {
			inf = maxval;
		}
		ramp.red[i] = inf;
		ramp.green[i] = inf;
		ramp.blue[i] = inf;
	}
	glw_state->device->SetGammaRamp(D3DSGR_CALIBRATE, &ramp);
#endif
}

