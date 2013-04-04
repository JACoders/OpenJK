#include "common_headers.h"

#ifdef _IMMERSION

//#include "ff.h"
//#include "ff_ffset.h"
//#include "ff_compound.h"
//#include "ff_system.h"

extern cvar_t *ff_developer;
#ifdef FF_DELAY
extern cvar_t *ff_delay;
#endif

extern FFSystem gFFSystem;

FFSet::FFSet( FFConfigParser &ConfigParser, CImmDevice *Device )
:	mParser( ConfigParser )
,	mDevice( Device )
,	mInclude()
,	mIncludePath()
{
	const char *setname = mParser.GetFFSet( mDevice );
	if ( setname )
	{
		TProject temp;
		mInclude.push_back( temp );
		mIncludePath.push_back( setname );
		InitIncludes( setname );
	}
}

FFSet::~FFSet()
{
	for
	(	TInclude::iterator itInclude = mInclude.begin()
	;	itInclude != mInclude.end()
	;	itInclude++
	){
		for
		(	TProject::iterator itProject = (*itInclude).begin()
		;	itProject != (*itInclude).end()
		;	itProject++
		){
			DeletePointer( (*itProject).second );
		}
	}
}

void FFSet::InitIncludes( const char *setname )
{
	FFConfigParser::TInclude &include = mParser.GetIncludes( setname );

	for	// each include listed in config file
	(	int i = 0
	;	i < include.size()
	;	i++
	){
		for	// each include entered into current list
		(	unsigned int j = 0
		;	j < mIncludePath.size()
		;	j++
		){
			if ( include[ i ] == mIncludePath[ j ] ) // exists in current list
				break;
		}

		if ( j == mIncludePath.size() ) // does not exist in current list
		{
			TProject temp;
			mInclude.push_back( temp );
			mIncludePath.push_back( include[ i ] );
			InitIncludes( include[ i ].c_str() ); // recurse
		}
	}
}

MultiEffect* FFSet::Register( const char *path, qboolean create )
{
	char outpath[ FF_MAX_PATH ];
	MultiEffect* effect = NULL;

	if ( FS_VerifyName( "FFSet::Register", path, outpath ) )
	{
		for	// each included set
		(	int i = 0
		;	i < mInclude.size() && !effect
		;	i++
		){
			char setpath[ FF_MAX_PATH ], *afterincludepath;
			// need to use explicit path if provided.
			sprintf( setpath, "%s/%s", mIncludePath[ i ].c_str(), UncommonDirectory( path, mIncludePath[ i ].c_str() ) );
			afterincludepath = setpath + mIncludePath[ i ].length() + 1;

			for	// each possible file/effectname combination
			(	int separator = _rcpos( afterincludepath, '/' )
			;	separator >= 0 && !effect
			;	separator = _rcpos( afterincludepath, '/', separator )
			){
				CImmProject *immProject;
				char temp[4];

				temp[0] = 0;
				afterincludepath[separator] = 0;
				if ( stricmp( afterincludepath + separator - 4, ".ifr" ) )
				{
					memcpy( temp, afterincludepath + separator + 1, 4 );
					sprintf( afterincludepath + separator, ".ifr" );
				}
				
				immProject = NULL;

				TProject::iterator itProject = mInclude[ i ].find( afterincludepath );
				if ( itProject != mInclude[ i ].end() )
				{
					immProject = (*itProject).second;
				}
				else if ( create )
				{
					void *buffer = LoadFile( setpath );
					if ( buffer )
					{
						immProject = new CImmProject;

						if ( immProject )
						{
							if ( !immProject->LoadProjectFromMemory( buffer, mDevice ) )
							{
								DeletePointer( immProject );
#ifdef FF_PRINT
								if ( ff_developer->integer ) 
									Com_Printf( "...Corrupt or invalid file: %s\n", setpath );
							}
							else
							{
								if ( ff_developer->integer )
									Com_Printf( "...Adding file \"%s\"\n", setpath );
#endif FF_PRINT
							}
						}

						FS_FreeFile( buffer );
					}

					mInclude[ i ][ afterincludepath ] = immProject;
				}

				if ( temp[ 0 ] )
				{
					afterincludepath[ separator ] = '/';
					memcpy( afterincludepath + separator + 1, temp, 4 );
				}

				if ( immProject )
				{
					effect = (MultiEffect*)immProject->GetCreatedEffect( afterincludepath + separator + 1 );
					if ( !effect && create )
					{
						effect = (MultiEffect*)immProject->CreateEffect( afterincludepath + separator + 1, mDevice, IMM_PARAM_NODOWNLOAD );
#ifdef FF_DELAY
						// Delay the effect (better sound synchronization)
						if ( effect
						&&	 ff_delay
						//&&	 *ff_delay
						){
							effect->ChangeStartDelay( ff_delay->integer );
						}
#endif // FF_DELAY
					}
				}
			}
		}
	}

	return effect;
}

qboolean FFSet::StopAll( void )
{
	for
	(	TInclude::iterator itInclude = mInclude.begin()
	;	itInclude != mInclude.end()
	;	itInclude++
	){
		for
		(	TProject::iterator itProject = (*itInclude).begin()
		;	itProject != (*itInclude).end()
		;	itProject++
		){
			if ( (*itProject).second )
				(*itProject).second->Stop();
		}
	}

	return qtrue;
}

void FFSet::GetRegisteredNames( TNameTable &NameTable )
{
	FFSystem::Handle ffHandle = gFFSystem.GetHandles();

	for
	(	int IncludeIndex = 0
	;	IncludeIndex < mInclude.size()
	;	IncludeIndex++
	){
		for
		(	TProject::iterator itProject = mInclude[ IncludeIndex ].begin()
		;	itProject != mInclude[ IncludeIndex ].end()
		;	itProject++
		){
			char effectname[ FF_MAX_PATH ];
			int i;

			if ( !(*itProject).second )
				continue;
			
			i = 0;
			
			for
			(	MultiEffect *Effect = (MultiEffect*)(*itProject).second->GetCreatedEffect( i )
			;	Effect
			;	Effect = (MultiEffect*)(*itProject).second->GetCreatedEffect( ++i )
			){
				sprintf( effectname, "%s/%s/%s", mIncludePath[ IncludeIndex ].c_str(), (*itProject).first.c_str(), Effect->GetName() );

				for
				(	int i = 0
				;	i < ffHandle.size()
				;	i++
				){
					ChannelCompound::Set &compound = ffHandle[ i ].GetSet();
					if
					(	NameTable[ i ].length() == 0
					&&	compound.find( Effect ) != compound.end()
					){
						NameTable[ i ] = effectname;
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////

#ifdef FF_CONSOLECOMMAND

void FFSet::GetDisplayTokens( TNameTable &Tokens )
{
	if ( ff_developer->integer )
	{
		Tokens.push_back( "order" );
		Tokens.push_back( "files" );
	}
}


void FFSet::Display( TNameTable &Unprocessed, TNameTable &Processed )
{
	for
	(	TNameTable::iterator itName = Unprocessed.begin()
	;	itName != Unprocessed.end()
	;
	){
		if ( stricmp( "order", (*itName).c_str() ) == 0 )
		{
			if ( ff_developer->integer )
				DisplaySearchOrder();
			//else
			//	Com_Printf( "\"order\" only available when ff_developer is set\n" );

			Processed.push_back( *itName );
			itName = Unprocessed.erase( itName );
		}
		else
		if ( stricmp( "files", (*itName).c_str() ) == 0 )
		{
			if ( ff_developer->integer )
				DisplayLoadedFiles();
			//else
			//	Com_Printf( "\"files\" only available when ff_developer is set\n" );

			Processed.push_back( *itName );
			itName = Unprocessed.erase( itName );
		}
		else
		{
			itName++;
		}
	}

}

void FFSet::DisplaySearchOrder( void )
{
	char ProductName[ FF_MAX_PATH ];
	*ProductName = 0;
	mDevice->GetProductName( ProductName, FF_MAX_PATH - 1 );
	Com_Printf( "[search order] -\"%s\"\n", ProductName );

	for
	(	int i = 0
	;	i < mInclude.size()
	;	i++
	){
		Com_Printf( "%d) %s\n", i, mIncludePath[ i ].c_str() );
	}
}

void FFSet::DisplayLoadedFiles( void )
{
	int	total = 0;
#ifdef _DEBUG
	int nulltotal = 0;		// Variable to indicate how bad my algorithm is
#endif

	char ProductName[ FF_MAX_PATH ];
	*ProductName = 0;
	mDevice->GetProductName( ProductName, FF_MAX_PATH - 1 );
	Com_Printf( "[loaded files] -\"%s\"\n", ProductName );

	for
	(	int i = 0
	;	i < mInclude.size()
	;	i++
	){
		for
		(	TProject::iterator itProject = mInclude[ i ].begin()
		;	itProject != mInclude[ i ].end()
		;	itProject++
		){
			if ( (*itProject).second )
			{
				++total;
				Com_Printf( "%s/%s\n", mIncludePath[ i ].c_str(), (*itProject).first.c_str() );
			}
#ifdef _DEBUG
			else
			{
				++nulltotal;
				Com_Printf( "%s/%s [null]\n", mIncludePath[ i ].c_str(), (*itProject).first.c_str() );
			}
#endif
		}
	}

	Com_Printf( "Total: %d files\n", total );
#ifdef _DEBUG
	Com_Printf( "Total: %d null files\n", nulltotal );
#endif
}

#endif // FF_CONSOLECOMMAND

#endif // _IMMERSION