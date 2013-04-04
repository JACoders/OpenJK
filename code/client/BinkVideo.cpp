/*
 * This version of BinkVideo.cpp now ONLY works on Xbox.
 * GCN support is hosed.
 */
#include "snd_local_console.h"
#include "../renderer/tr_local.h"
#include "BinkVideo.h"
#include "RAD.h"

char*	binkSndMem = NULL;

static void PTR4* RADEXPLINK AllocWrapper(U32 size)
{
	// Give bink pre-initialized sound mem on xbox
	if(size == XBOX_BINK_SND_MEM) {
		return binkSndMem;
	}

	return BinkVideo::Allocate(size);
}

static void RADEXPLINK FreeWrapper(void PTR4* ptr)
{
	BinkVideo::Free(ptr);
}


/*********
BinkVideo
*********/
BinkVideo::BinkVideo()
{
	bink		= NULL;
	buffer		= NULL;
	texture		= 0;
	x1			= 0.0f;
	y1			= 0.0f;
	x2			= 0.0f;
	y2			= 0.0f;
	w			= 0.0f;
	h			= 0.0f;
	status		= NS_BV_STOPPED;
	looping		= false;
	alpha		= false;
#ifdef _XBOX
	initialized = false;
#endif
}

/*********
~BinkVideo
*********/
BinkVideo::~BinkVideo()
{
	Free(buffer);
	BinkClose(bink);
}

/*********
AllocateXboxMem
Pre-Allocates sound memory for xbox to avoid fragmenting
*********/
void BinkVideo::AllocateXboxMem(void)
{
	binkSndMem = (char*)Allocate(XBOX_BINK_SND_MEM);
	initialized = true;
}

/*********
FreeXboxMem
*********/
void BinkVideo::FreeXboxMem(void)
{
	initialized = false;
	Z_Free(binkSndMem);
}


/*********
Start
Opens a bink file and gets it ready to play
*********/
bool BinkVideo::Start(const char *filename, float xOrigin, float yOrigin, float width, float height)
{
	assert(initialized);

	// Check to see if a video is being played.
	if(status == NS_BV_PLAYING)
	{
		// stop
		this->Stop();
	}

	// Set memory allocation wrapper
	RADSetMemory(AllocWrapper,FreeWrapper);

	// Set up sound for consoles

	// We are on XBox, tell Bink to play all of the 5.1 tracks
	U32 TrackIDsToPlay[ 4 ] = { 0, 1, 2, 3 };	
	BinkSetSoundTrack( 4, TrackIDsToPlay );
	
	// Now route the sound tracks to the correct speaker
	U32 bins[ 2 ];

	bins[ 0 ] = DSMIXBIN_FRONT_LEFT;
	bins[ 1 ] = DSMIXBIN_FRONT_RIGHT;
	BinkSetMixBins( bink, 0, bins, 2 );
	bins[ 0 ] = DSMIXBIN_FRONT_CENTER;
	BinkSetMixBins( bink, 1, bins, 1 );
	bins[ 0 ] = DSMIXBIN_LOW_FREQUENCY;
	BinkSetMixBins( bink, 2, bins, 1 );
	bins[ 0 ] = DSMIXBIN_BACK_LEFT;
	bins[ 1 ] = DSMIXBIN_BACK_RIGHT;
	BinkSetMixBins( bink, 3, bins, 2 );
	
	
	// Try to open the Bink file.
	bink = BinkOpen( filename, BINKSNDTRACK | BINKALPHA );
	if(!bink)
	{
		return false;
	}

	assert(bink->Width <= MAX_WIDTH && bink->Height <=MAX_HEIGHT);

	// allocate memory for the frame buffer
	buffer = AllocWrapper(XBOX_BUFFER_SIZE);

	// set the height, width, etc...
	x1 = xOrigin;
	y1 = yOrigin;
	x2 = x1 + width;
	y2 = y1 + height;
	w = width;
	h = height;
	// Did the source .bik file have an alpha plane?
	alpha = (bool)(bink->OpenFlags & BINKALPHA);

	// flush any background sound reads
	extern void S_DrainRawSoundData(void);
	S_DrainRawSoundData();
	
	// Create the video texture
	GLuint tex = (GLuint)texture;
	if (tex != 0)
		qglDeleteTextures(1, &tex);

	qglGenTextures(1, &tex);
	qglBindTexture(GL_TEXTURE_2D, tex);
	glState.currenttextures[glState.currenttmu] = tex;

	qglTexImage2D(GL_TEXTURE_2D,
				  0,
				  alpha ? GL_LIN_RGBA8 : GL_LIN_RGB8,
				  bink->Width,
				  bink->Height,
				  0,
				  alpha ? GL_LIN_RGBA : GL_LIN_RGB,
				  GL_UNSIGNED_BYTE,
				  buffer);

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	texture = (int)tex;

	status = NS_BV_PLAYING;

	return true;
}

/*********
Run
Decompresses a frame, renders it to the screen, and advances to
the next frame.
*********/
bool BinkVideo::Run(void)
{
	if(status == NS_BV_STOPPED) // A movie can't be run if it's not started first
	{
		return false;
	}
	
	while(BinkWait(bink)); // Wait
	
	DecompressFrame(); // Decompress
	Draw(); // Render
	
	if(status != NS_BV_PAUSED) // Only advance the frame is not paused
	{
		BinkNextFrame( bink );
	}

	if(bink->FrameNum == (bink->Frames - 1) && !looping) // The movie is done
	{
		Stop();
		return false;
	}
	
	return true;
}

/*********
GetBinkData
Returns the buffer data for the next frame of the video
*********/
void* BinkVideo::GetBinkData(void)
{
	//while(BinkWait(bink));
	// This doesn't follow Bink guidelines. They suggest that you call BinkWait()
	// very frequently, something like 4 to 5 times as fast as the framerate of
	// the movie. We're technically coming close to that, but this code won't work
	// if we have videoMap shaders with higher framerates than the planets (8).
	if (!BinkWait(bink))
	{
		DecompressFrame();
		BinkNextFrame(bink);
	}
	return buffer;
}

/********
Draw
Copies the decompressed frame to a texture to be rendered on
the screen.
********/
void BinkVideo::Draw(void)
{
	if(buffer)
	{
		qglFlush();
		
		extern void	RB_SetGL2D (void);
		RB_SetGL2D();

		GL_SelectTexture(0);

		// Update the video texture
		qglBindTexture(GL_TEXTURE_2D, (GLuint)texture);
		glState.currenttextures[glState.currenttmu] = texture;
		
		qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, bink->Width, bink->Height,
			alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, buffer );
			
		// Clear the screen.  We use triangles here (instead
		// of glClear) because we want the back buffer to stick
		// around... so we can get a nice, cheap fade on Gamecube
		// reset.
		qglColor3f(0.f, 0.f, 0.f);
#if defined (_XBOX) || (_GAMECUBE)
		qglBeginEXT (GL_TRIANGLE_STRIP, 4, 0, 0, 4, 0);
#else
		qglBegin(GL_TRIANGLE_STRIP);
#endif
		qglTexCoord2f ( 0,  0 );
		qglVertex2f (-10, -10);
		qglTexCoord2f ( bink->Width ,  0 );
		qglVertex2f (650, -10);
		qglTexCoord2f ( 0, bink->Height );
		qglVertex2f (-10, 490);
		qglTexCoord2f ( bink->Width, bink->Height );
		qglVertex2f (650, 490);
		qglEnd ();
		
		// Draw the video
		qglColor3f(1.f, 1.f, 1.f);
#if defined (_XBOX) || (_GAMECUBE)
		qglBeginEXT (GL_TRIANGLE_STRIP, 4, 0, 0, 4, 0);
#else
		qglBegin(GL_TRIANGLE_STRIP);
#endif
		qglTexCoord2f ( 0,  0 );
		qglVertex2f (x1, y1);
		qglTexCoord2f ( bink->Width ,  0 );
		qglVertex2f (x2, y1);
		qglTexCoord2f ( 0, bink->Height );
		qglVertex2f (x1, y2);
		qglTexCoord2f ( bink->Width, bink->Height );
		qglVertex2f (x2, y2);
		qglEnd ();
	}
}

/*********
Stop
Stops the current movie, and clears it from memory
*********/
void BinkVideo::Stop(void)
{
	if (bink)
		BinkClose(bink);
	bink	= NULL;

	if (buffer)
		FreeWrapper(buffer);
	buffer	= NULL;

	GLuint tex = (GLuint)texture;
	if (tex != 0)
		qglDeleteTextures(1, &tex);

	texture	= 0;
	x1		= 0.0f;
	y1		= 0.0f;
	x2		= 0.0f;
	y2		= 0.0f;
	w		= 0.0f;
	h		= 0.0f;
	status	= NS_BV_STOPPED;
}

/*********
Pause
Pauses the current movie. Only the current frame is rendered
*********/
void BinkVideo::Pause(void)
{
	status = NS_BV_PAUSED;
}

/*********
SetExtents
Sets dimmension variables
*********/

void BinkVideo::SetExtents(float xOrigin, float yOrigin, float width, float height)
{
	x1 = xOrigin;
	y1 = yOrigin;
	x2 = x1 + width;
	y2 = y1 + height;
	w = width;
	h = height;
}

/*********
SetMasterVolume
Sets the volume of the specified track
*********/
void BinkVideo::SetMasterVolume(s32 volume)
{
	int i;
	for(i = 0; i < 4; i++)
	{
		BinkSetVolume(bink,i,volume);
	}
}

/*********
DecompressFrame
Decompresses current frame and copies the data to
the buffer
*********/
S32 BinkVideo::DecompressFrame()
{
	BinkDoFrame(bink);

	S32 skip;	
	skip = BinkCopyToBuffer(
		bink,
		(void *)buffer,
		NS_BV_DEFAULT_CIN_BPS * bink->Width, //pitch
		bink->Height,
		0,
		0,
		alpha ? BINKCOPYALL | BINKSURFACE32A : BINKCOPYALL | BINKSURFACE32);
	return skip;
}

/*********
Allocate
Allocates memory for the frame buffer
*********/
void *BinkVideo::Allocate(U32 size)
{
	return Z_Malloc(size, TAG_BINK, qfalse, 32);
}

/*********
FreeBuffer
Releases the frame buffer memory
*********/
void BinkVideo::Free(void* ptr)
{
	Z_Free(ptr);
}
