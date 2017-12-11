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

	$Id: cupL1.c,v 1.3 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  cupL1.c  ***************************************************

MPEG audio decoder Layer I mpeg1 and mpeg2

include to clup.c


******************************************************************/
/*======================================================================*/
static const int bat_bit_masterL1[] =
{
   0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};
////@@@@static float *pMP3Stream->cs_factorL1 = &pMP3Stream->cs_factor[0];	// !!!!!!!!!!!!!!!!
static float look_c_valueL1[16];	// effectively constant
////@@@@static int nbatL1 = 32;

/*======================================================================*/
static void unpack_baL1()
{
   int j;
   int nstereo;

   pMP3Stream->bit_skip = 0;
   nstereo = pMP3Stream->stereo_sb;

   for (j = 0; j < pMP3Stream->nbatL1; j++)
   {
      mac_load_check(4);
      ballo[j] = samp_dispatch[j] = mac_load(4);
      if (j >= pMP3Stream->nsb_limit)
		pMP3Stream->bit_skip += bat_bit_masterL1[samp_dispatch[j]];
      c_value[j] = look_c_valueL1[samp_dispatch[j]];
      if (--nstereo < 0)
      {
	 ballo[j + 1] = ballo[j];
	 samp_dispatch[j] += 15;	/* flag as joint */
	 samp_dispatch[j + 1] = samp_dispatch[j];	/* flag for sf */
	 c_value[j + 1] = c_value[j];
	 j++;
      }
   }
/*-- terminate with bit skip and end --*/
   samp_dispatch[pMP3Stream->nsb_limit] = 31;
   samp_dispatch[j] = 30;
}
/*-------------------------------------------------------------------------*/
static void unpack_sfL1(void)	/* unpack scale factor */
{				/* combine dequant and scale factors */
   int i;

   for (i = 0; i < pMP3Stream->nbatL1; i++)
   {
      if (ballo[i])
      {
	 mac_load_check(6);
	 pMP3Stream->cs_factorL1[i] = c_value[i] * sf_table[mac_load(6)];
      }
   }
/*-- done --*/
}
/*-------------------------------------------------------------------------*/
#define UNPACKL1_N(n) s[k]     =  pMP3Stream->cs_factorL1[k]*(load(n)-((1 << (n-1)) -1));  \
    goto dispatch;
#define UNPACKL1J_N(n) tmp        =  (load(n)-((1 << (n-1)) -1));                 \
    s[k]       =  pMP3Stream->cs_factorL1[k]*tmp;                        \
    s[k+1]     =  pMP3Stream->cs_factorL1[k+1]*tmp;                      \
    k++;                                                     \
    goto dispatch;
/*-------------------------------------------------------------------------*/
static void unpack_sampL1()	/* unpack samples */
{
   int j, k;
   float *s;
   long tmp;

   s = sample;
   for (j = 0; j < 12; j++)
   {
      k = -1;
    dispatch:switch (samp_dispatch[++k])
      {
	 case 0:
	    s[k] = 0.0F;
	    goto dispatch;
	 case 1:
	    UNPACKL1_N(2)	/*  3 levels */
	 case 2:
	    UNPACKL1_N(3)	/*  7 levels */
	 case 3:
	    UNPACKL1_N(4)	/* 15 levels */
	 case 4:
	    UNPACKL1_N(5)	/* 31 levels */
	 case 5:
	    UNPACKL1_N(6)	/* 63 levels */
	 case 6:
	    UNPACKL1_N(7)	/* 127 levels */
	 case 7:
	    UNPACKL1_N(8)	/* 255 levels */
	 case 8:
	    UNPACKL1_N(9)	/* 511 levels */
	 case 9:
	    UNPACKL1_N(10)	/* 1023 levels */
	 case 10:
	    UNPACKL1_N(11)	/* 2047 levels */
	 case 11:
	    UNPACKL1_N(12)	/* 4095 levels */
	 case 12:
	    UNPACKL1_N(13)	/* 8191 levels */
	 case 13:
	    UNPACKL1_N(14)	/* 16383 levels */
	 case 14:
	    UNPACKL1_N(15)	/* 32767 levels */
/* -- joint ---- */
	 case 15 + 0:
	    s[k + 1] = s[k] = 0.0F;
	    k++;		/* skip right chan dispatch */
	    goto dispatch;
/* -- joint ---- */
	 case 15 + 1:
	    UNPACKL1J_N(2)	/*  3 levels */
	 case 15 + 2:
	    UNPACKL1J_N(3)	/*  7 levels */
	 case 15 + 3:
	    UNPACKL1J_N(4)	/* 15 levels */
	 case 15 + 4:
	    UNPACKL1J_N(5)	/* 31 levels */
	 case 15 + 5:
	    UNPACKL1J_N(6)	/* 63 levels */
	 case 15 + 6:
	    UNPACKL1J_N(7)	/* 127 levels */
	 case 15 + 7:
	    UNPACKL1J_N(8)	/* 255 levels */
	 case 15 + 8:
	    UNPACKL1J_N(9)	/* 511 levels */
	 case 15 + 9:
	    UNPACKL1J_N(10)	/* 1023 levels */
	 case 15 + 10:
	    UNPACKL1J_N(11)	/* 2047 levels */
	 case 15 + 11:
	    UNPACKL1J_N(12)	/* 4095 levels */
	 case 15 + 12:
	    UNPACKL1J_N(13)	/* 8191 levels */
	 case 15 + 13:
	    UNPACKL1J_N(14)	/* 16383 levels */
	 case 15 + 14:
	    UNPACKL1J_N(15)	/* 32767 levels */

/* -- end of dispatch -- */
	 case 31:
	    skip(pMP3Stream->bit_skip);
	 case 30:
	    s += 64;
      }				/* end switch */
   }				/* end j loop */

/*-- done --*/
}
/*-------------------------------------------------------------------*/
IN_OUT L1audio_decode(unsigned char *bs, signed short *pcm)
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
   pMP3Stream->pad = (load(1)) << 2;
   load(1);			/* skip to mode */
   pMP3Stream->stereo_sb = look_joint[load(4)];
   if (prot)
      load(4);			/* skip to data */
   else
      load(20);			/* skip crc */

   unpack_baL1();		/* unpack bit allocation */
   unpack_sfL1();		/* unpack scale factor */
   unpack_sampL1();		/* unpack samples */

   pMP3Stream->sbt(sample, pcm, 12);
/*-----------*/
   in_out.in_bytes = pMP3Stream->framebytes + pMP3Stream->pad;
   in_out.out_bytes = pMP3Stream->outbytes;

   return in_out;
}
/*-------------------------------------------------------------------------*/
int L1audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit)
{
   int i, k;
   static int first_pass = 1;
   long samprate;
   int limit;
   long step;
   int bit_code;

/*--- sf init done by layer II init ---*/
   if (first_pass)
   {
      for (step = 4, i = 1; i < 16; i++, step <<= 1)
	 look_c_valueL1[i] = (float) (2.0 / (step - 1));
      first_pass = 0;
   }
   pMP3Stream->cs_factorL1 = pMP3Stream->cs_factor[0];

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
   if (h->option != 3)
      return 0;			/* layer I only */

   pMP3Stream->nbatL1 = 32;
   pMP3Stream->max_sb = pMP3Stream->nbatL1;
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

   pMP3Stream->outvalues = 384 >> reduction_code;
   if (h->mode != 3)
   {				/* adjust for 2 channel modes */
      pMP3Stream->nbatL1 *= 2;
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
   for (i = 0; i < 768; i++)
      sample[i] = 0.0F;


/* init sub-band transform */
   sbt_init();

   return 1;
}
/*---------------------------------------------------------*/
#endif	// #ifdef COMPILE_ME
