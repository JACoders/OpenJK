#if 0
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

	$Id: wavep.c,v 1.3 1999/10/19 07:13:09 elrod Exp $
____________________________________________________________________________*/

/*---- wavep.c --------------------------------------------

WAVE FILE HEADER ROUTINES
with conditional pcm conversion to MS wave format
portable version

-----------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "port.h"

typedef struct
{
   unsigned char riff[4];
   unsigned char size[4];
   unsigned char wave[4];
   unsigned char fmt[4];
   unsigned char fmtsize[4];
   unsigned char tag[2];
   unsigned char nChannels[2];
   unsigned char nSamplesPerSec[4];
   unsigned char nAvgBytesPerSec[4];
   unsigned char nBlockAlign[2];
   unsigned char nBitsPerSample[2];
   unsigned char data[4];
   unsigned char pcm_bytes[4];
}
BYTE_WAVE;

static const BYTE_WAVE wave =
{
   "RIFF",
   {(sizeof(BYTE_WAVE) - 8), 0, 0, 0},
   "WAVE",
   "fmt ",
   {16, 0, 0, 0},
   {1, 0},
   {1, 0},
   {34, 86, 0, 0},		/* 86 * 256 + 34 = 22050 */
   {172, 68, 0, 0},		/* 172 * 256 + 68 = 44100 */
   {2, 0},
   {16, 0},
   "data",
   {0, 0, 0, 0}
};

/*----------------------------------------------------------------
  pcm conversion to wave format

  This conversion code required for big endian machine, or,
  if sizeof(short) != 16 bits.
  Conversion routines may be used on any machine, but if
  not required, the do nothing macros in port.h can be used instead
  to reduce overhead.

-----------------------------------------------------------------*/
#ifndef LITTLE_SHORT16
#include "wcvt.c"
#endif
/*-----------------------------------------------*/
#endif
