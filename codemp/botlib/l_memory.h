
/*****************************************************************************
 * name:		l_memory.h
 *
 * desc:		memory management
 *
 * $Archive: /source/code/botlib/l_memory.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

//#define MEMDEBUG

#ifdef MEMDEBUG
#define GetMemory(size)				GetMemoryDebug(size, #size, __FILE__, __LINE__);
#define GetClearedMemory(size)		GetClearedMemoryDebug(size, #size, __FILE__, __LINE__);
//allocate a memory block of the given size
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line);
//allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line);
//
#define GetHunkMemory(size)			GetHunkMemoryDebug(size, #size, __FILE__, __LINE__);
#define GetClearedHunkMemory(size)	GetClearedHunkMemoryDebug(size, #size, __FILE__, __LINE__);
//allocate a memory block of the given size
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
#else
//allocate a memory block of the given size
void *GetMemory(unsigned long size);
//allocate a memory block of the given size and clear it
void *GetClearedMemory(unsigned long size);
//
#ifdef BSPC
#define GetHunkMemory GetMemory
#define GetClearedHunkMemory GetClearedMemory
#else
//allocate a memory block of the given size
void *GetHunkMemory(unsigned long size);
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemory(unsigned long size);
#endif
#endif

//free the given memory block
void FreeMemory(void *ptr);
//returns the amount available memory
int AvailableMemory(void);
//prints the total used memory size
void PrintUsedMemorySize(void);
//print all memory blocks with label
void PrintMemoryLabels(void);
//returns the size of the memory block in bytes
int MemoryByteSize(void *ptr);
//free all allocated memory
void DumpMemory(void);
