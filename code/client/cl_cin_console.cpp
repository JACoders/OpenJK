
/*****************************************************************************
 * name:		cl_cin_console.cpp
 *
 * desc:		video and cinematic playback interface for Xbox (using Bink)
 *
 *****************************************************************************/

#include "client.h"
#include "../win32/win_local.h"
#include "../win32/win_input.h"
#include "BinkVideo.h"

#define XBOX_VIDEO_PATH "d:\\base\\video\\"

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
//	{ "jk1", 0, 0, 0, 0, 0 },
//	{ "jk2", 0, 0, 0, 0, 0 },
//	{ "jk3", 0, 0, 0, 0, 0 },
//	{ "jk4", 0, 0, 0, 0, 0 },
//	{ "jk5", 0, 0, 0, 0, 0 },
};

const int cinNumFiles = sizeof(cinFiles) / sizeof(cinFiles[0]);
static int currentHandle = -1;

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
		CIN_StopCinematic(currentHandle);
		if (!bVideo.Start(
					va(XBOX_VIDEO_PATH "%s.bik", cinFiles[handle].filename),
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
			bVideo.SetMasterVolume(32768);	// Default Bink volume
		}

		if (!(cinFiles[handle].bits & CIN_shader))
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
#ifdef _JK2MP
int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits)
#else
int CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits, const char *psAudioFile /* = NULL */)
#endif
{
//	char	name[MAX_OSPATH];
	char	arg[MAX_OSPATH];
	char*	nameonly;
	int		handle;

	// get a local copy of the name
	strcpy(arg,arg0);

	// remove path, find in list
	nameonly = COM_SkipPath(arg);
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

handle	- handle to a video (not used)
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

/********
CIN_DrawCinematic

handle	- handle to a video (not used)

Updates the current frame of the current video
*********/
void CIN_DrawCinematic (int handle)
{
	assert( handle == currentHandle );

	bVideo.Run();
}

/*********
SCR_DrawCinematic
*********/
void SCR_DrawCinematic (void)
{
	CIN_DrawCinematic(currentHandle);
}
/*********
SCR_RunCinematic
*********/
void SCR_RunCinematic (void)
{
	// This is called every frame, even when we're not playing a movie
	if (currentHandle > 0 && currentHandle < cinNumFiles)
		CIN_RunCinematic(currentHandle);
}

/*********
SCR_StopCinematic
*********/
#ifdef _JK2MP
void SCR_StopCinematic(void)
#else
void SCR_StopCinematic(qboolean bAllowRefusal /* = qfalse */)
#endif
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

#ifdef _JK2MP
	int Handle = CIN_PlayCinematic(arg, x, y, w, h, systemBits);
#else
	int Handle = CIN_PlayCinematic(arg, x, y, w, h, systemBits, NULL);
#endif
	if (Handle != -1)
	{
		while (CIN_RunCinematic(Handle) == FMV_PLAY && !(keyBreakAllowed && kg.anykeydown))
		{
			SCR_UpdateScreen	();
			IN_Frame			();
			Com_EventLoop		();
		}
#ifdef _XBOX
		while (CIN_RunCinematic(Handle) == FMV_PLAY && !(keyBreakAllowed && !kg.anykeydown))
		{
			SCR_UpdateScreen	();
			IN_Frame			();
			Com_EventLoop		();
		}
#endif
		CIN_StopCinematic(Handle);
	}

	retval =(keyBreakAllowed && kg.anykeydown);
	Key_ClearStates();
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
	// Nothing
	return qfalse;
}

qboolean CL_CheckPendingCinematic(void)
{
	// Nothing
	return qfalse;
}

void CL_PlayInGameCinematic_f(void)
{
	// Nothing
}

qboolean CL_InGameCinematicOnStandBy(void)
{
	// Nothing
	return qfalse;
}
/***** END *****/

