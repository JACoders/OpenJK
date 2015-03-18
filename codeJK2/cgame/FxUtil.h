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

#if !defined(FX_PRIMITIVES_H_INC)
	#include "FxPrimitives.h"
#endif

#ifndef FX_UTIL_H_INC
#define FX_UTIL_H_INC


bool	FX_Free( void );	// ditches all active effects;
int		FX_Init( void );	// called in CG_Init to purge the fx system.
void	FX_Add( void );		// called every cgame frame to add all fx into the scene.
void	FX_Stop( void );	// ditches all active effects without touching the templates.

bool	FX_ActiveFx(void);	// returns whether there are any active or scheduled effects


CParticle *FX_AddParticle( const vec3_t org, const vec3_t vel, const vec3_t accel, 
							float size1, float size2, float sizeParm, 
							float alpha1, float alpha2, float alphaParm, 
							const vec3_t rgb1, const vec3_t rgb2, float rgbParm,
							float rotation, float rotationDelta,
							const vec3_t min, const vec3_t max, float elasticity, 
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags );

CParticle *FX_AddParticle( int clientID, const vec3_t org, const vec3_t vel, const vec3_t accel, float gravity,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							const vec3_t rgb1, const vec3_t rgb2, float rgbParm,
							float rotation, float rotationDelta,
							int killTime, qhandle_t shader, int flags );

CLine *FX_AddLine( vec3_t start, vec3_t end, 
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, qhandle_t shader, int flags );

CLine *FX_AddLine( int clientID, vec3_t start, float size1, float size2, float sizeParm,
									float alpha1, float alpha2, float alphaParm,
									vec3_t sRGB, vec3_t eRGB, float rgbParm,
									int killTime, qhandle_t shader, int impactFX_id, int flags );

CElectricity *FX_AddElectricity( vec3_t start, vec3_t end, float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							float chaos, int killTime, qhandle_t shader, int flags );

CTail *FX_AddTail( vec3_t org, vec3_t vel, vec3_t accel, 
							float size1, float size2, float sizeParm, 
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t min, vec3_t max, float elasticity, 
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags );

CTail *FX_AddTail( int clientID, vec3_t org, vec3_t vel, vec3_t accel, 
							float size1, float size2, float sizeParm, 
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							vec3_t min, vec3_t max, float elasticity, 
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags );

CCylinder *FX_AddCylinder( vec3_t start, vec3_t normal, 
							float size1s, float size1e, float size1Parm,
							float size2s, float size2e, float size2Parm,
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, qhandle_t shader, int flags );

CEmitter *FX_AddEmitter( vec3_t org, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t angs, vec3_t deltaAngs,
							vec3_t min, vec3_t max, float elasticity, 
							int deathID, int impactID, int emitterID,
							float density, float variance,
							int killTime, qhandle_t model, int flags );

CLight *FX_AddLight( vec3_t org, float size1, float size2, float sizeParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, int flags );

COrientedParticle *FX_AddOrientedParticle( vec3_t org, vec3_t norm, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							float rotation, float rotationDelta,
							vec3_t min, vec3_t max, float bounce,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags );

CPoly *FX_AddPoly( vec3_t *verts, vec2_t *st, int numVerts,
							vec3_t vel, vec3_t accel,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t rotationDelta, float bounce, int motionDelay,
							int killTime, qhandle_t shader, int flags );

CFlash *FX_AddFlash( vec3_t origin, vec3_t sRGB, vec3_t eRGB, float rgbParm,
						int life, qhandle_t shader, int flags );


// Included for backwards compatibility with CHC and for doing quick programmatic effects.
void FX_AddSprite( vec3_t origin, vec3_t vel, vec3_t accel, 
							float scale, float dscale, 
							float sAlpha, float eAlpha, 
							float rotation, float bounce, 
							int life, qhandle_t shader, int flags = 0 );

void FX_AddSprite( vec3_t origin, vec3_t vel, vec3_t accel, 
							float scale, float dscale, 
							float sAlpha, float eAlpha, 
							vec3_t sRGB, vec3_t eRGB, 
							float rotation, float bounce, 
							int life, qhandle_t shader, int flags = 0 );

void FX_AddLine( vec3_t start, vec3_t end, float stScale, 
							float width, float dwidth, 
							float sAlpha, float eAlpha, 
							int life, qhandle_t shader, int flags = 0 );

void FX_AddLine( vec3_t start, vec3_t end, float stScale, 
							float width, float dwidth, 
							float sAlpha, float eAlpha, 
							vec3_t sRGB, vec3_t eRGB, 
							int life, qhandle_t shader, int flags = 0 );

void FX_AddQuad( vec3_t origin, vec3_t normal, 
						vec3_t vel, vec3_t accel, 
						float sradius, float eradius, 
						float salpha, float ealpha, 
						vec3_t sRGB, vec3_t eRGB, 
						float rotation, int life, qhandle_t shader, int flags = 0 );

CBezier *FX_AddBezier( const vec3_t start, const vec3_t end, 
						const vec3_t control1, const vec3_t control1Vel,
						const vec3_t control2, const vec3_t control2Vel,
						float size1, float size2, float sizeParm,
						float alpha1, float alpha2, float alphaParm,
						const vec3_t sRGB, const vec3_t eRGB, const float rgbParm,
						int killTime, qhandle_t shader, int flags = 0 );


#endif //FX_UTIL_H_INC