extern int SmartHeap_malloc;
extern int SmartHeap_new;

static void *refSmartHeap_malloc = &SmartHeap_malloc;

#if defined(_DEBUG) && !defined(MEM_DEBUG)
#define MEM_DEBUG 1
#endif

#if defined(MFC) && !defined(_AFXDLL)

static void *refSmartHeap_new = &SmartHeap_new;

#ifdef MEM_DEBUG
#if _MSC_VER < 1000
#pragma comment(lib, "hamfc32m.lib")
#else
#pragma comment(lib, "hamfc4m.lib")
#endif /* _MSC_VER */
#else
#if _MSC_VER >= 1000
#ifdef _MT
#pragma comment(lib, "shmfc4mt.lib")
#else
#pragma comment(lib, "shmfc4m.lib")
#endif /* _MT */
#endif /* _MSC_VER */
#endif /* MEM_DEBUG */

#endif /* MFC */

#if defined(MEM_DEBUG)
#pragma comment(lib, "haw32m.lib")
#elif defined(_DLL)
#ifdef _MT
#ifdef MEM_SMP
#pragma comment(lib, "shdsmpmt.lib")
#else
#pragma comment(lib, "shdw32mt.lib")
#endif /* MEM_SMP */
#else
#pragma comment(lib, "shdw32m.lib")
#endif /* _MT */
#else /* _DLL */
#ifdef _MT
#ifdef MEM_SMP
#pragma comment(lib, "shlsmpmt.lib")
#else
#pragma comment(lib, "shlw32mt.lib")
#endif /* MEM_SMP */
#else
#pragma comment(lib, "shlw32m.lib")
#endif /* _MT */
#endif /* MEM_DEBUG */
