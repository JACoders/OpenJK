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

	$Id: cdct.c,v 1.11 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  cdct.c  ***************************************************

mod 5/16/95 first stage in 8 pt dct does not drop last sb mono


MPEG audio decoder, dct
portable C

******************************************************************/

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

float coef32[31];	/* 32 pt dct coefs */		// !!!!!!!!!!!!!!!!!! (only generated once (always to same value)

/*------------------------------------------------------------*/
float *dct_coef_addr()
{
   return coef32;
}
/*------------------------------------------------------------*/
static void forward_bf(int m, int n, float x[], float f[], float coef[])
{
   int i, j, n2;
   int p, q, p0, k;

   p0 = 0;
   n2 = n >> 1;
   for (i = 0; i < m; i++, p0 += n)
   {
      k = 0;
      p = p0;
      q = p + n - 1;
      for (j = 0; j < n2; j++, p++, q--, k++)
      {
	 f[p] = x[p] + x[q];
	 f[n2 + p] = coef[k] * (x[p] - x[q]);
      }
   }
}
/*------------------------------------------------------------*/
static void back_bf(int m, int n, float x[], float f[])
{
   int i, j, n2, n21;
   int p, q, p0;

   p0 = 0;
   n2 = n >> 1;
   n21 = n2 - 1;
   for (i = 0; i < m; i++, p0 += n)
   {
      p = p0;
      q = p0;
      for (j = 0; j < n2; j++, p += 2, q++)
	 f[p] = x[q];
      p = p0 + 1;
      for (j = 0; j < n21; j++, p += 2, q++)
	 f[p] = x[q] + x[q + 1];
      f[p] = x[q];
   }
}
/*------------------------------------------------------------*/


void fdct32(float x[], float c[])
{
   float a[32];			/* ping pong buffers */
   float b[32];
   int p, q;

   float *src = x;

/* special first stage */
   for (p = 0, q = 31; p < 16; p++, q--)
   {
      a[p] = src[p] + src[q];
      a[16 + p] = coef32[p] * (src[p] - src[q]);
   }
   forward_bf(2, 16, a, b, coef32 + 16);
   forward_bf(4, 8, b, a, coef32 + 16 + 8);
   forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
   forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
   back_bf(8, 4, a, b);
   back_bf(4, 8, b, a);
   back_bf(2, 16, a, b);
   back_bf(1, 32, b, c);
}
/*------------------------------------------------------------*/
void fdct32_dual(float x[], float c[])
{
   float a[32];			/* ping pong buffers */
   float b[32];
   int p, pp, qq;

/* special first stage for dual chan (interleaved x) */
   pp = 0;
   qq = 2 * 31;
   for (p = 0; p < 16; p++, pp += 2, qq -= 2)
   {
      a[p] = x[pp] + x[qq];
      a[16 + p] = coef32[p] * (x[pp] - x[qq]);
   }
   forward_bf(2, 16, a, b, coef32 + 16);
   forward_bf(4, 8, b, a, coef32 + 16 + 8);
   forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
   forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
   back_bf(8, 4, a, b);
   back_bf(4, 8, b, a);
   back_bf(2, 16, a, b);
   back_bf(1, 32, b, c);
}
/*---------------convert dual to mono------------------------------*/
void fdct32_dual_mono(float x[], float c[])
{
   float a[32];			/* ping pong buffers */
   float b[32];
   float t1, t2;
   int p, pp, qq;

/* special first stage  */
   pp = 0;
   qq = 2 * 31;
   for (p = 0; p < 16; p++, pp += 2, qq -= 2)
   {
      t1 = 0.5F * (x[pp] + x[pp + 1]);
      t2 = 0.5F * (x[qq] + x[qq + 1]);
      a[p] = t1 + t2;
      a[16 + p] = coef32[p] * (t1 - t2);
   }
   forward_bf(2, 16, a, b, coef32 + 16);
   forward_bf(4, 8, b, a, coef32 + 16 + 8);
   forward_bf(8, 4, a, b, coef32 + 16 + 8 + 4);
   forward_bf(16, 2, b, a, coef32 + 16 + 8 + 4 + 2);
   back_bf(8, 4, a, b);
   back_bf(4, 8, b, a);
   back_bf(2, 16, a, b);
   back_bf(1, 32, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct -------------------------------*/
void fdct16(float x[], float c[])
{
   float a[16];			/* ping pong buffers */
   float b[16];
   int p, q;

/* special first stage (drop highest sb) */
   a[0] = x[0];
   a[8] = coef32[16] * x[0];
   for (p = 1, q = 14; p < 8; p++, q--)
   {
      a[p] = x[p] + x[q];
      a[8 + p] = coef32[16 + p] * (x[p] - x[q]);
   }
   forward_bf(2, 8, a, b, coef32 + 16 + 8);
   forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(4, 4, b, a);
   back_bf(2, 8, a, b);
   back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct dual chan---------------------*/
void fdct16_dual(float x[], float c[])
{
   float a[16];			/* ping pong buffers */
   float b[16];
   int p, pp, qq;

/* special first stage for interleaved input */
   a[0] = x[0];
   a[8] = coef32[16] * x[0];
   pp = 2;
   qq = 2 * 14;
   for (p = 1; p < 8; p++, pp += 2, qq -= 2)
   {
      a[p] = x[pp] + x[qq];
      a[8 + p] = coef32[16 + p] * (x[pp] - x[qq]);
   }
   forward_bf(2, 8, a, b, coef32 + 16 + 8);
   forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(4, 4, b, a);
   back_bf(2, 8, a, b);
   back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 16 pt fdct dual to mono-------------------*/
void fdct16_dual_mono(float x[], float c[])
{
   float a[16];			/* ping pong buffers */
   float b[16];
   float t1, t2;
   int p, pp, qq;

/* special first stage  */
   a[0] = 0.5F * (x[0] + x[1]);
   a[8] = coef32[16] * a[0];
   pp = 2;
   qq = 2 * 14;
   for (p = 1; p < 8; p++, pp += 2, qq -= 2)
   {
      t1 = 0.5F * (x[pp] + x[pp + 1]);
      t2 = 0.5F * (x[qq] + x[qq + 1]);
      a[p] = t1 + t2;
      a[8 + p] = coef32[16 + p] * (t1 - t2);
   }
   forward_bf(2, 8, a, b, coef32 + 16 + 8);
   forward_bf(4, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(8, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(4, 4, b, a);
   back_bf(2, 8, a, b);
   back_bf(1, 16, b, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct -------------------------------*/
void fdct8(float x[], float c[])
{
   float a[8];			/* ping pong buffers */
   float b[8];
   int p, q;

/* special first stage  */

   b[0] = x[0] + x[7];
   b[4] = coef32[16 + 8] * (x[0] - x[7]);
   for (p = 1, q = 6; p < 4; p++, q--)
   {
      b[p] = x[p] + x[q];
      b[4 + p] = coef32[16 + 8 + p] * (x[p] - x[q]);
   }

   forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(2, 4, b, a);
   back_bf(1, 8, a, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct dual chan---------------------*/
void fdct8_dual(float x[], float c[])
{
   float a[8];			/* ping pong buffers */
   float b[8];
   int p, pp, qq;

/* special first stage for interleaved input */
   b[0] = x[0] + x[14];
   b[4] = coef32[16 + 8] * (x[0] - x[14]);
   pp = 2;
   qq = 2 * 6;
   for (p = 1; p < 4; p++, pp += 2, qq -= 2)
   {
      b[p] = x[pp] + x[qq];
      b[4 + p] = coef32[16 + 8 + p] * (x[pp] - x[qq]);
   }
   forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(2, 4, b, a);
   back_bf(1, 8, a, c);
}
/*------------------------------------------------------------*/
/*---------------- 8 pt fdct dual to mono---------------------*/
void fdct8_dual_mono(float x[], float c[])
{
   float a[8];			/* ping pong buffers */
   float b[8];
   float t1, t2;
   int p, pp, qq;

/* special first stage  */
   t1 = 0.5F * (x[0] + x[1]);
   t2 = 0.5F * (x[14] + x[15]);
   b[0] = t1 + t2;
   b[4] = coef32[16 + 8] * (t1 - t2);
   pp = 2;
   qq = 2 * 6;
   for (p = 1; p < 4; p++, pp += 2, qq -= 2)
   {
      t1 = 0.5F * (x[pp] + x[pp + 1]);
      t2 = 0.5F * (x[qq] + x[qq + 1]);
      b[p] = t1 + t2;
      b[4 + p] = coef32[16 + 8 + p] * (t1 - t2);
   }
   forward_bf(2, 4, b, a, coef32 + 16 + 8 + 4);
   forward_bf(4, 2, a, b, coef32 + 16 + 8 + 4 + 2);
   back_bf(2, 4, b, a);
   back_bf(1, 8, a, c);
}
/*------------------------------------------------------------*/
