/*
===========================================================================
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

// ICARUS Interface header file

typedef	float vec3_t[3];

class CSequencer;
class CTaskManager;

typedef struct interface_export_s
{
	//General
	int				(*I_LoadFile)( const char *name, void **buf );
	void			(*I_CenterPrint)( const char *format, ... );
	void			(*I_DPrintf)( int, const char *, ... );
	sharedEntity_t *(*I_GetEntityByName)( const char *name );		//Polls the engine for the sequencer of the entity matching the name passed
	unsigned int			(*I_GetTime)( void );							//Gets the current time
	unsigned int			(*I_GetTimeScale)(void );
	int 			(*I_PlaySound)( int taskID, int entID, const char *name, const char *channel );
	void			(*I_Lerp2Pos)( int taskID, int entID, vec3_t origin, vec3_t angles, float duration );
	void			(*I_Lerp2Origin)( int taskID, int entID, vec3_t origin, float duration );
	void			(*I_Lerp2Angles)( int taskID, int entID, vec3_t angles, float duration );
	int				(*I_GetTag)( int entID, const char *name, int lookup, vec3_t info );
	void			(*I_Lerp2Start)( int taskID, int entID, float duration );
	void			(*I_Lerp2End)( int taskID, int entID, float duration );
	void			(*I_Set)( int taskID, int entID, const char *type_name, const char *data );
	void			(*I_Use)( int entID, const char *name );
	void			(*I_Kill)( int entID, const char *name );
	void			(*I_Remove)( int entID, const char *name );
	float			(*I_Random)( float min, float max );
	void			(*I_Play)( int taskID, int entID, const char *type, const char *name );

	//Camera functions
	void			(*I_CameraPan)( vec3_t angles, vec3_t dir, float duration );
	void			(*I_CameraMove)( vec3_t origin, float duration );
	void			(*I_CameraZoom)( float fov, float duration );
	void			(*I_CameraRoll)( float angle, float duration );
	void			(*I_CameraFollow)( const char *name, float speed, float initLerp );
	void			(*I_CameraTrack)( const char *name, float speed, float initLerp );
	void			(*I_CameraDistance)( float dist, float initLerp );
	void			(*I_CameraFade)( float sr, float sg, float sb, float sa, float dr, float dg, float db, float da, float duration );
	void			(*I_CameraPath)( const char *name );
	void			(*I_CameraEnable)( void );
	void			(*I_CameraDisable)( void );
	void			(*I_CameraShake)( float intensity, int duration );

	int				(*I_GetFloat)( int entID, int type, const char *name, float *value );
	int				(*I_GetVector)( int entID, int type, const char *name, vec3_t value );
	int				(*I_GetString)( int entID, int type, const char *name, char **value );

	int				(*I_Evaluate)( int p1Type, const char *p1, int p2Type, const char *p2, int operatorType );

	void			(*I_DeclareVariable)( int type, const char *name );
	void			(*I_FreeVariable)( const char *name );

	//Save / Load functions

	int				(*I_WriteSaveData)( unsigned long chid, const void *data, int length );
	// Below changed by BTO (VV). Visual C++ 7.1 compiler no longer allows default args on function pointers. Ack.
	int				(*I_ReadSaveData)( unsigned long chid, void *address, int length /* , void **addressptr = NULL */ );
	int				(*I_LinkEntity)( int entID, CSequencer *sequencer, CTaskManager *taskManager );

} interface_export_t;
