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

// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"

/*
Ghoul2 Insert Start
*/
// set up the appropriate ghoul2 info to a refent
void CG_SetGhoul2InfoRef( refEntity_t *ent, refEntity_t	*s1)
{
	ent->ghoul2 = s1->ghoul2;
	VectorCopy( s1->modelScale, ent->modelScale);
	ent->radius = s1->radius;
	VectorCopy( s1->angles, ent->angles);
}

/*
Ghoul2 Insert End
*/

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;
	int				handle;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		trap->Error( ERR_DROP, "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	if (item->giType == IT_TEAM &&
		(item->giTag == PW_REDFLAG || item->giTag == PW_BLUEFLAG) &&
		cgs.gametype == GT_CTY)
	{ //in CTY the flag model is different
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	}
	else if (item->giType == IT_WEAPON &&
		(item->giTag == WP_THERMAL || item->giTag == WP_TRIP_MINE || item->giTag == WP_DET_PACK))
	{
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[1] );
	}
	else
	{
		itemInfo->models[0] = trap->R_RegisterModel( item->world_model[0] );
	}
/*
Ghoul2 Insert Start
*/
	if (!Q_stricmp(&item->world_model[0][strlen(item->world_model[0]) - 4], ".glm"))
	{
		handle = trap->G2API_InitGhoul2Model(&itemInfo->g2Models[0], item->world_model[0], 0 , 0, 0, 0, 0);
		if (handle<0)
		{
			itemInfo->g2Models[0] = NULL;
		}
		else
		{
			itemInfo->radius[0] = 60;
		}
	}
/*
Ghoul2 Insert End
*/
	if (item->icon)
	{
		if (item->giType == IT_HEALTH)
		{ //medpack gets nomip'd by the ui or something I guess.
			itemInfo->icon = trap->R_RegisterShaderNoMip( item->icon );
		}
		else
		{
			itemInfo->icon = trap->R_RegisterShader( item->icon );
		}
	}
	else
	{
		itemInfo->icon = 0;
	}

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH ||
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap->R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

#define WEAPON_FORCE_BUSY_HOLSTER

#ifdef WEAPON_FORCE_BUSY_HOLSTER
//rww - this was done as a last resort. Forgive me.
static int cgWeapFrame = 0;
static int cgWeapFrameTime = 0;
#endif

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame, int animNum ) {
	animation_t *animations = bgHumanoidAnimations;
#ifdef WEAPON_FORCE_BUSY_HOLSTER
	if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE || cgWeapFrameTime > cg.time ) {
		// the reason for the after delay is so that it doesn't snap the weapon frame to the "idle" (0) frame for a very quick moment
		if ( cgWeapFrame < 6 ) {
			cgWeapFrame = 6;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cgWeapFrameTime < cg.time && cgWeapFrame < 10 ) {
			cgWeapFrame++;
			cgWeapFrameTime = cg.time + 10;
		}

		else if ( cg.snap->ps.forceHandExtend != HANDEXTEND_NONE && cgWeapFrame == 10 )
			cgWeapFrameTime = cg.time + 100;

		return cgWeapFrame;
	}
	else {
		cgWeapFrame = 0;
		cgWeapFrameTime = 0;
	}
#endif

	switch( animNum )
	{
	case TORSO_DROPWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5 )
		{
			return frame - animations[animNum].firstFrame + 6;
		}
		break;

	case TORSO_RAISEWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4 )
		{
			return frame - animations[animNum].firstFrame + 6 + 4;
		}
		break;
	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
	case BOTH_ATTACK10:
	case BOTH_THERMAL_THROW:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6 )
		{
			return 1 + ( frame - animations[animNum].firstFrame );
		}

		break;
	}
	return -1;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdef.viewangles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	if ( cg_weaponBob.value ) {
		// gun angles from bobbing
		angles[ROLL] += scale * cg.bobfracsin * 0.005;
		angles[YAW] += scale * cg.bobfracsin * 0.01;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;
	}

	if ( cg_fallingBob.value ) {
		// drop the weapon when landing
		delta = cg.time - cg.landTime;
		if ( delta < LAND_DEFLECT_TIME ) {
			origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
		} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
			origin[2] += cg.landChange*0.25 *
				(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
		}
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	if ( cg_weaponBob.value ) {
		// idle drift
		scale = cg.xyspeed + 40;
		fracsin = sin( cg.time * 0.001 );
		angles[ROLL] += scale * fracsin * 0.01;
		angles[YAW] += scale * fracsin * 0.01;
		angles[PITCH] += scale * fracsin * 0.01;
	}
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
//	trace_t  trace;
	refEntity_t  beam;
//	vec3_t   forward;
//	vec3_t   muzzlePoint, endPoint;

	//Must be a durational weapon that continuously generates an effect.
	if ( cent->currentState.weapon == WP_DEMP2 && cent->currentState.eFlags & EF_ALT_FIRING )
	{ /*nothing*/ }
	else
	{
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

	// NOTENOTE No lightning gun-ish stuff yet.
/*
	// CPMA  "true" lightning
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)) {
		vec3_t angle;
		int i;

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdef.viewangles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdef.viewangles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
//		VectorCopy(cg.refdef.vieworg, muzzlePoint );
	} else {
		// !CPMA
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
	}

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	VectorMA( muzzlePoint, LIGHTNING_RANGE, forward, endPoint );

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap->R_AddRefEntityToScene( &beam );
*/

	// NOTENOTE No lightning gun-ish stuff yet.
/*
	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap->R_AddRefEntityToScene( &beam );
	}
*/
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	trap->R_AddRefEntityToScene( gun );

	if (cg.predictedPlayerState.electrifyTime > cg.time)
	{ //add electrocution shell
		int preShader = gun->customShader;
		if ( rand() & 1 )
		{
			gun->customShader = cgs.media.electricBodyShader;
		}
		else
		{
			gun->customShader = cgs.media.electricBody2Shader;
		}
		trap->R_AddRefEntityToScene( gun );
		gun->customShader = preShader; //set back just to be safe
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, vec3_t newAngles, qboolean thirdPerson ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	refEntity_t	flash;

	weaponNum = cent->currentState.weapon;

	if (cent->currentState.weapon == WP_EMPLACED_GUN)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
		cent->currentState.number == cg.predictedPlayerState.clientNum)
	{ //spectator mode, don't draw it...
		return;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];
/*
Ghoul2 Insert Start
*/

	memset( &gun, 0, sizeof( gun ) );

	// only do this if we are in first person, since world weapons are now handled on the server by Ghoul2
	if (!thirdPerson)
	{

		// add the weapon
		VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
		gun.shadowPlane = parent->shadowPlane;
		gun.renderfx = parent->renderfx;

		if (ps)
		{	// this player, in first person view
			gun.hModel = weapon->viewModel;
		}
		else
		{
			gun.hModel = weapon->weaponModel;
		}
		if (!gun.hModel) {
			return;
		}

		if ( !ps ) {
			// add weapon ready sound
			cent->pe.lightningFiring = qfalse;
			if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
				// lightning gun and gauntlet make a different sound when fire is held down
				trap->S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
				cent->pe.lightningFiring = qtrue;
			} else if ( weapon->readySound ) {
				trap->S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
			}
		}

		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon");

		if (!CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups ); //don't draw the weapon if the player is invisible
			/*
			if ( weaponNum == WP_STUN_BATON )
			{
				gun.shaderRGBA[0] = gun.shaderRGBA[1] = gun.shaderRGBA[2] = 25;

				gun.customShader = trap->R_RegisterShader( "gfx/effects/stunPass" );
				gun.renderfx = RF_RGB_TINT | RF_FIRST_PERSON | RF_DEPTHHACK;
				trap->R_AddRefEntityToScene( &gun );
			}
			*/
		}

		if (weaponNum == WP_STUN_BATON)
		{
			int i = 0;

			while (i < 3)
			{
				memset( &barrel, 0, sizeof( barrel ) );
				VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				if (i == 0)
				{
					barrel.hModel = trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel.md3");
				}
				else if (i == 1)
				{
					barrel.hModel = trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel2.md3");
				}
				else
				{
					barrel.hModel = trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel3.md3");
				}
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis( angles, barrel.axis );

				if (i == 0)
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );
				}
				else if (i == 1)
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel2" );
				}
				else
				{
					CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel3" );
				}
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );

				i++;
			}
		}
		else
		{
			// add the spinning barrel
			if ( weapon->barrelModel ) {
				memset( &barrel, 0, sizeof( barrel ) );
				VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
				barrel.shadowPlane = parent->shadowPlane;
				barrel.renderfx = parent->renderfx;

				barrel.hModel = weapon->barrelModel;
				angles[YAW] = 0;
				angles[PITCH] = 0;
				angles[ROLL] = 0;

				AnglesToAxis( angles, barrel.axis );

				CG_PositionRotatedEntityOnTag( &barrel, parent/*&gun*/, /*weapon->weaponModel*/weapon->handsModel, "tag_barrel" );

				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
			}
		}
	}
/*
Ghoul2 Insert End
*/

	memset (&flash, 0, sizeof(flash));
	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

	VectorCopy(flash.origin, cg.lastFPFlashPoint);

	// Do special charge bits
	//-----------------------
	if ( (ps || cg.renderingThirdPerson || cg.predictedPlayerState.clientNum != cent->currentState.number) &&
		( ( cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_PISTOL ) ||
		  ( cent->currentState.modelindex2 == WEAPON_CHARGING_ALT && cent->currentState.weapon == WP_BRYAR_OLD ) ||
		  ( cent->currentState.weapon == WP_BOWCASTER && cent->currentState.modelindex2 == WEAPON_CHARGING ) ||
		  ( cent->currentState.weapon == WP_DEMP2 && cent->currentState.modelindex2 == WEAPON_CHARGING_ALT) ) )
	{
		int		shader = 0;
		float	val = 0.0f;
		float	scale = 1.0f;
		addspriteArgStruct_t fxSArgs;
		vec3_t flashorigin, flashdir;

		if (!thirdPerson)
		{
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		if ( cent->currentState.weapon == WP_BRYAR_PISTOL ||
			cent->currentState.weapon == WP_BRYAR_OLD)
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
			shader = cgs.media.bryarFrontFlash;
		}
		else if ( cent->currentState.weapon == WP_BOWCASTER )
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
			shader = cgs.media.greenFrontFlash;
		}
		else if ( cent->currentState.weapon == WP_DEMP2 )
		{
			val = ( cg.time - cent->currentState.constantLight ) * 0.001f;
			shader = cgs.media.lightningFlash;
			scale = 1.75f;
		}

		if ( val < 0.0f )
		{
			val = 0.0f;
		}
		else if ( val > 1.0f )
		{
			val = 1.0f;
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( /*0.1f*/0.2f, 100 );
			}
		}
		else
		{
			if (ps && cent->currentState.number == ps->clientNum)
			{
				CGCam_Shake( val * val * /*0.3f*/0.6f, 100 );
			}
		}

		val += Q_flrand(0.0f, 1.0f) * 0.5f;

		VectorCopy(flashorigin, fxSArgs.origin);
		VectorClear(fxSArgs.vel);
		VectorClear(fxSArgs.accel);
		fxSArgs.scale = 3.0f*val*scale;
		fxSArgs.dscale = 0.0f;
		fxSArgs.sAlpha = 0.7f;
		fxSArgs.eAlpha = 0.7f;
		fxSArgs.rotation = Q_flrand(0.0f, 1.0f)*360;
		fxSArgs.bounce = 0.0f;
		fxSArgs.life = 1.0f;
		fxSArgs.shader = shader;
		fxSArgs.flags = 0x08000000;

		//FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val, 0.0f, 0.7f, 0.7f, WHITE, WHITE, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
		trap->FX_AddSprite(&fxSArgs);
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( ( weaponNum == WP_DEMP2)
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) )
	{
		// continuous flash
	} else {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME) {
			return;
		}
	}

	if ( ps || cg.renderingThirdPerson ||
			cent->currentState.number != cg.predictedPlayerState.clientNum )
	{	// Make sure we don't do the thirdperson model effects for the local player if we're in first person
		vec3_t flashorigin, flashdir;
		refEntity_t	flash;

		memset (&flash, 0, sizeof(flash));

		if (!thirdPerson)
		{
			CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");
			VectorCopy(flash.origin, flashorigin);
			VectorCopy(flash.axis[0], flashdir);
		}
		else
		{
			mdxaBone_t 		boltMatrix;

			if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
			{ //it's quite possible that we may have have no weapon model and be in a valid state, so return here if this is the case
				return;
			}

			// go away and get me the bolt position for this frame please
 			if (!(trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, newAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale)))
			{	// Couldn't find bolt point.
				return;
			}

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, flashorigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, flashdir);
		}

		if ( cg.time - cent->muzzleFlashTime <= MUZZLE_FLASH_TIME + 10 )
		{	// Handle muzzle flashes
			if ( cent->currentState.eFlags & EF_ALT_FIRING )
			{	// Check the alt firing first.
				if (weapon->altMuzzleEffect)
				{
					if (!thirdPerson)
					{
						trap->FX_PlayEntityEffectID(weapon->altMuzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap->FX_PlayEffectID(weapon->altMuzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
					}
				}
			}
			else
			{	// Regular firing
				if (weapon->muzzleEffect)
				{
					if (!thirdPerson)
					{
						trap->FX_PlayEntityEffectID(weapon->muzzleEffect, flashorigin, flash.axis, -1, -1, -1, -1  );
					}
					else
					{
						trap->FX_PlayEffectID(weapon->muzzleEffect, flashorigin, flashdir, -1, -1, qfalse);
					}
				}
			}
		}

		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flashorigin );

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap->R_AddLightToScene( flashorigin, 300 + (rand()&31), weapon->flashDlightColor[0],
				weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;
	float cgFov = cg_fovViewmodel.integer ? cg_fovViewmodel.value : cg_fov.value;

	if (cgFov < 1)
		cgFov = 1;
	if (cgFov > 130)
		cgFov = 130;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer || cg.predictedPlayerState.zoomMode) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fovViewmodelAdjust.integer && cgFov > 90 )
		fovOffset = -0.2f * ( cgFov - 90 );
	else
		fovOffset = 0;

	cent = &cg_entities[cg.predictedPlayerState.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gunX.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gunY.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gunZ.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	if ( cg_fovViewmodel.integer )
	{
		float fracDistFOV = tanf( cg.refdef.fov_x * ( M_PI/180 ) * 0.5f );
		float fracWeapFOV = ( 1.0f / fracDistFOV ) * tanf( cgFov * ( M_PI/180 ) * 0.5f );
		VectorScale( hand.axis[0], fracWeapFOV, hand.axis[0] );
	}

	// map torso animations to weapon animations
	if ( cg_debugGun.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_debugGun.integer;
		hand.backlerp = 0;
	} else {
		float currentFrame;
		// get clientinfo for animation map
		if (cent->currentState.eType == ET_NPC)
		{
			if (!cent->npcClient)
			{
				return;
			}

			ci = cent->npcClient;
		}
		else
		{
			ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		}

		trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.gameModels, 0);
		hand.frame = CG_MapTorsoToWeaponFrame( ci, ceil( currentFrame ), ps->torsoAnim );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, floor( currentFrame ), ps->torsoAnim );
		hand.backlerp = 1.0f - (currentFrame-floor(currentFrame));

		// Handle the fringe situation where oldframe is invalid
		if ( hand.frame == -1 )
		{
			hand.frame = 0;
			hand.oldframe = 0;
			hand.backlerp = 0;
		}
		else if ( hand.oldframe == -1 )
		{
			hand.oldframe = hand.frame;
			hand.backlerp = 0;
		}
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;// | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg_entities[cg.predictedPlayerState.clientNum], ps->persistant[PERS_TEAM], angles, qfalse );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/
#define ICON_WEAPONS	0
#define ICON_FORCE		1
#define ICON_INVENTORY	2


void CG_DrawIconBackground(void)
{
	int				/*height, xAdd, x2, y2,*/ t;
//	int				prongLeftX,prongRightX;
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			fpTime = cg.forceSelectTime+WEAPON_SELECT_TIME;
//	int				drawType = cgs.media.weaponIconBackground;
//	int				yOffset = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if (cg_hudFiles.integer)
	{ //simple hud
		return;
	}

//	x2 = 30;
//	y2 = SCREEN_HEIGHT-70;

	//prongLeftX =x2+37;
	//prongRightX =x2+544;

	if (inTime > wpTime)
	{
//		drawType = cgs.media.inventoryIconBackground;
		cg.iconSelectTime = cg.invenSelectTime;
	}
	else
	{
//		drawType = cgs.media.weaponIconBackground;
		cg.iconSelectTime = cg.weaponSelectTime;
	}

	if (fpTime > inTime && fpTime > wpTime)
	{
//		drawType = cgs.media.forceIconBackground;
		cg.iconSelectTime = cg.forceSelectTime;
	}

	if ((cg.iconSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		if (cg.iconHUDActive)		// The time is up, but we still need to move the prongs back to their original position
		{
			t =  cg.time - (cg.iconSelectTime+WEAPON_SELECT_TIME);
			cg.iconHUDPercent = t/ 130.0f;
			cg.iconHUDPercent = 1 - cg.iconHUDPercent;

			if (cg.iconHUDPercent<0)
			{
				cg.iconHUDActive = qfalse;
				cg.iconHUDPercent=0;
			}

		//	xAdd = (int) 8*cg.iconHUDPercent;

		//	height = (int) (60.0f*cg.iconHUDPercent);
			//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
			//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

		}
		else
		{
		//	xAdd = 0;
		}

		return;
	}
	//prongLeftX =x2+37;
	//prongRightX =x2+544;

	if (!cg.iconHUDActive)
	{
		t = cg.time - cg.iconSelectTime;
		cg.iconHUDPercent = t/ 130.0f;

		// Calc how far into opening sequence we are
		if (cg.iconHUDPercent>1)
		{
			cg.iconHUDActive = qtrue;
			cg.iconHUDPercent=1;
		}
		else if (cg.iconHUDPercent<0)
		{
			cg.iconHUDPercent=0;
		}
	}
	else
	{
		cg.iconHUDPercent=1;
	}

	//trap->R_SetColor( colorTable[CT_WHITE] );
	//height = (int) (60.0f*cg.iconHUDPercent);
	//CG_DrawPic( x2+60, y2+30+yOffset, 460, -height, drawType);	// Top half
	//CG_DrawPic( x2+60, y2+30-2+yOffset, 460, height, drawType);	// Bottom half

	// And now for the prongs
/*	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_INVENTORY;
		background = &cgs.media.inventoryProngsOn;
	}
	else if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_WEAPONS;
	}
	else
	{
		cgs.media.currentBackground = ICON_FORCE;
		background = &cgs.media.forceProngsOn;
	}
*/
	// Side Prongs
//	trap->R_SetColor( colorTable[CT_WHITE]);
//	xAdd = (int) 8*cg.iconHUDPercent;
//	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, background);
//	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, background);

}

qboolean CG_WeaponCheck(int weap)
{
	if (cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].energyPerShot &&
		cg.snap->ps.ammo[weaponData[weap].ammoIndex] < weaponData[weap].altEnergyPerShot)
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
	/*if ( !cg.snap->ps.ammo[weaponData[i].ammoIndex] ) {
		return qfalse;
	}*/
	if (!i)
	{
		return qfalse;
	}

	if (cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot &&
		cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < weaponData[i].altEnergyPerShot)
	{
		return qfalse;
	}

	if (i == WP_DET_PACK && cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] < 1 &&
		!cg.predictedPlayerState.hasDetPackPlanted)
	{
		return qfalse;
	}

	if ( ! (cg.predictedPlayerState.stats[ STAT_WEAPONS ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int				i;
	int				bits;
	int				count;
	int				smallIconSize,bigIconSize;
	int				holdX,x,y,pad;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				sideMax,holdCount,iconCnt;
//	int				height;
	int		yOffset = 0;
	qboolean drewConc = qfalse;

	if (cg.predictedPlayerState.emplacedIndex)
	{ //can't cycle when on a weapon
		cg.weaponSelectTime = 0;
	}

	if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	bits = cg.predictedPlayerState.stats[ STAT_WEAPONS ];

	// count the number of weapons owned
	count = 0;

	if ( !CG_WeaponSelectable(cg.weaponSelect) &&
		(cg.weaponSelect == WP_THERMAL || cg.weaponSelect == WP_TRIP_MINE) )
	{ //display this weapon that we don't actually "have" as unhighlighted until it's deselected
	  //since it's selected we must increase the count to display the proper number of valid selectable weapons
		count++;
	}

	for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ )
	{
		if ( bits & ( 1 << i ) )
		{
			if ( CG_WeaponSelectable(i) ||
				(i != WP_THERMAL && i != WP_TRIP_MINE) )
			{
				count++;
			}
		}
	}

	if (count == 0)	// If no weapons, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	if ( cg.weaponSelect == WP_CONCUSSION )
	{
		i = WP_FLECHETTE;
	}
	else
	{
		i = cg.weaponSelect - 1;
	}
	if (i<1)
	{
		i = LAST_USEABLE_WEAPON;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 12;

	x = 320;
	y = 410;

	// Background
//	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
//	calcColor[3] = .35f;
//	trap->R_SetColor( calcColor);

	// Left side ICONS
	trap->R_SetColor(colorTable[CT_WHITE]);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
//	height = smallIconSize * 1;//cg.iconHUDPercent;
	drewConc = qfalse;

	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if ( i == WP_CONCUSSION )
		{
			i--;
		}
		else if ( i == WP_FLECHETTE && !drewConc && cg.weaponSelect != WP_CONCUSSION )
		{
			i = WP_CONCUSSION;
		}
		if (i<1)
		{
			//i = 13;
			//...don't ever do this.
			i = LAST_USEABLE_WEAPON;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			if ( i == WP_CONCUSSION )
			{
				drewConc = qtrue;
				i = WP_ROCKET_LAUNCHER;
			}
			continue;
		}

		if ( !CG_WeaponSelectable(i) &&
			(i == WP_THERMAL || i == WP_TRIP_MINE) )
		{ //Don't show thermal and tripmine when out of them
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.weaponIcons[i])
		{
		//	weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
		//	weaponInfo = &cg_weapons[i];

			trap->R_SetColor(colorTable[CT_WHITE]);
			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, /*weaponInfo->weaponIconNoAmmo*/cgs.media.weaponIcons_NA[i] );
			}
			else
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, /*weaponInfo->weaponIcon*/cgs.media.weaponIcons[i] );
			}

			holdX -= (smallIconSize+pad);
		}
		if ( i == WP_CONCUSSION )
		{
			drewConc = qtrue;
			i = WP_ROCKET_LAUNCHER;
		}
	}

	// Current Center Icon
//	height = bigIconSize * cg.iconHUDPercent;
	if (cgs.media.weaponIcons[cg.weaponSelect])
	{
	//	weaponInfo_t	*weaponInfo;
		CG_RegisterWeapon( cg.weaponSelect );
	//	weaponInfo = &cg_weapons[cg.weaponSelect];

		trap->R_SetColor( colorTable[CT_WHITE]);
		if (!CG_WeaponCheck(cg.weaponSelect))
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons_NA[cg.weaponSelect] );
		}
		else
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10+yOffset, bigIconSize, bigIconSize, cgs.media.weaponIcons[cg.weaponSelect] );
		}
	}

	if ( cg.weaponSelect == WP_CONCUSSION )
	{
		i = WP_ROCKET_LAUNCHER;
	}
	else
	{
		i = cg.weaponSelect + 1;
	}
	if (i> LAST_USEABLE_WEAPON)
	{
		i = 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
//	height = smallIconSize * cg.iconHUDPercent;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if ( i == WP_CONCUSSION )
		{
			i++;
		}
		else if ( i == WP_ROCKET_LAUNCHER && !drewConc && cg.weaponSelect != WP_CONCUSSION )
		{
			i = WP_CONCUSSION;
		}
		if (i>LAST_USEABLE_WEAPON)
		{
			i = 1;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			if ( i == WP_CONCUSSION )
			{
				drewConc = qtrue;
				i = WP_FLECHETTE;
			}
			continue;
		}

		if ( !CG_WeaponSelectable(i) &&
			(i == WP_THERMAL || i == WP_TRIP_MINE) )
		{ //Don't show thermal and tripmine when out of them
			continue;
		}

		++iconCnt;					// Good icon

		if (/*weaponData[i].weaponIcon[0]*/cgs.media.weaponIcons[i])
		{
		//	weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
		//	weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?
			trap->R_SetColor( colorTable[CT_WHITE]);
			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons_NA[i] );
			}
			else
			{
				CG_DrawPic( holdX, y+10+yOffset, smallIconSize, smallIconSize, cgs.media.weaponIcons[i] );
			}


			holdX += (smallIconSize+pad);
		}
		if ( i == WP_CONCUSSION )
		{
			drewConc = qtrue;
			i = WP_FLECHETTE;
		}
	}

	// draw the selected name
	if ( cg_weapons[ cg.weaponSelect ].item )
	{
		vec4_t			textColor = { .875f, .718f, .121f, 1.0f };
		char	text[1024];
		char	upperKey[1024];

		strcpy(upperKey, cg_weapons[ cg.weaponSelect ].item->classname);

		if ( trap->SE_GetStringTextString( va("SP_INGAME_%s",Q_strupr(upperKey)), text, sizeof( text )))
		{
			CG_DrawProportionalString(320, y+45+yOffset, text, UI_CENTER|UI_SMALLFONT, textColor);
		}
		else
		{
			CG_DrawProportionalString(320, y+45+yOffset, cg_weapons[ cg.weaponSelect ].item->classname, UI_CENTER|UI_SMALLFONT, textColor);
		}
	}

	trap->R_SetColor( NULL );
}


/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_FLECHETTE )
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if ( cg.weaponSelect == WP_CONCUSSION )
		{
			cg.weaponSelect = WP_ROCKET_LAUNCHER;
		}
		else if ( cg.weaponSelect == WP_DET_PACK )
		{
			cg.weaponSelect = WP_BRYAR_OLD;
		}
		else
		{
			cg.weaponSelect++;
		}
		if ( cg.weaponSelect == WP_NUM_WEAPONS ) {
			cg.weaponSelect = 0;
		}
	//	if ( cg.weaponSelect == WP_STUN_BATON ) {
	//		continue;		// never cycle to gauntlet
	//	}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < WP_NUM_WEAPONS ; i++ ) {
		//*SIGH*... Hack to put concussion rifle before rocketlauncher
		if ( cg.weaponSelect == WP_ROCKET_LAUNCHER )
		{
			cg.weaponSelect = WP_CONCUSSION;
		}
		else if ( cg.weaponSelect == WP_CONCUSSION )
		{
			cg.weaponSelect = WP_FLECHETTE;
		}
		else if ( cg.weaponSelect == WP_BRYAR_OLD )
		{
			cg.weaponSelect = WP_DET_PACK;
		}
		else
		{
			cg.weaponSelect--;
		}
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = WP_NUM_WEAPONS-1;
		}
	//	if ( cg.weaponSelect == WP_STUN_BATON ) {
	//		continue;		// never cycle to gauntlet
	//	}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == WP_NUM_WEAPONS ) {
		cg.weaponSelect = original;
	}
	else
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > LAST_USEABLE_WEAPON ) {
		return;
	}

	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		if (cg.predictedPlayerState.weaponTime < 1)
	//	if (cg.snap->ps.weaponTime < 1)
		{
			trap->SendConsoleCommand("sv_saberswitch\n");
		}
		return;
	}

	//rww - hack to make weapon numbers same as single player
	if (num > WP_STUN_BATON)
	{
		//num++;
		num += 2; //I suppose this is getting kind of crazy, what with the wp_melee in there too now.
	}
	else
	{
		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > LAST_USEABLE_WEAPON+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL &&
			cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}

	cg.weaponSelect = num;
}


//Version of the above which doesn't add +2 to a weapon.  The above can't
//triger WP_MELEE or WP_STUN_BATON.  Derogatory comments go here.
void CG_WeaponClean_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if (cg.snap->ps.emplacedIndex)
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > LAST_USEABLE_WEAPON ) {
		return;
	}

	if (num == 1 && cg.snap->ps.weapon == WP_SABER)
	{
		if (cg.snap->ps.weaponTime < 1)
		{
			trap->SendConsoleCommand("sv_saberswitch\n");
		}
		return;
	}

	if(num == WP_STUN_BATON) {
		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
		{
			num = WP_SABER;
		}
		else
		{
			num = WP_MELEE;
		}
	}

	if (num > LAST_USEABLE_WEAPON+1)
	{ //other weapons are off limits due to not actually being weapon weapons
		return;
	}

	if (num >= WP_THERMAL && num <= WP_DET_PACK)
	{
		int weap, i = 0;

		if (cg.snap->ps.weapon >= WP_THERMAL &&
			cg.snap->ps.weapon <= WP_DET_PACK)
		{
			// already in cycle range so start with next cycle item
			weap = cg.snap->ps.weapon + 1;
		}
		else
		{
			// not in cycle range, so start with thermal detonator
			weap = WP_THERMAL;
		}

		// prevent an endless loop
		while ( i <= 4 )
		{
			if (weap > WP_DET_PACK)
			{
				weap = WP_THERMAL;
			}

			if (CG_WeaponSelectable(weap))
			{
				num = weap;
				break;
			}

			weap++;
			i++;
		}
	}

	if (!CG_WeaponSelectable(num))
	{
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
	{
		if (num == WP_SABER)
		{ //don't have saber, try melee on the same slot
			num = WP_MELEE;

			if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
			{
				return;
			}
		}
		else
		{
			return;		// don't have the weapon
		}
	}

	if (cg.weaponSelect != num)
	{
		trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
	}

	cg.weaponSelect = num;
}



/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( int oldWeapon )
{
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = LAST_USEABLE_WEAPON ; i > 0 ; i-- )	//We don't want the emplaced or turret
	{
		if ( CG_WeaponSelectable( i ) )
		{
			/*
			if ( 1 == cg_autoswitch.integer &&
				( i == WP_TRIP_MINE || i == WP_DET_PACK || i == WP_THERMAL || i == WP_ROCKET_LAUNCHER) ) // safe weapon switch
			*/
			//rww - Don't we want to make sure i != one of these if autoswitch is 1 (safe)?
			if (cg_autoSwitch.integer != 1 || (i != WP_TRIP_MINE && i != WP_DET_PACK && i != WP_THERMAL && i != WP_ROCKET_LAUNCHER))
			{
				if (i != oldWeapon)
				{ //don't even do anything if we're just selecting the weapon we already have/had
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	trap->S_MuteSound(cg.snap->ps.clientNum, CHAN_WEAPON);
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vec3_t to)
{
	centity_t *cent;
	mdxaBone_t	boltMatrix;

	if (clIndex < 0 || clIndex >= MAX_CLIENTS)
	{
		return;
	}

	cent = &cg_entities[clIndex];

	if (!cent || !cent->ghoul2 || !trap->G2_HaveWeGhoul2Models(cent->ghoul2) ||
		!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1))
	{
		return;
	}

	trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, to);
}

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean altFire ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		trap->Error( ERR_DROP, "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	if (cg.predictedPlayerState.clientNum == cent->currentState.number)
	{
		if ((ent->weapon == WP_BRYAR_PISTOL && altFire) ||
			(ent->weapon == WP_BRYAR_OLD && altFire) ||
			(ent->weapon == WP_BOWCASTER && !altFire) ||
			(ent->weapon == WP_DEMP2 && altFire))
		{
			float val = ( cg.time - cent->currentState.constantLight ) * 0.001f;

			if (val > 3.0f)
			{
				val = 3.0f;
			}
			if (val < 0.2f)
			{
				val = 0.2f;
			}

			val *= 2;

			CGCam_Shake( val, 250 );
		}
		else if (ent->weapon == WP_ROCKET_LAUNCHER ||
			(ent->weapon == WP_REPEATER && altFire) ||
			ent->weapon == WP_FLECHETTE ||
			(ent->weapon == WP_CONCUSSION && !altFire))
		{
			if (ent->weapon == WP_CONCUSSION)
			{
				if (!cg.renderingThirdPerson )//gives an advantage to being in 3rd person, but would look silly otherwise
				{//kick the view back
					cg.kick_angles[PITCH] = flrand( -10, -15 );
					cg.kick_time = cg.time;
				}
			}
			else if (ent->weapon == WP_ROCKET_LAUNCHER)
			{
				CGCam_Shake(flrand(2, 3), 350);
			}
			else if (ent->weapon == WP_REPEATER)
			{
				CGCam_Shake(flrand(2, 3), 350);
			}
			else if (ent->weapon == WP_FLECHETTE)
			{
				if (altFire)
				{
					CGCam_Shake(flrand(2, 3), 350);
				}
				else
				{
					CGCam_Shake(1.5, 250);
				}
			}
		}
	}
	// lightning gun only does this this on initial press
	if ( ent->weapon == WP_DEMP2 ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	}

	#ifdef BASE_COMPAT
		// play quad sound if needed
		if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
			//trap->S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
		}
	#endif // BASE_COMPAT


	// play a sound
	if (altFire)
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->altFlashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->altFlashSound[c] )
			{
				trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSound[c] );
			}
		}
//		if ( weap->altFlashSnd )
//		{
//			trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->altFlashSnd );
//		}
	}
	else
	{
		// play a sound
		for ( c = 0 ; c < 4 ; c++ ) {
			if ( !weap->flashSound[c] ) {
				break;
			}
		}
		if ( c > 0 ) {
			c = rand() % c;
			if ( weap->flashSound[c] )
			{
				trap->S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
			}
		}
	}
}

qboolean CG_VehicleWeaponImpact( centity_t *cent )
{//see if this is a missile entity that's owned by a vehicle and should do a special, overridden impact effect
	if ((cent->currentState.eFlags&EF_JETPACK_ACTIVE)//hack so we know we're a vehicle Weapon shot
		&& cent->currentState.otherEntityNum2
		&& g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX)
	{//missile is from a special vehWeapon
		vec3_t normal;
		ByteToDir( cent->currentState.eventParm, normal );

		trap->FX_PlayEffectID( g_vehWeaponInfo[cent->currentState.otherEntityNum2].iImpactFX, cent->lerpOrigin, normal, -1, -1, qfalse );
		return qtrue;
	}
	return qfalse;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge)
{
	int parm;
	vec3_t up={0,0,1};

	switch( weapon )
	{
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitWall( origin, dir );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			parm = charge;
			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_TURRET:
		FX_TurretHitWall( origin, dir );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltMiss( origin, dir );
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitWall( origin, dir );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitWall( origin, dir );
		}
		else
		{
			FX_RepeaterHitWall( origin, dir );
		}
		break;

	case WP_DEMP2:
		if (altFire)
		{
			trap->FX_PlayEffectID(cgs.effects.mAltDetonate, origin, dir, -1, -1, qfalse);
		}
		else
		{
			FX_DEMP2_HitWall( origin, dir );
		}
		break;

	case WP_FLECHETTE:
		/*if (altFire)
		{
			CG_SurfaceExplosion(origin, dir, 20.0f, 12.0f, qtrue);
		}
		else
		*/
		if (!altFire)
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitWall( origin, dir );
		break;

	case WP_THERMAL:
		trap->FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1, qfalse );
		trap->FX_PlayEffectID( cgs.effects.thermalShockwaveEffect, origin, up, -1, -1, qfalse );
		break;

	case WP_EMPLACED_GUN:
		FX_BlasterWeaponHitWall( origin, dir );
		//FIXME: Give it its own hit wall effect
		break;
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire)
{
	qboolean	humanoid = qtrue;
	vec3_t up={0,0,1};

	/*
	// NOTENOTE Non-portable code from single player
	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];

		if ( other->client && other->client->playerTeam == TEAM_BOTS )
		{
			humanoid = qfalse;
		}
	}
	*/

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_CONCUSSION:
		FX_ConcussionHitPlayer( origin, dir, humanoid );
		break;

	case WP_BRYAR_OLD:
		if ( altFire )
		{
			FX_BryarAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_BryarHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_TURRET:
		FX_TurretHitPlayer( origin, dir, humanoid );
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_DISRUPTOR:
		FX_DisruptorAltHit( origin, dir);
		break;

	case WP_BOWCASTER:
		FX_BowcasterHitPlayer( origin, dir, humanoid );
		break;

	case WP_REPEATER:
		if ( altFire )
		{
			FX_RepeaterAltHitPlayer( origin, dir, humanoid );
		}
		else
		{
			FX_RepeaterHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_DEMP2:
		// Do a full body effect here for some more feedback
		// NOTENOTE The chaining of the demp2 is not yet implemented.
		/*
		if ( other )
		{
			other->s.powerups |= ( 1 << PW_DISINT_1 );
			other->client->ps.powerups[PW_DISINT_1] = cg.time + 650;
		}
		*/
		if (altFire)
		{
			trap->FX_PlayEffectID(cgs.effects.mAltDetonate, origin, dir, -1, -1, qfalse);
		}
		else
		{
			FX_DEMP2_HitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_FLECHETTE:
		FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitPlayer( origin, dir, humanoid );
		break;

	case WP_THERMAL:
		trap->FX_PlayEffectID( cgs.effects.thermalExplosionEffect, origin, dir, -1, -1, qfalse );
		trap->FX_PlayEffectID( cgs.effects.thermalShockwaveEffect, origin, up, -1, -1, qfalse );
		break;
	case WP_EMPLACED_GUN:
		//FIXME: Its own effect?
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
		break;

	default:
		break;
	}
}


/*
============================================================================

BULLETS

============================================================================
*/


/*
======================
CG_CalcMuzzlePoint
======================
*/
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward, right;
	vec3_t		gunpoint;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum )
	{ //I'm not exactly sure why we'd be rendering someone else's crosshair, but hey.
		int weapontype = cg.snap->ps.weapon;
		vec3_t weaponMuzzle;
		centity_t *pEnt = &cg_entities[cg.predictedPlayerState.clientNum];

		VectorCopy(WP_MuzzlePoint[weapontype], weaponMuzzle);

		if (weapontype == WP_DISRUPTOR || weapontype == WP_STUN_BATON || weapontype == WP_MELEE || weapontype == WP_SABER)
		{
			VectorClear(weaponMuzzle);
		}

		if (cg.renderingThirdPerson)
		{
			VectorCopy( pEnt->lerpOrigin, gunpoint );
			AngleVectors( pEnt->lerpAngles, forward, right, NULL );
		}
		else
		{
			VectorCopy( cg.refdef.vieworg, gunpoint );
			AngleVectors( cg.refdef.viewangles, forward, right, NULL );
		}

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			centity_t *gunEnt = &cg_entities[cg.snap->ps.emplacedIndex];

			if (gunEnt)
			{
				vec3_t pitchConstraint;

				VectorCopy(gunEnt->lerpOrigin, gunpoint);
				gunpoint[2] += 46;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(pEnt->lerpAngles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdef.viewangles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}
				AngleVectors( pitchConstraint, forward, right, NULL );
			}
		}

		VectorCopy(gunpoint, muzzle);

		VectorMA(muzzle, weaponMuzzle[0], forward, muzzle);
		VectorMA(muzzle, weaponMuzzle[1], right, muzzle);

		if (weapontype == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
		{
			//Do nothing
		}
		else if (cg.renderingThirdPerson)
		{
			muzzle[2] += cg.snap->ps.viewheight + weaponMuzzle[2];
		}
		else
		{
			muzzle[2] += weaponMuzzle[2];
		}

		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim;
	if ( anim == BOTH_CROUCH1WALK || anim == BOTH_CROUCH1IDLE ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}



/*
Ghoul2 Insert Start
*/

// create one instance of all the weapons we are going to use so we can just copy this info into each clients gun ghoul2 object in fast way
static void *g2WeaponInstances[MAX_WEAPONS];

void CG_InitG2Weapons(void)
{
	int i = 0;
	gitem_t		*item;
	memset(g2WeaponInstances, 0, sizeof(g2WeaponInstances));
	for ( item = bg_itemlist + 1 ; item->classname ; item++ )
	{
		if ( item->giType == IT_WEAPON )
		{
			assert(item->giTag < MAX_WEAPONS);

			// initialise model
			trap->G2API_InitGhoul2Model(&g2WeaponInstances[/*i*/item->giTag], item->world_model[0], 0, 0, 0, 0, 0);
//			trap->G2API_InitGhoul2Model(&g2WeaponInstances[i], item->world_model[0],G_ModelIndex( item->world_model[0] ) , 0, 0, 0, 0);
			if (g2WeaponInstances[/*i*/item->giTag])
			{
				// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
				trap->G2API_SetBoltInfo(g2WeaponInstances[/*i*/item->giTag], 0, 0);
				// now set up the gun bolt on it
				if (item->giTag == WP_SABER)
				{
					trap->G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*blade1");
				}
				else
				{
					trap->G2API_AddBolt(g2WeaponInstances[/*i*/item->giTag], 0, "*flash");
				}
				i++;
			}
			if (i == MAX_WEAPONS)
			{
				assert(0);
				break;
			}

		}
	}
}

// clean out any g2 models we instanciated for copying purposes
void CG_ShutDownG2Weapons(void)
{
	int i;
	for (i=0; i<MAX_WEAPONS; i++)
	{
		trap->G2API_CleanGhoul2Models(&g2WeaponInstances[i]);
	}
}

void *CG_G2WeaponInstance(centity_t *cent, int weapon)
{
	clientInfo_t *ci = NULL;

	if (weapon != WP_SABER)
	{
		return g2WeaponInstances[weapon];
	}

	if (cent->currentState.eType != ET_PLAYER &&
		cent->currentState.eType != ET_NPC)
	{
		return g2WeaponInstances[weapon];
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	if (!ci)
	{
		return g2WeaponInstances[weapon];
	}

	//Try to return the custom saber instance if we can.
	if (ci->saber[0].model[0] &&
		ci->ghoul2Weapons[0])
	{
		return ci->ghoul2Weapons[0];
	}

	//If no custom then just use the default.
	return g2WeaponInstances[weapon];
}

// what ghoul2 model do we want to copy ?
void CG_CopyG2WeaponInstance(centity_t *cent, int weaponNum, void *toGhoul2)
{
	//rww - the -1 is because there is no "weapon" for WP_NONE
	assert(weaponNum < MAX_WEAPONS);
	if (CG_G2WeaponInstance(cent, weaponNum/*-1*/))
	{
		if (weaponNum == WP_SABER)
		{
			clientInfo_t *ci = NULL;

			if (cent->currentState.eType == ET_NPC)
			{
				ci = cent->npcClient;
			}
			else
			{
				ci = &cgs.clientinfo[cent->currentState.number];
			}

			if (!ci)
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1);
			}
			else
			{ //Try both the left hand saber and the right hand saber
				int i = 0;

				while (i < MAX_SABERS)
				{
					if (ci->saber[i].model[0] &&
						ci->ghoul2Weapons[i])
					{
						trap->G2API_CopySpecificGhoul2Model(ci->ghoul2Weapons[i], 0, toGhoul2, i+1);
					}
					else if (ci->ghoul2Weapons[i])
					{ //if the second saber has been removed, then be sure to remove it and free the instance.
						qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

						if (g2HasSecondSaber)
						{ //remove it now since we're switching away from sabers
							trap->G2API_RemoveGhoul2Model(&(toGhoul2), 2);
						}
						trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[i]);
					}

					i++;
				}
			}
		}
		else
		{
			qboolean g2HasSecondSaber = trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 2);

			if (g2HasSecondSaber)
			{ //remove it now since we're switching away from sabers
				trap->G2API_RemoveGhoul2Model(&(toGhoul2), 2);
			}

			if (weaponNum == WP_EMPLACED_GUN)
			{ //a bit of a hack to remove gun model when using an emplaced weap
				if (trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap->G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else if (weaponNum == WP_MELEE)
			{ //don't want a weapon on the model for this one
				if (trap->G2API_HasGhoul2ModelOnIndex(&(toGhoul2), 1))
				{
					trap->G2API_RemoveGhoul2Model(&(toGhoul2), 1);
				}
			}
			else
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, weaponNum/*-1*/), 0, toGhoul2, 1);
			}
		}
	}
}

void CG_CheckPlayerG2Weapons(playerState_t *ps, centity_t *cent)
{
	if (!ps)
	{
		assert(0);
		return;
	}

	if (ps->pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		assert(0);
		return;
	}

	// should we change the gun model on this player?
	if (cent->currentState.saberInFlight)
	{
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{ //no updating weapons when dead
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cent->torsoBolt)
	{ //got our limb cut off, no updating weapons until it's restored
		cent->ghoul2weapon = NULL;
		return;
	}

	if (cgs.clientinfo[ps->clientNum].team == TEAM_SPECTATOR ||
		ps->persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		cent->ghoul2weapon = cg_entities[ps->clientNum].ghoul2weapon = NULL;
		cent->weapon = cg_entities[ps->clientNum].weapon = 0;
		return;
	}

	if (cent->ghoul2 && cent->ghoul2weapon != CG_G2WeaponInstance(cent, ps->weapon) &&
		ps->clientNum == cent->currentState.number) //don't want spectator mode forcing one client's weapon instance over another's
	{
		CG_CopyG2WeaponInstance(cent, ps->weapon, cent->ghoul2);
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, ps->weapon);
		if (cent->weapon == WP_SABER && cent->weapon != ps->weapon && !ps->saberHolstered)
		{ //switching away from the saber
			//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOff && !ps->saberHolstered)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOff);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOff &&
				cgs.clientinfo[ps->clientNum].saber[1].model[0] &&
				!ps->saberHolstered)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOff);
			}
		}
		else if (ps->weapon == WP_SABER && cent->weapon != ps->weapon && !cent->saberWasInFlight)
		{ //switching to the saber
			//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
			if (cgs.clientinfo[ps->clientNum].saber[0].soundOn)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[0].soundOn);
			}

			if (cgs.clientinfo[ps->clientNum].saber[1].soundOn)
			{
				trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.clientinfo[ps->clientNum].saber[1].soundOn);
			}

			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[0], 0, -1);
			BG_SI_SetDesiredLength(&cgs.clientinfo[ps->clientNum].saber[1], 0, -1);
		}
		cent->weapon = ps->weapon;
	}
}


/*
Ghoul2 Insert End
*/
