#pragma once

// ICARUS Public Header File

#pragma warning ( disable : 4786 )	//NOTENOTE: STL Debug name length warning

extern void *ICARUS_Malloc(int iSize);
extern void  ICARUS_Free(void *pMem);

#include "game/g_public.h"

#pragma warning( disable : 4786 )  // identifier was truncated 
#pragma warning( disable : 4514 )  // unreferenced inline was removed
#pragma warning( disable : 4710 )  // not inlined

#pragma warning( push, 3 )	//save current state and change to 3

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); a++ )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

#include "tokenizer.h"
#include "blockstream.h"
#include "interpreter.h"
#include "sequencer.h"
#include "taskmanager.h"
#include "instance.h"

#pragma warning( pop )	//restore
