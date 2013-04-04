#ifndef FF_MULTISET_H
#define FF_MULTISET_H

#include "ff_ffset.h"

//===[FFMultiSet]=====================================================/////////////
//
//	A collection class of FFSet objects. These functions generally
//	iterate over the entire set of FFSets, performing the same operation
//	on all contained FFSets.
//
//====================================================================/////////////

class FFMultiSet
{
public:
	typedef vector<FFSet*> Set;
protected:
	FFConfigParser *mConfig;
	Set mSet;
	CImmDevices *mDevices;
public:
	qboolean Init( FFConfigParser &config );
//	qboolean Lookup( set<MultiEffect*> &effect, const char *name );
	qboolean GetRegisteredNames( TNameTable &NameTable );
	qboolean StopAll();
	void clear();

	//
	//	Optional
	//
#ifdef FF_ACCESSOR
	Set& GetSets() { return mSet; }
	CImmDevices* GetDevices() { return mDevices; }
#endif

#ifdef FF_CONSOLECOMMAND
	void Display( TNameTable &Unprocessed, TNameTable &Processed );
	static void GetDisplayTokens( TNameTable &Tokens );
#endif
};

#endif // FF_MULTISET_H