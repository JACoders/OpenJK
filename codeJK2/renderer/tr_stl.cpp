// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#pragma warning(disable : 4786)	// identifier was truncated 

// Filename:-	tr_stl.cpp
//
// I mainly made this file because I was getting sick of all the stupid error messages in MS's STL implementation,
//	and didn't want them showing up in the renderer files they were used in. This way keeps them more or less invisible
//	because of minimal dependancies
//
#include "tr_local.h"	// this isn't actually needed other than getting rid of warnings via pragmas
#include "tr_stl.h"

#pragma warning( push,3 )

#pragma warning(disable : 4514)	// unreferenced inline function has been removed (within STL, not this code)
#pragma warning(disable : 4710)	// 
#pragma warning(disable : 4503)	// decorated name length xceeded, name was truncated

#include <map>
#include "../qcommon/sstring.h"	// #include <string>
using namespace std;

typedef map<sstring_t, const char *>	ShaderEntryPtrs_t;
										ShaderEntryPtrs_t ShaderEntryPtrs;






void ShaderEntryPtrs_Clear(void)
{
	ShaderEntryPtrs.clear();
}


int ShaderEntryPtrs_Size(void)
{
	return ShaderEntryPtrs.size();
}


void ShaderEntryPtrs_Insert(const char  *token, const char  *p)
{
	ShaderEntryPtrs_t::iterator it = ShaderEntryPtrs.find(token);

	if (it == ShaderEntryPtrs.end())
	{
		ShaderEntryPtrs[token] = p;
	}
}



// returns NULL if not found...
//
const char *ShaderEntryPtrs_Lookup(const char *psShaderName)
{
	ShaderEntryPtrs_t::iterator it = ShaderEntryPtrs.find(psShaderName);
	if (it != ShaderEntryPtrs.end())
	{
		const char *p = (*it).second;
		return p;
	}

	return NULL;
}



#pragma warning ( pop )

