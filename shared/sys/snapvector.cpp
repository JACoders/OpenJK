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

#if _MSC_VER
static inline float roundfloat(float n)
{
	return (n < 0.0f) ? ceilf(n - 0.5f) : floorf(n + 0.5f);
}
#endif

extern "C"
void Sys_SnapVector(float *v)
{
#if _MSC_VER
	unsigned int oldcontrol;
	unsigned int newcontrol;

	_controlfp_s(&oldcontrol, 0, 0);
	_controlfp_s(&newcontrol, _RC_NEAR, _MCW_RC);

	v[0] = roundfloat(v[0]);
	v[1] = roundfloat(v[1]);
	v[2] = roundfloat(v[2]);

	_controlfp_s(&newcontrol, oldcontrol, _MCW_RC);
#else
	// pure C99
	int oldround = fegetround();
	fesetround(FE_TONEAREST);

	v[0] = nearbyintf(v[0]);
	v[1] = nearbyintf(v[1]);
	v[2] = nearbyintf(v[2]);

	fesetround(oldround);
#endif
}
