#ifndef FFSET_H
#define FFSET_H

//#include "ff_ConfigParser.h"
//#include "ff_utils.h"
//#include "ff_compound.h"
//class MultiEffect;
//#include "ifc.h"
//class CImmDevice;
//class CImmProject;

#include "ff_MultiEffect.h"

class FFSet
{
	//
	//	Types
	//
public:
	typedef map<string, CImmProject*> TProject;
	typedef vector<TProject> TInclude;
	typedef vector<string> TIncludePath;

	//
	//	Variables
	//
protected:
	TInclude mInclude;
	TIncludePath mIncludePath;
	CImmDevice *mDevice;
	FFConfigParser &mParser;

	//
	//	Functions
	//
public:
	FFSet( FFConfigParser &ConfigParser, CImmDevice *Device );
	~FFSet();
	MultiEffect* Register( const char *path, qboolean create = qtrue );
	void GetRegisteredNames( TNameTable &NameTable );
	qboolean StopAll( void );

protected:
	void InitIncludes( const char *setname = NULL );

	//
	//	Optional
	//
#ifdef FF_ACCESSOR
public:
	CImmDevice* GetDevice( void ) { return mDevice; }
#endif

#ifdef FF_CONSOLECOMMAND
public:
	void Display( TNameTable &Unprocessed, TNameTable &Processed );
	void DisplaySearchOrder( void );
	void DisplayLoadedFiles( void );
	static void GetDisplayTokens( TNameTable &Tokens );
#endif

};

#endif // FFSET_H