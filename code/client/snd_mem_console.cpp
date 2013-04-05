// snd_mem.c: sound caching


// leave this as first line for PCH reasons...
//
// #include "../server/exe_headers.h"

#include "snd_local_console.h"

#ifdef _XBOX
#include <xtl.h>
#endif

#define SND_MAX_LOADS 48
static sfx_t** s_LoadList = NULL;
static int s_LoadListSize = 0;
qboolean gbInsideLoadSound = qfalse;	// Needed to link VVFIXME

extern int Sys_GetFileCode(const char *name);

//Drain sound main memory into ARAM.
void S_DrainRawSoundData(void)
{
	extern int s_soundStarted;
	if (!s_soundStarted) return;

	do
	{
		S_UpdateLoading();

#ifdef _GAMECUBE
		extern void ERR_DiscFail(bool);
		ERR_DiscFail(true);
#endif
	} 
	while (s_LoadListSize);
}



/*
============
GetWavInfo
============
*/
wavinfo_t GetWavInfo(byte *data)
{
	wavinfo_t info;
	memset(&info, 0, sizeof(wavinfo_t));

	if (!data) return info;

#ifdef _GAMECUBE
	if (*(short*)&data[14] != 0)
	{
		// invalid type, abort
		return info;
	}

	info.format = AL_FORMAT_MONO4;
	info.width = 4;
	info.size = ((*(int*)&data[20]) >> 1) + 96;
	info.rate = *(int*)&data[8];
#else
	int dataofs = 0;
	if (strncmp((char *)&data[dataofs + 0], "RIFF", 4) ||
		strncmp((char *)&data[dataofs + 8], "WAVE", 4))
	{
		// invalid type, abort
		return info;
	}
	dataofs += 12; // done with riff chunk

	WAVEFORMATEX* wav = (WAVEFORMATEX*)&data[dataofs + 8];
	info.format = wav->nChannels == 1 ? AL_FORMAT_MONO4 : AL_FORMAT_STEREO4;
	info.rate = wav->nSamplesPerSec;
	info.width = wav->wBitsPerSample;
	dataofs += sizeof(WAVEFORMATEX) + wav->cbSize + 8; // done with fmt chunk
	
	info.size = *(int*)&data[dataofs + 4];
	info.samples = (info.size * 2);

	dataofs += 8; // done with data chunk
#endif

	return info;
}

// adjust filename for foreign languages and WAV/MP3 issues. 
//
static qboolean S_LoadSound_FileNameAdjuster(char *psFilename)
{
#if defined(_XBOX)
	const char* ext = "wxb";
#elif defined(_WINDOWS)
	const char* ext = "wav";
#elif defined(_GAMECUBE)
	const char* ext = "wgc";
#endif
	
	int len = strlen(psFilename);
#if 0
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice)
	{
		// account for foreign voices...
		//		
		extern cvar_t* sp_language;
		if (sp_language && sp_language->integer==SP_LANGUAGE_GERMAN)
		{				
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
		}
		else
		if (sp_language && sp_language->integer==SP_LANGUAGE_FRENCH)
		{				
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
		}
		else
		{
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}
#else
	char *psVoice = NULL;
#endif

	psFilename[len-3] = ext[0];
	psFilename[len-2] = ext[1];
	psFilename[len-1] = ext[2];
	int code = Sys_GetFileCode( psFilename );

	if ( code == -1 )
	{
		//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
		// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...
		
		if (psVoice)	// were we trying to load foreign?
		{
			// yep, so fallback to re-try the english...
			//
			strncpy(psVoice,"chars",5);
			
			psFilename[len-3] = ext[0];
			psFilename[len-2] = ext[1];
			psFilename[len-1] = ext[2];
			code = Sys_GetFileCode( psFilename );
		}
	}

	return code;
}

/*
==============
S_GetFileCode
==============
*/
int S_GetFileCode( const char* sSoundName )
{
	char	sLoadName[MAX_QPATH];

	// make up a local filename to try wav/mp3 substitutes...
	//	
	Q_strncpyz(sLoadName, sSoundName, sizeof(sLoadName));	
	Q_strlwr( sLoadName );

	// make sure we have an extension...
	//
	if (sLoadName[strlen(sLoadName) - 4] != '.')
	{
		strcat(sLoadName, ".xxx");
	}

	return S_LoadSound_FileNameAdjuster(sLoadName);
}

/*
============
S_UpdateLoading
============
*/
void S_UpdateLoading(void) {
	for ( int i = 0; i < SND_MAX_LOADS; ++i )
	{
		if ( s_LoadList[i] &&
			(s_LoadList[i]->iFlags & SFX_FLAG_LOADING) &&
			!Sys_StreamIsReading(s_LoadList[i]->iStreamHandle) )
		{
			S_EndLoadSound(s_LoadList[i]);
			s_LoadList[i] = NULL;
			--s_LoadListSize;
		}
	}
}

/*
==============
S_BeginLoadSound
==============
*/
qboolean S_StartLoadSound( sfx_t *sfx )
{
	assert(sfx->iFlags & SFX_FLAG_UNLOADED);
	sfx->iFlags &= ~SFX_FLAG_UNLOADED;
	
	// Valid file?
	if (sfx->iFileCode == -1)
	{
		sfx->iFlags |= SFX_FLAG_RESIDENT | SFX_FLAG_DEFAULT;
		return qfalse;
	}

	// Finish up any pending loads
	do
	{
		S_UpdateLoading();
	} 
	while (s_LoadListSize >= SND_MAX_LOADS);

	// Open the file
	sfx->iSoundLength = Sys_StreamOpen(sfx->iFileCode, &sfx->iStreamHandle);
	if ( sfx->iSoundLength <= 0 )
	{
		sfx->iFlags |= SFX_FLAG_RESIDENT | SFX_FLAG_DEFAULT;
		return qfalse;
	}

#ifdef _GAMECUBE
	// Allocate a buffer to read into...
	sfx->pSoundData = Z_Malloc(sfx->iSoundLength + 64, TAG_SND_RAWDATA,
			qtrue, 32);
#else
	// Allocate a buffer to read into...
	sfx->pSoundData = Z_Malloc(sfx->iSoundLength, TAG_SND_RAWDATA, qtrue, 32);
#endif

	// Setup the background read
	if ( !sfx->pSoundData || 
			!Sys_StreamRead(sfx->pSoundData, sfx->iSoundLength, 0, 
				sfx->iStreamHandle) )
	{
		if(sfx->pSoundData) {
			Z_Free(sfx->pSoundData);
		}
		Sys_StreamClose(sfx->iStreamHandle);
		sfx->iFlags |= SFX_FLAG_RESIDENT | SFX_FLAG_DEFAULT;
		return qfalse;
	}
	sfx->iFlags |= SFX_FLAG_LOADING;

	// add sound to load list
	for (int i = 0; i < SND_MAX_LOADS; ++i)
	{
		if (!s_LoadList[i])
		{
			s_LoadList[i] = sfx;
			++s_LoadListSize;
			break;
		}
	}

	return qtrue;
}

/*
==============
S_EndLoadSound
==============
*/
qboolean S_EndLoadSound( sfx_t *sfx )
{
	wavinfo_t	info;
	byte*		data;
	ALuint		Buffer;

	assert(sfx->iFlags & SFX_FLAG_LOADING);
	sfx->iFlags &= ~SFX_FLAG_LOADING;

	// was the read successful?
	if (Sys_StreamIsError(sfx->iStreamHandle))
	{
#if defined(FINAL_BUILD)
		/*
		extern void ERR_DiscFail(bool);
		ERR_DiscFail(false);
		*/
#endif
		Sys_StreamClose(sfx->iStreamHandle);
		Z_Free(sfx->pSoundData);
		sfx->iFlags |= SFX_FLAG_RESIDENT | SFX_FLAG_DEFAULT;
		return qfalse;
	}

	Sys_StreamClose(sfx->iStreamHandle);
	SND_TouchSFX(sfx);

	sfx->iLastTimeUsed = Com_Milliseconds()+1;	// why +1? Hmmm, leave it for now I guess	

	// loading a WAV, presumably...
	data = (byte*)sfx->pSoundData;
	info = GetWavInfo( data );

	if (info.size == 0)
	{
		Z_Free(sfx->pSoundData);
		sfx->iFlags |= SFX_FLAG_RESIDENT | SFX_FLAG_DEFAULT;
		return qfalse;
	}
	
	sfx->iSoundLength = info.size;

	// make sure we have enough space for the sound
	SND_update(sfx);

	// Clear Open AL Error State
	alGetError();

	// Generate AL Buffer
	alGenBuffers(1, &Buffer);

	// Copy audio data to AL Buffer
	alBufferData(Buffer, info.format, data, 
		sfx->iSoundLength, info.rate);
	if (alGetError() != AL_NO_ERROR)
	{
		Z_Free(sfx->pSoundData);
		sfx->iFlags |= SFX_FLAG_UNLOADED;
		return qfalse;
	}
	
	sfx->Buffer = Buffer;

#ifdef _GAMECUBE
	Z_Free(sfx->pSoundData);
#endif
	sfx->iFlags |= SFX_FLAG_RESIDENT;

	return qtrue;
}

/*
============
S_InitLoad
============
*/
void S_InitLoad(void) {
	s_LoadList = new sfx_t*[SND_MAX_LOADS];
	memset(s_LoadList, 0, SND_MAX_LOADS * sizeof(sfx_t*));
	s_LoadListSize = 0;
}

/*
============
S_CloseLoad
============
*/
void S_CloseLoad(void) {
	delete [] s_LoadList;
}
