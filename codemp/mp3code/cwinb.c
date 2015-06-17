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

	$Id: cwinb.c,v 1.4 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  cwin.c  ***************************************************

include to cwinm.c

MPEG audio decoder, float window routines - 8 bit output
portable C

******************************************************************/
/*-------------------------------------------------------------------------*/

void windowB(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;
   int si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 16;
   bx = (si + 32) & 511;
   coef = wincoef;

/*-- first 16 --*/
   for (i = 0; i < 16; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si = (si + 64) & 511;
	 sum -= (*coef++) * vbuf[bx];
	 bx = (bx + 64) & 511;
      }
      si++;
      bx--;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx = (bx + 64) & 511;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
/*-- last 15 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 15; i++)
   {
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si = (si + 64) & 511;
	 sum += (*coef--) * vbuf[bx];
	 bx = (bx + 64) & 511;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
}
/*------------------------------------------------------------*/
void windowB_dual(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;			/* dual window interleaves output */
   int si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 16;
   bx = (si + 32) & 511;
   coef = wincoef;

/*-- first 16 --*/
   for (i = 0; i < 16; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si = (si + 64) & 511;
	 sum -= (*coef++) * vbuf[bx];
	 bx = (bx + 64) & 511;
      }
      si++;
      bx--;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx = (bx + 64) & 511;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
   pcm += 2;
/*-- last 15 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 15; i++)
   {
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si = (si + 64) & 511;
	 sum += (*coef--) * vbuf[bx];
	 bx = (bx + 64) & 511;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
}
/*------------------------------------------------------------*/
/*------------------- 16 pt window ------------------------------*/
void windowB16(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;
   unsigned char si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 8;
   bx = si + 16;
   coef = wincoef;

/*-- first 8 --*/
   for (i = 0; i < 8; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si += 32;
	 sum -= (*coef++) * vbuf[bx];
	 bx += 32;
      }
      si++;
      bx--;
      coef += 16;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx += 32;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
/*-- last 7 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 7; i++)
   {
      coef -= 16;
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si += 32;
	 sum += (*coef--) * vbuf[bx];
	 bx += 32;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
}
/*--------------- 16 pt dual window (interleaved output) -----------------*/
void windowB16_dual(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;
   unsigned char si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 8;
   bx = si + 16;
   coef = wincoef;

/*-- first 8 --*/
   for (i = 0; i < 8; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si += 32;
	 sum -= (*coef++) * vbuf[bx];
	 bx += 32;
      }
      si++;
      bx--;
      coef += 16;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx += 32;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
   pcm += 2;
/*-- last 7 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 7; i++)
   {
      coef -= 16;
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si += 32;
	 sum += (*coef--) * vbuf[bx];
	 bx += 32;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
}
/*------------------- 8 pt window ------------------------------*/
void windowB8(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;
   int si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 4;
   bx = (si + 8) & 127;
   coef = wincoef;

/*-- first 4 --*/
   for (i = 0; i < 4; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si = (si + 16) & 127;
	 sum -= (*coef++) * vbuf[bx];
	 bx = (bx + 16) & 127;
      }
      si++;
      bx--;
      coef += 48;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx = (bx + 16) & 127;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
/*-- last 3 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 3; i++)
   {
      coef -= 48;
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si = (si + 16) & 127;
	 sum += (*coef--) * vbuf[bx];
	 bx = (bx + 16) & 127;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm++ = ((unsigned char) (tmp >> 8)) ^ 0x80;
   }
}
/*--------------- 8 pt dual window (interleaved output) -----------------*/
void windowB8_dual(float *vbuf, int vb_ptr, unsigned char *pcm)
{
   int i, j;
   int si, bx;
   const float *coef;
   float sum;
   long tmp;

   si = vb_ptr + 4;
   bx = (si + 8) & 127;
   coef = wincoef;

/*-- first 4 --*/
   for (i = 0; i < 4; i++)
   {
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef++) * vbuf[si];
	 si = (si + 16) & 127;
	 sum -= (*coef++) * vbuf[bx];
	 bx = (bx + 16) & 127;
      }
      si++;
      bx--;
      coef += 48;
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
/*--  special case --*/
   sum = 0.0F;
   for (j = 0; j < 8; j++)
   {
      sum += (*coef++) * vbuf[bx];
      bx = (bx + 16) & 127;
   }
   tmp = (long) sum;
   if (tmp > 32767)
      tmp = 32767;
   else if (tmp < -32768)
      tmp = -32768;
   *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
   pcm += 2;
/*-- last 3 --*/
   coef = wincoef + 255;	/* back pass through coefs */
   for (i = 0; i < 3; i++)
   {
      coef -= 48;
      si--;
      bx++;
      sum = 0.0F;
      for (j = 0; j < 8; j++)
      {
	 sum += (*coef--) * vbuf[si];
	 si = (si + 16) & 127;
	 sum += (*coef--) * vbuf[bx];
	 bx = (bx + 16) & 127;
      }
      tmp = (long) sum;
      if (tmp > 32767)
	 tmp = 32767;
      else if (tmp < -32768)
	 tmp = -32768;
      *pcm = ((unsigned char) (tmp >> 8)) ^ 0x80;
      pcm += 2;
   }
}
/*------------------------------------------------------------*/
#endif	// #ifdef COMPILE_ME
