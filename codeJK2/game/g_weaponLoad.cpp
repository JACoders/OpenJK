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

// g_weaponLoad.cpp
// fills in memory struct with ext_dat\weapons.dat

#include "g_local.h"

typedef struct {
	const char	*name;
	void	(*func)(centity_t *cent, const struct weaponInfo_s *weapon );
} func_t;

// Bryar
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );

// Blaster
void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Bowcaster
void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Heavy Repeater
void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// DEMP2
void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_AltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Golan Arms Flechette
void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Personal Rocket Launcher
void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Emplaced weapon
void FX_EmplacedProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Turret weapon
void FX_TurretProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// ATST Main weapon
void FX_ATSTMainProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// ATST Side weapons
void FX_ATSTSideMainProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_ATSTSideAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Table used to attach an extern missile function string to the actual cgame function
func_t	funcs[] = {
	{"bryar_func",			FX_BryarProjectileThink},
	{"bryar_alt_func",		FX_BryarAltProjectileThink},
	{"blaster_func",		FX_BlasterProjectileThink},
	{"blaster_alt_func",	FX_BlasterAltFireThink},
	{"bowcaster_func",		FX_BowcasterProjectileThink},
	{"repeater_func",		FX_RepeaterProjectileThink},
	{"repeater_alt_func",	FX_RepeaterAltProjectileThink},
	{"demp2_func",			FX_DEMP2_ProjectileThink},
	{"demp2_alt_func",		FX_DEMP2_AltProjectileThink},
	{"flechette_func",		FX_FlechetteProjectileThink},
	{"flechette_alt_func",	FX_FlechetteAltProjectileThink},
	{"rocket_func",			FX_RocketProjectileThink},
	{"rocket_alt_func",		FX_RocketAltProjectileThink},
	{"emplaced_func",		FX_EmplacedProjectileThink},
	{"turret_func",			FX_TurretProjectileThink},
	{"atstmain_func",		FX_ATSTMainProjectileThink},
	{"atst_side_alt_func",	FX_ATSTSideAltProjectileThink},
	{"atst_side_main_func",	FX_ATSTSideMainProjectileThink},
	{NULL,					NULL}
};

//qboolean COM_ParseInt( char **data, int *i );
//qboolean COM_ParseString( char **data, char **s );
//qboolean COM_ParseFloat( char **data, float *f );

struct wpnParms_s
{
	int	weaponNum;	// Current weapon number
	int	ammoNum;
} wpnParms;

void WPN_Ammo (const char **holdBuf);
void WPN_AmmoIcon (const char **holdBuf);
void WPN_AmmoMax (const char **holdBuf);
void WPN_AmmoLowCnt (const char **holdBuf);
void WPN_AmmoType (const char **holdBuf);
void WPN_EnergyPerShot (const char **holdBuf);
void WPN_FireTime (const char **holdBuf);
void WPN_FiringSnd (const char **holdBuf);
void WPN_AltFiringSnd(const char **holdBuf );
void WPN_StopSnd( const char **holdBuf );
void WPN_ChargeSnd (const char **holdBuf);
void WPN_AltChargeSnd (const char **holdBuf);
void WPN_SelectSnd (const char **holdBuf);
void WPN_Range (const char **holdBuf);
void WPN_WeaponClass ( const char **holdBuf);
void WPN_WeaponIcon (const char **holdBuf);
void WPN_WeaponModel (const char **holdBuf);
void WPN_WeaponType (const char **holdBuf);
void WPN_AltEnergyPerShot (const char **holdBuf);
void WPN_AltFireTime (const char **holdBuf);
void WPN_AltRange (const char **holdBuf);
void WPN_BarrelCount(const char **holdBuf);
void WPN_MissileName(const char **holdBuf);
void WPN_AltMissileName(const char **holdBuf);
void WPN_MissileSound(const char **holdBuf);
void WPN_AltMissileSound(const char **holdBuf);
void WPN_MissileLight(const char **holdBuf);
void WPN_AltMissileLight(const char **holdBuf);
void WPN_MissileLightColor(const char **holdBuf);
void WPN_AltMissileLightColor(const char **holdBuf);
void WPN_FuncName(const char **holdBuf);
void WPN_AltFuncName(const char **holdBuf);
void WPN_MissileHitSound(const char **holdBuf);
void WPN_AltMissileHitSound(const char **holdBuf);
void WPN_MuzzleEffect(const char **holdBuf);
void WPN_AltMuzzleEffect(const char **holdBuf);

// OPENJK ADD

void WPN_Damage(const char **holdBuf);
void WPN_AltDamage(const char **holdBuf);
void WPN_SplashDamage(const char **holdBuf);
void WPN_SplashRadius(const char **holdBuf);
void WPN_AltSplashDamage(const char **holdBuf);
void WPN_AltSplashRadius(const char **holdBuf);

// Legacy weapons.dat force fields
void WPN_FuncSkip(const char **holdBuf);

typedef struct
{
	const char	*parmName;
	void	(*func)(const char **holdBuf);
} wpnParms_t;

// This is used as a fallback for each new field, in case they're using base files --eez
const int defaultDamage[] = {
	0,							// WP_NONE
	0,							// WP_SABER										// handled elsewhere
	BRYAR_PISTOL_DAMAGE,		// WP_BRYAR_PISTOL
	BLASTER_DAMAGE,				// WP_BLASTER
	DISRUPTOR_MAIN_DAMAGE,		// WP_DISRUPTOR
	BOWCASTER_DAMAGE,			// WP_BOWCASTER
	REPEATER_DAMAGE,			// WP_REPEATER
	DEMP2_DAMAGE,				// WP_DEMP2
	FLECHETTE_DAMAGE,			// WP_FLECHETTE
	ROCKET_DAMAGE,				// WP_ROCKET_LAUNCHER
	TD_DAMAGE,					// WP_THERMAL
	LT_DAMAGE,					// WP_TRIP_MINE
	FLECHETTE_MINE_DAMAGE,		// WP_DET_PACK									// HACK, this is what the code sez.
	STUN_BATON_DAMAGE,			// WP_STUN_BATON
	0,							// WP_MELEE										// handled by the melee attack function
	EMPLACED_DAMAGE,			// WP_EMPLACED
	BRYAR_PISTOL_DAMAGE,		// WP_BOT_LASER
	0,							// WP_TURRET									// handled elsewhere
	ATST_MAIN_DAMAGE,			// WP_ATST_MAIN
	ATST_SIDE_MAIN_DAMAGE,		// WP_ATST_SIDE
	EMPLACED_DAMAGE,			// WP_TIE_FIGHTER
	EMPLACED_DAMAGE,			// WP_RAPID_FIRE_CONC
	BRYAR_PISTOL_DAMAGE			// WP_BLASTER_PISTOL
};

const int defaultAltDamage[] = {
	0,							// WP_NONE
	0,							// WP_SABER										// handled elsewhere
	BRYAR_PISTOL_DAMAGE,		// WP_BRYAR_PISTOL
	BLASTER_DAMAGE,				// WP_BLASTER
	DISRUPTOR_ALT_DAMAGE,		// WP_DISRUPTOR
	BOWCASTER_DAMAGE,			// WP_BOWCASTER
	REPEATER_ALT_DAMAGE,		// WP_REPEATER
	DEMP2_ALT_DAMAGE,			// WP_DEMP2
	FLECHETTE_ALT_DAMAGE,		// WP_FLECHETTE
	ROCKET_DAMAGE,				// WP_ROCKET_LAUNCHER
	TD_ALT_DAMAGE,				// WP_THERMAL
	LT_DAMAGE,					// WP_TRIP_MINE
	FLECHETTE_MINE_DAMAGE,		// WP_DET_PACK									// HACK, this is what the code sez.
	STUN_BATON_ALT_DAMAGE,		// WP_STUN_BATON
	0,							// WP_MELEE										// handled by the melee attack function
	EMPLACED_DAMAGE,			// WP_EMPLACED
	BRYAR_PISTOL_DAMAGE,		// WP_BOT_LASER
	0,							// WP_TURRET									// handled elsewhere
	ATST_MAIN_DAMAGE,			// WP_ATST_MAIN
	ATST_SIDE_ALT_DAMAGE,		// WP_ATST_SIDE
	EMPLACED_DAMAGE,			// WP_TIE_FIGHTER
	0,							// WP_RAPID_FIRE_CONC							// repeater alt damage is used instead
	BRYAR_PISTOL_DAMAGE			// WP_BLASTER_PISTOL
};

const int defaultSplashDamage[] = {
	0,									// WP_NONE
	0,									// WP_SABER
	0,									// WP_BRYAR_PISTOL
	0,									// WP_BLASTER
	0,									// WP_DISRUPTOR
	BOWCASTER_SPLASH_DAMAGE,			// WP_BOWCASTER
	0,									// WP_REPEATER
	0,									// WP_DEMP2
	0,									// WP_FLECHETTE
	ROCKET_SPLASH_DAMAGE,				// WP_ROCKET_LAUNCHER
	TD_SPLASH_DAM,						// WP_THERMAL
	LT_SPLASH_DAM,						// WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_DAMAGE,		// WP_DET_PACK									// HACK, this is what the code sez.
	0,									// WP_STUN_BATON
	0,									// WP_MELEE
	0,									// WP_EMPLACED
	0,									// WP_BOT_LASER
	0,									// WP_TURRET
	0,									// WP_ATST_MAIN
	ATST_SIDE_MAIN_SPLASH_DAMAGE,		// WP_ATST_SIDE
	0,									// WP_TIE_FIGHTER
	0,									// WP_RAPID_FIRE_CONC
	0									// WP_BLASTER_PISTOL
};

const float defaultSplashRadius[] = {
	0,									// WP_NONE
	0,									// WP_SABER
	0,									// WP_BRYAR_PISTOL
	0,									// WP_BLASTER
	0,									// WP_DISRUPTOR
	BOWCASTER_SPLASH_RADIUS,			// WP_BOWCASTER
	0,									// WP_REPEATER
	0,									// WP_DEMP2
	0,									// WP_FLECHETTE
	ROCKET_SPLASH_RADIUS,				// WP_ROCKET_LAUNCHER
	TD_SPLASH_RAD,						// WP_THERMAL
	LT_SPLASH_RAD,						// WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_RADIUS,		// WP_DET_PACK									// HACK, this is what the code sez.
	0,									// WP_STUN_BATON
	0,									// WP_MELEE
	0,									// WP_EMPLACED
	0,									// WP_BOT_LASER
	0,									// WP_TURRET
	0,									// WP_ATST_MAIN
	ATST_SIDE_MAIN_SPLASH_RADIUS,		// WP_ATST_SIDE
	0,									// WP_TIE_FIGHTER
	0,									// WP_RAPID_FIRE_CONC
	0									// WP_BLASTER_PISTOL
};

const int defaultAltSplashDamage[] = {
	0,									// WP_NONE
	0,									// WP_SABER										// handled elsewhere
	0,									// WP_BRYAR_PISTOL
	0,									// WP_BLASTER
	0,									// WP_DISRUPTOR
	BOWCASTER_SPLASH_DAMAGE,			// WP_BOWCASTER
	REPEATER_ALT_SPLASH_DAMAGE,			// WP_REPEATER
	DEMP2_ALT_DAMAGE,					// WP_DEMP2
	FLECHETTE_ALT_SPLASH_DAM,			// WP_FLECHETTE
	ROCKET_SPLASH_DAMAGE,				// WP_ROCKET_LAUNCHER
	TD_ALT_SPLASH_DAM,					// WP_THERMAL
	TD_ALT_SPLASH_DAM,					// WP_TRIP_MINE
	FLECHETTE_MINE_SPLASH_DAMAGE,		// WP_DET_PACK									// HACK, this is what the code sez.
	0,									// WP_STUN_BATON
	0,									// WP_MELEE										// handled by the melee attack function
	0,									// WP_EMPLACED
	0,									// WP_BOT_LASER
	0,									// WP_TURRET									// handled elsewhere
	0,									// WP_ATST_MAIN
	ATST_SIDE_ALT_SPLASH_DAMAGE,		// WP_ATST_SIDE
	0,									// WP_TIE_FIGHTER
	0,									// WP_RAPID_FIRE_CONC
	0									// WP_BLASTER_PISTOL
};

const float defaultAltSplashRadius[] = {
	0,							// WP_NONE
	0,							// WP_SABER										// handled elsewhere
	0,							// WP_BRYAR_PISTOL
	0,							// WP_BLASTER
	0,							// WP_DISRUPTOR
	BOWCASTER_SPLASH_RADIUS,	// WP_BOWCASTER
	REPEATER_ALT_SPLASH_RADIUS,	// WP_REPEATER
	DEMP2_ALT_SPLASHRADIUS,		// WP_DEMP2
	FLECHETTE_ALT_SPLASH_RAD,	// WP_FLECHETTE
	ROCKET_SPLASH_RADIUS,		// WP_ROCKET_LAUNCHER
	TD_ALT_SPLASH_RAD,			// WP_THERMAL
	LT_SPLASH_RAD,				// WP_TRIP_MINE
	FLECHETTE_ALT_SPLASH_RAD,	// WP_DET_PACK									// HACK, this is what the code sez.
	0,							// WP_STUN_BATON
	0,							// WP_MELEE										// handled by the melee attack function
	0,							// WP_EMPLACED
	0,							// WP_BOT_LASER
	0,							// WP_TURRET									// handled elsewhere
	0,							// WP_ATST_MAIN
	ATST_SIDE_ALT_SPLASH_RADIUS,// WP_ATST_SIDE
	0,							// WP_TIE_FIGHTER
	0,							// WP_RAPID_FIRE_CONC
	0							// WP_BLASTER_PISTOL
};

wpnParms_t WpnParms[] =
{
	{ "ammo",				WPN_Ammo },	//ammo
	{ "ammoicon",			WPN_AmmoIcon },
	{ "ammomax",			WPN_AmmoMax },
	{ "ammolowcount",		WPN_AmmoLowCnt }, //weapons
	{ "ammotype",			WPN_AmmoType },
	{ "energypershot",	WPN_EnergyPerShot },
	{ "fireTime",			WPN_FireTime },
	{ "firingsound",		WPN_FiringSnd },
	{ "altfiringsound",	WPN_AltFiringSnd },
	{ "stopsound",		WPN_StopSnd },
	{ "chargesound",		WPN_ChargeSnd },
	{ "altchargesound",	WPN_AltChargeSnd },
	{ "selectsound",		WPN_SelectSnd },
	{ "range",			WPN_Range },
	{ "weaponclass",		WPN_WeaponClass },
	{ "weaponicon",		WPN_WeaponIcon },
	{ "weaponmodel",		WPN_WeaponModel },
	{ "weapontype",		WPN_WeaponType },
	{ "altenergypershot",	WPN_AltEnergyPerShot },
	{ "altfireTime",		WPN_AltFireTime },
	{ "altrange",			WPN_AltRange },
	{ "barrelcount",		WPN_BarrelCount },
	{ "missileModel",		WPN_MissileName },
	{ "altmissileModel", 	WPN_AltMissileName },
	{ "missileSound",		WPN_MissileSound },
	{ "altmissileSound", 	WPN_AltMissileSound },
	{ "missileLight",		WPN_MissileLight },
	{ "altmissileLight", 	WPN_AltMissileLight },
	{ "missileLightColor",WPN_MissileLightColor },
	{ "altmissileLightColor",	WPN_AltMissileLightColor },
	{ "missileFuncName",		WPN_FuncName },
	{ "altmissileFuncName",	WPN_AltFuncName },
	{ "missileHitSound",		WPN_MissileHitSound },
	{ "altmissileHitSound",	WPN_AltMissileHitSound },
	{ "muzzleEffect",			WPN_MuzzleEffect },
	{ "altmuzzleEffect",		WPN_AltMuzzleEffect },
	// OPENJK NEW FIELDS
	{ "damage",			WPN_Damage },
	{ "altdamage",		WPN_AltDamage },
	{ "splashDamage",		WPN_SplashDamage },
	{ "splashRadius",		WPN_SplashRadius },
	{ "altSplashDamage",	WPN_AltSplashDamage },
	{ "altSplashRadius",	WPN_AltSplashRadius },

	// Old legacy files contain these, so we skip them to shut up warnings
	{ "firingforce",		WPN_FuncSkip },
	{ "chargeforce",		WPN_FuncSkip },
	{ "altchargeforce",	WPN_FuncSkip },
	{ "selectforce",		WPN_FuncSkip },
};

static const size_t numWpnParms = ARRAY_LEN(WpnParms);

void WPN_FuncSkip( const char **holdBuf)
{
	SkipRestOfLine(holdBuf);
}

void WPN_WeaponType( const char **holdBuf)
{
	int weaponNum;
	const char	*tokenStr;

	if (COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	// FIXME : put this in an array (maybe a weaponDataInternal array???)
	if (!Q_stricmp(tokenStr,"WP_NONE"))
		weaponNum = WP_NONE;
	else if (!Q_stricmp(tokenStr,"WP_SABER"))
		weaponNum = WP_SABER;
	else if (!Q_stricmp(tokenStr,"WP_BRYAR_PISTOL"))
		weaponNum = WP_BRYAR_PISTOL;
	else if (!Q_stricmp(tokenStr,"WP_BLASTER"))
		weaponNum = WP_BLASTER;
	else if (!Q_stricmp(tokenStr,"WP_DISRUPTOR"))
		weaponNum = WP_DISRUPTOR;
	else if (!Q_stricmp(tokenStr,"WP_BOWCASTER"))
		weaponNum = WP_BOWCASTER;
	else if (!Q_stricmp(tokenStr,"WP_REPEATER"))
		weaponNum = WP_REPEATER;
	else if (!Q_stricmp(tokenStr,"WP_DEMP2"))
		weaponNum = WP_DEMP2;
	else if (!Q_stricmp(tokenStr,"WP_FLECHETTE"))
		weaponNum = WP_FLECHETTE;
	else if (!Q_stricmp(tokenStr,"WP_ROCKET_LAUNCHER"))
		weaponNum = WP_ROCKET_LAUNCHER;
	else if (!Q_stricmp(tokenStr,"WP_THERMAL"))
		weaponNum = WP_THERMAL;
	else if (!Q_stricmp(tokenStr,"WP_TRIP_MINE"))
		weaponNum = WP_TRIP_MINE;
	else if (!Q_stricmp(tokenStr,"WP_DET_PACK"))
		weaponNum = WP_DET_PACK;
	else if (!Q_stricmp(tokenStr,"WP_STUN_BATON"))
		weaponNum = WP_STUN_BATON;
	else if (!Q_stricmp(tokenStr,"WP_BOT_LASER"))
		weaponNum = WP_BOT_LASER;
	else if (!Q_stricmp(tokenStr,"WP_EMPLACED_GUN"))
		weaponNum = WP_EMPLACED_GUN;
	else if (!Q_stricmp(tokenStr,"WP_MELEE"))
		weaponNum = WP_MELEE;
	else if (!Q_stricmp(tokenStr,"WP_TURRET"))
		weaponNum = WP_TURRET;
	else if (!Q_stricmp(tokenStr,"WP_ATST_MAIN"))
		weaponNum = WP_ATST_MAIN;
	else if (!Q_stricmp(tokenStr,"WP_ATST_SIDE"))
		weaponNum = WP_ATST_SIDE;
	else if (!Q_stricmp(tokenStr,"WP_TIE_FIGHTER"))
		weaponNum = WP_TIE_FIGHTER;
	else if (!Q_stricmp(tokenStr,"WP_RAPID_FIRE_CONC"))
		weaponNum = WP_RAPID_FIRE_CONC;
	else if (!Q_stricmp(tokenStr,"WP_BLASTER_PISTOL"))
		weaponNum = WP_BLASTER_PISTOL;
	else
	{
		weaponNum = 0;
		gi.Printf(S_COLOR_YELLOW"WARNING: bad weapontype in external weapon data '%s'\n", tokenStr);
	}

	wpnParms.weaponNum = weaponNum;
}

//--------------------------------------------
void WPN_WeaponClass(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if (COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 32)
	{
		len = 32;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponclass too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].classname,tokenStr,len);
}


//--------------------------------------------
void WPN_WeaponModel(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponMdl too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].weaponMdl,tokenStr,len);
}

//--------------------------------------------
void WPN_WeaponIcon(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: weaponIcon too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].weaponIcon,tokenStr,len);
}

//--------------------------------------------
void WPN_AmmoType(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < AMMO_NONE ) || (tokenInt >= AMMO_MAX ))
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammotype in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].ammoIndex = tokenInt;
}

//--------------------------------------------
void WPN_AmmoLowCnt(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 100 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammolowcount in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].ammoLow = tokenInt;
}

//--------------------------------------------
void WPN_FiringSnd(const char **holdBuf)
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: firingSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].firingSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_AltFiringSnd( const char **holdBuf )
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: altFiringSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altFiringSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_StopSnd( const char **holdBuf )
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: stopSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].stopSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_ChargeSnd(const char **holdBuf)
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: chargeSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].chargeSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_AltChargeSnd(const char **holdBuf)
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: altChargeSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altChargeSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_SelectSnd( const char **holdBuf )
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString( holdBuf,&tokenStr ))
	{
		return;
	}

	len = strlen( tokenStr );
	len++;

	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: selectSnd too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz( weaponData[wpnParms.weaponNum].selectSnd,tokenStr,len);
}

//--------------------------------------------
void WPN_FireTime(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 10000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Firetime in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].fireTime = tokenInt;
}

//--------------------------------------------
void WPN_Range(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 10000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Range in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].range = tokenInt;
}

//--------------------------------------------
void WPN_EnergyPerShot(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 1000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad EnergyPerShot in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].energyPerShot = tokenInt;
}

//--------------------------------------------
void WPN_AltFireTime(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 10000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad altFireTime in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].altFireTime = tokenInt;
}

//--------------------------------------------
void WPN_AltRange(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 10000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad AltRange in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].altRange = tokenInt;
}

//--------------------------------------------
void WPN_AltEnergyPerShot(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 1000 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad AltEnergyPerShot in external weapon data '%d'\n", tokenInt);
		return;
	}
	weaponData[wpnParms.weaponNum].altEnergyPerShot = tokenInt;
}

//--------------------------------------------
void WPN_Ammo(const char **holdBuf)
{
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	if (!Q_stricmp(tokenStr,"AMMO_NONE"))
		wpnParms.ammoNum = AMMO_NONE;
	else if (!Q_stricmp(tokenStr,"AMMO_FORCE"))
		wpnParms.ammoNum = AMMO_FORCE;
	else if (!Q_stricmp(tokenStr,"AMMO_BLASTER"))
		wpnParms.ammoNum = AMMO_BLASTER;
	else if (!Q_stricmp(tokenStr,"AMMO_POWERCELL"))
		wpnParms.ammoNum = AMMO_POWERCELL;
	else if (!Q_stricmp(tokenStr,"AMMO_METAL_BOLTS"))
		wpnParms.ammoNum = AMMO_METAL_BOLTS;
	else if (!Q_stricmp(tokenStr,"AMMO_ROCKETS"))
		wpnParms.ammoNum = AMMO_ROCKETS;
	else if (!Q_stricmp(tokenStr,"AMMO_EMPLACED"))
		wpnParms.ammoNum = AMMO_EMPLACED;
	else if (!Q_stricmp(tokenStr,"AMMO_THERMAL"))
		wpnParms.ammoNum = AMMO_THERMAL;
	else if (!Q_stricmp(tokenStr,"AMMO_TRIPMINE"))
		wpnParms.ammoNum = AMMO_TRIPMINE;
	else if (!Q_stricmp(tokenStr,"AMMO_DETPACK"))
		wpnParms.ammoNum = AMMO_DETPACK;
	else
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad ammotype in external weapon data '%s'\n", tokenStr);
		wpnParms.ammoNum = 0;
	}
}

//--------------------------------------------
void WPN_AmmoIcon(const char **holdBuf)
{
	const char	*tokenStr;
	int		len;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: ammoicon too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(ammoData[wpnParms.ammoNum].icon,tokenStr,len);

}

//--------------------------------------------
void WPN_AmmoMax(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 1000 ))
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Ammo Max in external weapon data '%d'\n", tokenInt);
		return;
	}
	ammoData[wpnParms.ammoNum].max = tokenInt;
}

//--------------------------------------------
void WPN_BarrelCount(const char **holdBuf)
{
	int		tokenInt;

	if ( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	if ((tokenInt < 0) || (tokenInt > 4 ))
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad Range in external weapon data '%d'\n", tokenInt);
		return;
	}

	weaponData[wpnParms.weaponNum].numBarrels = tokenInt;
}


//--------------------------------------------
static void WP_ParseWeaponParms(const char **holdBuf)
{
	const char	*token;
	size_t	i;

	while (holdBuf)
	{
		token = COM_ParseExt( holdBuf, qtrue );

		if (!Q_stricmp( token, "}" ))	// End of data for this weapon
			break;

		// Loop through possible parameters
		for (i=0;i<numWpnParms;++i)
		{
			if (!Q_stricmp(token,WpnParms[i].parmName))
			{
				WpnParms[i].func(holdBuf);
				break;
			}
		}

		if (i < numWpnParms)	// Find parameter???
		{
			continue;
		}
		Com_Printf("^3WARNING: bad parameter in external weapon data '%s'\n", token); // errors are far too serious for me
		//Com_Error(ERR_FATAL,"bad parameter in external weapon data '%s'\n", token);
	}
}

//--------------------------------------------
void WPN_MissileName(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MissileName too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missileMdl,tokenStr,len);

}

//--------------------------------------------
void WPN_AltMissileName(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissileName too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].alt_missileMdl,tokenStr,len);

}


//--------------------------------------------
void WPN_MissileHitSound(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MissileHitSound too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missileHitSound,tokenStr,len);
}

//--------------------------------------------
void WPN_AltMissileHitSound(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissileHitSound too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].altmissileHitSound,tokenStr,len);
}

//--------------------------------------------
void WPN_MissileSound(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MissileSound too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].missileSound,tokenStr,len);

}


//--------------------------------------------
void WPN_AltMissileSound(const char **holdBuf)
{
	int len;
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMissileSound too long in external WEAPONS.DAT '%s'\n", tokenStr);
	}

	Q_strncpyz(weaponData[wpnParms.weaponNum].alt_missileSound,tokenStr,len);

}

//--------------------------------------------
void WPN_MissileLightColor(const char **holdBuf)
{
	int i;
	float	tokenFlt;

	for (i=0;i<3;++i)
	{
		if ( COM_ParseFloat(holdBuf,&tokenFlt))
		{
			SkipRestOfLine(holdBuf);
			continue;
		}

		if ((tokenFlt < 0) || (tokenFlt > 1 ))
		{
			gi.Printf(S_COLOR_YELLOW"WARNING: bad missilelightcolor in external weapon data '%f'\n", tokenFlt);
			continue;
		}
		weaponData[wpnParms.weaponNum].missileDlightColor[i] = tokenFlt;
	}

}

//--------------------------------------------
void WPN_AltMissileLightColor(const char **holdBuf)
{
	int i;
	float	tokenFlt;

	for (i=0;i<3;++i)
	{
		if ( COM_ParseFloat(holdBuf,&tokenFlt))
		{
			SkipRestOfLine(holdBuf);
			continue;
		}

		if ((tokenFlt < 0) || (tokenFlt > 1 ))
		{
			gi.Printf(S_COLOR_YELLOW"WARNING: bad altmissilelightcolor in external weapon data '%f'\n", tokenFlt);
			continue;
		}
		weaponData[wpnParms.weaponNum].alt_missileDlightColor[i] = tokenFlt;
	}

}


//--------------------------------------------
void WPN_MissileLight(const char **holdBuf)
{
	float	tokenFlt;

	if ( COM_ParseFloat(holdBuf,&tokenFlt))
	{
		SkipRestOfLine(holdBuf);
	}

	if ((tokenFlt < 0) || (tokenFlt > 255 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad missilelight in external weapon data '%f'\n", tokenFlt);
	}
	weaponData[wpnParms.weaponNum].missileDlight = tokenFlt;
}

//--------------------------------------------
void WPN_AltMissileLight(const char **holdBuf)
{
	float	tokenFlt;

	if ( COM_ParseFloat(holdBuf,&tokenFlt))
	{
		SkipRestOfLine(holdBuf);
	}

	if ((tokenFlt < 0) || (tokenFlt > 255 )) // FIXME :What are the right values?
	{
		gi.Printf(S_COLOR_YELLOW"WARNING: bad altmissilelight in external weapon data '%f'\n", tokenFlt);
	}
	weaponData[wpnParms.weaponNum].alt_missileDlight = tokenFlt;
}


//--------------------------------------------
void WPN_FuncName(const char **holdBuf)
{
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	size_t len = strlen(tokenStr);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: FuncName '%s' too long in external WEAPONS.DAT\n", tokenStr);
	}

	for ( func_t* s=funcs ; s->name ; s++ ) {
		if ( !Q_stricmp(s->name, tokenStr) ) {
			// found it
			weaponData[wpnParms.weaponNum].func = (void*)s->func;
			return;
		}
	}
	gi.Printf(S_COLOR_YELLOW"WARNING: FuncName '%s' in external WEAPONS.DAT does not exist\n", tokenStr);
}


//--------------------------------------------
void WPN_AltFuncName(const char **holdBuf)
{
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	size_t len = strlen(tokenStr);
	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltFuncName '%s' too long in external WEAPONS.DAT\n", tokenStr);
	}

	for ( func_t* s=funcs ; s->name ; s++ ) {
		if ( !Q_stricmp(s->name, tokenStr) ) {
			// found it
			weaponData[wpnParms.weaponNum].altfunc = (void*)s->func;
			return;
		}
	}
	gi.Printf(S_COLOR_YELLOW"WARNING: AltFuncName %s in external WEAPONS.DAT does not exist\n", tokenStr);
}

//--------------------------------------------
void WPN_MuzzleEffect(const char **holdBuf)
{
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	size_t len = strlen(tokenStr);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: MuzzleEffect '%s' too long in external WEAPONS.DAT\n", tokenStr);
	}

	G_EffectIndex( tokenStr );
	Q_strncpyz(weaponData[wpnParms.weaponNum].mMuzzleEffect,tokenStr,len);
}

//--------------------------------------------
void WPN_AltMuzzleEffect(const char **holdBuf)
{
	const char	*tokenStr;

	if ( COM_ParseString(holdBuf,&tokenStr))
	{
		return;
	}

	size_t len = strlen(tokenStr);

	len++;
	if (len > 64)
	{
		len = 64;
		gi.Printf(S_COLOR_YELLOW"WARNING: AltMuzzleEffect '%s' too long in external WEAPONS.DAT\n", tokenStr);
	}

	G_EffectIndex( tokenStr );
	Q_strncpyz(weaponData[wpnParms.weaponNum].mAltMuzzleEffect,tokenStr,len);
}

//--------------------------------------------

void WPN_Damage(const char **holdBuf)
{
	int		tokenInt;

	if( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].damage = tokenInt;
}

//--------------------------------------------

void WPN_AltDamage(const char **holdBuf)
{
	int		tokenInt;

	if( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].altDamage = tokenInt;
}

//--------------------------------------------

void WPN_SplashDamage(const char **holdBuf)
{
	int		tokenInt;

	if( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].splashDamage = tokenInt;
}

//--------------------------------------------

void WPN_SplashRadius(const char **holdBuf)
{
	float	tokenFlt;

	if( COM_ParseFloat(holdBuf,&tokenFlt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].splashRadius = tokenFlt;
}

//--------------------------------------------

void WPN_AltSplashDamage(const char **holdBuf)
{
	int		tokenInt;

	if( COM_ParseInt(holdBuf,&tokenInt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].altSplashDamage = tokenInt;
}

//--------------------------------------------

void WPN_AltSplashRadius(const char **holdBuf)
{
	float	tokenFlt;

	if( COM_ParseFloat(holdBuf,&tokenFlt))
	{
		SkipRestOfLine(holdBuf);
		return;
	}

	weaponData[wpnParms.weaponNum].altSplashRadius = tokenFlt;
}

//--------------------------------------------
static void WP_ParseParms(const char *buffer)
{
	const char	*holdBuf;
	const char	*token;

	holdBuf = buffer;
	COM_BeginParseSession();

	while ( holdBuf )
	{
		token = COM_ParseExt( &holdBuf, qtrue );

		if ( !Q_stricmp( token, "{" ) )
		{
			WP_ParseWeaponParms(&holdBuf);
		}

	}

	COM_EndParseSession(  );

}

//--------------------------------------------
void WP_LoadWeaponParms (void)
{
	char *buffer;

	gi.FS_ReadFile("ext_data/weapons.dat",(void **) &buffer);

	// initialise the data area
	memset(weaponData, 0, sizeof(weaponData));

	// put in the default values, because backwards compatibility is awesome!
	for(int i = 0; i < WP_NUM_WEAPONS; i++)
	{
		weaponData[i].damage = defaultDamage[i];
		weaponData[i].altDamage = defaultAltDamage[i];
		weaponData[i].splashDamage = defaultSplashDamage[i];
		weaponData[i].altSplashDamage = defaultAltSplashDamage[i];
		weaponData[i].splashRadius = defaultSplashRadius[i];
		weaponData[i].altSplashRadius = defaultAltSplashRadius[i];
	}

	WP_ParseParms(buffer);

	gi.FS_FreeFile( buffer );	//let go of the buffer
}