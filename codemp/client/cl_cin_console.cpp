
/*****************************************************************************
 * name:		cl_cin_stubs.cpp
 *
 * desc:		video and cinematic playback stubs to avoid link errors
 *
 *
 * cl_glconfig.hwtype trtypes 3dfx/ragepro need 256x256
 *
 *****************************************************************************/

#include "client.h"
#include "../win32/win_local.h"
#include "../win32/win_input.h"
#include "BinkVideo.h"

#define XBOX_VIDEO_PATH "d:\\base\\video\\"

BinkVideo bVideo;	// bink video object
connstate_t	previousState = CA_UNINITIALIZED;	// previous cinematic state

/********
CIN_CloseAllVideos
Stops all currently running videos
*********/
void CIN_CloseAllVideos(void)
{
	// Stop the current bink video
	bVideo.Stop();
}

/********
CIN_StopCinematic

handle	- Not used
return	- FMV status

Stops the current cinematic
*********/
e_status CIN_StopCinematic(int handle)
{
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
CIN_StopCinematic

handle	- Not used
return	- FMV status

Checks the status of the current cinematic
*********/
e_status CIN_RunCinematic (int handle)
{
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
	char	name[MAX_OSPATH];
	char	arg[MAX_OSPATH];
	char*	nameonly;

	// get a local copy of the name
	strcpy(arg,arg0);

	// remove path
	nameonly = COM_SkipPath(arg);

	// form the proper name with path
	Com_sprintf(name,sizeof(name),XBOX_VIDEO_PATH);
	strcat(name,nameonly);
	COM_DefaultExtension(name, sizeof(name),".bik");

	if(bVideo.Start(name,xpos,ypos,width,height))
	{
		if(bits & CIN_loop)
		{
			bVideo.SetLooping(true);
		}
		if(bits & CIN_silent)
		{
			bVideo.SetMasterVolume(0);
		}
		if(!(bits & CIN_shader))
		{
			previousState = cls.state;
			cls.state = CA_CINEMATIC;
		}
		return 1;
	}
	else
	{
		return -1;
	}
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
	bVideo.SetExtents(x,y,w,h);
}

/********
CIN_DrawCinematic

handle	- handle to a video (not used)

Updates the current frame of the current video
*********/
void CIN_DrawCinematic (int handle)
{
	bVideo.Run();
}

/*********
SCR_DrawCinematic
*********/
void SCR_DrawCinematic (void)
{
	CIN_DrawCinematic(1);
}
/*********
SCR_RunCinematic
*********/
void SCR_RunCinematic (void)
{
	CIN_RunCinematic(1);
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
	CIN_StopCinematic(1);
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

	if(!bVideo.Ready()) {
		return;
	}

	w		= bVideo.GetBinkWidth();
	h		= bVideo.GetBinkHeight();
	data	= (byte*)bVideo.GetBinkData();

	re.UploadCinematic( w, h, data, handle, 1);
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
#ifdef _XBOX
extern void Key_ClearStates(void);
#endif
bool CIN_PlayAllFrames( const char *arg, int x, int y, int w, int h, int systemBits, bool keyBreakAllowed )
{
//JLF new
	bool retval;
//endJLF
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
CIN_DisplayIntros
Draws intro movies to the screen
*********/
void CIN_DisplayIntros(void)
{

	////////////////////////////////////
	// Play 1st video: Activision
	////////////////////////////////////
	CIN_PlayAllFrames( "atvi.bik", 0, 0, 640, 480, 0, true );

	
	////////////////////////////////////
	// Play 2nd video: Vicarious Visions
	////////////////////////////////////
	CIN_PlayAllFrames( "vvintro.bik", 0, 0, 640, 480, 0, true );
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
	// Nothing
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

