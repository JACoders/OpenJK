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

#include "../../code/client/vmachine.h"
#include "../game/characters.h"

#include "../../code/qcommon/sstring.h"
//NOTENOTE: Be sure to change the mirrored code in g_shared.h
typedef	map< sstring_t, unsigned char, less<sstring_t>, allocator< unsigned char >  >	namePrecache_m;
extern namePrecache_m	*as_preCacheMap;
extern void CG_RegisterNPCCustomSounds( clientInfo_t *ci );
extern qboolean G_AddSexToMunroString ( char *string, qboolean qDoBoth );
extern void CG_RegisterNPCEffects( team_t team );
extern qboolean G_ParseAnimFileSet( const char *filename, const char *animCFG, int *animFileIndex );
extern void CG_DrawDataPadInventorySelect( void );

void CG_Init( int serverCommandSequence );
qboolean CG_ConsoleCommand( void );
void CG_Shutdown( void );
int CG_GetCameraPos( vec3_t camerapos );
void UseItem(int itemNum);
const char *CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight,
								const char *psText, int iFontHandle, float fScale,
								const vec4_t v4Color);

#define NUM_CHUNKS		6
/*
Ghoul2 Insert Start
*/

void CG_ResizeG2Bolt(boltInfo_v *bolt, int newCount);
void CG_ResizeG2Surface(surfaceInfo_v *surface, int newCount);
void CG_ResizeG2Bone(boneInfo_v *bone, int newCount);
void CG_ResizeG2(CGhoul2Info_v *ghoul2, int newCount);
void CG_ResizeG2TempBone(mdxaBone_v *tempBone, int newCount);
/*
Ghoul2 Insert End
*/


void CG_LoadHudMenu(void);
int inv_icons[INV_MAX];
const char *inv_names[] =
{
"ELECTROBINOCULARS",
"BACTA CANISTER",
"SEEKER",
"LIGHT AMP GOGGLES",
"ASSAULT SENTRY",
"GOODIE KEY",
"GOODIE KEY",
"GOODIE KEY",
"GOODIE KEY",
"GOODIE KEY",
"SECURITY KEY",
"SECURITY KEY",
"SECURITY KEY",
"SECURITY KEY",
"SECURITY KEY",
};

int	force_icons[NUM_FORCE_POWERS];


int cgi_UI_GetMenuInfo(char *menuFile,int *x,int *y);
void CG_DrawDataPadHUD( centity_t *cent );
void MissionInformation_Draw( centity_t *cent );
void CG_DrawIconBackground(void);
void CG_DrawDataPadIconBackground(int backgroundType);
void CG_DrawDataPadWeaponSelect( void );
void CG_DrawDataPadForceSelect( void );

/*
================
vmMain

This is the only way control passes into the cgame module.
This must be the very first function compiled into the .q3vm file
================
*/
extern "C" Q_EXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7 ) {
	centity_t		*cent;

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, (stereoFrame_t) arg1 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_CAMERA_POS:
		return CG_GetCameraPos( (float*)arg0);
/*
Ghoul2 Insert Start
*/
	case CG_RESIZE_G2:
		CG_ResizeG2((CGhoul2Info_v *)arg0, arg1);
		return 0;
	case CG_RESIZE_G2_BOLT:
		CG_ResizeG2Bolt((boltInfo_v *)arg0, arg1);
		return 0;
	case CG_RESIZE_G2_BONE:
		CG_ResizeG2Bone((boneInfo_v *)arg0, arg1);
		return 0;
	case CG_RESIZE_G2_SURFACE:
		CG_ResizeG2Surface((surfaceInfo_v *)arg0, arg1);
		return 0;
	case CG_RESIZE_G2_TEMPBONE:
		CG_ResizeG2TempBone((mdxaBone_v *)arg0, arg1);
		return 0;

/*
Ghoul2 Insert End
*/
	case CG_DRAW_DATAPAD_HUD:
		if (cg.snap)
		{
			cent = &cg_entities[cg.snap->ps.clientNum];
			CG_DrawDataPadHUD(cent);
		}
		return 0;

	case CG_DRAW_DATAPAD_OBJECTIVES:
		if (cg.snap)
		{
			cent = &cg_entities[cg.snap->ps.clientNum];
			MissionInformation_Draw(cent);
		}
		return 0;

	case CG_DRAW_DATAPAD_WEAPONS:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_WEAPONS);
			CG_DrawDataPadWeaponSelect();
		}
		return 0;
	case CG_DRAW_DATAPAD_INVENTORY:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_INVENTORY);
			CG_DrawDataPadInventorySelect();
		}
		return 0;
	case CG_DRAW_DATAPAD_FORCEPOWERS:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_FORCE);
			CG_DrawDataPadForceSelect();
		}
		return 0;
	}
	return -1;
}

/*
Ghoul2 Insert Start
*/

void CG_ResizeG2Bolt(boltInfo_v *bolt, int newCount)
{
	bolt->resize(newCount);
}

void CG_ResizeG2Surface(surfaceInfo_v *surface, int newCount)
{
	surface->resize(newCount);
}

void CG_ResizeG2Bone(boneInfo_v *bone, int newCount)
{
	bone->resize(newCount);
}

void CG_ResizeG2(CGhoul2Info_v *ghoul2, int newCount)
{
	ghoul2->resize(newCount);
}

void CG_ResizeG2TempBone(mdxaBone_v *tempBone, int newCount)
{
	tempBone->resize(newCount);
}
/*
Ghoul2 Insert End
*/

cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

typedef struct {
	qboolean		registered;		// Has the player picked it up
	qboolean		active;			// Is it the chosen inventory item
	int				count;			// Count of items.
	char			description[128];
} inventoryInfo_t;

inventoryInfo_t		cg_inventory[INV_MAX];


vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_paused;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_crosshairIdentifyTarget;
vmCvar_t	cg_dynamicCrosshair;
vmCvar_t	cg_crosshairForceHint;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_drawHUD;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugSaber;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_fovViewmodel;
vmCvar_t	cg_fovViewmodelAdjust;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_fovAspectAdjust;
vmCvar_t	cg_missionstatusscreen;
vmCvar_t	cg_endcredits;
vmCvar_t	cg_updatedDataPadForcePower1;
vmCvar_t	cg_updatedDataPadForcePower2;
vmCvar_t	cg_updatedDataPadForcePower3;
vmCvar_t	cg_updatedDataPadObjective;

vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonMaxRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_thirdPersonPitchOffset;
vmCvar_t	cg_thirdPersonVertOffset;
vmCvar_t	cg_thirdPersonCameraDamp;
vmCvar_t	cg_thirdPersonTargetDamp;
vmCvar_t	cg_saberAutoThird;
vmCvar_t	cg_gunAutoFirst;

vmCvar_t	cg_thirdPersonAlpha;
vmCvar_t	cg_thirdPersonAutoAlpha;
vmCvar_t	cg_thirdPersonHorzOffset;

vmCvar_t	cg_stereoSeparation;
vmCvar_t 	cg_developer;
vmCvar_t 	cg_timescale;
vmCvar_t	cg_skippingcin;

vmCvar_t	cg_pano;
vmCvar_t	cg_panoNumShots;

vmCvar_t	fx_freeze;
vmCvar_t	fx_debug;

vmCvar_t	cg_missionInfoCentered;
vmCvar_t	cg_missionInfoFlashTime;
vmCvar_t	cg_hudFiles;

/*
Ghoul2 Insert Start
*/
vmCvar_t	cg_debugBB;
/*
Ghoul2 Insert End
*/

vmCvar_t	cg_VariantSoundCap;	// 0 = no capping, else cap to (n) max (typically just 1, but allows more)
vmCvar_t	cg_turnAnims;
vmCvar_t	cg_motionBoneComp;
vmCvar_t	cg_reliableAnimSounds;

vmCvar_t	cg_smoothPlayerPos;
vmCvar_t	cg_smoothPlayerPlat;
vmCvar_t	cg_smoothPlayerPlatAccel;

typedef struct {
	vmCvar_t	*vmCvar;
	const char	*cvarName;
	const char	*defaultString;
	int			cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = {
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "80", CVAR_ARCHIVE },
	{ &cg_fovAspectAdjust, "cg_fovAspectAdjust", "0", CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE  },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },

	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  },
	{ &cg_drawHUD, "cg_drawHUD", "1", 0  },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE },
	{ &cg_dynamicCrosshair, "cg_dynamicCrosshair", "1", CVAR_ARCHIVE },
	{ &cg_crosshairIdentifyTarget, "cg_crosshairIdentifyTarget", "1", CVAR_ARCHIVE },
	{ &cg_crosshairForceHint, "cg_crosshairForceHint", "1", CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART },
	{ &cg_missionstatusscreen, "cg_missionstatusscreen", "0", CVAR_ROM},
	{ &cg_endcredits, "cg_endcredits", "0", 0},
	{ &cg_updatedDataPadForcePower1, "cg_updatedDataPadForcePower1", "0", 0},
	{ &cg_updatedDataPadForcePower2, "cg_updatedDataPadForcePower2", "0", 0},
	{ &cg_updatedDataPadForcePower3, "cg_updatedDataPadForcePower3", "0", 0},
	{ &cg_updatedDataPadObjective, "cg_updatedDataPadObjective", "0", 0},

	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },

	{ &cg_gun_frame, "gun_frame", "0", CVAR_CHEAT },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_fovViewmodel, "cg_fovViewModel", "0", CVAR_ARCHIVE },
	{ &cg_fovViewmodelAdjust, "cg_fovViewmodelAdjust", "1", CVAR_ARCHIVE },

	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_ARCHIVE },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE },

	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugSaber, "cg_debugsaber", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },

	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_SAVEGAME },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE },
	{ &cg_thirdPersonMaxRange, "cg_thirdPersonMaxRange", "150", 0 },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", 0 },
	{ &cg_thirdPersonPitchOffset, "cg_thirdPersonPitchOffset", "0", 0 },
	{ &cg_thirdPersonVertOffset, "cg_thirdPersonVertOffset", "16", 0},
	{ &cg_thirdPersonCameraDamp, "cg_thirdPersonCameraDamp", "0.3", 0},
	{ &cg_thirdPersonTargetDamp, "cg_thirdPersonTargetDamp", "0.5", 0},

	{ &cg_thirdPersonHorzOffset, "cg_thirdPersonHorzOffset", "0", 0},
	{ &cg_thirdPersonAlpha,	"cg_thirdPersonAlpha",	"1.0", CVAR_CHEAT },
	{ &cg_thirdPersonAutoAlpha,	"cg_thirdPersonAutoAlpha",	"0", 0 },

	{ &cg_saberAutoThird, "cg_saberAutoThird", "1", CVAR_ARCHIVE },
	{ &cg_gunAutoFirst, "cg_gunAutoFirst", "1", CVAR_ARCHIVE },

	{ &cg_pano, "pano", "0", 0 },
	{ &cg_panoNumShots, "panoNumShots", "10", 0 },

	{ &fx_freeze, "fx_freeze", "0", 0 },
	{ &fx_debug, "fx_debug", "0", 0 },
	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &cg_developer, "developer", "", 0 },
	{ &cg_timescale, "timescale", "1", 0 },
	{ &cg_skippingcin, "skippingCinematic", "0", CVAR_ROM},
	{ &cg_missionInfoCentered, "cg_missionInfoCentered", "1", CVAR_ARCHIVE },
	{ &cg_missionInfoFlashTime, "cg_missionInfoFlashTime", "10000", 0  },
	{ &cg_hudFiles, "cg_hudFiles", "ui/jk2hud.txt", CVAR_ARCHIVE},
/*
Ghoul2 Insert Start
*/
	{ &cg_debugBB, "debugBB", "0", 0},
/*
Ghoul2 Insert End
*/
	{ &cg_VariantSoundCap, "cg_VariantSoundCap", "0", 0 },
	{ &cg_turnAnims, "cg_turnAnims", "0", 0 },
	{ &cg_motionBoneComp, "cg_motionBoneComp", "2", 0 },
	{ &cg_reliableAnimSounds, "cg_reliableAnimSounds", "1", CVAR_ARCHIVE },
	{ &cg_smoothPlayerPos, "cg_smoothPlayerPos", "0.5", 0},
	{ &cg_smoothPlayerPlat, "cg_smoothPlayerPlat", "0.75", 0},
	{ &cg_smoothPlayerPlatAccel, "cg_smoothPlayerPlatAccel", "3.25", 0},
};

static const size_t cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	size_t		i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		cgi_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	size_t		i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			cgi_Cvar_Update( cv->vmCvar );
		}
	}
}

int CG_CrosshairPlayer( void )
{
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) )
	{
		return -1;
	}
	return cg.crosshairClientNum;
}


int CG_GetCameraPos( vec3_t camerapos ) {
	if ( in_camera) {
		VectorCopy(client_camera.origin, camerapos);
		return 1;
	}
	else if ( cg_entities[0].gent && cg_entities[0].gent->client && cg_entities[0].gent->client->ps.viewEntity > 0 && cg_entities[0].gent->client->ps.viewEntity < ENTITYNUM_WORLD )
	//else if ( cg.snap && cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{//in an entity camera view
		if ( g_entities[cg_entities[0].gent->client->ps.viewEntity].client && cg.renderingThirdPerson )
		{
			VectorCopy( g_entities[cg_entities[0].gent->client->ps.viewEntity].client->renderInfo.eyePoint, camerapos );
		}
		else
		{
			VectorCopy( g_entities[cg_entities[0].gent->client->ps.viewEntity].currentOrigin, camerapos );
		}
		//VectorCopy( cg_entities[cg_entities[0].gent->client->ps.viewEntity].lerpOrigin, camerapos );
		/*
		if ( g_entities[cg.snap->ps.viewEntity].client && cg.renderingThirdPerson )
		{
			VectorCopy( g_entities[cg.snap->ps.viewEntity].client->renderInfo.eyePoint, camerapos );
		}
		else
		{//use the g_ent because it may not have gotten over to the client yet...
			VectorCopy( g_entities[cg.snap->ps.viewEntity].currentOrigin, camerapos );
		}
		*/
		return 1;
	}
	else if ( cg.renderingThirdPerson )
	{//in third person
		//FIXME: what about hacks that render in third person regardless of this value?
		VectorCopy( cg.refdef.vieworg, camerapos );
		return 1;
	}
	else if (cg.snap && (cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE) )//implied: !cg.renderingThirdPerson
	{//first person saber hack
		VectorCopy( cg.refdef.vieworg, camerapos );
		return 1;
	}
	return 0;
}

void CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	cgi_Printf( text );
}

void CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	cgi_Error( text );
}


/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	cgi_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if (item->pickup_sound)
	{
		cgi_S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string",
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			cgi_S_RegisterSound( data );
		}
	}
}

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );
	cgi_UpdateScreen();
}


static void CG_AS_Register(void)
{
	CG_LoadingString( "ambient sound sets" );

	//Load the ambient sets

	cgi_AS_AddPrecacheEntry( "#clear" );	// ;-)
	//FIXME: Don't ask... I had to get around a really nasty MS error in the templates with this...
	namePrecache_m::iterator	pi;
	STL_ITERATE( pi, (*as_preCacheMap) )
	{
		cgi_AS_AddPrecacheEntry( ((*pi).first).c_str() );
	}

	cgi_AS_ParseSets();
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	name[MAX_QPATH];
	const char	*soundName;

	CG_AS_Register();

	CG_LoadingString( "general sounds" );

	//FIXME: add to cg.media?
	cgi_S_RegisterSound( "sound/player/fallsplat.wav" );

	cgs.media.selectSound = cgi_S_RegisterSound( "sound/weapons/change.wav" );
	cgs.media.selectSound2 = cgi_S_RegisterSound( "sound/interface/button1.wav" );
//	cgs.media.useNothingSound = cgi_S_RegisterSound( "sound/items/use_nothing.wav" );

	cgs.media.noAmmoSound = cgi_S_RegisterSound( "sound/weapons/noammo.wav" );
//	cgs.media.talkSound = 	cgi_S_RegisterSound( "sound/interface/communicator.wav" );
	cgs.media.landSound =	cgi_S_RegisterSound( "sound/player/land1.wav");
	cgs.media.rollSound =	cgi_S_RegisterSound( "sound/player/roll1.wav");

	cgs.media.overchargeFastSound	= cgi_S_RegisterSound("sound/weapons/overchargeFast.wav" );
	cgs.media.overchargeSlowSound	= cgi_S_RegisterSound("sound/weapons/overchargeSlow.wav" );
	cgs.media.overchargeLoopSound	= cgi_S_RegisterSound("sound/weapons/overchargeLoop.wav");
	cgs.media.overchargeEndSound	= cgi_S_RegisterSound("sound/weapons/overchargeEnd.wav");

	cgs.media.batteryChargeSound	= cgi_S_RegisterSound( "sound/interface/pickup_battery.wav" );

//	cgs.media.tedTextSound = cgi_S_RegisterSound( "sound/interface/tedtext.wav" );
	cgs.media.messageLitSound = cgi_S_RegisterSound( "sound/interface/update" );
	cg.messageLitActive = qfalse;

//	cgs.media.interfaceSnd1 = cgi_S_RegisterSound( "sound/interface/button4.wav" );
//	cgs.media.interfaceSnd2 = cgi_S_RegisterSound( "sound/interface/button2.wav" );
//	cgs.media.interfaceSnd3 = cgi_S_RegisterSound( "sound/interface/button1.wav" );

	cgs.media.watrInSound = cgi_S_RegisterSound ("sound/player/watr_in.wav");
	cgs.media.watrOutSound = cgi_S_RegisterSound ("sound/player/watr_out.wav");
	cgs.media.watrUnSound = cgi_S_RegisterSound ("sound/player/watr_un.wav");

	// Zoom
	cgs.media.zoomStart = cgi_S_RegisterSound( "sound/interface/zoomstart.wav" );
	cgs.media.zoomLoop = cgi_S_RegisterSound( "sound/interface/zoomloop.wav" );
	cgs.media.zoomEnd = cgi_S_RegisterSound( "sound/interface/zoomend.wav" );

	cgi_S_RegisterSound( "sound/chars/turret/startup.wav" );
	cgi_S_RegisterSound( "sound/chars/turret/shutdown.wav" );
	cgi_S_RegisterSound( "sound/chars/turret/ping.wav" );
	cgi_S_RegisterSound( "sound/chars/turret/move.wav" );
	cgi_S_RegisterSound( "sound/player/use_sentry" );
	cgi_R_RegisterModel( "models/items/psgun.glm" );
	theFxScheduler.RegisterEffect( "turret/explode" );
	theFxScheduler.RegisterEffect( "spark_exp_nosnd" );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/stone_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = cgi_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/metal_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = cgi_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = cgi_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WADE][i] = cgi_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_wade_0%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SWIM][i] = cgi_S_RegisterSound (name);

		// should these always be registered??
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgi_S_RegisterSound (name);
	}
	theFxScheduler.RegisterEffect( "water_impact" );

	cg.loadLCARSStage = 1;
	CG_LoadingString( "item sounds" );

	// only register the items that the server says we need
	char	items[MAX_ITEMS+1];
	//Raz: Fixed buffer overflow
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' )	//even with sound pooling, don't clutter it for low end machines
		{
			CG_RegisterItemSounds( i );
		}
	}

	cg.loadLCARSStage = 2;
	CG_LoadingString( "preregistered sounds" );

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		if (i&31) {
			CG_LoadingString( soundName );
		}
		cgs.sound_precache[i] = cgi_S_RegisterSound( soundName );
	}
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

qhandle_t CG_RegisterHeadSkin( const char *headModelName, const char *headSkinName, qboolean *extensions )
{
	char		hfilename[MAX_QPATH];
	qhandle_t	headSkin;

	Com_sprintf( hfilename, sizeof( hfilename ), "models/players/%s/head_%s.skin", headModelName, headSkinName );
	headSkin = cgi_R_RegisterSkin( hfilename );
	if ( headSkin < 0 )
	{	//have extensions
		*extensions = qtrue;
		headSkin = -headSkin;
	}
	else
	{
		*extensions = qfalse;	//just to be sure.
	}

	if ( !headSkin )
	{
		Com_Printf( "Failed to load skin file: %s : %s\n", headModelName, headSkinName );
	}
	return headSkin;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
qboolean	CG_RegisterClientSkin( clientInfo_t *ci,
								  const char *headModelName, const char *headSkinName,
								  const char *torsoModelName, const char *torsoSkinName,
								  const char *legsModelName, const char *legsSkinName)
{
	char		hfilename[MAX_QPATH];
	char		tfilename[MAX_QPATH];
	char		lfilename[MAX_QPATH];

	Com_sprintf( lfilename, sizeof( lfilename ), "models/players/%s/lower_%s.skin", legsModelName, legsSkinName );
	ci->legsSkin = cgi_R_RegisterSkin( lfilename );

	if ( !ci->legsSkin )
	{
//		Com_Printf( "Failed to load skin file: %s : %s\n", legsModelName, legsSkinName );
		//return qfalse;
	}

	if(torsoModelName && torsoSkinName && torsoModelName[0] && torsoSkinName[0])
	{
		Com_sprintf( tfilename, sizeof( tfilename ), "models/players/%s/upper_%s.skin", torsoModelName, torsoSkinName );
		ci->torsoSkin = cgi_R_RegisterSkin( tfilename );

		if ( !ci->torsoSkin )
		{
			Com_Printf( "Failed to load skin file: %s : %s\n", torsoModelName, torsoSkinName );
			return qfalse;
		}
	}

	if(headModelName && headSkinName && headModelName[0] && headSkinName[0])
	{
		Com_sprintf( hfilename, sizeof( hfilename ), "models/players/%s/head_%s.skin", headModelName, headSkinName );
		ci->headSkin = cgi_R_RegisterSkin( hfilename );
		if (ci->headSkin <0) {	//have extensions
			ci->extensions = qtrue;
			ci->headSkin = -ci->headSkin;
		} else {
			ci->extensions = qfalse;	//just to be sure.
		}

		if ( !ci->headSkin )
		{
			Com_Printf( "Failed to load skin file: %s : %s\n", headModelName, headSkinName );
			return qfalse;
		}
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
qboolean CG_RegisterClientModelname( clientInfo_t *ci,
									const char *headModelName, const char *headSkinName,
									const char *torsoModelName, const char *torsoSkinName,
									const char *legsModelName, const char *legsSkinName )
{
/*
Ghoul2 Insert Start
*/
#if 1
	char		filename[MAX_QPATH];


	if ( !legsModelName || !legsModelName[0] )
	{
		return qtrue;
	}
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.mdr", legsModelName );
	ci->legsModel = cgi_R_RegisterModel( filename );
	if ( !ci->legsModel )
	{//he's not skeletal, try the old way
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", legsModelName );
		ci->legsModel = cgi_R_RegisterModel( filename );
		if ( !ci->legsModel )
		{
			Com_Printf( S_COLOR_RED"Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	if(torsoModelName && torsoModelName[0])
	{//You are trying to set one
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.mdr", torsoModelName );
		ci->torsoModel = cgi_R_RegisterModel( filename );
		if ( !ci->torsoModel )
		{//he's not skeletal, try the old way
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", torsoModelName );
			ci->torsoModel = cgi_R_RegisterModel( filename );
			if ( !ci->torsoModel )
			{
				Com_Printf( S_COLOR_RED"Failed to load model file %s\n", filename );
				return qfalse;
			}
		}
	}
	else
	{
		ci->torsoModel = 0;
	}

	if(headModelName && headModelName[0])
	{//You are trying to set one
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", headModelName );
		ci->headModel = cgi_R_RegisterModel( filename );
		if ( !ci->headModel )
		{
			Com_Printf( S_COLOR_RED"Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	else
	{
		ci->headModel = 0;
	}


	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName, legsSkinName ) )
	{
		//Com_Printf( "Failed to load skin file: %s : %s/%s : %s/%s : %s\n", headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName, legsSkinName );
		return qfalse;
	}

	//FIXME: for now, uses the legs model dir for anim cfg, but should we set this in some sort of NPCs.cfg?
	// load the animation file set
	if ( !G_ParseAnimFileSet( legsModelName, legsModelName, &ci->animFileIndex ) )
	{
		Com_Printf( S_COLOR_RED"Failed to load animation file set models/players/%s\n", legsModelName );
		return qfalse;
	}
#endif
/*
Ghoul2 Insert End
*/
	return qtrue;
}


void CG_RegisterClientRenderInfo(clientInfo_t *ci, renderInfo_t *ri)
{
	char			*slash;
	char			headModelName[MAX_QPATH];
	char			torsoModelName[MAX_QPATH];
	char			legsModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
	char			torsoSkinName[MAX_QPATH];
	char			legsSkinName[MAX_QPATH];

	if(!ri->legsModelName || !ri->legsModelName[0])
	{//Must have at LEAST a legs model
		return;
	}

	Q_strncpyz( legsModelName, ri->legsModelName, sizeof( legsModelName ) );
	//Legs skin
	slash = strchr( legsModelName, '/' );
	if ( !slash )
	{
		// modelName didn not include a skin name
		Q_strncpyz( legsSkinName, "default", sizeof( legsSkinName ) );
	}
	else
	{
		Q_strncpyz( legsSkinName, slash + 1, sizeof( legsSkinName ) );
		// truncate modelName
		*slash = 0;
	}

	if(ri->torsoModelName && ri->torsoModelName[0])
	{
		Q_strncpyz( torsoModelName, ri->torsoModelName, sizeof( torsoModelName ) );
		//Torso skin
		slash = strchr( torsoModelName, '/' );
		if ( !slash )
		{
			// modelName didn't include a skin name
			Q_strncpyz( torsoSkinName, "default", sizeof( torsoSkinName ) );
		}
		else
		{
			Q_strncpyz( torsoSkinName, slash + 1, sizeof( torsoSkinName ) );
			// truncate modelName
			*slash = 0;
		}
	}
	else
	{
		torsoModelName[0] = 0;
	}

	//Head
	if(ri->headModelName && ri->headModelName[0])
	{
		Q_strncpyz( headModelName, ri->headModelName, sizeof( headModelName ) );
		//Head skin
		slash = strchr( headModelName, '/' );
		if ( !slash )
		{
			// modelName didn not include a skin name
			Q_strncpyz( headSkinName, "default", sizeof( headSkinName ) );
		}
		else
		{
			Q_strncpyz( headSkinName, slash + 1, sizeof( headSkinName ) );
			// truncate modelName
			*slash = 0;
		}
	}
	else
	{
		headModelName[0] = 0;
	}

	if ( !CG_RegisterClientModelname( ci, headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName, legsSkinName) )
	{
		if ( !CG_RegisterClientModelname( ci, DEFAULT_HEADMODEL, "default", DEFAULT_TORSOMODEL, "default", DEFAULT_LEGSMODEL, "default" ) )
		{
			CG_Error( "DEFAULT_MODELS failed to register");
		}
	}
}

//-------------------------------------
// CG_RegisterEffects
//
// Handles precaching all effect files
//	and any shader, model, or sound
//	files an effect may use.
//-------------------------------------
extern void CG_InitGlass( void );
extern void	cgi_R_WorldEffectCommand( const char *command );

static void CG_RegisterEffects( void )
{
	char	*effectName;
	int		i;

	// Register external effects
	for ( i = 1 ; i < MAX_FX ; i++ )
	{
		effectName = ( char *)CG_ConfigString( CS_EFFECTS + i );

		if ( !effectName[0] )
		{
			break;
		}

		theFxScheduler.RegisterEffect( (const char*)effectName );
	}

	// Start world effects
	for ( i = 1 ; i < MAX_WORLD_FX ; i++ )
	{
		effectName = ( char *)CG_ConfigString( CS_WORLD_FX + i );

		if ( !effectName[0] )
		{
			break;
		}

		cgi_R_WorldEffectCommand( effectName );
	}

	// Set up the glass effects mini-system.
	CG_InitGlass();
}

/*
void CG_RegisterClientModels (int entityNum)

Only call if clientInfo->infoValid is not true

For players and NPCs to register their models
*/
void CG_RegisterClientModels (int entityNum)
{
	gentity_t		*ent;

	if(entityNum < 0 || entityNum > ENTITYNUM_WORLD)
	{
		return;
	}

	ent = &g_entities[entityNum];

	if(!ent->client)
	{
		return;
	}

	ent->client->clientInfo.infoValid = qtrue;

	if ( ent->playerModel != -1 && ent->ghoul2.size() )
	{
		return;
	}

	CG_RegisterClientRenderInfo(&ent->client->clientInfo, &ent->client->renderInfo);

	ent->client->clientInfo.infoValid = qtrue;

	if(entityNum < MAX_CLIENTS)
	{
		memcpy(&cgs.clientinfo[entityNum], &ent->client->clientInfo, sizeof(clientInfo_t));
	}
}

//===================================================================================

forceTicPos_t forceTicPos[] =
{
	{ 11, 41, 20, 10, "gfx/hud/force_tick1", NULL_HANDLE },		// Left Top
	{ 12, 45, 20, 10, "gfx/hud/force_tick2", NULL_HANDLE },
	{ 14, 49, 20, 10, "gfx/hud/force_tick3", NULL_HANDLE },
	{ 17, 52, 20, 10, "gfx/hud/force_tick4", NULL_HANDLE },
	{ 22, 55, 10, 10, "gfx/hud/force_tick5", NULL_HANDLE },
	{ 28, 57, 10, 20, "gfx/hud/force_tick6", NULL_HANDLE },
	{ 34, 59, 10, 10, "gfx/hud/force_tick7", NULL_HANDLE },		// Left bottom

	{ 46, 59, -10, 10, "gfx/hud/force_tick7", NULL_HANDLE },		// Right bottom
	{ 52, 57, -10, 20, "gfx/hud/force_tick6", NULL_HANDLE },
	{ 58, 55, -10, 10, "gfx/hud/force_tick5", NULL_HANDLE },
	{ 63, 52, -20, 10, "gfx/hud/force_tick4", NULL_HANDLE },
	{ 66, 49, -20, 10, "gfx/hud/force_tick3", NULL_HANDLE },
	{ 68, 45, -20, 10, "gfx/hud/force_tick2", NULL_HANDLE },
	{ 69, 41, -20, 10, "gfx/hud/force_tick1", NULL_HANDLE },		// Right top
};

forceTicPos_t ammoTicPos[] =
{
	{ 12, 34, 10, 10, "gfx/hud/ammo_tick7-l", NULL_HANDLE }, 	// Bottom
	{ 13, 28, 10, 10, "gfx/hud/ammo_tick6-l", NULL_HANDLE },
	{ 15, 23, 10, 10, "gfx/hud/ammo_tick5-l", NULL_HANDLE },
	{ 19, 19, 10, 10, "gfx/hud/ammo_tick4-l", NULL_HANDLE },
	{ 23, 15, 10, 10, "gfx/hud/ammo_tick3-l", NULL_HANDLE },
	{ 29, 12, 10, 10, "gfx/hud/ammo_tick2-l", NULL_HANDLE },
	{ 34, 11, 10, 10, "gfx/hud/ammo_tick1-l", NULL_HANDLE },

	{ 47, 11, -10, 10, "gfx/hud/ammo_tick1-r", NULL_HANDLE },
	{ 52, 12, -10, 10, "gfx/hud/ammo_tick2-r", NULL_HANDLE },
	{ 58, 15, -10, 10, "gfx/hud/ammo_tick3-r", NULL_HANDLE },
	{ 62, 19, -10, 10, "gfx/hud/ammo_tick4-r", NULL_HANDLE },
	{ 66, 23, -10, 10, "gfx/hud/ammo_tick5-r", NULL_HANDLE },
	{ 68, 28, -10, 10, "gfx/hud/ammo_tick6-r", NULL_HANDLE },
	{ 69, 34, -10, 10, "gfx/hud/ammo_tick7-r", NULL_HANDLE },
};


extern void NPC_Precache ( gentity_t *spawner );
qboolean NPCsPrecached = qfalse;
/*
=================
CG_PrepRefresh

Call before entering a new level, or after changing renderers
This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	static char		*sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};

	static char		*sb_c_nums[11] = {
		"gfx/2d/numbers/c_zero",
		"gfx/2d/numbers/c_one",
		"gfx/2d/numbers/c_two",
		"gfx/2d/numbers/c_three",
		"gfx/2d/numbers/c_four",
		"gfx/2d/numbers/c_five",
		"gfx/2d/numbers/c_six",
		"gfx/2d/numbers/c_seven",
		"gfx/2d/numbers/c_eight",
		"gfx/2d/numbers/c_nine",
		"gfx/2d/numbers/t_minus", //?????
	};

	// Clean, then register...rinse...repeat...
	CG_LoadingString( "effects" );
	FX_Init();
	CG_RegisterEffects();

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	cgi_R_ClearScene();

	cg.loadLCARSStage = 3;
	CG_LoadingString( cgs.mapname );

	cgi_R_LoadWorldMap( cgs.mapname );

	cg.loadLCARSStage = 4;
	CG_LoadingString( "game media shaders" );

	for ( i=0; i < 11; i++ )
	{
		cgs.media.numberShaders[i]			= cgi_R_RegisterShaderNoMip( sb_nums[i] );
		cgs.media.smallnumberShaders[i]		= cgi_R_RegisterShaderNoMip( sb_t_nums[i] );
		cgs.media.chunkyNumberShaders[i]	= cgi_R_RegisterShaderNoMip( sb_c_nums[i] );
	}

	// FIXME: conditionally do this??  Something must be wrong with inventory item caching..?
	cgi_R_RegisterModel( "models/items/remote.md3" );

	cgs.media.explosionModel				= cgi_R_RegisterModel ( "models/weaphits/explosion.md3" );
	cgs.media.surfaceExplosionShader		= cgi_R_RegisterShader( "surfaceExplosion" );

	cgs.media.solidWhiteShader			= cgi_R_RegisterShader( "gfx/effects/solidWhite" );

	//on players
	cgs.media.personalShieldShader		= cgi_R_RegisterShader( "gfx/misc/personalshield" );
	cgs.media.cloakedShader				= cgi_R_RegisterShader( "gfx/effects/cloakedShader" );
											cgi_R_RegisterShader( "gfx/misc/ion_shield" );

	cgs.media.boltShader			= cgi_R_RegisterShader( "gfx/misc/blueLine" );

	// FIXME: do these conditionally
	cgi_R_RegisterShader( "gfx/2d/workingCamera" );
	cgi_R_RegisterShader( "gfx/2d/brokenCamera" );
	cgi_R_RegisterShader( "gfx/effects/irid_shield" ); // for galak, but he doesn't have his own weapon so I can't register the shader there.

	//interface
	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = cgi_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a'+i) );
	}
	cgs.media.backTileShader		= cgi_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader			= cgi_R_RegisterShaderNoMip( "gfx/hud/noammo");
	cgs.media.weaponIconBackground	= cgi_R_RegisterShaderNoMip( "gfx/hud/background");
	cgs.media.weaponProngsOn		= cgi_R_RegisterShaderNoMip( "gfx/hud/prong_on_w");
	cgs.media.weaponProngsOff		= cgi_R_RegisterShaderNoMip( "gfx/hud/prong_off");
	cgs.media.forceProngsOn			= cgi_R_RegisterShaderNoMip( "gfx/hud/prong_on_f");
	cgs.media.forceIconBackground	= cgi_R_RegisterShaderNoMip( "gfx/hud/background_f");
	cgs.media.inventoryIconBackground= cgi_R_RegisterShaderNoMip( "gfx/hud/background_i");
	cgs.media.inventoryProngsOn		= cgi_R_RegisterShaderNoMip( "gfx/hud/prong_on_i");
	cgs.media.dataPadFrame			= cgi_R_RegisterShaderNoMip( "gfx/hud/datapad2");

	cg.loadLCARSStage = 5;
	CG_LoadingString( "game media models" );

	// Chunk models
	//FIXME: jfm:? bother to conditionally load these if an ent has this material type?
	for ( i = 0; i < NUM_CHUNK_MODELS; i++ )
	{
		cgs.media.chunkModels[CHUNK_METAL2][i]	= cgi_R_RegisterModel( va( "models/chunks/metal/metal1_%i.md3", i+1 ) ); //_ /switched\ _
		cgs.media.chunkModels[CHUNK_METAL1][i]	= cgi_R_RegisterModel( va( "models/chunks/metal/metal2_%i.md3", i+1 ) ); //  \switched/
		cgs.media.chunkModels[CHUNK_ROCK1][i]	= cgi_R_RegisterModel( va( "models/chunks/rock/rock1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK2][i]	= cgi_R_RegisterModel( va( "models/chunks/rock/rock2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK3][i]	= cgi_R_RegisterModel( va( "models/chunks/rock/rock3_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE1][i]	= cgi_R_RegisterModel( va( "models/chunks/crate/crate1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE2][i]	= cgi_R_RegisterModel( va( "models/chunks/crate/crate2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_WHITE_METAL][i]	= cgi_R_RegisterModel( va( "models/chunks/metal/wmetal1_%i.md3", i+1 ) );
	}

	cgs.media.chunkSound			= cgi_S_RegisterSound("sound/weapons/explosions/glasslcar");
	cgs.media.grateSound			= cgi_S_RegisterSound( "sound/effects/grate_destroy" );
	cgs.media.rockBreakSound		= cgi_S_RegisterSound("sound/effects/wall_smash");
	cgs.media.rockBounceSound[0]	= cgi_S_RegisterSound("sound/effects/stone_bounce");
	cgs.media.rockBounceSound[1]	= cgi_S_RegisterSound("sound/effects/stone_bounce2");
	cgs.media.metalBounceSound[0]	= cgi_S_RegisterSound("sound/effects/metal_bounce");
	cgs.media.metalBounceSound[1]	= cgi_S_RegisterSound("sound/effects/metal_bounce2");
	cgs.media.glassChunkSound		= cgi_S_RegisterSound("sound/weapons/explosions/glassbreak1");
	cgs.media.crateBreakSound[0]	= cgi_S_RegisterSound("sound/weapons/explosions/crateBust1" );
	cgs.media.crateBreakSound[1]	= cgi_S_RegisterSound("sound/weapons/explosions/crateBust2" );

	cgs.media.weaponbox	 = cgi_R_RegisterShaderNoMip( "gfx/interface/weapon_box");

	//Models & Shaders
	cgs.media.damageBlendBlobShader	= cgi_R_RegisterShader( "gfx/misc/borgeyeflare" );

	cg.loadLCARSStage = 6;

//	cgs.media.HUDLeftFrame= cgi_R_RegisterShaderNoMip( "gfx/hud/hudleftframe" );
	cgs.media.HUDLeftFrame= cgi_R_RegisterShaderNoMip( "gfx/hud/static_test" );
	cgs.media.HUDInnerLeft = cgi_R_RegisterShaderNoMip( "gfx/hud/hudleft_innerframe" );
	cgs.media.HUDArmor1= cgi_R_RegisterShaderNoMip( "gfx/hud/armor1" );
	cgs.media.HUDArmor2= cgi_R_RegisterShaderNoMip( "gfx/hud/armor2" );
	cgs.media.HUDHealth= cgi_R_RegisterShaderNoMip( "gfx/hud/health" );
	cgs.media.HUDHealthTic= cgi_R_RegisterShaderNoMip( "gfx/hud/health_tic" );
//	cgs.media.HUDArmorTic= cgi_R_RegisterShaderNoMip( "gfx/hud/armor_tic" );

	cgs.media.HUDRightFrame= cgi_R_RegisterShaderNoMip( "gfx/hud/hudrightframe" );
	cgs.media.HUDInnerRight = cgi_R_RegisterShaderNoMip( "gfx/hud/hudright_innerframe" );

	cgs.media.messageLitOn = cgi_R_RegisterShaderNoMip( "gfx/hud/message_on" );
	cgs.media.messageLitOff = cgi_R_RegisterShaderNoMip( "gfx/hud/message_off" );
	cgs.media.messageObjCircle = cgi_R_RegisterShaderNoMip( "gfx/hud/objective_circle" );

	cgs.media.DPForcePowerOverlay = cgi_R_RegisterShader( "gfx/hud/force_swirl" );

	// battery charge shader when using a gonk
	cgs.media.batteryChargeShader = cgi_R_RegisterShader( "gfx/2d/battery" );
	cgi_R_RegisterShader( "gfx/2d/droid_view" );

	// Load force tics
	for (i=0;i<MAX_TICS;i++)
	{
		forceTicPos[i].tic = cgi_R_RegisterShaderNoMip( forceTicPos[i].file );
		ammoTicPos[i].tic = cgi_R_RegisterShaderNoMip( ammoTicPos[i].file );
	}

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz( items, CG_ConfigString( CS_ITEMS ), sizeof(items) );

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' )
		{
			if (bg_itemlist[i].classname)
			{
				CG_LoadingString( bg_itemlist[i].classname );
				CG_RegisterItemVisuals( i );
			}
		}
		if (bg_itemlist[i].giType == IT_HOLDABLE)
		{
			if (bg_itemlist[i].giTag < INV_MAX)
			{
				inv_icons[bg_itemlist[i].giTag] = cgi_R_RegisterShaderNoMip( bg_itemlist[i].icon );
			}
		}
	}

	cgi_R_RegisterShader( "gfx/misc/test_crackle" );

	// wall marks
	cgs.media.phaserMarkShader				= cgi_R_RegisterShader( "gfx/damage/burnmark3" );
	cgs.media.scavMarkShader				= cgi_R_RegisterShader( "gfx/damage/burnmark4" );
//	cgs.media.bulletmarksShader				= cgi_R_RegisterShader( "textures/decals/bulletmark4" );
	cgs.media.rivetMarkShader				= cgi_R_RegisterShader( "gfx/damage/rivetmark" );

	// doing one shader just makes it look like a shell.  By using two shaders with different bulge offsets and different texture scales, it has a much more chaotic look
	cgs.media.electricBodyShader			= cgi_R_RegisterShader( "gfx/misc/electric" );
	cgs.media.electricBody2Shader			= cgi_R_RegisterShader( "gfx/misc/fullbodyelectric2" );

	cgs.media.shadowMarkShader	= cgi_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader	= cgi_R_RegisterShader( "wake" );
	cgi_S_RegisterSound( "sound/effects/energy_crackle.wav" );


	CG_LoadingString("map brushes");
	// register the inline models
	cgs.numInlineModels = cgi_CM_NumInlineModels();
	assert( cgs.numInlineModels < (int)ARRAY_LEN( cgs.inlineDrawModel ) );
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = cgi_R_RegisterModel( name );
		cgi_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	cg.loadLCARSStage = 7;
	CG_LoadingString("map models");
	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.model_draw[i] = cgi_R_RegisterModel( modelName );
//		OutputDebugString(va("### CG_RegisterGraphics(): cgs.model_draw[%d] = \"%s\"\n",i,modelName));
	}

	cg.loadLCARSStage = 8;

/*
Ghoul2 Insert Start
*/
	CG_LoadingString("skins");
	// register all the server specified models
	for (i=1 ; i<MAX_CHARSKINS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_CHARSKINS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.skins[i] = cgi_R_RegisterSkin( modelName );
	}

/*
Ghoul2 Insert End
*/

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] )
		{
			continue;
		}

		//feedback( va("client %i", i ) );
		CG_NewClientinfo( i );
	}

	for (i=0 ; i < ENTITYNUM_WORLD ; i++)
	{
		if(&g_entities[i])
		{
			if(g_entities[i].client)
			{
				//if(!g_entities[i].client->clientInfo.infoValid)
				//We presume this
				{
					CG_LoadingString( va("client %s", g_entities[i].client->clientInfo.name ) );
					CG_RegisterClientModels(i);
					if ( i != 0 )
					{//Client weapons already precached
						CG_RegisterWeapon( g_entities[i].client->ps.weapon );
						CG_RegisterNPCCustomSounds( &g_entities[i].client->clientInfo );
						CG_RegisterNPCEffects( g_entities[i].client->playerTeam );
					}
				}
			}
			else if ( g_entities[i].svFlags & SVF_NPC_PRECACHE && g_entities[i].NPC_type && g_entities[i].NPC_type[0] )
			{//Precache the NPC_type
				//FIXME: make sure we didn't precache this NPC_type already
				CG_LoadingString( va("NPC %s", g_entities[i].NPC_type ) );
				NPC_Precache( &g_entities[i] );
			}
		}
	}


	cg.loadLCARSStage = 9;

	NPCsPrecached = qtrue;

	extern	cvar_t	*com_buildScript;

	if (com_buildScript->integer) {
		cgi_R_RegisterShader( "gfx/misc/nav_cpoint" );
		cgi_R_RegisterShader( "gfx/misc/nav_line" );
		cgi_R_RegisterShader( "gfx/misc/nav_arrow" );
		cgi_R_RegisterShader( "gfx/misc/nav_node" );
	}
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

void CG_LinkCentsToGents(void)
{
	int	i;

	for(i = 0; i < MAX_GENTITIES; i++)
	{
		cg_entities[i].gent = &g_entities[i];
	}
}

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( qboolean bForceStart ) {
	const char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	COM_BeginParseSession();
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );
	COM_EndParseSession();

	cgi_S_StartBackgroundTrack( parm1, parm2, !bForceStart );
}

/*
======================
CG_GameStateReceived

Displays the info screen while loading media
======================
*/

int iCGResetCount=0;
qboolean qbVidRestartOccured = qfalse;

//===================
qboolean gbUseTheseValuesFromLoadSave = qfalse;	// MUST default to this
int gi_cg_forcepowerSelect;
int gi_cg_inventorySelect;
//===================


static void CG_GameStateReceived( void ) {
	// clear everything

	extern void CG_ClearAnimSndCache( void );
	CG_ClearAnimSndCache();	// else sound handles wrong after vid_restart

	qbVidRestartOccured = qtrue;
	iCGResetCount++;
	if (iCGResetCount == 1)	// this will only equal 1 first time, after each vid_restart it just gets higher.
	{						//	This non-clear is so the user can vid_restart during scrolling text without losing it.
		qbVidRestartOccured = qfalse;
	}

	if (!qbVidRestartOccured)
	{
/*
Ghoul2 Insert Start
*/

// this is a No-No now we have stl vector classes in here.
//		memset( &cg, 0, sizeof( cg ) );
		CG_Init_CG();
/*
Ghoul2 Insert End
*/

	}
/*
Ghoul2 Insert Start
*/

//	memset( cg_entities, 0, sizeof(cg_entities) );
	CG_Init_CGents();
/*
Ghoul2 Insert End
*/

	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	CG_LinkCentsToGents();

	cg.weaponSelect = WP_BRYAR_PISTOL;
	cg.forcepowerSelect = FP_HEAL;

	if (gbUseTheseValuesFromLoadSave)
	{
		gbUseTheseValuesFromLoadSave = qfalse;	// ack
		cg.forcepowerSelect = gi_cg_forcepowerSelect;
		cg.inventorySelect	= gi_cg_inventorySelect;
	}


	// get the rendering configuration from the client system
	cgi_GetGlconfig( &cgs.glconfig );

/*	cgs.charScale = cgs.glconfig.vidHeight * (1.0/480.0);
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		// wide screen
		cgs.bias = 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen
		cgs.bias = 0;
	}
*/
	// get the gamestate from the client system
	cgi_GetGameState( &cgs.gameState );

	CG_ParseServerinfo();

	// load the new map
	cgs.media.levelLoad = cgi_R_RegisterShaderNoMip( "gfx/hud/mp_levelload" );
	CG_LoadingString( "collision map" );

	cgi_CM_LoadMap( cgs.mapname, qfalse );

	CG_RegisterSounds();

	CG_RegisterGraphics();


	//jfm: moved down to preinit
//	CG_InitLocalEntities();
//	CG_InitMarkPolys();

	CG_StartMusic( qfalse );

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	CGCam_Init();

	CG_ClearLightStyles();

}

void CG_WriteTheEvilCGHackStuff(void)
{
	gi.AppendToSaveGame(INT_ID('F','P','S','L'), &cg.forcepowerSelect, sizeof(cg.forcepowerSelect));
	gi.AppendToSaveGame(INT_ID('I','V','S','L'), &cg.inventorySelect,  sizeof(cg.inventorySelect));

}
void CG_ReadTheEvilCGHackStuff(void)
{
	gi.ReadFromSaveGame(INT_ID('F','P','S','L'), (void *)&gi_cg_forcepowerSelect, sizeof(gi_cg_forcepowerSelect), NULL);
	gi.ReadFromSaveGame(INT_ID('I','V','S','L'), (void *)&gi_cg_inventorySelect,  sizeof(gi_cg_inventorySelect), NULL);
	gbUseTheseValuesFromLoadSave = qtrue;
}

/*
Ghoul2 Insert Start
*/

// initialise the cg_entities structure - take into account the ghoul2 stl stuff in the active snap shots
void CG_Init_CG(void)
{
	memset( &cg, 0, sizeof(cg));
}

// initialise the cg_entities structure - take into account the ghoul2 stl stuff
void CG_Init_CGents(void)
{
	memset( cg_entities, 0, sizeof(cg_entities) );
}

/*
Ghoul2 Insert End
*/


/*
=================
CG_PreInit

Called when DLL loads (after subsystem restart, but before gamestate is received)
=================
*/
void CG_PreInit() {
/*
Ghoul2 Insert Start
*/

// this is a No-No now we have stl vector classes in here.
//	memset( &cg, 0, sizeof( cg ) );
	CG_Init_CG();
/*
Ghoul2 Insert End
*/

	memset( &cgs, 0, sizeof( cgs ) );
	iCGResetCount = 0;

	CG_RegisterCvars();

//moved from CG_GameStateReceived because it's loaded sooner now
	CG_InitLocalEntities();

	CG_InitMarkPolys();
}

/*
=================
CG_Init

Called after every level change or subsystem restart
=================
*/
void CG_Init( int serverCommandSequence ) {
	cgs.serverCommandSequence = serverCommandSequence;

	cgi_Cvar_Set( "cg_drawHUD", "1" );

	// fonts...
	//
	cgs.media.charsetShader = cgi_R_RegisterShaderNoMip("gfx/2d/charsgrid_med");

	cgs.media.qhFontSmall = cgi_R_RegisterFont("ocr_a");
	cgs.media.qhFontMedium= cgi_R_RegisterFont("ergoec");

	cgs.media.whiteShader   = cgi_R_RegisterShader( "white" );
	cgs.media.loadTick		= cgi_R_RegisterShaderNoMip( "gfx/hud/load_tick" );
	cgs.media.loadTickCap	= cgi_R_RegisterShaderNoMip( "gfx/hud/load_tick_cap" );

	static char		*force_icon_files[NUM_FORCE_POWERS] =
	{
		"gfx/hud/f_icon_heal",
		"gfx/hud/f_icon_levitation",
		"gfx/hud/f_icon_speed",
		"gfx/hud/f_icon_push",
		"gfx/hud/f_icon_pull",
		"gfx/hud/f_icon_telepathy",
		"gfx/hud/f_icon_grip",
		"gfx/hud/f_icon_l1",
		"gfx/hud/f_icon_saber_throw",
		"gfx/hud/f_icon_saber_defend",
		"gfx/hud/f_icon_saber_attack",
	};

	// Precache inventory icons
	for ( int i=0;i<NUM_FORCE_POWERS;i++)
	{
		if (force_icon_files[i])
		{
			force_icons[i] = cgi_R_RegisterShaderNoMip( force_icon_files[i] );
		}
	}

	cgi_SP_Register("SP_INGAME", qtrue);	//require load and keep around
	cgi_SP_Register("OBJECTIVES", qtrue);	//require load and keep around

	CG_LoadHudMenu();      // load new hud stuff

	cg.loadLCARSStage		= 0;

	CG_GameStateReceived();

	CG_InitConsoleCommands();

	cg.missionInfoFlashTime = 0;
	cg.missionStatusShow = qfalse;

}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void )
{
	in_camera = false;
	FX_Free();
}

//// DEBUG STUFF
/*
-------------------------
CG_DrawNode
-------------------------
*/
void CG_DrawNode( vec3_t origin, int type )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();

	ex->leType = LE_SPRITE;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 51;
	VectorCopy( origin, ex->refEntity.origin );

	ex->refEntity.customShader = cgi_R_RegisterShader( "gfx/misc/nav_node" );

	float	scale = 16.0f;

	switch ( type )
	{
	case NODE_NORMAL:
		ex->color[0] = 255;
		ex->color[1] = 0;
		ex->color[2] = 0;
		break;

	case NODE_START:
		ex->color[0] = 0;
		ex->color[1] = 0;
		ex->color[2] = 255;
		scale += 16.0f;
		break;

	case NODE_GOAL:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 0;
		scale += 16.0f;
		break;

	case NODE_NAVGOAL:
		ex->color[0] = 255;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;
	}

	ex->radius = scale;
}

/*
-------------------------
CG_DrawRadius
-------------------------
*/

void CG_DrawRadius( vec3_t origin, unsigned int radius, int type )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();

	ex->leType = LE_QUAD;
	ex->radius = radius;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 51;
	VectorCopy( origin, ex->refEntity.origin );

	ex->refEntity.customShader = cgi_R_RegisterShader( "gfx/misc/nav_radius" );

	switch ( type )
	{
	case NODE_NORMAL:
		ex->color[0] = 255;
		ex->color[1] = 0;
		ex->color[2] = 0;
		break;

	case NODE_START:
		ex->color[0] = 0;
		ex->color[1] = 0;
		ex->color[2] = 255;
		break;

	case NODE_GOAL:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;

	case NODE_NAVGOAL:
		ex->color[0] = 255;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;
	}
}

/*
-------------------------
CG_DrawEdge
-------------------------
*/

void CG_DrawEdge( vec3_t start, vec3_t end, int type )
{
	switch ( type )
	{
	case EDGE_PATH:
		FX_AddLine( start, end, 4.0f, 4.0f, 0.0f, 1.0f, 1.0f, 51, cgi_R_RegisterShader( "gfx/misc/nav_arrow" ), 0 );
		break;

	case EDGE_NORMAL:
		FX_AddLine( start, end, 8.0f, 4.0f, 0.0f, 0.5f, 0.5f, 51, cgi_R_RegisterShader( "gfx/misc/nav_line" ), 0 );
		break;

	case EDGE_BLOCKED:
		{
			vec3_t	color = { 255, 255, 0 };

			FX_AddLine( start, end, 8.0f, 4.0f, 0.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader( "gfx/misc/nav_line" ), 0 );
		}
		break;
	case EDGE_FAILED:
		{
			vec3_t	color = { 255, 0, 0 };

			FX_AddLine( start, end, 8.0f, 4.0f, 0.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader( "gfx/misc/nav_line" ), 0 );
		}
		break;
	case EDGE_MOVEDIR:
		{
			vec3_t	color = { 0, 255, 0 };

			FX_AddLine( start, end, 8.0f, 4.0f, 0.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader( "gfx/misc/nav_line" ), 0 );
		}
		break;
	default:
		break;
	}
}

/*
-------------------------
CG_DrawCombatPoint
-------------------------
*/

void CG_DrawCombatPoint( vec3_t origin, int type )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();

	ex->leType = LE_SPRITE;
	ex->startTime = cg.time;
	ex->radius = 8;
	ex->endTime = ex->startTime + 51;
	VectorCopy( origin, ex->refEntity.origin );

	ex->refEntity.customShader = cgi_R_RegisterShader( "gfx/misc/nav_cpoint" );

	ex->color[0] = 0;
	ex->color[1] = 255;
	ex->color[2] = 255;

/*
	switch( type )
	{
	case 0:	//FIXME: To shut up the compiler warning (more will be added here later of course)
	default:
		FX_AddSprite( origin, NULL, NULL, 8.0f, 0.0f, 1.0f, 1.0f, color, color, 0.0f, 0.0f, 51, cgi_R_RegisterShader( "gfx/misc/nav_cpoint" ) );
		break;
	}
*/
}

/*
-------------------------
CG_DrawAlert
-------------------------
*/

void CG_DrawAlert( vec3_t origin, float rating )
{
	vec3_t	drawPos;

	VectorCopy( origin, drawPos );
	drawPos[2] += 48;

	vec3_t	startRGB;

	//Fades from green at 0, to red at 1
	startRGB[0] = rating;
	startRGB[1] = 1 - rating;
	startRGB[2] = 0;

	FX_AddSprite( drawPos, NULL, NULL, 16, 0.0f, 1.0f, 1.0f, startRGB, startRGB, 0, 0, 50, cgs.media.whiteShader );
}


#define MAX_MENUDEFFILE				4096

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(const char **p)
{
	const char *token;
	const char *tempStr;
	int pointSize;

	token = COM_ParseExt(p, qtrue);

	if (!token)
	{
		return qfalse;
	}

	if (Q_stricmp(token, "{") != 0)
	{
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt(p, qtrue);
		if (!token)
		{
			return qfalse;
		}

		if (Q_stricmp(token, "}") == 0)
		{
			return qtrue;
		}

		// font
		if (Q_stricmp(token, "font") == 0)
		{
/*
			int pointSize;

			cgi_UI_Parse_String(tempStr);
			cgi_UI_Parse_Int(&pointSize);

			if (!tempStr || !pointSize)
			{
				return qfalse;
			}
*/
//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token, "smallFont") == 0)
		{
			if (!COM_ParseString(p, &tempStr) || !COM_ParseInt(p, &pointSize))
			{
				return qfalse;
			}
//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token, "bigfont") == 0)
		{
			int pointSize;
			if (!COM_ParseString(p, &tempStr) || !COM_ParseInt(p, &pointSize))
			{
				return qfalse;
			}
//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token, "gradientbar") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
//			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token, "menuEnterSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
//			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token, "menuExitSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
//			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token, "itemFocusSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
//			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token, "menuBuzzSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
//			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		if (Q_stricmp(token, "cursor") == 0)
		{
//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
//			{
//				return qfalse;
//			}
//			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "fadeClamp") == 0)
		{
//			if (!COM_ParseFloat(p, &cgDC.Assets.fadeClamp))
//			{
//				return qfalse;
//			}
			continue;
		}

		if (Q_stricmp(token, "fadeCycle") == 0)
		{
//			if (!COM_ParseInt(p, &cgDC.Assets.fadeCycle))
//			{
//				return qfalse;
//			}
			continue;
		}

		if (Q_stricmp(token, "fadeAmount") == 0)
		{
//			if (!COM_ParseFloat(p, &cgDC.Assets.fadeAmount))
//			{
//				return qfalse;
//			}
			continue;
		}

		if (Q_stricmp(token, "shadowX") == 0)
		{
//			if (!COM_ParseFloat(p, &cgDC.Assets.shadowX))
//			{
//				return qfalse;
//			}
			continue;
		}

		if (Q_stricmp(token, "shadowY") == 0)
		{
//			if (!COM_ParseFloat(p, &cgDC.Assets.shadowY))
//			{
//				return qfalse;
//			}
			continue;
		}

		if (Q_stricmp(token, "shadowColor") == 0)
		{
			/*
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor))
			{
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			*/
			continue;
		}
	}
	return qfalse; // bk001204 - why not?
}

void cgi_UI_EndParseSession(char *buf);

/*
=================
CG_ParseMenu();
=================
*/
void CG_ParseMenu(const char *menuFile)
{
	char			*token;
	int				result;
	char			*buf,*p;

	Com_Printf("Parsing menu file: %s\n", menuFile);

	result = cgi_UI_StartParseSession((char *) menuFile,&buf);

	if (!result)
	{
		Com_Printf("Unable to load hud menu file: %s. Using default ui/testhud.menu.\n", menuFile);
		result = cgi_UI_StartParseSession("ui/testhud.menu",&buf);
		if (!result)
		{
			Com_Printf("Unable to load default ui/testhud.menu.\n");
			return;
		}
	}

	p = buf;
	while ( 1 )
	{
		cgi_UI_ParseExt(&token);

		if(!token)
		{
			// NULL checking is the best kind of checking --eez
			Com_Error(ERR_FATAL, "cgi_UI_ParseExt: NULL token parameter");
		}

		if (!*token)	// All done?
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

//		if ( *token == '}' )
//		{
//			break;
//		}

		if (Q_stricmp(token, "assetGlobalDef") == 0)
		{
			/*
			if (CG_Asset_Parse(handle))
			{
				continue;
			}
			else
			{
				break;
			}
			*/
		}


		if (Q_stricmp(token, "menudef") == 0)
		{
			// start a new menu
			cgi_UI_Menu_New(p);
		}
	}

	cgi_UI_EndParseSession(buf);

}

/*
=================
CG_Load_Menu();

=================
*/
qboolean CG_Load_Menu( const char **p)
{
	const char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{')
	{
		return qfalse;
	}

	while ( 1 )
	{

		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0)
		{
			return qtrue;
		}

		if ( !token || token[0] == 0 )
		{
			return qfalse;
		}

		CG_ParseMenu(token);
	}
	return qfalse;
}

/*
=================
CG_LoadMenus();

=================
*/
void CG_LoadMenus(const char *menuFile)
{
	const char	*token;
	const char	*p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = cgi_Milliseconds();

	len = cgi_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f )
	{
		//cgi_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );	// what. this would not run
		cgi_Printf( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );	// the rest at all.. --eez
		len = cgi_FS_FOpenFile( "ui/jk2hud.txt", &f, FS_READ );
		if (!f)
		{
			cgi_Error( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n", menuFile ) );
		}
	}

	if ( len >= MAX_MENUDEFFILE )
	{
		cgi_FS_FCloseFile( f );
		cgi_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE ) );
		return;
	}

	cgi_FS_Read( buf, len, f );
	buf[len] = 0;
	cgi_FS_FCloseFile( f );

//	COM_Compress(buf);

//	cgi_UI_Menu_Reset();

	p = buf;

	COM_BeginParseSession();
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}')
		{
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 )
		{
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0)
		{
			if (CG_Load_Menu(&p))
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}
	COM_EndParseSession();

	Com_Printf("UI menu load time = %d milli seconds\n", cgi_Milliseconds() - start);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu(void)
{
	const char *hudSet;
/*
	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	cgDC.Error = &Com_Error;
	cgDC.Print = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
*/
//	Init_Display(&cgDC);

//	cgi_UI_String_Init();

//	cgi_UI_Menu_Reset();

	hudSet = cg_hudFiles.string;
	if (hudSet[0] == '\0')
	{
		hudSet = "ui/jk2hud.txt";
	}

	CG_LoadMenus(hudSet);
}


/*
==============================================================================

INVENTORY SELECTION

==============================================================================
*/

/*
===============
CG_InventorySelectable
===============
*/
static qboolean CG_InventorySelectable( int index)
{
	if (cg.snap->ps.inventory[index])	// Is there any in the inventory?
	{
		return qtrue;
	}

	return qfalse;
}


/*
===============
SetInventoryTime
===============
*/
static void SetInventoryTime(void)
{
	if (((cg.weaponSelectTime + WEAPON_SELECT_TIME) > cg.time) ||	// The Weapon HUD was currently active to just swap it out with Force HUD
		((cg.forcepowerSelectTime + WEAPON_SELECT_TIME) > cg.time))	// The Force HUD was currently active to just swap it out with Force HUD
	{
		cg.weaponSelectTime = 0;
		cg.forcepowerSelectTime = 0;
		cg.inventorySelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.inventorySelectTime = cg.time;
	}
}

/*
===============
CG_DPPrevInventory_f
===============
*/
void CG_DPPrevInventory_f( void )
{
	int		i;

	if ( !cg.snap )
	{
		return;
	}

	const int original = cg.DataPadInventorySelect;

	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		cg.DataPadInventorySelect--;

		if ((cg.DataPadInventorySelect < INV_ELECTROBINOCULARS) || (cg.DataPadInventorySelect >= INV_MAX))
		{
			cg.DataPadInventorySelect = (INV_MAX - 1);
		}

		if ( CG_InventorySelectable( cg.DataPadInventorySelect ) )
		{
			return;
		}
	}

	cg.DataPadInventorySelect = original;
}
/*
===============
CG_DPNextInventory_f
===============
*/
void CG_DPNextInventory_f( void )
{
	int		i;

	if ( !cg.snap )
	{
		return;
	}

	const int original = cg.DataPadInventorySelect;

	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		cg.DataPadInventorySelect++;

		if ((cg.DataPadInventorySelect < INV_ELECTROBINOCULARS) || (cg.DataPadInventorySelect >= INV_MAX))
		{
			cg.DataPadInventorySelect = INV_ELECTROBINOCULARS;
		}

		if ( CG_InventorySelectable( cg.DataPadInventorySelect ) && (inv_icons[cg.DataPadInventorySelect]))
		{
			return;
		}
	}

	cg.DataPadInventorySelect = original;
}

/*
===============
CG_NextInventory_f
===============
*/
void CG_NextInventory_f( void )
{
	int		i;
	float	*color;

	if ( !cg.snap )
	{
		return;
	}

	// The first time it's been hit so just show inventory but don't advance in inventory.
	color = CG_FadeColor( cg.inventorySelectTime, WEAPON_SELECT_TIME );
	if ( !color )
	{
		SetInventoryTime();
		return;
	}

	const int original = cg.inventorySelect;

	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		cg.inventorySelect++;

		if ((cg.inventorySelect < INV_ELECTROBINOCULARS) || (cg.inventorySelect >= INV_MAX))
		{
			cg.inventorySelect = INV_ELECTROBINOCULARS;
		}

		if ( CG_InventorySelectable( cg.inventorySelect ) && (inv_icons[cg.inventorySelect]))
		{
			cgi_S_StartSound (NULL, 0, CHAN_AUTO, cgs.media.selectSound2 );
			SetInventoryTime();
			return;
		}
	}

	cg.inventorySelect = original;
}

/*
===============
CG_UseInventory_f
===============
*/
/*
this func was moved to Cmd_UseInventory_f in g_cmds.cpp
*/

/*
===============
CG_PrevInventory_f
===============
*/
void CG_PrevInventory_f( void )
{
	int		i;
	float	*color;

	if ( !cg.snap )
	{
		return;
	}

	// The first time it's been hit so just show inventory but don't advance in inventory.
	color = CG_FadeColor( cg.inventorySelectTime, WEAPON_SELECT_TIME );
	if ( !color )
	{
		SetInventoryTime();
		return;
	}

	const int original = cg.inventorySelect;

	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		cg.inventorySelect--;

		if ((cg.inventorySelect < INV_ELECTROBINOCULARS) || (cg.inventorySelect >= INV_MAX))
		{
			cg.inventorySelect = (INV_MAX - 1);
		}

		if ( CG_InventorySelectable( cg.inventorySelect ) && (inv_icons[cg.inventorySelect]))
		{
			cgi_S_StartSound (NULL, 0, CHAN_AUTO, cgs.media.selectSound2 );
			SetInventoryTime();
			return;
		}
	}

	cg.inventorySelect = original;
}


/*
===================
FindInventoryItemTag
===================
*/
gitem_t *FindInventoryItemTag(int tag)
{
	int		i;

/*	if (!Q_stricmp(tokenStr,"INV_ELECTROBINOCULARS")
	{
		tag = INV_ELECTROBINOCULARS;
	}
	else if (!Q_stricmp(tokenStr,"INV_BACTA_CANISTER")
	{
		tag = INV_BACTA_CANISTER;
	}
	else if (!Q_stricmp(tokenStr,"INV_SEEKER")
	{
		tag = INV_SEEKER;
	}
	else if (!Q_stricmp(tokenStr,"INV_LIGHTAMP_GOGGLES")
	{
		tag = INV_LIGHTAMP_GOGGLES;
	}
	else if (!Q_stricmp(tokenStr,"INV_SENTRY")
	{
		tag = INV_SENTRY;
	}
	else if (!Q_stricmp(tokenStr,"INV_GOODIE_KEY")
	{
		tag = INV_GOODIE_KEY;
	}
	else if (!Q_stricmp(tokenStr,"INV_SECURITY_KEY")
	{
		tag = INV_SECURITY_KEY;
	}
*/

	for ( i = 1 ; i < bg_numItems ; i++ )
	{
		if ( bg_itemlist[i].giTag == tag && bg_itemlist[i].giType == IT_HOLDABLE ) // I guess giTag's aren't unique amongst items..must also make sure it's a holdable
		{
			return &bg_itemlist[i];
		}
	}

	return (0);
}




/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInventorySelect( void )
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	int				smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	int				holdX,x,y,pad;
	//int				height;
//	int				tag;
	float			addX;
	vec4_t			textColor = { .312f, .75f, .621f, 1.0f };
	char			text[1024]={0};

	// don't display if dead
	if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 || ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD ))
	{
		return;
	}

	if ((cg.inventorySelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	int x2,y2;
	if (!cgi_UI_GetMenuInfo("inventoryselecthud",&x2,&y2))
	{
		return;
	}

	cg.iconSelectTime = cg.inventorySelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}

	if (!count)
	{
		cgi_SP_GetStringTextString("INGAME_EMPTY_INV",text, sizeof(text) );
		int w = cgi_R_Font_StrLenPixels( text, cgs.media.qhFontSmall, 1.0f );
		x = ( SCREEN_WIDTH - w ) / 2;
		CG_DrawProportionalString(x, y2 + 22, text, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
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

	i = cg.inventorySelect - 1;
	if (i<0)
	{
		i = INV_MAX-1;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	x = 320;
	y = 410;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = INV_MAX-1;
		}

		if ((!CG_InventorySelectable(i)) || (!inv_icons[i]))
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, inv_icons[i] );

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	if (inv_icons[cg.inventorySelect])
	{
		cgi_R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, inv_icons[cg.inventorySelect] );
		addX = (float) bigIconSize * .75;
		cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
		CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12,
			NUM_FONT_SMALL,qfalse);

		if (inv_names[cg.inventorySelect])
		{
			// FIXME: This is ONLY a temp solution, the icon stuff, etc, should all just use items.dat for everything
			gitem_t *item = FindInventoryItemTag( cg.inventorySelect );

			if ( item && item->classname && item->classname[0] )
			{
				char itemName[256], data[1024]; // FIXME: do these really need to be this large??  does it matter?

				sprintf( itemName, "INGAME_%s",	item->classname );

				if ( cgi_SP_GetStringTextString( itemName, data, sizeof( data )))
				{
					int w = cgi_R_Font_StrLenPixels( data, cgs.media.qhFontSmall, 1.0f );
					int x = ( SCREEN_WIDTH - w ) / 2;

					cgi_R_Font_DrawString( x, (SCREEN_HEIGHT - 24), data, textColor, cgs.media.qhFontSmall, -1, 1.0f);
				}
			}
//			if (tag)
//			{
//				CG_DrawProportionalString(320, y + 53, inv_names[cg.inventorySelect], CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
//				CG_DrawProportionalString(320, y + 53, bg_itemlist[i].pickup_name, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
//			}
		}
	}

	i = cg.inventorySelect + 1;
	if (i> INV_MAX-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> INV_MAX-1)
		{
			i = 0;
		}

		if ((!CG_InventorySelectable(i)) || (!inv_icons[i]))
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, inv_icons[i] );

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);

			holdX += (smallIconSize+pad);
		}
	}
}

int cgi_UI_GetItemText(char *menuFile,char *itemName, char *text);

char *inventoryDesc[15] =
{
"NEURO_SAAV_DESC",
"BACTA_DESC",
"INQUISITOR_DESC",
"LA_GOGGLES_DESC",
"PORTABLE_SENTRY_DESC",
"GOODIE_KEY_DESC",
"SECURITY_KEY_DP_DESC",
};


/*
===================
CG_DrawDataPadInventorySelect
===================
*/
void CG_DrawDataPadInventorySelect( void )
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	int				smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	int				holdX,x,y,pad;
	//int				height;
	float			addX;
	char			text[1024]={0};
	vec4_t			textColor = { .312f, .75f, .621f, 1.0f };


	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < INV_MAX ; i++ )
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}


	if (!count)
	{
		cgi_SP_GetStringTextString("INGAME_EMPTY_INV",text, sizeof(text) );
		int w = cgi_R_Font_StrLenPixels( text, cgs.media.qhFontSmall, 1.0f );
		x = ( SCREEN_WIDTH - w ) / 2;
		CG_DrawProportionalString(x, 300 + 22, text, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
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

//	char buffer[256];
//	cgi_UI_GetItemText("datapadInventoryMenu",va("invdesc%d",cg.DataPadInventorySelect+1),buffer);

	i = cg.DataPadInventorySelect - 1;
	if (i<0)
	{
		i = INV_MAX-1;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 8;

	x = 320;
	y = 300;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = INV_MAX-1;
		}

		if ((!CG_InventorySelectable(i)) || (!inv_icons[i]))
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, inv_icons[i] );

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	if (inv_icons[cg.DataPadInventorySelect])
	{
		cgi_R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, inv_icons[cg.DataPadInventorySelect] );
		addX = (float) bigIconSize * .75;
		cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
		CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.DataPadInventorySelect], 6, 12,
			NUM_FONT_SMALL,qfalse);

		if (inv_names[cg.DataPadInventorySelect])
		{
			// FIXME :this has to use the bg_itemlist pickup name
//			tag = FindInventoryItemTag(cg.inventorySelect);

//			if (tag)
//			{
//				CG_DrawProportionalString(320, y + 53, inv_names[cg.inventorySelect], CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
//				CG_DrawProportionalString(320, y + 53, bg_itemlist[i].pickup_name, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
//			}
		}
	}

	i = cg.DataPadInventorySelect + 1;
	if (i> INV_MAX-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	addX = (float) smallIconSize * .75;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> INV_MAX-1)
		{
			i = 0;
		}

		if ((!CG_InventorySelectable(i)) || (!inv_icons[i]))
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, inv_icons[i] );

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);

			holdX += (smallIconSize+pad);
		}
	}

	// draw the weapon description
	x= 40;
	y= 70;

	if ((cg.DataPadInventorySelect>=0) && (cg.DataPadInventorySelect<13))
	{
		cgi_SP_GetStringTextString( va("INGAME_%s",inventoryDesc[cg.DataPadInventorySelect]), text, sizeof(text) );

		if (text[0])
		{
			CG_DisplayBoxedText(70,50,500,300,text,
														cgs.media.qhFontSmall,
														0.7f,
														textColor
														);
		}
	}
}

/*
===============
SetForcePowerTime
===============
*/
void SetForcePowerTime(void)
{
	if (((cg.weaponSelectTime + WEAPON_SELECT_TIME) > cg.time) ||	// The Weapon HUD was currently active to just swap it out with Force HUD
		((cg.inventorySelectTime + WEAPON_SELECT_TIME) > cg.time))	// The Inventory HUD was currently active to just swap it out with Force HUD
	{
		cg.weaponSelectTime = 0;
		cg.inventorySelectTime = 0;
		cg.forcepowerSelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.forcepowerSelectTime = cg.time;
	}
}

int showPowers[MAX_SHOWPOWERS] =
{
	FP_HEAL,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_TELEPATHY,
	FP_GRIP,
	FP_LIGHTNING
};
char *showPowersName[MAX_SHOWPOWERS] =
{
	"HEAL2",
	"SPEED2",
	"PUSH2",
	"PULL2",
	"MINDTRICK2",
	"GRIP2",
	"LIGHTNING2",
};

int showDataPadPowers[MAX_DPSHOWPOWERS] =
{
	FP_HEAL,
	FP_LEVITATION,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_TELEPATHY,
	FP_GRIP,
	FP_LIGHTNING,
	FP_SABERTHROW,
	FP_SABER_DEFENSE,
	FP_SABER_OFFENSE,
};

/*char *showDataPadPowersName[MAX_DPSHOWPOWERS] =
{
	"HEAL2",
	"JUMP2",
	"SPEED2",
	"PUSH2",
	"PULL2",
	"MINDTRICK2",
	"GRIP2",
	"LIGHTNING2",
	"SABER_THROW2",
	"SABER_DEFENSE2",
	"SABER_OFFENSE2",
};
/*

/*
===============
ForcePower_Valid
===============
*/
qboolean ForcePower_Valid(int index)
{
	gentity_t	*player = &g_entities[0];

	assert (MAX_SHOWPOWERS == ( sizeof(showPowers)/sizeof(showPowers[0]) ));
	assert (index < MAX_SHOWPOWERS );	//is this a valid index?
	if (player->client->ps.forcePowersKnown & (1 << showPowers[index]) &&
		player->client->ps.forcePowerLevel[showPowers[index]])	// Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
CG_NextForcePower_f
===============
*/
void CG_NextForcePower_f( void )
{
	int		i;

	if ( !cg.snap )
	{
		return;
	}

	SetForcePowerTime();

	if ((cg.forcepowerSelectTime + WEAPON_SELECT_TIME) < cg.time)
	{
		return;
	}

	const int original = cg.forcepowerSelect;

	for ( i = 0; i < MAX_SHOWPOWERS; i++ )
	{
		cg.forcepowerSelect++;

		if (cg.forcepowerSelect >= MAX_SHOWPOWERS)
		{
			cg.forcepowerSelect = 0;
		}

		if (ForcePower_Valid(cg.forcepowerSelect))	// Does he have the force power?
		{
			cgi_S_StartSound (NULL, 0, CHAN_AUTO, cgs.media.selectSound2 );
			return;
		}
	}

	cg.forcepowerSelect = original;
}

/*
===============
CG_PrevForcePower_f
===============
*/
void CG_PrevForcePower_f( void )
{
	int		i;

	if ( !cg.snap )
	{
		return;
	}

	SetForcePowerTime();

	if ((cg.forcepowerSelectTime + WEAPON_SELECT_TIME) < cg.time)
	{
		return;
	}

	const int original = cg.forcepowerSelect;

	for ( i = 0; i < MAX_SHOWPOWERS; i++ )
	{
		cg.forcepowerSelect--;

		if (cg.forcepowerSelect < 0)
		{
			cg.forcepowerSelect = MAX_SHOWPOWERS - 1;
		}

		if (ForcePower_Valid(cg.forcepowerSelect))	// Does he have the force power?
		{
			cgi_S_StartSound (NULL, 0, CHAN_AUTO, cgs.media.selectSound2 );
			return;
		}
	}


	cg.forcepowerSelect = original;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect( void )
{
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX,x,y,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	char	text[1024]={0};


	// don't display if dead
	if ( cg.predicted_player_state.stats[STAT_HEALTH] <= 0 || ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD ))
	{
		return;
	}

	if ((cg.forcepowerSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0; i<MAX_SHOWPOWERS; ++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

/*
	int x2,y2;
	if (!cgi_UI_GetMenuInfo("forceselecthud",&x2,&y2))
	{
		return;
	}
*/
	cg.iconSelectTime = cg.forcepowerSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

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

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	x = 320;
	y = 425;

	i = cg.forcepowerSelect - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS-1;
	}

	cgi_R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS-1;
		}

		if (!ForcePower_Valid(i))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (force_icons[showPowers[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, force_icons[showPowers[i]] );
			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	if (force_icons[showPowers[cg.forcepowerSelect]])
	{
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)), bigIconSize, bigIconSize, force_icons[showPowers[cg.forcepowerSelect]] ); //only cache the icon for display
	}


	i = cg.forcepowerSelect + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(i))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (force_icons[showPowers[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, force_icons[showPowers[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad);
		}
	}

	// This only a temp solution.
	if (cgi_SP_GetStringTextString( va("INGAME_%s",showPowersName[cg.forcepowerSelect]), text, sizeof(text) ))
	{
			int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
			int x = ( SCREEN_WIDTH - w ) / 2;
			cgi_R_Font_DrawString(x, (SCREEN_HEIGHT - 24), text, colorTable[CT_ICON_BLUE], cgs.media.qhFontSmall, -1, 1.0f);
	}
}

/*
===============
ForcePowerDataPad_Valid
===============
*/
qboolean ForcePowerDataPad_Valid(int index)
{
	gentity_t	*player = &g_entities[0];

	assert (index < MAX_DPSHOWPOWERS);
	if (player->client->ps.forcePowersKnown & (1 << showDataPadPowers[index]) &&
		player->client->ps.forcePowerLevel[showDataPadPowers[index]])	// Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
CG_DPNextForcePower_f
===============
*/
void CG_DPNextForcePower_f( void )
{
	int		i;
	int		original;

	if ( !cg.snap )
	{
		return;
	}

	original = cg.DataPadforcepowerSelect;

	for ( i = 0; i<MAX_DPSHOWPOWERS; i++ )
	{
		cg.DataPadforcepowerSelect++;

		if (cg.DataPadforcepowerSelect >= MAX_DPSHOWPOWERS)
		{
			cg.DataPadforcepowerSelect = 0;
		}

		if (ForcePowerDataPad_Valid(cg.DataPadforcepowerSelect))	// Does he have the force power?
		{
			return;
		}
	}

	cg.DataPadforcepowerSelect = original;
}

/*
===============
CG_DPPrevForcePower_f
===============
*/
void CG_DPPrevForcePower_f( void )
{
	int		i;
	int		original;

	if ( !cg.snap )
	{
		return;
	}

	original = cg.DataPadforcepowerSelect;

	for ( i = 0; i<MAX_DPSHOWPOWERS; i++ )
	{
		cg.DataPadforcepowerSelect--;

		if (cg.DataPadforcepowerSelect < 0)
		{
			cg.DataPadforcepowerSelect = MAX_DPSHOWPOWERS-1;
		}

		if (ForcePowerDataPad_Valid(cg.DataPadforcepowerSelect))	// Does he have the force power?
		{
			return;
		}
	}


	cg.DataPadforcepowerSelect = original;
}

char *forcepowerDesc[NUM_FORCE_POWERS] =
{
"FORCE_HEAL_DESC",
"FORCE_JUMP_DESC",
"FORCE_SPEED_DESC",
"FORCE_PUSH_DESC",
"FORCE_PULL_DESC",
"FORCE_MIND_TRICK_DESC",
"FORCE_GRIP_DESC",
"FORCE_LIGHTNING_DESC",
"FORCE_SABER_THROW_DESC",
"FORCE_SABER_DEFENSE_DESC",
"FORCE_SABER_OFFENSE_DESC",
};

char *forcepowerLvl1Desc[NUM_FORCE_POWERS] =
{
"FORCE_HEAL_LVL1_DESC",
"FORCE_JUMP_LVL1_DESC",
"FORCE_SPEED_LVL1_DESC",
"FORCE_PUSH_LVL1_DESC",
"FORCE_PULL_LVL1_DESC",
"FORCE_MIND_TRICK_LVL1_DESC",
"FORCE_GRIP_LVL1_DESC",
"FORCE_LIGHTNING_LVL1_DESC",
"FORCE_SABER_THROW_LVL1_DESC",
"FORCE_SABER_DEFENSE_LVL1_DESC",
"FORCE_SABER_OFFENSE_LVL1_DESC",
};

char *forcepowerLvl2Desc[NUM_FORCE_POWERS] =
{
"FORCE_HEAL_LVL2_DESC",
"FORCE_JUMP_LVL2_DESC",
"FORCE_SPEED_LVL2_DESC",
"FORCE_PUSH_LVL2_DESC",
"FORCE_PULL_LVL2_DESC",
"FORCE_MIND_TRICK_LVL2_DESC",
"FORCE_GRIP_LVL2_DESC",
"FORCE_LIGHTNING_LVL2_DESC",
"FORCE_SABER_THROW_LVL2_DESC",
"FORCE_SABER_DEFENSE_LVL2_DESC",
"FORCE_SABER_OFFENSE_LVL2_DESC",
};

char *forcepowerLvl3Desc[NUM_FORCE_POWERS] =
{
"FORCE_HEAL_LVL3_DESC",
"FORCE_JUMP_LVL3_DESC",
"FORCE_SPEED_LVL3_DESC",
"FORCE_PUSH_LVL3_DESC",
"FORCE_PULL_LVL3_DESC",
"FORCE_MIND_TRICK_LVL3_DESC",
"FORCE_GRIP_LVL3_DESC",
"FORCE_LIGHTNING_LVL3_DESC",
"FORCE_SABER_THROW_LVL3_DESC",
"FORCE_SABER_DEFENSE_LVL3_DESC",
"FORCE_SABER_OFFENSE_LVL3_DESC",
};

/*
===================
CG_DrawDataPadForceSelect
===================
*/
void CG_DrawDataPadForceSelect( void )
{
  	gentity_t	*player = &g_entities[0];
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX,x,y,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	char	text[1024]={0};
	char	text2[1024]={0};

	// count the number of powers owned
	count = 0;

	for (i=0;i<MAX_DPSHOWPOWERS;++i)
	{
		if (ForcePowerDataPad_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}


	// Time to switch new icon colors
	cgi_R_SetColor(colorTable[CT_WHITE]);

	cg.iconSelectTime = cg.forcepowerSelectTime;

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


	smallIconSize = 30;
	bigIconSize = 60;
	pad = 8;

	x = 320;
	y = 310;

	i = cg.DataPadforcepowerSelect - 1;
	if (i < 0)
	{
		i = MAX_DPSHOWPOWERS-1;
	}

	cgi_R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_DPSHOWPOWERS-1;
		}

		if (!ForcePowerDataPad_Valid(i))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (force_icons[showDataPadPowers[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, force_icons[showDataPadPowers[i]] );
		}

		// A new force power
		if (((cg_updatedDataPadForcePower1.integer - 1) == showDataPadPowers[i]) ||
			((cg_updatedDataPadForcePower2.integer - 1) == showDataPadPowers[i]) ||
			((cg_updatedDataPadForcePower3.integer - 1) == showDataPadPowers[i]))
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.DPForcePowerOverlay );
		}

		if (force_icons[showDataPadPowers[i]])
		{
			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	if (force_icons[showDataPadPowers[cg.DataPadforcepowerSelect]])
	{

		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)), bigIconSize, bigIconSize, force_icons[showDataPadPowers[cg.DataPadforcepowerSelect]] ); //only cache the icon for display

		// New force power
		if (((cg_updatedDataPadForcePower1.integer - 1) == showDataPadPowers[cg.DataPadforcepowerSelect]) ||
			((cg_updatedDataPadForcePower2.integer - 1) == showDataPadPowers[cg.DataPadforcepowerSelect]) ||
			((cg_updatedDataPadForcePower3.integer - 1) == showDataPadPowers[cg.DataPadforcepowerSelect]))
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)), bigIconSize, bigIconSize, cgs.media.DPForcePowerOverlay );
		}
	}


	i = cg.DataPadforcepowerSelect + 1;
	if (i>=MAX_DPSHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_DPSHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePowerDataPad_Valid(i))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (force_icons[showDataPadPowers[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, force_icons[showDataPadPowers[i]] ); //only cache the icon for display
		}

		// A new force power
		if (((cg_updatedDataPadForcePower1.integer - 1) == showDataPadPowers[i]) ||
			((cg_updatedDataPadForcePower2.integer - 1) == showDataPadPowers[i]) ||
			((cg_updatedDataPadForcePower3.integer - 1) == showDataPadPowers[i]))
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.DPForcePowerOverlay ); //only cache the icon for display
		}

		if (force_icons[showDataPadPowers[i]])
		{
			holdX += (smallIconSize+pad);
		}
	}

	cgi_SP_GetStringTextString( va("INGAME_%s",forcepowerDesc[cg.DataPadforcepowerSelect]), text, sizeof(text) );

	if (player->client->ps.forcePowerLevel[cg.DataPadforcepowerSelect]==1)
	{
		cgi_SP_GetStringTextString( va("INGAME_%s",forcepowerLvl1Desc[cg.DataPadforcepowerSelect]), text2, sizeof(text2) );
	}
	else if (player->client->ps.forcePowerLevel[cg.DataPadforcepowerSelect]==2)
	{
		cgi_SP_GetStringTextString( va("INGAME_%s",forcepowerLvl2Desc[cg.DataPadforcepowerSelect]), text2, sizeof(text2) );
	}
	else
	{
		cgi_SP_GetStringTextString( va("INGAME_%s",forcepowerLvl3Desc[cg.DataPadforcepowerSelect]), text2, sizeof(text2) );
	}

	if (text[0])
	{

		CG_DisplayBoxedText(70,50,500,300,va("%s%s",text,text2),
													cgs.media.qhFontSmall,
													0.7f,
													colorTable[CT_ICON_BLUE]
													);
	}
}

// actually, these are pretty pointless so far in CHC, since in TA codebase they were used only so init some HUD
//	function ptrs to allow cinematics in onscreen displays. So far, we don't use those, but here they are anyway...
//
/*	These stupid pragmas don't work, they still give the warning. Forget it, REM the lot.

#pragma warning ( disable : 4505)		// unreferenced local function has been removed
static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}
#pragma warning ( default : 4505)
*/




