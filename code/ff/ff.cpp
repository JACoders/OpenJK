#include "common_headers.h"

#ifdef _IMMERSION

#define INITGUID	// this will need removing if already defined in someone else's module. Only one must exist in whole game

//#include "ff.h"
//#include "ff_ffset.h"
//#include "ff_compound.h"
//#include "ff_system.h"

FFSystem gFFSystem;

cvar_t	*use_ff;
cvar_t	*ensureShake;
cvar_t	*ff_developer;
#ifdef FF_DELAY
cvar_t	*ff_delay;
#endif
cvar_t	*ff_channels;

static const char *_pass = "SUCCEEDED";
static const char *_fail = "FAILED";

const char *gChannelName[ FF_CHANNEL_MAX ] =
{	"FF_CHANNEL_WEAPON"
,	"FF_CHANNEL_MENU"
,	"FF_CHANNEL_TOUCH"
,	"FF_CHANNEL_DAMAGE"
,	"FF_CHANNEL_BODY"
,	"FF_CHANNEL_FORCE"
,	"FF_CHANNEL_FOOT"
};

// Enable/Disable Com_Printf in FF_* functions
#if( 1 )
#ifdef FF_PRINT
#define FF_PROLOGUE( name, string )			qboolean result = qfalse; if ( FF_IsAvailable() ) { if ( ff_developer && ff_developer->integer ) Com_Printf( "%s: \"%s\" ", #name, string );
#define FF_PROLOGUE_NOQUOTE( name, string )	qboolean result = qfalse; if ( FF_IsAvailable() ) { if ( ff_developer && ff_developer->integer ) Com_Printf( "%s: %s ", #name, string );
#define FF_EPILOGUE							FF_EPILOGUE_NORETURN; return result;
#define FF_EPILOGUE_NORETURN				} if ( ff_developer && ff_developer->integer ) Com_Printf( "[%s]\n", ( result ? _pass : _fail ) );
#define FF_RESULT( function )				result = qboolean( function );
#else
#define FF_PROLOGUE( name, string )			qboolean result = qfalse; if ( FF_IsAvailable() ) { 
#define FF_PROLOGUE_NOQUOTE( name, string )	qboolean result = qfalse; if ( FF_IsAvailable() ) { 
#define FF_EPILOGUE							FF_EPILOGUE_NORETURN; return result;
#define FF_EPILOGUE_NORETURN				}
#define FF_RESULT( function )				result = qboolean( function );
#endif
#else
#define FF_PROLOGUE( name, string )			qboolean result = qfalse;
#define FF_PROLOGUE_NOQUOTE( name, string )	qboolean result = qfalse;
#define FF_EPILOGUE							return result;
#define FF_EPILOGUE_NORETURN
#define FF_RESULT( function )				result = qboolean( function );
#endif

////--------------
/// FF_IsAvailable
//------------------
//	Test to see if force feedback is currently operating. This is almost useless.
//	The only good it does currently is temporarily toggle effects on/off for users
//	amusement... feedback on, feedback off, feedback on, feedback off. Results are
//	instantaneous. FF_* calls basically skip themselves harmlessly.
//
//	Assumptions:
//	*	External system unloads this module if no device is present.
//	*	External system unloads this module if feedback is disabled when system (re)starts
//
//	Parameters:
//		None
//
//	Returns:
//	-	true: feedback currently enabled
//	-	false: feedback currently disabled
//
qboolean FF_IsAvailable(void)
{
	return (use_ff && use_ff->integer && gFFSystem.IsInitialized()) ? qtrue : qfalse;
}

qboolean FF_IsInitialized(void)
{
	return gFFSystem.IsInitialized();
}

////-------
/// FF_Init
//-----------
//	Initializes the force feedback system.
//
//	This function may be called multiple times to apply changes in cvars.
//
//	Assumptions:
//	*	If FF_Init returns qfalse, caller calls FF_Shutdown
//
//	Parameters:
//		None
//
//	Returns:
//	-	qtrue: module initialized properly.
//	-	qfalse: module experienced an error. Caller MUST call FF_Shutdown.
//
qboolean FF_Init( void )
{
	if ( !gFFSystem.IsInitialized() )
	{
		//
		//	Console variable setup
		//

#ifdef FF_CONSOLECOMMAND
		Cmd_AddCommand( "ff_stopall",	CMD_FF_StopAll		);
		Cmd_AddCommand( "ff_info",		CMD_FF_Info			);
#endif
		use_ff				= Cvar_Get( "use_ff",				"1",	CVAR_ARCHIVE /*| CVAR_LATCH*/); 
		ensureShake			= Cvar_Get( "ff_ensureShake",		"1",	CVAR_TEMP);
		ff_developer		= Cvar_Get( "ff_developer",			"0",	CVAR_TEMP);
		ff_channels			= Cvar_Get( "ff_channels",		FF_CHANNEL,	CVAR_ARCHIVE);
#ifdef FF_DELAY
		ff_delay			= Cvar_Get( "ff_delay",			FF_DELAY,	CVAR_ARCHIVE);
#endif
	}
	
	return qboolean // assumes external system will call FF_Shutdown in case of failure
	(	ff_channels != NULL
	&&	gFFSystem.Init( ff_channels->string )
	);
}

////-----------
/// FF_Shutdown
//---------------
//	Shut force feedback system down and free resources.
//
//	Assumptions:
//	*	Always called if FF_Init returns qfalse. ALWAYS. (Or memory leaks occur)
//	*	Never called twice in succession. (always in response to previous assumption)
//
//	Parameters:
//		None
//
//	Returns:
//		None
//
void FF_Shutdown(void)
{
#ifdef FF_CONSOLECOMMAND
	Cmd_RemoveCommand( "ff_stopall" );
	Cmd_RemoveCommand( "ff_info" );
#endif

	gFFSystem.Shutdown();
}

////-----------
/// FF_Register
//---------------
//	Loads a named effect from an .ifr file through the game's file system. The handle
//	returned is not tied to any particular device. The feedback system may even change
//	which device receives the effect without the need to restart the external system.
//	The is ONE EXCEPTION. If this module is not loaded when the registration phase
//	passes, the external system will need to be restarted to register effects properly.
//
//	Parameters:
//	*	name: effect name from .ifr (case-sensitive)
//	*	channel: channel to output effect. A channel may play on 0+ devices.
//	*	notfound: return a valid handle even if effect is not found
//		- Allows temporary disabling of a channel in-game without losing effects
//		- Only use with trusted effect names, not user input. See CMD_FF_Play.
//
//	Returns:
//		Handle to loaded effect
//
ffHandle_t FF_Register( const char *name, int channel, qboolean notfound )
{
	ffHandle_t ff = FF_HANDLE_NULL;

	// Removed console print... too much spam with AddLoopingForce.
/*
	FF_PROLOGUE( FF_Register, ( name ? name : "" ) );
	ff = gFFSystem.Register( name, channel, notfound );
	FF_RESULT( ff != FF_HANDLE_NULL );
	FF_EPILOGUE_NORETURN;
*/
	if ( FF_IsAvailable() )
		ff = gFFSystem.Register( name, channel, notfound );

	return ff;
}

////----------------
/// FF_EnsurePlaying
//--------------------
//	Starts an effect if the effect is not currently playing.
//	Does not restart currently playing effects.
//
//	Parameters:
//	*	ff: handle of an effect
//
//	Returns:
//	-	qtrue: effect started
//	-	qfalse: effect was not started
//
qboolean FF_EnsurePlaying(ffHandle_t ff)
{
	FF_PROLOGUE( FF_EnsurePlaying, gFFSystem.GetName( ff ) );
	FF_RESULT( gFFSystem.EnsurePlaying( ff ) );
	FF_EPILOGUE;
}

////-------
/// FF_Play
//-----------
//	Start an effect on its registered channel.
//
//	Parameters
//	*	ff: handle to an effect
//
//	Returns:
//	-	qtrue: effect started
//	-	qfalse: effect was not started
//
qboolean FF_Play(ffHandle_t ff)
{
	FF_PROLOGUE( FF_Play, gFFSystem.GetName( ff ) );
	FF_RESULT( gFFSystem.Play( ff ) );
	FF_EPILOGUE;
}

////----------
/// FF_StopAll
//--------------
//	Stop all currently playing effects.
//
//	Parameters:
//		None
//
//	Returns:
//	-	qtrue: no errors occurred
//	-	qfalse: an error occurred
//
qboolean FF_StopAll(void)
{
	FF_PROLOGUE( FF_StopAll, "" );
	FF_RESULT( gFFSystem.StopAll() );
	FF_EPILOGUE;
}

////-------
/// FF_Stop
//-----------
//	Stop an effect. Only returns qfalse if there's an error
//
//	Parameters:
//	*	ff: handle to a playing effect
//
//	Returns:
//	-	qtrue: no errors occurred
//	-	qfalse: an error occurred
//
qboolean FF_Stop(ffHandle_t ff)
{
	FF_PROLOGUE( FF_Stop, gFFSystem.GetName( ff ) );
	FF_RESULT( gFFSystem.Stop( ff ) );
	FF_EPILOGUE;
}


////--------
/// FF_Shake
//------------
//	Shake the mouse (play the special "shake" effect) at a given strength
//	for a given duration. The shake effect can be a compound containing
//	multiple component effects, but each component effect's magnitude and
//	duration will be set to the parameters passed in this function.
//
//	Parameters:
//	*	intensity [0..10000] - Magnitude of effect
//	*	duration [0..MAXINT] - Length of shake in milliseconds
//
//	Returns:
//	-	qtrue: shake started
//	-	qfalse: shake did not start
//
qboolean FF_Shake(int intensity, int duration)
{
	char message[64];
	message[0] = 0;
	sprintf( message, "intensity=%d, duration=%d", intensity, duration );
	FF_PROLOGUE_NOQUOTE( FF_Shake, message );
	FF_RESULT( gFFSystem.Shake( intensity, duration, qboolean( ensureShake->integer != qfalse ) ) );
	FF_EPILOGUE;
}

#ifdef FF_CONSOLECOMMAND

////--------------
/// CMD_FF_StopAll
//------------------
//	Console function which stops all currently playing effects
//
//	Parameters:
//		None
//
//	Returns:
//		None
//
void CMD_FF_StopAll(void)
{
	// Display messages
	if ( FF_StopAll() )
	{
		Com_Printf( "stopping all effects\n" );
	}
	else
	{
		Com_Printf( "failed to stop all effects\n" );
	}
}

////-----------
/// CMD_FF_Info
//---------------
//	Console function which displays various ff-system information.
//
//	Parameters:
//	*	'devices'	display list of ff devices currently connected
//	*	'channels'	display list of support ff channels
//	*	'order'		display search order used by ff name-resolution system (ff_ffset.cpp)
//	*	'files'		display currently loaded .ifr files sorted by device
//	*	'effects'	display currently registered effects sorted by device
//
//	Returns:
//		None
//
void CMD_FF_Info(void)
{
	TNameTable Unprocessed, Processed;
	int i, max;

	for
	(	i = 1, max = Cmd_Argc()
	;	i < max
	;	i++
	){
		Unprocessed.push_back( Cmd_Argv( i ) );
	}

	if ( Unprocessed.size() == 0 )
	{
		
		if ( ff_developer->integer )
			Com_Printf( "Usage: ff_info [devices] [channels] [order] [files] [effects]\n" );
		else
			Com_Printf( "Usage: ff_info [devices] [channels]\n" );

		return;
	}

	gFFSystem.Display( Unprocessed, Processed );

	if ( Unprocessed.size() > 0 )
	{
		Com_Printf( "invalid parameters:" );
		for
		(	i = 0
		;	i < Unprocessed.size()
		;	i++
		){
			Com_Printf( " %s", Unprocessed[ i ].c_str() );
		}

		if ( ff_developer->integer )
			Com_Printf( "Usage: ff_info [devices] [channels] [order] [files] [effects]\n" );
		else
			Com_Printf( "Usage: ff_info [devices] [channels]\n" );
	}
}

#endif // FF_CONSOLECOMMAND

#endif // _IMMERSION