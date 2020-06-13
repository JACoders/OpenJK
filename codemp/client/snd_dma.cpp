/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 *
 *****************************************************************************/
#include "qcommon/qcommon.h"
#include "sdl/sdl_sound.h"
#include "snd_local.h"
#include "snd_mp3.h"
#include "snd_music.h"
#include "client.h"
#include "sys/sys_public.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static void S_Play_f(void);
static void S_SoundList_f(void);
static void S_Music_f(void);
static void S_StopMusic_f(void);
static void S_SetDynamicMusic_f(void);

void S_Update_();
void S_StopAllSounds(void);
sfx_t *S_FindName( const char *name );
static int SND_FreeSFXMem(sfx_t *sfx);

extern qboolean Sys_LowPhysicalMemory();

//////////////////////////
//
// vars for bgrnd music track...
//
const int iMP3MusicStream_DiskBytesToRead = 10000;//4096;
const int iMP3MusicStream_DiskBufferSize = iMP3MusicStream_DiskBytesToRead*2; //*10;

typedef struct MusicInfo_s {
	qboolean	bIsMP3;
	//
	// MP3 specific...
	//
	sfx_t		sfxMP3_Bgrnd;
	MP3STREAM	streamMP3_Bgrnd;	// this one is pointed at by the sfx_t's ptr, and is NOT the one the decoder uses every cycle
	channel_t	chMP3_Bgrnd;		// ... but the one in this struct IS.
	//
	// MP3 disk streamer stuff... (if music is non-dynamic)
	//
	byte		byMP3MusicStream_DiskBuffer[iMP3MusicStream_DiskBufferSize];
	int			iMP3MusicStream_DiskReadPos;
	int			iMP3MusicStream_DiskWindowPos;
	//
	// MP3 disk-load stuff (for use during dynamic music, which is mem-resident)
	//
	byte		*pLoadedData;	// Z_Malloc, Z_Free	// these two MUST be kept as valid/invalid together
	char		sLoadedDataName[MAX_QPATH];			//  " " " " "
	int			iLoadedDataLen;
	//
	// remaining dynamic fields...
	//
	int			iXFadeVolumeSeekTime;
	int			iXFadeVolumeSeekTo;	// when changing this, set the above timer to Sys_Milliseconds().
									//	Note that this should be thought of more as an up/down bool rather than as a
									//	number now, in other words set it only to 0 or 255. I'll probably change this
									//	to actually be a bool later.
	int			iXFadeVolume;		// 0 = silent, 255 = max mixer vol, though still modulated via overall music_volume
	float		fSmoothedOutVolume;
	qboolean	bActive;			// whether playing or not
	qboolean	bExists;			// whether was even loaded for this level (ie don't try and start playing it)
	//
	// new dynamic fields...
	//
	qboolean	bTrackSwitchPending;
	MusicState_e eTS_NewState;
	float		 fTS_NewTime;
	//
	// Generic...
	//
	fileHandle_t s_backgroundFile;	// valid handle, else -1 if an MP3 (so that NZ compares still work)
	wavinfo_t	s_backgroundInfo;
	int			s_backgroundSamples;

	void Rewind()
	{
		MP3Stream_Rewind( &chMP3_Bgrnd );
		s_backgroundSamples = sfxMP3_Bgrnd.iSoundLengthInSamples;
	}

	void SeekTo(float fTime)
	{
		chMP3_Bgrnd.iMP3SlidingDecodeWindowPos = 0;
		chMP3_Bgrnd.iMP3SlidingDecodeWritePos = 0;
		MP3Stream_SeekTo( &chMP3_Bgrnd, fTime );
		s_backgroundSamples = sfxMP3_Bgrnd.iSoundLengthInSamples;
	}
} MusicInfo_t;

static void S_SetDynamicMusicState( MusicState_e musicState );

#define fDYNAMIC_XFADE_SECONDS (1.0f)

static MusicInfo_t	tMusic_Info[eBGRNDTRACK_NUMBEROF]	= {};
static qboolean		bMusic_IsDynamic					= qfalse;
static MusicState_e	eMusic_StateActual					= eBGRNDTRACK_EXPLORE;	// actual state, can be any enum
static MusicState_e	eMusic_StateRequest					= eBGRNDTRACK_EXPLORE;	// requested state, can only be explore, action, boss, or silence
static char			sMusic_BackgroundLoop[MAX_QPATH]	= {0};	// only valid for non-dynamic music
static char			sInfoOnly_CurrentDynamicMusicSet[64];	// any old reasonable size, only has to fit stuff like "kejim_post"
//
//////////////////////////


// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	256

#define		SOUND_ATTENUATE		0.0008f
#define		VOICE_ATTENUATE		0.004f

const float	SOUND_FMAXVOL=0.75;//1.0;
const int	SOUND_MAXVOL=255;

channel_t   s_channels[MAX_CHANNELS];

qboolean	s_shutUp = qfalse;
int			s_soundStarted;
qboolean	s_soundMuted;

dma_t		dma;

int			listener_number;
vec3_t		listener_origin;
matrix3_t	listener_axis;

int			s_soundtime;		// sample PAIRS
int   		s_paintedtime; 		// sample PAIRS

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define		MAX_SFX			10000	//512 * 2
sfx_t		s_knownSfx[MAX_SFX];
int			s_numSfx;

#define		LOOP_HASH		128
static	sfx_t		*sfxHash[LOOP_HASH];

cvar_t		*s_volume;
cvar_t		*s_volumeVoice;
cvar_t		*s_testsound;
cvar_t		*s_khz;
cvar_t		*s_allowDynamicMusic;
cvar_t		*s_show;
cvar_t		*s_mixahead;
cvar_t		*s_mixPreStep;
cvar_t		*s_musicVolume;
cvar_t		*s_separation;
cvar_t		*s_lip_threshold_1;
cvar_t		*s_lip_threshold_2;
cvar_t		*s_lip_threshold_3;
cvar_t		*s_lip_threshold_4;
cvar_t		*s_language;	// note that this is distinct from "g_language"
cvar_t		*s_dynamix;
cvar_t		*s_debugdynamic;

cvar_t		*s_doppler;

int			numLoopSounds;
loopSound_t	loopSounds[MAX_LOOP_SOUNDS];

int			s_rawend;
portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];
vec3_t		s_entityPosition[MAX_GENTITIES];
int			s_entityWavVol[MAX_GENTITIES];
int			s_entityWavVol_back[MAX_GENTITIES];


// instead of clearing a whole channel_t struct, we're going to skip the MP3SlidingDecodeBuffer[] buffer in the middle...
//
#ifndef offsetof
#include <stddef.h>
#endif
static inline void Channel_Clear(channel_t *ch)
{
	// memset (ch, 0, sizeof(*ch));

	memset(ch,0,offsetof(channel_t,MP3SlidingDecodeBuffer));

	byte *const p = (byte *)ch + offsetof(channel_t,MP3SlidingDecodeBuffer) + sizeof(ch->MP3SlidingDecodeBuffer);

	memset(p,0,(sizeof(*ch) - offsetof(channel_t,MP3SlidingDecodeBuffer)) - sizeof(ch->MP3SlidingDecodeBuffer));
}

// ====================================================================
// User-setable variables
// ====================================================================
static void DynamicMusicInfoPrint(void)
{
	if (bMusic_IsDynamic)
	{
		// horribly lazy... ;-)
		//
		const char *psRequestMusicState	= Music_BaseStateToString( eMusic_StateRequest );
		const char *psActualMusicState	= Music_BaseStateToString( eMusic_StateActual, qtrue );
		if (psRequestMusicState == NULL)
		{
			psRequestMusicState = "<unknown>";
		}
		if (psActualMusicState	== NULL)
		{
			psActualMusicState	= "<unknown>";
		}

		Com_Printf("( Dynamic music ON, request state: '%s'(%d), actual: '%s' (%d) )\n", psRequestMusicState, eMusic_StateRequest, psActualMusicState, eMusic_StateActual);
	}
	else
	{
		Com_Printf("( Dynamic music OFF )\n");
	}
}

void S_SoundInfo_f(void) {
	Com_Printf("----- Sound Info -----\n" );

	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		if (S_AL_IsEnabled())
		{
            S_AL_SoundInfo_f();
		}
		else
		{
			Com_Printf("%5d stereo\n", dma.channels - 1);
			Com_Printf("%5d samples\n", dma.samples);
			Com_Printf("%5d samplebits\n", dma.samplebits);
			Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
			Com_Printf("%5d speed\n", dma.speed);
			Com_Printf( "0x%" PRIxPTR " dma buffer\n", dma.buffer );
		}

		if (bMusic_IsDynamic)
		{
			DynamicMusicInfoPrint();
			Com_Printf("( Dynamic music set name: \"%s\" )\n",sInfoOnly_CurrentDynamicMusicSet);
		}
		else
		{
			if (!s_allowDynamicMusic->integer)
			{
				Com_Printf("( Dynamic music inhibited (s_allowDynamicMusic == 0) )\n", sMusic_BackgroundLoop );
			}
			if ( tMusic_Info[eBGRNDTRACK_NONDYNAMIC].s_backgroundFile )
			{
				Com_Printf("Background file: %s\n", sMusic_BackgroundLoop );
			}
			else
			{
				Com_Printf("No background file.\n" );
			}
		}
	}
	S_DisplayFreeMemory();
	Com_Printf("----------------------\n" );
}

/*
================
S_Init
================
*/
void S_Init( void ) {
	cvar_t	*cv;
	qboolean	r;

	Com_Printf("\n------- sound initialization -------\n");

	s_volume = Cvar_Get ("s_volume", "0.5", CVAR_ARCHIVE, "Master Volume" );
	s_volumeVoice= Cvar_Get ("s_volumeVoice", "1.0", CVAR_ARCHIVE, "Volume for voice channels" );
	s_musicVolume = Cvar_Get ("s_musicvolume", "0.25", CVAR_ARCHIVE, "Music Volume" );

    Cvar_CheckRange(s_volume, 0.0f, 1.0f, qfalse);
    Cvar_CheckRange(s_volumeVoice, 0.0f, 1.0f, qfalse);
    Cvar_CheckRange(s_musicVolume, 0.0f, 1.0f, qfalse);

	s_separation = Cvar_Get ("s_separation", "0.5", CVAR_ARCHIVE);
	s_khz = Cvar_Get ("s_khz", "44", CVAR_ARCHIVE|CVAR_LATCH);
	s_allowDynamicMusic = Cvar_Get ("s_allowDynamicMusic", "1", CVAR_ARCHIVE_ND);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);

	s_mixPreStep = Cvar_Get ("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);
	s_debugdynamic = Cvar_Get("s_debugdynamic","0", CVAR_CHEAT);
	s_lip_threshold_1 = Cvar_Get("s_threshold1" , "0.5",0);
	s_lip_threshold_2 = Cvar_Get("s_threshold2" , "4.0",0);
	s_lip_threshold_3 = Cvar_Get("s_threshold3" , "7.0",0);
	s_lip_threshold_4 = Cvar_Get("s_threshold4" , "8.0",0);

	s_language = Cvar_Get("s_language","english",CVAR_ARCHIVE | CVAR_NORESTART, "Sound language" );

	s_doppler = Cvar_Get("s_doppler", "1", CVAR_ARCHIVE_ND);

	MP3_InitCvars();

	cv = Cvar_Get ("s_initsound", "1", 0);
	if ( !cv->integer ) {
		s_soundStarted = 0;	// needed in case you set s_initsound to 0 midgame then snd_restart (div0 err otherwise later)
		Com_Printf ("not initializing.\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f, "Plays a sound fx file" );
	Cmd_AddCommand("music", S_Music_f, "Plays a music file" );
	Cmd_AddCommand("stopmusic", S_StopMusic_f, "Stops all music" );
	Cmd_AddCommand("soundlist", S_SoundList_f, "Lists all cached sound and music files" );
	Cmd_AddCommand("soundinfo", S_SoundInfo_f, "Display information about the sound backend" );
	Cmd_AddCommand("soundstop", S_StopAllSounds, "Stops all sounds including music" );
	Cmd_AddCommand("mp3_calcvols", S_MP3_CalcVols_f);
	Cmd_AddCommand("s_dynamic", S_SetDynamicMusic_f, "Change dynamic music state" );

    S_AL_InitCvars();
	if (S_AL_IsEnabled())
	{
        r = S_AL_Init() ? qtrue : qfalse;
	}
	else
	{
		r = SNDDMA_Init(s_khz->integer);
	}

    if ( r ) {
        s_soundStarted = 1;
        s_soundMuted = qtrue;
//		s_numSfx = 0;	// do NOT do this here now!!!

        s_soundtime = 0;
        s_paintedtime = 0;

        S_StopAllSounds ();

        S_SoundInfo_f();
    }

	Com_Printf("------------------------------------\n");

	Com_Printf("\n--- ambient sound initialization ---\n");

	AS_Init();
}

// only called from snd_restart. QA request...
//
void S_ReloadAllUsedSounds(void)
{
	if (s_soundStarted && !s_soundMuted )
	{
		// new bit, reload all soundsthat are used on the current level...
		//
		for (int i=1 ; i < s_numSfx ; i++)	// start @ 1 to skip freeing default sound
		{
			sfx_t *sfx = &s_knownSfx[i];

			if (!sfx->bInMemory && !sfx->bDefaultSound && sfx->iLastLevelUsedOn == re->RegisterMedia_GetLevel()){
				S_memoryLoad(sfx);
			}
		}
	}
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown( void )
{
	if ( !s_soundStarted ) {
		return;
	}

	S_FreeAllSFXMem();
	S_UnCacheDynamicMusic();

	if (S_AL_IsEnabled())
	{
        S_AL_Shutdown();
	}
	else
	{
		SNDDMA_Shutdown();
	}

	s_soundStarted = 0;

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopmusic");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");
	Cmd_RemoveCommand("soundstop");
	Cmd_RemoveCommand("mp3_calcvols");
	Cmd_RemoveCommand("s_dynamic");
	AS_Free();
}

// =======================================================================
// Load a sound
// =======================================================================
/*
================
return a hash value for the sfx name
================
*/
static long S_HashSFXName(const char *name) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (name[i] != '\0') {
		letter = tolower(name[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (LOOP_HASH-1);
	return hash;
}

/*
==================
S_FindName

Will allocate a new sfx if it isn't found
==================
*/
sfx_t *S_FindName( const char *name ) {
	int		i;
	int		hash;

	sfx_t	*sfx;

	if (!name) {
		Com_Error (ERR_FATAL, "S_FindName: NULL");
	}
	if (!name[0]) {
		Com_Error (ERR_FATAL, "S_FindName: empty name");
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);
	}

	char sSoundNameNoExt[MAX_QPATH];
	COM_StripExtension(name,sSoundNameNoExt, sizeof( sSoundNameNoExt ));

	hash = S_HashSFXName(sSoundNameNoExt);

	sfx = sfxHash[hash];
	// see if already loaded
	while (sfx) {
		if (!Q_stricmp(sfx->sSoundName, sSoundNameNoExt) ) {
			return sfx;
		}
		sfx = sfx->next;
	}
/*
	// find a free sfx
	for (i=0 ; i < s_numSfx ; i++) {
		if (!s_knownSfx[i].soundName[0]) {
			break;
		}
	}
*/
	i = s_numSfx;	//we don't clear the soundName after failed loads any more, so it'll always be the last entry

	if (s_numSfx == MAX_SFX)
	{
		// ok, no sfx's free, but are there any with defaultSound set? (which the registering ent will never
		//	see because he gets zero returned if it's default...)
		//
		for (i=0 ; i < s_numSfx ; i++) {
			if (s_knownSfx[i].bDefaultSound) {
				break;
			}
		}

		if (i==s_numSfx)
		{
			// genuinely out of handles...

			// if we ever reach this, let me know and I'll either boost the array or put in a map-used-on
			//	reference to enable sfx_t recycling. TA codebase relies on being able to have structs for every sound
			//	used anywhere, ever, all at once (though audio bit-buffer gets recycled). SOF1 used about 1900 distinct
			//	events, so current MAX_SFX limit should do, or only need a small boost...	-ste
			//

			Com_Error (ERR_FATAL, "S_FindName: out of sfx_t");
		}
	}
	else
	{
		s_numSfx++;
	}

	sfx = &s_knownSfx[i];
	memset (sfx, 0, sizeof(*sfx));
	Q_strncpyz(sfx->sSoundName, sSoundNameNoExt, sizeof(sfx->sSoundName));
	Q_strlwr(sfx->sSoundName);//force it down low

	sfx->next = sfxHash[hash];
	sfxHash[hash] = sfx;

	return sfx;
}

/*
=================
S_DefaultSound
=================
*/
void S_DefaultSound( sfx_t *sfx ) {

	int		i;

	sfx->iSoundLengthInSamples	= 512;								// #samples, ie shorts
	sfx->pSoundData				= (short *)	SND_malloc(512*2, sfx);	// ... so *2 for alloc bytes
	sfx->bInMemory				= qtrue;

	for ( i=0 ; i < sfx->iSoundLengthInSamples ; i++ )
	{
		sfx->pSoundData[i] = i;
	}
}

/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_DisableSounds( void ) {
	S_StopAllSounds();
	s_soundMuted = qtrue;
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration( void )
{
	s_soundMuted = qfalse;		// we can play again

    S_AL_OnRegistration();

	if (s_numSfx == 0) {
		SND_setup();

		s_numSfx = 0;
		memset( s_knownSfx, 0, sizeof( s_knownSfx ) );
		memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

#ifdef _DEBUG
		sfx_t *sfx = S_FindName( "***DEFAULT***" );
		S_DefaultSound( sfx );
#else
		S_RegisterSound("sound/null.wav");
#endif
	}
}

/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t	S_RegisterSound( const char *name)
{
	if (!s_soundStarted) {
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( S_COLOR_RED"Sound name exceeds MAX_QPATH - %s\n", name );
		return 0;
	}

	sfx_t* sfx = S_FindName( name );

	SND_TouchSFX(sfx);

	if ( sfx->bDefaultSound )
		return 0;

    if ( sfx->pSoundData )
    {
        return sfx - s_knownSfx;
    }

	if (S_AL_IsEnabled() && sfx->Buffer)
	{
        return sfx - s_knownSfx;
	}

	sfx->bInMemory = qfalse;

	S_memoryLoad(sfx);

	if ( sfx->bDefaultSound ) {
#ifndef FINAL_BUILD
		if (!s_shutUp)
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: could not find %s - using default\n", sfx->sSoundName );
		}
#endif
		return 0;
	}

	return sfx - s_knownSfx;
}

void S_memoryLoad(sfx_t	*sfx)
{
	// load the sound file...
	//
	if ( !S_LoadSound( sfx ) )
	{
//		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n", sfx->sSoundName );
		sfx->bDefaultSound = qtrue;
	}
	sfx->bInMemory = qtrue;
}

//=============================================================================
static qboolean S_CheckChannelStomp( int chan1, int chan2 )
{
	if (!S_AL_IsEnabled())
	{
		if ( chan1 == chan2 )
		{
			return qtrue;
		}
	}

	if ( ( chan1 == CHAN_VOICE || chan1 == CHAN_VOICE_ATTEN || chan1 == CHAN_VOICE_GLOBAL  ) && ( chan2 == CHAN_VOICE || chan2 == CHAN_VOICE_ATTEN || chan2 == CHAN_VOICE_GLOBAL ) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
S_PickChannel
=================
*/
channel_t *S_PickChannel(int entnum, int entchannel)
{
    int			ch_idx;
	channel_t	*ch, *firstToDie;
	qboolean	foundChan = qfalse;

	if (S_AL_IsEnabled())
		return S_AL_PickChannel(entnum, entchannel);

	if ( entchannel<0 ) {
		Com_Error (ERR_DROP, "S_PickChannel: entchannel<0");
	}

	// Check for replacement sound, or find the best one to replace

    firstToDie = &s_channels[0];

	for ( int pass = 0; (pass < ((entchannel == CHAN_AUTO || entchannel == CHAN_LESS_ATTEN)?1:2)) && !foundChan; pass++ )
	{
		for (ch_idx = 0, ch = &s_channels[0]; ch_idx < MAX_CHANNELS ; ch_idx++, ch++ )
		{
			if ( entchannel == CHAN_AUTO || entchannel == CHAN_LESS_ATTEN || pass > 0 )
			{//if we're on the second pass, just find the first open chan
				if ( !ch->thesfx )
				{//grab the first open channel
					firstToDie = ch;
					break;
				}

			}
			else if ( ch->entnum == entnum && S_CheckChannelStomp( ch->entchannel, entchannel ) )
			{
				// always override sound from same entity
				if ( s_show->integer == 1 && ch->thesfx ) {
					Com_Printf( S_COLOR_YELLOW"...overrides %s\n", ch->thesfx->sSoundName );
					ch->thesfx = 0;	//just to clear the next error msg
				}
				firstToDie = ch;
				foundChan = qtrue;
				break;
			}

			// don't let anything else override local player sounds
			if ( ch->entnum == listener_number 	&& entnum != listener_number && ch->thesfx) {
				continue;
			}

			// don't override loop sounds
			if ( ch->loopSound ) {
				continue;
			}

			if ( ch->startSample < firstToDie->startSample ) {
				firstToDie = ch;
			}
		}
	}

	if ( s_show->integer == 1 && firstToDie->thesfx ) {
		Com_Printf( S_COLOR_RED"***kicking %s\n", firstToDie->thesfx->sSoundName );
	}

	Channel_Clear(firstToDie);	// memset(firstToDie, 0, sizeof(*firstToDie));

	return firstToDie;
}

/*
=================
S_SpatializeOrigin

Used for spatializing s_channels
=================
*/
void S_SpatializeOrigin (const vec3_t origin, float master_vol, int *left_vol, int *right_vol, int channel)
{
    float		dot;
    float		dist;
    float		lscale, rscale, scale;
    vec3_t		source_vec;
	float		dist_mult = SOUND_ATTENUATE;

	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	if ( channel == CHAN_VOICE )
	{
		dist -= SOUND_FULLVOLUME * 3.0f;
//		dist_mult = VOICE_ATTENUATE;	// tweak added (this fixes an NPC dialogue "in your ears" bug, but we're not sure if it'll make a bunch of others fade too early. Too close to shipping...)
	}
	else if ( channel == CHAN_LESS_ATTEN )
	{
		dist -= SOUND_FULLVOLUME * 8.0f; // maybe is too large
	}
	else if ( channel == CHAN_VOICE_ATTEN )
	{
		dist -= SOUND_FULLVOLUME * 1.35f; // used to be 0.15f, dropped off too sharply - dmv
		dist_mult = VOICE_ATTENUATE;
	}
	else if ( channel == CHAN_VOICE_GLOBAL )
	{
		dist = -1;
	}
	else	// use normal attenuation.
	{
		dist -= SOUND_FULLVOLUME;
	}

	if (dist < 0)
	{
		dist = 0;			// close enough to be at full volume
	}
	dist *= dist_mult;		// different attenuation levels

	dot = -DotProduct(listener_axis[1], source_vec);

	if (dma.channels == 1)	// || !dist_mult)
	{ // no attenuation = no spatialization
		rscale = SOUND_FMAXVOL;
		lscale = SOUND_FMAXVOL;
	}
	else
	{
		//rscale = 0.5 * (1.0 + dot);
		//lscale = 0.5 * (1.0 - dot);
		rscale = s_separation->value + ( 1.0f - s_separation->value ) * dot;
		lscale = s_separation->value - ( 1.0f - s_separation->value ) * dot;
		if ( rscale < 0 )
		{
			rscale = 0;
		}
		if ( lscale < 0 )
		{
			lscale = 0;
		}
	}

	// add in distance effect
	scale = (1.0f - dist) * rscale;
	*right_vol = (int) (master_vol * scale);
	if (*right_vol < 0)
	{
		*right_vol = 0;
	}

	scale = (1.0f - dist) * lscale;
	*left_vol = (int) (master_vol * scale);
	if (*left_vol < 0)
	{
		*left_vol = 0;
	}
}

// =======================================================================
// Start a sound effect
// =======================================================================

/*
====================
S_StartAmbientSound

Starts an ambient, 'one-shot" sound.
====================
*/

void S_StartAmbientSound( const vec3_t origin, int entityNum, unsigned char volume, sfxHandle_t sfxHandle )
{
	channel_t	*ch;
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
	if ( !origin && ( entityNum < 0 || entityNum >= MAX_GENTITIES ) )
		Com_Error( ERR_DROP, "S_StartAmbientSound: bad entitynum %i", entityNum );

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx )
		Com_Error( ERR_DROP, "S_StartAmbientSound: handle %i out of range", sfxHandle );

	sfx = &s_knownSfx[ sfxHandle ];
	if (sfx->bInMemory == qfalse){
		S_memoryLoad(sfx);
	}
	SND_TouchSFX(sfx);

	if (S_AL_IsEnabled() && volume == 0)
	{
        return;
	}

	if ( s_show->integer == 1 ) {
		Com_Printf( "%i : %s on (%d) Ambient\n", s_paintedtime, sfx->sSoundName, entityNum );
	}

	// pick a channel to play on
	ch = S_PickChannel( entityNum, CHAN_AMBIENT );
	if (!ch) {
		return;
	}

	if (origin)
	{
		VectorCopy (origin, ch->origin);
		ch->fixed_origin = qtrue;
	}
	else
	{
		ch->fixed_origin = qfalse;
	}

	ch->master_vol = volume;
	ch->entnum = entityNum;
	ch->entchannel = CHAN_AMBIENT;
	ch->thesfx = sfx;
	ch->startSample = START_SAMPLE_IMMEDIATE;

	ch->leftvol = ch->master_vol;		// these will get calced at next spatialize
	ch->rightvol = ch->master_vol;		// unless the game isn't running

	if (sfx->pMP3StreamHeader)
	{
		memcpy(&ch->MP3StreamHeader,sfx->pMP3StreamHeader,	sizeof(ch->MP3StreamHeader));
		//ch->iMP3SlidingDecodeWritePos = 0; // These will be zero from the memset in S_PickChannel(), but keep them here for reference...
		//ch->iMP3SlidingDecodeWindowPos= 0; //
	}
	else
	{
		memset(&ch->MP3StreamHeader,0,						sizeof(ch->MP3StreamHeader));
	}
}

/*
====================
S_MuteSound

Mutes sound on specified channel for specified entity.
====================
*/
void S_MuteSound(int entityNum, int entchannel)
{
	//I guess this works.
	channel_t *ch = S_PickChannel( entityNum, entchannel );

	if (!ch)
	{
		return;
	}

	ch->master_vol = 0;
	ch->entnum = 0;
	ch->entchannel = 0;
	ch->thesfx = 0;
	ch->startSample = 0;

	ch->leftvol = 0;
	ch->rightvol = 0;
}

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle )
{
	channel_t	*ch;
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( !origin && ( entityNum < 0 || entityNum >= MAX_GENTITIES ) ) {
		Com_Error( ERR_DROP, "S_StartSound: bad entitynum %i", entityNum );
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartSound: handle %i out of range", sfxHandle );
	}

	sfx = &s_knownSfx[ sfxHandle ];
	if (sfx->bInMemory == qfalse){
		S_memoryLoad(sfx);
	}
	SND_TouchSFX(sfx);

	if ( s_show->integer == 1 ) {
		Com_Printf( "%i : %s on (%d)\n", s_paintedtime, sfx->sSoundName, entityNum );
	}

    S_AL_OnStartSound(entityNum, entchannel);

	ch = S_PickChannel( entityNum, entchannel );
	if (!ch) {
		return;
	}

	if (origin) {
		VectorCopy (origin, ch->origin);
		ch->fixed_origin = qtrue;
	} else {
		ch->fixed_origin = qfalse;
	}

	ch->master_vol = SOUND_MAXVOL;	//FIXME: Um.. control?
	ch->entnum = entityNum;
	ch->entchannel = entchannel;
	ch->thesfx = sfx;
	ch->startSample = START_SAMPLE_IMMEDIATE;

	ch->leftvol = ch->master_vol;		// these will get calced at next spatialize
	ch->rightvol = ch->master_vol;		// unless the game isn't running

	if (entchannel < CHAN_AMBIENT && entityNum == listener_number) {	//only do it for body sounds not local sounds
		ch->master_vol = SOUND_MAXVOL * SOUND_FMAXVOL;	//this won't be attenuated so let it scale down
	}
	if ( entchannel == CHAN_VOICE || entchannel == CHAN_VOICE_ATTEN || entchannel == CHAN_VOICE_GLOBAL )
	{
		s_entityWavVol[ ch->entnum ] = -1;	//we've started the sound but it's silent for now
	}

	if (sfx->pMP3StreamHeader)
	{
		memcpy(&ch->MP3StreamHeader,sfx->pMP3StreamHeader,	sizeof(ch->MP3StreamHeader));
		//ch->iMP3SlidingDecodeWritePos = 0; // These will be zero from the memset in S_PickChannel(), but keep them here for reference...
		//ch->iMP3SlidingDecodeWindowPos= 0; //
	}
	else
	{
		memset(&ch->MP3StreamHeader,0,						sizeof(ch->MP3StreamHeader));
	}
}

/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartLocalSound: handle %i out of range", sfxHandle );
	}

	S_StartSound (NULL, listener_number, channelNum, sfxHandle );
}


/*
==================
S_StartLocalLoopingSound
==================
*/
void S_StartLocalLoopingSound( sfxHandle_t sfxHandle) {
	vec3_t nullVec = {0,0,0};

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartLocalLoopingSound: handle %i out of range", sfxHandle );
	}

	S_AddLoopingSound( listener_number, nullVec, nullVec, sfxHandle );

}

// returns length in milliseconds of supplied sound effect...  (else 0 for bad handle now)
//
float S_GetSampleLengthInMilliSeconds( sfxHandle_t sfxHandle)
{
	sfx_t *sfx;

	if (!s_soundStarted)
	{	//we have no sound, so let's just make a reasonable guess
		return 512 * 1000;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx )
		return 0.0f;

	sfx = &s_knownSfx[ sfxHandle ];

	float f = (float)sfx->iSoundLengthInSamples / (float)dma.speed;

	return (f * 1000);
}


/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_ClearSoundBuffer( void ) {
	int		clear;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
#if 0	//this causes scripts to freak when the sounds get cut...
	// clear all the sounds so they don't
	// start back up after the load finishes
	memset( s_channels, 0, sizeof( s_channels ) );
	// clear out the lip synching override array
	memset(s_entityWavVol, 0,sizeof(s_entityWavVol));
#endif
	s_rawend = 0;

	if (S_AL_IsEnabled())
	{
        S_AL_ClearSoundBuffer();
	}
    else
	{
		if (dma.samplebits == 8)
			clear = 0x80;
		else
			clear = 0;

		SNDDMA_BeginPainting ();
		if (dma.buffer)
			memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
		SNDDMA_Submit ();
	}
}

// kinda kludgy way to stop a special-use sfx_t playing...
//
void S_CIN_StopSound(sfxHandle_t sfxHandle)
{
	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_CIN_StopSound: handle %i out of range", sfxHandle );
	}

	sfx_t *sfx = &s_knownSfx[ sfxHandle ];
	channel_t *ch = s_channels;
	int i;

	for ( i = 0; i < MAX_CHANNELS ; i++, ch++ )
	{
		if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
			continue;
		}
		if (ch->thesfx == sfx)
		{
            S_AL_ClearChannel(&s_channels[i]);

			SND_FreeSFXMem(ch->thesfx);	// heh, may as well...
			ch->thesfx = NULL;
			memset(&ch->MP3StreamHeader, 0, sizeof(MP3STREAM));
			ch->bLooping = false;
			ch->bProcessed = false;
			ch->bPlaying = false;
			ch->bStreaming = false;
			break;
		}
	}
}


/*
==================
S_StopAllSounds
==================
*/
void S_StopSounds(void)
{
	if ( !s_soundStarted ) {
		return;
	}

	// stop looping sounds
	S_ClearLoopingSounds();

	// clear all the s_channels
	if (S_AL_IsEnabled())
	{
        S_AL_StopSounds();
	}
	else
	{
		memset(s_channels, 0, sizeof(s_channels));
	}

	// clear out the lip synching override array
	memset(s_entityWavVol, 0,sizeof(s_entityWavVol));

	S_ClearSoundBuffer ();
}

/*
==================
S_StopAllSounds
 and music
==================
*/
void S_StopAllSounds(void) {
	if ( !s_soundStarted ) {
		return;
	}
	// stop the background music
	S_StopBackgroundTrack();

	S_StopSounds();
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

/*
==================
S_ClearLoopingSounds

==================
*/
void S_ClearLoopingSounds( void )
{
    S_AL_OnClearLoopingSounds();

	numLoopSounds = 0;
}

/*
==================
S_StopLoopingSound

Stops all active looping sounds on a specified entity.
Sort of a slow method though, isn't there some better way?
==================
*/
void S_StopLoopingSound( int entityNum )
{
	int i = 0;

	while (i < numLoopSounds)
	{
		if (loopSounds[i].entnum == entityNum)
		{
			int x = i+1;
			while (x < numLoopSounds)
			{
				memcpy(&loopSounds[x-1], &loopSounds[x], sizeof(loopSounds[x]));
				x++;
			}
			numLoopSounds--;
		}
		i++;
	}
}

#define MAX_DOPPLER_SCALE 50.0f //arbitrary

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
Include velocity in case I get around to doing doppler...
==================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle ) {
	/*const*/ sfx_t *sfx;

  	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
	if ( numLoopSounds >= MAX_LOOP_SOUNDS ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_AddLoopingSound: handle %i out of range", sfxHandle );
	}

	sfx = &s_knownSfx[ sfxHandle ];
	if (sfx->bInMemory == qfalse) {
		S_memoryLoad(sfx);
	}
	SND_TouchSFX(sfx);

	if ( !sfx->iSoundLengthInSamples ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->sSoundName );
	}
	assert(!sfx->pMP3StreamHeader);

    loopSound_t* loopSound = loopSounds + numLoopSounds++;

	VectorCopy( origin, loopSound->origin );
	VectorCopy( velocity, loopSound->velocity );
	loopSound->doppler = qfalse;
	loopSound->dopplerScale = 1.0;
	loopSound->sfx = sfx;
	loopSound->volume = SOUND_MAXVOL;
	loopSound->entnum = entityNum;

	if ( s_doppler->integer && VectorLengthSquared(velocity) > 0.0 ) {
		vec3_t	out;
		float	lena, lenb;

		loopSound->doppler = qtrue;
		lena = DistanceSquared(listener_origin, loopSound->origin);
		VectorAdd(loopSound->origin, loopSound->velocity, out);
		lenb = DistanceSquared(listener_origin, out);

		loopSound->dopplerScale = lenb/(lena*100);
		if (loopSound->dopplerScale > MAX_DOPPLER_SCALE) {
			loopSound->dopplerScale = MAX_DOPPLER_SCALE;
		} else if (loopSound->dopplerScale <= 1.0) {
			loopSound->doppler = qfalse;			// don't bother doing the math
		}
	}
}


/*
==================
S_AddAmbientLoopingSound
==================
*/
void S_AddAmbientLoopingSound( const vec3_t origin, unsigned char volume, sfxHandle_t sfxHandle )
{
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
	if ( numLoopSounds >= MAX_LOOP_SOUNDS ) {
		return;
	}

	if (S_AL_IsEnabled() && volume == 0)
	{
        return;
	}

	if ( sfxHandle < 0 || sfxHandle >= s_numSfx ) {
		Com_Error( ERR_DROP, "S_StartSound: handle %i out of range", sfxHandle );
	}

	sfx = &s_knownSfx[ sfxHandle ];
	if (sfx->bInMemory == qfalse){
		S_memoryLoad(sfx);
	}
	SND_TouchSFX(sfx);

	if ( !sfx->iSoundLengthInSamples ) {
		Com_Error( ERR_DROP, "%s has length 0", sfx->sSoundName );
	}

    loopSound_t* loopSound = loopSounds + numLoopSounds++;
	VectorCopy( origin, loopSound->origin );
	loopSound->doppler = qfalse;
	loopSound->dopplerScale = 1.0;
	loopSound->sfx = sfx;
	assert(!sfx->pMP3StreamHeader);

	//TODO: Calculate the distance falloff
	loopSound->volume = volume;
}



/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds (void)
{
	int			i, j;
	int			left, right, left_total, right_total;
	channel_t	*ch;
	loopSound_t	*loop, *loop2;
	static int	loopFrame;

	loopFrame++;
	for ( i = 0 ; i < numLoopSounds ; i++) {
		loop = &loopSounds[i];
		if ( loop->mergeFrame == loopFrame ) {
			continue;	// already merged into an earlier sound
		}

		// find the total contribution of all sounds of this type
		left_total = right_total = 0;

		for ( j = i ; j < numLoopSounds ; j++) {
			loop2 = &loopSounds[j];
			if ( loop2->sfx != loop->sfx ) {
				continue;
			}
			loop2->mergeFrame = loopFrame;	// don't check this again later

			S_SpatializeOrigin( loop2->origin, loop2->volume, &left, &right, CHAN_AUTO);	//FIXME: Allow for volume change!!

			left_total += left;
			right_total += right;
		}

		if (left_total == 0 && right_total == 0)
			continue;		// not audible

		// allocate a channel
		ch = S_PickChannel(0, 0);
		if (!ch)
			return;

		if (left_total > SOUND_MAXVOL)
			left_total = SOUND_MAXVOL;
		if (right_total > SOUND_MAXVOL)
			right_total = SOUND_MAXVOL;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->loopSound = qtrue;	// remove next frame
		ch->thesfx = loop->sfx;

		ch->doppler = loop->doppler;
		ch->dopplerScale = loop->dopplerScale;

		// you cannot use MP3 files here because they offer only streaming access, not random
		//
		if (loop->sfx->pMP3StreamHeader)
		{
			Com_Error( ERR_DROP, "S_AddLoopSounds(): Cannot use streamed MP3 files here for random access (%s)\n",loop->sfx->sSoundName );
		}
		else
		{
			memset(&ch->MP3StreamHeader,0,						sizeof(ch->MP3StreamHeader));
		}
	}
}

//=============================================================================

/*
=================
S_ByteSwapRawSamples

If raw data has been loaded in little endian binary form, this must be done.
If raw data was calculated, as with ADPCM, this should not be called.
=================
*/
void S_ByteSwapRawSamples( int samples, int width, int s_channels, const byte *data ) {
	int		i;

	if ( width != 2 ) {
		return;
	}
	if ( LittleShort( 256 ) == 256 ) {
		return;
	}

	if ( s_channels == 2 ) {
		samples <<= 1;
	}
	for ( i = 0 ; i < samples ; i++ ) {
		((short *)data)[i] = LittleShort( ((short *)data)[i] );
	}
}


portable_samplepair_t *S_GetRawSamplePointer() {
	return s_rawsamples;
}


/*
============
S_RawSamples

Music streaming
============
*/
void S_RawSamples( int samples, int rate, int width, int channels, const byte *data, float volume, int bFirstOrOnlyUpdateThisFrame )
{
	int		i;
	int		src, dst;
	float	scale;
	int		intVolume;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	intVolume = 256 * volume;

	if ( s_rawend < s_soundtime ) {
		Com_DPrintf( "S_RawSamples: resetting minimum: %i < %i\n", s_rawend, s_soundtime );
		s_rawend = s_soundtime;
	}

	scale = (float)rate / dma.speed;

//Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend);
	if (channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			if (bFirstOrOnlyUpdateThisFrame)
			{
				for (i=0 ; i<samples ; i++)
				{
					dst = s_rawend&(MAX_RAW_SAMPLES-1);
					s_rawend++;
					s_rawsamples[dst].left = ((short *)data)[i*2] * intVolume;
					s_rawsamples[dst].right = ((short *)data)[i*2+1] * intVolume;
				}
			}
			else
			{
				for (i=0 ; i<samples ; i++)
				{
					dst = s_rawend&(MAX_RAW_SAMPLES-1);
					s_rawend++;
					s_rawsamples[dst].left  += ((short *)data)[i*2] * intVolume;
					s_rawsamples[dst].right += ((short *)data)[i*2+1] * intVolume;
				}
			}
		}
		else
		{
			if (bFirstOrOnlyUpdateThisFrame)
			{
				for (i=0 ; ; i++)
				{
					src = i*scale;
					if (src >= samples)
						break;
					dst = s_rawend&(MAX_RAW_SAMPLES-1);
					s_rawend++;
					s_rawsamples[dst].left = ((short *)data)[src*2] * intVolume;
					s_rawsamples[dst].right = ((short *)data)[src*2+1] * intVolume;
				}
			}
			else
			{
				for (i=0 ; ; i++)
				{
					src = i*scale;
					if (src >= samples)
						break;
					dst = s_rawend&(MAX_RAW_SAMPLES-1);
					s_rawend++;
					s_rawsamples[dst].left  += ((short *)data)[src*2] * intVolume;
					s_rawsamples[dst].right += ((short *)data)[src*2+1] * intVolume;
				}
			}
		}
	}
	else if (channels == 1 && width == 2)
	{
		if (bFirstOrOnlyUpdateThisFrame)
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left = ((short *)data)[src] * intVolume;
				s_rawsamples[dst].right = ((short *)data)[src] * intVolume;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left  += ((short *)data)[src] * intVolume;
				s_rawsamples[dst].right += ((short *)data)[src] * intVolume;
			}
		}
	}
	else if (channels == 2 && width == 1)
	{
		intVolume *= 256;

		if (bFirstOrOnlyUpdateThisFrame)
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left = ((char *)data)[src*2] * intVolume;
				s_rawsamples[dst].right = ((char *)data)[src*2+1] * intVolume;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left  += ((char *)data)[src*2] * intVolume;
				s_rawsamples[dst].right += ((char *)data)[src*2+1] * intVolume;
			}
		}
	}
	else if (channels == 1 && width == 1)
	{
		intVolume *= 256;

		if (bFirstOrOnlyUpdateThisFrame)
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left = (((byte *)data)[src]-128) * intVolume;
				s_rawsamples[dst].right = (((byte *)data)[src]-128) * intVolume;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left  += (((byte *)data)[src]-128) * intVolume;
				s_rawsamples[dst].right += (((byte *)data)[src]-128) * intVolume;
			}
		}
	}

	if ( s_rawend > s_soundtime + MAX_RAW_SAMPLES ) {
		Com_DPrintf( "S_RawSamples: overflowed %i > %i\n", s_rawend, s_soundtime );
	}
}

//=============================================================================

/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}

    S_AL_OnUpdateEntityPosition(entityNum, origin);

	VectorCopy( origin, s_entityPosition[entityNum] );
}


// Given a current wav we are playing, and our position within it, lets figure out its volume...
//
// (this is mostly Jake's code from EF1, which explains a lot...:-)
//
static int next_amplitude = 0;
static int S_CheckAmplitude(channel_t	*ch, const unsigned int s_oldpaintedtime )
{
	// now, is this a cycle - or have we just started a new sample - where we should update the backup table, and write this value
	// into the new table? or should we just take the value FROM the back up table and feed it out.
	assert( ch->startSample != START_SAMPLE_IMMEDIATE );
	if ( ch->startSample == s_oldpaintedtime || (next_amplitude < s_soundtime) )//(ch->startSample == START_SAMPLE_IMMEDIATE)//!s_entityWavVol_back[ch->entnum]
	{
		int	sample;
		int	sample_total = 0;
		int	count = 0;
		short *current_pos_s;
//		char *current_pos_c;
		int	offset = 0;

		// if we haven't started the sample yet, we must be at the beginning
		current_pos_s = ((short*)ch->thesfx->pSoundData);
//		current_pos_c = ((char*)ch->thesfx->data);

		//if (ch->startSample != START_SAMPLE_IMMEDIATE)
		//{
			// figure out where we are in the sample right now.
			offset = s_oldpaintedtime - ch->startSample;//s_paintedtime
			current_pos_s += offset;
//			current_pos_c += offset;
		//}

		// scan through 10 samples 100( at 11hz or 200 at 22hz) samples apart.
		//
		for (int i=0; i<10; i++)
		{
			//
			// have we run off the end?
			if ((offset + (i*100)) > ch->thesfx->iSoundLengthInSamples)
			{
				break;
			}
//			if (ch->thesfx->width == 1)
//			{
//				sample = current_pos_c[i*100];
//			}
//			else
			{
				switch (ch->thesfx->eSoundCompressionMethod)
				{
					case ct_16:
					{
						sample = current_pos_s[i*100];
					}
					break;

					case ct_MP3:
					{
						const int iIndex = (i*100) + ((offset * /*ch->thesfx->width*/2) - ch->iMP3SlidingDecodeWindowPos);
						const short* pwSamples = (short*) (ch->MP3SlidingDecodeBuffer + iIndex);

						sample = *pwSamples;
					}
					break;

					default:
					{
						assert(0);
						sample = 0;
					}
					break;
				}

//				if (sample < 0)
//					sample = -sample;
				sample = sample>>8;
			}
			// square it for better accuracy
			sample_total += (sample * sample);
			count++;
		}

		// if we are already done with this sample, then its silence
		if (!count)
		{
			return(0);
		}
		sample_total /= count;

		// I hate doing this, but its the simplest way
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_1->value)
		{
		// tell the scripts that are relying on this that we are still going, but actually silent right now.
			sample = -1;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_2->value)
		{
			sample = 1;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_3->value)
		{
			sample = 2;
		}
		else
		if (sample_total < ch->thesfx->fVolRange * s_lip_threshold_4->value)
		{
			sample = 3;
		}
		else
		{
			sample = 4;
		}

//		Com_OPrintf("Returning sample %d\n",sample);

		// store away the value we got into the back up table
		s_entityWavVol_back[ ch->entnum ] = sample;
		return (sample);
	}
	// no, just get last value calculated from backup table
	assert( s_entityWavVol_back[ch->entnum] );
	return (s_entityWavVol_back[ ch->entnum]);
}

/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, matrix3_t axis, int inwater )
{
	int			i;
	channel_t	*ch;
	vec3_t		origin;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if (S_AL_IsEnabled())
	{
        S_AL_Respatialize(entityNum, head, axis, inwater);
	}
	else
	{
		listener_number = entityNum;
		VectorCopy(head, listener_origin);
		VectorCopy(axis[0], listener_axis[0]);
		VectorCopy(axis[1], listener_axis[1]);
		VectorCopy(axis[2], listener_axis[2]);

		// update spatialization for dynamic sounds
		ch = s_channels;
		for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
			if ( !ch->thesfx ) {
				continue;
			}
			if ( ch->loopSound ) {	// loopSounds are regenerated fresh each frame
				Channel_Clear(ch);	// memset (ch, 0, sizeof(*ch));
				continue;
			}

			// anything coming from the view entity will always be full volume
			if (ch->entnum == listener_number) {
				ch->leftvol = ch->master_vol;
				ch->rightvol = ch->master_vol;
			} else {
				if (ch->fixed_origin) {
					VectorCopy( ch->origin, origin );
				} else {
					VectorCopy( s_entityPosition[ ch->entnum ], origin );
				}

				S_SpatializeOrigin (origin, (float)ch->master_vol, &ch->leftvol, &ch->rightvol, ch->entchannel);
			}

			//NOTE: Made it so that voice sounds keep playing, even out of range
			//		so that tasks waiting for sound completion keep proper timing
			if ( !( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL ) && !ch->leftvol && !ch->rightvol ) {
				Channel_Clear(ch);	// memset (ch, 0, sizeof(*ch));
				continue;
			}
		}

		// add loopsounds
		S_AddLoopSounds ();
	}
}


/*
========================
S_ScanChannelStarts

Returns qtrue if any new sounds were started since the last mix
========================
*/
qboolean S_ScanChannelStarts( void ) {
	channel_t		*ch;
	int				i;
	qboolean		newSamples;

	newSamples = qfalse;
	ch = s_channels;
	for (i=0; i<MAX_CHANNELS ; i++, ch++) {
		if ( !ch->thesfx ) {
			continue;
		}
		if ( ch->loopSound ) {
			continue;
		}

		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if ( ch->startSample == START_SAMPLE_IMMEDIATE ) {
			ch->startSample = s_paintedtime;
			newSamples = qtrue;
			continue;
		}

		// if it is completely finished by now, clear it
		if ( (int)(ch->startSample + ch->thesfx->iSoundLengthInSamples) <= s_paintedtime ) {
			ch->thesfx = NULL;
			continue;
		}
	}

	return newSamples;
}

// this is now called AFTER the DMA painting, since it's only the painter calls that cause the MP3s to be unpacked,
//	and therefore to have data readable by the lip-sync volume calc code.
//
void S_DoLipSynchs( const unsigned s_oldpaintedtime )
{
	channel_t		*ch;
	int				i;

	// clear out the lip synching override array for this frame
	memset(s_entityWavVol, 0,(MAX_GENTITIES * 4));

	ch = s_channels;
	for (i=0; i<MAX_CHANNELS ; i++, ch++) {
		if ( !ch->thesfx ) {
			continue;
		}
		if ( ch->loopSound ) {
			continue;
		}

		// if we are playing a sample that should override the lip texture on its owning model, lets figure out
		// what the amplitude is, stick it in a table, then return it
		if ( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL )
		{
			// go away and work out amplitude for this sound we are playing right now.
			s_entityWavVol[ ch->entnum ] = S_CheckAmplitude( ch, s_oldpaintedtime );
			if ( s_show->integer == 3 ) {
				Com_Printf( "(%i)%i %s vol = %i\n", ch->entnum, i, ch->thesfx->sSoundName, s_entityWavVol[ ch->entnum ] );
			}
		}
	}

	if (next_amplitude < s_soundtime)	{
		next_amplitude = s_soundtime + 800;
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( void ) {
	int			i;
	int			total;
	channel_t	*ch;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	//
	// debugging output
	//
	if ( s_show->integer == 2 ) {
		total = 0;
		int totalMeg =0;
		ch = s_channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (ch->thesfx && (ch->leftvol || ch->rightvol) ) {
				Com_Printf ("(%i) %3i %3i %s\n", ch->entnum, ch->leftvol, ch->rightvol, ch->thesfx->sSoundName);
				total++;
				totalMeg += Z_Size(ch->thesfx->pSoundData);
				if (ch->thesfx->pMP3StreamHeader)
				{
					totalMeg += sizeof(*ch->thesfx->pMP3StreamHeader);
				}
			}
		}

		if (total)
			Com_Printf ("----(%i)---- painted: %i, SND %.2fMB\n", total, s_paintedtime, totalMeg/1024.0f/1024.0f);
	}

	// The Open AL code, handles background music in the S_UpdateRawSamples function
	if (!S_AL_IsEnabled())
	{
		// add raw data from streamed samples
		S_UpdateBackgroundTrack();
	}

	// mix some sound
	S_Update_();
}

void S_GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;

	fullsamples = dma.samples / dma.channels;

	if( CL_VideoRecording( ) )
	{
		float fps = Q_min(cl_aviFrameRate->value, 1000.0f);
		float frameDuration = Q_max(dma.speed / fps, 1.0f) + clc.aviSoundFrameRemainder;
		int msec = (int)frameDuration;
		s_soundtime += msec;
		clc.aviSoundFrameRemainder = frameDuration - msec;
		return;
	}

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();
	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped

		if (s_paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds ();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers*fullsamples + samplepos/dma.channels;

#if 0
// check to make sure that we haven't overshot
	if (s_paintedtime < s_soundtime)
	{
		Com_DPrintf ("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}
#endif

	if ( dma.submission_chunk < 256 ) {
		s_paintedtime = (int)(s_soundtime + s_mixPreStep->value * dma.speed);
	} else {
		s_paintedtime = s_soundtime + dma.submission_chunk;
	}
}


void S_Update_(void) {
	unsigned        endtime;
	int				samps;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if (S_AL_IsEnabled())
	{
        S_AL_Update();
	}
	else
	{
		// Updates s_soundtime
		S_GetSoundtime();

		const unsigned s_oldpaintedtime = s_paintedtime;

		// clear any sound effects that end before the current time,
		// and start any new sounds
		S_ScanChannelStarts();

		// mix ahead of current position
		endtime = (int)(s_soundtime + s_mixahead->value * dma.speed);

		// mix to an even submission block size
		endtime = (endtime + dma.submission_chunk-1)
			& ~(dma.submission_chunk-1);

		// never mix more than the complete buffer
		samps = dma.samples >> (dma.channels-1);
		if (endtime - s_soundtime > (unsigned)samps)
			endtime = s_soundtime + samps;


		SNDDMA_BeginPainting ();

		S_PaintChannels (endtime);

		SNDDMA_Submit ();

		S_DoLipSynchs( s_oldpaintedtime );
	}
}

/*
===============================================================================

console functions

===============================================================================
*/

static void S_Play_f( void ) {
	int 	i;
	sfxHandle_t	h;
	char name[256];

	i = 1;
	while ( i<Cmd_Argc() ) {
		if ( !strrchr(Cmd_Argv(i), '.') ) {
			Com_sprintf( name, sizeof(name), "%s.wav", Cmd_Argv(1) );
		} else {
			Q_strncpyz( name, Cmd_Argv(i), sizeof(name) );
		}
		h = S_RegisterSound( name );
		if( h ) {
			S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

static void S_Music_f( void ) {
	int		c;

	c = Cmd_Argc();

	if ( c == 2 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(1), qfalse );
	} else if ( c == 3 ) {
		S_StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(2), qfalse );
	} else {
		Com_Printf ("music <musicfile> [loopfile]\n");
		return;
	}
}

static void S_StopMusic_f( void ) {
	S_StopBackgroundTrack();
}

// a debug function, but no harm to leave in...
//
static void S_SetDynamicMusic_f(void)
{
	int c = Cmd_Argc();

	if ( c == 2 )
	{
		if (bMusic_IsDynamic)
		{
			// don't need to check existance of 'explore' or 'action' music, since music wouldn't
			//	be counted as dynamic if either were missing, but other types are optional...
			//
			if (!Q_stricmp(Cmd_Argv(1),"explore"))
			{
				S_SetDynamicMusicState( eBGRNDTRACK_EXPLORE );
				return;
			}
			else
			if (!Q_stricmp(Cmd_Argv(1),"action"))
			{
				S_SetDynamicMusicState( eBGRNDTRACK_ACTION );
				return;
			}
			else
			if (!Q_stricmp(Cmd_Argv(1),"silence"))
			{
				S_SetDynamicMusicState( eBGRNDTRACK_SILENCE );
				return;
			}
			else
			if (!Q_stricmp(Cmd_Argv(1),"boss"))
			{
				if (tMusic_Info[ eBGRNDTRACK_BOSS ].bExists)
				{
					S_SetDynamicMusicState( eBGRNDTRACK_BOSS );
				}
				else
				{
					Com_Printf("No 'boss' music defined in current dynamic set\n");
				}
				return;
			}
			else
			if (!Q_stricmp(Cmd_Argv(1),"death"))
			{
				if (tMusic_Info[ eBGRNDTRACK_DEATH ].bExists)
				{
					S_SetDynamicMusicState( eBGRNDTRACK_DEATH );
				}
				else
				{
					Com_Printf("No 'death' music defined in current dynamic set\n");
				}
				return;
			}
		}
		else
		{
			DynamicMusicInfoPrint();	// print "inactive" string
			return;
		}
	}

	// show usage...
	//
	Com_Printf("Usage: s_dynamic <explore/action/silence/boss/death>\n");
	DynamicMusicInfoPrint();
}

// this table needs to be in-sync with the typedef'd enum "SoundCompressionMethod_t"...	-ste
//
static const char *sSoundCompressionMethodStrings[ct_NUMBEROF] =
{
	"16b",	// ct_16
	"mp3"	// ct_MP3
};
void S_SoundList_f( void ) {
	int		i;
	sfx_t	*sfx;
	int		size, total;
	int		iVariantCap = -1;	// for %d-inquiry stuff
	int		iTotalBytes = 0;
	char	*arg = NULL;

	qboolean bWavOnly = qfalse;
	qboolean bShouldBeMP3 = qfalse;

	if ( Cmd_Argc() == 2 )
	{
		arg = Cmd_Argv(1);
		if (!Q_stricmp(arg, "shouldbeMP3"))
		{
			bShouldBeMP3 = qtrue;
		}
		else if (!Q_stricmp(arg, "wavonly"))
		{
			bWavOnly = qtrue;
		}
		else
		{
			if (!Q_stricmp(arg, "1"))
			{
				iVariantCap = 1;
			}
			else
			if (!Q_stricmp(arg, "2"))
			{
				iVariantCap = 2;
			}
			else
			if (!Q_stricmp(arg, "3"))
			{
				iVariantCap = 3;
			}
		}
	}
	else
	{
		Com_Printf("( additional (mutually exclusive) options available:\n'wavonly', 'ShouldBeMP3', '1'/'2'/'3' for %%d-variant capping )\n" );
	}

	total = 0;

	Com_Printf("\n");
	Com_Printf("                    InMemory?\n");
	Com_Printf("                    |\n");
	Com_Printf("                    |  LevelLastUsedOn\n");
	Com_Printf("                    |  |\n");
	Com_Printf("                    |  |\n");
	Com_Printf(" Slot   Smpls Type  |  |   Name\n");
//	Com_Printf(" Slot   Smpls Type  InMem?   Name\n");

	for (sfx=s_knownSfx, i=0 ; i<s_numSfx ; i++, sfx++)
	{
		extern cvar_t *cv_MP3overhead;
		qboolean bMP3DumpOverride = (qboolean)(bShouldBeMP3 && cv_MP3overhead && !sfx->bDefaultSound && !sfx->pMP3StreamHeader && sfx->pSoundData && (Z_Size(sfx->pSoundData) > cv_MP3overhead->integer));

		if (bMP3DumpOverride || (!bShouldBeMP3 && (!bWavOnly || sfx->eSoundCompressionMethod == ct_16)))
		{
			qboolean bDumpThisOne = qtrue;
			if (iVariantCap >= 1 && iVariantCap <= 3)
			{
				int iStrLen = strlen(sfx->sSoundName);
				if (iStrLen > 2)	// crash-safety, jic.
				{
					char c  = sfx->sSoundName[iStrLen-1];
					char c2 = sfx->sSoundName[iStrLen-2];
					if (!isdigit(c2) // quick-avoid of stuff like "pain75"
						&& isdigit(c) && atoi(va("%c",c)) > iVariantCap)
					{
						// need to see if this %d-variant should be omitted, in other words if there's a %1 version then skip this...
						//
						char sFindName[MAX_QPATH];
						Q_strncpyz(sFindName,sfx->sSoundName,sizeof(sFindName));
						sFindName[iStrLen-1] = '1';
						int i2;
						sfx_t *sfx2;
						for (sfx2 = s_knownSfx, i2=0 ; i2<s_numSfx ; i2++, sfx2++)
						{
							if (!Q_stricmp(sFindName,sfx2->sSoundName))
							{
								bDumpThisOne = qfalse;	// found a %1-variant of this, so use variant capping and ignore this sfx_t
								break;
							}
						}
					}
				}
			}

			size = sfx->iSoundLengthInSamples;
			if (sfx->bDefaultSound)
			{
				Com_Printf("%5d Missing file: \"%s\"\n", i, sfx->sSoundName );
			}
			else
			{
				if (bDumpThisOne)
				{
					iTotalBytes += (sfx->bInMemory && sfx->pSoundData) ? Z_Size(sfx->pSoundData) : 0;
					iTotalBytes += (sfx->bInMemory && sfx->pMP3StreamHeader) ? sizeof(*sfx->pMP3StreamHeader) : 0;
					total		+=  sfx->bInMemory ? size : 0;
				}
				Com_Printf("%5d %7i [%s] %s %2d %s", i, size, sSoundCompressionMethodStrings[sfx->eSoundCompressionMethod], sfx->bInMemory?"y":"n", sfx->iLastLevelUsedOn, sfx->sSoundName );

				if (!bDumpThisOne)
				{
					Com_Printf("   ( Skipping, variant capped )");
					//Com_OPrintf("Variant capped: %s\n",sfx->sSoundName);
				}
				Com_Printf("\n");
			}
		}
	}
	Com_Printf(" Slot   Smpls Type  In? Lev  Name\n");

	Com_Printf ("Total resident samples: %i %s ( not mem usage, see 'meminfo' ).\n", total, bWavOnly?"(WAV only)":"");
	Com_Printf ("%d out of %d sfx_t slots used\n", s_numSfx, MAX_SFX);
	Com_Printf ("%.2fMB bytes used when counting sfx_t->pSoundData + MP3 headers (if any)\n", (float)iTotalBytes / 1024.0f / 1024.0f);
	S_DisplayFreeMemory();
}

/*
===============================================================================

background music functions

===============================================================================
*/

int	FGetLittleLong( fileHandle_t f ) {
	int		v;

	FS_Read( &v, sizeof(v), f );

	return LittleLong( v);
}

int	FGetLittleShort( fileHandle_t f ) {
	short	v;

	FS_Read( &v, sizeof(v), f );

	return LittleShort( v);
}

// returns the length of the data in the chunk, or 0 if not found
int S_FindWavChunk( fileHandle_t f, char *chunk ) {
	char	name[5];
	int		len;
	int		r;

	name[4] = 0;
	len = 0;
	r = FS_Read( name, 4, f );
	if ( r != 4 ) {
		return 0;
	}
	len = FGetLittleLong( f );
	if ( len < 0 || len > 0xfffffff ) {
		len = 0;
		return 0;
	}
	len = (len + 1 ) & ~1;		// pad to word boundary
//	s_nextWavChunk += len + 8;

	if ( strcmp( name, chunk ) ) {
		return 0;
	}
	return len;
}

// fixme: need to move this into qcommon sometime?, but too much stuff altered by other people and I won't be able
//	to compile again for ages if I check that out...
//
// DO NOT replace this with a call to FS_FileExists, that's for checking about writing out, and doesn't work for this.
//
qboolean S_FileExists( const char *psFilename )
{
	fileHandle_t fhTemp;

	FS_FOpenFileRead (psFilename, &fhTemp, qtrue);	// qtrue so I can fclose the handle without closing a PAK
	if (!fhTemp)
		return qfalse;

	FS_FCloseFile(fhTemp);
	return qtrue;
}

// some stuff for streaming MP3 files from disk (not pleasant, but nothing about MP3 is, other than compression ratios...)
//
static void MP3MusicStream_Reset(MusicInfo_t *pMusicInfo)
{
	pMusicInfo->iMP3MusicStream_DiskReadPos		= 0;
	pMusicInfo->iMP3MusicStream_DiskWindowPos	= 0;
}

//
// return is where the decoder should read from...
//
static byte *MP3MusicStream_ReadFromDisk(MusicInfo_t *pMusicInfo, int iReadOffset, int iReadBytesNeeded)
{
	if (iReadOffset < pMusicInfo->iMP3MusicStream_DiskWindowPos)
	{
		assert(0);											// should never happen
		return pMusicInfo->byMP3MusicStream_DiskBuffer;		// ...but return something safe anyway
	}

	while (iReadOffset + iReadBytesNeeded > pMusicInfo->iMP3MusicStream_DiskReadPos)
	{
		int iBytesRead = FS_Read( pMusicInfo->byMP3MusicStream_DiskBuffer + (pMusicInfo->iMP3MusicStream_DiskReadPos - pMusicInfo->iMP3MusicStream_DiskWindowPos), iMP3MusicStream_DiskBytesToRead, pMusicInfo->s_backgroundFile );

		pMusicInfo->iMP3MusicStream_DiskReadPos += iBytesRead;

		if (iBytesRead != iMP3MusicStream_DiskBytesToRead)	// quietly ignore any requests to read past file end
		{
			break;		// we need to do this because the disk read code can't know how much source data we need to
						//	read for a given number of requested output bytes, so we'll always be asking for too many
		}
	}

	// if reached halfway point in buffer (approx 20k), backscroll it...
	//
	if (pMusicInfo->iMP3MusicStream_DiskReadPos - pMusicInfo->iMP3MusicStream_DiskWindowPos > iMP3MusicStream_DiskBufferSize/2)
	{
		int iMoveSrcOffset = iReadOffset - pMusicInfo->iMP3MusicStream_DiskWindowPos;
		int iMoveCount     = (pMusicInfo->iMP3MusicStream_DiskReadPos - pMusicInfo->iMP3MusicStream_DiskWindowPos ) - iMoveSrcOffset;
		memmove( &pMusicInfo->byMP3MusicStream_DiskBuffer, &pMusicInfo->byMP3MusicStream_DiskBuffer[iMoveSrcOffset], iMoveCount);
		pMusicInfo->iMP3MusicStream_DiskWindowPos += iMoveSrcOffset;
	}

	return pMusicInfo->byMP3MusicStream_DiskBuffer + (iReadOffset - pMusicInfo->iMP3MusicStream_DiskWindowPos);
}

// does NOT set s_rawend!...
//
static void S_StopBackgroundTrack_Actual( MusicInfo_t *pMusicInfo )
{
	if ( pMusicInfo->s_backgroundFile )
	{
		if ( pMusicInfo->s_backgroundFile != -1)
		{
			FS_FCloseFile( pMusicInfo->s_backgroundFile );
		}
		pMusicInfo->s_backgroundFile = 0;
	}
}

static void FreeMusic( MusicInfo_t *pMusicInfo )
{
	if (pMusicInfo->pLoadedData)
	{
		Z_Free(pMusicInfo->pLoadedData);
		pMusicInfo->pLoadedData		= NULL;		// these two MUST be kept as valid/invalid together
		pMusicInfo->sLoadedDataName[0]= '\0';	//
		pMusicInfo->iLoadedDataLen	= 0;
	}
}

// called only by snd_shutdown (from snd_restart or app exit)
//
void S_UnCacheDynamicMusic( void )
{
	for (int i = eBGRNDTRACK_DATABEGIN; i != eBGRNDTRACK_DATAEND; i++)
	{
		FreeMusic( &tMusic_Info[i]);
	}
}

static qboolean S_StartBackgroundTrack_Actual( MusicInfo_t *pMusicInfo, qboolean qbDynamic, const char *intro, const char *loop )
{
	int		len;
	char	dump[16];
	char	name[MAX_QPATH];

	Q_strncpyz( name, intro, sizeof( name ) - 4 );	// this seems to be so that if the filename hasn't got an extension
													//	but doesn't have the room to append on either then you'll just
													//	get the "soft" fopen() error, rather than the ERR_DROP you'd get
													//	if COM_DefaultExtension didn't have room to add it on.
	COM_DefaultExtension( name, sizeof( name ), ".mp3" );

	// close the background track, but DON'T reset s_rawend (or remaining music bits that haven't been output yet will be cut off)
	//
	S_StopBackgroundTrack_Actual( pMusicInfo );

	pMusicInfo->bIsMP3 = qfalse;

	if ( !intro[0] ) {
		return qfalse;
	}

	// new bit, if file requested is not same any loaded one (if prev was in-mem), ditch it...
	//
	if (Q_stricmp(name, pMusicInfo->sLoadedDataName))
	{
		FreeMusic( pMusicInfo );
	}

	if (!Q_stricmpn(name+(strlen(name)-4),".mp3",4))
	{
		if (pMusicInfo->pLoadedData)
		{
			pMusicInfo->s_backgroundFile = -1;
		}
		else
		{
			pMusicInfo->iLoadedDataLen = FS_FOpenFileRead( name, &pMusicInfo->s_backgroundFile, qtrue );
		}

		if (!pMusicInfo->s_backgroundFile)
		{
			Com_Printf( S_COLOR_RED"Couldn't open music file %s\n", name );
			return qfalse;
		}

		MP3MusicStream_Reset( pMusicInfo );

		byte *pbMP3DataSegment	= NULL;
		int iInitialMP3ReadSize = 8192;		// fairly arbitrary, whatever size this is then the decoder is allowed to
											// scan up to halfway of it to find floating headers, so don't make it
											// too small. 8k works fine.
		qboolean bMusicSucceeded = qfalse;
		if (qbDynamic)
		{
			if (!pMusicInfo->pLoadedData)
			{
				pMusicInfo->pLoadedData = (byte *) Z_Malloc(pMusicInfo->iLoadedDataLen, TAG_SND_DYNAMICMUSIC, qfalse);

				S_ClearSoundBuffer();
				FS_Read(pMusicInfo->pLoadedData, pMusicInfo->iLoadedDataLen, pMusicInfo->s_backgroundFile);
				Q_strncpyz(pMusicInfo->sLoadedDataName, name, sizeof(pMusicInfo->sLoadedDataName));
			}

			// enable the rest of the code to work as before...
			//
			pbMP3DataSegment	= pMusicInfo->pLoadedData;
			iInitialMP3ReadSize = pMusicInfo->iLoadedDataLen;
		}
		else
		{
			pbMP3DataSegment = MP3MusicStream_ReadFromDisk(pMusicInfo, 0, iInitialMP3ReadSize);
		}

		if (MP3_IsValid(name, pbMP3DataSegment, iInitialMP3ReadSize, qtrue /*bStereoDesired*/))
		{
			// init stream struct...
			//
			memset(&pMusicInfo->streamMP3_Bgrnd,0,sizeof(pMusicInfo->streamMP3_Bgrnd));
			char *psError = C_MP3Stream_DecodeInit( &pMusicInfo->streamMP3_Bgrnd, pbMP3DataSegment, pMusicInfo->iLoadedDataLen,
													dma.speed,
													16,		// sfx->width * 8,
													qtrue	// bStereoDesired
													);

			if (psError == NULL)
			{
				// init sfx struct & setup the few fields I actually need...
				//
				memset(	   &pMusicInfo->sfxMP3_Bgrnd,0,sizeof(pMusicInfo->sfxMP3_Bgrnd));
				//			pMusicInfo->sfxMP3_Bgrnd.width					= 2;			// read by MP3_GetSamples()
							pMusicInfo->sfxMP3_Bgrnd.iSoundLengthInSamples	= 0x7FFFFFFF;	// max possible +ve int, since music finishes when decoder stops
							pMusicInfo->sfxMP3_Bgrnd.pMP3StreamHeader		= &pMusicInfo->streamMP3_Bgrnd;
				Q_strncpyz( pMusicInfo->sfxMP3_Bgrnd.sSoundName, name, sizeof(pMusicInfo->sfxMP3_Bgrnd.sSoundName) );

				if (qbDynamic)
				{
					MP3Stream_InitPlayingTimeFields ( &pMusicInfo->streamMP3_Bgrnd, name, pbMP3DataSegment, pMusicInfo->iLoadedDataLen, qtrue);
				}

				pMusicInfo->s_backgroundInfo.format		= WAV_FORMAT_MP3;	// not actually used this way, but just ensures we don't match one of the legit formats
				pMusicInfo->s_backgroundInfo.channels	= 2;		// always, for our MP3s when used for music (else 1 for FX)
				pMusicInfo->s_backgroundInfo.rate		= dma.speed;
				pMusicInfo->s_backgroundInfo.width		= 2;		// always, for our MP3s
				pMusicInfo->s_backgroundInfo.samples	= pMusicInfo->sfxMP3_Bgrnd.iSoundLengthInSamples;
				pMusicInfo->s_backgroundSamples			= pMusicInfo->sfxMP3_Bgrnd.iSoundLengthInSamples;

				memset(&pMusicInfo->chMP3_Bgrnd,0,sizeof(pMusicInfo->chMP3_Bgrnd));
						pMusicInfo->chMP3_Bgrnd.thesfx = &pMusicInfo->sfxMP3_Bgrnd;
				memcpy(&pMusicInfo->chMP3_Bgrnd.MP3StreamHeader, pMusicInfo->sfxMP3_Bgrnd.pMP3StreamHeader, sizeof(*pMusicInfo->sfxMP3_Bgrnd.pMP3StreamHeader));

				if (qbDynamic)
				{
					if (pMusicInfo->s_backgroundFile != -1)
					{
						FS_FCloseFile( pMusicInfo->s_backgroundFile );
						pMusicInfo->s_backgroundFile = -1;	// special mp3 value for "valid, but not a real file"
					}
				}

				pMusicInfo->bIsMP3 = qtrue;
				bMusicSucceeded = qtrue;
			}
			else
			{
				Com_Printf(S_COLOR_RED"Error streaming file %s: %s\n", name, psError);
				if (pMusicInfo->s_backgroundFile != -1)
				{
					FS_FCloseFile( pMusicInfo->s_backgroundFile );
				}
				pMusicInfo->s_backgroundFile = 0;
			}
		}
		else
		{
			// MP3_IsValid() will already have printed any errors via Com_Printf at this point...
			//
			if (pMusicInfo->s_backgroundFile != -1)
			{
				FS_FCloseFile( pMusicInfo->s_backgroundFile );
			}
			pMusicInfo->s_backgroundFile = 0;
		}

		return bMusicSucceeded;
	}
	else	// not an mp3 file
	{
		//
		// open up a wav file and get all the info
		//
		FS_FOpenFileRead( name, &pMusicInfo->s_backgroundFile, qtrue );
		if ( !pMusicInfo->s_backgroundFile ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", name );
			return qfalse;
		}

		// skip the riff wav header

		FS_Read(dump, 12, pMusicInfo->s_backgroundFile);

		if ( !S_FindWavChunk( pMusicInfo->s_backgroundFile, "fmt " ) ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: No fmt chunk in %s\n", name );
			FS_FCloseFile( pMusicInfo->s_backgroundFile );
			pMusicInfo->s_backgroundFile = 0;
			return qfalse;
		}

		// save name for soundinfo
		pMusicInfo->s_backgroundInfo.format = FGetLittleShort( pMusicInfo->s_backgroundFile );
		pMusicInfo->s_backgroundInfo.channels = FGetLittleShort( pMusicInfo->s_backgroundFile );
		pMusicInfo->s_backgroundInfo.rate = FGetLittleLong( pMusicInfo->s_backgroundFile );
		FGetLittleLong(  pMusicInfo->s_backgroundFile );
		FGetLittleShort(  pMusicInfo->s_backgroundFile );
		pMusicInfo->s_backgroundInfo.width = FGetLittleShort( pMusicInfo->s_backgroundFile ) / 8;

		if ( pMusicInfo->s_backgroundInfo.format != WAV_FORMAT_PCM ) {
			FS_FCloseFile( pMusicInfo->s_backgroundFile );
			pMusicInfo->s_backgroundFile = 0;
			Com_Printf(S_COLOR_YELLOW "WARNING: Not a microsoft PCM format wav: %s\n", name);
			return qfalse;
		}

		if ( pMusicInfo->s_backgroundInfo.channels != 2 || pMusicInfo->s_backgroundInfo.rate != 22050 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: music file %s is not 22k stereo\n", name );
		}

		if ( ( len = S_FindWavChunk( pMusicInfo->s_backgroundFile, "data" ) ) == 0 ) {
			FS_FCloseFile( pMusicInfo->s_backgroundFile );
			pMusicInfo->s_backgroundFile = 0;
			Com_Printf(S_COLOR_YELLOW "WARNING: No data chunk in %s\n", name);
			return qfalse;
		}

		pMusicInfo->s_backgroundInfo.samples = len / (pMusicInfo->s_backgroundInfo.width * pMusicInfo->s_backgroundInfo.channels);

		pMusicInfo->s_backgroundSamples = pMusicInfo->s_backgroundInfo.samples;
	}

	return qtrue;
}

static void S_SwitchDynamicTracks( MusicState_e eOldState, MusicState_e eNewState, qboolean bNewTrackStartsFullVolume )
{
	// copy old track into fader...
	//
	tMusic_Info[ eBGRNDTRACK_FADE ] = tMusic_Info[ eOldState ];
//	tMusic_Info[ eBGRNDTRACK_FADE ].bActive = qtrue;	// inherent
//	tMusic_Info[ eBGRNDTRACK_FADE ].bExists = qtrue;	// inherent
	tMusic_Info[ eBGRNDTRACK_FADE ].iXFadeVolumeSeekTime= Sys_Milliseconds();
	tMusic_Info[ eBGRNDTRACK_FADE ].iXFadeVolumeSeekTo	= 0;
	//
	// ... and deactivate...
	//
	tMusic_Info[ eOldState ].bActive = qfalse;
	//
	// set new track to either full volume or fade up...
	//
	tMusic_Info[eNewState].bActive				= qtrue;
	tMusic_Info[eNewState].iXFadeVolumeSeekTime	= Sys_Milliseconds();
	tMusic_Info[eNewState].iXFadeVolumeSeekTo	= 255;
	tMusic_Info[eNewState].iXFadeVolume			= bNewTrackStartsFullVolume ? 255 : 0;

	eMusic_StateActual = eNewState;

	if (s_debugdynamic->integer)
	{
		const char *psNewStateString = Music_BaseStateToString( eNewState, qtrue );
				psNewStateString = psNewStateString?psNewStateString:"<unknown>";

		Com_Printf( S_COLOR_MAGENTA "S_SwitchDynamicTracks( \"%s\" )\n", psNewStateString );
	}
}

// called by both the config-string parser and the console-command state-changer...
//
// This either changes the music right now (copying track structures etc), or leaves the new state as pending
//	so it gets picked up by the general music player if in a transition that can't be overridden...
//
static void S_SetDynamicMusicState( MusicState_e eNewState )
{
	if (eMusic_StateRequest != eNewState)
	{
		eMusic_StateRequest  = eNewState;

		if (s_debugdynamic->integer)
		{
			const char *psNewStateString = Music_BaseStateToString( eNewState, qtrue );
					psNewStateString = psNewStateString?psNewStateString:"<unknown>";

			Com_Printf( S_COLOR_MAGENTA "S_SetDynamicMusicState( Request: \"%s\" )\n", psNewStateString );
		}
	}
}

static void S_HandleDynamicMusicStateChange( void )
{
	if (eMusic_StateRequest != eMusic_StateActual)
	{
		// check whether or not the new request can be honoured, given what's currently playing...
		//
		if (Music_StateCanBeInterrupted( eMusic_StateActual, eMusic_StateRequest ))
		{
			LP_MP3STREAM pMP3StreamActual = &tMusic_Info[ eMusic_StateActual ].chMP3_Bgrnd.MP3StreamHeader;

			switch (eMusic_StateRequest)
			{
				case eBGRNDTRACK_EXPLORE:	// ... from action or silence
				{
					switch (eMusic_StateActual)
					{
						case eBGRNDTRACK_ACTION:	// action->explore
						{
							// find the transition track to play, and the entry point for explore when we get there,
							//	and also see if we're at a permitted exit point to switch at all...
							//
							float fPlayingTimeElapsed = MP3Stream_GetPlayingTimeInSeconds( pMP3StreamActual ) - MP3Stream_GetRemainingTimeInSeconds( pMP3StreamActual );

							// supply:
							//
							// playing point in float seconds
							// enum of track being queried
							//
							// get:
							//
							// enum of transition track to switch to
							// float time of entry point of new track *after* transition

							MusicState_e	eTransition;
							float			fNewTrackEntryTime = 0.0f;
							if (Music_AllowedToTransition( fPlayingTimeElapsed, eBGRNDTRACK_ACTION, &eTransition, &fNewTrackEntryTime))
							{
								S_SwitchDynamicTracks( eMusic_StateActual, eTransition, qfalse );	// qboolean bNewTrackStartsFullVolume

								tMusic_Info[eTransition].Rewind();
								tMusic_Info[eTransition].bTrackSwitchPending	= qtrue;
								tMusic_Info[eTransition].eTS_NewState			= eMusic_StateRequest;
								tMusic_Info[eTransition].fTS_NewTime			= fNewTrackEntryTime;
							}
						}
						break;

						case eBGRNDTRACK_SILENCE:	// silence->explore
						{
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume

//							float fEntryTime = Music_GetRandomEntryTime( eMusic_StateRequest );
//							tMusic_Info[ eMusic_StateRequest ].SeekTo(fEntryTime);
							tMusic_Info[ eMusic_StateRequest ].Rewind();
						}
						break;

						default:	// trying to transition from some state I wasn't aware you could transition from (shouldn't happen), so ignore
						{
							assert(0);
							S_SwitchDynamicTracks( eMusic_StateActual, eBGRNDTRACK_SILENCE, qfalse );	// qboolean bNewTrackStartsFullVolume
						}
						break;
					}
				}
				break;

				case eBGRNDTRACK_SILENCE:	// from explore or action
				{
					switch (eMusic_StateActual)
					{
						case eBGRNDTRACK_ACTION:	// action->silence
						case eBGRNDTRACK_EXPLORE:	// explore->silence
						{
							// find the transition track to play, and the entry point for explore when we get there,
							//	and also see if we're at a permitted exit point to switch at all...
							//
							float fPlayingTimeElapsed = MP3Stream_GetPlayingTimeInSeconds( pMP3StreamActual ) - MP3Stream_GetRemainingTimeInSeconds( pMP3StreamActual );

							MusicState_e	eTransition;
							float			fNewTrackEntryTime = 0.0f;
							if (Music_AllowedToTransition( fPlayingTimeElapsed, eMusic_StateActual, &eTransition, &fNewTrackEntryTime))
							{
								S_SwitchDynamicTracks( eMusic_StateActual, eTransition, qfalse );	// qboolean bNewTrackStartsFullVolume

								tMusic_Info[eTransition].Rewind();
								tMusic_Info[eTransition].bTrackSwitchPending	= qtrue;
								tMusic_Info[eTransition].eTS_NewState			= eMusic_StateRequest;
								tMusic_Info[eTransition].fTS_NewTime			= 0.0f;	//fNewTrackEntryTime;  irrelevant when switching to silence
							}
						}
						break;

						default:		// some unhandled type switching to silence
							assert(0);	// fall through since boss case just does silence->switch anyway

						case eBGRNDTRACK_BOSS:	// boss->silence
						{
							S_SwitchDynamicTracks( eMusic_StateActual, eBGRNDTRACK_SILENCE, qfalse );	// qboolean bNewTrackStartsFullVolume
						}
						break;
					}
				}
				break;

				case eBGRNDTRACK_ACTION:	// anything->action
				{
					switch (eMusic_StateActual)
					{
						case eBGRNDTRACK_SILENCE:	// silence->action
						{
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
							tMusic_Info[ eMusic_StateRequest ].Rewind();
						}
						break;

						default:	// !silence->action
						{
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qtrue );	// qboolean bNewTrackStartsFullVolume
							float fEntryTime = Music_GetRandomEntryTime( eMusic_StateRequest );
							tMusic_Info[ eMusic_StateRequest ].SeekTo(fEntryTime);
						}
						break;
					}
				}
				break;

				case eBGRNDTRACK_BOSS:
				{
					S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
					//
					// ( no need to fast forward or rewind, boss track is only entered into once, at start, and can't exit )
					//
				}
				break;

				case eBGRNDTRACK_DEATH:
				{
					S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qtrue );	// qboolean bNewTrackStartsFullVolume
					//
					// ( no need to fast forward or rewind, death track is only entered into once, at start, and can't exit or loop)
					//
				}
				break;

				default: assert(0); break;	// unknown new mode request, so just ignore it
			}
		}
	}
}

static char gsIntroMusic[MAX_QPATH]={0};
static char gsLoopMusic [MAX_QPATH]={0};

void S_RestartMusic( void )
{
	if (s_soundStarted && !s_soundMuted )
	{
		//if (gsIntroMusic[0] || gsLoopMusic[0])	// dont test this anymore (but still *use* them), they're blank for JK2 dynamic-music levels anyway
		{
			MusicState_e ePrevState	= eMusic_StateRequest;
			S_StartBackgroundTrack( gsIntroMusic, gsLoopMusic, qfalse );	// ( default music start will set the state to EXPLORE )
			S_SetDynamicMusicState( ePrevState );					// restore to prev state
		}
	}
}

// Basic logic here is to see if the intro file specified actually exists, and if so, then it's not dynamic music,
//	When called by the cgame start it loads up, then stops the playback (because of stutter issues), so that when the
//	actual snapshot is received and the real play request is processed the data has already been loaded so will be quicker.
//
// to be honest, although the code still plays WAVs some of the file-check logic only works for MP3s, so if you ever want
//	to use WAV music you'll have to do some tweaking below (but I've got other things to do so it'll have to wait - Ste)
//
void S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bCalledByCGameStart )
{
	bMusic_IsDynamic = qfalse;

	if (!s_soundStarted)
	{	//we have no sound, so don't even bother trying
		return;
	}

	if ( !intro ) {
		intro = "";
	}
	if ( !loop || !loop[0] ) {
		loop = intro;
	}

	if ( intro != gsIntroMusic ) {
		Q_strncpyz( gsIntroMusic, intro, sizeof(gsIntroMusic) );
	}
	if ( loop != gsLoopMusic ) {
		Q_strncpyz( gsLoopMusic, loop, sizeof(gsLoopMusic) );
	}

	char sNameIntro[MAX_QPATH];
	char sNameLoop [MAX_QPATH];
	Q_strncpyz(sNameIntro,	intro,	sizeof(sNameIntro));
	Q_strncpyz(sNameLoop,	loop,	sizeof(sNameLoop));

	COM_DefaultExtension( sNameIntro, sizeof( sNameIntro ), ".mp3" );
	COM_DefaultExtension( sNameLoop,  sizeof( sNameLoop),	".mp3" );

	// if dynamic music not allowed, then just stream the explore music instead of playing dynamic...
	//
	if (!s_allowDynamicMusic->integer && Music_DynamicDataAvailable(intro))	// "intro", NOT "sName" (i.e. don't use version with ".mp3" extension)
	{
		const char *psMusicName = Music_GetFileNameForState( eBGRNDTRACK_DATABEGIN );
		if (psMusicName && S_FileExists( psMusicName ))
		{
			Q_strncpyz(sNameIntro,psMusicName,sizeof(sNameIntro));
			Q_strncpyz(sNameLoop, psMusicName,sizeof(sNameLoop ));
		}
	}

	// conceptually we always play the 'intro'[/sName] track, intro-to-loop transition is handled in UpdateBackGroundTrack().
	//
	if ( (strstr(sNameIntro,"/") && S_FileExists( sNameIntro )) )	// strstr() check avoids extra file-exists check at runtime if reverting from streamed music to dynamic since literal files all need at least one slash in their name (eg "music/blah")
	{
		const char *psLoopName = S_FileExists( sNameLoop ) ? sNameLoop : sNameIntro;
		Com_DPrintf("S_StartBackgroundTrack: Found/using non-dynamic music track '%s' (loop: '%s')\n", sNameIntro, psLoopName);

        Q_strncpyz( sMusic_BackgroundLoop, psLoopName, sizeof( sMusic_BackgroundLoop ));
		S_StartBackgroundTrack_Actual( &tMusic_Info[eBGRNDTRACK_NONDYNAMIC], bMusic_IsDynamic, sNameIntro, psLoopName );
	}
	else
	{
		if (Music_DynamicDataAvailable(intro))	// "intro", NOT "sName" (i.e. don't use version with ".mp3" extension)
		{
			extern const char *Music_GetLevelSetName(void);
			Q_strncpyz(sInfoOnly_CurrentDynamicMusicSet, Music_GetLevelSetName(), sizeof(sInfoOnly_CurrentDynamicMusicSet));
			for (int i = eBGRNDTRACK_DATABEGIN; i != eBGRNDTRACK_DATAEND; i++)
			{
				qboolean bOk = qfalse;
				const char *psMusicName = Music_GetFileNameForState( (MusicState_e) i);
				if (psMusicName && (!Q_stricmp(tMusic_Info[i].sLoadedDataName, psMusicName) || S_FileExists( psMusicName )) )
				{
                    Q_strncpyz( sMusic_BackgroundLoop, loop, sizeof( sMusic_BackgroundLoop ));
					bOk = S_StartBackgroundTrack_Actual( &tMusic_Info[i], qtrue, psMusicName, loop );
				}

				tMusic_Info[i].bExists = bOk;

				if (!tMusic_Info[i].bExists)
				{
					FreeMusic( &tMusic_Info[i] );
				}
			}

			//
			// default all tracks to OFF first (and set any other vars)
			//
			for (int i=0; i<eBGRNDTRACK_NUMBEROF; i++)
			{
				tMusic_Info[i].bActive				= qfalse;
				tMusic_Info[i].bTrackSwitchPending	= qfalse;
				tMusic_Info[i].fSmoothedOutVolume	= 0.25f;
			}

			if (tMusic_Info[eBGRNDTRACK_EXPLORE].bExists &&
				tMusic_Info[eBGRNDTRACK_ACTION ].bExists
				)
			{
				Com_DPrintf("S_StartBackgroundTrack: Found dynamic music tracks\n");
				bMusic_IsDynamic = qtrue;

				//
				// ... then start the default music state...
				//
				eMusic_StateActual = eMusic_StateRequest = eBGRNDTRACK_EXPLORE;

				MusicInfo_t *pMusicInfo = &tMusic_Info[ eMusic_StateActual ];

				pMusicInfo->bActive				= qtrue;
				pMusicInfo->iXFadeVolumeSeekTime= Sys_Milliseconds();
				pMusicInfo->iXFadeVolumeSeekTo	= 255;
				pMusicInfo->iXFadeVolume		= 0;

	//#ifdef _DEBUG
	//			float fRemaining = MP3Stream_GetPlayingTimeInSeconds( &pMusicInfo->chMP3_Bgrnd.MP3StreamHeader);
	//#endif
			}
			else
			{
				Com_Printf( S_COLOR_RED "Dynamic music did not have both 'action' and 'explore' versions, inhibiting...\n");
				S_StopBackgroundTrack();
			}
		}
		else
		{
			if (sNameIntro[0]!='.')	// blank name with ".mp3" or whatever attached - no error print out
			{
				Com_Printf( S_COLOR_RED "Unable to find music \"%s\" as explicit track or dynamic music entry!\n",sNameIntro);
				S_StopBackgroundTrack();
			}
		}
	}

	if (bCalledByCGameStart)
	{
		S_StopBackgroundTrack();
	}
}

void S_StopBackgroundTrack( void )
{
	for (int i=0; i<eBGRNDTRACK_NUMBEROF; i++)
	{
		S_StopBackgroundTrack_Actual( &tMusic_Info[i] );
	}

	s_rawend = 0;
}

// qboolean return is true only if we're changing from a streamed intro to a dynamic loop...
//
static qboolean S_UpdateBackgroundTrack_Actual( MusicInfo_t *pMusicInfo, qboolean bFirstOrOnlyMusicTrack, float fDefaultVolume)
{
	int		bufferSamples;
	int		fileSamples;
	byte	raw[30000];		// just enough to fit in a mac stack frame  (note that MP3 doesn't use full size of it)
	int		fileBytes;
	int		r;

	float fMasterVol = fDefaultVolume; // s_musicVolume->value;

	if (bMusic_IsDynamic)
	{
		// step xfade volume...
		//
		if ( pMusicInfo->iXFadeVolume != pMusicInfo->iXFadeVolumeSeekTo )
		{
			int iFadeMillisecondsElapsed = Sys_Milliseconds() - pMusicInfo->iXFadeVolumeSeekTime;

			if (iFadeMillisecondsElapsed > (fDYNAMIC_XFADE_SECONDS * 1000))
			{
				pMusicInfo->iXFadeVolume = pMusicInfo->iXFadeVolumeSeekTo;
			}
			else
			{
				pMusicInfo->iXFadeVolume = (int) (255.0f * ((float)iFadeMillisecondsElapsed/(fDYNAMIC_XFADE_SECONDS * 1000.0f)));
				if (pMusicInfo->iXFadeVolumeSeekTo == 0)	// bleurgh
					pMusicInfo->iXFadeVolume = 255 - pMusicInfo->iXFadeVolume;
			}
		}
		fMasterVol *= (float)((float)pMusicInfo->iXFadeVolume / 255.0f);
	}

// this is to work around an obscure issue to do with sliding decoder windows and amounts being requested, since the
//	original MP3 stream-decoder wrapper was designed to work with audio-paintbuffer sized pieces... Basically 30000
//	is far too big for the window decoder to handle in one request because of the time-travel issue associated with
//	normal sfx buffer painting, and allowing sufficient sliding room, even though the music file never goes back in time.
//
#define SIZEOF_RAW_BUFFER_FOR_MP3 4096
#define RAWSIZE (pMusicInfo->bIsMP3?SIZEOF_RAW_BUFFER_FOR_MP3:sizeof(raw))

	if ( !pMusicInfo->s_backgroundFile ) {
		return qfalse;
	}

	pMusicInfo->fSmoothedOutVolume = (pMusicInfo->fSmoothedOutVolume + fMasterVol)/2.0f;
//	Com_OPrintf("%f\n",pMusicInfo->fSmoothedOutVolume);

	// don't bother playing anything if musicvolume is 0
	if ( pMusicInfo->fSmoothedOutVolume <= 0 ) {
		return qfalse;
	}

	// see how many samples should be copied into the raw buffer
	if ( s_rawend < s_soundtime ) {
		s_rawend = s_soundtime;
	}

	while ( s_rawend < s_soundtime + MAX_RAW_SAMPLES )
	{
		bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);

		// decide how much data needs to be read from the file
		fileSamples = bufferSamples * pMusicInfo->s_backgroundInfo.rate / dma.speed;

		// don't try to play if there are no more samples in the file
		if (!fileSamples) {
			return qfalse;
		}

		// don't try and read past the end of the file
		if ( fileSamples > pMusicInfo->s_backgroundSamples ) {
			fileSamples = pMusicInfo->s_backgroundSamples;
		}

		// our max buffer size
		fileBytes = fileSamples * (pMusicInfo->s_backgroundInfo.width * pMusicInfo->s_backgroundInfo.channels);
		if (fileBytes > (int)RAWSIZE ) {
			fileBytes = RAWSIZE;
			fileSamples = fileBytes / (pMusicInfo->s_backgroundInfo.width * pMusicInfo->s_backgroundInfo.channels);
		}

		qboolean qbForceFinish = qfalse;
		if (pMusicInfo->bIsMP3)
		{
			int iStartingSampleNum = pMusicInfo->chMP3_Bgrnd.thesfx->iSoundLengthInSamples - pMusicInfo->s_backgroundSamples;	// but this IS relevant
			//Com_Printf(S_COLOR_YELLOW "Requesting MP3 samples: sample %d\n",iStartingSampleNum);

			if (pMusicInfo->s_backgroundFile == -1)
			{
				// in-mem...
				//
				qbForceFinish = (MP3Stream_GetSamples(
                    &pMusicInfo->chMP3_Bgrnd,
                    iStartingSampleNum,
                    fileBytes/2,
                    (short*) raw,
                    qtrue)) ? qfalse : qtrue;

				//Com_Printf(S_COLOR_YELLOW "Music time remaining: %f seconds\n", MP3Stream_GetRemainingTimeInSeconds( &pMusicInfo->chMP3_Bgrnd.MP3StreamHeader ));
			}
			else
			{
				// streaming an MP3 file instead... (note that the 'fileBytes' request size isn't that relevant for MP3s,
				//										since code here can't know how much the MP3 needs to decompress)
				//
				byte *pbScrolledStreamData = MP3MusicStream_ReadFromDisk(pMusicInfo, pMusicInfo->chMP3_Bgrnd.MP3StreamHeader.iSourceReadIndex, fileBytes);

				pMusicInfo->chMP3_Bgrnd.MP3StreamHeader.pbSourceData = pbScrolledStreamData - pMusicInfo->chMP3_Bgrnd.MP3StreamHeader.iSourceReadIndex;

				qbForceFinish = (MP3Stream_GetSamples(
                    &pMusicInfo->chMP3_Bgrnd,
                    iStartingSampleNum,
                    fileBytes/2,
                    (short*) raw,
                    qtrue)) ? qfalse : qtrue;

				//Com_Printf(
                //    S_COLOR_YELLOW "Music time remaining: %f seconds\n",
                //    MP3Stream_GetRemainingTimeInSeconds(&pMusicInfo->chMP3_Bgrnd.MP3StreamHeader));
			}
		}
		else
		{
			// streaming a WAV off disk...
			//
			r = FS_Read ( raw, fileBytes, pMusicInfo->s_backgroundFile );
			if ( r != fileBytes ) {
				Com_Printf(S_COLOR_RED"StreamedRead failure on music track\n");
				S_StopBackgroundTrack();
				return qfalse;
			}

			// byte swap if needed (do NOT do for MP3 decoder, that has an internal big/little endian handler)
			//
			S_ByteSwapRawSamples( fileSamples, pMusicInfo->s_backgroundInfo.width, pMusicInfo->s_backgroundInfo.channels, raw );
		}

		// add to raw buffer
		S_RawSamples(	fileSamples, pMusicInfo->s_backgroundInfo.rate,
						pMusicInfo->s_backgroundInfo.width, pMusicInfo->s_backgroundInfo.channels, raw, pMusicInfo->fSmoothedOutVolume,
						bFirstOrOnlyMusicTrack
					);

        //Com_Printf(S_COLOR_YELLOW "bgSamples=%d fileSamples=%d forceFinish=%d rawend=%d soundtime=%d\n",
            //pMusicInfo->s_backgroundSamples, fileSamples, qbForceFinish, s_rawend, s_soundtime);

		pMusicInfo->s_backgroundSamples -= fileSamples;
		if ( !pMusicInfo->s_backgroundSamples || qbForceFinish )
		{
			// loop the music, or play the next piece if we were on the intro...
			//	(but not for dynamic, that can only be used for loop music)
			//
			if (bMusic_IsDynamic)	// needs special logic for this, different call
			{
				pMusicInfo->Rewind();
			}
			else
			{
				// for non-dynamic music we need to check if "sMusic_BackgroundLoop" is an actual filename,
				//	or if it's a dynamic music specifier (which can't literally exist), in which case it should set
				//	a return flag then exit...
				//
				char sTestName[MAX_QPATH*2];// *2 so COM_DefaultExtension doesn't do an ERR_DROP if there was no space
											//	for an extension, since this is a "soft" test
				Q_strncpyz( sTestName, sMusic_BackgroundLoop, sizeof(sTestName));
				COM_DefaultExtension(sTestName, sizeof(sTestName), ".mp3");

				if (S_FileExists( sTestName ))
				{
					S_StartBackgroundTrack_Actual( pMusicInfo, qfalse, sMusic_BackgroundLoop, sMusic_BackgroundLoop );
				}
				else
				{
					// proposed file doesn't exist, but this may be a dynamic track we're wanting to loop,
					//	so exit with a special flag...
					//
					return qtrue;
				}
			}
			if ( !pMusicInfo->s_backgroundFile )
			{
				return qfalse;		// loop failed to restart
			}
		}
	}

#undef SIZEOF_RAW_BUFFER_FOR_MP3
#undef RAWSIZE

	return qfalse;
}

// used to be just for dynamic, but now even non-dynamic music has to know whether it should be silent or not...
//
static const char *S_Music_GetRequestedState(void)
{
	/*
	int iStringOffset = cl.gameState.stringOffsets[CS_DYNAMIC_MUSIC_STATE];
	if (iStringOffset)
	{
		const char *psCommand = cl.gameState.stringData+iStringOffset;

		return psCommand;
	}
	*/
	//rwwFIXMEFIXME: Maybe use the above for something in MP?

	return NULL;
}

// scan the configstring to see if there's been a state-change requested...
// (note that even if the state doesn't change it still gets here, so do a same-state check for applying)
//
// then go on to do transition handling etc...
//
static void S_CheckDynamicMusicState(void)
{
	const char *psCommand = S_Music_GetRequestedState();

	if (psCommand)
	{
		MusicState_e eNewState;

		if ( !Q_stricmpn( psCommand, "silence", 7) )
		{
			eNewState = eBGRNDTRACK_SILENCE;
		}
		else if ( !Q_stricmpn( psCommand, "action", 6) )
		{
			eNewState = eBGRNDTRACK_ACTION;
		}
		else if ( !Q_stricmpn( psCommand, "boss", 4) )
		{
			// special case, boss music is optional and may not be defined...
			//
			if (tMusic_Info[ eBGRNDTRACK_BOSS ].bExists)
			{
				eNewState = eBGRNDTRACK_BOSS;
			}
			else
			{
				// ( leave it playing current track )
				//
				eNewState = eMusic_StateActual;
			}
		}
		else if ( !Q_stricmpn( psCommand, "death", 5) )
		{
			// special case, death music is optional and may not be defined...
			//
			if (tMusic_Info[ eBGRNDTRACK_DEATH ].bExists)
			{
				eNewState = eBGRNDTRACK_DEATH;
			}
			else
			{
				// ( leave it playing current track, typically either boss or action )
				//
				eNewState = eMusic_StateActual;
			}
		}
		else
		{
			// seems a reasonable default...
			//
			eNewState = eBGRNDTRACK_EXPLORE;
		}

		S_SetDynamicMusicState( eNewState );
	}

	S_HandleDynamicMusicStateChange();
}

void S_UpdateBackgroundTrack( void )
{
	if (bMusic_IsDynamic)
	{
		if (s_debugdynamic->integer == 2)
		{
			DynamicMusicInfoPrint();
		}

		S_CheckDynamicMusicState();

		if (eMusic_StateActual != eBGRNDTRACK_SILENCE)
		{
			MusicInfo_t *pMusicInfoCurrent = &tMusic_Info[ (eMusic_StateActual == eBGRNDTRACK_FADE)?eBGRNDTRACK_EXPLORE:eMusic_StateActual ];
			MusicInfo_t *pMusicInfoFadeOut = &tMusic_Info[ eBGRNDTRACK_FADE ];

			if ( pMusicInfoCurrent->s_backgroundFile == -1)
			{
				int iRawEnd = s_rawend;
				S_UpdateBackgroundTrack_Actual( pMusicInfoCurrent, qtrue, s_musicVolume->value );

	/*			static int iPrevFrontVol = 0;
				if (iPrevFrontVol != pMusicInfoCurrent->iXFadeVolume)
				{
					iPrevFrontVol  = pMusicInfoCurrent->iXFadeVolume;
					Com_Printf("front vol = %d\n",pMusicInfoCurrent->iXFadeVolume);
				}
	*/
				if (pMusicInfoFadeOut->bActive)
				{
					s_rawend = iRawEnd;
					S_UpdateBackgroundTrack_Actual( pMusicInfoFadeOut, qfalse, s_musicVolume->value );	// inactive-checked internally
	/*
					static int iPrevFadeVol = 0;
					if (iPrevFadeVol != pMusicInfoFadeOut->iXFadeVolume)
					{
						iPrevFadeVol  = pMusicInfoFadeOut->iXFadeVolume;
						Com_Printf("fade vol = %d\n",pMusicInfoFadeOut->iXFadeVolume);
					}
	*/
					//
					// only do this for the fader!...
					//
					if (pMusicInfoFadeOut->iXFadeVolume == 0)
					{
						pMusicInfoFadeOut->bActive = qfalse;
					}
				}

				float fRemainingTimeInSeconds = MP3Stream_GetRemainingTimeInSeconds( &pMusicInfoCurrent->chMP3_Bgrnd.MP3StreamHeader );
				// Com_Printf("Remaining: %3.3f\n",fRemainingTimeInSeconds);

				if ( fRemainingTimeInSeconds < fDYNAMIC_XFADE_SECONDS*2 )
				{
					// now either loop current track, switch if finishing a transition, or stop if finished a death...
					//
					if (pMusicInfoCurrent->bTrackSwitchPending)
					{
						pMusicInfoCurrent->bTrackSwitchPending = qfalse;	// ack
						S_SwitchDynamicTracks( eMusic_StateActual, pMusicInfoCurrent->eTS_NewState, qfalse);	// qboolean bNewTrackStartsFullVolume
						if (tMusic_Info[ pMusicInfoCurrent->eTS_NewState ].bExists)	// don't do this if switching to silence
						{
							tMusic_Info[ pMusicInfoCurrent->eTS_NewState ].SeekTo(pMusicInfoCurrent->fTS_NewTime);
						}
					}
					else
					{
						// normal looping, so set rewind current track, set volume to 0 and fade up to full (unless death track playing, then stays quiet)
						//	(while fader copy of end-section fades down)
						//
						// copy current track to fader...
						//
						*pMusicInfoFadeOut = *pMusicInfoCurrent;	// struct copy
						pMusicInfoFadeOut->iXFadeVolumeSeekTime	= Sys_Milliseconds();
						pMusicInfoFadeOut->iXFadeVolumeSeekTo	= 0;
						//
						pMusicInfoCurrent->Rewind();
						pMusicInfoCurrent->iXFadeVolumeSeekTime	= Sys_Milliseconds();
						pMusicInfoCurrent->iXFadeVolumeSeekTo	= (eMusic_StateActual == eBGRNDTRACK_DEATH) ? 0: 255;
						pMusicInfoCurrent->iXFadeVolume			= 0;
					}
				}
			}
		}
		else
		{
			// special case, when foreground music is shut off but fader still running to fade off previous track...
			//
			MusicInfo_t *pMusicInfoFadeOut = &tMusic_Info[ eBGRNDTRACK_FADE ];
			if (pMusicInfoFadeOut->bActive)
			{
				S_UpdateBackgroundTrack_Actual( pMusicInfoFadeOut, qtrue, s_musicVolume->value );
				if (pMusicInfoFadeOut->iXFadeVolume == 0)
				{
					pMusicInfoFadeOut->bActive = qfalse;
				}
			}
		}
	}
	else
	{
		// standard / non-dynamic one-track music...
		const char *psCommand = S_Music_GetRequestedState();	// special check just for "silence" case...
		qboolean bShouldBeSilent = (qboolean)(psCommand && !Q_stricmp(psCommand,"silence"));
		float fDesiredVolume = bShouldBeSilent ? 0.0f : s_musicVolume->value;
		//
		// internal to this code is a volume-smoother...
		//
		qboolean bNewTrackDesired = S_UpdateBackgroundTrack_Actual(&tMusic_Info[eBGRNDTRACK_NONDYNAMIC], qtrue, fDesiredVolume);

		if (bNewTrackDesired)
		{
			S_StartBackgroundTrack( sMusic_BackgroundLoop, sMusic_BackgroundLoop, qfalse );
		}
	}
}

cvar_t *s_soundpoolmegs = NULL;

// currently passing in sfx as a param in case I want to do something with it later.
//
byte *SND_malloc(int iSize, sfx_t *sfx)
{
	byte *pData = (byte *) Z_Malloc(iSize, TAG_SND_RAWDATA, qfalse);	// don't bother asking for zeroed mem

	// if "s_soundpoolmegs" is < 0, then the -ve of the value is the maximum amount of sounds we're allowed to have loaded...
	//
	if (s_soundpoolmegs && s_soundpoolmegs->integer < 0)
	{
		while ( (Z_MemSize(TAG_SND_RAWDATA) + Z_MemSize(TAG_SND_MP3STREAMHDR)) > ((-s_soundpoolmegs->integer) * 1024 * 1024))
		{
			int iBytesFreed = SND_FreeOldestSound(sfx);
			if (iBytesFreed == 0)
				break;	// sanity
		}
	}

	return pData;
}

// called once-only in EXE lifetime...
//
void SND_setup()
{
	s_soundpoolmegs = Cvar_Get("s_soundpoolmegs", "25", CVAR_ARCHIVE);
	if (Sys_LowPhysicalMemory() )
	{
		Cvar_Set("s_soundpoolmegs", "0");
	}

	Com_Printf("Sound memory manager started\n");
}

// ask how much mem an sfx has allocated...
//
static int SND_MemUsed(sfx_t *sfx)
{
	int iSize = 0;
	if (sfx->pSoundData){
		iSize += Z_Size(sfx->pSoundData);
	}

	if (sfx->pMP3StreamHeader) {
		iSize += Z_Size(sfx->pMP3StreamHeader);
	}

	return iSize;
}

// free any allocated sfx mem...
//
// now returns # bytes freed to help with z_malloc()-fail recovery
//
static int SND_FreeSFXMem(sfx_t *sfx)
{
	int iBytesFreed = 0;

    iBytesFreed += S_AL_OnFreeSfxMemory(sfx);

	if (						sfx->pSoundData) {
		iBytesFreed +=	Z_Size(	sfx->pSoundData);
						Z_Free(	sfx->pSoundData );
								sfx->pSoundData = NULL;
	}

	sfx->bInMemory = qfalse;

	if (						sfx->pMP3StreamHeader) {
		iBytesFreed +=	Z_Size(	sfx->pMP3StreamHeader);
						Z_Free(	sfx->pMP3StreamHeader );
								sfx->pMP3StreamHeader = NULL;
	}

	return iBytesFreed;
}

void S_DisplayFreeMemory()
{
	int iSoundDataSize = Z_MemSize ( TAG_SND_RAWDATA ) + Z_MemSize( TAG_SND_MP3STREAMHDR );
	int iMusicDataSize = Z_MemSize ( TAG_SND_DYNAMICMUSIC );

	if (iSoundDataSize || iMusicDataSize)
	{
		Com_Printf("\n%.2fMB audio data:  ( %.2fMB WAV/MP3 ) + ( %.2fMB Music )\n",
					((float)(iSoundDataSize+iMusicDataSize))/1024.0f/1024.0f,
										((float)(iSoundDataSize))/1024.0f/1024.0f,
																((float)(iMusicDataSize))/1024.0f/1024.0f
					);

		// now count up amount used on this level...
		//
		iSoundDataSize = 0;
		for (int i=1; i<s_numSfx; i++)
		{
			sfx_t *sfx = &s_knownSfx[i];

			if (sfx->iLastLevelUsedOn == re->RegisterMedia_GetLevel()){
				iSoundDataSize += SND_MemUsed(sfx);
			}
		}

		Com_Printf("%.2fMB in sfx_t alloc data (WAV/MP3) loaded this level\n",(float)iSoundDataSize/1024.0f/1024.0f);
	}
}

void SND_TouchSFX(sfx_t *sfx)
{
	sfx->iLastTimeUsed		= Com_Milliseconds()+1;
	sfx->iLastLevelUsedOn	= re->RegisterMedia_GetLevel();
}

// currently this is only called during snd_shutdown or snd_restart
//
void S_FreeAllSFXMem(void)
{
	for (int i=1 ; i < s_numSfx ; i++)	// start @ 1 to skip freeing default sound
	{
		SND_FreeSFXMem(&s_knownSfx[i]);
	}
}

// returns number of bytes freed up...
//
// new param is so we can be usre of not freeing ourselves (without having to rely on possible uninitialised timers etc)
//
int SND_FreeOldestSound(sfx_t *pButNotThisOne /* = NULL */)
{
	int iBytesFreed = 0;
	sfx_t *sfx;

	int	iOldest = Com_Milliseconds();
	int	iUsed	= 0;

	// start on 1 so we never dump the default sound...
	//
	for (int i=1 ; i < s_numSfx ; i++)
	{
		sfx = &s_knownSfx[i];

		if (sfx != pButNotThisOne)
		{
			if (!sfx->bDefaultSound && sfx->bInMemory && sfx->iLastTimeUsed < iOldest)
			{
				// new bit, we can't throw away any sfx_t struct in use by a channel, else the paint code will crash...
				//
				int iChannel;
				for (iChannel=0; iChannel<MAX_CHANNELS; iChannel++)
				{
					channel_t *ch = & s_channels[iChannel];

					if (ch->thesfx == sfx)
						break;	// damn, being used
				}
				if (iChannel == MAX_CHANNELS)
				{
					// this sfx_t struct wasn't used by any channels, so we can lose it...
					//
					iUsed = i;
					iOldest = sfx->iLastTimeUsed;
				}
			}
		}
	}

	if (iUsed)
	{
		sfx = &s_knownSfx[ iUsed ];

		Com_DPrintf("SND_FreeOldestSound: freeing sound %s\n", sfx->sSoundName);

		iBytesFreed = SND_FreeSFXMem(sfx);
	}

	return iBytesFreed;
}
int SND_FreeOldestSound(void)
{
	return SND_FreeOldestSound(NULL);	// I had to add a void-arg version of this because of link issues, sigh
}

// just before we drop into a level, ensure the audio pool is under whatever the maximum
//	pool size is (but not by dropping out sounds used by the current level)...
//
// returns qtrue if at least one sound was dropped out, so z_malloc-fail recovery code knows if anything changed
//
extern qboolean gbInsideLoadSound;
qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* 99% qfalse */)
{
	qboolean bAtLeastOneSoundDropped = qfalse;

	Com_DPrintf( "SND_RegisterAudio_LevelLoadEnd():\n");

	if (gbInsideLoadSound)
	{
		Com_DPrintf( "(Inside S_LoadSound (z_malloc recovery?), exiting...\n");
	}
	else
	{
		int iLoadedAudioBytes	 = Z_MemSize ( TAG_SND_RAWDATA ) + Z_MemSize( TAG_SND_MP3STREAMHDR );
		const int iMaxAudioBytes = s_soundpoolmegs->integer * 1024 * 1024;

		for (int i=1; i<s_numSfx && ( iLoadedAudioBytes > iMaxAudioBytes || bDeleteEverythingNotUsedThisLevel) ; i++) // i=1 so we never page out default sound
		{
			sfx_t *sfx = &s_knownSfx[i];

			if (sfx->bInMemory)
			{
				qboolean bDeleteThis = qfalse;

				if (bDeleteEverythingNotUsedThisLevel)
				{
					bDeleteThis = (qboolean)(sfx->iLastLevelUsedOn != re->RegisterMedia_GetLevel());
				}
				else
				{
					bDeleteThis = (qboolean)(sfx->iLastLevelUsedOn < re->RegisterMedia_GetLevel());
				}

				if (bDeleteThis)
				{
					Com_DPrintf( "Dumping sfx_t \"%s\"\n",sfx->sSoundName);

					if (SND_FreeSFXMem(sfx))
					{
						bAtLeastOneSoundDropped = qtrue;
					}

					iLoadedAudioBytes = Z_MemSize ( TAG_SND_RAWDATA ) + Z_MemSize( TAG_SND_MP3STREAMHDR );
				}
			}
		}
	}

	Com_DPrintf( "SND_RegisterAudio_LevelLoadEnd(): Ok\n");

	return bAtLeastOneSoundDropped;
}