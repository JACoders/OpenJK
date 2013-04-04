
#ifndef __XB_SETTINGS_H
#define __XB_SETTINGS_H

#include <xtl.h>

enum XBStartupState
{
	STARTUP_LOAD_SETTINGS,
	STARTUP_COMBINED_SPACE_CHECK,
	STARTUP_GAME_SPACE_CHECK,
	STARTUP_INVITE_CHECK,
	STARTUP_FINISH,
};

// Minimum save size on Xbox. Bleh:
#define SETTINGS_NUM_BLOCKS	4

struct XBSettings
{
	// Magic number/revision stamp:
	unsigned long version;

	// Controls, etc... One for SP/P1 in MP, other for P2 in MP:
	bool invertAim[2];
	int thumbstickMode[2];
	int buttonMode[2];
	int triggerMode[2];
	int rumble[2];
	int autolevel[2];
	int autoswitch[2];
	float sensitivityX[2];
	float sensitivityY[2];

	// Black/White/X assignments, SP:
	int hotswapSP[3];

	// Black/White for players one & two, MP:
	int hotswapMP[4];

	// A/V settings, Global:
	float effectsVolume;
	float musicVolume;
	float voiceVolume;
	float brightness;

	// Subtitles, only used in SP:
	int subtitles;

	// Voice/Live options, only used in MP:
	int voiceMode;
	int voiceMask;
	int appearOffline;

// INTERFACE:

	XBSettings( void );

	bool Save( void );
	bool Load( void );
	void Delete( void );

	// For determining why a Save/Load failed:
	bool Missing( void );
	bool Corrupt( void );

	// This copies all settings from the Settings struct to their various cvars
	void SetAll( void );

	// Turn off the settings file completely:
	void Disable( void );

	// Has the user turned off saving (by choosing "Continue Without Saving")?
	bool IsDisabled( void );

#ifdef XBOX_DEMO
	void RestoreDefaults( void );
#endif

private:
	bool Sign( XCALCSIG_SIGNATURE *pSig );
};

// One global copy (declared in xb_settings.cpp)
extern XBSettings Settings;

#endif
