#include "snd_local_console.h"
#include "../renderer/tr_local.h"
#include "BinkVideo.h"
#include "RAD.h"


bool	bvUseGCTexMem = true;

#ifdef _XBOX
int		memMarker     = 0;
char*	binkXboxStartAddr = NULL;
char*	binkXboxCurrentAddr = NULL;
char*	binkXboxNextAddr = NULL;
#endif

static void PTR4* RADEXPLINK AllocWrapper(U32 size)
{

// Give bink pre-initialized mem on xbox
#ifdef _XBOX
	switch(memMarker)
	{
	case 0:
		memMarker++;
		binkXboxCurrentAddr = binkXboxStartAddr;
		binkXboxNextAddr = binkXboxCurrentAddr + size;
		return (void *)binkXboxStartAddr;	
	case 1: case 2: case 3: case 4: case 5: case 6: case 7:
		memMarker++;
		binkXboxCurrentAddr = binkXboxNextAddr;
		binkXboxNextAddr = binkXboxCurrentAddr + size;
		return (void *)binkXboxCurrentAddr;
	case 8:
		memMarker = -1;
		binkXboxCurrentAddr = binkXboxNextAddr;
		binkXboxNextAddr = binkXboxStartAddr;
		return (void *)binkXboxCurrentAddr;
	default:
		return BinkVideo::Allocate(size);
	}
#endif

	return BinkVideo::Allocate(size);
}

static void RADEXPLINK FreeWrapper(void PTR4* ptr)
{

// Don't free the preinitialized mem
#ifdef _XBOX
	if(memMarker < 6)
	{
		memMarker++;
		return;
	}
	else if(memMarker == 6)
	{
		memMarker = 1;
		binkXboxNextAddr = binkXboxStartAddr + XBOX_MEM_STAGE_1;
		return;
	}
#endif
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
Pre-Allocates memory for xbox
*********/
#ifdef _XBOX
void BinkVideo::AllocateXboxMem(void)
{
	u32 memToAllocate = XBOX_MEM_STAGE_1 +
						XBOX_MEM_STAGE_2 +
						XBOX_MEM_STAGE_3 +
						XBOX_MEM_STAGE_4 +
						XBOX_MEM_STAGE_5 +
						XBOX_MEM_STAGE_6 +
						XBOX_MEM_STAGE_7 +
						XBOX_MEM_STAGE_8 +
						XBOX_BUFFER_SIZE;
	binkXboxStartAddr = (char*)Allocate(memToAllocate);
	memMarker = 0;
	initialized = true;
}

/*********
FreeXboxMem
*********/
void BinkVideo::FreeXboxMem(void)
{
	initialized = false;
	Z_Free(binkXboxStartAddr);
	memMarker = 0;
}
#endif


/*********
Start
Opens a bink file and gets it ready to play
*********/
bool BinkVideo::Start(const char *filename, float xOrigin, float yOrigin, float width, float height)
{

#ifdef _XBOX
	assert(initialized);
#endif

	// Check to see if a video is being played.
	if(status == NS_BV_PLAYING)
	{
		// stop
		this->Stop();
	}

	// Set memory allocation wrapper
	RADSetMemory(AllocWrapper,FreeWrapper);

	// Set up sound for consoles
#if defined(_XBOX)
	// If we are on XBox, tell Bink to play all of the 5.1 tracks
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

#elif defined(_GAMECUBE)	
	BinkSoundUseNGCSound();
	
	RADMEMALLOC a;
	alGeti(AL_MEMORY_ALLOCATOR, (ALint*)&a);
	
	RADMEMFREE f;
	alGeti(AL_MEMORY_DEALLOCATOR, (ALint*)&f);
	
	RADSetAudioMemory(a, f); 
#endif
	
	
	// Try to open the Bink file.
#ifdef _XBOX
	bink = BinkOpen( filename, BINKSNDTRACK );
	if(!bink)
	{
		return false;
	}
#elif defined _GAMECUBE
	if(bvUseGCTexMem)
	{
		extern void GLW_TexCacheLock(void);
		GLW_TexCacheLock();
	}

	bink = BinkOpen( filename, 0);

	if(!bink)
	{
		extern void GLW_TexCacheUnlock(void);
		GLW_TexCacheUnlock();
		return false;
	}
#endif


	assert(bink->Width <= MAX_WIDTH && bink->Height <=MAX_HEIGHT);

	// allocate memory for the frame buffer
#ifdef _XBOX
	buffer = AllocWrapper(XBOX_BUFFER_SIZE);
#elif _GAMECUBE
	buffer = Allocate(XBOX_BUFFER_SIZE);
#endif

	// set the height, width, etc...
	x1 = xOrigin;
	y1 = yOrigin;
	x2 = x1 + width;
	y2 = y1 + height;
	w = width;
	h = height;

	// flush any background sound reads
#if defined (_XBOX)|| (_GAMECUBE)
	extern void S_DrainRawSoundData(void);
	S_DrainRawSoundData();
#endif
	
	// Create the video texture
	GLuint tex = (GLuint)texture;
	if (tex != 0)
		qglDeleteTextures(1, &tex);

	qglGenTextures(1, &tex);
	qglBindTexture(GL_TEXTURE_2D, tex);
	glState.currenttextures[glState.currenttmu] = tex;
	
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB5, bink->Width, bink->Height, 0, 
		GL_RGB_SWIZZLE_EXT, GL_UNSIGNED_BYTE, buffer );
	
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
	while(BinkWait(bink));
	DecompressFrame();
	BinkNextFrame(bink);
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
			GL_RGB_SWIZZLE_EXT, GL_UNSIGNED_BYTE, buffer );
			
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
		qglTexCoord2f ( 1 ,  0 );
		qglVertex2f (650, -10);
		qglTexCoord2f ( 0, 1 );
		qglVertex2f (-10, 490);
		qglTexCoord2f ( 1, 1 );
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
		qglTexCoord2f ( 1 ,  0 );
		qglVertex2f (x2, y1);
		qglTexCoord2f ( 0, 1 );
		qglVertex2f (x1, y2);
		qglTexCoord2f ( 1, 1 );
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
	BinkClose(bink);
	bink	= NULL;
#ifdef _XBOX
	FreeWrapper(buffer);
#elif _GAMECUBE
	Free(buffer);
#endif
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
#ifdef _XBOX
	memMarker	= 0;
#endif
	
#ifdef _GAMECUBE
	extern void GLW_TexCacheUnlock(void);
	GLW_TexCacheUnlock();
#endif
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
SetExtends
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
#ifdef _XBOX
	int i;
	for(i = 0; i < 4; i++)
	{
		BinkSetVolume(bink,i,volume);
	}
#else
	BinkSetVolume(bink,0,volume);
#endif
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
		BINKCOPYALL | BINKSURFACE565);
	return skip;
}

/*********
Allocate
Allocates memory for the frame buffer
*********/
void *BinkVideo::Allocate(U32 size)
{

	size = RoundUp(size + 32, 32);
	char* ptr = NULL;

#ifdef _GAMECUBE
	if (bvUseGCTexMem)
	{
		// Try allocating from texture cache
		extern void* GLW_TexCacheAllocRaw(int size);
		ptr = (char*)GLW_TexCacheAllocRaw(size);
	}
#endif

	if (!ptr)
	{
		// Did not allocate texture cache memory, fall
		// back to main memory..
		ptr = (char*)Z_Malloc(size, TAG_BINK, qfalse, 32);
		ptr[0] = 'z';
	}
	else
	{
		// Allocated memory from the texture cache
		ptr[0] = 't';
	}

	return (void*)(ptr + 32);
}

/*********
FreeBuffer
Releases the frame buffer memory
*********/
void BinkVideo::Free(void* ptr)
{
	char* base = (char*)ptr - 32;
	
	switch (*base)
	{
#ifdef _GAMECUBE
	case 't':
		{
			// Free texture cache memory
			extern void GLW_TexCacheFreeRaw(void* ptr);
			GLW_TexCacheFreeRaw(base);
			break;
		}
#endif
		
	case 'z':
		// Free main memory
		Z_Free(base);
		break;
		
	default:
		assert(false);
	}
}
