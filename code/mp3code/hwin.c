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

	$Id: hwin.c,v 1.5 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  hwin.c  ***************************************************

Layer III

hybrid window/filter

******************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "mp3struct.h"
////@@@@extern int band_limit_nsb;

typedef float ARRAY36[36];

/*-- windows by block type --*/
static float win[4][36];	// effectively a constant


/*====================================================================*/
void imdct18(float f[]);	/* 18 point */
void imdct6_3(float f[]);	/* 6 point */

/*====================================================================*/
ARRAY36 *hwin_init_addr()
{
   return win;
}


/*====================================================================*/
int hybrid(float xin[], float xprev[], float y[18][32],
	   int btype, int nlong, int ntot, int nprev)
{
   int i, j;
   float *x, *x0;
   float xa, xb;
   int n;
   int nout;



   if (btype == 2)
      btype = 0;
   x = xin;
   x0 = xprev;

/*-- do long blocks (if any) --*/
   n = (nlong + 17) / 18;	/* number of dct's to do */
   for (i = 0; i < n; i++)
   {
      imdct18(x);
      for (j = 0; j < 9; j++)
      {
	 y[j][i] = x0[j] + win[btype][j] * x[9 + j];
	 y[9 + j][i] = x0[9 + j] + win[btype][9 + j] * x[17 - j];
      }
    /* window x for next time x0 */
      for (j = 0; j < 4; j++)
      {
	 xa = x[j];
	 xb = x[8 - j];
	 x[j] = win[btype][18 + j] * xb;
	 x[8 - j] = win[btype][(18 + 8) - j] * xa;
	 x[9 + j] = win[btype][(18 + 9) + j] * xa;
	 x[17 - j] = win[btype][(18 + 17) - j] * xb;
      }
      xa = x[j];
      x[j] = win[btype][18 + j] * xa;
      x[9 + j] = win[btype][(18 + 9) + j] * xa;

      x += 18;
      x0 += 18;
   }

/*-- do short blocks (if any) --*/
   n = (ntot + 17) / 18;	/* number of 6 pt dct's triples to do */
   for (; i < n; i++)
   {
      imdct6_3(x);
      for (j = 0; j < 3; j++)
      {
	 y[j][i] = x0[j];
	 y[3 + j][i] = x0[3 + j];

	 y[6 + j][i] = x0[6 + j] + win[2][j] * x[3 + j];
	 y[9 + j][i] = x0[9 + j] + win[2][3 + j] * x[5 - j];

	 y[12 + j][i] = x0[12 + j] + win[2][6 + j] * x[2 - j] + win[2][j] * x[(6 + 3) + j];
	 y[15 + j][i] = x0[15 + j] + win[2][9 + j] * x[j] + win[2][3 + j] * x[(6 + 5) - j];
      }
    /* window x for next time x0 */
      for (j = 0; j < 3; j++)
      {
	 x[j] = win[2][6 + j] * x[(6 + 2) - j] + win[2][j] * x[(12 + 3) + j];
	 x[3 + j] = win[2][9 + j] * x[6 + j] + win[2][3 + j] * x[(12 + 5) - j];
      }
      for (j = 0; j < 3; j++)
      {
	 x[6 + j] = win[2][6 + j] * x[(12 + 2) - j];
	 x[9 + j] = win[2][9 + j] * x[12 + j];
      }
      for (j = 0; j < 3; j++)
      {
	 x[12 + j] = 0.0f;
	 x[15 + j] = 0.0f;
      }
      x += 18;
      x0 += 18;
   }

/*--- overlap prev if prev longer that current --*/
   n = (nprev + 17) / 18;
   for (; i < n; i++)
   {
      for (j = 0; j < 18; j++)
	 y[j][i] = x0[j];
      x0 += 18;
   }
   nout = 18 * i;

/*--- clear remaining only to band limit --*/
   for (; i < pMP3Stream->band_limit_nsb; i++)
   {
      for (j = 0; j < 18; j++)
	 y[j][i] = 0.0f;
   }

   return nout;
}


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*-- convert to mono, add curr result to y,
    window and add next time to current left */
int hybrid_sum(float xin[], float xin_left[], float y[18][32],
	       int btype, int nlong, int ntot)
{
   int i, j;
   float *x, *x0;
   float xa, xb;
   int n;
   int nout;



   if (btype == 2)
      btype = 0;
   x = xin;
   x0 = xin_left;

/*-- do long blocks (if any) --*/
   n = (nlong + 17) / 18;	/* number of dct's to do */
   for (i = 0; i < n; i++)
   {
      imdct18(x);
      for (j = 0; j < 9; j++)
      {
	 y[j][i] += win[btype][j] * x[9 + j];
	 y[9 + j][i] += win[btype][9 + j] * x[17 - j];
      }
    /* window x for next time x0 */
      for (j = 0; j < 4; j++)
      {
	 xa = x[j];
	 xb = x[8 - j];
	 x0[j] += win[btype][18 + j] * xb;
	 x0[8 - j] += win[btype][(18 + 8) - j] * xa;
	 x0[9 + j] += win[btype][(18 + 9) + j] * xa;
	 x0[17 - j] += win[btype][(18 + 17) - j] * xb;
      }
      xa = x[j];
      x0[j] += win[btype][18 + j] * xa;
      x0[9 + j] += win[btype][(18 + 9) + j] * xa;

      x += 18;
      x0 += 18;
   }

/*-- do short blocks (if any) --*/
   n = (ntot + 17) / 18;	/* number of 6 pt dct's triples to do */
   for (; i < n; i++)
   {
      imdct6_3(x);
      for (j = 0; j < 3; j++)
      {
	 y[6 + j][i] += win[2][j] * x[3 + j];
	 y[9 + j][i] += win[2][3 + j] * x[5 - j];

	 y[12 + j][i] += win[2][6 + j] * x[2 - j] + win[2][j] * x[(6 + 3) + j];
	 y[15 + j][i] += win[2][9 + j] * x[j] + win[2][3 + j] * x[(6 + 5) - j];
      }
    /* window x for next time */
      for (j = 0; j < 3; j++)
      {
	 x0[j] += win[2][6 + j] * x[(6 + 2) - j] + win[2][j] * x[(12 + 3) + j];
	 x0[3 + j] += win[2][9 + j] * x[6 + j] + win[2][3 + j] * x[(12 + 5) - j];
      }
      for (j = 0; j < 3; j++)
      {
	 x0[6 + j] += win[2][6 + j] * x[(12 + 2) - j];
	 x0[9 + j] += win[2][9 + j] * x[12 + j];
      }
      x += 18;
      x0 += 18;
   }

   nout = 18 * i;

   return nout;
}
/*--------------------------------------------------------------------*/
void sum_f_bands(float a[], float b[], int n)
{
   int i;

   for (i = 0; i < n; i++)
      a[i] += b[i];
}
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
void FreqInvert(float y[18][32], int n)
{
   int i, j;

   n = (n + 17) / 18;
   for (j = 0; j < 18; j += 2)
   {
      for (i = 0; i < n; i += 2)
      {
	 y[1 + j][1 + i] = -y[1 + j][1 + i];
      }
   }
}
/*--------------------------------------------------------------------*/
