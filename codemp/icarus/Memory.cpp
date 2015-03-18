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

#include "icarus.h"

// leave these two as standard mallocs for the moment, there's something weird happening in ICARUS...
//
void *ICARUS_Malloc(int iSize)
{
	//return gi.Malloc(iSize, TAG_ICARUS);
	//return malloc(iSize);
	return Z_Malloc(iSize, TAG_ICARUS5, qfalse);
}

void ICARUS_Free(void *pMem)
{
	//gi.Free(pMem);
	//free(pMem);
	Z_Free(pMem);
}
