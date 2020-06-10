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

#pragma once

#include "qcommon/qcommon.h"

void S_Init( void );
void S_Shutdown( void );

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_AddAmbientLoopingSound( const vec3_t origin, unsigned char volume, sfxHandle_t sfxHandle );
void S_StartAmbientSound( const vec3_t origin, int entityNum, unsigned char volume, sfxHandle_t sfxHandle );
void S_MuteSound(int entityNum, int entchannel);
void S_StartSound( const vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
void S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void S_StartLocalLoopingSound( sfxHandle_t sfx);

void S_UnCacheDynamicMusic( void );
void S_RestartMusic( void );
void S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bCalledByCGameStart );
void S_StopBackgroundTrack( void );
float S_GetSampleLengthInMilliSeconds( sfxHandle_t sfxHandle);

// cinematics and voice-over-network will send raw samples
// 1.0 volume will be direct output of source samples
void S_RawSamples( int samples, int rate, int width, int channels, const byte *data, float volume, int bFirstOrOnlyUpdateThisFrame );
// stop all sounds
void S_StopSounds(void);	// from snd_dma.cpp
// stop all sounds and the background track
void S_StopAllSounds( void );

// scan all MP3s in the sound dir and add maxvol info if necessary.
void S_MP3_CalcVols_f( void );

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds( void );
void S_StopLoopingSound( int entityNum );
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );

// recompute the reletive volumes for all running sounds
// relative to the given entityNum / orientation
void S_Respatialize( int entityNum, const vec3_t head, matrix3_t axis, int inwater );

// let the sound system know where an entity currently is
void S_UpdateEntityPosition( int entityNum, const vec3_t origin );

void S_Update( void );

void S_DisableSounds( void );

void S_BeginRegistration( void );

// RegisterSound will allways return a valid sample, even if it
// has to create a placeholder.  This prevents continuous filesystem
// checks for missing files
sfxHandle_t	S_RegisterSound( const char *sample );

extern qboolean s_shutUp;

void S_FreeAllSFXMem(void);
