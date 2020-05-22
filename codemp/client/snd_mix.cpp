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

// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "client.h"
#include "snd_local.h"

portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
int 	*snd_p, snd_linear_count, snd_vol;
short	*snd_out;



// FIXME: proper fix for that ?
#if !defined(_MSC_VER) || !id386
void S_WriteLinearBlastStereo16 (void)
{
	int		i;
	int		val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = snd_p[i]>>8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i] = (short)0x8000;
		else
			snd_out[i] = val;

		val = snd_p[i+1]>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i+1] = (short)0x8000;
		else
			snd_out[i+1] = val;
	}
}
#else
unsigned int uiMMXAvailable = 0;	// leave as 32 bit
__declspec( naked ) void S_WriteLinearBlastStereo16 (void)
{
	__asm {

 push edi
 push ebx

 mov ecx,ds:dword ptr[snd_linear_count]		// snd_linear_count is always even at this point, but not nec. mult of 4
 mov ebx,ds:dword ptr[snd_p]
 mov edi,ds:dword ptr[snd_out]

 cmp		[uiMMXAvailable], dword ptr 0
 je			NoMMX

// writes 8 items (128 bits) per loop pass...
//
 cmp		ecx,8
 jb			NoMMX

LWLBLoopTop_MMX:

 movq		mm1,[-8+ebx+ecx*4]
 movq		mm0,[-16+ebx+ecx*4]
 movq		mm3,[-24+ebx+ecx*4]
 movq		mm2,[-32+ebx+ecx*4]
 psrad		mm0,8
 psrad		mm1,8
 psrad		mm2,8
 psrad		mm3,8
 packssdw	mm0,mm1
 packssdw	mm2,mm3
 movq		[-8+edi+ecx*2],mm0
 movq		[-16+edi+ecx*2],mm2

 sub		ecx,8
 cmp		ecx,8
 jae		LWLBLoopTop_MMX

 emms

 // now deal with any remaining count...
 //
 jecxz		LExit

NoMMX:

// writes 2 items (32 bits) per loop pass...
//
LWLBLoopTop:
 mov eax,ds:dword ptr[-8+ebx+ecx*4]
 sar eax,8
 cmp eax,07FFFh
 jg LClampHigh
 cmp eax,0FFFF8000h
 jnl LClampDone
 mov eax,0FFFF8000h
 jmp LClampDone
LClampHigh:
 mov eax,07FFFh
LClampDone:
 mov edx,ds:dword ptr[-4+ebx+ecx*4]
 sar edx,8
 cmp edx,07FFFh
 jg LClampHigh2
 cmp edx,0FFFF8000h
 jnl LClampDone2
 mov edx,0FFFF8000h
 jmp LClampDone2
LClampHigh2:
 mov edx,07FFFh
LClampDone2:
 shl edx,16
 and eax,0FFFFh
 or edx,eax
 mov ds:dword ptr[-4+edi+ecx*2],edx

 sub ecx,2
 jnz LWLBLoopTop

LExit:
 pop ebx
 pop edi
 ret
	}
}

#endif


void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		ls_paintedtime;

	snd_p = (int *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while (ls_paintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = ls_paintedtime & ((dma.samples>>1)-1);

		snd_out = (short *) pbuf + (lpos<<1);

		snd_linear_count = (dma.samples>>1) - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1;

	// write a linear blast of samples
		S_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		ls_paintedtime += (snd_linear_count>>1);

		if( CL_VideoRecording( ) )
		{
			if ( cls.state == CA_ACTIVE || cl_forceavidemo->integer) {
				CL_WriteAVIAudioFrame( (byte *)snd_out, snd_linear_count << 1 );
			}
		}
	}
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int 	*p;
	int 	step;
	int		val;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;


	if ( s_testsound->integer ) {
		int		i;
		int		count;

		// write a fixed sine wave
		count = (endtime - s_paintedtime);
		for (i=0 ; i<count ; i++)
			paintbuffer[i].left = paintbuffer[i].right = (int)(sin((s_paintedtime+i)*0.1)*20000*256);
	}


	if (dma.samplebits == 16 && dma.channels == 2)
	{	// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{	// general case
		p = (int *) paintbuffer;
		count = (endtime - s_paintedtime) * dma.channels;
		out_mask = dma.samples - 1;
		out_idx = s_paintedtime * dma.channels & out_mask;
		step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = (short)val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;
			while (count--)
			{
				val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < (short)0x8000)
					val = (short)0x8000;
				out[out_idx] = (short)((val>>8) + 128);
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/
static void S_PaintChannelFrom16( channel_t *ch, const sfx_t *sfx, int count, int sampleOffset, int bufferOffset )
{
	portable_samplepair_t	*pSamplesDest;
	int iData;
	float ofst = sampleOffset;

	int iLeftVol	= ch->leftvol  * snd_vol;
	int iRightVol	= ch->rightvol * snd_vol;

	pSamplesDest	= &paintbuffer[ bufferOffset ];

	for ( int i=0 ; i<count ; i++ )
	{
		iData = sfx->pSoundData[ (int)ofst ];

		pSamplesDest[i].left  += (iData * iLeftVol )>>8;
		pSamplesDest[i].right += (iData * iRightVol)>>8;
		if (ch->doppler && ch->dopplerScale > 1) {
			ofst += 1 * ch->dopplerScale;
		} else {
			ofst++;
		}
	}
}


void S_PaintChannelFromMP3( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset )
{
	int data;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;
	portable_samplepair_t	*samp;
	static short tempMP3Buffer[PAINTBUFFER_SIZE];

	MP3Stream_GetSamples( ch, sampleOffset, count, tempMP3Buffer, qfalse );	// qfalse = not stereo

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;
	sfx = tempMP3Buffer;

	samp = &paintbuffer[ bufferOffset ];


	while ( count & 3 ) {
		data = *sfx;
		samp->left += (data * leftvol)>>8;
		samp->right += (data * rightvol)>>8;

		sfx++;
		samp++;
		count--;
	}

	for ( i=0 ; i<count ; i += 4 ) {
		data = sfx[i];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		data = sfx[i+1];
		samp[i+1].left += (data * leftvol)>>8;
		samp[i+1].right += (data * rightvol)>>8;

		data = sfx[i+2];
		samp[i+2].left += (data * leftvol)>>8;
		samp[i+2].right += (data * rightvol)>>8;

		data = sfx[i+3];
		samp[i+3].left += (data * leftvol)>>8;
		samp[i+3].right += (data * rightvol)>>8;
	}
}


// subroutinised to save code dup (called twice)	-ste
//
void ChannelPaint(channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset)
{
	switch (sc->eSoundCompressionMethod)
	{
		case ct_16:

			S_PaintChannelFrom16		(ch, sc, count, sampleOffset, bufferOffset);
			break;

		case ct_MP3:

			S_PaintChannelFromMP3		(ch, sc, count, sampleOffset, bufferOffset);
			break;

		default:

			assert(0);	// debug aid, ignored in release. FIXME: Should we ERR_DROP here for badness-catch?
			break;
	}
}



void S_PaintChannels( int endtime ) {
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;
	int	normal_vol,voice_vol;

	snd_vol = normal_vol = s_volume->value*256;
	voice_vol  = (int)(s_volumeVoice->value*256);

//Com_Printf ("%i to %i\n", s_paintedtime, endtime);
	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// clear the paint buffer to either music or zeros
		if ( s_rawend < s_paintedtime ) {
			if ( s_rawend ) {
				//Com_DPrintf ("background sound underrun\n");
			}
			memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		} else {
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ ) {
				s = i&(MAX_RAW_SAMPLES-1);
				paintbuffer[i-s_paintedtime] = s_rawsamples[s];
			}
//		if (i != end)
//			Com_Printf ("partial stream\n");
//		else
//			Com_Printf ("full stream\n");
			for ( ; i < end ; i++ ) {
				paintbuffer[i-s_paintedtime].left =
				paintbuffer[i-s_paintedtime].right = 0;
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {
			if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
				continue;
			}

			if ( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL )
				snd_vol = voice_vol;
			else
				snd_vol = normal_vol;

			ltime = s_paintedtime;
			sc = ch->thesfx;

			// we might have to make 2 passes if it is
			//	a looping sound effect and the end of
			//	the sameple is hit...
			//
			do
			{
				if (ch->loopSound) {
					sampleOffset = ltime % sc->iSoundLengthInSamples;
				} else {
					sampleOffset = ltime - ch->startSample;
				}

				count = end - ltime;
				if ( ch->doppler && ch->dopplerScale > 1 ) {
					if ( sampleOffset + (count * ch->dopplerScale) > sc->iSoundLengthInSamples ) {
						count = (sc->iSoundLengthInSamples - sampleOffset) / ch->dopplerScale;

						// avoid infinite loop once length of remaining pSoundData (numerator)
						//	is smaller than dopplerScale (denominator), resulting in 0.
						if ( count == 0 ) {
							break;
						}
					}
				} else {
					if ( sampleOffset + count > sc->iSoundLengthInSamples ) {
						count = sc->iSoundLengthInSamples - sampleOffset;
					}
				}

				if ( count > 0 ) {
					ChannelPaint(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					ltime += count;
				}
			} while ( ltime < end && ch->loopSound );
		}
/* temprem
		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels ; i++, ch++ ) {
			if ( !ch->thesfx || (!ch->leftvol && !ch->rightvol )) {
				continue;
			}

			{

				ltime = s_paintedtime;
				sc = ch->thesfx;

				if (sc->soundData==NULL || sc->soundLength==0) {
					continue;
				}
				// we might have to make two passes if it
				// is a looping sound effect and the end of
				// the sample is hit
				do {
					sampleOffset = (ltime % sc->soundLength);

					count = end - ltime;
					if ( sampleOffset + count > sc->soundLength ) {
						count = sc->soundLength - sampleOffset;
					}

					if ( count > 0 )
					{
						ChannelPaint(ch, sc, count, sampleOffset, ltime - s_paintedtime);
						ltime += count;
					}

				} while ( ltime < end);
			}
		}
*/
		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		s_paintedtime = end;
	}
}
