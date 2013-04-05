#include "common_headers.h"

#ifdef _IMMERSION

//#include "ff_ConfigParser.h"
//#include "ifc.h"
//#include "ff_utils.h"

////--------------------
/// FFConfigParser::Init
//------------------------
//	Reads the force feedback configuration file. Call this once after the device
//	is initialized.
//
//	Parameters:
//	*	filename
//
//	Returns:
//	*	qtrue - the effects set directory has been set according to the initialized
//			device. (See base/fffx/fffx.cfg)
//	*	qfalse - no effects set could be determined for this device.
//
qboolean FFConfigParser::Init( const char *filename )
{
	Clear();	// Always cleanup

	return qboolean( filename && Parse( LoadFile( filename ) ) );
}

////---------------------
/// FFConfigParser::Clear
//-------------------------
//
//
//	Parameters:
//
//	Returns:
//
void FFConfigParser::Clear( void )
{
	mMap.clear();
	mDefaultSet.clear();
}

////---------------------
/// FFConfigParser::Parse
//-------------------------
//
//
//	Parameters:
//
//	Returns:
//
qboolean FFConfigParser::Parse( void *file )
{
	qboolean result = qboolean( file != NULL );

	if ( file )
	{
		const char *token = 0, *pos = (const char*)file;
		for
		(	token = COM_ParseExt( &pos, qtrue )
		;	token[ 0 ]
		&&	result // fail if any problem
		;	token = COM_ParseExt( &pos, qtrue )
		){
			if ( !stricmp( token, "ffdefaults" ) )
			{
				result &= ParseDefaults( &pos );
			}
			else
			if ( !stricmp( token, "ffsets" ) )
			{
				result &= ParseSets( &pos );
			}
			else
			{
				// unexpected field
				result = qfalse;
			}
		}

		FS_FreeFile( file );
	}

	return result;
}

////---------------------------------
/// FFConfigParser::ParseDefaultBlock
//-------------------------------------
//
//
//	Parameters:
//
//	Returns:
//
qboolean FFConfigParser::ParseDefault( const char **pos, TDeviceType &defaultSet )
{
	qboolean result = qboolean( pos != NULL );

	if ( pos )
	{
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				int device = 0;

				if ( sscanf( token, "%d", &device ) )
				{
					string &str = defaultSet[ device ];
					if ( !str.size() )
					{
						str = COM_ParseExt( pos, qfalse );
						result &= qboolean( str.size() > 0 );
					}
					else
					{
						result = qfalse;
#ifdef FF_PRINT
						ConsoleParseError
						(	"Redefinition of DeviceType index"
						,	token
						);
#endif
					}
				}
				else
				{
					result = qfalse;
#ifdef FF_PRINT
					ConsoleParseError
					(	"DeviceType field should begin with an integer"
					,	token
					);
#endif
				}
			}
		}
	}

	return result;
}



////----------------------------
/// FFConfigParser::ParseDefault
//--------------------------------
//
//
//	Parameters:
//
//	Returns:
//
qboolean FFConfigParser::ParseDefaults( const char **pos )
{
	qboolean result = qboolean( pos != NULL );

	if ( pos )
	{
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				int techType = 0;

				if ( sscanf( token, "%d", &techType ) )
				{
					TDeviceType &deviceType = mDefaultSet[ techType ];
					if ( !deviceType.size() )
					{
						result &= ParseDefault( pos, deviceType );
						mDefaultPriority.push_back( techType );
					}
					else
					{
						result = qfalse;
#ifdef FF_PRINT
						ConsoleParseError
						(	"Redefinition of TechType index"
						,	token
						);
#endif
					}
				}
				else
				{
					result = qfalse;
#ifdef FF_PRINT
					ConsoleParseError
					(	"TechType fields should begin with integers"
					,	token
					);
#endif
				}
			}
		}
		else
		{
			// expected '{'
			result = qfalse;
		}
	}

	return result;
}

////--------------------------
/// FFConfigParser::RightOfSet
//------------------------------
//
//
//	Parameters:
//
//	Returns:
//
const char* FFConfigParser::RightOfSet( const char *effectname )
{
	const char *s = effectname;

	// Check through all set names and test effectname against it
	for
	(	TMap::iterator itMap = mMap.begin()
	;	itMap != mMap.end() && s == effectname
	;	itMap++
	){
		s = RightOf( effectname, (*itMap).first.c_str() );
	}

	return s ? s : effectname;
}

qboolean FFConfigParser::ParseSetDevices( const char **pos, TDevice &device )
{
	qboolean result = qboolean( pos != NULL );

	if ( pos )
	{
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				device.insert( token );
			}

			result = qboolean( token[ 0 ] != 0 );
		}
		else
		{
			// expected '{'
			result = qfalse;
		}
	}

	return result;
}

qboolean FFConfigParser::ParseSetIncludes( const char **pos, TInclude &include )
{
	qboolean result = qboolean( pos != NULL );

	if ( pos )
	{
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				include.push_back( token );
			}

			result = qboolean( token[ 0 ] != 0 );
		}
		else
		{
			// expected '{'
			result = qfalse;
		}
	}

	return result;
}

qboolean FFConfigParser::ParseSet( const char **pos, TData &data )
{
	qboolean result = qboolean( pos != NULL );

	if ( pos )
	{
		const char *oldpos = *pos;	// allows set declarations with no attributes to have no "{}" 
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				if ( !stricmp( token, "includes" ) )
				{
					result &= ParseSetIncludes( pos, data.include );
				}
				else
				if ( !stricmp( token, "devices" ) )
				{
					result &= ParseSetDevices( pos, data.device );
				}
				else
				{
					result = qfalse;
#ifdef FF_PRINT
					ConsoleParseError
					(	"Invalid set parameter. Should be 'includes' or 'devices'"
					,	token
					);
#endif
				}
			}
		}
		else
		{
			// expected '{'		(no longer expected!)
			//result = qfalse;	(no longer an error!)
			*pos = oldpos;
		}
	}

	return result;
}

////-------------------------
/// FFConfigParser::ParseSets
//-----------------------------
//
//
//	Parameters:
//
//	Returns:
//
qboolean FFConfigParser::ParseSets( const char **pos )
{
	qboolean result = qboolean( pos != NULL );
	string			groupName;

	if ( pos )
	{
		char *token = COM_ParseExt( pos, qtrue );
		if ( token[ 0 ] == '{' )
		{
			for
			(	token = COM_ParseExt( pos, qtrue )
			;	token[ 0 ]
			&&	token[ 0 ] != '}'
			&&	result // fail if any problem
			;	token = COM_ParseExt( pos, qtrue )
			){
				TData &data = mMap[ token ];
				result &= ParseSet( pos, data );
			}
		}
		else
		{
			// expected '{'
			result = qfalse;
		}
	}

	return result;
}

////---------------------------
/// FFConfigParser::GetIncludes
//-------------------------------
//
//
//	Parameters:
//
//	Returns:
//
FFConfigParser::TInclude& FFConfigParser::GetIncludes( const char *name )
{
	TMap::iterator itMap = mMap.find( name );
	if ( itMap != mMap.end() )
		return (*itMap).second.include;

	// No includes present
	static TInclude emptyInclude;
	return emptyInclude;
}

const char * FFConfigParser::GetFFSet( CImmDevice *Device )
{
	char devName[ FF_MAX_PATH ];
	const char *ffset = NULL;

	//
	//	Check explicit name
	//

	devName[0] = 0;
	Device->GetProductName( devName, FF_MAX_PATH - 1 );
	for
	(	TMap::iterator itmap = mMap.begin()
	;	itmap != mMap.end()
	;	itmap++
	){
		TDevice::iterator itdev;

		itdev = (*itmap).second.device.find( devName );
		if ( itdev != (*itmap).second.device.end() )
			ffset = (*itmap).first.c_str();
	}


	//
	//	Check device defaults
	//

	for
	(	int i = 0
	;	!ffset && i < mDefaultPriority.size()
	;	i++
	){
		int defaultTechType;
		DWORD	productType = Device->GetProductType();
		WORD	deviceType	= HIWORD( productType );
		WORD	techType	= LOWORD( productType );

		defaultTechType = mDefaultPriority[ i ];

		//
		//	Check for minimum required features
		//

		if ( (techType & defaultTechType) >= defaultTechType )
		{
			//
			//	Check that device exists in this technology section
			//

			TDeviceType::iterator itDeviceType = mDefaultSet[ defaultTechType ].find( deviceType );
			if ( itDeviceType != mDefaultSet[ defaultTechType ].end() )
			{
				ffset = (*itDeviceType).second.c_str();
			}
		}

		//
		//	If not, try next technology section
		//
	}

	return ffset;
}

#endif // _IMMERSION