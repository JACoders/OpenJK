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

#include "cg_local.h"
#include "cg_media.h"
#include "FxScheduler.h"
#include "../game/wp_saber.h"
#include "../game/g_local.h"
#include "../game/anims.h"

extern void CG_LightningBolt( centity_t *cent, vec3_t origin );

#define	PHASER_HOLDFRAME	2
int cgi_UI_GetMenuInfo(char *menuFile,int *x,int *y);
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
const char *CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight,
								const char *psText, int iFontHandle, float fScale,
								const vec4_t v4Color);

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	// error checking
	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponNum >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	// clear out the memory we use
	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	// find the weapon in the item list
	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	// if we couldn't find which weapon this is, give us an error
	if ( !item->classname ) {
		CG_Error( "Couldn't find item for weapon %s\nNeed to update Items.dat!", weaponData[weaponNum].classname);
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// set up in view weapon model
	weaponInfo->weaponModel = cgi_R_RegisterModel( weaponData[weaponNum].weaponMdl );
	{//in case the weaponmodel isn't _w, precache the _w.glm
		char weaponModel[64];

		Q_strncpyz (weaponModel, weaponData[weaponNum].weaponMdl, sizeof(weaponModel));
		if (char *spot = strstr(weaponModel, ".md3") )
		{
			*spot = 0;
			spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
			if (!spot)
			{
				Q_strcat (weaponModel, sizeof(weaponModel), "_w");
			}
			Q_strcat (weaponModel, sizeof(weaponModel), ".glm");	//and change to ghoul2
		}
		gi.G2API_PrecacheGhoul2Model( weaponModel ); // correct way is item->world_model
	}

	if ( weaponInfo->weaponModel == NULL_HANDLE )
	{
		CG_Error( "Couldn't find weapon model %s\n", weaponData[weaponNum].classname);
		return;
	}

	// calc midpoint for rotation
	cgi_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	// setup the shader we will use for the icon
	if (weaponData[weaponNum].weaponIcon[0])
	{
		weaponInfo->weaponIcon = cgi_R_RegisterShaderNoMip( weaponData[weaponNum].weaponIcon);
		weaponInfo->weaponIconNoAmmo = cgi_R_RegisterShaderNoMip( va("%s_na",weaponData[weaponNum].weaponIcon));
	}

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponData[weaponNum].ammoIndex) {
			break;
		}
	}

	if ( ammo->classname && ammo->world_model ) {
		weaponInfo->ammoModel = cgi_R_RegisterModel( ammo->world_model );
	}

	for (i=0; i< weaponData[weaponNum].numBarrels; i++) {
		Q_strncpyz( path, weaponData[weaponNum].weaponMdl, sizeof(path) );
		COM_StripExtension( path, path, sizeof(path) );
		if (i)
		{
			//char	crap[50];
			//Com_sprintf(crap, sizeof(crap), "_barrel%d.md3", i+1 );
			//strcat ( path, crap );
			Q_strcat( path, sizeof(path), va("_barrel%d.md3", i+1) );
		}
		else
			Q_strcat( path, sizeof(path), "_barrel.md3" );
		weaponInfo->barrelModel[i] = cgi_R_RegisterModel( path );
	}


	// set up the world model for the weapon
	weaponInfo->weaponWorldModel = cgi_R_RegisterModel( item->world_model );
	if ( !weaponInfo->weaponWorldModel) {
		weaponInfo->weaponWorldModel = weaponInfo->weaponModel;
	}

	// set up the hand that holds the in view weapon - assuming we have one
	Q_strncpyz( path, weaponData[weaponNum].weaponMdl, sizeof(path) );
	COM_StripExtension( path, path, sizeof(path) );
	Q_strcat( path, sizeof(path), "_hand.md3" );
	weaponInfo->handsModel = cgi_R_RegisterModel( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = cgi_R_RegisterModel( "models/weapons2/briar_pistol/briar_pistol_hand.md3" );
	}

	// register the sounds for the weapon
	if (weaponData[weaponNum].firingSnd[0]) {
		weaponInfo->firingSound = cgi_S_RegisterSound( weaponData[weaponNum].firingSnd );
	}
	if (weaponData[weaponNum].altFiringSnd[0]) {
		weaponInfo->altFiringSound = cgi_S_RegisterSound( weaponData[weaponNum].altFiringSnd );
	}
	if (weaponData[weaponNum].stopSnd[0]) {
		weaponInfo->stopSound = cgi_S_RegisterSound( weaponData[weaponNum].stopSnd );
	}
	if (weaponData[weaponNum].chargeSnd[0]) {
		weaponInfo->chargeSound = cgi_S_RegisterSound( weaponData[weaponNum].chargeSnd );
	}
	if (weaponData[weaponNum].altChargeSnd[0]) {
		weaponInfo->altChargeSound = cgi_S_RegisterSound( weaponData[weaponNum].altChargeSnd );
	}
	if (weaponData[weaponNum].selectSnd[0]) {
		weaponInfo->selectSound = cgi_S_RegisterSound( weaponData[weaponNum].selectSnd );
	}

	// give us missile models if we should
	if (weaponData[weaponNum].missileMdl[0]) 	{
		weaponInfo->missileModel = cgi_R_RegisterModel(weaponData[weaponNum].missileMdl );
	}
	if (weaponData[weaponNum].alt_missileMdl[0]) 	{
		weaponInfo->alt_missileModel = cgi_R_RegisterModel(weaponData[weaponNum].alt_missileMdl );
	}
	if (weaponData[weaponNum].missileSound[0]) {
		weaponInfo->missileSound = cgi_S_RegisterSound( weaponData[weaponNum].missileSound );
	}
	if (weaponData[weaponNum].alt_missileSound[0]) {
		weaponInfo->alt_missileSound = cgi_S_RegisterSound( weaponData[weaponNum].alt_missileSound );
	}
	if (weaponData[weaponNum].missileHitSound[0]) {
		weaponInfo->missileHitSound = cgi_S_RegisterSound( weaponData[weaponNum].missileHitSound );
	}
	if (weaponData[weaponNum].altmissileHitSound[0]) {
		weaponInfo->altmissileHitSound = cgi_S_RegisterSound( weaponData[weaponNum].altmissileHitSound );
	}
	if ( weaponData[weaponNum].mMuzzleEffect[0] )
	{
		weaponData[weaponNum].mMuzzleEffectID = theFxScheduler.RegisterEffect( weaponData[weaponNum].mMuzzleEffect );
	}
	if ( weaponData[weaponNum].mAltMuzzleEffect[0] )
	{
		weaponData[weaponNum].mAltMuzzleEffectID = theFxScheduler.RegisterEffect( weaponData[weaponNum].mAltMuzzleEffect );
	}

	//fixme: don't really need to copy these, should just use directly
	// give ourselves the functions if we can
	if (weaponData[weaponNum].func)
	{
		weaponInfo->missileTrailFunc = (void (QDECL *)(struct centity_s *,const struct weaponInfo_s *))weaponData[weaponNum].func;
	}
	if (weaponData[weaponNum].altfunc)
	{
		weaponInfo->alt_missileTrailFunc = (void (QDECL *)(struct centity_s *,const struct weaponInfo_s *))weaponData[weaponNum].altfunc;
	}

	switch ( weaponNum )	//extra client only stuff
	{
	case WP_SABER:
		//saber/force FX
		theFxScheduler.RegisterEffect( "spark" );
		theFxScheduler.RegisterEffect( "blood_sparks" );
		theFxScheduler.RegisterEffect( "force_touch" );
		theFxScheduler.RegisterEffect( "saber_block" );
		theFxScheduler.RegisterEffect( "saber_cut" );
		theFxScheduler.RegisterEffect( "blaster/smoke_bolton" );
		theFxScheduler.RegisterEffect( "saber/fizz" );
		theFxScheduler.RegisterEffect( "saber/boil" );

		cgs.effects.forceHeal			= theFxScheduler.RegisterEffect( "force/heal" );
		cgs.effects.forceInvincibility	= theFxScheduler.RegisterEffect( "force/invin" );
		cgs.effects.forceConfusion		= theFxScheduler.RegisterEffect( "force/confusion" );
		cgs.effects.forceLightning		= theFxScheduler.RegisterEffect( "force/lightning" );
		cgs.effects.forceLightningWide	= theFxScheduler.RegisterEffect( "force/lightningwide" );

		cgs.media.HUDSaberStyleFast		= cgi_R_RegisterShader( "gfx/hud/saber_stylesFast" );
		cgs.media.HUDSaberStyleMed		= cgi_R_RegisterShader( "gfx/hud/saber_stylesMed" );
		cgs.media.HUDSaberStyleStrong	= cgi_R_RegisterShader( "gfx/hud/saber_stylesStrong" );

		//saber sounds
		cgi_S_RegisterSound( "sound/weapons/saber/saberon.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/enemy_saber_on.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/saberonquick.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/saberoff.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/enemy_saber_off.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/saberspinoff.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" );
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberbounce%d.wav", i ) );
		}
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberhit%d.wav", i ) );
		}
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberhitwall%d.wav", i ) );
		}
		for ( i = 1; i < 10; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberblock%d.wav", i ) );
		}
		for ( i = 1; i < 6; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberhum%d.wav", i ) );
		}
		for ( i = 1; i < 10; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberhup%d.wav", i ) );
		}
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/saberspin%d.wav", i ) );
		}
		cgi_S_RegisterSound( "sound/weapons/saber/saber_catch.wav" );
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/bounce%d.wav", i ) );
		}
		cgi_S_RegisterSound( "sound/weapons/saber/hitwater.wav" );
		cgi_S_RegisterSound( "sound/weapons/saber/boiling.wav" );
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/saber/rainfizz%d.wav", i ) );
		}

		//force sounds
		cgi_S_RegisterSound( "sound/weapons/force/heal.mp3" );
		cgi_S_RegisterSound( "sound/weapons/force/speed.mp3" );
		cgi_S_RegisterSound( "sound/weapons/force/speedloop.mp3" );
		for ( i = 1; i < 5; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/force/heal%d.mp3", i ) );
		}
		cgi_S_RegisterSound( "sound/weapons/force/lightning.wav" );
		cgi_S_RegisterSound( "sound/weapons/force/lightning2.wav" );
		for ( i = 1; i < 4; i++ )
		{
			cgi_S_RegisterSound( va( "sound/weapons/force/lightninghit%d.wav", i ) );
		}
		cgi_S_RegisterSound( "sound/weapons/force/push.wav" );
		cgi_S_RegisterSound( "sound/weapons/force/pull.wav" );
		cgi_S_RegisterSound( "sound/weapons/force/jump.wav" );
		cgi_S_RegisterSound( "sound/weapons/force/jumpbuild.wav" );
		cgi_S_RegisterSound( "sound/weapons/force/grip.mp3" );

		//saber graphics
		cgs.media.saberBlurShader			= cgi_R_RegisterShader("gfx/effects/sabers/saberBlur");
		cgs.media.yellowDroppedSaberShader	= cgi_R_RegisterShader("gfx/effects/yellow_glow");
		cgi_R_RegisterShader( "gfx/effects/saberDamageGlow" );
		cgi_R_RegisterShader( "gfx/effects/solidWhite_cull" );
		cgi_R_RegisterShader( "gfx/effects/forcePush" );
		cgi_R_RegisterShader( "gfx/effects/saberFlare" );
		cgs.media.redSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/red_glow" );
		cgs.media.redSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/red_line" );
		cgs.media.orangeSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/orange_glow" );
		cgs.media.orangeSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/orange_line" );
		cgs.media.yellowSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/yellow_glow" );
		cgs.media.yellowSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/yellow_line" );
		cgs.media.greenSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/green_glow" );
		cgs.media.greenSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/green_line" );
		cgs.media.blueSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/blue_glow" );
		cgs.media.blueSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/blue_line" );
		cgs.media.purpleSaberGlowShader		= cgi_R_RegisterShader( "gfx/effects/sabers/purple_glow" );
		cgs.media.purpleSaberCoreShader		= cgi_R_RegisterShader( "gfx/effects/sabers/purple_line" );

		cgs.media.forceCoronaShader			= cgi_R_RegisterShaderNoMip( "gfx/hud/force_swirl" );
		break;

	case WP_BRYAR_PISTOL:
		cgs.effects.bryarShotEffect			= theFxScheduler.RegisterEffect( "bryar/shot" );
											theFxScheduler.RegisterEffect( "bryar/NPCshot" );
		cgs.effects.bryarPowerupShotEffect	= theFxScheduler.RegisterEffect( "bryar/crackleShot" );
		cgs.effects.bryarWallImpactEffect	= theFxScheduler.RegisterEffect( "bryar/wall_impact" );
		cgs.effects.bryarWallImpactEffect2	= theFxScheduler.RegisterEffect( "bryar/wall_impact2" );
		cgs.effects.bryarWallImpactEffect3	= theFxScheduler.RegisterEffect( "bryar/wall_impact3" );
		cgs.effects.bryarFleshImpactEffect	= theFxScheduler.RegisterEffect( "bryar/flesh_impact" );

		// Note....these are temp shared effects
		theFxScheduler.RegisterEffect( "blaster/deflect" );
		theFxScheduler.RegisterEffect( "blaster/smoke_bolton" ); // note: this will be called game side
		break;

	case WP_BLASTER:
		cgs.effects.blasterShotEffect			= theFxScheduler.RegisterEffect( "blaster/shot" );
													theFxScheduler.RegisterEffect( "blaster/NPCshot" );
//		cgs.effects.blasterOverchargeEffect		= theFxScheduler.RegisterEffect( "blaster/overcharge" );
		cgs.effects.blasterWallImpactEffect		= theFxScheduler.RegisterEffect( "blaster/wall_impact" );
		cgs.effects.blasterFleshImpactEffect	= theFxScheduler.RegisterEffect( "blaster/flesh_impact" );
		theFxScheduler.RegisterEffect( "blaster/deflect" );
		theFxScheduler.RegisterEffect( "blaster/smoke_bolton" ); // note: this will be called game side
		break;

	case WP_DISRUPTOR:
		theFxScheduler.RegisterEffect( "disruptor/wall_impact" );
		theFxScheduler.RegisterEffect( "disruptor/flesh_impact" );
		theFxScheduler.RegisterEffect( "disruptor/alt_miss" );
		theFxScheduler.RegisterEffect( "disruptor/alt_hit" );
		theFxScheduler.RegisterEffect( "disruptor/line_cap" );
		theFxScheduler.RegisterEffect( "disruptor/death_smoke" );

		cgi_R_RegisterShader( "gfx/effects/redLine" );
		cgi_R_RegisterShader( "gfx/misc/whiteline2" );
		cgi_R_RegisterShader( "gfx/effects/smokeTrail" );
		cgi_R_RegisterShader( "gfx/effects/burn" );

		cgi_R_RegisterShaderNoMip( "gfx/2d/crop_charge" );

		// zoom sounds
		cgi_S_RegisterSound( "sound/weapons/disruptor/zoomstart.wav" );
		cgi_S_RegisterSound( "sound/weapons/disruptor/zoomend.wav" );
		cgs.media.disruptorZoomLoop = cgi_S_RegisterSound( "sound/weapons/disruptor/zoomloop.wav" );

		// Disruptor gun zoom interface
		cgs.media.disruptorMask			= cgi_R_RegisterShader( "gfx/2d/cropCircle2");
		cgs.media.disruptorInsert		= cgi_R_RegisterShader( "gfx/2d/cropCircle");
		cgs.media.disruptorLight		= cgi_R_RegisterShader( "gfx/2d/cropCircleGlow" );
		cgs.media.disruptorInsertTick	= cgi_R_RegisterShader( "gfx/2d/insertTick" );
		break;

	case WP_BOWCASTER:
		cgs.effects.bowcasterShotEffect		= theFxScheduler.RegisterEffect( "bowcaster/shot" );
		cgs.effects.bowcasterBounceEffect	= theFxScheduler.RegisterEffect( "bowcaster/bounce" );
		cgs.effects.bowcasterImpactEffect	= theFxScheduler.RegisterEffect( "bowcaster/explosion" );
		theFxScheduler.RegisterEffect( "bowcaster/deflect" );
		break;

	case WP_REPEATER:
		theFxScheduler.RegisterEffect( "repeater/muzzle_smoke" );
		theFxScheduler.RegisterEffect( "repeater/projectile" );
		theFxScheduler.RegisterEffect( "repeater/alt_projectile" );
		theFxScheduler.RegisterEffect( "repeater/wall_impact" );
//		theFxScheduler.RegisterEffect( "repeater/alt_wall_impact2" );
//		theFxScheduler.RegisterEffect( "repeater/flesh_impact" );
		theFxScheduler.RegisterEffect( "repeater/concussion" );
		break;

	case WP_DEMP2:
		theFxScheduler.RegisterEffect( "demp2/projectile" );
		theFxScheduler.RegisterEffect( "demp2/wall_impact" );
		theFxScheduler.RegisterEffect( "demp2/flesh_impact" );
		theFxScheduler.RegisterEffect( "demp2/altDetonate" );
		cgi_R_RegisterModel( "models/items/sphere.md3" );
		cgi_R_RegisterShader( "gfx/effects/demp2shell" );
		break;

	case WP_ATST_MAIN:
		theFxScheduler.RegisterEffect( "atst/shot" );
		theFxScheduler.RegisterEffect( "atst/wall_impact" );
		theFxScheduler.RegisterEffect( "atst/flesh_impact" );
		theFxScheduler.RegisterEffect( "atst/droid_impact" );
		break;

	case WP_ATST_SIDE:
		// For the ALT fire
		theFxScheduler.RegisterEffect( "atst/side_alt_shot" );
		theFxScheduler.RegisterEffect( "atst/side_alt_explosion" );

		// For the regular fire
		theFxScheduler.RegisterEffect( "atst/side_main_shot" );
		theFxScheduler.RegisterEffect( "atst/side_main_impact" );
		break;

	case WP_FLECHETTE:
		cgs.effects.flechetteShotEffect				= theFxScheduler.RegisterEffect( "flechette/shot" );
		cgs.effects.flechetteAltShotEffect			= theFxScheduler.RegisterEffect( "flechette/alt_shot" );
		cgs.effects.flechetteShotDeathEffect		= theFxScheduler.RegisterEffect( "flechette/wall_impact" ); // shot death
		cgs.effects.flechetteFleshImpactEffect		= theFxScheduler.RegisterEffect( "flechette/flesh_impact" );
		cgs.effects.flechetteRicochetEffect			= theFxScheduler.RegisterEffect( "flechette/ricochet" );

//		theFxScheduler.RegisterEffect( "flechette/explosion" );
		theFxScheduler.RegisterEffect( "flechette/alt_blow" );
		break;

	case WP_ROCKET_LAUNCHER:
		theFxScheduler.RegisterEffect( "rocket/shot" );
		theFxScheduler.RegisterEffect( "rocket/explosion" );

		cgi_R_RegisterShaderNoMip( "gfx/2d/wedge" );
		cgi_R_RegisterShaderNoMip( "gfx/2d/lock" );

		cgi_S_RegisterSound( "sound/weapons/rocket/lock.wav" );
		cgi_S_RegisterSound( "sound/weapons/rocket/tick.wav" );
		break;

	case WP_THERMAL:
		cgs.media.grenadeBounce1		= cgi_S_RegisterSound( "sound/weapons/thermal/bounce1.wav" );
		cgs.media.grenadeBounce2		= cgi_S_RegisterSound( "sound/weapons/thermal/bounce2.wav" );

		cgi_S_RegisterSound( "sound/weapons/thermal/thermloop.wav" );
		cgi_S_RegisterSound( "sound/weapons/thermal/warning.wav" );
		theFxScheduler.RegisterEffect( "thermal/explosion" );
		theFxScheduler.RegisterEffect( "thermal/shockwave" );
		break;

	case WP_TRIP_MINE:
		theFxScheduler.RegisterEffect( "tripMine/explosion" );
		theFxScheduler.RegisterEffect( "tripMine/laser" );
		theFxScheduler.RegisterEffect( "tripMine/laserImpactGlow" );
		theFxScheduler.RegisterEffect( "tripMine/glowBit" );

		cgs.media.tripMineStickSound = cgi_S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		cgi_S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		cgi_S_RegisterSound( "sound/weapons/laser_trap/hum_loop.wav" );
		break;

	case WP_DET_PACK:
		theFxScheduler.RegisterEffect( "detpack/explosion.efx" );

		cgs.media.detPackStickSound = cgi_S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		cgi_R_RegisterModel( "models/weapons2/detpack/detpack.md3" );
		cgi_S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		cgi_S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;

	case WP_EMPLACED_GUN:
		theFxScheduler.RegisterEffect( "emplaced/shot" );
		theFxScheduler.RegisterEffect( "emplaced/shotNPC" );
		theFxScheduler.RegisterEffect( "emplaced/wall_impact" );
		cgi_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_dmg" );
		cgi_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_on" );

		cgs.media.emplacedHealthBarShader		= cgi_R_RegisterShaderNoMip( "gfx/hud/atst_health_frame" );
		cgs.media.ladyLuckHealthShader			= cgi_R_RegisterShaderNoMip( "gfx/hud/ladyluck_health_frame" );
		cgs.media.turretComputerOverlayShader	= cgi_R_RegisterShaderNoMip( "gfx/hud/generic_target" );
		cgs.media.turretCrossHairShader			= cgi_R_RegisterShaderNoMip( "gfx/2d/panel_crosshair" );
		break;

	case WP_MELEE:
		//TEMP
		cgi_S_RegisterSound( "sound/weapons/melee/punch1.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch2.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch3.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch4.mp3" );
		break;

	case WP_STUN_BATON:
		cgi_R_RegisterShader( "gfx/effects/stunPass" );
		theFxScheduler.RegisterEffect( "stunBaton/flesh_impact" );
		//TEMP
		cgi_S_RegisterSound( "sound/weapons/melee/punch1.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch2.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch3.mp3" );
		cgi_S_RegisterSound( "sound/weapons/melee/punch4.mp3" );
		cgi_S_RegisterSound( "sound/weapons/baton/fire" );
		break;

	case WP_TURRET:
		theFxScheduler.RegisterEffect( "turret/shot" );
		theFxScheduler.RegisterEffect( "turret/wall_impact" );
		theFxScheduler.RegisterEffect( "turret/flesh_impact" );
		break;

	case WP_BLASTER_PISTOL: // enemy version
		cgs.effects.bryarShotEffect			= theFxScheduler.RegisterEffect( "bryar/shot" );
		cgs.effects.bryarPowerupShotEffect	= theFxScheduler.RegisterEffect( "bryar/crackleShot" );
		cgs.effects.bryarWallImpactEffect	= theFxScheduler.RegisterEffect( "bryar/wall_impact" );
		cgs.effects.bryarFleshImpactEffect	= theFxScheduler.RegisterEffect( "bryar/flesh_impact" );
		// Note....these are temp shared effects
		theFxScheduler.RegisterEffect( "blaster/deflect" );
		theFxScheduler.RegisterEffect( "blaster/smoke_bolton" ); // note: this will be called game side
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models = cgi_R_RegisterModel( item->world_model );

	if ( item->icon && item->icon[0] )
	{
		itemInfo->icon = cgi_R_RegisterShaderNoMip( item->icon );
	}
	else
	{
		itemInfo->icon = -1;
	}

	if ( item->giType == IT_WEAPON )
	{
		CG_RegisterWeapon( item->giTag );
	}

	// some ammo types are actually the weapon, like in the case of explosives
	if ( item->giType == IT_AMMO )
	{
		switch( item->giTag )
		{
		case AMMO_THERMAL:
			CG_RegisterWeapon( WP_THERMAL );
			break;
		case AMMO_TRIPMINE:
			CG_RegisterWeapon( WP_TRIP_MINE );
			break;
		case AMMO_DETPACK:
			CG_RegisterWeapon( WP_DET_PACK );
			break;
		}
	}


	if ( item->giType == IT_HOLDABLE )
	{
		// This should be set up to actually work.
		switch( item->giTag )
		{
		case INV_SEEKER:
			cgi_S_RegisterSound("sound/chars/seeker/misc/fire.wav");
			cgi_S_RegisterSound( "sound/chars/seeker/misc/hiss.wav");
			theFxScheduler.RegisterEffect( "env/small_explode");

			CG_RegisterWeapon( WP_BLASTER );
			break;

		case INV_SENTRY:
			CG_RegisterWeapon( WP_TURRET );
			cgi_S_RegisterSound( "sound/player/use_sentry" );
			break;

		case INV_ELECTROBINOCULARS:
			// Binocular interface
			cgs.media.binocularCircle		= cgi_R_RegisterShader( "gfx/2d/binCircle" );
			cgs.media.binocularMask			= cgi_R_RegisterShader( "gfx/2d/binMask" );
			cgs.media.binocularArrow		= cgi_R_RegisterShader( "gfx/2d/binSideArrow" );
			cgs.media.binocularTri			= cgi_R_RegisterShader( "gfx/2d/binTopTri" );
			cgs.media.binocularStatic		= cgi_R_RegisterShader( "gfx/2d/binocularWindow" );
			cgs.media.binocularOverlay		= cgi_R_RegisterShader( "gfx/2d/binocularNumOverlay" );
			break;

		case INV_LIGHTAMP_GOGGLES:
			// LA Goggles Shaders
			cgs.media.laGogglesStatic		= cgi_R_RegisterShader( "gfx/2d/lagogglesWindow" );
			cgs.media.laGogglesMask			= cgi_R_RegisterShader( "gfx/2d/amp_mask" );
			cgs.media.laGogglesSideBit		= cgi_R_RegisterShader( "gfx/2d/side_bit" );
			cgs.media.laGogglesBracket		= cgi_R_RegisterShader( "gfx/2d/bracket" );
			cgs.media.laGogglesArrow		= cgi_R_RegisterShader( "gfx/2d/bracket2" );
			break;

		case INV_BACTA_CANISTER:
			for ( int i = 1; i < 5; i++ )
			{
				cgi_S_RegisterSound( va( "sound/weapons/force/heal%d.mp3", i ));
			}
			break;
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

animations MUST match the defined pattern!
the weapon hand animation has 3 anims,
	6 frames of attack
	4 frames of drop
	5 frames of raise

  if the torso anim does not match these lengths, it will not animate correctly!
=================
*/
extern qboolean ValidAnimFileIndex ( int index );
int CG_MapTorsoToWeaponFrame( const clientInfo_t *ci, int frame, int animNum, int weaponNum, int firing )
{
	// we should use the animNum to map a weapon frame instead of relying on the torso frame
	if ( !ValidAnimFileIndex( ci->animFileIndex ) )
	{
		return 0;
	}
	animation_t *animations = level.knownAnimFileSets[ci->animFileIndex].animations;
	int ret=0;

	switch( animNum )
	{
	case TORSO_WEAPONREADY1:
	case TORSO_WEAPONREADY2:
	case TORSO_WEAPONREADY3:
	case TORSO_WEAPONREADY4:
	case TORSO_WEAPONREADY5:
	case TORSO_WEAPONREADY6:
	case TORSO_WEAPONREADY7:
	case TORSO_WEAPONREADY8:
	case TORSO_WEAPONREADY9:
	case TORSO_WEAPONREADY10:
	case TORSO_WEAPONREADY11:
	case TORSO_WEAPONREADY12:
		ret = 0;
		break;

	case TORSO_DROPWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 5 )
		{
			ret = frame - animations[animNum].firstFrame + 6;
		}
		else
		{
//			assert(0);
		}
		break;

	case TORSO_RAISEWEAP1:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 4 )
		{
			ret = frame - animations[animNum].firstFrame + 6 + 5;
		}
		else
		{
//			assert(0);
		}
		break;

	case BOTH_ATTACK1:
	case BOTH_ATTACK2:
	case BOTH_ATTACK3:
	case BOTH_ATTACK4:
		if ( frame >= animations[animNum].firstFrame && frame < animations[animNum].firstFrame + 6 )
		{
			ret = 1 + ( frame - animations[animNum].firstFrame );
		}
		else
		{
//			assert(0);
		}
		break;
	default:
		break;
	}

	return ret;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles )
{
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.0075;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.0075;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 *
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
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

	// idle drift
	scale = /*cg.xyspeed + */40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += (scale * 0.5f ) * fracsin * 0.01;
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
/*
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleNormalize360( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}
*/
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


//--------------------------------------------------------------------------
static void CG_DoMuzzleFlash( centity_t *cent, vec3_t org, vec3_t dir, weaponData_t *wData )
{
	// Handle muzzle flashes, really this could just be a qboolean instead of a time.......
	if ( cent->muzzleFlashTime > 0 )
	{
		cent->muzzleFlashTime  = 0;
		const char *effect = NULL;

//		CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

		// Try and get a default muzzle so we have one to fall back on
		if ( wData->mMuzzleEffect[0] )
		{
			effect = &wData->mMuzzleEffect[0];
		}

		if ( cent->altFire )
		{
			// We're alt-firing, so see if we need to override with a custom alt-fire effect
			if ( wData->mAltMuzzleEffect[0] )
			{
				effect = &wData->mAltMuzzleEffect[0];
			}
		}

		if (/*( cent->currentState.eFlags & EF_FIRING || cent->currentState.eFlags & EF_ALT_FIRING ) &&*/ effect )
		{
			vec3_t up={0,0,1}, ax[3];

			VectorCopy( dir, ax[0] );

			CrossProduct( up, ax[0], ax[1] );
			CrossProduct( ax[0], ax[1], ax[2] );

			if (( cent->gent && cent->gent->NPC ) || cg.renderingThirdPerson )
			{
				theFxScheduler.PlayEffect( effect, org, dir );
			}
			else
			{
				// We got an effect and we're firing, so let 'er rip.
				theFxScheduler.PlayEffect( effect, cent->currentState.clientNum );
			}
		}
	}
	else
	{
//		CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash", NULL);
	}
}

/*
Ghoul2 Insert End
*/

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
extern int PM_TorsoAnimForFrame( gentity_t *ent, int torsoFrame );
extern float CG_ForceSpeedFOV( void );

void CG_AddViewWeapon( playerState_t *ps )
{
	refEntity_t	hand;
	refEntity_t	gun;
	refEntity_t	flash;
	vec3_t		angles;
	const weaponInfo_t	*weapon;
	weaponData_t  *wData;
	centity_t	*cent;
	float		fovOffset, leanOffset;
	float		cgFov = (cg_fovViewmodel.integer) ? cg_fovViewmodel.integer : cg_fov.integer;
	int i;

	if (cgFov < 1)
		cgFov = 1;
	else if (cgFov > 130)
		cgFov = 130;

	// no gun if in third person view
	if ( cg.renderingThirdPerson )
		return;

	if ( ps->pm_type == PM_INTERMISSION )
		return;

	cent = &cg_entities[ps->clientNum];

	if ( ps->eFlags & EF_LOCKED_TO_WEAPON )
	{
		return;
	}

	if ( cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive&(1<<FP_LIGHTNING) )
	{//doing the electrocuting
		vec3_t tAng, fxDir, temp;
		VectorSet( tAng, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0 );

		VectorCopy( cent->gent->client->renderInfo.handLPoint, temp );
		VectorMA( temp, -5, cg.refdef.viewaxis[0], temp );
		if ( cent->gent->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
		{//arc
			vec3_t	fxAxis[3];
			AnglesToAxis( tAng, fxAxis );
			theFxScheduler.PlayEffect( cgs.effects.forceLightningWide, temp, fxAxis );
		}
		else
		{//line
			AngleVectors( tAng, fxDir, NULL, NULL );
			theFxScheduler.PlayEffect( cgs.effects.forceLightning, temp, fxDir );
		}
	}
	// allow the gun to be completely removed
	if ( !cg_drawGun.integer || cg.zoomMode )
	{
		vec3_t		origin;

		// special hack for lightning guns...
		VectorCopy( cg.refdef.vieworg, origin );
		VectorMA( origin, -10, cg.refdef.viewaxis[2], origin );
		VectorMA( origin, 16, cg.refdef.viewaxis[0], origin );
// Doesn't look like we'll have lightning style guns.  Clean this crap up when we are sure about this.
//		CG_LightningBolt( cent, origin );

		// We should still do muzzle flashes though...
		CG_RegisterWeapon( ps->weapon );
		weapon = &cg_weapons[ps->weapon];
		wData =  &weaponData[ps->weapon];

		CG_DoMuzzleFlash( cent, origin, cg.refdef.viewaxis[0], wData );

		// If we don't update this, the muzzle flash point won't even be updated properly
		VectorCopy( origin, cent->gent->client->renderInfo.muzzlePoint );
		VectorCopy( cg.refdef.viewaxis[0], cent->gent->client->renderInfo.muzzleDir );

		cent->gent->client->renderInfo.mPCalcTime = cg.time;
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun )
	{
		return;
	}

	// drop gun lower at higher fov
	float actualFOV;
		gentity_t	*player = &g_entities[0];
	if ( (cg.snap->ps.forcePowersActive&(1<<FP_SPEED)) && player->client->ps.forcePowerDuration[FP_SPEED] )//cg.renderingThirdPerson &&
	{
		actualFOV = CG_ForceSpeedFOV();
		actualFOV = (cg_fovViewmodel.integer) ? actualFOV + (cg_fovViewmodel.integer - cg_fov.integer) : actualFOV;
	}
	else
	{
		actualFOV = (cg.overrides.active&CG_OVERRIDE_FOV) ? cg.overrides.fov : cg_fov.value;
	}

	if ( cg_fovViewmodelAdjust.integer && actualFOV > 80 )
	{
		fovOffset = -0.1 * ( actualFOV - 80 );
	}
	else
	{
		fovOffset = 0;
	}

	if ( ps->leanofs != 0 )
	{
		//add leaning offset
		leanOffset = ps->leanofs * 0.25f;
		fovOffset += fabs((double)ps->leanofs) * -0.1f;
	}
	else
	{
		leanOffset = 0;
	}

	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ps->weapon];
	wData =  &weaponData[ps->weapon];

	memset (&hand, 0, sizeof(hand));

	if ( ps->weapon == WP_STUN_BATON )
	{
		cgi_S_AddLoopingSound( cent->currentState.number,
			cent->lerpOrigin,
			vec3_origin,
			weapon->firingSound );
	}

	if ( ps->weapon == WP_NONE )
	{
		return;
	}

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, (cg_gun_y.value+leanOffset), cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	if ( cg_fovViewmodel.integer )
	{
		float fracDistFOV = tanf( cg.refdef.fov_x * ( M_PI/180 ) * 0.5f );
		float fracWeapFOV = ( 1.0f / fracDistFOV ) * tanf( cgFov * ( M_PI/180 ) * 0.5f );
		VectorScale( hand.axis[0], fracWeapFOV, hand.axis[0] );
	}

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer )
	{
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}
	else
	{
		// get clientinfo for animation map
		const clientInfo_t	*ci = &cent->gent->client->clientInfo;
		int torsoAnim = cent->gent->client->ps.torsoAnim;//pe.torso.animationNumber;
		float currentFrame;
		int startFrame,endFrame,flags;
		float animSpeed;
		if (cent->gent->lowerLumbarBone>=0&& gi.G2API_GetBoneAnimIndex(&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->lowerLumbarBone, cg.time, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed,0) )
		{
			hand.oldframe = CG_MapTorsoToWeaponFrame( ci,floor(currentFrame), torsoAnim, cent->currentState.weapon, ( cent->currentState.eFlags & EF_FIRING ) );
			hand.frame = CG_MapTorsoToWeaponFrame( ci,ceil(currentFrame), torsoAnim, cent->currentState.weapon, ( cent->currentState.eFlags & EF_FIRING ) );
			hand.backlerp=1.0f-(currentFrame-floor(currentFrame));
			if ( cg_debugAnim.integer == 1 && cent->currentState.clientNum == 0 )
			{
				Com_Printf( "Torso frame %d to %d makes Weapon frame %d to %d\n", cent->pe.torso.oldFrame,  cent->pe.torso.frame, hand.oldframe, hand.frame );
			}
		}
		else
		{
//			assert(0); // no idea what to do here
			hand.oldframe=0;
			hand.frame=0;
			hand.backlerp=0.0f;
		}
	}

	// add the weapon
	memset (&gun, 0, sizeof(gun));

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel)
	{
		return;
	}

	AnglesToAxis( angles, gun.axis );
	CG_PositionEntityOnTag( &gun, &hand, weapon->handsModel, "tag_weapon");

	gun.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

//---------
	// OK, we are making an assumption here that if we have the phaser that it is always on....
	//FIXME: if saberInFlight, need to draw empty hand guiding it
	if ( cent->gent && cent->gent->client && cent->currentState.weapon == WP_SABER )
	{
		vec3_t org_, axis_[3];

		CG_GetTagWorldPosition( &gun, "tag_flash", org_, axis_ );
		if ( cent->gent->client->ps.saberActive && cent->gent->client->ps.saberLength < cent->gent->client->ps.saberLengthMax )
		{
			cent->gent->client->ps.saberLength += cg.frametime*0.03;
			if ( cent->gent->client->ps.saberLength > cent->gent->client->ps.saberLengthMax )
			{
				cent->gent->client->ps.saberLength = cent->gent->client->ps.saberLengthMax;
			}
		}
//		FX_Saber( org_, axis_[0], cent->gent->client->ps.saberLength, 2.0 + Q_flrand(-1.0f, 1.0f) * 0.2f, cent->gent->client->ps.saberColor );
		VectorCopy( axis_[0], cent->gent->client->renderInfo.muzzleDir );
	}
//---------

//	CG_AddRefEntityWithPowerups( &gun, cent->currentState.powerups, cent->gent );
	cgi_R_AddRefEntityToScene( &gun );

/*	if ( ps->weapon == WP_STUN_BATON )
	{
		gun.shaderRGBA[0] = gun.shaderRGBA[1] = gun.shaderRGBA[2] = 25;

		gun.customShader = cgi_R_RegisterShader( "gfx/effects/stunPass" );
		gun.renderfx = RF_RGB_TINT | RF_FIRST_PERSON | RF_DEPTHHACK;
		cgi_R_AddRefEntityToScene( &gun );
	}
*/
	// add the spinning barrel[s]
	for (i = 0; (i < wData->numBarrels); i++)
	{
		refEntity_t	barrel;
		memset( &barrel, 0, sizeof( barrel ) );
		barrel.hModel = weapon->barrelModel[i];

		//VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		//barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = gun.renderfx;
		angles[YAW] = 0;
		angles[PITCH] = 0;
//		if ( ps->weapon == WP_TETRION_DISRUPTOR) {
//			angles[ROLL] = CG_MachinegunSpinAngle( cent );
//		} else {
			angles[ROLL] = 0;//CG_MachinegunSpinAngle( cent );
//		}

		AnglesToAxis( angles, barrel.axis );
		if (!i)
		{
			CG_PositionRotatedEntityOnTag( &barrel, &hand, weapon->handsModel, "tag_barrel", NULL );
		} else
		{
			CG_PositionRotatedEntityOnTag( &barrel, &hand, weapon->handsModel, va("tag_barrel%d",i+1), NULL );
		}

		cgi_R_AddRefEntityToScene( &barrel );
	}

	memset (&flash, 0, sizeof(flash));

	// Seems like we should always do this in case we have an animating muzzle flash....that way we can always store the correct muzzle dir, etc.
	CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

	CG_DoMuzzleFlash( cent, flash.origin, flash.axis[0], wData );

	if ( cent->gent && cent->gent->client )
	{
		VectorCopy(flash.origin, cent->gent->client->renderInfo.muzzlePoint);
		VectorCopy(flash.axis[0], cent->gent->client->renderInfo.muzzleDir);
//		VectorNormalize( cent->gent->client->renderInfo.muzzleDir );
		cent->gent->client->renderInfo.mPCalcTime = cg.time;

		CG_LightningBolt( cent, flash.origin );
	}

	// Do special charge bits
	//-----------------------
	if (( ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BRYAR_PISTOL )
			|| ( ps->weapon == WP_BOWCASTER && ps->weaponstate == WEAPON_CHARGING )
			|| ( ps->weapon == WP_DEMP2 && ps->weaponstate == WEAPON_CHARGING_ALT ))
	{
		int		shader = 0;
		float	val = 0.0f, scale = 1.0f;
		vec3_t	WHITE	= {1.0f,1.0f,1.0f};

		if ( ps->weapon == WP_BRYAR_PISTOL )
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
			shader = cgi_R_RegisterShader( "gfx/effects/bryarFrontFlash" );
		}
		else if ( ps->weapon == WP_BOWCASTER )
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
			shader = cgi_R_RegisterShader( "gfx/effects/greenFrontFlash" );
		}
		else if ( ps->weapon == WP_DEMP2 )
		{
			// Hardcoded max charge time of 1 second
			val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
			shader = cgi_R_RegisterShader( "gfx/misc/lightningFlash" );
			scale = 1.75f;
		}

		if ( val < 0.0f )
		{
			val = 0.0f;
		}
		else if ( val > 1.0f )
		{
			val = 1.0f;
			CGCam_Shake( 0.1f, 100 );
		}
		else
		{
			CGCam_Shake( val * val * 0.3f, 100 );
		}

		val += Q_flrand(0.0f, 1.0f) * 0.5f;

		FX_AddSprite( flash.origin, NULL, NULL, 3.0f * val * scale, 0.0f, 0.7f, 0.7f, WHITE, WHITE, Q_flrand(0.0f, 1.0f) * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA | FX_DEPTH_HACK );
	}

	// Check if the heavy repeater is finishing up a sustained burst
	//-------------------------------
	if ( ps->weapon == WP_REPEATER && ps->weaponstate == WEAPON_FIRING )
	{
		if ( cent->gent && cent->gent->client && cent->gent->client->ps.weaponstate != WEAPON_FIRING )
		{
			int	ct = 0;

			// the more continuous shots we've got, the more smoke we spawn
			if ( cent->gent->client->ps.weaponShotCount > 60 ) {
				ct = 5;
			}
			else if ( cent->gent->client->ps.weaponShotCount > 35 ) {
				ct = 3;
			}
			else if ( cent->gent->client->ps.weaponShotCount > 15 ) {
				ct = 1;
			}

			for ( i = 0; i < ct; i++ )
			{
				theFxScheduler.PlayEffect( "repeater/muzzle_smoke", cent->currentState.clientNum );
			}

			cent->gent->client->ps.weaponShotCount = 0;
		}
	}
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_WeaponCheck
===================
*/
int CG_WeaponCheck( int weaponIndex )
{
	int				value;

	if ( weaponIndex == WP_SABER || weaponIndex == WP_STUN_BATON )
	{
		return qtrue;
	}

	value = weaponData[weaponIndex].energyPerShot < weaponData[weaponIndex].altEnergyPerShot
							? weaponData[weaponIndex].energyPerShot
							: weaponData[weaponIndex].altEnergyPerShot;

	if( !cg.snap )
	{
		return qfalse;
	}

	// check how much energy(ammo) it takes to fire this weapon against how much ammo we have
	if ( value > cg.snap->ps.ammo[weaponData[weaponIndex].ammoIndex] )
	{
		value = qfalse;
	}
	else
	{
		value = qtrue;
	}

	return value;
}

/*
===================
CG_DrawIconBackground
===================
*/
void CG_DrawIconBackground(void)
{
	int				height,xAdd,x2,y2,t;
	int				prongLeftX,prongRightX;
	qhandle_t		background;

	if (( cg.zoomMode != 0 ) || !( cg_drawHUD.integer ))
	{
		return;
	}

	if ((cg.snap->ps.viewEntity>0&&cg.snap->ps.viewEntity<ENTITYNUM_WORLD))
	{
		return;
	}


	if (!cgi_UI_GetMenuInfo("iconbackground",&x2,&y2))
	{
		return;
	}

	prongLeftX =x2+37;
	prongRightX =x2+544;

	if (((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time) || (cgs.media.currentBackground == ICON_INVENTORY))	// Display inventory background?
	{
		background = cgs.media.inventoryIconBackground;
	}
	else if (((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time) || (cgs.media.currentBackground == ICON_WEAPONS))	// Display weapon background?
	{
		background = cgs.media.weaponIconBackground;
	}
	else 	// Display force background?
	{
		background = cgs.media.forceIconBackground;
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

			xAdd = (int) 8*cg.iconHUDPercent;

			height = (int) (60.0f*cg.iconHUDPercent);
			CG_DrawPic( x2+60, y2+30, 460, -height, background);	// Top half
			CG_DrawPic( x2+60, y2+30-2,460, height, background);	// Bottom half

		}
		else
		{
			xAdd = 0;
		}

		cgi_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, cgs.media.weaponProngsOff);
		CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, cgs.media.weaponProngsOff);

		return;
	}

	prongLeftX =x2+37;
	prongRightX =x2+544;

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

	cgi_R_SetColor( colorTable[CT_WHITE] );
	height = (int) (60.0f*cg.iconHUDPercent);
	CG_DrawPic( x2+60, y2+30, 460, -height, background);	// Top half
	CG_DrawPic( x2+60, y2+30-2,460, height, background);	// Bottom half


	// And now for the prongs
	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_INVENTORY;
		background = cgs.media.inventoryProngsOn;
	}
	else if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)>cg.time)
	{
		cgs.media.currentBackground = ICON_WEAPONS;
		background = cgs.media.weaponProngsOn;
	}
	else
	{
		cgs.media.currentBackground = ICON_FORCE;
		background = cgs.media.forceProngsOn;
	}


	// Side Prongs
	cgi_R_SetColor( colorTable[CT_WHITE]);
	xAdd = (int) 8*cg.iconHUDPercent;
	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, background);
	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, background);
}

int cgi_UI_GetItemText(char *menuFile,char *itemName, char *text);

char *weaponDesc[13] =
{
"SABER_DESC",
"BLASTER_PISTOL_DESC",
"BLASTER_RIFLE_DESC",
"DISRUPTOR_RIFLE_DESC",
"BOWCASTER_DESC",
"HEAVYREPEATER_DESC",
"DEMP2_DESC",
"FLECHETTE_DESC",
"MERR_SONN_DESC",
"THERMAL_DETONATOR_DESC",
"TRIP_MINE_DESC",
"DET_PACK_DESC",
"STUN_BATON_DESC",
};


/*
===================
CG_DrawDataPadWeaponSelect
===================
*/
void CG_DrawDataPadWeaponSelect( void )
{
	int				i;
	int				bits;
	int				count;
	int				smallIconSize,bigIconSize;
	int				holdX,x,y,pad;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				sideMax,holdCount,iconCnt;
	vec4_t			calcColor;
	char			text[1024]={0};
	vec4_t			textColor = { .875f, .718f, .121f, 1.0f };

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	bits = cg.snap->ps.stats[ STAT_WEAPONS ];

	// count the number of weapons owned
	count = 0;
	for ( i = 1 ; i < 16 ; i++ )
	{
		if ( bits & ( 1 << i ) )
		{
			count++;
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

	// This seems to be a problem is datapad comes up too early
	if (cg.DataPadWeaponSelect<FIRST_WEAPON)
	{
		cg.DataPadWeaponSelect = FIRST_WEAPON;
	}
	else if (cg.DataPadWeaponSelect>MAX_PLAYER_WEAPONS)
	{
		cg.DataPadWeaponSelect = MAX_PLAYER_WEAPONS;
	}

	i = cg.DataPadWeaponSelect - 1;
	if (i<1)
	{
		i = 13;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 8;

	x = 320;
	y = 300;

	// Background
	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	calcColor[3] = .60f;
	cgi_R_SetColor( calcColor);

	// Left side ICONS
	cgi_R_SetColor( calcColor);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;

	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i<1)
		{
			i = 13;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (weaponData[i].weaponIcon[0])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
			weaponInfo = &cg_weapons[i];

			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo );
			}
			else
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIcon );
			}

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	cgi_R_SetColor(NULL);

//	char buffer[256];
//	cgi_UI_GetItemText("datapadWeaponsMenu",va("weapondesc%d",cg.DataPadWeaponSelect+1),buffer);

	if (weaponData[cg.DataPadWeaponSelect].weaponIcon[0])
	{
		weaponInfo_t	*weaponInfo;
		CG_RegisterWeapon( cg.DataPadWeaponSelect );
		weaponInfo = &cg_weapons[cg.DataPadWeaponSelect];

		if (!CG_WeaponCheck(cg.DataPadWeaponSelect))
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, weaponInfo->weaponIconNoAmmo );
		}
		else
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, weaponInfo->weaponIcon );
		}
	}

	i = cg.DataPadWeaponSelect + 1;
	if (i> 13)
	{
		i = 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	cgi_R_SetColor( calcColor);
	holdX = x + (bigIconSize/2) + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>13)
		{
			i = 1;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (weaponData[i].weaponIcon[0])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
			weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?
			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo );
			}
			else
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIcon );
			}


			holdX += (smallIconSize+pad);
		}
	}

	// draw the weapon description
	x= 40;
	y= 70;

	cgi_SP_GetStringTextString( va("INGAME_%s",weaponDesc[cg.DataPadWeaponSelect-1]), text, sizeof(text) );

	if (text[0])
	{
		CG_DisplayBoxedText(70,50,500,300,text,
												cgs.media.qhFontSmall,
												0.7f,
												textColor
												);
	}

/*	CG_DisplayBoxedText(70,50,500,300,weaponDesc[cg.DataPadWeaponSelect-1],
												cgs.media.qhFontSmall,
												0.7f,
												colorTable[CT_WHITE]
												);
*/

	cgi_R_SetColor( NULL );
}

/*
===================
CG_DrawDataPadIconBackground
===================
*/
void CG_DrawDataPadIconBackground(int backgroundType)
{
	int				height,xAdd,x2,y2;
	int				prongLeftX,prongRightX;
	qhandle_t		background;

	x2 = 0;
	y2 = 295;

	prongLeftX =x2+97;
	prongRightX =x2+544;

	if (backgroundType == ICON_INVENTORY)	// Display inventory background?
	{
		background = cgs.media.inventoryIconBackground;
	}
	else if (backgroundType == ICON_WEAPONS)	// Display weapon background?
	{
		background = cgs.media.weaponIconBackground;
	}
	else 	// Display force background?
	{
		background = cgs.media.forceIconBackground;
	}

/*	if ((cg.iconDataPadSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		if (cg.iconDataPadHUDActive)		// The time is up, but we still need to move the prongs back to their original position
		{
			t =  cg.time - (cg.iconDataPadSelectTime+WEAPON_SELECT_TIME);
			cg.iconDataPadHUDPercent = t/ 130.0f;
			cg.iconDataPadHUDPercent = 1 - cg.iconDataPadHUDPercent;

			if (cg.iconDataPadHUDPercent<0)
			{
				cg.iconDataPadHUDActive = qfalse;
				cg.iconDataPadHUDPercent=0;
			}

			xAdd = (int) 8*cg.iconDataPadHUDPercent;

			height = (int) (60.0f*cg.iconDataPadHUDPercent);
			CG_DrawPic( x2+60, y2+30, 460, -height, background);	// Top half
			CG_DrawPic( x2+60, y2+30-2,460, height, background);	// Bottom half

		}
		else
		{
			xAdd = 0;
		}

		cgi_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, cgs.media.weaponProngsOff);
		CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, cgs.media.weaponProngsOff);

		return;
	}
*/

	prongLeftX =x2+97;
	prongRightX =x2+544;
/*
	if (!cg.iconDataPadHUDActive)
	{
		t = cg.time - cg.iconDataPadSelectTime;
		cg.iconDataPadHUDPercent = t/ 130.0f;

		// Calc how far into opening sequence we are
		if (cg.iconDataPadHUDPercent>1)
		{
			cg.iconDataPadHUDActive = qtrue;
			cg.iconDataPadHUDPercent=1;
		}
		else if (cg.iconDataPadHUDPercent<0)
		{
			cg.iconDataPadHUDPercent=0;
		}
	}
	else
	{
		cg.iconDataPadHUDPercent=1;
	}
*/
	cgi_R_SetColor( colorTable[CT_WHITE] );
//	height = (int) (60.0f*cg.iconDataPadHUDPercent);
	height = (int) 60.0f;
	CG_DrawPic( x2+110, y2+30, 410, -height, background);	// Top half
	CG_DrawPic( x2+110, y2+30-2,410, height, background);	// Bottom half

	// And now for the prongs
	if (backgroundType==ICON_INVENTORY)
	{
		cgs.media.currentDataPadIconBackground = ICON_INVENTORY;
		background = cgs.media.inventoryProngsOn;
	}
	else if (backgroundType==ICON_WEAPONS)
	{
		cgs.media.currentDataPadIconBackground = ICON_WEAPONS;
		background = cgs.media.weaponProngsOn;
	}
	else
	{
		cgs.media.currentDataPadIconBackground = ICON_FORCE;
		background = cgs.media.forceProngsOn;
	}

	// Side Prongs
	cgi_R_SetColor( colorTable[CT_WHITE]);
//	xAdd = (int) 8*cg.iconDataPadHUDPercent;
	xAdd = (int) 8;
	CG_DrawPic( prongLeftX+xAdd, y2-10, 40, 80, background);
	CG_DrawPic( prongRightX-xAdd, y2-10, -40, 80, background);
}

/*
===============
SetWeaponSelectTime
===============
*/
void SetWeaponSelectTime(void)
{

	if (((cg.inventorySelectTime + WEAPON_SELECT_TIME) > cg.time) ||	// The Inventory HUD was currently active to just swap it out with Force HUD
		((cg.forcepowerSelectTime + WEAPON_SELECT_TIME) > cg.time))		// The Force HUD was currently active to just swap it out with Force HUD
	{
		cg.inventorySelectTime = 0;
		cg.forcepowerSelectTime = 0;
		cg.weaponSelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.weaponSelectTime = cg.time;
	}
}

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void )
{
	int		i;
	int		bits;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX,x,y,x2,y2,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	//int		height;
	vec4_t	calcColor;
	vec4_t	textColor = { .875f, .718f, .121f, 1.0f };

	if (!cgi_UI_GetMenuInfo("weaponselecthud",&x2,&y2))
	{
		return;
	}

	if ((cg.weaponSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	// don't display if dead
	if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	cg.iconSelectTime = cg.weaponSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	//cg.itemPickupTime = 0;

	bits = cg.snap->ps.stats[ STAT_WEAPONS ];

	// count the number of weapons owned
	count = 0;
	for ( i = 1 ; i < 16 ; i++ )
	{
		if ( bits & ( 1 << i ) )
		{
			count++;
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

	i = cg.weaponSelect - 1;
	if (i<1)
	{
		i = 13;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 12;

	x = 320;
	y = 410;

	// Background
	memcpy(calcColor, colorTable[CT_WHITE], sizeof(vec4_t));
	calcColor[3] = .60f;
	cgi_R_SetColor( calcColor);

	// Left side ICONS
	cgi_R_SetColor( calcColor);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;

	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i<1)
		{
			i = 13;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (weaponData[i].weaponIcon[0])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
			weaponInfo = &cg_weapons[i];

			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo );
			}
			else
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIcon );
			}

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	cgi_R_SetColor(NULL);
	if (weaponData[cg.weaponSelect].weaponIcon[0])
	{
		weaponInfo_t	*weaponInfo;
		CG_RegisterWeapon( cg.weaponSelect );
		weaponInfo = &cg_weapons[cg.weaponSelect];

		if (!CG_WeaponCheck(cg.weaponSelect))
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, weaponInfo->weaponIconNoAmmo );
		}
		else
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, weaponInfo->weaponIcon );
		}
	}

	i = cg.weaponSelect + 1;
	if (i> 13)
	{
		i = 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	cgi_R_SetColor( calcColor);
	holdX = x + (bigIconSize/2) + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>13)
		{
			i = 1;
		}

		if ( !(bits & ( 1 << i )))	// Does he have this weapon?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (weaponData[i].weaponIcon[0])
		{
			weaponInfo_t	*weaponInfo;
			CG_RegisterWeapon( i );
			weaponInfo = &cg_weapons[i];
			// No ammo for this weapon?
			if (!CG_WeaponCheck(i))
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIconNoAmmo );
			}
			else
			{
				CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, weaponInfo->weaponIcon );
			}


			holdX += (smallIconSize+pad);
		}
	}

	gitem_t *item = cg_weapons[ cg.weaponSelect ].item;

	// draw the selected name
	if ( item && item->classname && item->classname[0] )
	{
		char text[1024];

		if ( cgi_SP_GetStringTextString( va("INGAME_%s",item->classname), text, sizeof( text )))
		{
			int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
			int x = ( SCREEN_WIDTH - w ) / 2;
			cgi_R_Font_DrawString(x, (SCREEN_HEIGHT - 24), text, textColor, cgs.media.qhFontSmall, -1, 1.0f);
		}
	}

	cgi_R_SetColor( NULL );
}


/*
===============
CG_WeaponSelectable
===============
*/
qboolean CG_WeaponSelectable( int i, int original, qboolean dpMode )
{
	int	usage_for_weap;

	if (i > MAX_PLAYER_WEAPONS)
	{
#ifndef FINAL_BUILD
		Com_Printf("CG_WeaponSelectable() passed illegal index of %d!\n",i);
#endif
		return qfalse;
	}

	if ( cg.weaponSelectTime + 200 > cg.time )
	{//TEMP standard weapon cycle debounce for E3 because G2 can't keep up with fast weapon changes
		return qfalse;
	}

	//FIXME: this doesn't work below, can still cycle too fast!
	if ( original == WP_SABER && cg.weaponSelectTime + 500 > cg.time )
	{//when sqitch to lightsaber, have to stay there for at least half a second!
		return qfalse;
	}

	if (( weaponData[i].ammoIndex != AMMO_NONE ) && !dpMode )
	{//weapon uses ammo, see if we have any
		usage_for_weap = weaponData[i].energyPerShot < weaponData[i].altEnergyPerShot
									? weaponData[i].energyPerShot
									: weaponData[i].altEnergyPerShot;

		if ( cg.snap->ps.ammo[weaponData[i].ammoIndex] - usage_for_weap < 0 )
		{
			if ( i != WP_DET_PACK ) // detpack can be switched to...should possibly check if there are any stuck to a wall somewhere?
			{
				// This weapon doesn't have enough ammo to shoot either the main or the alt-fire
				return qfalse;
			}
		}
	}

	if (!(cg.snap->ps.stats[ STAT_WEAPONS ] & ( 1 << i )))
	{
		// Don't have this weapon to start with.
		return qfalse;
	}

	return qtrue;
}

void CG_ToggleATSTWeapon( void )
{
	if ( cg.weaponSelect == WP_ATST_MAIN )
	{
		cg.weaponSelect = WP_ATST_SIDE;
	}
	else
	{
		cg.weaponSelect = WP_ATST_MAIN;
	}
//	cg.weaponSelectTime = cg.time;
	SetWeaponSelectTime();
}

void CG_PlayerLockedWeaponSpeech( int jumping )
{
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
	static int speechDebounceTime = 0;
	if ( !in_camera )
	{//not in a cinematic
		if ( speechDebounceTime < cg.time )
		{//spoke more than 3 seconds ago
			if ( !Q3_TaskIDPending( &g_entities[0], TID_CHAN_VOICE ) )
			{//not waiting on a scripted sound to finish
				if( !jumping )
				{
					if( Q_flrand(0.0f, 1.0f) > 0.5 )
					{
						G_SoundOnEnt( player, CHAN_VOICE, va( "sound/chars/kyle/09kyk015.wav" ));
					}
					else
					{
						G_SoundOnEnt( player, CHAN_VOICE, va( "sound/chars/kyle/09kyk016.wav" ));
					}
				}
				else
				{
					G_SoundOnEnt( player, CHAN_VOICE, va( "sound/chars/kyle/16kyk007.wav" ));
				}
				speechDebounceTime = cg.time + 3000;
			}
		}
	}
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
	/*
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}
	*/

	if( g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS )
	{
		CG_PlayerLockedWeaponSpeech( qfalse );
		return;
	}

	if( g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST )
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if ( cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON )
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if ( cg.snap->ps.viewEntity )
	{
		// yeah, probably need a better check here
		if ( g_entities[cg.snap->ps.viewEntity].client && ( g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R5D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE ))
		{
			return;
		}
	}

	original = cg.weaponSelect;

	for ( i = 0 ; i <= MAX_PLAYER_WEAPONS ; i++ )
	{
		cg.weaponSelect++;

		if ( cg.weaponSelect < FIRST_WEAPON || cg.weaponSelect > MAX_PLAYER_WEAPONS) {
			cg.weaponSelect = FIRST_WEAPON;
		}

		if ( CG_WeaponSelectable( cg.weaponSelect, original, qfalse ) )
		{
//			cg.weaponSelectTime = cg.time;
			SetWeaponSelectTime();
			return;
		}
	}

	cg.weaponSelect = original;
}

/*
===============
CG_DPNextWeapon_f
===============
*/
void CG_DPNextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	/*
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}
	*/

	original = cg.DataPadWeaponSelect;

	for ( i = 0 ; i <= MAX_PLAYER_WEAPONS ; i++ )
	{
		cg.DataPadWeaponSelect++;

		if ( cg.DataPadWeaponSelect < FIRST_WEAPON || cg.DataPadWeaponSelect > MAX_PLAYER_WEAPONS) {
			cg.DataPadWeaponSelect = FIRST_WEAPON;
		}

		if ( CG_WeaponSelectable( cg.DataPadWeaponSelect, original, qtrue ) )
		{
			return;
		}
	}

	cg.DataPadWeaponSelect = original;
}

/*
===============
CG_DPPrevWeapon_f
===============
*/
void CG_DPPrevWeapon_f( void )
{
	int		i;
	int		original;

	if ( !cg.snap )
	{
		return;
	}

	/*
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		return;
	}
	*/

	original = cg.DataPadWeaponSelect;

	for ( i = 0 ; i <= MAX_PLAYER_WEAPONS ; i++ )
	{
		cg.DataPadWeaponSelect--;
		if ( cg.DataPadWeaponSelect < FIRST_WEAPON || cg.DataPadWeaponSelect > MAX_PLAYER_WEAPONS)
		{
			cg.DataPadWeaponSelect = MAX_PLAYER_WEAPONS;
		}

		if ( CG_WeaponSelectable( cg.DataPadWeaponSelect, original, qtrue ) )
		{
			return;
		}
	}

	cg.DataPadWeaponSelect = original;
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
	/*
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}
	*/

	if( g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS )
	{
		CG_PlayerLockedWeaponSpeech( qfalse );
		return;
	}

	if( g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST )
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if ( cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON )
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if ( cg.snap->ps.viewEntity )
	{
		// yeah, probably need a better check here
		if ( g_entities[cg.snap->ps.viewEntity].client && ( g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R5D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE ))
		{
			return;
		}
	}

	original = cg.weaponSelect;

	for ( i = 0 ; i <= MAX_PLAYER_WEAPONS ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect < FIRST_WEAPON || cg.weaponSelect > MAX_PLAYER_WEAPONS) {
			cg.weaponSelect = MAX_PLAYER_WEAPONS;
		}

		if ( CG_WeaponSelectable( cg.weaponSelect, original, qfalse ) )
		{
			SetWeaponSelectTime();
//			cg.weaponSelectTime = cg.time;
			return;
		}
	}

	cg.weaponSelect = original;
}

/*
void CG_ChangeWeapon( int num )

  Meant to be called from the normal game, so checks the game-side weapon inventory data
*/
void CG_ChangeWeapon( int num )
{
	gentity_t	*player = &g_entities[0];

	if ( num < WP_NONE || num >= WP_NUM_WEAPONS )
	{
		return;
	}

	if( player->flags & FL_LOCK_PLAYER_WEAPONS )
	{
		CG_PlayerLockedWeaponSpeech( qfalse );
		return;
	}

	if ( player->client != NULL && !(player->client->ps.stats[STAT_WEAPONS] & ( 1 << num )) )
	{
		return;		// don't have the weapon
	}

	// because we don't have an empty hand model for the thermal, don't allow selecting that weapon if it has no ammo
	if ( num == WP_THERMAL )
	{
		if ( cg.snap->ps.ammo[AMMO_THERMAL] <= 0 )
		{
			return;
		}
	}

	// because we don't have an empty hand model for the thermal, don't allow selecting that weapon if it has no ammo
	if ( num == WP_TRIP_MINE )
	{
		if ( cg.snap->ps.ammo[AMMO_TRIPMINE] <= 0 )
		{
			return;
		}
	}

	SetWeaponSelectTime();
//	cg.weaponSelectTime = cg.time;
	cg.weaponSelect = num;
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void )
{
	int		num;

	if ( cg.weaponSelectTime + 200 > cg.time )
	{
		return;
	}

	if ( !cg.snap ) {
		return;
	}
	/*
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}
	*/

	if( g_entities[0].flags & FL_LOCK_PLAYER_WEAPONS )
	{
		CG_PlayerLockedWeaponSpeech( qfalse );
		return;
	}

	if( g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST )
	{
		CG_ToggleATSTWeapon();
		return;
	}

	if ( cg.snap->ps.eFlags & EF_LOCKED_TO_WEAPON )
	{
		// can't do any sort of weapon switching when in the emplaced gun
		return;
	}

	if ( cg.snap->ps.viewEntity )
	{
		// yeah, probably need a better check here
		if ( g_entities[cg.snap->ps.viewEntity].client && ( g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R5D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_R2D2
				|| g_entities[cg.snap->ps.viewEntity].client->NPC_class == CLASS_MOUSE ))
		{
			return;
		}
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < WP_NONE || num >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( num == WP_SABER )
	{//lightsaber
		if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) )
		{//don't have saber, try stun baton
			num = WP_STUN_BATON;
		}
		else if ( num == cg.snap->ps.weapon )
		{//already have it up, let's try to toggle it
			if ( !in_camera )
			{//player can't activate/deactivate saber when in a cinematic
				//can't toggle it if not holding it and not controlling it or dead
				if ( cg.predicted_player_state.stats[STAT_HEALTH] > 0 && (!cg_entities[0].currentState.saberInFlight || (&g_entities[cg_entities[0].gent->client->ps.saberEntityNum] != NULL && g_entities[cg_entities[0].gent->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR) ) )
				{//it's either in-hand or it's under telekinetic control
					if ( cg_entities[0].currentState.saberActive )
					{
						cg_entities[0].gent->client->ps.saberActive = qfalse;
						if ( cg_entities[0].currentState.saberInFlight )
						{//play it on the saber
							cgi_S_UpdateEntityPosition( cg_entities[0].gent->client->ps.saberEntityNum, g_entities[cg_entities[0].gent->client->ps.saberEntityNum].currentOrigin );
							cgi_S_StartSound (NULL, cg_entities[0].gent->client->ps.saberEntityNum, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/saber/saberoff.wav" ) );
						}
						else
						{
							cgi_S_StartSound (NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/saber/saberoff.wav" ) );
						}
					}
					else
					{
						cg_entities[0].gent->client->ps.saberActive = qtrue;
					}
				}
			}
		}
	}
	else if ( num >= WP_THERMAL && num <= WP_DET_PACK ) // these weapons cycle
	{
		int weap, i = 0;

		if ( cg.snap->ps.weapon >= WP_THERMAL && cg.snap->ps.weapon <= WP_DET_PACK )
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
			if ( weap > WP_DET_PACK )
			{
				weap = WP_THERMAL;
			}

			if ( cg.snap->ps.ammo[weaponData[weap].ammoIndex] > 0 || weap == WP_DET_PACK )
			{
				if ( CG_WeaponSelectable( weap, cg.snap->ps.weapon, qfalse ) )
				{
					num = weap;
					break;
				}
			}

			weap++;
			i++;
		}
	}

	if ( ! ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	SetWeaponSelectTime();
//	cg.weaponSelectTime = cg.time;
	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;
	int		original;

	if ( cg.weaponSelectTime + 200 > cg.time )
		return;

	if( g_entities[0].client && g_entities[0].client->NPC_class == CLASS_ATST )
	{
		CG_ToggleATSTWeapon();
		return;
	}

	original = cg.weaponSelect;

	for ( i = WP_ROCKET_LAUNCHER; i > 0 ; i-- )
	{
		// We don't want the emplaced, melee, or explosive devices here
		if ( original != i && CG_WeaponSelectable( i, original, qfalse ) )
		{
			SetWeaponSelectTime();
			cg.weaponSelect = i;
			break;
		}
	}

	if ( cg_autoswitch.integer != 1 )
	{
		// didn't have that, so try these. Start with thermal...
		for ( i = WP_THERMAL; i <= WP_DET_PACK; i++ )
		{
			// We don't want the emplaced, or melee here
			if ( original != i && CG_WeaponSelectable( i, original, qfalse ) )
			{
				if ( i == WP_DET_PACK && cg.snap->ps.ammo[weaponData[i].ammoIndex] <= 0 )
				{
					// crap, no point in switching to this
				}
				else
				{
					SetWeaponSelectTime();
					cg.weaponSelect = i;
					break;
				}
			}
		}
	}

	// try stun baton as a last ditch effort
	if ( CG_WeaponSelectable( WP_STUN_BATON, original, qfalse ))
	{
		SetWeaponSelectTime();
		cg.weaponSelect = WP_STUN_BATON;
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean alt_fire )
{
	entityState_t *ent;
	//weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	//weap = &cg_weapons[ ent->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;
	cent->altFire = alt_fire;

	// lightning type guns only does this this on initial press
	if ( ent->weapon == WP_SABER )
	{
		if ( cent->pe.lightningFiring )
		{
/*			if ( ent->weapon == WP_DREADNOUGHT )
			{
				cgi_FF_EnsureFX( fffx_Laser3 );
			}
*/
			return;
		}
	}

	// Do overcharge sound that get's added to the top
/*	if (( ent->powerups & ( 1<<PW_WEAPON_OVERCHARGE )))
	{
		if ( alt_fire )
		{
			switch( ent->weapon )
			{
			case WP_THERMAL:
			case WP_DET_PACK:
			case WP_TRIP_MINE:
			case WP_ROCKET_LAUNCHER:
			case WP_FLECHETTE:
				// these weapon fires don't overcharge
				break;

			case WP_BLASTER:
				cgi_S_StartSound( NULL, ent->number, CHAN_AUTO, cgs.media.overchargeFastSound );
				break;

			default:
				cgi_S_StartSound( NULL, ent->number, CHAN_AUTO, cgs.media.overchargeSlowSound );
				break;
			}
		}
		else
		{
			switch( ent->weapon )
			{
			case WP_THERMAL:
			case WP_DET_PACK:
			case WP_TRIP_MINE:
			case WP_ROCKET_LAUNCHER:
				// these weapon fires don't overcharge
				break;

			case WP_REPEATER:
				cgi_S_StartSound( NULL, ent->number, CHAN_AUTO, cgs.media.overchargeFastSound );
				break;

			default:
				cgi_S_StartSound( NULL, ent->number, CHAN_AUTO, cgs.media.overchargeSlowSound );
				break;
			}
		}
	}*/
}

/*
=================
CG_BounceEffect

Caused by an EV_BOUNCE | EV_BOUNCE_HALF event
=================
*/
void CG_BounceEffect( centity_t *cent, int weapon, vec3_t origin, vec3_t normal )
{
	switch( weapon )
	{
	case WP_THERMAL:
		if ( rand() & 1 ) {
			cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce1 );
		} else {
			cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce2 );
		}
		break;

	case WP_BOWCASTER:
		theFxScheduler.PlayEffect( cgs.effects.bowcasterBounceEffect, origin, normal );
		break;

	case WP_FLECHETTE:
		theFxScheduler.PlayEffect( "flechette/ricochet", origin, normal );
		break;

	default:
		if ( rand() & 1 ) {
			cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce1 );
		} else {
			cgi_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.grenadeBounce2 );
		}
		break;
	}
}

//----------------------------------------------------------------------
void CG_MissileStick( centity_t *cent, int weapon, vec3_t position )
//----------------------------------------------------------------------
{
	sfxHandle_t snd = 0;

	switch( weapon )
	{
	case WP_FLECHETTE:
		snd = cgs.media.flechetteStickSound;
		break;

	case WP_DET_PACK:
		snd = cgs.media.detPackStickSound;
		break;

	case WP_TRIP_MINE:
		snd = cgs.media.tripMineStickSound;
		break;
	}

	if ( snd )
	{
		cgi_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, snd );
	}
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, qboolean altFire )
{
	int parm;

	switch( weapon )
	{
	case WP_BRYAR_PISTOL:
		if ( altFire )
		{
			parm = 0;

			if ( cent->gent )
			{
				parm += cent->gent->count;
			}

			FX_BryarAltHitWall( origin, dir, parm );
		}
		else
		{
			FX_BryarHitWall( origin, dir );
		}
		break;

	case WP_BLASTER:
		FX_BlasterWeaponHitWall( origin, dir );
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
		if ( altFire )
		{
		}
		else
		{
			FX_DEMP2_HitWall( origin, dir );
		}
		break;

	case WP_FLECHETTE:
		if ( altFire )
		{
			theFxScheduler.PlayEffect( "flechette/alt_blow", origin, dir );
		}
		else
		{
			FX_FlechetteWeaponHitWall( origin, dir );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitWall( origin, dir );
		break;

	case WP_THERMAL:
		theFxScheduler.PlayEffect( "thermal/explosion", origin, dir );
		theFxScheduler.PlayEffect( "thermal/shockwave", origin );
		break;

	case WP_EMPLACED_GUN:
		FX_EmplacedHitWall( origin, dir );
		break;

	case WP_ATST_MAIN:
		FX_ATSTMainHitWall( origin, dir );
		break;

	case WP_ATST_SIDE:
		if ( altFire )
		{
			theFxScheduler.PlayEffect( "atst/side_alt_explosion", origin, dir );
		}
		else
		{
			theFxScheduler.PlayEffect( "atst/side_main_impact", origin, dir );
		}
		break;

	case WP_TRIP_MINE:
		theFxScheduler.PlayEffect( "tripmine/explosion", origin, dir );
		break;

	case WP_DET_PACK:
		theFxScheduler.PlayEffect( "detpack/explosion", origin, dir );
		break;

	case WP_TURRET:
		theFxScheduler.PlayEffect( "turret/wall_impact", origin, dir );
		break;
	}
}

/*
-------------------------
CG_MissileHitPlayer
-------------------------
*/

void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, qboolean altFire )
{
	gentity_t *other = NULL;
	qboolean	humanoid = qtrue;

	if ( cent->gent )
	{
		other = &g_entities[cent->gent->s.otherEntityNum];
		if( other->client )
		{
			class_t	npc_class = other->client->NPC_class;
			// check for all droids, maybe check for certain monsters if they're considered non-humanoid..?
			if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
				 npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
				 npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
				 npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
			{
				humanoid = qfalse;
			}
		}
	}

	switch( weapon )
	{
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

	case WP_BLASTER:
		FX_BlasterWeaponHitPlayer( origin, dir, humanoid );
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
		if ( !altFire )
		{
			FX_DEMP2_HitPlayer( origin, dir, humanoid );
		}

		// Do a full body effect here for some more feedback
		if ( other && other->client )
		{
			other->s.powerups |= ( 1 << PW_SHOCKED );
			other->client->ps.powerups[PW_SHOCKED] = cg.time + 1000;
		}
		break;

	case WP_FLECHETTE:
		if ( altFire )
		{
			theFxScheduler.PlayEffect( "flechette/alt_blow", origin, dir );
		}
		else
		{
			FX_FlechetteWeaponHitPlayer( origin, dir, humanoid );
		}
		break;

	case WP_ROCKET_LAUNCHER:
		FX_RocketHitPlayer( origin, dir, humanoid );
		break;

	case WP_THERMAL:
		theFxScheduler.PlayEffect( "thermal/explosion", origin, dir );
		theFxScheduler.PlayEffect( "thermal/shockwave", origin );
		break;

	case WP_EMPLACED_GUN:
		FX_EmplacedHitWall( origin, dir );
		break;

	case WP_TRIP_MINE:
		theFxScheduler.PlayEffect( "tripmine/explosion", origin, dir );
		break;

	case WP_DET_PACK:
		theFxScheduler.PlayEffect( "detpack/explosion", origin, dir );
		break;

	case WP_TURRET:
		theFxScheduler.PlayEffect( "turret/flesh_impact", origin, dir );
		break;

	case WP_ATST_MAIN:
		FX_EmplacedHitWall( origin, dir );
		break;

	case WP_ATST_SIDE:
		if ( altFire )
		{
			theFxScheduler.PlayEffect( "atst/side_alt_explosion", origin, dir );
		}
		else
		{
			theFxScheduler.PlayEffect( "atst/side_main_impact", origin, dir );
		}
		break;
	}
}
