#ifdef COMPILE_ME
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

	$Id: cupini.c,v 1.3 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/*=========================================================
 initialization for cup.c - include to cup.c
 mpeg audio decoder portable "c"

mod 8/6/96 add 8 bit output

mod 5/10/95 add quick (low precision) window

mod 5/16/95 sb limit for reduced samprate output
            changed from 94% to 100% of Nyquist sb

mod 11/15/95 for Layer I


=========================================================*/
/*-- compiler bug, floating constant overflow w/ansi --*/

static const long steps[18] =
{
   0, 3, 5, 7, 9, 15, 31, 63, 127,
   255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535};


/* ABCD_INDEX = lookqt[mode][sr_index][br_index]  */
/* -1 = invalid  */
static const signed char lookqt[4][3][16] =
{
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks stereo */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks joint stereo */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks dual chan */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
// mono extended beyond legal br index
//  1,2,2,0,0,0,1,1,1,1,1,1,1,1,1,-1,          /*  44ks single chan */
//  0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,-1,          /*  48ks */
//  1,3,3,0,0,0,1,1,1,1,1,1,1,1,1,-1,          /*  32ks */
// legal mono
 {{1, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1},	/*  44ks single chan */
  {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},	/*  48ks */
  {1, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1}},	/*  32ks */
};

static const long sr_table[8] =
{22050L, 24000L, 16000L, 1L,
 44100L, 48000L, 32000L, 1L};

/* bit allocation table look up */
/* table per mpeg spec tables 3b2a/b/c/d  /e is mpeg2 */
/* look_bat[abcd_index][4][16]  */
static const unsigned char look_bat[5][4][16] =
{
/* LOOK_BATA */
 {{0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17},
  {0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATB */
 {{0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17},
  {0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATC */
 {{0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATD */
 {{0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATE */
 {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

/* look_nbat[abcd_index]][4] */
static const unsigned char look_nbat[5][4] =
{
  {3, 8, 12, 4},
  {3, 8, 12, 7},
  {2, 0, 6, 0},
  {2, 0, 10, 0},
  {4, 0, 7, 19},
};


void sbt_mono(float *sample, short *pcm, int n);
void sbt_dual(float *sample, short *pcm, int n);
void sbt_dual_mono(float *sample, short *pcm, int n);
void sbt_dual_left(float *sample, short *pcm, int n);
void sbt_dual_right(float *sample, short *pcm, int n);
void sbt16_mono(float *sample, short *pcm, int n);
void sbt16_dual(float *sample, short *pcm, int n);
void sbt16_dual_mono(float *sample, short *pcm, int n);
void sbt16_dual_left(float *sample, short *pcm, int n);
void sbt16_dual_right(float *sample, short *pcm, int n);
void sbt8_mono(float *sample, short *pcm, int n);
void sbt8_dual(float *sample, short *pcm, int n);
void sbt8_dual_mono(float *sample, short *pcm, int n);
void sbt8_dual_left(float *sample, short *pcm, int n);
void sbt8_dual_right(float *sample, short *pcm, int n);

/*--- 8 bit output ---*/
void sbtB_mono(float *sample, unsigned char *pcm, int n);
void sbtB_dual(float *sample, unsigned char *pcm, int n);
void sbtB_dual_mono(float *sample, unsigned char *pcm, int n);
void sbtB_dual_left(float *sample, unsigned char *pcm, int n);
void sbtB_dual_right(float *sample, unsigned char *pcm, int n);
void sbtB16_mono(float *sample, unsigned char *pcm, int n);
void sbtB16_dual(float *sample, unsigned char *pcm, int n);
void sbtB16_dual_mono(float *sample, unsigned char *pcm, int n);
void sbtB16_dual_left(float *sample, unsigned char *pcm, int n);
void sbtB16_dual_right(float *sample, unsigned char *pcm, int n);
void sbtB8_mono(float *sample, unsigned char *pcm, int n);
void sbtB8_dual(float *sample, unsigned char *pcm, int n);
void sbtB8_dual_mono(float *sample, unsigned char *pcm, int n);
void sbtB8_dual_left(float *sample, unsigned char *pcm, int n);
void sbtB8_dual_right(float *sample, unsigned char *pcm, int n);


static const SBT_FUNCTION sbt_table[2][3][5] =
{
 {{sbt_mono, sbt_dual, sbt_dual_mono, sbt_dual_left, sbt_dual_right},
  {sbt16_mono, sbt16_dual, sbt16_dual_mono, sbt16_dual_left, sbt16_dual_right},
  {sbt8_mono, sbt8_dual, sbt8_dual_mono, sbt8_dual_left, sbt8_dual_right}},
 {{(SBT_FUNCTION) sbtB_mono,
   (SBT_FUNCTION) sbtB_dual,
   (SBT_FUNCTION) sbtB_dual_mono,
   (SBT_FUNCTION) sbtB_dual_left,
   (SBT_FUNCTION) sbtB_dual_right},
  {(SBT_FUNCTION) sbtB16_mono,
   (SBT_FUNCTION) sbtB16_dual,
   (SBT_FUNCTION) sbtB16_dual_mono,
   (SBT_FUNCTION) sbtB16_dual_left,
   (SBT_FUNCTION) sbtB16_dual_right},
  {(SBT_FUNCTION) sbtB8_mono,
   (SBT_FUNCTION) sbtB8_dual,
   (SBT_FUNCTION) sbtB8_dual_mono,
   (SBT_FUNCTION) sbtB8_dual_left,
   (SBT_FUNCTION) sbtB8_dual_right}},
};

static const int out_chans[5] =
{1, 2, 1, 1, 1};


int audio_decode_initL1(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit);
void sbt_init();


IN_OUT L1audio_decode(unsigned char *bs, signed short *pcm);
IN_OUT L2audio_decode(unsigned char *bs, signed short *pcm);
IN_OUT L3audio_decode(unsigned char *bs, unsigned char *pcm);
static const AUDIO_DECODE_ROUTINE decode_routine_table[4] =
{
   L2audio_decode,
   (AUDIO_DECODE_ROUTINE)L3audio_decode,
   L2audio_decode,
   L1audio_decode,};

/*---------------------------------------------------------*/
static void table_init()
{
   int i, j;
   int code;
   static int iOnceOnly=0;

   if (!iOnceOnly++)
   {
	/*--  c_values (dequant) --*/
	   for (i = 1; i < 18; i++)
		  look_c_value[i] = 2.0F / steps[i];

	/*--  scale factor table, scale by 32768 for 16 pcm output  --*/
	   for (i = 0; i < 64; i++)
		  sf_table[i] = (float) (32768.0 * 2.0 * pow(2.0, -i / 3.0));

	/*--  grouped 3 level lookup table 5 bit token --*/
	   for (i = 0; i < 32; i++)
	   {
		  code = i;
		  for (j = 0; j < 3; j++)
		  {
		 group3_table[i][j] = (char) ((code % 3) - 1);
		 code /= 3;
		  }
	   }

	/*--  grouped 5 level lookup table 7 bit token --*/
	   for (i = 0; i < 128; i++)
	   {
		  code = i;
		  for (j = 0; j < 3; j++)
		  {
		 group5_table[i][j] = (char) ((code % 5) - 2);
		 code /= 5;
		  }
	   }

	/*--  grouped 9 level lookup table 10 bit token --*/
	   for (i = 0; i < 1024; i++)
	   {
		  code = i;
		  for (j = 0; j < 3; j++)
		  {
		 group9_table[i][j] = (short) ((code % 9) - 4);
		 code /= 9;
		  }
	   }
   }
}
/*---------------------------------------------------------*/
int L1audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit);
int L3audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit);

/*---------------------------------------------------------*/
/* mpeg_head defined in mhead.h  frame bytes is without pad */
int audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
		      int freq_limit)
{
   int i, j, k;
   static int first_pass = 1;
   int abcd_index;
   long samprate;
   int limit;
   int bit_code;

   if (first_pass)
   {
      table_init();
      first_pass = 0;
   }

/* select decoder routine Layer I,II,III */
   audio_decode_routine = decode_routine_table[h->option & 3];


   if (h->option == 3)		/* layer I */
      return L1audio_decode_init(h, framebytes_arg,
		  reduction_code, transform_code, convert_code, freq_limit);

   if (h->option == 1)		/* layer III */
      return L3audio_decode_init(h, framebytes_arg,
		  reduction_code, transform_code, convert_code, freq_limit);



   bit_code = 0;
   if (convert_code & 8)
      bit_code = 1;
   convert_code = convert_code & 3;	/* higher bits used by dec8 freq cvt */
   if (reduction_code < 0)
      reduction_code = 0;
   if (reduction_code > 2)
      reduction_code = 2;
   if (freq_limit < 1000)
      freq_limit = 1000;


   pMP3Stream->framebytes = framebytes_arg;
/* check if code handles */
   if (h->option != 2)
      return 0;			/* layer II only */
   if (h->sr_index == 3)
      return 0;			/* reserved */

/* compute abcd index for bit allo table selection */
   if (h->id)			/* mpeg 1 */
      abcd_index = lookqt[h->mode][h->sr_index][h->br_index];
   else
      abcd_index = 4;		/* mpeg 2 */

   if (abcd_index < 0)
      return 0;			// fail invalid Layer II bit rate index

   for (i = 0; i < 4; i++)
      for (j = 0; j < 16; j++)
		pMP3Stream->bat[i][j] = look_bat[abcd_index][i][j];
   for (i = 0; i < 4; i++)
      pMP3Stream->nbat[i] = look_nbat[abcd_index][i];
   pMP3Stream->max_sb = pMP3Stream->nbat[0] + pMP3Stream->nbat[1] + pMP3Stream->nbat[2] + pMP3Stream->nbat[3];
/*----- compute pMP3Stream->nsb_limit --------*/
   samprate = sr_table[4 * h->id + h->sr_index];
   pMP3Stream->nsb_limit = (freq_limit * 64L + samprate / 2) / samprate;
/*- caller limit -*/
/*---- limit = 0.94*(32>>reduction_code);  ----*/
   limit = (32 >> reduction_code);
   if (limit > 8)
      limit--;
   if (pMP3Stream->nsb_limit > limit)
      pMP3Stream->nsb_limit = limit;
   if (pMP3Stream->nsb_limit > pMP3Stream->max_sb)
      pMP3Stream->nsb_limit = pMP3Stream->max_sb;

   pMP3Stream->outvalues = 1152 >> reduction_code;
   if (h->mode != 3)
   {				/* adjust for 2 channel modes */
      for (i = 0; i < 4; i++)
		pMP3Stream->nbat[i] *= 2;
      pMP3Stream->max_sb *= 2;
      pMP3Stream->nsb_limit *= 2;
   }

/* set sbt function */
   k = 1 + convert_code;
   if (h->mode == 3)
   {
      k = 0;
   }
   pMP3Stream->sbt = sbt_table[bit_code][reduction_code][k];
   pMP3Stream->outvalues *= out_chans[k];
   if (bit_code)
      pMP3Stream->outbytes = pMP3Stream->outvalues;
   else
      pMP3Stream->outbytes = sizeof(short) * pMP3Stream->outvalues;

   decinfo.channels = out_chans[k];
   decinfo.outvalues = pMP3Stream->outvalues;
   decinfo.samprate = samprate >> reduction_code;
   if (bit_code)
      decinfo.bits = 8;
   else
      decinfo.bits = sizeof(short) * 8;

   decinfo.framebytes = pMP3Stream->framebytes;
   decinfo.type = 0;



/* clear sample buffer, unused sub bands must be 0 */
   for (i = 0; i < 2304*2; i++)	// the *2 here was inserted by me just in case, since the array is now *2, because of stereo files unpacking at 4608 bytes per frame (which may or may not be relevant, but in any case I don't think we use the L1 versions of MP3 now anyway
      sample[i] = 0.0F;


/* init sub-band transform */
   sbt_init();

   return 1;
}
/*---------------------------------------------------------*/
void audio_decode_info(DEC_INFO * info)
{
   *info = decinfo;		/* info return, call after init */
}
/*---------------------------------------------------------*/
void decode_table_init()
{
/* dummy for asm version compatability */
}
/*---------------------------------------------------------*/
#endif	// #ifdef COMPILE_ME

