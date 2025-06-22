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

#ifndef __G_ICARUS_H__
#define __G_ICARUS_H__

//NOTENOTE: Only change this to re-point ICARUS to a new script directory
#define Q3_SCRIPT_DIR	"scripts"

//ICARUS includes
extern	interface_export_t	interface_export;

extern	void Interface_Init( interface_export_t *pe );
extern	int ICARUS_RunScript( gentity_t *ent, const char *name );
extern	bool ICARUS_RegisterScript( const char *name, bool bCalledDuringInterrogate = false);
extern ICARUS_Instance	*iICARUS;
extern bufferlist_t		ICARUS_BufferList;
extern entlist_t		ICARUS_EntList;
extern cvarlist_t       ICARUS_CvarList;

//
//	g_ICARUS.cpp
//
void ICARUS_Init( void );
bool ICARUS_ValidEnt( gentity_t *ent );
void ICARUS_InitEnt( gentity_t *ent );
void ICARUS_FreeEnt( gentity_t *ent );
void ICARUS_AssociateEnt( gentity_t *ent );
void ICARUS_Shutdown( void );
void Svcmd_ICARUS_f( void );

extern int		ICARUS_entFilter;

#endif//#ifndef __G_ICARUS_H__
