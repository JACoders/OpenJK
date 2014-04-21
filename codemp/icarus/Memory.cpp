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
