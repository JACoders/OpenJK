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

	$Id: csbtL3.c,v 1.2 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  csbtL3.c  ***************************************************

layer III

  include to  csbt.c

******************************************************************/
/*============================================================*/
/*============ Layer III =====================================*/
/*============================================================*/
void sbt_mono_L3(float *sample, short *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct32(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		window(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
		pcm += 32;
	}
}
/*------------------------------------------------------------*/
void sbt_dual_L3(float *sample, short *pcm, int ch)
{
	int i;

	if (ch == 0)
	{
		for (i = 0; i < 18; i++)
		{
			fdct32(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
			window_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
			sample += 32;
			pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
			pcm += 64;
		}
	}
	else
	{
		for (i = 0; i < 18; i++)
		{
			fdct32(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
			window_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
			sample += 32;
			pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 32) & 511;
			pcm += 64;
		}
	}
}
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*---------------- 16 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt16_mono_L3(float *sample, short *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct16(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		window16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
		pcm += 16;
	}
}
/*------------------------------------------------------------*/
void sbt16_dual_L3(float *sample, short *pcm, int ch)
{
   int i;

   if (ch == 0)
   {
	   for (i = 0; i < 18; i++)
	   {
		   fdct16(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		   window16_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		   sample += 32;
		   pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
		   pcm += 32;
	   }
   }
   else
   {
	   for (i = 0; i < 18; i++)
	   {
		   fdct16(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
		   window16_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
		   sample += 32;
		   pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 16) & 255;
		   pcm += 32;
	   }
   }
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbt's  -------------------------------*/
/*------------------------------------------------------------*/
void sbt8_mono_L3(float *sample, short *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct8(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		window8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
		pcm += 8;
	}
}
/*------------------------------------------------------------*/
void sbt8_dual_L3(float *sample, short *pcm, int ch)
{
	int i;

	if (ch == 0)
	{
		for (i = 0; i < 18; i++)
		{
			fdct8(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
			window8_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
			sample += 32;
			pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
			pcm += 16;
		}
	}
	else
	{
		for (i = 0; i < 18; i++)
		{
			fdct8(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
			window8_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
			sample += 32;
			pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 8) & 127;
			pcm += 16;
		}
	}
}
/*------------------------------------------------------------*/
/*------- 8 bit output ---------------------------------------*/
/*------------------------------------------------------------*/
void sbtB_mono_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct32(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		windowB(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
		pcm += 32;
	}
}
/*------------------------------------------------------------*/
void sbtB_dual_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	if (ch == 0)
	{
		for (i = 0; i < 18; i++)
		{
			fdct32(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
			windowB_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
			sample += 32;
			pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 32) & 511;
			pcm += 64;
		}
	}
	else
	{
		for (i = 0; i < 18; i++)
		{
			fdct32(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
			windowB_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
			sample += 32;
			pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 32) & 511;
			pcm += 64;
		}
	}
}
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*---------------- 16 pt sbtB's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB16_mono_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct16(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		windowB16(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
		pcm += 16;
	}
}
/*------------------------------------------------------------*/
void sbtB16_dual_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	if (ch == 0)
	{
		for (i = 0; i < 18; i++)
		{
			fdct16(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
			windowB16_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
			sample += 32;
			pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 16) & 255;
			pcm += 32;
		}
	}
	else
	{
		for (i = 0; i < 18; i++)
		{
			fdct16(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
			windowB16_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
			sample += 32;
			pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 16) & 255;
			pcm += 32;
		}
	}
}
/*------------------------------------------------------------*/
/*---------------- 8 pt sbtB's  -------------------------------*/
/*------------------------------------------------------------*/
void sbtB8_mono_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	ch = 0;
	for (i = 0; i < 18; i++)
	{
		fdct8(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
		windowB8(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
		sample += 32;
		pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
		pcm += 8;
	}
}
/*------------------------------------------------------------*/
void sbtB8_dual_L3(float *sample, unsigned char *pcm, int ch)
{
	int i;

	if (ch == 0)
	{
		for (i = 0; i < 18; i++)
		{
			fdct8(sample, pMP3Stream->vbuf + pMP3Stream->vb_ptr);
			windowB8_dual(pMP3Stream->vbuf, pMP3Stream->vb_ptr, pcm);
			sample += 32;
			pMP3Stream->vb_ptr = (pMP3Stream->vb_ptr - 8) & 127;
			pcm += 16;
		}
	}
	else
	{
		for (i = 0; i < 18; i++)
		{
			fdct8(sample, pMP3Stream->vbuf2 + pMP3Stream->vb2_ptr);
			windowB8_dual(pMP3Stream->vbuf2, pMP3Stream->vb2_ptr, pcm + 1);
			sample += 32;
			pMP3Stream->vb2_ptr = (pMP3Stream->vb2_ptr - 8) & 127;
			pcm += 16;
		}
	}
}
/*------------------------------------------------------------*/
#endif	// #ifdef COMPILE_ME
