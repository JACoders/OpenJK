#ifdef COMPILE_ME
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

	$Id: csbtb.c,v 1.2 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  csbtb.c  ***************************************************
include to csbt.c

MPEG audio decoder, dct and window - byte (8 pcm bit output)
portable C

******************************************************************/
/*============================================================*/
/*============================================================*/
void windowB(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB16_dual(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8(float *vbuf, int vb_ptr, unsigned char *pcm);
void windowB8_dual(float *vbuf, int vb_ptr, unsigned char *pcm);

/*============================================================*/
void sbtB_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct32(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
      pcm += 32;
   }

}
/*------------------------------------------------------------*/
void sbtB_dual(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct32_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      fdct32_dual(sample + 1, pMP3Stream->vbuf2 + pMP3Stream->vb_ptr);
      windowB_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      windowB_dual(pMP3Stream->vbuf2, pMP3Stream->vb_ptr, pcm + 1);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
      pcm += 64;
   }


}
/*------------------------------------------------------------*/
/* convert dual to mono */
void sbtB_dual_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct32_dual_mono(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
      pcm += 32;
   }

}
/*------------------------------------------------------------*/
/* convert dual to left */
void sbtB_dual_left(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct32_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
      pcm += 32;
   }
}
/*------------------------------------------------------------*/
/* convert dual to right */
void sbtB_dual_right(float *sample, unsigned char *pcm, int n)
{
   int i;

   sample++;			/* point to right chan */
   for (i = 0; i < n; i++)
   {
      fdct32_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
      pcm += 32;
   }
}
/*------------------------------------------------------------*/
/*---------------- 16 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB16_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct16(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
      pcm += 16;
   }


}
/*------------------------------------------------------------*/
void sbtB16_dual(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct16_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      fdct16_dual(sample + 1, pMP3Stream->vbuf2 + pMP3Stream->vb_ptr);
      windowB16_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      windowB16_dual(pMP3Stream->vbuf2, pMP3Stream->vb_ptr, pcm + 1);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
      pcm += 32;
   }
}
/*------------------------------------------------------------*/
void sbtB16_dual_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct16_dual_mono(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
      pcm += 16;
   }
}
/*------------------------------------------------------------*/
void sbtB16_dual_left(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct16_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
      pcm += 16;
   }
}
/*------------------------------------------------------------*/
void sbtB16_dual_right(float *sample, unsigned char *pcm, int n)
{
   int i;

   sample++;
   for (i = 0; i < n; i++)
   {
      fdct16_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
      pcm += 16;
   }
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB8_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct8(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
      pcm += 8;
   }

}
/*------------------------------------------------------------*/
void sbtB8_dual(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct8_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      fdct8_dual(sample + 1, pMP3Stream->vbuf2 + pMP3Stream->vb_ptr);
      windowB8_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      windowB8_dual(pMP3Stream->vbuf2, pMP3Stream->vb_ptr, pcm + 1);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
      pcm += 16;
   }
}
/*------------------------------------------------------------*/
void sbtB8_dual_mono(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct8_dual_mono(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
      pcm += 8;
   }
}
/*------------------------------------------------------------*/
void sbtB8_dual_left(float *sample, unsigned char *pcm, int n)
{
   int i;

   for (i = 0; i < n; i++)
   {
      fdct8_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
      pcm += 8;
   }
}
/*------------------------------------------------------------*/
void sbtB8_dual_right(float *sample, unsigned char *pcm, int n)
{
   int i;

   sample++;
   for (i = 0; i < n; i++)
   {
      fdct8_dual(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
      windowB8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
      sample += 64;
      pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
      pcm += 8;
   }
}
/*------------------------------------------------------------*/
#endif	// #ifdef COMPILE_ME
