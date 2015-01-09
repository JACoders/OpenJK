/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// Disruptor Weapon

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"


/*
---------------------------
FX_DisruptorMainShot
---------------------------
*/
static vec3_t WHITE	={1.0f,1.0f,1.0f};

void FX_DisruptorMainShot( vec3_t start, vec3_t end )
{
	FX_AddLine( -1, start, end, 0.1f, 4.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							120, cgi_R_RegisterShader( "gfx/effects/redLine" ), 
							0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}


/*
---------------------------
FX_DisruptorAltShot
---------------------------
*/
void FX_DisruptorAltShot( vec3_t start, vec3_t end, qboolean fullCharge )
{
	FX_AddLine( -1, start, end, 0.1f, 10.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, cgi_R_RegisterShader( "gfx/effects/redLine" ), 
							0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	if ( fullCharge )
	{
		vec3_t	YELLER={0.8f,0.7f,0.0f};

		// add some beef
		FX_AddLine( -1, start, end, 0.1f, 7.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							YELLER, YELLER, 0.0f,
							150, cgi_R_RegisterShader( "gfx/misc/whiteline2" ), 
							0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
}

/*
---------------------------
FX_DisruptorAltMiss
---------------------------
*/

void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal )
{
	vec3_t pos, c1, c2;

	VectorMA( origin, 4.0f, normal, c1 );
	VectorCopy( c1, c2 );
	c1[2] += 4;
	c2[2] += 12;
	
	VectorAdd( origin, normal, pos );
	pos[2] += 28;

	FX_AddBezier( origin, pos, c1, vec3_origin, c2, vec3_origin, 6.0f, 6.0f, 0.0f, 0.0f, 0.2f, 0.5f, WHITE, WHITE, 0.0f, 4000, cgi_R_RegisterShader( "gfx/effects/smokeTrail" ), FX_ALPHA_WAVE );

	theFxScheduler.PlayEffect( "disruptor/alt_miss", origin, normal );
}

/*
---------------------------
FX_KothosBeam
---------------------------
*/
void FX_KothosBeam( vec3_t start, vec3_t end )
{
	FX_AddLine( -1, start, end, 0.1f, 10.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, cgi_R_RegisterShader( "gfx/misc/dr1" ), 
							0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	vec3_t	YELLER={0.8f,0.7f,0.0f};

	// add some beef
	FX_AddLine( -1, start, end, 0.1f, 7.0f, 0.0f, 
						1.0f, 0.0f, 0.0f,
						YELLER, YELLER, 0.0f,
						150, cgi_R_RegisterShader( "gfx/misc/whiteline2" ), 
						0, FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}
