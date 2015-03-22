/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// snddma_null.c
// all other sound mixing is portable

#include "qcommon/qcommon.h"

qboolean gbInsideLoadSound = qfalse; // important to default to this!!!

qboolean SNDDMA_Init(void)
{
	return qfalse;
}

int	SNDDMA_GetDMAPos(void)
{
	return 0;
}

void SNDDMA_Shutdown(void)
{
}

void SNDDMA_BeginPainting (void)
{
}

void SNDDMA_Submit(void)
{
}

sfxHandle_t S_RegisterSound( const char *name ) {
	return 0;
}

void S_StartLocalSound( sfxHandle_t sfxHandle, int channelNum ) {
}

void S_ClearSoundBuffer( void ) {
}

qboolean SND_RegisterAudio_LevelLoadEnd(qboolean bDeleteEverythingNotUsedThisLevel)
{
	return qfalse;
}

int SND_FreeOldestSound(void)
{
	return 0;
}
