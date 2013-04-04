// Disruptor Weapon

#include "cg_local.h"
#include "fx_local.h"

/*
---------------------------
FX_DisruptorMainShot
---------------------------
*/
static vec3_t WHITE={1.0f,1.0f,1.0f};

void FX_DisruptorMainShot( vec3_t start, vec3_t end )
{
//	vec3_t	dir;
//	float	len;

	trap_FX_AddLine( start, end, 0.1f, 6.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							150, trap_R_RegisterShader( "gfx/effects/redLine" ), 
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

//	VectorSubtract( end, start, dir );
//	len = VectorNormalize( dir );

//	FX_AddCylinder( start, dir, 5.0f, 5.0f, 0.0f,
//								5.0f, 5.0f, 0.0f,
//								len, len, 0.0f,
//								1.0f, 1.0f, 0.0f,
//								WHITE, WHITE, 0.0f,
//								400, cgi_R_RegisterShader( "gfx/effects/spiral" ), 0 );
}


/*
---------------------------
FX_DisruptorAltShot
---------------------------
*/
void FX_DisruptorAltShot( vec3_t start, vec3_t end, qboolean fullCharge )
{
	trap_FX_AddLine( start, end, 0.1f, 10.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap_R_RegisterShader( "gfx/effects/redLine" ), 
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	if ( fullCharge )
	{
		vec3_t	YELLER={0.8f,0.7f,0.0f};

		// add some beef
		trap_FX_AddLine( start, end, 0.1f, 7.0f, 0.0f, 
							1.0f, 0.0f, 0.0f,
							YELLER, YELLER, 0.0f,
							150, trap_R_RegisterShader( "gfx/misc/whiteline2" ), 
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
}


/*
---------------------------
FX_DisruptorAltMiss
---------------------------
*/
#define FX_ALPHA_WAVE		0x00000008

void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal )
{
	vec3_t pos, c1, c2;
	addbezierArgStruct_t b;

	VectorMA( origin, 4.0f, normal, c1 );
	VectorCopy( c1, c2 );
	c1[2] += 4;
	c2[2] += 12;
	
	VectorAdd( origin, normal, pos );
	pos[2] += 28;

	/*
	FX_AddBezier( origin, pos, c1, vec3_origin, c2, vec3_origin, 6.0f, 6.0f, 0.0f, 0.0f, 0.2f, 0.5f,
	WHITE, WHITE, 0.0f, 4000, trap_R_RegisterShader( "gfx/effects/smokeTrail" ), FX_ALPHA_WAVE );
	*/

	VectorCopy(origin, b.start);
	VectorCopy(pos, b.end);
	VectorCopy(c1, b.control1);
	VectorCopy(vec3_origin, b.control1Vel);
	VectorCopy(c2, b.control2);
	VectorCopy(vec3_origin, b.control2Vel);

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;
	
	VectorCopy(WHITE, b.sRGB);
	VectorCopy(WHITE, b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 4000;
	b.shader = trap_R_RegisterShader( "gfx/effects/smokeTrail" );
	b.flags = FX_ALPHA_WAVE;

	trap_FX_AddBezier(&b);

	trap_FX_PlayEffectID( cgs.effects.disruptorAltMissEffect, origin, normal, -1, -1 );
}

/*
---------------------------
FX_DisruptorAltHit
---------------------------
*/

void FX_DisruptorAltHit( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.disruptorAltHitEffect, origin, normal, -1, -1 );
}



/*
---------------------------
FX_DisruptorHitWall
---------------------------
*/

void FX_DisruptorHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.disruptorWallImpactEffect, origin, normal, -1, -1 );
}

/*
---------------------------
FX_DisruptorHitPlayer
---------------------------
*/

void FX_DisruptorHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.disruptorFleshImpactEffect, origin, normal, -1, -1 );
}
