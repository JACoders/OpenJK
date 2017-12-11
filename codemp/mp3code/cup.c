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

	$Id: cup.c,v 1.3 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  cup.c  ***************************************************

MPEG audio decoder Layer I/II  mpeg1 and mpeg2
should be portable ANSI C, should be endian independent


mod  2/21/95 2/21/95  add bit skip, sb limiting

mods 11/15/95 for Layer I

******************************************************************/
/******************************************************************

       MPEG audio software decoder portable ANSI c.
       Decodes all Layer I/II to 16 bit linear pcm.
       Optional stereo to mono conversion.  Optional
       output sample rate conversion to half or quarter of
       native mpeg rate. dec8.c adds oupuut conversion features.

-------------------------------------
int audio_decode_init(MPEG_HEAD *h, int framebytes_arg,
         int reduction_code, int transform_code, int convert_code,
         int freq_limit)

initilize decoder:
       return 0 = fail, not 0 = success

MPEG_HEAD *h    input, mpeg header info (returned by call to head_info)
pMP3Stream->framebytes      input, mpeg frame size (returned by call to head_info)
reduction_code  input, sample rate reduction code
                    0 = full rate
                    1 = half rate
                    2 = quarter rate

transform_code  input, ignored
convert_code    input, channel conversion
                  convert_code:  0 = two chan output
                                 1 = convert two chan to mono
                                 2 = convert two chan to left chan
                                 3 = convert two chan to right chan
freq_limit      input, limits bandwidth of pcm output to specified
                frequency.  Special use. Set to 24000 for normal use.


---------------------------------
void audio_decode_info( DEC_INFO *info)

information return:
          Call after audio_decode_init.  See mhead.h for
          information returned in DEC_INFO structure.


---------------------------------
IN_OUT audio_decode(unsigned char *bs, void *pcmbuf)

decode one mpeg audio frame:
bs        input, mpeg bitstream, must start with
          sync word.  Caution: may read up to 3 bytes
          beyond end of frame.
pcmbuf    output, pcm samples.

IN_OUT structure returns:
          Number bytes conceptually removed from mpeg bitstream.
          Returns 0 if sync loss.
          Number bytes of pcm output.

*******************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "mhead.h"		/* mpeg header structure */

#include "mp3struct.h"


/*-------------------------------------------------------
NOTE:  Decoder may read up to three bytes beyond end of
frame.  Calling application must ensure that this does
not cause a memory access violation (protection fault)
---------------------------------------------------------*/

/*====================================================================*/
/*----------------*/
//@@@@ This next one (decinfo) is ok:
DEC_INFO decinfo;		/* global for Layer III */	// only written into by decode init funcs, then copied to stack struct higher up

/*----------------*/
static float look_c_value[18];	/* built by init */	// effectively constant

/*----------------*/
////@@@@static int pMP3Stream->outbytes;		// !!!!!!!!!!!!!!?
////@@@@static int pMP3Stream->framebytes;		// !!!!!!!!!!!!!!!!
////@@@@static int pMP3Stream->outvalues;		// !!!!!!!!!!!!?
////@@@@static int pad;
static const int look_joint[16] =
{				/* lookup stereo sb's by mode+ext */
   64, 64, 64, 64,		/* stereo */
   2 * 4, 2 * 8, 2 * 12, 2 * 16,	/* joint */
   64, 64, 64, 64,		/* dual */
   32, 32, 32, 32,		/* mono */
};

/*----------------*/
////@@@@static int max_sb;		// !!!!!!!!! L1, 2 3
////@@@@static int stereo_sb;

/*----------------*/
////@@@@static int pMP3Stream->nsb_limit = 6;
////@@@@static int bit_skip;
static const int bat_bit_master[] =
{
   0, 5, 7, 9, 10, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48};

/*----------------*/
////@@@@static int nbat[4] = {3, 8, 12, 7};	// !!!!!!!!!!!!! not constant!!!!
////@@@@static int bat[4][16];	// built as constant, but built according to header type (sigh)
static int ballo[64];		/* set by unpack_ba */					// scratchpad
static unsigned int samp_dispatch[66];	/* set by unpack_ba */		// scratchpad?
static float c_value[64];	/* set by unpack_ba */					// scratchpad

/*----------------*/
static unsigned int sf_dispatch[66];	/* set by unpack_ba */		// scratchpad?
static float sf_table[64];		// effectively constant
////@@@@ static float cs_factor[3][64];

/*----------------*/
////@@@@FINDME - groan....  (I shoved a *2 in just in case it needed it for stereo. This whole thing is crap now
float sample[2304*2];		/* global for use by Later 3 */	// !!!!!!!!!!!!!!!!!!!!!! // scratchpad?
static signed char group3_table[32][3];		// effectively constant
static signed char group5_table[128][3];	// effectively constant
static signed short group9_table[1024][3];	// effectively constant

/*----------------*/

////@@@@typedef void (*SBT_FUNCTION) (float *sample, short *pcm, int n);
void sbt_mono(float *sample, short *pcm, int n);
void sbt_dual(float *sample, short *pcm, int n);
////@@@@static SBT_FUNCTION sbt = sbt_mono;


typedef IN_OUT(*AUDIO_DECODE_ROUTINE) (unsigned char *bs, signed short *pcm);
IN_OUT L2audio_decode(unsigned char *bs, signed short *pcm);
static AUDIO_DECODE_ROUTINE audio_decode_routine = L2audio_decode;

/*======================================================================*/
/*======================================================================*/
/* get bits from bitstream in endian independent way */
////@@@@ FINDME - this stuff doesn't appear to be used by any of our samples (phew)
static unsigned char *bs_ptr;
static unsigned long bitbuf;
static int bits;
static long bitval;

/*------------- initialize bit getter -------------*/
static void load_init(unsigned char *buf)
{
   bs_ptr = buf;
   bits = 0;
   bitbuf = 0;
}
/*------------- get n bits from bitstream -------------*/
static long load(int n)
{
   unsigned long x;

   if (bits < n)
   {				/* refill bit buf if necessary */
      while (bits <= 24)
      {
	 bitbuf = (bitbuf << 8) | *bs_ptr++;
	 bits += 8;
      }
   }
   bits -= n;
   x = bitbuf >> bits;
   bitbuf -= x << bits;
   return x;
}
/*------------- skip over n bits in bitstream -------------*/
static void skip(int n)
{
   int k;

   if (bits < n)
   {
      n -= bits;
      k = n >> 3;
/*--- bytes = n/8 --*/
      bs_ptr += k;
      n -= k << 3;
      bitbuf = *bs_ptr++;
      bits = 8;
   }
   bits -= n;
   bitbuf -= (bitbuf >> bits) << bits;
}
/*--------------------------------------------------------------*/
#define mac_load_check(n) if( bits < (n) ) {                           \
          while( bits <= 24 ) {               \
             bitbuf = (bitbuf << 8) | *bs_ptr++;  \
             bits += 8;                       \
          }                                   \
   }
/*--------------------------------------------------------------*/
#define mac_load(n) ( bits -= n,                    \
         bitval = bitbuf >> bits,      \
         bitbuf -= bitval << bits,     \
         bitval )
/*======================================================================*/
static void unpack_ba()
{
   int i, j, k;
   static int nbit[4] =
   {4, 4, 3, 2};
   int nstereo;

   pMP3Stream->bit_skip = 0;
   nstereo = pMP3Stream->stereo_sb;
   k = 0;
   for (i = 0; i < 4; i++)
   {
      for (j = 0; j < pMP3Stream->nbat[i]; j++, k++)
      {
	 mac_load_check(4);
	 ballo[k] = samp_dispatch[k] = pMP3Stream->bat[i][mac_load(nbit[i])];
	 if (k >= pMP3Stream->nsb_limit)
	    pMP3Stream->bit_skip += bat_bit_master[samp_dispatch[k]];
	 c_value[k] = look_c_value[samp_dispatch[k]];
	 if (--nstereo < 0)
	 {
	    ballo[k + 1] = ballo[k];
	    samp_dispatch[k] += 18;	/* flag as joint */
	    samp_dispatch[k + 1] = samp_dispatch[k];	/* flag for sf */
	    c_value[k + 1] = c_value[k];
	    k++;
	    j++;
	 }
      }
   }
   samp_dispatch[pMP3Stream->nsb_limit] = 37;	/* terminate the dispatcher with skip */
   samp_dispatch[k] = 36;	/* terminate the dispatcher */

}
/*-------------------------------------------------------------------------*/
static void unpack_sfs()	/* unpack scale factor selectors */
{
   int i;

   for (i = 0; i < pMP3Stream->max_sb; i++)
   {
      mac_load_check(2);
      if (ballo[i])
	 sf_dispatch[i] = mac_load(2);
      else
	 sf_dispatch[i] = 4;	/* no allo */
   }
   sf_dispatch[i] = 5;		/* terminate dispatcher */
}
/*-------------------------------------------------------------------------*/
static void unpack_sf()		/* unpack scale factor */
{				/* combine dequant and scale factors */
   int i;

   i = -1;
 dispatch:switch (sf_dispatch[++i])
   {
      case 0:			/* 3 factors 012 */
	 mac_load_check(18);
	 pMP3Stream->cs_factor[0][i] = c_value[i] * sf_table[mac_load(6)];
	 pMP3Stream->cs_factor[1][i] = c_value[i] * sf_table[mac_load(6)];
	 pMP3Stream->cs_factor[2][i] = c_value[i] * sf_table[mac_load(6)];
	 goto dispatch;
      case 1:			/* 2 factors 002 */
	 mac_load_check(12);
	 pMP3Stream->cs_factor[1][i] = pMP3Stream->cs_factor[0][i] = c_value[i] * sf_table[mac_load(6)];
	 pMP3Stream->cs_factor[2][i] = c_value[i] * sf_table[mac_load(6)];
	 goto dispatch;
      case 2:			/* 1 factor 000 */
	 mac_load_check(6);
	 pMP3Stream->cs_factor[2][i] = pMP3Stream->cs_factor[1][i] = pMP3Stream->cs_factor[0][i] =
	    c_value[i] * sf_table[mac_load(6)];
	 goto dispatch;
      case 3:			/* 2 factors 022 */
	 mac_load_check(12);
	 pMP3Stream->cs_factor[0][i] = c_value[i] * sf_table[mac_load(6)];
	 pMP3Stream->cs_factor[2][i] = pMP3Stream->cs_factor[1][i] = c_value[i] * sf_table[mac_load(6)];
	 goto dispatch;
      case 4:			/* no allo */
/*-- pMP3Stream->cs_factor[2][i] = pMP3Stream->cs_factor[1][i] = pMP3Stream->cs_factor[0][i] = 0.0;  --*/
	 goto dispatch;
      case 5:			/* all done */
	 ;
   }				/* end switch */
}
/*-------------------------------------------------------------------------*/
#define UNPACK_N(n) s[k]     =  pMP3Stream->cs_factor[i][k]*(load(n)-((1 << (n-1)) -1));   \
    s[k+64]  =  pMP3Stream->cs_factor[i][k]*(load(n)-((1 << (n-1)) -1));   \
    s[k+128] =  pMP3Stream->cs_factor[i][k]*(load(n)-((1 << (n-1)) -1));   \
    goto dispatch;
#define UNPACK_N2(n) mac_load_check(3*n);                                         \
    s[k]     =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    s[k+64]  =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    s[k+128] =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    goto dispatch;
#define UNPACK_N3(n) mac_load_check(2*n);                                         \
    s[k]     =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    s[k+64]  =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    mac_load_check(n);                                           \
    s[k+128] =  pMP3Stream->cs_factor[i][k]*(mac_load(n)-((1 << (n-1)) -1));   \
    goto dispatch;
#define UNPACKJ_N(n) tmp        =  (load(n)-((1 << (n-1)) -1));                \
    s[k]       =  pMP3Stream->cs_factor[i][k]*tmp;                       \
    s[k+1]     =  pMP3Stream->cs_factor[i][k+1]*tmp;                     \
    tmp        =  (load(n)-((1 << (n-1)) -1));                 \
    s[k+64]    =  pMP3Stream->cs_factor[i][k]*tmp;                       \
    s[k+64+1]  =  pMP3Stream->cs_factor[i][k+1]*tmp;                     \
    tmp        =  (load(n)-((1 << (n-1)) -1));                 \
    s[k+128]   =  pMP3Stream->cs_factor[i][k]*tmp;                       \
    s[k+128+1] =  pMP3Stream->cs_factor[i][k+1]*tmp;                     \
    k++;       /* skip right chan dispatch */                \
    goto dispatch;
/*-------------------------------------------------------------------------*/
static void unpack_samp()	/* unpack samples */
{
   int i, j, k;
   float *s;
   int n;
   long tmp;

   s = sample;
   for (i = 0; i < 3; i++)
   {				/* 3 groups of scale factors */
      for (j = 0; j < 4; j++)
      {
	 k = -1;
       dispatch:switch (samp_dispatch[++k])
	 {
	    case 0:
	       s[k + 128] = s[k + 64] = s[k] = 0.0F;
	       goto dispatch;
	    case 1:		/* 3 levels grouped 5 bits */
	       mac_load_check(5);
	       n = mac_load(5);
	       s[k] = pMP3Stream->cs_factor[i][k] * group3_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group3_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group3_table[n][2];
	       goto dispatch;
	    case 2:		/* 5 levels grouped 7 bits */
	       mac_load_check(7);
	       n = mac_load(7);
	       s[k] = pMP3Stream->cs_factor[i][k] * group5_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group5_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group5_table[n][2];
	       goto dispatch;
	    case 3:
	       UNPACK_N2(3)	/* 7 levels */
	    case 4:		/* 9 levels grouped 10 bits */
	       mac_load_check(10);
	       n = mac_load(10);
	       s[k] = pMP3Stream->cs_factor[i][k] * group9_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group9_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group9_table[n][2];
	       goto dispatch;
	    case 5:
	       UNPACK_N2(4)	/* 15 levels */
	    case 6:
	       UNPACK_N2(5)	/* 31 levels */
	    case 7:
	       UNPACK_N2(6)	/* 63 levels */
	    case 8:
	       UNPACK_N2(7)	/* 127 levels */
	    case 9:
	       UNPACK_N2(8)	/* 255 levels */
	    case 10:
	       UNPACK_N3(9)	/* 511 levels */
	    case 11:
	       UNPACK_N3(10)	/* 1023 levels */
	    case 12:
	       UNPACK_N3(11)	/* 2047 levels */
	    case 13:
	       UNPACK_N3(12)	/* 4095 levels */
	    case 14:
	       UNPACK_N(13)	/* 8191 levels */
	    case 15:
	       UNPACK_N(14)	/* 16383 levels */
	    case 16:
	       UNPACK_N(15)	/* 32767 levels */
	    case 17:
	       UNPACK_N(16)	/* 65535 levels */
/* -- joint ---- */
	    case 18 + 0:
	       s[k + 128 + 1] = s[k + 128] = s[k + 64 + 1] = s[k + 64] = s[k + 1] = s[k] = 0.0F;
	       k++;		/* skip right chan dispatch */
	       goto dispatch;
	    case 18 + 1:	/* 3 levels grouped 5 bits */
	       n = load(5);
	       s[k] = pMP3Stream->cs_factor[i][k] * group3_table[n][0];
	       s[k + 1] = pMP3Stream->cs_factor[i][k + 1] * group3_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group3_table[n][1];
	       s[k + 64 + 1] = pMP3Stream->cs_factor[i][k + 1] * group3_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group3_table[n][2];
	       s[k + 128 + 1] = pMP3Stream->cs_factor[i][k + 1] * group3_table[n][2];
	       k++;		/* skip right chan dispatch */
	       goto dispatch;
	    case 18 + 2:	/* 5 levels grouped 7 bits */
	       n = load(7);
	       s[k] = pMP3Stream->cs_factor[i][k] * group5_table[n][0];
	       s[k + 1] = pMP3Stream->cs_factor[i][k + 1] * group5_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group5_table[n][1];
	       s[k + 64 + 1] = pMP3Stream->cs_factor[i][k + 1] * group5_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group5_table[n][2];
	       s[k + 128 + 1] = pMP3Stream->cs_factor[i][k + 1] * group5_table[n][2];
	       k++;		/* skip right chan dispatch */
	       goto dispatch;
	    case 18 + 3:
	       UNPACKJ_N(3)	/* 7 levels */
	    case 18 + 4:	/* 9 levels grouped 10 bits */
	       n = load(10);
	       s[k] = pMP3Stream->cs_factor[i][k] * group9_table[n][0];
	       s[k + 1] = pMP3Stream->cs_factor[i][k + 1] * group9_table[n][0];
	       s[k + 64] = pMP3Stream->cs_factor[i][k] * group9_table[n][1];
	       s[k + 64 + 1] = pMP3Stream->cs_factor[i][k + 1] * group9_table[n][1];
	       s[k + 128] = pMP3Stream->cs_factor[i][k] * group9_table[n][2];
	       s[k + 128 + 1] = pMP3Stream->cs_factor[i][k + 1] * group9_table[n][2];
	       k++;		/* skip right chan dispatch */
	       goto dispatch;
	    case 18 + 5:
	       UNPACKJ_N(4)	/* 15 levels */
	    case 18 + 6:
	       UNPACKJ_N(5)	/* 31 levels */
	    case 18 + 7:
	       UNPACKJ_N(6)	/* 63 levels */
	    case 18 + 8:
	       UNPACKJ_N(7)	/* 127 levels */
	    case 18 + 9:
	       UNPACKJ_N(8)	/* 255 levels */
	    case 18 + 10:
	       UNPACKJ_N(9)	/* 511 levels */
	    case 18 + 11:
	       UNPACKJ_N(10)	/* 1023 levels */
	    case 18 + 12:
	       UNPACKJ_N(11)	/* 2047 levels */
	    case 18 + 13:
	       UNPACKJ_N(12)	/* 4095 levels */
	    case 18 + 14:
	       UNPACKJ_N(13)	/* 8191 levels */
	    case 18 + 15:
	       UNPACKJ_N(14)	/* 16383 levels */
	    case 18 + 16:
	       UNPACKJ_N(15)	/* 32767 levels */
	    case 18 + 17:
	       UNPACKJ_N(16)	/* 65535 levels */
/* -- end of dispatch -- */
	    case 37:
	       skip(pMP3Stream->bit_skip);
	    case 36:
	       s += 3 * 64;
	 }			/* end switch */
      }				/* end j loop */
   }				/* end i loop */


}
/*-------------------------------------------------------------------------*/
unsigned char *gpNextByteAfterData = NULL;
IN_OUT audio_decode(unsigned char *bs, signed short *pcm, unsigned char *pNextByteAfterData)
{
	gpNextByteAfterData = pNextByteAfterData;	// sigh....
   return audio_decode_routine(bs, pcm);
}
/*-------------------------------------------------------------------------*/
IN_OUT L2audio_decode(unsigned char *bs, signed short *pcm)
{
   int sync, prot;
   IN_OUT in_out;

   load_init(bs);		/* initialize bit getter */
/* test sync */
   in_out.in_bytes = 0;		/* assume fail */
   in_out.out_bytes = 0;
   sync = load(12);
   if (sync != 0xFFF)
      return in_out;		/* sync fail */

   load(3);			/* skip id and option (checked by init) */
   prot = load(1);		/* load prot bit */
   load(6);			/* skip to pad */
   pMP3Stream->pad = load(1);
   load(1);			/* skip to mode */
   pMP3Stream->stereo_sb = look_joint[load(4)];
   if (prot)
      load(4);			/* skip to data */
   else
      load(20);			/* skip crc */

   unpack_ba();			/* unpack bit allocation */
   unpack_sfs();		/* unpack scale factor selectors */
   unpack_sf();			/* unpack scale factor */
   unpack_samp();		/* unpack samples */

   pMP3Stream->sbt(sample, pcm, 36);
/*-----------*/
   in_out.in_bytes = pMP3Stream->framebytes + pMP3Stream->pad;
   in_out.out_bytes = pMP3Stream->outbytes;

   return in_out;
}
/*-------------------------------------------------------------------------*/
#define COMPILE_ME
#include "cupini.c"		/* initialization */
#include "cupl1.c"		/* Layer I */
/*-------------------------------------------------------------------------*/
