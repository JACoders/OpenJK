//
// cg_weaponinit.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel( item->world_model[0] );
	// load in-view model also
	weaponInfo->viewModel = trap_R_RegisterModel(item->view_model);

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel( ammo->world_model[0] );
	}

//	strcpy( path, item->view_model );
//	COM_StripExtension( path, path );
//	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = 0;//trap_R_RegisterModel( path );

	if (weaponNum == WP_DISRUPTOR ||
		weaponNum == WP_FLECHETTE ||
		weaponNum == WP_REPEATER ||
		weaponNum == WP_ROCKET_LAUNCHER)
	{
		strcpy( path, item->view_model );
		COM_StripExtension( path, path );
		strcat( path, "_barrel.md3" );
		weaponInfo->barrelModel = trap_R_RegisterModel( path );
	}
	else if (weaponNum == WP_STUN_BATON)
	{ //only weapon with more than 1 barrel..
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel.md3");
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel2.md3");
		trap_R_RegisterModel("models/weapons2/stun_baton/baton_barrel3.md3");
	}
	else
	{
		weaponInfo->barrelModel = 0;
	}

	if (weaponNum != WP_SABER)
	{
		strcpy( path, item->view_model );
		COM_StripExtension( path, path );
		strcat( path, "_hand.md3" );
		weaponInfo->handsModel = trap_R_RegisterModel( path );
	}
	else
	{
		weaponInfo->handsModel = 0;
	}

//	if ( !weaponInfo->handsModel ) {
//		weaponInfo->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
//	}

	switch ( weaponNum ) {
	case WP_STUN_BATON:
	case WP_MELEE:
/*		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/saber/saberhum.wav" );
//		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/melee/fstatck.wav" );
*/
		//trap_R_RegisterShader( "gfx/effects/stunPass" );
		trap_FX_RegisterEffect( "stunBaton/flesh_impact" );

		if (weaponNum == WP_STUN_BATON)
		{
			trap_S_RegisterSound( "sound/weapons/baton/idle.wav" );
			weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/baton/fire.mp3" );
			weaponInfo->altFlashSound[0] = trap_S_RegisterSound( "sound/weapons/baton/fire.mp3" );
		}
		else
		{
			/*
			int j = 0;

			while (j < 4)
			{
				weaponInfo->flashSound[j] = trap_S_RegisterSound( va("sound/weapons/melee/swing%i", j+1) );
				weaponInfo->altFlashSound[j] = weaponInfo->flashSound[j];
				j++;
			}
			*/
			//No longer needed, animsound config plays them for us
		}
		break;
	case WP_SABER:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap_S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/saber/saber_w.glm" );
		break;

	case WP_CONCUSSION:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/concussion/select.wav");

		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "concussion/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_ConcussionProjectileThink;

		weaponInfo->altFlashSound[0]	= NULL_SOUND;
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "concussion/altmuzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_ConcussionProjectileThink;

		cgs.effects.disruptorAltMissEffect		= trap_FX_RegisterEffect( "disruptor/alt_miss" );

		cgs.effects.concussionShotEffect		= trap_FX_RegisterEffect( "concussion/shot" );
		cgs.effects.concussionImpactEffect		= trap_FX_RegisterEffect( "concussion/explosion" );
		trap_R_RegisterShader("gfx/effects/blueLine");
		trap_R_RegisterShader("gfx/misc/whiteline2");
		break;

	case WP_BRYAR_PISTOL:
	case WP_BRYAR_OLD:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/bryar/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/bryar/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		//weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_BryarProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "bryar/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		//weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;

		cgs.effects.bryarShotEffect			= trap_FX_RegisterEffect( "bryar/shot" );
		cgs.effects.bryarPowerupShotEffect	= trap_FX_RegisterEffect( "bryar/crackleShot" );
		cgs.effects.bryarWallImpactEffect	= trap_FX_RegisterEffect( "bryar/wall_impact" );
		cgs.effects.bryarWallImpactEffect2	= trap_FX_RegisterEffect( "bryar/wall_impact2" );
		cgs.effects.bryarWallImpactEffect3	= trap_FX_RegisterEffect( "bryar/wall_impact3" );
		cgs.effects.bryarFleshImpactEffect	= trap_FX_RegisterEffect( "bryar/flesh_impact" );
		cgs.effects.bryarDroidImpactEffect	= trap_FX_RegisterEffect( "bryar/droid_impact" );

		cgs.media.bryarFrontFlash = trap_R_RegisterShader( "gfx/effects/bryarFrontFlash" );

		// Note these are temp shared effects
		trap_FX_RegisterEffect("blaster/wall_impact.efx");
		trap_FX_RegisterEffect("blaster/flesh_impact.efx");

		break;

	case WP_BLASTER:
	case WP_EMPLACED_GUN: //rww - just use the same as this for now..
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/blaster/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/blaster/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_BlasterProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/blaster/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "blaster/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BlasterProjectileThink;

		trap_FX_RegisterEffect( "blaster/deflect" );
		cgs.effects.blasterShotEffect			= trap_FX_RegisterEffect( "blaster/shot" );
		cgs.effects.blasterWallImpactEffect		= trap_FX_RegisterEffect( "blaster/wall_impact" );
		cgs.effects.blasterFleshImpactEffect	= trap_FX_RegisterEffect( "blaster/flesh_impact" );
		cgs.effects.blasterDroidImpactEffect	= trap_FX_RegisterEffect( "blaster/droid_impact" );
		break;

	case WP_DISRUPTOR:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/disruptor/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/disruptor/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/disruptor/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound("sound/weapons/disruptor/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		cgs.effects.disruptorRingsEffect		= trap_FX_RegisterEffect( "disruptor/rings" );
		cgs.effects.disruptorProjectileEffect	= trap_FX_RegisterEffect( "disruptor/projectile" );
		cgs.effects.disruptorWallImpactEffect	= trap_FX_RegisterEffect( "disruptor/wall_impact" );
		cgs.effects.disruptorFleshImpactEffect	= trap_FX_RegisterEffect( "disruptor/flesh_impact" );
		cgs.effects.disruptorAltMissEffect		= trap_FX_RegisterEffect( "disruptor/alt_miss" );
		cgs.effects.disruptorAltHitEffect		= trap_FX_RegisterEffect( "disruptor/alt_hit" );

		trap_R_RegisterShader( "gfx/effects/redLine" );
		trap_R_RegisterShader( "gfx/misc/whiteline2" );
		trap_R_RegisterShader( "gfx/effects/smokeTrail" );

		trap_S_RegisterSound("sound/weapons/disruptor/zoomstart.wav");
		trap_S_RegisterSound("sound/weapons/disruptor/zoomend.wav");

		// Disruptor gun zoom interface
		cgs.media.disruptorMask			= trap_R_RegisterShader( "gfx/2d/cropCircle2");
		cgs.media.disruptorInsert		= trap_R_RegisterShader( "gfx/2d/cropCircle");
		cgs.media.disruptorLight		= trap_R_RegisterShader( "gfx/2d/cropCircleGlow" );
		cgs.media.disruptorInsertTick	= trap_R_RegisterShader( "gfx/2d/insertTick" );
		cgs.media.disruptorChargeShader	= trap_R_RegisterShaderNoMip("gfx/2d/crop_charge");

		cgs.media.disruptorZoomLoop		= trap_S_RegisterSound( "sound/weapons/disruptor/zoomloop.wav" );
		break;

	case WP_BOWCASTER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/bowcaster/select.wav");

		weaponInfo->altFlashSound[0]		= trap_S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->altFiringSound			= NULL_SOUND;
		weaponInfo->altChargeSound			= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight		= 0;
//		weaponInfo->altMissileDlightColor	= {0,0,0};
		weaponInfo->altMissileHitSound		= NULL_SOUND;
		weaponInfo->altMissileTrailFunc	= FX_BowcasterProjectileThink;

		weaponInfo->flashSound[0]	= trap_S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->firingSound		= NULL_SOUND;
		weaponInfo->chargeSound		= trap_S_RegisterSound( "sound/weapons/bowcaster/altcharge.wav");
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight	= 0;
//		weaponInfo->missileDlightColor= {0,0,0};
		weaponInfo->missileHitSound	= NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BowcasterAltProjectileThink;

		cgs.effects.bowcasterShotEffect		= trap_FX_RegisterEffect( "bowcaster/shot" );
		cgs.effects.bowcasterImpactEffect	= trap_FX_RegisterEffect( "bowcaster/explosion" );

		trap_FX_RegisterEffect( "bowcaster/deflect" );

		cgs.media.greenFrontFlash = trap_R_RegisterShader( "gfx/effects/greenFrontFlash" );
		break;

	case WP_REPEATER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/repeater/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/repeater/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RepeaterProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "repeater/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;

		cgs.effects.repeaterProjectileEffect	= trap_FX_RegisterEffect( "repeater/projectile" );
		cgs.effects.repeaterAltProjectileEffect	= trap_FX_RegisterEffect( "repeater/alt_projectile" );
		cgs.effects.repeaterWallImpactEffect	= trap_FX_RegisterEffect( "repeater/wall_impact" );
		cgs.effects.repeaterFleshImpactEffect	= trap_FX_RegisterEffect( "repeater/flesh_impact" );
		//cgs.effects.repeaterAltWallImpactEffect	= trap_FX_RegisterEffect( "repeater/alt_wall_impact" );
		cgs.effects.repeaterAltWallImpactEffect	= trap_FX_RegisterEffect( "repeater/concussion" );
		break;

	case WP_DEMP2:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/demp2/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound("sound/weapons/demp2/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_DEMP2_ProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound("sound/weapons/demp2/altfire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound("sound/weapons/demp2/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		cgs.effects.demp2ProjectileEffect		= trap_FX_RegisterEffect( "demp2/projectile" );
		cgs.effects.demp2WallImpactEffect		= trap_FX_RegisterEffect( "demp2/wall_impact" );
		cgs.effects.demp2FleshImpactEffect		= trap_FX_RegisterEffect( "demp2/flesh_impact" );

		cgs.media.demp2Shell = trap_R_RegisterModel( "models/items/sphere.md3" );
		cgs.media.demp2ShellShader = trap_R_RegisterShader( "gfx/effects/demp2shell" );

		cgs.media.lightningFlash = trap_R_RegisterShader("gfx/misc/lightningFlash");
		break;

	case WP_FLECHETTE:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/flechette/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/flechette/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->missileModel		= trap_R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_FlechetteProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/flechette/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/golan_arms/projectile.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_FlechetteAltProjectileThink;

		cgs.effects.flechetteShotEffect			= trap_FX_RegisterEffect( "flechette/shot" );
		cgs.effects.flechetteAltShotEffect		= trap_FX_RegisterEffect( "flechette/alt_shot" );
		cgs.effects.flechetteWallImpactEffect	= trap_FX_RegisterEffect( "flechette/wall_impact" );
		cgs.effects.flechetteFleshImpactEffect	= trap_FX_RegisterEffect( "flechette/flesh_impact" );
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/rocket/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/rocket/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap_FX_RegisterEffect( "rocket/muzzle_flash" ); //trap_FX_RegisterEffect( "rocket/muzzle_flash2" );
		//flash2 still looks crappy with the fx bolt stuff. Because the fx bolt stuff doesn't work entirely right.
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->missileSound		= trap_S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->missileDlight		= 125;
		VectorSet(weaponInfo->missileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RocketProjectileThink;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/rocket/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap_FX_RegisterEffect( "rocket/altmuzzle_flash" );
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->altMissileSound		= trap_S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->altMissileDlight	= 125;
		VectorSet(weaponInfo->altMissileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RocketAltProjectileThink;

		cgs.effects.rocketShotEffect			= trap_FX_RegisterEffect( "rocket/shot" );
		cgs.effects.rocketExplosionEffect		= trap_FX_RegisterEffect( "rocket/explosion" );
	
		trap_R_RegisterShaderNoMip( "gfx/2d/wedge" );
		trap_R_RegisterShaderNoMip( "gfx/2d/lock" );

		trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" );
		trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" );
		break;

	case WP_THERMAL:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/thermal/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= trap_S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap_S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		cgs.effects.thermalExplosionEffect		= trap_FX_RegisterEffect( "thermal/explosion" );
		cgs.effects.thermalShockwaveEffect		= trap_FX_RegisterEffect( "thermal/shockwave" );

		cgs.media.grenadeBounce1		= trap_S_RegisterSound( "sound/weapons/thermal/bounce1.wav" );
		cgs.media.grenadeBounce2		= trap_S_RegisterSound( "sound/weapons/thermal/bounce2.wav" );

		trap_S_RegisterSound( "sound/weapons/thermal/thermloop.wav" );
		trap_S_RegisterSound( "sound/weapons/thermal/warning.wav" );

		break;

	case WP_TRIP_MINE:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/detpack/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= 0;//trap_R_RegisterModel( "models/weapons2/laser_trap/laser_trap_w.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= 0;//trap_R_RegisterModel( "models/weapons2/laser_trap/laser_trap_w.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		cgs.effects.tripmineLaserFX = trap_FX_RegisterEffect("tripMine/laserMP.efx");
		cgs.effects.tripmineGlowFX = trap_FX_RegisterEffect("tripMine/glowbit.efx");

		trap_FX_RegisterEffect( "tripMine/explosion" );
		// NOTENOTE temp stuff
		trap_S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		trap_S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		break;

	case WP_DET_PACK:
		weaponInfo->selectSound			= trap_S_RegisterSound("sound/weapons/detpack/select.wav");

		weaponInfo->flashSound[0]		= trap_S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
//		weaponInfo->missileDlightColor	= {0,0,0};
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;

		weaponInfo->altFlashSound[0]	= trap_S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
//		weaponInfo->altMissileDlightColor= {0,0,0};
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		trap_R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		trap_S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		trap_S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		trap_S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;
	case WP_TURRET:
		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_HANDLE;
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_TurretProjectileThink;

		trap_FX_RegisterEffect("effects/blaster/wall_impact.efx");
		trap_FX_RegisterEffect("effects/blaster/flesh_impact.efx");
		break;

	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav" );
		break;
	}
}
