// Copyright (C) 1999-2000 Id Software, Inc.
//
// Simple linear memory allocator
// g_mem.c
//

#include "g_local.h"

/*
  The purpose of G_Alloc is to efficiently allocate memory for objects
  typically when the game is starting up. It shouldn't be used as a general
  purpose allocator that's used when the game is running, and especially not
  from a client command! It is *by design* that memory blocks can't be
  deallocated while the game is running!

  If your mod experiences the allocation failed error often, use g_debugAlloc
  to trace down where G_Alloc is being called and to make sure it isn't used
  often while the game is running. Feel free to increase POOLSIZE for your mod!

  More information about Linear Allocators:
  http://www.altdevblogaday.com/2011/02/12/alternatives-to-malloc-and-new/
*/

#define POOLSIZE	(256 * 1024)

static char		memoryPool[POOLSIZE];
static int		allocPoint;

void *G_Alloc( int size ) {
	char	*p;

	if ( size <= 0 ) {
		trap->Error( ERR_DROP, "G_Alloc: zero-size allocation\n", size );
		return NULL;
	}

	if ( g_debugAlloc.integer ) {
		trap->Print( "G_Alloc of %i bytes (%i left)\n", size, POOLSIZE - allocPoint - ( ( size + 31 ) & ~31 ) );
	}

	if ( allocPoint + size > POOLSIZE ) {
	  trap->Error( ERR_DROP, "G_Alloc: failed on allocation of %i bytes\n", size ); // bk010103 - was %u, but is signed
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

void G_InitMemory( void ) {
	allocPoint = 0;
}

void Svcmd_GameMem_f( void ) {
	trap->Print( "Game memory status: %i out of %i bytes allocated\n", allocPoint, POOLSIZE );
}
