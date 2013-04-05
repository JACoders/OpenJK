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
