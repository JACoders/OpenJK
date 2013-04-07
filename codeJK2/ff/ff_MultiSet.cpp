#include "common_headers.h"

#ifdef _IMMERSION

#include "..\win32\win_local.h"

////----------------
/// FFMultiSet::Init
//--------------------
//	Initializes all attached force feedback devices. An empty FFSet is created
//	for each device. Each device will have its own copy of whatever .ifr file
//	set 'config' specifies.
//
//	Always pair with clear()
//
qboolean FFMultiSet::Init( FFSystem::Config &config )
{
	mConfig = &config;

#ifdef FF_PRINT
	Com_Printf( "Feedback devices:\n" );
#endif

	HINSTANCE hInstance = (HINSTANCE)g_wv.hInstance;
	HWND hWnd = (HWND)g_wv.hWnd;

	mDevices = new CImmDevices;
	if ( mDevices && mDevices->CreateDevices( hInstance, hWnd ) )
	{
		for
		(	int i = 0
		;	i < mDevices->GetNumDevices()
		;	i++
		){
			FFSet *ffSet = NULL;
			ffSet = new FFSet( config, mDevices->GetDevice( i ) ); 
			if ( ffSet )
			{
#ifdef FF_PRINT
				char ProductName[ FF_MAX_PATH ];
				*ProductName = 0;
				mDevices->GetDevice( i )->GetProductName( ProductName, FF_MAX_PATH - 1 );
				Com_Printf( "%d) %s\n", i, ProductName );
#endif
				mSet.push_back( ffSet );
			}
		}
	}

	return qboolean( mSet.size() );
}

////------------------------------
/// FFMultiSet::GetRegisteredNames
//----------------------------------
//
//
qboolean FFMultiSet::GetRegisteredNames( TNameTable &NameTable )
{
	for
	(	int i = 0
	;	i < mSet.size()
	;	i++
	){
		mSet[ i ]->GetRegisteredNames( NameTable );
	}

	return qtrue;
}

////-------------------
/// FFMultiSet::StopAll
//-----------------------
//	Stops all effects in every FFSet.
//	Returns qfalse if any return false.
//
qboolean FFMultiSet::StopAll()
{
	qboolean result = qtrue;

	for
	(	int i = 0
	;	i < mSet.size()
	;	i++
	){
		result &= mSet[ i ]->StopAll();
	}

	return result;
}

////-----------------
/// FFMultiSet::clear
//---------------------
//	Cleans up.
//
void FFMultiSet::clear()
{
	mConfig = NULL;
	for
	(	int i = 0
	;	i < mSet.size()
	;	i++
	){
		DeletePointer( mSet[ i ] );
	}
	mSet.clear();
	DeletePointer( mDevices );
}

#ifdef FF_CONSOLECOMMAND

void FFMultiSet::GetDisplayTokens( TNameTable &Tokens )
{
	FFSet::GetDisplayTokens( Tokens );
}

////-------------------
///	FFMultiSet::Display
//-----------------------
//
//
void FFMultiSet::Display( TNameTable &Unprocessed, TNameTable &Processed )
{
	for
	(	int i = 0
	;	i < mSet.size()
	;	i++
	){
		TNameTable Temp1, Temp2;
		Temp1.clear();
		Temp2.clear();
		Temp1.insert( Temp1.begin(), Processed.begin(), Processed.end() );
		mSet[ i ]->Display( Temp1, Temp2 );
	}
}

#endif // FF_CONSOLECOMMAND

#endif // _IMMERSION