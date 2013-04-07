
// mac_snddma.c
// all other sound mixing is portable

#include "../client/snd_local.h"
#include <sound.h>

#define	MAX_MIXED_SAMPLES	0x8000
#define	SUBMISSION_CHUNK	0x100

static	short			s_mixedSamples[MAX_MIXED_SAMPLES];
static	int				s_chunkCount;		// number of chunks submitted
static	SndChannel		*s_sndChan;
static	ExtSoundHeader	s_sndHeader;

/*
===============
S_Callback
===============
*/
void S_Callback( SndChannel *sc, SndCommand *cmd ) {
	SndCommand		mySndCmd;
	SndCommand		mySndCmd2;
	int				offset;
	
	offset = ( s_chunkCount * SUBMISSION_CHUNK ) & (MAX_MIXED_SAMPLES-1);
	
	// queue up another sound buffer
	memset( &s_sndHeader, 0, sizeof( s_sndHeader ) );
	s_sndHeader.samplePtr = (void *)(s_mixedSamples + offset);
	s_sndHeader.numChannels = 2;
	s_sndHeader.sampleRate = rate22khz;
	s_sndHeader.loopStart = 0;
	s_sndHeader.loopEnd = 0;
	s_sndHeader.encode = extSH;
	s_sndHeader.baseFrequency = 1;
	s_sndHeader.numFrames = SUBMISSION_CHUNK / 2;
	s_sndHeader.markerChunk = NULL;
	s_sndHeader.instrumentChunks = NULL;
	s_sndHeader.AESRecording = NULL;
	s_sndHeader.sampleSize = 16;
	
	mySndCmd.cmd = bufferCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (int)&s_sndHeader;
	SndDoCommand( sc, &mySndCmd, true );
	
	// and another callback
	mySndCmd2.cmd = callBackCmd;
	mySndCmd2.param1 = 0;
	mySndCmd2.param2 = 0;
	SndDoCommand( sc, &mySndCmd2, true );

	s_chunkCount++;		// this is the next buffer we will submit
}

/*
===============
S_MakeTestPattern
===============
*/
void S_MakeTestPattern( void ) {
	int		i;
	float	v;
	int		sample;
	
	for ( i = 0 ; i < dma.samples / 2 ; i ++ ) {
		v = sin( M_PI * 2 * i / 64 );
		sample = v * 0x4000;
		((short *)dma.buffer)[i*2] = sample;	
		((short *)dma.buffer)[i*2+1] = sample;	
	}
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void) {
	int		err;
	
	// create a sound channel
	s_sndChan = NULL;
	err = SndNewChannel( &s_sndChan, sampledSynth, initStereo, NewSndCallBackProc(S_Callback) );
	if ( err ) {
		return false;
	}
	
	dma.channels = 2;
	dma.samples = MAX_MIXED_SAMPLES;
	dma.submission_chunk = SUBMISSION_CHUNK;
	dma.samplebits = 16;
	dma.speed = 22050;
	dma.buffer = (byte *)s_mixedSamples;
	
	// que up the first submission-chunk sized buffer
	s_chunkCount = 0;
	
	S_Callback( s_sndChan, NULL );
	
	return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int	SNDDMA_GetDMAPos(void) {
	return s_chunkCount * SUBMISSION_CHUNK;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void) {
	if ( s_sndChan ) {
		SndDisposeChannel( s_sndChan, true );
		s_sndChan = NULL;
	}
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting(void) {
}

/*
===============
SNDDMA_Submit
===============
*/
void SNDDMA_Submit(void) {
}
