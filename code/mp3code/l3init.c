/*____________________________________________________________________________

	FreeAmp - The Free MP3 Player

        MP3 Decoder originally Copyright (C) 1995-1997 Xing Technology
        Corp.  http://www.xingtech.com

	Portions Copyright (C) 1998 EMusic.com

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

	$Id: l3init.c,v 1.2 1999/10/19 07:13:09 elrod Exp $
____________________________________________________________________________*/

/****  tinit.c  ***************************************************
  Layer III  init tables


******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "l3.h"

/*---------- quant ---------------------------------*/
/* 8 bit lookup x = pow(2.0, 0.25*(global_gain-210)) */
float *quant_init_global_addr();


/* x = pow(2.0, -0.5*(1+scalefact_scale)*scalefac + preemp) */
typedef float LS[4][32];
LS *quant_init_scale_addr();


float *quant_init_pow_addr();
float *quant_init_subblock_addr();

typedef int iARRAY22[22];
iARRAY22 *quant_init_band_addr();

/*---------- antialias ---------------------------------*/
typedef float PAIR[2];
PAIR *alias_init_addr();

static const float Ci[8] =
{
   -0.6f, -0.535f, -0.33f, -0.185f, -0.095f, -0.041f, -0.0142f, -0.0037f};


void hwin_init();		/* hybrid windows -- */
void imdct_init();
typedef struct
{
   float *w;
   float *w2;
   void *coef;
}
IMDCT_INIT_BLOCK;

void msis_init();
void msis_init_MPEG2();

/*=============================================================*/
int L3table_init()
{
   int i;
   float *x;
   LS *ls;
   int scalefact_scale, preemp, scalefac;
   double tmp;
   PAIR *csa;

   static int iOnceOnly = 0;

   	if (!iOnceOnly++)
	{
/*================ quant ===============================*/

	/* 8 bit plus 2 lookup x = pow(2.0, 0.25*(global_gain-210)) */
	/* extra 2 for ms scaling by 1/sqrt(2) */
	/* extra 4 for cvt to mono scaling by 1/2 */
	   x = quant_init_global_addr();
	   for (i = 0; i < 256 + 2 + 4; i++)
		  x[i] = (float) pow(2.0, 0.25 * ((i - (2 + 4)) - 210 + GLOBAL_GAIN_SCALE));



	/* x = pow(2.0, -0.5*(1+scalefact_scale)*scalefac + preemp) */
	   ls = quant_init_scale_addr();
	   for (scalefact_scale = 0; scalefact_scale < 2; scalefact_scale++)
	   {
		  for (preemp = 0; preemp < 4; preemp++)
		  {
		 for (scalefac = 0; scalefac < 32; scalefac++)
		 {
			ls[scalefact_scale][preemp][scalefac] =
			   (float) pow(2.0, -0.5 * (1 + scalefact_scale) * (scalefac + preemp));
		 }
		  }
	   }

	/*--- iSample**(4/3) lookup, -32<=i<=31 ---*/
	   x = quant_init_pow_addr();
	   for (i = 0; i < 64; i++)
	   {
		  tmp = i - 32;
		  x[i] = (float) (tmp * pow(fabs(tmp), (1.0 / 3.0)));
	   }


	/*-- pow(2.0, -0.25*8.0*subblock_gain)  3 bits --*/
	   x = quant_init_subblock_addr();
	   for (i = 0; i < 8; i++)
	   {
		  x[i] = (float) pow(2.0, 0.25 * -8.0 * i);
	   }

	/*-------------------------*/
	// quant_init_sf_band(sr_index);   replaced by code in sup.c


/*================ antialias ===============================*/
	   // onceonly!!!!!!!!!!!!!!!!!!!!!
	   csa = alias_init_addr();
	   for (i = 0; i < 8; i++)
	   {
		  csa[i][0] = (float) (1.0 / sqrt(1.0 + Ci[i] * Ci[i]));
		  csa[i][1] = (float) (Ci[i] / sqrt(1.0 + Ci[i] * Ci[i]));
	   }
   }

   // these 4 are iOnceOnly-protected inside...

/*================ msis ===============================*/
   msis_init();
   msis_init_MPEG2();

/*================ imdct ===============================*/
   imdct_init();

/*--- hybrid windows ------------*/
   hwin_init();

   return 0;
}
/*====================================================================*/
typedef float ARRAY36[36];
ARRAY36 *hwin_init_addr();

/*--------------------------------------------------------------------*/
void hwin_init()
{
   int i, j;
   double pi;
   ARRAY36 *win;

   static int iOnceOnly = 0;

   if (!iOnceOnly++)
   {
	   win = hwin_init_addr();

	   pi = 4.0 * atan(1.0);

	/* type 0 */
	   for (i = 0; i < 36; i++)
		  win[0][i] = (float) sin(pi / 36 * (i + 0.5));

	/* type 1 */
	   for (i = 0; i < 18; i++)
		  win[1][i] = (float) sin(pi / 36 * (i + 0.5));
	   for (i = 18; i < 24; i++)
		  win[1][i] = 1.0F;
	   for (i = 24; i < 30; i++)
		  win[1][i] = (float) sin(pi / 12 * (i + 0.5 - 18));
	   for (i = 30; i < 36; i++)
		  win[1][i] = 0.0F;

	/* type 3 */
	   for (i = 0; i < 6; i++)
		  win[3][i] = 0.0F;
	   for (i = 6; i < 12; i++)
		  win[3][i] = (float) sin(pi / 12 * (i + 0.5 - 6));
	   for (i = 12; i < 18; i++)
		  win[3][i] = 1.0F;
	   for (i = 18; i < 36; i++)
		  win[3][i] = (float) sin(pi / 36 * (i + 0.5));

	/* type 2 */
	   for (i = 0; i < 12; i++)
		  win[2][i] = (float) sin(pi / 12 * (i + 0.5));
	   for (i = 12; i < 36; i++)
		  win[2][i] = 0.0F;

	/*--- invert signs by region to match mdct 18pt --> 36pt mapping */
	   for (j = 0; j < 4; j++)
	   {
		  if (j == 2)
		 continue;
		  for (i = 9; i < 36; i++)
		 win[j][i] = -win[j][i];
	   }

	/*-- invert signs for short blocks --*/
	   for (i = 3; i < 12; i++)
		  win[2][i] = -win[2][i];
   }
}
/*=============================================================*/
typedef float ARRAY4[4];
const IMDCT_INIT_BLOCK *imdct_init_addr_18();
const IMDCT_INIT_BLOCK *imdct_init_addr_6();

/*-------------------------------------------------------------*/
void imdct_init()
{
   int k, p, n;
   double t, pi;
   const IMDCT_INIT_BLOCK *addr;
   float *w, *w2;
   float *v, *v2, *coef87;
   ARRAY4 *coef;

   static int iOnceOnly = 0;

   if (!iOnceOnly++)
   {
	/*--- 18 point --*/
	   addr = imdct_init_addr_18();
	   w = addr->w;
	   w2 = addr->w2;
	   coef = addr->coef;
	/*----*/
	   n = 18;
	   pi = 4.0 * atan(1.0);
	   t = pi / (4 * n);
	   for (p = 0; p < n; p++)
		  w[p] = (float) (2.0 * cos(t * (2 * p + 1)));
	   for (p = 0; p < 9; p++)
		  w2[p] = (float) (2.0 *cos(2 * t * (2 * p + 1)));

	   t = pi / (2 * n);
	   for (k = 0; k < 9; k++)
	   {
		  for (p = 0; p < 4; p++)
		 coef[k][p] = (float) cos(t * (2 * k) * (2 * p + 1));
	   }

	/*--- 6 point */
	   addr = imdct_init_addr_6();
	   v = addr->w;
	   v2 = addr->w2;
	   coef87 = addr->coef;
	/*----*/
	   n = 6;
	   pi = 4.0 * atan(1.0);
	   t = pi / (4 * n);
	   for (p = 0; p < n; p++)
		  v[p] = (float) (2.0 *cos(t * (2 * p + 1)));

	   for (p = 0; p < 3; p++)
		  v2[p] = (float) (2.0 *cos(2 * t * (2 * p + 1)));

	   t = pi / (2 * n);
	   k = 1;
	   p = 0;
	   *coef87 = (float) cos(t * (2 * k) * (2 * p + 1));
	/* adjust scaling to save a few mults */
	   for (p = 0; p < 6; p++)
		  v[p] = v[p] / 2.0f;
	   *coef87 = (float) (2.0 *(*coef87));

   }
}
/*===============================================================*/
typedef float ARRAY8_2[8][2];
ARRAY8_2 *msis_init_addr();

/*-------------------------------------------------------------*/
void msis_init()
{
   int i;
   double s, c;
   double pi;
   double t;
   ARRAY8_2 *lr;

   static int iOnceOnly = 0;

   if (!iOnceOnly++)
   {
	   lr = msis_init_addr();


	   pi = 4.0 * atan(1.0);
	   t = pi / 12.0;
	   for (i = 0; i < 7; i++)
	   {
		  s = sin(i * t);
		  c = cos(i * t);
		/* ms_mode = 0 */
		  lr[0][i][0] = (float) (s / (s + c));
		  lr[0][i][1] = (float) (c / (s + c));
		/* ms_mode = 1 */
		  lr[1][i][0] = (float) (sqrt(2.0) * (s / (s + c)));
		  lr[1][i][1] = (float) (sqrt(2.0) * (c / (s + c)));
	   }
	/* sf = 7 */
	/* ms_mode = 0 */
	   lr[0][i][0] = 1.0f;
	   lr[0][i][1] = 0.0f;
	/* ms_mode = 1, in is bands is routine does ms processing */
	   lr[1][i][0] = 1.0f;
	   lr[1][i][1] = 1.0f;


	/*-------
	for(i=0;i<21;i++) nBand[0][i] =
				sfBandTable[sr_index].l[i+1] - sfBandTable[sr_index].l[i];
	for(i=0;i<12;i++) nBand[1][i] =
				sfBandTable[sr_index].s[i+1] - sfBandTable[sr_index].s[i];
	-------------*/
   }
}
/*-------------------------------------------------------------*/
/*===============================================================*/
typedef float ARRAY2_64_2[2][64][2];
ARRAY2_64_2 *msis_init_addr_MPEG2();

/*-------------------------------------------------------------*/
void msis_init_MPEG2()
{
   int k, n;
   double t;
   ARRAY2_64_2 *lr2;
   int intensity_scale, ms_mode, sf, sflen;
   float ms_factor[2];

   static int iOnceOnly = 0;

   if (!iOnceOnly++)
   {
	   ms_factor[0] = 1.0;
	   ms_factor[1] = (float) sqrt(2.0);

	   lr2 = msis_init_addr_MPEG2();

	/* intensity stereo MPEG2 */
	/* lr2[intensity_scale][ms_mode][sflen_offset+sf][left/right] */

	   for (intensity_scale = 0; intensity_scale < 2; intensity_scale++)
	   {
		  t = pow(2.0, -0.25 * (1 + intensity_scale));
		  for (ms_mode = 0; ms_mode < 2; ms_mode++)
		  {

		 n = 1;
		 k = 0;
		 for (sflen = 0; sflen < 6; sflen++)
		 {
			for (sf = 0; sf < (n - 1); sf++, k++)
			{
			   if (sf == 0)
			   {
			  lr2[intensity_scale][ms_mode][k][0] = ms_factor[ms_mode] * 1.0f;
			  lr2[intensity_scale][ms_mode][k][1] = ms_factor[ms_mode] * 1.0f;
			   }
			   else if ((sf & 1))
			   {
			  lr2[intensity_scale][ms_mode][k][0] =
				 (float) (ms_factor[ms_mode] * pow(t, (sf + 1) / 2));
			  lr2[intensity_scale][ms_mode][k][1] = ms_factor[ms_mode] * 1.0f;
			   }
			   else
			   {
			  lr2[intensity_scale][ms_mode][k][0] = ms_factor[ms_mode] * 1.0f;
			  lr2[intensity_scale][ms_mode][k][1] =
				 (float) (ms_factor[ms_mode] * pow(t, sf / 2));
			   }
			}

		  /* illegal is_pos used to do ms processing */
			if (ms_mode == 0)
			{			/* ms_mode = 0 */
			   lr2[intensity_scale][ms_mode][k][0] = 1.0f;
			   lr2[intensity_scale][ms_mode][k][1] = 0.0f;
			}
			else
			{
			 /* ms_mode = 1, in is bands is routine does ms processing */
			   lr2[intensity_scale][ms_mode][k][0] = 1.0f;
			   lr2[intensity_scale][ms_mode][k][1] = 1.0f;
			}
			k++;
			n = n + n;
		 }
		  }
	   }
   }

}
/*-------------------------------------------------------------*/
