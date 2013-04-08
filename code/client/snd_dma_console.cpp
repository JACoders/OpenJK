/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 *
 *****************************************************************************/
// leave this as first line for PCH reasons...
//
// #include "../server/exe_headers.h"

#include "snd_local_console.h"
#include "snd_music.h"

// #include "../../toolbox/zlib/zlib.h"

#include "../client/client.h"
#include "../qcommon/fixedmap.h"

#ifdef _XBOX
#include <Xtl.h>
#endif

#ifdef _GAMECUBE
typedef const char* LPCSTR;
#endif

// Maps CRCs to offsets
struct LipFileInfo
{
	unsigned long crc;
	unsigned long offset;
};
static VVFixedMap< unsigned int, unsigned int >* s_lipSyncMap = NULL;
static char *s_lipSyncData = NULL;

static void S_Play_f(void);
#ifndef _JK2MP
static void S_PlayEx_f(void);
#endif
static void S_SoundList_f(void);
static void S_Music_f(void);

void S_Update_();
void S_StopAllSounds(void);
static void S_UpdateBackgroundTrack( void );
unsigned int S_HashName( const char *name );
static int SND_FreeSFXMem(sfx_t *sfx);

/*static void S_FreeAllSFXMem(void);
static void S_UnCacheDynamicMusic( void );
*/
extern unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned long len);

extern int Sys_GetFileCodeSize(int code);

extern void Sys_StreamInit(void);
extern void Sys_StreamShutdown(void);

qboolean SND_RegisterAudio_Clean(void);
void S_KillEntityChannel(int entnum, int chan);

//////////////////////////
//
// vars for bgrnd music track...
//
typedef struct
{	
	//
	// disk-load stuff
	//
	char		sLoadedDataName[MAX_QPATH];
	int			iFileCode;
	int			iFileSeekTo;
	bool		bLoaded;
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
	qboolean		bTrackSwitchPending;
	qboolean		bLooping;
	MusicState_e	eTS_NewState;
	float			fTS_NewTime;
	//
	// Generic...
	//
	int				s_backgroundSize;
	int				s_backgroundBPS;

	void Rewind()
	{
		iFileSeekTo = 0;
	}

	void SeekTo(float fTime)
	{
		iFileSeekTo = (int)((float)(s_backgroundBPS) * fTime);
	}

	float TotalTime(void)
	{
		return (float)(s_backgroundSize) / (float)(s_backgroundBPS);
	}

	float PlayTime(void)
	{
		ALfloat playTime;
		alGetStreamf(AL_TIME, &playTime);
		return playTime;
	}

	float ElapsedTime(void)
	{
		return fmod(PlayTime(), TotalTime());
	}
} MusicInfo_t;

static void S_SetDynamicMusicState( MusicState_e musicState );

#define fDYNAMIC_XFADE_SECONDS (1.f)

static MusicInfo_t	tMusic_Info[eBGRNDTRACK_NUMBEROF]	= {0};
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

// This number has dramatic affects on volume.  QA has
// determined that 100 is too quiet in some spots and
// 200 is too loud in others.  Modify with care...
#define		SOUND_REF_DIST_BASE	150.f

#define		SOUND_UPDATE_TIME	100

const float	SOUND_FMAXVOL=0.75;//1.0;
const int	SOUND_MAXVOL=255;

int					s_soundStarted;
qboolean			s_soundMuted;
int					s_loopEnabled;
int					s_updateTime;

struct listener_t
{
	ALuint handle;
	ALfloat pos[3];
	ALfloat orient[6];
	int entnum;
};

#define SND_MAX_LISTENERS 1
static listener_t s_listeners[SND_MAX_LISTENERS];
static int s_numListeners;

static int			s_numChannels;			// Number of AL Sources == Num of Channels

#ifdef _XBOX
#	define	MAX_CHANNELS_2D 64
#	define	MAX_CHANNELS_3D 64
#else
#	define	MAX_CHANNELS_2D 30
#	define	MAX_CHANNELS_3D 30
#endif

#define MAX_CHANNELS (MAX_CHANNELS_2D + MAX_CHANNELS_3D)
static channel_t*   s_channels;

#define	MAX_SFX 2048
#define INVALID_CODE 0
static sfx_t* s_sfxBlock;
static int* s_sfxCodes;

static bool s_registered = false;
static int s_defaultSound = 0;

typedef struct 
{ 
	int				volume;
	vec3_t			origin;
	sfx_t			*sfx;
	int				entnum;
	int				entchannel;
	bool			bProcessed;
	bool			bMarked;
} loopSound_t;

#define	MAX_LOOP_SOUNDS 32
static int numLoopSounds;
static loopSound_t* loopSounds;

int* s_entityWavVol = NULL;

cvar_t		*s_effects_volume;
cvar_t		*s_music_volume;
cvar_t		*s_voice_volume;
cvar_t		*s_testsound;
cvar_t		*s_allowDynamicMusic;
cvar_t		*s_show;
cvar_t		*s_separation;
cvar_t		*s_debugdynamic;
cvar_t		*s_soundpoolmegs;
cvar_t		*s_language;	// note that this is distinct from "g_language"



void S_SoundInfo_f(void) {	
	Com_Printf("----- Sound Info -----\n" );

	if (!s_soundStarted) {
		Com_Printf ("sound system not started\n");
	} else {
		if ( s_soundMuted ) {
			Com_Printf ("sound system is muted\n");
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
	ALCcontext *ALCContext = NULL;
	ALCdevice *ALCDevice = NULL;
	cvar_t	*cv;

	Com_Printf("\n------- sound initialization -------\n");

	AS_Init();

	s_effects_volume = Cvar_Get ("s_effects_volume", "0.5", CVAR_ARCHIVE);
	s_voice_volume= Cvar_Get ("s_voice_volume", "1.0", CVAR_ARCHIVE);
	s_music_volume = Cvar_Get ("s_music_volume", "0.25", CVAR_ARCHIVE);
	s_separation = Cvar_Get ("s_separation", "0.5", CVAR_ARCHIVE);
	s_allowDynamicMusic = Cvar_Get ("s_allowDynamicMusic", "1", CVAR_ARCHIVE);

	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);
	s_debugdynamic = Cvar_Get("s_debugdynamic","0", CVAR_CHEAT);

	s_soundpoolmegs = Cvar_Get("s_soundpoolmegs", "6", CVAR_ARCHIVE);

	s_language = Cvar_Get("s_language","english",CVAR_ARCHIVE | CVAR_NORESTART);

	cv = Cvar_Get ("s_initsound", "1", CVAR_ROM);
	if ( !cv->integer ) {
		s_soundStarted = 0;	// needed in case you set s_initsound to 0 midgame then snd_restart (div0 err otherwise later)
		Com_Printf ("not initializing.\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f);
#ifndef _JK2MP
	Cmd_AddCommand("playex", S_PlayEx_f);
#endif
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("soundlist", S_SoundList_f);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);
	Cmd_AddCommand("soundstop", S_StopAllSounds);

	s_entityWavVol = new int[MAX_GENTITIES];
	
	// clear out the lip synching override array
	memset(s_entityWavVol, 0, sizeof(int) * MAX_GENTITIES);

	ALCDevice = alcOpenDevice((ALubyte*)"DirectSound3D");
	if (!ALCDevice)
		return;

	//Create context(s)
	ALCContext = alcCreateContext(ALCDevice, NULL);
	if (!ALCContext)
		return;

	//Set active context
	alcMakeContextCurrent(ALCContext);		
	if (alcGetError(ALCDevice) != ALC_NO_ERROR)
		return;

	s_channels = new channel_t[MAX_CHANNELS];
	
	s_sfxBlock = new sfx_t[MAX_SFX];
	s_sfxCodes = new int[MAX_SFX];
	memset(s_sfxCodes, INVALID_CODE, sizeof(int) * MAX_SFX);

	loopSounds = new loopSound_t[MAX_LOOP_SOUNDS];

	S_StopAllSounds();

	s_soundStarted = 1;
	s_soundMuted = 1;
	s_loopEnabled = 0;
	s_updateTime = 0;

	S_SoundInfo_f();

	memset(s_channels, 0, sizeof(channel_t) * MAX_CHANNELS);
	s_numChannels = 0;
	
	// create music channel
	alGenStream();

	Com_Printf("------------------------------------\n");

	S_InitLoad();

	// Load all the lipsync index data first:
	void *buffer;
	int len = FS_ReadFile("lipdata.idx", &buffer);
	if( len == -1 )
		Com_Error(ERR_DROP, "ERROR: No lip sync index file\n");
	int numLipFiles = len / sizeof(LipFileInfo);
	LipFileInfo *lbuf = (LipFileInfo *)buffer;
	s_lipSyncMap = new VVFixedMap< unsigned int, unsigned int >(numLipFiles);
	for( int i = 0; i < numLipFiles; ++i )
		s_lipSyncMap->Insert(lbuf[i].offset, lbuf[i].crc);
	FS_FreeFile(buffer);

	// Now load the actual lip sync data
	len = FS_ReadFile("lipdata.dat", &buffer);
	if( len == -1 )
		Com_Error(ERR_DROP, "ERROR: No lip sync data file\n");
	s_lipSyncData = new char[len];
	memcpy(s_lipSyncData, buffer, len);
	FS_FreeFile(buffer);
}

// only called from snd_restart. QA request...
//
void S_ReloadAllUsedSounds(void)
{
	if (s_soundStarted && !s_soundMuted )
	{
		// new bit, reload all soundsthat are used on the current level->..
		//
		for (int i = 0; i < MAX_SFX; ++i)
		{
			if (s_sfxCodes[i] == INVALID_CODE || s_sfxCodes[i] == s_defaultSound) continue;

			sfx_t *sfx = &s_sfxBlock[i];

			if ((sfx->iFlags & SFX_FLAG_UNLOADED) && 
				!(sfx->iFlags & (SFX_FLAG_DEFAULT | SFX_FLAG_DEMAND)))
			{
				S_StartLoadSound(sfx);
			}
		}
	}
}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown( void )
{
	ALCcontext	*ALCContext;
	ALCdevice	*ALCDevice;
	int			i;

	delete [] s_entityWavVol;
	s_entityWavVol = NULL;

	if ( !s_soundStarted ) {
		return;
	}

	alDeleteStream();

	// Release all the AL Sources (including Music channel (Source 0))
	for (i = 0; i < s_numChannels; i++)
	{
		alDeleteSources(1, &(s_channels[i].alSource));
	}

	S_FreeAllSFXMem();
	S_UnCacheDynamicMusic();
	
	// Release listeners
	for (i = 0; i < s_numListeners; ++i)
	{
		alDeleteListeners(1, &s_listeners[i].handle);
	}
	s_numListeners = 0;
	
	delete [] s_channels;
	delete [] s_sfxBlock;
	delete [] s_sfxCodes;
	delete [] loopSounds;

	// Get active context
	ALCContext = alcGetCurrentContext();
	// Get device for active context
	ALCDevice = alcGetContextsDevice(ALCContext);
	// Release context(s)
	alcDestroyContext(ALCContext);
	// Close device
	alcCloseDevice(ALCDevice);
	
	s_numChannels = 0;
	s_soundStarted = 0;

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");
	AS_Free();
	S_CloseLoad();
}



/*
	Mutes / Unmutes all OpenAL sound
*/
void S_AL_MuteAllSounds(qboolean bMute)
{
	if (!s_soundStarted) return;
	if (bMute) alGain(0.f);
	else alGain(1.f);
}

void S_SetVolume(float volume)
{
	if (!s_soundStarted) return;
	alGain(volume);
	alUpdate();
}





// =======================================================================
// Load a sound
// =======================================================================
/*
==================
S_FixMusicFileExtension
==================
*/
char* S_FixMusicFileName(const char* name)
{
	static char xname[MAX_QPATH];

#if defined(_XBOX)
	const char* ext = "wxb";
#elif defined(_WINDOWS)
	const char* ext = "wav";
#elif defined(_GAMECUBE)
	const char* ext = "adp";
#endif

	Q_strncpyz(xname, name, sizeof(xname));
	if (xname[strlen(xname) - 4] != '.')
	{
		strcat(xname, ".");
		strcat(xname, ext);
	}
	else
	{
		int len = strlen(xname);
		xname[len-3] = ext[0];
		xname[len-2] = ext[1];
		xname[len-1] = ext[2];
	}

#ifdef _GAMECUBE
	if (!strncmp("music/", xname, 6) ||
		!strncmp("music\\", xname, 6))
	{
		char chan_name[MAX_QPATH];

		/*
		ALint is_stereo;
		alGeti(AL_STEREO, &is_stereo);

		sprintf(chan_name,"music-%s/%s", 
			is_stereo ? "stereo" : "mono", &xname[6]);
		strcpy(xname, chan_name);
		*/

		sprintf(chan_name,"music-stereo/%s", &xname[6]);
		strcpy(xname, chan_name);
	}
#endif
	
	return xname;
}


/*
==================
S_HashName
==================
*/
unsigned int S_HashName( const char *name ) {
	if (!name) {
		Com_Error (ERR_FATAL, "S_HashName: NULL\n");
	}
	if (!name[0]) {
		Com_Error (ERR_FATAL, "S_HashName: empty name\n");
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);
	}

	char sSoundNameNoExt[MAX_QPATH];
	COM_StripExtension(name,sSoundNameNoExt);

	Q_strlwr(sSoundNameNoExt);
	for (int i = 0; i < strlen(sSoundNameNoExt); ++i)
	{
		if (sSoundNameNoExt[i] == '\\') sSoundNameNoExt[i] = '/';
	}

	return crc32(0, (const byte *)sSoundNameNoExt, strlen(sSoundNameNoExt));
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
	if (!s_soundStarted) return;
	S_StopAllSounds();
	SND_RegisterAudio_Clean(); // unregister sounds
	s_soundMuted = qtrue;
}

void S_SetLoopState( qboolean s ) {
	if (!s_soundStarted) return;
	s_loopEnabled = s;
}

void S_CreateSources( void ) {
	int i;

	// Remove any old sources
	for (i = 0; i < s_numChannels; ++i)
	{
		alDeleteSources(1, &s_channels[i].alSource);
	}
	s_numChannels = 0;

	// Create as many AL Sources (up to Max) as possible
	int limit = MAX_CHANNELS_2D + MAX_CHANNELS_3D / s_numListeners;
	for (i = 0; i < limit; i++)
	{
		if (i < MAX_CHANNELS_2D)
		{
			alGenSources2D(1, &s_channels[i].alSource);
			s_channels[i].b2D = true;
		}
		else
		{
			alGenSources3D(1, &s_channels[i].alSource);
			s_channels[i].b2D = false;
		}

		if (alGetError() != AL_NO_ERROR)
		{
			// Reached limit of sources
			break;
		}

		if (!s_channels[i].b2D)
		{
			alSourcef(s_channels[i].alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE);
		}

		s_numChannels++;
	}

	assert(s_numChannels > MAX_CHANNELS_2D);
}

/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration( void )
{
	if (!s_soundStarted) return;

	int i;
	int num_listeners = 1;

	// Turn sound back on.
	s_soundMuted = qfalse;

	// Create listeners
	assert(num_listeners <= SND_MAX_LISTENERS);
	if (num_listeners < s_numListeners)
	{
		// remove some listeners
		for (i = num_listeners; i < s_numListeners; ++i)
		{
			alDeleteListeners(1, &s_listeners[i].handle);
		}

		s_numListeners = num_listeners;
	
		S_CreateSources();
	}
	else if (num_listeners > s_numListeners)
	{
		// add some listeners
		for (i = s_numListeners; i < num_listeners; ++i)
		{
			memset(&s_listeners[i], 0, sizeof(listener_t));
			s_listeners[i].entnum = i;
			s_listeners[i].orient[2] = -1;
			s_listeners[i].orient[4] = 1;
			alGenListeners(1, &s_listeners[i].handle);
			alListenerfv(s_listeners[i].handle, AL_POSITION, s_listeners[i].pos);
			alListenerfv(s_listeners[i].handle, AL_ORIENTATION, s_listeners[i].orient);
		}
		
		s_numListeners = num_listeners;
		
		S_CreateSources();
	}

	S_SetLoopState(qtrue);

	if (!s_registered) {
		s_defaultSound = S_RegisterSound("sound/null.wav");
		S_LoadSound(s_defaultSound);
		s_registered = true;
	}
}

/*
==================
S_LookupSfx
==================
*/
sfxHandle_t S_LookupSfx(int hash) 
{
	for (int i = 0; i < MAX_SFX; ++i)
	{
		if (s_sfxCodes[i] == hash)
		{
			return i;
		}
	}
	return -1;
}

/*
==================
S_AllocSfx
==================
*/
sfxHandle_t S_AllocSfx(int hash) 
{
	for (int i = 0; i < MAX_SFX; ++i)
	{
		if (s_sfxCodes[i] == INVALID_CODE)
		{
			s_sfxCodes[i] = hash;
			return i;
		}
	}
	return -1;
}

extern void	COM_StripExtension( const char *in, char *out );
extern char *FS_BuildOSPath( const char *qpath );

// Convert pathname to filecode
int Lip_GetFileCode(const char* name)
{
	// Get system level path
	char* osname = FS_BuildOSPath(name);

	// Generate hash for file name
	strlwr(osname);
	unsigned int code = crc32(0, (const unsigned char *)osname, strlen(osname));

	return code;
}

static void	S_LoadLips(sfx_t* thesfx, const char* name)
{
	// Sanity checks
	if( !s_lipSyncData || !s_lipSyncMap )
		return;

	// get the lipfile name -> turn it into a crc
	char lipfile[MAX_QPATH];
	COM_StripExtension(name,lipfile);
	strcat(lipfile,".lip");
	unsigned int code = Lip_GetFileCode( lipfile );

	// Lookup in the fixed map
	unsigned int *pOffset = s_lipSyncMap->Find(code);
	if( !pOffset )
		return;

	// OK. Set the pointer to the right data
	thesfx->pLipSyncData = s_lipSyncData + *pOffset;
}



/*
==================
S_RegisterSound

Creates a default buzz sound if the file can't be loaded
==================
*/
sfxHandle_t	S_RegisterSound(const char *name)
{
	sfx_t *sfx;
	unsigned int hash;
	sfxHandle_t handle;

	if (!s_soundStarted) {
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH || !name[0] ) {		
		Com_Printf( S_COLOR_RED"Sound name exceeds MAX_QPATH - %s\n", name );
		return s_defaultSound;
	}

	/* Temporary fix for levels that try to precache music and play them as sfx */
	if (strstr(name, "MUSIC"))
	{
		Com_Printf( "WARNING: Trying to play music file %s through S_StartSound!\n", name );
		return s_defaultSound;
	}

	hash = S_HashName( name );
	handle = S_LookupSfx(hash);
	
	if (handle < 0)
	{
		handle = S_AllocSfx(hash);

		if (handle < 0)
			Com_Error (ERR_DROP, "No free sound channels");

		sfx = &s_sfxBlock[handle];
		memset(sfx, 0, sizeof(sfx_t));

		if (strlen(name) < 5 || name[0] == '*') sfx->iFileCode = -1;
		else sfx->iFileCode = S_GetFileCode(name);

		sfx->iFlags |= SFX_FLAG_UNLOADED;
	}
	else
	{
		sfx = &s_sfxBlock[handle];
	}

	SND_TouchSFX(sfx);

	if ( sfx->iFileCode == -1 ) sfx->iFlags |= SFX_FLAG_DEFAULT;

	sfx->pLipSyncData = NULL;

	if (strstr(name, "chars") ||
		strstr(name, "chr_d") ||
		strstr(name, "chr_f") ||
		strstr(name, "CHARS"))
	{
		sfx->iFlags |= SFX_FLAG_VOICE;
		sfx->iFlags |= SFX_FLAG_DEMAND;

		// load up the lip sync data
		S_LoadLips(sfx, name);
	}

	if ( sfx->iFlags & SFX_FLAG_DEFAULT )
	{
		sfx->iFlags |= SFX_FLAG_RESIDENT;
		return s_defaultSound;
	}

	//can be uncommented for debugging if soundname is used
	//also uncomment sSoundName from sfx_t
	//sfx->sSoundName = CopyString(name);

	return handle;
}


//=============================================================================
channel_t *S_FindFurthestChannel(void)
{
	int			ch_idx;
	channel_t	*ch;
	channel_t	*ch_firstToDie = NULL;
	int			li_idx;
	listener_t	*li;
	int			longestDist = -1;
	int			dist;

	for (li_idx = 0, li = s_listeners; li_idx < s_numListeners; ++li_idx, ++li)
	{
		for (ch_idx = MAX_CHANNELS_2D, ch = s_channels + ch_idx; 
		ch_idx < s_numChannels; ch_idx++, ch++)
		{				
			dist = 
				((li->pos[0] - ch->origin[0]) * (li->pos[0] - ch->origin[0])) +
				((li->pos[1] - ch->origin[1]) * (li->pos[1] - ch->origin[1])) +
				((li->pos[2] - ch->origin[2]) * (li->pos[2] - ch->origin[2]));
			
			if (dist > longestDist)
			{
				longestDist = dist;
				ch_firstToDie = ch;
			}
		}
	}

	return ch_firstToDie;
}

static bool IsListenerEnt(int entnum)
{
	for (int i = 0; i < s_numListeners; ++i)
	{
		if (s_listeners[i].entnum == entnum) return true;
	}
	return false;
}

/*
=================
S_PickChannel
=================
*/
channel_t *S_PickChannel(int entnum, int entchannel, bool is2D, sfx_t* sfx)
{
	int			ch_idx;
	channel_t	*ch, *ch_firstToDie;
	bool	foundChan = false;

	if ( entchannel < 0 ) 
	{
		Com_Error (ERR_DROP, "S_PickChannel: entchannel<0");
	}

	// Check for replacement sound, or find the best one to replace

    ch_firstToDie = s_channels;
	unsigned int age = 0xFFFFFFFF;

	for (ch_idx = 0, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
	{
		// Special check to prevent 2d voices from being played
		// twice in 2 player games...
		if (is2D && ch->b2D && 
			sfx == ch->thesfx && 
			ch->bPlaying && 
			(sfx->iFlags & SFX_FLAG_VOICE))
		{
			return NULL;
		}
		
		// See if the channel is free
		if (!ch->thesfx && is2D == ch->b2D && ch->iLastPlayTime < age)
		{
			ch_firstToDie = ch;
			age = ch->iLastPlayTime;
			foundChan = true;
		}
	}

	if (!foundChan)
	{
		for (ch_idx = 0, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
		{
			if ( (ch->entnum == entnum) && 
				(ch->entchannel == entchannel) && 
				(ch->entchannel != CHAN_AMBIENT) && 
				(!IsListenerEnt(ch->entnum)) &&
				(ch->b2D == is2D) &&
				(!ch_firstToDie->thesfx || 
				!(ch_firstToDie->thesfx->iFlags & SFX_FLAG_LOADING)) ) 
			{
				// Same entity and same type of sound effect (entchannel)
				ch_firstToDie = ch;
				foundChan = true;
				break;
			}
		}
	}

	if (!foundChan)
	{
		if (is2D)
		{
			// Find random sound effect
			ch_firstToDie = s_channels + (rand() % MAX_CHANNELS_2D);
		}
		else
		{
			// Find sound effect furthest from listeners
			ch_firstToDie = S_FindFurthestChannel();
		}
	}

	assert(ch_firstToDie->b2D == is2D);

	if (ch_firstToDie->thesfx && ch_firstToDie->thesfx->iFlags & SFX_FLAG_LOADING)
	{
		// If the sound is loading, just give up...
		return NULL;
	}
	
	if (ch_firstToDie->bPlaying)
	{
#ifdef _XBOX
		// We have an insane amount of channels on the Xbox
		// and stopping one is a blocking operation.  Let's
		// just assume that no one will care if a sound is
		// dropped when over 100 are already playing...
		return NULL;
#else
		// Stop sound
		alSourceStop(ch_firstToDie->alSource);
		ch_firstToDie->bPlaying = false;
#endif
	}

	// Reset channel variables
	alSourcei(ch_firstToDie->alSource, AL_BUFFER, 0);
	ch_firstToDie->thesfx = NULL;
	ch_firstToDie->bLooping = false;
	
    return ch_firstToDie;
}



// =======================================================================
// Start a sound effect
// =======================================================================

static void SetChannelOrigin(channel_t *ch, const vec3_t origin, int entityNum)
{
	if (origin) 
	{
		ch->origin[0] = origin[0];
		ch->origin[1] = origin[1];
		ch->origin[2] = origin[2];
	}
	else
	{
		vec3_t pos;

		extern void G_EntityPosition( int i, vec3_t ret );
		G_EntityPosition(entityNum, pos);
		
		ch->origin[0] = pos[0];
		ch->origin[1] = pos[1];
		ch->origin[2] = pos[2];
	}

	ch->bOriginDirty = true;
}

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
	if ( sfxHandle < 0 || sfxHandle > MAX_SFX || s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return;
	}
	if ( !origin && ( entityNum < 0 || entityNum > MAX_GENTITIES ) ) {
		Com_Error( ERR_DROP, "S_StartAmbientSound: bad entitynum %i", entityNum );
	}

	sfx = &s_sfxBlock[sfxHandle];
	if (sfx->iFlags & SFX_FLAG_UNLOADED){
		S_StartLoadSound(sfx);
	}	
	SND_TouchSFX(sfx);

	// pick a channel to play on
	bool is2D = false;
	for (int i = 0; i < s_numListeners; ++i)
	{
		if ((entityNum == s_listeners[i].entnum && !origin) ||
			(origin &&
			origin[0] == s_listeners[i].pos[0] &&
			origin[1] == s_listeners[i].pos[1] &&
			origin[2] == s_listeners[i].pos[2]))
		{
			is2D = true;
			break;
		}
	}

	ch = S_PickChannel( entityNum, CHAN_AMBIENT, is2D, NULL );
	if (!ch) {
		return;
	}
	
	if (!is2D)
	{
		SetChannelOrigin(ch, origin, entityNum);
	}

	ch->master_vol = volume;
	ch->fLastVolume = -1;
	ch->entnum = entityNum;
	ch->entchannel = CHAN_AMBIENT;
	ch->thesfx = sfx;
}

/*
====================
S_MuteSound

Mutes sound on specified channel for specified entity.
This seems to be implemented quite incorrectly on PC. I
think the following is what this function should do...

Perhaps we should actually be changing the volume on all
the channels that meet our criteria, but for now we'll just
kill the sounds and hope it does what is expected.
====================
*/
void S_MuteSound(int entityNum, int entchannel) 
{
	S_KillEntityChannel( entityNum, entchannel );

/*
	if (!s_soundStarted) {
		return;
	}

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
*/
}

/*
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle ) 
{
	channel_t	*ch;
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle > MAX_SFX || s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return;
	}

	if ( !origin && ( entityNum < 0 || entityNum > MAX_GENTITIES ) ) {
		Com_Error( ERR_DROP, "S_StartSound: bad entitynum %i", entityNum );
	}

	sfx = &s_sfxBlock[sfxHandle];

	if (sfx->iFlags & SFX_FLAG_UNLOADED){
		S_StartLoadSound(sfx);
	}
	SND_TouchSFX(sfx);

	// pick a channel to play on
	bool is2D = false;
	for (int i = 0; i < s_numListeners; ++i)
	{
		if ((entityNum == s_listeners[i].entnum && !origin) ||
			(origin &&
			origin[0] == s_listeners[i].pos[0] &&
			origin[1] == s_listeners[i].pos[1] &&
			origin[2] == s_listeners[i].pos[2]))
		{
			is2D = true;
			break;
		}
	}
	
	ch = S_PickChannel( entityNum, entchannel, is2D, sfx );
	if (!ch) {
		return;
	}

	if (!is2D)
	{
		SetChannelOrigin(ch, origin, entityNum);
	}
		
	if (entchannel == CHAN_AUTO && (sfx->iFlags & SFX_FLAG_VOICE)) {
		entchannel = CHAN_VOICE; // Compensate of the incompetance of others. Yeah. ;)
//		entchannel = CHAN_VOICE_ATTEN; // Super hack to put Rancor noises on a different channel E3!
	}

	ch->master_vol = SOUND_MAXVOL;	//FIXME: Um.. control?
	ch->fLastVolume = -1;
	ch->entnum = entityNum;
	ch->entchannel = entchannel;
	ch->thesfx = sfx;

	if (entchannel < CHAN_AMBIENT && IsListenerEnt(ch->entnum))
	{
		ch->master_vol = SOUND_MAXVOL * SOUND_FMAXVOL;	//this won't be attenuated so let it scale down
	}
	if ( entchannel == CHAN_VOICE || entchannel == CHAN_VOICE_ATTEN || entchannel == CHAN_VOICE_GLOBAL ) 
	{
		s_entityWavVol[ ch->entnum ] = -1;	//we've started the sound but it's silent for now
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

	// Play a 2D sound -- doesn't matter which listener we use
	S_StartSound (NULL, 0, channelNum, sfxHandle );
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

	// Play a 2D sound -- doesn't matter which listener we use
	S_AddLoopingSound( 0, nullVec, nullVec, sfxHandle, CHAN_AMBIENT );

}

// Kill an voice sounds from an entity
void S_KillEntityChannel(int entnum, int chan)
{
	int i;
	channel_t *ch;

	if ( !s_soundStarted ) {
		return;
	}

	if ( entnum < s_numListeners && chan == CHAN_VOICE ) {
		// don't kill player death sounds
		return;
	}

	ch = s_channels;
	for (i = 0; i < s_numChannels; i++, ch++)
	{
		if (ch->bPlaying &&
			ch->entnum == entnum &&
			ch->entchannel == chan)
		{
			alSourceStop(ch->alSource);
			ch->bPlaying = false;
	
			alSourcei(ch->alSource, AL_BUFFER, 0);
			if(ch->thesfx)
				ch->thesfx->pLipSyncData = NULL;
			ch->thesfx = NULL;
			ch->bLooping = false;
		}
	}
}

/*
==================
S_StopLoopingSound

Stops all active looping sounds on a specified entity.
Sort of a slow method though, isn't there some better way?
==================
*/
void S_StopLoopingSound( int entnum )
{
	if (!s_soundStarted) {
		return;
	}

	int i = 0;

	while (i < numLoopSounds)
	{
		if (loopSounds[i].entnum == entnum)
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

// returns length in milliseconds of supplied sound effect...  (else 0 for bad handle now)
//
float S_GetSampleLengthInMilliSeconds( sfxHandle_t sfxHandle)
{
	sfx_t *sfx;

	if (!s_soundStarted)
	{	//we have no sound, so let's just make a reasonable guess
		return 512;
	}

	if ( s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return 0.0f;
	}

	sfx = &s_sfxBlock[sfxHandle];

	int size = Sys_GetFileCodeSize(sfx->iFileCode);
	if (size < 0) return 0;

	return 1000 * size / (22050 / 2);
}

/*
==================
S_LoadSound
==================
*/
void S_LoadSound( sfxHandle_t sfxHandle ) 
{
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle > MAX_SFX || s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return;
	}

	sfx = &s_sfxBlock[sfxHandle];

	if (sfx->iFlags & SFX_FLAG_UNLOADED){
		S_StartLoadSound(sfx);

		extern void S_DrainRawSoundData(void);
		S_DrainRawSoundData();
	}
}

/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_ClearSoundBuffer( void ) {
	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
}


/*
==================
S_StopAllSounds
==================
*/
void S_StopSounds(void)
{
	int i; //, j;
	channel_t *ch;

	if ( !s_soundStarted ) {
		return;
	}

	// stop looping sounds
	S_ClearLoopingSounds();

	// clear all the s_channels
	ch = s_channels;
	for (i = 0; i < s_numChannels; i++, ch++)
	{
		if (ch->bPlaying)
		{
			alSourceStop(ch->alSource);
			ch->bPlaying = false;
		}
	
		// free lip sync data
		if(ch->thesfx)
			ch->thesfx->pLipSyncData = NULL;

		alSourcei(ch->alSource, AL_BUFFER, 0);
		ch->thesfx = NULL;
		ch->bLooping = false;
	}

	// clear out the lip synching override array
	memset(s_entityWavVol, 0, sizeof(int) * MAX_GENTITIES);

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
	if ( !s_soundStarted ) {
		return;
	}

	int i;

	for (i = 0; i < MAX_LOOP_SOUNDS; i++)
	{
		loopSounds[i].bProcessed = false;
		loopSounds[i].bMarked = false;
		loopSounds[i].sfx = NULL;
	}

	numLoopSounds = 0;
}

/*
==================
S_AddLoopingSound

Called during entity generation for a frame
==================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle, int chan ) {
	/*const*/ sfx_t *sfx;

  	if ( !s_soundStarted || s_soundMuted || !s_loopEnabled ) {
		return;
	}
	if ( numLoopSounds >= MAX_LOOP_SOUNDS ) {
		return;
	}

	if ( sfxHandle < 0 || sfxHandle > MAX_SFX || s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return;
	}

	sfx = &s_sfxBlock[sfxHandle];
	if (sfx->iFlags & SFX_FLAG_UNLOADED){
		S_StartLoadSound(sfx);
	}
	SND_TouchSFX(sfx);

	loopSounds[numLoopSounds].origin[0] = origin[0];
	loopSounds[numLoopSounds].origin[1] = origin[1];
	loopSounds[numLoopSounds].origin[2] = origin[2];

	loopSounds[numLoopSounds].sfx = sfx;
	loopSounds[numLoopSounds].volume = SOUND_MAXVOL;
	loopSounds[numLoopSounds].entnum = entityNum;
	loopSounds[numLoopSounds].entchannel = chan;
	numLoopSounds++;
}


/*
==================
S_AddAmbientLoopingSound
==================
*/
void S_AddAmbientLoopingSound( const vec3_t origin, unsigned char volume, sfxHandle_t sfxHandle ) 
{
	/*const*/ sfx_t *sfx;

	if ( !s_soundStarted || s_soundMuted || !s_loopEnabled ) {
		return;
	}
	if ( numLoopSounds >= MAX_LOOP_SOUNDS ) {
		return;
	}

	if (volume == 0)
		return;

	if ( sfxHandle < 0 || sfxHandle > MAX_SFX || s_sfxCodes[sfxHandle] == INVALID_CODE ) {
		return;
	}

	sfx = &s_sfxBlock[sfxHandle];
	if (sfx->iFlags & SFX_FLAG_UNLOADED){
		S_StartLoadSound(sfx);
	}
	SND_TouchSFX(sfx);

	loopSounds[numLoopSounds].origin[0] = origin[0];
	loopSounds[numLoopSounds].origin[1] = origin[1];
	loopSounds[numLoopSounds].origin[2] = origin[2];
	
	loopSounds[numLoopSounds].sfx = sfx;	
	loopSounds[numLoopSounds].volume = volume;
	loopSounds[numLoopSounds].entnum = -1;
	numLoopSounds++;
}




/*
=====================
S_UpdateEntityPosition

let the sound system know where an entity currently is
======================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if ( !s_soundStarted ) {
		return;
	}

	channel_t *ch;
	int i;

	if ( entityNum < 0 || entityNum > MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}

	if (entityNum == 0)
		return;
	
	ch = s_channels;
	for (i = 0; i < s_numChannels; i++, ch++)
	{
		if ((ch->bPlaying) && 
			(ch->entnum == entityNum) &&
			(!ch->b2D))
		{
			if (ch->origin[0] != origin[0] ||
				ch->origin[1] != origin[1] ||
				ch->origin[2] != origin[2])
			{
				ch->origin[0] = origin[0];
				ch->origin[1] = origin[1];
				ch->origin[2] = origin[2];
				ch->bOriginDirty = true;
			}
		}
	}
}


/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], qboolean inwater )
{
	if ( !s_soundStarted || s_soundMuted ) {
		return; 
	}

	int index = 0;

#if 0	
	extern qboolean g_isMultiplayer;
	if ( g_isMultiplayer ) {
		index = entityNum;
	}
#endif

	if ( index >= s_numListeners ) {
		return;
	}

	listener_t *li = &s_listeners[index];

	li->entnum = entityNum;
	
	li->pos[0] = head[0];
	li->pos[1] = head[1];
	li->pos[2] = head[2];
	alListenerfv(li->handle, AL_POSITION, li->pos);
	
	li->orient[0] = axis[0][0];
	li->orient[1] = axis[0][1];
	li->orient[2] = axis[0][2];
	li->orient[3] = axis[2][0];
	li->orient[4] = axis[2][1];
	li->orient[5] = axis[2][2];
	
	alListenerfv(li->handle, AL_ORIENTATION, li->orient);
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( void ) {
	if ( !s_soundStarted ) {
		return;
	}

	// don't update too often
	int now = Sys_Milliseconds();
	if (now - s_updateTime < SOUND_UPDATE_TIME) {
		return;
	}
	s_updateTime = now;
	
	if ( s_soundMuted ) {
		alUpdate();
		return;
	}

	// finish up any pending loads
	S_UpdateLoading();

	// update the music stream
	S_UpdateBackgroundTrack();

	// mix some sound
	S_Update_();

	alUpdate();
}

static void UpdatePosition(channel_t *ch)
{
	if ( !ch->b2D )
	{
		if ( ch->bLooping && ch->bPlaying )
		{
			loopSound_t	*loop = &loopSounds[ch->loopChannel];
			if ( loop->origin[0] != ch->origin[0] ||
				loop->origin[1] != ch->origin[1] ||
				loop->origin[2] != ch->origin[2] )
			{
				ch->origin[0] = loop->origin[0];
				ch->origin[1] = loop->origin[1];
				ch->origin[2] = loop->origin[2];
				ch->bOriginDirty = true;
			}
		}
		
		if ( ch->bOriginDirty )
		{
			alSourcefv(ch->alSource, AL_POSITION, ch->origin);
			ch->bOriginDirty = false;
		}
	}
}

static void UpdateGain(channel_t *ch)
{
	float v = 0.f;

	if ( ch->bLooping && ch->bPlaying )
	{
		loopSound_t	*loop = &loopSounds[ch->loopChannel];
		if ( loop->volume != ch->master_vol )
		{
			ch->master_vol = loop->volume;
		}
	}
	
	if ( ch->entchannel == CHAN_ANNOUNCER ||
		ch->entchannel == CHAN_VOICE || 
		ch->entchannel == CHAN_VOICE_ATTEN || 
		ch->entchannel == CHAN_VOICE_GLOBAL )
	{
		v = ((float)(ch->master_vol) * s_voice_volume->value) / 255.0f;
	}
	else if ( ch->entchannel == CHAN_MUSIC )
	{
		v = ((float)(ch->master_vol) * s_music_volume->value) / 255.f;
	}
	else
	{
		v = ((float)(ch->master_vol) * s_effects_volume->value) / 255.f;
	}
	
	if ( ch->fLastVolume != v)
	{
		alSourcef(ch->alSource, AL_GAIN, v);
		ch->fLastVolume = v;
	}
}

static void UpdatePlayState(channel_t *ch)
{
	if (!ch->bPlaying) return;

	if (ch->bLooping)
	{
		// Looping sound
		loopSound_t	*loop = &loopSounds[ch->loopChannel];
		
		if (loop->bProcessed == false && loop->sfx != NULL &&
			(loop->sfx == ch->thesfx ||
			(loop->sfx->iFlags & SFX_FLAG_DEFAULT)))
		{
			// Playing
			loop->bProcessed = true;
		}
		else
		{
			// Sound no longer needed
			alSourceStop(ch->alSource);
			alSourcei(ch->alSource, AL_BUFFER, 0);
			ch->thesfx = NULL;
			ch->bPlaying = false;
		}
	}
	else
	{
		// Single shot sound
		ALint state;
		alGetSourcei(ch->alSource, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED)
		{
			alSourcei(ch->alSource, AL_BUFFER, 0);
			ch->thesfx = NULL;
			ch->bPlaying = false;
		}
	}
}

static void UpdateAttenuation(channel_t *ch)
{
	if (!ch->b2D)
	{
		/*
		if ( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL )
		{
			alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, 1500.0f);
		}
		else
		{
			alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, 400.0f);
		}
		*/
#if 0
		extern qboolean g_isMultiplayer;
		if (!g_isMultiplayer)
		{
#endif
			switch (ch->entchannel)
			{
			case CHAN_VOICE:
				alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE * 3.f);
				break;
			case CHAN_LESS_ATTEN:
				alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE * 8.f);
				break;
			case CHAN_VOICE_ATTEN:
				alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE * 1.35f);
				break;
			case CHAN_VOICE_GLOBAL:
				alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE * 100.f);
				break;
			default:
				alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE);
				break;
			}
#if 0
		}
		else
		{
			alSourcef(ch->alSource, AL_REFERENCE_DISTANCE, SOUND_REF_DIST_BASE * 2.f);
		}
#endif
	}
}

static void PlaySingleShot(channel_t *ch)
{
	alSourcei(ch->alSource, AL_LOOPING, AL_FALSE);
	
	UpdateAttenuation(ch);
	UpdatePosition(ch);
	
	// Attach buffer to source
	alSourcei(ch->alSource, AL_BUFFER, ch->thesfx->Buffer);
	
	// Clear error state, and check for successful Play call
	alGetError();
	alSourcePlay(ch->alSource);
	if (alGetError() == AL_NO_ERROR)
	{
		ch->bPlaying = true;
		ch->iLastPlayTime = Sys_Milliseconds();
	}
}

void UpdateLoopingSounds()
{
	// Look for non-processed loops that are ready to play
	for (int j = 0; j < numLoopSounds; j++)
	{
		loopSound_t	*loop = &loopSounds[j];
		
		{
			// merge all loops with the same sfx into a single loop
			float num = 1;
			for (int k = j+1; k < numLoopSounds; ++k)
			{
				if (loopSounds[k].sfx == loop->sfx)
				{
					loop->origin[0] += loopSounds[k].origin[0];
					loop->origin[1] += loopSounds[k].origin[1];
					loop->origin[2] += loopSounds[k].origin[2];
					loop->volume += loopSounds[k].volume;
					loopSounds[k].bProcessed = true;
					num += 1;
				}
			}

			loop->origin[0] /= num;
			loop->origin[1] /= num;
			loop->origin[2] /= num;
			loop->volume /= (int)num;
		}

		if (loop->bProcessed == false && (loop->sfx->iFlags & SFX_FLAG_RESIDENT))
		{
			// play the loop
			bool is2D = false;
			for (int i = 0; i < s_numListeners; ++i)
			{
				if (loop->entnum == s_listeners[i].entnum ||
					(loop->origin[0] == s_listeners[i].pos[0] &&
					loop->origin[1] == s_listeners[i].pos[1] &&
					loop->origin[2] == s_listeners[i].pos[2]))
				{
					is2D = true;
					break;
				}
			}

			channel_t *ch = S_PickChannel(0, 0, is2D, NULL);
			if (!ch) continue;

			ch->master_vol = loop->volume;
			ch->fLastVolume = -1;
			ch->entnum = loop->entnum;
			ch->entchannel = loop->entchannel;
			ch->thesfx = loop->sfx;
			ch->loopChannel = j;
			ch->bLooping = true;
			
			ch->origin[0] = loop->origin[0];
			ch->origin[1] = loop->origin[1];
			ch->origin[2] = loop->origin[2];
			ch->bOriginDirty = true;

			alSourcei(ch->alSource, AL_LOOPING, AL_TRUE);
			alSourcei(ch->alSource, AL_BUFFER, ch->thesfx->Buffer);
			UpdateAttenuation(ch);
			UpdatePosition(ch);
			UpdateGain(ch);
			
			alGetError();
			alSourcePlay(ch->alSource);
			if (alGetError() == AL_NO_ERROR)
			{
				ch->bPlaying = true;
				ch->iLastPlayTime = Sys_Milliseconds();
			}
		}
	}
}

static void SyncChannelLoops(void)
{
	channel_t		*ch;
	int				i, j;

	// Try to match up channels with looping sounds
	// (The order of sounds in loopSounds can change
	// frame to frame.)
	ch = s_channels;
	for ( i = 0; i < s_numChannels ; i++, ch++ )
	{
		if ( ch->bPlaying && ch->bLooping )
		{
			for ( j = 0; j < numLoopSounds; ++j )
			{
				if ( ch->thesfx == loopSounds[j].sfx && 
					!loopSounds[j].bMarked )
				{
					ch->loopChannel = j;
					loopSounds[j].bMarked = true;
					break;
				}
			}
		}
	}
}

void _UpdateLipSyncData( channel_t*	ch)
{
	int		samples;
	int		currentTime;
	int		timePlayed;
	int		length;
	char*	data;

	if (ch->thesfx->pLipSyncData == NULL)
	{
		//Com_Printf("Missing lip-sync info: %s\n", ch->thesfx->sSoundName);
		return;
	}

	// Get current time
	currentTime = Sys_Milliseconds();

	// Calculate how much time has passed since the sample was started
	timePlayed = currentTime - ch->iLastPlayTime;

	// There is a new computed lip-sync value every 1000 samples - so find out how many samples
	// have been played and lookup the value in the lip-sync table
	samples = (timePlayed * 22050) / 1000;

	// Get the number of total samples in this sound
	length	= *(int*)ch->thesfx->pLipSyncData;

	// Get a ptr to the lipsync data
	data	= (char*)((int*)ch->thesfx->pLipSyncData + 1);

	if ((ch->thesfx->pLipSyncData) && (samples < length))
	{
		int p = samples / 500 ;
		short t;

		if(p%2 == 0 )
		{
			t = data[p/2];
			t = t >> 4;
			if(t == 0)
				t = -1;

			s_entityWavVol[ ch->entnum ] = t;		// want left 4 bits
		}
		else
		{
			t = data[p/2] & 0x0f;
			if(t == 0)
				t = -1;
			s_entityWavVol[ ch->entnum ] = t;	// want right 4 bits
		}
		//Com_Printf("%s,  total samples = %d, current sample = %d, lip index = %d, lip type = %d \n", ch->thesfx->sSoundName, length, samples, p/2, s_entityWavVol[ ch->entnum ] );
	}

}

void S_Update_(void)
{
	channel_t		*ch;
	int				i;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}
	
	memset(s_entityWavVol, 0, sizeof(int) * MAX_GENTITIES);

	SyncChannelLoops();

	ch = s_channels;
	for ( i = 0; i < s_numChannels ; i++, ch++ )
	{	
		if ( !ch->thesfx ) continue;

		if ( ch->thesfx->iFlags & SFX_FLAG_UNLOADED )
		{
			// if the sound is not going to be loaded, force the 
			// playing flag high, stop the source, and hope that 
			// the update code cleans it up...
			ch->bPlaying = true;
			alSourceStop(ch->alSource);
			continue;
		}
		
		if ( ch->entchannel == CHAN_VOICE || 
			ch->entchannel == CHAN_VOICE_ATTEN || 
			ch->entchannel == CHAN_VOICE_GLOBAL )
		{
			//s_entityWavVol[ch->entnum] = ch->bPlaying ? 4 : -1;
			if(ch->bPlaying)
			{
				_UpdateLipSyncData(ch);
			}
			else
			{
				s_entityWavVol[ch->entnum] = -1;
			}

		}

		if ( !(ch->thesfx->iFlags & SFX_FLAG_RESIDENT) ) continue;

		UpdatePosition(ch);
		UpdateGain(ch);

		if ( ch->bPlaying )
		{
			UpdatePlayState(ch);
		}
		else
		{
			PlaySingleShot(ch);
		}
	}

	UpdateLoopingSounds();
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

/*
 * Crazy expanded play function:
 * playex <file name> xOffset yOffset zOffset channel
 */
#ifndef _JK2MP
static void S_PlayEx_f( void ) {
	sfxHandle_t	h;
	char		name[256] = { 0 };
	vec3_t		origin;
	int			entchannel;

	if (Cmd_Argc() < 6)
		return;

	Q_strncpyz( name, Cmd_Argv(1), sizeof(name) );
	h = S_RegisterSound( name );
	if (!h)
		return;

	extern void G_EntityPosition( int i, vec3_t ret );
	G_EntityPosition(0, origin);

	origin[0] += atof(Cmd_Argv(2));
	origin[1] += atof(Cmd_Argv(3));
	origin[2] += atof(Cmd_Argv(4));

	entchannel = atoi(Cmd_Argv(5));

	S_StartSound(origin, 0, entchannel, h);
}
#endif

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

void S_SoundList_f( void ) {
}


/*
===============================================================================

background music functions

===============================================================================
*/

// fixme: need to move this into qcommon sometime?, but too much stuff altered by other people and I won't be able
//	to compile again for ages if I check that out...
//
// DO NOT replace this with a call to FS_FileExists, that's for checking about writing out, and doesn't work for this.
//
qboolean S_MusicFileExists( const char *psFilename )
{
	fileHandle_t fhTemp;

	char* pLoadName = S_FixMusicFileName(psFilename);
	
	FS_FOpenFileRead (pLoadName, &fhTemp, qtrue);	// qtrue so I can fclose the handle without closing a PAK
	if (!fhTemp) 
		return qfalse;
	
	FS_FCloseFile(fhTemp);
	return qtrue;
}

static void S_StopBackgroundTrack_Actual( MusicInfo_t *pMusicInfo ) 
{
	pMusicInfo->bLoaded = false;
	pMusicInfo->Rewind();
	alStreamStop();
}

static void FreeMusic( MusicInfo_t *pMusicInfo )
{
	pMusicInfo->sLoadedDataName[0] = '\0';
}

// called only by snd_shutdown (from snd_restart or app exit)
//
static void S_UnCacheDynamicMusic( void )
{
	for (int i = eBGRNDTRACK_DATABEGIN; i != eBGRNDTRACK_DATAEND; i++)
	{
		FreeMusic( &tMusic_Info[i]);
	}
}

static qboolean S_StartBackgroundTrack_Actual( MusicInfo_t *pMusicInfo, qboolean qbDynamic, const char *intro, const char *loop )
{
	Q_strncpyz( sMusic_BackgroundLoop, loop, sizeof( sMusic_BackgroundLoop ));	

	char* name = S_FixMusicFileName(intro);

	if ( !intro[0] ) {
		S_StopBackgroundTrack_Actual( pMusicInfo );
		return qfalse;
	}

	// new bit, if file requested is not same any loaded one (if prev was in-mem), ditch it...
	//
	if (Q_stricmp(name, pMusicInfo->sLoadedDataName))
	{
		FreeMusic( pMusicInfo );
	}

	//
	// open up a wav file and get all the info
	//
	fileHandle_t handle;
	int len = FS_FOpenFileRead( name, &handle, qtrue );
	if ( !handle ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", name );
		S_StopBackgroundTrack_Actual( pMusicInfo );
		return qfalse;
	}
	
#if defined(_XBOX) || defined(_WINDOWS)
	// read enough of the file to get the header...
	byte buffer[128];
	FS_Read(buffer, sizeof(buffer), handle);
	FS_FCloseFile( handle );

	wavinfo_t info = GetWavInfo(buffer);
	if ( info.size == 0 ) {
		Com_Printf(S_COLOR_YELLOW "WARNING: Invalid format in %s\n", name);
		S_StopBackgroundTrack_Actual( pMusicInfo );
		return qfalse;
	}
	
	pMusicInfo->s_backgroundSize = info.size;
	pMusicInfo->s_backgroundBPS = info.rate * info.width / 8;
	if (info.format == AL_FORMAT_STEREO4)
	{
		pMusicInfo->s_backgroundBPS <<= 1;
	}
#elif defined(_GAMECUBE)
	FS_FCloseFile( handle );
	pMusicInfo->s_backgroundSize = len;
	pMusicInfo->s_backgroundBPS = 48000 * 4 / 8 * 2;
#endif
	
	Q_strncpyz(pMusicInfo->sLoadedDataName, intro, sizeof(pMusicInfo->sLoadedDataName));
	pMusicInfo->iFileCode = Sys_GetFileCode(name);
	pMusicInfo->bLoaded = true;
	
	return qtrue;
}

static void S_SwitchDynamicTracks( MusicState_e eOldState, MusicState_e eNewState, qboolean bNewTrackStartsFullVolume )
{
	// copy old track into fader...
	//
	tMusic_Info[ eBGRNDTRACK_FADE ] = tMusic_Info[ eOldState ];
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

	// sanity check
	if (tMusic_Info[eNewState].iFileSeekTo >= tMusic_Info[eNewState].s_backgroundSize)
	{
		tMusic_Info[eNewState].iFileSeekTo = 0;
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
			LPCSTR	psNewStateString = Music_BaseStateToString( eNewState, qtrue );
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
							float fPlayingTimeElapsed = tMusic_Info[ eMusic_StateActual ].ElapsedTime();

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
								tMusic_Info[eTransition].Rewind();
								tMusic_Info[eTransition].bTrackSwitchPending	= qtrue;
								tMusic_Info[eTransition].bLooping				= qfalse;
								tMusic_Info[eTransition].eTS_NewState			= eMusic_StateRequest;
								tMusic_Info[eTransition].fTS_NewTime			= fNewTrackEntryTime;

								S_SwitchDynamicTracks( eMusic_StateActual, eTransition, qfalse );	// qboolean bNewTrackStartsFullVolume
							}
						}
						break;						

						case eBGRNDTRACK_SILENCE:	// silence->explore
						{
							tMusic_Info[ eMusic_StateRequest ].Rewind();
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
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
							float fPlayingTimeElapsed = tMusic_Info[ eMusic_StateActual ].ElapsedTime();

							MusicState_e	eTransition;
							float			fNewTrackEntryTime = 0.0f;
							if (Music_AllowedToTransition( fPlayingTimeElapsed, eMusic_StateActual, &eTransition, &fNewTrackEntryTime))
							{
								tMusic_Info[eTransition].Rewind();
								tMusic_Info[eTransition].bTrackSwitchPending	= qtrue;
								tMusic_Info[eTransition].bLooping				= qfalse;
								tMusic_Info[eTransition].eTS_NewState			= eMusic_StateRequest;
								tMusic_Info[eTransition].fTS_NewTime			= 0.0f;	//fNewTrackEntryTime;  irrelevant when switching to silence

								S_SwitchDynamicTracks( eMusic_StateActual, eTransition, qfalse );	// qboolean bNewTrackStartsFullVolume
							}
						}
						break;

						default:		// some unhandled type switching to silence
							assert(0);	// fall through since boss case just does silence->switch anyway

						case eBGRNDTRACK_BOSS:	// boss->silence
						{
							tMusic_Info[eBGRNDTRACK_SILENCE].Rewind();
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
							tMusic_Info[ eMusic_StateRequest ].Rewind();
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
						}
						break;

						default:	// !silence->action
						{
							float fEntryTime = Music_GetRandomEntryTime( eMusic_StateRequest );
							tMusic_Info[ eMusic_StateRequest ].SeekTo(fEntryTime);
							S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
						}
						break;
					}
				}
				break;

				case eBGRNDTRACK_BOSS:
				{	
					tMusic_Info[eMusic_StateRequest].Rewind();
					S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qfalse );	// qboolean bNewTrackStartsFullVolume
				}
				break;

				case eBGRNDTRACK_DEATH:
				{	
					tMusic_Info[eMusic_StateRequest].Rewind();
					S_SwitchDynamicTracks( eMusic_StateActual, eMusic_StateRequest, qtrue );	// qboolean bNewTrackStartsFullVolume
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
		S_StartBackgroundTrack( gsIntroMusic, gsLoopMusic, qfalse );	// ( default music start will set the state to EXPLORE )
		S_SetDynamicMusicState( eMusic_StateRequest );					// restore to prev state
	}
}


// Basic logic here is to see if the intro file specified actually exists, and if so, then it's not dynamic music,
//	When called by the cgame start it loads up, then stops the playback (because of stutter issues), so that when the
//	actual snapshot is received and the real play request is processed the data has already been loaded so will be quicker.
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

	Q_strncpyz(gsIntroMusic,intro, sizeof(gsIntroMusic));
	Q_strncpyz(gsLoopMusic, loop,  sizeof(gsLoopMusic));

	char sName[MAX_QPATH];
	Q_strncpyz(sName,intro,sizeof(sName));

	COM_DefaultExtension( sName, sizeof( sName ), ".wxb" );

	// if dynamic music not allowed, then just stream the explore music instead of playing dynamic...
	//
	if (!s_allowDynamicMusic->integer && Music_DynamicDataAvailable(intro))	// "intro", NOT "sName" (i.e. don't use version with ".wxb" extension)
	{
		LPCSTR psMusicName = Music_GetFileNameForState( eBGRNDTRACK_DATABEGIN );
		if (psMusicName && S_MusicFileExists( psMusicName ))
		{
			Q_strncpyz(sName,psMusicName,sizeof(sName));
		}
	}

	// conceptually we always play the 'intro'[/sName] track, intro-to-loop transition is handled in UpdateBackGroundTrack().
	//
	if ( (strstr(sName,"/") && S_MusicFileExists( sName )) )	// strstr() check avoids extra file-exists check at runtime if reverting from streamed music to dynamic since literal files all need at least one slash in their name (eg "music/blah")
	{
		Com_DPrintf("S_StartBackgroundTrack: Found/using non-dynamic music track '%s'\n", sName);
		tMusic_Info[eBGRNDTRACK_NONDYNAMIC].bLooping = qtrue;
		S_StartBackgroundTrack_Actual( &tMusic_Info[eBGRNDTRACK_NONDYNAMIC], bMusic_IsDynamic, sName, sName );
	}
	else
	{
		if (Music_DynamicDataAvailable(intro))	// "intro", NOT "sName" (i.e. don't use version with ".wxb" extension)
		{
			int i;
			extern const char *Music_GetLevelSetName(void);
			Q_strncpyz(sInfoOnly_CurrentDynamicMusicSet, Music_GetLevelSetName(), sizeof(sInfoOnly_CurrentDynamicMusicSet));
			for (i = eBGRNDTRACK_DATABEGIN; i != eBGRNDTRACK_DATAEND; i++)
			{
				qboolean bOk = qfalse;
				LPCSTR psMusicName = Music_GetFileNameForState( (MusicState_e) i);
				if (psMusicName && (!Q_stricmp(tMusic_Info[i].sLoadedDataName, psMusicName) || S_MusicFileExists( psMusicName )) )
				{
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
			for (i=0; i<eBGRNDTRACK_NUMBEROF; i++)
			{
				tMusic_Info[i].bActive				= qfalse;
				tMusic_Info[i].bTrackSwitchPending	= qfalse;
				tMusic_Info[i].bLooping				= qtrue;
				tMusic_Info[i].fSmoothedOutVolume	= 0.25f;
			}

			tMusic_Info[eBGRNDTRACK_DEATH].bLooping		= qfalse;

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
			}
			else
			{
				Com_Printf( S_COLOR_RED "Dynamic music did not have both 'action' and 'explore' versions, inhibiting...\n");
				S_StopBackgroundTrack();
			}
		}
		else
		{
			if (sName[0]!='.')	// blank name with ".wxb" or whatever attached - no error print out
			{
				Com_Printf( S_COLOR_RED "Unable to find music \"%s\" as explicit track or dynamic music entry!\n",sName);
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
}



// qboolean return is true only if we're changing from a streamed intro to a dynamic loop...
//
static qboolean S_UpdateBackgroundTrack_Actual( MusicInfo_t *pMusicInfo, qboolean bFirstOrOnlyMusicTrack, float fDefaultVolume) 
{
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

	if ( pMusicInfo->bLoaded == false ) {
		return qfalse;
	}

	pMusicInfo->fSmoothedOutVolume = (pMusicInfo->fSmoothedOutVolume + fMasterVol)/2.0f;

	alStreamf(AL_GAIN, pMusicInfo->fSmoothedOutVolume);

	// don't bother playing anything if musicvolume is 0
	if ( pMusicInfo->fSmoothedOutVolume <= 0 ) {
		return qfalse;
	}

	// start playing if necessary
	if ( pMusicInfo->bLooping )
	{
		ALint state;
		alGetStreami(AL_SOURCE_STATE, &state);
		if ( state != AL_PLAYING )
		{
			alStreamPlay(pMusicInfo->iFileSeekTo, 
				pMusicInfo->iFileCode, 
				pMusicInfo->bLooping);
		}
	}

	if ( pMusicInfo->PlayTime() >= pMusicInfo->TotalTime() ) 
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
			
			if (S_MusicFileExists( sTestName ))
			{
				// Restart the music
				alStreamStop();
				alStreamPlay(pMusicInfo->iFileSeekTo, 
					pMusicInfo->iFileCode, 
					pMusicInfo->bLooping);
			}
			else
			{
				// proposed file doesn't exist, but this may be a dynamic track we're wanting to loop, 
				//	so exit with a special flag...
				//
				return qtrue;
			}
		}
	}

	return qfalse;
}


// used to be just for dynamic, but now even non-dynamic music has to know whether it should be silent or not...
//
static LPCSTR S_Music_GetRequestedState(void)
{
// This doesn't do anything in MP - just return NULL
#ifndef _JK2MP
	int iStringOffset = cl.gameState.stringOffsets[CS_DYNAMIC_MUSIC_STATE];
	if (iStringOffset)
	{
		LPCSTR psCommand = cl.gameState.stringData+iStringOffset; 

		return psCommand;
	}
#endif

	return NULL;
}


// scan the configstring to see if there's been a state-change requested...
// (note that even if the state doesn't change it still gets here, so do a same-state check for applying)
//
// then go on to do transition handling etc...
//
static void S_CheckDynamicMusicState(void)
{
	LPCSTR psCommand = S_Music_GetRequestedState();

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

static void S_UpdateBackgroundTrack( void )
{
	if (bMusic_IsDynamic)
	{
		S_CheckDynamicMusicState();
		
		if (eMusic_StateActual != eBGRNDTRACK_SILENCE)
		{
			MusicInfo_t *pMusicInfoCurrent = &tMusic_Info[ (eMusic_StateActual == eBGRNDTRACK_FADE)?eBGRNDTRACK_EXPLORE:eMusic_StateActual ];
			MusicInfo_t *pMusicInfoFadeOut = &tMusic_Info[ eBGRNDTRACK_FADE ];

			if ( pMusicInfoCurrent->bLoaded )
			{
				float fRemainingTimeInSeconds = 1000000;

				if (pMusicInfoFadeOut->bActive)
				{
					S_UpdateBackgroundTrack_Actual( pMusicInfoFadeOut, qfalse, s_music_volume->value );	// inactive-checked internally

					//
					// only do this for the fader!...
					//
					if (pMusicInfoFadeOut->iXFadeVolume == 0)
					{
						pMusicInfoFadeOut->bActive = qfalse;

						// play if we have a file
						if (pMusicInfoCurrent->iFileCode)
						{
							alStreamPlay(pMusicInfoCurrent->iFileSeekTo, 
								pMusicInfoCurrent->iFileCode,
								pMusicInfoCurrent->bLooping);
							
							pMusicInfoCurrent->iXFadeVolumeSeekTime = Sys_Milliseconds();
						}
						else
						{
							alStreamStop();
						}
					}
				}
				else
				{
					S_UpdateBackgroundTrack_Actual( pMusicInfoCurrent, qtrue, s_music_volume->value );
					fRemainingTimeInSeconds = pMusicInfoCurrent->TotalTime() - pMusicInfoCurrent->ElapsedTime();
				}
				
				if ( fRemainingTimeInSeconds < fDYNAMIC_XFADE_SECONDS*2 )
				{
					// now either loop current track, switch if finishing a transition, or stop if finished a death...
					//
					if (pMusicInfoCurrent->bTrackSwitchPending)
					{
						pMusicInfoCurrent->bTrackSwitchPending = qfalse;	// ack
						tMusic_Info[ pMusicInfoCurrent->eTS_NewState ].SeekTo(pMusicInfoCurrent->fTS_NewTime);
						S_SwitchDynamicTracks( eMusic_StateActual, pMusicInfoCurrent->eTS_NewState, qfalse);	// qboolean bNewTrackStartsFullVolume
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
				S_UpdateBackgroundTrack_Actual( pMusicInfoFadeOut, qtrue, s_music_volume->value );
				if (pMusicInfoFadeOut->iXFadeVolume == 0)
				{
					pMusicInfoFadeOut->bActive = qfalse;
					alStreamStop();
				}
			}	
		}
	}
	else
	{
		// standard / non-dynamic one-track music...
		//
		LPCSTR psCommand = S_Music_GetRequestedState();	// special check just for "silence" case...
		qboolean bShouldBeSilent = (psCommand && !Q_stricmp(psCommand,"silence"));
		float fDesiredVolume = bShouldBeSilent ? 0.0f : s_music_volume->value;
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

// Called from MusicFree in snd_music to prevent pending state changes from
// crashing after the level finishes loading, but before new music level data
// has been read. Not sure if this fixes the bug, but it seems good.
void S_AvertMusicDisaster(void)
{
	eMusic_StateRequest = eMusic_StateActual;
}


int SND_GetMemoryUsed(void)
{
	ALint used;
	alGeti(AL_MEMORY_USED, &used);
	return used;
}

void SND_update(sfx_t *sfx) 
{
	while ( SND_GetMemoryUsed() > (s_soundpoolmegs->integer * 1024 * 1024 * 3 / 4))
	{
		int iBytesFreed = SND_FreeOldestSound(sfx);
		if (iBytesFreed == 0)
			break;	// sanity
	}
}


// free any allocated sfx mem...
//
// now returns # bytes freed to help with z_malloc()-fail recovery
//
static int SND_FreeSFXMem(sfx_t *sfx)
{
	int iOrgMem = SND_GetMemoryUsed();

	alGetError();
	if (sfx->Buffer)
	{
		alDeleteBuffers(1, &(sfx->Buffer));
		sfx->Buffer = 0;
	}

	sfx->iFlags &= ~(SFX_FLAG_RESIDENT | SFX_FLAG_LOADING);
	sfx->iFlags |= SFX_FLAG_UNLOADED;

	return iOrgMem - SND_GetMemoryUsed();
}

void S_DisplayFreeMemory() 
{
}

void SND_TouchSFX(sfx_t *sfx)
{
	sfx->iLastTimeUsed		= Com_Milliseconds()+1;
}


// currently this is only called during snd_shutdown or snd_restart
//
static void S_FreeAllSFXMem(void)
{
	for (int i = 0; i < MAX_SFX; ++i)
	{
		if (s_sfxCodes[i] != INVALID_CODE && i != s_defaultSound)
		{
			SND_FreeSFXMem(&s_sfxBlock[i]);
		}
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
	bool bDemandLoad = false;

	// start on 1 so we never dump the default sound...
	//
	for (int i = 0; i < MAX_SFX; ++i)
	{
		if (s_sfxCodes[i] == INVALID_CODE || i == s_defaultSound) continue;

		sfx = &s_sfxBlock[i];

		if (sfx != pButNotThisOne)
		{
			// Don't throw out the default sound, sounds that
			// are not in memory, or sounds newer then the oldest.
			// Also, throw out demand load sounds first.
			//
			if (!(sfx->iFlags & SFX_FLAG_DEFAULT) && 
				(sfx->iFlags & SFX_FLAG_RESIDENT) && 
				(!bDemandLoad || (sfx->iFlags & SFX_FLAG_DEMAND)) &&
				sfx->iLastTimeUsed < iOldest) 
			{
				// new bit, we can't throw away any sfx_t struct in use by a channel, 
				// else the paint code will crash...
				//
				int iChannel;
				for (iChannel=0; iChannel<s_numChannels; iChannel++)
				{
					channel_t *ch = & s_channels[iChannel];

					if (ch->thesfx == sfx)
						break;	// damn, being used
				}
				if (iChannel == s_numChannels)
				{
					// this sfx_t struct wasn't used by any channels, so we can lose it...
					//			
					iUsed = i;
					iOldest = sfx->iLastTimeUsed;
					bDemandLoad = (sfx->iFlags & SFX_FLAG_DEMAND);
				}
			}
		}
	}

	if (iUsed)
	{
		sfx = &s_sfxBlock[ iUsed ];
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
qboolean SND_RegisterAudio_Clean(void)
{
	if ( !s_soundStarted ) {
		return qfalse;
	}

	qboolean bAtLeastOneSoundDropped = qfalse;

	Com_DPrintf( "SND_RegisterAudio_Clean():\n");

	extern void S_DrainRawSoundData(void);
	S_DrainRawSoundData();

	{
		for (int i = 0;	i < MAX_SFX; ++i)
		{
			if (s_sfxCodes[i] == INVALID_CODE || i == s_defaultSound) continue;

			sfx_t *sfx = &s_sfxBlock[i];

			if (sfx->iFlags & (SFX_FLAG_RESIDENT | SFX_FLAG_DEMAND))
			{
				qboolean bDeleteThis = qtrue;
				//if (bDeleteThis)
				{
					int iChannel;
					for (iChannel=0; iChannel<s_numChannels; iChannel++)
					{
						if (s_channels[iChannel].thesfx == sfx)
						{
							bDeleteThis = false;
							break;
						}
					}
					
					if (bDeleteThis)
					{
						if (!(sfx->iFlags & SFX_FLAG_DEFAULT) &&
							(sfx->iFlags & SFX_FLAG_RESIDENT) && 
							SND_FreeSFXMem(sfx))
						{
							bAtLeastOneSoundDropped = qtrue;
						}
						if (sfx->iFlags & SFX_FLAG_DEMAND)
						{
							s_sfxCodes[i] = INVALID_CODE;
						}
					}
				}
			}
		}
	}

	Com_DPrintf( "SND_RegisterAudio_Clean(): Ok\n");	

	return bAtLeastOneSoundDropped;
}

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel /* 99% qfalse */)
{
        return qfalse;
}

qboolean S_FileExists( const char *psFilename )
{
	// This is only really used for music. Need to swap .mp3 with .wxb on Xbox
	char *fixedName = S_FixMusicFileName(psFilename);

	// VVFIXME : This can be done better?
	fileHandle_t fhTemp;

	FS_FOpenFileRead (fixedName, &fhTemp, qtrue);	// qtrue so I can fclose the handle without closing a PAK
	if (!fhTemp) 
		return qfalse;
	
	FS_FCloseFile(fhTemp);
	return qtrue;
}

