/*
===========================================================================
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
	trap->FX_PlayEffectID(cgs.effects.forceDrained, origin, dir, -1, -1, qfalse);
}

