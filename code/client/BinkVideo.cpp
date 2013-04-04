/*
 * This version of BinkVideo.cpp now ONLY works on Xbox.
 * GCN support is hosed.
 */
#include "snd_local_console.h"
#include "../renderer/tr_local.h"
#include "BinkVideo.h"
#include "RAD.h"
#include "../win32/xbox_texture_man.h"
#include <xgraphics.h>

#include "../client/client.h"

char *binkSndMem = NULL;

// Taste the hackery!
extern void *BonePoolTempAlloc( unsigned long size );
extern void BonePoolTempFree( void *p );
extern void *TempAlloc( unsigned long size );
extern void TempFree( void );

extern void SP_DrawSPLoadScreen( void );

// Allocation wrappers, that go to our static 2.5MB buffer:
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
	x1			= 0.0f;
	y1			= 0.0f;
	x2			= 0.0f;
	y2			= 0.0f;
	status		= NS_BV_STOPPED;
	looping		= false;
	alpha		= false;
	initialized = false;
	loadScreenOnStop = false;

	Image[0].surface = NULL;
	Image[0].texture = NULL;
	Image[1].surface = NULL;
	Image[1].texture = NULL;

	stopNextFrame = false;
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
//	binkSndMem = (char*)Allocate(XBOX_BINK_SND_MEM);
	// Force the sound memory to come from the Zone:
	binkSndMem = (char *) Z_Malloc(XBOX_BINK_SND_MEM, TAG_BINK, qfalse, 32);
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

	// Hack! Remember if this was the logo movies, so that we can show the load screen later:
	if( strstr( filename, "logos" ) )
		loadScreenOnStop = true;
	else
		loadScreenOnStop = false;

	// Blow away all sounds that aren't playing - this helps prevent crashing:
	SND_FreeOldestSound();

	// Just use the zone for bink allocations:
	RADSetMemory( AllocWrapper, FreeWrapper );

	// Set up sound for consoles

	// We are on XBox, tell Bink to play all of the 5.1 tracks
//	U32 TrackIDsToPlay[ 4 ] = { 0, 1, 2, 3 };	
//	BinkSetSoundTrack( 4, TrackIDsToPlay );
	
	// Now route the sound tracks to the correct speaker
//	U32 bins[ 2 ];

//	bins[ 0 ] = DSMIXBIN_FRONT_LEFT;
//	bins[ 1 ] = DSMIXBIN_FRONT_RIGHT;
//	BinkSetMixBins( bink, 0, bins, 2 );
//	bins[ 0 ] = DSMIXBIN_FRONT_CENTER;
//	BinkSetMixBins( bink, 1, bins, 1 );
//	bins[ 0 ] = DSMIXBIN_LOW_FREQUENCY;
//	BinkSetMixBins( bink, 2, bins, 1 );
//	bins[ 0 ] = DSMIXBIN_BACK_LEFT;
//	bins[ 1 ] = DSMIXBIN_BACK_RIGHT;
//	BinkSetMixBins( bink, 3, bins, 2 );

	// Try to open the Bink file.
//	bink = BinkOpen( filename, BINKSNDTRACK | BINKALPHA );
	bink = BinkOpen( filename, BINKALPHA );
	if(!bink)
	{
		return false;
	}

	assert(bink->Width <= MAX_WIDTH && bink->Height <=MAX_HEIGHT);

	// Did the source .bik file have an alpha plane?
	alpha = (bool)(bink->OpenFlags & BINKALPHA);

	// set the height, width, etc...
	x1 = xOrigin;
	y1 = yOrigin;
	x2 = x1 + width;
	y2 = y1 + height;

	// flush any background sound reads
	extern void S_DrainRawSoundData(void);
	S_DrainRawSoundData();

	// Full-screen movies (without alpha) need a pair of YUV2 textures:
	if( !alpha )
	{
		// YUY2 is 16 bpp? But we need two:
		gTextures.SwapTextureMemory( (bink->Width * bink->Height * 4) + 1024 );

		// Make our two textures:
		Image[0].texture = new IDirect3DTexture9;
		Image[1].texture = new IDirect3DTexture9;

		// Fill in the texture headers:
		DWORD pixelSize = 
		XGSetTextureHeader( bink->Width,
							bink->Height,
							1,
							0,
							D3DFMT_YUY2,
							0,
							Image[0].texture,
							0,
							0 );

		XGSetTextureHeader( bink->Width,
							bink->Height,
							1,
							0,
							D3DFMT_YUY2,
							0,
							Image[1].texture,
							0,
							0 );

		// texNum is unused:
		Image[0].texture->Register( gTextures.Allocate( pixelSize, 0 ) );
		Image[1].texture->Register( gTextures.Allocate( pixelSize, 0 ) );

		// Turn on overlays:
		glw_state->device->EnableOverlay( TRUE );

		// Get surface pointers:
		Image[0].texture->GetSurfaceLevel( 0, &Image[0].surface );
		Image[1].texture->GetSurfaceLevel( 0, &Image[1].surface );

		// Just to be safe:
		currentImage = 0;
		buffer = NULL;
	}
	else
	{
		// Planet movies (with alpha) re-use tr.binkPlanetImage, so no texture setup
		// is needed. But we do need a temporary buffer to decompress into. Let's steal
		// from the bone pool.
		buffer = BonePoolTempAlloc( bink->Width * bink->Height * 4 );
	}

	status = NS_BV_PLAYING;

	return true;
}

/*********
Run
Decompresses a frame, renders it to the screen, and advances to
the next frame. Only used for full-screen movies (no alpha).
*********/
bool BinkVideo::Run(void)
{
	// Make sure movie is running:
	if( status == NS_BV_STOPPED )
		return false;

	// Wait for proper frame timing:
	while(BinkWait(bink));

	// Are we supposed to stop now?
	if( stopNextFrame )
	{
		stopNextFrame = false;
		Stop();
		return false;
	}

	// Try to decompress the frame:
	if( DecompressFrame( &Image[currentImage ^ 1] ) == 0 )
	{
		// The blt succeeded, update our current image index.
		currentImage ^= 1;

		// Draw the next frame.
		Draw( &Image[currentImage] );
	}

	// Are we done? Set a flag, we don't want to stop until next frame, so the
	// last frame stays up for the right amount of time!
	if( bink->FrameNum == bink->Frames && !looping )
	{
		stopNextFrame = true;
	}

	// Keep playing:
	BinkNextFrame( bink );

	// Are we done?
/*
	if( bink->FrameNum == (bink->Frames - 1) && !looping )
	{
		Stop();
		return false;
	}
*/

	return true;
}

/*********
GetBinkData
Returns the buffer data for the next frame of the video - only used for
movies with alpha (the planets).

This doesn't follow Bink guidelines. They suggest that you call BinkWait()
very frequently, something like 4 to 5 times as fast as the framerate of
the movie. We're technically coming close to that, but this code won't work
if we have videoMap shaders with higher framerates than the planets (8).
*********/
void* BinkVideo::GetBinkData(void)
{
	assert( alpha );

	if (!BinkWait(bink))
	{
		BinkDoFrame(bink);

		BinkCopyToBuffer( bink,
						  buffer,
						  bink->Width * 4,	// Pitch
						  bink->Height,
						  0,
						  0,
						  BINKCOPYALL | BINKSURFACE32A );

		BinkNextFrame(bink);
	}

	return buffer;
}

/********
Draw
Draws the current movie full-screen
********/
void BinkVideo::Draw( OVERLAYINFO * oi )
{
	// Draw the image on the screen (centered)...
	RECT dst_rect = { 0, 0, 640, 480 };
	RECT src_rect = { 0, 0, bink->Width, bink->Height };

	// Update this bugger.
	glw_state->device->UpdateOverlay( oi->surface, &src_rect, &dst_rect, FALSE, 0 );
}

/*********
Stop
Stops the current movie, and clears it from memory
*********/
void BinkVideo::Stop(void)
{
	if( bink ) {
		BinkClose( bink );
	}

	if( alpha )
	{
		// Release all the temp space we grabbed, no texture cleanup to do:
		if( buffer )
			BonePoolTempFree( buffer );
	}
	else
	{
		// We wrap all this in a single check - it should be all or nothing:
		if( Image[0].surface )
		{
			// Release surfaces:
			Image[0].surface->Release();
			Image[0].surface = NULL;

			Image[1].surface->Release();
			Image[1].surface = NULL;

			// Clean up the textures we made for the overlay stuff:
			Image[0].texture->BlockUntilNotBusy();
			delete Image[0].texture;
			Image[0].texture = NULL;

			Image[1].texture->BlockUntilNotBusy();
			delete Image[1].texture;
			Image[1].texture = NULL;

			// We're going to be stripping the overlay off, leave a black screen,
			// unless it was the logo movies that we just played, in which case we
			// draw the loading screen!
			if( loadScreenOnStop )
			{
				SP_DrawSPLoadScreen();
				glw_state->device->BlockUntilVerticalBlank();

				// Filth. Don't call Present until this gets cleared.
				extern bool connectSwapOverride;
				connectSwapOverride = true;
			}
			else
			{
				glw_state->device->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_COLORVALUE(0, 0, 0, 0), 0, 0 );
				glw_state->device->Present( NULL, NULL, NULL, NULL );
				glw_state->device->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_COLORVALUE(0, 0, 0, 0), 0, 0 );
				glw_state->device->Present( NULL, NULL, NULL, NULL );
				glw_state->device->BlockUntilVerticalBlank();
			}

			// Turn overlays back off:
			glw_state->device->EnableOverlay( FALSE );

			// Restore the textures that we dumped to disk
			gTextures.UnswapTextureMemory();
		}
	}

	x1		= 0.0f;
	y1		= 0.0f;
	x2		= 0.0f;
	y2		= 0.0f;
	buffer	= NULL;
	bink	= NULL;
	status	= NS_BV_STOPPED;

	// Now free all the temp memory that Bink took with it's internal allocations:
	TempFree();

	if( !alpha && (cls.state == CA_CINEMATIC || cls.state == CA_ACTIVE) )
        re.InitDissolve(qfalse);
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
S32 BinkVideo::DecompressFrame( OVERLAYINFO *oi )
{
	s32 copy_skipped;
	D3DLOCKED_RECT lock_rect;

	// Decompress the Bink frame.
	BinkDoFrame( bink );

	// Lock the 3D image so that we can copy the decompressed frame into it.
	oi->texture->LockRect( 0, &lock_rect, 0, 0 );

	// Copy the decompressed frame into the 3D image.
	copy_skipped = BinkCopyToBuffer( bink,
									 lock_rect.pBits,
									 lock_rect.Pitch,
									 bink->Height,
									 0, 0,
									 BINKSURFACEYUY2 | BINKCOPYALL );

	// Unlock the 3D image.
	oi->texture->UnlockRect( 0 );

	return copy_skipped;
}

/*********
Allocate
Allocates memory for the frame buffer
*********/
void *BinkVideo::Allocate(U32 size)
{
	void *retVal = TempAlloc( size );

	// Fall back to Zone if we didn't get it
	if( !retVal )
		retVal = Z_Malloc(size, TAG_BINK, qfalse, 32);

	return retVal;
}

/*********
FreeBuffer
Releases the frame buffer memory
*********/
void BinkVideo::Free(void* ptr)
{
	// Did this pointer come from the Zone up above?
	if( !Z_IsFromTempPool( ptr ) )
		Z_Free(ptr);

	// Else, do nothing - we don't free temp allocations until movie is done
}
