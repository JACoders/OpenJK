#pragma once

// snd_local.h -- private sound definations

#define sboolean int //rww - argh (in SP qboolean type is merely #define'd as an int, but I do not want to do that for MP over the whole base)

#include "snd_public.h"
#include "mp3code/mp3struct.h"

// Open AL Specific
#ifdef _WIN32
#include "OpenAL/al.h"
#include "OpenAL/alc.h"
#include "eax\eax.h"
#include "eax\eaxman.h"
#elif defined MACOS_X
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
// Added for Open AL to know when to mute all sounds (e.g when app. loses focus)
void S_AL_MuteAllSounds(sboolean bMute);


//from SND_AMBIENT
extern void AS_Init( void );
extern void AS_Free( void );


#define	PAINTBUFFER_SIZE	1024


// !!! if this is changed, the asm code must change !!!
typedef struct portable_samplepair_s {
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
} portable_samplepair_t;


// keep this enum in sync with the table "sSoundCompressionMethodStrings"	-ste
//
typedef enum
{
	ct_16 = 0,		// formerly ct_NONE in EF1, now indicates 16-bit samples (the default)
	ct_MP3,
	//
	ct_NUMBEROF		// used only for array sizing

} SoundCompressionMethod_t;


typedef struct sfx_s {
	short			*pSoundData;
	sboolean		bDefaultSound;			// couldn't be loaded, so use buzz
	sboolean		bInMemory;				// not in Memory, set qtrue when loaded, and qfalse when its buffers are freed up because of being old, so can be reloaded
	SoundCompressionMethod_t eSoundCompressionMethod;
	MP3STREAM		*pMP3StreamHeader;		// NULL ptr unless this sfx_t is an MP3. Use Z_Malloc and Z_Free
	int 			iSoundLengthInSamples;	// length in samples, always kept as 16bit now so this is #shorts (watch for stereo later for music?)
	char 			sSoundName[MAX_QPATH];
	int				iLastTimeUsed;
	float			fVolRange;				// used to set the highest volume this sample has at load time - used for lipsynching
	int				iLastLevelUsedOn;		// used for cacheing purposes

	// Open AL
	ALuint		Buffer;
	char		*lipSyncData;

	struct sfx_s	*next;					// only used because of hash table when registering
} sfx_t;

typedef struct dma_s {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;


#define START_SAMPLE_IMMEDIATE	0x7fffffff

// Open AL specific
typedef struct STREAMINGBUFFER_s {
	ALuint	BufferID;
	ALuint	Status;
	char	*Data;
} STREAMINGBUFFER;

#define NUM_STREAMING_BUFFERS	4
#define STREAMING_BUFFER_SIZE	4608		// 4 decoded MP3 frames

#define QUEUED		1
#define UNQUEUED	2


typedef struct channel_s {
// back-indented fields new in TA codebase, will re-format when MP3 code finished -ste
// note: field missing in TA: sboolean	loopSound;		// from an S_AddLoopSound call, cleared each frame
//
	unsigned int startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	int			leftvol;		// 0-255 volume after spatialization
	int			rightvol;		// 0-255 volume after spatialization
	int			master_vol;		// 0-255 volume before spatialization


	vec3_t		origin;			// only use if fixed_origin is set

	sboolean	fixed_origin;	// use origin instead of fetching entnum's origin
	sfx_t		*thesfx;		// sfx structure
	sboolean	loopSound;		// from an S_AddLoopSound call, cleared each frame
	//
	MP3STREAM	MP3StreamHeader;
	byte		MP3SlidingDecodeBuffer[50000/*12000*/];	// typical back-request = -3072, so roughly double is 6000 (safety), then doubled again so the 6K pos is in the middle of the buffer)
	int			iMP3SlidingDecodeWritePos;
	int			iMP3SlidingDecodeWindowPos;

	qboolean	doppler;
	float		dopplerScale;

	// Open AL specific
	bool	bLooping;	// Signifies if this channel / source is playing a looping sound
//	bool	bAmbient;	// Signifies if this channel / source is playing a looping ambient sound
	bool	bProcessed;	// Signifies if this channel / source has been processed
	bool	bStreaming;	// Set to true if the data needs to be streamed (MP3 or dialogue)
	STREAMINGBUFFER	buffers[NUM_STREAMING_BUFFERS];	// AL Buffers for streaming
	ALuint		alSource;		// Open AL Source
	bool		bPlaying;		// Set to true when a sound is playing on this channel / source
	int			iStartTime;		// Time playback of Source begins
	int			lSlotID;		// ID of Slot rendering Source's environment (enables a send to this FXSlot)
} channel_t;


#define	WAV_FORMAT_PCM		1
#define WAV_FORMAT_ADPCM	2	// not actually implemented, but is the value that you get in a header
#define WAV_FORMAT_MP3		3	// not actually used this way, but just ensures we don't match one of the legit formats


typedef struct wavinfo_s {
	int			format;
	int			rate;
	int			width;
	int			channels;
	int			samples;
	int			dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;



/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);

// gets the current DMA position
int		SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void	SNDDMA_Shutdown(void);

void	SNDDMA_BeginPainting (void);

void	SNDDMA_Submit(void);

//====================================================================

#define	MAX_CHANNELS			32
extern	channel_t   s_channels[MAX_CHANNELS];

extern	int		s_paintedtime;
extern	int		s_rawend;
extern	vec3_t	listener_origin;
extern	vec3_t	listener_forward;
extern	vec3_t	listener_right;
extern	vec3_t	listener_up;
extern	dma_t	dma;

#define	MAX_RAW_SAMPLES	16384
extern	portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];
portable_samplepair_t *S_GetRawSamplePointer();	// TA added this, but it just returns the s_rawsamples[] array above. Oh well...

extern cvar_t	*s_volume;
extern cvar_t	*s_volumeVoice;
extern cvar_t	*s_nosound;
extern cvar_t	*s_khz;
extern cvar_t	*s_allowDynamicMusic;
extern cvar_t	*s_show;
extern cvar_t	*s_mixahead;

extern cvar_t	*s_testsound;
extern cvar_t	*s_separation;

extern cvar_t	*s_doppler;

wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength);

sboolean S_LoadSound( sfx_t *sfx );


void S_PaintChannels(int endtime);

// picks a channel based on priorities, empty slots, number of channels
channel_t *S_PickChannel(int entnum, int entchannel);

// spatializes a channel
void S_Spatialize(channel_t *ch);


//////////////////////////////////
//
// new stuff from TA codebase

byte	*SND_malloc(int iSize, sfx_t *sfx);
void	 SND_setup();
int		 SND_FreeOldestSound(sfx_t *pButNotThisOne = NULL);
void	 SND_TouchSFX(sfx_t *sfx);

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* 99% qfalse */);

void S_DisplayFreeMemory(void);
void S_memoryLoad(sfx_t *sfx);
//
//////////////////////////////////

#include "snd_mp3.h"
