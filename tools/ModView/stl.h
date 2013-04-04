// Filename:-	stl.h
//
// pure laziness.... :-)

#ifndef STL_H
#define STL_H


#pragma warning (push, 3)	//go back down to 3 for the stl include
#include "disablewarnings.h"
#include <list>
#include <map>
#include <string>
#include <set>
#include <vector>
#pragma warning (pop)

using namespace std;

#include "disablewarnings.h"

// some common ones...
//
typedef set		<string> StringSet_t;
typedef vector	<string> StringVector_t;

#endif	// #ifndef STL_H



////////////////// eof /////////////////


