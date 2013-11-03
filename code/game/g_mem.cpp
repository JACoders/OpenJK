/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

//
// g_mem.c
//

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
