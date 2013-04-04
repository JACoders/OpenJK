// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#include <float.h>

#include "../client/snd_local.h"
#include "win_local.h"

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);
#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

#define SECONDARY_BUFFER_SIZE	0x10000


extern int s_UseOpenAL;

static qboolean	dsound_init;
static int		sample16;
static DWORD	gSndBufSize;
static DWORD	locksize;
static LPDIRECTSOUND pDS;
static LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;
static HINSTANCE hInstDS;

static int  SNDDMA_InitDS ();

static const char *DSoundError( int error ) {
	switch ( error ) {
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	case DSERR_ALLOCATED:
		return "DSERR_ALLOCATED";
	case DSERR_UNINITIALIZED:
		return "DSERR_UNINITIALIZED";
	case DSERR_UNSUPPORTED:
		return "DSERR_UNSUPPORTED ";
	}

	return "unknown";
}

/*
==================
SNDDMA_Shutdown
==================
*/
void SNDDMA_Shutdown( void ) {
	Com_DPrintf( "Shutting down sound system\n" );

	if ( pDS ) {
		Com_DPrintf( "Destroying DS buffers\n" );
		if ( pDS )
		{
			Com_DPrintf( "...setting NORMAL coop level\n" );
			pDS->SetCooperativeLevel( g_wv.hWnd, DSSCL_NORMAL );
		}

		if ( pDSBuf )
		{
			Com_DPrintf( "...stopping and releasing sound buffer\n" );
			pDSBuf->Stop( );
			pDSBuf->Release( );
		}

		// only release primary buffer if it's not also the mixing buffer we just released
		if ( pDSPBuf && ( pDSBuf != pDSPBuf ) )
		{
			Com_DPrintf( "...releasing primary buffer\n" );
			pDSPBuf->Release( );
		}
		pDSBuf = NULL;
		pDSPBuf = NULL;

		dma.buffer = NULL;

		Com_DPrintf( "...releasing DS object\n" );
		pDS->Release( );
	}

	if ( hInstDS ) {
		Com_DPrintf( "...freeing DSOUND.DLL\n" );
		FreeLibrary( hInstDS );
		hInstDS = NULL;
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	dsound_init = qfalse;
	memset ((void *)&dma, 0, sizeof (dma));
}

/*
==================
SNDDMA_Init

Initialize direct sound
Returns false if failed
==================
*/
qboolean SNDDMA_Init(void) {

	memset ((void *)&dma, 0, sizeof (dma));
	dsound_init = qfalse;

	if ( !SNDDMA_InitDS () ) {
		return qfalse;
	}

	dsound_init = qtrue;

	Com_DPrintf("Completed successfully\n" );

    return qtrue;
}


static int SNDDMA_InitDS ()
{
	HRESULT			hresult;
	qboolean		pauseTried;
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	format;

	Com_Printf( "Initializing DirectSound\n");

	if ( !hInstDS ) {
		Com_DPrintf( "...loading dsound.dll: " );

		hInstDS = LoadLibrary("dsound.dll");
		
		if ( hInstDS == NULL ) {
			Com_Printf ("failed\n");
			return 0;
		}

		Com_DPrintf ("ok\n");
		pDirectSoundCreate = (long (__stdcall *)(struct _GUID *,struct IDirectSound ** ,struct IUnknown *))
			GetProcAddress(hInstDS,"DirectSoundCreate");

		if ( !pDirectSoundCreate ) {
			Com_Printf ("*** couldn't get DS proc addr ***\n");
			return 0;
		}
	}

	Com_DPrintf( "...creating DS object: " );
	pauseTried = qfalse;
	while ( ( hresult = iDirectSoundCreate( NULL, &pDS, NULL ) ) != DS_OK ) {
		if ( hresult != DSERR_ALLOCATED ) {
			Com_Printf( "failed\n" );
			return 0;
		}

		if ( pauseTried ) {
			Com_Printf ("failed, hardware already in use\n" );
			return 0;
		}
		// first try just waiting five seconds and trying again
		// this will handle the case of a sysyem beep playing when the
		// game starts
		Com_DPrintf ("retrying...\n");
		Sleep( 3000 );
		pauseTried = qtrue;
	}
	Com_DPrintf( "ok\n" );

	Com_DPrintf("...setting DSSCL_PRIORITY coop level: " );

	if ( DS_OK != pDS->SetCooperativeLevel( g_wv.hWnd, DSSCL_PRIORITY ) )	{
		Com_Printf ("failed\n");
		SNDDMA_Shutdown ();
		return qfalse;
	}
	Com_DPrintf("ok\n" );


	// create the secondary buffer we'll actually work with
	dma.channels = 2;
	dma.samplebits = 16;

	if (s_khz->integer == 44)
		dma.speed = 44100;
	else if (s_khz->integer == 22)
		dma.speed = 22050;
	else
		dma.speed = 11025;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = dma.channels;
    format.wBitsPerSample = dma.samplebits;
    format.nSamplesPerSec = dma.speed;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign; 

	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);

#define idDSBCAPS_GETCURRENTPOSITION2 0x00010000

	dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCHARDWARE | idDSBCAPS_GETCURRENTPOSITION2;
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;
	
	Com_DPrintf( "...creating secondary buffer: " );
	hresult = pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL);
	if (hresult != DS_OK) 
	{
		if ( hresult == DSERR_CONTROLUNAVAIL )
		{
			Com_Printf( " - Ancient version of DirectX - this will slow FPS\n" );
			dsbuf.dwFlags &= ~idDSBCAPS_GETCURRENTPOSITION2;	// lose this DX8 cursor-position feature, and try again
			hresult = pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL);
		}

		if (hresult != DS_OK)
		{
			// we can't even specify sounds should be in hardware?...  
			//
			//  ( this seems to happen on integrated sound devices (eg SoundMax), regardless of DX version )
			//
			dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY;	// note that DX docs say that this can still use hardware if it wants to, since neither DSBCAPS_LOCHARDWARE nor DSBCAPS_LOCSOFTWARE were specified
			hresult = pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL);
			if (hresult != DS_OK) {			
				Com_Printf( "failed to create secondary buffer - %s\n", DSoundError( hresult ) );
				SNDDMA_Shutdown ();
				return qfalse;
			}
		}
	}
	Com_Printf( "locked hardware.  ok\n" );
		
	// Make sure mixer is active
	if ( DS_OK != pDSBuf->Play(0, 0, DSBPLAY_LOOPING) ) {
		Com_Printf ("*** Looped sound play failed ***\n");
		SNDDMA_Shutdown ();
		return qfalse;
	}

	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	// get the returned buffer size
	if ( DS_OK != pDSBuf->GetCaps (&dsbcaps) ) {
		Com_Printf ("*** GetCaps failed ***\n");
		SNDDMA_Shutdown ();
		return qfalse;
	}
	
	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.submission_chunk = 1;
	dma.buffer = NULL;			// must be locked first

	sample16 = (dma.samplebits/8) - 1;

	SNDDMA_BeginPainting ();
	if (dma.buffer)
		memset(dma.buffer, 0, dma.samples * dma.samplebits/8);
	SNDDMA_Submit ();
	return 1;
}
/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos( void ) {
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if ( !dsound_init ) {
		return 0;
	}

	mmtime.wType = TIME_SAMPLES;
	pDSBuf->GetCurrentPosition(&mmtime.u.sample, &dwWrite);

	s = mmtime.u.sample;

	s >>= sample16;

	s &= (dma.samples-1);

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting( void ) {
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if ( !pDSBuf ) {
		return;
	}

	// if the buffer was lost or stopped, restore it and/or restart it
	if ( pDSBuf->GetStatus (&dwStatus) != DS_OK ) {
		Com_Printf ("Couldn't get sound buffer status\n");
	}
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
		pDSBuf->Restore ();
	
	if (!(dwStatus & DSBSTATUS_PLAYING))
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ((hresult = pDSBuf->Lock(0, gSndBufSize, (void **)&pbuf, &locksize, 
								   (void **)&pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Com_Printf( "SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError( hresult ) );
			S_Shutdown ();
			return;
		}
		else
		{
			pDSBuf->Restore( );
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (unsigned char *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit( void ) {
    // unlock the dsound buffer
	if ( pDSBuf ) {
		pDSBuf->Unlock(dma.buffer, locksize, NULL, 0);
	}
}


/*
=================
SNDDMA_Activate

When we change windows we need to do this
=================
*/
void SNDDMA_Activate( qboolean bAppActive )
{
	if (s_UseOpenAL)
	{
		S_AL_MuteAllSounds(!bAppActive);
	}

	if ( !pDS ) {
		return;
	}

	if ( DS_OK != pDS->SetCooperativeLevel( g_wv.hWnd, DSSCL_PRIORITY ) )	{
		Com_Printf ("sound SetCooperativeLevel failed\n");
		SNDDMA_Shutdown ();
	}
}



// I know this is a bit horrible, but I need to pass our LPDIRECTSOUND ptr to Bink for video playback,
//	and I don't want other modules to have to know about LPDIRECTSOUND handles, hence the int casting
//
// (I'd prefer to use DWORD, but not all modules understand those)
//
unsigned int SNDDMA_GetDSHandle(void)
{
	return (unsigned int) pDS;
}


