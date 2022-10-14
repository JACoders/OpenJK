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

#pragma once

#include "tr_local.h"

#ifdef _G2_GORE

#define MAX_LODS (8)
#define MAX_GORE_VERTS (3000)
#define MAX_GORE_INDECIES (6000)

//TODO: This needs to be set via a scalability cvar with some reasonable minimum value if pgore is used at all
#define MAX_GORE_RECORDS (500)

struct R2GoreTextureCoordinates
{
	srfG2GoreSurface_t *tex[MAX_LODS];

	R2GoreTextureCoordinates();
	~R2GoreTextureCoordinates();
};

int AllocR2GoreRecord();
R2GoreTextureCoordinates *FindR2GoreRecord(int tag);
void DeleteR2GoreRecord(int tag);

#endif
