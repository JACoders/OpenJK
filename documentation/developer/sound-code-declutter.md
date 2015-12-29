As some may have noticed the Raven advancements to the entire soundcode are absolutely all over the place and is a huge mess.

I would like to start by seeing if we can get a rough similar port of the ioq3 sound code into jka and then start to re-add the JKA features.  (I know, I know don't need ioquake3 for everything)  But similar idea anyway.  Something more C++ would be ideal eventually however we still have to maintain the API so we can't get too crazy.

As a comparison take a look at some of the way things are handled here:

Interface:
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_main.c

Codec Handler similar to tr_image and tr_model
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_codec.c

Codecs:
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_codec_wav.c
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_codec_ogg.c
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_codec_opus.c
(MP3 should be handled differently IMO but we need to make sure it still supports things the same?)  There's a libmad implementation available:
See below for the ioEF snd_codec_mp3 which can be worked with?

OpenAL Interface split off into its own interface file:
- There is a lot of rethinking to do because Raven actually seems to maybe have semi-real channel support and Q3 does not.
- The ioq3 implementation also does some things differently that do not mix very well with Raven's mashup codebase.
- Add EFX support to replace EAX.  Though we can't really do much about the EAL files can we?  Currently "OpenAL" at all is only supported on Win32 anyway since it depends on the proprietary version with EAX.
- There is also a serious issue with allowing OJK on windows to be installed on the same directory as JO/JA.  Both regular versions of the games ship with an OpenAL32.dll which crashes since its some ancient dll from Creative when using OJK with alsoft lib/headers and vice versa.
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_openal.c

"Q3" sound interface by itself:
- https://github.com/ioquake/ioq3/blob/master/code/client/snd_dma.c

Other stuff:
- The ambient sound code could be rewritten and spun off probably with some of wonko's new file and string parsing code (it does not use the genericparser class though)
- Dynamic Music code could be adapted entirely so that its independent of the interface code and the interface can make calls to the dynamic music code.
- Dynamic Music also still needs the parser rewrite from SP in MP I think?

====

This was from the ioEF port and could be adapted to support MP3 better but it at least uses libmad instead of mp3code:

```
/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2005-2006 Joerg Dietrich <dietrich_joerg@gmx.de>
Copyright (C) 2006 Thilo Schulz <arny@ats.s.bawue.de>

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// includes for the Q3 sound system
#include "client.h"
#include "snd_codec.h"

// includes for the MP3 codec
#include <mad.h>

#define MP3_SAMPLE_WIDTH		2
#define MP3_PCMSAMPLES_PERSLICE		32

// buffer size used when reading through the mp3
#define MP3_DATA_BUFSIZ			128*1024

// undefine this if you don't want any dithering.
#define MP3_DITHERING

// Q3 MP3 codec
snd_codec_t mp3_codec =
{
	"mp3",
	S_MP3_CodecLoad,
	S_MP3_CodecOpenStream,
	S_MP3_CodecReadStream,
	S_MP3_CodecCloseStream,
	NULL
};

// structure used for info purposes
struct snd_codec_mp3_info
{
	byte encbuf[MP3_DATA_BUFSIZ];	// left over bytes not consumed
					// by the decoder.
	struct mad_stream madstream;	// uses encbuf as buffer.
	struct mad_frame madframe;	// control structures for libmad.
	struct mad_synth madsynth;

	byte *pcmbuf;			// buffer for not-used samples.
	int buflen;			// length of buffer data.
	int pcmbufsize;			// amount of allocated memory for
					// pcmbuf. This should have at least
					// the size of a decoded mp3 frame.	

	byte *dest;			// copy decoded data here.
	int destlen;			// amount of already copied data.
	int destsize;			// amount of bytes we must decode.
};

/*************** MP3 utility functions ***************/

/*
=================
S_MP3_ReadData
=================
*/

// feed libmad with data
int S_MP3_ReadData(snd_stream_t *stream, struct mad_stream *madstream, byte *encbuf, int encbufsize)
{
	int retval;
	int leftover;
	
	if(!stream)
		return -1;
	
	leftover =  madstream->bufend - madstream->next_frame;
	if(leftover > 0)
		memmove(encbuf, madstream->this_frame, leftover);

	// Fill the buffer right to the end
	
	retval = FS_Read(&encbuf[leftover], encbufsize - leftover, stream->file);

	if(retval <= 0)
	{
		// EOF reached, that's ok.
		return 0;
	}
	
	mad_stream_buffer(madstream, encbuf, retval + leftover);

	return retval;
}

/*
=================
S_MP3_Scanfile

to determine the samplecount, we apparently must get *all* headers :(
I basically used the xmms-mad plugin source to see how this stuff works.

returns a value < 0 on error.
=================
*/

int S_MP3_Scanfile(snd_stream_t *stream)
{
	struct mad_stream madstream;
	struct mad_header madheader;
	int retval;
	int samplecount;
	byte encbuf[MP3_DATA_BUFSIZ];

	// error out on invalid input.
	if(!stream)
		return -1;

	mad_stream_init(&madstream);
	mad_header_init(&madheader);
	
	while(1)
	{
		retval = S_MP3_ReadData(stream, &madstream, encbuf, sizeof(encbuf));
		if(retval < 0)
			return -1;
		else if(retval == 0)
			break;
		
		// Start decoding the headers.
		while(1)
		{
			if((retval = mad_header_decode(&madheader, &madstream)) < 0)
			{
				if(madstream.error == MAD_ERROR_BUFLEN)
				{
					// We need to read more data
					break;
				}

				if(!MAD_RECOVERABLE (madstream.error))
				{
					// unrecoverable error... we must bail out.
					return retval;
				}

				mad_stream_skip(&madstream, madstream.skiplen);
				continue;
			}
			
			// we got a valid header.
			
			if(madheader.layer != MAD_LAYER_III)
			{
				// we don't support non-mp3s
				return -1;
			}

			if(!stream->info.samples)
			{
				// This here is the very first frame. Set initial values now,
				// that we expect to stay constant throughout the whole mp3.
				
				stream->info.rate = madheader.samplerate;
				stream->info.width = MP3_SAMPLE_WIDTH;
				stream->info.channels = MAD_NCHANNELS(&madheader);
				stream->info.samples = 0;
				stream->info.size = 0;				// same here.
				stream->info.dataofs = 0;
			}
			else
			{
				// Check whether something changed that shouldn't.
				
				if(stream->info.rate != madheader.samplerate ||
				   stream->info.channels != MAD_NCHANNELS(&madheader))
					return -1;
			}

			// Update the counters
			samplecount = MAD_NSBSAMPLES(&madheader) * MP3_PCMSAMPLES_PERSLICE;
			stream->info.samples += samplecount;
			stream->info.size += samplecount * stream->info.channels * stream->info.width;			
		}
	}
	
	// Reset the file pointer so we can do the real decoding.
	FS_Seek(stream->file, 0, FS_SEEK_SET);
	
	return 0;
}

/************************ dithering functions ***************************/

#ifdef MP3_DITHERING

// All dithering done here is taken from the GPL'ed xmms-mad plugin.

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

/* Period parameters */
#define MP3_DITH_N 624
#define MP3_DITH_M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[MP3_DITH_N]; /* the array for the state vector  */
static int mti=MP3_DITH_N+1; /* mti==MP3_DITH_N+1 means mt[MP3_DITH_N] is not initialized */

/* initializing the array with a NONZERO seed */
void sgenrand(unsigned long seed)
{
    /* setting initial seeds to mt[MP3_DITH_N] using         */
    /* the generator Line 25 of Table 1 in          */
    /* [KNUTH 1981, The Art of Computer Programming */
    /*    Vol. 2 (2nd Ed.), pp102]                  */
    mt[0]= seed & 0xffffffff;
    for (mti=1; mti<MP3_DITH_N; mti++)
        mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

unsigned long genrand(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= MP3_DITH_N) { /* generate MP3_DITH_N words at one time */
        int kk;

        if (mti == MP3_DITH_N+1)   /* if sgenrand() has not been called, */
            sgenrand(4357); /* a default initial seed is used   */

        for (kk=0;kk<MP3_DITH_N-MP3_DITH_M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+MP3_DITH_M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<MP3_DITH_N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(MP3_DITH_M-MP3_DITH_N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[MP3_DITH_N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[MP3_DITH_N-1] = mt[MP3_DITH_M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }

    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

long triangular_dither_noise(int nbits) {
    // parameter nbits : the peak-to-peak amplitude desired (in bits)
    //  use with nbits set to    2 + nber of bits to be trimmed.
    // (because triangular is made from two uniformly distributed processes,
    // it starts at 2 bits peak-to-peak amplitude)
    // see The Theory of Dithered Quantization by Robert Alexander Wannamaker
    // for complete proof of why that's optimal

    long v = (genrand()/2 - genrand()/2); // in ]-2^31, 2^31[
    //int signe = (v>0) ? 1 : -1;
    long P = 1 << (32 - nbits); // the power of 2
    v /= P;
    // now v in ]-2^(nbits-1), 2^(nbits-1) [

    return v;
}

#endif // MP3_DITHERING

/************************ decoder functions ***************************/

/*
=================
S_MP3_Scale

Converts the signal to 16 bit LE-PCM data and does dithering.

- borrowed from xmms-mad plugin source.
=================
*/

/*
 * xmms-mad - mp3 plugin for xmms
 * Copyright (C) 2001-2002 Sam Clegg
 */

signed int S_MP3_Scale(mad_fixed_t sample)
{
	int n_bits_to_loose = MAD_F_FRACBITS + 1 - 16;
#ifdef MP3_DITHERING
	int dither;
#endif
	
	// round
	sample += (1L << (n_bits_to_loose - 1));

#ifdef MP3_DITHERING
	dither = triangular_dither_noise(n_bits_to_loose + 1);
	sample += dither;
#endif

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> n_bits_to_loose;
}

/*
=================
S_MP3_PCMCopy

Copy and convert pcm data until bytecount bytes have been written.
return the position in pcm->samples.
indicate the amount of actually written bytes in wrotecnt.
=================
*/

int S_MP3_PCMCopy(byte *buf, struct mad_pcm *pcm, int bufofs,
			 int sampleofs, int bytecount, int *wrotecnt)
{
	int written = 0;
	signed int sample;
	int framesize = pcm->channels * MP3_SAMPLE_WIDTH;

	// add new pcm data.
	while(written < bytecount && sampleofs < pcm->length)
	{
		sample = S_MP3_Scale(pcm->samples[0][sampleofs]);

#ifdef Q3_BIG_ENDIAN
		// output to 16 bit big endian PCM
		buf[bufofs++] = (sample >> 8) & 0xff;
		buf[bufofs++] = sample & 0xff;
#else
		// output to 16 bit little endian PCM
		buf[bufofs++] = sample & 0xff;
		buf[bufofs++] = (sample >> 8) & 0xff;
#endif
		
		if(pcm->channels == 2)
		{
			sample = S_MP3_Scale(pcm->samples[1][sampleofs]);

#ifdef Q3_BIG_ENDIAN
			buf[bufofs++] = (sample >> 8) & 0xff;
			buf[bufofs++] = sample & 0xff;
#else
			buf[bufofs++] = sample & 0xff;
			buf[bufofs++] = (sample >> 8) & 0xff;
#endif
		}
		
		sampleofs++;
		written += framesize;
	}	

	if(wrotecnt)
		*wrotecnt = written;

	return sampleofs;
}

/*
=================
S_MP3_Decode
=================
*/

// gets executed for every decoded frame.
int S_MP3_Decode(snd_stream_t *stream)
{
	struct snd_codec_mp3_info *mp3info;
	struct mad_stream *madstream;
	struct mad_frame *madframe;
	struct mad_synth *madsynth;
	struct mad_pcm *pcm;
	int cursize;
	int samplecount;
	int needcount;
	int wrote;
	int retval;

	if(!stream)
		return -1;

	mp3info = (struct snd_codec_mp3_info *)stream->ptr;
	madstream = &mp3info->madstream;
	madframe = &mp3info->madframe;

	if(mad_frame_decode(madframe, madstream))
	{
		if(madstream->error == MAD_ERROR_BUFLEN)
		{
			// we need more data. Read another chunk.
			retval = S_MP3_ReadData(stream, madstream, mp3info->encbuf, sizeof(mp3info->encbuf));

			// call myself again now that buffer is full.
			if(retval > 0)
				retval = S_MP3_Decode(stream);
		}
		else if(MAD_RECOVERABLE(madstream->error))
		{
			mad_stream_skip(madstream, madstream->skiplen);
			return S_MP3_Decode(stream);
		}
		else
			retval = -1;

		return retval;
	}

	// check whether this really is an mp3
	if(madframe->header.layer != MAD_LAYER_III)
		return -1;

	// generate pcm data
	madsynth = &mp3info->madsynth;
	mad_synth_frame(madsynth, madframe);

	pcm = &madsynth->pcm;

	// perform a few checks to see whether something changed that shouldn't.
		
	if(stream->info.rate != pcm->samplerate ||
	   stream->info.channels != pcm->channels)
	{
		return -1;
	}
	// see whether we have got enough data now.
	cursize = pcm->length * pcm->channels * stream->info.width;
	needcount = mp3info->destsize - mp3info->destlen;

	// Copy exactly as many samples as required.
	samplecount = S_MP3_PCMCopy(mp3info->dest, pcm,
				    mp3info->destlen, 0, needcount, &wrote);
	mp3info->destlen += wrote;
	
	if(samplecount < pcm->length)
	{
		// Not all samples got copied. Copy the rest into the pcm buffer.
		samplecount = S_MP3_PCMCopy(mp3info->pcmbuf, pcm,
					    mp3info->buflen,
					    samplecount,
					    mp3info->pcmbufsize - mp3info->buflen,
					    &wrote);
		mp3info->buflen += wrote;
		

		if(samplecount < pcm->length)
		{
			// The pcm buffer was not large enough. Make it bigger.
			byte *newbuf = (byte *)Z_Malloc(cursize);
			
			if(mp3info->pcmbuf)
			{
				Com_Memcpy(newbuf, mp3info->pcmbuf, mp3info->buflen);
				Z_Free(mp3info->pcmbuf);
			}
			
			mp3info->pcmbuf = newbuf;
			mp3info->pcmbufsize = cursize;
			
			samplecount = S_MP3_PCMCopy(mp3info->pcmbuf, pcm,
						    mp3info->buflen,
						    samplecount,
						    mp3info->pcmbufsize - mp3info->buflen,
						    &wrote);
			mp3info->buflen += wrote;		
		}		
		
		// we're definitely done.
		retval = 0;
	}
	else if(mp3info->destlen >= mp3info->destsize)
		retval = 0;
	else
		retval = 1;

	return retval;
}

/*************** Callback functions for quake3 ***************/

/*
=================
S_MP3_CodecOpenStream
=================
*/

snd_stream_t *S_MP3_CodecOpenStream(const char *filename)
{
	snd_stream_t *stream;
	struct snd_codec_mp3_info *mp3info;

	// Open the stream
	stream = S_CodecUtilOpen(filename, &mp3_codec);
	if(!stream || stream->length <= 0)
		return NULL;

	// We have to scan through the MP3 to determine the important mp3 info.
	if(S_MP3_Scanfile(stream) < 0)
	{
		// scanning didn't work out...
		S_CodecUtilClose(&stream);
		return NULL;
	}

	// Initialize the mp3 info structure we need for streaming
	mp3info = (struct snd_codec_mp3_info *)Z_Malloc(sizeof(*mp3info));
	if(!mp3info)
	{
		S_CodecUtilClose(&stream);
		return NULL;
	}

	stream->ptr = mp3info;

	// initialize the libmad control structures.
	mad_stream_init(&mp3info->madstream);
	mad_frame_init(&mp3info->madframe);
	mad_synth_init(&mp3info->madsynth);

	if(S_MP3_ReadData(stream, &mp3info->madstream, mp3info->encbuf, sizeof(mp3info->encbuf)) <= 0)
	{
		// we didnt read anything, that's bad.
		S_MP3_CodecCloseStream(stream);
		return NULL;
	}

	return stream;
}

/*
=================
S_MP3_CodecCloseStream
=================
*/

// free all memory we allocated.
void S_MP3_CodecCloseStream(snd_stream_t *stream)
{
	struct snd_codec_mp3_info *mp3info;
	
	if(!stream)
		return;
		
	// free all data in our mp3info tree

	if(stream->ptr)
	{
		mp3info = (struct snd_codec_mp3_info *)stream->ptr;

		if(mp3info->pcmbuf)
			Z_Free(mp3info->pcmbuf);

		mad_synth_finish(&mp3info->madsynth);
		mad_frame_finish(&mp3info->madframe);
		mad_stream_finish(&mp3info->madstream);
	
		Z_Free(stream->ptr);
	}

	S_CodecUtilClose(&stream);
}

/*
=================
S_MP3_CodecReadStream
=================
*/
int S_MP3_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	struct snd_codec_mp3_info *mp3info;
	int retval;
	
	if(!stream)
		return -1;
		
	mp3info = (struct snd_codec_mp3_info *)stream->ptr;

	// Make sure we get complete frames all the way through.
	bytes -= bytes % (stream->info.channels * stream->info.width);

	if(mp3info->buflen)
	{
		if(bytes < mp3info->buflen)
		{
			// we still have enough bytes in our decoded pcm buffer
			Com_Memcpy(buffer, mp3info->pcmbuf, bytes);
		
			// remove the portion from our buffer.
			mp3info->buflen -= bytes;
			memmove(mp3info->pcmbuf, &mp3info->pcmbuf[bytes], mp3info->buflen);
			return bytes;
		}
		else
		{
			// copy over the samples we already have.
			Com_Memcpy(buffer, mp3info->pcmbuf, mp3info->buflen);
			mp3info->destlen = mp3info->buflen;
			mp3info->buflen = 0;
		}
	}
	else
		mp3info->destlen = 0;
	
	mp3info->dest = (byte *)buffer;
	mp3info->destsize = bytes;

	do
	{
		retval = S_MP3_Decode(stream);
	} while(retval > 0);
	
	// if there was an error return nothing.
	if(retval < 0)
		return 0;
	
	return mp3info->destlen;
}

/*
=====================================================================
S_MP3_CodecLoad

We handle S_MP3_CodecLoad as a special case of the streaming functions 
where we read the whole stream at once.
+======================================================================
*/
void *S_MP3_CodecLoad(const char *filename, snd_info_t *info)
{
	snd_stream_t *stream;
	byte *pcmbuffer;

	// check if input is valid
	if(!filename)
		return NULL;

	stream = S_MP3_CodecOpenStream(filename);
	
	if(!stream)
		return NULL;
		
        // copy over the info
        info->rate = stream->info.rate;
        info->width = stream->info.width;
        info->channels = stream->info.channels;
        info->samples = stream->info.samples;
        info->dataofs = stream->info.dataofs;
	
	// allocate enough buffer for all pcm data
	pcmbuffer = (byte *)Hunk_AllocateTempMemory( stream->info.size );
	if(!pcmbuffer)
	{
		S_MP3_CodecCloseStream(stream);
		return NULL;
	}

	info->size = S_MP3_CodecReadStream(stream, stream->info.size, pcmbuffer);

	if(info->size <= 0)
	{
		// we didn't read anything at all. darn.
		Hunk_FreeTempMemory( pcmbuffer );
		pcmbuffer = NULL;
	}

	S_MP3_CodecCloseStream(stream);

	return pcmbuffer;
}
```


















