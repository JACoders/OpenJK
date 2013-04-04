
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

/*
 *	ZONE MEMORY MANAGER
 *
 *	Goals:
 *		1. Minimize overhead
 *		2. Minimize fragmentation
 *
 *	Constraints:
 *		1. Maximum allocated block size is 32MB
 *		2. Maximum 64 different memory tags supported
 *		3. Maximum 256 byte alignment
 *
 *	All memory required by the manager is allocated at startup in
 *	the form of one large pool.
 *
 *	Allocated blocks require a 4 byte header to store size, tag, and
 *	alignment information.  Blocks that need to support the Z_TagFree()
 *	feature require an additional 8 byte link list structure.
 *
 *	Free blocks require a 16 bytes of tracking information.  If possible
 *	this information is stored directly in the block (which is in the
 *	pool.)  If the free block is not large enough, its information is
 *	stored in an overflow buffer.
 *
 *	In an effort to reduce fragmentation, blocks allocated for a short
 *	period of time at the end of the pool.  All other blocks are allocated
 *	at the start.  Allocation is first fit.
 *
 */

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../renderer/qgl_console.h"

#ifdef _GAMECUBE
#include <dolphin/os.h>
#endif

#ifdef _WINDOWS
#include <windows.h>
#endif

#ifdef _XBOX
#include <Xtl.h>
#include "../win32/xbox_texture_man.h"
#endif

// Used to mark the start and end of blocks in debug mode
#define ZONE_MAGIC 0xfe

// Size of the free block overflow buffer
#define ZONE_FREE_OVERFLOW 4096

// Indicates whether or not special (slow) debug code should be enabled
#define ZONE_DEBUG 0

// Allocate all available memory minus this amount - texture pools are
// allocated before this, so just leave enough for framebuffer, etc...
// Gah! I hate Bink! Stupid thing allocates physical memory (probably via
// DSound) when starting. Need to leave just a little more.
#ifdef FINAL_BUILD
#	define ZONE_HEAP_FREE (1024*1024*6 + 512*1024 + 256*1024 + 64*1024)
#else
#	define ZONE_HEAP_FREE (1024*1024*16 + 16*1024*1024)
#endif

#ifdef FINAL_BUILD
#define TEXTURE_POOL_SIZE	16*1024*1024
#else
#define TEXTURE_POOL_SIZE	20*1024*1024
#endif

// Two systems (Bink, and Savegames) need large, contiguous allocations once things
// are running and fragmented. They get their own sandbox:
#define TEMP_ALLOC_POOL_SIZE	(2*1024*1024 + 512*1024)

__declspec (align(32)) char s_TempAllocPool[TEMP_ALLOC_POOL_SIZE];
int s_TempAllocPoint = 0;

void *TempAlloc( unsigned long size )
{
	if( s_TempAllocPoint + size > TEMP_ALLOC_POOL_SIZE )
	{
		Com_Printf( "WARNING: TempAlloc pool full!\n" );
		return NULL;
	}

	void *retVal = &s_TempAllocPool[s_TempAllocPoint];

	s_TempAllocPoint = (s_TempAllocPoint + size + 31) & ~31;

	return retVal;
}

void TempFree( void )
{
	s_TempAllocPoint = 0;
}

// Should we emulate the smaller memory footprint of actual release systems?
#define ZONE_EMULATE_SPACE 0

// All standard header data is crammed into 4 bytes
typedef unsigned int ZoneHeader;

// Debug markers to check for overflow/underflow
typedef unsigned int ZoneDebugHeader;
typedef unsigned char ZoneDebugFooter;

// Extended header information for memory freed with TagFree()
struct ZoneLinkHeader
{
	ZoneLinkHeader* m_Next;
	ZoneLinkHeader* m_Prev;
};

static ZoneLinkHeader* s_LinkBase;

// Free memory block tracking information
struct ZoneFreeBlock
{
	unsigned int m_Address;
	unsigned int m_Size;
	ZoneFreeBlock* m_Next;
	ZoneFreeBlock* m_Prev;
};

// Buffer to hold free memory information that we can't
// fit directly in the pool
static ZoneFreeBlock s_FreeOverflow[ZONE_FREE_OVERFLOW];
static int s_LastOverflowIndex;

static ZoneFreeBlock s_FreeStart;
static ZoneFreeBlock s_FreeEnd;

// Various stats collected at runtime
struct ZoneStats
{
	int	m_CountAlloc;
	int m_SizeAlloc;
	int m_OverheadAlloc;
	int	m_PeakAlloc;
	int	m_CountFree;
	int m_SizeFree;
	int	m_SizesPerTag[TAG_COUNT];
	int	m_CountsPerTag[TAG_COUNT];	
};

static ZoneStats s_Stats;

// Special empty block for zero size allocations
struct ZoneEmptyBlock
{
	ZoneHeader header;
#ifdef _DEBUG
	ZoneDebugHeader start;
	ZoneDebugFooter end;
#endif
};

#ifdef _DEBUG
static ZoneEmptyBlock s_EmptyBlock = {TAG_STATIC << 25, ZONE_MAGIC, ZONE_MAGIC};
#else
static ZoneEmptyBlock s_EmptyBlock = {TAG_STATIC << 25};
#endif

// Free block jump table for fast memory deallocation
#define Z_JUMP_TABLE_SIZE 64
static ZoneFreeBlock* s_FreeJumpTable[Z_JUMP_TABLE_SIZE];
static unsigned int s_FreeJumpResolution;

static void* s_PoolBase;
static int s_PoolSize;
static bool s_Initialized = false;

static memtag_t s_newDeleteTagStack[32] = { TAG_NEWDEL };
static int s_newDeleteTagStackTop = 0;

#ifndef _GAMECUBE
static HANDLE s_Mutex = INVALID_HANDLE_VALUE;
#endif

static void Z_Stats_f(void);
void Z_Details_f(void);
void Z_DumpMemMap_f(void);
void Z_CompactStats(void);

void Z_PushNewDeleteTag( memtag_t eTag )
{
	assert( s_newDeleteTagStackTop < 31 );
	s_newDeleteTagStack[++s_newDeleteTagStackTop] = eTag;
}

void Z_PopNewDeleteTag( void )
{
	assert( s_newDeleteTagStackTop );
	--s_newDeleteTagStackTop;
}

#ifdef _XBOX
void ShowOSMemory(void)
{
	MEMORYSTATUS stat;
	GlobalMemoryStatus(&stat);
	Com_Printf("     total mem: %d, free mem: %d\n", stat.dwTotalPhys / 1024,
			stat.dwAvailPhys / 1024);
	FILE *out = fopen("d:\\osmem.txt", "a");
	if(out) {
		fprintf(out, "total mem: %d, free mem: %d\n", stat.dwTotalPhys / 1024,
				stat.dwAvailPhys / 1024);
		fclose(out);
	}
}
#endif


int Z_MemFree(void)
{
	return s_Stats.m_SizeFree;
}

void Com_InitZoneMemory(void)
{
//	assert(!s_Initialized);
	// Zone now initializes on first use, can't reliably assume anything here
	if (s_Initialized)
		return;

	Com_Printf("Initialising zone memory .....\n");

	// Clear some globals
	memset(&s_Stats, 0, sizeof(s_Stats));
	memset(s_FreeOverflow, 0, sizeof(s_FreeOverflow));
	s_LastOverflowIndex = 0;
	s_LinkBase = NULL;

	// Alloc the pool
	MEMORYSTATUS status;
	GlobalMemoryStatus(&status);

	// BTO : VVFIXME - Extra little note to see how much memory
	// is being used by globals/statics
	Com_Printf("*** PhysRAM: %d used, %d free\n",
				status.dwTotalPhys-status.dwAvailPhys,
				status.dwAvailPhys);

	// Allocate the texture pool:
	gTextures.Initialize( TEXTURE_POOL_SIZE );

	GlobalMemoryStatus(&status);

	// BTO : VVFIXME - Extra little note to see how much memory
	// is being used by globals/statics
	Com_Printf("*** PhysRAM: %d used, %d free\n",
				status.dwTotalPhys-status.dwAvailPhys,
				status.dwAvailPhys);
	SIZE_T size;
#	if ZONE_EMULATE_SPACE
#ifdef _DEBUG
	//Emulated space is always about 6 megs off from release build.  Try
	//to compensate.  This number may need tweaking in the future.
	SIZE_T exe = 6500 * 1024;
#else
	SIZE_T exe = 0; //Exe size is already reflected in GlobalMemoryStatus().
#endif
	size = 0x4000000 - (exe + ZONE_HEAP_FREE);
#	else
	size = status.dwAvailPhys - ZONE_HEAP_FREE;
#	endif

	s_PoolBase = GlobalAlloc(0, size);
	s_PoolSize = size;

	// Setup the initial free block
	ZoneFreeBlock* base = (ZoneFreeBlock*)s_PoolBase;
	base->m_Address = (unsigned int)s_PoolBase;
	base->m_Size = size;
	base->m_Next = &s_FreeEnd;
	base->m_Prev = &s_FreeStart;

	// Init the free block jump table
	memset(s_FreeJumpTable, 0, Z_JUMP_TABLE_SIZE * sizeof(ZoneFreeBlock*));
	s_FreeJumpResolution = (size / Z_JUMP_TABLE_SIZE) + 1;
	s_FreeJumpTable[0] = base;

	// Setup free block dummies
	s_FreeStart.m_Address = 0;
	s_FreeStart.m_Size = 0;
	s_FreeStart.m_Next = base;
	s_FreeStart.m_Prev = NULL;

	s_FreeEnd.m_Address = 0xFFFFFFFF;
	s_FreeEnd.m_Size = 0;
	s_FreeEnd.m_Next = NULL;
	s_FreeEnd.m_Prev = base;

	s_Stats.m_CountFree = 1;
	s_Stats.m_SizeFree = size;

	s_Initialized = true;

	// Add some commands
	Cmd_AddCommand("zone_stats",	Z_Stats_f);
	Cmd_AddCommand("zone_details",	Z_Details_f);
	Cmd_AddCommand("zone_memmap",	Z_DumpMemMap_f);
	Cmd_AddCommand("zone_cstats",	Z_CompactStats);

#ifndef _GAMECUBE
	s_Mutex = CreateMutex(NULL, FALSE, NULL);
#endif

	// Super-size my hack. With fries. We allocate enough space for g_entities at the
	// end of the zone now. If it turns out (somehow) that there is no persisted surface,
	// then we need to take g_entities from the zone, but we'd normally allocate it so late
	// that we'll have a big chunk in the middle. That's bad. And if we reserve space at the
	// front, then we might not need it, and we've got a 1.2MB chunk of empty space there.
	// This works perfectly, though:
	extern void G_ReserveZoneGentities( void );
	G_ReserveZoneGentities();
}

void Com_ShutdownZoneMemory(void)
{
	assert(s_Initialized);

	// Remove commands
	Cmd_RemoveCommand("zone_stats");
	Cmd_RemoveCommand("zone_details");
	Cmd_RemoveCommand("zone_memmap");

	if (s_Stats.m_CountAlloc)
	{
		// Free all memory
//		CM_ReleaseVisData();
		Z_TagFree(TAG_ALL);
	}
	
	// Clear some globals
	memset(&s_Stats, 0, sizeof(s_Stats));
	memset(s_FreeOverflow, 0, sizeof(s_FreeOverflow));
	s_LastOverflowIndex = 0;
	s_LinkBase = NULL;

	// Free the pool
#ifndef _GAMECUBE
	GlobalFree(s_PoolBase);
	CloseHandle(s_Mutex);
#endif

	s_PoolBase = NULL;
	s_Initialized = false;
}


// Determine if a tag should only be allocated for a very
// short period of time.
static bool Z_IsTagTemp(memtag_t eTag)
{
	return 
		eTag == TAG_TEMP_WORKSPACE || 
		eTag == TAG_SND_RAWDATA ||
		eTag == TAG_ICARUS ||
		eTag == TAG_LISTFILES ||
		eTag == TAG_GP2;
}

// Determine if a tag needs TagFree() support.
static bool Z_IsTagLinked(memtag_t eTag)
{
	return
		eTag == TAG_BSP ||
		eTag == TAG_HUNKALLOC ||
//		eTag == TAG_HUNKMISCMODELS ||
		eTag == TAG_G_ALLOC ||
		eTag == TAG_UI_ALLOC;
}

static int Z_CalcAlignmentPad(int iAlign, unsigned int iAddress, unsigned int iOffset, 
	unsigned int iSize, unsigned int iHeaderSize, unsigned int iFooterSize)
{
	int align_size;

	if (iAlign == 0) return 0;
	
	if (iOffset == 0)
	{
		// Align data at low end of block
		align_size = iAlign - 
			((iAddress + iHeaderSize) % iAlign);
	}
	else
	{
		// Align data at high end of block
		unsigned int block_start = iAddress + iOffset - 
			iSize + iHeaderSize;
		align_size = block_start % iAlign;
	}
	
	if (align_size == iAlign)
	{
		return 0;
	}

	return align_size;
}

static ZoneFreeBlock* Z_GetOverflowBlock(void)
{
	for (int i = s_LastOverflowIndex; i < ZONE_FREE_OVERFLOW; ++i)
	{
		if (s_FreeOverflow[i].m_Address == 0)
		{
			s_LastOverflowIndex = i;
			return &s_FreeOverflow[i];
		}
	}

	for (int j = 0; j < s_LastOverflowIndex; ++j)
	{
		if (s_FreeOverflow[j].m_Address == 0)
		{
			s_LastOverflowIndex = j;
			return &s_FreeOverflow[j];
		}
	}

	return NULL;
}

static inline bool Z_IsFreeBlockLargeEnough(ZoneFreeBlock* pBlock, int iSize, 
	int iHeaderSize, int iFooterSize, int iAlign, bool bLow, int& iAlignPad)
{
	// Is the block large enough?
	if (pBlock->m_Size >= iSize)
	{
		if (iAlign > 0)
		{
			// If we need some aligment, we need to check size
			// against that as well.
			iAlignPad = Z_CalcAlignmentPad(iAlign,
				pBlock->m_Address, !bLow ? pBlock->m_Size : 0,
				iSize, iHeaderSize, iFooterSize);

			if (pBlock->m_Size < iAlignPad + iSize)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

static ZoneFreeBlock* Z_FindFirstFree(int iSize, int iHeaderSize, 
	int iFooterSize, int iAlign, int& iAlignPad)
{
	for (ZoneFreeBlock* block = s_FreeStart.m_Next; block; block = block->m_Next)
	{
		if (Z_IsFreeBlockLargeEnough(block, iSize, iHeaderSize, iFooterSize,
			iAlign, true, iAlignPad))
		{
			return block;
		}
	}
	return NULL;
}

static ZoneFreeBlock* Z_FindLastFree(int iSize, int iHeaderSize, 
	int iFooterSize, int iAlign, int& iAlignPad)
{
	for (ZoneFreeBlock* block = s_FreeEnd.m_Prev; block; block = block->m_Prev)
	{
		if (Z_IsFreeBlockLargeEnough(block, iSize, iHeaderSize, iFooterSize,
			iAlign, false, iAlignPad))
		{
			return block;
		}
	}
	return NULL;
}

static bool Z_ValidateFree(void)
{
#if ZONE_DEBUG
	// Make sure no free blocks are overlapping
	for (ZoneFreeBlock* a = &s_FreeStart; a; a = a->m_Next)
	{
		if (a->m_Address == 0 && a->m_Size != 0)
		{
			return false;
		}

		for (ZoneFreeBlock* b = &s_FreeStart; b; b = b->m_Next)
		{
			if (a != b && 
				a->m_Address >= b->m_Address && 
				a->m_Address < b->m_Address + b->m_Size)
			{
				return false;
			}
		}
	}
#endif

	return true;
}

static bool Z_ValidateLinks(void)
{
#if ZONE_DEBUG
	// Make sure links are sane
	for (ZoneLinkHeader* a = s_LinkBase; a; a = a->m_Next)
	{
		if ((a->m_Next && a != a->m_Next->m_Prev) ||
			(a->m_Prev && a != a->m_Prev->m_Next))
		{
			return false;
		}
	}
#endif

	return true;
}

static int Z_GetJumpTableIndex(unsigned int iAddress)
{
	int index = (iAddress - (unsigned int)s_PoolBase) / s_FreeJumpResolution;
	if (index < 0) return 0;
	if (index >= Z_JUMP_TABLE_SIZE) return Z_JUMP_TABLE_SIZE - 1;
	return index;
}

static ZoneFreeBlock* Z_GetFreeBlockBefore(unsigned int iAddress)
{
	// Find this block's position in the jump table
	int index = Z_GetJumpTableIndex(iAddress) - 1;

	// Find a valid jump table entry
	while (index >= 0 && !s_FreeJumpTable[index]) --index;

	if (index < 0) return &s_FreeStart;
	return s_FreeJumpTable[index];
}

static void Z_RemoveFromJumpTable(ZoneFreeBlock* pBlock)
{
	// Is this block in the jump table?
	int index = Z_GetJumpTableIndex(pBlock->m_Address);
	if (s_FreeJumpTable[index] == pBlock)
	{
		// See if the next block will fit in our slot
		if (pBlock->m_Next != &s_FreeEnd)
		{
			int nindex = Z_GetJumpTableIndex(pBlock->m_Next->m_Address);
			if (nindex == index)
			{
				s_FreeJumpTable[index] = pBlock->m_Next;
				return;
			}
		}

		// See if the previous block will fit in our slot
		if (pBlock->m_Prev != &s_FreeStart)
		{
			int pindex = Z_GetJumpTableIndex(pBlock->m_Prev->m_Address);
			if (pindex == index)
			{
				s_FreeJumpTable[index] = pBlock->m_Prev;
				return;
			}
		}

		// No other free blocks fit here, give up
		s_FreeJumpTable[index] = NULL;
	}
}

static void Z_LinkFreeBlock(ZoneFreeBlock* pBlock)
{
	ZoneFreeBlock* cur = Z_GetFreeBlockBefore(pBlock->m_Address);
	for (; cur; cur = cur->m_Next)
	{
		// Find the correct position, ordered by address
		if (cur->m_Address > pBlock->m_Address)
		{
			// Link up the block
			pBlock->m_Next = cur;
			pBlock->m_Prev = cur->m_Prev;
			cur->m_Prev->m_Next = pBlock;
			cur->m_Prev = pBlock;

			// Update the jump table if necessary
			int index = Z_GetJumpTableIndex(pBlock->m_Address);
			if (!s_FreeJumpTable[index])
			{
				s_FreeJumpTable[index] = pBlock;
			}

			s_Stats.m_CountFree++;
			s_Stats.m_SizeFree += pBlock->m_Size;
			
			assert(Z_ValidateFree());
			break;
		}
	}	
}

static void* Z_SplitFree(ZoneFreeBlock* pBlock, int iSize, bool bLow)
{
	assert(pBlock->m_Size >= iSize);
	
	Z_RemoveFromJumpTable(pBlock);

	// Delink the free block
	ZoneFreeBlock fblock = *pBlock;
	pBlock->m_Prev->m_Next = pBlock->m_Next;
	pBlock->m_Next->m_Prev = pBlock->m_Prev;
	pBlock->m_Address = 0;

	s_Stats.m_CountFree--;
	s_Stats.m_SizeFree -= pBlock->m_Size;
	assert(Z_ValidateFree());

	if (fblock.m_Size > iSize)
	{
		// Split the block into an allocated and free portion
		int remainder = fblock.m_Size - iSize;

		if (remainder < sizeof(ZoneFreeBlock))
		{
			// Free portion is not large to hold free info --
			// we're going to have to use the overflow buffer.
			ZoneFreeBlock* nblock = Z_GetOverflowBlock();

			if (nblock == NULL)
			{
				Z_Details_f();
				Com_Error(ERR_FATAL, "Zone free overflow buffer overflowed!");
			}

			// Split the block
			void* ret;
			if (bLow)
			{
				ret = (void*)fblock.m_Address;
				nblock->m_Address = fblock.m_Address + iSize;
			}
			else
			{
				ret = (void*)(fblock.m_Address + remainder);
				nblock->m_Address = fblock.m_Address;
			}
	
			nblock->m_Size = remainder;
			Z_LinkFreeBlock(nblock);

			return ret;
		}
		else
		{
			// Free portion is large enough -- split it
			void* ret;
			ZoneFreeBlock* nblock;
			if (bLow)
			{
				ret = (void*)fblock.m_Address;
				nblock = (ZoneFreeBlock*)(fblock.m_Address + iSize);
			}
			else
			{
				ret = (void*)(fblock.m_Address + remainder);
				nblock = (ZoneFreeBlock*)fblock.m_Address;
			}

			nblock->m_Address = (unsigned int)nblock;
			nblock->m_Size = remainder;
			
			Z_LinkFreeBlock(nblock);

			return ret;
		}
	}
	else
	{
		// No need to split, just return block.
		return (void*)fblock.m_Address;
	}
}

static void Z_SetupAlignmentPad(void* pBlock, int iAlignPad, bool bLow)
{
	// Clear alignment bytes
	memset(pBlock, 0, iAlignPad);
	
	// If we have more than 1 alignment byte, the first align byte
	// tells us how many additional bytes we have.
	if (iAlignPad > 1)
	{
		assert(iAlignPad < 256);
		unsigned char* ptr;
		if (bLow)
		{
			ptr = (unsigned char*)pBlock + (iAlignPad - 1);
		}
		else
		{
			ptr = (unsigned char*)pBlock;
		}
		*ptr = iAlignPad - 1;
	}
}

void Z_MallocFail(const char* pMessage, int iSize, memtag_t eTag)
{
	// Report the error
//	Com_Printf("Z_Malloc(): %s : %d bytes and tag %d !!!!\n", pMessage, iSize, eTag);
	Com_Printf("Z_Malloc(): %s : %d bytes and tag %d !!!!\n", pMessage, iSize, eTag);
	Z_Details_f();
	Z_DumpMemMap_f();
//	Com_Printf("(Repeat): Z_Malloc(): %s : %d bytes and tag %d !!!!\n", pMessage, iSize, eTag);
	Com_Printf("(Repeat): Z_Malloc(): %s : %d bytes and tag %d !!!!\n", pMessage, iSize, eTag);

	// Clear the screen blue to indicate out of memory
	for (;;)
	{
		qglBeginFrame();
		qglClearColor(0, 0, 1, 1);
		qglClear(GL_COLOR_BUFFER_BIT);
		qglEndFrame();
	}
}

void *Z_Malloc(int iSize, memtag_t eTag, qboolean bZeroit, int iAlign)
{
//	assert(s_Initialized);
	// Zone now initializes on first use. (During static constructors)
	if (!s_Initialized)
		Com_InitZoneMemory();
	
	if (iSize == 0)
	{
#ifdef _DEBUG
		return (void*)(&s_EmptyBlock.start + 1);
#else
		return (void*)(&s_EmptyBlock.header + 1);
#endif
	}

	if (iSize < 0)
	{
		Z_MallocFail("Negative size", iSize, eTag);
		return NULL;
	}

#ifndef _GAMECUBE
	WaitForSingleObject(s_Mutex, INFINITE);
#endif
	
	// Make new/delete memory temporary if requested
	if (eTag == TAG_NEWDEL )
	{
		eTag = s_newDeleteTagStack[s_newDeleteTagStackTop];
	}

	// Determine how much space we need with headers and footers
	int header_size = sizeof(ZoneHeader);
	int footer_size = 0;
	if (Z_IsTagLinked(eTag))
	{
		header_size += sizeof(ZoneLinkHeader);
	}
#ifdef _DEBUG
	header_size += sizeof(ZoneDebugHeader);
	footer_size += sizeof(ZoneDebugFooter);
#endif
	int real_size = iSize + header_size + footer_size;
	int align_pad = 0;

	// Get a bit of free memory.  Temporary memory is allocated
	// from the end.  More permanent allocations are done at the
	// begining of the pool.
	ZoneFreeBlock* fblock;
	if (Z_IsTagTemp(eTag))
	{
		fblock = Z_FindLastFree(real_size, header_size, footer_size, 
			iAlign, align_pad);
	}
	else
	{
		fblock = Z_FindFirstFree(real_size, header_size, footer_size, 
			iAlign, align_pad);
	}

	// Did we actually find some memory?
	if (!fblock)
	{
#ifndef _GAMECUBE
		ReleaseMutex(s_Mutex);
#endif
//		if(eTag == TAG_TEMP_SND_RAWDATA) {
		if(eTag == TAG_SND_RAWDATA) {
			return NULL;
		}

		Z_MallocFail("Out of memory", iSize, eTag);
		return NULL;
	}

	// Add any alignment bytes
	real_size += align_pad;

	// Split the free block and get a pointer to the start
	// allocated space.
	void* ablock;
	if (Z_IsTagTemp(eTag))
	{
		ablock = Z_SplitFree(fblock, real_size, false);
		
		// Append align pad to end of block
		Z_SetupAlignmentPad(
			(void*)((char*)ablock + real_size - align_pad), 
			align_pad, false);
	}
	else
	{
		ablock = Z_SplitFree(fblock, real_size, true);

		// Insert align pad at block start
		Z_SetupAlignmentPad(ablock, align_pad, true);
		ablock = (void*)((char*)ablock + align_pad);
	}

	if (!ablock)
	{
		Z_MallocFail("Failed to split", iSize, eTag);
	}

	// Add linking header if necessary
	if (Z_IsTagLinked(eTag))
	{
		ZoneLinkHeader* linked = (ZoneLinkHeader*)ablock;
		linked->m_Next = s_LinkBase;
		linked->m_Prev = NULL;
		if (s_LinkBase)
		{
			s_LinkBase->m_Prev = linked;
		}
		s_LinkBase = linked;

		assert(Z_ValidateLinks());

		// Next...
		ablock = (void*)((char*)ablock + sizeof(ZoneLinkHeader));
	}

	// Setup the header:
	//		31		- alignment flag
	//		25-30	- tag
	//		0-24	- size without headers/footers
	assert(iSize >= 0 && iSize < (1 << 25));
	assert(eTag >= 0 && eTag < 64);
	ZoneHeader* header = (ZoneHeader*)ablock;
	*header = 
		(((unsigned int)eTag) << 25) |
		((unsigned int)iSize);

	if (align_pad)
	{
		*header |= (1 << 31);
	}

	// Next...
	ablock = (void*)((char*)ablock + sizeof(ZoneHeader));

#ifdef _DEBUG
	{
		// Setup the debug markers
		ZoneDebugHeader* debug_header = (ZoneDebugHeader*)ablock;

		ZoneDebugFooter* debug_footer = (ZoneDebugFooter*)((char*)debug_header + 
			(sizeof(ZoneDebugHeader) + iSize));

		*debug_header = ZONE_MAGIC;
		*debug_footer = ZONE_MAGIC;

		// Next...
		ablock = (void*)((char*)ablock + sizeof(ZoneDebugHeader));
	}
#endif

	// Update the stats
	s_Stats.m_SizeAlloc += iSize;
	s_Stats.m_OverheadAlloc += header_size + footer_size + align_pad;
	s_Stats.m_SizesPerTag[eTag] += iSize;
	s_Stats.m_CountAlloc++;
	s_Stats.m_CountsPerTag[eTag]++;

	if (s_Stats.m_SizeAlloc + s_Stats.m_OverheadAlloc > s_Stats.m_PeakAlloc)
	{
		s_Stats.m_PeakAlloc = s_Stats.m_SizeAlloc + s_Stats.m_OverheadAlloc;
	}

	// Return a pointer to data memory
	if (bZeroit)
	{
		memset(ablock, 0, iSize);
	}

	assert(iAlign == 0 || (unsigned int)ablock % iAlign == 0);

	/*
	   This is useful for figuring out who's allocating a certain block of
	   memory.  Please don't remove it.
	if(eTag == TAG_NEWDEL && (unsigned int)ablock >= 0x806c0000 && 
			(unsigned int)ablock <= 0x806c1000 && iSize == 24) {
		int suck = 0;
	}
	if(eTag == TAG_SMALL && (iSize == 7 || iSize == 96)) {
		int suck = 0;
	}
	if(eTag == TAG_CLIENTS) {
		int suck = 0;
	}

	if ((unsigned)ablock >= 0x169b000 && (unsigned)ablock <= 0x169c000 && iSize == 20)
	{
		int suck = 0;
	}
	*/

#ifndef _GAMECUBE
	ReleaseMutex(s_Mutex);
#endif

	return ablock;
}

static memtag_t Z_GetTag(const ZoneHeader* header)
{
	return (*header & 0x7E000000) >> 25;
}

static unsigned int Z_GetSize(const ZoneHeader* header)
{
	return *header & 0x1FFFFFF;
}

static int Z_GetAlign(const ZoneHeader* header)
{
	if (*header & (1 << 31))
	{
		unsigned char* ptr = (unsigned char*)header;
		memtag_t tag = Z_GetTag(header);

		// point to the first alignment block
		if (Z_IsTagTemp(tag))
		{
			ptr += sizeof(ZoneHeader) + Z_GetSize(header);
#ifdef _DEBUG
			ptr += sizeof(ZoneDebugHeader) + sizeof(ZoneDebugFooter);
#endif
		}
		else
		{
			if (Z_IsTagLinked(tag))
			{
				// skip the link header
				ptr -= sizeof(ZoneLinkHeader);
			}
			ptr -= 1;
		}

		return *ptr + 1;
	}
	return 0;
}

int Z_Size(void *pvAddress)
{
	assert(s_Initialized);

#ifdef _DEBUG
	ZoneDebugHeader* debug = (ZoneDebugHeader*)pvAddress - 1;

	if (*debug != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Size(): Not a valid zone header!");
		return 0;	// won't get here
	}

	pvAddress = (void*)debug;
#endif

	ZoneHeader* header = (ZoneHeader*)pvAddress - 1;

	if (Z_GetTag(header) == TAG_STATIC)
	{
		return 0;	// kind of
	}
	
	return Z_GetSize(header);
}

static void Z_Coalasce(ZoneFreeBlock* pBlock)
{
	unsigned int size = 0;
	
	// Find later free blocks adjacent to us
	ZoneFreeBlock* end;
	for (end = pBlock->m_Next; 
	end->m_Next; 
	end = end->m_Next)
	{
		if (end->m_Address !=
			end->m_Prev->m_Address + end->m_Prev->m_Size)
		{
			break;
		}

		size += end->m_Size;

		Z_RemoveFromJumpTable(end);

		end->m_Address = 0; // invalidate block
		s_Stats.m_CountFree--;
	}

	// Find previous free blocks adjacent to us
	ZoneFreeBlock* start;
	for (start = pBlock; 
	start->m_Prev; 
	start = start->m_Prev)
	{
		if (start->m_Prev->m_Address + start->m_Prev->m_Size !=
			start->m_Address)
		{
			break;
		}

		size += start->m_Size;

		Z_RemoveFromJumpTable(start);

		start->m_Address = 0; // invalidate block
		s_Stats.m_CountFree--;
	}

	// Do we need to coalesce some blocks?
	if (start->m_Next != end)
	{
		start->m_Next = end;
		end->m_Prev = start;
		start->m_Size += size;
	}
}

// Return type of Z_Free differs in SP/MP. Macro hack to wrap it up
#ifdef _JK2MP
	void Z_Free(void *pvAddress)
	#define Z_FREE_RETURN(x) return
#else
int Z_Free(void *pvAddress)
	#define Z_FREE_RETURN(x) return (x)
#endif
{
#ifdef _WINDOWS
	if (!s_Initialized) return;
#endif

	assert(s_Initialized);

#ifdef _DEBUG
	// check the header magic
	ZoneDebugHeader* debug_header = (ZoneDebugHeader*)pvAddress - 1;

	if (*debug_header != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone header!");
		Z_FREE_RETURN( 0 );
	}

	ZoneHeader* header = (ZoneHeader*)debug_header - 1;

	// check the footer magic
	ZoneDebugFooter* debug_footer = (ZoneDebugFooter*)((char*)pvAddress + 
		Z_GetSize(header));

	if (*debug_footer != ZONE_MAGIC)
	{
		Com_Error(ERR_FATAL, "Z_Free(): Corrupt zone footer!");
		Z_FREE_RETURN( 0 );
	}
#else
	ZoneHeader* header = (ZoneHeader*)pvAddress - 1;
#endif

	memtag_t tag = Z_GetTag(header);

	if (tag != TAG_STATIC)
	{
#ifndef _GAMECUBE
		WaitForSingleObject(s_Mutex, INFINITE);
#endif

		// Determine size of header and footer
		int header_size = sizeof(ZoneHeader);
		int align_size = Z_GetAlign(header);
		int footer_size = 0;
		int data_size = Z_GetSize(header);
		if (Z_IsTagLinked(tag))
		{
			header_size += sizeof(ZoneLinkHeader);
		}
		if (Z_IsTagTemp(tag))
		{
			footer_size += align_size;
		}
		else
		{
			header_size += align_size;
		}
#ifdef _DEBUG
		header_size += sizeof(ZoneDebugHeader);
		footer_size += sizeof(ZoneDebugFooter);
#endif
		int real_size = data_size + header_size + footer_size;
		
		// Update the stats
		s_Stats.m_SizeAlloc -= data_size;
		s_Stats.m_OverheadAlloc -= header_size + footer_size;
		s_Stats.m_SizesPerTag[tag] -= data_size;
		s_Stats.m_CountAlloc--;
		s_Stats.m_CountsPerTag[tag]--;
	
		// Delink block
		if (Z_IsTagLinked(tag))
		{
			ZoneLinkHeader* linked = (ZoneLinkHeader*)header - 1;
			
			if (linked == s_LinkBase)
			{
				s_LinkBase = linked->m_Next;
				if (s_LinkBase)
				{
					s_LinkBase->m_Prev = NULL;
				}
			}
			else
			{
				if (linked->m_Next)
				{
					linked->m_Next->m_Prev = linked->m_Prev;
				}
				linked->m_Prev->m_Next = linked->m_Next;
			}

			assert(Z_ValidateLinks());
		}

		// Clear the block header for safety
		*header = 0;

		// Add block to free list
		ZoneFreeBlock* nblock = NULL;
		if (real_size < sizeof(ZoneFreeBlock))
		{
			// Not enough space in block to put free information --
			// use overflow buffer.
			nblock = Z_GetOverflowBlock();

			if (nblock == NULL)
			{
				Z_Details_f();
				Com_Error(ERR_FATAL, "Zone free overflow buffer overflowed!");
			}
		}
		else
		{
			// Place free information in block
			nblock = (ZoneFreeBlock*)((char*)pvAddress - header_size);
		}

		nblock->m_Address = (unsigned int)pvAddress - header_size;
		nblock->m_Size = real_size;
		Z_LinkFreeBlock(nblock);

		// Coalesce any adjacent free blocks
		Z_Coalasce(nblock);
#ifndef _GAMECUBE
		ReleaseMutex(s_Mutex);
#endif
	}

	Z_FREE_RETURN( 0 );
}


int Z_MemSize(memtag_t eTag)
{
	return s_Stats.m_SizesPerTag[eTag];
}

#if ZONE_DEBUG
void Z_FindLeak(void)
{
	assert(s_Initialized);

	static int cycle_count = 0;
	const memtag_t tag = TAG_NEWDEL;

	struct PointerInfo
	{
		void* data;
		int counter;
		bool mark;
	};
	
	const int max_pointers = 32768;
	static PointerInfo pointers[max_pointers];
	static int num_pointers = 0;

	// Clear pointer existance
	for (int i = 0; i < num_pointers; ++i)
	{
		pointers[i].mark = false;
	}
	
	// Add all known pointers
	int start_num = num_pointers;
	for (ZoneLinkHeader* link = s_LinkBase; link;)
	{
		ZoneHeader* header = (ZoneHeader*)(link + 1);
		link = link->m_Next;
		
		if (Z_GetTag(header) == tag)
		{
			// See if the pointer already is in the array
			bool found = false;
			for (int k = start_num; k < num_pointers; ++k)
			{
				if (pointers[k].data == header)
				{
					++pointers[k].counter;
					pointers[k].mark = true;
					found = true;
					break;
				}
			}

			// If the pointer is not in the array, add it
			if (!found)
			{
				assert(num_pointers < max_pointers);
				pointers[num_pointers].data = header;
				pointers[num_pointers].counter = 0;
				pointers[num_pointers].mark = true;
				++num_pointers;
			}
		}
	}

	// Remove pointers that are no longer used
	for (int j = 0; j < num_pointers; ++j)
	{
		if (pointers[j].mark)
		{
			if (pointers[j].counter != cycle_count && 
				pointers[j].counter != cycle_count - 1 && 
				pointers[j].counter != 0)
			{
				Com_Printf("Memory leak: %p\n", pointers[j].data);
			}
		}
		else
		{
			int k;
			for (k = j; k < num_pointers; ++k)
			{
				if (pointers[k].mark) break;
			}

			if (k == num_pointers) break;

			memmove(pointers + j, pointers + k, (num_pointers - k) * sizeof(PointerInfo));
			num_pointers -= k - j;
		}
	}

	++cycle_count;
}
#endif

void Z_TagPointers(memtag_t eTag)
{
	assert(s_Initialized);

#ifndef _GAMECUBE
	WaitForSingleObject(s_Mutex, INFINITE);
#endif

	Sys_Log( "pointers.txt", va("Pointers for tag %d:\n", eTag) );

	for (ZoneLinkHeader* link = s_LinkBase; link;)
	{
		ZoneHeader* header = (ZoneHeader*)(link + 1);
		link = link->m_Next;

		if (eTag == TAG_ALL || Z_GetTag(header) == eTag)
		{
#ifdef _DEBUG
			Sys_Log( "pointers.txt",
					va("%x - %d\n", ((void*)((char*)header + 
							sizeof(ZoneHeader) + sizeof(ZoneDebugHeader))),
					Z_Size(((void*)((char*)header + 
							sizeof(ZoneHeader) + sizeof(ZoneDebugHeader))))));
#else
			Sys_Log( "pointers.txt",
					va("%x - %d\n", (void*)(header + 1),
					Z_Size((void*)(header + 1))));
#endif
		}
	}

#ifndef _GAMECUBE
	ReleaseMutex(s_Mutex);
#endif
}

void Z_TagFree(memtag_t eTag)
{
	assert(s_Initialized);

	for (ZoneLinkHeader* link = s_LinkBase; link;)
	{
		ZoneHeader* header = (ZoneHeader*)(link + 1);
		link = link->m_Next;

		if (eTag == TAG_ALL || Z_GetTag(header) == eTag)
		{
#ifdef _DEBUG
			Z_Free((void*)((char*)header + sizeof(ZoneHeader) + 
				sizeof(ZoneDebugHeader)));
#else
			Z_Free((void*)(header + 1));
#endif
		}
	}
}

void Z_SetNewDeleteTemporary(bool bTemp)
{
	if( bTemp )
		Z_PushNewDeleteTag( TAG_TEMP_WORKSPACE );
	else
		Z_PopNewDeleteTag();
}

void *S_Malloc( int iSize ) 
{
	return Z_Malloc(iSize, TAG_SMALL, qfalse, 0);
}

int Z_GetLevelMemory(void)
{
#ifdef _JK2MP
	return s_Stats.m_SizesPerTag[TAG_BSP];
#else
	return s_Stats.m_SizesPerTag[TAG_HUNKALLOC] +
//		s_Stats.m_SizesPerTag[TAG_HUNKMISCMODELS] +
		s_Stats.m_SizesPerTag[TAG_BSP];
#endif
}

#ifdef _JK2MP
int Z_GetHunkMemory(void)
{
	return s_Stats.m_SizesPerTag[TAG_HUNKALLOC] +
		s_Stats.m_SizesPerTag[TAG_TEMP_HUNKALLOC];
}
#endif

int Z_GetMiscMemory(void)
{
	return s_Stats.m_SizeAlloc -
		(Z_GetLevelMemory() +
#ifdef _JK2MP
		Z_GetHunkMemory() +
#endif
		s_Stats.m_SizesPerTag[TAG_MODEL_GLM] +
		s_Stats.m_SizesPerTag[TAG_MODEL_GLA] +
		s_Stats.m_SizesPerTag[TAG_MODEL_MD3] +
		s_Stats.m_SizesPerTag[TAG_BINK] +
		s_Stats.m_SizesPerTag[TAG_SND_RAWDATA]);
}

#ifdef _GAMECUBE
static int texMemSize = 0;
#else
extern int texMemSize;
//extern unsigned long texturePoint;
#endif
void Z_CompactStats(void)
{
	// New and improved, super version of CompactStats:
	assert(s_Initialized);

	static int printHeader = 1;
	if( printHeader )
	{
		printHeader = 0;
		Sys_Log("memory-map.txt", "Level:\tTextures:\tFreeZone:\tOverhead:\tTags...\n");
	}

	// No more being conservative and doing strange math. I want real numbers:
	Sys_Log("memory-map.txt", va("%s\t%d\t%d\t%d",
								 Cvar_VariableString( "mapname" ),
								 gTextures.Size(),
								 s_Stats.m_SizeFree,
								 s_Stats.m_OverheadAlloc));
	for( int t = 0; t < TAG_COUNT; ++t )
		Sys_Log("memory-map.txt", va("\t%d", s_Stats.m_SizesPerTag[t]));
	Sys_Log("memory-map.txt", "\n");
}


static void Z_Stats_f(void)
{
	assert(s_Initialized);
	// Display some memory usage summary information...

	Com_Printf("\nThe zone is using %d bytes (%.2fMB) in %d memory blocks\n", 
		s_Stats.m_SizeAlloc,
		(float)s_Stats.m_SizeAlloc / 1024.0f / 1024.0f, 
		s_Stats.m_CountAlloc);

	Com_Printf("Free memory is %d bytes (%.2fMB) in %d memory blocks\n", 
		s_Stats.m_SizeFree,
		(float)s_Stats.m_SizeFree / 1024.0f / 1024.0f, 
		s_Stats.m_CountFree);

	Com_Printf("The zone peaked at %d bytes (%.2fMB)\n", 
		s_Stats.m_PeakAlloc,
		(float)s_Stats.m_PeakAlloc / 1024.0f / 1024.0f);

	Com_Printf("The zone overhead is %d bytes (%.2fMB)\n", 
		s_Stats.m_OverheadAlloc,
		(float)s_Stats.m_OverheadAlloc / 1024.0f / 1024.0f);
}

void Z_Details_f(void)
{
	assert(s_Initialized);
	// Display some tag specific information...

	Com_Printf("---------------------------------------------------------------------------\n");
	Com_Printf("%20s %9s\n","Zone Tag","Bytes");
	Com_Printf("%20s %9s\n","--------","-----");
	for (int i=0; i<TAG_COUNT; i++)
	{
		int iThisCount = s_Stats.m_CountsPerTag[i];
		int iThisSize = s_Stats.m_SizesPerTag[i];

		if (iThisCount)
		{
			float	fSize		= (float)(iThisSize) / 1024.0f / 1024.0f;
			int		iSize		= fSize;
			int		iRemainder 	= 100.0f * (fSize - floor(fSize));
			Com_Printf("%d %9d (%2d.%02dMB) in %6d blocks (%9d average)\n", 
				i, iThisSize, iSize, iRemainder, iThisCount, iThisSize / iThisCount);
		}
	}
	Com_Printf("---------------------------------------------------------------------------\n");

	Z_Stats_f();
}

void Z_DumpMemMap_f(void)
{
#	define WRITECHAR(C) \
		Sys_Log("memmap.txt", C, 1, false);	\
		cur += 1024;	\
		if ((++counter) % 81 == 0) Sys_Log("memmap.txt", "\n", 1, false);
	
	unsigned int cur = (unsigned int)s_PoolBase;
	unsigned int counter = 0;
	for (ZoneFreeBlock* fblock = &s_FreeStart; fblock != &s_FreeEnd; fblock = fblock->m_Next)
	{
		while (fblock->m_Address > cur + 1024)
		{
			WRITECHAR("*");
		}

		if (fblock->m_Address > cur && fblock->m_Address < cur + 1024)
		{
			WRITECHAR("+");
		}

		while (fblock->m_Address + fblock->m_Size > cur + 1024)
		{
			WRITECHAR("-");
		}

		if (fblock->m_Address + fblock->m_Size > cur && 
			fblock->m_Address + fblock->m_Size < cur + 1024)
		{
			WRITECHAR("+");
		}
	}

	Sys_Log("memmap.txt", "\n");
}

void Z_DisplayLevelMemory(int size, int surf, int block)
{
	Z_DumpMemMap_f();

	//Yes, it should be divided by 1024, but I'm going for a safety margin
	//by rounding down.
	//Com_Printf("level memory used: %d KB\n", size / 1000);
	//Z_CompactStats(size, surf, block);
	Z_CompactStats();
}

void Z_DisplayLevelMemory(void)
{
#ifdef _GAMECUBE
	extern void R_SurfMramUsed(int &surface, int &block);
	int surface, block;
	R_SurfMramUsed(surface, block);
	Z_DisplayLevelMemory(Z_GetLevelMemory(), surface, block);
#else
	Z_DisplayLevelMemory(Z_GetLevelMemory(), 0, 0);
#endif
}


/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
char *CopyString( const char *in )
{
	struct ZoneSingleChar
	{
		ZoneHeader header;
#ifdef _DEBUG
		ZoneDebugHeader start;
#endif
		char data[2];
#ifdef _DEBUG
		ZoneDebugFooter end;
#endif
	};

#ifdef _DEBUG
	static ZoneSingleChar empty = {(TAG_STATIC << 25) | 2, ZONE_MAGIC, "\0", ZONE_MAGIC};
	static ZoneSingleChar numbers[10] =
	{
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "0", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "1", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "2", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "3", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "4", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "5", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "6", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "7", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "8", ZONE_MAGIC},
		{(TAG_STATIC << 25) | 2, ZONE_MAGIC, "9", ZONE_MAGIC},
	};
#else
	static ZoneSingleChar empty = {(TAG_STATIC << 25) | 2, "\0"};
	static ZoneSingleChar numbers[10] =
	{
		{(TAG_STATIC << 25) | 2, "0"},
		{(TAG_STATIC << 25) | 2, "1"},
		{(TAG_STATIC << 25) | 2, "2"},
		{(TAG_STATIC << 25) | 2, "3"},
		{(TAG_STATIC << 25) | 2, "4"},
		{(TAG_STATIC << 25) | 2, "5"},
		{(TAG_STATIC << 25) | 2, "6"},
		{(TAG_STATIC << 25) | 2, "7"},
		{(TAG_STATIC << 25) | 2, "8"},
		{(TAG_STATIC << 25) | 2, "9"},
	};
#endif

	char	*out;

	if (!in[0])
	{
		return empty.data;
	}
	else if (!in[1])
	{
		if (in[0] >= '0' && in[0] <= '9')
		{
			return numbers[in[0]-'0'].data;
		}
	}

	out = (char *) S_Malloc (strlen(in)+1);
	strcpy (out, in);

//	Z_Label(out,in);

	return out;
}

void Com_TouchMemory(void)
{
	// Stub function. Do nothing.
	return;
}


qboolean Z_IsFromZone(void *pvAddress, memtag_t eTag)
{
	if(pvAddress >= s_PoolBase && pvAddress < (char*)s_PoolBase + s_PoolSize) {
		return qtrue;
	}

	return qfalse;
}


qboolean Z_IsFromTempPool(void *pvAddress)
{
	if(pvAddress >= s_TempAllocPool && pvAddress < s_TempAllocPool +
			s_TempAllocPoint) {
		return qtrue;
	}

	return qfalse;
}


/*
   Hunk emulation

   The emulation is pretty bad right now, we just use two tags:
   TAG_HUNKALLOC and TAG_TEMP_HUNKALLOC, to represent the permanent and
   temporary sides of the hunk respectively. We should make the
   Hunk allocations tagged so we can do this better.
*/
#ifdef _JK2MP

void Hunk_Clear(void)
{
	Z_TagFree(TAG_TEMP_HUNKALLOC);
	Z_TagFree(TAG_HUNKALLOC);
/*
	Z_TagFree(TAG_HUNKALLOC);
	Z_TagFree(TAG_BSP_HUNK);
	Z_TagFree(TAG_BOT_HUNK);
	Z_TagFree(TAG_RENDERER_HUNK);
	Z_TagFree(TAG_SKELETON);
	Z_TagFree(TAG_MODEL_OTHER);
	Z_TagFree(TAG_MODEL_CHAR);
	VM_Clear();
*/
}


//void *Hunk_Alloc( int size, ha_pref preference, memtag_t eTag ) 
void *Hunk_Alloc(int size, ha_pref preference)
{
	return Z_Malloc(size, TAG_HUNKALLOC, qtrue);
/*
	assert(eTag == TAG_HUNKALLOC ||
			eTag == TAG_BSP_HUNK ||
			eTag == TAG_BOT_HUNK ||
			eTag == TAG_RENDERER_HUNK ||
			eTag == TAG_SKELETON ||
			eTag == TAG_MODEL_OTHER ||
			eTag == TAG_MODEL_CHAR);
	return Z_Malloc(size, eTag, qtrue);
*/
}


void *Hunk_AllocateTempMemory(int size)
{
	return Z_Malloc(size, TAG_TEMP_HUNKALLOC, qtrue);
/*
	return Z_Malloc(size, TAG_TEMP_HUNK, qtrue);
*/
}


void Hunk_FreeTempMemory(void *buf)
{
	Z_Free(buf);
}


void Hunk_ClearTempMemory(void)
{
	Z_TagFree(TAG_TEMP_HUNKALLOC);
//	Z_TagFree(TAG_TEMP_HUNK);
}


void Com_InitHunkMemory(void)
{
}


int Hunk_MemoryRemaining(void)
{
	return 0;
}


void Hunk_ClearToMark(void)
{
}


qboolean Hunk_CheckMark(void)
{
	return qfalse;
}


void Hunk_SetMark(void)
{
}
#endif // _JK2MP

/*
XBOXAPI
LPVOID
WINAPI
XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	return XMemAllocDefault(dwSize, dwAllocAttributes);
}
*/

/*
	XTL Replacement functions
	XMemAlloc
	XMemFree
	XMemSize

	Replacing these lets us intercept ALL memory allocation done by the XTL, and lets the
	Zone take pretty much all available memory at startup
*/
/* This still doesn't work. Numrous allocations still use internal functions, so there's
   little benefit right now.

XBOXAPI
LPVOID
WINAPI
XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	PXALLOC_ATTRIBUTES pAllocAttributes = (PXALLOC_ATTRIBUTES)&dwAllocAttributes;
	LPVOID ptr = NULL;

	if (pAllocAttributes->dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{ // Heap allocation
		ptr = HeapAlloc(GetProcessHeap(),
						pAllocAttributes->dwZeroInitialize ? HEAP_ZERO_MEMORY : 0,
						dwSize);
		if (pAllocAttributes->dwHeapTracksAttributes)
			XSetAttributesOnHeapAlloc(ptr, dwAllocAttributes);
	}
	else
	{ // Physical allocation
		// Map requested alignment to real alignment
		ULONG_PTR ulAlign = 0;
		DWORD dwProtect = 0;

		switch(pAllocAttributes->dwAlignment)
		{
			case XALLOC_PHYSICAL_ALIGNMENT_8K:
				ulAlign = 8*1024;
				break;

			case XALLOC_PHYSICAL_ALIGNMENT_16K:
				ulAlign = 16*1024;
				break;

			case XALLOC_PHYSICAL_ALIGNMENT_32K:
				ulAlign = 32*1024;
				break;

			default:
				ulAlign = 4*1024;
				break;
		}

		if (pAllocAttributes->dwMemoryProtect & XALLOC_MEMPROTECT_READONLY)
			dwProtect = PAGE_READONLY;
		else
			dwProtect = PAGE_READWRITE;

		if (pAllocAttributes->dwMemoryProtect & XALLOC_MEMPROTECT_NOCACHE)
			dwProtect |= PAGE_NOCACHE;
		if (pAllocAttributes->dwMemoryProtect & XALLOC_MEMPROTECT_WRITECOMBINE)
			dwProtect |= PAGE_WRITECOMBINE;

		ptr = XPhysicalAlloc(dwSize, MAXULONG_PTR, ulAlign, dwProtect);
	}

	return ptr;
}
*/

/*
XBOXAPI
VOID
WINAPI
XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	XMemFreeDefault(pAddress, dwAllocAttributes);
}
*/

/*
XBOXAPI
VOID
WINAPI
XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	PXALLOC_ATTRIBUTES pAllocAttributes = (PXALLOC_ATTRIBUTES)&dwAllocAttributes;

	if (pAllocAttributes->dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{ // Heap pointer
		HeapFree(GetProcessHeap(), 0, pAddress);
	}
	else
	{ // Physical pointer
		XPhysicalFree(pAddress);
	}
}
*/

/*
XBOXAPI
SIZE_T
WINAPI
XMemSize(PVOID pAddress, DWORD dwAllocAttributes)
{
	return XMemSizeDefault(pAddress, dwAllocAttributes);
}
*/

/*
XBOXAPI
SIZE_T
WINAPI
XMemSize(PVOID pAddress, DWORD dwAllocAttributes)
{
	PXALLOC_ATTRIBUTES pAllocAttributes = (PXALLOC_ATTRIBUTES)&dwAllocAttributes;

	if (pAllocAttributes->dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{ // Heap pointer
		return HeapSize(GetProcessHeap(), 0, pAddress);
	}
	else
	{ // Physical pointer
		return XPhysicalSize(pAddress);
	}
}
*/
