// jawa.cpp : Defines the entry point for the application.

#include "stdafx.h"

#ifndef _DEBUG
#error Jawa should only be used in debug mode!
#endif

// Our (many) game sessions
#define MAX_SESSIONS 10
CSession sessions[MAX_SESSIONS];

// Handle the logon task
XONLINETASK_HANDLE	g_hLogonTask;

//-----------------------------------------------------------------------------
// Name: Print()
//-----------------------------------------------------------------------------
VOID __cdecl Print( const WCHAR* strFormat, ... )
{
	const int MAX_OUTPUT_STR = 80;
	WCHAR strBuffer[ MAX_OUTPUT_STR ];
	va_list pArglist;

	va_start( pArglist, strFormat );   
	INT iChars = wvsprintfW( strBuffer, strFormat, pArglist );
	assert( iChars < MAX_OUTPUT_STR );
	OutputDebugStringW( L"\n*** SimpleAuth: " );
	OutputDebugStringW( strBuffer );
	OutputDebugStringW( L"\n\n" );
	(VOID)iChars; // avoid compiler warning
	va_end( pArglist );
}

//-----------------------------------------------------------------------------
// Name: SignIn()
//-----------------------------------------------------------------------------
BOOL SignIn()
{
	XONLINE_USER StoredUsers[ XONLINE_MAX_STORED_ONLINE_USERS ];
	DWORD dwNumStoredUsers;
	HRESULT hr = XOnlineGetUsers( StoredUsers, &dwNumStoredUsers );
	assert( SUCCEEDED( hr ) );
	assert( dwNumStoredUsers );    

	XONLINE_USER LogonUsers[ XONLINE_MAX_LOGON_USERS ] = { 0 }; // Initially zeroed
	LogonUsers[0] = StoredUsers[0]; 
	const DWORD Services[]		= { XONLINE_MATCHMAKING_SERVICE };
	const DWORD dwNumServices	= sizeof( Services ) / sizeof( Services[0] );

	hr = XOnlineLogon( LogonUsers, Services, dwNumServices, NULL, &g_hLogonTask );
	assert( hr == S_OK );

	// 1. Check for system authentication errors.
	do { hr = XOnlineTaskContinue( g_hLogonTask ); } while ( hr == XONLINETASK_S_RUNNING );
	assert( hr == XONLINE_S_LOGON_CONNECTION_ESTABLISHED );

	// 2. Check for user authentication errors.
	PXONLINE_USER Users = XOnlineGetLogonUsers();
	assert( Users );

	for( DWORD i = 0; i < XONLINE_MAX_LOGON_USERS; ++i )
	{
		if( Users[i].xuid.qwUserID != 0 ) // A valid user
			assert( Users[i].hr == S_OK || Users[i].hr == XONLINE_S_LOGON_USER_HAS_MESSAGE );
	}

	// 3. Finally check the requested services
	for( DWORD i = 0; i < dwNumServices; ++i )
	{
		hr = XOnlineGetServiceInfo( Services[i], NULL );
		assert( hr == S_OK );
	}

	return TRUE;    
}


//-----------------------------------------------------------------------------
// Name: main()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
void __cdecl main()
{
	XInitDevices( 0, NULL );
	while ( XGetDeviceEnumerationStatus() == XDEVICE_ENUMERATION_BUSY ) {}

	// We need MANY keys registered!
	XNetStartupParams xnsp;
	ZeroMemory( &xnsp, sizeof(xnsp) );
	xnsp.cfgSizeOfStruct				= sizeof(xnsp);
	xnsp.cfgKeyRegMax					= 255;	// 4
	xnsp.cfgSecRegMax					= 255;	// 32
	XNetStartup( &xnsp );

	HRESULT hr = XOnlineStartup( NULL );
	assert( SUCCEEDED( hr ) );

	// Need to log in first.
	if( !SignIn() )
		return;

	char sessionName[16] = { 0 };
	{
		// Always generate worst case display
		CSession *sess = &sessions[0];
		sess->PrivateFilled = 0;
		sess->PrivateOpen = 0;
		sess->PublicFilled = 5;
		sess->PublicOpen = 3;
		sess->TotalPlayers = 5;

		sess->GameType = 4;	// PowerDuel
		sess->CurrentMap = 8; // Imperial Control Room

		strcpy( sessionName, "WWWWWWWWWWWWWWQ" );
		WCHAR wSessionName[16];
		wsprintfW( wSessionName, L"%hs", sessionName );
		sess->SetSessionName( wSessionName );

		sess->FriendlyFire = 0;
		sess->SaberOnly = 0;
		sess->JediMastery = 0;
		sess->Dedicated = 0;

		HRESULT hr = sess->Create();
		if (hr != S_OK)
		{
			OutputDebugString( "Session creation failed" );
		}

		do
		{
			hr = XOnlineTaskContinue( g_hLogonTask );
			if ( FAILED( hr ) )
			{
				OutputDebugString( "Logon task failed" );
			}
			hr = sess->Process();
		} while ( sess->IsCreating() );
	}
	for( int i = 1; i < MAX_SESSIONS; ++i )
	{
		CSession *sess = &sessions[i];
		int totalSlots = (rand() % 5) + 4;	// 4 - 8
		int publicSlots = (rand() % totalSlots); // 0 - (total-1)
		int privateSlots = totalSlots - publicSlots; // 1 - total
		int privateTaken = (rand() % privateSlots) + 1; // 1 - private
		int publicTaken = (rand() % (publicSlots+1)); // 0 - public
		sess->PrivateFilled = 1;	//privateTaken;
		sess->PrivateOpen = 2;	//privateSlots - privateTaken;
		sess->PublicFilled = 3;	//publicTaken;
		sess->PublicOpen = 4;	//publicSlots - publicTaken;
		sess->TotalPlayers = 4;	//privateTaken + publicTaken;

		sess->GameType = rand() % 10;
		sess->CurrentMap = rand() % 23;

		XNetRandom( (BYTE *)&sessionName[0], 10 );
		for( int j = 0; j < 10; ++j )
			sessionName[j] = (((unsigned char)sessionName[j]) % 26) + 'A';
		WCHAR wSessionName[11];
		wsprintfW( wSessionName, L"%hs", sessionName );
		wSessionName[10] = 0;
		sess->SetSessionName( wSessionName );

		sess->FriendlyFire = rand() % 2;
		sess->SaberOnly = rand() % 2;
		sess->JediMastery = rand() % 7;
		sess->Dedicated = rand() % 2;

		HRESULT hr = sess->Create();
		if (hr != S_OK)
		{
			OutputDebugString( "Session creation failed" );
		}

		do
		{
			hr = XOnlineTaskContinue( g_hLogonTask );
			if ( FAILED( hr ) )
			{
				OutputDebugString( "Logon task failed" );
			}
			hr = sess->Process();
		} while ( sess->IsCreating() );
	}

    while( TRUE )
    {
		// Service things
		hr = XOnlineTaskContinue( g_hLogonTask );
		assert( SUCCEEDED( hr ) );
		for( int j = 0; j < MAX_SESSIONS; ++j )
		{
			hr = sessions[j].Process();
			assert( SUCCEEDED( hr ) );
		}
    }
}


