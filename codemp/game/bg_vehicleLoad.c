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

//bg_vehicleLoad.c

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_vehicles.h"
#include "bg_weapons.h"

#ifdef _GAME
	#include "g_local.h"
#elif _CGAME
	#include "cgame/cg_local.h"
#elif UI_BUILD
	#include "ui/ui_local.h"
#endif

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

// These buffers are filled in with the same contents and then just read from in
// a few places. We only need one copy on Xbox.
#define MAX_VEH_WEAPON_DATA_SIZE 0x40000 // 0x4000
#define MAX_VEHICLE_DATA_SIZE 0x100000 // 0x10000

char	VehWeaponParms[MAX_VEH_WEAPON_DATA_SIZE];
char	VehicleParms[MAX_VEHICLE_DATA_SIZE];

void BG_ClearVehicleParseParms(void)
{
	//You can't strcat to these forever without clearing them!
	VehWeaponParms[0] = 0;
	VehicleParms[0] = 0;
}

#if defined(_GAME) || defined(_CGAME)
	//These funcs are actually shared in both projects
	extern void G_SetAnimalVehicleFunctions( vehicleInfo_t *pVehInfo );
	extern void G_SetSpeederVehicleFunctions( vehicleInfo_t *pVehInfo );
	extern void G_SetWalkerVehicleFunctions( vehicleInfo_t *pVehInfo );
	extern void G_SetFighterVehicleFunctions( vehicleInfo_t *pVehInfo );
#endif

vehWeaponInfo_t g_vehWeaponInfo[MAX_VEH_WEAPONS];
int		numVehicleWeapons = 1;//first one is null/default

vehicleInfo_t g_vehicleInfo[MAX_VEHICLES];
int		numVehicles = 0;//first one is null/default

void BG_VehicleLoadParms( void );

typedef enum {
	VF_IGNORE,
	VF_INT,
	VF_FLOAT,
	VF_STRING,	// string on disk, pointer in memory
	VF_VECTOR,
	VF_BOOL,
	VF_VEHTYPE,
	VF_ANIM,
	VF_WEAPON,	// take string, resolve into index into VehWeaponParms
	VF_MODEL,	// take the string, get the G_ModelIndex
	VF_MODEL_CLIENT,	// (cgame only) take the string, get the G_ModelIndex
	VF_EFFECT,	// take the string, get the G_EffectIndex
	VF_EFFECT_CLIENT,	// (cgame only) take the string, get the index
	VF_SHADER,	// (cgame only) take the string, call trap->R_RegisterShader
	VF_SHADER_NOMIP,// (cgame only) take the string, call trap->R_RegisterShaderNoMip
	VF_SOUND,	// take the string, get the G_SoundIndex
	VF_SOUND_CLIENT	// (cgame only) take the string, get the index
} vehFieldType_t;

typedef struct vehField_s {
	const char	*name;
	size_t		ofs;
	vehFieldType_t	type;
} vehField_t;

vehField_t vehWeaponFields[] =
{
	{"name", VWFOFS(name), VF_STRING},	//unique name of the vehicle
	{"projectile", VWFOFS(bIsProjectile), VF_BOOL},	//traceline or entity?
	{"hasGravity", VWFOFS(bHasGravity), VF_BOOL},	//if a projectile, drops
	{"ionWeapon", VWFOFS(bIonWeapon), VF_BOOL},	//disables ship shields and sends them out of control
	{"saberBlockable", VWFOFS(bSaberBlockable), VF_BOOL},	//lightsabers can deflect this projectile
	{"muzzleFX", VWFOFS(iMuzzleFX), VF_EFFECT_CLIENT},	//index of Muzzle Effect
	{"model", VWFOFS(iModel), VF_MODEL_CLIENT},	//handle to the model used by this projectile
	{"shotFX", VWFOFS(iShotFX), VF_EFFECT_CLIENT},	//index of Shot Effect
	{"impactFX", VWFOFS(iImpactFX), VF_EFFECT_CLIENT},	//index of Impact Effect
	{"g2MarkShader", VWFOFS(iG2MarkShaderHandle), VF_SHADER},	//index of shader to use for G2 marks made on other models when hit by this projectile
	{"g2MarkSize", VWFOFS(fG2MarkSize), VF_FLOAT},	//size (diameter) of the ghoul2 mark
	{"loopSound", VWFOFS(iLoopSound), VF_SOUND_CLIENT},	//index of loopSound
	{"speed", VWFOFS(fSpeed), VF_FLOAT},		//speed of projectile/range of traceline
	{"homing", VWFOFS(fHoming), VF_FLOAT},		//0.0 = not homing, 0.5 = half vel to targ, half cur vel, 1.0 = all vel to targ
	{"homingFOV", VWFOFS(fHomingFOV), VF_FLOAT},//missile will lose lock on if DotProduct of missile direction and direction to target ever drops below this (-1 to 1, -1 = never lose target, 0 = lose if ship gets behind missile, 1 = pretty much will lose it's target right away)
	{"lockOnTime", VWFOFS(iLockOnTime), VF_INT},	//0 = no lock time needed, else # of ms needed to lock on
	{"damage", VWFOFS(iDamage), VF_INT},		//damage done when traceline or projectile directly hits target
	{"splashDamage", VWFOFS(iSplashDamage), VF_INT},//damage done to ents in splashRadius of end of traceline or projectile origin on impact
	{"splashRadius", VWFOFS(fSplashRadius), VF_FLOAT},//radius that ent must be in to take splashDamage (linear fall-off)
	{"ammoPerShot", VWFOFS(iAmmoPerShot), VF_INT},//how much "ammo" each shot takes
	{"health", VWFOFS(iHealth), VF_INT},		//if non-zero, projectile can be shot, takes this much damage before being destroyed
	{"width", VWFOFS(fWidth), VF_FLOAT},		//width of traceline or bounding box of projecile (non-rotating!)
	{"height", VWFOFS(fHeight), VF_FLOAT},		//height of traceline or bounding box of projecile (non-rotating!)
	{"lifetime", VWFOFS(iLifeTime), VF_INT},	//removes itself after this amount of time
	{"explodeOnExpire", VWFOFS(bExplodeOnExpire), VF_BOOL},	//when iLifeTime is up, explodes rather than simply removing itself
};

static const size_t numVehWeaponFields = ARRAY_LEN( vehWeaponFields );

int vfieldcmp( const void *a, const void *b )
{
	return Q_stricmp( (const char *)a, ((vehField_t*)b)->name );
}

static qboolean BG_ParseVehWeaponParm( vehWeaponInfo_t *vehWeapon, const char *parmName, char *pValue )
{
	vehField_t *vehWeaponField;
	vec3_t	vec;
	byte	*b = (byte *)vehWeapon;
	int		_iFieldsRead = 0;
	vehicleType_t vehType;
	char	value[1024];

	Q_strncpyz( value, pValue, sizeof(value) );

	// Loop through possible parameters
	vehWeaponField = (vehField_t *)Q_LinearSearch( parmName, vehWeaponFields, numVehWeaponFields, sizeof( vehWeaponFields[0] ), vfieldcmp );

	if ( !vehWeaponField )
		return qfalse;

	// found it
	switch( vehWeaponField->type )
	{
	case VF_INT:
		*(int *)(b+vehWeaponField->ofs) = atoi(value);
		break;
	case VF_FLOAT:
		*(float *)(b+vehWeaponField->ofs) = atof(value);
		break;
	case VF_STRING:	// string on disk, pointer in memory
		if (!*(char **)(b+vehWeaponField->ofs))
		{ //just use 1024 bytes in case we want to write over the string
			*(char **)(b+vehWeaponField->ofs) = (char *)BG_Alloc(1024);//(char *)BG_Alloc(strlen(value));
			strcpy(*(char **)(b+vehWeaponField->ofs), value);
		}

		break;
	case VF_VECTOR:
		_iFieldsRead = sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
		//assert(_iFieldsRead==3 );
		if (_iFieldsRead!=3)
		{
			Com_Printf (S_COLOR_YELLOW"BG_ParseVehWeaponParm: VEC3 sscanf() failed to read 3 floats ('angle' key bug?)\n");
			VectorClear( vec );
		}
		((float *)(b+vehWeaponField->ofs))[0] = vec[0];
		((float *)(b+vehWeaponField->ofs))[1] = vec[1];
		((float *)(b+vehWeaponField->ofs))[2] = vec[2];
		break;
	case VF_BOOL:
		*(qboolean *)(b+vehWeaponField->ofs) = (qboolean)(atof(value)!=0);
		break;
	case VF_VEHTYPE:
		vehType = (vehicleType_t)GetIDForString( VehicleTable, value );
		*(vehicleType_t *)(b+vehWeaponField->ofs) = vehType;
		break;
	case VF_ANIM:
		{
			int anim = GetIDForString( animTable, value );
			*(int *)(b+vehWeaponField->ofs) = anim;
		}
		break;
	case VF_WEAPON:	// take string, resolve into index into VehWeaponParms
		//*(int *)(b+vehWeaponField->ofs) = VEH_VehWeaponIndexForName( value );
		break;
	case VF_MODEL:// take the string, get the G_ModelIndex
#ifdef _GAME
		*(int *)(b+vehWeaponField->ofs) = G_ModelIndex( value );
#else
		*(int *)(b+vehWeaponField->ofs) = trap->R_RegisterModel( value );
#endif
		break;
	case VF_MODEL_CLIENT:	// (MP cgame only) take the string, get the G_ModelIndex
#ifdef _GAME
		*(int *)(b+vehWeaponField->ofs) = G_ModelIndex( value );
#else
		*(int *)(b+vehWeaponField->ofs) = trap->R_RegisterModel( value );
#endif
		break;
	case VF_EFFECT:	// take the string, get the G_EffectIndex
#ifdef _GAME
		//*(int *)(b+vehWeaponField->ofs) = G_EffectIndex( value );
#elif _CGAME
		*(int *)(b+vehWeaponField->ofs) = trap->FX_RegisterEffect( value );
#endif
		break;
	case VF_EFFECT_CLIENT:	// (MP cgame only) take the string, get the index
#ifdef _GAME
		//*(int *)(b+vehWeaponField->ofs) = G_EffectIndex( value );
#elif _CGAME
		*(int *)(b+vehWeaponField->ofs) = trap->FX_RegisterEffect( value );
#endif
		break;
	case VF_SHADER:	// (cgame only) take the string, call trap_R_RegisterShader
#ifdef UI_BUILD
		*(int *)(b+vehWeaponField->ofs) = trap->R_RegisterShaderNoMip( value );
#elif CGAME
		*(int *)(b+vehWeaponField->ofs) = trap->R_RegisterShader( value );
#endif
		break;
	case VF_SHADER_NOMIP:// (cgame only) take the string, call trap_R_RegisterShaderNoMip
#if defined(_CGAME) || defined(UI_BUILD)
		*(int *)(b+vehWeaponField->ofs) = trap->R_RegisterShaderNoMip( value );
#endif
		break;
	case VF_SOUND:	// take the string, get the G_SoundIndex
#ifdef _GAME
		*(int *)(b+vehWeaponField->ofs) = G_SoundIndex( value );
#else
		*(int *)(b+vehWeaponField->ofs) = trap->S_RegisterSound( value );
#endif
		break;
	case VF_SOUND_CLIENT:	// (MP cgame only) take the string, get the index
#ifdef _GAME
		//*(int *)(b+vehWeaponField->ofs) = G_SoundIndex( value );
#else
		*(int *)(b+vehWeaponField->ofs) = trap->S_RegisterSound( value );
#endif
		break;
	default:
		//Unknown type?
		return qfalse;
	}

	return qtrue;
}

int VEH_LoadVehWeapon( const char *vehWeaponName )
{//load up specified vehWeapon and save in array: g_vehWeaponInfo
	const char	*token;
	char		parmName[128];//we'll assume that no parm name is longer than 128
	char		*value;
	const char	*p;
	vehWeaponInfo_t	*vehWeapon = NULL;

	//BG_VehWeaponSetDefaults( &g_vehWeaponInfo[0] );//set the first vehicle to default data

	//try to parse data out
	p = VehWeaponParms;

	COM_BeginParseSession("vehWeapons");

	vehWeapon = &g_vehWeaponInfo[numVehicleWeapons];
	// look for the right vehicle weapon
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, vehWeaponName ) )
		{
			break;
		}

		SkipBracedSection( &p, 0 );
	}
	if ( !p )
	{
		return qfalse;
	}

	token = COM_ParseExt( &p, qtrue );
	if ( token[0] == 0 )
	{//barf
		return VEH_WEAPON_NONE;
	}

	if ( Q_stricmp( token, "{" ) != 0 )
	{
		return VEH_WEAPON_NONE;
	}

	// parse the vehWeapon info block
	while ( 1 )
	{
		SkipRestOfLine( &p );
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing Vehicle Weapon '%s'\n", vehWeaponName );
			return VEH_WEAPON_NONE;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}
		Q_strncpyz( parmName, token, sizeof(parmName) );
		value = COM_ParseExt( &p, qtrue );
		if ( !value || !value[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: Vehicle Weapon token '%s' has no value!\n", parmName );
		}
		else
		{
			if ( !BG_ParseVehWeaponParm( vehWeapon, parmName, value ) )
			{
				Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle Weapon key/value pair '%s','%s'!\n", parmName, value );
			}
		}
	}
	if ( vehWeapon->fHoming )
	{//all lock-on weapons use these 2 sounds
#ifdef _GAME
		//Hmm, no need fo have server register this, is there?
		//G_SoundIndex( "sound/weapons/torpedo/tick.wav" );
		//G_SoundIndex( "sound/weapons/torpedo/lock.wav" );
#else
		trap->S_RegisterSound( "sound/vehicles/weapons/common/tick.wav" );
		trap->S_RegisterSound( "sound/vehicles/weapons/common/lock.wav" );
		trap->S_RegisterSound( "sound/vehicles/common/lockalarm1.wav" );
		trap->S_RegisterSound( "sound/vehicles/common/lockalarm2.wav" );
		trap->S_RegisterSound( "sound/vehicles/common/lockalarm3.wav" );
#endif
	}
	return (numVehicleWeapons++);
}

int VEH_VehWeaponIndexForName( const char *vehWeaponName )
{
	int vw;
	if ( !vehWeaponName || !vehWeaponName[0] )
	{
		Com_Printf( S_COLOR_RED"ERROR: Trying to read Vehicle Weapon with no name!\n" );
		return VEH_WEAPON_NONE;
	}
	for ( vw = VEH_WEAPON_BASE; vw < numVehicleWeapons; vw++ )
	{
		if ( g_vehWeaponInfo[vw].name
			&& Q_stricmp( g_vehWeaponInfo[vw].name, vehWeaponName ) == 0 )
		{//already loaded this one
			return vw;
		}
	}
	//haven't loaded it yet
	if ( vw >= MAX_VEH_WEAPONS )
	{//no more room!
		Com_Printf( S_COLOR_RED"ERROR: Too many Vehicle Weapons (max 16), aborting load on %s!\n", vehWeaponName );
		return VEH_WEAPON_NONE;
	}
	//we have room for another one, load it up and return the index
	//HMM... should we not even load the .vwp file until we want to?
	vw = VEH_LoadVehWeapon( vehWeaponName );
	if ( vw == VEH_WEAPON_NONE )
	{
		Com_Printf( S_COLOR_RED"ERROR: Could not find Vehicle Weapon %s!\n", vehWeaponName );
	}
	return vw;
}

vehField_t vehicleFields[] =
{
	{"name", VFOFS(name), VF_STRING},	//unique name of the vehicle

	//general data
	{"type", VFOFS(type), VF_VEHTYPE},	//what kind of vehicle
	{"numHands", VFOFS(numHands), VF_INT},	//if 2 hands, no weapons, if 1 hand, can use 1-handed weapons, if 0 hands, can use 2-handed weapons
	{"lookPitch", VFOFS(lookPitch), VF_FLOAT},	//How far you can look up and down off the forward of the vehicle
	{"lookYaw", VFOFS(lookYaw), VF_FLOAT},	//How far you can look left and right off the forward of the vehicle
	{"length", VFOFS(length), VF_FLOAT},		//how long it is - used for body length traces when turning/moving?
	{"width", VFOFS(width), VF_FLOAT},		//how wide it is - used for body length traces when turning/moving?
	{"height", VFOFS(height), VF_FLOAT},		//how tall it is - used for body length traces when turning/moving?
	{"centerOfGravity", VFOFS(centerOfGravity), VF_VECTOR},//offset from origin: {forward, right, up} as a modifier on that dimension (-1.0f is all the way back, 1.0f is all the way forward)

	//speed stats
	{"speedMax", VFOFS(speedMax), VF_FLOAT},		//top speed
	{"turboSpeed", VFOFS(turboSpeed), VF_FLOAT},	//turbo speed
	{"speedMin", VFOFS(speedMin), VF_FLOAT},		//if < 0, can go in reverse
	{"speedIdle", VFOFS(speedIdle), VF_FLOAT},		//what speed it drifts to when no accel/decel input is given
	{"accelIdle", VFOFS(accelIdle), VF_FLOAT},		//if speedIdle > 0, how quickly it goes up to that speed
	{"acceleration", VFOFS(acceleration), VF_FLOAT},	//when pressing on accelerator
	{"decelIdle", VFOFS(decelIdle), VF_FLOAT},		//when giving no input, how quickly it drops to speedIdle
	{"throttleSticks", VFOFS(throttleSticks), VF_BOOL},//if true, speed stays at whatever you accel/decel to, unless you turbo or brake
	{"strafePerc", VFOFS(strafePerc), VF_FLOAT},		//multiplier on current speed for strafing.  If 1.0f, you can strafe at the same speed as you're going forward, 0.5 is half, 0 is no strafing

	//handling stats
	{"bankingSpeed", VFOFS(bankingSpeed), VF_FLOAT},	//how quickly it pitches and rolls (not under player control)
	{"pitchLimit", VFOFS(pitchLimit), VF_FLOAT},		//how far it can roll forward or backward
	{"rollLimit", VFOFS(rollLimit), VF_FLOAT},		//how far it can roll to either side
	{"braking", VFOFS(braking), VF_FLOAT},		//when pressing on decelerator
	{"mouseYaw", VFOFS(mouseYaw), VF_FLOAT},			// The mouse yaw override.
	{"mousePitch", VFOFS(mousePitch), VF_FLOAT},			// The mouse yaw override.
	{"turningSpeed", VFOFS(turningSpeed), VF_FLOAT},	//how quickly you can turn
	{"turnWhenStopped", VFOFS(turnWhenStopped), VF_BOOL},//whether or not you can turn when not moving
	{"traction", VFOFS(traction), VF_FLOAT},		//how much your command input affects velocity
	{"friction", VFOFS(friction), VF_FLOAT},		//how much velocity is cut on its own
	{"maxSlope", VFOFS(maxSlope), VF_FLOAT},		//the max slope that it can go up with control
	{"speedDependantTurning", VFOFS(speedDependantTurning), VF_BOOL},//vehicle turns faster the faster it's going

	//durability stats
	{"mass", VFOFS(mass), VF_INT},			//for momentum and impact force (player mass is 10)
	{"armor", VFOFS(armor), VF_INT},			//total points of damage it can take
	{"shields", VFOFS(shields), VF_INT},			//energy shield damage points
	{"shieldRechargeMS", VFOFS(shieldRechargeMS), VF_INT},//energy shield milliseconds per point recharged
	{"toughness", VFOFS(toughness), VF_FLOAT},		//modifies incoming damage, 1.0 is normal, 0.5 is half, etc.  Simulates being made of tougher materials/construction
	{"malfunctionArmorLevel", VFOFS(malfunctionArmorLevel), VF_INT},//when armor drops to or below this point, start malfunctioning
	{"surfDestruction", VFOFS(surfDestruction), VF_INT},

	//visuals & sounds
	{"model", VFOFS(model), VF_STRING},				//what model to use - if make it an NPC's primary model, don't need this?
	{"skin", VFOFS(skin), VF_STRING},				//what skin to use - if make it an NPC's primary model, don't need this?
	{"g2radius", VFOFS(g2radius), VF_INT},			//render radius (really diameter, but...) for the ghoul2 model
	{"riderAnim", VFOFS(riderAnim), VF_ANIM},		//what animation the rider uses
	{"droidNPC", VFOFS(droidNPC), VF_STRING},		//NPC to attach to *droidunit tag (if it exists in the model)

	{"radarIcon", VFOFS(radarIconHandle), VF_SHADER_NOMIP},		//what icon to show on radar in MP
	{"dmgIndicFrame", VFOFS(dmgIndicFrameHandle), VF_SHADER_NOMIP},	//what image to use for the frame of the damage indicator
	{"dmgIndicShield", VFOFS(dmgIndicShieldHandle), VF_SHADER_NOMIP},//what image to use for the shield of the damage indicator
	{"dmgIndicBackground", VFOFS(dmgIndicBackgroundHandle), VF_SHADER_NOMIP},//what image to use for the background of the damage indicator
	{"icon_front", VFOFS(iconFrontHandle), VF_SHADER_NOMIP},	//what image to use for the front of the ship on the damage indicator
	{"icon_back", VFOFS(iconBackHandle), VF_SHADER_NOMIP},		//what image to use for the back of the ship on the damage indicator
	{"icon_right", VFOFS(iconRightHandle), VF_SHADER_NOMIP},	//what image to use for the right of the ship on the damage indicator
	{"icon_left", VFOFS(iconLeftHandle), VF_SHADER_NOMIP},		//what image to use for the left of the ship on the damage indicator
	{"crosshairShader", VFOFS(crosshairShaderHandle), VF_SHADER_NOMIP},	//what image to use as the crosshair
	{"shieldShader", VFOFS(shieldShaderHandle), VF_SHADER},		//What shader to use when drawing the shield shell

	//individual "area" health -rww
	{"health_front", VFOFS(health_front), VF_INT},
	{"health_back", VFOFS(health_back), VF_INT},
	{"health_right", VFOFS(health_right), VF_INT},
	{"health_left", VFOFS(health_left), VF_INT},

	{"soundOn",			VFOFS(soundOn),			VF_SOUND},//sound to play when get on it
	{"soundOff",		VFOFS(soundOff),		VF_SOUND},//sound to play when get off
	{"soundLoop",		VFOFS(soundLoop),		VF_SOUND},//sound to loop while riding it
	{"soundTakeOff",	VFOFS(soundTakeOff),	VF_SOUND},//sound to play when ship takes off
	{"soundEngineStart",VFOFS(soundEngineStart),VF_SOUND_CLIENT},//sound to play when ship's thrusters first activate
	{"soundSpin",		VFOFS(soundSpin),		VF_SOUND},//sound to loop while spiraling out of control
	{"soundTurbo",		VFOFS(soundTurbo),		VF_SOUND},//sound to play when turbo/afterburner kicks in
	{"soundHyper",		VFOFS(soundHyper),		VF_SOUND_CLIENT},//sound to play when hits hyperspace
	{"soundLand",		VFOFS(soundLand),		VF_SOUND},//sound to play when ship lands
	{"soundFlyBy",		VFOFS(soundFlyBy),		VF_SOUND_CLIENT},//sound to play when they buzz you
	{"soundFlyBy2",		VFOFS(soundFlyBy2),		VF_SOUND_CLIENT},//alternate sound to play when they buzz you
	{"soundShift1",		VFOFS(soundShift1),		VF_SOUND},//sound to play when changing speeds
	{"soundShift2",		VFOFS(soundShift2),		VF_SOUND},//sound to play when changing speeds
	{"soundShift3",		VFOFS(soundShift3),		VF_SOUND},//sound to play when changing speeds
	{"soundShift4",		VFOFS(soundShift4),		VF_SOUND},//sound to play when changing speeds

	{"exhaustFX", VFOFS(iExhaustFX), VF_EFFECT_CLIENT},		//exhaust effect, played from "*exhaust" bolt(s)
	{"turboFX", VFOFS(iTurboFX), VF_EFFECT_CLIENT},		//turbo exhaust effect, played from "*exhaust" bolt(s) when ship is in "turbo" mode
	{"turboStartFX", VFOFS(iTurboStartFX), VF_EFFECT},		//turbo start effect, played from "*exhaust" bolt(s) when ship is in "turbo" mode
	{"trailFX", VFOFS(iTrailFX), VF_EFFECT_CLIENT},		//trail effect, played from "*trail" bolt(s)
	{"impactFX", VFOFS(iImpactFX), VF_EFFECT_CLIENT},		//impact effect, for when it bumps into something
	{"explodeFX", VFOFS(iExplodeFX), VF_EFFECT},		//explosion effect, for when it blows up (should have the sound built into explosion effect)
	{"wakeFX", VFOFS(iWakeFX), VF_EFFECT_CLIENT},		//effect it makes when going across water
	{"dmgFX", VFOFS(iDmgFX), VF_EFFECT_CLIENT},		//effect to play on damage from a weapon or something
	{"injureFX", VFOFS(iInjureFX), VF_EFFECT_CLIENT}, //effect to play on partially damaged ship surface
	{"noseFX", VFOFS(iNoseFX), VF_EFFECT_CLIENT},		//effect for nose piece flying away when blown off
	{"lwingFX", VFOFS(iLWingFX), VF_EFFECT_CLIENT},		//effect for left wing piece flying away when blown off
	{"rwingFX", VFOFS(iRWingFX), VF_EFFECT_CLIENT},		//effect for right wing piece flying away when blown off

	// Weapon stuff:
	{"weap1", VFOFS(weapon[0].ID), VF_WEAPON},	//weapon used when press fire
	{"weap2", VFOFS(weapon[1].ID), VF_WEAPON},//weapon used when press alt-fire
	// The delay between shots for this weapon.
	{"weap1Delay", VFOFS(weapon[0].delay), VF_INT},
	{"weap2Delay", VFOFS(weapon[1].delay), VF_INT},
	// Whether or not all the muzzles for each weapon can be linked together (linked delay = weapon delay * number of muzzles linked!)
	{"weap1Link", VFOFS(weapon[0].linkable), VF_INT},
	{"weap2Link", VFOFS(weapon[1].linkable), VF_INT},
	// Whether or not to auto-aim the projectiles at the thing under the crosshair when we fire
	{"weap1Aim", VFOFS(weapon[0].aimCorrect), VF_BOOL},
	{"weap2Aim", VFOFS(weapon[1].aimCorrect), VF_BOOL},
	//maximum ammo
	{"weap1AmmoMax", VFOFS(weapon[0].ammoMax), VF_INT},
	{"weap2AmmoMax", VFOFS(weapon[1].ammoMax), VF_INT},
	//ammo recharge rate - milliseconds per unit (minimum of 100, which is 10 ammo per second)
	{"weap1AmmoRechargeMS", VFOFS(weapon[0].ammoRechargeMS), VF_INT},
	{"weap2AmmoRechargeMS", VFOFS(weapon[1].ammoRechargeMS), VF_INT},
	//sound to play when out of ammo (plays default "no ammo" sound if none specified)
	{"weap1SoundNoAmmo", VFOFS(weapon[0].soundNoAmmo), VF_SOUND_CLIENT},//sound to play when try to fire weapon 1 with no ammo
	{"weap2SoundNoAmmo", VFOFS(weapon[1].soundNoAmmo), VF_SOUND_CLIENT},//sound to play when try to fire weapon 2 with no ammo

	// Which weapon a muzzle fires (has to match one of the weapons this vehicle has).
	{"weapMuzzle1", VFOFS(weapMuzzle[0]), VF_WEAPON},
	{"weapMuzzle2", VFOFS(weapMuzzle[1]), VF_WEAPON},
	{"weapMuzzle3", VFOFS(weapMuzzle[2]), VF_WEAPON},
	{"weapMuzzle4", VFOFS(weapMuzzle[3]), VF_WEAPON},
	{"weapMuzzle5", VFOFS(weapMuzzle[4]), VF_WEAPON},
	{"weapMuzzle6", VFOFS(weapMuzzle[5]), VF_WEAPON},
	{"weapMuzzle7", VFOFS(weapMuzzle[6]), VF_WEAPON},
	{"weapMuzzle8", VFOFS(weapMuzzle[7]), VF_WEAPON},
	{"weapMuzzle9", VFOFS(weapMuzzle[8]), VF_WEAPON},
	{"weapMuzzle10", VFOFS(weapMuzzle[9]), VF_WEAPON},

	// The max height before this ship (?) starts (auto)landing.
	{"landingHeight", VFOFS(landingHeight), VF_FLOAT},

	//other misc stats
	{"gravity", VFOFS(gravity), VF_INT},			//normal is 800
	{"hoverHeight", VFOFS(hoverHeight), VF_FLOAT},	//if 0, it's a ground vehicle
	{"hoverStrength", VFOFS(hoverStrength), VF_FLOAT},	//how hard it pushes off ground when less than hover height... causes "bounce", like shocks
	{"waterProof", VFOFS(waterProof), VF_BOOL},		//can drive underwater if it has to
	{"bouyancy", VFOFS(bouyancy), VF_FLOAT},		//when in water, how high it floats (1 is neutral bouyancy)
	{"fuelMax", VFOFS(fuelMax), VF_INT},		//how much fuel it can hold (capacity)
	{"fuelRate", VFOFS(fuelRate), VF_INT},		//how quickly is uses up fuel
	{"turboDuration", VFOFS(turboDuration), VF_INT},		//how long turbo lasts
	{"turboRecharge", VFOFS(turboRecharge), VF_INT},		//how long turbo takes to recharge
	{"visibility", VFOFS(visibility), VF_INT},		//for sight alerts
	{"loudness", VFOFS(loudness), VF_INT},		//for sound alerts
	{"explosionRadius", VFOFS(explosionRadius), VF_FLOAT},//range of explosion
	{"explosionDamage", VFOFS(explosionDamage), VF_INT},//damage of explosion

	//new stuff
	{"maxPassengers", VFOFS(maxPassengers), VF_INT},	// The max number of passengers this vehicle may have (Default = 0).
	{"hideRider", VFOFS(hideRider), VF_BOOL},			// rider (and passengers?) should not be drawn
	{"killRiderOnDeath", VFOFS(killRiderOnDeath), VF_BOOL},//if rider is on vehicle when it dies, they should die
	{"flammable", VFOFS(flammable), VF_BOOL},			//whether or not the vehicle should catch on fire before it explodes
	{"explosionDelay", VFOFS(explosionDelay), VF_INT},	//how long the vehicle should be on fire/dying before it explodes
	//camera stuff
	{"cameraOverride", VFOFS(cameraOverride), VF_BOOL},//override the third person camera with the below values - normal is 0 (off)
	{"cameraRange", VFOFS(cameraRange), VF_FLOAT},		//how far back the camera should be - normal is 80
	{"cameraVertOffset", VFOFS(cameraVertOffset), VF_FLOAT},//how high over the vehicle origin the camera should be - normal is 16
	{"cameraHorzOffset", VFOFS(cameraHorzOffset), VF_FLOAT},//how far to left/right (negative/positive) of of the vehicle origin the camera should be - normal is 0
	{"cameraPitchOffset", VFOFS(cameraPitchOffset), VF_FLOAT},//a modifier on the camera's pitch (up/down angle) to the vehicle - normal is 0
	{"cameraFOV", VFOFS(cameraFOV), VF_FLOAT},			//third person camera FOV, default is 80
	{"cameraAlpha", VFOFS(cameraAlpha), VF_FLOAT},		//fade out the vehicle to this alpha (0.1-1.0f) if it's in the way of the crosshair
	{"cameraPitchDependantVertOffset", VFOFS(cameraPitchDependantVertOffset), VF_BOOL},		//use the hacky AT-ST pitch dependant vertical offset
//===TURRETS===========================================================================
	//Turret 1
	{"turret1Weap", VFOFS(turret[0].iWeapon), VF_WEAPON},
	{"turret1Delay", VFOFS(turret[0].iDelay), VF_INT},
	{"turret1AmmoMax", VFOFS(turret[0].iAmmoMax), VF_INT},
	{"turret1AmmoRechargeMS", VFOFS(turret[0].iAmmoRechargeMS), VF_INT},
	{"turret1YawBone", VFOFS(turret[0].yawBone), VF_STRING},
	{"turret1PitchBone", VFOFS(turret[0].pitchBone), VF_STRING},
	{"turret1YawAxis", VFOFS(turret[0].yawAxis), VF_INT},
	{"turret1PitchAxis", VFOFS(turret[0].pitchAxis), VF_INT},
	{"turret1ClampYawL", VFOFS(turret[0].yawClampLeft), VF_FLOAT},	//how far the turret is allowed to turn left
	{"turret1ClampYawR", VFOFS(turret[0].yawClampRight), VF_FLOAT},	//how far the turret is allowed to turn right
	{"turret1ClampPitchU", VFOFS(turret[0].pitchClampUp), VF_FLOAT},	//how far the turret is allowed to title up
	{"turret1ClampPitchD", VFOFS(turret[0].pitchClampDown), VF_FLOAT}, //how far the turret is allowed to tilt down
	{"turret1Muzzle1", VFOFS(turret[0].iMuzzle[0]), VF_INT},
	{"turret1Muzzle2", VFOFS(turret[0].iMuzzle[1]), VF_INT},
	{"turret1TurnSpeed", VFOFS(turret[0].fTurnSpeed), VF_FLOAT},
	{"turret1AI", VFOFS(turret[0].bAI), VF_BOOL},
	{"turret1AILead", VFOFS(turret[0].bAILead), VF_BOOL},
	{"turret1AIRange", VFOFS(turret[0].fAIRange), VF_FLOAT},
	{"turret1PassengerNum", VFOFS(turret[0].passengerNum), VF_INT},//which number passenger can control this turret
	{"turret1GunnerViewTag", VFOFS(turret[0].gunnerViewTag), VF_STRING},

	//Turret 2
	{"turret2Weap", VFOFS(turret[1].iWeapon), VF_WEAPON},
	{"turret2Delay", VFOFS(turret[1].iDelay), VF_INT},
	{"turret2AmmoMax", VFOFS(turret[1].iAmmoMax), VF_INT},
	{"turret2AmmoRechargeMS", VFOFS(turret[1].iAmmoRechargeMS), VF_INT},
	{"turret2YawBone", VFOFS(turret[1].yawBone), VF_STRING},
	{"turret2PitchBone", VFOFS(turret[1].pitchBone), VF_STRING},
	{"turret2YawAxis", VFOFS(turret[1].yawAxis), VF_INT},
	{"turret2PitchAxis", VFOFS(turret[1].pitchAxis), VF_INT},
	{"turret2ClampYawL", VFOFS(turret[1].yawClampLeft), VF_FLOAT},	//how far the turret is allowed to turn left
	{"turret2ClampYawR", VFOFS(turret[1].yawClampRight), VF_FLOAT},	//how far the turret is allowed to turn right
	{"turret2ClampPitchU", VFOFS(turret[1].pitchClampUp), VF_FLOAT},	//how far the turret is allowed to title up
	{"turret2ClampPitchD", VFOFS(turret[1].pitchClampDown), VF_FLOAT}, //how far the turret is allowed to tilt down
	{"turret2Muzzle1", VFOFS(turret[1].iMuzzle[0]), VF_INT},
	{"turret2Muzzle2", VFOFS(turret[1].iMuzzle[1]), VF_INT},
	{"turret2TurnSpeed", VFOFS(turret[1].fTurnSpeed), VF_FLOAT},
	{"turret2AI", VFOFS(turret[1].bAI), VF_BOOL},
	{"turret2AILead", VFOFS(turret[1].bAILead), VF_BOOL},
	{"turret2AIRange", VFOFS(turret[1].fAIRange), VF_FLOAT},
	{"turret2PassengerNum", VFOFS(turret[1].passengerNum), VF_INT},//which number passenger can control this turret
	{"turret2GunnerViewTag", VFOFS(turret[1].gunnerViewTag), VF_STRING},
//===END TURRETS===========================================================================
};

static const size_t numVehicleFields = ARRAY_LEN( vehicleFields );

stringID_table_t VehicleTable[VH_NUM_VEHICLES+1] =
{
	ENUM2STRING(VH_NONE),
	ENUM2STRING(VH_WALKER),		//something you ride inside of, it walks like you, like an AT-ST
	ENUM2STRING(VH_FIGHTER),	//something you fly inside of, like an X-Wing or TIE fighter
	ENUM2STRING(VH_SPEEDER),	//something you ride on that hovers, like a speeder or swoop
	ENUM2STRING(VH_ANIMAL),		//animal you ride on top of that walks, like a tauntaun
	ENUM2STRING(VH_FLIER),		//animal you ride on top of that flies, like a giant mynoc?
	{0,	-1}
};

// Setup the shared functions (one's that all vehicles would generally use).
extern void G_SetSharedVehicleFunctions( vehicleInfo_t *pVehInfo );
void BG_SetSharedVehicleFunctions( vehicleInfo_t *pVehInfo )
{
#ifdef _GAME
	//only do the whole thing if we're on game
	G_SetSharedVehicleFunctions(pVehInfo);
#endif

#if defined(_GAME) || defined(_CGAME)
	switch( pVehInfo->type )
	{
		case VH_SPEEDER:
			G_SetSpeederVehicleFunctions( pVehInfo );
			break;
		case VH_ANIMAL:
			G_SetAnimalVehicleFunctions( pVehInfo );
			break;
		case VH_FIGHTER:
			G_SetFighterVehicleFunctions( pVehInfo );
			break;
		case VH_WALKER:
			G_SetWalkerVehicleFunctions( pVehInfo );
			break;
		default:
			break;
	}
#endif
}

void BG_VehicleSetDefaults( vehicleInfo_t *vehicle )
{
	memset(vehicle, 0, sizeof(vehicleInfo_t));
/*
	if (!vehicle->name)
	{
		vehicle->name = (char *)BG_Alloc(1024);
	}
	strcpy(vehicle->name, "default");

	//general data
	vehicle->type = VH_SPEEDER;				//what kind of vehicle
	//FIXME: no saber or weapons if numHands = 2, should switch to speeder weapon, no attack anim on player
	vehicle->numHands = 0;					//if 2 hands, no weapons, if 1 hand, can use 1-handed weapons, if 0 hands, can use 2-handed weapons
	vehicle->lookPitch = 0;				//How far you can look up and down off the forward of the vehicle
	vehicle->lookYaw = 5;					//How far you can look left and right off the forward of the vehicle
	vehicle->length = 0;					//how long it is - used for body length traces when turning/moving?
	vehicle->width = 0;						//how wide it is - used for body length traces when turning/moving?
	vehicle->height = 0;					//how tall it is - used for body length traces when turning/moving?
	VectorClear( vehicle->centerOfGravity );//offset from origin: {forward, right, up} as a modifier on that dimension (-1.0f is all the way back, 1.0f is all the way forward)

	//speed stats - note: these are DESIRED speed, not actual current speed/velocity
	vehicle->speedMax = VEH_DEFAULT_SPEED_MAX;	//top speed
	vehicle->turboSpeed = 0;					//turboBoost
	vehicle->speedMin = 0;						//if < 0, can go in reverse
	vehicle->speedIdle = 0;						//what speed it drifts to when no accel/decel input is given
	vehicle->accelIdle = 0;						//if speedIdle > 0, how quickly it goes up to that speed
	vehicle->acceleration = VEH_DEFAULT_ACCEL;	//when pressing on accelerator (1/2 this when going in reverse)
	vehicle->decelIdle = VEH_DEFAULT_DECEL;		//when giving no input, how quickly it desired speed drops to speedIdle
	vehicle->strafePerc = VEH_DEFAULT_STRAFE_PERC;//multiplier on current speed for strafing.  If 1.0f, you can strafe at the same speed as you're going forward, 0.5 is half, 0 is no strafing

	//handling stats
	vehicle->bankingSpeed = VEH_DEFAULT_BANKING_SPEED;	//how quickly it pitches and rolls (not under player control)
	vehicle->rollLimit = VEH_DEFAULT_ROLL_LIMIT;		//how far it can roll to either side
	vehicle->pitchLimit = VEH_DEFAULT_PITCH_LIMIT;		//how far it can pitch forward or backward
	vehicle->braking = VEH_DEFAULT_BRAKING;				//when pressing on decelerator (backwards)
	vehicle->turningSpeed = VEH_DEFAULT_TURNING_SPEED;	//how quickly you can turn
	vehicle->turnWhenStopped = qfalse;					//whether or not you can turn when not moving
	vehicle->traction = VEH_DEFAULT_TRACTION;			//how much your command input affects velocity
	vehicle->friction = VEH_DEFAULT_FRICTION;			//how much velocity is cut on its own
	vehicle->maxSlope = VEH_DEFAULT_MAX_SLOPE;			//the max slope that it can go up with control

	//durability stats
	vehicle->mass = VEH_DEFAULT_MASS;			//for momentum and impact force (player mass is 10)
	vehicle->armor = VEH_DEFAULT_MAX_ARMOR;		//total points of damage it can take
	vehicle->toughness = VEH_DEFAULT_TOUGHNESS;	//modifies incoming damage, 1.0 is normal, 0.5 is half, etc.  Simulates being made of tougher materials/construction
	vehicle->malfunctionArmorLevel = 0;			//when armor drops to or below this point, start malfunctioning

	//visuals & sounds
	//vehicle->model = "models/map_objects/ships/swoop.md3";	//what model to use - if make it an NPC's primary model, don't need this?
	if (!vehicle->model)
	{
		vehicle->model = (char *)BG_Alloc(1024);
	}
	strcpy(vehicle->model, "models/map_objects/ships/swoop.md3");

	vehicle->modelIndex = 0;							//set internally, not until this vehicle is spawned into the level
	vehicle->skin = NULL;								//what skin to use - if make it an NPC's primary model, don't need this?
	vehicle->riderAnim = BOTH_GUNSIT1;					//what animation the rider uses

	vehicle->soundOn = NULL;							//sound to play when get on it
	vehicle->soundLoop = NULL;							//sound to loop while riding it
	vehicle->soundOff = NULL;							//sound to play when get off
	vehicle->exhaustFX = NULL;							//exhaust effect, played from "*exhaust" bolt(s)
	vehicle->trailFX = NULL;							//trail effect, played from "*trail" bolt(s)
	vehicle->impactFX = NULL;							//explosion effect, for when it blows up (should have the sound built into explosion effect)
	vehicle->explodeFX = NULL;							//explosion effect, for when it blows up (should have the sound built into explosion effect)
	vehicle->wakeFX = NULL;								//effect itmakes when going across water

	//other misc stats
	vehicle->gravity = VEH_DEFAULT_GRAVITY;				//normal is 800
	vehicle->hoverHeight = 0;//VEH_DEFAULT_HOVER_HEIGHT;	//if 0, it's a ground vehicle
	vehicle->hoverStrength = 0;//VEH_DEFAULT_HOVER_STRENGTH;//how hard it pushes off ground when less than hover height... causes "bounce", like shocks
	vehicle->waterProof = qtrue;						//can drive underwater if it has to
	vehicle->bouyancy = 1.0f;							//when in water, how high it floats (1 is neutral bouyancy)
	vehicle->fuelMax = 1000;							//how much fuel it can hold (capacity)
	vehicle->fuelRate = 1;								//how quickly is uses up fuel
	vehicle->visibility = VEH_DEFAULT_VISIBILITY;		//radius for sight alerts
	vehicle->loudness = VEH_DEFAULT_LOUDNESS;			//radius for sound alerts
	vehicle->explosionRadius = VEH_DEFAULT_EXP_RAD;
	vehicle->explosionDamage = VEH_DEFAULT_EXP_DMG;
	vehicle->maxPassengers = 0;

	//new stuff
	vehicle->hideRider = qfalse;						// rider (and passengers?) should not be drawn
	vehicle->killRiderOnDeath = qfalse;					//if rider is on vehicle when it dies, they should die
	vehicle->flammable = qfalse;						//whether or not the vehicle should catch on fire before it explodes
	vehicle->explosionDelay = 0;						//how long the vehicle should be on fire/dying before it explodes
	//camera stuff
	vehicle->cameraOverride = qfalse;					//whether or not to use all of the following 3rd person camera override values
	vehicle->cameraRange = 0.0f;						//how far back the camera should be - normal is 80
	vehicle->cameraVertOffset = 0.0f;					//how high over the vehicle origin the camera should be - normal is 16
	vehicle->cameraHorzOffset = 0.0f;					//how far to left/right (negative/positive) of of the vehicle origin the camera should be - normal is 0
	vehicle->cameraPitchOffset = 0.0f;					//a modifier on the camera's pitch (up/down angle) to the vehicle - normal is 0
	vehicle->cameraFOV = 0.0f;							//third person camera FOV, default is 80
	vehicle->cameraAlpha = qfalse;						//fade out the vehicle if it's in the way of the crosshair
*/
}

void BG_VehicleClampData( vehicleInfo_t *vehicle )
{//sanity check and clamp the vehicle's data
	int		i;

	for ( i = 0; i < 3; i++ )
	{
		if ( vehicle->centerOfGravity[i] > 1.0f )
		{
			vehicle->centerOfGravity[i] = 1.0f;
		}
		else if ( vehicle->centerOfGravity[i] < -1.0f )
		{
			vehicle->centerOfGravity[i] = -1.0f;
		}
	}

	// Validate passenger max.
	if ( vehicle->maxPassengers > VEH_MAX_PASSENGERS )
	{
		vehicle->maxPassengers = VEH_MAX_PASSENGERS;
	}
	else if ( vehicle->maxPassengers < 0 )
	{
		vehicle->maxPassengers = 0;
	}
}

static qboolean BG_ParseVehicleParm( vehicleInfo_t *vehicle, const char *parmName, char *pValue )
{
	vehField_t *vehField;
	vec3_t	vec;
	byte	*b = (byte *)vehicle;
	int		_iFieldsRead = 0;
	vehicleType_t vehType;
	char value[1024];

	Q_strncpyz( value, pValue, sizeof(value) );

	// Loop through possible parameters
	vehField = (vehField_t *)Q_LinearSearch( parmName, vehicleFields, numVehicleFields, sizeof( vehicleFields[0] ), vfieldcmp );

	if ( !vehField )
		return qfalse;

	// found it
	switch( vehField->type )
	{
	case VF_IGNORE:
		break;
	case VF_INT:
		*(int *)(b+vehField->ofs) = atoi(value);
		break;
	case VF_FLOAT:
		*(float *)(b+vehField->ofs) = atof(value);
		break;
	case VF_STRING:	// string on disk, pointer in memory
		if (!*(char **)(b+vehField->ofs))
		{ //just use 128 bytes in case we want to write over the string
			*(char **)(b+vehField->ofs) = (char *)BG_Alloc(128);//(char *)BG_Alloc(strlen(value));
			strcpy(*(char **)(b+vehField->ofs), value);
		}

		break;
	case VF_VECTOR:
		_iFieldsRead = sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
		//assert(_iFieldsRead==3 );
		if (_iFieldsRead!=3)
		{
			Com_Printf (S_COLOR_YELLOW"BG_ParseVehicleParm: VEC3 sscanf() failed to read 3 floats ('angle' key bug?)\n");
			VectorClear( vec );
		}
		((float *)(b+vehField->ofs))[0] = vec[0];
		((float *)(b+vehField->ofs))[1] = vec[1];
		((float *)(b+vehField->ofs))[2] = vec[2];
		break;
	case VF_BOOL:
		*(qboolean *)(b+vehField->ofs) = (qboolean)(atof(value)!=0);
		break;
	case VF_VEHTYPE:
		vehType = (vehicleType_t)GetIDForString( VehicleTable, value );
		*(vehicleType_t *)(b+vehField->ofs) = vehType;
		break;
	case VF_ANIM:
		{
			int anim = GetIDForString( animTable, value );
			*(int *)(b+vehField->ofs) = anim;
		}
		break;
	case VF_WEAPON:	// take string, resolve into index into VehWeaponParms
		*(int *)(b+vehField->ofs) = VEH_VehWeaponIndexForName( value );
		break;
	case VF_MODEL:	// take the string, get the G_ModelIndex
#ifdef _GAME
		*(int *)(b+vehField->ofs) = G_ModelIndex( value );
#else
		*(int *)(b+vehField->ofs) = trap->R_RegisterModel( value );
#endif
		break;
	case VF_MODEL_CLIENT:	// (MP cgame only) take the string, get the G_ModelIndex
#ifdef _GAME
		//*(int *)(b+vehField->ofs) = G_ModelIndex( value );
#else
		*(int *)(b+vehField->ofs) = trap->R_RegisterModel( value );
#endif
		break;
	case VF_EFFECT:	// take the string, get the G_EffectIndex
#ifdef _GAME
		*(int *)(b+vehField->ofs) = G_EffectIndex( value );
#elif _CGAME
		*(int *)(b+vehField->ofs) = trap->FX_RegisterEffect( value );
#endif
		break;
	case VF_EFFECT_CLIENT:	// (MP cgame only) take the string, get the G_EffectIndex
#ifdef _GAME
		//*(int *)(b+vehField->ofs) = G_EffectIndex( value );
#elif _CGAME
		*(int *)(b+vehField->ofs) = trap->FX_RegisterEffect( value );
#endif
		break;
	case VF_SHADER:	// (cgame only) take the string, call trap_R_RegisterShader
#ifdef UI_BUILD
		*(int *)(b+vehField->ofs) = trap->R_RegisterShaderNoMip( value );
#elif _CGAME
		*(int *)(b+vehField->ofs) = trap->R_RegisterShader( value );
#endif
		break;
	case VF_SHADER_NOMIP:// (cgame only) take the string, call trap_R_RegisterShaderNoMip
#if defined(_CGAME) || defined(UI_BUILD)
		*(int *)(b+vehField->ofs) = trap->R_RegisterShaderNoMip( value );
#endif
		break;
	case VF_SOUND:	// take the string, get the G_SoundIndex
#ifdef _GAME
		*(int *)(b+vehField->ofs) = G_SoundIndex( value );
#else
		*(int *)(b+vehField->ofs) = trap->S_RegisterSound( value );
#endif
		break;
	case VF_SOUND_CLIENT:	// (MP cgame only) take the string, get the G_SoundIndex
#ifdef _GAME
		//*(int *)(b+vehField->ofs) = G_SoundIndex( value );
#else
		*(int *)(b+vehField->ofs) = trap->S_RegisterSound( value );
#endif
		break;
	default:
		//Unknown type?
		return qfalse;
	}

	return qtrue;
}

int VEH_LoadVehicle( const char *vehicleName )
{//load up specified vehicle and save in array: g_vehicleInfo
	const char	*token;
	//we'll assume that no parm name is longer than 128
	char		parmName[128] = { 0 };
	char		weap1[128] = { 0 }, weap2[128] = { 0 };
	char		weapMuzzle1[128] = { 0 };
	char		weapMuzzle2[128] = { 0 };
	char		weapMuzzle3[128] = { 0 };
	char		weapMuzzle4[128] = { 0 };
	char		weapMuzzle5[128] = { 0 };
	char		weapMuzzle6[128] = { 0 };
	char		weapMuzzle7[128] = { 0 };
	char		weapMuzzle8[128] = { 0 };
	char		weapMuzzle9[128] = { 0 };
	char		weapMuzzle10[128] = { 0 };
	char		*value = NULL;
	const char	*p = NULL;
	vehicleInfo_t	*vehicle = NULL;

	// Load the vehicle parms if no vehicles have been loaded yet.
	if ( numVehicles == 0 )
	{
		BG_VehicleLoadParms();
	}

	//try to parse data out
	p = VehicleParms;

	COM_BeginParseSession("vehicles");

	vehicle = &g_vehicleInfo[numVehicles];
	// look for the right vehicle
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return VEHICLE_NONE;
		}

		if ( !Q_stricmp( token, vehicleName ) )
		{
			break;
		}

		SkipBracedSection( &p, 0 );
	}

	if ( !p )
	{
		return VEHICLE_NONE;
	}

	token = COM_ParseExt( &p, qtrue );
	if ( token[0] == 0 )
	{//barf
		return VEHICLE_NONE;
	}

	if ( Q_stricmp( token, "{" ) != 0 )
	{
		return VEHICLE_NONE;
	}

	BG_VehicleSetDefaults( vehicle );
	// parse the vehicle info block
	while ( 1 )
	{
		SkipRestOfLine( &p );
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing Vehicle '%s'\n", vehicleName );
			return VEHICLE_NONE;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}
		Q_strncpyz( parmName, token, sizeof(parmName) );
		value = COM_ParseExt( &p, qtrue );
		if ( !value || !value[0] )
		{
			Com_Printf( S_COLOR_RED"ERROR: Vehicle token '%s' has no value!\n", parmName );
		}
		else if ( Q_stricmp( "weap1", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weap1, value, sizeof(weap1) );
		}
		else if ( Q_stricmp( "weap2", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weap2, value, sizeof(weap2) );
		}
		else if ( Q_stricmp( "weapMuzzle1", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle1, value, sizeof(weapMuzzle1) );
		}
		else if ( Q_stricmp( "weapMuzzle2", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle2, value, sizeof(weapMuzzle2) );
		}
		else if ( Q_stricmp( "weapMuzzle3", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle3, value, sizeof(weapMuzzle3) );
		}
		else if ( Q_stricmp( "weapMuzzle4", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle4, value, sizeof(weapMuzzle4) );
		}
		else if ( Q_stricmp( "weapMuzzle5", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle5, value, sizeof(weapMuzzle5) );
		}
		else if ( Q_stricmp( "weapMuzzle6", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle6, value, sizeof(weapMuzzle6) );
		}
		else if ( Q_stricmp( "weapMuzzle7", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle7, value, sizeof(weapMuzzle7) );
		}
		else if ( Q_stricmp( "weapMuzzle8", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle8, value, sizeof(weapMuzzle8) );
		}
		else if ( Q_stricmp( "weapMuzzle9", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle9, value, sizeof(weapMuzzle9) );
		}
		else if ( Q_stricmp( "weapMuzzle10", parmName ) == 0 )
		{//hmm, store this off because we don't want to call another one of these text parsing routines while we're in the middle of one...
			Q_strncpyz( weapMuzzle10, value, sizeof(weapMuzzle10) );
		}
		else
		{
			if ( !BG_ParseVehicleParm( vehicle, parmName, value ) )
			{
#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair '%s', '%s'!\n", parmName, value );
#endif
			}
		}
	}
	//NOW: if we have any weapons, go ahead and load them
	if ( weap1[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weap1", weap1 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weap1', '%s'!\n", weap1 );
#endif
		}
	}
	if ( weap2[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weap2", weap2 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weap2', '%s'!\n", weap2 );
#endif
		}
	}
	if ( weapMuzzle1[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle1", weapMuzzle1 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle1', '%s'!\n", weapMuzzle1 );
#endif
		}
	}
	if ( weapMuzzle2[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle2", weapMuzzle2 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle2', '%s'!\n", weapMuzzle2 );
#endif
		}
	}
	if ( weapMuzzle3[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle3", weapMuzzle3 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle3', '%s'!\n", weapMuzzle3 );
#endif
		}
	}
	if ( weapMuzzle4[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle4", weapMuzzle4 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle4', '%s'!\n", weapMuzzle4 );
#endif
		}
	}
	if ( weapMuzzle5[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle5", weapMuzzle5 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle5', '%s'!\n", weapMuzzle5 );
#endif
		}
	}
	if ( weapMuzzle6[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle6", weapMuzzle6 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle6', '%s'!\n", weapMuzzle6 );
#endif
		}
	}
	if ( weapMuzzle7[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle7", weapMuzzle7 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle7', '%s'!\n", weapMuzzle7 );
#endif
		}
	}
	if ( weapMuzzle8[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle8", weapMuzzle8 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle8', '%s'!\n", weapMuzzle8 );
#endif
		}
	}
	if ( weapMuzzle9[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle9", weapMuzzle9 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle9', '%s'!\n", weapMuzzle9 );
#endif
		}
	}
	if ( weapMuzzle10[0] )
	{
		if ( !BG_ParseVehicleParm( vehicle, "weapMuzzle10", weapMuzzle10 ) )
		{
#ifndef FINAL_BUILD
			Com_Printf( S_COLOR_RED"ERROR: Unknown Vehicle key/value pair 'weapMuzzle10', '%s'!\n", weapMuzzle10 );
#endif
		}
	}

	//let's give these guys some defaults
	if (!vehicle->health_front)
	{
		vehicle->health_front = vehicle->armor/4;
	}
	if (!vehicle->health_back)
	{
		vehicle->health_back = vehicle->armor/4;
	}
	if (!vehicle->health_right)
	{
		vehicle->health_right = vehicle->armor/4;
	}
	if (!vehicle->health_left)
	{
		vehicle->health_left = vehicle->armor/4;
	}

	if ( vehicle->model )
	{
		#ifdef _GAME
			vehicle->modelIndex = G_ModelIndex( va( "models/players/%s/model.glm", vehicle->model ) );
		#else
			vehicle->modelIndex = trap->R_RegisterModel( va( "models/players/%s/model.glm", vehicle->model ) );
		#endif
	}

	#if defined(_CGAME) || defined(UI_BUILD)
		if ( VALIDSTRING( vehicle->skin ) )
			trap->R_RegisterSkin( va( "models/players/%s/model_%s.skin", vehicle->model, vehicle->skin) );
	#endif

	//sanity check and clamp the vehicle's data
	BG_VehicleClampData( vehicle );
	// Setup the shared function pointers.
	BG_SetSharedVehicleFunctions( vehicle );
	//misc effects... FIXME: not even used in MP, are they?
	if ( vehicle->explosionDamage )
	{
		#ifdef _GAME
			G_EffectIndex( "ships/ship_explosion_mark" );
		#elif defined(_CGAME)
			trap->FX_RegisterEffect( "ships/ship_explosion_mark" );
		#endif
	}
	if ( vehicle->flammable )
	{
		#ifdef _GAME
			G_SoundIndex( "sound/vehicles/common/fire_lp.wav" );
		#else
			trap->S_RegisterSound( "sound/vehicles/common/fire_lp.wav" );
		#endif
	}

	if ( vehicle->hoverHeight > 0 )
	{
		#ifdef _GAME
			G_EffectIndex( "ships/swoop_dust" );
		#elif defined(_CGAME)
			trap->FX_RegisterEffect( "ships/swoop_dust" );
		#endif
	}

#ifdef _GAME
	G_EffectIndex( "volumetric/black_smoke" );
	G_EffectIndex( "ships/fire" );
	G_SoundIndex( "sound/vehicles/common/release.wav" );
#elif defined(_CGAME)
	trap->R_RegisterShader( "gfx/menus/radar/bracket" );
	trap->R_RegisterShader( "gfx/menus/radar/lead" );
	trap->R_RegisterShaderNoMip( "gfx/menus/radar/asteroid" );
	trap->S_RegisterSound( "sound/vehicles/common/impactalarm.wav" );
	trap->S_RegisterSound( "sound/vehicles/common/linkweaps.wav" );
	trap->S_RegisterSound( "sound/vehicles/common/release.wav" );
	trap->FX_RegisterEffect("effects/ships/dest_burning.efx");
	trap->FX_RegisterEffect("effects/ships/dest_destroyed.efx");
	trap->FX_RegisterEffect( "volumetric/black_smoke" );
	trap->FX_RegisterEffect( "ships/fire" );
	trap->FX_RegisterEffect("ships/hyperspace_stars");

	if ( vehicle->hideRider )
	{
		trap->R_RegisterShaderNoMip( "gfx/menus/radar/circle_base" );
		trap->R_RegisterShaderNoMip( "gfx/menus/radar/circle_base_frame" );
		trap->R_RegisterShaderNoMip( "gfx/menus/radar/circle_base_shield" );
	}
#endif

	return (numVehicles++);
}

int VEH_VehicleIndexForName( const char *vehicleName )
{
	int v;
	if ( !vehicleName || !vehicleName[0] )
	{
		Com_Printf( S_COLOR_RED"ERROR: Trying to read Vehicle with no name!\n" );
		return VEHICLE_NONE;
	}
	for (  v = VEHICLE_BASE; v < numVehicles; v++ )
	{
		if ( g_vehicleInfo[v].name
			&& Q_stricmp( g_vehicleInfo[v].name, vehicleName ) == 0 )
		{//already loaded this one
			return v;
		}
	}
	//haven't loaded it yet
	if ( v >= MAX_VEHICLES )
	{//no more room!
		Com_Printf( S_COLOR_RED"ERROR: Too many Vehicles (max %d), aborting load on %s!\n", MAX_VEHICLES, vehicleName );
		return VEHICLE_NONE;
	}
	//we have room for another one, load it up and return the index
	//HMM... should we not even load the .veh file until we want to?
	v = VEH_LoadVehicle( vehicleName );
 	if ( v == VEHICLE_NONE )
	{
		Com_Printf( S_COLOR_RED"ERROR: Could not find Vehicle %s!\n", vehicleName );
	}
	return v;
}

void BG_VehWeaponLoadParms( void )
{
	int			len, totallen, vehExtFNLen, fileCnt, i;
	char		*holdChar, *marker;
	char		vehWeaponExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t	f;
	char		*tempReadBuffer;

	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = VehWeaponParms+totallen;
	*marker = 0;

	//now load in the extra .veh extensions
	fileCnt = trap->FS_GetFileList("ext_data/vehicles/weapons", ".vwp", vehWeaponExtensionListBuf, sizeof(vehWeaponExtensionListBuf) );

	holdChar = vehWeaponExtensionListBuf;

	tempReadBuffer = (char *)BG_TempAlloc(MAX_VEH_WEAPON_DATA_SIZE);

	// NOTE: Not use TempAlloc anymore...
	//Make ABSOLUTELY CERTAIN that BG_Alloc/etc. is not used before
	//the subsequent BG_TempFree or the pool will be screwed.

	for ( i = 0; i < fileCnt; i++, holdChar += vehExtFNLen + 1 )
	{
		vehExtFNLen = strlen( holdChar );

//		Com_Printf( "Parsing %s\n", holdChar );

		len = trap->FS_Open(va( "ext_data/vehicles/weapons/%s", holdChar), &f, FS_READ);

		if ( len == -1 )
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			trap->FS_Read(tempReadBuffer, len, f);
			tempReadBuffer[len] = 0;

			// Don't let it end on a } because that should be a stand-alone token.
			if ( totallen && *(marker-1) == '}' )
			{
				strcat( marker, " " );
				totallen++;
				marker++;
			}

			if ( totallen + len >= MAX_VEH_WEAPON_DATA_SIZE ) {
				trap->FS_Close( f );
				Com_Error(ERR_DROP, "Vehicle Weapon extensions (*.vwp) are too large" );
			}
			strcat( marker, tempReadBuffer );
			trap->FS_Close( f );

			totallen += len;
			marker = VehWeaponParms+totallen;
		}
	}

	BG_TempFree(MAX_VEH_WEAPON_DATA_SIZE);
}

void BG_VehicleLoadParms( void )
{//HMM... only do this if there's a vehicle on the level?
	int			len, totallen, vehExtFNLen, fileCnt, i;
//	const char	*filename = "ext_data/vehicles.dat";
	char		*holdChar, *marker;
	char		vehExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t	f;
	char		*tempReadBuffer;

	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = VehicleParms+totallen;
	*marker = 0;

	//now load in the extra .veh extensions
	fileCnt = trap->FS_GetFileList("ext_data/vehicles", ".veh", vehExtensionListBuf, sizeof(vehExtensionListBuf) );

	holdChar = vehExtensionListBuf;

	tempReadBuffer = (char *)BG_TempAlloc(MAX_VEHICLE_DATA_SIZE);

	// NOTE: Not use TempAlloc anymore...
	//Make ABSOLUTELY CERTAIN that BG_Alloc/etc. is not used before
	//the subsequent BG_TempFree or the pool will be screwed.

	for ( i = 0; i < fileCnt; i++, holdChar += vehExtFNLen + 1 )
	{
		vehExtFNLen = strlen( holdChar );

//		Com_Printf( "Parsing %s\n", holdChar );

		len = trap->FS_Open(va( "ext_data/vehicles/%s", holdChar), &f, FS_READ);

		if ( len == -1 )
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			trap->FS_Read(tempReadBuffer, len, f);
			tempReadBuffer[len] = 0;

			// Don't let it end on a } because that should be a stand-alone token.
			if ( totallen && *(marker-1) == '}' )
			{
				strcat( marker, " " );
				totallen++;
				marker++;
			}

			if ( totallen + len >= MAX_VEHICLE_DATA_SIZE ) {
				trap->FS_Close( f );
				Com_Error(ERR_DROP, "Vehicle extensions (*.veh) are too large" );
			}
			strcat( marker, tempReadBuffer );
			trap->FS_Close( f );

			totallen += len;
			marker = VehicleParms+totallen;
		}
	}

	BG_TempFree(MAX_VEHICLE_DATA_SIZE);

	numVehicles = 1;//first one is null/default
	//set the first vehicle to default data
	BG_VehicleSetDefaults( &g_vehicleInfo[VEHICLE_BASE] );
	//sanity check and clamp the vehicle's data
	BG_VehicleClampData( &g_vehicleInfo[VEHICLE_BASE] );
	// Setup the shared function pointers.
	BG_SetSharedVehicleFunctions( &g_vehicleInfo[VEHICLE_BASE] );

	//Load the Vehicle Weapons data, too
	BG_VehWeaponLoadParms();
}

int BG_VehicleGetIndex( const char *vehicleName )
{
	return (VEH_VehicleIndexForName( vehicleName ));
}

//We get the vehicle name passed in as modelname
//with a $ in front of it.
//we are expected to then get the model for the
//vehicle and stomp over modelname with it.
void BG_GetVehicleModelName(char *modelName, const char *vehicleName, size_t len)
{
	const char *vehName = &vehicleName[1];
	int vIndex = BG_VehicleGetIndex(vehName);
	assert(vehicleName[0] == '$');

	if (vIndex == VEHICLE_NONE)
		Com_Error(ERR_DROP, "BG_GetVehicleModelName:  couldn't find vehicle %s", vehName);

	Q_strncpyz( modelName, g_vehicleInfo[vIndex].model, len );
}

void BG_GetVehicleSkinName(char *skinname, int len)
{
	char *vehName = &skinname[1];
	int vIndex = BG_VehicleGetIndex(vehName);
	assert(skinname[0] == '$');

	if (vIndex == VEHICLE_NONE)
		Com_Error(ERR_DROP, "BG_GetVehicleSkinName:  couldn't find vehicle %s", vehName);

    if ( !VALIDSTRING( g_vehicleInfo[vIndex].skin ) )
		skinname[0] = 0;
	else
		Q_strncpyz( skinname, g_vehicleInfo[vIndex].skin, len );
}

#if defined(_GAME) || defined(_CGAME)
//so cgame can assign the function pointer for the vehicle attachment without having to
//bother with all the other funcs that don't really exist cgame-side.
extern int BG_GetTime(void);
void AttachRidersGeneric( Vehicle_t *pVeh )
{
	// If we have a pilot, attach him to the driver tag.
	if ( pVeh->m_pPilot )
	{
		mdxaBone_t boltMatrix;
		vec3_t	yawOnlyAngles;
		bgEntity_t *parent = pVeh->m_pParentEntity;
		bgEntity_t *pilot = pVeh->m_pPilot;
		int crotchBolt = trap->G2API_AddBolt(parent->ghoul2, 0, "*driver");

		assert(parent->playerState);

		VectorSet(yawOnlyAngles, 0, parent->playerState->viewangles[YAW], 0);

		// Get the driver tag.
		trap->G2API_GetBoltMatrix( parent->ghoul2, 0, crotchBolt, &boltMatrix, yawOnlyAngles, parent->playerState->origin, BG_GetTime(), NULL, parent->modelScale );
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, pilot->playerState->origin );
	}
}
#endif
