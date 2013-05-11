#pragma once

// Filename:-	cl_mp3.h
//
// (Interface to the rest of the game for the MP3 functions)
//

#ifndef sfx_t
#include "snd_local.h"
#endif



typedef struct id3v1_1 {
    char id[3];
    char title[30];		// <file basename>
    char artist[30];	// "Raven Software"
    char album[30];		// "#UNCOMP %d"		// needed
    char year[4];		// "2000"
    char comment[28];	// "#MAXVOL %g"		// needed
    char zero;
    char track;
    char genre;
} id3v1_1;	// 128 bytes in size


extern const char sKEY_MAXVOL[];
extern const char sKEY_UNCOMP[];


// (so far, all these functions are only called from one place in snd_mem.cpp)
//
// (filenames are used purely for error reporting, all files should already be loaded before you get here)
//
void		MP3_InitCvars			( void );
sboolean	MP3_IsValid				( const char *psLocalFilename, void *pvData, int iDataLen, sboolean bStereoDesired = qfalse );
int			MP3_GetUnpackedSize		( const char *psLocalFilename, void *pvData, int iDataLen, sboolean qbIgnoreID3Tag = qfalse, sboolean bStereoDesired = qfalse );
sboolean	MP3_UnpackRawPCM		( const char *psLocalFilename, void *pvData, int iDataLen, byte *pbUnpackBuffer, sboolean bStereoDesired = qfalse );
sboolean	MP3Stream_InitPlayingTimeFields( LP_MP3STREAM lpMP3Stream, const char *psLocalFilename, void *pvData, int iDataLen, sboolean bStereoDesired = qfalse);
float		MP3Stream_GetPlayingTimeInSeconds( LP_MP3STREAM lpMP3Stream );
float		MP3Stream_GetRemainingTimeInSeconds( LP_MP3STREAM lpMP3Stream );
sboolean	MP3_FakeUpWAVInfo		( const char *psLocalFilename, void *pvData, int iDataLen, int iUnpackedDataLength, int &format, int &rate, int &width, int &channels, int &samples, int &dataofs, sboolean bStereoDesired = qfalse );
sboolean	MP3_ReadSpecialTagInfo	( byte *pbLoadedFile, int iLoadedFileLen,
										id3v1_1** ppTAG = NULL, int *piUncompressedSize = NULL, float *pfMaxVol = NULL);
sboolean	MP3Stream_InitFromFile	( sfx_t* sfx, byte *pbSrcData, int iSrcDatalen, const char *psSrcDataFilename, int iMP3UnPackedSize, sboolean bStereoDesired = qfalse );
int			MP3Stream_Decode		( LP_MP3STREAM lpMP3Stream,  sboolean bDoingMusic );
sboolean	MP3Stream_SeekTo		( channel_t *ch, float fTimeToSeekTo );
sboolean	MP3Stream_Rewind		( channel_t *ch );
sboolean	MP3Stream_GetSamples	( channel_t *ch, int startingSampleNum, int count, short *buf, sboolean bStereo );





///////////////////////////////////////
//
// the real worker code deep down in the MP3 C code...  (now externalised here so the music streamer can access one)
//
#ifdef __cplusplus
extern "C"
{
#endif


char*	C_MP3_IsValid			(void *pvData, int iDataLen, int bStereoDesired);
char*	C_MP3_GetUnpackedSize	(void *pvData, int iDataLen, int *piUnpackedSize, int bStereoDesired);
char*	C_MP3_UnpackRawPCM		(void *pvData, int iDataLen, int *piUnpackedSize, void *pbUnpackBuffer, int bStereoDesired);
char*	C_MP3_GetHeaderData		(void *pvData, int iDataLen, int *piRate, int *piWidth, int *piChannels, int bStereoDesired);
char*	C_MP3Stream_DecodeInit	(LP_MP3STREAM pSFX_MP3Stream, void *pvSourceData, int iSourceBytesRemaining,
								int iGameAudioSampleRate, int iGameAudioSampleBits, int bStereoDesired);
unsigned int C_MP3Stream_Decode( LP_MP3STREAM pSFX_MP3Stream, int bFastForwarding );
char*	C_MP3Stream_Rewind		(LP_MP3STREAM pSFX_MP3Stream);


#ifdef __cplusplus
}
#endif
//
///////////////////////////////////////


///////////////// eof /////////////////////
