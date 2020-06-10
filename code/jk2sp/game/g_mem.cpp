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

//
// g_mem.c
//
// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"

#include "g_local.h"

/*#define POOLSIZE	(2 * 1024 * 1024)

static char		memoryPool[POOLSIZE];
*/
static int		allocPoint;
static cvar_t	*g_debugalloc;

void *G_Alloc( int size ) {
	if ( g_debugalloc->integer ) {
		gi.Printf( "G_Alloc of %i bytes\n", size );
	}


	allocPoint += size;
	
	return gi.Malloc(size, TAG_G_ALLOC, qfalse);
}

void G_InitMemory( void ) {
	allocPoint = 0;
	g_debugalloc = gi.cvar ("g_debugalloc", "0", 0);
}

void Svcmd_GameMem_f( void ) {
	gi.Printf( "Game memory status: %i allocated\n", allocPoint );
}
