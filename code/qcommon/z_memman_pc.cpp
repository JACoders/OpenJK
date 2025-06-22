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

// Created 2/3/03 by Brian Osman - split Zone code from common.cpp

#include "q_shared.h"
#include "qcommon.h"

#ifdef DEBUG_ZONE_ALLOCS
#include "sstring.h"
int giZoneSnaphotNum=0;
#define DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE 256
typedef sstring<DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE> sDebugString_t;
#endif

static void Z_Details_f(void);

// define a string table of all mem tags...
//
#ifdef TAGDEF	// itu?
#undef TAGDEF
#endif
#define TAGDEF(blah) #blah
static const char *psTagStrings[TAG_COUNT+1]=	// +1 because TAG_COUNT will itself become a string here. Oh well.
{
	#include "tags.h"
};

// This handles zone memory allocation.
// It is a wrapper around malloc with a tag id and a magic number at the start

#define ZONE_MAGIC			0x21436587

// if you change ANYTHING in this structure, be sure to update the tables below using DEF_STATIC...
//
typedef struct zoneHeader_s
{
		int					iMagic;
		memtag_t			eTag;
		int					iSize;
struct	zoneHeader_s		*pNext;
struct	zoneHeader_s		*pPrev;

#ifdef DEBUG_ZONE_ALLOCS
		char				sSrcFileBaseName[MAX_QPATH];
		int					iSrcFileLineNum;
		char				sOptionalLabel[DEBUG_ZONE_ALLOC_OPTIONAL_LABEL_SIZE];
		int					iSnapshotNumber;
#endif

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
std::map <void*,int> mapAllocatedZones;
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

int Z_Validate(void)
{
	int ret=0;
	if(!com_validateZone || !com_validateZone->integer)
	{
		return ret;
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
			return ret;
		}
		#endif

		if(pMemory->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone header!");
			return ret;
		}

		// this block of code is intended to make sure all of the data is paged in
		if (pMemory->eTag != TAG_IMAGE_T
			&& pMemory->eTag != TAG_MODEL_MD3
			&& pMemory->eTag != TAG_MODEL_GLM
			&& pMemory->eTag != TAG_MODEL_GLA )	//don't bother with disk caches as they've already been hit or will be thrown out next
		{
			unsigned char *memstart = (unsigned char *)pMemory;
			int totalSize = pMemory->iSize;
			while (totalSize > 4096)
			{
				memstart += 4096;
				ret += (int)(*memstart); // this fools the optimizer
				totalSize -= 4096;
			}
		}


		if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
		{
			Com_Error(ERR_FATAL, "Z_Validate(): Corrupt zone tail!");
			return ret;
		}

		pMemory = pMemory->pNext;
	}
	return ret;
}



// static mem blocks to reduce a lot of small zone overhead
//
#pragma pack(push)
#pragma pack(1)
typedef struct
{
	zoneHeader_t	Header;
//	byte mem[0];
	zoneTail_t		Tail;
} StaticZeroMem_t;

typedef struct
{
	zoneHeader_t	Header;
	byte mem[2];
	zoneTail_t		Tail;
} StaticMem_t;
#pragma pack(pop)

const static StaticZeroMem_t gZeroMalloc  =
	{ {ZONE_MAGIC, TAG_STATIC,0,NULL,NULL},{ZONE_MAGIC}};

#ifdef DEBUG_ZONE_ALLOCS
#define DEF_STATIC(_char) {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL, "<static>",0,"",0},{_char,'\0'},{ZONE_MAGIC}
#else
#define DEF_STATIC(_char) {ZONE_MAGIC, TAG_STATIC,2,NULL,NULL			        },{_char,'\0'},{ZONE_MAGIC}
#endif

const static StaticMem_t gEmptyString =
	{ DEF_STATIC('\0') };

const static StaticMem_t gNumberString[] = {
	{ DEF_STATIC('0') },
	{ DEF_STATIC('1') },
	{ DEF_STATIC('2') },
	{ DEF_STATIC('3') },
	{ DEF_STATIC('4') },
	{ DEF_STATIC('5') },
	{ DEF_STATIC('6') },
	{ DEF_STATIC('7') },
	{ DEF_STATIC('8') },
	{ DEF_STATIC('9') },
};

qboolean gbMemFreeupOccured = qfalse;

#ifdef DEBUG_ZONE_ALLOCS
// returns actual filename only, no path
// (copes with either slash-scheme for names)
//
// (normally I'd call another function for this, but this is supposed to be engine-independent,
//	 so a certain amount of re-invention of the wheel is to be expected...)
//
char *_D_Z_Filename_WithoutPath(const char *psFilename)
{
	static char sString[ MAX_QPATH ];

	const char *psCopyPos = psFilename;

	while (*psFilename)
	{
		if (*psFilename == PATH_SEP)
			psCopyPos = psFilename+1;
		psFilename++;
	}

	strcpy(sString,psCopyPos);

	return sString;
}
#endif

#include "../rd-common/tr_public.h"	// sorta hack sorta not
extern refexport_t re;

#ifdef DEBUG_ZONE_ALLOCS
void *_D_Z_Malloc ( int iSize, memtag_t eTag, qboolean bZeroit, const char *psFile, int iLine)
#else
void *Z_Malloc(int iSize, memtag_t eTag, qboolean bZeroit, int /*unusedAlign*/)
#endif
{
	gbMemFreeupOccured = qfalse;

	if (iSize == 0)
	{
		zoneHeader_t *pMemory = (zoneHeader_t *) &gZeroMalloc;
		return &pMemory[1];
	}

	// Add in tracking info and round to a longword...  (ignore longword aligning now we're not using contiguous blocks)
	//
//	int iRealSize = (iSize + sizeof(zoneHeader_t) + sizeof(zoneTail_t) + 3) & 0xfffffffc;
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


			// ditch any image_t's (and associated GL texture mem) not used on this level...
			//
			if (re.RegisterImages_LevelLoadEnd())
			{
				gbMemFreeupOccured = qtrue;
				continue;		// we've dropped at least one image, so try again with the malloc
			}


			// ditch the model-binaries cache...  (must be getting desperate here!)
			//
			if (re.RegisterModels_LevelLoadEnd(qtrue))
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
			extern int SND_FreeOldestSound(void);	// I had to add a void-arg version of this because of link issues, sigh
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


#ifdef DEBUG_ZONE_ALLOCS
	Q_strncpyz(pMemory->sSrcFileBaseName, _D_Z_Filename_WithoutPath(psFile), sizeof(pMemory->sSrcFileBaseName));
	pMemory->iSrcFileLineNum	= iLine;
	pMemory->sOptionalLabel[0]	= '\0';
	pMemory->iSnapshotNumber	= giZoneSnaphotNum;
#endif

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
extern "C" Q_EXPORT int openjk_minizip_free(void* to_free);

void* openjk_minizip_malloc(int size)
{
    return Z_Malloc(size, TAG_MINIZIP, qfalse);
}

int openjk_minizip_free(void *to_free)
{
    return Z_Free(to_free);
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


static int Zone_FreeBlock(zoneHeader_t *pMemory)
{
	const int iSize = pMemory->iSize;
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

		//debugging double frees
		pMemory->iMagic = INT_ID('F','R','E','E');
		free (pMemory);


		#ifdef DETAILED_ZONE_DEBUG_CODE
		// this has already been checked for in execution order, but wtf?
		int& iAllocCount = mapAllocatedZones[pMemory];
		if (iAllocCount == 0)
		{
			Com_Error(ERR_FATAL, "Zone_FreeBlock(): Double-freeing block!");
			return -1;
		}
		iAllocCount--;
		#endif
	}
	return iSize;
}

// stats-query function to to see if it's our malloc
// returns block size if so
qboolean Z_IsFromZone(const void *pvAddress, memtag_t eTag)
{
	const zoneHeader_t *pMemory = ((const zoneHeader_t *)pvAddress) - 1;
#if 1	//debugging double free
	if (pMemory->iMagic == INT_ID('F','R','E','E'))
	{
		Com_Printf("Z_IsFromZone(%x): Ptr has been freed already!(%9s)\n",pvAddress,pvAddress);
		return qfalse;
	}
#endif
	if (pMemory->iMagic != ZONE_MAGIC)
	{
		return qfalse;
	}

	//looks like it is from our zone, let's double check the tag

	if (pMemory->eTag != eTag)
	{
		return qfalse;
	}

	return (qboolean)(pMemory->iSize != 0);
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


#ifdef DEBUG_ZONE_ALLOCS
void Z_Label(const void *pvAddress, const char *psLabel)
{
	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

	if (pMemory->eTag == TAG_STATIC)
	{
		return;
	}

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "_D_Z_Label(): Not a valid zone header!");
	}

	Q_strncpyz(	pMemory->sOptionalLabel, psLabel, sizeof(pMemory->sOptionalLabel));
}
#endif



// Frees a block of memory...
//
int Z_Free(void *pvAddress)
{
	if (!TheZone.Stats.iCount)
	{
		//Com_Error(ERR_FATAL, "Z_Free(): Zone has been cleard already!");
		Com_Printf("Z_Free(%x): Zone has been cleard already!\n",pvAddress);
		return -1;
	}

	zoneHeader_t *pMemory = ((zoneHeader_t *)pvAddress) - 1;

#if 1	//debugging double free
	if (pMemory->iMagic == INT_ID('F','R','E','E'))
	{
		Com_Error(ERR_FATAL, "Z_Free(%s): Block already-freed, or not allocated through Z_Malloc!",pvAddress);
		return -1;
	}
#endif

	if (pMemory->eTag == TAG_STATIC)
	{
		return 0;
	}

	#ifdef DETAILED_ZONE_DEBUG_CODE
	//
	// check this error *before* barfing on bad magics...
	//
	int& iAllocCount = mapAllocatedZones[pMemory];
	if (iAllocCount <= 0)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Block already-freed, or not allocated through Z_Malloc!");
		return -1;
	}
	#endif

	if (pMemory->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone header!");
		return -1;
	}
	if (ZoneTailFromHeader(pMemory)->iMagic != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone tail!");
		return -1;
	}

	return Zone_FreeBlock(pMemory);
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


#ifdef DEBUG_ZONE_ALLOCS
void *_D_S_Malloc ( int iSize, const char *psFile, int iLine)
{
	return _D_Z_Malloc( iSize, TAG_SMALL, qfalse, psFile, iLine );
}
#else
void *S_Malloc( int iSize )
{
	return Z_Malloc( iSize, TAG_SMALL, qfalse);
}
#endif


#ifdef _DEBUG
static void Z_MemRecoverTest_f(void)
{
	// needs to be in _DEBUG only, not good for final game!
	//
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: zone_memrecovertest max2alloc\n" );
		return;
	}

	int iMaxAlloc = 1024*1024*atoi( Cmd_Argv(1) );
	int iTotalMalloc = 0;
	while (1)
	{
		const int iThisMalloc = 5* (1024 * 1024);
		Z_Malloc(iThisMalloc, TAG_SPECIAL_MEM_TEST, qfalse);	// and lose, just to consume memory
		iTotalMalloc += iThisMalloc;

		if (gbMemFreeupOccured || (iTotalMalloc >= iMaxAlloc) )
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
//
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
			Com_Printf("%20s %9d (%2d.%02dMB) in %6d blocks (%9d Bytes/block)\n",
				psTagStrings[i],
				iThisSize,
				iSize, iRemainder,
				iThisCount, iThisSize / iThisCount);
		}
	}
	Com_Printf("---------------------------------------------------------------------------\n");

	Z_Stats_f();
}

#ifdef DEBUG_ZONE_ALLOCS
typedef std::map <sDebugString_t, int> LabelRefCount_t;	// yet another place where Gil's string class works and MS's doesn't
typedef std::map <sDebugString_t, LabelRefCount_t>	TagBlockLabels_t;

static TagBlockLabels_t AllTagBlockLabels;

static void Z_Snapshot_f(void)
{
	AllTagBlockLabels.clear();

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		AllTagBlockLabels[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]++;
		pMemory = pMemory->pNext;
	}

	giZoneSnaphotNum++;
	Com_Printf("Ok.    ( Current snapshot num is now %d )\n",giZoneSnaphotNum);
}

static void Z_TagDebug_f(void)
{
	TagBlockLabels_t AllTagBlockLabels_Local;
	qboolean bSnapShotTestActive = qfalse;

	memtag_t eTag = TAG_ALL;

	const char *psTAGName = Cmd_Argv(1);
	if (psTAGName[0])
	{
		// check optional arg...
		//
		if (!Q_stricmp(psTAGName,"#snap"))
		{
			bSnapShotTestActive = qtrue;

			AllTagBlockLabels_Local = AllTagBlockLabels;	// horrible great STL copy

			psTAGName = Cmd_Argv(2);
		}

		if (psTAGName[0])
		{
			// skip over "tag_" if user supplied it...
			//
			if (!Q_stricmpn(psTAGName,"TAG_",4))
			{
				psTAGName += 4;
			}

			// see if the user specified a valid tag...
			//
			for (int i=0; i<TAG_COUNT; i++)
			{
				if (!Q_stricmp(psTAGName,psTagStrings[i]))
				{
					eTag = (memtag_t) i;
					break;
				}
			}
		}
	}
	else
	{
		Com_Printf("Usage: 'zone_tagdebug [#snap] <tag>', e.g. TAG_GHOUL2, TAG_ALL (careful!)\n");
		return;
	}

	Com_Printf("Dumping debug data for tag \"%s\"...%s\n\n",psTagStrings[eTag], bSnapShotTestActive?"( since snapshot only )":"");

	Com_Printf("%8s"," ");	// to compensate for code further down:   Com_Printf("(%5d) ",iBlocksListed);
	if (eTag == TAG_ALL)
	{
		Com_Printf("%20s ","Zone Tag");
	}
	Com_Printf("%9s\n","Bytes");
	Com_Printf("%8s"," ");
	if (eTag == TAG_ALL)
	{
		Com_Printf("%20s ","--------");
	}
	Com_Printf("%9s\n","-----");


	if (bSnapShotTestActive)
	{
		// dec ref counts in last snapshot for all current blocks (which will make new stuff go negative)
		//
		zoneHeader_t *pMemory = TheZone.Header.pNext;
		while (pMemory)
		{
			if (pMemory->eTag == eTag || eTag == TAG_ALL)
			{
				AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]--;
			}
			pMemory = pMemory->pNext;
		}
	}

	// now dump them out...
	//
	int iBlocksListed = 0;
	int iTotalSize = 0;
	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		if (	(pMemory->eTag == eTag	|| eTag == TAG_ALL)
			&&  (!bSnapShotTestActive	|| (pMemory->iSnapshotNumber == giZoneSnaphotNum && AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel] <0) )
			)
		{
			float	fSize		= (float)(pMemory->iSize) / 1024.0f / 1024.0f;
			int		iSize		= fSize;
			int		iRemainder 	= 100.0f * (fSize - floor(fSize));

			Com_Printf("(%5d) ",iBlocksListed);

			if (eTag == TAG_ALL)
			{
				Com_Printf("%20s",psTagStrings[pMemory->eTag]);
			}

			Com_Printf(" %9d (%2d.%02dMB) File: \"%s\", Line: %d\n",
						  pMemory->iSize,
 							  iSize,iRemainder,
												pMemory->sSrcFileBaseName,
															pMemory->iSrcFileLineNum
					   );
			if (pMemory->sOptionalLabel[0])
			{
				Com_Printf("( Label: \"%s\" )\n",pMemory->sOptionalLabel);
			}
			iBlocksListed++;
			iTotalSize += pMemory->iSize;

			if (bSnapShotTestActive)
			{
				// bump ref count so we only 1 warning per new string, not for every one sharing that label...
				//
				AllTagBlockLabels_Local[psTagStrings[pMemory->eTag]][pMemory->sOptionalLabel]++;
			}
		}
		pMemory = pMemory->pNext;
	}

	Com_Printf("( %d blocks listed, %d bytes (%.2fMB) total )\n",iBlocksListed, iTotalSize, (float)iTotalSize / 1024.0f / 1024.0f);
}
#endif

// Shuts down the zone memory system and frees up all memory
void Com_ShutdownZoneMemory(void)
{
	Cmd_RemoveCommand("zone_stats");
	Cmd_RemoveCommand("zone_details");

#ifdef _DEBUG
	Cmd_RemoveCommand("zone_memrecovertest");
#endif

#ifdef DEBUG_ZONE_ALLOCS
	Cmd_RemoveCommand("zone_tagdebug");
	Cmd_RemoveCommand("zone_snapshot");
#endif

	if(TheZone.Stats.iCount)
	{
		Com_Printf("Automatically freeing %d blocks making up %d bytes\n", TheZone.Stats.iCount, TheZone.Stats.iCurrent);
		Z_TagFree(TAG_ALL);

		//assert(!TheZone.Stats.iCount);	// These aren't really problematic per se, it's just warning us that we're freeing extra
		//assert(!TheZone.Stats.iCurrent);  // memory that is in the zone manager (but not actively tracked..) so if anything, zone_*
											// commands will just simply be wrong in displaying bytes, but in my tests, it's only off
											// by like 10 bytes / 1 block, which isn't a real problem --eez
		if(TheZone.Stats.iCount < 0) {
			Com_Printf(S_COLOR_YELLOW"WARNING: Freeing %d extra blocks (%d bytes) not tracked by the zone manager\n",
				abs(TheZone.Stats.iCount), abs(TheZone.Stats.iCurrent));
		}
	}
}

// Initialises the zone memory system

void Com_InitZoneMemory( void )
{
	Com_Printf("Initialising zone memory .....\n");

	memset(&TheZone, 0, sizeof(TheZone));
	TheZone.Header.iMagic = ZONE_MAGIC;
}

void Com_InitZoneMemoryVars( void)
{
	com_validateZone = Cvar_Get("com_validateZone", "0", 0);

	Cmd_AddCommand("zone_stats",	Z_Stats_f);
	Cmd_AddCommand("zone_details",	Z_Details_f);

#ifdef _DEBUG
	Cmd_AddCommand("zone_memrecovertest", Z_MemRecoverTest_f);
#endif

#ifdef DEBUG_ZONE_ALLOCS
	Cmd_AddCommand("zone_tagdebug",	Z_TagDebug_f);
	Cmd_AddCommand("zone_snapshot",	Z_Snapshot_f);
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

	Z_Label(out,in);

	return out;
}


/*
===============
Com_TouchMemory

Touch all known used data to make sure it is paged in
===============
*/
void Com_TouchMemory( void ) {
	//int		start, end;
	int		i, j;
	unsigned int		sum;
	//int		totalTouched;

	Z_Validate();

	//start = Sys_Milliseconds();

	sum = 0;
	//totalTouched=0;

	zoneHeader_t *pMemory = TheZone.Header.pNext;
	while (pMemory)
	{
		byte *pMem = (byte *) &pMemory[1];
		j = pMemory->iSize >> 2;
		for (i=0; i<j; i+=64){
			sum += ((unsigned int*)pMem)[i];
		}
		//totalTouched+=pMemory->iSize;
		pMemory = pMemory->pNext;
	}

	//end = Sys_Milliseconds();

	//Com_Printf( "Com_TouchMemory: %i bytes, %i msec\n", totalTouched, end - start );
}


