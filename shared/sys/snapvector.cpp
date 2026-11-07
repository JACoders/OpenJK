/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include <math.h>
#if _MSC_VER
# include <float.h>
# pragma fenv_access (on)
#else
# include <fenv.h>
#endif

// To nearest with ties to even, through an integer (so -0.5 gives +0), on every platform and compiler: what 1.01's
// fld / fistp did. MSVC used to round ties away from zero while the others rounded them to even, so a Windows client
// and a Linux server could round the same velocity (pmove) differently.
extern "C"
void Sys_SnapVector(float *v)
{
#if _MSC_VER
	unsigned int oldcontrol;
	unsigned int newcontrol;

	_controlfp_s(&oldcontrol, 0, 0);
	_controlfp_s(&newcontrol, _RC_NEAR, _MCW_RC);

	v[0] = (float)lrintf(v[0]);
	v[1] = (float)lrintf(v[1]);
	v[2] = (float)lrintf(v[2]);

	_controlfp_s(&newcontrol, oldcontrol, _MCW_RC);
#else
	// pure C99
	int oldround = fegetround();
	fesetround(FE_TONEAREST);

	v[0] = (float)lrintf(v[0]);
	v[1] = (float)lrintf(v[1]);
	v[2] = (float)lrintf(v[2]);

	fesetround(oldround);
#endif
}
