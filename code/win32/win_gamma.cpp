/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

/*
** WIN_GAMMA.C
*/
// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#include <assert.h>
#include "tr_local.h"
#include "../qcommon/qcommon.h"
#include "../win32/glw_win.h"
#include "../win32/win_local.h"

static unsigned short s_oldHardwareGamma[3][256];

/*
** WG_CheckHardwareGamma
**
** Determines if the underlying hardware supports the Win32 gamma correction API.
*/
void WG_CheckHardwareGamma( void )
{
	HDC			hDC;

	glConfig.deviceSupportsGamma = qfalse;

	if ( !r_ignorehwgamma->integer )
	{
		hDC = GetDC( GetDesktopWindow() );
		glConfig.deviceSupportsGamma = GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
		ReleaseDC( GetDesktopWindow(), hDC );

		if ( glConfig.deviceSupportsGamma )
		{
			//
			// do a sanity check on the gamma values
			//
			if ( ( HIBYTE( s_oldHardwareGamma[0][255] ) <= HIBYTE( s_oldHardwareGamma[0][0] ) ) ||
				 ( HIBYTE( s_oldHardwareGamma[1][255] ) <= HIBYTE( s_oldHardwareGamma[1][0] ) ) ||
				 ( HIBYTE( s_oldHardwareGamma[2][255] ) <= HIBYTE( s_oldHardwareGamma[2][0] ) ) )
			{
				glConfig.deviceSupportsGamma = qfalse;
				ri.Printf( PRINT_WARNING, "WARNING: device has broken gamma support, generated gamma.dat\n" );
			}

			//
			// make sure that we didn't have a prior crash in the game, and if so we need to
			// restore the gamma values to at least a linear value
			//
			if ( ( HIBYTE( s_oldHardwareGamma[0][181] ) == 255 ) )
			{
				int g;

				ri.Printf( PRINT_WARNING, "WARNING: suspicious gamma tables, using linear ramp for restoration\n" );

				for ( g = 0; g < 255; g++ )
				{
					s_oldHardwareGamma[0][g] = g << 8;
					s_oldHardwareGamma[1][g] = g << 8;
					s_oldHardwareGamma[2][g] = g << 8;
				}
			}
		}
	}
}

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
	unsigned short table[3][256];
	int		i, j;
	int		ret;
	OSVERSIONINFO	vinfo;

	if ( !glConfig.deviceSupportsGamma || r_ignorehwgamma->integer || !glw_state.hDC ) {
		return;
	}

//mapGammaMax();

	for ( i = 0; i < 256; i++ ) {
		table[0][i] = ( ( ( unsigned short ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( unsigned short ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( unsigned short ) blue[i] ) << 8 ) | blue[i];
	}

	// Win2K and newer puts this odd restriction on gamma ramps...
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);
	GetVersionEx( &vinfo );
	if ( vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
		Com_DPrintf( "performing gamma clamp.\n" );
		for ( j = 0 ; j < 3 ; j++ ) {
			for ( i = 0 ; i < 128 ; i++ ) {
				if ( table[j][i] > ( (128+i) << 8 ) ) {
					table[j][i] = (128+i) << 8;
				}
			}
			if ( table[j][127] > 254<<8 ) {
				table[j][127] = 254<<8;
			}
		}
	} else {
		Com_DPrintf( "skipping gamma clamp.\n" );
	}

	// enforce constantly increasing
	for ( j = 0 ; j < 3 ; j++ ) {
		for ( i = 1 ; i < 256 ; i++ ) {
			if ( table[j][i] < table[j][i-1] ) {
				table[j][i] = table[j][i-1];
			}
		}
	}

	ret = SetDeviceGammaRamp( glw_state.hDC, table );
	if ( !ret ) {
		Com_Printf( "SetDeviceGammaRamp failed.\n" );
	}
}

/*
** WG_RestoreGamma
*/
void WG_RestoreGamma( void )
{
	if ( glConfig.deviceSupportsGamma )
	{
		HDC hDC;
		
		hDC = GetDC( GetDesktopWindow() );
		SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
		ReleaseDC( GetDesktopWindow(), hDC );
	}
}

