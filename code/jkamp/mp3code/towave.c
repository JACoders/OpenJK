/*____________________________________________________________________________

	FreeAmp - The Free MP3 Player

        MP3 Decoder originally Copyright (C) 1995-1997 Xing Technology
        Corp.  http://www.xingtech.com

	Portions Copyright (C) 1998-1999 EMusic.com

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: towave.c,v 1.3 1999/10/19 07:13:09 elrod Exp $
____________________________________________________________________________*/

/* ------------------------------------------------------------------------

      NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

        This file exists for reference only. It is not actually used
        in the FreeAmp project. There is no need to mess with this
        file. There is no need to flatten the beavers, either.

      NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

/*---- towave.c --------------------------------------------
  32 bit version only

decode mpeg Layer I/II/III file using portable ANSI C decoder,
output to pcm wave file.

mod 8/19/98 decode 22 sf bands

mod 5/14/98  allow mpeg25 (dec8 not supported for mpeg25 samp rate)

mod 3/4/98 bs_trigger  bs_bufbytes  made signed, unsigned may
            not terminate properly.  Also extra test in bs_fill.

mod 8/6/96 add 8 bit output to standard decoder

ver 1.4 mods 7/18/96 32 bit and add asm option

mods 6/29/95  allow MS wave file for u-law.  bugfix u-law table dec8.c

mods 2/95 add sample rate reduction, freq_limit and conversions.
          add _decode8 for 8Ks output, 16bit 8bit, u-law output.
          add additional control parameters to init.
          add _info function

mod 5/12/95 add quick window cwinq.c

mod 5/19/95 change from stream io to handle io

mod 11/16/95 add Layer I

mod 1/5/95   integer overflow mod iup.c

ver 1.3
mod 2/5/96   portability mods
             drop Tom and Gloria pcm file types

ver 2.0
mod 1/7/97   Layer 3 (float mpeg-1 only)
    2/6/97   Layer 3 MPEG-2

ver 3.01     Layer III bugfix crc problem 8/18/97
ver 3.02     Layer III fix wannabe.mp3 problem 10/9/97
ver 3.03     allow mpeg 2.5  5/14/98

Decoder functions for _decode8 are defined in dec8.c.  Useage
is same as regular decoder.

Towave illustrates use of decoder.  Towave converts
mpeg audio files to 16 bit (short) pcm.  Default pcm file
format is wave. Other formats can be accommodated by
adding alternative write_pcm_header and write_pcm_tailer
functions.  The functions kbhit and getch used in towave.c
may not port to other systems.

The decoder handles all mpeg1 and mpeg2 Layer I/II  bitstreams.

For compatability with the asm decoder and future C versions,
source code users are discouraged from making modifications
to the decoder proper.  MS Windows applications can use wrapper
functions in a separate module if decoder functions need to
be exported.

NOTE additional control parameters.

mod 8/6/96 standard decoder adds 8 bit output

decode8 (8Ks output) convert_code:
   convert_code = 4*bit_code + chan_code
       bit_code:   1 = 16 bit linear pcm
                   2 =  8 bit (unsigned) linear pcm
                   3 = u-law (8 bits unsigned)
       chan_code:  0 = convert two chan to mono
                   1 = convert two chan to mono
                   2 = convert two chan to left chan
                   3 = convert two chan to right chan

decode (standard decoder) convert_code:
             0 = two chan output
             1 = convert two chan to mono
             2 = convert two chan to left chan
             3 = convert two chan to right chan
     or with 8 = 8 bit output
          (other bits ignored)

decode (standard decoder) reduction_code:
             0 = full sample rate output
             1 = half rate
             2 = quarter rate

-----------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>		/* file open flags */
#include <sys/types.h>		/* someone wants for port */
#include <sys/stat.h>		/* forward slash for portability */
#include "mhead.h"		/* mpeg header structure, decode protos */

#include "port.h"

// JDW
#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <errno.h>
#endif
// JDW

#include "mp3struct.h"
#include <assert.h>


#if !defined(MACOS_X) && !defined(byte) && !defined (__linux__)
typedef unsigned char byte;
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

id3v1_1 *gpTAG;
#define BYTESREMAINING_ACCOUNT_FOR_REAR_TAG(_pvData, _iBytesRemaining)								\
																									\
	/* account for trailing ID3 tag in _iBytesRemaining */											\
	gpTAG = (id3v1_1*) (((byte *)_pvData + _iBytesRemaining)-sizeof(id3v1_1));	/* sizeof = 128	*/	\
	if (!strncmp(gpTAG->id, "TAG", 3))																\
	{																								\
		_iBytesRemaining -= sizeof(id3v1_1);														\
	}





/********  pcm buffer ********/

#define PCM_BUFBYTES  60000U	// more than enough to cover the largest that one packet will ever expand to
char PCM_Buffer[PCM_BUFBYTES];	// better off being declared, so we don't do mallocs in this module (MAC reasons)

   typedef struct
   {
      int (*decode_init) (MPEG_HEAD * h, int framebytes_arg,
			  int reduction_code, int transform_code,
			  int convert_code, int freq_limit);
      void (*decode_info) (DEC_INFO * info);
	  IN_OUT(*decode) (unsigned char *bs, short *pcm, unsigned char *pNextByteAfterData);
   }
   AUDIO;

#if 0
   // stuff this...
   static AUDIO audio_table[2][2] =
   {
      {
	 {audio_decode_init, audio_decode_info, audio_decode},
	 {audio_decode8_init, audio_decode8_info, audio_decode8},
      },
      {
	 {i_audio_decode_init, i_audio_decode_info, i_audio_decode},
	 {audio_decode8_init, audio_decode8_info, audio_decode8},
      }
   };
   static AUDIO audio_table[2][2] =
   {
      {
		{audio_decode_init, audio_decode_info, audio_decode},
		{audio_decode_init, audio_decode_info, audio_decode},
      },
      {
		{audio_decode_init, audio_decode_info, audio_decode},
		{audio_decode_init, audio_decode_info, audio_decode},
      }
   };
#endif

   static const AUDIO audio = {audio_decode_init, audio_decode_info, audio_decode};	//audio_table[0][0];


// Do NOT change these, ever!!!!!!!!!!!!!!!!!!
//
const int reduction_code	= 0;		// unpack at full sample rate output
const int convert_code_mono	= 1;
const int convert_code_stereo = 0;
const int freq_limit		= 24000;	// no idea what this is about, but it's always this value so...

// the entire decode mechanism uses this now...
//
MP3STREAM _MP3Stream;
LP_MP3STREAM pMP3Stream = &_MP3Stream;
int bFastEstimateOnly = 0;	// MUST DEFAULT TO THIS VALUE!!!!!!!!!


// char *return is NZ for any errors (no trailing CR!)
//
char *C_MP3_IsValid(void *pvData, int iDataLen, int bStereoDesired)
{
//	char sTemp[1024];	/////////////////////////////////////////////////
	unsigned int iRealDataStart;
	MPEG_HEAD head;
	DEC_INFO  decinfo;

	int iBitRate;
	int iFrameBytes;

//#ifdef _DEBUG
//	int iIgnoreThisForNowIJustNeedItToBreakpointOnToReadAValue = sizeof(MP3STREAM);
//#endif

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

	iFrameBytes = head_info3( pvData, iDataLen/2, &head, &iBitRate, &iRealDataStart);
	if (iFrameBytes == 0)
	{
		return "MP3ERR: Bad or unsupported file!";
	}

	// check for files with bad frame unpack sizes (that would crash the game), or stereo files.
	//
	// although the decoder can convert stereo to mono (apparently), we want to know about stereo files
	//	because they're a waste of source space... (all FX are mono, and moved via panning)
	//
	if (head.mode != 3 && !bStereoDesired)	//3 seems to mean mono
	{
		if (iDataLen > 98000) {	// we'll allow it for small files even if stereo
			return "MP3ERR: Sound file is stereo!";
		}
	}
	if (audio.decode_init(&head, iFrameBytes, reduction_code, iRealDataStart, bStereoDesired?convert_code_stereo:convert_code_mono, freq_limit))
	{
		if (bStereoDesired)
		{
			if (pMP3Stream->outbytes > 4608)
			{
				return "MP3ERR: Source file has output packet size > 2304 (*2 for stereo) bytes!";
			}
		}
		else
		{
			if (pMP3Stream->outbytes > 2304)
			{
				return "MP3ERR: Source file has output packet size > 2304 bytes!";
			}
		}

		audio.decode_info(&decinfo);

		if (decinfo.bits != 16)
		{
			return "MP3ERR: Source file is not 16bit!";	// will this ever happen? oh well...
		}

		if (decinfo.samprate != 44100)
		{
			return "MP3ERR: Source file is not sampled @ 44100!";
		}

		if (bStereoDesired && decinfo.channels != 2)
		{
			return "MP3ERR: Source file is not stereo!";	// sod it, I'm going to count this as an error now
		}
	}
	else
	{
		return "MP3ERR: Decoder failed to initialise";
	}

	// file seems to be valid...
	//
	return NULL;
}



// char *return is NZ for any errors (no trailing CR!)
//
char* C_MP3_GetHeaderData(void *pvData, int iDataLen, int *piRate, int *piWidth, int *piChannels, int bStereoDesired)
{
	unsigned int iRealDataStart;
	MPEG_HEAD head;
	DEC_INFO  decinfo;

	int iBitRate;
	int iFrameBytes;

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

	iFrameBytes = head_info3( pvData, iDataLen/2, &head, &iBitRate, &iRealDataStart);
	if (iFrameBytes == 0)
	{
		return "MP3ERR: Bad or unsupported file!";
	}

	if (audio.decode_init(&head, iFrameBytes, reduction_code, iRealDataStart, bStereoDesired?convert_code_stereo:convert_code_mono, freq_limit))
	{
		audio.decode_info(&decinfo);

		*piRate		= decinfo.samprate;	// rate (eg 22050, 44100 etc)
		*piWidth	= decinfo.bits/8;	// 1 for 8bit, 2 for 16 bit
		*piChannels	= decinfo.channels;	// 1 for mono, 2 for stereo
	}
	else
	{
		return "MP3ERR: Decoder failed to initialise";
	}

	// everything ok...
	//
	return NULL;
}




// this duplicates work done in C_MP3_IsValid(), but it avoids global structs, and means that you can call this anytime
//	if you just want info for some reason
//
// ( size is now workd out just by decompressing each packet header, not the whole stream. MUCH faster :-)
//
// char *return is NZ for any errors (no trailing CR!)
//
char *C_MP3_GetUnpackedSize(void *pvData, int iSourceBytesRemaining, int *piUnpackedSize, int bStereoDesired )
{
	int iReadLimit;
	unsigned int iRealDataStart;
	MPEG_HEAD head;
	int iBitRate;

	char *pPCM_Buffer = PCM_Buffer;
	char *psReturn = NULL;
//	int  iSourceReadIndex = 0;
	int	 iDestWriteIndex = 0;

	int iFrameBytes;
	int iFrameCounter;

	DEC_INFO decinfo;
	IN_OUT	 x;

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

#define iSourceReadIndex iRealDataStart

//	iFrameBytes = head_info2( pvData, 0, &head, &iBitRate);
	iFrameBytes = head_info3( pvData, iSourceBytesRemaining/2, &head, &iBitRate, &iRealDataStart);

	BYTESREMAINING_ACCOUNT_FOR_REAR_TAG(pvData, iSourceBytesRemaining)
	iSourceBytesRemaining -= iRealDataStart;

	iReadLimit = iSourceReadIndex + iSourceBytesRemaining;

	if (iFrameBytes)
	{
		//pPCM_Buffer = malloc(PCM_BUFBYTES);

		//if (pPCM_Buffer)
		{
			// init decoder...

			if (audio.decode_init(&head, iFrameBytes, reduction_code, iRealDataStart, bStereoDesired?convert_code_stereo:convert_code_mono, freq_limit))
			{
				audio.decode_info(&decinfo);

				// decode...
				//
				for (iFrameCounter = 0;;iFrameCounter++)
				{
					if ( iSourceBytesRemaining == 0 || iSourceBytesRemaining < iFrameBytes)
						break;	// end of file

					bFastEstimateOnly = 1;	///////////////////////////////

							x = audio.decode((unsigned char *)pvData + iSourceReadIndex, (short *) ((char *)pPCM_Buffer
																							//+ iDestWriteIndex		// keep decoding over the same spot since we're only counting bytes in this function
																							),
											(unsigned char *)pvData + iReadLimit
											);

					bFastEstimateOnly = 0;	///////////////////////////////

					iSourceReadIndex		+= x.in_bytes;
					iSourceBytesRemaining	-= x.in_bytes;
					iDestWriteIndex			+= x.out_bytes;

					if (x.in_bytes <= 0)
					{
						//psReturn = "MP3ERR: Bad sync in file";
						break;
					}
				}

				*piUnpackedSize = iDestWriteIndex;	// yeeehaaa!
			}
			else
			{
				psReturn = "MP3ERR: Decoder failed to initialise";
			}
		}
//		else
//		{
//			psReturn = "MP3ERR: Unable to alloc temp decomp buffer";
//		}
	}
	else
	{
		psReturn = "MP3ERR: Bad or Unsupported MP3 file!";
	}


//	if (pPCM_Buffer)
//	{
//		free(pPCM_Buffer);
//		pPCM_Buffer = NULL;	// I know, I know...
//	}

	return psReturn;

#undef iSourceReadIndex
}




char *C_MP3_UnpackRawPCM( void *pvData, int iSourceBytesRemaining, int *piUnpackedSize, void *pbUnpackBuffer, int bStereoDesired)
{
	int iReadLimit;
	unsigned int iRealDataStart;
	MPEG_HEAD head;
	int iBitRate;

	char *psReturn = NULL;
//	int  iSourceReadIndex = 0;
	int	 iDestWriteIndex = 0;

	int iFrameBytes;
	int iFrameCounter;

	DEC_INFO decinfo;
	IN_OUT	 x;

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

#define iSourceReadIndex iRealDataStart

//	iFrameBytes = head_info2( pvData, 0, &head, &iBitRate);
	iFrameBytes = head_info3( pvData, iSourceBytesRemaining/2, &head, &iBitRate, &iRealDataStart);

	BYTESREMAINING_ACCOUNT_FOR_REAR_TAG(pvData, iSourceBytesRemaining)
	iSourceBytesRemaining -= iRealDataStart;

	iReadLimit = iSourceReadIndex + iSourceBytesRemaining;

	if (iFrameBytes)
	{
//		if (1)////////////////////////pPCM_Buffer)
		{
			// init decoder...

			if (audio.decode_init(&head, iFrameBytes, reduction_code, iRealDataStart, bStereoDesired?convert_code_stereo:convert_code_mono, freq_limit))
			{
				audio.decode_info(&decinfo);

//				printf("\n output samprate = %6ld",decinfo.samprate);
//				printf("\n output channels = %6d", decinfo.channels);
//				printf("\n output bits     = %6d", decinfo.bits);
//				printf("\n output type     = %6d", decinfo.type);

//===============

				// decode...
				//
				for (iFrameCounter = 0;;iFrameCounter++)
				{
					if ( iSourceBytesRemaining == 0 || iSourceBytesRemaining < iFrameBytes)
						break;	// end of file

					x = audio.decode((unsigned char *)pvData + iSourceReadIndex, (short *) ((char *)pbUnpackBuffer + iDestWriteIndex),
									 (unsigned char *)pvData + iReadLimit
									);

					iSourceReadIndex		+= x.in_bytes;
					iSourceBytesRemaining	-= x.in_bytes;
					iDestWriteIndex			+= x.out_bytes;

					if (x.in_bytes <= 0)
					{
						//psReturn = "MP3ERR: Bad sync in file";
						break;
					}
				}

				*piUnpackedSize = iDestWriteIndex;	// yeeehaaa!
			}
			else
			{
				psReturn = "MP3ERR: Decoder failed to initialise";
			}
		}
	}
	else
	{
		psReturn = "MP3ERR: Bad or Unsupported MP3 file!";
	}

	return psReturn;

#undef iSourceReadIndex
}


// called once, after we've decided to keep something as MP3. This just sets up the decoder for subsequent stream-calls.
//
// (the struct pSFX_MP3Stream is cleared internally, so pass as args anything you want stored in it)
//
// char * return is NULL for ok, else error string
//
// NEW CODE NOTE: Now that this function is being called for raw data that's going to be runtime-stored in a link-list
//	chunk format for SOF2 instead of just one alloc then you must re-init pMP3Stream->pbSourceData after you've called
//	this, or it'll be pointing at the deallocated original raw data, not the first chunk of the link list
//
char *C_MP3Stream_DecodeInit( LP_MP3STREAM pSFX_MP3Stream, void *pvSourceData, int iSourceBytesRemaining,
							  int iGameAudioSampleRate, int iGameAudioSampleBits, int bStereoDesired )
{
	char			*psReturn = NULL;
	MPEG_HEAD		head;			// only relevant within this function during init
	DEC_INFO		decinfo;		//   " "
	int				iBitRate;		// not used after being filled in by head_info3()

	pMP3Stream = pSFX_MP3Stream;

	memset(pMP3Stream,0,sizeof(*pMP3Stream));

	pMP3Stream->pbSourceData			= (byte *) pvSourceData;	// this MUST be re-initialised to link-mem outside here for SOF2, since raw data is now link-listed
	pMP3Stream->iSourceBytesRemaining	= iSourceBytesRemaining;
	pMP3Stream->iSourceFrameBytes		= head_info3( (byte *) pvSourceData, iSourceBytesRemaining/2, &head, &iBitRate, (unsigned int*)&pMP3Stream->iSourceReadIndex );

	// hack, do NOT do this for stereo, since music files are now streamed and therefore the data isn't actually fully
	//	loaded at this point, only about 4k or so for the header is actually in memory!!!...
	//
	if (!bStereoDesired)
	{
		BYTESREMAINING_ACCOUNT_FOR_REAR_TAG(pvSourceData, pMP3Stream->iSourceBytesRemaining);
		pMP3Stream->iSourceBytesRemaining  -= pMP3Stream->iSourceReadIndex;
	}

	// backup a couple of fields so we can play this again later...
	//
	pMP3Stream->iRewind_SourceReadIndex		= pMP3Stream->iSourceReadIndex;
	pMP3Stream->iRewind_SourceBytesRemaining= pMP3Stream->iSourceBytesRemaining;

	assert(pMP3Stream->iSourceFrameBytes);
	if (pMP3Stream->iSourceFrameBytes)
	{
		if (audio.decode_init(&head, pMP3Stream->iSourceFrameBytes, reduction_code, pMP3Stream->iSourceReadIndex, bStereoDesired?convert_code_stereo:convert_code_mono, freq_limit))
		{
			pMP3Stream->iRewind_FinalReductionCode = reduction_code;	// default = 0 (no reduction), 1=half, 2 = quarter

			pMP3Stream->iRewind_FinalConvertCode   = bStereoDesired?convert_code_stereo:convert_code_mono;
																		// default = 1 (mono), OR with 8 for 8-bit output

			// only now can we ask what kind of properties this file has, and then adjust to fit what the game wants...
			//
			audio.decode_info(&decinfo);

//			printf("\n output samprate = %6ld",decinfo.samprate);
//			printf("\n output channels = %6d", decinfo.channels);
//			printf("\n output bits     = %6d", decinfo.bits);
//			printf("\n output type     = %6d", decinfo.type);

			// decoder offers half or quarter rate adjustement only...
			//
			if (iGameAudioSampleRate == decinfo.samprate>>1)
				pMP3Stream->iRewind_FinalReductionCode = 1;
			else
			if (iGameAudioSampleRate == decinfo.samprate>>2)
				pMP3Stream->iRewind_FinalReductionCode = 2;

			if (iGameAudioSampleBits == decinfo.bits>>1)	// if game wants 8 bit sounds, then setup for that
				pMP3Stream->iRewind_FinalConvertCode |= 8;

			if (audio.decode_init(&head, pMP3Stream->iSourceFrameBytes, pMP3Stream->iRewind_FinalReductionCode, pMP3Stream->iSourceReadIndex, pMP3Stream->iRewind_FinalConvertCode, freq_limit))
			{
				audio.decode_info(&decinfo);
#ifdef _DEBUG
				assert( iGameAudioSampleRate == decinfo.samprate );
				assert( iGameAudioSampleBits == decinfo.bits );
#endif

				// sod it, no harm in one last check... (should never happen)
				//
				if ( iGameAudioSampleRate != decinfo.samprate || iGameAudioSampleBits != decinfo.bits )
				{
					psReturn = "MP3ERR: Decoder unable to convert to current game audio settings";
				}
			}
			else
			{
				psReturn = "MP3ERR: Decoder failed to initialise for pass 2 sample adjust";
			}
		}
		else
		{
			psReturn = "MP3ERR: Decoder failed to initialise";
		}
	}
	else
	{
		psReturn = "MP3ERR: Errr.... something's broken with this MP3 file";	// should never happen by this point
	}

	// restore global stream ptr before returning to normal functions (so the rest of the MP3 code still works)...
	//
	pMP3Stream = &_MP3Stream;

	return psReturn;
}

// return value is decoded bytes for this packet, which is effectively a BOOL, NZ for not finished decoding yet...
//
unsigned int C_MP3Stream_Decode( LP_MP3STREAM pSFX_MP3Stream )
{
	unsigned int uiDecoded = 0;	// default to "finished"
	IN_OUT	 x;

	pMP3Stream = pSFX_MP3Stream;

	if ( pSFX_MP3Stream->iSourceBytesRemaining == 0)// || pSFX_MP3Stream->iSourceBytesRemaining < pSFX_MP3Stream->iSourceFrameBytes)
	{
		uiDecoded = 0;	// finished
		pMP3Stream = &_MP3Stream;
		return uiDecoded;
	}

	x = audio.decode(pSFX_MP3Stream->pbSourceData + pSFX_MP3Stream->iSourceReadIndex, (short *) (pSFX_MP3Stream->bDecodeBuffer),
					 pSFX_MP3Stream->pbSourceData + pSFX_MP3Stream->iRewind_SourceReadIndex + pSFX_MP3Stream->iRewind_SourceBytesRemaining
					);

#ifdef _DEBUG
	pSFX_MP3Stream->iSourceFrameCounter++;
#endif

	pSFX_MP3Stream->iSourceReadIndex		+= x.in_bytes;
	pSFX_MP3Stream->iSourceBytesRemaining	-= x.in_bytes;
	pSFX_MP3Stream->iBytesDecodedTotal		+= x.out_bytes;
	pSFX_MP3Stream->iBytesDecodedThisPacket	 = x.out_bytes;

	uiDecoded = x.out_bytes;

	if (x.in_bytes <= 0)
	{
		//psReturn = "MP3ERR: Bad sync in file";
		uiDecoded= 0;	// finished
		pMP3Stream = &_MP3Stream;
		return uiDecoded;
	}

	// restore global stream ptr before returning to normal functions (so the rest of the MP3 code still works)...
	//
	pMP3Stream = &_MP3Stream;

	return uiDecoded;
}


// ret is char* errstring, else NULL for ok
//
char *C_MP3Stream_Rewind( LP_MP3STREAM pSFX_MP3Stream )
{
	char*		psReturn = NULL;
	MPEG_HEAD	head;			// only relevant within this function during init
	int			iBitRate;		// ditto
	int			iNULL;

	pMP3Stream = pSFX_MP3Stream;

	pMP3Stream->iSourceReadIndex		= pMP3Stream->iRewind_SourceReadIndex;
	pMP3Stream->iSourceBytesRemaining	= pMP3Stream->iRewind_SourceBytesRemaining;	// already adjusted for tags etc

	// I'm not sure that this is needed, but where else does decode_init get passed useful data ptrs?...
	//
	if (pMP3Stream->iSourceFrameBytes == head_info3( pMP3Stream->pbSourceData, pMP3Stream->iSourceBytesRemaining/2, &head, &iBitRate, (unsigned int*)&iNULL ) )
	{
		if (audio.decode_init(&head, pMP3Stream->iSourceFrameBytes, pMP3Stream->iRewind_FinalReductionCode, pMP3Stream->iSourceReadIndex, pMP3Stream->iRewind_FinalConvertCode, freq_limit))
		{
			// we should always get here...
			//
		}
		else
		{
			psReturn = "MP3ERR: Failed to re-init decoder for rewind!";
		}
	}
	else
	{
		psReturn = "MP3ERR: Frame bytes mismatch during rewind header-read!";
	}

	// restore global stream ptr before returning to normal functions (so the rest of the MP3 code still works)...
	//
	pMP3Stream = &_MP3Stream;

	return psReturn;
}

