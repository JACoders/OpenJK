
/*****************************************************************************
 * name:		cl_cin_console.cpp
 *
 * desc:		video and cinematic playback interface for Xbox (using Bink)
 *
 *****************************************************************************/

#include "client.h"
#include "../win32/win_local.h"
#include "../win32/win_input.h"
#include "../win32/glw_win_dx8.h"
#include "BinkVideo.h"

//#define XBOX_VIDEO_PATH "d:\\base\\video\\"
char XBOX_VIDEO_PATH[64] = "d:\\base\\video\\";
#define SHADER_VIDEO_PATH "z:\\"

BinkVideo bVideo;	// bink video object
connstate_t	previousState = CA_UNINITIALIZED;	// previous cinematic state

struct CinematicData
{
	char	filename[MAX_OSPATH];	// No path, no extension
	int 	x, y, w, h;				// Dimensions
	int		bits;					// Flags (loop, silent, shader)
};

// We have a fixed lookup table of all cinematics that can be played
// Video handles are just indices into the array. An entry is not
// considered initialized until its width is nonzero
CinematicData cinFiles[] = {
	// Opening logos
	{ "logos", 0, 0, 0, 0, 0 },
	// Attract sequence
	{ "attract", 0, 0, 0, 0, 0 },

	// Planet shaders
	{ "cos", 0, 0, 0, 0, 0 },
	{ "bakura", 0, 0, 0, 0, 0 },
	{ "blenjeel", 0, 0, 0, 0, 0 },
	{ "chandrila", 0, 0, 0, 0, 0 },
	{ "core", 0, 0, 0, 0, 0 },
	{ "ast", 0, 0, 0, 0, 0 },
	{ "dosunn", 0, 0, 0, 0, 0 },
	{ "krildor", 0, 0, 0, 0, 0 },
	{ "narkreeta", 0, 0, 0, 0, 0 },
	{ "ordman", 0, 0, 0, 0, 0 },
	{ "tanaab", 0, 0, 0, 0, 0 },
	{ "tatooine", 0, 0, 0, 0, 0 },
	{ "yalara", 0, 0, 0, 0, 0 },
	{ "zonju", 0, 0, 0, 0, 0 },

	// Others
//	{ "jk0101_sw", 0, 0, 0, 0, 0 },	// Folded into ja01!
//	{ "ja01", 0, 0, 0, 0, 0 },		// Contains the text crawl, so must be localized:
	{ "ja01_e", 0, 0, 0, 0, 0 },
	{ "ja01_f", 0, 0, 0, 0, 0 },
	{ "ja01_d", 0, 0, 0, 0, 0 },
	{ "ja02", 0, 0, 0, 0, 0 },
	{ "ja03", 0, 0, 0, 0, 0 },
	{ "ja04", 0, 0, 0, 0, 0 },
	{ "ja05", 0, 0, 0, 0, 0 },
	{ "ja06", 0, 0, 0, 0, 0 },
	{ "ja07", 0, 0, 0, 0, 0 },
	{ "ja08", 0, 0, 0, 0, 0 },
	{ "ja09", 0, 0, 0, 0, 0 },
	{ "ja10", 0, 0, 0, 0, 0 },
	{ "ja11", 0, 0, 0, 0, 0 },
	{ "ja12", 0, 0, 0, 0, 0 },
};

const int cinNumFiles = sizeof(cinFiles) / sizeof(cinFiles[0]);
static int currentHandle = -1;

// Stupid PC filth
static qboolean qbInGameCinematicOnStandBy = qfalse;
static char	 sInGameCinematicStandingBy[MAX_QPATH];

bool CIN_PlayAllFrames( const char *arg, int x, int y, int w, int h, int systemBits, bool keyBreakAllowed );

/********
CIN_CloseAllVideos
Stops all currently running videos
*********/
void CIN_CloseAllVideos(void)
{
	// Stop the current bink video
	bVideo.Stop();
	currentHandle = -1;
}

/********
CIN_StopCinematic

handle	- Not used
return	- FMV status

Stops the current cinematic
*********/
e_status CIN_StopCinematic(int handle)
{
	assert( handle == currentHandle );
	currentHandle = -1;

	if(previousState != CA_UNINITIALIZED)
	{
		cls.state = previousState;
		previousState = CA_UNINITIALIZED;
	}
	if(bVideo.GetStatus() != NS_BV_STOPPED)
	{
		bVideo.Stop();
	}
	return FMV_EOF;
}

/********
CIN_RunCinematic

handle	- Ensure that the supplied cinematic is the one running
return	- FMV status

Fetch and decompress the pending frame
*********/
e_status CIN_RunCinematic (int handle)
{
	if (handle < 0 || handle >= cinNumFiles || !cinFiles[handle].w)
	{
		assert( 0 );
		return FMV_EOF;
	}

	// If we weren't playing a movie, or playing the wrong one - start up
	if (handle != currentHandle)
	{
		bool shader = cinFiles[handle].bits & CIN_shader;

		CIN_StopCinematic(currentHandle);
		if (!bVideo.Start(
				va("%s%s.bik",
					shader ? SHADER_VIDEO_PATH : XBOX_VIDEO_PATH,
					cinFiles[handle].filename),
				cinFiles[handle].x, cinFiles[handle].y,
				cinFiles[handle].w, cinFiles[handle].h))
		{
			return FMV_EOF;
		}

		if (cinFiles[handle].bits & CIN_loop)
		{
			bVideo.SetLooping(true);
		}
		else
		{
			bVideo.SetLooping(false);
		}

		if (cinFiles[handle].bits & CIN_silent)
		{
			bVideo.SetMasterVolume(0);
		}
		else
		{
			bVideo.SetMasterVolume(16384);	//32768);	// Default Bink volume
		}

		if (!shader)
		{
			previousState = cls.state;
			cls.state = CA_CINEMATIC;
		}

		currentHandle = handle;
	}

	// Normal case does nothing here
	if(bVideo.GetStatus() == NS_BV_STOPPED)
	{
		return FMV_EOF;
	}
	else
	{
		return FMV_PLAY;
	}
}

/********
CIN_PlayCinematic

arg0	- filename of bink video
xpos	- x origin
ypos	- y origin
width	- width of the movie window
height	- height of the movie window
bits	- CIN flags
psAudioFile	- audio file for movie (not used)

Starts playing the given bink video file
*********/
int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */)
{
	char	arg[MAX_OSPATH];
	char*	nameonly;
	int		handle;

	// get a local copy of the name
	strcpy(arg,arg0);

	// remove path, find in list
	nameonly = COM_SkipPath(arg);

	// ja01 contains the text crawl, so we need to add on the right language suffix
	extern DWORD g_dwLanguage;
	if( Q_stricmp(nameonly, "ja01") == 0)
	{
		switch( g_dwLanguage )
		{
			case XC_LANGUAGE_FRENCH:
				strcat(nameonly, "_f");
				break;
			case XC_LANGUAGE_GERMAN:
				strcat(nameonly, "_d");
				break;
			case XC_LANGUAGE_ENGLISH:
			default:
				strcat(nameonly, "_e");
				break;
		}
	}

	for (handle = 0; handle < cinNumFiles; ++handle)
	{
		if (!Q_stricmp(cinFiles[handle].filename, nameonly))
			break;
	}

	// Don't have the requested movie in our table?
	if (handle == cinNumFiles)
	{
		Com_Printf( "ERROR: Movie file %s not found!\n", nameonly );
		return -1;
	}

	// Store off information about the movie in the right place. Don't
	// actually play them movie, CIN_RunCinematic takes care of that.
	cinFiles[handle].x = xpos;
	cinFiles[handle].y = ypos;
	cinFiles[handle].w = width;
	cinFiles[handle].h = height;
	cinFiles[handle].bits = bits;
	currentHandle = -1;
	return handle;
}

/*********
CIN_SetExtents

handle	- handle to a video
x		- x origin for window
y		- y origin for window
w		- width for window
h		- height for window
*********/
void CIN_SetExtents (int handle, int x, int y, int w, int h)
{
	if (handle < 0 || handle >= cinNumFiles)
		return;

	cinFiles[handle].x = x;
	cinFiles[handle].y = y;
	cinFiles[handle].w = w;
	cinFiles[handle].h = h;

	if (handle == currentHandle)
		bVideo.SetExtents(x,y,w,h);
}


/*********
SCR_DrawCinematic

Externally-called only, and only if cls.state == CA_CINEMATIC (or CL_IsRunningInGameCinematic() == true now)
*********/
void SCR_DrawCinematic (void)
{
	if (CL_InGameCinematicOnStandBy())
	{
		CIN_PlayAllFrames( sInGameCinematicStandingBy, 0, 0, 640, 480, 0, true );
	}
	else
	{
		// Run and draw a frame:
		bVideo.Run();
	}
}

/*********
SCR_RunCinematic
*********/
void SCR_RunCinematic (void)
{
	// This is called every frame, even when we're not playing a movie
	// VVFIXME - Check return val for EOF - then stop cinematic?
	if (currentHandle > 0 && currentHandle < cinNumFiles)
		CIN_RunCinematic(currentHandle);
}

/*********
SCR_StopCinematic
*********/
void SCR_StopCinematic(qboolean bAllowRefusal /* = qfalse */)
{
	CIN_StopCinematic(currentHandle);
}

/*********
CIN_UploadCinematic

handle		- (not used)

This function can be used to render a frame of a movie, if
it needs to be done outside of CA_CINEMATIC. For example,
a menu background or wall texture.
*********/
void CIN_UploadCinematic(int handle)
{
	int w, h;
	byte* data;

	assert( handle == currentHandle );

	if(!bVideo.Ready()) {
		return;
	}
	
	w		= bVideo.GetBinkWidth();
	h		= bVideo.GetBinkHeight();
	data	= (byte*)bVideo.GetBinkData();

	// handle is actually being used to pick from scratchImages in
	// this function - we only have two on Xbox, let's just use one.
	//re.UploadCinematic( w, h, data, handle, 1);
	re.UploadCinematic( w, h, data, 0, 1);
}

/*********
CIN_PlayAllFrames

arg				- bink video filename
x				- x origin for movie
y				- y origin for movie
w				- width of the movie
h				- height of the movie
systemBits		- bit rate for movie
keyBreakAllowed	- if true, button press will end playback

Plays the target movie in full
*********/
bool CIN_PlayAllFrames( const char *arg, int x, int y, int w, int h, int systemBits, bool keyBreakAllowed )
{
	bool retval;
	Key_ClearStates();

	// PC hack
	qbInGameCinematicOnStandBy = qfalse;

#ifdef XBOX_DEMO
	// When run from CDX, we can pause the timer during cutscenes:
	extern void Demo_TimerPause( bool bPaused );
	Demo_TimerPause( true );
#endif

	int Handle = CIN_PlayCinematic(arg, x, y, w, h, systemBits, NULL);
	if (Handle != -1)
	{
		while (CIN_RunCinematic(Handle) == FMV_PLAY && !(keyBreakAllowed && kg.anykeydown))
		{
			SCR_UpdateScreen	();
			IN_Frame			();
			Com_EventLoop		();
		}
#ifdef _XBOX
//		while (CIN_RunCinematic(Handle) == FMV_PLAY && !(keyBreakAllowed && !kg.anykeydown))
//		{
//			SCR_UpdateScreen	();
//			IN_Frame			();
//			Com_EventLoop		();
//		}
#endif
		CIN_StopCinematic(Handle);
	}

#ifdef XBOX_DEMO
	Demo_TimerPause( false );
#endif

	retval =(keyBreakAllowed && kg.anykeydown);
	Key_ClearStates();

	// Soooper hack! Game ends up running for a couple frames after this cutscene. We don't want it to!
	if( Q_stricmp(arg, "ja08") == 0 )
	{
		// Filth. Don't call Present until this gets cleared.
		extern bool connectSwapOverride;
		connectSwapOverride = true;
	}

	return retval;
}

/*********
CIN_Init
Initializes cinematic system
*********/
void CIN_Init(void)
{
	// Allocate Memory for Bink System
	bVideo.AllocateXboxMem();
}

/********
CIN_Shutdown
Shutdown the cinematic system
********/
void CIN_Shutdown(void)
{
	// Free Memory for the Bink System
	bVideo.FreeXboxMem();
}


/***** Possible FIXME *****/
/***** The following function may need to be implemented *****/
/***** BEGIN *****/
void CL_PlayCinematic_f(void)
{
	char	*arg;
	
	arg = Cmd_Argv(1);
	CIN_PlayAllFrames(arg, 48, 36, 544, 408, 0, true);
}

qboolean CL_IsRunningInGameCinematic(void)
{
	return qfalse; //qbPlayingInGameCinematic;
}

void CL_PlayInGameCinematic_f(void)
{
	if (cls.state == CA_ACTIVE)
	{
		// In some situations (during yavin1 intro) we move to a cutscene directly from
		// a shaking camera - so rumble never gets killed.
		IN_KillRumbleScripts();

		char *arg = Cmd_Argv( 1 );
		CIN_PlayAllFrames(arg, 48, 36, 544, 408, 0, true);
	}
	else
	{
		qbInGameCinematicOnStandBy = qtrue;
		strcpy(sInGameCinematicStandingBy,Cmd_Argv(1));
	}
}

qboolean CL_InGameCinematicOnStandBy(void)
{
	return qbInGameCinematicOnStandBy;
}

// Used by fatal error handler
void MuteBinkSystem( void )
{
	bVideo.SetMasterVolume( 0 );
}
/***** END *****/

