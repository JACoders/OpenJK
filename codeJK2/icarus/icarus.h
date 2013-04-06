// ICARUS Public Header File

#pragma warning ( disable : 4786 )	//NOTENOTE: STL Debug name length warning

#ifndef	__ICARUS__
#define __ICARUS__

#pragma warning( disable : 4786 )  // identifier was truncated 
#pragma warning( disable : 4514 )  // unreferenced inline was removed
#pragma warning( disable : 4710 )  // not inlined

#pragma warning( push, 3 )	//save current state and change to 3

//For system-wide prints
enum WL_e {
	WL_ERROR=1,
	WL_WARNING,
	WL_VERBOSE,
	WL_DEBUG
};

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); a++ )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

#include "Tokenizer.h"
#include "BlockStream.h"
#include "Interpreter.h"
#include "Sequencer.h"
#include "TaskManager.h"
#include "Instance.h"

#pragma warning( pop )	//restore


extern void *ICARUS_Malloc(int iSize);
extern void  ICARUS_Free(void *pMem);

#endif	//__ICARUS__
