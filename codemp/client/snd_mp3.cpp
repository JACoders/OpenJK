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

// Filename:-	cl_mp3.cpp
//
// (The interface module between all the MP3 stuff and the game)


#include "client.h"
#include "snd_mp3.h"					// only included directly by a few snd_xxxx.cpp files plus this one
#include "mp3code/mp3struct.h"	// keep this rather awful file secret from the rest of the program

// expects data already loaded, filename arg is for error printing only
//
// returns success/fail
//
qboolean MP3_IsValid( const char *psLocalFilename, void *pvData, int iDataLen, qboolean bStereoDesired /* = qfalse */)
{
	char *psError = C_MP3_IsValid(pvData, iDataLen, bStereoDesired);

	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s(%s)\n",psError, psLocalFilename));
	}

	return (qboolean)!psError;
}



// expects data already loaded, filename arg is for error printing only
//
// returns unpacked length, or 0 for errors (which will be printed internally)
//
int MP3_GetUnpackedSize( const char *psLocalFilename, void *pvData, int iDataLen, qboolean qbIgnoreID3Tag /* = qfalse */
						, qboolean bStereoDesired /* = qfalse */
						)
{
	int	iUnpackedSize = 0;

	// always do this now that we have fast-unpack code for measuring output size... (much safer than relying on tags that may have been edited, or if MP3 has been re-saved with same tag)
	//
	if (1)//qbIgnoreID3Tag || !MP3_ReadSpecialTagInfo((byte *)pvData, iDataLen, NULL, &iUnpackedSize))
	{
		char *psError = C_MP3_GetUnpackedSize( pvData, iDataLen, &iUnpackedSize, bStereoDesired);

		if (psError)
		{
			Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n",psError, psLocalFilename));
			return 0;
		}
	}

	return iUnpackedSize;
}



// expects data already loaded, filename arg is for error printing only
//
// returns byte count of unpacked data (effectively a success/fail bool)
//
int MP3_UnpackRawPCM( const char *psLocalFilename, void *pvData, int iDataLen, byte *pbUnpackBuffer, qboolean bStereoDesired /* = qfalse */)
{
	int iUnpackedSize;
	char *psError = C_MP3_UnpackRawPCM( pvData, iDataLen, &iUnpackedSize, pbUnpackBuffer, bStereoDesired);

	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n",psError, psLocalFilename));
		return 0;
	}

	return iUnpackedSize;
}


// psLocalFilename is just for error reporting (if any)...
//
qboolean MP3Stream_InitPlayingTimeFields( LP_MP3STREAM lpMP3Stream, const char *psLocalFilename, void *pvData, int iDataLen, qboolean bStereoDesired /* = qfalse */)
{
	qboolean bRetval = qfalse;

	int iRate, iWidth, iChannels;

	char *psError = C_MP3_GetHeaderData(pvData, iDataLen, &iRate, &iWidth, &iChannels, bStereoDesired );
	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"MP3Stream_InitPlayingTimeFields(): %s\n(File: %s)\n",psError, psLocalFilename));
	}
	else
	{
		int iUnpackLength = MP3_GetUnpackedSize( psLocalFilename, pvData, iDataLen, qfalse,	// qboolean qbIgnoreID3Tag
													bStereoDesired);
		if (iUnpackLength)
		{
			lpMP3Stream->iTimeQuery_UnpackedLength	= iUnpackLength;
			lpMP3Stream->iTimeQuery_SampleRate		= iRate;
			lpMP3Stream->iTimeQuery_Channels		= iChannels;
			lpMP3Stream->iTimeQuery_Width			= iWidth;

			bRetval = qtrue;
		}
	}

	return bRetval;
}

float MP3Stream_GetPlayingTimeInSeconds( LP_MP3STREAM lpMP3Stream )
{
	if (lpMP3Stream->iTimeQuery_UnpackedLength)	// fields initialised?
		return (float)((((double)lpMP3Stream->iTimeQuery_UnpackedLength / (double)lpMP3Stream->iTimeQuery_SampleRate) / (double)lpMP3Stream->iTimeQuery_Channels) / (double)lpMP3Stream->iTimeQuery_Width);

	return 0.0f;
}

float MP3Stream_GetRemainingTimeInSeconds( LP_MP3STREAM lpMP3Stream )
{
	if (lpMP3Stream->iTimeQuery_UnpackedLength)	// fields initialised?
		return (float)(((((double)(lpMP3Stream->iTimeQuery_UnpackedLength - (lpMP3Stream->iBytesDecodedTotal * (lpMP3Stream->iTimeQuery_SampleRate / dma.speed)))) / (double)lpMP3Stream->iTimeQuery_SampleRate) / (double)lpMP3Stream->iTimeQuery_Channels) / (double)lpMP3Stream->iTimeQuery_Width);

	return 0.0f;
}




// expects data already loaded, filename arg is for error printing only
//
qboolean MP3_FakeUpWAVInfo( const char *psLocalFilename, void *pvData, int iDataLen, int iUnpackedDataLength,
						   int &format, int &rate, int &width, int &channels, int &samples, int &dataofs,
						   qboolean bStereoDesired /* = qfalse */
						   )
{
	// some things can be done instantly...
	//
	format = 1;		// 1 for MS format
	dataofs= 0;		// will be 0 for me (since there's no header in the unpacked data)

	// some things need to be read...  (though the whole stereo flag thing is crap)
	//
	char *psError = C_MP3_GetHeaderData(pvData, iDataLen, &rate, &width, &channels, bStereoDesired );
	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n",psError, psLocalFilename));
	}

	// and some stuff needs calculating...
	//
	samples	= iUnpackedDataLength / width;

	return (qboolean)!psError;
}



const char sKEY_MAXVOL[]="#MAXVOL";	// formerly #defines
const char sKEY_UNCOMP[]="#UNCOMP";	//    "        "

// returns qtrue for success...
//
qboolean MP3_ReadSpecialTagInfo(byte *pbLoadedFile, int iLoadedFileLen,
								id3v1_1** ppTAG /* = NULL */,
								int *piUncompressedSize /* = NULL */,
								float *pfMaxVol /* = NULL */
								)
{
	qboolean qbError = qfalse;

	id3v1_1* pTAG = (id3v1_1*) ((pbLoadedFile+iLoadedFileLen)-sizeof(id3v1_1));	// sizeof = 128

	if (!Q_strncmp(pTAG->id, "TAG", 3))
	{
		// TAG found...
		//

		// read MAXVOL key...
		//
		if (Q_strncmp(pTAG->comment, sKEY_MAXVOL,	strlen(sKEY_MAXVOL)))
		{
			qbError = qtrue;
		}
		else
		{
			if ( pfMaxVol)
			{
				*pfMaxVol = atof(pTAG->comment + strlen(sKEY_MAXVOL));
			}
		}

		//
		// read UNCOMP key...
		//
		if (Q_strncmp(pTAG->album, sKEY_UNCOMP, strlen(sKEY_UNCOMP)))
		{
			qbError = qtrue;
		}
		else
		{
			if ( piUncompressedSize)
			{
				*piUncompressedSize = atoi(pTAG->album + strlen(sKEY_UNCOMP));
			}
		}
	}
	else
	{
		pTAG = NULL;
	}

	if (ppTAG)
	{
		*ppTAG = pTAG;
	}

	return (qboolean)(pTAG && !qbError);
}



#define FUZZY_AMOUNT (5*1024)	// so it has to be significantly over, not just break even, because of
								// the xtra CPU time versus memory saving

cvar_t* cv_MP3overhead = NULL;
void MP3_InitCvars(void)
{
	cv_MP3overhead = Cvar_Get("s_mp3overhead", va("%d", sizeof(MP3STREAM) + FUZZY_AMOUNT), CVAR_ARCHIVE );
}


// a file has been loaded in memory, see if we want to keep it as MP3, else as normal WAV...
//
// return = qtrue if keeping as MP3
//
// (note: the reason I pass in the unpacked size rather than working it out here is simply because I already have it)
//
qboolean MP3Stream_InitFromFile( sfx_t* sfx, byte *pbSrcData, int iSrcDatalen, const char *psSrcDataFilename,
									int iMP3UnPackedSize, qboolean bStereoDesired /* = qfalse */
								)
{
	// first, make a decision based on size here as to whether or not it's worth it because of MP3 buffer space
	//	making small files much bigger (and therefore best left as WAV)...
	//

	if (cv_MP3overhead &&
			(
			//iSrcDatalen + sizeof(MP3STREAM) + FUZZY_AMOUNT < iMP3UnPackedSize
			iSrcDatalen + cv_MP3overhead->integer < iMP3UnPackedSize
			)
		)
	{
		// ok, let's keep it as MP3 then...
		//
		float fMaxVol = 128;	// seems to be a reasonable typical default for maxvol (for lip synch). Naturally there's no #define I can use instead...

		MP3_ReadSpecialTagInfo(pbSrcData, iSrcDatalen, NULL, NULL, &fMaxVol );	// try and read a read maxvol from MP3 header

		// fill in some sfx_t fields...
		//
//		Q_strncpyz( sfx->name, psSrcDataFilename, sizeof(sfx->name) );
		sfx->eSoundCompressionMethod = ct_MP3;
		sfx->fVolRange = fMaxVol;
		//sfx->width  = 2;
		sfx->iSoundLengthInSamples = ((iMP3UnPackedSize / 2/*sfx->width*/) / (44100 / dma.speed)) / (bStereoDesired?2:1);
		//
		// alloc mem for data and store it (raw MP3 in this case)...
		//
		sfx->pSoundData = (short *) SND_malloc( iSrcDatalen, sfx );
		memcpy( sfx->pSoundData, pbSrcData, iSrcDatalen );

		// now init the low-level MP3 stuff...
		//
		MP3STREAM SFX_MP3Stream = {};	// important to init to all zeroes!
		char *psError = C_MP3Stream_DecodeInit( &SFX_MP3Stream, /*sfx->data*/ /*sfx->soundData*/ pbSrcData, iSrcDatalen,
												dma.speed,//(s_khz->value == 44)?44100:(s_khz->value == 22)?22050:11025,
												2/*sfx->width*/ * 8,
												bStereoDesired
												);
		SFX_MP3Stream.pbSourceData = (byte *) sfx->pSoundData;
		if (psError)
		{
			// This should never happen, since any errors or problems with the MP3 file would have stopped us getting
			//	to this whole function, but just in case...
			//
			Com_Printf(va(S_COLOR_YELLOW"File \"%s\": %s\n",psSrcDataFilename,psError));

			// This will leave iSrcDatalen bytes on the hunk stack (since you can't dealloc that), but MP3 files are
			//	usually small, and like I say, it should never happen.
			//
			// Strictly speaking, I should do a Z_Malloc above, then I could do a Z_Free if failed, else do a Hunk_Alloc
			//	to copy the Z_Malloc data into, then Z_Free, but for something that shouldn't happen it seemed bad to
			//	penalise the rest of the game with extra alloc demands.
			//
			return qfalse;
		}

		// success ( ...on a plate).
		//
		// make a copy of the filled-in stream struct and attach to the sfx_t struct...
		//
				sfx->pMP3StreamHeader = (MP3STREAM *) Z_Malloc( sizeof(MP3STREAM), TAG_SND_MP3STREAMHDR, qfalse );
		memcpy(	sfx->pMP3StreamHeader, &SFX_MP3Stream,		    sizeof(MP3STREAM) );
		//
		return qtrue;
	}

	return qfalse;
}



// decode one packet of MP3 data only (typical output size is 2304, or 2304*2 for stereo, so input size is less
//
// return is decoded byte count, else 0 for finished
//
int MP3Stream_Decode( LP_MP3STREAM lpMP3Stream, qboolean bDoingMusic )
{
	lpMP3Stream->iCopyOffset = 0;

	if (0)//!bDoingMusic)
	{
		/*
		// SOF2: need to make a local buffer up so we can decode the piece we want from a contiguous bitstream rather than
		//	this linklist junk...
		//
		// since MP3 packets are generally 416 or 417 bytes in length it seems reasonable to just find which linked-chunk
		//	the current read offset lies within then grab the next one as well (since they're 2048 bytes) and make one
		//	buffer with just the two concat'd together. Shouldn't be much of a processor hit.
		//
		sndBuffer *pChunk = (sndBuffer *) lpMP3Stream->pbSourceData;
		//
		// may as well make this static to avoid cut down on stack-validation run-time...
		//
		static byte	byRawBuffer[SND_CHUNK_SIZE_BYTE*2];	// *2 for byte->short	// easily enough to decode one frame of MP3 data, most are 416 or 417 bytes

		// fast-forward to the correct chunk...
		//
		int iBytesToSkipPast = lpMP3Stream->iSourceReadIndex;

		while (iBytesToSkipPast >= SND_CHUNK_SIZE_BYTE)
		{
			pChunk = pChunk->next;
			if (!pChunk)
			{
				// err.... reading off the end of the data stream guys...
				//
				// pChunk = (sndBuffer *) lpMP3Stream->pbSourceData;	// restart
				return 0;	// ... 0 bytes decoded, so will just stop caller-decoder all nice and legal as EOS
			}
			iBytesToSkipPast -= SND_CHUNK_SIZE_BYTE;
		}

		{
			// ok, pChunk is now the 2k or so chunk we're in the middle of...
			//
			int iChunk1BytesToCopy = SND_CHUNK_SIZE_BYTE - iBytesToSkipPast;
			memcpy(byRawBuffer,((byte *)pChunk->sndChunk) + iBytesToSkipPast, iChunk1BytesToCopy);
			//
			// concat next chunk on to this as well...
			//
			pChunk = pChunk->next;
			if (pChunk)
			{
				memcpy(byRawBuffer + iChunk1BytesToCopy, pChunk->sndChunk,	SND_CHUNK_SIZE_BYTE);
			}
			else
			{
				memset(byRawBuffer + iChunk1BytesToCopy, 0,					SND_CHUNK_SIZE_BYTE);
			}
		}


		{
			// now we need to backup some struct fields, fake 'em, do the lo-level call, then restore 'em...
			//
			byte *pbSourceData_Old	= lpMP3Stream->pbSourceData;
			int iSourceReadIndex_Old= lpMP3Stream->iSourceReadIndex;

			lpMP3Stream->pbSourceData	= &byRawBuffer[0];
			lpMP3Stream->iSourceReadIndex= 0;	// since this is zero, not the buffer offset within a chunk, we can play tricks further down when restoring

			{
				unsigned int uiBytesDecoded = C_MP3Stream_Decode( lpMP3Stream, qfalse );

				lpMP3Stream->iSourceReadIndex += iSourceReadIndex_Old;	// note '+=' rather than '=', to take account of movement.
				lpMP3Stream->pbSourceData	   = pbSourceData_Old;

				return uiBytesDecoded;
			}
		}
		*/
	}
	else
	{
		// SOF2 music, or EF1 anything...
		//
		return C_MP3Stream_Decode( lpMP3Stream, qfalse );	// bFastForwarding
	}
}


qboolean MP3Stream_SeekTo( channel_t *ch, float fTimeToSeekTo )
{
	const float fEpsilon = 0.05f;	// accurate to 1/50 of a second, but plus or minus this gives 1/10 of second

	MP3Stream_Rewind( ch );
	//
	// sanity... :-)
	//
	const float fTrackLengthInSeconds = MP3Stream_GetPlayingTimeInSeconds( &ch->MP3StreamHeader );
	if (fTimeToSeekTo > fTrackLengthInSeconds)
	{
		fTimeToSeekTo = fTrackLengthInSeconds;
	}

	// now do the seek...
	//
	while (1)
	{
		float fPlayingTimeElapsed = MP3Stream_GetPlayingTimeInSeconds( &ch->MP3StreamHeader ) - MP3Stream_GetRemainingTimeInSeconds( &ch->MP3StreamHeader );
		float fAbsTimeDiff = fabs(fTimeToSeekTo - fPlayingTimeElapsed);

		if ( fAbsTimeDiff <= fEpsilon)
			return qtrue;

		// when decoding, use fast-forward until within 3 seconds, then slow-decode (which should init stuff properly?)...
		//
		int iBytesDecodedThisPacket = C_MP3Stream_Decode( &ch->MP3StreamHeader, (fAbsTimeDiff > 3.0f) );	// bFastForwarding
		if (iBytesDecodedThisPacket == 0)
			break;	// EOS
	}

	return qfalse;
}


// returns qtrue for all ok
//
qboolean MP3Stream_Rewind( channel_t *ch )
{
	ch->iMP3SlidingDecodeWritePos = 0;
	ch->iMP3SlidingDecodeWindowPos= 0;

/*
	char *psError = C_MP3Stream_Rewind( &ch->MP3StreamHeader );

	if (psError)
	{
		Com_Printf(S_COLOR_YELLOW"%s\n",psError);
		return qfalse;
	}

	return qtrue;
*/

	// speed opt, since I know I already have the right data setup here...
	//
	memcpy(&ch->MP3StreamHeader, ch->thesfx->pMP3StreamHeader, sizeof(ch->MP3StreamHeader));
	return qtrue;

}


// returns qtrue while still playing normally, else qfalse for either finished or request-offset-error
//
qboolean MP3Stream_GetSamples( channel_t *ch, int startingSampleNum, int count, short *buf, qboolean bStereo )
{
	qboolean qbStreamStillGoing = qtrue;

	const int iQuarterOfSlidingBuffer		=  sizeof(ch->MP3SlidingDecodeBuffer)/4;
	const int iThreeQuartersOfSlidingBuffer	= (sizeof(ch->MP3SlidingDecodeBuffer)*3)/4;

//	Com_Printf("startingSampleNum %d\n",startingSampleNum);

	count *= 2/* <- = SOF2; ch->sfx->width*/;	// count arg was for words, so double it for bytes;

	// convert sample number into a byte offset... (make new variable for clarity?)
	//
	startingSampleNum *= 2 /* <- = SOF2; ch->sfx->width*/ * (bStereo?2:1);

	if ( startingSampleNum < ch->iMP3SlidingDecodeWindowPos)
	{
		// what?!?!?!   smegging time travel needed or something?, forget it
		memset(buf,0,count);
		return qfalse;
	}

//	Com_OPrintf("\nRequest: startingSampleNum %d, count %d\n",startingSampleNum,count);
//	Com_OPrintf("WindowPos %d, WindowWritePos %d\n",ch->iMP3SlidingDecodeWindowPos,ch->iMP3SlidingDecodeWritePos);

//	qboolean _bDecoded = qfalse;

	while (!
		(
			(startingSampleNum			>= ch->iMP3SlidingDecodeWindowPos)
			&&
			(startingSampleNum + count	<  ch->iMP3SlidingDecodeWindowPos + ch->iMP3SlidingDecodeWritePos)
			)
			)
	{
//		if (!_bDecoded)
//		{
//			Com_Printf(S_COLOR_YELLOW"Decode needed!\n");
//		}
//		_bDecoded = qtrue;
//		Com_OPrintf("Scrolling...");

		int _iBytesDecoded = MP3Stream_Decode( (LP_MP3STREAM) &ch->MP3StreamHeader, bStereo );	// stereo only for music, so this is safe
//		Com_OPrintf("%d bytes decoded\n",_iBytesDecoded);
		if (_iBytesDecoded == 0)
		{
			// no more source data left so clear the remainder of the buffer...
			//
			memset(ch->MP3SlidingDecodeBuffer + ch->iMP3SlidingDecodeWritePos, 0, sizeof(ch->MP3SlidingDecodeBuffer)-ch->iMP3SlidingDecodeWritePos);
//			Com_OPrintf("Finished\n");
			qbStreamStillGoing = qfalse;
			break;
		}
		else
		{
			memcpy(ch->MP3SlidingDecodeBuffer + ch->iMP3SlidingDecodeWritePos,ch->MP3StreamHeader.bDecodeBuffer,_iBytesDecoded);

			ch->iMP3SlidingDecodeWritePos += _iBytesDecoded;

			// if reached 3/4 of buffer pos, backscroll the decode window by one quarter...
			//
			if (ch->iMP3SlidingDecodeWritePos > iThreeQuartersOfSlidingBuffer)
			{
				memmove(ch->MP3SlidingDecodeBuffer, ((byte *)ch->MP3SlidingDecodeBuffer + iQuarterOfSlidingBuffer), iThreeQuartersOfSlidingBuffer);
				ch->iMP3SlidingDecodeWritePos -= iQuarterOfSlidingBuffer;
				ch->iMP3SlidingDecodeWindowPos+= iQuarterOfSlidingBuffer;
			}
		}
//		Com_OPrintf("WindowPos %d, WindowWritePos %d\n",ch->iMP3SlidingDecodeWindowPos,ch->iMP3SlidingDecodeWritePos);
	}

//	if (!_bDecoded)
//	{
//		Com_Printf(S_COLOR_YELLOW"No decode needed\n");
//	}

	assert(startingSampleNum >= ch->iMP3SlidingDecodeWindowPos);
	memcpy( buf, ch->MP3SlidingDecodeBuffer + (startingSampleNum-ch->iMP3SlidingDecodeWindowPos), count);

//	Com_OPrintf("OK\n\n");

	return qbStreamStillGoing;
}


///////////// eof /////////////

