#pragma once

// snd_local.h -- private sound definations

// Following #define is ONLY for MP JKA code.
// They want to keep qboolean pure enum in that code, so all
// sound code uses sboolean.
#define sboolean int

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"
#include "../mp3code/mp3struct.h"

#include "openal/al.h"
#include "openal/alc.h"

typedef int streamHandle_t;

//from SND_AMBIENT
extern void AS_Init( void );
extern void AS_Free( void );

typedef struct {
	int			format;
	int			size;
	int			width;
	int			rate;
} wavinfo_t;

extern wavinfo_t GetWavInfo(byte *data);


#define SFX_FLAG_UNLOADED	(1 << 0)
#define SFX_FLAG_LOADING	(1 << 1)
#define SFX_FLAG_RESIDENT	(1 << 2)
#define SFX_FLAG_DEFAULT	(1 << 3)
#define SFX_FLAG_DEMAND		(1 << 4)
#define SFX_FLAG_VOICE		(1 << 5)

typedef struct sfx_s {
	int				iFlags;
	int 			iSoundLength;			// length in bytes
	int				iLastTimeUsed;			// last time sound was played in ms
	int				iFileCode;				// CRC of the file name
	streamHandle_t	iStreamHandle;			// handle to the sound file when reading
	void*			pSoundData;				// buffer to hold sound as we are loading it
	ALuint			Buffer;
} sfx_t;

typedef struct
{
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	int			master_vol;		// 0-255 volume before spatialization
	float		fLastVolume;	// 0-1 last volume sent to AL

	vec3_t		origin;			// sound location
	bool		bOriginDirty;	// does the AL position need to be updated
	
	sfx_t		*thesfx;		// sfx structure

	int			loopChannel;	// index into loopSounds (if appropriate)

	bool		bPlaying;		// Set to true when a sound is playing on this channel / source
	bool		b2D;			// Signifies a 2d sound
	bool		bLooping;		// Signifies if this channel / source is playing a looping sound
	ALuint		alSource;		// Open AL Source

	unsigned int iLastPlayTime;	// Last time a sound was played on this channel
} channel_t;

extern cvar_t	*s_nosound;
extern cvar_t	*s_allowDynamicMusic;
extern cvar_t	*s_show;

extern cvar_t	*s_testsound;
extern cvar_t	*s_separation;

extern int*	s_entityWavVol;

int Sys_GetFileCode( const char* sSoundName );
int S_GetFileCode( const char* sSoundName );

qboolean S_StartLoadSound( sfx_t *sfx );
qboolean S_EndLoadSound( sfx_t *sfx );

void S_InitLoad(void);
void S_CloseLoad(void);
void S_UpdateLoading(void);

// New stuff from VV
void S_LoadSound( sfxHandle_t sfxHandle );

// picks a channel based on priorities, empty slots, number of channels
channel_t *S_PickChannel(int entnum, int entchannel);

//////////////////////////////////
//
// new stuff from TA codebase

void	 SND_update(sfx_t *sfx);
void	 SND_setup();
int		 SND_FreeOldestSound(sfx_t *pButNotThisOne = NULL);
void	 SND_TouchSFX(sfx_t *sfx);

void S_DisplayFreeMemory(void);
void S_memoryLoad(sfx_t *sfx);

bool Sys_StreamIsReading(streamHandle_t handle);
int  Sys_StreamOpen(int code, streamHandle_t *handle);
bool Sys_StreamRead(void* buffer, int size, int pos, streamHandle_t handle);
void Sys_StreamClose(streamHandle_t handle);
bool Sys_StreamIsError(streamHandle_t handle);

//
//////////////////////////////////
