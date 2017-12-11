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

// snd_mem.c: sound caching

#include "snd_local.h"
#include "snd_mp3.h"
#include "snd_ambient.h"

#include <string>

#ifdef USE_OPENAL
// Open AL
void S_PreProcessLipSync(sfx_t *sfx);
extern int s_UseOpenAL;
#endif
/*
===============================================================================

WAV loading

===============================================================================
*/

byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;
extern sfx_t		s_knownSfx[];
extern	int			s_numSfx;

extern cvar_t		*s_lip_threshold_1;
extern cvar_t		*s_lip_threshold_2;
extern cvar_t		*s_lip_threshold_3;
extern cvar_t		*s_lip_threshold_4;

short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = (short)(val + (*(data_p+1)<<8));
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!Q_strncmp((char *)data_p, name, 4))
			return;
	}
}

void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks(void)
{
	char	str[5];

	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Com_Printf ("0x%x : %s (%d)\n", (intptr_t)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !Q_strncmp((char *)data_p+8, "WAVE", 4)))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}


// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;


	return info;
}


/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
void ResampleSfx (sfx_t *sfx, int iInRate, int iInWidth, byte *pData)
{
	int		iOutCount;
	int		iSrcSample;
	float	fStepScale;
	int		i;
	int		iSample;
	unsigned int uiSampleFrac, uiFracStep;	// uiSampleFrac MUST be unsigned, or large samples (eg music tracks) crash

	fStepScale = (float)iInRate / dma.speed;	// this is usually 0.5, 1, or 2

	// When stepscale is > 1 (we're downsampling), we really ought to run a low pass filter on the samples

	iOutCount = (int)(sfx->iSoundLengthInSamples / fStepScale);
	sfx->iSoundLengthInSamples = iOutCount;

	sfx->pSoundData = (short *) SND_malloc( sfx->iSoundLengthInSamples*2 ,sfx );

	sfx->fVolRange	= 0;
	uiSampleFrac	= 0;
	uiFracStep		= (int)(fStepScale*256);

	for (i=0 ; i<sfx->iSoundLengthInSamples ; i++)
	{
		iSrcSample = uiSampleFrac >> 8;
		uiSampleFrac += uiFracStep;
		if (iInWidth == 2) {
			iSample = LittleShort ( ((short *)pData)[iSrcSample] );
		} else {
			iSample = (int)( (unsigned char)(pData[iSrcSample]) - 128) << 8;
		}

		sfx->pSoundData[i] = (short)iSample;

		// work out max vol for this sample...
		//
		if (iSample < 0)
			iSample = -iSample;
		if (sfx->fVolRange < (iSample >> 8) )
		{
			sfx->fVolRange =  iSample >> 8;
		}
	}
}


//=============================================================================


void S_LoadSound_Finalize(wavinfo_t	*info, sfx_t *sfx, byte *data)
{
	float	stepscale	= (float)info->rate / dma.speed;
	int		len			= (int)(info->samples / stepscale);

	len *= info->width;

	sfx->eSoundCompressionMethod = ct_16;
	sfx->iSoundLengthInSamples	 = info->samples;
	ResampleSfx( sfx, info->rate, info->width, data + info->dataofs );
}





// maybe I'm re-inventing the wheel, here, but I can't see any functions that already do this, so...
//
char *Filename_WithoutPath(const char *psFilename)
{
	static char sString[MAX_QPATH];	// !!
	const char *p = strrchr(psFilename,'\\');

  	if (!p++)
		p=psFilename;

	strcpy(sString,p);

	return sString;

}

// returns (eg) "\dir\name" for "\dir\name.bmp"
//
char *Filename_WithoutExt(const char *psFilename)
{
	static char sString[MAX_QPATH];	// !

	strcpy(sString,psFilename);

	char *p = strrchr(sString,'.');
	char *p2= strrchr(sString,'\\');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p && (p2==0 || (p2 && p>p2)))
		*p=0;

	return sString;

}



int iFilesFound;
int iFilesUpdated;
int iErrors;
qboolean qbForceRescan;
qboolean qbForceStereo;
std::string strErrors;

void R_CheckMP3s( const char *psDir )
{
//	Com_Printf(va("Scanning Dir: %s\n",psDir));
	Com_Printf(".");	// stops useful info scrolling off screen

	char	**sysFiles, **dirFiles;
	int		numSysFiles, i, numdirs;

	dirFiles = FS_ListFiles( psDir, "/", &numdirs);
	if (numdirs > 2)
	{
		for (i=2;i<numdirs;i++)
		{
			char	sDirName[MAX_QPATH];
			sprintf(sDirName, "%s\\%s", psDir, dirFiles[i]);
			R_CheckMP3s(sDirName);
		}
	}

	sysFiles = FS_ListFiles( psDir, ".mp3", &numSysFiles );
	for(i=0; i<numSysFiles; i++)
	{
		char	sFilename[MAX_QPATH];
		sprintf(sFilename,"%s\\%s", psDir, sysFiles[i]);

		Com_Printf("%sFound file: %s",!i?"\n":"",sFilename);

		iFilesFound++;

		// read it in...
		//
		byte *pbData = NULL;
		int iSize = FS_ReadFile( sFilename, (void **)&pbData);

		if (pbData)
		{
			id3v1_1* pTAG;

			// do NOT check 'qbForceRescan' here as an opt, because we need to actually fill in 'pTAG' if there is one...
			//
			qboolean qbTagNeedsUpdating = (/* qbForceRescan || */ !MP3_ReadSpecialTagInfo(pbData, iSize, &pTAG))?qtrue:qfalse;

			if (pTAG == NULL || qbTagNeedsUpdating || qbForceRescan)
			{
				Com_Printf(" ( Updating )\n");

				// I need to scan this file to get the volume...
				//
				// For EF1 I used a temp sfx_t struct, but I can't do that now with this new alloc scheme,
				//	I have to ask for it legally, so I'll keep re-using one, and restoring it's name after use.
				//	(slightly dodgy, but works ok if no-one else changes stuff)
				//
				//sfx_t SFX = {0};
				extern sfx_t *S_FindName( const char *name );
				//
				static sfx_t *pSFX = NULL;
				const char sReservedSFXEntrynameForMP3[] = "reserved_for_mp3";	// ( strlen() < MAX_QPATH )

				if (pSFX == NULL)	// once only
				{
					pSFX = S_FindName(sReservedSFXEntrynameForMP3);	// always returns, else ERR_FATAL
				}

				if (MP3_IsValid(sFilename,pbData, iSize, qbForceStereo))
				{
					wavinfo_t info;

					int iRawPCMDataSize = MP3_GetUnpackedSize(sFilename, pbData, iSize, qtrue, qbForceStereo);

					if (iRawPCMDataSize)	// should always be true, unless file is fucked, in which case, stop this conversion process
					{
						float fMaxVol = 128;	// any old default
						int iActualUnpackedSize = iRawPCMDataSize;	// default, override later if not doing music

						if (!qbForceStereo)	// no point for stereo files, which are for music and therefore no lip-sync
						{
							byte *pbUnpackBuffer = (byte *) Z_Malloc( iRawPCMDataSize+10, TAG_TEMP_WORKSPACE, qfalse );	// won't return if fails

							iActualUnpackedSize = MP3_UnpackRawPCM( sFilename, pbData, iSize, pbUnpackBuffer );
							if (iActualUnpackedSize != iRawPCMDataSize)
							{
								Com_Error(ERR_DROP, "******* Whoah! MP3 %s unpacked to %d bytes, but size calc said %d!\n",sFilename,iActualUnpackedSize,iRawPCMDataSize);
							}

							// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
							//
							MP3_FakeUpWAVInfo( sFilename, pbData, iSize, iActualUnpackedSize,
												// these params are all references...
												info.format, info.rate, info.width, info.channels, info.samples, info.dataofs
												);

							S_LoadSound_Finalize(&info, pSFX, pbUnpackBuffer);	// all this just for lipsynch. Oh well.

							fMaxVol = pSFX->fVolRange;

							// free sfx->data...
							//
							{
								#ifndef INT_MIN
								#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
								#endif
								//
								pSFX->iLastTimeUsed = INT_MIN;		// force this to be oldest sound file, therefore disposable...
								pSFX->bInMemory = qtrue;
								SND_FreeOldestSound();		// ... and do the disposal

								// now set our temp SFX struct back to default name so nothing else accidentally uses it...
								//
								strcpy(pSFX->sSoundName, sReservedSFXEntrynameForMP3);
								pSFX->bDefaultSound = qfalse;
							}

//							Com_OPrintf("File: \"%s\"   MaxVol %f\n",sFilename,pSFX->fVolRange);

							// other stuff...
							//
							Z_Free(pbUnpackBuffer);
						}

						// well, time to update the file now...
						//
						fileHandle_t f = FS_FOpenFileWrite( sFilename );
						if (f)
						{
							// write the file back out, but omitting the tag if there was one...
							//
							int iWritten = FS_Write(pbData, iSize-(pTAG?sizeof(*pTAG):0), f);

							if (iWritten)
							{
								// make up a new tag if we didn't find one in the original file...
								//
								id3v1_1 TAG;
								if (!pTAG)
								{
									pTAG = &TAG;
									memset(&TAG,0,sizeof(TAG));
									strncpy(pTAG->id,"TAG",3);
								}

								strncpy(pTAG->title,	Filename_WithoutPath(Filename_WithoutExt(sFilename)), sizeof(pTAG->title));
								strncpy(pTAG->artist,	"Raven Software",						sizeof(pTAG->artist)	);
								strncpy(pTAG->year,		"2002",									sizeof(pTAG->year)		);
								strncpy(pTAG->comment,	va("%s %g",sKEY_MAXVOL,fMaxVol),		sizeof(pTAG->comment)	);
								strncpy(pTAG->album,	va("%s %d",sKEY_UNCOMP,iActualUnpackedSize),sizeof(pTAG->album)	);

								if (FS_Write( pTAG, sizeof(*pTAG), f ))	// NZ = success
								{
									iFilesUpdated++;
								}
								else
								{
									Com_Printf("*********** Failed write to file \"%s\"!\n",sFilename);
									iErrors++;
									strErrors += va("Failed to write: \"%s\"\n",sFilename);
								}
							}
							else
							{
								Com_Printf("*********** Failed write to file \"%s\"!\n",sFilename);
								iErrors++;
								strErrors += va("Failed to write: \"%s\"\n",sFilename);
							}
							FS_FCloseFile( f );
						}
						else
						{
							Com_Printf("*********** Failed to re-open for write \"%s\"!\n",sFilename);
							iErrors++;
							strErrors += va("Failed to re-open for write: \"%s\"\n",sFilename);
						}
					}
					else
					{
						Com_Error(ERR_DROP, "******* This MP3 should be deleted: \"%s\"\n",sFilename);
					}
				}
				else
				{
					Com_Printf("*********** File was not a valid MP3!: \"%s\"\n",sFilename);
					iErrors++;
					strErrors += va("Not game-legal MP3 format: \"%s\"\n",sFilename);
				}
			}
			else
			{
				Com_Printf(" ( OK )\n");
			}

			FS_FreeFile( pbData );
		}
	}
	FS_FreeFileList( sysFiles );
	FS_FreeFileList( dirFiles );
}

// this console-function is for development purposes, and makes sure that sound/*.mp3 /s have tags in them
//	specifying stuff like their max volume (and uncompressed size) etc...
//
void S_MP3_CalcVols_f( void )
{
	char sStartDir[MAX_QPATH] = {"sound"};
	const char sUsage[] = "Usage: mp3_calcvols [-rescan] <startdir>\ne.g. mp3_calcvols sound/chars";

	if (Cmd_Argc() == 1 || Cmd_Argc()>4)	// 3 optional arguments
	{
		Com_Printf(sUsage);
		return;
	}

	S_StopAllSounds();


	qbForceRescan = qfalse;
	qbForceStereo = qfalse;
	iFilesFound		= 0;
	iFilesUpdated	= 0;
	iErrors			= 0;
	strErrors		= "";

	for (int i=1; i<Cmd_Argc(); i++)
	{
		if (Cmd_Argv(i)[0] == '-')
		{
			if (!Q_stricmp(Cmd_Argv(i),"-rescan"))
			{
				qbForceRescan = qtrue;
			}
			else
			if (!Q_stricmp(Cmd_Argv(i),"-stereo"))
			{
				qbForceStereo = qtrue;
			}
			else
			{
				// unknown switch...
				//
				Com_Printf(sUsage);
				return;
			}
			continue;
		}
		strcpy(sStartDir,Cmd_Argv(i));
	}

	Com_Printf(va("Starting Scan for Updates in Dir: %s\n",sStartDir));
	R_CheckMP3s( sStartDir );

	Com_Printf("\n%d files found/scanned, %d files updated      ( %d errors total)\n",iFilesFound,iFilesUpdated,iErrors);

	if (iErrors)
	{
		Com_Printf("\nBad Files:\n%s\n",strErrors.c_str());
	}
}





// adjust filename for foreign languages and WAV/MP3 issues.
//
// returns qfalse if failed to load, else fills in *pData
//
extern	cvar_t	*com_buildScript;
static qboolean S_LoadSound_FileLoadAndNameAdjuster(char *psFilename, byte **pData, int *piSize, int iNameStrlen)
{
	char *psVoice = strstr(psFilename,"chars");
	if (psVoice)
	{
		// cache foreign voices...
		//
		if (com_buildScript->integer)
		{
			fileHandle_t hFile;
			//German
			strncpy(psVoice,"chr_d",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//French
			strncpy(psVoice,"chr_f",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			//Spanish
			strncpy(psVoice,"chr_e",5);	// same number of letters as "chars"
			FS_FOpenFileRead(psFilename, &hFile, qfalse);		//cache the wav
			if (!hFile)
			{
				strcpy(&psFilename[iNameStrlen-3],"mp3");		//not there try mp3
				FS_FOpenFileRead(psFilename, &hFile, qfalse);	//cache the mp3
			}
			if (hFile)
			{
				FS_FCloseFile(hFile);
			}
			strcpy(&psFilename[iNameStrlen-3],"wav");	//put it back to wav

			strncpy(psVoice,"chars",5);	//put it back to chars
		}

		// account for foreign voices...
		//
		extern cvar_t* s_language;
		if ( s_language ) {
				 if ( !Q_stricmp( "DEUTSCH", s_language->string ) )
				strncpy( psVoice, "chr_d", 5 );	// same number of letters as "chars"
			else if ( !Q_stricmp( "FRANCAIS", s_language->string ) )
				strncpy( psVoice, "chr_f", 5 );	// same number of letters as "chars"
			else if ( !Q_stricmp( "ESPANOL", s_language->string ) )
				strncpy( psVoice, "chr_e", 5 );	// same number of letters as "chars"
			else
				psVoice = NULL;
		}
		else
		{
			psVoice = NULL;	// use this ptr as a flag as to whether or not we substituted with a foreign version
		}
	}

	*piSize = FS_ReadFile( psFilename, (void **)pData );	// try WAV
	if ( !*pData ) {
		psFilename[iNameStrlen-3] = 'm';
		psFilename[iNameStrlen-2] = 'p';
		psFilename[iNameStrlen-1] = '3';
		*piSize = FS_ReadFile( psFilename, (void **)pData );	// try MP3

		if ( !*pData )
		{
			//hmmm, not found, ok, maybe we were trying a foreign noise ("arghhhhh.mp3" that doesn't matter?) but it
			// was missing?   Can't tell really, since both types are now in sound/chars. Oh well, fall back to English for now...

			if (psVoice)	// were we trying to load foreign?
			{
				// yep, so fallback to re-try the english...
				//
#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW "Foreign file missing: \"%s\"! (using English...)\n",psFilename);
#endif

				strncpy(psVoice,"chars",5);

				psFilename[iNameStrlen-3] = 'w';
				psFilename[iNameStrlen-2] = 'a';
				psFilename[iNameStrlen-1] = 'v';
				*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English WAV
				if ( !*pData )
				{
					psFilename[iNameStrlen-3] = 'm';
					psFilename[iNameStrlen-2] = 'p';
					psFilename[iNameStrlen-1] = '3';
					*piSize = FS_ReadFile( psFilename, (void **)pData );	// try English MP3
				}
			}

			if (!*pData)
			{
				return qfalse;	// sod it, give up...
			}
		}
	}

	return qtrue;
}

// returns qtrue if this dir is allowed to keep loaded MP3s, else qfalse if they should be WAV'd instead...
//
// note that this is passed the original, un-language'd name
//
// (I was going to remove this, but on kejim_post I hit an assert because someone had got an ambient sound when the
//	perimter fence goes online that was an MP3, then it tried to get added as looping. Presumably it sounded ok or
//	they'd have noticed, but we therefore need to stop other levels using those. "sound/ambience" I can check for,
//	but doors etc could be anything. Sigh...)
//
#define SOUND_CHARS_DIR "sound/chars/"
#define SOUND_CHARS_DIR_LENGTH 12 // strlen( SOUND_CHARS_DIR )
static qboolean S_LoadSound_DirIsAllowedToKeepMP3s(const char *psFilename)
{
	if ( Q_stricmpn( psFilename, SOUND_CHARS_DIR, SOUND_CHARS_DIR_LENGTH ) == 0 )
		return qtrue;	// found a dir that's allowed to keep MP3s

	return qfalse;
}

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound	(or of a wav/mp3 substitution now -Ste)
==============
*/
qboolean gbInsideLoadSound = qfalse;
static qboolean S_LoadSound_Actual( sfx_t *sfx )
{
	byte	*data;
	short	*samples;
	wavinfo_t	info;
	int		size;
	char	*psExt;
	char	sLoadName[MAX_QPATH];

	int		len = strlen(sfx->sSoundName);
	if (len<5)
	{
		return qfalse;
	}

	// player specific sounds are never directly loaded...
	//
	if ( sfx->sSoundName[0] == '*') {
		return qfalse;
	}
	// make up a local filename to try wav/mp3 substitutes...
	//
	Q_strncpyz(sLoadName, sfx->sSoundName, sizeof(sLoadName));
	Q_strlwr( sLoadName );
	//
	// Ensure name has an extension (which it must have, but you never know), and get ptr to it...
	//
	psExt = &sLoadName[strlen(sLoadName)-4];
	if (*psExt != '.')
	{
		//Com_Printf( "WARNING: soundname '%s' does not have 3-letter extension\n",sLoadName);
		COM_DefaultExtension(sLoadName,sizeof(sLoadName),".wav");	// so psExt below is always valid
		psExt = &sLoadName[strlen(sLoadName)-4];
		len = strlen(sLoadName);
	}

	if (!S_LoadSound_FileLoadAndNameAdjuster(sLoadName, &data, &size, len))
	{
		return qfalse;
	}

	SND_TouchSFX(sfx);

//=========
	if (Q_stricmpn(psExt,".mp3",4)==0)
	{
		// load MP3 file instead...
		//
		if (MP3_IsValid(sLoadName,data, size, qfalse))
		{
			int iRawPCMDataSize = MP3_GetUnpackedSize(sLoadName,data,size,qfalse,qfalse);

			if (S_LoadSound_DirIsAllowedToKeepMP3s(sfx->sSoundName)	// NOT sLoadName, this uses original un-languaged name
				&&
				MP3Stream_InitFromFile(sfx, data, size, sLoadName, iRawPCMDataSize + 2304 /* + 1 MP3 frame size, jic */,qfalse)
				)
			{
//				Com_DPrintf("(Keeping file \"%s\" as MP3)\n",sLoadName);

#ifdef USE_OPENAL
				if (s_UseOpenAL)
				{
					// Create space for lipsync data (4 lip sync values per streaming AL buffer)
					if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
						sfx->lipSyncData = (char *)Z_Malloc(16, TAG_SND_RAWDATA, qfalse);
					else
						sfx->lipSyncData = NULL;
				}
#endif
			}
			else
			{
				// small file, not worth keeping as MP3 since it would increase in size (with MP3 header etc)...
				//
				Com_DPrintf("S_LoadSound: Unpacking MP3 file(%i) \"%s\" to wav(%i).\n",size,sLoadName,iRawPCMDataSize);
				//
				// unpack and convert into WAV...
				//
				{
					byte *pbUnpackBuffer = (byte *) Z_Malloc( iRawPCMDataSize+10 +2304 /* <g> */, TAG_TEMP_WORKSPACE, qfalse );	// won't return if fails

					{
						int iResultBytes = MP3_UnpackRawPCM( sLoadName, data, size, pbUnpackBuffer, qfalse );

						if (iResultBytes!= iRawPCMDataSize){
							Com_Printf(S_COLOR_YELLOW"**** MP3 %s final unpack size %d different to previous value %d\n",sLoadName,iResultBytes,iRawPCMDataSize);
							//assert (iResultBytes == iRawPCMDataSize);
						}


						// fake up a WAV structure so I can use the other post-load sound code such as volume calc for lip-synching
						//
						// (this is a bit crap really, but it lets me drop through into existing code)...
						//
						MP3_FakeUpWAVInfo( sLoadName, data, size, iResultBytes,
											// these params are all references...
											info.format, info.rate, info.width, info.channels, info.samples, info.dataofs,
											qfalse
										);

						S_LoadSound_Finalize(&info,sfx,pbUnpackBuffer);

#ifdef Q3_BIG_ENDIAN
						// the MP3 decoder returns the samples in the correct endianness, but ResampleSfx byteswaps them,
						// so we have to swap them again...
						sfx->fVolRange	= 0;

						for (int i = 0; i < sfx->iSoundLengthInSamples; i++)
						{
							sfx->pSoundData[i] = LittleShort(sfx->pSoundData[i]);
							// C++11 defines double abs(short) which is not what we want here,
							// because double >> int is not defined. Force interpretation as int
							if (sfx->fVolRange < (abs(static_cast<int>(sfx->pSoundData[i])) >> 8))
							{
								sfx->fVolRange = abs(static_cast<int>(sfx->pSoundData[i])) >> 8;
							}
						}
#endif

						// Open AL
#ifdef USE_OPENAL
						if (s_UseOpenAL)
						{
							if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
							{
								sfx->lipSyncData = (char *)Z_Malloc((sfx->iSoundLengthInSamples / 1000) + 1, TAG_SND_RAWDATA, qfalse);
								S_PreProcessLipSync(sfx);
							}
							else
								sfx->lipSyncData = NULL;

							// Clear Open AL Error state
							alGetError();

							// Generate AL Buffer
                            ALuint Buffer;
							alGenBuffers(1, &Buffer);
							if (alGetError() == AL_NO_ERROR)
							{
								// Copy audio data to AL Buffer
								alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
								if (alGetError() == AL_NO_ERROR)
								{
									sfx->Buffer = Buffer;
									Z_Free(sfx->pSoundData);
									sfx->pSoundData = NULL;
								}
							}
						}
#endif

						Z_Free(pbUnpackBuffer);
					}
				}
			}
		}
		else
		{
			// MP3_IsValid() will already have printed any errors via Com_Printf at this point...
			//
			FS_FreeFile (data);
			return qfalse;
		}
	}
	else
	{
		// loading a WAV, presumably...

//=========

		info = GetWavinfo( sLoadName, data, size );
		if ( info.channels != 1 ) {
			Com_Printf ("%s is a stereo wav file\n", sLoadName);
			FS_FreeFile (data);
			return qfalse;
		}

/*		if ( info.width == 1 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is a 8 bit wav file\n", sLoadName);
		}

		if ( info.rate != 22050 ) {
			Com_Printf(S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n", sLoadName);
		}
*/
		samples = (short *)Z_Malloc(info.samples * sizeof(short) * 2, TAG_TEMP_WORKSPACE, qfalse);

		sfx->eSoundCompressionMethod = ct_16;
		sfx->iSoundLengthInSamples	 = info.samples;
		sfx->pSoundData = NULL;
		ResampleSfx( sfx, info.rate, info.width, data + info.dataofs );

		// Open AL
#ifdef USE_OPENAL
		if (s_UseOpenAL)
		{
			if ((strstr(sfx->sSoundName, "chars")) || (strstr(sfx->sSoundName, "CHARS")))
			{
				sfx->lipSyncData = (char *)Z_Malloc((sfx->iSoundLengthInSamples / 1000) + 1, TAG_SND_RAWDATA, qfalse);
				S_PreProcessLipSync(sfx);
			}
			else
				sfx->lipSyncData = NULL;

			// Clear Open AL Error State
			alGetError();

			// Generate AL Buffer
            ALuint Buffer;
			alGenBuffers(1, &Buffer);
			if (alGetError() == AL_NO_ERROR)
			{
				// Copy audio data to AL Buffer
				alBufferData(Buffer, AL_FORMAT_MONO16, sfx->pSoundData, sfx->iSoundLengthInSamples*2, 22050);
				if (alGetError() == AL_NO_ERROR)
				{
					// Store AL Buffer in sfx struct, and release sample data
					sfx->Buffer = Buffer;
					Z_Free(sfx->pSoundData);
					sfx->pSoundData = NULL;
				}
			}
		}
#endif

		Z_Free(samples);
	}

	FS_FreeFile( data );

	return qtrue;
}


// wrapper function for above so I can guarantee that we don't attempt any audio-dumping during this call because
//	of a z_malloc() fail recovery...
//
qboolean S_LoadSound( sfx_t *sfx )
{
	gbInsideLoadSound = qtrue;	// !!!!!!!!!!!!!

		qboolean bReturn = S_LoadSound_Actual( sfx );

	gbInsideLoadSound = qfalse;	// !!!!!!!!!!!!!

	return bReturn;
}

#ifdef USE_OPENAL
/*
	Precalculate the lipsync values for the whole sample
*/
void S_PreProcessLipSync(sfx_t *sfx)
{
	int i, j;
	int sample;
	int sampleTotal = 0;

	j = 0;
	for (i = 0; i < sfx->iSoundLengthInSamples; i += 100)
	{
		sample = LittleShort(sfx->pSoundData[i]);

		sample = sample >> 8;
		sampleTotal += sample * sample;
		if (((i + 100) % 1000) == 0)
		{
			sampleTotal /= 10;

			if (sampleTotal < sfx->fVolRange *  s_lip_threshold_1->value)
			{
				// tell the scripts that are relying on this that we are still going, but actually silent right now.
				sample = -1;
			}
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_2->value)
				sample = 1;
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_3->value)
				sample = 2;
			else if (sampleTotal < sfx->fVolRange * s_lip_threshold_4->value)
				sample = 3;
			else
				sample = 4;

			sfx->lipSyncData[j] = sample;
			j++;

			sampleTotal = 0;
		}
	}

	if ((i % 1000) == 0)
		return;

	i -= 100;
	i = i % 1000;
	i = i / 100;
	// Process last < 1000 samples
	if (i != 0)
		sampleTotal /= i;
	else
		sampleTotal = 0;

	if (sampleTotal < sfx->fVolRange * s_lip_threshold_1->value)
	{
		// tell the scripts that are relying on this that we are still going, but actually silent right now.
		sample = -1;
	}
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_2->value)
		sample = 1;
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_3->value)
		sample = 2;
	else if (sampleTotal < sfx->fVolRange * s_lip_threshold_4->value)
		sample = 3;
	else
		sample = 4;

	sfx->lipSyncData[j] = sample;
}
#endif
