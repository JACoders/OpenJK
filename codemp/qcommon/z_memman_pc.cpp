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

// Created 3/13/03 by Brian Osman (VV) - Split Zone/Hunk from common

#include "client/client.h" // hi i'm bad

////////////////////////////////////////////////
//
#ifdef TAGDEF	// itu?
#undef TAGDEF
#endif
#define TAGDEF(blah) #blah
const static char *psTagStrings[TAG_COUNT+1]=	// +1 because TAG_COUNT will itself become a string here. Oh well.
{
	#include "qcommon/tags.h"
};
//
////////////////////////////////////////////////

static void Z_Details_f(void);
void CIN_CloseAllVideos();


// This handles zone memory allocation.
// It is a wrapper around malloc with a tag id and a magic number at the start

#define ZONE_MAGIC			0x21436587

typedef struct zoneHeader_s
{
		int					iMagic;
		memtag_t			eTag;
		int					iSize;
struct	zoneHeader_s		*pNext;
struct	zoneHeader_s		*pPrev;
} zoneHeader_t;

typedef struct
{
	int iMagic;

} zoneTail_t;

static inline zoneTail_t *ZoneTailFromHeader(zoneHeader_t *pHeader)
{
	return (zoneTail_t*) ( (char*)pHeader + sizeof(*pHeader) + pHeader->iSize );
}

#ifdef DETAILED_ZONE_DEBUG_CODE
map <void*,int> mapAllocatedZones;
#endif


typedef struct zoneStats_s
{
	int		iCount;
	int		iCurrent;
	int		iPeak;

	// I'm keeping these updated on the fly, since it's quicker for cache-pool
	//	purposes rather than recalculating each time...
	//
	int		iSizesPerTag [TAG_COUNT];
	int		iCountsPerTag[TAG_COUNT];

} zoneStats_t;

typedef struct zone_s
{
	zoneStats_t				Stats;
	zoneHeader_t			Header;
} zone_t;

cvar_t	*com_validateZone;

zone_t	TheZone = {};


// Scans through the linked list of mallocs and makes sure no data has been overwritten

void Z_Validate(void)
{
	if(!com_validateZone || !com_validateZone->integer)
	{
		return;
	}

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		#ifdef DETAILED_ZONE_DEBUG_CODE
		// this won't happen here, but wtf?
		int& iAllocCount = mapAllocatedZones[pMemory];
		if (iAllocCount <= 0)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Bad block allocation count!");
			return;
		}
		#endif

		if(pMemory->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone header!");
			return;
		}

		if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone tail!");
			return;
		}

		pMemory = pMemory->pNext;
	}
}



// static mem blocks to reduce a lot of small zone overhead
//
#pragma pack(push)
#pragma pack(1)
typedef struct StaticZeroMem_s {
	zoneHeader_t	Header;
//	byte mem[0];
	zoneTail_t		Tail;
} StaticZeroMem_t;

typedef struct StaticMem_s {
	zoneHeader_t	Header;
	byte mem[2];
	zoneTail_t		Tail;
} StaticMem_t;
#pragma pack(pop)

StaticZeroMem_t gZeroMalloc  =
	{ {ZONE_MAGIC, TAG_STATIC,0,NULL,NULL},{ZONE_MAGIC}};
StaticMem_t gEmptyString =
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'\0','\0'},{ZONE_MAGIC}};
StaticMem_t gNumberString[] = {
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'0','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'1','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'2','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'3','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'4','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'5','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'6','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'7','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'8','\0'},{ZONE_MAGIC}},
	{ {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL},{'9','\0'},{ZONE_MAGIC}},
};

qboolean gbMemFreeupOccured = qfalse;
void *Z_Malloc(int iSize, memtag_t eTag, qboolean bZeroit /* = qfalse */, int iUnusedAlign /* = 4 */)
{
	gbMemFreeupOccured = qfalse;

	if (iSize == 0)
	{
		zoneHeader_t *pMemory = (zoneHeader_t *) &gZeroMalloc;
		return &pMemory[1];
	}

	// Add in tracking info
	//
	int iRealSize = (iSize + sizeof(zoneHeader_t) + sizeof(zoneTail_t));

	// Allocate a chunk...
	//
	zoneHeader_t *pMemory = NULL;
	while (pMemory == NULL)
	{
		if (gbMemFreeupOccured)
		{
			Sys_Sleep(1000);	// sleep for a second, so Windows has a chance to shuffle mem to de-swiss-cheese it
		}

		if (bZeroit) {
			pMemory = (zoneHeader_t *) calloc ( iRealSize, 1 );
		} else {
			pMemory = (zoneHeader_t *) malloc ( iRealSize );
		}
		if (!pMemory)
		{
			// new bit, if we fail to malloc memory, try dumping some of the cached stuff that's non-vital and try again...
			//

			// ditch the BSP cache...
			//
			extern qboolean CM_DeleteCachedMap(qboolean bGuaranteedOkToDelete);
			if (CM_DeleteCachedMap(qfalse))
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've just ditched a whole load of memory, so try again with the malloc
			}


			// ditch any sounds not used on this level...
			//
			extern qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel);
			if (SND_RegisterAudio_LevelLoadEnd(qtrue))
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've dropped at least one sound, so try again with the malloc
			}

#ifndef DEDICATED
			// ditch any image_t's (and associated GL memory) not used on this level...
			//
			if (re->RegisterImages_LevelLoadEnd())
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've dropped at least one image, so try again with the malloc
			}
#endif

			// ditch the model-binaries cache...  (must be getting desperate here!)
			//
			if ( re->RegisterModels_LevelLoadEnd(qtrue) )
			{
				gbMemFreeupOccured = qtrue;
				continue;
			}

			// as a last panic measure, dump all the audio memory, but not if we're in the audio loader
			//	(which is annoying, but I'm not sure how to ensure we're not dumping any memory needed by the sound
			//	currently being loaded if that was the case)...
			//
			// note that this keeps querying until it's freed up as many bytes as the requested size, but freeing
			//	several small blocks might not mean that one larger one is satisfiable after freeup, however that'll
			//	just make it go round again and try for freeing up another bunch of blocks until the total is satisfied
			//	again (though this will have freed twice the requested amount in that case), so it'll either work
			//	eventually or not free up enough and drop through to the final ERR_DROP. No worries...
			//
			extern qboolean gbInsideLoadSound;
			extern int SND_FreeOldestSound();
			if (!gbInsideLoadSound)
			{
				int iBytesFreed = SND_FreeOldestSound();
				if (iBytesFreed)
				{
					int iTheseBytesFreed = 0;
					while ( (iTheseBytesFreed = SND_FreeOldestSound()) != 0)
					{
						iBytesFreed += iTheseBytesFreed;
						if (iBytesFreed >= iRealSize)
							break;	// early opt-out since we've managed to recover enough (mem-contiguity issues aside)
					}
					gbMemFreeupOccured = qtrue;
					continue;
				}
			}

			// sigh, dunno what else to try, I guess we'll have to give up and report this as an out-of-mem error...
			//
			// findlabel:  "recovermem"

			Com_Printf(S_COLOR_RED"Z_Malloc(): Failed to alloc %d bytes (TAG_%s) !!!!!\n", iSize, psTagStrings[eTag]);
			Z_Details_f();
			Com_Error(ERR_FATAL,"(Repeat): Z_Malloc(): Failed to alloc %d bytes (TAG_%s) !!!!!\n", iSize, psTagStrings[eTag]);
			return NULL;
		}
	}

	// Link in
	pMemory->iMagic	= ZONE_MAGIC;
	pMemory->eTag	= eTag;
	pMemory->iSize	= iSize;
	pMemory->pNext  = TheZone.Header.pNext;
	TheZone.Header.pNext = pMemory;
	if (pMemory->pNext)
	{
		pMemory->pNext->pPrev = pMemory;
	}
	pMemory->pPrev = &TheZone.Header;
	//
	// add tail...
	//
	ZoneTailFromHeader(pMemory)->iMagic = ZONE_MAGIC;

	// Update stats...
	//
	TheZone.Stats.iCurrent += iSize;
	TheZone.Stats.iCount++;
	TheZone.Stats.iSizesPerTag	[eTag] += iSize;
	TheZone.Stats.iCountsPerTag	[eTag]++;

	if (TheZone.Stats.iCurrent > TheZone.Stats.iPeak)
	{
		TheZone.Stats.iPeak	= TheZone.Stats.iCurrent;
	}

#ifdef DETAILED_ZONE_DEBUG_CODE
	mapAllocatedZones[pMemory]++;
#endif

	Z_Validate();	// check for corruption

	void *pvReturnMem = &pMemory[1];
	return pvReturnMem;
}

// Special wrapper around Z_Malloc for better separation between the main engine
// code and the bundled minizip library.

extern "C" Q_EXPORT void* openjk_minizip_malloc(int size);
extern "C" Q_EXPORT void openjk_minizip_free(void* to_free);

void* openjk_minizip_malloc(int size)
{
    return Z_Malloc(size, TAG_MINIZIP, qfalse, 0);
}

void openjk_minizip_free(void *to_free)
{
    Z_Free(to_free);
}

// used during model cacheing to save an extra malloc, lets us morph the disk-load buffer then
//	just not fs_freefile() it afterwards.
//
void Z_MorphMallocTag( void *pvAddress, memtag_t eDesiredTag )
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_MorphMallocTag(): Not a valid zone header!");
		return;	// won't get here
	}

	// DEC existing tag stats...
	//
//	TheZone.Stats.iCurrent	- unchanged
//	TheZone.Stats.iCount	- unchanged
	TheZone.Stats.iSizesPerTag	[pMemory->eTag] -= pMemory->iSize;
	TheZone.Stats.iCountsPerTag	[pMemory->eTag]--;

	// morph...
	//
	pMemory->eTag = eDesiredTag;

	// INC new tag stats...
	//
//	TheZone.Stats.iCurrent	- unchanged
//	TheZone.Stats.iCount	- unchanged
	TheZone.Stats.iSizesPerTag	[pMemory->eTag] += pMemory->iSize;
	TheZone.Stats.iCountsPerTag	[pMemory->eTag]++;
}

static void Zone_FreeBlock(zoneHeader_t *pMemory)
{
	if (pMemory->eTag != TAG_STATIC)	// belt and braces, should never hit this though
	{
		// Update stats...
		//
		TheZone.Stats.iCount--;
		TheZone.Stats.iCurrent -= pMemory->iSize;
		TheZone.Stats.iSizesPerTag	[pMemory->eTag] -= pMemory->iSize;
		TheZone.Stats.iCountsPerTag	[pMemory->eTag]--;

		// Sanity checks...
		//
		assert(pMemory->pPrev->pNext == pMemory);
		assert(!pMemory->pNext || (pMemory->pNext->pPrev == pMemory));

		// Unlink and free...
		//
		pMemory->pPrev->pNext = pMemory->pNext;
		if(pMemory->pNext)
		{
			pMemory->pNext->pPrev = pMemory->pPrev;
		}
		free (pMemory);


		#ifdef DETAILED_ZONE_DEBUG_CODE
		// this has already been checked for in execution order, but wtf?
		int& iAllocCount = mapAllocatedZones[pMemory];
		if (iAllocCount == 0)
		{
			Com_Error(ERR_FATAL, "Zone_FreeBlock(): Double-freeing block!");
			return;
		}
		iAllocCount--;
		#endif
	}
}

// stats-query function to ask how big a malloc is...
//
int Z_Size(void *pvAddress)
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return 0;	// kind of
	}

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Size(): Not a valid zone header!");
		return 0;	// won't get here
	}

	return pMemory->iSize;
}


// Frees a block of memory...
//
void Z_Free(void *pvAddress)
{
	if (pvAddress == NULL)	// I've put this in as a safety measure because of some bits of #ifdef BSPC stuff	-Ste.
	{
		//Com_Error(ERR_FATAL, "Z_Free(): NULL arg");
		return;
	}

	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return;
	}

	#ifdef DETAILED_ZONE_DEBUG_CODE
	//
	// check this error *before* barfing on bad magics...
	//
	int& iAllocCount = mapAllocatedZones[pMemory];
	if (iAllocCount <= 0)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Block already-freed, or not allocated through Z_Malloc!");
		return;
	}
	#endif

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone header!");
		return;
	}
	if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone tail!");
		return;
	}

	Zone_FreeBlock(pMemory);
}


int Z_MemSize(memtag_t eTag)
{
	return TheZone.Stats.iSizesPerTag[eTag];
}

// Frees all blocks with the specified tag...
//
void Z_TagFree(memtag_t eTag)
{
//#ifdef _DEBUG
//	int iZoneBlocks = TheZone.Stats.iCount;
//#endif

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		zoneHeader_t *pNext = pMemory->pNext;
		if ( (eTag == TAG_ALL) || (pMemory->eTag == eTag))
		{
			Zone_FreeBlock(pMemory);
		}
		pMemory = pNext;
	}

// these stupid pragmas don't work here???!?!?!
//
//#ifdef _DEBUG
//#pragma warning( disable : 4189)
//	int iBlocksFreed = iZoneBlocks - TheZone.Stats.iCount;
//#pragma warning( default : 4189)
//#endif
}


void *S_Malloc( int iSize ) {
	return Z_Malloc( iSize, TAG_SMALL );
}


#ifdef _DEBUG
static void Z_MemRecoverTest_f(void)
{
	// needs to be in _DEBUG only, not good for final game!
	// fixme: findmeste: Remove this sometime
	//
	int iTotalMalloc = 0;
	while (1)
	{
		int iThisMalloc = 5* (1024 * 1024);
		Z_Malloc(iThisMalloc, TAG_SPECIAL_MEM_TEST, qfalse);	// and lose, just to consume memory
		iTotalMalloc += iThisMalloc;

		if (gbMemFreeupOccured)
			break;
	}

	Z_TagFree(TAG_SPECIAL_MEM_TEST);
}
#endif



// Gives a summary of the zone memory usage

static void Z_Stats_f(void)
{
	Com_Printf("\nThe zone is using %d bytes (%.2fMB) in %d memory blocks\n",
								  TheZone.Stats.iCurrent,
									        (float)TheZone.Stats.iCurrent / 1024.0f / 1024.0f,
													  TheZone.Stats.iCount
				);

	Com_Printf("The zone peaked at %d bytes (%.2fMB)\n",
									TheZone.Stats.iPeak,
									         (float)TheZone.Stats.iPeak / 1024.0f / 1024.0f
				);
}

// Gives a detailed breakdown of the memory blocks in the zone

static void Z_Details_f(void)
{
	Com_Printf("---------------------------------------------------------------------------\n");
	Com_Printf("%20s %9s\n","Zone Tag","Bytes");
	Com_Printf("%20s %9s\n","--------","-----");
	for (int i=0; i<TAG_COUNT; i++)
	{
		int iThisCount = TheZone.Stats.iCountsPerTag[i];
		int iThisSize  = TheZone.Stats.iSizesPerTag	[i];

		if (iThisCount)
		{
			// can you believe that using %2.2f as a format specifier doesn't bloody work?
			//	It ignores the left-hand specifier. Sigh, now I've got to do shit like this...
			//
			float	fSize		= (float)(iThisSize) / 1024.0f / 1024.0f;
			int		iSize		= fSize;
			int		iRemainder 	= 100.0f * (fSize - floor(fSize));
			Com_Printf("%20s %9d (%2d.%02dMB) in %6d blocks (%9d average)\n",
					    psTagStrings[i],
							  iThisSize,
								iSize,iRemainder,
								           iThisCount, iThisSize / iThisCount
					   );
		}
	}
	Com_Printf("---------------------------------------------------------------------------\n");

	Z_Stats_f();
}

// Shuts down the zone memory system and frees up all memory
void Com_ShutdownZoneMemory(void)
{
//	Com_Printf("Shutting down zone memory .....\n");

	Cmd_RemoveCommand("zone_stats");
	Cmd_RemoveCommand("zone_details");

	if(TheZone.Stats.iCount)
	{
		Com_Printf("Automatically freeing %d blocks making up %d bytes\n", TheZone.Stats.iCount, TheZone.Stats.iCurrent);
		Z_TagFree(TAG_ALL);

		assert(!TheZone.Stats.iCount);
		assert(!TheZone.Stats.iCurrent);
	}
}

// Initialises the zone memory system

void Com_InitZoneMemory( void )
{
	memset(&TheZone, 0, sizeof(TheZone));
	TheZone.Header.iMagic = ZONE_MAGIC;
}

void Com_InitZoneMemoryVars( void ) {
	//#ifdef _DEBUG
//	com_validateZone = Cvar_Get("com_validateZone", "1", 0);
//#else
	com_validateZone = Cvar_Get("com_validateZone", "0", 0);
//#endif

	Cmd_AddCommand("zone_stats", Z_Stats_f, "Prints out zone memory stats" );
	Cmd_AddCommand("zone_details", Z_Details_f, "Prints out full detailed zone memory info" );

#ifdef _DEBUG
	Cmd_AddCommand("zone_memrecovertest", Z_MemRecoverTest_f);
#endif
}



/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
char *CopyString( const char *in ) {
	char	*out;

	if (!in[0]) {
		return ((char *)&gEmptyString) + sizeof(zoneHeader_t);
	}
	else if (!in[1]) {
		if (in[0] >= '0' && in[0] <= '9') {
			return ((char *)&gNumberString[in[0]-'0']) + sizeof(zoneHeader_t);
		}
	}

	out = (char *) S_Malloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}



static memtag_t hunk_tag;


/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void ) {
//	int		start, end;
	int		i, j;
	int		sum;

//	start = Sys_Milliseconds();
	Z_Validate();

	sum = 0;

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		byte *pMem = (byte *) &pMemory[1];
		j = pMemory->iSize >> 2;
		for (i=0; i<j; i+=64){
			sum += ((int*)pMem)[i];
		}

		pMemory = pMemory->pNext;
	}

//	end = Sys_Milliseconds();
//	Com_Printf( "Com_TouchMemory: %i msec\n", end - start );
}



qboolean Com_TheHunkMarkHasBeenMade(void)
{
	if (hunk_tag == TAG_HUNK_MARK2)
	{
		return qtrue;
	}
	return qfalse;
}

/*
=================
Com_InitHunkMemory
=================
*/
void Com_InitHunkMemory( void ) {
	hunk_tag = TAG_HUNK_MARK1;
	Hunk_Clear();
}

void Com_ShutdownHunkMemory(void)
{
	//Er, ok. Clear it then I guess.
	Z_TagFree(TAG_HUNK_MARK1);
	Z_TagFree(TAG_HUNK_MARK2);
}

/*
====================
Hunk_MemoryRemaining
====================
*/
int	Hunk_MemoryRemaining( void ) {
	return (64*1024*1024) - (Z_MemSize(TAG_HUNK_MARK1)+Z_MemSize(TAG_HUNK_MARK2));	//Yeah. Whatever. We've got no size now.
}

/*
===================
Hunk_SetMark

The server calls this after the level and game VM have been loaded
===================
*/
void Hunk_SetMark( void ) {
	hunk_tag = TAG_HUNK_MARK2;
}

/*
=================
Hunk_ClearToMark

The client calls this before starting a vid_restart or snd_restart
=================
*/
void Hunk_ClearToMark( void ) {
	assert(hunk_tag == TAG_HUNK_MARK2); //if this is not true then no mark has been made
	Z_TagFree(TAG_HUNK_MARK2);
}

/*
=================
Hunk_CheckMark
=================
*/
qboolean Hunk_CheckMark( void ) {
	//if( hunk_low.mark || hunk_high.mark ) {
	if (hunk_tag != TAG_HUNK_MARK1)
	{
		return qtrue;
	}
	return qfalse;
}

void CL_ShutdownCGame( void );
void CL_ShutdownUI( void );
void SV_ShutdownGameProgs( void );

/*
=================
Hunk_Clear

The server calls this before shutting down or loading a new map
=================
*/
void R_HunkClearCrap(void);
#ifdef _FULL_G2_LEAK_CHECKING
void G2_DEBUG_ReportLeaks(void);
#endif

void Hunk_Clear( void ) {

#ifndef DEDICATED
	CL_ShutdownCGame();
	CL_ShutdownUI();
#endif
	SV_ShutdownGameProgs();

#ifndef DEDICATED
	CIN_CloseAllVideos();
#endif

	hunk_tag = TAG_HUNK_MARK1;
	Z_TagFree(TAG_HUNK_MARK1);
	Z_TagFree(TAG_HUNK_MARK2);

	if ( re && re->HunkClearCrap ) {
		re->HunkClearCrap();
	}

//	Com_Printf( "Hunk_Clear: reset the hunk ok\n" );
	VM_Clear();

//See if any ghoul2 stuff was leaked, at this point it should be all cleaned up.
#ifdef _FULL_G2_LEAK_CHECKING
	assert(g_Ghoul2Allocations == 0 && g_G2ClientAlloc == 0 && g_G2ServerAlloc == 0);
	if (g_Ghoul2Allocations)
	{
		Com_Printf("%i bytes leaked by ghoul2 routines (%i client, %i server)\n", g_Ghoul2Allocations, g_G2ClientAlloc, g_G2ServerAlloc);
		G2_DEBUG_ReportLeaks();
	}
#endif
}

/*
=================
Hunk_Alloc

Allocate permanent (until the hunk is cleared) memory
=================
*/
void *Hunk_Alloc( int size, ha_pref preference ) {
	return Z_Malloc(size, hunk_tag, qtrue);
}

/*
=================
Hunk_AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void *Hunk_AllocateTempMemory( int size ) {
	// don't bother clearing, because we are going to load a file over it
	return Z_Malloc(size, TAG_TEMP_HUNKALLOC, qfalse);
}


/*
==================
Hunk_FreeTempMemory
==================
*/
void Hunk_FreeTempMemory( void *buf )
{
	Z_Free(buf);
}


/*
=================
Hunk_ClearTempMemory

The temp space is no longer needed.  If we have left more
touched but unused memory on this side, have future
permanent allocs use this side.
=================
*/
void Hunk_ClearTempMemory( void ) {
	Z_TagFree(TAG_TEMP_HUNKALLOC);
}
