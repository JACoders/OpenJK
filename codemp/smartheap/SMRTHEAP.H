/* smrtheap.h -- SmartHeap (tm) public C header file
 * Professional Memory Management Library
 *
 * Copyright (C) 1991-1999 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the author.
 *
 */

#if !defined(_SMARTHEAP_H)
#define _SMARTHEAP_H

#include <limits.h>
#include <stddef.h>

#if !defined(macintosh) && !defined(THINK_C) && !defined(__MWERKS__) \
   && !defined(SHANSI) && UINT_MAX == 0xFFFFu \
   && (defined(_Windows) || defined(_WINDOWS) || defined(__WINDOWS__))
   #define MEM_WIN16
#endif

#if (UINT_MAX == 0xFFFFu) && (defined(MEM_WIN16) \
	|| defined(MSDOS) || defined(__MSDOS__) || defined(__DOS__))
   /* 16-bit X86 */
   #if defined(SYS_DLL)
      #if defined(_MSC_VER) && _MSC_VER <= 600
         #define MEM_ENTRY _export _loadds far pascal
      #else
         #define MEM_ENTRY _export far pascal
      #endif
   #else
      #define MEM_ENTRY far pascal
   #endif
   #ifdef __WATCOMC__
      #define MEM_ENTRY_ANSI __far
   #else
      #define MEM_ENTRY_ANSI far cdecl
   #endif
   #define MEM_FAR far
   #if defined(MEM_WIN16)
      #define MEM_ENTRY2 _export far pascal
   #elif defined(DOS16M) || defined(DOSX286)
      #define MEM_ENTRY2 _export _loadds far pascal
   #endif

#else  /* not 16-bit X86 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) \
    || defined(__WIN32__) || defined(__NT__)
   #define MEM_WIN32
   #if defined(_MSC_VER)
      #if defined(_SHI_Pool) && defined(SYS_DLL)
         #define MEM_ENTRY1 __declspec(dllexport)
         #define MEM_ENTRY4 __declspec(dllexport) extern
      #elif !defined(_SHI_Pool) && (defined(MEM_DEBUG) || defined(MEM_DLL))
         #define MEM_ENTRY1 __declspec(dllimport)
         #if defined(_M_IX86) || defined(_X86_)
            #define MemDefaultPool shi_MemDefaultPool
            #define MEM_ENTRY4 __declspec(dllimport)
         #endif
      #endif
   #endif
   #if (defined(_MT) || defined(__MT__)) && !defined(MEM_DEBUG)
/* @@@     #define MEM_MT 1 */
   #endif
   #if !defined(_MSC_VER) || defined(_M_IX86) || defined(_X86_)
      #define MEM_ENTRY __stdcall
   #else
      #define MEM_ENTRY __cdecl  /* for NT/RISC */
   #endif
   #ifndef __WATCOMC__
      #define MEM_ENTRY_ANSI __cdecl
   #endif

#elif defined(__OS2__)
   #if defined(__BORLANDC__) || defined(__WATCOMC__)
      #if defined(SYS_DLL)
         #define MEM_ENTRY __export __syscall
      #else
         #define MEM_ENTRY __syscall
      #endif /* SYS_DLL */
      #ifdef __BORLANDC__
         #define MEM_ENTRY_ANSI __stdcall
      #endif
   #elif defined(__IBMC__) || defined(__IBMCPP__)
      #if defined(SYS_DLL) && 0
         #define MEM_ENTRY _Export _System
      #else
         #define MEM_ENTRY _System
      #endif
      #define MEM_ENTRY_ANSI _Optlink
      #define MEM_ENTRY3 MEM_ENTRY
      #define MEM_CALLBACK MEM_ENTRY3
      #define MEM_ENTRY2
   #endif

#elif defined(__sun) || defined(__hpux) || defined(__osf__) || defined(sgi)
   #if defined(_REENTRANT) && !defined(MEM_DEBUG)
/* @@@   #define MEM_MT 1 */
   #endif

#elif defined(_AIX)
   #if defined(_THREAD_SAFE) && !defined(MEM_DEBUG)
/*   #define MEM_MT 1 */
   #endif

#endif /* WIN32, OS2, UNIX */

#if defined(__WATCOMC__) && defined(__SW_3S)
   /* Watcom stack calling convention */
#ifndef __OS2__
#ifdef __WINDOWS_386__
   #pragma aux syscall "*_" parm routine [eax ebx ecx edx fs gs] modify [eax];
#else
   #pragma aux syscall "*_" parm routine [eax ebx ecx edx] modify [eax];
#endif
#ifndef MEM_ENTRY
   #define MEM_ENTRY __syscall
#endif /* MEM_ENTRY */
#endif
#endif /* Watcom stack calling convention */

#endif /* end of system-specific declarations */

#ifndef MEM_ENTRY
   #define MEM_ENTRY
#endif
#ifndef MEM_ENTRY1
   #define MEM_ENTRY1
#endif
#ifndef MEM_ENTRY2
   #define MEM_ENTRY2 MEM_ENTRY
#endif
#ifndef MEM_ENTRY3
   #define MEM_ENTRY3
#endif
#ifndef MEM_ENTRY4
   #define MEM_ENTRY4 extern
#endif
#ifndef MEM_CALLBACK
#define MEM_CALLBACK MEM_ENTRY2
#endif
#ifndef MEM_ENTRY_ANSI
   #define MEM_ENTRY_ANSI
#endif
#ifndef MEM_FAR
   #define MEM_FAR
#endif

#ifdef applec
/* Macintosh: Apple MPW C/C++ passes char/short parms as longs (4 bytes),
 * whereas Symantec C/C++ for MPW passes these as words (2 bytes);
 * therefore, canonicalize all integer parms as 'int' for this platform.
 */
   #define MEM_USHORT unsigned
   #define MEM_UCHAR unsigned
#else
   #define MEM_USHORT unsigned short
   #define MEM_UCHAR unsigned char
#endif /* applec */

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(MEM_DEBUG) || !(defined(MEM_WIN16) || defined(MEM_WIN32))
#define SHI_MAJOR_VERSION 5
#define SHI_MINOR_VERSION 0
#define SHI_UPDATE_LEVEL 0
#endif /* !(MEM_WIN16 || MEM_WIN32) */

	 
/*** Types ***/

typedef int MEM_BOOL;

/* Version Masks */
typedef unsigned MEM_VERSION;
#define MEM_MAJOR_VERSION(v) (((v) & 0xF000u) >> 12)
#define MEM_MINOR_VERSION(v) (((v) & 0x0F00u) >> 8)
#define MEM_UPDATE_VERSION(v) ((v) & 0x00FFu)

/* Note: these types are struct's rather than integral types to facilitate
 * compile-time type-checking.  MEM_POOL and MEM_HANDLE should be regarded
 * as black boxes, and treated just like handles.
 * You should not have any type casts to or from MEM_POOL or MEM_HANDLE;
 * nor should you dereference variables of type MEM_POOL or MEM_HANDLE
 * (unless you are using SmartHeap to replace NewHandle on the Mac, and
 * you have existing code that dereferences handles).
 */
#ifdef _SHI_Pool
  typedef struct _SHI_Pool MEM_FAR *MEM_POOL;
  typedef struct _SHI_MovHandle MEM_FAR *MEM_HANDLE;
#else
  #ifdef THINK_C
    typedef void *MEM_POOL;
    typedef void **MEM_HANDLE;
  #else
    typedef struct _SHI_Pool { int reserved; } MEM_FAR *MEM_POOL;
    typedef struct _SHI_MovHandle { int reserved; } MEM_FAR *MEM_HANDLE;
  #endif
#endif
    

/* Error codes: errorCode field of MEM_ERROR_INFO */
typedef enum
{
   MEM_NO_ERROR=0,
   MEM_INTERNAL_ERROR,
   MEM_OUT_OF_MEMORY,
   MEM_BLOCK_TOO_BIG,
   MEM_ALLOC_ZERO,
   MEM_RESIZE_FAILED,
   MEM_LOCK_ERROR,
   MEM_EXCEEDED_CEILING,
   MEM_TOO_MANY_PAGES,
   MEM_TOO_MANY_TASKS,
   MEM_BAD_MEM_POOL,
   MEM_BAD_BLOCK,
   MEM_BAD_FREE_BLOCK,
   MEM_BAD_HANDLE,
   MEM_BAD_POINTER,
   MEM_WRONG_TASK,
   MEM_NOT_FIXED_SIZE,
   MEM_BAD_FLAGS,
#ifdef MEM_DEBUG
   MEM_BAD_BUFFER,
   MEM_DOUBLE_FREE,
   MEM_UNDERWRITE,
   MEM_OVERWRITE, 
   MEM_FREE_BLOCK_WRITE,
   MEM_READONLY_MODIFIED,
   MEM_NOFREE,
   MEM_NOREALLOC,
   MEM_LEAKAGE,
   MEM_FREE_BLOCK_READ,
   MEM_UNINITIALIZED_READ,
   MEM_UNINITIALIZED_WRITE,
   MEM_OUT_OF_BOUNDS_READ,
	MEM_UNDERWRITE_STACK,
	MEM_OVERWRITE_STACK,
	MEM_FREE_STACK_READ,
	MEM_UNINITIALIZED_READ_STACK,
	MEM_UNINITIALIZED_WRITE_STACK,
	MEM_OUT_OF_BOUNDS_READ_STACK,
   MEM_LASTOK,
   MEM_BREAKPOINT,
   MEM_ERROR_CODE_COUNT,
#endif /* MEM_DEBUG */
   MEM_ERROR_CODE_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_ERROR_CODE;

/* HeapAgent Entry-Point API identifiers: errorAPI field of MEM_ERROR_INFO */
typedef enum
{
   MEM_NO_API,
   MEM_MEMVERSION,
   MEM_MEMREGISTERTASK,
   MEM_MEMUNREGISTERTASK,
   MEM_MEMPOOLINIT,
   MEM_MEMPOOLINITFS,
   MEM_MEMPOOLFREE,
   MEM_MEMPOOLSETPAGESIZE,
   MEM_MEMPOOLSETBLOCKSIZEFS,
   MEM_MEMPOOLSETFLOOR,
   MEM_MEMPOOLSETCEILING,
   MEM_MEMPOOLPREALLOCATE,
   MEM_MEMPOOLPREALLOCATEHANDLES,
   MEM_MEMPOOLSHRINK,
   MEM_MEMPOOLSIZE,
   MEM_MEMPOOLCOUNT,
   MEM_MEMPOOLINFO,
   MEM_MEMPOOLFIRST,
   MEM_MEMPOOLNEXT,
   MEM_MEMPOOLWALK,
   MEM_MEMPOOLCHECK,
   MEM_MEMALLOC,
   MEM_MEMREALLOC,
   MEM_MEMFREE,
   MEM_MEMLOCK,
   MEM_MEMUNLOCK,
   MEM_MEMFIX,
   MEM_MEMUNFIX,
   MEM_MEMLOCKCOUNT,
   MEM_MEMISMOVEABLE,
   MEM_MEMREFERENCE,
   MEM_MEMHANDLE,
   MEM_MEMSIZE,
   MEM_MEMALLOCPTR,
   MEM_MEMREALLOCPTR,
   MEM_MEMFREEPTR,
   MEM_MEMSIZEPTR,
   MEM_MEMCHECKPTR,
   MEM_MEMALLOCFS,
   MEM_MEMFREEFS,
   MEM_MEM_MALLOC,
   MEM_MEM_CALLOC,
   MEM_MEM_REALLOC,
   MEM_MEM_FREE,
   MEM_NEW,
   MEM_DELETE,
   MEM_DBGMEMPOOLSETCHECKFREQUENCY,
   MEM_DBGMEMPOOLDEFERFREEING,
   MEM_DBGMEMPOOLFREEDEFERRED,
   MEM_DBGMEMPROTECTPTR,
   MEM_DBGMEMREPORTLEAKAGE,
   MEM_MEMPOOLINITNAMEDSHARED,
   MEM_MEMPOOLINITNAMEDSHAREDEX,
   MEM_MEMPOOLATTACHSHARED,
   MEM_DBGMEMPOOLINFO,
   MEM_DBGMEMPTRINFO,
   MEM_DBGMEMSETTINGSINFO,
   MEM_DBGMEMCHECKPTR,
   MEM_DBGMEMPOOLSETNAME,
   MEM_DBGMEMPOOLSETDEFERQUEUELEN,
   MEM_DBGMEMFREEDEFERRED,
   MEM_DBGMEMCHECKALL,
   MEM_DBGMEMBREAKPOINT,
   MEM_MEMPOOLLOCK,
   MEM_MEMPOOLUNLOCK,
	MEM_MEMPOOLSETSMALLBLOCKSIZE,
   MEM_MEMSIZEREQUESTED,
	MEM_MSIZE,
	MEM_EXPAND,
	MEM_GETPROCESSHEAP,
	MEM_GETPROCESSHEAPS,
	MEM_GLOBALALLOC,
	MEM_GLOBALFLAGS,
	MEM_GLOBALFREE,
	MEM_GLOBALHANDLE,
	MEM_GLOBALLOCK,
	MEM_GLOBALREALLOC,
	MEM_GLOBALSIZE,
	MEM_GLOBALUNLOCK,
	MEM_HEAPALLOC,
	MEM_HEAPCOMPACT,
	MEM_HEAPCREATE,
	MEM_HEAPDESTROY,
	MEM_HEAPFREE,
	MEM_HEAPLOCK,
	MEM_HEAPREALLOC,
	MEM_HEAPSIZE,
	MEM_HEAPUNLOCK,
	MEM_HEAPVALIDATE,
	MEM_HEAPWALK,
	MEM_LOCALALLOC,
	MEM_LOCALFLAGS,
	MEM_LOCALFREE,
	MEM_LOCALHANDLE,
	MEM_LOCALLOCK,
	MEM_LOCALREALLOC,
	MEM_LOCALSIZE,
	MEM_LOCALUNLOCK,
	MEM_MEMPOOLINITREGION,
	MEM_TERMINATE,
   MEM_HEAPAGENT,
   MEM_USER_API,
   MEM_API_COUNT,
   MEM_API_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_API;

#define MEM_MAXCALLSTACK 16  /* maximum number of call stack frames recorded */

/* Error info, passed to error-handling callback routine */
typedef struct _MEM_ERROR_INFO
{
   MEM_ERROR_CODE errorCode; /* error code identifying type of error      */
   MEM_POOL pool;            /* pool in which error occurred, if known    */

/* all fields below this are valid only for debugging lib                 */
   /* the following seven fields identify the call where error detected   */
   MEM_API errorAPI;         /* fn ID of entry-point where error detected */
   MEM_POOL argPool;         /* memory pool parameter, if applicable      */
   void MEM_FAR *argPtr;     /* memory pointer parameter, if applicable   */
   void MEM_FAR *argBuf;     /* result buffer parameter, if applicable    */
   MEM_HANDLE argHandle;     /* memory handle parameter, if applicable    */
   unsigned long argSize;    /* size parameter, if applicable             */
   unsigned long argCount;   /* count parameter, if applicable            */
   unsigned argFlags;        /* flags parameter, if applicable            */

   /* the following two fields identify the app source file and line      */
   const char MEM_FAR *file; /* app source file containing above call     */
   int line;            /* source line in above file                 */

   /* the following two fields identify call instance of error detection  */
   unsigned long allocCount; /* enumeration of allocation since 1st alloc */
   unsigned long passCount;  /* enumeration of call at at above file/line */
   unsigned checkpoint;      /* group with which call has been tagged     */
   
   /* the following fields, if non-NULL, points to the address where an
      overwrite was detected and another MEM_ERROR_INFO structure
      identifying where the corrupted object was first created, if known  */
   void MEM_FAR *errorAlloc;  /* ptr to beginning of alloc related to error */
   void MEM_FAR *corruptAddr;
   struct _MEM_ERROR_INFO MEM_FAR *objectCreationInfo;

   unsigned long threadID;    /* ID of thread where error detected */
   unsigned long pid;         /* ID of process where error detected */

	void MEM_FAR *callStack[MEM_MAXCALLSTACK];
} MEM_ERROR_INFO;

/* Error handling callback function */
typedef MEM_BOOL (MEM_ENTRY2 * MEM_ENTRY3 MEM_ERROR_FN)
   (MEM_ERROR_INFO MEM_FAR *);


/* Block Type: field of MEM_POOL_ENTRY, field of MEM_POOL_INFO,
 * parameter to MemPoolPreAllocate
 */
typedef enum
{
   MEM_FS_BLOCK               = 0x0001u,
   MEM_VAR_MOVEABLE_BLOCK     = 0x0002u,
   MEM_VAR_FIXED_BLOCK        = 0x0004u,
   MEM_EXTERNAL_BLOCK         = 0x0008u,
   MEM_BLOCK_TYPE_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_BLOCK_TYPE;

typedef enum
{
	MEM_SMALL_BLOCK_NONE,
	MEM_SMALL_BLOCK_SH3,
	MEM_SMALL_BLOCK_SH5,
   MEM_SMALL_BLOCK_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_SMALL_BLOCK_ALLOCATOR;

/* Pool Entry: parameter to MemPoolWalk */
typedef struct
{
   void MEM_FAR *entry;
   MEM_POOL pool;
   MEM_BLOCK_TYPE type;
   MEM_BOOL isInUse;
   unsigned long size;
   MEM_HANDLE handle;
   unsigned lockCount;
   void MEM_FAR *reserved_ptr;
} MEM_POOL_ENTRY;

/* Pool Status: returned by MemPoolWalk, MemPoolFirst, MemPoolNext */
typedef enum
{
   MEM_POOL_OK            = 1,
   MEM_POOL_CORRUPT       = -1,
   MEM_POOL_CORRUPT_FATAL = -2,
   MEM_POOL_END           = 0,
   MEM_POOL_STATUS_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_POOL_STATUS;

/* Pointer Status: returned by MemCheckPtr */
typedef enum
{
   MEM_POINTER_OK    = 1,
   MEM_POINTER_WILD  = 0,
   MEM_POINTER_FREE  = -1,
   MEM_POINTER_STATUS_INT_MAX = INT_MAX /* to ensure enum is full int */
} MEM_POINTER_STATUS;

/* Pool Info: parameter to MemPoolInfo, MemPoolFirst, MemPoolNext */
typedef struct
{
   MEM_POOL pool;
   MEM_BLOCK_TYPE type; /* disjunctive combination of block type flags */
	unsigned short blockSizeFS;
	unsigned short smallBlockSize;
   unsigned pageSize;
   unsigned long floor;
   unsigned long ceiling;
   unsigned flags;
   MEM_ERROR_FN errorFn;
} MEM_POOL_INFO;

/* Flags passed to MemAlloc, MemAllocPtr, MemReAlloc, MemReAllocPtr */
#define MEM_FIXED           0x0000u /* fixed handle-based block            */
#define MEM_ZEROINIT        0x0001u /* == TRUE for SH 1.5 compatibility    */
#define MEM_MOVEABLE        0x0002u /* moveable handle-based block         */
#define MEM_RESIZEABLE      0x0004u /* reserve space above block           */
#define MEM_RESIZE_IN_PLACE 0x0008u /* do not move block (realloc)         */
#define MEM_NOGROW          0x0010u /* do not grow heap to satisfy request */
#define MEM_NOEXTERNAL      0x0020u /* reserved for internal use           */
#define MEM_NOCOMPACT       0x0040u /* do not compact to satisfy request   */
#define MEM_NO_SERIALIZE    0x0080u /* do not serialize this request       */
#define MEM_HANDLEBASED     0x4000u /* for internal use */
#define MEM_RESERVED        0x8000u /* for internal use */

#define MEM_UNLOCK_FAILED USHRT_MAX

/* Flags passed to MemPoolInit, MemPoolInitFS */
#define MEM_POOL_SHARED       0x0001u /* == TRUE for SH 1.5 compatibility  */
#define MEM_POOL_SERIALIZE    0x0002u /* pool used in more than one thread */
#define MEM_POOL_VIRTUAL_LOCK 0x0004u /* pool is locked in physical memory */
#define MEM_POOL_ZEROINIT     0x0008u /* malloc/new from pool zero-inits   */
#define MEM_POOL_REGION       0x0010u /* store pool in user-supplied region*/
#define MEM_POOL_DEFAULT      0x8000u /* pool with default characteristics */

/* Default memory pool for C malloc, C++ new (for backwards compatibility) */
#define MEM_DEFAULT_POOL MemDefaultPool

/* define and initialize these variables at file scope to change defaults */
extern unsigned short MemDefaultPoolBlockSizeFS;
extern unsigned MemDefaultPoolPageSize;
extern unsigned MemDefaultPoolFlags;

/* define SmartHeap_malloc at file scope if you
 * are intentionally _NOT_ linking in the SmartHeap malloc definition
 * ditto for SmartHeap operator new, and fmalloc et al.
 */
extern int SmartHeap_malloc;
extern int SmartHeap_far_malloc;
extern int SmartHeap_new;

#define MEM_ERROR_RET ULONG_MAX

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef MEM_DEBUG
#include "heapagnt.h"
#endif

#endif /* !defined(_SMARTHEAP_H) */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEM_MT
#ifdef MemDefaultPool
#undef MemDefaultPool
#endif
#define MemDefaultPool shi_getThreadPool()
#ifndef MemInitDefaultPool
#define MemInitDefaultPool() shi_getThreadPool()
#define MemFreeDefaultPool() shi_freeThreadPools()
#endif
MEM_ENTRY1 MEM_POOL MEM_ENTRY shi_getThreadPool(void);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY shi_freeThreadPools(void);

#else /* MEM_MT */
MEM_ENTRY4 MEM_POOL MemDefaultPool;
MEM_POOL MEM_ENTRY MemInitDefaultPool(void);
MEM_BOOL MEM_ENTRY MemFreeDefaultPool(void);
#endif /* MEM_MT */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*** Function Prototypes ***/

#ifndef _SMARTHEAP_PROT
#define _SMARTHEAP_PROT

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _shAPI
   #if defined(MEM_DEBUG) && !defined(SHI_NO_MEM_DEBUG)
      #define _shAPI(ret, name) MEM_ENTRY1 ret MEM_ENTRY _dbg ## name
   #else
      #define _shAPI(ret, name) MEM_ENTRY1 ret MEM_ENTRY name
   #endif
#endif

#ifndef _dbgARGS
   #if defined(MEM_DEBUG) && !defined(SHI_NO_MEM_DEBUG)
      #define _dbgARGS1 const char MEM_FAR *, int
      #define _dbgARGS , _dbgARGS1
   #else
      #define _dbgARGS1 void
      #define _dbgARGS
   #endif
#endif

   
/**** HOW TO READ SmartHeap PROTOTYPES ****
 * prototypes below have the follow syntax in order to support both debug
 * and non-debug APIs with single-source:
 *
 * _shiAPI(<return-type>, <API name>)([<parms>] _dbgARGS);
 *
 * the above translates to a C prototype as follows:
 *
 * <return-type> <API name>([<parms>]);
 */
   
/* Library Version */
MEM_ENTRY1 MEM_VERSION MEM_ENTRY MemVersion(void);

/* Library Registration */
_shAPI(MEM_BOOL, MemRegisterTask)(_dbgARGS1);
_shAPI(MEM_BOOL, MemUnregisterTask)(_dbgARGS1);

/* Process heap usage */
MEM_ENTRY1 void MEM_ENTRY MemProcessSetGrowIncrement(unsigned long);

/* Memory Pool Functions */
_shAPI(MEM_POOL, MemPoolInit)(unsigned _dbgARGS);
_shAPI(MEM_POOL, MemPoolInitFS)(MEM_USHORT, unsigned long, 
   unsigned _dbgARGS);
_shAPI(MEM_POOL, MemPoolInitRegion)(void MEM_FAR *,
												unsigned long size, unsigned _dbgARGS);
_shAPI(MEM_POOL, MemPoolInitRegionEx)(void MEM_FAR *addr,
	unsigned long size, unsigned flags, void MEM_FAR *security _dbgARGS);
_shAPI(MEM_POOL, MemPoolInitNamedShared)(const char MEM_FAR *,
                                         unsigned long size,unsigned _dbgARGS);
_shAPI(MEM_POOL, MemPoolInitNamedSharedEx)(void MEM_FAR *addr,
   unsigned pidCount, unsigned long MEM_FAR *pids, void MEM_FAR *security,
   const char MEM_FAR *name, unsigned long size, unsigned flags _dbgARGS);
_shAPI(MEM_POOL, MemPoolAttachShared)(MEM_POOL, const char MEM_FAR * _dbgARGS);
_shAPI(MEM_BOOL, MemPoolFree)(MEM_POOL _dbgARGS);
_shAPI(unsigned, MemPoolSetPageSize)(MEM_POOL, unsigned _dbgARGS);
_shAPI(MEM_BOOL, MemPoolSetBlockSizeFS)(MEM_POOL, MEM_USHORT _dbgARGS);
MEM_ENTRY1 unsigned long MEM_ENTRY MemPoolSetGrowIncrement(MEM_POOL,
	unsigned long);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY MemPoolSetSmallBlockAllocator(MEM_POOL, MEM_SMALL_BLOCK_ALLOCATOR);
_shAPI(MEM_BOOL, MemPoolSetSmallBlockSize)(MEM_POOL, MEM_USHORT _dbgARGS);
_shAPI(unsigned long, MemPoolSetFloor)(MEM_POOL, unsigned long _dbgARGS);
_shAPI(unsigned long, MemPoolSetCeiling)(MEM_POOL, unsigned long _dbgARGS);
_shAPI(unsigned long, MemPoolPreAllocate)(MEM_POOL, unsigned long, 
   MEM_BLOCK_TYPE _dbgARGS);
_shAPI(unsigned long, MemPoolPreAllocateHandles)(MEM_POOL,
   unsigned long _dbgARGS);
_shAPI(unsigned long, MemPoolShrink)(MEM_POOL _dbgARGS);
_shAPI(unsigned long, MemPoolSize)(MEM_POOL _dbgARGS);
_shAPI(unsigned long, MemPoolCount)(MEM_POOL _dbgARGS);
_shAPI(MEM_BOOL, MemPoolInfo)(MEM_POOL, void MEM_FAR *, 
   MEM_POOL_INFO MEM_FAR* _dbgARGS);
_shAPI(MEM_POOL_STATUS, MemPoolFirst)(MEM_POOL_INFO MEM_FAR *,
   MEM_BOOL _dbgARGS);
_shAPI(MEM_POOL_STATUS,MemPoolNext)(MEM_POOL_INFO MEM_FAR*,MEM_BOOL _dbgARGS);
_shAPI(MEM_POOL_STATUS,MemPoolWalk)(MEM_POOL,MEM_POOL_ENTRY MEM_FAR*_dbgARGS);
_shAPI(MEM_BOOL, MemPoolCheck)(MEM_POOL _dbgARGS);
_shAPI(MEM_BOOL, MemPoolLock)(MEM_POOL _dbgARGS);
_shAPI(MEM_BOOL, MemPoolUnlock)(MEM_POOL _dbgARGS);

/* Handle-based API for moveable memory within heap. */
_shAPI(MEM_HANDLE, MemAlloc)(MEM_POOL, unsigned, unsigned long _dbgARGS);
_shAPI(MEM_HANDLE, MemReAlloc)(MEM_HANDLE,unsigned long,unsigned _dbgARGS);
_shAPI(MEM_BOOL, MemFree)(MEM_HANDLE _dbgARGS);
_shAPI(void MEM_FAR *, MemLock)(MEM_HANDLE _dbgARGS);
_shAPI(unsigned, MemUnlock)(MEM_HANDLE _dbgARGS);
_shAPI(void MEM_FAR *, MemFix)(MEM_HANDLE _dbgARGS);
_shAPI(unsigned, MemUnfix)(MEM_HANDLE _dbgARGS);
_shAPI(unsigned, MemLockCount)(MEM_HANDLE _dbgARGS);
#ifndef MemFlags
#define MemFlags(mem) MemLockCount(mem)
#endif
_shAPI(MEM_BOOL, MemIsMoveable)(MEM_HANDLE _dbgARGS);
_shAPI(unsigned long, MemSize)(MEM_HANDLE _dbgARGS);
_shAPI(unsigned long, MemSizeRequested)(MEM_HANDLE _dbgARGS);
_shAPI(MEM_HANDLE, MemHandle)(void MEM_FAR * _dbgARGS);
#ifndef MEM_REFERENCE
   #ifdef MEM_DEBUG
      MEM_ENTRY1 void MEM_FAR * MEM_ENTRY _dbgMemReference(MEM_HANDLE, 
        const char MEM_FAR *, int);
      #define MEM_REFERENCE(handle) \
         _dbgMemReference(handle, __FILE__, __LINE__)
   #else
      #define MEM_REFERENCE(handle) (*(void MEM_FAR * MEM_FAR *)handle)
   #endif
#endif

/* General Heap Allocator (returns direct pointer to memory) */
_shAPI(void MEM_FAR*,MemAllocPtr)(MEM_POOL,unsigned long,unsigned _dbgARGS);
_shAPI(void MEM_FAR *, MemReAllocPtr)(void MEM_FAR *, unsigned long,
   unsigned _dbgARGS);
_shAPI(MEM_BOOL, MemFreePtr)(void MEM_FAR * _dbgARGS);
_shAPI(unsigned long, MemSizePtr)(void MEM_FAR * _dbgARGS);
_shAPI(MEM_POINTER_STATUS, MemCheckPtr)(MEM_POOL, void MEM_FAR * _dbgARGS);

/* Fixed-Size Allocator */
_shAPI(void MEM_FAR *, MemAllocFS)(MEM_POOL _dbgARGS);
_shAPI(MEM_BOOL, MemFreeFS)(void MEM_FAR * _dbgARGS);

/* Error Handling Functions */
MEM_ENTRY1 MEM_ERROR_FN MEM_ENTRY MemSetErrorHandler(MEM_ERROR_FN);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY MemDefaultErrorHandler(MEM_ERROR_INFO MEM_FAR*);
MEM_ENTRY1 void MEM_ENTRY MemErrorUnwind(void);

#ifdef MEM_WIN32
/* patching control */

#ifndef MEM_PATCHING_DEFINED
#define MEM_PATCHING_DEFINED
typedef enum
{
	MEM_PATCH_ALL = 0,
	MEM_SKIP_PATCHING_THIS_DLL = 1,
	MEM_DISABLE_SYSTEM_HEAP_PATCHING = 2,
	MEM_DISABLE_ALL_PATCHING = 4|2|1,
   MEM_PATCHING_INT_MAX = INT_MAX /* to ensure enum is full int in size */
} MEM_PATCHING;
#endif /* MEM_PATCHING_DEFINED */

#ifdef _MSC_VER
__declspec(dllexport)
#endif
MEM_PATCHING MEM_ENTRY MemSetPatching(const char ***skipDLLs);

#endif /* MEM_WIN32 */

/* Internal routines */
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _shi_enterCriticalSection(void);
MEM_ENTRY1 void MEM_ENTRY _shi_leaveCriticalSection(void);
MEM_BOOL shi_call_new_handler_msc(size_t, MEM_BOOL);


/* Wrapper macros for debugging API */
#ifndef _SHI_dbgMacros
#ifdef MEM_DEBUG
#define MemRegisterTask()        _dbgMemRegisterTask(__FILE__, __LINE__)
#define MemUnregisterTask()      _dbgMemUnregisterTask(__FILE__, __LINE__)
#define MemPoolInit(flags)       _dbgMemPoolInit(flags, __FILE__, __LINE__)
#define MemPoolInitFS(bs, bc, f) _dbgMemPoolInitFS(bs,bc,f,__FILE__,__LINE__)
#define MemPoolInitRegion(addr, sz, f) \
     _dbgMemPoolInitRegion(addr, sz, f, __FILE__, __LINE__)
#define MemPoolInitRegionEx(addr, sz, f) \
     _dbgMemPoolInitRegionEx(addr, sz, f, s, __FILE__, __LINE__)
#define MemPoolInitNamedShared(nm, sz, f) \
     _dbgMemPoolInitNamedShared(nm, sz, f, __FILE__, __LINE__)
#define MemPoolInitNamedSharedEx(a, c, p, sec, nm, sz, f) \
     _dbgMemPoolInitNamedSharedEx(a, c, p, sec, nm, sz, f, __FILE__, __LINE__)
#define MemPoolAttachShared(p, n) \
     _dbgMemPoolAttachShared(p, n, __FILE__, __LINE__)
#define MemPoolFree(pool)        _dbgMemPoolFree(pool, __FILE__, __LINE__)
#define MemPoolSetPageSize(p, s) _dbgMemPoolSetPageSize(p,s,__FILE__,__LINE__)
#define MemPoolSetBlockSizeFS(p, s) \
     _dbgMemPoolSetBlockSizeFS(p, s, __FILE__, __LINE__)
#define MemPoolSetSmallBlockSize(p, s) \
     _dbgMemPoolSetSmallBlockSize(p, s, __FILE__, __LINE__)
#define MemPoolSetFloor(p, f)    _dbgMemPoolSetFloor(p, f, __FILE__, __LINE__)
#define MemPoolSetCeiling(p, c)  _dbgMemPoolSetCeiling(p,c,__FILE__, __LINE__)
#define MemPoolPreAllocate(p,s,t) \
     _dbgMemPoolPreAllocate(p,s,t,__FILE__, __LINE__)
#define MemPoolPreAllocateHandles(p,h) \
     _dbgMemPoolPreAllocateHandles(p,h,__FILE__, __LINE__)
#define MemPoolShrink(p)         _dbgMemPoolShrink(p, __FILE__, __LINE__)
#define MemPoolCheck(p)          _dbgMemPoolCheck(p, __FILE__, __LINE__)
#define MemPoolWalk(p, e)        _dbgMemPoolWalk(p, e, __FILE__, __LINE__)
#define MemPoolSize(p)           _dbgMemPoolSize(p, __FILE__, __LINE__)
#define MemPoolCount(p)          _dbgMemPoolCount(p, __FILE__, __LINE__)
#define MemPoolInfo(p,x,i)       _dbgMemPoolInfo(p,x,i, __FILE__, __LINE__)
#define MemPoolFirst(i, b)       _dbgMemPoolFirst(i, b, __FILE__, __LINE__)
#define MemPoolNext(i, b)        _dbgMemPoolNext(i, b, __FILE__, __LINE__)
#define MemPoolLock(p)           _dbgMemPoolLock(p, __FILE__, __LINE__)
#define MemPoolUnlock(p)         _dbgMemPoolUnlock(p, __FILE__, __LINE__)
#define MemAlloc(p, f, s)        _dbgMemAlloc(p, f, s, __FILE__, __LINE__)
#define MemReAlloc(h, s, f)      _dbgMemReAlloc(h, s, f, __FILE__, __LINE__)
#define MemFree(h)               _dbgMemFree(h, __FILE__, __LINE__)
#define MemLock(h)               _dbgMemLock(h, __FILE__, __LINE__)
#define MemUnlock(h)             _dbgMemUnlock(h, __FILE__, __LINE__)
#define MemFix(h)                _dbgMemFix(h, __FILE__, __LINE__)
#define MemUnfix(h)              _dbgMemUnfix(h, __FILE__, __LINE__)
#define MemSize(h)               _dbgMemSize(h, __FILE__, __LINE__)
#define MemSizeRequested(h)      _dbgMemSizeRequested(h, __FILE__, __LINE__)
#define MemLockCount(h)          _dbgMemLockCount(h, __FILE__, __LINE__)
#define MemIsMoveable(h)         _dbgMemIsMoveable(h, __FILE__, __LINE__)
#define MemHandle(p)             _dbgMemHandle(p, __FILE__, __LINE__)
#define MemAllocPtr(p, s, f)     _dbgMemAllocPtr(p, s, f, __FILE__, __LINE__)
#define MemReAllocPtr(p, s, f)   _dbgMemReAllocPtr(p, s, f, __FILE__,__LINE__)
#define MemFreePtr(p)            _dbgMemFreePtr(p, __FILE__, __LINE__)
#define MemSizePtr(p)            _dbgMemSizePtr(p, __FILE__, __LINE__)
#define MemCheckPtr(p, x)        _dbgMemCheckPtr(p, x, __FILE__, __LINE__)
#define MemAllocFS(p)            _dbgMemAllocFS(p, __FILE__, __LINE__)
#define MemFreeFS(p)             _dbgMemFreeFS(p, __FILE__, __LINE__)

#else /* MEM_DEBUG */

/* MEM_DEBUG not defined: define dbgMemXXX as no-op macros
 * each macro returns "success" value when MEM_DEBUG not defined
 */
#ifndef dbgMemBreakpoint
#define dbgMemBreakpoint() ((void)0)
#define dbgMemCheckAll() 1
#define dbgMemCheckPtr(p, f, s) 1
#define dbgMemDeferFreeing(b) 1
#define dbgMemFormatCall(i, b, s) 0
#define dbgMemFormatErrorInfo(i, b, s) 0
#define dbgMemPoolDeferFreeing(p, b) 1
#define dbgMemFreeDeferred() 1
#define dbgMemPoolFreeDeferred(p) 1
#define dbgMemPoolInfo(p, b) 1
#define dbgMemPoolSetCheckFrequency(p, f) 1
#define dbgMemPoolSetDeferQueueLen(p, b) 1
#define dbgMemPoolSetName(p, n) 1
#define dbgMemProtectPtr(p, f) 1
#define dbgMemPtrInfo(p, b) 1
#define dbgMemReallocMoves(b) 1
#define dbgMemReportLeakage(p, c1, c2) 1
#define dbgMemReportWrongTaskRef(b) 1
#define dbgMemScheduleChecking(b, p, i) 1
#define dbgMemSetCheckFrequency(f) 1
#define dbgMemSetCheckpoint(c) 1
#define dbgMemSetDefaultErrorOutput(x, f) 1
#define dbgMemSetDeferQueueLen(l) 1
#define dbgMemSetDeferSizeThreshold(s) 1
#define dbgMemSetEntryHandler(f) 0
#define dbgMemSetExitHandler(f) 0
#define dbgMemSetFreeFill(c) 1
#define dbgMemSetGuardFill(c) 1
#define dbgMemSetGuardSize(s) 1
#define dbgMemSetInUseFill(c) 1
#define dbgMemSetCallstackChains(s) 1
#define dbgMemSetStackChecking(s) 1
#define dbgMemSetSafetyLevel(s) 1
#define dbgMemSettingsInfo(b) 1
#define dbgMemSuppressFreeFill(b) 1
#define dbgMemTotalCount() 1
#define dbgMemTotalSize() 1
#define dbgMemWalkHeap(b) MEM_POOL_OK
#endif /* dbgMemBreakpoint */

#endif /* MEM_DEBUG */
#endif /* _SHI_dbgMacros */

#if defined(__WATCOMC__) && defined(__SW_3S)
/* Watcom stack calling convention */
   #pragma aux MemDefaultPool "_*";
   #pragma aux MemDefaultPoolBlockSizeFS "_*";
   #pragma aux MemDefaultPoolPageSize "_*";
   #pragma aux MemDefaultPoolFlags "_*";
   #pragma aux SmartHeap_malloc "_*";
   #pragma aux SmartHeap_far_malloc "_*";
   #pragma aux SmartHeap_new "_*";
#ifdef MEM_DEBUG
   #pragma aux dbgMemGuardSize "_*";
   #pragma aux dbgMemGuardFill "_*";
   #pragma aux dbgMemFreeFill "_*";
   #pragma aux dbgMemInUseFill "_*";
#endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !defined(_SMARTHEAP_PROT) */
