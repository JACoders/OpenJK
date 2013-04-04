
#include "xb_settings.h"
#include <xtl.h>
#include "../game/q_shared.h"
#include "qcommon.h"
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"

#define SETTINGS_VERSION	0x00082877
#define SETTINGS_DIRNAME	"Settings"
#define SETTINGS_FILENAME	"settings.dat"
#define SETTINGS_IMAGE		"saveimage.xbx"
#define SETTINGS_IMAGE_SRC	"d:\\base\\media\\settings.xbx"

// The one copy of Settings:
XBSettings Settings;
const DWORD settingsSize = sizeof(Settings);
const DWORD sigSize = sizeof(XCALCSIG_SIGNATURE);

// This isn't user data, don't put it in XBSettings!
enum XBSettingsStatus
{
	SETTINGS_OK,		// Everything is ok
	SETTINGS_MISSING,	// File is not on disk
	SETTINGS_CORRUPT,	// File on disk is corrupt
	SETTINGS_FAILED,	// General error
};
XBSettingsStatus SettingsStatus;

bool settingsDisabled = false;

const char *buttonConfigStrings[3] = {
	"weaponsbias",
	"forcebias",
	"southpaw",
};

const char *triggerConfigStrings[2] = {
	"default",
	"southpaw",
};

XBSettings::XBSettings( void )
{
	version = SETTINGS_VERSION;

	// Defaults:
	invertAim[0]		= invertAim[1]		= false;

	thumbstickMode[0]	= thumbstickMode[1]	= 0;
	buttonMode[0]		= buttonMode[1]		= 0;
	triggerMode[0]		= triggerMode[1]	= 0;

	rumble[0]			= rumble[1]			= 1;
	autolevel[0]		= autolevel[0]		= 0;
	autoswitch[0]		= autoswitch[1]		= 1;
	sensitivityX[0]		= sensitivityX[1]	= 2.0f;
	sensitivityY[0]		= sensitivityY[1]	= 2.0f;

	hotswapSP[0] = hotswapSP[1] = hotswapSP[2] = -1;
	hotswapMP[0] = hotswapMP[1] = -1;
	hotswapMP[2] = hotswapMP[3] = -1;

	effectsVolume = 1.0f;
	musicVolume = 0.25f;
	voiceVolume = 1.0f;

	subtitles = 0;

	voiceMode = 2;
	voiceMask = 0;
	appearOffline = 0;

	brightness = 6.0f;
}

// Write the current stored settings to the HD:
bool XBSettings::Save( void )
{
	// Do nothing if user chose "Continue Without Saving"
	if( settingsDisabled )
		return true;

	char settingsPath[128];
	char *pathEnd;
	DWORD dwWritten;

	// Build the settings directory:
	unsigned short wideName[128];
	mbstowcs( wideName, SETTINGS_DIRNAME, sizeof(wideName) );

	// Open/create the settings directory:
	if (XCreateSaveGame( "U:\\", wideName, OPEN_ALWAYS, 0, settingsPath, sizeof(settingsPath) ) != ERROR_SUCCESS )
	{
		SettingsStatus = SETTINGS_FAILED;
		return false;
	}

	// Build path to settings file:
	pathEnd = settingsPath + strlen( settingsPath );
	strcpy( pathEnd, SETTINGS_FILENAME );

	// Open/create the settings file:
	HANDLE hFile = CreateFile( settingsPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
	{
		SettingsStatus = SETTINGS_FAILED;
		return false;
	}

	// Write the data:
	if( !WriteFile( hFile, this, settingsSize, &dwWritten, NULL ) || (dwWritten != settingsSize) )
	{
		SettingsStatus = SETTINGS_FAILED;
		CloseHandle( hFile );
		return false;
	}

	// Sign the data:
	XCALCSIG_SIGNATURE xsig;
	if( !Sign( &xsig ) )
	{
		SettingsStatus = SETTINGS_FAILED;
		CloseHandle( hFile );
		return false;
	}

	// Write signature:
	if( !WriteFile( hFile, &xsig, sigSize, &dwWritten, NULL ) || (dwWritten != sigSize) )
	{
		SettingsStatus = SETTINGS_FAILED;
		CloseHandle( hFile );
		return false;
	}

	// Truncate and close file:
	SetEndOfFile( hFile );
	CloseHandle( hFile );

	// Copy the save image over:
	strcpy( pathEnd, SETTINGS_IMAGE );
	CopyFile( SETTINGS_IMAGE_SRC, settingsPath, FALSE );

	return true;
}

// Read saved settings from the HD:
bool XBSettings::Load( void )
{
	// Do nothing if user chose "Continue Without Saving"
	if( settingsDisabled )
		return true;

	char settingsPath[128];
	char *pathEnd;
	DWORD dwRead;

	// Build the settings directory:
	unsigned short wideName[128];
	mbstowcs( wideName, SETTINGS_DIRNAME, sizeof(wideName) );

	// Open the settings directory:
	if( XCreateSaveGame( "U:\\", wideName, OPEN_EXISTING, 0, settingsPath, sizeof(settingsPath) ) != ERROR_SUCCESS )
	{
		SettingsStatus = SETTINGS_MISSING;
		return false;
	}

	// Build path to settings file:
	pathEnd = settingsPath + strlen( settingsPath );
	strcpy( pathEnd, SETTINGS_FILENAME );

	HANDLE hFile = CreateFile( settingsPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		return false;
	}

	// Verify file size:
	if( GetFileSize( hFile, NULL ) != (settingsSize + sigSize) )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		CloseHandle( hFile );
		return false;
	}

	// Temp struct to read data into:
	XBSettings temp;
	if( !ReadFile( hFile, &temp, settingsSize, &dwRead, NULL ) || (dwRead != settingsSize) )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		CloseHandle( hFile );
		return false;
	}

	// Calculate signature over the read-in data:
	XCALCSIG_SIGNATURE xsig;
	if( !temp.Sign( &xsig ) )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		CloseHandle( hFile );
		return false;
	}

	// Read in stored signature:
	XCALCSIG_SIGNATURE storedSig;
	if( !ReadFile( hFile, &storedSig, sigSize, &dwRead, NULL ) || (dwRead != sigSize) )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		CloseHandle( hFile );
		return false;
	}

	// We're done with the file:
	CloseHandle( hFile );

	// Compare signatures:
	if( memcmp( &xsig, &storedSig, sigSize ) != 0 )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		return false;
	}

	// Lastly, verify that the version number is right:
	if( temp.version != SETTINGS_VERSION )
	{
		SettingsStatus = SETTINGS_CORRUPT;
		return false;
	}

	// OK. The data checks out!
	*this = temp;

	// TODO: Range-check all the values?

	return true;
}

void XBSettings::Delete( void )
{
	// Build the settings directory:
	unsigned short wideName[128];
	mbstowcs( wideName, SETTINGS_DIRNAME, sizeof(wideName) );

	// Delete the game:
	XDeleteSaveGame( "U:\\", wideName );
}

bool XBSettings::Corrupt( void )
{
	return (SettingsStatus == SETTINGS_CORRUPT);
}

bool XBSettings::Missing( void )
{
	return (SettingsStatus == SETTINGS_MISSING);
}

// Copy all stored settings into cvars
void XBSettings::SetAll( void )
{
	int clNum = ClientManager::ActiveClientNum();

	ClientManager::ActiveClient().cg_pitch = invertAim[clNum] ? 0.022f : -0.022f;

	Cbuf_ExecuteText( EXEC_APPEND, va("exec cfg/uibuttonConfig%d.cfg\n", buttonMode[clNum]) );
	Cbuf_ExecuteText( EXEC_APPEND, va("exec cfg/triggersConfig%d.cfg\n", triggerMode[clNum]) );

	// Do both of these, easier than checking:
	Cvar_SetValue( "in_useRumble", rumble[0] );
	Cvar_SetValue( "in_useRumble2", rumble[1] );

	ClientManager::ActiveClient().cg_autolevel = autolevel[clNum];
	ClientManager::ActiveClient().cg_autoswitch = autoswitch[clNum];

	ClientManager::ActiveClient().cg_sensitivity = sensitivityX[clNum];
	ClientManager::ActiveClient().cg_sensitivityY = sensitivityY[clNum];

	if( hotswapMP[0] >= 0 )
		Cvar_SetValue( "hotswap0", hotswapMP[0] );
	else
		Cvar_Set( "hotswap0", "" );

	if( hotswapMP[1] >= 0 )
		Cvar_SetValue( "hotswap1", hotswapMP[1] );
	else
		Cvar_Set( "hotswap1", "" );

	if( hotswapMP[2] >= 0 )
		Cvar_SetValue( "hotswap2", hotswapMP[2] );
	else
		Cvar_Set( "hotswap2", "" );

	if( hotswapMP[3] >= 0 )
		Cvar_SetValue( "hotswap3", hotswapMP[3] );
	else
		Cvar_Set( "hotswap3", "" );

	Cvar_SetValue( "s_effects_volume", effectsVolume );
	Cvar_SetValue( "s_music_volume", musicVolume );
	Cvar_SetValue( "s_voice_volume", voiceVolume );
	Cvar_SetValue( "s_brightness_volume", brightness );
	extern void GLimp_SetGamma(float);
	GLimp_SetGamma(Cvar_VariableValue( "s_brightness_volume" ) / 5.0f);



	// Online options stuff is grabbed when it's needed
}

// Utility - signs the current contents of this XBSettings into the supplied struct:
bool XBSettings::Sign( XCALCSIG_SIGNATURE *pSig )
{
	// Start the signature:
	HANDLE hSig = XCalculateSignatureBegin( 0 );
	if( hSig == INVALID_HANDLE_VALUE )
		return false;

	// Build the signature
	if( XCalculateSignatureUpdate( hSig, (BYTE *) this, sizeof(*this) ) != ERROR_SUCCESS )
		return false;

	// Finish the signature:
	if( XCalculateSignatureEnd( hSig, pSig ) != ERROR_SUCCESS )
		return false;

	// Done!
	return true;
}

// Master switch for turning off settings when user picks
// "Continue Without Saving"
void XBSettings::Disable( void )
{
	settingsDisabled = true;
}

bool XBSettings::IsDisabled( void )
{
	return settingsDisabled;
}
