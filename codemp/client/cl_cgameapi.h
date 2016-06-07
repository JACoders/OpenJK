/*
===========================================================================
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

void			CGVM_Init					( int serverMessageNum, int serverCommandSequence, int clientNum );
void			CGVM_Shutdown				( void );
qboolean		CGVM_ConsoleCommand			( void );
void			CGVM_DrawActiveFrame		( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
int				CGVM_CrosshairPlayer		( void );
int				CGVM_LastAttacker			( void );
void			CGVM_KeyEvent				( int key, qboolean down );
void			CGVM_MouseEvent				( int x, int y );
void			CGVM_EventHandling			( int type );
int				CGVM_PointContents			( void );
void			CGVM_GetLerpOrigin			( void );
void			CGVM_GetLerpData			( void );
void			CGVM_Trace					( void );
void			CGVM_G2Trace				( void );
void			CGVM_G2Mark					( void );
int				CGVM_RagCallback			( int callType );
qboolean		CGVM_IncomingConsoleCommand	( void );
qboolean		CGVM_NoUseableForce			( void );
void			CGVM_GetOrigin				( int entID, vec3_t out );
void			CGVM_GetAngles				( int entID, vec3_t out );
trajectory_t *	CGVM_GetOriginTrajectory	( int entID );
trajectory_t *	CGVM_GetAngleTrajectory		( int entID );
void			CGVM_ROFF_NotetrackCallback	( int entID, const char *notetrack );
void			CGVM_MapChange				( void );
void			CGVM_AutomapInput			( void );
void			CGVM_MiscEnt				( void );
void			CGVM_CameraShake			( void );

void CL_BindCGame( void );
void CL_UnbindCGame( void );
