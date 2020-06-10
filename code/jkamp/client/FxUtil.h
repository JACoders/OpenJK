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

#include "FxPrimitives.h"

bool	FX_Free( bool templates );	// ditches all active effects;
int		FX_Init( refdef_t* refdef );	// called in CG_Init to purge the fx system.
void	FX_SetRefDef(refdef_t *refdef);
void	FX_Add( bool portal );		// called every cgame frame to add all fx into the scene.
void	FX_Stop( void );	// ditches all active effects without touching the templates.


CParticle *FX_AddParticle( vec3_t org, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							float rotation, float rotationDelta,
							vec3_t min, vec3_t max, float elasticity,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CLine *FX_AddLine( vec3_t start, vec3_t end,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CElectricity *FX_AddElectricity( vec3_t start, vec3_t end, float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							float chaos, int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CTail *FX_AddTail( vec3_t org, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t min, vec3_t max, float elasticity,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CCylinder *FX_AddCylinder( vec3_t start, vec3_t normal,
							float size1s, float size1e, float size1Parm,
							float size2s, float size2e, float size2Parm,
							float length1, float length2, float lengthParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1,
							qboolean traceEnd = qfalse);

CEmitter *FX_AddEmitter( vec3_t org, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t angs, vec3_t deltaAngs,
							vec3_t min, vec3_t max, float elasticity,
							int deathID, int impactID, int emitterID,
							float density, float variance,
							int killTime, qhandle_t model, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CLight *FX_AddLight( vec3_t org, float size1, float size2, float sizeParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							int killTime, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

COrientedParticle *FX_AddOrientedParticle( vec3_t org, vec3_t norm, vec3_t vel, vec3_t accel,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							float rotation, float rotationDelta,
							vec3_t min, vec3_t max, float bounce,
							int deathID, int impactID,
							int killTime, qhandle_t shader, int flags,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1,
							CGhoul2Info_v *ghoul2 = NULL, int entNum=-1, int modelNum=-1, int boltNum=-1);

CPoly *FX_AddPoly( vec3_t *verts, vec2_t *st, int numVerts,
							vec3_t vel, vec3_t accel,
							float alpha1, float alpha2, float alphaParm,
							vec3_t rgb1, vec3_t rgb2, float rgbParm,
							vec3_t rotationDelta, float bounce, int motionDelay,
							int killTime, qhandle_t shader, int flags );

CFlash *FX_AddFlash( vec3_t origin,
							float size1, float size2, float sizeParm,
							float alpha1, float alpha2, float alphaParm,
							vec3_t sRGB, vec3_t eRGB, float rgbParm,
							int killTime, qhandle_t shader, int flags = 0,
							EMatImpactEffect matImpactFX = MATIMPACTFX_NONE, int fxParm = -1);

CBezier *FX_AddBezier( vec3_t start, vec3_t end,
								vec3_t control1, vec3_t control1Vel,
								vec3_t control2, vec3_t control2Vel,
								float size1, float size2, float sizeParm,
								float alpha1, float alpha2, float alphaParm,
								vec3_t sRGB, vec3_t eRGB, float rgbParm,
								int killTime, qhandle_t shader, int flags = 0 );
