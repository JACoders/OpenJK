/* heapagnt.h -- HeapAgent (tm) public C/C++ header file
 *
 * Copyright (C) 1991-1997 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the copyright owner.
 *
 */

#if !defined(_HEAPAGNT_H)
#define _HEAPAGNT_H

#if !defined(_SMARTHEAP_H)
#include "smrtheap.h"
#endif /* _SMARTHEAP_H */


#ifdef __cplusplus
extern "C" {
#endif

#define HA_MAJOR_VERSION 3
#define HA_MINOR_VERSION 1
#define HA_UPDATE_LEVEL 2

   
/*** Types ***/

/* Saftey Levels: parameter to dbgMemSetSafetyLevel */
typedef enum
{
   MEM_SAFETY_SOME=2,     /* fast: minimal debug performance degredation   */
   MEM_SAFETY_FULL,       /* slower: recommended during development        */
   MEM_SAFETY_DEBUG,      /* entire memory pool is checked each entrypoint */
   MEM_SAFETY_LEVEL_INT_MAX = INT_MAX  /* to ensure enum is full int in size */
} MEM_SAFETY_LEVEL;

/* Pointer types: parameter to dbgMemCheckPtr */
typedef enum
{
   MEM_POINTER_HEAP,
   MEM_POINTER_STACK,
   MEM_POINTER_STATIC,
   MEM_POINTER_CODE,
   MEM_POINTER_READONLY,
   MEM_POINTER_READWRITE
} MEM_POINTER_TYPE;

/* Debug Pool Info: parameter to dbgMemPoolInfo */
typedef struct
{
   MEM_POOL pool;
   const char MEM_FAR *name;
   MEM_API createAPI;
   const char MEM_FAR *createFile;
   int createLine;
   unsigned long createPass;
   unsigned long createAllocCount;
   unsigned createCheckpoint;
   int isDeferFreeing;
   unsigned long deferQueueLength;
   unsigned checkFrequency;
   MEM_ERROR_INFO lastOkInfo;
   unsigned long threadID;
   unsigned long pid;
	void *callStack[MEM_MAXCALLSTACK];
} DBGMEM_POOL_INFO;

/* Debug Ptr Info: parameter to dbgMemPtrInfo */
typedef struct
{
   void MEM_FAR *ptr;
   MEM_POOL pool;
   unsigned long argSize;
   MEM_BLOCK_TYPE blockType;
   MEM_BOOL isInUse;
   MEM_API createAPI;
   const char MEM_FAR *createFile;
   int createLine;
   unsigned long createPass;
   unsigned checkpoint;
   unsigned long allocCount;
   MEM_BOOL isDeferFreed;
   MEM_BOOL isFreeFillSuppressed;
   MEM_BOOL isReadOnly;
   MEM_BOOL isNoFree;
   MEM_BOOL isNoRealloc;
   unsigned long threadID;
   unsigned long pid;
	void MEM_FAR *callStack[MEM_MAXCALLSTACK];
} DBGMEM_PTR_INFO;

/* Stack checking settting: parameter to dbgMemSetStackChecking */
typedef enum
{
	DBGMEM_STACK_CHECK_NONE=1,
	DBGMEM_STACK_CHECK_ENTRY,
	DBGMEM_STACK_CHECK_RETURN,
	DBGMEM_STACK_CHECK_INT_MAX = INT_MAX  /* to ensure enum is full int size */
} DBGMEM_STACK_CHECKING;

/* Debug Settings Info: parameter to dbgMemSettingsInfo */
typedef struct
{
   MEM_SAFETY_LEVEL safetyLevel;
   unsigned checkFrequency;
   unsigned long allocCount;
   unsigned checkpoint;
   MEM_BOOL isDeferFreeing;
   unsigned long deferQueueLength;
   unsigned long deferSizeThreshold;
   MEM_BOOL isFreeFillSuppressed;
   MEM_BOOL isReallocAlwaysMoves;
   MEM_BOOL isWrongTaskRefReported;
   unsigned outputFlags;
   const char MEM_FAR *outputFile;
   unsigned guardSize;
	unsigned callstackChains;
	DBGMEM_STACK_CHECKING stackChecking;
   unsigned char guardFill;
   unsigned char freeFill;
   unsigned char inUseFill;
} DBGMEM_SETTINGS_INFO;

/* Trace function pointer: parameter to dbgMemSet[Entry/Exit]Handler */
typedef void (MEM_ENTRY2 * MEM_ENTRY3 MEM_TRACE_FN)
   (MEM_ERROR_INFO MEM_FAR *, unsigned long);

/* define and initialize these variables at file scope to change defaults */
extern const unsigned dbgMemGuardSize;
extern const unsigned char dbgMemGuardFill;
extern const unsigned char dbgMemFreeFill;
extern const unsigned char dbgMemInUseFill;

#define DBGMEM_PTR_NOPROTECTION    0x0000u
#define DBGMEM_PTR_READONLY        0x0001u
#define DBGMEM_PTR_NOFREE          0x0002u
#define DBGMEM_PTR_NOREALLOC       0x0004u

#define DBGMEM_OUTPUT_PROMPT       0x0001u
#define DBGMEM_OUTPUT_CONSOLE      0x0002u
#define DBGMEM_OUTPUT_BEEP         0x0004u
#define DBGMEM_OUTPUT_FILE         0x0010u
#define DBGMEM_OUTPUT_FILE_APPEND  0x0020u



/*** Function Prototypes ***/

#ifdef MEM_DEBUG

#if defined(MEM_WIN16) || defined(MEM_WIN32)
#define SHI_MAJOR_VERSION HA_MAJOR_VERSION
#define SHI_MINOR_VERSION HA_MINOR_VERSION
#define SHI_UPDATE_LEVEL HA_UPDATE_LEVEL
#endif /* MEM_WIN16 || MEM_WIN32 */

/* malloc, new, et al call these entry-point in debug lib */
MEM_ENTRY1 void MEM_FAR * MEM_ENTRY _dbgMemAllocPtr1(MEM_POOL, unsigned long,
   unsigned, MEM_API, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemFreePtr1(void MEM_FAR *, MEM_API,
   const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_FAR * MEM_ENTRY _dbgMemReAllocPtr1(void MEM_FAR *,
   unsigned long, unsigned, MEM_API, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemSizePtr1(void MEM_FAR *, MEM_API,
   const char MEM_FAR *, int);
void MEM_FAR * MEM_ENTRY _dbgMEM_alloc(size_t, unsigned, MEM_API,
                                       const char MEM_FAR *, int);
void MEM_FAR * MEM_ENTRY _dbgMEM_realloc(void MEM_FAR *, size_t,
                                         const char MEM_FAR *, int);
void MEM_ENTRY _dbgMEM_free(void MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_ENTRY _shi_deleteLoc(const char MEM_FAR *file, int line);


/* functions to control error detection */
MEM_ENTRY1 MEM_SAFETY_LEVEL MEM_ENTRY dbgMemSetSafetyLevel(MEM_SAFETY_LEVEL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetGuardSize(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetGuardFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetFreeFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetInUseFill(MEM_UCHAR);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetCallstackChains(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetStackChecking(DBGMEM_STACK_CHECKING);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemSetCheckpoint(unsigned);
MEM_ENTRY1 unsigned MEM_ENTRY _dbgMemPoolSetCheckFrequency(MEM_POOL,
   unsigned, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolDeferFreeing(MEM_POOL, int,
   const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolFreeDeferred(MEM_POOL,
   const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemProtectPtr(void MEM_FAR *, unsigned,
   const char MEM_FAR *, int);

/* functions to control error reporting */
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemFormatErrorInfo(MEM_ERROR_INFO MEM_FAR *,
   char MEM_FAR *, unsigned);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemFormatCall(MEM_ERROR_INFO MEM_FAR *,
   char MEM_FAR *, unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSetDefaultErrorOutput(unsigned flags,
   const char MEM_FAR *file);
MEM_ENTRY1 MEM_TRACE_FN MEM_ENTRY dbgMemSetEntryHandler(MEM_TRACE_FN);
MEM_ENTRY1 MEM_TRACE_FN MEM_ENTRY dbgMemSetExitHandler(MEM_TRACE_FN);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemReportLeakage(MEM_POOL,unsigned,unsigned,
   const char MEM_FAR *, unsigned);

MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemTotalCount(const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemTotalSize(const char MEM_FAR *, int);
MEM_ENTRY1 void MEM_ENTRY _dbgMemBreakpoint(const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbg_MemPoolInfo(MEM_POOL,
   DBGMEM_POOL_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPtrInfo(void MEM_FAR *,
   DBGMEM_PTR_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemSettingsInfo(
   DBGMEM_SETTINGS_INFO MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned MEM_ENTRY dbgMemSetCheckFrequency(unsigned);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemDeferFreeing(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemFreeDeferred(const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemReallocMoves(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemSuppressFreeFill(MEM_BOOL);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemReportWrongTaskRef(MEM_BOOL);
MEM_ENTRY1 unsigned long MEM_ENTRY dbgMemSetDeferQueueLen(unsigned long);
MEM_ENTRY1 unsigned long MEM_ENTRY dbgMemSetDeferSizeThreshold(unsigned long);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemPoolSetName(MEM_POOL,
   const char MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 unsigned long MEM_ENTRY _dbgMemPoolSetDeferQueueLen(MEM_POOL,
   unsigned long, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbgMemCheckAll(const char MEM_FAR *, int);
MEM_POOL_STATUS MEM_ENTRY _dbgMemWalkHeap(
   MEM_POOL_ENTRY MEM_FAR *, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY _dbg_MemCheckPtr(void MEM_FAR *,
   MEM_POINTER_TYPE, unsigned long, const char MEM_FAR *, int);
MEM_ENTRY1 MEM_BOOL MEM_ENTRY dbgMemScheduleChecking(MEM_BOOL, int, unsigned);
#endif


/* Wrapper macros for passing file/line info */

#ifndef _SHI_dbgMacros
#ifdef MEM_DEBUG
#ifndef MALLOC_MACRO
#define MEM_malloc(s_) _dbgMEM_alloc(s_, 0, MEM_MEM_MALLOC, __FILE__, __LINE__)
#define MEM_calloc(s_, c_) _dbgMEM_alloc((s_)*(c_), 1 /*MEM_ZEROINIT*/, \
   MEM_MEM_CALLOC, __FILE__, __LINE__)
#define MEM_realloc(p_, s_) _dbgMEM_realloc(p_, s_, __FILE__, __LINE__)
#define MEM_free(p_) _dbgMEM_free(p_, __FILE__, __LINE__)
#endif
#if !defined(NO_MALLOC_MACRO)
#ifdef malloc
#undef malloc
#endif
#ifdef calloc
#undef calloc
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef free
#undef free
#endif
#define malloc(s_) MEM_malloc(s_)
#define calloc(s_, c_) MEM_calloc(s_, c_)
#define realloc(p_, s_) MEM_realloc(p_, s_)
#define free(p_) MEM_free(p_)
#endif /* NO_MALLOC_MACRO */

#define dbgMemPoolSetCheckFrequency(p, b) \
     _dbgMemPoolSetCheckFrequency(p, b, __FILE__, __LINE__)
#define dbgMemPoolDeferFreeing(p, b) \
     _dbgMemPoolDeferFreeing(p, b, __FILE__, __LINE__)
#define dbgMemPoolFreeDeferred(p) _dbgMemPoolFreeDeferred(p,__FILE__,__LINE__)
#define dbgMemProtectPtr(p, b)    _dbgMemProtectPtr(p, b, __FILE__, __LINE__)
#define dbgMemReportLeakage(p, c1, c2) \
     _dbgMemReportLeakage(p, c1, c2, __FILE__, __LINE__)

#define dbgMemTotalCount() _dbgMemTotalCount(__FILE__, __LINE__)
#define dbgMemTotalSize() _dbgMemTotalSize(__FILE__, __LINE__)
#define dbgMemPoolInfo(p, b) _dbg_MemPoolInfo(p, b, __FILE__, __LINE__)
#define dbgMemPtrInfo(p, b) _dbgMemPtrInfo(p, b, __FILE__, __LINE__)
#define dbgMemSettingsInfo(b) _dbgMemSettingsInfo(b, __FILE__, __LINE__)
#define dbgMemPoolSetName(p, n) _dbgMemPoolSetName(p, n, __FILE__, __LINE__)
#define dbgMemPoolSetDeferQueueLen(p, l) \
   _dbgMemPoolSetDeferQueueLen(p, l, __FILE__, __LINE__)
#define dbgMemFreeDeferred() _dbgMemFreeDeferred(__FILE__, __LINE__)
#define dbgMemWalkHeap(b) _dbgMemWalkHeap(b, __FILE__, __LINE__)
#define dbgMemCheckAll() _dbgMemCheckAll(__FILE__, __LINE__)
#define dbgMemCheckPtr(p, t, s) _dbg_MemCheckPtr(p, t, s, __FILE__, __LINE__)
#define dbgMemBreakpoint() _dbgMemBreakpoint(__FILE__, __LINE__)

#endif /* MEM_DEBUG */
#endif /* _SHI_dbgMacros */

       
#ifdef __cplusplus
}

#ifndef __BORLANDC__
/* Borland C++ does not treat extern "C++" correctly */
extern "C++"
{
#endif /* __BORLANDC__ */

#if defined(_MSC_VER) && _MSC_VER >= 900
#pragma warning(disable : 4507)
#endif

#if defined(new)
#if defined(MEM_DEBUG)
#undef new
#define DEFINE_NEW_MACRO 1
#endif
#endif

#ifdef DEBUG_NEW
#undef DEBUG_NEW
#endif
#ifdef DEBUG_DELETE
#undef DEBUG_DELETE
#endif
   
#ifdef MEM_DEBUG
#define DBG_FORMAL , const char MEM_FAR *file, int line
#define DBG_ACTUAL , file, line


/* both debug and non-debug versions are both defined so that calls
 * to shi_New from inline versions of operator new in modules
 * that were not recompiled with MEM_DEBUG will resolve correctly
 */
void MEM_FAR * MEM_ENTRY_ANSI shi_New(unsigned long DBG_FORMAL, unsigned=0,
   MEM_POOL=0);

/* operator new variants: */

/* compiler-specific versions of new */
#if UINT_MAX == 0xFFFFu
#if defined(__BORLANDC__)
#if (defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__))
inline void far *operator new(unsigned long sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#if __BORLANDC__ >= 0x450
inline void MEM_FAR *operator new[](unsigned long sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#endif /* __BORLANDC__ >= 0x450 */
#endif /* __LARGE__ */

#elif defined(_MSC_VER)
#if (defined(M_I86LM) || defined(M_I86CM) || defined(M_I86HM))
inline void __huge * operator new(unsigned long count, size_t sz DBG_FORMAL)
   { return (void __huge *)shi_New(count * sz DBG_ACTUAL, MEM_HUGE); }
#endif /* M_I86LM */
#endif /* _MSC_VER */

#endif /* compiler-specific versions of new */

/* version of new that passes memory allocation flags */
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags); }

/* version of new that allocates from a specified memory pool with alloc flags*/
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, MEM_POOL pool)
   { return shi_New(sz DBG_ACTUAL, 0, pool); }
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL, MEM_POOL pool, 
                                  unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags, pool); }
#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL, MEM_POOL pool)
   { return shi_New(sz DBG_ACTUAL, 0, pool); }
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL, MEM_POOL pool, 
                                  unsigned flags)
   { return shi_New(sz DBG_ACTUAL, flags, pool); }
#endif /* SHI_ARRAY_NEW */

/* version of new that changes the size of a memory block */
inline void MEM_FAR *operator new(size_t new_sz DBG_FORMAL,
                                  void MEM_FAR *lpMem, unsigned flags)
   { return _dbgMemReAllocPtr1(lpMem, new_sz, flags, MEM_NEW DBG_ACTUAL); }
#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t new_sz DBG_FORMAL,
                                    void MEM_FAR *lpMem, unsigned flags)
   { return _dbgMemReAllocPtr1(lpMem, new_sz, flags, MEM_NEW DBG_ACTUAL); }
#endif /* SHI_ARRAY_NEW */

/* To have HeapAgent track file/line of C++ allocations,
 * define new/delete as macros:
 * #define new DEBUG_NEW
 * #define delete DEBUG_DELETE
 *
 * In cases where you use explicit placement syntax, or in modules that define
 * operator new/delete, you must undefine the new/delete macros, e.g.:
 * #undef new
 *    void *x = new(placementArg) char[30];  // cannot track file/line info
 * #define new DEBUG_NEW
 *    void *y = new char[20];                // resume tracking file/line info
 */

#if (!(defined(_AFX) && defined(_DEBUG)) \
	  && !(defined(_MSC_VER) && _MSC_VER >= 900))
/* this must be defined out-of-line for _DEBUG MFC and MEM_DEBUG VC++/Win32 */
inline void MEM_FAR *operator new(size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#else
void MEM_FAR * MEM_ENTRY_ANSI operator new(size_t sz DBG_FORMAL);
#endif /* _AFX && _DEBUG */

#ifdef SHI_ARRAY_NEW
inline void MEM_FAR *operator new[](size_t sz DBG_FORMAL)
   { return shi_New(sz DBG_ACTUAL); }
#endif /* SHI_ARRAY_NEW */

#if !(defined(__IBMCPP__) && defined(__DEBUG_ALLOC__))
/* debug new/delete built in for IBM Set C++ and Visual Age C++ */

#define DEBUG_NEW new(__FILE__, __LINE__)
#define DEBUG_NEW1(x_) new(__FILE__, __LINE__, x_)
#define DEBUG_NEW2(x_, y_) new(__FILE__, __LINE__, x_, y_)
#define DEBUG_NEW3(x_, y_, z_) new(__FILE__, __LINE__, x_, y_, z_)

#define DEBUG_DELETE _shi_deleteLoc(__FILE__, __LINE__), delete

#ifdef DEFINE_NEW_MACRO
#ifdef macintosh  /* MPW C++ bug precludes new --> DEBUG_NEW --> new(...) */
#define new new(__FILE__, __LINE__)
#define delete _shi_deleteLoc(__FILE__, __LINE__), delete
#else
#define new DEBUG_NEW
#define delete DEBUG_DELETE
#endif /* macintosh */
#endif /* DEFINE_NEW_MACRO */
#endif /* __IBMCPP__ */
#endif /* MEM_DEBUG */

#include "smrtheap.hpp"

#ifndef __BORLANDC__
}
#endif /* __BORLANDC__ */

#endif /* __cplusplus */

#endif /* !defined(_HEAPAGNT_H) */
