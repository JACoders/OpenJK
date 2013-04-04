#ifndef __ICR_STDAFX__
#define __ICR_STDAFX__

#pragma warning( disable : 4786 )  // identifier was truncated 

#pragma warning (push, 3)
#include <string>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#pragma warning (pop)

using namespace std;

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); a++ )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

#endif