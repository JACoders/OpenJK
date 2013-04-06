#ifndef __FF_CONFIGPARSER_H
#define __FF_CONFIGPARSER_H

//#include "ff_public.h"	// in precompiled header

//class CImmDevice;

class FFConfigParser
{
public:
	typedef vector<string> TInclude;
	typedef set<string> TDevice;
	typedef map<int, string> TDeviceType;
	typedef map<int, TDeviceType> TTechType;
	typedef vector<int> TDefaultPriority;

	typedef struct
	{
		TInclude include;
		TDevice device;
	} TData;

	typedef map<string, TData> TMap;

protected:
	TTechType mDefaultSet;
	TDefaultPriority mDefaultPriority;
	TMap mMap;					// Contains all effect sets by name

	qboolean Parse( void *file );
	qboolean ParseSets( const char **pos );
	qboolean ParseSet( const char **pos, TData &data );
	qboolean ParseSetIncludes( const char **pos, TInclude &include );
	qboolean ParseSetDevices( const char **pos, TDevice &device );
	qboolean ParseDefaults( const char **pos );
	qboolean ParseDefault( const char **pos, TDeviceType &defaultSet );

public:

	qboolean Init( const char *filename/*, CImmDevice *Device = NULL*/ );
	void Clear( void );

//	const char* RightOfBase( const char *effectname );
//	const char* RightOfGame( const char *effectname );
	const char* RightOfSet( const char *effectname );

	const char* GetFFSet( CImmDevice *Device );
	TInclude& GetIncludes( const char *name = NULL );
};

#endif // __FF_CONFIGPARSER_H
