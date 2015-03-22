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

/*****************************************************************************
 * name:		be_aas_file.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_file.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#pragma once

#ifdef AASINTERN
//loads the AAS file with the given name
int AAS_LoadAASFile(char *filename);
//writes an AAS file with the given name
qboolean AAS_WriteAASFile(char *filename);
//dumps the loaded AAS data
void AAS_DumpAASData(void);
//print AAS file information
void AAS_FileInfo(void);
#endif //AASINTERN
