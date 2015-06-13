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

	$Id: cwinm.c,v 1.3 1999/10/19 07:13:08 elrod Exp $
____________________________________________________________________________*/

/****  cwinm.c  ***************************************************

MPEG audio decoder, window master routine
portable C


******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

const float wincoef[264] =
{				/* window coefs */
#include "tableawd.h"
};

/*--------------------------------------------------------*/
#define COMPILE_ME
#include "cwin.c"
#include "cwinb.c"
/*--------------------------------------------------------*/
