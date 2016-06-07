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

// cg_ents.c -- present snapshot entities, happens every single frame
#include "../game/g_local.h"
#include "cg_local.h"
#include "cg_media.h"
#include "../game/g_functions.h"
#include "../../code/ghoul2/G2.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"

extern void CG_AddSaberBlade( centity_t *cent, centity_t *scent, refEntity_t *saber, int renderfx, int modelIndex, vec3_t origin, vec3_t angles);
extern void CG_CheckSaberInWater( centity_t *cent, centity_t *scent, int modelIndex, vec3_t origin, vec3_t angles );
extern void CG_ForcePushBlur( const vec3_t org );

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	cgi_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) 
	{
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}

/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName, orientation_t *tagOrient ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

	// lerp the tag
	cgi_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0f - parent->backlerp, tagName );

	if ( tagOrient )
	{
		VectorCopy( lerped.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			VectorCopy( lerped.axis[i], tagOrient->axis[i] );
		}
	}

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
vec3_t *CG_SetEntitySoundPosition( centity_t *cent ) {
	static vec3_t v3Return;
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		cgi_S_UpdateEntityPosition( cent->currentState.number, origin );
		VectorCopy(origin, v3Return);
	} else {
		cgi_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
		VectorCopy(cent->lerpOrigin, v3Return);
	}

	return &v3Return;
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	vec3_t v3Origin;
	VectorCopy(*CG_SetEntitySoundPosition( cent ),v3Origin);

	// add loop sound
	if ( cent->currentState.loopSound ) 
	{
		
		sfxHandle_t	sfx = ( cent->currentState.eType == ET_MOVER ) ? cent->currentState.loopSound : cgs.sound_precache[ cent->currentState.loopSound ];

		cgi_S_AddLoopingSound( cent->currentState.number, v3Origin/*cent->lerpOrigin*/, vec3_origin, sfx );
	}

	// constant light glow
	if ( cent->currentState.constantLight ) {
		int		cl;
		float	i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float) (cl & 0xFF) / 255.0;
		g = (float) ((cl >> 8) & 0xFF) / 255.0;
		b = (float) ((cl >> 16) & 0xFF) / 255.0;
		i = (float) ((cl >> 24) & 0xFF) * 4.0;
		cgi_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
	}
}

void CG_AddRefEntWithTransportEffect ( centity_t *cent, refEntity_t *ent )
{
	// We are a normal thing....
	cgi_R_AddRefEntityToScene (ent);

	if ( ent->renderfx & RF_PULSATE && cent->gent->owner && cent->gent->owner->health &&
		!cent->gent->owner->s.number && cent->gent->owner->client && //only for player
			cent->gent->owner->client->ps.saberEntityState == SES_RETURNING && 
			cent->currentState.saberActive == qfalse )
	{
		// if we are the saber and we have been dropped, do a glow so it can be spotted easier
		float	wv;
		vec3_t	org;

		ent->customShader = cgi_R_RegisterShader( "gfx/effects/solidWhite_cull" );
		ent->renderfx = RF_RGB_TINT;
		wv = sin( cg.time * 0.003f ) * 0.08f + 0.1f;
		ent->shaderRGBA[0] = wv * 255;
		ent->shaderRGBA[1] = wv * 255;
		ent->shaderRGBA[2] = wv * 0;
		cgi_R_AddRefEntityToScene (ent);

		for ( int i = -4; i < 10; i += 1 )
		{
			VectorMA( ent->origin, -i, ent->axis[2], org );

			FX_AddSprite( org, NULL, NULL, 5.5f, 5.5f, wv, wv, 0.0f, 0.0f, 1.0f, cgs.media.yellowDroppedSaberShader, 0x08000000 );
		}
		if ( cent->gent->owner->s.weapon == WP_SABER )
		{//he's still controlling me
			FX_AddSprite( cent->gent->owner->client->renderInfo.handRPoint, NULL, NULL, 8.0f, 8.0f, wv, wv, 0.0f, 0.0f, 1.0f, cgs.media.yellowDroppedSaberShader, 0x08000000 );
		}
	}
}


/*
Ghoul2 Insert Start
*/

// Copy the ghoul2 data into the ref ent correctly
void CG_SetGhoul2Info( refEntity_t *ent, centity_t *cent)
{
	ent->ghoul2 = &cent->gent->ghoul2;
	VectorCopy( cent->currentState.modelScale, ent->modelScale);
	ent->radius = cent->currentState.radius;
	VectorCopy (cent->lerpAngles, ent->angles);
}



// create 8 new points on screen around a model so we can see it's bounding box
void CG_CreateBBRefEnts(entityState_t *s1, vec3_t origin )
{
#if 0	//loadsavecrash	_DEBUG
	refEntity_t		point[8];
	int				i;
	vec3_t			angles = {0,0,0};

	for (i=0; i<8; i++)
	{
		memset (&point[i], 0, sizeof(refEntity_t));
		point[i].reType = RT_SPRITE;
		point[i].radius = 1;
		point[i].customShader = cgi_R_RegisterShader("textures/tests/circle");
		point[i].shaderRGBA[0] = 255;
		point[i].shaderRGBA[1] = 255;
		point[i].shaderRGBA[2] = 255;
		point[i].shaderRGBA[3] = 255;

		AnglesToAxis( angles, point[i].axis );

		// now, we need to put the correct origins into each origin from the mins and max's
		switch(i)
		{
		case 0:
			VectorCopy(s1->mins, point[i].origin);
   			break;
		case 1:
			VectorCopy(s1->mins, point[i].origin);
			point[i].origin[0] = s1->maxs[0];
   			break;
		case 2:
			VectorCopy(s1->mins, point[i].origin);
			point[i].origin[1] = s1->maxs[1];
   			break;
		case 3:
			VectorCopy(s1->mins, point[i].origin);
			point[i].origin[0] = s1->maxs[0];
			point[i].origin[1] = s1->maxs[1];
   			break;
		case 4:
			VectorCopy(s1->maxs, point[i].origin);
   			break;
		case 5:
			VectorCopy(s1->maxs, point[i].origin);
			point[i].origin[0] = s1->mins[0];
   			break;
		case 6:
			VectorCopy(s1->maxs, point[i].origin);
			point[i].origin[1] = s1->mins[1];
   			break;
		case 7:
			VectorCopy(s1->maxs, point[i].origin);
			point[i].origin[0] = s1->mins[0];
			point[i].origin[1] = s1->mins[1];
   			break;
		}

		// add the original origin to each point and then stuff them out there
		VectorAdd(point[i].origin, origin, point[i].origin);

		cgi_R_AddRefEntityToScene (&point[i]);
	}
#endif
}

// write in the axis and stuff
void G2_BoltToGhoul2Model(centity_t *cent, refEntity_t *ent)
{
		// extract the wraith ID from the bolt info
	int modelNum = cent->currentState.boltInfo >> MODEL_SHIFT;
	modelNum &= MODEL_AND;
	int boltNum	= cent->currentState.boltInfo >> BOLT_SHIFT;
	boltNum &= BOLT_AND;
	int	entNum = cent->currentState.boltInfo >> ENTITY_SHIFT;
	entNum &= ENTITY_AND;

 	mdxaBone_t 		boltMatrix;

 	// go away and get me the bolt position for this frame please
 	gi.G2API_GetBoltMatrix(cent->gent->ghoul2, modelNum, boltNum, &boltMatrix, cg_entities[entNum].currentState.angles, cg_entities[entNum].currentState.origin, cg.time, cgs.model_draw, cent->currentState.modelScale);

	// set up the axis and origin we need for the actual effect spawning
 	ent->origin[0] = boltMatrix.matrix[0][3];
 	ent->origin[1] = boltMatrix.matrix[1][3];
 	ent->origin[2] = boltMatrix.matrix[2][3];

 	ent->axis[0][0] = boltMatrix.matrix[0][0];
 	ent->axis[0][1] = boltMatrix.matrix[1][0];
 	ent->axis[0][2] = boltMatrix.matrix[2][0];

 	ent->axis[1][0] = boltMatrix.matrix[0][1];
 	ent->axis[1][1] = boltMatrix.matrix[1][1];
 	ent->axis[1][2] = boltMatrix.matrix[2][1];

 	ent->axis[2][0] = boltMatrix.matrix[0][2];
 	ent->axis[2][1] = boltMatrix.matrix[1][2];
 	ent->axis[2][2] = boltMatrix.matrix[2][2];
}

void ScaleModelAxis(refEntity_t	*ent)

{		// scale the model should we need to
		if (ent->modelScale[0] && ent->modelScale[0] != 1.0f)
		{
			VectorScale( ent->axis[0], ent->modelScale[0] , ent->axis[0] );
			ent->nonNormalizedAxes = qtrue;
		}
		if (ent->modelScale[1] && ent->modelScale[1] != 1.0f)
		{
			VectorScale( ent->axis[1], ent->modelScale[1] , ent->axis[1] );
			ent->nonNormalizedAxes = qtrue;
		}
		if (ent->modelScale[2] && ent->modelScale[2] != 1.0f)
		{
			VectorScale( ent->axis[2], ent->modelScale[2] , ent->axis[2] );
			ent->nonNormalizedAxes = qtrue;
		}
}
/*
Ghoul2 Insert End
*/

/*
==================
CG_General
==================
*/
extern int CG_SaberHumSoundForEnt( gentity_t *gent );
static void CG_General( centity_t *cent ) 
{
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;
/*
Ghoul2 Insert Start
*/

	// if set to invisible, skip
	if (!s1->modelindex && !cent->gent->ghoul2.IsValid() ) {
		return;
	}
/*
Ghoul2 Insert End
*/

	if ( s1->eFlags & EF_NODRAW )
	{
		// If you don't like it doing NODRAW, then don't set the flag
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	if ( s1->eFlags & EF_SHADER_ANIM )
	{
		// Deliberately setting it up so that shader anim will completely override any kind of model animation frame setting.
		ent.renderfx|=RF_SETANIMINDEX;
		ent.skinNum = s1->frame;
	}
	else if ( s1->eFlags & EF_ANIM_ONCE )
	{
		//s1->frame++;
		//ent.frame = s1->frame;
		ent.frame = cent->gent->s.frame;
		ent.renderfx|=RF_CAP_FRAMES;
	}
	else if ( s1->eFlags & EF_ANIM_ALLFAST )
	{
		ent.frame = (cg.time / 100);
		ent.renderfx|=RF_WRAP_FRAMES;
	}
	else
	{
		ent.frame = s1->frame;
	}
	ent.oldframe = ent.frame;
	ent.backlerp = 0;
/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);
/*
Ghoul2 Insert End
*/

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.model_draw[s1->modelindex];

	if ( s1->eFlags & EF_AUTO_SIZE && cent->gent )
	{
		cgi_R_ModelBounds( ent.hModel, cent->gent->mins, cent->gent->maxs );
		//Only do this once
		cent->gent->s.eFlags &= ~EF_AUTO_SIZE;
	}

	// player model
	if (s1->number == cg.snap->ps.clientNum) 
	{
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}
/*
Ghoul2 Insert Start
*/
	// are we bolted to a Ghoul2 model?
	if (s1->boltInfo)
	{
		G2_BoltToGhoul2Model(cent, &ent);
	}
	else
	{
		//-------------------------------------------------------
		// Start of chair
		//-------------------------------------------------------
		if ( cent->gent->s.weapon == WP_EMPLACED_GUN || ( cent->gent->activator && cent->gent->activator->owner && 
				cent->gent->activator->s.eFlags & EF_LOCKED_TO_WEAPON ))
		{
			vec3_t	temp;

			if ( cent->gent->health <= 0 && cent->gent->e_ThinkFunc == thinkF_NULL )
			{
				ent.customShader = cgi_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_dmg" );

				VectorSet( temp, 0, 0, 1 );

				// add a big scorch mark under the gun
				CG_ImpactMark( cgs.media.scavMarkShader, cent->lerpOrigin, temp, 
					0, 1,1,1, 1.0f, qfalse, 92, qtrue );
				CG_ImpactMark( cgs.media.scavMarkShader, cent->lerpOrigin, temp, 
					90, 1,1,1, 1.0f, qfalse, 48, qtrue );
			}
			else
			{
				VectorSet( temp, 0, 0, 1 );

				if ( !( cent->gent->svFlags & SVF_INACTIVE ))
				{
					ent.customShader = cgi_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_on" );
				}

				// shadow under the gun
				CG_ImpactMark( cgs.media.shadowMarkShader, cent->lerpOrigin, temp, 
					0, 1,1,1, 1.0f, qfalse, 32, qtrue );
			}
		}

		if ( cent->gent->activator && cent->gent->activator->owner && 
				cent->gent->activator->s.eFlags & EF_LOCKED_TO_WEAPON && 
				cent->gent->activator->owner->s.number == cent->currentState.number ) // gun number must be same as current entities number
		{
			centity_t *cc = &cg_entities[cent->gent->activator->s.number];

const weaponData_t  *wData = NULL;

			if ( cc->currentState.weapon )
			{
				wData = &weaponData[cc->currentState.weapon];
			}

			if ( !( cc->currentState.eFlags & EF_FIRING ) && !( cc->currentState.eFlags & EF_ALT_FIRING ))
			{
				// not animating..pausing was leaving the barrels in a bad state
				gi.G2API_PauseBoneAnim( &cent->gent->ghoul2[cent->gent->playerModel], "model_root", cg.time );

//				gi.G2API_SetBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->rootBone, 
//						0, 0, BONE_ANIM_OVERRIDE, 1.0f, cg.time );
			}

			// get alternating muzzle end bolts
			int			bolt = cent->gent->handRBolt;
			mdxaBone_t	boltMatrix;

			if ( !cc->gent->fxID )
			{
				bolt = cent->gent->handLBolt;
			}

			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, 0, bolt, 
					&boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
					cgs.model_draw, cent->currentState.modelScale );

			// store the muzzle point and direction so that we can fire in the right direction
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cc->gent->client->renderInfo.muzzlePoint );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, cc->gent->client->renderInfo.muzzleDir );
			cc->gent->client->renderInfo.mPCalcTime = cg.time;

			// HACK: adding in muzzle flashes
			if ( cc->muzzleFlashTime > 0 && wData )
			{
				const char *effect = NULL;
				cc->muzzleFlashTime = 0;

				// Try and get a default muzzle so we have one to fall back on
				if ( wData->mMuzzleEffect[0] )
				{
					effect = &wData->mMuzzleEffect[0];
				}

				if ( cc->currentState.eFlags & EF_ALT_FIRING )
				{
					// We're alt-firing, so see if we need to override with a custom alt-fire effect
					if ( wData->mAltMuzzleEffect[0] )
					{
						effect = &wData->mAltMuzzleEffect[0];
					}
				}

				if ( cc->currentState.eFlags & EF_FIRING || cc->currentState.eFlags & EF_ALT_FIRING )
				{
					gi.G2API_SetBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->rootBone, 
						0, 3, BONE_ANIM_OVERRIDE_FREEZE, 0.6f, cg.time, -1, -1 );
		
					if ( effect )
					{
						// We got an effect and we're firing, so let 'er rip.
						theFxScheduler.PlayEffect( effect, cc->gent->client->renderInfo.muzzlePoint, 
													cc->gent->client->renderInfo.muzzleDir );
					}
				}
			}

			VectorCopy( cent->gent->s.apos.trBase, cent->lerpAngles );
		}
		//-------------------------------------------------------
		// End of chair
		//-------------------------------------------------------

		AnglesToAxis( cent->lerpAngles, ent.axis );
	}

	ScaleModelAxis(&ent);

	if (cent->gent->ghoul2.size())
	{
		if ( s1->weapon == WP_SABER && cent->gent && cent->gent->owner && cent->gent->owner->inuse )
		{//flying lightsaber
			//FIXME: better way to tell what it is would be nice
			if ( cent->gent->classname && !Q_stricmp( "limb", cent->gent->classname ) )
			{//limb, just add blade
				if ( cent->gent->owner->client )
				{
					if ( cent->gent->owner->client->ps.saberLength > 0 )
					{
						CG_AddSaberBlade( &cg_entities[cent->gent->owner->s.number], 
							&cg_entities[cent->gent->s.number], NULL, ent.renderfx, 
							cent->gent->weaponModel, cent->lerpOrigin, cent->lerpAngles );
					}
					else if ( cent->gent->owner->client->ps.saberEventFlags & SEF_INWATER )
					{
						CG_CheckSaberInWater( &cg_entities[cent->gent->owner->s.number], 
							&cg_entities[cent->gent->s.number], cent->gent->weaponModel, 
							cent->lerpOrigin, cent->lerpAngles );
					}
				}
			}
			else
			{//thrown saber
				//light?  sound?
				if ( cent->gent->owner->client && cg_entities[cent->currentState.otherEntityNum].currentState.saberActive )
				{//saber is in-flight and active, play a sound on it
					if ( cent->gent->owner->client->ps.saberEntityState == SES_RETURNING )
					{
						cgi_S_AddLoopingSound( cent->currentState.number, 
							cent->lerpOrigin, vec3_origin, 
							CG_SaberHumSoundForEnt( cent->gent->owner ) );
					}
					else
					{
						int spinSound;
						switch ( cent->gent->owner->client->ps.forcePowerLevel[FP_SABERTHROW] )
						{
						case FORCE_LEVEL_1:
							spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin3.wav" );
							break;
						case FORCE_LEVEL_2:
							spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin2.wav" );
							break;
						default:
						case FORCE_LEVEL_3:
							spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin1.wav" );
							break;
						}
						cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, 
							vec3_origin, spinSound );
						/*
						if ( cg_weapons[WP_SABER].missileSound )
						{
							cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cg_weapons[WP_SABER].missileSound );
						}
						*/
					}
				}
				if ( cent->gent->owner->client )
				{
					if ( cent->gent->owner->client->ps.saberLength > 0 )
					{//only add the blade if it's on
						CG_AddSaberBlade( &cg_entities[cent->gent->owner->s.number], 
							&cg_entities[cent->gent->s.number], NULL, ent.renderfx, 
							0, cent->lerpOrigin, cent->lerpAngles );
					}
					else if ( cent->gent->owner->client->ps.saberEventFlags & SEF_INWATER )
					{
						CG_CheckSaberInWater( &cg_entities[cent->gent->owner->s.number], 
							&cg_entities[cent->gent->s.number], 0, cent->lerpOrigin, 
							cent->lerpAngles );
					}
				}
				if ( cent->gent->owner->health )
				{
					//make sure we can always be seen
					ent.renderfx |= RF_PULSATE;
				}
			}
		}
	}
	else
	{
		if ( s1->weapon == WP_SABER && cent->gent && cent->gent->owner )
		{//flying lightsaber
			//light?  sound?
			if ( cent->gent->owner->client && cent->currentState.saberActive )
			{//saber is in-flight and active, play a sound on it
				if ( cent->gent->owner->client->ps.saberEntityState == SES_RETURNING )
				{
					if ( cg_weapons[WP_SABER].firingSound )
					{
						cgi_S_AddLoopingSound( cent->currentState.number, 
							cent->lerpOrigin, vec3_origin, cg_weapons[WP_SABER].firingSound );
					}
				}
				else
				{
					int spinSound;
					switch ( cent->gent->owner->client->ps.forcePowerLevel[FP_SABERTHROW] )
					{
					case FORCE_LEVEL_1:
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin3.wav" );
						break;
					case FORCE_LEVEL_2:
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin2.wav" );
						break;
					default:
					case FORCE_LEVEL_3:
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin1.wav" );
						break;
					}
					cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, 
						vec3_origin, spinSound );
					/*
					if ( cg_weapons[WP_SABER].missileSound )
					{
						cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cg_weapons[WP_SABER].missileSound );
					}
					*/
				}
			}
			CG_AddSaberBlade( &cg_entities[cent->gent->owner->s.number], 
				NULL, &ent, ent.renderfx, 0, NULL, NULL );

			if ( cent->gent->owner->health )
			{
				//make sure we can always be seen
				ent.renderfx |= RF_PULSATE;
			}
		}
	}
/*
Ghoul2 Insert End
*/

	if ( cent->gent && cent->gent->forcePushTime > cg.time )
	{//FIXME: if I'm a rather large model, this will look kind of stupid...
		CG_ForcePushBlur( cent->lerpOrigin );
	}
	CG_AddRefEntWithTransportEffect( cent, &ent );

	if ( cent->gent->health <= 0 && cent->gent->s.weapon == WP_EMPLACED_GUN && cent->gent->e_ThinkFunc )
	{
		// make the gun pulse red to warn about it exploding
		float val = (1.0f - (float)(cent->gent->nextthink - cg.time) / 3200.0f ) * 0.3f;

		ent.customShader = cgi_R_RegisterShader( "gfx/effects/solidWhite" );
		ent.shaderRGBA[0] = (sin( cg.time * 0.04f ) * val * 0.4f + val) * 255;
		ent.shaderRGBA[1] = ent.shaderRGBA[2] = 0;
		ent.renderfx |= RF_RGB_TINT;
		cgi_R_AddRefEntityToScene( &ent );
	}
/*
Ghoul2 Insert Start
*/

	if (cg_debugBB.integer)
	{
		CG_CreateBBRefEnts(s1, cent->lerpOrigin);
	}
/*
Ghoul2 Insert End
*/
	//--------------------------
	if ( s1->eFlags & EF_FIRING && cent->gent->inuse )
	{
		//special code for adding the beam to the attached tripwire mine
		vec3_t			beamOrg;
		int				handle = 0;
		SEffectTemplate	*temp;

		VectorMA( ent.origin, 6.6f, ent.axis[0], beamOrg );// forward

		// overriding the effect, so give us a copy first
		temp = theFxScheduler.GetEffectCopy( "tripMine/laser", &handle );	

		if ( temp )
		{
			// have a copy, so get the line element out of there
			CPrimitiveTemplate *prim = theFxScheduler.GetPrimitiveCopy( temp, "line1" );

			if ( prim )
			{
				// we have the primitive, so modify the endpoint
				prim->mOrigin2X.SetRange( cent->gent->pos4[0], cent->gent->pos4[0] );
				prim->mOrigin2Y.SetRange( cent->gent->pos4[1], cent->gent->pos4[1] );
				prim->mOrigin2Z.SetRange( cent->gent->pos4[2], cent->gent->pos4[2] );

				// have a copy, so get the line element out of there
				CPrimitiveTemplate *prim = theFxScheduler.GetPrimitiveCopy( temp, "line2" );

				if ( prim )
				{
					// we have the primitive, so modify the cent->gent->pos3point
					prim->mOrigin2X.SetRange( cent->gent->pos4[0], cent->gent->pos4[0] );
					prim->mOrigin2Y.SetRange( cent->gent->pos4[1], cent->gent->pos4[1] );
					prim->mOrigin2Z.SetRange( cent->gent->pos4[2], cent->gent->pos4[2] );

					// play the modified effect
					theFxScheduler.PlayEffect( handle, beamOrg, ent.axis[0] );
				}
			}
		}

		theFxScheduler.PlayEffect( "tripMine/laserImpactGlow", cent->gent->pos4, ent.axis[0] );
	}


	if ( s1->eFlags & EF_PROX_TRIP )
	{
		//special code for adding the glow end to proximity tripmine
		vec3_t	beamOrg;

		VectorMA( ent.origin, 6.6f, ent.axis[0], beamOrg );// forward
		theFxScheduler.PlayEffect( "tripMine/glowBit", beamOrg, ent.axis[0] );
	}

	if ( s1->eFlags & EF_ALT_FIRING )
	{
		// hack for the spotlight
		vec3_t	org, axis[3], dir;

		AngleVectors( cent->lerpAngles, dir, NULL, NULL );

		CG_GetTagWorldPosition( &ent, "tag_flash", org, axis );

		theFxScheduler.PlayEffect( "env/light_cone", org, axis[0] );

		VectorMA( cent->lerpOrigin, cent->gent->radius - 5, dir, org ); // stay a bit back from the impact point...this may not be enough?

		cgi_R_AddLightToScene( org, 225, 1.0f, 1.0f, 1.0f );
	}

	//-----------------------------------------------------------
	if ( cent->gent->flags & (FL_DMG_BY_HEAVY_WEAP_ONLY | FL_SHIELDED ))
	{
		// Dumb assumption, but I guess we must be a shielded ion_cannon??  We should probably verify
		// if it's an ion_cannon that's Heavy Weapon only, we don't want to make it shielded do we...?
		if ( (!strcmp( "misc_ion_cannon", cent->gent->classname )) && (cent->gent->flags & FL_SHIELDED ))
		{
			// must be doing "pain"....er, impact
			if ( cent->gent->painDebounceTime > cg.time )
			{
				float t = (float)(cent->gent->painDebounceTime - cg.time ) / 1000.0f;

				// Only display when we have damage
				if ( t >= 0.0f && t <= 1.0f ) 
				{
					t *= random();

					ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = 255.0f * t;
					ent.shaderRGBA[3] = 255;
					ent.renderfx &= ~RF_ALPHA_FADE;
					ent.renderfx |= RF_RGB_TINT;
					ent.customShader = cgi_R_RegisterShader( "gfx/misc/ion_shield" );
				
					cgi_R_AddRefEntityToScene( &ent );
				}
			}
		}
	}
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	cgi_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.sound_precache[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = (int)(cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom());
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) 
{
	refEntity_t			ent;
	entityState_t		*es;
	gitem_t				*item;
//	int					msec;
//	float				frac;
	float				scale;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) 
	{
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}
/*
Ghoul2 Insert Start
*/

	// if set to invisible, skip
	if ( (!es->modelindex && !cent->gent->ghoul2.IsValid() ) || ( es->eFlags & EF_NODRAW ) ) 
	{
		return;
	}
/*
Ghoul2 Insert End
*/
	if ( cent->gent && !cent->gent->inuse )
	{
		// Yeah, I know....items were being freed on touch, but it could still get here and draw incorrectly...
		return;
	}

	item = &bg_itemlist[ es->modelindex ];

	if ( cg_simpleItems.integer ) 
	{
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		cgi_R_AddRefEntityToScene(&ent);
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// items bob up and down continuously
	if( item->giType == IT_HOLOCRON )
	{
		scale = 0.005f + cent->currentState.number * 0.00001f;
		cent->lerpOrigin[2] += (float)(4 + cos( ( cg.time + 1000 ) *  scale ) * 3)+8; // just raised them up a bit
	}



	// autorotate at one of two speeds
//	if ( item->giType == IT_HEALTH ) {
//		VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
//		AxisCopy( cg.autoAxisFast, ent.axis );
//	} else {
	if( item->giType == IT_HOLOCRON )
	{
		VectorCopy( cg.autoAngles, cent->lerpAngles );
		AxisCopy( cg.autoAxis, ent.axis );
	}


	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
//	if ( item->giType == IT_WEAPON ) {
//		weaponInfo_t	*wi;
//
//		wi = &cg_weapons[item->giTag];
//		cent->lerpOrigin[0] -= 
//			wi->weaponMidpoint[0] * ent.axis[0][0] +
//			wi->weaponMidpoint[1] * ent.axis[1][0] +
//			wi->weaponMidpoint[2] * ent.axis[2][0];
//		cent->lerpOrigin[1] -= 
//			wi->weaponMidpoint[0] * ent.axis[0][1] +
//			wi->weaponMidpoint[1] * ent.axis[1][1] +
//			wi->weaponMidpoint[2] * ent.axis[2][1];
//		cent->lerpOrigin[2] -= 
//			wi->weaponMidpoint[0] * ent.axis[0][2] +
//			wi->weaponMidpoint[1] * ent.axis[1][2] +
//			wi->weaponMidpoint[2] * ent.axis[2][2];

//		cent->lerpOrigin[2] += 8;	// an extra height boost
//	}
	vec3_t spinAngles;

	//AxisClear( ent.axis );
	VectorCopy( cent->gent->s.angles, spinAngles );

	ent.hModel = cg_items[es->modelindex].models;
/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);  
/*
Ghoul2 Insert End
*/

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// lovely...this is for weapons that should be oriented vertically.  For weapons lockers and such.
	if ( cent->gent->spawnflags & 16 )
	{	//VectorClear( spinAngles );
		spinAngles[PITCH] -= 75;
	}
	
	if( item->giType != IT_HOLOCRON )
	{
		AnglesToAxis( spinAngles, ent.axis );
	}

	// items without glow textures need to keep a minimum light value
	// so they are always visible
/*	if (( item->giType == IT_WEAPON ) || ( item->giType == IT_ARMOR )) 
	{
		ent.renderfx |= RF_MINLIGHT;
	}
*/
	// increase the size of the weapons when they are presented as items
//	if ( item->giType == IT_WEAPON ) {
//		VectorScale( ent.axis[0], 1.5f, ent.axis[0] );
//		VectorScale( ent.axis[1], 1.5f, ent.axis[1] );
//		VectorScale( ent.axis[2], 1.5f, ent.axis[2] );
//		ent.nonNormalizedAxes = qtrue;
//	}

	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);

	if ( item->giType == IT_WEAPON && item->giTag == WP_SABER )
	{
		// saber pickup item needs to be more visible
		float	wv;
		vec3_t	org;

		ent.customShader = cgi_R_RegisterShader( "gfx/effects/solidWhite_cull" );
		ent.renderfx = RF_RGB_TINT;
		wv = sin( cg.time * 0.002f ) * 0.08f + 0.2f;
		ent.shaderRGBA[0] = ent.shaderRGBA[1] = wv * 255;
		ent.shaderRGBA[2] = 0;
		cgi_R_AddRefEntityToScene(&ent);

		for ( int i = -4; i < 10; i += 1 )
		{
			VectorMA( ent.origin, -i, ent.axis[2], org );

			FX_AddSprite( org, NULL, NULL, 10.0f, 10.0f, wv * 0.5f, wv * 0.5f, 0.0f, 0.0f, 1.0f, cgs.media.yellowDroppedSaberShader, 0x08000000 );
		}

		// THIS light looks crappy...maybe it should just be removed...
		cgi_R_AddLightToScene( ent.origin, wv * 350 + 180, 1.0f, 1.0f, 0.0f );
	}
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t	*weapon;
	const weaponData_t  *wData;

	if ( !cent->gent->inuse )
		return;

	s1 = &cent->currentState;
	if ( s1->weapon >= WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];
	wData = &weaponData[s1->weapon];

	if ( s1->pos.trType != TR_INTERPOLATE )
	{
		// calculate the axis
		VectorCopy( s1->angles, cent->lerpAngles );
	}

	if ( cent->gent->alt_fire )
	{
		// add trails
		if ( weapon->alt_missileTrailFunc )  
			weapon->alt_missileTrailFunc( cent, weapon );

		// add dynamic light
		if ( wData->alt_missileDlight )
				cgi_R_AddLightToScene(cent->lerpOrigin, wData->alt_missileDlight, 
					wData->alt_missileDlightColor[0], wData->alt_missileDlightColor[1], wData->alt_missileDlightColor[2] );

		// add missile sound
		if ( weapon->alt_missileSound )
			cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->alt_missileSound );

		//Don't draw something without a model
		if ( weapon->alt_missileModel == NULL_HANDLE )
			return;
	}
	else
	{
		// add trails
		if ( weapon->missileTrailFunc )  
			weapon->missileTrailFunc( cent, weapon );

		// add dynamic light
		if ( wData->missileDlight )
			cgi_R_AddLightToScene(cent->lerpOrigin, wData->missileDlight, 
				wData->missileDlightColor[0], wData->missileDlightColor[1], wData->missileDlightColor[2] );

		// add missile sound
		if ( weapon->missileSound )
			cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound );

		//Don't draw something without a model
		if ( weapon->missileModel == NULL_HANDLE )
			return;
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);  

/*
Ghoul2 Insert End
*/

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.renderfx = /*weapon->missileRenderfx | */RF_NOSHADOW;

	if ( cent->gent->alt_fire )
		ent.hModel = weapon->alt_missileModel;
	else
		ent.hModel = weapon->missileModel;

	// spin as it moves
	if ( s1->apos.trType != TR_INTERPOLATE )
	{
		// convert direction of travel into axis
		if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
			ent.axis[0][2] = 1;
		}
		if ( s1->pos.trType != TR_STATIONARY )
		{
			if ( s1->eFlags & EF_MISSILE_STICK )
				RotateAroundDirection( ent.axis, cg.time * 0.5f );//Did this so regular missiles don't get broken
			else
				RotateAroundDirection( ent.axis, cg.time * 0.25f );//JFM:FLOAT FIX
		}
		else
		{
			if ( s1->eFlags & EF_MISSILE_STICK )
				RotateAroundDirection( ent.axis, (float)s1->pos.trTime * 0.5f );
			else
				RotateAroundDirection( ent.axis, (float)s1->time );
		}
	}
	else
	{
		AnglesToAxis( cent->lerpAngles, ent.axis );
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups( &ent, s1->powerups, NULL );
}

/*
===============
CG_Mover
===============
*/

#define DOOR_OPENING	1
#define DOOR_CLOSING	2
#define DOOR_OPEN		3
#define DOOR_CLOSED		4

static void CG_Mover( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	//FIXME: why are these always 0, 0, 0???!
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );
/*
Ghoul2 Insert Start
*/

	CG_SetGhoul2Info(&ent, cent);  
/*
Ghoul2 Insert End
*/

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.model_draw[s1->modelindex];
	}

	// If there isn't an hModel for this mover, an RGB axis model will get drawn.
	if ( !ent.hModel )
	{
		return;
	}

	if ( cent->currentState.eFlags & EF_DISABLE_SHADER_ANIM )
	{
		// by setting the shader time to the current time, we can force an animating shader to not animate
		ent.shaderTime = cg.time * 0.001f;
	}

	// add the secondary model
	if ( s1->solid == SOLID_BMODEL && s1->modelindex2 ) 
	{
//		vec3_t	org;
		if ( !(s1->eFlags & EF_NODRAW) )
		{
			// add to refresh list
			CG_AddRefEntWithTransportEffect( cent, &ent );
		}

/*		// Um, this does not interpolate nicely?  Not sure why it was here....
		VectorAdd(cent->gent->absmin, cent->gent->absmax, org);
		VectorScale(org, 0.5, org);
		VectorCopy( org, ent.origin);
		VectorCopy( org, ent.oldorigin);
*/
		ent.hModel = cgs.model_draw[s1->modelindex2];
	}

	// I changed it to always do it because nodraw seemed like it should actually do what it says. Be aware that if you change this,
	//	the movers for the shooting gallery on doom_detention will break.
	if ( (s1->eFlags & EF_NODRAW) )
	{
		return;
	}
	//fall through and render the hModel or...

	//We're a normal model being moved, animate our model
	ent.skinNum = 0;
	if ( s1->eFlags & EF_ANIM_ONCE )
	{//FIXME: needs to anim at once per 100 ms
		ent.frame = cent->gent->s.frame;
		ent.renderfx|=RF_CAP_FRAMES;
	}
	else if ( s1->eFlags & EF_ANIM_ALLFAST )
	{
		ent.frame = (cg.time / 100);
		ent.renderfx|=RF_WRAP_FRAMES;
	}
	else
	{
		ent.frame = s1->frame;
	}

	if ( s1->eFlags & EF_SHADER_ANIM )
	{
		ent.renderfx|=RF_SETANIMINDEX;
		ent.skinNum = s1->frame;
		//ent.shaderTime = cg.time*0.001f - s1->frame/s1->time;//NOTE: s1->time is number of frames
	}

	// add to refresh list
	CG_AddRefEntWithTransportEffect( cent, &ent );
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent, int color ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;
	ent.skinNum = color;

	ent.renderfx = RF_NOSHADOW;
/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);  

/*
Ghoul2 Insert End
*/

	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);
}

static vec2_t st[] = 
{
	{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }
};

void CG_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha ) 
{
	vec3_t	point[4], rot={0,0,0};
	int		vec[3];
	int		axis, i;

	for ( axis = 0, vec[0] = 0, vec[1] = 1, vec[2] = 2; axis < 3; axis++, vec[0]++, vec[1]++, vec[2]++ )
	{
		for ( i = 0; i < 3; i++ )
		{
			if ( vec[i] > 2 )
			{
				vec[i] = 0;
			}
		}

		point[0][vec[1]] = mins[vec[1]];
		point[0][vec[2]] = mins[vec[2]];

		point[1][vec[1]] = mins[vec[1]];
		point[1][vec[2]] = maxs[vec[2]];

		point[2][vec[1]] = maxs[vec[1]];
		point[2][vec[2]] = maxs[vec[2]];
		
		point[3][vec[1]] = maxs[vec[1]];
		point[3][vec[2]] = mins[vec[2]];

		//- face
		point[0][vec[0]] = point[1][vec[0]] = point[2][vec[0]] = point[3][vec[0]] = mins[vec[0]];

		FX_AddPoly( point, st, 4, NULL, NULL, alpha, alpha, 0.0f, 
						color, color, 0.0f, rot, 0.0f, 0.0f, 
						100, cgs.media.solidWhiteShader, 0 );

		//+ face
		point[0][vec[0]] = point[1][vec[0]] = point[2][vec[0]] = point[3][vec[0]] = maxs[vec[0]];

		FX_AddPoly( point, st, 4, NULL, NULL, alpha, alpha, 0.0f, 
						color, color, 0.0f, rot, 0.0f, 0.0f, 
						100, cgs.media.solidWhiteShader, 0 );
	}
}

void CG_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha )
{
	vec3_t	point1, point2, point3, point4;
	int		vec[3];
	int		axis, i;

	for ( axis = 0, vec[0] = 0, vec[1] = 1, vec[2] = 2; axis < 3; axis++, vec[0]++, vec[1]++, vec[2]++ )
	{
		for ( i = 0; i < 3; i++ )
		{
			if ( vec[i] > 2 )
			{
				vec[i] = 0;
			}
		}

		point1[vec[1]] = mins[vec[1]];
		point1[vec[2]] = mins[vec[2]];

		point2[vec[1]] = mins[vec[1]];
		point2[vec[2]] = maxs[vec[2]];

		point3[vec[1]] = maxs[vec[1]];
		point3[vec[2]] = maxs[vec[2]];
		
		point4[vec[1]] = maxs[vec[1]];
		point4[vec[2]] = mins[vec[2]];

		//- face
		point1[vec[0]] = point2[vec[0]] = point3[vec[0]] = point4[vec[0]] = mins[vec[0]];

		CG_TestLine( point1, point2, time, color, 1 );
		CG_TestLine( point2, point3, time, color, 1 );
		CG_TestLine( point1, point4, time, color, 1 );
		CG_TestLine( point4, point3, time, color, 1 );

		//+ face
		point1[vec[0]] = point2[vec[0]] = point3[vec[0]] = point4[vec[0]] = maxs[vec[0]];

		CG_TestLine( point1, point2, time, color, 1 );
		CG_TestLine( point2, point3, time, color, 1 );
		CG_TestLine( point1, point4, time, color, 1 );
		CG_TestLine( point4, point1, time, color, 1 );
	}
}

void CG_Line( vec3_t start, vec3_t end, vec3_t color, float alpha ) 
{
	/*FX_AddLine( start,
				end,
				1.0f,
				1.0,
				1.0f,
				alpha,
				alpha,
				color, 
				color, 
				100.0f,
				cgs.media.whiteShader );*/
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	//FIXME: this tends to give a bad axis[1], perhaps we
	//should just do the VectorSubtraction here rather than
	//on the game side.  Would also allow camera to follow
	//a moving target.

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = (int)(s1->clientNum/256.0 * 360);	// roll offset

/*
Ghoul2 Insert Start
*/
	CG_SetGhoul2Info(&ent, cent);  
/*
Ghoul2 Insert End
*/


	// add to refresh list
	cgi_R_AddRefEntityToScene(&ent);
}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int atTime, vec3_t out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
//	vec3_t	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

	EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, oldOrigin );
//	EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, oldAngles );

	EvaluateTrajectory( &cent->currentState.pos, atTime, origin );
//	EvaluateTrajectory( &cent->currentState.apos, atTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
//	VectorSubtract( angles, oldAngles, deltaAngles );


	VectorAdd( in, deltaOrigin, out );

	// FIXME: origin change when on a rotating object
}
/*
===============
CG_CalcEntityLerpPositions

===============
*/
extern char	*vtos( const vec3_t v );
#if 1
void CG_CalcEntityLerpPositions( centity_t *cent ) {
	if ( cent->currentState.number == cg.snap->ps.clientNum)
	{
		// if the player, take position from prediction
		VectorCopy( cg.predicted_player_state.origin, cent->lerpOrigin );
		VectorCopy( cg.predicted_player_state.viewangles, cent->lerpAngles );
/*
Ghoul2 Insert Start
*/
//		LerpBoneAngleOverrides(cent);
/*
Ghoul2 Insert End
*/
		return;
	}
	
	//FIXME: prediction on clients in timescale results in jerky positional translation
	if ( cent->interpolate )
	{
		// if the entity has a valid next state, interpolate a value between the frames
		// unless it is a mover with a known start and stop
		vec3_t		current, next;
		float		f;

		// it would be an internal error to find an entity that interpolates without
		// a snapshot ahead of the current one
		if ( cg.nextSnap == NULL ) {
			CG_Error( "CG_AddCEntity: cg.nextSnap == NULL" );
		}

		f = cg.frameInterpolation;

		if ( cent->currentState.apos.trType == TR_INTERPOLATE )
		{
			EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
			EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

			cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
			cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
			cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

			/*
			if(cent->gent && cent->currentState.eFlags & EF_NPC && !VectorCompare(current, next))
			{
				Com_Printf("%s last/next/lerp apos %s/%s/%s, f = %4.2f\n", cent->gent->script_targetname, vtos(current), vtos(next), vtos(cent->lerpAngles), f);
			}
			*/
	/*
	Ghoul2 Insert Start
	*/
			// now the nasty stuff - this will interpolate all ghoul2 models bone angle overrides per model attached to this cent
			/*
			if (cent->gent->ghoul2.size())
			{
				LerpBoneAngleOverrides(cent);
			}
			*/
	/*
	Ghoul2 Insert End
	*/
		}
		if ( cent->currentState.pos.trType == TR_INTERPOLATE ) 
		{
			// this will linearize a sine or parabolic curve, but it is important
			// to not extrapolate player positions if more recent data is available
			EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
			EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

			cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
			cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
			cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

			/*
			if ( cent->gent && cent->currentState.eFlags & EF_NPC )
			{
				Com_Printf("%s last/next/lerp pos %s/%s/%s, f = %4.2f\n", cent->gent->script_targetname, vtos(current), vtos(next), vtos(cent->lerpOrigin), f);
			}
			*/
			return;//FIXME: should this be outside this if?
		}
	}
	else
	{
		if ( cent->currentState.apos.trType == TR_INTERPOLATE )
		{
			EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, cent->lerpAngles );
		}
		if ( cent->currentState.pos.trType == TR_INTERPOLATE ) 
		{
			EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
			/*
			if(cent->gent && cent->currentState.eFlags & EF_NPC )
			{
				Com_Printf("%s last/next/lerp pos %s, f = 1.0\n", cent->gent->script_targetname, vtos(cent->lerpOrigin) );
			}
			*/
			return;
		}
	}
	
	// FIXME: if it's blocked, it wigs out, draws it in a predicted spot, but never
	// makes it there - we need to predict it in the right place if this is happens...

	// just use the current frame and evaluate as best we can
	trajectory_t *posData = &cent->currentState.pos;
	{
		gentity_t *ent = &g_entities[cent->currentState.number];

		if ( ent && ent->inuse)
		{
			if ( ent->s.eFlags & EF_BLOCKED_MOVER || ent->s.pos.trType == TR_STATIONARY )
			{//this mover has stopped moving and is going to wig out if we predict it
				//based on last frame's info- cut across the network and use the currentOrigin
				VectorCopy( ent->currentOrigin, cent->lerpOrigin );
				posData = NULL;
			}
			else
			{
				posData = &ent->s.pos;
			}
		}
	}

	if ( posData )
	{
		EvaluateTrajectory( posData, cg.time, cent->lerpOrigin );
	}

	// FIXME: this will stomp an apos trType of TR_INTERPOLATE!!
	EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover
	CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, cg.time, cent->lerpOrigin );
/*
Ghoul2 Insert Start
*/
		// now the nasty stuff - this will interpolate all ghoul2 models bone angle overrides per model attached to this cent
	/*
	if (cent->gent->ghoul2.size())
	{
		LerpBoneAngleOverrides(cent);
	}
	*/
/*
Ghoul2 Insert End
*/
	// FIXME: perform general error decay?
}
#else
void CG_CalcEntityLerpPositions( centity_t *cent ) 
{
	if ( cent->currentState.number == cg.snap->ps.clientNum)
	{
		// if the player, take position from prediction
		VectorCopy( cg.predicted_player_state.origin, cent->lerpOrigin );
		VectorCopy( cg.predicted_player_state.viewangles, cent->lerpAngles );
OutputDebugString(va("b=(%6.2f,%6.2f,%6.2f)\n",cent->lerpOrigin[0],cent->lerpOrigin[1],cent->lerpOrigin[2]));
		return;
	}


if (cent->currentState.number != cg.snap->ps.clientNum&&cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE)
{
	if (cent->interpolate)
	{
OutputDebugString(va("[%3d] interp %4.2f t=%6d  st = %6d  nst = %6d     b=%6.2f   nb=%6.2f\n",
		cent->currentState.number,
		cg.frameInterpolation,
		cg.time,
		cg.snap->serverTime,
		cg.nextSnap->serverTime,
		cent->currentState.pos.trBase[0],
		cent->nextState.pos.trBase[0]));
	}
	else
	{
OutputDebugString(va("[%3d] nonext %4.2f t=%6d  st = %6d  nst = %6d     b=%6.2f   nb=%6.2f\n",
		cent->currentState.number,
		cg.frameInterpolation,
		cg.time,
		cg.snap->serverTime,
		0,
		cent->currentState.pos.trBase[0],
		0.0f));
	}
}

	//FIXME: prediction on clients in timescale results in jerky positional translation
	if (cent->interpolate && 
		(cent->currentState.number == cg.snap->ps.clientNum ||
		cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) )
	{
		vec3_t		current, next;
		float		f;

		// it would be an internal error to find an entity that interpolates without
		// a snapshot ahead of the current one
		if ( cg.nextSnap == NULL ) 
		{
			CG_Error( "CG_AddCEntity: cg.nextSnap == NULL" );
		}

		f = cg.frameInterpolation;

		EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
		EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

		cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
		cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
		cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

		EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
		EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

		cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
		cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
		cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );
		return;
	}
	// just use the current frame and evaluate as best we can
	trajectory_t *posData = &cent->currentState.pos;
	{
		gentity_t *ent = &g_entities[cent->currentState.number];

		if ( ent && ent->inuse)
		{
			if ( ent->s.eFlags & EF_BLOCKED_MOVER || ent->s.pos.trType == TR_STATIONARY )
			{//this mover has stopped moving and is going to wig out if we predict it
				//based on last frame's info- cut across the network and use the currentOrigin
				VectorCopy( ent->currentOrigin, cent->lerpOrigin );
				posData = NULL;
			}
			else
			{
				posData = &ent->s.pos;
				EvaluateTrajectory(&ent->s.pos,cg.time, cent->lerpOrigin );
			}
		}
		else
		{
			EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
		}
	}
	EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover
	CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, cg.time, cent->lerpOrigin );
}
#endif
/*
===============
CG_AddLocalSet
===============
*/

static void CG_AddLocalSet( centity_t *cent )
{
	cent->gent->setTime = cgi_S_AddLocalSet( cent->gent->soundSet, cg.refdef.vieworg, cent->lerpOrigin, cent->gent->s.number, cent->gent->setTime );
}

/*
-------------------------
CAS_GetBModelSound
-------------------------
*/

sfxHandle_t CAS_GetBModelSound( const char *name, int stage )
{
	return cgi_AS_GetBModelSound( name, stage );
}

void CG_DLightThink ( centity_t *cent )
{
	if(cent->gent)
	{
		float	tDelta = cg.time - cent->gent->painDebounceTime;
		float	percentage = ( tDelta/((float)cent->gent->speed) );
		vec3_t	org;
		vec4_t	currentRGBA;
		gentity_t	*owner = NULL;
		int		i;
		
		if ( percentage >= 1.0f )
		{//We hit the end
			percentage = 1.0f;
			switch( cent->gent->pushDebounceTime )
			{
			case 0://Fading from start to final
				if ( cent->gent->spawnflags & 8 )
				{//PULSER
					if ( tDelta - cent->gent->speed - cent->gent->wait >= 0 )
					{//Time to start fading down
						cent->gent->painDebounceTime = cg.time;
						cent->gent->pushDebounceTime = 1;
						percentage = 0.0f;
					}
				}
				else
				{//Stick on startRGBA
					percentage = 0.0f;
				}
				break;
			case 1://Fading from final to start
				if ( tDelta - cent->gent->speed - cent->gent->radius >= 0 )
				{//Time to start fading up
					cent->gent->painDebounceTime = cg.time;
					cent->gent->pushDebounceTime = 0;
					percentage = 0.0f;
				}
				break;
			case 2://Fading from 0 intensity to start intensity
				//Time to start fading from start to final
				cent->gent->painDebounceTime = cg.time;
				cent->gent->pushDebounceTime = 0;
				percentage = 0.0f;
				break;
			case 3://Fading from current intensity to 0 intensity
				//Time to turn off
				cent->gent->misc_dlight_active = qfalse;
				cent->gent->e_clThinkFunc = clThinkF_NULL;
				cent->gent->s.eType = ET_GENERAL;
				cent->gent->svFlags &= ~SVF_BROADCAST;
				return;		
				break;
			default:
				break;
			}
		}

		switch( cent->gent->pushDebounceTime )
		{
		case 0://Fading from start to final
			for ( i = 0; i < 4; i++ )
			{
				currentRGBA[i] = cent->gent->startRGBA[i] + ( (cent->gent->finalRGBA[i] - cent->gent->startRGBA[i]) * percentage );
			}
			break;
		case 1://Fading from final to start
			for ( i = 0; i < 4; i++ )
			{
				currentRGBA[i] = cent->gent->finalRGBA[i] + ( (cent->gent->startRGBA[i] - cent->gent->finalRGBA[i]) * percentage );
			}
			break;
		case 2://Fading from 0 intensity to start
			for ( i = 0; i < 3; i++ )
			{
				currentRGBA[i] = cent->gent->startRGBA[i];
			}
			currentRGBA[3] = cent->gent->startRGBA[3] * percentage;
			break;
		case 3://Fading from current intensity to 0
			for ( i = 0; i < 3; i++ )
			{//FIXME: use last
				currentRGBA[i] = cent->gent->startRGBA[i];
			}
			currentRGBA[3] = cent->gent->startRGBA[3] - (cent->gent->startRGBA[3] * percentage);
			break;
		default:
			return;
			break;
		}

		if ( cent->gent->owner )
		{
			owner = cent->gent->owner;
		}
		else
		{
			owner = cent->gent;
		}

		if ( owner->s.pos.trType == TR_INTERPOLATE )
		{
			VectorCopy( cg_entities[owner->s.number].lerpOrigin, org );
		}
		else
		{
			VectorCopy( owner->currentOrigin, org );
		}

		cgi_R_AddLightToScene(org, currentRGBA[3]*10, currentRGBA[0], currentRGBA[1], currentRGBA[2] );
	}
}

void CG_Limb ( centity_t *cent )
{//first time we're drawn, remove us from the owner ent
	if ( cent->gent && cent->gent->owner && cent->gent->owner->ghoul2.size() )
	{
		gentity_t	*owner = cent->gent->owner;
		if ( cent->gent->aimDebounceTime )
		{//done with dismemberment, just waiting to mark owner dismemberable again
			if ( cent->gent->aimDebounceTime > cg.time )
			{//still waiting
				return;
			}
			//done!
			owner->client->dismembered = qfalse;
			//done!
			cent->gent->e_clThinkFunc = clThinkF_NULL;
		}
		else
		{
extern cvar_t	*g_dismemberment;
extern cvar_t	*g_saberRealisticCombat;
			//3) turn off w/descendants that surf in original model
			if ( cent->gent->target )//stubTagName )
			{//add smoke to cap surf, spawn effect
				int newBolt = gi.G2API_AddBolt( &owner->ghoul2[owner->playerModel], cent->gent->target );
				if ( newBolt != -1 )
				{
					G_PlayEffect( "blaster/smoke_bolton", owner->playerModel, newBolt, owner->s.number );
				}
			}
			if ( cent->gent->target2 )//limbName
			{//turn the limb off
				gi.G2API_SetSurfaceOnOff( &owner->ghoul2[owner->playerModel], cent->gent->target2, 0x00000100 );//G2SURFACEFLAG_NODESCENDANTS
			}
			if ( cent->gent->target3 )//stubCapName )
			{//turn on caps
				gi.G2API_SetSurfaceOnOff( &owner->ghoul2[owner->playerModel], cent->gent->target3, 0 );
			}
			if ( owner->weaponModel >= 0 )
			{//the corpse hasn't dropped their weapon
				if ( cent->gent->count == BOTH_DISMEMBER_RARM || cent->gent->count == BOTH_DISMEMBER_TORSO1 )//&& ent->s.weapon == WP_SABER && ent->weaponModel != -1 )
				{//FIXME: is this first check needed with this lower one?
					gi.G2API_RemoveGhoul2Model( owner->ghoul2, owner->weaponModel );
					owner->weaponModel = -1;
				}
			}
			if ( owner->client->NPC_class == CLASS_PROTOCOL 
				|| g_dismemberment->integer >= 11381138
				|| g_saberRealisticCombat->integer )
			{
				//wait 100ms before allowing owner to be dismembered again
				cent->gent->aimDebounceTime = cg.time + 100;
				return;
			}
			else
			{
				//done!
				cent->gent->e_clThinkFunc = clThinkF_NULL;
			}
		}
	}
}

qboolean	MatrixMode = qfalse;
void CG_MatrixEffect ( centity_t *cent )
{
	float	MATRIX_EFFECT_TIME = 1000.0f;
	//VectorCopy( cent->lerpOrigin, cg.refdef.vieworg );
	float totalElapsedTime = (float)(cg.time - cent->currentState.time);
	float elapsedTime = totalElapsedTime;
	if ( totalElapsedTime > cent->currentState.eventParm //MATRIX_EFFECT_TIME
		|| (cent->currentState.weapon&&g_entities[cent->currentState.otherEntityNum].client&&g_entities[cent->currentState.otherEntityNum].client->ps.groundEntityNum!=ENTITYNUM_NONE) 
		|| cg.missionStatusShow )
	{//time is up or this is a falling spin and they hit the ground or mission end screen is up
		cg.overrides.active &= ~(/*CG_OVERRIDE_3RD_PERSON_ENT|*/CG_OVERRIDE_3RD_PERSON_RNG|CG_OVERRIDE_3RD_PERSON_ANG|CG_OVERRIDE_3RD_PERSON_POF);
		//cg.overrides.thirdPersonEntity = 0;
		cg.overrides.thirdPersonAngle = 0;
		cg.overrides.thirdPersonPitchOffset = 0;
		cg.overrides.thirdPersonRange = 0;
		cgi_Cvar_Set( "timescale", "1.0" );
		MatrixMode = qfalse;
		cent->gent->e_clThinkFunc = clThinkF_NULL;
		return;
	}
	else 
	{
		while ( elapsedTime > MATRIX_EFFECT_TIME )
		{
			elapsedTime -= MATRIX_EFFECT_TIME;
		}
	}
	
	MatrixMode = qtrue;

	//FIXME: move the position towards them and back?
	//cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ENT;
	//cg.overrides.thirdPersonEntity = cent->currentState.otherEntityNum;

	//rotate
	cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
	cg.overrides.thirdPersonAngle = 360.0f*elapsedTime/MATRIX_EFFECT_TIME;

	if ( !cent->currentState.weapon ) 
	{//go ahead and do all the slowdown and vert bob stuff
		//slowdown
		float timescale = (elapsedTime/MATRIX_EFFECT_TIME);
		if ( timescale < 0.01f )
		{
			timescale = 0.01f;
		}
		cgi_Cvar_Set( "timescale", va("%4.2f",timescale) );

		//pitch
		//dip - FIXME: use pitchOffet?
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_POF;
		cg.overrides.thirdPersonPitchOffset = cg_thirdPersonPitchOffset.value;
		if ( elapsedTime < MATRIX_EFFECT_TIME*0.33f )
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f*elapsedTime/(MATRIX_EFFECT_TIME*0.33);
		}
		else if ( elapsedTime > MATRIX_EFFECT_TIME*0.66f )
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f*(MATRIX_EFFECT_TIME-elapsedTime)/(MATRIX_EFFECT_TIME*0.33);
		}
		else
		{
			cg.overrides.thirdPersonPitchOffset -= 30.0f;
		}

		//pull back
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
		cg.overrides.thirdPersonRange = cg_thirdPersonRange.value;
		if ( elapsedTime < MATRIX_EFFECT_TIME*0.33 )
		{
			cg.overrides.thirdPersonRange += 80.0f*elapsedTime/(MATRIX_EFFECT_TIME*0.33);
		}
		else if ( elapsedTime > MATRIX_EFFECT_TIME*0.66 )
		{
			cg.overrides.thirdPersonRange += 80.0f*(MATRIX_EFFECT_TIME-elapsedTime)/(MATRIX_EFFECT_TIME*0.33);
		}
		else
		{
			cg.overrides.thirdPersonRange += 80.0f;
		}
	}
	else
	{//FIXME: if they're on the ground, stop spinning and stop timescale
		//FIXME: if they go to the menu, restore timescale
		cgi_Cvar_Set( "timescale", "0.25f" );
	}
}

static void CG_Think ( centity_t *cent )
{
	if(!cent->gent)
	{
		return;
	}

	CEntity_ThinkFunc(cent);	//	cent->gent->clThink(cent);
}

static void CG_Clouds( centity_t *cent )
{
	refEntity_t		ent;

	// create the render entity
	memset( &ent, 0, sizeof( ent ));

	VectorCopy( cent->lerpOrigin, ent.origin );

	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[2] = ent.shaderRGBA[3] = 255;

	ent.radius = cent->gent->radius;
	ent.backlerp = cent->gent->wait;

	ent.reType = RT_CLOUDS;

	if ( cent->gent->spawnflags & 1 ) // TUBE type, the one with a hole in the middle
	{
		ent.rotation = cent->gent->random;
		ent.renderfx = RF_GROW;// tube flag 
	}

	if ( cent->gent->spawnflags & 2 ) // ALT type, uses a different shader
	{
		ent.customShader = cgi_R_RegisterShader( "gfx/world/haze2" );
	}
	else
	{
		ent.customShader = cgi_R_RegisterShader( "gfx/world/haze" );
	}
	
	cgi_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) 
{
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	//we must have restarted the game
	if (!cent->gent)
	{
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	// add local sound set if any
	if ( cent->gent && cent->gent->soundSet && cent->gent->soundSet[0] && cent->currentState.eType != ET_MOVER )
	{
		CG_AddLocalSet( cent );
	}
/*
Ghoul2 Insert Start
*/
	// do this before we copy the data to refEnts
	if (cent->gent->ghoul2.IsValid())
	{
		trap_G2_SetGhoul2ModelIndexes(cent->gent->ghoul2, cgs.model_draw, cgs.skins);
	}

/*
Ghoul2 Insert End
*/

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent, 0 );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		if ( cent->gent && cent->gent->soundSet && cent->gent->soundSet[0] )
		{
			break;
		}
		CG_Speaker( cent );
		break;
	case ET_THINKER:
		CG_General( cent );
		CG_Think( cent );
		break;
	case ET_CLOUD: // dumb
		CG_Clouds( cent );
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) 
	{
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) 
		{
			cg.frameInterpolation = 0;
		} 
		else 
		{
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
//OutputDebugString(va("interp %4.2f ct=%6d nt=%6d st=%6d\n",cg.frameInterpolation,cg.time,cg.nextSnap->serverTime,cg.snap->serverTime));
	} 
	else 
	{
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
//OutputDebugString(va("noterp %4.2f ct=%6d nt=%6d st=%6d\n",cg.frameInterpolation,cg.time,0,cg.snap->serverTime));
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0f;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predicted_player_state;
	PlayerStateToEntityState( ps, &cg_entities[ ps->clientNum ].currentState );
//	cent = &cg_entities[ ps->clientNum ];	// not needed now that player is in the snap packet
//	CG_AddCEntity( cent );					//

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}
}

//rww - This function is not currently called. Use it as the client-side ROFF
//callback once that's implemented fully.
void CG_ROFF_NotetrackCallback( centity_t *cent, const char *notetrack)
{
	int i = 0, r = 0, objectID = 0, anglesGathered = 0, posoffsetGathered = 0;
	char type[256];
	char argument[512];
	char addlArg[512];
	char errMsg[256];
	char t[64];
	int addlArgs = 0;
	vec3_t parsedAngles, parsedOffset, useAngles, useOrigin, forward, right, up;

	if (!cent || !notetrack)
	{
		return;
	}

	//notetrack = "effect effects/explosion1.efx 0+0+64 0-0-1";

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if (notetrack[i] != ' ')
	{ //didn't pass in a valid notetrack type, or forgot the argument for it
		return;
	}

	i++;

	while (notetrack[i] && notetrack[i] != ' ')
	{
		argument[r] = notetrack[i];
		r++;
		i++;
	}
	argument[r] = '\0';

	if (!r)
	{
		return;
	}

	if (notetrack[i] == ' ')
	{ //additional arguments...
		addlArgs = 1;

		i++;
		r = 0;
		while (notetrack[i])
		{
			addlArg[r] = notetrack[i];
			r++;
			i++;
		}
		addlArg[r] = '\0';
	}

	if (strcmp(type, "effect") == 0)
	{
		if (!addlArgs)
		{
			//sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
			//goto functionend;
			VectorClear(parsedOffset);
			goto defaultoffsetposition;
		}

		i = 0;

		while (posoffsetGathered < 3)
		{
			r = 0;
			while (addlArg[i] && addlArg[i] != '+' && addlArg[i] != ' ')
			{
				t[r] = addlArg[i];
				r++;
				i++;
			}
			t[r] = '\0';
			i++;
			if (!r)
			{ //failure..
				//sprintf(errMsg, "Offset position argument for 'effect' type is invalid.");
				//goto functionend;
				VectorClear(parsedOffset);
				i = 0;
				goto defaultoffsetposition;
			}
			parsedOffset[posoffsetGathered] = atof(t);
			posoffsetGathered++;
		}

		if (posoffsetGathered < 3)
		{
			Q_strncpyz(errMsg, "Offset position argument for 'effect' type is invalid.", sizeof(errMsg));
			goto functionend;
		}

		i--;

		if (addlArg[i] != ' ')
		{
			addlArgs = 0;
		}

defaultoffsetposition:

		objectID = theFxScheduler.RegisterEffect(argument);

		if (objectID)
		{
			if (addlArgs)
			{ //if there is an additional argument for an effect it is expected to be XANGLE-YANGLE-ZANGLE
				i++;
				while (anglesGathered < 3)
				{
					r = 0;
					while (addlArg[i] && addlArg[i] != '-')
					{
						t[r] = addlArg[i];
						r++;
						i++;
					}
					t[r] = '\0';
					i++;

					if (!r)
					{ //failed to get a new part of the vector
						anglesGathered = 0;
						break;
					}

					parsedAngles[anglesGathered] = atof(t);
					anglesGathered++;
				}

				if (anglesGathered)
				{
					VectorCopy(parsedAngles, useAngles);
				}
				else
				{ //failed to parse angles from the extra argument provided..
					VectorCopy(cent->lerpAngles, useAngles);
				}
			}
			else
			{ //if no constant angles, play in direction entity is facing
				VectorCopy(cent->lerpAngles, useAngles);
			}

			AngleVectors(useAngles, forward, right, up);

			VectorCopy(cent->lerpOrigin, useOrigin);

			//forward
			useOrigin[0] += forward[0]*parsedOffset[0];
			useOrigin[1] += forward[1]*parsedOffset[0];
			useOrigin[2] += forward[2]*parsedOffset[0];

			//right
			useOrigin[0] += right[0]*parsedOffset[1];
			useOrigin[1] += right[1]*parsedOffset[1];
			useOrigin[2] += right[2]*parsedOffset[1];

			//up
			useOrigin[0] += up[0]*parsedOffset[2];
			useOrigin[1] += up[1]*parsedOffset[2];
			useOrigin[2] += up[2]*parsedOffset[2];

			theFxScheduler.PlayEffect(objectID, useOrigin, useAngles);
		}
	}
	else if (strcmp(type, "sound") == 0)
	{
		objectID = cgi_S_RegisterSound(argument);
		cgi_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_BODY, objectID);
	}
	else if (strcmp(type, "loop") == 0)
	{ //handled server-side
		return;
	}
	//else if ...
	else
	{
		if (type[0])
		{
			Com_Printf("^3Warning: \"%s\" is an invalid ROFF notetrack function\n", type);
		}
		else
		{
			Com_Printf("^3Warning: Notetrack is missing function and/or arguments\n");
		}
	}

	return;

functionend:
	Com_Printf("^3Type-specific notetrack error: %s\n", errMsg);
	return;
}
